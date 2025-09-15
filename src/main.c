#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include "Drivers/Screens/LPM013M126A/LPM013M126A.h"
#include "Drivers/Screens/WatchAnalog/WatchAnalog.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);
/* tô kín 1 màu */
static void draw_solid(uint8_t color) {
    for (int y = 0; y < LCD_DEVICE_HEIGHT; ++y) {
        for (int x = 0; x < LCD_DEVICE_WIDTH; ++x) {
            cmlcd_draw_pixel(x, y, color);
        }
    }
}

/* caro để dễ quan sát */
static void draw_checkerboard(int cell) {
    for (int y = 0; y < LCD_DEVICE_HEIGHT; ++y) {
        for (int x = 0; x < LCD_DEVICE_WIDTH; ++x) {
            uint8_t c = ((x / cell) + (y / cell)) & 0x0F;
            cmlcd_draw_pixel(x, y, c);
        }
    }
}

int main(void)
{
    // if (cmlcd_init()) {
    //     LOG_ERR("LCD init failed");
    //     return 0;
    // }

    // /* 1) Test nhanh: trắng -> đen */
    // draw_solid(LCD_COLOR_WHITE);
    // cmlcd_refresh();
    // k_msleep(1500);

    // draw_solid(LCD_COLOR_BLUE);
    // cmlcd_refresh();
    // k_msleep(1500);

    // /* 2) Vẽ caro rồi refresh */
    // draw_checkerboard(16);
    // cmlcd_refresh();

    // /* 3) Giữ refresh định kỳ (để toggle EXTCOMIN >= 1 lần/phút) */
    // // while (1) {
    // //     k_msleep(3000);   // 30s
    // //     cmlcd_refresh();   // chỉ gửi lại frame hiện tại + toggle EXTCOMIN
    // // }

    // lpm_gfx_t gfx;
    // if (!lpm_gfx_init(&gfx)) {
    //     printk("LCD init failed\n");
    //     return;
    // }
    // lpm_gfx_set_rotation(&gfx, 3); // 0,1,2,3 tương ứng 0,90,180,270 độ

    // uint16_t white = lpm_gfx_Color565(255,255,255);
    // uint16_t red   = lpm_gfx_Color565(255,0,0);
    // uint16_t blue  = lpm_gfx_Color565(0,0,255);
    // uint16_t green = lpm_gfx_Color565(0,255,0);
    // uint16_t black = lpm_gfx_Color565(0,0,0);

    // lpm_gfx_fill_screen(&gfx, white);

    // lpm_gfx_draw_rect(&gfx, 10, 10, 156, 156, black);
    // lpm_gfx_fill_rect(&gfx, 30, 30, 40, 40, red);

    // for (int i = 0; i < 176; i++) {
    //     lpm_gfx_draw_pixel(&gfx, i, i, blue);
    //     lpm_gfx_draw_pixel(&gfx, 175 - i, i, green);
    // }

    // lpm_gfx_display(&gfx);

    // lpm_gfx_t gfx;
    // if (!lpm_gfx_init(&gfx)) {
    //     printk("LCD init failed\n");
    //     return;
    // }

    // /* Nền trắng */
    // uint16_t WHITE = lpm_gfx_Color565(255,255,255);
    // uint16_t BLACK = lpm_gfx_Color565(0,0,0);
    // uint16_t BLUE  = lpm_gfx_Color565(0,0,255);

    // lpm_gfx_fill_screen(&gfx, WHITE);

    // /* Text đen, transparent bg, size 1x1 */
    // lpm_gfx_set_text_color(&gfx, BLACK);
    // lpm_gfx_set_text_size(&gfx, 1, 1);
    // lpm_gfx_set_cursor(&gfx, 6, 8);
    // lpm_gfx_write(&gfx, "HELLO 123");

    // /* Dòng 2, text xanh, có background vàng, size 2x2 */
    // uint16_t YELLOW = lpm_gfx_Color565(255,255,0);
    // lpm_gfx_set_text_color_bg(&gfx, BLUE, YELLOW);
    // lpm_gfx_set_text_size(&gfx, 2, 2);
    // lpm_gfx_set_cursor(&gfx, 6, 24);
    // lpm_gfx_printf(&gfx, "TEMP: %d C", 27);

    // /* Đẩy ra LCD */
    // lpm_gfx_display(&gfx);

    lpm_gfx_t gfx;
    if (!lpm_gfx_init(&gfx)) {
        printk("LCD init failed\n");
        return;
    }

    analog_watch_theme_t theme;
    analog_watch_get_default_theme(&theme);

    /* Vẽ ngay mặt đồng hồ + thời gian “giả lập” */
    analog_watch_draw(&gfx, &theme, 10, 10, 30, true);

    /* Hoặc cập nhật mỗi giây (demo bằng uptime) */
    while (1) {
        analog_watch_draw_now(&gfx, &theme);
        k_msleep(1000);
    }
    return 0;
}