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

#include <linux/platform_device.h>
#include <linux/gpio_event.h>

#include <asm/mach-types.h>


static unsigned int keypad_row_gpios[] = {35,34,33,32,31,29,28,26,23,49,0}; 
static unsigned int keypad_col_gpios[] = {42,41,40,39,38,37,36}; 

#define KEYMAP_INDEX(row, col) ((row)*ARRAY_SIZE(keypad_col_gpios) + (col))


static const unsigned short keypad_keymap_turies[ARRAY_SIZE(keypad_col_gpios) *
					      ARRAY_SIZE(keypad_row_gpios)] = {
	/*                       row, col   */
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

	[KEYMAP_INDEX(4, 0)] = KEY_MUTE, 
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
	[KEYMAP_INDEX(5, 6)] = KEY_SEND,  

	[KEYMAP_INDEX(6, 0)] = KEY_RESERVED, 
	[KEYMAP_INDEX(6, 1)] = KEY_VOLUMEDOWN, 
	[KEYMAP_INDEX(6, 2)] = KEY_RESERVED,
	[KEYMAP_INDEX(6, 3)] = KEY_END,   
	[KEYMAP_INDEX(6, 4)] = KEY_HOME, 
	[KEYMAP_INDEX(6, 5)] = KEY_VOLUMEUP, 
	[KEYMAP_INDEX(6, 6)] = KEY_BACK,  

	[KEYMAP_INDEX(7, 0)] = KEY_RESERVED, 
	[KEYMAP_INDEX(7, 1)] = KEY_OK,    
	[KEYMAP_INDEX(7, 2)] = KEY_RESERVED,
	[KEYMAP_INDEX(7, 3)] = KEY_RIGHT,   
	[KEYMAP_INDEX(7, 4)] = KEY_DOWN, 
	[KEYMAP_INDEX(7, 5)] = KEY_UP,  
	[KEYMAP_INDEX(7, 6)] = KEY_LEFT, 

	[KEYMAP_INDEX(8, 0)] = KEY_RESERVED, 
	[KEYMAP_INDEX(8, 1)] = KEY_BACKSPACE, 
	[KEYMAP_INDEX(8, 2)] = KEY_RESERVED, 
	[KEYMAP_INDEX(8, 3)] = KEY_SPACE, 
	[KEYMAP_INDEX(8, 4)] = KEY_0,  
	[KEYMAP_INDEX(8, 5)] = KEY_COMPOSE,  
	[KEYMAP_INDEX(8, 6)] = KEY_CAMERA, 

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

static const unsigned short turies_keypad_virtual_keys[] = {
	KEY_END,
	KEY_POWER
};

#ifdef CONFIG_MSM_GPIO_WAKE
const unsigned short *p_keypad_keymap;
void zte_get_gpio_for_key(int *keycode)
{
	int gpio_wakeup_col;  
	int col_array_size = ARRAY_SIZE(keypad_col_gpios);
	int r=0;
	int keymap_index= 0;
	pr_info("[IRQWAKE] wakeup APP keycode %d\n",*keycode);
	while((*keycode != p_keypad_keymap[keymap_index]) && keymap_index <= (ARRAY_SIZE(keypad_col_gpios) * ARRAY_SIZE(keypad_row_gpios)))
	{
		keymap_index ++;
	}
	
	//pr_info("[IRQWAKE]  keymap_index %d\n",keymap_index);
	
	do{
		gpio_wakeup_col =  keymap_index - r * col_array_size;
		r++;
	}while(gpio_wakeup_col > col_array_size);
	
	//pr_info("[IRQWAKE]  gpio_col %d\n",gpio_wakeup_col);
	*keycode = keypad_col_gpios[gpio_wakeup_col];
	pr_info("[IRQWAKE]  wakeup gpio_num %d\n",*keycode);
	
}

#endif
/* turies keypad platform device information */
static struct gpio_event_matrix_info turies_keypad_matrix_info = {
	.info.func	= gpio_event_matrix_func,
	.keymap		= keypad_keymap_turies,
	.output_gpios	= keypad_row_gpios,
	.input_gpios	= keypad_col_gpios,
	.noutputs	= ARRAY_SIZE(keypad_row_gpios),
	.ninputs	= ARRAY_SIZE(keypad_col_gpios),
	.settle_time.tv.nsec = 0,
	.poll_time.tv.nsec = 20 * NSEC_PER_MSEC,
#if 1 
	.flags		= GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_DRIVE_INACTIVE |
			  GPIOKPF_PRINT_UNMAPPED_KEYS
#else
	.flags		= GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_DRIVE_INACTIVE | GPIOKPF_ACTIVE_HIGH | GPIOKPF_PRINT_UNMAPPED_KEYS /*| GPIOKPF_PRINT_MAPPED_KEYS*/
#endif
};

static struct gpio_event_info *turies_keypad_info[] = {
	&turies_keypad_matrix_info.info
};

static struct gpio_event_platform_data turies_keypad_data = {
	.name		= "turies_keypad",
	.info		= turies_keypad_info,
	.info_count	= ARRAY_SIZE(turies_keypad_info)
};

struct platform_device keypad_device_turies = {
	.name	= GPIO_EVENT_DEV_NAME,
	.id	= -1,
	.dev	= {
		.platform_data	= &turies_keypad_data,
	},
};

#ifdef CONFIG_ZTE_FTM_FLAG_SUPPORT
extern int zte_get_ftm_flag(void);
#endif

static int __init turies_init_keypad(void)
{
	#ifdef CONFIG_ZTE_FTM_FLAG_SUPPORT
	int ftm_flag;
	ftm_flag = zte_get_ftm_flag();
	if (1 ==ftm_flag)return 0;
	#endif

	turies_keypad_matrix_info.keymap = keypad_keymap_turies;
#ifdef CONFIG_MSM_GPIO_WAKE
	p_keypad_keymap = turies_keypad_matrix_info.keymap;	
#endif
	return platform_device_register(&keypad_device_turies);
}

device_initcall(turies_init_keypad);

