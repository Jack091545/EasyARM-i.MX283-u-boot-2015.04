/*
 * Freescale MX28EVK board
 *
 * (C) Copyright 2011 Freescale Semiconductor, Inc.
 *
 * Author: Fabio Estevam <fabio.estevam@freescale.com>
 *
 * Based on m28evk.c:
 * Copyright (C) 2011 Marek Vasut <marek.vasut@gmail.com>
 * on behalf of DENX Software Engineering GmbH
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/gpio.h>
#include <asm/io.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/iomux-mx28.h>
#include <asm/arch/clock.h>
#include <asm/arch/sys_proto.h>
#include <linux/mii.h>
#include <miiphy.h>
#include <netdev.h>
#include <errno.h>

DECLARE_GLOBAL_DATA_PTR;

/*
 * Functions
 */
int board_early_init_f(void)
{
	/* IO0 clock at 480MHz */
	mxs_set_ioclk(MXC_IOCLK0, 480000);
	/* IO1 clock at 480MHz */
	mxs_set_ioclk(MXC_IOCLK1, 480000);

	/* SSP0 clock at 96MHz */
	mxs_set_sspclk(MXC_SSPCLK0, 96000, 0);
	/* SSP2 clock at 160MHz */
	mxs_set_sspclk(MXC_SSPCLK2, 160000, 0);

#ifdef	CONFIG_CMD_USB
	mxs_iomux_setup_pad(MX28_PAD_SSP2_SS1__USB1_OVERCURRENT);
	mxs_iomux_setup_pad(MX28_PAD_AUART2_RX__GPIO_3_8 |
			MXS_PAD_4MA | MXS_PAD_3V3 | MXS_PAD_NOPULL);
	gpio_direction_output(MX28_PAD_AUART2_RX__GPIO_3_8, 1);
#endif

	/* ÉèÖÃNANDµÄIO */
	const iomux_cfg_t gpmi_pins[] = {
		MX28_PAD_GPMI_D00__GPMI_D0 | MXS_PAD_4MA | MXS_PAD_3V3 | MXS_PAD_NOPULL,
		MX28_PAD_GPMI_D01__GPMI_D1 | MXS_PAD_4MA | MXS_PAD_3V3 | MXS_PAD_NOPULL,
		MX28_PAD_GPMI_D02__GPMI_D2 | MXS_PAD_4MA | MXS_PAD_3V3 | MXS_PAD_NOPULL,
		MX28_PAD_GPMI_D03__GPMI_D3 | MXS_PAD_4MA | MXS_PAD_3V3 | MXS_PAD_NOPULL,
		MX28_PAD_GPMI_D04__GPMI_D4 | MXS_PAD_4MA | MXS_PAD_3V3 | MXS_PAD_NOPULL,
		MX28_PAD_GPMI_D05__GPMI_D5 | MXS_PAD_4MA | MXS_PAD_3V3 | MXS_PAD_NOPULL,
		MX28_PAD_GPMI_D06__GPMI_D6 | MXS_PAD_4MA | MXS_PAD_3V3 | MXS_PAD_NOPULL,
		MX28_PAD_GPMI_D07__GPMI_D7 | MXS_PAD_4MA | MXS_PAD_3V3 | MXS_PAD_NOPULL,
		MX28_PAD_GPMI_RDN__GPMI_RDN | MXS_PAD_8MA | MXS_PAD_1V8 | MXS_PAD_PULLUP,
		MX28_PAD_GPMI_WRN__GPMI_WRN | MXS_PAD_4MA | MXS_PAD_3V3 | MXS_PAD_NOPULL,
		MX28_PAD_GPMI_ALE__GPMI_ALE | MXS_PAD_4MA | MXS_PAD_3V3 | MXS_PAD_NOPULL,
		MX28_PAD_GPMI_CLE__GPMI_CLE | MXS_PAD_4MA | MXS_PAD_3V3 | MXS_PAD_NOPULL,
		MX28_PAD_GPMI_RDY0__GPMI_READY0 | MXS_PAD_4MA | MXS_PAD_3V3 | MXS_PAD_NOPULL,
		MX28_PAD_GPMI_RDY1__GPMI_READY1 | MXS_PAD_4MA | MXS_PAD_3V3 | MXS_PAD_NOPULL,
		MX28_PAD_GPMI_CE0N__GPMI_CE0N | MXS_PAD_4MA | MXS_PAD_3V3 | MXS_PAD_NOPULL,
		MX28_PAD_GPMI_CE1N__GPMI_CE1N | MXS_PAD_4MA | MXS_PAD_3V3 | MXS_PAD_NOPULL,
		MX28_PAD_GPMI_RESETN__GPMI_RESETN  | MXS_PAD_4MA | MXS_PAD_3V3 | MXS_PAD_NOPULL
	};
	mxs_iomux_setup_multiple_pads(gpmi_pins, ARRAY_SIZE(gpmi_pins));
	

	return 0;
}



int dram_init(void)
{	
	return mxs_dram_init();
}

int board_init(void)
{
	/* Adress of boot parameters */
	gd->bd->bi_boot_params = PHYS_SDRAM_1 + 0x100;

	return 0;
}

#ifdef	CONFIG_CMD_MMC
static int mx28evk_mmc_wp(int id)
{
	if (id != 0) {
		printf("MXS MMC: Invalid card selected (card id = %d)\n", id);
		return 1;
	}

	return gpio_get_value(MX28_PAD_SSP1_SCK__GPIO_2_12);
}


int board_mmc_init(bd_t *bis)
{
	/* Configure WP as input */
	gpio_direction_input(MX28_PAD_SSP1_SCK__GPIO_2_12);

	/* Configure MMC0 Power Enable */
	gpio_direction_output(MX28_PAD_PWM3__GPIO_3_28, 0);

	return mxsmmc_initialize(bis, 0, mx28evk_mmc_wp, NULL);
}
#endif

#ifdef	CONFIG_CMD_NET

int board_eth_init(bd_t *bis)
{
	struct mxs_clkctrl_regs *clkctrl_regs =
		(struct mxs_clkctrl_regs *)MXS_CLKCTRL_BASE;
	struct eth_device *dev;
	int ret;

	ret = cpu_eth_init(bis);
	if (ret)
		return ret;

	/* MX28EVK uses ENET_CLK PAD to drive FEC clock */
	writel(CLKCTRL_ENET_TIME_SEL_RMII_CLK | CLKCTRL_ENET_CLK_OUT_EN,
	       &clkctrl_regs->hw_clkctrl_enet);

	/* Power-on FECs */
	gpio_direction_output(MX28_PAD_SSP1_DATA3__GPIO_2_15, 0);

	/* Reset FEC PHYs */
	gpio_direction_output(MX28_PAD_ENET0_RX_CLK__GPIO_4_13, 0);
	udelay(200);
	gpio_set_value(MX28_PAD_ENET0_RX_CLK__GPIO_4_13, 1);

	ret = fecmxc_initialize_multi(bis, 0, 0, MXS_ENET0_BASE);
	if (ret) {
		puts("FEC MXS: Unable to init FEC0\n");
		return ret;
	}

	ret = fecmxc_initialize_multi(bis, 1, 3, MXS_ENET1_BASE);
	if (ret) {
		puts("FEC MXS: Unable to init FEC1\n");
		return ret;
	}

	dev = eth_get_dev_by_name("FEC0");
	if (!dev) {
		puts("FEC MXS: Unable to get FEC0 device entry\n");
		return -EINVAL;
	}

	dev = eth_get_dev_by_name("FEC1");
	if (!dev) {
		puts("FEC MXS: Unable to get FEC1 device entry\n");
		return -EINVAL;
	}

	return ret;
}




#endif
