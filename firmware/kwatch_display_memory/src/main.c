#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "driver_display_memory/LPM013M126A.h"
#include "Kwatch_GFX.h"

LOG_MODULE_REGISTER(app, LOG_LEVEL_INF);

// static void draw_test_pattern(void)
// {
//     // 1) Clear buffer (white)
//     cmlcd_cls();
//
//     // 2) Vẽ 2 đường chéo + 1 ô vuông để nhìn rõ refresh
//     for (int i = 0; i < LCD_DISP_WIDTH; i++) {
//         cmlcd_draw_pixel(i, i, LCD_COLOR_BLACK);                       
//         cmlcd_draw_pixel(LCD_DISP_WIDTH - 1 - i, i, LCD_COLOR_RED);    
//     }
//
//     // 3) Vẽ khung chữ nhật ở giữa
//     int x0 = 40, y0 = 40, x1 = 136, y1 = 136;
//     for (int x = x0; x <= x1; x++) {
//         cmlcd_draw_pixel(x, y0, LCD_COLOR_BLUE);
//         cmlcd_draw_pixel(x, y1, LCD_COLOR_BLUE);
//     }
//     for (int y = y0; y <= y1; y++) {
//         cmlcd_draw_pixel(x0, y, LCD_COLOR_BLUE);
//         cmlcd_draw_pixel(x1, y, LCD_COLOR_BLUE);
//     }
//
//     // 4) Refresh để đẩy lên LCD (CS auto sẽ chạy ở đây)
//     cmlcd_refresh();
// }

// int main(void)
// {
//     LOG_INF("K-Watch LCD auto-CS test start");

//     int ret = cmlcd_init();
//     if (ret) {
//         LOG_ERR("cmlcd_init failed: %d", ret);
//         return 0;
//     }

//     // Test backlight
//     cmlcd_backlight_set(100);
//     k_msleep(300);
//     cmlcd_backlight_set(20);
//     k_msleep(300);
//     cmlcd_backlight_set(100);

//     // Test blink (nếu bạn muốn)
//     cmlcd_set_blink_mode(LCD_BLINKMODE_NONE);
//     k_msleep(200);

//     // Test 1: clear bằng command ALL_CLEAR đã nằm trong cmlcd_init() rồi,
//     // nhưng gọi lại cho chắc
//     cmlcd_clear_display();
//     k_msleep(200);

//     // Test 2: vẽ pattern và refresh
//     draw_test_pattern();

//     // Loop: đổi blink mode + refresh định kỳ để nhìn EXTCOMIN + CS auto ổn định
//     while (1) {
//         k_msleep(1500);
//         cmlcd_set_blink_mode(LCD_BLINKMODE_INVERSE);

//         k_msleep(1500);
//         cmlcd_set_blink_mode(LCD_BLINKMODE_NONE);

//         // vẽ thêm vài pixel thay đổi để chắc chắn refresh cập nhật
//         for (int i = 0; i < 50; i++) {
//             cmlcd_draw_pixel(10 + i, 160, LCD_COLOR_GREEN);
//         }
//         cmlcd_refresh();
//     }

//     return 0;
// }


#include "Kwatch_GFX.h"

/* Demo bitmap 4bpp 16x16: ô vuông viền đỏ, nền trắng, tâm xanh */
static const uint8_t demo_16x16_4bpp[] = {
    0xEE,0xEE,0xEE,0xEE, 0xEE,0xEE,0xEE,0xEE,
    0xE8,0x88,0x88,0x88, 0x88,0x88,0x88,0xEE,
    0xE8,0xEE,0xEE,0xEE, 0xEE,0xEE,0xEE,0x8E,
    0xE8,0xEE,0x66,0x66, 0x66,0x66,0xEE,0x8E,
    0xE8,0xEE,0x66,0x66, 0x66,0x66,0xEE,0x8E,
    0xE8,0xEE,0x66,0x66, 0x66,0x66,0xEE,0x8E,
    0xE8,0xEE,0x66,0x66, 0x66,0x66,0xEE,0x8E,
    0xE8,0xEE,0x66,0x66, 0x66,0x66,0xEE,0x8E,
    0xE8,0xEE,0x66,0x66, 0x66,0x66,0xEE,0x8E,
    0xE8,0xEE,0x66,0x66, 0x66,0x66,0xEE,0x8E,
    0xE8,0xEE,0x66,0x66, 0x66,0x66,0xEE,0x8E,
    0xE8,0xEE,0x66,0x66, 0x66,0x66,0xEE,0x8E,
    0xE8,0xEE,0x66,0x66, 0x66,0x66,0xEE,0x8E,
    0xE8,0xEE,0xEE,0xEE, 0xEE,0xEE,0xEE,0x8E,
    0xE8,0x88,0x88,0x88, 0x88,0x88,0x88,0xEE,
    0xEE,0xEE,0xEE,0xEE, 0xEE,0xEE,0xEE,0xEE,
};

/* Demo bitmap 1bpp 16x16: dấu cộng */
static const uint8_t demo_16x16_mono[] = {
    0x00,0x00,0x00,0x00,
    0x00,0x10,0x00,0x00,
    0x00,0x10,0x00,0x00,
    0x00,0x10,0x00,0x00,
    0x7F,0xFF,0xFF,0xE0,
    0x7F,0xFF,0xFF,0xE0,
    0x00,0x10,0x00,0x00,
    0x00,0x10,0x00,0x00,
    0x00,0x10,0x00,0x00,
    0x00,0x10,0x00,0x00,
    0x00,0x10,0x00,0x00,
    0x00,0x10,0x00,0x00,
    0x7F,0xFF,0xFF,0xE0,
    0x7F,0xFF,0xFF,0xE0,
    0x00,0x10,0x00,0x00,
    0x00,0x10,0x00,0x00,
};

static void gfx_demo(void)
{
    kgfx_init();
    kgfx_clear(LCD_COLOR_WHITE);

    /* Hình cơ bản */
    kgfx_fill_rect(10, 10, 50, 30, LCD_COLOR_YELLOW);
    kgfx_draw_rect(8, 8, 54, 34, LCD_COLOR_RED);
    kgfx_fill_circle(120, 40, 20, LCD_COLOR_BLUE);
    kgfx_draw_triangle(30, 80, 10, 140, 80, 150, LCD_COLOR_MAGENTA);

    /* Text với trạng thái hiện tại */
    kgfx_set_text_size(2);
    kgfx_set_text_color(LCD_COLOR_BLACK, LCD_COLOR_WHITE, true);
    kgfx_write_text(10, 100, "Hello\nK-Watch");

    /* Bitmap 4bpp và 1bpp */
    kgfx_draw_bitmap_4bpp(140, 120, 16, 16, demo_16x16_4bpp);
    kgfx_draw_bitmap_mono(20, 140, 16, 16, demo_16x16_mono,
                          LCD_COLOR_RED, LCD_COLOR_WHITE, true);

    /* Thử xoay 90 độ và vẽ thêm để kiểm tra transform */
    kgfx_set_rotation(1);
    kgfx_set_text_size(1);
    kgfx_set_text_color(LCD_COLOR_GREEN, LCD_COLOR_WHITE, false);
    kgfx_write_text(0, 0, "Rot90");

    kgfx_flush();
}

int main(void)
{
    LOG_INF("K-Watch GFX demo");
    if (cmlcd_init() != 0) {
        LOG_ERR("LCD init failed");
        return 0;
    }

    gfx_demo();

    while (1) {
        k_msleep(1000);
    }
    return 0;
}