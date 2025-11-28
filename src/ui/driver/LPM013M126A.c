#include "../../ui/driver/LPM013M126A.h"

LOG_MODULE_REGISTER(LPM013M126A, LOG_LEVEL_DBG);

/* ===== Internal state ===== */
static struct spi_config lcd_cfg = {
    .frequency = FREQUENCY_1MHZ,
    .operation = OPERATION,
    .slave = 0,
    .cs = {
        .gpio = {
            .port = NULL,
            .pin = 0,
            .dt_flags = 0,
        },
        .delay = 0,
    },
};

static uint32_t freq_hz   = FREQUENCY_1MHZ; 
static uint8_t background = LCD_COLOR_WHITE;
static uint8_t blink_cmd  = LCD_COLOR_CMD_NO_UPDATE;
static uint8_t trans_mode = LCD_TRANSMODE_OPAQUE;
static bool    polarity   = 0;     /* bit6 in the instruction */
static bool    ext_state  = 0;

/* Working window: full screen */
static int window_x = 0, window_y = 0, window_w = LCD_DISP_WIDTH, window_h = LCD_DISP_HEIGHT;

/* Buffer: 176x176, 4bpp => 2 pixel/byte => 88 byte/line => ~15.5 KB */
static uint8_t cmd_buf[LCD_DISP_WIDTH / 2];                         /* 88 bytes */
static uint8_t disp_buf[(LCD_DISP_WIDTH / 2) * LCD_DISP_HEIGHT];    /* 88 * 176 */

/* ===================================== Helpers ===================================== */

/* Set IO level to 'active' or 'inactive' based on dt_flags (ACTIVE_LOW/HIGH)  */
static inline void gpio_set_active(const struct gpio_dt_spec *s, bool active)
{
    if (!device_is_ready(s->port))
        return;
    const bool is_active_low = (s->dt_flags & GPIO_ACTIVE_LOW);
    gpio_pin_set_dt(s, active ? (is_active_low ? 0 : 1) : (is_active_low ? 1 : 0));
}

/* Manual CS (alias lcdcs) */
static inline void cs_set_active(bool active)
{
    gpio_set_active(&dp_cs, active);
}

/* Write 1 buffer with bit-order selection (MSB/LSB) */
static int spi_write_bytes(const uint8_t *buf, size_t len, bool lsb_first,
                           struct spi_config *io_cfg /* in/out */)
{
    struct spi_config cfg = *io_cfg; /* copy để chỉnh bit-order */
    cfg.operation &= ~SPI_TRANSFER_LSB;
    if (lsb_first)
        cfg.operation |= SPI_TRANSFER_LSB;

    struct spi_buf sb = {.buf = (void *)buf, .len = len};
    struct spi_buf_set tx = {.buffers = &sb, .count = 1};
    return spi_write(lcd_spi.bus, &cfg, &tx);
}

/* Toggle EXTCOMIN (at least 1 time/minute, I do it every refresh) */
static inline void extcomin_toggle(void)
{
    if (!device_is_ready(dp_ext.port)) return;
    ext_state = !ext_state;
    gpio_pin_set_dt(&dp_ext, ext_state ? 1 : 0);
}

static int spi_packet_mixed(const uint8_t *msb, size_t msb_len,
                            const uint8_t *lsb, size_t lsb_len,
                            uint32_t freq_hz, uint32_t cs_setup_us, uint32_t cs_hold_us)
{
    cs_set_active(true);
    k_busy_wait(cs_setup_us);  /* giống Arduino: CS lên -> 6us -> clock */

    int err = 0;
    if (msb && msb_len) {
        err = spi_write_bytes(msb, msb_len, false, &lcd_cfg);  /* MSB-first */
        if (err) goto out;
    }
    if (lsb && lsb_len) {
        err = spi_write_bytes(lsb, lsb_len, true,  &lcd_cfg);  /* LSB-first */
        if (err) goto out;
    }

out:
    k_busy_wait(cs_hold_us);
    cs_set_active(false);
    return err;
}

/* ===== Public API ===== */

int cmlcd_init(void)
{
    int ret;

    if (!device_is_ready(lcd_spi.bus)) {
        LOG_ERR("SPI bus not ready");
        return -ENODEV;
    }

    /* Configure your 3 GPIO aliases to OUTPUT */
    if (device_is_ready(dp_ext.port)) {
        ret = gpio_pin_configure_dt(&dp_ext, GPIO_OUTPUT_INACTIVE);
        if (ret) return ret;
    }
    if (device_is_ready(dp_on.port)) {
        ret = gpio_pin_configure_dt(&dp_on, GPIO_OUTPUT_INACTIVE);
        if (ret) return ret;
    }

    if (device_is_ready(dp_cs.port))
    {
        int ret = gpio_pin_configure_dt(&dp_cs, GPIO_OUTPUT_INACTIVE);
        if (ret)
            return ret;
    }

    gpio_set_active(&dp_on, true);
    cmlcd_backlight_set(100);  // 100%

    cmlcd_clear_display();
    return 0;
}

int cmlcd_backlight_set(uint8_t percent)  // 0..100
{
    if (percent > 100) percent = 100;

    uint32_t period = PWM_MSEC(1);                 // khớp với DTS
    uint32_t pulse  = (period * percent) / 100;    // duty theo %
    /* Lưu ý: vì DTS đã INVERTED, driver sẽ tự xử lý mức tác dụng là LOW.
       Nếu thấy sáng/tối bị đảo, đổi thành: pulse = period - pulse; */

    if (!pwm_is_ready_dt(&dp_bl)) return -ENODEV;
    return pwm_set_dt(&dp_bl, period, pulse);
}

void cmlcd_set_trans_mode(uint8_t mode)
{
    trans_mode = mode; /* hiện tại chưa dùng trong pipeline vẽ */
}

void cmlcd_set_blink_mode(uint8_t mode)
{
    switch (mode) {
    case LCD_BLINKMODE_NONE:    blink_cmd = LCD_COLOR_CMD_NO_UPDATE;     break;
    case LCD_BLINKMODE_WHITE:   blink_cmd = LCD_COLOR_CMD_BLINKING_WHITE;break;
    case LCD_BLINKMODE_BLACK:   blink_cmd = LCD_COLOR_CMD_BLINKING_BLACK;break;
    case LCD_BLINKMODE_INVERSE: blink_cmd = LCD_COLOR_CMD_INVERSION;     break;
    default:                    blink_cmd = LCD_COLOR_CMD_NO_UPDATE;     break;
    }

    uint8_t cmd = blink_cmd | (polarity ? 0x40 : 0x00);
    uint8_t zero = 0x00;
    (void)spi_packet_mixed(&cmd, 1, &zero, 1, freq_hz, 6, 6);
}

void cmlcd_draw_pixel(int16_t x, int16_t y, uint8_t color)
{
    if (x < window_x || x >= window_x + window_w) return;
    if (y < window_y || y >= window_y + window_h) return;

    size_t idx = ((window_w / 2) * (y - window_y)) + ((x - window_x) / 2);
    if ((x & 1) == 0) {
        /* even x -> high nibble */
        disp_buf[idx] = (disp_buf[idx] & 0x0F) | ((color & 0x0F) << 4);
    } else {
        /* odd x -> low nibble */
        disp_buf[idx] = (disp_buf[idx] & 0xF0) | (color & 0x0F);
    }
}

void cmlcd_cls(void)
{
    uint8_t nib  = (background & 0x0F);
    uint8_t pair = (nib << 4) | nib;
    memset(disp_buf, pair, sizeof(disp_buf));
}

void cmlcd_clear_display(void)
{
    LOG_INF("Clear display");
    cmlcd_cls();
    uint8_t cmd = LCD_COLOR_CMD_ALL_CLEAR | (polarity ? 0x40 : 0x00);
    uint8_t zero = 0x00;
    (void)spi_packet_mixed(&cmd, 1, &zero, 1, freq_hz, 6, 6);
    k_msleep(15);  // wait for deletion
    extcomin_toggle();
}

void cmlcd_refresh(void)
{
    /* Gửi từng dòng: (CMD MSB) (LINE MSB) (DATA 88B MSB) (0x00 0x00 LSB) */
    const int copy_width = (window_x + window_w < LCD_DISP_WIDTH)
                         ? (window_w / 2)
                         : ((LCD_DISP_WIDTH - window_x) / 2);

    for (int i = 0; i < window_h; ++i)
    {
        if (window_y + i >= LCD_DISP_HEIGHT)
            break;

        uint8_t nib = (background & 0x0F);
        uint8_t pair = (nib << 4) | nib;
        memset(cmd_buf, pair, sizeof(cmd_buf));
        memcpy(&cmd_buf[window_x / 2], &disp_buf[(window_w / 2) * i], copy_width);

        /* head MSB (2 byte) + data MSB (88 byte) + tail LSB (2 byte) trong 1 phiên CS */
        uint8_t head[2] = {
            (uint8_t)(LCD_COLOR_CMD_UPDATE | (polarity ? 0x40 : 0x00)),
            (uint8_t)(window_y + i + 1)};
        uint8_t tail[2] = {0x00, 0x00};

        cs_set_active(true);
        k_busy_wait(6);

        (void)spi_write_bytes(head, sizeof(head), false, &lcd_cfg);          // MSB
        (void)spi_write_bytes(cmd_buf, LCD_DISP_WIDTH / 2, false, &lcd_cfg); // MSB
        (void)spi_write_bytes(tail, sizeof(tail), true, &lcd_cfg);           // LSB

        k_busy_wait(6);
        cs_set_active(false);
    }
    extcomin_toggle();
}
