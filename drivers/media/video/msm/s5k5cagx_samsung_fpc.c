/*
 * drivers/media/video/msm/s5k5cagx_samsung_fpc.c
 *
 * Refer to drivers/media/video/msm/mt9t11x_qtech_mcnex_fpc.c
 * For S5K5CAGX: 3.0Mp,Digital Image Sensor
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

#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <media/msm_camera.h>
#include <mach/gpio.h>
#include "s5k5cagx.h"
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
 */
//#undef S5K5CAGX_SENSOR_PROBE_INIT
#define S5K5CAGX_SENSOR_PROBE_INIT

#ifdef S5K5CAGX_SENSOR_PROBE_INIT
/*
 * To implement the parallel init process
 */
#define S5K5CAGX_PROBE_WORKQUEUE
#endif /* define S5K5CAGX_SENSOR_PROBE_INIT */

#if defined(S5K5CAGX_PROBE_WORKQUEUE)
#include <linux/workqueue.h>
static struct platform_device *pdev_wq = NULL;
static struct workqueue_struct *s5k5cagx_wq = NULL;
static void s5k5cagx_workqueue(struct work_struct *work);
static DECLARE_WORK(s5k5cagx_cb_work, s5k5cagx_workqueue);
#endif /* defined(S5K5CAGX_PROBE_WORKQUEUE) */

#define ENOINIT 100 /*have not power up,so don't need to power down*/

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
//#define S5K5CAGX_CAMIO_MCLK  48000000
#define S5K5CAGX_CAMIO_MCLK  24000000    //PCLK=48MHz, set by PLL setting

/*
 * Micron S5K5CAGX Registers and their values
 */
/* Sensor I2C Board Name */
#define S5K5CAGX_I2C_BOARD_NAME "s5k5cagx"

/* Sensor I2C Bus Number (Master I2C Controller: 0) */
#define S5K5CAGX_I2C_BUS_ID  (0)

/* Sensor I2C Slave Address */
#define S5K5CAGX_SLAVE_WR_ADDR 0x78 /* replaced by "msm_i2c_devices.addr" */
#define S5K5CAGX_SLAVE_RD_ADDR 0x79 /* replaced by "msm_i2c_devices.addr" */

/* Sensor I2C Device ID */
#define REG_S5K5CAGX_MODEL_ID   0x0F12

#define S5K5CAGX_MODEL_ID       0x05CA

//GYL S5K5CAGX SETTINGS BEGIN
/* SOC Registers */
#define REG_S5K5CAGX_RESET_AND_MISC_CTRL 0x001A
#define S5K5CAGX_SOC_RESET               0x0219  /* SOC is in soft reset */
#define S5K5CAGX_SOC_NOT_RESET           0x0218  /* SOC is not in soft reset */
#define S5K5CAGX_SOC_RESET_DELAY_MSECS   10

#define REG_S5K5CAGX_STANDBY_CONTROL     0x0018
#define S5K5CAGX_SOC_STANDBY             0x402C  /* SOC is in standby state */
//GYL S5K5CAGX SETTINGS END

//GYL S5K5CAGX SETTINGS BEGIN
/*
 * GPIO For Sensor Clock Switch
 */
#if defined(CONFIG_MACH_RAISE)
#define S5K5CAGX_GPIO_SWITCH_CTL     39
#define S5K5CAGX_GPIO_SWITCH_VAL     1
#elif defined(CONFIG_MACH_V9)
#define S5K5CAGX_GPIO_SWITCH_CTL     107
#define S5K5CAGX_GPIO_SWITCH_VAL     0
#else
#undef S5K5CAGX_GPIO_SWITCH_CTL
#undef S5K5CAGX_GPIO_SWITCH_VAL
#endif
//GYL S5K5CAGX SETTINGS END

/* 
 * GPIO For Lowest-Power mode (SHUTDOWN mode)
 * #define S5K5CAGX_GPIO_SHUTDOWN_CTL   Dummy
 */

/*-----------------------------------------------------------------------------------------
 *
 * TYPE DECLARATION
 *
 *----------------------------------------------------------------------------------------*/
struct s5k5cagx_work_t {
    struct work_struct work;
};

struct s5k5cagx_ctrl_t {
    const struct msm_camera_sensor_info *sensordata;
};

/*-----------------------------------------------------------------------------------------
 *
 * GLOBAL VARIABLE DEFINITION
 *
 *----------------------------------------------------------------------------------------*/
static struct s5k5cagx_work_t *s5k5cagx_sensorw = NULL;
static struct i2c_client *s5k5cagx_client = NULL;
static struct s5k5cagx_ctrl_t *s5k5cagx_ctrl = NULL;

/*
 * For coexistence of MT9T111 and MT9T112
 */
static uint16_t model_id;
static struct s5k5cagx_base_reg_t *s5k5cagx_base_regs = NULL;

DECLARE_MUTEX(s5k5cagx_sem);

static struct wake_lock s5k5cagx_wake_lock;

/*-----------------------------------------------------------------------------------------
 *
 * FUNCTION DECLARATION
 *
 *----------------------------------------------------------------------------------------*/
static int s5k5cagx_sensor_init(const struct msm_camera_sensor_info *data);
static int s5k5cagx_sensor_config(void __user *argp);
static int s5k5cagx_sensor_release(void);
static int s5k5cagx_sensor_release_internal(void);
static int32_t s5k5cagx_i2c_add_driver(void);
static void s5k5cagx_i2c_del_driver(void);
static int32_t s5k5cagx_i2c_read(unsigned short saddr, unsigned short raddr,
                                     unsigned short *rdata, enum s5k5cagx_width_t width);
static int32_t s5k5cagx_i2c_write(unsigned short saddr, unsigned short waddr,
                                      unsigned short wdata, enum s5k5cagx_width_t width);

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
static inline void s5k5cagx_init_suspend(void)
{
    CDBG("%s: entry\n", __func__);
    wake_lock_init(&s5k5cagx_wake_lock, WAKE_LOCK_IDLE, "s5k5cagx");
}

static inline void s5k5cagx_deinit_suspend(void)
{
    CDBG("%s: entry\n", __func__);
    wake_lock_destroy(&s5k5cagx_wake_lock);
}

static inline void s5k5cagx_prevent_suspend(void)
{
    CDBG("%s: entry\n", __func__);
    wake_lock(&s5k5cagx_wake_lock);
}

static inline void s5k5cagx_allow_suspend(void)
{
    CDBG("%s: entry\n", __func__);
    wake_unlock(&s5k5cagx_wake_lock);
}

/*
 * Hard standby of sensor
 * on: =1, enter into hard standby
 * on: =0, exit from hard standby
 *
 * Hard standby mode is set by register of REG_S5K5CAGX_STANDBY_CONTROL.
 */
static int s5k5cagx_hard_standby(const struct msm_camera_sensor_info *dev, uint32_t on)
{
    int rc;

    CDBG("%s: entry\n", __func__);

    if(on == 1)
    {

        rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0028, 0x7000, WORD_LEN);
        if(rc < 0)
        {
            return rc;
        }            
        rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x2824, WORD_LEN);
        if (rc < 0)
        {
            return rc;
        }
        rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0000, WORD_LEN);
        if (rc < 0)
        {
            return rc;
        }
        rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0028, 0x7000, WORD_LEN);
        if(rc < 0)
        {
            return rc;
        }            
        rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x0254, WORD_LEN);
        if (rc < 0)
        {
            return rc;
        }
        rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0001, WORD_LEN);
        if (rc < 0)
        {
            return rc;
        }
        rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x0252, WORD_LEN);
        if(rc < 0)
        {
            return rc;
        }            
        rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0004, WORD_LEN);
        if (rc < 0)
        {
            return rc;
        }
        msleep(133);
        rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x0252, WORD_LEN);
        if (rc < 0)
        {
            return rc;
        }
        rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0002, WORD_LEN);
        if (rc < 0)
        {
            return rc;
        }
        msleep(200);
    }

    rc = gpio_request(dev->sensor_pwd, "s5k5cagx");
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
static int s5k5cagx_hard_reset(const struct msm_camera_sensor_info *dev)
{
    int rc = 0;

    CDBG("%s: entry\n", __func__);

    rc = gpio_request(dev->sensor_reset, "s5k5cagx");
    if (0 == rc)
    {
        /* ignore "rc" */
        rc = gpio_direction_output(dev->sensor_reset, 1);

        mdelay(10);
    }

    gpio_free(dev->sensor_reset);

    return rc;
}

static int32_t s5k5cagx_i2c_txdata(unsigned short saddr,
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

    if (i2c_transfer(s5k5cagx_client->adapter, msg, 1) < 0)
    {
        CCRT("%s: failed!\n", __func__);
        return -EIO;
    }

    return 0;
}

static int32_t s5k5cagx_i2c_write(unsigned short saddr,
                                      unsigned short waddr,
                                      unsigned short wdata,
                                      enum s5k5cagx_width_t width)
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

            rc = s5k5cagx_i2c_txdata(saddr, buf, 4);
        }
        break;

        case BYTE_LEN:
        {
            buf[0] = waddr;
            buf[1] = wdata;

            rc = s5k5cagx_i2c_txdata(saddr, buf, 2);
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

static int32_t s5k5cagx_i2c_write_table(struct s5k5cagx_i2c_reg_conf const *reg_conf_tbl,
                                             int len)
{
    uint32_t i;
    int32_t rc = 0;

#ifdef S5K5CAGX_SENSOR_PROBE_INIT
    for (i = 0; i < len; i++)
    {
        rc = s5k5cagx_i2c_write(s5k5cagx_client->addr,
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
    if(reg_conf_tbl == s5k5cagx_base_regs->prevsnap_tbl)
    {
        for (i = 0; i < len; i++)
        {
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr,
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
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr,
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

static int s5k5cagx_i2c_rxdata(unsigned short saddr,
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

    if (i2c_transfer(s5k5cagx_client->adapter, msgs, 2) < 0)
    {
        CCRT("%s: failed!\n", __func__);
        return -EIO;
    }

    return 0;
}

static int32_t s5k5cagx_i2c_read(unsigned short saddr,
                                     unsigned short raddr,
                                     unsigned short *rdata,
                                     enum s5k5cagx_width_t width)
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

            rc = s5k5cagx_i2c_rxdata(saddr, buf, 2);
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
 * Auto Focus Trigger
 */
static int32_t s5k5cagx_af_trigger(void)
{
    int32_t rc = 0;
    uint16_t af_status = 0x0001;
    uint16_t af_2nd_status = 0x0100;    //added by JSK SEMCO
    uint32_t i = 0;
   // int retry = 0;

    CDBG("%s: entry\n", __func__);

    rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->af_tbl, s5k5cagx_base_regs->af_tbl_sz);
    if (rc < 0)
    {
        return rc;
    }
    mdelay(500);

    /*
      * af_status: 0, AF is done successfully
      *            1, AF is being done
      */
  
    for (i = 0; i < 50; ++i)
    {       
        mdelay(100);
        rc = s5k5cagx_i2c_write(s5k5cagx_client->addr,0xFCFC, 0xD000, WORD_LEN);
        if (rc < 0)
        {
         return rc;
        }
        rc = s5k5cagx_i2c_write(s5k5cagx_client->addr,0x002C, 0x7000, WORD_LEN);
        if (rc < 0)
        {
         return rc;
        }
        rc = s5k5cagx_i2c_write(s5k5cagx_client->addr,0x002E, 0x26FE, WORD_LEN);
        if (rc < 0)
        {
         return rc;
        }
        rc = s5k5cagx_i2c_read(s5k5cagx_client->addr, 0x0F12, &af_status, WORD_LEN);
        if (rc < 0)
        {
         return rc;
        }
  
        /*af_statue */			    
        /* 0x0001 : Searching */
        /* 0x0002 : AF Fail   */ 
        /* 0x0003 : AF Success*/ 
        if ((0x0002 == af_status)||(0x0003 == af_status))
        {
            break;
        }
    }

    if (0x0002 == af_status) //need 2nd Scan
    {
        //mdelay(100);
        for(i = 0; i < 50; ++i)
        {
            mdelay(100);
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr,0xFCFC, 0xD000, WORD_LEN);
            if (rc < 0)
            {
                  return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr,0x002C, 0x7000, WORD_LEN);
            if (rc < 0)
            {
                  return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr,0x002E, 0x1B2F, WORD_LEN);
            if (rc < 0)
            {
                  return rc;
            }
            rc = s5k5cagx_i2c_read(s5k5cagx_client->addr, 0x0F12, &af_2nd_status, WORD_LEN);
            if (rc < 0)
            {
                  return rc;
            }
            /*af_2nd_statue */       
            /* 0x01xx : 2nd scan not finish  */
            /* 0x00xx : 2nd scan finish */ 
            if(0x0000 == (af_2nd_status&0xFF00))
            {
                mdelay(100);
                return 0;   
            }                   
        }
    }
    else if(0x0003 == af_status) //don't need 2nd scan
    {
        mdelay(100);
        return 0;         
    }
    /*
      * AF is failed
      */
    return -EIO;
}

/*
 * White Balance Setting
 */
static int32_t s5k5cagx_set_wb(int8_t wb_mode)
{
    int32_t rc = 0;

    CDBG("%s: entry: wb_mode=%d\n", __func__, wb_mode);

    switch (wb_mode)
    {
        case CAMERA_WB_MODE_AWB:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->wb_auto_tbl, 
                                         s5k5cagx_base_regs->wb_auto_tbl_sz);
        }
        break;

        case CAMERA_WB_MODE_SUNLIGHT:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->wb_daylight_tbl,
                                         s5k5cagx_base_regs->wb_daylight_tbl_sz);
        }
        break;

        case CAMERA_WB_MODE_INCANDESCENT:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->wb_incandescent_tbl,
                                         s5k5cagx_base_regs->wb_incandescent_tbl_sz);
        }
        break;
        
        case CAMERA_WB_MODE_FLUORESCENT:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->wb_flourescant_tbl,
                                         s5k5cagx_base_regs->wb_flourescant_tbl_sz);
        }
        break; 

        case CAMERA_WB_MODE_CLOUDY:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->wb_cloudy_tbl,
                                         s5k5cagx_base_regs->wb_cloudy_tbl_sz);
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

static int32_t s5k5cagx_set_contrast(int8_t contrast)
{
    int32_t rc = 0;

    CDBG("%s: entry: contrast=%d\n", __func__, contrast);

    switch (contrast)
    {
        case CAMERA_CONTRAST_0:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->contrast_tbl[0],
                                         s5k5cagx_base_regs->contrast_tbl_sz[0]);
        }
        break;

        case CAMERA_CONTRAST_1:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->contrast_tbl[1],
                                         s5k5cagx_base_regs->contrast_tbl_sz[1]);
        }
        break;

        case CAMERA_CONTRAST_2:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->contrast_tbl[2],
                                         s5k5cagx_base_regs->contrast_tbl_sz[2]);
        }
        break;
        
        case CAMERA_CONTRAST_3:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->contrast_tbl[3],
                                         s5k5cagx_base_regs->contrast_tbl_sz[3]);
        }
        break; 

        case CAMERA_CONTRAST_4:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->contrast_tbl[4],
                                         s5k5cagx_base_regs->contrast_tbl_sz[4]);
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

static int32_t s5k5cagx_set_brightness(int8_t brightness)
{
    int32_t rc = 0;

    CCRT("%s: entry: brightness=%d\n", __func__, brightness);

    switch (brightness)
    {
        case CAMERA_BRIGHTNESS_0:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->brightness_tbl[0],
                                         s5k5cagx_base_regs->brightness_tbl_sz[0]);
        }
        break;

        case CAMERA_BRIGHTNESS_1:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->brightness_tbl[1],
                                         s5k5cagx_base_regs->brightness_tbl_sz[1]);
        }
        break;

        case CAMERA_BRIGHTNESS_2:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->brightness_tbl[2],
                                         s5k5cagx_base_regs->brightness_tbl_sz[2]);
        }
        break;
        
        case CAMERA_BRIGHTNESS_3:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->brightness_tbl[3],
                                         s5k5cagx_base_regs->brightness_tbl_sz[3]);
        }
        break; 

        case CAMERA_BRIGHTNESS_4:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->brightness_tbl[4],
                                         s5k5cagx_base_regs->brightness_tbl_sz[4]);
        }
        break;
        
        case CAMERA_BRIGHTNESS_5:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->brightness_tbl[5],
                                         s5k5cagx_base_regs->brightness_tbl_sz[5]);
        }
        break;
        
        case CAMERA_BRIGHTNESS_6:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->brightness_tbl[6],
                                         s5k5cagx_base_regs->brightness_tbl_sz[6]);
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

static int32_t s5k5cagx_set_saturation(int8_t saturation)
{
    int32_t rc = 0;

    CCRT("%s: entry: saturation=%d\n", __func__, saturation);

    switch (saturation)
    {
        case CAMERA_SATURATION_0:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->saturation_tbl[0],
                                         s5k5cagx_base_regs->saturation_tbl_sz[0]);
        }
        break;

        case CAMERA_SATURATION_1:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->saturation_tbl[1],
                                         s5k5cagx_base_regs->saturation_tbl_sz[1]);
        }
        break;

        case CAMERA_SATURATION_2:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->saturation_tbl[2],
                                         s5k5cagx_base_regs->saturation_tbl_sz[2]);
        }
        break;
        
        case CAMERA_SATURATION_3:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->saturation_tbl[3],
                                         s5k5cagx_base_regs->saturation_tbl_sz[3]);
        }
        break; 

        case CAMERA_SATURATION_4:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->saturation_tbl[4],
                                         s5k5cagx_base_regs->saturation_tbl_sz[4]);
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

static int32_t s5k5cagx_set_sharpness(int8_t sharpness)
{
    int32_t rc = 0;

    CDBG("%s: entry: sharpness=%d\n", __func__, sharpness);
     
    switch (sharpness)
    {
        case CAMERA_SHARPNESS_0:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->sharpness_tbl[0],
                                         s5k5cagx_base_regs->sharpness_tbl_sz[0]);
        }
        break;

        case CAMERA_SHARPNESS_1:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->sharpness_tbl[1],
                                         s5k5cagx_base_regs->sharpness_tbl_sz[1]);
        }
        break;

        case CAMERA_SHARPNESS_2:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->sharpness_tbl[2],
                                         s5k5cagx_base_regs->sharpness_tbl_sz[2]);
        }
        break;
        
        case CAMERA_SHARPNESS_3:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->sharpness_tbl[3],
                                         s5k5cagx_base_regs->sharpness_tbl_sz[3]);
        }
        break; 

        case CAMERA_SHARPNESS_4:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->sharpness_tbl[4],
                                         s5k5cagx_base_regs->sharpness_tbl_sz[4]);
        }
        break;        
        
        case CAMERA_SHARPNESS_5:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->sharpness_tbl[5],
                                         s5k5cagx_base_regs->sharpness_tbl_sz[5]);
        }
        break;        

        case CAMERA_SHARPNESS_6:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->sharpness_tbl[6],
                                         s5k5cagx_base_regs->sharpness_tbl_sz[6]);
        }
        break;        

        case CAMERA_SHARPNESS_7:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->sharpness_tbl[7],
                                         s5k5cagx_base_regs->sharpness_tbl_sz[7]);
        }
        break;        

        case CAMERA_SHARPNESS_8:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->sharpness_tbl[8],
                                         s5k5cagx_base_regs->sharpness_tbl_sz[8]);
        }
        break;        

        case CAMERA_SHARPNESS_9:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->sharpness_tbl[9],
                                         s5k5cagx_base_regs->sharpness_tbl_sz[9]);
        }
        break;        

        case CAMERA_SHARPNESS_10:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->sharpness_tbl[10],
                                         s5k5cagx_base_regs->sharpness_tbl_sz[10]);
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
static int32_t s5k5cagx_set_iso(int8_t iso_val)
{

    int32_t rc = 0;

    CDBG("%s: entry: iso_val=%d\n", __func__, iso_val);

    switch (iso_val)
    {
        case CAMERA_ISO_SET_AUTO:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->iso_tbl[0],
                                         s5k5cagx_base_regs->iso_tbl_sz[0]);
        }
        break;

        case CAMERA_ISO_SET_HJR:
        {
            //do nothing
        }
        break;

        case CAMERA_ISO_SET_100:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->iso_tbl[1],
                                         s5k5cagx_base_regs->iso_tbl_sz[1]);
        }
        break;

        case CAMERA_ISO_SET_200:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->iso_tbl[2],
                                         s5k5cagx_base_regs->iso_tbl_sz[2]);
        }
        break;

        case CAMERA_ISO_SET_400:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->iso_tbl[3],
                                         s5k5cagx_base_regs->iso_tbl_sz[3]);
        }
        break;

        case CAMERA_ISO_SET_800:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->iso_tbl[4],
                                         s5k5cagx_base_regs->iso_tbl_sz[4]);
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
static int32_t  s5k5cagx_set_antibanding(int8_t antibanding)
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
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0xFCFC, 0xD000, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0028, 0x7000, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x04D2, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x065F, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x04BA, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0002, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0001, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }            
        break;

        case CAMERA_ANTIBANDING_SET_50HZ:
        {
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0xFCFC, 0xD000, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0028, 0x7000, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x04D2, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x065F, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x04BA, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0001, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0001, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;

        case CAMERA_ANTIBANDING_SET_AUTO:
        {
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0xFCFC, 0xD000, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0028, 0x7000, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x04D2, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x067F, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x04BA, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0001, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0001, WORD_LEN);
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
 * Len Shading Setting
 */
static int32_t s5k5cagx_set_lensshading(int8_t lensshading)
{
    CDBG("%s: not supported!\n", __func__);
    return 0;
}

static int32_t s5k5cagx_set_exposure_compensation(int8_t exposure)
{
    int32_t rc = 0;

    CDBG("%s: entry: exposure=%d\n", __func__, exposure);

    switch (exposure)
    {
        case CAMERA_EXPOSURE_0:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->exposure_tbl[0],
                                         s5k5cagx_base_regs->exposure_tbl_sz[0]);
        }
        break;

        case CAMERA_EXPOSURE_1:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->exposure_tbl[1],
                                         s5k5cagx_base_regs->exposure_tbl_sz[1]);
        }
        break;

        case CAMERA_EXPOSURE_2:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->exposure_tbl[2],
                                         s5k5cagx_base_regs->exposure_tbl_sz[2]);
        }
        break;
        
        case CAMERA_EXPOSURE_3:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->exposure_tbl[3],
                                         s5k5cagx_base_regs->exposure_tbl_sz[3]);
        }
        break; 

        case CAMERA_EXPOSURE_4:
        {
            rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->exposure_tbl[4],
                                         s5k5cagx_base_regs->exposure_tbl_sz[4]);
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

static long s5k5cagx_reg_init(void)
{
    long rc;
    
    CDBG("%s: entry\n", __func__);

    /* PLL Setup */
    rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->pll_tbl, s5k5cagx_base_regs->pll_tbl_sz);
    if (rc < 0)
    {
        return rc;
    }

    /* Clock Setup */
    rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->clk_tbl, s5k5cagx_base_regs->clk_tbl_sz);
    if (rc < 0)
    {
        return rc;
    }

    msleep(10);

    /* Preview & Snapshot Setup */
    rc = s5k5cagx_i2c_write_table(s5k5cagx_base_regs->prevsnap_tbl, s5k5cagx_base_regs->prevsnap_tbl_sz);
    if (rc < 0)
    {
        return rc;
    }

    /*
      * Lens Shading Setting
      */
    s5k5cagx_set_lensshading(1);

    return 0;
}

static long s5k5cagx_set_sensor_mode(int32_t mode)
{
    long rc = 0;

    /*
    * In order to avoid reentry of SENSOR_PREVIEW_MODE by
    * Qualcomm Camera HAL, e.g. vfe_set_dimension/vfe_preview_config,
    * 
    * enter into SENSOR_PREVIEW_MODE only when s5k5cagx_previewmode_entry_cnt=0
    */
    static uint32_t s5k5cagx_previewmode_entry_cnt = 0;
    
    CDBG("%s: entry\n", __func__);

    switch (mode)
    {
        case SENSOR_PREVIEW_MODE:
        {
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0xFCFC, 0xD000, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0028, 0x7000, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x07E6, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0000, WORD_LEN);	//700007E6 //TVAR_afit_pBaseValS[17] //AFIT16_demsharpmix1_iHystThLow
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0000, WORD_LEN);	//700007E8 //TVAR_afit_pBaseValS[18] //AFIT16_demsharpmix1_iHystThHigh
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x08B6, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0000, WORD_LEN);	//700008B6 //TVAR_afit_pBaseValS[121] /AFIT16_demsharpmix1_iHystThLow
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0000, WORD_LEN);	//700008B8 //TVAR_afit_pBaseValS[122] /AFIT16_demsharpmix1_iHystThHigh
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x0986, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0000, WORD_LEN);	//70000986 //TVAR_afit_pBaseValS[225] /AFIT16_demsharpmix1_iHystThLow
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0000, WORD_LEN);	//70000988 //TVAR_afit_pBaseValS[226] /AFIT16_demsharpmix1_iHystThHigh
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x0A56, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0000, WORD_LEN);	//70000A56 //TVAR_afit_pBaseValS[329] /AFIT16_demsharpmix1_iHystThLow
                        if (rc < 0)
            {
                return rc;
            }            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0000, WORD_LEN);	//70000A58 //TVAR_afit_pBaseValS[330] /AFIT16_demsharpmix1_iHystThHigh
            
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x0838, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x003C, WORD_LEN);	//TVAR_afit_pBaseValS[58] //AFIT8_ccm_oscar_iSaturation[7:0],   AFIT8_RGB2YUV_iYOffset [15:8]
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x0908, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0046, WORD_LEN);	//TVAR_afit_pBaseValS[162] /AFIT8_ccm_oscar_iSaturation[7:0],   AFIT8_RGB2YUV_iYOffset [15:8]
            if (rc < 0)
            {
                return rc;
            }

            //HIGHLUX RECOVER Value
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002a, 0x0B2C, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x003C, WORD_LEN);	//70000B2C //TVAR_afit_pBaseValS[436] /AFIT16_YUV422_DENOISE_iUVLowThresh
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0050, WORD_LEN);	//70000B2E //TVAR_afit_pBaseValS[437] /AFIT16_YUV422_DENOISE_iUVHighThresh
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002a, 0x0B50, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x012D, WORD_LEN);	//70000B50 //TVAR_afit_pBaseValS[454] /AFIT8_sddd8a_iRadialLimit [7:0],   AFIT8_sddd8a_iRadialPower [15:8]
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002a, 0x0B84, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x210C, WORD_LEN);	//70000B84 //TVAR_afit_pBaseValS[480] /AFIT8_sddd8a_iDenThreshHigh[7:0],   AFIT8_Demosaicing_iEdgeDesat [15:8]
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002a, 0x0252, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0003, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x023C, WORD_LEN); //Normal Preview//
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0000, WORD_LEN); //config 0 //
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x0240, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0001, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x0230, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0001, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x023E, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0001, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x0220, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0001, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x0222, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0001, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            mdelay(80);

            /*
               * Enter Into SENSOR_PREVIEW_MODE
               * Only When s5k5cagx_previewmode_entry_cnt=0
               */
            s5k5cagx_previewmode_entry_cnt++;
        }
        break;

        case SENSOR_SNAPSHOT_MODE:
        {
            /*
               * Enter Into SENSOR_PREVIEW_MODE
               * Only When s5k5cagx_previewmode_entry_cnt=0
               */
            s5k5cagx_previewmode_entry_cnt = 0;

            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0028, 0x7000, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x035C, WORD_LEN);   //Normal capture //
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0000, WORD_LEN);   //REG_0TC_CCFG_uCaptureMode                 	
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0800, WORD_LEN);   //REG_0TC_CCFG_usWidth  //Insert here the Image Width
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0600, WORD_LEN);   //REG_0TC_CCFG_usHeight //Insert here the Image
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0xFCFC, 0xD000, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0028, 0x7000, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x07E6, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0020, WORD_LEN);//TVAR_afit_pBaseValS[17] //AFIT16_demsharpmix1_iHystThLow                                                                     
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0020, WORD_LEN);//TVAR_afit_pBaseValS[18] //AFIT16_demsharpmix1_iHystThHigh
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x08B6, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x000A, WORD_LEN);//700008B6 //TVAR_afit_pBaseValS[121] /AFIT16_demsharpmix1_iHystThLow
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0005, WORD_LEN);//700008B8 //TVAR_afit_pBaseValS[122] /AFIT16_demsharpmix1_iHystThHigh
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x0986, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0005, WORD_LEN);//70000986 //TVAR_afit_pBaseValS[225] /AFIT16_demsharpmix1_iHystThLow
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0005, WORD_LEN);//70000988 //TVAR_afit_pBaseValS[226] /AFIT16_demsharpmix1_iHystThHigh
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x0A56, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0005, WORD_LEN);//70000A56 //TVAR_afit_pBaseValS[329] /AFIT16_demsharpmix1_iHystThLow
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0005, WORD_LEN);//70000A58 //TVAR_afit_pBaseValS[330] /AFIT16_demsharpmix1_iHystThHigh
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x0244, WORD_LEN);//Normal capture//
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0000, WORD_LEN);//config 0 //
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x0230, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0001, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x0246, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0001, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x0224, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }            
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0001, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0001, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            msleep(300);
        }
        break;

        default:
        {
            return -EFAULT;
        }
    }

    return 0;
}

static long s5k5cagx_set_effect(int32_t mode, int32_t effect)
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
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0xFCFC, 0xD000, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0028, 0x7000, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x3286, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0001, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x021E, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0000, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }          
        }            
        break;

        case CAMERA_EFFECT_MONO:
        {
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0xFCFC, 0xD000, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0028, 0x7000, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x021E, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0001, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;

        case CAMERA_EFFECT_NEGATIVE:
        {
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0xFCFC, 0xD000, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0028, 0x7000, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A,  0x021E, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0003, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;

        case CAMERA_EFFECT_SOLARIZE:
        {
            //do nothing because no solarize mode
        }            
        break;

        case CAMERA_EFFECT_SEPIA:
        {
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0xFCFC, 0xD000, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0028, 0x7000, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x021E, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0004, WORD_LEN);
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

                 rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0xXXXX, reg_addr, WORD_LEN);
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
 * Power-up Process
 */
static long s5k5cagx_power_up(void)
{
    CDBG("%s: not supported!\n", __func__);
    return 0;
}

/*
 * Power-down Process
 */
static long s5k5cagx_power_down(void)
{
    CDBG("%s: not supported!\n", __func__);
    return 0;
}

/*
 * Set lowest-power mode (SHUTDOWN mode)
 *
 * S5K5CAGX_GPIO_SHUTDOWN_CTL: 0, to quit lowest-power mode, or
 *                            1, to enter lowest-power mode
 */
#if 0 /* Dummy */
static int s5k5cagx_power_shutdown(uint32_t on)
{
    int rc;

    CDBG("%s: entry\n", __func__);

    rc = gpio_request(S5K5CAGX_GPIO_SHUTDOWN_CTL, "s5k5cagx");
    if (0 == rc)
    {
        /* ignore "rc" */
        rc = gpio_direction_output(S5K5CAGX_GPIO_SHUTDOWN_CTL, on);

        /* time delay */
        mdelay(1);
    }

    gpio_free(S5K5CAGX_GPIO_SHUTDOWN_CTL);

    return rc;
}
#endif

#if !defined(CONFIG_SENSOR_ADAPTER)
static int s5k5cagx_sensor_init_probe(const struct msm_camera_sensor_info *data)
{
    uint32_t switch_on;
    int rc = 0;

    /* Exit From Hard Standby */
    switch_on = 0;
    rc = s5k5cagx_hard_standby(data, switch_on);
    if (rc < 0)
    {
        CCRT("set standby failed!\n");
        goto init_probe_fail;
    }

    /* Enable Hard Reset */
    rc = s5k5cagx_hard_reset(data);
    if (rc < 0)
    {
        CCRT("hard reset failed!\n");
        goto init_probe_fail;
    }

    /* Read the Model ID of the sensor */
    model_id = 0x0000;
    rc = s5k5cagx_i2c_read(s5k5cagx_client->addr, REG_S5K5CAGX_MODEL_ID, &model_id, WORD_LEN);
    if (rc < 0)
    {
        goto init_probe_fail;
    }

    pr_err("%s: model_id = 0x%x\n", __func__, model_id);

    /* Check if it matches it with the value in Datasheet */
	
    /*
      * set sensor id for EM (Engineering Mode)
      */
#ifdef CONFIG_SENSOR_INFO
	msm_sensorinfo_set_sensor_id(model_id);
#else
    // do nothing here
#endif
    pr_err("1 model_id = 0x%x\n",  model_id);

	if (model_id == S5K5CAGX_MODEL_ID)
    {
        s5k5cagx_base_regs = &s5k5cagx_regs;
    }
    else
    {
        s5k5cagx_base_regs = NULL;
        rc = -EFAULT;
        goto init_probe_fail;
    }

    /* 
    * Soft reset of sensor "s5k5cagx"
    * RESET the sensor image part via I2C command 
    * REG_S5K5CAGX_SENSOR_RESET: 0x0219
    * bit9: Parallel output is enabled
    * bit8: GPIO[2] does not function as OE
    * bit7: reserved
    * bit6: reserved
    * bit5: EXTCLK is not gated off during standby
    * bit4: GPIO signals are turned off during standby
    * bit3: VGPIO signals are turned off during standby
    * bit2: reserved
    * bit1: MIPI transmitter is not in reset
    * bit0: SOC is in reset
    */
    rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 
                           REG_S5K5CAGX_RESET_AND_MISC_CTRL, 
                           S5K5CAGX_SOC_RESET,
                           WORD_LEN);
    if (rc < 0)
    {
        CCRT("soft reset failed!\n");
        goto init_probe_fail;
    }

    mdelay(1);

    /*
    * Soft reset of sensor "s5k5cagx"
    * soc_cfg: 0x0018
    * bit9: Parallel output is disabled
    * bit8: GPIO[2] does not function as OE
    * bit7: reserved
    * bit6: reserved
    * bit5: EXTCLK is not gated off during standby
    * bit4: GPIO signals are turned off during standby
    * bit3: VGPIO signals are turned off during standby
    * bit2: reserved
    * bit1: MIPI transmitter is not in reset
    * bit0: SOC is not in reset
    */
    rc = s5k5cagx_i2c_write(s5k5cagx_client->addr,
                           REG_S5K5CAGX_RESET_AND_MISC_CTRL,
                           S5K5CAGX_SOC_NOT_RESET & 0x0018,
                           WORD_LEN);
    if (rc < 0)
    {
        CCRT("soft reset failed!\n");
        goto init_probe_fail;
    }

    /*
      * Soft reset of sensor "s5k5cagx"
      * S5K5CAGX_SOC_NOT_RESET: 0x0218
      * bit9: Parallel output is enabled
      * bit8: GPIO[2] does not function as OE
      * bit7: reserved
      * bit6: reserved
      * bit5: EXTCLK is not gated off during standby
      * bit4: GPIO signals are turned off during standby
      * bit3: VGPIO signals are turned off during standby
      * bit2: reserved
      * bit1: MIPI transmitter is not in reset
      * bit0: SOC is not in reset
      */
    rc = s5k5cagx_i2c_write(s5k5cagx_client->addr,
                           REG_S5K5CAGX_RESET_AND_MISC_CTRL,
                           S5K5CAGX_SOC_NOT_RESET,
                           WORD_LEN);
    if (rc < 0)
    {
        CCRT("soft reset failed!\n");
        goto init_probe_fail;
    }

    mdelay(S5K5CAGX_SOC_RESET_DELAY_MSECS);

    /*
      * Sensor Registers Setting
      */
    rc = s5k5cagx_reg_init();
    if (rc < 0)
    {
        goto init_probe_fail;
    }

    /*
     * Set Standby Control Register
     *
     * Enable Interrupt request
     * Exit Soft standby
     */
    rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, REG_S5K5CAGX_STANDBY_CONTROL, 0x0028, WORD_LEN);
    if (rc < 0)
    {
        goto init_probe_fail;
    }
    mdelay(100);

    return rc;

init_probe_fail:
    CCRT("%s: rc = %d, failed!\n", __func__, rc);
    return rc;
}
#else
static int s5k5cagx_sensor_i2c_probe_on(void)
{
    int rc;
    struct i2c_board_info info;
    struct i2c_adapter *adapter;
    struct i2c_client *client;

    rc = s5k5cagx_i2c_add_driver();
    if (rc < 0)
    {
        CCRT("%s: add i2c driver failed!\n", __func__);
        return rc;
    }

    memset(&info, 0, sizeof(struct i2c_board_info));
    info.addr = S5K5CAGX_SLAVE_WR_ADDR >> 1;
    strlcpy(info.type, S5K5CAGX_I2C_BOARD_NAME, I2C_NAME_SIZE);

    adapter = i2c_get_adapter(S5K5CAGX_I2C_BUS_ID);
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

    s5k5cagx_client = client;

    return 0;

i2c_probe_failed:
    s5k5cagx_i2c_del_driver();
    return -ENODEV;
}

static void s5k5cagx_sensor_i2c_probe_off(void)
{
    i2c_unregister_device(s5k5cagx_client);
    s5k5cagx_i2c_del_driver();
}

static int s5k5cagx_sensor_dev_probe(const struct msm_camera_sensor_info *pinfo)
{
    int rc;

    rc = msm_camera_power_backend(MSM_CAMERA_PWRUP_MODE);
    if (rc < 0)
    {
        return rc;
    }

    /*
    * Sensor Clock Switch
    */
#if defined(CONFIG_MACH_RAISE) || defined(CONFIG_MACH_V9)
    rc = msm_camera_clk_switch(pinfo, S5K5CAGX_GPIO_SWITCH_CTL, S5K5CAGX_GPIO_SWITCH_VAL);
    if (rc < 0)
    {
        CCRT("%s: camera_clk_switch failed!\n", __func__);
        return rc;;
    }
#else
    // Do nothing
#endif

    msm_camio_clk_rate_set(S5K5CAGX_CAMIO_MCLK);
    mdelay(5);

    rc = s5k5cagx_hard_standby(pinfo, 0);
    if (rc < 0)
    {
        return rc;
    }

    mdelay(15);

    rc = s5k5cagx_hard_reset(pinfo);
    if (rc < 0)
    {
        return rc;
    }

	mdelay(10);

    model_id = 0x0000;
    rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002C, 0x0000, WORD_LEN);    
    if (rc < 0)
    {
        return rc;    
    }
    rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002E, 0x0040, WORD_LEN);    
    if (rc < 0)
    {
        return rc;    
    }  
    rc = s5k5cagx_i2c_read(s5k5cagx_client->addr, REG_S5K5CAGX_MODEL_ID, &model_id, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }

    /*
    * set sensor id for EM (Engineering Mode)
    */
#ifdef CONFIG_SENSOR_INFO
	msm_sensorinfo_set_sensor_id(model_id);
#else
    // do nothing here
#endif

    if (model_id == S5K5CAGX_MODEL_ID)
    {
        s5k5cagx_base_regs = &s5k5cagx_regs;
    }
    else
    {
        s5k5cagx_base_regs = NULL;
        return -EFAULT;
    }

    return 0;
}
#endif

static int s5k5cagx_sensor_probe_init(const struct msm_camera_sensor_info *data)
{
    int rc;

    CDBG("%s: entry\n", __func__);

    if (!data || strcmp(data->sensor_name, "s5k5cagx"))
    {
        CCRT("%s: invalid parameters!\n", __func__);
        rc = -ENODEV;
        goto probe_init_fail;
    }

    s5k5cagx_ctrl = kzalloc(sizeof(struct s5k5cagx_ctrl_t), GFP_KERNEL);
    if (!s5k5cagx_ctrl)
    {
        CCRT("%s: kzalloc failed!\n", __func__);
        rc = -ENOMEM;
        goto probe_init_fail;
    }

    s5k5cagx_ctrl->sensordata = data;

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
#if defined(CONFIG_MACH_RAISE) || defined(CONFIG_MACH_V9)
    rc = msm_camera_clk_switch(s5k5cagx_ctrl->sensordata, S5K5CAGX_GPIO_SWITCH_CTL, S5K5CAGX_GPIO_SWITCH_VAL);
    if (rc < 0)
    {
        CCRT("%s: camera_clk_switch failed!\n", __func__);
        goto probe_init_fail;
    }
#else
    // do nothing
#endif

    msm_camio_clk_rate_set(S5K5CAGX_CAMIO_MCLK);
    mdelay(5);

    rc = s5k5cagx_sensor_init_probe(s5k5cagx_ctrl->sensordata);
    if (rc < 0)
    {
        CCRT("%s: sensor_init_probe failed!\n", __func__);
        goto probe_init_fail;
    }
#else
    rc = s5k5cagx_sensor_dev_probe(s5k5cagx_ctrl->sensordata);
    if (rc < 0)
    {
        CCRT("%s: s5k5cagx_sensor_dev_probe failed!\n", __func__);
        goto probe_init_fail;
    }

    /*
    * Sensor Register Setting
    */
    rc = s5k5cagx_reg_init();
    if (rc < 0)
    {
        CCRT("%s: s5k5cagx_reg_init failed!\n", __func__);
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
    if(s5k5cagx_ctrl)
    {
        kfree(s5k5cagx_ctrl);
    }
    return rc;
}

#ifdef S5K5CAGX_SENSOR_PROBE_INIT
static int s5k5cagx_sensor_init(const struct msm_camera_sensor_info *data)
{
    uint32_t switch_on; 
    int rc;

    CDBG("%s: entry\n", __func__);

    if ((NULL == data)
        || strcmp(data->sensor_name, "s5k5cagx")
        || strcmp(s5k5cagx_ctrl->sensordata->sensor_name, "s5k5cagx"))
    {
        CCRT("%s: data is NULL, or sensor_name is not equal to s5k5cagx!\n", __func__);
        rc = -ENODEV;
        goto sensor_init_fail;
    }

    /* Input MCLK */
    msm_camio_clk_rate_set(S5K5CAGX_CAMIO_MCLK);
    mdelay(5);

    msm_camio_camif_pad_reg_reset();
    /* time delay for resetting CAMIF's PAD */
    mdelay(10);

    /* Exit From Hard Standby */   
    switch_on = 0;
    rc = s5k5cagx_hard_standby(s5k5cagx_ctrl->sensordata, switch_on);
    if (rc < 0)
    {
        CCRT("set standby failed!\n");
        goto sensor_init_fail;
    }
    mdelay(10);
    

    rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0028, 0x7000, WORD_LEN);
    if(rc < 0)
    {
        return rc;
    }            
    rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x2824, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }
    rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0001, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }
    rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0028, 0x7000, WORD_LEN);
    if(rc < 0)
    {
        return rc;
    }            
    rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x0252, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }
    rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0003, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }
    rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0028, 0x7000, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }
    rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x0254, WORD_LEN);
    if(rc < 0)
    {
        return rc;
    }  
    rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0000, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }
    msleep(133);
    rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x002A, 0x0252, WORD_LEN);
    if(rc < 0)
    {
        return rc;
    }  
    rc = s5k5cagx_i2c_write(s5k5cagx_client->addr, 0x0F12, 0x0002, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }


    return 0;

sensor_init_fail:
    return rc;
}
#else
static int s5k5cagx_sensor_init(const struct msm_camera_sensor_info *data)
{
    int rc;

    rc = s5k5cagx_sensor_probe_init(data);

    return rc;
}
#endif /* define S5K5CAGX_SENSOR_PROBE_INIT */

static int s5k5cagx_sensor_config(void __user *argp)
{
    struct sensor_cfg_data cfg_data;
    long rc = 0;

    CDBG("%s: entry\n", __func__);

    if (copy_from_user(&cfg_data, (void *)argp, sizeof(struct sensor_cfg_data)))
    {
        CCRT("%s: copy_from_user failed!\n", __func__);
        return -EFAULT;
    }

    /* down(&s5k5cagx_sem); */

    CDBG("%s: cfgtype = %d, mode = %d\n", __func__, cfg_data.cfgtype, cfg_data.mode);

    switch (cfg_data.cfgtype)
    {
        case CFG_SET_MODE:
        {
            rc = s5k5cagx_set_sensor_mode(cfg_data.mode);
        }
        break;

        case CFG_SET_EFFECT:
        {
            rc = s5k5cagx_set_effect(cfg_data.mode, cfg_data.cfg.effect);
        }
        break;

        case CFG_PWR_UP:
        {
            rc = s5k5cagx_power_up();
        }
        break;

        case CFG_PWR_DOWN:
        {
            rc = s5k5cagx_power_down();
        }
        break;

        case CFG_SET_WB:
        {
            rc = s5k5cagx_set_wb(cfg_data.cfg.wb_mode);
        }
        break;

        case CFG_SET_AF:
        {
            /*
           * ignore "rc"
           */
            rc = s5k5cagx_set_lensshading(0);//wangtao
            rc = s5k5cagx_af_trigger();
        }
        break;

        case CFG_SET_ISO:
        {
            rc = s5k5cagx_set_iso(cfg_data.cfg.iso_val);
        }
        break;

        case CFG_SET_ANTIBANDING:
        {
            rc = s5k5cagx_set_antibanding(cfg_data.cfg.antibanding);
        }
        break;

        case CFG_SET_BRIGHTNESS:
        {
            rc = s5k5cagx_set_brightness(cfg_data.cfg.brightness);
        }
        break;

        case CFG_SET_SATURATION:
        {
            rc = s5k5cagx_set_saturation(cfg_data.cfg.saturation);
        }
        break;

        case CFG_SET_CONTRAST:
        {
            rc = s5k5cagx_set_contrast(cfg_data.cfg.contrast);
        }
        break;

        case CFG_SET_SHARPNESS:
        {
            rc = s5k5cagx_set_sharpness(cfg_data.cfg.sharpness);
        }
        break;

        case CFG_SET_LENS_SHADING:
        {
            /*
           * no code here
           */
            //rc = s5k5cagx_set_lensshading(cfg_data.cfg.lensshading);
            rc = 0;
        }
        break;

        case CFG_SET_EXPOSURE_COMPENSATION:
        {
            rc = s5k5cagx_set_exposure_compensation(cfg_data.cfg.exposure);
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
    s5k5cagx_prevent_suspend();

    /* up(&s5k5cagx_sem); */

    return rc;
}

#ifdef S5K5CAGX_SENSOR_PROBE_INIT
static int s5k5cagx_sensor_release_internal(void)
{
    int rc;
    uint32_t switch_on;

    CDBG("%s: entry\n", __func__);

    /*
    * s5k5cagx_ctrl's memory is allocated in s5k5cagx_sensor_probe_init
    * during system initialization in the condition of attachment of sensor hardware
    * otherwise, there will be NULL pointer for s5k5cagx_ctrl->sensordata
    */
    switch_on = 1;
    rc = s5k5cagx_hard_standby(s5k5cagx_ctrl->sensordata, switch_on);
    if (rc < 0)
    {
        return rc;
    }
    mdelay(200);
    /* up(&s5k5cagx_sem); */

    return 0;
}
#else
static int s5k5cagx_sensor_release_internal(void)
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

    kfree(s5k5cagx_ctrl);

    return rc;
}
#endif /* s5k5cagx_sensor_release_internal */

static int s5k5cagx_sensor_release(void)
{
    int rc;

    rc = s5k5cagx_sensor_release_internal();

    s5k5cagx_allow_suspend();

    return rc;
}

static int s5k5cagx_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int rc = 0;

    CDBG("%s: entry\n", __func__);

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
    {
        rc = -ENOTSUPP;
        goto probe_failure;
    }

    s5k5cagx_sensorw = kzalloc(sizeof(struct s5k5cagx_work_t), GFP_KERNEL);
    if (!s5k5cagx_sensorw)
    {
        rc = -ENOMEM;
        goto probe_failure;
    }

    i2c_set_clientdata(client, s5k5cagx_sensorw);

    s5k5cagx_client = client;

    return 0;

probe_failure:
    kfree(s5k5cagx_sensorw);
    s5k5cagx_sensorw = NULL;
    CCRT("%s: rc = %d, failed!\n", __func__, rc);
    return rc;
}

static int __exit s5k5cagx_i2c_remove(struct i2c_client *client)
{
    struct s5k5cagx_work_t *sensorw = i2c_get_clientdata(client);

    CDBG("%s: entry\n", __func__);

    free_irq(client->irq, sensorw);   
    kfree(sensorw);

    /*
    * Wake lock to prevent suspend
    */
    s5k5cagx_deinit_suspend();

    s5k5cagx_client = NULL;
    s5k5cagx_sensorw = NULL;

    return 0;
}

static const struct i2c_device_id s5k5cagx_id[] = {
    { "s5k5cagx", 0},
    { },
};

static struct i2c_driver s5k5cagx_driver = {
    .id_table = s5k5cagx_id,
    .probe  = s5k5cagx_i2c_probe,
    .remove = __exit_p(s5k5cagx_i2c_remove),
    .driver = {
        .name = S5K5CAGX_I2C_BOARD_NAME,
    },
};

static int32_t s5k5cagx_i2c_add_driver(void)
{
    int32_t rc = 0;

    rc = i2c_add_driver(&s5k5cagx_driver);
    if (IS_ERR_VALUE(rc))
    {
        goto init_failure;
    }

    return rc;

init_failure:
    CCRT("%s: rc = %d, failed!\n", __func__, rc);
    return rc;
}

static void s5k5cagx_i2c_del_driver(void)
{
    i2c_del_driver(&s5k5cagx_driver);
}

void s5k5cagx_exit(void)
{
    CDBG("%s: entry\n", __func__);
    s5k5cagx_i2c_del_driver();
}

int s5k5cagx_sensor_probe(const struct msm_camera_sensor_info *info,
                                struct msm_sensor_ctrl *s)
{
    int rc;

    CDBG("%s: entry\n", __func__);

#if !defined(CONFIG_SENSOR_ADAPTER)
    rc = s5k5cagx_i2c_add_driver();
    if (rc < 0)
    {
        goto probe_failed;
    }
#else
    // Do nothing
#endif

#ifdef S5K5CAGX_SENSOR_PROBE_INIT
    rc = s5k5cagx_sensor_probe_init(info);
    if (rc < 0)
    {
        CCRT("%s: s5k5cagx_sensor_probe_init failed!\n", __func__);
        goto probe_failed;
    }

    /*
    * Enter Into Hard Standby
    */
    rc = s5k5cagx_sensor_release_internal();
    if (rc < 0)
    {
        CCRT("%s: s5k5cagx_sensor_release failed!\n", __func__);
        goto probe_failed;
    }
#endif /* define S5K5CAGX_SENSOR_PROBE_INIT */

    /*
    * Wake lock to prevent suspend
    */
    s5k5cagx_init_suspend();


    s->s_mount_angle = 0;
    s->s_camera_type = BACK_CAMERA_2D;

    s->s_init       = s5k5cagx_sensor_init;
    s->s_config     = s5k5cagx_sensor_config;
    s->s_release    = s5k5cagx_sensor_release;

    return 0;

probe_failed:
    CCRT("%s: rc = %d, failed!\n", __func__, rc);

#if !defined(CONFIG_SENSOR_ADAPTER)
    s5k5cagx_i2c_del_driver();
#else
    // Do nothing
#endif

    return rc;
}

#if defined(S5K5CAGX_PROBE_WORKQUEUE)
/* To implement the parallel init process */
static void s5k5cagx_workqueue(struct work_struct *work)
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
    rc = msm_camera_drv_start(pdev_wq, s5k5cagx_sensor_probe);
#else
    rc = msm_camera_dev_start(pdev_wq,
                              s5k5cagx_sensor_i2c_probe_on,
                              s5k5cagx_sensor_i2c_probe_off,
                              s5k5cagx_sensor_dev_probe);
    if (rc < 0)
    {
        CCRT("%s: msm_camera_dev_start failed!\n", __func__);
        goto probe_failed;
    }

    rc = msm_camera_drv_start(pdev_wq, s5k5cagx_sensor_probe);
    if (rc < 0)
    {
        goto probe_failed;
    }

    return;

probe_failed:
    CCRT("%s: rc = %d, failed!\n", __func__, rc);

    if(rc != -ENOINIT){
	 	pr_err("%s: rc != -ENOINIT\n", __func__);
    	msm_camera_power_backend(MSM_CAMERA_PWRDWN_MODE);
    }
    //msm_camera_power_backend(MSM_CAMERA_PWRDWN_MODE);
    return;
#endif
}

static int32_t s5k5cagx_probe_workqueue(void)
{
    int32_t rc;

    s5k5cagx_wq = create_singlethread_workqueue("s5k5cagx_wq");

    if (!s5k5cagx_wq)
    {
        CCRT("%s: s5k5cagx_wq is NULL!\n", __func__);
        return -EFAULT;
    }

    /*
    * Ignore "rc"
    * "queue_work"'s rc:
    * 0: already in work queue
    * 1: added into work queue
    */  
    rc = queue_work(s5k5cagx_wq, &s5k5cagx_cb_work);

    return 0;
}

static int __s5k5cagx_probe(struct platform_device *pdev)
{
    int32_t rc;

    pdev_wq = pdev;

    rc = s5k5cagx_probe_workqueue();

    return rc;
}
#else
static int __s5k5cagx_probe(struct platform_device *pdev)
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

    return msm_camera_drv_start(pdev, s5k5cagx_sensor_probe);
}
#endif /* defined(S5K5CAGX_PROBE_WORKQUEUE) */

static struct platform_driver msm_camera_driver = {
    .probe = __s5k5cagx_probe,
    .driver = {
        .name = "msm_camera_s5k5cagx",
        .owner = THIS_MODULE,
    },
};

static int __init s5k5cagx_init(void)
{
    return platform_driver_register(&msm_camera_driver);
}

module_init(s5k5cagx_init);

