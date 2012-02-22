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
  --------     ----         -------------------------------------    ---------------------
  2011-07-28   wangtao      add settings                             ZTE_CAM_WT_20100728
  2011-04-16   lijing       add param setting                        ZTE_CAM_LJ_20110416
  2011-04-14   lijing       modify sensor angle from 90 to 180, else ZTE_CAM_LJ_20110414_01
                            snapshot angle is 90 rotated
  2009-10-24   jia.jia      Merged from kernel-v4515                 ZTE_MSM_CAMERA_JIA_001
------------------------------------------------------------------------------------------*/

#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <media/msm_camera.h>
#include <mach/gpio.h>
#include "ovm7690.h"
#include <linux/slab.h>

/*
 * Micron MT9V113 Registers and their values
 */
/* Sensor I2C Slave Address */
#define OVM7690_SLAVE_WR_ADDR 0x60 /* replaced by "msm_i2c_devices.addr" */
#define OVM7690_SLAVE_RD_ADDR 0x61

/* Sensor I2C Device ID */
#define REG_OVM7690_MODEL_ID    0x04
#define OVM7690_MODEL_ID        0x7691

/* SOC Registers */
//#define REG_MT9V113_SENSOR_RESET     0x001A
//#define REG_MT9V113_STANDBY_CONTROL  0x0018
//#define REG_MT9V113_MCU_BOOT         0x001C

/* CAMIO Input MCLK (MHz) */
#define OVM7690_CAMIO_MCLK  24000000 /*48000000*/

/* GPIO For Sensor Clock Switch */
#define OVM7690_GPIO_SWITCH_CTL     107

#define OVM7690_GPIO_SWITCH_VAL     1


struct ovm7690_work_t {
    struct work_struct work;
};

static struct ovm7690_work_t *ovm7690_sensorw;
static struct i2c_client *ovm7690_client;

struct ovm7690_ctrl_t {
    const struct msm_camera_sensor_info *sensordata;
};

static struct ovm7690_ctrl_t *ovm7690_ctrl;

static DECLARE_WAIT_QUEUE_HEAD(ovm7690_wait_queue);
DECLARE_MUTEX(ovm7690_sem);

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
static int __attribute__((unused)) ovm7690_hard_standby(const struct msm_camera_sensor_info *dev, uint32_t on)
{
    int rc;
    
    pr_err("%s: entry\n", __func__);
    
    rc = gpio_request(dev->sensor_pwd, "ovm7690");
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
static int ovm7690_hard_reset(const struct msm_camera_sensor_info *dev)
{
    int rc = 0;
    
    pr_err("%s: entry\n", __func__);

    rc = gpio_request(dev->sensor_reset, "ovm7690");
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
static int32_t ovm7690_i2c_txdata(unsigned short saddr,
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
    
    if (i2c_transfer(ovm7690_client->adapter, msg, 1) < 0)
    {
        CCRT("%s: failed!\n", __func__);
        return -EIO;
    }

    return 0;
}

static int32_t ovm7690_i2c_write(unsigned short saddr,
                                      unsigned short waddr,
                                      unsigned short wdata,
                                      enum ovm7690_width_t width)
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

            rc = ovm7690_i2c_txdata(saddr, buf, 4);
        }
        break;

        case BYTE_LEN:
        {
            buf[0] = waddr;
            buf[1] = wdata;

            rc = ovm7690_i2c_txdata(saddr, buf, 2);
        }
        break;

        default:
        {
        }
        break;
    }

    if (rc < 0)
    {
        pr_err("%s: saddr = 0x%x, waddr = 0x%x, wdata = 0x%x, failed!\n", __func__, saddr,waddr, wdata);
    }else
    {
    		//pr_err("%s: saddr = 0x%x, waddr = 0x%x, wdata = 0x%x, ok!\n", __func__, saddr,waddr, wdata);
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
static int ovm7690_i2c_rxdata(unsigned short saddr,
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
    
    if (i2c_transfer(ovm7690_client->adapter, msgs, 2)  <  0)
    {
        CCRT("%s: failed!\n", __func__);
        return -EIO;
    }

    return 0;
}


static int32_t ovm7690_i2c_read(unsigned short saddr,
                                     unsigned short raddr,
                                     unsigned short *rdata,
                                     enum ovm7690_width_t width)
{
    int32_t rc = 0;
    unsigned char buf[4];
    pr_err("wt %s ",__func__);
    
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

            rc = ovm7690_i2c_rxdata(saddr, buf, 2);
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

            rc = ovm7690_i2c_rxdata(saddr, buf, 1);
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

static long ovm7690_set_sensor_mode(int32_t mode)
{
    return 0;
}

static long ovm7690_set_effect(int32_t mode, int32_t effect)
{
    long rc = 0;
 
    pr_err("%s model = %d effect =%d",__func__,mode ,effect);


    switch (effect)
    {

        case CAMERA_EFFECT_OFF:
        {
        	  pr_err("effect =%d CAMERA_EFFECT_OFF",effect);
        	 
            
            
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xbe,0x26,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xbf,0x7b,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xc0,0xac,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
             rc = ovm7690_i2c_write(ovm7690_client->addr,0xbb,0x80,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
             rc = ovm7690_i2c_write(ovm7690_client->addr,0xbc,0x62,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xbd,0x1e,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xc1,0x1e,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
             rc = ovm7690_i2c_write(ovm7690_client->addr,0x28,0x00,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
      
        }            
        break;

        case CAMERA_EFFECT_MONO:
        {   pr_err("effect =%d CAMERA_EFFECT_MONO",effect);

           
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xbe,0x00,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xbf,0x00,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xc0,0x00,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
             rc = ovm7690_i2c_write(ovm7690_client->addr,0xbb,0x00,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
             rc = ovm7690_i2c_write(ovm7690_client->addr,0xbc,0x00,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xbd,0x00,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xc1,0x00,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
             rc = ovm7690_i2c_write(ovm7690_client->addr,0x28,0x00,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            } 
        }
        break;

        case CAMERA_EFFECT_NEGATIVE:
        {
        	pr_err("effect =%d CAMERA_EFFECT_NEGATIVE",effect);
 
          
             rc = ovm7690_i2c_write(ovm7690_client->addr,0xbe,0x26,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xbf,0x7b,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xc0,0xac,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
             rc = ovm7690_i2c_write(ovm7690_client->addr,0xbb,0x80,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
             rc = ovm7690_i2c_write(ovm7690_client->addr,0xbc,0x62,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xbd,0x1e,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xc1,0x1e,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
             rc = ovm7690_i2c_write(ovm7690_client->addr,0x28,0x80,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }     	
        }
        break;

        case CAMERA_EFFECT_SOLARIZE:
        {
           pr_err("effect =%d CAMERA_EFFECT_SOLARIZE not surport ",effect);        
        }            
        break;

        case CAMERA_EFFECT_SEPIA:
        {
        	 pr_err("effect =%d CAMERA_EFFECT_SEPIA",effect);
            
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xbe,0x26,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xbf,0x7b,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xc0,0x10,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
             rc = ovm7690_i2c_write(ovm7690_client->addr,0xbb,0x90,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
             rc = ovm7690_i2c_write(ovm7690_client->addr,0xbc,0x28,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xbd,0x1e,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xc1,0x1e,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
             rc = ovm7690_i2c_write(ovm7690_client->addr,0x28,0x00,BYTE_LEN);
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
    pr_err("test effect ");
            mdelay(20);

    return rc;
}


/*
 * White Balance Setting
 */
static int32_t ovm7690_set_wb(int8_t wb_mode)
{
    int32_t rc = 0;
		
    pr_err("%s: entry: wb_mode=%d\n", __func__, wb_mode);

    switch (wb_mode)
    {
        case CAMERA_WB_MODE_AWB:
        {
        	rc = ovm7690_i2c_write(ovm7690_client->addr,0x13,0xf7,BYTE_LEN); //AWB on
          if (rc < 0)
          {
              return rc;
          }
            
        }
        break;

        case CAMERA_WB_MODE_SUNLIGHT:
        {
          rc = ovm7690_i2c_write(ovm7690_client->addr,0x13,0xf5,BYTE_LEN); //AWB off
          if (rc < 0)
          {
              return rc;
          }
          rc = ovm7690_i2c_write(ovm7690_client->addr,0x01,0x5a,BYTE_LEN);
          if (rc < 0)
          {
              return rc;
          }
          rc = ovm7690_i2c_write(ovm7690_client->addr,0x02,0x5c,BYTE_LEN);
          if (rc < 0)
          {
              return rc;
          }  
          
        }
        break;

        case CAMERA_WB_MODE_INCANDESCENT:
        {
          rc = ovm7690_i2c_write(ovm7690_client->addr,0x13,0xf5,BYTE_LEN); //AWB off
          if (rc < 0)
          {
              return rc;
          }
          rc = ovm7690_i2c_write(ovm7690_client->addr,0x01,0x96,BYTE_LEN);
          if (rc < 0)
          {
              return rc;
          }
          rc = ovm7690_i2c_write(ovm7690_client->addr,0x02,0x40,BYTE_LEN);
          if (rc < 0)
          {
              return rc;
          }    
        }
        break;
        
        case CAMERA_WB_MODE_FLUORESCENT:
        {
        	 rc = ovm7690_i2c_write(ovm7690_client->addr,0x13,0xf5,BYTE_LEN); //AWB off
          if (rc < 0)
          {
              return rc;
          }
          rc = ovm7690_i2c_write(ovm7690_client->addr,0x01,0x84,BYTE_LEN);
          if (rc < 0)
          {
              return rc;
          }
          rc = ovm7690_i2c_write(ovm7690_client->addr,0x02,0x4c,BYTE_LEN);
          if (rc < 0)
          {
              return rc;
          }    
            
        }
        break; 

        case CAMERA_WB_MODE_CLOUDY:
        {
        	rc = ovm7690_i2c_write(ovm7690_client->addr,0x13,0xf5,BYTE_LEN); //AWB off
          if (rc < 0)
          {
              return rc;
          }
          rc = ovm7690_i2c_write(ovm7690_client->addr,0x01,0x58,BYTE_LEN);
          if (rc < 0)
          {
              return rc;
          }
          rc = ovm7690_i2c_write(ovm7690_client->addr,0x02,0x60,BYTE_LEN);
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

static int32_t ovm7690_set_contrast(int8_t contrast)
{
    int32_t rc = 0;
		uint16_t temp=0;
		
    pr_err("lijing:%s: entry: contrast=%d\n", __func__, contrast);

          
    switch (contrast)
    {
           
            
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xd2,0x07,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }         	
        case CAMERA_CONTRAST_0:
        {

            
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xd5,0x20,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xd4,0x20,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xd3,0x00,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            
            temp=0;
            rc = ovm7690_i2c_read(ovm7690_client->addr, 0xdc, &temp, BYTE_LEN);
				    if (rc < 0)
				    {
				        return rc;
				    }
				    temp |= 0x04;	    
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xdc,temp,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            
            
            
        }
        break;

        case CAMERA_CONTRAST_1:
        {

            rc = ovm7690_i2c_write(ovm7690_client->addr,0xd5,0x20,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xd4,0x24,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xd3,0x00,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            
            
            temp=0;
            rc = ovm7690_i2c_read(ovm7690_client->addr, 0xdc, &temp, BYTE_LEN);
				    if (rc < 0)
				    {
				        return rc;
				    }
				    temp |= 0x04;	    
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xdc,temp,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
           
        }
        break;

        case CAMERA_CONTRAST_2:
        {

            
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xd5,0x20,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xd4,0x28,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xd3,0x00,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            
            temp=0;
            rc = ovm7690_i2c_read(ovm7690_client->addr, 0xdc, &temp, BYTE_LEN);
				    if (rc < 0)
				    {
				        return rc;
				    }
				    temp &= 0xfb;	    
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xdc,temp,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            
        }
        break;
        
        case CAMERA_CONTRAST_3:
        {

            rc = ovm7690_i2c_write(ovm7690_client->addr,0xd5,0x20,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xd4,0x2c,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xd3,0x00,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            
            temp=0;
            rc = ovm7690_i2c_read(ovm7690_client->addr, 0xdc, &temp, BYTE_LEN);
				    if (rc < 0)
				    {
				        return rc;
				    }
				    temp &= 0xfb;	    
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xdc,temp,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break; 

        case CAMERA_CONTRAST_4:
        {

            
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xd5,0x20,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xd4,0x30,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xd3,0x00,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            
            temp=0;
            rc = ovm7690_i2c_read(ovm7690_client->addr, 0xdc, &temp, BYTE_LEN);
				    if (rc < 0)
				    {
				        return rc;
				    }
				    temp &= 0xfb;	    
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xdc,temp,BYTE_LEN);
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


static int32_t ovm7690_set_exposure_compensation(int8_t exposure)
{
    int32_t rc = 0;

    pr_err("lijing:%s: entry: exposure=%d\n", __func__, exposure);

    switch (exposure)
    {
        case CAMERA_EXPOSURE_0:
        {
          	rc = ovm7690_i2c_write(ovm7690_client->addr,0x24,0x38,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0x25,0x28,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0x26,0x81,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }   
            
        }
        break;

        case CAMERA_EXPOSURE_1:
        {
           	rc = ovm7690_i2c_write(ovm7690_client->addr,0x24,0x58,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0x25,0x48,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0x26,0xa2,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }   
        }
        break;

        case CAMERA_EXPOSURE_2:
        {
          	rc = ovm7690_i2c_write(ovm7690_client->addr,0x24,0x88,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0x25,0x78,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0x26,0xd3,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }     
        }
        break;
        
        case CAMERA_EXPOSURE_3:
        {
        		rc = ovm7690_i2c_write(ovm7690_client->addr,0x24,0xa8,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0x25,0x98,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0x26,0xd4,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }   
           
        }
        break; 

        case CAMERA_EXPOSURE_4:
        {
         		rc = ovm7690_i2c_write(ovm7690_client->addr,0x24,0xc8,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0x25,0xb8,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0x26,0xf5,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }   
        }
        break;

        default:
        {
            pr_err("%s: parameter error!\n", __func__);
            rc = -EFAULT;
        }     
    }

    
    return rc;
}  


static int32_t ovm7690_set_brightness(int8_t brightness)
{
    int32_t rc = 0;
		uint16_t temp = 0;
		
    pr_err("%s: entry: brightness=%d\n", __func__, brightness);


    switch (brightness)
    {

    	
        case CAMERA_BRIGHTNESS_0: //->-4
        {  

          
          rc = ovm7690_i2c_write(ovm7690_client->addr,0xd3,0x40,BYTE_LEN);
          if (rc < 0)
          {
              return rc;
          }
          
          
          temp=0;
          rc = ovm7690_i2c_read(ovm7690_client->addr, 0xdc, &temp, BYTE_LEN);
			    if (rc < 0)
			    {
			        return rc;
			    }
			    temp |= 0x08;	    
          rc = ovm7690_i2c_write(ovm7690_client->addr,0xdc,temp,BYTE_LEN);
          if (rc < 0)
          {
              return rc;
          }
          
          pr_err("brightness =%d reg 0xd3 value =0x%0x",brightness,temp);
            
        }
        break;

        case CAMERA_BRIGHTNESS_1: //->-2
        {
          
          rc = ovm7690_i2c_write(ovm7690_client->addr,0xd3,0x20,BYTE_LEN);
          if (rc < 0)
          {
              return rc;
          }
         
          rc = ovm7690_i2c_write(ovm7690_client->addr,0xdc,0x09,BYTE_LEN);
          if (rc < 0)
          {
              return rc;
          }          
          pr_err("brightness =%d reg 0xd3 value =0x%0x",brightness,temp);  
        }
        break;

        case CAMERA_BRIGHTNESS_2: //-> 0
        {
          
          rc = ovm7690_i2c_write(ovm7690_client->addr,0xd3,0x00,BYTE_LEN);
          if (rc < 0)
          {
              return rc;
          }         

           
          rc = ovm7690_i2c_write(ovm7690_client->addr,0xdc,0x09,BYTE_LEN);
          if (rc < 0)
          {
              return rc;
          }      
          pr_err("brightness =%d reg 0xd3 value =0x%0x",brightness,temp);  
        }
        break;
        
        case CAMERA_BRIGHTNESS_3:
        {
          rc = ovm7690_i2c_write(ovm7690_client->addr,0xd3,0x10,BYTE_LEN);
          if (rc < 0)
          {
              return rc;
          }         

         
          rc = ovm7690_i2c_write(ovm7690_client->addr,0xdc,0x01,BYTE_LEN);
          if (rc < 0)
          {
              return rc;
          }      
          pr_err("brightness =%d reg 0xd3 value =0x%0x",brightness,temp);  
        }
        break; 

        case CAMERA_BRIGHTNESS_4:
        {
          rc = ovm7690_i2c_write(ovm7690_client->addr,0xd3,0x20,BYTE_LEN);
          if (rc < 0)
          {
              return rc;
          }        

          
           
          rc = ovm7690_i2c_write(ovm7690_client->addr,0xdc,0x01,BYTE_LEN);
          if (rc < 0)
          {
              return rc;
          }  
          pr_err("brightness =%d reg 0xd3 value =0x%0x",brightness,temp); 
        }
        break;
        
        case CAMERA_BRIGHTNESS_5:
        {
          
          rc = ovm7690_i2c_write(ovm7690_client->addr,0xd3,0x30,BYTE_LEN);
          if (rc < 0)
          {
              return rc;
          }
                    
          
          rc = ovm7690_i2c_write(ovm7690_client->addr,0xdc,0x01,BYTE_LEN);
          if (rc < 0)
          {
              return rc;
          }  
          pr_err("brightness =%d reg 0xd3 value =0x%0x",brightness,temp); 
        }
        break;
        
        case CAMERA_BRIGHTNESS_6:
        {

          rc = ovm7690_i2c_write(ovm7690_client->addr,0xd3,0x40,BYTE_LEN);
          if (rc < 0)
          {
              return rc;
          }
                    
          
          rc = ovm7690_i2c_write(ovm7690_client->addr,0xdc,0x01,BYTE_LEN);
          if (rc < 0)
          {
              return rc;
          }  
          pr_err("brightness =%d reg 0xd3 value =0x%0x",brightness,temp);  
        }        
        break;

        default:
        {
            CCRT("%s: parameter error!\n", __func__);
            rc = -EFAULT;
        }     
    }
            mdelay(20);


	return rc;
}    

static int32_t ovm7690_set_saturation(int8_t saturation)
{
    int32_t rc = 0;
		//uint16_t temp=0;
		
    pr_err("lijing:%s: entry: saturation=%d\n", __func__, saturation);
			
    switch (saturation)
    {

            rc = ovm7690_i2c_write(ovm7690_client->addr,0xd2,0x07,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        case CAMERA_SATURATION_0: //-> - 4
        {
        	

            
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xD8,0x00,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xD9,0x00,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            
            
        }
        break;

        case CAMERA_SATURATION_1: // -> - 2
        {

            rc = ovm7690_i2c_write(ovm7690_client->addr,0xD8,0x20,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xD9,0x20,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
           
        }
        break;

        case CAMERA_SATURATION_2: // -> 0
        {

            
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xD8,0x40,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xD9,0x40,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }

            
        }
        break;
        
        case CAMERA_SATURATION_3: // ->2
        {

            
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xD8,0x60,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xD9,0x60,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }

        }
        break; 

        case CAMERA_SATURATION_4: //->4
        {

            
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xD8,0x80,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xD9,0x80,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }

        }
        break;

        default:
        {
            pr_err("%s: parameter error!\n", __func__);
            rc = -EFAULT;
        }     
    }

    return rc;
}    

static int32_t ovm7690_set_sharpness(int8_t sharpness)
{
    int32_t rc = 0;
    //uint16_t brightness_lev = 0;

    pr_err("lijing:%s: entry: sharpness=%d\n", __func__, sharpness);
    switch (sharpness)
    {
        case CAMERA_SHARPNESS_0:
        {
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xb4,0x26,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xb6,0x00,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }

           
            
        }
        break;

        case CAMERA_SHARPNESS_1:
        {
        	  rc = ovm7690_i2c_write(ovm7690_client->addr,0xb4,0x26,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xb6,0x02,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
          
        }
        break;

        case CAMERA_SHARPNESS_2:
        {
        		rc = ovm7690_i2c_write(ovm7690_client->addr,0xb4,0x26,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xb6,0x04,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
           
        }
        break;
        
        case CAMERA_SHARPNESS_3:
        {
        		rc = ovm7690_i2c_write(ovm7690_client->addr,0xb4,0x26,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xb6,0x06,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break; 

        case CAMERA_SHARPNESS_4:
        {
           	rc = ovm7690_i2c_write(ovm7690_client->addr,0xb4,0x26,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0xb6,0x08,BYTE_LEN);
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
static int32_t ovm7690_set_iso(int8_t iso_val)
{
    int32_t rc = 0;

    pr_err("lijing:%s: entry: iso_val=%d\n", __func__, iso_val);

    switch (iso_val)
    {
        case CAMERA_ISO_SET_AUTO:
        {
            rc = ovm7690_i2c_write(ovm7690_client->addr,0x14,0x21,BYTE_LEN);
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
        	  rc = ovm7690_i2c_write(ovm7690_client->addr,0x14,0x01,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
           
        }
        break;

        case CAMERA_ISO_SET_200:
        {
            rc = ovm7690_i2c_write(ovm7690_client->addr,0x14,0x11,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;

        case CAMERA_ISO_SET_400:
        {
            rc = ovm7690_i2c_write(ovm7690_client->addr,0x14,0x21,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;

        case CAMERA_ISO_SET_800:
        {
            rc = ovm7690_i2c_write(ovm7690_client->addr,0x14,0x31,BYTE_LEN);
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
static int32_t  ovm7690_set_antibanding(int8_t antibanding)
{
    int32_t rc = 0;
        

    
    pr_err("lijing:%s: entry: antibanding=%d\n", __func__, antibanding);

    switch (antibanding)
    {
        case CAMERA_ANTIBANDING_SET_OFF:
        {
            CCRT("%s: CAMERA_ANTIBANDING_SET_OFF NOT supported!\n", __func__);
        }
        break;

        case CAMERA_ANTIBANDING_SET_60HZ:
        {
           rc = ovm7690_i2c_write(ovm7690_client->addr,0x14,0x20,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }  
					 rc = ovm7690_i2c_write(ovm7690_client->addr,0x51,0x80,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            } 
            
        }            
        break;

        case CAMERA_ANTIBANDING_SET_50HZ:
        {
            rc = ovm7690_i2c_write(ovm7690_client->addr,0x14,0x21,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }
            rc = ovm7690_i2c_write(ovm7690_client->addr,0x50,0x9a,BYTE_LEN);
            if (rc < 0)
            {
                return rc;
            }  

        }
        break;

        case CAMERA_ANTIBANDING_SET_AUTO:
        {
            pr_err("auto banding can not support");
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
static long ovm7690_power_up(void)
{
    pr_err("%s: not supported!\n", __func__);
    return 0;
}
static long ovm7690_power_down(const struct msm_camera_sensor_info *dev, uint32_t on)
{
    long rc = 0;

    pr_err("%s: entry dev->sensor_pwd=%d  on=%d \n", __func__,dev->sensor_pwd,on);
    
    rc = gpio_request(dev->sensor_pwd, "ovm7690");
    if (0 == rc)
    {
        /* ignore "rc" */
        rc = gpio_direction_output(dev->sensor_pwd, on);

        /*
         * time delay for entry into standby
         * decrease time delay from 200ms to 10ms
         * to improve sensor init
         */
         pr_err("ovm7690_power_down:rc=%ld",rc);
        mdelay(10);
    }

    gpio_free(dev->sensor_pwd);

    return rc;

}


static long ovm7690_reg_init(void)
{

    int32_t array_length;
    int32_t i;
    long rc;
    
    pr_err("%s: entry\n", __func__);

    array_length = ovm7690_regs.prev_snap_reg_settings_size;

    /* Configure sensor for Preview mode and Snapshot mode */
    for (i = 0; i < array_length; i++)
    {
        rc = ovm7690_i2c_write(ovm7690_client->addr,
                               ovm7690_regs.prev_snap_reg_settings[i].waddr,
                               ovm7690_regs.prev_snap_reg_settings[i].wdata,
                               BYTE_LEN);

        if (rc < 0)
        {
            pr_err("write reg failed");
            return rc;
        }
     }

    return 0;
}



static int ovm7690_sensor_probe_init(const struct msm_camera_sensor_info *data)
{
    uint16_t model_idh = 0;
    uint16_t model_idl = 0;
    int rc;

    pr_err("%s: entry\n", __func__);

    if (!data || strcmp(data->sensor_name, "ovm7690"))
    {
        CCRT("%s: invalid parameters!\n", __func__);
        rc = -ENODEV;
        goto probe_init_fail;
    }

    ovm7690_ctrl = kzalloc(sizeof(struct ovm7690_ctrl_t), GFP_KERNEL);
    if (!ovm7690_ctrl)
    {
        CCRT("%s: kzalloc failed!\n", __func__);
        rc = -ENOMEM;
        goto probe_init_fail;
    }

    ovm7690_ctrl->sensordata = data;
    
    
   /*
    * Power sensor on
    */
    rc = msm_camera_power_frontend(MSM_CAMERA_PWRUP_MODE);
    if (rc < 0)
    {
        CCRT("%s: camera_power_frontend failed!\n", __func__);
        goto probe_init_fail;
    }
   

    mdelay(2);
    
 		// for  ovm7690 
		ovm7690_power_down(ovm7690_ctrl->sensordata,0);
		mdelay(5);
  

    /* Input MCLK */
    msm_camio_clk_rate_set(OVM7690_CAMIO_MCLK);
    mdelay(10);
    

 		rc = ovm7690_i2c_write(ovm7690_client->addr,0x12,0x80,BYTE_LEN);
	  if (rc < 0)
	  {
	      return rc;
	  }
    mdelay(5);
    rc = ovm7690_i2c_write(ovm7690_client->addr,0x49,0x0D,BYTE_LEN);
	  if (rc < 0)
	  {
	      return rc;
	  }


   	rc = ovm7690_reg_init();
    if (rc < 0)
    {
        goto probe_init_fail;
    }

     mdelay(1);
     
   

    rc = ovm7690_i2c_read(ovm7690_client->addr, 0x0A, &model_idh, BYTE_LEN);// RESET_REGISTER
    if (rc < 0)
    {
        return rc;
    }
    pr_err("%s: model_id HI = 0x%x\n", __func__, model_idh);
    rc = ovm7690_i2c_read(ovm7690_client->addr, 0x0B, &model_idl, BYTE_LEN);// RESET_REGISTER
    if (rc < 0)
    {
        return rc;
    }
    pr_err("%s: model_id LO = 0x%x\n", __func__, model_idl);

    /* Check if it matches it with the value in Datasheet */
    if (model_idh !=0x76 || model_idl !=0x91  )
    {
        pr_err("ovm7690  model_ id is error ");        
        rc = -EFAULT;
        goto probe_init_fail;
    }

 		ovm7690_power_down(ovm7690_ctrl->sensordata,1); 

 		mdelay(100);
 
    pr_err("%s is ok",__func__);
    return 0;

probe_init_fail:
    /*
      * To shut sensor down
      * Ignore "rc"
      */
    ovm7690_power_down(ovm7690_ctrl->sensordata,1);
    msm_camera_power_frontend(MSM_CAMERA_PWRDWN_MODE);
    kfree(ovm7690_ctrl);
    return rc;
}
static int ovm7690_sensor_init_probe(const struct msm_camera_sensor_info *data)
{
    uint16_t model_idh = 0;
    uint16_t model_idl = 0;
    int rc = 0;
//    uint16_t reg_value = 0;
    
    pr_err("%s: entry\n", __func__);

    /* Read the Model ID of the sensor */
    rc = ovm7690_i2c_read(ovm7690_client->addr, 0x0A, &model_idh, BYTE_LEN);// RESET_REGISTER
    if (rc < 0)
    {
        return rc;
    }
    rc = ovm7690_i2c_read(ovm7690_client->addr, 0x0B, &model_idl, BYTE_LEN);// RESET_REGISTER
    if (rc < 0)
    {
        return rc;
    }
   

    pr_err("%s: model_idh  model_idl 0x%x 0x%x\n", __func__, model_idh,model_idl);

    /* Check if it matches it with the value in Datasheet */
    if (model_idh != 0x76 || model_idl != 0x91)
    {
    	  pr_err("ovm7690 : model_id  is error \n");
        rc = -EFAULT;
        goto init_probe_fail;
    }


    rc = ovm7690_reg_init();
    if (rc < 0)
    {
        goto init_probe_fail;
    }

    return rc;

init_probe_fail:
    CCRT("%s: rc = %d, failed!\n", __func__, rc);
    return rc;
}

int ovm7690_sensor_init(const struct msm_camera_sensor_info *data)
{
    int rc = 0;

    pr_err("wt %s: entry\n", __func__);

    ovm7690_ctrl = kzalloc(sizeof(struct ovm7690_ctrl_t), GFP_KERNEL);
    if (!ovm7690_ctrl)
    {
        CCRT("%s: kzalloc failed!\n", __func__);
        rc = -ENOMEM;
        goto init_done;
    }

    if (data)
    {
        ovm7690_ctrl->sensordata = data;
    }

    
    rc = msm_camera_power_frontend(MSM_CAMERA_PWRUP_MODE);
    if (rc < 0)
    {
        pr_err("%s: camera_power_frontend failed!\n", __func__);
        goto init_fail;
    }


   //set ov5640 pwd h && set ovm760 pwd l to swtich ovm7690
    rc = gpio_request(1, "ov5640");
    if (0 == rc)
    {
        /* ignore "rc" */
        rc = gpio_direction_output(1, 1);

        /*
         * time delay for entry into standby
         * decrease time delay from 200ms to 10ms
         * to improve sensor init
         */
         pr_err("%s set ov5640 pwd high :rc=%d",__func__,rc);
        //mdelay();
    }
     gpio_free(1);

    
     mdelay(20);
     ovm7690_power_down(ovm7690_ctrl->sensordata,0);
     
    mdelay(5);
    
       
    /* Input MCLK */
    msm_camio_clk_rate_set(OVM7690_CAMIO_MCLK);
    
   // msm_camio_camif_pad_reg_reset();
    
    mdelay(5);
   
    mdelay(60);

   // for 20110719_night
		rc = ovm7690_i2c_write(ovm7690_client->addr,0x12,0x80,BYTE_LEN);
	  if (rc < 0)
	  {
	      return rc;
	  }
    mdelay(5);
    rc = ovm7690_i2c_write(ovm7690_client->addr,0x49,0x0D,BYTE_LEN);
	  if (rc < 0)
	  {
	      return rc;
	  }

	  
    rc = ovm7690_sensor_init_probe(ovm7690_ctrl->sensordata);
    if (rc < 0)
    {
        pr_err("%s: sensor_init_probe failed!\n", __func__);
        goto init_fail;
    }

init_done:
	pr_err(" %s wt is ok ",__func__);
    return rc;

init_fail:
    /*
      * To power sensor down
      * Ignore "rc"
      */
    msm_camera_power_frontend(MSM_CAMERA_PWRDWN_MODE);
    kfree(ovm7690_ctrl);
    return rc;
}

static int ovm7690_init_client(struct i2c_client *client)
{
    /* Initialize the MSM_CAMI2C Chip */
    init_waitqueue_head(&ovm7690_wait_queue);
    return 0;
}

int ovm7690_sensor_config(void __user *argp)
{
    struct sensor_cfg_data cfg_data;
    long rc = 0;

    pr_err("%s: entry\n", __func__);

    if (copy_from_user(&cfg_data, (void *)argp, sizeof(struct sensor_cfg_data)))
    {
        CCRT("%s: copy_from_user failed!\n", __func__);
        return -EFAULT;
    }


    pr_err("%s: cfgtype = %d, mode = %d\n", __func__, cfg_data.cfgtype, cfg_data.mode);

    switch (cfg_data.cfgtype)
    {
        case CFG_SET_MODE:
        {
            rc = ovm7690_set_sensor_mode(cfg_data.mode);
        }
        break;

        case CFG_SET_EFFECT:
        {
            rc = ovm7690_set_effect(cfg_data.mode, cfg_data.cfg.effect);
        }
        break;
        
        case CFG_PWR_UP:
        {
            rc = ovm7690_power_up();
        }
        break;
        
        case CFG_PWR_DOWN:
        {
            //rc = ovm7690_power_down();
        }
        break;
        case CFG_SET_WB:
        {
            rc = ovm7690_set_wb(cfg_data.cfg.wb_mode);
        }
        break;

        case CFG_SET_ISO:
        {
            rc = ovm7690_set_iso(cfg_data.cfg.iso_val);
        }
        break;

        case CFG_SET_ANTIBANDING:
        {
            rc = ovm7690_set_antibanding(cfg_data.cfg.antibanding);
        }
        break;

        case CFG_SET_BRIGHTNESS:
        {
            rc = ovm7690_set_brightness(cfg_data.cfg.brightness);
        }
        break;

        case CFG_SET_SATURATION:
        {
            rc = ovm7690_set_saturation(cfg_data.cfg.saturation);
        }
        break;

        case CFG_SET_CONTRAST:
        {
            rc = ovm7690_set_contrast(cfg_data.cfg.contrast);
        }
        break;

        case CFG_SET_SHARPNESS:
        {
            rc = ovm7690_set_sharpness(cfg_data.cfg.sharpness);
        }
        break;

        case CFG_SET_EXPOSURE_COMPENSATION:
        {
            rc = ovm7690_set_exposure_compensation(cfg_data.cfg.exposure);
        }
        break;
        
        default:
        {
            rc = -EFAULT;
        }
        break;
    }


    return rc;
}

int ovm7690_sensor_release(void)
{
    int rc = 0;
    
    pr_err("wt %s: entry\n", __func__);


    /*
      * Power VREG Off
      * Ignore "rc"
      */
#if 1
    ovm7690_power_down(ovm7690_ctrl->sensordata,1);

    rc = msm_camera_power_frontend(MSM_CAMERA_PWRDWN_MODE);

    kfree(ovm7690_ctrl);
#endif
   

    return rc;
}

static int ovm7690_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int rc = 0;

    pr_err("wt %s: entry\n", __func__);

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
    {
        rc = -ENOTSUPP;
        goto probe_failure;
    }

    ovm7690_sensorw = kzalloc(sizeof(struct ovm7690_work_t), GFP_KERNEL);
    if (!ovm7690_sensorw)
    {
        rc = -ENOMEM;
        goto probe_failure;
    }

    i2c_set_clientdata(client, ovm7690_sensorw);
    ovm7690_init_client(client);
    ovm7690_client = client;
    
    pr_err("wt %s ok",__func__);
    return 0;

probe_failure:
    kfree(ovm7690_sensorw);
    ovm7690_sensorw = NULL;
    pr_err("%s: rc = %d, failed!\n", __func__, rc);
    return rc;
}

static int __exit ovm7690_i2c_remove(struct i2c_client *client)
{
    struct ovm7690_work_t *sensorw = i2c_get_clientdata(client);

    pr_err("%s: entry\n", __func__);

    free_irq(client->irq, sensorw);   
    kfree(sensorw);

    ovm7690_client = NULL;
    ovm7690_sensorw = NULL;

    return 0;
}

static const struct i2c_device_id ovm7690_id[] = {
    { "ovm7690", 0},
    { },
};

static struct i2c_driver ovm7690_driver = {
    .id_table = ovm7690_id,
    .probe  = ovm7690_i2c_probe,
    .remove = __exit_p(ovm7690_i2c_remove),
    .driver = {
        .name = "ovm7690",
    },
};

static int32_t ovm7690_i2c_add_driver(void)
{
    int32_t rc = 0;
    
    pr_err("wt %s: entry\n", __func__);
    
    rc = i2c_add_driver(&ovm7690_driver);
    if (IS_ERR_VALUE(rc))
    {
        goto init_failure;
    }

    return rc;

init_failure:
    pr_err("%s: rc = %d, failed!\n", __func__, rc);
    return rc;
}

static void ovm7690_i2c_del_driver(void)
{
    i2c_del_driver(&ovm7690_driver);
}

void ovm7690_exit(void)
{
    CDBG("%s: entry\n", __func__);
    
    i2c_del_driver(&ovm7690_driver);
}

int ovm7690_sensor_probe(const struct msm_camera_sensor_info *info,
				                 struct msm_sensor_ctrl *s)
{
    int rc = 0;

    pr_err("%s: entry\n", __func__);
    rc = ovm7690_i2c_add_driver();
    if (rc < 0)
    {
        goto probe_failed;
    }


    rc = ovm7690_sensor_probe_init(info);
    if (rc < 0)
    {
        pr_err("%s: ovm7690_sensor_probe_init failed!\n", __func__);
        goto probe_failed;
    }

    pr_err("ovm ovm7690_sensor_probe 1");
    
    rc = ovm7690_sensor_release();
    if (rc < 0)
    {
        pr_err("%s: ovm7690_sensor_release failed!\n", __func__);
        goto probe_failed;
    }

    /*
     * modify sensor angle from 90 to 180, else snapshot angle is 90 rotated
     * ZTE_CAM_LJ_20110414_01
     */
    s->s_mount_angle = 0;
    s->s_camera_type = FRONT_CAMERA_2D;

    s->s_init       = ovm7690_sensor_init;
    s->s_release    = ovm7690_sensor_release;
    s->s_config     = ovm7690_sensor_config;

		pr_err("ovm7690_sensor_probe is ok ");
    return rc;

probe_failed:
    pr_err("%s: rc = %d, failed!\n", __func__, rc);


    ovm7690_i2c_del_driver();


    return rc;
}

static int __ovm7690_probe(struct platform_device *pdev)
{
	return msm_camera_drv_start(pdev, ovm7690_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
	.probe = __ovm7690_probe,
	.driver = {
		.name = "msm_camera_ovm7690",
		.owner = THIS_MODULE,
	},
};

static int __init ovm7690_init(void)
{	
	pr_err("wt entry %s ",__func__);
	return platform_driver_register(&msm_camera_driver);
}

//module_init(ovm7690_init);
fs_initcall(ovm7690_init);

MODULE_DESCRIPTION("OMNI VGA YUV sensor driver");
MODULE_LICENSE("GPL v2");
