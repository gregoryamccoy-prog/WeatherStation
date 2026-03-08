// config.h — Global pin assignments, API constants, and tuneable parameters
#pragma once

#include <cstdint>

// ---------------------------------------------------------------------------
// Hardware pins  (Waveshare ESP32 driver board defaults)
// ---------------------------------------------------------------------------
// E-paper SPI
static constexpr uint8_t PIN_EPD_MOSI  = 23;
static constexpr uint8_t PIN_EPD_SCK   = 18;
static constexpr uint8_t PIN_EPD_CS    = 5;
static constexpr uint8_t PIN_EPD_DC    = 16;
static constexpr uint8_t PIN_EPD_RST   = 17;
static constexpr uint8_t PIN_EPD_BUSY  = 25;

// DHT11 sensor
static constexpr uint8_t PIN_DHT       = 26;
static constexpr uint8_t DHT_TYPE      = 11;   // DHT11

// ---------------------------------------------------------------------------
// Location & units
// ---------------------------------------------------------------------------
static constexpr double LOCATION_LAT  =  38.3242;
static constexpr double LOCATION_LON  = -85.4725;
static constexpr char   LOCATION_TZ[] = "America/New_York";   // used in API request
static constexpr char   LOCATION_NAME[] = "Crestwood, KY";

// ---------------------------------------------------------------------------
// Open-Meteo REST endpoint  (no API key required)
// ---------------------------------------------------------------------------
static constexpr char OPENMETEO_HOST[] = "api.open-meteo.com";
static constexpr char OPENMETEO_URL[]  =
    "https://api.open-meteo.com/v1/forecast"
    "?latitude=38.3242&longitude=-85.4725"
    "&current=temperature_2m,relative_humidity_2m,pressure_msl,wind_speed_10m,weather_code"
    "&daily=weather_code,temperature_2m_max,temperature_2m_min"
    ",precipitation_probability_max,sunrise,sunset"
    "&timezone=auto"
    "&forecast_days=3";

// ---------------------------------------------------------------------------
// NTP
// ---------------------------------------------------------------------------
static constexpr char NTP_SERVER1[]    = "pool.ntp.org";
static constexpr char NTP_SERVER2[]    = "time.nist.gov";
static constexpr long GMT_OFFSET_SEC   = -18000;   // UTC-5 (EST); DST handled by TZ env
static constexpr int  DAYLIGHT_OFFSET  = 3600;     // +1 h during EDT

// ---------------------------------------------------------------------------
// Power management
// ---------------------------------------------------------------------------
static constexpr uint64_t SLEEP_DURATION_US = 10ULL * 60ULL * 1000000ULL;  // 10 minutes

// ---------------------------------------------------------------------------
// Display geometry (Waveshare 4.2" 400×300 Rev 2.1)
// ---------------------------------------------------------------------------
static constexpr uint16_t EPD_WIDTH  = 400;
static constexpr uint16_t EPD_HEIGHT = 300;

// Layout zones (x, y, w, h)
static constexpr uint16_t TOPBAR_H          = 24;
static constexpr uint16_t FORECAST_ROW_Y    = 200;
static constexpr uint16_t FORECAST_ROW_H    = 58;
static constexpr uint16_t MAIN_PANEL_H      = FORECAST_ROW_Y - TOPBAR_H;

// Left / centre / right panel widths
static constexpr uint16_t LEFT_PANEL_W      = 110;
static constexpr uint16_t RIGHT_PANEL_W     = 110;
static constexpr uint16_t CENTRE_PANEL_W    = EPD_WIDTH - LEFT_PANEL_W - RIGHT_PANEL_W;

// ---------------------------------------------------------------------------
// Application constants
// ---------------------------------------------------------------------------
static constexpr uint8_t  WIFI_CONNECT_TIMEOUT  = 20;    // seconds
static constexpr uint8_t  DHT_READ_RETRIES      = 3;
static constexpr uint16_t JSON_DOC_SIZE         = 8192;

// ---------------------------------------------------------------------------
// Icon dimensions
// ---------------------------------------------------------------------------
static constexpr uint8_t ICON_SIZE = 48;   // 48×48 px, 1-bit
