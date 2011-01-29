/* drivers/input/keyboard/synaptics_i2c_rmi.c
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
#include <linux/proc_fs.h>
#include <linux/platform_device.h>
#include <linux/synaptics_i2c_rmi.h>
#include <mach/gpio.h>

#if defined(CONFIG_MACH_BLADE)
#define GPIO_TOUCH_EN_OUT  31
#elif defined(CONFIG_MACH_R750)
#define GPIO_TOUCH_EN_OUT  33
#else
#define GPIO_TOUCH_EN_OUT  31
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


#if defined(CONFIG_MACH_R750)
#define TS_KEY_REPORT 
#endif

#define ABS_SINGLE_TAP	0x21	/* Major axis of touching ellipse */
#define ABS_TAP_HOLD	0x22	/* Minor axis (omit if circular) */
#define ABS_DOUBLE_TAP	0x23	/* Major axis of approaching ellipse */
#define ABS_EARLY_TAP	0x24	/* Minor axis (omit if circular) */
#define ABS_FLICK	0x25	/* Ellipse orientation */
#define ABS_PRESS	0x26	/* Major axis of touching ellipse */
#define ABS_PINCH 	0x27	/* Minor axis (omit if circular) */
#define sigle_tap  (1 << 0)
#define tap_hold   (1 << 1)
#define double_tap (1 << 2)
#define early_tap  (1 << 3)
#define flick      (1 << 4)
#define press      (1 << 5)
#define pinch      (1 << 6)
unsigned long polling_time = 12500000;

static struct workqueue_struct *synaptics_wq;
static struct i2c_driver synaptics_ts_driver;
#define POLL_IN_INT   
#if defined (POLL_IN_INT)
#undef POLL_IN_INT
#endif

struct synaptics_ts_data
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
static void synaptics_ts_early_suspend(struct early_suspend *h);
static void synaptics_ts_late_resume(struct early_suspend *h);
#endif

#ifdef TS_KEY_REPORT
const char ts_keys_size_synaptics[] = "0x01:102:51:503:102:1007:0x01:139:158:503:102:1007:0x01:158:266:503:102:1007";
struct attribute ts_key_report_attr_synaptics = {
        .name = "virtualkeys.synaptics-rmi-touchscreen",
        .mode = S_IRWXUGO,
};
 
static struct attribute *def_attrs_synaptics[] = {
        &ts_key_report_attr_synaptics,
        NULL,
};
 
void ts_key_report_synaptics_release(struct kobject *kobject)
{
        return;
}
 
ssize_t ts_key_report_synaptics_show(struct kobject *kobject, struct attribute *attr,char *buf)
{
        sprintf(buf,"%s\n",ts_keys_size_synaptics);
        return strlen(ts_keys_size_synaptics)+2;
}
 
ssize_t ts_key_report_synaptics_store(struct kobject *kobject,struct attribute *attr,const char *buf, size_t count)
{
        return count;
}
 
struct sysfs_ops ts_key_report_sysops_synaptics =
{
        .show = ts_key_report_synaptics_show,
        .store = ts_key_report_synaptics_store,
};
 
struct kobj_type ktype_synaptics = 
{
        .release = ts_key_report_synaptics_release,
        .sysfs_ops=&ts_key_report_sysops_synaptics,
        .default_attrs=def_attrs_synaptics,
};
 
struct kobject kobj_synaptics;
static void ts_key_report_synaptics_init(void)
{
	int ret = 0;
        ret = kobject_init_and_add(&kobj_synaptics,&ktype_synaptics,NULL,"board_properties");
	if(ret)
		printk(KERN_ERR "ts_key_report_init: Unable to init and add the kobject\n");
}
#endif

static int synaptics_i2c_read(struct i2c_client *client, int reg, u8 * buf, int count)
{
    int rc;
    int ret = 0;

    buf[0] = reg;
    rc = i2c_master_send(client, buf, 1);
    if (rc != 1)
    {
        dev_err(&client->dev, "synaptics_i2c_read FAILED: read of register %d\n", reg);
        ret = -1;
        goto tp_i2c_rd_exit;
    }
    rc = i2c_master_recv(client, buf, count);
    if (rc != count)
    {
        dev_err(&client->dev, "synaptics_i2c_read FAILED: read %d bytes from reg %d\n", count, reg);
        ret = -1;
    }

  tp_i2c_rd_exit:
    return ret;
}
static int synaptics_i2c_write(struct i2c_client *client, int reg, u8 data)
{
    u8 buf[2];
    int rc;
    int ret = 0;

    buf[0] = reg;
    buf[1] = data;
    rc = i2c_master_send(client, buf, 2);
    if (rc != 2)
    {
        dev_err(&client->dev, "synaptics_i2c_write FAILED: writing to reg %d\n", reg);
        ret = -1;
    }
    return ret;
}


static int
proc_read_val(char *page, char **start, off_t off, int count, int *eof,
	  void *data)
{
	int len = 0;
	len += sprintf(page + len, "%s\n", "touchscreen module");
	len += sprintf(page + len, "name     : %s\n", "synaptics");
	#if defined(CONFIG_MACH_R750)
	len += sprintf(page + len, "i2c address  : %x\n", 0x23);
	#else
	len += sprintf(page + len, "i2c address  : 0x%x\n", 0x22);
	#endif
	len += sprintf(page + len, "IC type    : %s\n", "2000 series");
	#if defined(CONFIG_MACH_R750)
	len += sprintf(page + len, "firmware version    : %s\n", "TM1551");
	#elif  defined(CONFIG_MACH_JOE)
	len += sprintf(page + len, "firmware version    : %s\n", "TM1419-001");
	#elif  defined(CONFIG_MACH_BLADE)
	len += sprintf(page + len, "firmware version    : %s\n", "TM1541");
	#endif
	len += sprintf(page + len, "module : %s\n", "synaptics + TPK");
	if (off + count >= len)
		*eof = 1;
	if (len < off)
		return 0;
	*start = page + off;
	return ((count < len - off) ? count : len - off);
}

static int proc_write_val(struct file *file, const char *buffer,
           unsigned long count, void *data)
{
		unsigned long val;
		sscanf(buffer, "%lu", &val);
		if (val >= 0) {
			polling_time= val;
			return count;
		}
		return -EINVAL;
}


#ifdef TOUCHSCREEN_DUPLICATED_FILTER
static int duplicated_filter(struct synaptics_ts_data *ts, int x,int y,int x2,int y2,
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
	if (drift_x[0] < ts->dup_threshold && drift_y[0] < ts->dup_threshold && z != 0) {
		discard[0] = 1;
	}
	if (!finger2 || (drift_x[1] < ts->dup_threshold && drift_y[1] < ts->dup_threshold)) {
		discard[1] = 1;
	}
	if (discard[0] && discard[1]) {
		return 1;
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
#endif

static void synaptics_ts_work_func(struct work_struct *work)
{
	int ret, x, y, z, finger, w, x2, y2,w2,z2,finger2,pressure,pressure2;
	__s8  gesture, flick_y, flick_x, direction = 0;  
	uint8_t buf[16];
	struct synaptics_ts_data *ts = container_of(work, struct synaptics_ts_data, work);
	finger=0;
	ret = synaptics_i2c_read(ts->client, 0x14, buf, 16);
	if (ret < 0){
   	printk(KERN_ERR "synaptics_ts_work_func: synaptics_i2c_write failed, go to poweroff.\n");
    gpio_direction_output(GPIO_TOUCH_EN_OUT, 0);
    msleep(200);
    gpio_direction_output(GPIO_TOUCH_EN_OUT, 1);
    msleep(200);
  }
  else
  {
			x = (uint16_t) buf[2] << 4| (buf[4] & 0x0f) ; 
			y = (uint16_t) buf[3] << 4| ((buf[4] & 0xf0) >> 4); 
			pressure = buf[6];
			w = buf[5] >> 4;
			z = buf[5]&0x0f;
			finger = buf[1] & 0x3;
	
			x2 = (uint16_t) buf[7] << 4| (buf[9] & 0x0f) ;  
			y2 = (uint16_t) buf[8] << 4| ((buf[9] & 0xf0) >> 4); 

	#ifdef CONFIG_MACH_JOE
			y = 2787 - y;
			y2 = 2787 - y2;
	#endif		
	
			pressure2 = buf[11]; 
			w2 = buf[10] >> 4; 
			z2 = buf[10] & 0x0f;
			finger2 = buf[1] & 0xc; 
			gesture = buf[12];
	
			flick_x = buf[14];
			flick_y = buf[15];
			if((16==gesture)||(flick_x)||(flick_y))
			{
				if ((flick_x >0 )&& (abs(flick_x) > abs(flick_y))) 
				direction = 1;
				else if((flick_x <0 )&& (abs(flick_x) > abs(flick_y)))  
				direction = 2;
				else if ((flick_y >0 )&& (abs(flick_x) < abs(flick_y))) 
				direction = 3;
				else if ((flick_y <0 )&& (abs(flick_x) < abs(flick_y))) 
				direction = 4;

			}
			
#ifdef TOUCHSCREEN_DUPLICATED_FILTER
	ret = duplicated_filter(ts, x,y,x2,y2, finger2, pressure);
	if (ret == 0) 
	{
#endif
				input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, pressure);
				input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 10);
				input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
				input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);
				input_mt_sync(ts->input_dev);

			if(finger2)
			{
				input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, pressure2);			
				input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 10);
				input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x2);
				input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y2);
				input_mt_sync(ts->input_dev);
			}
			
			input_sync(ts->input_dev);
	}
#ifdef TOUCHSCREEN_DUPLICATED_FILTER
	}	  	
#endif	
		#ifdef POLL_IN_INT
		if(finger)
		{
			hrtimer_start(&ts->timer, ktime_set(0, polling_time), HRTIMER_MODE_REL);
		}
		else
		{
			hrtimer_cancel(&ts->timer);
			enable_irq(ts->client->irq);
		}
		#else
		if (ts->use_irq)
		enable_irq(ts->client->irq);
		#endif
}

static enum hrtimer_restart synaptics_ts_timer_func(struct hrtimer *timer)
{
	struct synaptics_ts_data *ts = container_of(timer, struct synaptics_ts_data, timer);

	queue_work(synaptics_wq, &ts->work);
	#ifndef POLL_IN_INT
	hrtimer_start(&ts->timer, ktime_set(0, polling_time), HRTIMER_MODE_REL);
	#endif
	return HRTIMER_NORESTART;
}

static irqreturn_t synaptics_ts_irq_handler(int irq, void *dev_id)
{
	struct synaptics_ts_data *ts = dev_id;

	disable_irq_nosync(ts->client->irq);
	#ifdef POLL_IN_INT
	hrtimer_start(&ts->timer, ktime_set(0, 0), HRTIMER_MODE_REL);
	#else
	queue_work(synaptics_wq, &ts->work);
	#endif
	return IRQ_HANDLED;
}

static int synaptics_ts_probe(
	struct i2c_client *client, const struct i2c_device_id *id)
{
	struct synaptics_ts_data *ts;
	uint8_t buf1[9];
	int ret = 0;
	uint16_t max_x, max_y;
	struct proc_dir_entry *dir, *refresh;
  ret = gpio_request(GPIO_TOUCH_EN_OUT, "touch voltage");
	if (ret)
	{	
		printk("gpio 31 request is error!\n");
		goto err_check_functionality_failed;
	}   
	gpio_direction_output(GPIO_TOUCH_EN_OUT, 1);
	msleep(250);
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
    {
		printk(KERN_ERR "synaptics_ts_probe: need I2C_FUNC_I2C\n");
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}
	ts = kzalloc(sizeof(*ts), GFP_KERNEL);
    if (ts == NULL)
    {
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}

	INIT_WORK(&ts->work, synaptics_ts_work_func);
	ts->client = client;
	
	i2c_set_clientdata(client, ts);
	client->driver = &synaptics_ts_driver;
	{
		int retry = 3;
        while (retry-- > 0)
        {
            ret = synaptics_i2c_read(ts->client, 0x78, buf1, 9);
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
    ret = synaptics_i2c_write(ts->client, 0x25, 0x00);
    ret = synaptics_i2c_read(ts->client, 0x2D, buf1, 2);
    if (ret < 0)
    {
        printk(KERN_ERR "synaptics_i2c_read failed\n");
		goto err_detect_failed;
	}
    ts->max[0] = max_x = buf1[0] | ((buf1[1] & 0x0f) << 8);
    ret = synaptics_i2c_read(ts->client, 0x2F, buf1, 2);
    if (ret < 0)
    {
        printk(KERN_ERR "synaptics_i2c_read failed\n");
		goto err_detect_failed;
	}
    ts->max[1] = max_y = buf1[0] | ((buf1[1] & 0x0f) << 8);
	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) {
		ret = -ENOMEM;
		printk(KERN_ERR "synaptics_ts_probe: Failed to allocate input device\n");
		goto err_input_dev_alloc_failed;
	}
	ts->input_dev->name = "synaptics-rmi-touchscreen";
	ts->input_dev->phys = "synaptics-rmi-touchscreen/input0";
	
	#ifdef TOUCHSCREEN_DUPLICATED_FILTER
	ts->dup_threshold=max_y/LCD_MAX_Y;
	#endif
	
	set_bit(EV_SYN, ts->input_dev->evbit);
	set_bit(EV_KEY, ts->input_dev->evbit);
	set_bit(BTN_TOUCH, ts->input_dev->keybit);
	set_bit(EV_ABS, ts->input_dev->evbit);
	set_bit(ABS_SINGLE_TAP, ts->input_dev->absbit);
	set_bit(ABS_TAP_HOLD, ts->input_dev->absbit);
	set_bit(ABS_EARLY_TAP, ts->input_dev->absbit);
	set_bit(ABS_FLICK, ts->input_dev->absbit);
	set_bit(ABS_PRESS, ts->input_dev->absbit);
	set_bit(ABS_DOUBLE_TAP, ts->input_dev->absbit);
	set_bit(ABS_PINCH, ts->input_dev->absbit);
	set_bit(ABS_MT_TOUCH_MAJOR, ts->input_dev->absbit);
	set_bit(ABS_MT_POSITION_X, ts->input_dev->absbit);
	set_bit(ABS_MT_POSITION_Y, ts->input_dev->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, ts->input_dev->absbit);

#ifdef TS_KEY_REPORT
	max_y = 2739;
#endif
	
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, max_x, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, max_y, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_SINGLE_TAP, 0, 5, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_TAP_HOLD, 0, 5, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_EARLY_TAP, 0, 5, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_FLICK, 0, 5, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_PRESS, 0, 5, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_DOUBLE_TAP, 0, 5, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_PINCH, -255, 255, 0, 0);
	ret = input_register_device(ts->input_dev);
    if (ret)
    {
		printk(KERN_ERR "synaptics_ts_probe: Unable to register %s input device\n", ts->input_dev->name);
		goto err_input_register_device_failed;
	}
	#ifdef POLL_IN_INT
	hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ts->timer.function = synaptics_ts_timer_func;
	ret = request_irq(client->irq, synaptics_ts_irq_handler, IRQF_TRIGGER_FALLING, "synaptics_touch", ts);
	if(ret == 0)
		{
		ret = synaptics_i2c_write(ts->client, 0x26, 0x07); 
		if (ret)
		free_irq(client->irq, ts);
		}
	if(ret == 0)
		ts->use_irq = 1;
	else
		dev_err(&client->dev, "request_irq failed\n");
	#else
   if (client->irq)
    {
        ret = request_irq(client->irq, synaptics_ts_irq_handler, IRQF_TRIGGER_FALLING, "synaptics_touch", ts);
		if (ret == 0) {
    ret = synaptics_i2c_write(ts->client, 0x26, 0x07);
			if (ret)
				free_irq(client->irq, ts);
		}
		if (ret == 0)
			ts->use_irq = 1;
		else
			dev_err(&client->dev, "request_irq failed\n");
	}
    if (!ts->use_irq)
    {
		hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		ts->timer.function = synaptics_ts_timer_func;
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
	}
	#endif
#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = synaptics_ts_early_suspend;
	ts->early_suspend.resume = synaptics_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif
  dir = proc_mkdir("touchscreen", NULL);
	refresh = create_proc_entry("ts_information", 0644, dir);
	if (refresh) {
		refresh->data		= NULL;
		refresh->read_proc  = proc_read_val;
		refresh->write_proc = proc_write_val;
	}
	printk(KERN_INFO "synaptics_ts_probe: Start touchscreen %s in %s mode\n", ts->input_dev->name, ts->use_irq ? "interrupt" : "polling");

#ifdef TS_KEY_REPORT
	ts_key_report_synaptics_init();
#endif

	return 0;

err_input_register_device_failed:
	input_free_device(ts->input_dev);

err_input_dev_alloc_failed:
err_detect_failed:
//err_power_failed:
	kfree(ts);
err_alloc_data_failed:
err_check_functionality_failed:
	gpio_free(GPIO_TOUCH_EN_OUT);
	return ret;
}

static int synaptics_ts_remove(struct i2c_client *client)
{
	struct synaptics_ts_data *ts = i2c_get_clientdata(client);
	unregister_early_suspend(&ts->early_suspend);
	if (ts->use_irq)
		free_irq(client->irq, ts);
	else
		hrtimer_cancel(&ts->timer);
	input_unregister_device(ts->input_dev);
	kfree(ts);
	gpio_direction_output(GPIO_TOUCH_EN_OUT, 0);
	return 0;
}

static int synaptics_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int ret;
	struct synaptics_ts_data *ts = i2c_get_clientdata(client);

	if (ts->use_irq)
		disable_irq(client->irq);
	else
		hrtimer_cancel(&ts->timer);
	ret = cancel_work_sync(&ts->work);
	if (ret && ts->use_irq)
		enable_irq(client->irq);
    ret = synaptics_i2c_write(ts->client, 0x26, 0);
	if (ret < 0)
        printk(KERN_ERR "synaptics_ts_suspend: synaptics_i2c_write failed\n");

    ret = synaptics_i2c_write(client, 0x25, 0x01);
	if (ret < 0)
        printk(KERN_ERR "synaptics_ts_suspend: synaptics_i2c_write failed\n");

	return 0;
}

static int synaptics_ts_resume(struct i2c_client *client)
{
	int ret;
	struct synaptics_ts_data *ts = i2c_get_clientdata(client);
	gpio_direction_output(GPIO_TOUCH_EN_OUT, 1);
    ret = synaptics_i2c_write(ts->client, 0x25, 0x00);
	if (ts->use_irq)
		enable_irq(client->irq);

	if (!ts->use_irq)
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
	else
		{
        synaptics_i2c_write(ts->client, 0x26, 0x07);
	    	synaptics_i2c_write(ts->client, 0x31, 0x7F);
		}
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void synaptics_ts_early_suspend(struct early_suspend *h)
{
	struct synaptics_ts_data *ts;
	ts = container_of(h, struct synaptics_ts_data, early_suspend);
	synaptics_ts_suspend(ts->client, PMSG_SUSPEND);
}

static void synaptics_ts_late_resume(struct early_suspend *h)
{
	struct synaptics_ts_data *ts;
	ts = container_of(h, struct synaptics_ts_data, early_suspend);
	synaptics_ts_resume(ts->client);
}
#endif

static const struct i2c_device_id synaptics_ts_id[] = {
	{ SYNAPTICS_I2C_RMI_NAME, 0 },
	{ }
};

static struct i2c_driver synaptics_ts_driver = {
	.probe		= synaptics_ts_probe,
	.remove		= synaptics_ts_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= synaptics_ts_suspend,
	.resume		= synaptics_ts_resume,
#endif
	.id_table	= synaptics_ts_id,
	.driver = {
		.name	= SYNAPTICS_I2C_RMI_NAME,
	},
};

static int __devinit synaptics_ts_init(void)
{
	synaptics_wq = create_rt_workqueue("synaptics_wq");
	if (!synaptics_wq)
		return -ENOMEM;
	return i2c_add_driver(&synaptics_ts_driver);
}

static void __exit synaptics_ts_exit(void)
{
	i2c_del_driver(&synaptics_ts_driver);
	if (synaptics_wq)
		destroy_workqueue(synaptics_wq);
}

module_init(synaptics_ts_init);
module_exit(synaptics_ts_exit);

MODULE_DESCRIPTION("Synaptics Touchscreen Driver");
MODULE_LICENSE("GPL");
