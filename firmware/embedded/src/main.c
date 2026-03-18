#include <stdint.h>

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "app.h"
#include "lib/drivers/rtc/RV8263_C8.h"
#include "lib/drivers/driver_display_memory/display_memory.h"
#include "lib/mgmt_screen/api_screen/KGFX.h"
#include "lib/mgmt_screen/dashboard_screen/dashboard_screen.h"

#define UI_CLEAR_COLOR 0x0E

LOG_MODULE_REGISTER(main_app, LOG_LEVEL_INF);

static kgfx_t g_ui;
static dashboard_view_model_t g_vm;
static struct rv8263_dev rtc;
static bool g_rtc_ready;

int main(void)
{
        int ret;

        LOG_INF("Dashboard demo started");

        ret = cmlcd_init();
        if (ret) {
                LOG_ERR("Display init failed: %d", ret);
                return ret;
        }

        kgfx_init(&g_ui, LCD_DISP_WIDTH, LCD_DISP_HEIGHT, display_mem_kgfx_pixel_cb, NULL);
        kgfx_set_rotation(&g_ui, 0);
        ret = app_runtime_init(&g_vm, &rtc, &g_rtc_ready);
        if (ret) {
                LOG_WRN("app_runtime_init failed (%d)", ret);
        }

        while (1) {
                uint8_t brightness;

                app_runtime_step(&g_vm, &rtc, &g_rtc_ready, k_uptime_get());
                brightness = dashboard_brightness_for_time(g_vm.hour, g_vm.minute);
                (void)cmlcd_backlight_set(brightness);

                if (app_runtime_consume_screen_changed()) {
                        cmlcd_clear_display();
                        kgfx_fill_screen(&g_ui, UI_CLEAR_COLOR);
                        cmlcd_refresh();
                        k_msleep(8);
                }

                app_runtime_render(&g_ui, &g_vm);
                cmlcd_refresh();

                k_sleep(K_MSEC(250));
        }
}
