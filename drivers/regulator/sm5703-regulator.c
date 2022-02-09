#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/property.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
#include <linux/of_gpio.h>

#define SM5703_REG_LDO1			0x1A
#define SM5703_REG_LDO2			0x1B
#define SM5703_REG_LDO3			0x1C
#define SM5703_LDO_EN			BIT(3)
#define SM5703_MASK_VSET		0x7

#define SM5703_LDO_VOLT_MIN		1500000
#define SM5703_LDO_VOLT_MAX		3300000

#define SM5703_REG_USBLDO12			0x1C
#define SM5703_REG_EN_USBLDO1		BIT(6)
#define SM5703_REG_EN_USBLDO2		BIT(7)

#define SM5703_USBLDO_VOLT_MIN		4800000
#define SM5703_USBLDO_VOLT_MAX		4800000

#define SM5703_DEVICE_ID			0x1E

struct sm5703 {
	struct device *dev;
	struct i2c_client *i2c;
	struct gpio_desc *mrstb;
};

enum sm5703_regulators {
	SM5703_LDO1,
	SM5703_LDO2,
	SM5703_LDO3,
	SM5703_USBLDO1,
	SM5703_USBLDO2,
	SM5703_MAX_REGULATORS,
};

static const int sm5703_ldo_voltagemap[] = {
	1500000, 1800000, 2600000, 2800000, 3000000, 3300000,
};

static const int sm5703_usbldo_voltagemap[] = {
	4800000,
};

#define SM5703USBLDO(_name, _id)					\
	[SM5703_USBLDO ## _id] = {						\
		.id = SM5703_USBLDO ## _id,				\
		.name = _name,						\
		.of_match = _name,					\
		.regulators_node = "regulators",			\
		.ops = &sm5703_usbldo_ops,					\
		.min_uV = SM5703_USBLDO_VOLT_MIN,				\
		.n_voltages = ARRAY_SIZE(sm5703_usbldo_voltagemap), \
		.volt_table = sm5703_usbldo_voltagemap,			\
		.enable_reg = SM5703_REG_USBLDO12,			\
		.enable_mask = SM5703_REG_EN_USBLDO ##_id,			\
		.owner			= THIS_MODULE,			\
	}

#define SM5703LDO(_name, _id)					\
	[SM5703_LDO ## _id] = {						\
		.id = SM5703_LDO ## _id,				\
		.name = _name,						\
		.of_match = _name,					\
		.type = REGULATOR_VOLTAGE,					\
		.regulators_node = "regulators",			\
		.ops = &sm5703_ldo_ops,					\
		.min_uV = SM5703_LDO_VOLT_MIN,				\
		.n_voltages = ARRAY_SIZE(sm5703_ldo_voltagemap), \
		.volt_table = sm5703_ldo_voltagemap,			\
		.vsel_reg = SM5703_REG_LDO ##_id,			\
		.vsel_mask = SM5703_MASK_VSET,				\
		.enable_reg = SM5703_REG_LDO ##_id,			\
		.enable_mask = SM5703_LDO_EN,			\
		.owner			= THIS_MODULE,			\
	}

static int sm5703_regulator_enable(struct regulator_dev *rdev) {
	unsigned char data;
	struct sm5703 *client = (struct sm5703*)rdev->reg_data;
	data = i2c_smbus_read_byte_data(client->i2c, rdev->desc->enable_reg);
	data |= rdev->desc->enable_mask;
	return i2c_smbus_write_byte_data(client->i2c, rdev->desc->enable_reg, data);
}

static int sm5703_regulator_disable(struct regulator_dev *rdev) {
	unsigned char data;
	struct sm5703 *client = (struct sm5703*)rdev->reg_data;
	data = i2c_smbus_read_byte_data(client->i2c, rdev->desc->enable_reg);
	data &= ~(rdev->desc->enable_mask);
	return i2c_smbus_write_byte_data(client->i2c, rdev->desc->enable_reg, data);
}

static int sm5703_regulator_is_enabled(struct regulator_dev *rdev) {
	unsigned char data;
	struct sm5703 *client = (struct sm5703*)rdev->reg_data;
	data = i2c_smbus_read_byte_data(client->i2c, rdev->desc->enable_reg);
	return data & rdev->desc->enable_mask;
}

static int sm5703_regulator_get_voltage(struct regulator_dev *rdev) {
	unsigned char data;
	struct sm5703 *client = (struct sm5703*)rdev->reg_data;
	data = i2c_smbus_read_byte_data(client->i2c, rdev->desc->vsel_reg);
	return rdev->desc->volt_table[data & rdev->desc->vsel_mask];
}

static int sm5703_regulator_set_voltage(struct regulator_dev *rdev, int min_uV, int max_uV, unsigned *selector) {
	unsigned char i;
	unsigned char voltage = rdev->desc->n_voltages;
	struct sm5703 *client = (struct sm5703*)rdev->reg_data;
	for (i = 0; i < rdev->desc->n_voltages; i++) {
		if (rdev->desc->volt_table[i] >= min_uV) {
			if (rdev->desc->volt_table[i] <= max_uV) {
				voltage = i;
				break;
			}
		}
	}
	if (voltage == rdev->desc->n_voltages) return -EINVAL;
	*selector = rdev->desc->volt_table[voltage];
	return i2c_smbus_write_byte_data(client->i2c, rdev->desc->vsel_reg, voltage & SM5703_MASK_VSET);
}

static const struct regulator_ops sm5703_ldo_ops = {
	.enable			= sm5703_regulator_enable,
	.disable		= sm5703_regulator_disable,
	.is_enabled		= sm5703_regulator_is_enabled,
	.get_voltage	= sm5703_regulator_get_voltage,
	.set_voltage	= sm5703_regulator_set_voltage,
};

static const struct regulator_ops sm5703_usbldo_ops = {
	.enable			= sm5703_regulator_enable,
	.disable		= sm5703_regulator_disable,
	.is_enabled		= sm5703_regulator_is_enabled,
	.get_voltage	= sm5703_regulator_get_voltage,
};

static struct regulator_desc sm5703_regulators_desc[SM5703_MAX_REGULATORS] = {
	SM5703LDO("ldo1", 1),
	SM5703LDO("ldo2", 2),
	SM5703LDO("ldo3", 3),
	SM5703USBLDO("usbldo1", 1),
	SM5703USBLDO("usbldo2", 2),
};

static int sm5703_regulator_probe (struct i2c_client* i2c, const struct i2c_device_id* id) {
    printk(KERN_INFO "probe\n");
	struct device* dev;
	int ret, i;
	struct sm5703* ctx;
	struct regulator_config config = { NULL, };
	struct regulator_dev *rdev;
	dev = &i2c->dev;

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx) return -ENOMEM;
	config.dev = dev;
	config.driver_data = ctx;
	config.of_node = dev->of_node;
	ctx->dev = dev;
	ctx->i2c = i2c;

	ctx->mrstb = devm_gpiod_get(dev, "mrstb", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->mrstb)) {
		ret = PTR_ERR(ctx->mrstb);
		return dev_err_probe(dev, ret, "no MRSTB GPIO\n");
	}
	gpiod_set_value_cansleep(ctx->mrstb, 1);

	if (!i2c_smbus_read_byte_data(i2c, SM5703_DEVICE_ID)) {
		dev_err_probe(dev, -EINVAL, "device ID read failure");
	}
	
	// i2c_smbus_write_byte_data(i2c, SM5703_REG_LDO3, 0x5);
	// dev_info(dev, "voltage: %d\n", i2c_smbus_read_byte_data(i2c, SM5703_REG_LDO3));
	// unsigned char data = i2c_smbus_read_byte_data(i2c, SM5703_REG_LDO3);
	// i2c_smbus_write_byte_data(i2c, SM5703_REG_LDO3, data | 8);
	// dev_info(dev, "voltage: %d\n", i2c_smbus_read_byte_data(i2c, SM5703_REG_LDO3));

	for (i = 0; i < SM5703_MAX_REGULATORS; i++) {
		rdev = devm_regulator_register(dev,
					       &sm5703_regulators_desc[i],
					       &config);
		if (IS_ERR(rdev)) {
			dev_err(dev, "Failed to register regulator!\n");
			return PTR_ERR(rdev);
		}
	}
	return 0;
}

static int sm5703_regulator_remove (struct i2c_client* i2c) {
    printk(KERN_INFO "remove\n");
	return 0;
}

static const struct i2c_device_id sm5703_regulator_id[] = {
	{"sm5703-regulator", 0},
	{}
};

static struct of_device_id sm5703_i2c_match_table[] = {
	{ .compatible = "siliconmitus,sm5703-regulator", },
	{ },
};
MODULE_DEVICE_TABLE(i2c, sm5703_i2c_match_table);

static struct i2c_driver sm5703_regulator_driver = {
	.driver = {
		   .name = "sm5703-regulator",
		   .of_match_table = sm5703_i2c_match_table,
	},
	.probe	= sm5703_regulator_probe,
	.remove	= sm5703_regulator_remove,
	.id_table   = sm5703_regulator_id,
};

static int __init sm5703_regulator_init(void)
{
	printk(KERN_INFO "Hello, world!\n");

	return i2c_add_driver(&sm5703_regulator_driver);
}

static void __exit sm5703_regulator_exit(void)
{
	i2c_del_driver(&sm5703_regulator_driver);
}

module_init(sm5703_regulator_init);
module_exit(sm5703_regulator_exit);

MODULE_DESCRIPTION("Samsung SM5703");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");