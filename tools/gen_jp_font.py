#!/usr/bin/env python3
"""
Generate an Adafruit-GFX-compatible sparse Japanese font header.

Each glyph is stored as a continuous bitstream (MSB-first, w bits per row),
matching the layout that Adafruit GFX uses for custom fonts.  The glyph table
is sorted by Unicode codepoint so the runtime can binary-search it.

Output: src/fonts/JapaneseFont12.h

Requirements:
    pip install Pillow          (usually pre-installed)
    fonts-noto-cjk installed    (sudo apt install fonts-noto-cjk)
"""

import os, sys, math
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

# ---------------------------------------------------------------------------
# All unique characters needed by the weather-station UI
# ---------------------------------------------------------------------------
STRINGS = [
    "室内", "室外", "風速", "気圧", "湿度", "月相",
    "オンライン", "オフライン", "キャッシュデータ",
    "快晴", "晴れ時々曇り", "曇り", "霧", "雨", "雪",
    "にわか雨", "にわか雪", "雷雨", "不明",
    "今日", "明日", "明後日", "降水",
    "日の出", "日の入り",
    "照射",
    "新月", "三日月", "上弦", "十三夜", "満月", "居待月", "下弦", "有明月",
]

CHARS = sorted(set("".join(STRINGS)))

# ---------------------------------------------------------------------------
# Rendering parameters
# ---------------------------------------------------------------------------
FONT_PATH  = "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"
FONT_SIZE  = 12      # render size in pixels
CANVAS     = 24      # scratch canvas (must be >= FONT_SIZE + some headroom)
YADVANCE   = 14      # line height (pixels, used by renderer for line spacing)

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def pack_bits(bits):
    """Pack a list of 0/1 into bytes, MSB-first (GFX bitstream format)."""
    out = bytearray()
    for i in range(0, len(bits), 8):
        byte = 0
        for j in range(8):
            if i + j < len(bits) and bits[i + j]:
                byte |= (0x80 >> j)
        out.append(byte)
    return bytes(out)


def render_glyph(font, ch, ascent):
    """
    Render one character and return (bitmap_bytes, w, h, xOff, yOff, xAdv).

    xOff / yOff follow the Adafruit GFX sign convention:
        pixel drawn at (cursor_x + xOff + col, cursor_y + yOff + row)
    where cursor_y is the *baseline*.  So yOff is negative for glyphs that
    sit above the baseline (which is almost every character).
    """
    img  = Image.new("L", (CANVAS, CANVAS), 0)
    draw = ImageDraw.Draw(img)
    draw.text((0, 0), ch, font=font, fill=255)

    pix = img.load()
    min_x = min_y = CANVAS
    max_x = max_y = -1
    for y in range(CANVAS):
        for x in range(CANVAS):
            if pix[x, y] > 64:
                min_x = min(min_x, x)
                max_x = max(max_x, x)
                min_y = min(min_y, y)
                max_y = max(max_y, y)

    if max_x < 0:
        # Blank glyph — return a zero-size placeholder
        return b"", 0, 0, 0, 0, max(FONT_SIZE // 2, 6)

    w = max_x - min_x + 1
    h = max_y - min_y + 1

    bits = []
    for y in range(min_y, max_y + 1):
        for x in range(min_x, max_x + 1):
            bits.append(1 if pix[x, y] > 64 else 0)

    bitmap = pack_bits(bits)

    xOff = min_x
    yOff = min_y - ascent   # negative → above baseline

    # xAdv: use PIL's advance width (Pillow ≥ 9.2 has getlength)
    try:
        xAdv = int(font.getlength(ch)) + 1
    except AttributeError:
        try:
            xAdv = font.getsize(ch)[0] + 1   # older Pillow
        except Exception:
            xAdv = w + 2

    # Clamp to int8 range for xOff / yOff
    xOff = max(-128, min(127, xOff))
    yOff = max(-128, min(127, yOff))
    xAdv = max(1, min(255, xAdv))

    return bitmap, w, h, xOff, yOff, xAdv


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main():
    if not os.path.exists(FONT_PATH):
        print(f"ERROR: font not found at {FONT_PATH}")
        print("Install with:  sudo apt install fonts-noto-cjk")
        sys.exit(1)

    font = ImageFont.truetype(FONT_PATH, FONT_SIZE, index=0)

    # PIL ascent: distance from top of image to baseline when rendering at y=0
    try:
        ascent, descent = font.getmetrics()
    except AttributeError:
        ascent = FONT_SIZE

    print(f"Font: {FONT_PATH}  size={FONT_SIZE}px  ascent={ascent}  descent={descent if 'descent' in dir() else '?'}")
    print(f"Rendering {len(CHARS)} glyphs...")

    all_bitmap  = bytearray()
    glyph_rows  = []    # list of (cp, offset, w, h, xAdv, xOff, yOff)
    bitmap_offset = 0

    for ch in CHARS:
        bmp, w, h, xOff, yOff, xAdv = render_glyph(font, ch, ascent)
        glyph_rows.append((ord(ch), bitmap_offset, w, h, xAdv, xOff, yOff, ch))
        all_bitmap += bmp
        bitmap_offset += len(bmp)

    print(f"Bitmap total: {len(all_bitmap)} bytes  ({len(all_bitmap)/1024:.1f} KB)")

    # -----------------------------------------------------------------------
    # Write the header
    # -----------------------------------------------------------------------
    out_path = Path(__file__).parent.parent / "src" / "fonts" / "JapaneseFont12.h"
    out_path.parent.mkdir(parents=True, exist_ok=True)

    with open(out_path, "w", encoding="utf-8") as f:
        f.write(
f"""// JapaneseFont12.h — auto-generated Japanese glyph subset for Adafruit GFX
// Font: NotoSansCJK-Regular  |  Size: {FONT_SIZE}px  |  Glyphs: {len(CHARS)}  |  Bitmap: {len(all_bitmap)} bytes
// Generated by tools/gen_jp_font.py  — DO NOT EDIT MANUALLY
// Regenerate: python3 tools/gen_jp_font.py
#pragma once
#include <stdint.h>
#include <pgmspace.h>

struct JPGlyph {{
    uint32_t cp;      // Unicode codepoint (sorted ascending)
    uint16_t offset;  // byte offset into JP_BITMAP
    uint8_t  w, h;    // glyph bounding-box in pixels
    uint8_t  xAdv;    // cursor x-advance after this glyph
    int8_t   xOff;    // pixel offset: cursor_x + xOff = left edge of glyph
    int8_t   yOff;    // pixel offset: baseline_y + yOff = top edge of glyph
}};

// Glyph bitmaps — continuous bitstream, MSB-first, w bits per row (no row padding)
static const uint8_t JP_BITMAP[] PROGMEM = {{
""")
        # Bitmap bytes, 16 per line
        for i, byte in enumerate(all_bitmap):
            if i % 16 == 0:
                f.write("    ")
            f.write(f"0x{byte:02X},")
            if (i % 16 == 15) or (i == len(all_bitmap) - 1):
                f.write("\n")
            else:
                f.write(" ")
        f.write("};\n\n")

        # Glyph table
        f.write(
f"""// Glyph table — sorted by codepoint for binary search
static const JPGlyph JP_GLYPHS[] PROGMEM = {{
    //  codepoint   off    w    h  xAdv xOff yOff   char
""")
        for cp, off, w, h, xAdv, xOff, yOff, ch in glyph_rows:
            f.write(f"    {{ 0x{cp:05X}, {off:5d}, {w:3d}, {h:3d}, {xAdv:3d},"
                    f" {xOff:4d}, {yOff:4d} }},  // {ch}\n")
        f.write("};\n\n")

        f.write(f"static constexpr uint16_t JP_GLYPH_COUNT = {len(CHARS)};\n")
        f.write(f"static constexpr uint8_t  JP_YADVANCE    = {YADVANCE};\n")

    print(f"Written: {out_path}")
    print("\nCharacter inventory:")
    for cp, off, w, h, xAdv, xOff, yOff, ch in glyph_rows:
        print(f"  U+{cp:04X}  {ch}  w={w} h={h} xAdv={xAdv} xOff={xOff} yOff={yOff}  bytes={(w*h+7)//8}")

if __name__ == "__main__":
    main()
