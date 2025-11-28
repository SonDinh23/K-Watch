#include "face_manager.h"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(face_mgr);

static kw_gfx_t g;

static void face_draw_cb(int16_t x, int16_t y, uint16_t c, void *u)
{
    (void)u;
    cmlcd_draw_pixel(x, y, (uint8_t)(c & 0x0F));
}

static void face_refresh_cb(void *u)
{
    (void)u;
    cmlcd_refresh();
}

void face_manager_init(void)
{
    kw_gfx_init(&g, 176, 176, face_draw_cb, face_refresh_cb, NULL);

    cmlcd_init();
    cmlcd_backlight_set(30);  /* 100% brightness */
    cmlcd_clear_display();

    face_digital_init(&g);

    LOG_INF("Face manager initialized");
}

void face_manager_show_digital(uint8_t hour, uint8_t minute, uint8_t second)
{
    face_digital_update(&g, hour, minute, second);
}

void face_manager_show_digital_with_date(uint8_t hour, uint8_t minute, uint8_t second,
                                          uint8_t day, uint8_t month, uint8_t weekday)
{
    face_digital_update_with_date(&g, hour, minute, second, day, month, weekday);
}

void test_face_manager(const char *app_id, const char *title, const char *notification)
{
    cmlcd_clear_display();

    face_information_frame(&g);
    face_information_App_Identifier_update(&g, app_id);
    face_information_Title_update(&g, title);
    face_information_Notification_update(&g, notification);

    kw_gfx_refresh(&g);
}

