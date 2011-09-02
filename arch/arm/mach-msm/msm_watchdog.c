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
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/pm.h>
#include <linux/mfd/pmic8058.h>
#include <linux/jiffies.h>
#include <linux/suspend.h>
#include <mach/msm_iomap.h>

#define TCSR_BASE 0x16B00000
#define TCSR_WDT_CFG 0x30

#define WDT0_RST	(MSM_TMR0_BASE + 0x38)
#define WDT0_EN		(MSM_TMR0_BASE + 0x40)
#define WDT0_BARK_TIME	(MSM_TMR0_BASE + 0x4C)
#define WDT0_BITE_TIME	(MSM_TMR0_BASE + 0x5C)

/* Watchdog pet interval in ms */
#define PET_DELAY 3000
static unsigned long delay_time;

/*
 * On the kernel command line specify
 * msm_watchdog.enable=1 to enable the watchdog
 * By default watchdog is turned on
 */
static int enable = 1;
module_param(enable, int, 0);

static void *tcsr_base;
static void pet_watchdog(struct work_struct *work);
static DECLARE_DELAYED_WORK(dogwork_struct, pet_watchdog);

static int msm_watchdog_suspend(void)
{
	writel(1, WDT0_RST);
	writel(0, WDT0_EN);
	return NOTIFY_DONE;
}
static int msm_watchdog_resume(void)
{
	writel(1, WDT0_EN);
	writel(1, WDT0_RST);
	return NOTIFY_DONE;
}

static int msm_watchdog_power_event(struct notifier_block *this,
				unsigned long event, void *ptr)
{
	switch (event) {
	case PM_POST_HIBERNATION:
	case PM_POST_SUSPEND:
		return msm_watchdog_resume();
	case PM_HIBERNATION_PREPARE:
	case PM_SUSPEND_PREPARE:
		return msm_watchdog_suspend();
	default:
		return NOTIFY_DONE;
	}
}

static int panic_wdog_handler(struct notifier_block *this,
			      unsigned long event, void *ptr)
{
	if (panic_timeout == 0) {
		writel(0, WDT0_EN);
		writel(0, tcsr_base + TCSR_WDT_CFG);
	} else {
		writel(32768 * (panic_timeout + 4), WDT0_BARK_TIME);
		writel(32768 * (panic_timeout + 4), WDT0_BITE_TIME);
		writel(1, WDT0_RST);
	}
	return NOTIFY_DONE;
}

static struct notifier_block panic_blk = {
	.notifier_call	= panic_wdog_handler,
};

static struct notifier_block msm_watchdog_power_notifier = {
	.notifier_call = msm_watchdog_power_event,
};

static void pet_watchdog(struct work_struct *work)
{
	writel(1, WDT0_RST);

	if (enable)
		schedule_delayed_work(&dogwork_struct, delay_time);
}

static void __exit exit_watchdog(void)
{
	if (enable) {
		writel(0, WDT0_EN);
		unregister_pm_notifier(&msm_watchdog_power_notifier);
		writel(0, WDT0_EN); /* In case we got suspended mid-exit */
		writel(0, tcsr_base + TCSR_WDT_CFG);
		iounmap(tcsr_base);
		enable = 0;
	}
	printk(KERN_INFO "MSM Watchdog Exit - Deactivated\n");
}

static int __init init_watchdog(void)
{
	int ret;
	if (enable) {
		tcsr_base = ioremap_nocache(TCSR_BASE, SZ_4K);
		if (tcsr_base == NULL)
			return -ENOMEM;

		writel(1, tcsr_base + TCSR_WDT_CFG);
		delay_time = msecs_to_jiffies(PET_DELAY);

		/* 32768 ticks = 1 second */
		writel(32768*4, WDT0_BARK_TIME);
		writel(32768*4, WDT0_BITE_TIME);

		ret = register_pm_notifier(&msm_watchdog_power_notifier);
		if (ret)
			return ret;

		INIT_DELAYED_WORK(&dogwork_struct, pet_watchdog);
		schedule_delayed_work(&dogwork_struct, delay_time);

		atomic_notifier_chain_register(&panic_notifier_list,
					       &panic_blk);

		writel(1, WDT0_EN);
		printk(KERN_INFO "MSM Watchdog Initialized\n");
	} else {
		printk(KERN_INFO "MSM Watchdog Not Initialized\n");
	}

	return 0;
}

late_initcall(init_watchdog);
module_exit(exit_watchdog);
MODULE_DESCRIPTION("MSM Watchdog Driver");
MODULE_VERSION("1.0");
MODULE_LICENSE("GPL v2");
