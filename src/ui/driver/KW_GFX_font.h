
// ===============================
// File: kw_gfx_font.h
// Text rendering helpers for kw_gfx using Adafruit-style GFXfont
// and an optional classic 5x7 bitmap font.
// ===============================
#ifndef KW_GFX_FONT_H
#define KW_GFX_FONT_H

#include <stdint.h>
#include <stdbool.h>
#include "../../ui/driver/KW_GFX.h"

#ifdef __cplusplus
extern "C" {
#endif

// ---- Adafruit GFXfont-compatible structs ----
typedef struct {
  uint16_t bitmapOffset; // index into GFXfont->bitmap
  uint8_t  width;        // glyph bitmap width
  uint8_t  height;       // glyph bitmap height
  uint8_t  xAdvance;     // distance to advance cursor (x)
  int8_t   xOffset;      // x offset from cursor to UL corner
  int8_t   yOffset;      // y offset from cursor to UL corner (relative to baseline)
} GFXglyph;

typedef struct {
  const uint8_t *bitmap; // concatenated glyph bitmaps
  const GFXglyph *glyph; // glyph metadata array
  uint16_t first;        // first supported char code
  uint16_t last;         // last supported char code
  uint8_t  yAdvance;     // line height (baseline-to-baseline)
} GFXfont;

// ---- Rendering with GFXfont ----
void kw_gfx_draw_char_gfxfont(kw_gfx_t *g,
                              int16_t x, int16_t y,       // baseline origin
                              char c,
                              const GFXfont *f,
                              gfx_color_t color);

void kw_gfx_draw_text_gfxfont(kw_gfx_t *g,
                              int16_t x, int16_t y,       // baseline origin
                              const char *text,
                              const GFXfont *f,
                              gfx_color_t color);

// ---- Optional classic 5x7 fixed font (5 columns x 7 rows, ASCII 32..127) ----
// Provide a table shaped as: 96 characters * 5 bytes each (column-major, LSB=top or bottom depending on your table).
// Pass pointer to the first byte for ASCII 32 (space).
void kw_gfx_draw_char_5x7(kw_gfx_t *g,
                          int16_t x, int16_t y,          // top-left corner
                          char c,
                          const uint8_t *font5x7_cols,    // 96*5 bytes table (ASCII 32..127)
                          bool lsb_top,                   // true if bit0 is top pixel in a column
                          gfx_color_t color,
                          bool bg_enable,
                          gfx_color_t bg);

void kw_gfx_draw_text_5x7(kw_gfx_t *g,
                          int16_t x, int16_t y,          // top-left corner of first char
                          const char *text,
                          const uint8_t *font5x7_cols,
                          bool lsb_top,
                          uint8_t x_spacing,             // extra pixels between chars
                          gfx_color_t color,
                          bool bg_enable,
                          gfx_color_t bg);

int text_width_gfxfont(const GFXfont *f, const char *s);

#ifdef __cplusplus
}
#endif

#endif // KW_GFX_FONT_H
