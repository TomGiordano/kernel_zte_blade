#include <linux/delay.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/mfd/marimba-tsadc.h>
#include <linux/pm.h>

#if defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#endif

#include <mach/msm_ts.h>
#include <linux/jiffies.h>

#define TSSC_CTL			0x100
#define 	TSSC_CTL_PENUP_IRQ	(1 << 12)
#define 	TSSC_CTL_DATA_FLAG	(1 << 11)
#define 	TSSC_CTL_DEBOUNCE_EN	(1 << 6)
#define 	TSSC_CTL_EN_AVERAGE	(1 << 5)
#define 	TSSC_CTL_MODE_MASTER	(3 << 3)
#define 	TSSC_CTL_SW_RESET	(1 << 2)
#define 	TSSC_CTL_ENABLE		(1 << 0)
#define TSSC_OPN			0x104
#define 	TSSC_OPN_NOOP		0x00
#define 	TSSC_OPN_4WIRE_X	0x01
#define 	TSSC_OPN_4WIRE_Y	0x02
#define 	TSSC_OPN_4WIRE_Z1	0x03
#define 	TSSC_OPN_4WIRE_Z2	0x04
#define TSSC_SAMPLING_INT		0x108
#define TSSC_STATUS			0x10c
#define TSSC_AVG_12			0x110
#define TSSC_AVG_34			0x114
#define TSSC_SAMPLE(op,samp)		((0x118 + ((op & 0x3) * 0x20)) + \
					 ((samp & 0x7) * 0x4))
#define TSSC_TEST_1			0x198
	#define TSSC_TEST_1_EN_GATE_DEBOUNCE (1 << 2)
#define TSSC_TEST_2			0x19c
#define TS_PENUP_TIMEOUT_MS 70 

struct msm_ts {
	struct msm_ts_platform_data	*pdata;
	struct input_dev		*input_dev;
	void __iomem			*tssc_base;
	uint32_t			ts_down:1;
	struct ts_virt_key		*vkey_down;
	//struct marimba_tsadc_client	*ts_client;

	unsigned int			sample_irq;
	unsigned int			pen_up_irq;

#if defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend		early_suspend;
#endif
	struct device			*dev;
struct timer_list timer;
};

static uint32_t msm_tsdebug;
module_param_named(tsdebug, msm_tsdebug, uint, 0664);

#define tssc_readl(t, a)	(readl(((t)->tssc_base) + (a)))
#define tssc_writel(t, v, a)	do {writel(v, ((t)->tssc_base) + (a));} while(0)

static void setup_next_sample(struct msm_ts *ts)
{
	uint32_t tmp;
	tmp = ((7 << 7) | TSSC_CTL_DEBOUNCE_EN | TSSC_CTL_EN_AVERAGE |
	       TSSC_CTL_MODE_MASTER | TSSC_CTL_ENABLE);
	tssc_writel(ts, tmp, TSSC_CTL);
}

static void ts_timer(unsigned long arg)
{
	struct msm_ts *ts = (struct msm_ts *)arg;
 	input_report_key(ts->input_dev, BTN_TOUCH, 0);
 	input_sync(ts->input_dev);
}
static irqreturn_t msm_ts_irq(int irq, void *dev_id)
{
	struct msm_ts *ts = dev_id;
	struct msm_ts_platform_data *pdata = ts->pdata;

	uint32_t tssc_avg12, tssc_avg34, tssc_status, tssc_ctl;
	int x, y, z1, z2;
	int was_down;
	int down;
  int z=0;
#if defined( CONFIG_MACH_V9)
	int temp = 0;
#endif
  del_timer_sync(&ts->timer);
	tssc_ctl = tssc_readl(ts, TSSC_CTL);
	tssc_status = tssc_readl(ts, TSSC_STATUS);
	tssc_avg12 = tssc_readl(ts, TSSC_AVG_12);
	tssc_avg34 = tssc_readl(ts, TSSC_AVG_34);

	setup_next_sample(ts);

	x = tssc_avg12 & 0xffff;
	y = tssc_avg12 >> 16;
	z1 = tssc_avg34 & 0xffff;
	z2 = tssc_avg34 >> 16;
	down = !(tssc_ctl & TSSC_CTL_PENUP_IRQ);
	was_down = ts->ts_down;
	ts->ts_down = down;

	/* no valid data */
	if (down && !(tssc_ctl & TSSC_CTL_DATA_FLAG))
		return IRQ_HANDLED;

	if (msm_tsdebug & 2)
		printk("%s: down=%d, x=%d, y=%d, z1=%d, z2=%d, status %x\n",
		       __func__, down, x, y, z1, z2, tssc_status);
	if (down) 
		{
			if ( 0 == z1 ) return IRQ_HANDLED;
			//z = ( ( 2 * z2 - 2 * z1 - 3) * x) / ( 2 * z1 + 3);
			z = ( ( z2 - z1 - 2)*x) / ( z1 + 2 );
			z = ( 2500 - z ) * 1000 / ( 2500 - 900 );
			if( z<0 ) z = 255;
		}
#if defined( CONFIG_MACH_V9)
	//int temp;
	if (pdata->inv_x) temp = pdata->inv_x - x;
	if (pdata->inv_y) x = pdata->inv_y - y;
	y = temp;
#else
	if (pdata->inv_x) x = pdata->inv_x - x;
	if (pdata->inv_y) y = pdata->inv_y - y;
#endif
	if (x < 0) x = 0;
	if (y < 0) y = 0;

	if (down) {
		input_report_abs(ts->input_dev, ABS_X, x);
		input_report_abs(ts->input_dev, ABS_Y, y);
			input_report_abs(ts->input_dev, ABS_PRESSURE, z);
	}
	input_report_key(ts->input_dev, BTN_TOUCH, down);
	input_sync(ts->input_dev);

	if (30 == irq)mod_timer(&ts->timer,jiffies + msecs_to_jiffies(TS_PENUP_TIMEOUT_MS));//ZTE_TS_ZT_005 @2010-03-05
	return IRQ_HANDLED;
}
static int __devinit msm_ts_hw_init(struct msm_ts *ts)
{
#if 0
	uint32_t tmp;

	tssc_writel(ts, TSSC_CTL_ENABLE, TSSC_CTL);
	/* Enable software reset*/
	tssc_writel(ts, TSSC_CTL_SW_RESET, TSSC_CTL);

	/* op1 - measure X, 1 sample, 12bit resolution */
	tmp = (TSSC_OPN_4WIRE_X << 16) | (2 << 8) | (2 << 0);
	/* op2 - measure Y, 1 sample, 12bit resolution */
	tmp |= (TSSC_OPN_4WIRE_Y << 20) | (2 << 10) | (2 << 2);
	/* op3 - measure Z1, 1 sample, 8bit resolution */
	tmp |= (TSSC_OPN_4WIRE_Z1 << 24) | (2 << 12) | (0 << 4);

	/* XXX: we don't actually need to measure Z2 (thus 0 samples) when
	 * doing voltage-driven measurement */
	/* op4 - measure Z2, 0 samples, 8bit resolution */
	tmp |= (TSSC_OPN_4WIRE_Z2 << 28) | (0 << 14) | (0 << 6);
	tssc_writel(ts, tmp, TSSC_OPN);
	tssc_writel(ts, 16, TSSC_SAMPLING_INT);
	tssc_writel(ts, TSSC_TEST_1_EN_GATE_DEBOUNCE, TSSC_TEST_1);
#endif
	setup_next_sample(ts);

	return 0;
}

#ifdef CONFIG_PM
static int
msm_ts_suspend(struct device *dev)
{
	struct msm_ts *ts =  dev_get_drvdata(dev);
	uint32_t val;

	disable_irq(ts->sample_irq);
	disable_irq(ts->pen_up_irq);

	val = tssc_readl(ts, TSSC_CTL);
	val &= ~TSSC_CTL_ENABLE;
	tssc_writel(ts, val, TSSC_CTL);

	return 0;
}

static int
msm_ts_resume(struct device *dev)
{
	struct msm_ts *ts =  dev_get_drvdata(dev);

	msm_ts_hw_init(ts);

	enable_irq(ts->sample_irq);
	enable_irq(ts->pen_up_irq);

	return 0;
}

static struct dev_pm_ops msm_touchscreen_pm_ops = {
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= msm_ts_suspend,
	.resume		= msm_ts_resume,
#endif
};
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
static void msm_ts_early_suspend(struct early_suspend *h)
{
	struct msm_ts *ts = container_of(h, struct msm_ts, early_suspend);

	msm_ts_suspend(ts->dev);
}

static void msm_ts_late_resume(struct early_suspend *h)
{
	struct msm_ts *ts = container_of(h, struct msm_ts, early_suspend);

	msm_ts_resume(ts->dev);
}
#endif


static int __devinit msm_ts_probe(struct platform_device *pdev)
{
	struct msm_ts_platform_data *pdata = pdev->dev.platform_data;
	struct msm_ts *ts;
	struct resource *tssc_res;
	struct resource *irq1_res;
	struct resource *irq2_res;
	int err = 0;
	tssc_res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "tssc");
	irq1_res = platform_get_resource_byname(pdev, IORESOURCE_IRQ, "tssc1");
	irq2_res = platform_get_resource_byname(pdev, IORESOURCE_IRQ, "tssc2");

	if (!tssc_res || !irq1_res || !irq2_res) {
		pr_err("%s: required resources not defined\n", __func__);
		return -ENODEV;
	}

	if (pdata == NULL) {
		pr_err("%s: missing platform_data\n", __func__);
		return -ENODEV;
	}

	ts = kzalloc(sizeof(struct msm_ts), GFP_KERNEL);
	if (ts == NULL) {
		pr_err("%s: No memory for struct msm_ts\n", __func__);
		return -ENOMEM;
	}
	ts->pdata = pdata;
	ts->dev	  = &pdev->dev;

	ts->sample_irq = irq1_res->start;
	ts->pen_up_irq = irq2_res->start;

	ts->tssc_base = ioremap(tssc_res->start, resource_size(tssc_res));
	if (ts->tssc_base == NULL) {
		pr_err("%s: Can't ioremap region (0x%08x - 0x%08x)\n", __func__,
		       (uint32_t)tssc_res->start, (uint32_t)tssc_res->end);
		err = -ENOMEM;
		goto err_ioremap_tssc;
	}
#if 0
	ts_client = marimba_tsadc_register(pdev, 1);
	if (IS_ERR(ts_client)) {
		pr_err("%s: Unable to register with TSADC\n", __func__);
		err = -ENOMEM;
		goto err_tsadc_register;
	}
	ts->ts_client = ts_client;

	err = marimba_tsadc_start(ts_client);
	if (err) {
		pr_err("%s: Unable to start TSADC\n", __func__);
		err = -EINVAL;
		goto err_start_tsadc;
	}
#endif
	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) {
		pr_err("failed to allocate touchscreen input device\n");
		err = -ENOMEM;
		goto err_alloc_input_dev;
	}
	ts->input_dev->name = "msm-touchscreen";
	ts->input_dev->dev.parent = &pdev->dev;

	input_set_drvdata(ts->input_dev, ts);

	input_set_capability(ts->input_dev, EV_KEY, BTN_TOUCH);
	set_bit(EV_ABS, ts->input_dev->evbit);

	input_set_abs_params(ts->input_dev, ABS_X, pdata->min_x, pdata->max_x,
			     0, 0);
	input_set_abs_params(ts->input_dev, ABS_Y, pdata->min_y, pdata->max_y,
			     0, 0);
	input_set_abs_params(ts->input_dev, ABS_PRESSURE, pdata->min_press,
			     pdata->max_press, 0, 0);
	err = input_register_device(ts->input_dev);
	if (err != 0) {
		pr_err("%s: failed to register input device\n", __func__);
		goto err_input_dev_reg;
	}

  setup_timer(&ts->timer, ts_timer, (unsigned long)ts);
	msm_ts_hw_init(ts);

	err = request_irq(ts->sample_irq, msm_ts_irq,
			  (irq1_res->flags & ~IORESOURCE_IRQ) | IRQF_DISABLED,
			  "msm_touchscreen", ts);
	if (err != 0) {
		pr_err("%s: Cannot register irq1 (%d)\n", __func__, err);
		goto err_request_irq1;
	}

	err = request_irq(ts->pen_up_irq, msm_ts_irq,
			  (irq2_res->flags & ~IORESOURCE_IRQ) | IRQF_DISABLED,
			  "msm_touchscreen", ts);
	if (err != 0) {
		pr_err("%s: Cannot register irq2 (%d)\n", __func__, err);
		goto err_request_irq2;
	}

	platform_set_drvdata(pdev, ts);

#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN +
						TSSC_SUSPEND_LEVEL;
	ts->early_suspend.suspend = msm_ts_early_suspend;
	ts->early_suspend.resume = msm_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif

	pr_info("%s: tssc_base=%p irq1=%d irq2=%d\n", __func__,
		ts->tssc_base, (int)ts->sample_irq, (int)ts->pen_up_irq);
//	dump_tssc_regs(ts);
	return 0;

err_request_irq2:
	free_irq(ts->sample_irq, ts);

err_request_irq1:
	tssc_writel(ts, TSSC_CTL_ENABLE, TSSC_CTL);

err_input_dev_reg:
	input_set_drvdata(ts->input_dev, NULL);
	input_free_device(ts->input_dev);

err_alloc_input_dev:
#if 0
err_start_tsadc:
	marimba_tsadc_unregister(ts->ts_client);

err_tsadc_register:
#endif
	iounmap(ts->tssc_base);

err_ioremap_tssc:
	kfree(ts);
	return err;
}
#if 0
static int __devexit msm_ts_remove(struct platform_device *pdev)
{
	struct msm_ts *ts = platform_get_drvdata(pdev);

	marimba_tsadc_unregister(ts->ts_client);
	free_irq(ts->sample_irq, ts);
	free_irq(ts->pen_up_irq, ts);
	input_unregister_device(ts->input_dev);
	iounmap(ts->tssc_base);
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ts->early_suspend);
#endif
	platform_set_drvdata(pdev, NULL);
	kfree(ts);

	return 0;
}
#endif
static struct platform_driver msm_touchscreen_driver = {
	.driver = {
		.name = "msm_touchscreen",
		.owner = THIS_MODULE,
#ifdef CONFIG_PM
		.pm = &msm_touchscreen_pm_ops,
#endif
	},
	.probe		= msm_ts_probe,
	//.remove = __devexit_p(msm_ts_remove),
};

static int __init msm_ts_init(void)
{
	return platform_driver_register(&msm_touchscreen_driver);
}

static void __exit msm_ts_exit(void)
{
	platform_driver_unregister(&msm_touchscreen_driver);
}

module_init(msm_ts_init);
module_exit(msm_ts_exit);
MODULE_DESCRIPTION("Qualcomm MSM/QSD Touchscreen controller driver");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:msm_touchscreen");
