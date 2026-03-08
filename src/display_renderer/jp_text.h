// jp_text.h — UTF-8 Japanese text rendering helpers for Adafruit GFX
//
// Include AFTER the display object and JapaneseFont12.h are in scope.
// On ESP32, PROGMEM arrays are memory-mapped flash; pgm_read_* is a no-op
// in practice but kept for correctness.
//
// API:
//   jpStringWidth(str)                          — pixel width of a JP string
//   jpDrawString(gfx, str, x, y, color)         — draw UTF-8, return new x
//   jpPrintCentred(gfx, str, boxX, boxW, y, c)  — centred JP string
#pragma once

#include <stdint.h>
#include <pgmspace.h>
#include "../fonts/JapaneseFont12.h"

// ---------------------------------------------------------------------------
// UTF-8 decoder
// ---------------------------------------------------------------------------
// Decodes one Unicode codepoint from *pp and advances the pointer.
// Returns 0 at end-of-string.
inline uint32_t jpDecodeUTF8(const char** pp) {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(*pp);
    if (!*p) return 0;
    uint32_t cp;
    if      (*p < 0x80) { cp =  (*p++ & 0x7F);
    } else if (*p < 0xE0) { cp =  (*p++ & 0x1F) << 6;
                             cp |= (*p++ & 0x3F);
    } else if (*p < 0xF0) { cp =  (*p++ & 0x0F) << 12;
                             cp |= (*p++ & 0x3F) << 6;
                             cp |= (*p++ & 0x3F);
    } else                { cp =  (*p++ & 0x07) << 18;
                             cp |= (*p++ & 0x3F) << 12;
                             cp |= (*p++ & 0x3F) << 6;
                             cp |= (*p++ & 0x3F);
    }
    *pp = reinterpret_cast<const char*>(p);
    return cp;
}

// ---------------------------------------------------------------------------
// Glyph lookup — binary search on sorted JP_GLYPHS table
// ---------------------------------------------------------------------------
inline const JPGlyph* jpFindGlyph(uint32_t cp) {
    int lo = 0, hi = (int)JP_GLYPH_COUNT - 1;
    while (lo <= hi) {
        int mid = (lo + hi) >> 1;
        uint32_t mcp = JP_GLYPHS[mid].cp;
        if      (mcp < cp) lo = mid + 1;
        else if (mcp > cp) hi = mid - 1;
        else               return &JP_GLYPHS[mid];
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// String width (pixels) — used for centering
// ---------------------------------------------------------------------------
inline int16_t jpStringWidth(const char* str) {
    int16_t w = 0;
    const char* p = str;
    while (*p) {
        uint32_t cp = jpDecodeUTF8(&p);
        if (!cp) break;
        const JPGlyph* g = jpFindGlyph(cp);
        w += g ? (int16_t)g->xAdv : (int16_t)(JP_YADVANCE / 2);
    }
    return w;
}

// ---------------------------------------------------------------------------
// Draw a single glyph
// ---------------------------------------------------------------------------
template<typename GFX>
inline void jpDrawGlyph(GFX& gfx, const JPGlyph* g, int16_t x, int16_t y, uint16_t color) {
    uint8_t w    = g->w;
    uint8_t h    = g->h;
    int8_t  xOff = g->xOff;
    int8_t  yOff = g->yOff;
    if (w == 0 || h == 0) return;

    const uint8_t* bmp = JP_BITMAP + g->offset;
    uint16_t bit = 0;
    for (uint8_t row = 0; row < h; row++) {
        for (uint8_t col = 0; col < w; col++) {
            uint8_t byte = bmp[bit >> 3];
            if (byte & (0x80 >> (bit & 7)))
                gfx.drawPixel(x + xOff + col, y + yOff + row, color);
            bit++;
        }
    }
}

// ---------------------------------------------------------------------------
// Draw a UTF-8 string that may contain both JP glyphs and ASCII.
// ASCII codepoints (0x20–0x7E) are rendered with whatever GFX font is
// currently active on `gfx` (via gfx.write()), so set the font BEFORE
// calling this if you want ASCII in a specific typeface.
// Returns the new x cursor position.
// ---------------------------------------------------------------------------
template<typename GFX>
inline int16_t jpDrawString(GFX& gfx, const char* str, int16_t x, int16_t y, uint16_t color) {
    const char* p = str;
    while (*p) {
        uint32_t cp = jpDecodeUTF8(&p);
        if (!cp) break;
        if (cp >= 0x20 && cp <= 0x7E) {
            // ASCII: delegate to GFX so the active font is used
            gfx.setCursor(x, y);
            gfx.setTextColor(color);
            gfx.write((uint8_t)cp);
            x = gfx.getCursorX();
        } else {
            const JPGlyph* g = jpFindGlyph(cp);
            if (g) {
                jpDrawGlyph(gfx, g, x, y, color);
                x += (int16_t)g->xAdv;
            } else {
                x += (int16_t)(JP_YADVANCE / 2);
            }
        }
    }
    return x;
}

// Width of a mixed ASCII+JP string.  Requires a GFX reference so ASCII
// character advances can be measured via getTextBounds (accurate for any font).
template<typename GFX>
inline int16_t jpMixedWidth(GFX& gfx, const char* str) {
    int16_t w = 0;
    const char* p = str;
    while (*p) {
        uint32_t cp = jpDecodeUTF8(&p);
        if (!cp) break;
        if (cp >= 0x20 && cp <= 0x7E) {
            char tmp[2] = { (char)cp, '\0' };
            int16_t x1, y1; uint16_t cw, ch;
            gfx.getTextBounds(tmp, 0, 20, &x1, &y1, &cw, &ch);
            w += (int16_t)cw;
        } else {
            const JPGlyph* g = jpFindGlyph(cp);
            w += g ? (int16_t)g->xAdv : (int16_t)(JP_YADVANCE / 2);
        }
    }
    return w;
}

// Centre a mixed ASCII+JP string within [boxX, boxX+boxW)
template<typename GFX>
inline void jpPrintMixedCentred(GFX& gfx, const char* str,
                                 int16_t boxX, int16_t boxW,
                                 int16_t y, uint16_t color) {
    int16_t tw = jpMixedWidth(gfx, str);
    int16_t x  = boxX + (boxW - tw) / 2;
    jpDrawString(gfx, str, x, y, color);
}

// ---------------------------------------------------------------------------
// Centre a JP string within [boxX, boxX+boxW)
// ---------------------------------------------------------------------------
template<typename GFX>
inline void jpPrintCentred(GFX& gfx, const char* str,
                            int16_t boxX, int16_t boxW,
                            int16_t y, uint16_t color) {
    int16_t tw = jpStringWidth(str);
    int16_t x  = boxX + (boxW - tw) / 2;
    jpDrawString(gfx, str, x, y, color);
}
