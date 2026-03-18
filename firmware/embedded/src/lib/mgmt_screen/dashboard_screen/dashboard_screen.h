#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "../api_screen/KGFX.h"

/* ========= Enums ========= */

typedef enum {
	DASHBOARD_BLE_ADVERTISING = 0,
	DASHBOARD_BLE_DISCONNECTED,
	DASHBOARD_BLE_CONNECTED,
} dashboard_ble_state_t;

typedef enum {
	DASHBOARD_SENSOR_OFF = 0,
	DASHBOARD_SENSOR_IDLE,
	DASHBOARD_SENSOR_ACTIVE,
	DASHBOARD_SENSOR_ERROR,
} dashboard_sensor_state_t;

/* ========= View Model ========= */

typedef struct {
	uint8_t hour;
	uint8_t minute;

	uint8_t weekday; /* 0..6 => SUN..SAT */
	uint8_t day;     /* 1..31 */
	uint8_t month;   /* 1..12 */

	dashboard_ble_state_t ble_state;

	uint16_t unread_messages;
	dashboard_sensor_state_t ppg_state;
	dashboard_sensor_state_t imu_state;

	uint8_t battery_percent;
	bool battery_show_text;
	bool battery_charging;
} dashboard_view_model_t;

/* ========= Theme (configurable colors) ========= */

typedef struct {
	uint8_t bg;
	uint8_t fg;
	uint8_t time_hh;
	uint8_t time_colon;
	uint8_t time_mm;
	uint8_t ble_text;
	uint8_t date_text;
	uint8_t batt_good;   /* >= 60 % */
	uint8_t batt_mid;    /* >= 25 % */
	uint8_t batt_low;    /* < 25 %  */
} dashboard_theme_t;

/* ========= Layout (responsive positioning) ========= */

typedef struct {
	const GFXfont *font_time;
	const GFXfont *font_meta;
	int16_t ble_y;
	int16_t time_y;
	int16_t date_y;
	int16_t icon_y;
	int16_t icon_r;
	int16_t icon_spacing;
	int16_t batt_x;
	int16_t batt_y;
	int16_t batt_w;
	int16_t batt_h;
} dashboard_layout_t;

/* ========= Defaults ========= */

void dashboard_theme_default(dashboard_theme_t *t);
void dashboard_layout_build(kgfx_t *g, dashboard_layout_t *l);

/* ========= Full render (convenience) ========= */

const char *dashboard_ble_label(dashboard_ble_state_t state);
uint8_t dashboard_brightness_for_time(uint8_t hour, uint8_t minute);

/** Simple render with default theme + auto-computed layout */
void dashboard_render(kgfx_t *g, const dashboard_view_model_t *vm);

/** Full-control render with custom theme and layout */
void dashboard_render_ex(kgfx_t *g, const dashboard_view_model_t *vm,
			 const dashboard_theme_t *theme,
			 const dashboard_layout_t *layout);

/* ========= Individual widgets (for custom composition) ========= */

void dashboard_draw_battery(kgfx_t *g, const dashboard_view_model_t *vm,
			    const dashboard_layout_t *l, const dashboard_theme_t *t);
void dashboard_draw_ble_status(kgfx_t *g, const dashboard_view_model_t *vm,
			       const dashboard_layout_t *l, const dashboard_theme_t *t);
void dashboard_draw_clock(kgfx_t *g, const dashboard_view_model_t *vm,
			  const dashboard_layout_t *l, const dashboard_theme_t *t);
void dashboard_draw_date(kgfx_t *g, const dashboard_view_model_t *vm,
			 const dashboard_layout_t *l, const dashboard_theme_t *t);
void dashboard_draw_notification_icon(kgfx_t *g, const dashboard_layout_t *l,
				      int16_t cx, int16_t cy, uint16_t unread);
void dashboard_draw_ppg_icon(kgfx_t *g, const dashboard_layout_t *l,
			     int16_t cx, int16_t cy, dashboard_sensor_state_t state);
void dashboard_draw_imu_icon(kgfx_t *g, const dashboard_layout_t *l,
			     int16_t cx, int16_t cy, dashboard_sensor_state_t state);
