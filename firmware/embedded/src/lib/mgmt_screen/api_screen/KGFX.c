#include "KGFX.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

static inline bool in_bounds(const kgfx_t *g, int16_t x, int16_t y)
{
    return (x >= 0 && y >= 0 && x < g->_width && y < g->_height);
}

/* Transform logical (x,y) -> physical panel coordinates based on rotation */
static inline void transform_xy(const kgfx_t *g, int16_t *x, int16_t *y)
{
    int16_t tx = *x, ty = *y;

    switch (g->rotation & 3U) {
    default:
    case 0:
        /* no-op */
        break;
    case 1:
        /* 90 deg */
        *x = g->width - 1 - ty;
        *y = tx;
        return;
    case 2:
        /* 180 deg */
        *x = g->width - 1 - tx;
        *y = g->height - 1 - ty;
        return;
    case 3:
        /* 270 deg */
        *x = ty;
        *y = g->height - 1 - tx;
        return;
    }
    *x = tx;
    *y = ty;
}

void kgfx_init(kgfx_t *g,
               int16_t w, int16_t h,
               kgfx_draw_pixel_cb_t cb,
               void *user)
{
    if (!g) return;
    memset(g, 0, sizeof(*g));

    g->width  = w;
    g->height = h;
    g->_width  = w;
    g->_height = h;

    g->rotation = 0;

    g->cursor_x = 0;
    g->cursor_y = 0;
    g->textcolor   = 0x0F;
    g->textbgcolor = 0x00;
    g->textsize_x = 1;
    g->textsize_y = 1;
    g->wrap = true;

    g->font = NULL; /* you should set a GFXfont (FreeMono..pt7b) */

    g->draw_pixel_cb = cb;
    g->user = user;
}

void kgfx_set_rotation(kgfx_t *g, uint8_t r)
{
    if (!g) return;
    g->rotation = (uint8_t)(r & 3U);

    if ((g->rotation & 1U) == 0U) {
        g->_width  = g->width;
        g->_height = g->height;
    } else {
        g->_width  = g->height;
        g->_height = g->width;
    }
}

uint8_t kgfx_get_rotation(const kgfx_t *g)
{
    return g ? g->rotation : 0;
}

int16_t kgfx_width(const kgfx_t *g)  { return g ? g->_width  : 0; }
int16_t kgfx_height(const kgfx_t *g) { return g ? g->_height : 0; }

void kgfx_draw_pixel(kgfx_t *g, int16_t x, int16_t y, uint8_t color)
{
    if (!g || !g->draw_pixel_cb) return;
    if (!in_bounds(g, x, y)) return;

    transform_xy(g, &x, &y);

    /* After transform, clip to physical */
    if (x < 0 || y < 0 || x >= g->width || y >= g->height) return;

    g->draw_pixel_cb(x, y, color, g->user);
}

void kgfx_draw_fast_hline(kgfx_t *g, int16_t x, int16_t y, int16_t w, uint8_t color)
{
    if (!g || w <= 0) return;
    for (int16_t i = 0; i < w; i++) {
        kgfx_draw_pixel(g, x + i, y, color);
    }
}

void kgfx_draw_fast_vline(kgfx_t *g, int16_t x, int16_t y, int16_t h, uint8_t color)
{
    if (!g || h <= 0) return;
    for (int16_t i = 0; i < h; i++) {
        kgfx_draw_pixel(g, x, y + i, color);
    }
}

/* Bresenham */
void kgfx_draw_line(kgfx_t *g, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color)
{
    if (!g) return;

    int16_t dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t dy = (y1 > y0) ? (y0 - y1) : (y1 - y0); /* negative */
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx + dy;

    while (1) {
        kgfx_draw_pixel(g, x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int16_t e2 = (int16_t)(2 * err);
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void kgfx_draw_rect(kgfx_t *g, int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color)
{
    if (!g || w <= 0 || h <= 0) return;
    kgfx_draw_fast_hline(g, x, y, w, color);
    kgfx_draw_fast_hline(g, x, y + h - 1, w, color);
    kgfx_draw_fast_vline(g, x, y, h, color);
    kgfx_draw_fast_vline(g, x + w - 1, y, h, color);
}

void kgfx_fill_rect(kgfx_t *g, int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color)
{
    if (!g || w <= 0 || h <= 0) return;
    for (int16_t yy = 0; yy < h; yy++) {
        kgfx_draw_fast_hline(g, x, y + yy, w, color);
    }
}

void kgfx_fill_screen(kgfx_t *g, uint8_t color)
{
    if (!g) return;
    kgfx_fill_rect(g, 0, 0, g->_width, g->_height, color);
}

/* Midpoint circle */
void kgfx_draw_circle(kgfx_t *g, int16_t x0, int16_t y0, int16_t r, uint8_t color)
{
    if (!g || r < 0) return;

    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    kgfx_draw_pixel(g, x0, y0 + r, color);
    kgfx_draw_pixel(g, x0, y0 - r, color);
    kgfx_draw_pixel(g, x0 + r, y0, color);
    kgfx_draw_pixel(g, x0 - r, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        kgfx_draw_pixel(g, x0 + x, y0 + y, color);
        kgfx_draw_pixel(g, x0 - x, y0 + y, color);
        kgfx_draw_pixel(g, x0 + x, y0 - y, color);
        kgfx_draw_pixel(g, x0 - x, y0 - y, color);

        kgfx_draw_pixel(g, x0 + y, y0 + x, color);
        kgfx_draw_pixel(g, x0 - y, y0 + x, color);
        kgfx_draw_pixel(g, x0 + y, y0 - x, color);
        kgfx_draw_pixel(g, x0 - y, y0 - x, color);
    }
}

void kgfx_fill_circle(kgfx_t *g, int16_t x0, int16_t y0, int16_t r, uint8_t color)
{
    if (!g || r < 0) return;

    kgfx_draw_fast_vline(g, x0, y0 - r, 2 * r + 1, color);

    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        kgfx_draw_fast_vline(g, x0 + x, y0 - y, 2 * y + 1, color);
        kgfx_draw_fast_vline(g, x0 - x, y0 - y, 2 * y + 1, color);
        kgfx_draw_fast_vline(g, x0 + y, y0 - x, 2 * x + 1, color);
        kgfx_draw_fast_vline(g, x0 - y, y0 - x, 2 * x + 1, color);
    }
}

/* 1-bit bitmap, MSB-first in each byte */
void kgfx_draw_bitmap_1bpp(kgfx_t *g, int16_t x, int16_t y,
                          const uint8_t *bitmap,
                          int16_t w, int16_t h,
                          uint8_t color)
{
    if (!g || !bitmap || w <= 0 || h <= 0) return;

    int16_t byteWidth = (w + 7) / 8;
    for (int16_t j = 0; j < h; j++) {
        for (int16_t i = 0; i < w; i++) {
            uint8_t b = bitmap[j * byteWidth + (i / 8)];
            if (b & (0x80 >> (i & 7))) {
                kgfx_draw_pixel(g, x + i, y + j, color);
            }
        }
    }
}

/* ===== Rounded rectangle (circle-quarter corners) ===== */

static void kgfx_draw_circle_helper(kgfx_t *g, int16_t x0, int16_t y0,
                                    int16_t r, uint8_t corners, uint8_t color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    while (x < y) {
        if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
        x++; ddF_x += 2; f += ddF_x;
        if (corners & 0x4) { kgfx_draw_pixel(g, x0+x, y0+y, color); kgfx_draw_pixel(g, x0+y, y0+x, color); }
        if (corners & 0x2) { kgfx_draw_pixel(g, x0+x, y0-y, color); kgfx_draw_pixel(g, x0+y, y0-x, color); }
        if (corners & 0x8) { kgfx_draw_pixel(g, x0-y, y0+x, color); kgfx_draw_pixel(g, x0-x, y0+y, color); }
        if (corners & 0x1) { kgfx_draw_pixel(g, x0-y, y0-x, color); kgfx_draw_pixel(g, x0-x, y0-y, color); }
    }
}

static void kgfx_fill_circle_helper(kgfx_t *g, int16_t x0, int16_t y0,
                                    int16_t r, uint8_t corners, int16_t delta,
                                    uint8_t color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    while (x < y) {
        if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
        x++; ddF_x += 2; f += ddF_x;
        if (corners & 0x1) {
            kgfx_draw_fast_vline(g, x0+x, y0-y, 2*y+1+delta, color);
            kgfx_draw_fast_vline(g, x0+y, y0-x, 2*x+1+delta, color);
        }
        if (corners & 0x2) {
            kgfx_draw_fast_vline(g, x0-x, y0-y, 2*y+1+delta, color);
            kgfx_draw_fast_vline(g, x0-y, y0-x, 2*x+1+delta, color);
        }
    }
}

void kgfx_draw_round_rect(kgfx_t *g, int16_t x, int16_t y, int16_t w, int16_t h,
                          int16_t r, uint8_t color)
{
    if (!g || w <= 0 || h <= 0) return;
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;

    kgfx_draw_fast_hline(g, x + r, y,         w - 2 * r, color);
    kgfx_draw_fast_hline(g, x + r, y + h - 1, w - 2 * r, color);
    kgfx_draw_fast_vline(g, x,         y + r, h - 2 * r, color);
    kgfx_draw_fast_vline(g, x + w - 1, y + r, h - 2 * r, color);

    kgfx_draw_circle_helper(g, x + r,         y + r,         r, 1, color);
    kgfx_draw_circle_helper(g, x + w - r - 1, y + r,         r, 2, color);
    kgfx_draw_circle_helper(g, x + w - r - 1, y + h - r - 1, r, 4, color);
    kgfx_draw_circle_helper(g, x + r,         y + h - r - 1, r, 8, color);
}

void kgfx_fill_round_rect(kgfx_t *g, int16_t x, int16_t y, int16_t w, int16_t h,
                          int16_t r, uint8_t color)
{
    if (!g || w <= 0 || h <= 0) return;
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;

    kgfx_fill_rect(g, x + r, y, w - 2 * r, h, color);
    kgfx_fill_circle_helper(g, x + w - r - 1, y + r, r, 1, h - 2 * r - 1, color);
    kgfx_fill_circle_helper(g, x + r,         y + r, r, 2, h - 2 * r - 1, color);
}

/* ===== Triangle ===== */

void kgfx_draw_triangle(kgfx_t *g, int16_t x0, int16_t y0,
                        int16_t x1, int16_t y1,
                        int16_t x2, int16_t y2, uint8_t color)
{
    kgfx_draw_line(g, x0, y0, x1, y1, color);
    kgfx_draw_line(g, x1, y1, x2, y2, color);
    kgfx_draw_line(g, x2, y2, x0, y0, color);
}

void kgfx_fill_triangle(kgfx_t *g, int16_t x0, int16_t y0,
                        int16_t x1, int16_t y1,
                        int16_t x2, int16_t y2, uint8_t color)
{
    int16_t a, b, y, last;
    int16_t sa, sb;

    if (y0 > y1) { int16_t t = y0; y0 = y1; y1 = t; t = x0; x0 = x1; x1 = t; }
    if (y1 > y2) { int16_t t = y1; y1 = y2; y2 = t; t = x1; x1 = x2; x2 = t; }
    if (y0 > y1) { int16_t t = y0; y0 = y1; y1 = t; t = x0; x0 = x1; x1 = t; }

    if (y0 == y2) {
        a = b = x0;
        if (x1 < a) a = x1; else if (x1 > b) b = x1;
        if (x2 < a) a = x2; else if (x2 > b) b = x2;
        kgfx_draw_fast_hline(g, a, y0, b - a + 1, color);
        return;
    }

    int16_t dx01 = x1 - x0, dy01 = y1 - y0;
    int16_t dx02 = x2 - x0, dy02 = y2 - y0;
    int16_t dx12 = x2 - x1, dy12 = y2 - y1;

    last = (y1 == y2) ? y1 : y1 - 1;

    sa = 0; sb = 0;
    for (y = y0; y <= last; y++) {
        a = x0 + sa / dy01;
        b = x0 + sb / dy02;
        sa += dx01; sb += dx02;
        if (a > b) { int16_t t = a; a = b; b = t; }
        kgfx_draw_fast_hline(g, a, y, b - a + 1, color);
    }

    sa = dx12 * (y - y1);
    sb = dx02 * (y - y0);
    for (; y <= y2; y++) {
        a = x1 + sa / dy12;
        b = x0 + sb / dy02;
        sa += dx12; sb += dx02;
        if (a > b) { int16_t t = a; a = b; b = t; }
        kgfx_draw_fast_hline(g, a, y, b - a + 1, color);
    }
}

/* ============================ Text rendering (GFX font) ============================ */

void kgfx_set_cursor(kgfx_t *g, int16_t x, int16_t y)
{
    if (!g) return;
    g->cursor_x = x;
    g->cursor_y = y;
}

void kgfx_set_text_color(kgfx_t *g, uint8_t c)
{
    if (!g) return;
    g->textcolor = c;
    g->textbgcolor = c;
}

void kgfx_set_text_color_bg(kgfx_t *g, uint8_t c, uint8_t bg)
{
    if (!g) return;
    g->textcolor = c;
    g->textbgcolor = bg;
}

void kgfx_set_text_size(kgfx_t *g, uint8_t sx, uint8_t sy)
{
    if (!g) return;
    if (sx == 0) sx = 1;
    if (sy == 0) sy = 1;
    g->textsize_x = sx;
    g->textsize_y = sy;
}

void kgfx_set_text_wrap(kgfx_t *g, bool wrap)
{
    if (!g) return;
    g->wrap = wrap;
}

void kgfx_set_font(kgfx_t *g, const GFXfont *f)
{
    if (!g) return;
    g->font = f;
}

/* Lookup glyph in current font */
static const GFXglyph *lookup_glyph(const GFXfont *f, uint32_t cp)
{
    if (!f) return NULL;
    if (cp < f->first || cp > f->last) return NULL;
    uint16_t idx = (uint16_t)(cp - f->first);
    return &f->glyph[idx];
}

/* Draw one character using current GFXfont */
static bool draw_char_gfxfont(kgfx_t *g, int16_t x, int16_t y_baseline, uint32_t cp)
{
    if (!g || !g->font) return false;
    const GFXfont *f = g->font;

    const GFXglyph *glyph = lookup_glyph(f, cp);
    if (!glyph) return false;

    const uint8_t  *bitmap = f->bitmap;

    uint16_t bo = glyph->bitmapOffset;
    uint8_t  w  = glyph->width;
    uint8_t  h  = glyph->height;
    int8_t   xo = glyph->xOffset;
    int8_t   yo = glyph->yOffset;

    int16_t x0 = x + xo;
    int16_t y0 = y_baseline + yo;

    if (g->textbgcolor != g->textcolor) {
        kgfx_fill_rect(g, x0, y0,
                       (int16_t)w * g->textsize_x,
                       (int16_t)h * g->textsize_y,
                       g->textbgcolor);
    }

    uint8_t bits = 0;
    uint8_t bit  = 0;

    for (uint8_t yy = 0; yy < h; yy++) {
        for (uint8_t xx = 0; xx < w; xx++) {
            if ((bit & 7U) == 0U) {
                bits = bitmap[bo++];
            }

            if (bits & 0x80) {
                if (g->textsize_x == 1 && g->textsize_y == 1) {
                    kgfx_draw_pixel(g, x0 + xx, y0 + yy, g->textcolor);
                } else {
                    kgfx_fill_rect(g,
                                   x0 + (int16_t)xx * g->textsize_x,
                                   y0 + (int16_t)yy * g->textsize_y,
                                   g->textsize_x, g->textsize_y,
                                   g->textcolor);
                }
            }

            bits <<= 1;
            bit++;
        }
    }
    return true;
}

bool kgfx_write_codepoint(kgfx_t *g, uint32_t cp)
{
    if (!g) return false;

    if (cp == '\r') return true;
    if (cp == '\n') {
        if (g->font) {
            g->cursor_x = 0;
            g->cursor_y = (int16_t)(g->cursor_y + (int16_t)g->font->yAdvance * g->textsize_y);
        } else {
            g->cursor_x = 0;
            g->cursor_y = (int16_t)(g->cursor_y + 8 * g->textsize_y);
        }
        return true;
    }

    if (!g->font) {
        return false;
    }

    const GFXglyph *glyph = lookup_glyph(g->font, cp);
    if (!glyph) {
        return false;
    }

    int16_t adv = (int16_t)glyph->xAdvance * g->textsize_x;
    if (g->wrap && (g->cursor_x + adv) > g->_width) {
        g->cursor_x = 0;
        g->cursor_y = (int16_t)(g->cursor_y + (int16_t)g->font->yAdvance * g->textsize_y);
    }

    if (!draw_char_gfxfont(g, g->cursor_x, g->cursor_y, cp)) {
        return false;
    }
    g->cursor_x = (int16_t)(g->cursor_x + adv);
    return true;
}

void kgfx_write_char(kgfx_t *g, char c)
{
    (void)kgfx_write_codepoint(g, (uint8_t)c);
}

void kgfx_print(kgfx_t *g, const char *s)
{
    if (!g || !s) return;
    while (*s) {
        kgfx_write_char(g, *s++);
    }
}

void kgfx_printf(kgfx_t *g, const char *fmt, ...)
{
    if (!g || !fmt) return;

    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    kgfx_print(g, buf);
}

/* ===== UTF-8 decode (basic) ===== */
static bool utf8_next(const char **s, uint32_t *out_cp)
{
    const uint8_t *p = (const uint8_t *)(*s);
    if (!p || !*p) return false;

    uint32_t cp;
    if (p[0] < 0x80) {
        cp = p[0];
        *s += 1;
    } else if ((p[0] & 0xE0) == 0xC0 && p[1]) {
        if ((p[1] & 0xC0) == 0x80) {
            cp = ((p[0] & 0x1F) << 6) | (p[1] & 0x3F);
            *s += 2;
        } else {
            cp = p[0];
            *s += 1;
        }
    } else if ((p[0] & 0xF0) == 0xE0 && p[1] && p[2]) {
        if (((p[1] & 0xC0) == 0x80) && ((p[2] & 0xC0) == 0x80)) {
            cp = ((p[0] & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
            *s += 3;
        } else {
            cp = p[0];
            *s += 1;
        }
    } else {
        cp = p[0];
        *s += 1; /* invalid -> best-effort */
    }

    *out_cp = cp;
    return true;
}

/* ===== Map Vietnamese codepoint -> ASCII base letter ===== */
static char vn_cp_to_ascii(uint32_t cp)
{
    /* đ/Đ */
    if (cp == 0x0111) return 'd';
    if (cp == 0x0110) return 'D';

    /* Vietnamese blocks U+1EA0..U+1EF9 (precomposed) */
    if (cp >= 0x1EA0 && cp <= 0x1EB7) return (cp & 1) ? 'a' : 'A'; /* A, Â, Ă + tones */
    if (cp >= 0x1EB8 && cp <= 0x1EC7) return (cp & 1) ? 'e' : 'E'; /* E, Ê + tones */
    if (cp >= 0x1EC8 && cp <= 0x1ECB) return (cp & 1) ? 'i' : 'I'; /* I + tones */
    if (cp >= 0x1ECC && cp <= 0x1EE3) return (cp & 1) ? 'o' : 'O'; /* O, Ô, Ơ + tones */
    if (cp >= 0x1EE4 && cp <= 0x1EF1) return (cp & 1) ? 'u' : 'U'; /* U, Ư + tones */
    if (cp >= 0x1EF2 && cp <= 0x1EF9) return (cp & 1) ? 'y' : 'Y'; /* Y + tones */

    /* Common Latin-1 accents you may encounter */
    switch (cp) {
    /* a/A */
    case 0x00E0: case 0x00E1: case 0x00E2: case 0x00E3: case 0x00E4: case 0x00E5: return 'a';
    case 0x00C0: case 0x00C1: case 0x00C2: case 0x00C3: case 0x00C4: case 0x00C5: return 'A';
    case 0x0103: return 'a'; case 0x0102: return 'A'; /* ă/Ă */
    /* e/E */
    case 0x00E8: case 0x00E9: case 0x00EA: case 0x00EB: return 'e';
    case 0x00C8: case 0x00C9: case 0x00CA: case 0x00CB: return 'E';
    /* i/I */
    case 0x00EC: case 0x00ED: case 0x00EE: case 0x00EF: return 'i';
    case 0x00CC: case 0x00CD: case 0x00CE: case 0x00CF: return 'I';
    /* o/O */
    case 0x00F2: case 0x00F3: case 0x00F4: case 0x00F5: case 0x00F6: return 'o';
    case 0x00D2: case 0x00D3: case 0x00D4: case 0x00D5: case 0x00D6: return 'O';
    case 0x01A1: return 'o'; case 0x01A0: return 'O'; /* ơ/Ơ */
    /* u/U */
    case 0x00F9: case 0x00FA: case 0x00FB: case 0x00FC: return 'u';
    case 0x00D9: case 0x00DA: case 0x00DB: case 0x00DC: return 'U';
    case 0x01B0: return 'u'; case 0x01AF: return 'U'; /* ư/Ư */
    /* y/Y */
    case 0x00FD: case 0x00FF: return 'y';
    case 0x00DD: return 'Y';
    default: break;
    }

    return 0; /* not Vietnamese */
}

void kgfx_print_vn_strip(kgfx_t *g, const char *utf8)
{
    if (!g || !utf8) return;

    const char *p = utf8;
    uint32_t cp;

    while (utf8_next(&p, &cp)) {
        if (kgfx_write_codepoint(g, cp)) {
            continue;
        }

        /* Vietnamese fallback to ASCII base if glyph missing */
        char base = vn_cp_to_ascii(cp);
        if (base) {
            if (kgfx_write_codepoint(g, (uint8_t)base)) {
                continue;
            }
        }

        if (cp == 0x2013 || cp == 0x2014) { (void)kgfx_write_codepoint(g, '-'); continue; }
        if (cp == 0x2018 || cp == 0x2019) { (void)kgfx_write_codepoint(g, '\''); continue; }
        if (cp == 0x201C || cp == 0x201D) { (void)kgfx_write_codepoint(g, '"');  continue; }
        if (cp == 0x2026) { kgfx_write_codepoint(g, '.'); kgfx_write_codepoint(g, '.'); kgfx_write_codepoint(g, '.'); continue; }

        (void)kgfx_write_codepoint(g, '?');
    }
}

void kgfx_print_utf8(kgfx_t *g, const char *utf8)
{
    /* Use same logic as vn_strip: render if glyph exists, fallback to ASCII base for VN */
    kgfx_print_vn_strip(g, utf8);
}

/* ===== Text measurement & aligned draw ===== */

int16_t kgfx_text_width(const GFXfont *font, const char *s)
{
    if (!font || !s) return 0;
    int16_t w = 0;
    while (*s) {
        uint8_t c = (uint8_t)*s++;
        const GFXglyph *gl = lookup_glyph(font, c);
        if (gl) {
            w += (int16_t)gl->xAdvance;
        }
    }
    return w;
}

int16_t kgfx_font_height(const GFXfont *font)
{
    if (!font) return 0;
    return (int16_t)font->yAdvance;
}

void kgfx_draw_text_aligned(kgfx_t *g, const GFXfont *font,
                            int16_t x, int16_t y, int16_t area_w,
                            kgfx_align_t align, uint8_t color,
                            const char *text)
{
    if (!g || !text || !font) return;
    int16_t tw = kgfx_text_width(font, text);
    switch (align) {
    case KGFX_ALIGN_CENTER: x += (area_w - tw) / 2; break;
    case KGFX_ALIGN_RIGHT:  x += area_w - tw;       break;
    default: break;
    }
    kgfx_set_font(g, font);
    kgfx_set_text_color(g, color);
    kgfx_set_cursor(g, x, y);
    kgfx_print(g, text);
}