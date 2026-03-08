// main.cpp — Wake / sleep orchestration for ESP32 Weather Station
//
// Wake sequence:
//   Power on → WiFi → NTP (once/day) → DHT11 → Open-Meteo API →
//   Parse → Render → EPD refresh → Deep sleep 10 min

#include <Arduino.h>
#include <time.h>

#include "config.h"
#include "data_model.h"
#include "secrets.h"

#include "wifi_manager/wifi_manager.h"
#include "ntp_service/ntp_service.h"
#include "openmeteo_client/openmeteo_client.h"
#include "sensor_service/sensor_service.h"
#include "display_renderer/display_renderer.h"
#include "power_manager/power_manager.h"

// ---------------------------------------------------------------------------
// RTC-retained state (survives deep sleep)
// ---------------------------------------------------------------------------
RTC_DATA_ATTR static bool            rtc_initialized         = false;
RTC_DATA_ATTR static CurrentConditions rtc_lastCurrent       = {};
RTC_DATA_ATTR static ForecastDay     rtc_lastForecast[3]     = {};
RTC_DATA_ATTR static time_t          rtc_lastApiSuccess      = 0;
RTC_DATA_ATTR static int             rtc_lastNtpDay          = -1;    // day-of-year of last NTP sync
RTC_DATA_ATTR static uint32_t        rtc_wakeCount           = 0;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static int dayOfYear(const struct tm* t) {
    return t->tm_yday;
}

// ---------------------------------------------------------------------------
// setup() — runs once per wake from deep sleep
// ---------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(200);   // allow USB-serial bridge to enumerate before first print
    Serial.printf("\n[main] Wake #%u\n", rtc_wakeCount++);

    // -----------------------------------------------------------------------
    // 0. Initialise display hardware FIRST — grabs GxEPD2 heap/DMA allocations
    //    before WiFi stack fragments the heap.
    // -----------------------------------------------------------------------
    DisplayRenderer::init();

    // -----------------------------------------------------------------------
    // 1. Connect to Wi-Fi
    // -----------------------------------------------------------------------
    bool wifiOk = WifiManager::connect(WIFI_SSID, WIFI_PASSWORD, WIFI_CONNECT_TIMEOUT);
    if (!wifiOk) {
        Serial.println("[main] Wi-Fi failed – rendering cached data");
        // Still render whatever is cached so the display doesn't go blank
        WeatherSnapshot snap = {};
        snap.apiOk           = false;
        snap.indoor          = SensorService::read();
        snap.current         = rtc_lastCurrent;
        snap.lastUpdate      = rtc_lastApiSuccess;
        for (int i = 0; i < 3; i++) snap.forecast[i] = rtc_lastForecast[i];
        DisplayRenderer::render(snap);
        PowerManager::deepSleep(SLEEP_DURATION_US);
        return;
    }

    // -----------------------------------------------------------------------
    // 2. NTP sync (once per calendar day)
    // -----------------------------------------------------------------------
    struct tm timeinfo = {};
    bool timeValid     = NtpService::getTime(timeinfo);

    if (!timeValid || dayOfYear(&timeinfo) != rtc_lastNtpDay) {
        Serial.println("[main] Syncing NTP...");
        NtpService::sync();
        if (NtpService::getTime(timeinfo)) {
            rtc_lastNtpDay = dayOfYear(&timeinfo);
            Serial.printf("[main] NTP synced – day %d\n", rtc_lastNtpDay);
        }
    }

    // -----------------------------------------------------------------------
    // 3. Read DHT11
    // -----------------------------------------------------------------------
    SensorReading indoor = SensorService::read();
    Serial.printf("[main] Indoor %.1f°C  %.0f%%RH  valid=%d\n",
                  indoor.temperature, indoor.humidity, indoor.valid);

    // -----------------------------------------------------------------------
    // 4. Fetch Open-Meteo
    // -----------------------------------------------------------------------
    CurrentConditions current   = {};
    ForecastDay       forecast[3] = {};
    bool apiOk = OpenMeteoClient::fetch(current, forecast);

    if (apiOk) {
        rtc_lastCurrent   = current;
        for (int i = 0; i < 3; i++) rtc_lastForecast[i] = forecast[i];
        rtc_lastApiSuccess = time(nullptr);
        Serial.printf("[main] API OK – %.1f°C wmo=%d\n",
                      current.outdoorTemp, current.weatherCode);
    } else {
        Serial.println("[main] API failed – using cached outdoor data");
        current = rtc_lastCurrent;
        for (int i = 0; i < 3; i++) forecast[i] = rtc_lastForecast[i];
    }

    // -----------------------------------------------------------------------
    // 5. Disconnect Wi-Fi (saves ~70 mA during render)
    // -----------------------------------------------------------------------
    WifiManager::disconnect();

    // -----------------------------------------------------------------------
    // 6. Assemble snapshot and render
    // -----------------------------------------------------------------------
    WeatherSnapshot snap = {};
    snap.current     = current;
    snap.indoor      = indoor;
    snap.apiOk       = apiOk;
    snap.lastUpdate  = rtc_lastApiSuccess;
    for (int i = 0; i < 3; i++) snap.forecast[i] = forecast[i];

    DisplayRenderer::render(snap);

    // -----------------------------------------------------------------------
    // 7. Deep sleep
    // -----------------------------------------------------------------------
    Serial.printf("[main] Entering deep sleep for %llu s\n",
                  SLEEP_DURATION_US / 1000000ULL);
    PowerManager::deepSleep(SLEEP_DURATION_US);
}

// loop() is never reached — device wakes fresh each cycle via deep sleep reset
void loop() {}
