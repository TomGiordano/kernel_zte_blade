/*
 * drivers/media/video/msm/mt9p111_reg_samsung.c
 *
 * Refer to drivers/media/video/msm/mt9d112_reg.c
 * For IC MT9P111 of Module SAMSUNG: 5.0Mp 1/4-Inch System-On-A-Chip (SOC) CMOS Digital Image Sensor
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
  2011-03-14   wt           merge reg settings from 729 mt9p111      WT_CAM_20110314
                            5.0MP             
                            
  2011-03-07   wt           change image_width and  image_height     WT_CAM_20110307
                             of preview for zoom
  2011-02-21   wt           modify reg for mirror                    ZTE_WT_CAM_20110221
  2011-02-15   zt           modify the value of registers for the    ZTE_ZT_CAM_20110215
                            pictures taken outdoors under sunny
                            are too red
  2010-12-24   lijing       update pll setting to resolve init       ZTE_CAM_LJ_20101224
                            failure problem
  2010-12-24   lijing       improve reddish of preview               ZTE_LJ_CAM_20101224
                            update AF setting for lab test
                            update sharpness setting for lab test
  2010-12-15   jia.jia      add support for exposure compensation    ZTE_MSM_CAMERA_JIA_20101215
  2010-12-08   jia          modify setting for lab test,             ZTE_JIA_CAM_20101208
                            such saturation, sharpness and PLL
  2010-12-03   lijing       add support for reading EEPROM LSC       ZTE_LJ_CAM_20101203
                            modify config for lab test
  2010-11-25   jia          modify time delay in init process        ZTE_JIA_CAM_20101125
                            recommended by Aptina
  2010-11-24   jia          modify preview & snapshot orientation    ZTE_JIA_CAM_20101124
                            for blade board
  2010-10-22   jia          modify PLL setting to fix abnormal       ZTE_JIA_CAM_20100930
                            preview problem
  2010-09-30   lijing       update wb setting for invalidation       ZTE_LJ_CAM_20100930
                            i.e., add time delay of 500ms for
                            register of "0x8404" in MWB
  2010-09-07   lijing       fix WB_AUTO setting error                ZTE_LJ_CAM_20100907
  2010-08-24   lijing       update wb settings                       ZTE_LJ_CAM_20100824
  2010-07-20   jia          Improve effect for lab test,             JIA_CAM_20100720
                            Improve effect of AF
  2010-06-22   ye.ganlin    Add config for loading LSC config        YGL_CAM_20100622
                            from OTP memory
  2010-06-14   ye.ganlin    add new settings such as effect          YGL_CAM_20100613
  2010-01-25   li.jing      add new settings for 3.0 version         ZTE_MSM_CAMERA_LIJING_001
  2010-01-18   li.jing      rotate camera preview and snapshot image ZTE_MSM_CAMERA_LIJING_001
  2009-10-24   jia.jia      Merged from kernel-v4515                 ZTE_MSM_CAMERA_JIA_001
------------------------------------------------------------------------------------------*/

#include "mt9p111.h"

static struct mt9p111_i2c_reg_conf const otp_lsc_reg_tbl_0[] = {
    {0x098E, 0x602A, WORD_LEN, 0},   // LOGICAL_ADDRESS_ACCESS [IO_NV_MEM_COMMAND]
    {0xE02A, 0x0001, WORD_LEN, 200}, // IO_NV_MEM_COMMAND
    {0xD004, 0x04, BYTE_LEN, 0},    // PGA_SOLUTION
    {0xD00C, 0x03, BYTE_LEN, 0},    // PGA_NO_OF_ZONES

    /*
     * For 1st memory of OTP
     */
    {0xD006, 0x0000, WORD_LEN, 0},  // PGA_ZONE_ADDR_0
    {0xD008, 0x0000, WORD_LEN, 0},  // PGA_ZONE_ADDR_1
    {0xD00A, 0x0000, WORD_LEN, 0},  // PGA_ZONE_ADDR_2
    {0xD005, 0x02, BYTE_LEN, 0},    // PGA_CURRENT_ZONE

    {0xD00D, 0x00, BYTE_LEN, 0}, // PGA_ZONE_LOW_0
    {0xD00E, 0x30, BYTE_LEN, 0}, // PGA_ZONE_LOW_1
    {0xD00F, 0x65, BYTE_LEN, 0}, // PGA_ZONE_LOW_2
    {0xD011, 0x35, BYTE_LEN, 0}, // PGA_ZONE_HIGH_0
    {0xD012, 0x70, BYTE_LEN, 0}, // PGA_ZONE_HIGH_1
    {0xD013, 0xFF, BYTE_LEN, 0}, // PGA_ZONE_HIGH_2

    {0xD002, 0x0004, WORD_LEN, 0},	// PGA_ALGO
};

static struct mt9p111_i2c_reg_conf const otp_lsc_reg_tbl_1[] = {
    {0x098E, 0x602A, WORD_LEN, 0},   // LOGICAL_ADDRESS_ACCESS [IO_NV_MEM_COMMAND]
    {0xE02A, 0x0001, WORD_LEN, 200}, // IO_NV_MEM_COMMAND

    {0xD004, 0x04, BYTE_LEN, 0},    // PGA_SOLUTION
    {0xD00C, 0x03, BYTE_LEN, 0},    // PGA_NO_OF_ZONES

    /*
     * For 2nd memory of OTP
     */
    {0xD006, 0x0100, WORD_LEN, 0},  // PGA_ZONE_ADDR_0
    {0xD008, 0x0100, WORD_LEN, 0},  // PGA_ZONE_ADDR_1
    {0xD00A, 0x0100, WORD_LEN, 0},  // PGA_ZONE_ADDR_2
    {0xD005, 0x02, BYTE_LEN, 0},    // PGA_CURRENT_ZONE

    {0xD00D, 0x00, BYTE_LEN, 0}, // PGA_ZONE_LOW_0
    {0xD00E, 0x30, BYTE_LEN, 0}, // PGA_ZONE_LOW_1
    {0xD00F, 0x65, BYTE_LEN, 0}, // PGA_ZONE_LOW_2
    {0xD011, 0x35, BYTE_LEN, 0}, // PGA_ZONE_HIGH_0
    {0xD012, 0x70, BYTE_LEN, 0}, // PGA_ZONE_HIGH_1
    {0xD013, 0xFF, BYTE_LEN, 0}, // PGA_ZONE_HIGH_2

    {0xD002, 0x0004, WORD_LEN, 0},	// PGA_ALGO
};

static struct mt9p111_i2c_reg_conf const otp_lsc_reg_tbl_2[] = {
    {0x098E, 0x602A, WORD_LEN, 0},   // LOGICAL_ADDRESS_ACCESS [IO_NV_MEM_COMMAND]
    {0xE02A, 0x0001, WORD_LEN, 200}, // IO_NV_MEM_COMMAND

    {0xD004, 0x04, BYTE_LEN, 0},    // PGA_SOLUTION
    {0xD00C, 0x03, BYTE_LEN, 0},    // PGA_NO_OF_ZONES

    /*
     * For 3rd memory of OTP
     */
    {0xD006, 0x0200, WORD_LEN, 0},  // PGA_ZONE_ADDR_0
    {0xD008, 0x0200, WORD_LEN, 0},  // PGA_ZONE_ADDR_1
    {0xD00A, 0x0200, WORD_LEN, 0},  // PGA_ZONE_ADDR_2
    {0xD005, 0x02, BYTE_LEN, 0},    // PGA_CURRENT_ZONE

    {0xD00D, 0x00, BYTE_LEN, 0}, // PGA_ZONE_LOW_0
    {0xD00E, 0x30, BYTE_LEN, 0}, // PGA_ZONE_LOW_1
    {0xD00F, 0x65, BYTE_LEN, 0}, // PGA_ZONE_LOW_2
    {0xD011, 0x35, BYTE_LEN, 0}, // PGA_ZONE_HIGH_0
    {0xD012, 0x70, BYTE_LEN, 0}, // PGA_ZONE_HIGH_1
    {0xD013, 0xFF, BYTE_LEN, 0}, // PGA_ZONE_HIGH_2

    {0xD002, 0x0004, WORD_LEN, 0},	// PGA_ALGO
};

static struct mt9p111_i2c_reg_conf const *otp_lsc_reg_tbl[] = {
    otp_lsc_reg_tbl_0,
    otp_lsc_reg_tbl_1,
    otp_lsc_reg_tbl_2,
};

static uint16_t const otp_lsc_reg_tbl_sz[] = {
    ARRAY_SIZE(otp_lsc_reg_tbl_0),
    ARRAY_SIZE(otp_lsc_reg_tbl_1),
    ARRAY_SIZE(otp_lsc_reg_tbl_2),
};

//new settings from USA for version 3.0 by lijing
static struct mt9p111_i2c_reg_conf const preview_snapshot_mode_reg_settings_array[] = {
     // Timming A   
    {0xC83A, 0x000C, WORD_LEN, 0}, 	// CAM_CORE_A_Y_ADDR_START
    {0xC83C, 0x0018, WORD_LEN, 0}, 	// CAM_CORE_A_X_ADDR_START
    {0xC83E, 0x07B1, WORD_LEN, 0}, 	// CAM_CORE_A_Y_ADDR_END
    {0xC840, 0x0A45, WORD_LEN, 0}, 	// CAM_CORE_A_X_ADDR_END
    {0xC842, 0x0001, WORD_LEN, 0}, 	// CAM_CORE_A_ROW_SPEED
    {0xC844, 0x0103, WORD_LEN, 0}, 	// CAM_CORE_A_SKIP_X_CORE
    {0xC846, 0x0103, WORD_LEN, 0}, 	// CAM_CORE_A_SKIP_Y_CORE
    {0xC848, 0x0103, WORD_LEN, 0}, 	// CAM_CORE_A_SKIP_X_PIPE
    {0xC84A, 0x0103, WORD_LEN, 0}, 	// CAM_CORE_A_SKIP_Y_PIPE
    {0xC84C, 0x0080, WORD_LEN, 0}, 	// CAM_CORE_A_POWER_MODE
    {0xC84E, 0x0001, WORD_LEN, 0}, 	// CAM_CORE_A_BIN_MODE

    /* ZTE_JIA_CAM_20101124 WT_CAM_20110314
     * setting for preview orientation
     */
#if defined (CONFIG_MACH_BLADE)
    // Set orientation for blade. Flip vertically and horizontally.
    {0xC850, 0x03,   BYTE_LEN, 0},   // CAM_CORE_A_ORIENTATION
#else
    {0xC850, 0x00,   BYTE_LEN, 0}, 	// CAM_CORE_A_ORIENTATION
#endif
    {0xC851, 0x00, 	 BYTE_LEN, 0},// CAM_CORE_A_PIXEL_ORDER
    {0xC852, 0x019C, WORD_LEN, 0}, 	// CAM_CORE_A_FINE_CORRECTION
    {0xC854, 0x0732, WORD_LEN, 0}, 	// CAM_CORE_A_FINE_ITMIN
    {0xC858, 0x0002, WORD_LEN, 0}, 	// CAM_CORE_A_COARSE_ITMIN
    {0xC85A, 0x0001, WORD_LEN, 0}, 	// CAM_CORE_A_COARSE_ITMAX_MARGIN
    {0xC85C, 0x0423, WORD_LEN, 0}, 	// CAM_CORE_A_MIN_FRAME_LENGTH_LINES
    {0xC85E, 0xFFFF, WORD_LEN, 0}, 	// CAM_CORE_A_MAX_FRAME_LENGTH_LINES
    {0xC860, 0x0423, WORD_LEN, 0}, 	// CAM_CORE_A_BASE_FRAME_LENGTH_LINES
    {0xC862, 0x1194, WORD_LEN, 0}, 	// CAM_CORE_A_MIN_LINE_LENGTH_PCLK
    {0xC864, 0xFFFE, WORD_LEN, 0}, 	// CAM_CORE_A_MAX_LINE_LENGTH_PCLK
    {0xC866, 0x7F5A, WORD_LEN, 0}, 	// CAM_CORE_A_P4_5_6_DIVIDER
    {0xC868, 0x0423, WORD_LEN, 0}, 	// CAM_CORE_A_FRAME_LENGTH_LINES
    {0xC86A, 0x1194, WORD_LEN, 0}, 	// CAM_CORE_A_LINE_LENGTH_PCK
    {0xC86C, 0x0518, WORD_LEN, 0}, 	// CAM_CORE_A_OUTPUT_SIZE_WIDTH
    {0xC86E, 0x03D4, WORD_LEN, 0}, 	// CAM_CORE_A_OUTPUT_SIZE_HEIGHT
    {0xC870, 0x0014, WORD_LEN, 0}, 	// CAM_CORE_A_RX_FIFO_TRIGGER_MARK
    {0xC8B8, 0x0004, WORD_LEN, 0}, 	// CAM_OUTPUT_0_JPEG_CONTROL
    {0xC8AE, 0x0001, WORD_LEN, 0}, 	// CAM_OUTPUT_0_OUTPUT_FORMAT
    //WT_CAM_20110307 change image_width and image_height of preview for zoom
    {0xC8AA, 0x0400, WORD_LEN, 0}, 	// CAM_OUTPUT_0_IMAGE_WIDTH
    {0xC8AC, 0x0300, WORD_LEN, 0}, 	// CAM_OUTPUT_0_IMAGE_HEIGHT
    
    //Timming B for YUV 
    {0xC872, 0x0010, WORD_LEN, 0}, 	// CAM_CORE_B_Y_ADDR_START
    {0xC874, 0x001C, WORD_LEN, 0}, 	// CAM_CORE_B_X_ADDR_START
    {0xC876, 0x07AF, WORD_LEN, 0}, 	// CAM_CORE_B_Y_ADDR_END
    {0xC878, 0x0A43, WORD_LEN, 0}, 	// CAM_CORE_B_X_ADDR_END
    {0xC87A, 0x0001, WORD_LEN, 0}, 	// CAM_CORE_B_ROW_SPEED
    {0xC87C, 0x0101, WORD_LEN, 0}, 	// CAM_CORE_B_SKIP_X_CORE
    {0xC87E, 0x0101, WORD_LEN, 0}, 	// CAM_CORE_B_SKIP_Y_CORE
    {0xC880, 0x0101, WORD_LEN, 0}, 	// CAM_CORE_B_SKIP_X_PIPE
    {0xC882, 0x0101, WORD_LEN, 0}, 	// CAM_CORE_B_SKIP_Y_PIPE
    {0xC884, 0x0064, WORD_LEN, 0}, 	// CAM_CORE_B_POWER_MODE
    {0xC886, 0x0000, WORD_LEN, 0}, 	// CAM_CORE_B_BIN_MODE

    /* ZTE_JIA_CAM_20101124
     * setting for snapshot orientation
     */
#if defined (CONFIG_MACH_BLADE)
    // Set orientation for blade. Flip vertically and horizontally.
    {0xC888, 0x03,   BYTE_LEN, 0},  // CAM_CORE_B_ORIENTATION
#else
    {0xC888, 0x00,   BYTE_LEN, 0},  // CAM_CORE_B_ORIENTATION
#endif
    {0xC889, 0x00, 	 BYTE_LEN, 0},// CAM_CORE_B_PIXEL_ORDER
    {0xC88A, 0x009C, WORD_LEN, 0}, 	// CAM_CORE_B_FINE_CORRECTION
    {0xC88C, 0x034A, WORD_LEN, 0}, 	// CAM_CORE_B_FINE_ITMIN
    {0xC890, 0x0002, WORD_LEN, 0}, 	// CAM_CORE_B_COARSE_ITMIN
    {0xC892, 0x0001, WORD_LEN, 0}, 	// CAM_CORE_B_COARSE_ITMAX_MARGIN
    {0xC894, 0x082F, WORD_LEN, 0}, 	// CAM_CORE_B_MIN_FRAME_LENGTH_LINES
    {0xC896, 0xFFFF, WORD_LEN, 0}, 	// CAM_CORE_B_MAX_FRAME_LENGTH_LINES
    {0xC898, 0x082F, WORD_LEN, 0}, 	// CAM_CORE_B_BASE_FRAME_LENGTH_LINES
    {0xC89C, 0xFFFE, WORD_LEN, 0}, 	// CAM_CORE_B_MAX_LINE_LENGTH_PCLK
    {0xC89E, 0x7F5A, WORD_LEN, 0}, 	// CAM_CORE_B_P4_5_6_DIVIDER
    {0xC8A0, 0x082F, WORD_LEN, 0}, 	// CAM_CORE_B_FRAME_LENGTH_LINES
    {0xC8A4, 0x0A28, WORD_LEN, 0}, 	// CAM_CORE_B_OUTPUT_SIZE_WIDTH
    {0xC8A6, 0x07A0, WORD_LEN, 0}, 	// CAM_CORE_B_OUTPUT_SIZE_HEIGHT
    {0xC8A8, 0x0021, WORD_LEN, 0}, 	// CAM_CORE_B_RX_FIFO_TRIGGER_MARK
    {0xC8C4, 0x0001, WORD_LEN, 0}, 	// CAM_OUTPUT_1_OUTPUT_FORMAT
    {0xC8C0, 0x0A20, WORD_LEN, 0}, 	// CAM_OUTPUT_1_IMAGE_WIDTH
    {0xC8C2, 0x0798, WORD_LEN, 0}, 	// CAM_OUTPUT_1_IMAGE_HEIGHT
    {0xC89A, 0x2500, WORD_LEN, 0}, 	// CAM_CORE_B_MIN_LINE_LENGTH_PCLK
    {0xC8A2, 0x2328, WORD_LEN, 0}, 	// CAM_CORE_B_LINE_LENGTH_PCK
    {0xC8CE, 0x0014, WORD_LEN, 0}, 	// CAM_OUTPUT_1_JPEG_CONTROL
    
    // Flicker detection
    {0xA018, 0x00CC, WORD_LEN, 0}, 	// FD_EXPECTED50HZ_FLICKER_PERIOD_IN_CONTEXT_A
    {0xA01A, 0x0061, WORD_LEN, 0}, 	// FD_EXPECTED50HZ_FLICKER_PERIOD_IN_CONTEXT_B
    {0xA01C, 0x00A9, WORD_LEN, 0}, 	// FD_EXPECTED60HZ_FLICKER_PERIOD_IN_CONTEXT_A
    {0xA01E, 0x0050, WORD_LEN, 0}, 	// FD_EXPECTED60HZ_FLICKER_PERIOD_IN_CONTEXT_B
    {0xA010, 0x00B8, WORD_LEN, 0}, 	// FD_MIN_EXPECTED50HZ_FLICKER_PERIOD
    {0xA012, 0x00D6, WORD_LEN, 0}, 	// FD_MAX_EXPECTED50HZ_FLICKER_PERIOD
    {0xA014, 0x0095, WORD_LEN, 0}, 	// FD_MIN_EXPECTED60HZ_FLICKER_PERIOD
    {0xA016, 0x00B3, WORD_LEN, 0}, 	// FD_MAX_EXPECTED60HZ_FLICKER_PERIOD  
    
    //Patch 
    {0x0982, 0x0000, WORD_LEN, 0}, 	// ACCESS_CTL_STAT
    {0x098A, 0x086C, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0xC0F1, WORD_LEN, 0},
    {0x0992, 0xC5E1, WORD_LEN, 0},
    {0x0994, 0x246A, WORD_LEN, 0},
    {0x0996, 0x1280, WORD_LEN, 0},
    {0x0998, 0xC4E1, WORD_LEN, 0},
    {0x099A, 0xD20F, WORD_LEN, 0},
    {0x099C, 0x2069, WORD_LEN, 0},
    {0x099E, 0x0000, WORD_LEN, 0},
    {0x098A, 0x087C, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x6A62, WORD_LEN, 0},
    {0x0992, 0x1303, WORD_LEN, 0},
    {0x0994, 0x0084, WORD_LEN, 0},
    {0x0996, 0x1734, WORD_LEN, 0},
    {0x0998, 0x7005, WORD_LEN, 0},
    {0x099A, 0xD801, WORD_LEN, 0},
    {0x099C, 0x8A41, WORD_LEN, 0},
    {0x099E, 0xD900, WORD_LEN, 0},
    {0x098A, 0x088C, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x0D5A, WORD_LEN, 0},
    {0x0992, 0x0664, WORD_LEN, 0},
    {0x0994, 0x8B61, WORD_LEN, 0},
    {0x0996, 0xE80B, WORD_LEN, 0},
    {0x0998, 0x000D, WORD_LEN, 0},
    {0x099A, 0x0020, WORD_LEN, 0},
    {0x099C, 0xD508, WORD_LEN, 0},
    {0x099E, 0x1504, WORD_LEN, 0},
    {0x098A, 0x089C, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x1400, WORD_LEN, 0},
    {0x0992, 0x7840, WORD_LEN, 0},
    {0x0994, 0xD007, WORD_LEN, 0},
    {0x0996, 0x0DFB, WORD_LEN, 0},
    {0x0998, 0x9004, WORD_LEN, 0},
    {0x099A, 0xC4C1, WORD_LEN, 0},
    {0x099C, 0x2029, WORD_LEN, 0},
    {0x099E, 0x0300, WORD_LEN, 0},
    {0x098A, 0x08AC, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x0219, WORD_LEN, 0},
    {0x0992, 0x06C4, WORD_LEN, 0},
    {0x0994, 0xFF80, WORD_LEN, 0},
    {0x0996, 0x08C8, WORD_LEN, 0},
    {0x0998, 0xFF80, WORD_LEN, 0},
    {0x099A, 0x086C, WORD_LEN, 0},
    {0x099C, 0xFF80, WORD_LEN, 0},
    {0x099E, 0x08C0, WORD_LEN, 0},
    {0x098A, 0x08BC, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0xFF80, WORD_LEN, 0},
    {0x0992, 0x08C8, WORD_LEN, 0},
    {0x0994, 0xFF80, WORD_LEN, 0},
    {0x0996, 0x0B50, WORD_LEN, 0},
    {0x0998, 0xFF80, WORD_LEN, 0},
    {0x099A, 0x08D0, WORD_LEN, 0},
    {0x099C, 0x0002, WORD_LEN, 0},
    {0x099E, 0x0003, WORD_LEN, 0},
    {0x098A, 0x08CC, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x0000, WORD_LEN, 0},
    {0x0992, 0x0000, WORD_LEN, 0},
    {0x0994, 0xC0F1, WORD_LEN, 0},
    {0x0996, 0x0982, WORD_LEN, 0},
    {0x0998, 0x06E4, WORD_LEN, 0},
    {0x099A, 0xDA08, WORD_LEN, 0},
    {0x099C, 0xD16B, WORD_LEN, 0},
    {0x099E, 0x8900, WORD_LEN, 0},
    {0x098A, 0x08DC, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x19B0, WORD_LEN, 0},
    {0x0992, 0x0082, WORD_LEN, 0},
    {0x0994, 0xDB51, WORD_LEN, 0},
    {0x0996, 0x19B2, WORD_LEN, 0},
    {0x0998, 0x00C2, WORD_LEN, 0},
    {0x099A, 0xB8A6, WORD_LEN, 0},
    {0x099C, 0xA900, WORD_LEN, 0},
    {0x099E, 0xD8F0, WORD_LEN, 0},
    {0x098A, 0x08EC, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x19B1, WORD_LEN, 0},
    {0x0992, 0x0002, WORD_LEN, 0},
    {0x0994, 0x19B5, WORD_LEN, 0},
    {0x0996, 0x0002, WORD_LEN, 0},
    {0x0998, 0xD855, WORD_LEN, 0},
    {0x099A, 0x19B6, WORD_LEN, 0},
    {0x099C, 0x0002, WORD_LEN, 0},
    {0x099E, 0xD856, WORD_LEN, 0},
    {0x098A, 0x08FC, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x19B7, WORD_LEN, 0},
    {0x0992, 0x0002, WORD_LEN, 0},
    {0x0994, 0xD896, WORD_LEN, 0},
    {0x0996, 0x19B8, WORD_LEN, 0},
    {0x0998, 0x0004, WORD_LEN, 0},
    {0x099A, 0xD814, WORD_LEN, 0},
    {0x099C, 0x19BA, WORD_LEN, 0},
    {0x099E, 0x0004, WORD_LEN, 0},
    {0x098A, 0x090C, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0xD55F, WORD_LEN, 0},
    {0x0992, 0xD805, WORD_LEN, 0},
    {0x0994, 0xDB52, WORD_LEN, 0},
    {0x0996, 0xB111, WORD_LEN, 0},
    {0x0998, 0x19B3, WORD_LEN, 0},
    {0x099A, 0x00C2, WORD_LEN, 0},
    {0x099C, 0x19B4, WORD_LEN, 0},
    {0x099E, 0x0082, WORD_LEN, 0},
    {0x098A, 0x091C, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0xD15C, WORD_LEN, 0},
    {0x0992, 0x76A9, WORD_LEN, 0},
    {0x0994, 0x70C9, WORD_LEN, 0},
    {0x0996, 0x0ED2, WORD_LEN, 0},
    {0x0998, 0x06A4, WORD_LEN, 0},
    {0x099A, 0xDA2C, WORD_LEN, 0},
    {0x099C, 0xD05A, WORD_LEN, 0},
    {0x099E, 0xA503, WORD_LEN, 0},
    {0x098A, 0x092C, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0xD05A, WORD_LEN, 0},
    {0x0992, 0x0191, WORD_LEN, 0},
    {0x0994, 0x06E4, WORD_LEN, 0},
    {0x0996, 0xA0C0, WORD_LEN, 0},
    {0x0998, 0xC0F1, WORD_LEN, 0},
    {0x099A, 0x091E, WORD_LEN, 0},
    {0x099C, 0x06C4, WORD_LEN, 0},
    {0x099E, 0xD653, WORD_LEN, 0},
    {0x098A, 0x093C, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x8E01, WORD_LEN, 0},
    {0x0992, 0xB8A4, WORD_LEN, 0},
    {0x0994, 0xAE01, WORD_LEN, 0},
    {0x0996, 0x8E09, WORD_LEN, 0},
    {0x0998, 0xB8E0, WORD_LEN, 0},
    {0x099A, 0xF29B, WORD_LEN, 0},
    {0x099C, 0xD554, WORD_LEN, 0},
    {0x099E, 0x153A, WORD_LEN, 0},
    {0x098A, 0x094C, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x1080, WORD_LEN, 0},
    {0x0992, 0x153B, WORD_LEN, 0},
    {0x0994, 0x1081, WORD_LEN, 0},
    {0x0996, 0xB808, WORD_LEN, 0},
    {0x0998, 0x7825, WORD_LEN, 0},
    {0x099A, 0x16B8, WORD_LEN, 0},
    {0x099C, 0x1101, WORD_LEN, 0},
    {0x099E, 0x092D, WORD_LEN, 0},
    {0x098A, 0x095C, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x0003, WORD_LEN, 0},
    {0x0992, 0x16B0, WORD_LEN, 0},
    {0x0994, 0x1082, WORD_LEN, 0},
    {0x0996, 0x1E3C, WORD_LEN, 0},
    {0x0998, 0x1082, WORD_LEN, 0},
    {0x099A, 0x16B1, WORD_LEN, 0},
    {0x099C, 0x1082, WORD_LEN, 0},
    {0x099E, 0x1E3D, WORD_LEN, 0},
    {0x098A, 0x096C, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x1082, WORD_LEN, 0},
    {0x0992, 0x16B4, WORD_LEN, 0},
    {0x0994, 0x1082, WORD_LEN, 0},
    {0x0996, 0x1E3E, WORD_LEN, 0},
    {0x0998, 0x1082, WORD_LEN, 0},
    {0x099A, 0x16B5, WORD_LEN, 0},
    {0x099C, 0x1082, WORD_LEN, 0},
    {0x099E, 0x1E3F, WORD_LEN, 0},
    {0x098A, 0x097C, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x1082, WORD_LEN, 0},
    {0x0992, 0x8E40, WORD_LEN, 0},
    {0x0994, 0xBAA6, WORD_LEN, 0},
    {0x0996, 0xAE40, WORD_LEN, 0},
    {0x0998, 0x098F, WORD_LEN, 0},
    {0x099A, 0x0022, WORD_LEN, 0},
    {0x099C, 0x16BA, WORD_LEN, 0},
    {0x099E, 0x1102, WORD_LEN, 0},
    {0x098A, 0x098C, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x0A87, WORD_LEN, 0},
    {0x0992, 0x0003, WORD_LEN, 0},
    {0x0994, 0x16B2, WORD_LEN, 0},
    {0x0996, 0x1084, WORD_LEN, 0},
    {0x0998, 0x0A52, WORD_LEN, 0},
    {0x099A, 0x06A4, WORD_LEN, 0},
    {0x099C, 0x16B0, WORD_LEN, 0},
    {0x099E, 0x1083, WORD_LEN, 0},
    {0x098A, 0x099C, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x1E3C, WORD_LEN, 0},
    {0x0992, 0x1002, WORD_LEN, 0},
    {0x0994, 0x153A, WORD_LEN, 0},
    {0x0996, 0x1080, WORD_LEN, 0},
    {0x0998, 0x153B, WORD_LEN, 0},
    {0x099A, 0x1081, WORD_LEN, 0},
    {0x099C, 0x16B3, WORD_LEN, 0},
    {0x099E, 0x1084, WORD_LEN, 0},
    {0x098A, 0x09AC, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0xB808, WORD_LEN, 0},
    {0x0992, 0x7825, WORD_LEN, 0},
    {0x0994, 0x16B8, WORD_LEN, 0},
    {0x0996, 0x1101, WORD_LEN, 0},
    {0x0998, 0x16BA, WORD_LEN, 0},
    {0x099A, 0x1102, WORD_LEN, 0},
    {0x099C, 0x0A2E, WORD_LEN, 0},
    {0x099E, 0x06A4, WORD_LEN, 0},
    {0x098A, 0x09BC, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x16B1, WORD_LEN, 0},
    {0x0992, 0x1083, WORD_LEN, 0},
    {0x0994, 0x1E3D, WORD_LEN, 0},
    {0x0996, 0x1002, WORD_LEN, 0},
    {0x0998, 0x153A, WORD_LEN, 0},
    {0x099A, 0x1080, WORD_LEN, 0},
    {0x099C, 0x153B, WORD_LEN, 0},
    {0x099E, 0x1081, WORD_LEN, 0},
    {0x098A, 0x09CC, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x16B6, WORD_LEN, 0},
    {0x0992, 0x1084, WORD_LEN, 0},
    {0x0994, 0xB808, WORD_LEN, 0},
    {0x0996, 0x7825, WORD_LEN, 0},
    {0x0998, 0x16B8, WORD_LEN, 0},
    {0x099A, 0x1101, WORD_LEN, 0},
    {0x099C, 0x16BA, WORD_LEN, 0},
    {0x099E, 0x1102, WORD_LEN, 0},
    {0x098A, 0x09DC, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x0A0A, WORD_LEN, 0},
    {0x0992, 0x06A4, WORD_LEN, 0},
    {0x0994, 0x16B4, WORD_LEN, 0},
    {0x0996, 0x1083, WORD_LEN, 0},
    {0x0998, 0x1E3E, WORD_LEN, 0},
    {0x099A, 0x1002, WORD_LEN, 0},
    {0x099C, 0x153A, WORD_LEN, 0},
    {0x099E, 0x1080, WORD_LEN, 0},
    {0x098A, 0x09EC, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x153B, WORD_LEN, 0},
    {0x0992, 0x1081, WORD_LEN, 0},
    {0x0994, 0x16B7, WORD_LEN, 0},
    {0x0996, 0x1084, WORD_LEN, 0},
    {0x0998, 0xB808, WORD_LEN, 0},
    {0x099A, 0x7825, WORD_LEN, 0},
    {0x099C, 0x16B8, WORD_LEN, 0},
    {0x099E, 0x1101, WORD_LEN, 0},
    {0x098A, 0x09FC, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x16BA, WORD_LEN, 0},
    {0x0992, 0x1102, WORD_LEN, 0},
    {0x0994, 0x09E6, WORD_LEN, 0},
    {0x0996, 0x06A4, WORD_LEN, 0},
    {0x0998, 0x16B5, WORD_LEN, 0},
    {0x099A, 0x1083, WORD_LEN, 0},
    {0x099C, 0x1E3F, WORD_LEN, 0},
    {0x099E, 0x1002, WORD_LEN, 0},
    {0x098A, 0x0A0C, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x8E00, WORD_LEN, 0},
    {0x0992, 0xB8A6, WORD_LEN, 0},
    {0x0994, 0xAE00, WORD_LEN, 0},
    {0x0996, 0x153A, WORD_LEN, 0},
    {0x0998, 0x1081, WORD_LEN, 0},
    {0x099A, 0x153B, WORD_LEN, 0},
    {0x099C, 0x1080, WORD_LEN, 0},
    {0x099E, 0xB908, WORD_LEN, 0},
    {0x098A, 0x0A1C, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x7905, WORD_LEN, 0},
    {0x0992, 0x16BA, WORD_LEN, 0},
    {0x0994, 0x1100, WORD_LEN, 0},
    {0x0996, 0x085B, WORD_LEN, 0},
    {0x0998, 0x0042, WORD_LEN, 0},
    {0x099A, 0xD01E, WORD_LEN, 0},
    {0x099C, 0x9E31, WORD_LEN, 0},
    {0x099E, 0x904D, WORD_LEN, 0},
    {0x098A, 0x0A2C, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x0A2B, WORD_LEN, 0},
    {0x0992, 0x0063, WORD_LEN, 0},
    {0x0994, 0x8E00, WORD_LEN, 0},
    {0x0996, 0x16B0, WORD_LEN, 0},
    {0x0998, 0x1081, WORD_LEN, 0},
    {0x099A, 0x1E3C, WORD_LEN, 0},
    {0x099C, 0x1042, WORD_LEN, 0},
    {0x099E, 0x16B1, WORD_LEN, 0},
    {0x098A, 0x0A3C, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x1081, WORD_LEN, 0},
    {0x0992, 0x1E3D, WORD_LEN, 0},
    {0x0994, 0x1042, WORD_LEN, 0},
    {0x0996, 0x16B4, WORD_LEN, 0},
    {0x0998, 0x1081, WORD_LEN, 0},
    {0x099A, 0x1E3E, WORD_LEN, 0},
    {0x099C, 0x1042, WORD_LEN, 0},
    {0x099E, 0x16B5, WORD_LEN, 0},
    {0x098A, 0x0A4C, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x1081, WORD_LEN, 0},
    {0x0992, 0x1E3F, WORD_LEN, 0},
    {0x0994, 0x1042, WORD_LEN, 0},
    {0x0996, 0xB886, WORD_LEN, 0},
    {0x0998, 0xF012, WORD_LEN, 0},
    {0x099A, 0x16B2, WORD_LEN, 0},
    {0x099C, 0x1081, WORD_LEN, 0},
    {0x099E, 0xB8A6, WORD_LEN, 0},
    {0x098A, 0x0A5C, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x1E3C, WORD_LEN, 0},
    {0x0992, 0x1042, WORD_LEN, 0},
    {0x0994, 0x16B3, WORD_LEN, 0},
    {0x0996, 0x1081, WORD_LEN, 0},
    {0x0998, 0x1E3D, WORD_LEN, 0},
    {0x099A, 0x1042, WORD_LEN, 0},
    {0x099C, 0x16B6, WORD_LEN, 0},
    {0x099E, 0x1081, WORD_LEN, 0},
    {0x098A, 0x0A6C, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x1E3E, WORD_LEN, 0},
    {0x0992, 0x1042, WORD_LEN, 0},
    {0x0994, 0x16B7, WORD_LEN, 0},
    {0x0996, 0x1081, WORD_LEN, 0},
    {0x0998, 0x1E3F, WORD_LEN, 0},
    {0x099A, 0x1042, WORD_LEN, 0},
    {0x099C, 0xAE00, WORD_LEN, 0},
    {0x099E, 0x08B6, WORD_LEN, 0},
    {0x098A, 0x0A7C, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x01C4, WORD_LEN, 0},
    {0x0992, 0x0041, WORD_LEN, 0},
    {0x0994, 0x06C4, WORD_LEN, 0},
    {0x0996, 0x78E0, WORD_LEN, 0},
    {0x0998, 0xFF80, WORD_LEN, 0},
    {0x099A, 0x0314, WORD_LEN, 0},
    {0x099C, 0xFF80, WORD_LEN, 0},
    {0x099E, 0x0BD8, WORD_LEN, 0},
    {0x098A, 0x0A8C, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x0000, WORD_LEN, 0},
    {0x0992, 0xF444, WORD_LEN, 0},
    {0x0994, 0xFF80, WORD_LEN, 0},
    {0x0996, 0x0934, WORD_LEN, 0},
    {0x0998, 0x8000, WORD_LEN, 0},
    {0x099A, 0x009C, WORD_LEN, 0},
    {0x099C, 0xFF80, WORD_LEN, 0},
    {0x099E, 0x0250, WORD_LEN, 0},
    {0x098A, 0x0A9C, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0xFF80, WORD_LEN, 0},
    {0x0992, 0x050C, WORD_LEN, 0},
    {0x0994, 0xC0F1, WORD_LEN, 0},
    {0x0996, 0x0FA2, WORD_LEN, 0},
    {0x0998, 0x0684, WORD_LEN, 0},
    {0x099A, 0xD633, WORD_LEN, 0},
    {0x099C, 0x7708, WORD_LEN, 0},
    {0x099E, 0x8E01, WORD_LEN, 0},
    {0x098A, 0x0AAC, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x1604, WORD_LEN, 0},
    {0x0992, 0x1091, WORD_LEN, 0},
    {0x0994, 0x2046, WORD_LEN, 0},
    {0x0996, 0x00C1, WORD_LEN, 0},
    {0x0998, 0x202F, WORD_LEN, 0},
    {0x099A, 0x2047, WORD_LEN, 0},
    {0x099C, 0xAE21, WORD_LEN, 0},
    {0x099E, 0x0F8F, WORD_LEN, 0},
    {0x098A, 0x0ABC, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x1440, WORD_LEN, 0},
    {0x0992, 0x8EAA, WORD_LEN, 0},
    {0x0994, 0x8E0B, WORD_LEN, 0},
    {0x0996, 0x224A, WORD_LEN, 0},
    {0x0998, 0x2040, WORD_LEN, 0},
    {0x099A, 0x8E2D, WORD_LEN, 0},
    {0x099C, 0xBD08, WORD_LEN, 0},
    {0x099E, 0x7D05, WORD_LEN, 0},
    {0x098A, 0x0ACC, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x8E0C, WORD_LEN, 0},
    {0x0992, 0xB808, WORD_LEN, 0},
    {0x0994, 0x7825, WORD_LEN, 0},
    {0x0996, 0x7510, WORD_LEN, 0},
    {0x0998, 0x22C2, WORD_LEN, 0},
    {0x099A, 0x248C, WORD_LEN, 0},
    {0x099C, 0x081D, WORD_LEN, 0},
    {0x099E, 0x0363, WORD_LEN, 0},
    {0x098A, 0x0ADC, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0xD9FF, WORD_LEN, 0},
    {0x0992, 0x2502, WORD_LEN, 0},
    {0x0994, 0x1002, WORD_LEN, 0},
    {0x0996, 0x2A05, WORD_LEN, 0},
    {0x0998, 0x03FE, WORD_LEN, 0},
    {0x099A, 0x0842, WORD_LEN, 0},
    {0x099C, 0x06E4, WORD_LEN, 0},
    {0x099E, 0x702F, WORD_LEN, 0},
    {0x098A, 0x0AEC, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x7810, WORD_LEN, 0},
    {0x0992, 0x7D02, WORD_LEN, 0},
    {0x0994, 0x7DB0, WORD_LEN, 0},
    {0x0996, 0xF00B, WORD_LEN, 0},
    {0x0998, 0x78A2, WORD_LEN, 0},
    {0x099A, 0x2805, WORD_LEN, 0},
    {0x099C, 0x03FE, WORD_LEN, 0},
    {0x099E, 0x082E, WORD_LEN, 0},
    {0x098A, 0x0AFC, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x06E4, WORD_LEN, 0},
    {0x0992, 0x702F, WORD_LEN, 0},
    {0x0994, 0x7810, WORD_LEN, 0},
    {0x0996, 0x651D, WORD_LEN, 0},
    {0x0998, 0x7DB0, WORD_LEN, 0},
    {0x099A, 0x7DAF, WORD_LEN, 0},
    {0x099C, 0x8E08, WORD_LEN, 0},
    {0x099E, 0xBD06, WORD_LEN, 0},
    {0x098A, 0x0B0C, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0xD11A, WORD_LEN, 0},
    {0x0992, 0xB8C3, WORD_LEN, 0},
    {0x0994, 0x78A5, WORD_LEN, 0},
    {0x0996, 0xB88F, WORD_LEN, 0},
    {0x0998, 0x1908, WORD_LEN, 0},
    {0x099A, 0x0024, WORD_LEN, 0},
    {0x099C, 0x2841, WORD_LEN, 0},
    {0x099E, 0x0201, WORD_LEN, 0},
    {0x098A, 0x0B1C, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x1E26, WORD_LEN, 0},
    {0x0992, 0x1042, WORD_LEN, 0},
    {0x0994, 0x0F15, WORD_LEN, 0},
    {0x0996, 0x1463, WORD_LEN, 0},
    {0x0998, 0x1E27, WORD_LEN, 0},
    {0x099A, 0x1002, WORD_LEN, 0},
    {0x099C, 0x224C, WORD_LEN, 0},
    {0x099E, 0xA000, WORD_LEN, 0},
    {0x098A, 0x0B2C, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x224A, WORD_LEN, 0},
    {0x0992, 0x2040, WORD_LEN, 0},
    {0x0994, 0x22C2, WORD_LEN, 0},
    {0x0996, 0x2482, WORD_LEN, 0},
    {0x0998, 0x204F, WORD_LEN, 0},
    {0x099A, 0x2040, WORD_LEN, 0},
    {0x099C, 0x224C, WORD_LEN, 0},
    {0x099E, 0xA000, WORD_LEN, 0},
    {0x098A, 0x0B3C, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0xB8A2, WORD_LEN, 0},
    {0x0992, 0xF204, WORD_LEN, 0},
    {0x0994, 0x2045, WORD_LEN, 0},
    {0x0996, 0x2180, WORD_LEN, 0},
    {0x0998, 0xAE01, WORD_LEN, 0},
    {0x099A, 0x0BCA, WORD_LEN, 0},
    {0x099C, 0xFFE3, WORD_LEN, 0},
    {0x099E, 0x70E9, WORD_LEN, 0},
    {0x098A, 0x0B4C, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x0751, WORD_LEN, 0},
    {0x0992, 0x0684, WORD_LEN, 0},
    {0x0994, 0xC0F1, WORD_LEN, 0},
    {0x0996, 0xD00A, WORD_LEN, 0},
    {0x0998, 0xD20A, WORD_LEN, 0},
    {0x099A, 0xD10B, WORD_LEN, 0},
    {0x099C, 0xA040, WORD_LEN, 0},
    {0x099E, 0x890C, WORD_LEN, 0},
    {0x098A, 0x0B5C, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x080F, WORD_LEN, 0},
    {0x0992, 0x01DE, WORD_LEN, 0},
    {0x0994, 0xB8A7, WORD_LEN, 0},
    {0x0996, 0x8243, WORD_LEN, 0},
    {0x0998, 0xA90C, WORD_LEN, 0},
    {0x099A, 0x7A60, WORD_LEN, 0},
    {0x099C, 0x890C, WORD_LEN, 0},
    {0x099E, 0xC0D1, WORD_LEN, 0},
    {0x098A, 0x0B6C, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x7EE0, WORD_LEN, 0},
    {0x0992, 0x78E0, WORD_LEN, 0},
    {0x0994, 0xFF80, WORD_LEN, 0},
    {0x0996, 0x0158, WORD_LEN, 0},
    {0x0998, 0xFF00, WORD_LEN, 0},
    {0x099A, 0x0618, WORD_LEN, 0},
    {0x099C, 0x8000, WORD_LEN, 0},
    {0x099E, 0x0008, WORD_LEN, 0},
    {0x098A, 0x0B7C, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0xFF80, WORD_LEN, 0},
    {0x0992, 0x0BC8, WORD_LEN, 0},
    {0x0994, 0xFF80, WORD_LEN, 0},
    {0x0996, 0x0174, WORD_LEN, 0},
    {0x0998, 0xE280, WORD_LEN, 0},
    {0x099A, 0x24CA, WORD_LEN, 0},
    {0x099C, 0x7082, WORD_LEN, 0},
    {0x099E, 0x78E0, WORD_LEN, 0},
    {0x098A, 0x0B8C, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x20E8, WORD_LEN, 0},
    {0x0992, 0x01A2, WORD_LEN, 0},
    {0x0994, 0x1002, WORD_LEN, 0},
    {0x0996, 0x0D02, WORD_LEN, 0},
    {0x0998, 0x1902, WORD_LEN, 0},
    {0x099A, 0x0094, WORD_LEN, 0},
    {0x099C, 0x7FE0, WORD_LEN, 0},
    {0x099E, 0x7028, WORD_LEN, 0},
    {0x098A, 0x0B9C, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x7308, WORD_LEN, 0},
    {0x0992, 0x1000, WORD_LEN, 0},
    {0x0994, 0x0900, WORD_LEN, 0},
    {0x0996, 0x7904, WORD_LEN, 0},
    {0x0998, 0x7947, WORD_LEN, 0},
    {0x099A, 0x1B00, WORD_LEN, 0},
    {0x099C, 0x0064, WORD_LEN, 0},
    {0x099E, 0x7EE0, WORD_LEN, 0},
    {0x098A, 0x0BAC, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0xE280, WORD_LEN, 0},
    {0x0992, 0x24CA, WORD_LEN, 0},
    {0x0994, 0x7082, WORD_LEN, 0},
    {0x0996, 0x78E0, WORD_LEN, 0},
    {0x0998, 0x20E8, WORD_LEN, 0},
    {0x099A, 0x01A2, WORD_LEN, 0},
    {0x099C, 0x1102, WORD_LEN, 0},
    {0x099E, 0x0502, WORD_LEN, 0},
    {0x098A, 0x0BBC, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0x1802, WORD_LEN, 0},
    {0x0992, 0x00B4, WORD_LEN, 0},
    {0x0994, 0x7FE0, WORD_LEN, 0},
    {0x0996, 0x7028, WORD_LEN, 0},
    {0x0998, 0x0000, WORD_LEN, 0},
    {0x099A, 0x0000, WORD_LEN, 0},
    {0x099C, 0xFF80, WORD_LEN, 0},
    {0x099E, 0x0B50, WORD_LEN, 0},
    {0x098A, 0x0BCC, WORD_LEN, 0}, 	// PHYSICAL_ADDRESS_ACCESS
    {0x0990, 0xFF80, WORD_LEN, 0},
    {0x0992, 0x0AA0, WORD_LEN, 0},
    {0x0994, 0x0000, WORD_LEN, 0},
    {0x0996, 0x08DC, WORD_LEN, 0},
    {0x0998, 0x0000, WORD_LEN, 0},
    {0x099A, 0x0998, WORD_LEN, 0},                            
    {0x098E, 0x0016, WORD_LEN, 0}, 	// LOGICAL_ADDRESS_ACCESS [MON_ADDRESS_LO]
    {0x8016, 0x086C, WORD_LEN, 0}, 	// MON_ADDRESS_LO                     

    /* ZTE_JIA_CAM_20101125
     * modify time delay from 100ms to 200ms recommended by Aptina
     */
    {0x8002, 0x0001, WORD_LEN, 200},// MON_CMD
                                
    //Char setting              
    {0x30D4, 0x9080, WORD_LEN, 0}, 	// COLUMN_CORRECTION           
    {0x316E, 0xC400, WORD_LEN, 0}, 	// DAC_ECL                     
    {0x305E, 0x10A0, WORD_LEN, 0}, 	// GLOBAL_GAIN                 
    {0x3E00, 0x0010, WORD_LEN, 0}, 	// SAMP_CONTROL                
    {0x3E02, 0xED02, WORD_LEN, 0}, 	// SAMP_ADDR_EN                
    {0x3E04, 0xC88C, WORD_LEN, 0}, 	// SAMP_RD1_SIG                
    {0x3E06, 0xC88C, WORD_LEN, 0}, 	// SAMP_RD1_SIG_BOOST          
    {0x3E08, 0x700A, WORD_LEN, 0}, 	// SAMP_RD1_RST                
    {0x3E0A, 0x701E, WORD_LEN, 0}, 	// SAMP_RD1_RST_BOOST          
    {0x3E0C, 0x00FF, WORD_LEN, 0}, 	// SAMP_RST1_EN                
    {0x3E0E, 0x00FF, WORD_LEN, 0}, 	// SAMP_RST1_BOOST             
    {0x3E10, 0x00FF, WORD_LEN, 0}, 	// SAMP_RST1_CLOOP_SH          
    {0x3E12, 0x0000, WORD_LEN, 0}, 	// SAMP_RST_BOOST_SEQ          
    {0x3E14, 0xC78C, WORD_LEN, 0}, 	// SAMP_SAMP1_SIG              
    {0x3E16, 0x6E06, WORD_LEN, 0}, 	// SAMP_SAMP1_RST              
    {0x3E18, 0xA58C, WORD_LEN, 0}, 	// SAMP_TX_EN                  
    {0x3E1A, 0xA58E, WORD_LEN, 0}, 	// SAMP_TX_BOOST               
    {0x3E1C, 0xA58E, WORD_LEN, 0}, 	// SAMP_TX_CLOOP_SH            
    {0x3E1E, 0xC0D0, WORD_LEN, 0}, 	// SAMP_TX_BOOST_SEQ           
    {0x3E20, 0xEB00, WORD_LEN, 0}, 	// SAMP_VLN_EN                 
    {0x3E22, 0x00FF, WORD_LEN, 0}, 	// SAMP_VLN_HOLD               
    {0x3E24, 0xEB02, WORD_LEN, 0}, 	// SAMP_VCL_EN                 
    {0x3E26, 0xEA02, WORD_LEN, 0}, 	// SAMP_COLCLAMP               
    {0x3E28, 0xEB0A, WORD_LEN, 0}, 	// SAMP_SH_VCL                 
    {0x3E2A, 0xEC01, WORD_LEN, 0}, 	// SAMP_SH_VREF                
    {0x3E2C, 0xEB01, WORD_LEN, 0}, 	// SAMP_SH_VBST                
    {0x3E2E, 0x00FF, WORD_LEN, 0}, 	// SAMP_SPARE                  
    {0x3E30, 0x00F3, WORD_LEN, 0}, 	// SAMP_READOUT                
    {0x3E32, 0x3DFA, WORD_LEN, 0}, 	// SAMP_RESET_DONE             
    {0x3E34, 0x00FF, WORD_LEN, 0}, 	// SAMP_VLN_CLAMP              
    {0x3E36, 0x00F3, WORD_LEN, 0}, 	// SAMP_ASC_INT                
    {0x3E38, 0x0000, WORD_LEN, 0}, 	// SAMP_RS_CLOOP_SH_R          
    {0x3E3A, 0xF802, WORD_LEN, 0}, 	// SAMP_RS_CLOOP_SH            
    {0x3E3C, 0x0FFF, WORD_LEN, 0}, 	// SAMP_RS_BOOST_SEQ           
    {0x3E3E, 0xEA10, WORD_LEN, 0}, 	// SAMP_TXLO_GND               
    {0x3E40, 0xEB05, WORD_LEN, 0}, 	// SAMP_VLN_PER_COL            
    {0x3E42, 0xE5C8, WORD_LEN, 0}, 	// SAMP_RD2_SIG                
    {0x3E44, 0xE5C8, WORD_LEN, 0}, 	// SAMP_RD2_SIG_BOOST          
    {0x3E46, 0x8C70, WORD_LEN, 0}, 	// SAMP_RD2_RST                
    {0x3E48, 0x8C71, WORD_LEN, 0}, 	// SAMP_RD2_RST_BOOST          
    {0x3E4A, 0x00FF, WORD_LEN, 0}, 	// SAMP_RST2_EN                
    {0x3E4C, 0x00FF, WORD_LEN, 0}, 	// SAMP_RST2_BOOST             
    {0x3E4E, 0x00FF, WORD_LEN, 0}, 	// SAMP_RST2_CLOOP_SH          
    {0x3E50, 0xE38D, WORD_LEN, 0}, 	// SAMP_SAMP2_SIG              
    {0x3E52, 0x8B0A, WORD_LEN, 0}, 	// SAMP_SAMP2_RST              
    {0x3E58, 0xEB0A, WORD_LEN, 0}, 	// SAMP_PIX_CLAMP_EN           
    {0x3E5C, 0x0A00, WORD_LEN, 0}, 	// SAMP_PIX_PULLUP_EN          
    {0x3E5E, 0x00FF, WORD_LEN, 0}, 	// SAMP_PIX_PULLDOWN_EN_R      
    {0x3E60, 0x00FF, WORD_LEN, 0}, 	// SAMP_PIX_PULLDOWN_EN_S      
    {0x3E90, 0x3C01, WORD_LEN, 0}, 	// RST_ADDR_EN                 
    {0x3E92, 0x00FF, WORD_LEN, 0}, 	// RST_RST_EN                  
    {0x3E94, 0x00FF, WORD_LEN, 0}, 	// RST_RST_BOOST               
    {0x3E96, 0x3C00, WORD_LEN, 0}, 	// RST_TX_EN                   
    {0x3E98, 0x3C00, WORD_LEN, 0}, 	// RST_TX_BOOST                
    {0x3E9A, 0x3C00, WORD_LEN, 0}, 	// RST_TX_CLOOP_SH             
    {0x3E9C, 0xC0E0, WORD_LEN, 0}, 	// RST_TX_BOOST_SEQ            
    {0x3E9E, 0x00FF, WORD_LEN, 0}, 	// RST_RST_CLOOP_SH            
    {0x3EA0, 0x0000, WORD_LEN, 0}, 	// RST_RST_BOOST_SEQ           
    {0x3EA6, 0x3C00, WORD_LEN, 0}, 	// RST_PIX_PULLUP_EN           
    {0x3ED8, 0x3057, WORD_LEN, 0}, 	// DAC_LD_12_13                
    {0x316C, 0xB44F, WORD_LEN, 0}, 	// DAC_TXLO                    
    {0x316E, 0xC6FF, WORD_LEN, 0}, 	// DAC_ECL                     
    {0x3ED2, 0xEA0A, WORD_LEN, 0}, 	// DAC_LD_6_7                  
    {0x3ED4, 0x00A3, WORD_LEN, 0}, 	// DAC_LD_8_9                  
    {0x3EDC, 0x6020, WORD_LEN, 0}, 	// DAC_LD_16_17                
    {0x3EE6, 0xA541, WORD_LEN, 0}, 	// DAC_LD_26_27                
    {0x31E0, 0x0000, WORD_LEN, 0}, 	// PIX_DEF_ID                  
    {0x3ED0, 0x2409, WORD_LEN, 0}, 	// DAC_LD_4_5                  
    {0x3EDE, 0x0A49, WORD_LEN, 0}, 	// DAC_LD_18_19                
    {0x3EE0, 0x4910, WORD_LEN, 0}, 	// DAC_LD_20_21                
    {0x3EE2, 0x09D2, WORD_LEN, 0}, 	// DAC_LD_22_23                
    {0x30B6, 0x0006, WORD_LEN, 0}, 	// AUTOLR_CONTROL
    {0x337C, 0x0006, WORD_LEN, 0}, 	// YUV_YCBCR_CONTROL           
    {0x3E1A, 0xA582, WORD_LEN, 0}, 	// SAMP_TX_BOOST               
    {0x3E2E, 0xEC05, WORD_LEN, 0}, 	// SAMP_SPARE                  
    {0x3EE6, 0xA5C0, WORD_LEN, 0}, 	// DAC_LD_26_27                
    //merge from 729 mt9p111 5.0MP reg settings WT_CAM_20110314
    {0x316C, 0xF43F, WORD_LEN, 0}, 	// DAC_TXLO                    
    {0x316E, 0xC6FF, WORD_LEN, 0}, 	// DAC_ECL                     
    
    //[AWB and CCM]
    {0x098E, 0x2C46,WORD_LEN,0}, 	// LOGICAL_ADDRESS_ACCESS
    {0xAC46, 0x01F8,WORD_LEN,0}, 	// AWB_LEFT_CCM_0
    {0xAC48, 0xFECC,WORD_LEN,0}, 	// AWB_LEFT_CCM_1
    {0xAC4A, 0x003D,WORD_LEN,0}, 	// AWB_LEFT_CCM_2
    {0xAC4C, 0xFFBA,WORD_LEN,0}, 	// AWB_LEFT_CCM_3
    {0xAC4E, 0x015E,WORD_LEN,0}, 	// AWB_LEFT_CCM_4
    {0xAC50, 0xFFE8,WORD_LEN,0}, 	// AWB_LEFT_CCM_5
    {0xAC52, 0xFFFD,WORD_LEN,0}, 	// AWB_LEFT_CCM_6
    {0xAC54, 0xFF30,WORD_LEN,0}, 	// AWB_LEFT_CCM_7
    {0xAC56, 0x01D2,WORD_LEN,0}, 	// AWB_LEFT_CCM_8
    {0xAC58, 0x00B6,WORD_LEN,0}, 	// AWB_LEFT_CCM_R2BRATIO
    {0xAC5C, 0x01CD,WORD_LEN,0}, 	// AWB_RIGHT_CCM_0
    {0xAC5E, 0xFF63,WORD_LEN,0}, 	// AWB_RIGHT_CCM_1
    {0xAC60, 0xFFD0,WORD_LEN,0}, 	// AWB_RIGHT_CCM_2
    {0xAC62, 0xFFC3,WORD_LEN,0}, 	// AWB_RIGHT_CCM_3
    {0xAC64, 0x0151,WORD_LEN,0}, 	// AWB_RIGHT_CCM_4
    {0xAC66, 0xFFEC,WORD_LEN,0}, 	// AWB_RIGHT_CCM_5
    {0xAC68, 0xFFF0,WORD_LEN,0}, 	// AWB_RIGHT_CCM_6
    {0xAC6A, 0xFFA4,WORD_LEN,0}, 	// AWB_RIGHT_CCM_7
    {0xAC6C, 0x016D,WORD_LEN,0}, 	// AWB_RIGHT_CCM_8
    {0xAC6E, 0x0056,WORD_LEN,0}, 	// AWB_RIGHT_CCM_R2BRATIO
    {0xB83E, 0x00,BYTE_LEN,0}, 	// STAT_AWB_WINDOW_POS_X
    {0xB83F, 0x00,BYTE_LEN,0}, 	// STAT_AWB_WINDOW_POS_Y
    {0xB840, 0xFF,BYTE_LEN,0}, 	// STAT_AWB_WINDOW_SIZE_X
    {0xB841, 0xEF,BYTE_LEN,0}, 	// STAT_AWB_WINDOW_SIZE_Y
    {0xAC3C, 0x2E,BYTE_LEN,0}, 	// AWB_MIN_ACCEPTED_PRE_AWB_R2G_RATIO
    {0xAC3D, 0x84,BYTE_LEN,0}, 	// AWB_MAX_ACCEPTED_PRE_AWB_R2G_RATIO
    {0xAC3E, 0x11,BYTE_LEN,0}, 	// AWB_MIN_ACCEPTED_PRE_AWB_B2G_RATIO
    {0xAC3F, 0x63,BYTE_LEN,0}, 	// AWB_MAX_ACCEPTED_PRE_AWB_B2G_RATIO
	/* 
	 * ZTE_LJ_CAM_20101224
	 * improve reddish of preview
	 */
    {0xAC40, 0x62,BYTE_LEN,0}, 	// AWB_MIN_ACCEPTED_POST_AWB_R2G_RATIO
    {0xAC41, 0x68,BYTE_LEN,0}, 	// AWB_MAX_ACCEPTED_POST_AWB_R2G_RATIO
    
    {0xAC42, 0x62,BYTE_LEN,0}, 	// AWB_MIN_ACCEPTED_POST_AWB_B2G_RATIO
    {0xAC43, 0x6A,BYTE_LEN,0}, 	// AWB_MAX_ACCEPTED_POST_AWB_B2G_RATIO
    {0xB842, 0x003D,WORD_LEN,0}, 	// STAT_AWB_GRAY_CHECKER_OFFSET_X
    {0xB844, 0x0044,WORD_LEN,0}, 	// STAT_AWB_GRAY_CHECKER_OFFSET_Y
    {0x3240, 0x0024,WORD_LEN,0}, 	// AWB_XY_SCALE
    {0x3240, 0x0024,WORD_LEN,0}, 	// AWB_XY_SCALE
    {0x3242, 0x0000,WORD_LEN,0}, 	// AWB_WEIGHT_R0
    {0x3244, 0x0000,WORD_LEN,0}, 	// AWB_WEIGHT_R1
    {0x3246, 0x0000,WORD_LEN,0}, 	// AWB_WEIGHT_R2
    {0x3248, 0x7F00,WORD_LEN,0}, 	// AWB_WEIGHT_R3
    {0x324A, 0xA500,WORD_LEN,0}, 	// AWB_WEIGHT_R4
    {0x324C, 0x1540,WORD_LEN,0}, 	// AWB_WEIGHT_R5
    {0x324E, 0x01AC,WORD_LEN,0}, 	// AWB_WEIGHT_R6
    {0x3250, 0x003E,WORD_LEN,0}, 	// AWB_WEIGHT_R7
    {0x3262, 0xF708,WORD_LEN,0}, 	// AWB_LUMA_TH
    {0xACB0, 0x2B,BYTE_LEN,0}, 	// AWB_RG_MIN
    {0xACB1, 0x84,BYTE_LEN,0}, 	// AWB_RG_MAX
    {0xACB2, 0x51,BYTE_LEN,0}, 	// AWB_RG_MIN_BRIGHT
    {0xACB3, 0x52,BYTE_LEN,0}, 	// AWB_RG_MAX_BRIGHT
    {0xACB4, 0x11,BYTE_LEN,0}, 	// AWB_BG_MIN
    {0xACB5, 0x63,BYTE_LEN,0}, 	// AWB_BG_MAX
    {0xACB6, 0x55,BYTE_LEN,0}, 	// AWB_BG_MIN_BRIGHT
    {0xACB7, 0x56,BYTE_LEN,0}, 	// AWB_BG_MAX_BRIGHT
    {0xACB8, 0x0096,WORD_LEN,0}, 	// AWB_START_NUM_INT_LINES
    {0xACBA, 0x0014,WORD_LEN,0}, 	// AWB_END_NUM_INT_LINES    
    
    {0xAC01, 0xEB,BYTE_LEN,0}, 	// AWB_MODE

    {0xAC96, 0x90,BYTE_LEN,0}, 	// AWB_MATRIX_TH
    {0xAC97, 0x6E,BYTE_LEN,0}, 	// AWB_LEFT_TINT_COEF_FOR_CCM_ROW_0 For RED in A light
    {0xAC98, 0x80,BYTE_LEN,0}, 	// AWB_LEFT_TINT_COEF_FOR_CCM_ROW_1 For Green do not change this
    {0xAC99, 0x97,BYTE_LEN,0}, 	// AWB_LEFT_TINT_COEF_FOR_CCM_ROW_2 FOr Blue in A light
    {0xAC9A, 0x8A,BYTE_LEN,0}, 	// AWB_RIGHT_TINT_COEF_FOR_CCM_ROW_0
    {0xAC9B, 0x80,BYTE_LEN,0}, 	// AWB_RIGHT_TINT_COEF_FOR_CCM_ROW_1
    {0xAC9C, 0x70,BYTE_LEN,0}, 	// AWB_RIGHT_TINT_COEF_FOR_CCM_ROW_2
    
    {0x8404, 0x06,BYTE_LEN,250}, 	// SEQ_CMD
    {0x8404, 0x05,BYTE_LEN,250}, 	// SEQ_CMD
    
    // Bright DNP Patch
    {0xAC09, 0x01,   BYTE_LEN, 0}, 	// AWB_MODE_EX
    {0xACB0, 0x2F,   BYTE_LEN, 0}, 	// AWB_RG_MIN
    {0xACB1, 0x7F,   BYTE_LEN, 0}, 	// AWB_RG_MAX
    {0xACB4, 0x13,   BYTE_LEN, 0}, 	// AWB_BG_MIN
    {0xACB5, 0x5B,   BYTE_LEN, 0}, 	// AWB_BG_MAX
    {0xACB2, 0x32,   BYTE_LEN, 0}, 	// AWB_RG_MIN_BRIGHT
    {0xACB3, 0x42,   BYTE_LEN, 0}, 	// AWB_RG_MAX_BRIGHT
    {0xACB6, 0x30,   BYTE_LEN, 0}, 	// AWB_BG_MIN_BRIGHT
    {0xACB7, 0x4F,   BYTE_LEN, 0}, 	// AWB_BG_MAX_BRIGHT
    {0xAC22, 0x0005, WORD_LEN, 0}, 	// AWB_SHARPNESS_TH
    {0xACB8, 0x0056, WORD_LEN, 0}, 	// AWB_START_NUM_INT_LINES
    {0xACBA, 0x0014, WORD_LEN, 0}, 	// AWB_END_NUM_INT_LINES
    
    // setction6 PA calibration
    //JPEG Quailty
    //system setting
    {0x301A, 0x10F4, WORD_LEN, 0}, 	// RESET_REGISTER
    {0x301E, 0x0084, WORD_LEN, 0}, 	// DATA_PEDESTAL
    {0x301A, 0x10FC, WORD_LEN, 0}, 	// RESET_REGISTER
    {0x326E, 0x00A4, WORD_LEN, 0}, 	// LOW_PASS_YUV_FILTER
    {0x35A4, 0x0596, WORD_LEN, 0}, 	// BRIGHT_COLOR_KILL_CONTROLS
    {0x35A2, 0x009B, WORD_LEN, 0}, 	// DARK_COLOR_KILL_CONTROLS
    {0xDC36, 0x33,   BYTE_LEN, 0}, 	// SYS_DARK_COLOR_KILL
    {0xDC33, 0x21,   BYTE_LEN, 0}, 	// SYS_FIRST_BLACK_LEVEL
    {0xDC35, 0x05,   BYTE_LEN, 0}, 	// SYS_UV_COLOR_BOOST
    {0xDC37, 0x62,   BYTE_LEN, 0}, 	// SYS_BRIGHT_COLORKILL
    
    // Gamma
    {0xBC18, 0x00,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_0
    {0xBC19, 0x11,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_1
    {0xBC1A, 0x23,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_2
    {0xBC1B, 0x3F,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_3
    {0xBC1C, 0x67,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_4
    {0xBC1D, 0x85,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_5
    {0xBC1E, 0x9B,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_6
    {0xBC1F, 0xAD,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_7
    {0xBC20, 0xBB,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_8
    {0xBC21, 0xC7,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_9
    {0xBC22, 0xD1,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_10
    {0xBC23, 0xDA,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_11
    {0xBC24, 0xE1,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_12
    {0xBC25, 0xE8,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_13
    {0xBC26, 0xEE,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_14
    {0xBC27, 0xF3,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_15
    {0xBC28, 0xF7,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_16
    {0xBC29, 0xFB,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_17
    {0xBC2A, 0xFF,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_18
    {0xBC2B, 0x00,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_0
    {0xBC2C, 0x11,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_1
    {0xBC2D, 0x23,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_2
    {0xBC2E, 0x3F,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_3
    {0xBC2F, 0x67,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_4
    {0xBC30, 0x85,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_5
    {0xBC31, 0x9B,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_6
    {0xBC32, 0xAD,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_7
    {0xBC33, 0xBB,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_8
    {0xBC34, 0xC7,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_9
    {0xBC35, 0xD1,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_10
    {0xBC36, 0xDA,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_11
    {0xBC37, 0xE1,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_12
    {0xBC38, 0xE8,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_13
    {0xBC39, 0xEE,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_14
    {0xBC3A, 0xF3,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_15
    {0xBC3B, 0xF7,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_16
    {0xBC3C, 0xFB,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_17
    {0xBC3D, 0xFF,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_18
    {0xBC3E, 0x00,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_0
    {0xBC3F, 0x18,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_1
    {0xBC40, 0x25,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_2
    {0xBC41, 0x3A,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_3
    {0xBC42, 0x59,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_4
    {0xBC43, 0x70,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_5
    {0xBC44, 0x81,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_6
    {0xBC45, 0x90,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_7
    {0xBC46, 0x9E,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_8
    {0xBC47, 0xAB,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_9
    {0xBC48, 0xB6,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_10
    {0xBC49, 0xC1,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_11
    {0xBC4A, 0xCB,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_12
    {0xBC4B, 0xD5,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_13
    {0xBC4C, 0xDE,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_14
    {0xBC4D, 0xE7,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_15
    {0xBC4E, 0xEF,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_16
    {0xBC4F, 0xF7,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_17
    {0xBC50, 0xFF,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_18
    {0xB801, 0xE0,   BYTE_LEN, 0}, 	// STAT_MODE
    {0xB862, 0x04,   BYTE_LEN, 0}, 	// STAT_BMTRACKING_SPEED

    {0xBC51, 0x01,   BYTE_LEN, 0}, 	// STAT_BMTRACKING_SPEED

    //AE//Frame rate control//Brightness Metric
    {0xB829, 0x02,   BYTE_LEN, 0}, 	// STAT_LL_BRIGHTNESS_METRIC_DIVISOR
    {0xB863, 0x02,   BYTE_LEN, 0}, 	// STAT_BM_MUL
    {0xA401, 0x00,   BYTE_LEN, 0}, 	// AE_RULE_MODE
    {0xA409, 0x3C,   BYTE_LEN, 0}, 	// AE_RULE_BASE_TARGET
    {0xA801, 0x01,   BYTE_LEN, 0}, 	// AE_TRACK_MODE
    {0xA818, 0x0578, WORD_LEN, 0}, 	// AE_TRACK_TARGET_INT_TIME_ROWS
    {0xA81A, 0x0834, WORD_LEN, 0}, 	// AE_TRACK_MAX_INT_TIME_ROWS
    {0xA402, 0x0004, WORD_LEN, 0}, 	// AE_RULE_ALGO
    {0xA802, 0x0007, WORD_LEN, 0}, 	// AE_TRACK_ALGO
    {0xA81C, 0x0028, WORD_LEN, 0}, 	// AE_TRACK_MIN_AGAIN
    {0xA81E, 0x00C0, WORD_LEN, 0}, 	// AE_TRACK_TARGET_AGAIN
    {0xA820, 0x0140, WORD_LEN, 0}, 	// AE_TRACK_MAX_AGAIN
    {0xA824, 0x0080, WORD_LEN, 0}, 	// AE_TRACK_MAX_DGAIN
    {0xA822, 0x0080, WORD_LEN, 0}, 	// AE_TRACK_MIN_DGAIN
    {0xBC52, 0x0160, WORD_LEN, 0}, 	// LL_START_BRIGHTNESS_METRIC
    {0xBC54, 0x0700, WORD_LEN, 0}, 	// LL_END_BRIGHTNESS_METRIC
    {0xBC58, 0x0160, WORD_LEN, 0}, 	// LL_START_GAIN_METRIC
    {0xBC5A, 0x0500, WORD_LEN, 0}, 	// LL_END_GAIN_METRIC
    {0xBC5E, 0x00FA, WORD_LEN, 0}, 	// LL_START_APERTURE_GAIN_BM
    {0xBC60, 0x028A, WORD_LEN, 0}, 	// LL_END_APERTURE_GAIN_BM
    {0xBC66, 0x00FA, WORD_LEN, 0}, 	// LL_START_APERTURE_GM
    {0xBC68, 0x028A, WORD_LEN, 0}, 	// LL_END_APERTURE_GM
    {0xBC86, 0x00C8, WORD_LEN, 0}, 	// LL_START_FFNR_GM
    {0xBC88, 0x028A, WORD_LEN, 0}, 	// LL_END_FFNR_GM
    {0xBCBC, 0x0040, WORD_LEN, 0}, 	// LL_SFFB_START_GAIN
    {0xBCBE, 0x01FC, WORD_LEN, 0}, 	// LL_SFFB_END_GAIN
    {0xBCCC, 0x00C8, WORD_LEN, 0}, 	// LL_SFFB_START_MAX_GM
    {0xBCCE, 0x0640, WORD_LEN, 0}, 	// LL_SFFB_END_MAX_GM
    {0xBC90, 0x00C8, WORD_LEN, 0}, 	// LL_START_GRB_GM
    {0xBC92, 0x028A, WORD_LEN, 0}, 	// LL_END_GRB_GM
    {0xBCAA, 0x044C, WORD_LEN, 0}, 	// LL_CDC_THR_ADJ_START_POS
    {0xBCAC, 0x00AF, WORD_LEN, 0}, 	// LL_CDC_THR_ADJ_MID_POS
    {0xBCAE, 0x0009, WORD_LEN, 0}, 	// LL_CDC_THR_ADJ_END_POS
    {0xBCD8, 0x00C8, WORD_LEN, 0}, 	// LL_PCR_START_BM
    {0xBCDA, 0x0A28, WORD_LEN, 0}, 	// LL_PCR_END_BM
    {0xBC02, 0x01F6, WORD_LEN, 0}, 	// LL_ALGO
    
    //Kernel
    {0x3380, 0x0504, WORD_LEN, 0}, 	// KERNEL_CONFIG
    {0x3380, 0x0584, WORD_LEN, 0}, 	// KERNEL_CONFIG
    {0x3380, 0x0586, WORD_LEN, 0}, 	// KERNEL_CONFIG
    {0x3380, 0x05CF, WORD_LEN, 0}, 	// KERNEL_CONFIG wanglei
    {0x33B0, 0x2A16, WORD_LEN, 0}, 	// FFNR_ALPHA_BETA
    {0xBCBA, 0x0009, WORD_LEN, 0}, 	// LL_SFFB_CONFIG
    {0xBCB2, 0x28,   BYTE_LEN, 0}, 	// LL_CDC_DARK_CLUS_SLOPE
    {0xBCB3, 0x5F,   BYTE_LEN, 0}, 	// LL_CDC_DARK_CLUS_SATUR
    {0xBC94, 0x12,   BYTE_LEN, 0}, 	// LL_GB_START_THRESHOLD_0
    {0xBC95, 0x0C,   BYTE_LEN, 0}, 	// LL_GB_START_THRESHOLD_1
    {0xBC9C, 0x37,   BYTE_LEN, 0}, 	// LL_GB_END_THRESHOLD_0
    {0xBC9D, 0x24,   BYTE_LEN, 0}, 	// LL_GB_END_THRESHOLD_1
    {0xBC8A, 0x00,   BYTE_LEN, 0}, 	// LL_START_FF_MIX_THRESH_Y
    {0xBC8B, 0x28,   BYTE_LEN, 0}, 	// LL_END_FF_MIX_THRESH_Y
    {0xBC8C, 0x01,   BYTE_LEN, 0}, 	// LL_START_FF_MIX_THRESH_YGAIN
    {0xBC8D, 0x01,   BYTE_LEN, 0}, 	// LL_END_FF_MIX_THRESH_YGAIN
    {0xBC8E, 0x0A,   BYTE_LEN, 0}, 	// LL_START_FF_MIX_THRESH_GAIN
    {0xBC8F, 0x00,   BYTE_LEN, 0}, 	// LL_END_FF_MIX_THRESH_GAIN
    {0xBCC0, 0x10,   BYTE_LEN, 0}, 	// LL_SFFB_RAMP_START
    {0xBCC1, 0x03,   BYTE_LEN, 0}, 	// LL_SFFB_RAMP_STOP
    {0xBCC2, 0x0A,   BYTE_LEN, 0}, 	// LL_SFFB_SLOPE_START
    {0xBCC3, 0x01,   BYTE_LEN, 0}, 	// LL_SFFB_SLOPE_STOP
    {0xBCC4, 0x0A,   BYTE_LEN, 0}, 	// LL_SFFB_THSTART
    {0xBCC5, 0x0E,   BYTE_LEN, 0}, 	// LL_SFFB_THSTOP
    
    //Section7 PA reference
    {0x33BA, 0x0088, WORD_LEN, 0}, 	// APEDGE_CONTROL
    {0x33BE, 0x0000, WORD_LEN, 0}, 	// UA_KNEE_L
    {0x33C2, 0x3100, WORD_LEN, 0}, 	// UA_WEIGHTS
    {0xBC5E, 0x0154, WORD_LEN, 0}, 	// LL_START_APERTURE_GAIN_BM
    {0xBC60, 0x0640, WORD_LEN, 0}, 	// LL_END_APERTURE_GAIN_BM
    {0xBCD0, 0x000A, WORD_LEN, 0}, 	// LL_SFFB_SOBEL_FLAT_START
    {0xBCD2, 0x00FE, WORD_LEN, 0}, 	// LL_SFFB_SOBEL_FLAT_STOP
    {0xBCD4, 0x001E, WORD_LEN, 0}, 	// LL_SFFB_SOBEL_SHARP_START
    {0xBCD6, 0x00FF, WORD_LEN, 0}, 	// LL_SFFB_SOBEL_SHARP_STOP
    {0xBC62, 0x07,   BYTE_LEN, 0}, 	// LL_START_APERTURE_KPGAIN
    {0xBC64, 0x05,   BYTE_LEN, 0}, 	// LL_START_APERTURE_KNGAIN
    {0xBC63, 0x50,   BYTE_LEN, 0}, 	// LL_END_APERTURE_KPGAIN
    {0xBC65, 0x50,   BYTE_LEN, 0}, 	// LL_END_APERTURE_KNGAIN
    {0xBC6A, 0x02,   BYTE_LEN, 0}, 	// LL_START_APERTURE_INTEGER_GAIN
    {0xBC6C, 0x01,   BYTE_LEN, 0}, 	// LL_START_APERTURE_EXP_GAIN
    //{0xBC56, 0xB0,   BYTE_LEN, 0}, 	// LL_START_CCM_SATURATION
    {0xBC56, 0xFF,   BYTE_LEN, 0}, 	// LL_START_CCM_SATURATION
    {0xBC57, 0x30,   BYTE_LEN, 0}, 	// LL_END_CCM_SATURATION
    {0xBCDE, 0x03,   BYTE_LEN, 0}, 	// LL_START_SYS_THRESHOLD
    {0xBCDF, 0x50,   BYTE_LEN, 0}, 	// LL_STOP_SYS_THRESHOLD
    {0xBCE0, 0x08,   BYTE_LEN, 0}, 	// LL_START_SYS_GAIN
    {0xBCE1, 0x03,   BYTE_LEN, 0}, 	// LL_STOP_SYS_GAIN
    {0xBCC6, 0x00,   BYTE_LEN, 0}, 	// LL_SFFB_SHARPENING_START
    {0xBCC7, 0x00,   BYTE_LEN, 0}, 	// LL_SFFB_SHARPENING_STOP
    {0xBCC8, 0x20,   BYTE_LEN, 0}, 	// LL_SFFB_FLATNESS_START
    {0xBCC9, 0x40,   BYTE_LEN, 0}, 	// LL_SFFB_FLATNESS_STOP
    {0xBCCA, 0x04,   BYTE_LEN, 0}, 	// LL_SFFB_TRANSITION_START
    {0xBCCB, 0x00,   BYTE_LEN, 0}, 	// LL_SFFB_TRANSITION_STOP

    //[Bright_DNP_Patch]
    //Enable the Patch
    //VAR8= 11, 0x0009, 0x01 	// AWB_MODE_EX
    {0xAC09, 0x01 	, BYTE_LEN, 0},// AWB_MODE_EX
    {0xAC22, 0x0005 , WORD_LEN, 0},// AWB_SHARPNESS_TH
    {0xACB0, 0x32 	, BYTE_LEN, 0},// AWB_RG_MIN
    {0xACB1, 0x4C 	, BYTE_LEN, 0},// AWB_RG_MAX
    {0xACB4, 0x28 	, BYTE_LEN, 0},// AWB_BG_MIN
    {0xACB5, 0x4F 	, BYTE_LEN, 0},// AWB_BG_MAX
    {0xACB2, 0x35 	, BYTE_LEN, 0},// AWB_RG_MIN_BRIGHT
    {0xACB3, 0x46 	, BYTE_LEN, 0},// AWB_RG_MAX_BRIGHT
    {0xACB6, 0x2F 	, BYTE_LEN, 0},// AWB_BG_MIN_BRIGHT
    {0xACB7, 0x4E 	, BYTE_LEN, 0},// AWB_BG_MAX_BRIGHT
    {0xACB8, 0x0056 , WORD_LEN, 0},// AWB_START_NUM_INT_LINES
    {0xACBA, 0x0014 , WORD_LEN, 0},// AWB_END_NUM_INT_LINES

    //Int Time Patch (SEMCO) variables
    //VAR8= 15, 0x00E9, 0x01 	// LL_MODE_EX
    //Enables
    {0x098E, 0xBCE9 , WORD_LEN, 0},	// Logical Access address
    {0xBCE9, 0x01 	, BYTE_LEN, 0},// LL_MODE_EX
    //Copy Again max set
    {0xBCEA, 0x0154 , WORD_LEN, 0},	// LL_MAX_AGAIN_USER_SET
    //Copy Dgain max set
    {0xBCEC, 0x00B0 , WORD_LEN, 0},	// LL_MAX_DGAIN_USER_SET
    //BM Threshold for patch to kick in
    {0xA838, 0x00B4 , WORD_LEN, 0},	// AE_TRACK_BM_THRESHOLD
    
    // setction6 PA calibration
    //JPEG Quailty
    //system setting
    {0x098E, 0xD80F	, WORD_LEN, 0},	//VAR8= 22, 0x000F, 0x04 	// JPEG_QSCALE_0
    {0xD80F, 0x04 	, BYTE_LEN, 0},// JPEG_QSCALE_0
    {0xD810, 0x08 	, BYTE_LEN, 0},// JPEG_QSCALE_1

    //VAR8= 18, 0x00D2, 0x04 	// CAM_OUTPUT_1_JPEG_QSCALE_0
    {0x098E, 0xC8D2  ,WORD_LEN, 0},	//VAR8= 18, 0x00D2, 0x04 	// JPEG_QSCALE_0
    {0xC8D2, 0x04 	 ,BYTE_LEN, 0},// CAM_OUTPUT_1_JPEG_QSCALE_0
    {0xC8D3, 0x08 	 ,BYTE_LEN, 0},// CAM_OUTPUT_1_JPEG_QSCALE_1
    {0xC8BC, 0x04 	 ,BYTE_LEN, 0},// CAM_OUTPUT_0_JPEG_QSCALE_0
    {0xC8BD, 0x08 	 ,BYTE_LEN, 0},// CAM_OUTPUT_0_JPEG_QSCALE_1

    //[Sys_Settings]
    {0x301A, 0x10F4, WORD_LEN, 0}, 	// RESET_REGISTER
    {0x301E, 0x0084, WORD_LEN, 0}, 	// DATA_PEDESTAL
    {0x301A, 0x10FC, WORD_LEN, 0}, 	// RESET_REGISTER
    //VAR8= 23, 0x0033, 0x21 	// SYS_FIRST_BLACK_LEVEL
    {0x098E, 0xDC33 ,WORD_LEN ,0 },		//VAR8= 23, DC = 23
    {0xDC33, 0x21	  ,BYTE_LEN ,0 }, //0x21 	// SYS_FIRST_BLACK_LEVEL
    {0xA80E, 0x12 	,BYTE_LEN ,0 },// AE_TRACK_MAX_BLACK_LEVEL
    {0xDC35, 0x05 	,BYTE_LEN ,0 },// SYS_UV_COLOR_BOOST
    {0xDC36, 0x33 	,BYTE_LEN ,0 },// SYS_DARK_COLOR_KILL
    {0xDC37, 0x62 	,BYTE_LEN ,0 },// SYS_BRIGHT_COLORKILL

    {0x326E, 0x00A4 ,WORD_LEN ,0 }, 	// LOW_PASS_YUV_FILTER
    {0x35A4, 0x0596 ,WORD_LEN ,0 }, 	// BRIGHT_COLOR_KILL_CONTROLS
    {0x35A2, 0x009B ,WORD_LEN ,0 }, 	// DARK_COLOR_KILL_CONTROLS

    //[BM_Dampening]
    //VAR8= 14, 0x0001, 0xE0 	// STAT_MODE
    {0x098E, 0xB801 ,	WORD_LEN, 0},//VAR8= 14, 0xB8 = 14
    {0xB801, 0xE0 	, BYTE_LEN, 0},// STAT_MODE
    {0xB862, 0x04 	, BYTE_LEN, 0},// STAT_BMTRACKING_SPEED
    
    //AE//Frame rate control//Brightness Metric
    //[AE]
    //frame rate control _ shutter speed control
    //VAR= 10, 0x0018, 838 //1755 	// AE_TRACK_TARGET_INT_TIME_ROWS
    {0x098E, 0xA816, WORD_LEN, 0},		// VAR=10, 0xA8 = 10
    {0xA816, 0x000A, WORD_LEN, 0}, 	// AE_TRACK_MIN_INT_TIME_ROWS
    {0xA818, 0x0200, WORD_LEN, 0}, 	// AE_TRACK_TARGET_INT_TIME_ROWS
    {0xA81A, 0x09E7, WORD_LEN, 0}, 	// AE_TRACK_MAX_INT_TIME_ROWS // Min frame control

    //AE mode control
    {0x098E, 0xA401 , WORD_LEN, 0},		// VAR= 9, 0xA4 = 9
    {0xA401, 0x00 	, BYTE_LEN, 0}, // AE_RULE_MODE
    {0xA402, 0x0004 , WORD_LEN, 0}, 	// AE_RULE_ALGO
    {0xA409, 0x3A	  , BYTE_LEN, 0},	// AE_RULE_BASE_TARGET
                  
    {0x098E, 0xA801 , WORD_LEN, 0},		// VAR=10, 0xA8 = 10
    {0xA801, 0x01 	, BYTE_LEN, 0}, // AE_TRACK_MODE	
    {0xA802, 0x0007 , WORD_LEN, 0}, 	// AE_TRACK_ALGO
    {0xA81C, 0x0028 , WORD_LEN, 0}, 	// AE_TRACK_MIN_AGAIN
    {0xA81E, 0x00DC , WORD_LEN, 0}, 	// AE_TRACK_TARGET_AGAIN
    {0xA820, 0x0154 , WORD_LEN, 0}, 	// AE_TRACK_MAX_AGAIN
    {0xA824, 0x00B0 , WORD_LEN, 0}, 	// AE_TRACK_MAX_DGAIN
    {0xA822, 0x0080 , WORD_LEN, 0}, 	// AE_TRACK_MIN_DGAIN

    //AE sub-window number
    //VAR8= 14, 0x0024, 0x05 	// STAT_AE_NUM_ZONES_X
    {0x098E, 0xB824 , WORD_LEN, 0},		// VAR=14, 0xB8 = 14
    {0xB824, 0x05 	, BYTE_LEN, 0}, // STAT_AE_NUM_ZONES_X
    {0xB825, 0x05 	, BYTE_LEN, 0}, // STAT_AE_NUM_ZONES_Y
                    
    //Basic
    //FIELD_WR= STAT_BM_MUL, 0x02 	// VAR8= 14, 0x0063, 0x02
    {0x098E, 0xB829 , WORD_LEN, 0},	// VAR=14, 0xB8 = 14
    {0xB829, 0x02 	, BYTE_LEN, 0}, // STAT_LL_BRIGHTNESS_METRIC_DIVISOR
    {0xB863, 0x02 	, BYTE_LEN, 0}, // STAT_BM_MUL

    //BNR Control
    {0x098E, 0xBC73 , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_END_BNR_STRENGTH]
    {0xBC72, 0x0F 	, BYTE_LEN, 0}, // LL_START_BNR_STRENGTH
    {0xBC73, 0x1A 	, BYTE_LEN, 0}, // LL_END_BNR_STRENGTH

    //[BM_GM_Start_Stop]
    //VAR= 15, 0x0002, 502 	// LL_ALGO
    {0x098E, 0xBC52, WORD_LEN, 0},		// VAR=15 , 0xBC = 15
    {0xBC52, 0x0160, WORD_LEN, 0}, 	// LL_START_BRIGHTNESS_METRIC
    {0xBC54, 0x0800, WORD_LEN, 0}, 	// LL_END_BRIGHTNESS_METRIC
    {0xBC58, 0x0160, WORD_LEN, 0}, 	// LL_START_GAIN_METRIC
    {0xBC5A, 0x0800, WORD_LEN, 0}, 	// LL_END_GAIN_METRIC
    {0xBC5E, 0x00FA, WORD_LEN, 0}, 	// LL_START_APERTURE_GAIN_BM
    {0xBC60, 0x088A, WORD_LEN, 0}, 	// LL_END_APERTURE_GAIN_BM
    {0xBC66, 0x00FA, WORD_LEN, 0}, 	// LL_START_APERTURE_GM
    {0xBC68, 0x038A, WORD_LEN, 0}, 	// LL_END_APERTURE_GM
    {0xBC86, 0x00C8, WORD_LEN, 0}, 	// LL_START_FFNR_GM
    {0xBC88, 0x038A, WORD_LEN, 0}, 	// LL_END_FFNR_GM
    {0xBCBC, 0x0040, WORD_LEN, 0}, 	// LL_SFFB_START_GAIN
    {0xBCBE, 0x014C, WORD_LEN, 0}, 	// LL_SFFB_END_GAIN
    {0xBCCC, 0x00C8, WORD_LEN, 0}, 	// LL_SFFB_START_MAX_GM
    {0xBCCE, 0x0340, WORD_LEN, 0}, 	// LL_SFFB_END_MAX_GM
    {0xBC90, 0x00C8, WORD_LEN, 0}, 	// LL_START_GRB_GM
    {0xBC92, 0x038A, WORD_LEN, 0}, 	// LL_END_GRB_GM
    {0xBCAA, 0x044C, WORD_LEN, 0}, 	// LL_CDC_THR_ADJ_START_POS
    {0xBCAC, 0x00AF, WORD_LEN, 0}, 	// LL_CDC_THR_ADJ_MID_POS
    {0xBCAE, 0x0009, WORD_LEN, 0}, 	// LL_CDC_THR_ADJ_END_POS
    {0xBCD8, 0x00DC, WORD_LEN, 0}, 	// LL_PCR_START_BM
    {0xBCDA, 0x0BDA, WORD_LEN, 0}, 	// LL_PCR_END_BM //0x0BDA : 50Lux
    {0xBC02, 0x01F6, WORD_LEN, 0}, 	// LL_ALGO
                   
    {0x098E, 0x3C14, WORD_LEN, 0}, 	// LOGICAL_ADDRESS_ACCESS [LL_GAMMA_FADE_TO_BLACK_START_POS]
    {0xBC14, 0xFFDC, WORD_LEN, 0}, 	// LL_GAMMA_FADE_TO_BLACK_START_POS
    {0xBC16, 0xFFFF, WORD_LEN, 0}, 	// LL_GAMMA_FADE_TO_BLACK_END_POS

    //this tuning is based on the noise model....only change if new noise model has been completed.
    //[Kernel]
    //turns off single pixel, enables 1.5 cluster
    {0x3380, 0x0504, WORD_LEN, 0}, 	// KERNEL_CONFIG
    //tuned dark cluster settings
    //FIELD_WR= LL_CDC_DARK_CLUS_SLOPE, 0x28 	// VAR8= 15, 0x00B2, 0x28
    {0x098E, 0xBCB2 , WORD_LEN, 0},		// VAR=15 , 0xBC = 15
    {0xBCB2, 0x28 	, BYTE_LEN, 0}, // LL_CDC_DARK_CLUS_SLOPE
    {0xBCB3, 0x5F 	, BYTE_LEN, 0}, // LL_CDC_DARK_CLUS_SATUR
    {0x3380, 0x0584 , WORD_LEN, 0}, 	// KERNEL_CONFIG
    {0x3380, 0x0586 , WORD_LEN, 0}, 	// KERNEL_CONFIG
    {0x3380, 0x0505 , WORD_LEN, 0}, 	// KERNEL_CONFIG

    //[GRB2.2]
    //FIELD_WR= LL_GB_START_THRESHOLD_0, 18 	// VAR8= 15, 0x0094, 0x06
    {0x098E, 0xBC94 , WORD_LEN ,0},	// VAR=15 , 0xBC = 15
    {0xBC94, 0x12 	, BYTE_LEN ,0}, // LL_GB_START_THRESHOLD_0
    {0xBC95, 0x0C 	, BYTE_LEN ,0}, // LL_GB_START_THRESHOLD_1
    {0xBC9C, 0x37 	, BYTE_LEN ,0}, // LL_GB_END_THRESHOLD_0
    {0xBC9D, 0x24 	, BYTE_LEN ,0}, // LL_GB_END_THRESHOLD_1

    //[Demosaic_REV2.1]
    {0x33B0, 0x2A16, WORD_LEN, 0}, 	// FFNR_ALPHA_BETA
    //VAR8=15, 0x008F, 0 	// LL_END_FF_MIX_THRESH_GAIN
    {0x098E, 0xBC8A , WORD_LEN, 0},	// VAR=15 , 0xBC = 15
    {0xBC8A, 0x00 	, BYTE_LEN, 0}, // LL_START_FF_MIX_THRESH_Y
    {0xBC8B, 0x28 	, BYTE_LEN, 0}, // LL_END_FF_MIX_THRESH_Y
    {0xBC8C, 0x01 	, BYTE_LEN, 0}, // LL_START_FF_MIX_THRESH_YGAIN
    {0xBC8D, 0x01 	, BYTE_LEN, 0}, // LL_END_FF_MIX_THRESH_YGAIN
    //REG= 0xBC8E, 0x18	 //0x0A 	// LL_START_FF_MIX_THRESH_GAIN
    //REG= 0xBCE2, 0x0A         //D         // LL_START_POS_KNEE
    //REG= 0xBCE4, 0x08          // LL_START_NEG_KNEE
    {0xBC8F, 0x00, BYTE_LEN, 0}, 	// LL_END_FF_MIX_THRESH_GAIN      

    //[SFFB_REV2.1_noisemodel]
    //FIELD_WR= LL_SFFB_RAMP_START, 0x10 	// VAR8= 15, 0x00C0, 0x1F
    {0xBCC0, 0x10 	, BYTE_LEN, 0}, // LL_SFFB_RAMP_START
    {0xBCC1, 0x03 	, BYTE_LEN, 0}, // LL_SFFB_RAMP_STOP
    {0xBCC2, 0x0A 	, BYTE_LEN, 0}, // LL_SFFB_SLOPE_START
    {0xBCC3, 0x01 	, BYTE_LEN, 0}, // LL_SFFB_SLOPE_STOP
    {0xBCC4, 0x0A 	, BYTE_LEN, 0}, // LL_SFFB_THSTART
    {0xBCC5, 0x0E 	, BYTE_LEN, 0}, // LL_SFFB_THSTOP
    {0xBCBA, 0x0009 , WORD_LEN, 0},	// LL_SFFB_CONFIG
    
    //[**********section7*************]
    //[Section7]//PA preference
    //[Aperture_Rev3.0]
    {0x33BA, 0x006A, WORD_LEN, 0}, 	// APEDGE_CONTROL
    {0x33BE, 0x0001, WORD_LEN, 0}, 	// UA_KNEE_L
    {0x33C2, 0x1000, WORD_LEN, 0}, 	// UA_WEIGHTS	//Black patch_Correction

    //FIELD_WR= LL_START_APERTURE_GAIN_BM, 0x0154
    // VAR= 15, 0x005E, 0x0154
    {0x098E, 0xBC5E	, WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS
    {0xBC5E, 0x0124 , WORD_LEN, 0},	// LL_START_APERTURE_GAIN_BM
    {0xBC60, 0x05D0 , WORD_LEN, 0},	// LL_END_APERTURE_GAIN_BM	 // 100Lux
    {0xBC62, 0x07 	, BYTE_LEN, 0},// LL_START_APERTURE_KPGAIN
    {0xBC64, 0x05 	, BYTE_LEN, 0},// LL_START_APERTURE_KNGAIN
    {0xBC63, 0x50 	, BYTE_LEN, 0},// LL_END_APERTURE_KPGAIN
    {0xBC65, 0x50 	, BYTE_LEN, 0},// LL_END_APERTURE_KNGAIN
    {0xBC6A, 0x02 	, BYTE_LEN, 0},// LL_START_APERTURE_INTEGER_GAIN
    {0xBC6C, 0x02 	, BYTE_LEN, 0},// LL_START_APERTURE_EXP_GAIN

    //[DCCM_REV3]
    {0x098E, 0xBCDE , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS
    {0xBCDE, 0x03 	, BYTE_LEN, 0}, // LL_START_SYS_THRESHOLD
    {0xBCDF, 0x50 	, BYTE_LEN, 0}, // LL_STOP_SYS_THRESHOLD
    {0xBCE0, 0x08 	, BYTE_LEN, 0}, // LL_START_SYS_GAIN
    {0xBCE1, 0x03 	, BYTE_LEN, 0}, // LL_STOP_SYS_GAIN

    //[Sobel_REV3]
    //FIELD_WR=LL_SFFB_SOBEL_FLAT_START, 10 	// VAR= 15, 0x00CC, 0x000B
    {0x098E, 0xBCD0 , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS
    {0xBCD0, 0x000A , WORD_LEN, 0},	// LL_SFFB_SOBEL_FLAT_START
    {0xBCD2, 0x00CC , WORD_LEN, 0},	// LL_SFFB_SOBEL_FLAT_STOP
    {0xBCD4, 0x0014 , WORD_LEN, 0},	// LL_SFFB_SOBEL_SHARP_START
    {0xBCD6, 0x00CD , WORD_LEN, 0},	// LL_SFFB_SOBEL_SHARP_STOP
    {0xBCC6, 0x00   , BYTE_LEN, 0}, // LL_SFFB_SHARPENING_START
    {0xBCC7, 0x00   , BYTE_LEN, 0}, // LL_SFFB_SHARPENING_STOP
    {0xBCC8, 0x20   , BYTE_LEN, 0}, //0x18 	// LL_SFFB_FLATNESS_START
    {0xBCC9, 0x40   , BYTE_LEN, 0}, //0x50 	// LL_SFFB_FLATNESS_STOP
    {0xBCCA, 0x04   , BYTE_LEN, 0}, // LL_SFFB_TRANSITION_START
    {0xBCCB, 0x00   , BYTE_LEN, 0}, // LL_SFFB_TRANSITION_STOP

    /* ZTE_JIA_CAM_20101124
     * setting for preview & snapshot orientation
     */
    {0x098E, 0xC8ED , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS
    {0xC8ED, 0x00   , BYTE_LEN, 0},	// LL_SFFB_SOBEL_FLAT_START
    
    //Section8 AF feature
    {0xC400, 0x88,   BYTE_LEN, 0}, 	// AFM_ALGO
    {0x8419, 0x05,   BYTE_LEN, 0}, 	// SEQ_STATE_CFG_1_AF
    {0xC400, 0x08,   BYTE_LEN, 0}, 	// AFM_ALGO
    /*
	 * ZTE_LJ_CAM_20101224
	 * update AF setting recommended by samsung for lab test
	 */
    {0xB018, 0x0A, BYTE_LEN, 0},      // AF_FS_POS_0
    {0xB019, 0x1B, BYTE_LEN, 0},      // AF_FS_POS_1
    {0xB01A, 0x2C, BYTE_LEN, 0},      // AF_FS_POS_2
    {0xB01B, 0x3D, BYTE_LEN, 0},      // AF_FS_POS_3
    {0xB01C, 0x4E, BYTE_LEN, 0},      // AF_FS_POS_4
    {0xB01D, 0x5F, BYTE_LEN, 0},      // AF_FS_POS_5
    {0xB01E, 0x70, BYTE_LEN, 0},      // AF_FS_POS_6
    {0xB01F, 0x81, BYTE_LEN, 0},      // AF_FS_POS_7
    {0xB020, 0x92, BYTE_LEN, 0},      // AF_FS_POS_8
    {0xB021, 0xA3, BYTE_LEN, 0},      // AF_FS_POS_9
    {0xB022, 0xB4, BYTE_LEN, 0},      // AF_FS_POS_10
    {0xB023, 0xC5, BYTE_LEN, 0},      // AF_FS_POS_11
    {0xB024, 0xD6, BYTE_LEN, 0},      // AF_FS_POS_12
    {0xB025, 0xF8, BYTE_LEN, 0},      // AF_FS_POS_13
    {0xB012, 0x0D, BYTE_LEN, 0}, 	// AF_FS_NUM_STEPS
    {0xB013, 0x66, BYTE_LEN, 0}, 	// AF_FS_NUM_STEPS2

    {0xB014, 0x03,   BYTE_LEN, 0}, 	// AF_FS_STEP_SIZE

    /*
	 * ZTE_LJ_CAM_20101224
	 * update AF setting recommended by  samsung for lab test
	 */
    {0xB010, 0x00, BYTE_LEN, 0}, 	// AF_FS_Sharpness_Variation_TH

    {0xB002, 0x0047, WORD_LEN, 0}, 	// AF_MODE
    {0xB004, 0x0002, WORD_LEN, 0}, 	// AF_ALGO

    /*
      * Macro Position Setting
      */
    /*
	 * ZTE_LJ_CAM_20101224
	 * update AF setting recommended by  samsung for lab test
      */
    {0xC40C, 0x00FF, WORD_LEN, 0},	// AFM_POS_MAX
    {0xC40A, 0x0010, WORD_LEN, 0},	// AFM_POS_MIN


    {0xB008, 0x0000, WORD_LEN, 0},
    {0xB00A, 0x003F, WORD_LEN, 0}, 	// AF_ZONE_WEIGHTS_HI
    {0xB00C, 0x0FC3, WORD_LEN, 0}, 	// AF_ZONE_WEIGHTS_LO
    {0xB00E, 0xF000, WORD_LEN, 0},
};

static struct mt9p111_i2c_reg_conf const noise_reduction_reg_settings_array[] = {
    /* 
     * add code here
     * e.g. {0xXXXX, 0xXXXX, WORD_LEN, 0},
     */
    {0x0000, 0x0000, WORD_LEN, 0},
};

static struct mt9p111_i2c_reg_conf const lens_roll_off_tbl[] = {
    /* 
     * add code here
     * e.g. {0xXXXX, 0xXXXX, WORD_LEN, 0},
     */
    {0x0000, 0x0000, WORD_LEN, 0},
};

//new settings from USA for version 3.0 by lijing
static struct mt9p111_i2c_reg_conf const pll_setup_tbl[] = {
    {0x0010, 0x0115, WORD_LEN, 0},
    {0x0012, 0x0060, WORD_LEN, 0},
    {0x0014, 0x2025, WORD_LEN, 0},
    {0x001E, 0x0575, WORD_LEN, 0}, 	// PAD_SLEW_PAD_CONFIG
    {0x0022, 0x0030, WORD_LEN, 0},         // VDD_DIS_COUNTER  

    /* ZTE_JIA_CAM_20100930
     * modify PLL setting to fix abnormal preview problem, from 0x7F78 to 0x7F7E
     */
    {0x002A, 0x7F7E, WORD_LEN, 0},

    {0x002C, 0x0000, WORD_LEN, 0},
    {0x002E, 0x0000, WORD_LEN, 0},
    {0x0018, 0x4008, WORD_LEN, 100},         // STANDBY_CONTROL_AND_STATUS
};

//new settings from USA for version 3.0
static struct mt9p111_i2c_reg_conf const sequencer_tbl[] = {
    /* ZTE_JIA_CAM_20101125
     * modify time delay from 50ms to 150ms recommended by Aptina
     */
    {0x8404, 0x05,   BYTE_LEN, 150}, 	// SEQ_CMD         

    {0xAC02, 0x00FF, WORD_LEN, 0}, 	    // AWB_ALGO      
    {0xAC02, 0x00FF, WORD_LEN, 0}, 	    // AWB_ALGO      

    /* ZTE_JIA_CAM_20101125
     * modify time delay from 50ms to 150ms recommended by Aptina
     */
    {0x8404, 0x06,   BYTE_LEN, 150},    // SEQ_CMD         

    {0x0018, 0x2008, WORD_LEN, 100}, 	// STANDBY_CONTROL_AND_STATUS
};

static struct mt9p111_i2c_reg_conf const wb_cloudy_tbl[] = {
    {0x098E, 0x8410, WORD_LEN, 0}, // LOGICAL_ADDRESS_ACCESS [SEQ_STATE_CFG_0_AWB]         					
    {0x8410, 0x01  , BYTE_LEN, 0}, // SEQ_STATE_CFG_0_AWB                                  					
    {0x8418, 0x01  , BYTE_LEN, 0}, // SEQ_STATE_CFG_1_AWB                                  					
    {0x8420, 0x01  , BYTE_LEN, 0}, // SEQ_STATE_CFG_2_AWB                                  					
    {0xAC44, 0x7F  , BYTE_LEN, 0}, // AWB_LEFT_CCM_POS_RANGE_LIMIT                         					
    {0xAC45, 0x7F  , BYTE_LEN, 0}, // AWB_RIGHT_CCM_POS_RANGE_LIMIT                        					
    {0xAC04, 0x2E  , BYTE_LEN, 0}, // AWB_PRE_AWB_R2G_RATIO
    {0xAC05, 0x4D  , BYTE_LEN, 0}, // AWB_PRE_AWB_B2G_RATIO  
    {0x098E, 0xAC08, WORD_LEN, 0}, // LOGICAL_ADDRESS_ACCESS [SEQ_STATE_CFG_0_AWB]         					
    {0xAC08, 0x7F  , BYTE_LEN, 50}, // SEQ_STATE_CFG_0_AWB  
    {0x8404, 0x06  , BYTE_LEN, 500}, // SEQ_CMD    
    {0xAC04, 0x2E  , BYTE_LEN, 0}, // AWB_PRE_AWB_R2G_RATIO
    {0xAC05, 0x4D  , BYTE_LEN, 0}, // AWB_PRE_AWB_B2G_RATIO     
};

static struct mt9p111_i2c_reg_conf const wb_daylight_tbl[] = {
    {0x098E, 0x8410, WORD_LEN, 0},     // LOGICAL_ADDRESS_ACCESS [SEQ_STATE_CFG_0_AWB]            					
    {0x8410, 0x01  , BYTE_LEN, 0},     // SEQ_STATE_CFG_0_AWB                                     					
    {0x8418, 0x01  , BYTE_LEN, 0},     // SEQ_STATE_CFG_1_AWB                                     					
    {0x8420, 0x01  , BYTE_LEN, 0},     // SEQ_STATE_CFG_2_AWB                                     					
    {0xAC44, 0x7F  , BYTE_LEN, 0},     // AWB_LEFT_CCM_POS_RANGE_LIMIT                            					
    {0xAC45, 0x7F  , BYTE_LEN, 0},     // AWB_RIGHT_CCM_POS_RANGE_LIMIT                           					
    {0xAC04, 0x39  , BYTE_LEN, 0},     // AWB_PRE_AWB_R2G_RATIO
    {0xAC05, 0x4D  , BYTE_LEN, 0},     // AWB_PRE_AWB_B2G_RATIO  
    {0x098E, 0xAC08, WORD_LEN, 0}, // LOGICAL_ADDRESS_ACCESS [SEQ_STATE_CFG_0_AWB]         					
    {0xAC08, 0x7F  , BYTE_LEN, 50}, // SEQ_STATE_CFG_0_AWB  
    
    {0x8404, 0x06  , BYTE_LEN, 500},   // SEQ_CMD                                                 					
    {0xAC04, 0x39  , BYTE_LEN, 0},     // AWB_PRE_AWB_R2G_RATIO
    {0xAC05, 0x4D  , BYTE_LEN, 0},     // AWB_PRE_AWB_B2G_RATIO     
};

static struct mt9p111_i2c_reg_conf const wb_flourescant_tbl[] = {
    {0x098E, 0x8410, WORD_LEN, 0},     // LOGICAL_ADDRESS_ACCESS [SEQ_STATE_CFG_0_AWB]            					
    {0x8410, 0x01  , BYTE_LEN, 0},     // SEQ_STATE_CFG_0_AWB                                     					
    {0x8418, 0x01  , BYTE_LEN, 0},     // SEQ_STATE_CFG_1_AWB                                     					
    {0x8420, 0x01  , BYTE_LEN, 0},     // SEQ_STATE_CFG_2_AWB                                     					
    {0xAC44, 0x7F  , BYTE_LEN, 0},     // AWB_LEFT_CCM_POS_RANGE_LIMIT                            					
    {0xAC45, 0x7F  , BYTE_LEN, 0},     // AWB_RIGHT_CCM_POS_RANGE_LIMIT                           					

    {0xAC04, 0x3E  , BYTE_LEN, 0},     // AWB_PRE_AWB_R2G_RATIO
    {0xAC05, 0x3C  , BYTE_LEN, 0},     // AWB_PRE_AWB_B2G_RATIO  
    {0x098E, 0xAC08, WORD_LEN, 0}, // LOGICAL_ADDRESS_ACCESS [SEQ_STATE_CFG_0_AWB]         					
    {0xAC08, 0x7F  , BYTE_LEN, 50}, // SEQ_STATE_CFG_0_AWB  
    
    {0x8404, 0x06  , BYTE_LEN, 500},   // SEQ_CMD             
    {0xAC04, 0x3E  , BYTE_LEN, 0},     // AWB_PRE_AWB_R2G_RATIO
    {0xAC05, 0x3C  , BYTE_LEN, 0},     // AWB_PRE_AWB_B2G_RATIO     
};

static struct mt9p111_i2c_reg_conf const wb_incandescent_tbl[] = {
    {0x098E, 0x8410, WORD_LEN, 0}, // LOGICAL_ADDRESS_ACCESS [SEQ_STATE_CFG_0_AWB]         					
    {0x8410, 0x01  , BYTE_LEN, 0}, // SEQ_STATE_CFG_0_AWB                                  					
    {0x8418, 0x01  , BYTE_LEN, 0}, // SEQ_STATE_CFG_1_AWB                                  					
    {0x8420, 0x01  , BYTE_LEN, 0}, // SEQ_STATE_CFG_2_AWB                                  					
    {0xAC44, 0x7F  , BYTE_LEN, 0}, // AWB_LEFT_CCM_POS_RANGE_LIMIT                         					
    {0xAC45, 0x7F  , BYTE_LEN, 0}, // AWB_RIGHT_CCM_POS_RANGE_LIMIT                        					
    
    {0xAC04, 0x50  , BYTE_LEN, 0}, // AWB_PRE_AWB_R2G_RATIO
    {0xAC05, 0x2C  , BYTE_LEN, 0}, // AWB_PRE_AWB_B2G_RATIO  
    {0x098E, 0xAC08, WORD_LEN, 0}, // LOGICAL_ADDRESS_ACCESS [SEQ_STATE_CFG_0_AWB]         					
    {0xAC08, 0x7F  , BYTE_LEN, 50}, // SEQ_STATE_CFG_0_AWB  
    
    {0x8404, 0x06  , BYTE_LEN, 500}, // SEQ_CMD                                              					
    {0xAC04, 0x50  , BYTE_LEN, 0}, // AWB_PRE_AWB_R2G_RATIO
    {0xAC05, 0x2C  , BYTE_LEN, 0}, // AWB_PRE_AWB_B2G_RATIO  
};

static struct mt9p111_i2c_reg_conf const wb_auto_tbl[] = {
    {0x098E, 0x8410 , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [SEQ_STATE_CFG_0_AWB] 
    {0x8410, 0x02 	, BYTE_LEN, 0},// SEQ_STATE_CFG_0_AWB                           
    {0x8418, 0x02 	, BYTE_LEN, 0},// SEQ_STATE_CFG_1_AWB                           
    {0x8420, 0x02 	, BYTE_LEN, 0},// SEQ_STATE_CFG_2_AWB                           
    {0xAC44, 0x00 	, BYTE_LEN, 0},// AWB_LEFT_CCM_POS_RANGE_LIMIT                  
    {0xAC45, 0x7F 	, BYTE_LEN, 50},// AWB_RIGHT_CCM_POS_RANGE_LIMIT                 
    {0x8404, 0x06 	, BYTE_LEN, 50},// SEQ_CMD                                       
};

/* Contrast Setup */
static struct mt9p111_i2c_reg_conf const contrast_tbl_0[] = {
    {0x098E, 0xBC51 , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_GAMMA_CURVE_SELECTOR]  
    {0xBC51, 0x04 	, BYTE_LEN, 0},// LL_GAMMA_CURVE_SELECTOR                            
    {0xBC2B, 0x00 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_0                           
    {0xBC2C, 0x32 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_1                           
    {0xBC2D, 0x3E 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_2                           
    {0xBC2E, 0x4F 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_3                           
    {0xBC2F, 0x67 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_4                           
    {0xBC30, 0x78 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_5                           
    {0xBC31, 0x87 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_6                           
    {0xBC32, 0x93 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_7                           
    {0xBC33, 0x9F 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_8                           
    {0xBC34, 0xA9 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_9                           
    {0xBC35, 0xB3 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_10                          
    {0xBC36, 0xBC 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_11                          
    {0xBC37, 0xC4 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_12                          
    {0xBC38, 0xCC 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_13                          
    {0xBC39, 0xD4 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_14                          
    {0xBC3A, 0xDC 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_15                          
    {0xBC3B, 0xEA 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_16                          
    {0xBC3C, 0xF8 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_17                          
    {0xBC3D, 0xFF 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_18                          
    {0x8404, 0x05 	, BYTE_LEN, 0},// SEQ_CMD
};

static struct mt9p111_i2c_reg_conf const contrast_tbl_1[] = {
    {0x098E, 0xBC51 , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_GAMMA_CURVE_SELECTOR]  
    {0xBC51, 0x04 	, BYTE_LEN, 0},// LL_GAMMA_CURVE_SELECTOR                            
    {0xBC2B, 0x00 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_0                           
    {0xBC2C, 0x26 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_1                           
    {0xBC2D, 0x34 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_2                           
    {0xBC2E, 0x48 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_3                           
    {0xBC2F, 0x63 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_4                           
    {0xBC30, 0x77 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_5                           
    {0xBC31, 0x88 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_6                           
    {0xBC32, 0x96 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_7                           
    {0xBC33, 0xA3 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_8                           
    {0xBC34, 0xAF 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_9                           
    {0xBC35, 0xBA 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_10                          
    {0xBC36, 0xC4 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_11                          
    {0xBC37, 0xCE 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_12                          
    {0xBC38, 0xD7 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_13                          
    {0xBC39, 0xE0 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_14                          
    {0xBC3A, 0xE8 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_15                          
    {0xBC3B, 0xF1 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_16                          
    {0xBC3C, 0xF9 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_17                          
    {0xBC3D, 0xFF 	, BYTE_LEN, 0},// LL_GAMMA_NEUTRAL_CURVE_18                          
    {0x8404, 0x05 	, BYTE_LEN, 0},// SEQ_CMD    
};

static struct mt9p111_i2c_reg_conf const contrast_tbl_2[] = {
    {0xBC51, 0x01,   BYTE_LEN, 0}, 	// STAT_BMTRACKING_SPEED
    {0xBC18, 0x00,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_0
    {0xBC19, 0x11,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_1
    {0xBC1A, 0x23,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_2
    {0xBC1B, 0x3F,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_3
    {0xBC1C, 0x67,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_4
    {0xBC1D, 0x85,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_5
    {0xBC1E, 0x9B,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_6
    {0xBC1F, 0xAD,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_7
    {0xBC20, 0xBB,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_8
    {0xBC21, 0xC7,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_9
    {0xBC22, 0xD1,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_10
    {0xBC23, 0xDA,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_11
    {0xBC24, 0xE1,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_12
    {0xBC25, 0xE8,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_13
    {0xBC26, 0xEE,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_14
    {0xBC27, 0xF3,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_15
    {0xBC28, 0xF7,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_16
    {0xBC29, 0xFB,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_17
    {0xBC2A, 0xFF,   BYTE_LEN, 0}, 	// LL_GAMMA_CONTRAST_CURVE_18
    {0xBC2B, 0x00,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_0
    {0xBC2C, 0x11,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_1
    {0xBC2D, 0x23,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_2
    {0xBC2E, 0x3F,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_3
    {0xBC2F, 0x67,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_4
    {0xBC30, 0x85,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_5
    {0xBC31, 0x9B,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_6
    {0xBC32, 0xAD,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_7
    {0xBC33, 0xBB,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_8
    {0xBC34, 0xC7,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_9
    {0xBC35, 0xD1,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_10
    {0xBC36, 0xDA,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_11
    {0xBC37, 0xE1,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_12
    {0xBC38, 0xE8,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_13
    {0xBC39, 0xEE,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_14
    {0xBC3A, 0xF3,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_15
    {0xBC3B, 0xF7,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_16
    {0xBC3C, 0xFB,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_17
    {0xBC3D, 0xFF,   BYTE_LEN, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_18
    {0xBC3E, 0x00,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_0
    {0xBC3F, 0x18,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_1
    {0xBC40, 0x25,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_2
    {0xBC41, 0x3A,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_3
    {0xBC42, 0x59,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_4
    {0xBC43, 0x70,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_5
    {0xBC44, 0x81,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_6
    {0xBC45, 0x90,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_7
    {0xBC46, 0x9E,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_8
    {0xBC47, 0xAB,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_9
    {0xBC48, 0xB6,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_10
    {0xBC49, 0xC1,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_11
    {0xBC4A, 0xCB,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_12
    {0xBC4B, 0xD5,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_13
    {0xBC4C, 0xDE,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_14
    {0xBC4D, 0xE7,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_15
    {0xBC4E, 0xEF,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_16
    {0xBC4F, 0xF7,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_17
    {0xBC50, 0xFF,   BYTE_LEN, 0}, 	// LL_GAMMA_NR_CURVE_18

    {0x8404, 0x06 	, BYTE_LEN, 0},// SEQ_CMD 
};

static struct mt9p111_i2c_reg_conf const contrast_tbl_3[] = {
    {0x098E, 0xBC51,WORD_LEN,0}, 	// LOGICAL_ADDRESS_ACCESS [LL_GAMMA_CURVE_SELECTOR]
    {0xBC51, 0x02,BYTE_LEN,0}, 	// LL_GAMMA_CURVE_SELECTOR    
    {0xBC18, 0x00,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_0
    {0xBC19, 0x08,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_1
    {0xBC1A, 0x17,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_2
    {0xBC1B, 0x2F,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_3
    {0xBC1C, 0x50,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_4
    {0xBC1D, 0x6D,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_5
    {0xBC1E, 0x88,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_6
    {0xBC1F, 0x9E,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_7
    {0xBC20, 0xAF,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_8
    {0xBC21, 0xBD,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_9
    {0xBC22, 0xC9,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_10
    {0xBC23, 0xD3,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_11
    {0xBC24, 0xDB,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_12
    {0xBC25, 0xE3,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_13
    {0xBC26, 0xEA,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_14
    {0xBC27, 0xF0,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_15
    {0xBC28, 0xF5,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_16
    {0xBC29, 0xFA,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_17
    {0xBC2A, 0xFF,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_18
    {0x8404, 0x05,BYTE_LEN,0}, 	// SEQ_CMD
};

static struct mt9p111_i2c_reg_conf const contrast_tbl_4[] = {
    {0x098E, 0xBC51,WORD_LEN,0}, 	// LOGICAL_ADDRESS_ACCESS [LL_GAMMA_CURVE_SELECTOR]
    {0xBC51, 0x02,BYTE_LEN,0}, 	// LL_GAMMA_CURVE_SELECTOR
    {0xBC18, 0x00,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_0
    {0xBC19, 0x04,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_1
    {0xBC1A, 0x0C,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_2
    {0xBC1B, 0x1A,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_3
    {0xBC1C, 0x32,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_4
    {0xBC1D, 0x4C,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_5
    {0xBC1E, 0x68,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_6
    {0xBC1F, 0x87,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_7
    {0xBC20, 0xA1,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_8
    {0xBC21, 0xB5,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_9
    {0xBC22, 0xC4,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_10
    {0xBC23, 0xD1,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_11
    {0xBC24, 0xDB,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_12
    {0xBC25, 0xE3,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_13
    {0xBC26, 0xEA,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_14
    {0xBC27, 0xF0,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_15
    {0xBC28, 0xF6,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_16
    {0xBC29, 0xFA,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_17
    {0xBC2A, 0xFF,BYTE_LEN,0}, 	// LL_GAMMA_CONTRAST_CURVE_18
    {0x8404, 0x05,BYTE_LEN,0}, 	// SEQ_CMD
};

static struct mt9p111_i2c_reg_conf const *contrast_tbl[] = {
    contrast_tbl_0,
    contrast_tbl_1,
    contrast_tbl_2,
    contrast_tbl_3,
    contrast_tbl_4,
};

static uint16_t const contrast_tbl_sz[] = {
    ARRAY_SIZE(contrast_tbl_0),
    ARRAY_SIZE(contrast_tbl_1),
    ARRAY_SIZE(contrast_tbl_2),
    ARRAY_SIZE(contrast_tbl_3),
    ARRAY_SIZE(contrast_tbl_4),
};

static struct mt9p111_i2c_reg_conf const brightness_tbl_0[] = {
    {0x337E, 0xA000, WORD_LEN, 0},  // Y_RGB_OFFSET
};

static struct mt9p111_i2c_reg_conf const brightness_tbl_1[] = {
    {0x337E, 0xC000, WORD_LEN, 0},  // Y_RGB_OFFSET
};

static struct mt9p111_i2c_reg_conf const brightness_tbl_2[] = {
    {0x337E, 0xE000, WORD_LEN, 0},  // Y_RGB_OFFSET
};

static struct mt9p111_i2c_reg_conf const brightness_tbl_3[] = {
    {0x337E, 0x0000 , WORD_LEN, 0}, // Y_RGB_OFFSET  
};

static struct mt9p111_i2c_reg_conf const brightness_tbl_4[] = {
    {0x337E, 0x1800, WORD_LEN, 0},  // Y_RGB_OFFSET
};

static struct mt9p111_i2c_reg_conf const brightness_tbl_5[] = {
    {0x337E, 0x3000, WORD_LEN, 0},  // Y_RGB_OFFSET
};

static struct mt9p111_i2c_reg_conf const brightness_tbl_6[] = {
    {0x337E, 0x5000, WORD_LEN, 0},  // Y_RGB_OFFSET
};

static struct mt9p111_i2c_reg_conf const *brightness_tbl[] = {
    brightness_tbl_0,
    brightness_tbl_1,
    brightness_tbl_2,
    brightness_tbl_3,
    brightness_tbl_4,
    brightness_tbl_5,
    brightness_tbl_6,
};

static uint16_t const brightness_tbl_sz[] = {
    ARRAY_SIZE(brightness_tbl_0),
    ARRAY_SIZE(brightness_tbl_1),
    ARRAY_SIZE(brightness_tbl_2),
    ARRAY_SIZE(brightness_tbl_3),
    ARRAY_SIZE(brightness_tbl_4),
    ARRAY_SIZE(brightness_tbl_5),
    ARRAY_SIZE(brightness_tbl_6),
};

static struct mt9p111_i2c_reg_conf const saturation_tbl_0[] = {
    {0xBC56, 0x00, BYTE_LEN, 0}, // LL_START_CCM_SATURATION
    {0xBC57, 0x00, BYTE_LEN, 0}, // LL_END_CCM_SATURATION  
    {0x098E, 0xDC35, WORD_LEN, 0}, // LOGICAL_ADDRESS_ACCESS [SYS_UV_COLOR_BOOST]      
    {0xDC35, 0x04, BYTE_LEN, 0}, // SYS_UV_COLOR_BOOST  
    {0x8404, 0x06, BYTE_LEN, 10}, // SEQ_CMD 
};

static struct mt9p111_i2c_reg_conf const saturation_tbl_1[] = {
    {0xBC56, 0x40, BYTE_LEN, 0}, // LL_START_CCM_SATURATION
    {0xBC57, 0x10, BYTE_LEN, 0}, // LL_END_CCM_SATURATION  
    {0x098E, 0xDC35, WORD_LEN, 0}, // LOGICAL_ADDRESS_ACCESS [SYS_UV_COLOR_BOOST]       
    {0xDC35, 0x04, BYTE_LEN, 0}, // SYS_UV_COLOR_BOOST                                 
    {0x8404, 0x06, BYTE_LEN, 10}, // SEQ_CMD 
	
};

static struct mt9p111_i2c_reg_conf const saturation_tbl_2[] = {
    {0xBC56, 0xA5, BYTE_LEN, 0}, // LL_START_CCM_SATURATION
    {0xBC57, 0x30, BYTE_LEN, 0}, // LL_END_CCM_SATURATION  
    {0x098E, 0xDC35, WORD_LEN, 0}, // LOGICAL_ADDRESS_ACCESS [SYS_UV_COLOR_BOOST]       
    {0xDC35, 0x05, BYTE_LEN, 0}, // SYS_UV_COLOR_BOOST                                 
    {0x8404, 0x06, BYTE_LEN, 10}, // SEQ_CMD 
};

static struct mt9p111_i2c_reg_conf const saturation_tbl_3[] = {
    {0xBC56, 0xe4,   BYTE_LEN, 0}, 	// LL_START_CCM_SATURATION
    {0xBC57, 0x60,   BYTE_LEN, 0}, 	// LL_END_CCM_SATURATION    
    {0x098E, 0xDC35, WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [SYS_UV_COLOR_BOOST]
    {0xDC35, 0x05, BYTE_LEN, 0}, // SYS_UV_COLOR_BOOST
    {0x8404, 0x06, BYTE_LEN, 10}, // SEQ_CMD
};

static struct mt9p111_i2c_reg_conf const saturation_tbl_4[] = {
    {0xBC56, 0xC2,   BYTE_LEN, 0}, 	// LL_START_CCM_SATURATION
    {0xBC57, 0x80,   BYTE_LEN, 0}, 	// LL_END_CCM_SATURATION 
    {0x098E, 0xDC35, WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [SYS_UV_COLOR_BOOST]
    {0xDC35, 0x06, BYTE_LEN, 0}, // SYS_UV_COLOR_BOOST
    {0x8404, 0x06, BYTE_LEN, 10}, // SEQ_CMD
};

static struct mt9p111_i2c_reg_conf const *saturation_tbl[] = {
    saturation_tbl_0,
    saturation_tbl_1,
    saturation_tbl_2,
    saturation_tbl_3,
    saturation_tbl_4,
};

static uint16_t const saturation_tbl_sz[] = {
    ARRAY_SIZE(saturation_tbl_0),
    ARRAY_SIZE(saturation_tbl_1),
    ARRAY_SIZE(saturation_tbl_2),
    ARRAY_SIZE(saturation_tbl_3),
    ARRAY_SIZE(saturation_tbl_4),
};

/*
 * ZTE_LJ_CAM_20101224
 * update sharpness setting for lab test recommended by aptina
 */
static struct mt9p111_i2c_reg_conf const sharpness_tbl_0[] = {
    {0x098E, 0xBC6A , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_INTEGER_GAIN]   
    {0xBC6A, 0x0004 , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_INTEGER_GAIN]
    {0xBC6B, 0x0000 , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_INTEGER_GAIN]
    {0x33C2, 0x0000 , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_INTEGER_GAIN]   
    {0x33BA, 0x006A , WORD_LEN, 0}, // LL_START_APERTURE_INTEGER_GAIN                            
    {0x8404, 0x06   , BYTE_LEN, 10}, // SEQ_CMD                                                   
};

static struct mt9p111_i2c_reg_conf const sharpness_tbl_1[] = {
    {0x098E, 0xBC6A , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_INTEGER_GAIN]   
    {0xBC6A, 0x0004 , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_INTEGER_GAIN]
    {0xBC6B, 0x0000 , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_INTEGER_GAIN]
    {0x33C2, 0x1200 , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_INTEGER_GAIN]   
    {0x33BA, 0x006A , WORD_LEN, 0}, // LL_START_APERTURE_INTEGER_GAIN                            
    {0x8404, 0x06   , BYTE_LEN, 10}, // SEQ_CMD                                                   
};

static struct mt9p111_i2c_reg_conf const sharpness_tbl_2[] = {
    {0x098E, 0xBC6A , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_INTEGER_GAIN]  
    {0xBC6A, 0x0006 , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_INTEGER_GAIN]
    {0xBC6B, 0x0005 , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_INTEGER_GAIN]
    {0x33C2, 0x6600 , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_INTEGER_GAIN]   
    {0x33BA, 0x006B , WORD_LEN, 0}, // LL_START_APERTURE_INTEGER_GAIN                            
    {0x8404, 0x06,    BYTE_LEN, 10}, // SEQ_CMD
};

static struct mt9p111_i2c_reg_conf const sharpness_tbl_3[] = {
    {0x098E, 0xBC6A , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_INTEGER_GAIN]   
    {0xBC6A, 0x0007 , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_INTEGER_GAIN]
    {0xBC6B, 0x0005 , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_INTEGER_GAIN]
    {0x33C2, 0xBB00 , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_INTEGER_GAIN]   
    {0x33BA, 0x006D , WORD_LEN, 0}, // LL_START_APERTURE_INTEGER_GAIN                            
    {0x8404, 0x06   , BYTE_LEN, 10}, // SEQ_CMD                                                       
};

static struct mt9p111_i2c_reg_conf const sharpness_tbl_4[] = {
    {0x098E, 0xBC6A , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_INTEGER_GAIN]
    {0xBC6A, 0x0007 , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_INTEGER_GAIN]
    {0xBC6B, 0x0005 , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_INTEGER_GAIN]
    {0x33C2, 0xFF00 , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_INTEGER_GAIN]
    {0x33BA, 0x006F , WORD_LEN, 0}, // LL_START_APERTURE_INTEGER_GAIN
    {0x8404, 0x06 	,  BYTE_LEN, 10}, // SEQ_CMD   
};

static struct mt9p111_i2c_reg_conf const *sharpness_tbl[] = {
    sharpness_tbl_0,
    sharpness_tbl_1,
    sharpness_tbl_2,
    sharpness_tbl_3,
    sharpness_tbl_4,
};

static uint16_t const sharpness_tbl_sz[] = {
    ARRAY_SIZE(sharpness_tbl_0),
    ARRAY_SIZE(sharpness_tbl_1),
    ARRAY_SIZE(sharpness_tbl_2),
    ARRAY_SIZE(sharpness_tbl_3),
    ARRAY_SIZE(sharpness_tbl_4),
};

static struct mt9p111_i2c_reg_conf const exposure_tbl_0[] = {
    {0xA805, 0x04, BYTE_LEN, 0},    // RESERVED_AE_TRACK_05
    {0xA409, 0x10, BYTE_LEN, 0},    // AE_RULE_BASE_TARGET
};

static struct mt9p111_i2c_reg_conf const exposure_tbl_1[] = {
    {0xA805, 0x04, BYTE_LEN, 0},    // RESERVED_AE_TRACK_05
    {0xA409, 0x22, BYTE_LEN, 0},    // AE_RULE_BASE_TARGET
};

static struct mt9p111_i2c_reg_conf const exposure_tbl_2[] = {
    {0xA805, 0x04, BYTE_LEN, 0},    // RESERVED_AE_TRACK_05
    {0xA409, 0x3A, BYTE_LEN, 0},    // AE_RULE_BASE_TARGET
};

static struct mt9p111_i2c_reg_conf const exposure_tbl_3[] = {
    {0xA805, 0x04, BYTE_LEN, 0},    // RESERVED_AE_TRACK_05
    {0xA409, 0x50, BYTE_LEN, 0},    // AE_RULE_BASE_TARGET
};

static struct mt9p111_i2c_reg_conf const exposure_tbl_4[] = {
    {0xA805, 0x04, BYTE_LEN, 0},    // RESERVED_AE_TRACK_05
    {0xA409, 0x70, BYTE_LEN, 0},    // AE_RULE_BASE_TARGET
};

static struct mt9p111_i2c_reg_conf const *exposure_tbl[] = {
    exposure_tbl_0,
    exposure_tbl_1,
    exposure_tbl_2,
    exposure_tbl_3,
    exposure_tbl_4,
};

static uint16_t const exposure_tbl_sz[] = {
    ARRAY_SIZE(exposure_tbl_0),
    ARRAY_SIZE(exposure_tbl_1),
    ARRAY_SIZE(exposure_tbl_2),
    ARRAY_SIZE(exposure_tbl_3),
    ARRAY_SIZE(exposure_tbl_4),
};

/* ZTE_LJ_CAM_20101203
 * Mapping between addr of EEPROM and addr of sensor
 */
struct mt9p111_eeprom_lsc_reg_conf lsc_reg_addr_tbl[] = {
    {0x00, 0x3640},
    {0x02, 0x3642},
    {0x04, 0x3644},
    {0x06, 0x3646},
    {0x08, 0x3648},
    {0x0A, 0x364A},
    {0x0C, 0x364C},
    {0x0E, 0x364E},
    {0x10, 0x3650},
    {0x12, 0x3652},
    {0x14, 0x3654},
    {0x16, 0x3656},
    {0x18, 0x3658},
    {0x1A, 0x365A},
    {0x1C, 0x365C},
    {0x1E, 0x365E},
    {0x20, 0x3660},
    {0x22, 0x3662},
    {0x24, 0x3664},
    {0x26, 0x3666},
    {0x28, 0x3680},
    {0x2A, 0x3682},
    {0x2C, 0x3684},
    {0x2E, 0x3686},
    {0x30, 0x3688},
    {0x32, 0x368A},
    {0x34, 0x368C},
    {0x36, 0x368E},
    {0x38, 0x3690},
    {0x3A, 0x3692},
    {0x3C, 0x3694},
    {0x3E, 0x3696},
    {0x40, 0x3698},
    {0x42, 0x369A},
    {0x44, 0x369C},
    {0x46, 0x369E},
    {0x48, 0x36A0},
    {0x4A, 0x36A2},
    {0x4C, 0x36A4},
    {0x4E, 0x36A6},
    {0x50, 0x36C0},
    {0x52, 0x36C2},
    {0x54, 0x36C4},
    {0x56, 0x36C6},
    {0x58, 0x36C8},
    {0x5A, 0x36CA},
    {0x5C, 0x36CC},
    {0x5E, 0x36CE},
    {0x60, 0x36D0},
    {0x62, 0x36D2},
    {0x64, 0x36D4},
    {0x66, 0x36D6},
    {0x68, 0x36D8},
    {0x6A, 0x36DA},
    {0x6C, 0x36DC},
    {0x6E, 0x36DE},
    {0x70, 0x36E0},
    {0x72, 0x36E2},
    {0x74, 0x36E4},
    {0x76, 0x36E6},
    {0x78, 0x3700},
    {0x7A, 0x3702},
    {0x7C, 0x3704},
    {0x7E, 0x3706},
    {0x80, 0x3708},
    {0x82, 0x370A},
    {0x84, 0x370C},
    {0x86, 0x370E},
    {0x88, 0x3710},
    {0x8A, 0x3712},
    {0x8C, 0x3714},
    {0x8E, 0x3716},
    {0x90, 0x3718},
    {0x92, 0x371A},
    {0x94, 0x371C},
    {0x96, 0x371E},
    {0x98, 0x3720},
    {0x9A, 0x3722},
    {0x9C, 0x3724},
    {0x9E, 0x3726},
    {0xA0, 0x3740},
    {0xA2, 0x3742},
    {0xA4, 0x3744},
    {0xA6, 0x3746},
    {0xA8, 0x3748},
    {0xAA, 0x374A},
    {0xAC, 0x374C},
    {0xAE, 0x374E},
    {0xB0, 0x3750},
    {0xB2, 0x3752},
    {0xB4, 0x3754},
    {0xB6, 0x3756},
    {0xB8, 0x3758},
    {0xBA, 0x375A},
    {0xBC, 0x375C},
    {0xBE, 0x375E},
    {0xC0, 0x3760},
    {0xC2, 0x3762},
    {0xC4, 0x3764},
    {0xC6, 0x3766},
    {0xC8, 0x3782},
    {0xCA, 0x3784},
};

struct mt9p111_reg_t mt9p111_regs = {
    .prev_snap_reg_settings =               preview_snapshot_mode_reg_settings_array,
    .prev_snap_reg_settings_size =          ARRAY_SIZE(preview_snapshot_mode_reg_settings_array),

    .noise_reduction_reg_settings =         noise_reduction_reg_settings_array,
    .noise_reduction_reg_settings_size =    0,  /* ARRAY_SIZE(noise_reduction_reg_settings_array), */

    .plltbl =                               pll_setup_tbl,
    .plltbl_size =                          ARRAY_SIZE(pll_setup_tbl),

    .stbl =                                 sequencer_tbl,
    .stbl_size =                            ARRAY_SIZE(sequencer_tbl),

    .rftbl =                                lens_roll_off_tbl,
    .rftbl_size =                           0,  /* ARRAY_SIZE(lens_roll_off_tbl), */

    .wb_cloudy_tbl                          = wb_cloudy_tbl,
    .wb_cloudy_tbl_sz                       = ARRAY_SIZE(wb_cloudy_tbl),

    .wb_daylight_tbl                        = wb_daylight_tbl,
    .wb_daylight_tbl_sz                     = ARRAY_SIZE(wb_daylight_tbl),

    .wb_flourescant_tbl                     = wb_flourescant_tbl,
    .wb_flourescant_tbl_sz                  = ARRAY_SIZE(wb_flourescant_tbl),

    .wb_incandescent_tbl                    = wb_incandescent_tbl,
    .wb_incandescent_tbl_sz                 = ARRAY_SIZE(wb_incandescent_tbl),

    .wb_auto_tbl                            = wb_auto_tbl,
    .wb_auto_tbl_sz                         = ARRAY_SIZE(wb_auto_tbl),

    .contrast_tbl                           = contrast_tbl,
    .contrast_tbl_sz                        = contrast_tbl_sz,

    .brightness_tbl                         = brightness_tbl,
    .brightness_tbl_sz                      = brightness_tbl_sz,

    .saturation_tbl                         = saturation_tbl,
    .saturation_tbl_sz                      = saturation_tbl_sz,

    .sharpness_tbl                          = sharpness_tbl,
    .sharpness_tbl_sz                       = sharpness_tbl_sz,

    .exposure_tbl                           = exposure_tbl,
    .exposure_tbl_sz                        = exposure_tbl_sz,

    .otp_lsc_reg_tbl                        = otp_lsc_reg_tbl,
    .otp_lsc_reg_tbl_sz                     = otp_lsc_reg_tbl_sz,

    .lsc_reg_addr_tbl                       = lsc_reg_addr_tbl,
    .lsc_reg_addr_tbl_sz                    = ARRAY_SIZE(lsc_reg_addr_tbl),
};


