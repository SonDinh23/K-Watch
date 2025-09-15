#ifndef LPM013M126A_H
#define LPM013M126A_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
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
 * window system define
 */
#define LCD_DISP_WIDTH          (176)
#define LCD_DISP_HEIGHT         (176)
#define LCD_DISP_HEIGHT_MAX_BUF (44)

/** @def
 * some RGB color definitions
 */
/*                                        R, G, B     */
#define LCD_COLOR_BLACK     (0x00)    /*  0  0  0  0  */
#define LCD_COLOR_GRAY      (0x01)    /*  0  0  0  1  */
#define LCD_COLOR_BLUE      (0x02)    /*  0  0  1  0  */
#define LCD_COLOR_BRIGHTBLUE (0x03)  /*  0  0  1  1  */
#define LCD_COLOR_GREEN     (0x04)    /*  0  1  0  0  */
#define LCD_COLOR_LIME      (0x05)    /*  0  1  1  1  */
#define LCD_COLOR_CYAN      (0x06)    /*  0  1  1  0  */
#define LCD_COLOR_TURQUOISE (0x07)    /*  0  1  1  0  */
#define LCD_COLOR_RED       (0x08)    /*  1  0  0  0  */
#define LCD_COLOR_PINK      (0x09)    /*  1  0  0  1  */
#define LCD_COLOR_MAGENTA   (0x0a)    /*  1  0  1  0  */
#define LCD_COLOR_VIOLET    (0x0b)    /*  1  0  1  1  */
#define LCD_COLOR_YELLOW    (0x0c)    /*  1  1  0  0  */
#define LCD_COLOR_BROWN     (0x0d)    /*  1  1  0  1  */
#define LCD_COLOR_WHITE     (0x0e)    /*  1  1  1  0  */
#define LCD_COLOR_LIGHTGRAY (0x0f)    /*  1  1  1  1  */

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
static const struct gpio_dt_spec dp_bl  = GPIO_DT_SPEC_GET(DP_BL_PIN, gpios);
static const struct gpio_dt_spec dp_ext = GPIO_DT_SPEC_GET(DP_EXT_PIN, gpios);
static const struct gpio_dt_spec dp_on  = GPIO_DT_SPEC_GET(DP_ON_PIN, gpios);
static const struct gpio_dt_spec dp_cs  = GPIO_DT_SPEC_GET(DP_CS_PIN, gpios);

static const struct spi_dt_spec lcd_spi = SPI_DT_SPEC_GET(LPM_NODE, SPI_WORD_SET(8) | SPI_TRANSFER_MSB, 0);

int  cmlcd_init(void);
void cmlcd_draw_pixel(int16_t x, int16_t y, uint8_t color);
void cmlcd_cls(void);
void cmlcd_clear_display(void);
void cmlcd_refresh(void);
void cmlcd_set_blink_mode(uint8_t mode);
void cmlcd_set_trans_mode(uint8_t mode);


/* Kích thước panel gốc (không xoay) */
#define LPM_GFX_RAW_WIDTH   176
#define LPM_GFX_RAW_HEIGHT  176

/* Tạo màu 16-bit RGB565 (tiện giống Adafruit) */
static inline uint16_t lpm_gfx_Color565(uint8_t r, uint8_t g, uint8_t b) {
    return (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

/* Context GFX thuần C */
typedef struct {
    int16_t width;     /* theo rotation hiện tại */
    int16_t height;    /* theo rotation hiện tại */
    uint8_t rotation;  /* 0..3 */

    /* ===== Text state ===== */
    int16_t  cursor_x;       /* vị trí con trỏ */
    int16_t  cursor_y;
    uint16_t textcolor;      /* màu chữ (RGB565) */
    uint16_t textbgcolor;    /* màu nền (RGB565); nếu textbgcolor==textcolor => transparent */
    uint8_t  textsize_x;     /* scale ngang (>=1) */
    uint8_t  textsize_y;     /* scale dọc  (>=1) */
    bool     wrap;           /* xuống dòng khi chạm mép phải */
} lpm_gfx_t;

/* Khởi tạo LCD & context (gọi cmlcd_init + clear). Trả về true nếu OK */
bool     lpm_gfx_init(lpm_gfx_t *g);

/* Đẩy framebuffer driver ra LCD (gọi cmlcd_refresh) */
void     lpm_gfx_display(lpm_gfx_t *g);

/* Xoay 0..3 (giống Adafruit: 0,90,180,270) */
void     lpm_gfx_set_rotation(lpm_gfx_t *g, uint8_t r);

/* Lấy kích thước hiện tại */
int16_t  lpm_gfx_width (const lpm_gfx_t *g);
int16_t  lpm_gfx_height(const lpm_gfx_t *g);

/* ===== Vẽ cơ bản (màu đầu vào RGB565) ===== */
void     lpm_gfx_draw_pixel      (lpm_gfx_t *g, int16_t x, int16_t y, uint16_t color565);
void     lpm_gfx_draw_fast_hline (lpm_gfx_t *g, int16_t x, int16_t y, int16_t w, uint16_t color565);
void     lpm_gfx_draw_fast_vline (lpm_gfx_t *g, int16_t x, int16_t y, int16_t h, uint16_t color565);
void     lpm_gfx_fill_rect       (lpm_gfx_t *g, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color565);
void     lpm_gfx_draw_rect       (lpm_gfx_t *g, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color565);
void     lpm_gfx_draw_line       (lpm_gfx_t *g, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color565);
void     lpm_gfx_fill_screen     (lpm_gfx_t *g, uint16_t color565);

/* ===== Text API ===== */
/* set/get cursor */
static inline void lpm_gfx_set_cursor(lpm_gfx_t *g, int16_t x, int16_t y) { if (g){ g->cursor_x = x; g->cursor_y = y; } }
static inline int16_t lpm_gfx_get_cursor_x(const lpm_gfx_t *g){ return g? g->cursor_x : 0; }
static inline int16_t lpm_gfx_get_cursor_y(const lpm_gfx_t *g){ return g? g->cursor_y : 0; }

/* màu chữ + nền; nếu bg==fg => nền transparent (giống Adafruit_GFX::setTextColor(c)) */
static inline void lpm_gfx_set_text_color(lpm_gfx_t *g, uint16_t c){ if (g){ g->textcolor=c; g->textbgcolor=c; } }
static inline void lpm_gfx_set_text_color_bg(lpm_gfx_t *g, uint16_t c, uint16_t bg){ if (g){ g->textcolor=c; g->textbgcolor=bg; } }

/* size & wrap */
static inline void lpm_gfx_set_text_size(lpm_gfx_t *g, uint8_t sx, uint8_t sy){ if (g){ g->textsize_x = sx? sx:1; g->textsize_y = sy? sy:1; } }
static inline void lpm_gfx_set_text_wrap(lpm_gfx_t *g, bool w){ if (g){ g->wrap = w; } }

/* write 1 ký tự (ASCII cơ bản) tại cursor, rồi dịch cursor */
void     lpm_gfx_write_char(lpm_gfx_t *g, char c);
/* in chuỗi C kết thúc '\0' */
void     lpm_gfx_write(lpm_gfx_t *g, const char *s);
/* printf tiện dụng */
int      lpm_gfx_printf(lpm_gfx_t *g, const char *fmt, ...);

/* (Tùy chọn) Nếu bạn có hàm driver đặt background + cls siêu nhanh,
   hãy define macro này ở compile (-DLPM_GFX_USE_DRIVER_BG) và cung cấp
   prototype trong driver:
       void cmlcd_set_background(uint8_t nibble);
*/

#endif // LPM013M126A_H
