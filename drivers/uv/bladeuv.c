/*
 * Author: doixanh <doixanh at xda-developers>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 */

#include <linux/module.h>
#include <linux/kernel.h>	




// defines
#define DX_MODULE_NAME 			"bladeuv"
#define DX_PROCFS_NAME 			"bladeuv"
#define DXDBG(x)					

// patch offsets

#define DEVICE_NAME				"Blade"
#define OFS_KALLSYMS_LOOKUP_NAME	0xc00cfd6c			// kallsyms_lookup_name

// struct definition
struct clkctl_acpu_speed_dx {
	unsigned int	use_for_scaling;
	unsigned int	a11clk_khz;
	int		pll;
	unsigned int	a11clk_src_sel;
	unsigned int	a11clk_src_div;
	unsigned int	ahbclk_khz;
	unsigned int	ahbclk_div;
	int		vdd;
	unsigned int 	axiclk_khz;
	unsigned long	lpj; /* loops_per_jiffy */
/* Pointers in acpu_freq_tbl[] for max up/down steppings. */
	struct clkctl_acpu_speed_dx *down[3];
	struct clkctl_acpu_speed_dx *up[3];
};


// variables
static struct clkctl_acpu_speed_dx *standard_clocks;

// external variables / functions
typedef unsigned long (*kallsyms_lookup_name_type)(const char *name);
static kallsyms_lookup_name_type kallsyms_lookup_name_dx;

// init module
static int __init bladeuv_init(void)
{
	printk(KERN_INFO DX_MODULE_NAME ": Module loaded. Built for target device: " DEVICE_NAME "\n");
	
	// our 'GetProcAddress' :D
	kallsyms_lookup_name_dx = (void*) OFS_KALLSYMS_LOOKUP_NAME;

	// look for other offsets
	standard_clocks = (void*) kallsyms_lookup_name_dx("pll0_960_pll1_245_pll2_1200");
	
	// do some undervoltage
	standard_clocks[2].vdd = 0;		// a little bit under voltage for 122880
	standard_clocks[4].vdd = 2;		// a little bit under voltage for 245760
	standard_clocks[5].vdd = 3;		// a little bit under voltage for 320000
	standard_clocks[7].vdd = 5;		// a little bit under voltage for 480000
	standard_clocks[8].vdd = 7;		// same voltage for max

	printk(KERN_INFO DX_MODULE_NAME ": patching done. enjoy better battery life.\n");
	return 0;
}


// exit module - will most likely not be called
static void __exit bladeuv_exit(void)
{
	// We must revert voltages to their default values
	standard_clocks[2].vdd = 3;
	standard_clocks[4].vdd = 4;
	standard_clocks[5].vdd = 5;
	standard_clocks[7].vdd = 6;
	standard_clocks[8].vdd = 7;

	printk(KERN_INFO DX_MODULE_NAME ": module unloaded\n");
}

module_init(bladeuv_init);
module_exit(bladeuv_exit);

MODULE_DESCRIPTION("Undervoltage module for Blade");
MODULE_LICENSE("GPL");
