/*
 * drivers/media/video/msm/mt9v113_reg.c
 *
 * Refer to drivers/media/video/msm/mt9d112_reg.c
 * For MT9V113: 0.3Mp, 1/11-Inch System-On-A-Chip (SOC) CMOS Digital Image Sensor
 *
 * Copyright (C) 2009-2010 ZTE Corporation.
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
 * Created by jia.jia@zte.com.cn
 */
/*-----------------------------------------------------------------------------------------
  when         who          what, where, why                         comment tag
  --------     ----         -------------------------------------    ----------------------
  2011-07-28   wangtao      add settings                             ZTE_CAM_WT_20100728
  2011-04-15   lijing       add settings                             ZTE_CAM_LJ_20100415
  2009-10-24   jia.jia      Merged from kernel-v4515                 ZTE_MSM_CAMERA_JIA_001
------------------------------------------------------------------------------------------*/

#include "ovm7690.h"

static struct ovm7690_i2c_reg_conf const preview_snapshot_mode_reg_settings_array[] = {
//{0x12, 0x80, BYTE_LEN, 0},
{0x0c, 0x96, BYTE_LEN, 0},
{0x0c, 0xd6, BYTE_LEN, 0},
{0x0c, 0x46, BYTE_LEN, 0},
{0x0c, 0x06, BYTE_LEN, 0},
{0x48, 0x42, BYTE_LEN, 0},
{0x41, 0x43, BYTE_LEN, 0},
{0x4c, 0x73, BYTE_LEN, 0},
{0x81, 0xff, BYTE_LEN, 0}, 
{0x21, 0x44, BYTE_LEN, 0},
{0x16, 0x03, BYTE_LEN, 0},
{0x39, 0x80, BYTE_LEN, 0},
{0x1e, 0xb1, BYTE_LEN, 0},
{0x12, 0x00, BYTE_LEN, 0},
{0x82, 0x03, BYTE_LEN, 0},
{0xd0, 0x48, BYTE_LEN, 0},
{0x80, 0x7f, BYTE_LEN, 0},
{0x3e, 0x30, BYTE_LEN, 0},
{0x22, 0x00, BYTE_LEN, 0},
{0x17, 0x69, BYTE_LEN, 0},
{0x18, 0xa4, BYTE_LEN, 0},
{0x19, 0x0c, BYTE_LEN, 0},
{0x1a, 0xf6, BYTE_LEN, 0},
{0xc8, 0x02, BYTE_LEN, 0},
{0xc9, 0x80, BYTE_LEN, 0},
{0xca, 0x01, BYTE_LEN, 0},
{0xcb, 0xe0, BYTE_LEN, 0},
{0xcc, 0x02, BYTE_LEN, 0},
{0xcd, 0x80, BYTE_LEN, 0},
{0xce, 0x01, BYTE_LEN, 0},
{0xcf, 0xe0, BYTE_LEN, 0},
{0x85, 0x90, BYTE_LEN, 0},
{0x86, 0x10, BYTE_LEN, 0},
{0x87, 0x00, BYTE_LEN, 0},
{0x88, 0x10, BYTE_LEN, 0},
{0x89, 0x18, BYTE_LEN, 0},
{0x8a, 0x10, BYTE_LEN, 0},
{0x8b, 0x14, BYTE_LEN, 0},

{0xb7, 0x02, BYTE_LEN, 0},
{0xb8, 0x07, BYTE_LEN, 0}, //0x0b neil 20110825
{0xb9, 0x00, BYTE_LEN, 0},
{0xba, 0x18, BYTE_LEN, 0},
{0x5A, 0x4A, BYTE_LEN, 0},
{0x5B, 0x9F, BYTE_LEN, 0},
{0x5C, 0x48, BYTE_LEN, 0},
{0x5d, 0x32, BYTE_LEN, 0},
{0x8e, 0x92, BYTE_LEN, 0},
{0x96, 0xff, BYTE_LEN, 0},
{0x97, 0x00, BYTE_LEN, 0},
{0x8c, 0x5d, BYTE_LEN, 0},
{0x8d, 0x11, BYTE_LEN, 0},
{0x8e, 0x12, BYTE_LEN, 0},
{0x8f, 0x11, BYTE_LEN, 0},
{0x90, 0x50, BYTE_LEN, 0},
{0x91, 0x22, BYTE_LEN, 0},
{0x92, 0xd1, BYTE_LEN, 0},
{0x93, 0xa7, BYTE_LEN, 0},
{0x94, 0x23, BYTE_LEN, 0},
{0x24, 0x88, BYTE_LEN, 0},
{0x25, 0x78, BYTE_LEN, 0},
{0x26, 0xc4, BYTE_LEN, 0},
{0x95, 0x3b, BYTE_LEN, 0},
{0x96, 0xff, BYTE_LEN, 0},
{0x97, 0x00, BYTE_LEN, 0},
{0x98, 0x4a, BYTE_LEN, 0},
{0x99, 0x46, BYTE_LEN, 0},
{0x9a, 0x3d, BYTE_LEN, 0},
{0x9b, 0x3a, BYTE_LEN, 0},
{0x9c, 0xf0, BYTE_LEN, 0},
{0x9d, 0xf0, BYTE_LEN, 0},
{0x9e, 0xf0, BYTE_LEN, 0},
{0x9f, 0xff, BYTE_LEN, 0},
{0xa0, 0x56, BYTE_LEN, 0},
{0xa1, 0x55, BYTE_LEN, 0},
{0xa2, 0x13, BYTE_LEN, 0},
{0x50, 0x4d, BYTE_LEN, 0},   
{0x51, 0x3f, BYTE_LEN, 0},
{0x21, 0x57, BYTE_LEN, 0},
{0x20, 0x00, BYTE_LEN, 0},
{0x14, 0x29, BYTE_LEN, 0},
{0x13, 0xf7, BYTE_LEN, 0},
{0x11, 0x01, BYTE_LEN, 0}, 
{0x15, 0x98, BYTE_LEN, 0}, //neil add auto frame function
{0x68, 0xb0, BYTE_LEN, 0},
{0xd2, 0x07, BYTE_LEN, 0},


#if 0
{0xa3, 0x08, BYTE_LEN, 0}, //gamma
{0xa4, 0x15, BYTE_LEN, 0},
{0xa5, 0x24, BYTE_LEN, 0},
{0xa6, 0x45, BYTE_LEN, 0},	
{0xa7, 0x55, BYTE_LEN, 0},
{0xa8, 0x6a, BYTE_LEN, 0},
{0xa9, 0x78, BYTE_LEN, 0},
{0xaa, 0x87, BYTE_LEN, 0},
{0xab, 0x96, BYTE_LEN, 0},
{0xac, 0xa3, BYTE_LEN, 0},
{0xad, 0xb4, BYTE_LEN, 0},
{0xae, 0xc3, BYTE_LEN, 0},
{0xaf, 0xd6, BYTE_LEN, 0},
{0xb0, 0xe6, BYTE_LEN, 0},
{0xb1, 0xf2, BYTE_LEN, 0},
{0xb2, 0x12, BYTE_LEN, 0}, 


#else
{0xa3, 0x04, BYTE_LEN, 0}, //new gamma neil 20110825
{0xa4, 0x0c, BYTE_LEN, 0},
{0xa5, 0x23, BYTE_LEN, 0},
{0xa6, 0x55, BYTE_LEN, 0},	
{0xa7, 0x69, BYTE_LEN, 0},
{0xa8, 0x78, BYTE_LEN, 0},
{0xa9, 0x80, BYTE_LEN, 0},
{0xaa, 0x88, BYTE_LEN, 0},
{0xab, 0x90, BYTE_LEN, 0},
{0xac, 0x97, BYTE_LEN, 0},
{0xad, 0xa4, BYTE_LEN, 0},
{0xae, 0xb0, BYTE_LEN, 0},
{0xaf, 0xc5, BYTE_LEN, 0},
{0xb0, 0xd7, BYTE_LEN, 0},
{0xb1, 0xe8, BYTE_LEN, 0},
{0xb2, 0x20, BYTE_LEN, 0}, 

#endif

{0xbb, 0x80, BYTE_LEN, 0}, //cmx
{0xbc, 0x62, BYTE_LEN, 0},
{0xbd, 0x1e, BYTE_LEN, 0},
{0xbe, 0x26, BYTE_LEN, 0},
{0xbf, 0x7b, BYTE_LEN, 0},
{0xc0, 0xac, BYTE_LEN, 0},
{0xc1, 0x1e, BYTE_LEN, 0},      
};

static struct ovm7690_i2c_reg_conf const noise_reduction_reg_settings_array[] = {
    /* add code here
        e.g. {0xXXXX, 0xXXXX, WORD_LEN, 0},
     */
    {0x0000, 0x0000, WORD_LEN, 0},
};

static struct ovm7690_i2c_reg_conf const lens_roll_off_tbl[] = {
    /* add code here
        e.g. {0xXXXX, 0xXXXX, WORD_LEN, 0},
     */
    {0x0000, 0x0000, WORD_LEN, 0},
};

static struct ovm7690_i2c_reg_conf const pll_setup_tbl[] = {
    /* add code here
        e.g. {0xXXXX, 0xXXXX, WORD_LEN, 0},
     */
    {0x0000, 0x0000, WORD_LEN, 0},
};

/* Refresh Sequencer */
static struct ovm7690_i2c_reg_conf const sequencer_tbl[] = {
    /* add code here
        e.g. {0xXXXX, 0xXXXX, WORD_LEN, 0},
     */
    {0x0000, 0x0000, WORD_LEN, 0},
};

/*
 * ZTE_CAM_LJ_20100415
 * add antibanding settings
 */
 static struct ovm7690_i2c_reg_conf const antibanding_auto_tbl[] = {
    {0x00, 0x00, BYTE_LEN, 0},
    
};

static struct ovm7690_i2c_reg_conf const antibanding_50_tbl[] = {
    {0x00, 0x00, BYTE_LEN, 0},
    

};

static struct ovm7690_i2c_reg_conf const antibanding_60_tbl[] = {
   {0x00, 0x00, BYTE_LEN, 0},
    
};


struct ovm7690_reg_t ovm7690_regs = {
    .prev_snap_reg_settings             = &preview_snapshot_mode_reg_settings_array[0],
    .prev_snap_reg_settings_size        = ARRAY_SIZE(preview_snapshot_mode_reg_settings_array),

    .noise_reduction_reg_settings        = &noise_reduction_reg_settings_array[0],
    .noise_reduction_reg_settings_size   = 0,  /* ARRAY_SIZE(noise_reduction_reg_settings_array), */

    .plltbl                              = &pll_setup_tbl[0],
    .plltbl_size                         = 0,  /* ARRAY_SIZE(pll_setup_tbl), */

    .stbl                                = &sequencer_tbl[0],
    .stbl_size                           = 0,  /* ARRAY_SIZE(sequencer_tbl), */

    .rftbl                               = &lens_roll_off_tbl[0],
    .rftbl_size                          =  0,  /* ARRAY_SIZE(lens_roll_off_tbl), */

    .antibanding_auto_tbl                = antibanding_auto_tbl,
    .antibanding_auto_tbl_size           = ARRAY_SIZE(antibanding_auto_tbl),

    .antibanding_50_tbl                  = antibanding_50_tbl,
    .antibanding_50_tbl_size             = ARRAY_SIZE(antibanding_50_tbl),
    
    .antibanding_60_tbl                  = antibanding_60_tbl,
    .antibanding_60_tbl_size             = ARRAY_SIZE(antibanding_60_tbl),
};

