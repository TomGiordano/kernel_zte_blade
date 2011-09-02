/* Copyright (c) 2010, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */
#include <linux/module.h>
#include <mach/irqs.h>
#include <asm/mach-types.h>
#include "gpiomux.h"
#include "gpiomux-8x60.h"

#define GPIOMUX_CFG(f, d, p) {.func = f, .drv = d, .pull = p}

#define CONSOLE_UART GPIOMUX_CFG(GPIOMUX_FUNC_2, GPIOMUX_DRV_8MA, 0)

#define GSBI1 GPIOMUX_CFG(GPIOMUX_FUNC_1, GPIOMUX_DRV_8MA, 0)
#define GSBI3 GPIOMUX_CFG(GPIOMUX_FUNC_1, GPIOMUX_DRV_8MA, 0)
#define GSBI4 GPIOMUX_CFG(GPIOMUX_FUNC_1, GPIOMUX_DRV_8MA, 0)
#define GSBI7 GPIOMUX_CFG(GPIOMUX_FUNC_1, GPIOMUX_DRV_12MA, 0)
#define GSBI8 GPIOMUX_CFG(GPIOMUX_FUNC_1, 0, 0)

#define PS_HOLD	GPIOMUX_CFG(GPIOMUX_FUNC_1, GPIOMUX_DRV_12MA, 0)

#define USB_SWITCH_EN_ACTV_CFG \
	GPIOMUX_CFG(GPIOMUX_FUNC_GPIO, GPIOMUX_DRV_2MA, GPIOMUX_PULL_NONE)
#define USB_SWITCH_CNTL_ACTV_CFG \
	GPIOMUX_CFG(GPIOMUX_FUNC_GPIO, GPIOMUX_DRV_2MA, GPIOMUX_PULL_NONE)
#define USB_HUB_RESET_ACTV_CFG \
	GPIOMUX_CFG(GPIOMUX_FUNC_GPIO, GPIOMUX_DRV_2MA, GPIOMUX_PULL_NONE)
#define USB_SWITCH_EN_SUSP_CFG \
	GPIOMUX_CFG(GPIOMUX_FUNC_GPIO, 0, GPIOMUX_PULL_DOWN)
#define USB_SWITCH_CNTL_SUSP_CFG \
	GPIOMUX_CFG(GPIOMUX_FUNC_GPIO, 0, GPIOMUX_PULL_DOWN)
#define USB_HUB_RESET_SUSP_CFG \
	GPIOMUX_CFG(GPIOMUX_FUNC_GPIO, 0, GPIOMUX_PULL_DOWN)

#define MSM_SNDDEV_ACTIVE_CONFIG \
	GPIOMUX_CFG(GPIOMUX_FUNC_1, GPIOMUX_DRV_2MA, GPIOMUX_PULL_NONE)
#define MSM_SNDDEV_SUSPEND_CONFIG GPIOMUX_CFG(0, 0, GPIOMUX_PULL_DOWN)

#define WLAN_PWDN_N_ACTV_CFG \
	GPIOMUX_CFG(GPIOMUX_FUNC_GPIO, GPIOMUX_DRV_2MA, GPIOMUX_PULL_UP)
#define WLAN_PWDN_N_SUSP_CFG \
	GPIOMUX_CFG(GPIOMUX_FUNC_GPIO, 0, GPIOMUX_PULL_DOWN)

#define EBI2_A_D GPIOMUX_CFG(GPIOMUX_FUNC_1, GPIOMUX_DRV_8MA, GPIOMUX_PULL_UP)
#define EBI2_OE  GPIOMUX_CFG(GPIOMUX_FUNC_1, GPIOMUX_DRV_8MA, GPIOMUX_PULL_UP)
#define EBI2_WE	 GPIOMUX_CFG(GPIOMUX_FUNC_1, GPIOMUX_DRV_8MA, GPIOMUX_PULL_UP)
#define EBI2_CS2 GPIOMUX_CFG(GPIOMUX_FUNC_2, GPIOMUX_DRV_8MA, GPIOMUX_PULL_UP)
#define EBI2_CS3 GPIOMUX_CFG(GPIOMUX_FUNC_1, GPIOMUX_DRV_8MA, GPIOMUX_PULL_UP)
#define EBI2_ADV GPIOMUX_CFG(GPIOMUX_FUNC_1, GPIOMUX_DRV_8MA, GPIOMUX_PULL_UP)

#define SDCC1_DAT_0_3_CMD_ACTV_CFG \
	GPIOMUX_CFG(GPIOMUX_FUNC_1, GPIOMUX_DRV_10MA, GPIOMUX_PULL_UP)
#define SDCC1_DAT_4_7_CMD_ACTV_CFG \
	GPIOMUX_CFG(GPIOMUX_FUNC_1, GPIOMUX_DRV_10MA, GPIOMUX_PULL_UP)
#define SDCC1_CLK_ACTV_CFG \
	GPIOMUX_CFG(GPIOMUX_FUNC_1, GPIOMUX_DRV_16MA, GPIOMUX_PULL_NONE)
#define SDCC1_SUSPEND_CONFIG GPIOMUX_CFG(0, 0, GPIOMUX_PULL_UP)

#define SDCC2_DAT_0_3_CMD_ACTV_CFG \
	GPIOMUX_CFG(GPIOMUX_FUNC_2, GPIOMUX_DRV_10MA, GPIOMUX_PULL_UP)
#define SDCC2_DAT_4_7_CMD_ACTV_CFG \
	GPIOMUX_CFG(GPIOMUX_FUNC_2, GPIOMUX_DRV_10MA, GPIOMUX_PULL_UP)
#define SDCC2_CLK_ACTV_CFG \
	GPIOMUX_CFG(GPIOMUX_FUNC_2, GPIOMUX_DRV_16MA, GPIOMUX_PULL_NONE)
#define SDCC2_SUSPEND_CONFIG GPIOMUX_CFG(0, 0, GPIOMUX_PULL_DOWN)

#define SDCC5_DAT_0_3_CMD_ACTV_CFG \
	GPIOMUX_CFG(GPIOMUX_FUNC_2, GPIOMUX_DRV_10MA, GPIOMUX_PULL_UP)
#define SDCC5_DAT_4_7_CMD_ACTV_CFG \
	GPIOMUX_CFG(GPIOMUX_FUNC_2, GPIOMUX_DRV_10MA, GPIOMUX_PULL_UP)
#define SDCC5_CLK_ACTV_CFG \
	GPIOMUX_CFG(GPIOMUX_FUNC_2, GPIOMUX_DRV_16MA, GPIOMUX_PULL_NONE)
#define SDCC5_SUSPEND_CONFIG GPIOMUX_CFG(0, 0, GPIOMUX_PULL_DOWN)

#define AUX_PCM_ACTIVE_CONFIG \
	GPIOMUX_CFG(GPIOMUX_FUNC_1, GPIOMUX_DRV_2MA, GPIOMUX_PULL_NONE)
#define AUX_PCM_SUSPEND_CONFIG GPIOMUX_CFG(0, 0, GPIOMUX_PULL_NONE)

#define UART1DM_ACTIVE \
	GPIOMUX_CFG(GPIOMUX_FUNC_1, GPIOMUX_DRV_8MA, GPIOMUX_PULL_NONE)
#define UART1DM_SUSPENDED \
	GPIOMUX_CFG(GPIOMUX_FUNC_GPIO, GPIOMUX_DRV_2MA, GPIOMUX_PULL_DOWN)

#define MI2S_ACTIVE_CFG \
	GPIOMUX_CFG(GPIOMUX_FUNC_1, GPIOMUX_DRV_2MA, GPIOMUX_PULL_NONE)
#define MI2S_SUSPEND_CFG GPIOMUX_CFG(0, 0, GPIOMUX_PULL_DOWN)

#define LCDC_ACTIVE_CFG \
	GPIOMUX_CFG(GPIOMUX_FUNC_1, GPIOMUX_DRV_16MA, GPIOMUX_PULL_NONE)
#define LCDC_SUSPEND_CFG GPIOMUX_CFG(0, 0, GPIOMUX_PULL_DOWN)

#define HDMI_SUSPEND_CFG GPIOMUX_CFG(0, 0, GPIOMUX_PULL_DOWN)

#define MDM2AP_STATUS_ACTIVE_CFG \
	GPIOMUX_CFG(GPIOMUX_FUNC_GPIO, GPIOMUX_DRV_2MA, GPIOMUX_PULL_NONE)
#define MDM2AP_STATUS_SUSPEND_CFG \
	GPIOMUX_CFG(0, 0, GPIOMUX_PULL_NONE)

#define CAM_SUSPEND_CFG GPIOMUX_CFG(0, 0, GPIOMUX_PULL_DOWN)

#define MDM2AP_SYNC_ACTIVE_CFG \
	GPIOMUX_CFG(GPIOMUX_FUNC_GPIO, GPIOMUX_DRV_2MA, GPIOMUX_PULL_NONE)
#define MDM2AP_SYNC_SUSPEND_CFG \
	GPIOMUX_CFG(0, 0, GPIOMUX_PULL_NONE)

static struct msm_gpiomux_config msm8x60_gsbi_configs[] __initdata = {
	{
		.gpio      = 33,
		.settings = {
			[GPIOMUX_SUSPENDED] = GSBI1,
		},
	},
	{
		.gpio      = 34,
		.settings = {
			[GPIOMUX_SUSPENDED] = GSBI1,
		},
	},
	{
		.gpio      = 35,
		.settings = {
			[GPIOMUX_SUSPENDED] = GSBI1,
		},
	},
	{
		.gpio      = 36,
		.settings = {
			[GPIOMUX_SUSPENDED] = GSBI1,
		},
	},
	{
		.gpio      = 43,
		.settings = {
			[GPIOMUX_SUSPENDED] = GSBI3,
		},
	},
	{
		.gpio      = 44,
		.settings = {
			[GPIOMUX_SUSPENDED] = GSBI3,
		},
	},
	{
		.gpio      = 47,
		.settings = {
			[GPIOMUX_SUSPENDED] = GSBI4,
		},
	},
	{
		.gpio      = 48,
		.settings = {
			[GPIOMUX_SUSPENDED] = GSBI4,
		},
	},
	{
		.gpio      = 59,
		.settings = {
			[GPIOMUX_SUSPENDED] = GSBI7,
		},
	},
	{
		.gpio      = 60,
		.settings = {
			[GPIOMUX_SUSPENDED] = GSBI7,
		},
	},
	{
		.gpio      = 64,
		.settings = {
			[GPIOMUX_SUSPENDED] = GSBI8,
		},
	},
	{
		.gpio      = 65,
		.settings = {
			[GPIOMUX_SUSPENDED] = GSBI8,
		},
	},
};

static struct msm_gpiomux_config msm8x60_ebi2_configs[] __initdata = {
	{
		.gpio      = 40,
		.settings = {
			[GPIOMUX_SUSPENDED] = EBI2_CS2,
		},
	},
	{
		.gpio      = 92,
		.settings = {
			[GPIOMUX_SUSPENDED] = PS_HOLD,
		},
	},
	{
		.gpio      = 123,
		.settings = {
			[GPIOMUX_SUSPENDED] = EBI2_A_D,
		},
	},
	{
		.gpio      = 124,
		.settings = {
			[GPIOMUX_SUSPENDED] = EBI2_A_D,
		},
	},
	{
		.gpio      = 125,
		.settings = {
			[GPIOMUX_SUSPENDED] = EBI2_A_D,
		},
	},
	{
		.gpio      = 126,
		.settings = {
			[GPIOMUX_SUSPENDED] = EBI2_A_D,
		},
	},
	{
		.gpio      = 127,
		.settings = {
			[GPIOMUX_SUSPENDED] = EBI2_A_D,
		},
	},
	{
		.gpio      = 128,
		.settings = {
			[GPIOMUX_SUSPENDED] = EBI2_A_D,
		},
	},
	{
		.gpio      = 129,
		.settings = {
			[GPIOMUX_SUSPENDED] = EBI2_A_D,
		},
	},
	{
		.gpio      = 130,
		.settings = {
			[GPIOMUX_SUSPENDED] = EBI2_A_D,
		},
	},
	{
		.gpio      = 133,
		.settings = {
			[GPIOMUX_SUSPENDED] = EBI2_CS3,
		},
	},
	{
		.gpio      = 135,
		.settings = {
			[GPIOMUX_SUSPENDED] = EBI2_A_D,
		},
	},
	{
		.gpio      = 136,
		.settings = {
			[GPIOMUX_SUSPENDED] = EBI2_A_D,
		},
	},
	{
		.gpio      = 137,
		.settings = {
			[GPIOMUX_SUSPENDED] = EBI2_A_D,
		},
	},
	{
		.gpio      = 138,
		.settings = {
			[GPIOMUX_SUSPENDED] = EBI2_A_D,
		},
	},
	{
		.gpio      = 139,
		.settings = {
			[GPIOMUX_SUSPENDED] = EBI2_A_D,
		},
	},
	{
		.gpio      = 140,
		.settings = {
			[GPIOMUX_SUSPENDED] = EBI2_A_D,
		},
	},
	{
		.gpio      = 141,
		.settings = {
			[GPIOMUX_SUSPENDED] = EBI2_A_D,
		},
	},
	{
		.gpio      = 142,
		.settings = {
			[GPIOMUX_SUSPENDED] = EBI2_A_D,
		},
	},
	{
		.gpio      = 143,
		.settings = {
			[GPIOMUX_SUSPENDED] = EBI2_A_D,
		},
	},
	{
		.gpio      = 144,
		.settings = {
			[GPIOMUX_SUSPENDED] = EBI2_A_D,
		},
	},
	{
		.gpio      = 145,
		.settings = {
			[GPIOMUX_SUSPENDED] = EBI2_A_D,
		},
	},
	{
		.gpio      = 146,
		.settings = {
			[GPIOMUX_SUSPENDED] = EBI2_A_D,
		},
	},
	{
		.gpio      = 147,
		.settings = {
			[GPIOMUX_SUSPENDED] = EBI2_A_D,
		},
	},
	{
		.gpio      = 148,
		.settings = {
			[GPIOMUX_SUSPENDED] = EBI2_A_D,
		},
	},
	{
		.gpio      = 149,
		.settings = {
			[GPIOMUX_SUSPENDED] = EBI2_A_D,
		},
	},
	{
		.gpio      = 150,
		.settings = {
			[GPIOMUX_SUSPENDED] = EBI2_A_D,
		},
	},
	{
		.gpio      = 151,
		.settings = {
			[GPIOMUX_SUSPENDED] = EBI2_OE,
		},
	},
	{
		.gpio      = 153,
		.settings = {
			[GPIOMUX_SUSPENDED] = EBI2_ADV,
		},
	},
	{
		.gpio      = 157,
		.settings = {
			[GPIOMUX_SUSPENDED] = EBI2_WE,
		},
	},
};

static struct msm_gpiomux_config msm8x60_uart_configs[] __initdata = {
	{ /* UARTDM_TX */
		.gpio      = 53,
		.settings = {
			[GPIOMUX_ACTIVE]    = UART1DM_ACTIVE,
			[GPIOMUX_SUSPENDED] = UART1DM_SUSPENDED,
		},
	},
	{ /* UARTDM_RX */
		.gpio      = 54,
		.settings = {
			[GPIOMUX_ACTIVE]    = UART1DM_ACTIVE,
			[GPIOMUX_SUSPENDED] = UART1DM_SUSPENDED,
		},
	},
	{ /* UARTDM_CTS */
		.gpio      = 55,
		.settings = {
			[GPIOMUX_ACTIVE]    = UART1DM_ACTIVE,
			[GPIOMUX_SUSPENDED] = UART1DM_SUSPENDED,
		},
	},
	{ /* UARTDM_RFR */
		.gpio      = 56,
		.settings = {
			[GPIOMUX_ACTIVE]    = UART1DM_ACTIVE,
			[GPIOMUX_SUSPENDED] = UART1DM_SUSPENDED,
		},
	},
	{
		.gpio      = 115,
		.settings = {
			[GPIOMUX_SUSPENDED] = CONSOLE_UART,
		},
	},
	{
		.gpio      = 116,
		.settings = {
			[GPIOMUX_SUSPENDED] = CONSOLE_UART,
		},
	},
	{
		.gpio      = 117,
		.settings = {
			[GPIOMUX_SUSPENDED] = CONSOLE_UART,
		},
	},
	{
		.gpio      = 118,
		.settings = {
			[GPIOMUX_SUSPENDED] = CONSOLE_UART,
		},
	},
};

static struct msm_gpiomux_config msm8x60_ts_configs[] __initdata = {
	{
		/* TS_ATTN */
		.gpio = 58,
		.settings = {
			[GPIOMUX_SUSPENDED] =
				GPIOMUX_CFG(0, 0, GPIOMUX_PULL_DOWN),
		},
	},
};

static struct msm_gpiomux_config msm8x60_tmg200_configs[] __initdata = {
	{
		.gpio = 61,
		.settings = {
			[GPIOMUX_ACTIVE]    = GPIOMUX_CFG(GPIOMUX_FUNC_GPIO,
				GPIOMUX_DRV_2MA, GPIOMUX_PULL_NONE),
			[GPIOMUX_SUSPENDED] = GPIOMUX_CFG(0, 0,
				GPIOMUX_PULL_NONE),
		},
	},
};

static struct msm_gpiomux_config msm8x60_tma300_configs[] __initdata = {
	{
		.gpio = 61,
		.settings = {
			[GPIOMUX_ACTIVE]    = GPIOMUX_CFG(GPIOMUX_FUNC_GPIO,
				GPIOMUX_DRV_6MA, GPIOMUX_PULL_UP),
			[GPIOMUX_SUSPENDED] = GPIOMUX_CFG(0, 0,
				GPIOMUX_PULL_NONE),
		},
	},
};

static struct msm_gpiomux_config msm8x60_aux_pcm_configs[] __initdata = {
	{
		.gpio = 111,
		.settings = {
			[GPIOMUX_ACTIVE]    = AUX_PCM_ACTIVE_CONFIG,
			[GPIOMUX_SUSPENDED] = AUX_PCM_SUSPEND_CONFIG,
		},
	},
	{
		.gpio = 112,
		.settings = {
			[GPIOMUX_ACTIVE]    = AUX_PCM_ACTIVE_CONFIG,
			[GPIOMUX_SUSPENDED] = AUX_PCM_SUSPEND_CONFIG,
		},
	},
	{
		.gpio = 113,
		.settings = {
			[GPIOMUX_ACTIVE]    = AUX_PCM_ACTIVE_CONFIG,
			[GPIOMUX_SUSPENDED] = AUX_PCM_SUSPEND_CONFIG,
		},
	},
	{
		.gpio = 114,
		.settings = {
			[GPIOMUX_ACTIVE]    = AUX_PCM_ACTIVE_CONFIG,
			[GPIOMUX_SUSPENDED] = AUX_PCM_SUSPEND_CONFIG,
		},
	},
};

static struct msm_gpiomux_config msm8x60_sdc_configs[] __initdata = {
	/* SDCC1 data[0] */
	{
		.gpio = 159,
		.settings = {
			[GPIOMUX_ACTIVE]    = SDCC1_DAT_0_3_CMD_ACTV_CFG,
			[GPIOMUX_SUSPENDED] = SDCC1_SUSPEND_CONFIG,
		},
	},
	/* SDCC1 data[1] */
	{
		.gpio = 160,
		.settings = {
			[GPIOMUX_ACTIVE]    = SDCC1_DAT_0_3_CMD_ACTV_CFG,
			[GPIOMUX_SUSPENDED] = SDCC1_SUSPEND_CONFIG,
		},
	},
	/* SDCC1 data[2] */
	{
		.gpio = 161,
		.settings = {
			[GPIOMUX_ACTIVE]    = SDCC1_DAT_0_3_CMD_ACTV_CFG,
			[GPIOMUX_SUSPENDED] = SDCC1_SUSPEND_CONFIG,
		},
	},
	/* SDCC1 data[3] */
	{
		.gpio = 162,
		.settings = {
			[GPIOMUX_ACTIVE]    = SDCC1_DAT_0_3_CMD_ACTV_CFG,
			[GPIOMUX_SUSPENDED] = SDCC1_SUSPEND_CONFIG,
		},
	},
	/* SDCC1 data[4] */
	{
		.gpio = 163,
		.settings = {
			[GPIOMUX_ACTIVE]    = SDCC1_DAT_4_7_CMD_ACTV_CFG,
			[GPIOMUX_SUSPENDED] = SDCC1_SUSPEND_CONFIG,
		},
	},
	/* SDCC1 data[5] */
	{
		.gpio = 164,
		.settings = {
			[GPIOMUX_ACTIVE]    = SDCC1_DAT_4_7_CMD_ACTV_CFG,
			[GPIOMUX_SUSPENDED] = SDCC1_SUSPEND_CONFIG,
		},
	},
	/* SDCC1 data[6] */
	{
		.gpio = 165,
		.settings = {
			[GPIOMUX_ACTIVE]    = SDCC1_DAT_4_7_CMD_ACTV_CFG,
			[GPIOMUX_SUSPENDED] = SDCC1_SUSPEND_CONFIG,
		},
	},
	/* SDCC1 data[7] */
	{
		.gpio = 166,
		.settings = {
			[GPIOMUX_ACTIVE]    = SDCC1_DAT_4_7_CMD_ACTV_CFG,
			[GPIOMUX_SUSPENDED] = SDCC1_SUSPEND_CONFIG,
		},
	},
	/* SDCC1 CLK */
	{
		.gpio = 167,
		.settings = {
			[GPIOMUX_ACTIVE]    = SDCC1_CLK_ACTV_CFG,
			[GPIOMUX_SUSPENDED] = SDCC1_SUSPEND_CONFIG,
		},
	},
	/* SDCC1 CMD */
	{
		.gpio = 168,
		.settings = {
			[GPIOMUX_ACTIVE]    = SDCC1_DAT_0_3_CMD_ACTV_CFG,
			[GPIOMUX_SUSPENDED] = SDCC1_SUSPEND_CONFIG,
		},
	},
};

static struct msm_gpiomux_config msm_qrdc_sdc_configs[] __initdata = {
	{
		.gpio      = 118,
		.settings = {
			[GPIOMUX_ACTIVE]    = WLAN_PWDN_N_ACTV_CFG,
			[GPIOMUX_SUSPENDED] = WLAN_PWDN_N_SUSP_CFG,
		},
	},
};


static struct msm_gpiomux_config msm8x60_charm_sdc_configs[] __initdata = {
	/* SDCC5 cmd */
	{
		.gpio = 95,
		.settings = {
			[GPIOMUX_ACTIVE]    = SDCC5_DAT_0_3_CMD_ACTV_CFG,
			[GPIOMUX_SUSPENDED] = SDCC5_SUSPEND_CONFIG,
		},
	},
	/* SDCC5 data[3]*/
	{
		.gpio = 96,
		.settings = {
			[GPIOMUX_ACTIVE]    = SDCC5_DAT_0_3_CMD_ACTV_CFG,
			[GPIOMUX_SUSPENDED] = SDCC5_SUSPEND_CONFIG,
		},
	},
	/* SDCC5 clk */
	{
		.gpio = 97,
		.settings = {
			[GPIOMUX_ACTIVE]    = SDCC5_CLK_ACTV_CFG,
			[GPIOMUX_SUSPENDED] = SDCC5_SUSPEND_CONFIG,
		},
	},
	/* SDCC5 data[2]*/
	{
		.gpio = 98,
		.settings = {
			[GPIOMUX_ACTIVE]    = SDCC5_DAT_0_3_CMD_ACTV_CFG,
			[GPIOMUX_SUSPENDED] = SDCC5_SUSPEND_CONFIG,
		},
	},
	/* SDCC5 data[1]*/
	{
		.gpio = 99,
		.settings = {
			[GPIOMUX_ACTIVE]    = SDCC5_DAT_0_3_CMD_ACTV_CFG,
			[GPIOMUX_SUSPENDED] = SDCC5_SUSPEND_CONFIG,
		},
	},
	/* SDCC5 data[0]*/
	{
		.gpio = 100,
		.settings = {
			[GPIOMUX_ACTIVE]    = SDCC5_DAT_0_3_CMD_ACTV_CFG,
			[GPIOMUX_SUSPENDED] = SDCC5_SUSPEND_CONFIG,
		},
	},
	/* MDM2AP_SYNC */
	{
		.gpio = 129,
		.settings = {
			[GPIOMUX_ACTIVE]    = MDM2AP_SYNC_ACTIVE_CFG,
			[GPIOMUX_SUSPENDED] = MDM2AP_SYNC_SUSPEND_CFG,
		},
	},
	/* SDCC2 data[0] */
	{
		.gpio = 143,
		.settings = {
			[GPIOMUX_ACTIVE]    = SDCC2_DAT_0_3_CMD_ACTV_CFG,
			[GPIOMUX_SUSPENDED] = SDCC2_SUSPEND_CONFIG,
		},
	},
	/* SDCC2 data[1] */
	{
		.gpio = 144,
		.settings = {
			[GPIOMUX_ACTIVE]    = SDCC2_DAT_0_3_CMD_ACTV_CFG,
			[GPIOMUX_SUSPENDED] = SDCC2_SUSPEND_CONFIG,
		},
	},
	/* SDCC2 data[2] */
	{
		.gpio = 145,
		.settings = {
			[GPIOMUX_ACTIVE]    = SDCC2_DAT_0_3_CMD_ACTV_CFG,
			[GPIOMUX_SUSPENDED] = SDCC2_SUSPEND_CONFIG,
		},
	},
	/* SDCC2 data[3] */
	{
		.gpio = 146,
		.settings = {
			[GPIOMUX_ACTIVE]    = SDCC2_DAT_0_3_CMD_ACTV_CFG,
			[GPIOMUX_SUSPENDED] = SDCC2_SUSPEND_CONFIG,
		},
	},
	/* SDCC2 data[4] */
	{
		.gpio = 147,
		.settings = {
			[GPIOMUX_ACTIVE]    = SDCC2_DAT_4_7_CMD_ACTV_CFG,
			[GPIOMUX_SUSPENDED] = SDCC2_SUSPEND_CONFIG,
		},
	},
	/* SDCC2 data[5] */
	{
		.gpio = 148,
		.settings = {
			[GPIOMUX_ACTIVE]    = SDCC2_DAT_4_7_CMD_ACTV_CFG,
			[GPIOMUX_SUSPENDED] = SDCC2_SUSPEND_CONFIG,
		},
	},
	/* SDCC2 data[6] */
	{
		.gpio = 149,
		.settings = {
			[GPIOMUX_ACTIVE]    = SDCC2_DAT_4_7_CMD_ACTV_CFG,
			[GPIOMUX_SUSPENDED] = SDCC2_SUSPEND_CONFIG,
		},
	},
	/* SDCC2 data[7] */
	{
		.gpio = 150,
		.settings = {
			[GPIOMUX_ACTIVE]    = SDCC2_DAT_4_7_CMD_ACTV_CFG,
			[GPIOMUX_SUSPENDED] = SDCC2_SUSPEND_CONFIG,
		},
	},
	/* SDCC2 CMD */
	{
		.gpio = 151,
		.settings = {
			[GPIOMUX_ACTIVE]    = SDCC2_DAT_0_3_CMD_ACTV_CFG,
			[GPIOMUX_SUSPENDED] = SDCC2_SUSPEND_CONFIG,
		},
	},

	/* SDCC2 CLK */
	{
		.gpio = 152,
		.settings = {
			[GPIOMUX_ACTIVE]    = SDCC2_CLK_ACTV_CFG,
			[GPIOMUX_SUSPENDED] = SDCC2_SUSPEND_CONFIG,
		},
	},
};

static struct msm_gpiomux_config msm_qrdc_usb_configs[] __initdata = {
	{
		.gpio      = 34,
		.settings = {
			[GPIOMUX_ACTIVE]    = USB_HUB_RESET_ACTV_CFG,
			[GPIOMUX_SUSPENDED] = USB_HUB_RESET_SUSP_CFG,
		},
	},
	{
		.gpio      = 131,
		.settings = {
			[GPIOMUX_ACTIVE]    = USB_SWITCH_CNTL_ACTV_CFG,
			[GPIOMUX_SUSPENDED] = USB_SWITCH_CNTL_SUSP_CFG,
		},
	},
	{
		.gpio      = 132,
		.settings = {
			[GPIOMUX_ACTIVE]    = USB_SWITCH_EN_ACTV_CFG,
			[GPIOMUX_SUSPENDED] = USB_SWITCH_EN_SUSP_CFG,
		},
	},
};

static struct msm_gpiomux_config msm8x60_snd_configs[] __initdata = {
	{
		.gpio = 108,
		.settings = {
			[GPIOMUX_ACTIVE]    = MSM_SNDDEV_ACTIVE_CONFIG,
			[GPIOMUX_SUSPENDED] = MSM_SNDDEV_SUSPEND_CONFIG,
		},
	},
	{
		.gpio = 109,
		.settings = {
			[GPIOMUX_ACTIVE]    = MSM_SNDDEV_ACTIVE_CONFIG,
			[GPIOMUX_SUSPENDED] = MSM_SNDDEV_SUSPEND_CONFIG,
		},
	},
};

static struct msm_gpiomux_config msm8x60_mi2s_configs[] __initdata = {
	/* MI2S WS */
	{
		.gpio = 101,
		.settings = {
			[GPIOMUX_ACTIVE]    = MI2S_ACTIVE_CFG,
			[GPIOMUX_SUSPENDED] = MI2S_SUSPEND_CFG,
		},
	},
	/* MI2S SCLK */
	{
		.gpio = 102,
		.settings = {
			[GPIOMUX_ACTIVE]    = MI2S_ACTIVE_CFG,
			[GPIOMUX_SUSPENDED] = MI2S_SUSPEND_CFG,
		},
	},
	/* MI2S MCLK */
	{
		.gpio = 103,
		.settings = {
			[GPIOMUX_ACTIVE]    = MI2S_ACTIVE_CFG,
			[GPIOMUX_SUSPENDED] = MI2S_SUSPEND_CFG,
		},
	},
	/* MI2S SD3 */
	{
		.gpio = 107,
		.settings = {
			[GPIOMUX_ACTIVE]    = MI2S_ACTIVE_CFG,
			[GPIOMUX_SUSPENDED] = MI2S_SUSPEND_CFG,
		},
	},
};

static struct msm_gpiomux_config msm8x60_lcdc_configs[] __initdata = {
	/* lcdc_pclk */
	{
		.gpio = 0,
		.settings = {
			[GPIOMUX_ACTIVE]    = LCDC_ACTIVE_CFG,
			[GPIOMUX_SUSPENDED] = LCDC_SUSPEND_CFG,
		},
	},
	/* lcdc_hsync */
	{
		.gpio = 1,
		.settings = {
			[GPIOMUX_ACTIVE]    = LCDC_ACTIVE_CFG,
			[GPIOMUX_SUSPENDED] = LCDC_SUSPEND_CFG,
		},
	},
	/* lcdc_vsync */
	{
		.gpio = 2,
		.settings = {
			[GPIOMUX_ACTIVE]    = LCDC_ACTIVE_CFG,
			[GPIOMUX_SUSPENDED] = LCDC_SUSPEND_CFG,
		},
	},
	/* lcdc_den */
	{
		.gpio = 3,
		.settings = {
			[GPIOMUX_ACTIVE]    = LCDC_ACTIVE_CFG,
			[GPIOMUX_SUSPENDED] = LCDC_SUSPEND_CFG,
		},
	},
	/* lcdc_red7 */
	{
		.gpio = 4,
		.settings = {
			[GPIOMUX_ACTIVE]    = LCDC_ACTIVE_CFG,
			[GPIOMUX_SUSPENDED] = LCDC_SUSPEND_CFG,
		},
	},
	/* lcdc_red6 */
	{
		.gpio = 5,
		.settings = {
			[GPIOMUX_ACTIVE]    = LCDC_ACTIVE_CFG,
			[GPIOMUX_SUSPENDED] = LCDC_SUSPEND_CFG,
		},
	},
	/* lcdc_red5 */
	{
		.gpio = 6,
		.settings = {
			[GPIOMUX_ACTIVE]    = LCDC_ACTIVE_CFG,
			[GPIOMUX_SUSPENDED] = LCDC_SUSPEND_CFG,
		},
	},
	/* lcdc_red4 */
	{
		.gpio = 7,
		.settings = {
			[GPIOMUX_ACTIVE]    = LCDC_ACTIVE_CFG,
			[GPIOMUX_SUSPENDED] = LCDC_SUSPEND_CFG,
		},
	},
	/* lcdc_red3 */
	{
		.gpio = 8,
		.settings = {
			[GPIOMUX_ACTIVE]    = LCDC_ACTIVE_CFG,
			[GPIOMUX_SUSPENDED] = LCDC_SUSPEND_CFG,
		},
	},
	/* lcdc_red2 */
	{
		.gpio = 9,
		.settings = {
			[GPIOMUX_ACTIVE]    = LCDC_ACTIVE_CFG,
			[GPIOMUX_SUSPENDED] = LCDC_SUSPEND_CFG,
		},
	},
	/* lcdc_red1 */
	{
		.gpio = 10,
		.settings = {
			[GPIOMUX_ACTIVE]    = LCDC_ACTIVE_CFG,
			[GPIOMUX_SUSPENDED] = LCDC_SUSPEND_CFG,
		},
	},
	/* lcdc_red0 */
	{
		.gpio = 11,
		.settings = {
			[GPIOMUX_ACTIVE]    = LCDC_ACTIVE_CFG,
			[GPIOMUX_SUSPENDED] = LCDC_SUSPEND_CFG,
		},
	},
	/* lcdc_grn7 */
	{
		.gpio = 12,
		.settings = {
			[GPIOMUX_ACTIVE]    = LCDC_ACTIVE_CFG,
			[GPIOMUX_SUSPENDED] = LCDC_SUSPEND_CFG,
		},
	},
	/* lcdc_grn6 */
	{
		.gpio = 13,
		.settings = {
			[GPIOMUX_ACTIVE]    = LCDC_ACTIVE_CFG,
			[GPIOMUX_SUSPENDED] = LCDC_SUSPEND_CFG,
		},
	},
	/* lcdc_grn5 */
	{
		.gpio = 14,
		.settings = {
			[GPIOMUX_ACTIVE]    = LCDC_ACTIVE_CFG,
			[GPIOMUX_SUSPENDED] = LCDC_SUSPEND_CFG,
		},
	},
	/* lcdc_grn4 */
	{
		.gpio = 15,
		.settings = {
			[GPIOMUX_ACTIVE]    = LCDC_ACTIVE_CFG,
			[GPIOMUX_SUSPENDED] = LCDC_SUSPEND_CFG,
		},
	},
	/* lcdc_grn3 */
	{
		.gpio = 16,
		.settings = {
			[GPIOMUX_ACTIVE]    = LCDC_ACTIVE_CFG,
			[GPIOMUX_SUSPENDED] = LCDC_SUSPEND_CFG,
		},
	},
	/* lcdc_grn2 */
	{
		.gpio = 17,
		.settings = {
			[GPIOMUX_ACTIVE]    = LCDC_ACTIVE_CFG,
			[GPIOMUX_SUSPENDED] = LCDC_SUSPEND_CFG,
		},
	},
	/* lcdc_grn1 */
	{
		.gpio = 18,
		.settings = {
			[GPIOMUX_ACTIVE]    = LCDC_ACTIVE_CFG,
			[GPIOMUX_SUSPENDED] = LCDC_SUSPEND_CFG,
		},
	},
	/* lcdc_grn0 */
	{
		.gpio = 19,
		.settings = {
			[GPIOMUX_ACTIVE]    = LCDC_ACTIVE_CFG,
			[GPIOMUX_SUSPENDED] = LCDC_SUSPEND_CFG,
		},
	},
	/* lcdc_blu7 */
	{
		.gpio = 20,
		.settings = {
			[GPIOMUX_ACTIVE]    = LCDC_ACTIVE_CFG,
			[GPIOMUX_SUSPENDED] = LCDC_SUSPEND_CFG,
		},
	},
	/* lcdc_blu6 */
	{
		.gpio = 21,
		.settings = {
			[GPIOMUX_ACTIVE]    = LCDC_ACTIVE_CFG,
			[GPIOMUX_SUSPENDED] = LCDC_SUSPEND_CFG,
		},
	},
	/* lcdc_blu5 */
	{
		.gpio = 22,
		.settings = {
			[GPIOMUX_ACTIVE]    = LCDC_ACTIVE_CFG,
			[GPIOMUX_SUSPENDED] = LCDC_SUSPEND_CFG,
		},
	},
	/* lcdc_blu4 */
	{
		.gpio = 23,
		.settings = {
			[GPIOMUX_ACTIVE]    = LCDC_ACTIVE_CFG,
			[GPIOMUX_SUSPENDED] = LCDC_SUSPEND_CFG,
		},
	},
	/* lcdc_blu3 */
	{
		.gpio = 24,
		.settings = {
			[GPIOMUX_ACTIVE]    = LCDC_ACTIVE_CFG,
			[GPIOMUX_SUSPENDED] = LCDC_SUSPEND_CFG,
		},
	},
	/* lcdc_blu2 */
	{
		.gpio = 25,
		.settings = {
			[GPIOMUX_ACTIVE]    = LCDC_ACTIVE_CFG,
			[GPIOMUX_SUSPENDED] = LCDC_SUSPEND_CFG,
		},
	},
	/* lcdc_blu1 */
	{
		.gpio = 26,
		.settings = {
			[GPIOMUX_ACTIVE]    = LCDC_ACTIVE_CFG,
			[GPIOMUX_SUSPENDED] = LCDC_SUSPEND_CFG,
		},
	},
	/* lcdc_blu0 */
	{
		.gpio = 27,
		.settings = {
			[GPIOMUX_ACTIVE]    = LCDC_ACTIVE_CFG,
			[GPIOMUX_SUSPENDED] = LCDC_SUSPEND_CFG,
		},
	},
};

static struct msm_gpiomux_config msm8x60_hdmi_configs[] __initdata = {
	{
		.gpio = 169,
		.settings = {
			[GPIOMUX_ACTIVE]    = GPIOMUX_CFG(GPIOMUX_FUNC_1,
				0, GPIOMUX_PULL_UP),
			[GPIOMUX_SUSPENDED] = HDMI_SUSPEND_CFG,
		},
	},
	{
		.gpio = 170,
		.settings = {
			[GPIOMUX_ACTIVE]    = GPIOMUX_CFG(GPIOMUX_FUNC_1,
				GPIOMUX_DRV_16MA, 0),
			[GPIOMUX_SUSPENDED] = HDMI_SUSPEND_CFG,
		},
	},
	{
		.gpio = 171,
		.settings = {
			[GPIOMUX_ACTIVE]    = GPIOMUX_CFG(GPIOMUX_FUNC_1,
				GPIOMUX_DRV_16MA, 0),
			[GPIOMUX_SUSPENDED] = HDMI_SUSPEND_CFG,
		},
	},
	{
		.gpio = 172,
		.settings = {
			[GPIOMUX_ACTIVE]    = GPIOMUX_CFG(GPIOMUX_FUNC_1,
				0, GPIOMUX_PULL_DOWN),
			[GPIOMUX_SUSPENDED] = HDMI_SUSPEND_CFG,
		},
	},
};

/* Because PMIC drivers do not use gpio-management routines and PMIC
 * gpios must never sleep, a "good enough" config is obtained by placing
 * the active config in the 'suspended' slot and leaving the active
 * config invalid: the suspended config will be installed at boot
 * and never replaced.
 */
static struct msm_gpiomux_config msm8x60_pmic_configs[] __initdata = {
	{
		.gpio = 88,
		.settings = {
			[GPIOMUX_SUSPENDED] = GPIOMUX_CFG(GPIOMUX_FUNC_GPIO,
				GPIOMUX_DRV_2MA, GPIOMUX_PULL_NONE),
		},
	},
	{
		.gpio = 91,
		.settings = {
			[GPIOMUX_SUSPENDED] = GPIOMUX_CFG(GPIOMUX_FUNC_GPIO,
				GPIOMUX_DRV_2MA, GPIOMUX_PULL_NONE),
		},
	},
};


static struct msm_gpiomux_config msm8x60_common_configs[] __initdata = {
	/* MDM2AP_STATUS */
	{
		.gpio = 77,
		.settings = {
			[GPIOMUX_ACTIVE]    = MDM2AP_STATUS_ACTIVE_CFG,
			[GPIOMUX_SUSPENDED] = MDM2AP_STATUS_SUSPEND_CFG,
		},
	},
};

static struct msm_gpiomux_config msm8x60_cam_configs[] __initdata = {
	{
		.gpio = 29,
		.settings = {
			[GPIOMUX_ACTIVE]    = GPIOMUX_CFG(GPIOMUX_FUNC_1,
				GPIOMUX_DRV_2MA, GPIOMUX_PULL_NONE),
			[GPIOMUX_SUSPENDED] = CAM_SUSPEND_CFG,
		},
	},
	{
		.gpio = 30,
		.settings = {
			[GPIOMUX_ACTIVE]    = GPIOMUX_CFG(GPIOMUX_FUNC_GPIO,
				GPIOMUX_DRV_2MA, GPIOMUX_PULL_NONE),
			[GPIOMUX_SUSPENDED] = CAM_SUSPEND_CFG,
		},
	},
	{
		.gpio = 31,
		.settings = {
			[GPIOMUX_ACTIVE]    = GPIOMUX_CFG(GPIOMUX_FUNC_GPIO,
				GPIOMUX_DRV_2MA, GPIOMUX_PULL_NONE),
			[GPIOMUX_SUSPENDED] = CAM_SUSPEND_CFG,
		},
	},
	{
		.gpio = 32,
		.settings = {
			[GPIOMUX_ACTIVE]    = GPIOMUX_CFG(GPIOMUX_FUNC_1,
				GPIOMUX_DRV_2MA, GPIOMUX_PULL_NONE),
			[GPIOMUX_SUSPENDED] = CAM_SUSPEND_CFG,
		},
	},
	{
		.gpio = 47,
		.settings = {
			[GPIOMUX_ACTIVE]    = GPIOMUX_CFG(GPIOMUX_FUNC_1,
				GPIOMUX_DRV_8MA, GPIOMUX_PULL_UP),
			[GPIOMUX_SUSPENDED] = CAM_SUSPEND_CFG,
		},
	},
	{
		.gpio = 48,
		.settings = {
			[GPIOMUX_ACTIVE]    = GPIOMUX_CFG(GPIOMUX_FUNC_1,
				GPIOMUX_DRV_8MA, GPIOMUX_PULL_UP),
			[GPIOMUX_SUSPENDED] = CAM_SUSPEND_CFG,
		},
	},
	{
		.gpio = 105,
		.settings = {
			[GPIOMUX_ACTIVE]    = GPIOMUX_CFG(GPIOMUX_FUNC_GPIO,
				GPIOMUX_DRV_2MA, GPIOMUX_PULL_NONE),
			[GPIOMUX_SUSPENDED] = CAM_SUSPEND_CFG,
		},
	},
	{
		.gpio = 106,
		.settings = {
			[GPIOMUX_ACTIVE]    = GPIOMUX_CFG(GPIOMUX_FUNC_GPIO,
				GPIOMUX_DRV_2MA, GPIOMUX_PULL_NONE),
			[GPIOMUX_SUSPENDED] = CAM_SUSPEND_CFG,
		},
	},
};

struct msm_gpiomux_configs
msm8x60_surf_ffa_gpiomux_cfgs[] __initdata = {
	{msm8x60_gsbi_configs, ARRAY_SIZE(msm8x60_gsbi_configs)},
	{msm8x60_ebi2_configs, ARRAY_SIZE(msm8x60_ebi2_configs)},
	{msm8x60_uart_configs, ARRAY_SIZE(msm8x60_uart_configs)},
	{msm8x60_ts_configs, ARRAY_SIZE(msm8x60_ts_configs)},
	{msm8x60_aux_pcm_configs, ARRAY_SIZE(msm8x60_aux_pcm_configs)},
	{msm8x60_sdc_configs, ARRAY_SIZE(msm8x60_sdc_configs)},
	{msm8x60_snd_configs, ARRAY_SIZE(msm8x60_snd_configs)},
	{msm8x60_mi2s_configs, ARRAY_SIZE(msm8x60_mi2s_configs)},
	{msm8x60_lcdc_configs, ARRAY_SIZE(msm8x60_lcdc_configs)},
	{msm8x60_hdmi_configs, ARRAY_SIZE(msm8x60_hdmi_configs)},
	{msm8x60_pmic_configs, ARRAY_SIZE(msm8x60_pmic_configs)},
	{msm8x60_common_configs, ARRAY_SIZE(msm8x60_common_configs)},
	{msm8x60_cam_configs, ARRAY_SIZE(msm8x60_cam_configs)},
	{msm8x60_tmg200_configs, ARRAY_SIZE(msm8x60_tmg200_configs)},
	{NULL, 0},
};

struct msm_gpiomux_configs
msm8x60_fluid_gpiomux_cfgs[] __initdata = {
	{msm8x60_gsbi_configs, ARRAY_SIZE(msm8x60_gsbi_configs)},
	{msm8x60_ebi2_configs, ARRAY_SIZE(msm8x60_ebi2_configs)},
	{msm8x60_uart_configs, ARRAY_SIZE(msm8x60_uart_configs)},
	{msm8x60_ts_configs, ARRAY_SIZE(msm8x60_ts_configs)},
	{msm8x60_aux_pcm_configs, ARRAY_SIZE(msm8x60_aux_pcm_configs)},
	{msm8x60_sdc_configs, ARRAY_SIZE(msm8x60_sdc_configs)},
	{msm8x60_snd_configs, ARRAY_SIZE(msm8x60_snd_configs)},
	{msm8x60_mi2s_configs, ARRAY_SIZE(msm8x60_mi2s_configs)},
	{msm8x60_lcdc_configs, ARRAY_SIZE(msm8x60_lcdc_configs)},
	{msm8x60_hdmi_configs, ARRAY_SIZE(msm8x60_hdmi_configs)},
	{msm8x60_pmic_configs, ARRAY_SIZE(msm8x60_pmic_configs)},
	{msm8x60_common_configs, ARRAY_SIZE(msm8x60_common_configs)},
	{msm8x60_cam_configs, ARRAY_SIZE(msm8x60_cam_configs)},
	{msm8x60_tma300_configs, ARRAY_SIZE(msm8x60_tma300_configs)},
	{NULL, 0},
};

struct msm_gpiomux_configs
msm8x60_qrdc_gpiomux_cfgs[] __initdata = {
	{msm8x60_gsbi_configs, ARRAY_SIZE(msm8x60_gsbi_configs)},
	{msm8x60_ebi2_configs, ARRAY_SIZE(msm8x60_ebi2_configs)},
	{msm8x60_uart_configs, ARRAY_SIZE(msm8x60_uart_configs)},
	{msm8x60_ts_configs, ARRAY_SIZE(msm8x60_ts_configs)},
	{msm8x60_aux_pcm_configs, ARRAY_SIZE(msm8x60_aux_pcm_configs)},
	{msm8x60_sdc_configs, ARRAY_SIZE(msm8x60_sdc_configs)},
	{msm8x60_snd_configs, ARRAY_SIZE(msm8x60_snd_configs)},
	{msm8x60_mi2s_configs, ARRAY_SIZE(msm8x60_mi2s_configs)},
	{msm8x60_lcdc_configs, ARRAY_SIZE(msm8x60_lcdc_configs)},
	{msm8x60_hdmi_configs, ARRAY_SIZE(msm8x60_hdmi_configs)},
	{msm8x60_pmic_configs, ARRAY_SIZE(msm8x60_pmic_configs)},
	{msm8x60_common_configs, ARRAY_SIZE(msm8x60_common_configs)},
	{msm8x60_cam_configs, ARRAY_SIZE(msm8x60_cam_configs)},
	{msm_qrdc_usb_configs, ARRAY_SIZE(msm_qrdc_usb_configs)},
	{msm_qrdc_sdc_configs, ARRAY_SIZE(msm_qrdc_sdc_configs)},
	{NULL, 0},
};

struct msm_gpiomux_configs
msm8x60_charm_gpiomux_cfgs[] __initdata = {
	{msm8x60_gsbi_configs, ARRAY_SIZE(msm8x60_gsbi_configs)},
	{msm8x60_uart_configs, ARRAY_SIZE(msm8x60_uart_configs)},
	{msm8x60_ts_configs, ARRAY_SIZE(msm8x60_ts_configs)},
	{msm8x60_aux_pcm_configs, ARRAY_SIZE(msm8x60_aux_pcm_configs)},
	{msm8x60_sdc_configs, ARRAY_SIZE(msm8x60_sdc_configs)},
	{msm8x60_snd_configs, ARRAY_SIZE(msm8x60_snd_configs)},
	{msm8x60_mi2s_configs, ARRAY_SIZE(msm8x60_mi2s_configs)},
	{msm8x60_lcdc_configs, ARRAY_SIZE(msm8x60_lcdc_configs)},
	{msm8x60_hdmi_configs, ARRAY_SIZE(msm8x60_hdmi_configs)},
	{msm8x60_pmic_configs, ARRAY_SIZE(msm8x60_pmic_configs)},
	{msm8x60_common_configs, ARRAY_SIZE(msm8x60_common_configs)},
	{msm8x60_cam_configs, ARRAY_SIZE(msm8x60_cam_configs)},
	{msm8x60_tmg200_configs, ARRAY_SIZE(msm8x60_tmg200_configs)},
	{msm8x60_charm_sdc_configs, ARRAY_SIZE(msm8x60_charm_sdc_configs)},
	{NULL, 0},
};

void __init msm8x60_init_gpiomux(struct msm_gpiomux_configs *cfgs)
{
	int rc;

	rc = msm_gpiomux_init(NR_GPIO_IRQS);
	if (rc) {
		pr_err("%s failure: %d\n", __func__, rc);
		return;
	}

	while (cfgs->cfg) {
		msm_gpiomux_install(cfgs->cfg, cfgs->ncfg);
		++cfgs;
	}
}
