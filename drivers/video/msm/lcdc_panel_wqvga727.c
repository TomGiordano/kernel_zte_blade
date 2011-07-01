/* drivers/video/msm/src/panel/lcdc/lcdc_panel.c
 *
 * Copyright (C) 2008 QUALCOMM USA, INC.
 * 
 * All source code in this file is licensed under the following license
 * except where indicated.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can find it at http://www.fsf.org
 */
 /* 
======================================================
when         who           what, where, why                          comment tag
--------     ----          -------------------------------------    ------------------------------
2010-06-11   lht		   project mode display panel info         	ZTE_LCD_LHT_20100611_001
2010-06-09   lht		   change max bkl level to 25           	ZTE_LCD_LHT_20100609_001
2010-05-21   lht		   early suspand current adjust           	ZTE_LCD_LHT_20100521_001
2010-05-19   lht		   sleep current adjust           			ZTE_LCD_LHT_20100519_001
2010-03-19   liuhaitao     for lcd dithering while shake 			ZTE_LCD_LHT_20100319_001
2010-03-01   liuhaitao     for sleep/wakeup bkl black
2010-01-26   luya		   fix white-screen(permanent) when wakeup	ZTE_LCD_LUYA_20100126_001
2010-01-14   luya		   fix white-screen when wakeup(cause:bkl 
						   on before lcd)							ZTE_LCD_LUYA_20100114_001
2009-12-18   lixiongwei    fix the lcd wave problem                 ZTE_BACKLIGHT_LXW_001
2009-11-28   hp            fix transient white screen               ZTE_BACKLIGHT_HP_001
2009-11-27   weilanying    ajust backlight                          ZTE_BACKLIGHT_WLY_004 
2009-10-30   weilanying    backlight go to dim                      ZTE_BACKLIGHT_WLY_002 
2009-10-31   weilanying    backlight set 32 class                   ZTE_BACKLIGHT_WLY_003
=======================================================
*/

#include "msm_fb.h"
#include <asm/gpio.h>
#include <linux/module.h>
#include <linux/delay.h>
#ifdef CONFIG_MACH_JOECDMA
#define GPIO_LCD_BL_PWM_OUT  97
#else
#define GPIO_LCD_BL_PWM_OUT  57 
#endif
#define GPIO_LCD_BL_EN_OUT   38   //modified for P722,different LCD BL drive ic

#define LCD_BL_LEVEL 32
#define lcd_bl_max   lcd_bl_level-1
#define lcd_bl_min   0

extern u32 LcdPanleID;   

static DEFINE_SPINLOCK(tps61061_lock);

static int himax_panel_initialized = 1;
static int isnew=0;
static int spi_cs;
static int spi_sclk;
static int spi_sdi;
static int spi_sdo;
static int himax_reset;


static struct msm_panel_common_pdata * lcdc_himax_pdata;
static void gpio_lcd_emuspi_write_one_para(unsigned char addr, unsigned char para);
static void gpio_lcd_emuspi_write_cmd(unsigned char addr, unsigned char para);
static void gpio_lcd_emuspi_read_one_para(unsigned char addr, unsigned int* para);
static void ILI9481_WriteReg(unsigned char SPI_COMMD);
static void ILI9481_WriteData(unsigned char SPI_DATA);
#if 0			/
static void lcdc_himax_wakeup(void);
#endif
static void lcdc_himax_sleep(void);
static void lcdc_himax_init(void);
/*static void lcdc_set_bl(int bl_lvl);*/

static void lcdc_himax_n_sleep(void);
static void lcdc_himax_n_init(void);
static void lcdc_ili_sleep(void);
static void lcdc_ili_init(void);
static void spi_init(void);
static int lcdc_panel_on(struct platform_device *pdev);
static int lcdc_panel_off(struct platform_device *pdev);
static void lcdc_ili_sleep(void)
{
	ILI9481_WriteReg(0x10);
	ILI9481_WriteReg(0x10);
}
static void lcdc_ili_init(void)
{
	gpio_direction_output(himax_reset, 1);
	msleep(5);
	gpio_direction_output(himax_reset, 0);
	msleep(10);
	gpio_direction_output(himax_reset, 1);
	msleep(20);

	gpio_direction_output(spi_cs, 1);
	gpio_direction_output(spi_sdo, 1);
	gpio_direction_output(spi_sclk, 1);
	msleep(20);
#if 1
	ILI9481_WriteReg(0xE9);
	ILI9481_WriteData(0x20);
	ILI9481_WriteReg(0x11);//EXIT Sleep
	msleep(100);
	
	ILI9481_WriteReg(0xD1);
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x68);//SET VCOMH
	ILI9481_WriteData(0x1a);//SET VCOM
	
	ILI9481_WriteReg(0xD0);
	ILI9481_WriteData(0x07);
	ILI9481_WriteData(0x01);
	ILI9481_WriteData(0x8b);

	ILI9481_WriteReg(0x36);
	ILI9481_WriteData(0x48);

	ILI9481_WriteReg(0x3A);
	ILI9481_WriteData(0x66);//0x66, CPU18,0x55 CPU16

	ILI9481_WriteReg(0xC1);
	ILI9481_WriteData(0x10);//LINE INVERSION
	ILI9481_WriteData(0x10);
	ILI9481_WriteData(0x02);
	ILI9481_WriteData(0x02);

	ILI9481_WriteReg(0xC0);
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x35);
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x01);
	ILI9481_WriteData(0x02);

    ILI9481_WriteReg(0xC5);//Set frame rate
	ILI9481_WriteData(0x02);//72Hz
	
	ILI9481_WriteReg(0xC6);
	ILI9481_WriteData(0x9B);///5B

	ILI9481_WriteReg(0xD2);
	ILI9481_WriteData(0x01);
	ILI9481_WriteData(0x22);

	ILI9481_WriteReg(0xC8);//Gamma setting
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x47);
	ILI9481_WriteData(0x33);
	ILI9481_WriteData(0x10);
	ILI9481_WriteData(0x0C);
	ILI9481_WriteData(0x06);
    ILI9481_WriteData(0x44);
	ILI9481_WriteData(0x03);
	ILI9481_WriteData(0x77);
	ILI9481_WriteData(0x21);
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x10);
	ILI9481_WriteData(0x08);
	ILI9481_WriteData(0x80);
	ILI9481_WriteData(0x00);

	ILI9481_WriteReg(0xB4);
	ILI9481_WriteData(0x11);
	
    ILI9481_WriteReg(0x29);//display on
    ILI9481_WriteReg(0x2C);	
	#endif 
	
#if 0
	ILI9481_WriteReg(0xE9);
	ILI9481_WriteData(0x20);
	ILI9481_WriteReg(0x11);//EXIT Sleep
	msleep(100);
	
	ILI9481_WriteReg(0xD1);
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x71);//SET VCOMH
	ILI9481_WriteData(0x19);//SET VCOM
	
	ILI9481_WriteReg(0xD0);
	ILI9481_WriteData(0x07);
	ILI9481_WriteData(0x01);
	ILI9481_WriteData(0x08);

	ILI9481_WriteReg(0x36);
	ILI9481_WriteData(0x48);

	ILI9481_WriteReg(0x3A);
	ILI9481_WriteData(0x66);//0x66, CPU18,0x55 CPU16

	ILI9481_WriteReg(0xC1);
	ILI9481_WriteData(0x10);//LINE INVERSION
	ILI9481_WriteData(0x10);
	ILI9481_WriteData(0x02);
	ILI9481_WriteData(0x02);

	ILI9481_WriteReg(0xC0);
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x35);
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x01);
	ILI9481_WriteData(0x02);

    ILI9481_WriteReg(0xC5);//Set frame rate
	ILI9481_WriteData(0x04);//72Hz
	
	ILI9481_WriteReg(0xC6);
	ILI9481_WriteData(0x9B);///5B

	ILI9481_WriteReg(0xD2);
	ILI9481_WriteData(0x01);
	ILI9481_WriteData(0x44);

	ILI9481_WriteReg(0xC8);//Gamma setting
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x47);
	ILI9481_WriteData(0x33);
	ILI9481_WriteData(0x09);
	ILI9481_WriteData(0x0C);
	ILI9481_WriteData(0x06);
    ILI9481_WriteData(0x44);
	ILI9481_WriteData(0x03);
	ILI9481_WriteData(0x77);
	ILI9481_WriteData(0x50);
	ILI9481_WriteData(0x00);
	ILI9481_WriteData(0x10);
	ILI9481_WriteData(0x08);
	ILI9481_WriteData(0x80);
	ILI9481_WriteData(0x00);

	ILI9481_WriteReg(0xB4);
	ILI9481_WriteData(0x11);
	
    ILI9481_WriteReg(0x29);//display on
    ILI9481_WriteReg(0x2C);	
	#endif 
}
/*#if 0*/
static void lcdc_himax_sleep(void)
{
//Display off     
     
         gpio_lcd_emuspi_write_one_para(0x24, 0x38);//GON=1;DTE=1;D=10
         msleep(40);
         gpio_lcd_emuspi_write_one_para(0x24, 0x28);//GON=1;DTE=0;D=10
         msleep(40);
         gpio_lcd_emuspi_write_one_para(0x24, 0x00);//GON=0;DTE=0;D=00
         
//Power off 

         gpio_lcd_emuspi_write_one_para(0x1E, 0x14);//VCOMG=0;VDV=1_0100
         msleep(10);
         gpio_lcd_emuspi_write_one_para(0x19, 0x02);//GASENB=0;PON=0;DK=0;XDK=0;VLCD_TRI=1;STB=0
         msleep(10);
         gpio_lcd_emuspi_write_one_para(0x19, 0x0A);//GASENB=0;PON=0;DK=1;XDK=0;VLCD_TRI=1;STB=0
         msleep(10);
         
         gpio_lcd_emuspi_write_one_para(0x1B, 0x40);//AP=000
         msleep(10);
         gpio_lcd_emuspi_write_one_para(0x3C, 0x00);//N_SAP=1100,0000
         msleep(10);
         
//Into Sleep mode

         gpio_lcd_emuspi_write_one_para(0x19, 0x0B);//GASENB=0;PON=0;DK=0;XDK=0;VLCD_TRI=1;STB=1
         msleep(10);
         
//Stop OSC   CPU interface 

         gpio_lcd_emuspi_write_one_para(0x17, 0x90);//RADJ=1001,OSC_EN=0
         msleep(10);                
  

}


static void lcdc_himax_init(void)
{
	gpio_direction_output(himax_reset, 1);
	msleep(5);
	gpio_direction_output(himax_reset, 0);
	msleep(10);
	gpio_direction_output(himax_reset, 1);
	msleep(20);

	gpio_direction_output(spi_cs, 1);
	gpio_direction_output(spi_sdo, 1);
	gpio_direction_output(spi_sclk, 1);
	msleep(20);
	//**********************Start initial squence******************
	gpio_lcd_emuspi_write_one_para(0x83, 0x02); 
	gpio_lcd_emuspi_write_one_para(0x85, 0x03); 
	gpio_lcd_emuspi_write_one_para(0x8B, 0x00); 
	gpio_lcd_emuspi_write_one_para(0x8C, 0x13); 
	gpio_lcd_emuspi_write_one_para(0x91, 0x01); 
	gpio_lcd_emuspi_write_one_para(0x83, 0x00); 
	mdelay(5);
	 
	//------------  Gamma Setting  -------------------------------
	gpio_lcd_emuspi_write_one_para(0x3E, 0xe2);//0xc4
	gpio_lcd_emuspi_write_one_para(0x3F, 0x26);//0x44
	gpio_lcd_emuspi_write_one_para(0x40, 0x00);//0x22
	gpio_lcd_emuspi_write_one_para(0x41, 0x55);//0x57
	gpio_lcd_emuspi_write_one_para(0x42, 0x06);//0x03
	gpio_lcd_emuspi_write_one_para(0x43, 0x17);//0x47
	gpio_lcd_emuspi_write_one_para(0x44, 0x21);//0x02
	gpio_lcd_emuspi_write_one_para(0x45, 0x77);//0x55
	gpio_lcd_emuspi_write_one_para(0x46, 0x01);//0x06
	gpio_lcd_emuspi_write_one_para(0x47, 0x0a);//0x4c
	gpio_lcd_emuspi_write_one_para(0x48, 0x05);//0x06
	gpio_lcd_emuspi_write_one_para(0x49, 0x02);//0x8c

	//-----------  Power Supply Setting --------------------------
	gpio_lcd_emuspi_write_one_para(0x2B, 0xF9); 
	msleep(20);

	gpio_lcd_emuspi_write_one_para(0x17, 0x91); 
	gpio_lcd_emuspi_write_one_para(0x18, 0x3A); //0x3a
	gpio_lcd_emuspi_write_one_para(0x1B, 0x13);//0x14 
	gpio_lcd_emuspi_write_one_para(0x1A, 0x11); 
	gpio_lcd_emuspi_write_one_para(0x1C, 0x0a); //0x0a
	gpio_lcd_emuspi_write_one_para(0x1F, 0x58); 
/*
	gpio_lcd_emuspi_write_one_para(0x17, 0x90); 
	gpio_lcd_emuspi_write_one_para(0x18, 0x3A); //0x3a
	gpio_lcd_emuspi_write_one_para(0x1B, 0x11);//0x14 
	gpio_lcd_emuspi_write_one_para(0x1A, 0x11); 
	gpio_lcd_emuspi_write_one_para(0x1C, 0x06); //0x0a
	gpio_lcd_emuspi_write_one_para(0x1F, 0x57); 
*/
// ZTE_LCD_LHT_20100319_001 END
	msleep(30);
	gpio_lcd_emuspi_write_one_para(0x19, 0x0A);             
	gpio_lcd_emuspi_write_one_para(0x19, 0x1A); 
	msleep(50);
	gpio_lcd_emuspi_write_one_para(0x19, 0x12); 
	msleep(50);
	gpio_lcd_emuspi_write_one_para(0x1E, 0x2e);//0x35 
	msleep(100);

	//Display ON Setting
	gpio_lcd_emuspi_write_one_para(0x3C, 0xC0); 
	gpio_lcd_emuspi_write_one_para(0x3D, 0x1C); 
	gpio_lcd_emuspi_write_one_para(0x34, 0x38);
	gpio_lcd_emuspi_write_one_para(0x35, 0x38);
	gpio_lcd_emuspi_write_one_para(0x24, 0x38);
	msleep(50);
	gpio_lcd_emuspi_write_one_para(0x24, 0x3C);
	gpio_lcd_emuspi_write_one_para(0x16, 0x1C); 
	/*gpio_lcd_emuspi_write_one_para(0x3A, 0xC0); // RGB interface control*/
	gpio_lcd_emuspi_write_one_para(0x3A, 0xce); // RGB interface control
	gpio_lcd_emuspi_write_one_para(0x01, 0x06); 
	gpio_lcd_emuspi_write_one_para(0x55, 0x00);



}
static void lcdc_himax_n_sleep(void)
{
	gpio_lcd_emuspi_write_one_para(0x28,0x3c);
	msleep(10);
	gpio_lcd_emuspi_write_one_para(0x28,0x38);
	msleep(40);
	gpio_lcd_emuspi_write_one_para(0x28,0x20);
	msleep(10);
	// Power off
    gpio_lcd_emuspi_write_one_para(0x1f,0x94);
    msleep(10);
    gpio_lcd_emuspi_write_one_para(0x1F,0x84);
 	msleep(10);
  	gpio_lcd_emuspi_write_one_para(0x1F,0x8c);
 	//into STB mode
 	msleep(10);
    gpio_lcd_emuspi_write_one_para(0x1F,0x8d);
        
	msleep(10);
   	//stop oscillation
    gpio_lcd_emuspi_write_one_para(0x19,0x00);            
  	msleep(50);

}


static void lcdc_himax_n_init(void)
{
unsigned int id;
	gpio_direction_output(himax_reset, 1);
	msleep(5);
	gpio_direction_output(himax_reset, 0);
	msleep(10);
	gpio_direction_output(himax_reset, 1);
	msleep(20);

	gpio_direction_output(spi_cs, 1);
	gpio_direction_output(spi_sdo, 1);
	gpio_direction_output(spi_sclk, 1);
	msleep(20);
	//**********************Start initial squence******************
	gpio_lcd_emuspi_read_one_para(0x00,&id);
      //Start initial Sequence
       	msleep(10); 
	// Function setting
        gpio_lcd_emuspi_write_one_para(0xE2,0x15); 
        gpio_lcd_emuspi_write_one_para(0xE5,0x18);
        gpio_lcd_emuspi_write_one_para(0xE7,0x18);
        gpio_lcd_emuspi_write_one_para(0xE8,0x64);
        gpio_lcd_emuspi_write_one_para(0xEC,0x08);
        gpio_lcd_emuspi_write_one_para(0xED,0x47);
        gpio_lcd_emuspi_write_one_para(0xEE,0x20);
		gpio_lcd_emuspi_write_one_para(0xEF,0x50);
        // Power on Setting
        
        gpio_lcd_emuspi_write_one_para(0x23,0x83);
        gpio_lcd_emuspi_write_one_para(0x24,0x79);
        gpio_lcd_emuspi_write_one_para(0x25,0x4F);
        gpio_lcd_emuspi_write_one_para(0x29,0x00);
        gpio_lcd_emuspi_write_one_para(0x2B,0x03);
        gpio_lcd_emuspi_write_one_para(0x1B,0x1E);
        
        // Power on sequence
		
        gpio_lcd_emuspi_write_one_para(0x01,0x00);
        gpio_lcd_emuspi_write_one_para(0x1C,0x03);
        gpio_lcd_emuspi_write_one_para(0x19,0x01);
        msleep(5);
        gpio_lcd_emuspi_write_one_para(0x1F,0x90);
 		msleep(10);
        gpio_lcd_emuspi_write_one_para(0x1F,0xd4);
        msleep(10);
        msleep(5);
       
       // Gamma Setting

        gpio_lcd_emuspi_write_one_para(0x40,0x00);
        gpio_lcd_emuspi_write_one_para(0x41,0x29);
        gpio_lcd_emuspi_write_one_para(0x42,0x26);
        gpio_lcd_emuspi_write_one_para(0x43,0x3E);
        gpio_lcd_emuspi_write_one_para(0x44,0x3D);
        gpio_lcd_emuspi_write_one_para(0x45,0x3F);        
       	gpio_lcd_emuspi_write_one_para(0x46,0x1b);  
       	gpio_lcd_emuspi_write_one_para(0x47,0x68);        
       	gpio_lcd_emuspi_write_one_para(0x48,0x04);        
       	gpio_lcd_emuspi_write_one_para(0x49,0x05);        
       	gpio_lcd_emuspi_write_one_para(0x4A,0x06);        
       	gpio_lcd_emuspi_write_one_para(0x4B,0x0c);      
       	gpio_lcd_emuspi_write_one_para(0x4C,0x17);       
        gpio_lcd_emuspi_write_one_para(0x50,0x00);
        gpio_lcd_emuspi_write_one_para(0x51,0x02);
        gpio_lcd_emuspi_write_one_para(0x52,0x01);
        gpio_lcd_emuspi_write_one_para(0x53,0x19);
        gpio_lcd_emuspi_write_one_para(0x54,0x16);
        gpio_lcd_emuspi_write_one_para(0x55,0x3F);        
       	gpio_lcd_emuspi_write_one_para(0x56,0x17);   
       	gpio_lcd_emuspi_write_one_para(0x57,0x64);        
       	gpio_lcd_emuspi_write_one_para(0x58,0x08);        
       	gpio_lcd_emuspi_write_one_para(0x59,0x13);        
       	gpio_lcd_emuspi_write_one_para(0x5A,0x19);        
       	gpio_lcd_emuspi_write_one_para(0x5B,0x1a);
       	gpio_lcd_emuspi_write_one_para(0x5C,0x1b);
		gpio_lcd_emuspi_write_one_para(0x5D,0xFF);
 

       	gpio_lcd_emuspi_write_one_para(0x28,0x20);
		msleep(40);   
		gpio_lcd_emuspi_write_one_para(0x28,0x38);
       	msleep(40);  
       	gpio_lcd_emuspi_write_one_para(0x28,0x3C);
   		gpio_lcd_emuspi_write_one_para(0x17,0x05); //05  
       	gpio_lcd_emuspi_write_one_para(0x16,0x49);  
	

		gpio_lcd_emuspi_write_one_para(0x31,0x02);  
		gpio_lcd_emuspi_write_one_para(0x32,0x0e); 
		gpio_lcd_emuspi_write_cmd(0x22,0x83);
		msleep(200);

}



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


    pr_info("[ZYF] lcdc_set_bl level=%d, %d\n", 
		   new_bl_level , mfd->panel_power_on);

    if(!mfd->panel_power_on)
    {
    	gpio_direction_output(GPIO_LCD_BL_PWM_OUT,0);   //20100301 liuhaitao for sleep/wakeup bkl black
	    return;	
    }

		
    if(new_bl_level < 1)
    {
        new_bl_level = 0;
    }
    if (new_bl_level > 25)   
    {
        new_bl_level = 25;
    }

    if(0 == new_bl_level)
    {
        	   gpio_direction_output(GPIO_LCD_BL_PWM_OUT,0);    
		   /* at least delay 720us to shut off */  
    		   udelay(800); 
    } 
   else 
   {
          if(0 == last_bl_level)
          {
		  	gpio_direction_output(GPIO_LCD_BL_PWM_OUT,1);
			/* at least delay 1.5us between steps */
			udelay(5); 
			last_bl_level = 16;
          }  
		  
	   if(new_bl_level == last_bl_level)	return;
		
	   if(new_bl_level > last_bl_level)   //need increasing feedback vlotage
	   {
		        diff = new_bl_level - last_bl_level;
		        for(cnt=0;cnt < diff;cnt++)
		        {  
		           spin_lock_irqsave(&tps61061_lock, flags);  //lock 
				   
			    gpio_direction_output(GPIO_LCD_BL_PWM_OUT,0);
			     /* delay 1--75us lcd-backlight brightness increase */    
			    udelay(10);
			    gpio_direction_output(GPIO_LCD_BL_PWM_OUT,1);  
				
			    spin_unlock_irqrestore(&tps61061_lock, flags); //unlock
				
			     /* at least delay 1.5us between steps */    
			    udelay(3); 
		        }     	  
	   }
	   else   //need decreasing feedback vlotage
	   {
		        diff = last_bl_level - new_bl_level;
		        for(cnt=0;cnt < diff;cnt++)
		        {  
		           spin_lock_irqsave(&tps61061_lock, flags);   //lock 
				   
			    gpio_direction_output(GPIO_LCD_BL_PWM_OUT,0);
			     /* delay 180 --300us lcd-backlight brightness decrease */    
			    udelay(200);
			    gpio_direction_output(GPIO_LCD_BL_PWM_OUT,1);  
				
			    spin_unlock_irqrestore(&tps61061_lock, flags); //unlock
				
			     /* at least delay 1.5us between steps */    
			    udelay(3); 
		        }     
	   }
   }

    last_bl_level = new_bl_level; //record new level value
	
}


static void spi_init(void)
{
	/* Setting the Default GPIO's */
	spi_sclk = *(lcdc_himax_pdata->gpio_num);
	spi_cs   = *(lcdc_himax_pdata->gpio_num + 1);
	spi_sdi  = *(lcdc_himax_pdata->gpio_num + 2);
	spi_sdo  = *(lcdc_himax_pdata->gpio_num + 3);
	himax_reset = *(lcdc_himax_pdata->gpio_num + 4);

	/* Set the output so that we dont disturb the slave device */
	gpio_set_value(spi_sclk, 0);
	gpio_set_value(spi_sdi, 0);

	/* Set the Chip Select De-asserted */
	gpio_set_value(spi_cs, 0);

}

static int lcdc_panel_on(struct platform_device *pdev)
{

	spi_init();   
/
	if(himax_panel_initialized==1) 
	{
		himax_panel_initialized = 0;
	}
	else 
	{
		//spi_init();
		if(isnew==1)
		lcdc_himax_init();
		else if(isnew==2)
		lcdc_ili_init();
		else
		lcdc_himax_n_init();
	}
	return 0;
}

static int lcdc_panel_off(struct platform_device *pdev)
{
		if(isnew==1)
		lcdc_himax_sleep();
		else if (isnew==2)
		lcdc_ili_sleep();
		else
		lcdc_himax_n_sleep();
		gpio_direction_output(himax_reset, 0);
		mdelay(200);
		
		gpio_direction_output(spi_sclk, 0);
		gpio_direction_output(spi_sdi, 0);
		gpio_direction_output(spi_sdo, 0);
		gpio_direction_output(spi_cs, 0);
		himax_panel_initialized = 0;
	return 0;
}

static void gpio_lcd_emuspi_write_cmd(unsigned char addr, unsigned char para)
{
	unsigned short  i;
	int j;

	i = addr | 0x7000;
	gpio_direction_output(spi_cs, 0);

	for (j = 0; j < 16; j++) {

		if ((i << j) & 0x8000)
			gpio_direction_output(spi_sdo, 1);	
		else
			gpio_direction_output(spi_sdo, 0);	

		gpio_direction_output(spi_sclk, 0);	
		gpio_direction_output(spi_sclk, 1);	
	}
	gpio_direction_output(spi_cs, 1);
	

}


static void gpio_lcd_emuspi_write_one_para(unsigned char addr, unsigned char para)
{
	unsigned short  i;
	int j;

	i = addr | 0x7000;
	gpio_direction_output(spi_cs, 0);

	for (j = 0; j < 16; j++) {

		if ((i << j) & 0x8000)
			gpio_direction_output(spi_sdo, 1);	
		else
			gpio_direction_output(spi_sdo, 0);	

		gpio_direction_output(spi_sclk, 0);	
		gpio_direction_output(spi_sclk, 1);	
	}
	gpio_direction_output(spi_cs, 1);

	i = para | 0x7200;
	gpio_direction_output(spi_cs, 0);
	for (j = 0; j < 16; j++) {

		if ((i << j) & 0x8000)
			gpio_direction_output(spi_sdo, 1);
		else
			gpio_direction_output(spi_sdo, 0);

		gpio_direction_output(spi_sclk, 0);	
		gpio_direction_output(spi_sclk, 1);	
	}
	gpio_direction_output(spi_cs, 1);

}
static void gpio_lcd_emuspi_read_one_para(unsigned char addr, unsigned int* para)
{
	unsigned short  i;
	int j;
	unsigned int dbit,bits1;
	i = addr | 0x7000;
	gpio_direction_output(spi_cs, 0);

	for (j = 0; j < 16; j++) {

		if ((i << j) & 0x8000)
			gpio_direction_output(spi_sdo, 1);	
		else
			gpio_direction_output(spi_sdo, 0);	

		gpio_direction_output(spi_sclk, 0);	
		gpio_direction_output(spi_sclk, 1);	
	}
	gpio_direction_output(spi_cs, 1);

	i = 0x7300;
	gpio_direction_output(spi_cs, 0);
	for (j = 0; j < 8; j++) {

		if ((i << j) & 0x8000)
			gpio_direction_output(spi_sdo, 1);
		else
			gpio_direction_output(spi_sdo, 0);

		gpio_direction_output(spi_sclk, 0);	
		gpio_direction_output(spi_sclk, 1);	
	}
	bits1=0;
	gpio_direction_input(spi_sdi);
	for (j = 0; j < 8; j++) {
		gpio_direction_output(spi_sclk, 0);	
		gpio_direction_output(spi_sclk, 1);	
		dbit=gpio_get_value(spi_sdi);
		bits1 = 2*bits1+dbit;
	}
	*para =  bits1;
	pr_info("[LHT] 0x%x is 0x%x\n",addr,bits1);
	gpio_direction_output(spi_cs, 1);

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

static int __init lcdc_panel_probe(struct platform_device *pdev)
{
	if(pdev->id == 0) {
		lcdc_himax_pdata = pdev->dev.platform_data;
		return 0;
	}
	msm_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = lcdc_panel_probe,
	.driver = {
		.name   = "lcdc_himax_wqvga",
	},
};

static struct msm_fb_panel_data lcdc_himax_panel_data = {
    
    .panel_info = {.bl_max = 20},      
    
	.on = lcdc_panel_on,
	.off = lcdc_panel_off,
       .set_backlight = lcdc_set_bl,
};

static struct platform_device this_device = {
	.name   = "lcdc_himax_wqvga",
	.id	= 1,
	.dev	= {
		.platform_data = &lcdc_himax_panel_data,
	}
};

static int __init lcdc_himax_panel_init(void)
{
	int ret;
	unsigned int panleid=0;
	struct msm_panel_info *pinfo;

	ret = platform_driver_register(&this_driver);
	if (ret) 
		return ret; 
	spi_init();
	gpio_lcd_emuspi_read_one_para(0x00,&panleid);
	if(panleid==0x52)
	{
		isnew=1;
		LcdPanleID=(u32)LCD_PANEL_P727_HX8352A;   
	}
	else if(panleid==0x65)
	{
		isnew=0;
		gpio_lcd_emuspi_write_one_para(0x22,0x00);
		LcdPanleID=21;   
	}
	else
	{
		isnew=2;
		LcdPanleID=22; 
	}
	pinfo = &lcdc_himax_panel_data.panel_info;
	pinfo->xres = 240;
	pinfo->yres = 400;
	pinfo->type = LCDC_PANEL;
	pinfo->pdest = DISPLAY_1;
	pinfo->wait_cycle = 0;
	pinfo->bpp = 18;
	pinfo->fb_num = 2;
	pinfo->clk_rate = 8192000;

	pinfo->lcdc.h_back_porch = 4;
	pinfo->lcdc.h_front_porch = 4;
	pinfo->lcdc.h_pulse_width = 4;
	pinfo->lcdc.v_back_porch = 3;
	pinfo->lcdc.v_front_porch = 3;
	pinfo->lcdc.v_pulse_width = 1;
	pinfo->lcdc.border_clr = 0;	/* blk */
	pinfo->lcdc.underflow_clr = 0xff;	/* blue */
	pinfo->lcdc.hsync_skew = 0;

	ret = platform_device_register(&this_device);
	if (ret)
		platform_driver_unregister(&this_driver);

	pr_info("himax panel loaded!\n");
	return ret;
}

module_init(lcdc_himax_panel_init);
