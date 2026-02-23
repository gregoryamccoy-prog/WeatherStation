// data_model.h — Shared data structures used across all modules
#pragma once

#include <stdint.h>
#include <time.h>

// ---------------------------------------------------------------------------
// Current outdoor conditions (from Open-Meteo "current" block)
// ---------------------------------------------------------------------------
struct CurrentConditions {
    float   outdoorTemp;     // °C
    float   pressure;        // hPa
    float   windSpeed;       // km/h
    int     humidity;        // %
    uint8_t weatherCode;     // WMO code
    bool    valid;           // false when API call failed and no cache exists
};

// ---------------------------------------------------------------------------
// Three-day forecast
// ---------------------------------------------------------------------------
struct ForecastDay {
    float   minTemp;         // °C
    float   maxTemp;         // °C
    int     rainChance;      // %
    uint8_t weatherCode;
    char    sunrise[6];      // "HH:MM\0"
    char    sunset[6];
    char    moonPhase[16];   // e.g. "waxing_crescent"
    bool    valid;
};

// ---------------------------------------------------------------------------
// NVS daily trend record  (packed to minimise flash writes)
// ---------------------------------------------------------------------------
struct DailyTrend {
    uint16_t dayIndex;         // days since epoch (wraps ~179 years)
    float    indoorTemp;
    float    indoorHumidity;
    float    outdoorTemp;
    float    pressure;
};

// ---------------------------------------------------------------------------
// Indoor sensor reading
// ---------------------------------------------------------------------------
struct SensorReading {
    float temperature;    // °C
    float humidity;       // %
    bool  valid;
};

// ---------------------------------------------------------------------------
// Aggregate snapshot passed to the display renderer
// ---------------------------------------------------------------------------
struct WeatherSnapshot {
    CurrentConditions current;
    ForecastDay       forecast[3];
    SensorReading     indoor;
    DailyTrend        graph[30];   // last ≤30 records for trend graph
    uint8_t           graphCount;  // actual populated entries (0–30)
    bool              apiOk;       // false → show offline warning
    time_t            lastUpdate;  // unix timestamp of last successful API call
};
