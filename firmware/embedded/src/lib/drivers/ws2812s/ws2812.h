#ifndef WS2812_H
#define WS2812_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	WS2812_COLOR_OFF = 0,
	WS2812_COLOR_RED,
	WS2812_COLOR_GREEN,
	WS2812_COLOR_BLUE,
	WS2812_COLOR_WHITE,
	WS2812_COLOR_PURPLE,
	WS2812_COLOR_ORANGE,
} ws2812_color_t;

/* ===== Unified API style ===== */
int ws2812_init(void);
int ws2812_set_brightness(uint8_t brightness_percent);
int ws2812_set_rgb(uint8_t red, uint8_t green, uint8_t blue);
int ws2812_set_color(ws2812_color_t color);

/* ===== Legacy API (kept for backward compatibility) ===== */
void beginWS2812(void);
void setBrightNess(uint8_t _brightness);
void setBrightColor(uint8_t _red, uint8_t _green, uint8_t _blue);

void red(void);
void green(void);
void blue(void);
void white(void);
void notBright(void);
void purple(void);
void orange(void);

#ifdef __cplusplus
}
#endif

#endif