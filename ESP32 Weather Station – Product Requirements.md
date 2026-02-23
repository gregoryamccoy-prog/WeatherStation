ESP32 Weather Station – Product Requirements Document (PRD)
1. Product Overview
1.1 Objective

Target ESP32 WROOM.
Buildplatform PlatforIO.
Create a file structure typical and consistent with standard C++ projects.
JSON and API reference at https://www.weatherapi.com/docs/

Sample datalayout for project at https://www.makerguides.com/wp-content/uploads/2024/07/image-96-1024x632.png

Design a low-power ESP32-based indoor weather station that:

Measures indoor temperature and humidity using DHT11

Retrieves outdoor weather and a 3-day forecast

Displays sunrise, sunset, and lunar phase

Stores long-term trend data

Operates on a 10-minute deep sleep cycle

Uses a free weather API with no authentication

Display: Waveshare 4.2" 400×300 SPI e-paper (Rev 2.1)
Location: Crestwood, Kentucky (38.3242, -85.4725)
Units: Metric
Time: NTP

2. Weather Data Provider (No API Key)
2.1 Selected Service: Open-Meteo

Base URL:

https://api.open-meteo.com/v1/forecast
2.2 Request
latitude=38.3242
&longitude=-85.4725
&current=temperature_2m,relative_humidity_2m,pressure_msl,wind_speed_10m,weather_code
&daily=temperature_2m_max,temperature_2m_min,precipitation_probability_max,sunrise,sunset,moonrise,moonset,moon_phase
&timezone=auto
&forecast_days=3

Single request returns:

Current conditions

3-day forecast

Astronomical data

This minimizes Wi-Fi on-time.

3. Weather Code → Icon Mapping

Open-Meteo uses numeric weather_code.

Firmware shall include a lookup table:

0 → Clear
1–3 → Partly cloudy
45–48 → Fog
51–67 → Rain
71–77 → Snow
80–82 → Showers
95–99 → Thunderstorm

Mapped to local bitmap icons stored in flash.

4. Data Model
4.1 Current Conditions
struct CurrentConditions {
  float outdoorTemp;
  float pressure;
  float windSpeed;
  int humidity;
  uint8_t weatherCode;
};
4.2 Forecast
struct ForecastDay {
  float minTemp;
  float maxTemp;
  int rainChance;
  uint8_t weatherCode;
  char sunrise[6];
  char sunset[6];
  char moonPhase[8];
};
4.3 Trend Record (NVS)
struct DailyTrend {
  uint16_t dayIndex;
  float indoorTemp;
  float indoorHumidity;
  float outdoorTemp;
  float pressure;
};

Retention: 365 days

5. Power and Runtime Model

Wake interval: 10 minutes

Active sequence:

Wake

Connect Wi-Fi

NTP sync (conditional)

Read DHT11

Call Open-Meteo

Parse JSON

Update daily trend (once per day)

Render framebuffer

Full e-paper refresh

Deep sleep

Target active time: ≤ 15 seconds

6. Display Layout
6.1 Top Bar

Time

Date

Wi-Fi status

6.2 Left Panel

Indoor:

Temperature

Humidity

6.3 Center Panel

Large outdoor temperature

Weather icon

6.4 Right Panel

Wind speed

Pressure

Rain probability

6.5 Forecast Row

3 columns:

Day label

Icon

Min / Max

Rain %

6.6 Bottom Bar

Sunrise / Sunset

Moon phase

6.7 Trend Graph

Last 30 days:

Indoor temperature

Outdoor temperature

Line graph, auto-scaled.

7. Memory Budget
7.1 JSON Document

Open-Meteo payload (filtered fields):

~3–5 KB

Safe ArduinoJson allocation:

DynamicJsonDocument doc(8192);
7.2 Framebuffer

400 × 300 monochrome:

≈ 15 KB

Fits comfortably in ESP32 RAM.

8. Flash Storage Strategy

Daily write only.

Estimated NVS usage:

~6 KB for 365 records.

Flash lifetime: multi-decade

9. Connectivity Behavior

If API call fails:

Use cached weather data

Show warning icon

Continue normal sleep cycle

10. Firmware Modules

wifi_manager

ntp_service

openmeteo_client

sensor_service

trend_storage

display_renderer

power_manager

11. Performance Targets
Metric	Target
Wi-Fi connect	< 4 s
API call + parse	< 3 s
Render	< 2 s
Display refresh	< 4 s
Total active time	≤ 15 s
12. Build Environment

PlatformIO
Arduino framework

13. Acceptance Criteria

The system shall:

Operate with no API key

Wake every 10 minutes

Display indoor conditions

Display 3-day forecast

Display astronomy data

Maintain 365-day trend history

Render 30-day graph

Enter deep sleep automatically

Recover from Wi-Fi failure

Engineering Reality Check (the useful kind)

Open-Meteo is a strong choice for an embedded node because:

No TLS token exchange overhead

Predictable JSON

One request per cycle

No rate-limit anxiety for a single device

You’ve effectively reduced both power consumption and firmware complexity by removing authenticated APIs — which is exactly the sort of trade a long-lived wall device should make.

Next Iteration (if you want the PRD to become directly compilable)

The next pass can add:

Exact Arduino / ESP-IDF library stack

Concrete NVS partition table

Icon asset resolution and storage layout

Graph rendering algorithm (integer-only for speed)

Deep sleep drift compensation math

That’s the step where this stops being documentation and turns into a deterministic build plan.