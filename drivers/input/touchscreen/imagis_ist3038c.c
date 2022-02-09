#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/property.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
#include <linux/of_gpio.h>
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

struct imagis_ts {
	struct i2c_client *client;
    struct device *dev;
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
			.buf = (unsigned char*) &reg_be,
			.len = IST3038_ADDR_LEN,
		}, {
			.addr = ts->client->addr,
			.flags = I2C_M_RD,
			.buf = (unsigned char*) buffer,
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
    finger_count = (intr_message >> 12) & 0xF;
    finger_pressed = intr_message & 0x3FF;

    //dev_info(ts->dev, "intr_message: fingerstatus: %d, reserved: %d, finger count: %d, checksum: %d, id: %d\n", intr_message & 0x3FF, (intr_message >> 10) & 3,  (intr_message >> 12) & 0xF, (intr_message >> 24) & 0xFF, (finger_status >> 24) & 0xFF);

    for (i = 0; i < finger_count; i++){
        ret = imagis_i2c_read_reg(ts, IST3038_REG_TOUCH_COORD + (i * 4), &finger_status);
        input_mt_slot(ts->input_dev, i);
        input_mt_report_slot_state(ts->input_dev, MT_TOOL_FINGER, finger_pressed & BIT(i) ? 1 : 0);
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
	int i;

	input_dev = devm_input_allocate_device(ts->dev);
	if (!input_dev) {
		dev_err(&ts->client->dev,
			"Failed to allocate input device.");
		return -ENOMEM;
	}

	ts->input_dev = input_dev;

	input_dev->name = "Imagis IST3038 capacitive touchscreen";
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

	error = input_mt_init_slots(input_dev, 2,
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

static int imagis_probe (struct i2c_client* i2c, const struct i2c_device_id* id) {
    struct device *dev;
    struct imagis_ts *ts;
    int ret;
    dev = &i2c->dev;

	ts = devm_kzalloc(dev, sizeof(*ts), GFP_KERNEL);
	if (!ts) return -ENOMEM;
	ts->client = i2c;
    ts->dev = dev;

	ret = devm_request_threaded_irq(dev, i2c->irq,
					  NULL, imagis_interrupt,
					  IRQF_ONESHOT | IRQF_TRIGGER_FALLING,
					  "imagis-touchscreen", ts);
    if (ret < 0) {
		dev_err_probe(dev, ret, "IRQ allocation failure: %d\n", ret);
		return ret;
	}

    ret = imagis_init_regulators(ts);
	if (ret < 0) {
		dev_err_probe(dev, ret, "regulator init error: %d\n", ret);
		return ret;
	}

    ret = imagis_start(ts);
	if (ret < 0) {
		dev_err_probe(dev, ret, "regulator enable error: %d\n", ret); 
		return ret;
	}

    unsigned int chip_id;
    ret = imagis_i2c_read_reg(ts, IST3038_REG_CHIPID | IST3038_DIRECT_ACCESS, &chip_id);

	if (chip_id == IST3038C_WHOAMI){
		dev_info(dev, "Detected IST3038C chip\n");
	}
	else {
		dev_err_probe(dev, -EINVAL, "Unknown chip ID: 0x%x\n", chip_id);
	}

    ret = imagis_init_input_dev(ts);
	if (IS_ERR(&ret)) dev_err_probe(dev, ret, "input subsystem init error: %d\n", ret);

	return 0;
	
}

static int imagis_remove (struct i2c_client* i2c) {
    struct device *dev;
    dev = &i2c->dev;
    dev_info(dev, "remove\n");
	return 0;
}

static const struct i2c_device_id imagis_id[] = {
	{"imagis-touchscreen", 0},
	{}
};

static struct of_device_id imagis_i2c_match_table[] = {
	{ .compatible = "imagis,ist3038c", },
	{ },
};
MODULE_DEVICE_TABLE(i2c, imagis_i2c_match_table);

static struct i2c_driver imagis_driver = {
	.driver = {
		   .name = "imagis-touchscreen",
		   .of_match_table = imagis_i2c_match_table,
	},
	.probe	= imagis_probe,
	.remove	= imagis_remove,
	.id_table   = imagis_id,
};

static int __init imagis_init(void)
{
	printk(KERN_INFO "Hello, world!\n");

	return i2c_add_driver(&imagis_driver);
}

static void __exit imagis_exit(void)
{
	i2c_del_driver(&imagis_driver);
}

module_init(imagis_init);
module_exit(imagis_exit);

MODULE_DESCRIPTION("Imagis IST3038C Touchscreen Driver");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");