#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2c.h>
#include "drivers/rtc/rv8263.h"
#include "ui/watchface/face_manager.h"

LOG_MODULE_REGISTER(main);

static const struct device *i2c_dev;

int rtc_init(void)
{
    i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
    
    if (!device_is_ready(i2c_dev)) {
        LOG_ERR("I2C device not ready");
        return -ENODEV;
    }

    int ret = rv8263_init(i2c_dev);
    if (ret < 0) {
        LOG_ERR("Failed to initialize RTC: %d", ret);
        return ret;
    }

    LOG_INF("RTC initialized");
    return 0;
}

int rtc_read_time(struct rv8263_time *time)
{
    if (!time) {
        return -EINVAL;
    }

    int ret = rv8263_get_time(i2c_dev, time);
    if (ret < 0) {
        LOG_ERR("Failed to read RTC time: %d", ret);
        return ret;
    }

    LOG_INF("RTC Time: %02d:%02d:%02d %02d/%02d/%02d (weekday: %d)",
            time->hours, time->minutes, time->seconds,
            time->day, time->month, time->year, time->weekday);

    return 0;
}

int rtc_set_time(const struct rv8263_time *time)
{
    if (!time) {
        return -EINVAL;
    }

    int ret = rv8263_set_time(i2c_dev, time);
    if (ret < 0) {
        LOG_ERR("Failed to set RTC time: %d", ret);
        return ret;
    }

    LOG_INF("RTC time set to: %02d:%02d:%02d %02d/%02d/%02d",
            time->hours, time->minutes, time->seconds,
            time->day, time->month, time->year);

    return 0;
}

int main(void)
{
    LOG_INF("K-Watch Firmware started");
    
    if (rtc_init() < 0) {
        LOG_ERR("RTC initialization failed");
    }

    face_manager_init();
    
    uint32_t counter = 0;
    while (1) {
        k_sleep(K_SECONDS(1));
        
        if (++counter >= 1) {
            counter = 0;
            struct rv8263_time time;
            if (rtc_read_time(&time) == 0) {
                /* Update digital watchface with current time and date */
                face_manager_show_digital_with_date(time.hours, time.minutes, time.seconds,
                                                     time.day, time.month, time.weekday);
            }
        }
    }
    return 0;
}
