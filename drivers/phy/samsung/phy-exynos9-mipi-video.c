#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/property.h>
#include <linux/of_gpio.h>
#include <linux/power_supply.h>
#include <linux/regmap.h>
#include <linux/platform_device.h>
#include <linux/property.h>

struct regmap_config map_cfg = {
	.reg_bits = 32,
	.val_bits = 32,
	.reg_stride = 4,
};

static int exynos_dsim_probe (struct platform_device *pdev) {
	struct device *dev = &pdev->dev;
        struct regmap *map;
	struct exynos_dsim *dsim;
	void __iomem *mem;
	int ret;
        unsigned int s;

	mem = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(mem))
		return PTR_ERR(mem);

	map = devm_regmap_init_mmio(dev, mem, &map_cfg);
	if (IS_ERR(map))
		return dev_err_probe(dev, PTR_ERR(map), "Failed to init the registers map\n");

	ret = regmap_read(map, 0, &s);
	if (ret)
		return ret;

        dev_err(&pdev->dev, "DSIM version: 0x%x", s);

	// /* Software reset DSIM */
	// ret = regmap_set_bits(map, 4, (1 << 16));
	// if (ret)
	// 	return ret;

        msleep(250);

	/* And out of it */
	ret = regmap_clear_bits(map, 4, (1 << 16));
	if (ret)
		return ret;

	return 0;
}

static const struct of_device_id exynos_dsim_match[] = {
	{ .compatible = "samsung,exynos9-dsim" },
	{}
};
MODULE_DEVICE_TABLE(of, exynos_dsim_match);

static struct platform_driver exynos_dsim_driver = {
	.driver = {
		.name = "exynos-dsim",
		.of_match_table = exynos_dsim_match,
	},
	.probe	=	exynos_dsim_probe,
};

module_platform_driver(exynos_dsim_driver);

MODULE_DESCRIPTION("Samsung Exynos DSIM I2C Controller Driver");
MODULE_AUTHOR("Markuss Broks <markuss.broks@gmail.com>");
MODULE_LICENSE("GPL"); 