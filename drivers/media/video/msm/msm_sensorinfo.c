/*
 * drivers/media/video/msm/msm_sensorinfo.c
 *
 * For sensor cfg test
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
 * Created by li.jing19@zte.com.cn
 */
/*-----------------------------------------------------------------------------------------
  when         who      what, where, why                        comment tag
  --------     ----     -------------------------------------   ---------------------------
  2010-09-03   jia      modify  MT9D113_MODEL_ID                ZTE_CAMERA_JIA_20100903
                        and MT9D115_MODEL_ID
  2010-09-02   jia      set MT9D113_MODEL_ID                    ZTE_CAMERA_JIA_20100902
                        and MT9D115_MODEL_ID
  2010-08-18   jia      set OV5642_MODEL_ID                     ZTE_CAMERA_JIA_20100818
  2010-07-06   li.jing  set MT9D115_MODEL_ID                    ZTE_CAMERA_LIJING_20100706
  2010-06-29   li.jing  add config for MT9D115-2.0Mp-FF-Socket  ZTE_CAMERA_LIJING_20100629
  2010-06-13   lijing   modify file permission                  LIJING_CAM_20100613
  2010-06-10   lijing   create file                             
------------------------------------------------------------------------------------------*/

#include <linux/sysdev.h>
#include <linux/i2c.h>

/*-----------------------------------------------------------------------------------------
 *
 * MACRO DEFINITION
 *
 *----------------------------------------------------------------------------------------*/
#define MAX_NAME_LENGTH     32

#define SENSOR_INFO_MT9T111_MODEL_ID    0x2680
#define SENSOR_INFO_MT9T112_MODEL_ID    0x2682
#define SENSOR_INFO_MT9P111_MODEL_ID    0x2880
#define SENSOR_INFO_MT9D115_MODEL_ID    0x0011
#define SENSOR_INFO_MT9D113_MODEL_ID    0x0003
#define SENSOR_INFO_OV5642_MODEL_ID     0x5642

/*-----------------------------------------------------------------------------------------
 *
 * TYPE DECLARATION
 *
 *----------------------------------------------------------------------------------------*/
static struct sysdev_class camera_sysdev_class = {
    .name = "camera",
};

static struct sys_device camera_sys_device = {
    .id = 0,
    .cls = &camera_sysdev_class,
};

/*-----------------------------------------------------------------------------------------
 *
 * GLOBAL VARIABLE DEFINITION
 *
 *----------------------------------------------------------------------------------------*/
static uint16_t g_sensor_id = 0;

/*-----------------------------------------------------------------------------------------
 *
 * FUNCTION DECLARATION
 *
 *----------------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------------------
 *
 * FUNCTION DEFINITION
 *
 *----------------------------------------------------------------------------------------*/
static ssize_t sensorinfo_show_id(struct sys_device *dev,
                                        struct sysdev_attribute *attr,
                                        char *buf)
{
    return snprintf(buf, PAGE_SIZE, "0x%x\n", g_sensor_id);
}

static ssize_t sensorinfo_show_name(struct sys_device *dev,
                                            struct sysdev_attribute *attr,
                                            char *buf)
{
    char sensor_name[MAX_NAME_LENGTH] = {0x00};

    switch(g_sensor_id)
    {
        case SENSOR_INFO_MT9T112_MODEL_ID:
            sprintf(sensor_name, "MT9T112-3.0Mp-AF");
            break;
        case SENSOR_INFO_MT9T111_MODEL_ID:
            sprintf(sensor_name, "MT9T111-3.0Mp-AF");
            break;
        case SENSOR_INFO_MT9P111_MODEL_ID:
            sprintf(sensor_name, "MT9P111-5.0Mp-AF");
            break;
        case SENSOR_INFO_MT9D115_MODEL_ID:
            sprintf(sensor_name, "MT9D115-2.0Mp-FF");
            break;
        case SENSOR_INFO_MT9D113_MODEL_ID:
            sprintf(sensor_name, "MT9D113-2.0Mp-FF");
            break;
        case SENSOR_INFO_OV5642_MODEL_ID:
            sprintf(sensor_name, "OV5642-5.0Mp-AF");
            break;
        default:
            sprintf(sensor_name, "No sensor or error ID!");
            break;
    }

    return snprintf(buf, PAGE_SIZE, "%s\n", sensor_name);
}

/*
 * LIJING_CAM_20100613
 * modify file permission from 0400->0404
 */
static struct sysdev_attribute sensorinfo_files[] = {
    _SYSDEV_ATTR(id, 0404, sensorinfo_show_id, NULL),
    _SYSDEV_ATTR(name, 0404, sensorinfo_show_name, NULL),
};

static void sensorinfo_create_files(struct sys_device *dev,
                                        struct sysdev_attribute files[],
                                        int size)
{
    int i;

    for (i = 0; i < size; i++) {
        int err = sysdev_create_file(dev, &files[i]);
        if (err) {
            pr_err("%s: sysdev_create_file(%s)=%d\n",
                   __func__, files[i].attr.name, err);
            return;
        }
    }
}

DECLARE_MUTEX(set_sensor_id_sem);
void msm_sensorinfo_set_sensor_id(uint16_t id)
{
    down(&set_sensor_id_sem);
    g_sensor_id = id;
    up(&set_sensor_id_sem);
}
EXPORT_SYMBOL(msm_sensorinfo_set_sensor_id);

/*
 * Attention:
 *
 * Path of camera's sysdev: /sys/devices/system/camera/camera0
 */
static int __init sensorinfo_init(void)
{
    int err;

    err = sysdev_class_register(&camera_sysdev_class);
    if (err) {
        pr_err("%s: sysdev_class_register fail (%d)\n",
               __func__, err);
        return -EFAULT;
    }

    err = sysdev_register(&camera_sys_device);
    if (err) {
        pr_err("%s: sysdev_register fail (%d)\n",
               __func__, err);
        return -EFAULT;
    }

    sensorinfo_create_files(&camera_sys_device, sensorinfo_files,
    ARRAY_SIZE(sensorinfo_files));

    return 0;
}
module_init(sensorinfo_init);

