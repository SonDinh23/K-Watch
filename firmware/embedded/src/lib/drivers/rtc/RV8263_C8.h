/* Simple C-style RV-8263-C8 helper for Zephyr
 * Based on Arduino-style API but implemented in plain C.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

/* 7-bit I2C address */
#define RV8263_ADDR 0x51

/* Register map */
#define CONTROL1_REG       0x00
#define CONTROL2_REG       0x01
#define OFFSET_REG         0x02
#define RAM_REG            0x03
#define SECONDS_REG        0x04
#define MINUTES_REG        0x05
#define HOURS_REG          0x06
#define DATE_REG           0x07
#define WEEKDAY_REG        0x08
#define MONTH_REG          0x09
#define YEAR_REG           0x0A
#define SECONDS_ALARM_REG  0x0B
#define MINUTES_ALARM_REG  0x0C
#define HOURS_ALARM_REG    0x0D
#define DATE_ALARM_REG     0x0E
#define WEEKDAY_ALARM_REG  0x0F
#define TIMER_VALUE_REG    0x10
#define TIMER_MODE_REG     0x11

enum rv8263_weekday {
	RV_SUNDAY = 0,
	RV_MONDAY,
	RV_TUESDAY,
	RV_WEDNESDAY,
	RV_THURSDAY,
	RV_FRIDAY,
	RV_SATURDAY,
};

struct rv8263_time {
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;
	enum rv8263_weekday weekday;
	uint8_t date;
	uint8_t month;
	uint16_t year; /* full year, e.g. 2026 */
};

struct rv8263_dev {
	struct i2c_dt_spec bus; /* I2C bus + address */
};

int rv8263_init(struct rv8263_dev *dev, const struct i2c_dt_spec *bus);
/* Init using devicetree node label rv8263c8 */
int rv8263_init_default(struct rv8263_dev *dev);
int rv8263_set_time(struct rv8263_dev *dev, const struct rv8263_time *t);
int rv8263_get_time(struct rv8263_dev *dev, struct rv8263_time *t);
int rv8263_get_epoch(struct rv8263_dev *dev, time_t *epoch_out);
int rv8263_set_time_from_compile(struct rv8263_dev *dev);

/* ===== Unified API style (non-breaking wrappers) ===== */
typedef struct rv8263_dev rtc_dev_t;
typedef struct rv8263_time rtc_time_t;

static inline int rtc_init(rtc_dev_t *dev, const struct i2c_dt_spec *bus)
{
	return rv8263_init(dev, bus);
}

static inline int rtc_init_default(rtc_dev_t *dev)
{
	return rv8263_init_default(dev);
}

static inline int rtc_set_time(rtc_dev_t *dev, const rtc_time_t *t)
{
	return rv8263_set_time(dev, t);
}

static inline int rtc_get_time(rtc_dev_t *dev, rtc_time_t *t)
{
	return rv8263_get_time(dev, t);
}

static inline int rtc_get_epoch(rtc_dev_t *dev, time_t *epoch_out)
{
	return rv8263_get_epoch(dev, epoch_out);
}

static inline int rtc_set_time_from_compile(rtc_dev_t *dev)
{
	return rv8263_set_time_from_compile(dev);
}
