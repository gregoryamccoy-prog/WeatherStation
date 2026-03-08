// openmeteo_client.cpp
// Fetches weather + forecast from Open-Meteo (no API key).
// Uses WiFiClientSecure with setInsecure() — appropriate for a sealed
// single-device wall appliance that has no user-facing key material to protect.

#include "openmeteo_client.h"
#include "../config.h"
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <string.h>
#include <math.h>
#include <time.h>

namespace OpenMeteoClient {

// ---------------------------------------------------------------------------
// Moon-phase: compute fraction (0–1) from a unix timestamp, then label it.
// Reference new moon: 2000-01-06 18:14 UTC (947182440 s)
// Verified: 946684800 (Jan 1 2000 00:00 UTC) + 5×86400 + 18×3600 + 14×60 = 947182440
// Synodic period: 29.53058770576 days
// ---------------------------------------------------------------------------
static float moonPhaseForTime(time_t t) {
    const double NEW_MOON_REF  = 947182440.0;   // unix seconds
    const double SYNODIC_DAYS  = 29.53058770576;
    double days  = (static_cast<double>(t) - NEW_MOON_REF) / 86400.0;
    double phase = fmod(days / SYNODIC_DAYS, 1.0);
    if (phase < 0.0) phase += 1.0;
    return static_cast<float>(phase);
}

static void moonPhaseLabel(float phase, char* buf, size_t bufLen) {
    if      (phase < 0.03f || phase > 0.97f) strncpy(buf, "New",      bufLen);
    else if (phase < 0.22f)                  strncpy(buf, "Wax Cresc",bufLen);
    else if (phase < 0.28f)                  strncpy(buf, "1stQtr",   bufLen);
    else if (phase < 0.47f)                  strncpy(buf, "Wax Gibb", bufLen);
    else if (phase < 0.53f)                  strncpy(buf, "Full",     bufLen);
    else if (phase < 0.72f)                  strncpy(buf, "Wan Gibb", bufLen);
    else if (phase < 0.78f)                  strncpy(buf, "LastQtr",  bufLen);
    else                                     strncpy(buf, "Wan Cresc",bufLen);
    buf[bufLen - 1] = '\0';
}

// ---------------------------------------------------------------------------
// Extract HH:MM from an ISO 8601 datetime string "YYYY-MM-DDTHH:MM"
// ---------------------------------------------------------------------------
static void extractTime(const char* iso, char* out, size_t outLen) {
    if (!iso || strlen(iso) < 16) {
        strncpy(out, "--:--", outLen);
        return;
    }
    // Characters at position 11..15 are "HH:MM"
    strncpy(out, iso + 11, 5);
    out[5] = '\0';
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
bool fetch(CurrentConditions& current, ForecastDay forecast[3]) {
    WiFiClientSecure client;
    client.setInsecure();   // No CA bundle on device — skip cert verification

    HTTPClient http;
    http.setTimeout(8000);   // 8 s total (generous for slow API responses)

    Serial.println("[OpenMeteo] Starting request...");
    if (!http.begin(client, OPENMETEO_URL)) {
        Serial.println("[OpenMeteo] http.begin() failed");
        return false;
    }

    int httpCode = http.GET();
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("[OpenMeteo] HTTP %d\n", httpCode);
        http.end();
        return false;
    }

    // Read the full response body — getString() handles chunked transfer
    // encoding correctly, whereas getStreamPtr() on HTTPS often does not.
    // Typical payload is 2–4 KB, well within ESP32 free heap.
    String payload = http.getString();
    http.end();

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);

    if (err) {
        Serial.printf("[OpenMeteo] JSON parse error: %s\n", err.c_str());
        return false;
    }

    // -----------------------------------------------------------------------
    // Current conditions
    // -----------------------------------------------------------------------
    JsonObject cur = doc["current"];
    if (cur.isNull()) {
        Serial.println("[OpenMeteo] Missing 'current' key");
        return false;
    }

    current.outdoorTemp  = cur["temperature_2m"]    | 0.0f;
    current.humidity     = cur["relative_humidity_2m"] | 0;
    current.pressure     = cur["pressure_msl"]      | 0.0f;
    current.windSpeed    = cur["wind_speed_10m"]    | 0.0f;
    current.weatherCode  = static_cast<uint8_t>(cur["weather_code"].as<int>());
    current.valid        = true;

    // -----------------------------------------------------------------------
    // 3-day daily forecast
    // -----------------------------------------------------------------------
    JsonObject daily = doc["daily"];
    if (daily.isNull()) {
        Serial.println("[OpenMeteo] Missing 'daily' key");
        return false;
    }

    JsonArray maxTemps    = daily["temperature_2m_max"];
    JsonArray minTemps    = daily["temperature_2m_min"];
    JsonArray rainChances = daily["precipitation_probability_max"];
    JsonArray wCodes      = daily["weather_code"];
    JsonArray sunrises    = daily["sunrise"];
    JsonArray sunsets     = daily["sunset"];

    // Moon phase is not in the Open-Meteo response; calculate locally from
    // the forecast day's date (today + i days offset from current time).
    time_t now = time(nullptr);

    for (int i = 0; i < 3; i++) {
        ForecastDay& fd = forecast[i];

        fd.maxTemp    = maxTemps[i]    | 0.0f;
        fd.minTemp    = minTemps[i]    | 0.0f;
        fd.rainChance = rainChances[i] | 0;
        fd.weatherCode = static_cast<uint8_t>(wCodes[i].as<int>());

        extractTime(sunrises[i] | "", fd.sunrise, sizeof(fd.sunrise));
        extractTime(sunsets[i]  | "", fd.sunset,  sizeof(fd.sunset));

        float mp = moonPhaseForTime(now + (time_t)i * 86400);
        moonPhaseLabel(mp, fd.moonPhase, sizeof(fd.moonPhase));
        fd.moonPhaseFraction = mp;

        fd.valid = true;
    }

    Serial.printf("[OpenMeteo] Parsed OK: %.1f°C, WMO=%d, wind=%.1f km/h\n",
                  current.outdoorTemp, current.weatherCode, current.windSpeed);
    return true;
}

} // namespace OpenMeteoClient
