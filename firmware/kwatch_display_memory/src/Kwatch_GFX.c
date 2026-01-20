#include "Kwatch_GFX.h"

#define GFX_W LCD_DISP_WIDTH
#define GFX_H LCD_DISP_HEIGHT

/* 5x7 font, each char 5 bytes, LSB is top pixel, from ' ' (0x20) to '~' (0x7E). */
static const uint8_t font5x7_data[] = {
    0x00,0x00,0x00,0x00,0x00, 0x00,0x00,0x5F,0x00,0x00, 0x00,0x07,0x00,0x07,0x00, 0x14,0x7F,0x14,0x7F,0x14,
    0x24,0x2A,0x7F,0x2A,0x12, 0x23,0x13,0x08,0x64,0x62, 0x36,0x49,0x55,0x22,0x50, 0x00,0x05,0x03,0x00,0x00,
    0x00,0x1C,0x22,0x41,0x00, 0x00,0x41,0x22,0x1C,0x00, 0x14,0x08,0x3E,0x08,0x14, 0x08,0x08,0x3E,0x08,0x08,
    0x00,0x50,0x30,0x00,0x00, 0x08,0x08,0x08,0x08,0x08, 0x00,0x60,0x60,0x00,0x00, 0x20,0x10,0x08,0x04,0x02,
    0x3E,0x51,0x49,0x45,0x3E, 0x00,0x42,0x7F,0x40,0x00, 0x72,0x49,0x49,0x49,0x46, 0x21,0x41,0x49,0x4D,0x33,
    0x18,0x14,0x12,0x7F,0x10, 0x27,0x45,0x45,0x45,0x39, 0x3C,0x4A,0x49,0x49,0x30, 0x01,0x71,0x09,0x05,0x03,
    0x36,0x49,0x49,0x49,0x36, 0x06,0x49,0x49,0x29,0x1E, 0x00,0x36,0x36,0x00,0x00, 0x00,0x56,0x36,0x00,0x00,
    0x08,0x14,0x22,0x41,0x00, 0x14,0x14,0x14,0x14,0x14, 0x00,0x41,0x22,0x14,0x08, 0x02,0x01,0x59,0x09,0x06,
    0x3E,0x41,0x5D,0x59,0x4E, 0x7E,0x11,0x11,0x11,0x7E, 0x7F,0x49,0x49,0x49,0x36, 0x3E,0x41,0x41,0x41,0x22,
    0x7F,0x41,0x41,0x22,0x1C, 0x7F,0x49,0x49,0x49,0x41, 0x7F,0x09,0x09,0x09,0x01, 0x3E,0x41,0x49,0x49,0x7A,
    0x7F,0x08,0x08,0x08,0x7F, 0x00,0x41,0x7F,0x41,0x00, 0x20,0x40,0x41,0x3F,0x01, 0x7F,0x08,0x14,0x22,0x41,
    0x7F,0x40,0x40,0x40,0x40, 0x7F,0x02,0x0C,0x02,0x7F, 0x7F,0x04,0x08,0x10,0x7F, 0x3E,0x41,0x41,0x41,0x3E,
    0x7F,0x09,0x09,0x09,0x06, 0x3E,0x41,0x51,0x21,0x5E, 0x7F,0x09,0x19,0x29,0x46, 0x46,0x49,0x49,0x49,0x31,
    0x01,0x01,0x7F,0x01,0x01, 0x3F,0x40,0x40,0x40,0x3F, 0x1F,0x20,0x40,0x20,0x1F, 0x3F,0x40,0x38,0x40,0x3F,
    0x63,0x14,0x08,0x14,0x63, 0x07,0x08,0x70,0x08,0x07, 0x61,0x51,0x49,0x45,0x43, 0x00,0x7F,0x41,0x41,0x00,
    0x02,0x04,0x08,0x10,0x20, 0x00,0x41,0x41,0x7F,0x00, 0x04,0x02,0x01,0x02,0x04, 0x40,0x40,0x40,0x40,0x40,
    0x00,0x01,0x02,0x04,0x00, 0x20,0x54,0x54,0x54,0x78, 0x7F,0x48,0x44,0x44,0x38, 0x38,0x44,0x44,0x44,0x28,
    0x38,0x44,0x44,0x48,0x7F, 0x38,0x54,0x54,0x54,0x18, 0x08,0x7E,0x09,0x01,0x02, 0x0C,0x52,0x52,0x52,0x3E,
    0x7F,0x08,0x04,0x04,0x78, 0x00,0x44,0x7D,0x40,0x00, 0x20,0x40,0x44,0x3D,0x00, 0x7F,0x10,0x28,0x44,0x00,
    0x00,0x41,0x7F,0x40,0x00, 0x7C,0x04,0x18,0x04,0x78, 0x7C,0x08,0x04,0x04,0x78, 0x38,0x44,0x44,0x44,0x38,
    0x7C,0x14,0x14,0x14,0x08, 0x08,0x14,0x14,0x18,0x7C, 0x7C,0x08,0x04,0x04,0x08, 0x48,0x54,0x54,0x54,0x20,
    0x04,0x3F,0x44,0x40,0x20, 0x3C,0x40,0x40,0x20,0x7C, 0x1C,0x20,0x40,0x20,0x1C, 0x3C,0x40,0x30,0x40,0x3C,
    0x44,0x28,0x10,0x28,0x44, 0x0C,0x50,0x50,0x50,0x3C, 0x44,0x64,0x54,0x4C,0x44, 0x00,0x08,0x36,0x41,0x00,
    0x00,0x00,0x7F,0x00,0x00, 0x00,0x41,0x36,0x08,0x00, 0x10,0x08,0x08,0x10,0x08, 0x00,0x00,0x00,0x00,0x00
};

static const kgfx_font_t font5x7 = {
    .width = 5,
    .height = 7,
    .first = 0x20,
    .last = 0x7E,
    .data = font5x7_data,
};

static uint8_t s_rotation = 0;        /* 0/1/2/3 => 0/90/180/270 deg */
static int16_t s_w = GFX_W;
static int16_t s_h = GFX_H;
static uint8_t s_text_size = 1;
static uint8_t s_text_fg = LCD_COLOR_BLACK;
static uint8_t s_text_bg = LCD_COLOR_WHITE;
static bool    s_text_solid = false;
static const kgfx_font_t *s_font = &font5x7;

static inline bool in_bounds(int16_t x, int16_t y)
{
    return (x >= 0 && y >= 0 && x < s_w && y < s_h);
}

static inline void transform(int16_t *x, int16_t *y)
{
    int16_t tx = *x;
    int16_t ty = *y;
    switch (s_rotation & 3) {
    default:
    case 0: break;
    case 1: tx = GFX_W - 1 - ty; ty = *x; break;
    case 2: tx = GFX_W - 1 - tx; ty = GFX_H - 1 - ty; break;
    case 3: tx = ty; ty = GFX_H - 1 - *x; break;
    }
    *x = tx;
    *y = ty;
}

void kgfx_init(void)
{
    s_rotation = 0;
    s_w = GFX_W;
    s_h = GFX_H;
    s_text_size = 1;
    s_text_fg = LCD_COLOR_BLACK;
    s_text_bg = LCD_COLOR_WHITE;
    s_text_solid = false;
    s_font = &font5x7;
    cmlcd_clear_display();
}

void kgfx_set_rotation(uint8_t r)
{
    s_rotation = r & 3;
    s_w = (s_rotation & 1) ? GFX_H : GFX_W;
    s_h = (s_rotation & 1) ? GFX_W : GFX_H;
}

int16_t kgfx_width(void)  { return s_w; }
int16_t kgfx_height(void) { return s_h; }

void kgfx_set_text_size(uint8_t s)
{
    s_text_size = (s == 0) ? 1 : s;
}

void kgfx_set_text_color(uint8_t fg, uint8_t bg, bool solid_bg)
{
    s_text_fg = fg;
    s_text_bg = bg;
    s_text_solid = solid_bg;
}

void kgfx_set_font(const kgfx_font_t *font)
{
    s_font = font ? font : &font5x7;
}

void kgfx_clear(uint8_t color)
{
    for (int y = 0; y < s_h; ++y) {
        for (int x = 0; x < s_w; ++x) {
            kgfx_draw_pixel(x, y, color);
        }
    }
}

void kgfx_flush(void)
{
    cmlcd_refresh();
}

void kgfx_draw_pixel(int16_t x, int16_t y, uint8_t color)
{
    if (!in_bounds(x, y)) return;
    transform(&x, &y);
    cmlcd_draw_pixel(x, y, color);
}

void kgfx_draw_fast_hline(int16_t x, int16_t y, int16_t w, uint8_t color)
{
    if (w <= 0 || !in_bounds(x, y)) return;
    int16_t x_end = x + w - 1;
    if (x_end < 0 || x >= s_w) return;
    if (x < 0) x = 0;
    if (x_end >= s_w) x_end = s_w - 1;
    for (int16_t xi = x; xi <= x_end; ++xi) {
        kgfx_draw_pixel(xi, y, color);
    }
}

void kgfx_draw_fast_vline(int16_t x, int16_t y, int16_t h, uint8_t color)
{
    if (h <= 0 || !in_bounds(x, y)) return;
    int16_t y_end = y + h - 1;
    if (y_end < 0 || y >= s_h) return;
    if (y < 0) y = 0;
    if (y_end >= s_h) y_end = s_h - 1;
    for (int16_t yi = y; yi <= y_end; ++yi) {
        kgfx_draw_pixel(x, yi, color);
    }
}

void kgfx_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint8_t color)
{
    int16_t dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t dy = (y0 > y1) ? (y1 - y0) : (y0 - y1);
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx + dy;

    while (1) {
        kgfx_draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int16_t e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void kgfx_draw_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t color)
{
    kgfx_draw_line(x0, y0, x1, y1, color);
    kgfx_draw_line(x1, y1, x2, y2, color);
    kgfx_draw_line(x2, y2, x0, y0, color);
}

static void swap_int16(int16_t *a, int16_t *b)
{
    int16_t t = *a; *a = *b; *b = t;
}

void kgfx_fill_triangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t color)
{
    /* sort by y */
    if (y0 > y1) { swap_int16(&y0, &y1); swap_int16(&x0, &x1); }
    if (y1 > y2) { swap_int16(&y1, &y2); swap_int16(&x1, &x2); }
    if (y0 > y1) { swap_int16(&y0, &y1); swap_int16(&x0, &x1); }

    if (y0 == y2) {
        /* flat triangle */
        int16_t xmin = x0 < x1 ? x0 : x1; xmin = xmin < x2 ? xmin : x2;
        int16_t xmax = x0 > x1 ? x0 : x1; xmax = xmax > x2 ? xmax : x2;
        kgfx_fill_rect(xmin, y0, xmax - xmin + 1, 1, color);
        return;
    }

    int32_t dx01 = x1 - x0;
    int32_t dy01 = y1 - y0;
    int32_t dx02 = x2 - x0;
    int32_t dy02 = y2 - y0;
    int32_t dx12 = x2 - x1;
    int32_t dy12 = y2 - y1;

    int32_t sa = 0;
    int32_t sb = 0;

    int16_t y;
    int16_t last = (y1 == y2) ? y1 : y1 - 1;

    for (y = y0; y <= last; y++) {
        int16_t a = x0 + sa / dy01;
        int16_t b = x0 + sb / dy02;
        sa += dx01;
        sb += dx02;
        if (a > b) swap_int16(&a, &b);
        kgfx_draw_fast_hline(a, y, b - a + 1, color);
    }

    sa = dx12 * (y - y1);
    sb = dx02 * (y - y0);
    for (; y <= y2; y++) {
        int16_t a = x1 + sa / dy12;
        int16_t b = x0 + sb / dy02;
        sa += dx12;
        sb += dx02;
        if (a > b) swap_int16(&a, &b);
        kgfx_draw_fast_hline(a, y, b - a + 1, color);
    }
}

void kgfx_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color)
{
    if (w <= 0 || h <= 0) return;
    kgfx_draw_fast_hline(x, y, w, color);
    kgfx_draw_fast_hline(x, y + h - 1, w, color);
    kgfx_draw_fast_vline(x, y, h, color);
    kgfx_draw_fast_vline(x + w - 1, y, h, color);
}

void kgfx_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t color)
{
    if (w <= 0 || h <= 0) return;
    for (int16_t yi = y; yi < y + h; ++yi) {
        kgfx_draw_fast_hline(x, yi, w, color);
    }
}

void kgfx_draw_circle(int16_t x0, int16_t y0, int16_t r, uint8_t color)
{
    if (r < 0) return;
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    kgfx_draw_pixel(x0, y0 + r, color);
    kgfx_draw_pixel(x0, y0 - r, color);
    kgfx_draw_pixel(x0 + r, y0, color);
    kgfx_draw_pixel(x0 - r, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        kgfx_draw_pixel(x0 + x, y0 + y, color);
        kgfx_draw_pixel(x0 - x, y0 + y, color);
        kgfx_draw_pixel(x0 + x, y0 - y, color);
        kgfx_draw_pixel(x0 - x, y0 - y, color);
        kgfx_draw_pixel(x0 + y, y0 + x, color);
        kgfx_draw_pixel(x0 - y, y0 + x, color);
        kgfx_draw_pixel(x0 + y, y0 - x, color);
        kgfx_draw_pixel(x0 - y, y0 - x, color);
    }
}

void kgfx_fill_circle(int16_t x0, int16_t y0, int16_t r, uint8_t color)
{
    if (r < 0) return;
    kgfx_draw_fast_vline(x0, y0 - r, 2 * r + 1, color);
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

        kgfx_draw_fast_vline(x0 + x, y0 - y, 2 * y + 1, color);
        kgfx_draw_fast_vline(x0 - x, y0 - y, 2 * y + 1, color);
        kgfx_draw_fast_vline(x0 + y, y0 - x, 2 * x + 1, color);
        kgfx_draw_fast_vline(x0 - y, y0 - x, 2 * x + 1, color);
    }
}

static uint8_t glyph_byte(const kgfx_font_t *font, char c, int col, int row_byte)
{
    if (!font) font = &font5x7;
    if (c < font->first || c > font->last) c = '?';
    int glyph_index = (c - font->first);
    int bytes_per_col = (font->height + 7) / 8;
    int offset = glyph_index * font->width * bytes_per_col + col * bytes_per_col + row_byte;
    return font->data[offset];
}

static void render_char(int16_t x, int16_t y, char c,
                        uint8_t color, uint8_t bg, bool solid_bg, uint8_t size,
                        const kgfx_font_t *font)
{
    if (!font) font = &font5x7;
    if (size < 1) size = 1;

    for (int col = 0; col < font->width; ++col) {
        for (int row = 0; row < font->height; ++row) {
            int row_byte = row / 8;
            int row_bit  = row % 8;
            uint8_t line = glyph_byte(font, c, col, row_byte);
            uint8_t pix_on = (line >> row_bit) & 0x01;
            if (pix_on || solid_bg) {
                uint8_t draw_color = pix_on ? color : bg;
                int16_t px = x + col * size;
                int16_t py = y + row * size;
                for (int dx = 0; dx < size; ++dx) {
                    for (int dy = 0; dy < size; ++dy) {
                        kgfx_draw_pixel(px + dx, py + dy, draw_color);
                    }
                }
            }
        }
    }
    if (solid_bg) {
        for (int sx = 0; sx < size; ++sx) {
            for (int sy = 0; sy < font->height * size; ++sy) {
                kgfx_draw_pixel(x + font->width * size + sx, y + sy, bg);
            }
        }
    }
}

void kgfx_draw_char(int16_t x, int16_t y, char c,
                    uint8_t color, uint8_t bg, bool solid_bg, uint8_t size)
{
    render_char(x, y, c, color, bg, solid_bg, size, s_font);
}

void kgfx_draw_text(int16_t x, int16_t y, const char *str,
                    uint8_t color, uint8_t bg, bool solid_bg, uint8_t size)
{
    int16_t cursor_x = x;
    while (str && *str) {
        if (*str == '\n') {
            cursor_x = x;
            y += (s_font->height + 1) * size;
            ++str;
            continue;
        }
        render_char(cursor_x, y, *str, color, bg, solid_bg, size, s_font);
        cursor_x += (s_font->width + 1) * size;
        ++str;
    }
}

void kgfx_write_text(int16_t x, int16_t y, const char *str)
{
    int16_t cursor_x = x;
    while (str && *str) {
        if (*str == '\n') {
            cursor_x = x;
            y += (s_font->height + 1) * s_text_size;
            ++str;
            continue;
        }
        render_char(cursor_x, y, *str, s_text_fg, s_text_bg, s_text_solid, s_text_size, s_font);
        cursor_x += (s_font->width + 1) * s_text_size;
        ++str;
    }
}

void kgfx_draw_bitmap_mono(int16_t x, int16_t y, int16_t w, int16_t h,
                           const uint8_t *data, uint8_t color, uint8_t bg, bool solid_bg)
{
    if (!data || w <= 0 || h <= 0) return;
    for (int16_t j = 0; j < h; ++j) {
        for (int16_t i = 0; i < w; ++i) {
            int idx = (j * w + i);
            uint8_t byte = data[idx / 8];
            uint8_t bit  = 0x80 >> (idx & 7);
            if (byte & bit) {
                kgfx_draw_pixel(x + i, y + j, color);
            } else if (solid_bg) {
                kgfx_draw_pixel(x + i, y + j, bg);
            }
        }
    }
}

void kgfx_draw_bitmap_4bpp(int16_t x, int16_t y, int16_t w, int16_t h,
                           const uint8_t *data)
{
    if (!data || w <= 0 || h <= 0) return;
    int bytes_per_row = (w + 1) / 2;
    for (int16_t j = 0; j < h; ++j) {
        const uint8_t *row = data + j * bytes_per_row;
        for (int16_t i = 0; i < w; ++i) {
            uint8_t byte = row[i / 2];
            uint8_t pix = (i & 1) ? (byte & 0x0F) : (byte >> 4);
            kgfx_draw_pixel(x + i, y + j, pix);
        }
    }
}
