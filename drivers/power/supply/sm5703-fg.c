#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/property.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
#include <linux/of_gpio.h>
#include <linux/power_supply.h>

#define SM5703_REG_DEVICE_ID                 0x00
#define SM5703_REG_CNTL                      0x01
#define SM5703_REG_INTFG                     0x02
#define SM5703_REG_INTFG_MASK                0x03
#define SM5703_REG_STATUS                    0x04
#define SM5703_REG_SOC                       0x05
#define SM5703_REG_OCV                       0x06
#define SM5703_REG_VOLTAGE                   0x07
#define SM5703_REG_CURRENT                   0x08
#define SM5703_REG_TEMPERATURE               0x09
#define SM5703_REG_CURRENT_EST      0x85
#define SM5703_REG_FG_OP_STATUS              0x10

static enum power_supply_property sm5703_fg_props[] = {
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
};

unsigned short sm5703_fg_readw(struct i2c_client* i2c, unsigned char reg) {
    unsigned char result[2];
    int ret;
    ret = i2c_smbus_read_i2c_block_data(i2c, reg, 2, result);
    if (ret < 0) return ret;
    return *(unsigned short*)result;
}

static int sm5703_fg_get_property(struct power_supply *psy,
				   enum power_supply_property psp,
				   union power_supply_propval *val) 
{
    unsigned short ret;
    struct i2c_client *i2c;

    i2c = power_supply_get_drvdata(psy);
    switch(psp) {
        case POWER_SUPPLY_PROP_TEMP:
            ret = sm5703_fg_readw(i2c, SM5703_REG_TEMPERATURE);
            // Convert to millicelcius
            ret = (((ret & 0x7f00) >> 8) * 10) + (((ret & 0xff) * 10) / 256);
            val->intval = ret;
            break;
        case POWER_SUPPLY_PROP_CAPACITY:
            ret = sm5703_fg_readw(i2c, SM5703_REG_SOC);
            // Convert to %
            ret = ((((ret & 0xff00) >> 8) * 10) + (((ret & 0xff) * 10) / 256)) / 10;
            val->intval = ret;
            break;
        case POWER_SUPPLY_PROP_VOLTAGE_NOW:
            ret = sm5703_fg_readw(i2c, SM5703_REG_OCV);
            // Convert to mV
            ret = (((ret & 0x700) >> 8) * 1000) + (((ret & 0xff) * 1000) / 256);
            val->intval = ret;
            break;
        case POWER_SUPPLY_PROP_CURRENT_NOW:
            ret = sm5703_fg_readw(i2c, SM5703_REG_FG_OP_STATUS);
            // Convert to uA
            ret = (((ret & 0x700) >> 8) * 1000) + (((ret & 0xff)*1000)/256);
            val->intval = ret;
            break;
        default:
            return -EINVAL;
    }
    return 0;
}

static const struct power_supply_desc sm5703_fg_desc = {
	.name			= "sm5703_fg",
	.type			= POWER_SUPPLY_TYPE_BATTERY,
	.properties		= sm5703_fg_props,
	.num_properties		= ARRAY_SIZE(sm5703_fg_props),
	.get_property		= sm5703_fg_get_property,
};

static int sm5703_fg_probe (struct i2c_client* i2c, const struct i2c_device_id* id) {
    dev_info(&i2c->dev, "probe\n");
    struct power_supply_config fg_cfg = { };
    int ret;

    fg_cfg.drv_data = i2c;
	fg_cfg.of_node = i2c->dev.of_node;
	struct power_supply* supply = devm_power_supply_register(&i2c->dev, &sm5703_fg_desc,
						   &fg_cfg);

	if (IS_ERR(supply)) {
		dev_err(&i2c->dev, "failed to register power supply\n");
		return PTR_ERR(supply);
	}
	return 0;
}

static int sm5703_fg_remove (struct i2c_client* i2c) {
    printk(KERN_INFO "remove\n");
	return 0;
}

static const struct i2c_device_id sm5703_fg_id[] = {
	{"sm5703-fg", 0},
	{}
};

static struct of_device_id sm5703_i2c_match_table[] = {
	{ .compatible = "siliconmitus,sm5703-fg", },
	{ },
};
MODULE_DEVICE_TABLE(i2c, sm5703_i2c_match_table);

static struct i2c_driver sm5703_fg_driver = {
	.driver = {
		   .name = "sm5703-fg",
		   .of_match_table = sm5703_i2c_match_table,
	},
	.probe	= sm5703_fg_probe,
	.remove	= sm5703_fg_remove,
	.id_table   = sm5703_fg_id,
};

static int __init sm5703_fg_init(void)
{
	printk(KERN_INFO "Hello, world!\n");

	return i2c_add_driver(&sm5703_fg_driver);
}

static void __exit sm5703_fg_exit(void)
{
	i2c_del_driver(&sm5703_fg_driver);
}

module_init(sm5703_fg_init);
module_exit(sm5703_fg_exit);

MODULE_DESCRIPTION("Samsung SM5703");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");