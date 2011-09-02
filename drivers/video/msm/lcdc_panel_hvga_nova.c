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

#define GPIO_LCD_BL_SC_OUT 57    //97     //changed by zte_gequn091966,20110406

static int lcdc_samsung_regist = 1;
static boolean is_firsttime = true;

static int spi_cs;
static int spi_sclk;
static int spi_sdi;
static int spi_sdo;
static int lcd_panel_reset;
static int lcd_panel_id;
extern u32 LcdPanleID;   //ZTE_LCD_LHT_20100611_001
static bool onewiremode = true;  //added by zte_gequn091966,20110428

typedef enum
{
	LCD_PANEL_NONE = 0,
	LCD_PANEL_LEAD_HVGA,
	LCD_PANEL_TRULY_HVGA,   //added by zte_gequn091966,20110429
}LCD_PANEL_TYPE;

LCD_PANEL_TYPE g_lcd_panel_type = LCD_PANEL_NONE;

//static __u32 last_bl_level = 6;

//static DEFINE_SPINLOCK(tps61061_lock);

static struct msm_panel_common_pdata * lcd_panel_pdata;
static void lcd_panel_sleep(void);
static void lcd_panel_wakeup(void);
static void lcd_panel_init(void);
static void lcdc_set_bl(struct msm_fb_data_type *mfd);

static void spi_init(void);
static int lcdc_panel_on(struct platform_device *pdev);
static int lcdc_panel_off(struct platform_device *pdev);

static boolean bSleepWhenSuspend = false;

//added by zte_gequn091966,20110428
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

/***added by zte_gequn091966,20110429***/
static void R61581B_WriteReg(unsigned char SPI_COMMD)
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

static void R61581B_WriteData(unsigned char SPI_DATA)
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

static void gpio_lcd_truly_R61581B_emuspi_read_one_index(unsigned char SPI_COMMD, unsigned int *data)
{
     unsigned short SBit,SBuffer;
     unsigned char BitCounter;
     int j;
     volatile int i;
     unsigned int dbit,bits1;

    gpio_direction_output(spi_cs, 0);
	
    SBuffer=SPI_COMMD;  
    
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

       gpio_direction_input(spi_sdi);

  bits1 = 0;

     for (j = 0; j < 8; j++)
   	{
	    gpio_direction_output(spi_sclk, 0); 
	    for(i= 0; i < 33;i++)
		{
				
		}  
	   gpio_direction_output(spi_sclk, 1);
	   gpio_get_value(spi_sdi);
	   for(i= 0; i < 33;i++)
		{
				
		}
	
       }

     for (j = 0; j < 8; j++)
   	{
	    gpio_direction_output(spi_sclk, 0); 
	    for(i= 0; i < 33;i++)
		{
				
		}  
	   gpio_direction_output(spi_sclk, 1);
	   gpio_get_value(spi_sdi);
	   for(i= 0; i < 33;i++)
		{
				
		}
	
       }

     for (j = 0; j < 8; j++)
   	{
	    gpio_direction_output(spi_sclk, 0); 
	    for(i= 0; i < 33;i++)
		{
				
		}  
	   gpio_direction_output(spi_sclk, 1);
	   dbit=gpio_get_value(spi_sdi);
	   for(i= 0; i < 33;i++)
{
				
		}
	
	  bits1 = 2*bits1+dbit;
}
   *data =  bits1;
    gpio_direction_output(spi_cs, 1);  
    printk("read R61581B PID***lkej***:0x%x\n!",bits1);
}
/***end added by zte_gequn091966,20110429***/

static void lcd_panel_sleep(void)
{
	switch(g_lcd_panel_type)
		{
			case LCD_PANEL_TRULY_HVGA:
				R61581B_WriteReg(0x10);
				mdelay(10);
				break;
			case LCD_PANEL_LEAD_HVGA:
				ILI9481_WriteReg(0x10);
				break;
			default:
				break;
		}	
}

static void lcd_panel_wakeup(void)
{

	switch(g_lcd_panel_type)
		{
			case LCD_PANEL_TRULY_HVGA:
				R61581B_WriteData(0x11);
				mdelay(120);
				break;
			case LCD_PANEL_LEAD_HVGA:
				ILI9481_WriteReg(0x11);
				ILI9481_WriteReg(0x29);
				break;
			default:
				break;
		}
	
}

static void lcdc_lead_init(void)
{
	//************* Start Initial Sequence **********//
	ILI9481_WriteReg(0x11);
	mdelay(20);//Delay(10*20);
	
	//CPT+ILI9481
	ILI9481_WriteReg(0xC6);
		ILI9481_WriteData(0x9B);///5B

	ILI9481_WriteReg(0xD0);
	ILI9481_WriteData(0x07);
	ILI9481_WriteData(0x41);
	ILI9481_WriteData(0x1A);
	
	ILI9481_WriteReg(0xD1);
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x0B);//10
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
	ILI9481_WriteData(0x02);//3//
	
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
	ILI9481_WriteData(0x09);   //0x0A
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
	ILI9481_WriteData(0x10);//ILI9481_WriteData(0x10);//

	ILI9481_WriteReg(0x29);
	ILI9481_WriteReg(0x2C);
}

/***added by zte_gequn091966,20110429***/
static void lcdc_truly_R61581B_init(void)
{
   R61581B_WriteReg(0xB0);
   R61581B_WriteData(0x00);

   R61581B_WriteReg(0xB3);
   R61581B_WriteData(0x02);
   R61581B_WriteData(0x00);
   R61581B_WriteData(0x00); 
   R61581B_WriteData(0x10);
	
   R61581B_WriteReg(0xB4);
   R61581B_WriteData(0x11);
	
   R61581B_WriteReg(0xC0);
   R61581B_WriteData(0x06);  // gequn for inverse 0x03
   R61581B_WriteData(0x3B);
   R61581B_WriteData(0x00); 
   R61581B_WriteData(0x02);
   R61581B_WriteData(0x00);
   R61581B_WriteData(0x01);
   R61581B_WriteData(0x00); 
   R61581B_WriteData(0x43);
	
   R61581B_WriteReg(0xC1);
   R61581B_WriteData(0x08);
   R61581B_WriteData(0x14);
   R61581B_WriteData(0x04); 
   R61581B_WriteData(0x04);
	
   R61581B_WriteReg(0xC4);
   R61581B_WriteData(0x22);
   R61581B_WriteData(0x03);
   R61581B_WriteData(0x03); 
   R61581B_WriteData(0x00);
	
   R61581B_WriteReg(0xC6);
   R61581B_WriteData(0x1B);  //0x02
	
   R61581B_WriteReg(0xFD);
   R61581B_WriteData(0x00);
   R61581B_WriteData(0x0F);
   R61581B_WriteData(0x17); 
   R61581B_WriteData(0x00);
   R61581B_WriteData(0x1F);
   R61581B_WriteData(0x14); 
   R61581B_WriteData(0x00);
   R61581B_WriteData(0x07);
	
   R61581B_WriteReg(0x21);

   R61581B_WriteReg(0xC8);
   R61581B_WriteData(0x09);
   R61581B_WriteData(0x08);
   R61581B_WriteData(0x0B); 
   R61581B_WriteData(0x85);
   R61581B_WriteData(0x00);
   R61581B_WriteData(0x08); 
   R61581B_WriteData(0x16);
   R61581B_WriteData(0x05);
   R61581B_WriteData(0x00);
   R61581B_WriteData(0x32);
   R61581B_WriteData(0x04); 
   R61581B_WriteData(0x16);
   R61581B_WriteData(0x08);
   R61581B_WriteData(0x88); 
   R61581B_WriteData(0x0C);
   R61581B_WriteData(0x08);
   R61581B_WriteData(0x00);
   R61581B_WriteData(0x06);
   R61581B_WriteData(0x32);
   R61581B_WriteData(0x00);

   R61581B_WriteReg(0x2A);
   R61581B_WriteData(0x00);
   R61581B_WriteData(0x00);
   R61581B_WriteData(0x01); 
   R61581B_WriteData(0x3F);

   R61581B_WriteReg(0x2B);
   R61581B_WriteData(0x00);
   R61581B_WriteData(0x00);
   R61581B_WriteData(0x01); 
   R61581B_WriteData(0xDF);

   R61581B_WriteReg(0x35);
   R61581B_WriteData(0x00);
   
//added by gequn for inverse
   R61581B_WriteReg(0x36);
   R61581B_WriteData(0x00);
   
   R61581B_WriteReg(0x3A);
   R61581B_WriteData(0x60);

   R61581B_WriteReg(0x44);
   R61581B_WriteData(0x00);
   R61581B_WriteData(0x01);

   R61581B_WriteReg(0x2C);
   R61581B_WriteReg(0x11);

   mdelay(150);

   R61581B_WriteReg(0xD0);
   R61581B_WriteData(0x07);
   R61581B_WriteData(0x07);
   R61581B_WriteData(0x18); 
   R61581B_WriteData(0x71);

   R61581B_WriteReg(0xD1);
   R61581B_WriteData(0x03);
   R61581B_WriteData(0x43);
   R61581B_WriteData(0x11); 

   R61581B_WriteReg(0xD2);
   R61581B_WriteData(0x02);
   R61581B_WriteData(0x00);
   R61581B_WriteData(0x02); 
 
   R61581B_WriteReg(0x29);
   mdelay(10);
   R61581B_WriteReg(0x2C);
}
/***end added by zte_gequn091966,20110429***/

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
		case LCD_PANEL_TRULY_HVGA:
			lcdc_truly_R61581B_init();
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
       .panel_info = {.bl_max = 32},
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
      unsigned int id_h;
      //R61581B_WriteReg(0xB0);
      //R61581B_WriteData(0x00);
      gpio_lcd_truly_R61581B_emuspi_read_one_index(0xBF,&id_h);

	if(id_h == 0x22)
		return LCD_PANEL_TRULY_HVGA;
	else
		return LCD_PANEL_LEAD_HVGA;

}

static int __init lcdc_panel_probe(struct platform_device *pdev)
{
	struct msm_panel_info *pinfo;
	int ret;

	if(pdev->id == 0) {
		printk("use lead 320x480 panel driver!\n");
		lcd_panel_pdata = pdev->dev.platform_data;
		lcd_panel_pdata->panel_config_gpio(1);
		spi_init();
		g_lcd_panel_type = lcd_panel_detect();

		if(g_lcd_panel_type == LCD_PANEL_TRULY_HVGA )
			{
			   R61581B_WriteReg(0x2C);

			}

		switch(g_lcd_panel_type)
		{
			case LCD_PANEL_TRULY_HVGA:
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
		switch(g_lcd_panel_type)
		{
		  case LCD_PANEL_TRULY_HVGA:
		  	
		   pinfo->lcdc.h_back_porch = 3;
		   pinfo->lcdc.h_front_porch = 3;
		   pinfo->lcdc.h_pulse_width = 3;
		   pinfo->lcdc.v_back_porch = 2;
		   pinfo->lcdc.v_front_porch = 10;
		   pinfo->lcdc.v_pulse_width = 2;
		  break;
		  
                case LCD_PANEL_LEAD_HVGA:
		
		pinfo->lcdc.h_back_porch = 3;
		pinfo->lcdc.h_front_porch = 3;
		pinfo->lcdc.h_pulse_width = 3;
		pinfo->lcdc.v_back_porch = 2;
		pinfo->lcdc.v_front_porch = 4;
		pinfo->lcdc.v_pulse_width = 2;
		  break;
		  default:
		  break;
		}
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

