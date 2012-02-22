/* drivers/input/touchscreen/atmel.c - ATMEL Touch driver
 *
 * Copyright (C) 2009 
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
/* ========================================================================================
when            who       what, where, why                         		comment tag
--------     ----       -------------------------------------    --------------------------
2011-08-30   hjy        add the TOUCH_LONG_SLIDE function    TOUCH_LONG_SLIDE
2011-04-25   zfj         add P732A driver code                      ZTE_TS_ZFJ_20110425
2011-02-23   hjy         ATMEL 140 用在P727A20上       ZTE_HJY_CRDB00000000
2010-12-14   wly         v9默认竖屏                                 ZTE_WLY_CRDB00586327
2010-12-13   wly         v9＋默认竖屏                               ZTE_WLY_CRDB00586327
2010-11-24   wly         解决手掌在屏上，睡眠唤醒后数据乱报问题     ZTE_WLY_CRDB00577718
2010-10-25   wly         update report data format                  ZTE_WLY_CRDB00517999
========================================================================================*/
#include <linux/module.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <asm/io.h>
#include <asm/gpio.h>
#include <mach/board.h>
#include <asm/mach-types.h>
#include <linux/atmel_qt602240.h>
#include <linux/jiffies.h>
#include <mach/msm_hsusb.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#ifdef CONFIG_ATMEL_FW_UPDATE
#include <linux/firmware.h>
#endif
#define ATMEL_EN_SYSFS
#define ATMEL_I2C_RETRY_TIMES 10
#define ENABLE_IME_IMPROVEMENT

enum atmel_chip
{
	ATMEL_140,
	ATMEL_224,
	ATMEL_UNKNOW=0XFF,
};
uint8_t ATMEL_CHIP_TYPE;

#define ATMEL_NEW_CAL
#ifdef ATMEL_NEW_CAL
/*
new_cal_state:
0: cal open
1: cal close
*/
static int new_cal_state=0;
#endif
#if defined(CONFIG_TOUCHSCREEN_VIRTUAL_KEYS)
#define virtualkeys virtualkeys.atmel-touchscreen
#endif
#ifdef CONFIG_ATMEL_FW_UPDATE
static int update_result_flag=0;
#endif

struct atmel_ts_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct workqueue_struct *atmel_wq;
	struct work_struct work;

	int (*power) (int on);
	struct early_suspend early_suspend;
	struct info_id_t *id;
	struct object_t *object_table;
	uint8_t finger_count;
	uint16_t abs_x_min;
	uint16_t abs_x_max;
	uint16_t abs_y_min;
	uint16_t abs_y_max;
	uint8_t abs_pressure_min;
	uint8_t abs_pressure_max;
	uint8_t abs_width_min;
	uint8_t abs_width_max;
	//uint8_t first_pressed;
	uint8_t debug_log_level;
	struct atmel_finger_data finger_data[10];
	uint8_t finger_type;
	uint8_t finger_support;
	uint16_t finger_pressed;
	//uint8_t face_suppression;
	//uint8_t grip_suppression;
	//uint8_t noise_status[2];
	uint16_t *filter_level;
	uint8_t calibration_confirm;
	uint64_t timestamp;
	uint64_t timestamp1;
	
	struct atmel_config_data config_setting[2];
	uint8_t status;
	uint8_t GCAF_sample;
	uint8_t *GCAF_level;
	uint8_t noisethr;
#ifdef ATMEL_EN_SYSFS
	struct device dev;
#endif
#ifdef ENABLE_IME_IMPROVEMENT
	int display_width;	/* display width in pixel */
	int display_height;	/* display height in pixel */
	int ime_threshold_pixel;	/* threshold in pixel */
	int ime_threshold[2];		/* threshold X & Y in raw data */
	int ime_area_pixel[4];		/* ime area in pixel */
	int ime_area_pos[4];		/* ime area in raw data */
	int ime_finger_report[2];
	int ime_move;
#endif

};

static struct atmel_ts_data *private_ts;

#ifdef CONFIG_HAS_EARLYSUSPEND
static void atmel_ts_early_suspend(struct early_suspend *h);
static void atmel_ts_late_resume(struct early_suspend *h);
#endif

int i2c_atmel_read(struct i2c_client *client, uint16_t address, uint8_t *data, uint8_t length)
{
	int retry;
	uint8_t addr[2];
	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = 2,
			.buf = addr,
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = length,
			.buf = data,
		}
	};
	addr[0] = address & 0xFF;
	addr[1] = (address >> 8) & 0xFF;

	for (retry = 0; retry < ATMEL_I2C_RETRY_TIMES; retry++) {
		if (i2c_transfer(client->adapter, msg, 2) == 2)
			break;
		mdelay(10);
	}
	if (retry == ATMEL_I2C_RETRY_TIMES) {
		printk(KERN_ERR "i2c_read_block retry over %d\n",
			ATMEL_I2C_RETRY_TIMES);
		return -EIO;
	}
	return 0;
}

int i2c_atmel_write(struct i2c_client *client, uint16_t address, uint8_t *data, uint8_t length)
{
	int retry, loop_i;
	uint8_t buf[length + 2];

	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = length + 2,
			.buf = buf,
		}
	};

	buf[0] = address & 0xFF;
	buf[1] = (address >> 8) & 0xFF;

	for (loop_i = 0; loop_i < length; loop_i++)
		buf[loop_i + 2] = data[loop_i];

	for (retry = 0; retry < ATMEL_I2C_RETRY_TIMES; retry++) {
		if (i2c_transfer(client->adapter, msg, 1) == 1)
			break;
		mdelay(10);
	}

	if (retry == ATMEL_I2C_RETRY_TIMES) {
		printk(KERN_ERR "i2c_write_block retry over %d\n",
			ATMEL_I2C_RETRY_TIMES);
		return -EIO;
	}
	return 0;
}

int i2c_atmel_write_byte_data(struct i2c_client *client, uint16_t address, uint8_t value)
{
	i2c_atmel_write(client, address, &value, 1);
	return 0;
}
int i2c_atmel_write_word(struct i2c_client *client, uint16_t address, int16_t *data, uint8_t length)
{
	int i,ret;
	uint8_t buf[length*2];
	for(i =0;i<length;i++)
	{
		buf[i*2] = data[i] & 0xff;
		buf[i*2+1] = data[i]>> 8;
  }
  ret = i2c_atmel_write(client, address, buf, length*2);
  return ret;
    	
}
int i2c_atmel_read_word(struct i2c_client *client, uint16_t address, int16_t *data, uint8_t length)
{
	  uint8_t i,buf[length*2],ret;
    ret = i2c_atmel_read(client, address, buf, length*2);
    for(i=0;i<length*2;i++)
    data[i]= buf[i*2]|(buf[i*2+1] << 8);
    return ret;
}
uint16_t get_object_address(struct atmel_ts_data *ts, uint8_t object_type)
{
	uint8_t loop_i;
	for (loop_i = 0; loop_i < ts->id->num_declared_objects; loop_i++) {
		if (ts->object_table[loop_i].object_type == object_type)
			return ts->object_table[loop_i].i2c_address;
	}
	return 0;
}
uint8_t get_object_size(struct atmel_ts_data *ts, uint8_t object_type)
{
	uint8_t loop_i;
	for (loop_i = 0; loop_i < ts->id->num_declared_objects; loop_i++) {
		if (ts->object_table[loop_i].object_type == object_type)
			return ts->object_table[loop_i].size;
	}
	return 0;
}

uint8_t get_report_ids_size(struct atmel_ts_data *ts, uint8_t object_type)
{
	uint8_t loop_i;
	for (loop_i = 0; loop_i < ts->id->num_declared_objects; loop_i++) {
		if (ts->object_table[loop_i].object_type == object_type)
			return ts->object_table[loop_i].report_ids;
	}
	return 0;
}
 static uint8_t get_object_type_from_idnum(struct atmel_ts_data *ts, uint8_t id_num)
{
	uint8_t loop_i;
	for (loop_i = 0; loop_i < ts->id->num_declared_objects; loop_i++) {
		if ((id_num>=ts->object_table[loop_i].report_ids)&&
			(id_num<=ts->object_table[loop_i].report_ids+ts->object_table[loop_i].num_report_ids-1))
			return ts->object_table[loop_i].object_type;
	}
	return 0;
}

#ifdef ATMEL_EN_SYSFS
//huangjinyu ZTE_TS_HUANGJINYU_20110223 BEGIN
#if defined (CONFIG_MACH_MOONCAKE )
static const char ts_keys_size[] = "0x01:102:40:350:50:15:0x01:158:200:350:50:15";
#elif defined (CONFIG_MACH_V9PLUS)
static const char ts_keys_size[] = "0x01:102:100:1030:100:10:0x01:139:300:1030:100:10:0x01:158:500:1030:100:10";
#elif defined(CONFIG_MACH_ARTHUR)
static const char ts_keys_size[] =
"0x01:102:60:850:80:30:0x01:139:180:850:80:30:0x01:158:300:850:80:30:0x01:217:420:850:80:30";
#elif defined(CONFIG_MACH_SKATEPLUS)
static const char ts_keys_size[] =
"0x01:102:60:850:80:30:0x01:139:240:850:80:30:0x01:158:420:850:80:30";

#elif defined(CONFIG_MACH_SEAN)
static const char ts_keys_size[] = "0x01:102:40:500:60:20:0x01:139:120:500:60:20:0x01:158:200:500:60:20:0x01:217:280:500:60:20";
#else
static const char ts_keys_size[] = "0x01:102:80:810:100:10:0x01:139:240:810:100:10:0x01:158:400:810:100:10";
#endif
//huangjinyu ZTE_TS_HUANGJINYU_20110223 END
static ssize_t atmel_virtualkeys_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	        sprintf(buf,"%s\n",ts_keys_size);
        return strlen(ts_keys_size)+2;
}
static DEVICE_ATTR(virtualkeys, 0444, atmel_virtualkeys_show, NULL);

static int lcd_flag = 0;
static ssize_t atmel_lcd_gpio_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret = 0;
	struct atmel_ts_data *ts_data;
	struct atmel_i2c_platform_data *pdata;
		ts_data = private_ts;
	pdata = ts_data->client->dev.platform_data;

	gpio_free(57);
	ret = gpio_request(57, "lcd gpio");
	if (ret) {
		pr_err("%s: unable to request gpio 57 (%d)\n",
			__func__, ret);
	}
	
  if (!lcd_flag)
  {
  	gpio_direction_output(57, 0);
  }
  else 
  {
  	gpio_direction_output(57, 1);
  }
  lcd_flag = (!lcd_flag);
  ret = gpio_get_value(57);
	sprintf(buf, "lcd_gpio=%d\n", ret);
	ret = strlen(buf) + 1;
	return ret;
}
static DEVICE_ATTR(lcd_gpio, 0444, atmel_lcd_gpio_show, NULL);



static ssize_t atmel_gpio_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret = 0;
	struct atmel_ts_data *ts_data;
	struct atmel_i2c_platform_data *pdata;

	ts_data = private_ts;
	pdata = ts_data->client->dev.platform_data;

	ret = gpio_get_value(pdata->gpio_irq);
	printk(KERN_DEBUG "GPIO_TP_INT_N=%d\n", pdata->gpio_irq);
	sprintf(buf, "GPIO_TP_INT_N=%d\n", ret);
	ret = strlen(buf) + 1;
	return ret;
}
static DEVICE_ATTR(gpio, 0444, atmel_gpio_show, NULL);
static ssize_t atmel_vendor_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret = 0;
	struct atmel_ts_data *ts_data;
	ts_data = private_ts;
	sprintf(buf, "%s_x%4.4X_x%4.4X\n", "ATMEL",
		ts_data->id->family_id, ts_data->id->version);
	ret = strlen(buf) + 1;
	return ret;
}

static DEVICE_ATTR(vendor, 0444, atmel_vendor_show, NULL);

static uint16_t atmel_reg_addr;

static ssize_t atmel_register_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int ret = 0;
	uint8_t ptr[1];
	struct atmel_ts_data *ts_data;
	ts_data = private_ts;
	if (i2c_atmel_read(ts_data->client, atmel_reg_addr, ptr, 1) < 0) {
		printk(KERN_WARNING "%s: read fail\n", __func__);
		return ret;
	}
	ret += sprintf(buf, "addr: %d, data: %d\n", atmel_reg_addr, ptr[0]);
	return ret;
}

static ssize_t atmel_register_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	int ret = 0;
	struct atmel_ts_data *ts_data;
	char buf_tmp[4], buf_zero[200];
	uint8_t write_da;

	ts_data = private_ts;
	memset(buf_tmp, 0x0, sizeof(buf_tmp));
	if ((buf[0] == 'r' || buf[0] == 'w') && buf[1] == ':' &&
		(buf[5] == ':' || buf[5] == '\n')) {
		memcpy(buf_tmp, buf + 2, 3);
		atmel_reg_addr = simple_strtol(buf_tmp, NULL, 10);
		printk(KERN_DEBUG "read addr: 0x%X\n", atmel_reg_addr);
		if (!atmel_reg_addr) {
			printk(KERN_WARNING "%s: string to number fail\n",
								__func__);
			return count;
		}
		printk(KERN_DEBUG "%s: set atmel_reg_addr is: %d\n",
						__func__, atmel_reg_addr);
		if (buf[0] == 'w' && buf[5] == ':' && buf[9] == '\n') {
			memcpy(buf_tmp, buf + 6, 3);
			write_da = simple_strtol(buf_tmp, NULL, 10);
			printk(KERN_DEBUG "write addr: 0x%X, data: 0x%X\n",
						atmel_reg_addr, write_da);
			ret = i2c_atmel_write_byte_data(ts_data->client,
						atmel_reg_addr, write_da);
			if (ret < 0) {
				printk(KERN_ERR "%s: write fail(%d)\n",
								__func__, ret);
			}
		}
	}
	if ((buf[0] == '0') && (buf[1] == ':') && (buf[5] == ':')) {
		memcpy(buf_tmp, buf + 2, 3);
		atmel_reg_addr = simple_strtol(buf_tmp, NULL, 10);
		memcpy(buf_tmp, buf + 6, 3);
		memset(buf_zero, 0x0, sizeof(buf_zero));
		ret = i2c_atmel_write(ts_data->client, atmel_reg_addr,
			buf_zero, simple_strtol(buf_tmp, NULL, 10) - atmel_reg_addr + 1);
		if (buf[9] == 'r') {
			i2c_atmel_write_byte_data(ts_data->client,
				get_object_address(ts_data, GEN_COMMANDPROCESSOR_T6) + 1, 0x55);
			i2c_atmel_write_byte_data(ts_data->client,
				get_object_address(ts_data, GEN_COMMANDPROCESSOR_T6), 0x11);
		}
	}

	return count;
}

static DEVICE_ATTR(register, 0644, atmel_register_show, atmel_register_store);

static ssize_t atmel_regdump_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int count = 0, ret_t = 0;
	struct atmel_ts_data *ts_data;
	uint16_t loop_i;
	uint8_t ptr[1];
	ts_data = private_ts;
	if (ts_data->id->version >= 0x14) {
		for (loop_i = 230; loop_i <= 425; loop_i++) {
			ret_t = i2c_atmel_read(ts_data->client, loop_i, ptr, 1);
			if (ret_t < 0) {
				printk(KERN_WARNING "dump fail, addr: %d\n",
								loop_i);
			}
			count += sprintf(buf + count, "addr[%3d]: %3d, ",
								loop_i , *ptr);
			if (((loop_i - 230) % 4) == 3)
				count += sprintf(buf + count, "\n");
		}
		count += sprintf(buf + count, "\n");
	}
	return count;
}

static ssize_t atmel_regdump_dump(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct atmel_ts_data *ts_data;
	ts_data = private_ts;
	if (buf[0] >= '0' && buf[0] <= '9' && buf[1] == '\n')
		ts_data->debug_log_level = buf[0] - 0x30;

	return count;

}

static DEVICE_ATTR(regdump, 0644, atmel_regdump_show, atmel_regdump_dump);

static ssize_t atmel_debug_level_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct atmel_ts_data *ts_data;
	size_t count = 0;
	ts_data = private_ts;

	count += sprintf(buf, "%d\n", ts_data->debug_log_level);

	return count;
}

static ssize_t atmel_debug_level_dump(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct atmel_ts_data *ts_data;
	ts_data = private_ts;
	if (buf[0] >= '0' && buf[0] <= '9' && buf[1] == '\n')
		ts_data->debug_log_level = buf[0] - 0x30;

	return count;
}

static DEVICE_ATTR(debug_level, 0644, atmel_debug_level_show, atmel_debug_level_dump);

#ifdef ENABLE_IME_IMPROVEMENT
static ssize_t ime_threshold_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct atmel_ts_data *ts = private_ts;

	return sprintf(buf, "%d\n", ts->ime_threshold_pixel);
}

static ssize_t ime_threshold_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct atmel_ts_data *ts = private_ts;
	char *ptr_data = (char *)buf;
	unsigned long val;

	val = simple_strtoul(ptr_data, NULL, 10);

	if (val >= 0 && val <= max(ts->display_width, ts->display_height))
		ts->ime_threshold_pixel = val;
	else
		ts->ime_threshold_pixel = 0;

	ts->ime_threshold[0] = ts->ime_threshold_pixel * (ts->abs_x_max - ts->abs_x_min) / ts->display_width;
	ts->ime_threshold[1] = ts->ime_threshold_pixel * (ts->abs_y_max - ts->abs_y_min) / ts->display_height;

	return count;
}

static ssize_t ime_work_area_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct atmel_ts_data *ts = private_ts;

	return sprintf(buf, "%d,%d,%d,%d\n", ts->ime_area_pixel[0],
			ts->ime_area_pixel[1], ts->ime_area_pixel[2], ts->ime_area_pixel[3]);
}

static ssize_t ime_work_area_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	struct atmel_ts_data *ts = private_ts;
	char *ptr_data = (char *)buf;
	char *p;
	int pt_count = 0;
	unsigned long val[4];

	while ((p = strsep(&ptr_data, ","))) {
		if (!*p)
			break;

		if (pt_count >= 4)
			break;

		val[pt_count] = simple_strtoul(p, NULL, 10);

		pt_count++;
	}

	if (pt_count >= 4 && ts->display_width && ts->display_height) {
		ts->ime_area_pixel[0] = val[0]; /* Left */
		ts->ime_area_pixel[1] = val[1]; /* Right */
		ts->ime_area_pixel[2] = val[2]; /* Top */
		ts->ime_area_pixel[3] = val[3]; /* Bottom */

		if (val[0] < 0 || val[0] > ts->display_width)
			ts->ime_area_pos[0] = 0;
		else
			ts->ime_area_pos[0] = val[0] * (ts->abs_x_max - ts->abs_x_min) / ts->display_width + ts->abs_x_min;

		if (val[1] < 0 || val[1] > ts->display_width)
			ts->ime_area_pos[1] = ts->abs_x_max;
		else
			ts->ime_area_pos[1] = val[1] * (ts->abs_x_max - ts->abs_x_min) / ts->display_width + ts->abs_x_min;

		if (val[2] < 0 || val[2] > ts->display_height)
			ts->ime_area_pos[2] = 0;
		else
			ts->ime_area_pos[2] = val[2] * (ts->abs_y_max - ts->abs_y_min) / ts->display_height + ts->abs_y_min;

		if (val[3] < 0 || val[3] > ts->display_height)
			ts->ime_area_pos[3] = ts->abs_y_max;
		else
			ts->ime_area_pos[3] = val[3] * (ts->abs_y_max - ts->abs_y_min) / ts->display_height + ts->abs_y_min;
	}

	return count;
}

static int ime_report_filter(struct atmel_ts_data *ts, uint8_t *data)
{
	int drift_x,drift_y,x,y,w,z,report_type;
	uint8_t discard = 0;
	report_type = data[0] -2;
	x = data[2] << 2 | data[4] >> 6;
	y = data[3] << 2 | (data[4] & 0x0C) >> 2;
	w = data[5];
	z = data[6];
	drift_x = abs(ts->finger_data[report_type].x - x);
	drift_y = abs(ts->finger_data[report_type].y - y);
	if (drift_x < ts->ime_threshold_pixel && drift_y < ts->ime_threshold_pixel && z != 0) {
		discard = 1;
	}
	if (discard) {
		/* if finger's movement < threshold , discard it. */
		ts->finger_data[report_type].z = z;//add by huangjinyu 20110523 
		return 1;
	}
	ts->finger_data[report_type].x = x;
	ts->finger_data[report_type].y = y;
	ts->finger_data[report_type].w = w;
	ts->finger_data[report_type].z = z;
	return 0;
}

/* sys/class/input/inputX/ime_threshold */
static DEVICE_ATTR(ime_threshold, 0664, ime_threshold_show,
		ime_threshold_store);
static DEVICE_ATTR(ime_work_area, 0664, ime_work_area_show,
		ime_work_area_store);
#endif


#if defined(CONFIG_TOUCHSCREEN_VIRTUAL_KEYS)
extern struct kobject *android_touch_kobj;
#endif
static struct kobject * virtual_key_kobj;

static int atmel_touch_sysfs_init(void)
{
	int ret;

	#if defined(CONFIG_TOUCHSCREEN_VIRTUAL_KEYS)
	virtual_key_kobj = kobject_get(android_touch_kobj);
	#endif

	if (virtual_key_kobj == NULL) {
		virtual_key_kobj = kobject_create_and_add("board_properties", NULL);
		if (virtual_key_kobj == NULL) {
		printk(KERN_ERR "%s: subsystem_register failed\n", __func__);
		ret = -ENOMEM;
		return ret;
	}
	}
	ret = sysfs_create_file(virtual_key_kobj, &dev_attr_virtualkeys.attr);
	printk("xiayc, ret=%d\n",ret);
	if (ret) {
		printk(KERN_ERR "%s: sysfs_create_file failed\n", __func__);
		return ret;
	}
	ret = sysfs_create_file(virtual_key_kobj, &dev_attr_lcd_gpio.attr);
	if (ret) {
		printk(KERN_ERR "%s: sysfs_create_file failed\n", __func__);
		return ret;
	}
	
	
	ret = sysfs_create_file(virtual_key_kobj, &dev_attr_gpio.attr);
	if (ret) {
		printk(KERN_ERR "%s: sysfs_create_file failed\n", __func__);
		return ret;
	}
	ret = sysfs_create_file(virtual_key_kobj, &dev_attr_vendor.attr);
	if (ret) {
		printk(KERN_ERR "%s: sysfs_create_file failed\n", __func__);
		return ret;
	}
	atmel_reg_addr = 0;
	ret = sysfs_create_file(virtual_key_kobj, &dev_attr_register.attr);
	if (ret) {
		printk(KERN_ERR "%s: sysfs_create_file failed\n", __func__);
		return ret;
	}
	ret = sysfs_create_file(virtual_key_kobj, &dev_attr_regdump.attr);
	if (ret) {
		printk(KERN_ERR "%s: sysfs_create_file failed\n", __func__);
		return ret;
	}
	ret = sysfs_create_file(virtual_key_kobj, &dev_attr_debug_level.attr);
	if (ret) {
		printk(KERN_ERR "%s: sysfs_create_file failed\n", __func__);
		return ret;
	}
	return 0;
}

static void atmel_touch_sysfs_deinit(void)
{
	sysfs_remove_file(virtual_key_kobj, &dev_attr_regdump.attr);
	sysfs_remove_file(virtual_key_kobj, &dev_attr_register.attr);
	sysfs_remove_file(virtual_key_kobj, &dev_attr_vendor.attr);
	sysfs_remove_file(virtual_key_kobj, &dev_attr_gpio.attr);
	sysfs_remove_file(virtual_key_kobj, &dev_attr_lcd_gpio.attr);
	sysfs_remove_file(virtual_key_kobj, &dev_attr_virtualkeys.attr);
	kobject_del(virtual_key_kobj);
}

#endif




/*static int check_delta(struct atmel_ts_data*ts)
{
	int8_t data[128];
	uint8_t loop_i;
	int16_t rawdata, count = 0;
	
	memset(data, 0xFF, sizeof(data));
	i2c_atmel_write_byte_data(ts->client,
		get_object_address(ts, GEN_COMMANDPROCESSOR_T6) + 5, 0x10);

	for (loop_i = 0; !(data[0] == 0x10 && data[1] == 0x00) && loop_i < 10; loop_i++) {
		msleep(5);
		i2c_atmel_read(ts->client, get_object_address(ts, DIAGNOSTIC_T37), data, 2);
	}
	if (loop_i == 10)
		printk(KERN_ERR "%s: Diag data not ready\n", __func__);

	i2c_atmel_read(ts->client, get_object_address(ts, DIAGNOSTIC_T37), data, 128);
	if (data[0] == 0x10 && data[1] == 0x00) {
		for (loop_i = 2; loop_i < 127; loop_i += 2) {
			rawdata = data[loop_i+1] << 8 | data[loop_i];
			if (abs(rawdata) > 50)
				count++;
		}
		printk("***********: check_delta , count =%d\n", count);
		if (count > 32)
			return 1;
	}
	return 0;
}*/
#if 1 //wly add
static void release_all_fingers(struct atmel_ts_data*ts)
{
	int loop_i;

	for(loop_i =0; loop_i< ts->finger_support; loop_i ++ )
	{
		if(ts->finger_data[loop_i].report_enable)
		{
			printk("release all fingers!\n");
			ts->finger_data[loop_i].z = 0;
			ts->finger_data[loop_i].report_enable = 0;
			input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR,ts->finger_data[loop_i].z);
			input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR,ts->finger_data[loop_i].w);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_X,ts->finger_data[loop_i].x);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_Y,ts->finger_data[loop_i].y);	
			input_mt_sync(ts->input_dev);	
		}
		
	}
	input_sync(ts->input_dev);
	ts->finger_pressed = 0;
	ts->finger_count = 0;
}
#endif
	static uint8_t count=0;

#define TOUCH_LONG_SLIDE
#ifdef TOUCH_LONG_SLIDE
static int x_value;
static int y_value;
//是否取了起点的标志位
// 0 no
// 1 yes
// 2 ok
static uint8_t temp_flag=0;
//是否滑动完成的标志位
// 0 no
// 1 yes
static uint8_t temp_flag2=0;
#endif
static struct atmel_ts_data *ts_temp;


//#endif

#if defined CONFIG_TS_NOTIFIER
static int usb_status=0;

static int atmel_ts_event(struct notifier_block *this, unsigned long event,
			   void *ptr)
{
	int ret;
#ifdef CONFIG_ATMEL_FW_UPDATE
	if(update_result_flag==3)//updating
		return 0;
#endif

	switch(event)
		{
		case 0:
			//offline
			if(usb_status!=0){
		 		usb_status=0;
				printk("ts config change to offline status\n");
		 		if(ts_temp->id->version!=0x16){
		 		i2c_atmel_write_byte_data(ts_temp->client,
		 			get_object_address(ts_temp, TOUCH_MULTITOUCHSCREEN_T9) + 31,
		 			ts_temp->config_setting[0].config_T9[31]);
		 		}				

		 		i2c_atmel_write_byte_data(ts_temp->client,
		 			get_object_address(ts_temp, TOUCH_MULTITOUCHSCREEN_T9) + 7,
		 			ts_temp->config_setting[0].config_T9[7]);
		 		i2c_atmel_write_byte_data(ts_temp->client,
		 			get_object_address(ts_temp, TOUCH_MULTITOUCHSCREEN_T9) + 11,
		 			ts_temp->config_setting[0].config_T9[11]);
		 		
		 		i2c_atmel_write_byte_data(ts_temp->client,
		 			get_object_address(ts_temp, TOUCH_MULTITOUCHSCREEN_T9) + 12,
		 			ts_temp->config_setting[0].config_T9[12]);			
		 		i2c_atmel_write_byte_data(ts_temp->client,
		 			get_object_address(ts_temp, SPT_CTECONFIG_T28) + 3,
		 			ts_temp->config_setting[0].config_T28[3]);
		 		i2c_atmel_write_byte_data(ts_temp->client,
		 			get_object_address(ts_temp, SPT_CTECONFIG_T28) + 4,
		 			ts_temp->config_setting[0].config_T28[4]);
			}

			break;
		case 1:
			//online
			if(usb_status!=1){
		 		usb_status=1;
				printk("ts config change to online status\n");
	 			if(ts_temp->id->version!=0x16){
	 			i2c_atmel_write_byte_data(ts_temp->client,
	 				get_object_address(ts_temp, TOUCH_MULTITOUCHSCREEN_T9) + 31,
	 				ts_temp->config_setting[0].config_T9_charge[1]);
	 			}
	 			i2c_atmel_write_byte_data(ts_temp->client,
	 				get_object_address(ts_temp, TOUCH_MULTITOUCHSCREEN_T9) + 7,
	 				ts_temp->config_setting[0].config_T9_charge[0]);
	 			
	 			i2c_atmel_write_byte_data(ts_temp->client,
	 				get_object_address(ts_temp, TOUCH_MULTITOUCHSCREEN_T9) + 11,
	 				ts_temp->config_setting[0].config_T9_charge[2]);
	 			
	 			i2c_atmel_write_byte_data(ts_temp->client,
	 				get_object_address(ts_temp, TOUCH_MULTITOUCHSCREEN_T9) + 12,
	 				ts_temp->config_setting[0].config_T9_charge[3]);
	 #if 0
	 			i2c_atmel_write_byte_data(ts_temp->client,
	 				get_object_address(ts_temp, SPT_CTECONFIG_T28) + 3,
	 				ts_temp->config_setting[0].config_T28_charge[0]);
	 			i2c_atmel_write_byte_data(ts_temp->client,
	 				get_object_address(ts_temp, SPT_CTECONFIG_T28) + 4,
	 				ts_temp->config_setting[0].config_T28_charge[1]);
	#endif
			}
			break;
		default:
			break;
		}

	ret = NOTIFY_DONE;

	return ret;
}

static struct notifier_block atmel_ts_notifier = {
	.notifier_call = atmel_ts_event,
};


static BLOCKING_NOTIFIER_HEAD(atmel_ts_chain_head);

int register_atmel_ts_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_register(&atmel_ts_chain_head, nb);
}
EXPORT_SYMBOL_GPL(register_atmel_ts_notifier);

int unregister_atmel_ts_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&atmel_ts_chain_head, nb);
}
EXPORT_SYMBOL_GPL(unregister_atmel_ts_notifier);

int ts_notifier_call_chain(unsigned long val)
{
	return (blocking_notifier_call_chain(&atmel_ts_chain_head, val, NULL)
			== NOTIFY_BAD) ? -EINVAL : 0;
}

#endif
static void auto_cal_set(struct atmel_ts_data*ts,int val)
{
		i2c_atmel_write_byte_data(ts->client,
			get_object_address(ts, GEN_ACQUISITIONCONFIG_T8) + 4, 
			val);	
}
/*
其实这个函数还控制了PALM校准
*/
static void atch_cal_control(struct atmel_ts_data*ts,int on)
{
	
	printk(" atmel atch_cal_control:%d.    1:open 0:close\n",on);
	if(on==1)//open
	{
		//原始值为打开负向校准
		i2c_atmel_write_byte_data(ts->client,
			get_object_address(ts, GEN_ACQUISITIONCONFIG_T8) + 6, 
			ts->config_setting[0].config_T8[6]);
		i2c_atmel_write_byte_data(ts->client,
			get_object_address(ts, GEN_ACQUISITIONCONFIG_T8) + 7, 
			ts->config_setting[0].config_T8[7]);

		if((ts->id->version!=0x16)){
			i2c_atmel_write_byte_data(ts->client,
			 get_object_address(ts, GEN_ACQUISITIONCONFIG_T8) + 8, 
			 ts->config_setting[0].config_T8[8]);
			i2c_atmel_write_byte_data(ts->client,
			 get_object_address(ts, GEN_ACQUISITIONCONFIG_T8) + 9, 
			 ts->config_setting[0].config_T8[9]);
				
			i2c_atmel_write_byte_data(ts->client,
			 get_object_address(ts, TOUCH_MULTITOUCHSCREEN_T9) + 31, 
			 ts->config_setting[0].config_T9[31]);				
			}

	}else{
		i2c_atmel_write_byte_data(ts->client,
			get_object_address(ts, GEN_ACQUISITIONCONFIG_T8) + 6, 255);
		i2c_atmel_write_byte_data(ts->client,
			get_object_address(ts, GEN_ACQUISITIONCONFIG_T8) + 7, 1);

		if((ts->id->version!=0x16)){
			i2c_atmel_write_byte_data(ts->client,
				get_object_address(ts, GEN_ACQUISITIONCONFIG_T8) + 8, 0);	
			i2c_atmel_write_byte_data(ts->client,
				get_object_address(ts, GEN_ACQUISITIONCONFIG_T8) + 9, 0);	
			//降低检查校准OK时候负向出现的概率
			i2c_atmel_write_byte_data(ts->client,
				get_object_address(ts, TOUCH_MULTITOUCHSCREEN_T9) + 31,0);	


			}
	}
}
/*
控制face和auto校准
*/
static void new_cal_control(struct atmel_ts_data*ts,int on)
{
	printk(" atmel new_cal_control:%d.    1:open 0:close\n",on);
	if(on==1)//open
	{
		//原始值为打开各种校准
		i2c_atmel_write_byte_data(ts->client,
			get_object_address(ts, PROCI_GRIPFACESUPPRESSION_T20), 
			ts->config_setting[0].config_T20[0]);
		i2c_atmel_write_byte_data(ts->client,
			get_object_address(ts, GEN_ACQUISITIONCONFIG_T8) + 4, 
			ts->config_setting[0].config_T8[4]);
		//快漂开
		i2c_atmel_write_byte_data(ts->client,
			get_object_address(ts, NEW_CAL_T59), 
			7);	

	}else{
		i2c_atmel_write_byte_data(ts->client,
			get_object_address(ts, PROCI_GRIPFACESUPPRESSION_T20), 
			0);
		i2c_atmel_write_byte_data(ts->client,
			get_object_address(ts, GEN_ACQUISITIONCONFIG_T8) + 4, 
			0);
		//快漂关
		i2c_atmel_write_byte_data(ts->client,
			get_object_address(ts, NEW_CAL_T59), 
			0);		

	}	
}
static void check_atch_and_tch(struct atmel_ts_data*ts,uint8_t*atch_and_tch)
{
	
	uint8_t data[82];
	uint8_t x_limit,check_mask,loop_i,loop_j, tch_ch = 0, atch_ch = 0;
	
	i2c_atmel_write_byte_data(ts->client,
		get_object_address(ts, GEN_COMMANDPROCESSOR_T6) + 5, 0xF3);

	for (loop_i = 0; !(data[0] == 0xF3 && data[1] == 0x00) && loop_i < 100; loop_i++) {
		msleep(5);
		i2c_atmel_read(ts->client, get_object_address(ts, DIAGNOSTIC_T37), data, 2);
	}

	if (loop_i == 100){
		printk(KERN_ERR "%s: Diag data not ready\n", __func__);
		
		ts->calibration_confirm = 0;
		//reset
		i2c_atmel_write_byte_data(ts->client,
		get_object_address(ts, GEN_COMMANDPROCESSOR_T6), 0x1);	
		return ;
	}	   
	i2c_atmel_read(ts->client, get_object_address(ts, DIAGNOSTIC_T37), data, 82);
	if (data[0] == 0xF3 && data[1] == 0x00) {	
		x_limit = ts->config_setting[0].config_T9[3]+ts->config_setting[0].config_T9[1] 
			+ ts->config_setting[0].config_T28[2];
		x_limit = x_limit << 1;
		if(ATMEL_CHIP_TYPE==ATMEL_140)
		{
	 		for (loop_i = ts->config_setting[0].config_T9[1]; loop_i < x_limit; loop_i += 2) {
	 			for (loop_j = ts->config_setting[0].config_T9[2]; loop_j < 8; loop_j++) {
	 				check_mask = 1 << loop_j;
	 				if (data[2 + loop_i] & check_mask)
	 					tch_ch++;
	 				if (data[32 + loop_i] & check_mask)
	 					atch_ch++;
	 
	 			}
	 			for (loop_j = 0; 
				loop_j < ts->config_setting[0].config_T9[4]+ts->config_setting[0].config_T9[2]-8; 
				loop_j++) {
	 				check_mask = 1 << loop_j;
	 				if (data[3 + loop_i] & check_mask)
	 					tch_ch++;
	 				if (data[33 + loop_i] & check_mask)
	 					atch_ch++;
	 			}
			}
		}else if(ATMEL_CHIP_TYPE==ATMEL_224){
	 		for (loop_i = ts->config_setting[0].config_T9[1]; loop_i < x_limit; loop_i += 2) {
	 			for (loop_j = ts->config_setting[0].config_T9[2]; loop_j < 8; loop_j++) {
	 				check_mask = 1 << loop_j;
	 				if (data[2 + loop_i] & check_mask)
	 					tch_ch++;
	 				if (data[42 + loop_i] & check_mask)
	 					atch_ch++;
	 			}
	 			for (loop_j = 0; 
				loop_j < ts->config_setting[0].config_T9[4]+ts->config_setting[0].config_T9[2]-8; 
				loop_j++) {
	 				check_mask = 1 << loop_j;
	 				if (data[3 + loop_i] & check_mask)
	 					tch_ch++;
	 				if (data[43 + loop_i] & check_mask)
	 					atch_ch++;
	 			}
			}
		}
		atch_and_tch[0]=atch_ch;
		atch_and_tch[1]=tch_ch;		
	}
}
static int atmel_ts_get_build_info(struct atmel_ts_data*ts)
{
	uint8_t data[7];
	int ret;
	printk("i2c address:0x%x",ts->client->addr);
	ret = i2c_atmel_read(ts->client, 0x0000, data, 7);

	ts->id->family_id = data[0];
	ts->id->variant_id = data[1];
	ts->id->version = data[2];
	ts->id->build = data[3];
	ts->id->matrix_x_size = data[4];
	ts->id->matrix_y_size = data[5];
	ts->id->num_declared_objects = data[6];

	printk(KERN_INFO "info block: 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X\n",
	ts->id->family_id, ts->id->variant_id,
	ts->id->version, ts->id->build,
	ts->id->matrix_x_size, ts->id->matrix_y_size,
	ts->id->num_declared_objects);
	
	return 0;
}
static int atmel_ts_get_table_info(struct atmel_ts_data*ts)
{
	uint8_t data[6],type_count = 0;
	int i,ret;
	if (ts->object_table==NULL){
		printk("ts->object_table==NULL\n");
		return 0;
		}
	for (i = 0; i < ts->id->num_declared_objects; i++) {
		ret = i2c_atmel_read(ts->client, i * 6 + 0x07, data, 6);
		ts->object_table[i].object_type = data[0];
		ts->object_table[i].i2c_address = data[1] | data[2] << 8;
		ts->object_table[i].size = data[3] + 1;
		ts->object_table[i].instances = data[4];
		ts->object_table[i].num_report_ids = data[5];
		if (data[5]) {
			ts->object_table[i].report_ids = type_count + 1;
			type_count += data[5];
		}
		if (data[0] == 9)
			ts->finger_type = ts->object_table[i].report_ids;
		printk(KERN_INFO "Type: %2d, Start: 0x%4.4X, Size: %2d, Instance: %2X, RD#: 0x%2X, %2d\n",
			ts->object_table[i].object_type , ts->object_table[i].i2c_address,
			ts->object_table[i].size, ts->object_table[i].instances,
			ts->object_table[i].num_report_ids, ts->object_table[i].report_ids);
	}
	return 0;
}
static int atmel_ts_load_config(struct atmel_ts_data*ts)
{
	int ret,loop_i;
	uint8_t data[9];
	struct atmel_i2c_platform_data *pdata;
	pdata = ts->client->dev.platform_data;

	printk(KERN_INFO "Touch: Config load\n");
	i2c_atmel_write(ts->client, get_object_address(ts, GEN_POWERCONFIG_T7),
		pdata->config_T7, get_object_size(ts, GEN_POWERCONFIG_T7));
	i2c_atmel_write(ts->client, get_object_address(ts, GEN_ACQUISITIONCONFIG_T8),
		pdata->config_T8, get_object_size(ts, GEN_ACQUISITIONCONFIG_T8));
	i2c_atmel_write(ts->client, get_object_address(ts, TOUCH_MULTITOUCHSCREEN_T9),
		pdata->config_T9, get_object_size(ts, TOUCH_MULTITOUCHSCREEN_T9));
	i2c_atmel_write(ts->client, get_object_address(ts, TOUCH_KEYARRAY_T15),
		pdata->config_T15, get_object_size(ts, TOUCH_KEYARRAY_T15));
	i2c_atmel_write(ts->client, get_object_address(ts, SPT_GPIOPWM_T19),
		pdata->config_T19, get_object_size(ts, SPT_GPIOPWM_T19));
	i2c_atmel_write(ts->client, get_object_address(ts, PROCI_GRIPFACESUPPRESSION_T20),
		pdata->config_T20, get_object_size(ts, PROCI_GRIPFACESUPPRESSION_T20));
	i2c_atmel_write(ts->client, get_object_address(ts, PROCG_NOISESUPPRESSION_T22),
		pdata->config_T22, get_object_size(ts, PROCG_NOISESUPPRESSION_T22));
	i2c_atmel_write(ts->client, get_object_address(ts, TOUCH_PROXIMITY_T23),
		pdata->config_T23, get_object_size(ts, TOUCH_PROXIMITY_T23));
	i2c_atmel_write(ts->client, get_object_address(ts, PROCI_ONETOUCHGESTUREPROCESSOR_T24),
		pdata->config_T24, get_object_size(ts, PROCI_ONETOUCHGESTUREPROCESSOR_T24));
	i2c_atmel_write(ts->client, get_object_address(ts, SPT_SELFTEST_T25),
		pdata->config_T25, get_object_size(ts, SPT_SELFTEST_T25));
	i2c_atmel_write(ts->client, get_object_address(ts, PROCI_TWOTOUCHGESTUREPROCESSOR_T27),
		pdata->config_T27, get_object_size(ts, PROCI_TWOTOUCHGESTUREPROCESSOR_T27));
	i2c_atmel_write(ts->client, get_object_address(ts, SPT_CTECONFIG_T28),
		pdata->config_T28, get_object_size(ts, SPT_CTECONFIG_T28));
	i2c_atmel_write(ts->client, get_object_address(ts, NEW_CAL_T35),
		pdata->config_T35, get_object_size(ts, NEW_CAL_T35));
	i2c_atmel_write(ts->client, get_object_address(ts, NEW_CAL_T59),
		pdata->config_T59, get_object_size(ts, NEW_CAL_T59));

	ret = i2c_atmel_write_byte_data(ts->client, get_object_address(ts, GEN_COMMANDPROCESSOR_T6) + 1, 0x55);
	ret = i2c_atmel_write_byte_data(ts->client, get_object_address(ts, GEN_COMMANDPROCESSOR_T6), 0x11);
	msleep(64);
	for (loop_i = 0; loop_i < 10; loop_i++) {
		if (!gpio_get_value(pdata->gpio_irq))
			break;
		printk(KERN_INFO "Touch: wait for Message(%d)\n", loop_i + 1);
		msleep(10);
	}

	i2c_atmel_read(ts->client, get_object_address(ts, GEN_MESSAGEPROCESSOR_T5), data, 9);
	printk(KERN_INFO "Touch: 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X\n",
		data[0], data[1], data[2], data[3], data[4], data[5], data[6]);
	return 0;
}


static void check_calibration(struct atmel_ts_data*ts)
  	{
	uint8_t data[2];
	uint8_t atch_and_tch[2];
	uint8_t tch_ch = 0, atch_ch = 0,loop_i;

	memset(data, 0xFF, sizeof(data));
	memset(atch_and_tch, 0, sizeof(atch_and_tch));
	
	i2c_atmel_write_byte_data(ts->client,
		get_object_address(ts, GEN_COMMANDPROCESSOR_T6) + 5, 0xF3);

	for (loop_i = 0; !(data[0] == 0xF3 && data[1] == 0x00) && loop_i < 100; loop_i++) {
		msleep(5);
		i2c_atmel_read(ts->client, get_object_address(ts, DIAGNOSTIC_T37), data, 2);
	}

	if (loop_i == 100){
		printk(KERN_ERR "%s: Diag data not ready\n", __func__);
		
		ts->calibration_confirm = 0;
		//reset
		i2c_atmel_write_byte_data(ts->client,
		get_object_address(ts, GEN_COMMANDPROCESSOR_T6), 0x1);	
		return ;
	}
	
	check_atch_and_tch(ts,atch_and_tch);
	atch_ch=atch_and_tch[0];
	tch_ch=atch_and_tch[1];	
	
#ifdef TOUCH_LONG_SLIDE
			printk("***********: check_calibration, tch_ch=%d, atch_ch=%d temp_flag:%d temp_flag2:%d\n", tch_ch, atch_ch,temp_flag,temp_flag2);
#else
			printk("***********: check_calibration, tch_ch=%d, atch_ch=%d\n", tch_ch, atch_ch);

#endif

	i2c_atmel_write_byte_data(ts->client,
		get_object_address(ts, GEN_COMMANDPROCESSOR_T6) + 5, 0x01);

	//20110531
	//对于这个tch_ch=0的条件，还是有疑问的
	//henk说不要这个条件，但没有这个条件的话，不能在第一时间对计时进行设置
	//影响了校准OK的时间，从而影响了RELEASE ALL FINGERS的时间，导致错误的释放
	//所以需要知道这个计时到底从什么时候开始
	if ((tch_ch>=0) && (atch_ch == 0)&&(tch_ch <40)) 
	{
		#ifdef TOUCH_LONG_SLIDE
		if ((jiffies > (ts->timestamp + HZ/2)||(jiffies<ts->timestamp && jiffies>HZ/2)) 
			&& (ts->calibration_confirm == 1)&&(temp_flag2==1)) 			
		{
			ts->calibration_confirm = 2;
			
			atch_cal_control(ts,0);
			
			auto_cal_set(ts,15);
			
			temp_flag2=0;
			#ifdef ATMEL_NEW_CAL
			ts->timestamp1=jiffies;
			#endif
			printk("***********: calibration is ok\n");
		}

		#else
		if (jiffies > (ts->timestamp + HZ/2) && (ts->calibration_confirm == 1)) 			
		{
			ts->calibration_confirm = 2;
			atch_cal_control(ts,0);
			printk("***********: calibration is ok\n");
		}
		#endif
		if (ts->calibration_confirm == 0)
		{   	
			ts->calibration_confirm = 1;
			ts->timestamp = jiffies;
		}
	} else 
			{
				if(((atch_ch>2)&&(tch_ch==0))||((tch_ch>0)&&(atch_ch>7))||(tch_ch>15)){
					ts->calibration_confirm = 0;
					i2c_atmel_write_byte_data(ts->client,
						get_object_address(ts, GEN_COMMANDPROCESSOR_T6) + 2, 0x55);
					#ifdef TOUCH_LONG_SLIDE
					temp_flag=0;
					temp_flag2=0;
					#endif
					ts->timestamp = jiffies;
					count = 0;
				}else {
					count++;
				}
				
				if (count >=50)
				{
					count = 0;
					ts->calibration_confirm = 0;
					
					//reset
					i2c_atmel_write_byte_data(ts->client,
					get_object_address(ts, GEN_COMMANDPROCESSOR_T6), 0x1);	
					msleep(100);
					i2c_atmel_write_byte_data(ts->client,
					get_object_address(ts, GEN_COMMANDPROCESSOR_T6) + 2, 0x55);
					#ifdef TOUCH_LONG_SLIDE
					temp_flag=0;
					temp_flag2=0;
					#endif
				}
	}

}


void get_finger_position(struct atmel_ts_data *ts, uint8_t report_type,uint8_t *data){
	//int ret;
			report_type = data[0] - ts->finger_type;
			if (report_type >= 0 && report_type < ts->finger_support) {
			/* for issue debug only */
			if ((data[ 1] & 0x60) == 0x60)
		{
			printk(KERN_INFO"x60 ISSUE happened: %x, %x, %x, %x, %x, %x, %x, %x\n",
			data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
		return;
	}
		if ((data[1] & 0x22) && (((ts->finger_pressed >> report_type) & 1) == 1)) {
				ts->finger_count--;
				ts->finger_pressed &= ~(1 << report_type);
			} 
			else if ((data[1] & 0xC0) && (((ts->finger_pressed >> report_type) & 1) == 0)) {
				ts->finger_count++;
				ts->finger_pressed |= 1 << report_type;
			}
		#ifdef ENABLE_IME_IMPROVEMENT
		if (ts->ime_threshold_pixel > 0)
			ime_report_filter(ts, data);
		#else
			ts->finger_data[report_type].x = data[2] << 2 | data[4] >> 6;
			ts->finger_data[report_type].y = data[3] << 2 | (data[4] & 0x0C) >> 2;
			ts->finger_data[report_type].w = data[5];
			ts->finger_data[report_type].z = data[6];
			#endif
		ts->finger_data[report_type].report_enable=2;
		if((data[1]&0x80)==0)
			{
			ts->finger_data[report_type].report_enable=1;/*The finger is up.*/
			ts->finger_data[report_type].z = 0;
			}
			}
}
void command_process(struct atmel_ts_data *ts, uint8_t *data){

	struct atmel_i2c_platform_data *pdata;
	pdata = ts->client->dev.platform_data;
	printk(KERN_INFO"command_process: %x, %x, %x, %x, %x, %x, %x, %x\n",
				data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7]);
			if ((data[1] & 0x08) == 0x08)
			{
				atmel_ts_load_config(ts);

			}
			else if (data[1] & 0x10)
			{
				#ifdef TOUCH_LONG_SLIDE
				temp_flag=0;
				temp_flag2=0;
				#endif
				ts->calibration_confirm = 0;
				#ifdef ATMEL_NEW_CAL
				new_cal_state=0;
				atch_cal_control(ts,1);
				new_cal_control(ts,1);
				#endif
				printk("wly: command calibration set\n");
			}
			else if (!(data[1]&0x10))
			{
				if (ts->calibration_confirm == 0)
				{
					release_all_fingers(ts);
					check_calibration(ts);
				}
			}

	
} 
void virtual_keys(struct atmel_ts_data *ts, uint8_t key_code, uint8_t key_state,uint8_t *data){
	//printk("huangjinyu v-key coming data[2] = %d \n",data[2]);
if(!ts->finger_count){
			key_state = !!(data[1]& 0x80);
      //huangjinyu ZTE_TS_HUANGJINYU_20110223 BEGIN
      #if defined(CONFIG_MACH_MOONCAKE)
			switch(data[2])
			{
				case 1:
					key_code = KEY_BACK;
					ts->finger_data[0].x = 200;
					break;
				case 2:
					key_code = KEY_HOME;
					ts->finger_data[0].x = 40;
					break;
				default:
				break;
			}
	/*ZTE_TS_ZFJ_20110425 begin*/
	  #elif defined(CONFIG_MACH_NOVA)
			switch(data[2])
			{
				case 1:
					key_code = KEY_MENU;
					ts->finger_data[0].x = 30;
					break;
				case 2:
					key_code = KEY_HOME;
					ts->finger_data[0].x = 110;
					break;
				case 4:
					key_code = KEY_BACK;
					ts->finger_data[0].x = 200;				
					break;
				case 8:
					key_code = KEY_SEARCH;
					ts->finger_data[0].x = 300;				
					break;
				default:
				break;
			}
	/*ZTE_TS_ZFJ_20110425 end*/
      #else
			switch(data[2])
			{
				case 1:
					key_code = KEY_HOME;
					ts->finger_data[0].x = 80;
					break;
				case 2:
					key_code = KEY_MENU;
					ts->finger_data[0].x = 240;
					break;
				case 4:
					key_code = KEY_BACK;
					ts->finger_data[0].x = 400;				
					break;
				default:
				break;
			}
			#if 0
			if (1 == data[2])
			{
				key_code = KEY_HOME;
				ts->finger_data[0].x = 80;
  
			}
			else if (2 == data[2])
			{
				key_code = KEY_MENU;
				ts->finger_data[0].x = 240;
			}
			else if(4 == data[2])
			{
				key_code = KEY_BACK;
				ts->finger_data[0].x = 400;
			}
			#endif
	  #endif
      //huangjinyu ZTE_TS_HUANGJINYU_20110223 end
			if (!key_state)
			{
				ts->finger_data[0].w = 0;
				ts->finger_data[0].z = 0;
			}
			else
			{
				ts->finger_data[0].w = 10;
				ts->finger_data[0].z = 255;
			}
      //huangjinyu ZTE_TS_HUANGJINYU_20110223 BEGIN
      #if defined(CONFIG_MACH_MOONCAKE) 
			ts->finger_data[0].y = 350;
      /*ZTE_TS_ZFJ_20110425 begin*/
	  #elif defined(CONFIG_MACH_NOVA)
			ts->finger_data[0].y = 520;
	  /*ZTE_TS_ZFJ_20110425 end*/
      #else
			ts->finger_data[0].y = 810;
	  #endif
	  //huangjinyu ZTE_TS_HUANGJINYU_20110223 end
			input_report_key(ts->input_dev, BTN_TOUCH, !!key_state);
			input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR,ts->finger_data[0].z);
			input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR,ts->finger_data[0].w);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_X,ts->finger_data[0].x);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_Y,ts->finger_data[0].y);

			//pr_err("huangjinyu report touch v-key major= %d ,width major = %d ,x = %d ,y = %d \n",ts->finger_data[0].z,ts->finger_data[0].w,ts->finger_data[0].x,ts->finger_data[0].y);

			input_mt_sync(ts->input_dev);	
			input_sync(ts->input_dev);
			}
}


static void atmel_ts_work_func(struct work_struct *work)
{
	int ret;
	
	struct atmel_ts_data *ts = container_of(work, struct atmel_ts_data, work);
	uint8_t data[9];
	//uint8_t atch_and_tch[2];

	//int16_t data1[8];
	int i;
	uint8_t loop_i =0, report_type =0, report_id=0,key_state = 0; 
	static uint8_t key_code = 0;
	ret = i2c_atmel_read(ts->client, get_object_address(ts,GEN_MESSAGEPROCESSOR_T5), data, 9);	
	report_id = data[0];
	//printk("huangjinyu11111 *******************************************\n");

	if (ts->calibration_confirm < 2 && (data[1] & 0x80) )
		{
				check_calibration(ts);

		}
	
	//memset(atch_and_tch, 0, sizeof(atch_and_tch));
	//check_atch_and_tch(ts,atch_and_tch);
	//printk("atch:%d,tch:%d\n",atch_and_tch[0],atch_and_tch[1]);
	/*printk(KERN_INFO"atmel_ts_work_func: %x, %x, %x, %x, %x, %x, %x, %x,ts->finger_count:%d\n",
				data[0], data[1], data[2], data[3], data[4], data[5], data[6], data[7],ts->finger_count);*/
	#ifdef ATMEL_NEW_CAL
	if(ts->calibration_confirm ==2){
		if(new_cal_state==0){
			if(jiffies > (ts->timestamp1 + 20*HZ)||(jiffies<ts->timestamp && jiffies>20*HZ)){
				new_cal_state=1;
				new_cal_control(ts,0);
			}
		}
	}
	#endif
	
	report_type=get_object_type_from_idnum(ts,report_id);
	switch(report_type)
	{
		case TOUCH_MULTITOUCHSCREEN_T9:{					
			get_finger_position(ts, report_type,data);
			for(i=0;i<ts->finger_support+1;i++)
			{
				ret = i2c_atmel_read(ts->client, get_object_address(ts,GEN_MESSAGEPROCESSOR_T5), data, 9);	
				if(get_object_type_from_idnum(ts,data[0])==TOUCH_MULTITOUCHSCREEN_T9)
				{
					
					report_id = data[0];
					get_finger_position(ts, data[0],data);
				}
				else
					break;
			}
			}break;
		case GEN_COMMANDPROCESSOR_T6:{command_process(ts, data);}break;
		case TOUCH_KEYARRAY_T15:{virtual_keys(ts, key_code, key_state,data);}break;
		case PROCI_GRIPFACESUPPRESSION_T20:{
				if((data[1]&0x01)==0x01){
					printk("face cal\n");
					i2c_atmel_write_byte_data(ts->client,
						get_object_address(ts, GEN_COMMANDPROCESSOR_T6) + 2, 0x55);
				}
				break;		
			}
		case PROCI_ONETOUCHGESTUREPROCESSOR_T24://T24,gesture and position
		{
			ts->finger_data[0].x = data[2] << 2 | data[4] >> 6;
			ts->finger_data[0].y = data[3] << 2 | (data[4] & 0x0C) >> 2;
		}break; 	 
		default:break;		
			
	}
	//printk("x: %d\ny: %d\n",ts->finger_data[report_type].x,ts->finger_data[report_type].y);

	if( report_type==TOUCH_MULTITOUCHSCREEN_T9 )
	{	
		for(loop_i =0; loop_i< ts->finger_support; loop_i ++ )
		{
			switch(ts->finger_data[loop_i].report_enable)
			{
				case 1:
				{
					ts->finger_data[loop_i].report_enable=0;
				};
				case 2:
				{
					input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID,loop_i);
					input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR,ts->finger_data[loop_i].z);
					input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR,ts->finger_data[loop_i].w);
					input_report_abs(ts->input_dev, ABS_MT_POSITION_X,ts->finger_data[loop_i].x);
					input_report_abs(ts->input_dev, ABS_MT_POSITION_Y,ts->finger_data[loop_i].y);	
					//pr_err("huangjinyu report touch major= %d ,width major = %d ,x = %d ,y = %d \n",ts->finger_data[loop_i].z,ts->finger_data[loop_i].w,ts->finger_data[loop_i].x,ts->finger_data[loop_i].y);
					input_mt_sync(ts->input_dev);	
				}break;
				default:break;
				}
			
		}
		input_sync(ts->input_dev);	
		//input_report_key(ts->input_dev, BTN_TOUCH, !!ts->finger_count);
		//input_sync(ts->input_dev);	
	}
	#ifdef TOUCH_LONG_SLIDE
	if(ts->calibration_confirm !=2){
	if((temp_flag==0)&&((ts->finger_data[0].report_enable==2))){
		x_value=ts->finger_data[0].x;
		y_value=ts->finger_data[0].y;
		temp_flag=1;
	}
	if(ts->finger_data[0].report_enable==0)
		temp_flag=0;
	
	for(loop_i =1; loop_i< ts->finger_support; loop_i ++ )
	{
		if (ts->finger_data[loop_i].report_enable!=0){
			temp_flag=0;
			//temp_flag2=0;
			break;
			}
	}	
	
	if((temp_flag==1)&&((ts->finger_data[0].report_enable==2))){
		if(((x_value-ts->finger_data[0].x)>(ts->abs_x_max/4))||
			((y_value-ts->finger_data[0].y)>(ts->abs_y_max/4))||
			((ts->finger_data[0].x-x_value)>(ts->abs_x_max/4))||
			((ts->finger_data[0].y-y_value)>(ts->abs_y_max/4))){
				temp_flag2=1;
				temp_flag=2;
				printk("huangjinyu long slide ok!***************************************\n");
			}
	}

	}
	#endif
	enable_irq(ts->client->irq);
}
static irqreturn_t atmel_ts_irq_handler(int irq, void *dev_id)
{
	struct atmel_ts_data *ts = dev_id;
	//printk("wly: atmel_ts_irq_handler\n");
	disable_irq_nosync(ts->client->irq);
	queue_work(ts->atmel_wq, &ts->work);
	return IRQ_HANDLED;
}


#if defined(CONFIG_ATMEL_FW_UPDATE)
/* Object types */
#define QT602240_DEBUG_DIAGNOSTIC	37
#define QT602240_GEN_MESSAGE		5
#define QT602240_GEN_COMMAND		6
#define QT602240_GEN_POWER		7
#define QT602240_GEN_ACQUIRE		8
#define QT602240_TOUCH_MULTI		9
#define QT602240_TOUCH_KEYARRAY		15
#define QT602240_TOUCH_PROXIMITY	23
#define QT602240_PROCI_GRIPFACE		20
#define QT602240_PROCG_NOISE		22
#define QT602240_PROCI_ONETOUCH		24
#define QT602240_PROCI_TWOTOUCH		27
#define QT602240_SPT_COMMSCONFIG	18	/* firmware ver 21 over */
#define QT602240_SPT_GPIOPWM		19
#define QT602240_SPT_SELFTEST		25
#define QT602240_SPT_CTECONFIG		28
#define QT602240_SPT_USERDATA		38	/* firmware ver 21 over */


/* QT602240_GEN_COMMAND field */
#define QT602240_COMMAND_RESET		0
#define QT602240_COMMAND_BACKUPNV	1

/* Define for QT602240_GEN_COMMAND */
#define QT602240_BOOT_VALUE		0xa5
#define QT602240_BACKUP_VALUE		0x55
#define QT602240_BACKUP_TIME		25	/* msec */
#define QT602240_RESET_TIME		65	/* msec */
#define QT602240_FWRESET_TIME		175	/* msec */

/* Slave addresses */
#define QT602240_APP_LOW		0x4a
#define QT602240_APP_HIGH		0x4b
#define QT602240_BOOT_LOW		0x24
#define QT602240_BOOT_HIGH		0x25
/* Bootloader mode status */
#define QT602240_WAITING_BOOTLOAD_CMD	0xc0	/* valid 7 6 bit only */
#define QT602240_WAITING_FRAME_DATA	0x80	/* valid 7 6 bit only */
#define QT602240_FRAME_CRC_CHECK	0x02
#define QT602240_FRAME_CRC_FAIL		0x03
#define QT602240_FRAME_CRC_PASS		0x04
#define QT602240_APP_CRC_FAIL		0x40	/* valid 7 8 bit only */
#define QT602240_BOOT_STATUS_MASK	0x3f
/* Version */
#define QT602240_VER_20			20
#define QT602240_VER_21			21
#define QT602240_VER_22			22

/* Firmware */
//后续要区分224和140不同的FW 重要
//7x30上暂时没有140的项目 7x30_4.1上n850为140的芯片
//需要重点关注的是7X27和7X27A
#define QT602240_FW_NAME_224		"mXT224C23_v1.1.09.enc"
#define QT602240_FW_NAME_140		"mXT140C27_v1.0.01.enc"

//#include QT602240_FW_NAME

#define QT602240_UNLOCK_CMD_MSB		0xaa
#define QT602240_UNLOCK_CMD_LSB		0xdc

/* Registers */
#define QT602240_FAMILY_ID		0x00
#define QT602240_VARIANT_ID		0x01
#define QT602240_VERSION		0x02
#define QT602240_BUILD			0x03
#define QT602240_MATRIX_X_SIZE		0x04
#define QT602240_MATRIX_Y_SIZE		0x05
#define QT602240_OBJECT_NUM		0x06
#define QT602240_OBJECT_START		0x07

#define QT602240_OBJECT_SIZE		6

#define QT602240_MAX_FINGER		10

static int qt602240_check_bootloader(struct i2c_client *client,
				     unsigned int state)
{
	u8 val;
	int i=0,loop_i,ret;
recheck:
	if (i2c_master_recv(client, &val, 1) != 1) {
		dev_err(&client->dev, "%s: i2c recv failed\n", __func__);
		i++;
		if(i<10)
			goto recheck;
		return -EIO;
	}
	printk("huangjinyu state:0x%x,val:0x%x\n",state,val);
	switch (state) {
	case QT602240_WAITING_BOOTLOAD_CMD:
	case QT602240_WAITING_FRAME_DATA:
		val &= ~QT602240_BOOT_STATUS_MASK;
		break;
	case QT602240_FRAME_CRC_PASS:
		if (val == QT602240_FRAME_CRC_CHECK){
	for (loop_i = 0; loop_i < 10; loop_i++) {
		ret = gpio_get_value(55);
		printk("wly:gpio 55 value = %d\n",ret);
		if (!ret/*gpio_get_value(intr)*/)
			break;
		msleep(10);
	}				
			goto recheck;
			}
		//if (val!=QT602240_FRAME_CRC_PASS){
			//printk("huangjinyu check crc again\n");
			//goto recheck;
		//}
		break;
	default:
		return -EINVAL;
	}
	if((state==0x4)&&(val==0x8a))
		return 0;
		
	if (val != state) {
		dev_err(&client->dev, "Unvalid bootloader mode state\n");
		return -EINVAL;
	}

	return 0;
}
static int qt602240_unlock_bootloader(struct i2c_client *client)
{
	u8 buf[2];

	buf[0] = QT602240_UNLOCK_CMD_LSB;
	buf[1] = QT602240_UNLOCK_CMD_MSB;

	if (i2c_master_send(client, buf, 2) != 2) {
		dev_err(&client->dev, "%s: i2c send failed\n", __func__);
		return -EIO;
	}

	return 0;
}
static int qt602240_fw_write(struct i2c_client *client,
			     const u8 *data, unsigned int frame_size)
{
	//int i;
	//for(i=0;i<=frame_size;i++)
		//printk("huangjinyu i:%d\n",i);
	if (i2c_master_send(client, data, frame_size) != frame_size) {
		dev_err(&client->dev, "%s: i2c send failed \n", __func__);
		return -EIO;
	}
	printk("huangjinyu qt602240_fw_write succeed!\n");
	return 0;
}



static int qt602240_load_fw(struct device *dev, const char *fn)
{
	//struct qt602240_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = ts_temp->client;
	const struct firmware *fw = NULL;
	unsigned int frame_size;
	unsigned int pos = 0;
	int ret;
	int j,loop_i;
	u8 *data,data0,data1;
	size_t size;
	ret = request_firmware(&fw, fn, dev);
	if (ret) {
		dev_err(dev, "Unable to open firmware %s\n", fn);
		return ret;
	}
	//change_firmware_asc2bin(&fw);
		data=kzalloc((fw->size/2)*sizeof(*data), GFP_KERNEL);
		for(j=0;j<fw->size/2;j++){
			
		if(fw->data[2*j]>=0x41)
			data0=fw->data[2*j]-0x41+10;
		else
			data0=fw->data[2*j]-0x30;
		
		if(fw->data[2*j+1]>=0x41)
			data1=fw->data[2*j+1]-0x41+10;
		else
			data1=fw->data[2*j+1]-0x30;		
		
		data[j]=(data0<<4)|(data1);
		
		//printk("0x%x,",data[j]);
		//if(j==10)
		//printk("\n");
		}
		//fw->data=data;
		size=fw->size/2;


		
	/* Change to the bootloader mode */
	//qt602240_write_object(data, QT602240_GEN_COMMAND,
			//QT602240_COMMAND_RESET, QT602240_BOOT_VALUE);
	i2c_atmel_write_byte_data(client,
	 	get_object_address(ts_temp, QT602240_GEN_COMMAND)+QT602240_COMMAND_RESET,
	 	QT602240_BOOT_VALUE);
	
	msleep(QT602240_RESET_TIME*2);

	/* Change to slave address of bootloader */
	//if (client->addr == QT602240_APP_LOW)
		client->addr = QT602240_BOOT_LOW;
	//else
		//client->addr = QT602240_BOOT_HIGH;

	ret = qt602240_check_bootloader(client, QT602240_WAITING_BOOTLOAD_CMD);
	if (ret)
		goto out;

	/* Unlock bootloader */
	qt602240_unlock_bootloader(client);

	while (pos < size) {

	for (loop_i = 0; loop_i < 10; loop_i++) {
		ret = gpio_get_value(55);
		printk("wly:gpio 55 value = %d\n",ret);
		if (!ret/*gpio_get_value(intr)*/)
			break;
		msleep(10);
	}		
		ret = qt602240_check_bootloader(client,
						QT602240_WAITING_FRAME_DATA);
		if (ret)
			goto out;

		frame_size = ((*(data + pos) << 8) | *(data + pos + 1));
		printk("data[0]:0x%x,data[1]:0x%x\n",*(data + pos),*(data + pos + 1));
		/* We should add 2 at frame size as the the firmware data is not
		 * included the CRC bytes.
		 */
		frame_size += 2;
		printk("frame data:\n");
		//for(i=0;i<frame_size;i++){
		//printk("0x%x,",data[pos+i]);
		//if(i==10)
			//printk("\n");
			//}
		printk("huangjinyu frame_size:%d,pos:%d,fw->size:%d\n",frame_size,pos,size);
		/* Write one frame to device */
		qt602240_fw_write(client, data + pos, frame_size);

		for (loop_i = 0; loop_i < 10; loop_i++) {
			ret = gpio_get_value(55);
			printk("wly:gpio 55 value = %d\n",ret);
			if (!ret/*gpio_get_value(intr)*/)
				break;
			msleep(10);
		}

		ret = qt602240_check_bootloader(client,
						QT602240_FRAME_CRC_PASS);
		if (ret)
			goto out;


		pos += frame_size;

		dev_dbg(dev, "Updated %d bytes / %zd bytes\n", pos, size);
	}
	


out:
	release_firmware(fw);

	/* Change to slave address of application */
	if (client->addr == QT602240_BOOT_LOW)
		client->addr = QT602240_APP_LOW;
	else
		client->addr = QT602240_APP_HIGH;

	return ret;
}
/* Each client has this additional data */

struct qt602240_finger {
	int status;
	int x;
	int y;
	int area;
};


struct qt602240_object {
	u8 type;
	u16 start_address;
	u8 size;
	u8 instances;
	u8 num_report_ids;

	/* to map object and message */
	u8 max_reportid;
};

static int qt602240_make_highchg(struct atmel_ts_data *data)
{
	struct device *dev = &data->client->dev;
	int count = 10;
	int error;
	u8 val;

	/* Read dummy message to make high CHG pin */
	do {
		//error = qt602240_read_object(data, QT602240_GEN_MESSAGE, 0, &val);
		error=i2c_atmel_read(data->client,
			get_object_address(data, QT602240_GEN_MESSAGE),&val,1);		
		if (error)
			return error;
	} while ((val != 0xff) && --count);

	if (!count) {
		dev_err(dev, "CHG pin isn't cleared\n");
		return -EBUSY;
	}

	return 0;
}

static int qt602240_initialize(struct atmel_ts_data *data)
{
	int error;
	//u8 val;

	atmel_ts_get_build_info(ts_temp);
	atmel_ts_get_table_info(ts_temp);

	error = qt602240_make_highchg(data);
	if (error)
		return error;

	atmel_ts_load_config(ts_temp);
	//qt602240_handle_pdata(data);

	/* Backup to memory */
	//qt602240_write_object(data, QT602240_GEN_COMMAND,
			//QT602240_COMMAND_BACKUPNV,
			//QT602240_BACKUP_VALUE);
	i2c_atmel_write_byte_data(ts_temp->client,
		get_object_address(ts_temp, QT602240_GEN_COMMAND) + QT602240_COMMAND_BACKUPNV, 
		QT602240_BACKUP_VALUE);
	printk("qt602240_initialize QT602240_BACKUP_VALUE succeed\n");
			
	msleep(QT602240_BACKUP_TIME);

	/* Soft reset */
	//qt602240_write_object(data, QT602240_GEN_COMMAND,
			//QT602240_COMMAND_RESET, 1);
	i2c_atmel_write_byte_data(ts_temp->client,
		get_object_address(ts_temp, QT602240_GEN_COMMAND) + QT602240_COMMAND_RESET, 1);
	
	msleep(QT602240_RESET_TIME);
	printk("qt602240_initialize reset succeed\n");

	/* Update matrix size at info struct */
	//error = qt602240_read_reg(client, QT602240_MATRIX_X_SIZE, &val);
	//if (error)
		//return error;
	//info->matrix_xsize = val;

	//error = qt602240_read_reg(client, QT602240_MATRIX_Y_SIZE, &val);
	//if (error)
		//return error;
	//info->matrix_ysize = val;

	//dev_info(&client->dev,
			//"Family ID: %d Variant ID: %d Version: %d Build: %d\n",
			//info->family_id, info->variant_id, info->version,
			//info->build);

	//dev_info(&client->dev,
			//"Matrix X Size: %d Matrix Y Size: %d Object Num: %d\n",
			//info->matrix_xsize, info->matrix_ysize,
			//info->object_num);

	return 0;
}

static uint8_t qt602240_update_fw(uint8_t vol)
{
	struct atmel_ts_data *data = ts_temp;
	struct device *dev=&(ts_temp->client->dev);
	//unsigned int version;
	int error;
	update_result_flag=3;
	#if 0 //这个版本判断暂时不要
	if (sscanf(buf, "%u", &version) != 1) {
		dev_err(dev, "Invalid values\n");
		return -EINVAL;
	}
	if (ts_temp->id->variant_id< QT602240_VER_21 || version < QT602240_VER_21) {
		dev_err(dev, "FW update supported starting with version 21\n");
		return -EINVAL;
	}
	#endif
	disable_irq(ts_temp->client->irq);
	
	if(ATMEL_CHIP_TYPE==ATMEL_224)
	error = qt602240_load_fw(dev, QT602240_FW_NAME_224);
	else if(ATMEL_CHIP_TYPE==ATMEL_140)
	error = qt602240_load_fw(dev, QT602240_FW_NAME_140);
	else
	error = 1;
	
	if (error) {
		printk("The firmware update failed(%d)\n", error);
		//count = error;
	} else {
		printk("The firmware update succeeded\n");

		/* Wait for reset */
		msleep(QT602240_FWRESET_TIME*2);

		//kfree(data->object_table);
		//data->object_table = NULL;

		qt602240_initialize(data);
	}
	update_result_flag=0;

	enable_irq(ts_temp->client->irq);

	return 0;
}
#if 1
static int qt602240_load_fw_probe(struct device *dev, const char *fn)
{
	//struct qt602240_data *data = dev_get_drvdata(dev);
	struct i2c_client *client = ts_temp->client;
	const struct firmware *fw = NULL;
	unsigned int frame_size;
	unsigned int pos = 0;
	int ret;
	int j,loop_i;
	u8 *data,data0,data1;
	size_t size;
	ret = request_firmware(&fw, fn, dev);
	if (ret) {
		dev_err(dev, "Unable to open firmware %s\n", fn);
		return ret;
	}
	//change_firmware_asc2bin(&fw);
		data=kzalloc((fw->size/2)*sizeof(*data), GFP_KERNEL);
		for(j=0;j<fw->size/2;j++){
			
		if(fw->data[2*j]>=0x41)
			data0=fw->data[2*j]-0x41+10;
		else
			data0=fw->data[2*j]-0x30;
		
		if(fw->data[2*j+1]>=0x41)
			data1=fw->data[2*j+1]-0x41+10;
		else
			data1=fw->data[2*j+1]-0x30;		
		
		data[j]=(data0<<4)|(data1);
		
		//printk("0x%x,",data[j]);
		//if(j==10)
		//printk("\n");
		}
		//fw->data=data;
		size=fw->size/2;


		
	/* Change to the bootloader mode */
	//qt602240_write_object(data, QT602240_GEN_COMMAND,
			//QT602240_COMMAND_RESET, QT602240_BOOT_VALUE);
	//i2c_atmel_write_byte_data(client,
	 	//get_object_address(ts_temp, QT602240_GEN_COMMAND)+QT602240_COMMAND_RESET,
	 	//QT602240_BOOT_VALUE);
	
	msleep(QT602240_RESET_TIME);

	/* Change to slave address of bootloader */
	//if (client->addr == QT602240_APP_LOW)
		client->addr = QT602240_BOOT_LOW;
	//else
		//client->addr = QT602240_BOOT_HIGH;

	ret = qt602240_check_bootloader(client, QT602240_WAITING_BOOTLOAD_CMD);
	if (ret)
		goto out;

	/* Unlock bootloader */
	qt602240_unlock_bootloader(client);

	while (pos < size) {

	for (loop_i = 0; loop_i < 10; loop_i++) {
		ret = gpio_get_value(55);
		printk("wly:gpio 55 value = %d\n",ret);
		if (!ret/*gpio_get_value(intr)*/)
			break;
		msleep(10);
	}		
		ret = qt602240_check_bootloader(client,
						QT602240_WAITING_FRAME_DATA);
		if (ret)
			goto out;

		frame_size = ((*(data + pos) << 8) | *(data + pos + 1));
		printk("data[0]:0x%x,data[1]:0x%x\n",*(data + pos),*(data + pos + 1));
		/* We should add 2 at frame size as the the firmware data is not
		 * included the CRC bytes.
		 */
		frame_size += 2;
		printk("frame data:\n");
		//for(i=0;i<frame_size;i++){
		//printk("0x%x,",data[pos+i]);
		//if(i==10)
			//printk("\n");
			//}
		printk("huangjinyu frame_size:%d,pos:%d,fw->size:%d\n",frame_size,pos,size);
		/* Write one frame to device */
		qt602240_fw_write(client, data + pos, frame_size);

		for (loop_i = 0; loop_i < 10; loop_i++) {
			ret = gpio_get_value(55);
			printk("wly:gpio 55 value = %d\n",ret);
			if (!ret/*gpio_get_value(intr)*/)
				break;
			msleep(10);
		}

		ret = qt602240_check_bootloader(client,
						QT602240_FRAME_CRC_PASS);
		if (ret)
			goto out;


		pos += frame_size;

		dev_dbg(dev, "Updated %d bytes / %zd bytes\n", pos, size);
	}
	


out:
	release_firmware(fw);

	/* Change to slave address of application */
	if (client->addr == QT602240_BOOT_LOW)
		client->addr = QT602240_APP_LOW;
	else
		client->addr = QT602240_APP_HIGH;

	return ret;
}

static uint8_t qt602240_update_fw_probe(uint8_t vol)
{
	struct atmel_ts_data *data = ts_temp;
	struct device *dev=&(ts_temp->client->dev);
	//unsigned int version;
	int error;
	update_result_flag=3;
	
	#if 0 //这个版本判断暂时不要
	if (sscanf(buf, "%u", &version) != 1) {
		dev_err(dev, "Invalid values\n");
		return -EINVAL;
	}
	if (ts_temp->id->variant_id< QT602240_VER_21 || version < QT602240_VER_21) {
		dev_err(dev, "FW update supported starting with version 21\n");
		return -EINVAL;
	}
	#endif
	disable_irq(ts_temp->client->irq);
	if(ATMEL_CHIP_TYPE==ATMEL_224)
	error = qt602240_load_fw_probe(dev, QT602240_FW_NAME_224);
	else if(ATMEL_CHIP_TYPE==ATMEL_140)
	error = qt602240_load_fw_probe(dev, QT602240_FW_NAME_140);
	else
	error = 1;

	if (error) {
		dev_err(dev, "The firmware update failed(%d)\n", error);
		//count = error;
	} else {
		dev_dbg(dev, "The firmware update succeeded\n");

		/* Wait for reset */
		msleep(QT602240_FWRESET_TIME*2);

		//kfree(data->object_table);
		//data->object_table = NULL;

		qt602240_initialize(data);
	}
	update_result_flag=0;

	enable_irq(ts_temp->client->irq);

	return 0;
}
#endif
#endif
static int
proc_read_val(char *page, char **start, off_t off, int count, int *eof,
	  void *data)
{
	int len = 0;
	len += sprintf(page + len, "%s\n", "touchscreen module");
	len += sprintf(page + len, "name     : %s\n", "atmel");
	len += sprintf(page + len, "i2c address  : 0x%x\n", 0x4a);
	if(ATMEL_CHIP_TYPE==ATMEL_140)
	len += sprintf(page + len, "IC type           : %s\n", "atmel 140");	
	else if(ATMEL_CHIP_TYPE==ATMEL_224)
	len += sprintf(page + len, "IC type           : %s\n", "atmel 224");
	else 
	len += sprintf(page + len, "IC type 		  : %s\n", "atmel unknow");
	
	len += sprintf(page + len, "family id         : 0x%x\n", ts_temp->id->family_id);
	len += sprintf(page + len, "variant id        : 0x%x\n", ts_temp->id->variant_id);
	len += sprintf(page + len, "firmware version  : 0x%x\n", ts_temp->id->version);
	len += sprintf(page + len, "build id  : 0x%x\n", ts_temp->id->build);
	#if defined(CONFIG_MACH_BLADE2)||defined(CONFIG_MACH_BLUETICK)
	len += sprintf(page + len, "module            : %s\n", "atmel + Truly");
	#else
	len += sprintf(page + len, "module            : %s\n", "atmel + WINTEK");
	#endif
#if defined (CONFIG_ATMEL_FW_UPDATE)
        len += sprintf(page + len, "update flag : 0x%x\n", update_result_flag);
#endif

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
#ifdef CONFIG_ATMEL_FW_UPDATE
	printk("atmel fw update begin ******************************\n");
	update_result_flag=0;
	qt602240_update_fw(1);
	update_result_flag=2;
	printk("atmel fw update end ******************************\n");
#endif

	sscanf(buffer, "%lu", &val);
				
	return -EINVAL;
}

static int atmel_ts_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	struct atmel_ts_data *ts;
	struct atmel_i2c_platform_data *pdata;
	int ret = 0, intr = 0;
	uint8_t loop_i;
	uint8_t data[16];
	uint8_t CRC_check = 0;
	struct proc_dir_entry *dir, *refresh;//ZTE_WLY_CRDB00509514	
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk(KERN_ERR"%s: need I2C_FUNC_I2C\n", __func__);
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}

	ts = kzalloc(sizeof(struct atmel_ts_data), GFP_KERNEL);
	if (ts == NULL) {
		printk(KERN_ERR"%s: allocate atmel_ts_data failed\n", __func__);
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}
	
	ts_temp=ts;
	
	ts->atmel_wq = create_singlethread_workqueue("atmel_wq");
	if (!ts->atmel_wq) {
		printk(KERN_ERR"%s: create workqueue failed\n", __func__);
		ret = -ENOMEM;
		goto err_cread_wq_failed;
	}

	INIT_WORK(&ts->work, atmel_ts_work_func);
	ts->client = client;
	ts->calibration_confirm = 0;
	i2c_set_clientdata(client, ts);
	pdata = client->dev.platform_data;

	if (pdata) {
		ts->power = pdata->power;
		intr = pdata->gpio_irq;
	}
	//huangjinyu ZTE_TS_HJY_20110223 BEGIN
	else
	{
		printk(KERN_WARNING"atmel ts:there is no platform_data!\n");
	}
	//huangjinyu ZTE_TS_HJY_20110223 END
	if (ts->power) {
		ret = ts->power(1);
		msleep(200);
		if (ret < 0) {
			printk(KERN_ERR "%s:power on failed\n", __func__);
			goto err_power_failed;
		}
	}
		ret = gpio_request(intr, "gpio irq");
	if (ret) {
		pr_err("%s: unable to request gpio %d (%d)\n",
			__func__, intr,ret);
			goto err_power_failed;
	}

	for (loop_i = 0; loop_i < 10; loop_i++) {
		ret = gpio_get_value(intr);
		printk("wly:gpio %d value = %d\n",intr,ret);
		if (!ret/*gpio_get_value(intr)*/)
			break;
		msleep(10);
	}

	if (loop_i == 10)
	printk(KERN_ERR "No Messages\n");
	
  	printk("wly: %s, ts->client->addr=%d\n", __FUNCTION__,ts->client->addr);
#if defined(CONFIG_ATMEL_FW_UPDATE)
	//again:	
	/* read message*/
	ret = i2c_atmel_read(client, 0x0000, data, 7);
	if (ret < 0) {
		printk(KERN_INFO "No Atmel chip inside\n");
		goto i2c_addr_wrong;
	}
	
	printk(KERN_INFO "Touch: 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X\n",
		data[0], data[1], data[2], data[3], data[4], data[5], data[6]);	
	goto i2c_addr_right;
	i2c_addr_wrong:
		client->addr=0x24;
		ret = i2c_atmel_read(client, 0x0000, data, 1);
		if (ret < 0) {
		printk(KERN_INFO "No Atmel chip inside\n");
		goto err_detect_failed;
	}	
		qt602240_update_fw_probe(1);
		client->addr=0x4a;
	i2c_addr_right:
#else
	/* read message*/
	ret = i2c_atmel_read(client, 0x0000, data, 7);
	if (ret < 0) {
		printk(KERN_INFO "No Atmel chip inside\n");
		goto err_detect_failed;
	}
	printk(KERN_INFO "Touch: 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X 0x%2.2X\n",
		data[0], data[1], data[2], data[3], data[4], data[5], data[6]);
#endif

	ts->id = kzalloc(sizeof(struct info_id_t), GFP_KERNEL);
	if (ts->id == NULL) {
		printk(KERN_ERR"%s: allocate info_id_t failed\n", __func__);
		goto err_alloc_failed;
	}

	atmel_ts_get_build_info(ts);

	/* Read object table. */
	ts->object_table = kzalloc(sizeof(struct object_t)*ts->id->num_declared_objects, GFP_KERNEL);
	if (ts->object_table == NULL) {
		printk(KERN_ERR"%s: allocate object_table failed\n", __func__);
		goto err_alloc_failed;
	}
	atmel_ts_get_table_info(ts);
	//后续有新的芯片加入的时候，需要区分不同的芯片
	if((ts->id->family_id==0x80)&&(ts->id->variant_id==0x13)){
		ATMEL_CHIP_TYPE=ATMEL_224;
		
		#ifdef CONFIG_ATMEL_FW_UPDATE
		if((ts->id->version!=0x11)||(ts->id->build!=0x09)){
			printk("atmel touch screen fw auto update!\n");
			qt602240_update_fw(1);
			//不需要再回到前面去，要做的工作在固件升级结束后自动完成各个结构体的再填充
			//将CRC填写正确后后面的config也不会重复写入
			//goto again;
		}
		#endif
		}else if((ts->id->family_id==0x80)&&((ts->id->variant_id==0x16)||(ts->id->variant_id==0x07))){
			ATMEL_CHIP_TYPE=ATMEL_140;
#ifdef CONFIG_ATMEL_FW_UPDATE
			if((ts->id->version!=0x10)||(ts->id->build!=0x01)){
				printk("atmel touch screen fw auto update!\n");
				qt602240_update_fw(1);
			}
#endif
		}

	else
		ATMEL_CHIP_TYPE=ATMEL_UNKNOW;
	
	if (pdata) {
		ts->finger_support = pdata->config_T9[14];
		printk(KERN_INFO"finger_type: %d, max finger: %d\n", ts->finger_type, ts->finger_support);

		/*infoamtion block CRC check */
		if (pdata->object_crc[0]) {
			ret = i2c_atmel_write_byte_data(client,
				get_object_address(ts, GEN_COMMANDPROCESSOR_T6) + 1, 0x55);
			msleep(64);
			for (loop_i = 0; loop_i < 10; loop_i++) {
				ret = i2c_atmel_read(ts->client, get_object_address(ts,
					GEN_MESSAGEPROCESSOR_T5), data, 9);
					printk(KERN_INFO "crc checksum: %2d, %2d,  %2d\n",data[2],data[3],data[4]);
				  if (data[0] == get_report_ids_size(ts, GEN_COMMANDPROCESSOR_T6))
					break;
				msleep(5);
			}
			for (loop_i = 0; loop_i < 3; loop_i++) {
				if (pdata->object_crc[loop_i] != data[loop_i + 2]) {
					printk(KERN_ERR"CRC Error: %x, %x\n", pdata->object_crc[loop_i], data[loop_i + 2]);
					break;
				}
			}
			if (loop_i == 3) {
				printk(KERN_INFO "CRC passed: ");
				for (loop_i = 0; loop_i < 3; loop_i++)
					printk("0x%2.2X ", pdata->object_crc[loop_i]);
				printk("\n");
				CRC_check = 1;
			}
		}
		ts->abs_x_min = pdata->abs_x_min;
		ts->abs_x_max = pdata->abs_x_max;
		ts->abs_y_min = pdata->abs_y_min;
		ts->abs_y_max = pdata->abs_y_max;
		ts->abs_pressure_min = pdata->abs_pressure_min;
		ts->abs_pressure_max = pdata->abs_pressure_max;
		ts->abs_width_min = pdata->abs_width_min;
		ts->abs_width_max = pdata->abs_width_max;
		ts->GCAF_level = pdata->GCAF_level;
		printk(KERN_INFO "GCAF_level: %d, %d, %d, %d\n",
			ts->GCAF_level[0], ts->GCAF_level[1],ts->GCAF_level[2], ts->GCAF_level[3]);
		ts->filter_level = pdata->filter_level;
		printk(KERN_INFO "filter_level: %d, %d, %d, %d\n",
			ts->filter_level[0], ts->filter_level[1], ts->filter_level[2], ts->filter_level[3]);
#ifdef ENABLE_IME_IMPROVEMENT
		ts->display_width = pdata->display_width;
		ts->display_height = pdata->display_height;
		if (!ts->display_width || !ts->display_height)
			ts->display_width = ts->display_height = 1;
#endif
		ts->config_setting[0].config_T7
			= ts->config_setting[1].config_T7
			= pdata->config_T7;
		ts->config_setting[0].config_T8
			= ts->config_setting[1].config_T8
			= pdata->config_T8;
		ts->config_setting[0].config_T9 = pdata->config_T9;
		ts->config_setting[0].config_T20 = pdata->config_T20;
		ts->config_setting[0].config_T22 = pdata->config_T22;
		ts->config_setting[0].config_T28 = pdata->config_T28;
#if defined(CONFIG_TS_NOTIFIER)		
		ts->config_setting[0].config_T9_charge = pdata->config_T9_charge;
		ts->config_setting[0].config_T28_charge = pdata->config_T28_charge;
#endif
		if (pdata->cable_config[0]) {
			ts->config_setting[0].config[0] = pdata->config_T9[7];
			ts->config_setting[0].config[1] = pdata->config_T22[8];
			ts->config_setting[0].config[2] = pdata->config_T28[3];
			ts->config_setting[0].config[3] = pdata->config_T28[4];
			for (loop_i = 0; loop_i < 4; loop_i++)
				ts->config_setting[1].config[loop_i] = pdata->cable_config[loop_i];
			ts->GCAF_sample = ts->config_setting[1].config[3];
			ts->noisethr = pdata->cable_config[1];
		} else {
			if (pdata->cable_config_T7[0])
				ts->config_setting[1].config_T7 = pdata->cable_config_T7;
			if (pdata->cable_config_T8[0])
				ts->config_setting[1].config_T8 = pdata->cable_config_T8;
			if (pdata->cable_config_T9[0]) {
				ts->config_setting[1].config_T9 = pdata->cable_config_T9;
				ts->config_setting[1].config_T22 = pdata->cable_config_T22;
				ts->config_setting[1].config_T28 = pdata->cable_config_T28;
				ts->GCAF_sample = ts->config_setting[0].config_T28[4];
			}
		}
		if (!CRC_check) 
		{
			atmel_ts_load_config(ts);
		}
	}
	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) {
		ret = -ENOMEM;
		dev_err(&client->dev, "Failed to allocate input device\n");
		goto err_input_dev_alloc_failed;
	}
	ts->input_dev->name = "atmel-touchscreen";
	set_bit(EV_SYN, ts->input_dev->evbit);
	set_bit(EV_KEY, ts->input_dev->evbit);
	set_bit(BTN_TOUCH, ts->input_dev->keybit);
	set_bit(BTN_2, ts->input_dev->keybit);
	set_bit(EV_ABS, ts->input_dev->evbit);


	set_bit(KEY_HOME, ts->input_dev->keybit);
	set_bit(KEY_MENU, ts->input_dev->keybit);
	set_bit(KEY_BACK, ts->input_dev->keybit);
	set_bit(KEY_SEARCH, ts->input_dev->keybit);
	
	input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID,
				0, 10, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X,
				ts->abs_x_min, ts->abs_x_max, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y,
				ts->abs_y_min, ts->abs_y_max, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR,
				ts->abs_pressure_min, ts->abs_pressure_max,
				0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR,
				ts->abs_width_min, ts->abs_width_max, 0, 0);
	ret = input_register_device(ts->input_dev);
	if (ret) {
		dev_err(&client->dev,
			"atmel_ts_probe: Unable to register %s input device\n",
			ts->input_dev->name);
		goto err_input_register_device_failed;
	}

	ret = request_irq(client->irq, atmel_ts_irq_handler, IRQF_TRIGGER_LOW,//IRQF_TRIGGER_FALLING,
			client->name, ts);
	if (ret)
		dev_err(&client->dev, "request_irq failed\n");

	//enable_irq(client->irq);

		//ADD BY HUANGJINYU
		printk("atmel probe set the calibration\n");

		i2c_atmel_write_byte_data(client,
			get_object_address(ts, GEN_COMMANDPROCESSOR_T6) + 2, 0x55);
		atch_cal_control(ts,1);
		//ADD BY HUANGJINYU END
		
#ifdef ENABLE_IME_IMPROVEMENT
	ts->ime_threshold_pixel = 1;
	ts->ime_finger_report[0] = -1;
	ts->ime_finger_report[1] = -1;
	ret = device_create_file(&ts->input_dev->dev, &dev_attr_ime_threshold);
	if (ret) {
		printk(KERN_ERR "ENABLE_IME_IMPROVEMENT: "
					"Error to create ime_threshold\n");
		goto err_input_register_device_failed;
	}
	ret = device_create_file(&ts->input_dev->dev, &dev_attr_ime_work_area);
	if (ret) {
		printk(KERN_ERR "ENABLE_IME_IMPROVEMENT: "
					"Error to create ime_work_area\n");
		device_remove_file(&ts->input_dev->dev,
					   &dev_attr_ime_threshold);
		goto err_input_register_device_failed;
	}
#endif

#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = atmel_ts_early_suspend;
	ts->early_suspend.resume = atmel_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif

	private_ts = ts;

/*ZTE_TOUCH_WLY_008,@2010-01-19,begin*/
//ZTE_WLY_CRDB00509514,BEGIN
  dir = proc_mkdir("touchscreen", NULL);
	refresh = create_proc_entry("ts_information", 0777, dir);
//ZTE_WLY_CRDB00509514,END
	if (refresh) {
		refresh->data		= NULL;
		refresh->read_proc  = proc_read_val;
		refresh->write_proc = proc_write_val;
	}
/*ZTE_TOUCH_WLY_008,@2010-01-19,end*/

#ifdef ATMEL_EN_SYSFS
	atmel_touch_sysfs_init();
#endif

	dev_info(&client->dev, "Start touchscreen %s in interrupt mode\n",
			ts->input_dev->name);
#ifdef CONFIG_TS_NOTIFIER
		register_atmel_ts_notifier(&atmel_ts_notifier);
#endif

	return 0;

err_input_register_device_failed:
	input_free_device(ts->input_dev);

err_input_dev_alloc_failed:
err_alloc_failed:
err_detect_failed:
err_power_failed:
	destroy_workqueue(ts->atmel_wq);
	gpio_free(intr);

err_cread_wq_failed:
	kfree(ts);

err_alloc_data_failed:
err_check_functionality_failed:

	return ret;
}

static int atmel_ts_remove(struct i2c_client *client)
{
	struct atmel_ts_data *ts = i2c_get_clientdata(client);

#ifdef ATMEL_EN_SYSFS
	atmel_touch_sysfs_deinit();
#endif
#ifdef ENABLE_IME_IMPROVEMENT
	device_remove_file(&ts->input_dev->dev, &dev_attr_ime_threshold);
	device_remove_file(&ts->input_dev->dev, &dev_attr_ime_work_area);
#endif

	unregister_early_suspend(&ts->early_suspend);
	free_irq(client->irq, ts);

	destroy_workqueue(ts->atmel_wq);
	input_unregister_device(ts->input_dev);
	kfree(ts);

	return 0;
}

static int atmel_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int ret;
	struct atmel_ts_data *ts = i2c_get_clientdata(client);

	printk(KERN_INFO "%s: enter\n", __func__);

	disable_irq(ts->client->irq);

	ret = cancel_work_sync(&ts->work);
	if (ret)
		enable_irq(ts->client->irq);
	count=0;

	ts->finger_pressed = 0;
	ts->finger_count = 0;

	i2c_atmel_write_byte_data(client,
		get_object_address(ts, GEN_POWERCONFIG_T7), 0x0);
	i2c_atmel_write_byte_data(client,
		get_object_address(ts, GEN_POWERCONFIG_T7) + 1, 0x0);
	//ts->power(0);
	//release_all_fingers(ts);
	return 0;
}

static int atmel_ts_resume(struct i2c_client *client)
{
	struct atmel_ts_data *ts = i2c_get_clientdata(client);
	//ts->power(1);
	//msleep(200);

	i2c_atmel_write(ts->client, get_object_address(ts, GEN_POWERCONFIG_T7),
		ts->config_setting[0].config_T7, get_object_size(ts, GEN_POWERCONFIG_T7));
	printk("***********: go to check calibration\n");
		ts->calibration_confirm = 0;
		release_all_fingers(ts);

		i2c_atmel_write_byte_data(client,
			get_object_address(ts, GEN_COMMANDPROCESSOR_T6) + 2, 0x55);
		#ifdef TOUCH_LONG_SLIDE
		temp_flag=0;
		temp_flag2=0;
		#endif
		atch_cal_control(ts,1);
		
		#ifdef ATMEL_NEW_CAL
		new_cal_control(ts,1);
		auto_cal_set(ts,10);
		#endif
	enable_irq(client->irq);
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void atmel_ts_early_suspend(struct early_suspend *h)
{
	struct atmel_ts_data *ts;
	ts = container_of(h, struct atmel_ts_data, early_suspend);
	atmel_ts_suspend(ts->client, PMSG_SUSPEND);
}

static void atmel_ts_late_resume(struct early_suspend *h)
{
	struct atmel_ts_data *ts;
	ts = container_of(h, struct atmel_ts_data, early_suspend);
	atmel_ts_resume(ts->client);
}
#endif

static const struct i2c_device_id atml_ts_i2c_id[] = {
	{ ATMEL_QT602240_NAME, 0 },
	{ }
};

static struct i2c_driver atmel_ts_driver = {
	.id_table = atml_ts_i2c_id,
	.probe = atmel_ts_probe,
	.remove = atmel_ts_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend = atmel_ts_suspend,
	.resume = atmel_ts_resume,
#endif
	.driver = {
			.name = ATMEL_QT602240_NAME,
	},
};

static int __devinit atmel_ts_init(void)
{
	printk(KERN_INFO "atmel_ts_init():\n");
	return i2c_add_driver(&atmel_ts_driver);
}

static void __exit atmel_ts_exit(void)
{
	i2c_del_driver(&atmel_ts_driver);
}

module_init(atmel_ts_init);
module_exit(atmel_ts_exit);

MODULE_DESCRIPTION("ATMEL Touch driver");
MODULE_LICENSE("GPL");

