// SPDX-License-Identifier: GPL-2.0-only
/*
 * MFD core driver for the Silicon Mitus SM5703.
 *
 * SM5703 comprises multiple sub-devices: charger, fuel gauge,
 * flash LED, LDO and BUCK regulators.
 */

#include <linux/err.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/of_device.h>
#include <linux/mfd/core.h>
#include <linux/regmap.h>
#include <linux/i2c.h>
#include <linux/gpio/consumer.h>
#include <linux/mfd/sm5703.h>

static const struct mfd_cell sm5703_devs[] = {
	{
		.name = "sm5703-regulator",
		.of_compatible = "siliconmitus,sm5703-regulator",
	},
	{
		.name = "sm5703-flash",
		.of_compatible = "siliconmitus,sm5703-flash",
	},
	{
		.name = "sm5703-charger",
		.of_compatible = "siliconmitus,sm5703-charger",
	},
};

static const struct regmap_config sm5703_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 8,
};

static int sm5703_i2c_probe(struct i2c_client *i2c,
				const struct i2c_device_id *id)
{
	struct sm5703_dev *sm5703;
	unsigned int dev_id;
	int ret;

	sm5703 = devm_kzalloc(&i2c->dev, sizeof(*sm5703), GFP_KERNEL);
	if (!sm5703)
		return -ENOMEM;

	i2c_set_clientdata(i2c, sm5703);
	sm5703->dev = &i2c->dev;
    sm5703->i2c = i2c;

	sm5703->regmap = devm_regmap_init_i2c(i2c, &sm5703_regmap_config);
	if (IS_ERR(sm5703->regmap)) {
		dev_err(&i2c->dev, "Failed to allocate register map.\n");
		return PTR_ERR(sm5703->regmap);
	}

	sm5703->reset_gpio = devm_gpiod_get(&i2c->dev, "reset", GPIOD_OUT_HIGH);

	if (IS_ERR(sm5703->reset_gpio)) {
		ret = PTR_ERR(sm5703->reset_gpio);
		return dev_err_probe(&i2c->dev, ret, "no reset GPIO\n");
	}
	gpiod_set_value_cansleep(sm5703->reset_gpio, 1);

	ret = regmap_read(sm5703->regmap, SM5703_DEVICE_ID, &dev_id);
	if (ret) {
		dev_err(&i2c->dev, "Device not found\n");
		return -ENODEV;
	}

	ret = devm_mfd_add_devices(sm5703->dev, -1, sm5703_devs,
				   ARRAY_SIZE(sm5703_devs), NULL, 0, NULL);
	if (ret < 0) {
		dev_err(&i2c->dev, "Failed to add SM5703 child devices.\n");
		return ret;
	}
    dev_err(&i2c->dev, "probe done\n");
	return 0;
}

static const struct of_device_id sm5703_dt_match[] = {
	{ .compatible = "siliconmitus,sm5703-mfd", },
	{ }
};
MODULE_DEVICE_TABLE(of, sm5703_dt_match);

static struct i2c_driver sm5703_driver = {
	.driver = {
		.name = "sm5703",
		.of_match_table = sm5703_dt_match,
	},
	.probe = sm5703_i2c_probe,
};
module_i2c_driver(sm5703_driver);

MODULE_DESCRIPTION("Silicon Mitus SM5703 multi-function core driver");
MODULE_AUTHOR("Beomho Seo <beomho.seo@samsung.com>");
MODULE_LICENSE("GPL");
