#include "../driver_display_memory/LS013B7DH03.h"

LOG_MODULE_REGISTER(LS013B7DH03, LOG_LEVEL_DBG);

/* ===== Internal state ===== */
static struct spi_config lcd_cfg; /* clone từ DTS, bỏ auto-CS để manual CS */

static uint8_t background = LCD_COLOR_WHITE;
static uint8_t trans_mode = LCD_TRANSMODE_OPAQUE;

static bool ext_state = 0;

/* Full-frame buffer: 128x128, 1bpp => 2048 bytes */
static uint8_t disp_buf[LS013_BYTES_PER_LINE * LCD_DISP_HEIGHT];

/* ===== Helpers ===== */

static inline void gpio_set_active(const struct gpio_dt_spec *s, bool active)
{
    if (!device_is_ready(s->port)) {
        return;
    }
    const bool is_active_low = (s->dt_flags & GPIO_ACTIVE_LOW);
    gpio_pin_set_dt(s, active ? (is_active_low ? 0 : 1) : (is_active_low ? 1 : 0));
}

static inline void cs_set_active(bool active)
{
    gpio_set_active(&dp_cs, active); /* SCS active theo dt_flags */
}

/* Write bytes with current config (already LSB-first from DTS; vẫn giữ hàm để đồng bộ style) */
static int spi_write_bytes(const uint8_t *buf, size_t len)
{
    struct spi_buf sb = { .buf = (void *)buf, .len = len };
    struct spi_buf_set tx = { .buffers = &sb, .count = 1 };
    return spi_write(lcd_spi.bus, &lcd_cfg, &tx);
}

/* Toggle EXTCOMIN (datasheet: period should be constant; thường toggle mỗi refresh hoặc dùng timer 1Hz/2Hz) */
static inline void extcomin_toggle(void)
{
    if (!device_is_ready(dp_ext.port)) return;
    ext_state = !ext_state;
    gpio_pin_set_dt(&dp_ext, ext_state ? 1 : 0);
}

/* LS013 command byte (LSB-first mapping)
 * Byte layout (bit0 first on wire):
 *  bit0=M0, bit1=M1, bit2=M2, bit3..7=DMY(0)
 */
#define LS013_CMD_DISPLAY     (0x00) /* M0=0, M2=0 */
#define LS013_CMD_UPDATE      (0x01) /* M0=1, M2=0 */
#define LS013_CMD_ALL_CLEAR   (0x04) /* M0=0, M2=1 */
#define LS013_DUMMY_BYTE      (0x00) /* recommended 'L' */

/* ===== Public API ===== */

int cmlcd_init(void)
{
    int ret;

    if (!device_is_ready(lcd_spi.bus)) {
        LOG_ERR("SPI bus not ready");
        return -ENODEV;
    }

    /* Clone SPI config from DTS; keep manual CS */
    lcd_cfg = lcd_spi.config;
    lcd_cfg.cs.gpio.port     = NULL;
    lcd_cfg.cs.gpio.pin      = 0;
    lcd_cfg.cs.gpio.dt_flags = 0;
    lcd_cfg.cs.delay         = 0;

    /* GPIOs */
    if (device_is_ready(dp_ext.port)) {
        ret = gpio_pin_configure_dt(&dp_ext, GPIO_OUTPUT_INACTIVE);
        if (ret) return ret;
    }

    if (device_is_ready(dp_on.port)) {
        ret = gpio_pin_configure_dt(&dp_on, GPIO_OUTPUT_INACTIVE);
        if (ret) return ret;
    }

    if (device_is_ready(dp_cs.port)) {
        ret = gpio_pin_configure_dt(&dp_cs, GPIO_OUTPUT_INACTIVE);
        if (ret) return ret;
    }

    /* DISP = ON */
    gpio_set_active(&dp_on, true);

    /* Optional backlight */
    (void)cmlcd_backlight_set(100);

    /* Init buffer + panel memory */
    cmlcd_clear_display();

    return 0;
}

int cmlcd_backlight_set(uint8_t percent)
{
    if (!pwm_is_ready_dt(&dp_bl)) {
        return -ENODEV; /* nếu board không có dpbl */
    }
    if (percent > 100) percent = 100;

    uint32_t period = PWM_MSEC(1);
    uint32_t pulse  = (period * percent) / 100;

    return pwm_set_dt(&dp_bl, period, pulse);
}

void cmlcd_set_trans_mode(uint8_t mode)
{
    trans_mode = mode; /* LS013 1bpp: bạn có thể tự dùng mode này trong GUI layer */
    (void)trans_mode;
}

int cmlcd_set_blink_mode(uint8_t mode)
{
    ARG_UNUSED(mode);
    /* LS013B7DH03 datasheet không có blink/inversion command kiểu LPM013M126A */
    return -ENOTSUP;
}

void cmlcd_draw_pixel(int16_t x, int16_t y, uint8_t color)
{
    if (x < 0 || x >= LCD_DISP_WIDTH)  return;
    if (y < 0 || y >= LCD_DISP_HEIGHT) return;

    const size_t byte_index = (size_t)y * LS013_BYTES_PER_LINE + (size_t)(x >> 3);
    const uint8_t bit_mask  = (uint8_t)(1u << (x & 7)); /* LSB-first: x=0 -> bit0 */

    if (color == LCD_COLOR_BLACK) {
        /* black => D=0 */
        disp_buf[byte_index] &= (uint8_t)~bit_mask;
    } else {
        /* white => D=1 */
        disp_buf[byte_index] |= bit_mask;
    }
}

void cmlcd_cls(void)
{
    /* background: 1=white => 0xFF, 0=black => 0x00 */
    memset(disp_buf, (background ? 0xFF : 0x00), sizeof(disp_buf));
}

void cmlcd_clear_display(void)
{
    /* Clear local buffer to white */
    background = LCD_COLOR_WHITE;
    cmlcd_cls();

    /* Panel all clear: M0=0, M2=1.
     * Datasheet yêu cầu “data transfer period > 13ck”, nên gửi thêm 1 dummy byte cho chắc.
     */
    uint8_t cmd  = LS013_CMD_ALL_CLEAR;
    uint8_t dmy  = LS013_DUMMY_BYTE;

    cs_set_active(true);
    k_busy_wait(2);

    int err = spi_write_bytes(&cmd, 1);
    if (!err) err = spi_write_bytes(&dmy, 1); /* đảm bảo đủ clock transfer */

    k_busy_wait(2);
    cs_set_active(false);

    if (err) {
        LOG_ERR("All clear SPI failed (%d)", err);
        return;
    }

    /* datasheet có nhắc timing init latch ~30us; mình để margin */
    k_busy_wait(50);
    extcomin_toggle();
}

/* Full refresh: dùng Data Update Mode (Multiple Lines) để giảm CS toggle
 * Frame format (LSB-first):
 *  [CMD_UPDATE]
 *  For each line (1..128):
 *    [ADDR] [16 bytes DATA] [DUMMY]
 */
void cmlcd_refresh(void)
{
    uint8_t cmd = LS013_CMD_UPDATE;
    uint8_t dmy = LS013_DUMMY_BYTE;

    cs_set_active(true);
    k_busy_wait(2);

    int err = spi_write_bytes(&cmd, 1);
    if (err) goto out;

    for (uint16_t y = 0; y < LCD_DISP_HEIGHT; y++) {
        uint8_t addr = (uint8_t)(y + 1); /* Gate line address: 1..128 */

        const uint8_t *line_ptr = &disp_buf[(size_t)y * LS013_BYTES_PER_LINE];

        err = spi_write_bytes(&addr, 1);
        if (err) break;

        err = spi_write_bytes(line_ptr, LS013_BYTES_PER_LINE);
        if (err) break;

        err = spi_write_bytes(&dmy, 1); /* dummy between lines / end line */
        if (err) break;
    }

out:
    k_busy_wait(2);
    cs_set_active(false);

    if (err) {
        LOG_ERR("Refresh SPI failed (%d)", err);
        return;
    }

    extcomin_toggle();
}
