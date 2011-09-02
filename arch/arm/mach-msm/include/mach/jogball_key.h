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

/* zhanglei, 2010-03-09, begin*/

struct jogball_key_platform_data {
	int gpio_irq_up;
	int gpio_irq_down;
	int gpio_irq_left;
	int gpio_irq_right;
};

/* zhanglei, 2010-03-09, end */
