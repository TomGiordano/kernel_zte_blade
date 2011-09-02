/* Copyright (c) 2010, Code Aurora Forum. All rights reserved.
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
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/msm_audio.h>
#include <linux/kthread.h>
#include <linux/completion.h>
#include <linux/wait.h>
#include <mach/qdsp6v2/audio_dev_ctl.h>
#include <mach/dal.h>
#include "q6voice.h"
#include "audio_acdb.h"

#define TIMEOUT_MS 3000
#define SNDDEV_CAP_TTY 0x20

#define BUFFER_PAYLOAD_SIZE 4000

struct voice_data {
	int voc_state;/*INIT, CHANGE, RELEASE, RUN */
	struct task_struct *task;

	wait_queue_head_t mvm_wait;
	wait_queue_head_t cvs_wait;
	wait_queue_head_t cvp_wait;

	uint32_t device_events;

	/* cache the values related to Rx and Tx */
	struct device_data dev_rx;
	struct device_data dev_tx;

	/* these default values are for all devices */
	uint32_t default_mute_val;
	uint32_t default_vol_val;
	uint32_t default_sample_val;

	/* call status */
	int v_call_status; /* Start or End */

	void *apr_mvm;
	void *apr_cvs;
	void *apr_cvp;

	u32 mvm_state;
	u32 cvs_state;
	u32 cvp_state;

	/* handle */
	u16 mvm_handle;
	u16 cvs_handle;
	u16 cvp_handle;
};

struct voice_data voice;

static void voice_auddev_cb_function(u32 evt_id,
			union auddev_evt_data *evt_payload,
			void *private_data);

static int32_t modem_mvm_callback(struct apr_client_data *data, void *priv);
static int32_t modem_cvs_callback(struct apr_client_data *data, void *priv);
static int32_t modem_cvp_callback(struct apr_client_data *data, void *priv);

static int voice_apr_register(struct voice_data *v)
{
	int rc = 0;

	pr_debug("into voice_apr_register_callback\n");
	/* register callback to APR */
	if (v->apr_mvm == NULL) {
		pr_debug("start to register MVM callback\n");
		v->apr_mvm = apr_register("MODEM", "MVM", modem_mvm_callback,
								0xFFFFFFFF, v);
		if (v->apr_mvm == NULL) {
			pr_err("Unable to register MVM\n");
			rc = -ENODEV;
			goto done;
		}
	}
	if (v->apr_cvs == NULL) {
		pr_debug("start to register CVS callback\n");
		v->apr_cvs = apr_register("MODEM", "CVS", modem_cvs_callback,
								0xFFFFFFFF, v);
		if (v->apr_cvs == NULL) {
			pr_err("Unable to register CVS\n");
			rc = -ENODEV;
			goto err;
		}
	}
	if (v->apr_cvp == NULL) {
		pr_debug("start to register CVP callback\n");
		v->apr_cvp = apr_register("MODEM", "CVP", modem_cvp_callback,
								0xFFFFFFFF, v);
		if (v->apr_cvp == NULL) {
			pr_err("Unable to register CVP\n");
			rc = -ENODEV;
			goto err1;
		}
	}
	return 0;

err1:
	apr_deregister(v->apr_cvs);
err:
	apr_deregister(v->apr_mvm);

done:
	return rc;

}

static int voice_create_mvm_cvs_session(struct voice_data *v)
{
	int ret = 0;
	struct mvm_create_passive_ctl_session_cmd mvm_session_cmd;
	struct cvs_create_passive_ctl_session_cmd cvs_session_cmd;

	/* start to ping if modem service is up */
	pr_debug("in voice_create_mvm_cvs_session, mvm_hdl=%d, cvs_hdl=%d\n",
					v->mvm_handle, v->cvs_handle);
	/* send cmd to create mvm session and wait for response */
	if (!v->mvm_handle) {
		mvm_session_cmd.hdr.hdr_field = APR_HDR_FIELD(
						APR_MSG_TYPE_SEQ_CMD,
					APR_HDR_LEN(APR_HDR_SIZE), APR_PKT_VER);
		mvm_session_cmd.hdr.pkt_size = APR_PKT_SIZE(APR_HDR_SIZE,
					sizeof(mvm_session_cmd) - APR_HDR_SIZE);
		pr_info("send mvm create session pkt size = %d\n",
					mvm_session_cmd.hdr.pkt_size);
		mvm_session_cmd.hdr.src_port = 0;
		mvm_session_cmd.hdr.dest_port = 0;
		mvm_session_cmd.hdr.token = 0;
		mvm_session_cmd.hdr.opcode =
				VSS_IMVM_CMD_CREATE_PASSIVE_CONTROL_SESSION;
		v->mvm_state = 1;
		ret = apr_send_pkt(v->apr_mvm, (uint32_t *) &mvm_session_cmd);
		if (ret < 0) {
			pr_err("Fail in sending MVM_CONTROL_SESSION\n");
			goto fail;
	}
		ret = wait_event_timeout(v->mvm_wait, (v->mvm_state == 0),
						msecs_to_jiffies(TIMEOUT_MS));
		if (!ret) {
			pr_err("%s: wait_event timeout\n", __func__);
			goto fail;
		}
	}

	/* send cmd to create cvs session */
	if (!v->cvs_handle) {
		cvs_session_cmd.hdr.hdr_field = APR_HDR_FIELD(
						APR_MSG_TYPE_SEQ_CMD,
					APR_HDR_LEN(APR_HDR_SIZE), APR_PKT_VER);
		cvs_session_cmd.hdr.pkt_size = APR_PKT_SIZE(APR_HDR_SIZE,
					sizeof(cvs_session_cmd) - APR_HDR_SIZE);
		pr_info("send stream create session pkt size = %d\n",
					cvs_session_cmd.hdr.pkt_size);
		cvs_session_cmd.hdr.src_port = 0;
		cvs_session_cmd.hdr.dest_port = 0;
		cvs_session_cmd.hdr.token = 0;
		cvs_session_cmd.hdr.opcode =
				VSS_ISTREAM_CMD_CREATE_PASSIVE_CONTROL_SESSION;
		strcpy(cvs_session_cmd.cvs_session.name, "default modem voice");

		v->cvs_state = 1;
		ret = apr_send_pkt(v->apr_cvs, (uint32_t *) &cvs_session_cmd);
		if (ret < 0) {
			pr_err("Fail in sending STREAM_CONTROL_SESSION\n");
			goto fail;
		}
		ret = wait_event_timeout(v->cvs_wait, (v->cvs_state == 0),
						msecs_to_jiffies(TIMEOUT_MS));
		if (!ret) {
			pr_err("%s: wait_event timeout\n", __func__);
			goto fail;
		}
	}
	return 0;

fail:
	apr_deregister(v->apr_mvm);
	apr_deregister(v->apr_cvs);
	apr_deregister(v->apr_cvp);
	v->cvp_handle = 0;
	v->cvs_handle = 0;
	return -EINVAL;
}

static int voice_send_tty_mode_to_modem(struct voice_data *v)
{
	struct msm_snddev_info *dev_tx_info;
	struct msm_snddev_info *dev_rx_info;
	int tty_mode = 0;
	int ret = 0;
	struct mvm_set_tty_mode_cmd mvm_tty_mode_cmd;

	dev_rx_info = audio_dev_ctrl_find_dev(v->dev_rx.dev_id);
	if (IS_ERR(dev_rx_info)) {
		pr_err("bad dev_id %d\n", v->dev_rx.dev_id);
		goto done;
	}

	dev_tx_info = audio_dev_ctrl_find_dev(v->dev_tx.dev_id);
	if (IS_ERR(dev_tx_info)) {
		pr_err("bad dev_id %d\n", v->dev_tx.dev_id);
		goto done;
	}

	if ((dev_rx_info->capability & SNDDEV_CAP_TTY) &&
		(dev_tx_info->capability & SNDDEV_CAP_TTY))
		tty_mode = 3; /* FULL */
	else if (!(dev_tx_info->capability & SNDDEV_CAP_TTY) &&
		(dev_rx_info->capability & SNDDEV_CAP_TTY))
		tty_mode = 2; /* VCO */
	else if ((dev_tx_info->capability & SNDDEV_CAP_TTY) &&
		!(dev_rx_info->capability & SNDDEV_CAP_TTY))
		tty_mode = 1; /* HCO */

	if (tty_mode) {
		/* send tty mode cmd to mvm */
		mvm_tty_mode_cmd.hdr.hdr_field = APR_HDR_FIELD(
			APR_MSG_TYPE_SEQ_CMD, APR_HDR_LEN(APR_HDR_SIZE),
								APR_PKT_VER);
		mvm_tty_mode_cmd.hdr.pkt_size = APR_PKT_SIZE(APR_HDR_SIZE,
			sizeof(mvm_tty_mode_cmd) - APR_HDR_SIZE);
		pr_debug("pkt size = %d\n", mvm_tty_mode_cmd.hdr.pkt_size);
		mvm_tty_mode_cmd.hdr.src_port = 0;
		mvm_tty_mode_cmd.hdr.dest_port = v->mvm_handle;
		mvm_tty_mode_cmd.hdr.token = 0;
		mvm_tty_mode_cmd.hdr.opcode = VSS_ISTREAM_CMD_SET_TTY_MODE;
		mvm_tty_mode_cmd.tty_mode.mode = tty_mode;
		pr_info("tty mode =%d\n", mvm_tty_mode_cmd.tty_mode.mode);

		v->mvm_state = 1;
		ret = apr_send_pkt(v->apr_mvm, (uint32_t *) &mvm_tty_mode_cmd);
		if (ret < 0) {
			pr_err("Fail: sending VSS_ISTREAM_CMD_SET_TTY_MODE\n");
			goto done;
		}
		ret = wait_event_timeout(v->mvm_wait, (v->mvm_state == 0),
						msecs_to_jiffies(TIMEOUT_MS));
		if (!ret) {
			pr_err("%s: wait_event timeout\n", __func__);
			goto done;
		}
	}
	return 0;
done:
	return -EINVAL;
}

static int voice_send_cvs_cal_to_modem(struct voice_data *v)
{
	struct apr_hdr cvs_cal_cmd_hdr;
	uint32_t *cmd_buf;
	struct acdb_cal_data cal_data;
	struct acdb_cal_block *cal_blk;
	int32_t cal_size_per_network;
	uint32_t *cal_data_per_network;
	int index = 0;
	int ret = 0;

	/* fill the header */
	cvs_cal_cmd_hdr.hdr_field = APR_HDR_FIELD(APR_MSG_TYPE_SEQ_CMD,
		APR_HDR_LEN(APR_HDR_SIZE), APR_PKT_VER);
	cvs_cal_cmd_hdr.pkt_size = APR_PKT_SIZE(APR_HDR_SIZE,
		sizeof(cvs_cal_cmd_hdr) - APR_HDR_SIZE);
	cvs_cal_cmd_hdr.src_port = 0;
	cvs_cal_cmd_hdr.dest_port = v->cvs_handle;
	cvs_cal_cmd_hdr.token = 0;
	cvs_cal_cmd_hdr.opcode =
		VSS_ISTREAM_CMD_CACHE_CALIBRATION_DATA;

	pr_debug("voice_send_cvs_cal_to_modem\n");
	/* get the cvs cal data */
	get_vocstrm_cal(&cal_data);
	if (cal_data.num_cal_blocks == 0) {
		pr_err("%s: No calibration data to send!\n", __func__);
		goto done;
	}

	/* send cvs cal to modem */
	cmd_buf = kzalloc((sizeof(struct apr_hdr) + BUFFER_PAYLOAD_SIZE),
								GFP_KERNEL);
	if (!cmd_buf) {
		pr_err("No memory is allocated.\n");
		return -ENOMEM;
	}
	pr_debug("----- num_cal_blocks=%d\n", (s32)cal_data.num_cal_blocks);
	cal_blk = cal_data.cal_blocks;
	pr_debug("cal_blk =%x\n", (uint32_t)cal_data.cal_blocks);

	for (; index < cal_data.num_cal_blocks; index++) {
		cal_size_per_network = cal_blk[index].cal_size;
		pr_debug(" cal size =%d\n", cal_size_per_network);
		if (cal_size_per_network >= BUFFER_PAYLOAD_SIZE)
			pr_err("Cal size is too big\n");
		cal_data_per_network = (u32 *)cal_blk[index].cal_kvaddr;
		pr_debug(" cal data=%x\n", (uint32_t)cal_data_per_network);
		cvs_cal_cmd_hdr.pkt_size = APR_PKT_SIZE(APR_HDR_SIZE,
			cal_size_per_network);
		pr_debug("header size =%d,  pkt_size =%d\n",
			APR_HDR_SIZE, cvs_cal_cmd_hdr.pkt_size);
		memcpy(cmd_buf, &cvs_cal_cmd_hdr,  APR_HDR_SIZE);
		memcpy(cmd_buf + (APR_HDR_SIZE / sizeof(uint32_t)),
			cal_data_per_network, cal_size_per_network);
		pr_debug("send cvs cal: index =%d\n", index);
		v->cvs_state = 1;
		ret = apr_send_pkt(v->apr_cvs, cmd_buf);
		if (ret < 0) {
			pr_err("Fail: sending cvs cal, idx=%d\n", index);
			continue;
		}
		ret = wait_event_timeout(v->cvs_wait, (v->cvs_state == 0),
			msecs_to_jiffies(TIMEOUT_MS));
		if (!ret) {
			pr_err("%s: wait_event timeout\n", __func__);
			return -EINVAL;
		}
	}
	kfree(cmd_buf);
done:
	return 0;
}

static int voice_send_cvp_cal_to_modem(struct voice_data *v)
{
	struct apr_hdr cvp_cal_cmd_hdr;
	uint32_t *cmd_buf;
	struct acdb_cal_data cal_data;
	struct acdb_cal_block *cal_blk;
	int32_t cal_size_per_network;
	uint32_t *cal_data_per_network;
	int index = 0;
	int ret = 0;


	/* fill the header */
	cvp_cal_cmd_hdr.hdr_field = APR_HDR_FIELD(APR_MSG_TYPE_SEQ_CMD,
		APR_HDR_LEN(APR_HDR_SIZE), APR_PKT_VER);
	cvp_cal_cmd_hdr.pkt_size = APR_PKT_SIZE(APR_HDR_SIZE,
		sizeof(cvp_cal_cmd_hdr) - APR_HDR_SIZE);
	cvp_cal_cmd_hdr.src_port = 0;
	cvp_cal_cmd_hdr.dest_port = v->cvp_handle;
	cvp_cal_cmd_hdr.token = 0;
	cvp_cal_cmd_hdr.opcode =
		VSS_IVOCPROC_CMD_CACHE_CALIBRATION_DATA;

	/* get cal data */
	get_vocproc_cal(&cal_data);
	if (cal_data.num_cal_blocks == 0) {
		pr_err("%s: No calibration data to send!\n", __func__);
		goto done;
	}

	/* send cal to modem */
	cmd_buf = kzalloc((sizeof(struct apr_hdr) + BUFFER_PAYLOAD_SIZE),
								GFP_KERNEL);
	if (!cmd_buf) {
		pr_err("No memory is allocated.\n");
		return -ENOMEM;
	}
	pr_debug("----- num_cal_blocks=%d\n", (s32)cal_data.num_cal_blocks);
	cal_blk = cal_data.cal_blocks;
	pr_debug(" cal_blk =%x\n", (uint32_t)cal_data.cal_blocks);

	for (; index < cal_data.num_cal_blocks; index++) {
		cal_size_per_network = cal_blk[index].cal_size;
		if (cal_size_per_network >= BUFFER_PAYLOAD_SIZE)
			pr_err("Cal size is too big\n");
		pr_debug(" cal size =%d\n", cal_size_per_network);
		cal_data_per_network = (u32 *)cal_blk[index].cal_kvaddr;
		pr_debug(" cal data=%x\n", (uint32_t)cal_data_per_network);

		cvp_cal_cmd_hdr.pkt_size = APR_PKT_SIZE(APR_HDR_SIZE,
			cal_size_per_network);
		memcpy(cmd_buf, &cvp_cal_cmd_hdr,  APR_HDR_SIZE);
		memcpy(cmd_buf + (APR_HDR_SIZE / sizeof(*cmd_buf)),
			cal_data_per_network, cal_size_per_network);
		pr_debug("Send cvp cal\n");
		v->cvp_state = 1;
		ret = apr_send_pkt(v->apr_cvp, cmd_buf);
		if (ret < 0) {
			pr_err("Fail: sending cvp cal, idx=%d\n", index);
			continue;
		}
		ret = wait_event_timeout(v->cvp_wait, (v->cvp_state == 0),
			msecs_to_jiffies(TIMEOUT_MS));
		if (!ret) {
			pr_err("%s: wait_event timeout\n", __func__);
			return -EINVAL;
		}
	}
	kfree(cmd_buf);
done:
	return 0;
}

static int voice_send_cvp_vol_tbl_to_modem(struct voice_data *v)
{
	struct apr_hdr cvp_vol_cal_cmd_hdr;
	uint32_t *cmd_buf;
	struct acdb_cal_data cal_data;
	struct acdb_cal_block *cal_blk;
	int32_t cal_size_per_network;
	uint32_t *cal_data_per_network;
	int index = 0;
	int ret = 0;


	/* fill the header */
	cvp_vol_cal_cmd_hdr.hdr_field = APR_HDR_FIELD(APR_MSG_TYPE_SEQ_CMD,
		APR_HDR_LEN(APR_HDR_SIZE), APR_PKT_VER);
	cvp_vol_cal_cmd_hdr.pkt_size = APR_PKT_SIZE(APR_HDR_SIZE,
		sizeof(cvp_vol_cal_cmd_hdr) - APR_HDR_SIZE);
	cvp_vol_cal_cmd_hdr.src_port = 0;
	cvp_vol_cal_cmd_hdr.dest_port = v->cvp_handle;
	cvp_vol_cal_cmd_hdr.token = 0;
	cvp_vol_cal_cmd_hdr.opcode =
		VSS_IVOCPROC_CMD_CACHE_VOLUME_CALIBRATION_TABLE;

	/* get cal data */
	get_vocvol_cal(&cal_data);
	if (cal_data.num_cal_blocks == 0) {
		pr_err("%s: No calibration data to send!\n", __func__);
		goto done;
	}

	/* send cal to modem */
	cmd_buf = kzalloc((sizeof(struct apr_hdr) + BUFFER_PAYLOAD_SIZE),
								GFP_KERNEL);
	if (!cmd_buf) {
		pr_err("No memory is allocated.\n");
		return -ENOMEM;
	}
	pr_debug("----- num_cal_blocks=%d\n", (s32)cal_data.num_cal_blocks);
	cal_blk = cal_data.cal_blocks;
	pr_debug("Cal_blk =%x\n", (uint32_t)cal_data.cal_blocks);

	for (; index < cal_data.num_cal_blocks; index++) {
		cal_size_per_network = cal_blk[index].cal_size;
		cal_data_per_network = (u32 *)cal_blk[index].cal_kvaddr;
		pr_debug("Cal size =%d, index=%d\n", cal_size_per_network,
			index);
		pr_debug("Cal data=%x\n", (uint32_t)cal_data_per_network);
		cvp_vol_cal_cmd_hdr.pkt_size = APR_PKT_SIZE(APR_HDR_SIZE,
			cal_size_per_network);
		memcpy(cmd_buf, &cvp_vol_cal_cmd_hdr,  APR_HDR_SIZE);
		memcpy(cmd_buf + (APR_HDR_SIZE / sizeof(uint32_t)),
			cal_data_per_network, cal_size_per_network);
		pr_debug("Send vol table\n");

		v->cvp_state = 1;
		ret = apr_send_pkt(v->apr_cvp, cmd_buf);
		if (ret < 0) {
			pr_err("Fail: sending cvp vol cal, idx=%d\n", index);
			continue;
		}
		ret = wait_event_timeout(v->cvp_wait, (v->cvp_state == 0),
			msecs_to_jiffies(TIMEOUT_MS));
		if (!ret) {
			pr_err("%s: wait_event timeout\n", __func__);
			return -EINVAL;
		}
	}
	kfree(cmd_buf);
done:
	return 0;
}

static int voice_send_start_voice_cmd(struct voice_data *v)
{
	struct apr_hdr mvm_start_voice_cmd;
	int ret = 0;

	mvm_start_voice_cmd.hdr_field = APR_HDR_FIELD(APR_MSG_TYPE_SEQ_CMD,
				APR_HDR_LEN(APR_HDR_SIZE), APR_PKT_VER);
	mvm_start_voice_cmd.pkt_size = APR_PKT_SIZE(APR_HDR_SIZE,
				sizeof(mvm_start_voice_cmd) - APR_HDR_SIZE);
	pr_info("send mvm_start_voice_cmd pkt size = %d\n",
				mvm_start_voice_cmd.pkt_size);
	mvm_start_voice_cmd.src_port = 0;
	mvm_start_voice_cmd.dest_port = v->mvm_handle;
	mvm_start_voice_cmd.token = 0;
	mvm_start_voice_cmd.opcode = VSS_IMVM_CMD_START_VOICE;

	v->mvm_state = 1;
	ret = apr_send_pkt(v->apr_mvm, (uint32_t *) &mvm_start_voice_cmd);
	if (ret < 0) {
		pr_err("Fail in sending VSS_IMVM_CMD_START_VOICE\n");
		goto fail;
	}
	ret = wait_event_timeout(v->mvm_wait, (v->mvm_state == 0),
		msecs_to_jiffies(TIMEOUT_MS));
	if (!ret) {
		pr_err("%s: wait_event timeout\n", __func__);
		goto fail;
	}
	return 0;
fail:
	return -EINVAL;
}

static int voice_send_stop_voice_cmd(struct voice_data *v)
{
	struct apr_hdr mvm_stop_voice_cmd;
	int ret = 0;

	mvm_stop_voice_cmd.hdr_field = APR_HDR_FIELD(APR_MSG_TYPE_SEQ_CMD,
				APR_HDR_LEN(APR_HDR_SIZE), APR_PKT_VER);
	mvm_stop_voice_cmd.pkt_size = APR_PKT_SIZE(APR_HDR_SIZE,
				sizeof(mvm_stop_voice_cmd) - APR_HDR_SIZE);
	pr_info("send mvm_stop_voice_cmd pkt size = %d\n",
				mvm_stop_voice_cmd.pkt_size);
	mvm_stop_voice_cmd.src_port = 0;
	mvm_stop_voice_cmd.dest_port = v->mvm_handle;
	mvm_stop_voice_cmd.token = 0;
	mvm_stop_voice_cmd.opcode = VSS_IMVM_CMD_STOP_VOICE;

	v->mvm_state = 1;
	ret = apr_send_pkt(v->apr_mvm, (uint32_t *) &mvm_stop_voice_cmd);
	if (ret < 0) {
		pr_err("Fail in sending VSS_IMVM_CMD_STOP_VOICE\n");
		goto fail;
	}
	ret = wait_event_timeout(v->mvm_wait, (v->mvm_state == 0),
					msecs_to_jiffies(TIMEOUT_MS));
	if (!ret) {
		pr_err("%s: wait_event timeout\n", __func__);
		goto fail;
	}

	return 0;
fail:
	return -EINVAL;
}

static int voice_setup_modem_voice(struct voice_data *v)
{
	struct cvp_create_full_ctl_session_cmd cvp_session_cmd;
	struct apr_hdr cvp_enable_cmd;
	struct mvm_attach_vocproc_cmd mvm_a_vocproc_cmd;
	int ret = 0;
	struct msm_snddev_info *dev_tx_info;


	/* create cvp session and wait for response */
	cvp_session_cmd.hdr.hdr_field = APR_HDR_FIELD(APR_MSG_TYPE_SEQ_CMD,
				APR_HDR_LEN(APR_HDR_SIZE), APR_PKT_VER);
	cvp_session_cmd.hdr.pkt_size = APR_PKT_SIZE(APR_HDR_SIZE,
				sizeof(cvp_session_cmd) - APR_HDR_SIZE);
	pr_info(" send create cvp session, pkt size = %d\n",
				cvp_session_cmd.hdr.pkt_size);
	cvp_session_cmd.hdr.src_port = 0;
	cvp_session_cmd.hdr.dest_port = 0;
	cvp_session_cmd.hdr.token = 0;
	cvp_session_cmd.hdr.opcode =
		VSS_IVOCPROC_CMD_CREATE_FULL_CONTROL_SESSION;

	dev_tx_info = audio_dev_ctrl_find_dev(v->dev_tx.dev_id);
	if (IS_ERR(dev_tx_info)) {
		pr_err("bad dev_id %d\n", v->dev_tx.dev_id);
		goto fail;
	}

	if (dev_tx_info->channel_mode > 1)
		cvp_session_cmd.cvp_session.tx_topology_id =
			VSS_IVOCPROC_TOPOLOGY_ID_TX_DM_FLUENCE;
	else
		cvp_session_cmd.cvp_session.tx_topology_id =
			VSS_IVOCPROC_TOPOLOGY_ID_TX_SM_ECNS;
	cvp_session_cmd.cvp_session.rx_topology_id =
			VSS_IVOCPROC_TOPOLOGY_ID_RX_DEFAULT;
	cvp_session_cmd.cvp_session.direction = 2; /*tx and rx*/
	cvp_session_cmd.cvp_session.network_id = VSS_NETWORK_ID_DEFAULT;
	cvp_session_cmd.cvp_session.tx_port_id = v->dev_tx.dev_port_id;
	cvp_session_cmd.cvp_session.rx_port_id = v->dev_rx.dev_port_id;
	pr_info("topology=%d net_id=%d, dir=%d tx_port_id=%d, rx_port_id=%d\n",
			cvp_session_cmd.cvp_session.tx_topology_id,
			cvp_session_cmd.cvp_session.network_id,
			cvp_session_cmd.cvp_session.direction,
			cvp_session_cmd.cvp_session.tx_port_id,
			cvp_session_cmd.cvp_session.rx_port_id);

	v->cvp_state = 1;
	ret = apr_send_pkt(v->apr_cvp, (uint32_t *) &cvp_session_cmd);
	if (ret < 0) {
		pr_err("Fail in sending VOCPROC_FULL_CONTROL_SESSION\n");
		goto fail;
	}
	pr_debug("wait for cvp create session event\n");
	ret = wait_event_timeout(v->cvp_wait, (v->cvp_state == 0),
				msecs_to_jiffies(TIMEOUT_MS));
	if (!ret) {
		pr_err("%s: wait_event timeout\n", __func__);
		goto fail;
	}

	/* send cvs cal */
	voice_send_cvs_cal_to_modem(v);

	/* send cvp cal */
	voice_send_cvp_cal_to_modem(v);

	/* send cvp vol table cal */
	voice_send_cvp_vol_tbl_to_modem(v);

	/* enable vocproc and wait for respose */
	cvp_enable_cmd.hdr_field = APR_HDR_FIELD(APR_MSG_TYPE_SEQ_CMD,
				APR_HDR_LEN(APR_HDR_SIZE), APR_PKT_VER);
	cvp_enable_cmd.pkt_size = APR_PKT_SIZE(APR_HDR_SIZE,
				sizeof(cvp_enable_cmd) - APR_HDR_SIZE);
	pr_debug("cvp_enable_cmd pkt size = %d, cvp_handle=%d\n",
			cvp_enable_cmd.pkt_size, v->cvp_handle);
	cvp_enable_cmd.src_port = 0;
	cvp_enable_cmd.dest_port = v->cvp_handle;
	cvp_enable_cmd.token = 0;
	cvp_enable_cmd.opcode = VSS_IVOCPROC_CMD_ENABLE;

	v->cvp_state = 1;
	ret = apr_send_pkt(v->apr_cvp, (uint32_t *) &cvp_enable_cmd);
	if (ret < 0) {
		pr_err("Fail in sending VSS_IVOCPROC_CMD_ENABLE\n");
		goto fail;
	}
	ret = wait_event_timeout(v->cvp_wait, (v->cvp_state == 0),
					msecs_to_jiffies(TIMEOUT_MS));
	if (!ret) {
		pr_err("%s: wait_event timeout\n", __func__);
		goto fail;
	}

	/* attach vocproc and wait for response */
	mvm_a_vocproc_cmd.hdr.hdr_field = APR_HDR_FIELD(APR_MSG_TYPE_SEQ_CMD,
					APR_HDR_LEN(APR_HDR_SIZE), APR_PKT_VER);
	mvm_a_vocproc_cmd.hdr.pkt_size = APR_PKT_SIZE(APR_HDR_SIZE,
				sizeof(mvm_a_vocproc_cmd) - APR_HDR_SIZE);
	pr_info("send mvm_a_vocproc_cmd pkt size = %d\n",
				mvm_a_vocproc_cmd.hdr.pkt_size);
	mvm_a_vocproc_cmd.hdr.src_port = 0;
	mvm_a_vocproc_cmd.hdr.dest_port = v->mvm_handle;
	mvm_a_vocproc_cmd.hdr.token = 0;
	mvm_a_vocproc_cmd.hdr.opcode = VSS_ISTREAM_CMD_ATTACH_VOCPROC;
	mvm_a_vocproc_cmd.mvm_attach_cvp_handle.handle = v->cvp_handle;

	v->mvm_state = 1;
	ret = apr_send_pkt(v->apr_mvm, (uint32_t *) &mvm_a_vocproc_cmd);
	if (ret < 0) {
		pr_err("Fail in sending VSS_ISTREAM_CMD_ATTACH_VOCPROC\n");
		goto fail;
	}
	ret = wait_event_timeout(v->mvm_wait, (v->mvm_state == 0),
						msecs_to_jiffies(TIMEOUT_MS));
	if (!ret) {
		pr_err("%s: wait_event timeout\n", __func__);
		goto fail;
	}

	/* send tty mode if tty device is used */
	voice_send_tty_mode_to_modem(v);

	return 0;
fail:
	return -EINVAL;
}

static int voice_destroy_modem_voice(struct voice_data *v)
{
	struct mvm_detach_vocproc_cmd mvm_d_vocproc_cmd;
	struct apr_hdr cvp_destroy_session_cmd;
	int ret = 0;

	/* detach VOCPROC and wait for response from mvm */
	mvm_d_vocproc_cmd.hdr.hdr_field = APR_HDR_FIELD(APR_MSG_TYPE_SEQ_CMD,
					APR_HDR_LEN(APR_HDR_SIZE), APR_PKT_VER);
	mvm_d_vocproc_cmd.hdr.pkt_size = APR_PKT_SIZE(APR_HDR_SIZE,
				sizeof(mvm_d_vocproc_cmd) - APR_HDR_SIZE);
	pr_info("mvm_d_vocproc_cmd  pkt size = %d\n",
				mvm_d_vocproc_cmd.hdr.pkt_size);
	mvm_d_vocproc_cmd.hdr.src_port = 0;
	mvm_d_vocproc_cmd.hdr.dest_port = v->mvm_handle;
	mvm_d_vocproc_cmd.hdr.token = 0;
	mvm_d_vocproc_cmd.hdr.opcode = VSS_ISTREAM_CMD_DETACH_VOCPROC;
	mvm_d_vocproc_cmd.mvm_detach_cvp_handle.handle = v->cvp_handle;

	v->mvm_state = 1;
	ret = apr_send_pkt(v->apr_mvm, (uint32_t *) &mvm_d_vocproc_cmd);
	if (ret < 0) {
		pr_err("Fail in sending VSS_ISTREAM_CMD_DETACH_VOCPROC\n");
		goto fail;
	}
	ret = wait_event_timeout(v->mvm_wait, (v->mvm_state == 0),
						msecs_to_jiffies(TIMEOUT_MS));
	if (!ret) {
		pr_err("%s: wait_event timeout\n", __func__);
		goto fail;
	}

	/* destrop cvp session */
	cvp_destroy_session_cmd.hdr_field = APR_HDR_FIELD(APR_MSG_TYPE_SEQ_CMD,
					APR_HDR_LEN(APR_HDR_SIZE), APR_PKT_VER);
	cvp_destroy_session_cmd.pkt_size = APR_PKT_SIZE(APR_HDR_SIZE,
				sizeof(cvp_destroy_session_cmd) - APR_HDR_SIZE);
	pr_info("cvp_destroy_session_cmd pkt size = %d\n",
				cvp_destroy_session_cmd.pkt_size);
	cvp_destroy_session_cmd.src_port = 0;
	cvp_destroy_session_cmd.dest_port = v->cvp_handle;
	cvp_destroy_session_cmd.token = 0;
	cvp_destroy_session_cmd.opcode = APRV2_IBASIC_CMD_DESTROY_SESSION;

	v->cvp_state = 1;
	ret = apr_send_pkt(v->apr_cvp, (uint32_t *) &cvp_destroy_session_cmd);
	if (ret < 0) {
		pr_err("Fail in sending APRV2_IBASIC_CMD_DESTROY_SESSION\n");
		goto fail;
	}
	ret = wait_event_timeout(v->cvp_wait, (v->cvp_state == 0),
						msecs_to_jiffies(TIMEOUT_MS));
	if (!ret) {
		pr_err("%s: wait_event timeout\n", __func__);
		goto fail;
	}

	v->cvp_handle = 0;

	return 0;

fail:
	return -EINVAL;
}

static int voice_send_mute_cmd_to_modem(struct voice_data *v)
{
	struct cvs_set_mute_cmd cvs_mute_cmd;
	int ret = 0;

	/* send mute/unmute to cvs */
	cvs_mute_cmd.hdr.hdr_field = APR_HDR_FIELD(APR_MSG_TYPE_SEQ_CMD,
				APR_HDR_LEN(APR_HDR_SIZE), APR_PKT_VER);
	cvs_mute_cmd.hdr.pkt_size = APR_PKT_SIZE(APR_HDR_SIZE,
				sizeof(cvs_mute_cmd) - APR_HDR_SIZE);
	cvs_mute_cmd.hdr.src_port = 0;
	cvs_mute_cmd.hdr.dest_port = v->cvs_handle;
	cvs_mute_cmd.hdr.token = 0;
	cvs_mute_cmd.hdr.opcode = VSS_ISTREAM_CMD_SET_MUTE;
	cvs_mute_cmd.cvs_set_mute.direction = 0; /*tx*/
	cvs_mute_cmd.cvs_set_mute.mute_flag = v->dev_tx.mute;

	pr_info(" mute value =%d\n", cvs_mute_cmd.cvs_set_mute.mute_flag);

	v->cvs_state = 1;
	ret = apr_send_pkt(v->apr_cvs, (uint32_t *) &cvs_mute_cmd);
	if (ret < 0) {
		pr_err("Fail: send STREAM SET MUTE\n");
		goto fail;
	}
	ret = wait_event_timeout(v->cvs_wait, (v->cvs_state == 0),
					msecs_to_jiffies(TIMEOUT_MS));
	if (!ret)
		pr_err("%s: wait_event timeout\n", __func__);

fail:
	return 0;
}

static int voice_send_vol_index_to_modem(struct voice_data *v)
{
	struct cvp_set_rx_volume_index_cmd cvp_vol_cmd;
	int ret = 0;

	/* send volume index to cvp */
	cvp_vol_cmd.hdr.hdr_field = APR_HDR_FIELD(APR_MSG_TYPE_SEQ_CMD,
		APR_HDR_LEN(APR_HDR_SIZE), APR_PKT_VER);
	cvp_vol_cmd.hdr.pkt_size = APR_PKT_SIZE(APR_HDR_SIZE,
		sizeof(cvp_vol_cmd) - APR_HDR_SIZE);
	cvp_vol_cmd.hdr.src_port = 0;
	cvp_vol_cmd.hdr.dest_port = v->cvp_handle;
	cvp_vol_cmd.hdr.token = 0;
	cvp_vol_cmd.hdr.opcode =
		VSS_IVOCPROC_CMD_SET_RX_VOLUME_INDEX;
	cvp_vol_cmd.cvp_set_vol_idx.vol_index = v->dev_rx.volume;
	pr_debug(" vol index= %d\n", cvp_vol_cmd.cvp_set_vol_idx.vol_index);
	v->cvp_state = 1;
	ret = apr_send_pkt(v->apr_cvp, (uint32_t *) &cvp_vol_cmd);
	if (ret < 0) {
		pr_err("Fail in sending RX VOL INDEX\n");
		return -EINVAL;
	}
	ret = wait_event_timeout(v->cvp_wait, (v->cvp_state == 0),
		msecs_to_jiffies(TIMEOUT_MS));
	if (!ret) {
		pr_err("%s: wait_event timeout\n", __func__);
		return -EINVAL;
	}
	return 0;
}

static void voice_auddev_cb_function(u32 evt_id,
			union auddev_evt_data *evt_payload,
			void *private_data)
{
	struct voice_data *v = &voice;
	struct sidetone_cal sidetone_cal_data;

	pr_info("auddev_cb_function, evt_id=%d,\n", evt_id);
	if ((evt_id != AUDDEV_EVT_START_VOICE) ||
			(evt_id != AUDDEV_EVT_END_VOICE)) {
		if (evt_payload == NULL) {
			pr_err(" evt_payload is NULL pointer\n");
			return;
		}
	}

	switch (evt_id) {
	case AUDDEV_EVT_START_VOICE:
		if ((v->voc_state == VOC_INIT) ||
				(v->voc_state == VOC_RELEASE)) {
			v->v_call_status = VOICE_CALL_START;
			if ((v->dev_rx.enabled == VOICE_DEV_ENABLED)
				&& (v->dev_tx.enabled == VOICE_DEV_ENABLED)) {
				voice_apr_register(v);
				voice_create_mvm_cvs_session(v);
				voice_setup_modem_voice(v);
				voice_send_start_voice_cmd(v);
				get_sidetone_cal(&sidetone_cal_data);
				msm_snddev_enable_sidetone(
					v->dev_rx.dev_id,
					sidetone_cal_data.enable,
					sidetone_cal_data.gain);
				v->voc_state = VOC_RUN;
			}
		}
		break;
	case AUDDEV_EVT_DEV_CHG_VOICE:
		if (v->dev_rx.enabled == VOICE_DEV_ENABLED)
			msm_snddev_enable_sidetone(v->dev_rx.dev_id, 0, 0);
		v->dev_rx.enabled = VOICE_DEV_DISABLED;
		v->dev_tx.enabled = VOICE_DEV_DISABLED;
		if (v->voc_state == VOC_RUN) {
			/* send cmd to modem to do voice device change */
			voice_destroy_modem_voice(v);
			v->voc_state = VOC_CHANGE;
		}
		break;
	case AUDDEV_EVT_DEV_RDY:
		if (v->voc_state == VOC_CHANGE) {
			/* get port Ids */
			if (evt_payload->voc_devinfo.dev_type == DIR_RX) {
				v->dev_rx.dev_port_id =
					evt_payload->voc_devinfo.dev_port_id;
				v->dev_rx.sample =
					evt_payload->voc_devinfo.dev_sample;
				v->dev_rx.dev_id =
				evt_payload->voc_devinfo.dev_id;
				v->dev_rx.enabled = VOICE_DEV_ENABLED;
			} else {
				v->dev_tx.dev_port_id =
					evt_payload->voc_devinfo.dev_port_id;
				v->dev_tx.sample =
					evt_payload->voc_devinfo.dev_sample;
				v->dev_tx.enabled = VOICE_DEV_ENABLED;
				v->dev_tx.dev_id =
				evt_payload->voc_devinfo.dev_id;
			}
			if ((v->dev_rx.enabled == VOICE_DEV_ENABLED) &&
				(v->dev_tx.enabled == VOICE_DEV_ENABLED)) {
				voice_setup_modem_voice(v);
				voice_send_mute_cmd_to_modem(v);
				voice_send_vol_index_to_modem(v);
				get_sidetone_cal(&sidetone_cal_data);
				msm_snddev_enable_sidetone(
					v->dev_rx.dev_id,
					sidetone_cal_data.enable,
					sidetone_cal_data.gain);
				v->voc_state = VOC_RUN;
			}
		} else if ((v->voc_state == VOC_INIT) ||
			(v->voc_state == VOC_RELEASE)) {
			/* get AFE ports */
			if (evt_payload->voc_devinfo.dev_type == DIR_RX) {
				/* get rx port id */
				v->dev_rx.dev_port_id =
					evt_payload->voc_devinfo.dev_port_id;
				v->dev_rx.sample =
					evt_payload->voc_devinfo.dev_sample;
				v->dev_rx.dev_id =
				evt_payload->voc_devinfo.dev_id;
				v->dev_rx.enabled = VOICE_DEV_ENABLED;
			} else {
				/* get tx port id */
				v->dev_tx.dev_port_id =
					evt_payload->voc_devinfo.dev_port_id;
				v->dev_tx.sample =
					evt_payload->voc_devinfo.dev_sample;
				v->dev_tx.dev_id =
				evt_payload->voc_devinfo.dev_id;
				v->dev_tx.enabled = VOICE_DEV_ENABLED;
			}
			if ((v->dev_rx.enabled == VOICE_DEV_ENABLED) &&
				(v->dev_tx.enabled == VOICE_DEV_ENABLED) &&
				(v->v_call_status == VOICE_CALL_START)) {
				voice_apr_register(v);
				voice_create_mvm_cvs_session(v);
				voice_setup_modem_voice(v);
				voice_send_start_voice_cmd(v);
				get_sidetone_cal(&sidetone_cal_data);
				msm_snddev_enable_sidetone(
					v->dev_rx.dev_id,
					sidetone_cal_data.enable,
					sidetone_cal_data.gain);
				v->voc_state = VOC_RUN;
			}
		}
		break;
	case AUDDEV_EVT_DEVICE_VOL_MUTE_CHG:
		/* cache the mute and volume index value */
		if (evt_payload->voc_devinfo.dev_type == DIR_TX) {
			v->dev_tx.mute =
				evt_payload->voc_vm_info.dev_vm_val.mute;
			if (v->voc_state == VOC_RUN)
				voice_send_mute_cmd_to_modem(v);
		} else {
			v->dev_rx.volume = evt_payload->
				voc_vm_info.dev_vm_val.vol;
			if (v->voc_state == VOC_RUN)
				voice_send_vol_index_to_modem(v);
		}
		break;
	case AUDDEV_EVT_REL_PENDING:
		/* recover the tx mute and rx volume to the default values */
		if (v->voc_state == VOC_RUN) {
			/* send stop voice to modem */
			voice_send_stop_voice_cmd(v);
			voice_destroy_modem_voice(v);
			v->voc_state = VOC_RELEASE;
		}
		if (evt_payload->voc_devinfo.dev_type == DIR_RX)
			v->dev_rx.enabled = VOICE_DEV_DISABLED;
		else
				v->dev_tx.enabled = VOICE_DEV_DISABLED;

		break;
	case AUDDEV_EVT_END_VOICE:
		/* recover the tx mute and rx volume to the default values */
		v->dev_tx.mute = v->default_mute_val;
		v->dev_rx.volume = v->default_vol_val;
		if (v->dev_rx.enabled == VOICE_DEV_ENABLED)
			msm_snddev_enable_sidetone(v->dev_rx.dev_id, 0, 0);
		if (v->voc_state == VOC_RUN) {
			/* call stop modem voice */
			voice_send_stop_voice_cmd(v);
			voice_destroy_modem_voice(v);
			v->voc_state = VOC_RELEASE;
		}
			v->v_call_status = VOICE_CALL_END;

		break;
	default:
		pr_err("UNKNOWN EVENT\n");
	}
	return;
}
EXPORT_SYMBOL(voice_auddev_cb_function);

static int32_t modem_mvm_callback(struct apr_client_data *data, void *priv)
{
	uint32_t *ptr;
	struct voice_data *v = &voice;

	pr_debug("%s\n", __func__);
	pr_debug("%s: Payload Length = %d, opcode=%x\n", __func__,
				data->payload_size, data->opcode);

	if (data->opcode == APR_BASIC_RSP_RESULT) {
		if (data->payload_size) {
			ptr = data->payload;

			pr_info("%x %x\n", ptr[0], ptr[1]);
			/* ping mvm service ACK */

			if (ptr[0] ==
			 VSS_IMVM_CMD_CREATE_PASSIVE_CONTROL_SESSION) {
				pr_debug("%s: cmd = 0x%x\n", __func__, ptr[0]);
				if (!ptr[1]) {
					v->mvm_handle = data->src_port;
				} else
					pr_info("got NACK for sending \
							MVM create session \n");
				v->mvm_state = 0;
				wake_up(&v->mvm_wait);
			} else if (ptr[0] == VSS_IMVM_CMD_START_VOICE) {
				pr_debug("%s: cmd = 0x%x\n", __func__, ptr[0]);
				v->mvm_state = 0;
				wake_up(&v->mvm_wait);
			} else if (ptr[0] == VSS_ISTREAM_CMD_ATTACH_VOCPROC) {
				pr_debug("%s: cmd = 0x%x\n", __func__, ptr[0]);
				v->mvm_state = 0;
				wake_up(&v->mvm_wait);
			} else if (ptr[0] == VSS_IMVM_CMD_STOP_VOICE) {
				v->mvm_state = 0;
				wake_up(&v->mvm_wait);
			} else if (ptr[0] == VSS_ISTREAM_CMD_DETACH_VOCPROC) {
				v->mvm_state = 0;
				wake_up(&v->mvm_wait);
			} else if (ptr[0] == VSS_ISTREAM_CMD_SET_TTY_MODE) {
				v->mvm_state = 0;
				wake_up(&v->mvm_wait);
			} else
				pr_debug("%s: not match cmd = 0x%x\n",
							__func__, ptr[0]);
		}
	}

	return 0;
}

static int32_t modem_cvs_callback(struct apr_client_data *data, void *priv)
{
	uint32_t *ptr;
	struct voice_data *v = &voice;

	pr_debug("%s\n", __func__);
	pr_debug("%s: Payload Length = %d, opcode=%x\n", __func__,
					data->payload_size, data->opcode);

	if (data->opcode == APR_BASIC_RSP_RESULT) {
		if (data->payload_size) {
			ptr = data->payload;

			pr_info("%x %x\n", ptr[0], ptr[1]);
			/*response from modem CVS */
			if (ptr[0] ==
			VSS_ISTREAM_CMD_CREATE_PASSIVE_CONTROL_SESSION) {
				if (!ptr[1]) {
					v->cvs_handle = data->src_port;
				} else
					pr_info("got NACK for sending \
							CVS create session \n");
				v->cvs_state = 0;
				wake_up(&v->cvs_wait);
			} else if (ptr[0] ==
				VSS_ISTREAM_CMD_CACHE_CALIBRATION_DATA) {
				v->cvs_state = 0;
				wake_up(&v->cvs_wait);
			} else if (ptr[0] ==
					VSS_ISTREAM_CMD_SET_MUTE) {
				v->cvs_state = 0;
				wake_up(&v->cvs_wait);
			} else
				pr_debug("%s: cmd = 0x%x\n", __func__, ptr[0]);
		}
	}
	return 0;
}

static int32_t modem_cvp_callback(struct apr_client_data *data, void *priv)
{
	uint32_t *ptr;
	struct voice_data *v = &voice;

	pr_debug("%s\n", __func__);
	pr_debug("%s: Payload Length = %d, opcode=%x\n", __func__,
				data->payload_size, data->opcode);

	if (data->opcode == APR_BASIC_RSP_RESULT) {
		if (data->payload_size) {
			ptr = data->payload;

			pr_info("%x %x\n", ptr[0], ptr[1]);
			/*response from modem CVP */
			if (ptr[0] ==
				VSS_IVOCPROC_CMD_CREATE_FULL_CONTROL_SESSION) {
				pr_debug("%s: cmd = 0x%x\n", __func__, ptr[0]);
				if (!ptr[1]) {
					v->cvp_handle = data->src_port;
					pr_debug("cvphdl=%d\n", data->src_port);
				} else
					pr_info("got NACK from CVP create \
						session response\n");
				v->cvp_state = 0;
				wake_up(&v->cvp_wait);
			} else if (ptr[0] ==
				VSS_IVOCPROC_CMD_CACHE_CALIBRATION_DATA) {
				pr_debug("%s: cmd = 0x%x\n", __func__, ptr[0]);
				v->cvp_state = 0;
				wake_up(&v->cvp_wait);
			} else if (ptr[0] ==
					VSS_IVOCPROC_CMD_SET_RX_VOLUME_INDEX) {
				v->cvp_state = 0;
				wake_up(&v->cvp_wait);
			} else if (ptr[0] == VSS_IVOCPROC_CMD_ENABLE) {
				v->cvp_state = 0;
				wake_up(&v->cvp_wait);
			} else if (ptr[0] == APRV2_IBASIC_CMD_DESTROY_SESSION) {
				v->cvp_state = 0;
				wake_up(&v->cvp_wait);
			} else if (ptr[0] ==
				VSS_IVOCPROC_CMD_CACHE_VOLUME_CALIBRATION_TABLE
				) {

				pr_debug("%s: cmd = 0x%x\n", __func__, ptr[0]);
				v->cvp_state = 0;
				wake_up(&v->cvp_wait);
			} else
				pr_debug("%s: not match cmd = 0x%x\n",
							__func__, ptr[0]);
		}
	}
	return 0;
}


static int __init voice_init(void)
{
	int rc = 0;
	struct voice_data *v = &voice;
	pr_info("%s\n", __func__); /* Macro prints the file name and function */

	/* set default value */
	v->default_mute_val = 1;  /* default is mute */
	v->default_vol_val = 0;
	v->default_sample_val = 8000;

	/* initialize dev_rx and dev_tx */
	memset(&v->dev_tx, 0, sizeof(struct device_data));
	memset(&v->dev_rx, 0, sizeof(struct device_data));
	v->dev_rx.volume = v->default_vol_val;
	v->dev_tx.mute = v->default_mute_val;

	v->voc_state = VOC_INIT;
	init_waitqueue_head(&v->mvm_wait);
	init_waitqueue_head(&v->cvs_wait);
	init_waitqueue_head(&v->cvp_wait);

	v->mvm_handle = 0;
	v->cvs_handle = 0;
	v->cvp_handle = 0;

	v->apr_mvm = NULL;
	v->apr_cvs = NULL;
	v->apr_cvp = NULL;


	v->device_events = AUDDEV_EVT_DEV_CHG_VOICE |
			AUDDEV_EVT_DEV_RDY |
			AUDDEV_EVT_REL_PENDING |
			AUDDEV_EVT_START_VOICE |
			AUDDEV_EVT_END_VOICE |
			AUDDEV_EVT_DEVICE_VOL_MUTE_CHG |
			AUDDEV_EVT_FREQ_CHG;

	pr_debug("to register call back\n");
	/* register callback to auddev */
	auddev_register_evt_listner(v->device_events, AUDDEV_CLNT_VOC,
				0, voice_auddev_cb_function, v);

	return rc;
}

device_initcall(voice_init);
