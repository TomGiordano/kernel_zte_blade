/*
 *  isa1200.c - Haptic Motor
 *
 *  Copyright (C) 2009 Samsung Electronics
 *  Kyungmin Park <kyungmin.park@samsung.com>
 *  Copyright (c) 2010, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/pwm.h>
#include <linux/workqueue.h>
#include <linux/i2c/isa1200.h>
#include "../staging/android/timed_output.h"

#define ISA1200_SCTRL0		0x00
#define ISA1200_THSWRST		(1 << 7)
#define ISA1200_EXT2DIV		(1 << 4)
#define ISA1200_LDOADJ_24V	(0x9 << 0)
#define ISA1200_LDOADJ_25V	(0xa << 0)
#define ISA1200_LDOADJ_26V	(0xb << 0)
#define ISA1200_LDOADJ_27V	(0xc << 0)
#define ISA1200_LDOADJ_28V	(0xd << 0)
#define ISA1200_LDOADJ_29V	(0xe << 0)
#define ISA1200_LDOADJ_30V	(0xf << 0)
#define ISA1200_LDOADJ_31V	(0x0 << 0)
#define ISA1200_LDOADJ_32V	(0x1 << 0)
#define ISA1200_LDOADJ_33V	(0x2 << 0)
#define ISA1200_LDOADJ_34V	(0x3 << 0)
#define ISA1200_LDOADJ_35V	(0x4 << 0)
#define ISA1200_LDOADJ_36V	(0x5 << 0)
#define ISA1200_LDOADJ_MASK	(0xf << 0)
#define ISA1200_HCTRL0		0x30
#define ISA1200_HAPDREN		(1 << 7)
#define ISA1200_OVEREN		(1 << 6)
#define ISA1200_OVERHL		(1 << 5)
#define ISA1200_HAPDIGMOD_PWM_IN	(1 << 3)
#define ISA1200_HAPDIGMOD_PWM_GEN	(2 << 3)
#define ISA1200_PLLMOD		(1 << 2)
#define ISA1200_PWMMOD_DIVIDER_128	(0 << 0)
#define ISA1200_PWMMOD_DIVIDER_256	(1 << 0)
#define ISA1200_PWMMOD_DIVIDER_512	(2 << 0)
#define ISA1200_PWMMOD_DIVIDER_1024	(3 << 0)
#define ISA1200_HCTRL1		0x31
#define ISA1200_EXTCLKSEL	(1 << 7)
#define ISA1200_BIT6_ON		(1 << 6)
#define ISA1200_MOTTYP_ERM	(1 << 5)
#define ISA1200_MOTTYP_LRA	(0 << 5)
#define ISA1200_PLLEN		(1 << 4)
#define ISA1200_SMARTEN		(1 << 3)
#define ISA1200_SMARTONT	(1 << 2)
#define ISA1200_SMARTOFFT_16	(0 << 0)
#define ISA1200_SMARTOFFT_32	(1 << 0)
#define ISA1200_SMARTOFFT_64	(2 << 0)
#define ISA1200_SMARTOFFT_100	(3 << 0)
#define ISA1200_HCTRL2		0x32
#define ISA1200_HSWRST		(1 << 7)
#define ISA1200_SESTMOD		(1 << 2)
#define ISA1200_SEEN		(1 << 1)
#define ISA1200_SEEVENT		(1 << 0)
#define ISA1200_HCTRL3		0x33
#define ISA1200_PPLLDIV_MASK	(0xf0)
#define ISA1200_PPLLDIV_SHIFT	(4)
#define ISA1200_PPLLDIV_1	(0x0)
#define ISA1200_PPLLDIV_2	(0x1)
#define ISA1200_PPLLDIV_4	(0x2)
#define ISA1200_PPLLDIV_8	(0x3)
#define ISA1200_PPLLDIV_16	(0x4)
#define ISA1200_PPLLDIV_32	(0x5)
#define ISA1200_PPLLDIV_64	(0x6)
#define ISA1200_PPLLDIV_128	(0x7)
#define ISA1200_WPLLDIV_MASK	(0x0f)
#define ISA1200_WPLLDIV_SHIFT	(0)
#define ISA1200_WPLLDIV_1	(0x0)
#define ISA1200_WPPLLDIV_2	(0x1)
#define ISA1200_WPPLLDIV_4	(0x2)
#define ISA1200_WPPLLDIV_8	(0x3)
#define ISA1200_WPPLLDIV_16	(0x4)
#define ISA1200_WPPLLDIV_32	(0x5)
#define ISA1200_WPPLLDIV_64	(0x6)
#define ISA1200_WPPLLDIV_128	(0x7)
#define ISA1200_HCTRL4		0x34
#define ISA1200_HCTRL5		0x35
#define ISA1200_HCTRL6		0x36
#define ISA1200_HCTRL7		0x37
#define ISA1200_HCTRL8		0x38
#define ISA1200_HCTRL9		0x39
#define ISA1200_PLLP_SHIFT	(4)
#define ISA1200_PLLP_MASK	(0xf0)
#define ISA1200_PLLS_SHIFT	(0)
#define ISA1200_PLLS_MASK	(0x0f)
#define ISA1200_HCTRLA		0x3A
#define ISA1200_PLLMM_MASK	(0xfc)
#define ISA1200_PLLMM_SHIFT	(2)
#define ISA1200_PLLMS_MASK	(0x03)
#define ISA1200_PLLMS_SHIFT	(0)
#define ISA1200_HCTRLB		0x3B
#define ISA1200_HCTRLC		0x3C
#define ISA1200_HCTRLD		0x3D

#define PWM_HAPTIC_PERIOD	44640
#define PWM_HAPTIC_DEFAULT_LEVEL	99

struct isa1200_chip {
	struct i2c_client *client;
	struct isa1200_platform_data *pdata;
	struct pwm_device *pwm;
	struct hrtimer timer;
	struct timed_output_dev dev;
	struct work_struct work;
	spinlock_t lock;

	int enable;
	int powered;
};

static int isa1200_chip_set_pwm_cycle(struct isa1200_chip *haptic)
{
	return pwm_config(haptic->pwm, PWM_HAPTIC_PERIOD/2, PWM_HAPTIC_PERIOD);
}

static int isa1200_read_reg(struct i2c_client *client, int reg)
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ret;
}

static int isa1200_write_reg(struct i2c_client *client, int reg, u8 value)
{
	int ret;

	ret = i2c_smbus_write_byte_data(client, reg, value);
	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ret;
}

static void isa1200_chip_power_on(struct isa1200_chip *haptic)
{
	if (haptic->powered)
		return;
	haptic->powered = 1;
	/* Use smart mode enable control */
	pwm_enable(haptic->pwm);
}

static void isa1200_chip_power_off(struct isa1200_chip *haptic)
{
	if (!haptic->powered)
		return;
	haptic->powered = 0;
	/* Use smart mode enable control */
	pwm_disable(haptic->pwm);
}

static void isa1200_chip_work(struct work_struct *work)
{
	struct isa1200_chip *haptic;
	int ret;

	haptic = container_of(work, struct isa1200_chip, work);
	if (haptic->enable) {
		ret = isa1200_chip_set_pwm_cycle(haptic);
		if (ret) {
			dev_dbg(haptic->dev.dev, "set_pwm_cycle failed\n");
			return;
		}
		isa1200_chip_power_on(haptic);
	} else {
		isa1200_chip_power_off(haptic);
	}
}

static void isa1200_chip_enable(struct timed_output_dev *dev, int value)
{
	struct isa1200_chip *haptic = container_of(dev, struct isa1200_chip,
					dev);
	unsigned long flags;

	spin_lock_irqsave(&haptic->lock, flags);
	hrtimer_cancel(&haptic->timer);
	if (value == 0)
		haptic->enable = 0;
	else {
		value = (value > haptic->pdata->max_timeout ?
				haptic->pdata->max_timeout : value);
		haptic->enable = 1;
		hrtimer_start(&haptic->timer,
			ktime_set(value / 1000, (value % 1000) * 1000000),
			HRTIMER_MODE_REL);
	}
	spin_unlock_irqrestore(&haptic->lock, flags);
	schedule_work(&haptic->work);
}

static int isa1200_chip_get_time(struct timed_output_dev *dev)
{
	struct isa1200_chip *haptic = container_of(dev, struct isa1200_chip,
					dev);

	if (hrtimer_active(&haptic->timer)) {
		ktime_t r = hrtimer_get_remaining(&haptic->timer);
		return r.tv.sec * 1000 + r.tv.nsec / 1000000;
	} else
		return 0;
}

static enum hrtimer_restart isa1200_vib_timer_func(struct hrtimer *timer)
{
	struct isa1200_chip *haptic = container_of(timer, struct isa1200_chip,
					timer);
	haptic->enable = 0;
	schedule_work(&haptic->work);
	return HRTIMER_NORESTART;
}
static void isa1200_setup(struct i2c_client *client)
{
	struct isa1200_chip *haptic = i2c_get_clientdata(client);
	int value;

	gpio_set_value(haptic->pdata->hap_en_gpio, 0);
	udelay(250);
	gpio_set_value(haptic->pdata->hap_en_gpio, 1);

	value = ISA1200_HAPDREN | ISA1200_OVERHL | ISA1200_HAPDIGMOD_PWM_IN |
		ISA1200_PWMMOD_DIVIDER_128;
	isa1200_write_reg(client, ISA1200_HCTRL0, value);

	value = ISA1200_EXTCLKSEL | ISA1200_BIT6_ON | ISA1200_MOTTYP_LRA |
		ISA1200_SMARTEN | ISA1200_SMARTOFFT_64;
	isa1200_write_reg(client, ISA1200_HCTRL1, value);

	value = isa1200_read_reg(client, ISA1200_HCTRL2);
	value |= ISA1200_SEEN;
	isa1200_write_reg(client, ISA1200_HCTRL2, value);
}

static int __devinit isa1200_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct isa1200_chip *haptic;
	struct isa1200_platform_data *pdata;
	int ret;

	if (!i2c_check_functionality(client->adapter,
			I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&client->dev, "%s: no support for i2c read/write"
				"byte data\n", __func__);
		return -EIO;
	}

	pdata = client->dev.platform_data;
	if (!pdata) {
		dev_err(&client->dev, "%s: no platform data\n", __func__);
		return -EINVAL;
	}

	haptic = kzalloc(sizeof(struct isa1200_chip), GFP_KERNEL);
	if (!haptic)
		return -ENOMEM;

	haptic->client = client;
	haptic->enable = 0;
	haptic->pdata = pdata;

	if (pdata->power_on) {
		ret = pdata->power_on(1);
		if (ret) {
			dev_err(&client->dev, "%s: power-up failed\n",
							__func__);
			goto pwr_up_fail;
		}
	}

	haptic->pwm = pwm_request(pdata->pwm_ch_id, id->name);
	if (IS_ERR(haptic->pwm)) {
		dev_err(&client->dev, "%s: pwm request failed\n", __func__);
		ret = PTR_ERR(haptic->pwm);
		goto pwm_req_fail;
	}

	spin_lock_init(&haptic->lock);
	INIT_WORK(&haptic->work, isa1200_chip_work);

	hrtimer_init(&haptic->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	haptic->timer.function = isa1200_vib_timer_func;

	/*register with timed output class*/
	haptic->dev.name = pdata->name;
	haptic->dev.get_time = isa1200_chip_get_time;
	haptic->dev.enable = isa1200_chip_enable;
	ret = timed_output_dev_register(&haptic->dev);
	if (ret < 0)
		goto timed_reg_fail;

	i2c_set_clientdata(client, haptic);

	ret = gpio_is_valid(pdata->hap_en_gpio);
	if (ret) {
		ret = gpio_request(pdata->hap_en_gpio, "haptic_gpio");
		if (ret) {
			dev_err(&client->dev, "%s: gpio %d request failed\n",
					__func__, pdata->hap_en_gpio);
			goto gpio_fail;
		}
	} else {
		dev_err(&client->dev, "%s: Invalid gpio %d\n", __func__,
					pdata->hap_en_gpio);
		goto gpio_fail;
	}

	isa1200_setup(client);
	printk(KERN_INFO "%s: %s registered\n", __func__, id->name);
	return 0;

gpio_fail:
	timed_output_dev_unregister(&haptic->dev);
timed_reg_fail:
	pwm_free(haptic->pwm);
pwm_req_fail:
	if (pdata->power_on)
		pdata->power_on(0);
pwr_up_fail:
	kfree(haptic);
	return ret;
}

static int __devexit isa1200_remove(struct i2c_client *client)
{
	struct isa1200_chip *haptic = i2c_get_clientdata(client);

	cancel_work_sync(&haptic->work);
	hrtimer_cancel(&haptic->timer);
	timed_output_dev_unregister(&haptic->dev);
	gpio_free(haptic->pdata->hap_en_gpio);

	if (haptic->pdata->power_on)
		haptic->pdata->power_on(0);

	pwm_free(haptic->pwm);
	kfree(haptic);
	return 0;
}
#ifdef CONFIG_PM
static int isa1200_suspend(struct i2c_client *client, pm_message_t mesg)
{
	struct isa1200_chip *haptic = i2c_get_clientdata(client);

	cancel_work_sync(&haptic->work);
	hrtimer_cancel(&haptic->timer);
	isa1200_chip_power_off(haptic);

	if (haptic->pdata->power_on)
		haptic->pdata->power_on(0);

	return 0;
}

/* REVISIT: resolve i2c errors while resume though motors work fine*/
static int isa1200_resume(struct i2c_client *client)
{
	struct isa1200_chip *haptic = i2c_get_clientdata(client);
	int ret;

	if (haptic->pdata->power_on) {
		ret = haptic->pdata->power_on(1);
		if (ret) {
			dev_err(&client->dev, "power-up failed\n");
			return ret;
		}
	}

	isa1200_setup(client);
	return 0;
}
#else
#define isa1200_suspend		NULL
#define isa1200_resume		NULL
#endif

static const struct i2c_device_id isa1200_id[] = {
	{ "isa1200_1", 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, isa1200_id);

static struct i2c_driver isa1200_driver = {
	.driver	= {
		.name	= "isa1200",
	},
	.probe		= isa1200_probe,
	.remove		= __devexit_p(isa1200_remove),
	.suspend	= isa1200_suspend,
	.resume		= isa1200_resume,
	.id_table	= isa1200_id,
};

static int __init isa1200_init(void)
{
	return i2c_add_driver(&isa1200_driver);
}

static void __exit isa1200_exit(void)
{
	i2c_del_driver(&isa1200_driver);
}

module_init(isa1200_init);
module_exit(isa1200_exit);

MODULE_AUTHOR("Kyungmin Park <kyungmin.park@samsung.com>");
MODULE_DESCRIPTION("ISA1200 Haptic Motor driver");
MODULE_LICENSE("GPL");
