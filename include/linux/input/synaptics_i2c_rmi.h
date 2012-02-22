/*
 * include/linux/synaptics_i2c_rmi.h - platform data structure for f75375s sensor
 *
 * Copyright (C) 2008 Google, Inc.
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

#ifndef _LINUX_SYNAPTICS_I2C_RMI_H
#define _LINUX_SYNAPTICS_I2C_RMI_H

#include <linux/interrupt.h>
#include <linux/earlysuspend.h>


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

#define SYNAPTICS_I2C_RMI4_NAME "synaptics-rmi4-ts"
#define PDT_PAGE_SELECT	0xFF
#define PDT_PROPERTIES	0xEF

#define PDT_ADDR_BASE	0xEE
#define PDT_BYTE_COUNT	6

enum{
	TS_POWER_ON=0,
	TS_RESUME,
	TS_SUSPEND,
};

struct f11_2d_point_data {
	__u8 xh;
	__u8 yh;
	__u8 xyl;
	__u8 wxy;
	__u8 z;
};

struct rmi4_function_descriptor {
	__u8 query_base;
	__u8 cmd_base;
	__u8 ctrl_base;
	__u8 data_base;
	__u8 intr_src_count;
	__u8 function_number;
};

struct rmi4_function_info {
	__u8 flag;	//0-function invalid.otherwise function valid.
	__u8 points_supported;
	__u8 intr_offset;
	__u8 intr_mask;
	__u8 ctrl_base;
	__u8 query_base;
	__u8 data_offset;
	__u8 data_len;
};

//#define SYNAPTICS_RMI4_MAXP_NEEDED	5
#define SYNAPTICS_RMI4_MAXP_NEEDED	2

struct synaptics_rmi4_data{
	__u32 version;	/* Use this entry for panels with */
				/* (major << 8 | minor) version or above. */
				/* If non-zero another array entry follows */

	__u16 addr;
	struct i2c_client *client;
	struct input_dev *input_dev;
	int use_irq;
	int gpio_irq;
	struct hrtimer timer;
	//struct hrtimer resume_timer;  //ZTE_WLY_CRDB00512790
	struct work_struct  work;
	__u16 max[2];			// maxmum x/y position supported
	struct early_suspend early_suspend;
	__u32 dup_threshold;    //ZTE_XUKE_TOUCH_THRESHOLD_20100201

	__u8 points_supported;
	__u8 orientation;

	__u8 data_base;
	__u8 data_len;

	struct rmi4_function_info	f01;
	struct rmi4_function_info	f11;
//	struct rmi4_function_info	f34;

	int (*power)(int on);	/* Only valid in first array entry */
};


#endif /* _LINUX_SYNAPTICS_I2C_RMI_H */
