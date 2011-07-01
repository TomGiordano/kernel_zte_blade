/*
 *  isa1200.h - ISA1200 Haptic Motor driver
 *
 *  Copyright (C) 2009 Samsung Electronics
 *  Kyungmin Park <kyungmin.park@samsung.com>
 *  Copyright (c) 2010, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __LINUX_ISA1200_H
#define __LINUX_ISA1200_H

struct isa1200_platform_data {
	char name[30];
	int pwm_ch_id; /* pwm channel id */
	int hap_en_gpio; /* haptic enable gpio */
	int max_timeout;
	int (*power_on)(int on);
};

#endif /* __LINUX_ISA1200_H */
