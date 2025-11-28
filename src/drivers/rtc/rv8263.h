/*
 * RV-8263-C8 RTC Driver
 * I2C Real-Time Clock Interface
 */

#ifndef RV8263_H
#define RV8263_H

#include <stdint.h>

/* RV-8263-C8 I2C address */
#define RV8263_I2C_ADDR         0x51

/* RV-8263-C8 Register addresses */
#define RV8263_REG_CONTROL1     0x00
#define RV8263_REG_CONTROL2     0x01
#define RV8263_REG_SECONDS      0x04
#define RV8263_REG_MINUTES      0x05
#define RV8263_REG_HOURS        0x06
#define RV8263_REG_DAYS         0x07
#define RV8263_REG_WEEKDAYS     0x08
#define RV8263_REG_MONTHS       0x09
#define RV8263_REG_YEARS        0x0A

/* Control register bits */
#define RV8263_CTRL1_STOP       (1 << 5)

/* Time register masks */
#define RV8263_SECONDS_MASK     0x7F
#define RV8263_MINUTES_MASK     0x7F
#define RV8263_HOURS_MASK       0x3F
#define RV8263_DAYS_MASK        0x3F
#define RV8263_WEEKDAYS_MASK    0x07
#define RV8263_MONTHS_MASK      0x1F
#define RV8263_YEARS_MASK       0xFF

/* Time structure */
struct rv8263_time {
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;
	uint8_t day;
	uint8_t weekday;
	uint8_t month;
	uint8_t year;
};

/**
 * Initialize RV-8263 RTC
 * @param i2c_dev Pointer to I2C device
 * @return 0 on success, negative error code on failure
 */
int rv8263_init(const struct device *i2c_dev);

/**
 * Set RTC time
 * @param i2c_dev Pointer to I2C device
 * @param time Pointer to time structure
 * @return 0 on success, negative error code on failure
 */
int rv8263_set_time(const struct device *i2c_dev, const struct rv8263_time *time);

/**
 * Get RTC time
 * @param i2c_dev Pointer to I2C device
 * @param time Pointer to time structure to fill
 * @return 0 on success, negative error code on failure
 */
int rv8263_get_time(const struct device *i2c_dev, struct rv8263_time *time);

#endif /* RV8263_H */
