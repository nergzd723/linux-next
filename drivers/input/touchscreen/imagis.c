// SPDX-License-Identifier: GPL-2.0-only

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/property.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/input/touchscreen.h>

#define IST3038_ADDR_LEN		4
#define IST3038_DATA_LEN		4
#define IST3038_HIB_ACCESS		(0x800B << 16)
#define IST3038_DIRECT_ACCESS   BIT(31)
#define IST3038_REG_CHIPID      0x40001000

#define IST3038_REG_HIB_BASE		(0x30000100)
#define IST3038_REG_TOUCH_STATUS        (IST3038_REG_HIB_BASE | IST3038_HIB_ACCESS)
#define IST3038_REG_TOUCH_COORD        (IST3038_REG_HIB_BASE | IST3038_HIB_ACCESS | 0x8)
#define IST3038_REG_INTR_MESSAGE        (IST3038_REG_HIB_BASE | IST3038_HIB_ACCESS | 0x4)

#define IST3038C_WHOAMI			0x38c
#define CHIP_ON_DELAY				60 // ms

#define I2C_RETRY_COUNT			3

#define MAX_SUPPORTED_FINGER_NUM		10

struct imagis_ts {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct touchscreen_properties prop;
	struct regulator_bulk_data supplies[2];
};

static int imagis_i2c_read_reg(struct imagis_ts *ts,
			       unsigned int reg, unsigned int *buffer)
{
	unsigned int reg_be = __cpu_to_be32(reg);
	struct i2c_msg msg[] = {
		{
			.addr = ts->client->addr,
			.flags = 0,
			.buf = (unsigned char *)&reg_be,
			.len = IST3038_ADDR_LEN,
		}, {
			.addr = ts->client->addr,
			.flags = I2C_M_RD,
			.buf = (unsigned char *)buffer,
			.len = IST3038_DATA_LEN,
		},
	};
	int res;
	int error;
	int retry = I2C_RETRY_COUNT;

	do {
		res = i2c_transfer(ts->client->adapter, msg, ARRAY_SIZE(msg));
		if (res == ARRAY_SIZE(msg)) {
			*buffer = __be32_to_cpu(*buffer);
			return 0;
		}

		error = res < 0 ? res : -EIO;
		dev_err(&ts->client->dev,
			"%s - i2c_transfer failed: %d (%d)\n",
			__func__, error, res);
	} while (--retry);

	return error;
}

static irqreturn_t imagis_interrupt(int irq, void *dev_id)
{
	struct imagis_ts *ts = dev_id;
	unsigned int finger_status, intr_message;
	int ret, i, finger_count, finger_pressed;

	ret = imagis_i2c_read_reg(ts, IST3038_REG_INTR_MESSAGE, &intr_message);
	if (ret) {
		dev_err(&ts->client->dev, "failed to read the interrupt message\n");
		return IRQ_HANDLED;
	}

	finger_count = (intr_message >> 12) & 0xF;
	finger_pressed = intr_message & 0x3FF;
	if (finger_count > 10) {
		dev_err(&ts->client->dev, "finger count is more than maximum supported\n");
		return IRQ_HANDLED;
	}

	for (i = 0; i < finger_count; i++) {
		ret = imagis_i2c_read_reg(ts, IST3038_REG_TOUCH_COORD + (i * 4), &finger_status);
		if (ret) {
			dev_err(&ts->client->dev, "failed to read coordinates for finger %d\n", i);
			return IRQ_HANDLED;
		}
		input_mt_slot(ts->input_dev, i);
		input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER,
					   (finger_pressed & BIT(i)) ? 1 : 0);
		touchscreen_report_pos(ts->input_dev, &ts->prop,
				       (finger_status >> 12) & 0xFFF, finger_status & 0xFFF, 1);
		input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, (finger_status >> 24) & 0xFFF);
	}
	input_mt_sync_frame(ts->input_dev);
	input_sync(ts->input_dev);

	return IRQ_HANDLED;
}

static int imagis_start(struct imagis_ts *ts)
{
	int error;

	error = regulator_bulk_enable(ARRAY_SIZE(ts->supplies),
				      ts->supplies);
	if (error) {
		dev_err(&ts->client->dev,
			"Failed to enable regulators: %d\n", error);
		return error;
	}

	msleep(CHIP_ON_DELAY);

	enable_irq(ts->client->irq);
	return 0;
}

static int imagis_stop(struct imagis_ts *ts)
{
	int error;

	disable_irq(ts->client->irq);

	error = regulator_bulk_disable(ARRAY_SIZE(ts->supplies),
				       ts->supplies);
	if (error) {
		dev_err(&ts->client->dev,
			"Failed to disable regulators: %d\n", error);
		return error;
	}

	return 0;
}

static int imagis_input_open(struct input_dev *dev)
{
	struct imagis_ts *ts = input_get_drvdata(dev);

	return imagis_start(ts);
}

static void imagis_input_close(struct input_dev *dev)
{
	struct imagis_ts *ts = input_get_drvdata(dev);

	imagis_stop(ts);
}

static int imagis_init_input_dev(struct imagis_ts *ts)
{
	struct input_dev *input_dev;
	int error;

	input_dev = devm_input_allocate_device(&ts->client->dev);
	if (!input_dev) {
		dev_err(&ts->client->dev,
			"Failed to allocate input device.");
		return -ENOMEM;
	}

	ts->input_dev = input_dev;

	input_dev->name = "Imagis capacitive touchscreen";
	input_dev->phys = "input/ts";
	input_dev->id.bustype = BUS_I2C;
	input_dev->open = imagis_input_open;
	input_dev->close = imagis_input_close;

	input_set_drvdata(input_dev, ts);

	input_set_capability(input_dev, EV_ABS, ABS_MT_POSITION_X);
	input_set_capability(input_dev, EV_ABS, ABS_MT_POSITION_Y);
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);

	touchscreen_parse_properties(input_dev, true, &ts->prop);
	if (!ts->prop.max_x || !ts->prop.max_y) {
		dev_err(&ts->client->dev,
			"Touchscreen-size-x and/or touchscreen-size-y not set in dts\n");
		return -EINVAL;
	}

	error = input_mt_init_slots(input_dev, MAX_SUPPORTED_FINGER_NUM,
				    INPUT_MT_DIRECT | INPUT_MT_DROP_UNUSED);
	if (error) {
		dev_err(&ts->client->dev,
			"Failed to initialize MT slots: %d", error);
		return error;
	}

	error = input_register_device(input_dev);
	if (error) {
		dev_err(&ts->client->dev,
			"Failed to register input device: %d", error);
		return error;
	}

	return 0;
}

static int imagis_init_regulators(struct imagis_ts *ts)
{
	struct i2c_client *client = ts->client;
	int error;

	ts->supplies[0].supply = "vdd";
	ts->supplies[1].supply = "vddio";
	error = devm_regulator_bulk_get(&client->dev,
					ARRAY_SIZE(ts->supplies),
					ts->supplies);
	if (error < 0) {
		dev_err(&client->dev, "Failed to get regulators: %d\n", error);
		return error;
	}

	return 0;
}

static int imagis_probe(struct i2c_client *i2c)
{
	struct device *dev;
	struct imagis_ts *ts;
	int chip_id, ret;

	dev = &i2c->dev;

	ts = devm_kzalloc(dev, sizeof(*ts), GFP_KERNEL);
	if (!ts)
		return -ENOMEM;

	ts->client = i2c;

	ret = devm_request_threaded_irq(dev, i2c->irq,
					NULL, imagis_interrupt,
					IRQF_ONESHOT | IRQF_TRIGGER_FALLING,
					"imagis-touchscreen", ts);
	if (ret)
		return dev_err_probe(dev, ret, "IRQ allocation failure: %d\n", ret);

	ret = imagis_init_regulators(ts);
	if (ret)
		return dev_err_probe(dev, ret, "regulator init error: %d\n", ret);

	ret = imagis_start(ts);
	if (ret)
		return dev_err_probe(dev, ret, "regulator enable error: %d\n", ret);

	ret = imagis_i2c_read_reg(ts, IST3038_REG_CHIPID | IST3038_DIRECT_ACCESS, &chip_id);
	if (ret)
		return dev_err_probe(dev, ret, "chip ID read failure: %d\n", ret);

	if (chip_id == IST3038C_WHOAMI)
		dev_info(dev, "Detected IST3038C chip\n");
	else
		return dev_err_probe(dev, -EINVAL, "unknown chip ID: 0x%x\n", chip_id);

	ret = imagis_init_input_dev(ts);
	if (ret)
		return dev_err_probe(dev, ret, "input subsystem init error: %d\n", ret);

	return 0;
}

static int __maybe_unused imagis_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct imagis_ts *ts = i2c_get_clientdata(client);

	mutex_lock(&ts->input_dev->mutex);

	if (input_device_enabled(ts->input_dev))
		imagis_stop(ts);

	mutex_unlock(&ts->input_dev->mutex);

	return 0;
}

static int __maybe_unused imagis_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct imagis_ts *ts = i2c_get_clientdata(client);

	mutex_lock(&ts->input_dev->mutex);

	if (input_device_enabled(ts->input_dev))
		imagis_start(ts);

	mutex_unlock(&ts->input_dev->mutex);

	return 0;
}

static SIMPLE_DEV_PM_OPS(imagis_pm_ops, imagis_suspend, imagis_resume);

#ifdef CONFIG_OF
static const struct of_device_id imagis_of_match[] = {
	{ .compatible = "imagis,ist3038c", },
	{ },
};
MODULE_DEVICE_TABLE(i2c, imagis_of_match);
#endif

static struct i2c_driver imagis_ts_driver = {
	.driver = {
		   .name = "imagis-touchscreen",
		   .pm = &imagis_pm_ops,
		   .of_match_table = of_match_ptr(imagis_of_match),
	},
	.probe_new	= imagis_probe,
};

module_i2c_driver(imagis_ts_driver);

MODULE_DESCRIPTION("Imagis IST3038C Touchscreen Driver");
MODULE_AUTHOR("Markuss Broks <markuss.broks@gmail.com>");
MODULE_LICENSE("GPL v2");
