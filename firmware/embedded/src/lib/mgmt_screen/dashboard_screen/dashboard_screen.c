#include "dashboard_screen.h"

#include <stdio.h>

#include "../api_screen/fonts/FreeSans9pt7b.h"
#include "../api_screen/fonts/FreeSansBold18pt7b.h"
#include "../api_screen/fonts/FreeSansBold24pt7b.h"
#include "../api_screen/fonts/Picopixel.h"
#include "../api_screen/fonts/TomThumb.h"

#define UI_COLOR_WHITE  (0x0E)
#define UI_COLOR_BLACK  (0x00)
#define UI_COLOR_RED    (0x08)
#define UI_COLOR_GREEN  (0x04)
#define UI_COLOR_BLUE   (0x02)
#define UI_COLOR_YELLOW (0x0C)

static const char *const k_weekday_abbr[] = {
	"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"
};

static const char *const k_month_abbr[] = {
	"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
	"JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};

/* ========= Theme default ========= */

void dashboard_theme_default(dashboard_theme_t *t)
{
	if (!t) return;
	t->bg         = UI_COLOR_WHITE;
	t->fg         = UI_COLOR_BLACK;
	t->time_hh    = UI_COLOR_RED;
	t->time_colon = UI_COLOR_GREEN;
	t->time_mm    = UI_COLOR_BLUE;
	t->ble_text   = UI_COLOR_BLACK;
	t->date_text  = UI_COLOR_BLACK;
	t->batt_good  = UI_COLOR_GREEN;
	t->batt_mid   = UI_COLOR_YELLOW;
	t->batt_low   = UI_COLOR_RED;
}

/* ========= Layout builder ========= */

static int16_t clamp_i16(int16_t v, int16_t lo, int16_t hi)
{
	if (v < lo) return lo;
	if (v > hi) return hi;
	return v;
}

void dashboard_layout_build(kgfx_t *g, dashboard_layout_t *l)
{
	int16_t w = kgfx_width(g);
	int16_t h = kgfx_height(g);
	bool small = (w <= 128 || h <= 128);

	l->font_time = small ? &FreeSansBold18pt7b : &FreeSansBold24pt7b;
	l->font_meta = &FreeSans9pt7b;

	l->ble_y = (int16_t)(h * 12 / 100);
	l->time_y = (int16_t)(h * 46 / 100);
	l->date_y = (int16_t)(h * 60 / 100);
	l->icon_y = (int16_t)(h * 83 / 100);

	l->icon_r = small ? 11 : 14;
	l->icon_spacing = (int16_t)(w * 30 / 100);

	l->batt_w = small ? 16 : 22;
	l->batt_h = small ? 8 : 10;
	l->batt_x = (int16_t)(w - l->batt_w - 7);
	l->batt_y = small ? 4 : 6;

	l->ble_y  = clamp_i16(l->ble_y, 14, h - 8);
	l->time_y = clamp_i16(l->time_y, 45, h - 40);
	l->date_y = clamp_i16(l->date_y, 60, h - 24);
	l->icon_y = clamp_i16(l->icon_y, 82, h - 16);
}

static uint8_t battery_color(const dashboard_theme_t *t, uint8_t percent)
{
	if (percent >= 60U) return t->batt_good;
	if (percent >= 25U) return t->batt_mid;
	return t->batt_low;
}

static void draw_battery_charge_bolt(kgfx_t *g, int16_t x, int16_t y,
				      int16_t body_w, int16_t body_h,
				      uint8_t color)
{
	int16_t left = x + body_w / 2 - 4;
	int16_t top = y + 1;
	int16_t bottom = y + body_h - 2;
	int16_t mid = y + body_h / 2;

	kgfx_fill_triangle(g,
			   left + 3, top,
			   left, mid,
			   left + 4, mid,
			   color);
	kgfx_fill_triangle(g,
			   left + 4, mid - 1,
			   left + 8, mid - 1,
			   left + 3, bottom,
			   color);
	kgfx_fill_rect(g, left + 3, mid - 1, 2, 2, color);
}

uint8_t dashboard_brightness_for_time(uint8_t hour, uint8_t minute)
{
	uint16_t t = (uint16_t)((hour % 24U) * 60U + (minute % 60U));

	if (t < 360U) {
		return 18U;
	}
	if (t < 480U) {
		return (uint8_t)(18U + ((t - 360U) * 52U) / 120U);
	}
	if (t < 1020U) {
		return 70U;
	}
	if (t < 1140U) {
		return (uint8_t)(70U - ((t - 1020U) * 24U) / 120U);
	}
	if (t < 1320U) {
		return 46U;
	}
	return 26U;
}

const char *dashboard_ble_label(dashboard_ble_state_t state)
{
	(void)state;
	return "BT";
}

static void draw_ble_state_icon(kgfx_t *g, dashboard_ble_state_t state,
				int16_t x, int16_t y, int16_t size,
				uint8_t color)
{
	int16_t cx;
	int16_t cy;
	int16_t r;

	if (!g) {
		return;
	}

	if (size < 8) {
		size = 8;
	}

	cx = x + size / 2;
	cy = y + size / 2;
	r = size / 2 - 1;

	if (r < 2) {
		r = 2;
	}

	switch (state) {
	case DASHBOARD_BLE_CONNECTED:
		kgfx_draw_circle(g, cx, cy, r, color);
		kgfx_draw_line(g, cx - r / 2, cy, cx - 1, cy + r / 2, color);
		kgfx_draw_line(g, cx - 1, cy + r / 2, cx + r / 2, cy - r / 2, color);
		break;

	case DASHBOARD_BLE_DISCONNECTED:
		kgfx_draw_circle(g, cx, cy, r, color);
		kgfx_draw_line(g, cx - r / 2, cy - r / 2, cx + r / 2, cy + r / 2, color);
		kgfx_draw_line(g, cx - r / 2, cy + r / 2, cx + r / 2, cy - r / 2, color);
		break;

	case DASHBOARD_BLE_ADVERTISING:
	default:
		kgfx_fill_circle(g, cx, cy, 1, color);
		kgfx_draw_line(g, cx, cy + 1, cx, cy + r, color);
		kgfx_draw_line(g, cx, cy + r, cx - 2, cy + r + 2, color);
		kgfx_draw_line(g, cx, cy + r, cx + 2, cy + r + 2, color);

		kgfx_draw_line(g, cx - r, cy - r + 1, cx - r, cy + r - 1, color);
		kgfx_draw_line(g, cx + r, cy - r + 1, cx + r, cy + r - 1, color);
		if (r > 3) {
			kgfx_draw_line(g, cx - r + 2, cy - r + 2, cx - r + 2, cy + r - 2, color);
			kgfx_draw_line(g, cx + r - 2, cy - r + 2, cx + r - 2, cy + r - 2, color);
		}
		break;
	}
}

/* ========= Public widget functions ========= */

void dashboard_draw_battery(kgfx_t *g, const dashboard_view_model_t *vm,
			    const dashboard_layout_t *l, const dashboard_theme_t *t)
{
	uint8_t pct = vm->battery_percent;
	if (pct > 100U) {
		pct = 100U;
	}

	const int16_t body_w = l->batt_w;
	const int16_t body_h = l->batt_h;
	const int16_t cap_w = 2;
	const int16_t cap_h = body_h > 8 ? 4 : 3;

	int16_t x = l->batt_x;
	int16_t y = l->batt_y;

	uint8_t fill_color = battery_color(t, pct);

	kgfx_draw_rect(g, x, y, body_w, body_h, t->fg);
	kgfx_fill_rect(g, x + body_w, y + (body_h - cap_h) / 2, cap_w, cap_h, t->fg);

	int16_t inner_w = body_w - 4;
	int16_t fill_w = (int16_t)((inner_w * pct) / 100U);
	kgfx_fill_rect(g, x + 2, y + 2, inner_w, body_h - 4, t->bg);
	if (fill_w > 0) {
		kgfx_fill_rect(g, x + 2, y + 2, fill_w, body_h - 4, fill_color);
	}

	if (vm->battery_charging) {
		draw_battery_charge_bolt(g, x, y, body_w, body_h, t->fg);
	}

	if (vm->battery_show_text) {
		char batt_txt[8];
		(void)snprintf(batt_txt, sizeof(batt_txt), "%u", pct);
		kgfx_set_font(g, &FreeSans9pt7b);
		kgfx_set_text_color(g, t->fg);
		kgfx_set_cursor(g, x - kgfx_text_width(&FreeSans9pt7b, batt_txt) - 4, y + body_h);
		kgfx_print(g, batt_txt);
	}
}

static uint8_t sensor_color(dashboard_sensor_state_t state)
{
	switch (state) {
	case DASHBOARD_SENSOR_ACTIVE:
		return UI_COLOR_GREEN;
	case DASHBOARD_SENSOR_ERROR:
		return UI_COLOR_RED;
	case DASHBOARD_SENSOR_IDLE:
		return UI_COLOR_YELLOW;
	case DASHBOARD_SENSOR_OFF:
	default:
		return UI_COLOR_BLACK;
	}
}

static char sensor_mark(dashboard_sensor_state_t state)
{
	switch (state) {
	case DASHBOARD_SENSOR_ACTIVE:
		return 'A';
	case DASHBOARD_SENSOR_IDLE:
		return 'I';
	case DASHBOARD_SENSOR_ERROR:
		return '!';
	case DASHBOARD_SENSOR_OFF:
	default:
		return '-';
	}
}

static void draw_icon_base(kgfx_t *g, int16_t cx, int16_t cy, int16_t r,
			   uint8_t border_color, uint8_t bg)
{
	kgfx_draw_circle(g, cx, cy, r, border_color);
	kgfx_fill_circle(g, cx, cy, r - 2, bg);
}

static void draw_text_centered(kgfx_t *g, const GFXfont *font,
			       int16_t cx, int16_t baseline_y,
			       uint8_t color, const char *text)
{
	int16_t tw;

	if (!g || !font || !text) {
		return;
	}

	tw = kgfx_text_width(font, text);
	kgfx_set_font(g, font);
	kgfx_set_text_color(g, color);
	kgfx_set_cursor(g, (int16_t)(cx - tw / 2), baseline_y);
	kgfx_print(g, text);
}

static void draw_state_tag(kgfx_t *g, int16_t cx, int16_t y_top,
			   int16_t w, int16_t h,
			   uint8_t tag_color, const char *text)
{
	if (!g || !text) {
		return;
	}

	kgfx_fill_round_rect(g, (int16_t)(cx - w / 2), y_top, w, h, 1, tag_color);
	kgfx_draw_round_rect(g, (int16_t)(cx - w / 2), y_top, w, h, 1, UI_COLOR_BLACK);

	/* One-char state marker: use larger font for better glance readability. */
	draw_text_centered(g, &FreeSans9pt7b, cx, (int16_t)(y_top + h), UI_COLOR_WHITE, text);
}

void dashboard_draw_notification_icon(kgfx_t *g, const dashboard_layout_t *l,
				      int16_t cx, int16_t cy, uint16_t unread)
{
	char txt[4];
	int16_t mail_w = l->icon_r - 1;
	int16_t mail_h = l->icon_r - 5;
	int16_t badge_r = l->icon_r > 12 ? 8 : 6;
	int16_t badge_off = l->icon_r > 12 ? 11 : 8;

	draw_icon_base(g, cx, cy, l->icon_r, unread > 0 ? UI_COLOR_RED : UI_COLOR_BLACK, UI_COLOR_WHITE);

	kgfx_draw_rect(g, cx - mail_w / 2, cy - mail_h / 2, mail_w, mail_h, UI_COLOR_BLACK);
	kgfx_draw_line(g, cx - mail_w / 2, cy - mail_h / 2, cx, cy + 1, UI_COLOR_BLACK);
	kgfx_draw_line(g, cx + mail_w / 2 - 1, cy - mail_h / 2, cx, cy + 1, UI_COLOR_BLACK);

	if (unread > 0U) {
		int16_t bx = cx + badge_off;
		int16_t by = cy - badge_off;

		if (unread > 9U) {
			(void)snprintf(txt, sizeof(txt), "9+");
			kgfx_fill_circle(g, bx, by, badge_r, UI_COLOR_RED);
			kgfx_draw_circle(g, bx, by, badge_r, UI_COLOR_BLACK);
			draw_text_centered(g, &Picopixel, bx, (int16_t)(by + 4), UI_COLOR_WHITE, txt);
		} else {
			(void)snprintf(txt, sizeof(txt), "%u", unread);
			kgfx_fill_round_rect(g, (int16_t)(bx - badge_r), (int16_t)(by - badge_r + 1),
					    (int16_t)(2 * badge_r), (int16_t)(2 * badge_r), 2, UI_COLOR_RED);
			kgfx_draw_round_rect(g, (int16_t)(bx - badge_r), (int16_t)(by - badge_r + 1),
					    (int16_t)(2 * badge_r), (int16_t)(2 * badge_r), 2, UI_COLOR_BLACK);
			draw_text_centered(g, &FreeSans9pt7b, bx, (int16_t)(by + badge_r - 1), UI_COLOR_WHITE, txt);
		}
	}
}

void dashboard_draw_ppg_icon(kgfx_t *g, const dashboard_layout_t *l,
			     int16_t cx, int16_t cy, dashboard_sensor_state_t state)
{
	uint8_t c = sensor_color(state);
	char mark[2] = { sensor_mark(state), '\0' };
	int16_t r = l->icon_r;
	int16_t tag_w = (r > 12) ? 18 : 15;
	int16_t tag_h = (r > 12) ? 12 : 10;
	int16_t tag_y = (int16_t)(cy - r - 3);

	draw_icon_base(g, cx, cy, r, c, UI_COLOR_WHITE);

	kgfx_draw_line(g, cx - r + 3, cy + 1, cx - 2, cy + 1, c);
	kgfx_draw_line(g, cx - 2, cy + 1, cx, cy - r + 4, c);
	kgfx_draw_line(g, cx, cy - r + 4, cx + 2, cy + r - 6, c);
	kgfx_draw_line(g, cx + 2, cy + r - 6, cx + 4, cy - 1, c);
	kgfx_draw_line(g, cx + 4, cy - 1, cx + r - 3, cy - 1, c);

	draw_state_tag(g, cx, tag_y, tag_w, tag_h, c, mark);
}

void dashboard_draw_imu_icon(kgfx_t *g, const dashboard_layout_t *l,
			     int16_t cx, int16_t cy, dashboard_sensor_state_t state)
{
	uint8_t c = sensor_color(state);
	char mark[2] = { sensor_mark(state), '\0' };
	int16_t r = l->icon_r;
	int16_t tag_w = (r > 12) ? 18 : 15;
	int16_t tag_h = (r > 12) ? 12 : 10;
	int16_t tag_y = (int16_t)(cy - r - 3);

	draw_icon_base(g, cx, cy, r, c, UI_COLOR_WHITE);

	kgfx_draw_line(g, cx, cy - r + 2, cx - r + 3, cy + r - 5, c);
	kgfx_draw_line(g, cx - r + 3, cy + r - 5, cx + r - 3, cy + r - 5, c);
	kgfx_draw_line(g, cx + r - 3, cy + r - 5, cx, cy - r + 2, c);
	kgfx_fill_circle(g, cx, cy + r - 4, 2, c);

	draw_state_tag(g, cx, tag_y, tag_w, tag_h, c, mark);
}

/* ========= Composite widget functions ========= */

void dashboard_draw_ble_status(kgfx_t *g, const dashboard_view_model_t *vm,
			       const dashboard_layout_t *l, const dashboard_theme_t *t)
{
	int16_t icon_size = (kgfx_width(g) <= 128) ? 12 : 16;
	int16_t icon_x = (kgfx_width(g) <= 128) ? 5 : 7;
	int16_t icon_y = l->batt_y;

	draw_ble_state_icon(g, vm->ble_state, icon_x, icon_y, icon_size, t->ble_text);
}

void dashboard_draw_clock(kgfx_t *g, const dashboard_view_model_t *vm,
			  const dashboard_layout_t *l, const dashboard_theme_t *t)
{
	char hh[3], mm[3];
	(void)snprintf(hh, sizeof(hh), "%02u", vm->hour % 24U);
	(void)snprintf(mm, sizeof(mm), "%02u", vm->minute % 60U);

	int16_t w_hh    = kgfx_text_width(l->font_time, hh);
	int16_t w_colon = kgfx_text_width(l->font_time, ":");
	int16_t w_mm    = kgfx_text_width(l->font_time, mm);
	int16_t total   = w_hh + 4 + w_colon + 4 + w_mm;
	int16_t x_hh    = (kgfx_width(g) - total) / 2;
	int16_t x_colon = x_hh + w_hh + 4;
	int16_t x_mm    = x_colon + w_colon + 4;

	kgfx_set_font(g, l->font_time);

	kgfx_set_text_color(g, t->time_hh);
	kgfx_set_cursor(g, x_hh, l->time_y);
	kgfx_print(g, hh);

	kgfx_set_text_color(g, t->time_colon);
	kgfx_set_cursor(g, x_colon, l->time_y);
	kgfx_print(g, ":");

	kgfx_set_text_color(g, t->time_mm);
	kgfx_set_cursor(g, x_mm, l->time_y);
	kgfx_print(g, mm);
}

void dashboard_draw_date(kgfx_t *g, const dashboard_view_model_t *vm,
			 const dashboard_layout_t *l, const dashboard_theme_t *t)
{
	char date_buf[20];
	const char *weekday = (vm->weekday < 7U) ? k_weekday_abbr[vm->weekday] : "DAY";
	const char *month = (vm->month >= 1U && vm->month <= 12U)
			     ? k_month_abbr[vm->month - 1U] : "???";
	(void)snprintf(date_buf, sizeof(date_buf), "%s %u %s", weekday, vm->day, month);
	kgfx_draw_text_aligned(g, l->font_meta, 0, l->date_y,
			       kgfx_width(g), KGFX_ALIGN_CENTER,
			       t->date_text, date_buf);
}

/* ========= Full render ========= */

void dashboard_render_ex(kgfx_t *g, const dashboard_view_model_t *vm,
			 const dashboard_theme_t *theme,
			 const dashboard_layout_t *layout)
{
	if (!g || !vm || !theme || !layout) return;

	int16_t cx = kgfx_width(g) / 2;

	kgfx_fill_screen(g, theme->bg);

	dashboard_draw_battery(g, vm, layout, theme);
	dashboard_draw_ble_status(g, vm, layout, theme);
	dashboard_draw_clock(g, vm, layout, theme);
	dashboard_draw_date(g, vm, layout, theme);

	dashboard_draw_notification_icon(g, layout, cx - layout->icon_spacing, layout->icon_y,
					 vm->unread_messages);
	dashboard_draw_ppg_icon(g, layout, cx, layout->icon_y, vm->ppg_state);
	dashboard_draw_imu_icon(g, layout, cx + layout->icon_spacing, layout->icon_y, vm->imu_state);
}

void dashboard_render(kgfx_t *g, const dashboard_view_model_t *vm)
{
	if (!g || !vm) return;

	dashboard_theme_t theme;
	dashboard_layout_t layout;

	dashboard_theme_default(&theme);
	dashboard_layout_build(g, &layout);

	dashboard_render_ex(g, vm, &theme, &layout);
}

