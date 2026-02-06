#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "driver_display_memory/LPM013M126A.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Lightweight graphics helpers for the K-Watch LCD (4bpp). */

typedef struct {
    uint8_t width;      /* glyph width in pixels */
    uint8_t height;     /* glyph height in pixels */
    char    first;      /* first supported ASCII */
    char    last;       /* last supported ASCII  */
    const uint8_t *data;/* packed bitmap, column-major, LSB = top pixel */
} kgfx_font_t;

void kgfx_init(void);
void kgfx_clear(uint8_t color);
void kgfx_flush(void);

/* State setters */
void kgfx_set_rotation(uint8_t r); /* 0,1,2,3 => 0/90/180/270 deg */
int16_t kgfx_width(void);
int16_t kgfx_height(void);
void kgfx_set_text_size(uint8_t s); /* scale factor */
void kgfx_set_text_color(uint8_t fg, uint8_t bg, bool solid_bg);
void kgfx_set_font(const kgfx_font_t *font); /* NULL => default 5x7 */

/* Primitives */
void kgfx_draw_pixel(int16_t x, int16_t y, uint8_t color);
void kgfx_draw_fast_hline(int16_t x, int16_t y, int16_t w, uint8_t color);
void kgfx_draw_fast_vline(int16_t x, int16_t y, int16_t h, uint8_t color);
void kgfx_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color);
void kgfx_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);
void kgfx_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color);
void kgfx_draw_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t color);
void kgfx_fill_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t color);
void kgfx_draw_circle(int16_t x0, int16_t y0, int16_t r, uint8_t color);
void kgfx_fill_circle(int16_t x0, int16_t y0, int16_t r, uint8_t color);

/* Bitmaps */
void kgfx_draw_bitmap_mono(int16_t x, int16_t y, int16_t w, int16_t h,
                           const uint8_t *data, uint8_t color, uint8_t bg, bool solid_bg);
void kgfx_draw_bitmap_4bpp(int16_t x, int16_t y, int16_t w, int16_t h,
                           const uint8_t *data); /* nibble-packed, MS nibble = left pixel */

/* Text (uses current font/color/size state) */
void kgfx_draw_char(int16_t x, int16_t y, char c,
                    uint8_t color, uint8_t bg, bool solid_bg, uint8_t size);
void kgfx_draw_text(int16_t x, int16_t y, const char *str,
                    uint8_t color, uint8_t bg, bool solid_bg, uint8_t size);
void kgfx_write_text(int16_t x, int16_t y, const char *str); /* uses current text state */

#ifdef __cplusplus
}
#endif
