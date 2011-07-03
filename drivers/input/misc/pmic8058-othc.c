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

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/switch.h>
#include <linux/pm.h>

#include <linux/mfd/pmic8058.h>
#include <linux/pmic8058-othc.h>

#define PM8058_OTHC_LOW_CURR_MASK	0xF0
#define PM8058_OTHC_HIGH_CURR_MASK	0x0F
#define PM8058_OTHC_EN_SIG_MASK		0x3F
#define PM8058_OTHC_HYST_PREDIV_MASK	0xC7
#define PM8058_OTHC_CLK_PREDIV_MASK	0xF8
#define PM8058_OTHC_HYST_CLK_MASK	0x0F
#define PM8058_OTHC_PERIOD_CLK_MASK	0xF0

#define PM8058_OTHC_LOW_CURR_SHIFT	0x4
#define PM8058_OTHC_EN_SIG_SHIFT	0x6
#define PM8058_OTHC_HYST_PREDIV_SHIFT	0x3
#define PM8058_OTHC_HYST_CLK_SHIFT	0x4

enum {
	NO_DEVICE,
	MSM_HEADSET,
};

struct pm8058_othc {
	struct input_dev *othc_ipd;
	struct switch_dev othc_sdev;
	struct pmic8058_othc_config_pdata *othc_pdata;
	int othc_base;
	int othc_irq_sw;
	int othc_irq_ir;
	bool othc_sw_state;
	bool othc_ir_state;
	int othc_nc_gpio;
	struct pm8058_chip *pm_chip;
	struct work_struct switch_work;
};

static struct pm8058_othc *config[OTHC_MICBIAS_MAX];

/*
 * The API pm8058_micbias_enable() allows to configure
 * the MIC_BIAS. Only the lines which are not used for
 * headset detection can be configured using this API.
 * The API returns an error code if it fails to configure
 * the specified MIC_BIAS line, else it returns 0.
 */
int pm8058_micbias_enable(enum othc_micbias micbias,
		enum othc_micbias_enable enable)
{
	int rc;
	u8 reg;
	struct pm8058_othc *dd = config[micbias];

	if (dd == NULL) {
		pr_err("%s:MIC_BIAS not registered, cannot enable\n",
						__func__);
		return -ENODEV;
	}

	if (dd->othc_pdata->micbias_capability != OTHC_MICBIAS) {
		pr_err("%s:MIC_BIAS enable capability not supported\n",
							__func__);
		return -EINVAL;
	}

	rc = pm8058_read(dd->pm_chip, dd->othc_base + 1, &reg, 1);
	if (rc < 0) {
		pr_err("%s: PM8058 read failed \n", __func__);
		return rc;
	}

	reg &= PM8058_OTHC_EN_SIG_MASK;
	reg |= (enable << PM8058_OTHC_EN_SIG_SHIFT);

	rc = pm8058_write(dd->pm_chip, dd->othc_base + 1, &reg, 1);
	if (rc < 0) {
		pr_err("%s: PM8058 write failed \n", __func__);
		return rc;
	}

	return rc;
}
EXPORT_SYMBOL(pm8058_micbias_enable);

#ifdef CONFIG_PM
static int pm8058_othc_suspend(struct device *dev)
{
	struct pm8058_othc *dd = dev_get_drvdata(dev);

	if (device_may_wakeup(dev)) {
		enable_irq_wake(dd->othc_irq_sw);
		enable_irq_wake(dd->othc_irq_ir);
	}

	return 0;
}

static int pm8058_othc_resume(struct device *dev)
{
	struct pm8058_othc *dd = dev_get_drvdata(pd);

	if (device_may_wakeup(dev)) {
		disable_irq_wake(dd->othc_irq_sw);
		disable_irq_wake(dd->othc_irq_ir);
	}

	return 0;
}

static struct dev_pm_ops pm8058_othc_pm_ops = {
	.suspend = pm8058_othc_suspend,
	.resume = pm8058_othc_resume,
};
#endif

static int __devexit pm8058_othc_remove(struct platform_device *pd)
{
	struct pm8058_othc *dd = platform_get_drvdata(pd);

	platform_set_drvdata(pd, NULL);
	device_init_wakeup(&pd->dev, 0);

	free_irq(dd->othc_irq_sw, dd);
	free_irq(dd->othc_irq_ir, dd);

	input_unregister_device(dd->othc_ipd);
	kfree(dd);

	return 0;
}

static void switch_work_f(struct work_struct *work)
{
	struct pm8058_othc *dd =
		container_of(work, struct pm8058_othc, switch_work);

	if (dd->othc_ir_state == true)
		switch_set_state(&dd->othc_sdev, 1);
	else
		switch_set_state(&dd->othc_sdev, 0);

	enable_irq(dd->othc_irq_ir);
}

static void
pm8058_headset_switch(struct input_dev *dev, int key, int value)
{
	struct pm8058_othc *dd = input_get_drvdata(dev);

	input_report_switch(dev, key, value);
	schedule_work(&dd->switch_work);
}

/*
 * The pm8058_no_sw detects the switch press and release operation.
 * The odd number call is press and even number call is release.
 * The current state of the button is maintained in othc_sw_state variable.
 * This isr gets called only for NO type headsets.
 */
static irqreturn_t pm8058_no_sw(int irq, void *dev_id)
{
	struct pm8058_othc *dd = dev_id;

	if (dd->othc_sw_state == false) {
		/*  Switch has been pressed */
		dd->othc_sw_state = true;
		input_report_key(dd->othc_ipd, KEY_MEDIA, 1);
	} else {
		/* Switch has been released */
		dd->othc_sw_state = false;
		input_report_key(dd->othc_ipd, KEY_MEDIA, 0);
	}
	input_sync(dd->othc_ipd);

	return IRQ_HANDLED;
}

/*
 * For the NC type headset, a single interrupt is triggered
 * for all the events (insert, remove, press, release).
 * A external GPIO is used to distinguish between diff. states.
 * The GPIO is pulled low when the headset is inserted. Using
 * the current state of the switch, headset and the gpio value we
 * determine which event has occured.
 */
static void othc_process_nc(struct pm8058_othc *dd)
{
	if (!gpio_get_value(dd->othc_nc_gpio)) {
		/* headset plugged in and/or button press/release */
		if (dd->othc_ir_state == false) {
			/* disable irq, this gets enabled in the workqueue */
			disable_irq_nosync(dd->othc_irq_ir);
			/* headset plugged in */
			dd->othc_ir_state = true;
			pm8058_headset_switch(dd->othc_ipd,
					SW_HEADPHONE_INSERT, 1);
		} else if (dd->othc_sw_state == false) {
			/*  Switch has been pressed */
			dd->othc_sw_state = true;
			input_report_key(dd->othc_ipd, KEY_MEDIA, 1);
		} else {
			/* Switch has been released */
			dd->othc_sw_state = false;
			input_report_key(dd->othc_ipd, KEY_MEDIA, 0);
		}
	} else {
		/* disable irq, this gets enabled in the workqueue */
		disable_irq_nosync(dd->othc_irq_ir);
		/* headset has been plugged out */
		dd->othc_ir_state = false;
		pm8058_headset_switch(dd->othc_ipd,
				SW_HEADPHONE_INSERT, 0);
	}
}

/*
 * The pm8058_nc_ir detects insert / remove of the headset (for NO and NC),
 * as well as button press / release (for NC type).
 * The current state of the headset is maintained in othc_ir_state variable.
 */
static irqreturn_t pm8058_nc_ir(int irq, void *dev_id)
{
	struct pm8058_othc *dd = dev_id;
	struct othc_hsed_config *hsed_config = dd->othc_pdata->hsed_config;

	if (hsed_config->othc_headset == OTHC_HEADSET_NC)
		othc_process_nc(dd);
	else {
		/* disable irq, this gets enabled in the workqueue */
		disable_irq_nosync(dd->othc_irq_ir);
		/* Processing for NO type headset */
		if (dd->othc_ir_state == false) {
			/*  headset jack inserted */
			dd->othc_ir_state = true;
			pm8058_headset_switch(dd->othc_ipd,
					SW_HEADPHONE_INSERT, 1);
		} else {
			/* headset jack removed */
			dd->othc_ir_state = false;
			pm8058_headset_switch(dd->othc_ipd,
					SW_HEADPHONE_INSERT, 0);
		}
	}
	input_sync(dd->othc_ipd);

	return IRQ_HANDLED;
}

static int pm8058_configure_othc(struct pm8058_othc *dd)
{
	int rc;
	u8 reg, value;
	u32 value1;
	u16 base_addr = dd->othc_base;
	struct othc_hsed_config *hsed_config = dd->othc_pdata->hsed_config;

	/* Intialize the OTHC module */
	/* Control Register 1*/
	rc = pm8058_read(dd->pm_chip, base_addr, &reg, 1);
	if (rc < 0) {
		pr_err("%s: PM8058 read failed \n", __func__);
		return rc;
	}

	if (hsed_config->othc_headset == OTHC_HEADSET_NO) {
		/* set iDAC high current threshold */
		value = (hsed_config->othc_highcurr_thresh_uA / 100) - 2;
		reg =  (reg & PM8058_OTHC_HIGH_CURR_MASK) | value;
	} else {
		/* set iDAC low current threshold */
		value = (hsed_config->othc_lowcurr_thresh_uA / 10) - 1;
		reg &= PM8058_OTHC_LOW_CURR_MASK;
		reg |= (value << PM8058_OTHC_LOW_CURR_SHIFT);
	}

	rc = pm8058_write(dd->pm_chip, base_addr, &reg, 1);
	if (rc < 0) {
		pr_err("%s: PM8058 read failed \n", __func__);
		return rc;
	}

	/* Control register 2*/
	rc = pm8058_read(dd->pm_chip, base_addr + 1, &reg, 1);
	if (rc < 0) {
		pr_err("%s: PM8058 read failed \n", __func__);
		return rc;
	}

	value = dd->othc_pdata->micbias_enable;
	reg &= PM8058_OTHC_EN_SIG_MASK;
	reg |= (value << PM8058_OTHC_EN_SIG_SHIFT);

	value = 0;
	value1 = (hsed_config->othc_hyst_prediv_us << 10) / USEC_PER_SEC;
	while (value1 != 0) {
		value1 = value1 >> 1;
		value++;
	}
	if (value > 7) {
		pr_err("%s: Invalid input argument - othc_hyst_prediv_us \n",
								__func__);
		return -EINVAL;
	}
	reg &= PM8058_OTHC_HYST_PREDIV_MASK;
	reg |= (value << PM8058_OTHC_HYST_PREDIV_SHIFT);

	value = 0;
	value1 = (hsed_config->othc_period_clkdiv_us << 10) / USEC_PER_SEC;
	while (value1 != 1) {
		value1 = value1 >> 1;
		value++;
	}
	if (value > 8) {
		pr_err("%s: Invalid input argument - othc_period_clkdiv_us \n",
								__func__);
		return -EINVAL;
	}
	reg = (reg &  PM8058_OTHC_CLK_PREDIV_MASK) | (value - 1);

	rc = pm8058_write(dd->pm_chip, base_addr + 1, &reg, 1);
	if (rc < 0) {
		pr_err("%s: PM8058 read failed \n", __func__);
		return rc;
	}

	/* Control register 3 */
	rc = pm8058_read(dd->pm_chip, base_addr + 2 , &reg, 1);
	if (rc < 0) {
		pr_err("%s: PM8058 read failed \n", __func__);
		return rc;
	}

	value = hsed_config->othc_hyst_clk_us /
					hsed_config->othc_hyst_prediv_us;
	if (value > 15) {
		pr_err("%s: Invalid input argument - othc_hyst_prediv_us \n",
								__func__);
		return -EINVAL;
	}
	reg &= PM8058_OTHC_HYST_CLK_MASK;
	reg |= value << PM8058_OTHC_HYST_CLK_SHIFT;

	value = hsed_config->othc_period_clk_us /
					hsed_config->othc_period_clkdiv_us;
	if (value > 15) {
		pr_err("%s: Invalid input argument - othc_hyst_prediv_us \n",
								__func__);
		return -EINVAL;
	}
	reg = (reg & PM8058_OTHC_PERIOD_CLK_MASK) | value;

	rc = pm8058_write(dd->pm_chip, base_addr + 2, &reg, 1);
	if (rc < 0) {
		pr_err("%s: PM8058 read failed \n", __func__);
		return rc;
	}

	return 0;
}

static ssize_t othc_headset_print_name(struct switch_dev *sdev, char *buf)
{
	switch (switch_get_state(sdev)) {
	case NO_DEVICE:
		return sprintf(buf, "No Device\n");
	case MSM_HEADSET:
		return sprintf(buf, "Headset\n");
	}
	return -EINVAL;
}

static int
othc_configure_hsed(struct pm8058_othc *dd, struct platform_device *pd)
{
	int rc;
	struct input_dev *ipd;
	struct pmic8058_othc_config_pdata *pdata = pd->dev.platform_data;
	struct othc_hsed_config *hsed_config = pdata->hsed_config;

	dd->othc_sdev.name = "h2w";
	dd->othc_sdev.print_name = othc_headset_print_name;

	rc = switch_dev_register(&dd->othc_sdev);
	if (rc) {
		pr_err("%s: Unable to register switch device \n", __func__);
		return rc;
	}

	ipd = input_allocate_device();
	if (ipd == NULL) {
		pr_err("%s: Unable to allocate memory \n", __func__);
		rc = -ENOMEM;
		goto fail_input_alloc;
	}

	/* Get the IRQ for Headset Insert-remove and Switch-press */
	dd->othc_irq_sw = platform_get_irq(pd, 0);
	dd->othc_irq_ir = platform_get_irq(pd, 1);
	if (dd->othc_irq_ir < 0) {
		pr_err("%s: othc resource:IRQ_IR absent \n", __func__);
		rc = -ENXIO;
		goto fail_othc_config;
	}
	if (hsed_config->othc_headset == OTHC_HEADSET_NO) {
		if (dd->othc_irq_sw < 0) {
			pr_err("%s: othc resource:IRQ_SW absent\n", __func__);
			rc = -ENXIO;
			goto fail_othc_config;
		}
	}

	ipd->name = "pmic8058_othc";
	ipd->phys = "pmic8058_othc/input0";
	ipd->dev.parent = &pd->dev;

	input_set_capability(ipd, EV_SW, SW_HEADPHONE_INSERT);
	input_set_capability(ipd, EV_KEY, KEY_MEDIA);

	input_set_drvdata(ipd, dd);

	dd->othc_ipd = ipd;
	dd->othc_sw_state = false;
	dd->othc_ir_state = false;

	if (hsed_config->othc_headset == OTHC_HEADSET_NC) {
		/* Check if NC specific pdata is present */
		if (!hsed_config->othc_nc_gpio_setup ||
					!hsed_config->othc_nc_gpio) {
			pr_err("%s: NC headset pdata missing \n", __func__);
			rc = -EINVAL;
			goto fail_othc_config;
		}
	}

	rc = pm8058_configure_othc(dd);
	if (rc < 0)
		goto fail_othc_config;

	rc = input_register_device(ipd);
	if (rc) {
		pr_err("%s: Unable to register OTHC device \n", __func__);
		goto fail_othc_config;
	}

	/* Check if the headset is already inserted during boot up */
	rc = pm8058_irq_get_rt_status(dd->pm_chip, dd->othc_irq_ir);
	if (rc < 0) {
		pr_err("%s: Unable to get headset status at boot\n", __func__);
		goto fail_ir_irq;
	}
	if (rc) {
		pr_debug("%s: Headset inserted during boot up\n", __func__);
		/* Headset present */
		dd->othc_ir_state = true;
		switch_set_state(&dd->othc_sdev, 1);
		input_report_switch(dd->othc_ipd, SW_HEADPHONE_INSERT, 1);
		input_sync(dd->othc_ipd);
	}

	rc = request_irq(dd->othc_irq_ir, pm8058_nc_ir,
				IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
				"pm8058_othc_ir", dd);
	if (rc < 0) {
		pr_err("%s: Unable to request pm8058_othc_ir IRQ\n", __func__);
		goto fail_ir_irq;
	}

	if (hsed_config->othc_headset == OTHC_HEADSET_NO) {
		/* This irq is used only for NO type headset */
		rc = request_irq(dd->othc_irq_sw, pm8058_no_sw,
			IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
				"pm8058_othc_sw", dd);
		if (rc < 0) {
			pr_err("%s: Unable to request pm8058_othc_sw IRQ \n",
								__func__);
			goto fail_sw_irq;
		}
	} else {
		/* NC type of headset use GPIO for IR */
		rc = hsed_config->othc_nc_gpio_setup();
		if (rc < 0) {
			pr_err("%s: Unable to setup gpio for NC type \n",
								__func__);
			goto fail_sw_irq;
		}
	}

	device_init_wakeup(&pd->dev, hsed_config->othc_wakeup);

	INIT_WORK(&dd->switch_work, switch_work_f);

	return 0;

fail_sw_irq:
	free_irq(dd->othc_irq_ir, dd);
fail_ir_irq:
	input_unregister_device(ipd);
	dd->othc_ipd = NULL;
fail_othc_config:
	input_free_device(ipd);
fail_input_alloc:
	switch_dev_unregister(&dd->othc_sdev);
	return rc;
}

static int __devinit pm8058_othc_probe(struct platform_device *pd)
{
	int rc;
	struct pm8058_othc *dd;
	struct pm8058_chip *chip;
	struct resource *res;
	struct pmic8058_othc_config_pdata *pdata = pd->dev.platform_data;

	chip = platform_get_drvdata(pd);
	if (chip == NULL) {
		pr_err("%s: Invalid driver information \n", __func__);
		return  -EINVAL;
	}

	/* Check PMIC8058 version. A0 version is not supported */
	if (pm8058_rev(chip) == PM_8058_REV_1p0) {
		pr_err("%s: PMIC8058 version not supported \n", __func__);
		return -ENODEV;
	}

	if (pdata == NULL) {
		pr_err("%s: Platform data not present \n", __func__);
		return -EINVAL;
	}

	dd = kzalloc(sizeof(*dd), GFP_KERNEL);
	if (dd == NULL) {
		pr_err("%s: Unable to allocate memory \n", __func__);
		return -ENOMEM;
	}

	res = platform_get_resource_byname(pd, IORESOURCE_IO, "othc_base");
	if (res == NULL) {
		pr_err("%s: othc resource:Base address absent \n", __func__);
		rc = -ENXIO;
		goto fail_get_res;
	}

	dd->othc_pdata = pdata;
	dd->pm_chip = chip;
	dd->othc_base = res->start;

	if (pdata->micbias_capability == OTHC_MICBIAS_HSED) {
		/* HSED to be supported on this MICBIAS line */
		if (pdata->hsed_config != NULL) {
			rc = othc_configure_hsed(dd, pd);
			if (rc < 0)
				goto fail_get_res;
		} else {
			pr_err("%s: HSED config data not present\n", __func__);
			rc = -EINVAL;
			goto fail_get_res;
		}
	}

	/* Store the local driver data structure */
	if (dd->othc_pdata->micbias_select < OTHC_MICBIAS_MAX)
		config[dd->othc_pdata->micbias_select] = dd;

	platform_set_drvdata(pd, dd);

	pr_debug("%s: Device %s:%d successfully registered\n",
				__func__, pd->name, pd->id);

	return 0;

fail_get_res:
	kfree(dd);
	return rc;
}

static struct platform_driver pm8058_othc_driver = {
	.driver = {
		.name = "pm8058-othc",
		.owner = THIS_MODULE,
#ifdef CONFIG_PM
		.pm = &pm8058_othc_pm_ops;
#endif
	},
	.probe = pm8058_othc_probe,
	.remove = __devexit_p(pm8058_othc_remove),
};

static int __init pm8058_othc_init(void)
{
	return platform_driver_register(&pm8058_othc_driver);
}

static void __exit pm8058_othc_exit(void)
{
	platform_driver_unregister(&pm8058_othc_driver);
}

module_init(pm8058_othc_init);
module_exit(pm8058_othc_exit);

MODULE_ALIAS("platform:pmic8058_othc");
MODULE_DESCRIPTION("PMIC 8058 OTHC");
MODULE_LICENSE("GPL v2");
