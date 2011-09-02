/*
 * Jogball driver for MSM platform.
 *
 * Copyright (c) 2008 QUALCOMM USA, INC.
 *
 * All source code in this file is licensed under the following license
 * except where indicated.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can find it at http://www.fsf.org
 *
*/

/*===========================================================================
when         who         what, where, why                       comment tag
--------     ----      -------------------------------------    -------------
2010-08-03	xiayuchun	add earlysuspend mode coded by yintianci.	ZTE_XYC_PM_20100803
2010-05-24  huangyanjun    add suspend and resume function for jogball ZTE_HYJ_PM_20100524
2010-04-19  zhanglei    creat new jogball driver
===========================================================================*/

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <mach/gpio.h>
#include <mach/jogball_key.h>


MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Android Jogball KEY driver");

struct jogball_driver_data {
       struct input_dev  *jogball_input_dev;
	int gpio_irq_up;
	int gpio_irq_down;
	int gpio_irq_left;
	int gpio_irq_right;
//ZTE_XYC_PM_20100803
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend earlysuspend;
#endif
};

#define GPIO_JOGBALL_UP_INT  MSM_GPIO_TO_INT(30)
#define GPIO_JOGBALL_DOWN_INT  MSM_GPIO_TO_INT(38)
#define GPIO_JOGBALL_LEFT_INT  MSM_GPIO_TO_INT(76)
#define GPIO_JOGBALL_RIGHT_INT  MSM_GPIO_TO_INT(88)

//#define TraceBall_TAG "ZL:[TraceBall]--"

#define MAX_NUM 6

static irqreturn_t jogball_key_interrupt(int irq, void *dev_id)
{      
       static int num_up_irq = 0;
	static int num_down_irq = 0;
	static int num_left_irq = 0;
	static int num_right_irq = 0;

	bool report_key = 0; /*  1  表示需要上报值*/
       
	struct jogball_driver_data *dd_jogball = dev_id;

	struct input_dev *jogball_input_dev = dd_jogball->jogball_input_dev;
       //pr_info(TraceBall_TAG"Enter into jogball_key_interrupt()");
	   
  //disable all the gpio irq
  //disable_irq(GPIO_JOGBALL_UP_INT);
	//disable_irq(GPIO_JOGBALL_LEFT_INT);
	//disable_irq(GPIO_JOGBALL_DOWN_INT);
  //disable_irq(GPIO_JOGBALL_RIGHT_INT);

	switch (irq) {
	case GPIO_JOGBALL_UP_INT:
		num_up_irq+=1;
		//pr_info(TraceBall_TAG "Enter into jogball_key_interrupt()--GPIO_JOGBALL_UP_INT\n");
		break;
	case GPIO_JOGBALL_DOWN_INT:
		num_down_irq+=1;
		//pr_info(TraceBall_TAG "Enter into jogball_key_interrupt()--GPIO_JOGBALL_DOWN_INT\n");
		break;
	case GPIO_JOGBALL_LEFT_INT:
		num_left_irq+=1;
		//pr_info(TraceBall_TAG "Enter into jogball_key_interrupt()--GPIO_JOGBALL_LEFT_INT\n");
		break;
	case GPIO_JOGBALL_RIGHT_INT:
		num_right_irq+=1;
		//pr_info(TraceBall_TAG "Enter into jogball_key_interrupt()--GPIO_JOGBALL_RIGHT_INT\n");
		break;
	default:
		break;
	}

	//最先达到MAX_NUM  次中断的上报该键值( 可以根据情况设置该值)
	
	if((MAX_NUM == num_up_irq)&&(0 == report_key))
	{    
	      //pr_info(TraceBall_TAG "Enter into jogball_key_interrupt()--Report UP Key--Begin\n");
	      report_key = 1;
	      /*  调用系统api  上报(  方向: 上)  */
        input_report_key(jogball_input_dev, KEY_UP, 1);/*  按下*/
	      input_report_key(jogball_input_dev, KEY_UP, 0);/*  弹起*/  /*  系统只有收到按下和弹起两个事件后才会处理，只收到按下，是不会处理的*/
	      //pr_info(TraceBall_TAG "Enter into jogball_key_interrupt()--Report UP Key--End\n");
		  
	}
	else if((MAX_NUM == num_down_irq)&&(0 == report_key))
	{    
	      //pr_info(TraceBall_TAG "Enter into jogball_key_interrupt()--Report DOWN Key--Begin\n");
	      report_key = 1;
	      /*  调用系统api  上报(  方向: 下)  */
        input_report_key(jogball_input_dev, KEY_DOWN, 1);
	      input_report_key(jogball_input_dev, KEY_DOWN, 0);
        //pr_info(TraceBall_TAG "Enter into jogball_key_interrupt()--Report DOWN Key--End\n");
   
	}
	else if((MAX_NUM == num_left_irq)&&(0 == report_key))
	{    
	      //pr_info(TraceBall_TAG "Enter into jogball_key_interrupt()--Report LEFT Key--Begin\n");
	      report_key = 1;
	      /*  调用系统api  上报(  方向: 左)  */	
        input_report_key(jogball_input_dev, KEY_LEFT, 1);
	      input_report_key(jogball_input_dev, KEY_LEFT, 0);
	      //pr_info(TraceBall_TAG "Enter into jogball_key_interrupt()--Report LEFT Key--End\n");
		  
	}
	else if((MAX_NUM == num_right_irq)&&(0 == report_key))
	{   
	     //pr_info(TraceBall_TAG "Enter into jogball_key_interrupt()--Report RIGHT Key--Begin\n");
	     report_key = 1;
	     /*  调用系统api  上报(  方向: 右)  */	
       input_report_key(jogball_input_dev, KEY_RIGHT, 1);
	     input_report_key(jogball_input_dev, KEY_RIGHT, 0);
	     //pr_info(TraceBall_TAG "Enter into jogball_key_interrupt()--Report RIGHT Key--End\n");
		 
	}

	if(report_key) 
	{
	   num_up_irq = 0;
	   num_down_irq = 0;
	   num_left_irq = 0;
	   num_right_irq = 0;
	   report_key = 0;
	}

	//enable all the gpio irq
  //enable_irq(GPIO_JOGBALL_UP_INT);
	//enable_irq(GPIO_JOGBALL_LEFT_INT);
	//enable_irq(GPIO_JOGBALL_DOWN_INT);
  //enable_irq(GPIO_JOGBALL_RIGHT_INT);
	
	return IRQ_HANDLED;
	 
}

//ZTE_XYC_PM_20100803
#ifdef CONFIG_HAS_EARLYSUSPEND
static void msmjb_early_suspend(struct early_suspend *h)
{
		disable_irq(GPIO_JOGBALL_UP_INT);
		disable_irq(GPIO_JOGBALL_DOWN_INT);
		disable_irq(GPIO_JOGBALL_LEFT_INT);
		disable_irq(GPIO_JOGBALL_RIGHT_INT);
}

static void msmjb_early_resume(struct early_suspend *h)
{
		enable_irq(GPIO_JOGBALL_UP_INT);
		enable_irq(GPIO_JOGBALL_DOWN_INT);
		enable_irq(GPIO_JOGBALL_LEFT_INT);
		enable_irq(GPIO_JOGBALL_RIGHT_INT);
}
#endif

static int __devinit jogball_key_probe(struct platform_device *pdev)
{
  int result;
	struct input_dev *input_dev;
	struct jogball_driver_data *dd_jogball;
	struct jogball_key_platform_data *pd_jogball = pdev->dev.platform_data;
	//pr_info(TraceBall_TAG "Enter into jogball_key_probe()\n");
  dd_jogball = kzalloc(sizeof(struct jogball_driver_data), GFP_KERNEL);
 
	input_dev = input_allocate_device();

	if (!input_dev || !dd_jogball) {
	      result = -ENOMEM;
	      goto fail_alloc_mem;
	}
	
      platform_set_drvdata(pdev,dd_jogball);

      dd_jogball->gpio_irq_up = pd_jogball->gpio_irq_up;
      dd_jogball->gpio_irq_down = pd_jogball->gpio_irq_down;
      dd_jogball->gpio_irq_left = pd_jogball->gpio_irq_left;
      dd_jogball->gpio_irq_right = pd_jogball->gpio_irq_right;

      input_dev->name = "jogball_key";
      //input_dev->id.bustype = BUS_HOST;
      //input_dev->id.vendor = 0x0001;
      //input_dev->id.product = 0x0002;
      //input_dev->id.version = 0x0100;

      //__set_bit(EV_KEY,    input_dev->keybit);

      input_set_capability(input_dev,EV_KEY,KEY_UP);
      input_set_capability(input_dev,EV_KEY,KEY_DOWN);
      input_set_capability(input_dev,EV_KEY,KEY_LEFT);
      input_set_capability(input_dev,EV_KEY,KEY_RIGHT);

      result = input_register_device(input_dev);
		
	if (result) {
             goto fail_ip_reg;
	}

	dd_jogball->jogball_input_dev = input_dev;

       result = request_irq(GPIO_JOGBALL_UP_INT, jogball_key_interrupt, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,"jogball_key", dd_jogball); 
       if (result)
		goto fail_req_irq;
       
       result = request_irq(GPIO_JOGBALL_DOWN_INT, jogball_key_interrupt, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,"jogball_key", dd_jogball); 
       if (result)
		goto fail_req_irq;
       
       result = request_irq(GPIO_JOGBALL_LEFT_INT, jogball_key_interrupt, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,"jogball_key", dd_jogball); 
       if (result)
		goto fail_req_irq;
       
       result = request_irq(GPIO_JOGBALL_RIGHT_INT, jogball_key_interrupt, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,"jogball_key", dd_jogball); 
       if (result)
		goto fail_req_irq;

	//platform_set_drvdata(pdev,dd_jogball);

//ZTE_XYC_PM_20100803
#ifdef CONFIG_HAS_EARLYSUSPEND
	dd_jogball->earlysuspend.suspend = msmjb_early_suspend;
	dd_jogball->earlysuspend.resume = msmjb_early_resume;
	dd_jogball->earlysuspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 2;
	register_early_suspend(&dd_jogball->earlysuspend);
#endif

	return 0;
	
fail_req_irq:
	
fail_ip_reg:
        //pr_info(TraceBall_TAG "Enter into jogball_key_probe()--fail_ip_reg\n");
	      input_unregister_device(input_dev);
	      input_dev = NULL;
fail_alloc_mem:
        //pr_info(TraceBall_TAG "Enter into jogball_key_probe()--fail_alloc_mem\n");
        input_free_device(input_dev);
	      kfree(dd_jogball);
	      return result;

}

static int __devexit jogball_key_remove(struct platform_device *pdev)
{
	struct jogball_driver_data *dd_jogball = platform_get_drvdata(pdev);

	free_irq(GPIO_JOGBALL_UP_INT, dd_jogball);
	free_irq(GPIO_JOGBALL_DOWN_INT, dd_jogball);
	free_irq(GPIO_JOGBALL_LEFT_INT, dd_jogball);
	free_irq(GPIO_JOGBALL_RIGHT_INT, dd_jogball);

	input_unregister_device(dd_jogball->jogball_input_dev);

	platform_set_drvdata(pdev, NULL);
	
	kfree(dd_jogball);

	return 0;
}
/*ZTE_HYJ_PM_20100524 begin*/
#ifdef CONFIG_PM
int jogball_suspend(struct platform_device *pdev, pm_message_t state)
{
	disable_irq(GPIO_JOGBALL_UP_INT);
	disable_irq(GPIO_JOGBALL_DOWN_INT);
	disable_irq(GPIO_JOGBALL_LEFT_INT);
	disable_irq(GPIO_JOGBALL_RIGHT_INT);
	return 0;
}

int jogball_resume(struct platform_device *pdev)
{
	enable_irq(GPIO_JOGBALL_UP_INT);
	enable_irq(GPIO_JOGBALL_DOWN_INT);
	enable_irq(GPIO_JOGBALL_LEFT_INT);
	enable_irq(GPIO_JOGBALL_RIGHT_INT);
	return 0;
}
#endif /* CONFIG_PM */
/*ZTE_HYJ_PM_20100524 end*/

static struct platform_driver jogball_key_driver = {
	.probe		= jogball_key_probe,
	.remove		= __devexit_p(jogball_key_remove),
//ZTE_XYC_PM_20100803
/*
#ifdef CONFIG_PM
	.suspend		= jogball_suspend,
	.resume		= jogball_resume,
#endif *//* CONFIG_PM */

	.driver		= {
		.name = "jogball_key",
		.owner = THIS_MODULE,
	},
};

static int __init jogball_key_init(void)
{      
       return platform_driver_register(&jogball_key_driver);
}

static void __exit jogball_key_exit(void) 
{
	platform_driver_unregister(&jogball_key_driver);
}

module_init(jogball_key_init);
module_exit(jogball_key_exit);

