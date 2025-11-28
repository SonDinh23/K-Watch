
// ===============================
// File: KW_GFX_font.c
// ===============================
#include "../../ui/driver/KW_GFX_font.h"

// ---- GFXfont rendering ----
void kw_gfx_draw_char_gfxfont(kw_gfx_t *g,
                              int16_t x, int16_t y,
                              char c,
                              const GFXfont *f,
                              gfx_color_t color)
{
    if (!f) return;
    uint16_t cc = (uint8_t)c;
    if (cc < f->first || cc > f->last) return;

    const GFXglyph *glyph = &f->glyph[cc - f->first];
    uint8_t  w  = glyph->width;
    uint8_t  h  = glyph->height;
    int8_t   xo = glyph->xOffset;
    int8_t   yo = glyph->yOffset;      // so với baseline (thường âm)
    uint32_t bo = glyph->bitmapOffset; // offset vào bitmap

    int16_t x0 = x + xo;
    int16_t y0 = y + yo;

    const uint8_t *bitmap = f->bitmap;

    uint8_t bitMask = 0, bits = 0;     // MSB-first như Adafruit
    for (uint8_t yy = 0; yy < h; ++yy) {
        for (uint8_t xx = 0; xx < w; ++xx) {
            if (bitMask == 0) {        // nạp byte mới khi hết 8 bit
                bits = bitmap[bo++];
                bitMask = 0x80;
            }
            if (bits & bitMask) {
                kw_gfx_draw_pixel(g, x0 + xx, y0 + yy, color);
            }
            bitMask >>= 1;
        }
    }
}

void kw_gfx_draw_text_gfxfont(kw_gfx_t *g,
                              int16_t x, int16_t y,
                              const char *text,
                              const GFXfont *f,
                              gfx_color_t color)
{
    if (!f || !text)
        return;
    int16_t cx = x;
    for (const char *p = text; *p; ++p)
    {
        char c = *p;
        if (c == '\r') continue;                 // bỏ CR trong CRLF
        if (c == '\n') { y += f->yAdvance; cx = x; continue; }
        if ((uint8_t)c < f->first || (uint8_t)c > f->last) continue;
        const GFXglyph *glyph = &f->glyph[(uint8_t)c - f->first];
        kw_gfx_draw_char_gfxfont(g, cx, y, c, f, color);
        cx += glyph->xAdvance;
    }
}

// ---- Classic 5x7 fixed-font rendering ----
void kw_gfx_draw_char_5x7(kw_gfx_t *g,
                          int16_t x, int16_t y,
                          char c,
                          const uint8_t *font5x7_cols,
                          bool lsb_top,
                          gfx_color_t color,
                          bool bg_enable,
                          gfx_color_t bg)
{
    if (!font5x7_cols)
        return;
    if ((uint8_t)c < 32 || (uint8_t)c > 127)
        return;
    uint16_t idx = ((uint8_t)c - 32) * 5; // 5 columns per glyph

    for (uint8_t col = 0; col < 5; ++col)
    {
        uint8_t bits = font5x7_cols[idx + col];
        for (uint8_t row = 0; row < 7; ++row)
        {
            bool on = lsb_top ? ((bits >> row) & 1) : ((bits >> (6 - row)) & 1);
            if (on)
                kw_gfx_draw_pixel(g, x + col, y + row, color);
            else if (bg_enable)
                kw_gfx_draw_pixel(g, x + col, y + row, bg);
        }
    }
}

void kw_gfx_draw_text_5x7(kw_gfx_t *g,
                          int16_t x, int16_t y,
                          const char *text,
                          const uint8_t *font5x7_cols,
                          bool lsb_top,
                          uint8_t x_spacing,
                          gfx_color_t color,
                          bool bg_enable,
                          gfx_color_t bg)
{
    if (!text)
        return;
    int16_t cx = x;
    for (const char *p = text; *p; ++p)
    {
        if (*p == ' ') { y += 8; cx = x; continue; } // 7px tall + 1px gap
        kw_gfx_draw_char_5x7(g, cx, y, *p, font5x7_cols, lsb_top, color, bg_enable, bg);
        cx += 5 + x_spacing; // 5 columns + spacing
    }
}

// === helpers ===
int text_width_gfxfont(const GFXfont *f, const char *s)
{
    int w = 0;
    for (const char *p = s; *p; ++p) {
        uint8_t c = (uint8_t)*p;
        if (c < f->first || c > f->last) continue;
        w += f->glyph[c - f->first].xAdvance;
    }
    return w;
}