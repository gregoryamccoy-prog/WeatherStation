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
    char    sunrise[6];           // "HH:MM\0"
    char    sunset[6];
    char    moonPhase[16];        // e.g. "Wax Gibb"
    float   moonPhaseFraction;    // 0.0 (new) → 0.5 (full) → 1.0 (new)
    bool    valid;
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
    bool              apiOk;       // false → show offline warning
    time_t            lastUpdate;  // unix timestamp of last successful API call
};
