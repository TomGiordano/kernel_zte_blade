/*
 * =====================================================================================
 *
 *       Filename:  lcdc_panel_wvga_v9.c
 *
 *    Description:  V9 WVGA panel(800x480) driver 
 *
 *        Version:  1.0
 *        Created:  07/06/2010 
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Lu Ya
 *        Company:  ZTE Corp.
 *
 * =====================================================================================
 */
/* ========================================================================================
when         who        what, where, why                                  comment tag
--------     ----       -----------------------------                --------------------------
2011-03-22   luya    	modify LcdPanleID	CRDB00628094				 ZTE_LCD_LUYA_20110322_001
2010-07-16   luya    	support BKL adjust					 ZTE_LCD_LUYA_20100716_001
2010-07-09   luya    	modify clk_rate and h&v para		 ZTE_LCD_LUYA_20100709_001
==========================================================================================*/


#include "msm_fb.h"
#include <asm/gpio.h>
#include <linux/module.h>
#include <linux/delay.h>

#define GPIO_LCD_BL_SC_OUT 57

#define LCD_BL_LEVEL 32

static bool onewiremode = TRUE;

static int v9_reset;
static boolean is_firsttime = true;		///ZTE_LCD_LUYA_20091221_001
extern u32 LcdPanleID;    //ZTE_LCD_LHT_20100611_001

static struct msm_panel_common_pdata * lcdc_v9_pdata;
static void lcdc_set_bl(struct msm_fb_data_type *mfd);

static void reset_init(void);
static int lcdc_panel_on(struct platform_device *pdev);
static int lcdc_panel_off(struct platform_device *pdev);

static int lcdc_panel_on(struct platform_device *pdev)
{

	reset_init();
	return 0;
}

////ZTE_LCD_LUYA_20100716_001
static void select_1wire_mode(void)
{
	gpio_direction_output(GPIO_LCD_BL_SC_OUT, 1);
	udelay(120);
	gpio_direction_output(GPIO_LCD_BL_SC_OUT, 0);
	udelay(280);				////ZTE_LCD_LUYA_20100226_001
	gpio_direction_output(GPIO_LCD_BL_SC_OUT, 1);
	udelay(650);				////ZTE_LCD_LUYA_20100226_001
	
}

static void send_bkl_address(void)
{
	unsigned int i,j;
	i = 0x72;
	gpio_direction_output(GPIO_LCD_BL_SC_OUT, 1);
	udelay(10);
	printk("[LY] send_bkl_address \n");
	for(j = 0; j < 8; j++)
	{
		if(i & 0x80)
		{
			gpio_direction_output(GPIO_LCD_BL_SC_OUT, 0);
			udelay(10);
			gpio_direction_output(GPIO_LCD_BL_SC_OUT, 1);
			udelay(180);
		}
		else
		{
			gpio_direction_output(GPIO_LCD_BL_SC_OUT, 0);
			udelay(180);
			gpio_direction_output(GPIO_LCD_BL_SC_OUT, 1);
			udelay(10);
		}
		i <<= 1;
	}
	gpio_direction_output(GPIO_LCD_BL_SC_OUT, 0);
	udelay(10);
	gpio_direction_output(GPIO_LCD_BL_SC_OUT, 1);

}

static void send_bkl_data(int level)
{
	unsigned int i,j;
	i = level & 0x1F;
	gpio_direction_output(GPIO_LCD_BL_SC_OUT, 1);
	udelay(10);
	printk("[LY] send_bkl_data \n");
	for(j = 0; j < 8; j++)
	{
		if(i & 0x80)
		{
			gpio_direction_output(GPIO_LCD_BL_SC_OUT, 0);
			udelay(10);
			gpio_direction_output(GPIO_LCD_BL_SC_OUT, 1);
			udelay(180);
		}
		else
		{
			gpio_direction_output(GPIO_LCD_BL_SC_OUT, 0);
			udelay(180);
			gpio_direction_output(GPIO_LCD_BL_SC_OUT, 1);
			udelay(10);
		}
		i <<= 1;
	}
	gpio_direction_output(GPIO_LCD_BL_SC_OUT, 0);
	udelay(10);
	gpio_direction_output(GPIO_LCD_BL_SC_OUT, 1);

}


static void lcdc_set_bl(struct msm_fb_data_type *mfd)
{
       /*value range is 1--32*/
    int current_lel = mfd->bl_level;
    unsigned long flags;


    printk("[ZYF] lcdc_set_bl level=%d, %d\n", 
		   current_lel , mfd->panel_power_on);


    if(!mfd->panel_power_on)
	{
    	    gpio_direction_output(GPIO_LCD_BL_SC_OUT, 0);			///ZTE_LCD_LUYA_20100201_001
	    return;
    	}

    if(current_lel < 1)
    {
        current_lel = 0;
    }
    if(current_lel > 32)
    {
        current_lel = 32;
    }
    if(!onewiremode)	//select 1 wire modeZTE_LCD_LUYA_20100226_001
    {
	    printk("[LY] before msleep\n");
	    msleep(230);			////ZTE_LCD_LUYA_20101113_001
    }

    /*ZTE_BACKLIGHT_WLY_005,@2009-11-28, set backlight as 32 levels, end*/
    local_irq_save(flags);
    if(current_lel==0)
    	{
    		gpio_direction_output(GPIO_LCD_BL_SC_OUT, 0);
		mdelay(3);
		onewiremode = FALSE;
			
    	}
    else 
	{
		if(!onewiremode)	//select 1 wire mode
		{
			printk("[LY] before select_1wire_mode\n");
			select_1wire_mode();
			onewiremode = TRUE;
		}
		send_bkl_address();
		send_bkl_data(current_lel-1);

	}
    local_irq_restore(flags);
}

static void reset_init(void)
{
	v9_reset = *(lcdc_v9_pdata->gpio_num + 4);

	if(!is_firsttime)				/////ZTE_LCD_LUYA_20101027_001
	{

		/* reset lcd module */////ZTE_LCD_LUYA_20101113_001
//		gpio_direction_output(v9_reset, 1);
		mdelay(1);
//		gpio_direction_output(v9_reset, 0);
		mdelay(10);
		gpio_direction_output(v9_reset, 1);
		mdelay(3);
	}
	else
	{
		is_firsttime = false;
	}



}
static int lcdc_panel_off(struct platform_device *pdev)
{
	gpio_direction_output(v9_reset, 0);
	return 0;
}

static struct msm_fb_panel_data lcdc_v9_panel_data = {
       .panel_info = {.bl_max = 32},
	.on = lcdc_panel_on,
	.off = lcdc_panel_off,
       .set_backlight = lcdc_set_bl,
};

static struct platform_device this_device = {
	.name   = "lcdc_panel_qvga",
	.id	= 1,
	.dev	= {
		.platform_data = &lcdc_v9_panel_data,
	}
};

static int __init lcdc_panel_probe(struct platform_device *pdev)
{
	struct msm_panel_info *pinfo;
	int ret;

	if(pdev->id == 0) {
		lcdc_v9_pdata = pdev->dev.platform_data;
		lcdc_v9_pdata->panel_config_gpio(1);
	
	 	LcdPanleID=(u32)LCD_PANEL_V9_NT39416I;   //ZTE_LCD_LUYA_20110322_001
		pinfo = &lcdc_v9_panel_data.panel_info;
		pinfo->xres = 800;
		pinfo->yres = 480;
		pinfo->type = LCDC_PANEL;
		pinfo->pdest = DISPLAY_1;
		pinfo->wait_cycle = 0;
		pinfo->bpp = 18;
		pinfo->fb_num = 2;

		////ZTE_LCD_LUYA_20100709_001
		pinfo->clk_rate = 30720000;
		
		pinfo->lcdc.h_back_porch = 40;
		pinfo->lcdc.h_front_porch = 40;
		pinfo->lcdc.h_pulse_width = 48;
		pinfo->lcdc.v_back_porch = 31;
		pinfo->lcdc.v_front_porch = 13;
		pinfo->lcdc.v_pulse_width = 1;
		pinfo->lcdc.border_clr = 0;	/* blk */
		pinfo->lcdc.underflow_clr = 0xff;	/* blue */
		pinfo->lcdc.hsync_skew = 0;

    	ret = platform_device_register(&this_device);
		
		return 0;
	}
	msm_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = lcdc_panel_probe,
	.driver = {
		.name   = "lcdc_panel_qvga",
	},
};

static int __init lcdc_v9_panel_init(void)
{
	int ret;
	ret = platform_driver_register(&this_driver);
	return ret;
}

module_init(lcdc_v9_panel_init);

