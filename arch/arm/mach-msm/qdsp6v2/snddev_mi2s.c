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
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/debugfs.h>
#include <asm/uaccess.h>
#include <mach/board.h>
#include <mach/qdsp6v2/audio_dev_ctl.h>
#include <mach/qdsp6v2/apr_audio.h>
#include "q6afe.h"
#include "snddev_mi2s.h"

#define SNDDEV_MI2S_PCM_SZ 32 /* 16 bit / sample stereo mode */
#define SNDDEV_MI2S_MUL_FACTOR 3 /* Multi by 8 Shift by 3  */
#define SNDDEV_MI2S_CLK_RATE(freq) \
	(((freq) * (SNDDEV_MI2S_PCM_SZ)) << (SNDDEV_MI2S_MUL_FACTOR))


/* Global state for the driver */
struct snddev_mi2s_drv_state {

	struct clk *tx_osrclk;
	struct clk *tx_bitclk;
	int mi2s_ws;
	int mi2s_mclk;
	int mi2s_sclk;
	int fm_mi2s_sd;
};

static struct snddev_mi2s_drv_state snddev_mi2s_drv;

static struct msm_mi2s_gpio_data *mi2s_gpio;

static int mi2s_gpios_request(void)
{
	int rc = 0;

	pr_debug("%s\n", __func__);
	rc = gpio_request(snddev_mi2s_drv.mi2s_ws, "MI2S_WS");
	if (rc < 0) {
		pr_err("%s: GPIO request for MI2S_WS failed\n", __func__);
		return rc;
	}

	rc = gpio_request(snddev_mi2s_drv.mi2s_sclk, "MI2S_SCLK");
	if (rc < 0) {
		pr_err("%s: GPIO request for MI2S_SCLK failed\n", __func__);
		gpio_free(snddev_mi2s_drv.mi2s_sclk);
		return rc;
	}

	rc = gpio_request(snddev_mi2s_drv.mi2s_mclk, "MI2S_MCLK");
	if (rc < 0) {
		pr_err("%s: GPIO request for MI2S_MCLK failed\n",
			__func__);
		gpio_free(snddev_mi2s_drv.mi2s_ws);
		gpio_free(snddev_mi2s_drv.mi2s_sclk);
		return rc;
	}

	rc = gpio_request(snddev_mi2s_drv.fm_mi2s_sd, "FM_MI2S_SD");
	if (rc < 0) {
		pr_err("%s: GPIO request for FM_MI2S_SD failed\n",
			__func__);
		gpio_free(snddev_mi2s_drv.mi2s_ws);
		gpio_free(snddev_mi2s_drv.mi2s_sclk);
		gpio_free(snddev_mi2s_drv.mi2s_mclk);
		return rc;
	}

	return rc;
}

static void mi2s_gpios_free(void)
{
	pr_debug("%s\n", __func__);
	gpio_free(snddev_mi2s_drv.mi2s_ws);
	gpio_free(snddev_mi2s_drv.mi2s_sclk);
	gpio_free(snddev_mi2s_drv.mi2s_mclk);
	gpio_free(snddev_mi2s_drv.fm_mi2s_sd);
}

static int mi2s_get_gpios(struct platform_device *pdev)
{
	int rc = 0;
	struct resource *res;

	/* Claim all of the GPIOs. */
	res = platform_get_resource_byname(pdev, IORESOURCE_IO, "mi2s_ws");
	if (!res) {
		pr_err("%s: failed to get gpio MI2S_WS\n", __func__);
		return -ENODEV;
	}

	snddev_mi2s_drv.mi2s_ws = res->start;

	res = platform_get_resource_byname(pdev, IORESOURCE_IO, "mi2s_sclk");
	if (!res) {
		pr_err("%s: failed to get gpio MI2S_SCLK\n", __func__);
		return -ENODEV;
	}

	snddev_mi2s_drv.mi2s_sclk = res->start;

	res = platform_get_resource_byname(pdev, IORESOURCE_IO,
					   "mi2s_mclk");
	if (!res) {
		pr_err("%s: failed to get gpio MI2S_MCLK\n", __func__);
		return -ENODEV;
	}

	snddev_mi2s_drv.mi2s_mclk = res->start;

	res = platform_get_resource_byname(pdev, IORESOURCE_IO,
					   "fm_mi2s_sd");
	if (!res) {
		pr_err("%s: failed to get gpio FM_MI2S_SD\n", __func__);
		return -ENODEV;
	}

	snddev_mi2s_drv.fm_mi2s_sd = res->start;

	return rc;
}

static int mi2s_fm_probe(struct platform_device *pdev)
{
	int rc = 0;

	pr_info("%s:\n", __func__);

	rc = mi2s_get_gpios(pdev);
	if (rc < 0) {
		pr_err("%s: GPIO configuration failed\n", __func__);
		return rc;
	}

	mi2s_gpio = (struct msm_mi2s_gpio_data *)(pdev->dev.platform_data);
	return rc;
}

static struct platform_driver mi2s_fm_driver = {
	.probe = mi2s_fm_probe,
	.driver = { .name = "msm_mi2s"}
};

static int snddev_mi2s_open(struct msm_snddev_info *dev_info)
{
	int rc = 0;
	union afe_port_config afe_config;
	struct snddev_mi2s_drv_state *drv = &snddev_mi2s_drv;
	struct snddev_mi2s_data *snddev_mi2s_data = dev_info->private_data;

	if (!dev_info) {
		pr_err("%s:  msm_snddev_info is null\n", __func__);
		return -EINVAL;
	}

	/* set up osr clk */
	drv->tx_osrclk = clk_get(0, "mi2s_osr_clk");
	if (IS_ERR(drv->tx_osrclk))
		pr_err("%s master clock Error\n", __func__);

	rc =  clk_set_rate(drv->tx_osrclk,
			 SNDDEV_MI2S_CLK_RATE(dev_info->sample_rate));
	if (IS_ERR_VALUE(rc)) {
		pr_err("ERROR setting osr clock\n");
		return -ENODEV;
	}
	clk_enable(drv->tx_osrclk);

	/* set up bit clk */
	drv->tx_bitclk = clk_get(0, "mi2s_bit_clk");
	if (IS_ERR(drv->tx_bitclk))
		pr_err("%s clock Error\n", __func__);

	rc =  clk_set_rate(drv->tx_bitclk, 8);
	if (IS_ERR_VALUE(rc)) {
		pr_err("ERROR setting bit clock\n");
		clk_disable(drv->tx_osrclk);
		return -ENODEV;
	}
	clk_enable(drv->tx_bitclk);

	afe_config.mi2s.channel = snddev_mi2s_data->channel_mode;
	afe_config.mi2s.bitwidth = 16;
	afe_config.mi2s.line = 4;
	afe_config.mi2s.ws = 1;
	rc = afe_open(snddev_mi2s_data->copp_id, &afe_config,
		dev_info->sample_rate);

	if (rc < 0) {
		pr_err("%s:  afe_open failed\n", __func__);
		clk_disable(drv->tx_bitclk);
		clk_disable(drv->tx_osrclk);
		return -EINVAL;
	}

	/*enable fm gpio here*/
	rc = mi2s_gpios_request();
	if (rc < 0) {
		pr_err("%s: GPIO request failed\n", __func__);
		return rc;
	}

	pr_info("%s:  afe_open  done\n", __func__);

	return rc;
}

static int snddev_mi2s_close(struct msm_snddev_info *dev_info)
{

	struct snddev_mi2s_drv_state *mi2s_drv = &snddev_mi2s_drv;
	struct snddev_mi2s_data *snddev_mi2s_data = dev_info->private_data;

	if (!dev_info) {
		pr_err("%s:  msm_snddev_info is null\n", __func__);
		return -EINVAL;
	}

	if (!dev_info->opened) {
		pr_err(" %s: calling close device with out opening the"
		       " device\n", __func__);
		return -EIO;
	}
	afe_close(snddev_mi2s_data->copp_id);
	clk_disable(mi2s_drv->tx_bitclk);
	clk_disable(mi2s_drv->tx_osrclk);

	mi2s_gpios_free();

	pr_info("%s:\n", __func__);

	return 0;
}

static int snddev_mi2s_set_freq(struct msm_snddev_info *dev_info, u32 req_freq)
{
	if (req_freq != 48000) {
		pr_info("%s: Unsupported Frequency:%d\n", __func__, req_freq);
		return -EINVAL;
	}
	return 48000;
}


static int snddev_mi2s_probe(struct platform_device *pdev)
{
	int rc = 0;
	struct snddev_mi2s_data *pdata;
	struct msm_snddev_info *dev_info;

	if (!pdev || !pdev->dev.platform_data) {
		printk(KERN_ALERT "Invalid caller\n");
		return -ENODEV;
	}

	pdata = pdev->dev.platform_data;

	dev_info = kzalloc(sizeof(struct msm_snddev_info), GFP_KERNEL);
	if (!dev_info) {
		pr_err("%s: uneable to allocate memeory for msm_snddev_info\n",
		       __func__);

		return -ENOMEM;
	}

	dev_info->name = pdata->name;
	dev_info->copp_id = pdata->copp_id;
	dev_info->dev_ops.open = snddev_mi2s_open;
	dev_info->dev_ops.close = snddev_mi2s_close;
	dev_info->private_data = (void *)pdata;
	dev_info->dev_ops.set_freq = snddev_mi2s_set_freq;
	dev_info->capability = pdata->capability;
	dev_info->opened = 0;
	dev_info->sample_rate = pdata->sample_rate;
	msm_snddev_register(dev_info);

	pr_info("%s: probe done for %s\n", __func__, pdata->name);
	return rc;
}

static struct platform_driver snddev_mi2s_driver = {
	.probe = snddev_mi2s_probe,
	.driver = {.name = "snddev_mi2s"}
};

static int __init snddev_mi2s_init(void)
{
	s32 rc = 0;

	rc = platform_driver_register(&mi2s_fm_driver);
	if (IS_ERR_VALUE(rc)) {
		pr_err("%s: platform_driver_register for mi2s_fm_driver failed\n",
				__func__);
		goto error_mi2s_fm_platform_driver;
	}

	rc = platform_driver_register(&snddev_mi2s_driver);
	if (IS_ERR_VALUE(rc)) {

		pr_err("%s: platform_driver_register failed\n", __func__);
		goto error_platform_driver;
	}

	pr_info("snddev_mi2s_init : done\n");

	return rc;

error_platform_driver:
	platform_driver_unregister(&mi2s_fm_driver);
error_mi2s_fm_platform_driver:
	pr_err("%s: encounter error\n", __func__);
	return -ENODEV;
}

static void __exit snddev_mi2s_exit(void)
{
	struct snddev_mi2s_drv_state *mi2s_drv = &snddev_mi2s_drv;

	platform_driver_unregister(&snddev_mi2s_driver);
	clk_put(mi2s_drv->tx_osrclk);
	clk_put(mi2s_drv->tx_bitclk);
	return;
}


module_init(snddev_mi2s_init);
module_exit(snddev_mi2s_exit);

MODULE_DESCRIPTION("MI2S Sound Device driver");
MODULE_VERSION("1.0");
MODULE_LICENSE("GPL v2");
