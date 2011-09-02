/*
 * =====================================================================================
 *
 *       Filename:  lcdc_panel_qvga_samsung.c
 *
 *    Description:  Lead QVGA panel(240x320) driver which ic No. is 9325
 *
 *        Version:  1.0
 *        Created:  01/18/2010 02:05:46 PM
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
2010-06-11   lht		project mode display panel info         	ZTE_LCD_LHT_20100611_001
==========================================================================================*/


#include "msm_fb.h"
#include <asm/gpio.h>
#include <linux/module.h>
#include <linux/delay.h>

#define GPIO_LCD_BL_SC_OUT 97

static int lcdc_samsung_regist = 1;
static boolean is_firsttime = true;

static int spi_cs;
static int spi_sclk;
static int spi_sdi;
static int spi_sdo;
static int lcd_panel_reset;
static int lcd_panel_id;
extern u32 LcdPanleID;   //ZTE_LCD_LHT_20100611_001
typedef enum
{
	LCD_PANEL_NONE = 0,
	LCD_PANEL_LEAD_HVGA,
	LCD_PANEL_XINGLI_HVGA,
}LCD_PANEL_TYPE;

LCD_PANEL_TYPE g_lcd_panel_type = LCD_PANEL_NONE;

static __u32 last_bl_level = 6;    

static DEFINE_SPINLOCK(tps61061_lock);

static struct msm_panel_common_pdata * lcd_panel_pdata;
static void lcd_panel_sleep(void);
static void lcd_panel_wakeup(void);
static void lcd_panel_init(void);
static void lcdc_set_bl(struct msm_fb_data_type *mfd);

static void spi_init(void);
static int lcdc_panel_on(struct platform_device *pdev);
static int lcdc_panel_off(struct platform_device *pdev);

static boolean bSleepWhenSuspend = false;

static void ILI9481_WriteReg(unsigned char SPI_COMMD)
{
	unsigned short SBit,SBuffer;
	unsigned char BitCounter;
	
	SBuffer=SPI_COMMD;
	gpio_direction_output(spi_cs, 0);	//Set_CS(0); //CLR CS
	for(BitCounter=0;BitCounter<9;BitCounter++)
	{
		SBit = SBuffer&0x100;
		if(SBit)
			gpio_direction_output(spi_sdo, 1);//Set_SDA(1);
		else
			gpio_direction_output(spi_sdo, 0);//Set_SDA(0);
			
		gpio_direction_output(spi_sclk, 0);//Set_SCK(0); //CLR SCL
		gpio_direction_output(spi_sclk, 1);//Set_SCK(1); //SET SCL
		SBuffer = SBuffer<<1;
	}
	gpio_direction_output(spi_cs, 1);//Set_CS(1); //SET CS
}
//***********************************************
//***********************************************
static void ILI9481_WriteData(unsigned char SPI_DATA)
{
	unsigned short SBit,SBuffer;
	unsigned char BitCounter;
	
	SBuffer=SPI_DATA | 0x100;
	gpio_direction_output(spi_cs, 0);//Set_CS(0); //CLR CS
	
	for(BitCounter=0;BitCounter<9;BitCounter++)
	{
		SBit = SBuffer&0x100;
		if(SBit)
			gpio_direction_output(spi_sdo, 1);//Set_SDA(1);
		else
			gpio_direction_output(spi_sdo, 0);//Set_SDA(0);
			
		gpio_direction_output(spi_sclk, 0);//Set_SCK(0); //CLR SCL
		gpio_direction_output(spi_sclk, 1);//Set_SCK(1); //SET SCL
		SBuffer = SBuffer<<1;
	}
	gpio_direction_output(spi_cs, 1);//Set_CS(1); //SET CS
}

static void lcd_panel_sleep(void)
{
	ILI9481_WriteReg(0x10);
}

static void lcd_panel_wakeup(void)
{
	ILI9481_WriteReg(0x11);
	ILI9481_WriteReg(0x29);
}

static void lcdc_lead_init(void)
{
       printk("gequn lead init kernel \n");
	//************* Start Initial Sequence **********//
	ILI9481_WriteReg(0x11);
	mdelay(20);//Delay(10*20);
	
	//CPT+ILI9481
	ILI9481_WriteReg(0xC6);
	ILI9481_WriteData(0x9B);///5B

	ILI9481_WriteReg(0xD0);
	ILI9481_WriteData(0x07);
	ILI9481_WriteData(0x47);
	ILI9481_WriteData(0x1A);
	
	ILI9481_WriteReg(0xD1);
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x23);//10
	ILI9481_WriteData(0x11);
	
	ILI9481_WriteReg(0xD2);
	ILI9481_WriteData(0x01);
	ILI9481_WriteData(0x11);
	
	ILI9481_WriteReg(0xC0);
	ILI9481_WriteData(0x10);
	ILI9481_WriteData(0x3B);
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x02);
	ILI9481_WriteData(0x11);
	
	ILI9481_WriteReg(0xC5);
	ILI9481_WriteData(0x01);//3//
	
	ILI9481_WriteReg(0xC8);
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x66);
	ILI9481_WriteData(0x15);
	ILI9481_WriteData(0x24);
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x08);
	ILI9481_WriteData(0x26);
	ILI9481_WriteData(0x11);
	ILI9481_WriteData(0x77);
	ILI9481_WriteData(0x42);
	ILI9481_WriteData(0x08);
	ILI9481_WriteData(0x00);
	
	ILI9481_WriteReg(0x36);
	ILI9481_WriteData(0x09);  //0x0A
	ILI9481_WriteReg(0x3A);
	ILI9481_WriteData(0x66);
	ILI9481_WriteReg(0x0C);
	ILI9481_WriteData(0x66);
	ILI9481_WriteReg(0x2A);
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x01);
	ILI9481_WriteData(0x3F);
	ILI9481_WriteReg(0x2B);
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x01);
	ILI9481_WriteData(0xE0);
	mdelay(120);//Delay(10*120);
	ILI9481_WriteReg(0xB4);
	ILI9481_WriteData(0x11);//ILI9481_WriteData(0x10);//

	ILI9481_WriteReg(0x29);
	ILI9481_WriteReg(0x2C);
}

static void lcdc_xingli_init(void)
{
       printk("gequn truly init kernel \n");
	//************* Start Initial Sequence **********//
	ILI9481_WriteReg(0x11);
	mdelay(20);//Delay(10*20);

	//AUO+ILI9481
		ILI9481_WriteReg(0xC6);
		ILI9481_WriteData(0x5B);
	
	ILI9481_WriteReg(0xD0);
	ILI9481_WriteData(0x07);
	ILI9481_WriteData(0x44);///47
	ILI9481_WriteData(0x1B);
	
	ILI9481_WriteReg(0xD1);
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x24);//07  2D
	ILI9481_WriteData(0x14);
	
	ILI9481_WriteReg(0xD2);
	ILI9481_WriteData(0x01);
	ILI9481_WriteData(0x11);
	
	ILI9481_WriteReg(0xC0);
	ILI9481_WriteData(0x10);
	ILI9481_WriteData(0x3B);
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x02);
	ILI9481_WriteData(0x11);
	
	ILI9481_WriteReg(0xC5);
	ILI9481_WriteData(0x03);
	
	ILI9481_WriteReg(0xC8);
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x54);
	ILI9481_WriteData(0x25);
	ILI9481_WriteData(0x22);
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x08);
	ILI9481_WriteData(0x25);
	ILI9481_WriteData(0x32);
	ILI9481_WriteData(0x77);
	ILI9481_WriteData(0x22);
	ILI9481_WriteData(0x08);
	ILI9481_WriteData(0x00);
	
	ILI9481_WriteReg(0x36);
	ILI9481_WriteData(0x09);  //0x0A
	ILI9481_WriteReg(0x3A);
	ILI9481_WriteData(0x66);
	ILI9481_WriteReg(0x0C);
	ILI9481_WriteData(0x66);
	ILI9481_WriteReg(0x2A);
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x01);
	ILI9481_WriteData(0x3F);
	ILI9481_WriteReg(0x2B);
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x01);
	ILI9481_WriteData(0xE0);
	mdelay(120);//Delay(10*120);
	ILI9481_WriteReg(0xB4);
	ILI9481_WriteData(0x11);//ILI9481_WriteData(0x10);//
	ILI9481_WriteReg(0x29);
	ILI9481_WriteReg(0x2C);
}

static void lcd_panel_init(void)
{
	mdelay(10);
	gpio_direction_output(lcd_panel_reset, 1);
	mdelay(1);
	gpio_direction_output(lcd_panel_reset, 0);
	mdelay(10);
	gpio_direction_output(lcd_panel_reset, 1);
	mdelay(100);

	switch(g_lcd_panel_type)
	{
		case LCD_PANEL_XINGLI_HVGA:
			lcdc_xingli_init();
			break;
		case LCD_PANEL_LEAD_HVGA:
		default:
			lcdc_lead_init();
	}
}

static int lcdc_panel_on(struct platform_device *pdev)
{
	if(!is_firsttime)
	{
		if(bSleepWhenSuspend)
		{
			lcd_panel_wakeup();
		}
		else
		{
			spi_init();
			lcd_panel_init();
		}
	}
	else
	{
		is_firsttime = false;
	}

	return 0;
}

static void lcdc_set_bl(struct msm_fb_data_type *mfd)
{
	__u32 new_bl_level = mfd->bl_level;
	__u32 cnt,diff;
	unsigned long flags;


	pr_info("[ZYF] lcdc_set_bl level=%d, %d\n", new_bl_level , mfd->panel_power_on);

	if(!mfd->panel_power_on)
	{
		gpio_direction_output(GPIO_LCD_BL_SC_OUT,0);
		return;
	}

	if(new_bl_level < 1)
	{
		new_bl_level = 0;
	}

	if (new_bl_level > 17)
	{
		new_bl_level = 17;
	}

	if(0 == new_bl_level)
	{
		gpio_direction_output(GPIO_LCD_BL_SC_OUT,0);    
		udelay(800); /* at least delay 720us to shut off */  
	} 
	else 
	{
		if(0 == last_bl_level)
		{
			gpio_direction_output(GPIO_LCD_BL_SC_OUT,1);
			udelay(5); /* at least delay 1.5us between steps */
			last_bl_level = 16;
		}  
		  
		if(new_bl_level == last_bl_level)	return;
		
		if(new_bl_level > last_bl_level)   //need increasing feedback vlotage
		{
			diff = new_bl_level - last_bl_level;
			for(cnt=0;cnt < diff;cnt++)
			{  
				spin_lock_irqsave(&tps61061_lock, flags);  //lock 
				gpio_direction_output(GPIO_LCD_BL_SC_OUT,0);
				udelay(10);	/* delay 1--75us lcd-backlight brightness increase */    
				gpio_direction_output(GPIO_LCD_BL_SC_OUT,1);  
				spin_unlock_irqrestore(&tps61061_lock, flags); //unlock
				udelay(3); 	/* at least delay 1.5us between steps */
			}     	  
		}
		else   //need decreasing feedback vlotage
		{
			diff = last_bl_level - new_bl_level;
			for(cnt=0;cnt < diff;cnt++)
			{  
				spin_lock_irqsave(&tps61061_lock, flags);   //lock 
				gpio_direction_output(GPIO_LCD_BL_SC_OUT,0);
				udelay(200);	/* delay 180 --300us lcd-backlight brightness decrease */    
				gpio_direction_output(GPIO_LCD_BL_SC_OUT,1);  
				spin_unlock_irqrestore(&tps61061_lock, flags); //unlock
				udelay(30); 	/* at least delay 1.5us between steps */    
			}     
		}
	}

	last_bl_level = new_bl_level; //record new level value
}

static void spi_init(void)
{
	spi_sclk = *(lcd_panel_pdata->gpio_num);
	spi_cs   = *(lcd_panel_pdata->gpio_num + 1);
	spi_sdi  = *(lcd_panel_pdata->gpio_num + 2);
	spi_sdo  = *(lcd_panel_pdata->gpio_num + 3);
	lcd_panel_reset = *(lcd_panel_pdata->gpio_num + 4);
	lcd_panel_id =  *(lcd_panel_pdata->gpio_num + 5);

	gpio_set_value(spi_sclk, 1);
	gpio_set_value(spi_sdo, 1);
	gpio_set_value(spi_cs, 1);
}
static int lcdc_panel_off(struct platform_device *pdev)
{
	lcd_panel_sleep();

	if(!bSleepWhenSuspend)
	{
		gpio_direction_output(lcd_panel_reset, 0);
		gpio_direction_output(spi_sclk, 0);
		gpio_direction_output(spi_sdi, 0);
		gpio_direction_output(spi_sdo, 0);
		gpio_direction_output(spi_cs, 0);
	}

	return 0;
}

static struct msm_fb_panel_data lcd_lcdcpanel_panel_data = {
       .panel_info = {.bl_max = 17},
	.on = lcdc_panel_on,
	.off = lcdc_panel_off,
       .set_backlight = lcdc_set_bl,
};

static struct platform_device this_device = {
	.name   = "lcdc_panel_qvga",
	.id	= 1,
	.dev	= {
		.platform_data = &lcd_lcdcpanel_panel_data,
	}
};

static LCD_PANEL_TYPE lcd_panel_detect(void)
{
    spi_init();
    printk("gequn sailboat lcd_panel_id =%d\n",  lcd_panel_id );
	gpio_direction_input(lcd_panel_id);

	if(gpio_get_value(lcd_panel_id))
	   {
	       printk("gequn truly detect kernel \n");
		return LCD_PANEL_XINGLI_HVGA;
	    }
	else
	    {
	      printk("gequn lead detect kernel \n");
	      return LCD_PANEL_LEAD_HVGA;
	     }
}

static int __init lcdc_panel_probe(struct platform_device *pdev)
{
	struct msm_panel_info *pinfo;
	int ret;

	if(pdev->id == 0) {
		printk("use lead 320x480 panel driver!\n");
		lcd_panel_pdata = pdev->dev.platform_data;
		lcd_panel_pdata->panel_config_gpio(1);

		g_lcd_panel_type = lcd_panel_detect();

		switch(g_lcd_panel_type)
		{
			case LCD_PANEL_XINGLI_HVGA:
				LcdPanleID=(u32)LCD_PANEL_R750_ILI9481_3;   //ZTE_LCD_LHT_20100611_001
				break;
			case LCD_PANEL_LEAD_HVGA:
				LcdPanleID=(u32)LCD_PANEL_R750_ILI9481_1;   //ZTE_LCD_LHT_20100611_001
				break;
			default:
				break;
		}		

		pinfo = &lcd_lcdcpanel_panel_data.panel_info;
		pinfo->xres = 320;
		pinfo->yres = 480;
		pinfo->type = LCDC_PANEL;
		pinfo->pdest = DISPLAY_1;
		pinfo->wait_cycle = 0;
		pinfo->bpp = 18;
		pinfo->fb_num = 2;

		pinfo->clk_rate = 8192000;
		
		pinfo->lcdc.h_back_porch = 3;
		pinfo->lcdc.h_front_porch = 3;
		pinfo->lcdc.h_pulse_width = 3;
		pinfo->lcdc.v_back_porch = 2;
		pinfo->lcdc.v_front_porch = 4;
		pinfo->lcdc.v_pulse_width = 2;
		pinfo->lcdc.border_clr = 0;	/* blk */
		pinfo->lcdc.underflow_clr = 0xff;	/* blue */
		pinfo->lcdc.hsync_skew = 3;

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



static int __init lcd_panel_panel_init(void)
{
	int ret;

	ret = platform_driver_register(&this_driver);

	if(lcdc_samsung_regist == 0)
	{
		printk("[lxw@lcd&fb]:unregist samsung driver!\n");
		platform_driver_unregister(&this_driver);
		return ret;
	}

	return ret;
}

module_init(lcd_panel_panel_init);

