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
#include <linux/bitmap.h>
#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/spinlock.h>
#include <mach/msm_iomap.h>
#include "tlmm-msm8660.h"

/**
 * struct msm_gpio_dev: the MSM8660 SoC GPIO device structure
 *
 * @enabled_irqs: a bitmap used to optimize the summary-irq handler.  By
 * keeping track of which gpios are unmasked as irq sources, we avoid
 * having to do readl calls on hundreds of iomapped registers each time
 * the summary interrupt fires in order to locate the active interrupts.
 */
struct msm_gpio_dev {
	struct gpio_chip gpio_chip;
	spinlock_t       lock;
	DECLARE_BITMAP(enabled_irqs, NR_MSM_GPIOS);
};

static inline struct msm_gpio_dev *to_msm_gpio_dev(struct gpio_chip *chip)
{
	return container_of(chip, struct msm_gpio_dev, gpio_chip);
}

static inline void set_gpio_bits(unsigned n, void __iomem *reg)
{
	writel(readl(reg) | n, reg);
}

static inline void clr_gpio_bits(unsigned n, void __iomem *reg)
{
	writel(readl(reg) & ~n, reg);
}

static int msm_gpio_get(struct gpio_chip *chip, unsigned offset)
{
	return readl(GPIO_IN_OUT(offset)) & BIT(GPIO_IN_BIT);
}

static void msm_gpio_set(struct gpio_chip *chip, unsigned offset, int val)
{
	writel(val ? BIT(GPIO_OUT_BIT) : 0, GPIO_IN_OUT(offset));
}

static int msm_gpio_direction_input(struct gpio_chip *chip, unsigned offset)
{
	struct msm_gpio_dev *dev = to_msm_gpio_dev(chip);
	unsigned long irq_flags;

	spin_lock_irqsave(&dev->lock, irq_flags);
	clr_gpio_bits(BIT(GPIO_OE_BIT), GPIO_CONFIG(offset));
	spin_unlock_irqrestore(&dev->lock, irq_flags);
	return 0;
}

static int msm_gpio_direction_output(struct gpio_chip *chip,
				unsigned offset,
				int val)
{
	struct msm_gpio_dev *dev = to_msm_gpio_dev(chip);
	unsigned long irq_flags;

	spin_lock_irqsave(&dev->lock, irq_flags);
	msm_gpio_set(chip, offset, val);
	set_gpio_bits(BIT(GPIO_OE_BIT), GPIO_CONFIG(offset));
	spin_unlock_irqrestore(&dev->lock, irq_flags);
	return 0;
}

static int msm_gpio_to_irq(struct gpio_chip *chip, unsigned offset)
{
	return MSM_GPIO_TO_INT(offset - chip->base);
}

static inline int msm_irq_to_gpio(struct gpio_chip *chip, unsigned irq)
{
	return irq - MSM_GPIO_TO_INT(chip->base);
}

static struct msm_gpio_dev msm_gpio = {
	.gpio_chip = {
		.base             = 0,
		.ngpio            = NR_MSM_GPIOS,
		.direction_input  = msm_gpio_direction_input,
		.direction_output = msm_gpio_direction_output,
		.get              = msm_gpio_get,
		.set              = msm_gpio_set,
		.to_irq           = msm_gpio_to_irq,
	},
};

static void msm_gpio_irq_ack(unsigned int irq)
{
	writel(BIT(INTR_STATUS_BIT),
	       GPIO_INTR_STATUS(msm_irq_to_gpio(&msm_gpio.gpio_chip, irq)));
}

static void msm_gpio_irq_mask(unsigned int irq)
{
	int gpio = msm_irq_to_gpio(&msm_gpio.gpio_chip, irq);
	unsigned long irq_flags;

	spin_lock_irqsave(&msm_gpio.lock, irq_flags);
	writel(TARGET_PROC_NONE, GPIO_INTR_CFG_SU(gpio));
	clr_gpio_bits(INTR_RAW_STATUS_EN | INTR_ENABLE, GPIO_INTR_CFG(gpio));
	__clear_bit(gpio, msm_gpio.enabled_irqs);
	spin_unlock_irqrestore(&msm_gpio.lock, irq_flags);
}

static void msm_gpio_irq_unmask(unsigned int irq)
{
	int gpio = msm_irq_to_gpio(&msm_gpio.gpio_chip, irq);
	unsigned long irq_flags;

	spin_lock_irqsave(&msm_gpio.lock, irq_flags);
	__set_bit(gpio, msm_gpio.enabled_irqs);
	set_gpio_bits(INTR_RAW_STATUS_EN | INTR_ENABLE, GPIO_INTR_CFG(gpio));
	writel(TARGET_PROC_SCORPION, GPIO_INTR_CFG_SU(gpio));
	spin_unlock_irqrestore(&msm_gpio.lock, irq_flags);
}

static int msm_gpio_irq_set_type(unsigned int irq, unsigned int flow_type)
{
	void *addr = GPIO_INTR_CFG(msm_irq_to_gpio(&msm_gpio.gpio_chip, irq));
	unsigned long irq_flags;
	uint32_t bits;

	if ((flow_type & IRQ_TYPE_EDGE_BOTH) == IRQ_TYPE_EDGE_BOTH)
		return -EINVAL;

	spin_lock_irqsave(&msm_gpio.lock, irq_flags);

	bits = readl(addr);

	if (flow_type & IRQ_TYPE_EDGE_BOTH) {
		bits |= INTR_DECT_CTL_EDGE;
		irq_desc[irq].handle_irq = handle_edge_irq;
	} else {
		bits &= ~INTR_DECT_CTL_EDGE;
		irq_desc[irq].handle_irq = handle_level_irq;
	}

	if (flow_type & (IRQ_TYPE_EDGE_RISING | IRQ_TYPE_LEVEL_HIGH))
		bits |= INTR_POL_CTL_HI;
	else
		bits &= ~INTR_POL_CTL_HI;

	writel(bits, addr);

	spin_unlock_irqrestore(&msm_gpio.lock, irq_flags);

	return 0;
}

/*
 * When the summary IRQ is raised, any number of GPIO lines may be high.
 * It is the job of the summary handler to find all those GPIO lines
 * which have been set as summary IRQ lines and which are triggered,
 * and to call their interrupt handlers.
 */
static void msm_summary_irq_handler(unsigned int irq, struct irq_desc *desc)
{
	unsigned long i;

	for (i = find_first_bit(msm_gpio.enabled_irqs, NR_MSM_GPIOS);
	     i < NR_MSM_GPIOS;
	     i = find_next_bit(msm_gpio.enabled_irqs, NR_MSM_GPIOS, i + 1)) {
		if (readl(GPIO_INTR_STATUS(i)) & BIT(INTR_STATUS_BIT))
			generic_handle_irq(msm_gpio_to_irq(&msm_gpio.gpio_chip,
							   i));
	}
	desc->chip->ack(irq);
}

static struct irq_chip msm_gpio_irq_chip = {
	.name		= "msm_gpio",
	.mask		= msm_gpio_irq_mask,
	.unmask		= msm_gpio_irq_unmask,
	.ack		= msm_gpio_irq_ack,
	.set_type	= msm_gpio_irq_set_type,
};

static int __devinit msm_gpio_probe(struct platform_device *dev)
{
	int i, irq, ret;

	spin_lock_init(&msm_gpio.lock);
	bitmap_zero(msm_gpio.enabled_irqs, NR_MSM_GPIOS);
	msm_gpio.gpio_chip.label = dev->name;
	ret = gpiochip_add(&msm_gpio.gpio_chip);
	if (ret < 0)
		return ret;

	for (i = 0; i < msm_gpio.gpio_chip.ngpio; ++i) {
		irq = msm_gpio_to_irq(&msm_gpio.gpio_chip, i);
		set_irq_chip(irq, &msm_gpio_irq_chip);
		set_irq_handler(irq, handle_level_irq);
		set_irq_flags(irq, IRQF_VALID);
	}

	set_irq_chained_handler(TLMM_SCSS_SUMMARY_IRQ,
				msm_summary_irq_handler);
	return 0;
}

static int __devexit msm_gpio_remove(struct platform_device *dev)
{
	int ret = gpiochip_remove(&msm_gpio.gpio_chip);

	if (ret < 0)
		return ret;

	set_irq_handler(TLMM_SCSS_SUMMARY_IRQ, NULL);
	return 0;
}

static struct platform_driver msm_gpio_driver = {
	.probe = msm_gpio_probe,
	.remove = __devexit_p(msm_gpio_remove),
	.driver = {
		.name = "msm8660-gpio",
		.owner = THIS_MODULE,
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

MODULE_AUTHOR("Gregory Bean <gbean@codeaurora.org>");
MODULE_DESCRIPTION("Driver for Qualcomm MSM 8660-family SoC GPIOs");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:msm8660-gpio");
