/*
 * drivers/media/video/msm/mt9d115_qtech_sunny_socket.c
 *
 * Refer to drivers/media/video/msm/mt9d112.c
 * For MT9D115: 2.0Mp, 1/5-Inch System-On-A-Chip (SOC) CMOS Digital Image Sensor
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
#include "mt9d115.h"
#include <linux/slab.h>

/*-----------------------------------------------------------------------------------------
 *
 * MACRO DEFINITION
 *
 *----------------------------------------------------------------------------------------*/
/*
 * For Sensor Init During System Startup
 * otherwise, sensor is initialized during
 * camera app startup
 *
 * Attention: "CONFIG_SENSOR_ADAPTER" should support "MT9D115_SENSOR_PROBE_INIT" only
 */
#define MT9D115_SENSOR_PROBE_INIT

#ifdef MT9D115_SENSOR_PROBE_INIT
/*
 * To implement the parallel init process
 */
#define MT9D115_PROBE_WORKQUEUE
#endif /* define MT9D115_SENSOR_PROBE_INIT */

#if defined(MT9D115_PROBE_WORKQUEUE)
#include <linux/workqueue.h>
static struct platform_device *pdev_wq = NULL;
static struct workqueue_struct *mt9d115_wq = NULL;
static void mt9d115_workqueue(struct work_struct *work);
static DECLARE_WORK(mt9d115_cb_work, mt9d115_workqueue);
#endif /* defined(MT9D115_PROBE_WORKQUEUE) */

/*
 * CAMIO Input MCLK (MHz)
 *
 * MCLK: 6-54 MHz
 *
 * maximum frame rate: 
 * 15 fps at full resolution (JPEG),
 * 30 fps in preview mode
 *
 * when MCLK=40MHZ, PCLK=48MHZ (PLL is enabled by sensor)
 * when MCLK=48MHZ, PCLK=48MHZ (PLL is disabled by sensor)
 *
 * 54MHz is the maximum value accepted by sensor
 */
#define MT9D115_CAMIO_MCLK  24000000

/*
 * Micron MT9D115 Registers and their values
 */
/* Sensor I2C Board Name */
#define MT9D115_I2C_BOARD_NAME "mt9d115"

/* Sensor I2C Bus Number (Master I2C Controller: 0) */
#define MT9D115_I2C_BUS_ID  (0)

/* Sensor I2C Slave Address */
#define MT9D115_SLAVE_WR_ADDR 0x78 /* replaced by "msm_i2c_devices.addr" */
#define MT9D115_SLAVE_RD_ADDR 0x79 /* replaced by "msm_i2c_devices.addr" */

/* Sensor I2C Device ID */
#define REG_MT9D115_MODEL_ID    0x0000
#define MT9D115_MODEL_ID        0x2580

/* Sensor I2C Device Sub ID */
#define REG_MT9D115_MODEL_ID_SUB    0x31FE
#define MT9D115_MODEL_ID_SUB        0x0011

/* SOC Registers */
#define REG_MT9D115_STANDBY_CONTROL     0x0018

/*
 * GPIO For Sensor Clock Switch
 */
#if defined(CONFIG_MACH_R750) || defined(CONFIG_MACH_JOE)
#define MT9D115_GPIO_SWITCH_CTL     39
#define MT9D115_GPIO_SWITCH_VAL     0
#else
#undef MT9D115_GPIO_SWITCH_CTL
#undef MT9D115_GPIO_SWITCH_VAL
#endif

/* 
 * GPIO For Lowest-Power mode (SHUTDOWN mode)
 * #define MT9D115_GPIO_SHUTDOWN_CTL   Dummy
 */

/*-----------------------------------------------------------------------------------------
 *
 * TYPE DECLARATION
 *
 *----------------------------------------------------------------------------------------*/
struct mt9d115_work_t {
    struct work_struct work;
};

struct mt9d115_ctrl_t {
    const struct msm_camera_sensor_info *sensordata;
};

/*-----------------------------------------------------------------------------------------
 *
 * GLOBAL VARIABLE DEFINITION
 *
 *----------------------------------------------------------------------------------------*/
static struct mt9d115_work_t *mt9d115_sensorw = NULL;
static struct i2c_client *mt9d115_client = NULL;
static struct mt9d115_ctrl_t *mt9d115_ctrl = NULL;

/*
 * For coexistence of MT9T111, MT9T112 and MT9D115
 */
static uint16_t model_id;

static int8_t current_brightness = CAMERA_BRIGHTNESS_3;
static int8_t current_exposure = CAMERA_EXPOSURE_2;

DECLARE_MUTEX(mt9d115_sem);

static struct wake_lock mt9d115_wake_lock;

/*-----------------------------------------------------------------------------------------
 *
 * FUNCTION DECLARATION
 *
 *----------------------------------------------------------------------------------------*/
static int mt9d115_sensor_init(const struct msm_camera_sensor_info *data);
static int mt9d115_sensor_config(void __user *argp);
static int mt9d115_sensor_release(void);
static int mt9d115_sensor_release_internal(void);
static int32_t mt9d115_i2c_add_driver(void);
static void mt9d115_i2c_del_driver(void);

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
static inline void mt9d115_init_suspend(void)
{
    CDBG("%s: entry\n", __func__);
    wake_lock_init(&mt9d115_wake_lock, WAKE_LOCK_IDLE, "mt9d115");
}

static inline void mt9d115_deinit_suspend(void)
{
    CDBG("%s: entry\n", __func__);
    wake_lock_destroy(&mt9d115_wake_lock);
}

static inline void mt9d115_prevent_suspend(void)
{
    CDBG("%s: entry\n", __func__);
    wake_lock(&mt9d115_wake_lock);
}

static inline void mt9d115_allow_suspend(void)
{
    CDBG("%s: entry\n", __func__);
    wake_unlock(&mt9d115_wake_lock);
}

/*
 * Hard standby of sensor
 * on: =1, enter into hard standby
 * on: =0, exit from hard standby
 *
 * Hard standby mode is set by register of REG_MT9D115_STANDBY_CONTROL.
 */
static int mt9d115_hard_standby(const struct msm_camera_sensor_info *dev, uint32_t on)
{
    int rc;

    CDBG("%s: entry\n", __func__);

    rc = gpio_request(dev->sensor_pwd, "mt9d115");
    if (0 == rc)
    {
        /* ignore "rc" */
        rc = gpio_direction_output(dev->sensor_pwd, on);

        /*
         * time delay for entry into standby
         * decrease time delay from 200ms to 10ms
         * to improve sensor init
         */
        mdelay(10);
    }

    gpio_free(dev->sensor_pwd);

    return rc;
}

/*
 * Hard reset: RESET_BAR pin (active LOW)
 * Hard reset has the same effect as the soft reset.
 */
static int mt9d115_hard_reset(const struct msm_camera_sensor_info *dev)
{
    int rc = 0;

    CDBG("%s: entry\n", __func__);

    rc = gpio_request(dev->sensor_reset, "mt9d115");
    if (0 == rc)
    {
        /* ignore "rc" */
        rc = gpio_direction_output(dev->sensor_reset, 1);

        mdelay(10);

        /* ignore "rc" */
        rc = gpio_direction_output(dev->sensor_reset, 0);

        /*
          * RESET_BAR pulse width: Min 70 EXTCLKs
          * EXTCLKs: = MCLK (i.e., MT9D115_CAMIO_MCLK)
          */
        mdelay(10);

        /* ignore "rc" */
        rc = gpio_direction_output(dev->sensor_reset, 1);

        /*
          * Time delay before first serial write: Min 100 EXTCLKs
          * EXTCLKs: = MCLK (i.e., MT9D115_CAMIO_MCLK)
          */
        mdelay(10);
    }

    gpio_free(dev->sensor_reset);

    return rc;
}

static int32_t mt9d115_i2c_txdata(unsigned short saddr,
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

    if (i2c_transfer(mt9d115_client->adapter, msg, 1) < 0)
    {
        CCRT("%s: failed!\n", __func__);
        return -EIO;
    }

    return 0;
}

static int32_t mt9d115_i2c_write(unsigned short saddr,
                                      unsigned short waddr,
                                      unsigned short wdata,
                                      enum mt9d115_width_t width)
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

            rc = mt9d115_i2c_txdata(saddr, buf, 4);
        }
        break;

        case BYTE_LEN:
        {
            buf[0] = waddr;
            buf[1] = wdata;

            rc = mt9d115_i2c_txdata(saddr, buf, 2);
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

static int32_t mt9d115_i2c_write_table(struct mt9d115_i2c_reg_conf const *reg_conf_tbl,
                                             int len)
{
    uint32_t i;
    int32_t rc = 0;

#ifdef MT9D115_SENSOR_PROBE_INIT
    for (i = 0; i < len; i++)
    {
        rc = mt9d115_i2c_write(mt9d115_client->addr,
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

        /*
          * To fix the bug of preview failure
          * time delay of 1ms is recommended after writing per 16 items (0x10)
          */
        if (0x00 == (!(i | 0xFFFFFFE0) && 0x0F))
        {
            mdelay(1);
        }
    }
#else
    /*
      * To fix the bug of preview failure
      * time delay of 1ms is recommended after writing per 16 items (0x10)
      *
      * Attention: to improve sensor init, time delay is only set for
      * prevsnap_tbl
      */
    if(reg_conf_tbl == mt9d115_regs.prevsnap_tbl)
    {
        for (i = 0; i < len; i++)
        {
            rc = mt9d115_i2c_write(mt9d115_client->addr,
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
            rc = mt9d115_i2c_write(mt9d115_client->addr,
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

static int mt9d115_i2c_rxdata(unsigned short saddr,
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

    if (i2c_transfer(mt9d115_client->adapter, msgs, 2) < 0)
    {
        CCRT("%s: failed!\n", __func__);
        return -EIO;
    }

    return 0;
}

static int32_t mt9d115_i2c_read(unsigned short saddr,
                                     unsigned short raddr,
                                     unsigned short *rdata,
                                     enum mt9d115_width_t width)
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

            rc = mt9d115_i2c_rxdata(saddr, buf, 2);
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

/*
 * Auto Focus Trigger (not supported)
 */
static int32_t __attribute__((unused)) mt9d115_af_trigger(void)
{
    CDBG("%s: not supported!\n", __func__);
    return 0;
}


static int32_t mt9d115_set_exposure_brightness(int8_t exposure, int8_t brightness)
{
    int32_t rc = 0;

    CDBG("%s: entry: exposure=%d, brightness=%d\n", __func__, exposure, brightness);

    rc = mt9d115_i2c_write_table(mt9d115_regs.brightness_exposure_tbl[exposure*CAMERA_BRIGHTNESS_MAX+brightness],
                                         mt9d115_regs.brightness_exposure_tbl_sz[exposure*CAMERA_BRIGHTNESS_MAX+brightness]); 

    return rc;
}
/*
 * White Balance Setting
 */
static int32_t mt9d115_set_wb(int8_t wb_mode)
{
    int32_t rc = 0;

    CDBG("%s: entry: wb_mode=%d\n", __func__, wb_mode);

    switch (wb_mode)
    {
        case CAMERA_WB_MODE_AWB:
        {
            rc = mt9d115_i2c_write_table(mt9d115_regs.wb_auto_tbl, 
                                         mt9d115_regs.wb_auto_tbl_sz);
        }
        break;

        case CAMERA_WB_MODE_SUNLIGHT:
        {
            rc = mt9d115_i2c_write_table(mt9d115_regs.wb_daylight_tbl,
                                         mt9d115_regs.wb_daylight_tbl_sz);
        }
        break;

        case CAMERA_WB_MODE_INCANDESCENT:
        {
            rc = mt9d115_i2c_write_table(mt9d115_regs.wb_incandescent_tbl,
                                         mt9d115_regs.wb_incandescent_tbl_sz);
        }
        break;
        
        case CAMERA_WB_MODE_FLUORESCENT:
        {
            rc = mt9d115_i2c_write_table(mt9d115_regs.wb_flourescant_tbl,
                                         mt9d115_regs.wb_flourescant_tbl_sz);
        }
        break; 

        case CAMERA_WB_MODE_CLOUDY:
        {
            rc = mt9d115_i2c_write_table(mt9d115_regs.wb_cloudy_tbl,
                                         mt9d115_regs.wb_cloudy_tbl_sz);
        }
        break;

        default:
        {
            CCRT("%s: parameter error!\n", __func__);
            rc = -EFAULT;
        }     
    }

    /*
      * Attention
      *
      * Time delay of 100ms or more is required by sensor,
      *
      * WB config will have no effect after setting 
      * without time delay of 100ms or more
      */
    mdelay(100);

	return rc;
}    

static int32_t mt9d115_set_contrast(int8_t contrast)
{
    int32_t rc = 0;

    CDBG("%s: entry: contrast=%d\n", __func__, contrast);

    switch (contrast)
    {
        case CAMERA_CONTRAST_0:
        {
            rc = mt9d115_i2c_write_table(mt9d115_regs.contrast_tbl[0],
                                         mt9d115_regs.contrast_tbl_sz[0]);
        }
        break;

        case CAMERA_CONTRAST_1:
        {
            rc = mt9d115_i2c_write_table(mt9d115_regs.contrast_tbl[1],
                                         mt9d115_regs.contrast_tbl_sz[1]);
        }
        break;

        case CAMERA_CONTRAST_2:
        {
            rc = mt9d115_i2c_write_table(mt9d115_regs.contrast_tbl[2],
                                         mt9d115_regs.contrast_tbl_sz[2]);
        }
        break;
        
        case CAMERA_CONTRAST_3:
        {
            rc = mt9d115_i2c_write_table(mt9d115_regs.contrast_tbl[3],
                                         mt9d115_regs.contrast_tbl_sz[3]);
        }
        break; 

        case CAMERA_CONTRAST_4:
        {
            rc = mt9d115_i2c_write_table(mt9d115_regs.contrast_tbl[4],
                                         mt9d115_regs.contrast_tbl_sz[4]);
        }
        break;

        default:
        {
            CCRT("%s: parameter error!\n", __func__);
            rc = -EFAULT;
        }     
    }

    /*
      * Attention
      *
      * Time delay of 100ms or more is required by sensor,
      *
      * Contrast config will have no effect after setting 
      * without time delay of 100ms or more
      */
    mdelay(100);

	return rc;
}

static int32_t mt9d115_set_brightness(int8_t brightness)
{
    int32_t rc = 0;

    CCRT("%s: entry: brightness=%d\n", __func__, brightness);

    current_brightness = brightness;

    rc = mt9d115_set_exposure_brightness(current_exposure, 
        current_brightness);

	return rc;
}    

static int32_t mt9d115_set_saturation(int8_t saturation)
{
    int32_t rc = 0;

    CCRT("%s: entry: saturation=%d\n", __func__, saturation);

    switch (saturation)
    {
        case CAMERA_SATURATION_0:
        {
            rc = mt9d115_i2c_write_table(mt9d115_regs.saturation_tbl[0],
                                         mt9d115_regs.saturation_tbl_sz[0]);
        }
        break;

        case CAMERA_SATURATION_1:
        {
            rc = mt9d115_i2c_write_table(mt9d115_regs.saturation_tbl[1],
                                         mt9d115_regs.saturation_tbl_sz[1]);
        }
        break;

        case CAMERA_SATURATION_2:
        {
            rc = mt9d115_i2c_write_table(mt9d115_regs.saturation_tbl[2],
                                         mt9d115_regs.saturation_tbl_sz[2]);
        }
        break;
        
        case CAMERA_SATURATION_3:
        {
            rc = mt9d115_i2c_write_table(mt9d115_regs.saturation_tbl[3],
                                         mt9d115_regs.saturation_tbl_sz[3]);
        }
        break; 

        case CAMERA_SATURATION_4:
        {
            rc = mt9d115_i2c_write_table(mt9d115_regs.saturation_tbl[4],
                                         mt9d115_regs.saturation_tbl_sz[4]);
        }
        break;

        default:
        {
            CCRT("%s: parameter error!\n", __func__);
            rc = -EFAULT;
        }     
    }

    /*
      * Attention
      *
      * Time delay of 100ms or more is required by sensor,
      *
      * Saturation config will have no effect after setting 
      * without time delay of 100ms or more
      */
    mdelay(100);

	return rc;
}    

static int32_t mt9d115_set_sharpness(int8_t sharpness)
{
    int32_t rc = 0;

    CDBG("%s: entry: sharpness=%d\n", __func__, sharpness);

    switch (sharpness)
    {
        case CAMERA_SHARPNESS_0:
        {
            rc = mt9d115_i2c_write_table(mt9d115_regs.sharpness_tbl[0],
                                         mt9d115_regs.sharpness_tbl_sz[0]);
        }
        break;

        case CAMERA_SHARPNESS_1:
        {
            rc = mt9d115_i2c_write_table(mt9d115_regs.sharpness_tbl[1],
                                         mt9d115_regs.sharpness_tbl_sz[1]);
        }
        break;

        case CAMERA_SHARPNESS_2:
        {
            rc = mt9d115_i2c_write_table(mt9d115_regs.sharpness_tbl[2],
                                         mt9d115_regs.sharpness_tbl_sz[2]);
        }
        break;
        
        case CAMERA_SHARPNESS_3:
        {
            rc = mt9d115_i2c_write_table(mt9d115_regs.sharpness_tbl[3],
                                         mt9d115_regs.sharpness_tbl_sz[3]);
        }
        break; 

        case CAMERA_SHARPNESS_4:
        {
            rc = mt9d115_i2c_write_table(mt9d115_regs.sharpness_tbl[4],
                                         mt9d115_regs.sharpness_tbl_sz[4]);
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
static int32_t mt9d115_set_iso(int8_t iso_val)
{
    int32_t rc = 0;

    CDBG("%s: entry: iso_val=%d\n", __func__, iso_val);

    switch (iso_val)
    {
        case CAMERA_ISO_SET_AUTO:
        {
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C,  0xA20D, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0020, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA20E, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0090, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA103, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0006, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            
            mdelay(200);
            
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C,  0xA20D, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0020, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA20E, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0090, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA103, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0005, WORD_LEN);
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
             rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C,  0xA20D, WORD_LEN);
             if (rc < 0)
             {
                 return rc;
             }
             rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0020, WORD_LEN);
             if (rc < 0)
             {
                 return rc;
             }
             rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA20E, WORD_LEN);
             if (rc < 0)
             {
                 return rc;
             }
             rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0028, WORD_LEN);
             if (rc < 0)
             {
                 return rc;
             }
             rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA103, WORD_LEN);
             if (rc < 0)
             {
                 return rc;
             }
             rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0005, WORD_LEN);
             if (rc < 0)
             {
                 return rc;
             }    
        }
        break;

        case CAMERA_ISO_SET_200:
        {
             rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C,  0xA20D, WORD_LEN);
             if (rc < 0)
             {
                return rc;
             }
             rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0040, WORD_LEN);
             if (rc < 0)
             {
                return rc;
             }
             rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA20E, WORD_LEN);
             if (rc < 0)
             {
                return rc;
             }
             rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0048, WORD_LEN);
             if (rc < 0)
             {
                return rc;
             }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA103, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0005, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }    
        }
        break;

        case CAMERA_ISO_SET_400:
        {
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C,  0xA20D, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0050, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA20E, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0080, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA103, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0005, WORD_LEN);
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

    /*
      * Attention
      *
      * Time delay of 100ms or more is required by sensor,
      *
      * ISO config will have no effect after setting 
      * without time delay of 100ms or more
      */
    mdelay(100);

	return rc;
} 

/*
 * Antibanding Setting
 */
static int32_t  mt9d115_set_antibanding(int8_t antibanding)
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
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA118, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0002, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA11E, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0002, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA124, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0002, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }    
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA12A, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0002, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }  
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA404, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }  
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x00A0, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA103, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0005, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }            
        break;

        case CAMERA_ANTIBANDING_SET_50HZ:
        {
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA118, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0002, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA11E, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0002, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA124, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0002, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }    
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA12A, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0002, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }  
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA404, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }  
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x00E0, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA103, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0005, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;

        case CAMERA_ANTIBANDING_SET_AUTO:
        {
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA118, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0001, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA11E, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0001, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA124, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0000, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }    
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA12A, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0001, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }  
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA103, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0005, WORD_LEN);
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

    /*
      * Attention
      *
      * Time delay of 100ms or more is required by sensor,
      *
      * Antibanding config will have no effect after setting 
      * without time delay of 100ms or more
      */
    mdelay(100);

	return rc;
} 

/*
 * Len Shading Setting
 */
static int32_t __attribute__((unused))mt9d115_set_lensshading(int8_t lensshading)
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

    /*
      * Set lens shading value according to brightness level
      */
    rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098E, 0x3835, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }

    rc = mt9d115_i2c_read(mt9d115_client->addr, 0x0990, &brightness_lev, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }

    if (brightness_lev < 5)
    {
        rc = mt9d115_i2c_write_table(mt9d115_regs.lens_for_outdoor_tbl, mt9d115_regs.lens_for_outdoor_tbl_sz);
        if (rc < 0)
        {
            return rc;
        } 
    }
    else
    {
        rc = mt9d115_i2c_write_table(mt9d115_regs.lens_for_indoor_tbl, mt9d115_regs.lens_for_indoor_tbl_sz);
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

static long mt9d115_set_exposure_compensation(int8_t exposure)
{
    long rc = 0;

    CDBG("%s: entry: exposure=%d\n", __func__, exposure);

    current_exposure = exposure;

    rc = (int32_t)mt9d115_set_exposure_brightness(current_exposure, 
        current_brightness);

    return rc;
}

static long mt9d115_reg_init(void)
{
    long rc;

    CDBG("%s: entry\n", __func__);

    /* PLL Setup */
    //no code here

    /* Preview & Snapshot Setup */
    rc = mt9d115_i2c_write_table(mt9d115_regs.prevsnap_tbl, mt9d115_regs.prevsnap_tbl_sz);
    if (rc < 0)
    {
        return rc;
    }

    /*
      * Lens Shading Setting
      * Disused here
      */
    //mt9d115_set_lensshading(1);

    /*
     * Set Standby Control Register
     *
     * Enable Interrupt request
     * Exit Soft standby
     */
    rc = mt9d115_i2c_write(mt9d115_client->addr, REG_MT9D115_STANDBY_CONTROL, 0x0028, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }
    mdelay(100);

    /*
     * Enable refresh mode
     * i.e, set value of 0x0006 or 0x0005 to register 0xA103
     */
    rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA103, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }
    rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0006, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }

    mdelay(200);

    /*
     * Enable refresh mode
     * i.e, set value of 0x0006 or 0x0005 to register 0xA103
     */
    rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA103, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }
    rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0005, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }

    return 0;
}

static long mt9d115_set_sensor_mode(int32_t mode)
{
    long rc = 0;

    CDBG("%s: entry\n", __func__);

    switch (mode)
    {
        case SENSOR_PREVIEW_MODE:
        {
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA115, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }

            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0000, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }

            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA103, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }

            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0001, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }

            /*
               * To improve efficiency of switch between preview and snapshot mode,
               * decrease time delay from 200ms to 80ms
               *
               * Attention:
               * The process of time delay should be set after process of setting
               * manual mode for autofocus in order to avoid display exception during
               * sensor initialization.
               */
            mdelay(80);
        }
        break;

        case SENSOR_SNAPSHOT_MODE:
        {
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA115, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }

            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0002, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }

            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA103, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }

            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0002, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }

            /*
               * To improve efficiency of switch between preview and snapshot mode,
               * decrease time delay from 10ms to 0ms
               */
            //msleep(10);
        }
        break;

        default:
        {
            return -EFAULT;
        }
    }

    return 0;
}

static long mt9d115_set_effect(int32_t mode, int32_t effect)
{
    uint16_t __attribute__((unused)) reg_addr;
    uint16_t __attribute__((unused)) reg_val;
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
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0x2759, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x6440, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0x275B, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x6440, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA103, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0005, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }          
        }            
        break;

        case CAMERA_EFFECT_MONO:
        {
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0x2759, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x6441, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0x275B, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x6441, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA103, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0005, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }          
        }
        break;

        case CAMERA_EFFECT_NEGATIVE:
        {
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0x2759, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x6443, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0x275B, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x6443, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA103, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0005, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }          
        }
        break;

        case CAMERA_EFFECT_SOLARIZE:
        {
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0x2759, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x6444, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0x275B, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x6444, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA103, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0005, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }          
        }            
        break;

        case CAMERA_EFFECT_SEPIA:
        {
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0x2759, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x6442, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0x275B, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x6442, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098C, 0xA103, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0005, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098E, 0xE886, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x00D8, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098E, 0xEC85, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x001F, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }            
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098E, 0xEC86, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x00D8, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }        
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x098E, 0x8400, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0990, 0x0006, WORD_LEN);
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

                 rc = mt9d115_i2c_write(mt9d115_client->addr, 0xXXXX, reg_addr, WORD_LEN);
                 if (rc < 0)
                 {
                    return rc;
                 }
               */

            return -EFAULT;
        }
    }

    /*
      * Attention
      *
      * Time delay of 100ms or more is required by sensor,
      *
      * Effect config will have no effect after setting 
      * without time delay of 100ms or more
      */
    mdelay(100);

    return rc;
}

/*
 * Power-up Process
 */
static long mt9d115_power_up(void)
{
    CDBG("%s: not supported!\n", __func__);
    return 0;
}

/*
 * Power-down Process
 */
static long mt9d115_power_down(void)
{
    CDBG("%s: not supported!\n", __func__);
    return 0;
}

/*
 * Set lowest-power mode (SHUTDOWN mode)
 *
 * MT9D115_GPIO_SHUTDOWN_CTL: 0, to quit lowest-power mode, or
 *                            1, to enter lowest-power mode
 */
#if 0 /* Dummy */
static int mt9d115_power_shutdown(uint32_t on)
{
    int rc;

    CDBG("%s: entry\n", __func__);

    rc = gpio_request(MT9D115_GPIO_SHUTDOWN_CTL, "mt9d115");
    if (0 == rc)
    {
        /* ignore "rc" */
        rc = gpio_direction_output(MT9D115_GPIO_SHUTDOWN_CTL, on);

        /* time delay */
        mdelay(1);
    }

    gpio_free(MT9D115_GPIO_SHUTDOWN_CTL);

    return rc;
}
#endif

#if !defined(CONFIG_SENSOR_ADAPTER)
static int mt9d115_sensor_init_probe(const struct msm_camera_sensor_info *data)
{
    uint32_t switch_on;
    int rc = 0;

    CDBG("%s: entry\n", __func__);

    /* Disable Lowest-Power State */
#if 0 /* Dummy */
    switch_on = 0;
    rc = mt9d115_power_shutdown(switch_on);
    if (rc < 0)
    {
        CCRT("enter/quit lowest-power mode failed!\n");
        goto init_probe_fail;
    }
#endif

    /* Exit From Hard Standby */
    switch_on = 0;
    rc = mt9d115_hard_standby(data, switch_on);
    if (rc < 0)
    {
        CCRT("set standby failed!\n");
        goto init_probe_fail;
    }

    /* Enable Hard Reset */
    rc = mt9d115_hard_reset(data);
    if (rc < 0)
    {
        CCRT("hard reset failed!\n");
        goto init_probe_fail;
    }

    /* Read the Model ID of the sensor */
    model_id = 0x0000;
    rc = mt9d115_i2c_read(mt9d115_client->addr, REG_MT9D115_MODEL_ID, &model_id, WORD_LEN);
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
    if (model_id != MT9D115_MODEL_ID)
    {
        rc = -EFAULT;
        goto init_probe_fail;
    }

    /*
     * Enable PLL in order to read reg of REG_MT9D115_MODEL_ID_SUB successfully
     */
    rc = mt9d115_i2c_write_table(mt9d115_regs.pll_tbl, mt9d115_regs.pll_tbl_sz);
    if (rc < 0)
    {
        return rc;
    }

    /* Read the Model ID of the sensor */
    model_id = 0x0000;
    rc = mt9d115_i2c_read(mt9d115_client->addr, REG_MT9D115_MODEL_ID_SUB, &model_id, WORD_LEN);
    if (rc < 0)
    {
        goto init_probe_fail;
    }

    CDBG("%s: model_id_sub = 0x%x\n", __func__, model_id);

    /*
      * set sensor id for EM (Engineering Mode)
      */
#ifdef CONFIG_SENSOR_INFO
    msm_sensorinfo_set_sensor_id(model_id);
#else
    // do nothing here
#endif

    /* Check if it matches it with the value in Datasheet */
    if (model_id != MT9D115_MODEL_ID_SUB)
    {
        rc = -EFAULT;
        goto init_probe_fail;
    }


    /*
      * Sensor Registers Setting
      */
    rc = mt9d115_reg_init();
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
static int mt9d115_sensor_i2c_probe_on(void)
{
    int rc;
    struct i2c_board_info info;
    struct i2c_adapter *adapter;
    struct i2c_client *client;

    rc = mt9d115_i2c_add_driver();
    if (rc < 0)
    {
        CCRT("%s: add i2c driver failed!\n", __func__);
        return rc;
    }

    memset(&info, 0, sizeof(struct i2c_board_info));
    info.addr = MT9D115_SLAVE_WR_ADDR >> 1;
    strlcpy(info.type, MT9D115_I2C_BOARD_NAME, I2C_NAME_SIZE);

    adapter = i2c_get_adapter(MT9D115_I2C_BUS_ID);
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

    mt9d115_client = client;

    return 0;

i2c_probe_failed:
    mt9d115_i2c_del_driver();
    return -ENODEV;
}

static void mt9d115_sensor_i2c_probe_off(void)
{
    i2c_unregister_device(mt9d115_client);
    mt9d115_i2c_del_driver();
}

static int mt9d115_sensor_dev_probe(const struct msm_camera_sensor_info *pinfo)
{
    int rc;

    rc = msm_camera_power_backend(MSM_CAMERA_PWRUP_MODE);
    if (rc < 0)
    {
        CCRT("%s: camera_power_backend failed!\n", __func__);
        return rc;
    }

    /*
      * Sensor Clock Switch
      */
#if defined(CONFIG_MACH_R750) || defined(CONFIG_MACH_JOE)
    rc = msm_camera_clk_switch(pinfo, MT9D115_GPIO_SWITCH_CTL, MT9D115_GPIO_SWITCH_VAL);
    if (rc < 0)
    {
        CCRT("%s: camera_clk_switch failed!\n", __func__);
        return rc;;
    }
#else
    // Do nothing
#endif

    msm_camio_clk_rate_set(MT9D115_CAMIO_MCLK);
    mdelay(5);

    rc = mt9d115_hard_standby(pinfo, 0);
    if (rc < 0)
    {
        CCRT("set standby failed!\n");
        return rc;
    }

    rc = mt9d115_hard_reset(pinfo);
    if (rc < 0)
    {
        CCRT("hard reset failed!\n");
        return rc;
    }

    model_id = 0x0000;
    rc = mt9d115_i2c_read(mt9d115_client->addr, REG_MT9D115_MODEL_ID, &model_id, WORD_LEN);
    if (rc < 0)
    {
        return rc;
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

    if (model_id != MT9D115_MODEL_ID)
    {
        return -EFAULT;
    }

    /*
      * Enable PLL in order to read reg of REG_MT9D115_MODEL_ID_SUB successfully
      */
    rc = mt9d115_i2c_write_table(mt9d115_regs.pll_tbl, mt9d115_regs.pll_tbl_sz);
    if (rc < 0)
    {
        return rc;
    }

    model_id = 0x0000;
    rc = mt9d115_i2c_read(mt9d115_client->addr, REG_MT9D115_MODEL_ID_SUB, &model_id, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }

    CDBG("%s: model_id_sub = 0x%x\n", __func__, model_id);

    /*
      * set sensor id for EM (Engineering Mode)
      */
#ifdef CONFIG_SENSOR_INFO
	msm_sensorinfo_set_sensor_id(model_id);
#else
    // do nothing here
#endif

    if (model_id != MT9D115_MODEL_ID_SUB)
    {
        return -EFAULT;
    }

    return 0;
}
#endif

static int mt9d115_sensor_probe_init(const struct msm_camera_sensor_info *data)
{
    int rc;

    CDBG("%s: entry\n", __func__);

    if (!data || strcmp(data->sensor_name, "mt9d115"))
    {
        CCRT("%s: invalid parameters!\n", __func__);
        rc = -ENODEV;
        goto probe_init_fail;
    }

    mt9d115_ctrl = kzalloc(sizeof(struct mt9d115_ctrl_t), GFP_KERNEL);
    if (!mt9d115_ctrl)
    {
        CCRT("%s: kzalloc failed!\n", __func__);
        rc = -ENOMEM;
        goto probe_init_fail;
    }

    mt9d115_ctrl->sensordata = data;

#if !defined(CONFIG_SENSOR_ADAPTER)
    /*
      * Power VREG On for sensor initialization
      */
    rc = msm_camera_power_backend(MSM_CAMERA_PWRUP_MODE);
    if (rc < 0)
    {
        CCRT("%s: camera_power_backend failed!\n", __func__);
        goto probe_init_fail;
    }

    /*
     * Sensor Clock Switch
     */
#if defined(CONFIG_MACH_R750) || defined(CONFIG_MACH_JOE)
    rc = msm_camera_clk_switch(mt9d115_ctrl->sensordata, MT9D115_GPIO_SWITCH_CTL, MT9D115_GPIO_SWITCH_VAL);
    if (rc < 0)
    {
        CCRT("%s: camera_clk_switch failed!\n", __func__);
        goto probe_init_fail;
    }
#else
    // do nothing
#endif

    msm_camio_clk_rate_set(MT9D115_CAMIO_MCLK);
    mdelay(5);

    rc = mt9d115_sensor_init_probe(mt9d115_ctrl->sensordata);
    if (rc < 0)
    {
        CCRT("%s: sensor_init_probe failed!\n", __func__);
        goto probe_init_fail;
    }
#else
    rc = mt9d115_sensor_dev_probe(mt9d115_ctrl->sensordata);
    if (rc < 0)
    {
        CCRT("%s: mt9d115_sensor_dev_probe failed!\n", __func__);
        goto probe_init_fail;
    }

    /*
      * Sensor Register Setting
      */
    rc = mt9d115_reg_init();
    if (rc < 0)
    {
        CCRT("%s: mt9d115_reg_init failed!\n", __func__);
        goto probe_init_fail;
    }
#endif

    return 0;

probe_init_fail:
    /*
      * To shut sensor down
      * Ignore "rc"
      */
    msm_camera_power_backend(MSM_CAMERA_PWRDWN_MODE);
    if(mt9d115_ctrl)
    {
        kfree(mt9d115_ctrl);
    }
    return rc;
}

#ifdef MT9D115_SENSOR_PROBE_INIT
static int mt9d115_sensor_init(const struct msm_camera_sensor_info *data)
{
    uint32_t switch_on; 
    int rc;

    CDBG("%s: entry\n", __func__);

    if ((NULL == data)
        || strcmp(data->sensor_name, "mt9d115")
        || strcmp(mt9d115_ctrl->sensordata->sensor_name, "mt9d115"))
    {
        CCRT("%s: data is NULL, or sensor_name is not equal to mt9d115!\n", __func__);
        rc = -ENODEV;
        goto sensor_init_fail;
    }
    
    /*
      * To exit from standby mode
      */
    rc = msm_camera_power_backend(MSM_CAMERA_NORMAL_MODE);
    if (rc < 0)
    {
        CCRT("%s: camera_power_backend failed!\n", __func__);
        goto sensor_init_fail;
    }

    /* Input MCLK */
    msm_camio_clk_rate_set(MT9D115_CAMIO_MCLK);
    mdelay(5);

    msm_camio_camif_pad_reg_reset();
    /* time delay for resetting CAMIF's PAD */
    mdelay(10);

    /* Exit From Hard Standby */   
    switch_on = 0;
    rc = mt9d115_hard_standby(mt9d115_ctrl->sensordata, switch_on);
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
static int mt9d115_sensor_init(const struct msm_camera_sensor_info *data)
{
    int rc;

    rc = mt9d115_sensor_probe_init(data);

    return rc;
}
#endif /* define MT9D115_SENSOR_PROBE_INIT */

static int mt9d115_sensor_config(void __user *argp)
{
    struct sensor_cfg_data cfg_data;
    long rc = 0;

    CDBG("%s: entry\n", __func__);

    if (copy_from_user(&cfg_data, (void *)argp, sizeof(struct sensor_cfg_data)))
    {
        CCRT("%s: copy_from_user failed!\n", __func__);
        return -EFAULT;
    }

    /* down(&mt9d115_sem); */

    CDBG("%s: cfgtype = %d, mode = %d\n", __func__, cfg_data.cfgtype, cfg_data.mode);

    switch (cfg_data.cfgtype)
    {
        case CFG_SET_MODE:
        {
            rc = mt9d115_set_sensor_mode(cfg_data.mode);
        }
        break;

        case CFG_SET_EFFECT:
        {
            rc = mt9d115_set_effect(cfg_data.mode, cfg_data.cfg.effect);
        }
        break;

        case CFG_PWR_UP:
        {
            rc = mt9d115_power_up();
        }
        break;

        case CFG_PWR_DOWN:
        {
            rc = mt9d115_power_down();
        }
        break;

        case CFG_SET_WB:
        {
            rc = mt9d115_set_wb(cfg_data.cfg.wb_mode);
        }
        break;

        case CFG_SET_AF:
        {
            /*
               * Not supported by FF module
               * ignore "rc"
               */
            //rc = mt9d115_set_lensshading(1);
            //rc = mt9d115_af_trigger();
            rc = 0;
        }
        break;

        case CFG_SET_ISO:
        {
            rc = mt9d115_set_iso(cfg_data.cfg.iso_val);
        }
        break;

        case CFG_SET_ANTIBANDING:
        {
            rc = mt9d115_set_antibanding(cfg_data.cfg.antibanding);
        }
        break;

        case CFG_SET_BRIGHTNESS:
        {
            rc = mt9d115_set_brightness(cfg_data.cfg.brightness);
        }
        break;

        case CFG_SET_SATURATION:
        {
            rc = mt9d115_set_saturation(cfg_data.cfg.saturation);
        }
        break;

        case CFG_SET_CONTRAST:
        {
            rc = mt9d115_set_contrast(cfg_data.cfg.contrast);
        }
        break;

        case CFG_SET_SHARPNESS:
        {
            rc = mt9d115_set_sharpness(cfg_data.cfg.sharpness);
        }
        break;

        case CFG_SET_LENS_SHADING:
        {
            /*
               * no code here
               */
            //rc = mt9d115_set_lensshading(cfg_data.cfg.lensshading);
            rc = 0;
        }
        break;

        case CFG_SET_EXPOSURE_COMPENSATION:
        {
            rc = mt9d115_set_exposure_compensation(cfg_data.cfg.exposure);
        }
        break;

        default:
        {
            rc = -EFAULT;
        }
        break;
    }

    /*
     * Wake lock to prevent suspend
     */
    mt9d115_prevent_suspend();

    /* up(&mt9d115_sem); */

    return rc;
}

#ifdef MT9D115_SENSOR_PROBE_INIT
static int mt9d115_sensor_release_internal(void)
{
    int rc;
    uint32_t switch_on;

    CDBG("%s: entry\n", __func__);

    /*
      * Save values of other registers before enterring standby mode
      * default value: 0x0001: disable
      * current value: 0x0000: enable
      */
    rc = mt9d115_i2c_write(mt9d115_client->addr, 0x0028, 0x0000, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }
    mdelay(1);

    /*
      * mt9d115_ctrl's memory is allocated in mt9d115_sensor_probe_init
      * during system initialization in the condition of attachment of sensor hardware
      * otherwise, there will be NULL pointer for mt9d115_ctrl->sensordata
      */
    switch_on = 1;
    rc = mt9d115_hard_standby(mt9d115_ctrl->sensordata, switch_on);
    if (rc < 0)
    {
        return rc;
    }
    mdelay(200);

    /*
      * Enter into standby mode
      */
    rc = msm_camera_power_backend(MSM_CAMERA_STANDBY_MODE);
    if (rc < 0)
    {
        return rc;
    }

    /* up(&mt9d115_sem); */

    return 0;
}
#else
static int mt9d115_sensor_release_internal(void)
{
    int rc;

    /*
      * MCLK is disabled by 
      * msm_camio_clk_disable(CAMIO_VFE_CLK)
      * in msm_camio_disable
      */

    /*
      * To power VREG off
      * Ignore "rc"
      */
    rc = msm_camera_power_backend(MSM_CAMERA_PWRDWN_MODE);

    kfree(mt9d115_ctrl);

    return rc;
}
#endif /* mt9d115_sensor_release_internal */

static int mt9d115_sensor_release(void)
{
    int rc;

    rc = mt9d115_sensor_release_internal();

    mt9d115_allow_suspend();

    return rc;
}

static int mt9d115_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int rc = 0;

    CDBG("%s: entry\n", __func__);

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
    {
        rc = -ENOTSUPP;
        goto probe_failure;
    }

    mt9d115_sensorw = kzalloc(sizeof(struct mt9d115_work_t), GFP_KERNEL);
    if (!mt9d115_sensorw)
    {
        rc = -ENOMEM;
        goto probe_failure;
    }

    i2c_set_clientdata(client, mt9d115_sensorw);

    mt9d115_client = client;

    return 0;

probe_failure:
    kfree(mt9d115_sensorw);
    mt9d115_sensorw = NULL;
    CCRT("%s: rc = %d, failed!\n", __func__, rc);
    return rc;
}

static int __exit mt9d115_i2c_remove(struct i2c_client *client)
{
    struct mt9d115_work_t *sensorw = i2c_get_clientdata(client);

    CDBG("%s: entry\n", __func__);

    free_irq(client->irq, sensorw);   
    kfree(sensorw);

    /*
     * Wake lock to prevent suspend
     */
    mt9d115_deinit_suspend();

    mt9d115_client = NULL;
    mt9d115_sensorw = NULL;

    return 0;
}

static const struct i2c_device_id mt9d115_id[] = {
    { "mt9d115", 0},
    { },
};

static struct i2c_driver mt9d115_driver = {
    .id_table = mt9d115_id,
    .probe  = mt9d115_i2c_probe,
    .remove = __exit_p(mt9d115_i2c_remove),
    .driver = {
        .name = MT9D115_I2C_BOARD_NAME,
    },
};

static int32_t mt9d115_i2c_add_driver(void)
{
    int32_t rc = 0;

    rc = i2c_add_driver(&mt9d115_driver);
    if (IS_ERR_VALUE(rc))
    {
        goto init_failure;
    }

    return rc;

init_failure:
    CCRT("%s: rc = %d, failed!\n", __func__, rc);
    return rc;
}

static void mt9d115_i2c_del_driver(void)
{
    i2c_del_driver(&mt9d115_driver);
}

void mt9d115_exit(void)
{
    CDBG("%s: entry\n", __func__);
    mt9d115_i2c_del_driver();
}

int mt9d115_sensor_probe(const struct msm_camera_sensor_info *info,
                                struct msm_sensor_ctrl *s)
{
    int rc;

    CDBG("%s: entry\n", __func__);

#if !defined(CONFIG_SENSOR_ADAPTER)
    rc = mt9d115_i2c_add_driver();
    if (rc < 0)
    {
        goto probe_failed;
    }
#else
    // Do nothing
#endif

#ifdef MT9D115_SENSOR_PROBE_INIT
    rc = mt9d115_sensor_probe_init(info);
    if (rc < 0)
    {
        CCRT("%s: mt9d115_sensor_probe_init failed!\n", __func__);
        goto probe_failed;
    }

    /*
      * Enter Into Hard Standby
      */
    rc = mt9d115_sensor_release_internal();
    if (rc < 0)
    {
        CCRT("%s: mt9d115_sensor_release failed!\n", __func__);
        goto probe_failed;
    }
#endif /* define MT9D115_SENSOR_PROBE_INIT */

    /*
     * Wake lock to prevent suspend
     */
    mt9d115_init_suspend();

    s->s_init       = mt9d115_sensor_init;
    s->s_config     = mt9d115_sensor_config;
    s->s_release    = mt9d115_sensor_release;

    return 0;

probe_failed:
    CCRT("%s: rc = %d, failed!\n", __func__, rc);

#if !defined(CONFIG_SENSOR_ADAPTER)
    mt9d115_i2c_del_driver();
#else
    // Do nothing
#endif

    return rc;
}

#if defined(MT9D115_PROBE_WORKQUEUE)
/* To implement the parallel init process */
static void mt9d115_workqueue(struct work_struct *work)
{
    int32_t rc;

/*
 * Get FTM flag to adjust 
 * the initialize process 
 * of camera
 */
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
    rc = msm_camera_drv_start(pdev_wq, mt9d115_sensor_probe);
#else
    rc = msm_camera_dev_start(pdev_wq,
                              mt9d115_sensor_i2c_probe_on,
                              mt9d115_sensor_i2c_probe_off,
                              mt9d115_sensor_dev_probe);
    if (rc < 0)
    {
        CCRT("%s: msm_camera_dev_start failed!\n", __func__);
        goto probe_failed;
    }

    rc = msm_camera_drv_start(pdev_wq, mt9d115_sensor_probe);
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

static int32_t mt9d115_probe_workqueue(void)
{
    int32_t rc;

    mt9d115_wq = create_singlethread_workqueue("mt9d115_wq");

    if (!mt9d115_wq)
    {
        CCRT("%s: mt9d115_wq is NULL!\n", __func__);
        return -EFAULT;
    }

    /*
      * Ignore "rc"
      * "queue_work"'s rc:
      * 0: already in work queue
      * 1: added into work queue
      */   
    rc = queue_work(mt9d115_wq, &mt9d115_cb_work);

    return 0;
}

static int __mt9d115_probe(struct platform_device *pdev)
{
    int32_t rc;

    pdev_wq = pdev;

    rc = mt9d115_probe_workqueue();

    return rc;
}
#else
static int __mt9d115_probe(struct platform_device *pdev)
{
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

    return msm_camera_drv_start(pdev, mt9d115_sensor_probe);
}
#endif /* defined(MT9D115_PROBE_WORKQUEUE) */

static struct platform_driver msm_camera_driver = {
    .probe = __mt9d115_probe,
    .driver = {
        .name = "msm_camera_mt9d115",
        .owner = THIS_MODULE,
    },
};

static int __init mt9d115_init(void)
{
    return platform_driver_register(&msm_camera_driver);
}

module_init(mt9d115_init);

