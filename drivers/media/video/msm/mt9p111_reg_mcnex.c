/*
 * drivers/media/video/msm/ov5642_reg_globaloptics.c
 *
 * Refer to drivers/media/video/msm/mt9d112_reg.c
 * For IC OV5642 of Module GLOBALOPTICS: 5.0Mp 1/4-Inch System-On-A-Chip (SOC) CMOS Digital Image Sensor
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
 * Created by zhang.shengjie@zte.com.cn
 */
 
#include "mt9p111.h"

static struct mt9p111_i2c_reg_conf const preview_snapshot_mode_reg_settings_array[] = {
    {0x098E, 0x486C, WORD_LEN, 0},   
    {0x0990, 0x0518, WORD_LEN, 0},   
    {0x098E, 0x486E, WORD_LEN, 0},   
    {0x0990, 0x03D4, WORD_LEN, 0},   
    {0x098E, 0x483A, WORD_LEN, 0},   
    {0x0990, 0x000C, WORD_LEN, 0},   
    {0x098E, 0x483C, WORD_LEN, 0},   
    {0x0990, 0x0018, WORD_LEN, 0},   
    {0x098E, 0x483E, WORD_LEN, 0},   
    {0x0990, 0x07B1, WORD_LEN, 0},   
    {0x098E, 0x4840, WORD_LEN, 0},   
    {0x0990, 0x0A45, WORD_LEN, 0},   
    {0x098E, 0x4842, WORD_LEN, 0},   
    {0x0990, 0x0001, WORD_LEN, 0},   
    {0x098E, 0x4844, WORD_LEN, 0},   
    {0x0990, 0x0103, WORD_LEN, 0},   
    {0x098E, 0x4846, WORD_LEN, 0},   
    {0x0990, 0x0103, WORD_LEN, 0},   
    {0x098E, 0x4848, WORD_LEN, 0},   
    {0x0990, 0x0103, WORD_LEN, 0},   
    {0x098E, 0x484A, WORD_LEN, 0},   
    {0x0990, 0x0103, WORD_LEN, 0},   
    {0x098E, 0x484C, WORD_LEN, 0},   
    {0x0990, 0x008C, WORD_LEN, 0},   
    {0x098E, 0x484E, WORD_LEN, 0},   
    {0x0990, 0x0001, WORD_LEN, 0},   
    {0x098E, 0xC850, WORD_LEN, 0},   
    {0x0990, 0x0000, WORD_LEN, 0},   
    {0x098E, 0xC851, WORD_LEN, 0},   
    {0x0990, 0x0000, WORD_LEN, 0},   
    {0x098E, 0x4852, WORD_LEN, 0},   
    {0x0990, 0x015C, WORD_LEN, 0},   
    {0x098E, 0x4854, WORD_LEN, 0},   
    {0x0990, 0x0702, WORD_LEN, 0},   
    {0x098E, 0x4856, WORD_LEN, 0},   
    {0x0990, 0x2113, WORD_LEN, 0},   
    {0x098E, 0x4858, WORD_LEN, 0},   
    {0x0990, 0x0000, WORD_LEN, 0},   
    {0x098E, 0x485A, WORD_LEN, 0},   
    {0x0990, 0x0001, WORD_LEN, 0},   
    {0x098E, 0x485C, WORD_LEN, 0},   
    {0x0990, 0x045A, WORD_LEN, 0},   
    {0x098E, 0x485E, WORD_LEN, 0},   
    {0x0990, 0xFFFF, WORD_LEN, 0},   
    {0x098E, 0x4860, WORD_LEN, 0},   
    {0x0990, 0x045A, WORD_LEN, 0},   
    {0x098E, 0x4862, WORD_LEN, 0},   
    {0x0990, 0x0FB1, WORD_LEN, 0},   
    {0x098E, 0x4864, WORD_LEN, 0},   
    {0x0990, 0xFFFE, WORD_LEN, 0},   
    {0x098E, 0x4866, WORD_LEN, 0},   
    {0x0990, 0x7F57, WORD_LEN, 0},   
    {0x098E, 0x4868, WORD_LEN, 0},   
    {0x0990, 0x045A, WORD_LEN, 0},   
    {0x098E, 0x486A, WORD_LEN, 0},   
    {0x0990, 0x0FB1, WORD_LEN, 0},   
    {0x098E, 0x4870, WORD_LEN, 0},   
    {0x0990, 0x0002, WORD_LEN, 0},   

#if defined(CONFIG_MACH_RAISE)
    {0x098E, 0x48AA, WORD_LEN, 0},   
    {0x0990, 648,    WORD_LEN, 0},   
    {0x098E, 0x48AC, WORD_LEN, 0},   
    {0x0990, 486,    WORD_LEN, 0},   
#elif defined(CONFIG_MACH_MOONCAKE)
    {0x098E, 0x48AA, WORD_LEN, 0},   
    {0x0990, 648,    WORD_LEN, 0},   
    {0x098E, 0x48AC, WORD_LEN, 0},   
    {0x0990, 486,    WORD_LEN, 0},   
#elif defined(CONFIG_MACH_JOE)
    {0x098E, 0x48AA, WORD_LEN, 0},   
    {0x0990, 648,    WORD_LEN, 0},   
    {0x098E, 0x48AC, WORD_LEN, 0},   
    {0x0990, 486,    WORD_LEN, 0},   
#else
    {0x098E, 0x48AA, WORD_LEN, 0},   
    {0x0990, 648,    WORD_LEN, 0},   
    {0x098E, 0x48AC, WORD_LEN, 0},   
    {0x0990, 486,    WORD_LEN, 0},   
#endif 

    {0x098E, 0x48AE, WORD_LEN, 0},   
    {0x0990, 0x0001, WORD_LEN, 0},   
    {0x098E, 0x48B0, WORD_LEN, 0},   
    {0x0990, 0x0000, WORD_LEN, 0},   
    {0x098E, 0x48B8, WORD_LEN, 0},   
    {0x0990, 0x0004, WORD_LEN, 0},   
    {0x098E, 0x48A4, WORD_LEN, 0},   
    {0x0990, 0x0A28, WORD_LEN, 0},   
    {0x098E, 0x48A6, WORD_LEN, 0},   
    {0x0990, 0x07A0, WORD_LEN, 0},   
    {0x098E, 0x4872, WORD_LEN, 0},   
    {0x0990, 0x0010, WORD_LEN, 0},   
    {0x098E, 0x4874, WORD_LEN, 0},   
    {0x0990, 0x001C, WORD_LEN, 0},   
    {0x098E, 0x4876, WORD_LEN, 0},   
    {0x0990, 0x07AF, WORD_LEN, 0},   
    {0x098E, 0x4878, WORD_LEN, 0},   
    {0x0990, 0x0A43, WORD_LEN, 0},   
    {0x098E, 0x487A, WORD_LEN, 0},   
    {0x0990, 0x0001, WORD_LEN, 0},   
    {0x098E, 0x487C, WORD_LEN, 0},   
    {0x0990, 0x0101, WORD_LEN, 0},   
    {0x098E, 0x487E, WORD_LEN, 0},   
    {0x0990, 0x0101, WORD_LEN, 0},   
    {0x098E, 0x4880, WORD_LEN, 0},   
    {0x0990, 0x0101, WORD_LEN, 0},   
    {0x098E, 0x4882, WORD_LEN, 0},   
    {0x0990, 0x0101, WORD_LEN, 0},   
    {0x098E, 0x4884, WORD_LEN, 0},   
    {0x0990, 0x0064, WORD_LEN, 0},   
    {0x098E, 0x4886, WORD_LEN, 0},   
    {0x0990, 0x0000, WORD_LEN, 0},   
    {0x098E, 0xC888, WORD_LEN, 0},   
    {0x0990, 0x0000, WORD_LEN, 0},   
    {0x098E, 0xC889, WORD_LEN, 0},   
    {0x0990, 0x0000, WORD_LEN, 0},   
    {0x098E, 0x488A, WORD_LEN, 0},   
    {0x0990, 0x009C, WORD_LEN, 0},   
    {0x098E, 0x488C, WORD_LEN, 0},   
    {0x0990, 0x034A, WORD_LEN, 0},   
    {0x098E, 0x488E, WORD_LEN, 0},   
    {0x0990, 0x2113, WORD_LEN, 0},   
    {0x098E, 0x4890, WORD_LEN, 0},   
    {0x0990, 0x0000, WORD_LEN, 0},   
    {0x098E, 0x4892, WORD_LEN, 0},   
    {0x0990, 0x0001, WORD_LEN, 0},   
    {0x098E, 0x4894, WORD_LEN, 0},   
    {0x0990, 0x07EF, WORD_LEN, 0},   
    {0x098E, 0x4896, WORD_LEN, 0},   
    {0x0990, 0xFFFF, WORD_LEN, 0},   
    {0x098E, 0x4898, WORD_LEN, 0},   
    {0x0990, 0x07EF, WORD_LEN, 0},   
    {0x098E, 0x489A, WORD_LEN, 0},   
    {0x0990, 0x37C4, WORD_LEN, 0},   
    {0x098E, 0x489C, WORD_LEN, 0},   
    {0x0990, 0xFFFE, WORD_LEN, 0},   
    {0x098E, 0x489E, WORD_LEN, 0},   
    {0x0990, 0x7F57, WORD_LEN, 0},   
    {0x098E, 0x48A0, WORD_LEN, 0},   
    {0x0990, 0x07EF, WORD_LEN, 0},   
    {0x098E, 0x48A2, WORD_LEN, 0},   
    {0x0990, 0x37C4, WORD_LEN, 0},   
    {0x098E, 0x48A8, WORD_LEN, 0},   
    {0x0990, 0x0014, WORD_LEN, 0},   

#if defined(CONFIG_MACH_RAISE)
    {0x098E, 0x48C0, WORD_LEN, 0},   
    {0x0990, 2592,   WORD_LEN, 0},   
    {0x098E, 0x48C2, WORD_LEN, 0},   
    {0x0990, 1944,   WORD_LEN, 0},   
#elif defined(CONFIG_MACH_MOONCAKE)
    {0x098E, 0x48C0, WORD_LEN, 0},   
    {0x0990, 2592,   WORD_LEN, 0},   
    {0x098E, 0x48C2, WORD_LEN, 0},   
    {0x0990, 1944,   WORD_LEN, 0},   
#elif defined(CONFIG_MACH_JOE)
    {0x098E, 0x48C0, WORD_LEN, 0},   
    {0x0990, 2592,   WORD_LEN, 0},   
    {0x098E, 0x48C2, WORD_LEN, 0},   
    {0x0990, 1944,   WORD_LEN, 0},   
#else
    {0x098E, 0x48C0, WORD_LEN, 0},   
    {0x0990, 2592,   WORD_LEN, 0},   
    {0x098E, 0x48C2, WORD_LEN, 0},   
    {0x0990, 1944,   WORD_LEN, 0},   
#endif 
    
    {0x098E, 0x48C4, WORD_LEN, 0},   
    {0x0990, 0x0001, WORD_LEN, 0},   
    {0x098E, 0x48C6, WORD_LEN, 0},   
    {0x0990, 0x0000, WORD_LEN, 0},   
    {0x098E, 0x48CE, WORD_LEN, 0},   
    {0x0990, 0x0004, WORD_LEN, 0},   
    {0x098E, 0x2010, WORD_LEN, 0},
    {0x0990, 0x00E6, WORD_LEN, 0},   
    {0x098E, 0x2012, WORD_LEN, 0},
    {0x0990, 0x00FA, WORD_LEN, 0},   
    {0x098E, 0x2014, WORD_LEN, 0},
    {0x0990, 0x00BE, WORD_LEN, 0},   
    {0x098E, 0x2016, WORD_LEN, 0},
    {0x0990, 0x00D2, WORD_LEN, 0},   
    {0x098E, 0x2018, WORD_LEN, 0},
    {0x0990, 0x00F0, WORD_LEN, 0},   
    {0x098E, 0x201A, WORD_LEN, 0},
    {0x0990, 0x0044, WORD_LEN, 0},   
    {0x098E, 0x201C, WORD_LEN, 0},
    {0x0990, 0x00C8, WORD_LEN, 0},   
    {0x098E, 0x201E, WORD_LEN, 0},
    {0x0990, 0x0038, WORD_LEN, 0},   
    {0x098E, 0x6004, WORD_LEN, 0},   
    {0x0990, 0x03C0, WORD_LEN, 0},   
    {0x098E, 0xA000, WORD_LEN, 0},   
    {0x0990, 0x1800, WORD_LEN, 0},
    {0x098E, 0x8417, WORD_LEN, 0},   
    {0x0990, 0x0400, WORD_LEN, 0},

    {0x30D4, 0xB080, WORD_LEN, 0},   // COLUMN_CORRECTION
    {0x316E, 0xC400, WORD_LEN, 0},   // DAC_ECL
    {0x305E, 0x10A0, WORD_LEN, 0},   // GLOBAL_GAIN
    {0x3E00, 0x0010, WORD_LEN, 0},   // SAMP_CONTROL
    {0x3E02, 0xED02, WORD_LEN, 0},   // SAMP_ADDR_EN
    {0x3E04, 0xC88C, WORD_LEN, 0},   // SAMP_RD1_SIG
    {0x3E06, 0xC88C, WORD_LEN, 0},   // SAMP_RD1_SIG_BOOST
    {0x3E08, 0x700A, WORD_LEN, 0},   // SAMP_RD1_RST
    {0x3E0A, 0x701E, WORD_LEN, 0},   // SAMP_RD1_RST_BOOST
    {0x3E0C, 0x00FF, WORD_LEN, 0},   // SAMP_RST1_EN
    {0x3E0E, 0x00FF, WORD_LEN, 0},   // SAMP_RST1_BOOST
    {0x3E10, 0x00FF, WORD_LEN, 0},   // SAMP_RST1_CLOOP_SH
    {0x3E12, 0x0000, WORD_LEN, 0},   // SAMP_RST_BOOST_SEQ
    {0x3E14, 0xC78C, WORD_LEN, 0},   // SAMP_SAMP1_SIG
    {0x3E16, 0x6E06, WORD_LEN, 0},   // SAMP_SAMP1_RST
    {0x3E18, 0xA58C, WORD_LEN, 0},   // SAMP_TX_EN
    {0x3E1A, 0xA58E, WORD_LEN, 0},   // SAMP_TX_BOOST
    {0x3E1C, 0xA58E, WORD_LEN, 0},   // SAMP_TX_CLOOP_SH
    {0x3E1E, 0xC0D0, WORD_LEN, 0},   // SAMP_TX_BOOST_SEQ
    {0x3E20, 0xEB00, WORD_LEN, 0},   // SAMP_VLN_EN
    {0x3E22, 0x00FF, WORD_LEN, 0},   // SAMP_VLN_HOLD
    {0x3E24, 0xEB02, WORD_LEN, 0},   // SAMP_VCL_EN
    {0x3E26, 0xEA02, WORD_LEN, 0},   // SAMP_COLCLAMP
    {0x3E28, 0xEB0A, WORD_LEN, 0},   // SAMP_SH_VCL
    {0x3E2A, 0xEC01, WORD_LEN, 0},   // SAMP_SH_VREF
    {0x3E2C, 0xEB01, WORD_LEN, 0},   // SAMP_SH_VBST
    {0x3E2E, 0x00FF, WORD_LEN, 0},   // SAMP_SPARE
    {0x3E30, 0x00F3, WORD_LEN, 0},   // SAMP_READOUT
    {0x3E32, 0x3DFA, WORD_LEN, 0},   // SAMP_RESET_DONE
    {0x3E34, 0x00FF, WORD_LEN, 0},   // SAMP_VLN_CLAMP
    {0x3E36, 0x00F3, WORD_LEN, 0},   // SAMP_ASC_INT
    {0x3E38, 0x0000, WORD_LEN, 0},   // SAMP_RS_CLOOP_SH_R
    {0x3E3A, 0xF802, WORD_LEN, 0},   // SAMP_RS_CLOOP_SH
    {0x3E3C, 0x0FFF, WORD_LEN, 0},   // SAMP_RS_BOOST_SEQ
    {0x3E3E, 0xEA10, WORD_LEN, 0},   // SAMP_TXLO_GND
    {0x3E40, 0xEB05, WORD_LEN, 0},   // SAMP_VLN_PER_COL
    {0x3E42, 0xE5C8, WORD_LEN, 0},   // SAMP_RD2_SIG
    {0x3E44, 0xE5C8, WORD_LEN, 0},   // SAMP_RD2_SIG_BOOST
    {0x3E46, 0x8C70, WORD_LEN, 0},   // SAMP_RD2_RST
    {0x3E48, 0x8C71, WORD_LEN, 0},   // SAMP_RD2_RST_BOOST
    {0x3E4A, 0x00FF, WORD_LEN, 0},   // SAMP_RST2_EN
    {0x3E4C, 0x00FF, WORD_LEN, 0},   // SAMP_RST2_BOOST
    {0x3E4E, 0x00FF, WORD_LEN, 0},   // SAMP_RST2_CLOOP_SH
    {0x3E50, 0xE38D, WORD_LEN, 0},   // SAMP_SAMP2_SIG
    {0x3E52, 0x8B0A, WORD_LEN, 0},   // SAMP_SAMP2_RST
    {0x3E58, 0xEB0A, WORD_LEN, 0},   // SAMP_PIX_CLAMP_EN
    {0x3E5C, 0x0A00, WORD_LEN, 0},   // SAMP_PIX_PULLUP_EN
    {0x3E5E, 0x00FF, WORD_LEN, 0},   // SAMP_PIX_PULLDOWN_EN_R
    {0x3E60, 0x00FF, WORD_LEN, 0},   // SAMP_PIX_PULLDOWN_EN_S
    {0x3E90, 0x3C01, WORD_LEN, 0},   // RST_ADDR_EN
    {0x3E92, 0x00FF, WORD_LEN, 0},   // RST_RST_EN
    {0x3E94, 0x00FF, WORD_LEN, 0},   // RST_RST_BOOST
    {0x3E96, 0x3C00, WORD_LEN, 0},   // RST_TX_EN
    {0x3E98, 0x3C00, WORD_LEN, 0},   // RST_TX_BOOST
    {0x3E9A, 0x3C00, WORD_LEN, 0},   // RST_TX_CLOOP_SH
    {0x3E9C, 0xC0E0, WORD_LEN, 0},   // RST_TX_BOOST_SEQ
    {0x3E9E, 0x00FF, WORD_LEN, 0},   // RST_RST_CLOOP_SH
    {0x3EA0, 0x0000, WORD_LEN, 0},   // RST_RST_BOOST_SEQ
    {0x3EA6, 0x3C00, WORD_LEN, 0},   // RST_PIX_PULLUP_EN
    {0x3ED8, 0x3057, WORD_LEN, 0},   // DAC_LD_12_13
    {0x316C, 0xB44F, WORD_LEN, 0},   // DAC_TXLO
    {0x316E, 0xC6FF, WORD_LEN, 0},   // DAC_ECL
    {0x3ED2, 0xEA0F, WORD_LEN, 0},   // DAC_LD_6_7
    {0x3ED4, 0x0004, WORD_LEN, 0},   // DAC_LD_8_9
    {0x3EDC, 0x6020, WORD_LEN, 0},   // DAC_LD_16_17
    {0x3EE6, 0xA5C0, WORD_LEN, 0},   // DAC_LD_26_27
    {0x3ED0, 0x2409, WORD_LEN, 0},   // DAC_LD_4_5
    {0x3EDE, 0x0A49, WORD_LEN, 0},   // DAC_LD_18_19
    {0x3EE0, 0x4910, WORD_LEN, 0},   // DAC_LD_20_21
    {0x3EE2, 0x09D2, WORD_LEN, 0},   // DAC_LD_22_23
    {0x3EDA, 0xE060, WORD_LEN, 0},   // DAC_LD_14_15??????
    {0x30B6, 0x0006, WORD_LEN, 0},   // AUTOLR_CONTROL
    {0x301A, 0x10F4, WORD_LEN, 0},   
    {0x301E, 0x0000, WORD_LEN, 0},   
    {0x301A, 0x10FC, WORD_LEN, 0},   
    {0x098E, 0xDC33, WORD_LEN, 0},   
    {0x0990, 0x0000, WORD_LEN, 0},
    {0x098E, 0xDC35, WORD_LEN, 0},   
    {0x0990, 0x0400, WORD_LEN, 0},
    {0x326E, 0x0006, WORD_LEN, 0},   
    {0x098E, 0xDC37, WORD_LEN, 0},   
    {0x0990, 0x6200, WORD_LEN, 0},
    {0x35A4, 0x0596, WORD_LEN, 0},   
    {0x35A2, 0x0094, WORD_LEN, 0},   
    {0x098E, 0xDC36, WORD_LEN, 0},   
    {0x0990, 0x2300, WORD_LEN, 0},

    {0x31E0, 0x0003, WORD_LEN, 0},   // PIX_DEF_ID
    {0x098E, 0xBC18, WORD_LEN, 0},
    {0x0990, 0x0000, WORD_LEN, 0},   
    {0x098E, 0xBC19, WORD_LEN, 0},
    {0x0990, 0x0B00, WORD_LEN, 0},   
    {0x098E, 0xBC1A, WORD_LEN, 0},
    {0x0990, 0x1F00, WORD_LEN, 0},   
    {0x098E, 0xBC1B, WORD_LEN, 0},
    {0x0990, 0x3C00, WORD_LEN, 0},   
    {0x098E, 0xBC1C, WORD_LEN, 0},
    {0x0990, 0x6000, WORD_LEN, 0},   
    {0x098E, 0xBC1D, WORD_LEN, 0},
    {0x0990, 0x7D00, WORD_LEN, 0},   
    {0x098E, 0xBC1E, WORD_LEN, 0},
    {0x0990, 0x9500, WORD_LEN, 0},   
    {0x098E, 0xBC1F, WORD_LEN, 0},
    {0x0990, 0xA800, WORD_LEN, 0},   
    {0x098E, 0xBC20, WORD_LEN, 0},
    {0x0990, 0xB700, WORD_LEN, 0},   
    {0x098E, 0xBC21, WORD_LEN, 0},
    {0x0990, 0xC300, WORD_LEN, 0},   
    {0x098E, 0xBC22, WORD_LEN, 0},
    {0x0990, 0xCD00, WORD_LEN, 0},   
    {0x098E, 0xBC23, WORD_LEN, 0},
    {0x0990, 0xD600, WORD_LEN, 0},   
    {0x098E, 0xBC24, WORD_LEN, 0},
    {0x0990, 0xDE00, WORD_LEN, 0},   
    {0x098E, 0xBC25, WORD_LEN, 0},
    {0x0990, 0xE500, WORD_LEN, 0},   
    {0x098E, 0xBC26, WORD_LEN, 0},
    {0x0990, 0xEB00, WORD_LEN, 0},   
    {0x098E, 0xBC27, WORD_LEN, 0},
    {0x0990, 0xF100, WORD_LEN, 0},   
    {0x098E, 0xBC28, WORD_LEN, 0},
    {0x0990, 0xF600, WORD_LEN, 0},   
    {0x098E, 0xBC29, WORD_LEN, 0},
    {0x0990, 0xFB00, WORD_LEN, 0},   
    {0x098E, 0xBC2A, WORD_LEN, 0},
    {0x0990, 0xFF00, WORD_LEN, 0},   
    {0x098E, 0xBC51, WORD_LEN, 0},
    {0x0990, 0x0200, WORD_LEN, 0},   

    {0x098E, 0xB801, WORD_LEN, 0},   // [STAT_MODE]
    {0x0990, 0xE000, WORD_LEN, 0},
    {0x098E, 0xB862, WORD_LEN, 0},   // [STAT_BMTRACKING_SPEED]
    {0x0990, 0x0400, WORD_LEN, 0},
    {0x098E, 0xB829, WORD_LEN, 0},   // [STAT_LL_BRIGHTNESS_METRIC_DIVISOR]
    {0x0990, 0x6600, WORD_LEN, 0},
    {0x098E, 0xB863, WORD_LEN, 0},   // [STAT_BM_MUL]
    {0x0990, 0x2000, WORD_LEN, 0},
    {0x098E, 0xB827, WORD_LEN, 0},   
    {0x0990, 0x0E00, WORD_LEN, 0},
    {0x098E, 0xA409, WORD_LEN, 0},   
    {0x0990, 0x3800, WORD_LEN, 0},
    {0x098E, 0xA401, WORD_LEN, 0},   
    {0x0990, 0x1C00, WORD_LEN, 0},
    {0x098E, 0x281C, WORD_LEN, 0},   
    {0x0990, 0x0040, WORD_LEN, 0},
    {0x098E, 0x2820, WORD_LEN, 0},   
    {0x0990, 0x01FC, WORD_LEN, 0},
    {0x098E, 0x2822, WORD_LEN, 0},   
    {0x0990, 0x0080, WORD_LEN, 0},   
    {0x098E, 0x2824, WORD_LEN, 0},   
    {0x0990, 0x0100, WORD_LEN, 0},
    {0x098E, 0x2818, WORD_LEN, 0},   
    {0x0990, 0x07D0, WORD_LEN, 0},   
    {0x098E, 0x281A, WORD_LEN, 0},   
    {0x0990, 0x0BB8, WORD_LEN, 0},   

    {0x098E, 0x3C52, WORD_LEN, 0},   // [LL_START_BRIGHTNESS_METRIC]
    {0x0990, 0x026C, WORD_LEN, 0},
    {0x098E, 0x3C54, WORD_LEN, 0},   // [LL_END_BRIGHTNESS_METRIC]
    {0x0990, 0x05DC, WORD_LEN, 0},
    {0x098E, 0x3C58, WORD_LEN, 0},   // [LL_START_GAIN_METRIC]
    {0x0990, 0x012C, WORD_LEN, 0},
    {0x098E, 0x3C5A, WORD_LEN, 0},   // [LL_END_GAIN_METRIC]
    {0x0990, 0x106C, WORD_LEN, 0},
    {0x098E, 0x3C5E, WORD_LEN, 0},   // [LL_START_APERTURE_GAIN_BM]
    {0x0990, 0x00FA, WORD_LEN, 0},
    {0x098E, 0x3C60, WORD_LEN, 0},   // [LL_END_APERTURE_GAIN_BM]
    {0x0990, 0x02B2, WORD_LEN, 0},
    {0x098E, 0x3C66, WORD_LEN, 0},   // [LL_START_APERTURE_GM]
    {0x0990, 0x00FA, WORD_LEN, 0},
    {0x098E, 0x3C68, WORD_LEN, 0},   // [LL_END_APERTURE_GM]
    {0x0990, 0x02B2, WORD_LEN, 0},
    {0x098E, 0x3C86, WORD_LEN, 0},   // [LL_START_FFNR_GM]
    {0x0990, 0x00C8, WORD_LEN, 0},
    {0x098E, 0x3C88, WORD_LEN, 0},   // [LL_END_FFNR_GM]
    {0x0990, 0x0658, WORD_LEN, 0},
    {0x098E, 0x3CBC, WORD_LEN, 0},   // [LL_SFFB_START_GAIN]
    {0x0990, 0x0040, WORD_LEN, 0},
    {0x098E, 0x3CBE, WORD_LEN, 0},   // [LL_SFFB_END_GAIN]
    {0x0990, 0x01FC, WORD_LEN, 0},
    {0x098E, 0x3CCC, WORD_LEN, 0},   // [LL_SFFB_START_MAX_GM]
    {0x0990, 0x00C8, WORD_LEN, 0},
    {0x098E, 0x3CCE, WORD_LEN, 0},   // [LL_SFFB_END_MAX_GM]
    {0x0990, 0x0633, WORD_LEN, 0},
    {0x098E, 0x3C90, WORD_LEN, 0},   // [LL_START_GRB_GM]
    {0x0990, 0x00C8, WORD_LEN, 0},
    {0x098E, 0x3C92, WORD_LEN, 0},   // [LL_END_GRB_GM]
    {0x0990, 0x0658, WORD_LEN, 0},
    {0x098E, 0x3C0E, WORD_LEN, 0},   // [LL_GAMMA_CURVE_ADJ_START_POS]
    {0x0990, 0x0001, WORD_LEN, 0},
    {0x098E, 0x3C10, WORD_LEN, 0},   // [LL_GAMMA_CURVE_ADJ_MID_POS]
    {0x0990, 0x0002, WORD_LEN, 0},
    {0x098E, 0x3C12, WORD_LEN, 0},   // [LL_GAMMA_CURVE_ADJ_END_POS]
    {0x0990, 0x09C4, WORD_LEN, 0},
    {0x098E, 0x3CAA, WORD_LEN, 0},   // [LL_CDC_THR_ADJ_START_POS]
    {0x0990, 0x05DC, WORD_LEN, 0},
    {0x098E, 0x3CAC, WORD_LEN, 0},   // [LL_CDC_THR_ADJ_MID_POS]
    {0x0990, 0x026C, WORD_LEN, 0},
    {0x098E, 0x3CAE, WORD_LEN, 0},   // [LL_CDC_THR_ADJ_END_POS]
    {0x0990, 0x0028, WORD_LEN, 0},
    {0x098E, 0x3CD8, WORD_LEN, 0},   // [LL_PCR_START_BM]
    {0x0990, 0x00E0, WORD_LEN, 0},
    {0x098E, 0x3CDA, WORD_LEN, 0},   // [LL_PCR_END_BM]
    {0x0990, 0x05DC, WORD_LEN, 0},

    {0x3380, 0x0504, WORD_LEN, 0},   
    {0x098E, 0xBCB2, WORD_LEN, 0},   
    {0x0990, 0x2800, WORD_LEN, 0},
    {0x098E, 0xBCB3, WORD_LEN, 0},   
    {0x0990, 0x5F00, WORD_LEN, 0},

    {0x098E, 0xBC94, WORD_LEN, 0},   // [LL_GB_START_THRESHOLD_0]
    {0x0990, 0x0600, WORD_LEN, 0},
    {0x098E, 0xBC95, WORD_LEN, 0},   // [LL_GB_START_THRESHOLD_1]
    {0x0990, 0x0500, WORD_LEN, 0},
    {0x098E, 0xBC96, WORD_LEN, 0},   // [LL_GB_START_THRESHOLD_2]
    {0x0990, 0x0600, WORD_LEN, 0},
    {0x098E, 0xBC97, WORD_LEN, 0},   // [LL_GB_START_THRESHOLD_3]
    {0x0990, 0x0500, WORD_LEN, 0},
    {0x098E, 0xBC9C, WORD_LEN, 0},   // [LL_GB_END_THRESHOLD_0]
    {0x0990, 0x0900, WORD_LEN, 0},
    {0x098E, 0xBC9D, WORD_LEN, 0},   // [LL_GB_END_THRESHOLD_1]
    {0x0990, 0x0500, WORD_LEN, 0},
    {0x098E, 0xBC9E, WORD_LEN, 0},   // [LL_GB_END_THRESHOLD_2]
    {0x0990, 0x0900, WORD_LEN, 0},
    {0x098E, 0xBC9F, WORD_LEN, 0},   // [LL_GB_END_THRESHOLD_3]
    {0x0990, 0x0500, WORD_LEN, 0},
    {0x33B0, 0x2A16, WORD_LEN, 0},   // FFNR_ALPHA_BETA
    {0x098E, 0xBC8A, WORD_LEN, 0},   
    {0x0990, 0x2C00, WORD_LEN, 0},
    {0x098E, 0xBC8B, WORD_LEN, 0},   
    {0x0990, 0xCF00, WORD_LEN, 0},
    {0x098E, 0xBC8C, WORD_LEN, 0},   
    {0x0990, 0x8B00, WORD_LEN, 0},
    {0x098E, 0xBC8D, WORD_LEN, 0},   
    {0x0990, 0xD700, WORD_LEN, 0},
    {0x098E, 0xBC8E, WORD_LEN, 0},   
    {0x0990, 0x7F00, WORD_LEN, 0},
    {0x098E, 0xBC8F, WORD_LEN, 0},   
    {0x0990, 0x0000, WORD_LEN, 0},

    {0x098E, 0xBCC0, WORD_LEN, 0},   // [LL_SFFB_RAMP_START]
    {0x0990, 0x1F00, WORD_LEN, 0},
    {0x098E, 0xBCC1, WORD_LEN, 0},   // [LL_SFFB_RAMP_STOP]
    {0x0990, 0x0300, WORD_LEN, 0},
    {0x098E, 0xBCC2, WORD_LEN, 0},   // [LL_SFFB_SLOPE_START]
    {0x0990, 0x1E00, WORD_LEN, 0},
    {0x098E, 0xBCC3, WORD_LEN, 0},   // [LL_SFFB_SLOPE_STOP]
    {0x0990, 0x0F00, WORD_LEN, 0},
    {0x098E, 0xBCC4, WORD_LEN, 0},   // [LL_SFFB_THSTART]
    {0x0990, 0x0A00, WORD_LEN, 0},
    {0x098E, 0xBCC5, WORD_LEN, 0},   // [LL_SFFB_THSTOP]
    {0x0990, 0x0F00, WORD_LEN, 0},
    {0x098E, 0x3CBA, WORD_LEN, 0},   // [LL_SFFB_CONFIG]
    {0x0990, 0x0009, WORD_LEN, 0},
    {0x098E, 0xB42F, WORD_LEN, 0},   
    {0x0990, 0x0100, WORD_LEN, 0},
    {0x098E, 0xB42F, WORD_LEN, 0},   
    {0x0990, 0x0200, WORD_LEN, 0},

    {0x098E, 0x3C14, WORD_LEN, 0},   // [LL_GAMMA_FADE_TO_BLACK_START_POS]
    {0x0990, 0xFFFE, WORD_LEN, 0},
    {0x098E, 0x3C16, WORD_LEN, 0},   // [LL_GAMMA_FADE_TO_BLACK_END_POS]
    {0x0990, 0xFFFF, WORD_LEN, 0},
    {0x33BA, 0x0086, WORD_LEN, 0},   
    {0x33BE, 0x0000, WORD_LEN, 0},   
    {0x098E, 0xBCE2, WORD_LEN, 0},   
    {0x0990, 0x7700, WORD_LEN, 0},
    {0x098E, 0xBCE3, WORD_LEN, 0},   
    {0x0990, 0x3300, WORD_LEN, 0},
    {0x098E, 0xBC62, WORD_LEN, 0},   
    {0x0990, 0x1900, WORD_LEN, 0},
    {0x098E, 0xBC63, WORD_LEN, 0},   
    {0x0990, 0x1F00, WORD_LEN, 0},
    {0x098E, 0xBC64, WORD_LEN, 0},   
    {0x0990, 0x1900, WORD_LEN, 0},
    {0x098E, 0xBC65, WORD_LEN, 0},   
    {0x0990, 0x1F00, WORD_LEN, 0},
    {0x098E, 0xBC6A, WORD_LEN, 0},   
    {0x0990, 0x0400, WORD_LEN, 0},
    {0x098E, 0xBC6B, WORD_LEN, 0},   
    {0x0990, 0x0000, WORD_LEN, 0},
    {0x098E, 0xBC6C, WORD_LEN, 0},   
    {0x0990, 0x0000, WORD_LEN, 0},
    {0x098E, 0xBC6D, WORD_LEN, 0},   
    {0x0990, 0x0000, WORD_LEN, 0},
    {0x3284, 0x0000, WORD_LEN, 0},   

    {0x098E, 0xBC56, WORD_LEN, 0},   // [LL_START_CCM_SATURATION]
    {0x0990, 0xF000, WORD_LEN, 0},
    {0x098E, 0xBC57, WORD_LEN, 0},   // [LL_END_CCM_SATURATION]
    {0x0990, 0x1E00, WORD_LEN, 0},
    {0x098E, 0xBCDE, WORD_LEN, 0},   
    {0x0990, 0x0400, WORD_LEN, 0},
    {0x098E, 0xBCDF, WORD_LEN, 0},   
    {0x0990, 0x1800, WORD_LEN, 0},
    {0x098E, 0xBCE0, WORD_LEN, 0},   
    {0x0990, 0x0400, WORD_LEN, 0},
    {0x098E, 0xBCE1, WORD_LEN, 0},   
    {0x0990, 0x0400, WORD_LEN, 0},

    {0x098E, 0x3CD0, WORD_LEN, 0},   // [LL_SFFB_SOBEL_FLAT_START]
    {0x0990, 0x000A, WORD_LEN, 0},
    {0x098E, 0x3CD2, WORD_LEN, 0},   // [LL_SFFB_SOBEL_FLAT_STOP]
    {0x0990, 0x00FE, WORD_LEN, 0},
    {0x098E, 0x3CD4, WORD_LEN, 0},   // [LL_SFFB_SOBEL_SHARP_START]
    {0x0990, 0x001E, WORD_LEN, 0},
    {0x098E, 0x3CD6, WORD_LEN, 0},   // [LL_SFFB_SOBEL_SHARP_STOP]
    {0x0990, 0x00FF, WORD_LEN, 0},
    {0x098E, 0xBCC6, WORD_LEN, 0},   // [LL_SFFB_SHARPENING_START]
    {0x0990, 0x0000, WORD_LEN, 0},
    {0x098E, 0xBCC7, WORD_LEN, 0},   // [LL_SFFB_SHARPENING_STOP]
    {0x0990, 0x0000, WORD_LEN, 0},
    {0x098E, 0xBCC8, WORD_LEN, 0},   // [LL_SFFB_FLATNESS_START]
    {0x0990, 0x2000, WORD_LEN, 0},
    {0x098E, 0xBCC9, WORD_LEN, 0},   // [LL_SFFB_FLATNESS_STOP]
    {0x0990, 0x4000, WORD_LEN, 0},
    {0x098E, 0xBCCA, WORD_LEN, 0},   // [LL_SFFB_TRANSITION_START]
    {0x0990, 0x0400, WORD_LEN, 0},
    {0x098E, 0xBCCB, WORD_LEN, 0},   // [LL_SFFB_TRANSITION_STOP]
    {0x0990, 0x0000, WORD_LEN, 0},
};

static struct mt9p111_i2c_reg_conf const noise_reduction_reg_settings_array[] = {

    {0x0000, 0x0000, WORD_LEN, 0},
};

static struct mt9p111_i2c_reg_conf const lens_roll_off_tbl[] = {
    {0x0000, 0x0000, WORD_LEN, 0},
};

static struct mt9p111_i2c_reg_conf const pll_setup_tbl[] = {
    {0x0010, 0x0560, WORD_LEN, 0},      
    {0x0012, 0x0070, WORD_LEN, 0},      
    {0x0014, 0x2025, WORD_LEN, 0},      

#if defined(CONFIG_MACH_RAISE)
    {0x001E, 0x0777, WORD_LEN, 0},      
#elif defined(CONFIG_MACH_MOONCAKE)
    {0x001E, 0x0474, WORD_LEN, 0},      
#elif defined(CONFIG_MACH_JOE)
    {0x001E, 0x0474, WORD_LEN, 0},      
#else
    {0x001E, 0x0777, WORD_LEN, 0},      
#endif 
    {0x0022, 0x01E0, WORD_LEN, 0},      
    {0x002A, 0x7F57, WORD_LEN, 0},      
    {0x002C, 0x0000, WORD_LEN, 0},      
    {0x002E, 0x0000, WORD_LEN, 0},      
    {0x0018, 0x4008, WORD_LEN, 100},    
};

static struct mt9p111_i2c_reg_conf const sequencer_tbl[] = {
    {0x098E, 0x8404, WORD_LEN, 0},
    {0x0990, 0x0600, WORD_LEN, 300},
};

struct mt9p111_reg_t mt9p111_regs = {
    .prev_snap_reg_settings =               preview_snapshot_mode_reg_settings_array,
    .prev_snap_reg_settings_size =          ARRAY_SIZE(preview_snapshot_mode_reg_settings_array),

    .noise_reduction_reg_settings =         noise_reduction_reg_settings_array,
    .noise_reduction_reg_settings_size =    0,  

    .plltbl =                               pll_setup_tbl,
    .plltbl_size =                          ARRAY_SIZE(pll_setup_tbl),

    .stbl =                                 sequencer_tbl,
    .stbl_size =                            ARRAY_SIZE(sequencer_tbl),

    .rftbl =                                lens_roll_off_tbl,
    .rftbl_size =                           0,  
};


