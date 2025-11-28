// ===============================
// File: kw_gfx.h
// Minimal GFX-like C library for Zephyr drivers
// Works with LPM013M126A by providing a draw-pixel callback
// ===============================
#ifndef KW_GFX_H
#define KW_GFX_H

#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

// Color type: keep generic (16-bit). If your panel is 4-bit, use low nibble.
typedef uint16_t gfx_color_t;

// Forward decl
struct kw_gfx;

typedef void (*kw_gfx_draw_pixel_cb)(int16_t x, int16_t y, gfx_color_t color, void *user);
typedef void (*kw_gfx_refresh_cb)(void *user);  // optional (may be NULL)

// Graphics context (similar spirit to Adafruit_GFX but in C)
typedef struct kw_gfx {
    int16_t width;           // raw width
    int16_t height;          // raw height
    uint8_t rotation;        // 0..3

    // text state (hooks only; you can ignore if not using text)
    int16_t cursor_x;
    int16_t cursor_y;
    gfx_color_t text_color;
    gfx_color_t text_bg;
    uint8_t text_size_x;
    uint8_t text_size_y;
    bool text_wrap;          // wrap text at right edge if true

    // callbacks
    kw_gfx_draw_pixel_cb draw_cb;
    kw_gfx_refresh_cb    refresh_cb; // can be NULL
    void *user;                       // user context passed to callbacks
} kw_gfx_t;

// ======== Init ========
void kw_gfx_init(kw_gfx_t *g, int16_t w, int16_t h,
                 kw_gfx_draw_pixel_cb draw_cb,
                 kw_gfx_refresh_cb refresh_cb,
                 void *user_ctx);

// ======== Core pixel ========
void kw_gfx_draw_pixel(kw_gfx_t *g, int16_t x, int16_t y, gfx_color_t color);

// ======== Primitives ========
void kw_gfx_draw_fast_hline(kw_gfx_t *g, int16_t x, int16_t y, int16_t w, gfx_color_t c);
void kw_gfx_draw_fast_vline(kw_gfx_t *g, int16_t x, int16_t y, int16_t h, gfx_color_t c);
void kw_gfx_draw_line(kw_gfx_t *g, int16_t x0, int16_t y0, int16_t x1, int16_t y1, gfx_color_t c);
void kw_gfx_draw_rect(kw_gfx_t *g, int16_t x, int16_t y, int16_t w, int16_t h, gfx_color_t c);
void kw_gfx_fill_rect(kw_gfx_t *g, int16_t x, int16_t y, int16_t w, int16_t h, gfx_color_t c);
void kw_gfx_fill_round_rect(kw_gfx_t *g, int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint8_t color);
void kw_gfx_fill_screen(kw_gfx_t *g, gfx_color_t c);


void kw_gfx_draw_circle(kw_gfx_t *g, int16_t x0, int16_t y0, int16_t r, gfx_color_t c);
void kw_gfx_fill_circle(kw_gfx_t *g, int16_t x0, int16_t y0, int16_t r, gfx_color_t c);

void kw_gfx_draw_triangle(kw_gfx_t *g, int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, gfx_color_t c);
void kw_gfx_fill_triangle(kw_gfx_t *g, int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, gfx_color_t c);

// 1bpp bitmap: each row is packed MSB-first; set bits -> color, cleared -> bg if bg_enable
void kw_gfx_draw_bitmap_1bpp(kw_gfx_t *g, int16_t x, int16_t y,
                             const uint8_t *bitmap, int16_t w, int16_t h,
                             gfx_color_t color, bool bg_enable, gfx_color_t bg);

// ======== Rotation ========
void kw_gfx_set_rotation(kw_gfx_t *g, uint8_t r);   // 0..3

// ======== Text hooks (minimal) ========
static inline void kw_gfx_set_cursor(kw_gfx_t *g, int16_t x, int16_t y){ g->cursor_x = x; g->cursor_y = y; }
static inline void kw_gfx_set_text_color(kw_gfx_t *g, gfx_color_t fg, gfx_color_t bg){ g->text_color=fg; g->text_bg=bg; }
static inline void kw_gfx_set_text_size(kw_gfx_t *g, uint8_t sx, uint8_t sy){ g->text_size_x=sx; g->text_size_y=sy; }
static inline void kw_gfx_set_wrap(kw_gfx_t *g, bool w){ g->text_wrap=w; }

// (Optional) If you later add fonts, expose APIs here.

// ======== Refresh hook ========
static inline void kw_gfx_refresh(kw_gfx_t *g){ if(g->refresh_cb) g->refresh_cb(g->user); }

#ifdef __cplusplus
}
#endif

#endif // KW_GFX_H

