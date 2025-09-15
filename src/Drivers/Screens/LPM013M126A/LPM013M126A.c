#include "LPM013M126A.h"

LOG_MODULE_REGISTER(LPM013M126A, LOG_LEVEL_DBG);

/* ===== Internal state ===== */
static struct spi_config lcd_cfg = {
    .frequency = FREQUENCY_8MHZ,
    .operation = OPERATION,
    .slave     = 0,
    .cs        = {
        .gpio = {
            .port = NULL,
            .pin  = 0,
            .dt_flags = 0,
        },
        .delay = 0,
    },
};

static uint32_t freq_hz   = FREQUENCY_8MHZ; /* tần số SPI mặc định */
static uint8_t background = LCD_COLOR_WHITE;
static uint8_t blink_cmd  = LCD_COLOR_CMD_NO_UPDATE;
static uint8_t trans_mode = LCD_TRANSMODE_OPAQUE;
static bool    polarity   = 0;     /* bit6 trong lệnh */
static bool    ext_state  = 0;

/* Cửa sổ làm việc: full screen */
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

    /* Cấu hình 3 GPIO alias của bạn thành OUTPUT */
    if (device_is_ready(dp_bl.port)) {
        ret = gpio_pin_configure_dt(&dp_bl, GPIO_OUTPUT_INACTIVE);
        if (ret) return ret;
    }
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
    gpio_set_active(&dp_bl, true);

    cmlcd_clear_display();
    return 0;
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


/* ==== Palette 16 màu (RGB565) để map từ 565 -> nibble 0..15 ==== */
/* Có thể chỉnh bảng này khớp hơn với panel của bạn. */
static const uint16_t s_palette565[16] = {
    0x0000, /* 0 BLACK       */
    0x8410, /* 1 GRAY        (~50%) */
    0x001F, /* 2 BLUE        */
    0x051F, /* 3 BRIGHT BLUE */
    0x07E0, /* 4 GREEN       */
    0x07E0, /* 5 LIME        */
    0x07FF, /* 6 CYAN        */
    0x06DF, /* 7 TURQUOISE   */
    0xF800, /* 8 RED         */
    0xF8B2, /* 9 PINK        */
    0xF81F, /* 10 MAGENTA    */
    0x781F, /* 11 VIOLET     */
    0xFFE0, /* 12 YELLOW     */
    0xA145, /* 13 BROWN      */
    0xFFFF, /* 14 WHITE      */
    0xC618  /* 15 LIGHT GRAY */
};

/* ===== Utility: 565 -> 8-bit thành phần ===== */
static inline uint8_t to8(uint16_t c565, int shift, int mask, int maxv) {
    int v = (c565 >> shift) & mask;
    return (uint8_t)((v * 255 + (maxv / 2)) / maxv);
}

/* ===== 565 -> nibble gần nhất (0..15) theo khoảng cách Euclid ===== */
static uint8_t color565_to_nibble(uint16_t c) {
    uint8_t r = to8(c, 11, 0x1F, 31);
    uint8_t g = to8(c,  5, 0x3F, 63);
    uint8_t b = to8(c,  0, 0x1F, 31);

    int best = 0;
    int bestd = 0x7FFFFFFF;

    for (int i = 0; i < 16; i++) {
        uint16_t p = s_palette565[i];
        uint8_t pr = to8(p, 11, 0x1F, 31);
        uint8_t pg = to8(p,  5, 0x3F, 63);
        uint8_t pb = to8(p,  0, 0x1F, 31);
        int dr = (int)r - (int)pr;
        int dg = (int)g - (int)pg;
        int db = (int)b - (int)pb;
        int d2 = dr*dr + dg*dg + db*db;
        if (d2 < bestd) { bestd = d2; best = i; }
    }
    return (uint8_t)best;
}

/* ===== Biến đổi toạ độ theo rotation (0,90,180,270) ===== */
static inline void xform_by_rotation(uint8_t rot, int16_t *x, int16_t *y) {
    int16_t xr = *x, yr = *y;
    switch (rot & 3) {
    default:
    case 0: /* (x,y) */ break;
    case 1: /* 90° */   *x = yr; *y = (int16_t)(LPM_GFX_RAW_WIDTH - 1 - xr); return;
    case 2: /* 180° */  *x = (int16_t)(LPM_GFX_RAW_WIDTH  - 1 - xr);
                        *y = (int16_t)(LPM_GFX_RAW_HEIGHT - 1 - yr); return;
    case 3: /* 270° */  *x = (int16_t)(LPM_GFX_RAW_HEIGHT - 1 - yr); *y = xr; return;
    }
}

/* ====== FONT 5x7 demo (subset đủ để test text cơ bản) ====== */
/* Mỗi glyph: 5 cột, bit LSB=điểm trên cùng (top). Chiều cao logic 8 (7 dùng, 1 trống). */
/* Để ngắn gọn, mình cung cấp subset cho ' ' '!' '.' ':' '-' '?' '0'..'9' 'A'..'Z'.
   Ký tự khác sẽ rơi về glyph '?' */
static const uint8_t* font5x7_glyph(char c) {
    /* khoảng trắng */
    static const uint8_t sp[] = {0x00,0x00,0x00,0x00,0x00};
    /* punctuation */
    static const uint8_t ex[] = {0x00,0x00,0x5F,0x00,0x00}; /* '!' */
    static const uint8_t dot[]={0x00,0x40,0x60,0x00,0x00}; /* '.' */
    static const uint8_t col[]={0x00,0x36,0x36,0x00,0x00}; /* ':' */
    static const uint8_t dash[]={0x08,0x08,0x08,0x08,0x08};/* '-' */
    static const uint8_t qst[]={0x02,0x01,0x59,0x09,0x06}; /* '?' */

    /* digits 0..9 */
    static const uint8_t d0[]={0x3E,0x51,0x49,0x45,0x3E};
    static const uint8_t d1[]={0x00,0x42,0x7F,0x40,0x00};
    static const uint8_t d2[]={0x62,0x51,0x49,0x49,0x46};
    static const uint8_t d3[]={0x22,0x49,0x49,0x49,0x36};
    static const uint8_t d4[]={0x18,0x14,0x12,0x7F,0x10};
    static const uint8_t d5[]={0x2F,0x49,0x49,0x49,0x31};
    static const uint8_t d6[]={0x3E,0x49,0x49,0x49,0x32};
    static const uint8_t d7[]={0x01,0x71,0x09,0x05,0x03};
    static const uint8_t d8[]={0x36,0x49,0x49,0x49,0x36};
    static const uint8_t d9[]={0x26,0x49,0x49,0x49,0x3E};

    /* A..Z */
    static const uint8_t A[]={0x7E,0x11,0x11,0x11,0x7E};
    static const uint8_t B[]={0x7F,0x49,0x49,0x49,0x36};
    static const uint8_t C[]={0x3E,0x41,0x41,0x41,0x22};
    static const uint8_t D[]={0x7F,0x41,0x41,0x22,0x1C};
    static const uint8_t E[]={0x7F,0x49,0x49,0x49,0x41};
    static const uint8_t F[]={0x7F,0x09,0x09,0x09,0x01};
    static const uint8_t G[]={0x3E,0x41,0x49,0x49,0x7A};
    static const uint8_t H[]={0x7F,0x08,0x08,0x08,0x7F};
    static const uint8_t I[]={0x00,0x41,0x7F,0x41,0x00};
    static const uint8_t J[]={0x20,0x40,0x41,0x3F,0x01};
    static const uint8_t K[]={0x7F,0x08,0x14,0x22,0x41};
    static const uint8_t L[]={0x7F,0x40,0x40,0x40,0x40};
    static const uint8_t M[]={0x7F,0x02,0x0C,0x02,0x7F};
    static const uint8_t N[]={0x7F,0x04,0x08,0x10,0x7F};
    static const uint8_t O[]={0x3E,0x41,0x41,0x41,0x3E};
    static const uint8_t P[]={0x7F,0x09,0x09,0x09,0x06};
    static const uint8_t Q[]={0x3E,0x41,0x51,0x21,0x5E};
    static const uint8_t R[]={0x7F,0x09,0x19,0x29,0x46};
    static const uint8_t S[]={0x46,0x49,0x49,0x49,0x31};
    static const uint8_t T[]={0x01,0x01,0x7F,0x01,0x01};
    static const uint8_t U[]={0x3F,0x40,0x40,0x40,0x3F};
    static const uint8_t V[]={0x1F,0x20,0x40,0x20,0x1F};
    static const uint8_t W[]={0x7F,0x20,0x18,0x20,0x7F};
    static const uint8_t X[]={0x63,0x14,0x08,0x14,0x63};
    static const uint8_t Y[]={0x07,0x08,0x70,0x08,0x07};
    static const uint8_t Z[]={0x61,0x51,0x49,0x45,0x43};

    switch (c) {
        case ' ': return sp;
        case '!': return ex;
        case '.': return dot;
        case ':': return col;
        case '-': return dash;
        case '?': return qst;
        case '0': return d0; case '1': return d1; case '2': return d2; case '3': return d3; case '4': return d4;
        case '5': return d5; case '6': return d6; case '7': return d7; case '8': return d8; case '9': return d9;
        case 'A': return A;  case 'B': return B;  case 'C': return C;  case 'D': return D;  case 'E': return E;
        case 'F': return F;  case 'G': return G;  case 'H': return H;  case 'I': return I;  case 'J': return J;
        case 'K': return K;  case 'L': return L;  case 'M': return M;  case 'N': return N;  case 'O': return O;
        case 'P': return P;  case 'Q': return Q;  case 'R': return R;  case 'S': return S;  case 'T': return T;
        case 'U': return U;  case 'V': return V;  case 'W': return W;  case 'X': return X;  case 'Y': return Y;
        case 'Z': return Z;
        default:  return qst; /* fallback '?' */
    }
}

/* ===== API cơ bản ===== */

bool lpm_gfx_init(lpm_gfx_t *g) {
    if (!g) return false;
    if (cmlcd_init() != 0) return false;

    g->rotation = 0;
    g->width  = LPM_GFX_RAW_WIDTH;
    g->height = LPM_GFX_RAW_HEIGHT;

    /* Text defaults */
    g->cursor_x = 0;
    g->cursor_y = 0;
    g->textcolor   = lpm_gfx_Color565(0,0,0);     /* đen */
    g->textbgcolor = g->textcolor;                /* bg==fg => transparent */
    g->textsize_x  = 1;
    g->textsize_y  = 1;
    g->wrap        = true;

    cmlcd_clear_display(); /* ALL_CLEAR + toggle EXTCOMIN */
    return true;
}

void lpm_gfx_display(lpm_gfx_t *g) {
    (void)g;
    cmlcd_refresh();
}

void lpm_gfx_set_rotation(lpm_gfx_t *g, uint8_t r) {
    if (!g) return;
    g->rotation = (uint8_t)(r & 3);
    if ((g->rotation & 1) == 0) {
        g->width  = LPM_GFX_RAW_WIDTH;
        g->height = LPM_GFX_RAW_HEIGHT;
    } else {
        g->width  = LPM_GFX_RAW_HEIGHT;
        g->height = LPM_GFX_RAW_WIDTH;
    }
}

int16_t lpm_gfx_width(const lpm_gfx_t *g)  { return g ? g->width  : 0; }
int16_t lpm_gfx_height(const lpm_gfx_t *g) { return g ? g->height : 0; }

/* ===== Drawing primitives ===== */

void lpm_gfx_draw_pixel(lpm_gfx_t *g, int16_t x, int16_t y, uint16_t color565) {
    if (!g) return;
    if (x < 0 || y < 0 || x >= g->width || y >= g->height) return;

    int16_t xr = x, yr = y;
    xform_by_rotation(g->rotation, &xr, &yr);

    uint8_t nib = color565_to_nibble(color565);
    cmlcd_draw_pixel(xr, yr, nib);
}

void lpm_gfx_draw_fast_hline(lpm_gfx_t *g, int16_t x, int16_t y, int16_t w, uint16_t color565) {
    if (!g || w <= 0) return;
    if (y < 0 || y >= g->height) return;
    if (x < 0) { w += x; x = 0; }
    if (x + w > g->width) w = g->width - x;
    if (w <= 0) return;

    for (int16_t i = 0; i < w; i++) {
        lpm_gfx_draw_pixel(g, x + i, y, color565);
    }
}

void lpm_gfx_draw_fast_vline(lpm_gfx_t *g, int16_t x, int16_t y, int16_t h, uint16_t color565) {
    if (!g || h <= 0) return;
    if (x < 0 || x >= g->width) return;
    if (y < 0) { h += y; y = 0; }
    if (y + h > g->height) h = g->height - y;
    if (h <= 0) return;

    for (int16_t i = 0; i < h; i++) {
        lpm_gfx_draw_pixel(g, x, y + i, color565);
    }
}

void lpm_gfx_fill_rect(lpm_gfx_t *g, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color565) {
    if (!g || w <= 0 || h <= 0) return;

    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > g->width)  w = g->width  - x;
    if (y + h > g->height) h = g->height - y;
    if (w <= 0 || h <= 0) return;

    for (int16_t j = 0; j < h; j++) {
        lpm_gfx_draw_fast_hline(g, x, y + j, w, color565);
    }
}

void lpm_gfx_draw_rect(lpm_gfx_t *g, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color565) {
    if (!g || w <= 0 || h <= 0) return;
    lpm_gfx_draw_fast_hline(g, x, y,         w, color565);
    lpm_gfx_draw_fast_hline(g, x, y + h - 1, w, color565);
    lpm_gfx_draw_fast_vline(g, x,         y, h, color565);
    lpm_gfx_draw_fast_vline(g, x + w - 1, y, h, color565);
}

/* Bresenham */
void lpm_gfx_draw_line(lpm_gfx_t *g, int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color565) {
    if (!g) return;

    int16_t dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int16_t sx = (x0 < x1) ? 1 : -1;
    int16_t dy = (y1 > y0) ? (y0 - y1) : (y1 - y0); /* dy = -abs(y1 - y0) */
    int16_t sy = (y0 < y1) ? 1 : -1;
    int16_t err = dx + dy;  /* err = dx + (-abs(dy)) */

    while (1) {
        lpm_gfx_draw_pixel(g, x0, y0, color565);
        if (x0 == x1 && y0 == y1) break;
        int16_t e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void lpm_gfx_fill_screen(lpm_gfx_t *g, uint16_t color565) {
#if defined(LPM_GFX_USE_DRIVER_BG)
    extern void cmlcd_set_background(uint8_t nibble); /* do bạn cung cấp nếu muốn tối ưu */
    cmlcd_set_background(color565_to_nibble(color565));
    cmlcd_cls();   /* memset toàn buffer theo background */
#else
    lpm_gfx_fill_rect(g, 0, 0, lpm_gfx_width(g), lpm_gfx_height(g), color565);
#endif
}

/* ===== Text rendering ===== */
/* Vẽ 1 ký tự 5x7 đã scale tại (cursor_x,cursor_y), rồi dịch cursor */
void lpm_gfx_write_char(lpm_gfx_t *g, char c) {
    if (!g) return;

    if (c == '\n') {
        g->cursor_y += g->textsize_y * 8;  /* cao 8 (7 + 1 khoảng trống) */
        g->cursor_x  = 0;
        return;
    } else if (c == '\r') {
        return;
    }

    /* Wrap nếu cần */
    int16_t wchar = (int16_t)(g->textsize_x * 6); /* 5 cột + 1 cột trống */
    if (g->wrap && (g->cursor_x + wchar > g->width)) {
        g->cursor_y += g->textsize_y * 8;
        g->cursor_x  = 0;
    }

    const uint8_t *glyph = font5x7_glyph(c);
    uint16_t fg = g->textcolor;
    uint16_t bg = g->textbgcolor;
    bool transparent = (fg == bg);

    for (int8_t i = 0; i < 5; i++) {        /* 5 cột */
        uint8_t line = glyph[i];
        for (int8_t j = 0; j < 8; j++) {    /* 8 hàng (bit j) */
            bool on = (line & 0x01);
            if (on) {
                lpm_gfx_fill_rect(g,
                    g->cursor_x + i * g->textsize_x,
                    g->cursor_y + j * g->textsize_y,
                    g->textsize_x, g->textsize_y, fg);
            } else if (!transparent) {
                lpm_gfx_fill_rect(g,
                    g->cursor_x + i * g->textsize_x,
                    g->cursor_y + j * g->textsize_y,
                    g->textsize_x, g->textsize_y, bg);
            }
            line >>= 1;
        }
    }
    /* 1 cột trống bên phải làm kerning cơ bản */
    if (!transparent) {
        lpm_gfx_fill_rect(g,
            g->cursor_x + 5 * g->textsize_x,
            g->cursor_y,
            g->textsize_x, g->textsize_y * 8, bg);
    }
    g->cursor_x += wchar;
}

/* In chuỗi C */
void lpm_gfx_write(lpm_gfx_t *g, const char *s) {
    if (!g || !s) return;
    while (*s) {
        lpm_gfx_write_char(g, *s++);
    }
}

/* printf tiện dụng (buffer cục bộ) */
int lpm_gfx_printf(lpm_gfx_t *g, const char *fmt, ...) {
    if (!g || !fmt) return 0;
    char buf[256];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n > 0) {
        /* Nếu tràn buffer, vẫn in phần vừa đủ */
        buf[sizeof(buf)-1] = '\0';
        lpm_gfx_write(g, buf);
    }
    return n;
}