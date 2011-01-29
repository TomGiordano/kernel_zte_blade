
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <mach/board.h>
#include <linux/mutex.h>
#include <linux/timer.h>
#include <linux/remote_spinlock.h>
#include <linux/pm_qos_params.h>
#include <mach/gpio.h>

#define DEBUG 0

enum {
	I2C_WRITE_DATA          = 0x00,
	I2C_CLK_CTL             = 0x04,
	I2C_STATUS              = 0x08,
	I2C_READ_DATA           = 0x0c,
	I2C_INTERFACE_SELECT    = 0x10,

	I2C_WRITE_DATA_DATA_BYTE            = 0xff,
	I2C_WRITE_DATA_ADDR_BYTE            = 1U << 8,
	I2C_WRITE_DATA_LAST_BYTE            = 1U << 9,

	I2C_CLK_CTL_FS_DIVIDER_VALUE        = 0xff,
	I2C_CLK_CTL_HS_DIVIDER_VALUE        = 7U << 8,

	I2C_STATUS_WR_BUFFER_FULL           = 1U << 0,
	I2C_STATUS_RD_BUFFER_FULL           = 1U << 1,
	I2C_STATUS_BUS_ERROR                = 1U << 2,
	I2C_STATUS_PACKET_NACKED            = 1U << 3,
	I2C_STATUS_ARB_LOST                 = 1U << 4,
	I2C_STATUS_INVALID_WRITE            = 1U << 5,
	I2C_STATUS_FAILED                   = 3U << 6,
	I2C_STATUS_BUS_ACTIVE               = 1U << 8,
	I2C_STATUS_BUS_MASTER               = 1U << 9,
	I2C_STATUS_ERROR_MASK               = 0xfc,

	I2C_INTERFACE_SELECT_INTF_SELECT    = 1U << 0,
	I2C_INTERFACE_SELECT_SCL            = 1U << 8,
	I2C_INTERFACE_SELECT_SDA            = 1U << 9,
	I2C_STATUS_RX_DATA_STATE            = 3U << 11,
	I2C_STATUS_LOW_CLK_STATE            = 3U << 13,
};

struct msm_i2c_dev {
	struct device                *dev;
	void __iomem                 *base;	/* virtual */
	int                          irq;
	struct clk                   *clk;
	struct i2c_adapter           adap_pri;
	struct i2c_adapter           adap_aux;

	spinlock_t                   lock;

	struct i2c_msg               *msg;
	int                          rem;
	int                          pos;
	int                          cnt;
	int                          err;
	int                          flush_cnt;
	int                          rd_acked;
	int                          one_bit_t;
	remote_mutex_t               r_lock;
	remote_spinlock_t            s_lock;
	int                          suspended;
	struct mutex                 mlock;
	struct msm_i2c_platform_data *pdata;
	struct timer_list            pwr_timer;
	int                          clk_state;
	void                         *complete;
};

static void
msm_i2c_pwr_mgmt(struct msm_i2c_dev *dev, unsigned int state)
{
	dev->clk_state = state;
	if (state != 0)
		clk_enable(dev->clk);
	else
		clk_disable(dev->clk);
}

static void
msm_i2c_pwr_timer(unsigned long data)
{
	struct msm_i2c_dev *dev = (struct msm_i2c_dev *) data;
	dev_dbg(dev->dev, "I2C_Power: Inactivity based power management\n");
	if (dev->clk_state == 1)
		msm_i2c_pwr_mgmt(dev, 0);
}

#if DEBUG
static void
dump_status(uint32_t status)
{
	printk("STATUS (0x%.8x): ", status);
	if (status & I2C_STATUS_BUS_MASTER)
		printk("MST ");
	if (status & I2C_STATUS_BUS_ACTIVE)
		printk("ACT ");
	if (status & I2C_STATUS_INVALID_WRITE)
		printk("INV_WR ");
	if (status & I2C_STATUS_ARB_LOST)
		printk("ARB_LST ");
	if (status & I2C_STATUS_PACKET_NACKED)
		printk("NAK ");
	if (status & I2C_STATUS_BUS_ERROR)
		printk("BUS_ERR ");
	if (status & I2C_STATUS_RD_BUFFER_FULL)
		printk("RD_FULL ");
	if (status & I2C_STATUS_WR_BUFFER_FULL)
		printk("WR_FULL ");
	if (status & I2C_STATUS_FAILED)
		printk("FAIL 0x%x", (status & I2C_STATUS_FAILED));
	printk("\n");
}
#endif

static irqreturn_t
msm_i2c_interrupt(int irq, void *devid)
{
	struct msm_i2c_dev *dev = devid;
	uint32_t status = readl(dev->base + I2C_STATUS);
	int err = 0;

#if DEBUG
	dump_status(status);
#endif

	spin_lock(&dev->lock);
	if (!dev->msg) {
		printk(KERN_ERR "%s: IRQ but nothing to do!\n", __func__);
		spin_unlock(&dev->lock);
		return IRQ_HANDLED;
	}

	if (status & I2C_STATUS_ERROR_MASK) {
		err = -EIO;
		goto out_err;
	}

	if (dev->msg->flags & I2C_M_RD) {
		if (status & I2C_STATUS_RD_BUFFER_FULL) {

			if (dev->cnt) { /* DATA */
				uint8_t *data = &dev->msg->buf[dev->pos];

				if (dev->cnt == 2)
					writel(I2C_WRITE_DATA_LAST_BYTE,
						dev->base + I2C_WRITE_DATA);
				*data = readl(dev->base + I2C_READ_DATA);
				dev->cnt--;
				dev->pos++;
				if (dev->msg->len == 1)
					dev->rd_acked = 0;
				if (dev->cnt == 0)
					goto out_complete;

			} else {
				dev_err(dev->dev,
					"read did not stop, status - %x\n",
					status);
				err = -EIO;
				goto out_err;
			}
		} else if (dev->msg->len == 1 && dev->rd_acked == 0 &&
				((status & I2C_STATUS_RX_DATA_STATE) ==
				 I2C_STATUS_RX_DATA_STATE))
			writel(I2C_WRITE_DATA_LAST_BYTE,
				dev->base + I2C_WRITE_DATA);
	} else {
		uint16_t data;

		if (status & I2C_STATUS_WR_BUFFER_FULL) {
			dev_err(dev->dev,
				"Write buffer full in ISR on write?\n");
			err = -EIO;
			goto out_err;
		}

		if (dev->cnt) {
			data = dev->msg->buf[dev->pos];
			if (dev->cnt == 1 && dev->rem == 1)
				data |= I2C_WRITE_DATA_LAST_BYTE;

			status = readl(dev->base + I2C_STATUS);
			if ((status & I2C_STATUS_LOW_CLK_STATE) ==
					I2C_STATUS_LOW_CLK_STATE)
				udelay((dev->one_bit_t >> 1) + 1);
			writel(data, dev->base + I2C_WRITE_DATA);
			dev->pos++;
			dev->cnt--;
		} else
			goto out_complete;
	}

	spin_unlock(&dev->lock);
	return IRQ_HANDLED;

 out_err:
	dev->err = err;
 out_complete:
	complete(dev->complete);
	spin_unlock(&dev->lock);
	return IRQ_HANDLED;
}

static int
msm_i2c_poll_writeready(struct msm_i2c_dev *dev)
{
	uint32_t retries = 0;

	while (retries != 2000) {
		uint32_t status = readl(dev->base + I2C_STATUS);

		if (!(status & I2C_STATUS_WR_BUFFER_FULL))
			return 0;
		if (retries++ > 1000)
			udelay(100);
	}
	return -ETIMEDOUT;
}

static int
msm_i2c_poll_notbusy(struct msm_i2c_dev *dev)
{
	uint32_t retries = 0;

	while (retries != 2000) {
		uint32_t status = readl(dev->base + I2C_STATUS);

		if (!(status & I2C_STATUS_BUS_ACTIVE))
			return 0;
		if (retries++ > 1000)
			udelay(100);
	}
	return -ETIMEDOUT;
}

static int
msm_i2c_recover_bus_busy(struct msm_i2c_dev *dev, struct i2c_adapter *adap)
{
	int i;
	int gpio_clk;
	int gpio_dat;
	uint32_t status = readl(dev->base + I2C_STATUS);
	bool gpio_clk_status = false;

	if (!(status & (I2C_STATUS_BUS_ACTIVE | I2C_STATUS_WR_BUFFER_FULL)))
		return 0;

	dev->pdata->msm_i2c_config_gpio(adap->nr, 0);
	if (adap->nr % 2) {
		gpio_clk = dev->pdata->aux_clk;
		gpio_dat = dev->pdata->aux_dat;
	} else {
		gpio_clk = dev->pdata->pri_clk;
		gpio_dat = dev->pdata->pri_dat;
	}

	disable_irq(dev->irq);
	if (status & I2C_STATUS_RD_BUFFER_FULL) {
		dev_warn(dev->dev, "Read buffer full, status %x, intf %x\n",
			 status, readl(dev->base + I2C_INTERFACE_SELECT));
		writel(I2C_WRITE_DATA_LAST_BYTE, dev->base + I2C_WRITE_DATA);
		readl(dev->base + I2C_READ_DATA);
	} else if (status & I2C_STATUS_BUS_MASTER) {
		dev_warn(dev->dev, "Still the bus master, status %x, intf %x\n",
			 status, readl(dev->base + I2C_INTERFACE_SELECT));
		writel(I2C_WRITE_DATA_LAST_BYTE | 0xff,
		       dev->base + I2C_WRITE_DATA);
	}

	for (i = 0; i < 9; i++) {
		if (gpio_get_value(gpio_dat) && gpio_clk_status)
			break;
		gpio_direction_output(gpio_clk, 0);
		udelay(5);
		gpio_direction_output(gpio_dat, 0);
		udelay(5);
		gpio_direction_input(gpio_clk);
		udelay(5);
		if (!gpio_get_value(gpio_clk))
			udelay(20);
		if (!gpio_get_value(gpio_clk))
			msleep(10);
		gpio_clk_status = gpio_get_value(gpio_clk);
		gpio_direction_input(gpio_dat);
		udelay(5);
	}
	dev->pdata->msm_i2c_config_gpio(adap->nr, 1);
	udelay(10);

	status = readl(dev->base + I2C_STATUS);
	if (!(status & I2C_STATUS_BUS_ACTIVE)) {
		dev_info(dev->dev, "Bus busy cleared after %d clock cycles, "
			 "status %x, intf %x\n",
			 i, status, readl(dev->base + I2C_INTERFACE_SELECT));
		enable_irq(dev->irq);
		return 0;
	}

	dev_err(dev->dev, "Bus still busy, status %x, intf %x\n",
		 status, readl(dev->base + I2C_INTERFACE_SELECT));
	enable_irq(dev->irq);
	return -EBUSY;
}

static void
msm_i2c_rspin_lock(struct msm_i2c_dev *dev)
{
	int gotlock = 0;
	unsigned long flags;
	uint32_t *smem_ptr = (uint32_t *)dev->pdata->rmutex;
	do {
		remote_spin_lock_irqsave(&dev->s_lock, flags);
		if (*smem_ptr == 0) {
			*smem_ptr = 1;
			gotlock = 1;
		}
		remote_spin_unlock_irqrestore(&dev->s_lock, flags);
	} while (!gotlock);
}

static void
msm_i2c_rspin_unlock(struct msm_i2c_dev *dev)
{
	unsigned long flags;
	uint32_t *smem_ptr = (uint32_t *)dev->pdata->rmutex;
	remote_spin_lock_irqsave(&dev->s_lock, flags);
	*smem_ptr = 0;
	remote_spin_unlock_irqrestore(&dev->s_lock, flags);
}

static int
msm_i2c_xfer(struct i2c_adapter *adap, struct i2c_msg msgs[], int num)
{
	DECLARE_COMPLETION_ONSTACK(complete);
	struct msm_i2c_dev *dev = i2c_get_adapdata(adap);
	int ret;
	int rem = num;
	uint16_t addr;
	long timeout;
	unsigned long flags;
	int check_busy = 1;

	del_timer_sync(&dev->pwr_timer);
	mutex_lock(&dev->mlock);
	if (dev->suspended) {
		mutex_unlock(&dev->mlock);
		return -EIO;
	}

	if (dev->clk_state == 0) {
		dev_dbg(dev->dev, "I2C_Power: Enable I2C clock(s)\n");
		msm_i2c_pwr_mgmt(dev, 1);
	}
	pm_qos_update_requirement(PM_QOS_CPU_DMA_LATENCY, "msm_i2c",
					dev->pdata->pm_lat);
	if (dev->pdata->rmutex) {
		if (dev->pdata->rsl_id[0] == 'S')
			msm_i2c_rspin_lock(dev);
		else
			remote_mutex_lock(&dev->r_lock);
		get_irq_chip(dev->irq)->ack(dev->irq);
	}

	if (adap == &dev->adap_pri)
		writel(0, dev->base + I2C_INTERFACE_SELECT);
	else
		writel(I2C_INTERFACE_SELECT_INTF_SELECT,
				dev->base + I2C_INTERFACE_SELECT);
	enable_irq(dev->irq);
	while (rem) {
		addr = msgs->addr << 1;
		if (msgs->flags & I2C_M_RD)
			addr |= 1;

		spin_lock_irqsave(&dev->lock, flags);
		dev->msg = msgs;
		dev->rem = rem;
		dev->pos = 0;
		dev->err = 0;
		dev->flush_cnt = 0;
		dev->cnt = msgs->len;
		dev->complete = &complete;
		spin_unlock_irqrestore(&dev->lock, flags);

		if (check_busy) {
			ret = msm_i2c_poll_notbusy(dev);
			if (ret)
				ret = msm_i2c_recover_bus_busy(dev, adap);
				if (ret) {
					dev_err(dev->dev,
						"Error waiting for notbusy\n");
					goto out_err;
				}
			check_busy = 0;
		}

		if (rem == 1 && msgs->len == 0)
			addr |= I2C_WRITE_DATA_LAST_BYTE;

		/* Wait for WR buffer not full */
		ret = msm_i2c_poll_writeready(dev);
		if (ret) {
			ret = msm_i2c_recover_bus_busy(dev, adap);
			if (ret) {
				dev_err(dev->dev,
				"Error waiting for write ready before addr\n");
				goto out_err;
			}
		}
		if ((msgs->len == 1) && (msgs->flags & I2C_M_RD)) {
			uint32_t retries = 0;
			spin_lock_irqsave(&dev->lock, flags);

			writel(I2C_WRITE_DATA_ADDR_BYTE | addr,
				dev->base + I2C_WRITE_DATA);

			while (retries != 2000) {
				uint32_t status = readl(dev->base + I2C_STATUS);

					if ((status & I2C_STATUS_RX_DATA_STATE)
						== I2C_STATUS_RX_DATA_STATE)
						break;
				retries++;
			}
			if (retries >= 2000) {
				dev->rd_acked = 0;
				spin_unlock_irqrestore(&dev->lock, flags);
				goto wait_for_int;
			}

			dev->rd_acked = 1;
			writel(I2C_WRITE_DATA_LAST_BYTE,
					dev->base + I2C_WRITE_DATA);
			spin_unlock_irqrestore(&dev->lock, flags);
		} else {
			writel(I2C_WRITE_DATA_ADDR_BYTE | addr,
					 dev->base + I2C_WRITE_DATA);
		}

wait_for_int:

		timeout = wait_for_completion_timeout(&complete, HZ);
		if (!timeout) {
			dev_err(dev->dev, "Transaction timed out\n");
			writel(I2C_WRITE_DATA_LAST_BYTE,
				dev->base + I2C_WRITE_DATA);
			msleep(100);
			/* FLUSH */
			readl(dev->base + I2C_READ_DATA);
			readl(dev->base + I2C_STATUS);
			ret = -ETIMEDOUT;
			goto out_err;
		}
		if (dev->err) {
			dev_err(dev->dev,
				"Error during data xfer (%d)\n",
				dev->err);
			ret = dev->err;
			goto out_err;
		}

		if (msgs->flags & I2C_M_RD)
			check_busy = 1;

		msgs++;
		rem--;
	}

	ret = num;
 out_err:
	spin_lock_irqsave(&dev->lock, flags);
	dev->complete = NULL;
	dev->msg = NULL;
	dev->rem = 0;
	dev->pos = 0;
	dev->err = 0;
	dev->flush_cnt = 0;
	dev->cnt = 0;
	spin_unlock_irqrestore(&dev->lock, flags);
	disable_irq(dev->irq);
	if (dev->pdata->rmutex) {
		if (dev->pdata->rsl_id[0] == 'S')
			msm_i2c_rspin_unlock(dev);
		else
			remote_mutex_unlock(&dev->r_lock);
	}
	pm_qos_update_requirement(PM_QOS_CPU_DMA_LATENCY, "msm_i2c",
					PM_QOS_DEFAULT_VALUE);
	mod_timer(&dev->pwr_timer, (jiffies + 3*HZ));
	mutex_unlock(&dev->mlock);
	return ret;
}

static u32
msm_i2c_func(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | (I2C_FUNC_SMBUS_EMUL & ~I2C_FUNC_SMBUS_QUICK);
}

static const struct i2c_algorithm msm_i2c_algo = {
	.master_xfer	= msm_i2c_xfer,
	.functionality	= msm_i2c_func,
};

static int
msm_i2c_probe(struct platform_device *pdev)
{
	struct msm_i2c_dev	*dev;
	struct resource		*mem, *irq, *ioarea;
	int ret;
	int fs_div;
	int hs_div;
	int i2c_clk;
	int clk_ctl;
	struct clk *clk;
	struct msm_i2c_platform_data *pdata;

	printk(KERN_INFO "msm_i2c_probe\n");

	/* NOTE: driver uses the static register mapping */
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem) {
		dev_err(&pdev->dev, "no mem resource?\n");
		return -ENODEV;
	}
	irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (!irq) {
		dev_err(&pdev->dev, "no irq resource?\n");
		return -ENODEV;
	}

	ioarea = request_mem_region(mem->start, (mem->end - mem->start) + 1,
			pdev->name);
	if (!ioarea) {
		dev_err(&pdev->dev, "I2C region already claimed\n");
		return -EBUSY;
	}
	clk = clk_get(&pdev->dev, "i2c_clk");
	if (IS_ERR(clk)) {
		dev_err(&pdev->dev, "Could not get clock\n");
		ret = PTR_ERR(clk);
		goto err_clk_get_failed;
	}

	pdata = pdev->dev.platform_data;
	if (!pdata) {
		dev_err(&pdev->dev, "platform data not initialized\n");
		ret = -ENOSYS;
		goto err_clk_get_failed;
	}
#ifdef CONFIG_ZTE_PLATFORM
       //  if(machine_is_mooncake())
       {
        	printk(KERN_INFO" Warning: msm on-chip aux i2c bus disabled on Board mooncake!\n");
       }
#else
	{
		if (!pdata->msm_i2c_config_gpio) {
		
			dev_err(&pdev->dev, "config_gpio function not initialized\n");
			ret = -ENOSYS;
			goto err_clk_get_failed;
		}
	}
#endif
	if (pdata->clk_freq <= 0 || pdata->clk_freq > 400000) {
		dev_err(&pdev->dev, "clock frequency not supported\n");
		ret = -EIO;
		goto err_clk_get_failed;
	}

	dev = kzalloc(sizeof(struct msm_i2c_dev), GFP_KERNEL);
	if (!dev) {
		ret = -ENOMEM;
		goto err_alloc_dev_failed;
	}

	dev->dev = &pdev->dev;
	dev->irq = irq->start;
	dev->clk = clk;
	dev->pdata = pdata;
	dev->base = ioremap(mem->start, (mem->end - mem->start) + 1);
	if (!dev->base) {
		ret = -ENOMEM;
		goto err_ioremap_failed;
	}

	dev->one_bit_t = USEC_PER_SEC/pdata->clk_freq;
	spin_lock_init(&dev->lock);
	platform_set_drvdata(pdev, dev);

	clk_enable(clk);

	if (pdata->rmutex && pdata->rsl_id[0] == 'S') {
		remote_spinlock_id_t rmid;
		rmid = pdata->rsl_id;
		if (remote_spin_lock_init(&dev->s_lock, rmid) != 0)
			pdata->rmutex = 0;
	} else if (pdata->rmutex) {
		struct remote_mutex_id rmid;
		rmid.r_spinlock_id = pdata->rsl_id;
		rmid.delay_us = 10000000/pdata->clk_freq;
		if (remote_mutex_init(&dev->r_lock, &rmid) != 0)
			pdata->rmutex = 0;
	}
	i2c_clk = 19200000; /* input clock */
	fs_div = ((i2c_clk / pdata->clk_freq) / 2) - 3;
	hs_div = 3;
	clk_ctl = ((hs_div & 0x7) << 8) | (fs_div & 0xff);
	writel(clk_ctl, dev->base + I2C_CLK_CTL);
	printk(KERN_INFO "msm_i2c_probe: clk_ctl %x, %d Hz\n",
	       clk_ctl, i2c_clk / (2 * ((clk_ctl & 0xff) + 3)));

	i2c_set_adapdata(&dev->adap_pri, dev);
	dev->adap_pri.algo = &msm_i2c_algo;
	strlcpy(dev->adap_pri.name,
		"MSM I2C adapter-PRI",
		sizeof(dev->adap_pri.name));

	dev->adap_pri.nr = pdev->id;
	ret = i2c_add_numbered_adapter(&dev->adap_pri);
	if (ret) {
		dev_err(&pdev->dev, "Primary i2c_add_adapter failed\n");
		goto err_i2c_add_adapter_failed;
	}

#ifdef CONFIG_ZTE_PLATFORM
      //  if(machine_is_mooncake())
       {
       }
#else
	{
		i2c_set_adapdata(&dev->adap_aux, dev);
		dev->adap_aux.algo = &msm_i2c_algo;
		strlcpy(dev->adap_aux.name,
			"MSM I2C adapter-AUX",
			sizeof(dev->adap_aux.name));

		dev->adap_aux.nr = pdev->id + 1;
		ret = i2c_add_numbered_adapter(&dev->adap_aux);
		if (ret) {
			dev_err(&pdev->dev, "auxiliary i2c_add_adapter failed\n");
			i2c_del_adapter(&dev->adap_pri);
			goto err_i2c_add_adapter_failed;
		}
	}
#endif
	ret = request_irq(dev->irq, msm_i2c_interrupt,
			IRQF_TRIGGER_RISING, pdev->name, dev);
	if (ret) {
		dev_err(&pdev->dev, "request_irq failed\n");
		goto err_request_irq_failed;
	}
	pm_qos_add_requirement(PM_QOS_CPU_DMA_LATENCY, "msm_i2c",
					PM_QOS_DEFAULT_VALUE);
	disable_irq(dev->irq);
	dev->suspended = 0;
	mutex_init(&dev->mlock);
	dev->clk_state = 0;
	/* Config GPIOs for primary and secondary lines */
	pdata->msm_i2c_config_gpio(dev->adap_pri.nr, 1);
/*
	pdata->msm_i2c_config_gpio(dev->adap_aux.nr, 1);
*/
#ifdef CONFIG_ZTE_PLATFORM

       {
       }
#else
	{
		pdata->msm_i2c_config_gpio(dev->adap_aux.nr, 1);
	}
#endif
	clk_disable(dev->clk);
	setup_timer(&dev->pwr_timer, msm_i2c_pwr_timer, (unsigned long) dev);

	return 0;
err_request_irq_failed:
	i2c_del_adapter(&dev->adap_pri);
	i2c_del_adapter(&dev->adap_aux);
err_i2c_add_adapter_failed:
	clk_disable(clk);
	iounmap(dev->base);
err_ioremap_failed:
	kfree(dev);
err_alloc_dev_failed:
	clk_put(clk);
err_clk_get_failed:
	release_mem_region(mem->start, (mem->end - mem->start) + 1);
	return ret;
}

static int
msm_i2c_remove(struct platform_device *pdev)
{
	struct msm_i2c_dev	*dev = platform_get_drvdata(pdev);
	struct resource		*mem;

	mutex_lock(&dev->mlock);
	dev->suspended = 1;
	mutex_unlock(&dev->mlock);
	mutex_destroy(&dev->mlock);
	del_timer_sync(&dev->pwr_timer);
	if (dev->clk_state != 0)
		msm_i2c_pwr_mgmt(dev, 0);
	platform_set_drvdata(pdev, NULL);
	pm_qos_remove_requirement(PM_QOS_CPU_DMA_LATENCY, "msm_i2c");
	free_irq(dev->irq, dev);
	i2c_del_adapter(&dev->adap_pri);
	i2c_del_adapter(&dev->adap_aux);
	clk_put(dev->clk);
	iounmap(dev->base);
	kfree(dev);
	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	release_mem_region(mem->start, (mem->end - mem->start) + 1);
	return 0;
}

static int msm_i2c_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct msm_i2c_dev *dev = platform_get_drvdata(pdev);
	if (dev) {
		mutex_lock(&dev->mlock);
		dev->suspended = 1;
		mutex_unlock(&dev->mlock);
		del_timer_sync(&dev->pwr_timer);
		if (dev->clk_state != 0)
			msm_i2c_pwr_mgmt(dev, 0);
	}

	return 0;
}

static int msm_i2c_resume(struct platform_device *pdev)
{
	struct msm_i2c_dev *dev = platform_get_drvdata(pdev);
	dev->suspended = 0;
	return 0;
}

static struct platform_driver msm_i2c_driver = {
	.probe		= msm_i2c_probe,
	.remove		= msm_i2c_remove,
	.suspend	= msm_i2c_suspend,
	.resume		= msm_i2c_resume,
	.driver		= {
		.name	= "msm_i2c",
		.owner	= THIS_MODULE,
	},
};
static int __init
msm_i2c_init_driver(void)
{
	return platform_driver_register(&msm_i2c_driver);
}
subsys_initcall(msm_i2c_init_driver);

static void __exit msm_i2c_exit_driver(void)
{
	platform_driver_unregister(&msm_i2c_driver);
}
module_exit(msm_i2c_exit_driver);

