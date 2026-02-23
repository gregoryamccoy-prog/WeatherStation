// icon_renderer.h
// Draws weather icons using Adafruit GFX primitives.
// Icons are 48×48 pixels and drawn at the given (x, y) top-left origin.
// `gfx` must be the active GFX canvas/display object.
// All drawing uses GFX_BLACK (foreground) on the white e-paper background.
#pragma once

#include <stdint.h>
#include <Adafruit_GFX.h>

namespace IconRenderer {

enum class WeatherIcon {
    CLEAR,
    PARTLY_CLOUDY,
    CLOUDY,
    FOG,
    RAIN,
    SNOW,
    SHOWERS,
    THUNDERSTORM,
    UNKNOWN
};

// Map WMO weather_code to icon enum
inline WeatherIcon fromWmoCode(uint8_t code) {
    if (code == 0)                        return WeatherIcon::CLEAR;
    if (code >= 1  && code <= 3)          return WeatherIcon::PARTLY_CLOUDY;
    if (code >= 45 && code <= 48)         return WeatherIcon::FOG;
    if (code >= 51 && code <= 67)         return WeatherIcon::RAIN;
    if (code >= 71 && code <= 77)         return WeatherIcon::SNOW;
    if (code >= 80 && code <= 82)         return WeatherIcon::SHOWERS;
    if (code >= 95 && code <= 99)         return WeatherIcon::THUNDERSTORM;
    return WeatherIcon::UNKNOWN;
}

// Draw a sun (clear sky)
template<typename GFX>
void drawSun(GFX& gfx, int16_t x, int16_t y, uint16_t fg) {
    const int16_t cx = x + 24, cy = y + 24;
    // Core circle
    gfx.fillCircle(cx, cy, 10, fg);
    // 8 rays
    const int8_t cosTab[8] = { 0, 13, 18, 13,  0, -13, -18, -13 };
    const int8_t sinTab[8] = { 18, 13,  0, -13, -18, -13,   0,  13 };
    for (uint8_t i = 0; i < 8; i++) {
        int16_t x0 = cx + cosTab[i] * 13 / 18;
        int16_t y0 = cy + sinTab[i] * 13 / 18;
        int16_t x1 = cx + cosTab[i];
        int16_t y1 = cy + sinTab[i];
        gfx.drawLine(x0, y0, x1, y1, fg);
        gfx.drawLine(x0+1, y0, x1+1, y1, fg);
    }
}

// Draw a cloud
template<typename GFX>
void drawCloud(GFX& gfx, int16_t x, int16_t y, uint16_t fg) {
    // cloud shape: two overlapping filled circles + rectangle base
    gfx.fillCircle(x + 18, y + 28, 10, fg);
    gfx.fillCircle(x + 30, y + 26, 12, fg);
    gfx.fillRect(x + 8,  y + 28, 32, 12, fg);
    // Carve out inside of cloud to make it hollow (just outline)
    // Actually for e-paper filled looks better — leave filled
}

// Partly cloudy: sun peeking behind cloud
template<typename GFX>
void drawPartlyCloudy(GFX& gfx, int16_t x, int16_t y, uint16_t fg) {
    // Small sun top-right
    const int16_t sx = x + 32, sy = y + 8;
    gfx.fillCircle(sx, sy, 7, fg);
    gfx.drawLine(sx,   sy-10, sx,   sy-12, fg);
    gfx.drawLine(sx,   sy+10, sx,   sy+12, fg);
    gfx.drawLine(sx-10, sy,  sx-12, sy,    fg);
    gfx.drawLine(sx+10, sy,  sx+12, sy,    fg);
    // Cloud overlapping
    gfx.fillCircle(x + 16, y + 30, 9,  fg);
    gfx.fillCircle(x + 27, y + 27, 11, fg);
    gfx.fillRect(x + 7,  y + 30, 30, 10, fg);
}

// Fog: horizontal dashed lines
template<typename GFX>
void drawFog(GFX& gfx, int16_t x, int16_t y, uint16_t fg) {
    for (uint8_t row = 0; row < 5; row++) {
        int16_t ly = y + 14 + row * 6;
        for (uint8_t seg = 0; seg < 4; seg++) {
            int16_t lx = x + 4 + seg * 11;
            gfx.drawFastHLine(lx, ly, 8, fg);
            gfx.drawFastHLine(lx, ly+1, 8, fg);
        }
    }
}

// Rain: cloud with 3 drop lines
template<typename GFX>
void drawRain(GFX& gfx, int16_t x, int16_t y, uint16_t fg) {
    // Cloud
    gfx.fillCircle(x + 16, y + 18, 9,  fg);
    gfx.fillCircle(x + 28, y + 15, 11, fg);
    gfx.fillRect(x + 7,   y + 18, 30, 10, fg);
    // Drops
    for (uint8_t d = 0; d < 4; d++) {
        int16_t dx = x + 12 + d * 8;
        int16_t dy = y + 32 + (d % 2) * 4;
        gfx.drawLine(dx, dy, dx - 3, dy + 8, fg);
        gfx.drawLine(dx+1, dy, dx - 2, dy + 8, fg);
    }
}

// Snow: cloud with asterisk flakes
template<typename GFX>
void drawSnow(GFX& gfx, int16_t x, int16_t y, uint16_t fg) {
    // Cloud
    gfx.fillCircle(x + 16, y + 18, 9,  fg);
    gfx.fillCircle(x + 28, y + 15, 11, fg);
    gfx.fillRect(x + 7,   y + 18, 30, 10, fg);
    // Snowflake asterisks
    const int16_t flakeX[4] = { x+12, x+20, x+28, x+36 };
    const int16_t flakeY[4] = { y+34, y+38, y+32, y+38 };
    for (uint8_t f = 0; f < 4; f++) {
        int16_t fx = flakeX[f], fy = flakeY[f];
        gfx.drawLine(fx - 4, fy, fx + 4, fy, fg);
        gfx.drawLine(fx, fy - 4, fx, fy + 4, fg);
        gfx.drawLine(fx - 3, fy - 3, fx + 3, fy + 3, fg);
        gfx.drawLine(fx - 3, fy + 3, fx + 3, fy - 3, fg);
    }
}

// Showers: lighter rain (sparse drops, dotted cloud)
template<typename GFX>
void drawShowers(GFX& gfx, int16_t x, int16_t y, uint16_t fg) {
    // Thin cloud outline
    gfx.drawCircle(x + 16, y + 18, 9,  fg);
    gfx.drawCircle(x + 28, y + 15, 11, fg);
    gfx.drawFastHLine(x + 7, y + 27, 30, fg);
    // Sparse drops
    for (uint8_t d = 0; d < 3; d++) {
        int16_t dx = x + 14 + d * 10;
        gfx.drawLine(dx, y + 32, dx - 3, y + 42, fg);
    }
}

// Thunderstorm: cloud with lightning bolt
template<typename GFX>
void drawThunderstorm(GFX& gfx, int16_t x, int16_t y, uint16_t fg) {
    // Cloud
    gfx.fillCircle(x + 16, y + 16, 9,  fg);
    gfx.fillCircle(x + 28, y + 13, 11, fg);
    gfx.fillRect(x + 7,   y + 16, 30, 10, fg);
    // Lightning bolt — filled polygon via lines
    // Bolt: top-right to bottom-left zig-zag
    gfx.drawLine(x + 26, y + 28, x + 20, y + 36, fg);
    gfx.drawLine(x + 27, y + 28, x + 21, y + 36, fg);
    gfx.drawLine(x + 20, y + 36, x + 26, y + 36, fg);
    gfx.drawLine(x + 26, y + 36, x + 18, y + 46, fg);
    gfx.drawLine(x + 27, y + 36, x + 19, y + 46, fg);
    // Rain drops alongside
    gfx.drawLine(x + 12, y + 30, x + 9,  y + 38, fg);
    gfx.drawLine(x + 35, y + 30, x + 32, y + 38, fg);
}

// Unknown: question mark in circle
template<typename GFX>
void drawUnknown(GFX& gfx, int16_t x, int16_t y, uint16_t fg) {
    gfx.drawCircle(x + 24, y + 24, 20, fg);
    // Use built-in 6x8 font for the "?" so it doesn't depend on
    // whatever GFX font happens to be active.
    gfx.setFont(nullptr);
    gfx.setTextColor(fg);
    gfx.setTextSize(2);
    gfx.setCursor(x + 18, y + 18);
    gfx.print('?');
    // IMPORTANT: restore text size so callers aren't affected
    gfx.setTextSize(1);
}

// Dispatch: draw icon for a given WMO code
template<typename GFX>
void draw(GFX& gfx, int16_t x, int16_t y, uint8_t wmoCode, uint16_t fg) {
    switch (fromWmoCode(wmoCode)) {
        case WeatherIcon::CLEAR:         drawSun(gfx, x, y, fg);          break;
        case WeatherIcon::PARTLY_CLOUDY: drawPartlyCloudy(gfx, x, y, fg); break;
        case WeatherIcon::CLOUDY:        drawCloud(gfx, x, y, fg);        break;
        case WeatherIcon::FOG:           drawFog(gfx, x, y, fg);          break;
        case WeatherIcon::RAIN:          drawRain(gfx, x, y, fg);         break;
        case WeatherIcon::SNOW:          drawSnow(gfx, x, y, fg);         break;
        case WeatherIcon::SHOWERS:       drawShowers(gfx, x, y, fg);      break;
        case WeatherIcon::THUNDERSTORM:  drawThunderstorm(gfx, x, y, fg); break;
        default:                         drawUnknown(gfx, x, y, fg);      break;
    }
}

} // namespace IconRenderer
