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
---------     -------       -------------------------------------------------------     --------------------------
2010-12-21		zhongfj      exchange KEY_VOLUMEUP and KEY_VOLUMEDOWN											ZTE_KB_ZFJ_20101221
2010-07-03     liwei       Copy from board-mooncake-keypad and modify "mooncake" to "v9" to support v9 keypad ,ZTE_BOOT_LIWEI_20100703
2009-12-11     wly         support ftm mode                                             ZTE_FTM_MODE_WLY_001
2009-10-30   qhhuang       Copy from board-raise-keypad and modify "raise" to "mooncake" to support mooncake keypad      
=========================================================================================*/

#include <linux/platform_device.h>
#include <linux/gpio_event.h>

#include <asm/mach-types.h>


// output_gpios
static unsigned int keypad_row_gpios[] = {31}; 
// input_gpios
static unsigned int keypad_col_gpios[] = {37, 41}; 


#define KEYMAP_INDEX(row, col) ((row)*ARRAY_SIZE(keypad_col_gpios) + (col))


static const unsigned short keypad_keymap_v9[ARRAY_SIZE(keypad_col_gpios) *
					      ARRAY_SIZE(keypad_row_gpios)] = {
	/*                       row, col   */
	
	[KEYMAP_INDEX(0, 0)] = KEY_VOLUMEUP, //Volume up  key [31, 37]
	[KEYMAP_INDEX(0, 1)] = KEY_VOLUMEDOWN,   //Volume down  key [31, 41]
	
	
};

static const unsigned short v9_keypad_virtual_keys[] = {
	KEY_END,
	KEY_POWER
};

/* v9 keypad platform device information */
static struct gpio_event_matrix_info v9_keypad_matrix_info = {
	.info.func	= gpio_event_matrix_func,
	.keymap		= keypad_keymap_v9,
	.output_gpios	= keypad_row_gpios,
	.input_gpios	= keypad_col_gpios,
	.noutputs	= ARRAY_SIZE(keypad_row_gpios),
	.ninputs	= ARRAY_SIZE(keypad_col_gpios),
	.settle_time.tv.nsec = 0,
	.poll_time.tv.nsec = 20 * NSEC_PER_MSEC,
#if 1 // chenjun
	.flags		= GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_DRIVE_INACTIVE |
			  GPIOKPF_PRINT_UNMAPPED_KEYS
#else
	.flags		= GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_DRIVE_INACTIVE | GPIOKPF_ACTIVE_HIGH | GPIOKPF_PRINT_UNMAPPED_KEYS /*| GPIOKPF_PRINT_MAPPED_KEYS*/
#endif
};

static struct gpio_event_info *v9_keypad_info[] = {
	&v9_keypad_matrix_info.info
};

static struct gpio_event_platform_data v9_keypad_data = {
	.name		= "v9_keypad",
	.info		= v9_keypad_info,
	.info_count	= ARRAY_SIZE(v9_keypad_info)
};

struct platform_device keypad_device_v9 = {
	.name	= GPIO_EVENT_DEV_NAME,
	.id	= -1,
	.dev	= {
		.platform_data	= &v9_keypad_data,
	},
};

#ifdef CONFIG_ZTE_FTM_FLAG_SUPPORT
extern int zte_get_ftm_flag(void);
#endif

static int __init v9_init_keypad(void)
{
        
	#ifdef CONFIG_ZTE_FTM_FLAG_SUPPORT
	int ftm_flag;
	ftm_flag = zte_get_ftm_flag();
	if (1 ==ftm_flag)return 0;
	#endif
        
	v9_keypad_matrix_info.keymap = keypad_keymap_v9;
	return platform_device_register(&keypad_device_v9);
}

device_initcall(v9_init_keypad);

