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
------------------------------------------------------------------------------------------*/

#include "mt9v113.h"

static struct mt9v113_i2c_reg_conf const preview_snapshot_mode_reg_settings_array[] = {
    {0x98C, 0x2703, WORD_LEN, 0},        //Output Width (A)        
    {0x990, 640,    WORD_LEN, 0},        //
    {0x98C, 0x2705, WORD_LEN, 0},        //Output Height (A)       
    {0x990, 480,    WORD_LEN, 0},        //
    {0x98C, 0x2707, WORD_LEN, 0},        //Output Width (B)        
    {0x990, 640,    WORD_LEN, 0},        //
    {0x98C, 0x2709, WORD_LEN, 0},        //Output Height (B)       
    {0x990, 480,    WORD_LEN, 0},        //
    {0x98C, 0x270D, WORD_LEN, 0},        //Row Start (A)           
    {0x990, 0x004 , WORD_LEN, 0},        //      = 4                
    {0x98C, 0x270F, WORD_LEN, 0},        //Column Start (A)        
    {0x990, 0x004 , WORD_LEN, 0},        //      = 4                
    {0x98C, 0x2711, WORD_LEN, 0},        //Row End (A)             
    {0x990, 0x1EB , WORD_LEN, 0},        //      = 491              
    {0x98C, 0x2713, WORD_LEN, 0},        //Column End (A)          
    {0x990, 0x28B , WORD_LEN, 0},        //      = 651              
    {0x98C, 0x2715, WORD_LEN, 0},        //Row Speed (A)           
    {0x990, 0x0001, WORD_LEN, 0},        //      = 1               
    {0x98C, 0x2717, WORD_LEN, 0},        //Read Mode (A)           
    {0x990, 0x0024, WORD_LEN, 0},        //      zhanglinhui modify for Flip                 
    {0x98C, 0x2719, WORD_LEN, 0},        //sensor_fine_correction (
    {0x990, 0x001A, WORD_LEN, 0},        //      = 26              
    {0x98C, 0x271B, WORD_LEN, 0},        //sensor_fine_IT_min (A)  
    {0x990, 0x006B, WORD_LEN, 0},        //      = 107             
    {0x98C, 0x271D, WORD_LEN, 0},        //sensor_fine_IT_max_margi
    {0x990, 0x006B, WORD_LEN, 0},        //      = 107             
    {0x98C, 0x271F, WORD_LEN, 0},        //Frame Lines (A)         
    {0x990, 0x025C, WORD_LEN, 0},        //      = 604             
    {0x98C, 0x2721, WORD_LEN, 0},        //Line Length (A)         
    {0x990, 0x0694, WORD_LEN, 0},        //      = 842             
    {0x98C, 0x2723, WORD_LEN, 0},        //Row Start (B)           
    {0x990, 0x004 , WORD_LEN, 0},        //      = 4                
    {0x98C, 0x2725, WORD_LEN, 0},        //Column Start (B)        
    {0x990, 0x004 , WORD_LEN, 0},        //      = 4                
    {0x98C, 0x2727, WORD_LEN, 0},        //Row End (B)             
    {0x990, 0x1EB , WORD_LEN, 0},        //      = 491              
    {0x98C, 0x2729, WORD_LEN, 0},        //Column End (B)          
    {0x990, 0x28B , WORD_LEN, 0},        //      = 651              
    {0x98C, 0x272B, WORD_LEN, 0},        //Row Speed (B)           
    {0x990, 0x0001, WORD_LEN, 0},        //      = 1               
    {0x98C, 0x272D, WORD_LEN, 0},        //Read Mode (B)           
    {0x990, 0x0024, WORD_LEN, 0},        //      zhanglinhui modify for Flip         
    {0x98C, 0x272F, WORD_LEN, 0},        //sensor_fine_correction (
    {0x990, 0x001A, WORD_LEN, 0},        //      = 26              
    {0x98C, 0x2731, WORD_LEN, 0},        //sensor_fine_IT_min (B)  
    {0x990, 0x006B, WORD_LEN, 0},        //      = 107             
    {0x98C, 0x2733, WORD_LEN, 0},        //sensor_fine_IT_max_margi
    {0x990, 0x006B, WORD_LEN, 0},        //      = 107             
    {0x98C, 0x2735, WORD_LEN, 0},        //Frame Lines (B)         
    {0x990, 0x025C, WORD_LEN, 0},        //      = 604             
    {0x98C, 0x2737, WORD_LEN, 0},        //Line Length (B)         
    {0x990, 0x0694, WORD_LEN, 0},        //      = 842             
    {0x98C, 0x2739, WORD_LEN, 0},        //Crop_X0 (A)             
    {0x990, 0x0000, WORD_LEN, 0},        //      = 0               
    {0x98C, 0x273B, WORD_LEN, 0},        //Crop_X1 (A)             
    {0x990, 0x027F, WORD_LEN, 0},        //      = 639             
    {0x98C, 0x273D, WORD_LEN, 0},        //Crop_Y0 (A)             
    {0x990, 0x0000, WORD_LEN, 0},        //      = 0               
    {0x98C, 0x273F, WORD_LEN, 0},        //Crop_Y1 (A)             
    {0x990, 0x01DF, WORD_LEN, 0},        //      = 479             
    {0x98C, 0x2747, WORD_LEN, 0},        //Crop_X0 (B)             
    {0x990, 0x0000, WORD_LEN, 0},        //      = 0               
    {0x98C, 0x2749, WORD_LEN, 0},        //Crop_X1 (B)             
    {0x990, 0x027F, WORD_LEN, 0},        //      = 639             
    {0x98C, 0x274B, WORD_LEN, 0},        //Crop_Y0 (B)             
    {0x990, 0x0000, WORD_LEN, 0},        //      = 0               
    {0x98C, 0x274D, WORD_LEN, 0},        //Crop_Y1 (B)             
    {0x990, 0x01DF, WORD_LEN, 0},        //      = 479             
    {0x98C, 0x222D, WORD_LEN, 0},        //R9 Step                 
    {0x990, 0x008B, WORD_LEN, 0},        //      = 139             
    {0x98C, 0xA408, WORD_LEN, 0},        //search_f1_50            
    {0x990, 0x1F  , WORD_LEN, 0},        //      = 31                
    {0x98C, 0xA409, WORD_LEN, 0},        //search_f2_50            
    {0x990, 0x22  , WORD_LEN, 0},        //      = 34                
    {0x98C, 0xA40A, WORD_LEN, 0},        //search_f1_60            
    {0x990, 0x19  , WORD_LEN, 0},        //      = 25                
    {0x98C, 0xA40B, WORD_LEN, 0},        //search_f2_60            
    {0x990, 0x1C  , WORD_LEN, 0},        //      = 28                
    {0x98C, 0x2411, WORD_LEN, 0},        //R9_Step_60 (A)          
    {0x990, 0x008B, WORD_LEN, 0},        //      = 139             
    {0x98C, 0x2413, WORD_LEN, 0},        //R9_Step_50 (A)          
    {0x990, 0x00A6, WORD_LEN, 0},        //      = 166             
    {0x98C, 0x2415, WORD_LEN, 0},        //R9_Step_60 (B)          
    {0x990, 0x008B, WORD_LEN, 0},        //      = 139             
    {0x98C, 0x2417, WORD_LEN, 0},        //R9_Step_50 (B)          
    {0x990, 0x00A6, WORD_LEN, 0},        //      = 166             
    {0x98C, 0xA40D, WORD_LEN, 0},        //Stat_min                
    {0x990, 0x02  , WORD_LEN, 0},        //      = 2                 
    {0x98C, 0xA410, WORD_LEN, 0},        //Min_amplitude           
    {0x990, 0x01  , WORD_LEN, 0},        //      = 1                 
                                                      
    /*optimize FPS and output fromat*/                

    {0x098C, 0xA20C, WORD_LEN, 0},
    {0x0990, 0x0008, WORD_LEN, 0},
    {0x098C, 0xA215, WORD_LEN, 0},
    {0x0990, 0x0008, WORD_LEN, 0},
                  
    {0x098C, 0xA24F, WORD_LEN, 0},
    {0x0990, 0x004B, WORD_LEN, 0},
                 
    {0x3658, 0x0030, WORD_LEN, 0},
    {0x365A, 0xB82B, WORD_LEN, 0},
    {0x365C, 0x42F2, WORD_LEN, 0},
    {0x365E, 0xD1F0, WORD_LEN, 0},
    {0x3660, 0xBB33, WORD_LEN, 0},
    {0x3680, 0x084C, WORD_LEN, 0},
    {0x3682, 0xA0AC, WORD_LEN, 0},
    {0x3684, 0xFD6F, WORD_LEN, 0},
    {0x3686, 0xF031, WORD_LEN, 0},
    {0x3688, 0xB92E, WORD_LEN, 0},
    {0x36A8, 0x0893, WORD_LEN, 0},
    {0x36AA, 0xF792, WORD_LEN, 0},
    {0x36AC, 0xB255, WORD_LEN, 0},
    {0x36AE, 0x0D17, WORD_LEN, 0},
    {0x36B0, 0x65B8, WORD_LEN, 0},
    {0x36D0, 0x88F0, WORD_LEN, 0},
    {0x36D2, 0x5D6E, WORD_LEN, 0},
    {0x36D4, 0xFC53, WORD_LEN, 0},
    {0x36D6, 0x91D7, WORD_LEN, 0},
    {0x36D8, 0x9719, WORD_LEN, 0},
    {0x36F8, 0xF7B3, WORD_LEN, 0},
    {0x36FA, 0x6FB4, WORD_LEN, 0},
    {0x36FC, 0xA1D7, WORD_LEN, 0},
    {0x36FE, 0x163A, WORD_LEN, 0},
    {0x3700, 0x4FBD, WORD_LEN, 0},
    {0x364E, 0x0030, WORD_LEN, 0},
    {0x3650, 0xF32C, WORD_LEN, 0},
    {0x3652, 0x41F2, WORD_LEN, 0},
    {0x3654, 0xDCF0, WORD_LEN, 0},
    {0x3656, 0x8194, WORD_LEN, 0},
    {0x3676, 0xDDEA, WORD_LEN, 0},
    {0x3678, 0x050E, WORD_LEN, 0},
    {0x367A, 0x14B0, WORD_LEN, 0},
    {0x367C, 0x9073, WORD_LEN, 0},
    {0x367E, 0x8193, WORD_LEN, 0},
    {0x369E, 0x04B3, WORD_LEN, 0},
    {0x36A0, 0x97D3, WORD_LEN, 0},
    {0x36A2, 0xCFD5, WORD_LEN, 0},
    {0x36A4, 0x2157, WORD_LEN, 0},
    {0x36A6, 0x5118, WORD_LEN, 0},
    {0x36C6, 0xC14E, WORD_LEN, 0},
    {0x36C8, 0x9C92, WORD_LEN, 0},
    {0x36CA, 0xB6B1, WORD_LEN, 0},
    {0x36CC, 0x86D7, WORD_LEN, 0},
    {0x36CE, 0xBCD9, WORD_LEN, 0},
    {0x36EE, 0xA454, WORD_LEN, 0},
    {0x36F0, 0x7CB5, WORD_LEN, 0},
    {0x36F2, 0xE1D7, WORD_LEN, 0},
    {0x36F4, 0x2B19, WORD_LEN, 0},
    {0x36F6, 0x5AFD, WORD_LEN, 0},
    {0x3662, 0x0030, WORD_LEN, 0},
    {0x3664, 0x89ED, WORD_LEN, 0},
    {0x3666, 0x3AB2, WORD_LEN, 0},
    {0x3668, 0xFFCF, WORD_LEN, 0},
    {0x366A, 0xEB93, WORD_LEN, 0},
    {0x368A, 0x654A, WORD_LEN, 0},
    {0x368C, 0xE6CE, WORD_LEN, 0},
    {0x368E, 0xB6F0, WORD_LEN, 0},
    {0x3690, 0x0531, WORD_LEN, 0},
    {0x3692, 0x6792, WORD_LEN, 0},
    {0x36B2, 0x6CD2, WORD_LEN, 0},
    {0x36B4, 0xA7F3, WORD_LEN, 0},
    {0x36B6, 0x8536, WORD_LEN, 0},
    {0x36B8, 0x36D7, WORD_LEN, 0},
    {0x36BA, 0x7978, WORD_LEN, 0},
    {0x36DA, 0x928C, WORD_LEN, 0},
    {0x36DC, 0x4392, WORD_LEN, 0},
    {0x36DE, 0x8FD3, WORD_LEN, 0},
    {0x36E0, 0x82B8, WORD_LEN, 0},
    {0x36E2, 0xC1F9, WORD_LEN, 0},
    {0x3702, 0xBCF3, WORD_LEN, 0},
    {0x3704, 0x4036, WORD_LEN, 0},
    {0x3706, 0x8DB6, WORD_LEN, 0},
    {0x3708, 0x42D7, WORD_LEN, 0},
    {0x370A, 0x3B9D, WORD_LEN, 0},
    {0x366C, 0x0150, WORD_LEN, 0},
    {0x366E, 0xC90C, WORD_LEN, 0},
    {0x3670, 0x4332, WORD_LEN, 0},
    {0x3672, 0xBD10, WORD_LEN, 0},
    {0x3674, 0xF2B3, WORD_LEN, 0},
    {0x3694, 0x8F2C, WORD_LEN, 0},
    {0x3696, 0x118E, WORD_LEN, 0},
    {0x3698, 0x494F, WORD_LEN, 0},
    {0x369A, 0x9B93, WORD_LEN, 0},
    {0x369C, 0xA753, WORD_LEN, 0},
    {0x36BC, 0x02B3, WORD_LEN, 0},
    {0x36BE, 0x9B13, WORD_LEN, 0},
    {0x36C0, 0xBDD5, WORD_LEN, 0},
    {0x36C2, 0x1CD7, WORD_LEN, 0},
    {0x36C4, 0x2538, WORD_LEN, 0},
    {0x36E4, 0x98CE, WORD_LEN, 0},
    {0x36E6, 0xDDD2, WORD_LEN, 0},
    {0x36E8, 0xD971, WORD_LEN, 0},
    {0x36EA, 0xEF76, WORD_LEN, 0},
    {0x36EC, 0xA279, WORD_LEN, 0},
    {0x370C, 0x8A94, WORD_LEN, 0},
    {0x370E, 0x06D6, WORD_LEN, 0},
    {0x3710, 0xF457, WORD_LEN, 0},
    {0x3712, 0x5DB9, WORD_LEN, 0},
    {0x3714, 0x6DFD, WORD_LEN, 0},
    {0x3644, 0x015C, WORD_LEN, 0},
    {0x3642, 0x00F4, WORD_LEN, 0},
    {0x3210, 0x09B8, WORD_LEN, 0},
    {0x3644, 0x0160, WORD_LEN, 0},
    {0x3642, 0x00F4, WORD_LEN, 0},
    {0x3644, 0x0160, WORD_LEN, 0},
    {0x3642, 0x00F4, WORD_LEN, 0},
                 
    {0x098C, 0x2B1B, WORD_LEN, 0},
    {0x0990, 0x1D60, WORD_LEN, 0},
    {0x098C, 0x2B1D, WORD_LEN, 0},
    {0x0990, 0x1D60, WORD_LEN, 0},
    {0x098C, 0xAB20, WORD_LEN, 0},
    {0x0990, 0x0070, WORD_LEN, 0},
    {0x098C, 0xAB22, WORD_LEN, 0},
    {0x0990, 0x0004, WORD_LEN, 0},
    {0x098C, 0xAB23, WORD_LEN, 0},
    {0x0990, 0x0004, WORD_LEN, 0},
    {0x098C, 0xAB24, WORD_LEN, 0},
    {0x0990, 0x0030, WORD_LEN, 0},
    {0x098C, 0xAB27, WORD_LEN, 0},
    {0x0990, 0x0010, WORD_LEN, 0},
    {0x098C, 0x2B28, WORD_LEN, 0},
    {0x0990, 0x2000, WORD_LEN, 0},
    {0x098C, 0x2B2A, WORD_LEN, 0},
    {0x0990, 0x7000, WORD_LEN, 0},
//[mirror]
    {0x098C, 0x2717, WORD_LEN, 0},  // MCU_ADDRESS [MODE_SENSOR_READ_MODE_A]
    {0x0990, 0x0027, WORD_LEN, 0},  // MCU_DATA_0
    {0x098C, 0x272D, WORD_LEN, 0},  // MCU_ADDRESS [MODE_SENSOR_READ_MODE_B]
    {0x0990, 0x0027, WORD_LEN, 0},  // MCU_DATA_0
    {0x098C, 0xA103, WORD_LEN, 0},  // MCU_ADDRESS [SEQ_CMD]
    {0x0990, 0x0006, WORD_LEN, 10},  // MCU_DATA_0
};

static struct mt9v113_i2c_reg_conf const noise_reduction_reg_settings_array[] = {
    /* add code here
        e.g. {0xXXXX, 0xXXXX, WORD_LEN, 0},
     */
    {0x0000, 0x0000, WORD_LEN, 0},
};

static struct mt9v113_i2c_reg_conf const lens_roll_off_tbl[] = {
    /* add code here
        e.g. {0xXXXX, 0xXXXX, WORD_LEN, 0},
     */
    {0x0000, 0x0000, WORD_LEN, 0},
};

static struct mt9v113_i2c_reg_conf const pll_setup_tbl[] = {
    /* add code here
        e.g. {0xXXXX, 0xXXXX, WORD_LEN, 0},
     */
    {0x0000, 0x0000, WORD_LEN, 0},
};

/* Refresh Sequencer */
static struct mt9v113_i2c_reg_conf const sequencer_tbl[] = {
    /* add code here
        e.g. {0xXXXX, 0xXXXX, WORD_LEN, 0},
     */
    {0x0000, 0x0000, WORD_LEN, 0},
};

struct mt9v113_reg_t mt9v113_regs = {
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
};


