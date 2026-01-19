#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>

#include <errno.h>
#include <string.h>

#define LOG_LEVEL 4
LOG_MODULE_REGISTER(main);

static const struct gpio_dt_spec vib_en   = GPIO_DT_SPEC_GET(DT_ALIAS(vib_en), gpios);

static const struct pwm_dt_spec vib_pwm = PWM_DT_SPEC_GET(DT_ALIAS(pwm_vibrator));


/* =========================
 * Chọn mode theo phần cứng
 * =========================
 * 1 = LRA (LRA/ERM kéo HIGH)
 * 0 = ERM (LRA/ERM kéo LOW)
 */
#define DRV2603_IS_LRA   1

static inline uint32_t clamp_u32(uint32_t v, uint32_t lo, uint32_t hi)
{
	return (v < lo) ? lo : (v > hi) ? hi : v;
}

/* duty_permille: 0..1000 => 0..100% */
static int vib_pwm_set_permille(uint32_t period_ns, uint32_t duty_permille)
{
	duty_permille = clamp_u32(duty_permille, 0, 1000);

	uint32_t pulse_ns = (uint32_t)(((uint64_t)period_ns * duty_permille) / 1000ULL);
	return pwm_set_dt(&vib_pwm, period_ns, pulse_ns);
}

static void drv2603_enable(bool on)
{
	gpio_pin_set_dt(&vib_en, on ? 1 : 0);
}

/* intensity: 0..100 (%)
 * - ERM: 50% = stop, 50..100% = quay thuận mạnh dần
 * - LRA: 50..100% = tăng biên độ; 0..50% là vùng brake/stop
 */
static int drv2603_set_intensity(uint8_t intensity)
{
	uint32_t period = vib_pwm.period;
	intensity = (uint8_t)clamp_u32(intensity, 0, 100);

	/* Map 0..100 -> 500..1000 permille */
	uint32_t duty_permille = 500 + (uint32_t)intensity * 5;
	return vib_pwm_set_permille(period, duty_permille);
}

/* Stop “êm”: duty = 50% */
static int drv2603_stop(void)
{
	return vib_pwm_set_permille(vib_pwm.period, 500);
}

/* Brake nhanh: duty = 0% (dùng ngắn) */
static int drv2603_brake_strong(void)
{
	return vib_pwm_set_permille(vib_pwm.period, 0);
}

/* 1 click haptic (default hợp ERM) */
static void vib_click(void)
{
	// /* an toàn */
	// (void)drv2603_stop();

	// drv2603_enable(true);
	// k_sleep(K_MSEC(2)); /* cho IC wake */

	/* overdrive */
	(void)drv2603_set_intensity(100);
	k_sleep(K_MSEC(25));

	// /* brake */
	// (void)drv2603_brake_strong();
	// k_sleep(K_MSEC(10));

	// /* stop + disable */
	// (void)drv2603_stop();
	// drv2603_enable(false);
}

int main(void)
{
	int ret;

	LOG_INF("DRV2603 vibrator demo (vib_en + pwm_vibrator)");

	if (!pwm_is_ready_dt(&vib_pwm)) {
		LOG_INF("Error: PWM not ready (%s)", vib_pwm.dev->name);
		return 0;
	}
	if (!gpio_is_ready_dt(&vib_en)) {
		LOG_INF("Error: vib_en GPIO not ready");
		return 0;
	}

	ret = gpio_pin_configure_dt(&vib_en, GPIO_OUTPUT_INACTIVE);
	if (ret) {
		LOG_INF("Error %d: configure vib_en", ret);
		return 0;
	}
        k_msleep(300);
	/* trạng thái ban đầu */
	(void)drv2603_stop();
	drv2603_enable(false);

	while (1) {
		vib_click();
		k_sleep(K_SECONDS(2));
	}

	return 0;
}