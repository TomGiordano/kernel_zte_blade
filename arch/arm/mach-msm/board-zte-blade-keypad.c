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
2010-05-20   weilanying   修改blade_keypad名称          ZTE_KEYPAD_WLY_0520
2010-04-02   zhangtao     modified the keypad map			  ZTE_KEYPAD_ZT_20100402_001
2010-03-23   jiangfeng     增加BLADE core支持                         
2009-12-11     wly         support ftm mode                          ZTE_FTM_MODE_WLY_001
2009-10-30   qhhuang       Copy from board-raise-keypad and modify "raise" to "mooncake" to support mooncake keypad      
=========================================================================================*/

#include <linux/platform_device.h>
#include <linux/gpio_event.h>

#include <asm/mach-types.h>


static unsigned int keypad_row_gpios[] = {33, 28}; 

static unsigned int keypad_col_gpios[] = {37, 41, 40}; 

#define KEYMAP_INDEX(row, col) ((row)*ARRAY_SIZE(keypad_col_gpios) + (col))


static const unsigned short keypad_keymap_blade[ARRAY_SIZE(keypad_col_gpios) *
					      ARRAY_SIZE(keypad_row_gpios)] = {
	/*                       row, col   */
	[KEYMAP_INDEX(0, 0)] = KEY_BACK, 
	[KEYMAP_INDEX(0, 1)] = 0, 
	[KEYMAP_INDEX(0, 2)] = KEY_MENU,
	[KEYMAP_INDEX(1, 0)] = KEY_VOLUMEUP,
	[KEYMAP_INDEX(1, 1)] = KEY_VOLUMEDOWN,
	[KEYMAP_INDEX(1, 2)] = KEY_HOME,
};


static const unsigned short blade_keypad_virtual_keys[] = {
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

//<<------LHX_PM_20110506_01 get which GPIO for BACK HOME MENU
#endif
/* blade keypad platform device information */
static struct gpio_event_matrix_info blade_keypad_matrix_info = {
	.info.func	= gpio_event_matrix_func,
	.keymap		= keypad_keymap_blade,
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

static struct gpio_event_info *blade_keypad_info[] = {
	&blade_keypad_matrix_info.info
};

static struct gpio_event_platform_data blade_keypad_data = {
	.name		= "blade_keypad",
	.info		= blade_keypad_info,
	.info_count	= ARRAY_SIZE(blade_keypad_info)
};

struct platform_device keypad_device_blade = {
	.name	= GPIO_EVENT_DEV_NAME,
	.id	= -1,
	.dev	= {
		.platform_data	= &blade_keypad_data,
	},
};

/* ZTE_FTM_MODE_WLY_001, @2009-12-11, START*/
#ifdef CONFIG_ZTE_FTM_FLAG_SUPPORT
extern int zte_get_ftm_flag(void);
#endif
/* ZTE_FTM_MODE_WLY_001, @2009-12-11, END*/
static int __init blade_init_keypad(void)
{
        /* ZTE_FTM_MODE_WLY_001, @2009-12-11, START*/
	#ifdef CONFIG_ZTE_FTM_FLAG_SUPPORT
	int ftm_flag;
	ftm_flag = zte_get_ftm_flag();
	if (1 ==ftm_flag)return 0;
	#endif
        /* ZTE_FTM_MODE_WLY_001, @2009-12-11, START*/
	blade_keypad_matrix_info.keymap = keypad_keymap_blade;
#ifdef CONFIG_MSM_GPIO_WAKE
	p_keypad_keymap = blade_keypad_matrix_info.keymap;	//LHX_PM_20110506_01 get which GPIO for BACK HOME MENU
#endif
	return platform_device_register(&keypad_device_blade);
}

device_initcall(blade_init_keypad);

