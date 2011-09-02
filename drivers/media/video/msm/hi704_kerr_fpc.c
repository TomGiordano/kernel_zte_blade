/*
 * drivers/media/video/msm/mt9v113.c
 *
 * Refer to drivers/media/video/msm/mt9d112.c
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

#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <media/msm_camera.h>
#include <mach/gpio.h>
#include "hi704.h"
#include <linux/slab.h>

/*
 * Micron MT9V113 Registers and their values
 */
/* Sensor I2C Slave Address */
#define HI704_SLAVE_WR_ADDR 0x60 /* replaced by "msm_i2c_devices.addr" */
#define HI704_SLAVE_RD_ADDR 0x61

/* Sensor I2C Device ID */
#define REG_HI704_MODEL_ID    0x04
#define HI704_MODEL_ID        0x96

/* SOC Registers */
//#define REG_MT9V113_SENSOR_RESET     0x001A
//#define REG_MT9V113_STANDBY_CONTROL  0x0018
//#define REG_MT9V113_MCU_BOOT         0x001C

/* CAMIO Input MCLK (MHz) */
#define HI704_CAMIO_MCLK  24000000 /*48000000*/

/* GPIO For Sensor Clock Switch */
#define HI704_GPIO_SWITCH_CTL     107

#define HI704_GPIO_SWITCH_VAL     1


struct hi704_work_t {
    struct work_struct work;
};

static struct hi704_work_t *hi704_sensorw;
static struct i2c_client *hi704_client;

struct hi704_ctrl_t {
    const struct msm_camera_sensor_info *sensordata;
};

static struct hi704_ctrl_t *hi704_ctrl;

static DECLARE_WAIT_QUEUE_HEAD(hi704_wait_queue);
DECLARE_MUTEX(hi704_sem);

extern int32_t msm_camera_power_frontend(enum msm_camera_pwr_mode_t pwr_mode);
extern int msm_camera_clk_switch(const struct msm_camera_sensor_info *data,
                                         uint32_t  gpio_switch,
                                         uint32_t  switch_val);
/*=============================================================*/

/*
 * Hard standby of sensor
 * on: =1, enter hard standby
 * on: =0, exit hard standby
 *
 * Hard standby mode is set by register of REG_MT9V113_STANDBY_CONTROL.
 */
static int __attribute__((unused)) mt9v113_hard_standby(const struct msm_camera_sensor_info *dev, uint32_t on)
{
    int rc;
    
    pr_err("%s: entry\n", __func__);
    
    rc = gpio_request(dev->sensor_pwd, "hi704");
    if (0 == rc)
    {
        /* ignore "rc" */
        rc = gpio_direction_output(dev->sensor_pwd, on);
        /* time delay for enterring into standby */
        mdelay(30);
    }

    gpio_free(dev->sensor_pwd);

    return rc;
}

/*
 * Soft reset has the same effect as the hard reset.
 */
 #if 0
static int hi704_hard_reset(const struct msm_camera_sensor_info *dev)
{
    int rc = 0;
    
    pr_err("%s: entry\n", __func__);

    rc = gpio_request(dev->sensor_reset, "hi704");
    if (0 == rc)
    {
        /* ignore "rc" */
        rc = gpio_direction_output(dev->sensor_reset, 1);

        mdelay(10);

        /* ignore "rc" */
        rc = gpio_direction_output(dev->sensor_reset, 0);

        /*
          * RESET_BAR pulse width: Min 70 EXTCLKs
          * EXTCLKs: = MCLK (i.e., MT9P111_CAMIO_MCLK)
          */
        mdelay(10);

        /* ignore "rc" */
        rc = gpio_direction_output(dev->sensor_reset, 1);

        /*
          * Time delay before first serial write: Min 100 EXTCLKs
          * EXTCLKs: = MCLK (i.e., MT9P111_CAMIO_MCLK)
          */
        mdelay(10);
    }

    gpio_free(dev->sensor_reset);

    return rc;
}
#endif
static int32_t hi704_i2c_txdata(unsigned short saddr,
                                       unsigned char *txdata,
                                       int length)
{
    struct i2c_msg msg[] = {
        {
            .addr = saddr,
            .flags = 0,
            .len    = length,
            .buf    = txdata,
        },
    };
    
    if (i2c_transfer(hi704_client->adapter, msg, 1) < 0)
    {
        CCRT("%s: failed!\n", __func__);
        return -EIO;
    }

    return 0;
}

static int32_t hi704_i2c_write(unsigned short saddr,
                                      unsigned short waddr,
                                      unsigned short wdata,
                                      enum hi704_width_t width)
{
    int32_t rc = -EFAULT;
    unsigned char buf[4];

    memset(buf, 0, sizeof(buf));

    switch (width)
    {
        case WORD_LEN:
        {
            buf[0] = (waddr & 0xFF00) >> 8;
            buf[1] = (waddr & 0x00FF);
            buf[2] = (wdata & 0xFF00) >> 8;
            buf[3] = (wdata & 0x00FF);

            rc = hi704_i2c_txdata(saddr, buf, 4);
        }
        break;

        case BYTE_LEN:
        {
            buf[0] = waddr;
            buf[1] = wdata;

            rc = hi704_i2c_txdata(saddr, buf, 2);
        }
        break;

        default:
        {
        }
        break;
    }

    if (rc < 0)
    {
        CCRT("%s: saddr = 0x%x, waddr = 0x%x, wdata = 0x%x, failed!\n", __func__, saddr,waddr, wdata);
    }

    return rc;
}

/*static int32_t mt9v113_i2c_write_table(struct mt9v113_i2c_reg_conf const *reg_conf_tbl,
                                             int num_of_items_in_table)
{

    int i;
    int32_t rc = -EFAULT;
    CCRT("%s: entry\n", __func__);
    for (i = 0; i < num_of_items_in_table; i++)
    {
        rc = mt9v113_i2c_write(mt9v113_client->addr,
                               reg_conf_tbl->waddr,
                               reg_conf_tbl->wdata,
                               reg_conf_tbl->width);
        if (rc < 0)
        {
            break;
        }

        if (reg_conf_tbl->mdelay_time != 0)
        {
            mdelay(reg_conf_tbl->mdelay_time);
        }
        
        reg_conf_tbl++;
    }

    return rc;
}*/
#if 1
static int hi704_i2c_rxdata(unsigned short saddr,
                                    unsigned char *rxdata,
                                    int length)
{
    struct i2c_msg msgs[] = {
    {
        .addr   = saddr,
        .flags = 0,
        .len   = 1,
        .buf   = rxdata,
    },
    {
        .addr   = saddr,
        .flags = I2C_M_RD,
        .len   = length,
        .buf   = rxdata,
    },
    };
    
    if (i2c_transfer(hi704_client->adapter, msgs, 2)  <  0)
    {
        CCRT("%s: failed!\n", __func__);
        return -EIO;
    }

    return 0;
}


static int32_t hi704_i2c_read(unsigned short saddr,
                                     unsigned short raddr,
                                     unsigned short *rdata,
                                     enum hi704_width_t width)
{
    int32_t rc = 0;
    unsigned char buf[4];

    if (!rdata)
    {
        CCRT("%s: rdata points to NULL!\n", __func__);
        return -EIO;
    }

    memset(buf, 0, sizeof(buf));

    switch (width)
    {
        case WORD_LEN:
        {
            buf[0] = (raddr & 0xFF00) >> 8;
            buf[1] = (raddr & 0x00FF);

            rc = hi704_i2c_rxdata(saddr, buf, 2);
            if (rc < 0)
            {
                return rc;
            }

            *rdata = buf[0] << 8 | buf[1];
        }
        break;

        case BYTE_LEN:
        {
            buf[0] = raddr;

            rc = hi704_i2c_rxdata(saddr, buf, 1);
            if (rc < 0)
            {
                return rc;
            }

            *rdata = buf[0]| 0x0000;
        }
        break;

        default:
        {
        }
        break;
    }

    if (rc < 0)
    {
        CCRT("%s: failed!\n", __func__);
    }

    return rc;
}
#endif

static long hi704_set_sensor_mode(int32_t mode)
{
    return 0;
}

static long hi704_set_effect(int32_t mode, int32_t effect)
{
    long rc = 0;

    switch (mode)
    {
        case SENSOR_PREVIEW_MODE:
        {
            /* Context A Special Effects */
            /* add code here
                 e.g.
                 reg_addr = 0xXXXX;
               */
        }
        break;

        case SENSOR_SNAPSHOT_MODE:
        {
            /* Context B Special Effects */
            /* add code here
                 e.g.
                 reg_addr = 0xXXXX;
               */
        }
        break;

        default:
        {
            /* add code here
                 e.g.
                 reg_addr = 0xXXXX;
               */
        }
        break;
    }

    switch (effect)
    {
        case CAMERA_EFFECT_OFF:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x10,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x11,0x03,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x12,0x30,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x13,0x00,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x44,0x80,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x45,0x80,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x47,0x7f,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x13,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x20,0x06,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x21,0x04,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }

        }            
        break;

        case CAMERA_EFFECT_MONO:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x10,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x11,0x03,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x12,0x23,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x13,0x00,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x44,0x80,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x45,0x80,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x47,0x7f,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x13,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x20,0x07,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x21,0x03,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }       
        }
        break;

        case CAMERA_EFFECT_NEGATIVE:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x10,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x11,0x03,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x12,0x28,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x13,0x00,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x44,0x80,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x45,0x80,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x47,0x7f,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x13,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x20,0x07,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x21,0x03,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }         
        }
        break;

        case CAMERA_EFFECT_SOLARIZE:
        {
                     
        }            
        break;

        case CAMERA_EFFECT_SEPIA:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x10,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x11,0x03,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x12,0x23,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x13,0x00,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x44,0x70,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x45,0x98,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x47,0x7f,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x13,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x20,0x07,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x21,0x03,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }           
        }         
        break;

        default:
        {
            /* add code here
                 e.g.
                 reg_val = 0xXXXX;

                 rc = mt9t11x_i2c_write(mt9t11x_client->addr, 0xXXXX, reg_addr, WORD_LEN);
                 if (rc < 0)
                 {
                    return rc;
                 }
               */

            return -EFAULT;
        }
    }

    return rc;
}


/*
 * White Balance Setting
 */
static int32_t hi704_set_wb(int8_t wb_mode)
{
    int32_t rc = 0;

    CDBG("%s: entry: wb_mode=%d\n", __func__, wb_mode);

    switch (wb_mode)
    {
        case CAMERA_WB_MODE_AWB:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x22,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x10,0x6a,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x80,0x48,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x81,0x20,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x82,0x40,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x83,0x58,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x84,0x10,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x85,0x70,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x86,0x10,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x10,0xea,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }  
        }
        break;

        case CAMERA_WB_MODE_SUNLIGHT:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x22,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x10,0x6a,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x80,0x50,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x81,0x20,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x82,0x2d,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x83,0x52,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x84,0x40,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x85,0x30,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x86,0x1c,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;

        case CAMERA_WB_MODE_INCANDESCENT:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x22,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x10,0x6a,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x80,0x26,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x81,0x20,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x82,0x55,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x83,0x24,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x84,0x1e,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x85,0x58,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x86,0x4a,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;
        
        case CAMERA_WB_MODE_FLUORESCENT:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x22,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x10,0x6a,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x80,0x40,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x81,0x20,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x82,0x4f,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x83,0x44,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x84,0x3a,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x85,0x47,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x86,0x3a,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break; 

        case CAMERA_WB_MODE_CLOUDY:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x22,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x10,0x6a,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x80,0x62,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x81,0x20,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x82,0x2e,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x83,0x6d,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x84,0x65,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x85,0x30,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x86,0x25,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;

        default:
        {
            CCRT("%s: parameter error!\n", __func__);
            rc = -EFAULT;
        }     
    }


	return rc;
}    

static int32_t hi704_set_contrast(int8_t contrast)
{
    int32_t rc = 0;

    CDBG("lijing:%s: entry: contrast=%d\n", __func__, contrast);

    switch (contrast)
    {
        case CAMERA_CONTRAST_0:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x10,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x48,0x54,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;

        case CAMERA_CONTRAST_1:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x10,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x48,0x74,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;

        case CAMERA_CONTRAST_2:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x10,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x48,0x84,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;
        
        case CAMERA_CONTRAST_3:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x10,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x48,0x94,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break; 

        case CAMERA_CONTRAST_4:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x10,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x48,0xb4,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;

        default:
        {
            CCRT("%s: parameter error!\n", __func__);
            rc = -EFAULT;
        }     
    }
    return rc;
}


static int32_t hi704_set_exposure_compensation(int8_t exposure)
{
    int32_t rc = 0;

    CDBG("lijing:%s: entry: exposure=%d\n", __func__, exposure);

    switch (exposure)
    {
        case CAMERA_EXPOSURE_0:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x20,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x70,0x2a,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;

        case CAMERA_EXPOSURE_1:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x20,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x70,0x3a,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;

        case CAMERA_EXPOSURE_2:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x20,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x70,0x42,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;
        
        case CAMERA_EXPOSURE_3:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x20,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x70,0x4a,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break; 

        case CAMERA_EXPOSURE_4:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x20,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x70,0x5a,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;

        default:
        {
            CCRT("%s: parameter error!\n", __func__);
            rc = -EFAULT;
        }     
    }

    
    return rc;
}  


static int32_t hi704_set_brightness(int8_t brightness)
{
    int32_t rc = 0;

    CDBG("%s: entry: brightness=%d\n", __func__, brightness);

    switch (brightness)
    {
        case CAMERA_BRIGHTNESS_0:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x10,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x40,0xc0,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;

        case CAMERA_BRIGHTNESS_1:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x10,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x40,0xa0,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;

        case CAMERA_BRIGHTNESS_2:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x10,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x40,0x90,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;
        
        case CAMERA_BRIGHTNESS_3:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x10,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x40,0x00,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break; 

        case CAMERA_BRIGHTNESS_4:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x10,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x40,0x10,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;
        
        case CAMERA_BRIGHTNESS_5:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x10,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x40,0x20,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;
        
        case CAMERA_BRIGHTNESS_6:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x10,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x40,0x30,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }        
        break;

        default:
        {
            CCRT("%s: parameter error!\n", __func__);
            rc = -EFAULT;
        }     
    }

	return rc;
}    

static int32_t hi704_set_saturation(int8_t saturation)
{
    int32_t rc = 0;

    CDBG("lijing:%s: entry: saturation=%d\n", __func__, saturation);

    switch (saturation)
    {
        case CAMERA_SATURATION_0:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x10,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x62,0x70,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x63,0x60,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;

        case CAMERA_SATURATION_1:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x10,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x62,0x80,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x63,0x80,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;

        case CAMERA_SATURATION_2:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x10,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x62,0x90,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x63,0x90,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;
        
        case CAMERA_SATURATION_3:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x10,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x62,0xa0,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x63,0xa0,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break; 

        case CAMERA_SATURATION_4:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x10,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x62,0xc0,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x63,0xc0,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;

        default:
        {
            CCRT("%s: parameter error!\n", __func__);
            rc = -EFAULT;
        }     
    }
    return rc;
}    

static int32_t hi704_set_sharpness(int8_t sharpness)
{
    int32_t rc = 0;
    //uint16_t brightness_lev = 0;

    CDBG("lijing:%s: entry: sharpness=%d\n", __func__, sharpness);
    switch (sharpness)
    {
        case CAMERA_SHARPNESS_0:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x13,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x90,0x04,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x91,0x00,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;

        case CAMERA_SHARPNESS_1:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x13,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x90,0x04,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x91,0x02,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;

        case CAMERA_SHARPNESS_2:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x13,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x90,0x04,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x91,0x06,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;
        
        case CAMERA_SHARPNESS_3:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x13,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x90,0x04,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x91,0x0a,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break; 

        case CAMERA_SHARPNESS_4:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x13,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x90,0x04,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x91,0x0f,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;        
       default:
        {
            CCRT("%s: parameter error!\n", __func__);
            rc = -EFAULT;
        }     
    }
    return rc;

}    


/*
 * ISO Setting
 */
static int32_t hi704_set_iso(int8_t iso_val)
{
    int32_t rc = 0;

    CDBG("lijing:%s: entry: iso_val=%d\n", __func__, iso_val);

    switch (iso_val)
    {
        case CAMERA_ISO_SET_AUTO:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x20,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0xb0,0x18,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;

        case CAMERA_ISO_SET_HJR:
        {        
        }
        break;

        case CAMERA_ISO_SET_100:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x20,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x10,0x0c,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0xb0,0x10,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;

        case CAMERA_ISO_SET_200:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x20,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x10,0x0c,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0xb0,0x50,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;

        case CAMERA_ISO_SET_400:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x20,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x10,0x0c,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0xb0,0x90,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;

        case CAMERA_ISO_SET_800:
        {
            rc = hi704_i2c_write(hi704_client->addr,0x03,0x20,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0x10,0x0c,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = hi704_i2c_write(hi704_client->addr,0xb0,0xd0,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;

        default:
        {
            CCRT("%s: parameter error!\n", __func__);
            rc = -EFAULT;
        }     
    }
    return rc;
} 

/*
 * Antibanding Setting
 */
static int32_t  hi704_set_antibanding(int8_t antibanding)
{
    int32_t rc = 0;
    int32_t array_length;
    int32_t i;

    
    CDBG("lijing:%s: entry: antibanding=%d\n", __func__, antibanding);

    switch (antibanding)
    {
        case CAMERA_ANTIBANDING_SET_OFF:
        {
            CCRT("%s: CAMERA_ANTIBANDING_SET_OFF NOT supported!\n", __func__);
        }
        break;

        case CAMERA_ANTIBANDING_SET_60HZ:
        {
            array_length = hi704_regs.antibanding_60_tbl_size;

            /* Configure sensor for Preview mode and Snapshot mode */
            for (i = 0; i < array_length; i++)
            {
                rc = hi704_i2c_write(hi704_client->addr,
                                       hi704_regs.antibanding_60_tbl[i].waddr,
                                       hi704_regs.antibanding_60_tbl[i].wdata,
                                       BYTE_LEN);

                if (rc < 0)
                {
                    pr_err("write reg failed");
                    return rc;
                }
             }
        }            
        break;

        case CAMERA_ANTIBANDING_SET_50HZ:
        {
            array_length = hi704_regs.antibanding_50_tbl_size;

            /* Configure sensor for Preview mode and Snapshot mode */
            for (i = 0; i < array_length; i++)
            {
                rc = hi704_i2c_write(hi704_client->addr,
                                       hi704_regs.antibanding_50_tbl[i].waddr,
                                       hi704_regs.antibanding_50_tbl[i].wdata,
                                       BYTE_LEN);

                if (rc < 0)
                {
                    pr_err("write reg failed");
                    return rc;
                }
             }       
        }
        break;

        case CAMERA_ANTIBANDING_SET_AUTO:
        {
            array_length = hi704_regs.antibanding_auto_tbl_size;

            /* Configure sensor for Preview mode and Snapshot mode */
            for (i = 0; i < array_length; i++)
            {
                rc = hi704_i2c_write(hi704_client->addr,
                                       hi704_regs.antibanding_auto_tbl[i].waddr,
                                       hi704_regs.antibanding_auto_tbl[i].wdata,
                                       BYTE_LEN);

                if (rc < 0)
                {
                    pr_err("write reg failed");
                    return rc;
                }
             }        
        }
        break;

        default:
        {
            CCRT("%s: parameter error!\n", __func__);
            rc = -EFAULT;
        }     
    }

    return rc;
} 
/*static int32_t mt9v113_set_lens_roll_off(void)
{

    int32_t rc = 0;
    CCRT("%s: entry\n", __func__);
    rc = mt9v113_i2c_write_table(&mt9v113_regs.rftbl[0], mt9v113_regs.rftbl_size);
    return rc;
}*/
    
/*
 * Power-down Steps:
 *
 * Configure sensor as standby mode;
 * Assert standby pin;
 * Time delay for 200ms;
 * Cut off MCLK;
 */

/*
 * Power-up Process
 */
static long hi704_power_up(void)
{
    CDBG("%s: not supported!\n", __func__);
    return 0;
}
static long hi704_power_down(const struct msm_camera_sensor_info *dev, uint32_t on)
{
    long rc = 0;

    CDBG("%s: entry\n", __func__);
    
    rc = gpio_request(dev->sensor_pwd, "hi704");
    if (0 == rc)
    {
        /* ignore "rc" */
        rc = gpio_direction_output(dev->sensor_pwd, on);

        /*
         * time delay for entry into standby
         * decrease time delay from 200ms to 10ms
         * to improve sensor init
         */
         CDBG("hi704_power_down:rc=%ld",rc);
        mdelay(10);
    }

    gpio_free(dev->sensor_pwd);

    return rc;

}


static long hi704_reg_init(void)
{

    int32_t array_length;
    int32_t i;
    long rc;
    
    CDBG("%s: entry\n", __func__);

    array_length = hi704_regs.prev_snap_reg_settings_size;

    /* Configure sensor for Preview mode and Snapshot mode */
    for (i = 0; i < array_length; i++)
    {
        rc = hi704_i2c_write(hi704_client->addr,
                               hi704_regs.prev_snap_reg_settings[i].waddr,
                               hi704_regs.prev_snap_reg_settings[i].wdata,
                               BYTE_LEN);

        if (rc < 0)
        {
            pr_err("write reg failed");
            return rc;
        }
     }

    return 0;
}

#if 0
static long hi704_set_effect(int32_t mode, int32_t effect)
{

#if 0
    uint16_t __attribute__((unused)) reg_addr;
    uint16_t __attribute__((unused)) reg_val;
    long rc = 0;
    
    pr_err("%s: entry\n", __func__);
    
    switch (mode)
    {
        case SENSOR_PREVIEW_MODE:
        {
               reg_addr = 0x2759;
        }
        break;

        case SENSOR_SNAPSHOT_MODE:
        {
               reg_addr = 0x275B;
        }
        break;

        default:
        {
               reg_addr = 0x2759;   //default: SENSOR_PREVIEW_MODE
        }
        break;
    }

    switch (effect)
    {
        case CAMERA_EFFECT_OFF:
        {            
            rc = fi704_i2c_write(mt9v113_client->addr, 0x098C, reg_addr, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            
            reg_val = 0x6440;
            rc = fi704_i2c_write(mt9v113_client->addr, 0x0990, reg_val, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            
            break;
        }
        
        case CAMERA_EFFECT_MONO:
        {            
            rc = fi704_i2c_write(mt9v113_client->addr, 0x098C, reg_addr, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            
            reg_val = 0x6441;
            rc = fi704_i2c_write(mt9v113_client->addr, 0x0990, reg_val, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            
            break;
        }
        
        case CAMERA_EFFECT_NEGATIVE:
        {            
            rc = fi704_i2c_write(mt9v113_client->addr, 0x098C, reg_addr, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            
            reg_val = 0x6443;
            rc = fi704_i2c_write(mt9v113_client->addr, 0x0990, reg_val, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            
            break;
        }
        
        case CAMERA_EFFECT_SOLARIZE:
        {            
            rc = fi704_i2c_write(mt9v113_client->addr, 0x098C, reg_addr, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            
            reg_val = 0x6445;                    
            rc = fi704_i2c_write(mt9v113_client->addr, 0x0990, reg_val, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            
            break;
        }
        case CAMERA_EFFECT_SEPIA:
        {            
            rc = fi704_i2c_write(mt9v113_client->addr, 0x098C, reg_addr, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            
            reg_val = 0x6442;
            rc = fi704_i2c_write(mt9v113_client->addr, 0x0990, reg_val, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            
            break;            
        }
        
        default:
        {        
            rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, reg_addr, WORD_LEN);
            if (rc < 0)
            {
                 return rc;
            }

            reg_val = 0x6442;
            rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, reg_val, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }        
        }
    }

    /* Refresh Sequencer */
    /* add code here
        e.g.
        rc = mt9v113_i2c_write(mt9v113_client->addr, 0xXXXX, 0xXXXX, WORD_LEN);
        if (rc < 0)
        {
            return rc;
        }
      */

    return rc;
#else
    return 0;
#endif
}
#endif
#if 0
static long _set_sensor_mode(int32_t mode)
{
#if 0
long __attribute__((unused)) rc = 0;
    
    pr_err("%s: entry\n", __func__);
    
    switch (mode)
    {
        case SENSOR_PREVIEW_MODE:
        {
            rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }

            /*
               * SEQ_STATE_CFG_5_MAX_FRAME_CNT
               */
            rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0001, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }

            rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA115, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }

            /*
               * SEQ_CMD
               */
            rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0000, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            mdelay(5);
            rc = mt9v113_set_effect(SENSOR_PREVIEW_MODE, CAMERA_EFFECT_OFF);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;

        case SENSOR_SNAPSHOT_MODE:
        {
            /* Switch to lower fps for Snapshot */             
            rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA115, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }

            /*
               * SEQ_STATE_CFG_5_MAX_FRAME_CNT
               */
            rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0002, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            mdelay(5);
            rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }

            /*
               * SEQ_CMD
               */
            rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0002, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
        
            rc = mt9v113_set_effect(SENSOR_SNAPSHOT_MODE, CAMERA_EFFECT_OFF);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;

        default:
        {
            return -EFAULT;
        }
    }
#endif
    return 0;
}

#endif

static int hi704_sensor_probe_init(const struct msm_camera_sensor_info *data)
{
    uint16_t model_id = 0;
    int rc;

    pr_err("%s: entry\n", __func__);

    if (!data || strcmp(data->sensor_name, "hi704"))
    {
        CCRT("%s: invalid parameters!\n", __func__);
        rc = -ENODEV;
        goto probe_init_fail;
    }

    hi704_ctrl = kzalloc(sizeof(struct hi704_ctrl_t), GFP_KERNEL);
    if (!hi704_ctrl)
    {
        CCRT("%s: kzalloc failed!\n", __func__);
        rc = -ENOMEM;
        goto probe_init_fail;
    }

    hi704_ctrl->sensordata = data;
    /*
      * Power sensor on
      */
    rc = msm_camera_power_frontend(MSM_CAMERA_PWRUP_MODE);
    if (rc < 0)
    {
        CCRT("%s: camera_power_frontend failed!\n", __func__);
        goto probe_init_fail;
    }

    hi704_power_down(hi704_ctrl->sensordata,1);

    mdelay(5);
    
    /*
     * Camera clock switch for both frontend and backend sensors, i.e., MT9V113 and MT9P111
     *
     * For MT9V113: 0.3Mp, 1/11-Inch System-On-A-Chip (SOC) CMOS Digital Image Sensor
     * For MT9P111: 5.0Mp, 1/4-Inch System-On-A-Chip (SOC) CMOS Digital Image Sensor
     *
     * switch_val: 0, to switch clock to frontend sensor, i.e., MT9V113, or
     *             1, to switch clock to backend sensor, i.e., MT9P111
     */
    rc = msm_camera_clk_switch(hi704_ctrl->sensordata, HI704_GPIO_SWITCH_CTL, 1);
    if (rc < 0)
    {
        CCRT("%s: camera_clk_switch failed!\n", __func__);
        goto probe_init_fail;
    }

    /* Input MCLK */
    msm_camio_clk_rate_set(HI704_CAMIO_MCLK);
    mdelay(5);

    //msm_camio_camif_pad_reg_reset();

    //mdelay(10);

    hi704_power_down(hi704_ctrl->sensordata,0);

    mdelay(10);

    rc = hi704_i2c_write(hi704_client->addr, 0x03, 0x00, BYTE_LEN);// RESET_REGISTER
    if (rc < 0)
    {
        return rc;
    }
    rc = hi704_i2c_read(hi704_client->addr, 0x04, &model_id, BYTE_LEN);// RESET_REGISTER
    if (rc < 0)
    {
        return rc;
    }
   

    pr_err("%s: model_id = 0x%x\n", __func__, model_id);

    /* Check if it matches it with the value in Datasheet */
    if (model_id != HI704_MODEL_ID)
    {
        rc = -EFAULT;
        goto probe_init_fail;
    }


    return 0;

probe_init_fail:
    /*
      * To shut sensor down
      * Ignore "rc"
      */
    hi704_power_down(hi704_ctrl->sensordata,1);
    msm_camera_power_frontend(MSM_CAMERA_PWRDWN_MODE);
    kfree(hi704_ctrl);
    return rc;
}
static int hi704_sensor_init_probe(const struct msm_camera_sensor_info *data)
{
    uint16_t model_id = 0;
    int rc = 0;
//    uint16_t reg_value = 0;
    
    CDBG("%s: entry\n", __func__);

    /* ".sensor_pwd" NOT used
        switch_on = 1; //pwd active HIGH, if switch_on = 0, i2c R/W will fail.
        rc = mt9v113_hard_standby(data, switch_on);
        if (rc < 0)
        {
            CCRT("set standby failed!\n");
            goto init_probe_fail;
        }
     */

    /* Enable Hard Reset */
    /*rc = hi704_hard_reset(data);
    if (rc < 0)
    {
        CCRT("reset failed!\n");
        goto init_probe_fail;
    }
    
    rc = hi704_power_up();
    if (rc < 0)
    {
        return rc;
    }*/
#if 1
    /* Read the Model ID of the sensor */
     rc = hi704_i2c_write(hi704_client->addr, 0x03, 0x00, BYTE_LEN);// RESET_REGISTER
    if (rc < 0)
    {
        return rc;
    }
    rc = hi704_i2c_read(hi704_client->addr, 0x04, &model_id, BYTE_LEN);// RESET_REGISTER
    if (rc < 0)
    {
        return rc;
    }
   

    pr_err("%s: model_id = 0x%x\n", __func__, model_id);

    /* Check if it matches it with the value in Datasheet */
    if (model_id != HI704_MODEL_ID)
    {
        rc = -EFAULT;
        goto init_probe_fail;
    }
#endif
#if 1
    rc = hi704_reg_init();
    if (rc < 0)
    {
        goto init_probe_fail;
    }
#endif
    return rc;

init_probe_fail:
    CCRT("%s: rc = %d, failed!\n", __func__, rc);
    return rc;
}

int hi704_sensor_init(const struct msm_camera_sensor_info *data)
{
    int rc = 0;

    CDBG("%s: entry\n", __func__);

    hi704_ctrl = kzalloc(sizeof(struct hi704_ctrl_t), GFP_KERNEL);
    if (!hi704_ctrl)
    {
        CCRT("%s: kzalloc failed!\n", __func__);
        rc = -ENOMEM;
        goto init_done;
    }

    if (data)
    {
        hi704_ctrl->sensordata = data;
    }

    /*
      * Power sensor on
      */
    rc = msm_camera_power_frontend(MSM_CAMERA_PWRUP_MODE);
    if (rc < 0)
    {
        CCRT("%s: camera_power_frontend failed!\n", __func__);
        goto init_fail;
    }

    hi704_power_down(hi704_ctrl->sensordata,1);

    mdelay(5);
    
    /*
     * Camera clock switch for both frontend and backend sensors, i.e., MT9V113 and MT9P111
     *
     * For MT9V113: 0.3Mp, 1/11-Inch System-On-A-Chip (SOC) CMOS Digital Image Sensor
     * For MT9P111: 5.0Mp, 1/4-Inch System-On-A-Chip (SOC) CMOS Digital Image Sensor
     *
     * switch_val: 0, to switch clock to frontend sensor, i.e., MT9V113, or
     *             1, to switch clock to backend sensor, i.e., MT9P111
     */
    rc = msm_camera_clk_switch(hi704_ctrl->sensordata, HI704_GPIO_SWITCH_CTL, 1);
    if (rc < 0)
    {
        CCRT("%s: camera_clk_switch failed!\n", __func__);
        goto init_fail;
    }

    /* Input MCLK */
    msm_camio_clk_rate_set(HI704_CAMIO_MCLK);
    
    msm_camio_camif_pad_reg_reset();
    
    mdelay(5);

    

    //mdelay(10);

    hi704_power_down(hi704_ctrl->sensordata,0);

    mdelay(10);

    rc = hi704_sensor_init_probe(hi704_ctrl->sensordata);
    if (rc < 0)
    {
        CCRT("%s: sensor_init_probe failed!\n", __func__);
        goto init_fail;
    }

init_done:
    return rc;

init_fail:
    /*
      * To power sensor down
      * Ignore "rc"
      */
    msm_camera_power_frontend(MSM_CAMERA_PWRDWN_MODE);
    kfree(hi704_ctrl);
    return rc;
}

static int hi704_init_client(struct i2c_client *client)
{
    /* Initialize the MSM_CAMI2C Chip */
    init_waitqueue_head(&hi704_wait_queue);
    return 0;
}

int hi704_sensor_config(void __user *argp)
{
    struct sensor_cfg_data cfg_data;
    long rc = 0;

    CDBG("%s: entry\n", __func__);

    if (copy_from_user(&cfg_data, (void *)argp, sizeof(struct sensor_cfg_data)))
    {
        CCRT("%s: copy_from_user failed!\n", __func__);
        return -EFAULT;
    }

    /* down(&mt9v113_sem); */

    CDBG("%s: cfgtype = %d, mode = %d\n", __func__, cfg_data.cfgtype, cfg_data.mode);

    switch (cfg_data.cfgtype)
    {
        case CFG_SET_MODE:
        {
            rc = hi704_set_sensor_mode(cfg_data.mode);
        }
        break;

        case CFG_SET_EFFECT:
        {
            rc = hi704_set_effect(cfg_data.mode, cfg_data.cfg.effect);
        }
        break;
        
        case CFG_PWR_UP:
        {
            rc = hi704_power_up();
        }
        break;
        
        case CFG_PWR_DOWN:
        {
            //rc = hi704_power_down();
        }
        break;
        case CFG_SET_WB:
        {
            rc = hi704_set_wb(cfg_data.cfg.wb_mode);
        }
        break;

        case CFG_SET_ISO:
        {
            rc = hi704_set_iso(cfg_data.cfg.iso_val);
        }
        break;

        case CFG_SET_ANTIBANDING:
        {
            rc = hi704_set_antibanding(cfg_data.cfg.antibanding);
        }
        break;

        case CFG_SET_BRIGHTNESS:
        {
            rc = hi704_set_brightness(cfg_data.cfg.brightness);
        }
        break;

        case CFG_SET_SATURATION:
        {
            rc = hi704_set_saturation(cfg_data.cfg.saturation);
        }
        break;

        case CFG_SET_CONTRAST:
        {
            rc = hi704_set_contrast(cfg_data.cfg.contrast);
        }
        break;

        case CFG_SET_SHARPNESS:
        {
            rc = hi704_set_sharpness(cfg_data.cfg.sharpness);
        }
        break;

        case CFG_SET_EXPOSURE_COMPENSATION:
        {
            rc = hi704_set_exposure_compensation(cfg_data.cfg.exposure);
        }
        break;
        
        default:
        {
            rc = -EFAULT;
        }
        break;
    }

    /* up(&mt9v113_sem); */

    return rc;
}

int hi704_sensor_release(void)
{
    int rc = 0;
    
    pr_err("%s: entry\n", __func__);

    /* down(&mt9v113_sem); */

    /*
      * Power VREG Off
      * Ignore "rc"
      */
#if 1
    hi704_power_down(hi704_ctrl->sensordata,1);

    rc = msm_camera_power_frontend(MSM_CAMERA_PWRDWN_MODE);

    kfree(hi704_ctrl);
#endif
   
    /* up(&mt9v113_sem); */

    return rc;
}

static int hi704_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int rc = 0;

    CDBG("%s: entry\n", __func__);

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
    {
        rc = -ENOTSUPP;
        goto probe_failure;
    }

    hi704_sensorw = kzalloc(sizeof(struct hi704_work_t), GFP_KERNEL);
    if (!hi704_sensorw)
    {
        rc = -ENOMEM;
        goto probe_failure;
    }

    i2c_set_clientdata(client, hi704_sensorw);
    hi704_init_client(client);
    hi704_client = client;

    return 0;

probe_failure:
    kfree(hi704_sensorw);
    hi704_sensorw = NULL;
    CCRT("%s: rc = %d, failed!\n", __func__, rc);
    return rc;
}

static int __exit hi704_i2c_remove(struct i2c_client *client)
{
    struct hi704_work_t *sensorw = i2c_get_clientdata(client);

    CDBG("%s: entry\n", __func__);

    free_irq(client->irq, sensorw);   
    kfree(sensorw);

    hi704_client = NULL;
    hi704_sensorw = NULL;

    return 0;
}

static const struct i2c_device_id hi704_id[] = {
    { "hi704", 0},
    { },
};

static struct i2c_driver hi704_driver = {
    .id_table = hi704_id,
    .probe  = hi704_i2c_probe,
    .remove = __exit_p(hi704_i2c_remove),
    .driver = {
        .name = "hi704",
    },
};

static int32_t hi704_i2c_add_driver(void)
{
    int32_t rc = 0;
    
    CDBG("%s: entry\n", __func__);
    
    rc = i2c_add_driver(&hi704_driver);
    if (IS_ERR_VALUE(rc))
    {
        goto init_failure;
    }

    return rc;

init_failure:
    CCRT("%s: rc = %d, failed!\n", __func__, rc);
    return rc;
}

static void hi704_i2c_del_driver(void)
{
    i2c_del_driver(&hi704_driver);
}

void hi704_exit(void)
{
    CDBG("%s: entry\n", __func__);
    
    i2c_del_driver(&hi704_driver);
}

int hi704_sensor_probe(const struct msm_camera_sensor_info *info,
				                 struct msm_sensor_ctrl *s)
{
    int rc = 0;

    CDBG("%s: entry\n", __func__);
#if !defined(CONFIG_SENSOR_ADAPTER)
    rc = hi704_i2c_add_driver();
    if (rc < 0)
    {
        goto probe_failed;
    }
#else
    // Do nothing
#endif

    rc = hi704_sensor_probe_init(info);
    if (rc < 0)
    {
        CCRT("%s: hi704_sensor_probe_init failed!\n", __func__);
        goto probe_failed;
    }

    rc = hi704_sensor_release();
    if (rc < 0)
    {
        CCRT("%s: hi704_sensor_release failed!\n", __func__);
        goto probe_failed;
    }

  
    s->s_mount_angle = 0;
    s->s_camera_type = FRONT_CAMERA_2D;

    s->s_init       = hi704_sensor_init;
    s->s_release    = hi704_sensor_release;
    s->s_config     = hi704_sensor_config;

    return rc;

probe_failed:
    CCRT("%s: rc = %d, failed!\n", __func__, rc);

#if !defined(CONFIG_SENSOR_ADAPTER)
    hi704_i2c_del_driver();
#else
    // Do nothing
#endif

    return rc;
}

static int __hi704_probe(struct platform_device *pdev)
{
	return msm_camera_drv_start(pdev, hi704_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
	.probe = __hi704_probe,
	.driver = {
		.name = "msm_camera_hi704",
		.owner = THIS_MODULE,
	},
};

static int __init hi704_init(void)
{
	return platform_driver_register(&msm_camera_driver);
}

module_init(hi704_init);

