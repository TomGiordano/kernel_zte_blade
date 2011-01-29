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
#include "mt9v113.h"

#define MT9V113_SLAVE_WR_ADDR 0x78 /* replaced by "msm_i2c_devices.addr" */
#define MT9V113_SLAVE_RD_ADDR 0x79

#define REG_MT9V113_MODEL_ID    0x0000
#define MT9V113_MODEL_ID        0x2280
#define REG_MT9V113_SENSOR_RESET     0x001A
#define REG_MT9V113_STANDBY_CONTROL  0x0018
#define REG_MT9V113_MCU_BOOT         0x001C

#define MT9V113_CAMIO_MCLK  24000000 /*48000000*/

#define MT9V113_GPIO_SWITCH_CTL     39

#if defined(CONFIG_MACH_RAISE)
#define MT9V113_GPIO_SWITCH_VAL     0
#elif defined(CONFIG_MACH_JOE)
#define MT9V113_GPIO_SWITCH_VAL     1
#else
#define MT9V113_GPIO_SWITCH_VAL     0
#endif 

struct mt9v113_work_t {
    struct work_struct work;
};

static struct mt9v113_work_t *mt9v113_sensorw;
static struct i2c_client *mt9v113_client;

struct mt9v113_ctrl_t {
    const struct msm_camera_sensor_info *sensordata;
};

static struct mt9v113_ctrl_t *mt9v113_ctrl;

static DECLARE_WAIT_QUEUE_HEAD(mt9v113_wait_queue);
DECLARE_MUTEX(mt9v113_sem);

extern int32_t msm_camera_power_frontend(enum msm_camera_pwr_mode_t pwr_mode);
extern int msm_camera_clk_switch(const struct msm_camera_sensor_info *data,
                                         uint32_t  gpio_switch,
                                         uint32_t  switch_val);
static int __attribute__((unused)) mt9v113_hard_standby(const struct msm_camera_sensor_info *dev, uint32_t on)
{
    int rc;
    
    CDBG("%s: entry\n", __func__);
    
    rc = gpio_request(dev->sensor_pwd, "mt9v113");
    if (0 == rc)
    {
        rc = gpio_direction_output(dev->sensor_pwd, on);
        mdelay(30);
    }

    gpio_free(dev->sensor_pwd);

    return rc;
}

static int mt9v113_hard_reset(const struct msm_camera_sensor_info *dev)
{
    int rc = 0;
    
    CDBG("%s: entry\n", __func__);

    rc = gpio_request(dev->sensor_reset, "mt9v113");
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

static int32_t mt9v113_i2c_txdata(unsigned short saddr,
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
    
    if (i2c_transfer(mt9v113_client->adapter, msg, 1) < 0)
    {
        CCRT("%s: failed!\n", __func__);
        return -EIO;
    }

    return 0;
}

static int32_t mt9v113_i2c_write(unsigned short saddr,
                                      unsigned short waddr,
                                      unsigned short wdata,
                                      enum mt9v113_width_t width)
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

            rc = mt9v113_i2c_txdata(saddr, buf, 4);
        }
        break;

        case BYTE_LEN:
        {
            buf[0] = waddr;
            buf[1] = wdata;

            rc = mt9v113_i2c_txdata(saddr, buf, 2);
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


static int mt9v113_i2c_rxdata(unsigned short saddr,
                                    unsigned char *rxdata,
                                    int length)
{
    struct i2c_msg msgs[] = {
    {
        .addr   = saddr,
        .flags = 0,
        .len   = 2,
        .buf   = rxdata,
    },
    {
        .addr   = saddr,
        .flags = I2C_M_RD,
        .len   = length,
        .buf   = rxdata,
    },
    };
    
    if (i2c_transfer(mt9v113_client->adapter, msgs, 2)  <  0)
    {
        CCRT("%s: failed!\n", __func__);
        return -EIO;
    }

    return 0;
}

static int32_t mt9v113_i2c_read(unsigned short saddr,
                                     unsigned short raddr,
                                     unsigned short *rdata,
                                     enum mt9v113_width_t width)
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

            rc = mt9v113_i2c_rxdata(saddr, buf, 2);
            if (rc < 0)
            {
                return rc;
            }

            *rdata = buf[0] << 8 | buf[1];
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

    
static long mt9v113_power_down(void)
{
    long rc = 0;

    CDBG("%s: entry\n", __func__);

    rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0018, 0x4028, WORD_LEN); 
    if (rc < 0)
    {
        return rc;
    }
    
    rc = mt9v113_i2c_write(mt9v113_client->addr, 0x001A, 0x0030, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }
    
    rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0018, 0x4029, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }
    
    rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0016, 0x00DF, WORD_LEN); 
    if (rc < 0)
    {
        return rc;
    }
    
    mdelay(10);


    return rc;
}

static long mt9v113_power_up(void)
{
    long rc = 0;
    
    CDBG("%s: entry\n", __func__);
    
    rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0018, 0x4028, WORD_LEN); 
    if (rc < 0)
    {
                return rc;
    }

    rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0016, 0x42DF, WORD_LEN);
    if (rc < 0)
    {
                return rc;
    }
    mdelay(10);


    return rc;
}

static long mt9v113_reg_init(void)
{

    int32_t array_length;
    int32_t i;
    long rc;
    
    CDBG("%s: entry\n", __func__);
    

    rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0016, 0x42DF, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }
    
    rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0010, 0x0110, WORD_LEN);    
    if (rc < 0)
    {
        return rc;
    }
    
    rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0012, 0x0000, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }
    
    mdelay(10);
    
    rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0014, 0x2147, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }
    
    mdelay(10);
    
    rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0014, 0x2047, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }
    
    mdelay(10);
    
    rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0014, 0xA046,WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }
    
    mdelay(10);

    array_length = mt9v113_regs.prev_snap_reg_settings_size;
    for (i = 0; i < array_length; i++)
    {
        rc = mt9v113_i2c_write(mt9v113_client->addr,
                               mt9v113_regs.prev_snap_reg_settings[i].waddr,
                               mt9v113_regs.prev_snap_reg_settings[i].wdata,
                               WORD_LEN);

        if (rc < 0)
        {
            return rc;
        }
     }

    

    rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }
    
    rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0005, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }
    
    mdelay(100);
    
    rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }
    
    rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, 0x0006, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }
    
    mdelay(100);

    return 0;
}
static long mt9v113_set_effect(int32_t mode, int32_t effect)
{

    uint16_t __attribute__((unused)) reg_addr;
    uint16_t __attribute__((unused)) reg_val;
    long rc = 0;
    
    CDBG("%s: entry\n", __func__);
    
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
               reg_addr = 0x2759;   
        }
        break;
    }

    switch (effect)
    {
        case CAMERA_EFFECT_OFF:
        {            
            rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, reg_addr, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            
            reg_val = 0x6440;
            rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, reg_val, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            
            break;
        }
        
        case CAMERA_EFFECT_MONO:
        {            
            rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, reg_addr, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            
            reg_val = 0x6441;
            rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, reg_val, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            
            break;
        }
        
        case CAMERA_EFFECT_NEGATIVE:
        {            
            rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, reg_addr, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            
            reg_val = 0x6443;
            rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, reg_val, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            
            break;
        }
        
        case CAMERA_EFFECT_SOLARIZE:
        {            
            rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, reg_addr, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            
            reg_val = 0x6445;                    
            rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0990, reg_val, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
            
            break;
        }
        case CAMERA_EFFECT_SEPIA:
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


    return rc;
}

static long mt9v113_set_sensor_mode(int32_t mode)
{

    long __attribute__((unused)) rc = 0;
    
    CDBG("%s: entry\n", __func__);
    
    switch (mode)
    {
        case SENSOR_PREVIEW_MODE:
        {
            rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA103, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }

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
            rc = mt9v113_i2c_write(mt9v113_client->addr, 0x098C, 0xA115, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }

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

    return 0;
}


static int mt9v113_sensor_init_probe(const struct msm_camera_sensor_info *data)
{
    uint16_t model_id = 0;
    int rc = 0;
    
    CDBG("%s: entry\n", __func__);


    rc = mt9v113_hard_reset(data);
    if (rc < 0)
    {
        CCRT("reset failed!\n");
        goto init_probe_fail;
    }
    
    rc = mt9v113_power_up();
    if (rc < 0)
    {
        return rc;
    }

    rc = mt9v113_i2c_write(mt9v113_client->addr, 0x301A, 0x1218, WORD_LEN);// RESET_REGISTER
    if (rc < 0)
    {
        return rc;
    }
    mdelay(100);
    
    rc = mt9v113_i2c_write(mt9v113_client->addr, 0x301A, 0x121C, WORD_LEN);
    if (rc < 0)
    {
        return rc;
    }
    mdelay(100);

    rc = mt9v113_i2c_write(mt9v113_client->addr, 0x001A, 0x0011, WORD_LEN); // RESET_AND_MISC_CONTROL
    if (rc < 0)
    {
        return rc;
    }
    mdelay(10);
    
    rc = mt9v113_i2c_write(mt9v113_client->addr, 0x001A, 0x0010, WORD_LEN); // RESET_AND_MISC_CONTROL
    if (rc < 0)
    {
        return rc;
    }
    mdelay(10);
    
    rc = mt9v113_i2c_write(mt9v113_client->addr, 0x0018, 0x4028, WORD_LEN); // STANDBY_CONTROL
    if (rc < 0)
    {
        return rc;
    }
    mdelay(50);
    
    rc = mt9v113_i2c_write(mt9v113_client->addr, 0x001A, 0x0210, WORD_LEN); // RESET_AND_MISC_CONTROL
    if (rc < 0)
    {
        return rc;
    }

    rc = mt9v113_i2c_write(mt9v113_client->addr, 0x001E, 0x0777, WORD_LEN); // PAD_SLEW
    if (rc < 0)
    {
        return rc;
    }
    mdelay(100);
    rc = mt9v113_i2c_read(mt9v113_client->addr, REG_MT9V113_MODEL_ID, &model_id, WORD_LEN);
    if (rc < 0)
    {
        goto init_probe_fail;
    }

    CINF("%s: model_id = 0x%x\n", __func__, model_id);

    if (model_id != MT9V113_MODEL_ID)
    {
        rc = -EFAULT;
        goto init_probe_fail;
    }

    rc = mt9v113_reg_init();
    if (rc < 0)
    {
        goto init_probe_fail;
    }

    return rc;

init_probe_fail:
    CCRT("%s: rc = %d, failed!\n", __func__, rc);
    return rc;
}

int mt9v113_sensor_init(const struct msm_camera_sensor_info *data)
{
    int rc = 0;

    CDBG("%s: entry\n", __func__);

    mt9v113_ctrl = kzalloc(sizeof(struct mt9v113_ctrl_t), GFP_KERNEL);
    if (!mt9v113_ctrl)
    {
        CCRT("%s: kzalloc failed!\n", __func__);
        rc = -ENOMEM;
        goto init_done;
    }

    if (data)
    {
        mt9v113_ctrl->sensordata = data;
    }

    rc = msm_camera_power_frontend(MSM_CAMERA_PWRUP_MODE);
    if (rc < 0)
    {
        CCRT("%s: camera_power_frontend failed!\n", __func__);
        goto init_fail;
    }
    
    rc = msm_camera_clk_switch(mt9v113_ctrl->sensordata, MT9V113_GPIO_SWITCH_CTL, MT9V113_GPIO_SWITCH_VAL);
    if (rc < 0)
    {
        CCRT("%s: camera_clk_switch failed!\n", __func__);
        goto init_fail;
    }

    msm_camio_clk_rate_set(MT9V113_CAMIO_MCLK);
    mdelay(5);

    msm_camio_camif_pad_reg_reset();

    rc = mt9v113_sensor_init_probe(mt9v113_ctrl->sensordata);
    if (rc < 0)
    {
        CCRT("%s: sensor_init_probe failed!\n", __func__);
        goto init_fail;
    }

init_done:
    return rc;

init_fail:
    msm_camera_power_frontend(MSM_CAMERA_PWRDWN_MODE);
    kfree(mt9v113_ctrl);
    return rc;
}

static int mt9v113_init_client(struct i2c_client *client)
{
    init_waitqueue_head(&mt9v113_wait_queue);
    return 0;
}

int mt9v113_sensor_config(void __user *argp)
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
            rc = mt9v113_set_sensor_mode(cfg_data.mode);
        }
        break;

        case CFG_SET_EFFECT:
        {
            rc = mt9v113_set_effect(cfg_data.mode, cfg_data.cfg.effect);
        }
        break;
        
        case CFG_PWR_UP:
        {
            rc = mt9v113_power_up();
        }
        break;
        
        case CFG_PWR_DOWN:
        {
            rc = mt9v113_power_down();
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

int mt9v113_sensor_release(void)
{
    int rc = 0;
    
    CDBG("%s: entry\n", __func__);
    rc = msm_camera_power_frontend(MSM_CAMERA_PWRDWN_MODE);

    kfree(mt9v113_ctrl);
   
    return rc;
}

static int mt9v113_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int rc = 0;

    CDBG("%s: entry\n", __func__);

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
    {
        rc = -ENOTSUPP;
        goto probe_failure;
    }

    mt9v113_sensorw = kzalloc(sizeof(struct mt9v113_work_t), GFP_KERNEL);
    if (!mt9v113_sensorw)
    {
        rc = -ENOMEM;
        goto probe_failure;
    }

    i2c_set_clientdata(client, mt9v113_sensorw);
    mt9v113_init_client(client);
    mt9v113_client = client;

    return 0;

probe_failure:
    kfree(mt9v113_sensorw);
    mt9v113_sensorw = NULL;
    CCRT("%s: rc = %d, failed!\n", __func__, rc);
    return rc;
}

static int __exit mt9v113_i2c_remove(struct i2c_client *client)
{
    struct mt9v113_work_t *sensorw = i2c_get_clientdata(client);

    CDBG("%s: entry\n", __func__);

    free_irq(client->irq, sensorw);   
    kfree(sensorw);

    mt9v113_client = NULL;
    mt9v113_sensorw = NULL;

    return 0;
}

static const struct i2c_device_id mt9v113_id[] = {
    { "mt9v113", 0},
    { },
};

static struct i2c_driver mt9v113_driver = {
    .id_table = mt9v113_id,
    .probe  = mt9v113_i2c_probe,
    .remove = __exit_p(mt9v113_i2c_remove),
    .driver = {
        .name = "mt9v113",
    },
};

static int32_t mt9v113_i2c_add_driver(void)
{
    int32_t rc = 0;
    
    CDBG("%s: entry\n", __func__);
    
    rc = i2c_add_driver(&mt9v113_driver);
    if (IS_ERR_VALUE(rc))
    {
        goto init_failure;
    }

    return rc;

init_failure:
    CCRT("%s: rc = %d, failed!\n", __func__, rc);
    return rc;
}

void mt9v113_exit(void)
{
    CDBG("%s: entry\n", __func__);
    
    i2c_del_driver(&mt9v113_driver);
}

int mt9v113_sensor_probe(const struct msm_camera_sensor_info *info,
				                 struct msm_sensor_ctrl *s)
{
    int rc = 0;

    CDBG("%s: entry\n", __func__);

    rc = mt9v113_i2c_add_driver();
    if (rc < 0)
    {
        goto probe_done;
    }

    s->s_init       = mt9v113_sensor_init;
    s->s_release    = mt9v113_sensor_release;
    s->s_config     = mt9v113_sensor_config;

    return rc;

probe_done:
    CCRT("%s: rc = %d, failed!\n", __func__, rc);
    return rc;
}

static int __mt9v113_probe(struct platform_device *pdev)
{
	return msm_camera_drv_start(pdev, mt9v113_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
	.probe = __mt9v113_probe,
	.driver = {
		.name = "msm_camera_mt9v113",
		.owner = THIS_MODULE,
	},
};

static int __init mt9v113_init(void)
{
	return platform_driver_register(&msm_camera_driver);
}

module_init(mt9v113_init);

