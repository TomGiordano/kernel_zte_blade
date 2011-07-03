/*
 * =====================================================================================
 *
 *       Filename:  lcdc_panel_hvga_tvout.c
 *
 *    Description:  TVOUT HVGA panel(480x320) driver
 *
 *        Version:  1.0
 *        Created:  09/15/2010
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  
 *        Company:  ZTE Corp.
 *
 * =====================================================================================
 */
/* ========================================================================================
when         who        what, where, why                                  comment tag
--------     ----       -----------------------------                --------------------------
2010-12-27	luya		modify some reg for sharpness CRDB00593231	ZTE_LCD_LUYA_20101227_001
2010-11-11	luya		modify some reg for sharpness		         	ZTE_LCD_LUYA_20101111_001
2010-09-20	luya		add BKL adjust & output format change         	ZTE_LCD_LUYA_20100920_001
2010-09-13	luya		modify reg setting			         	ZTE_LCD_LUYA_20100913_001
==========================================================================================*/


#include "msm_fb.h"
#include <asm/gpio.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/i2c.h>


#define LCD_BL_LEVEL 32
#define lcd_bl_max   lcd_bl_level-1
#define lcd_bl_min   0

static boolean is_firsttime = true;		/
static boolean tvic_can_write = false;
static unsigned short reg_lev = 0;	   

extern u32 LcdPanleID;   
static int tv_18;
static int tv_25;
static int tv_rst;
static int tv_33;

struct ch7026_work {
	struct work_struct work;
};

static struct  ch7026_work *ch7026_sensorw;
static struct  i2c_client *ch7026_client;

enum ch7026_width {
	WORD_LEN,
	BYTE_LEN
};

typedef enum
{
	TV_FORMAT_NONE = 0,
	TV_FORMAT_PAL_BDGHI,
	TV_FORMAT_NTSC_M,
	TV_FORMAT_PAL_M,
	TV_FORMAT_PAL_N,
	TV_FORMAT_PAL_Nc,
	TV_FORMAT_PAL_60,
	TV_FORMAT_NTSC_J,
	TV_FORMAT_NTSC_443,
}TV_FORMAT_TYPE;

TV_FORMAT_TYPE	tv_format_id=TV_FORMAT_PAL_BDGHI;

struct ch7026_i2c_reg_conf {
	unsigned short waddr;
	unsigned short wdata;
	enum ch7026_width width;
	unsigned short mdelay_time;
};
static struct msm_panel_common_pdata * lcdc_tvout_pdata;
static void lcdc_reset(void);
static void lcdc_set_bl(struct msm_fb_data_type *mfd);

static void gpio_init(void);
static int lcdc_panel_on(struct platform_device *pdev);
static int lcdc_panel_off(struct platform_device *pdev);
static int32_t ch7026_i2c_write_table(struct ch7026_i2c_reg_conf const *reg_conf_tbl,int num_of_items_in_table);
static int32_t ch7026_i2c_write(unsigned short saddr,	unsigned short waddr, unsigned short wdata, enum ch7026_width width);
int tvic_change_format(void);

//
static struct ch7026_i2c_reg_conf const reg_init_tbl[] = {
	{ 0x02, 0x01, BYTE_LEN, 0 },
	{ 0x02, 0x03, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x04, 0x39, BYTE_LEN, 0 },

	{ 0x09, 0x80, BYTE_LEN, 0 },

	
//	{ 0x0A, 0x10, BYTE_LEN, 0 },
	{ 0x0C, 0x82, BYTE_LEN, 0 },
	{ 0x0D, 0x03, BYTE_LEN, 0 },
//
	{ 0x0F, 0x12, BYTE_LEN, 0 },
//	{ 0x30, 0xC0, BYTE_LEN, 0 },
	
	{ 0x10, 0x80, BYTE_LEN, 0 },
	{ 0x11, 0x92, BYTE_LEN, 0 },
	{ 0x12, 0x40, BYTE_LEN, 0 },
	{ 0x13, 0x08, BYTE_LEN, 0 },
	{ 0x14, 0x02, BYTE_LEN, 0 },
	{ 0x15, 0x09, BYTE_LEN, 0 },
	{ 0x16, 0xE0, BYTE_LEN, 0 },
	{ 0x17, 0xF2, BYTE_LEN, 0 },
	{ 0x19, 0x08, BYTE_LEN, 0 },
	{ 0x1A, 0x02, BYTE_LEN, 0 },

	{ 0x1B, 0x34, BYTE_LEN, 0 },
	{ 0x1C, 0xF0, BYTE_LEN, 0 },
	
	{ 0x1D, 0xC0, BYTE_LEN, 0 },
	{ 0x21, 0x11, BYTE_LEN, 0 },
	{ 0x22, 0xD0, BYTE_LEN, 0 },
	{ 0x23, 0x71, BYTE_LEN, 0 },

	{ 0x2F, 0x50, BYTE_LEN, 0 },
	{ 0x30, 0x2E, BYTE_LEN, 0 },
	{ 0x31, 0x83, BYTE_LEN, 0 },
	
	{ 0x32, 0x77, BYTE_LEN, 0 },
	
	{ 0x35, 0x09, BYTE_LEN, 0 },
	{ 0x36, 0x00, BYTE_LEN, 0 },
/
	{ 0x3E, 0xA0, BYTE_LEN, 0 },
	{ 0x3F, 0x8F, BYTE_LEN, 0 },
	
	{ 0x41, 0xDE, BYTE_LEN, 0 },

	{ 0x46, 0x00, BYTE_LEN, 0 },
	{ 0x47, 0x00, BYTE_LEN, 0 },
	{ 0x48, 0x00, BYTE_LEN, 0 },
	{ 0x49, 0x10, BYTE_LEN, 0 },
	{ 0x4A, 0x6F, BYTE_LEN, 0 },
	{ 0x4B, 0x7A, BYTE_LEN, 0 },
	{ 0x4C, 0x5A, BYTE_LEN, 0 },
	
	{ 0x4D, 0x04, BYTE_LEN, 0 },
	{ 0x4E, 0x27, BYTE_LEN, 0 },
	{ 0x4F, 0x62, BYTE_LEN, 0 },
	{ 0x50, 0x76, BYTE_LEN, 0 },
	{ 0x51, 0x51, BYTE_LEN, 0 },
	{ 0x52, 0x1B, BYTE_LEN, 0 },
	{ 0x53, 0x1A, BYTE_LEN, 0 },
	{ 0x55, 0xE5, BYTE_LEN, 0 },

	{ 0x58, 0x00, BYTE_LEN, 0 },

	{ 0x5E, 0x80, BYTE_LEN, 0 },

	{ 0x70, 0x15, BYTE_LEN, 0 },
	{ 0x71, 0x15, BYTE_LEN, 0 },
	{ 0x72, 0x10, BYTE_LEN, 0 },
	{ 0x73, 0x10, BYTE_LEN, 0 },
	{ 0x74, 0x89, BYTE_LEN, 0 },
	{ 0x78, 0x22, BYTE_LEN, 0 },
	{ 0x79, 0x18, BYTE_LEN, 0 },
	{ 0x7A, 0x18, BYTE_LEN, 0 },
	{ 0x7B, 0x00, BYTE_LEN, 0 },
	{ 0x7C, 0x00, BYTE_LEN, 0 },

	{ 0x7D, 0x62, BYTE_LEN, 0 },
	{ 0x7E, 0x08, BYTE_LEN, 0 },
	
	{ 0x04, 0x38, BYTE_LEN, 0 },
	{ 0x06, 0x71, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x06, 0x70, BYTE_LEN, 0 },
	{ 0x02, 0x02, BYTE_LEN, 0 },
	{ 0x02, 0x03, BYTE_LEN, 0 },
	{ 0x04, 0x00, BYTE_LEN, 0 },
	
};

static struct ch7026_i2c_reg_conf const reg_pal_bdghi_tbl[] = {
	{ 0x0D, 0x83, BYTE_LEN, 0 },
		
	{ 0x1C, 0x1E, BYTE_LEN, 0 },
	{ 0x1D, 0xC0, BYTE_LEN, 0 },
	{ 0x21, 0x12, BYTE_LEN, 0 },
	{ 0x22, 0x20, BYTE_LEN, 0 },
	{ 0x23, 0x71, BYTE_LEN, 0 },
	
	{ 0x36, 0xB0, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x06, 0x70, BYTE_LEN, 0 },
	{ 0x02, 0x02, BYTE_LEN, 0 },
	{ 0x02, 0x03, BYTE_LEN, 0 },
	{ 0x04, 0x30, BYTE_LEN, 0 },

};

static struct ch7026_i2c_reg_conf const reg_ntsc_m_tbl[] = {
	{ 0x0D, 0x80, BYTE_LEN, 0 },
		
	{ 0x1C, 0x46, BYTE_LEN, 0 },
	{ 0x1D, 0xB4, BYTE_LEN, 0 },
	{ 0x21, 0x11, BYTE_LEN, 0 },
	{ 0x22, 0xE0, BYTE_LEN, 0 },
	{ 0x23, 0x0D, BYTE_LEN, 0 },
	{ 0x36, 0x00, BYTE_LEN, 0 },
	
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x06, 0x70, BYTE_LEN, 0 },
	{ 0x02, 0x02, BYTE_LEN, 0 },
	{ 0x02, 0x03, BYTE_LEN, 0 },
	{ 0x04, 0x30, BYTE_LEN, 0 },

};

static struct ch7026_i2c_reg_conf const reg_pal_m_tbl[] = {
	{ 0x0D, 0x84, BYTE_LEN, 0 },
		
	{ 0x1C, 0x46, BYTE_LEN, 0 },
	{ 0x1D, 0xB4, BYTE_LEN, 0 },
	{ 0x21, 0x11, BYTE_LEN, 0 },
	{ 0x22, 0xE0, BYTE_LEN, 0 },
	{ 0x23, 0x0D, BYTE_LEN, 0 },
	{ 0x36, 0x00, BYTE_LEN, 0 },
	
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x06, 0x70, BYTE_LEN, 0 },
	{ 0x02, 0x02, BYTE_LEN, 0 },
	{ 0x02, 0x03, BYTE_LEN, 0 },
	{ 0x04, 0x30, BYTE_LEN, 0 },

};

static struct ch7026_i2c_reg_conf const reg_pal_n_tbl[] = {
	{ 0x0D, 0x85, BYTE_LEN, 0 },
		
	{ 0x1C, 0x1E, BYTE_LEN, 0 },
	{ 0x1D, 0xC0, BYTE_LEN, 0 },
	{ 0x21, 0x12, BYTE_LEN, 0 },
	{ 0x22, 0x20, BYTE_LEN, 0 },
	{ 0x23, 0x71, BYTE_LEN, 0 },
	
	{ 0x36, 0xB0, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x06, 0x70, BYTE_LEN, 0 },
	{ 0x02, 0x02, BYTE_LEN, 0 },
	{ 0x02, 0x03, BYTE_LEN, 0 },
	{ 0x04, 0x30, BYTE_LEN, 0 },

};

static struct ch7026_i2c_reg_conf const reg_pal_nc_tbl[] = {
	{ 0x0D, 0x86, BYTE_LEN, 0 },
		
	{ 0x1C, 0x1E, BYTE_LEN, 0 },
	{ 0x1D, 0xC0, BYTE_LEN, 0 },
	{ 0x21, 0x12, BYTE_LEN, 0 },
	{ 0x22, 0x20, BYTE_LEN, 0 },
	{ 0x23, 0x71, BYTE_LEN, 0 },
	
	{ 0x36, 0xB0, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x06, 0x70, BYTE_LEN, 0 },
	{ 0x02, 0x02, BYTE_LEN, 0 },
	{ 0x02, 0x03, BYTE_LEN, 0 },
	{ 0x04, 0x30, BYTE_LEN, 0 },

};


static struct ch7026_i2c_reg_conf const reg_pal_60_tbl[] = {
	{ 0x0D, 0x87, BYTE_LEN, 0 },
		
	{ 0x1C, 0x46, BYTE_LEN, 0 },
	{ 0x1D, 0xB4, BYTE_LEN, 0 },
	{ 0x21, 0x11, BYTE_LEN, 0 },
	{ 0x22, 0xE0, BYTE_LEN, 0 },
	{ 0x23, 0x0D, BYTE_LEN, 0 },
	{ 0x36, 0x00, BYTE_LEN, 0 },
	
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x06, 0x70, BYTE_LEN, 0 },
	{ 0x02, 0x02, BYTE_LEN, 0 },
	{ 0x02, 0x03, BYTE_LEN, 0 },
	{ 0x04, 0x30, BYTE_LEN, 0 },

};


static struct ch7026_i2c_reg_conf const reg_ntsc_j_tbl[] = {
	{ 0x0D, 0x81, BYTE_LEN, 0 },
		
	{ 0x1C, 0x46, BYTE_LEN, 0 },
	{ 0x1D, 0xB4, BYTE_LEN, 0 },
	{ 0x21, 0x11, BYTE_LEN, 0 },
	{ 0x22, 0xE0, BYTE_LEN, 0 },
	{ 0x23, 0x0D, BYTE_LEN, 0 },
	{ 0x36, 0x00, BYTE_LEN, 0 },
	
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x06, 0x70, BYTE_LEN, 0 },
	{ 0x02, 0x02, BYTE_LEN, 0 },
	{ 0x02, 0x03, BYTE_LEN, 0 },
	{ 0x04, 0x30, BYTE_LEN, 0 },

};


static struct ch7026_i2c_reg_conf const reg_ntsc_443_tbl[] = {
	{ 0x0D, 0x82, BYTE_LEN, 0 },
		
	{ 0x1C, 0x46, BYTE_LEN, 0 },
	{ 0x1D, 0xB4, BYTE_LEN, 0 },
	{ 0x21, 0x11, BYTE_LEN, 0 },
	{ 0x22, 0xE0, BYTE_LEN, 0 },
	{ 0x23, 0x0D, BYTE_LEN, 0 },
	{ 0x36, 0x00, BYTE_LEN, 0 },
	
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x03, 0x00, BYTE_LEN, 0 },
	{ 0x06, 0x70, BYTE_LEN, 0 },
	{ 0x02, 0x02, BYTE_LEN, 0 },
	{ 0x02, 0x03, BYTE_LEN, 0 },
	{ 0x04, 0x30, BYTE_LEN, 0 },

};


static int lcdc_panel_on(struct platform_device *pdev)
{

	printk("LUYA :lcdc_panel_on!!\n");


	gpio_init();

	gpio_direction_output(tv_33, 1);
	mdelay(10);
	gpio_direction_output(tv_18, 1);
	mdelay(10);
	gpio_direction_output(tv_25, 1);
	mdelay(10);


	if(!is_firsttime)
	{
	printk("LUYA :lcdc_panel_on in!!\n");

		lcdc_reset();
		ch7026_i2c_write_table(&reg_init_tbl[0], ARRAY_SIZE(reg_init_tbl));
	}
	else
	{
		is_firsttime = false;
	}
	return 0;
}

static void lcdc_reset(void)
{
	/* reset lcd module */
	gpio_direction_output(tv_rst, 1);
	mdelay(10);
	gpio_direction_output(tv_rst, 0);
	mdelay(20);
	gpio_direction_output(tv_rst, 1);
	mdelay(10);	
}

/
static void lcdc_set_bl(struct msm_fb_data_type *mfd)
{
       /*value range is 1--32*/
    int current_lel = mfd->bl_level;
	   
    printk("[BKL] lcdc_set_bl level=%d, %d\n", 
		   current_lel , mfd->panel_power_on);

    if((!mfd->panel_power_on)||(!tvic_can_write))
	{
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

    if(current_lel==0)
    	{
    	}
    else 
	{
//		reg_lev = 4*current_lel - 1;
//		ch7026_i2c_write(0x76,0x30, reg_lev,BYTE_LEN);
		reg_lev = 149 + (current_lel - 16)*6;
		ch7026_i2c_write(0x76,0x31, reg_lev,BYTE_LEN);

    	}
}

int tvic_change_format(void)
{
	int ret = 0;
	tv_format_id++;
	tv_format_id = tv_format_id%9;
	
	if(!tvic_can_write)
	{
	    return -EPERM;
    	}

	switch(tv_format_id)
	{
		case TV_FORMAT_PAL_BDGHI:
			ret = ch7026_i2c_write_table(&reg_pal_bdghi_tbl[0], ARRAY_SIZE(reg_pal_bdghi_tbl));
			break;
		case TV_FORMAT_NTSC_M:
			ret = ch7026_i2c_write_table(&reg_ntsc_m_tbl[0], ARRAY_SIZE(reg_ntsc_m_tbl));
			break;
		case TV_FORMAT_PAL_M:
			ret = ch7026_i2c_write_table(&reg_pal_m_tbl[0], ARRAY_SIZE(reg_pal_m_tbl));
			break;
		case TV_FORMAT_PAL_N:
			ret = ch7026_i2c_write_table(&reg_pal_n_tbl[0], ARRAY_SIZE(reg_pal_n_tbl));
			break;
		case TV_FORMAT_PAL_Nc:
			ret = ch7026_i2c_write_table(&reg_pal_nc_tbl[0], ARRAY_SIZE(reg_pal_nc_tbl));
			break;
		case TV_FORMAT_PAL_60:
			ret = ch7026_i2c_write_table(&reg_pal_60_tbl[0], ARRAY_SIZE(reg_pal_60_tbl));
			break;
		case TV_FORMAT_NTSC_J:
			ret = ch7026_i2c_write_table(&reg_ntsc_j_tbl[0], ARRAY_SIZE(reg_ntsc_j_tbl));
			break;
		case TV_FORMAT_NTSC_443:
			ret = ch7026_i2c_write_table(&reg_ntsc_443_tbl[0], ARRAY_SIZE(reg_ntsc_443_tbl));
			break;
		default:
		printk("LUYA: unknown format (format=%d) received!\n", tv_format_id);
		ret = -EINVAL;
		break;
			
	}
	return ret;
}
static void gpio_init(void)
{
	tv_18 = *(lcdc_tvout_pdata->gpio_num);
	tv_25   =  *(lcdc_tvout_pdata->gpio_num + 1);
	tv_rst  =  *(lcdc_tvout_pdata->gpio_num + 2);
	tv_33  =  *(lcdc_tvout_pdata->gpio_num + 3);

}
static int lcdc_panel_off(struct platform_device *pdev)
{

	gpio_direction_output(tv_rst, 0);
	gpio_direction_output(tv_25, 0);
	gpio_direction_output(tv_18, 0);
	gpio_direction_output(tv_33, 0);

	return 0;
}

static struct msm_fb_panel_data lcdc_tvout_panel_data = {
       .panel_info = {.bl_max = 32},
	.on = lcdc_panel_on,
	.off = lcdc_panel_off,
       .set_backlight = lcdc_set_bl,
};


static int32_t ch7026_i2c_txdata(unsigned short saddr,
	unsigned char *txdata, int length)
{
	int32_t rc = -EIO;

	struct i2c_msg msg[] = {
		{
			.addr = saddr,
			.flags = 0,
			.len = length,
			.buf = txdata,
		},
	};
	
	rc = i2c_transfer(ch7026_client->adapter, msg, 1) ;
	if (rc < 0) {
		printk("ch7026_i2c_txdata failed\n");
		return -EIO;
	}

	return 0;
}

static int32_t ch7026_i2c_write(unsigned short saddr,
	unsigned short waddr, unsigned short wdata, enum ch7026_width width)
{
	int32_t rc = -EIO;
	unsigned char buf[4];

	memset(buf, 0, sizeof(buf));
	switch (width) {
	case WORD_LEN: {
		buf[0] = (waddr & 0xFF00)>>8;
		buf[1] = (waddr & 0x00FF);
		buf[2] = (wdata & 0xFF00)>>8;
		buf[3] = (wdata & 0x00FF);

		rc = ch7026_i2c_txdata(saddr, buf, 4);
	}
		break;

	case BYTE_LEN: {
		buf[0] = waddr;
		buf[1] = wdata;
		rc = ch7026_i2c_txdata(saddr, buf, 2);
	}
		break;

	default:
		break;
	}

	if (rc < 0)
		printk(
		"i2c_write failed, addr = 0x%x, val = 0x%x!\n",
		waddr, wdata);

	return rc;
}

static int32_t ch7026_i2c_write_table(
	struct ch7026_i2c_reg_conf const *reg_conf_tbl,
	int num_of_items_in_table)
{
	int i;
	int32_t rc = -EIO;

	for (i = 0; i < num_of_items_in_table; i++) {
		rc = ch7026_i2c_write(ch7026_client->addr,
			reg_conf_tbl->waddr, reg_conf_tbl->wdata,
			reg_conf_tbl->width);
		if (rc < 0)
			break;
		if (reg_conf_tbl->mdelay_time != 0)
			mdelay(reg_conf_tbl->mdelay_time);
		reg_conf_tbl++;
	}

	return rc;
}
static int ch7026_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{

	int rc = 0;
	printk("LUYA :ch7026_i2c_probe4!!\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		return -EIO;
	}


	ch7026_sensorw =
		kzalloc(sizeof(struct ch7026_work), GFP_KERNEL);

	if (!ch7026_sensorw) {
		rc = -ENOMEM;
		goto probe_failure;
	}
	i2c_set_clientdata(client, ch7026_sensorw);

	ch7026_client = client;

	printk("ch7026_probe succeeded!\n");

	lcdc_reset();
	ch7026_i2c_write_table(&reg_init_tbl[0], ARRAY_SIZE(reg_init_tbl));
	tvic_can_write = true;

	return 0;

probe_failure:
	kfree(ch7026_sensorw);
	ch7026_sensorw = NULL;
	printk("ch7026_probe failed!\n");
	return rc;

}

static struct platform_device this_device = {
	.name   = "lcdc_panel_qvga",
	.id	= 1,
	.dev	= {
		.platform_data = &lcdc_tvout_panel_data,
	}
};

static const struct i2c_device_id ch7026_i2c_id[] = {
	{ "ch7026", 0},
	{ },
};

static struct i2c_driver ch7026_i2c_driver = {
	.id_table = ch7026_i2c_id,
	.probe  = ch7026_i2c_probe,
	.remove = NULL,
	.driver = {
		.name = "ch7026",
	},
};

static int __init lcdc_panel_probe(struct platform_device *pdev)
{
	struct msm_panel_info *pinfo;
	int ret;

	if(pdev->id == 0) {
		lcdc_tvout_pdata = pdev->dev.platform_data;
		lcdc_tvout_pdata->panel_config_gpio(1);
		gpio_init();	
	 	LcdPanleID=(u32)LCD_PANEL_P726_S6D04M0X01;   
		pinfo = &lcdc_tvout_panel_data.panel_info;
		pinfo->xres = 640;
		pinfo->yres = 480;
		pinfo->type = LCDC_PANEL;
		pinfo->pdest = DISPLAY_1;
		pinfo->wait_cycle = 0;
		pinfo->bpp = 18;
		pinfo->fb_num = 2;

		pinfo->clk_rate = 10240000;
		
		pinfo->lcdc.h_back_porch = 8;
		pinfo->lcdc.h_front_porch = 8;
		pinfo->lcdc.h_pulse_width = 2;
		pinfo->lcdc.v_back_porch = 8;
		pinfo->lcdc.v_front_porch = 8;
		pinfo->lcdc.v_pulse_width = 2;
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



static int __init lcdc_tvout_panel_init(void)
{
	int ret;
	i2c_add_driver(&ch7026_i2c_driver);
	
	ret = platform_driver_register(&this_driver);

	if(ret)
	{
		printk("[lxw@lcd&fb]:unregist tvout driver!\n");
		platform_driver_unregister(&this_driver);
		return ret;
	}

	return ret;
}

module_init(lcdc_tvout_panel_init);

