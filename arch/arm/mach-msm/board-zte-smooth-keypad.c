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
2010-06-21	 zhaotao			modify scancode map									  ZTE_KEYPAD_ZT_20100621
2010-04-20   zhaotao/hemulu  modify scancode map ,                                       ZTE_USB_HML_20100420  
2010-04-20   liwei         Modify row/column GPIOs for smooth to resolve boot crash issue, ZTE_BOOT_LIWEI_20100420
2010-03-23   jiangfeng     ???¨®BLADE core?¡ì3?                         
2009-12-11     wly         support ftm mode                                             ZTE_FTM_MODE_WLY_001
2009-10-30   qhhuang       Copy from board-raise-keypad and modify "raise" to "mooncake" to support mooncake keypad      
=========================================================================================*/

#include <linux/platform_device.h>
#include <linux/gpio_event.h>

#include <asm/mach-types.h>


/* chenjun, 2009-07-01, begin */
// chenjun: DDoutput_gpios
static unsigned int keypad_row_gpios[] = {35, 34, 33, 32, 31, 29, 28}; // Should be OUTPUT GPIOS,ZTE_BOOT_LIWEI_20100420

// chenjun: ¨¢Dinput_gpios
static unsigned int keypad_col_gpios[] = {42, 41, 40, 39, 38, 37, 36}; // Should be IN GPIOS,ZTE_BOOT_LIWEI_20100420
/* chenjun, 2009-07-01, end */


#define KEYMAP_INDEX(row, col) ((row)*ARRAY_SIZE(keypad_col_gpios) + (col))


static const unsigned short keypad_keymap_smooth[ARRAY_SIZE(keypad_col_gpios) *
					      ARRAY_SIZE(keypad_row_gpios)] = {
	/*   ZTE_USB_HML_20100420         row, col   */ 
	[KEYMAP_INDEX(0, 0)] = KEY_Q,//Q
	[KEYMAP_INDEX(0, 1)] = KEY_I,//I
	[KEYMAP_INDEX(0, 2)] = KEY_F,//F
	[KEYMAP_INDEX(0, 3)] = KEY_Z,//KEY_X,//X ZTE_KEYPAD_ZT_20100621
	[KEYMAP_INDEX(0, 4)] = KEY_M,//KEY_DOT,//. ZTE_KEYPAD_ZT_20100621
	[KEYMAP_INDEX(0, 5)] = KEY_RESERVED,
	[KEYMAP_INDEX(0, 6)] = KEY_UP,//Up
	
	[KEYMAP_INDEX(1, 0)] = KEY_W,//W
	[KEYMAP_INDEX(1, 1)] = KEY_O,//O
	[KEYMAP_INDEX(1, 2)] = KEY_G,//G
	[KEYMAP_INDEX(1, 3)] = KEY_X,//KEY_C,//C ZTE_KEYPAD_ZT_20100621
	[KEYMAP_INDEX(1, 4)] = KEY_CAPSLOCK,//KEY_LEFTSHIFT,//shift ZTE_KEYPAD_ZT_20100621
	[KEYMAP_INDEX(1, 5)] = KEY_RESERVED,
	[KEYMAP_INDEX(1, 6)] = KEY_DOWN,//Down

	[KEYMAP_INDEX(2, 0)] = KEY_E,//E
	[KEYMAP_INDEX(2, 1)] = KEY_P,//P
	[KEYMAP_INDEX(2, 2)] = KEY_H,//H
	[KEYMAP_INDEX(2, 3)] = KEY_C,//KEY_V,//V ZTE_KEYPAD_ZT_20100621
	[KEYMAP_INDEX(2, 4)] = KEY_TAB,//KEY_CAPSLOCK,//Caps Lock
	[KEYMAP_INDEX(2, 5)] = KEY_DELETE,//delete
	[KEYMAP_INDEX(2, 6)] = KEY_LEFT,//Left

	[KEYMAP_INDEX(3, 0)] = KEY_R,//R
	[KEYMAP_INDEX(3, 1)] = KEY_A,//A
	[KEYMAP_INDEX(3, 2)] = KEY_J,//J
	[KEYMAP_INDEX(3, 3)] = KEY_V,//KEY_B,//B ZTE_KEYPAD_ZT_20100621
	[KEYMAP_INDEX(3, 4)] = KEY_COMMA,//KEY_0,//0 ZTE_KEYPAD_ZT_20100621
	[KEYMAP_INDEX(3, 5)] = KEY_ENTER,//Enter
	[KEYMAP_INDEX(3, 6)] = KEY_RIGHT,//Right

	[KEYMAP_INDEX(4, 0)] = KEY_T,//T
	[KEYMAP_INDEX(4, 1)] = KEY_S,//S
	[KEYMAP_INDEX(4, 2)] = KEY_K,//K
	[KEYMAP_INDEX(4, 3)] = KEY_B,//KEY_N,//N ZTE_KEYPAD_ZT_20100621
	[KEYMAP_INDEX(4, 4)] = KEY_SPACE,//Space
	[KEYMAP_INDEX(4, 5)] = KEY_MENU,//Menu
	[KEYMAP_INDEX(4, 6)] = KEY_BACK,//Back

	[KEYMAP_INDEX(5, 0)] = KEY_Y,//Y
	[KEYMAP_INDEX(5, 1)] = KEY_D,//D
	[KEYMAP_INDEX(5, 2)] = KEY_L,//L
	[KEYMAP_INDEX(5, 3)] = KEY_N,//KEY_M,//M ZTE_KEYPAD_ZT_20100621
	[KEYMAP_INDEX(5, 4)] = KEY_DOT,//KEY_TAB,//Tab ZTE_KEYPAD_ZT_20100621
	[KEYMAP_INDEX(5, 5)] = KEY_HOME,//Home
	[KEYMAP_INDEX(5, 6)] = KEY_SEND,//Send

	[KEYMAP_INDEX(6, 0)] = KEY_U,//U
	[KEYMAP_INDEX(6, 1)] = KEY_VOLUMEDOWN,//Volume Down
	[KEYMAP_INDEX(6, 2)] = KEY_LEFTSHIFT,//KEY_Z,//Z ZTE_KEYPAD_ZT_20100621
	[KEYMAP_INDEX(6, 3)] = KEY_RESERVED,
	[KEYMAP_INDEX(6, 4)] = KEY_RESERVED,
	[KEYMAP_INDEX(6, 5)] = KEY_VOLUMEUP,//Volume Up
	[KEYMAP_INDEX(6, 6)] = KEY_END,//End
};

static const unsigned short smooth_keypad_virtual_keys[] = {
	KEY_END,
	KEY_POWER
};

/* smooth keypad platform device information */
static struct gpio_event_matrix_info smooth_keypad_matrix_info = {
	.info.func	= gpio_event_matrix_func,
	.keymap		= keypad_keymap_smooth,
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

static struct gpio_event_info *smooth_keypad_info[] = {
	&smooth_keypad_matrix_info.info
};

static struct gpio_event_platform_data smooth_keypad_data = {
	.name		= "smooth_keypad",
	.info		= smooth_keypad_info,
	.info_count	= ARRAY_SIZE(smooth_keypad_info)
};

struct platform_device keypad_device_smooth = {
	.name	= GPIO_EVENT_DEV_NAME,
	.id	= -1,
	.dev	= {
		.platform_data	= &smooth_keypad_data,
	},
};

#ifdef CONFIG_ZTE_FTM_FLAG_SUPPORT
extern int zte_get_ftm_flag(void);
#endif

static int __init smooth_init_keypad(void)
{
        
	#ifdef CONFIG_ZTE_FTM_FLAG_SUPPORT
	int ftm_flag;
	ftm_flag = zte_get_ftm_flag();
	if (1 ==ftm_flag)return 0;
	#endif
        
	smooth_keypad_matrix_info.keymap = keypad_keymap_smooth;
	return platform_device_register(&keypad_device_smooth);
}

device_initcall(smooth_init_keypad);

