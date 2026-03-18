#pragma once

#include <stdint.h>

/* Select one panel implementation based on devicetree. */
#if DT_NODE_EXISTS(DT_NODELABEL(lpm013m126a))
#include "LPM013M126A.h"
#elif DT_NODE_EXISTS(DT_NODELABEL(ls013b7dh03))
#include "LS013B7DH03.h"
#else
#error "No supported display memory panel node found"
#endif

/* ===== Unified API style (non-breaking wrappers around cmlcd_*) ===== */

static inline int display_mem_init(void)
{
	return cmlcd_init();
}

static inline int display_mem_set_backlight(uint8_t percent)
{
	return cmlcd_backlight_set(percent);
}

static inline void display_mem_draw_pixel(int16_t x, int16_t y, uint8_t color)
{
	cmlcd_draw_pixel(x, y, color);
}

static inline void display_mem_clear_buffer(void)
{
	cmlcd_cls();
}

static inline void display_mem_clear_panel(void)
{
	cmlcd_clear_display();
}

static inline void display_mem_refresh(void)
{
	cmlcd_refresh();
}

static inline int display_mem_set_blink_mode(uint8_t mode)
{
#if DT_NODE_EXISTS(DT_NODELABEL(ls013b7dh03))
	return cmlcd_set_blink_mode(mode);
#else
	cmlcd_set_blink_mode(mode);
	return 0;
#endif
}

static inline void display_mem_set_trans_mode(uint8_t mode)
{
	cmlcd_set_trans_mode(mode);
}

/* ===== KGFX integration ===== */

#define DISPLAY_MEM_IS_MONO DT_NODE_EXISTS(DT_NODELABEL(ls013b7dh03))

static inline uint8_t display_mem_ui_to_panel_color(uint8_t ui_color)
{
#if DISPLAY_MEM_IS_MONO
	return (ui_color == 0x0E) ? LCD_COLOR_WHITE : LCD_COLOR_BLACK;
#else
	return ui_color;
#endif
}

static inline void display_mem_kgfx_pixel_cb(int16_t x, int16_t y, uint8_t color, void *user)
{
	(void)user;
	cmlcd_draw_pixel(x, y, display_mem_ui_to_panel_color(color));
}
