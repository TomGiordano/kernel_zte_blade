/*
 * drivers/media/video/msm/ov5642.h
 *
 * Refer to drivers/media/video/msm/mt9d112.h
 * For OV5642: 5.0Mp 1/4-Inch System-On-A-Chip (SOC) CMOS Digital Image Sensor
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
/*-----------------------------------------------------------------------------------------
  when         who          what, where, why                         comment tag
  --------     ----         -------------------------------------    ----------------------
 
------------------------------------------------------------------------------------------*/

#ifndef OV5640_H
#define OV5640_H

#include <mach/board.h>
#include <mach/camera.h>

extern struct ov5640_reg_t ov5640_regs;

enum ov5640_width_t {
    WORD_LEN,
    BYTE_LEN
};

struct ov5640_i2c_reg_conf {
    unsigned short waddr;
    unsigned short wdata;
    enum ov5640_width_t width;
    unsigned short mdelay_time;
};

struct ov5640_reg_t {
    struct ov5640_i2c_reg_conf const *prev_snap_reg_settings;
    uint16_t prev_snap_reg_settings_size;
    struct ov5640_i2c_reg_conf const *noise_reduction_reg_settings;
    uint16_t noise_reduction_reg_settings_size;
    struct ov5640_i2c_reg_conf const *plltbl;
    uint16_t plltbl_size;
    struct ov5640_i2c_reg_conf const *stbl;
    uint16_t stbl_size;
    struct ov5640_i2c_reg_conf const *rftbl;
    uint16_t rftbl_size;
    struct ov5640_i2c_reg_conf const *snapshot2preview_tbl;
    uint16_t snapshot2preview_size;    
    struct ov5640_i2c_reg_conf const *preview2snapshot_tbl;
    uint16_t preview2snapshot_size;    
    struct ov5640_i2c_reg_conf const *autofocus_reg_settings;
    uint16_t autofocus_reg_settings_size;
};

#endif /* OV5640_H */
