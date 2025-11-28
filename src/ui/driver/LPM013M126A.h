#ifndef LPM013M126A_H
#define LPM013M126A_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>
#include <string.h>

#define DP_BL_PIN               DT_ALIAS(dpbl)
#define DP_EXT_PIN              DT_ALIAS(dpext)
#define DP_ON_PIN               DT_ALIAS(dpon)
#define DP_CS_PIN               DT_ALIAS(dpcs)

#define LPM_NODE                DT_NODELABEL(lpm013m126a)
#define OPERATION               (SPI_OP_MODE_MASTER | SPI_WORD_SET(8))

/** @def
 * Device define
 */
#define LCD_DEVICE_WIDTH        (176)
#define LCD_DEVICE_HEIGHT       (176)

/** @def
 * Window system define
 */
#define LCD_DISP_WIDTH          (176)
#define LCD_DISP_HEIGHT         (176)
#define LCD_DISP_HEIGHT_MAX_BUF (44)

/** @def
 * some RGB color definitions
 */
/*                                        R, G, B     */ 
#define LCD_COLOR_BLACK     (0x00)    /*  0  0  0  0  */
#define LCD_COLOR_BLUE      (0x02)    /*  0  0  1  0  */
#define LCD_COLOR_GREEN     (0x04)    /*  0  1  0  0  */
#define LCD_COLOR_CYAN      (0x06)    /*  0  1  1  0  */
#define LCD_COLOR_RED       (0x08)    /*  1  0  0  0  */
#define LCD_COLOR_MAGENTA   (0x0a)    /*  1  0  1  0  */
#define LCD_COLOR_YELLOW    (0x0c)    /*  1  1  0  0  */
#define LCD_COLOR_WHITE     (0x0e)    /*  1  1  1  0  */

/** @def
 * ID for setTransMode
 */
#define LCD_TRANSMODE_OPAQUE        (0x00)  //!< BackGroud is Opaque
#define LCD_TRANSMODE_TRANSPARENT   (0x01)  //!< BackGroud is Transparent
#define LCD_TRANSMODE_TRANSLUCENT   (0x02)  //!< BackGroud is Translucent

/** @def
 *ID for setBlinkMode
 */
#define LCD_BLINKMODE_NONE      (0x00)  //!< Blinking None
#define LCD_BLINKMODE_WHITE     (0x01)  //!< Blinking White
#define LCD_BLINKMODE_BLACK     (0x02)  //!< Blinking Black
#define LCD_BLINKMODE_INVERSE   (0x03)  //!< Inversion Mode

/** @def
 * LCD_Color SPI commands
 */
#define LCD_COLOR_CMD_UPDATE            (0x90) //!< Update Mode (4bit Data Mode)
#define LCD_COLOR_CMD_ALL_CLEAR         (0x20) //!< All Clear Mode
#define LCD_COLOR_CMD_NO_UPDATE         (0x00) //!< No Update Mode
#define LCD_COLOR_CMD_BLINKING_WHITE    (0x18) //!< Display Blinking Color Mode (White)
#define LCD_COLOR_CMD_BLINKING_BLACK    (0x10) //!< Display Blinking Color Mode (Black)
#define LCD_COLOR_CMD_INVERSION         (0x14) //!< Display Inversion Mode

/** @def
 * LCD_Color SPI frequency
 */
#define FREQUENCY_1MHZ    (1000000)
#define FREQUENCY_4MHZ    (4000000)
#define FREQUENCY_8MHZ    (8000000)
#define FREQUENCY_16MHZ   (16000000)

/* internal state */
static const struct gpio_dt_spec dp_ext = GPIO_DT_SPEC_GET(DP_EXT_PIN, gpios);
static const struct gpio_dt_spec dp_on  = GPIO_DT_SPEC_GET(DP_ON_PIN, gpios);
static const struct gpio_dt_spec dp_cs  = GPIO_DT_SPEC_GET(DP_CS_PIN, gpios);
static const struct pwm_dt_spec dp_bl = PWM_DT_SPEC_GET(DT_ALIAS(dpbl));
static const struct spi_dt_spec lcd_spi = SPI_DT_SPEC_GET(LPM_NODE, SPI_WORD_SET(8) | SPI_TRANSFER_MSB, 0);

int cmlcd_init(void);
int cmlcd_backlight_set(uint8_t percent);   
void cmlcd_draw_pixel(int16_t x, int16_t y, uint8_t color);
void cmlcd_cls(void);
void cmlcd_clear_display(void);
void cmlcd_refresh(void);
void cmlcd_set_blink_mode(uint8_t mode);
void cmlcd_set_trans_mode(uint8_t mode);

#endif // LPM013M126A_H
