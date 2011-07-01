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
2010-10-26	xiayuchun		support flip detect.                       ZTE_KEYPD_XIAYC_20101026
2010-09-18	liwei			Rotate the orientation keys to suit slide phone design, 		       ZTE_KEYPAD_LIWEI_20100918
2010-09-16	liwei			modify scancode map for amigo according actual keypad map		ZTE_KEYPAD_LIWEI_20100916
2010-09-11   liwei         Porting Code From Eclair Branch, ZTE_KEYPAD_LIWEI_20100911
=========================================================================================*/

#include <linux/platform_device.h>
#include <linux/gpio_event.h>

#include <asm/mach-types.h>


static unsigned int keypad_row_gpios[] = {35,34,33,32,31,29,28,26,23,81,82}; 
static unsigned int keypad_col_gpios[] = {42,41,40,39,38,37,36}; 

#define KEYMAP_INDEX(row, col) ((row)*ARRAY_SIZE(keypad_col_gpios) + (col))


static const unsigned short keypad_keymap_amigo[ARRAY_SIZE(keypad_col_gpios) *
					      ARRAY_SIZE(keypad_row_gpios)] = {
	/*          row, col   */
	[KEYMAP_INDEX(0, 0)] = KEY_RESERVED, 
	[KEYMAP_INDEX(0, 1)] = KEY_A, 
	[KEYMAP_INDEX(0, 2)] = KEY_RESERVED,
	[KEYMAP_INDEX(0, 3)] = KEY_Z, 
	[KEYMAP_INDEX(0, 4)] = KEY_E, 
	[KEYMAP_INDEX(0, 5)] = KEY_R, 
	[KEYMAP_INDEX(0, 6)] = KEY_T, 

	[KEYMAP_INDEX(1, 0)] = KEY_RESERVED, 
	[KEYMAP_INDEX(1, 1)] = KEY_Q, 
	[KEYMAP_INDEX(1, 2)] = KEY_RESERVED,
	[KEYMAP_INDEX(1, 3)] = KEY_S, 
	[KEYMAP_INDEX(1, 4)] = KEY_D, 
	[KEYMAP_INDEX(1, 5)] = KEY_F, 
	[KEYMAP_INDEX(1, 6)] = KEY_G, 	

	[KEYMAP_INDEX(2, 0)] = KEY_RESERVED, 
	[KEYMAP_INDEX(2, 1)] = KEY_W, 
	[KEYMAP_INDEX(2, 2)] = KEY_RESERVED,
	[KEYMAP_INDEX(2, 3)] = KEY_X, 
	[KEYMAP_INDEX(2, 4)] = KEY_C, 
	[KEYMAP_INDEX(2, 5)] = KEY_V, 
	[KEYMAP_INDEX(2, 6)] = KEY_B, 

	[KEYMAP_INDEX(3, 0)] = KEY_RESERVED, 
	[KEYMAP_INDEX(3, 1)] = KEY_Y, 
	[KEYMAP_INDEX(3, 2)] = KEY_RESERVED,
	[KEYMAP_INDEX(3, 3)] = KEY_U, 
	[KEYMAP_INDEX(3, 4)] = KEY_I, 
	[KEYMAP_INDEX(3, 5)] = KEY_O, 
	[KEYMAP_INDEX(3, 6)] = KEY_P, 

	[KEYMAP_INDEX(4, 0)] = KEY_RESERVED, 
	[KEYMAP_INDEX(4, 1)] = KEY_H, 
	[KEYMAP_INDEX(4, 2)] = KEY_RESERVED,
	[KEYMAP_INDEX(4, 3)] = KEY_J, 
	[KEYMAP_INDEX(4, 4)] = KEY_K, 
	[KEYMAP_INDEX(4, 5)] = KEY_L, 
	[KEYMAP_INDEX(4, 6)] = KEY_DOT, 

	[KEYMAP_INDEX(5, 0)] = KEY_RESERVED, 
	[KEYMAP_INDEX(5, 1)] = KEY_N, 
	[KEYMAP_INDEX(5, 2)] = KEY_RESERVED,
	[KEYMAP_INDEX(5, 3)] = KEY_M, 
	[KEYMAP_INDEX(5, 4)] = KEY_COMMA, 
	[KEYMAP_INDEX(5, 5)] = KEY_ENTER, 
	[KEYMAP_INDEX(5, 6)] = KEY_BACKSPACE, 

	[KEYMAP_INDEX(6, 0)] = KEY_RESERVED, 
	[KEYMAP_INDEX(6, 1)] = KEY_VOLUMEDOWN, 
	[KEYMAP_INDEX(6, 2)] = KEY_RESERVED,
	[KEYMAP_INDEX(6, 3)] = KEY_MENU, 
	[KEYMAP_INDEX(6, 4)] = KEY_BACK,  // BACK key on top ,ZTE_KEYPAD_LIWEI_20100916
	[KEYMAP_INDEX(6, 5)] = KEY_VOLUMEUP, 
	[KEYMAP_INDEX(6, 6)] = KEY_HOME, // HOME key  on top ,ZTE_KEYPAD_LIWEI_20100916

	[KEYMAP_INDEX(7, 0)] = KEY_RESERVED, 
	[KEYMAP_INDEX(7, 1)] = KEY_UP, // KEY_LEFT, ZTE_KEYPAD_LIWEI_20100918
	[KEYMAP_INDEX(7, 2)] = KEY_RESERVED,
	[KEYMAP_INDEX(7, 3)] = KEY_LEFT, //KEY_DOWN, ZTE_KEYPAD_LIWEI_20100918
	[KEYMAP_INDEX(7, 4)] = KEY_RIGHT,//KEY_UP, ZTE_KEYPAD_LIWEI_20100918
	[KEYMAP_INDEX(7, 5)] = KEY_DOWN, //KEY_RIGHT, ZTE_KEYPAD_LIWEI_20100918
	[KEYMAP_INDEX(7, 6)] = KEY_OK, 

	[KEYMAP_INDEX(8, 0)] = KEY_RESERVED, 
	[KEYMAP_INDEX(8, 1)] = KEY_SYSRQ,   // "?" and  0
	[KEYMAP_INDEX(8, 2)] = KEY_RESERVED,
	[KEYMAP_INDEX(8, 3)] = KEY_SPACE, 
	[KEYMAP_INDEX(8, 4)] = KEY_BACK, //RETURN
	[KEYMAP_INDEX(8, 5)] = KEY_RESERVED, 
	[KEYMAP_INDEX(8, 6)] = KEY_RESERVED, 

	[KEYMAP_INDEX(9, 0)] = KEY_FN, 
	[KEYMAP_INDEX(9, 1)] = KEY_RESERVED, 
	[KEYMAP_INDEX(9, 2)] = KEY_MENU,
	[KEYMAP_INDEX(9, 3)] = KEY_RESERVED, 
	[KEYMAP_INDEX(9, 4)] = KEY_RESERVED, 
	[KEYMAP_INDEX(9, 5)] = KEY_RESERVED, 
	[KEYMAP_INDEX(9, 6)] = KEY_RESERVED, 

	[KEYMAP_INDEX(10, 0)] = KEY_LEFTSHIFT, 
	[KEYMAP_INDEX(10, 1)] = KEY_RESERVED, 
	[KEYMAP_INDEX(10, 2)] = KEY_SEARCH,
	[KEYMAP_INDEX(10, 3)] = KEY_RESERVED, 
	[KEYMAP_INDEX(10, 4)] = KEY_RESERVED, 
	[KEYMAP_INDEX(10, 5)] = KEY_RESERVED, 
	[KEYMAP_INDEX(10, 6)] = KEY_RESERVED, 
};

static const unsigned short amigo_keypad_virtual_keys[] = {
	KEY_END,
	KEY_POWER
};

/* amigo keypad platform device information */
static struct gpio_event_matrix_info amigo_keypad_matrix_info = {
	.info.func		= gpio_event_matrix_func,
	.keymap			= keypad_keymap_amigo,
	.output_gpios	= keypad_row_gpios,
	.input_gpios	= keypad_col_gpios,
	.noutputs		= ARRAY_SIZE(keypad_row_gpios),
	.ninputs		= ARRAY_SIZE(keypad_col_gpios),
	.settle_time.tv.nsec 	= 0,
	.poll_time.tv.nsec		= 20 * NSEC_PER_MSEC,
#if 1 // chenjun
	.flags = GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_DRIVE_INACTIVE |
			GPIOKPF_PRINT_UNMAPPED_KEYS
#else
	.flags = GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_DRIVE_INACTIVE | GPIOKPF_ACTIVE_HIGH | GPIOKPF_PRINT_UNMAPPED_KEYS /*| GPIOKPF_PRINT_MAPPED_KEYS*/
#endif
};

static struct gpio_event_direct_entry amigo_keypad_switch_map[] = {
	{ 49,       SW_LID       }	//gpio49- flip detect pin
};

static struct gpio_event_input_info amigo_keypad_switch_info = {
	.info.func = gpio_event_input_func,
	.flags = 0,
	.type = EV_SW,
	.keymap = amigo_keypad_switch_map,
	.keymap_size = ARRAY_SIZE(amigo_keypad_switch_map)
};

static struct gpio_event_info *amigo_keypad_info[] = {
	&amigo_keypad_matrix_info.info,
	&amigo_keypad_switch_info.info,	
};

static struct gpio_event_platform_data amigo_keypad_data = {
	.name		= "amigo_keypad",
	.info		= amigo_keypad_info,
	.info_count	= ARRAY_SIZE(amigo_keypad_info)
};

struct platform_device keypad_device_amigo = {
	.name	= GPIO_EVENT_DEV_NAME,
	.id		= -1,
	.dev 	= {
		.platform_data	= &amigo_keypad_data,
	},
};

#ifdef CONFIG_ZTE_FTM_FLAG_SUPPORT
extern int zte_get_ftm_flag(void);
#endif

static int __init amigo_init_keypad(void)
{
#ifdef CONFIG_ZTE_FTM_FLAG_SUPPORT
	int ftm_flag;
	ftm_flag = zte_get_ftm_flag();
	if (1 ==ftm_flag)return 0;
#endif

	amigo_keypad_matrix_info.keymap = keypad_keymap_amigo;
	return platform_device_register(&keypad_device_amigo);
}

device_initcall(amigo_init_keypad);

