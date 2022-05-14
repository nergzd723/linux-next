// SPDX-License-Identifier: GPL-2.0-only

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/module.h>

#include <linux/gpio/consumer.h>
#include <linux/iio/iio.h>

#include <linux/regmap.h>

#define TCS3490_ENABLE			0x80
#define TCS3490_SUSPEND			0
#define TCS3490_ADC_ENABLE		BIT(1)
#define TCS3490_POWER_ON_INTERNAL	BIT(0)

#define TCS3490_GAIN_CTRL		0x8f
#define TCS3490_REVID			0x91
#define TCS3490_ID			0x92
#define TCS3490_STATUS			0x93

#define TCS3490_STATUS_RGBC_VALID	BIT(0)
#define TCS3490_STATUS_ALS_SAT		BIT(7)

#define TCS3490_CLEAR_IR_MODE		0xc0
#define TCS3490_MODE_CLEAR		0
#define TCS3490_MODE_IR			BIT(7)

#define TCS3490_CLEAR_IR_BASE		0x94
#define TCS3490_RED_BASE		0x96
#define TCS3490_GREEN_BASE		0x98
#define TCS3490_BLUE_BASE		0x9a

#define TCS3490_GAIN_MASK		0x3

#define TCS3490_LIGHT_CHANNEL(_color, _idx) {		\
	.type = IIO_INTENSITY,				\
	.info_mask_separate = BIT(IIO_CHAN_INFO_RAW),	\
	.info_mask_shared_by_type =			\
			BIT(IIO_CHAN_INFO_CALIBSCALE),	\
	.address = _idx,				\
	.modified = 1,					\
	.channel2 = IIO_MOD_LIGHT_##_color,		\
}							\

struct tcs3490 {
	struct i2c_client *client;
	struct regmap *regmap;
	struct regulator *vdd_supply;
};

const unsigned int tcs3490_gain_multiplier[] = {1, 4, 16, 64};

static const struct regmap_config tcs3490_regmap_config = {
	.reg_bits	= 8,
	.val_bits	= 8,
};

static const struct iio_chan_spec tcs3490_channels[] = {
	TCS3490_LIGHT_CHANNEL(RED, TCS3490_RED_BASE),
	TCS3490_LIGHT_CHANNEL(GREEN, TCS3490_GREEN_BASE),
	TCS3490_LIGHT_CHANNEL(BLUE, TCS3490_BLUE_BASE),
	TCS3490_LIGHT_CHANNEL(CLEAR, TCS3490_CLEAR_IR_BASE),
	TCS3490_LIGHT_CHANNEL(IR, TCS3490_CLEAR_IR_BASE),
};

static int tcs3490_get_gain(struct tcs3490 *data, int *val)
{
	int ret;
	unsigned int gain;

	ret = regmap_read(data->regmap, TCS3490_GAIN_CTRL, &gain);
	if (ret)
		return ret;

	gain &= TCS3490_GAIN_MASK;

	*val = tcs3490_gain_multiplier[gain];
	return 0;
}

static int tcs3490_set_gain(struct tcs3490 *data, unsigned int gain)
{
	int ret, i;

	for (i = 0; i < ARRAY_SIZE(tcs3490_gain_multiplier); i++) {
		if (tcs3490_gain_multiplier[i] == gain)
			break;
	}

	if (i == ARRAY_SIZE(tcs3490_gain_multiplier))
		return -EINVAL;

	ret = regmap_write(data->regmap, TCS3490_GAIN_CTRL, i);
	if (ret)
		return ret;

	return 0;
}

static int tcs3490_read_channel(struct tcs3490 *data,
				const struct iio_chan_spec *chan,
				int *val)
{
	unsigned int tries = 20;
	unsigned int val_l, val_h, status;
	int ret;

	ret = regmap_write(data->regmap, TCS3490_ENABLE,
			   TCS3490_POWER_ON_INTERNAL | TCS3490_ADC_ENABLE);
	if (ret)
		return ret;

	switch (chan->channel2) {
	case IIO_MOD_LIGHT_RED:
	case IIO_MOD_LIGHT_GREEN:
	case IIO_MOD_LIGHT_BLUE:
		break;
	case IIO_MOD_LIGHT_CLEAR:
		ret = regmap_write(data->regmap, TCS3490_CLEAR_IR_MODE, TCS3490_MODE_CLEAR);
		break;
	case IIO_MOD_LIGHT_IR:
		ret = regmap_write(data->regmap, TCS3490_CLEAR_IR_MODE, TCS3490_MODE_IR);
		break;
	default:
		return -EINVAL;
	}

	do {
		usleep_range(3000, 4000);

		ret = regmap_read(data->regmap, TCS3490_STATUS, &status);
		if (ret)
			return ret;
		if (status & TCS3490_STATUS_RGBC_VALID)
			break;
	} while (--tries);

	if (!tries)
		return -ETIMEDOUT;

	ret = regmap_read(data->regmap, chan->address, &val_l);
	if (ret)
		return ret;
	ret = regmap_read(data->regmap, chan->address + 1, &val_h);
	if (ret)
		return ret;

	*val = (val_h << 8) | val_l;

	ret = regmap_write(data->regmap, TCS3490_ENABLE, TCS3490_SUSPEND);
	if (ret)
		return ret;

	return 0;
}

static int tcs3490_read_raw(struct iio_dev *indio_dev,
			    const struct iio_chan_spec *chan,
			    int *val, int *val2, long mask)
{
	struct tcs3490 *data = iio_priv(indio_dev);
	int ret;

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		ret = tcs3490_read_channel(data, chan, val);
		if (ret < 0)
			return ret;

		ret = IIO_VAL_INT;
		break;
	case IIO_CHAN_INFO_CALIBSCALE:
		ret = tcs3490_get_gain(data, val);
		ret = IIO_VAL_INT;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	if (ret < 0)
		return ret;
	return IIO_VAL_INT;
}

static int tcs3490_write_raw(struct iio_dev *indio_dev,
			     struct iio_chan_spec const *chan,
			     int val, int val2, long mask)
{
	struct tcs3490 *data = iio_priv(indio_dev);

	if (mask == IIO_CHAN_INFO_CALIBSCALE)
		return tcs3490_set_gain(data, val);

	return -EINVAL;
}

static const struct iio_info tcs3490_info = {
	.read_raw = tcs3490_read_raw,
	.write_raw = tcs3490_write_raw,
};

static int tcs3490_probe(struct i2c_client *client)
{
	struct tcs3490 *data;
	struct iio_dev *indio_dev;
	int ret;

	indio_dev = devm_iio_device_alloc(&client->dev, sizeof(*data));
	if (!indio_dev)
		return -ENOMEM;

	data = iio_priv(indio_dev);

	data->client = client;

	data->vdd_supply = devm_regulator_get(&client->dev, "vdd");
	if (IS_ERR(data->vdd_supply))
		return dev_err_probe(&client->dev, PTR_ERR(data->vdd_supply),
				     "Unable to get regulators\n");

	data->regmap = devm_regmap_init_i2c(client, &tcs3490_regmap_config);
	if (IS_ERR(data->regmap))
		return dev_err_probe(&client->dev, PTR_ERR(data->regmap),
				     "Failed to register the register map\n");

	ret = regulator_enable(data->vdd_supply);
	if (ret)
		return dev_err_probe(&client->dev, ret,
				     "Unable to enable regulators\n");

	indio_dev->name = "tcs3490";
	indio_dev->info = &tcs3490_info;
	indio_dev->channels = tcs3490_channels;
	indio_dev->num_channels = ARRAY_SIZE(tcs3490_channels);
	indio_dev->modes = INDIO_DIRECT_MODE;

	ret = regmap_write(data->regmap, TCS3490_ENABLE, TCS3490_SUSPEND);
	if (ret)
		return ret;

	return devm_iio_device_register(&client->dev, indio_dev);
}

#ifdef CONFIG_OF
static const struct of_device_id tcs3490_of_match[] = {
	{ .compatible = "ams,tcs3490", },
	{ },
};
MODULE_DEVICE_TABLE(of, tcs3490_of_match);
#endif

static struct i2c_driver tcs3490_driver = {
	.driver = {
		.name = "tcs3490",
		.of_match_table = of_match_ptr(tcs3490_of_match),
	},
	.probe_new = tcs3490_probe,
};

module_i2c_driver(tcs3490_driver);

MODULE_DESCRIPTION("AMS TCS3490 RGBC/IR Light Sensor driver");
MODULE_AUTHOR("Markuss Broks <markuss.broks@gmail.com>");
MODULE_LICENSE("GPL");
