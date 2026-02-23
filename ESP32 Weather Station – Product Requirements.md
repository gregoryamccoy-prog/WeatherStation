# ESP32 Weather Station – Product Requirements Document (PRD)

## 1. Product Overview

### 1.1 Objective

Design a low-power ESP32-based indoor/outdoor weather station that:

- Measures indoor temperature and humidity using a DHT11 sensor
- Retrieves outdoor weather and a 3-day forecast from the Open-Meteo API (no API key)
- Displays sunrise, sunset, and lunar phase
- Stores long-term trend data in NVS flash (365 days)
- Operates on a 10-minute deep-sleep cycle
- Renders all data onto a Waveshare 4.2" 400 × 300 e-paper display

### 1.2 Hardware

| Component | Detail |
|-----------|--------|
| MCU | ESP32 WROOM (esp32dev) |
| Display | Waveshare 4.2" 400 × 300 B/W e-paper, Rev 2.1 (GDEW042T2) |
| Sensor | DHT11 (temperature + humidity) |
| Build platform | PlatformIO, Arduino framework |

### 1.3 Location

Crestwood, Kentucky (38.3242, -85.4725). Timezone: America/New_York. Units: Metric.

---

## 2. Weather Data Provider

### 2.1 Selected Service: Open-Meteo

- **No API key required** — eliminates TLS token exchange overhead and rate-limit concerns for a single device.
- Base URL: `https://api.open-meteo.com/v1/forecast`

### 2.2 API Request

```
https://api.open-meteo.com/v1/forecast
  ?latitude=38.3242&longitude=-85.4725
  &current=temperature_2m,relative_humidity_2m,pressure_msl,wind_speed_10m,weather_code
  &daily=weather_code,temperature_2m_max,temperature_2m_min,
         precipitation_probability_max,sunrise,sunset
  &timezone=auto
  &forecast_days=3
```

A single HTTPS request returns:
- Current conditions (temperature, humidity, pressure, wind, WMO weather code)
- 3-day daily forecast (min/max temp, rain probability, sunrise, sunset)

**Moon phase** is not requested from the API; it is calculated locally using a synodic-period algorithm.

### 2.3 Response Handling

- HTTPS via `WiFiClientSecure` with `setInsecure()` (no CA bundle on device).
- Full response body read via `http.getString()` to correctly handle chunked transfer encoding.
- Parsed with ArduinoJson v7 (`JsonDocument`, heap-allocated, ~3–5 KB payload).

---

## 3. Weather Code → Icon Mapping

Open-Meteo uses WMO numeric `weather_code` values. The firmware maps these to vector icons drawn with Adafruit GFX primitives (48 × 48 px, no bitmaps):

| WMO Code | Icon | Description |
|----------|------|-------------|
| 0 | Sun | Clear sky |
| 1–3 | Sun + cloud | Partly cloudy |
| 45–48 | Dashed lines | Fog |
| 51–67 | Cloud + drops | Rain |
| 71–77 | Cloud + flakes | Snow |
| 80–82 | Outline cloud + drops | Showers |
| 95–99 | Cloud + bolt | Thunderstorm |
| Other | Circle + ? | Unknown |

---

## 4. Data Model

### 4.1 Current Conditions

```cpp
struct CurrentConditions {
    float   outdoorTemp;     // °C
    float   pressure;        // hPa
    float   windSpeed;       // km/h
    int     humidity;        // %
    uint8_t weatherCode;     // WMO code
    bool    valid;
};
```

### 4.2 Forecast

```cpp
struct ForecastDay {
    float   minTemp;         // °C
    float   maxTemp;         // °C
    int     rainChance;      // %
    uint8_t weatherCode;
    char    sunrise[6];      // "HH:MM\0"
    char    sunset[6];
    char    moonPhase[16];   // e.g. "Wax Cresc"
    bool    valid;
};
```

### 4.3 Trend Record (NVS)

```cpp
struct DailyTrend {
    uint16_t dayIndex;       // days since epoch
    float    indoorTemp;
    float    indoorHumidity;
    float    outdoorTemp;
    float    pressure;
};
```

- Retention: 365 records in a circular NVS buffer
- Written once per calendar day (only when both sensor and API data are valid)

### 4.4 Indoor Sensor Reading

```cpp
struct SensorReading {
    float temperature;    // °C
    float humidity;       // %
    bool  valid;
};
```

### 4.5 Aggregate Snapshot

```cpp
struct WeatherSnapshot {
    CurrentConditions current;
    ForecastDay       forecast[3];
    SensorReading     indoor;
    DailyTrend        graph[30];   // last ≤30 records for trend graph
    uint8_t           graphCount;
    bool              apiOk;
    time_t            lastUpdate;
};
```

---

## 5. Power and Runtime Model

- **Wake interval:** 10 minutes (configurable via `SLEEP_DURATION_US`)
- **Sleep current:** ~10 µA (ESP32 deep sleep)
- **Target active time:** ≤ 15 seconds

### Active Sequence

1. Init e-paper hardware (before Wi-Fi to avoid heap fragmentation)
2. Connect Wi-Fi (20 s timeout)
3. NTP sync (once per calendar day, tracked via RTC-retained day-of-year)
4. Read DHT11 (up to 3 retries, 2 s between attempts)
5. HTTPS GET Open-Meteo API → `getString()` → `deserializeJson()`
6. Write NVS trend record (once per calendar day)
7. Render framebuffer → full e-paper refresh
8. Deep sleep

### RTC-Retained State

Preserved across deep-sleep cycles:
- Last successful `CurrentConditions` and `ForecastDay[3]`
- Timestamp of last API success
- Day-of-year of last NTP sync and NVS write
- Wake counter

---

## 6. Display Layout

**Resolution:** 400 × 300 px, 1-bit monochrome  
**Driver:** GxEPD2_BW with GxEPD2_420 (page-buffered rendering)  
**Rotation:** 0° (connector at top)

### Layout Zones

| Zone | Y start | Height | Content |
|------|---------|--------|---------|
| Top bar | 0 | 22 px | Time (left), date (centre), Online/Offline (right) |
| Main panels | 22 | 133 px | Left (100 px): indoor. Centre (200 px): outdoor temp + icon. Right (100 px): wind, pressure, humidity |
| Forecast row | 155 | 62 px | 3 columns (133 px each): TODAY, TMRW, +2 DAY with max/min and rain % |
| Astronomy bar | 217 | 20 px | Sunrise, sunset, moon phase label |
| Trend graph | 237 | 63 px | 30-day indoor (solid) + outdoor (dotted) line graph, auto-scaled |

### Fonts

- **FreeSansBold 18pt:** Outdoor temperature (centre panel)
- **FreeSansBold 12pt:** Indoor temperature
- **FreeSansBold 9pt:** Top bar, forecast day labels, panel values
- **FreeSans 9pt:** Labels, descriptions, forecast data, astronomy bar
- **Built-in 6×8:** Trend graph legend

### Moon Phase

Computed locally via synodic-period algorithm (reference new moon: 2000-01-06 18:14 UTC, period: 29.53059 days). Labels: New, Wax Cresc, 1stQtr, Wax Gibb, Full, Wan Gibb, LastQtr, Wan Cresc.

---

## 7. Memory Budget

### 7.1 JSON Document

- Open-Meteo payload (filtered fields): ~3–5 KB
- ArduinoJson v7 `JsonDocument` (heap-allocated, auto-sized)

### 7.2 Framebuffer

- 400 × 300 monochrome: ~15 KB
- GxEPD2 uses page-buffered rendering (not full framebuffer)

### 7.3 Stack

- Arduino loop task stack: **16 KB** (`-DARDUINO_LOOP_STACK_SIZE=16384`)
- Required by GxEPD2 page-write + Adafruit GFX font rendering + `WeatherSnapshot` on stack

### 7.4 Overall

- RAM usage: ~19% (62 KB / 328 KB)
- Flash usage: ~49% (953 KB / 1966 KB)

---

## 8. Flash Storage Strategy

- **Daily write only** — one NVS record per calendar day
- NVS circular buffer: 365 records × 18 bytes ≈ 6.4 KB
- Partition scheme: `min_spiffs.csv` (maximises NVS headroom)
- Flash lifetime: multi-decade at one write/day

---

## 9. Connectivity Behaviour

If API call or Wi-Fi fails:
- Use cached weather data from RTC memory
- Show "! Offline – cached data" overlay + "Offline" in top bar
- Still display live indoor sensor readings
- Continue normal sleep cycle

---

## 10. Firmware Modules

| Module | Directory | Responsibility |
|--------|-----------|----------------|
| `wifi_manager` | `src/wifi_manager/` | Connect/disconnect with configurable timeout |
| `ntp_service` | `src/ntp_service/` | SNTP sync, once per calendar day |
| `openmeteo_client` | `src/openmeteo_client/` | HTTPS fetch, JSON parse, moon phase calc |
| `sensor_service` | `src/sensor_service/` | DHT11 read with 3-retry logic |
| `trend_storage` | `src/trend_storage/` | NVS circular buffer (365 records) |
| `display_renderer` | `src/display_renderer/` | GxEPD2 layout, fonts, vector icons |
| `power_manager` | `src/power_manager/` | Deep sleep entry |

---

## 11. Performance Targets

| Metric | Target |
|--------|--------|
| Wi-Fi connect | < 4 s |
| API call + parse | < 3 s |
| Render + refresh | < 4 s |
| **Total active time** | **≤ 15 s** |
| Sleep current | ~10 µA |
| Wake interval | 10 min |

---

## 12. Build Environment

- **PlatformIO** with Arduino framework
- Board: `esp32dev`
- Partition: `min_spiffs.csv`
- Stack: 16 KB loop task
- Libraries: GxEPD2 ^1.6.0, ArduinoJson ^7.0.0, DHT sensor library ^1.4.6, Adafruit Unified Sensor ^1.1.14

---

## 13. Acceptance Criteria

The system shall:

1. Operate with no API key (Open-Meteo)
2. Wake every 10 minutes from deep sleep
3. Display indoor temperature and humidity from DHT11
4. Display outdoor conditions (temp, wind, pressure, humidity, weather icon)
5. Display 3-day forecast (min/max, rain probability)
6. Display sunrise, sunset, and computed moon phase
7. Maintain 365-day trend history in NVS
8. Render 30-day dual-line trend graph (indoor + outdoor)
9. Enter deep sleep automatically after each refresh
10. Recover gracefully from Wi-Fi and API failures using cached data
11. Complete full wake cycle in ≤ 15 seconds
12. Fit within ESP32 RAM (~19%) and flash (~49%) budgets
