#include "ws2812.h"

#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include <zephyr/autoconf.h>
#ifdef CONFIG_LED_STRIP
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/led_strip.h>
#endif
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(ws2812, LOG_LEVEL_DBG);

#define RGB(_r, _g, _b) { .r = (_r), .g = (_g), .b = (_b) }

#ifdef CONFIG_LED_STRIP
#if DT_NODE_HAS_STATUS(DT_ALIAS(led_strip), okay)
#define WS2812_STRIP_NODE DT_ALIAS(led_strip)
#define WS2812_HAS_STRIP 1
#define STRIP_NUM_PIXELS DT_PROP(WS2812_STRIP_NODE, chain_length)
static const struct device *const strip = DEVICE_DT_GET(WS2812_STRIP_NODE);
static struct led_rgb pixels[STRIP_NUM_PIXELS];
#else
#define WS2812_HAS_STRIP 0
#endif
#else
#define WS2812_HAS_STRIP 0
#endif

static uint8_t brightness = 100;
static bool ws_ready;


int ws2812_init(void)
{
    ws_ready = false;
#if WS2812_HAS_STRIP
    if (device_is_ready(strip)) {
        ws_ready = true;
        LOG_INF("Found LED strip device %s", strip->name);
        return 0;
    }

    LOG_ERR("LED strip device %s is not ready", strip->name);
    return -ENODEV;
#else
    LOG_WRN("No led_strip alias in devicetree; WS2812 disabled");
    return -ENODEV;
#endif
}

int ws2812_set_brightness(uint8_t brightness_percent)
{
    brightness = CLAMP(brightness_percent, 0, 100);
    return 0;
}

int ws2812_set_rgb(uint8_t red, uint8_t green, uint8_t blue)
{
    uint8_t color[3] = {0};

    if (!ws_ready) {
        return -ENODEV;
    }

    color[0] = red;
    color[1] = green;
    color[2] = blue;
    for (int8_t i = 0; i < 3; i++) {
        color[i] = (uint8_t)((color[i] * brightness) / 100U);
    }

#if WS2812_HAS_STRIP
    struct led_rgb selected = RGB(color[0], color[1], color[2]);
    memset(&pixels, 0x00, sizeof(pixels));
    memcpy(&pixels[0], &selected, sizeof(struct led_rgb));
    return led_strip_update_rgb(strip, pixels, STRIP_NUM_PIXELS);
#else
    return -ENODEV;
#endif
}

int ws2812_set_color(ws2812_color_t color)
{
    switch (color) {
    case WS2812_COLOR_RED:
        return ws2812_set_rgb(255, 0, 0);
    case WS2812_COLOR_GREEN:
        return ws2812_set_rgb(0, 255, 0);
    case WS2812_COLOR_BLUE:
        return ws2812_set_rgb(0, 0, 255);
    case WS2812_COLOR_WHITE:
        return ws2812_set_rgb(255, 255, 255);
    case WS2812_COLOR_PURPLE:
        return ws2812_set_rgb(255, 0, 255);
    case WS2812_COLOR_ORANGE:
        return ws2812_set_rgb(255, 165, 0);
    case WS2812_COLOR_OFF:
    default:
        return ws2812_set_rgb(0, 0, 0);
    }
}


/// @brief Set up led pixel ws2812b  
void beginWS2812(void) {
    (void)ws2812_init();
}

/// @brief Set up brighness led pixel
/// @param _brightness Desired LED pixel light intensity value (range 0 - 100%)
void setBrightNess(uint8_t _brightness) {
    (void)ws2812_set_brightness(_brightness);
}

/// @brief The function to change the light color of LED pixel
/// @param _red 
/// @param _green 
/// @param _blue 
void setBrightColor(uint8_t _red, uint8_t _green, uint8_t _blue) {
    (void)ws2812_set_rgb(_red, _green, _blue);
}


/// @brief Set led pixel red
void red(void) {
    (void)ws2812_set_color(WS2812_COLOR_RED);
}

/// @brief Set led pixel green
void green(void) {
    (void)ws2812_set_color(WS2812_COLOR_GREEN);
}

/// @brief Set led pixel blue
void blue(void) {
    (void)ws2812_set_color(WS2812_COLOR_BLUE);
}

/// @brief Set led pixel white
void white(void) {
    (void)ws2812_set_color(WS2812_COLOR_WHITE);
}

/// @brief Set led pixel no color
void notBright(void) {
    (void)ws2812_set_color(WS2812_COLOR_OFF);
}

/// @brief Set led pixel purple
void purple(void) {
    (void)ws2812_set_color(WS2812_COLOR_PURPLE);
}

/// @brief Set led pixel orange
void orange(void) {
    (void)ws2812_set_color(WS2812_COLOR_ORANGE);
}