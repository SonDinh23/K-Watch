#include "RV8263_C8.h"

#include <stdio.h>
#include <string.h>

LOG_MODULE_REGISTER(rv8263_c8_simple, LOG_LEVEL_DBG);

static uint8_t dec_to_bcd(uint8_t v)
{
	return (uint8_t)((v / 10U * 16U) + (v % 10U));
}

static uint8_t bcd_to_dec(uint8_t v)
{
	return (uint8_t)((v / 16U * 10U) + (v % 16U));
}

static int write_reg(const struct rv8263_dev *dev, uint8_t reg, uint8_t val)
{
	return i2c_reg_write_byte_dt(&dev->bus, reg, val);
}

int rv8263_init(struct rv8263_dev *dev, const struct i2c_dt_spec *bus)
{
	if (dev == NULL || bus == NULL) {
		return -EINVAL;
	}

	*dev = (struct rv8263_dev){
		.bus = *bus,
	};

	if (!i2c_is_ready_dt(&dev->bus)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}

	/* Disable timer and clear control registers to a safe state. */
	(void)write_reg(dev, TIMER_VALUE_REG, 0x00);
	(void)write_reg(dev, TIMER_MODE_REG, 0x00);
	(void)write_reg(dev, CONTROL1_REG, 0x00); /* 24h, clock on */
	(void)write_reg(dev, CONTROL2_REG, 0x00); /* alarms off */

	return 0;
}

int rv8263_init_default(struct rv8263_dev *dev)
{
#if DT_NODE_EXISTS(DT_NODELABEL(rv8263c8))
	static const struct i2c_dt_spec bus = I2C_DT_SPEC_GET(DT_NODELABEL(rv8263c8));
	return rv8263_init(dev, &bus);
#else
	ARG_UNUSED(dev);
	return -ENODEV;
#endif
}

int rv8263_set_time(struct rv8263_dev *dev, const struct rv8263_time *t)
{
	uint8_t buf[8];

	if (dev == NULL || t == NULL) {
		return -EINVAL;
	}

	/* Basic range checks. */
	if (t->seconds > 59 || t->minutes > 59 || t->hours > 23 || t->date == 0 ||
	    t->date > 31 || t->month == 0 || t->month > 12 || t->weekday > RV_SATURDAY) {
		return -EINVAL;
	}

	buf[0] = SECONDS_REG;
	buf[1] = dec_to_bcd(t->seconds) & 0x7F; /* keep OS bit clear */
	buf[2] = dec_to_bcd(t->minutes);
	buf[3] = dec_to_bcd(t->hours);
	buf[4] = dec_to_bcd(t->date);
	buf[5] = (uint8_t)t->weekday;
	buf[6] = dec_to_bcd(t->month);
	buf[7] = dec_to_bcd((uint8_t)(t->year % 100U));

	return i2c_write_dt(&dev->bus, buf, sizeof(buf));
}

int rv8263_get_time(struct rv8263_dev *dev, struct rv8263_time *t)
{
	uint8_t regs[7];

	if (dev == NULL || t == NULL) {
		return -EINVAL;
	}

	int err = i2c_burst_read_dt(&dev->bus, SECONDS_REG, regs, sizeof(regs));
	if (err < 0) {
		return err;
	}

	/* OSC stop flag is bit 7 of seconds. */
	if (regs[0] & BIT(7)) {
		return -ENODATA;
	}

	t->seconds = bcd_to_dec(regs[0] & 0x7F);
	t->minutes = bcd_to_dec(regs[1]);
	t->hours = bcd_to_dec(regs[2] & 0x3F); /* 24h */
	t->date = bcd_to_dec(regs[3]);
	t->weekday = (enum rv8263_weekday)(regs[4] & 0x07);
	t->month = bcd_to_dec(regs[5] & 0x1F);
	t->year = 2000U + bcd_to_dec(regs[6]);

	return 0;
}

int rv8263_get_epoch(struct rv8263_dev *dev, time_t *epoch_out)
{
	struct rv8263_time t;
	struct tm tmval = {0};

	if (epoch_out == NULL) {
		return -EINVAL;
	}

	int err = rv8263_get_time(dev, &t);
	if (err < 0) {
		return err;
	}

	tmval.tm_sec = t.seconds;
	tmval.tm_min = t.minutes;
	tmval.tm_hour = t.hours;
	tmval.tm_mday = t.date;
	tmval.tm_mon = t.month - 1; /* struct tm month is 0-11 */
	tmval.tm_year = (int)t.year - 1900; /* struct tm years since 1900 */
	tmval.tm_wday = t.weekday;
	tmval.tm_isdst = 0;

	*epoch_out = mktime(&tmval);
	return 0;
}

int rv8263_set_time_from_compile(struct rv8263_dev *dev)
{
	static const char *const months[] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
	char mon[4] = {0};
	int day;
	int year;
	int hh;
	int mm;
	int ss;
	int month = -1;
	struct rv8263_time t;

	if (sscanf(__DATE__, "%3s %d %d", mon, &day, &year) != 3) {
		return -EINVAL;
	}

	if (sscanf(__TIME__, "%d:%d:%d", &hh, &mm, &ss) != 3) {
		return -EINVAL;
	}

	for (int i = 0; i < 12; i++) {
		if (strcmp(mon, months[i]) == 0) {
			month = i + 1;
			break;
		}
	}

	if (month < 1) {
		return -EINVAL;
	}

	t.seconds = (uint8_t)ss;
	t.minutes = (uint8_t)mm;
	t.hours = (uint8_t)hh;
	t.date = (uint8_t)day;
	t.month = (uint8_t)month;
	t.year = (uint16_t)year;
	t.weekday = RV_SUNDAY;

	/* Keep existing weekday when available; compile-time init uses a safe default. */

	return rv8263_set_time(dev, &t);
}
