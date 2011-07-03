/* linux/sound/soc/msm/qsd8k.c
 *
 * Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * All source code in this file is licensed under the following license except
 * where indicated.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can find it at http://www.fsf.org.
 */

#include <linux/init.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/time.h>
#include <linux/wait.h>
#include <linux/platform_device.h>
#include <sound/core.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/pcm.h>
#include <sound/initval.h>
#include <sound/control.h>
#include <asm/dma.h>
#include <linux/dma-mapping.h>

#include "qsd-pcm.h"

static struct platform_device *qsd_audio_snd_device;

static int snd_qsd_route_info(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1; /* Device */
	uinfo->value.integer.min = (int)CAD_HW_DEVICE_ID_HANDSET_MIC;
	uinfo->value.integer.max = (int)CAD_HW_DEVICE_ID_MAX_NUM;
	return 0;
}

static int snd_qsd_route_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] =
			(uint32_t) qsd_glb_ctl.device;
	return 0;
}

static int snd_qsd_route_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	int rc = 0;
	int device;

	device = ucontrol->value.integer.value[0];

	rc = audio_switch_device(device);
	if (rc < 0) {
		printk(KERN_ERR "audio_switch_device  failed\n");
		return rc;
	}

	qsd_glb_ctl.device = device;

	return 0;
}

static int snd_vol_info(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1; /* Volume */
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 100;
	return 0;
}

static int snd_rx_vol_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = (uint32_t) qsd_glb_ctl.rx_volume;
	return 0;
}

static int snd_rx_vol_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct msm_vol_info vi;
	int rc = 0;

	vi.vol = ucontrol->value.integer.value[0];
	vi.path = CAD_RX_DEVICE;

	rc = audio_set_device_volume_path(&vi);

	if (rc)
		printk(KERN_ERR "audio_set_device_volume failed\n");
	else
		qsd_glb_ctl.rx_volume = ucontrol->value.integer.value[0];

	return rc;
}

static int snd_tx_vol_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = (uint32_t) qsd_glb_ctl.tx_volume;
	return 0;
}

static int snd_tx_vol_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct msm_vol_info vi;
	int rc = 0;

	vi.vol = ucontrol->value.integer.value[0];
	vi.path = CAD_TX_DEVICE;

	rc = audio_set_device_volume_path(&vi);

	if (rc)
		printk(KERN_ERR "audio_set_device_volume failed\n");
	else
		qsd_glb_ctl.tx_volume = ucontrol->value.integer.value[0];

	return rc;
}

static int snd_tx_mute_info(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1; /* MUTE */
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 1;
	return 0;
}

static int snd_tx_mute_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = (uint32_t) qsd_glb_ctl.tx_mute;
	return 0;
}

static int snd_tx_mute_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	int rc = 0;
	struct msm_mute_info m;

	m.path = CAD_TX_DEVICE;
	m.mute = ucontrol->value.integer.value[0];

	rc = audio_set_device_mute(&m);
	if (rc)
		printk(KERN_ERR "Capture device mute failed\n");
	else
		qsd_glb_ctl.tx_mute = ucontrol->value.integer.value[0];
	return rc;
}

static int snd_rx_mute_info(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1; /* MUTE */
	uinfo->value.integer.min = 0;
	uinfo->value.integer.max = 1;
	return 0;
}

static int snd_rx_mute_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = (uint32_t) qsd_glb_ctl.rx_mute;
	return 0;
}

static int snd_rx_mute_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	int rc = 0;
	struct msm_mute_info m;

	m.path = CAD_RX_DEVICE;
	m.mute = ucontrol->value.integer.value[0];

	rc = audio_set_device_mute(&m);
	if (rc)
		printk(KERN_ERR "Playback device mute failed\n");
	else
		qsd_glb_ctl.rx_mute = ucontrol->value.integer.value[0];
	return rc;
}

static int snd_strm_vol_info(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_info *uinfo)
{
	uinfo->type = SNDRV_CTL_ELEM_TYPE_INTEGER;
	uinfo->count = 1; /* Volume Param, in gain */
	uinfo->value.integer.min = CAD_STREAM_MIN_GAIN;
	uinfo->value.integer.max = CAD_STREAM_MAX_GAIN;
	return 0;
}

static int snd_strm_vol_get(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = qsd_glb_ctl.strm_volume;
	return 0;
}

static int snd_strm_vol_put(struct snd_kcontrol *kcontrol,
				struct snd_ctl_elem_value *ucontrol)
{
	int change;
	int volume;

	if (ucontrol->value.integer.value[0] > CAD_STREAM_MAX_GAIN)
		ucontrol->value.integer.value[0] = CAD_STREAM_MAX_GAIN;
	if (ucontrol->value.integer.value[0] < CAD_STREAM_MIN_GAIN)
		ucontrol->value.integer.value[0] = CAD_STREAM_MIN_GAIN;

	volume = ucontrol->value.integer.value[0];
	change = (qsd_glb_ctl.strm_volume != volume);
	mutex_lock(&the_locks.mixer_lock);
	if (change) {
		qsd_glb_ctl.strm_volume = volume;
		qsd_glb_ctl.update = 1;
	}
	mutex_unlock(&the_locks.mixer_lock);
	return 0;
}

#define QSD_EXT(xname, xindex, fp_info, fp_get, fp_put, addr) \
{ .iface = SNDRV_CTL_ELEM_IFACE_MIXER, \
  .access = SNDRV_CTL_ELEM_ACCESS_READWRITE, \
  .name = xname, .index = xindex, \
  .info = fp_info,\
  .get = fp_get, .put = fp_put, \
  .private_value = addr, \
}

static struct snd_kcontrol_new snd_qsd_controls[] = {
	QSD_EXT("Master Route", 1, snd_qsd_route_info, \
			 snd_qsd_route_get, snd_qsd_route_put, 0),
	QSD_EXT("Master Volume Playback", 2, snd_vol_info, \
			 snd_rx_vol_get, snd_rx_vol_put, 0),
	QSD_EXT("Master Volume Capture", 3, snd_vol_info, \
			 snd_tx_vol_get, snd_tx_vol_put, 0),
	QSD_EXT("Master Mute Playback", 4, snd_rx_mute_info, \
			 snd_rx_mute_get, snd_rx_mute_put, 0),
	QSD_EXT("Master Mute Capture", 5, snd_tx_mute_info, \
			 snd_tx_mute_get, snd_tx_mute_put, 0),
	QSD_EXT("Stream Volume", 6, snd_strm_vol_info, \
			 snd_strm_vol_get, snd_strm_vol_put, 0),
};

static int qsd_new_mixer(struct snd_card *card)
{
	unsigned int idx;
	int err;

	strcpy(card->mixername, "MSM Mixer");
	for (idx = 0; idx < ARRAY_SIZE(snd_qsd_controls); idx++) {
		err = snd_ctl_add(card,
				snd_ctl_new1(&snd_qsd_controls[idx], NULL));
		if (err < 0)
			return err;
	}
	return 0;
}

static int qsd_soc_dai_init(struct snd_soc_codec *codec)
{

	int ret = 0;
	ret = qsd_new_mixer(codec->card);
	if (ret < 0) {
		pr_err("msm_soc: ALSA MSM Mixer Fail\n");
	}

	return ret;
}

static struct snd_soc_dai_link qsd_dai = {
	.name = "ASOC",
	.stream_name = "ASOC",
	.codec_dai = &msm_dais[0],
	.cpu_dai = &msm_dais[1],
	.init   = qsd_soc_dai_init,
};

struct snd_soc_card snd_soc_card_qsd = {
	.name 		= "qsd-audio",
	.dai_link	= &qsd_dai,
	.num_links 	= 1,
	.platform = &qsd_soc_platform,
};

/* qsd_audio audio subsystem */
static struct snd_soc_device qsd_audio_snd_devdata = {
	.card = &snd_soc_card_qsd,
	.codec_dev = &soc_codec_dev_msm,
};

static int __init qsd_audio_init(void)
{
	int ret;

	qsd_audio_snd_device = platform_device_alloc("soc-audio", -1);
	if (!qsd_audio_snd_device)
		return -ENOMEM;

	platform_set_drvdata(qsd_audio_snd_device, &qsd_audio_snd_devdata);
	qsd_audio_snd_devdata.dev = &qsd_audio_snd_device->dev;
	ret = platform_device_add(qsd_audio_snd_device);
	if (ret) {
		platform_device_put(qsd_audio_snd_device);
		return ret;
	}
	mutex_init(&the_locks.mixer_lock);
	init_waitqueue_head(&the_locks.eos_wait);
	spin_lock_init(&the_locks.alsa_lock);

	qsd_glb_ctl.tx_volume = 100;
	qsd_glb_ctl.rx_volume = 100;
	qsd_glb_ctl.strm_volume = 100;
	qsd_glb_ctl.device = CAD_HW_DEVICE_ID_HANDSET_SPKR;
	qsd_glb_ctl.tx_mute = 0;
	qsd_glb_ctl.rx_mute = 0;
	qsd_glb_ctl.update = 0;

	return ret;
}

static void __exit qsd_audio_exit(void)
{
	kfree(qsd_audio_snd_devdata.codec_dev);
	platform_device_unregister(qsd_audio_snd_device);
}

module_init(qsd_audio_init);
module_exit(qsd_audio_exit);

MODULE_DESCRIPTION("PCM module");
MODULE_LICENSE("GPL v2");
