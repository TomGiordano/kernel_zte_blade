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

#include "hi704.h"

static struct hi704_i2c_reg_conf const preview_snapshot_mode_reg_settings_array[] = {
    {0x01, 0xf1,BYTE_LEN,0},
{0x01, 0xf3,BYTE_LEN,0},
{0x01, 0xf1,BYTE_LEN,0},
{0x03, 0x00,BYTE_LEN,0},
{0x11, 0x92,BYTE_LEN,0}, //by lijing0506
{0x12, 0x00,BYTE_LEN,0},
//{0x12, 0x04,BYTE_LEN,0},
{0x20, 0x00,BYTE_LEN,0},
{0x21, 0x04,BYTE_LEN,0},
{0x22, 0x00,BYTE_LEN,0},
{0x23, 0x04,BYTE_LEN,0},

//{0x10, 0x08},// william
{0x11, 0x92, BYTE_LEN, 0},// william

//BLC
{0x80, 0x2e, BYTE_LEN, 0},
{0x81, 0x7e, BYTE_LEN, 0},
{0x82, 0x90, BYTE_LEN, 0},
{0x83, 0x30, BYTE_LEN, 0},
{0x84, 0x2c, BYTE_LEN, 0},//*** Change 100406
{0x85, 0x4b, BYTE_LEN, 0},//*** Change 100406 
{0x89, 0x48, BYTE_LEN, 0},//BLC hold
{0x90, 0x0b, BYTE_LEN, 0},//TIME_IN  11/100  _100318
{0x91, 0x0b, BYTE_LEN, 0},//TIME_OUT 11/100  _100318
{0x92, 0x48, BYTE_LEN, 0},//AG_IN
{0x93, 0x48, BYTE_LEN, 0},//AG_OUT
{0x98, 0x38, BYTE_LEN, 0},
{0x99, 0x40, BYTE_LEN, 0}, //Out BLC
{0xa0, 0x00, BYTE_LEN, 0}, //Dark BLC
{0xa8, 0x40, BYTE_LEN, 0}, //Normal BLC

//Page 2  Last Update 10_03_12
{0x03, 0x02, BYTE_LEN, 0},
{0x13, 0x40, BYTE_LEN, 0}, //*** ADD 100402 
{0x14, 0x04, BYTE_LEN, 0}, //*** ADD 100402 
{0x1a, 0x00, BYTE_LEN, 0}, //*** ADD 100402 
{0x1b, 0x08, BYTE_LEN, 0}, //*** ADD 100402 
{0x20, 0x33, BYTE_LEN, 0},
{0x21, 0xaa, BYTE_LEN, 0},//*** Change 100402 
{0x22, 0xa7, BYTE_LEN, 0},
{0x23, 0x32, BYTE_LEN, 0},//*** Change 100405 
{0x3b, 0x48, BYTE_LEN, 0},//*** ADD 100405 
{0x50, 0x21, BYTE_LEN, 0}, //*** ADD 100406
{0x52, 0xa2, BYTE_LEN, 0},
{0x53, 0x0a, BYTE_LEN, 0},
{0x54, 0x30, BYTE_LEN, 0},//*** ADD 100405 
{0x55, 0x10, BYTE_LEN, 0},//*** Change 100402 
{0x56, 0x0c, BYTE_LEN, 0},
{0x59, 0x0F, BYTE_LEN, 0},//*** ADD 100405 
{0x60, 0xca, BYTE_LEN, 0},
{0x61, 0xdb, BYTE_LEN, 0},
{0x62, 0xca, BYTE_LEN, 0},
{0x63, 0xda, BYTE_LEN, 0},
{0x64, 0xca, BYTE_LEN, 0},
{0x65, 0xda, BYTE_LEN, 0},
{0x72, 0xcb, BYTE_LEN, 0},
{0x73, 0xd8, BYTE_LEN, 0},
{0x74, 0xcb, BYTE_LEN, 0},
{0x75, 0xd8, BYTE_LEN, 0},
{0x80, 0x02, BYTE_LEN, 0},
{0x81, 0xbd, BYTE_LEN, 0},
{0x82, 0x24, BYTE_LEN, 0},
{0x83, 0x3e, BYTE_LEN, 0},
{0x84, 0x24, BYTE_LEN, 0},
{0x85, 0x3e, BYTE_LEN, 0},
{0x92, 0x72, BYTE_LEN, 0},
{0x93, 0x8c, BYTE_LEN, 0},
{0x94, 0x72, BYTE_LEN, 0},
{0x95, 0x8c, BYTE_LEN, 0},
{0xa0, 0x03, BYTE_LEN, 0},
{0xa1, 0xbb, BYTE_LEN, 0},
{0xa4, 0xbb, BYTE_LEN, 0},
{0xa5, 0x03, BYTE_LEN, 0},
{0xa8, 0x44, BYTE_LEN, 0},
{0xa9, 0x6a, BYTE_LEN, 0},
{0xaa, 0x92, BYTE_LEN, 0},
{0xab, 0xb7, BYTE_LEN, 0},
{0xb8, 0xc9, BYTE_LEN, 0},
{0xb9, 0xd0, BYTE_LEN, 0},
{0xbc, 0x20, BYTE_LEN, 0},
{0xbd, 0x28, BYTE_LEN, 0},
{0xc0, 0xDE, BYTE_LEN, 0},//*** Change 100402
{0xc1, 0xEC, BYTE_LEN, 0},//*** Change 100402
{0xc2, 0xDE, BYTE_LEN, 0},//*** Change 100402
{0xc3, 0xEC, BYTE_LEN, 0},//*** Change 100402
{0xc4, 0xE0, BYTE_LEN, 0},//*** Change 100402
{0xc5, 0xEA, BYTE_LEN, 0},//*** Change 100402
{0xc6, 0xE0, BYTE_LEN, 0},//*** Change 100402
{0xc7, 0xEa, BYTE_LEN, 0},//*** Change 100402
{0xc8, 0xe1, BYTE_LEN, 0},
{0xc9, 0xe8, BYTE_LEN, 0},
{0xca, 0xe1, BYTE_LEN, 0},
{0xcb, 0xe8, BYTE_LEN, 0},
{0xcc, 0xe2, BYTE_LEN, 0},
{0xcd, 0xe7, BYTE_LEN, 0},
{0xce, 0xe2, BYTE_LEN, 0},
{0xcf, 0xe7, BYTE_LEN, 0},
{0xd0, 0xc8, BYTE_LEN, 0},
{0xd1, 0xef, BYTE_LEN, 0},


//Page 10
{0x03, 0x10, BYTE_LEN, 0},
{0x10, 0x01, BYTE_LEN, 0},//{0x10, 0x03}, //03, //ISPCTL1, YUV ORDER(FIX)// william
{0x11, 0x43, BYTE_LEN, 0},
{0x12, 0x30, BYTE_LEN, 0}, //Y offet, dy offseet enable
{0x40, 0x80, BYTE_LEN, 0},
{0x41, 0x10, BYTE_LEN, 0}, //00 DYOFS  00->10  _100318
{0x48, 0x84, BYTE_LEN, 0}, //Contrast  88->84  _100318
{0x50, 0x48, BYTE_LEN, 0}, //AGBRT
       
{0x60, 0x7f, BYTE_LEN, 0},
{0x61, 0x00, BYTE_LEN, 0}, //Use default
{0x62, 0x90, BYTE_LEN, 0}, //SATB  (1.4x)
{0x63, 0xa0, BYTE_LEN, 0}, //SATR  (1.2x)
{0x64, 0x48, BYTE_LEN, 0}, //AGSAT
{0x66, 0x90, BYTE_LEN, 0}, //wht_th2
{0x67, 0x36, BYTE_LEN, 0}, //wht_gain  Dark (0.4x), Normal (0.75x)

//LPF
{0x03, 0x11, BYTE_LEN, 0},
{0x10, 0x25, BYTE_LEN, 0},	//LPF_CTL1 //0x01
{0x11, 0x1f, BYTE_LEN, 0},	//Test Setting
{0x20, 0x00, BYTE_LEN, 0},	//LPF_AUTO_CTL
{0x21, 0x38, BYTE_LEN, 0},	//LPF_PGA_TH
{0x23, 0x0a, BYTE_LEN, 0},	//LPF_TIME_TH
{0x60, 0x0a, BYTE_LEN, 0},	//ZARA_SIGMA_TH //40->10
{0x61, 0x82, BYTE_LEN, 0},
{0x62, 0x00, BYTE_LEN, 0},	//ZARA_HLVL_CTL
{0x63, 0x83, BYTE_LEN, 0},	//ZARA_LLVL_CTL
{0x64, 0x83, BYTE_LEN, 0},	//ZARA_DY_CTL

{0x67, 0xF0, BYTE_LEN, 0},	//*** Change 100402     //Dark
{0x68, 0x30, BYTE_LEN, 0},	//*** Change 100402     //Middle
{0x69, 0x10, BYTE_LEN, 0},	//High

{0x03, 0x12, BYTE_LEN, 0},
{0x40, 0xe9, BYTE_LEN, 0},	//YC2D_LPF_CTL1
{0x41, 0x09, BYTE_LEN, 0},	//YC2D_LPF_CTL2
{0x50, 0x18, BYTE_LEN, 0},	//Test Setting
{0x51, 0x24, BYTE_LEN, 0},	//Test Setting
{0x70, 0x1f, BYTE_LEN, 0},	//GBGR_CTL1 //0x1f
{0x71, 0x00, BYTE_LEN, 0},	//Test Setting
{0x72, 0x00, BYTE_LEN, 0},	//Test Setting
{0x73, 0x00, BYTE_LEN, 0},	//Test Setting
{0x74, 0x10, BYTE_LEN, 0},	//GBGR_G_UNIT_TH
{0x75, 0x10, BYTE_LEN, 0},	//GBGR_RB_UNIT_TH
{0x76, 0x20, BYTE_LEN, 0},	//GBGR_EDGE_TH
{0x77, 0x80, BYTE_LEN, 0},	//GBGR_HLVL_TH
{0x78, 0x88, BYTE_LEN, 0},	//GBGR_HLVL_COMP
{0x79, 0x18, BYTE_LEN, 0},	//Test Setting
{0xb0, 0x7d, BYTE_LEN, 0},   //dpc

//Edge
{0x03, 0x13, BYTE_LEN, 0},
{0x10, 0x01, BYTE_LEN, 0},	
{0x11, 0x89, BYTE_LEN, 0},	
{0x12, 0x14, BYTE_LEN, 0},	
{0x13, 0x19, BYTE_LEN, 0},	
{0x14, 0x08, BYTE_LEN, 0},	//Test Setting
{0x20, 0x06, BYTE_LEN, 0},	//SHARP_Negative
{0x21, 0x04, BYTE_LEN, 0},	//SHARP_Positive
{0x23, 0x30, BYTE_LEN, 0},	//SHARP_DY_CTL
{0x24, 0x33, BYTE_LEN, 0},	//40->33
{0x25, 0x08, BYTE_LEN, 0},	//SHARP_PGA_TH
{0x26, 0x18, BYTE_LEN, 0},	//Test Setting
{0x27, 0x00, BYTE_LEN, 0},	//Test Setting
{0x28, 0x08, BYTE_LEN, 0},	//Test Setting
{0x29, 0x50, BYTE_LEN, 0},	//AG_TH
{0x2a, 0xe0, BYTE_LEN, 0},	//region ratio
{0x2b, 0x10, BYTE_LEN, 0},	//Test Setting
{0x2c, 0x28, BYTE_LEN, 0},	//Test Setting
{0x2d, 0x40, BYTE_LEN, 0},	//Test Setting
{0x2e, 0x00, BYTE_LEN, 0},	//Test Setting
{0x2f, 0x00, BYTE_LEN, 0},	//Test Setting
{0x30, 0x11, BYTE_LEN, 0},	//Test Setting
{0x80, 0x03, BYTE_LEN, 0},	//SHARP2D_CTL
{0x81, 0x07, BYTE_LEN, 0},	//Test Setting
{0x90, 0x06, BYTE_LEN, 0},	//SHARP2D_SLOPE
{0x91, 0x04, BYTE_LEN, 0},	//SHARP2D_DIFF_CTL
{0x92, 0x00, BYTE_LEN, 0},	//SHARP2D_HI_CLIP
{0x93, 0x20, BYTE_LEN, 0},	//SHARP2D_DY_CTL
{0x94, 0x42, BYTE_LEN, 0},	//Test Setting
{0x95, 0x60, BYTE_LEN, 0},	//Test Setting

//Shading
{0x03, 0x14, BYTE_LEN, 0},
{0x10, 0x01, BYTE_LEN, 0},
{0x20, 0x80, BYTE_LEN, 0}, //XCEN
{0x21, 0x80, BYTE_LEN, 0}, //YCEN
{0x22, 0x66, BYTE_LEN, 0}, //76, 34, 2b
{0x23, 0x50, BYTE_LEN, 0},
{0x24, 0x44, BYTE_LEN, 0}, //3

//Page 15 CMC
{0x03, 0x15, BYTE_LEN, 0}, 
{0x10, 0x03, BYTE_LEN, 0},
       
{0x14, 0x3c, BYTE_LEN, 0},
{0x16, 0x2c, BYTE_LEN, 0},
{0x17, 0x2f, BYTE_LEN, 0},

{0x30, 0xcb, BYTE_LEN, 0},
{0x31, 0x61, BYTE_LEN, 0},
{0x32, 0x16, BYTE_LEN, 0},
{0x33, 0x23, BYTE_LEN, 0},
{0x34, 0xce, BYTE_LEN, 0},
{0x35, 0x2b, BYTE_LEN, 0},
{0x36, 0x01, BYTE_LEN, 0},
{0x37, 0x34, BYTE_LEN, 0},
{0x38, 0x75, BYTE_LEN, 0},
       
{0x40, 0x87, BYTE_LEN ,0},
{0x41, 0x18, BYTE_LEN ,0},
{0x42, 0x91, BYTE_LEN ,0},
{0x43, 0x94, BYTE_LEN ,0},
{0x44, 0x9f, BYTE_LEN ,0},
{0x45, 0x33, BYTE_LEN ,0},
{0x46, 0x00, BYTE_LEN ,0},
{0x47, 0x94, BYTE_LEN ,0},
{0x48, 0x14, BYTE_LEN ,0},

//Gamma
//normal
{0x03,0x16, BYTE_LEN, 0},
{0x30,0x00, BYTE_LEN, 0},
{0x31,0x0a, BYTE_LEN, 0},
{0x32,0x1b, BYTE_LEN, 0},
{0x33,0x2e, BYTE_LEN, 0},
{0x34,0x5c, BYTE_LEN, 0},
{0x35,0x79, BYTE_LEN, 0},
{0x36,0x95, BYTE_LEN, 0},
{0x37,0xa4, BYTE_LEN, 0},
{0x38,0xb1, BYTE_LEN, 0},
{0x39,0xbd, BYTE_LEN, 0},
{0x3a,0xc8, BYTE_LEN, 0},
{0x3b,0xd9, BYTE_LEN, 0},
{0x3c,0xe8, BYTE_LEN, 0},
{0x3d,0xf5, BYTE_LEN, 0},
{0x3e,0xff, BYTE_LEN, 0},

//night mode
//{0x03,0x16, BYTE_LEN, 0},
//{0x30,0x00, BYTE_LEN, 0},
//{0x31,0x1c, BYTE_LEN, 0},
//{0x32,0x2d, BYTE_LEN, 0},
//{0x33,0x4e, BYTE_LEN, 0},
//{0x34,0x6d, BYTE_LEN, 0},
//{0x35,0x8b, BYTE_LEN, 0},
//{0x36,0xa2, BYTE_LEN, 0},
//{0x37,0xb5, BYTE_LEN, 0},
//{0x38,0xc4, BYTE_LEN, 0},
//{0x39,0xd0, BYTE_LEN, 0},
//{0x3a,0xda, BYTE_LEN, 0},
//{0x3b,0xea, BYTE_LEN, 0},
//{0x3c,0xf4, BYTE_LEN, 0},
//{0x3d,0xfb, BYTE_LEN, 0},
//{0x3e,0xff, BYTE_LEN, 0},

//Page 17 AE 
{0x03, 0x17, BYTE_LEN, 0},
{0xc4, 0x3c, BYTE_LEN, 0},
{0xc5, 0x32, BYTE_LEN, 0},

//Page 20 AE 
{0x03, 0x20, BYTE_LEN, 0},
{0x10, 0x0c, BYTE_LEN, 0},
{0x11, 0x04, BYTE_LEN, 0},
       
{0x20, 0x01, BYTE_LEN, 0},
{0x28, 0x27, BYTE_LEN, 0},
{0x29, 0xa1, BYTE_LEN, 0},

{0x2a, 0xf0, BYTE_LEN, 0},
{0x2b, 0x34, BYTE_LEN, 0},
{0x2c, 0x2b, BYTE_LEN, 0}, //23->2b 2010_04_06 hhzin
       
{0x30, 0xf8, BYTE_LEN, 0},

{0x39, 0x22, BYTE_LEN,0},
{0x3a, 0xde, BYTE_LEN,0},
{0x3b, 0x22, BYTE_LEN,0}, //23->22 _10_04_06 hhzin
{0x3c, 0xde, BYTE_LEN,0},

{0x60, 0x95, BYTE_LEN, 0}, //d5, 99
{0x68, 0x3c, BYTE_LEN, 0},
{0x69, 0x64, BYTE_LEN, 0},
{0x6A, 0x28, BYTE_LEN, 0},
{0x6B, 0xc8, BYTE_LEN, 0},

{0x70, 0x42, BYTE_LEN, 0},//Y Target 42

{0x76, 0x22, BYTE_LEN, 0}, //Unlock bnd1
{0x77, 0x02, BYTE_LEN, 0}, //Unlock bnd2 02->a2 _10_04_06 hhzin

{0x78, 0x12, BYTE_LEN, 0}, //Yth 1
{0x79, 0x27, BYTE_LEN, 0}, //Yth 2 26->27 _10_04_06 hhzin
{0x7a, 0x23, BYTE_LEN, 0}, //Yth 3

{0x7c, 0x1d, BYTE_LEN, 0}, //
{0x7d, 0x22, BYTE_LEN, 0},

//50Hz
{0x83, 0x00, BYTE_LEN, 0},//ExpTime 30fps
{0x84, 0xaf, BYTE_LEN, 0},
{0x85, 0xc8, BYTE_LEN, 0},

//60Hz
//{0x83, 0x00, BYTE_LEN, 0},//ExpTime 30fps
//{0x84, 0xc3, BYTE_LEN, 0},
//{0x85, 0x50, BYTE_LEN, 0},

{0x86, 0x00, BYTE_LEN, 0},//ExpMin
{0x87, 0xfa, BYTE_LEN, 0},

//50Hz_8fps
{0x88, 0x02,BYTE_LEN, 0},//ExpMax 8fps(8fps)
{0x89, 0xbf,BYTE_LEN, 0},
{0x8a, 0x20,BYTE_LEN, 0},

//50Hz_5fps
//{0x88, 0x04},//ExpMax 8fps(8fps)
//{0x89, 0x93},
//{0x8a, 0xe0},

//60Hz_8fps
//{0x88, 0x02},//ExpMax 8fps(8fps)
//{0x89, 0xdc},
//{0x8a, 0x6c},

{0x8b, 0x3a, BYTE_LEN, 0},//Exp100
{0x8c, 0x98, BYTE_LEN, 0},

{0x8d, 0x30, BYTE_LEN, 0},//Exp120
{0x8e, 0xd4, BYTE_LEN, 0},

{0x91, 0x02, BYTE_LEN, 0},
{0x92, 0xdc, BYTE_LEN, 0},
{0x93, 0x6c, BYTE_LEN, 0},

{0x94, 0x01, BYTE_LEN, 0}, //fix_step
{0x95, 0xb7, BYTE_LEN, 0},
{0x96, 0x74, BYTE_LEN, 0},

{0x98, 0x8C, BYTE_LEN, 0},
{0x99, 0x23, BYTE_LEN, 0},

{0x9c, 0x0b, BYTE_LEN, 0}, //4
{0x9d, 0x3b, BYTE_LEN, 0}, // 0x06d3 --> 0x0b3b
{0x9e, 0x00, BYTE_LEN, 0}, //4
{0x9f, 0xfa, BYTE_LEN, 0}, // 0x01f4 --> 0xfa

{0xb1, 0x14, BYTE_LEN, 0},
{0xb2, 0x50, BYTE_LEN, 0},
{0xb4, 0x14, BYTE_LEN, 0},
{0xb5, 0x38, BYTE_LEN, 0},
{0xb6, 0x26, BYTE_LEN, 0},
{0xb7, 0x20, BYTE_LEN, 0},
{0xb8, 0x1d, BYTE_LEN, 0},
{0xb9, 0x1b, BYTE_LEN, 0},
{0xba, 0x1a, BYTE_LEN, 0},
{0xbb, 0x19, BYTE_LEN, 0},
{0xbc, 0x19, BYTE_LEN, 0},
{0xbd, 0x18, BYTE_LEN, 0},

{0xc0, 0x1a, BYTE_LEN, 0},
{0xc3, 0x48, BYTE_LEN, 0},
{0xc4, 0x48, BYTE_LEN, 0},


//Page 22 AWB
{0x03, 0x22, BYTE_LEN, 0},
{0x10, 0xe2, BYTE_LEN, 0},
{0x11, 0x26, BYTE_LEN, 0},
{0x21, 0x40, BYTE_LEN, 0},
       
{0x30, 0x80, BYTE_LEN, 0},
{0x31, 0x80, BYTE_LEN, 0},
{0x38, 0x12, BYTE_LEN, 0},
{0x39, 0x33, BYTE_LEN, 0},
{0x40, 0xf0, BYTE_LEN, 0},
{0x41, 0x33, BYTE_LEN, 0},
{0x42, 0x33, BYTE_LEN, 0},
{0x43, 0xf3, BYTE_LEN, 0},
{0x44, 0x55, BYTE_LEN, 0},
{0x45, 0x44, BYTE_LEN, 0},
{0x46, 0x02, BYTE_LEN, 0},
       
{0x80, 0x45, BYTE_LEN, 0},
{0x81, 0x20, BYTE_LEN, 0},
{0x82, 0x40, BYTE_LEN, 0},//48
{0x83, 0x52, BYTE_LEN, 0}, //RMAX Default : 50 -> 48 -> 52 
{0x84, 0x1b, BYTE_LEN, 0}, //RMIN Default : 20
{0x85, 0x50, BYTE_LEN, 0}, //BMAX Default : 50, 5a -> 58 -> 55
{0x86, 0x25, BYTE_LEN, 0}, //BMIN Default : 20
{0x87, 0x4d, BYTE_LEN, 0}, //RMAXB Default : 50, 4d
{0x88, 0x38, BYTE_LEN, 0}, //RMINB Default : 3e, 45 --> 42
{0x89, 0x3e, BYTE_LEN, 0}, //BMAXB Default : 2e, 2d --> 30
{0x8a, 0x29, BYTE_LEN, 0}, //BMINB Default : 20, 22 --> 26 --> 29
{0x8b, 0x02, BYTE_LEN, 0}, //OUT TH
{0x8d, 0x22, BYTE_LEN, 0},
{0x8e, 0x71, BYTE_LEN, 0},

{0x8f, 0x63, BYTE_LEN, 0},
{0x90, 0x60, BYTE_LEN, 0},
{0x91, 0x5c, BYTE_LEN, 0},
{0x92, 0x56, BYTE_LEN, 0},
{0x93, 0x52, BYTE_LEN, 0},
{0x94, 0x4c, BYTE_LEN, 0},
{0x95, 0x36, BYTE_LEN, 0},
{0x96, 0x31, BYTE_LEN, 0},
{0x97, 0x2e, BYTE_LEN, 0},
{0x98, 0x2a, BYTE_LEN, 0},
{0x99, 0x29, BYTE_LEN, 0},
{0x9a, 0x26, BYTE_LEN, 0},
{0x9b, 0x09, BYTE_LEN, 0},
{0x03, 0x22, BYTE_LEN, 0},
{0x10, 0xfb, BYTE_LEN, 0},
{0x03, 0x20, BYTE_LEN, 0},
{0x10, 0x9c, BYTE_LEN, 0},
{0x01, 0xf0, BYTE_LEN, 0},


//{0x03, 0x00,BYTE_LEN,0},
//{0x11, 0x92,BYTE_LEN,0}, //by lijing0506
       
};

static struct hi704_i2c_reg_conf const noise_reduction_reg_settings_array[] = {
    /* add code here
        e.g. {0xXXXX, 0xXXXX, WORD_LEN, 0},
     */
    {0x0000, 0x0000, WORD_LEN, 0},
};

static struct hi704_i2c_reg_conf const lens_roll_off_tbl[] = {
    /* add code here
        e.g. {0xXXXX, 0xXXXX, WORD_LEN, 0},
     */
    {0x0000, 0x0000, WORD_LEN, 0},
};

static struct hi704_i2c_reg_conf const pll_setup_tbl[] = {
    /* add code here
        e.g. {0xXXXX, 0xXXXX, WORD_LEN, 0},
     */
    {0x0000, 0x0000, WORD_LEN, 0},
};

/* Refresh Sequencer */
static struct hi704_i2c_reg_conf const sequencer_tbl[] = {
    /* add code here
        e.g. {0xXXXX, 0xXXXX, WORD_LEN, 0},
     */
    {0x0000, 0x0000, WORD_LEN, 0},
};


 static struct hi704_i2c_reg_conf const antibanding_auto_tbl[] = {
    {0x03, 0x20, BYTE_LEN, 0},
    {0x10, 0x0c, BYTE_LEN, 0},

    {0x03, 0x17, BYTE_LEN, 0}, //Page 17 
    {0xC4, 0x3c, BYTE_LEN, 0}, //FLK200 
    {0xC5, 0x32, BYTE_LEN, 0}, //FLK240 

    {0x03, 0x20, BYTE_LEN, 0}, //Page 20
    {0x83, 0x00, BYTE_LEN, 0}, //EXP Normal 33.33 fps 
    {0x84, 0xaf, BYTE_LEN, 0}, 
    {0x85, 0xc8, BYTE_LEN, 0}, 
    {0x86, 0x00, BYTE_LEN, 0}, //EXPMin 6000.00 fps
    {0x87, 0xfa, BYTE_LEN, 0}, 
    {0x88, 0x02, BYTE_LEN, 0}, //EXP Max 8.33 fps 
    {0x89, 0xbf, BYTE_LEN, 0}, 
    {0x8a, 0x20, BYTE_LEN, 0}, 
    {0x8B, 0x3a, BYTE_LEN, 0}, //EXP100 
    {0x8C, 0x98, BYTE_LEN, 0}, 
    {0x8D, 0x30, BYTE_LEN, 0}, //EXP120 
    {0x8E, 0xd4, BYTE_LEN, 0}, 
    {0x9c, 0x06, BYTE_LEN, 0}, //EXP Limit 857.14 fps 
    {0x9d, 0xd6, BYTE_LEN, 0}, 
    {0x9e, 0x00, BYTE_LEN, 0}, //EXP Unit 
    {0x9f, 0xfa, BYTE_LEN, 0}, 

    {0x03, 0x20, BYTE_LEN, 0},
    {0x10, 0x9c, BYTE_LEN, 0},
    {0x10, 0xdc, BYTE_LEN, 0},
};

static struct hi704_i2c_reg_conf const antibanding_50_tbl[] = {
    {0x03, 0x20, BYTE_LEN, 0},
    {0x10, 0x0c, BYTE_LEN, 0},
                              
                              
    {0x03, 0x17, BYTE_LEN, 0}, //Page 17   
    {0xC4, 0x3c, BYTE_LEN, 0}, //FLK200 
    {0xC5, 0x32, BYTE_LEN, 0}, //FLK240 

    {0x03, 0x20, BYTE_LEN, 0}, //Page 20
    {0x83, 0x00, BYTE_LEN, 0}, //EXP Normal 33.33 fps 
    {0x84, 0xaf, BYTE_LEN, 0}, 
    {0x85, 0xc8, BYTE_LEN, 0}, 
    {0x86, 0x00, BYTE_LEN, 0}, //EXPMin 6000.00 fps
    {0x87, 0xfa, BYTE_LEN, 0}, 
    {0x88, 0x02, BYTE_LEN, 0}, //EXP Max 8.33 fps 
    {0x89, 0xbf, BYTE_LEN, 0}, 
    {0x8a, 0x20, BYTE_LEN, 0}, 
    {0x8B, 0x3a, BYTE_LEN, 0}, //EXP100 
    {0x8C, 0x98, BYTE_LEN, 0}, 
    {0x8D, 0x30, BYTE_LEN, 0}, //EXP120 
    {0x8E, 0xd4, BYTE_LEN, 0}, 
    {0x9c, 0x06, BYTE_LEN, 0}, //EXP Limit 857.14 fps 
    {0x9d, 0xd6, BYTE_LEN, 0}, 
    {0x9e, 0x00, BYTE_LEN, 0}, //EXP Unit 
    {0x9f, 0xfa, BYTE_LEN, 0}, 

    {0x03, 0x20, BYTE_LEN, 0},
    {0x10, 0x9c, BYTE_LEN, 0},

};

static struct hi704_i2c_reg_conf const antibanding_60_tbl[] = {
   {0x03, 0x20, BYTE_LEN, 0},
    {0x10, 0x0c, BYTE_LEN, 0},


    {0x03, 0x17, BYTE_LEN, 0}, //Page 17 
    {0xC4, 0x3c, BYTE_LEN, 0}, //FLK200 
    {0xC5, 0x32, BYTE_LEN, 0}, //FLK240 

    {0x03, 0x20, BYTE_LEN, 0}, //Page 20
    {0x83, 0x00, BYTE_LEN, 0}, //EXP Normal 33.33 fps 
    {0x84, 0xaf, BYTE_LEN, 0}, 
    {0x85, 0xc8, BYTE_LEN, 0}, 
    {0x86, 0x00, BYTE_LEN, 0}, //EXPMin 6000.00 fps
    {0x87, 0xfa, BYTE_LEN, 0}, 
    {0x88, 0x02, BYTE_LEN, 0}, //EXP Max 8.33 fps 
    {0x89, 0xbf, BYTE_LEN, 0}, 
    {0x8a, 0x20, BYTE_LEN, 0}, 
    {0x8B, 0x3a, BYTE_LEN, 0}, //EXP100 
    {0x8C, 0x98, BYTE_LEN, 0}, 
    {0x8D, 0x30, BYTE_LEN, 0}, //EXP120 
    {0x8E, 0xd4, BYTE_LEN, 0}, 
    {0x9c, 0x06, BYTE_LEN, 0}, //EXP Limit 857.14 fps 
    {0x9d, 0xd6, BYTE_LEN, 0}, 
    {0x9e, 0x00, BYTE_LEN, 0}, //EXP Unit 
    {0x9f, 0xfa, BYTE_LEN, 0}, 

    {0x03, 0x20, BYTE_LEN, 0},
    {0x10, 0x8c, BYTE_LEN, 0},
};


struct hi704_reg_t hi704_regs = {
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

