/* Copyright (c) 2010, Code Aurora Forum. All rights reserved.
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

#include <linux/i2c.h>
#include <linux/types.h>
#include <linux/bitops.h>
#include <linux/adv7520.h>
#include "msm_fb.h"


static struct i2c_client *hclient;
static struct i2c_client *eclient;

struct msm_panel_info *pinfo;
static bool power_on = FALSE;	/* For power on/off */

static u8 reg[256];	/* HDMI panel registers */
static short  *hdtv_mux ;

/*
 * Read a value from a register on ADV7520 device
 * If sucessfull returns value read , otherwise error.
 */
static u8 adv7520_read_reg(struct i2c_client *client, u8 reg)
{
	int err;
	struct i2c_msg msg[2];
	u8 reg_buf[] = { reg };
	u8 data_buf[] = { 0 };

	if (!client->adapter)
		return -ENODEV;

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = reg_buf;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = data_buf;

	err = i2c_transfer(client->adapter, msg, 2);

	if (err < 0)
		return err;

	return *data_buf;
}

/*
 * Write a value to a register on adv7520 device.
 * Returns zero if successful, or non-zero otherwise.
 */
static int adv7520_write_reg(struct i2c_client *client, u8 reg, u8 val)
{
	int err;
	struct i2c_msg msg[1];
	unsigned char data[2];

	if (!client->adapter)
		return -ENODEV;

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 2;
	msg->buf = data;
	data[0] = reg;
	data[1] = val;

	err = i2c_transfer(client->adapter, msg, 1);
	if (err >= 0)
		return 0;
	return err;
}



/*  Power ON/OFF  ADV7520 chip */
static int adv7520_power_on(struct platform_device *pdev)
{
	unsigned long reg0x41 = 0xff, reg0x42 = 0xff, reg0xaf = 0xff;
	bool hpd_set = TRUE;

	/* Attempting to enable power */
	if (!power_on) {
		/* Get the register holding the HPD bit, this must
		   be set before power up. */
		reg0x42 = adv7520_read_reg(hclient, 0x42);
		if (!test_bit(6, &reg0x42))
			hpd_set = FALSE;
	}

	if (hpd_set) {

		/* Get the current register holding the power bit. */
		reg0x41 = adv7520_read_reg(hclient, 0x41);
		reg0xaf = adv7520_read_reg(hclient, 0xaf);

		/* Enable power */
		if (!power_on) {
			/* Clear the power down bit to enable power. */
			clear_bit(6, &reg0x41);
			/* Set the HDMI select bit. */
			set_bit(1, &reg0xaf);

			adv7520_write_reg(hclient, 0x41, (u8)reg0x41);
			adv7520_write_reg(hclient, 0xaf, (u8)reg0xaf);
			power_on = TRUE ;
			}
		}

	return 0;
}

static int adv7520_power_off(struct platform_device *pdev)
{
	unsigned long reg0x41 = 0xff;

		if (power_on) {
			reg0x41 = adv7520_read_reg(hclient, 0x41);
			/* Power down the whole chip,except I2C,HPD interrupt */
			reg0x41 = adv7520_read_reg(hclient, 0x41);
			set_bit(6, &reg0x41);
			adv7520_write_reg(hclient, 0x41, (u8)reg0x41);
			power_on = FALSE ;
		}
	return 0;
}

#if DEBUG
/* Read status registers for debugging */
static void adv7520_read_status(void)
{
	adv7520_read_reg(hclient, 0x94);
	adv7520_read_reg(hclient, 0x95);
	adv7520_read_reg(hclient, 0x97);
	adv7520_read_reg(hclient, 0xb8);
	adv7520_read_reg(hclient, 0xc8);
	adv7520_read_reg(hclient, 0x41);
	adv7520_read_reg(hclient, 0x42);
	adv7520_read_reg(hclient, 0xa1);
	adv7520_read_reg(hclient, 0xb4);
	adv7520_read_reg(hclient, 0xc5);
	adv7520_read_reg(hclient, 0x3e);
	adv7520_read_reg(hclient, 0x3d);
	adv7520_read_reg(hclient, 0xaf);
	adv7520_read_reg(hclient, 0xc6);

}
#else
#define adv7520_read_status() do {} while (0)
#endif

/* AV7520 chip specific initialization */
static void adv7520_enable(void)
{
	/* Initialize the variables used to read/write the ADV7520 chip. */
	memset(&reg, 0xff, sizeof(reg));

	/* Get the values from the "Fixed Registers That Must Be Set". */
	reg[0x98] = adv7520_read_reg(hclient, 0x98);
	reg[0x9c] = adv7520_read_reg(hclient, 0x9c);
	reg[0x9d] = adv7520_read_reg(hclient, 0x9d);
	reg[0xa2] = adv7520_read_reg(hclient, 0xa2);
	reg[0xa3] = adv7520_read_reg(hclient, 0xa3);
	reg[0xde] = adv7520_read_reg(hclient, 0xde);

	/* EDID Registers */
	reg[0x43] = adv7520_read_reg(hclient, 0x43);
	reg[0xc8] = adv7520_read_reg(hclient, 0xc8);

	/* Get the "HDMI/DVI Selection" register. */
	reg[0xaf] = adv7520_read_reg(hclient, 0xaf);

	/* Hard coded values provided by ADV7520 data sheet. */
	reg[0x98] = 0x03;
	reg[0x9c] = 0x38;
	reg[0x9d] = 0x61 ;
	reg[0xa2] = 0x94;
	reg[0xa3] = 0x94;
	reg[0xde] = 0x88;

	/* Set the HDMI select bit. */
	reg[0xaf] |= 0x16;

	/* Set the audio related registers. */
	reg[0x01] = 0x00;
	reg[0x02] = 0x2d;
	reg[0x03] = 0x80;
	reg[0x0a] = 0x4d ;
	reg[0x0b] = 0x0e ;
	reg[0x0c] =  0x84 ;
	reg[0x0d] =  0x10 ;
	reg[0x12] =  0x00 ;
	reg[0x14] =  0x00 ;
	reg[0x15] =  0x20 ;
	reg[0x44] =  0x79 ;
	reg[0x73] =  0x1 ;
	reg[0x76] =  0x00 ;

	/* Set 720p display related registers */
	reg[0x16] = 0x00;
	reg[0x18] = 0x46;
	reg[0x55] = 0x00;
	reg[0xba] = 0x60;
	reg[0x3c] = 0x04;

	/* Set the values from the "Fixed Registers That Must Be Set". */
	adv7520_write_reg(hclient, 0x98, reg[0x98]);
	adv7520_write_reg(hclient, 0x9c, reg[0x9c]);
	adv7520_write_reg(hclient, 0x9d, reg[0x9d]);
	adv7520_write_reg(hclient, 0xa2, reg[0xa2]);
	adv7520_write_reg(hclient, 0xa3, reg[0xa3]);
	adv7520_write_reg(hclient, 0xde, reg[0xde]);

	/* Set the "HDMI/DVI Selection" register. */
	adv7520_write_reg(hclient, 0xaf, reg[0xaf]);

	/* Set EDID Monitor address */
	reg[0x43] = ADV7520_EDIDI2CSLAVEADDRESS ;
	adv7520_write_reg(hclient, 0x43, reg[0x43]);

	/*  Enable the i2s audio input.  */
	adv7520_write_reg(hclient, 0x01, reg[0x01]);
	adv7520_write_reg(hclient, 0x02, reg[0x02]);
	adv7520_write_reg(hclient, 0x03, reg[0x03]);
	adv7520_write_reg(hclient, 0x0a, reg[0x0a]);
	adv7520_write_reg(hclient, 0x0b, reg[0x0b]);
	adv7520_write_reg(hclient, 0x0c, reg[0x0c]);
	adv7520_write_reg(hclient, 0x0d, reg[0x0d]);
	adv7520_write_reg(hclient, 0x12, reg[0x12]);
	adv7520_write_reg(hclient, 0x14, reg[0x14]);
	adv7520_write_reg(hclient, 0x15, reg[0x15]);
	adv7520_write_reg(hclient, 0x44, reg[0x44]);
	adv7520_write_reg(hclient, 0x73, reg[0x73]);
	adv7520_write_reg(hclient, 0x76, reg[0x76]);

       /* Enable  720p display */
	adv7520_write_reg(hclient, 0x16, reg[0x16]);
	adv7520_write_reg(hclient, 0x18, reg[0x18]);
	adv7520_write_reg(hclient, 0x55, reg[0x55]);
	adv7520_write_reg(hclient, 0xba, reg[0xba]);
	adv7520_write_reg(hclient, 0x3c, reg[0x3c]);

}

static const struct i2c_device_id adv7520_id[] = {
	{ ADV7520_DRV_NAME , 0},
	{}
};

static struct msm_fb_panel_data hdmi_panel_data = {
	.on  = adv7520_power_on,
	.off = adv7520_power_off,
};

static struct platform_device hdmi_device = {
	.name = ADV7520_DRV_NAME ,
	.id   = 2,
	.dev  = {
		.platform_data = &hdmi_panel_data,
		}
};

static int __devinit
adv7520_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
		return -ENODEV;

	/* Init real i2c_client */
	hclient = client;
	adv7520_enable();
	msm_fb_add_device(&hdmi_device);
	return 0;
}

static int __devexit adv7520_remove(struct i2c_client *client)
{
	int err = 0 ;
	if (!client->adapter) {
		printk(KERN_ERR "<%s> No HDMI Device \n",
			__func__);
		return -ENODEV;
	}
	i2c_unregister_device(eclient);
	return err ;
}

static struct i2c_driver hdmi_i2c_driver = {
	.driver		= {
		.name   = ADV7520_DRV_NAME,
	},
	.probe 		= adv7520_probe,
	.id_table 	= adv7520_id,
	.remove		= __devexit_p(adv7520_remove),
};

static int __init adv7520_init(void)
{
	int rc;
	pinfo = &hdmi_panel_data.panel_info;

	pinfo->xres = 1280 ;
	pinfo->yres = 720 ;
	pinfo->type = DTV_PANEL;
	pinfo->pdest = DISPLAY_2;
	pinfo->wait_cycle = 0;
	pinfo->bpp = 24;
	pinfo->fb_num = 1;
	pinfo->clk_rate = 74250000;
	pinfo->lcdc.h_back_porch = 220;
	pinfo->lcdc.h_front_porch = 110;
	pinfo->lcdc.h_pulse_width = 40;
	pinfo->lcdc.v_back_porch = 20;
	pinfo->lcdc.v_front_porch = 5;
	pinfo->lcdc.v_pulse_width = 5;
	pinfo->lcdc.border_clr = 0;	/* blk */
	pinfo->lcdc.underflow_clr = 0xff;	/* blue */
	pinfo->lcdc.hsync_skew = 0;

	rc = i2c_add_driver(&hdmi_i2c_driver);
	if (rc) {
		printk(KERN_ERR "hdmi_init FAILED: i2c_add_driver rc=%d\n", rc);
		goto init_exit;
	}

	if (machine_is_msm7x30_surf()) {
		hdtv_mux  = (short *)ioremap(0x8e000170 , 0x100);
		*hdtv_mux++  = 0x0202;
		*hdtv_mux  = 0x8000;
	}
	return 0;

init_exit:
	return rc;
}

static void __exit adv7520_exit(void)
{
	i2c_del_driver(&hdmi_i2c_driver);
}

module_init(adv7520_init);
module_exit(adv7520_exit);
MODULE_LICENSE("GPL v2");
MODULE_VERSION("0.1");
MODULE_AUTHOR("Qualcomm Innovation Center, Inc.");
MODULE_DESCRIPTION("ADV7520 HDMI driver");
