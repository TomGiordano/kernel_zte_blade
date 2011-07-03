/*
 * Copyright (C) 2009 ZTE,  Corporation.
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
when             who             what, where, why                                                                            comment tag
---------     -------       ------------------------------------------------------- 
2010-06-30     wly          change camera kecode                ZTE_WLY_CRDB00516832
2010-04-19     wly          reconfig keypad                     ZTE_KEYBOARD_WLY_003
2009-12-29     wly          support 2G+4G or 2G+2G board        ZTE_KEYBOARD_WLY_002
2009-12-11     wly          support ftm mode          					ZTE_FTM_MODE_WLY_001
=========================================================================================*/

#include <linux/platform_device.h>
#include <linux/gpio_event.h>

#include <asm/mach-types.h>

/* chenjun, 2009-07-01, begin */
// chenjun: 行output_gpios
static unsigned int keypad_row_gpios[] = {28, 33, 34}; // {31, 32, 33, 34, 35, 41}  

// chenjun: 列input_gpios
static unsigned int keypad_col_gpios[] = {41, 40, 37, 36}; // { 36, 37, 38, 39, 40 }
/* chenjun, 2009-07-01, end */

#define KEYMAP_INDEX(row, col) ((row)*ARRAY_SIZE(keypad_col_gpios) + (col))

static const unsigned short keypad_keymap_joe[ARRAY_SIZE(keypad_col_gpios)
                                              * ARRAY_SIZE(keypad_row_gpios)] = {
	/* row, col */
	[KEYMAP_INDEX(0, 0)] = KEY_VOLUMEDOWN,     //下侧键
	[KEYMAP_INDEX(0, 1)] = KEY_OK,    //first camera
	[KEYMAP_INDEX(0, 2)] = KEY_VOLUMEUP,    //上侧键
	[KEYMAP_INDEX(0, 3)] = KEY_CAMERA,      //second camera
	/* row, col */
	[KEYMAP_INDEX(1, 0)] = KEY_RESERVED, //not used in joe !!!!
	[KEYMAP_INDEX(1, 1)] = KEY_HOME,    //left
	[KEYMAP_INDEX(1, 2)] = KEY_MENU,   //middle key
	[KEYMAP_INDEX(1, 3)] = KEY_BACK, //right key
	[KEYMAP_INDEX(2, 0)] = KEY_RESERVED, //not used in joe !!!!
	[KEYMAP_INDEX(2, 1)] = KEY_SEARCH,    //search
	[KEYMAP_INDEX(2, 2)] = KEY_RESERVED,   //not used in joe
	[KEYMAP_INDEX(2, 3)] = KEY_RESERVED, //not used in joe
};

/* JOE keypad platform device information */
static struct gpio_event_matrix_info joe_keypad_matrix_info = {
	.info.func	= gpio_event_matrix_func,
	.keymap		    = keypad_keymap_joe,
	.output_gpios	= keypad_row_gpios,
	.input_gpios	= keypad_col_gpios,
	.noutputs	    = ARRAY_SIZE(keypad_row_gpios),
	.ninputs	    = ARRAY_SIZE(keypad_col_gpios),
	.settle_time.tv.nsec    = 0,
	.poll_time.tv.nsec      = 20 * NSEC_PER_MSEC,
#if 1 // chenjun
	.flags		= GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_DRIVE_INACTIVE |
			  GPIOKPF_PRINT_UNMAPPED_KEYS
#else
	.flags		= GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_DRIVE_INACTIVE | GPIOKPF_ACTIVE_HIGH | GPIOKPF_PRINT_UNMAPPED_KEYS /*| GPIOKPF_PRINT_MAPPED_KEYS*/
#endif
};

static struct gpio_event_info *joe_keypad_info[] = {
	&joe_keypad_matrix_info.info
};

static struct gpio_event_platform_data joe_keypad_data = {
	.name		= "joe_keypad",
	.info		= joe_keypad_info,
	.info_count	= ARRAY_SIZE(joe_keypad_info)
};

struct platform_device keypad_device_joe = {
	.name	= GPIO_EVENT_DEV_NAME,
	.id     = -1,
	.dev	= {
		.platform_data	= &joe_keypad_data,
	},
};

#ifdef CONFIG_ZTE_FTM_FLAG_SUPPORT
extern int zte_get_ftm_flag(void);
#endif

static int __init joe_init_keypad(void)
{
        
	#ifdef CONFIG_ZTE_FTM_FLAG_SUPPORT
	int ftm_flag;
	ftm_flag = zte_get_ftm_flag();
	if (1 ==ftm_flag)return 0;
	#endif
        
	joe_keypad_matrix_info.keymap = keypad_keymap_joe;
	return platform_device_register(&keypad_device_joe);
}

device_initcall(joe_init_keypad);

