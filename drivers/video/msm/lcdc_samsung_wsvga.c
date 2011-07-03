/* Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
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

#include <linux/delay.h>
#include <linux/pwm.h>
#ifdef CONFIG_PMIC8058_PWM
#include <linux/mfd/pmic8058.h>
#include <linux/pmic8058-pwm.h>
#endif
#include <mach/gpio.h>
#include "msm_fb.h"



#ifdef CONFIG_PMIC8058_PWM
static struct pwm_device *bl_pwm0;
static struct pwm_device *bl_pwm1;

/* for samsung panel 300hz was the minimum freq where flickering wasnt
 * observed as the screen was dimmed
 */

#define PWM_FREQ_HZ 300
#define PWM_PERIOD_USEC (USEC_PER_SEC / PWM_FREQ_HZ)
#define PWM_LEVEL 15
#define PWM_DUTY_LEVEL (PWM_PERIOD_USEC / PWM_LEVEL)
#endif

static struct msm_panel_common_pdata *lcdc_samsung_pdata;


static void lcdc_samsung_panel_set_backlight(struct msm_fb_data_type *mfd)
{
	int bl_level;
	int ret;

	bl_level = mfd->bl_level;

#ifdef CONFIG_PMIC8058_PWM
	if (bl_pwm0) {
		ret = pwm_config(bl_pwm0, PWM_DUTY_LEVEL * bl_level,
			PWM_PERIOD_USEC);
		if (ret)
			printk(KERN_ERR "pwm_config on pwm 0 failed %d\n", ret);
	}

	if (bl_pwm1) {
		ret = pwm_config(bl_pwm1,
			PWM_PERIOD_USEC - (PWM_DUTY_LEVEL * bl_level),
			PWM_PERIOD_USEC);
		if (ret)
			printk(KERN_ERR "pwm_config on pwm 1 failed %d\n", ret);
	}

	if (bl_pwm0) {
		ret = pwm_enable(bl_pwm0);
		if (ret)
			printk(KERN_ERR "pwm_enable on pwm 0 failed %d\n", ret);
	}

	if (bl_pwm1) {
		ret = pwm_enable(bl_pwm1);
		if (ret)
			printk(KERN_ERR "pwm_enable on pwm 1 failed %d\n", ret);
	}
#endif

}

static int __init samsung_probe(struct platform_device *pdev)
{
	if (pdev->id == 0) {
		lcdc_samsung_pdata = pdev->dev.platform_data;
		return 0;
	}

#ifdef CONFIG_PMIC8058_PWM
	bl_pwm0 = pwm_request(lcdc_samsung_pdata->gpio_num[0], "backlight1");
	if (bl_pwm0 == NULL || IS_ERR(bl_pwm0)) {
		pr_err("%s pwm_request() failed\n", __func__);
		bl_pwm0 = NULL;
	}

	bl_pwm1 = pwm_request(lcdc_samsung_pdata->gpio_num[1], "backlight2");
	if (bl_pwm1 == NULL || IS_ERR(bl_pwm1)) {
		pr_err("%s pwm_request() failed\n", __func__);
		bl_pwm1 = NULL;
	}

	printk(KERN_INFO "samsung_probe: bl_pwm0=%p LPG_chan0=%d "
			"bl_pwm1=%p LPG_chan1=%d\n",
			bl_pwm0, (int)lcdc_samsung_pdata->gpio_num[0],
			bl_pwm1, (int)lcdc_samsung_pdata->gpio_num[1]
			);
#endif

	msm_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = samsung_probe,
	.driver = {
		.name   = "lcdc_samsung_wsvga",
	},
};

static struct msm_fb_panel_data samsung_panel_data = {
	.set_backlight = lcdc_samsung_panel_set_backlight,
};

static struct platform_device this_device = {
	.name   = "lcdc_samsung_wsvga",
	.id	= 1,
	.dev	= {
		.platform_data = &samsung_panel_data,
	}
};

static int __init lcdc_samsung_panel_init(void)
{
	int ret;
	struct msm_panel_info *pinfo;

#ifdef CONFIG_FB_MSM_MDDI_AUTO_DETECT
	if (msm_fb_detect_client("lcdc_samsung_wsvga"))
		return 0;
#endif

	ret = platform_driver_register(&this_driver);
	if (ret)
		return ret;

	pinfo = &samsung_panel_data.panel_info;
	pinfo->xres = 1024;
	pinfo->yres = 600;
	pinfo->type = LCDC_PANEL;
	pinfo->pdest = DISPLAY_1;
	pinfo->wait_cycle = 0;
	pinfo->bpp = 18;
	pinfo->fb_num = 2;
	pinfo->clk_rate = 53990000;
	pinfo->bl_max = 15;
	pinfo->bl_min = 1;

	pinfo->lcdc.h_back_porch = 80;
	pinfo->lcdc.h_front_porch = 48;
	pinfo->lcdc.h_pulse_width = 32;
	pinfo->lcdc.v_back_porch = 4;
	pinfo->lcdc.v_front_porch = 3;
	pinfo->lcdc.v_pulse_width = 1;
	pinfo->lcdc.border_clr = 0;
	pinfo->lcdc.underflow_clr = 0xff;
	pinfo->lcdc.hsync_skew = 0;

	ret = platform_device_register(&this_device);
	if (ret)
		platform_driver_unregister(&this_driver);

	return ret;
}

module_init(lcdc_samsung_panel_init);
