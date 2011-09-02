/*
 * =====================================================================================
 *
 *       Filename:  lcdc_panel_wvga_oled.c
 *
 *    Description:  Samsung WVGA panel(480x800) driver 
 *
 *        Version:  1.0
 *        Created:  03/25/2010 02:05:46 PM
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
--------     ----       -----------------------------      --------------------------
2011-05-12 	 lkej		modify init code of lead for flick                   ZTE_LCD_LKEJ_20110512_001
2011-04-25   lkej 	modify backlight level for reduce  current      ZTE_LCD_LKEJ_20110425_001
2011-04-22   lkej 	modify backlight level for reduce  current	ZTE_LCD_LKEJ_20110422_001
2011-02-16   lkej 	modify code support for compatible lcd 	      ZTE_LCD_LKEJ_20110216_001
2011-02-12	 lkej		merged code for  lcd driver				
==========================================================================================*/
#include "msm_fb.h"
#include <asm/gpio.h>
#include <linux/module.h>
#include <linux/delay.h>

#define GPIO_LCD_BL_SC_OUT 97
//ZTE_LCD_LKEJ_20110216_001,start
#define GPIO_LCD_ID_DETECT  107
//ZTE_LCD_LKEJ_20110216_001,end

#define GPIO_SPI_CS          122
#define GPIO_SPI_SDO         123
#define GPIO_SPI_SCLK        124
#define GPIO_SPI_SDI         132

#define LCD_BL_LEVEL 12
#define lcd_bl_max   lcd_bl_level-1
#define lcd_bl_min   0

typedef enum
{
	LCD_PANEL_NONE = 0,
	LCD_PANEL_LEAD_QVGA,
	LCD_PANEL_YASSY_QVGA,
}LCD_PANEL_TYPE;

static LCD_PANEL_TYPE g_lcd_panel_type = LCD_PANEL_NONE;

static boolean is_firsttime = true;		
static boolean lcd_init_once = false;

extern u32 LcdPanleID;   
static int spi_cs;
static int spi_sclk;
static int spi_sdi;
static int spi_sdo;
static int panel_reset;
static DEFINE_SPINLOCK(tps61061_lock);

static struct msm_panel_common_pdata * lcdc_tft_pdata;
static void HX8368_WriteData(unsigned char SPI_DATA);
static void HX8368_WriteReg(unsigned char SPI_COMMD);
static void lcdc_lead_init(void);
static void lcdc_yassy_init(void);
static void lcd_panel_init(void);
static void lcdc_set_bl(struct msm_fb_data_type *mfd);
void lcdc_lead_sleep(void);
void lcdc_yassy_sleep(void);
static void spi_init(void);
static int lcdc_panel_on(struct platform_device *pdev);
static int lcdc_panel_off(struct platform_device *pdev);

/*===========================================================================

FUNCTION  LCDC_SET_BL

DESCRIPTION
  The lcd-backlight has 32 levels, when it is from zero to one level, voltage of 
  the backlight is 250mv,that is to say, the backlight at midpoint of 16 levels.

===========================================================================*/

static __u32 last_bl_level = 6;  

//tps61061 backlight chip
static void lcdc_set_bl(struct msm_fb_data_type *mfd)
{
    /*
      * value range is 0--32
      */
    __u32 new_bl_level = mfd->bl_level;
    __u32 cnt,diff;
    unsigned long flags;

//ZTE_LCD_LUYA_20100114_001,start
    pr_info("[ZYF] lcdc_set_bl level=%d, %d\n", 
		   new_bl_level , mfd->panel_power_on);

    if(!mfd->panel_power_on)
    {
    	gpio_direction_output(GPIO_LCD_BL_SC_OUT,0);   //20100301 liuhaitao for sleep/wakeup bkl black 
    	udelay(800);
	    return;	
    }
//ZTE_LCD_LUYA_20100114_001,return
		
    if(new_bl_level < 1)
    {
        new_bl_level = 0;
    }
    if (new_bl_level > 15)   //ZTE_LCD_LKEJ_20110422_001
    {
        new_bl_level = 15;//ZTE_LCD_LKEJ_20110425_001
    }

    if(0 == new_bl_level)
    {
        	   gpio_direction_output(GPIO_LCD_BL_SC_OUT,0);    
		   /* at least delay 720us to shut off */  
    		   udelay(800); 
    } 
   else 
   {
          if(lcd_init_once)
          {
   		    msleep(40);
		    lcd_init_once = false;
		  }
          if(0 == last_bl_level)
          {
		  	gpio_direction_output(GPIO_LCD_BL_SC_OUT,1);
			/* at least delay 1.5us between steps */
			udelay(5); 
			last_bl_level = 16;
          }  
		  
	   if(new_bl_level == last_bl_level)	return;
		
	   if(new_bl_level > last_bl_level)   //need increasing feedback vlotage
	   {
		        diff = new_bl_level - last_bl_level;
				spin_lock_irqsave(&tps61061_lock, flags);  //lock 
		        for(cnt=0;cnt < diff;cnt++)
		        {  	    				   
			    gpio_direction_output(GPIO_LCD_BL_SC_OUT,0);
			     /* delay 1--75us lcd-backlight brightness increase */    
			    udelay(30);
			    gpio_direction_output(GPIO_LCD_BL_SC_OUT,1);  
				udelay(10); 
			     /* at least delay 1.5us between steps */    
			   // udelay(3); 
		        }    
				spin_unlock_irqrestore(&tps61061_lock, flags); //unlock
	   }
	   else   //need decreasing feedback vlotage
	   {
		        diff = last_bl_level - new_bl_level;
				spin_lock_irqsave(&tps61061_lock, flags);	//lock 
		        for(cnt=0;cnt < diff;cnt++)
		        {  
				   
			    gpio_direction_output(GPIO_LCD_BL_SC_OUT,0);
			     /* delay 180 --300us lcd-backlight brightness decrease */    
			    udelay(250);
			    gpio_direction_output(GPIO_LCD_BL_SC_OUT,1);  
				udelay(10); 	
			     /* at least delay 1.5us between steps */    
			  //  udelay(3); 
		        }     
				spin_unlock_irqrestore(&tps61061_lock, flags); //unlock
	   }
   }

    last_bl_level = new_bl_level; //record new level value
	
}

static int lcdc_panel_on(struct platform_device *pdev)
{
	spi_init();
///ZTE_LCD_LUYA_20091221_001,start	ZTE_LCD_LUYA_20100513_001
	if(!is_firsttime)
	{
		lcd_panel_init();
		lcd_init_once = true;
	}
	else
	{
		is_firsttime = false;
	}
///ZTE_LCD_LUYA_20091221_001,end	
	return 0;
}
static void HX8368_WriteReg(unsigned char SPI_COMMD)
{
	unsigned short SBit,SBuffer;
	unsigned char BitCounter;
	
	SBuffer=SPI_COMMD;
	gpio_direction_output(GPIO_SPI_CS, 0);	
	for(BitCounter=0;BitCounter<9;BitCounter++)
	{
		SBit = SBuffer&0x100;
		if(SBit)
			gpio_direction_output(GPIO_SPI_SDO, 1);
		else
			gpio_direction_output(GPIO_SPI_SDO, 0);
			
		gpio_direction_output(GPIO_SPI_SCLK, 0);
		gpio_direction_output(GPIO_SPI_SCLK, 1);
		SBuffer = SBuffer<<1;
	}
	gpio_direction_output(GPIO_SPI_CS, 1);
}
//***********************************************
//***********************************************
static void HX8368_WriteData(unsigned char SPI_DATA)
{
	unsigned short SBit,SBuffer;
	unsigned char BitCounter;
	
	SBuffer=SPI_DATA | 0x100;
	gpio_direction_output(GPIO_SPI_CS, 0);//Set_CS(0); //CLR CS
	
	for(BitCounter=0;BitCounter<9;BitCounter++)
	{
		SBit = SBuffer&0x100;
		if(SBit)
			gpio_direction_output(GPIO_SPI_SDO, 1);//Set_SDA(1);
		else
			gpio_direction_output(GPIO_SPI_SDO, 0);//Set_SDA(0);
			
		gpio_direction_output(GPIO_SPI_SCLK, 0);//Set_SCK(0); //CLR SCL
		gpio_direction_output(GPIO_SPI_SCLK, 1);//Set_SCK(1); //SET SCL
		SBuffer = SBuffer<<1;
	}
	gpio_direction_output(GPIO_SPI_CS, 1);//Set_CS(1); //SET CS
}
//***********************************************************
//***********************************************************
static void lcdc_lead_init(void)
{ 
		HX8368_WriteReg(0x11); 
		msleep(120); 
		HX8368_WriteReg(0xB9);		  // Set EXTC 
		HX8368_WriteData(0xFF);   
		HX8368_WriteData(0x83);   
		HX8368_WriteData(0x68);   
		msleep(5); 
		HX8368_WriteReg(0xE3); 
		HX8368_WriteData(0x00); 
		HX8368_WriteData(0x60); 
		HX8368_WriteData(0x00); 
		HX8368_WriteData(0x60); 
		msleep(5);						   
		HX8368_WriteReg(0xBB); // Set OTP 
		HX8368_WriteData(0x00); 
		HX8368_WriteData(0x00); 
		HX8368_WriteData(0x80); 
		msleep(5); 
		HX8368_WriteReg(0xEA);		  // Set SETMESSI 
		HX8368_WriteData(0x00);   
		msleep(5); 
		HX8368_WriteReg(0xB3); 
		HX8368_WriteData(0x0E);//0x0F 
		HX8368_WriteData(0x08); 
		HX8368_WriteData(0x02); 
		msleep(5); 
		HX8368_WriteReg(0xC0);		  // Set OPON 
		HX8368_WriteData(0x30); 
		HX8368_WriteData(0x05); 
		HX8368_WriteData(0x09); 
		HX8368_WriteData(0x82);   
		msleep(5);			 
		HX8368_WriteReg(0xB6); // Set VCOM 
		HX8368_WriteData(0x85); 
		HX8368_WriteData(0x34); 
		HX8368_WriteData(0x64); 
		// Set Gamma 2.2 
		HX8368_WriteReg(0xE0); // Set Gamma 
		HX8368_WriteData(0x00); 
		HX8368_WriteData(0x29); 
		HX8368_WriteData(0x28); 
		HX8368_WriteData(0x3F); 
		HX8368_WriteData(0x3F); 
		HX8368_WriteData(0x3F); 
		HX8368_WriteData(0x25); 
		HX8368_WriteData(0x7F); 
		HX8368_WriteData(0x09); 
		HX8368_WriteData(0x07); 
		HX8368_WriteData(0x09); 
		HX8368_WriteData(0x0F); 
		HX8368_WriteData(0x1F); 
		HX8368_WriteData(0x00); 
		HX8368_WriteData(0x00); 
		HX8368_WriteData(0x00); 
		HX8368_WriteData(0x17); 
		HX8368_WriteData(0x16); 
		HX8368_WriteData(0x3F); 
		HX8368_WriteData(0x00); 
		HX8368_WriteData(0x5A); 
		HX8368_WriteData(0x00); 
		HX8368_WriteData(0x10); 
		HX8368_WriteData(0x16); 
		HX8368_WriteData(0x18); 
		HX8368_WriteData(0x17); 
		HX8368_WriteData(0xFF); 

		HX8368_WriteReg(0x3a); /// 
		HX8368_WriteData(0x60); 		
		msleep(5); 
		HX8368_WriteReg(0x36); /// 
		HX8368_WriteData(0x08);   
		msleep(5); 
		HX8368_WriteReg(0x29); 
		
		printk("lcd module TFT LEAD init finish\n!"); 
}


//ZTE_LCD_LKEJ_20110216_001,start
static void lcdc_yassy_init(void)
{
	int aa1; 
	HX8368_WriteReg(0x11);	
	msleep(120); 

	HX8368_WriteReg(0xB9);
	HX8368_WriteData(0xFF);
	HX8368_WriteData(0x83);
	HX8368_WriteData(0x68);
	msleep(20);
	HX8368_WriteReg(0xB3);
	HX8368_WriteData(0x0E);
	HX8368_WriteReg(0x36);
	HX8368_WriteData(0X08);//c8

	HX8368_WriteReg(0x3A);
	HX8368_WriteData(0x60);

	HX8368_WriteReg(0xEA);
	HX8368_WriteData(0x02);
	msleep(20);
	HX8368_WriteReg(0x2D);
	for(aa1=0;aa1<32;aa1++)
	{
	HX8368_WriteData(2*aa1);
	}
	for(aa1=0;aa1<64;aa1++)
	{
	HX8368_WriteData(1*aa1);
	}
	for(aa1=0;aa1<64;aa1++)
	{
	HX8368_WriteData(2*aa1);
	}
	msleep(50);
	HX8368_WriteReg(0xC0);
	HX8368_WriteData(0x1B);
	HX8368_WriteData(0x05);
	HX8368_WriteData(0x08);
	HX8368_WriteData(0xEC);
	HX8368_WriteData(0x00);
	HX8368_WriteData(0x01);

	HX8368_WriteReg(0xE3);
	HX8368_WriteData(0x00);
	HX8368_WriteData(0x4F);
	HX8368_WriteData(0x00);
	HX8368_WriteData(0x4F);

	HX8368_WriteReg(0xB1); //POWER SETTING
	HX8368_WriteData(0x00);
	HX8368_WriteData(0x02);
	HX8368_WriteData(0x1E);
	HX8368_WriteData(0x04);
	HX8368_WriteData(0x22);
	HX8368_WriteData(0x11);
	HX8368_WriteData(0xD4);

	HX8368_WriteReg(0xB6);
	HX8368_WriteData(0x81); //71 81
	HX8368_WriteData(0x6F); //93 6F
	HX8368_WriteData(0x50); //74 50

	HX8368_WriteReg(0xB0);
	HX8368_WriteData(0x0F);
	
	HX8368_WriteReg(0xE0);
	HX8368_WriteData(0x00); //V63
	HX8368_WriteData(0x21); //V62
	HX8368_WriteData(0x20); //V61
	HX8368_WriteData(0x1F); //V2
	HX8368_WriteData(0x21); //V1
	HX8368_WriteData(0x3F); //V0
	HX8368_WriteData(0x10); //V55
	HX8368_WriteData(0x57); //V8
	HX8368_WriteData(0x02); //V60
	HX8368_WriteData(0x05); //V43
	HX8368_WriteData(0x08); //V32
	HX8368_WriteData(0x0D); //V20
	HX8368_WriteData(0x15); //V3
	HX8368_WriteData(0x00); //V0
	HX8368_WriteData(0x1E); //V1
	HX8368_WriteData(0x20); //V2
	HX8368_WriteData(0x1F); //V61
	HX8368_WriteData(0x1E); //V62
	HX8368_WriteData(0x3F); //V63
	HX8368_WriteData(0x28); //V8
	HX8368_WriteData(0x6F); //V55
	HX8368_WriteData(0x1a); //V3
	HX8368_WriteData(0x12); //V20
	HX8368_WriteData(0x17); //V32
	HX8368_WriteData(0x1A); //V43
	HX8368_WriteData(0x1D); //V60
	HX8368_WriteData(0xFF); //
	msleep(50);
	HX8368_WriteReg(0x35);//FMKAR ON
	HX8368_WriteData(0x00); //V63
	HX8368_WriteReg(0x29);
	msleep(50);
	HX8368_WriteReg(0x2C);
	printk("lcd module TFT YASSY init finish\n!");
}
//ZTE_LCD_LKEJ_20110216_001,end
static void spi_init(void)
{
	spi_sclk = *(lcdc_tft_pdata->gpio_num);
	spi_cs   = *(lcdc_tft_pdata->gpio_num + 1);
	spi_sdi  = *(lcdc_tft_pdata->gpio_num + 2);
	spi_sdo  = *(lcdc_tft_pdata->gpio_num + 3);
	panel_reset = *(lcdc_tft_pdata->gpio_num + 4);
	printk("spi_init\n!");
	printk("spi_init spi_sclk = %d,spi_cs = %d,spi_sdi = %d,spi_sdo = %d\n",spi_sclk,spi_cs,spi_sdi,spi_sdo);

	gpio_set_value(spi_sclk, 1);
	gpio_set_value(spi_sdo, 1);
	gpio_set_value(spi_cs, 1);
	gpio_direction_input(spi_sdi);
}
void lcdc_yassy_sleep(void)
{
	HX8368_WriteReg(0x28);
	HX8368_WriteReg(0x10);
	mdelay(120);//120MS
}

void lcdc_lead_sleep(void)
{
	HX8368_WriteReg(0x28);
	HX8368_WriteReg(0x10);
	mdelay(120);//120MS
}
static int lcdc_panel_off(struct platform_device *pdev)
{
	printk("lcdc_panel_off , g_lcd_panel_type is %d(1 LEAD. 2 YASSY. )\n",g_lcd_panel_type);
	g_lcd_panel_type = LCD_PANEL_LEAD_QVGA;

	switch(g_lcd_panel_type)
	{
		case LCD_PANEL_YASSY_QVGA:
			lcdc_yassy_sleep();
			break;
		case LCD_PANEL_LEAD_QVGA:
			lcdc_lead_sleep();
			break;	
		default:
			break;
	}
	gpio_direction_output(panel_reset, 0);
	gpio_direction_output(spi_sclk, 0);
	gpio_direction_output(spi_sdi, 0);
	gpio_direction_output(spi_sdo, 0);
	gpio_direction_output(spi_cs, 0);
	return 0;
}
static LCD_PANEL_TYPE lcd_panel_detect(void)
{
	spi_init();
	gpio_direction_input(GPIO_LCD_ID_DETECT);
	if(gpio_get_value(GPIO_LCD_ID_DETECT))
	{
		printk("lead id is LCD_PANEL_LEAD_QVGA");
		return LCD_PANEL_LEAD_QVGA;
	}
	else
	{
		printk("yassy id is LCD_PANEL_YASSY_QVGA");
		return LCD_PANEL_YASSY_QVGA;
	}	
}

void lcd_panel_init(void)
{
	msleep(10);
	gpio_direction_output(panel_reset, 1);
	msleep(10);						////ZTE_LCD_LUYA_20100629_001
	gpio_direction_output(panel_reset, 0);
	msleep(20);						////ZTE_LCD_LUYA_20100629_001
	gpio_direction_output(panel_reset, 1);
	msleep(20);
	switch(g_lcd_panel_type)
	{
		case LCD_PANEL_YASSY_QVGA:
			lcdc_yassy_init();
			break;
		case LCD_PANEL_LEAD_QVGA:
			lcdc_lead_init();
			break;
		default:
			break;
	}
}

static struct msm_fb_panel_data lcdc_tft_panel_data = {
       .panel_info = {.bl_max = 15},//ZTE_LCD_LKEJ_20110425_001
	.on = lcdc_panel_on,
	.off = lcdc_panel_off,
       .set_backlight = lcdc_set_bl,
};

static struct platform_device this_device = {
	.name   = "lcdc_panel_qvga",
	.id	= 1,
	.dev	= {
		.platform_data = &lcdc_tft_panel_data,
	}
};
//ZTE_LCD_LKEJ_20110216_001,start
static int __init lcdc_panel_probe(struct platform_device *pdev)
{
	struct msm_panel_info *pinfo;
	int ret;

	if(pdev->id == 0) 
		{	  
			lcdc_tft_pdata = pdev->dev.platform_data;
			lcdc_tft_pdata->panel_config_gpio(1);  
			g_lcd_panel_type = lcd_panel_detect();
		
		 	if(g_lcd_panel_type==LCD_PANEL_LEAD_QVGA)
			{
				pinfo = &lcdc_tft_panel_data.panel_info;
				pinfo->lcdc.h_back_porch = 20;
				pinfo->lcdc.h_front_porch = 40;
				pinfo->lcdc.h_pulse_width = 10;
				pinfo->lcdc.v_back_porch = 2;
				pinfo->lcdc.v_front_porch = 4;
				pinfo->lcdc.v_pulse_width = 2;
				pinfo->lcdc.border_clr = 0;	/* blk */
				pinfo->lcdc.underflow_clr = 0xffff;	/* blue */
				pinfo->lcdc.hsync_skew = 0;
				printk("@lkej!lcd_panel_lead_probe\n");
			}
			else
			{
				pinfo = &lcdc_tft_panel_data.panel_info;
				pinfo->lcdc.h_back_porch = 20;
				pinfo->lcdc.h_front_porch = 40;
				pinfo->lcdc.h_pulse_width = 10;
				pinfo->lcdc.v_back_porch = 2;
				pinfo->lcdc.v_front_porch = 4;
				pinfo->lcdc.v_pulse_width = 2;
				pinfo->lcdc.border_clr = 0;	/* blk */
				pinfo->lcdc.underflow_clr = 0xffff;	/* blue */
				pinfo->lcdc.hsync_skew = 0;
			}	
			pinfo->xres = 320;
			pinfo->yres = 240;		
			pinfo->type = LCDC_PANEL;
			pinfo->pdest = DISPLAY_1;
			pinfo->wait_cycle = 0;
			pinfo->bpp = 18;
			pinfo->fb_num = 2;
			switch(g_lcd_panel_type)
			{
				case LCD_PANEL_YASSY_QVGA:
					LcdPanleID=(u32)91;   //ZTE_LCD_LHT_20100611_001
					pinfo->clk_rate = 8192000;//ZTE_LCD_LKEJ_20110512_001
					ret = platform_device_register(&this_device);
					break;
				case LCD_PANEL_LEAD_QVGA:
					pinfo->clk_rate = 8192000;//ZTE_LCD_LKEJ_20110512_001
					LcdPanleID=(u32)90;   //ZTE_LCD_LHT_20100611_001
					ret = platform_device_register(&this_device);
					break;
				default:
					break;
			}		
			return 0;	
			}
	msm_fb_add_device(pdev);
	return 0;
}
//ZTE_LCD_LKEJ_20110216_001,end
static struct platform_driver this_driver = {
	.probe  = lcdc_panel_probe,
	.driver = {
		.name   = "lcdc_panel_qvga",
	},
};
static int __init lcdc_tft_panel_init(void)
{
	int ret;

	ret = platform_driver_register(&this_driver);

	return ret;
}

module_init(lcdc_tft_panel_init);

