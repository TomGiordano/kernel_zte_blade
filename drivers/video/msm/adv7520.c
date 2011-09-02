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
#include <linux/time.h>
#include <linux/completion.h>
#include "msm_fb.h"

#include "external_common.h"

/* #define PORT_DEBUG */

static struct external_common_state_type hdmi_common;

static struct i2c_client *hclient;

static bool chip_power_on = FALSE;	/* For chip power on/off */
static bool gpio_power_on = FALSE;	/* For dtv power on/off (I2C) */

static u8 reg[256];	/* HDMI panel registers */

struct hdmi_data {
	struct msm_hdmi_platform_data *pd;
	struct work_struct work;
};
static struct hdmi_data *dd;
static struct work_struct handle_work;

#ifdef CONFIG_FB_MSM_HDMI_ADV7520_PANEL_HDCP_SUPPORT
static struct work_struct hdcp_handle_work;
static int hdcp_activating;
static DEFINE_MUTEX(hdcp_state_mutex);
#endif

static struct timer_list hpd_timer;
static unsigned int monitor_sense;

/* Change HDMI state */
static void change_hdmi_state(int online)
{
	if (!external_common_state)
		return;

	mutex_lock(&external_common_state_hpd_mutex);
	external_common_state->hpd_state = online;
	mutex_unlock(&external_common_state_hpd_mutex);

	if (!external_common_state->uevent_kobj)
		return;

	if (online)
		kobject_uevent(external_common_state->uevent_kobj,
			KOBJ_ONLINE);
	else
		kobject_uevent(external_common_state->uevent_kobj,
			KOBJ_OFFLINE);
	DEV_DBG("adv7520_uevent: %d\n", online);
}


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
	if (!gpio_power_on) {
		DEV_WARN("%s: WARN: missing GPIO power\n", __func__);
		return -ENODEV;
	}

	msg[0].addr = client->addr;
	msg[0].flags = 0;
	msg[0].len = 1;
	msg[0].buf = reg_buf;

	msg[1].addr = client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = data_buf;

	err = i2c_transfer(client->adapter, msg, 2);

	if (err < 0) {
		DEV_INFO("%s: I2C err: %d\n", __func__, err);
		return err;
	}

#ifdef PORT_DEBUG
	DEV_INFO("HDMI[%02x] [R] %02x\n", reg, data);
#endif
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
	if (!gpio_power_on) {
		DEV_WARN("%s: WARN: missing GPIO power\n", __func__);
		return -ENODEV;
	}

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 2;
	msg->buf = data;
	data[0] = reg;
	data[1] = val;

	err = i2c_transfer(client->adapter, msg, 1);
	if (err >= 0)
		return 0;
#ifdef PORT_DEBUG
	DEV_INFO("HDMI[%02x] [W] %02x [%d]\n", reg, val, err);
#endif
	return err;
}

#ifdef CONFIG_FB_MSM_HDMI_ADV7520_PANEL_HDCP_SUPPORT
static void adv7520_close_hdcp_link(void)
{
	DEV_INFO("HDCP ERROR Close HDMI link\n");

	reg[0xD5] = adv7520_read_reg(hclient, 0xD5);
	reg[0xD5] &= 0xFE;
	adv7520_write_reg(hclient, 0xD5, (u8)reg[0xD5]);

	reg[0x16] = adv7520_read_reg(hclient, 0x16);
	reg[0x16] &= 0xFE;
	adv7520_write_reg(hclient, 0x16, (u8)reg[0x16]);

	/* UnMute Audio */
	adv7520_write_reg(hclient, 0x0C, (u8)0x84);

	external_common_state->hdcp_active = FALSE;
	mutex_lock(&hdcp_state_mutex);
	hdcp_activating = FALSE;
	mutex_unlock(&hdcp_state_mutex);
}

static void adv7520_hdcp_enable(struct work_struct *work)
{
	DEV_INFO("%s: Start reg[0xaf]=%02x (mute audio)\n",
		__func__, reg[0xaf]);

	/* Mute Audio */
	adv7520_write_reg(hclient, 0x0C, (u8)0xC3);

	msleep(200);
	/* Wait for BKSV ready interrupt */
	/* Read BKSV's keys from HDTV */
	reg[0xBF] = adv7520_read_reg(hclient, 0xBF);
	reg[0xC0] = adv7520_read_reg(hclient, 0xC0);
	reg[0xC1] = adv7520_read_reg(hclient, 0xC1);
	reg[0xC2] = adv7520_read_reg(hclient, 0xC2);
	reg[0xc3] = adv7520_read_reg(hclient, 0xC3);

	DEV_INFO("BKSV={%02x,%02x,%02x,%02x,%02x}\n", reg[0xbf], reg[0xc0],
		reg[0xc1], reg[0xc2], reg[0xc3]);

	/* Is SINK repeater */
	reg[0xBE] = adv7520_read_reg(hclient, 0xBE);
	DEV_INFO("Sink Repeater reg is %x\n", reg[0xbe]);

	if (~(reg[0xBE] & 0x40)) {
		; /* compare with revocation list */
		/* Check 20 1's and 20 zero's */
	} else {
		/* Don't implement HDCP if sink as a repeater */
		adv7520_write_reg(hclient, 0x0C, (u8)0x84);
		mutex_lock(&hdcp_state_mutex);
		hdcp_activating = FALSE;
		mutex_unlock(&hdcp_state_mutex);
		DEV_INFO("%s Sink Repeater, (UnMute Audio)\n", __func__);
		return;
	}

	msleep(200);
	reg[0xB8] = adv7520_read_reg(hclient, 0xB8);
	DEV_INFO("%s HDCP Status: reg[0xB8] is %x\n",
						__func__, reg[0xb8]);
	if (reg[0xb8] & 0x40) {
		/* UnMute Audio */
		adv7520_write_reg(hclient, 0x0C, (u8)0x84);
		DEV_INFO("%s A/V content Encrypted (UnMute Audio)\n", __func__);
		external_common_state->hdcp_active = TRUE;
	}

	mutex_lock(&hdcp_state_mutex);
	hdcp_activating = FALSE;
	mutex_unlock(&hdcp_state_mutex);
}
#endif

static int adv7520_read_edid_block(int block, uint8 *edid_buf)
{
	u8 r = 0;
	int ret;
	struct i2c_msg msg[] = {
		{ .addr = reg[0x43] >> 1,
		  .flags = 0,
		  .len = 1,
		  .buf = &r },
		{ .addr = reg[0x43] >> 1,
		  .flags = I2C_M_RD,
		  .len = 0x100,
		  .buf = edid_buf } };

	if (block > 0)
		return 0;
	ret = i2c_transfer(hclient->adapter, msg, 2);
	DEV_DBG("EDID block: addr=%02x, ret=%d\n", reg[0x43] >> 1, ret);
	return (ret < 2) ? -ENODEV : 0;
}

static void adv7520_read_edid(void)
{
	external_common_state->read_edid_block = adv7520_read_edid_block;
	hdmi_common_read_edid();
}

static void adv7520_chip_on(void)
{
	if (!chip_power_on) {
		/* Get the current register holding the power bit. */
		unsigned long reg0x41 = adv7520_read_reg(hclient, 0x41);
		unsigned long reg0xaf = adv7520_read_reg(hclient, 0xaf);

		DEV_INFO("%s: turn on chip power\n", __func__);

		/* Enable power */
		/* Clear the power down bit to enable power. */
		clear_bit(6, &reg0x41);
		/* Set the HDMI select bit. */
		set_bit(1, &reg0xaf);

		adv7520_write_reg(hclient, 0x41, (u8)reg0x41);
		adv7520_write_reg(hclient, 0xaf, (u8)reg0xaf);
		chip_power_on = TRUE;
	} else
		DEV_INFO("%s: chip already has power\n", __func__);
}

static void adv7520_chip_off(void)
{
	if (chip_power_on) {
		unsigned long reg0x41;

		DEV_INFO("%s: turn off chip power\n", __func__);

		/* Power down the whole chip,except I2C,HPD interrupt */
		reg0x41 = adv7520_read_reg(hclient, 0x41);
		set_bit(6, &reg0x41);
		adv7520_write_reg(hclient, 0x41, (u8)reg0x41);
		chip_power_on = FALSE;
	} else
		DEV_INFO("%s: chip is already off\n", __func__);
}

/* Power ON/OFF  ADV7520 chip */
static void adb7520_chip_init(void);
static irqreturn_t adv7520_interrupt(int irq, void *dev_id);
static void adv7520_isr(struct work_struct *work);
static int adv7520_power_on(struct platform_device *pdev)
{
	static bool init_done;
	struct msm_fb_data_type *mfd = platform_get_drvdata(pdev);

	external_common_state->dev = &pdev->dev;
	if (mfd != NULL) {
		DEV_INFO("adv7520_power: ON (%dx%d %d)\n",
			mfd->var_xres, mfd->var_yres, mfd->var_pixclock);
		hdmi_common_get_video_format_from_drv_data(mfd);
	}

	gpio_power_on = TRUE;
	if (!init_done) {
		int rc;
		adb7520_chip_init();
		rc = dd->pd->init_irq();
		if (rc) {
			DEV_ERR("adv7520_init: init_irq: %d\n", rc);
			return rc;
		}
		init_done = TRUE;
	}

	DEV_INFO("%s: 'enable_irq'\n", __func__);
	if (adv7520_read_reg(hclient, 0x42) & (1 << 6)) {
		monitor_sense = adv7520_read_reg(hclient, 0xC6);
		schedule_work(&handle_work);
	}
	return request_irq(dd->pd->irq, &adv7520_interrupt,
		IRQF_TRIGGER_FALLING, "adv7520_cable", dd);
}

static int adv7520_power_off(struct platform_device *pdev)
{
	DEV_INFO("%s: 'disable_irq', chip off, I2C off\n", __func__);
	free_irq(dd->pd->irq, dd);
	adv7520_chip_off();

	gpio_power_on = FALSE;
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
static void adb7520_chip_init(void)
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

	/* Get the "HDMI/DVI Selection" register. */
	reg[0xaf] = adv7520_read_reg(hclient, 0xaf);

	/* Read Packet Memory I2C Address */
	reg[0x45] = adv7520_read_reg(hclient, 0x45);

	/* Hard coded values provided by ADV7520 data sheet. */
	reg[0x98] = 0x03;
	reg[0x9c] = 0x38;
	reg[0x9d] = 0x61;
	reg[0xa2] = 0x94;
	reg[0xa3] = 0x94;
	reg[0xde] = 0x88;

	/* Set the HDMI select bit. */
	reg[0xaf] |= 0x16;

	/* Set the audio related registers. */
	reg[0x01] = 0x00;
	reg[0x02] = 0x2d;
	reg[0x03] = 0x80;
	reg[0x0a] = 0x4d;
	reg[0x0b] = 0x0e;
	reg[0x0c] = 0x84;
	reg[0x0d] = 0x10;
	reg[0x12] = 0x00;
	reg[0x14] = 0x00;
	reg[0x15] = 0x20;
	reg[0x44] = 0x79;
	reg[0x73] = 0x01;
	reg[0x76] = 0x00;

	/* Set 720p display related registers */
	reg[0x16] = 0x00;

	reg[0x18] = 0x46;
	reg[0x55] = 0x00;
	reg[0x3c] = 0x04;

	/* Set Interrupt Mask register for HPD/HDCP */
	reg[0x94] = 0xC0;
#ifdef CONFIG_FB_MSM_HDMI_ADV7520_PANEL_HDCP_SUPPORT
	reg[0x95] = 0xC0;
#else
	reg[0x95] = 0x00;
#endif
	adv7520_write_reg(hclient, 0x94, reg[0x94]);
	adv7520_write_reg(hclient, 0x95, reg[0x95]);

	/* Set Packet Memory I2C Address */
	reg[0x45] = 0x74;

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
	reg[0x43] = 0x7E;
	adv7520_write_reg(hclient, 0x43, reg[0x43]);

	/* Enable the i2s audio input. */
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

	/* Enable 720p display */
	adv7520_write_reg(hclient, 0x16, reg[0x16]);
	adv7520_write_reg(hclient, 0x18, reg[0x18]);
	adv7520_write_reg(hclient, 0x55, reg[0x55]);
	adv7520_write_reg(hclient, 0x3c, reg[0x3c]);

	/* Set Packet Memory address to avoid conflict
	with Bosch Accelerometer */
	adv7520_write_reg(hclient, 0x45, reg[0x45]);
}

static void adv7520_handle_cable_work(struct work_struct *work)
{
	if (!gpio_power_on) {
		DEV_WARN("adv7520_timer: WARN GPIO power off, skipping\n");
		return;
	}

	if (monitor_sense & 0x4) {
		int timeout;
		DEV_DBG("adv7520_timer: Power ON\n");
		adv7520_chip_on();
		msleep(500);
		timeout = (adv7520_read_reg(hclient, 0x96) & (1 << 2));
		if (timeout) {
			DEV_DBG("adv7520_timer: EDID-Ready..\n");
			adv7520_read_edid();

#ifdef CONFIG_FB_MSM_HDMI_ADV7520_PANEL_HDCP_SUPPORT
			mutex_lock(&hdcp_state_mutex);
			if (hdcp_activating) {
				DEV_WARN("adv7520_timer: HDCP already"
					" activating, skipping\n");
				mutex_unlock(&hdcp_state_mutex);
				return;
			}
			hdcp_activating = TRUE;
			mutex_unlock(&hdcp_state_mutex);

			msleep(500);
			/* request for HDCP after EDID is read */
			reg[0xaf] = adv7520_read_reg(hclient, 0xaf);
			reg[0xaf] |= 0x90;
			adv7520_write_reg(hclient, 0xaf, reg[0xaf]);
			reg[0xaf] = adv7520_read_reg(hclient, 0xaf);
			DEV_INFO("adv7520_timer: Start HDCP reg[0xaf]=0x%02x\n",
						reg[0xaf]);

			reg[0xba] = adv7520_read_reg(hclient, 0xba);
			reg[0xba] |= 0x10;
			adv7520_write_reg(hclient, 0xba, reg[0xba]);
			reg[0xba] = adv7520_read_reg(hclient, 0xba);
			DEV_INFO("adv7520_timer: reg[0xba]=0x%02x\n",
						reg[0xba]);
			msleep(500);
#endif
		} else
			DEV_DBG("adv7520_timer: EDID TIMEOUT\n");
		change_hdmi_state(1);
	} else {
		change_hdmi_state(0);
		adv7520_chip_off();
		DEV_DBG("adv7520_timer: Power OFF\n");
	}
}
static void adv7520_handle_cable(unsigned long data)
{
	schedule_work(&handle_work);
}

static void adv7520_isr(struct work_struct *work)
{
	u8 reg0xc8;
	u8 reg0x96 = adv7520_read_reg(hclient, 0x96);
#ifdef CONFIG_FB_MSM_HDMI_ADV7520_PANEL_HDCP_SUPPORT
	u8 reg0x97 = adv7520_read_reg(hclient, 0x97);
	DEV_INFO("adv7520_irq: reg[0x96]=%x reg[0x97]=%x\n", reg0x96, reg0x97);
	/* Clearing the Interrupts */
	adv7520_write_reg(hclient, 0x97, reg0x97);
#else
	DEV_INFO("adv7520_irq: reg[0x96]=%x\n", reg0x96);
#endif
	/* Clearing the Interrupts */
	adv7520_write_reg(hclient, 0x96, reg0x96);

	if ((reg0x96 == 0xC0) || (reg0x96 & 0x40)) {
		unsigned int hpd_state = adv7520_read_reg(hclient, 0x42);
		monitor_sense = adv7520_read_reg(hclient, 0xC6);
		DEV_DBG("adv7520_irq: reg[0x42]=%x && reg[0xC6]=%x\n",
			hpd_state, monitor_sense);

		/* Timer for catching interrupt debouning */
		if (!timer_pending(&hpd_timer)) {
			init_timer(&hpd_timer);
			DEV_DBG("adv7520_irq: Add Timer\n");
			hpd_timer.function = adv7520_handle_cable;
			hpd_timer.data = (unsigned long)NULL;
			hpd_timer.expires = jiffies + 1*HZ;
			add_timer(&hpd_timer);
		} else {
			DEV_DBG("adv7520_irq: MOD Timer pending\n");
			mod_timer(&hpd_timer, jiffies + 1*HZ);
		}
	}
#ifdef CONFIG_FB_MSM_HDMI_ADV7520_PANEL_HDCP_SUPPORT
	if (hdcp_activating) {
		/* HDCP controller error Interrupt */
		if (reg0x97 & 0x80) {
			DEV_ERR("adv7520_irq: HDCP_ERROR\n");
			adv7520_close_hdcp_link();
		/* BKSV Ready interrupts */
		} else if (reg0x97 & 0x40) {
			DEV_INFO("adv7520_irq: BKSV keys ready, Begin"
				" HDCP encryption\n");
			schedule_work(&hdcp_handle_work);
		}
		reg0xc8 = adv7520_read_reg(hclient, 0xc8);
		DEV_INFO("DDC controller reg[0xC8]=0x%02x\n", reg0xc8);
	} else
		DEV_INFO("adv7520_irq: final reg[0x96]=%02x reg[0x97]=%02x\n",
			reg0x96, reg0x97);
#endif
}

static irqreturn_t adv7520_interrupt(int irq, void *dev_id)
{
	if (!gpio_power_on) {
		DEV_WARN("adv7520_irq: WARN: GPIO power off, skipping\n");
		return IRQ_HANDLED;
	}
	schedule_work(&dd->work);
	return IRQ_HANDLED;
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
	int rc;
	struct platform_device *fb_dev;

	dd = kzalloc(sizeof *dd, GFP_KERNEL);
	if (!dd) {
		rc = -ENOMEM;
		goto probe_exit;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
		return -ENODEV;

	external_common_state->dev = &client->dev;

	/* Init real i2c_client */
	hclient = client;

	i2c_set_clientdata(client, dd);
	dd->pd = client->dev.platform_data;
	if (!dd->pd) {
		rc = -ENODEV;
		goto probe_free;
	}

	INIT_WORK(&dd->work, adv7520_isr);
	INIT_WORK(&handle_work, adv7520_handle_cable_work);
#ifdef CONFIG_FB_MSM_HDMI_ADV7520_PANEL_HDCP_SUPPORT
	INIT_WORK(&hdcp_handle_work, adv7520_hdcp_enable);
#endif
	fb_dev = msm_fb_add_device(&hdmi_device);
	if (fb_dev) {
		rc = external_common_state_create(fb_dev);
		if (rc)
			goto probe_free;
	} else
		DEV_ERR("adv7520_probe: failed to add fb device\n");

	return 0;

probe_free:
	i2c_set_clientdata(client, NULL);
	kfree(dd);
probe_exit:
	return rc;

}

static int __devexit adv7520_remove(struct i2c_client *client)
{
	int err = 0;
	if (!client->adapter) {
		DEV_ERR("%s: No HDMI Device\n", __func__);
		return -ENODEV;
	}
	return err;
}

static struct i2c_driver hdmi_i2c_driver = {
	.driver		= {
		.name   = ADV7520_DRV_NAME,
	},
	.probe		= adv7520_probe,
	.id_table	= adv7520_id,
	.remove		= __devexit_p(adv7520_remove),
};

static int __init adv7520_init(void)
{
	int rc;

	external_common_state = &hdmi_common;
	external_common_state->video_resolution = HDMI_VFRMT_1280x720p60_16_9;
	HDMI_SETUP_LUT(640x480p60_4_3);
	HDMI_SETUP_LUT(720x480p60_16_9);
	HDMI_SETUP_LUT(1280x720p60_16_9);

	hdmi_common_init_panel_info(&hdmi_panel_data.panel_info);

	rc = i2c_add_driver(&hdmi_i2c_driver);
	if (rc) {
		pr_err("hdmi_init FAILED: i2c_add_driver rc=%d\n", rc);
		goto init_exit;
	}

	if (machine_is_msm7x30_surf() || machine_is_msm8x55_surf()) {
		short *hdtv_mux = (short *)ioremap(0x8e000170 , 0x100);
		*hdtv_mux++ = 0x020b;
		*hdtv_mux = 0x8000;
		iounmap(hdtv_mux);
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
