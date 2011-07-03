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
 *
 */

/*
2010.11.30   cuihj_modified     remove the reset  in  msm_otg_suspend.  
2010.12.17   ruanmeisi           schedule a resume work in otg_irq.
*/
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/ioport.h>

#include <linux/device.h>
#include <mach/msm_hsusb_hw.h>
#include <mach/msm72k_otg.h>
#include <mach/msm_hsusb.h>
#include <linux/debugfs.h>
#include <linux/uaccess.h>
#include <mach/clk.h>

#define MSM_USB_BASE	(dev->regs)
#define is_host()	((OTGSC_ID & readl(USB_OTGSC)) ? 0 : 1)
#define is_b_sess_vld()	((OTGSC_BSV & readl(USB_OTGSC)) ? 1 : 0)
#define USB_LINK_RESET_TIMEOUT	(msecs_to_jiffies(10))
#define DRIVER_NAME	"msm_otg"

static void otg_reset(struct otg_transceiver *xceiv);
static void msm_otg_set_vbus_state(int online);
//ruanmeisi_20101217 work for resume
static void schedule_otg_work(struct msm_otg *dev);
static void init_otg_work(void);
//end

struct msm_otg *the_msm_otg;

static unsigned ulpi_read(struct msm_otg *dev, unsigned reg)
{
	unsigned timeout = 100000;

	/* initiate read operation */
	writel(ULPI_RUN | ULPI_READ | ULPI_ADDR(reg),
	       USB_ULPI_VIEWPORT);

	/* wait for completion */
	while ((readl(USB_ULPI_VIEWPORT) & ULPI_RUN) && (--timeout))
		cpu_relax();

	if (timeout == 0) {
		printk(KERN_ERR "ulpi_read: timeout %08x\n",
			readl(USB_ULPI_VIEWPORT));
		return 0xffffffff;
	}
	return ULPI_DATA_READ(readl(USB_ULPI_VIEWPORT));
}

static int ulpi_write(struct msm_otg *dev, unsigned val, unsigned reg)
{
	unsigned timeout = 10000;

	/* initiate write operation */
	writel(ULPI_RUN | ULPI_WRITE |
	       ULPI_ADDR(reg) | ULPI_DATA(val),
	       USB_ULPI_VIEWPORT);

	/* wait for completion */
	while ((readl(USB_ULPI_VIEWPORT) & ULPI_RUN) && (--timeout))
		;

	if (timeout == 0) {
		printk(KERN_ERR "ulpi_write: timeout\n");
		return -1;
	}

	return 0;
}

static void enable_idgnd(struct msm_otg *dev)
{
	ulpi_write(dev, (1<<4), 0x0E);
	ulpi_write(dev, (1<<4), 0x11);
	writel(readl(USB_OTGSC) | OTGSC_IDIE, USB_OTGSC);
}

static void disable_idgnd(struct msm_otg *dev)
{
	ulpi_write(dev, (1<<4), 0x0F);
	ulpi_write(dev, (1<<4), 0x12);
	writel(readl(USB_OTGSC) & ~OTGSC_IDIE, USB_OTGSC);
}

static void enable_sess_valid(struct msm_otg *dev)
{
	ulpi_write(dev, (1<<2), 0x0E);
	ulpi_write(dev, (1<<2), 0x11);
	writel(readl(USB_OTGSC) | OTGSC_BSVIE, USB_OTGSC);
}

static void disable_sess_valid(struct msm_otg *dev)
{
	ulpi_write(dev, (1<<2), 0x0F);
	ulpi_write(dev, (1<<2), 0x12);
	writel(readl(USB_OTGSC) & ~OTGSC_BSVIE, USB_OTGSC);
}

static inline void set_pre_emphasis_level(struct msm_otg *dev)
{
	unsigned res = 0;

	if (!dev->pdata || dev->pdata->pemp_level == PRE_EMPHASIS_DEFAULT)
		return;

	res = ulpi_read(dev, ULPI_CONFIG_REG3);
	res &= ~(ULPI_PRE_EMPHASIS_MASK);
	if (dev->pdata->pemp_level != PRE_EMPHASIS_DISABLE)
		res |= dev->pdata->pemp_level;
	ulpi_write(dev, res, ULPI_CONFIG_REG3);
}

static inline void set_cdr_auto_reset(struct msm_otg *dev)
{
	unsigned res = 0;

	if (!dev->pdata || dev->pdata->cdr_autoreset == CDR_AUTO_RESET_DEFAULT)
		return;

	res = ulpi_read(dev, ULPI_DIGOUT_CTRL);
	if (dev->pdata->cdr_autoreset == CDR_AUTO_RESET_ENABLE)
		res &=  ~ULPI_CDR_AUTORESET;
	else
		res |=  ULPI_CDR_AUTORESET;
	ulpi_write(dev, res, ULPI_DIGOUT_CTRL);
}

static inline void set_driver_amplitude(struct msm_otg *dev)
{
	unsigned res = 0;

	if (!dev->pdata || dev->pdata->drv_ampl == HS_DRV_AMPLITUDE_DEFAULT)
		return;

	res = ulpi_read(dev, ULPI_CONFIG_REG2);
	res &= ~ULPI_DRV_AMPL_MASK;
	if (dev->pdata->drv_ampl != HS_DRV_AMPLITUDE_ZERO_PERCENT)
		res |= dev->pdata->drv_ampl;
	ulpi_write(dev, res, ULPI_CONFIG_REG2);
}

static int msm_otg_set_clk(struct otg_transceiver *xceiv, int on)
{
	struct msm_otg *dev = container_of(xceiv, struct msm_otg, otg);

	if (!dev || (dev != the_msm_otg))
		return -ENODEV;

	if (on)
		/* enable clocks */
		clk_enable(dev->hs_clk);
	else
		clk_disable(dev->hs_clk);

	return 0;
}
static void msm_otg_start_peripheral(struct otg_transceiver *xceiv, int on)
{
	if (!xceiv->gadget)
		return;

	if (on)
		usb_gadget_vbus_connect(xceiv->gadget);
	else
		usb_gadget_vbus_disconnect(xceiv->gadget);
}

static void msm_otg_start_host(struct otg_transceiver *xceiv, int on)
{
	struct msm_otg *dev = container_of(xceiv, struct msm_otg, otg);

	if (!xceiv->host)
		return;

	if (dev->start_host)
		dev->start_host(xceiv->host, on);
}

static int msm_otg_suspend(struct msm_otg *dev)
{
	unsigned long timeout;
	int vbus = 0;
	unsigned otgsc;

	disable_irq(dev->irq);
	if (atomic_read(&dev->in_lpm))
		goto out;

	/* Don't reset if mini-A cable is connected */
	/* deleted by cuihj for  the ac charger is identified as USB    cuihj_modified  */
	#if  1
	if (!is_host())
		otg_reset(&dev->otg);
   #endif 
	/* In case of fast plug-in and plug-out inside the otg_reset() the
	 * servicing of BSV is missed (in the window of after phy and link
	 * reset). Handle it if any missing bsv is detected.
	 * Ignore BSV, as it may remain set while using debugfs to change modes
	 */
	if (is_b_sess_vld() && !is_host() &&
				(dev->pdata->otg_mode == OTG_ID)) {
		otgsc = readl(USB_OTGSC);
		writel(otgsc, USB_OTGSC);
		pr_info("%s:Process mising BSV\n", __func__);
		msm_otg_start_peripheral(&dev->otg, 1);
		enable_irq(dev->irq);
		return -1;
	}

	ulpi_read(dev, 0x14);/* clear PHY interrupt latch register */
	/* If there is no pmic notify support turn on phy comparators. */
	if (!dev->pmic_notif_supp)
		ulpi_write(dev, 0x01, 0x30);
	ulpi_write(dev, 0x08, 0x09);/* turn off PLL on integrated phy */

	timeout = jiffies + msecs_to_jiffies(500);
	disable_phy_clk();
	while (!is_phy_clk_disabled()) {
		if (time_after(jiffies, timeout)) {
			pr_err("%s: Unable to suspend phy\n", __func__);
			otg_reset(&dev->otg);
			goto out;
		}
		msleep(1);
	}

	writel(readl(USB_USBCMD) | ASYNC_INTR_CTRL | ULPI_STP_CTRL, USB_USBCMD);
	if (dev->hs_pclk)
		clk_disable(dev->hs_pclk);
	if (dev->hs_cclk)
		clk_disable(dev->hs_cclk);
	if (device_may_wakeup(dev->otg.dev)) {
		enable_irq_wake(dev->irq);
		if (dev->vbus_on_irq)
			enable_irq_wake(dev->vbus_on_irq);
	}

	atomic_set(&dev->in_lpm, 1);

	if (!vbus && dev->pmic_notif_supp)
		dev->pdata->pmic_enable_ldo(0);

	pr_info("%s: usb in low power mode\n", __func__);

out:
	enable_irq(dev->irq);

	/* TBD: as there is no bus suspend implemented as of now
	 * it should be dummy check
	 */

	return 0;
}

static int msm_otg_resume(struct msm_otg *dev)
{
	unsigned temp;

	if (!atomic_read(&dev->in_lpm))
		return 0;


	if (dev->hs_pclk)
		clk_enable(dev->hs_pclk);
	if (dev->hs_cclk)
		clk_enable(dev->hs_cclk);

	temp = readl(USB_USBCMD);
	temp &= ~ASYNC_INTR_CTRL;
	temp &= ~ULPI_STP_CTRL;
	writel(temp, USB_USBCMD);

	/* If resume signalling finishes before lpm exit, PCD is not set in
	 * USBSTS register. Drive resume signal to the downstream device now
	 * so that host driver can process the upcoming port change interrupt.*/
	if (is_host())
		writel(readl(USB_PORTSC) | PORTSC_FPR, USB_PORTSC);

	if (device_may_wakeup(dev->otg.dev)) {
		disable_irq_wake(dev->irq);
		if (dev->vbus_on_irq)
			disable_irq_wake(dev->vbus_on_irq);
	}

	atomic_set(&dev->in_lpm, 0);

	pr_info("%s: usb exited from low power mode\n", __func__);

	return 0;
}

static int msm_otg_set_suspend(struct otg_transceiver *xceiv, int suspend)
{
	struct msm_otg *dev = container_of(xceiv, struct msm_otg, otg);

	if (!dev || (dev != the_msm_otg))
		return -ENODEV;

	if (suspend == atomic_read(&dev->in_lpm))
		return 0;

	if (suspend)
		msm_otg_suspend(dev);
	else {
		unsigned long timeout;

		disable_irq(dev->irq);
		if (dev->pmic_notif_supp)
			dev->pdata->pmic_enable_ldo(1);

		msm_otg_resume(dev);

		if (!is_phy_clk_disabled())
			goto out;

		timeout = jiffies + usecs_to_jiffies(100);
		enable_phy_clk();
		while (is_phy_clk_disabled()) {
			if (time_after(jiffies, timeout)) {
				pr_err("%s: Unable to wakeup phy\n", __func__);
				otg_reset(&dev->otg);
				break;
			}
			udelay(10);
		}
out:
		enable_irq(dev->irq);

	}

	return 0;
}

static int msm_otg_set_peripheral(struct otg_transceiver *xceiv,
			struct usb_gadget *gadget)
{
	struct msm_otg *dev = container_of(xceiv, struct msm_otg, otg);

	if (!dev || (dev != the_msm_otg))
		return -ENODEV;

	if (!gadget) {
		msm_otg_start_peripheral(xceiv, 0);
		dev->otg.gadget = 0;
		disable_sess_valid(dev);
		if (dev->pmic_notif_supp && dev->pdata->pmic_unregister_vbus_sn)
			dev->pdata->pmic_unregister_vbus_sn
				(&msm_otg_set_vbus_state);
		return 0;
	}
	dev->otg.gadget = gadget;
	enable_sess_valid(dev);
	if (dev->pmic_notif_supp && dev->pdata->pmic_register_vbus_sn)
		dev->pdata->pmic_register_vbus_sn(&msm_otg_set_vbus_state);
	pr_info("peripheral driver registered w/ tranceiver\n");

	if (is_host())
		msm_otg_start_host(&dev->otg, 1);
	else if (is_b_sess_vld())
		msm_otg_start_peripheral(&dev->otg, 1);
	else
		msm_otg_suspend(dev);

	return 0;
}

static int msm_otg_set_host(struct otg_transceiver *xceiv, struct usb_bus *host)
{
	struct msm_otg *dev = container_of(xceiv, struct msm_otg, otg);

	if (!dev || (dev != the_msm_otg))
		return -ENODEV;

	if (!dev->start_host)
		return -ENODEV;

	if (!host) {
		msm_otg_start_host(xceiv, 0);
		dev->otg.host = 0;
		dev->start_host = 0;
		disable_idgnd(dev);
		return 0;
	}
	dev->otg.host = host;
	enable_idgnd(dev);
	pr_info("host driver registered w/ tranceiver\n");

#ifndef CONFIG_USB_GADGET_MSM_72K
	if (is_host())
		msm_otg_start_host(&dev->otg, 1);
	else
		msm_otg_suspend(dev);
#endif
	return 0;
}

static void msm_otg_set_vbus_state(int online)
{
	struct msm_otg *dev = the_msm_otg;

	if (online)
		msm_otg_set_suspend(&dev->otg, 0);
}

/* pmic irq handlers are called from thread context and
 * are allowed to sleep
 */
static irqreturn_t pmic_vbus_on_irq(int irq, void *data)
{
	struct msm_otg *dev = the_msm_otg;

	if (!dev->otg.gadget)
		return IRQ_HANDLED;

	pr_info("%s: vbus notification from pmic\n", __func__);

	msm_otg_set_suspend(&dev->otg, 0);

	return IRQ_HANDLED;
}


static irqreturn_t msm_otg_irq(int irq, void *data)
{
	struct msm_otg *dev = data;
	u32 otgsc = 0;

	if (atomic_read(&dev->in_lpm)) {
         //ruanmeisi_20101216 call msm_otg_set_suspend to resume in workqueue
		//msm_otg_resume(dev);
		schedule_otg_work(dev);
		//end
		return IRQ_HANDLED;
	}

	otgsc = readl(USB_OTGSC);
	if (!(otgsc & OTGSC_INTR_STS_MASK))
		return IRQ_HANDLED;

	if ((otgsc & OTGSC_IDIS) && (otgsc & OTGSC_IDIE)) {
		pr_info("ID -> (%s)\n", (otgsc & OTGSC_ID) ? "B" : "A");
		msm_otg_start_host(&dev->otg, is_host());
	} else if ((otgsc & OTGSC_BSVIS) && (otgsc & OTGSC_BSVIE)) {
		pr_info("VBUS - (%s)\n", otgsc & OTGSC_BSV ? "ON" : "OFF");
		if (!is_host())
			msm_otg_start_peripheral(&dev->otg, is_b_sess_vld());
	}
	writel(otgsc, USB_OTGSC);

	return IRQ_HANDLED;
}

#define ULPI_VERIFY_MAX_LOOP_COUNT  5
#define PHY_CALIB_RETRY_COUNT 10
static void phy_clk_reset(struct msm_otg *dev)
{
	unsigned rc;
	enum clk_reset_action assert = CLK_RESET_ASSERT;

	if (dev->pdata->phy_reset_sig_inverted)
		assert = CLK_RESET_DEASSERT;

	rc = clk_reset(dev->phy_reset_clk, assert);
	if (rc) {
		pr_err("%s: phy clk assert failed\n", __func__);
		return;
	}

	msleep(1);

	rc = clk_reset(dev->phy_reset_clk, !assert);
	if (rc) {
		pr_err("%s: phy clk deassert failed\n", __func__);
		return;
	}

	msleep(1);
}

static unsigned ulpi_read_with_reset(struct msm_otg *dev, unsigned reg)
{
	int temp;
	unsigned res;

	for (temp = 0; temp < ULPI_VERIFY_MAX_LOOP_COUNT; temp++) {
		res = ulpi_read(dev, reg);
		if (res != 0xffffffff)
			return res;

		phy_clk_reset(dev);
	}

	pr_err("%s: ulpi read failed for %d times\n",
			__func__, ULPI_VERIFY_MAX_LOOP_COUNT);

	return -1;
}

static int ulpi_write_with_reset(struct msm_otg *dev,
unsigned val, unsigned reg)
{
	int temp, res;

	for (temp = 0; temp < ULPI_VERIFY_MAX_LOOP_COUNT; temp++) {
		res = ulpi_write(dev, val, reg);
		if (!res)
			return 0;
		phy_clk_reset(dev);
	}
	pr_err("%s: ulpi write failed for %d times\n",
		__func__, ULPI_VERIFY_MAX_LOOP_COUNT);

	return -1;
}

/* some of the older targets does not turn off the PLL
 * if onclock bit is set and clocksuspendM bit is on,
 * hence clear them too and initiate the suspend mode
 * by clearing SupendM bit.
 */
static inline int turn_off_phy_pll(struct msm_otg *dev)
{
	unsigned res;

	res = ulpi_read_with_reset(dev, ULPI_CONFIG_REG1);
	if (res == 0xffffffff)
		return -ETIMEDOUT;

	res = ulpi_write_with_reset(dev,
		res & ~(ULPI_ONCLOCK), ULPI_CONFIG_REG1);
	if (res)
		return -ETIMEDOUT;

	res = ulpi_write_with_reset(dev,
		ULPI_CLOCK_SUSPENDM, ULPI_IFC_CTRL_CLR);
	if (res)
		return -ETIMEDOUT;

	/*Clear SuspendM bit to initiate suspend mode */
	res = ulpi_write_with_reset(dev,
		ULPI_SUSPENDM, ULPI_FUNC_CTRL_CLR);
	if (res)
		return -ETIMEDOUT;

	return res;
}

static inline int check_phy_caliberation(struct msm_otg *dev)
{
	unsigned res;

	res = ulpi_read_with_reset(dev, ULPI_DEBUG);

	if (res == 0xffffffff)
		return -ETIMEDOUT;

	if (!(res & ULPI_CALIB_STS) && ULPI_CALIB_VAL(res))
		return 0;

	return -1;
}

static int msm_otg_phy_caliberate(struct msm_otg *dev)
{
	int i = 0;
	unsigned long res;

	do {
		res = turn_off_phy_pll(dev);
		if (res)
			return -ETIMEDOUT;

		/* bring phy out of suspend */
		phy_clk_reset(dev);

		res = check_phy_caliberation(dev);
		if (!res)
			return res;
		i++;

	} while (i < PHY_CALIB_RETRY_COUNT);

	return res;
}

static int msm_otg_phy_reset(struct msm_otg *dev)
{
	unsigned rc;
	unsigned temp;
	unsigned long timeout;

	rc = clk_reset(dev->hs_clk, CLK_RESET_ASSERT);
	if (rc) {
		pr_err("%s: usb hs clk assert failed\n", __func__);
		return -1;
	}

	phy_clk_reset(dev);

	rc = clk_reset(dev->hs_clk, CLK_RESET_DEASSERT);
	if (rc) {
		pr_err("%s: usb hs clk deassert failed\n", __func__);
		return -1;
	}

	/* select ULPI phy */
	temp = (readl(USB_PORTSC) & ~PORTSC_PTS);
	writel(temp | PORTSC_PTS_ULPI, USB_PORTSC);

	rc = msm_otg_phy_caliberate(dev);
	if (rc)
		return rc;

	/* TBD: There are two link resets. One is below and other one
	 * is done immediately after this function. See if we can
	 * eliminate one of these.
	 */
	writel(USBCMD_RESET, USB_USBCMD);
	timeout = jiffies + USB_LINK_RESET_TIMEOUT;
	do {
		if (time_after(jiffies, timeout)) {
			pr_err("msm_otg: usb link reset timeout\n");
			break;
		}
		msleep(1);
	} while (readl(USB_USBCMD) & USBCMD_RESET);

	if (readl(USB_USBCMD) & USBCMD_RESET) {
		pr_err("%s: usb core reset failed\n", __func__);
		return -1;
	}

	return 0;
}

static void otg_reset(struct otg_transceiver *xceiv)
{
	struct msm_otg *dev = container_of(xceiv, struct msm_otg, otg);
	unsigned long timeout;

	clk_enable(dev->hs_clk);
	if (dev->pdata->phy_reset)
		dev->pdata->phy_reset(dev->regs);
	else
		msm_otg_phy_reset(dev);

	/*disable all phy interrupts*/
	ulpi_write(dev, 0xFF, 0x0F);
	ulpi_write(dev, 0xFF, 0x12);
	msleep(100);

	writel(USBCMD_RESET, USB_USBCMD);
	timeout = jiffies + USB_LINK_RESET_TIMEOUT;
	do {
		if (time_after(jiffies, timeout)) {
			pr_err("msm_otg: usb link reset timeout\n");
			break;
		}
		msleep(1);
	} while (readl(USB_USBCMD) & USBCMD_RESET);

	/* select ULPI phy */
	writel(0x80000000, USB_PORTSC);

	set_pre_emphasis_level(dev);
	set_cdr_auto_reset(dev);
	set_driver_amplitude(dev);

	writel(0x0, USB_AHB_BURST);
	writel(0x00, USB_AHB_MODE);
	clk_disable(dev->hs_clk);

	if (dev->otg.gadget)
		enable_sess_valid(dev);
	if (dev->otg.host)
		enable_idgnd(dev);
}

#ifdef CONFIG_DEBUG_FS
static int otg_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}
static ssize_t otg_mode_write(struct file *file, const char __user *buf,
				size_t count, loff_t *ppos)
{
	struct msm_otg *dev = file->private_data;
	struct otg_transceiver *otg = &dev->otg;
	int ret = count;

	if (otg->host) {
		pr_err("%s: mode switch not supported with host mode\n",
						__func__);
		return -EINVAL;
	}

	if (!memcmp(buf, "none", count - 1)) {
		msm_otg_start_peripheral(otg, 0);
	} else if (!memcmp(buf, "peripheral", count - 1)) {
		if (atomic_read(&dev->in_lpm))
			msm_otg_set_suspend(otg, 0);
		msm_otg_start_peripheral(otg, 1);
	} else {
		pr_info("%s: unknown mode specified\n", __func__);
		ret = -EINVAL;
	}

	return ret;
}
const struct file_operations otgfs_fops = {
	.open	= otg_open,
	.write	= otg_mode_write,
};

struct dentry *otg_debug_root;
struct dentry *otg_debug_mode;

static int otg_debugfs_init(struct msm_otg *dev)
{
	otg_debug_root = debugfs_create_dir("otg", NULL);
	if (!otg_debug_root)
		return -ENOENT;

	otg_debug_mode = debugfs_create_file("mode", 0222,
						otg_debug_root, dev,
						&otgfs_fops);
	if (!otg_debug_mode) {
		debugfs_remove(otg_debug_root);
		otg_debug_root = NULL;
		return -ENOENT;
	}

	return 0;
}

static void otg_debugfs_cleanup(void)
{
       debugfs_remove(otg_debug_mode);
       debugfs_remove(otg_debug_root);
}
#endif
static int __init msm_otg_probe(struct platform_device *pdev)
{
	int ret = 0;
	int vbus_on_irq = 0;
	struct resource *res;
	struct msm_otg *dev;

	dev = kzalloc(sizeof(struct msm_otg), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	dev->otg.dev = &pdev->dev;
	dev->pdata = pdev->dev.platform_data;

	if (!dev->pdata) {
		ret = -ENODEV;
		goto free_dev;
	}

	if (dev->pdata->pmic_vbus_irq) {
		vbus_on_irq = platform_get_irq_byname(pdev, "vbus_on");
		if (vbus_on_irq < 0) {
			pr_err("%s: unable to get vbus on irq\n", __func__);
			ret = vbus_on_irq;
			goto free_dev;
		}
	}

	if (dev->pdata->rpc_connect) {
		ret = dev->pdata->rpc_connect(1);
		pr_info("%s: rpc_connect(%d)\n", __func__, ret);
		if (ret) {
			pr_err("%s: rpc connect failed\n", __func__);
			ret = -ENODEV;
			goto free_dev;
		}
	}

	dev->hs_clk = clk_get(&pdev->dev, "usb_hs_clk");
	if (IS_ERR(dev->hs_clk)) {
		pr_err("%s: failed to get usb_hs_clk\n", __func__);
		ret = PTR_ERR(dev->hs_clk);
		goto rpc_fail;
	}
	clk_set_rate(dev->hs_clk, 60000000);

	if (!dev->pdata->usb_in_sps) {
		dev->hs_pclk = clk_get(&pdev->dev, "usb_hs_pclk");
		if (IS_ERR(dev->hs_pclk)) {
			pr_err("%s: failed to get usb_hs_pclk\n", __func__);
			ret = PTR_ERR(dev->hs_pclk);
			goto put_hs_clk;
		}
	}

	if (dev->pdata->core_clk) {
		dev->hs_cclk = clk_get(&pdev->dev, "usb_hs_core_clk");
		if (IS_ERR(dev->hs_cclk)) {
			pr_err("%s: failed to get usb_hs_core_clk\n", __func__);
			ret = PTR_ERR(dev->hs_cclk);
			goto put_hs_pclk;
		}
	}

	if (!dev->pdata->phy_reset) {
		dev->phy_reset_clk = clk_get(&pdev->dev, "usb_phy_clk");
		if (IS_ERR(dev->phy_reset_clk)) {
			pr_err("%s: failed to get usb_phy_clk\n", __func__);
			ret = PTR_ERR(dev->phy_reset_clk);
			goto put_hs_cclk;
		}
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		pr_err("%s: failed to get platform resource mem\n", __func__);
		ret = -ENODEV;
		goto put_phy_clk;
	}

	dev->regs = ioremap(res->start, resource_size(res));
	if (!dev->regs) {
		pr_err("%s: ioremap failed\n", __func__);
		ret = -ENOMEM;
		goto put_phy_clk;
	}
	dev->irq = platform_get_irq(pdev, 0);
	if (!dev->irq) {
		pr_err("%s: platform_get_irq failed\n", __func__);
		ret = -ENODEV;
		goto free_regs;
	}

	/* enable clocks */
	if (dev->hs_pclk)
		clk_enable(dev->hs_pclk);
	if (dev->hs_cclk)
		clk_enable(dev->hs_cclk);

	/* To reduce phy power consumption and to avoid external LDO
	 * on the board, PMIC comparators can be used to detect VBUS
	 * session change.
	 */
	if (dev->pdata->pmic_notif_init) {
		ret = dev->pdata->pmic_notif_init();
		if (!ret) {
			dev->pmic_notif_supp = 1;
			dev->pdata->pmic_enable_ldo(1);
		} else if (ret != -ENOTSUPP) {
			if (dev->hs_pclk)
				clk_disable(dev->hs_pclk);
			if (dev->hs_cclk)
				clk_disable(dev->hs_cclk);
			goto free_regs;
		}
	}

	otg_reset(&dev->otg);

	ret = request_irq(dev->irq, msm_otg_irq, IRQF_SHARED,
					"msm_otg", dev);
	if (ret) {
		pr_info("%s: request irq failed\n", __func__);
		if (dev->hs_pclk)
			clk_disable(dev->hs_pclk);
		if (dev->hs_cclk)
			clk_disable(dev->hs_cclk);
		goto free_regs;
	}

	the_msm_otg = dev;
	dev->vbus_on_irq = vbus_on_irq;
	dev->otg.set_peripheral = msm_otg_set_peripheral;
	dev->otg.set_host = msm_otg_set_host;
	dev->otg.set_suspend = msm_otg_set_suspend;
	dev->set_clk = msm_otg_set_clk;
	dev->reset = otg_reset;
	if (otg_set_transceiver(&dev->otg)) {
		WARN_ON(1);
		goto free_otg_irq;
	}

	device_init_wakeup(&pdev->dev, 1);

	if (vbus_on_irq) {
		ret = request_irq(vbus_on_irq, pmic_vbus_on_irq,
				IRQF_TRIGGER_RISING, "msm_otg_vbus_on", NULL);
		if (ret) {
			pr_info("%s: request_irq for vbus_on"
					"interrupt failed\n", __func__);
			goto free_otg_irq;
		}
	}
#ifdef CONFIG_DEBUG_FS
	ret = otg_debugfs_init(dev);
	if (ret) {
		pr_info("%s: otg_debugfs_init failed\n", __func__);
		goto free_vbus_irq;
	}
#endif
	return 0;

#ifdef CONFIG_DEBUG_FS
free_vbus_irq:
	if (vbus_on_irq)
		free_irq(vbus_on_irq, 0);
#endif
free_otg_irq:
	free_irq(dev->irq, dev);
free_regs:
	iounmap(dev->regs);
put_phy_clk:
	if (dev->phy_reset_clk)
		clk_put(dev->phy_reset_clk);
put_hs_cclk:
	if (dev->hs_cclk)
		clk_put(dev->hs_cclk);
put_hs_pclk:
	if (dev->hs_pclk)
		clk_put(dev->hs_pclk);
put_hs_clk:
	if (dev->hs_clk)
		clk_put(dev->hs_clk);
rpc_fail:
	dev->pdata->rpc_connect(0);
free_dev:
	kfree(dev);
	return ret;
}

static int __exit msm_otg_remove(struct platform_device *pdev)
{
	struct msm_otg *dev = the_msm_otg;

#ifdef CONFIG_DEBUG_FS
	otg_debugfs_cleanup();
#endif
	if (dev->pmic_notif_supp)
		dev->pdata->pmic_notif_deinit();

	free_irq(dev->irq, pdev);
	if (dev->vbus_on_irq)
		free_irq(dev->irq, 0);
	iounmap(dev->regs);
	if (dev->hs_cclk) {
		clk_disable(dev->hs_cclk);
		clk_put(dev->hs_cclk);
	}
	if (dev->hs_pclk) {
		clk_disable(dev->hs_pclk);
		clk_put(dev->hs_pclk);
	}
	if (dev->hs_clk)
		clk_put(dev->hs_clk);
	if (dev->phy_reset_clk)
		clk_put(dev->phy_reset_clk);
	kfree(dev);
	if (dev->pdata->rpc_connect)
		dev->pdata->rpc_connect(0);
	return 0;
}

static struct platform_driver msm_otg_driver = {
	.remove = __exit_p(msm_otg_remove),
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init msm_otg_init(void)
{
    //ruanmeisi_20101217
	init_otg_work();
	//end
	return platform_driver_probe(&msm_otg_driver, msm_otg_probe);
}

static void __exit msm_otg_exit(void)
{
	platform_driver_unregister(&msm_otg_driver);
}

module_init(msm_otg_init);
module_exit(msm_otg_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("MSM usb transceiver driver");
MODULE_VERSION("1.00");


struct otg_resume_work {
	struct msm_otg *dev ;
	struct work_struct work;
	
};

static struct otg_resume_work otg_work;

static void otg_resume_worker(struct work_struct *work)
{
	struct otg_resume_work *rwork = container_of(work, struct otg_resume_work, work);

	if (NULL == rwork || NULL == rwork->dev) {
		printk(KERN_ERR"otg:err %s %d: otg_resume_worker fail\n", __FUNCTION__, __LINE__);
		return ;
	}
	msm_otg_set_suspend(&(rwork->dev->otg), 0);
	return ;
}

static void init_otg_work(void)
{
	memset(&otg_work, 0, sizeof(otg_work));

	INIT_WORK(&(otg_work.work), otg_resume_worker);
}

static void schedule_otg_work(struct msm_otg *dev)
{
	otg_work.dev = dev;
	schedule_work(&(otg_work.work));
}
