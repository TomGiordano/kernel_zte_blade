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
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/gpio.h>

#include <linux/input/kp_flip_switch.h>

struct flip_switch {
	struct input_dev *input;
	struct flip_switch_pdata *fs_pdata;
};

static irqreturn_t flip_switch_irq(int irq, void *data)
{
	struct flip_switch *flip = data;
	int state;

	state = (gpio_get_value_cansleep(flip->fs_pdata->flip_gpio) ? 1 : 0) ^
						flip->fs_pdata->active_low;

	/* Flip switch gpio with value active_low means left portion of
	 * the switch has been pressed and it means right portion otherwise.
	 * So, the gpio value will never give us the release state.
	 * To compensate this the driver releases a corresponding fake
	 * key event, assuming that the user space does not need
	 * to know if the key is being pressed and held.
	 */
	if (!state) {
		input_report_key(flip->input, flip->fs_pdata->left_key, 1);
		input_sync(flip->input);
		input_report_key(flip->input, flip->fs_pdata->left_key, 0);
		input_sync(flip->input);
	} else {
		input_report_key(flip->input, flip->fs_pdata->right_key, 1);
		input_sync(flip->input);
		input_report_key(flip->input, flip->fs_pdata->right_key, 0);
		input_sync(flip->input);
	}

	return IRQ_HANDLED;
}

static int __devinit kp_flip_switch_probe(struct platform_device *pdev)
{
	struct flip_switch_pdata *pdata = pdev->dev.platform_data;
	struct input_dev *input;
	struct flip_switch *flip;
	int rc;

	if (!pdata->flip_gpio && !pdata->right_key && !pdata->left_key) {
		dev_err(&pdev->dev, "No proper platform data\n");
		return -ENODATA;
	}

	flip = kzalloc(sizeof(*flip), GFP_KERNEL);
	if (!flip)
		return -ENOMEM;

	input = input_allocate_device();
	if (!input) {
		dev_err(&pdev->dev, "unable to allocate input device\n");
		rc = -ENOMEM;
		goto err_alloc_device;
	}

	input->name = pdata->name;

	input->dev.parent	= &pdev->dev;

	input->id.bustype	= BUS_I2C;
	input->id.version	= 0x0001;
	input->id.product	= 0x0001;
	input->id.vendor	= 0x0001;

	input_set_capability(input, EV_KEY, pdata->right_key);
	input_set_capability(input, EV_KEY, pdata->left_key);

	rc = input_register_device(input);
	if (rc < 0) {
		dev_err(&pdev->dev, "unable to register flip input device\n");
		goto err_reg_input_dev;
	}

	input_set_drvdata(input, flip);

	flip->input = input;
	flip->fs_pdata = pdata;

	rc = gpio_request(flip->fs_pdata->flip_gpio, "flip_gpio");
	if (rc) {
		dev_err(&pdev->dev, "unable to request flip gpio\n");
		goto err_gpio_request;
	}

	if (flip->fs_pdata->flip_mpp_config) {
		rc = flip->fs_pdata->flip_mpp_config();
		if (rc < 0) {
			dev_err(&pdev->dev, "unable to config flip mpp\n");
			goto err_mpp_config;
		}
	}

	rc = request_threaded_irq(gpio_to_irq(flip->fs_pdata->flip_gpio),
		NULL, flip_switch_irq,
		IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
		"flip_switch", flip);
	if (rc) {
		dev_err(&pdev->dev, "failed to request flip irq\n");
		goto err_req_irq;
	}

	/* Wake-up source */
	device_init_wakeup(&pdev->dev, pdata->wakeup);

	if (device_may_wakeup(&pdev->dev))
		enable_irq_wake(gpio_to_irq(flip->fs_pdata->flip_gpio));

	platform_set_drvdata(pdev, flip);
	return 0;

err_req_irq:
err_mpp_config:
	gpio_free(flip->fs_pdata->flip_gpio);
err_gpio_request:
	input_unregister_device(input);
	input = NULL;
err_reg_input_dev:
	input_free_device(input);
err_alloc_device:
	kfree(flip);
	return rc;
}

static int __devexit kp_flip_switch_remove(struct platform_device *pdev)
{
	struct flip_switch *flip = platform_get_drvdata(pdev);

	if (device_may_wakeup(&pdev->dev))
		disable_irq_wake(gpio_to_irq(flip->fs_pdata->flip_gpio));
	device_init_wakeup(&pdev->dev, 0);

	free_irq(gpio_to_irq(flip->fs_pdata->flip_gpio), flip);
	gpio_free(flip->fs_pdata->flip_gpio);
	input_unregister_device(flip->input);
	kfree(flip);

	return 0;
}

static struct platform_driver kp_flip_switch_driver = {
	.probe		= kp_flip_switch_probe,
	.remove		= __devexit_p(kp_flip_switch_remove),
	.driver		= {
		.name = "kp_flip_switch",
		.owner = THIS_MODULE,
	},
};

static int __init kp_flip_switch_init(void)
{
	return platform_driver_register(&kp_flip_switch_driver);
}
module_init(kp_flip_switch_init);

static void __exit kp_flip_switch_exit(void)
{
	platform_driver_unregister(&kp_flip_switch_driver);
}
module_exit(kp_flip_switch_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("flip switch driver");
MODULE_VERSION("1.0");
MODULE_ALIAS("platform:kp_flip_switch");
