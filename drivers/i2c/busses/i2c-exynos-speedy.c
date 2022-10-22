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

/* SPEEDY Register MAP */
#define SPEEDY_CTRL					0x000
#define SPEEDY_FIFO_CTRL				0x004
#define SPEEDY_CMD					0x008
#define SPEEDY_INT_ENABLE				0x00C
#define SPEEDY_INT_STATUS				0x010
#define SPEEDY_FIFO_STATUS				0x030
#define SPEEDY_TX_DATA					0x034
#define SPEEDY_RX_DATA					0x038
#define SPEEDY_PACKET_GAP_TIME				0x044
#define SPEEDY_TIMEOUT_COUNT				0x048
#define SPEEDY_FIFO_DEBUG				0x100
#define SPEEDY_CTRL_STATUS				0x104

/* SPEEDY_CTRL Register bits */
#define SPEEDY_ENABLE					(1 << 0)
#define SPEEDY_TIMEOUT_CMD_DISABLE			(1 << 1)
#define SPEEDY_TIMEOUT_STANDBY_DISABLE			(1 << 2)
#define SPEEDY_TIMEOUT_DATA_DISABLE			(1 << 3)
#define SPEEDY_ALWAYS_PULLUP_EN				(1 << 7)
#define SPEEDY_DATA_WIDTH_8BIT				(0 << 8)
#define SPEEDY_REMOTE_RESET_REQ				(1 << 30)
#define SPEEDY_SW_RST					(1 << 31)

/* SPEEDY_FIFO_CTRL Register bits */
#define SPEEDY_RX_TRIGGER_LEVEL(x)			((x) << 0)
#define SPEEDY_TX_TRIGGER_LEVEL(x)			((x) << 8)
#define SPEEDY_FIFO_DEBUG_INDEX				(0 << 24) // TODO : modify define
#define SPEEDY_FIFO_RESET				(1 << 31)

/* SPEEDY_CMD Register bits */
#define SPEEDY_BURST_LENGTH(x)				((x) << 0)
#define SPEEDY_BURST_FIXED				(0 << 5)
#define SPEEDY_BURST_INCR				(1 << 5)
#define SPEEDY_BURST_EXTENSION				(2 << 5)
#define SPEEDY_ADDRESS(x)				((x & 0xFFF) << 7)
#define SPEEDY_ACCESS_BURST				(0 << 19)
#define SPEEDY_ACCESS_RANDOM				(1 << 19)
#define SPEEDY_DIRECTION_READ				(0 << 20)
#define SPEEDY_DIRECTION_WRITE				(1 << 20)

/* SPEEDY_INT_ENABLE Register bits */
#define SPEEDY_TRANSFER_DONE_EN				(1 << 0)
#define SPEEDY_TIMEOUT_CMD_EN				(1 << 1)
#define SPEEDY_TIMEOUT_STANDBY_EN			(1 << 2)
#define SPEEDY_TIMEOUT_DATA_EN				(1 << 3)
#define SPEEDY_FIFO_TX_ALMOST_EMPTY_EN			(1 << 4)
#define SPEEDY_FIFO_RX_ALMOST_FULL_EN			(1 << 8)
#define SPEEDY_RX_FIFO_INT_TRAILER_EN			(1 << 9)
#define SPEEDY_RX_MODEBIT_ERR_EN			(1 << 16)
#define SPEEDY_RX_GLITCH_ERR_EN				(1 << 17)
#define SPEEDY_RX_ENDBIT_ERR_EN				(1 << 18)
#define SPEEDY_TX_LINE_BUSY_ERR_EN			(1 << 20)
#define SPEEDY_TX_STOPBIT_ERR_EN			(1 << 21)
#define SPEEDY_REMOTE_RESET_REQ_EN			(1 << 31)

/* SPEEDY_INT_STATUS Register bits */
#define SPEEDY_TRANSFER_DONE				(1 << 0)
#define SPEEDY_TIMEOUT_CMD				(1 << 1)
#define SPEEDY_TIMEOUT_STANDBY				(1 << 2)
#define SPEEDY_TIMEOUT_DATA				(1 << 3)
#define SPEEDY_FIFO_TX_ALMOST_EMPTY			(1 << 4)
#define SPEEDY_FIFO_RX_ALMOST_FULL			(1 << 8)
#define SPEEDY_RX_FIFO_INT_TRAILER			(1 << 9)
#define SPEEDY_RX_MODEBIT_ERR				(1 << 16)
#define SPEEDY_RX_GLITCH_ERR				(1 << 17)
#define SPEEDY_RX_ENDBIT_ERR				(1 << 18)
#define SPEEDY_TX_LINE_BUSY_ERR				(1 << 20)
#define SPEEDY_TX_STOPBIT_ERR				(1 << 21)
#define SPEEDY_REMOTE_RESET_REQ_STAT			(1 << 31)

/* SPEEDY_FIFO_STATUS Register bits */
#define SPEEDY_VALID_DATA_CNT				(0 << 0) // TODO : modify define
#define SPEEDY_FIFO_FULL				(1 << 5)
#define SPEEDY_FIFO_EMPTY				(1 << 6)

/* SPEEDY_PACKET_GAP_TIME Register bits */#define SPEEDY_FIFO_TX_ALMOST_EMPTY			(1 << 4)
#define SPEEDY_FIFO_RX_ALMOST_FULL			(1 << 8)	(1 << 0)
#define SPEEDY_FSM_INIT					(1 << 1)
#define SPEEDY_FSM_TX_CMD				(1 << 2)
#define SPEEDY_FSM_STANDBY				(1 << 3)
#define SPEEDY_FSM_DATA					(1 << 4)
#define SPEEDY_FSM_TIMEOUT				(1 << 5)
#define SPEEDY_FSM_TRANS_DONE				(1 << 6)
#define SPEEDY_FSM_IO_RX_STAT_MASK			(3 << 7)
#define SPEEDY_FSM_IO_TX_IDLE				(1 << 9)
#define SPEEDY_FSM_IO_TX_GET_PACKET			(1 << 10)
#define SPEEDY_FSM_IO_TX_PACKET				(1 << 11)
#define SPEEDY_FSM_IO_TX_DONE				(1 << 12)

/* IP_BATCHER Register MAP */
#define IPBATCHER_CON					0x0500
#define IPBATCHER_STATE					0x0504
#define IPBATCHER_INT_EN				0x0508
#define IPBATCHER_FSM_UNEXPEN				0x050C
#define IPBATCHER_FSM_TXEN				0x0510
#define IPBATCHER_FSM_RXFIFO				0x0514
#define IPBATCHER_FSM_CON				0x0518
#define IP_FIFO_STATUS					0x051C
#define IP_INT_STATUS					0x0520
#define IP_INTR_UNEXP_STATE				0x0524
#define IP_INTR_TX_STATE				0x0528
#define IP_INTR_RX_STATE				0x052C
#define BATCHER_OPCODE					0x0600
#define BATCHER_START_PAYLOAD				0x1000
#define BATCHER_END_PAYLOAD				0x1060
#define IPBATCHER_SEMA_REL				0x0200

/* IPBATCHER_CON Register bits */
#define BATCHER_ENABLE					(1 << 0)
#define DEDICATED_BATCHER_APB				(1 << 1)
#define START_BATCHER					(1 << 4)
#define APB_RESP_CPU					(1 << 5)
#define IP_SW_RST					(1 << 6)
#define MP_APBSEMA_SW_RST				(1 << 7)
#define MP_APBSEMA_DISABLE				(1 << 8)
#define BATCHER_SW_RESET				(1 << 31)

/* IPBATCHER_STATE Register bits */
#define BATCHER_OPERATION_COMPLETE			(1 << 0)
#define UNEXPECTED_IP_INTR				(1 << 1)
#define BATCHER_FSM_STATE_IDLE				(1 << 3)
#define BATCHER_FSM_STATE_INIT				(1 << 4)
#define BATCHER_FSM_STATE_GET_SEMAPHORE			(1 << 5)
#define BATCHER_FSM_STATE_CONFIG			(1 << 6)
#define BATCHER_FSM_STATE_WAIT_INT			(1 << 7)
#define BATCHER_FSM_STATE_SW_RESET_IP			(1 << 8)
#define BATCHER_FSM_STATE_INTR_ROUTINE			(1 << 9)
#define BATCHER_FSM_STATE_WRITE_TX_DATA			(1 << 10)
#define BATCHER_FSM_STATE_READ_RX_DATA			(1 << 11)
#define BATCHER_FSM_STATE_STOP_I2C			(1 << 12)
#define BATCHER_FSM_STATE_CLEAN_INTR_STAT		(1 << 13)
#define BATCHER_FSM_STATE_REL_SEMAPHORE			(1 << 14)
#define BATCHER_FSM_STATE_GEN_INT			(1 << 15)
#define BATCHER_FSM_STATE_UNEXPECTED_INT		(1 << 16)
#define MP_APBSEMA_CH_LOCK_STATUS			(1 << 20)
#define MP_APBSEMA_DISABLE_STATUS			(1 << 21)
#define MP_APBSEMA_SW_RST_STATUS			(1 << 22)

/* IPBATCHER_INT_EN Register bits */
#define BATCHER_INTERRUPT_ENABLE			(1 << 0)

/* IPBATCHER_FSM_CON Register bits */
#define DISABLE_STOP_CMD				(1 << 0)
#define DISABLE_SEMAPHORE_RELEASE			(1 << 1)

#define EXYNOS_SPEEDY_TIMEOUT				(msecs_to_jiffies(500))
#define BATCHER_INIT_CMD				0xFFFFFFFF

struct exynos_speedy {
	struct platform_device *pdev;
	struct regmap *map;
	struct i2c_adapter	adap;
};

struct regmap_config map_cfg = {
	.reg_bits = 32,
	.val_bits = 32,
	.reg_stride = 4,
};

static void exynos_speedy_dump(struct exynos_speedy *speedy) {
	uint32_t ctrl, fifo_ctrl, cmd, int_enable, int_status, fifo_status, gap_time, timeout_count, ctrl_status;
	for (int i = 0; i < 30; i++){
		regmap_read(speedy->map, SPEEDY_RX_DATA, &ctrl_status);
		dev_err(&speedy->pdev->dev, "RX             0x%08x i: %d\n", ctrl_status, i);
	}
}

static int exynos_speedy_batcher_prepare(struct exynos_speedy *speedy)
{
	//regmap_set_bits(speedy->map, IPBATCHER_CON, BATCHER_ENABLE | DEDICATED_BATCHER_APB);
	return 0;
}

static int exynos_speedy_xfer_batcher(struct exynos_speedy *speedy,
				struct i2c_msg *msgs)
{
	int i = 0;
	int ret;
	unsigned int ctl_reset = 0x30050;
	exynos_speedy_batcher_prepare(speedy);
	return ret;
}

static int exynos_speedy_xfer(struct i2c_adapter *adap,
			  struct i2c_msg *msgs, int num)
{
	struct exynos_speedy *speedy = (struct exynos_speedy *)adap->algo_data;
	struct i2c_msg *msgs_ptr = msgs;

	int retry, i = 0;
	int ret = 0;

	return 0;
	for (retry = 0; retry < adap->retries; retry++) {
		for (i = 0; i < num; i++) {
			ret = exynos_speedy_xfer_batcher(speedy, msgs_ptr);

			msgs_ptr++;

			if (ret == -EAGAIN) {
				msgs_ptr = msgs;
				break;
			} else if (ret < 0) {
				goto out;
			}
		}

		if ((i == num) && (ret != -EAGAIN))
			break;

		udelay(100);
	}

	if (i == num)
		ret = num;
	else
		ret = -EREMOTEIO;

 out:
	return ret;
}

static unsigned int exynos_speedy_func(struct i2c_adapter *adap)
{
        return I2C_FUNC_I2C | (I2C_FUNC_SMBUS_EMUL & ~I2C_FUNC_SMBUS_QUICK);
}

static const struct i2c_algorithm exynos_speedy_algorithm = {
	.master_xfer		= exynos_speedy_xfer,
        .functionality          = exynos_speedy_func,
};

static int exynos_speedy_probe (struct platform_device *pdev) {
	struct device *dev = &pdev->dev;
	struct exynos_speedy *speedy;
	void __iomem *mem;
	int ret;

	speedy = devm_kzalloc(dev, sizeof(*speedy), GFP_KERNEL);
	if (!speedy)
		return -ENOMEM;

	speedy->pdev = pdev;

	mem = devm_platform_ioremap_resource(pdev, 0);
	if (IS_ERR(mem))
		return PTR_ERR(mem);

	speedy->map = devm_regmap_init_mmio(dev, mem, &map_cfg);
	if (IS_ERR(speedy->map))
		return dev_err_probe(dev, PTR_ERR(speedy->map), "Failed to init the registers map\n");

	exynos_speedy_dump(speedy);

	/* Software reset SPEEDY */
	ret = regmap_set_bits(speedy->map, SPEEDY_CTRL, SPEEDY_SW_RST);
	if (ret)
		return ret;

	exynos_speedy_dump(speedy);


	strlcpy(speedy->adap.name, "exynos-speedy", sizeof(speedy->adap.name));
	speedy->adap.owner   = THIS_MODULE;
	speedy->adap.algo    = &exynos_speedy_algorithm;
	speedy->adap.retries = 3;
	speedy->adap.dev.of_node = pdev->dev.of_node;
	speedy->adap.algo_data = speedy;
	speedy->adap.dev.parent = dev;

	ret = i2c_add_adapter(&speedy->adap);
	if (ret)
		return dev_err_probe(dev, ret, "failed to register bus\n");

	return 0;
}

static const struct of_device_id exynos_speedy_match[] = {
	{ .compatible = "samsung,exynos-speedy" },
	{}
};
MODULE_DEVICE_TABLE(of, exynos_speedy_match);

static struct platform_driver exynos_speedy_driver = {
	.driver = {
		.name = "exynos-speedy",
		.of_match_table = exynos_speedy_match,
	},
	.probe	=	exynos_speedy_probe,
};

module_platform_driver(exynos_speedy_driver);

MODULE_DESCRIPTION("Samsung Exynos SPEEDY I2C Controller Driver");
MODULE_AUTHOR("Markuss Broks <markuss.broks@gmail.com>");
MODULE_LICENSE("GPL"); 