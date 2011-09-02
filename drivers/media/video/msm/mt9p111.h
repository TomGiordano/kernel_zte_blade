/*
 * drivers/media/video/msm/mt9p111.h
 *
 * Refer to drivers/media/video/msm/mt9d112.h
 * For MT9P111: 5.0Mp 1/4-Inch System-On-A-Chip (SOC) CMOS Digital Image Sensor
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

#ifndef MT9P111_H
#define MT9P111_H

#include <mach/board.h>
#include <mach/camera.h>

extern struct mt9p111_reg_t mt9p111_regs;

enum mt9p111_width_t {
    WORD_LEN,
    BYTE_LEN
};

struct mt9p111_i2c_reg_conf {
    unsigned short waddr;
    unsigned short wdata;
    enum mt9p111_width_t width;
    unsigned short mdelay_time;
};

struct mt9p111_eeprom_lsc_reg_conf {
    uint8_t     eeprom_addr;
    uint16_t    sensor_addr;
};

struct mt9p111_reg_t {
    struct mt9p111_i2c_reg_conf const *prev_snap_reg_settings;
    uint16_t prev_snap_reg_settings_size;

    struct mt9p111_i2c_reg_conf const *noise_reduction_reg_settings;
    uint16_t noise_reduction_reg_settings_size;

    struct mt9p111_i2c_reg_conf const *plltbl;
    uint16_t plltbl_size;

    struct mt9p111_i2c_reg_conf const *stbl;
    uint16_t stbl_size;

    struct mt9p111_i2c_reg_conf const *rftbl;
    uint16_t rftbl_size;

    struct mt9p111_i2c_reg_conf const *wb_cloudy_tbl;
    uint16_t wb_cloudy_tbl_sz;

    struct mt9p111_i2c_reg_conf const *wb_daylight_tbl;
    uint16_t wb_daylight_tbl_sz;

    struct mt9p111_i2c_reg_conf const *wb_flourescant_tbl;
    uint16_t wb_flourescant_tbl_sz;

    struct mt9p111_i2c_reg_conf const *wb_incandescent_tbl;
    uint16_t wb_incandescent_tbl_sz;

    struct mt9p111_i2c_reg_conf const *wb_auto_tbl;
    uint16_t wb_auto_tbl_sz;

    struct mt9p111_i2c_reg_conf const *af_tbl;
    uint16_t af_tbl_sz;

    struct mt9p111_i2c_reg_conf const **contrast_tbl;
    uint16_t const *contrast_tbl_sz;

    struct mt9p111_i2c_reg_conf const **brightness_tbl;
    uint16_t const *brightness_tbl_sz;

    struct mt9p111_i2c_reg_conf const **saturation_tbl;
    uint16_t const *saturation_tbl_sz;

    struct mt9p111_i2c_reg_conf const **sharpness_tbl;
    uint16_t const *sharpness_tbl_sz;

    struct mt9p111_i2c_reg_conf const **exposure_tbl;
    uint16_t const *exposure_tbl_sz;

    struct mt9p111_i2c_reg_conf const *lens_for_outdoor_tbl;
    uint16_t const lens_for_outdoor_tbl_sz;

    struct mt9p111_i2c_reg_conf const *lens_for_indoor_tbl;
    uint16_t const lens_for_indoor_tbl_sz;

    struct mt9p111_i2c_reg_conf const **otp_lsc_reg_tbl;
    uint16_t const *otp_lsc_reg_tbl_sz;

    struct mt9p111_eeprom_lsc_reg_conf *lsc_reg_addr_tbl;
    uint16_t const lsc_reg_addr_tbl_sz;
};

#endif /* MT9P111_H */
