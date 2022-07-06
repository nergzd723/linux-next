// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2022 Samsung Electronics Co., Ltd.
 * Author: Chanho Park <chanho61.park@samsung.com>
 *
 * Common Clock Framework support for ExynosAuto V9 SoC.
 */

#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>

#include <dt-bindings/clock/samsung,exynos9810.h>

#include "clk.h"
#include "clk-exynos-arm64.h"

/* ---- CMU_TOP ------------------------------------------------------------ */

/* Register Offset definitions for CMU_TOP (0x1a240000) */

#define PLL_LOCKTIME_PLL_MMC  0x0000
#define PLL_LOCKTIME_PLL_SHARED0  0x0004
#define PLL_LOCKTIME_PLL_SHARED1  0x0008
#define PLL_LOCKTIME_PLL_SHARED2  0x000c
#define PLL_LOCKTIME_PLL_SHARED3  0x0010
#define PLL_LOCKTIME_PLL_SHARED4  0x0014
#define PLL_CON0_PLL_MMC  0x0100
#define PLL_CON3_PLL_MMC  0x010c
#define PLL_CON0_PLL_SHARED0  0x0120
#define PLL_CON0_PLL_SHARED1  0x01a0
#define PLL_CON0_PLL_SHARED2  0x0220
#define PLL_CON0_PLL_SHARED3  0x02a0
#define PLL_CON0_PLL_SHARED4  0x0320

/* MUX */
#define CLKOUT_CON_BLK_CMU_CMU_CMU_CLKOUT0  0x0810
#define CLKOUT_CON_BLK_CMU_CMU_CMU_CLKOUT1  0x0814
#define CLK_CON_MUX_CLKCMU_DPU_BUS  0x1000
#define CLK_CON_MUX_CLKCMU_APM_BUS  0x1004
#define CLK_CON_MUX_CLKCMU_AUD_CPU  0x1008
#define CLK_CON_MUX_CLKCMU_BUS1_BUS  0x100c
#define CLK_CON_MUX_CLKCMU_BUSC_BUS  0x1010
#define CLK_CON_MUX_CLKCMU_CHUB_BUS  0x1014
#define CLK_CON_MUX_CLKCMU_CIS_CLK0  0x1018
#define CLK_CON_MUX_CLKCMU_CIS_CLK1  0x101c
#define CLK_CON_MUX_CLKCMU_CIS_CLK2  0x1020
#define CLK_CON_MUX_CLKCMU_CIS_CLK3  0x1024
#define CLK_CON_MUX_CLKCMU_CMGP_BUS  0x1028
#define CLK_CON_MUX_CLKCMU_CORE_BUS  0x102c
#define CLK_CON_MUX_CLKCMU_CPUCL0_DBG_BUS  0x1030
#define CLK_CON_MUX_CLKCMU_CPUCL0_SWITCH  0x1034
#define CLK_CON_MUX_CLKCMU_CPUCL1_SWITCH  0x1038
#define CLK_CON_MUX_CLKCMU_DCF_BUS  0x103c
#define CLK_CON_MUX_CLKCMU_DCPOST_BUS  0x1040
#define CLK_CON_MUX_CLKCMU_DCRD_BUS  0x1044
#define CLK_CON_MUX_CLKCMU_DPU_BUS  0x1048
#define CLK_CON_MUX_CLKCMU_DSPM_BUS  0x104c
#define CLK_CON_MUX_CLKCMU_DSPS_AUD  0x1050
#define CLK_CON_MUX_CLKCMU_FSYS0_BUS  0x1054
#define CLK_CON_MUX_CLKCMU_FSYS0_DPGTC  0x1058
#define CLK_CON_MUX_CLKCMU_FSYS0_UFS_EMBD  0x105c
#define CLK_CON_MUX_CLKCMU_FSYS0_USB30DRD  0x1060
#define CLK_CON_MUX_CLKCMU_FSYS0_USBDP_DEBUG  0x1064
#define CLK_CON_MUX_CLKCMU_FSYS1_BUS  0x1068
#define CLK_CON_MUX_CLKCMU_FSYS1_MMC_CARD  0x106c
#define CLK_CON_MUX_CLKCMU_FSYS1_PCIE  0x1070
#define CLK_CON_MUX_CLKCMU_FSYS1_UFS_CARD  0x1074
#define CLK_CON_MUX_CLKCMU_G2D_G2D  0x1078
#define CLK_CON_MUX_CLKCMU_G2D_MSCL  0x107c
#define CLK_CON_MUX_CLKCMU_HPM  0x1080
#define CLK_CON_MUX_CLKCMU_ISPHQ_BUS  0x1084
#define CLK_CON_MUX_CLKCMU_ISPLP_BUS  0x1088
#define CLK_CON_MUX_CLKCMU_ISPLP_GDC  0x108c
#define CLK_CON_MUX_CLKCMU_ISPLP_VRA  0x1090
#define CLK_CON_MUX_CLKCMU_ISPPRE_BUS  0x1094
#define CLK_CON_MUX_CLKCMU_IVA_BUS  0x1098
#define CLK_CON_MUX_CLKCMU_MFC_BUS  0x109c
#define CLK_CON_MUX_CLKCMU_MFC_WFD  0x10a0
#define CLK_CON_MUX_CLKCMU_MIF_BUSP  0x10a4
#define CLK_CON_MUX_CLKCMU_MIF_SWITCH  0x10a8
#define CLK_CON_MUX_CLKCMU_PERIC0_BUS  0x10ac
#define CLK_CON_MUX_CLKCMU_PERIC0_IP  0x10b0
#define CLK_CON_MUX_CLKCMU_PERIC1_BUS  0x10b4
#define CLK_CON_MUX_CLKCMU_PERIC1_IP  0x10b8
#define CLK_CON_MUX_CLKCMU_PERIS_BUS  0x10bc
#define CLK_CON_MUX_CLKCMU_VTS_BUS  0x10c0
#define CLK_CON_MUX_CLK_CMU_CMUREF  0x10c4
#define CLK_CON_MUX_CMU_CMUREF  0x10c8

/* DIV */
#define CLK_CON_DIV_CLKCMU_APM_BUS  0x1800
#define CLK_CON_DIV_CLKCMU_AUD_CPU  0x1804
#define CLK_CON_DIV_CLKCMU_BUS1_BUS  0x1808
#define CLK_CON_DIV_CLKCMU_BUSC_BUS  0x180c
#define CLK_CON_DIV_CLKCMU_CHUB_BUS  0x1810
#define CLK_CON_DIV_CLKCMU_CIS_CLK0  0x1814
#define CLK_CON_DIV_CLKCMU_CIS_CLK1  0x1818
#define CLK_CON_DIV_CLKCMU_CIS_CLK2  0x181c
#define CLK_CON_DIV_CLKCMU_CIS_CLK3  0x1820
#define CLK_CON_DIV_CLKCMU_CMGP_BUS  0x1824
#define CLK_CON_DIV_CLKCMU_CORE_BUS  0x1828
#define CLK_CON_DIV_CLKCMU_CPUCL0_DBG_BUS  0x182c
#define CLK_CON_DIV_CLKCMU_CPUCL0_SWITCH  0x1830
#define CLK_CON_DIV_CLKCMU_CPUCL1_SWITCH  0x1834
#define CLK_CON_DIV_CLKCMU_DCF_BUS  0x1838
#define CLK_CON_DIV_CLKCMU_DCPOST_BUS  0x183c
#define CLK_CON_DIV_CLKCMU_DCRD_BUS  0x1840
#define CLK_CON_DIV_CLKCMU_DSPM_BUS  0x1844
#define CLK_CON_DIV_CLKCMU_DSPS_AUD  0x1848
#define CLK_CON_DIV_CLKCMU_FSYS0_BUS  0x184c
#define CLK_CON_DIV_CLKCMU_FSYS0_DPGTC  0x1850
#define CLK_CON_DIV_CLKCMU_FSYS0_UFS_EMBD  0x1854
#define CLK_CON_DIV_CLKCMU_FSYS0_USB30DRD  0x1858
#define CLK_CON_DIV_CLKCMU_FSYS0_USBDP_DEBUG  0x185c
#define CLK_CON_DIV_CLKCMU_FSYS1_BUS  0x1860
#define CLK_CON_DIV_CLKCMU_FSYS1_MMC_CARD  0x1864
#define CLK_CON_DIV_CLKCMU_FSYS1_PCIE  0x1868
#define CLK_CON_DIV_CLKCMU_FSYS1_UFS_CARD  0x186c
#define CLK_CON_DIV_CLKCMU_G2D_G2D  0x1870
#define CLK_CON_DIV_CLKCMU_G2D_MSCL  0x1874
#define CLK_CON_DIV_CLKCMU_G3D_SWITCH  0x1878
#define CLK_CON_DIV_CLKCMU_HPM  0x187c
#define CLK_CON_DIV_CLKCMU_ISPHQ_BUS  0x1880
#define CLK_CON_DIV_CLKCMU_ISPLP_BUS  0x1884
#define CLK_CON_DIV_CLKCMU_ISPLP_GDC  0x1888
#define CLK_CON_DIV_CLKCMU_ISPLP_VRA  0x188c
#define CLK_CON_DIV_CLKCMU_ISPPRE_BUS  0x1890
#define CLK_CON_DIV_CLKCMU_IVA_BUS  0x1894
#define CLK_CON_DIV_CLKCMU_MFC_BUS  0x1898
#define CLK_CON_DIV_CLKCMU_MFC_WFD  0x189c
#define CLK_CON_DIV_CLKCMU_MIF_BUSP  0x18a0
#define CLK_CON_DIV_CLKCMU_MODEM_SHARED0  0x18a4
#define CLK_CON_DIV_CLKCMU_MODEM_SHARED1  0x18a8
#define CLK_CON_DIV_CLKCMU_OTP  0x18ac
#define CLK_CON_DIV_CLKCMU_PERIC0_BUS  0x18b0
#define CLK_CON_DIV_CLKCMU_PERIC0_IP  0x18b4
#define CLK_CON_DIV_CLKCMU_PERIC1_BUS  0x18b8
#define CLK_CON_DIV_CLKCMU_PERIC1_IP  0x18bc
#define CLK_CON_DIV_CLKCMU_PERIS_BUS  0x18c0
#define CLK_CON_DIV_CLKCMU_VTS_BUS  0x18c4
#define CLK_CON_DIV_CLKCMU_DPU  0x18c8
#define CLK_CON_DIV_CLKCMU_DPU_BUS  0x18cc
#define CLK_CON_DIV_CLK_CMU_CMUREF  0x18d0
#define CLK_CON_DIV_PLL_SHARED0_DIV2  0x18d4
#define CLK_CON_DIV_PLL_SHARED0_DIV3  0x18d8
#define CLK_CON_DIV_PLL_SHARED0_DIV4  0x18dc
#define CLK_CON_DIV_PLL_SHARED1_DIV2  0x18e0
#define CLK_CON_DIV_PLL_SHARED1_DIV3  0x18e4
#define CLK_CON_DIV_PLL_SHARED1_DIV4  0x18e8
#define CLK_CON_DIV_PLL_SHARED2_DIV2  0x18ec
#define CLK_CON_DIV_PLL_SHARED3_DIV2  0x18f0
#define CLK_CON_DIV_PLL_SHARED4_DIV2  0x18f4

/* GATE */
#define CLK_CON_GATE_CLKCMU_MIF_SWITCH  0x2000
#define CLK_CON_GATE_CLKCMU_APM_BUS  0x2004
#define CLK_CON_GATE_CLKCMU_AUD_CPU  0x2008
#define CLK_CON_GATE_CLKCMU_BUS1_BUS  0x200c
#define CLK_CON_GATE_CLKCMU_BUSC_BUS  0x2010
#define CLK_CON_GATE_CLKCMU_CHUB_BUS  0x2014
#define CLK_CON_GATE_CLKCMU_CIS_CLK0  0x2018
#define CLK_CON_GATE_CLKCMU_CIS_CLK1  0x201c
#define CLK_CON_GATE_CLKCMU_CIS_CLK2  0x2020
#define CLK_CON_GATE_CLKCMU_CIS_CLK3  0x2024
#define CLK_CON_GATE_CLKCMU_CMGP_BUS  0x2028
#define CLK_CON_GATE_CLKCMU_CORE_BUS  0x202c
#define CLK_CON_GATE_CLKCMU_CPUCL0_DBG_BUS  0x2030
#define CLK_CON_GATE_CLKCMU_CPUCL0_SWITCH  0x2034
#define CLK_CON_GATE_CLKCMU_CPUCL1_SWITCH  0x2038
#define CLK_CON_GATE_CLKCMU_DCF_BUS  0x203c
#define CLK_CON_GATE_CLKCMU_DCPOST_BUS  0x2040
#define CLK_CON_GATE_CLKCMU_DCRD_BUS  0x2044
#define CLK_CON_GATE_CLKCMU_DPU  0x2048
#define CLK_CON_GATE_CLKCMU_DPU_BUS  0x204c
#define CLK_CON_GATE_CLKCMU_DSPM_BUS  0x2050
#define CLK_CON_GATE_CLKCMU_DSPS_AUD  0x2054
#define CLK_CON_GATE_CLKCMU_FSYS0_BUS  0x2058
#define CLK_CON_GATE_CLKCMU_FSYS0_DPGTC  0x205c
#define CLK_CON_GATE_CLKCMU_FSYS0_UFS_EMBD  0x2060
#define CLK_CON_GATE_CLKCMU_FSYS0_USB30DRD  0x2064
#define CLK_CON_GATE_CLKCMU_FSYS0_USBDP_DEBUG  0x2068
#define CLK_CON_GATE_CLKCMU_FSYS1_BUS  0x206c
#define CLK_CON_GATE_CLKCMU_FSYS1_MMC_CARD  0x2070
#define CLK_CON_GATE_CLKCMU_FSYS1_PCIE  0x2074
#define CLK_CON_GATE_CLKCMU_FSYS1_UFS_CARD  0x2078
#define CLK_CON_GATE_CLKCMU_G2D_G2D  0x207c
#define CLK_CON_GATE_CLKCMU_G2D_MSCL  0x2080
#define CLK_CON_GATE_CLKCMU_G3D_SWITCH  0x2084
#define CLK_CON_GATE_CLKCMU_HPM  0x2088
#define CLK_CON_GATE_CLKCMU_ISPHQ_BUS  0x208c
#define CLK_CON_GATE_CLKCMU_ISPLP_BUS  0x2090
#define CLK_CON_GATE_CLKCMU_ISPLP_GDC  0x2094
#define CLK_CON_GATE_CLKCMU_ISPLP_VRA  0x2098
#define CLK_CON_GATE_CLKCMU_ISPPRE_BUS  0x209c
#define CLK_CON_GATE_CLKCMU_IVA_BUS  0x20a0
#define CLK_CON_GATE_CLKCMU_MFC_BUS  0x20a4
#define CLK_CON_GATE_CLKCMU_MFC_WFD  0x20a8
#define CLK_CON_GATE_CLKCMU_MIF_BUSP  0x20ac
#define CLK_CON_GATE_CLKCMU_MODEM_SHARED0  0x20b0
#define CLK_CON_GATE_CLKCMU_MODEM_SHARED1  0x20b4
#define CLK_CON_GATE_CLKCMU_PERIC0_BUS  0x20b8
#define CLK_CON_GATE_CLKCMU_PERIC0_IP  0x20bc
#define CLK_CON_GATE_CLKCMU_PERIC1_BUS  0x20c0
#define CLK_CON_GATE_CLKCMU_PERIC1_IP  0x20c4
#define CLK_CON_GATE_CLKCMU_PERIS_BUS  0x20c8
#define CLK_CON_GATE_CLKCMU_VTS_BUS  0x20cc

static const unsigned long top_clk_regs[] __initconst = {
	PLL_LOCKTIME_PLL_MMC,
	PLL_LOCKTIME_PLL_SHARED0,
	PLL_LOCKTIME_PLL_SHARED1,
	PLL_LOCKTIME_PLL_SHARED2,
	PLL_LOCKTIME_PLL_SHARED3,
	PLL_LOCKTIME_PLL_SHARED4,
	PLL_CON0_PLL_MMC,
	PLL_CON3_PLL_MMC,
	PLL_CON0_PLL_SHARED0,
	PLL_CON0_PLL_SHARED1,
	PLL_CON0_PLL_SHARED2,
	PLL_CON0_PLL_SHARED3,
	PLL_CON0_PLL_SHARED4,
	CLK_CON_MUX_CLKCMU_DPU_BUS,
	CLK_CON_MUX_CLKCMU_APM_BUS,
	CLK_CON_MUX_CLKCMU_AUD_CPU,
	CLK_CON_MUX_CLKCMU_BUS1_BUS,
	CLK_CON_MUX_CLKCMU_BUSC_BUS,
	CLK_CON_MUX_CLKCMU_CHUB_BUS,
	CLK_CON_MUX_CLKCMU_CIS_CLK0,
	CLK_CON_MUX_CLKCMU_CIS_CLK1,
	CLK_CON_MUX_CLKCMU_CIS_CLK2,
	CLK_CON_MUX_CLKCMU_CIS_CLK3,
	CLK_CON_MUX_CLKCMU_CMGP_BUS,
	CLK_CON_MUX_CLKCMU_CORE_BUS,
	CLK_CON_MUX_CLKCMU_CPUCL0_DBG_BUS,
	CLK_CON_MUX_CLKCMU_CPUCL0_SWITCH,
	CLK_CON_MUX_CLKCMU_CPUCL1_SWITCH,
	CLK_CON_MUX_CLKCMU_DCF_BUS,
	CLK_CON_MUX_CLKCMU_DCPOST_BUS,
	CLK_CON_MUX_CLKCMU_DCRD_BUS,
	CLK_CON_MUX_CLKCMU_DPU_BUS,
	CLK_CON_MUX_CLKCMU_DSPM_BUS,
	CLK_CON_MUX_CLKCMU_DSPS_AUD,
	CLK_CON_MUX_CLKCMU_FSYS0_BUS,
	CLK_CON_MUX_CLKCMU_FSYS0_DPGTC,
	CLK_CON_MUX_CLKCMU_FSYS0_UFS_EMBD,
	CLK_CON_MUX_CLKCMU_FSYS0_USB30DRD,
	CLK_CON_MUX_CLKCMU_FSYS0_USBDP_DEBUG,
	CLK_CON_MUX_CLKCMU_FSYS1_BUS,
	CLK_CON_MUX_CLKCMU_FSYS1_MMC_CARD,
	CLK_CON_MUX_CLKCMU_FSYS1_PCIE,
	CLK_CON_MUX_CLKCMU_FSYS1_UFS_CARD,
	CLK_CON_MUX_CLKCMU_G2D_G2D,
	CLK_CON_MUX_CLKCMU_G2D_MSCL,
	CLK_CON_MUX_CLKCMU_HPM,
	CLK_CON_MUX_CLKCMU_ISPHQ_BUS,
	CLK_CON_MUX_CLKCMU_ISPLP_BUS,
	CLK_CON_MUX_CLKCMU_ISPLP_GDC,
	CLK_CON_MUX_CLKCMU_ISPLP_VRA,
	CLK_CON_MUX_CLKCMU_ISPPRE_BUS,
	CLK_CON_MUX_CLKCMU_IVA_BUS,
	CLK_CON_MUX_CLKCMU_MFC_BUS,
	CLK_CON_MUX_CLKCMU_MFC_WFD,
	CLK_CON_MUX_CLKCMU_MIF_BUSP,
	CLK_CON_MUX_CLKCMU_MIF_SWITCH,
	CLK_CON_MUX_CLKCMU_PERIC0_BUS,
	CLK_CON_MUX_CLKCMU_PERIC0_IP,
	CLK_CON_MUX_CLKCMU_PERIC1_BUS,
	CLK_CON_MUX_CLKCMU_PERIC1_IP,
	CLK_CON_MUX_CLKCMU_PERIS_BUS,
	CLK_CON_MUX_CLKCMU_VTS_BUS,
	CLK_CON_MUX_CLK_CMU_CMUREF,
	CLK_CON_MUX_CMU_CMUREF,
	CLK_CON_DIV_CLKCMU_APM_BUS,
	CLK_CON_DIV_CLKCMU_AUD_CPU,
	CLK_CON_DIV_CLKCMU_BUS1_BUS,
	CLK_CON_DIV_CLKCMU_BUSC_BUS,
	CLK_CON_DIV_CLKCMU_CHUB_BUS,
	CLK_CON_DIV_CLKCMU_CIS_CLK0,
	CLK_CON_DIV_CLKCMU_CIS_CLK1,
	CLK_CON_DIV_CLKCMU_CIS_CLK2,
	CLK_CON_DIV_CLKCMU_CIS_CLK3,
	CLK_CON_DIV_CLKCMU_CMGP_BUS,
	CLK_CON_DIV_CLKCMU_CORE_BUS,
	CLK_CON_DIV_CLKCMU_CPUCL0_DBG_BUS,
	CLK_CON_DIV_CLKCMU_CPUCL0_SWITCH,
	CLK_CON_DIV_CLKCMU_CPUCL1_SWITCH,
	CLK_CON_DIV_CLKCMU_DCF_BUS,
	CLK_CON_DIV_CLKCMU_DCPOST_BUS,
	CLK_CON_DIV_CLKCMU_DCRD_BUS,
	CLK_CON_DIV_CLKCMU_DSPM_BUS,
	CLK_CON_DIV_CLKCMU_DSPS_AUD,
	CLK_CON_DIV_CLKCMU_FSYS0_BUS,
	CLK_CON_DIV_CLKCMU_FSYS0_DPGTC,
	CLK_CON_DIV_CLKCMU_FSYS0_UFS_EMBD,
	CLK_CON_DIV_CLKCMU_FSYS0_USB30DRD,
	CLK_CON_DIV_CLKCMU_FSYS0_USBDP_DEBUG,
	CLK_CON_DIV_CLKCMU_FSYS1_BUS,
	CLK_CON_DIV_CLKCMU_FSYS1_MMC_CARD,
	CLK_CON_DIV_CLKCMU_FSYS1_PCIE,
	CLK_CON_DIV_CLKCMU_FSYS1_UFS_CARD,
	CLK_CON_DIV_CLKCMU_G2D_G2D,
	CLK_CON_DIV_CLKCMU_G2D_MSCL,
	CLK_CON_DIV_CLKCMU_G3D_SWITCH,
	CLK_CON_DIV_CLKCMU_HPM,
	CLK_CON_DIV_CLKCMU_ISPHQ_BUS,
	CLK_CON_DIV_CLKCMU_ISPLP_BUS,
	CLK_CON_DIV_CLKCMU_ISPLP_GDC,
	CLK_CON_DIV_CLKCMU_ISPLP_VRA,
	CLK_CON_DIV_CLKCMU_ISPPRE_BUS,
	CLK_CON_DIV_CLKCMU_IVA_BUS,
	CLK_CON_DIV_CLKCMU_MFC_BUS,
	CLK_CON_DIV_CLKCMU_MFC_WFD,
	CLK_CON_DIV_CLKCMU_MIF_BUSP,
	CLK_CON_DIV_CLKCMU_MODEM_SHARED0,
	CLK_CON_DIV_CLKCMU_MODEM_SHARED1,
	CLK_CON_DIV_CLKCMU_OTP,
	CLK_CON_DIV_CLKCMU_PERIC0_BUS,
	CLK_CON_DIV_CLKCMU_PERIC0_IP,
	CLK_CON_DIV_CLKCMU_PERIC1_BUS,
	CLK_CON_DIV_CLKCMU_PERIC1_IP,
	CLK_CON_DIV_CLKCMU_PERIS_BUS,
	CLK_CON_DIV_CLKCMU_VTS_BUS,
	CLK_CON_DIV_CLKCMU_DPU,
	CLK_CON_DIV_CLKCMU_DPU_BUS,
	CLK_CON_DIV_CLK_CMU_CMUREF,
	CLK_CON_DIV_PLL_SHARED0_DIV2,
	CLK_CON_DIV_PLL_SHARED0_DIV3,
	CLK_CON_DIV_PLL_SHARED0_DIV4,
	CLK_CON_DIV_PLL_SHARED1_DIV2,
	CLK_CON_DIV_PLL_SHARED1_DIV3,
	CLK_CON_DIV_PLL_SHARED1_DIV4,
	CLK_CON_DIV_PLL_SHARED2_DIV2,
	CLK_CON_DIV_PLL_SHARED3_DIV2,
	CLK_CON_DIV_PLL_SHARED4_DIV2,
	CLK_CON_GATE_CLKCMU_APM_BUS,
	CLK_CON_GATE_CLKCMU_AUD_CPU,
	CLK_CON_GATE_CLKCMU_BUS1_BUS,
	CLK_CON_GATE_CLKCMU_BUSC_BUS,
	CLK_CON_GATE_CLKCMU_CHUB_BUS,
	CLK_CON_GATE_CLKCMU_CIS_CLK0,
	CLK_CON_GATE_CLKCMU_CIS_CLK1,
	CLK_CON_GATE_CLKCMU_CIS_CLK2,
	CLK_CON_GATE_CLKCMU_CIS_CLK3,
	CLK_CON_GATE_CLKCMU_CMGP_BUS,
	CLK_CON_GATE_CLKCMU_CORE_BUS,
	CLK_CON_GATE_CLKCMU_CPUCL0_DBG_BUS,
	CLK_CON_GATE_CLKCMU_CPUCL0_SWITCH,
	CLK_CON_GATE_CLKCMU_CPUCL1_SWITCH,
	CLK_CON_GATE_CLKCMU_DCF_BUS,
	CLK_CON_GATE_CLKCMU_DCPOST_BUS,
	CLK_CON_GATE_CLKCMU_DCRD_BUS,
	CLK_CON_GATE_CLKCMU_DPU,
	CLK_CON_GATE_CLKCMU_DPU_BUS,
	CLK_CON_GATE_CLKCMU_DSPM_BUS,
	CLK_CON_GATE_CLKCMU_DSPS_AUD,
	CLK_CON_GATE_CLKCMU_FSYS0_BUS,
	CLK_CON_GATE_CLKCMU_FSYS0_DPGTC,
	CLK_CON_GATE_CLKCMU_FSYS0_UFS_EMBD,
	CLK_CON_GATE_CLKCMU_FSYS0_USB30DRD,
	CLK_CON_GATE_CLKCMU_FSYS0_USBDP_DEBUG,
	CLK_CON_GATE_CLKCMU_FSYS1_BUS,
	CLK_CON_GATE_CLKCMU_FSYS1_MMC_CARD,
	CLK_CON_GATE_CLKCMU_FSYS1_PCIE,
	CLK_CON_GATE_CLKCMU_FSYS1_UFS_CARD,
	CLK_CON_GATE_CLKCMU_G2D_G2D,
	CLK_CON_GATE_CLKCMU_G2D_MSCL,
	CLK_CON_GATE_CLKCMU_G3D_SWITCH,
	CLK_CON_GATE_CLKCMU_HPM,
	CLK_CON_GATE_CLKCMU_ISPHQ_BUS,
	CLK_CON_GATE_CLKCMU_ISPLP_BUS,
	CLK_CON_GATE_CLKCMU_ISPLP_GDC,
	CLK_CON_GATE_CLKCMU_ISPLP_VRA,
	CLK_CON_GATE_CLKCMU_ISPPRE_BUS,
	CLK_CON_GATE_CLKCMU_IVA_BUS,
	CLK_CON_GATE_CLKCMU_MFC_BUS,
	CLK_CON_GATE_CLKCMU_MFC_WFD,
	CLK_CON_GATE_CLKCMU_MIF_BUSP,
	CLK_CON_GATE_CLKCMU_MODEM_SHARED0,
	CLK_CON_GATE_CLKCMU_MODEM_SHARED1,
	CLK_CON_GATE_CLKCMU_PERIC0_BUS,
	CLK_CON_GATE_CLKCMU_PERIC0_IP,
	CLK_CON_GATE_CLKCMU_PERIC1_BUS,
	CLK_CON_GATE_CLKCMU_PERIC1_IP,
	CLK_CON_GATE_CLKCMU_PERIS_BUS,
	CLK_CON_GATE_CLKCMU_VTS_BUS,
};

static const struct samsung_pll_clock top_pll_clks[] __initconst = {
	/* CMU_TOP_PURECLKCOMP */
	PLL(pll_1417x, FOUT_SHARED0_PLL, "fout_shared0_pll", "oscclk",
	    PLL_LOCKTIME_PLL_SHARED0, PLL_CON0_PLL_SHARED0, NULL),
	PLL(pll_1417x, FOUT_SHARED0_PLL, "fout_shared1_pll", "oscclk",
	    PLL_LOCKTIME_PLL_SHARED1, PLL_CON0_PLL_SHARED1, NULL),
	PLL(pll_1417x, FOUT_SHARED0_PLL, "fout_shared2_pll", "oscclk", // 2 3 4 are 1018 actually
	    PLL_LOCKTIME_PLL_SHARED2, PLL_CON0_PLL_SHARED2, NULL),
	PLL(pll_1417x, FOUT_SHARED0_PLL, "fout_shared3_pll", "oscclk",
	    PLL_LOCKTIME_PLL_SHARED3, PLL_CON0_PLL_SHARED3, NULL),
	PLL(pll_1417x, FOUT_SHARED0_PLL, "fout_shared4_pll", "oscclk",
	    PLL_LOCKTIME_PLL_SHARED4, PLL_CON0_PLL_SHARED4, NULL),
	PLL(pll_0831x, FOUT_MMC_PLL, "fout_mmc_pll", "oscclk",
	     PLL_LOCKTIME_PLL_MMC, PLL_CON0_PLL_MMC, NULL),
};

/* List of parent clocks for Muxes in CMU_TOP */
PNAME(mout_clkcmu_apm_bus_p) = { "dout_shared0_div3", "dout_shared2_div2" };
PNAME(mout_clkcmu_core_bus_p) = { "dout_shared0_div2", "dout_shared1_div2",
				  "fout_shared2_pll", "dout_shared1_div3", "fout_shared4_pll",
				  "dout_shared0_div4", "fout_shared3_pll", "fout_mmc_pll" };
PNAME(mout_clkcmu_dpu_bus_p) = { "fout_shared3_pll", "fout_shared4_pll",
				"dout_shared1_div3", "dout_shared2_div2", };
PNAME(mout_clkcmu_aud_cpu_p) = { "dout_shared0_div2", "dout_shared1_div2",
				  "fout_shared2_pll", "fout_shared4_pll" };
PNAME(mout_clkcmu_fsys0_bus_p) = { "dout_shared1_div2", "fout_shared2_pll",
					"fout_shared4_pll", "dout_shared0_div4" };
PNAME(mout_clkcmu_fsys0_dpgtc_p) = { "oscclk", "dout_shared0_div4",
					"dout_shared2_div2", "dout_shared4_div2", };
PNAME(mout_clkcmu_fsys0_ufs_embd_p) = { "oscclk", "dout_shared0_div4",
					"dout_shared2_div2", "dout_shared4_div2" };
PNAME(mout_clkcmu_fsys0_usb30drd_p) = { "oscclk", "dout_shared0_div4",
					"dout_shared2_div2", "dout_shared4_div2", };
PNAME(mout_clkcmu_fsys1_bus_p) = { "dout_shared1_div2", "fout_shared2_pll",
					"fout_shared4_pll","dout_shared0_div4" };
PNAME(mout_clkcmu_fsys1_mmc_card_p) = { "oscclk", "fout_shared2_pll", "fout_shared4_pll",
				  "dout_shared0_div4", "fout_mmc_pll", "oscclk", "oscclk", "oscclk" };
PNAME(mout_clkcmu_mif_switch_p) = { "fout_shared0_pll", "fout_shared1_pll", "dout_shared0_div2",
					"dout_shared1_div2", "fout_shared2_pll", "dout_shared0_div4",
					"dout_shared2_div2", "oscclk" };
PNAME(mout_clkcmu_peris_bus_p) = { "dout_shared0_div4", "dout_shared2_div2" };

static const struct samsung_mux_clock top_mux_clks[] __initconst = {
	/* CORE */
	MUX(MOUT_CLKCMU_CORE_BUS, "mout_clkcmu_core_bus", mout_clkcmu_core_bus_p,
	    CLK_CON_MUX_CLKCMU_CORE_BUS, 0, 3),

	/* APM */
	MUX(MOUT_CLKCMU_APM_BUS, "mout_clkcmu_apm_bus", mout_clkcmu_apm_bus_p,
	    CLK_CON_MUX_CLKCMU_APM_BUS, 0, 1),

	/* AUD */
	MUX(MOUT_CLKCMU_AUD_CPU, "mout_clkcmu_aud_cpu", mout_clkcmu_aud_cpu_p,
	    CLK_CON_MUX_CLKCMU_AUD_CPU, 0, 1),

	/* FSYS0 */
	MUX(MOUT_CLKCMU_FSYS0_BUS, "mout_clkcmu_fsys0_bus", mout_clkcmu_fsys0_bus_p,
	    CLK_CON_MUX_CLKCMU_FSYS0_BUS, 0, 2),
	MUX(MOUT_CLKCMU_FSYS0_DPGTC, "mout_clkcmu_fsys0_dpgtc", mout_clkcmu_fsys0_dpgtc_p,
	    CLK_CON_MUX_CLKCMU_FSYS0_DPGTC, 0, 2),
	MUX(MOUT_CLKCMU_FSYS0_UFS_EMBD, "mout_clkcmu_fsys0_ufs_embd", mout_clkcmu_fsys0_ufs_embd_p,
	    CLK_CON_MUX_CLKCMU_FSYS0_UFS_EMBD, 0, 2),
	MUX(MOUT_CLKCMU_FSYS0_USB30DRD, "mout_clkcmu_fsys0_usb30drd", mout_clkcmu_fsys0_usb30drd_p,
	    CLK_CON_MUX_CLKCMU_FSYS0_USB30DRD, 0, 2),

	/* FSYS1 */
	MUX(MOUT_CLKCMU_FSYS1_BUS, "mout_clkcmu_fsys1_bus", mout_clkcmu_fsys1_bus_p,
	    CLK_CON_MUX_CLKCMU_FSYS1_BUS, 0, 2),
	MUX(MOUT_CLKCMU_FSYS1_MMC_CARD, "mout_clkcmu_fsys1_mmc_card", mout_clkcmu_fsys1_mmc_card_p,
	    CLK_CON_MUX_CLKCMU_FSYS1_MMC_CARD, 0, 2),

	/* MIF */
	MUX(MOUT_CLKCMU_MIF_SWITCH, "mout_clkcmu_mif_switch", mout_clkcmu_mif_switch_p,
	     CLK_CON_MUX_CLKCMU_MIF_SWITCH, 0, 3),

	/* PERIS */
	MUX(MOUT_CLKCMU_PERIS_BUS, "mout_clkcmu_peris_bus", mout_clkcmu_peris_bus_p,
	    CLK_CON_MUX_CLKCMU_PERIS_BUS, 0, 1),
};

static const struct samsung_div_clock top_div_clks[] __initconst = {
	/* SHARED0 */
	DIV(DOUT_PLL_SHARED0_DIV2, "dout_shared0_div2", "fout_shared0_pll",
	    CLK_CON_DIV_PLL_SHARED0_DIV2, 0, 1),

	DIV(DOUT_PLL_SHARED0_DIV3, "dout_shared0_div3", "fout_shared0_pll",
	    CLK_CON_DIV_PLL_SHARED0_DIV3, 0, 2),

	DIV(DOUT_PLL_SHARED0_DIV4, "dout_shared0_div4", "dout_shared0_div2",
	    CLK_CON_DIV_PLL_SHARED0_DIV4, 0, 1),

	/* SHARED1 */
	DIV(DOUT_PLL_SHARED1_DIV2, "dout_shared1_div2", "fout_shared1_pll",
	    CLK_CON_DIV_PLL_SHARED1_DIV2, 0, 1),

	DIV(DOUT_PLL_SHARED1_DIV3, "dout_shared1_div3", "fout_shared1_pll",
	    CLK_CON_DIV_PLL_SHARED1_DIV3, 0, 2),

	DIV(DOUT_PLL_SHARED1_DIV4, "dout_shared1_div4", "dout_shared1_div2",
	    CLK_CON_DIV_PLL_SHARED1_DIV4, 0, 1),

	/* SHARED2 */
	DIV(DOUT_PLL_SHARED2_DIV2, "dout_shared2_div2", "fout_shared2_pll",
	    CLK_CON_DIV_PLL_SHARED2_DIV2, 0, 1),

	/* SHARED3 */
	DIV(DOUT_PLL_SHARED3_DIV2, "dout_shared3_div2", "fout_shared3_pll",
	    CLK_CON_DIV_PLL_SHARED3_DIV2, 0, 1),

	/* SHARED4 */
	DIV(DOUT_PLL_SHARED4_DIV2, "dout_shared4_div2", "fout_shared4_pll",
	    CLK_CON_DIV_PLL_SHARED4_DIV2, 0, 1),

	/* CORE */
	DIV(DOUT_CLKCMU_CORE_BUS, "dout_clkcmu_core_bus", "gout_clkcmu_core_bus",
	    CLK_CON_DIV_CLKCMU_CORE_BUS, 0, 4),

	/* APM */
	DIV(DOUT_CLKCMU_APM_BUS, "dout_clkcmu_apm_bus", "gout_clkcmu_apm_bus",
	    CLK_CON_DIV_CLKCMU_APM_BUS, 0, 3),

	/* AUD */
	DIV(DOUT_CLKCMU_AUD_CPU, "dout_clkcmu_aud_cpu", "gout_clkcmu_aud_cpu",
	    CLK_CON_DIV_CLKCMU_AUD_CPU, 0, 3),

	/* FSYS0 */
	DIV(DOUT_CLKCMU_FSYS0_BUS, "dout_clkcmu_fsys0_bus", "gout_clkcmu_fsys0_bus",
	    CLK_CON_DIV_CLKCMU_FSYS0_BUS, 0, 4),
	DIV(DOUT_CLKCMU_FSYS0_DPGTC, "dout_clkcmu_fsys0_dpgtc", "gout_clkcmu_fsys0_dpgtc",
	    CLK_CON_DIV_CLKCMU_FSYS0_DPGTC, 0, 3),
	DIV(DOUT_CLKCMU_FSYS0_UFS_EMBD, "dout_clkcmu_fsys0_ufs_embd", "gout_clkcmu_fsys0_ufs_embd",
	    CLK_CON_DIV_CLKCMU_FSYS0_UFS_EMBD, 0, 3),
	DIV(DOUT_CLKCMU_FSYS0_USB30DRD, "dout_clkcmu_fsys0_usb30drd", "gout_clkcmu_fsys0_usb30drd",
	    CLK_CON_DIV_CLKCMU_FSYS0_USB30DRD, 0, 4),

	/* FSYS1 */
	DIV(DOUT_CLKCMU_FSYS1_BUS, "dout_clkcmu_fsys1_bus", "gout_clkcmu_fsys1_bus",
	    CLK_CON_DIV_CLKCMU_FSYS0_BUS, 0, 4),
	DIV(DOUT_CLKCMU_FSYS1_MMC_CARD, "dout_clkcmu_fsys1_mmc_card", "gout_clkcmu_fsys1_mmc_card",
	    CLK_CON_DIV_CLKCMU_FSYS1_MMC_CARD, 0, 9),

	/* PERIS */
	DIV(DOUT_CLKCMU_PERIS_BUS, "dout_clkcmu_peris_bus", "gout_clkcmu_peris_bus",
	    CLK_CON_DIV_CLKCMU_PERIS_BUS, 0, 4),
	
	/* G3D */
	DIV(DOUT_CLKCMU_G3D_SWITCH, "dout_clkcmu_g3d_switch", "gout_clkcmu_g3d_switch",
	    CLK_CON_DIV_CLKCMU_G3D_SWITCH, 0, 3),
};

static const struct samsung_gate_clock top_gate_clks[] __initconst = {
	/* CORE */
	GATE(GOUT_CLKCMU_CORE_BUS, "gout_clkcmu_core_bus", "mout_clkcmu_core_bus",
	     CLK_CON_GATE_CLKCMU_CORE_BUS, 21, CLK_IGNORE_UNUSED, 0),

	/* APM */
	GATE(GOUT_CLKCMU_APM_BUS, "gout_clkcmu_apm_bus", "mout_clkcmu_apm_bus",
	     CLK_CON_GATE_CLKCMU_APM_BUS, 21, CLK_IGNORE_UNUSED, 0),

	/* AUD */
	GATE(GOUT_CLKCMU_AUD_CPU, "gout_clkcmu_aud_cpu", "mout_clkcmu_aud_cpu",
	     CLK_CON_GATE_CLKCMU_AUD_CPU, 21, 0, 0),

	/* FSYS0 */
	GATE(GOUT_CLKCMU_FSYS0_BUS, "gout_clkcmu_fsys0_bus", "mout_clkcmu_fsys0_bus",
	     CLK_CON_GATE_CLKCMU_FSYS0_BUS, 21, CLK_IGNORE_UNUSED, 0),
	GATE(GOUT_CLKCMU_FSYS0_DPGTC, "gout_clkcmu_fsys0_dpgtc", "mout_clkcmu_fsys0_dpgtc",
	     CLK_CON_GATE_CLKCMU_FSYS0_DPGTC, 21, 0, 0),
	GATE(GOUT_CLKCMU_FSYS0_UFS_EMBD, "gout_clkcmu_fsys0_ufs_embd", "mout_clkcmu_fsys0_ufs_embd",
	     CLK_CON_GATE_CLKCMU_FSYS0_UFS_EMBD, 21, CLK_IGNORE_UNUSED, 0),
	GATE(GOUT_CLKCMU_FSYS0_USB30DRD, "gout_clkcmu_fsys0_usb30drd", "mout_clkcmu_fsys0_usb30drd",
	     CLK_CON_GATE_CLKCMU_FSYS0_USB30DRD, 21, 0, 0),
	
	/* FSYS1 */
	GATE(GOUT_CLKCMU_FSYS1_BUS, "gout_clkcmu_fsys1_bus", "mout_clkcmu_fsys1_bus",
	     CLK_CON_GATE_CLKCMU_FSYS1_BUS, 21, CLK_IGNORE_UNUSED, 0),
	GATE(GOUT_CLKCMU_FSYS1_MMC_CARD, "gout_clkcmu_fsys1_mmc_card", "mout_clkcmu_fsys1_mmc_card",
	     CLK_CON_GATE_CLKCMU_FSYS1_MMC_CARD, 21, CLK_IGNORE_UNUSED, 0),

	/* MIF */
	GATE(GOUT_CLKCMU_MIF_SWITCH, "gout_clkcmu_mif_switch", "mout_clkcmu_mif_switch",
	     CLK_CON_GATE_CLKCMU_MIF_SWITCH, 21, CLK_IGNORE_UNUSED, 0),

	/* PERIS */
	GATE(GOUT_CLKCMU_PERIS_BUS, "gout_clkcmu_peris_bus", "mout_clkcmu_peris_bus",
	     CLK_CON_GATE_CLKCMU_PERIS_BUS, 21, CLK_IGNORE_UNUSED, 0),

	/* G3D */
	GATE(GOUT_CLKCMU_G3D_SWITCH, "gout_clkcmu_g3d_switch", "fout_shared2_pll",
	     CLK_CON_GATE_CLKCMU_G3D_SWITCH, 21, 0, 0),
};

static const struct samsung_cmu_info top_cmu_info __initconst = {
	.pll_clks		= top_pll_clks,
	.nr_pll_clks		= ARRAY_SIZE(top_pll_clks),
	.mux_clks		= top_mux_clks,
	.nr_mux_clks		= ARRAY_SIZE(top_mux_clks),
	.div_clks		= top_div_clks,
	.nr_div_clks		= ARRAY_SIZE(top_div_clks),
	.gate_clks		= top_gate_clks,
	.nr_gate_clks		= ARRAY_SIZE(top_gate_clks),
	.nr_clk_ids		= 200,
	.clk_regs		= top_clk_regs,
	.nr_clk_regs		= ARRAY_SIZE(top_clk_regs),
};

static void __init exynos9810_cmu_top_init(struct device_node *np)
{
	exynos_arm64_register_cmu(NULL, np, &top_cmu_info);
}

/* Register CMU_TOP early, as it's a dependency for other early domains */
CLK_OF_DECLARE(exynos9810_cmu_top, "samsung,exynos9810-cmu-top",
	       exynos9810_cmu_top_init);


/* ---- CMU_FSYS0 ---------------------------------------------------------- */

/* Register Offset definitions for CMU_FSYS0 (0x11000000) */
#define CLK_CON_MUX_CLKCMU_FSYS0_BUS_USER				0x0100
#define CLK_CON_MUX_CLKCMU_FSYS0_UFS_EMBD_USER				0x0180
#define PLL_CON0_MUX_CLKCMU_FSYS0_USB30DRD_USER				0x01e0
#define CLK_CON_GAT_FSYS0_UFS_EMBD_IPCLKPORT_UNIPRO			0x205c
#define CLK_CON_GAT_GOUT_BLK_FSYS0_UID_USB30DRD_IPCLKPORT_I_USB30DRD_REF_CLK 0x206c

static const unsigned long fsys0_clk_regs[] __initconst = {
	CLK_CON_MUX_CLKCMU_FSYS0_BUS_USER,
	CLK_CON_MUX_CLKCMU_FSYS0_UFS_EMBD_USER,
	PLL_CON0_MUX_CLKCMU_FSYS0_USB30DRD_USER,
	CLK_CON_GAT_FSYS0_UFS_EMBD_IPCLKPORT_UNIPRO,
};

/* List of parent clocks for Muxes in CMU_FSYS0 */
PNAME(mout_fsys0_bus_user_p) = { "oscclk", "dout_clkcmu_fsys0_bus" };
PNAME(mout_fsys0_ufs_embd_user_p) = { "oscclk", "dout_clkcmu_fsys0_ufs_embd" };
PNAME(mout_fsys0_usbdrd30_user_p) = { "oscclk", "dout_clkcmu_fsys0_usb30drd" };

static const struct samsung_mux_clock fsys0_mux_clks[] __initconst = {
	MUX(MOUT_FSYS0_BUS_USER, "mout_fsys0_bus_user",
	    mout_fsys0_bus_user_p, CLK_CON_MUX_CLKCMU_FSYS0_BUS_USER, 4, 1),
	MUX(MOUT_FSYS0_UFS_EMBD_USER, "mout_fsys0_ufs_embd_user",
	    mout_fsys0_ufs_embd_user_p, CLK_CON_MUX_CLKCMU_FSYS0_UFS_EMBD_USER, 4, 1),
	MUX(MOUT_FSYS0_USBDRD30_USER, "mout_fsys0_usbdrd30_user",
	    mout_fsys0_usbdrd30_user_p, PLL_CON0_MUX_CLKCMU_FSYS0_USB30DRD_USER, 4, 1),
};

static const struct samsung_gate_clock fsys0_gate_clks[] __initconst = {
	GATE(GOUT_FSYS0_UFS_EMBD_UNIPRO, "gout_fsys0_ufs_embd_unipro",
	     "mout_fsys0_ufs_embd_user", CLK_CON_GAT_FSYS0_UFS_EMBD_IPCLKPORT_UNIPRO, 21, CLK_IGNORE_UNUSED, 0),
	GATE(GOUT_FSYS0_USBDRD30_REFCLK, "gout_fsys0_usbdrd30_refclk",
	     "mout_fsys0_usbdrd30_user", CLK_CON_GAT_GOUT_BLK_FSYS0_UID_USB30DRD_IPCLKPORT_I_USB30DRD_REF_CLK, 21, CLK_IGNORE_UNUSED, 0),
};

static const struct samsung_cmu_info fsys0_cmu_info __initconst = {
	.mux_clks		= fsys0_mux_clks,
	.nr_mux_clks		= ARRAY_SIZE(fsys0_mux_clks),
	.gate_clks		= fsys0_gate_clks,
	.nr_gate_clks		= ARRAY_SIZE(fsys0_gate_clks),
	.nr_clk_ids		= 200,
	.clk_regs		= fsys0_clk_regs,
	.nr_clk_regs		= ARRAY_SIZE(fsys0_clk_regs),
	.clk_name		= "dout_clkcmu_fsys0_bus",
};

/* ---- CMU_MIF ---------------------------------------------------------- */

/* Register Offset definitions for CMU_MIF (0x1b800000) */
#define PLL_LOCKTIME_PLL_MIF						0x0000
#define PLL_CON0_PLL_MIF						0x0120
#define CLK_CON_MUX_CLKCMU_MIF_DDRPHY2X					0x1000
#define CLK_CON_DIV_CLKCMU_MIF_PRE					0x1810

static const unsigned long mif_clk_regs[] __initconst = {
	PLL_LOCKTIME_PLL_MIF,
	PLL_CON0_PLL_MIF,
	CLK_CON_MUX_CLKCMU_MIF_DDRPHY2X,
	CLK_CON_DIV_CLKCMU_MIF_PRE,
};

static const struct samsung_pll_clock mif_pll_clks[] __initconst = {
	PLL(pll_1450x, FOUT_MIF_PLL, "fout_mif_pll", "oscclk",
	    PLL_LOCKTIME_PLL_MIF, PLL_CON0_PLL_MIF, NULL),
};

/* List of parent clocks for Muxes in CMU_MIF */
PNAME(mout_mif_ddrphy2x_p) = { "gout_clkcmu_mif_switch", "fout_mif_pll" };

static const struct samsung_mux_clock mif_mux_clks[] __initconst = {
	MUX(MOUT_MIF_DDRPHY2X, "mout_mif_ddrphy2x",
	    mout_mif_ddrphy2x_p, CLK_CON_MUX_CLKCMU_MIF_DDRPHY2X, 0, 1),
};

static const struct samsung_div_clock mif_div_clocks[] __initconst = {
	DIV(DOUT_MIF_PRE, "dout_mif_pre", "fout_mif_pll",
	    CLK_CON_DIV_CLKCMU_MIF_PRE, 0, 3),
};

static const struct samsung_cmu_info mif_cmu_info __initconst = {
	.pll_clks		= mif_pll_clks,
	.nr_pll_clks		= ARRAY_SIZE(mif_pll_clks),
	.mux_clks		= mif_mux_clks,
	.nr_mux_clks		= ARRAY_SIZE(mif_mux_clks),
	.div_clks		= mif_div_clocks,
	.nr_div_clks		= ARRAY_SIZE(mif_div_clocks),
	.nr_clk_ids		= 200,
	.clk_regs		= mif_clk_regs,
	.nr_clk_regs		= ARRAY_SIZE(mif_clk_regs),
	.clk_name		= "gout_clkcmu_mif_switch",
};

/* ---- CMU_G3D ---------------------------------------------------------- */

/* Register Offset definitions for CMU_G3D (0x1b800000) */
#define PLL_LOCKTIME_PLL_G3D						0x0000
#define PLL_CON0_MUX_CLKCMU_EMBEDDED_G3D_USER				0x0100
#define PLL_CON0_PLL_G3D						0x0140
#define PLL_CON0_MUX_CLKCMU_G3D_SWITCH_USER				0x0120
#define CLK_CON_MUX_G3D_BUSD						0x1000
#define CLK_CON_DIV_G3D_BUSD						0x1800

static const unsigned long g3d_clk_regs[] __initconst = {
	PLL_LOCKTIME_PLL_G3D,
	PLL_CON0_MUX_CLKCMU_EMBEDDED_G3D_USER,
	PLL_CON0_PLL_G3D,
	PLL_CON0_MUX_CLKCMU_G3D_SWITCH_USER,
	CLK_CON_MUX_G3D_BUSD,
	CLK_CON_DIV_G3D_BUSD,
};

static const struct samsung_pll_clock g3d_pll_clks[] __initconst = {
	PLL(pll_1450x, FOUT_G3D_PLL, "fout_g3d_pll", "oscclk",
	    PLL_LOCKTIME_PLL_G3D, PLL_CON0_PLL_G3D, NULL),
};

/* List of parent clocks for Muxes in CMU_G3D */
PNAME(mout_g3d_switch_user_p) = { "oscclk", "gout_clkcmu_g3d_switch" };
PNAME(mout_g3d_busd_p) = { "fout_g3d_pll", "mout_g3d_switch_user" };
PNAME(mout_g3d_embedded_g3d_user_p) = { "oscclk", "dout_g3d_busd" };

static const struct samsung_mux_clock g3d_mux_clks[] __initconst = {
	MUX(MOUT_G3D_SWITCH_USER, "mout_g3d_switch_user",
	    mout_g3d_switch_user_p, PLL_CON0_MUX_CLKCMU_G3D_SWITCH_USER, 4, 1),
	MUX(MOUT_G3D_BUSD, "mout_g3d_busd",
	    mout_g3d_busd_p, CLK_CON_MUX_G3D_BUSD, 0, 1),
	MUX(MOUT_G3D_EMBEDDED_USER, "mout_g3d_embedded_user",
	    mout_g3d_embedded_g3d_user_p, PLL_CON0_MUX_CLKCMU_EMBEDDED_G3D_USER, 4, 1),
};

static const struct samsung_div_clock g3d_div_clocks[] __initconst = {
	DIV(DOUT_G3D_BUSD, "dout_g3d_busd", "mout_g3d_busd",
	    CLK_CON_DIV_G3D_BUSD, 0, 12),
};

static const struct samsung_cmu_info g3d_cmu_info __initconst = {
	.pll_clks		= g3d_pll_clks,
	.nr_pll_clks		= ARRAY_SIZE(g3d_pll_clks),
	.mux_clks		= g3d_mux_clks,
	.nr_mux_clks		= ARRAY_SIZE(g3d_mux_clks),
	.div_clks		= g3d_div_clocks,
	.nr_div_clks		= ARRAY_SIZE(g3d_div_clocks),
	.nr_clk_ids		= 200,
	.clk_regs		= g3d_clk_regs,
	.nr_clk_regs		= ARRAY_SIZE(g3d_clk_regs),
	.clk_name		= "gout_clkcmu_g3d_switch",
};


/* ---- platform_driver ----------------------------------------------------- */

static int __init exynos9810_cmu_probe(struct platform_device *pdev)
{
	const struct samsung_cmu_info *info;
	struct device *dev = &pdev->dev;

	info = of_device_get_match_data(dev);
	exynos_arm64_register_cmu(dev, dev->of_node, info);

	return 0;
}

static const struct of_device_id exynos9810_cmu_of_match[] = {
	{
		.compatible = "samsung,exynos9810-cmu-fsys0",
		.data = &fsys0_cmu_info,
	},
	{
		.compatible = "samsung,exynos9810-cmu-mif",
		.data = &mif_cmu_info,
	},
	{
		.compatible = "samsung,exynos9810-cmu-g3d",
		.data = &g3d_cmu_info,
	}, 
	{ },
};

static struct platform_driver exynos9810_cmu_driver __refdata = {
	.driver	= {
		.name = "exynos9810-cmu",
		.of_match_table = exynos9810_cmu_of_match,
		.suppress_bind_attrs = true,
	},
	.probe = exynos9810_cmu_probe,
};

static int __init exynos9810_cmu_init(void)
{
	return platform_driver_register(&exynos9810_cmu_driver);
}
core_initcall(exynos9810_cmu_init);
