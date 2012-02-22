/*
 * drivers/media/video/msm/mt9v113.h
 *
 * Refer to drivers/media/video/msm/mt9d112.h
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
  2011-04-15   lijing       add setting array                        ZTE_CAM_LJ_20100415
  2009-10-24   jia.jia      Merged from kernel-v4515                 ZTE_MSM_CAMERA_JIA_001
------------------------------------------------------------------------------------------*/

#ifndef OVM7690_H
#define OVM7690_H

#include <mach/board.h>
#include <mach/camera.h>

extern struct ovm7690_reg_t ovm7690_regs;

enum ovm7690_width_t {
    WORD_LEN,
    BYTE_LEN
};

struct ovm7690_i2c_reg_conf {
    unsigned short waddr;
    unsigned short wdata;
    enum ovm7690_width_t width;
    unsigned short mdelay_time;
};

struct ovm7690_reg_t {
    struct ovm7690_i2c_reg_conf const *prev_snap_reg_settings;
    uint16_t prev_snap_reg_settings_size;
    struct ovm7690_i2c_reg_conf const *noise_reduction_reg_settings;
    uint16_t noise_reduction_reg_settings_size;
    struct ovm7690_i2c_reg_conf const *plltbl;
    uint16_t plltbl_size;
    struct ovm7690_i2c_reg_conf const *stbl;
    uint16_t stbl_size;
    struct ovm7690_i2c_reg_conf const *rftbl;
    uint16_t rftbl_size;

    /*
     * ZTE_CAM_LJ_20100415
     * add antibanding array
     */
    struct ovm7690_i2c_reg_conf const *antibanding_auto_tbl;
    uint16_t antibanding_auto_tbl_size;

    struct ovm7690_i2c_reg_conf const *antibanding_50_tbl;
    uint16_t antibanding_50_tbl_size;

    struct ovm7690_i2c_reg_conf const *antibanding_60_tbl;
    uint16_t antibanding_60_tbl_size;
};

#endif /* OVM7690_H */
