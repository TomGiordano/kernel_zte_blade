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
#include "mt9p111.h"

#define MT9P111_SLAVE_WR_ADDR 0x7A 
#define MT9P111_SLAVE_RD_ADDR 0x7B 

#define REG_MT9P111_MODEL_ID    0x0000
#define MT9P111_MODEL_ID        0x2880
#define REG_MT9P111_SENSOR_RESET     0x001A
#define REG_MT9P111_STANDBY_CONTROL  0x0018

#define MT9P111_CAMIO_MCLK  12000000    /* UNCONFIRMED */

#define MT9P111_GPIO_SHUTDOWN_CTL   32
#define MT9P111_GPIO_SWITCH_CTL     39

#if defined(CONFIG_MACH_RAISE)
#define MT9P111_GPIO_SWITCH_VAL     1
#elif defined(CONFIG_MACH_JOE)
#define MT9P111_GPIO_SWITCH_VAL     0
#else
#define MT9P111_GPIO_SWITCH_VAL     1
#endif 

struct mt9p111_work_t {
    struct work_struct work;
};

static struct mt9p111_work_t *mt9p111_sensorw;
static struct i2c_client *mt9p111_client;

struct mt9p111_ctrl_t {
	const struct msm_camera_sensor_info *sensordata;
};

static struct mt9p111_ctrl_t *mt9p111_ctrl;

static DECLARE_WAIT_QUEUE_HEAD(mt9p111_wait_queue);
DECLARE_MUTEX(mt9p111_sem);

extern int32_t msm_camera_power_backend(enum msm_camera_pwr_mode_t pwr_mode);
extern int msm_camera_clk_switch(const struct msm_camera_sensor_info *data,
                                         uint32_t gpio_switch,
                                         uint32_t switch_val);


static int mt9p111_hard_standby(const struct msm_camera_sensor_info *dev, uint32_t on)
{
    int rc;

    CDBG("%s: entry\n", __func__);

    rc = gpio_request(dev->sensor_pwd, "mt9p111");
    if (0 == rc)
    {
        rc = gpio_direction_output(dev->sensor_pwd, on);
        mdelay(200);
    }

    gpio_free(dev->sensor_pwd);

    return rc;
}

static int mt9p111_hard_reset(const struct msm_camera_sensor_info *dev)
{
    int rc = 0;

    CDBG("%s: entry\n", __func__);

    rc = gpio_request(dev->sensor_reset, "mt9p111");
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

static int32_t mt9p111_i2c_txdata(unsigned short saddr,
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

    if (i2c_transfer(mt9p111_client->adapter, msg, 1) < 0)
    {
        CCRT("%s: failed!\n", __func__);
        return -EIO;
    }

    return 0;
}

static int32_t mt9p111_i2c_write(unsigned short saddr,
                                      unsigned short waddr,
                                      unsigned short wdata,
                                      enum mt9p111_width_t width)
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

            rc = mt9p111_i2c_txdata(saddr, buf, 4);
        }
        break;

        case BYTE_LEN:
        {
            buf[0] = waddr;
            buf[1] = wdata;

            rc = mt9p111_i2c_txdata(saddr, buf, 2);
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

static int32_t mt9p111_i2c_write_table(struct mt9p111_i2c_reg_conf const *reg_conf_tbl,
                                             int len)
{
    uint32_t i;
    int32_t rc = 0;

    for (i = 0; i < len; i++)
    {
        rc = mt9p111_i2c_write(mt9p111_client->addr,
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

    return rc;
}

static int mt9p111_i2c_rxdata(unsigned short saddr,
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

    if (i2c_transfer(mt9p111_client->adapter, msgs, 2) < 0)
    {
        CCRT("%s: failed!\n", __func__);
        return -EIO;
    }

    return 0;
}

static int32_t mt9p111_i2c_read(unsigned short saddr,
                                     unsigned short raddr,
                                     unsigned short *rdata,
                                     enum mt9p111_width_t width)
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

            rc = mt9p111_i2c_rxdata(saddr, buf, 2);
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

static int32_t mt9p111_set_lens_roll_off(void)
{
    int32_t rc = 0;
    rc = mt9p111_i2c_write_table(mt9p111_regs.rftbl, mt9p111_regs.rftbl_size);
    return rc;
}

static long mt9p111_reg_init(void)
{
    long rc;

    CDBG("%s: entry\n", __func__);

    rc = mt9p111_i2c_write_table(mt9p111_regs.plltbl, mt9p111_regs.plltbl_size);
    if (rc < 0)
    {
        return rc;
    }
    rc = mt9p111_i2c_write_table(mt9p111_regs.prev_snap_reg_settings, mt9p111_regs.prev_snap_reg_settings_size);
    if (rc < 0)
    {
        return rc;
    }

    rc = mt9p111_i2c_write_table(mt9p111_regs.noise_reduction_reg_settings, mt9p111_regs.noise_reduction_reg_settings_size);
    if (rc < 0)
    {
        return rc;
    }
    rc = mt9p111_i2c_write_table(mt9p111_regs.stbl, mt9p111_regs.stbl_size);
    if (rc < 0)
    {
        return rc;
    }

    rc = mt9p111_set_lens_roll_off();
    if (rc < 0)
    {
        return rc;
    }

    return 0;
}

static long mt9p111_set_sensor_mode(int32_t mode)
{
    long rc = 0;

    CDBG("%s: entry\n", __func__);

    switch (mode)
    {
        case SENSOR_PREVIEW_MODE:
        {
            rc = mt9p111_i2c_write(mt9p111_client->addr, 0x098E, 0x843C, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }

            rc = mt9p111_i2c_write(mt9p111_client->addr, 0x0990, 0x0100, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }

            rc = mt9p111_i2c_write(mt9p111_client->addr, 0x098E, 0x8404, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }

            rc = mt9p111_i2c_write(mt9p111_client->addr, 0x0990, 0x0100, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }
        }
        break;

        case SENSOR_SNAPSHOT_MODE:
        {
            rc = mt9p111_i2c_write(mt9p111_client->addr, 0x098E, 0x843C, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }

            rc = mt9p111_i2c_write(mt9p111_client->addr, 0x0990, 0xFF00, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }

            rc = mt9p111_i2c_write(mt9p111_client->addr, 0x098E, 0x8404, WORD_LEN);
            if (rc < 0)
            {
                return rc;
            }

            rc = mt9p111_i2c_write(mt9p111_client->addr, 0x0990, 0x0200, WORD_LEN);
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

static long mt9p111_set_effect(int32_t mode, int32_t effect)
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
        case CAMERA_EFFECT_MONO:
        case CAMERA_EFFECT_NEGATIVE:
        case CAMERA_EFFECT_SOLARIZE:
        case CAMERA_EFFECT_SEPIA:
        {
        }
        break;

        default:
        {

            return -EFAULT;
        }
    }

    return rc;
}

static long mt9p111_power_up(void)
{
    long rc = 0;
    uint32_t switch_on;

    CDBG("%s: entry\n", __func__);


    switch_on = 0;
    rc = mt9p111_hard_standby(mt9p111_ctrl->sensordata, switch_on);
    if (rc < 0)
    {
        return rc;
    }

    return rc;
}
static long mt9p111_power_down(void)
{
    long rc = 0;
    uint32_t switch_on;

    CDBG("%s: entry\n", __func__);

    rc = mt9p111_i2c_write(mt9p111_client->addr, 0x0028, 0x0000, WORD_LEN); 
    if (rc < 0)
    {
        return rc;
    }

    switch_on = 1;
    rc = mt9p111_hard_standby(mt9p111_ctrl->sensordata, switch_on);
    if (rc < 0)
    {
        return rc;
    }


    return rc;
}

#if defined(CONFIG_MACH_RAISE) || defined(CONFIG_MACH_JOE)
static int mt9p111_power_shutdown(uint32_t on)
#elif defined(CONFIG_MACH_MOONCAKE)
static int __attribute__((unused)) mt9p111_power_shutdown(uint32_t on)
#else
static int __attribute__((unused)) mt9p111_power_shutdown(uint32_t on)
#endif 
{
    int rc;

    CDBG("%s: entry\n", __func__);

    rc = gpio_request(MT9P111_GPIO_SHUTDOWN_CTL, "mt9p111");
    if (0 == rc)
    {
        rc = gpio_direction_output(MT9P111_GPIO_SHUTDOWN_CTL, on);
        mdelay(1);
    }

    gpio_free(MT9P111_GPIO_SHUTDOWN_CTL);

    return rc;
}

static int mt9p111_sensor_init_probe(const struct msm_camera_sensor_info *data)
{
    uint16_t model_id = 0;
    uint32_t switch_on;
    int rc = 0;

    CDBG("%s: entry\n", __func__);

#if defined(CONFIG_MACH_RAISE) || defined(CONFIG_MACH_JOE)
    switch_on = 0;
    rc = mt9p111_power_shutdown(switch_on);
    if (rc < 0)
    {
        CCRT("enter/quit lowest-power mode failed!\n");
        goto init_probe_fail;
    }
#elif defined(CONFIG_MACH_MOONCAKE)
#else
#endif 

    switch_on = 0;
    rc = mt9p111_hard_standby(data, switch_on);
    if (rc < 0)
    {
        CCRT("set standby failed!\n");
        goto init_probe_fail;
    }
    rc = mt9p111_hard_reset(data);
    if (rc < 0)
    {
        CCRT("reset failed!\n");
        goto init_probe_fail;
    }

    rc = mt9p111_i2c_read(mt9p111_client->addr, REG_MT9P111_MODEL_ID, &model_id, WORD_LEN);
    if (rc < 0)
    {
        goto init_probe_fail;
    }

    CDBG("%s: model_id = 0x%x\n", __func__, model_id);
    if (model_id != MT9P111_MODEL_ID)
    {
        rc = -EFAULT;
        goto init_probe_fail;
    }

    rc = mt9p111_reg_init();
    if (rc < 0)
    {
        goto init_probe_fail;
    }

    rc = mt9p111_i2c_write(mt9p111_client->addr, REG_MT9P111_STANDBY_CONTROL, 0x2008, WORD_LEN);
    if (rc < 0)
    {
        goto init_probe_fail;
    }

    return rc;

init_probe_fail:
    CCRT("%s: rc = %d, failed!\n", __func__, rc);
    return rc;
}

int mt9p111_sensor_init(const struct msm_camera_sensor_info *data)
{
    int rc = 0;

    CDBG("%s: entry\n", __func__);

    mt9p111_ctrl = kzalloc(sizeof(struct mt9p111_ctrl_t), GFP_KERNEL);
    if (!mt9p111_ctrl)
    {
        CCRT("%s: kzalloc failed!\n", __func__);
        rc = -ENOMEM;
        goto init_done;
    }

    if (data)
    {
        mt9p111_ctrl->sensordata = data;
    }

    rc = msm_camera_power_backend(MSM_CAMERA_PWRUP_MODE);
    if (rc < 0)
    {
        CCRT("%s: camera_power_backend failed!\n", __func__);
        goto init_fail;
    }

#if defined(CONFIG_MACH_RAISE) || defined(CONFIG_MACH_JOE)
    rc = msm_camera_clk_switch(mt9p111_ctrl->sensordata, MT9P111_GPIO_SWITCH_CTL, MT9P111_GPIO_SWITCH_VAL);
    if (rc < 0)
    {
        CCRT("%s: camera_clk_switch failed!\n", __func__);
        goto init_fail;
    }
#elif defined(CONFIG_MACH_MOONCAKE)
#else
#endif 

    msm_camio_clk_rate_set(MT9P111_CAMIO_MCLK);
    mdelay(5);

    msm_camio_camif_pad_reg_reset();

    rc = mt9p111_sensor_init_probe(mt9p111_ctrl->sensordata);
    if (rc < 0)
    {
        CCRT("%s: sensor_init_probe failed!\n", __func__);
        goto init_fail;
    }

init_done:
    return rc;

init_fail:
    msm_camera_power_backend(MSM_CAMERA_PWRDWN_MODE);
    kfree(mt9p111_ctrl);
    return rc;
}

static int mt9p111_init_client(struct i2c_client *client)
{
    init_waitqueue_head(&mt9p111_wait_queue);
    return 0;
}

int mt9p111_sensor_config(void __user *argp)
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
            rc = mt9p111_set_sensor_mode(cfg_data.mode);
        }
        break;

        case CFG_SET_EFFECT:
        {
            rc = mt9p111_set_effect(cfg_data.mode, cfg_data.cfg.effect);
        }
        break;

        case CFG_PWR_UP:
        {
            rc = mt9p111_power_up();
        }
        break;

        case CFG_PWR_DOWN:
        {
            rc = mt9p111_power_down();
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

int mt9p111_sensor_release(void)
{
    int rc = 0;

    CDBG("%s: entry\n", __func__);
    msm_camera_power_backend(MSM_CAMERA_PWRDWN_MODE);

    kfree(mt9p111_ctrl);

    return rc;
}

static int mt9p111_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int rc = 0;

    CDBG("%s: entry\n", __func__);

    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
    {
        rc = -ENOTSUPP;
        goto probe_failure;
    }

    mt9p111_sensorw = kzalloc(sizeof(struct mt9p111_work_t), GFP_KERNEL);
    if (!mt9p111_sensorw)
    {
        rc = -ENOMEM;
        goto probe_failure;
    }

    i2c_set_clientdata(client, mt9p111_sensorw);
    mt9p111_init_client(client);
    mt9p111_client = client;

    return 0;

probe_failure:
    kfree(mt9p111_sensorw);
    mt9p111_sensorw = NULL;
    CCRT("%s: rc = %d, failed!\n", __func__, rc);
    return rc;
}

static int __exit mt9p111_i2c_remove(struct i2c_client *client)
{
    struct mt9p111_work_t *sensorw = i2c_get_clientdata(client);

    CDBG("%s: entry\n", __func__);

    free_irq(client->irq, sensorw);   
    kfree(sensorw);

    mt9p111_client = NULL;
    mt9p111_sensorw = NULL;

    return 0;
}

static const struct i2c_device_id mt9p111_id[] = {
    { "mt9p111", 0},
    { },
};

static struct i2c_driver mt9p111_driver = {
    .id_table = mt9p111_id,
    .probe  = mt9p111_i2c_probe,
    .remove = __exit_p(mt9p111_i2c_remove),
    .driver = {
        .name = "mt9p111",
    },
};

static int32_t mt9p111_i2c_add_driver(void)
{
    int32_t rc = 0;

    rc = i2c_add_driver(&mt9p111_driver);
    if (IS_ERR_VALUE(rc))
    {
        goto init_failure;
    }

    return rc;

init_failure:
    CCRT("%s: rc = %d, failed!\n", __func__, rc);
    return rc;
}

void mt9p111_exit(void)
{
    CDBG("%s: entry\n", __func__);
    i2c_del_driver(&mt9p111_driver);
}

int mt9p111_sensor_probe(const struct msm_camera_sensor_info *info,
				                 struct msm_sensor_ctrl *s)
{
    int rc = 0;

    CDBG("%s: entry\n", __func__);

    rc = mt9p111_i2c_add_driver();
    if (rc < 0)
    {
        goto probe_done;
    }

    s->s_init       = mt9p111_sensor_init;
    s->s_release    = mt9p111_sensor_release;
    s->s_config     = mt9p111_sensor_config;

    return rc;

probe_done:
    CCRT("%s: rc = %d, failed!\n", __func__, rc);
    return rc;
}

static int __mt9p111_probe(struct platform_device *pdev)
{
	return msm_camera_drv_start(pdev, mt9p111_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
	.probe = __mt9p111_probe,
	.driver = {
		.name = "msm_camera_mt9p111",
		.owner = THIS_MODULE,
	},
};

static int __init mt9p111_init(void)
{
	return platform_driver_register(&msm_camera_driver);
}

module_init(mt9p111_init);
