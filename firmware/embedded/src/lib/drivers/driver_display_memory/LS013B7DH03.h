#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>

/* ===== Aliases giống bạn ===== */
#define DP_EXT_PIN              DT_ALIAS(dpext)
#define DP_ON_PIN               DT_ALIAS(dpon)
#define DP_CS_PIN               DT_ALIAS(dpcs)

/* ===== Node label cho panel ===== */
#define LS_NODE                 DT_NODELABEL(ls013b7dh03)

/* ===== Panel geometry ===== */
#define LCD_DISP_WIDTH          (128)
#define LCD_DISP_HEIGHT         (128)

/* 1bpp => 128/8 = 16 byte/line */
#define LS013_BYTES_PER_LINE    (LCD_DISP_WIDTH / 8)

/* ===== Monochrome “color” mapping =====
 * Datasheet: D = 0 -> Black, D = 1 -> White
 */
#define LCD_COLOR_BLACK         (0x00)
#define LCD_COLOR_WHITE         (0x01)

/* ===== Optional: giữ API cũ ===== */
#define LCD_TRANSMODE_OPAQUE        (0x00)
#define LCD_TRANSMODE_TRANSPARENT   (0x01)
#define LCD_TRANSMODE_TRANSLUCENT   (0x02)

#define LCD_BLINKMODE_NONE      (0x00)
#define LCD_BLINKMODE_WHITE     (0x01)
#define LCD_BLINKMODE_BLACK     (0x02)
#define LCD_BLINKMODE_INVERSE   (0x03)

/* ===== SPI freq recommend ===== */
#define FREQUENCY_1MHZ          (1000000)

/* ===== DTS specs ===== */
static const struct gpio_dt_spec dp_ext = GPIO_DT_SPEC_GET(DP_EXT_PIN, gpios);
static const struct gpio_dt_spec dp_on  = GPIO_DT_SPEC_GET(DP_ON_PIN, gpios);
static const struct gpio_dt_spec dp_cs  = GPIO_DT_SPEC_GET(DP_CS_PIN, gpios);

/* Backlight là OPTIONAL (Memory LCD reflective). Nếu board không có dpbl, bạn có thể bỏ PWM_DT_SPEC_GET. */
static const struct pwm_dt_spec  dp_bl  = PWM_DT_SPEC_GET(DT_ALIAS(dpbl));

/* Mặc định dùng LSB-first cho LS013 để map bit đúng theo timing chart */
static const struct spi_dt_spec lcd_spi =
    SPI_DT_SPEC_GET(LS_NODE, SPI_WORD_SET(8) | SPI_TRANSFER_LSB, 0);

/* ===== API giống bạn ===== */
int  cmlcd_init(void);
int  cmlcd_backlight_set(uint8_t percent);
void cmlcd_draw_pixel(int16_t x, int16_t y, uint8_t color);
void cmlcd_cls(void);
void cmlcd_clear_display(void);
void cmlcd_refresh(void);
int  cmlcd_set_blink_mode(uint8_t mode);   /* LS013 không hỗ trợ -> -ENOTSUP */
void cmlcd_set_trans_mode(uint8_t mode);
