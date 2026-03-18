#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ========= Adafruit GFX Font compatibility (struct names kept) ========= */
/* If your font headers are from Adafruit (FreeMono..pt7b.h), they usually
 * declare: extern const GFXfont FreeMono12pt7b;
 * So we keep the same struct names for drop-in compatibility.
 */

typedef struct {
    uint16_t bitmapOffset; /* Pointer into GFXfont->bitmap */
    uint8_t  width;
    uint8_t  height;
    uint8_t  xAdvance;
    int8_t   xOffset;
    int8_t   yOffset;
} GFXglyph;

typedef struct {
    const uint8_t *bitmap; /* Glyph bitmaps, packed MSB-first */
    const GFXglyph *glyph; /* Glyph array */
    uint16_t first;        /* ASCII extents (first char) */
    uint16_t last;         /* ASCII extents (last char) */
    uint8_t  yAdvance;     /* Line advance (baseline-to-baseline) */
} GFXfont;

/* ========= Driver hook ========= */
typedef void (*kgfx_draw_pixel_cb_t)(int16_t x, int16_t y, uint8_t color, void *user);

/* ========= KGFX object ========= */
typedef struct {
    /* Physical panel size */
    int16_t width;
    int16_t height;

    /* Current logical size after rotation */
    int16_t _width;
    int16_t _height;

    uint8_t rotation; /* 0..3 */

    /* Text state */
    int16_t cursor_x;
    int16_t cursor_y;     /* For GFX font: baseline y */
    uint8_t textcolor;
    uint8_t textbgcolor;
    uint8_t textsize_x;   /* 1..n */
    uint8_t textsize_y;   /* 1..n */
    bool    wrap;
    const GFXfont *font;  /* NULL => no font (you should set one) */

    /* Driver callback */
    kgfx_draw_pixel_cb_t draw_pixel_cb;
    void *user;
} kgfx_t;

/* ========= Init / core ========= */
void kgfx_init(kgfx_t *g,
               int16_t w, int16_t h,
               kgfx_draw_pixel_cb_t cb,
               void *user);

void kgfx_set_rotation(kgfx_t *g, uint8_t r);
uint8_t kgfx_get_rotation(const kgfx_t *g);

int16_t kgfx_width(const kgfx_t *g);
int16_t kgfx_height(const kgfx_t *g);

/* ========= Low-level draw ========= */
void kgfx_draw_pixel(kgfx_t *g, int16_t x, int16_t y, uint8_t color);

/* ========= Primitives ========= */
void kgfx_draw_fast_hline(kgfx_t *g, int16_t x, int16_t y, int16_t w, uint8_t color);
void kgfx_draw_fast_vline(kgfx_t *g, int16_t x, int16_t y, int16_t h, uint8_t color);
void kgfx_draw_line(kgfx_t *g, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color);

void kgfx_draw_rect(kgfx_t *g, int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);
void kgfx_fill_rect(kgfx_t *g, int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);
void kgfx_fill_screen(kgfx_t *g, uint8_t color);

void kgfx_draw_circle(kgfx_t *g, int16_t x0, int16_t y0, int16_t r, uint8_t color);
void kgfx_fill_circle(kgfx_t *g, int16_t x0, int16_t y0, int16_t r, uint8_t color);

/* 1-bit bitmap (packed MSB-first per byte) */
void kgfx_draw_bitmap_1bpp(kgfx_t *g, int16_t x, int16_t y,
                          const uint8_t *bitmap,
                          int16_t w, int16_t h,
                          uint8_t color);

/* ========= Rounded rectangle ========= */
void kgfx_draw_round_rect(kgfx_t *g, int16_t x, int16_t y, int16_t w, int16_t h,
                          int16_t r, uint8_t color);
void kgfx_fill_round_rect(kgfx_t *g, int16_t x, int16_t y, int16_t w, int16_t h,
                          int16_t r, uint8_t color);

/* ========= Triangle ========= */
void kgfx_draw_triangle(kgfx_t *g, int16_t x0, int16_t y0,
                        int16_t x1, int16_t y1,
                        int16_t x2, int16_t y2, uint8_t color);
void kgfx_fill_triangle(kgfx_t *g, int16_t x0, int16_t y0,
                        int16_t x1, int16_t y1,
                        int16_t x2, int16_t y2, uint8_t color);

/* ========= Text ========= */
void kgfx_set_cursor(kgfx_t *g, int16_t x, int16_t y);
void kgfx_set_text_color(kgfx_t *g, uint8_t c);
void kgfx_set_text_color_bg(kgfx_t *g, uint8_t c, uint8_t bg);
void kgfx_set_text_size(kgfx_t *g, uint8_t sx, uint8_t sy);
void kgfx_set_text_wrap(kgfx_t *g, bool wrap);
void kgfx_set_font(kgfx_t *g, const GFXfont *f);

void kgfx_write_char(kgfx_t *g, char c);
void kgfx_print(kgfx_t *g, const char *s);
bool kgfx_write_codepoint(kgfx_t *g, uint32_t cp);
void kgfx_print_utf8(kgfx_t *g, const char *utf8);

/* Optional: printf-like helper (uses vsnprintf) */
void kgfx_printf(kgfx_t *g, const char *fmt, ...);

void kgfx_print_vn_strip(kgfx_t *g, const char *utf8);

/* ========= Text measurement ========= */
int16_t kgfx_text_width(const GFXfont *font, const char *s);
int16_t kgfx_font_height(const GFXfont *font);

typedef enum {
    KGFX_ALIGN_LEFT = 0,
    KGFX_ALIGN_CENTER,
    KGFX_ALIGN_RIGHT,
} kgfx_align_t;

void kgfx_draw_text_aligned(kgfx_t *g, const GFXfont *font,
                            int16_t x, int16_t y, int16_t area_w,
                            kgfx_align_t align, uint8_t color,
                            const char *text);