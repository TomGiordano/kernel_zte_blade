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
 *
 */
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/mfd/msm-adie-codec.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/debugfs.h>
#include <linux/wakelock.h>
#include <linux/pmic8058-othc.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>
#include <asm/uaccess.h>
#include <mach/qdsp6v2/audio_dev_ctl.h>
#include <mach/qdsp6v2/apr_audio.h>
#include <mach/vreg.h>
#include <mach/pmic.h>
#include <mach/debug_mm.h>
#include "q6afe.h"
#include "snddev_icodec.h"

#define SNDDEV_ICODEC_PCM_SZ 32 /* 16 bit / sample stereo mode */
#define SNDDEV_ICODEC_MUL_FACTOR 3 /* Multi by 8 Shift by 3  */
#define SNDDEV_ICODEC_CLK_RATE(freq) \
	(((freq) * (SNDDEV_ICODEC_PCM_SZ)) << (SNDDEV_ICODEC_MUL_FACTOR))
#define SNDDEV_LOW_POWER_MODE 0
#define SNDDEV_HIGH_POWER_MODE 1
/* Voltage required for S4 in microVolts, 2.2V or 2200000microvolts */
#define SNDDEV_VREG_8058_S4_VOLTAGE (2200000)
/* Load Current required for S4 in microAmps,
   36mA - 56mA */
#define SNDDEV_VREG_LOW_POWER_LOAD (36000)
#define SNDDEV_VREG_HIGH_POWER_LOAD (56000)

#ifdef CONFIG_DEBUG_FS
static struct adie_codec_action_unit debug_rx_actions[] = {
	{ ADIE_CODEC_ACTION_STAGE_REACHED, ADIE_CODEC_DIGITAL_OFF},
	{ ADIE_CODEC_ACTION_DELAY_WAIT, 0xbb8},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x80, 0x02, 0x02)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x80, 0x02, 0x00)},
	{ ADIE_CODEC_ACTION_STAGE_REACHED, ADIE_CODEC_DIGITAL_READY },
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x24, 0x6F, 0x44)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x04, 0x5F, 0xBC)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x81, 0xFF, 0x4E)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x25, 0x0F, 0x0E)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x26, 0xfc, 0xfc)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x36, 0xc0, 0x80)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x3A, 0xFF, 0x2B)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x3d, 0xFF, 0xD5)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x83, 0x21, 0x21)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x33, 0x80, 0x80)},
	{ ADIE_CODEC_ACTION_DELAY_WAIT,  0x2710},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x33, 0x40, 0x40)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x84, 0xff, 0x00)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x8A, 0x05, 0x04)},
	{ ADIE_CODEC_ACTION_STAGE_REACHED, ADIE_CODEC_DIGITAL_ANALOG_READY},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x8a, 0x01, 0x01)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x36, 0xc0, 0x00)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x33, 0x40, 0x00)},
	{ ADIE_CODEC_ACTION_STAGE_REACHED,  ADIE_CODEC_ANALOG_OFF},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x33, 0x80, 0x00)}
};

static struct adie_codec_action_unit debug_tx_lb_actions[] = {
	{ ADIE_CODEC_ACTION_STAGE_REACHED, ADIE_CODEC_DIGITAL_OFF },
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x80, 0x01, 0x01)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x80, 0x01, 0x00) },
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x8A, 0x30, 0x30)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x11, 0xfc, 0xfc)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x13, 0xfc, 0x58)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x14, 0xff, 0x65)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x15, 0xff, 0x64)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x82, 0xff, 0x5C)},
	{ ADIE_CODEC_ACTION_STAGE_REACHED, ADIE_CODEC_DIGITAL_READY },
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x0D, 0xF0, 0xd0)},
	{ ADIE_CODEC_ACTION_DELAY_WAIT, 0xbb8},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x83, 0x14, 0x14)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x86, 0xff, 0x00)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x8A, 0x50, 0x40)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x91, 0xFF, 0x01)}, /* Start loop back */
	{ ADIE_CODEC_ACTION_STAGE_REACHED, ADIE_CODEC_DIGITAL_ANALOG_READY},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x8A, 0x10, 0x30)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x0D, 0xFF, 0x00)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x83, 0x14, 0x00)},
	{ ADIE_CODEC_ACTION_STAGE_REACHED, ADIE_CODEC_ANALOG_OFF},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x11, 0xff, 0x00)}
};

static struct adie_codec_action_unit debug_tx_actions[] = {
	{ ADIE_CODEC_ACTION_STAGE_REACHED, ADIE_CODEC_DIGITAL_OFF },
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x80, 0x01, 0x01)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x80, 0x01, 0x00) },
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x8A, 0x30, 0x30)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x11, 0xfc, 0xfc)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x13, 0xfc, 0x58)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x14, 0xff, 0x65)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x15, 0xff, 0x64)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x82, 0xff, 0x5C)},
	{ ADIE_CODEC_ACTION_STAGE_REACHED, ADIE_CODEC_DIGITAL_READY },
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x0D, 0xF0, 0xd0)},
	{ ADIE_CODEC_ACTION_DELAY_WAIT, 0xbb8},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x83, 0x14, 0x14)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x86, 0xff, 0x00)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x8A, 0x50, 0x40)},
	{ ADIE_CODEC_ACTION_STAGE_REACHED, ADIE_CODEC_DIGITAL_ANALOG_READY},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x8A, 0x10, 0x30)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x0D, 0xFF, 0x00)},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x83, 0x14, 0x00)},
	{ ADIE_CODEC_ACTION_STAGE_REACHED, ADIE_CODEC_ANALOG_OFF},
	{ ADIE_CODEC_ACTION_ENTRY,
	ADIE_CODEC_PACK_ENTRY(0x11, 0xff, 0x00)}
};

static struct adie_codec_hwsetting_entry debug_rx_settings[] = {
	{
		.freq_plan = 8000,
		.osr = 256,
		.actions = debug_rx_actions,
		.action_sz = ARRAY_SIZE(debug_rx_actions),
	}
};

static struct adie_codec_hwsetting_entry debug_tx_lb_settings[] = {
	{
		.freq_plan = 8000,
		.osr = 256,
		.actions = debug_tx_lb_actions,
		.action_sz = ARRAY_SIZE(debug_tx_lb_actions),
	}
};

static struct adie_codec_hwsetting_entry debug_tx_settings[] = {
	{
		.freq_plan = 8000,
		.osr = 256,
		.actions = debug_tx_actions,
		.action_sz = ARRAY_SIZE(debug_tx_actions),
	}
};

static struct adie_codec_dev_profile debug_rx_profile = {
	.path_type = ADIE_CODEC_RX,
	.settings = debug_rx_settings,
	.setting_sz = ARRAY_SIZE(debug_rx_settings),
};

static struct adie_codec_dev_profile debug_tx_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = debug_tx_settings,
	.setting_sz = ARRAY_SIZE(debug_tx_settings),
};

static struct adie_codec_dev_profile debug_tx_lb_profile = {
	.path_type = ADIE_CODEC_TX,
	.settings = debug_tx_lb_settings,
	.setting_sz = ARRAY_SIZE(debug_tx_lb_settings),
};
#endif /* CONFIG_DEBUG_FS */

/* Context for each internal codec sound device */
struct snddev_icodec_state {
	struct snddev_icodec_data *data;
	struct adie_codec_path *adie_path;
	u32 sample_rate;
	u32 enabled;
};

/* Global state for the driver */
struct snddev_icodec_drv_state {
	struct mutex rx_lock;
	struct mutex tx_lock;
	u32 rx_active; /* ensure one rx device at a time */
	u32 tx_active; /* ensure one tx device at a time */
	struct clk *rx_osrclk;
	struct clk *rx_bitclk;
	struct clk *tx_osrclk;
	struct clk *tx_bitclk;

	struct wake_lock rx_idlelock;
	struct wake_lock tx_idlelock;

	/* handle to pmic8058 regulator smps4 */
	struct regulator *snddev_vreg;
};

static struct snddev_icodec_drv_state snddev_icodec_drv;

struct regulator *vreg_init(void)
{
	int rc;
	struct regulator *vreg_ptr;

	vreg_ptr = regulator_get(NULL, "8058_s4");
	if (IS_ERR(vreg_ptr)) {
		pr_err("%s: regulator_get 8058_s4 failed\n", __func__);
		return NULL;
	}

	rc = regulator_set_voltage(vreg_ptr, SNDDEV_VREG_8058_S4_VOLTAGE,
				SNDDEV_VREG_8058_S4_VOLTAGE);
	if (rc == 0)
		return vreg_ptr;
	else
		return NULL;
}

static void vreg_deinit(struct regulator *vreg)
{
	regulator_put(vreg);
}

static void vreg_mode_vote(struct regulator *vreg, int enable, int mode)
{
	int rc;
	if (enable) {
		rc = regulator_enable(vreg);
		if (rc != 0)
			pr_err("%s:Enabling regulator failed\n", __func__);
		else {
			if (mode)
				regulator_set_optimum_mode(vreg,
						SNDDEV_VREG_HIGH_POWER_LOAD);
			else
				regulator_set_optimum_mode(vreg,
						SNDDEV_VREG_LOW_POWER_LOAD);
		}
	} else {
		rc = regulator_disable(vreg);
		if (rc != 0)
			pr_err("%s:Disabling regulator failed\n", __func__);
	}
}

struct msm_cdcclk_ctl_state {
	unsigned int rx_mclk;
	unsigned int rx_mclk_requested;
	unsigned int tx_mclk;
	unsigned int tx_mclk_requested;
};

static struct msm_cdcclk_ctl_state the_msm_cdcclk_ctl_state;

static int msm_snddev_rx_mclk_request(void)
{
	int rc = 0;

	rc = gpio_request(the_msm_cdcclk_ctl_state.rx_mclk,
		"MSM_SNDDEV_RX_MCLK");
	if (rc < 0) {
		pr_err("%s: GPIO request for MSM SNDDEV RX failed\n", __func__);
		return rc;
	}
	the_msm_cdcclk_ctl_state.rx_mclk_requested = 1;
	return rc;
}
static int msm_snddev_tx_mclk_request(void)
{
	int rc = 0;

	rc = gpio_request(the_msm_cdcclk_ctl_state.tx_mclk,
		"MSM_SNDDEV_TX_MCLK");
	if (rc < 0) {
		pr_err("%s: GPIO request for MSM SNDDEV TX failed\n", __func__);
		return rc;
	}
	the_msm_cdcclk_ctl_state.tx_mclk_requested = 1;
	return rc;
}
static void msm_snddev_rx_mclk_free(void)
{
	if (the_msm_cdcclk_ctl_state.rx_mclk_requested) {
		gpio_free(the_msm_cdcclk_ctl_state.rx_mclk);
		the_msm_cdcclk_ctl_state.rx_mclk_requested = 0;
	}
}
static void msm_snddev_tx_mclk_free(void)
{
	if (the_msm_cdcclk_ctl_state.tx_mclk_requested) {
		gpio_free(the_msm_cdcclk_ctl_state.tx_mclk);
		the_msm_cdcclk_ctl_state.tx_mclk_requested = 0;
	}
}
static int get_msm_cdcclk_ctl_gpios(struct platform_device *pdev)
{
	int rc = 0;
	struct resource *res;

	/* Claim all of the GPIOs. */
	res = platform_get_resource_byname(pdev, IORESOURCE_IO,
			"msm_snddev_rx_mclk");
	if (!res) {
		pr_err("%s: failed to get gpio MSM SNDDEV RX\n", __func__);
		return -ENODEV;
	}
	the_msm_cdcclk_ctl_state.rx_mclk = res->start;
	the_msm_cdcclk_ctl_state.rx_mclk_requested = 0;

	res = platform_get_resource_byname(pdev, IORESOURCE_IO,
			"msm_snddev_tx_mclk");
	if (!res) {
		pr_err("%s: failed to get gpio MSM SNDDEV TX\n", __func__);
		return -ENODEV;
	}
	the_msm_cdcclk_ctl_state.tx_mclk = res->start;
	the_msm_cdcclk_ctl_state.tx_mclk_requested = 0;

	return rc;
}
static int msm_cdcclk_ctl_probe(struct platform_device *pdev)
{
	int rc = 0;

	pr_info("%s:\n", __func__);

	rc = get_msm_cdcclk_ctl_gpios(pdev);
	if (rc < 0) {
		pr_err("%s: GPIO configuration failed\n", __func__);
		return -ENODEV;
	}
	return rc;
}
static struct platform_driver msm_cdcclk_ctl_driver = {
	.probe = msm_cdcclk_ctl_probe,
	.driver = { .name = "msm_cdcclk_ctl"}
};
static int snddev_icodec_open_rx(struct snddev_icodec_state *icodec)
{
	int trc;
	int afe_channel_mode;
	union afe_port_config afe_config;
	struct snddev_icodec_drv_state *drv = &snddev_icodec_drv;

	wake_lock(&drv->rx_idlelock);

	if (drv->snddev_vreg) {
		if (!strcmp(icodec->data->name, "headset_stereo_rx"))
			vreg_mode_vote(drv->snddev_vreg, 1,
					SNDDEV_LOW_POWER_MODE);
		else
			vreg_mode_vote(drv->snddev_vreg, 1,
					SNDDEV_HIGH_POWER_MODE);
	}
	msm_snddev_rx_mclk_request();

	drv->rx_osrclk = clk_get(0, "i2s_spkr_osr_clk");
	if (IS_ERR(drv->rx_osrclk))
		pr_err("%s master clock Error\n", __func__);

	trc =  clk_set_rate(drv->rx_osrclk,
			SNDDEV_ICODEC_CLK_RATE(icodec->sample_rate));
	if (IS_ERR_VALUE(trc)) {
		pr_err("ERROR setting m clock1\n");
		goto error_invalid_freq;
	}

	clk_enable(drv->rx_osrclk);
	drv->rx_bitclk = clk_get(0, "i2s_spkr_bit_clk");
	if (IS_ERR(drv->rx_bitclk))
		pr_err("%s clock Error\n", __func__);

	/* Master clock = Sample Rate * OSR rate bit clock
	 * OSR Rate bit clock = bit/sample * channel master
	 * clock / bit clock = divider value = 8
	 */
	trc =  clk_set_rate(drv->rx_bitclk, 8);
	if (IS_ERR_VALUE(trc)) {
		pr_err("ERROR setting m clock1\n");
		goto error_adie;
	}
	clk_enable(drv->rx_bitclk);

	if (icodec->data->voltage_on)
		icodec->data->voltage_on();

	/* Configure ADIE */
	trc = adie_codec_open(icodec->data->profile, &icodec->adie_path);
	if (IS_ERR_VALUE(trc))
		pr_err("%s: adie codec open failed\n", __func__);
	else
		adie_codec_setpath(icodec->adie_path,
					icodec->sample_rate, 256);
	/* OSR default to 256, can be changed for power optimization
	 * If OSR is to be changed, need clock API for setting the divider
	 */

	switch (icodec->data->channel_mode) {
	case 2:
		afe_channel_mode = MSM_AFE_STEREO;
		break;
	case 1:
	default:
		afe_channel_mode = MSM_AFE_MONO;
		break;
	}
	afe_config.mi2s.channel = afe_channel_mode;
	afe_config.mi2s.bitwidth = 16;
	afe_config.mi2s.line = 1;
	afe_config.mi2s.ws = 1;
	trc = afe_open(icodec->data->copp_id, &afe_config, icodec->sample_rate);

	/* Enable ADIE */
	if (icodec->adie_path) {
		adie_codec_proceed_stage(icodec->adie_path,
					ADIE_CODEC_DIGITAL_READY);
		adie_codec_proceed_stage(icodec->adie_path,
					ADIE_CODEC_DIGITAL_ANALOG_READY);
	}

	/* Enable power amplifier */
	if (icodec->data->pamp_on)
		icodec->data->pamp_on();

	icodec->enabled = 1;

	wake_unlock(&drv->rx_idlelock);
	return 0;

error_adie:
	clk_disable(drv->rx_osrclk);
error_invalid_freq:

	pr_err("%s: encounter error\n", __func__);

	wake_unlock(&drv->rx_idlelock);
	return -ENODEV;
}

static int snddev_icodec_open_tx(struct snddev_icodec_state *icodec)
{
	int trc;
	int afe_channel_mode;
	union afe_port_config afe_config;
	struct snddev_icodec_drv_state *drv = &snddev_icodec_drv;;

	wake_lock(&drv->tx_idlelock);

	if (drv->snddev_vreg)
		vreg_mode_vote(drv->snddev_vreg, 1, SNDDEV_HIGH_POWER_MODE);

	/* Reuse pamp_on for TX platform-specific setup  */
	if (icodec->data->pamp_on)
		icodec->data->pamp_on();

	msm_snddev_tx_mclk_request();

	drv->tx_osrclk = clk_get(0, "i2s_mic_osr_clk");
	if (IS_ERR(drv->tx_osrclk))
		pr_err("%s master clock Error\n", __func__);

	trc =  clk_set_rate(drv->tx_osrclk,
			SNDDEV_ICODEC_CLK_RATE(icodec->sample_rate));
	if (IS_ERR_VALUE(trc)) {
		pr_err("ERROR setting m clock1\n");
		goto error_invalid_freq;
	}

	clk_enable(drv->tx_osrclk);
	drv->tx_bitclk = clk_get(0, "i2s_mic_bit_clk");
	if (IS_ERR(drv->tx_bitclk))
		pr_err("%s clock Error\n", __func__);

	/* Master clock = Sample Rate * OSR rate bit clock
	 * OSR Rate bit clock = bit/sample * channel master
	 * clock / bit clock = divider value = 8
	 */
	trc =  clk_set_rate(drv->tx_bitclk, 8);
	clk_enable(drv->tx_bitclk);

	/* Enable ADIE */
	trc = adie_codec_open(icodec->data->profile, &icodec->adie_path);
	if (IS_ERR_VALUE(trc))
		pr_err("%s: adie codec open failed\n", __func__);
	else
		adie_codec_setpath(icodec->adie_path,
					icodec->sample_rate, 256);

	switch (icodec->data->channel_mode) {
	case 2:
		afe_channel_mode = MSM_AFE_STEREO;
		break;
	case 1:
	default:
		afe_channel_mode = MSM_AFE_MONO;
		break;
	}
	afe_config.mi2s.channel = afe_channel_mode;
	afe_config.mi2s.bitwidth = 16;
	afe_config.mi2s.line = 1;
	afe_config.mi2s.ws = 1;
	trc = afe_open(icodec->data->copp_id, &afe_config, icodec->sample_rate);

	if (icodec->adie_path) {
		adie_codec_proceed_stage(icodec->adie_path,
					ADIE_CODEC_DIGITAL_READY);
		adie_codec_proceed_stage(icodec->adie_path,
					ADIE_CODEC_DIGITAL_ANALOG_READY);
	}

	icodec->enabled = 1;

	wake_unlock(&drv->tx_idlelock);
	return 0;

error_invalid_freq:

	if (icodec->data->pamp_off)
		icodec->data->pamp_off();

	pr_err("%s: encounter error\n", __func__);

	wake_unlock(&drv->tx_idlelock);
	return -ENODEV;
}

static int snddev_icodec_close_rx(struct snddev_icodec_state *icodec)
{
	struct snddev_icodec_drv_state *drv = &snddev_icodec_drv;

	wake_lock(&drv->rx_idlelock);

	if (drv->snddev_vreg)
		vreg_mode_vote(drv->snddev_vreg, 0, SNDDEV_HIGH_POWER_MODE);

	/* Disable power amplifier */
	if (icodec->data->pamp_off)
		icodec->data->pamp_off();

	/* Disable ADIE */
	if (icodec->adie_path) {
		adie_codec_proceed_stage(icodec->adie_path,
			ADIE_CODEC_DIGITAL_OFF);
		adie_codec_close(icodec->adie_path);
		icodec->adie_path = NULL;
	}

	afe_close(icodec->data->copp_id);

	if (icodec->data->voltage_off)
		icodec->data->voltage_off();

	clk_disable(drv->rx_bitclk);
	clk_disable(drv->rx_osrclk);

	msm_snddev_rx_mclk_free();

	icodec->enabled = 0;

	wake_unlock(&drv->rx_idlelock);
	return 0;
}

static int snddev_icodec_close_tx(struct snddev_icodec_state *icodec)
{
	struct snddev_icodec_drv_state *drv = &snddev_icodec_drv;

	wake_lock(&drv->tx_idlelock);

	if (drv->snddev_vreg)
		vreg_mode_vote(drv->snddev_vreg, 0, SNDDEV_HIGH_POWER_MODE);

	/* Disable ADIE */
	if (icodec->adie_path) {
		adie_codec_proceed_stage(icodec->adie_path,
					ADIE_CODEC_DIGITAL_OFF);
		adie_codec_close(icodec->adie_path);
		icodec->adie_path = NULL;
	}

	afe_close(icodec->data->copp_id);

	clk_disable(drv->tx_bitclk);
	clk_disable(drv->tx_osrclk);

	msm_snddev_tx_mclk_free();

	/* Reuse pamp_off for TX platform-specific setup  */
	if (icodec->data->pamp_off)
		icodec->data->pamp_off();

	icodec->enabled = 0;

	wake_unlock(&drv->tx_idlelock);
	return 0;
}

static int snddev_icodec_set_device_volume_impl(
		struct msm_snddev_info *dev_info, u32 volume)
{
	struct snddev_icodec_state *icodec;

	int rc = 0;

	icodec = dev_info->private_data;

	if (icodec->data->dev_vol_type & SNDDEV_DEV_VOL_DIGITAL) {

		rc = adie_codec_set_device_digital_volume(icodec->adie_path,
				icodec->data->channel_mode, volume);
		if (rc < 0) {
			pr_err("%s: unable to set_device_digital_volume for"
				"%s volume in percentage = %u\n",
				__func__, dev_info->name, volume);
			return rc;
		}

	} else if (icodec->data->dev_vol_type & SNDDEV_DEV_VOL_ANALOG)
		rc = adie_codec_set_device_analog_volume(icodec->adie_path,
				icodec->data->channel_mode, volume);
		if (rc < 0) {
			pr_err("%s: unable to set_device_analog_volume for"
				"%s volume in percentage = %u\n",
				__func__, dev_info->name, volume);
			return rc;
		}
	else {
		pr_err("%s: Invalid device volume control\n", __func__);
		return -EPERM;
	}
}

static int snddev_icodec_open(struct msm_snddev_info *dev_info)
{
	int rc = 0;
	struct snddev_icodec_state *icodec;
	struct snddev_icodec_drv_state *drv = &snddev_icodec_drv;

	if (!dev_info) {
		rc = -EINVAL;
		goto error;
	}

	icodec = dev_info->private_data;

	if (icodec->data->capability & SNDDEV_CAP_RX) {
		mutex_lock(&drv->rx_lock);
		if (drv->rx_active) {
			mutex_unlock(&drv->rx_lock);
			rc = -EBUSY;
			goto error;
		}
		rc = snddev_icodec_open_rx(icodec);

		if (!IS_ERR_VALUE(rc)) {
			drv->rx_active = 1;
			if ((icodec->data->dev_vol_type & (
				SNDDEV_DEV_VOL_DIGITAL |
				SNDDEV_DEV_VOL_ANALOG)))
				rc = snddev_icodec_set_device_volume_impl(
						dev_info, dev_info->dev_volume);
		}
		mutex_unlock(&drv->rx_lock);
	} else {
		mutex_lock(&drv->tx_lock);
		if (drv->tx_active) {
			mutex_unlock(&drv->tx_lock);
			rc = -EBUSY;
			goto error;
		}
		rc = snddev_icodec_open_tx(icodec);

		if (!IS_ERR_VALUE(rc)) {
			drv->tx_active = 1;
			if ((icodec->data->dev_vol_type & (
				SNDDEV_DEV_VOL_DIGITAL |
				SNDDEV_DEV_VOL_ANALOG)))
				rc = snddev_icodec_set_device_volume_impl(
						dev_info, dev_info->dev_volume);
		}
		mutex_unlock(&drv->tx_lock);
	}
error:
	return rc;
}

static int snddev_icodec_close(struct msm_snddev_info *dev_info)
{
	int rc = 0;
	struct snddev_icodec_state *icodec;
	struct snddev_icodec_drv_state *drv = &snddev_icodec_drv;
	if (!dev_info) {
		rc = -EINVAL;
		goto error;
	}

	icodec = dev_info->private_data;

	if (icodec->data->capability & SNDDEV_CAP_RX) {
		mutex_lock(&drv->rx_lock);
		if (!drv->rx_active) {
			mutex_unlock(&drv->rx_lock);
			rc = -EPERM;
			goto error;
		}
		rc = snddev_icodec_close_rx(icodec);
		if (!IS_ERR_VALUE(rc))
			drv->rx_active = 0;
		mutex_unlock(&drv->rx_lock);
	} else {
		mutex_lock(&drv->tx_lock);
		if (!drv->tx_active) {
			mutex_unlock(&drv->tx_lock);
			rc = -EPERM;
			goto error;
		}
		rc = snddev_icodec_close_tx(icodec);
		if (!IS_ERR_VALUE(rc))
			drv->tx_active = 0;
		mutex_unlock(&drv->tx_lock);
	}

error:
	return rc;
}

static int snddev_icodec_check_freq(u32 req_freq)
{
	int rc = -EINVAL;

	if ((req_freq != 0) && (req_freq >= 8000) && (req_freq <= 48000)) {
		if ((req_freq == 8000) || (req_freq == 11025) ||
			(req_freq == 12000) || (req_freq == 16000) ||
			(req_freq == 22050) || (req_freq == 24000) ||
			(req_freq == 32000) || (req_freq == 44100) ||
			(req_freq == 48000)) {
				rc = 0;
		} else
			pr_info("%s: Unsupported Frequency:%d\n", __func__,
								req_freq);
	}
	return rc;
}

static int snddev_icodec_set_freq(struct msm_snddev_info *dev_info, u32 rate)
{
	int rc;
	struct snddev_icodec_state *icodec;

	if (!dev_info) {
		rc = -EINVAL;
		goto error;
	}

	icodec = dev_info->private_data;
	if (adie_codec_freq_supported(icodec->data->profile, rate) != 0) {
		rc = -EINVAL;
		goto error;
	} else {
		if (snddev_icodec_check_freq(rate) != 0) {
			rc = -EINVAL;
			goto error;
		} else
			icodec->sample_rate = rate;
	}

	if (icodec->enabled) {
		snddev_icodec_close(dev_info);
		snddev_icodec_open(dev_info);
	}

	return icodec->sample_rate;

error:
	return rc;
}

static int snddev_icodec_enable_sidetone(struct msm_snddev_info *dev_info,
	u32 enable, uint16_t gain)
{
	int rc = 0;
	struct snddev_icodec_state *icodec;
	struct snddev_icodec_drv_state *drv = &snddev_icodec_drv;

	if (!dev_info) {
		pr_err("invalid dev_info\n");
		rc = -EINVAL;
		goto error;
	}

	icodec = dev_info->private_data;

	if (icodec->data->capability & SNDDEV_CAP_RX) {
		mutex_lock(&drv->rx_lock);
		if (!drv->rx_active || !dev_info->opened) {
			pr_err("dev not active\n");
			rc = -EPERM;
			mutex_unlock(&drv->rx_lock);
			goto error;
		}
		rc = afe_sidetone(PRIMARY_I2S_TX, PRIMARY_I2S_RX, enable, gain);
		if (rc < 0)
			pr_err("%s: AFE command sidetone failed\n", __func__);
		mutex_unlock(&drv->rx_lock);
	} else {
		rc = -EINVAL;
		pr_err("rx device only\n");
	}

error:
	return rc;

}

int snddev_icodec_set_device_volume(struct msm_snddev_info *dev_info,
		u32 volume)
{
	struct snddev_icodec_state *icodec;
	struct mutex *lock;
	struct snddev_icodec_drv_state *drv = &snddev_icodec_drv;
	int rc = -EPERM;

	if (!dev_info) {
		pr_info("%s : device not intilized.\n", __func__);
		return  -EINVAL;
	}

	icodec = dev_info->private_data;

	if (!(icodec->data->dev_vol_type & (SNDDEV_DEV_VOL_DIGITAL
				| SNDDEV_DEV_VOL_ANALOG))) {

		pr_info("%s : device %s does not support device volume "
				"control.", __func__, dev_info->name);
		return -EPERM;
	}
	dev_info->dev_volume =  volume;

	if (icodec->data->capability & SNDDEV_CAP_RX)
		lock = &drv->rx_lock;
	else
		lock = &drv->tx_lock;

	mutex_lock(lock);

	rc = snddev_icodec_set_device_volume_impl(dev_info,
			dev_info->dev_volume);
	mutex_unlock(lock);
	return rc;
}

static int snddev_icodec_probe(struct platform_device *pdev)
{
	int rc = 0;
	struct snddev_icodec_data *pdata;
	struct msm_snddev_info *dev_info;
	struct snddev_icodec_state *icodec;

	if (!pdev || !pdev->dev.platform_data) {
		printk(KERN_ALERT "Invalid caller\n");
		rc = -1;
		goto error;
	}
	pdata = pdev->dev.platform_data;
	if ((pdata->capability & SNDDEV_CAP_RX) &&
	   (pdata->capability & SNDDEV_CAP_TX)) {
		pr_err("%s: invalid device data either RX or TX\n", __func__);
		goto error;
	}
	icodec = kzalloc(sizeof(struct snddev_icodec_state), GFP_KERNEL);
	if (!icodec) {
		rc = -ENOMEM;
		goto error;
	}
	dev_info = kmalloc(sizeof(struct msm_snddev_info), GFP_KERNEL);
	if (!dev_info) {
		kfree(icodec);
		rc = -ENOMEM;
		goto error;
	}

	dev_info->name = pdata->name;
	dev_info->copp_id = pdata->copp_id;
	dev_info->private_data = (void *) icodec;
	dev_info->dev_ops.open = snddev_icodec_open;
	dev_info->dev_ops.close = snddev_icodec_close;
	dev_info->dev_ops.set_freq = snddev_icodec_set_freq;
	dev_info->dev_ops.set_device_volume = snddev_icodec_set_device_volume;
	dev_info->capability = pdata->capability;
	dev_info->opened = 0;
	msm_snddev_register(dev_info);
	icodec->data = pdata;
	icodec->sample_rate = pdata->default_sample_rate;
	dev_info->sample_rate = pdata->default_sample_rate;
	dev_info->channel_mode = pdata->channel_mode;
	if (pdata->capability & SNDDEV_CAP_RX)
		dev_info->dev_ops.enable_sidetone =
			snddev_icodec_enable_sidetone;
	else
		dev_info->dev_ops.enable_sidetone = NULL;

error:
	return rc;
}

static int snddev_icodec_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver snddev_icodec_driver = {
  .probe = snddev_icodec_probe,
  .remove = snddev_icodec_remove,
  .driver = { .name = "snddev_icodec" }
};

#ifdef CONFIG_DEBUG_FS
static struct dentry *debugfs_sdev_dent;
static struct dentry *debugfs_afelb;
static struct dentry *debugfs_adielb;
static struct adie_codec_path *debugfs_rx_adie;
static struct adie_codec_path *debugfs_tx_adie;

static int snddev_icodec_debug_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	pr_info("snddev_icodec: debug intf %s\n", (char *) file->private_data);
	return 0;
}

static void debugfs_adie_loopback(u32 loop)
{
	struct snddev_icodec_drv_state *drv = &snddev_icodec_drv;
	int rc = 0;

	if (loop) {

		/* enable MI2S RX master block */
		/* enable MI2S RX bit clock */
		clk_enable(drv->rx_osrclk);
		clk_enable(drv->rx_bitclk);

		pr_info("%s: configure ADIE RX path\n", __func__);
		/* Configure ADIE */
		rc = adie_codec_open(&debug_rx_profile, &debugfs_rx_adie);
		if (IS_ERR_VALUE(rc))
			pr_err("%s: adie codec open failed\n", __func__);
		else {
			adie_codec_setpath(debugfs_rx_adie, 8000, 256);
			adie_codec_proceed_stage(debugfs_rx_adie,
					ADIE_CODEC_DIGITAL_ANALOG_READY);
		}

		pr_info("%s: Enable Handset Mic bias\n", __func__);
		clk_enable(drv->tx_osrclk);
		clk_enable(drv->tx_bitclk);

		pm8058_micbias_enable(OTHC_MICBIAS_0,
						OTHC_SIGNAL_ALWAYS_ON);
		pr_info("%s: configure ADIE TX path\n", __func__);
		/* Configure ADIE */
		rc = adie_codec_open(&debug_tx_lb_profile, &debugfs_tx_adie);
		if (IS_ERR_VALUE(rc))
			pr_err("%s: adie codec open failed\n", __func__);
		else {
			adie_codec_setpath(debugfs_tx_adie, 8000, 256);
			adie_codec_proceed_stage(debugfs_tx_adie,
					ADIE_CODEC_DIGITAL_ANALOG_READY);
		}
	} else {
		/* Disable ADIE */
		if (debugfs_rx_adie) {
			adie_codec_proceed_stage(debugfs_rx_adie,
						ADIE_CODEC_DIGITAL_OFF);
			adie_codec_close(debugfs_rx_adie);
		}
		if (debugfs_tx_adie) {
			adie_codec_proceed_stage(debugfs_tx_adie,
						ADIE_CODEC_DIGITAL_OFF);
			adie_codec_close(debugfs_tx_adie);
		}

		pm8058_micbias_enable(OTHC_MICBIAS_0,
						OTHC_SIGNAL_OFF);

		clk_disable(drv->rx_bitclk);
		clk_disable(drv->rx_osrclk);

		clk_disable(drv->tx_bitclk);
		clk_disable(drv->tx_osrclk);
	}
}

static void debugfs_afe_loopback(u32 loop)
{
	int trc;
	int sample_rate, channel_mode;
	union afe_port_config afe_config;
	struct snddev_icodec_drv_state *drv = &snddev_icodec_drv;

	if (loop) {

		/* enable MI2S RX master block */
		/* enable MI2S RX bit clock */
		clk_enable(drv->rx_osrclk);
		clk_enable(drv->rx_bitclk);
		pr_info("%s: configure ADIE RX path\n", __func__);
		/* Configure ADIE */
		trc = adie_codec_open(&debug_rx_profile, &debugfs_rx_adie);
		if (IS_ERR_VALUE(trc))
			pr_err("%s: adie codec open failed\n", __func__);
		else
			adie_codec_setpath(debugfs_rx_adie, 8000, 256);
		sample_rate = 8000;
		channel_mode = 3;  /* stereo */
		afe_config.mi2s.channel = channel_mode;
		afe_config.mi2s.bitwidth = 16;
		afe_config.mi2s.line = 1;
		afe_config.mi2s.ws = 1;
		trc = afe_open(PRIMARY_I2S_RX, &afe_config, sample_rate);
		if (!trc)
			adie_codec_proceed_stage(debugfs_rx_adie,
				ADIE_CODEC_DIGITAL_ANALOG_READY);

		pr_info("%s: Enable Handset Mic bias\n", __func__);
		/* enable MI2S TX master block */
		/* enable MI2S TX bit clock */
		clk_enable(drv->tx_osrclk);
		clk_enable(drv->tx_bitclk);
		pm8058_micbias_enable(OTHC_MICBIAS_0,
						OTHC_SIGNAL_ALWAYS_ON);
		pr_info("%s: configure ADIE TX path\n", __func__);
		/* Configure ADIE */
		trc = adie_codec_open(&debug_tx_profile, &debugfs_tx_adie);
		if (IS_ERR_VALUE(trc))
			pr_err("%s: adie codec open failed\n", __func__);
		else
			adie_codec_setpath(debugfs_tx_adie, 8000, 256);
		sample_rate = 8000;
		afe_config.mi2s.channel = channel_mode;
		afe_config.mi2s.bitwidth = 16;
		afe_config.mi2s.line = 1;
		afe_config.mi2s.ws = 1;
		trc = afe_open(PRIMARY_I2S_TX, &afe_config, sample_rate);
		if (!trc)
			adie_codec_proceed_stage(debugfs_tx_adie,
				ADIE_CODEC_DIGITAL_ANALOG_READY);
	} else {
		/* Disable ADIE */
		if (debugfs_rx_adie) {
			adie_codec_proceed_stage(debugfs_rx_adie,
					ADIE_CODEC_DIGITAL_OFF);
			adie_codec_close(debugfs_rx_adie);
		}
		if (debugfs_tx_adie) {
			adie_codec_proceed_stage(debugfs_tx_adie,
					ADIE_CODEC_DIGITAL_OFF);
			adie_codec_close(debugfs_tx_adie);
		}

		pm8058_micbias_enable(OTHC_MICBIAS_0,
						OTHC_SIGNAL_OFF);
		afe_close(PRIMARY_I2S_TX);
		afe_close(PRIMARY_I2S_RX);

		/* Disable MI2S RX master block */
		/* Disable MI2S RX bit clock */
		clk_disable(drv->rx_bitclk);
		clk_disable(drv->rx_osrclk);

		/* Disable MI2S TX master block */
		/* Disable MI2S TX bit clock */
		clk_disable(drv->tx_bitclk);
		clk_disable(drv->tx_osrclk);
	}
}

static ssize_t snddev_icodec_debug_write(struct file *filp,
	const char __user *ubuf, size_t cnt, loff_t *ppos)
{
	char *lb_str = filp->private_data;
	char cmd;

	if (get_user(cmd, ubuf))
		return -EFAULT;

	pr_info("%s: %s %c\n", __func__, lb_str, cmd);

	if (!strcmp(lb_str, "adie_loopback")) {
		switch (cmd) {
		case '1':
			debugfs_adie_loopback(1);
			break;
		case '0':
			debugfs_adie_loopback(0);
			break;
		}
	} else if (!strcmp(lb_str, "afe_loopback")) {
		switch (cmd) {
		case '1':
			debugfs_afe_loopback(1);
			break;
		case '0':
			debugfs_afe_loopback(0);
			break;
		}
	}

	return cnt;
}

static const struct file_operations snddev_icodec_debug_fops = {
	.open = snddev_icodec_debug_open,
	.write = snddev_icodec_debug_write
};
#endif

static int __init snddev_icodec_init(void)
{
	s32 rc;
	struct snddev_icodec_drv_state *icodec_drv = &snddev_icodec_drv;

	rc = platform_driver_register(&snddev_icodec_driver);
	if (IS_ERR_VALUE(rc)) {
		pr_err("%s: platform_driver_register for snddev icodec failed\n",
					__func__);
		goto error_snddev_icodec_driver;
	}

	rc = platform_driver_register(&msm_cdcclk_ctl_driver);
	if (IS_ERR_VALUE(rc)) {
		pr_err("%s: platform_driver_register for msm snddev failed\n",
					__func__);
		goto error_msm_cdcclk_ctl_driver;
	}

#ifdef CONFIG_DEBUG_FS
	debugfs_sdev_dent = debugfs_create_dir("snddev_icodec", 0);
	if (!IS_ERR(debugfs_sdev_dent)) {
		debugfs_afelb = debugfs_create_file("afe_loopback",
		S_IFREG | S_IRUGO, debugfs_sdev_dent,
		(void *) "afe_loopback", &snddev_icodec_debug_fops);
		debugfs_adielb = debugfs_create_file("adie_loopback",
		S_IFREG | S_IRUGO, debugfs_sdev_dent,
		(void *) "adie_loopback", &snddev_icodec_debug_fops);
	}
#endif
	mutex_init(&icodec_drv->rx_lock);
	mutex_init(&icodec_drv->tx_lock);
	icodec_drv->rx_active = 0;
	icodec_drv->tx_active = 0;
	icodec_drv->snddev_vreg = vreg_init();

	wake_lock_init(&icodec_drv->tx_idlelock, WAKE_LOCK_IDLE,
			"snddev_tx_idle");
	wake_lock_init(&icodec_drv->rx_idlelock, WAKE_LOCK_IDLE,
			"snddev_rx_idle");
	return 0;

error_msm_cdcclk_ctl_driver:
	platform_driver_unregister(&snddev_icodec_driver);
error_snddev_icodec_driver:
	return -ENODEV;
}

static void __exit snddev_icodec_exit(void)
{
	struct snddev_icodec_drv_state *icodec_drv = &snddev_icodec_drv;

#ifdef CONFIG_DEBUG_FS
	debugfs_remove(debugfs_afelb);
	debugfs_remove(debugfs_adielb);
	debugfs_remove(debugfs_sdev_dent);
#endif
	platform_driver_unregister(&snddev_icodec_driver);
	platform_driver_unregister(&msm_cdcclk_ctl_driver);

	clk_put(icodec_drv->rx_osrclk);
	clk_put(icodec_drv->tx_osrclk);
	if (icodec_drv->snddev_vreg) {
		vreg_deinit(icodec_drv->snddev_vreg);
		icodec_drv->snddev_vreg = NULL;
	}
	return;
}

module_init(snddev_icodec_init);
module_exit(snddev_icodec_exit);

MODULE_DESCRIPTION("ICodec Sound Device driver");
MODULE_VERSION("1.0");
MODULE_LICENSE("GPL v2");
