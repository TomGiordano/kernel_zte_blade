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
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/pm_qos_params.h>
#include <asm/system.h>
#include <asm/mach-types.h>
#include <mach/hardware.h>
#include <mach/msm_reqs.h>

#include "msm_fb.h"

static int dtv_probe(struct platform_device *pdev);
static int dtv_remove(struct platform_device *pdev);

static int dtv_off(struct platform_device *pdev);
static int dtv_on(struct platform_device *pdev);

static struct platform_device *pdev_list[MSM_FB_MAX_DEV_LIST];
static int pdev_list_cnt;

static struct clk *tv_src_clk;
static struct clk *tv_enc_clk;
static struct clk *tv_dac_clk;
static struct clk *hdmi_clk;
static struct clk *mdp_tv_clk;

static struct platform_driver dtv_driver = {
	.probe = dtv_probe,
	.remove = dtv_remove,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = NULL,
	.driver = {
		   .name = "dtv",
		   },
};

static struct lcdc_platform_data *dtv_pdata;

static int dtv_off(struct platform_device *pdev)
{
	int ret = 0;

	ret = panel_next_off(pdev);

	clk_disable(tv_enc_clk);
	clk_disable(tv_dac_clk);
	clk_disable(hdmi_clk);
	if (mdp_tv_clk)
		clk_disable(mdp_tv_clk);

	if (dtv_pdata && dtv_pdata->lcdc_power_save)
		dtv_pdata->lcdc_power_save(0);

	if (dtv_pdata && dtv_pdata->lcdc_gpio_config)
		ret = dtv_pdata->lcdc_gpio_config(0);

	pm_qos_update_requirement(PM_QOS_SYSTEM_BUS_FREQ , "dtv",
					PM_QOS_DEFAULT_VALUE);

	return ret;
}

static int dtv_on(struct platform_device *pdev)
{
	int ret = 0;
	struct msm_fb_data_type *mfd;
	unsigned long panel_pixclock_freq , pm_qos_rate;

	mfd = platform_get_drvdata(pdev);
	panel_pixclock_freq = mfd->fbi->var.pixclock;

#ifdef CONFIG_MSM_NPA_SYSTEM_BUS
	pm_qos_rate = MSM_AXI_FLOW_MDP_DTV_720P_2BPP;
#else
	if (panel_pixclock_freq > 58000000)
		/* pm_qos_rate should be in Khz */
		pm_qos_rate = panel_pixclock_freq / 1000 ;
	else
		pm_qos_rate = 58000;
#endif

	pm_qos_update_requirement(PM_QOS_SYSTEM_BUS_FREQ , "dtv",
						pm_qos_rate);
	mfd = platform_get_drvdata(pdev);

	clk_set_rate(tv_src_clk, mfd->fbi->var.pixclock);

	clk_enable(tv_enc_clk);
	clk_enable(tv_dac_clk);
	clk_enable(hdmi_clk);
	if (mdp_tv_clk)
		clk_enable(mdp_tv_clk);

	if (dtv_pdata && dtv_pdata->lcdc_power_save)
		dtv_pdata->lcdc_power_save(1);
	if (dtv_pdata && dtv_pdata->lcdc_gpio_config)
		ret = dtv_pdata->lcdc_gpio_config(1);

	ret = panel_next_on(pdev);
	return ret;
}

static int dtv_probe(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct fb_info *fbi;
	struct platform_device *mdp_dev = NULL;
	struct msm_fb_panel_data *pdata = NULL;
	int rc;

	if (pdev->id == 0) {
		dtv_pdata = pdev->dev.platform_data;
		if (dtv_pdata && dtv_pdata->lcdc_power_save)
			dtv_pdata->lcdc_power_save(1);
		return 0;
	}

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;

	if (mfd->key != MFD_KEY)
		return -EINVAL;

	if (pdev_list_cnt >= MSM_FB_MAX_DEV_LIST)
		return -ENOMEM;

	mdp_dev = platform_device_alloc("mdp", pdev->id);
	if (!mdp_dev)
		return -ENOMEM;

	/*
	 * link to the latest pdev
	 */
	mfd->pdev = mdp_dev;
	mfd->dest = DISPLAY_LCDC;

	/*
	 * alloc panel device data
	 */
	if (platform_device_add_data
	    (mdp_dev, pdev->dev.platform_data,
	     sizeof(struct msm_fb_panel_data))) {
		printk(KERN_ERR "dtv_probe: platform_device_add_data failed!\n");
		platform_device_put(mdp_dev);
		return -ENOMEM;
	}
	/*
	 * data chain
	 */
	pdata = (struct msm_fb_panel_data *)mdp_dev->dev.platform_data;
	pdata->on = dtv_on;
	pdata->off = dtv_off;
	pdata->next = pdev;

	/*
	 * get/set panel specific fb info
	 */
	mfd->panel_info = pdata->panel_info;
	mfd->fb_imgType = MDP_RGB_565;

	fbi = mfd->fbi;
	fbi->var.pixclock = mfd->panel_info.clk_rate;
	fbi->var.left_margin = mfd->panel_info.lcdc.h_back_porch;
	fbi->var.right_margin = mfd->panel_info.lcdc.h_front_porch;
	fbi->var.upper_margin = mfd->panel_info.lcdc.v_back_porch;
	fbi->var.lower_margin = mfd->panel_info.lcdc.v_front_porch;
	fbi->var.hsync_len = mfd->panel_info.lcdc.h_pulse_width;
	fbi->var.vsync_len = mfd->panel_info.lcdc.v_pulse_width;

	/*
	 * set driver data
	 */
	platform_set_drvdata(mdp_dev, mfd);

	/*
	 * register in mdp driver
	 */
	rc = platform_device_add(mdp_dev);
	if (rc)
		goto dtv_probe_err;

	pdev_list[pdev_list_cnt++] = pdev;
		return 0;

dtv_probe_err:
	platform_device_put(mdp_dev);
	return rc;
}

static int dtv_remove(struct platform_device *pdev)
{
	pm_qos_remove_requirement(PM_QOS_SYSTEM_BUS_FREQ , "dtv");
	return 0;
}

static int dtv_register_driver(void)
{
	return platform_driver_register(&dtv_driver);
}

static int __init dtv_driver_init(void)
{
	tv_enc_clk = clk_get(NULL, "tv_enc_clk");
	if (IS_ERR(tv_enc_clk)) {
		printk(KERN_ERR "error: can't get tv_enc_clk!\n");
		return IS_ERR(tv_enc_clk);
	}

	tv_dac_clk = clk_get(NULL, "tv_dac_clk");
	if (IS_ERR(tv_dac_clk)) {
		printk(KERN_ERR "error: can't get tv_dac_clk!\n");
		return IS_ERR(tv_dac_clk);
	}

	tv_src_clk = clk_get(NULL, "tv_src_clk");
	if (IS_ERR(tv_src_clk))
		tv_src_clk = tv_enc_clk; /* Fallback to slave */

	hdmi_clk = clk_get(NULL, "hdmi_clk");
	if (IS_ERR(hdmi_clk)) {
		printk(KERN_ERR "error: can't get hdmi_clk!\n");
		return IS_ERR(hdmi_clk);
	}

	mdp_tv_clk = clk_get(NULL, "mdp_tv_clk");
	if (IS_ERR(mdp_tv_clk))
		mdp_tv_clk = NULL;

	pm_qos_add_requirement(PM_QOS_SYSTEM_BUS_FREQ , "dtv",
				PM_QOS_DEFAULT_VALUE);

	return dtv_register_driver();
}

module_init(dtv_driver_init);
