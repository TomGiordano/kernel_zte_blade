/* Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
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
/*
 * Bluetooth Power Switch Module
 * controls power to external Bluetooth device
 * with interface to power management device
 */

 /*-----------------------------------------------------------------------------------------
  when         who              what, where, why                        comment tag
  ----------   -------------    -------------------------------------   -------------------                             
2011-01-26     xumei    modified according to bluetooth-power of kernel 2.6.35     ZTE_BT_QXX_20101207
2010-12-07     qxx      compatible of qualcomm and broadcomm bluetooth chip        ZTE_BT_QXX_20101207  
------------------------------------------------------------------------------------------*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/rfkill.h>

static bool previous;
static int bcm_toggle_radio(void *data, bool blocked)
{
	int ret = 0;
	int (*power_control)(int enable);

	power_control = data;
	if (previous != blocked)
		ret = (*power_control)(!blocked);
	previous = blocked;
	return ret;
}

static const struct rfkill_ops bcm_power_rfkill_ops = {
	.set_block = bcm_toggle_radio,
};

static int bcm_power_rfkill_probe(struct platform_device *pdev)
{
	struct rfkill *rfkill;
	int ret;

	rfkill = rfkill_alloc("bcm_power", &pdev->dev, RFKILL_TYPE_BLUETOOTH,
			      &bcm_power_rfkill_ops,
			      pdev->dev.platform_data);

	if (!rfkill) {
		printk(KERN_DEBUG
			"%s: rfkill allocate failed\n", __func__);
		return -ENOMEM;
	}

	/* force Bluetooth off during init to allow for user control */
	rfkill_init_sw_state(rfkill, 1);
	previous = 1;

	ret = rfkill_register(rfkill);
	if (ret) {
		printk(KERN_DEBUG
			"%s: rfkill register failed=%d\n", __func__,
			ret);
		rfkill_destroy(rfkill);
		return ret;
	}

	platform_set_drvdata(pdev, rfkill);

	return 0;
}

static void bcm_power_rfkill_remove(struct platform_device *pdev)
{
	struct rfkill *rfkill;

	dev_dbg(&pdev->dev, "%s\n", __func__);
	rfkill = platform_get_drvdata(pdev);
	if (rfkill)
		rfkill_unregister(rfkill);
	rfkill_destroy(rfkill);
	platform_set_drvdata(pdev, NULL);
}

static int __devinit bcm_power_probe(struct platform_device *pdev)
{
	int ret = 0;

	printk(KERN_DEBUG "%s\n", __func__);

	if (!pdev->dev.platform_data) {
		printk(KERN_ERR "%s: platform data not initialized\n",
				__func__);
		return -ENOSYS;
	}

	ret = bcm_power_rfkill_probe(pdev);

	return ret;
}

static int __devexit bcm_power_remove(struct platform_device *pdev)
{
	printk(KERN_DEBUG "%s\n", __func__);

	bcm_power_rfkill_remove(pdev);

	return 0;
}

static struct platform_driver bcm_power_driver = {
	.probe = bcm_power_probe,
	.remove = __devexit_p(bcm_power_remove),
	.driver = {
		.name = "bcm_power",
		.owner = THIS_MODULE,
	},
};

static int __init bcm_power_init(void)
{
	int ret;

	printk(KERN_DEBUG "%s\n", __func__);
	ret = platform_driver_register(&bcm_power_driver);
	return ret;
}

static void __exit bcm_power_exit(void)
{
	printk(KERN_DEBUG "%s\n", __func__);
	platform_driver_unregister(&bcm_power_driver);
}

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MSM Bluetooth power control driver");
MODULE_VERSION("1.40");

module_init(bcm_power_init);
module_exit(bcm_power_exit);
