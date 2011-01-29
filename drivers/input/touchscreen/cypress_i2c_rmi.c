/* drivers/input/keyboard/cypress_i2c_rmi.c
 *
 * Copyright (C) 2007 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
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
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#if defined (CONFIG_DOWNLOAD_FIRMWARE)  
#include "issp_extern.h"
#include "issp_defs.h"
#include <linux/wakelock.h>
#endif

#if defined(CONFIG_MACH_R750)
#define LCD_MAX_X   320
#define LCD_MAX_Y   480
#elif  defined(CONFIG_MACH_JOE)
#define LCD_MAX_X   240
#define LCD_MAX_Y   400
#elif  defined(CONFIG_MACH_BLADE)
#define TOUCHSCREEN_DUPLICATED_FILTER
#define LCD_MAX_X   480
#define LCD_MAX_Y   800
#endif

#define INT_POLLING
#define CYPRESS_GESTURE
#define PROC_INTERFACE
#define PINCH_VALUE 10

#if defined(CONFIG_MACH_R750)
#define TS_KEY_REPORT 
#endif

static struct workqueue_struct *cypress_wq;
static struct i2c_driver cypress_ts_driver;

#if defined(CONFIG_MACH_BLADE)
#define GPIO_TOUCH_EN_OUT  31
#elif defined(CONFIG_MACH_R750)
#define GPIO_TOUCH_EN_OUT  33
#else
#define GPIO_TOUCH_EN_OUT  31
#endif

#ifdef CYPRESS_GESTURE
#define EVENT_SINGLE_CLICK		0x21
#define EVENT_TAP_HOLD			0x22
#define EVENT_DOUBLE_CLICK		0x23
#define EVENT_EARLY_TAP			0x24
#define EVENT_FLICK					0x25
#define EVENT_PRESS				0x26
#define EVENT_PINCH 				0x27

#define SINGLE_CLICK 		(1 << 0)
#define TAP_AND_HOLD 		(1 << 1)
#define DOUBLE_CLICK 		(1 << 2)
#define EARLY_TAP 			(1 << 3)
#define PRESS					(1 << 5)

#define ST_NORTH				0x10
#define ST_SOUTH				0x18
#define ST_WEST				0x1C
#define ST_EAST				0x14

#define FLICK_RIGHT			1			
#define FLICK_LEFT			2
#define FLICK_UP				3
#define FLICK_DOWN			4
#endif
static uint8_t firmware_version = 0;
struct cypress_ts_data *cypress_td;
extern int update_flag;
int update_result_flag;
struct cypress_ts_data
{
	uint16_t addr;
	struct i2c_client *client;
	struct input_dev *input_dev;
	int use_irq;
	struct hrtimer timer;
	struct work_struct  work;
	uint16_t max[2];
	struct early_suspend early_suspend;
	uint32_t dup_threshold;
};

#ifdef CONFIG_HAS_EARLYSUSPEND
static void cypress_ts_early_suspend(struct early_suspend *h);
static void cypress_ts_late_resume(struct early_suspend *h);
#endif

#ifdef TS_KEY_REPORT
const char ts_keys_size[] = "0x01:102:50:504:100:50:0x01:139:160:504:100:50:0x01:158:270:504:100:50";

struct attribute ts_key_report_attr = {
        .name = "virtualkeys.cypress_touch",
        .mode = S_IRWXUGO,
};
 
static struct attribute *def_attrs[] = {
        &ts_key_report_attr,
        NULL,
};
 
void ts_key_report_release(struct kobject *kobject)
{
        return;
}
 
ssize_t ts_key_report_show(struct kobject *kobject, struct attribute *attr,char *buf)
{
        sprintf(buf,"%s\n",ts_keys_size);
        return strlen(ts_keys_size)+2;
}
 
ssize_t ts_key_report_store(struct kobject *kobject,struct attribute *attr,const char *buf, size_t count)
{
        return count;
}
 
struct sysfs_ops ts_key_report_sysops =
{
        .show = ts_key_report_show,
        .store = ts_key_report_store,
};
 
struct kobj_type ktype = 
{
        .release = ts_key_report_release,
        .sysfs_ops=&ts_key_report_sysops,
        .default_attrs=def_attrs,
};
 
struct kobject kobj;
static void ts_key_report_init(void)
{
	int ret = 0;
        ret = kobject_init_and_add(&kobj,&ktype,NULL,"board_properties");
	if(ret)
		printk(KERN_ERR "ts_key_report_init: Unable to init and add the kobject\n");
}
#endif

#ifdef CYPRESS_GESTURE
static  int judge_flick_direction(int flick_direction)
{
    if(flick_direction == ST_NORTH)
        return FLICK_UP;
    else if(flick_direction == ST_SOUTH)
	 return FLICK_DOWN;
    else if(flick_direction == ST_WEST)
	 return FLICK_LEFT;
    else if(ST_EAST)
	 return FLICK_RIGHT;
    return 0;
} 
#endif

static int cypress_i2c_read(struct i2c_client *client, int reg, u8 * buf, int count)
{
    int rc;
    int ret = 0;

    buf[0] = reg;
    rc = i2c_master_send(client, buf, 1);
    if (rc != 1)
    {
        dev_err(&client->dev, "cypress_i2c_read fail: read of register %d\n", reg);
        ret = -1;
        goto tp_i2c_rd_exit;
    }
    rc = i2c_master_recv(client, buf, count);
    if (rc != count)
    {
        dev_err(&client->dev, "cypress_i2c_read fail: read %d bytes from reg %d\n", count, reg);
        ret = -1;
    }

  tp_i2c_rd_exit:
    return ret;
}
static int cypress_i2c_write(struct i2c_client *client, int reg, u8 data)
{
    u8 buf[2];
    int rc;
    int ret = 0;

    buf[0] = reg;
    buf[1] = data;
    rc = i2c_master_send(client, buf, 2);
    if (rc != 2)
    {
        dev_err(&client->dev, "cypress_i2c_write fail: writing to reg %d\n", reg);
        ret = -1;
    }
    return ret;
}

#ifdef TOUCHSCREEN_DUPLICATED_FILTER
static int duplicated_filter(struct cypress_ts_data *ts, int x,int y,int x2,int y2,
						const int finger2, const int z)
{
	int drift_x[2];
	int drift_y[2];
	static int ref_x[2], ref_y[2];
	uint8_t discard[2] = {0, 0};

	drift_x[0] = abs(ref_x[0] - x);
	drift_y[0] = abs(ref_y[0] - y);
	if (finger2) {
		drift_x[1] = abs(ref_x[1] - x2);
		drift_y[1] = abs(ref_y[1] - y2);
	}
	/* printk("ref_x :%d, ref_y: %d, x: %d, y: %d\n", ref_x, ref_y, pos[0][0], pos[0][1]); */
	if (drift_x[0] < ts->dup_threshold && drift_y[0] < ts->dup_threshold && z != 0) {
		/* printk("ref_x :%d, ref_y: %d, x: %d, y: %d\n", ref_x[0], ref_y[0], pos[0][0], pos[0][1]); */
		discard[0] = 1;
	}
	if (!finger2 || (drift_x[1] < ts->dup_threshold && drift_y[1] < ts->dup_threshold)) {
		discard[1] = 1;
	}
	if (discard[0] && discard[1]) {
		/* if finger 0 and finger 1's movement < threshold , discard it. */
		return 1;
		printk("*");
	}
	ref_x[0] = x;
	ref_y[0] = y;
	if (finger2) {
		ref_x[1] = x2;
		ref_y[1] = y2;
	}
	if (z == 0) {
		ref_x[0] = ref_y[0] = 0;
		ref_x[1] = ref_y[1] = 0;
	}

	return 0;
}
#endif /* TOUCHSCREEN_DUPLICATED_FILTER */


static void cypress_ts_work_func(struct work_struct *work)
{
    int ret = 0;
    int retry = 20;
    int finger2=0;
    int x1 = 0, y1 = 0, flick_direction = 0, report_direction = 0, gesture = 0, pressure = 0, pinch_value = 0,x2 = 0, y2 = 0;
    uint8_t buf[15];
    struct cypress_ts_data *ts = container_of(work, struct cypress_ts_data, work);
    //ret = cypress_i2c_read(ts->client, 0x00, buf, 14);
    while (retry-- > 0)
    {
    	ret = cypress_i2c_read(ts->client, 0x00, buf, 14);    /* go to sleep */
	    if (ret >= 0)
    	  break;	
    	msleep(10);
    }
    if (ret < 0)
		{
        printk(KERN_ERR "cypress_ts_work_func: cypress_i2c_read failed, go to poweroff.\n");
        gpio_direction_output(GPIO_TOUCH_EN_OUT, 0);
        msleep(200);
        gpio_direction_output(GPIO_TOUCH_EN_OUT, 1);
        msleep(200);
   	}
    buf[14] =0;
    if (0 == firmware_version)firmware_version = buf[12];
		//printk("cypress_ts_work_func,buf[0] =%d\n", buf[0]);
    if(buf[0] == 0x80)
		  cypress_i2c_write(ts->client, 0x0b, 0x80);
    else  
    	pressure = 255;

    x1=buf[2] | (uint16_t)(buf[1] & 0x03) << 8; 
	  y1=buf[4] | (uint16_t)(buf[3] & 0x03) << 8; 
    x2=buf[6] | (uint16_t)(buf[5] & 0x03) << 8; 
    y2=buf[8] | (uint16_t)(buf[7] & 0x03) << 8;
	if(buf[0] == 0x82) 
		finger2=1;

#ifdef TOUCHSCREEN_DUPLICATED_FILTER
	ret = duplicated_filter(ts, x1,y1,x2,y2, finger2, pressure);
	if (ret == 0) 
	{
		/* printk("%s: duplicated_filter\n", __func__); */

#endif

		if (((buf[0] == 0x81)&&(x1 == x2)&&(y1 == y2))||finger2 ||(pressure == 0))
		{
			input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, pressure);
			input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 10);			
			input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x1);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y1);

			input_mt_sync(ts->input_dev);
		}
		if(finger2)
		{
			input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, pressure);
			input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 10);			
			input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x2);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y2);
			input_mt_sync(ts->input_dev);
		}
		input_sync(ts->input_dev);
		
#ifdef TOUCHSCREEN_DUPLICATED_FILTER
	}	
#endif	
		
		#ifdef CYPRESS_GESTURE
    if(buf[14])
    {
        flick_direction = buf[9];
	 			gesture = buf[13];
	 			cypress_i2c_write(ts->client, 0x0e, 0);
    }
		//if(( 0x48 == buf[9])||( 0x49 == buf[9]))pinch_value = buf[9];
		if( 0x48 == buf[9])pinch_value = PINCH_VALUE;
		if( 0x49 == buf[9])pinch_value = -PINCH_VALUE;
    report_direction = judge_flick_direction(flick_direction);
    input_report_abs(ts->input_dev, EVENT_PINCH, pinch_value);
    input_sync(ts->input_dev);
    
		#endif

    if (ts->use_irq)
       enable_irq(ts->client->irq);

}

static enum hrtimer_restart cypress_ts_timer_func(struct hrtimer *timer)
{
	struct cypress_ts_data *ts = container_of(timer, struct cypress_ts_data, timer);
	/* printk("cypress_ts_timer_func\n"); */
	queue_work(cypress_wq, &ts->work);

	hrtimer_start(&ts->timer, ktime_set(0, 12500000), HRTIMER_MODE_REL);
	return HRTIMER_NORESTART;
}
static irqreturn_t cypress_ts_irq_handler(int irq, void *dev_id)
{
	struct cypress_ts_data *ts = dev_id;

	disable_irq_nosync(ts->client->irq);
	queue_work(cypress_wq, &ts->work);
	return IRQ_HANDLED;
}

static int
proc_read_val(char *page, char **start, off_t off, int count, int *eof,
	  void *data)
{
	int len = 0;
	len += sprintf(page + len, "%s\n", "touchscreen module");
	len += sprintf(page + len, "name     : %s\n", "cypress");
	len += sprintf(page + len, "i2c address  : 0x%x\n", 0x0a);
	len += sprintf(page + len, "IC type    : %s\n", "301e");
	len += sprintf(page + len, "firmware version    : 0x%x\n", firmware_version);
	#if defined(CONFIG_MACH_JOE)
	len += sprintf(page + len, "module : %s\n", "cypress + goworld-lcd");
	#else
	len += sprintf(page + len, "module : %s\n", "cypress + winteck");
  #endif
  len += sprintf(page + len, "update flag : 0x%x\n", update_result_flag);
	if (off + count >= len)
		*eof = 1;
	if (len < off)
		return 0;
	*start = page + off;
	return ((count < len - off) ? count : len - off);
}

#if defined (CONFIG_DOWNLOAD_FIRMWARE)
void init(void)
{
	disable_irq(MSM_GPIO_TO_INT(29));
	gpio_direction_output(29, 0);
	//gpio_tlmm_config(GPIO_CFG(GPIO_TOUCH_EN_OUT, 0, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_16MA), GPIO_ENABLE);
	gpio_direction_output(GPIO_TOUCH_EN_OUT, 1);
	gpio_tlmm_config(GPIO_CFG(SCLK_GPIO, 0, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_16MA), GPIO_ENABLE);
	gpio_tlmm_config(GPIO_CFG(SDATA_GPIO, 0, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_16MA), GPIO_ENABLE);
}

void reset (void)
{
	gpio_tlmm_config(GPIO_CFG(SCLK_GPIO, 1, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
	gpio_tlmm_config(GPIO_CFG(SDATA_GPIO, 1, GPIO_INPUT, GPIO_NO_PULL, GPIO_2MA), GPIO_ENABLE);
	gpio_direction_output(GPIO_TOUCH_EN_OUT, 0);
	msleep(500);
	gpio_direction_output(GPIO_TOUCH_EN_OUT, 1);
	msleep(2000);
	gpio_direction_input(29);
	enable_irq(MSM_GPIO_TO_INT(29));
}
#endif
static int proc_write_val(struct file *file, const char *buffer,
           unsigned long count, void *data)
{
	char *buf;
	#if defined (CONFIG_DOWNLOAD_FIRMWARE) 
  struct wake_lock ts_fwup_wake_lock;
  char *tgt_fw = "/system/etc/cypress_firmware.bin";   
  update_flag=0;
  update_result_flag=0;	
  firmware_version=0;	   
  #endif   
	buf = kmalloc(count + 1, GFP_KERNEL);
	if (buf == NULL)
		return -ENOMEM;

	if (copy_from_user(buf, buffer, count)) {
		count = -EFAULT;
		goto out;
	}
	buf[count] = '\0';
	#if defined (CONFIG_DOWNLOAD_FIRMWARE) 
	init();
	wake_lock_init(&ts_fwup_wake_lock, WAKE_LOCK_SUSPEND, "ts_fwup");
	wake_lock(&ts_fwup_wake_lock);	
	download_firmware_main(tgt_fw);
	wake_unlock(&ts_fwup_wake_lock);
	wake_lock_destroy(&ts_fwup_wake_lock);
	reset();	
	update_result_flag=update_flag;
	#endif
	return count;
out:
	kfree(buf);
	return -EFAULT;
	
}

static int cypress_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct cypress_ts_data *ts;
     //uint8_t buf0[4];
     uint8_t buf1[13];
     //	struct i2c_msg msg[2];
     int ret = 0;
     uint16_t max_x, max_y;
     // struct rmi_i2c_data *pdata;
	   struct proc_dir_entry *dir,*refresh;
	   ret = gpio_request(GPIO_TOUCH_EN_OUT, "touch voltage");
	if (ret)
	{
		printk("gpio 31 request is error!\n");
		goto err_check_functionality_failed;
	}	   
    gpio_direction_output(GPIO_TOUCH_EN_OUT, 1);
    msleep(20);
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
    {
        printk(KERN_ERR "cypress_ts_probe: need I2C_FUNC_I2C\n");
        ret = -ENODEV;
        goto err_check_functionality_failed;
    }
    ts = kzalloc(sizeof(*ts), GFP_KERNEL);
    if (ts == NULL)
    {
        ret = -ENOMEM;
        goto err_alloc_data_failed;
    }

    INIT_WORK(&ts->work, cypress_ts_work_func);
    ts->client = client;
    
    #ifdef TOUCHSCREEN_DUPLICATED_FILTER	
    ts->dup_threshold=1; 
    #endif
	
    i2c_set_clientdata(client, ts);
    client->driver = &cypress_ts_driver;
    {
        int retry = 3;
		
        while (retry-- > 0)
        {
            ret = cypress_i2c_read(ts->client, 0x00, buf1, 13);
	     if (ret >= 0)
    	     break;	
    	       msleep(10);
        }
	 if (retry < 0)
	 {
	     ret = -1;
	     goto err_detect_failed;
	 }
    }
    firmware_version = buf1[12];
    ts->input_dev = input_allocate_device();

    if (ts->input_dev == NULL)
    {
        ret = -ENOMEM;
        printk(KERN_ERR "cypress_ts_probe: Failed to allocate input device\n");
        goto err_input_dev_alloc_failed;
    }
    // cypress_td = ts;
    ts->input_dev->name = "cypress_touch";
    ts->input_dev->phys = "cypress_touch/input0";
/*
    ts->input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
    ts->input_dev->absbit[0] = BIT(ABS_X) | BIT(ABS_Y) | BIT(ABS_PRESSURE);
    ts->input_dev->absbit[BIT_WORD(ABS_MISC)] = BIT_MASK(ABS_MISC);
    ts->input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
*/
    set_bit(EV_SYN, ts->input_dev->evbit);
    set_bit(EV_KEY, ts->input_dev->evbit);
    set_bit(BTN_TOUCH, ts->input_dev->keybit);
    set_bit(EV_ABS, ts->input_dev->evbit);
	set_bit(ABS_MT_TOUCH_MAJOR, ts->input_dev->absbit);
	set_bit(ABS_MT_POSITION_X, ts->input_dev->absbit);
	set_bit(ABS_MT_POSITION_Y, ts->input_dev->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, ts->input_dev->absbit);
#ifdef CYPRESS_GESTURE
    set_bit(EVENT_SINGLE_CLICK, ts->input_dev->absbit);
    set_bit(EVENT_TAP_HOLD, ts->input_dev->absbit);
    set_bit(EVENT_EARLY_TAP, ts->input_dev->absbit);
    set_bit(EVENT_FLICK, ts->input_dev->absbit);
    set_bit(EVENT_PRESS, ts->input_dev->absbit);
     set_bit(EVENT_PINCH, ts->input_dev->absbit);
#endif
	
#if defined(CONFIG_MACH_BLADE)
		max_x=479;
    max_y=799;
#elif defined(CONFIG_MACH_R750)
    max_x=319;
    max_y=479;
#else
    max_x=319;
    max_y=479;
#endif
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, max_x, 0, 0);
    input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, max_y, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
#ifdef CYPRESS_GESTURE
    input_set_abs_params(ts->input_dev, EVENT_SINGLE_CLICK, 0, 255, 0, 0);
    input_set_abs_params(ts->input_dev, EVENT_TAP_HOLD, 0, 255, 0, 0);
    input_set_abs_params(ts->input_dev, EVENT_EARLY_TAP, 0, 255, 0, 0);
    input_set_abs_params(ts->input_dev, EVENT_FLICK, 0, 255, 0, 0);
    input_set_abs_params(ts->input_dev, EVENT_PRESS, 0, 128, 0, 0);
    input_set_abs_params(ts->input_dev, EVENT_PINCH, 0, 255, 0, 0);
#endif

    ret = input_register_device(ts->input_dev);
    if (ret)
    {
        printk(KERN_ERR "cypress_ts_probe: Unable to register %s input device\n", ts->input_dev->name);
        goto err_input_register_device_failed;
    }
   	cypress_td = ts;
    if (client->irq)
    {
        ret = request_irq(ts->client->irq, cypress_ts_irq_handler, IRQF_TRIGGER_FALLING, "cypress_touch", ts);
        if (ret == 0) ts->use_irq = 1;
    }
    if (!ts->use_irq)
    {
        hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
        ts->timer.function = cypress_ts_timer_func;
        hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
    }
#ifdef CONFIG_HAS_EARLYSUSPEND
    ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
    ts->early_suspend.suspend = cypress_ts_early_suspend;
    ts->early_suspend.resume = cypress_ts_late_resume;
    register_early_suspend(&ts->early_suspend);
#endif

    printk(KERN_INFO "cypress_ts_probe: Start touchscreen %s in %s mode\n", ts->input_dev->name,
           ts->use_irq ? "interrupt" : "polling");
    dir = proc_mkdir("touchscreen", NULL);
    refresh = create_proc_entry("ts_information", 0777, dir);
	  if (refresh) {
	 		refresh->data		= NULL;
			refresh->read_proc  = proc_read_val;
			refresh->write_proc = proc_write_val;	
	}

#ifdef TS_KEY_REPORT
	ts_key_report_init();
#endif

    return 0;
err_input_register_device_failed:
    input_unregister_device(ts->input_dev);

err_input_dev_alloc_failed:
	input_free_device(ts->input_dev);
err_detect_failed:
//err_power_failed:
    kfree(ts);
err_alloc_data_failed:
err_check_functionality_failed:
gpio_free(GPIO_TOUCH_EN_OUT);	
    return ret;
}

static int  cypress_ts_remove(struct i2c_client *client)
{
	struct cypress_ts_data *ts = i2c_get_clientdata(client);
	unregister_early_suspend(&ts->early_suspend);
	if (ts->use_irq)
		free_irq(ts->client->irq, ts);
	else
		hrtimer_cancel(&ts->timer);
	input_unregister_device(ts->input_dev);
	kfree(ts);
	 gpio_direction_output(GPIO_TOUCH_EN_OUT, 0);
	return 0;
}
static int deep_sleep = 0;
static int cypress_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int ret;
	int retry = 20;
	struct cypress_ts_data *ts = i2c_get_clientdata(client);
	if (ts->use_irq)
		disable_irq(ts->client->irq);
	else
		hrtimer_cancel(&ts->timer);
	ret = cancel_work_sync(&ts->work);
	if (ret && ts->use_irq) /* if work was pending disable-count is now 2 */
		enable_irq(ts->client->irq);
	while (retry-- > 0)
  {
    ret = cypress_i2c_write(ts->client, 0x0a, 0x08);     /* go to sleep */
	  if (ret >= 0)
    break;	
    msleep(10);
  }

	if (ret < 0)
	{
        printk(KERN_ERR "cypress_ts_suspend: cypress_i2c_write failed,go to poweroff\n");
        gpio_direction_output(GPIO_TOUCH_EN_OUT, 0);
        deep_sleep = 0;
        gpio_direction_output(29, 0);
   }
   else 
   {
   			gpio_direction_output(29, 1);
   			deep_sleep =1;
   }
	return 0;
}
static int cypress_ts_resume(struct i2c_client *client)
{
    struct cypress_ts_data *ts = i2c_get_clientdata(client);
	if (deep_sleep){
	gpio_direction_output(29, 0);
	msleep(5);
	gpio_direction_input(29);
		}
		else 
		{
			gpio_direction_output(GPIO_TOUCH_EN_OUT, 1);
  		msleep(200);
  		gpio_direction_input(29);
}
	if (ts->use_irq)
		enable_irq(ts->client->irq);
    if (!ts->use_irq)
        hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void cypress_ts_early_suspend(struct early_suspend *h)
{
	struct cypress_ts_data *ts;
	ts = container_of(h, struct cypress_ts_data, early_suspend);
	cypress_ts_suspend(ts->client, PMSG_SUSPEND);
}

static void cypress_ts_late_resume(struct early_suspend *h)
{
	struct cypress_ts_data *ts;
	ts = container_of(h, struct cypress_ts_data, early_suspend);
	cypress_ts_resume(ts->client);
}
#endif
static const struct i2c_device_id cypress_touch_id[] = {
	{ "cypress_touch", 0},
	{ },
};
static struct i2c_driver cypress_ts_driver = {

	   .probe		= cypress_ts_probe,
       .remove      = cypress_ts_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	   .suspend	    = cypress_ts_suspend,
	   .resume		= cypress_ts_resume,
#endif
       .id_table    = cypress_touch_id,
	   .driver      = {
                     	.name   = "cypress_touch",
	             		.owner  = THIS_MODULE,
					},
};
static int __devinit cypress_ts_init(void)
{
	cypress_wq = create_rt_workqueue("cypress_wq");
	if (!cypress_wq)
		return -ENOMEM;
	return i2c_add_driver(&cypress_ts_driver);
}

static void __exit cypress_ts_exit(void)
{
	i2c_del_driver(&cypress_ts_driver);
        if (cypress_wq)
		destroy_workqueue(cypress_wq);
}

module_init(cypress_ts_init);
module_exit(cypress_ts_exit);

MODULE_DESCRIPTION("Cypress Touchscreen Driver");
MODULE_LICENSE("GPL");
