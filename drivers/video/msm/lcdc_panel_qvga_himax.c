/*
 * =====================================================================================
 *
 *       Filename:  lcdc_panel_qvga_himax.c
 *
 *    Description:  Himax QVGA panel(240x320) driver 
 *
 *        Version:  1.0
 *        Created:  11/12/2009 02:05:46 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Li Xiongwei
 *        Company:  ZTE Corp.
 *
 * =====================================================================================
 */
/* ========================================================================================
when         who        	what, where, why                             		comment tag
--------     ----       	-----------------------------                       --------------------------
2010-06-11   lht		   	project mode display panel info         			ZTE_LCD_LHT_20100611_001
2010-02-21   luya			merge samsung IC for QVGA &pull down spi when sleep ZTE_LCD_LUYA_20100221_001        
2010-01-16   luya			disable lcdc_himax_init for the 1st time  			ZTE_LCD_LUYA_20090116_001
2009-11-12   lixiongwei     merge new driver for himax qvga          			ZTE_LCD_LIXW_001
2009-12-26   luya			merge nopanel(QVGA)driver to avoid the 
								problem of bootup abnormally without lcd panel	ZTE_LCD_LUYA_20091226_001

==========================================================================================*/


#include "msm_fb.h"
#include <asm/gpio.h>
#include <linux/module.h>
#include <linux/delay.h>

#define GPIO_LCD_BL_SC_OUT 97
#define GPIO_LCD_BL_EN

#define LCD_BL_LEVEL 32
#define lcd_bl_max   lcd_bl_level-1
#define lcd_bl_min   0

static int lcdc_himax_regist = 1;
uint32 gpio_ic_himax;			//ZTE_LCD_LUYA_20091226_001
extern uint32 gpio_ic_lead;
static boolean is_firsttime = true;		///ZTE_LCD_LUYA_20090116_001
extern u32 LcdPanleID;    //ZTE_LCD_LHT_20100611_001

static int spi_cs;
static int spi_sclk;
static int spi_sdi;
static int spi_sdo;
static int himax_reset;


static struct msm_panel_common_pdata * lcdc_himax_pdata;
static void gpio_lcd_emuspi_write_one_para(unsigned short addr, unsigned short para);
static void lcdc_himax_sleep(void);
static void lcdc_himax_init(void);
static void lcdc_set_bl(struct msm_fb_data_type *mfd);

static void spi_init(void);
static int lcdc_panel_on(struct platform_device *pdev);
static int lcdc_panel_off(struct platform_device *pdev);

static int lcdc_panel_on(struct platform_device *pdev)
{
	spi_init();
	//ZTE_LCD_LUYA_20090116_001,start
	if(!is_firsttime)
	{
	lcdc_himax_init();
	}
	else
	{
		is_firsttime = false;
	}
	//ZTE_LCD_LUYA_20090116_001,end
	return 0;
}

static void lcdc_himax_sleep(void)
{
/*Display off */    
         gpio_lcd_emuspi_write_one_para(0x0102, 0x180);/*PON=0;PSON=1 */
         mdelay(40);
         gpio_lcd_emuspi_write_one_para(0x0007, 0x0000);
         mdelay(40);
         gpio_lcd_emuspi_write_one_para(0x0100, 0x0004);/*DSTB=1 */
     
/*Power off  */

         gpio_lcd_emuspi_write_one_para(0x1F, 0x02);/*GASENB=0;PON=0;DK=0;XDK=0;VLCD_TRI=1;STB=0 */
         mdelay(10);
         gpio_lcd_emuspi_write_one_para(0x1F, 0x0A);/*GASENB=0;PON=0;DK=1;XDK=0;VLCD_TRI=1;STB=0 */
         mdelay(10);
         
         gpio_lcd_emuspi_write_one_para(0x1C, 0x40);/*AP=000 */
         mdelay(10);
         
/*Into Sleep mode */

         gpio_lcd_emuspi_write_one_para(0x1F, 0x0B);/*GASENB=0;PON=0;DK=0;XDK=0;VLCD_TRI=1;STB=1 */
         mdelay(10);
         
/*Stop OSC   CPU interface  */

         gpio_lcd_emuspi_write_one_para(0x19, 0x90);/*RADJ=1001,OSC_EN=0 */
         mdelay(10);                
  

}

static void lcdc_himax_init(void)
{
	gpio_direction_output(himax_reset, 1);
	mdelay(5);
	gpio_direction_output(himax_reset, 0);
	mdelay(10);
	gpio_direction_output(himax_reset, 1);
	mdelay(50);

	/***********************Start initial squence****************** */
	gpio_lcd_emuspi_write_one_para(0xEA, 0x00); 
	gpio_lcd_emuspi_write_one_para(0xEB, 0x20); 
	gpio_lcd_emuspi_write_one_para(0xEC, 0x0C); 
	gpio_lcd_emuspi_write_one_para(0xED, 0xC4); 
	gpio_lcd_emuspi_write_one_para(0xE8, 0x38); 
	gpio_lcd_emuspi_write_one_para(0xE9, 0x38); 
	gpio_lcd_emuspi_write_one_para(0xF1, 0x01); 
	gpio_lcd_emuspi_write_one_para(0xF2, 0x10);
	gpio_lcd_emuspi_write_one_para(0x27, 0xA3); 
	mdelay(5);
	 
	/*------------  Gamma Setting  ------------------------------- */
	gpio_lcd_emuspi_write_one_para(0x40, 0x00);
	gpio_lcd_emuspi_write_one_para(0x41, 0x00);
	gpio_lcd_emuspi_write_one_para(0x42, 0x01);
	gpio_lcd_emuspi_write_one_para(0x43, 0x12);
	gpio_lcd_emuspi_write_one_para(0x44, 0x10);
	gpio_lcd_emuspi_write_one_para(0x45, 0x26);
	gpio_lcd_emuspi_write_one_para(0x46, 0x08);
	gpio_lcd_emuspi_write_one_para(0x47, 0x54);
	gpio_lcd_emuspi_write_one_para(0x48, 0x02);
	gpio_lcd_emuspi_write_one_para(0x49, 0x15);
	gpio_lcd_emuspi_write_one_para(0x4A, 0x19);
	gpio_lcd_emuspi_write_one_para(0x4B, 0x19);
	gpio_lcd_emuspi_write_one_para(0x4C, 0x16);

	gpio_lcd_emuspi_write_one_para(0x50, 0x19);
	gpio_lcd_emuspi_write_one_para(0x51, 0x2F);
	gpio_lcd_emuspi_write_one_para(0x52, 0x2D);
	gpio_lcd_emuspi_write_one_para(0x53, 0x3E);
	gpio_lcd_emuspi_write_one_para(0x54, 0x3F);
	gpio_lcd_emuspi_write_one_para(0x55, 0x3F);
	gpio_lcd_emuspi_write_one_para(0x56, 0x2B);
	gpio_lcd_emuspi_write_one_para(0x57, 0x77);
	gpio_lcd_emuspi_write_one_para(0x58, 0x09);
	gpio_lcd_emuspi_write_one_para(0x59, 0x06);
	gpio_lcd_emuspi_write_one_para(0x5A, 0x06);
	gpio_lcd_emuspi_write_one_para(0x5B, 0x0A);
	gpio_lcd_emuspi_write_one_para(0x5C, 0x1D);
	gpio_lcd_emuspi_write_one_para(0x5D, 0xCC);

	/*-----------  Power Supply Setting -------------------------- */
	gpio_lcd_emuspi_write_one_para(0x1B, 0x1B); 
	gpio_lcd_emuspi_write_one_para(0x1A, 0x01); 
	gpio_lcd_emuspi_write_one_para(0x24, 0x39); 
	gpio_lcd_emuspi_write_one_para(0x25, 0x7C); 
	gpio_lcd_emuspi_write_one_para(0x23, 0x79); 
	
	/*power on setting */
	gpio_lcd_emuspi_write_one_para(0x18, 0x36); 
	gpio_lcd_emuspi_write_one_para(0x19, 0x01); 
	gpio_lcd_emuspi_write_one_para(0x01, 0x00); 
	gpio_lcd_emuspi_write_one_para(0x1F, 0x88); 
	mdelay(5);
	gpio_lcd_emuspi_write_one_para(0x1F, 0x80); 
	mdelay(5);
	gpio_lcd_emuspi_write_one_para(0x1F, 0x90); 
	mdelay(5);
	gpio_lcd_emuspi_write_one_para(0x1F, 0xD0); 
	mdelay(5);
	
	/*26k/65k color selection */
	gpio_lcd_emuspi_write_one_para(0x17,0x60);
	gpio_lcd_emuspi_write_one_para(0x36,0x00);
	/*Display On setting */
	gpio_lcd_emuspi_write_one_para(0x28,0x38);
	mdelay(40);
	gpio_lcd_emuspi_write_one_para(0x28,0x3C);

	/*240*320 window setting */
 	gpio_lcd_emuspi_write_one_para(0x02,0x00);
	gpio_lcd_emuspi_write_one_para(0x03,0x00);
	gpio_lcd_emuspi_write_one_para(0x04,0x00);
	gpio_lcd_emuspi_write_one_para(0x05,0xEF);

	gpio_lcd_emuspi_write_one_para(0x06,0x00);
	gpio_lcd_emuspi_write_one_para(0x07,0x00);
	gpio_lcd_emuspi_write_one_para(0x08,0x01);
	gpio_lcd_emuspi_write_one_para(0x09,0x3F);

	/*CABC control */
/*	gpio_lcd_emuspi_write_one_para(0x22,0x00); */
	gpio_lcd_emuspi_write_one_para(0x3C,0xF0);
	gpio_lcd_emuspi_write_one_para(0x3D,0x2C);
	gpio_lcd_emuspi_write_one_para(0x3E,0x01);
	gpio_lcd_emuspi_write_one_para(0x3F,0x00);

	gpio_lcd_emuspi_write_one_para(0x31, 0x03); /* RGB interface control */
	gpio_lcd_emuspi_write_one_para(0x32, 0xCE); 
	pr_debug("lcd module himax init exit\n!");


}

static void lcdc_set_bl(struct msm_fb_data_type *mfd)
{
       /*value range is 1--32*/
    int current_lel = mfd->bl_level;
    uint8_t cnt = 0;
    unsigned long flags;

    /*ZTE_BACKLIGHT_HP_001 2009-11-28 */
    printk("[ZYF] lcdc_set_bl level=%d, %d\n", 
		   current_lel , mfd->panel_power_on);

    if(!mfd->panel_power_on)
  	  return;
    if(current_lel < 1)
    {
        current_lel = 0;
    }
    if(current_lel > 32)
    {
        current_lel = 32;
    }
   /* current_lel /= 8;  */

    local_irq_save(flags);
    if(current_lel==0)
	    gpio_direction_output(GPIO_LCD_BL_SC_OUT, 0);
    else {
	    for(cnt = 0;cnt < 33-current_lel;cnt++) //ZTE_KEYBOARD_WLY_CRDB00421949, @2009-12-29
	    { 
		    gpio_direction_output(GPIO_LCD_BL_SC_OUT, 1);
		    udelay(20);
		    gpio_direction_output(GPIO_LCD_BL_SC_OUT, 0);
		    udelay(5);
	    }     
	    gpio_direction_output(GPIO_LCD_BL_SC_OUT, 1);
	    mdelay(8);

    }
    local_irq_restore(flags);
}

static void spi_init(void)
{
	spi_sclk = *(lcdc_himax_pdata->gpio_num);
	spi_cs   = *(lcdc_himax_pdata->gpio_num + 1);
	spi_sdi  = *(lcdc_himax_pdata->gpio_num + 2);
	spi_sdo  = *(lcdc_himax_pdata->gpio_num + 3);
	himax_reset = *(lcdc_himax_pdata->gpio_num + 4);

	gpio_set_value(spi_sclk, 1);
	gpio_set_value(spi_sdo, 1);
	gpio_set_value(spi_cs, 1);
	mdelay(10);

}
static int lcdc_panel_off(struct platform_device *pdev)
{
	lcdc_himax_sleep();

	gpio_direction_output(himax_reset, 0);

	///ZTE_LCD_LUYA_20100221_001
	gpio_direction_output(spi_sclk, 0);
	gpio_direction_output(spi_sdi, 0);
	gpio_direction_output(spi_sdo, 0);
	gpio_direction_output(spi_cs, 0);
	mdelay(100);

	return 0;
}


static void gpio_lcd_emuspi_write_one_para(unsigned short addr, unsigned short para)
{
	unsigned int i;
	int j;

	i = addr | 0x700000;
	gpio_direction_output(spi_cs, 0);
	/*udelay(4);*/

	for (j = 0; j < 24; j++) {

		if (i & 0x800000)
			gpio_direction_output(spi_sdo, 1);	
		else
			gpio_direction_output(spi_sdo, 0);	

		gpio_direction_output(spi_sclk, 0);	
		/*udelay(4);*/
		gpio_direction_output(spi_sclk, 1);	
		/*udelay(4);*/
		i <<= 1;
	}
	gpio_direction_output(spi_cs, 1);
	/*udelay(4);*/

	i = para | 0x720000;
	gpio_direction_output(spi_cs, 0);
	/*udelay(4);*/
	for (j = 0; j < 24; j++) {

		if (i & 0x800000)
			gpio_direction_output(spi_sdo, 1);
		else
			gpio_direction_output(spi_sdo, 0);

		gpio_direction_output(spi_sclk, 0);	
		/*udelay(4);*/
		gpio_direction_output(spi_sclk, 1);
		/*udelay(4);*/
		i <<= 1;
	}
	gpio_direction_output(spi_cs, 1);

}
static void gpio_lcd_emuspi_read_one_para(unsigned short addr, uint32 *data)
{
	unsigned int i;
	int j,ret;
	uint32 dbit,bits;
	i = addr | 0x700000;
	gpio_direction_output(spi_cs, 0);
	/*udelay(4);*/

	for (j = 0; j < 24; j++) {

		if (i & 0x800000)
			gpio_direction_output(spi_sdo, 1);	
		else
			gpio_direction_output(spi_sdo, 0);	

		gpio_direction_output(spi_sclk, 0);	
		/*udelay(4);*/
		gpio_direction_output(spi_sclk, 1);	
		/*udelay(4);*/
		i <<= 1;
	}
	gpio_direction_output(spi_cs, 1);
	/*udelay(4);*/

	i = 0x73;
	gpio_direction_output(spi_cs, 0);
	/*udelay(4);*/

	for (j = 0; j < 8; j++) {

		if (i & 0x80)
			gpio_direction_output(spi_sdo, 1);	
		else
			gpio_direction_output(spi_sdo, 0);	

		gpio_direction_output(spi_sclk, 0);	
		/*udelay(4);*/
		gpio_direction_output(spi_sclk, 1);	
		/*udelay(4);*/
		i <<= 1;
	}

	ret = gpio_direction_input(spi_sdo);
	/*udelay(10); */
	bits=0;
	for (j = 0; j < 8; j++) {

		bits <<= 1;
	  
		gpio_direction_output(spi_sclk, 0);
        ;
        gpio_direction_output(spi_sclk, 1);
		udelay(1);
		dbit=gpio_get_value(spi_sdo);
		/*udelay(45); */
		udelay(1);
		bits|=dbit;
		
	}
	*data = bits;
	gpio_direction_output(spi_cs, 1);

}

static struct msm_fb_panel_data lcdc_himax_panel_data = {
       .panel_info = {.bl_max = 32},
	.on = lcdc_panel_on,
	.off = lcdc_panel_off,
       .set_backlight = lcdc_set_bl,
};

static struct platform_device this_device = {
	.name   = "lcdc_panel_qvga",
	.id	= 1,
	.dev	= {
		.platform_data = &lcdc_himax_panel_data,
	}
};

static int __init lcdc_panel_probe(struct platform_device *pdev)
{
	struct msm_panel_info *pinfo;
	int ret;
	
	if(pdev->id == 0) {
		lcdc_himax_pdata = pdev->dev.platform_data;
		lcdc_himax_pdata->panel_config_gpio(1);
		spi_init();	
		gpio_lcd_emuspi_read_one_para(0x00,&gpio_ic_himax);
		printk(KERN_INFO "lcd panel ic number :gpio_ic_himax %d,gpio_ic_lead %d!\n",gpio_ic_himax,gpio_ic_lead);
		
			/* use the gpio to identify which ic is used */
			///ZTE_LCD_LUYA_20100221_001,ZTE_LCD_LUYA_20100221_001,ZTE_LCD_LUYA_20091226_001, if nopanel,register himax
		 if(gpio_ic_himax != 0x47){
				printk("Fail to register this himax driver!~\n");
				lcdc_himax_regist = 0;
				return -ENODEV;
		}
		 else{
		 		 LcdPanleID=(u32)LCD_PANEL_P726_HX8347D;   //ZTE_LCD_LHT_20100611_001
				 pinfo = &lcdc_himax_panel_data.panel_info;
				 pinfo->xres = 240;
				 pinfo->yres = 320;
				 pinfo->type = LCDC_PANEL;
				 pinfo->pdest = DISPLAY_1;
				 pinfo->wait_cycle = 0;
				 pinfo->bpp = 18;
				 pinfo->fb_num = 2;
			 /*  pinfo->clk_rate = 8192000; */
				 pinfo->clk_rate = 6144000;
				 
		 
				 pinfo->lcdc.h_back_porch = 4;
				 pinfo->lcdc.h_front_porch = 4;
				 pinfo->lcdc.h_pulse_width = 4;
				 pinfo->lcdc.v_back_porch = 3;
				 pinfo->lcdc.v_front_porch = 3;
				 pinfo->lcdc.v_pulse_width = 1;
				 pinfo->lcdc.border_clr = 0; /* blk */
				 pinfo->lcdc.underflow_clr = 0xff;	 /* blue */
				 pinfo->lcdc.hsync_skew = 0;

				ret = platform_device_register(&this_device);	
				return 0;
		 	}
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



static int __init lcdc_himax_panel_init(void)
{
	int ret;

	ret = platform_driver_register(&this_driver);
	if(lcdc_himax_regist==0)
	{
		printk("[lxw@lcd&fb]:unregist himax driver!\n");
		platform_driver_unregister(&this_driver);
		return ret;
	}

	
	return ret;
}

module_init(lcdc_himax_panel_init);

