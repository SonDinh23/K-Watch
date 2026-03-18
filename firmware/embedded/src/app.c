#include "app.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/mfd/npm13xx.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>

#include "lib/drivers/ws2812s/ws2812.h"
#include "lib/mgmt_screen/menu_screen/menu_screen.h"
#include "lib/mgmt_screen/notification_screen/notification_screen.h"
#include "lib/mgmt_screen/api_screen/fonts/FreeSans9pt7b.h"
#include "lib/mgmt_screen/api_screen/fonts/FreeSansBold12pt7b.h"
#include "lib/mgmt_system/fuelgauge/fuel_gauge.h"

LOG_MODULE_REGISTER(app_runtime, LOG_LEVEL_INF);

#define BTN_EVENT_MENU   BIT(0)
#define BTN_EVENT_UP     BIT(1)
#define BTN_EVENT_DOWN   BIT(2)
#define BTN_EVENT_SELECT BIT(3)

#if DT_NODE_EXISTS(DT_NODELABEL(npm1300_pmic))
#define NPM13XX_DEVICE(dev) DEVICE_DT_GET(DT_NODELABEL(npm1300_ ## dev))
#elif DT_NODE_EXISTS(DT_NODELABEL(npm1304_pmic))
#define NPM13XX_DEVICE(dev) DEVICE_DT_GET(DT_NODELABEL(npm1304_ ## dev))
#else
#error "neither npm1300 nor npm1304 found in devicetree"
#endif

#define UI_COLOR_WHITE (0x0E)
#define UI_COLOR_BLACK (0x00)

typedef enum {
	APP_SCREEN_DASHBOARD = 0,
	APP_SCREEN_MENU,
	APP_SCREEN_NOTIFICATION,
	APP_SCREEN_FEATURE,
} app_screen_t;

typedef struct {
	const char *name;
	bool enabled;
} feature_item_t;

static const struct gpio_dt_spec g_btn_menu = GPIO_DT_SPEC_GET_OR(DT_ALIAS(btn0), gpios, {0});
static const struct gpio_dt_spec g_btn_up = GPIO_DT_SPEC_GET_OR(DT_ALIAS(btn1), gpios, {0});
static const struct gpio_dt_spec g_btn_down = GPIO_DT_SPEC_GET_OR(DT_ALIAS(btn2), gpios, {0});
static const struct gpio_dt_spec g_btn_select = GPIO_DT_SPEC_GET_OR(DT_ALIAS(btn3), gpios, {0});

static struct gpio_callback g_btn_menu_cb;
static struct gpio_callback g_btn_up_cb;
static struct gpio_callback g_btn_down_cb;
static struct gpio_callback g_btn_select_cb;

static atomic_t g_button_events;

static const struct device *g_pmic = NPM13XX_DEVICE(pmic);
static const struct device *g_charger = NPM13XX_DEVICE(charger);
static struct gpio_callback g_pmic_event_cb;
static volatile bool g_vbus_connected;
static bool g_fuel_gauge_ready;

static app_screen_t g_active_screen = APP_SCREEN_DASHBOARD;
static bool g_screen_changed;
static uint8_t g_menu_selected;
static uint8_t g_feature_selected;
static uint8_t g_notification_selected;

static int64_t g_last_time_update_ms;

static notification_item_t g_notifications[] = {
	{ "Battery", "Low at 18%", 1U, true },
	{ "Bluetooth", "Phone linked", 2U, false },
	{ "Reminder", "Take a short walk", 5U, true },
};

static feature_item_t g_features[] = {
	{ "EMG", true },
	{ "IMU", true },
	{ "PPG", true },
};

static const char *const g_menu_items[] = {
	"Notifications",
	"EMG",
	"IMU",
	"PPG",
};

static void pmic_event_callback(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(cb);

	if (pins & BIT(NPM13XX_EVENT_VBUS_DETECTED)) {
		g_vbus_connected = true;
	}

	if (pins & BIT(NPM13XX_EVENT_VBUS_REMOVED)) {
		g_vbus_connected = false;
	}
}

static int init_fuel_gauge(void)
{
	int ret;
	struct sensor_value val;

	g_fuel_gauge_ready = false;

	if (!device_is_ready(g_pmic)) {
		LOG_WRN("PMIC device not ready");
		return -ENODEV;
	}

	if (!device_is_ready(g_charger)) {
		LOG_WRN("Charger device not ready");
		return -ENODEV;
	}

	ret = fuel_gauge_init(g_charger);
	if (ret < 0) {
		LOG_WRN("Fuel gauge init failed: %d", ret);
		return ret;
	}

	gpio_init_callback(&g_pmic_event_cb, pmic_event_callback,
			   BIT(NPM13XX_EVENT_VBUS_DETECTED) |
			   BIT(NPM13XX_EVENT_VBUS_REMOVED));

	ret = mfd_npm13xx_add_callback(g_pmic, &g_pmic_event_cb);
	if (ret) {
		LOG_WRN("PMIC callback failed: %d", ret);
		return ret;
	}

	ret = sensor_attr_get(g_charger, SENSOR_CHAN_CURRENT, SENSOR_ATTR_UPPER_THRESH, &val);
	if (ret < 0) {
		LOG_WRN("VBUS status read failed: %d", ret);
		return ret;
	}

	g_vbus_connected = (val.val1 != 0) || (val.val2 != 0);
	g_fuel_gauge_ready = true;

	LOG_INF("Fuel gauge ready");
	return 0;
}

static void set_active_screen(app_screen_t next)
{
	if (g_active_screen != next) {
		g_active_screen = next;
		g_screen_changed = true;
	}
}

static uint8_t calc_weekday(uint16_t year, uint8_t month, uint8_t day)
{
	/* 0 = Sunday .. 6 = Saturday */
	static const uint8_t t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};

	if (month < 3U) {
		year--;
	}

	return (uint8_t)((year + year / 4U - year / 100U + year / 400U + t[month - 1U] + day) % 7U);
}

static void update_unread_count(dashboard_view_model_t *vm)
{
	uint16_t unread = 0U;

	for (size_t i = 0; i < ARRAY_SIZE(g_notifications); i++) {
		if (g_notifications[i].unread) {
			unread++;
		}
	}

	vm->unread_messages = unread;
}

static void apply_rtc_to_vm(dashboard_view_model_t *vm, const struct rv8263_time *t)
{
	vm->hour = t->hours;
	vm->minute = t->minutes;
	vm->day = t->date;
	vm->month = t->month;
	vm->weekday = calc_weekday(t->year, t->month, t->date);
}

static void fallback_tick_vm(dashboard_view_model_t *vm)
{
	vm->minute++;
	if (vm->minute < 60U) {
		return;
	}

	vm->minute = 0U;
	vm->hour++;
	if (vm->hour < 24U) {
		return;
	}

	vm->hour = 0U;
	vm->weekday = (uint8_t)((vm->weekday + 1U) % 7U);
}

static int setup_one_button(const struct gpio_dt_spec *spec,
			   struct gpio_callback *cb,
			   gpio_callback_handler_t handler,
			   const char *name)
{
	int ret;

	if (!spec->port || !device_is_ready(spec->port)) {
		LOG_WRN("%s button unavailable", name);
		return -ENODEV;
	}

	ret = gpio_pin_configure_dt(spec, GPIO_INPUT);
	if (ret) {
		LOG_WRN("%s configure failed: %d", name, ret);
		return ret;
	}

	ret = gpio_pin_interrupt_configure_dt(spec, GPIO_INT_EDGE_TO_ACTIVE);
	if (ret) {
		LOG_WRN("%s irq setup failed: %d", name, ret);
		return ret;
	}

	gpio_init_callback(cb, handler, BIT(spec->pin));
	ret = gpio_add_callback(spec->port, cb);
	if (ret) {
		LOG_WRN("%s callback failed: %d", name, ret);
	}

	return ret;
}

static void btn_menu_isr(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);
	atomic_or(&g_button_events, BTN_EVENT_MENU);
}

static void btn_up_isr(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);
	atomic_or(&g_button_events, BTN_EVENT_UP);
}

static void btn_down_isr(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);
	atomic_or(&g_button_events, BTN_EVENT_DOWN);
}

static void btn_select_isr(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);
	atomic_or(&g_button_events, BTN_EVENT_SELECT);
}

static void process_navigation_events(dashboard_view_model_t *vm)
{
	atomic_val_t events = atomic_set(&g_button_events, 0);

	if (events & BTN_EVENT_MENU) {
		if (g_active_screen == APP_SCREEN_DASHBOARD) {
			set_active_screen(APP_SCREEN_MENU);
		} else {
			set_active_screen(APP_SCREEN_DASHBOARD);
		}
	}

	if (events & BTN_EVENT_UP) {
		if (g_active_screen == APP_SCREEN_MENU && g_menu_selected > 0U) {
			g_menu_selected--;
		} else if (g_active_screen == APP_SCREEN_NOTIFICATION && g_notification_selected > 0U) {
			g_notification_selected--;
		}
	}

	if (events & BTN_EVENT_DOWN) {
		if (g_active_screen == APP_SCREEN_MENU &&
		    g_menu_selected + 1U < ARRAY_SIZE(g_menu_items)) {
			g_menu_selected++;
		} else if (g_active_screen == APP_SCREEN_NOTIFICATION &&
			   g_notification_selected + 1U < ARRAY_SIZE(g_notifications)) {
			g_notification_selected++;
		}
	}

	if (events & BTN_EVENT_SELECT) {
		if (g_active_screen == APP_SCREEN_MENU) {
			if (g_menu_selected == 0U) {
				set_active_screen(APP_SCREEN_NOTIFICATION);
			} else {
				g_feature_selected = (uint8_t)(g_menu_selected - 1U);
				set_active_screen(APP_SCREEN_FEATURE);
			}
		} else if (g_active_screen == APP_SCREEN_NOTIFICATION) {
			g_notifications[g_notification_selected].unread = false;
			update_unread_count(vm);
		} else if (g_active_screen == APP_SCREEN_FEATURE) {
			g_features[g_feature_selected].enabled = !g_features[g_feature_selected].enabled;
		}
	}
}

static void render_feature_screen(kgfx_t *g)
{
	char state_text[16];
	const char *feature = g_features[g_feature_selected].name;
	const bool enabled = g_features[g_feature_selected].enabled;

	snprintk(state_text, sizeof(state_text), "%s", enabled ? "Enabled" : "Disabled");

	kgfx_fill_screen(g, UI_COLOR_WHITE);
	kgfx_draw_text_aligned(g, &FreeSansBold12pt7b, 0, 28, kgfx_width(g),
			       KGFX_ALIGN_CENTER, UI_COLOR_BLACK, feature);
	kgfx_draw_text_aligned(g, &FreeSans9pt7b, 0, 58, kgfx_width(g),
			       KGFX_ALIGN_CENTER, UI_COLOR_BLACK, state_text);
	kgfx_draw_text_aligned(g, &FreeSans9pt7b, 0, 86, kgfx_width(g),
			       KGFX_ALIGN_CENTER, UI_COLOR_BLACK, "Select: toggle");
	kgfx_draw_text_aligned(g, &FreeSans9pt7b, 0, 104, kgfx_width(g),
			       KGFX_ALIGN_CENTER, UI_COLOR_BLACK, "Menu: back");
}

int app_runtime_init(dashboard_view_model_t *vm, struct rv8263_dev *rtc, bool *rtc_ready)
{
	int ret;
	struct rv8263_time t;

	if (!vm || !rtc || !rtc_ready) {
		return -EINVAL;
	}

	vm->hour = 9U;
	vm->minute = 41U;
	vm->weekday = 4U;
	vm->day = 12U;
	vm->month = 12U;
	vm->ble_state = DASHBOARD_BLE_DISCONNECTED;
	vm->ppg_state = DASHBOARD_SENSOR_ACTIVE;
	vm->imu_state = DASHBOARD_SENSOR_IDLE;
	vm->battery_percent = 72U;
	vm->battery_charging = false;
	vm->battery_show_text = true;
	update_unread_count(vm);

	g_last_time_update_ms = k_uptime_get();
	g_active_screen = APP_SCREEN_DASHBOARD;
	g_screen_changed = true;
	g_menu_selected = 0U;
	g_feature_selected = 0U;
	g_notification_selected = 0U;
	atomic_set(&g_button_events, 0);

	ret = setup_one_button(&g_btn_menu, &g_btn_menu_cb, btn_menu_isr, "menu");
	if (ret) {
		LOG_WRN("menu button disabled");
	}
	ret = setup_one_button(&g_btn_up, &g_btn_up_cb, btn_up_isr, "up");
	if (ret) {
		LOG_WRN("up button disabled");
	}
	ret = setup_one_button(&g_btn_down, &g_btn_down_cb, btn_down_isr, "down");
	if (ret) {
		LOG_WRN("down button disabled");
	}
	ret = setup_one_button(&g_btn_select, &g_btn_select_cb, btn_select_isr, "select");
	if (ret) {
		LOG_WRN("select button disabled");
	}

	ret = ws2812_init();
	if (ret) {
		LOG_WRN("WS2812 init failed: %d", ret);
	}

	ret = init_fuel_gauge();
	if (ret) {
		LOG_WRN("Using fallback battery value");
	}
	(void)ws2812_set_brightness(20U);
	(void)ws2812_set_color(WS2812_COLOR_BLUE);

	ret = rv8263_init_default(rtc);
	if (ret) {
		*rtc_ready = false;
		LOG_WRN("RTC init failed: %d", ret);
		return 0;
	}

	ret = rv8263_get_time(rtc, &t);
	if (ret == -ENODATA) {
		(void)rv8263_set_time_from_compile(rtc);
		ret = rv8263_get_time(rtc, &t);
	}

	if (ret) {
		*rtc_ready = false;
		LOG_WRN("RTC read failed: %d", ret);
		return 0;
	}

	apply_rtc_to_vm(vm, &t);
	*rtc_ready = true;
	return 0;
}

void app_runtime_step(dashboard_view_model_t *vm, struct rv8263_dev *rtc, bool *rtc_ready,
		      int64_t uptime_ms)
{
	if (!vm || !rtc || !rtc_ready) {
		return;
	}

	process_navigation_events(vm);

	if (uptime_ms - g_last_time_update_ms >= 1000) {
		g_last_time_update_ms = uptime_ms;
		if (*rtc_ready) {
			struct rv8263_time t;
			int ret = rv8263_get_time(rtc, &t);

			if (ret == 0) {
				apply_rtc_to_vm(vm, &t);
			} else {
				*rtc_ready = false;
				fallback_tick_vm(vm);
			}
		} else {
			fallback_tick_vm(vm);
		}

		if (vm->ble_state == DASHBOARD_BLE_CONNECTED) {
			(void)ws2812_set_color(WS2812_COLOR_GREEN);
		} else {
			(void)ws2812_set_color(WS2812_COLOR_BLUE);
		}

		if (g_fuel_gauge_ready) {
			bool charging = vm->battery_charging;
			uint8_t soc = vm->battery_percent;
			int ret = fuel_gauge_update(g_charger, g_vbus_connected, &charging, &soc);

			if (ret == 0) {
				vm->battery_percent = soc;
				vm->battery_charging = charging;
			} else {
				LOG_WRN("Fuel gauge update failed: %d", ret);
			}
		}
	}
}

void app_runtime_render(kgfx_t *g, const dashboard_view_model_t *vm)
{
	if (!g || !vm) {
		return;
	}

	switch (g_active_screen) {
	case APP_SCREEN_DASHBOARD:
		dashboard_render(g, vm);
		break;
	case APP_SCREEN_MENU:
		menu_screen_render(g, "Menu", g_menu_items,
				   (uint8_t)ARRAY_SIZE(g_menu_items), g_menu_selected);
		break;
	case APP_SCREEN_NOTIFICATION:
		notification_screen_render(g, g_notifications,
					   ARRAY_SIZE(g_notifications), g_notification_selected);
		break;
	case APP_SCREEN_FEATURE:
		render_feature_screen(g);
		break;
	default:
		dashboard_render(g, vm);
		break;
	}
}

bool app_runtime_consume_screen_changed(void)
{
	bool changed = g_screen_changed;

	g_screen_changed = false;
	return changed;
}
