#include "WatchAnalog.h"
#include <math.h>
#include <string.h>

/* ====== tiện ích hình học nội bộ ====== */

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* Tâm & bán kính chính của mặt */
#define AW_CX  (LPM_GFX_RAW_WIDTH  / 2)  /* 88 */
#define AW_CY  (LPM_GFX_RAW_HEIGHT / 2)  /* 88 */
#define AW_RADIUS_FACE   82
#define AW_RADIUS_TICKS  78
#define AW_RADIUS_NUM    60  /* vòng đặt số 12-3-6-9 */

/* Đổi độ -> rad */
static inline float deg2rad(float deg) { return (deg * (float)M_PI) / 180.0f; }

/* Tính điểm cực theo góc độ (0° ở trục 12h, tăng theo chiều kim đồng hồ) */
static void polar_point(int cx, int cy, float r, float deg, int *xo, int *yo) {
    float a = deg2rad(deg - 90.0f); /* quay để 0° hướng lên */
    float x = (float)cx + r * cosf(a);
    float y = (float)cy + r * sinf(a);
    *xo = (int)lroundf(x);
    *yo = (int)lroundf(y);
}

/* Vẽ đường tròn rỗng (midpoint) */
static void draw_circle(lpm_gfx_t *g, int cx, int cy, int r, uint16_t c) {
    int f = 1 - r;
    int ddF_x = 1;
    int ddF_y = -2 * r;
    int x = 0;
    int y = r;

    lpm_gfx_draw_pixel(g, cx, cy + r, c);
    lpm_gfx_draw_pixel(g, cx, cy - r, c);
    lpm_gfx_draw_pixel(g, cx + r, cy, c);
    lpm_gfx_draw_pixel(g, cx - r, cy, c);

    while (x < y) {
        if (f >= 0) { y--; ddF_y += 2; f += ddF_y; }
        x++; ddF_x += 2; f += ddF_x;

        lpm_gfx_draw_pixel(g, cx + x, cy + y, c);
        lpm_gfx_draw_pixel(g, cx - x, cy + y, c);
        lpm_gfx_draw_pixel(g, cx + x, cy - y, c);
        lpm_gfx_draw_pixel(g, cx - x, cy - y, c);
        lpm_gfx_draw_pixel(g, cx + y, cy + x, c);
        lpm_gfx_draw_pixel(g, cx - y, cy + x, c);
        lpm_gfx_draw_pixel(g, cx + y, cy - x, c);
        lpm_gfx_draw_pixel(g, cx - y, cy - x, c);
    }
}

/* Vẽ hình tròn tô đầy bằng các scanline ngang */
static void fill_circle(lpm_gfx_t *g, int cx, int cy, int r, uint16_t c) {
    int x = r, y = 0;
    int err = 0;

    while (x >= y) {
        /* 4 scanline dày 1 px ở các y đối xứng */
        lpm_gfx_draw_fast_hline(g, cx - x, cy + y, 2*x + 1, c);
        lpm_gfx_draw_fast_hline(g, cx - y, cy + x, 2*y + 1, c);
        lpm_gfx_draw_fast_hline(g, cx - x, cy - y, 2*x + 1, c);
        lpm_gfx_draw_fast_hline(g, cx - y, cy - x, 2*y + 1, c);
        y++;
        if (err <= 0) { err += 2*y + 1; }
        if (err > 0) { x--; err -= 2*x + 1; }
    }
}

/* Vẽ “đường dày” bằng cách dập các vòng tròn nhỏ theo đường thẳng (stroke tròn) */
static void draw_thick_line(lpm_gfx_t *g, int x0, int y0, int x1, int y1, int thickness, uint16_t c) {
    if (thickness <= 1) {
        lpm_gfx_draw_line(g, x0, y0, x1, y1, c);
        return;
    }
    int r = thickness / 2;
    int dx = x1 - x0, dy = y1 - y0;
    int steps = abs(dx) > abs(dy) ? abs(dx) : abs(dy);
    if (steps == 0) { fill_circle(g, x0, y0, r, c); return; }
    /* bước đi nhỏ hơn bán kính để không bị đứt nét */
    float sx = (float)dx / (float)steps;
    float sy = (float)dy / (float)steps;
    float x = (float)x0, y = (float)y0;
    for (int i = 0; i <= steps; i++) {
        fill_circle(g, (int)lroundf(x), (int)lroundf(y), r, c);
        x += sx; y += sy;
    }
}

/* ====== Theme mặc định ====== */
void analog_watch_get_default_theme(analog_watch_theme_t *t) {
    if (!t) return;
    t->bg         = lpm_gfx_Color565(6, 10, 20);     /* xanh đen */
    t->dial       = lpm_gfx_Color565(10, 16, 30);    /* mặt hơi sáng hơn */
    t->ring_outer = lpm_gfx_Color565(180, 180, 180); /* viền xám nhạt */
    t->tick_min   = lpm_gfx_Color565(120, 120, 130); /* vạch phút xám */
    t->tick_5min  = lpm_gfx_Color565(240, 240, 240); /* vạch 5 phút sáng */
    t->numeral    = lpm_gfx_Color565(240, 240, 240); /* số sáng */
    t->hand_hour  = lpm_gfx_Color565(240, 240, 240); /* kim giờ trắng */
    t->hand_min   = lpm_gfx_Color565(180, 220, 255); /* kim phút xanh nhạt */
    t->hand_sec   = lpm_gfx_Color565(230, 40, 40);   /* kim giây đỏ */
    t->hub        = lpm_gfx_Color565(255, 255, 255); /* chốt trắng */
    t->accent     = lpm_gfx_Color565(0, 220, 220);   /* accent cyan */
}

/* Vẽ số (12/3/6/9) căn giữa thô theo font 5x7 scale */
static void draw_centered_num(lpm_gfx_t *g, int x, int y, const char *txt, uint8_t sx, uint8_t sy, uint16_t fg) {
    int w_char = 6 * sx;
    int h_char = 8 * sy;
    int len = 0; while (txt[len]) len++;

    int total_w = w_char * len;
    int total_h = h_char;
    int x0 = x - total_w/2;
    int y0 = y - total_h/2;

    lpm_gfx_set_text_color(g, fg);
    lpm_gfx_set_text_size(g, sx, sy);
    lpm_gfx_set_cursor(g, x0, y0);
    lpm_gfx_write(g, txt);
}

/* Vẽ các vạch phút/giờ */
static void draw_ticks(lpm_gfx_t *g, const analog_watch_theme_t *t) {
    for (int i = 0; i < 60; i++) {
        float deg = (float)i * 6.0f; /* 360/60 */
        int x0, y0, x1, y1;
        int inner = (i % 5 == 0) ? (AW_RADIUS_TICKS - 10) : (AW_RADIUS_TICKS - 5);
        int thick = (i % 5 == 0) ? 3 : 1;

        polar_point(AW_CX, AW_CY, (float)inner, deg, &x0, &y0);
        polar_point(AW_CX, AW_CY, (float)AW_RADIUS_TICKS, deg, &x1, &y1);
        uint16_t col = (i % 5 == 0) ? t->tick_5min : t->tick_min;
        draw_thick_line(g, x0, y0, x1, y1, thick, col);
    }
}

/* Vẽ 4 số lớn 12,3,6,9 */
static void draw_numerals(lpm_gfx_t *g, const analog_watch_theme_t *t) {
    /* 12 */
    { int x,y; polar_point(AW_CX, AW_CY, (float)AW_RADIUS_NUM, 0, &x, &y);
      draw_centered_num(g, x, y, "12", 2, 2, t->numeral); }
    /* 3 */
    { int x,y; polar_point(AW_CX, AW_CY, (float)AW_RADIUS_NUM, 90, &x, &y);
      draw_centered_num(g, x, y, "3",  2, 2, t->numeral); }
    /* 6 */
    { int x,y; polar_point(AW_CX, AW_CY, (float)AW_RADIUS_NUM, 180, &x, &y);
      draw_centered_num(g, x, y, "6",  2, 2, t->numeral); }
    /* 9 */
    { int x,y; polar_point(AW_CX, AW_CY, (float)AW_RADIUS_NUM, 270, &x, &y);
      draw_centered_num(g, x, y, "9",  2, 2, t->numeral); }
}

/* ====== API chính ====== */

void analog_watch_draw_face(lpm_gfx_t *g, const analog_watch_theme_t *t) {
    if (!g || !t) return;

    /* Nền */
    lpm_gfx_fill_screen(g, t->bg);

    /* Mặt trong (tuỳ theme, có thể cùng màu bg) */
    fill_circle(g, AW_CX, AW_CY, AW_RADIUS_FACE, t->dial);

    /* Bezel/viền */
    draw_circle(g, AW_CX, AW_CY, AW_RADIUS_FACE, t->ring_outer);
    draw_circle(g, AW_CX, AW_CY, AW_RADIUS_TICKS+1, t->ring_outer);

    /* Vạch */
    draw_ticks(g, t);

    /* Số */
    draw_numerals(g, t);

    /* Logo/nhãn nhỏ (tuỳ thích) */
    lpm_gfx_set_text_color(g, t->accent);
    lpm_gfx_set_text_size(g, 1, 1);
    lpm_gfx_set_cursor(g, AW_CX - 20, AW_CY + 36);
    lpm_gfx_write(g, "MEM-LCD");
}

/* Vẽ kim theo hướng, chiều dài và độ dày, thêm “đuôi” ngắn cho đẹp */
static void draw_hand(lpm_gfx_t *g, float deg, float r_len, int thick, uint16_t col, bool tail) {
    int x_tip, y_tip, x_tail, y_tail;
    polar_point(AW_CX, AW_CY, r_len, deg, &x_tip, &y_tip);
    draw_thick_line(g, AW_CX, AW_CY, x_tip, y_tip, thick, col);

    /* chóp tròn + chấm accent ở đầu */
    fill_circle(g, x_tip, y_tip, thick/2 + 1, col);

    if (tail) {
        polar_point(AW_CX, AW_CY, r_len * 0.18f, deg + 180.0f, &x_tail, &y_tail);
        draw_thick_line(g, AW_CX, AW_CY, x_tail, y_tail, thick-1 > 1 ? thick-1 : 1, col);
    }
}

void analog_watch_draw_time(lpm_gfx_t *g, const analog_watch_theme_t *t,
                            int hour, int minute, int second)
{
    if (!g || !t) return;

    /* Chuẩn hoá */
    if (hour < 0) hour = 0;
    if (minute < 0) minute = 0;
    if (second < 0) second = 0;

    /* Góc kim (độ) — chuẩn đồng hồ */
    float deg_sec  = (float)second * 6.0f;                  /* 6 deg mỗi giây */
    float deg_min  = (float)minute * 6.0f + (float)second * 0.1f;
    int   h12 = hour % 12;
    float deg_hour = (float)h12 * 30.0f + (float)minute * 0.5f; /* 30 deg mỗi giờ + bù phút */

    /* Kim giờ (ngắn, dày) */
    draw_hand(g, deg_hour, 48.0f, 7, t->hand_hour, true);

    /* Kim phút (dài, vừa) */
    draw_hand(g, deg_min,  66.0f, 5, t->hand_min, true);

    /* Kim giây (mảnh, rất dài) */
    draw_hand(g, deg_sec,  73.0f, 2, t->hand_sec, true);

    /* Chốt trung tâm */
    fill_circle(g, AW_CX, AW_CY, 4, t->hub);
    draw_circle(g, AW_CX, AW_CY, 5, t->tick_5min);
}

void analog_watch_draw(lpm_gfx_t *g, const analog_watch_theme_t *t,
                       int hour, int minute, int second, bool do_display)
{
    analog_watch_draw_face(g, t);
    analog_watch_draw_time(g, t, hour, minute, second);
    if (do_display) lpm_gfx_display(g);
}

/* Ví dụ demo: tự đếm thời gian bằng uptime (không dùng RTC, chỉ để test) */
void analog_watch_draw_now(lpm_gfx_t *g, const analog_watch_theme_t *t) {
    uint64_t ms = k_uptime_get();
    int total_sec = (int)(ms / 1000ULL);
    int second = total_sec % 60;
    int minute = (total_sec / 60) % 60;
    int hour   = (total_sec / 3600) % 12;

    analog_watch_draw(g, t, hour, minute, second, true);
}
