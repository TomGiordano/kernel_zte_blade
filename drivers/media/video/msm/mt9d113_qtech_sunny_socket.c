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

#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <media/msm_camera.h>
#include <mach/gpio.h>
#include "mt9d113.h"

#define MT9D113_SENSOR_PROBE_INIT

#ifdef MT9D113_SENSOR_PROBE_INIT
#define MT9D113_PROBE_WORKQUEUE
#endif 

#if defined(MT9D113_PROBE_WORKQUEUE)
#include <linux/workqueue.h>
static struct platform_device *pdev_wq = NULL;
static struct workqueue_struct *mt9d113_wq = NULL;
static void mt9d113_workqueue(struct work_struct *work);
static DECLARE_WORK(mt9d113_cb_work, mt9d113_workqueue);
#endif 

#define MT9D113_CAMIO_MCLK  24000000

#define MT9D113_I2C_BOARD_NAME "mt9d113"
#define MT9D113_I2C_BUS_ID  (0)

#define MT9D113_SLAVE_WR_ADDR 0x78
#define MT9D113_SLAVE_RD_ADDR 0x79 

#define REG_MT9D113_MODEL_ID    0x0000
#define MT9D113_MODEL_ID        0x2580
#define REG_MT9D113_MODEL_ID_SUB    0x31FE
#define MT9D113_MODEL_ID_SUB        0x0003

#define REG_MT9D113_STANDBY_CONTROL     0x0018
#if defined(CONFIG_MACH_R750) || defined(CONFIG_MACH_JOE)
#define MT9D113_GPIO_SWITCH_CTL     39
#define MT9D113_GPIO_SWITCH_VAL     0
#else
#undef MT9D113_GPIO_SWITCH_CTL
#undef MT9D113_GPIO_SWITCH_VAL
#endif


struct mt9d113_work_t {
    struct work_struct work;
};

struct mt9d113_ctrl_t {
    const struct msm_camera_sensor_info *sensordata;
};

static struct mt9d113_work_t *mt9d113_sensorw = NULL;
static struct i2c_client *mt9d113_client = NULL;
static struct mt9d113_ctrl_t *mt9d113_ctrl = NULL;

static uint16_t model_id;

DECLARE_MUTEX(mt9d113_sem);

static struct wake_lock mt9d113_wake_lock;

static int mt9d113_sensor_init(const struct msm_camera_sensor_info *data);
static int mt9d113_sensor_config(void __user *argp);
static int mt9d113_sensor_release(void);
static int mt9d113_sensor_release_internal(void);
static int32_t mt9d113_i2c_add_driver(void);
static void mt9d113_i2c_del_driver(void);

extern int32_t msm_camera_power_backend(enum msm_camera_pwr_mode_t pwr_mode);
extern int msm_camera_clk_switch(const struct msm_camera_sensor_info *data,
                                         uint32_t gpio_switch,
                                         uint32_t switch_val);

#ifdef CONFIG_ZTE_PLATFORM
#ifdef CONFIG_ZTE_FTM_FLAG_SUPPORT
extern int zte_get_ftm_flag(void);
#endif
#endif

static inline void mt9d113_init_suspend(void)
{
    CDBG("%s: entry\n", __func__);
    wake_lock_init(&mt9d113_wake_lock, WAKE_LOCK_IDLE, "mt9d113");
}

static inline void mt9d113_deinit_suspend(void)
{
    CDBG("%s: entry\n", __func__);
    wake_lock_destroy(&mt9d113_wake_lock);
}

static inline void mt9d113_prevent_suspend(void)
{
    CDBG("%s: entry\n", __func__);
    wake_lock(&mt9d113_wake_lock);
}

static inline void mt9d113_allow_suspend(void)
{
    CDBG("%s: entry\n", __func__);
    wake_unlock(&mt9d113_wake_lock);
}

static int mt9d113_hard_standby(const struct msm_camera_sensor_info *dev, uint32_t on)
{
    int rc;

    CDBG("%s: entry\n", __func__);

    rc = gpio_request(dev->sensor_pwd, "mt9d113");
    if (0 == rc)
    {
        rc = gpio_direction_output(dev->sensor_pwd, on);
        mdelay(10);
    }

    gpio_free(dev->sensor_pwd);

    return rc;
}

static int mt9d113_hard_reset(const struct msm_camera_sensor_info *dev)
{
    int rc = 0;

    CDBG("%s: entry\n", __func__);

    rc = gpio_request(dev->sensor_reset, "mt9d113");
    if (0 == rc)
    {
        rc = gpio_direction_output(dev->sensor_reset, 1);

        mdelay(10);
        rc = gpio_direction_output(dev->sensor_reset, 0);

        mdelay(10);

        rc = gpio_direction_output(dev->sensor_reset, 1);
        mdelay(10);
    }

    gpio_free(dev->sensor_reset);

    return rc;
}

static int32_t mt9d113_i2c_txdata(unsigned short saddr,
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

    if (i2c_transfer(mt9d113_client->adapter, msg, 1) < 0)
    {
        CCRT("%s: failed!\n", __func__);
        return -EIO;
    }

    return 0;
}

static int32_t mt9d113_i2c_write(unsigned short saddr,
                                      unsigned short waddr,
                                      unsigned short wdata,
                                      enum mt9d113_width_t width)
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

            rc = mt9d113_i2c_txdata(saddr, buf, 4);
        }
        break;

        case BYTE_LEN:
        {
            buf[0] = waddr;
            buf[1] = wdata;

            rc = mt9d113_i2c_txdata(saddr, buf, 2);
        }
        break;

        default:
        {
            rc = -EFAULT;
        }
        break;
    }

    if (rc < 0)
    {
        CCRT("%s: waddr = 0x%x, wdata = 0x%x, failed!\n", __func__, waddr, wdata);
    }

    return rc;
}

static int32_t mt9d113_i2c_write_table(struct mt9d113_i2c_reg_conf const *reg_conf_tbl,
                                             int len)
{
    uint32_t i;
    int32_t rc = 0;

#ifdef MT9D113_SENSOR_PROBE_INIT
    for (i = 0; i < len; i++)
    {
        rc = mt9d113_i2c_write(mt9d113_client->addr,
                               reg_conf_tbl[i].waddr,
                               reg_conf_tbl[i].wdata,
                               reg_conf_tbl[i].width);
        if (rc < 0)
        {
            break;
        }

        if (reg_conf_tbl[i].mdelay_time != 0)
        {
            mdelay(reg_conf_tbl[i].mdelay_time);
        }

        if (0x00 == (!(i | 0xFFFFFFE0) && 0x0F))
        {
            mdelay(1);
        }
    }
#else
    if(reg_conf_tbl == mt9d113_regs.prevsnap_tbl)
    {
        for (i = 0; i < len; i++)
        {
            rc = mt9d113_i2c_write(mt9d113_client->addr,
                                   reg_conf_tbl[i].waddr,
                                   reg_conf_tbl[i].wdata,
                                   reg_conf_tbl[i].width);
            if (rc < 0)
            {
                break;
            }

            if (reg_conf_tbl[i].mdelay_time != 0)
            {
                mdelay(reg_conf_tbl[i].mdelay_time);
            }

            if ((i < (len >> 6)) && (0x00 == (!(i | 0xFFFFFFE0) && 0x0F)))
            {
                mdelay(1);
            }   
        }
    }
    else
    {
        for (i = 0; i < len; i++)
        {
            rc = mt9d113_i2c_write(mt9d113_client->addr,
                                   reg_conf_tbl[i].waddr,
                                   reg_conf_tbl[i].wdata,
                                   reg_conf_tbl[i].width);
            if (rc < 0)
            {
                break;
            }

            if (reg_conf_tbl[i].mdelay_time != 0)
            {
                mdelay(reg_conf_tbl[i].mdelay_time);
            }
        }
    }
#endif

    return rc;
}

static int mt9d113_i2c_rxdata(unsigned short saddr,
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

    if (i2c_transfer(mt9d113_client->adapter, msgs, 2) < 0)
    {
        CCRT("%s: failed!\n", __func__);
        return -EIO;
    }

    return 0;
}

static int32_t mt9d113_i2c_read(unsigned short saddr,
                                     unsigned short raddr,
                                     unsigned short *rdata,
                                     enum mt9d113_width_t width)
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

            rc = mt9d113_i2c_rxdata(saddr, buf, 2);
            if (rc < 0)
            {
                return rc;
            }

            *rdata = buf[0] << 8 | buf[1];
        }
        break;

        default:
        {
            rc = -EFAULT;
        }
        break;
    }

    if (rc < 0)
    {
        CCRT("%s: failed!\n", __func__);
    }

    return rc;
}

static int32_t __attribute__((unused)) mt9d113_af_trigger(void)
{
    CDBG("%s: not supported!\n", __func__);
    return 0;
}

static int32_t mt9d113_set_wb(int8_t wb_mode)
{
    int32_t rc = 0;

    CDBG("%s: entry: wb_mode=%d\n", __func__, wb_mode);

    switch (wb_mode)
    {
        case CAMERA_WB_MODE_AWB:
        {
            rc = mt9d113_i2c_write_table(mt9d113_regs.wb_auto_tbl, 
                                         mt9d113_regs.wb_auto_tbl_sz);
        }
        break;

        case CAMERA_WB_MODE_SUNLIGHT:
        {
            rc = mt9d113_i2c_write_table(mt9d113_regs.wb_daylight_tbl,
                                         mt9d113_regs.wb_daylight_tbl_sz);
        }
        break;

        case CAMERA_WB_MODE_INCANDESCENT:
        {
            rc = mt9d113_i2c_write_table(mt9d113_regs.wb_incandescent_tbl,
                                         mt9d113_regs.wb_incandescent_tbl_sz);
        }
        break;
        
        case CAMERA_WB_MODE_FLUORESCENT:
        {
            rc = mt9d113_i2c_write_table(mt9d113_regs.wb_flourescant_tbl,
                                         mt9d113_regs.wb_flourescant_tbl_sz);
        }
        break; 

        case CAMERA_WB_MODE_CLOUDY:
        {
            rc = mt9d113_i2c_write_table(mt9d113_regs.wb_cloudy_tbl,
                                         mt9d113_regs.wb_cloudy_tbl_sz);
        }
        break;

        default:
        {
            CCRT("%s: parameter error!\n", __func__);
            rc = -EFAULT;
        }     
    }

    mdelay(100);

	return rc;
}    

static int32_t mt9d113_set_contrast(int8_t contrast)
{
    int32_t rc = 0;

    CDBG("%s: entry: contrast=%d\n", __func__, contrast);

    switch (contrast)
    {
        case CAMERA_CONTRAST_0:
        {
            rc = mt9d113_i2c_write_table(mt9d113_regs.contrast_tbl[0],
                                         mt9d113_regs.contrast_tbl_sz[0]);
        }
        break;

        case CAMERA_CONTRAST_1:
        {
            rc = mt9d113_i2c_write_table(mt9d113_regs.contrast_tbl[1],
                                         mt9d113_regs.contrast_tbl_sz[1]);
        }
        break;

        case CAMERA_CONTRAST_2:
        {
            rc = mt9d113_i2c_write_table(mt9d113_regs.contrast_tbl[2],
                                         mt9d113_regs.contrast_tbl_sz[2]);
        }
        break;
        
        case CAMERA_CONTRAST_3:
        {
            rc = mt9d113_i2c_write_table(mt9d113_regs.contrast_tbl[3],
                                         mt9d113_regs.contrast_tbl_sz[3]);
        }
        break; 

        case CAMERA_CONTRAST_4:
        {
            rc = mt9d113_i2c_write_table(mt9d113_regs.contrast_tbl[4],
                                         mt9d113_regs.contrast_tbl_sz[4]);
        }
        break;

        default:
        {
            CCRT("%s: parameter error!\n", __func__);
            rc = -EFAULT;
        }     
    }

    mdelay(100);

	return rc;
}

static int32_t mt9d113_set_brightness(int8_t brightness)
{
    int32_t rc = 0;

    CCRT("%s: entry: brightness=%d\n", __func__, brightness);

    switch (brightness)
    {
        case CAMERA_BRIGHTNESS_0:
        {
            rc = mt9d113_i2c_write_table(mt9d113_regs.brightness_tbl[0],
                                         mt9d113_regs.brightness_tbl_sz[0]);
        }
        break;

        case CAMERA_BRIGHTNESS_1:
        {
            rc = mt9d113_i2c_write_table(mt9d113_regs.brightness_tbl[1],
                                         mt9d113_regs.brightness_tbl_sz[1]);
        }
        break;

        case CAMERA_BRIGHTNESS_2:
        {
            rc = mt9d113_i2c_write_table(mt9d113_regs.brightness_tbl[2],
                                         mt9d113_regs.brightness_tbl_sz[2]);
        }
        break;
        
        case CAMERA_BRIGHTNESS_3:
        {
            rc = mt9d113_i2c_write_table(mt9d113_regs.brightness_tbl[3],
                                         mt9d113_regs.brightness_tbl_sz[3]);
        }
        break; 

        case CAMERA_BRIGHTNESS_4:
        {
            rc = mt9d113_i2c_write_table(mt9d113_regs.brightness_tbl[4],
                                         mt9d113_regs.brightness_tbl_sz[4]);
        }
        break;
        
        case CAMERA_BRIGHTNESS_5:
        {
            rc = mt9d113_i2c_write_table(mt9d113_regs.brightness_tbl[5],
                                         mt9d113_regs.brightness_tbl_sz[5]);
        }
        break;
        
        case CAMERA_BRIGHTNESS_6:
        {
            rc = mt9d113_i2c_write_table(mt9d113_regs.brightness_tbl[6],
                                         mt9d113_regs.brightness_tbl_sz[6]);
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

static int32_t mt9d113_set_saturation(int8_t saturation)
{
    int32_t rc = 0;

    CCRT("%s: entry: saturation=%d\n", __func__, saturation);

    switch (saturation)
    {
        case CAMERA_SATURATION_0:
        {
            rc = mt9d113_i2c_write_table(mt9d113_regs.saturation_tbl[0],
                                         mt9d113_regs.saturation_tbl_sz[0]);
        }
        break;

        case CAMERA_SATURATION_1:
        {
            rc = mt9d113_i2c_write_table(mt9d113_regs.saturation_tbl[1],
                                         mt9d113_regs.saturation_tbl_sz[1]);
        }
        break;

        case CAMERA_SATURATION_2:
        {
            rc = mt9d113_i2c_write_table(mt9d113_regs.saturation_tbl[2],
                                         mt9d113_regs.saturation_tbl_sz[2]);
        }
        break;
        
        case CAMERA_SATURATION_3:
        {
            rc = mt9d113_i2c_write_table(mt9d113_regs.saturation_tbl[3],
                                         mt9d113_regs.saturation_tbl_sz[3]);
        }
        break; 

        case CAMERA_SATURATION_4:
        {
            rc = mt9d113_i2c_write_table(mt9d113_regs.saturation_tbl[4],
                                         mt9d113_regs.saturation_tbl_sz[4]);
        }
        break;

        default:
        {
            CCRT("%s: parameter error!\n", __func__);
            rc = -EFAULT;
        }     
    }

    mdelay(100);

	return rc;
}    

static int32_t mt9d113_set_sharpness(int8_t sharpness)
{
    int32_t rc = 0;

    CDBG("%s: entry: sharpness=%d\n", __func__, sharpness);

    switch (sharpness)
    {
        case CAMERA_SHARPNESS_0:
        {
            rc = mt9d113_i2c_write_table(mt9d113_regs.sharpness_tbl[0],
                                         mt9d113_regs.sharpness_tbl_sz[0]);
        }
        break;

        case CAMERA_SHARPNESS_1:
        {
            rc = mt9d113_i2c_write_table(mt9d113_regs.sharpness_tbl[1],
                                         mt9d113_regs.sharpness_tbl_sz[1]);
        }
        break;

        case CAMERA_SHARPNESS_2:
        {
            rc = mt9d113_i2c_write_table(mt9d113_regs.sharpness_tbl[2],
                                         mt9d113_regs.sharpness_tbl_sz[2]);
        }
        break;
        
        case CAMERA_SHARPNESS_3:
        {
            rc = mt9d113_i2c_write_table(mt9d113_regs.sharpness_tbl[3],
                                         mt9d113_regs.sharpness_tbl_sz[3]);
        }
        break; 

        case CAMERA_SHARPNESS_4:
        {
            rc = mt9d113_i2c_write_table(mt9d113_regs.sharpness_tbl[4],
                                         mt9d113_regs.sharpness_tbl_sz[4]);
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

static int32_t mt9d113_set_iso(int8_t iso_val)
{
    int32_t rc = 0;

    CDBG("%s: entry: iso_val=%d\n", __func__, iso_val);

    switch (iso_val)
    {
        case CAMERA_ISO_SET_AUTO:
        {
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C,  0xA20D, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0020, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA20E, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0090, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA103, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0006, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            
            mdelay(200);
            
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C,  0xA20D, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0020, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA20E, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0090, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA103, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0005, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;

        case CAMERA_ISO_SET_HJR:
        {
             CCRT("%s: not supported!\n", __func__);
             rc = -EFAULT;
        }
        break;

        case CAMERA_ISO_SET_100:
        {
             rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C,  0xA20D, WORD_LEN);
             if (rc < 0)
             {
                 return rc;
             }
             rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0020, WORD_LEN);
             if (rc < 0)
             {
                 return rc;
             }
             rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA20E, WORD_LEN);
             if (rc < 0)
             {
                 return rc;
             }
             rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0028, WORD_LEN);
             if (rc < 0)
             {
                 return rc;
             }
             rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA103, WORD_LEN);
             if (rc < 0)
             {
                 return rc;
             }
             rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0005, WORD_LEN);
             if (rc < 0)
             {
                 return rc;
             }    
        }
        break;

        case CAMERA_ISO_SET_200:
        {
             rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C,  0xA20D, WORD_LEN);
             if (rc < 0)
             {
                return rc;
             }
             rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0040, WORD_LEN);
             if (rc < 0)
             {
                return rc;
             }
             rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA20E, WORD_LEN);
             if (rc < 0)
             {
                return rc;
             }
             rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0048, WORD_LEN);
             if (rc < 0)
             {
                return rc;
             }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA103, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0005, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }    
        }
        break;

        case CAMERA_ISO_SET_400:
        {
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C,  0xA20D, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0050, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA20E, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0080, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA103, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0005, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }  
        }
        break;

        case CAMERA_ISO_SET_800:
        {
             CCRT("%s: not supported!\n", __func__);
             rc = -EFAULT;
        }
        break;

        default:
        {
            CCRT("%s: parameter error!\n", __func__);
            rc = -EFAULT;
        }     
    }

    mdelay(100);

	return rc;
} 

static int32_t  mt9d113_set_antibanding(int8_t antibanding)
{
    int32_t rc = 0;

    CDBG("%s: entry: antibanding=%d\n", __func__, antibanding);

    switch (antibanding)
    {
        case CAMERA_ANTIBANDING_SET_OFF:
        {
            CCRT("%s: CAMERA_ANTIBANDING_SET_OFF NOT supported!\n", __func__);
        }
        break;

        case CAMERA_ANTIBANDING_SET_60HZ:
        {
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA118, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0002, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA11E, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0002, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA124, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0002, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }    
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA12A, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0002, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }  
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA404, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }  
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x00A0, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA103, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0005, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }            
        break;

        case CAMERA_ANTIBANDING_SET_50HZ:
        {
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA118, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0002, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA11E, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0002, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA124, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0002, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }    
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA12A, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0002, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }  
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA404, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }  
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x00E0, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA103, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0005, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;

        case CAMERA_ANTIBANDING_SET_AUTO:
        {
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA118, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0001, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA11E, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0001, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA124, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0000, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }    
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA12A, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0001, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }  
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA103, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0005, WORD_LEN);
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

    mdelay(100);

	return rc;
} 

static int32_t __attribute__((unused))mt9d113_set_lensshading(int8_t lensshading)
{
#if 0
    int32_t rc = 0;
    uint16_t brightness_lev = 0;

    CDBG("%s: entry: lensshading=%d\n", __func__, lensshading);

    if (0 == lensshading)
    {
        CCRT("%s: lens shading is disabled!\n", __func__);
        return rc;
    }

    rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098E, 0x3835, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }

    rc = mt9d113_i2c_read(mt9d113_client->addr, 0x0990, &brightness_lev, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }

    if (brightness_lev < 5)
    {
        rc = mt9d113_i2c_write_table(mt9d113_regs.lens_for_outdoor_tbl, mt9d113_regs.lens_for_outdoor_tbl_sz);
        if (rc < 0)
        {
            return rc;
        } 
    }
    else
    {
        rc = mt9d113_i2c_write_table(mt9d113_regs.lens_for_indoor_tbl, mt9d113_regs.lens_for_indoor_tbl_sz);
        if (rc < 0)
        {
            return rc;
        } 
    }

    return rc;
#else
    return 0;
#endif
}

static long mt9d113_set_exposure_compensation(int8_t exposure)
{
    long rc = 0;

    CDBG("%s: entry: exposure=%d\n", __func__, exposure);

    switch(exposure)
    {
        case CAMERA_EXPOSURE_0:
        {
            
        }
        break;
        
        case CAMERA_EXPOSURE_1:
        {
            
        }
        break;

        case CAMERA_EXPOSURE_2:
        {
            
        }
        break;

        case CAMERA_EXPOSURE_3:
        {
            
        }
        break;

        case CAMERA_EXPOSURE_4:
        {
            
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

static long mt9d113_reg_init(void)
{
    long rc;

    CDBG("%s: entry\n", __func__);

    rc = mt9d113_i2c_write_table(mt9d113_regs.prevsnap_tbl, mt9d113_regs.prevsnap_tbl_sz);
    if (rc < 0)
    {
        return rc;
    }

    

    rc = mt9d113_i2c_write(mt9d113_client->addr, REG_MT9D113_STANDBY_CONTROL, 0x0028, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }
    mdelay(10);

    rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA103, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }
    rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0006, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }

    mdelay(300);

    rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA103, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }

    rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0005, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }

    return 0;
}

static long mt9d113_set_sensor_mode(int32_t mode)
{
    long rc = 0;

    CDBG("%s: entry\n", __func__);

    switch (mode)
    {
        case SENSOR_PREVIEW_MODE:
        {
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA115, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }

            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0000, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }

            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA103, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }

            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0001, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }

            mdelay(80);
        }
        break;

        case SENSOR_SNAPSHOT_MODE:
        {
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA115, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }

            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0002, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }

            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA103, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }

            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0002, WORD_LEN);
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

    return 0;
}

static long mt9d113_set_effect(int32_t mode, int32_t effect)
{
    uint16_t __attribute__((unused)) reg_addr;
    uint16_t __attribute__((unused)) reg_val;
    long rc = 0;

    switch (mode)
    {
        case SENSOR_PREVIEW_MODE:
        {
        }
        break;

        case SENSOR_SNAPSHOT_MODE:
        {
        }
        break;

        default:
        {
        }
        break;
    }

    switch (effect)
    {
        case CAMERA_EFFECT_OFF:
        {
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0x2759, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x6440, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0x275B, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x6440, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA103, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0005, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }          
        }            
        break;

        case CAMERA_EFFECT_MONO:
        {
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0x2759, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x6441, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0x275B, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x6441, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA103, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0005, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }          
        }
        break;

        case CAMERA_EFFECT_NEGATIVE:
        {
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0x2759, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x6443, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0x275B, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x6443, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA103, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0005, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }          
        }
        break;

        case CAMERA_EFFECT_SOLARIZE:
        {
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0x2759, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x6444, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0x275B, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x6444, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA103, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0005, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }          
        }            
        break;

        case CAMERA_EFFECT_SEPIA:
        {
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0x2763, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0xB023, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0x2759, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x6442, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0x275B, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x6442, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x098C, 0xA103, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0990, 0x0005, WORD_LEN);
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

    mdelay(100);

    return rc;
}

static long mt9d113_power_up(void)
{
    CDBG("%s: not supported!\n", __func__);
    return 0;
}

static long mt9d113_power_down(void)
{
    CDBG("%s: not supported!\n", __func__);
    return 0;
}

#if 0 
static int mt9d113_power_shutdown(uint32_t on)
{
    int rc;

    CDBG("%s: entry\n", __func__);

    rc = gpio_request(MT9D113_GPIO_SHUTDOWN_CTL, "mt9d113");
    if (0 == rc)
    {
        rc = gpio_direction_output(MT9D113_GPIO_SHUTDOWN_CTL, on);
        mdelay(1);
    }

    gpio_free(MT9D113_GPIO_SHUTDOWN_CTL);

    return rc;
}
#endif

#if !defined(CONFIG_SENSOR_ADAPTER)
static int mt9d113_sensor_init_probe(const struct msm_camera_sensor_info *data)
{
    uint32_t switch_on;
    int rc = 0;

    CDBG("%s: entry\n", __func__);


#if 0 
    switch_on = 0;
    rc = mt9d113_power_shutdown(switch_on);
    if (rc < 0)
    {
        CCRT("enter/quit lowest-power mode failed!\n");
        goto init_probe_fail;
    }
#endif

    switch_on = 0;
    rc = mt9d113_hard_standby(data, switch_on);
    if (rc < 0)
    {
        CCRT("set standby failed!\n");
        goto init_probe_fail;
    }
    rc = mt9d113_hard_reset(data);
    if (rc < 0)
    {
        CCRT("hard reset failed!\n");
        goto init_probe_fail;
    }

    model_id = 0x0000;
    rc = mt9d113_i2c_read(mt9d113_client->addr, REG_MT9D113_MODEL_ID, &model_id, WORD_LEN);
    if (rc < 0)
    {
        goto init_probe_fail;
    }

    CDBG("%s: model_id = 0x%x\n", __func__, model_id);
#ifdef CONFIG_SENSOR_INFO
    msm_sensorinfo_set_sensor_id(model_id);
#else
    
#endif

    if (model_id != MT9D113_MODEL_ID)
    {
        rc = -EFAULT;
        goto init_probe_fail;
    }
    rc = mt9d113_i2c_write_table(mt9d113_regs.pll_tbl, mt9d113_regs.pll_tbl_sz);
    if (rc < 0)
    {
        return rc;
    }

    model_id = 0x0000;
    rc = mt9d113_i2c_read(mt9d113_client->addr, REG_MT9D113_MODEL_ID_SUB, &model_id, WORD_LEN);
    if (rc < 0)
    {
        goto init_probe_fail;
    }

    CDBG("%s: model_id_sub = 0x%x\n", __func__, model_id);
#ifdef CONFIG_SENSOR_INFO
    msm_sensorinfo_set_sensor_id(model_id);
#else
    
#endif

    if (model_id != MT9D113_MODEL_ID_SUB)
    {
        rc = -EFAULT;
        goto init_probe_fail;
    }
    rc = mt9d113_reg_init();
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
static int mt9d113_sensor_i2c_probe_on(void)
{
    int rc;
    struct i2c_board_info info;
    struct i2c_adapter *adapter;
    struct i2c_client *client;

    rc = mt9d113_i2c_add_driver();
    if (rc < 0)
    {
        CCRT("%s: add i2c driver failed!\n", __func__);
        return rc;
    }

    memset(&info, 0, sizeof(struct i2c_board_info));
    info.addr = MT9D113_SLAVE_WR_ADDR >> 1;
    strlcpy(info.type, MT9D113_I2C_BOARD_NAME, I2C_NAME_SIZE);

    adapter = i2c_get_adapter(MT9D113_I2C_BUS_ID);
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

    mt9d113_client = client;

    return 0;

i2c_probe_failed:
    mt9d113_i2c_del_driver();
    return -ENODEV;
}

static void mt9d113_sensor_i2c_probe_off(void)
{
    i2c_unregister_device(mt9d113_client);
    mt9d113_i2c_del_driver();
}

static int mt9d113_sensor_dev_probe(const struct msm_camera_sensor_info *pinfo)
{
    int rc;

    rc = msm_camera_power_backend(MSM_CAMERA_PWRUP_MODE);
    if (rc < 0)
    {
        CCRT("%s: camera_power_backend failed!\n", __func__);
        return rc;
    }

#if defined(CONFIG_MACH_R750) || defined(CONFIG_MACH_JOE)
    rc = msm_camera_clk_switch(pinfo, MT9D113_GPIO_SWITCH_CTL, MT9D113_GPIO_SWITCH_VAL);
    if (rc < 0)
    {
        CCRT("%s: camera_clk_switch failed!\n", __func__);
        return rc;;
    }
#else
    
#endif

    msm_camio_clk_rate_set(MT9D113_CAMIO_MCLK);
    mdelay(5);

    rc = mt9d113_hard_standby(pinfo, 0);
    if (rc < 0)
    {
        CCRT("set standby failed!\n");
        return rc;
    }

    rc = mt9d113_hard_reset(pinfo);
    if (rc < 0)
    {
        CCRT("hard reset failed!\n");
        return rc;
    }

    model_id = 0x0000;
    rc = mt9d113_i2c_read(mt9d113_client->addr, REG_MT9D113_MODEL_ID, &model_id, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }

    CDBG("%s: model_id = 0x%x\n", __func__, model_id);

#ifdef CONFIG_SENSOR_INFO
	msm_sensorinfo_set_sensor_id(model_id);
#else
    
#endif

    if (model_id != MT9D113_MODEL_ID)
    {
        return -EFAULT;
    }


    rc = mt9d113_i2c_write_table(mt9d113_regs.pll_tbl, mt9d113_regs.pll_tbl_sz);
    if (rc < 0)
    {
        return rc;
    }

    model_id = 0x0000;
    rc = mt9d113_i2c_read(mt9d113_client->addr, REG_MT9D113_MODEL_ID_SUB, &model_id, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }

    CDBG("%s: model_id_sub = 0x%x\n", __func__, model_id);

#ifdef CONFIG_SENSOR_INFO
	msm_sensorinfo_set_sensor_id(model_id);
#else
    
#endif

    if (model_id != MT9D113_MODEL_ID_SUB)
    {
        return -EFAULT;
    }

    return 0;
}
#endif

static int mt9d113_sensor_probe_init(const struct msm_camera_sensor_info *data)
{
    int rc;

    CDBG("%s: entry\n", __func__);

    if (!data || strcmp(data->sensor_name, "mt9d113"))
    {
        CCRT("%s: invalid parameters!\n", __func__);
        rc = -ENODEV;
        goto probe_init_fail;
    }

    mt9d113_ctrl = kzalloc(sizeof(struct mt9d113_ctrl_t), GFP_KERNEL);
    if (!mt9d113_ctrl)
    {
        CCRT("%s: kzalloc failed!\n", __func__);
        rc = -ENOMEM;
        goto probe_init_fail;
    }

    mt9d113_ctrl->sensordata = data;

#if !defined(CONFIG_SENSOR_ADAPTER)
    rc = msm_camera_power_backend(MSM_CAMERA_PWRUP_MODE);
    if (rc < 0)
    {
        CCRT("%s: camera_power_backend failed!\n", __func__);
        goto probe_init_fail;
    }

#if defined(CONFIG_MACH_R750) || defined(CONFIG_MACH_JOE)
    rc = msm_camera_clk_switch(mt9d113_ctrl->sensordata, MT9D113_GPIO_SWITCH_CTL, MT9D113_GPIO_SWITCH_VAL);
    if (rc < 0)
    {
        CCRT("%s: camera_clk_switch failed!\n", __func__);
        goto probe_init_fail;
    }
#else
    
#endif

    msm_camio_clk_rate_set(MT9D113_CAMIO_MCLK);
    mdelay(5);

    rc = mt9d113_sensor_init_probe(mt9d113_ctrl->sensordata);
    if (rc < 0)
    {
        CCRT("%s: sensor_init_probe failed!\n", __func__);
        goto probe_init_fail;
    }
#else
    rc = mt9d113_sensor_dev_probe(mt9d113_ctrl->sensordata);
    if (rc < 0)
    {
        CCRT("%s: mt9d113_sensor_dev_probe failed!\n", __func__);
        goto probe_init_fail;
    }

    rc = mt9d113_reg_init();
    if (rc < 0)
    {
        CCRT("%s: mt9d113_reg_init failed!\n", __func__);
        goto probe_init_fail;
    }
#endif

    return 0;

probe_init_fail:
    msm_camera_power_backend(MSM_CAMERA_PWRDWN_MODE);
    if(mt9d113_ctrl)
    {
        kfree(mt9d113_ctrl);
    }
    return rc;
}

#ifdef MT9D113_SENSOR_PROBE_INIT
static int mt9d113_sensor_init(const struct msm_camera_sensor_info *data)
{
    uint32_t switch_on; 
    int rc;

    CDBG("%s: entry\n", __func__);

    if ((NULL == data)
        || strcmp(data->sensor_name, "mt9d113")
        || strcmp(mt9d113_ctrl->sensordata->sensor_name, "mt9d113"))
    {
        CCRT("%s: data is NULL, or sensor_name is not equal to mt9d113!\n", __func__);
        rc = -ENODEV;
        goto sensor_init_fail;
    }
    
    rc = msm_camera_power_backend(MSM_CAMERA_NORMAL_MODE);
    if (rc < 0)
    {
        CCRT("%s: camera_power_backend failed!\n", __func__);
        goto sensor_init_fail;
    }

    msm_camio_clk_rate_set(MT9D113_CAMIO_MCLK);
    mdelay(5);

    msm_camio_camif_pad_reg_reset();
    mdelay(10);

    switch_on = 0;
    rc = mt9d113_hard_standby(mt9d113_ctrl->sensordata, switch_on);
    if (rc < 0)
    {
        CCRT("set standby failed!\n");
        goto sensor_init_fail;
    }
    mdelay(10);

    return 0;

sensor_init_fail:
    return rc;
}
#else
static int mt9d113_sensor_init(const struct msm_camera_sensor_info *data)
{
    int rc;

    rc = mt9d113_sensor_probe_init(data);

    return rc;
}
#endif 

static int mt9d113_sensor_config(void __user *argp)
{
    struct sensor_cfg_data cfg_data;
    long rc = 0;

    CDBG("%s: entry\n", __func__);

    if (copy_from_user(&cfg_data, (void *)argp, sizeof(struct sensor_cfg_data)))
    {
        CCRT("%s: copy_from_user failed!\n", __func__);
        return -EFAULT;
    }

    CDBG("%s: cfgtype = %d, mode = %d\n", __func__, cfg_data.cfgtype, cfg_data.mode);

    switch (cfg_data.cfgtype)
    {
        case CFG_SET_MODE:
        {
            rc = mt9d113_set_sensor_mode(cfg_data.mode);
        }
        break;

        case CFG_SET_EFFECT:
        {
            rc = mt9d113_set_effect(cfg_data.mode, cfg_data.cfg.effect);
        }
        break;

        case CFG_PWR_UP:
        {
            rc = mt9d113_power_up();
        }
        break;

        case CFG_PWR_DOWN:
        {
            rc = mt9d113_power_down();
        }
        break;

        case CFG_SET_WB:
        {
            rc = mt9d113_set_wb(cfg_data.cfg.wb_mode);
        }
        break;

        case CFG_SET_AF:
        {
            
            
            rc = 0;
        }
        break;

        case CFG_SET_ISO:
        {
            rc = mt9d113_set_iso(cfg_data.cfg.iso_val);
        }
        break;

        case CFG_SET_ANTIBANDING:
        {
            rc = mt9d113_set_antibanding(cfg_data.cfg.antibanding);
        }
        break;

        case CFG_SET_BRIGHTNESS:
        {
            rc = mt9d113_set_brightness(cfg_data.cfg.brightness);
        }
        break;

        case CFG_SET_SATURATION:
        {
            rc = mt9d113_set_saturation(cfg_data.cfg.saturation);
        }
        break;

        case CFG_SET_CONTRAST:
        {
            rc = mt9d113_set_contrast(cfg_data.cfg.contrast);
        }
        break;

        case CFG_SET_SHARPNESS:
        {
            rc = mt9d113_set_sharpness(cfg_data.cfg.sharpness);
        }
        break;

        case CFG_SET_LENS_SHADING:
        {
            
            rc = 0;
        }
        break;

        case CFG_SET_EXPOSURE_COMPENSATION:
        {
            rc = mt9d113_set_exposure_compensation(cfg_data.cfg.exposure);
        }
        break;

        default:
        {
            rc = -EFAULT;
        }
        break;
    }

    mt9d113_prevent_suspend();

    return rc;
}

#ifdef MT9D113_SENSOR_PROBE_INIT
static int mt9d113_sensor_release_internal(void)
{
    int rc;
    uint32_t switch_on;

    CDBG("%s: entry\n", __func__);
    rc = mt9d113_i2c_write(mt9d113_client->addr, 0x0028, 0x0000, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }
    mdelay(1);

    switch_on = 1;
    rc = mt9d113_hard_standby(mt9d113_ctrl->sensordata, switch_on);
    if (rc < 0)
    {
        return rc;
    }
    mdelay(200);

    rc = msm_camera_power_backend(MSM_CAMERA_STANDBY_MODE);
    if (rc < 0)
    {
        return rc;
    }


    return 0;
}
#else
static int mt9d113_sensor_release_internal(void)
{
    int rc;
    rc = msm_camera_power_backend(MSM_CAMERA_PWRDWN_MODE);

    kfree(mt9d113_ctrl);

    return rc;
}
#endif 

static int mt9d113_sensor_release(void)
{
    int rc;

    rc = mt9d113_sensor_release_internal();

    mt9d113_allow_suspend();

    return rc;
}

static int mt9d113_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int rc = 0;

    CDBG("%s: entry\n", __func__);

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
    {
        rc = -ENOTSUPP;
        goto probe_failure;
    }

    mt9d113_sensorw = kzalloc(sizeof(struct mt9d113_work_t), GFP_KERNEL);
    if (!mt9d113_sensorw)
    {
        rc = -ENOMEM;
        goto probe_failure;
    }

    i2c_set_clientdata(client, mt9d113_sensorw);

    mt9d113_client = client;

    return 0;

probe_failure:
    kfree(mt9d113_sensorw);
    mt9d113_sensorw = NULL;
    CCRT("%s: rc = %d, failed!\n", __func__, rc);
    return rc;
}

static int __exit mt9d113_i2c_remove(struct i2c_client *client)
{
    struct mt9d113_work_t *sensorw = i2c_get_clientdata(client);

    CDBG("%s: entry\n", __func__);

    free_irq(client->irq, sensorw);   
    kfree(sensorw);

    mt9d113_deinit_suspend();

    mt9d113_client = NULL;
    mt9d113_sensorw = NULL;

    return 0;
}

static const struct i2c_device_id mt9d113_id[] = {
    { "mt9d113", 0},
    { },
};

static struct i2c_driver mt9d113_driver = {
    .id_table = mt9d113_id,
    .probe  = mt9d113_i2c_probe,
    .remove = __exit_p(mt9d113_i2c_remove),
    .driver = {
        .name = MT9D113_I2C_BOARD_NAME,
    },
};

static int32_t mt9d113_i2c_add_driver(void)
{
    int32_t rc = 0;

    rc = i2c_add_driver(&mt9d113_driver);
    if (IS_ERR_VALUE(rc))
    {
        goto init_failure;
    }

    return rc;

init_failure:
    CCRT("%s: rc = %d, failed!\n", __func__, rc);
    return rc;
}

static void mt9d113_i2c_del_driver(void)
{
    i2c_del_driver(&mt9d113_driver);
}

void mt9d113_exit(void)
{
    CDBG("%s: entry\n", __func__);
    mt9d113_i2c_del_driver();
}

int mt9d113_sensor_probe(const struct msm_camera_sensor_info *info,
                                struct msm_sensor_ctrl *s)
{
    int rc;

    CDBG("%s: entry\n", __func__);

#if !defined(CONFIG_SENSOR_ADAPTER)
    rc = mt9d113_i2c_add_driver();
    if (rc < 0)
    {
        goto probe_failed;
    }
#else
    
#endif

#ifdef MT9D113_SENSOR_PROBE_INIT
    rc = mt9d113_sensor_probe_init(info);
    if (rc < 0)
    {
        CCRT("%s: mt9d113_sensor_probe_init failed!\n", __func__);
        goto probe_failed;
    }

    rc = mt9d113_sensor_release_internal();
    if (rc < 0)
    {
        CCRT("%s: mt9d113_sensor_release failed!\n", __func__);
        goto probe_failed;
    }
#endif 

    mt9d113_init_suspend();

    s->s_init       = mt9d113_sensor_init;
    s->s_config     = mt9d113_sensor_config;
    s->s_release    = mt9d113_sensor_release;

    return 0;

probe_failed:
    CCRT("%s: rc = %d, failed!\n", __func__, rc);

#if !defined(CONFIG_SENSOR_ADAPTER)
    mt9d113_i2c_del_driver();
#else
    
#endif

    return rc;
}

#if defined(MT9D113_PROBE_WORKQUEUE)
static void mt9d113_workqueue(struct work_struct *work)
{
    int32_t rc;
#ifdef CONFIG_ZTE_PLATFORM
#ifdef CONFIG_ZTE_FTM_FLAG_SUPPORT
    if(zte_get_ftm_flag())
    {
        return;
    }
#endif
#endif

    if (!pdev_wq)
    {
        CCRT("%s: pdev_wq is NULL!\n", __func__);
        return;
    }

#if !defined(CONFIG_SENSOR_ADAPTER)
    rc = msm_camera_drv_start(pdev_wq, mt9d113_sensor_probe);
#else
    rc = msm_camera_dev_start(pdev_wq,
                              mt9d113_sensor_i2c_probe_on,
                              mt9d113_sensor_i2c_probe_off,
                              mt9d113_sensor_dev_probe);
    if (rc < 0)
    {
        CCRT("%s: msm_camera_dev_start failed!\n", __func__);
        goto probe_failed;
    }

    rc = msm_camera_drv_start(pdev_wq, mt9d113_sensor_probe);
    if (rc < 0)
    {
        goto probe_failed;
    }

    return;

probe_failed:
    CCRT("%s: rc = %d, failed!\n", __func__, rc);
    msm_camera_power_backend(MSM_CAMERA_PWRDWN_MODE);
    return;
#endif
}

static int32_t mt9d113_probe_workqueue(void)
{
    int32_t rc;

    mt9d113_wq = create_singlethread_workqueue("mt9d113_wq");

    if (!mt9d113_wq)
    {
        CCRT("%s: mt9d113_wq is NULL!\n", __func__);
        return -EFAULT;
    }

    rc = queue_work(mt9d113_wq, &mt9d113_cb_work);

    return 0;
}

static int __mt9d113_probe(struct platform_device *pdev)
{
    int32_t rc;

    pdev_wq = pdev;

    rc = mt9d113_probe_workqueue();

    return rc;
}
#else
static int __mt9d113_probe(struct platform_device *pdev)
{
#ifdef CONFIG_ZTE_PLATFORM
#ifdef CONFIG_ZTE_FTM_FLAG_SUPPORT
    if(zte_get_ftm_flag())
    {
        return 0;
    }
#endif
#endif

    return msm_camera_drv_start(pdev, mt9d113_sensor_probe);
}
#endif 

static struct platform_driver msm_camera_driver = {
    .probe = __mt9d113_probe,
    .driver = {
        .name = "msm_camera_mt9d113",
        .owner = THIS_MODULE,
    },
};

static int __init mt9d113_init(void)
{
    return platform_driver_register(&msm_camera_driver);
}

module_init(mt9d113_init);

