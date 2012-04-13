/* arch/arm/mach-msm/qdsp5/snd.c
 *
 * interface to "snd" service on the baseband cpu
 *
 * Copyright (C) 2008 HTC Corporation
 * Copyright (c) 2009, Code Aurora Forum. All rights reserved.
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
 */

/*===========================================================================
when       who       what, where, why                        comment tag
--------   ----    -------------------------------------    ----------------------------------
2011--3-2  chenjun  fix CRDB00616634  ZTE_Audio_CJ_110302
2010-08-31 yangxs  modify for fm to save power 				ZTE_FM_YXS_20100831
2010-5-17 chenjun  fix CRDB00480912                         ZTE_Audio_CJ_100511
2010-1-25   chenjun  add snd debug info                       ZTE_Audio_DEBUG_CJ_100125
2009-12-17 chenjun modify for snd debug                     ZTE_Audio_DEBUG_CJ_091217
2009-11-8  chenjun  add audio loopback                       ZTE_Audio-LB_CJ_1
2009-11-10 chenjun  add snd debug info                       ZTE_Audio-DEBUG_CJ_1
===========================================================================*/

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/msm_audio.h>
#include <linux/seq_file.h>
#include <asm/atomic.h>
#include <asm/ioctls.h>
#include <mach/board.h>
#include <mach/msm_rpcrouter.h>
#include <mach/debug_mm.h>
/* ZTE_Audio_CJ_100302, chenjun, 2010-3-2, start */
// 2010-3-2:this file merged from 4735
/* ZTE_Audio-DEBUG_CJ_1, chenjun, 2009-11-6, start */
#include <linux/switch.h>
/* ZTE_Audio-DEBUG_CJ_1, chenjun, 2009-11-6, end */
/* ZTE_Audio_CJ_100302, chenjun, 2010-3-2, end */

struct snd_ctxt {
	struct mutex lock;
	int opened;
	struct msm_rpc_endpoint *ept;
	struct msm_snd_endpoints *snd_epts;

/* ZTE_Audio-DEBUG_CJ_1, chenjun, 2009-11-6, start */
	struct switch_dev snd_dev_info;
/* ZTE_Audio-DEBUG_CJ_1, chenjun, 2009-11-6, end */
};

struct snd_sys_ctxt {
	struct mutex lock;
	struct msm_rpc_endpoint *ept;
};

static struct snd_sys_ctxt the_snd_sys;

static struct snd_ctxt the_snd;

/* ZTE_Audio-DEBUG_CJ_1, chenjun, 2009-11-6, start */
#define SND_DEV 0
#define EAR_MUTE 1
#define MIC_MUTE 2
#define SND_DEV_INFO_NUM 3
static int keep_snd_dev_info[SND_DEV_INFO_NUM] = {0, 0, 0};
/* ZTE_Audio-DEBUG_CJ_1, chenjun, 2009-11-6, end */

#define RPC_SND_PROG	0x30000002
#define RPC_SND_CB_PROG	0x31000002

#define RPC_SND_VERS                    0x00020001

#define SND_SET_DEVICE_PROC 2
#define SND_SET_VOLUME_PROC 3
#define SND_AVC_CTL_PROC 29
#define SND_AGC_CTL_PROC 30
/* ZTE_Audio-LB_CJ_1 chenjun 2009-10-28 start */
  #define SND_AUDIO_LOOPBACK_PROC 37
/* ZTE_Audio-LB_CJ_1 chenjun 2009-10-28 end */
/* ZTE_Audio_CJ_100511, chenjun, 2010-5-11, start */
  #define SND_HPH_AMP_CTL_PROC 38
/* ZTE_Audio_CJ_100511, chenjun, 2010-5-11, end */

/* ZTE_FM_YXS_20100831 start */
#define SND_DEVICE_FM_STEREO_HEADSET_ZTE	32
#define SND_SET_FM_HEADSET_DEVICE_PROC    41
/* ZTE_FM_YXS_20100831 end */

struct rpc_snd_set_device_args {
	uint32_t device;
	uint32_t ear_mute;
	uint32_t mic_mute;

	uint32_t cb_func;
	uint32_t client_data;
};

struct rpc_snd_set_volume_args {
	uint32_t device;
	uint32_t method;
	uint32_t volume;

	uint32_t cb_func;
	uint32_t client_data;
};

struct rpc_snd_avc_ctl_args {
	uint32_t avc_ctl;
	uint32_t cb_func;
	uint32_t client_data;
};

struct rpc_snd_agc_ctl_args {
	uint32_t agc_ctl;
	uint32_t cb_func;
	uint32_t client_data;
};


/* ZTE_Audio-LB_CJ_1 chenjun 2009-10-28 start */
struct rpc_snd_lb_ctl_args {
	uint32_t lb_ctl;
	uint32_t cb_func;
	uint32_t client_data;
};
/* ZTE_Audio-LB_CJ_1 chenjun 2009-10-28 end */

/* ZTE_Audio_CJ_100511, chenjun, 2010-5-11, start */
struct rpc_snd_hph_amp_ctl_args {
	uint32_t amp_ctl;
	uint32_t cb_func;
	uint32_t client_data;
};
/* ZTE_Audio_CJ_100511, chenjun, 2010-5-11, end */

struct snd_set_device_msg {
	struct rpc_request_hdr hdr;
	struct rpc_snd_set_device_args args;
};

struct snd_set_volume_msg {
	struct rpc_request_hdr hdr;
	struct rpc_snd_set_volume_args args;
};

struct snd_avc_ctl_msg {
	struct rpc_request_hdr hdr;
	struct rpc_snd_avc_ctl_args args;
};

struct snd_agc_ctl_msg {
	struct rpc_request_hdr hdr;
	struct rpc_snd_agc_ctl_args args;
};


/* ZTE_Audio-LB_CJ_1 chenjun 2009-10-28 start */
struct snd_set_lb_msg {
	struct rpc_request_hdr hdr;
	struct rpc_snd_lb_ctl_args args;
};
/* ZTE_Audio-LB_CJ_1 chenjun 2009-10-28 end */

/* ZTE_Audio_CJ_100511, chenjun, 2010-5-11, start */
struct snd_hph_amp_msg {
	struct rpc_request_hdr hdr;
	struct rpc_snd_hph_amp_ctl_args args;
};
/* ZTE_Audio_CJ_100511, chenjun, 2010-5-11, end */

struct snd_endpoint *get_snd_endpoints(int *size);

static inline int check_mute(int mute)
{
	return (mute == SND_MUTE_MUTED ||
		mute == SND_MUTE_UNMUTED) ? 0 : -EINVAL;
}

static int get_endpoint(struct snd_ctxt *snd, unsigned long arg)
{
	int rc = 0, index;
	struct msm_snd_endpoint ept;

	if (copy_from_user(&ept, (void __user *)arg, sizeof(ept))) {
		MM_ERR("snd_ioctl get endpoint: invalid read pointer\n");
		return -EFAULT;
	}

	index = ept.id;
	if (index < 0 || index >= snd->snd_epts->num) {
		MM_ERR("snd_ioctl get endpoint: invalid index!\n");
		return -EINVAL;
	}

	ept.id = snd->snd_epts->endpoints[index].id;
	strncpy(ept.name,
		snd->snd_epts->endpoints[index].name,
		sizeof(ept.name));

	if (copy_to_user((void __user *)arg, &ept, sizeof(ept))) {
		MM_ERR("snd_ioctl get endpoint: invalid write pointer\n");
		rc = -EFAULT;
	}

	return rc;
}

static long snd_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct snd_set_device_msg dmsg;
	struct snd_set_volume_msg vmsg;
	struct snd_avc_ctl_msg avc_msg;
	struct snd_agc_ctl_msg agc_msg;
/* ZTE_Audio-LB_CJ_1 chenjun 2009-10-28 start */
	struct snd_set_lb_msg lb_msg;
/* ZTE_Audio-LB_CJ_1 chenjun 2009-10-28 end */

	struct msm_snd_device_config dev;
	struct msm_snd_volume_config vol;
	struct snd_ctxt *snd = file->private_data;
	int rc = 0;

	uint32_t avc, agc;
/* ZTE_Audio-LB_CJ_1 chenjun 2009-10-28 start */
	uint32_t set_lb;
/* ZTE_Audio-LB_CJ_1 chenjun 2009-10-28 end */

	mutex_lock(&snd->lock);
	switch (cmd) {
	case SND_SET_DEVICE:
		if (copy_from_user(&dev, (void __user *) arg, sizeof(dev))) {
			MM_ERR("set device: invalid pointer\n");
			rc = -EFAULT;
			break;
		}

		dmsg.args.device = cpu_to_be32(dev.device);
		dmsg.args.ear_mute = cpu_to_be32(dev.ear_mute);
		dmsg.args.mic_mute = cpu_to_be32(dev.mic_mute);
		if (check_mute(dev.ear_mute) < 0 ||
				check_mute(dev.mic_mute) < 0) {
			MM_ERR("set device: invalid mute status\n");
			rc = -EINVAL;
			break;
		}
		dmsg.args.cb_func = -1;
		dmsg.args.client_data = 0;

		MM_INFO("snd_set_device %d %d %d\n", dev.device,
				dev.ear_mute, dev.mic_mute);
		
		/* ZTE_FM_YXS_20100831 start */
		#if 0
		rc = msm_rpc_call(snd->ept,
			SND_SET_DEVICE_PROC,
			&dmsg, sizeof(dmsg), 5 * HZ);
		
		#else
		if(SND_DEVICE_FM_STEREO_HEADSET_ZTE == dev.device)
		{
			MM_ERR("[CHYL] set the headset for fm\n");
			rc = msm_rpc_call(snd->ept,
				SND_SET_FM_HEADSET_DEVICE_PROC,
				&dmsg, sizeof(dmsg), 5 * HZ);
		}
		else	
		{
		rc = msm_rpc_call(snd->ept,
			SND_SET_DEVICE_PROC,
			&dmsg, sizeof(dmsg), 5 * HZ);
        }
		#endif
		/* ZTE_FM_YXS_20100831 end */
/* ZTE_Audio_DEBUG_CJ_091217, chenjun, 2009-12-17, start */
/* ZTE_Audio-DEBUG_CJ_1, chenjun, 2009-11-6, start */
              if (dev.device != 28)
              {
		    keep_snd_dev_info[SND_DEV] = dev.device;
              }
		keep_snd_dev_info[EAR_MUTE] = dev.ear_mute;
		keep_snd_dev_info[MIC_MUTE] = dev.mic_mute;
/* ZTE_Audio-DEBUG_CJ_1, chenjun, 2009-11-6, end */
/* ZTE_Audio_DEBUG_CJ_091217, chenjun, 2009-12-17, end */
		break;

	case SND_SET_VOLUME:
		if (copy_from_user(&vol, (void __user *) arg, sizeof(vol))) {
			MM_ERR("set volume: invalid pointer\n");
			rc = -EFAULT;
			break;
		}

		vmsg.args.device = cpu_to_be32(vol.device);
		vmsg.args.method = cpu_to_be32(vol.method);
		if (vol.method != SND_METHOD_VOICE) {
			MM_ERR("set volume: invalid method\n");
			rc = -EINVAL;
			break;
		}

		vmsg.args.volume = cpu_to_be32(vol.volume);
		vmsg.args.cb_func = -1;
		vmsg.args.client_data = 0;

		MM_INFO("snd_set_volume %d %d %d\n", vol.device,
				vol.method, vol.volume);

		rc = msm_rpc_call(snd->ept,
			SND_SET_VOLUME_PROC,
			&vmsg, sizeof(vmsg), 5 * HZ);
		break;

	case SND_AVC_CTL:
		if (get_user(avc, (uint32_t __user *) arg)) {
			rc = -EFAULT;
			break;
		} else if ((avc != 1) && (avc != 0)) {
			rc = -EINVAL;
			break;
		}

		avc_msg.args.avc_ctl = cpu_to_be32(avc);
		avc_msg.args.cb_func = -1;
		avc_msg.args.client_data = 0;

		MM_INFO("snd_avc_ctl %d\n", avc);

		rc = msm_rpc_call(snd->ept,
			SND_AVC_CTL_PROC,
			&avc_msg, sizeof(avc_msg), 5 * HZ);
		break;

	case SND_AGC_CTL:
		if (get_user(agc, (uint32_t __user *) arg)) {
			rc = -EFAULT;
			break;
		} else if ((agc != 1) && (agc != 0)) {
			rc = -EINVAL;
			break;
		}
		agc_msg.args.agc_ctl = cpu_to_be32(agc);
		agc_msg.args.cb_func = -1;
		agc_msg.args.client_data = 0;

		MM_INFO("snd_agc_ctl %d\n", agc);

		rc = msm_rpc_call(snd->ept,
			SND_AGC_CTL_PROC,
			&agc_msg, sizeof(agc_msg), 5 * HZ);
		break;

	case SND_GET_NUM_ENDPOINTS:
		if (copy_to_user((void __user *)arg,
				&snd->snd_epts->num, sizeof(unsigned))) {
			MM_ERR("get endpoint: invalid pointer\n");
			rc = -EFAULT;
		}
		break;

	case SND_GET_ENDPOINT:
		rc = get_endpoint(snd, arg);
		break;


/* ZTE_Audio-LB_CJ_1 chenjun 2009-10-28 start */
	case SND_SET_AUDIO_LOOPBACK:
		if (get_user(set_lb, (uint32_t __user *) arg)) {
			rc = -EFAULT;
			break;
		} else if ((set_lb != 1) && (set_lb != 0)) {
			rc = -EINVAL;
			break;
		}

		lb_msg.args.lb_ctl = cpu_to_be32(set_lb);

		lb_msg.args.cb_func = -1;
		lb_msg.args.client_data = 0;

		pr_info("snd_lb_ctl %d\n", set_lb);

		rc = msm_rpc_call(snd->ept,
			SND_AUDIO_LOOPBACK_PROC,
			&lb_msg, sizeof(lb_msg), 5 * HZ);
		break;
/* ZTE_Audio-LB_CJ_1 chenjun 2009-10-28 end */

	default:
		MM_ERR("unknown command\n");
		rc = -EINVAL;
		break;
	}
	mutex_unlock(&snd->lock);

/* ZTE_Audio_DEBUG_CJ_100125, chenjun, 2010-1-25, start */
		MM_INFO("chenjun:rc = %d\n", rc);
/* ZTE_Audio_DEBUG_CJ_100125, chenjun, 2010-1-25, end */

	return rc;
}

static int snd_release(struct inode *inode, struct file *file)
{
	struct snd_ctxt *snd = file->private_data;
	int rc;

	mutex_lock(&snd->lock);
	rc = msm_rpc_close(snd->ept);
	if (rc < 0)
		MM_ERR("msm_rpc_close failed\n");
	snd->ept = NULL;
	snd->opened = 0;
	mutex_unlock(&snd->lock);
	return 0;
}
static int snd_sys_release(void)
{
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	int rc = 0;

	mutex_lock(&snd_sys->lock);
	rc = msm_rpc_close(snd_sys->ept);
	if (rc < 0)
		MM_ERR("msm_rpc_close failed\n");
	snd_sys->ept = NULL;
	mutex_unlock(&snd_sys->lock);
	return rc;
}
static int snd_open(struct inode *inode, struct file *file)
{
	struct snd_ctxt *snd = &the_snd;
	int rc = 0;

	mutex_lock(&snd->lock);
	if (snd->opened == 0) {
		if (snd->ept == NULL) {
			snd->ept = msm_rpc_connect_compatible(RPC_SND_PROG,
					RPC_SND_VERS, 0);
			if (IS_ERR(snd->ept)) {
				rc = PTR_ERR(snd->ept);
				snd->ept = NULL;
				MM_ERR("failed to connect snd svc\n");
				goto err;
			}
		}
		file->private_data = snd;
		snd->opened = 1;
	} else {
		MM_ERR("snd already opened\n");
/* ZTE_Audio_CJ_110302, chenjun, 2011-03-02, start */
#if 0
		rc = -EBUSY;
#else
		if (snd->ept == NULL) {
			snd->ept = msm_rpc_connect_compatible(RPC_SND_PROG,
					RPC_SND_VERS, 0);
			if (IS_ERR(snd->ept)) {
				rc = PTR_ERR(snd->ept);
				snd->ept = NULL;
				MM_ERR("chenjun:failed to connect snd svc\n");
				goto err;
			}
		}
		file->private_data = snd;
		snd->opened = 1;
		MM_ERR("chenjun:reuse opened-snd\n");
#endif
/* ZTE_Audio_CJ_110302, chenjun, 2011-03-02, end */
	}

err:
	mutex_unlock(&snd->lock);
	return rc;
}
static int snd_sys_open(void)
{
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	int rc = 0;

	mutex_lock(&snd_sys->lock);
	if (snd_sys->ept == NULL) {
		snd_sys->ept = msm_rpc_connect_compatible(RPC_SND_PROG,
			RPC_SND_VERS, 0);
		if (IS_ERR(snd_sys->ept)) {
			rc = PTR_ERR(snd_sys->ept);
			snd_sys->ept = NULL;
			MM_ERR("failed to connect snd svc\n");
			goto err;
		}
	} else
		MM_DBG("snd already opened\n");

err:
	mutex_unlock(&snd_sys->lock);
	return rc;
}

static struct file_operations snd_fops = {
	.owner		= THIS_MODULE,
	.open		= snd_open,
	.release	= snd_release,
	.unlocked_ioctl	= snd_ioctl,
};

struct miscdevice snd_misc = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "msm_snd",
	.fops	= &snd_fops,
};

static long snd_agc_enable(unsigned long arg)
{
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	struct snd_agc_ctl_msg agc_msg;
	int rc = 0;

	if ((arg != 1) && (arg != 0))
		return -EINVAL;

	agc_msg.args.agc_ctl = cpu_to_be32(arg);
	agc_msg.args.cb_func = -1;
	agc_msg.args.client_data = 0;

	MM_DBG("snd_agc_ctl %ld,%d\n", arg, agc_msg.args.agc_ctl);

	rc = msm_rpc_call(snd_sys->ept,
		SND_AGC_CTL_PROC,
		&agc_msg, sizeof(agc_msg), 5 * HZ);
	return rc;
}

static long snd_avc_enable(unsigned long arg)
{
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	struct snd_avc_ctl_msg avc_msg;
	int rc = 0;

	if ((arg != 1) && (arg != 0))
		return -EINVAL;

	avc_msg.args.avc_ctl = cpu_to_be32(arg);

	avc_msg.args.cb_func = -1;
	avc_msg.args.client_data = 0;

	MM_DBG("snd_avc_ctl %ld,%d\n", arg, avc_msg.args.avc_ctl);

	rc = msm_rpc_call(snd_sys->ept,
		SND_AVC_CTL_PROC,
		&avc_msg, sizeof(avc_msg), 5 * HZ);
	return rc;
}

static ssize_t snd_agc_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t status;
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	int rc = 0;

	rc = snd_sys_open();
	if (rc)
		return rc;

	mutex_lock(&snd_sys->lock);

	if (sysfs_streq(buf, "enable"))
		status = snd_agc_enable(1);
	else if (sysfs_streq(buf, "disable"))
		status = snd_agc_enable(0);
	else
		status = -EINVAL;

	mutex_unlock(&snd_sys->lock);
	rc = snd_sys_release();
	if (rc)
		return rc;

	return status ? : size;
}

static ssize_t snd_avc_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t status;
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	int rc = 0;

	rc = snd_sys_open();
	if (rc)
		return rc;

	mutex_lock(&snd_sys->lock);

	if (sysfs_streq(buf, "enable"))
		status = snd_avc_enable(1);
	else if (sysfs_streq(buf, "disable"))
		status = snd_avc_enable(0);
	else
		status = -EINVAL;

	mutex_unlock(&snd_sys->lock);
	rc = snd_sys_release();
	if (rc)
		return rc;

	return status ? : size;
}

static long snd_vol_enable(const char *arg)
{
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	struct snd_set_volume_msg vmsg;
	struct msm_snd_volume_config vol;
	int rc = 0;

	rc = sscanf(arg, "%d %d %d", &vol.device, &vol.method, &vol.volume);
	if (rc != 3) {
		MM_ERR("Invalid arguments. Usage: <device> <method> \
				<volume>\n");
		rc = -EINVAL;
		return rc;
	}

	vmsg.args.device = cpu_to_be32(vol.device);
	vmsg.args.method = cpu_to_be32(vol.method);
	if (vol.method != SND_METHOD_VOICE) {
		MM_ERR("snd_ioctl set volume: invalid method\n");
		rc = -EINVAL;
		return rc;
	}

	vmsg.args.volume = cpu_to_be32(vol.volume);
	vmsg.args.cb_func = -1;
	vmsg.args.client_data = 0;

	MM_DBG("snd_set_volume %d %d %d\n", vol.device, vol.method,
			vol.volume);

	rc = msm_rpc_call(snd_sys->ept,
		SND_SET_VOLUME_PROC,
		&vmsg, sizeof(vmsg), 5 * HZ);
	return rc;
}

static long snd_dev_enable(const char *arg)
{
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	struct snd_set_device_msg dmsg;
	struct msm_snd_device_config dev;
	int rc = 0;

	rc = sscanf(arg, "%d %d %d", &dev.device, &dev.ear_mute, &dev.mic_mute);
	if (rc != 3) {
		MM_ERR("Invalid arguments. Usage: <device> <ear_mute> \
				<mic_mute>\n");
		rc = -EINVAL;
		return rc;
	}
	dmsg.args.device = cpu_to_be32(dev.device);
	dmsg.args.ear_mute = cpu_to_be32(dev.ear_mute);
	dmsg.args.mic_mute = cpu_to_be32(dev.mic_mute);
	if (check_mute(dev.ear_mute) < 0 ||
			check_mute(dev.mic_mute) < 0) {
		MM_ERR("snd_ioctl set device: invalid mute status\n");
		rc = -EINVAL;
		return rc;
	}
	dmsg.args.cb_func = -1;
	dmsg.args.client_data = 0;

	MM_INFO("snd_set_device %d %d %d\n", dev.device, dev.ear_mute,
			dev.mic_mute);

	rc = msm_rpc_call(snd_sys->ept,
		SND_SET_DEVICE_PROC,
		&dmsg, sizeof(dmsg), 5 * HZ);
	return rc;
}

static ssize_t snd_dev_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t status;
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	int rc = 0;

	rc = snd_sys_open();
	if (rc)
		return rc;

	mutex_lock(&snd_sys->lock);
	status = snd_dev_enable(buf);
	mutex_unlock(&snd_sys->lock);

	rc = snd_sys_release();
	if (rc)
		return rc;

	return status ? : size;
}

static ssize_t snd_vol_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t status;
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	int rc = 0;

	rc = snd_sys_open();
	if (rc)
		return rc;

	mutex_lock(&snd_sys->lock);
	status = snd_vol_enable(buf);
	mutex_unlock(&snd_sys->lock);

	rc = snd_sys_release();
	if (rc)
		return rc;

	return status ? : size;
}

/* ZTE_Audio_CJ_100511, chenjun, 2010-5-11, start */
int snd_hph_amp_ctl(uint32_t on)
{
	struct snd_ctxt *snd = &the_snd;
	struct snd_hph_amp_msg hph_amp_msg;
	int rc = 0;

	pr_info("snd_hph_amp_ctl:%d\n", on);

	mutex_lock(&snd->lock);
    
	hph_amp_msg.args.amp_ctl = cpu_to_be32(on);
	hph_amp_msg.args.cb_func = -1;
	hph_amp_msg.args.client_data = 0;

	rc = msm_rpc_call(snd->ept,
			SND_HPH_AMP_CTL_PROC,
			&hph_amp_msg, sizeof(hph_amp_msg), 5 * HZ);

	mutex_unlock(&snd->lock);
    
	return rc;
}
/* ZTE_Audio_CJ_100511, chenjun, 2010-5-11, end */

static DEVICE_ATTR(agc, S_IWUSR | S_IRUGO,
		NULL, snd_agc_store);

static DEVICE_ATTR(avc, S_IWUSR | S_IRUGO,
		NULL, snd_avc_store);

static DEVICE_ATTR(device, S_IWUSR | S_IRUGO,
		NULL, snd_dev_store);

static DEVICE_ATTR(volume, S_IWUSR | S_IRUGO,
		NULL, snd_vol_store);

/* ZTE_Audio-DEBUG_CJ_1, chenjun, 2009-11-6, start */
static ssize_t print_snd_dev_name(struct switch_dev *sdev, char *buf)
{
	return sprintf(buf, "%s\n", "dev,ear_mute,mic_mute");
}

static ssize_t print_snd_dev_state(struct switch_dev *sdev, char *buf)
{
	return sprintf(buf, "%d,%d,%d\n", keep_snd_dev_info[SND_DEV], keep_snd_dev_info[EAR_MUTE], keep_snd_dev_info[MIC_MUTE]);
}
/* ZTE_Audio-DEBUG_CJ_1, chenjun, 2009-11-6, end */

static int snd_probe(struct platform_device *pdev)
{
	struct snd_ctxt *snd = &the_snd;
	struct snd_sys_ctxt *snd_sys = &the_snd_sys;
	int rc = 0;

	mutex_init(&snd->lock);
	mutex_init(&snd_sys->lock);
	snd_sys->ept = NULL;
	snd->snd_epts = (struct msm_snd_endpoints *)pdev->dev.platform_data;
	rc = misc_register(&snd_misc);
	if (rc)
		return rc;

	rc = device_create_file(snd_misc.this_device, &dev_attr_agc);
	if (rc) {
		misc_deregister(&snd_misc);
		return rc;
	}

	rc = device_create_file(snd_misc.this_device, &dev_attr_avc);
	if (rc) {
		device_remove_file(snd_misc.this_device,
						&dev_attr_agc);
		misc_deregister(&snd_misc);
		return rc;
	}

	rc = device_create_file(snd_misc.this_device, &dev_attr_device);
	if (rc) {
		device_remove_file(snd_misc.this_device,
						&dev_attr_agc);
		device_remove_file(snd_misc.this_device,
						&dev_attr_avc);
		misc_deregister(&snd_misc);
		return rc;
	}

	rc = device_create_file(snd_misc.this_device, &dev_attr_volume);
	if (rc) {
		device_remove_file(snd_misc.this_device,
						&dev_attr_agc);
		device_remove_file(snd_misc.this_device,
						&dev_attr_avc);
		device_remove_file(snd_misc.this_device,
						&dev_attr_device);
		misc_deregister(&snd_misc);
	}

/* ZTE_Audio-DEBUG_CJ_1, chenjun, 2009-11-6, start */
	snd->snd_dev_info.name = "snd_debug";
	snd->snd_dev_info.print_name = print_snd_dev_name;
	snd->snd_dev_info.print_state = print_snd_dev_state;
	rc = switch_dev_register(&snd->snd_dev_info);
	if (rc < 0)
		switch_dev_unregister(&snd->snd_dev_info);
/* ZTE_Audio-DEBUG_CJ_1, chenjun, 2009-11-6, end */

	return rc;
}

static struct platform_driver snd_plat_driver = {
	.probe = snd_probe,
	.driver = {
		.name = "msm_snd",
		.owner = THIS_MODULE,
	},
};

static int __init snd_init(void)
{
	return platform_driver_register(&snd_plat_driver);
}

module_init(snd_init);
