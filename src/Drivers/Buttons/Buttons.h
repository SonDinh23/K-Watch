#ifndef BUTTONS_H
#define BUTTONS_H

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

#define BUTTON_COUNT 4

#define BUTTON_PIN_0    DT_ALIAS(sw0)
#define BUTTON_PIN_1    DT_ALIAS(sw1)
#define BUTTON_PIN_2    DT_ALIAS(sw2)
#define BUTTON_PIN_3    DT_ALIAS(sw3)

struct button {
    const struct gpio_dt_spec *gpio;
    uint32_t debounce_time;
    void (*callback)(void);
};

void buttons_init(const struct button *buttons);
void buttons_poll(void);

#endif // BUTTONS_H