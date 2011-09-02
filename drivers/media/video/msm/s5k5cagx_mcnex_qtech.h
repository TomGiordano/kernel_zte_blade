/*  
 * drivers/media/video/msm/s5k5cagx_mcnex_qtech.h
 * 
 * Refer to drivers/media/video/msm/mt9t11x.h
 * For S5K5CAGX_MCNEX_QTECH: 3.0Mp,Digital Image Sensor
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
 * Created by guoyanling
 */
/*-----------------------------------------------------------------------------------------
  when         who          what, where, why                         comment tag
  --------     ----         -------------------------------------    ----------------------

------------------------------------------------------------------------------------------*/

#ifndef S5K5CAGX_MCNEX_QTECH_H
#define S5K5CAGX_MCNEX_QTECH_H

#include <mach/board.h>
#include <mach/camera.h>

extern struct s5k5cagx_mcnex_qtech_base_reg_t s5k5cagx_mcnex_qtech_regs;

enum s5k5cagx_mcnex_qtech_width_t {
    WORD_LEN,
    BYTE_LEN
};

struct s5k5cagx_mcnex_qtech_i2c_reg_conf {
    unsigned short waddr;
    unsigned short wdata;
    enum s5k5cagx_mcnex_qtech_width_t width;
    unsigned short mdelay_time;
};

struct s5k5cagx_mcnex_qtech_base_reg_t {
    struct s5k5cagx_mcnex_qtech_i2c_reg_conf const *pll_tbl;
    uint16_t pll_tbl_sz;

    struct s5k5cagx_mcnex_qtech_i2c_reg_conf const *clk_tbl;
    uint16_t clk_tbl_sz;

    struct s5k5cagx_mcnex_qtech_i2c_reg_conf const *prevsnap_tbl;
    uint16_t prevsnap_tbl_sz;

    struct s5k5cagx_mcnex_qtech_i2c_reg_conf const *wb_cloudy_tbl;
    uint16_t wb_cloudy_tbl_sz;

    struct s5k5cagx_mcnex_qtech_i2c_reg_conf const *wb_daylight_tbl;
    uint16_t wb_daylight_tbl_sz;

    struct s5k5cagx_mcnex_qtech_i2c_reg_conf const *wb_flourescant_tbl;
    uint16_t wb_flourescant_tbl_sz;

    struct s5k5cagx_mcnex_qtech_i2c_reg_conf const *wb_incandescent_tbl;
    uint16_t wb_incandescent_tbl_sz;

    struct s5k5cagx_mcnex_qtech_i2c_reg_conf const *wb_auto_tbl;
    uint16_t wb_auto_tbl_sz;

    struct s5k5cagx_mcnex_qtech_i2c_reg_conf const *af_tbl;
    uint16_t af_tbl_sz;

    struct s5k5cagx_mcnex_qtech_i2c_reg_conf const **contrast_tbl;
    uint16_t const *contrast_tbl_sz;

    struct s5k5cagx_mcnex_qtech_i2c_reg_conf const **brightness_tbl;
    uint16_t const *brightness_tbl_sz;

    struct s5k5cagx_mcnex_qtech_i2c_reg_conf const **saturation_tbl;
    uint16_t const *saturation_tbl_sz;

    struct s5k5cagx_mcnex_qtech_i2c_reg_conf const **sharpness_tbl;
    uint16_t const *sharpness_tbl_sz;

    struct s5k5cagx_mcnex_qtech_i2c_reg_conf const **exposure_tbl;
    uint16_t const *exposure_tbl_sz;

    struct s5k5cagx_mcnex_qtech_i2c_reg_conf const *lens_for_outdoor_tbl;
    uint16_t const lens_for_outdoor_tbl_sz;

    struct s5k5cagx_mcnex_qtech_i2c_reg_conf const *lens_for_indoor_tbl;
    uint16_t const lens_for_indoor_tbl_sz;

    struct s5k5cagx_mcnex_qtech_i2c_reg_conf const **iso_tbl;
    uint16_t const *iso_tbl_sz;  
};

#endif /* S5K5CAGX_MCNEX_QTECH_H */
