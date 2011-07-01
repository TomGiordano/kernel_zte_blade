/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */
/*-----------------------------------------------------------------------------------------
  when         who          what, where, why                         comment tag
  --------     ----        -------------------------------------    ----------------------
  2010-06-05   ye.ganlin   add process for flash LED                YGL_CAM_20100605
------------------------------------------------------------------------------------------*/

#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/pwm.h>
#include <linux/pmic8058-pwm.h>
#include <mach/pmic.h>
#include <mach/camera.h>
#include <mach/gpio.h>

/*
 * Commented by YGL_CAM_20100605
 * Add process for flash LED
 * and for CONFIG_MACH_JOE ONLY
 */
#if defined(CONFIG_MSM_CAMERA_FLASH)

/*
 * Camera Flash LED GPIO setting according to ARM9 AMSS
 */
#if defined(CONFIG_MACH_JOE)
#define MSM_CAMERA_FLASH_LED_GPIO   (88)
#else
#define MSM_CAMERA_FLASH_LED_GPIO   (255)   // illegal value
#endif
/*
 * Flash LED Status
 * 1: enable Flash LED
 * 0: disable Flash LED
 */
static uint32_t flash_led_enable = 0;

/*
 * Flash LED GPIO Setting
 * 1: pull up GPIO
 * 0: pull down GPIO
 */
static int32_t msm_camera_flash_set_led_gpio(int32_t gpio_val)
{
    int32_t rc = -EFAULT;

    CDBG("%s: gpio_val=%d\n", __func__, gpio_val);

    rc = gpio_request(MSM_CAMERA_FLASH_LED_GPIO, "flashled-gpio");
    if (0 == rc)
    {
        /* ignore "rc" */
        rc = gpio_direction_output(MSM_CAMERA_FLASH_LED_GPIO, gpio_val);
    }
    gpio_free(MSM_CAMERA_FLASH_LED_GPIO);

    return rc;
}

/*
 * Refer to MSM_CAM_IOCTL_FLASH_LED_CFG used by mm-camera in user space
 * flash_led_enable is set in apps's menu item selected by user
 * 0: disable Flash LED
 * 1: enable Flash LED
 */
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
            /*
               * MSM_CAMERA_LED_LOW is as same as MSM_CAMERA_LED_HIGH
               */
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

/*
 * External Function
 */
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

/*
 * External Function
 */
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
#endif /* defined(CONFIG_MSM_CAMERA_FLASH) */
