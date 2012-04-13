/*
 *---------------------------------------------------------------------------*
 *                                                                           *
 *               COPYRIGHT. SAMSUNG ELECTRONICS CO., LTD.                    *
 *                          ALL RIGHTS RESERVED                              *
 *                                                                           *
 *   Permission is hereby granted to licensees of Samsung Electronics Co.,   *
 *   Ltd. products to use this computer program only in accordance with the  *
 *   terms of the SAMSUNG FLASH MEMORY DRIVER SOFTWARE LICENSE AGREEMENT.    *
 *                                                                           *
 *---------------------------------------------------------------------------*
*/
/**
 *  @version    LinuStoreIII_1.2.0_b035-FSR_1.2.1p1_b129_RC
 *  @file       drivers/fsr/debug.c
 *  @brief      This file is debug file module
 *
 */

#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include "debug.h"

extern struct proc_dir_entry *fsr_proc_dir;







static int __init fsr_debug_init(void)
{


	return 0;

}

static void __exit fsr_debug_exit(void)
{

}

module_init(fsr_debug_init);
module_exit(fsr_debug_exit);

