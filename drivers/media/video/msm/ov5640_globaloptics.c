/*
 * drivers/media/video/msm/ov5640_globaloptics.c
 *
 * Refer to drivers/media/video/msm/mt9d112.c
 * For IC OV5640 of Module GLOBALOPTICS: 5.0Mp, 1/4-Inch System-On-A-Chip (SOC) CMOS Digital Image Sensor
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
  when              who                   what, where, why                    comment tag
  --------      -- ----     -------------------------------------    --------------------------

------------------------------------------------------------------------------------------*/

#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <media/msm_camera.h>
#include <mach/gpio.h>
#include "ov5640.h"
#include <linux/slab.h>

/*-----------------------------------------------------------------------------------------
 *
 * MACRO DEFINITION
 *
 *----------------------------------------------------------------------------------------*/
/*
 * To implement the parallel init process
 */
#define OV5640_PROBE_WORKQUEUE

#if defined(OV5640_PROBE_WORKQUEUE)
#include <linux/workqueue.h>
static struct platform_device *pdev_wq = NULL;
static struct workqueue_struct *ov5640_wq = NULL;
static void ov5640_workqueue(struct work_struct *work);
static DECLARE_WORK(ov5640_cb_work, ov5640_workqueue);
#endif /* defined(OV5640_PROBE_WORKQUEUE) */

#define ENOINIT 100 /*have not power up,so don't need to power down*/

/*
 * OV5640 Registers and their values
 */
/* Sensor I2C Board Name */
#define OV5640_I2C_BOARD_NAME "ov5640"

/* Sensor I2C Bus Number (Master I2C Controller: 0) */
#define OV5640_I2C_BUS_ID  (0)

/* Sensor I2C Slave Address */
#define OV5640_SLAVE_WR_ADDR 0x78 /* replaced by "msm_i2c_devices.addr" */
#define OV5640_SLAVE_RD_ADDR 0x79 /* replaced by "msm_i2c_devices.addr" */

/* Sensor I2C Device ID */
#define REG_OV5640_MODEL_ID    0x300A
#define OV5640_MODEL_ID        0x5640

/* SOC Registers */
#define REG_OV5640_SENSOR_RESET     0x001A

/* CAMIO Input MCLK (MHz) */
#define OV5640_CAMIO_MCLK  24000000 // from 12Mhz to 24Mhz

/* GPIO For Lowest-Power mode (SHUTDOWN mode) */
#define OV5640_GPIO_SHUTDOWN_CTL   32

/* GPIO For Sensor Clock Switch */

#define OV5640_GPIO_SWITCH_CTL     39

/* 
 * modify for mclk switch for msm7627_joe
 */
#if defined(CONFIG_MACH_RAISE)
#define OV5640_GPIO_SWITCH_VAL     1
#elif defined(CONFIG_MACH_R750) || defined(CONFIG_MACH_JOE)
#define OV5640_GPIO_SWITCH_VAL     0
#else
#define OV5640_GPIO_SWITCH_VAL     1
#endif

//#define CAPTURE_FRAMERATE 375
//#define PREVIEW_FRAMERATE 1500

#define CAPTURE_FRAMERATE 750
#define PREVIEW_FRAMERATE 1500


#define OV5640_AF_WINDOW_FULL_WIDTH  64
#define OV5640_AF_WINDOW_FULL_HEIGHT 48
/* 
 * Auto Exposure Gain Factor (disused)
 * #define AE_GAIN_FACTOR  50    // 2.2: underexposure, 200: overexposure, 50: normal exposure
 */

/*-----------------------------------------------------------------------------------------
 *
 * TYPE DECLARATION
 *
 *----------------------------------------------------------------------------------------*/
struct ov5640_work_t {
    struct work_struct work;
};

static struct ov5640_work_t *ov5640_sensorw;
static struct i2c_client *ov5640_client;

struct ov5640_ctrl_t {
    const struct msm_camera_sensor_info *sensordata;
};

/*-----------------------------------------------------------------------------------------
 *
 * GLOBAL VARIABLE DEFINITION
 *
 *----------------------------------------------------------------------------------------*/
static struct ov5640_ctrl_t *ov5640_ctrl;

static uint32_t g_preview_exposure;
static uint32_t g_preview_line_width;
static uint16_t g_preview_gain,g_preview_gain_low,g_preview_gain_high;
static uint32_t g_preview_frame_rate = 0;

#if 0 
static uint16_t temp_l,temp_m,temp_h;
#endif

DECLARE_MUTEX(ov5640_sem);

/*-----------------------------------------------------------------------------------------
 *
 * FUNCTION DECLARATION
 *
 *----------------------------------------------------------------------------------------*/
static int ov5640_sensor_init(const struct msm_camera_sensor_info *data);
static int ov5640_sensor_config(void __user *argp);
static int ov5640_sensor_release(void);

static int32_t ov5640_i2c_add_driver(void);
static void ov5640_i2c_del_driver(void);

extern int32_t msm_camera_power_backend(enum msm_camera_pwr_mode_t pwr_mode);
extern int msm_camera_clk_switch(const struct msm_camera_sensor_info *data,
                                         uint32_t gpio_switch,
                                         uint32_t switch_val);

/*
 * Get FTM flag to adjust 
 * the initialize process 
 * of camera
 */
#ifdef CONFIG_ZTE_PLATFORM
#ifdef CONFIG_ZTE_FTM_FLAG_SUPPORT
extern int zte_get_ftm_flag(void);
#endif
#endif

/*-----------------------------------------------------------------------------------------
 *
 * FUNCTION DEFINITION
 *
 *----------------------------------------------------------------------------------------*/
/*
 * Hard standby of sensor
 * on: =1, enter into hard standby
 * on: =0, exit from hard standby
 *
 * Hard standby mode is set by register of REG_OV5640_STANDBY_CONTROL.
 */
static int ov5640_hard_standby(const struct msm_camera_sensor_info *dev, uint32_t on)
{
    int rc;

    CDBG("%s: entry\n", __func__);

    rc = gpio_request(dev->sensor_pwd, "ov5640");
    if (0 == rc)
    {
        /* ignore "rc" */
        rc = gpio_direction_output(dev->sensor_pwd, on);

        /* time delay for the entry into standby */
        mdelay(10);
    }

    gpio_free(dev->sensor_pwd);

    return rc;
}

/*
 * Hard reset: RESET_BAR pin (active LOW)
 * Hard reset has the same effect as the soft reset.
 */
static int __attribute__((unused))ov5640_hard_reset(const struct msm_camera_sensor_info *dev)
{
    int rc = 0;

    CDBG("%s: entry\n", __func__);

    rc = gpio_request(dev->sensor_reset, "ov5640");
    if (0 == rc)
    {
        /* ignore "rc" */
        rc = gpio_direction_output(dev->sensor_reset, 1);

        /* time delay for asserting RESET */
        mdelay(10);

        /* ignore "rc" */
        rc = gpio_direction_output(dev->sensor_reset, 0);

        /*
          * RESET_BAR pulse width: Min 70 EXTCLKs
          * EXTCLKs: = MCLK (i.e., OV5640_CAMIO_MCLK)
          */
        mdelay(10);

        /* ignore "rc" */
        rc = gpio_direction_output(dev->sensor_reset, 1);

        /*
          * Time delay before first serial write: Min 100 EXTCLKs
          * EXTCLKs: = MCLK (i.e., OV5640_CAMIO_MCLK)
          */
        mdelay(10);
    }

    gpio_free(dev->sensor_reset);

    return rc;
}

static int32_t ov5640_i2c_txdata(unsigned short saddr,
                                       unsigned char *txdata,
                                       int length)
{
    struct i2c_msg msg[] = {
        {
            .addr  = saddr,
            .flags = 0,
            .len   = length,
            .buf   = txdata,
        },
    };

    if (i2c_transfer(ov5640_client->adapter, msg, 1) < 0)
    {
        CCRT("%s: failed!\n", __func__);
        return -EIO;
    }

    return 0;
}

static int32_t ov5640_i2c_write(unsigned short saddr,
                                    unsigned short waddr,
                                    unsigned short wdata,
                                    enum ov5640_width_t width)
{
    int32_t rc = -EFAULT;
    unsigned char buf[3];

    memset(buf, 0, sizeof(buf));

    switch (width)
    {
        case WORD_LEN:
        {
            buf[0] = (waddr & 0xFF00) >> 8;
            buf[1] = (waddr & 0x00FF);
#if 0 // 16-bit data width 
            buf[2] = (wdata & 0xFF00) >> 8;
            buf[3] = (wdata & 0x00FF);
            rc = ov5640_i2c_txdata(saddr, buf, 4);
#else // 8-bit data width for OV sensor  
            buf[2] = (wdata & 0x00FF);
            rc = ov5640_i2c_txdata(saddr, buf, 3);
#endif
        }
        break;

        case BYTE_LEN:
        {
            buf[0] = (waddr & 0xFF00) >> 8;
            buf[1] = (waddr & 0x00FF);
            buf[2] = wdata;

            rc = ov5640_i2c_txdata(saddr, buf, 3);
        }
        break;

        default:
        {
        }
        break;
    }

    if (rc < 0)
    {
        CCRT("%s: waddr = 0x%x, wdata = 0x%x, failed!\n", __func__, waddr, wdata);
    }

    return rc;
}

static int32_t ov5640_i2c_write_table(struct ov5640_i2c_reg_conf const *reg_conf_tbl,
                                             int len)
{
    uint32_t i;
    int32_t rc = 0;

    for (i = 0; i < len; i++)
    {
        rc = ov5640_i2c_write(ov5640_client->addr,
                               reg_conf_tbl[i].waddr,
                               reg_conf_tbl[i].wdata,
                               reg_conf_tbl[i].width);
        if (rc < 0)
        {
        
            break;
        }

        if (reg_conf_tbl[i].mdelay_time != 0)
        {
            /* time delay for writing I2C data */
            mdelay(reg_conf_tbl[i].mdelay_time);
        }
    }

    return rc;
}

static int ov5640_i2c_rxdata(unsigned short saddr,
                                   unsigned char *rxdata,
                                   int length)
{
    struct i2c_msg msgs[] = {
        {
            .addr  = saddr,
            .flags = 0,
            .len   = 2,
            .buf   = rxdata,
        },
        {
            .addr  = saddr,
            .flags = I2C_M_RD,
            .len   = length,
            .buf   = rxdata,
        },
    };

    if (i2c_transfer(ov5640_client->adapter, msgs, 2) < 0)
    {
        CCRT("%s: failed!\n", __func__);
        return -EIO;
    }

    return 0;
}

static int32_t ov5640_i2c_read(unsigned short saddr,
                                     unsigned short raddr,
                                     unsigned short *rdata,
                                     enum ov5640_width_t width)
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

            rc = ov5640_i2c_rxdata(saddr, buf, 2);
            if (rc < 0)
            {
                return rc;
            }

            *rdata = buf[0] << 8 | buf[1];
        }
        break;

        case BYTE_LEN:
        {
            buf[0] = (raddr & 0xFF00) >> 8;
            buf[1] = (raddr & 0x00FF);

            rc = ov5640_i2c_rxdata(saddr, buf, 2);
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

static int32_t ov5640_set_lens_roll_off(void)
{
   // return 0;
    int32_t rc = 0;
	return rc;
    CDBG("%s: entry\n", __func__);

    rc = ov5640_i2c_write_table(ov5640_regs.rftbl, ov5640_regs.rftbl_size);

    return rc;
}

static long ov5640_set_brightness(int8_t brightness)
{
    long rc = 0;
    uint16_t tmp_reg = 0;

    pr_err("%s: entry: brightness=%d\n", __func__, brightness);

    switch(brightness)
    {
        case CAMERA_BRIGHTNESS_0:
        {
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5001, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0080;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5001, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5587, 0x0030, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            //WT_CAM_20110411 write 5580
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5580, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg |= 0x0004;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5580, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }	
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5588, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0008;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5588, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }   
        }
        break;

        case CAMERA_BRIGHTNESS_1:
        {
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5001, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0080;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5001, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5587, 0x0020, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            //WT_CAM_20110411 write 5580
           rc = ov5640_i2c_read(ov5640_client->addr, 0x5580, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg |= 0x0004;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5580, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }	
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5588, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0008;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5588, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }   
        }
        break;

        case CAMERA_BRIGHTNESS_2:
        {
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5001, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0080;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5001, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5587, 0x0010, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            //WT_CAM_20110411 write 5580
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5580, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg |= 0x0004;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5580, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }	           
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5588, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0008;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5588, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }   
        }
        break;

        case CAMERA_BRIGHTNESS_3:
        {
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5001, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0080;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5001, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5587, 0x0000, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            //WT_CAM_20110411 write 5580
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5580, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg |= 0x0004;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5580, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }	
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5588, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x00F7;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5588, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }  
            
        }
        break;

        case CAMERA_BRIGHTNESS_4:
        {
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5001, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0080;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5001, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5587, 0x0010, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            //WT_CAM_20110411 write 5580
           rc = ov5640_i2c_read(ov5640_client->addr, 0x5580, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg |= 0x0004;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5580, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }	
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5588, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x00F7;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5588, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }   
        }
        break;

        case CAMERA_BRIGHTNESS_5:
        {
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5001, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0080;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5001, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5587, 0x0020, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            //WT_CAM_20110411 write 5580
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5580, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg |= 0x0004;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5580, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }	
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5588, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x00F7;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5588, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
        }
        break;

        case CAMERA_BRIGHTNESS_6:
        {
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5001, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0080;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5001, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5587, 0x0030, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            //WT_CAM_20110411 write 5580
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5580, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg |= 0x0004;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5580, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }	
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5588, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x00F7;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5588, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }   
        }
        break;

        default:
        {
            CCRT("%s: parameter error!\n", __func__);
            return -EFAULT;
        }            
    }

    return rc;
}

static long ov5640_set_contrast(int8_t contrast_val)
{
    long rc = 0;
    uint16_t tmp_reg = 0;

    CINF("%s: entry: contrast_val=%d\n", __func__, contrast_val);

    switch(contrast_val)
    {
        case CAMERA_CONTRAST_0:
        {
            //WT_CAM_20110421
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5001, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }

        
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0080;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5001, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5580, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0004;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5580, tmp_reg, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }            
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5586, 0x0010, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }            
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5585, 0x0010, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }            
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5588, 0x0001, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }            
        }
        break;

        case CAMERA_CONTRAST_1:
        {
            //WT_CAM_20110421
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5001, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }

            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0080;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5001, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5580, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0004;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5580, tmp_reg, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }            
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5586, 0x0018, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }            
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5585, 0x0018, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }            
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5588, 0x0001, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }            
        }
        break;

        case CAMERA_CONTRAST_2:
        {
            //WT_CAM_20110421
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5001, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }

        
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0080;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5001, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5580, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0004;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5580, tmp_reg, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }            
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5586, 0x0020, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }            
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5585, 0x0000, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }            
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5588, 0x0001, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }            
        }
        break;

        case CAMERA_CONTRAST_3:
        {  
            //WT_CAM_20110421
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5001, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }

        
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0080;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5001, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5580, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0004;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5580, tmp_reg, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }            
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5586, 0x0024, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }            
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5585, 0x0010, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }            
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5588, 0x0001, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }            
        }
        break;

        case CAMERA_CONTRAST_4:
        {   
            //WT_CAM_20110421
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5001, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }

        
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0080;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5001, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5580, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0004;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5580, tmp_reg, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }            
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5586, 0x002c, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }            
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5585, 0x001c, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }            
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5588, 0x0001, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }            
        }        
        break;

        default:
        {
            CCRT("%s: parameter error!\n", __func__);
            return -EFAULT;
        }            
    }
    return rc;
}

static long ov5640_set_saturation(int8_t saturation_val)
{
    long rc = 0;
    uint16_t tmp_reg = 0;
 


    pr_err("%s: entry: saturation_val=%d\n", __func__, saturation_val);

    switch(saturation_val)
    {
        case CAMERA_SATURATION_0:
        {
        	
            //WT_CAM_20110421
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5001, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }

       
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0080;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5001, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5583, 0x0020, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            } 
                 
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5584, 0x0010, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5580, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0002;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5580, tmp_reg, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }    
            tmp_reg = 0;
            
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5588, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0040;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5588, 0x00, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }              
        }
        break;

        case CAMERA_SATURATION_1:
        {
            //WT_CAM_20110421
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5001, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }

            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0080;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5001, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5583, 0x0038, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            } 
                 
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5584, 0x0020, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }            
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5580, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0002;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5580, tmp_reg, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }  
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5588, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0040;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5588, 0x00, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }      
        }
        break;
        
        case CAMERA_SATURATION_2:
        {
            //WT_CAM_20110421
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5001, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }

        
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0080;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5001, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5583, 0x0040, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            } 
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5584, 0x0028, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5580, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0002;
         
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5580, tmp_reg, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5588, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x00FF;
            tmp_reg &= 0x00bf;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5588, 0x00, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
        }
        break;
               
        case CAMERA_SATURATION_3:
        {
            //WT_CAM_20110421
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5001, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }

      
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0080;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5001, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5583, 0x0050, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            } 
                 
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5584, 0x0038, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5580, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0002;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5580, tmp_reg, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }  
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5588, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0040;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5588, 0x00, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }      
        }
        break;
        
        case CAMERA_SATURATION_4:
        {
            //WT_CAM_20110421
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5001, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }

            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0080;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5001, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5583, 0x0060, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
                    
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5584, 0x0048, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5580, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0002;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5580, tmp_reg, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }  
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5588, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0040;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5588, 0x00, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }      
        }        
        break;
        
        default:
        {
            pr_err("%s: parameter error!\n", __func__);
            return -EFAULT;
        }            
    }

    return rc;
}

/*
 * Auto Focus Trigger
 * WT_CAM_20110127 set af register to enable af function 
 */
static int32_t ov5640_af_trigger(void)
{
    int32_t rc;
    int32_t rc_3024;
    int32_t rc_3025;
    int32_t rc_3026;
    int32_t rc_3027;
    int32_t rc_3028;
    //uint16_t af_status;
    uint16_t af_ack=0x0002;
    uint32_t i;
    uint16_t af_status_3024,af_status_3025, af_status_3026, af_status_3027,af_status_3028;
  

    CDBG("%s: entry\n", __func__);

   //Use Trig Auto Focus command to start auto focus	

    rc = ov5640_i2c_write(ov5640_client->addr, 0x3023, 0x0001, WORD_LEN);
    if (rc < 0)
    {
        CCRT("%s: failed, rc=%d!\n", __func__, rc);
        return rc;
    }

    rc = ov5640_i2c_write(ov5640_client->addr, 0x3022, 0x0003, WORD_LEN);
    if (rc < 0)
    {
        CCRT("%s: failed, rc=%d!\n",__func__, rc);        
        return rc;
    }
    /*
      * af_status: = 0x00,  AF is done successfully
      *            != 0x00, AF is being done
      *
      * i: < 100, time delay for tuning AF
      *    >= 100, time out for tuning AF
      */
    for (i = 0; (i < 200) && (0x0000 != af_ack); ++i)
    {
        af_ack = 0x0002;
        rc = ov5640_i2c_read(ov5640_client->addr, 0x3023, &af_ack, BYTE_LEN);
		
        if (rc < 0)
        {
          CCRT("%s: rc < 0\n", __func__);
           return rc;
        }        
	
        mdelay(15);
	//pr_err("Trig Auto Focus  i  = %d ",i);
    }
    

     //Get Focus Result
    rc = ov5640_i2c_write(ov5640_client->addr, 0x3023, 0x0001, WORD_LEN);
    if (rc < 0)
    {
        CCRT("%s: failed, rc=%d!\n", __func__, rc);
        return rc;
    }

    rc = ov5640_i2c_write(ov5640_client->addr, 0x3022, 0x0007, WORD_LEN);
    if (rc < 0)
    {
        CCRT("%s: failed, rc=%d!\n",__func__, rc);        
        return rc;
    } 
	
    af_ack = 0x0002;//set af_ack value for compare its value from get 
    for (i = 0; (i < 200) && (0x0000 != af_ack); ++i)
    {
        af_ack = 0x0002;
        rc = ov5640_i2c_read(ov5640_client->addr, 0x3023, &af_ack, BYTE_LEN);
		
        if (rc < 0)
        {
         
           return rc;
        } 
	 mdelay(15);
	//pr_err("Get Focus Result  i  = %d ",i);
        
    }
	//af is success if any rc_302x=0
	rc_3024 = ov5640_i2c_read(ov5640_client->addr, 0x3024, &af_status_3024, BYTE_LEN);
	rc_3025 = ov5640_i2c_read(ov5640_client->addr, 0x3025, &af_status_3025, BYTE_LEN);
	rc_3026 = ov5640_i2c_read(ov5640_client->addr, 0x3026, &af_status_3026, BYTE_LEN);
	rc_3027 = ov5640_i2c_read(ov5640_client->addr, 0x3027, &af_status_3027, BYTE_LEN);
	rc_3028 = ov5640_i2c_read(ov5640_client->addr, 0x3028, &af_status_3028, BYTE_LEN);

	if(  (0 ==af_status_3024)||(0 ==af_status_3025)||
	      (0 ==af_status_3026)||(0 ==af_status_3027)||(0 ==af_status_3028) )
	{
		pr_err("rc_3024= %d ; rc_3025= %d; rc_3026= %d; rc_3027= %d ;rc_3028= %d,\n", af_status_3024, af_status_3025,af_status_3026,af_status_3027,af_status_3028);
	
		return 0;
	}
	else
	{

	       // pr_err("af error : %s: AF Statu_x=%x,\n", __func__, af_status);
				
	}
 
	return 0;

}
    
   

static long ov5640_set_wb(int8_t wb_mode)
{
    long rc = 0;
    uint16_t tmp_reg = 0;

    CDBG("%s: entry: wb_mode=%d\n", __func__, wb_mode);

    switch(wb_mode)
    {
        case CAMERA_WB_MODE_AWB:
        {
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3406, 0x0000, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
        }
        break;

        case CAMERA_WB_MODE_SUNLIGHT:
        {
        	  rc = ov5640_i2c_write(ov5640_client->addr, 0x3406 ,0x0001, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3400 ,0x0006, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3401 ,0x001c, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3402 ,0x0004, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3403 ,0x0000, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3404 ,0x0005, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3405 ,0x0020, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
          
        }
        break;

        case CAMERA_WB_MODE_INCANDESCENT:
        {
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3406 ,0x0001, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3400 ,0x0004, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3401 ,0x0010, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3402 ,0x0004, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3403 ,0x0000, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3404 ,0x0008, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3405 ,0x0040, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
        }
        break;
        
        case CAMERA_WB_MODE_FLUORESCENT:
        {
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3406 ,0x0001, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3400 ,0x0005, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3401 ,0x0048, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3402 ,0x0004, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3403 ,0x0000, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3404 ,0x0007, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3405 ,0x00c0, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
        }
        break; 

        case CAMERA_WB_MODE_CLOUDY:
        {
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3406 ,0x0001, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3400 ,0x0006, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3401 ,0x0048, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3402 ,0x0004, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3403 ,0x0000, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3404 ,0x0004, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3405 ,0x00d3, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
        }
        break;

        case CAMERA_WB_MODE_NIGHT:
        {
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x3a00, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0004;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a00, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a02 ,0x000b, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }

            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a03 ,0x0088, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a14 ,0x000b, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }

            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a15 ,0x0088, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            /*
               * Change preview FPS from 1500 to 375
               */
            g_preview_frame_rate  = 375;
        }
        break;

        default:
        {
            CCRT("%s: parameter error!\n", __func__);
            return -EFAULT;
        }     
    }

    if(wb_mode != CAMERA_WB_MODE_NIGHT)
    {
    	     /*
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x3a00, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x00FB;    
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a00, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            */
    }

    return rc;
}


static int32_t ov5640_set_aec_rio(aec_rio_cfg position)
{
    
    int32_t rc = 0;
    uint16_t af_status;
    uint32_t i;
    uint16_t window_full_width;
    uint16_t window_full_height;
    uint16_t x_ratio = 0;
    uint16_t y_ratio = 0;
  
	return rc;
    /*
     * window_full_width/window_full_height: 0~63
     */
    window_full_width = OV5640_AF_WINDOW_FULL_WIDTH;
    window_full_height = OV5640_AF_WINDOW_FULL_HEIGHT;
    x_ratio = position.preview_width/window_full_width;
    y_ratio = position.preview_height/window_full_height;
	
    if (x_ratio == 0 || y_ratio == 0)
    {
       return rc;
    }
	
    /* 
    * 1. set as idle mode
    */
    rc = ov5640_i2c_write(ov5640_client->addr, 0x3024, 0x08, BYTE_LEN);
    if (rc < 0)
    {
       return rc;
    }
	
    /* 
    * 2. read reg 0x3027, if =0, it means idle state
    */
    af_status = 0x0000;
    rc = ov5640_i2c_read(ov5640_client->addr, 0x3027, &af_status, BYTE_LEN);
    if (rc < 0)
    {
       return rc;
    }
    for (i = 0; (i < 100) && (0x0000 != af_status); ++i)
    {
        af_status = 0x0000;
        rc = ov5640_i2c_read(ov5640_client->addr, 0x3027, &af_status, BYTE_LEN);
        if (rc < 0)
        {
           return rc;
        }        

        mdelay(10);
    }
	
    /* 
    * 3. set windows position
    */
    rc = ov5640_i2c_write(ov5640_client->addr, 0x3025, 0x11, BYTE_LEN);
    if (rc < 0)
    {
       return rc;
    }
    rc = ov5640_i2c_write(ov5640_client->addr, 0x5084, (int32_t)(position.x / x_ratio), WORD_LEN);
    if (rc < 0)
    {
       return rc;
    }
    rc = ov5640_i2c_write(ov5640_client->addr, 0x5085, (int32_t)(position.y / y_ratio), WORD_LEN);
    if (rc < 0)
    {
       return rc;
    }
    rc = ov5640_i2c_write(ov5640_client->addr, 0x3024, 0x10, BYTE_LEN);
    if (rc < 0)
    {
       return rc;
    }
	
    /* 
    * 4. single foucs trigger
    */
    rc = ov5640_i2c_write(ov5640_client->addr, 0x3024, 0x03, BYTE_LEN);
    if (rc < 0)
    {
       return rc;
    }

    af_status = 0x0000;
    for (i = 0; (i < 1000) && (0x0002 != af_status); ++i)
    {
        af_status = 0x0000;
        rc = ov5640_i2c_read(ov5640_client->addr, 0x3027, &af_status, BYTE_LEN);
        if (rc < 0)
        {
           return rc;
        }        

        mdelay(15);
    }

    return rc;
}


/*
 * ISO Setting
 */
static int32_t ov5640_set_iso(int8_t iso_val)
{
    int32_t rc = 0;

    CDBG("%s: entry: iso_val=%d\n", __func__, iso_val);

    switch (iso_val)
    {
        case CAMERA_ISO_SET_AUTO:
        {
            //WT_CAM_20110428 iso value
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3A18 ,0x0000, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            #if 1 // 主观测试版本 2011-06-16 ken
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3A19 ,0x00f8, WORD_LEN);
            #else
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3A19 ,0x0040, WORD_LEN);
            #endif
            if (rc < 0)
            {
               return rc;
            }
        }
        break;

        case CAMERA_ISO_SET_HJR:
        {
            //add code here     
        }
        break;

        case CAMERA_ISO_SET_100:
        {
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3A18 ,0x0000, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3A19 ,0x0020, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
        }
        break;

        case CAMERA_ISO_SET_200:
        {
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3A18 ,0x0000, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3A19 ,0x0040, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
        }
        break;

        case CAMERA_ISO_SET_400:
        {
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3A18 ,0x0000, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3A19 ,0x0080, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
        }
        break;

        case CAMERA_ISO_SET_800:
        {
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3A18 ,0x0000, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3A19 ,0x00f8, WORD_LEN);
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
static int32_t  ov5640_set_antibanding(int8_t antibanding)
{
    int32_t rc = 0;
    int16_t tmp_reg = 0;

    pr_err("%s: entry: antibanding=%d\n", __func__, antibanding);

    switch (antibanding)
    {
        case CAMERA_ANTIBANDING_SET_OFF:
        {
            pr_err("%s: CAMERA_ANTIBANDING_SET_OFF NOT supported!\n", __func__);
        }
        break;

        case CAMERA_ANTIBANDING_SET_60HZ:
        {
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x3c01, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg |= 0x0080;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3C01, tmp_reg, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }

            
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3C00, 0x0000, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3A0A, 0x0000, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3A0B, 0x00f6, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3A0D, 0x0004, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }            
        break;

        case CAMERA_ANTIBANDING_SET_50HZ:
        {
        	  
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x3c01, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg |= 0x0080;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3C01, tmp_reg, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3C00, 0x0004, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
             
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3A08, 0x0001, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3A09, 0x0027, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3A0E, 0x0003, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            pr_err("50hz");
        }
        break;

        case CAMERA_ANTIBANDING_SET_AUTO:
        {
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3622, 0x0001, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3635, 0x001c, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3634, 0x0040, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3C00, 0x0000, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3C01, 0x0034, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3C04, 0x0028, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3C05, 0x0098, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3C06, 0x0000, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3C07, 0x0008, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3C08, 0x0000, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3C09, 0x001c, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x300c, 0x0022, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3C0A, 0x009c, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3C0B, 0x0040, WORD_LEN);
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
 * Sharpness Setting
 */
static int32_t ov5640_set_sharpness(int8_t sharpness)
{
    int32_t rc = 0;

    CDBG("%s: entry: sharpness=%d\n", __func__, sharpness);
       
    switch (sharpness)
    {
        case CAMERA_SHARPNESS_0:
        {
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5308 ,0x0065, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5302 ,0x0018, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5303 ,0x0000, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
        }
        break;

        case CAMERA_SHARPNESS_1:
        {
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5308 ,0x0065, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5302 ,0x0010, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5303 ,0x0000, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
        }
        break;

        case CAMERA_SHARPNESS_2:
        {
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5308 ,0x0025, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5302 ,0x0010, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5303 ,0x0008, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
        }
        break;
        
        case CAMERA_SHARPNESS_3:
        {
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5308 ,0x0065, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5302 ,0x0008, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5303 ,0x0000, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
        }
        break; 

        case CAMERA_SHARPNESS_4:
        {
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5308 ,0x0065, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5302 ,0x0002, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5303 ,0x0000, WORD_LEN);
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

static long ov5640_set_exposure_compensation(int8_t exposure)
{
    long rc = 0;

    pr_err("%s: entry: exposure=%d\n", __func__, exposure);

    switch(exposure)
    {
        case CAMERA_EXPOSURE_0:
        {
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a0f, 0x0018, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a10, 0x0010, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a11, 0x0050, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a1b, 0x0018, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a1e, 0x0010, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a1f, 0x0004, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }        
        }
        break;
        
        case CAMERA_EXPOSURE_1:
        {
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a0f, 0x0020, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a10, 0x0018, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a11, 0x0050, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a1b, 0x0020, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a1e, 0x0018, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a1f, 0x0008, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }               
        }
        break;

        case CAMERA_EXPOSURE_2:
        {
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a0f, 0x0038, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a10, 0x0030, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a11, 0x0060, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a1b, 0x0038, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a1e, 0x0030, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a1f, 0x0014, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
        }
        break;

        case CAMERA_EXPOSURE_3:
        {
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a0f, 0x0048, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a10, 0x0040, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a11, 0x0070, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a1b, 0x0048, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a1e, 0x0040, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a1f, 0x0020, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }            
        }
        break;

        case CAMERA_EXPOSURE_4:
        {
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a0f, 0x0058, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a10, 0x0050, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a11, 0x0080, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a1b, 0x0058, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a1e, 0x0050, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3a1f, 0x0020, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }       
        }
        break;
        
        default:
        {
            pr_err("%s: parameter error!\n", __func__);
            return -EFAULT;
        }        
    }

    return rc;
}

static long ov5640_reg_init(void)
{
    long rc;

    CDBG("%s: entry\n", __func__);
printk("zt debug: %s\n", __func__);//zhangtao
    /* PLL Setup */
    rc = ov5640_i2c_write_table(ov5640_regs.plltbl, ov5640_regs.plltbl_size);
    if (rc < 0)
    {
        return rc;
    }

    /* Configure sensor for Preview mode and Snapshot mode */
    rc = ov5640_i2c_write_table(ov5640_regs.prev_snap_reg_settings, ov5640_regs.prev_snap_reg_settings_size);
    if (rc < 0)
    {
        return rc;
    }

    /* Configure for Noise Reduction */
    rc = ov5640_i2c_write_table(ov5640_regs.noise_reduction_reg_settings, ov5640_regs.noise_reduction_reg_settings_size);
    if (rc < 0)
    {
        return rc;
    }

    /* Configure for Refresh Sequencer */
    rc = ov5640_i2c_write_table(ov5640_regs.stbl, ov5640_regs.stbl_size);
    if (rc < 0)
    {
        return rc;
    }

    /* Configuration for AF */
    rc = ov5640_i2c_write_table(ov5640_regs.autofocus_reg_settings, ov5640_regs.autofocus_reg_settings_size);
    if (rc < 0)
    {
        return rc;
    }

    /* Configuration for Lens Roll */
    rc = ov5640_set_lens_roll_off();
    if (rc < 0)
    {
        return rc;
    }

    return 0;
}

static long ov5640_hw_ae_parameter_record(void)
{
    //return 0;
    uint16_t ret_l, ret_m, ret_h;
    uint16_t temp_l, temp_m, temp_h;

    //uint16_t temp_awb;
    uint32_t extra_line;
    int32_t rc = 0;
    // return rc;

    rc = ov5640_i2c_write(ov5640_client->addr, 0x3503, 0x0007, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }
    #if 0 //neil WT_CAM_20110506
    //WT_CAM_20110421
    rc = ov5640_i2c_read(ov5640_client->addr, 0x3406, &temp_awb, BYTE_LEN);
    if (rc < 0)
    {
        return rc;
    }
    temp_awb = temp_awb | 0x01; //disable awb
    rc = ov5640_i2c_write(ov5640_client->addr, 0x3406, temp_awb, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }
    #endif
    rc = ov5640_i2c_read(ov5640_client->addr, 0x3500, &ret_h, BYTE_LEN);
    if (rc < 0)
    {
        return rc;
    }
   
   
    rc = ov5640_i2c_read(ov5640_client->addr, 0x3501, &ret_m, BYTE_LEN);
    if (rc < 0)
    {
        return rc;
    }
    rc = ov5640_i2c_read(ov5640_client->addr, 0x3502, &ret_l, BYTE_LEN);
    if (rc < 0)
    {
        return rc;
    }

    pr_err("ov5640_parameter_record:ret_h=%x,ret_m=%x,ret_l=%x\n",ret_h,ret_m,ret_l);

    temp_l = ret_l & 0x00FF;
    temp_m = ret_m & 0x00FF;
    temp_h = ret_h & 0x00FF;
    g_preview_exposure = (temp_h << 12) + (temp_m << 4) + (temp_l >> 4);

#if 0 //kenxu @20110707 remove for short exposure color shift, change in preview setting 0x3708 = 0x64
//kenxu add 20110513 Start ----------------------------------------------------------
    rc = ov5640_i2c_read(ov5640_client->addr, 0x3820, &temp_reg, BYTE_LEN);
    if (rc < 0)
    {
        return rc;
    }
    temp_reg = temp_reg & 0x0002; 

if(temp_reg == 0) //no flip
{
    pr_err("ov5640_parameter_record:flip=%x\n",temp_reg);
	if(g_preview_exposure < 4 )
	{
		pr_err("ov5640_parameter_record:g_preview_exposure=%x\n",g_preview_exposure);
    		rc = ov5640_i2c_write(ov5640_client->addr, 0x5183, 0x0094, BYTE_LEN);
    		if (rc < 0)
    		{
        		return rc;
    		}	
	}
}	
//kenxu add 20110513 End ----------------------------------------------------------
#endif
    pr_err("ov5640_parameter_record:ret_l=%x\n",ret_l);
    pr_err("ov5640_parameter_record:ret_m=%x\n",ret_m);
    pr_err("ov5640_parameter_record:ret_h=%x\n",ret_h);
   /****************neil add for night mode ************************/
    rc = ov5640_i2c_read(ov5640_client->addr, 0x350c, &ret_h, BYTE_LEN);
    if (rc < 0)
    {
        return rc;
    }

    rc = ov5640_i2c_read(ov5640_client->addr, 0x350d, &ret_l, BYTE_LEN);
    if (rc < 0)
    {
        return rc;
    }
    extra_line = (ret_h <<8) + ret_l;

    g_preview_exposure = g_preview_exposure + extra_line;
   /******************************************************************/
    /*
      * Set as global metering mode
      */
    rc = ov5640_i2c_read(ov5640_client->addr, 0x380e, &ret_h, BYTE_LEN);
    if (rc < 0)
    {
        return rc;
    }
    rc = ov5640_i2c_read(ov5640_client->addr, 0x380f, &ret_l, BYTE_LEN);
    if (rc < 0)
    {
        return rc;
    }
     pr_err("ov5640_parameter_record:0x380e=%x  0x380f=%x\n",ret_h,ret_l);
    //CDBG("ov5640_parameter_record:0x350c=%x, 0x350d=%x\n",ret_h,ret_l);

    g_preview_line_width = ret_h & 0xff;
    g_preview_line_width = g_preview_line_width * 256 + ret_l;
   
    g_preview_line_width = g_preview_line_width + extra_line; // neil add for night mode
   

    //Read back AGC gain for preview
	
    rc = ov5640_i2c_read(ov5640_client->addr, 0x350b, &g_preview_gain_low, BYTE_LEN);
    if (rc < 0)
    {
        return rc;
    }
    rc = ov5640_i2c_read(ov5640_client->addr, 0x350a, &g_preview_gain_high, BYTE_LEN);
    if (rc < 0)
    {
        return rc;
    }    
	g_preview_gain = g_preview_gain_low & 0xff;
	g_preview_gain = g_preview_gain_high * 256 + g_preview_gain_low;

    return 0;
} 

static long ov5640_hw_ae_transfer(void)
{
	  long rc = 0;
    uint8_t exposure_low;
    uint8_t exposure_mid;
    uint8_t exposure_high;
    uint16_t night;
    uint32_t capture_exposure;
   // uint32_t capture_exposure_gain;
    uint32_t capture_gain;
    uint16_t lines_10ms;
    uint32_t m_60Hz = 0;
    uint16_t reg_l,reg_h;
    uint16_t preview_lines_max = g_preview_line_width;
    uint16_t gain = g_preview_gain;
    uint32_t capture_lines_max;
   
   
    uint16_t YAVG; //kenxu add for improve noise.


    pr_err("1ov5640_hw_ae_transfer:gain=%x\n",gain);

    YAVG = 0;
    rc = ov5640_i2c_read(ov5640_client->addr, 0x56a1, &YAVG, BYTE_LEN);
    if (rc < 0)
    {
       return rc;
    }

    if (0 == g_preview_frame_rate)
    {
        g_preview_frame_rate = PREVIEW_FRAMERATE;
    }
    /****************************disable night mode**********************/
    rc = ov5640_i2c_read(ov5640_client->addr, 0x3a00, &night, BYTE_LEN);
    if (rc < 0)
    {
        return rc;
    }
    
    night = night & 0xfb;
    
		rc = ov5640_i2c_write(ov5640_client->addr, 0x3a00, night, WORD_LEN);
		if (rc < 0)
		{
		    return rc;
		} 
		rc = ov5640_i2c_write(ov5640_client->addr, 0x350c, 0x00, WORD_LEN);
		if (rc < 0)
		{
		    return rc;
		} 
				rc = ov5640_i2c_write(ov5640_client->addr, 0x350d, 0x00, WORD_LEN);
		if (rc < 0)
		{
		    return rc;
		} 		
   /***********************************************************/    
    
    rc = ov5640_i2c_write_table(ov5640_regs.preview2snapshot_tbl, ov5640_regs.preview2snapshot_size);
    if (rc < 0)
    {
       return rc;
    }        

    /*
      * Set as global metering mode
      */

    rc = ov5640_i2c_read(ov5640_client->addr, 0x380e, &reg_h, BYTE_LEN);
    if (rc < 0)
    {
        return rc;
    }   
    rc = ov5640_i2c_read(ov5640_client->addr, 0x380f, &reg_l, BYTE_LEN);
    if (rc < 0)
    {
        return rc;
    }
    capture_lines_max = reg_h & 0xff;
    capture_lines_max = (capture_lines_max << 8) + reg_l;

    pr_err("ov5640_hw_ae_transfer:0x380e=%x  0x380f=%x\n",reg_h,reg_l);
    //CDBG("ov5640_hw_ae_transfer:capture_lines_max=%x\n",capture_lines_max);
    if(m_60Hz== 1)
    {
        lines_10ms = 2 * CAPTURE_FRAMERATE * capture_lines_max / 12000;
    }
    else
    {
        lines_10ms = 2 * CAPTURE_FRAMERATE * capture_lines_max / 10000;
    }

        pr_err("ov5640_hw_ae_transfer:lines_10ms=%x\n",lines_10ms);

    if(preview_lines_max == 0)
    {
        preview_lines_max = 1;
        //CDBG("can not divide zero\n");
    }


    {  
    	#if 0
        capture_exposure = (g_preview_exposure * (CAPTURE_FRAMERATE) * (capture_lines_max)) 
                          / (((preview_lines_max) * (g_preview_frame_rate )));
        
       // capture_exposure = g_preview_exposure * 42 / 28 ;
       // capture_exposure = capture_exposure * 1896 / 2844;
        
       // capture_exposure = capture_exposure ; //{0x3035, 0x0041}->/2; {0x3035, 0x0081}->/4 {0x3035, 0x0021} ->/1 neil add
        capture_gain = gain; //neil modify 20110315

        capture_exposure_gain =  capture_exposure * capture_gain; //24
        if(capture_exposure_gain < (capture_lines_max * 16))
        {
            capture_exposure = capture_exposure_gain / 16;

            if (capture_exposure > lines_10ms)
            {
                capture_exposure /= lines_10ms;
                capture_exposure *= lines_10ms;
            }
        }
        else
        {
            capture_exposure = capture_lines_max;  
           // capture_exposure = 1770;     
         
        }

        if(capture_exposure == 0)
        {
            capture_exposure = 1;
        }

        capture_gain = (capture_exposure_gain * 2 / capture_exposure + 1) / 2;

        exposure_low = ((unsigned char)capture_exposure) << 4;
        exposure_mid = (unsigned char)(capture_exposure >> 4) & 0xff;
        exposure_high = (unsigned char)(capture_exposure >> 12);
        pr_err("ov5640_hw_ae_transfer:capture_exposure =%x \n",capture_exposure);
        pr_err("ov5640_hw_ae_transfer:capture_gain =%x \n",capture_gain);
        
        //ulCapture_Exposure_end=capture_exposure;
        //iCapture_Gain_end=capture_gain;
       #else
       
       //WT_CAM_20110426 resolve bright is differ from preview and snapshot
        capture_exposure = g_preview_exposure * 42 / 28 ;
        //capture_exposure = g_preview_exposure;
        capture_exposure = capture_exposure * 1896 / 2844;
#if 1 // 主观测试版本 2011-06-16 ken
        //kenxu add for reduce noise under dark condition
        if(gain > 32) //gain > 2x, gain / 2, exposure * 2;
        {
          gain = gain / 2;
          capture_exposure = capture_exposure * 2;

          if(gain > 122) //reach to max gain 16x, change blc to pass SNR test under low light
          {           
          	if(YAVG < 9)
          	{
	            rc = ov5640_i2c_write(ov5640_client->addr, 0x4009, 0x40, WORD_LEN);
	            if (rc < 0)
	            {
	              return rc;
	            }
          	}
          	else if(YAVG == 9)
          	{
	            rc = ov5640_i2c_write(ov5640_client->addr, 0x4009, 0x3c, WORD_LEN);
	            if (rc < 0)
	            {
	              return rc;
	            }
          	}			
          	else if(YAVG == 10)
          	{
	            rc = ov5640_i2c_write(ov5640_client->addr, 0x4009, 0x38, WORD_LEN);
	            if (rc < 0)
	            {
	              return rc;
	            }
          	}	
          	else if(YAVG == 11)
          	{
	            rc = ov5640_i2c_write(ov5640_client->addr, 0x4009, 0x34, WORD_LEN);
	            if (rc < 0)
	            {
	              return rc;
	            }
          	}			
          	else if(YAVG == 12)
          	{
	            rc = ov5640_i2c_write(ov5640_client->addr, 0x4009, 0x30, WORD_LEN);
	            if (rc < 0)
	            {
	              return rc;
	            }
          	}	
          	else if(YAVG == 13)
          	{
	            rc = ov5640_i2c_write(ov5640_client->addr, 0x4009, 0x2c, WORD_LEN);
	            if (rc < 0)
	            {
	              return rc;
	            }
          	}			
          	else if(YAVG == 14)
          	{
	            rc = ov5640_i2c_write(ov5640_client->addr, 0x4009, 0x28, WORD_LEN);
	            if (rc < 0)
	            {
	              return rc;
	            }
          	}	
          	else if(YAVG == 15)
          	{
	            rc = ov5640_i2c_write(ov5640_client->addr, 0x4009, 0x24, WORD_LEN);
	            if (rc < 0)
	            {
	              return rc;
	            }
          	}			
          	else if(YAVG == 16)
          	{
	            rc = ov5640_i2c_write(ov5640_client->addr, 0x4009, 0x20, WORD_LEN);
	            if (rc < 0)
	            {
	              return rc;
	            }
          	}	
          	else if(YAVG == 17)
          	{
	            rc = ov5640_i2c_write(ov5640_client->addr, 0x4009, 0x1c, WORD_LEN);
	            if (rc < 0)
	            {
	              return rc;
	            }
          	}			
          	else if(YAVG == 18)
          	{
	            rc = ov5640_i2c_write(ov5640_client->addr, 0x4009, 0x18, WORD_LEN);
	            if (rc < 0)
	            {
	              return rc;
	            }
          	}	
          	else if(YAVG == 19)
          	{
	            rc = ov5640_i2c_write(ov5640_client->addr, 0x4009, 0x14, WORD_LEN);
	            if (rc < 0)
	            {
	              return rc;
	            }
            }
          }
        }
#endif
        capture_gain = gain;
        exposure_low = ((unsigned char)capture_exposure) << 4;
        exposure_mid = (unsigned char)(capture_exposure >> 4) & 0xff;
        exposure_high = (unsigned char)(capture_exposure >> 12);
       #endif
 
        rc = ov5640_i2c_write(ov5640_client->addr, 0x350b, capture_gain, WORD_LEN);
        if (rc < 0)
        {
            return rc;
        }    
        pr_err("capture_gain  =%x",capture_gain);
        //m_iWrite0x3502=exposure_low;
        rc = ov5640_i2c_write(ov5640_client->addr, 0x3502, exposure_low, WORD_LEN);
        if (rc < 0)
        {
            return rc;
        }    
        pr_err("exposure_low  =%x",exposure_low);

        //m_iWrite0x3501=exposure_mid;
        rc = ov5640_i2c_write(ov5640_client->addr, 0x3501, exposure_mid, WORD_LEN);
        if (rc < 0)
        {
            return rc;
        }    
        pr_err("exposure_mid  =%x",exposure_mid);

        //m_iWrite0x3500=exposure_high;
        rc = ov5640_i2c_write(ov5640_client->addr, 0x3500, exposure_high, WORD_LEN);
        if (rc < 0)
        {
            return rc;
        }
        pr_err("exposure_high  =%x ",exposure_high);
    }

    /* time delay for auto exposure */
    //mdelay(300); //inoder to deduce snapshot time   WT_CAM_20110411
    
     
    return 0;
}

static long ov5640_set_effect(int32_t mode, int32_t effect)
{
    uint16_t __attribute__((unused)) reg_addr;
    uint16_t __attribute__((unused)) reg_val;
    uint16_t tmp_reg = 0;
    long rc = 0;

    pr_err("%s: entry: mode=%d, effect=%d\n", __func__, mode, effect);

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

            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5001, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }

          
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0080;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5001, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5583, 0x0040, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5584, 0x0028, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5580, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x00FF;
            tmp_reg &= 0x0006;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5580, tmp_reg, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }   
        }
        break;

        case CAMERA_EFFECT_MONO:
        {
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5001, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0080;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5001, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5583, 0x0080, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5584, 0x0080, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5580, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x0087;
            tmp_reg |= 0x0018;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5580, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }	
        }
        break;
        
        case CAMERA_EFFECT_NEGATIVE:
        {
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5001, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0080;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5001, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5580, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x0087;
            tmp_reg |= 0x0040;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5580, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }	
        }
        break;          
        
        case CAMERA_EFFECT_SEPIA:
        {
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5001, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x00FF;
            tmp_reg |= 0x0080;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5001, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5583, 0x0040, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5584, 0x00a0, WORD_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg = 0;
            rc = ov5640_i2c_read(ov5640_client->addr, 0x5580, &tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }
            tmp_reg &= 0x0087;
            tmp_reg |= 0x0018;
            rc = ov5640_i2c_write(ov5640_client->addr, 0x5580, tmp_reg, BYTE_LEN);
            if (rc < 0)
            {
               return rc;
            }	
        }
        break;
        
        case CAMERA_EFFECT_BULISH:    
        case CAMERA_EFFECT_GREENISH:
        case CAMERA_EFFECT_REDDISH:
        {
            CCRT("%s: not supported!\n", __func__);
            rc = 0;
        }
        break;      
        
        default:
        {
            CCRT("%s: parameter error!\n", __func__);
            return -EFAULT;
        }
    }

    return rc;
}

static long ov5640_set_sensor_mode(int32_t mode)
{
    long rc = 0;
    uint16_t af_ack;
    //uint16_t temp_awb;
    int i;
    /*
      * In order to avoid reentry of SENSOR_PREVIEW_MODE by
      * Qualcomm Camera HAL, e.g. vfe_set_dimension/vfe_preview_config,
      * 
      * enter into SENSOR_PREVIEW_MODE only when ov5640_previewmode_entry_cnt=0
      */
    static uint32_t ov5640_previewmode_entry_cnt = 0;

    pr_err("%s: entry: mode=%d\n", __func__, mode);

    switch (mode)
    {
        case SENSOR_PREVIEW_MODE:
        {
            ov5640_i2c_write_table(ov5640_regs.snapshot2preview_tbl, ov5640_regs.snapshot2preview_size);
            
            rc = ov5640_i2c_write(ov5640_client->addr, 0x3503, 0x0000, WORD_LEN);   //enable auto exposure 
            if (rc < 0)
            {
               return rc;
            } 
            #if 0 //WT_CAM_20110506
            rc = ov5640_i2c_read(ov5640_client->addr, 0x3406, &temp_awb, BYTE_LEN);
				    if (rc < 0)
				    {
				        return rc;
				    }
				    temp_awb = temp_awb & 0xfe; //enable awb
				    rc = ov5640_i2c_write(ov5640_client->addr, 0x3406, temp_awb, WORD_LEN);
				    if (rc < 0)
				    {
				        return rc;
				    }
				    #endif
						//updata preview auto focus zone WT_CAM_20110503
				    
				    rc = ov5640_i2c_write(ov5640_client->addr, 0x3023, 0x0001, WORD_LEN);
				    if (rc < 0)
				    {
				        CCRT("%s: failed, rc=%d!\n", __func__, rc);
				        return rc;
				    }
				
				    rc = ov5640_i2c_write(ov5640_client->addr, 0x3022, 0x0012, WORD_LEN);
				    if (rc < 0)
				    {
				        CCRT("%s: failed, rc=%d!\n",__func__, rc);        
				        return rc;
				    }
				     af_ack = 0x0002;//set af_ack value for compare its value from get 
				    for ( i = 0; (i < 200) && (0x0000 != af_ack); ++i)
				    {
				        af_ack = 0x0002;
				        rc = ov5640_i2c_read(ov5640_client->addr, 0x3023, &af_ack, BYTE_LEN);
						
				        if (rc < 0)
				        {
				         
				           return rc;
				        }
				        mdelay(15);  
				     }

				     
				    
            /* Exit from AF mode */
				    rc = ov5640_i2c_write(ov5640_client->addr, 0x3023, 0x0001, WORD_LEN);
				    if (rc < 0)
				    {
				        CCRT("%s: failed, rc=%d!\n", __func__, rc);
				        return rc;
				    }
				        rc = ov5640_i2c_write(ov5640_client->addr, 0x3022, 0x0008, WORD_LEN);
				    if (rc < 0)
				    {
				        CCRT("%s: failed, rc=%d!\n", __func__, rc);
				        return rc;
				    }       

	           return 0;
            /*
               * Enter Into SENSOR_PREVIEW_MODE
               * Only When ov5640_previewmode_entry_cnt=0
               */
            if (0 != ov5640_previewmode_entry_cnt)
            {
                CDBG("%s: reentry of SENSOR_PREVIEW_MODE: entry count=%d\n", __func__,
                     ov5640_previewmode_entry_cnt);
                break;
            }

            rc = ov5640_i2c_write_table(ov5640_regs.snapshot2preview_tbl, ov5640_regs.snapshot2preview_size);
            if (rc < 0)
            {
                return rc;
            }  

            /*
               * Enter Into SENSOR_PREVIEW_MODE
               * Only When ov5640_previewmode_entry_cnt=0
               */
            ov5640_previewmode_entry_cnt++;
        }
        break;

        case SENSOR_SNAPSHOT_MODE:
        {
        //ov5640_hw_ae_transfer();
        //return 0;
            /*
               * Enter Into SENSOR_PREVIEW_MODE
               * Only When ov5640_previewmode_entry_cnt=0
               */
            ov5640_previewmode_entry_cnt = 0;
            pr_err("SENSOR_SNAPSHOT_MODE");
#if 1 //enable exposure algorithm
            rc = ov5640_hw_ae_parameter_record();
            if (rc < 0)
            {
                return rc;
            }

            rc = ov5640_hw_ae_transfer();
            if (rc < 0)
            {
                return rc;
            }            
#else //disable exposure algorithm
            rc = ov5640_i2c_write_table(ov5640_regs.preview2snapshot_tbl, ov5640_regs.preview2snapshot_size);
            if (rc < 0)
            {
                return rc;
            }        
            CDBG("5MP snapshot!!!\n");
            mdelay(3000);         
#endif            
        }
        break;

        default:
        {
            CCRT("%s: parameter error!\n", __func__);
            return -EFAULT;
        }
    }

    return 0;
}

/*
 * Power-up Process
 */
static long ov5640_power_up(void)
{
    CDBG("%s: not supported!\n", __func__);
    return 0;
}

/*
 * Power-down Process
 */
static long ov5640_power_down(void)
{
    CDBG("%s: not supported!\n", __func__);
    return 0;
}

/*
 * Set lowest-power mode (SHUTDOWN mode)
 *
 * OV5640_GPIO_SHUTDOWN_CTL: 0, to quit lowest-power mode, or
 *                            1, to enter lowest-power mode
 */
#if defined(CONFIG_MACH_RAISE)
static int __attribute__((unused))ov5640_power_shutdown(uint32_t on)
#elif defined(CONFIG_MACH_MOONCAKE)
static int __attribute__((unused)) ov5640_power_shutdown(uint32_t on)
#elif defined(CONFIG_MACH_JOE)
static int __attribute__((unused)) ov5640_power_shutdown(uint32_t on)
#else
static int __attribute__((unused)) ov5640_power_shutdown(uint32_t on)
#endif /* defined(CONFIG_MACH_RAISE) */
{
    int rc;

    CDBG("%s: entry\n", __func__);

    rc = gpio_request(OV5640_GPIO_SHUTDOWN_CTL, "ov5640");
    if (0 == rc)
    {
        /* ignore "rc" */
        rc = gpio_direction_output(OV5640_GPIO_SHUTDOWN_CTL, on);

        /* time delay for shutting sensor down */
        mdelay(1);
    }

    gpio_free(OV5640_GPIO_SHUTDOWN_CTL);

    return rc;
}

#if !defined(CONFIG_SENSOR_ADAPTER)
static int ov5640_sensor_init_probe(const struct msm_camera_sensor_info *data)
{
    uint16_t model_id = 0;    
    int rc = 0;

    CDBG("%s: entry\n", __func__);


    rc = ov5640_i2c_read(ov5640_client->addr, REG_OV5640_MODEL_ID, &model_id, WORD_LEN);
    if (rc < 0)
    {
        goto init_probe_fail;
    }

    CDBG("%s: model_id = 0x%x\n", __func__, model_id);

    /*
      * set sensor id for EM (Engineering Mode)
      */
#ifdef CONFIG_SENSOR_INFO
    msm_sensorinfo_set_sensor_id(model_id);
#else
    // do nothing here
#endif  

    /* Check if it matches it with the value in Datasheet */
    if (model_id != OV5640_MODEL_ID)
    {
        rc = -EFAULT;
        goto init_probe_fail;
    }

    rc = ov5640_reg_init();
    if (rc < 0)
    {
        goto init_probe_fail;
    }

    return rc;

init_probe_fail:
    CCRT("%s: rc = %d, failed!\n", __func__, rc);
    return rc;
}
#else
static int ov5640_sensor_i2c_probe_on(void)
{
    int rc;
    struct i2c_board_info info;
    struct i2c_adapter *adapter;
    struct i2c_client *client;

    rc = ov5640_i2c_add_driver();
    if (rc < 0)
    {
        CCRT("%s: add i2c driver failed!\n", __func__);
        return rc;
    }

    memset(&info, 0, sizeof(struct i2c_board_info));
    info.addr = OV5640_SLAVE_WR_ADDR >> 1;
    strlcpy(info.type, OV5640_I2C_BOARD_NAME, I2C_NAME_SIZE);

    adapter = i2c_get_adapter(OV5640_I2C_BUS_ID);
    if (!adapter)
    {
        CCRT("%s: get i2c adapter failed!\n", __func__);
        goto i2c_probe_failed;
    }

    client = i2c_new_device(adapter, &info);
    i2c_put_adapter(adapter);
    if (!client)
    {
        CCRT("%s: add i2c device failed!\n", __func__);
        goto i2c_probe_failed;
    }

    ov5640_client = client;

    return 0;

i2c_probe_failed:
    ov5640_i2c_del_driver();
    return -ENODEV;
}

static void ov5640_sensor_i2c_probe_off(void)
{
    i2c_unregister_device(ov5640_client);
    ov5640_i2c_del_driver();
}

static int ov5640_sensor_dev_probe(const struct msm_camera_sensor_info *pinfo)
{
    uint16_t model_id;
    uint32_t switch_on;
    int rc;

    pr_err("%s: entry\n", __func__);

    /*
      * Deassert Sensor Reset
      * Ignore "rc"
      */

    rc = gpio_request(pinfo->sensor_reset, "ov5640");
    rc = gpio_direction_output(pinfo->sensor_reset, 0);
    gpio_free(pinfo->sensor_reset);

    /* time delay for deasserting RESET */
    mdelay(1);
	
    /*
      * Enter Into Hard Standby
      */
    switch_on = 1;
    rc = ov5640_hard_standby(pinfo, switch_on);
    if (rc < 0)
    {
        goto dev_probe_fail;
    }

    /*
      * Power VREG on
      */
    rc = msm_camera_power_backend(MSM_CAMERA_PWRUP_MODE);
    if (rc < 0)
    {
        goto dev_probe_fail;
    }

    /*
     * Camera clock switch for both frontend and backend sensors, i.e., MT9V113 and OV5640
     *
     * For MT9V113: 0.3Mp, 1/11-Inch System-On-A-Chip (SOC) CMOS Digital Image Sensor
     * For OV5640: 5.0Mp, 1/4-Inch System-On-A-Chip (SOC) CMOS Digital Image Sensor
     *
     * switch_val: 0, to switch clock to frontend sensor, i.e., MT9V113, or
     *             1, to switch clock to backend sensor, i.e., OV5640
     */
#if defined(CONFIG_MACH_RAISE)
    rc = msm_camera_clk_switch(pinfo, OV5640_GPIO_SWITCH_CTL, OV5640_GPIO_SWITCH_VAL);
    if (rc < 0)
    {
        goto dev_probe_fail;
    }
#elif defined(CONFIG_MACH_MOONCAKE)
    /* Do nothing */
#elif defined(CONFIG_MACH_R750) || defined(CONFIG_MACH_JOE)
    rc = msm_camera_clk_switch(pinfo, OV5640_GPIO_SWITCH_CTL, OV5640_GPIO_SWITCH_VAL);
    if (rc < 0)
    {
        goto dev_probe_fail;
    }
#else
    /* Do nothing */
#endif /* defined(CONFIG_MACH_RAISE) */

    /* Input MCLK */
    msm_camio_clk_rate_set(OV5640_CAMIO_MCLK);

    /* time delay for enabling MCLK */
    mdelay(10);

    /*
      * Exit From Hard Standby
      */
    switch_on = 0;
    rc = ov5640_hard_standby(pinfo, switch_on);
    if (rc < 0)
    {
        goto dev_probe_fail;
    }

    /*
      * Assert Sensor Reset
      * Ignore "rc"
      */
    rc = gpio_request(pinfo->sensor_reset, "ov5640");
    rc = gpio_direction_output(pinfo->sensor_reset, 1);
    gpio_free(pinfo->sensor_reset);

    /* time delay for asserting RESET */
    mdelay(2);

    model_id = 0;
    rc = ov5640_i2c_read(ov5640_client->addr, REG_OV5640_MODEL_ID, &model_id, WORD_LEN);
    if (rc < 0)
    {
        goto dev_probe_fail;
    }

	pr_err("zt debug: %s : model_id %x\n", __func__, model_id);//zhangtao
    CDBG("%s: model_id = 0x%x\n", __func__, model_id);

    /*
      * set sensor id for EM (Engineering Mode)
      */
#ifdef CONFIG_SENSOR_INFO
    msm_sensorinfo_set_sensor_id(model_id);
#else
    // do nothing here
#endif

    /* Check if it matches it with the value in Datasheet */
    if (model_id != OV5640_MODEL_ID)
    {
        rc = -EFAULT;
        goto dev_probe_fail;
    }

    return 0;

dev_probe_fail:
    pr_err("%s: rc = %d, failed!\n", __func__, rc);
    return rc;
}
#endif

static int ov5640_sensor_probe_init(const struct msm_camera_sensor_info *data)
{
#if !defined(CONFIG_SENSOR_ADAPTER)
    uint32_t switch_on; 
#endif
    int rc = 0;

    CDBG("%s: entry\n", __func__);

printk("zt debug: %s\n", __func__);//zhangtao
    if (!data || strcmp(data->sensor_name, "ov5640"))
    {
        CCRT("%s: invalid parameters!\n", __func__);
        rc = -ENODEV;
        goto probe_init_fail;
    }

    ov5640_ctrl = kzalloc(sizeof(struct ov5640_ctrl_t), GFP_KERNEL);
    if (!ov5640_ctrl)
    {
        CCRT("%s: kzalloc failed!\n", __func__);
        rc = -ENOMEM;
        goto probe_init_fail;
    }

    ov5640_ctrl->sensordata = data;

#if !defined(CONFIG_SENSOR_ADAPTER)
    /*
      * Deassert Sensor Reset
      * Ignore "rc"
      */
    rc = gpio_request(ov5640_ctrl->sensordata->sensor_reset, "ov5640");
    rc = gpio_direction_output(ov5640_ctrl->sensordata->sensor_reset, 0);
    gpio_free(ov5640_ctrl->sensordata->sensor_reset);

    /* time delay for deasserting RESET */
    mdelay(1);

    /* Enter Into Hard Standby */
    switch_on = 1;
    rc = ov5640_hard_standby(ov5640_ctrl->sensordata, switch_on);
    if (rc < 0)
    {
        CCRT("set standby failed!\n");
        goto probe_init_fail;
    }

    /*
      * Power VREG on
      */
    rc = msm_camera_power_backend(MSM_CAMERA_PWRUP_MODE);
    if (rc < 0)
    {
        CCRT("%s: camera_power_backend failed!\n", __func__);
        goto probe_init_fail;
    }

    /*
     * Camera clock switch for both frontend and backend sensors, i.e., MT9V113 and OV5640
     *
     * For MT9V113: 0.3Mp, 1/11-Inch System-On-A-Chip (SOC) CMOS Digital Image Sensor
     * For OV5640: 5.0Mp, 1/4-Inch System-On-A-Chip (SOC) CMOS Digital Image Sensor
     *
     * switch_val: 0, to switch clock to frontend sensor, i.e., MT9V113, or
     *             1, to switch clock to backend sensor, i.e., OV5640
     */
#if defined(CONFIG_MACH_RAISE)
    rc = msm_camera_clk_switch(ov5640_ctrl->sensordata, OV5640_GPIO_SWITCH_CTL, OV5640_GPIO_SWITCH_VAL);
    if (rc < 0)
    {
        CCRT("%s: camera_clk_switch failed!\n", __func__);
        goto probe_init_fail;
    }
#elif defined(CONFIG_MACH_MOONCAKE)
    /* Do nothing */
#elif defined(CONFIG_MACH_JOE)
    /* 
    * add for mclk switch for msm7627_joe
    */
    rc = msm_camera_clk_switch(ov5640_ctrl->sensordata, OV5640_GPIO_SWITCH_CTL, OV5640_GPIO_SWITCH_VAL);
    if (rc < 0)
    {
        CCRT("%s: camera_clk_switch failed!\n", __func__);
        goto probe_init_fail;
    }
#else
    /* Do nothing */
#endif /* defined(CONFIG_MACH_RAISE) */

    /* Input MCLK */
    msm_camio_clk_rate_set(OV5640_CAMIO_MCLK);

    /* time delay for enabling MCLK */
    mdelay(10);

    /* Exit From Hard Standby */
    switch_on = 0;
    rc = ov5640_hard_standby(ov5640_ctrl->sensordata, switch_on);
    if (rc < 0)
    {
        CCRT("set standby failed!\n");
        goto probe_init_fail;
    }

    /*
      * Assert Sensor Reset
      * Ignore "rc"
      */
    rc = gpio_request(ov5640_ctrl->sensordata->sensor_reset, "ov5640");
    rc = gpio_direction_output(ov5640_ctrl->sensordata->sensor_reset, 1);
    gpio_free(ov5640_ctrl->sensordata->sensor_reset);

    /* time delay for asserting RESET */
    mdelay(2);

    /* Sensor Register Setting */
    rc = ov5640_sensor_init_probe(ov5640_ctrl->sensordata);
    if (rc < 0)
    {
        CCRT("%s: sensor_init_probe failed!\n", __func__);
        goto probe_init_fail;
    }
#else
    rc = ov5640_sensor_dev_probe(ov5640_ctrl->sensordata);
    if (rc < 0)
    {
        CCRT("%s: ov5640_sensor_dev_probe failed!\n", __func__);
        goto probe_init_fail;
    }

    /* Sensor Register Setting */
    rc = ov5640_reg_init();
    if (rc < 0)
    {
        CCRT("%s: ov5640_reg_init failed!\n", __func__);
        goto probe_init_fail;
    }
#endif

    /* Enter Into Hard Standby */
    rc = ov5640_sensor_release();
    if (rc < 0)
    {
        CCRT("%s: sensor_release failed!\n", __func__);
        goto probe_init_fail;
    }

    return 0;

probe_init_fail:
    /*
      * To power sensor down
      * Ignore "rc"
      */
    msm_camera_power_backend(MSM_CAMERA_PWRDWN_MODE);
    if(ov5640_ctrl)
    {
        kfree(ov5640_ctrl);
    }
    return rc;
}

static int ov5640_sensor_init(const struct msm_camera_sensor_info *data)
{
    uint32_t switch_on; 
    int rc;

    CDBG("%s: entry\n", __func__);

    if (!data || strcmp(data->sensor_name, "ov5640"))
    {
        CCRT("%s: invalid parameters!\n", __func__);
        rc = -ENODEV;
        goto sensor_init_fail;
    }

    msm_camio_camif_pad_reg_reset();

    /* time delay for resetting CAMIF's PAD */
    mdelay(10);

    /* Input MCLK */
    msm_camio_clk_rate_set(OV5640_CAMIO_MCLK);

    /* time delay for enabling MCLK */
    mdelay(10);

    /* Exit From Hard Standby */   
    switch_on = 0;
    rc = ov5640_hard_standby(ov5640_ctrl->sensordata, switch_on);
    if (rc < 0)
    {
        CCRT("set standby failed!\n");
        rc = -EFAULT;
        goto sensor_init_fail;
    }

  
    mdelay(400);

    return 0;

sensor_init_fail:
    return rc; 
}

static int ov5640_sensor_config(void __user *argp)
{
    struct sensor_cfg_data cfg_data;
    long rc = 0;

    pr_err("%s: entry\n", __func__);

    if (copy_from_user(&cfg_data, (void *)argp, sizeof(struct sensor_cfg_data)))
    {
        CCRT("%s: copy_from_user failed!\n", __func__);
        return -EFAULT;
    }

    /* down(&ov5640_sem); */

    pr_err("%s: cfgtype = %d, mode = %d ,antibanding =%d\n", __func__, cfg_data.cfgtype, cfg_data.mode,cfg_data.cfg.antibanding);

    switch (cfg_data.cfgtype)
    {
        case CFG_SET_MODE:
        {
            rc = ov5640_set_sensor_mode(cfg_data.mode);
        }
        break;

        case CFG_SET_EFFECT:
        {
            rc = ov5640_set_effect(cfg_data.mode, cfg_data.cfg.effect);
        }
        break;

        case CFG_PWR_UP:
        {
            rc = ov5640_power_up();
        }
        break;

        case CFG_PWR_DOWN:
        {
            rc = ov5640_power_down();
        }
        break;

        case CFG_SET_WB:
        {
            rc = ov5640_set_wb(cfg_data.cfg.wb_mode);
        }
        break;

        case CFG_SET_BRIGHTNESS:
        {
            rc = ov5640_set_brightness(cfg_data.cfg.brightness);
        }
        break;
        
        case CFG_SET_CONTRAST:
        {
            rc = ov5640_set_contrast(cfg_data.cfg.contrast);
        }
        break;
        
        case CFG_SET_SATURATION:
        {
            rc = ov5640_set_saturation(cfg_data.cfg.saturation);
        }
        break;

        case CFG_SET_AF:
        {
            rc = ov5640_af_trigger();
        }
        break;

        case CFG_SET_ISO:
        {
            rc = ov5640_set_iso(cfg_data.cfg.iso_val);
        }
        break;

        case CFG_SET_ANTIBANDING:
        {
        	  pr_err("CFG_SET_ANTIBANDING");
            rc = ov5640_set_antibanding(cfg_data.cfg.antibanding);
        }
        break;

        case CFG_SET_SHARPNESS:
        {
            rc = ov5640_set_sharpness(cfg_data.cfg.sharpness);
        }
        break;

        case CFG_SET_EXPOSURE_COMPENSATION:
        {
            rc = ov5640_set_exposure_compensation(cfg_data.cfg.exposure);
        }
        break;

       
        case CFG_SET_AEC_RIO:
        {
            rc = ov5640_set_aec_rio(cfg_data.cfg.aec_rio);
        }
        break;
        
        default:
        {
            CCRT("%s: parameter error!\n", __func__);
            rc = -EFAULT;
        }
        break;
    }

    /* up(&ov5640_sem); */

    return rc;
}

static int ov5640_sensor_release(void)
{
    uint32_t switch_on;
    int rc = 0;

    CDBG("%s: entry\n", __func__);

    /* down(&ov5640_sem); */

    /*
      * MCLK is disabled by 
      * msm_camio_clk_disable(CAMIO_VFE_CLK)
      * in msm_camio_disable
      */

    /* Enter Into Hard Standby */
    /* ignore rc */
    switch_on = 1;
    rc = ov5640_hard_standby(ov5640_ctrl->sensordata, switch_on);

    /* up(&ov5640_sem); */

    return rc;
}

static int ov5640_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int rc = 0;

    CDBG("%s: entry\n", __func__);

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
    {
        rc = -ENOTSUPP;
        goto probe_failure;
    }

    ov5640_sensorw = kzalloc(sizeof(struct ov5640_work_t), GFP_KERNEL);
    if (!ov5640_sensorw)
    {
        rc = -ENOMEM;
        goto probe_failure;
    }

    i2c_set_clientdata(client, ov5640_sensorw);

    ov5640_client = client;

    return 0;

probe_failure:
    kfree(ov5640_sensorw);
    ov5640_sensorw = NULL;
    CCRT("%s: rc = %d, failed!\n", __func__, rc);
    return rc;
}

static int __exit ov5640_i2c_remove(struct i2c_client *client)
{
    struct ov5640_work_t *sensorw = i2c_get_clientdata(client);

    CDBG("%s: entry\n", __func__);

    free_irq(client->irq, sensorw);   
    kfree(sensorw);

    ov5640_client = NULL;
    ov5640_sensorw = NULL;

    return 0;
}

static const struct i2c_device_id ov5640_id[] = {
    { "ov5640", 0},
    { },
};

static struct i2c_driver ov5640_driver = {
    .id_table = ov5640_id,
    .probe  = ov5640_i2c_probe,
    .remove = __exit_p(ov5640_i2c_remove),
    .driver = {
        .name = "ov5640",
    },
};

static int32_t ov5640_i2c_add_driver(void)
{
    int32_t rc = 0;

    rc = i2c_add_driver(&ov5640_driver);
    if (IS_ERR_VALUE(rc))
    {
        goto init_failure;
    }

    return rc;

init_failure:
    CCRT("%s: rc = %d, failed!\n", __func__, rc);
    return rc;
}

static void ov5640_i2c_del_driver(void)
{
    i2c_del_driver(&ov5640_driver);
}

void ov5640_exit(void)
{
    CDBG("%s: entry\n", __func__);
    ov5640_i2c_del_driver();
}

int ov5640_sensor_probe(const struct msm_camera_sensor_info *info,
                               struct msm_sensor_ctrl *s)
{
    int rc = 0;

    CDBG("%s: entry\n", __func__);


#if !defined(CONFIG_SENSOR_ADAPTER)
    rc = ov5640_i2c_add_driver();
    if (rc < 0)
    {
        goto probe_failed;
    }
#endif

    rc = ov5640_sensor_probe_init(info);
    if (rc < 0)
    {
        CCRT("%s: ov5640_sensor_probe_init failed!\n", __func__);
        goto probe_failed;
    }

  
    s->s_mount_angle = 0;
    s->s_camera_type = BACK_CAMERA_2D;

    s->s_init       = ov5640_sensor_init;
    s->s_config     = ov5640_sensor_config;
    s->s_release    = ov5640_sensor_release;

    return 0;

probe_failed:
    CCRT("%s: rc = %d, failed!\n", __func__, rc);

#if !defined(CONFIG_SENSOR_ADAPTER)
    ov5640_i2c_del_driver();
#else
    // Do nothing
#endif

    return rc;
}

static int __ov5640_probe_internal(struct platform_device *pdev)
{
#if defined(CONFIG_SENSOR_ADAPTER)
 	int rc;
#else
    // Do nothing
#endif

/*
 * Get FTM flag to adjust 
 * the initialize process 
 * of camera
 */
#ifdef CONFIG_ZTE_PLATFORM
#ifdef CONFIG_ZTE_FTM_FLAG_SUPPORT
    if(zte_get_ftm_flag())
    {
        return 0;
    }
#endif
#endif


#if !defined(CONFIG_SENSOR_ADAPTER)
    return msm_camera_drv_start(pdev, ov5640_sensor_probe);
#else
    rc = msm_camera_dev_start(pdev,
                              ov5640_sensor_i2c_probe_on,
                              ov5640_sensor_i2c_probe_off,
                              ov5640_sensor_dev_probe);
    if (rc < 0)
    {
        CCRT("%s: msm_camera_dev_start failed!\n", __func__);
        goto probe_failed;
    }

    rc = msm_camera_drv_start(pdev, ov5640_sensor_probe);
    if (rc < 0)
    {
        goto probe_failed;
    }

    return 0;

probe_failed:
    CCRT("%s: rc = %d, failed!\n", __func__, rc);
   
      if(rc != -ENOINIT){
	 	pr_err("%s: rc != -ENOINIT\n", __func__);
    	msm_camera_power_backend(MSM_CAMERA_PWRDWN_MODE);
     }
    //msm_camera_power_backend(MSM_CAMERA_PWRDWN_MODE);
    return rc;
#endif
}

#if defined(OV5640_PROBE_WORKQUEUE)
/* To implement the parallel init process */
static void ov5640_workqueue(struct work_struct *work)
{
    int32_t rc;

    /*
     * ignore "rc"
     */

    rc = __ov5640_probe_internal(pdev_wq);
}

static int32_t ov5640_probe_workqueue(void)
{
    int32_t rc;


    ov5640_wq = create_singlethread_workqueue("ov5640_wq");

    if (!ov5640_wq)
    {
        CCRT("%s: ov5640_wq is NULL!\n", __func__);
        return -EFAULT;
    }

    /*
      * Ignore "rc"
      * "queue_work"'s rc:
      * 0: already in work queue
      * 1: added into work queue
      */   
    rc = queue_work(ov5640_wq, &ov5640_cb_work);

    return 0;
}

static int __ov5640_probe(struct platform_device *pdev)
{
    int32_t rc;

    pdev_wq = pdev;


    rc = ov5640_probe_workqueue();

    return rc;
}
#else
static int __ov5640_probe(struct platform_device *pdev)
{
    return __ov5640_probe_internal(pdev);
}
#endif /* OV5640_PROBE_WORKQUEUE */

static struct platform_driver msm_camera_driver = {
    .probe = __ov5640_probe,
    .driver = {
        .name = "msm_camera_ov5640",
        .owner = THIS_MODULE,
    },
};

static int __init ov5640_init(void)
{
    printk("zt debug: %s\n", __func__);//zhangtao
    return platform_driver_register(&msm_camera_driver);
}

module_init(ov5640_init);

