/*
 * =====================================================================================
 *
 *       Filename:  lcdc_panel_qvga_lead.c
 *
 *    Description:  Lead QVGA panel(240x320) driver which ic No. is 9325
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
when         who        what, where, why                                  comment tag
--------     ----       -----------------------------                --------------------------
2011-03-24   lht		   modify delay	 CRDB00629443        	ZTE_LCD_LUYA_20110324_001
2010-06-11   lht		   project mode display panel info         	ZTE_LCD_LHT_20100611_001
2010-02-21   luya		merge samsung IC for QVGA &pull down spi when sleep		ZTE_LCD_LUYA_20100221_001        
2010-02-01   luya    	turn off BKL when fb enter early suspend					 ZTE_LCD_LUYA_20100201_001
2009-12-29   wly               ajust max backlight level                     ZTE_KEYBOARD_WLY_CRDB00421949        
2009-11-27   lixiongwei        merge new driver for lead 9325c qvga          ZTE_LCD_LIXW_001
2009-12-21   luya			   do not do panel_init for the 1st time		 ZTE_LCD_LUYA_20091221_001
2009-12-26   luya			   merge nopanel(QVGA)driver to avoid the problem 
							   of bootup abnormally without lcd panel		 ZTE_LCD_LUYA_20091226_001

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

static int lcdc_lead_regist = 1;
static boolean is_firsttime = true;		///ZTE_LCD_LUYA_20091221_001
uint32 gpio_ic_lead;			//ZTE_LCD_LUYA_20091226_001

static int spi_cs;
static int spi_sclk;
static int spi_sdi;
static int spi_sdo;
static int lead_reset;
extern u32 LcdPanleID;    //ZTE_LCD_LHT_20100611_001

static struct msm_panel_common_pdata * lcdc_lead_pdata;
static void gpio_lcd_emuspi_write_one_para(unsigned short addr, unsigned short para);
/*static void lcdc_lead_wakeup(void);*/
static void lcdc_lead_sleep(void);
static void lcdc_lead_init(void);
static void lcdc_set_bl(struct msm_fb_data_type *mfd);

static void spi_init(void);
static int lcdc_panel_on(struct platform_device *pdev);
static int lcdc_panel_off(struct platform_device *pdev);

static int lcdc_panel_on(struct platform_device *pdev)
{

	spi_init();
///ZTE_LCD_LUYA_20091221_001,start	
	if(!is_firsttime)
	{
		lcdc_lead_init();
		
	}
	else
	{
		is_firsttime = false;
	}
///ZTE_LCD_LUYA_20091221_001,end	
	return 0;
}

static void lcdc_lead_sleep(void)
{
	gpio_lcd_emuspi_write_one_para(0x0007,0x0131); 
	mdelay(10);

	gpio_lcd_emuspi_write_one_para(0x0007,0x0130);    
	mdelay(10);

	gpio_lcd_emuspi_write_one_para(0x0007,0x0000);  /* display OFF   */

	gpio_lcd_emuspi_write_one_para(0x0010,0x0080);   /* SAP, BT[3:0], APE, AP, DSTB, SLP   */
	gpio_lcd_emuspi_write_one_para(0x0011,0x0000);   /* DC1[2:0], DC0[2:0], VC[2:0]  */
	gpio_lcd_emuspi_write_one_para(0x0012,0x0000);  /* VREG1OUT voltage  */
	gpio_lcd_emuspi_write_one_para(0x0013,0x0000);  /* VDV[4:0] for VCOM amplitude   */
//	mdelay(200);   /* Dis-charge capacitor power voltage */

	gpio_lcd_emuspi_write_one_para(0x0010,0x0082);   /*SAP, BT[3:0], APE, AP, DSTB, SLP*/
	msleep(200);

}

static void lcdc_lead_init(void)
{
	/* reset lcd module */
	gpio_direction_output(lead_reset, 1);
	mdelay(10);
	gpio_direction_output(lead_reset, 0);
	mdelay(20);
	gpio_direction_output(lead_reset, 1);
	msleep(30);


	gpio_lcd_emuspi_write_one_para(0x0001,0x0100);    /* set SS and SM bit  */
	gpio_lcd_emuspi_write_one_para(0x0002,0x0700);    /* set 1 line inversion    */
	gpio_lcd_emuspi_write_one_para(0x0003,0x1030);    /* set GRAM write direction and BGR=1 */
	gpio_lcd_emuspi_write_one_para(0x0004,0x0000);    /* Resize register                   */

	gpio_lcd_emuspi_write_one_para(0x0008,0x0202);    /* set the back porch and front porch  */
	gpio_lcd_emuspi_write_one_para(0x0009,0x0000);    /* set non-display area refresh cycle */
	gpio_lcd_emuspi_write_one_para(0x000A,0x0000);    /* FMARK function */
	/*gpio_lcd_emuspi_write_one_para(0x000C,0x0101);    // RGB interface setting    16bit */
	/*gpio_lcd_emuspi_write_one_para(0x000C,0x0111);    // RGB interface setting    */
	gpio_lcd_emuspi_write_one_para(0x000D,0x0000);    /* Frame marker Position         */
	gpio_lcd_emuspi_write_one_para(0x000F,0x001b);    /* RGB interface polarity  */

	/*************Power On sequence ****************/ 
	/* Dis-charge capacitor power voltage  */
	gpio_lcd_emuspi_write_one_para(0x0010,0x0000);      /* SAP, BT[3:0], AP, DSTB, SLP, STB  */
	gpio_lcd_emuspi_write_one_para(0x0011,0x0007);      /* DC1[2:0], DC0[2:0], VC[2:0]      */
	gpio_lcd_emuspi_write_one_para(0x0012,0x0000);     /* VREG1OUT voltage                 */
	/*ZTE_LCD_HP_002 2009-10-30 for 9325c*/
	gpio_lcd_emuspi_write_one_para(0x0013,0x0000);     /* VDV[4:0] for VCOM amplitude     */
	gpio_lcd_emuspi_write_one_para(0x0007,0x0001);
	msleep(150);                                        /* Dis-charge capacitor power voltage*/                                        
	gpio_lcd_emuspi_write_one_para(0x0010,0x1290);     /* SAP, BT[3:0], AP, DSTB, SLP, STB */
	/*gpio_lcd_emuspi_write_one_para(0x0010,0x1290);     // SAP, BT[3:0], AP, DSTB, SLP, STB  */
	gpio_lcd_emuspi_write_one_para(0x0011,0x0227);     
	msleep(30);

	gpio_lcd_emuspi_write_one_para(0x0012,0x009A);     /*External reference voltage= Vci*/
	/*gpio_lcd_emuspi_write_one_para(0x0012,0x001A);     //External reference voltage= Vci*/
	msleep(30);
	gpio_lcd_emuspi_write_one_para(0x0013,0x1600);     /* VDV[4:0] for VCOM amplitude  */
	gpio_lcd_emuspi_write_one_para(0x0029,0x001D);      /* VCM[5:0] for VCOMH   */
	gpio_lcd_emuspi_write_one_para(0x002B,0x000D);       /* Frame Rate = 99Hz  */
	/*gpio_lcd_emuspi_write_one_para(0x002B,0x000c);       // Frame Rate = 99Hz   */
	msleep(30);
	gpio_lcd_emuspi_write_one_para(0x0020,0x0000);
	gpio_lcd_emuspi_write_one_para(0x0021,0x0000);	

	/* ----------- Adjust the Gamma Curve ----------*/                                                                  
	gpio_lcd_emuspi_write_one_para(0x0030,0x0000);
	gpio_lcd_emuspi_write_one_para(0x0031,0x0000);
	gpio_lcd_emuspi_write_one_para(0x0032,0x0101);
	gpio_lcd_emuspi_write_one_para(0x0035,0x0201);
	gpio_lcd_emuspi_write_one_para(0x0036,0x0000);
	gpio_lcd_emuspi_write_one_para(0x0037,0x0206);
	gpio_lcd_emuspi_write_one_para(0x0038,0x0000);
	gpio_lcd_emuspi_write_one_para(0x0039,0x0702);
	gpio_lcd_emuspi_write_one_para(0x003C,0x0102);
	gpio_lcd_emuspi_write_one_para(0x003D,0x0000);
	/*ZTE_LCD_HP_002 end */
	/*------------------ Set  GRAM area ---------------*/                                                               
	gpio_lcd_emuspi_write_one_para(0x0050,0x0000);     /* Horizontal GRAM Start Address*/
	gpio_lcd_emuspi_write_one_para(0x0051,0x00EF);     /* Horizontal GRAM End Address  */
	gpio_lcd_emuspi_write_one_para(0x0052,0x0000);     /* Vertical GRAM Start Address */
	gpio_lcd_emuspi_write_one_para(0x0053,0x013F);     /* Vertical GRAM Start Address*/
	gpio_lcd_emuspi_write_one_para(0x0060,0xa700); 
	gpio_lcd_emuspi_write_one_para(0x0061,0x0001);
	gpio_lcd_emuspi_write_one_para(0x006A,0x0000);     /* set scrolling line     */

	/*-------------- Partial Display Control ---------*/                                                                
	gpio_lcd_emuspi_write_one_para(0x0080,0x0000);   
	gpio_lcd_emuspi_write_one_para(0x0081,0x0000);  
	gpio_lcd_emuspi_write_one_para(0x0082,0x0000); 
	gpio_lcd_emuspi_write_one_para(0x0083,0x0000);
	gpio_lcd_emuspi_write_one_para(0x0084,0x0000);
	gpio_lcd_emuspi_write_one_para(0x0085,0x0000); 

	/*-------------- Panel Control -------------------*/                                                                
	gpio_lcd_emuspi_write_one_para(0x0090,0x0010);   
	gpio_lcd_emuspi_write_one_para(0x0092,0x0600); 
	msleep(50);                                                                        
	gpio_lcd_emuspi_write_one_para(0x0007,0x0133);     /* 262K color and display ON   */

	gpio_lcd_emuspi_write_one_para(0x000C,0x0100);    /* RGB interface setting */  
	pr_debug("lcd module lead init exit\n!");

}


static void lcdc_set_bl(struct msm_fb_data_type *mfd)
{
       /*value range is 1--32*/
    int current_lel = mfd->bl_level;
    uint8_t cnt = 0;
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

    /*ZTE_BACKLIGHT_WLY_005,@2009-11-28, set backlight as 32 levels, end*/
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
	spi_sclk = *(lcdc_lead_pdata->gpio_num);
	spi_cs   = *(lcdc_lead_pdata->gpio_num + 1);
	spi_sdi  = *(lcdc_lead_pdata->gpio_num + 2);
	spi_sdo  = *(lcdc_lead_pdata->gpio_num + 3);
	lead_reset = *(lcdc_lead_pdata->gpio_num + 4);

	gpio_set_value(spi_sclk, 1);
	gpio_set_value(spi_sdo, 1);
	gpio_set_value(spi_cs, 1);
	mdelay(10);

}
static int lcdc_panel_off(struct platform_device *pdev)
{

	lcdc_lead_sleep();

	gpio_direction_output(lead_reset, 0);

	///ZTE_LCD_LUYA_20100221_001
	gpio_direction_output(spi_sclk, 0);
	gpio_direction_output(spi_sdi, 0);
	gpio_direction_output(spi_sdo, 0);
	gpio_direction_output(spi_cs, 0);
	
	msleep(20);

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

static void gpio_lcd_emuspi_read_one_para1(unsigned short addr, uint32 *data)
{
	unsigned int i;
	int j,ret;
	uint32 dbit,bits,bits1;
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

	ret = gpio_direction_input(spi_sdi);
	bits=0;
	for (j = 0; j < 16; j++) {
  
		gpio_direction_output(spi_sclk, 0);
        ;
        gpio_direction_output(spi_sclk, 1);
		udelay(1);
		dbit=gpio_get_value(spi_sdi);
		udelay(1);
		bits = 2*bits+dbit;
		
	}

	udelay(1);
	bits1=0;
	for (j = 0; j < 8; j++) {
 
		gpio_direction_output(spi_sclk, 0);
        ;
        gpio_direction_output(spi_sclk, 1);
		udelay(1);
		dbit=gpio_get_value(spi_sdi);
		udelay(1);
		bits1 = 2*bits1+dbit;
		
	}

	*data = ((bits>>8)<<8) + bits1;
	gpio_direction_output(spi_cs, 1);

}

static struct msm_fb_panel_data lcdc_lead_panel_data = {
       .panel_info = {.bl_max = 32},
	.on = lcdc_panel_on,
	.off = lcdc_panel_off,
       .set_backlight = lcdc_set_bl,
};

static struct platform_device this_device = {
	.name   = "lcdc_panel_qvga",
	.id	= 1,
	.dev	= {
		.platform_data = &lcdc_lead_panel_data,
	}
};

static int __init lcdc_panel_probe(struct platform_device *pdev)
{
	struct msm_panel_info *pinfo;
	int ret;

	if(pdev->id == 0) {
		lcdc_lead_pdata = pdev->dev.platform_data;
		lcdc_lead_pdata->panel_config_gpio(1);
		spi_init();	
		gpio_lcd_emuspi_read_one_para1(0x00,&gpio_ic_lead);
		printk(KERN_INFO "lcd panel ic number on gpio 0x00h gpio_ic_lead %d!\n",gpio_ic_lead);
	
		/*use the gpio to identify which ic is used*/
		//gpio_ic_lead = 0x9325 ;
	 if(gpio_ic_lead != 0x9325){
			printk("Fail to register this lead driver!\n");
			lcdc_lead_regist = 0;
			return -ENODEV;
	 }
	 else{
	 	LcdPanleID=(u32)LCD_PANEL_P726_ILI9325C;   //ZTE_LCD_LHT_20100611_001
		pinfo = &lcdc_lead_panel_data.panel_info;
		pinfo->xres = 240;
		pinfo->yres = 320;
		pinfo->type = LCDC_PANEL;
		pinfo->pdest = DISPLAY_1;
		pinfo->wait_cycle = 0;
		pinfo->bpp = 18;
		pinfo->fb_num = 2;

		pinfo->clk_rate = 6144000;
		
		pinfo->lcdc.h_back_porch = 4;
		pinfo->lcdc.h_front_porch = 4;
		pinfo->lcdc.h_pulse_width = 5;
		pinfo->lcdc.v_back_porch = 2;
		pinfo->lcdc.v_front_porch = 2;
		pinfo->lcdc.v_pulse_width = 2;
		pinfo->lcdc.border_clr = 0;	/* blk */
		pinfo->lcdc.underflow_clr = 0xff;	/* blue */
		pinfo->lcdc.hsync_skew = 5;

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



static int __init lcdc_lead_panel_init(void)
{
	int ret;

	ret = platform_driver_register(&this_driver);

	if(lcdc_lead_regist == 0)
	{
		printk("[lxw@lcd&fb]:unregist lead driver!\n");
		platform_driver_unregister(&this_driver);
		return ret;
	}

	return ret;
}

module_init(lcdc_lead_panel_init);

