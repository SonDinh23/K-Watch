/*
 * RV-8263-C8 RTC Driver Implementation
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include "rv8263.h"

/* BCD conversion macros */
#define BCD2BIN(val)  (((val) & 0x0F) + ((val) >> 4) * 10)
#define BIN2BCD(val)  ((((val) / 10) << 4) + (val) % 10)

static int rv8263_write_reg(const struct device *i2c_dev, uint8_t reg, uint8_t value)
{
	uint8_t data[2] = {reg, value};
	return i2c_write(i2c_dev, data, 2, RV8263_I2C_ADDR);
}

static int rv8263_read_reg(const struct device *i2c_dev, uint8_t reg, uint8_t *value)
{
	return i2c_write_read(i2c_dev, RV8263_I2C_ADDR, &reg, 1, value, 1);
}

static int rv8263_read_regs(const struct device *i2c_dev, uint8_t reg, uint8_t *buf, uint8_t len)
{
	return i2c_write_read(i2c_dev, RV8263_I2C_ADDR, &reg, 1, buf, len);
}

int rv8263_init(const struct device *i2c_dev)
{
	int ret;
	uint8_t ctrl;

	if (!device_is_ready(i2c_dev)) {
		return -ENODEV;
	}

	ret = rv8263_read_reg(i2c_dev, RV8263_REG_CONTROL1, &ctrl);
	if (ret < 0) {
		return ret;
	}

	ctrl &= ~RV8263_CTRL1_STOP;
	ret = rv8263_write_reg(i2c_dev, RV8263_REG_CONTROL1, ctrl);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

int rv8263_set_time(const struct device *i2c_dev, const struct rv8263_time *time)
{
	uint8_t data[8];
	int ret;

	if (!time) {
		return -EINVAL;
	}

	ret = rv8263_read_reg(i2c_dev, RV8263_REG_CONTROL1, &data[0]);
	if (ret < 0) {
		return ret;
	}
	
	data[0] |= RV8263_CTRL1_STOP;
	ret = rv8263_write_reg(i2c_dev, RV8263_REG_CONTROL1, data[0]);
	if (ret < 0) {
		return ret;
	}

	data[0] = RV8263_REG_SECONDS;
	data[1] = BIN2BCD(time->seconds);
	data[2] = BIN2BCD(time->minutes);
	data[3] = BIN2BCD(time->hours);
	data[4] = BIN2BCD(time->day);
	data[5] = time->weekday;
	data[6] = BIN2BCD(time->month);
	data[7] = BIN2BCD(time->year);

	ret = i2c_write(i2c_dev, data, 8, RV8263_I2C_ADDR);
	if (ret < 0) {
		return ret;
	}

	ret = rv8263_read_reg(i2c_dev, RV8263_REG_CONTROL1, &data[0]);
	if (ret < 0) {
		return ret;
	}
	
	data[0] &= ~RV8263_CTRL1_STOP;
	ret = rv8263_write_reg(i2c_dev, RV8263_REG_CONTROL1, data[0]);
	
	return ret;
}

int rv8263_get_time(const struct device *i2c_dev, struct rv8263_time *time)
{
	uint8_t data[7];
	int ret;

	if (!time) {
		return -EINVAL;
	}

	ret = rv8263_read_regs(i2c_dev, RV8263_REG_SECONDS, data, 7);
	if (ret < 0) {
		return ret;
	}

	time->seconds = BCD2BIN(data[0] & RV8263_SECONDS_MASK);
	time->minutes = BCD2BIN(data[1] & RV8263_MINUTES_MASK);
	time->hours = BCD2BIN(data[2] & RV8263_HOURS_MASK);
	time->day = BCD2BIN(data[3] & RV8263_DAYS_MASK);
	time->weekday = data[4] & RV8263_WEEKDAYS_MASK;
	time->month = BCD2BIN(data[5] & RV8263_MONTHS_MASK);
	time->year = BCD2BIN(data[6] & RV8263_YEARS_MASK);

	return 0;
}
