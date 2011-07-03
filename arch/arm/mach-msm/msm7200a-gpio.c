/*
 * Driver for Qualcomm MSM7200a and related SoC GPIO.
 * Supported chipset families include:
 * MSM7x01(a), MSM7x25, MSM7x27, MSM7x30, QSD8x50(a)
 *
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2010, Code Aurora Forum. All rights reserved.
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
#include <linux/kernel.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include "msm7200a-gpio.h"

/*
 * The INT_STATUS register latches both edge- and level-detection events,
 * which is atypical.  Turning on DONT_LATCH_LEVEL_IRQS causes level irq
 * triggers to be forgotten across mask/unmask calls, emulating a more
 * traditional setup.
 */
#define MSM_GPIO_DONT_LATCH_LEVEL_IRQS 1

enum {
	IRQ_MASK_NORMAL = 0,
	IRQ_MASK_WAKE_ON,
	NUM_IRQ_MASKS
};

struct msm_gpio_dev {
	struct gpio_chip		gpio_chip;
	spinlock_t			lock;
	unsigned			irq_base;
	unsigned			irq_summary;
	struct msm7200a_gpio_regs	regs;
	u32				irq_masks[NUM_IRQ_MASKS];
	int				nsuspend;
};

#define TO_MSM_GPIO_DEV(c) container_of(c, struct msm_gpio_dev, gpio_chip)

static inline unsigned bit(unsigned offset)
{
	BUG_ON(offset >= sizeof(unsigned) * 8);
	return 1U << offset;
}

/*
 * This function assumes that msm_gpio_dev::lock is held.
 */
static inline void set_gpio_bit(unsigned n, void __iomem *reg)
{
	writel(readl(reg) | bit(n), reg);
}

/*
 * This function assumes that msm_gpio_dev::lock is held.
 */
static inline void clr_gpio_bit(unsigned n, void __iomem *reg)
{
	writel(readl(reg) & ~bit(n), reg);
}

/*
 * This function assumes that msm_gpio_dev::lock is held.
 */
static inline void
msm_gpio_write(struct msm_gpio_dev *dev, unsigned n, unsigned on)
{
	if (on)
		set_gpio_bit(n, dev->regs.out);
	else
		clr_gpio_bit(n, dev->regs.out);
}

static int gpio_chip_direction_input(struct gpio_chip *chip, unsigned offset)
{
	struct msm_gpio_dev *msm_gpio = TO_MSM_GPIO_DEV(chip);
	unsigned long irq_flags;

	spin_lock_irqsave(&msm_gpio->lock, irq_flags);
	clr_gpio_bit(offset, msm_gpio->regs.oe);
	spin_unlock_irqrestore(&msm_gpio->lock, irq_flags);

	return 0;
}

static int
gpio_chip_direction_output(struct gpio_chip *chip, unsigned offset, int value)
{
	struct msm_gpio_dev *msm_gpio = TO_MSM_GPIO_DEV(chip);
	unsigned long irq_flags;

	spin_lock_irqsave(&msm_gpio->lock, irq_flags);

	msm_gpio_write(msm_gpio, offset, value);
	set_gpio_bit(offset, msm_gpio->regs.oe);
	spin_unlock_irqrestore(&msm_gpio->lock, irq_flags);

	return 0;
}

static int gpio_chip_get(struct gpio_chip *chip, unsigned offset)
{
	struct msm_gpio_dev *msm_gpio = TO_MSM_GPIO_DEV(chip);
	unsigned long irq_flags;
	int ret;

	spin_lock_irqsave(&msm_gpio->lock, irq_flags);
	ret = readl(msm_gpio->regs.in) & bit(offset) ? 1 : 0;
	spin_unlock_irqrestore(&msm_gpio->lock, irq_flags);

	return ret;
}

static void gpio_chip_set(struct gpio_chip *chip, unsigned offset, int value)
{
	struct msm_gpio_dev *msm_gpio = TO_MSM_GPIO_DEV(chip);
	unsigned long irq_flags;

	spin_lock_irqsave(&msm_gpio->lock, irq_flags);
	msm_gpio_write(msm_gpio, offset, value);
	spin_unlock_irqrestore(&msm_gpio->lock, irq_flags);
}

static int gpio_chip_to_irq(struct gpio_chip *chip, unsigned offset)
{
	struct msm_gpio_dev *msm_gpio = TO_MSM_GPIO_DEV(chip);
	return msm_gpio->irq_base + offset;
}

#if MSM_GPIO_DONT_LATCH_LEVEL_IRQS
static inline void forget_level_irq(struct msm_gpio_dev *msm_gpio,
				unsigned offset)
{
	unsigned v = readl(msm_gpio->regs.int_edge);
	unsigned b = bit(offset);

	if (!(v & b))
		writel(b, msm_gpio->regs.int_clear);

}
#else
static inline void forget_level_irq(struct msm_gpio_dev *msm, unsigned off)
{
}
#endif

static void msm_gpio_irq_mask(unsigned int irq)
{
	unsigned long irq_flags;
	struct msm_gpio_dev *msm_gpio = get_irq_chip_data(irq);
	unsigned offset = irq - msm_gpio->irq_base;

	spin_lock_irqsave(&msm_gpio->lock, irq_flags);
	forget_level_irq(msm_gpio, offset);
	msm_gpio->irq_masks[IRQ_MASK_NORMAL] &= ~bit(offset);
	writel(msm_gpio->irq_masks[IRQ_MASK_NORMAL], msm_gpio->regs.int_en);
	spin_unlock_irqrestore(&msm_gpio->lock, irq_flags);
}

static void msm_gpio_irq_unmask(unsigned int irq)
{
	unsigned long irq_flags;
	struct msm_gpio_dev *msm_gpio = get_irq_chip_data(irq);
	unsigned offset = irq - msm_gpio->irq_base;

	spin_lock_irqsave(&msm_gpio->lock, irq_flags);
	forget_level_irq(msm_gpio, offset);
	msm_gpio->irq_masks[IRQ_MASK_NORMAL] |= bit(offset);
	writel(msm_gpio->irq_masks[IRQ_MASK_NORMAL], msm_gpio->regs.int_en);
	spin_unlock_irqrestore(&msm_gpio->lock, irq_flags);
}

static int msm_gpio_irq_set_type(unsigned int irq, unsigned int flow_type)
{
	unsigned long irq_flags;
	struct msm_gpio_dev *msm_gpio = get_irq_chip_data(irq);
	unsigned offset = irq - msm_gpio->irq_base;

	if ((flow_type & (IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING)) ==
		(IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING))
		return -ENOTSUPP;

	if ((flow_type & (IRQF_TRIGGER_HIGH | IRQF_TRIGGER_LOW)) ==
		(IRQF_TRIGGER_HIGH | IRQF_TRIGGER_LOW))
		return -ENOTSUPP;

	spin_lock_irqsave(&msm_gpio->lock, irq_flags);

	if (flow_type & (IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING)) {
		set_gpio_bit(offset, msm_gpio->regs.int_edge);
		irq_desc[irq].handle_irq = handle_edge_irq;
	} else {
		clr_gpio_bit(offset, msm_gpio->regs.int_edge);
		irq_desc[irq].handle_irq = handle_level_irq;
	}

	if (flow_type & (IRQF_TRIGGER_HIGH | IRQF_TRIGGER_RISING))
		set_gpio_bit(offset, msm_gpio->regs.int_pos);
	else
		clr_gpio_bit(offset, msm_gpio->regs.int_pos);

	spin_unlock_irqrestore(&msm_gpio->lock, irq_flags);

	return 0;
}

static void msm_gpio_irq_mask_ack(unsigned int irq)
{
	msm_gpio_irq_mask(irq);
}

static int msm_gpio_irq_set_affinity(unsigned int irq,
				const struct cpumask *dest)
{
	return -ENOTSUPP;
}

static int msm_gpio_irq_retrigger(unsigned int irq)
{
	return -ENOTSUPP;
}

static int msm_gpio_irq_set_wake(unsigned int irq, unsigned int on)
{
	unsigned long irq_flags;
	struct msm_gpio_dev *msm_gpio = get_irq_chip_data(irq);
	unsigned offset = irq - msm_gpio->irq_base;

	spin_lock_irqsave(&msm_gpio->lock, irq_flags);
	if (on)
		msm_gpio->irq_masks[IRQ_MASK_WAKE_ON] |= bit(offset);
	else
		msm_gpio->irq_masks[IRQ_MASK_WAKE_ON] &= ~bit(offset);
	spin_unlock_irqrestore(&msm_gpio->lock, irq_flags);

	return set_irq_wake(msm_gpio->irq_summary, on);
}

static irqreturn_t msm_gpio_irq_handler(int irq, void *dev)
{
	unsigned long irq_flags;
	int b, m;
	unsigned e, s, v;

	struct msm_gpio_dev *msm_gpio = (struct msm_gpio_dev *)dev;

	/*
	 * The int_status register latches trigger events whether or not
	 * the gpio line is enabled as an interrupt source.  Therefore,
	 * the set of pins which defines the interrupts which need to fire
	 * is the intersection of int_status and int_en - int_status
	 * alone provides an incomplete picture.
	 */
	spin_lock_irqsave(&msm_gpio->lock, irq_flags);
	s = readl(msm_gpio->regs.int_status);
	e = readl(msm_gpio->regs.int_en);
	v = s & e;
	if (v)
		writel(v, msm_gpio->regs.int_clear);
	spin_unlock_irqrestore(&msm_gpio->lock, irq_flags);

	if (!v)
		return IRQ_NONE;

	while (v) {
		m = v & -v;
		b = fls(m) - 1;
		v &= ~m;
		generic_handle_irq(msm_gpio->irq_base + b);
	}
	return IRQ_HANDLED;
}

static struct irq_chip msm_gpio_irq_chip = {
	.name			= "msm_gpio",
	.mask			= msm_gpio_irq_mask,
	.mask_ack		= msm_gpio_irq_mask_ack,
	.unmask			= msm_gpio_irq_unmask,
	.set_affinity		= msm_gpio_irq_set_affinity,
	.retrigger		= msm_gpio_irq_retrigger,
	.set_type		= msm_gpio_irq_set_type,
	.set_wake		= msm_gpio_irq_set_wake,
};

static int msm_gpio_probe(struct platform_device *dev)
{
	struct msm_gpio_dev *msm_gpio;
	struct msm7200a_gpio_platform_data *pdata =
		(struct msm7200a_gpio_platform_data *)dev->dev.platform_data;
	int i, irq, ret;

	if (!pdata)
		return -EINVAL;

	msm_gpio = kzalloc(sizeof(struct msm_gpio_dev), GFP_KERNEL);
	if (!msm_gpio)
		return -ENOMEM;

	spin_lock_init(&msm_gpio->lock);
	platform_set_drvdata(dev, msm_gpio);
	memcpy(&msm_gpio->regs,
	       &pdata->regs,
	       sizeof(struct msm7200a_gpio_regs));

	msm_gpio->gpio_chip.label            = dev->name;
	msm_gpio->gpio_chip.base             = pdata->gpio_base;
	msm_gpio->gpio_chip.ngpio            = pdata->ngpio;
	msm_gpio->gpio_chip.direction_input  = gpio_chip_direction_input;
	msm_gpio->gpio_chip.direction_output = gpio_chip_direction_output;
	msm_gpio->gpio_chip.get              = gpio_chip_get;
	msm_gpio->gpio_chip.set              = gpio_chip_set;
	msm_gpio->gpio_chip.to_irq           = gpio_chip_to_irq;
	msm_gpio->irq_base                   = pdata->irq_base;
	msm_gpio->irq_summary                = pdata->irq_summary;

	ret = gpiochip_add(&msm_gpio->gpio_chip);
	if (ret < 0)
		goto err_post_malloc;

	for (i = 0; i < msm_gpio->gpio_chip.ngpio; ++i) {
		irq = msm_gpio->irq_base + i;
		set_irq_chip_data(irq, msm_gpio);
		set_irq_chip(irq, &msm_gpio_irq_chip);
		set_irq_handler(irq, handle_level_irq);
		set_irq_flags(irq, IRQF_VALID);
	}

	/*
	 * We use a level-triggered interrupt because of the nature
	 * of the shared GPIO-group interrupt.
	 *
	 * Many GPIO chips may be sharing the same group IRQ line, and
	 * it is possible for GPIO interrupt to re-occur while the system
	 * is still servicing the group interrupt associated with it.
	 * The group IRQ line would not de-assert and re-assert, and
	 * we'd get no second edge to cause the group IRQ to be handled again.
	 *
	 * Using a level interrupt guarantees that the group IRQ handlers
	 * will continue to be called as long as any GPIO chip in the group
	 * is asserting, even if the condition began while the group
	 * handler was in mid-pass.
	 */
	ret = request_irq(msm_gpio->irq_summary,
			  msm_gpio_irq_handler,
			  IRQF_SHARED | IRQF_TRIGGER_HIGH,
			  dev->name,
			  msm_gpio);
	if (ret < 0)
		goto err_post_gpiochip_add;

#ifdef CONFIG_PM_RUNTIME
	ret = pm_runtime_set_active(&dev->dev);
	if (ret < 0)
		goto err_post_req_irq;
	pm_runtime_enable(&dev->dev);
#endif

	return ret;
#ifdef CONFIG_PM_RUNTIME
err_post_req_irq:
	free_irq(msm_gpio->irq_summary, msm_gpio);
#endif
err_post_gpiochip_add:
	/*
	 * Under no circumstances should a line be held on a gpiochip
	 * which hasn't finished probing.
	 */
	BUG_ON(gpiochip_remove(&msm_gpio->gpio_chip) < 0);
err_post_malloc:
	kfree(msm_gpio);
	return ret;
}

static int msm_gpio_remove(struct platform_device *dev)
{
	struct msm_gpio_dev *msm_gpio = platform_get_drvdata(dev);
	int ret;

	ret = gpiochip_remove(&msm_gpio->gpio_chip);
	if (ret < 0)
		return ret;

#ifdef CONFIG_PM_RUNTIME
	pm_runtime_disable(&dev->dev);
#endif
	free_irq(msm_gpio->irq_summary, msm_gpio);
	kfree(msm_gpio);

	return 0;
}

static int msm_gpio_suspend(struct msm_gpio_dev *msm_gpio)
{
	unsigned long irq_flags;
	unsigned long irq_mask;

	spin_lock_irqsave(&msm_gpio->lock, irq_flags);
	if ((msm_gpio->nsuspend)++ == 0) {
		irq_mask = msm_gpio->irq_masks[IRQ_MASK_NORMAL] &
			   msm_gpio->irq_masks[IRQ_MASK_WAKE_ON];
		writel(irq_mask, msm_gpio->regs.int_en);
	}
	spin_unlock_irqrestore(&msm_gpio->lock, irq_flags);

	return 0;
}

static int msm_gpio_resume(struct msm_gpio_dev *msm_gpio)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&msm_gpio->lock, irq_flags);
	if (--(msm_gpio->nsuspend) == 0)
		writel(msm_gpio->irq_masks[IRQ_MASK_NORMAL],
		       msm_gpio->regs.int_en);
	spin_unlock_irqrestore(&msm_gpio->lock, irq_flags);

	return 0;
}

static int msm_gpio_legacy_suspend(struct platform_device *dev,
				   pm_message_t state)
{
	struct msm_gpio_dev *msm_gpio = platform_get_drvdata(dev);

	return msm_gpio_suspend(msm_gpio);
}

static int msm_gpio_legacy_resume(struct platform_device *dev)
{
	struct msm_gpio_dev *msm_gpio = platform_get_drvdata(dev);

	return msm_gpio_resume(msm_gpio);
}

static int msm_gpio_dev_pm_suspend(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct msm_gpio_dev *msm_gpio = platform_get_drvdata(pdev);

	return msm_gpio_suspend(msm_gpio);
}

static int msm_gpio_dev_pm_resume(struct device *dev)
{
	struct platform_device *pdev = to_platform_device(dev);
	struct msm_gpio_dev *msm_gpio = platform_get_drvdata(pdev);

	return msm_gpio_resume(msm_gpio);
}

static SIMPLE_DEV_PM_OPS(msm_gpio_pm_ops,
			 msm_gpio_dev_pm_suspend,
			 msm_gpio_dev_pm_resume);

static struct platform_driver msm_gpio_driver = {
	.probe   = msm_gpio_probe,
	.remove  = msm_gpio_remove,
	.suspend = msm_gpio_legacy_suspend,
	.resume  = msm_gpio_legacy_resume,
	.driver  = {
		.name  = "msm7200a-gpio",
		.owner = THIS_MODULE,
		.pm    = &msm_gpio_pm_ops,
	},
};

static int __init msm_gpio_init(void)
{
	return platform_driver_register(&msm_gpio_driver);
}

static void __exit msm_gpio_exit(void)
{
	platform_driver_unregister(&msm_gpio_driver);
}

postcore_initcall(msm_gpio_init);
module_exit(msm_gpio_exit);

MODULE_DESCRIPTION("Driver for Qualcomm MSM 7200a-family SoC GPIOs");
MODULE_LICENSE("GPLv2");
