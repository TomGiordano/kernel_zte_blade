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

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/pwm.h>
#include <linux/pmic8058-pwm.h>
#include <mach/pmic.h>
#include <mach/camera.h>
#include <mach/gpio.h>


#if defined(CONFIG_MSM_CAMERA_FLASH)

#if defined(CONFIG_MACH_JOE)
#define MSM_CAMERA_FLASH_LED_GPIO   (88)
#else
#define MSM_CAMERA_FLASH_LED_GPIO   (255)   
#endif
static uint32_t flash_led_enable = 0;

static int32_t msm_camera_flash_set_led_gpio(int32_t gpio_val)
{
    int32_t rc = -EFAULT;

    CDBG("%s: gpio_val=%d\n", __func__, gpio_val);

    rc = gpio_request(MSM_CAMERA_FLASH_LED_GPIO, "flashled-gpio");
    if (0 == rc)
    {
        rc = gpio_direction_output(MSM_CAMERA_FLASH_LED_GPIO, gpio_val);
    }
    gpio_free(MSM_CAMERA_FLASH_LED_GPIO);

    return rc;
}
int32_t msm_camera_flash_set_led_state(struct msm_camera_sensor_flash_data *fdata,
                                                 unsigned led_state)
{
    int32_t rc = 0;

    CDBG("%s: led_state: %d\n", __func__, led_state);
	
    if (fdata->flash_type != MSM_CAMERA_FLASH_LED)
    {
		return -ENODEV;
    }
	
    switch(led_state)
    {
        case MSM_CAMERA_LED_OFF:
            flash_led_enable = 0;
            break;

        case MSM_CAMERA_LED_LOW:
        case MSM_CAMERA_LED_HIGH:
            CDBG("%s: set MSM_CAMERA_LED_LOW/MSM_CAMERA_LED_HIGH\n", __func__);
            flash_led_enable = 1;
            break;

        default:
            flash_led_enable = 0;
            rc = -EFAULT;
            CDBG("%s: rc=%d\n", __func__, rc);
            return rc;
    }

    CDBG("%s: rc=%d\n", __func__, rc);

    return rc;
}

int32_t msm_camera_flash_led_enable(void)
{
    int32_t gpio_val;
    int32_t rc = 0;

    CDBG("%s: entry: flash_led_enable=%d\n", __func__, flash_led_enable);

    if (flash_led_enable)
    {
        gpio_val = 1;
        rc = msm_camera_flash_set_led_gpio(gpio_val);
    }

    return rc;
}

int32_t msm_camera_flash_led_disable(void)
{
    int32_t gpio_val;
    int32_t rc;

    CDBG("%s: entry\n", __func__);

    gpio_val = 0;
    rc = msm_camera_flash_set_led_gpio(gpio_val);

    return rc;
}
#else
static int msm_camera_flash_pwm(
	struct msm_camera_sensor_flash_pwm *pwm,
	unsigned led_state)
{
	int rc = 0;
	int PWM_PERIOD = NSEC_PER_SEC / pwm->freq;

	static struct pwm_device *flash_pwm;

	if (!flash_pwm) {
		flash_pwm = pwm_request(pwm->channel, "camera-flash");
		if (flash_pwm == NULL || IS_ERR(flash_pwm)) {
			pr_err("%s: FAIL pwm_request(): flash_pwm=%p\n",
			       __func__, flash_pwm);
			flash_pwm = NULL;
			return -ENXIO;
		}
	}

	switch (led_state) {
	case MSM_CAMERA_LED_LOW:
		rc = pwm_config(flash_pwm,
			(PWM_PERIOD/pwm->max_load)*pwm->low_load,
			PWM_PERIOD);
		if (rc >= 0)
			rc = pwm_enable(flash_pwm);
		break;

	case MSM_CAMERA_LED_HIGH:
		rc = pwm_config(flash_pwm,
			(PWM_PERIOD/pwm->max_load)*pwm->high_load,
			PWM_PERIOD);
		if (rc >= 0)
			rc = pwm_enable(flash_pwm);
		break;

	case MSM_CAMERA_LED_OFF:
		pwm_disable(flash_pwm);
		break;

	default:
		rc = -EFAULT;
		break;
	}

	return rc;
}

int msm_camera_flash_pmic(
	struct msm_camera_sensor_flash_pmic *pmic,
	unsigned led_state)
{
	int rc = 0;
	switch (led_state) {
	case MSM_CAMERA_LED_OFF:
		rc = pmic_flash_led_set_current(0);
		break;

	case MSM_CAMERA_LED_LOW:
		rc = pmic_flash_led_set_current(pmic->low_current);
		break;

	case MSM_CAMERA_LED_HIGH:
		rc = pmic_flash_led_set_current(pmic->high_current);
		break;

	default:
		rc = -EFAULT;
		break;
	}

	CDBG("flash_set_led_state: return %d\n", rc);

	return rc;
}

int32_t msm_camera_flash_set_led_state(
	struct msm_camera_sensor_flash_data *fdata, unsigned led_state)
{
	int32_t rc;

	CDBG("flash_set_led_state: %d flash_sr_type=%d\n", led_state,
	    fdata->flash_src->flash_sr_type);

	if (fdata->flash_type != MSM_CAMERA_FLASH_LED)
		return -ENODEV;

	switch (fdata->flash_src->flash_sr_type) {
	case MSM_CAMERA_FLASH_SRC_PMIC:
		rc = msm_camera_flash_pmic(&fdata->flash_src->_fsrc.pmic_src,
			led_state);
		break;

	case MSM_CAMERA_FLASH_SRC_PWM:
		rc = msm_camera_flash_pwm(&fdata->flash_src->_fsrc.pwm_src,
			led_state);
		break;

	default:
		rc = -ENODEV;
		break;
	}

	return rc;
}
#endif 
