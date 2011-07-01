/* Copyright (c) 2010, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of Code Aurora Forum, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef __PMIC8058_OTHC_H__
#define __PMIC8058_OTHC_H__

enum othc_headset_type {
	OTHC_HEADSET_NO,
	OTHC_HEADSET_NC,
};

/* Signal control for OTHC module */
enum othc_micbias_enable {
	/* Turn off MICBIAS signal */
	OTHC_SIGNAL_OFF,
	/* Turn on MICBIAS signal when TCXO is enabled */
	OTHC_SIGNAL_TCXO,
	/* Turn on MICBIAS signal when PWM is high or TCXO is enabled */
	OTHC_SIGNAL_PWM_TCXO,
	/* MICBIAS always enabled */
	OTHC_SIGNAL_ALWAYS_ON,
};

/* Number of MICBIAS lines supported by PMIC8058 */
enum othc_micbias {
	OTHC_MICBIAS_0,
	OTHC_MICBIAS_1,
	OTHC_MICBIAS_2,
	OTHC_MICBIAS_MAX,
};

enum othc_micbias_capability {
	/* MICBIAS used only for BIAS with on/off capability */
	OTHC_MICBIAS,
	/* MICBIAS used to support HSED functionality */
	OTHC_MICBIAS_HSED,
};

/* Configuration data for HSED */
struct othc_hsed_config {
	enum othc_headset_type othc_headset;
	u16 othc_lowcurr_thresh_uA;
	u16 othc_highcurr_thresh_uA;
	u32 othc_hyst_prediv_us;
	u32 othc_period_clkdiv_us;
	u32 othc_hyst_clk_us;
	u32 othc_period_clk_us;
	int othc_nc_gpio;
	int othc_wakeup;
	int (*othc_nc_gpio_setup)(void);
};

struct pmic8058_othc_config_pdata {
	enum othc_micbias micbias_select;
	enum othc_micbias_enable micbias_enable;
	enum othc_micbias_capability micbias_capability;
	struct othc_hsed_config *hsed_config;
};

int pm8058_micbias_enable(enum othc_micbias micbias,
			enum othc_micbias_enable enable);

#endif /* __PMIC8058_OTHC_H__ */
