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
//   y=217 │ Astronomy bar (20 px): sunrise · sunset
//   y=237 │ Moon phase panel (63 px): illuminated disc + phase name + % illuminated

#include "display_renderer.h"
#include "icon_renderer.h"
#include "jp_text.h"
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

static constexpr int16_t MOON_Y     = ASTRO_Y + ASTRO_H;  // 237
static constexpr int16_t MOON_H     = EPD_HEIGHT - MOON_Y; // 63

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
    time_t now = time(nullptr);
    if (now < 1000000000L) now = snap.lastUpdate;
    localtime_r(&now, &t);

    // Time — HH:MM (ASCII)
    char timeBuf[9];
    strftime(timeBuf, sizeof(timeBuf), "%H:%M", &t);

    // Date — Japanese: M月D日(曜)
    // strftime %a gives English; use a JP weekday lookup instead.
    static const char* JP_DOW[7] = {
        u8"日", u8"月", u8"火", u8"水", u8"木", u8"金", u8"土"
    };
    char dateBuf[32];
    snprintf(dateBuf, sizeof(dateBuf), "%d\xe6\x9c\x88%d\xe6\x97\xa5(%s)",
             t.tm_mon + 1, t.tm_mday, JP_DOW[t.tm_wday]);
    // Note: \xe6\x9c\x88 = 月 (month), \xe6\x97\xa5 = 日 (day) in UTF-8

    // Wi-Fi status — JP, right-aligned
    const char* wifiStr = snap.apiOk ? u8"オンライン" : u8"オフライン";
    int16_t wifiW = jpStringWidth(wifiStr);

    display.setFont(&FreeSansBold9pt7b);
    display.setTextColor(BLK);

    // Time — left
    display.setCursor(4, TOPBAR_H2 - 4);
    display.print(timeBuf);

    // Date — centred using mixed JP/ASCII renderer
    jpPrintMixedCentred(display, dateBuf, 0, EPD_WIDTH, TOPBAR_H2 - 4, BLK);

    // Wi-Fi — right-aligned
    jpDrawString(display, wifiStr, EPD_WIDTH - wifiW - 3, TOPBAR_H2 - 4, BLK);

    hline(TOPBAR_H2);
}

static void drawIndoorPanel(const SensorReading& indoor) {
    const int16_t midX = LEFT_X + LEFT_W / 2;
    display.setFont(&FreeSans9pt7b);
    display.setTextColor(BLK);

    // Label — 室内
    jpPrintCentred(display, u8"室内", LEFT_X, LEFT_W, MAIN_Y + 16, BLK);

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
        snprintf(buf, sizeof(buf), "%.0f%%", indoor.humidity);  // RH unit shown via 湿度 label
    } else {
        strncpy(buf, "--%", sizeof(buf));
    }
    printCentred(buf, LEFT_X, LEFT_W, MAIN_Y + 64);

    // Horizontal bar separating temp from humidity (visual aid)
    display.drawFastHLine(LEFT_X + 8, MAIN_Y + 70, LEFT_W - 16, BLK);

    vline(LEFT_X + LEFT_W, MAIN_Y, MAIN_H);
}

static void drawCentrePanel(const CurrentConditions& cur) {
    display.setTextColor(BLK);

    // Label — mirrors "INDOOR" on left panel
    display.setFont(&FreeSans9pt7b);
    printCentred("OUTDOOR", CTR_X, CTR_W, MAIN_Y + 16);

    // Large temperature
    display.setFont(&FreeSansBold18pt7b);
    char tempBuf[12];
    if (cur.valid) {
        snprintf(tempBuf, sizeof(tempBuf), "%.1f\xB0", cur.outdoorTemp);
    } else {
        strncpy(tempBuf, "--.-\xB0", sizeof(tempBuf));
    }
    printCentred(tempBuf, CTR_X, CTR_W, MAIN_Y + 44);

    // Current atmospheric condition description (Japanese)
    display.setFont(&FreeSans9pt7b);
    const char* desc = u8"不明";
    uint8_t wc = cur.weatherCode;
    if      (wc == 0)                   desc = u8"快晴";
    else if (wc >= 1  && wc <= 2)       desc = u8"晴れ時々曇り";
    else if (wc == 3)                   desc = u8"曇り";
    else if (wc >= 45 && wc <= 48)      desc = u8"霧";
    else if (wc >= 51 && wc <= 67)      desc = u8"雨";
    else if (wc >= 71 && wc <= 77)      desc = u8"雪";
    else if (wc >= 80 && wc <= 82)      desc = u8"にわか雨";
    else if (wc >= 85 && wc <= 86)      desc = u8"にわか雪";
    else if (wc >= 95 && wc <= 99)      desc = u8"雷雨";
    jpPrintCentred(display, desc, CTR_X, CTR_W, MAIN_Y + 64, BLK);

    // Weather icon — 48×48 centred, based on current (not forecast) weather code
    IconRenderer::draw(display,
                       CTR_X + (CTR_W - ICON_SIZE) / 2,
                       MAIN_Y + 72,
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

    // Wind — 風速
    jpPrintCentred(display, u8"風速", RIGHT_X, RIGHT_W, ly, BLK);
    display.setFont(&FreeSansBold9pt7b);
    if (cur.valid) snprintf(buf, sizeof(buf), "%.0f km/h", cur.windSpeed);
    else strncpy(buf, "-- km/h", sizeof(buf));
    printCentred(buf, RIGHT_X, RIGHT_W, ly + 20);

    display.drawFastHLine(RIGHT_X + 6, ly + 25, RIGHT_W - 12, BLK);

    // Pressure — 気圧
    jpPrintCentred(display, u8"気圧", RIGHT_X, RIGHT_W, ly + 42, BLK);
    display.setFont(&FreeSansBold9pt7b);
    if (cur.valid) snprintf(buf, sizeof(buf), "%.0f hPa", cur.pressure);
    else strncpy(buf, "---- hPa", sizeof(buf));
    printCentred(buf, RIGHT_X, RIGHT_W, ly + 58);

    display.drawFastHLine(RIGHT_X + 6, ly + 63, RIGHT_W - 12, BLK);

    // Humidity — 湿度
    jpPrintCentred(display, u8"湿度", RIGHT_X, RIGHT_W, ly + 78, BLK);
    display.setFont(&FreeSansBold9pt7b);
    if (cur.valid) snprintf(buf, sizeof(buf), "%d%%", cur.humidity);
    else strncpy(buf, "--%", sizeof(buf));
    printCentred(buf, RIGHT_X, RIGHT_W, ly + 94);
}

// Helper: pixel width of an ASCII string using FreeSans9pt7b
static int16_t asciiWidth(const char* str) {
    int16_t x1, y1; uint16_t w, h;
    display.setFont(&FreeSans9pt7b);
    display.getTextBounds(str, 0, 20, &x1, &y1, &w, &h);
    return (int16_t)w;
}

static void drawForecastRow(const ForecastDay forecast[3]) {
    hline(FCST_Y);

    // Day labels — 今日 / 明日 / 明後日
    static const char* DAY_LABELS[3] = { u8"今日", u8"明日", u8"明後日" };

    for (int i = 0; i < 3; i++) {
        int16_t fx = i * FCST_COL_W;
        const ForecastDay& f = forecast[i];

        if (i > 0) vline(fx, FCST_Y, FCST_H);

        // Day label — JP font (bold weight approximated with regular at this size)
        display.setTextColor(BLK);
        jpPrintCentred(display, DAY_LABELS[i], fx, FCST_COL_W, FCST_Y + 15, BLK);

        char buf[20];

        // Max / min temperature
        display.setFont(&FreeSans9pt7b);
        if (f.valid) {
            snprintf(buf, sizeof(buf), "%.0f / %.0f\xB0""C", f.maxTemp, f.minTemp);
        } else {
            strncpy(buf, "-- / --", sizeof(buf));
        }
        printCentred(buf, fx, FCST_COL_W, FCST_Y + 35);

        // Rain chance — 降水 XX%
        // Render JP label + ASCII number in one line
        {
            const char* label = u8"降水 ";
            char pctBuf[8];
            if (f.valid) snprintf(pctBuf, sizeof(pctBuf), "%d%%", f.rainChance);
            else         strncpy(pctBuf, "--%", sizeof(pctBuf));
            int16_t tw = jpStringWidth(label) + asciiWidth(pctBuf);
            int16_t rx = fx + (FCST_COL_W - tw) / 2;
            rx = jpDrawString(display, label, rx, FCST_Y + 53, BLK);
            display.setFont(&FreeSans9pt7b);
            display.setCursor(rx, FCST_Y + 53);
            display.print(pctBuf);
        }
    }
    hline(FCST_Y + FCST_H);
}

static void drawAstronomyBar(const ForecastDay forecast[3]) {
    hline(ASTRO_Y);

    display.setFont(&FreeSans9pt7b);
    display.setTextColor(BLK);

    // Mixed JP + ASCII layout:
    // 「日の出」HH:MM   「日の入り」HH:MM   <moon phase>
    const ForecastDay& today = forecast[0];
    char rise[8], sset[8];
    if (today.valid) {
        snprintf(rise, sizeof(rise), " %s   ", today.sunrise);
        snprintf(sset, sizeof(sset), " %s   ", today.sunset);
    } else {
        strncpy(rise, " --:--   ", sizeof(rise));
        strncpy(sset, " --:--   ", sizeof(sset));
    }
    // Compute total width for centering
    int16_t totalW = jpStringWidth(u8"日の出") + asciiWidth(rise)
                   + jpStringWidth(u8"日の入り") + asciiWidth(sset)
                   + jpStringWidth(today.valid ? today.moonPhase : "--");
    int16_t x  = (EPD_WIDTH - totalW) / 2;
    int16_t bl = ASTRO_Y + ASTRO_H - 4;   // baseline

    x = jpDrawString(display, u8"日の出", x, bl, BLK);
    display.setFont(&FreeSans9pt7b); display.setTextColor(BLK);
    display.setCursor(x, bl); display.print(rise);
    x += asciiWidth(rise);

    x = jpDrawString(display, u8"日の入り", x, bl, BLK);
    display.setFont(&FreeSans9pt7b); display.setTextColor(BLK);
    display.setCursor(x, bl); display.print(sset);
    x += asciiWidth(sset);

    jpDrawString(display, today.valid ? today.moonPhase : u8"--", x, bl, BLK);
}

// Moon phase panel — illuminated disc on left, phase name + illumination % on right
static void drawMoonPanel(const ForecastDay forecast[3]) {
    hline(MOON_Y);

    const ForecastDay& today = forecast[0];
    const float phase = today.valid ? today.moonPhaseFraction : 0.0f;

    // Disc: radius 26, centred vertically in the 63 px band
    const int16_t disc_r  = 26;
    const int16_t disc_cx = 50;
    const int16_t disc_cy = MOON_Y + MOON_H / 2;  // 268
    IconRenderer::drawMoonDisc(display, disc_cx, disc_cy, disc_r, phase, BLK);

    // Text block to the right of the disc
    const int16_t tx = disc_cx + disc_r + 14;  // x = 90

    display.setFont(&FreeSansBold9pt7b);
    display.setTextColor(BLK);
    display.setCursor(tx, MOON_Y + 16);
    display.print("MOON ");   // keep ASCII for layout anchor
    jpDrawString(display, u8"月相", display.getCursorX(), MOON_Y + 16, BLK);

    display.setFont(&FreeSans9pt7b);
    display.setCursor(tx, MOON_Y + 34);
    display.print(today.valid ? today.moonPhase : u8"--");

    // Illumination: XX% 照射
    char illumBuf[20];
    if (today.valid) {
        int pct = (int)roundf((1.0f - cosf(2.0f * 3.14159265f * phase)) / 2.0f * 100.0f);
        snprintf(illumBuf, sizeof(illumBuf), "%d%% ", pct);
    } else {
        strncpy(illumBuf, "--% ", sizeof(illumBuf));
    }
    display.setFont(&FreeSans9pt7b);
    display.setCursor(tx, MOON_Y + 52);
    display.print(illumBuf);
    jpDrawString(display, u8"照射", display.getCursorX(), MOON_Y + 52, BLK);
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
        drawMoonPanel(snap.forecast);

        // Offline warning overlay
        if (!snap.apiOk) {
            display.setFont(&FreeSansBold9pt7b);
            display.setTextColor(BLK);
            // "! " in ASCII then オフライン in JP
            int16_t tw = asciiWidth("! ") + jpStringWidth(u8"オフライン");
            int16_t x  = (EPD_WIDTH - tw) / 2;
            display.setCursor(x, 12);
            display.print("! ");
            jpDrawString(display, u8"オフライン", display.getCursorX(), 12, BLK);
        }

    } while (display.nextPage());

    display.hibernate();
    Serial.println("[Display] Render complete");
}

} // namespace DisplayRenderer
