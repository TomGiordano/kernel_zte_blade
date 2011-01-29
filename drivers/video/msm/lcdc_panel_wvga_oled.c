
#include "msm_fb.h"
#include <asm/gpio.h>
#include <linux/module.h>
#include <linux/delay.h>

#define GPIO_LCD_BL_SC_OUT 57
#define GPIO_LCD_BL_EN

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
	LCD_PANEL_LEAD_WVGA,
	LCD_PANEL_TRULY_WVGA,
	LCD_PANEL_OLED_WVGA,
}LCD_PANEL_TYPE;

static LCD_PANEL_TYPE g_lcd_panel_type = LCD_PANEL_NONE;

static boolean is_firsttime = true;	
static boolean isOdd = true;
extern u32 LcdPanleID;
static int spi_cs;
static int spi_sclk;
static int spi_sdi;
static int spi_sdo;
static int panel_reset;
static bool onewiremode = true;
static struct msm_panel_common_pdata * lcdc_tft_pdata;
static unsigned int array[12][42] = {	{0x40,0x05,0x41,0x3f,0x42,0x39,0x43,0x23,0x44,0x22,0x45,0x22,0x46,0x20,0x50,0x05,0x51,0x00,0x52,0x00,0x53,0x2c,0x54,0x21,0x55,0x22,0x56,0x21,0x60,0x05,0x61,0x3f,0x62,0x35,0x63,0x21,0x64,0x21,0x65,0x20,0x66,0x2b},
							{0x40,0x05,0x41,0x3f,0x42,0x1f,0x43,0x22,0x44,0x24,0x45,0x1e,0x46,0x2c,0x50,0x05,0x51,0x00,0x52,0x00,0x53,0x1c,0x54,0x22,0x55,0x1d,0x56,0x2d,0x60,0x05,0x61,0x3f,0x62,0x13,0x63,0x1e,0x64,0x21,0x65,0x1b,0x66,0x3a},
							{0x40,0x05,0x41,0x3f,0x42,0x25,0x43,0x20,0x44,0x22,0x45,0x1e,0x46,0x2f,0x50,0x05,0x51,0x00,0x52,0x00,0x53,0x1e,0x54,0x21,0x55,0x1f,0x56,0x2f,0x60,0x05,0x61,0x3f,0x62,0x22,0x63,0x1f,0x64,0x1f,0x65,0x1c,0x66,0x3e},
							{0x40,0x05,0x41,0x3f,0x42,0x22,0x43,0x20,0x44,0x21,0x45,0x1e,0x46,0x32,0x50,0x05,0x51,0x00,0x52,0x00,0x53,0x1f,0x54,0x20,0x55,0x1e,0x56,0x33,0x60,0x05,0x61,0x3f,0x62,0x1f,0x63,0x1f,0x64,0x1f,0x65,0x1b,0x66,0x42},
							{0x40,0x05,0x41,0x3f,0x42,0x22,0x43,0x1f,0x44,0x21,0x45,0x1d,0x46,0x35,0x50,0x05,0x51,0x00,0x52,0x00,0x53,0x1f,0x54,0x20,0x55,0x1d,0x56,0x36,0x60,0x05,0x61,0x3f,0x62,0x1d,0x63,0x1e,0x64,0x1f,0x65,0x1a,0x66,0x46},
							{0x40,0x05,0x41,0x3f,0x42,0x1a,0x43,0x20,0x44,0x20,0x45,0x1c,0x46,0x38,0x50,0x05,0x51,0x00,0x52,0x00,0x53,0x1e,0x54,0x20,0x55,0x1c,0x56,0x39,0x60,0x05,0x61,0x3f,0x62,0x19,0x63,0x1e,0x64,0x1e,0x65,0x19,0x66,0x4a},
							{0x40,0x05,0x41,0x3f,0x42,0x1b,0x43,0x1e,0x44,0x21,0x45,0x1c,0x46,0x3a,0x50,0x05,0x51,0x00,0x52,0x00,0x53,0x1e,0x54,0x20,0x55,0x1c,0x56,0x3b,0x60,0x05,0x61,0x3f,0x62,0x17,0x63,0x1d,0x64,0x1f,0x65,0x19,0x66,0x4c},
							{0x40,0x05,0x41,0x3f,0x42,0x1d,0x43,0x1e,0x44,0x20,0x45,0x1b,0x46,0x3d,0x50,0x05,0x51,0x00,0x52,0x00,0x53,0x1f,0x54,0x1f,0x55,0x1b,0x56,0x3e,0x60,0x05,0x61,0x3f,0x62,0x18,0x63,0x1d,0x64,0x1e,0x65,0x18,0x66,0x50},
							{0x40,0x05,0x41,0x3f,0x42,0x1a,0x43,0x1e,0x44,0x1f,0x45,0x1b,0x46,0x3f,0x50,0x05,0x51,0x00,0x52,0x00,0x53,0x1e,0x54,0x1f,0x55,0x1b,0x56,0x40,0x60,0x05,0x61,0x3f,0x62,0x14,0x63,0x1d,0x64,0x1d,0x65,0x18,0x66,0x52},
							{0x40,0x05,0x41,0x3f,0x42,0x15,0x43,0x1e,0x44,0x20,0x45,0x1a,0x46,0x41,0x50,0x05,0x51,0x00,0x52,0x00,0x53,0x1e,0x54,0x20,0x55,0x1a,0x56,0x42,0x60,0x05,0x61,0x3f,0x62,0x13,0x63,0x1c,0x64,0x1f,0x65,0x17,0x66,0x54},
							{0x40,0x05,0x41,0x3f,0x42,0x15,0x43,0x1e,0x44,0x1f,0x45,0x1a,0x46,0x43,0x50,0x05,0x51,0x00,0x52,0x00,0x53,0x1e,0x54,0x1f,0x55,0x1a,0x56,0x44,0x60,0x05,0x61,0x3f,0x62,0x0f,0x63,0x1d,0x64,0x1d,0x65,0x17,0x66,0x56},
							{0x40,0x05,0x41,0x3f,0x42,0x16,0x43,0x1d,0x44,0x1f,0x45,0x19,0x46,0x46,0x50,0x05,0x51,0x00,0x52,0x00,0x53,0x1e,0x54,0x1f,0x55,0x19,0x56,0x46,0x60,0x05,0x61,0x3f,0x62,0x10,0x63,0x1c,0x64,0x1d,0x65,0x16,0x66,0x5b}};

static void gpio_lcd_truly_emuspi_write_one_index(unsigned short addr);
static void gpio_lcd_truly_emuspi_write_one_data(unsigned short data);
static void gpio_lcd_emuspi_write_one_index(unsigned short addr);
static void gpio_lcd_emuspi_write_one_data(unsigned short data);
static void gpio_lcd_emuspi_write_more(unsigned int num,unsigned int level);
static void gpio_lcd_lead_emuspi_write_one_index(unsigned int addr,unsigned short data);
static void lcdc_lead_init(void);
static void lcdc_truly_init(void);
static void lcd_panel_init(void);
static void lcdc_set_bl(struct msm_fb_data_type *mfd);
void lcdc_lead_sleep(void);
void lcdc_truly_sleep(void);
static void spi_init(void);
static int lcdc_panel_on(struct platform_device *pdev);
static int lcdc_panel_off(struct platform_device *pdev);
static void SPI_Start(void)
{
	gpio_direction_output(spi_cs, 0);
}
static void SPI_Stop(void)
{
	gpio_direction_output(spi_cs, 1);
}


static void select_1wire_mode(void)
{
	gpio_direction_output(GPIO_LCD_BL_SC_OUT, 1);
	udelay(120);
	gpio_direction_output(GPIO_LCD_BL_SC_OUT, 0);
	udelay(300);
	gpio_direction_output(GPIO_LCD_BL_SC_OUT, 1);
	udelay(600);
	
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
    	gpio_direction_output(GPIO_LCD_BL_SC_OUT, 0);
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

 
    local_irq_save(flags);
    if(current_lel==0)
    {
    	gpio_direction_output(GPIO_LCD_BL_SC_OUT, 0);
		mdelay(3);
		onewiremode = FALSE;
			
    }
    else 
	{
		if(!onewiremode)
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
static void lcdc_set_bl_oled(struct msm_fb_data_type *mfd)
{

	int current_lel = mfd->bl_level;
	unsigned int num = (sizeof(array[current_lel-1])/sizeof(array[current_lel-1][0]))/2;
	pr_info( "[oled] lcdc_set_bl level=%d, %d\n", current_lel , mfd->panel_power_on);
	
    if(!mfd->panel_power_on)
  	  return;

	if(current_lel < 1)
	{
		current_lel = 1;
	}
	if(current_lel > LCD_BL_LEVEL)
	{
		current_lel = LCD_BL_LEVEL;
	}
	if(isOdd)
	{
		gpio_lcd_emuspi_write_one_index(0x39);
		gpio_lcd_emuspi_write_one_data(0x43);


	}
	else
	{
		gpio_lcd_emuspi_write_one_index(0x39);
		gpio_lcd_emuspi_write_one_data(0x34);


	}
	gpio_lcd_emuspi_write_more(num,current_lel);
	if(isOdd)			
	{
		gpio_lcd_emuspi_write_one_index(0x39);
		gpio_lcd_emuspi_write_one_data(0x34);
		isOdd = false;
	}
	else
	{
		gpio_lcd_emuspi_write_one_index(0x39);
		gpio_lcd_emuspi_write_one_data(0x43);
		isOdd = true;
	}
}
static int lcdc_panel_on(struct platform_device *pdev)
{

	spi_init();

	if(!is_firsttime)
	{
		lcd_panel_init();
		
	}
	else
	{
		is_firsttime = false;
	}
	
	return 0;
}

static void gpio_lcd_lead_emuspi_write_one_index(unsigned int addr,unsigned short data)
{
	unsigned int i;
	int j;

	i=0x20;
	gpio_direction_output(GPIO_SPI_SCLK, 0);	
	gpio_direction_output(GPIO_SPI_CS, 0);
	for (j = 0; j < 8; j++) 
	{
		if (i & 0x80)
			gpio_direction_output(GPIO_SPI_SDO, 1);	
		else
			gpio_direction_output(GPIO_SPI_SDO, 0);	
		gpio_direction_output(GPIO_SPI_SCLK, 0);	
		gpio_direction_output(GPIO_SPI_SCLK, 1);	
		i <<= 1;
	}
	i = addr ;
	for (j = 0; j < 8; j++) {

		if (i & 0x8000)
			gpio_direction_output(GPIO_SPI_SDO, 1);	
		else
			gpio_direction_output(GPIO_SPI_SDO, 0);	
		gpio_direction_output(GPIO_SPI_SCLK, 0);	
		gpio_direction_output(GPIO_SPI_SCLK, 1);	
		i <<= 1;
	}
	gpio_direction_output(GPIO_SPI_CS, 1);
	gpio_direction_output(GPIO_SPI_CS, 0);

	i=0x00;
	for (j = 0; j < 8; j++) 
	{

		if (i & 0x80)
			gpio_direction_output(GPIO_SPI_SDO, 1);	
		else
			gpio_direction_output(GPIO_SPI_SDO, 0);	

		gpio_direction_output(GPIO_SPI_SCLK, 0);	
		gpio_direction_output(GPIO_SPI_SCLK, 1);	
		//gpio_direction_output(GPIO_SPI_SCLK, 0);	
		/*udelay(4);*/
		i <<= 1;
	}
	i = addr ;
	for (j = 0; j < 8; j++) {

		if (i & 0x80)
			gpio_direction_output(GPIO_SPI_SDO, 1);	
		else
			gpio_direction_output(GPIO_SPI_SDO, 0);	
		gpio_direction_output(GPIO_SPI_SCLK, 0);	
		gpio_direction_output(GPIO_SPI_SCLK, 1);	
		i <<= 1;
	}

	gpio_direction_output(GPIO_SPI_CS, 1);
	gpio_direction_output(GPIO_SPI_CS, 0);
	i=0x40;
	for (j = 0; j < 8; j++) 
	{

		if (i & 0x80)
			gpio_direction_output(GPIO_SPI_SDO, 1);	
		else
			gpio_direction_output(GPIO_SPI_SDO, 0);	

		gpio_direction_output(GPIO_SPI_SCLK, 0);	
		gpio_direction_output(GPIO_SPI_SCLK, 1);
		i <<= 1;
	}
	i = data ;
	for (j = 0; j < 8; j++) {

		if (i & 0x80)
			gpio_direction_output(GPIO_SPI_SDO, 1);	
		else
			gpio_direction_output(GPIO_SPI_SDO, 0);	

		gpio_direction_output(GPIO_SPI_SCLK, 0);	
		gpio_direction_output(GPIO_SPI_SCLK, 1);
		i <<= 1;
	}
	gpio_direction_output(GPIO_SPI_CS, 1);
}
static void gpio_lcd_lead_emuspi_write_cmd(unsigned int addr,unsigned short data)
{
	unsigned int i;
	int j;

	i=0x20;
	gpio_direction_output(GPIO_SPI_SCLK, 0);	
	gpio_direction_output(GPIO_SPI_CS, 0);
	for (j = 0; j < 8; j++) 
	{
		if (i & 0x80)
			gpio_direction_output(GPIO_SPI_SDO, 1);	
		else
			gpio_direction_output(GPIO_SPI_SDO, 0);	
		gpio_direction_output(GPIO_SPI_SCLK, 0);	
		gpio_direction_output(GPIO_SPI_SCLK, 1);
		i <<= 1;
	}
	i = addr ;
	for (j = 0; j < 8; j++) {

		if (i & 0x8000)
			gpio_direction_output(GPIO_SPI_SDO, 1);	
		else
			gpio_direction_output(GPIO_SPI_SDO, 0);	
		gpio_direction_output(GPIO_SPI_SCLK, 0);	
		gpio_direction_output(GPIO_SPI_SCLK, 1);	
		i <<= 1;
	}
	gpio_direction_output(GPIO_SPI_CS, 1);
	gpio_direction_output(GPIO_SPI_CS, 0);

	i=0x00;
	for (j = 0; j < 8; j++) 
	{

		if (i & 0x80)
			gpio_direction_output(GPIO_SPI_SDO, 1);	
		else
			gpio_direction_output(GPIO_SPI_SDO, 0);	
		gpio_direction_output(GPIO_SPI_SCLK, 0);	
		gpio_direction_output(GPIO_SPI_SCLK, 1);	
		i <<= 1;
	}
	i = addr ;
	for (j = 0; j < 8; j++) {

		if (i & 0x80)
			gpio_direction_output(GPIO_SPI_SDO, 1);	
		else
			gpio_direction_output(GPIO_SPI_SDO, 0);	
		gpio_direction_output(GPIO_SPI_SCLK, 0);	
		gpio_direction_output(GPIO_SPI_SCLK, 1);	
		i <<= 1;
	}
	gpio_direction_output(GPIO_SPI_CS, 1);
	
}
static void gpio_lcd_lead_emuspi_read_one_index(unsigned int addr,unsigned int *data)
{
	unsigned int i;
	int j;
       unsigned int dbit,bits1;
	i=0x20;
	gpio_direction_output(GPIO_SPI_SCLK, 0);	
	gpio_direction_output(GPIO_SPI_CS, 0);
	for (j = 0; j < 8; j++) 
	{

		if (i & 0x80)
			gpio_direction_output(GPIO_SPI_SDO, 1);	
		else
			gpio_direction_output(GPIO_SPI_SDO, 0);	
		gpio_direction_output(GPIO_SPI_SCLK, 0);	
		gpio_direction_output(GPIO_SPI_SCLK, 1);
		i <<= 1;
	}
	i = addr ;
	for (j = 0; j < 8; j++) {

		if (i & 0x8000)
			gpio_direction_output(GPIO_SPI_SDO, 1);	
		else
			gpio_direction_output(GPIO_SPI_SDO, 0);	
		gpio_direction_output(GPIO_SPI_SCLK, 0);	
		gpio_direction_output(GPIO_SPI_SCLK, 1);	
		i <<= 1;
	}
	gpio_direction_output(GPIO_SPI_CS, 1);
	mdelay(1);
	gpio_direction_output(GPIO_SPI_CS, 0);
	i=0x00;
	for (j = 0; j < 8; j++) 
	{

		if (i & 0x80)
			gpio_direction_output(GPIO_SPI_SDO, 1);	
		else
			gpio_direction_output(GPIO_SPI_SDO, 0);	

		gpio_direction_output(GPIO_SPI_SCLK, 0);	
		gpio_direction_output(GPIO_SPI_SCLK, 1);
		i <<= 1;
	}
	i = addr ;
	for (j = 0; j < 8; j++) {

		if (i & 0x80)
			gpio_direction_output(GPIO_SPI_SDO, 1);	
		else
			gpio_direction_output(GPIO_SPI_SDO, 0);	

		gpio_direction_output(GPIO_SPI_SCLK, 0);	
		gpio_direction_output(GPIO_SPI_SCLK, 1);	
		i <<= 1;
	}

	gpio_direction_output(GPIO_SPI_CS, 1);
	mdelay(1);
	gpio_direction_output(GPIO_SPI_CS, 0);

	i=0xc0;
	for (j = 0; j < 8; j++) 
	{

		if (i & 0x80)
			gpio_direction_output(GPIO_SPI_SDO, 1);	
		else
			gpio_direction_output(GPIO_SPI_SDO, 0);	
		gpio_direction_output(GPIO_SPI_SCLK, 0);	
		gpio_direction_output(GPIO_SPI_SCLK, 1);	
		i <<= 1;
	}
	bits1=0;
	for (j = 0; j < 8; j++) {
		gpio_direction_output(GPIO_SPI_SCLK, 0);	
		gpio_direction_output(GPIO_SPI_SCLK, 1);	
		dbit=gpio_get_value(GPIO_SPI_SDI);
		bits1 = 2*bits1+dbit;
	}
	*data =  bits1;
	gpio_direction_output(GPIO_SPI_CS, 1);
}
static void gpio_lcd_truly_emuspi_write_one_index(unsigned short addr)
{
	unsigned int i;
	int j;
	i = addr | 0x7000;
	for (j = 0; j < 9; j++) {

		if (i & 0x0100)
			gpio_direction_output(spi_sdo, 1);	
		else
			gpio_direction_output(spi_sdo, 0);	

		gpio_direction_output(spi_sclk, 0);	
		/*udelay(4);*/
		gpio_direction_output(spi_sclk, 1);	
		/*udelay(4);*/
		i <<= 1;
	}
	
}

static void gpio_lcd_truly_emuspi_write_one_data(unsigned short data)
{
	unsigned int i;
	int j;

	i = data | 0x7100;

	for (j = 0; j < 9; j++) {

		if (i & 0x0100)
			gpio_direction_output(spi_sdo, 1);	
		else
			gpio_direction_output(spi_sdo, 0);	

		gpio_direction_output(spi_sclk, 0);	
		/*udelay(4);*/
		gpio_direction_output(spi_sclk, 1);	
		/*udelay(4);*/
		i <<= 1;
	}

}
static void gpio_lcd_truly_emuspi_read_one_para(unsigned short addr, unsigned int *data1)
{
	
	int j,i;
	unsigned int dbit,bits1;		
	i = addr | 0x7000;

	for (j = 0; j < 9; j++) {

		if (i & 0x0100)
			gpio_direction_output(GPIO_SPI_SDO, 1);	
		else
			gpio_direction_output(GPIO_SPI_SDO, 0);	

		gpio_direction_output(GPIO_SPI_SCLK, 0);	
		gpio_direction_output(GPIO_SPI_SCLK, 1);
		i <<= 1;
	}

	//ret = gpio_config(GPIO_SPI_SDI,0);	

	bits1=0;
	for (j = 0; j < 16; j++) {
 
		gpio_direction_output(GPIO_SPI_SCLK, 0);
        dbit=gpio_get_value(GPIO_SPI_SDI);
        gpio_direction_output(GPIO_SPI_SCLK, 1);
		bits1 = 2*bits1+dbit;
		
	}
	*data1 =  bits1;	

}
static void gpio_lcd_emuspi_write_one_index(unsigned short addr)
{
	unsigned int i;
	int j;

	i = addr | 0x7000;
	gpio_direction_output(spi_cs, 0);
	/*udelay(4);*/

	for (j = 0; j < 16; j++) {

		if (i & 0x8000)
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
static void gpio_lcd_emuspi_write_one_data(unsigned short data)
{
	unsigned int i;
	int j;

	i = data | 0x7200;
	gpio_direction_output(spi_cs, 0);
	/*udelay(4);*/

	for (j = 0; j < 16; j++) {

		if (i & 0x8000)
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
static void gpio_lcd_emuspi_read_one_data(unsigned int *data)
{
	unsigned int i;
	int j;
	unsigned int dbit,bits1;
	i = 0x7100;
	gpio_direction_output(spi_cs, 0);
	/*udelay(4);*/

	for (j = 0; j < 8; j++) {

		if (i & 0x8000)
			gpio_direction_output(spi_sdo, 1);	
		else
			gpio_direction_output(spi_sdo, 0);	

		gpio_direction_output(spi_sclk, 0);	
		/*udelay(4);*/
		gpio_direction_output(spi_sclk, 1);	
		/*udelay(4);*/
		i <<= 1;
	}
	bits1=0;
	for (j = 0; j < 8; j++) {
		gpio_direction_output(GPIO_SPI_SCLK, 0);	
		gpio_direction_output(GPIO_SPI_SCLK, 1);	
		dbit=gpio_get_value(GPIO_SPI_SDI);
		bits1 = 2*bits1+dbit;
	}
	*data =  bits1;
	gpio_direction_output(spi_cs, 1);
}
static void gpio_lcd_emuspi_write_more(unsigned int num,unsigned int level)
{
	unsigned int i;
	for(i = 0; i < num;i++)
	{
		gpio_lcd_emuspi_write_one_index(array[level-1][2*i]);
		gpio_lcd_emuspi_write_one_data(array[level-1][2*i+1]);
	}
}
static void lcdc_oled_init(void)
{	
	gpio_lcd_emuspi_write_one_index(0x31);
	gpio_lcd_emuspi_write_one_data(0x08);
	
	gpio_lcd_emuspi_write_one_index(0x32);
	gpio_lcd_emuspi_write_one_data(0x14);

	gpio_lcd_emuspi_write_one_index(0x30);
	gpio_lcd_emuspi_write_one_data(0x02);

	gpio_lcd_emuspi_write_one_index(0x27);
	gpio_lcd_emuspi_write_one_data(0x01);

	gpio_lcd_emuspi_write_one_index(0x12);
	gpio_lcd_emuspi_write_one_data(0x08);

	gpio_lcd_emuspi_write_one_index(0x13);
	gpio_lcd_emuspi_write_one_data(0x08);

	gpio_lcd_emuspi_write_one_index(0x15);
	gpio_lcd_emuspi_write_one_data(0x0f);

	gpio_lcd_emuspi_write_one_index(0x16);
	gpio_lcd_emuspi_write_one_data(0x00);

	gpio_lcd_emuspi_write_one_index(0xef);
	gpio_lcd_emuspi_write_one_data(0xd0);
	gpio_lcd_emuspi_write_one_data(0xe8);

	gpio_lcd_emuspi_write_one_index(0x39);
	gpio_lcd_emuspi_write_one_data(0x44);

	gpio_lcd_emuspi_write_one_index(0x40);
	gpio_lcd_emuspi_write_one_data(0x05);

	gpio_lcd_emuspi_write_one_index(0x41);
	gpio_lcd_emuspi_write_one_data(0x3F);

	gpio_lcd_emuspi_write_one_index(0x42);
	gpio_lcd_emuspi_write_one_data(0x16);

	gpio_lcd_emuspi_write_one_index(0x43);
	gpio_lcd_emuspi_write_one_data(0x1D);

	gpio_lcd_emuspi_write_one_index(0x44);
	gpio_lcd_emuspi_write_one_data(0x1F);

	gpio_lcd_emuspi_write_one_index(0x45);
	gpio_lcd_emuspi_write_one_data(0x19);

	gpio_lcd_emuspi_write_one_index(0x46);
	gpio_lcd_emuspi_write_one_data(0x46);

	gpio_lcd_emuspi_write_one_index(0x50);
	gpio_lcd_emuspi_write_one_data(0x05);

	gpio_lcd_emuspi_write_one_index(0x51);
	gpio_lcd_emuspi_write_one_data(0x00);

	gpio_lcd_emuspi_write_one_index(0x52);
	gpio_lcd_emuspi_write_one_data(0x00);

	gpio_lcd_emuspi_write_one_index(0x53);
	gpio_lcd_emuspi_write_one_data(0x1E);

	gpio_lcd_emuspi_write_one_index(0x54);
	gpio_lcd_emuspi_write_one_data(0x1F);

	gpio_lcd_emuspi_write_one_index(0x55);
	gpio_lcd_emuspi_write_one_data(0x19);

	gpio_lcd_emuspi_write_one_index(0x56);
	gpio_lcd_emuspi_write_one_data(0x46);

	gpio_lcd_emuspi_write_one_index(0x60);
	gpio_lcd_emuspi_write_one_data(0x05);

	gpio_lcd_emuspi_write_one_index(0x61);
	gpio_lcd_emuspi_write_one_data(0x3F);

	gpio_lcd_emuspi_write_one_index(0x62);
	gpio_lcd_emuspi_write_one_data(0x10);

	gpio_lcd_emuspi_write_one_index(0x63);
	gpio_lcd_emuspi_write_one_data(0x1C);

	gpio_lcd_emuspi_write_one_index(0x64);
	gpio_lcd_emuspi_write_one_data(0x1D);

	gpio_lcd_emuspi_write_one_index(0x65);
	gpio_lcd_emuspi_write_one_data(0x16);

	gpio_lcd_emuspi_write_one_index(0x66);
	gpio_lcd_emuspi_write_one_data(0x5B);

	gpio_lcd_emuspi_write_one_index(0x17);
	gpio_lcd_emuspi_write_one_data(0x22);

	gpio_lcd_emuspi_write_one_index(0x18);
	gpio_lcd_emuspi_write_one_data(0x33);

	gpio_lcd_emuspi_write_one_index(0x19);
	gpio_lcd_emuspi_write_one_data(0x03);

	gpio_lcd_emuspi_write_one_index(0x1a);
	gpio_lcd_emuspi_write_one_data(0x01);

	gpio_lcd_emuspi_write_one_index(0x22);
	gpio_lcd_emuspi_write_one_data(0xa4);

	gpio_lcd_emuspi_write_one_index(0x23);
	gpio_lcd_emuspi_write_one_data(0x00);
//	mdelay(250);

	gpio_lcd_emuspi_write_one_index(0x26);
	gpio_lcd_emuspi_write_one_data(0xa0);

	gpio_lcd_emuspi_write_one_index(0x1d);
	gpio_lcd_emuspi_write_one_data(0xa0);
	msleep(200);		
	
	gpio_lcd_emuspi_write_one_index(0x14);
	gpio_lcd_emuspi_write_one_data(0x03);

	printk("lcd module oled init finish phase1\n!");
	msleep(10);
	printk("lcd module oled init finish phase2\n!");

}
static void lcdc_lead_init(void)
{
     
	gpio_lcd_lead_emuspi_write_one_index(0x2e80,0x01);
	msleep(20);
	gpio_lcd_lead_emuspi_write_one_index(0x0680,0x21);
	gpio_lcd_lead_emuspi_write_one_index(0xD380,0x04);
	gpio_lcd_lead_emuspi_write_one_index(0xD480,0x5B);
	gpio_lcd_lead_emuspi_write_one_index(0xD580,0x07);
	gpio_lcd_lead_emuspi_write_one_index(0xD680,0x52);
	gpio_lcd_lead_emuspi_write_one_index(0xD080,0x17);
	gpio_lcd_lead_emuspi_write_one_index(0xD180,0x0D);
	gpio_lcd_lead_emuspi_write_one_index(0xD280,0x04);
	gpio_lcd_lead_emuspi_write_one_index(0xDC80,0x04);
	gpio_lcd_lead_emuspi_write_one_index(0xD780,0x01);
	gpio_lcd_lead_emuspi_write_one_index(0x2280,0x0a);
	gpio_lcd_lead_emuspi_write_one_index(0x2480,0x52);
	gpio_lcd_lead_emuspi_write_one_index(0x2580,0x25);
	gpio_lcd_lead_emuspi_write_one_index(0x2780,0x75);
	gpio_lcd_lead_emuspi_write_one_index(0x3A00,0x66);
	gpio_lcd_lead_emuspi_write_one_index(0x3b00,0x28);
	gpio_lcd_lead_emuspi_write_one_index(0x0180,0x02);
	gpio_lcd_lead_emuspi_write_one_index(0x4080,0x00);
	gpio_lcd_lead_emuspi_write_one_index(0x4180,0x05);
	gpio_lcd_lead_emuspi_write_one_index(0x4280,0x15);
	gpio_lcd_lead_emuspi_write_one_index(0x4380,0x27);
	gpio_lcd_lead_emuspi_write_one_index(0x4480,0x19);
	gpio_lcd_lead_emuspi_write_one_index(0x4580,0x2D);
	gpio_lcd_lead_emuspi_write_one_index(0x4680,0x5E);
	gpio_lcd_lead_emuspi_write_one_index(0x4780,0x43);
	gpio_lcd_lead_emuspi_write_one_index(0x4880,0x1F);
	gpio_lcd_lead_emuspi_write_one_index(0x4980,0x26);
	gpio_lcd_lead_emuspi_write_one_index(0x4A80,0x9C);
	gpio_lcd_lead_emuspi_write_one_index(0x4B80,0x1D);
	gpio_lcd_lead_emuspi_write_one_index(0x4C80,0x41);
	gpio_lcd_lead_emuspi_write_one_index(0x4D80,0x5A);
	gpio_lcd_lead_emuspi_write_one_index(0x4E80,0x8B);
	gpio_lcd_lead_emuspi_write_one_index(0x4F80,0x95);
	gpio_lcd_lead_emuspi_write_one_index(0x5080,0x64);
	gpio_lcd_lead_emuspi_write_one_index(0x5180,0x7F);
	gpio_lcd_lead_emuspi_write_one_index(0x5880,0x00);
	gpio_lcd_lead_emuspi_write_one_index(0x5980,0x1A);
	gpio_lcd_lead_emuspi_write_one_index(0x5A80,0x68);
	gpio_lcd_lead_emuspi_write_one_index(0x5B80,0x79);
	gpio_lcd_lead_emuspi_write_one_index(0x5C80,0x2A);
	gpio_lcd_lead_emuspi_write_one_index(0x5D80,0x43);
	gpio_lcd_lead_emuspi_write_one_index(0x5E80,0x67);
	gpio_lcd_lead_emuspi_write_one_index(0x5F80,0x65);
	gpio_lcd_lead_emuspi_write_one_index(0x6080,0x17);
	gpio_lcd_lead_emuspi_write_one_index(0x6180,0x1E);
	gpio_lcd_lead_emuspi_write_one_index(0x6280,0xBF);
	gpio_lcd_lead_emuspi_write_one_index(0x6380,0x20);
	gpio_lcd_lead_emuspi_write_one_index(0x6480,0x51);
	gpio_lcd_lead_emuspi_write_one_index(0x6580,0x66);
	gpio_lcd_lead_emuspi_write_one_index(0x6680,0xD7);
	gpio_lcd_lead_emuspi_write_one_index(0x6780,0xE9);
	gpio_lcd_lead_emuspi_write_one_index(0x6880,0x79);
	gpio_lcd_lead_emuspi_write_one_index(0x6980,0x7F);
	gpio_lcd_lead_emuspi_write_one_index(0x7080,0x00);
	gpio_lcd_lead_emuspi_write_one_index(0x7180,0x07);
	gpio_lcd_lead_emuspi_write_one_index(0x7280,0x1A);
	gpio_lcd_lead_emuspi_write_one_index(0x7380,0x30);
	gpio_lcd_lead_emuspi_write_one_index(0x7480,0x1C);
	gpio_lcd_lead_emuspi_write_one_index(0x7580,0x30);
	gpio_lcd_lead_emuspi_write_one_index(0x7680,0x61);
	gpio_lcd_lead_emuspi_write_one_index(0x7780,0x4F);
	gpio_lcd_lead_emuspi_write_one_index(0x7880,0x20);
	gpio_lcd_lead_emuspi_write_one_index(0x7980,0x27);
	gpio_lcd_lead_emuspi_write_one_index(0x7A80,0xA5);
	gpio_lcd_lead_emuspi_write_one_index(0x7B80,0x1E);
	gpio_lcd_lead_emuspi_write_one_index(0x7C80,0x48);
	gpio_lcd_lead_emuspi_write_one_index(0x7D80,0x5E);
	gpio_lcd_lead_emuspi_write_one_index(0x7E80,0x87);
	gpio_lcd_lead_emuspi_write_one_index(0x7F80,0xAD);
	gpio_lcd_lead_emuspi_write_one_index(0x8080,0x6A);
	gpio_lcd_lead_emuspi_write_one_index(0x8180,0x7F);
	gpio_lcd_lead_emuspi_write_one_index(0x8880,0x00);
	gpio_lcd_lead_emuspi_write_one_index(0x8980,0x15);
	gpio_lcd_lead_emuspi_write_one_index(0x8A80,0x53);
	gpio_lcd_lead_emuspi_write_one_index(0x8B80,0x76);
	gpio_lcd_lead_emuspi_write_one_index(0x8C80,0x2D);
	gpio_lcd_lead_emuspi_write_one_index(0x8D80,0x43);
	gpio_lcd_lead_emuspi_write_one_index(0x8E80,0x66);
	gpio_lcd_lead_emuspi_write_one_index(0x8F80,0x5C);
	gpio_lcd_lead_emuspi_write_one_index(0x9080,0x18);
	gpio_lcd_lead_emuspi_write_one_index(0x9180,0x1F);
	gpio_lcd_lead_emuspi_write_one_index(0x9280,0xB2);
	gpio_lcd_lead_emuspi_write_one_index(0x9380,0x1E);
	gpio_lcd_lead_emuspi_write_one_index(0x9480,0x4E);
	gpio_lcd_lead_emuspi_write_one_index(0x9580,0x64);
	gpio_lcd_lead_emuspi_write_one_index(0x9680,0xCE);
	gpio_lcd_lead_emuspi_write_one_index(0x9780,0xE3);
	gpio_lcd_lead_emuspi_write_one_index(0x9880,0x78);
	gpio_lcd_lead_emuspi_write_one_index(0x9980,0x7F);
	gpio_lcd_lead_emuspi_write_one_index(0xA080,0x00);
	gpio_lcd_lead_emuspi_write_one_index(0xA180,0x09);
	gpio_lcd_lead_emuspi_write_one_index(0xA280,0x25);
	gpio_lcd_lead_emuspi_write_one_index(0xA380,0x41);
	gpio_lcd_lead_emuspi_write_one_index(0xA480,0x20);
	gpio_lcd_lead_emuspi_write_one_index(0xA580,0x35);
	gpio_lcd_lead_emuspi_write_one_index(0xA680,0x63);
	gpio_lcd_lead_emuspi_write_one_index(0xA780,0x5D);
	gpio_lcd_lead_emuspi_write_one_index(0xA880,0x1F);
	gpio_lcd_lead_emuspi_write_one_index(0xA980,0x26);
	gpio_lcd_lead_emuspi_write_one_index(0xAA80,0xAE);
	gpio_lcd_lead_emuspi_write_one_index(0xAB80,0x19);
	gpio_lcd_lead_emuspi_write_one_index(0xAC80,0x3D);
	gpio_lcd_lead_emuspi_write_one_index(0xAD80,0x4B);
	gpio_lcd_lead_emuspi_write_one_index(0xAE80,0xA7);
	gpio_lcd_lead_emuspi_write_one_index(0xAF80,0xF8);
	gpio_lcd_lead_emuspi_write_one_index(0xB080,0x7D);
	gpio_lcd_lead_emuspi_write_one_index(0xB180,0x7F);
	gpio_lcd_lead_emuspi_write_one_index(0xB880,0x00);
	gpio_lcd_lead_emuspi_write_one_index(0xB980,0x02);
	gpio_lcd_lead_emuspi_write_one_index(0xBA80,0x08);
	gpio_lcd_lead_emuspi_write_one_index(0xBB80,0x59);
	gpio_lcd_lead_emuspi_write_one_index(0xBC80,0x34);
	gpio_lcd_lead_emuspi_write_one_index(0xBD80,0x4A);
	gpio_lcd_lead_emuspi_write_one_index(0xBE80,0x6B);
	gpio_lcd_lead_emuspi_write_one_index(0xBF80,0x53);
	gpio_lcd_lead_emuspi_write_one_index(0xC080,0x16);
	gpio_lcd_lead_emuspi_write_one_index(0xC180,0x1E);
	gpio_lcd_lead_emuspi_write_one_index(0xC280,0xA4);
	gpio_lcd_lead_emuspi_write_one_index(0xC380,0x1B);
	gpio_lcd_lead_emuspi_write_one_index(0xC480,0x4B);
	gpio_lcd_lead_emuspi_write_one_index(0xC580,0x62);
	gpio_lcd_lead_emuspi_write_one_index(0xC680,0xBD);
	gpio_lcd_lead_emuspi_write_one_index(0xC780,0xD7);
	gpio_lcd_lead_emuspi_write_one_index(0xC880,0x75);
	gpio_lcd_lead_emuspi_write_one_index(0xC980,0x7F);
	gpio_lcd_lead_emuspi_write_one_index(0x3500,0x00);
	//gpio_lcd_lead_emuspi_write_one_index(0x5300,0x2C);
	gpio_lcd_lead_emuspi_write_one_index(0x3600,0x00);
	gpio_lcd_lead_emuspi_write_cmd(0x1100,0x00);
	msleep(120);
	gpio_lcd_lead_emuspi_write_cmd(0x2900,0x00);

	
	msleep(10);
	printk("lcd module TFT LEAD init finish\n!");
	

}
void lcdc_truly_init(void)
{

#if 1 
  	SPI_Start();
	gpio_lcd_truly_emuspi_write_one_index(0xB9); // SET password
	gpio_lcd_truly_emuspi_write_one_data(0xFF);
	gpio_lcd_truly_emuspi_write_one_data(0x83);
	gpio_lcd_truly_emuspi_write_one_data(0x69);
	SPI_Stop(); 
	SPI_Start(); 
	gpio_lcd_truly_emuspi_write_one_index(0x36);   
	gpio_lcd_truly_emuspi_write_one_data(0x00);   //  GBR MODE
	SPI_Stop(); 
  	SPI_Start();
	gpio_lcd_truly_emuspi_write_one_index(0xB2); // SET Display 480x800
	gpio_lcd_truly_emuspi_write_one_data(0x00);
	gpio_lcd_truly_emuspi_write_one_data(0x23);
	gpio_lcd_truly_emuspi_write_one_data(0x03);
	gpio_lcd_truly_emuspi_write_one_data(0x03);
	gpio_lcd_truly_emuspi_write_one_data(0x70);
	gpio_lcd_truly_emuspi_write_one_data(0x00);
	gpio_lcd_truly_emuspi_write_one_data(0xFF);
	gpio_lcd_truly_emuspi_write_one_data(0x00);
	gpio_lcd_truly_emuspi_write_one_data(0x00);
	gpio_lcd_truly_emuspi_write_one_data(0x00);
	gpio_lcd_truly_emuspi_write_one_data(0x00);
	gpio_lcd_truly_emuspi_write_one_data(0x03);
	gpio_lcd_truly_emuspi_write_one_data(0x03);
	gpio_lcd_truly_emuspi_write_one_data(0x00);
	gpio_lcd_truly_emuspi_write_one_data(0x01);
	SPI_Stop(); 
  	SPI_Start();
	gpio_lcd_truly_emuspi_write_one_index(0xB4); // SET Display waveform cycles
	gpio_lcd_truly_emuspi_write_one_data(0x00);
	gpio_lcd_truly_emuspi_write_one_data(0x18);
	gpio_lcd_truly_emuspi_write_one_data(0x80);
	gpio_lcd_truly_emuspi_write_one_data(0x06);
	gpio_lcd_truly_emuspi_write_one_data(0x02);
	SPI_Stop(); 
 	 SPI_Start();
	gpio_lcd_truly_emuspi_write_one_index(0xB6); // SET VCOM
	gpio_lcd_truly_emuspi_write_one_data(0x42);
	gpio_lcd_truly_emuspi_write_one_data(0x42);
	SPI_Stop(); 
  	SPI_Start();
	gpio_lcd_truly_emuspi_write_one_index(0xD5); // SET GIP
	gpio_lcd_truly_emuspi_write_one_data(0x00);
	gpio_lcd_truly_emuspi_write_one_data(0x04);
	gpio_lcd_truly_emuspi_write_one_data(0x03);
	gpio_lcd_truly_emuspi_write_one_data(0x00);
	gpio_lcd_truly_emuspi_write_one_data(0x01);
	gpio_lcd_truly_emuspi_write_one_data(0x05);
	gpio_lcd_truly_emuspi_write_one_data(0x28);
	gpio_lcd_truly_emuspi_write_one_data(0x70);
	gpio_lcd_truly_emuspi_write_one_data(0x01);
	gpio_lcd_truly_emuspi_write_one_data(0x03);
	gpio_lcd_truly_emuspi_write_one_data(0x00);
	gpio_lcd_truly_emuspi_write_one_data(0x00);
	gpio_lcd_truly_emuspi_write_one_data(0x40);
	gpio_lcd_truly_emuspi_write_one_data(0x06);
	gpio_lcd_truly_emuspi_write_one_data(0x51);
	gpio_lcd_truly_emuspi_write_one_data(0x07);
	gpio_lcd_truly_emuspi_write_one_data(0x00);
	gpio_lcd_truly_emuspi_write_one_data(0x00);
	gpio_lcd_truly_emuspi_write_one_data(0x41);
	gpio_lcd_truly_emuspi_write_one_data(0x06);
	gpio_lcd_truly_emuspi_write_one_data(0x50);
	gpio_lcd_truly_emuspi_write_one_data(0x07);
	gpio_lcd_truly_emuspi_write_one_data(0x07);
	gpio_lcd_truly_emuspi_write_one_data(0x0F);
	gpio_lcd_truly_emuspi_write_one_data(0x04);
	gpio_lcd_truly_emuspi_write_one_data(0x00);
	SPI_Stop(); 
  	SPI_Start();
	gpio_lcd_truly_emuspi_write_one_index(0xB1); //Set Power
	gpio_lcd_truly_emuspi_write_one_data(0x85);
	gpio_lcd_truly_emuspi_write_one_data(0x00);
	gpio_lcd_truly_emuspi_write_one_data(0x34);
	gpio_lcd_truly_emuspi_write_one_data(0x07);
	gpio_lcd_truly_emuspi_write_one_data(0x00);
	gpio_lcd_truly_emuspi_write_one_data(0x0F);
	gpio_lcd_truly_emuspi_write_one_data(0x0F);
	gpio_lcd_truly_emuspi_write_one_data(0x2A);
	gpio_lcd_truly_emuspi_write_one_data(0x32);
	gpio_lcd_truly_emuspi_write_one_data(0x3F);
	gpio_lcd_truly_emuspi_write_one_data(0x3F);
	gpio_lcd_truly_emuspi_write_one_data(0x01);
	gpio_lcd_truly_emuspi_write_one_data(0x3A);
	gpio_lcd_truly_emuspi_write_one_data(0x01);
	gpio_lcd_truly_emuspi_write_one_data(0xE6);
	gpio_lcd_truly_emuspi_write_one_data(0xE6);
	gpio_lcd_truly_emuspi_write_one_data(0xE6);
	gpio_lcd_truly_emuspi_write_one_data(0xE6);
	gpio_lcd_truly_emuspi_write_one_data(0xE6);
	SPI_Stop(); 
 	SPI_Start();
	gpio_lcd_truly_emuspi_write_one_index(0xE0); //SET GAMMA 2.2
	gpio_lcd_truly_emuspi_write_one_data(0x00);
	gpio_lcd_truly_emuspi_write_one_data(0x13);
	gpio_lcd_truly_emuspi_write_one_data(0x19);
	gpio_lcd_truly_emuspi_write_one_data(0x38);
	gpio_lcd_truly_emuspi_write_one_data(0x3D);
	gpio_lcd_truly_emuspi_write_one_data(0x3F);
	gpio_lcd_truly_emuspi_write_one_data(0x28);
	gpio_lcd_truly_emuspi_write_one_data(0x46);
	gpio_lcd_truly_emuspi_write_one_data(0x07);
	gpio_lcd_truly_emuspi_write_one_data(0x0D);
	gpio_lcd_truly_emuspi_write_one_data(0x0E);
	gpio_lcd_truly_emuspi_write_one_data(0x12);
	gpio_lcd_truly_emuspi_write_one_data(0x15);
	gpio_lcd_truly_emuspi_write_one_data(0x12);
	gpio_lcd_truly_emuspi_write_one_data(0x14);
	gpio_lcd_truly_emuspi_write_one_data(0x0F);
	gpio_lcd_truly_emuspi_write_one_data(0x17);
	gpio_lcd_truly_emuspi_write_one_data(0x00);
	gpio_lcd_truly_emuspi_write_one_data(0x13);
	gpio_lcd_truly_emuspi_write_one_data(0x19);
	gpio_lcd_truly_emuspi_write_one_data(0x38);
	gpio_lcd_truly_emuspi_write_one_data(0x3D);
	gpio_lcd_truly_emuspi_write_one_data(0x3F);
	gpio_lcd_truly_emuspi_write_one_data(0x28);
	gpio_lcd_truly_emuspi_write_one_data(0x46);
	gpio_lcd_truly_emuspi_write_one_data(0x07);
	gpio_lcd_truly_emuspi_write_one_data(0x0D);
	gpio_lcd_truly_emuspi_write_one_data(0x0E);
	gpio_lcd_truly_emuspi_write_one_data(0x12);
	gpio_lcd_truly_emuspi_write_one_data(0x15);
	gpio_lcd_truly_emuspi_write_one_data(0x12);
	gpio_lcd_truly_emuspi_write_one_data(0x14);
	gpio_lcd_truly_emuspi_write_one_data(0x0F);
	gpio_lcd_truly_emuspi_write_one_data(0x17);
	SPI_Stop(); 
  	SPI_Start();
	gpio_lcd_truly_emuspi_write_one_index(0xC1); //SET DGC
	gpio_lcd_truly_emuspi_write_one_data(0x01);
	gpio_lcd_truly_emuspi_write_one_data(0x04);
	gpio_lcd_truly_emuspi_write_one_data(0x0A);
	gpio_lcd_truly_emuspi_write_one_data(0x13);
	gpio_lcd_truly_emuspi_write_one_data(0x1A);
	gpio_lcd_truly_emuspi_write_one_data(0x21);
	gpio_lcd_truly_emuspi_write_one_data(0x29);
	gpio_lcd_truly_emuspi_write_one_data(0x31);
	gpio_lcd_truly_emuspi_write_one_data(0x37);
	gpio_lcd_truly_emuspi_write_one_data(0x3E);
	gpio_lcd_truly_emuspi_write_one_data(0x47);
	gpio_lcd_truly_emuspi_write_one_data(0x4F);
	gpio_lcd_truly_emuspi_write_one_data(0x56);
	gpio_lcd_truly_emuspi_write_one_data(0x5E);
	gpio_lcd_truly_emuspi_write_one_data(0x65);
	gpio_lcd_truly_emuspi_write_one_data(0x6E);
	gpio_lcd_truly_emuspi_write_one_data(0x78);
	gpio_lcd_truly_emuspi_write_one_data(0x80);
	gpio_lcd_truly_emuspi_write_one_data(0x88);
	gpio_lcd_truly_emuspi_write_one_data(0x8F);
	gpio_lcd_truly_emuspi_write_one_data(0x98);
	gpio_lcd_truly_emuspi_write_one_data(0xA0);
	gpio_lcd_truly_emuspi_write_one_data(0xA8);
	gpio_lcd_truly_emuspi_write_one_data(0xB1);
	gpio_lcd_truly_emuspi_write_one_data(0xBA);
	gpio_lcd_truly_emuspi_write_one_data(0xC3);
	gpio_lcd_truly_emuspi_write_one_data(0xCB);
	gpio_lcd_truly_emuspi_write_one_data(0xD3);
	gpio_lcd_truly_emuspi_write_one_data(0xDB);
	gpio_lcd_truly_emuspi_write_one_data(0xE4);
	gpio_lcd_truly_emuspi_write_one_data(0xEB);
	gpio_lcd_truly_emuspi_write_one_data(0xF3);
	gpio_lcd_truly_emuspi_write_one_data(0xFA);
	gpio_lcd_truly_emuspi_write_one_data(0xFF);
	gpio_lcd_truly_emuspi_write_one_data(0x20);
	gpio_lcd_truly_emuspi_write_one_data(0x35);
	gpio_lcd_truly_emuspi_write_one_data(0xE6);
	gpio_lcd_truly_emuspi_write_one_data(0x2D);
	gpio_lcd_truly_emuspi_write_one_data(0x8C);
	gpio_lcd_truly_emuspi_write_one_data(0x29);
	gpio_lcd_truly_emuspi_write_one_data(0xE3);
	gpio_lcd_truly_emuspi_write_one_data(0x2F);
	gpio_lcd_truly_emuspi_write_one_data(0xC0);
	gpio_lcd_truly_emuspi_write_one_data(0x04);
	gpio_lcd_truly_emuspi_write_one_data(0x0A);
	gpio_lcd_truly_emuspi_write_one_data(0x13);
	gpio_lcd_truly_emuspi_write_one_data(0x1A);
	gpio_lcd_truly_emuspi_write_one_data(0x21);
	gpio_lcd_truly_emuspi_write_one_data(0x29);
	gpio_lcd_truly_emuspi_write_one_data(0x31);
	gpio_lcd_truly_emuspi_write_one_data(0x37);
	gpio_lcd_truly_emuspi_write_one_data(0x3E);
	gpio_lcd_truly_emuspi_write_one_data(0x47);
	gpio_lcd_truly_emuspi_write_one_data(0x4F);
	gpio_lcd_truly_emuspi_write_one_data(0x56);
	gpio_lcd_truly_emuspi_write_one_data(0x5E);
	gpio_lcd_truly_emuspi_write_one_data(0x65);
	gpio_lcd_truly_emuspi_write_one_data(0x6E);
	gpio_lcd_truly_emuspi_write_one_data(0x78);
	gpio_lcd_truly_emuspi_write_one_data(0x80);
	gpio_lcd_truly_emuspi_write_one_data(0x88);
	gpio_lcd_truly_emuspi_write_one_data(0x8F);
	gpio_lcd_truly_emuspi_write_one_data(0x98);
	gpio_lcd_truly_emuspi_write_one_data(0xA0);
	gpio_lcd_truly_emuspi_write_one_data(0xA8);
	gpio_lcd_truly_emuspi_write_one_data(0xB1);
	gpio_lcd_truly_emuspi_write_one_data(0xBA);
	gpio_lcd_truly_emuspi_write_one_data(0xC3);
	gpio_lcd_truly_emuspi_write_one_data(0xCB);
	gpio_lcd_truly_emuspi_write_one_data(0xD3);
	gpio_lcd_truly_emuspi_write_one_data(0xDB);
	gpio_lcd_truly_emuspi_write_one_data(0xE4);
	gpio_lcd_truly_emuspi_write_one_data(0xEB);
	gpio_lcd_truly_emuspi_write_one_data(0xF3);
	gpio_lcd_truly_emuspi_write_one_data(0xFA);
	gpio_lcd_truly_emuspi_write_one_data(0xFF);
	gpio_lcd_truly_emuspi_write_one_data(0x20);
	gpio_lcd_truly_emuspi_write_one_data(0x35);
	gpio_lcd_truly_emuspi_write_one_data(0xE6);
	gpio_lcd_truly_emuspi_write_one_data(0x2D);
	gpio_lcd_truly_emuspi_write_one_data(0x8C);
	gpio_lcd_truly_emuspi_write_one_data(0x29);
	gpio_lcd_truly_emuspi_write_one_data(0xE3);
	gpio_lcd_truly_emuspi_write_one_data(0x2F);
	gpio_lcd_truly_emuspi_write_one_data(0xC0);
	gpio_lcd_truly_emuspi_write_one_data(0x04);
	gpio_lcd_truly_emuspi_write_one_data(0x0A);
	gpio_lcd_truly_emuspi_write_one_data(0x13);
	gpio_lcd_truly_emuspi_write_one_data(0x1A);
	gpio_lcd_truly_emuspi_write_one_data(0x21);
	gpio_lcd_truly_emuspi_write_one_data(0x29);
	gpio_lcd_truly_emuspi_write_one_data(0x31);
	gpio_lcd_truly_emuspi_write_one_data(0x37);
	gpio_lcd_truly_emuspi_write_one_data(0x3E);
	gpio_lcd_truly_emuspi_write_one_data(0x47);
	gpio_lcd_truly_emuspi_write_one_data(0x4F);
	gpio_lcd_truly_emuspi_write_one_data(0x56);
	gpio_lcd_truly_emuspi_write_one_data(0x5E);
	gpio_lcd_truly_emuspi_write_one_data(0x65);
	gpio_lcd_truly_emuspi_write_one_data(0x6E);
	gpio_lcd_truly_emuspi_write_one_data(0x78);
	gpio_lcd_truly_emuspi_write_one_data(0x80);
	gpio_lcd_truly_emuspi_write_one_data(0x88);
	gpio_lcd_truly_emuspi_write_one_data(0x8F);
	gpio_lcd_truly_emuspi_write_one_data(0x98);
	gpio_lcd_truly_emuspi_write_one_data(0xA0);
	gpio_lcd_truly_emuspi_write_one_data(0xA8);
	gpio_lcd_truly_emuspi_write_one_data(0xB1);
	gpio_lcd_truly_emuspi_write_one_data(0xBA);
	gpio_lcd_truly_emuspi_write_one_data(0xC3);
	gpio_lcd_truly_emuspi_write_one_data(0xCB);
	gpio_lcd_truly_emuspi_write_one_data(0xD3);
	gpio_lcd_truly_emuspi_write_one_data(0xDB);
	gpio_lcd_truly_emuspi_write_one_data(0xE4);
	gpio_lcd_truly_emuspi_write_one_data(0xEB);
	gpio_lcd_truly_emuspi_write_one_data(0xF3);
	gpio_lcd_truly_emuspi_write_one_data(0xFA);
	gpio_lcd_truly_emuspi_write_one_data(0xFF);
	gpio_lcd_truly_emuspi_write_one_data(0x20);
	gpio_lcd_truly_emuspi_write_one_data(0x35);
	gpio_lcd_truly_emuspi_write_one_data(0xE6);
	gpio_lcd_truly_emuspi_write_one_data(0x2D);
	gpio_lcd_truly_emuspi_write_one_data(0x8C);
	gpio_lcd_truly_emuspi_write_one_data(0x29);
	gpio_lcd_truly_emuspi_write_one_data(0xE3);
	gpio_lcd_truly_emuspi_write_one_data(0x2F);
	gpio_lcd_truly_emuspi_write_one_data(0xC0);
	SPI_Stop(); 
	SPI_Start(); 
	gpio_lcd_truly_emuspi_write_one_index(0xB3);  // SET Display  480x800 
	gpio_lcd_truly_emuspi_write_one_data(0x0f); 
	SPI_Stop();  
  	SPI_Start();
	gpio_lcd_truly_emuspi_write_one_index(0x3A); //SET pixel format 24-bit
	gpio_lcd_truly_emuspi_write_one_data(0x66);
	SPI_Stop(); 
  	SPI_Start();
	gpio_lcd_truly_emuspi_write_one_index(0x11); // sleep out command
	msleep(120);
	SPI_Stop(); 
  	SPI_Start();
	gpio_lcd_truly_emuspi_write_one_index(0x29); // display on command
	SPI_Stop(); 
	#endif 


	printk("lcd module TFT TRULY init finish\n!");

}

static void spi_init(void)
{
	spi_sclk = *(lcdc_tft_pdata->gpio_num);
	spi_cs   = *(lcdc_tft_pdata->gpio_num + 1);
	spi_sdi  = *(lcdc_tft_pdata->gpio_num + 2);
	spi_sdo  = *(lcdc_tft_pdata->gpio_num + 3);
	panel_reset = *(lcdc_tft_pdata->gpio_num + 4);
	printk("spi_init\n!");

	gpio_set_value(spi_sclk, 1);
	gpio_set_value(spi_sdo, 1);
	gpio_set_value(spi_cs, 1);


}
static void lcdc_oled_sleep(void)
{
	gpio_lcd_emuspi_write_one_index(0x14);
	gpio_lcd_emuspi_write_one_data(0x00);

	gpio_lcd_emuspi_write_one_index(0x1D);
	gpio_lcd_emuspi_write_one_data(0xA1);
	
	msleep(20);				

}
void lcdc_truly_sleep(void)
{
	SPI_Start(); 
	    gpio_lcd_truly_emuspi_write_one_index(0x28);  
	SPI_Stop(); 
	SPI_Start(); 
	    gpio_lcd_truly_emuspi_write_one_index(0x10);  
	SPI_Stop(); 
	mdelay(120);
}
void lcdc_lead_sleep(void)
{
	gpio_lcd_lead_emuspi_write_cmd(0x2800,0x00);
	gpio_lcd_lead_emuspi_write_cmd(0x1000,0x00);	
	msleep(200);	
	//gpio_lcd_lead_emuspi_write_cmd(0x2800,0x00);
}
static int lcdc_panel_off(struct platform_device *pdev)
{
	printk("lcdc_panel_off , g_lcd_panel_type is %d(1 LEAD. 2 TRULY. 3 OLED. )\n",g_lcd_panel_type);
	switch(g_lcd_panel_type)
	{
		case LCD_PANEL_TRULY_WVGA:
			//lcdc_truly_sleep();
			break;
		case LCD_PANEL_LEAD_WVGA:
			//lcdc_lead_sleep();
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
static int lcdc_panel_oled_off(struct platform_device *pdev)
{

	lcdc_oled_sleep();

	gpio_direction_output(panel_reset, 0);


	return 0;
}
static LCD_PANEL_TYPE lcd_panel_detect(void)
{
	unsigned int id_h,id_l,id;
	LCD_PANEL_TYPE panel_type;
	boolean read_truly_id=true;
	spi_init();
	if(read_truly_id)
	{
		SPI_Start(); 
	    gpio_lcd_truly_emuspi_write_one_index(0xB9);  // SET password 
	    gpio_lcd_truly_emuspi_write_one_data(0xFF);   
	    gpio_lcd_truly_emuspi_write_one_data(0x83);   
	    gpio_lcd_truly_emuspi_write_one_data(0x69);   
	  	SPI_Stop();   
		//Read from RF4H=R69H 
		//mdelay(100); 
		SPI_Start();
		gpio_lcd_truly_emuspi_write_one_index(0xFE);  // read id
		gpio_lcd_truly_emuspi_write_one_data(0xF4);   
	  	SPI_Stop();   

		SPI_Start();
		gpio_lcd_truly_emuspi_read_one_para(0xFF,&id);
		SPI_Stop();
		printk("truly id is 0x%x\n",id);
	}
	else
	{
		gpio_lcd_lead_emuspi_read_one_index(0x1080,&id_h);
		gpio_lcd_lead_emuspi_read_one_index(0x1180,&id_l);
		printk("lead id is 0x%x%x\n",id_h,id_l);
	}
	/*
	if((read_truly_id&&(id!=0x6902))||((!read_truly_id)&&((id_h==0x55)&&(id_l==0x80))))
	
	{
		printk("lead is selected\n");
		return LCD_PANEL_LEAD_WVGA;
	}
	else
	{
		printk("truly is selected\n");
		return LCD_PANEL_TRULY_WVGA;
	}
	*/
	if(id==0x6902)
	{
		panel_type=LCD_PANEL_TRULY_WVGA;
		return panel_type;
	}
	//gpio_lcd_lead_emuspi_read_one_index(0x1080,&id_h);
	//gpio_lcd_lead_emuspi_read_one_index(0x1180,&id_l);
	gpio_lcd_emuspi_write_one_index(0x00);
	gpio_lcd_emuspi_read_one_data(&id_h);
	gpio_lcd_emuspi_write_one_index(0x01);
	gpio_lcd_emuspi_read_one_data(&id_l);
	printk("oled id is 0x%x%x\n",id_h,id_l);
	if((id_h==0x27)&&(id_l==0x96))
	{
		panel_type=LCD_PANEL_OLED_WVGA;
		return panel_type;
	}
	panel_type=LCD_PANEL_LEAD_WVGA;
	return panel_type;
}
void lcd_panel_init(void)
{
	gpio_direction_output(panel_reset, 1);
	msleep(10);	
	gpio_direction_output(panel_reset, 0);
	msleep(20);		
	gpio_direction_output(panel_reset, 1);
	msleep(20);	
	switch(g_lcd_panel_type)
	{
		case LCD_PANEL_TRULY_WVGA:
			lcdc_truly_init();
			break;
		case LCD_PANEL_LEAD_WVGA:
			lcdc_lead_init();
			break;
		case LCD_PANEL_OLED_WVGA:
			lcdc_oled_init();
			break;
		default:
			break;
	}
}

static struct msm_fb_panel_data lcdc_tft_panel_data = {
       .panel_info = {.bl_max = 32},
	.on = lcdc_panel_on,
	.off = lcdc_panel_off,
       .set_backlight = lcdc_set_bl,
};
static struct msm_fb_panel_data lcdc_oled_panel_data = {
       .panel_info = {.bl_max = 12},
	.on = lcdc_panel_on,
	.off = lcdc_panel_oled_off,
       .set_backlight = lcdc_set_bl_oled,
};

static struct platform_device this_device = {
	.name   = "lcdc_panel_qvga",
	.id	= 1,
	.dev	= {
		.platform_data = &lcdc_tft_panel_data,
	}
};
static struct platform_device this_device2 = {
	.name   = "lcdc_panel_qvga",
	.id	= 1,
	.dev	= {
		.platform_data = &lcdc_oled_panel_data,
	}
};
static int __init lcdc_panel_probe(struct platform_device *pdev)
{
	struct msm_panel_info *pinfo;
	int ret;

	if(pdev->id == 0) {     
		lcdc_tft_pdata = pdev->dev.platform_data;
		lcdc_tft_pdata->panel_config_gpio(1);   

		g_lcd_panel_type = lcd_panel_detect();
		if(g_lcd_panel_type==LCD_PANEL_TRULY_WVGA)
		{
			pinfo = &lcdc_tft_panel_data.panel_info;
			pinfo->lcdc.h_back_porch = 5;
			pinfo->lcdc.h_front_porch = 5;
			pinfo->lcdc.h_pulse_width = 1;
			pinfo->lcdc.v_back_porch = 5;
			pinfo->lcdc.v_front_porch = 5;
			pinfo->lcdc.v_pulse_width = 1;
			pinfo->lcdc.border_clr = 0;	/* blk */
			pinfo->lcdc.underflow_clr = 0xffff;	/* blue */
			pinfo->lcdc.hsync_skew = 0;
		}
		else if(g_lcd_panel_type==LCD_PANEL_LEAD_WVGA)
		{
			pinfo = &lcdc_tft_panel_data.panel_info;
			pinfo->lcdc.h_back_porch = 3;
			pinfo->lcdc.h_front_porch = 3;
			pinfo->lcdc.h_pulse_width = 1;
			pinfo->lcdc.v_back_porch = 10;
			pinfo->lcdc.v_front_porch = 3;
			pinfo->lcdc.v_pulse_width = 1;
			pinfo->lcdc.border_clr = 0;	/* blk */
			pinfo->lcdc.underflow_clr = 0xffff;	/* blue */
			pinfo->lcdc.hsync_skew = 0;
		}
		else
		{
			pinfo = &lcdc_oled_panel_data.panel_info;
			pinfo->lcdc.h_back_porch = 8;
			pinfo->lcdc.h_front_porch = 8;
			pinfo->lcdc.h_pulse_width = 1;
			pinfo->lcdc.v_back_porch = 8;
			pinfo->lcdc.v_front_porch = 8;
			pinfo->lcdc.v_pulse_width = 1;
			pinfo->lcdc.border_clr = 0;	/* blk */
			pinfo->lcdc.underflow_clr = 0xffff;	/* blue */
			pinfo->lcdc.hsync_skew = 0;
		}
		pinfo->xres = 480;
		pinfo->yres = 800;		
		pinfo->type = LCDC_PANEL;
		pinfo->pdest = DISPLAY_1;
		pinfo->wait_cycle = 0;
		pinfo->bpp = 18;
		pinfo->fb_num = 2;
		switch(g_lcd_panel_type)
		{
			case LCD_PANEL_TRULY_WVGA:
				LcdPanleID=(u32)41; 
				pinfo->clk_rate = 24576000;
				ret = platform_device_register(&this_device);
				break;
			case LCD_PANEL_OLED_WVGA:
				LcdPanleID=(u32)40;  
				pinfo->clk_rate = 24576000;
				ret = platform_device_register(&this_device2);
				break;
			case LCD_PANEL_LEAD_WVGA:
				pinfo->clk_rate = 18432000;
				LcdPanleID=(u32)42; 
				ret = platform_device_register(&this_device);
				break;
			default:
				break;
		}		


    	//ret = platform_device_register(&this_device);
		
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



static int __init lcdc_oled_panel_init(void)
{
	int ret;

	ret = platform_driver_register(&this_driver);

	return ret;
}

module_init(lcdc_oled_panel_init);

