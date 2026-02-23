// display_renderer.cpp
//
// Full e-paper layout for Waveshare 4.2" 400×300 (Rev 2.1, GDEW042T2)
// using GxEPD2_BW and Adafruit GFX fonts.
//
// Layout (400 × 300 px):
//   y=  0 │ Top bar (22 px): time · date · Wi-Fi status
//   y= 22 │ Main panels (133 px):
//          │   Left (100 w): indoor temp + humidity
//          │   Centre (200 w): large outdoor temp + 48×48 weather icon
//          │   Right (100 w): wind · pressure · rain%
//   y=155 │ Forecast row (62 px): 3 × (day, mini-icon, max/min, rain%)
//   y=217 │ Astronomy bar (20 px): sunrise · sunset · moon phase
//   y=237 │ Trend graph (63 px): indoor + outdoor 30-day line graph

#include "display_renderer.h"
#include "icon_renderer.h"
#include "../config.h"

#include <Arduino.h>
#include <SPI.h>
#include <GxEPD2_BW.h>

// Waveshare 4.2" 400×300 Rev 2.1 (GDEW042T2)
// If linker reports missing symbol, try GxEPD2_420_GDEY042T81 instead.
#include <epd/GxEPD2_420.h>

// Adafruit GFX fonts
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>

// ---------------------------------------------------------------------------
// Display object (file-static so it is constructed once at program start)
// ---------------------------------------------------------------------------
static GxEPD2_BW<GxEPD2_420, GxEPD2_420::HEIGHT>
    display(GxEPD2_420(PIN_EPD_CS, PIN_EPD_DC, PIN_EPD_RST, PIN_EPD_BUSY));

// ---------------------------------------------------------------------------
// Colour aliases  (GxEPD2 uses these names)
// ---------------------------------------------------------------------------
static constexpr uint16_t BLK = GxEPD_BLACK;
static constexpr uint16_t WHT = GxEPD_WHITE;

// ---------------------------------------------------------------------------
// Layout constants (local — more granular than config.h)
// ---------------------------------------------------------------------------
static constexpr int16_t TOPBAR_Y   = 0;
static constexpr int16_t TOPBAR_H2  = 22;

static constexpr int16_t MAIN_Y     = TOPBAR_H2;
static constexpr int16_t MAIN_H     = 133;

static constexpr int16_t LEFT_X     = 0;
static constexpr int16_t LEFT_W     = 100;

static constexpr int16_t CTR_X      = LEFT_W;
static constexpr int16_t CTR_W      = 200;

static constexpr int16_t RIGHT_X    = CTR_X + CTR_W;
static constexpr int16_t RIGHT_W    = 100;

static constexpr int16_t FCST_Y     = MAIN_Y + MAIN_H;    // 155
static constexpr int16_t FCST_H     = 62;
static constexpr int16_t FCST_COL_W = EPD_WIDTH / 3;      // 133

static constexpr int16_t ASTRO_Y    = FCST_Y + FCST_H;    // 217
static constexpr int16_t ASTRO_H    = 20;

static constexpr int16_t GRAPH_Y2   = ASTRO_Y + ASTRO_H;  // 237
static constexpr int16_t GRAPH_H2   = EPD_HEIGHT - GRAPH_Y2; // 63

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

// Centre-align text horizontally within a bounding box
static void printCentred(const char* str, int16_t boxX, int16_t boxW, int16_t y) {
    int16_t  x1, y1;
    uint16_t tw, th;
    display.getTextBounds(str, 0, y, &x1, &y1, &tw, &th);
    int16_t tx = boxX + (boxW - (int16_t)tw) / 2;
    display.setCursor(tx, y);
    display.print(str);
}

// Right-align text within a bounding box
static void printRight(const char* str, int16_t boxX, int16_t boxW, int16_t y) {
    int16_t  x1, y1;
    uint16_t tw, th;
    display.getTextBounds(str, 0, y, &x1, &y1, &tw, &th);
    int16_t tx = boxX + boxW - (int16_t)tw - 2;
    display.setCursor(tx, y);
    display.print(str);
}

// Thin horizontal divider
static void hline(int16_t y) {
    display.drawFastHLine(0, y, EPD_WIDTH, BLK);
}

// Thin vertical divider
static void vline(int16_t x, int16_t y, int16_t h) {
    display.drawFastVLine(x, y, h, BLK);
}

// ---------------------------------------------------------------------------
// Section renderers
// ---------------------------------------------------------------------------

static void drawTopBar(const WeatherSnapshot& snap) {
    struct tm t = {};
    // Use current wall-clock time for the top bar; fall back to lastUpdate
    // only if NTP hasn't run yet (time() > some sane threshold).
    time_t now = time(nullptr);
    if (now < 1000000000L) now = snap.lastUpdate;  // NTP not yet synced
    localtime_r(&now, &t);

    char timeBuf[9], dateBuf[20];
    strftime(timeBuf, sizeof(timeBuf), "%H:%M",      &t);
    strftime(dateBuf, sizeof(dateBuf), "%a %b %d",   &t);

    display.setFont(&FreeSansBold9pt7b);
    display.setTextColor(BLK);

    // Time left
    display.setCursor(4, TOPBAR_H2 - 4);
    display.print(timeBuf);

    // Date centre
    printCentred(dateBuf, 0, EPD_WIDTH, TOPBAR_H2 - 4);

    // Wi-Fi / offline status right
    printRight(snap.apiOk ? "Online" : "Offline", 0, EPD_WIDTH - 3, TOPBAR_H2 - 4);

    hline(TOPBAR_H2);
}

static void drawIndoorPanel(const SensorReading& indoor) {
    const int16_t midX = LEFT_X + LEFT_W / 2;
    display.setFont(&FreeSans9pt7b);
    display.setTextColor(BLK);

    // Label
    printCentred("INDOOR", LEFT_X, LEFT_W, MAIN_Y + 16);

    display.setFont(&FreeSansBold12pt7b);
    char buf[12];

    if (indoor.valid) {
        snprintf(buf, sizeof(buf), "%.1f\xB0""C", indoor.temperature);
    } else {
        strncpy(buf, "--.-\xB0""C", sizeof(buf));
    }
    printCentred(buf, LEFT_X, LEFT_W, MAIN_Y + 44);

    display.setFont(&FreeSans9pt7b);
    if (indoor.valid) {
        snprintf(buf, sizeof(buf), "%.0f%% RH", indoor.humidity);
    } else {
        strncpy(buf, "--% RH", sizeof(buf));
    }
    printCentred(buf, LEFT_X, LEFT_W, MAIN_Y + 64);

    // Horizontal bar separating temp from humidity (visual aid)
    display.drawFastHLine(LEFT_X + 8, MAIN_Y + 70, LEFT_W - 16, BLK);

    vline(LEFT_X + LEFT_W, MAIN_Y, MAIN_H);
}

static void drawCentrePanel(const CurrentConditions& cur) {
    // Large temperature
    display.setFont(&FreeSansBold18pt7b);
    display.setTextColor(BLK);

    char tempBuf[12];
    if (cur.valid) {
        snprintf(tempBuf, sizeof(tempBuf), "%.1f\xB0", cur.outdoorTemp);
    } else {
        strncpy(tempBuf, "--.-\xB0", sizeof(tempBuf));
    }
    printCentred(tempBuf, CTR_X, CTR_W, MAIN_Y + 34);

    // Weather description under temp
    display.setFont(&FreeSans9pt7b);
    const char* desc = "Unknown";
    uint8_t wc = cur.weatherCode;
    if      (wc == 0)                   desc = "Clear";
    else if (wc >= 1  && wc <= 3)       desc = "Partly Cloudy";
    else if (wc >= 45 && wc <= 48)      desc = "Foggy";
    else if (wc >= 51 && wc <= 67)      desc = "Rainy";
    else if (wc >= 71 && wc <= 77)      desc = "Snowy";
    else if (wc >= 80 && wc <= 82)      desc = "Showers";
    else if (wc >= 95 && wc <= 99)      desc = "Thunderstorm";
    printCentred(desc, CTR_X, CTR_W, MAIN_Y + 52);

    // Weather icon — 48×48 centred in lower half of centre panel
    IconRenderer::draw(display,
                       CTR_X + (CTR_W - ICON_SIZE) / 2,
                       MAIN_Y + 60,
                       cur.valid ? cur.weatherCode : 255,
                       BLK);

    // Safety: icon renderers may change textSize/font; reset for later sections
    display.setTextSize(1);
}

static void drawRightPanel(const CurrentConditions& cur) {
    vline(RIGHT_X, MAIN_Y, MAIN_H);

    display.setFont(&FreeSans9pt7b);
    display.setTextColor(BLK);

    const int16_t ly = MAIN_Y + 16;
    char buf[16];

    // Wind
    printCentred("WIND", RIGHT_X, RIGHT_W, ly);
    display.setFont(&FreeSansBold9pt7b);
    if (cur.valid) snprintf(buf, sizeof(buf), "%.0f km/h", cur.windSpeed);
    else strncpy(buf, "-- km/h", sizeof(buf));
    printCentred(buf, RIGHT_X, RIGHT_W, ly + 20);

    display.drawFastHLine(RIGHT_X + 6, ly + 25, RIGHT_W - 12, BLK);

    // Pressure
    display.setFont(&FreeSans9pt7b);
    printCentred("PRESSURE", RIGHT_X, RIGHT_W, ly + 42);
    display.setFont(&FreeSansBold9pt7b);
    if (cur.valid) snprintf(buf, sizeof(buf), "%.0f hPa", cur.pressure);
    else strncpy(buf, "---- hPa", sizeof(buf));
    printCentred(buf, RIGHT_X, RIGHT_W, ly + 58);

    display.drawFastHLine(RIGHT_X + 6, ly + 63, RIGHT_W - 12, BLK);

    // Humidity (outdoor)
    display.setFont(&FreeSans9pt7b);
    printCentred("HUMIDITY", RIGHT_X, RIGHT_W, ly + 78);
    display.setFont(&FreeSansBold9pt7b);
    if (cur.valid) snprintf(buf, sizeof(buf), "%d%%", cur.humidity);
    else strncpy(buf, "--%", sizeof(buf));
    printCentred(buf, RIGHT_X, RIGHT_W, ly + 94);
}

static void drawForecastRow(const ForecastDay forecast[3]) {
    hline(FCST_Y);

    // Day labels  (62 px zone — 48 px icons do NOT fit; use text-only layout)
    // Line 1 (bold) : day name          — baseline at FCST_Y + 15
    // Line 2        : max / min         — baseline at FCST_Y + 35
    // Line 3        : rain probability  — baseline at FCST_Y + 53
    static const char* DAY_LABELS[3] = { "TODAY", "TMRW", "+2 DAY" };

    for (int i = 0; i < 3; i++) {
        int16_t fx = i * FCST_COL_W;
        const ForecastDay& f = forecast[i];

        if (i > 0) vline(fx, FCST_Y, FCST_H);

        // Day label — bold
        display.setFont(&FreeSansBold9pt7b);
        display.setTextColor(BLK);
        printCentred(DAY_LABELS[i], fx, FCST_COL_W, FCST_Y + 15);

        char buf[20];

        // Max / min temperature
        display.setFont(&FreeSans9pt7b);
        if (f.valid) {
            snprintf(buf, sizeof(buf), "%.0f / %.0f\xB0""C", f.maxTemp, f.minTemp);
        } else {
            strncpy(buf, "-- / --", sizeof(buf));
        }
        printCentred(buf, fx, FCST_COL_W, FCST_Y + 35);

        // Rain chance
        if (f.valid) snprintf(buf, sizeof(buf), "Rain %d%%", f.rainChance);
        else         strncpy(buf, "Rain --%", sizeof(buf));
        printCentred(buf, fx, FCST_COL_W, FCST_Y + 53);
    }
    hline(FCST_Y + FCST_H);
}

static void drawAstronomyBar(const ForecastDay forecast[3]) {
    hline(ASTRO_Y);

    display.setFont(&FreeSans9pt7b);
    display.setTextColor(BLK);

    char buf[64];
    const ForecastDay& today = forecast[0];
    if (today.valid) {
        snprintf(buf, sizeof(buf), "Rise %s   Set %s   %s",
                 today.sunrise, today.sunset, today.moonPhase);
    } else {
        strncpy(buf, "Rise --:--   Set --:--   Moon --", sizeof(buf));
    }
    printCentred(buf, 0, EPD_WIDTH, ASTRO_Y + ASTRO_H - 4);
}

// Integer-only trend graph — draws indoor (solid) and outdoor (dotted) lines
static void drawTrendGraph(const DailyTrend data[], uint8_t count) {
    if (count < 2) {
        display.setFont(&FreeSans9pt7b);
        display.setTextColor(BLK);
        printCentred("No trend data yet", 0, EPD_WIDTH, GRAPH_Y2 + GRAPH_H2 / 2 + 6);
        return;
    }

    // Find temperature range across both indoor and outdoor
    int16_t tMin = (int16_t)(data[0].indoorTemp * 10);
    int16_t tMax = tMin;
    for (uint8_t i = 0; i < count; i++) {
        int16_t iv = (int16_t)(data[i].indoorTemp  * 10);
        int16_t ov = (int16_t)(data[i].outdoorTemp * 10);
        if (iv < tMin) tMin = iv;
        if (ov < tMin) tMin = ov;
        if (iv > tMax) tMax = iv;
        if (ov > tMax) tMax = ov;
    }

    // Add 5° (×10) headroom; prevent divide-by-zero
    tMin -= 10;
    tMax += 10;
    int16_t tRange = tMax - tMin;
    if (tRange <= 0) tRange = 1;

    // Graph drawing area (inset 2 px each side)
    const int16_t gx0  = 4;
    const int16_t gy0  = GRAPH_Y2 + 2;
    const int16_t gw   = EPD_WIDTH - 8;
    const int16_t gh   = GRAPH_H2 - 4;

    // Y-axis label (small font)
    display.setFont(nullptr);   // built-in 6×8 font
    display.setTextSize(1);
    display.setTextColor(BLK);
    display.setCursor(gx0, gy0);
    display.print("30d");

    // Column step (fixed-point: x per data point)
    int32_t stepFp = ((int32_t)gw << 8) / (count - 1);   // Q8 fixed point

    auto tempToY = [&](float t) -> int16_t {
        int16_t tv = (int16_t)(t * 10);
        // map [tMin..tMax] → [gy0+gh..gy0] (bottom=hot, top=cold — invert)
        return gy0 + gh - (int16_t)(((int32_t)(tv - tMin) * gh) / tRange);
    };

    // Plot lines
    for (uint8_t i = 1; i < count; i++) {
        int16_t x0 = gx0 + (int16_t)(((int32_t)(i - 1) * stepFp) >> 8);
        int16_t x1 = gx0 + (int16_t)(((int32_t)i        * stepFp) >> 8);

        // Indoor — solid line (2 px thick)
        int16_t iy0 = tempToY(data[i - 1].indoorTemp);
        int16_t iy1 = tempToY(data[i].indoorTemp);
        display.drawLine(x0, iy0,     x1, iy1,     BLK);
        display.drawLine(x0, iy0 + 1, x1, iy1 + 1, BLK);

        // Outdoor — dotted (draw every other pixel)
        int16_t oy0 = tempToY(data[i - 1].outdoorTemp);
        int16_t oy1 = tempToY(data[i].outdoorTemp);
        if ((i & 1) == 0) {
            display.drawLine(x0, oy0, x1, oy1, BLK);
        }
    }

    // Legend
    display.setCursor(gx0 + 20, gy0 + 2);
    display.print("IN");
    display.setCursor(gx0 + 20, gy0 + 10);
    display.print("OUT");
}

// ---------------------------------------------------------------------------
// Public entry point
// ---------------------------------------------------------------------------
namespace DisplayRenderer {

void init() {
    Serial.println("[Display] Initialising hardware...");
    display.init(115200);
    display.setRotation(0);   // 0° – connector at top of installed panel
    display.hibernate();   // power down panel; keeps SPI/DMA allocations alive
}

void render(const WeatherSnapshot& snap) {
    display.setFullWindow();
    display.firstPage();

    do {
        display.fillScreen(WHT);
        display.setTextSize(1);   // ensure default text scale
        display.setTextColor(BLK);

        drawTopBar(snap);
        drawIndoorPanel(snap.indoor);
        drawCentrePanel(snap.current);
        drawRightPanel(snap.current);
        drawForecastRow(snap.forecast);
        drawAstronomyBar(snap.forecast);
        drawTrendGraph(snap.graph, snap.graphCount);

        // Offline warning overlay
        if (!snap.apiOk) {
            display.setFont(&FreeSansBold9pt7b);
            display.setTextColor(BLK);
            printCentred("! Offline – cached data", 0, EPD_WIDTH, 12);
        }

    } while (display.nextPage());

    display.hibernate();
    Serial.println("[Display] Render complete");
}

} // namespace DisplayRenderer
