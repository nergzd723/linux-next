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

#include "i2c-exynos-acpm.h"

struct exynos_acpm {
	struct platform_device *pdev;
	void __iomem *sram;
	void __iomem *ap2apm;
	struct i2c_adapter	adap;
};

static unsigned int exynos_acpm_func(struct i2c_adapter *adap)
{
        return I2C_FUNC_I2C | (I2C_FUNC_SMBUS_EMUL & ~I2C_FUNC_SMBUS_QUICK);
}

static const struct i2c_algorithm exynos_acpm_algorithm = {
        .functionality          = exynos_acpm_func,
};

static int exynos_mailbox_send (struct exynos_acpm *acpm, void* data) 
{
	struct acpm_framework *initdata;
	struct ipc_channel *ipc_channels;

	int counter = 0;

	unsigned int command[4] = {0, };
	unsigned int i2c_reg = 0;
	unsigned int i2c_addr = 0;
	unsigned int front, rear, tmp_index;

	command[0] = ((i2c_reg & 0xf) << 8) | (i2c_addr & 0xff);
	command[1] = 0;

        initdata = acpm->sram + 0x7F00;
        ipc_channels = acpm->sram + initdata->ipc_channels;

	front = __raw_readl(acpm->sram + ipc_channels[4].ch.tx_front);
	rear = __raw_readl(acpm->sram + ipc_channels[4].ch.tx_rear);

	tmp_index = front + 1;
	
	dev_err(&acpm->pdev->dev, "front: %x, rear: %x", front, rear);

	command[0] |= (1 << 16);

	memcpy(acpm->sram + ipc_channels[4].ch.tx_base, command, 32);

	writel(1, acpm->sram + ipc_channels[4].ch.tx_front);

	writel(1 << 4, acpm->ap2apm + 0x10);
	writel(1 << 4, acpm->ap2apm + 8);
	
	int intmsc1 = __raw_readl(acpm->ap2apm + 0x18);
	dev_err(&acpm->pdev->dev, "intmsr0: %x", intmsc1);

	intmsc1 = __raw_readl(acpm->ap2apm + 0x18);
	dev_err(&acpm->pdev->dev, "intmsr0: %x", intmsc1);

	return 0;
}

static int exynos_acpm_probe (struct platform_device *pdev) {
	struct device *dev = &pdev->dev;
	struct exynos_acpm *acpm;
	struct acpm_framework *initdata;
	struct ipc_channel *ipc_channels;
	int ret;

	acpm = devm_kzalloc(dev, sizeof(*acpm), GFP_KERNEL);
	if (!acpm)
		return -ENOMEM;

	acpm->pdev = pdev;

	acpm->ap2apm = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(acpm->ap2apm))
		return PTR_ERR(acpm->ap2apm);

	acpm->sram = devm_platform_ioremap_resource(pdev, 1);
	if (IS_ERR(acpm->sram))
		return PTR_ERR(acpm->sram);

        initdata = acpm->sram + 0x7F00;

        ipc_channels = acpm->sram + initdata->ipc_channels;

	dev_err(dev, "Channel 4 ID: %d, rx_ch base: 0x%x, tx_ch base: 0x%x", ipc_channels[4].id, ipc_channels[4].ch.rx_base, ipc_channels[4].ch.tx_base);

	strlcpy(acpm->adap.name, "exynos-acpm", sizeof(acpm->adap.name));
	acpm->adap.owner   = THIS_MODULE;
	acpm->adap.algo    = &exynos_acpm_algorithm;
	acpm->adap.retries = 3;
	acpm->adap.dev.of_node = pdev->dev.of_node;
	acpm->adap.algo_data = acpm;
	acpm->adap.dev.parent = dev;

	ret = i2c_add_adapter(&acpm->adap);
	if (ret)
		return dev_err_probe(dev, ret, "failed to register bus\n");

	exynos_mailbox_send(acpm, NULL);

	return 0;
}

static const struct of_device_id exynos_acpm_match[] = {
	{ .compatible = "samsung,exynos-acpm-ipc" },
	{}
};
MODULE_DEVICE_TABLE(of, exynos_acpm_match);

static struct platform_driver exynos_acpm_driver = {
	.driver = {
		.name = "exynos-acpm",
		.of_match_table = exynos_acpm_match,
	},
	.probe	=	exynos_acpm_probe,
};

module_platform_driver(exynos_acpm_driver);

MODULE_DESCRIPTION("Samsung Exynos ACPM I2C Controller Driver");
MODULE_AUTHOR("Markuss Broks <markuss.broks@gmail.com>");
MODULE_LICENSE("GPL"); 


/*
int acpm_ipc_send_data(unsigned int channel_id, struct ipc_config *cfg)
{
	unsigned int front;
	unsigned int rear;
	unsigned int tmp_index;
	struct acpm_ipc_ch *channel;
	bool timeout_flag = 0;
	int ret;
	u64 timeout, now;
	u32 retry_cnt = 0;

	channel = &acpm_ipc->channel[channel_id];

	spin_lock(&channel->tx_lock);

	front = __raw_readl(channel->tx_ch.front);
	rear = __raw_readl(channel->tx_ch.rear);

	tmp_index = front + 1;

	if (tmp_index >= channel->tx_ch.len)
		tmp_index = 0;

	if (++channel->seq_num == 64)
		channel->seq_num = 1;

	cfg->cmd[0] |= (channel->seq_num & 0x3f) << ACPM_IPC_PROTOCOL_SEQ_NUM;

	memcpy_align_4(channel->tx_ch.base + channel->tx_ch.size * front, cfg->cmd,
			channel->tx_ch.size);

	cfg->cmd[1] = 0;
	cfg->cmd[2] = 0;
	cfg->cmd[3] = 0;

	writel(tmp_index, channel->tx_ch.front);

	apm_interrupt_gen(channel->id);
	spin_unlock(&channel->tx_lock);

	while (!(__raw_readl(acpm_ipc->intr + INTSR1) & (1 << channel->id)));
	return 0;
}
*/
