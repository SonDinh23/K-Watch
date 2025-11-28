// ===============================
// File: KW_GFX.c
// ===============================
#include "../../ui/driver/KW_GFX.h"

#ifndef KW_GFX_ABS
#define KW_GFX_ABS(x) ((x) < 0 ? -(x) : (x))
#endif

static inline void swap_i16(int16_t *a, int16_t *b){ int16_t t=*a; *a=*b; *b=t; }

// Map logical (x,y) to rotated coordinates
static inline void map_xy_rot(const kw_gfx_t *g, int16_t *x, int16_t *y){
    int16_t tx=*x, ty=*y;
    switch(g->rotation & 3){
    case 0: break;
    case 1: *x = g->height - 1 - ty; *y = tx; break;
    case 2: *x = g->width  - 1 - tx; *y = g->height - 1 - ty; break;
    default:/*3*/ *x = ty; *y = g->width - 1 - tx; break;
    }
}

void kw_gfx_init(kw_gfx_t *g, int16_t w, int16_t h,
                 kw_gfx_draw_pixel_cb draw_cb,
                 kw_gfx_refresh_cb refresh_cb,
                 void *user_ctx)
{
    g->width  = w;
    g->height = h;
    g->rotation = 0;
    g->cursor_x = 0;
    g->cursor_y = 0;
    g->text_color = 0xFFFF;
    g->text_bg = 0x0000;
    g->text_size_x = 1;
    g->text_size_y = 1;
    g->text_wrap = true;
    g->draw_cb = draw_cb;
    g->refresh_cb = refresh_cb;
    g->user = user_ctx;
}

void kw_gfx_draw_pixel(kw_gfx_t *g, int16_t x, int16_t y, gfx_color_t color)
{
    // clipping before rotation (logical space)
    if (x < 0 || y < 0 || x >= g->width || y >= g->height) return;
    // map rotation to physical
    map_xy_rot(g, &x, &y);
    if (g->draw_cb) g->draw_cb(x, y, color, g->user);
}

void kw_gfx_draw_fast_hline(kw_gfx_t *g, int16_t x, int16_t y, int16_t w, gfx_color_t c)
{
    if (w <= 0) return;
    for (int16_t i = 0; i < w; ++i) kw_gfx_draw_pixel(g, x + i, y, c);
}

void kw_gfx_draw_fast_vline(kw_gfx_t *g, int16_t x, int16_t y, int16_t h, gfx_color_t c)
{
    if (h <= 0) return;
    for (int16_t i = 0; i < h; ++i) kw_gfx_draw_pixel(g, x, y + i, c);
}

void kw_gfx_draw_rect(kw_gfx_t *g, int16_t x, int16_t y, int16_t w, int16_t h, gfx_color_t c)
{
    if (w <= 0 || h <= 0) return;
    kw_gfx_draw_fast_hline(g, x, y, w, c);
    kw_gfx_draw_fast_hline(g, x, y + h - 1, w, c);
    kw_gfx_draw_fast_vline(g, x, y, h, c);
    kw_gfx_draw_fast_vline(g, x + w - 1, y, h, c);
}

void kw_gfx_fill_rect(kw_gfx_t *g, int16_t x, int16_t y, int16_t w, int16_t h, gfx_color_t c)
{
    if (w <= 0 || h <= 0) return;
    for (int16_t j = 0; j < h; ++j) kw_gfx_draw_fast_hline(g, x, y + j, w, c);
}

void kw_gfx_fill_screen(kw_gfx_t *g, gfx_color_t c)
{
    kw_gfx_fill_rect(g, 0, 0, g->width, g->height, c);
}

void kw_gfx_draw_line(kw_gfx_t *g, int16_t x0, int16_t y0, int16_t x1, int16_t y1, gfx_color_t c)
{
    int16_t dx = KW_GFX_ABS(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int16_t dy = -KW_GFX_ABS(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy, e2;

    for(;;){
        kw_gfx_draw_pixel(g, x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void kw_gfx_draw_circle(kw_gfx_t *g, int16_t x0, int16_t y0, int16_t r, gfx_color_t c)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    kw_gfx_draw_pixel(g, x0, y0 + r, c);
    kw_gfx_draw_pixel(g, x0, y0 - r, c);
    kw_gfx_draw_pixel(g, x0 + r, y0, c);
    kw_gfx_draw_pixel(g, x0 - r, y0, c);

    while (x < y) {
        if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
        x++; ddF_x += 2; f += ddF_x;
        kw_gfx_draw_pixel(g, x0 + x, y0 + y, c);
        kw_gfx_draw_pixel(g, x0 - x, y0 + y, c);
        kw_gfx_draw_pixel(g, x0 + x, y0 - y, c);
        kw_gfx_draw_pixel(g, x0 - x, y0 - y, c);
        kw_gfx_draw_pixel(g, x0 + y, y0 + x, c);
        kw_gfx_draw_pixel(g, x0 - y, y0 + x, c);
        kw_gfx_draw_pixel(g, x0 + y, y0 - x, c);
        kw_gfx_draw_pixel(g, x0 - y, y0 - x, c);
    }
}

void kw_gfx_fill_circle(kw_gfx_t *g, int16_t x0, int16_t y0, int16_t r, gfx_color_t c)
{
    kw_gfx_draw_fast_vline(g, x0, y0 - r, 2 * r + 1, c);
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    while (x < y) {
        if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
        x++; ddF_x += 2; f += ddF_x;
        kw_gfx_draw_fast_vline(g, x0 + x, y0 - y, 2 * y + 1, c);
        kw_gfx_draw_fast_vline(g, x0 - x, y0 - y, 2 * y + 1, c);
        kw_gfx_draw_fast_vline(g, x0 + y, y0 - x, 2 * x + 1, c);
        kw_gfx_draw_fast_vline(g, x0 - y, y0 - x, 2 * x + 1, c);
    }
}

void kw_gfx_draw_triangle(kw_gfx_t *g, int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, gfx_color_t c)
{
    kw_gfx_draw_line(g, x0, y0, x1, y1, c);
    kw_gfx_draw_line(g, x1, y1, x2, y2, c);
    kw_gfx_draw_line(g, x2, y2, x0, y0, c);
}

static void draw_hline_clip(kw_gfx_t *g, int16_t x, int16_t y, int16_t w, gfx_color_t c)
{
    if (y < 0 || y >= g->height || w <= 0) return;
    if (x < 0) { w += x; x = 0; }
    if (x + w > g->width) w = g->width - x;
    if (w <= 0) return;
    kw_gfx_draw_fast_hline(g, x, y, w, c);
}

void kw_gfx_fill_triangle(kw_gfx_t *g, int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, gfx_color_t c)
{
    // Sort by y
    if (y0 > y1){ swap_i16(&y0,&y1); swap_i16(&x0,&x1);} 
    if (y1 > y2){ swap_i16(&y1,&y2); swap_i16(&x1,&x2);} 
    if (y0 > y1){ swap_i16(&y0,&y1); swap_i16(&x0,&x1);} 

    int32_t dx01 = x1 - x0, dy01 = y1 - y0;
    int32_t dx02 = x2 - x0, dy02 = y2 - y0;
    int32_t dx12 = x2 - x1, dy12 = y2 - y1;

    int32_t sa = 0, sb = 0;
    int16_t y;

    if (y1 == y2) {
        for (y = y0; y <= y1; y++) {
            int16_t a = x0 + (dy01 ? sa / dy01 : 0);
            int16_t b = x0 + (dy02 ? sb / dy02 : 0);
            sa += dx01; sb += dx02;
            if (a > b) swap_i16(&a, &b);
            draw_hline_clip(g, a, y, b - a + 1, c);
        }
    } else if (y0 == y1) {
        sa = 0; sb = 0;
        for (y = y0; y <= y2; y++) {
            int16_t a = x0 + (dy02 ? sa / dy02 : 0);
            int16_t b = x1 + (dy12 ? sb / dy12 : 0);
            sa += dx02; sb += dx12;
            if (a > b) swap_i16(&a, &b);
            draw_hline_clip(g, a, y, b - a + 1, c);
        }
    } else {
        for (y = y0; y <= y1; y++) {
            int16_t a = x0 + (dy01 ? sa / dy01 : 0);
            int16_t b = x0 + (dy02 ? sb / dy02 : 0);
            sa += dx01; sb += dx02;
            if (a > b) swap_i16(&a, &b);
            draw_hline_clip(g, a, y, b - a + 1, c);
        }
        sa = dx12 * (int32_t)(y - y1);
        sb = dx02 * (int32_t)(y - y0);
        for (; y <= y2; y++) {
            int16_t a = x1 + (dy12 ? sa / dy12 : 0);
            int16_t b = x0 + (dy02 ? sb / dy02 : 0);
            sa += dx12; sb += dx02;
            if (a > b) swap_i16(&a, &b);
            draw_hline_clip(g, a, y, b - a + 1, c);
        }
    }
}

void kw_gfx_draw_bitmap_1bpp(kw_gfx_t *g, int16_t x, int16_t y,
                             const uint8_t *bitmap, int16_t w, int16_t h,
                             gfx_color_t color, bool bg_enable, gfx_color_t bg)
{
    for (int16_t j = 0; j < h; ++j) {
        int16_t bit_idx = 0; uint8_t byte = 0;
        for (int16_t i = 0; i < w; ++i) {
            if ((bit_idx & 7) == 0) byte = bitmap[(j * ((w + 7) >> 3)) + (bit_idx >> 3)];
            bool set = (byte & (0x80 >> (bit_idx & 7))) != 0;
            if (set) kw_gfx_draw_pixel(g, x + i, y + j, color);
            else if (bg_enable) kw_gfx_draw_pixel(g, x + i, y + j, bg);
            bit_idx++;
        }
    }
}

void kw_gfx_set_rotation(kw_gfx_t *g, uint8_t r) { g->rotation = (r & 3); }

// (Optional) Text API can be added later by providing a bitmap font and using
// kw_gfx_draw_bitmap_1bpp() with scaling and cursor state.


// vẽ 1 dải ngang [x0..x1] trên hàng y
static inline void hspan(kw_gfx_t *g, int x0, int x1, int y, uint8_t c)
{
    if (y < 0 || y >= g->height) return;
    if (x0 > x1) { int t=x0; x0=x1; x1=t; }
    if (x1 < 0 || x0 >= g->width) return;
    if (x0 < 0) x0 = 0;
    if (x1 >= g->width) x1 = g->width - 1;
    for (int x = x0; x <= x1; ++x) kw_gfx_draw_pixel(g, x, y, c);
}

void kw_gfx_fill_round_rect(kw_gfx_t *g, int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint8_t color)
{
    if (w <= 0 || h <= 0) return;
    if (r <= 0) { kw_gfx_fill_rect(g, x, y, w, h, color); return; }
    if (r > w/2) r = w/2;
    if (r > h/2) r = h/2;

    // phần chữ nhật giữa (không tính 4 góc)
    kw_gfx_fill_rect(g, x + r, y,       w - 2*r, h,       color);

    // 4 góc bo: đi từng hàng từ r xuống 0, tính bán kính ngang và tô thêm 2 bên
    for (int dy = 0; dy < r; ++dy) {
        int xr = (int)floorf(sqrtf((float)r*r - (float)dy*dy)); // bán kính ngang tại hàng này

        // top
        int y_top = y + dy;
        hspan(g, x + r - xr,         x + r - 1,           y_top, color);         // trái
        hspan(g, x + w - r,          x + w - r + xr - 1,  y_top, color);         // phải

        // bottom
        int y_bot = y + h - 1 - dy;
        hspan(g, x + r - xr,         x + r - 1,           y_bot, color);         // trái
        hspan(g, x + w - r,          x + w - r + xr - 1,  y_bot, color);         // phải
    }
}