#include <zephyr/kernel.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/printk.h>
#include <inttypes.h>

#define BUTTON_COUNT 4

#if !DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(btn0)) || \
    !DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(btn1)) || \
    !DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(btn2)) || \
    !DT_NODE_HAS_STATUS_OKAY(DT_ALIAS(btn3))
#error "Unsupported board: btn0 - btn3 devicetree alias is not defined"
#endif

struct button_data {
	struct gpio_dt_spec spec;
	struct gpio_callback cb;
	const char *name;
};

#define BUTTON_INIT(idx) { \
	.spec = GPIO_DT_SPEC_GET_OR(DT_ALIAS(btn##idx), gpios, {0}), \
	.name = "BTN" #idx, \
}

static struct button_data buttons[BUTTON_COUNT] = {
	BUTTON_INIT(0),
	BUTTON_INIT(1),
	BUTTON_INIT(2),
	BUTTON_INIT(3),
};

static void button_pressed(const struct device *port, struct gpio_callback *cb,
			   uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	struct button_data *data = CONTAINER_OF(cb, struct button_data, cb);
	int state = gpio_pin_get_dt(&data->spec);

	printk("%s %s at %" PRIu32 "\n", data->name,
	       state ? "pressed" : "released", k_cycle_get_32());
}

int main(void)
{
	printk("Starting button interrupt example\n");
	for (int i = 0; i < BUTTON_COUNT; i++) {
		int ret;

		if (!gpio_is_ready_dt(&buttons[i].spec)) {
			printk("Error: button %s device %s is not ready\n",
			       buttons[i].name, buttons[i].spec.port->name);
			return 0;
		}

		ret = gpio_pin_configure_dt(&buttons[i].spec, GPIO_INPUT);
		if (ret != 0) {
			printk("Error %d: failed to configure %s pin %d for %s\n",
			       ret, buttons[i].spec.port->name,
			       buttons[i].spec.pin, buttons[i].name);
			return 0;
		}

		ret = gpio_pin_interrupt_configure_dt(&buttons[i].spec,
						      GPIO_INT_EDGE_BOTH);
		if (ret != 0) {
			printk("Error %d: failed to configure interrupt on %s pin %d for %s\n",
			       ret, buttons[i].spec.port->name,
			       buttons[i].spec.pin, buttons[i].name);
			return 0;
		}

		gpio_init_callback(&buttons[i].cb, button_pressed,
				   BIT(buttons[i].spec.pin));
		gpio_add_callback(buttons[i].spec.port, &buttons[i].cb);
		printk("Set up button %s at %s pin %d\n", buttons[i].name,
		       buttons[i].spec.port->name, buttons[i].spec.pin);
	}

	printk("Press any of the four buttons\n");
	while (1) {
		k_msleep(200);
	}
}
