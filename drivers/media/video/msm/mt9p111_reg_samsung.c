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
 * 
 */
#include "mt9p111.h"

static struct mt9p111_i2c_reg_conf const otp_lsc_reg_tbl_0[] = {
    {0x098E, 0x602A, WORD_LEN, 0},   
    {0xE02A, 0x0001, WORD_LEN, 200}, 
    {0xD004, 0x04, BYTE_LEN, 0},    
    {0xD00C, 0x03, BYTE_LEN, 0},    

    {0xD006, 0x0000, WORD_LEN, 0},  
    {0xD008, 0x0000, WORD_LEN, 0},  
    {0xD00A, 0x0000, WORD_LEN, 0},  
    {0xD005, 0x02, BYTE_LEN, 0},    

    {0xD00D, 0x00, BYTE_LEN, 0}, 
    {0xD00E, 0x30, BYTE_LEN, 0}, 
    {0xD00F, 0x65, BYTE_LEN, 0}, 
    {0xD011, 0x35, BYTE_LEN, 0}, 
    {0xD012, 0x70, BYTE_LEN, 0}, 
    {0xD013, 0xFF, BYTE_LEN, 0}, 

    {0xD002, 0x0004, WORD_LEN, 0},	
};

static struct mt9p111_i2c_reg_conf const otp_lsc_reg_tbl_1[] = {
    {0x098E, 0x602A, WORD_LEN, 0},   
    {0xE02A, 0x0001, WORD_LEN, 200}, 

    {0xD004, 0x04, BYTE_LEN, 0},    
    {0xD00C, 0x03, BYTE_LEN, 0},    

    {0xD006, 0x0100, WORD_LEN, 0},  
    {0xD008, 0x0100, WORD_LEN, 0},  
    {0xD00A, 0x0100, WORD_LEN, 0},  
    {0xD005, 0x02, BYTE_LEN, 0},    

    {0xD00D, 0x00, BYTE_LEN, 0}, 
    {0xD00E, 0x30, BYTE_LEN, 0}, 
    {0xD00F, 0x65, BYTE_LEN, 0}, 
    {0xD011, 0x35, BYTE_LEN, 0}, 
    {0xD012, 0x70, BYTE_LEN, 0}, 
    {0xD013, 0xFF, BYTE_LEN, 0}, 

    {0xD002, 0x0004, WORD_LEN, 0},	
};

static struct mt9p111_i2c_reg_conf const otp_lsc_reg_tbl_2[] = {
    {0x098E, 0x602A, WORD_LEN, 0},   
    {0xE02A, 0x0001, WORD_LEN, 200}, 

    {0xD004, 0x04, BYTE_LEN, 0},    
    {0xD00C, 0x03, BYTE_LEN, 0},    

    {0xD006, 0x0200, WORD_LEN, 0},  
    {0xD008, 0x0200, WORD_LEN, 0},  
    {0xD00A, 0x0200, WORD_LEN, 0},  
    {0xD005, 0x02, BYTE_LEN, 0},    

    {0xD00D, 0x00, BYTE_LEN, 0}, 
    {0xD00E, 0x30, BYTE_LEN, 0}, 
    {0xD00F, 0x65, BYTE_LEN, 0}, 
    {0xD011, 0x35, BYTE_LEN, 0}, 
    {0xD012, 0x70, BYTE_LEN, 0}, 
    {0xD013, 0xFF, BYTE_LEN, 0}, 

    {0xD002, 0x0004, WORD_LEN, 0},	
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


static struct mt9p111_i2c_reg_conf const preview_snapshot_mode_reg_settings_array[] = {
     
    {0xC83A, 0x000C, WORD_LEN, 0}, 	
    {0xC83C, 0x0018, WORD_LEN, 0}, 	
    {0xC83E, 0x07B1, WORD_LEN, 0}, 	
    {0xC840, 0x0A45, WORD_LEN, 0}, 	
    {0xC842, 0x0001, WORD_LEN, 0}, 	
    {0xC844, 0x0103, WORD_LEN, 0}, 	
    {0xC846, 0x0103, WORD_LEN, 0}, 	
    {0xC848, 0x0103, WORD_LEN, 0}, 	
    {0xC84A, 0x0103, WORD_LEN, 0}, 	
    {0xC84C, 0x0080, WORD_LEN, 0}, 	
    {0xC84E, 0x0001, WORD_LEN, 0}, 	
    {0xC850, 0x03,   BYTE_LEN, 0}, 	
    {0xC851, 0x00,   BYTE_LEN, 0}, 	
    {0xC852, 0x019C, WORD_LEN, 0}, 	
    {0xC854, 0x0732, WORD_LEN, 0}, 	
    {0xC858, 0x0002, WORD_LEN, 0}, 	
    {0xC85A, 0x0001, WORD_LEN, 0}, 	
    {0xC85C, 0x0423, WORD_LEN, 0}, 	
    {0xC85E, 0xFFFF, WORD_LEN, 0}, 	
    {0xC860, 0x0423, WORD_LEN, 0}, 	
    {0xC862, 0x1194, WORD_LEN, 0}, 	
    {0xC864, 0xFFFE, WORD_LEN, 0}, 	
    {0xC866, 0x7F5A, WORD_LEN, 0}, 	
    {0xC868, 0x0423, WORD_LEN, 0}, 	
    {0xC86A, 0x1194, WORD_LEN, 0}, 	
    {0xC86C, 0x0518, WORD_LEN, 0}, 	
    {0xC86E, 0x03D4, WORD_LEN, 0}, 	
    {0xC870, 0x0014, WORD_LEN, 0}, 	
    {0xC8B8, 0x0004, WORD_LEN, 0}, 	
    {0xC8AE, 0x0001, WORD_LEN, 0}, 	
    {0xC8AA, 0x0280, WORD_LEN, 0}, 	
    {0xC8AC, 0x01E0, WORD_LEN, 0}, 	
    
    
    {0xC872, 0x0010, WORD_LEN, 0}, 	
    {0xC874, 0x001C, WORD_LEN, 0}, 	
    {0xC876, 0x07AF, WORD_LEN, 0}, 	
    {0xC878, 0x0A43, WORD_LEN, 0}, 	
    {0xC87A, 0x0001, WORD_LEN, 0}, 	
    {0xC87C, 0x0101, WORD_LEN, 0}, 	
    {0xC87E, 0x0101, WORD_LEN, 0}, 	
    {0xC880, 0x0101, WORD_LEN, 0}, 	
    {0xC882, 0x0101, WORD_LEN, 0}, 	
    {0xC884, 0x0064, WORD_LEN, 0}, 	
    {0xC886, 0x0000, WORD_LEN, 0}, 	
    {0xC888, 0x03,   BYTE_LEN, 0},  
    {0xC889, 0x00,   BYTE_LEN, 0},  
    {0xC88A, 0x009C, WORD_LEN, 0}, 	
    {0xC88C, 0x034A, WORD_LEN, 0}, 	
    {0xC890, 0x0002, WORD_LEN, 0}, 	
    {0xC892, 0x0001, WORD_LEN, 0}, 	
    {0xC894, 0x082F, WORD_LEN, 0}, 	
    {0xC896, 0xFFFF, WORD_LEN, 0}, 	
    {0xC898, 0x082F, WORD_LEN, 0}, 	
    {0xC89C, 0xFFFE, WORD_LEN, 0}, 	
    {0xC89E, 0x7F5A, WORD_LEN, 0}, 	
    {0xC8A0, 0x082F, WORD_LEN, 0}, 	
    {0xC8A4, 0x0A28, WORD_LEN, 0}, 	
    {0xC8A6, 0x07A0, WORD_LEN, 0}, 	
    {0xC8A8, 0x0021, WORD_LEN, 0}, 	
    {0xC8C4, 0x0001, WORD_LEN, 0}, 	
    {0xC8C0, 0x0A20, WORD_LEN, 0}, 	
    {0xC8C2, 0x0798, WORD_LEN, 0}, 	
    {0xC89A, 0x2500, WORD_LEN, 0}, 	
    {0xC8A2, 0x2328, WORD_LEN, 0}, 	
    {0xC8CE, 0x0014, WORD_LEN, 0}, 	
    
    
    {0xA018, 0x00CC, WORD_LEN, 0}, 	
    {0xA01A, 0x0061, WORD_LEN, 0}, 	
    {0xA01C, 0x00A9, WORD_LEN, 0}, 	
    {0xA01E, 0x0050, WORD_LEN, 0}, 	
    {0xA010, 0x00B8, WORD_LEN, 0}, 	
    {0xA012, 0x00D6, WORD_LEN, 0}, 	
    {0xA014, 0x0095, WORD_LEN, 0}, 	
    {0xA016, 0x00B3, WORD_LEN, 0}, 	
    
    
    {0x0982, 0x0000, WORD_LEN, 0}, 	
    {0x098A, 0x086C, WORD_LEN, 0}, 	
    {0x0990, 0xC0F1, WORD_LEN, 0},
    {0x0992, 0xC5E1, WORD_LEN, 0},
    {0x0994, 0x246A, WORD_LEN, 0},
    {0x0996, 0x1280, WORD_LEN, 0},
    {0x0998, 0xC4E1, WORD_LEN, 0},
    {0x099A, 0xD20F, WORD_LEN, 0},
    {0x099C, 0x2069, WORD_LEN, 0},
    {0x099E, 0x0000, WORD_LEN, 0},
    {0x098A, 0x087C, WORD_LEN, 0}, 	
    {0x0990, 0x6A62, WORD_LEN, 0},
    {0x0992, 0x1303, WORD_LEN, 0},
    {0x0994, 0x0084, WORD_LEN, 0},
    {0x0996, 0x1734, WORD_LEN, 0},
    {0x0998, 0x7005, WORD_LEN, 0},
    {0x099A, 0xD801, WORD_LEN, 0},
    {0x099C, 0x8A41, WORD_LEN, 0},
    {0x099E, 0xD900, WORD_LEN, 0},
    {0x098A, 0x088C, WORD_LEN, 0}, 	
    {0x0990, 0x0D5A, WORD_LEN, 0},
    {0x0992, 0x0664, WORD_LEN, 0},
    {0x0994, 0x8B61, WORD_LEN, 0},
    {0x0996, 0xE80B, WORD_LEN, 0},
    {0x0998, 0x000D, WORD_LEN, 0},
    {0x099A, 0x0020, WORD_LEN, 0},
    {0x099C, 0xD508, WORD_LEN, 0},
    {0x099E, 0x1504, WORD_LEN, 0},
    {0x098A, 0x089C, WORD_LEN, 0}, 	
    {0x0990, 0x1400, WORD_LEN, 0},
    {0x0992, 0x7840, WORD_LEN, 0},
    {0x0994, 0xD007, WORD_LEN, 0},
    {0x0996, 0x0DFB, WORD_LEN, 0},
    {0x0998, 0x9004, WORD_LEN, 0},
    {0x099A, 0xC4C1, WORD_LEN, 0},
    {0x099C, 0x2029, WORD_LEN, 0},
    {0x099E, 0x0300, WORD_LEN, 0},
    {0x098A, 0x08AC, WORD_LEN, 0}, 	
    {0x0990, 0x0219, WORD_LEN, 0},
    {0x0992, 0x06C4, WORD_LEN, 0},
    {0x0994, 0xFF80, WORD_LEN, 0},
    {0x0996, 0x08C8, WORD_LEN, 0},
    {0x0998, 0xFF80, WORD_LEN, 0},
    {0x099A, 0x086C, WORD_LEN, 0},
    {0x099C, 0xFF80, WORD_LEN, 0},
    {0x099E, 0x08C0, WORD_LEN, 0},
    {0x098A, 0x08BC, WORD_LEN, 0}, 	
    {0x0990, 0xFF80, WORD_LEN, 0},
    {0x0992, 0x08C8, WORD_LEN, 0},
    {0x0994, 0xFF80, WORD_LEN, 0},
    {0x0996, 0x0B50, WORD_LEN, 0},
    {0x0998, 0xFF80, WORD_LEN, 0},
    {0x099A, 0x08D0, WORD_LEN, 0},
    {0x099C, 0x0002, WORD_LEN, 0},
    {0x099E, 0x0003, WORD_LEN, 0},
    {0x098A, 0x08CC, WORD_LEN, 0}, 	
    {0x0990, 0x0000, WORD_LEN, 0},
    {0x0992, 0x0000, WORD_LEN, 0},
    {0x0994, 0xC0F1, WORD_LEN, 0},
    {0x0996, 0x0982, WORD_LEN, 0},
    {0x0998, 0x06E4, WORD_LEN, 0},
    {0x099A, 0xDA08, WORD_LEN, 0},
    {0x099C, 0xD16B, WORD_LEN, 0},
    {0x099E, 0x8900, WORD_LEN, 0},
    {0x098A, 0x08DC, WORD_LEN, 0}, 	
    {0x0990, 0x19B0, WORD_LEN, 0},
    {0x0992, 0x0082, WORD_LEN, 0},
    {0x0994, 0xDB51, WORD_LEN, 0},
    {0x0996, 0x19B2, WORD_LEN, 0},
    {0x0998, 0x00C2, WORD_LEN, 0},
    {0x099A, 0xB8A6, WORD_LEN, 0},
    {0x099C, 0xA900, WORD_LEN, 0},
    {0x099E, 0xD8F0, WORD_LEN, 0},
    {0x098A, 0x08EC, WORD_LEN, 0}, 	
    {0x0990, 0x19B1, WORD_LEN, 0},
    {0x0992, 0x0002, WORD_LEN, 0},
    {0x0994, 0x19B5, WORD_LEN, 0},
    {0x0996, 0x0002, WORD_LEN, 0},
    {0x0998, 0xD855, WORD_LEN, 0},
    {0x099A, 0x19B6, WORD_LEN, 0},
    {0x099C, 0x0002, WORD_LEN, 0},
    {0x099E, 0xD856, WORD_LEN, 0},
    {0x098A, 0x08FC, WORD_LEN, 0}, 	
    {0x0990, 0x19B7, WORD_LEN, 0},
    {0x0992, 0x0002, WORD_LEN, 0},
    {0x0994, 0xD896, WORD_LEN, 0},
    {0x0996, 0x19B8, WORD_LEN, 0},
    {0x0998, 0x0004, WORD_LEN, 0},
    {0x099A, 0xD814, WORD_LEN, 0},
    {0x099C, 0x19BA, WORD_LEN, 0},
    {0x099E, 0x0004, WORD_LEN, 0},
    {0x098A, 0x090C, WORD_LEN, 0}, 	
    {0x0990, 0xD55F, WORD_LEN, 0},
    {0x0992, 0xD805, WORD_LEN, 0},
    {0x0994, 0xDB52, WORD_LEN, 0},
    {0x0996, 0xB111, WORD_LEN, 0},
    {0x0998, 0x19B3, WORD_LEN, 0},
    {0x099A, 0x00C2, WORD_LEN, 0},
    {0x099C, 0x19B4, WORD_LEN, 0},
    {0x099E, 0x0082, WORD_LEN, 0},
    {0x098A, 0x091C, WORD_LEN, 0}, 	
    {0x0990, 0xD15C, WORD_LEN, 0},
    {0x0992, 0x76A9, WORD_LEN, 0},
    {0x0994, 0x70C9, WORD_LEN, 0},
    {0x0996, 0x0ED2, WORD_LEN, 0},
    {0x0998, 0x06A4, WORD_LEN, 0},
    {0x099A, 0xDA2C, WORD_LEN, 0},
    {0x099C, 0xD05A, WORD_LEN, 0},
    {0x099E, 0xA503, WORD_LEN, 0},
    {0x098A, 0x092C, WORD_LEN, 0}, 	
    {0x0990, 0xD05A, WORD_LEN, 0},
    {0x0992, 0x0191, WORD_LEN, 0},
    {0x0994, 0x06E4, WORD_LEN, 0},
    {0x0996, 0xA0C0, WORD_LEN, 0},
    {0x0998, 0xC0F1, WORD_LEN, 0},
    {0x099A, 0x091E, WORD_LEN, 0},
    {0x099C, 0x06C4, WORD_LEN, 0},
    {0x099E, 0xD653, WORD_LEN, 0},
    {0x098A, 0x093C, WORD_LEN, 0}, 	
    {0x0990, 0x8E01, WORD_LEN, 0},
    {0x0992, 0xB8A4, WORD_LEN, 0},
    {0x0994, 0xAE01, WORD_LEN, 0},
    {0x0996, 0x8E09, WORD_LEN, 0},
    {0x0998, 0xB8E0, WORD_LEN, 0},
    {0x099A, 0xF29B, WORD_LEN, 0},
    {0x099C, 0xD554, WORD_LEN, 0},
    {0x099E, 0x153A, WORD_LEN, 0},
    {0x098A, 0x094C, WORD_LEN, 0}, 	
    {0x0990, 0x1080, WORD_LEN, 0},
    {0x0992, 0x153B, WORD_LEN, 0},
    {0x0994, 0x1081, WORD_LEN, 0},
    {0x0996, 0xB808, WORD_LEN, 0},
    {0x0998, 0x7825, WORD_LEN, 0},
    {0x099A, 0x16B8, WORD_LEN, 0},
    {0x099C, 0x1101, WORD_LEN, 0},
    {0x099E, 0x092D, WORD_LEN, 0},
    {0x098A, 0x095C, WORD_LEN, 0}, 	
    {0x0990, 0x0003, WORD_LEN, 0},
    {0x0992, 0x16B0, WORD_LEN, 0},
    {0x0994, 0x1082, WORD_LEN, 0},
    {0x0996, 0x1E3C, WORD_LEN, 0},
    {0x0998, 0x1082, WORD_LEN, 0},
    {0x099A, 0x16B1, WORD_LEN, 0},
    {0x099C, 0x1082, WORD_LEN, 0},
    {0x099E, 0x1E3D, WORD_LEN, 0},
    {0x098A, 0x096C, WORD_LEN, 0}, 	
    {0x0990, 0x1082, WORD_LEN, 0},
    {0x0992, 0x16B4, WORD_LEN, 0},
    {0x0994, 0x1082, WORD_LEN, 0},
    {0x0996, 0x1E3E, WORD_LEN, 0},
    {0x0998, 0x1082, WORD_LEN, 0},
    {0x099A, 0x16B5, WORD_LEN, 0},
    {0x099C, 0x1082, WORD_LEN, 0},
    {0x099E, 0x1E3F, WORD_LEN, 0},
    {0x098A, 0x097C, WORD_LEN, 0}, 	
    {0x0990, 0x1082, WORD_LEN, 0},
    {0x0992, 0x8E40, WORD_LEN, 0},
    {0x0994, 0xBAA6, WORD_LEN, 0},
    {0x0996, 0xAE40, WORD_LEN, 0},
    {0x0998, 0x098F, WORD_LEN, 0},
    {0x099A, 0x0022, WORD_LEN, 0},
    {0x099C, 0x16BA, WORD_LEN, 0},
    {0x099E, 0x1102, WORD_LEN, 0},
    {0x098A, 0x098C, WORD_LEN, 0}, 	
    {0x0990, 0x0A87, WORD_LEN, 0},
    {0x0992, 0x0003, WORD_LEN, 0},
    {0x0994, 0x16B2, WORD_LEN, 0},
    {0x0996, 0x1084, WORD_LEN, 0},
    {0x0998, 0x0A52, WORD_LEN, 0},
    {0x099A, 0x06A4, WORD_LEN, 0},
    {0x099C, 0x16B0, WORD_LEN, 0},
    {0x099E, 0x1083, WORD_LEN, 0},
    {0x098A, 0x099C, WORD_LEN, 0}, 	
    {0x0990, 0x1E3C, WORD_LEN, 0},
    {0x0992, 0x1002, WORD_LEN, 0},
    {0x0994, 0x153A, WORD_LEN, 0},
    {0x0996, 0x1080, WORD_LEN, 0},
    {0x0998, 0x153B, WORD_LEN, 0},
    {0x099A, 0x1081, WORD_LEN, 0},
    {0x099C, 0x16B3, WORD_LEN, 0},
    {0x099E, 0x1084, WORD_LEN, 0},
    {0x098A, 0x09AC, WORD_LEN, 0}, 	
    {0x0990, 0xB808, WORD_LEN, 0},
    {0x0992, 0x7825, WORD_LEN, 0},
    {0x0994, 0x16B8, WORD_LEN, 0},
    {0x0996, 0x1101, WORD_LEN, 0},
    {0x0998, 0x16BA, WORD_LEN, 0},
    {0x099A, 0x1102, WORD_LEN, 0},
    {0x099C, 0x0A2E, WORD_LEN, 0},
    {0x099E, 0x06A4, WORD_LEN, 0},
    {0x098A, 0x09BC, WORD_LEN, 0}, 	
    {0x0990, 0x16B1, WORD_LEN, 0},
    {0x0992, 0x1083, WORD_LEN, 0},
    {0x0994, 0x1E3D, WORD_LEN, 0},
    {0x0996, 0x1002, WORD_LEN, 0},
    {0x0998, 0x153A, WORD_LEN, 0},
    {0x099A, 0x1080, WORD_LEN, 0},
    {0x099C, 0x153B, WORD_LEN, 0},
    {0x099E, 0x1081, WORD_LEN, 0},
    {0x098A, 0x09CC, WORD_LEN, 0}, 	
    {0x0990, 0x16B6, WORD_LEN, 0},
    {0x0992, 0x1084, WORD_LEN, 0},
    {0x0994, 0xB808, WORD_LEN, 0},
    {0x0996, 0x7825, WORD_LEN, 0},
    {0x0998, 0x16B8, WORD_LEN, 0},
    {0x099A, 0x1101, WORD_LEN, 0},
    {0x099C, 0x16BA, WORD_LEN, 0},
    {0x099E, 0x1102, WORD_LEN, 0},
    {0x098A, 0x09DC, WORD_LEN, 0}, 	
    {0x0990, 0x0A0A, WORD_LEN, 0},
    {0x0992, 0x06A4, WORD_LEN, 0},
    {0x0994, 0x16B4, WORD_LEN, 0},
    {0x0996, 0x1083, WORD_LEN, 0},
    {0x0998, 0x1E3E, WORD_LEN, 0},
    {0x099A, 0x1002, WORD_LEN, 0},
    {0x099C, 0x153A, WORD_LEN, 0},
    {0x099E, 0x1080, WORD_LEN, 0},
    {0x098A, 0x09EC, WORD_LEN, 0}, 	
    {0x0990, 0x153B, WORD_LEN, 0},
    {0x0992, 0x1081, WORD_LEN, 0},
    {0x0994, 0x16B7, WORD_LEN, 0},
    {0x0996, 0x1084, WORD_LEN, 0},
    {0x0998, 0xB808, WORD_LEN, 0},
    {0x099A, 0x7825, WORD_LEN, 0},
    {0x099C, 0x16B8, WORD_LEN, 0},
    {0x099E, 0x1101, WORD_LEN, 0},
    {0x098A, 0x09FC, WORD_LEN, 0}, 	
    {0x0990, 0x16BA, WORD_LEN, 0},
    {0x0992, 0x1102, WORD_LEN, 0},
    {0x0994, 0x09E6, WORD_LEN, 0},
    {0x0996, 0x06A4, WORD_LEN, 0},
    {0x0998, 0x16B5, WORD_LEN, 0},
    {0x099A, 0x1083, WORD_LEN, 0},
    {0x099C, 0x1E3F, WORD_LEN, 0},
    {0x099E, 0x1002, WORD_LEN, 0},
    {0x098A, 0x0A0C, WORD_LEN, 0}, 	
    {0x0990, 0x8E00, WORD_LEN, 0},
    {0x0992, 0xB8A6, WORD_LEN, 0},
    {0x0994, 0xAE00, WORD_LEN, 0},
    {0x0996, 0x153A, WORD_LEN, 0},
    {0x0998, 0x1081, WORD_LEN, 0},
    {0x099A, 0x153B, WORD_LEN, 0},
    {0x099C, 0x1080, WORD_LEN, 0},
    {0x099E, 0xB908, WORD_LEN, 0},
    {0x098A, 0x0A1C, WORD_LEN, 0}, 	
    {0x0990, 0x7905, WORD_LEN, 0},
    {0x0992, 0x16BA, WORD_LEN, 0},
    {0x0994, 0x1100, WORD_LEN, 0},
    {0x0996, 0x085B, WORD_LEN, 0},
    {0x0998, 0x0042, WORD_LEN, 0},
    {0x099A, 0xD01E, WORD_LEN, 0},
    {0x099C, 0x9E31, WORD_LEN, 0},
    {0x099E, 0x904D, WORD_LEN, 0},
    {0x098A, 0x0A2C, WORD_LEN, 0}, 	
    {0x0990, 0x0A2B, WORD_LEN, 0},
    {0x0992, 0x0063, WORD_LEN, 0},
    {0x0994, 0x8E00, WORD_LEN, 0},
    {0x0996, 0x16B0, WORD_LEN, 0},
    {0x0998, 0x1081, WORD_LEN, 0},
    {0x099A, 0x1E3C, WORD_LEN, 0},
    {0x099C, 0x1042, WORD_LEN, 0},
    {0x099E, 0x16B1, WORD_LEN, 0},
    {0x098A, 0x0A3C, WORD_LEN, 0}, 	
    {0x0990, 0x1081, WORD_LEN, 0},
    {0x0992, 0x1E3D, WORD_LEN, 0},
    {0x0994, 0x1042, WORD_LEN, 0},
    {0x0996, 0x16B4, WORD_LEN, 0},
    {0x0998, 0x1081, WORD_LEN, 0},
    {0x099A, 0x1E3E, WORD_LEN, 0},
    {0x099C, 0x1042, WORD_LEN, 0},
    {0x099E, 0x16B5, WORD_LEN, 0},
    {0x098A, 0x0A4C, WORD_LEN, 0}, 	
    {0x0990, 0x1081, WORD_LEN, 0},
    {0x0992, 0x1E3F, WORD_LEN, 0},
    {0x0994, 0x1042, WORD_LEN, 0},
    {0x0996, 0xB886, WORD_LEN, 0},
    {0x0998, 0xF012, WORD_LEN, 0},
    {0x099A, 0x16B2, WORD_LEN, 0},
    {0x099C, 0x1081, WORD_LEN, 0},
    {0x099E, 0xB8A6, WORD_LEN, 0},
    {0x098A, 0x0A5C, WORD_LEN, 0}, 	
    {0x0990, 0x1E3C, WORD_LEN, 0},
    {0x0992, 0x1042, WORD_LEN, 0},
    {0x0994, 0x16B3, WORD_LEN, 0},
    {0x0996, 0x1081, WORD_LEN, 0},
    {0x0998, 0x1E3D, WORD_LEN, 0},
    {0x099A, 0x1042, WORD_LEN, 0},
    {0x099C, 0x16B6, WORD_LEN, 0},
    {0x099E, 0x1081, WORD_LEN, 0},
    {0x098A, 0x0A6C, WORD_LEN, 0}, 	
    {0x0990, 0x1E3E, WORD_LEN, 0},
    {0x0992, 0x1042, WORD_LEN, 0},
    {0x0994, 0x16B7, WORD_LEN, 0},
    {0x0996, 0x1081, WORD_LEN, 0},
    {0x0998, 0x1E3F, WORD_LEN, 0},
    {0x099A, 0x1042, WORD_LEN, 0},
    {0x099C, 0xAE00, WORD_LEN, 0},
    {0x099E, 0x08B6, WORD_LEN, 0},
    {0x098A, 0x0A7C, WORD_LEN, 0}, 	
    {0x0990, 0x01C4, WORD_LEN, 0},
    {0x0992, 0x0041, WORD_LEN, 0},
    {0x0994, 0x06C4, WORD_LEN, 0},
    {0x0996, 0x78E0, WORD_LEN, 0},
    {0x0998, 0xFF80, WORD_LEN, 0},
    {0x099A, 0x0314, WORD_LEN, 0},
    {0x099C, 0xFF80, WORD_LEN, 0},
    {0x099E, 0x0BD8, WORD_LEN, 0},
    {0x098A, 0x0A8C, WORD_LEN, 0}, 	
    {0x0990, 0x0000, WORD_LEN, 0},
    {0x0992, 0xF444, WORD_LEN, 0},
    {0x0994, 0xFF80, WORD_LEN, 0},
    {0x0996, 0x0934, WORD_LEN, 0},
    {0x0998, 0x8000, WORD_LEN, 0},
    {0x099A, 0x009C, WORD_LEN, 0},
    {0x099C, 0xFF80, WORD_LEN, 0},
    {0x099E, 0x0250, WORD_LEN, 0},
    {0x098A, 0x0A9C, WORD_LEN, 0}, 	
    {0x0990, 0xFF80, WORD_LEN, 0},
    {0x0992, 0x050C, WORD_LEN, 0},
    {0x0994, 0xC0F1, WORD_LEN, 0},
    {0x0996, 0x0FA2, WORD_LEN, 0},
    {0x0998, 0x0684, WORD_LEN, 0},
    {0x099A, 0xD633, WORD_LEN, 0},
    {0x099C, 0x7708, WORD_LEN, 0},
    {0x099E, 0x8E01, WORD_LEN, 0},
    {0x098A, 0x0AAC, WORD_LEN, 0}, 	
    {0x0990, 0x1604, WORD_LEN, 0},
    {0x0992, 0x1091, WORD_LEN, 0},
    {0x0994, 0x2046, WORD_LEN, 0},
    {0x0996, 0x00C1, WORD_LEN, 0},
    {0x0998, 0x202F, WORD_LEN, 0},
    {0x099A, 0x2047, WORD_LEN, 0},
    {0x099C, 0xAE21, WORD_LEN, 0},
    {0x099E, 0x0F8F, WORD_LEN, 0},
    {0x098A, 0x0ABC, WORD_LEN, 0}, 	
    {0x0990, 0x1440, WORD_LEN, 0},
    {0x0992, 0x8EAA, WORD_LEN, 0},
    {0x0994, 0x8E0B, WORD_LEN, 0},
    {0x0996, 0x224A, WORD_LEN, 0},
    {0x0998, 0x2040, WORD_LEN, 0},
    {0x099A, 0x8E2D, WORD_LEN, 0},
    {0x099C, 0xBD08, WORD_LEN, 0},
    {0x099E, 0x7D05, WORD_LEN, 0},
    {0x098A, 0x0ACC, WORD_LEN, 0}, 	
    {0x0990, 0x8E0C, WORD_LEN, 0},
    {0x0992, 0xB808, WORD_LEN, 0},
    {0x0994, 0x7825, WORD_LEN, 0},
    {0x0996, 0x7510, WORD_LEN, 0},
    {0x0998, 0x22C2, WORD_LEN, 0},
    {0x099A, 0x248C, WORD_LEN, 0},
    {0x099C, 0x081D, WORD_LEN, 0},
    {0x099E, 0x0363, WORD_LEN, 0},
    {0x098A, 0x0ADC, WORD_LEN, 0}, 	
    {0x0990, 0xD9FF, WORD_LEN, 0},
    {0x0992, 0x2502, WORD_LEN, 0},
    {0x0994, 0x1002, WORD_LEN, 0},
    {0x0996, 0x2A05, WORD_LEN, 0},
    {0x0998, 0x03FE, WORD_LEN, 0},
    {0x099A, 0x0842, WORD_LEN, 0},
    {0x099C, 0x06E4, WORD_LEN, 0},
    {0x099E, 0x702F, WORD_LEN, 0},
    {0x098A, 0x0AEC, WORD_LEN, 0}, 	
    {0x0990, 0x7810, WORD_LEN, 0},
    {0x0992, 0x7D02, WORD_LEN, 0},
    {0x0994, 0x7DB0, WORD_LEN, 0},
    {0x0996, 0xF00B, WORD_LEN, 0},
    {0x0998, 0x78A2, WORD_LEN, 0},
    {0x099A, 0x2805, WORD_LEN, 0},
    {0x099C, 0x03FE, WORD_LEN, 0},
    {0x099E, 0x082E, WORD_LEN, 0},
    {0x098A, 0x0AFC, WORD_LEN, 0}, 	
    {0x0990, 0x06E4, WORD_LEN, 0},
    {0x0992, 0x702F, WORD_LEN, 0},
    {0x0994, 0x7810, WORD_LEN, 0},
    {0x0996, 0x651D, WORD_LEN, 0},
    {0x0998, 0x7DB0, WORD_LEN, 0},
    {0x099A, 0x7DAF, WORD_LEN, 0},
    {0x099C, 0x8E08, WORD_LEN, 0},
    {0x099E, 0xBD06, WORD_LEN, 0},
    {0x098A, 0x0B0C, WORD_LEN, 0}, 	
    {0x0990, 0xD11A, WORD_LEN, 0},
    {0x0992, 0xB8C3, WORD_LEN, 0},
    {0x0994, 0x78A5, WORD_LEN, 0},
    {0x0996, 0xB88F, WORD_LEN, 0},
    {0x0998, 0x1908, WORD_LEN, 0},
    {0x099A, 0x0024, WORD_LEN, 0},
    {0x099C, 0x2841, WORD_LEN, 0},
    {0x099E, 0x0201, WORD_LEN, 0},
    {0x098A, 0x0B1C, WORD_LEN, 0}, 	
    {0x0990, 0x1E26, WORD_LEN, 0},
    {0x0992, 0x1042, WORD_LEN, 0},
    {0x0994, 0x0F15, WORD_LEN, 0},
    {0x0996, 0x1463, WORD_LEN, 0},
    {0x0998, 0x1E27, WORD_LEN, 0},
    {0x099A, 0x1002, WORD_LEN, 0},
    {0x099C, 0x224C, WORD_LEN, 0},
    {0x099E, 0xA000, WORD_LEN, 0},
    {0x098A, 0x0B2C, WORD_LEN, 0}, 	
    {0x0990, 0x224A, WORD_LEN, 0},
    {0x0992, 0x2040, WORD_LEN, 0},
    {0x0994, 0x22C2, WORD_LEN, 0},
    {0x0996, 0x2482, WORD_LEN, 0},
    {0x0998, 0x204F, WORD_LEN, 0},
    {0x099A, 0x2040, WORD_LEN, 0},
    {0x099C, 0x224C, WORD_LEN, 0},
    {0x099E, 0xA000, WORD_LEN, 0},
    {0x098A, 0x0B3C, WORD_LEN, 0}, 	
    {0x0990, 0xB8A2, WORD_LEN, 0},
    {0x0992, 0xF204, WORD_LEN, 0},
    {0x0994, 0x2045, WORD_LEN, 0},
    {0x0996, 0x2180, WORD_LEN, 0},
    {0x0998, 0xAE01, WORD_LEN, 0},
    {0x099A, 0x0BCA, WORD_LEN, 0},
    {0x099C, 0xFFE3, WORD_LEN, 0},
    {0x099E, 0x70E9, WORD_LEN, 0},
    {0x098A, 0x0B4C, WORD_LEN, 0}, 	
    {0x0990, 0x0751, WORD_LEN, 0},
    {0x0992, 0x0684, WORD_LEN, 0},
    {0x0994, 0xC0F1, WORD_LEN, 0},
    {0x0996, 0xD00A, WORD_LEN, 0},
    {0x0998, 0xD20A, WORD_LEN, 0},
    {0x099A, 0xD10B, WORD_LEN, 0},
    {0x099C, 0xA040, WORD_LEN, 0},
    {0x099E, 0x890C, WORD_LEN, 0},
    {0x098A, 0x0B5C, WORD_LEN, 0}, 	
    {0x0990, 0x080F, WORD_LEN, 0},
    {0x0992, 0x01DE, WORD_LEN, 0},
    {0x0994, 0xB8A7, WORD_LEN, 0},
    {0x0996, 0x8243, WORD_LEN, 0},
    {0x0998, 0xA90C, WORD_LEN, 0},
    {0x099A, 0x7A60, WORD_LEN, 0},
    {0x099C, 0x890C, WORD_LEN, 0},
    {0x099E, 0xC0D1, WORD_LEN, 0},
    {0x098A, 0x0B6C, WORD_LEN, 0}, 	
    {0x0990, 0x7EE0, WORD_LEN, 0},
    {0x0992, 0x78E0, WORD_LEN, 0},
    {0x0994, 0xFF80, WORD_LEN, 0},
    {0x0996, 0x0158, WORD_LEN, 0},
    {0x0998, 0xFF00, WORD_LEN, 0},
    {0x099A, 0x0618, WORD_LEN, 0},
    {0x099C, 0x8000, WORD_LEN, 0},
    {0x099E, 0x0008, WORD_LEN, 0},
    {0x098A, 0x0B7C, WORD_LEN, 0}, 	
    {0x0990, 0xFF80, WORD_LEN, 0},
    {0x0992, 0x0BC8, WORD_LEN, 0},
    {0x0994, 0xFF80, WORD_LEN, 0},
    {0x0996, 0x0174, WORD_LEN, 0},
    {0x0998, 0xE280, WORD_LEN, 0},
    {0x099A, 0x24CA, WORD_LEN, 0},
    {0x099C, 0x7082, WORD_LEN, 0},
    {0x099E, 0x78E0, WORD_LEN, 0},
    {0x098A, 0x0B8C, WORD_LEN, 0}, 	
    {0x0990, 0x20E8, WORD_LEN, 0},
    {0x0992, 0x01A2, WORD_LEN, 0},
    {0x0994, 0x1002, WORD_LEN, 0},
    {0x0996, 0x0D02, WORD_LEN, 0},
    {0x0998, 0x1902, WORD_LEN, 0},
    {0x099A, 0x0094, WORD_LEN, 0},
    {0x099C, 0x7FE0, WORD_LEN, 0},
    {0x099E, 0x7028, WORD_LEN, 0},
    {0x098A, 0x0B9C, WORD_LEN, 0}, 	
    {0x0990, 0x7308, WORD_LEN, 0},
    {0x0992, 0x1000, WORD_LEN, 0},
    {0x0994, 0x0900, WORD_LEN, 0},
    {0x0996, 0x7904, WORD_LEN, 0},
    {0x0998, 0x7947, WORD_LEN, 0},
    {0x099A, 0x1B00, WORD_LEN, 0},
    {0x099C, 0x0064, WORD_LEN, 0},
    {0x099E, 0x7EE0, WORD_LEN, 0},
    {0x098A, 0x0BAC, WORD_LEN, 0}, 	
    {0x0990, 0xE280, WORD_LEN, 0},
    {0x0992, 0x24CA, WORD_LEN, 0},
    {0x0994, 0x7082, WORD_LEN, 0},
    {0x0996, 0x78E0, WORD_LEN, 0},
    {0x0998, 0x20E8, WORD_LEN, 0},
    {0x099A, 0x01A2, WORD_LEN, 0},
    {0x099C, 0x1102, WORD_LEN, 0},
    {0x099E, 0x0502, WORD_LEN, 0},
    {0x098A, 0x0BBC, WORD_LEN, 0}, 	
    {0x0990, 0x1802, WORD_LEN, 0},
    {0x0992, 0x00B4, WORD_LEN, 0},
    {0x0994, 0x7FE0, WORD_LEN, 0},
    {0x0996, 0x7028, WORD_LEN, 0},
    {0x0998, 0x0000, WORD_LEN, 0},
    {0x099A, 0x0000, WORD_LEN, 0},
    {0x099C, 0xFF80, WORD_LEN, 0},
    {0x099E, 0x0B50, WORD_LEN, 0},
    {0x098A, 0x0BCC, WORD_LEN, 0}, 	
    {0x0990, 0xFF80, WORD_LEN, 0},
    {0x0992, 0x0AA0, WORD_LEN, 0},
    {0x0994, 0x0000, WORD_LEN, 0},
    {0x0996, 0x08DC, WORD_LEN, 0},
    {0x0998, 0x0000, WORD_LEN, 0},
    {0x099A, 0x0998, WORD_LEN, 0},                            
    {0x098E, 0x0016, WORD_LEN, 0}, 	
    {0x8016, 0x086C, WORD_LEN, 0}, 	
    {0x8002, 0x0001, WORD_LEN, 100},
                                
    
    {0x30D4, 0x9080, WORD_LEN, 0}, 	
    {0x316E, 0xC400, WORD_LEN, 0}, 	
    {0x305E, 0x10A0, WORD_LEN, 0}, 	
    {0x3E00, 0x0010, WORD_LEN, 0}, 	
    {0x3E02, 0xED02, WORD_LEN, 0}, 	
    {0x3E04, 0xC88C, WORD_LEN, 0}, 	
    {0x3E06, 0xC88C, WORD_LEN, 0}, 	
    {0x3E08, 0x700A, WORD_LEN, 0}, 	
    {0x3E0A, 0x701E, WORD_LEN, 0}, 	
    {0x3E0C, 0x00FF, WORD_LEN, 0}, 	
    {0x3E0E, 0x00FF, WORD_LEN, 0}, 	
    {0x3E10, 0x00FF, WORD_LEN, 0}, 	
    {0x3E12, 0x0000, WORD_LEN, 0}, 	
    {0x3E14, 0xC78C, WORD_LEN, 0}, 	
    {0x3E16, 0x6E06, WORD_LEN, 0}, 	
    {0x3E18, 0xA58C, WORD_LEN, 0}, 	
    {0x3E1A, 0xA58E, WORD_LEN, 0}, 	
    {0x3E1C, 0xA58E, WORD_LEN, 0}, 	
    {0x3E1E, 0xC0D0, WORD_LEN, 0}, 	
    {0x3E20, 0xEB00, WORD_LEN, 0}, 	
    {0x3E22, 0x00FF, WORD_LEN, 0}, 	
    {0x3E24, 0xEB02, WORD_LEN, 0}, 	
    {0x3E26, 0xEA02, WORD_LEN, 0}, 	
    {0x3E28, 0xEB0A, WORD_LEN, 0}, 	
    {0x3E2A, 0xEC01, WORD_LEN, 0}, 	
    {0x3E2C, 0xEB01, WORD_LEN, 0}, 	
    {0x3E2E, 0x00FF, WORD_LEN, 0}, 	
    {0x3E30, 0x00F3, WORD_LEN, 0}, 	
    {0x3E32, 0x3DFA, WORD_LEN, 0}, 	
    {0x3E34, 0x00FF, WORD_LEN, 0}, 	
    {0x3E36, 0x00F3, WORD_LEN, 0}, 	
    {0x3E38, 0x0000, WORD_LEN, 0}, 	
    {0x3E3A, 0xF802, WORD_LEN, 0}, 	
    {0x3E3C, 0x0FFF, WORD_LEN, 0}, 	
    {0x3E3E, 0xEA10, WORD_LEN, 0}, 	
    {0x3E40, 0xEB05, WORD_LEN, 0}, 	
    {0x3E42, 0xE5C8, WORD_LEN, 0}, 	
    {0x3E44, 0xE5C8, WORD_LEN, 0}, 	
    {0x3E46, 0x8C70, WORD_LEN, 0}, 	
    {0x3E48, 0x8C71, WORD_LEN, 0}, 	
    {0x3E4A, 0x00FF, WORD_LEN, 0}, 	
    {0x3E4C, 0x00FF, WORD_LEN, 0}, 	
    {0x3E4E, 0x00FF, WORD_LEN, 0}, 	
    {0x3E50, 0xE38D, WORD_LEN, 0}, 	
    {0x3E52, 0x8B0A, WORD_LEN, 0}, 	
    {0x3E58, 0xEB0A, WORD_LEN, 0}, 	
    {0x3E5C, 0x0A00, WORD_LEN, 0}, 	
    {0x3E5E, 0x00FF, WORD_LEN, 0}, 	
    {0x3E60, 0x00FF, WORD_LEN, 0}, 	
    {0x3E90, 0x3C01, WORD_LEN, 0}, 	
    {0x3E92, 0x00FF, WORD_LEN, 0}, 	
    {0x3E94, 0x00FF, WORD_LEN, 0}, 	
    {0x3E96, 0x3C00, WORD_LEN, 0}, 	
    {0x3E98, 0x3C00, WORD_LEN, 0}, 	
    {0x3E9A, 0x3C00, WORD_LEN, 0}, 	
    {0x3E9C, 0xC0E0, WORD_LEN, 0}, 	
    {0x3E9E, 0x00FF, WORD_LEN, 0}, 	
    {0x3EA0, 0x0000, WORD_LEN, 0}, 	
    {0x3EA6, 0x3C00, WORD_LEN, 0}, 	
    {0x3ED8, 0x3057, WORD_LEN, 0}, 	
    {0x316C, 0xB44F, WORD_LEN, 0}, 	
    {0x316E, 0xC6FF, WORD_LEN, 0}, 	
    {0x3ED2, 0xEA0A, WORD_LEN, 0}, 	
    {0x3ED4, 0x00A3, WORD_LEN, 0}, 	
    {0x3EDC, 0x6020, WORD_LEN, 0}, 	
    {0x3EE6, 0xA541, WORD_LEN, 0}, 	
    {0x31E0, 0x0000, WORD_LEN, 0}, 	
    {0x3ED0, 0x2409, WORD_LEN, 0}, 	
    {0x3EDE, 0x0A49, WORD_LEN, 0}, 	
    {0x3EE0, 0x4910, WORD_LEN, 0}, 	
    {0x3EE2, 0x09D2, WORD_LEN, 0}, 	
    {0x30B6, 0x0006, WORD_LEN, 0}, 	
    {0x337C, 0x0006, WORD_LEN, 0}, 	
    {0x3E1A, 0xA582, WORD_LEN, 0}, 	
    {0x3E2E, 0xEC05, WORD_LEN, 0}, 	
    {0x3EE6, 0xA5C0, WORD_LEN, 0}, 	
    {0x316C, 0xB43F, WORD_LEN, 0}, 	
    {0x316E, 0xC6FF, WORD_LEN, 0}, 	
    
    
    {0x098E, 0x2C46,WORD_LEN,0}, 	
    {0xAC46, 0x01F8,WORD_LEN,0}, 	
    {0xAC48, 0xFECC,WORD_LEN,0}, 	
    {0xAC4A, 0x003D,WORD_LEN,0}, 	
    {0xAC4C, 0xFFBA,WORD_LEN,0}, 	
    {0xAC4E, 0x015E,WORD_LEN,0}, 	
    {0xAC50, 0xFFE8,WORD_LEN,0}, 	
    {0xAC52, 0xFFFD,WORD_LEN,0}, 	
    {0xAC54, 0xFF30,WORD_LEN,0}, 	
    {0xAC56, 0x01D2,WORD_LEN,0}, 	
    {0xAC58, 0x00B6,WORD_LEN,0}, 	
    {0xAC5C, 0x01CD,WORD_LEN,0}, 	
    {0xAC5E, 0xFF63,WORD_LEN,0}, 	
    {0xAC60, 0xFFD0,WORD_LEN,0}, 	
    {0xAC62, 0xFFC3,WORD_LEN,0}, 	
    {0xAC64, 0x0151,WORD_LEN,0}, 	
    {0xAC66, 0xFFEC,WORD_LEN,0}, 	
    {0xAC68, 0xFFF0,WORD_LEN,0}, 	
    {0xAC6A, 0xFFA4,WORD_LEN,0}, 	
    {0xAC6C, 0x016D,WORD_LEN,0}, 	
    {0xAC6E, 0x0056,WORD_LEN,0}, 	
    {0xB83E, 0x00,BYTE_LEN,0}, 	
    {0xB83F, 0x00,BYTE_LEN,0}, 	
    {0xB840, 0xFF,BYTE_LEN,0}, 	
    {0xB841, 0xEF,BYTE_LEN,0}, 	
    {0xAC3C, 0x2E,BYTE_LEN,0}, 	
    {0xAC3D, 0x84,BYTE_LEN,0}, 	
    {0xAC3E, 0x11,BYTE_LEN,0}, 	
    {0xAC3F, 0x63,BYTE_LEN,0}, 	
    {0xAC40, 0x62,BYTE_LEN,0}, 	
    {0xAC41, 0x68,BYTE_LEN,0}, 	
    {0xAC42, 0x62,BYTE_LEN,0}, 	
    {0xAC43, 0x68,BYTE_LEN,0}, 	
    {0xB842, 0x003D,WORD_LEN,0}, 	
    {0xB844, 0x0044,WORD_LEN,0}, 	
    {0x3240, 0x0024,WORD_LEN,0}, 	
    {0x3240, 0x0024,WORD_LEN,0}, 	
    {0x3242, 0x0000,WORD_LEN,0}, 	
    {0x3244, 0x0000,WORD_LEN,0}, 	
    {0x3246, 0x0000,WORD_LEN,0}, 	
    {0x3248, 0x7F00,WORD_LEN,0}, 	
    {0x324A, 0xA500,WORD_LEN,0}, 	
    {0x324C, 0x1540,WORD_LEN,0}, 	
    {0x324E, 0x01AC,WORD_LEN,0}, 	
    {0x3250, 0x003E,WORD_LEN,0}, 	
    {0x3262, 0xF708,WORD_LEN,0}, 	
    {0xACB0, 0x2B,BYTE_LEN,0}, 	
    {0xACB1, 0x84,BYTE_LEN,0}, 	
    {0xACB2, 0x51,BYTE_LEN,0}, 	
    {0xACB3, 0x52,BYTE_LEN,0}, 	
    {0xACB4, 0x11,BYTE_LEN,0}, 	
    {0xACB5, 0x63,BYTE_LEN,0}, 	
    {0xACB6, 0x55,BYTE_LEN,0}, 	
    {0xACB7, 0x56,BYTE_LEN,0}, 	
    {0xACB8, 0x0096,WORD_LEN,0}, 	
    {0xACBA, 0x0014,WORD_LEN,0}, 	
    
    {0xAC01, 0xEB,BYTE_LEN,0}, 	
    {0xAC97, 0x70,BYTE_LEN,0}, 	
    {0xAC98, 0x80,BYTE_LEN,0}, 	
    {0xAC99, 0x95,BYTE_LEN,0}, 	
    {0xAC9A, 0x80,BYTE_LEN,0}, 	
    {0xAC9B, 0x80,BYTE_LEN,0}, 	
    {0xAC9C, 0x78,BYTE_LEN,0}, 	
    
    {0x8404, 0x06,BYTE_LEN,0}, 	
    {0x8404, 0x05,BYTE_LEN,0}, 	
    
    
    {0xAC09, 0x01,   BYTE_LEN, 0}, 	
    {0xACB0, 0x2F,   BYTE_LEN, 0}, 	
    {0xACB1, 0x7F,   BYTE_LEN, 0}, 	
    {0xACB4, 0x13,   BYTE_LEN, 0}, 	
    {0xACB5, 0x5B,   BYTE_LEN, 0}, 	
    {0xACB2, 0x32,   BYTE_LEN, 0}, 	
    {0xACB3, 0x42,   BYTE_LEN, 0}, 	
    {0xACB6, 0x30,   BYTE_LEN, 0}, 	
    {0xACB7, 0x4F,   BYTE_LEN, 0}, 	
    {0xAC22, 0x0005, WORD_LEN, 0}, 	
    {0xACB8, 0x0056, WORD_LEN, 0}, 	
    {0xACBA, 0x0014, WORD_LEN, 0}, 	
    
    
    
    
    {0x301A, 0x10F4, WORD_LEN, 0}, 	
    {0x301E, 0x0084, WORD_LEN, 0}, 	
    {0x301A, 0x10FC, WORD_LEN, 0}, 	
    {0x326E, 0x00A4, WORD_LEN, 0}, 	
    {0x35A4, 0x0596, WORD_LEN, 0}, 	
    {0x35A2, 0x009B, WORD_LEN, 0}, 	
    {0xDC36, 0x33,   BYTE_LEN, 0}, 	
    {0xDC33, 0x21,   BYTE_LEN, 0}, 	
    {0xDC35, 0x05,   BYTE_LEN, 0}, 	
    {0xDC37, 0x62,   BYTE_LEN, 0}, 	
    
    
    {0xBC18, 0x00,   BYTE_LEN, 0}, 	
    {0xBC19, 0x11,   BYTE_LEN, 0}, 	
    {0xBC1A, 0x23,   BYTE_LEN, 0}, 	
    {0xBC1B, 0x3F,   BYTE_LEN, 0}, 	
    {0xBC1C, 0x67,   BYTE_LEN, 0}, 	
    {0xBC1D, 0x85,   BYTE_LEN, 0}, 	
    {0xBC1E, 0x9B,   BYTE_LEN, 0}, 	
    {0xBC1F, 0xAD,   BYTE_LEN, 0}, 	
    {0xBC20, 0xBB,   BYTE_LEN, 0}, 	
    {0xBC21, 0xC7,   BYTE_LEN, 0}, 	
    {0xBC22, 0xD1,   BYTE_LEN, 0}, 	
    {0xBC23, 0xDA,   BYTE_LEN, 0}, 	
    {0xBC24, 0xE1,   BYTE_LEN, 0}, 	
    {0xBC25, 0xE8,   BYTE_LEN, 0}, 	
    {0xBC26, 0xEE,   BYTE_LEN, 0}, 	
    {0xBC27, 0xF3,   BYTE_LEN, 0}, 	
    {0xBC28, 0xF7,   BYTE_LEN, 0}, 	
    {0xBC29, 0xFB,   BYTE_LEN, 0}, 	
    {0xBC2A, 0xFF,   BYTE_LEN, 0}, 	
    {0xBC2B, 0x00,   BYTE_LEN, 0}, 	
    {0xBC2C, 0x11,   BYTE_LEN, 0}, 	
    {0xBC2D, 0x23,   BYTE_LEN, 0}, 	
    {0xBC2E, 0x3F,   BYTE_LEN, 0}, 	
    {0xBC2F, 0x67,   BYTE_LEN, 0}, 	
    {0xBC30, 0x85,   BYTE_LEN, 0}, 	
    {0xBC31, 0x9B,   BYTE_LEN, 0}, 	
    {0xBC32, 0xAD,   BYTE_LEN, 0}, 	
    {0xBC33, 0xBB,   BYTE_LEN, 0}, 	
    {0xBC34, 0xC7,   BYTE_LEN, 0}, 	
    {0xBC35, 0xD1,   BYTE_LEN, 0}, 	
    {0xBC36, 0xDA,   BYTE_LEN, 0}, 	
    {0xBC37, 0xE1,   BYTE_LEN, 0}, 	
    {0xBC38, 0xE8,   BYTE_LEN, 0}, 	
    {0xBC39, 0xEE,   BYTE_LEN, 0}, 	
    {0xBC3A, 0xF3,   BYTE_LEN, 0}, 	
    {0xBC3B, 0xF7,   BYTE_LEN, 0}, 	
    {0xBC3C, 0xFB,   BYTE_LEN, 0}, 	
    {0xBC3D, 0xFF,   BYTE_LEN, 0}, 	
    {0xBC3E, 0x00,   BYTE_LEN, 0}, 	
    {0xBC3F, 0x18,   BYTE_LEN, 0}, 	
    {0xBC40, 0x25,   BYTE_LEN, 0}, 	
    {0xBC41, 0x3A,   BYTE_LEN, 0}, 	
    {0xBC42, 0x59,   BYTE_LEN, 0}, 	
    {0xBC43, 0x70,   BYTE_LEN, 0}, 	
    {0xBC44, 0x81,   BYTE_LEN, 0}, 	
    {0xBC45, 0x90,   BYTE_LEN, 0}, 	
    {0xBC46, 0x9E,   BYTE_LEN, 0}, 	
    {0xBC47, 0xAB,   BYTE_LEN, 0}, 	
    {0xBC48, 0xB6,   BYTE_LEN, 0}, 	
    {0xBC49, 0xC1,   BYTE_LEN, 0}, 	
    {0xBC4A, 0xCB,   BYTE_LEN, 0}, 	
    {0xBC4B, 0xD5,   BYTE_LEN, 0}, 	
    {0xBC4C, 0xDE,   BYTE_LEN, 0}, 	
    {0xBC4D, 0xE7,   BYTE_LEN, 0}, 	
    {0xBC4E, 0xEF,   BYTE_LEN, 0}, 	
    {0xBC4F, 0xF7,   BYTE_LEN, 0}, 	
    {0xBC50, 0xFF,   BYTE_LEN, 0}, 	
    {0xB801, 0xE0,   BYTE_LEN, 0}, 	
    {0xB862, 0x04,   BYTE_LEN, 0}, 	

    {0xBC51, 0x01,   BYTE_LEN, 0}, 	

    
    {0xB829, 0x02,   BYTE_LEN, 0}, 	
    {0xB863, 0x02,   BYTE_LEN, 0}, 	
    {0xA401, 0x00,   BYTE_LEN, 0}, 	
    {0xA409, 0x3C,   BYTE_LEN, 0}, 	
    {0xA801, 0x01,   BYTE_LEN, 0}, 	
    {0xA818, 0x0578, WORD_LEN, 0}, 	
    {0xA81A, 0x0834, WORD_LEN, 0}, 	
    {0xA402, 0x0004, WORD_LEN, 0}, 	
    {0xA802, 0x0007, WORD_LEN, 0}, 	
    {0xA81C, 0x005A, WORD_LEN, 0}, 	
    {0xA81E, 0x00C0, WORD_LEN, 0}, 	
    {0xA820, 0x0140, WORD_LEN, 0}, 	
    {0xA824, 0x0080, WORD_LEN, 0}, 	
    {0xA822, 0x0080, WORD_LEN, 0}, 	
    {0xBC52, 0x0160, WORD_LEN, 0}, 	
    {0xBC54, 0x0700, WORD_LEN, 0}, 	
    {0xBC58, 0x0160, WORD_LEN, 0}, 	
    {0xBC5A, 0x0500, WORD_LEN, 0}, 	
    {0xBC5E, 0x00FA, WORD_LEN, 0}, 	
    {0xBC60, 0x028A, WORD_LEN, 0}, 	
    {0xBC66, 0x00FA, WORD_LEN, 0}, 	
    {0xBC68, 0x028A, WORD_LEN, 0}, 	
    {0xBC86, 0x00C8, WORD_LEN, 0}, 	
    {0xBC88, 0x028A, WORD_LEN, 0}, 	
    {0xBCBC, 0x0040, WORD_LEN, 0}, 	
    {0xBCBE, 0x01FC, WORD_LEN, 0}, 	
    {0xBCCC, 0x00C8, WORD_LEN, 0}, 	
    {0xBCCE, 0x0640, WORD_LEN, 0}, 	
    {0xBC90, 0x00C8, WORD_LEN, 0}, 	
    {0xBC92, 0x028A, WORD_LEN, 0}, 	
    {0xBCAA, 0x044C, WORD_LEN, 0}, 	
    {0xBCAC, 0x00AF, WORD_LEN, 0}, 	
    {0xBCAE, 0x0009, WORD_LEN, 0}, 	
    {0xBCD8, 0x00C8, WORD_LEN, 0}, 	
    {0xBCDA, 0x0A28, WORD_LEN, 0}, 	
    {0xBC02, 0x01F6, WORD_LEN, 0}, 	
    
    
    {0x3380, 0x0504, WORD_LEN, 0}, 	
    {0x3380, 0x0584, WORD_LEN, 0}, 	
    {0x3380, 0x0586, WORD_LEN, 0}, 	
    {0x3380, 0x05CF, WORD_LEN, 0}, 	
    {0x33B0, 0x2A16, WORD_LEN, 0}, 	
    {0xBCBA, 0x0009, WORD_LEN, 0}, 	
    {0xBCB2, 0x28,   BYTE_LEN, 0}, 	
    {0xBCB3, 0x5F,   BYTE_LEN, 0}, 	
    {0xBC94, 0x12,   BYTE_LEN, 0}, 	
    {0xBC95, 0x0C,   BYTE_LEN, 0}, 	
    {0xBC9C, 0x37,   BYTE_LEN, 0}, 	
    {0xBC9D, 0x24,   BYTE_LEN, 0}, 	
    {0xBC8A, 0x00,   BYTE_LEN, 0}, 	
    {0xBC8B, 0x28,   BYTE_LEN, 0}, 	
    {0xBC8C, 0x01,   BYTE_LEN, 0}, 	
    {0xBC8D, 0x01,   BYTE_LEN, 0}, 	
    {0xBC8E, 0x0A,   BYTE_LEN, 0}, 	
    {0xBC8F, 0x00,   BYTE_LEN, 0}, 	
    {0xBCC0, 0x10,   BYTE_LEN, 0}, 	
    {0xBCC1, 0x03,   BYTE_LEN, 0}, 	
    {0xBCC2, 0x0A,   BYTE_LEN, 0}, 	
    {0xBCC3, 0x01,   BYTE_LEN, 0}, 	
    {0xBCC4, 0x0A,   BYTE_LEN, 0}, 	
    {0xBCC5, 0x0E,   BYTE_LEN, 0}, 	
    
    
    {0x33BA, 0x0088, WORD_LEN, 0}, 	
    {0x33BE, 0x0000, WORD_LEN, 0}, 	
    {0x33C2, 0x3100, WORD_LEN, 0}, 	
    {0xBC5E, 0x0154, WORD_LEN, 0}, 	
    {0xBC60, 0x0640, WORD_LEN, 0}, 	
    {0xBCD0, 0x000A, WORD_LEN, 0}, 	
    {0xBCD2, 0x00FE, WORD_LEN, 0}, 	
    {0xBCD4, 0x001E, WORD_LEN, 0}, 	
    {0xBCD6, 0x00FF, WORD_LEN, 0}, 	
    {0xBC62, 0x07,   BYTE_LEN, 0}, 	
    {0xBC64, 0x05,   BYTE_LEN, 0}, 	
    {0xBC63, 0x50,   BYTE_LEN, 0}, 	
    {0xBC65, 0x50,   BYTE_LEN, 0}, 	
    {0xBC6A, 0x02,   BYTE_LEN, 0}, 	
    {0xBC6C, 0x01,   BYTE_LEN, 0}, 	
    
    {0xBC56, 0xFF,   BYTE_LEN, 0}, 	
    {0xBC57, 0x30,   BYTE_LEN, 0}, 	
    {0xBCDE, 0x03,   BYTE_LEN, 0}, 	
    {0xBCDF, 0x50,   BYTE_LEN, 0}, 	
    {0xBCE0, 0x08,   BYTE_LEN, 0}, 	
    {0xBCE1, 0x03,   BYTE_LEN, 0}, 	
    {0xBCC6, 0x00,   BYTE_LEN, 0}, 	
    {0xBCC7, 0x00,   BYTE_LEN, 0}, 	
    {0xBCC8, 0x20,   BYTE_LEN, 0}, 	
    {0xBCC9, 0x40,   BYTE_LEN, 0}, 	
    {0xBCCA, 0x04,   BYTE_LEN, 0}, 	
    {0xBCCB, 0x00,   BYTE_LEN, 0}, 	

    
    
    
    {0xAC09, 0x01 	, BYTE_LEN, 0},
    {0xAC22, 0x0005 , WORD_LEN, 0},
    {0xACB0, 0x32 	, BYTE_LEN, 0},
    {0xACB1, 0x4C 	, BYTE_LEN, 0},
    {0xACB4, 0x28 	, BYTE_LEN, 0},
    {0xACB5, 0x4F 	, BYTE_LEN, 0},
    {0xACB2, 0x35 	, BYTE_LEN, 0},
    {0xACB3, 0x3B 	, BYTE_LEN, 0},
    {0xACB6, 0x3F 	, BYTE_LEN, 0},
    {0xACB7, 0x4E 	, BYTE_LEN, 0},
    {0xACB8, 0x0056 , WORD_LEN, 0},
    {0xACBA, 0x0014 , WORD_LEN, 0},

    
    
    
    {0x098E, 0xBCE9 , WORD_LEN, 0},	
    {0xBCE9, 0x01 	, BYTE_LEN, 0},
    
    {0xBCEA, 0x0154 , WORD_LEN, 0},	
    
    {0xBCEC, 0x00B0 , WORD_LEN, 0},	
    
    {0xA838, 0x00B4 , WORD_LEN, 0},	
    
    
    
    
    {0x098E, 0xD80F	, WORD_LEN, 0},	
    {0xD80F, 0x04 	, BYTE_LEN, 0},
    {0xD810, 0x08 	, BYTE_LEN, 0},

    
    {0x098E, 0xC8D2  ,WORD_LEN, 0},	
    {0xC8D2, 0x04 	 ,BYTE_LEN, 0},
    {0xC8D3, 0x08 	 ,BYTE_LEN, 0},
    {0xC8BC, 0x04 	 ,BYTE_LEN, 0},
    {0xC8BD, 0x08 	 ,BYTE_LEN, 0},

    
    {0x301A, 0x10F4, WORD_LEN, 0}, 	
    {0x301E, 0x0084, WORD_LEN, 0}, 	
    {0x301A, 0x10FC, WORD_LEN, 0}, 	
    
    {0x098E, 0xDC33 ,WORD_LEN ,0 },		
    {0xDC33, 0x21	  ,BYTE_LEN ,0 }, 
    {0xA80E, 0x12 	,BYTE_LEN ,0 },
    {0xDC35, 0x05 	,BYTE_LEN ,0 },
    {0xDC36, 0x33 	,BYTE_LEN ,0 },
    {0xDC37, 0x62 	,BYTE_LEN ,0 },

    {0x326E, 0x00A4 ,WORD_LEN ,0 }, 	
    {0x35A4, 0x0596 ,WORD_LEN ,0 }, 	
    {0x35A2, 0x009B ,WORD_LEN ,0 }, 	

    
    
    {0x098E, 0xB801 ,	WORD_LEN, 0},
    {0xB801, 0xE0 	, BYTE_LEN, 0},
    {0xB862, 0x04 	, BYTE_LEN, 0},
    
    
    
    
    
    {0x098E, 0xA816, WORD_LEN, 0},		
    {0xA816, 0x000A, WORD_LEN, 0}, 	
    {0xA818, 0x070B, WORD_LEN, 0}, 	
    {0xA81A, 0x09E7, WORD_LEN, 0}, 	

    
    {0x098E, 0xA401 , WORD_LEN, 0},		
    {0xA401, 0x00 	, BYTE_LEN, 0}, 
    {0xA402, 0x0004 , WORD_LEN, 0}, 	
    {0xA409, 0x3A	  , BYTE_LEN, 0},	
                  
    {0x098E, 0xA801 , WORD_LEN, 0},		
    {0xA801, 0x01 	, BYTE_LEN, 0}, 
    {0xA802, 0x0007 , WORD_LEN, 0}, 	
    {0xA81C, 0x0060 , WORD_LEN, 0}, 	
    {0xA81E, 0x00DC , WORD_LEN, 0}, 	
    {0xA820, 0x0154 , WORD_LEN, 0}, 	
    {0xA824, 0x00B0 , WORD_LEN, 0}, 	
    {0xA822, 0x0080 , WORD_LEN, 0}, 	

    
    
    {0x098E, 0xB824 , WORD_LEN, 0},		
    {0xB824, 0x05 	, BYTE_LEN, 0}, 
    {0xB825, 0x05 	, BYTE_LEN, 0}, 
                    
    
    
    {0x098E, 0xB829 , WORD_LEN, 0},	
    {0xB829, 0x02 	, BYTE_LEN, 0}, 
    {0xB863, 0x02 	, BYTE_LEN, 0}, 

    
    {0x098E, 0xBC73 , WORD_LEN, 0},	
    {0xBC72, 0x0F 	, BYTE_LEN, 0}, 
    {0xBC73, 0x1A 	, BYTE_LEN, 0}, 

    
    
    {0x098E, 0xBC52, WORD_LEN, 0},		
    {0xBC52, 0x0160, WORD_LEN, 0}, 	
    {0xBC54, 0x0800, WORD_LEN, 0}, 	
    {0xBC58, 0x0160, WORD_LEN, 0}, 	
    {0xBC5A, 0x0800, WORD_LEN, 0}, 	
    {0xBC5E, 0x00FA, WORD_LEN, 0}, 	
    {0xBC60, 0x088A, WORD_LEN, 0}, 	
    {0xBC66, 0x00FA, WORD_LEN, 0}, 	
    {0xBC68, 0x038A, WORD_LEN, 0}, 	
    {0xBC86, 0x00C8, WORD_LEN, 0}, 	
    {0xBC88, 0x038A, WORD_LEN, 0}, 	
    {0xBCBC, 0x0040, WORD_LEN, 0}, 	
    {0xBCBE, 0x014C, WORD_LEN, 0}, 	
    {0xBCCC, 0x00C8, WORD_LEN, 0}, 	
    {0xBCCE, 0x0340, WORD_LEN, 0}, 	
    {0xBC90, 0x00C8, WORD_LEN, 0}, 	
    {0xBC92, 0x038A, WORD_LEN, 0}, 	
    {0xBCAA, 0x044C, WORD_LEN, 0}, 	
    {0xBCAC, 0x00AF, WORD_LEN, 0}, 	
    {0xBCAE, 0x0009, WORD_LEN, 0}, 	
    {0xBCD8, 0x00DC, WORD_LEN, 0}, 	
    {0xBCDA, 0x0BDA, WORD_LEN, 0}, 	
    {0xBC02, 0x01F6, WORD_LEN, 0}, 	
                   
    {0x098E, 0x3C14, WORD_LEN, 0}, 	
    {0xBC14, 0xFFDC, WORD_LEN, 0}, 	
    {0xBC16, 0xFFFF, WORD_LEN, 0}, 	

    
    
    
    {0x3380, 0x0504, WORD_LEN, 0}, 	
    
    
    {0x098E, 0xBCB2 , WORD_LEN, 0},		
    {0xBCB2, 0x28 	, BYTE_LEN, 0}, 
    {0xBCB3, 0x5F 	, BYTE_LEN, 0}, 
    {0x3380, 0x0584 , WORD_LEN, 0}, 	
    {0x3380, 0x0586 , WORD_LEN, 0}, 	
    {0x3380, 0x0505 , WORD_LEN, 0}, 	

    
    
    {0x098E, 0xBC94 , WORD_LEN ,0},	
    {0xBC94, 0x12 	, BYTE_LEN ,0}, 
    {0xBC95, 0x0C 	, BYTE_LEN ,0}, 
    {0xBC9C, 0x37 	, BYTE_LEN ,0}, 
    {0xBC9D, 0x24 	, BYTE_LEN ,0}, 

    
    {0x33B0, 0x2A16, WORD_LEN, 0}, 	
    
    {0x098E, 0xBC8A , WORD_LEN, 0},	
    {0xBC8A, 0x00 	, BYTE_LEN, 0}, 
    {0xBC8B, 0x28 	, BYTE_LEN, 0}, 
    {0xBC8C, 0x01 	, BYTE_LEN, 0}, 
    {0xBC8D, 0x01 	, BYTE_LEN, 0}, 
    
    
    
    {0xBC8F, 0x00, BYTE_LEN, 0}, 	

    
    
    {0xBCC0, 0x10 	, BYTE_LEN, 0}, 
    {0xBCC1, 0x03 	, BYTE_LEN, 0}, 
    {0xBCC2, 0x0A 	, BYTE_LEN, 0}, 
    {0xBCC3, 0x01 	, BYTE_LEN, 0}, 
    {0xBCC4, 0x0A 	, BYTE_LEN, 0}, 
    {0xBCC5, 0x0E 	, BYTE_LEN, 0}, 
    {0xBCBA, 0x0009 , WORD_LEN, 0},	
    
    
    
    
    {0x33BA, 0x006A, WORD_LEN, 0}, 	
    {0x33BE, 0x0001, WORD_LEN, 0}, 	
    {0x33C2, 0x1000, WORD_LEN, 0}, 	

    
    
    {0x098E, 0xBC5E	, WORD_LEN, 0},	
    {0xBC5E, 0x0124 , WORD_LEN, 0},	
    {0xBC60, 0x05D0 , WORD_LEN, 0},	
    {0xBC62, 0x07 	, BYTE_LEN, 0},
    {0xBC64, 0x05 	, BYTE_LEN, 0},
    {0xBC63, 0x50 	, BYTE_LEN, 0},
    {0xBC65, 0x50 	, BYTE_LEN, 0},
    {0xBC6A, 0x02 	, BYTE_LEN, 0},
    {0xBC6C, 0x02 	, BYTE_LEN, 0},

    
    {0x098E, 0xBCDE , WORD_LEN, 0},	
    {0xBCDE, 0x03 	, BYTE_LEN, 0}, 
    {0xBCDF, 0x50 	, BYTE_LEN, 0}, 
    {0xBCE0, 0x08 	, BYTE_LEN, 0}, 
    {0xBCE1, 0x03 	, BYTE_LEN, 0}, 

    
    
    {0x098E, 0xBCD0 , WORD_LEN, 0},	
    {0xBCD0, 0x000A , WORD_LEN, 0},	
    {0xBCD2, 0x00CC , WORD_LEN, 0},	
    {0xBCD4, 0x0014 , WORD_LEN, 0},	
    {0xBCD6, 0x00CD , WORD_LEN, 0},	
    {0xBCC6, 0x00 	, BYTE_LEN, 0}, 
    {0xBCC7, 0x00 	, BYTE_LEN, 0}, 
    {0xBCC8, 0x20		, BYTE_LEN, 0}, 
    {0xBCC9, 0x40		, BYTE_LEN, 0}, 
    {0xBCCA, 0x04 	, BYTE_LEN, 0}, 
    {0xBCCB, 0x00 	, BYTE_LEN, 0}, 

    
    {0xC400, 0x88,   BYTE_LEN, 0}, 	
    {0x8419, 0x05,   BYTE_LEN, 0}, 	
    {0xC400, 0x08,   BYTE_LEN, 0}, 	
    {0xB018, 0x06,   BYTE_LEN, 0}, 	
    {0xB019, 0x1E,   BYTE_LEN, 0}, 	
    {0xB01A, 0x3C,   BYTE_LEN, 0}, 	
    {0xB01B, 0x5A,   BYTE_LEN, 0}, 	
    {0xB01C, 0x78,   BYTE_LEN, 0}, 	
    {0xB01D, 0x96,   BYTE_LEN, 0}, 	
    {0xB01E, 0xB4,   BYTE_LEN, 0}, 	
    {0xB01F, 0xD2,   BYTE_LEN, 0}, 	
    {0xB020, 0xF0,   BYTE_LEN, 0}, 	
    {0xB021, 0xFF,   BYTE_LEN, 0},	
    {0xB012, 0x0A,   BYTE_LEN, 0}, 	
    {0xB013, 0xAA,   BYTE_LEN, 0}, 	
    {0xB014, 0x03,   BYTE_LEN, 0}, 	
    {0xB002, 0x0047, WORD_LEN, 0}, 	
    {0xB004, 0x0002, WORD_LEN, 0}, 	

    {0xC40C, 0x00F0, WORD_LEN, 0},	

    {0xC40A, 0x0008, WORD_LEN, 0},	

    {0xB008, 0x0000, WORD_LEN, 0},
    {0xB00A, 0x003F, WORD_LEN, 0}, 	
    {0xB00C, 0x0FC3, WORD_LEN, 0}, 	
    {0xB00E, 0xF000, WORD_LEN, 0},
};

static struct mt9p111_i2c_reg_conf const noise_reduction_reg_settings_array[] = {
    {0x0000, 0x0000, WORD_LEN, 0},
};

static struct mt9p111_i2c_reg_conf const lens_roll_off_tbl[] = {
    {0x0000, 0x0000, WORD_LEN, 0},
};


static struct mt9p111_i2c_reg_conf const pll_setup_tbl[] = {
     {0x0010, 0x0115, WORD_LEN, 0},                               
     {0x0012, 0x0060, WORD_LEN, 0},                               
     {0x0014, 0x2025, WORD_LEN, 0},                               
     {0x001E, 0x0575, WORD_LEN, 0}, 	
     {0x0022, 0x01E0, WORD_LEN, 0}, 	
     {0x002A, 0x7F78, WORD_LEN, 0},                               
     {0x002C, 0x0000, WORD_LEN, 0},                               
     {0x002E, 0x0000, WORD_LEN, 0},                              
     {0x0018, 0x4008, WORD_LEN, 50}, 	
};


static struct mt9p111_i2c_reg_conf const sequencer_tbl[] = {
    {0x8404, 0x05,   BYTE_LEN, 50}, 	
    {0xAC02, 0x00FF, WORD_LEN, 0}, 	    
    {0xAC02, 0x00FF, WORD_LEN, 0}, 	    
    {0x8404, 0x06,   BYTE_LEN, 50},    
    {0x0018, 0x2008, WORD_LEN, 100}, 	
};

static struct mt9p111_i2c_reg_conf const wb_cloudy_tbl[] = {
    {0x098E, 0x8410, WORD_LEN, 0}, 
    {0x8410, 0x01  , BYTE_LEN, 0}, 
    {0x8418, 0x01  , BYTE_LEN, 0}, 
    {0x8420, 0x01  , BYTE_LEN, 0}, 
    {0xAC44, 0x7F  , BYTE_LEN, 0}, 
    {0xAC45, 0x7F  , BYTE_LEN, 0}, 
    {0x8404, 0x06  , BYTE_LEN, 0}, 
    {0xAC04, 0x2E  , BYTE_LEN, 0}, 
    {0xAC05, 0x4D  , BYTE_LEN, 0}, 
};

static struct mt9p111_i2c_reg_conf const wb_daylight_tbl[] = {
    {0x098E, 0x8410, WORD_LEN, 0},     
    {0x8410, 0x01  , BYTE_LEN, 0},     
    {0x8418, 0x01  , BYTE_LEN, 0},     
    {0x8420, 0x01  , BYTE_LEN, 0},     
    {0xAC44, 0x7F  , BYTE_LEN, 0},     
    {0xAC45, 0x7F  , BYTE_LEN, 0},     
    {0x8404, 0x06  , BYTE_LEN, 0},     
    {0xAC04, 0x39  , BYTE_LEN, 0},     
    {0xAC05, 0x4D  , BYTE_LEN, 0},     
};

static struct mt9p111_i2c_reg_conf const wb_flourescant_tbl[] = {
    {0x098E, 0x8410, WORD_LEN, 0},     
    {0x8410, 0x01  , BYTE_LEN, 0},     
    {0x8418, 0x01  , BYTE_LEN, 0},     
    {0x8420, 0x01  , BYTE_LEN, 0},     
    {0xAC44, 0x7F  , BYTE_LEN, 0},     
    {0xAC45, 0x7F  , BYTE_LEN, 0},     
    {0x8404, 0x06  , BYTE_LEN, 0},     
    {0xAC04, 0x3E  , BYTE_LEN, 0},     
    {0xAC05, 0x3C  , BYTE_LEN, 0},     
};

static struct mt9p111_i2c_reg_conf const wb_incandescent_tbl[] = {
    {0x098E, 0x8410, WORD_LEN, 0}, 
    {0x8410, 0x01  , BYTE_LEN, 0}, 
    {0x8418, 0x01  , BYTE_LEN, 0}, 
    {0x8420, 0x01  , BYTE_LEN, 0}, 
    {0xAC44, 0x7F  , BYTE_LEN, 0}, 
    {0xAC45, 0x7F  , BYTE_LEN, 0}, 
    {0x8404, 0x06  , BYTE_LEN, 0}, 
    {0xAC04, 0x50  , BYTE_LEN, 0}, 
    {0xAC05, 0x2C  , BYTE_LEN, 0}, 
};

static struct mt9p111_i2c_reg_conf const wb_auto_tbl[] = {
    {0x098E, 0x8410 , WORD_LEN, 0},	
    {0x8410, 0x02 	, BYTE_LEN, 0},
    {0x8418, 0x02 	, BYTE_LEN, 0},
    {0x8420, 0x02 	, BYTE_LEN, 0},
    {0xAC44, 0x00 	, BYTE_LEN, 0},
    {0xAC45, 0x7F 	, BYTE_LEN, 0},
    {0x8404, 0x06 	, BYTE_LEN, 0},
};

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
    {0x337E, 0x0000 , WORD_LEN, 0},	// Y_RGB_OFFSET
    {0xA805, 0x04 	, BYTE_LEN, 0}, // RESERVED_AE_TRACK_05                    
    {0xA409, 0x0A 	, BYTE_LEN, 0}, // AE_RULE_BASE_TARGET                     
    {0x8404, 0x06 	, BYTE_LEN, 0}, // SEQ_CMD                                 
};

static struct mt9p111_i2c_reg_conf const brightness_tbl_1[] = {
    {0x337E, 0x0000 , WORD_LEN, 0},	// Y_RGB_OFFSET
    {0xA805, 0x04 	, BYTE_LEN, 0}, // RESERVED_AE_TRACK_05                    
    {0xA409, 0x1E 	, BYTE_LEN, 0}, // AE_RULE_BASE_TARGET                     
    {0x8404, 0x06 	, BYTE_LEN, 0}, // SEQ_CMD                                 
};

static struct mt9p111_i2c_reg_conf const brightness_tbl_2[] = {
    {0x337E, 0x0000 , WORD_LEN, 0},	// Y_RGB_OFFSET
    {0xA805, 0x04 	, BYTE_LEN, 0}, // RESERVED_AE_TRACK_05                    
    {0xA409, 0x28 	, BYTE_LEN, 0}, // AE_RULE_BASE_TARGET                     
    {0x8404, 0x06 	, BYTE_LEN, 0}, // SEQ_CMD
};

static struct mt9p111_i2c_reg_conf const brightness_tbl_3[] = {
    {0x337E, 0x0000 , WORD_LEN, 0},	// Y_RGB_OFFSET
    {0xA805, 0x04 	, BYTE_LEN, 0}, // RESERVED_AE_TRACK_05                    
    {0xA409, 0x3A 	, BYTE_LEN, 0}, // AE_RULE_BASE_TARGET                     
    {0x8404, 0x06 	, BYTE_LEN, 0}, // SEQ_CMD
};

static struct mt9p111_i2c_reg_conf const brightness_tbl_4[] = {
    {0x337E, 0x1000 , WORD_LEN, 0},	// Y_RGB_OFFSET
    {0xA805, 0x04 	, BYTE_LEN, 0}, // RESERVED_AE_TRACK_05                        
    {0xA409, 0x3F 	, BYTE_LEN, 0}, // AE_RULE_BASE_TARGET                         
    {0x8404, 0x06 	, BYTE_LEN, 0}, // SEQ_CMD                                     
};

static struct mt9p111_i2c_reg_conf const brightness_tbl_5[] = {
    {0x337E, 0x2000 , WORD_LEN, 0},	// Y_RGB_OFFSET
    {0xA805, 0x04 	, BYTE_LEN, 0}, // RESERVED_AE_TRACK_05                     
    {0xA409, 0x3F 	, BYTE_LEN, 0}, // AE_RULE_BASE_TARGET                      
    {0x8404, 0x06 	, BYTE_LEN, 0}, // SEQ_CMD                                  
};

static struct mt9p111_i2c_reg_conf const brightness_tbl_6[] = {
    {0x337E, 0x3000 , WORD_LEN, 0}, // Y_RGB_OFFSET
    {0xA805, 0x04   , BYTE_LEN, 0}, // RESERVED_AE_TRACK_05                   
    {0xA409, 0x3F   , BYTE_LEN, 0}, // AE_RULE_BASE_TARGET                    
    {0x8404, 0x06   , BYTE_LEN, 0}, // SEQ_CMD                                
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
    {0x8404, 0x06, BYTE_LEN, 0}, // SEQ_CMD 
};

static struct mt9p111_i2c_reg_conf const saturation_tbl_1[] = {
    {0xBC56, 0x40, BYTE_LEN, 0}, // LL_START_CCM_SATURATION
    {0xBC57, 0x10, BYTE_LEN, 0}, // LL_END_CCM_SATURATION  
    {0x098E, 0xDC35, WORD_LEN, 0}, // LOGICAL_ADDRESS_ACCESS [SYS_UV_COLOR_BOOST]       
    {0xDC35, 0x04, BYTE_LEN, 0}, // SYS_UV_COLOR_BOOST                                 
    {0x8404, 0x06, BYTE_LEN, 0}, // SEQ_CMD 
	
};

static struct mt9p111_i2c_reg_conf const saturation_tbl_2[] = {
    {0xBC56, 0xF0, BYTE_LEN, 0}, // LL_START_CCM_SATURATION
    {0xBC57, 0x30, BYTE_LEN, 0}, // LL_END_CCM_SATURATION    
    {0x098E, 0xDC35, WORD_LEN, 0}, // LOGICAL_ADDRESS_ACCESS [SYS_UV_COLOR_BOOST]       
    {0xDC35, 0x04, BYTE_LEN, 0}, // SYS_UV_COLOR_BOOST                                 
    {0x8404, 0x06, BYTE_LEN, 0}, // SEQ_CMD 

};

static struct mt9p111_i2c_reg_conf const saturation_tbl_3[] = {
    {0xBC56, 0xe4,   BYTE_LEN, 0}, 	// LL_START_CCM_SATURATION
    {0xBC57, 0x60,   BYTE_LEN, 0}, 	// LL_END_CCM_SATURATION    
    {0x098E, 0xDC35, WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [SYS_UV_COLOR_BOOST]
    {0xDC35, 0x05, BYTE_LEN, 0}, // SYS_UV_COLOR_BOOST
    {0x8404, 0x06, BYTE_LEN, 0}, // SEQ_CMD
};

static struct mt9p111_i2c_reg_conf const saturation_tbl_4[] = {
    {0xBC56, 0xC2,   BYTE_LEN, 0}, 	// LL_START_CCM_SATURATION
    {0xBC57, 0x80,   BYTE_LEN, 0}, 	// LL_END_CCM_SATURATION 
    {0x098E, 0xDC35, WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [SYS_UV_COLOR_BOOST]
    {0xDC35, 0x06, BYTE_LEN, 0}, // SYS_UV_COLOR_BOOST
    {0x8404, 0x06, BYTE_LEN, 0}, // SEQ_CMD
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

static struct mt9p111_i2c_reg_conf const sharpness_tbl_0[] = {
    {0x098E, 0xBC6A , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_INTEGER_GAIN]   
    {0xBC6A, 0x0004 , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_INTEGER_GAIN]
    {0x33C2, 0x0000 , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_INTEGER_GAIN]   
    {0x33BA, 0x006A , WORD_LEN, 0}, // LL_START_APERTURE_INTEGER_GAIN                            
    {0x8404, 0x06 	, BYTE_LEN, 0}, // SEQ_CMD                                                   
};

static struct mt9p111_i2c_reg_conf const sharpness_tbl_1[] = {
    {0x098E, 0xBC6A , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_INTEGER_GAIN]   
    {0xBC6A, 0x0004 , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_INTEGER_GAIN]
    {0x33C2, 0x1200 , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_INTEGER_GAIN]   
    {0x33BA, 0x006A , WORD_LEN, 0}, // LL_START_APERTURE_INTEGER_GAIN                            
    {0x8404, 0x06 	, BYTE_LEN, 0}, // SEQ_CMD                                                   
};

static struct mt9p111_i2c_reg_conf const sharpness_tbl_2[] = {
    {0x098E, 0xBC6A , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_INTEGER_GAIN]  
    {0xBC6A, 0x0004 , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_INTEGER_GAIN]
    {0x33C2, 0x4400 , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_INTEGER_GAIN]   
    {0x33BA, 0x0066 , WORD_LEN, 0}, // LL_START_APERTURE_INTEGER_GAIN                            
    {0x8404, 0x06, BYTE_LEN, 0}, // SEQ_CMD
};

static struct mt9p111_i2c_reg_conf const sharpness_tbl_3[] = {
    {0x098E, 0xBC6A , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_INTEGER_GAIN]   
    {0xBC6A, 0x0004 , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_INTEGER_GAIN]
    {0x33C2, 0xBB00 , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_INTEGER_GAIN]   
    {0x33BA, 0x006D , WORD_LEN, 0}, // LL_START_APERTURE_INTEGER_GAIN                            
    {0x8404, 0x06 	, BYTE_LEN, 0}, // SEQ_CMD                                                       
};

static struct mt9p111_i2c_reg_conf const sharpness_tbl_4[] = {
    {0x098E, 0xBC6A , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_INTEGER_GAIN]
    {0xBC6A, 0x0004 , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_INTEGER_GAIN]
    {0x33C2, 0xFF00 , WORD_LEN, 0},	// LOGICAL_ADDRESS_ACCESS [LL_START_APERTURE_INTEGER_GAIN]
    {0x33BA, 0x006F , WORD_LEN, 0}, // LL_START_APERTURE_INTEGER_GAIN
    {0x8404, 0x06 	, BYTE_LEN, 0}, // SEQ_CMD   
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

    .otp_lsc_reg_tbl                        = otp_lsc_reg_tbl,
    .otp_lsc_reg_tbl_sz                     = otp_lsc_reg_tbl_sz,
};


