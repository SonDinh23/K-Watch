#pragma once
#ifndef ANALOG_WATCH_H
#define ANALOG_WATCH_H

#include <stdint.h>
#include <stdbool.h>

/* Thư viện GFX bạn đã có (các hàm lpm_gfx_*) */
#include "../LPM013M126A/LPM013M126A.h" /* chứa lpm_gfx_t + API */

/* Cấu hình màu sắc cho giao diện */
typedef struct {
    uint16_t bg;          /* nền */
    uint16_t dial;        /* mặt trong (nếu muốn khác bg) */
    uint16_t ring_outer;  /* vòng bezel/viền */
    uint16_t tick_min;    /* vạch mỗi phút */
    uint16_t tick_5min;   /* vạch mỗi 5 phút */
    uint16_t numeral;     /* chữ số 12,3,6,9 */
    uint16_t hand_hour;   /* kim giờ */
    uint16_t hand_min;    /* kim phút */
    uint16_t hand_sec;    /* kim giây */
    uint16_t hub;         /* chốt trung tâm */
    uint16_t accent;      /* điểm nhấn (ví dụ chóp kim giây) */
} analog_watch_theme_t;

/* Một theme mặc định (dark) */
void analog_watch_get_default_theme(analog_watch_theme_t *t);

/* Vẽ cố định phần mặt (nền, vòng, vạch, số) — gọi một lần, hoặc mỗi lần update nếu bạn muốn đơn giản */
void analog_watch_draw_face(lpm_gfx_t *g, const analog_watch_theme_t *t);

/* Vẽ 3 kim theo giờ-phút-giây (giờ 0..23 hoặc 1..12 đều được) */
void analog_watch_draw_time(lpm_gfx_t *g, const analog_watch_theme_t *t,
                            int hour, int minute, int second);

/* Vẽ cả mặt + kim, rồi display() luôn (đơn giản nhất) */
void analog_watch_draw(lpm_gfx_t *g, const analog_watch_theme_t *t,
                       int hour, int minute, int second, bool do_display);

/* (Tuỳ chọn) Ví dụ cập nhật theo thời gian uptime (demo) */
void analog_watch_draw_now(lpm_gfx_t *g, const analog_watch_theme_t *t);

#endif /* ANALOG_WATCH_H */
