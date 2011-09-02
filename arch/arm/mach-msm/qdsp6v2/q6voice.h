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
 *	from this software without specific prior written permission.
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
#ifndef __QDSP6VOICE_H__
#define __QDSP6VOICE_H__

#include <mach/qdsp6v2/apr.h>

/* Device Event */
#define DEV_CHANGE_READY                0x1

#define VOICE_CALL_START        0x1
#define VOICE_CALL_END          0

#define VOICE_DEV_ENABLED       0x1
#define VOICE_DEV_DISABLED      0

struct voice_header {
	uint32_t id;
	uint32_t data_len;
};

struct voice_init {
	struct voice_header hdr;
	void *cb_handle;
};


/* Device information payload structure */

struct device_data {
	uint32_t dev_acdb_id;
	uint32_t volume; /* in percentage */
	uint32_t mute;
	uint32_t sample;
	uint32_t enabled;
	uint32_t dev_id;
	uint32_t dev_port_id;
};

enum {
	VOC_INIT = 0,
	VOC_RUN,
	VOC_CHANGE,
	VOC_RELEASE,
};

/* TO MVM commands */
#define VSS_IMVM_CMD_CREATE_PASSIVE_CONTROL_SESSION	0x000110FF
/**< No payload. Wait for APRV2_IBASIC_RSP_RESULT response. */

#define APRV2_IBASIC_CMD_DESTROY_SESSION		0x0001003C
/**< No payload. Wait for APRV2_IBASIC_RSP_RESULT response. */

#define VSS_IMVM_CMD_START_VOICE			0x00011190
/**< No payload. Wait for APRV2_IBASIC_RSP_RESULT response. */

#define VSS_IMVM_CMD_STOP_VOICE				0x00011192
/**< No payload. Wait for APRV2_IBASIC_RSP_RESULT response. */

#define VSS_ISTREAM_CMD_ATTACH_VOCPROC			0x000110F8
/**< Wait for APRV2_IBASIC_RSP_RESULT response. */

#define VSS_ISTREAM_CMD_DETACH_VOCPROC			0x000110F9
/**< Wait for APRV2_IBASIC_RSP_RESULT response. */


#define VSS_ISTREAM_CMD_SET_TTY_MODE			0x00011196
/**< Wait for APRV2_IBASIC_RSP_RESULT response. */

struct vss_istream_cmd_set_tty_mode_t {
	uint32_t mode;
	/**<
	* TTY mode.
	*
	* 0 : TTY disabled
	* 1 : HCO
	* 2 : VCO
	* 3 : FULL
	*/
} __attribute__((packed));

struct vss_istream_cmd_attach_vocproc_t {
	uint16_t handle;
	/**< Handle of vocproc being attached. */
} __attribute__((packed));

struct vss_istream_cmd_detach_vocproc_t {
	uint16_t handle;
	/**< Handle of vocproc being detached. */
} __attribute__((packed));

struct mvm_attach_vocproc_cmd {
	struct apr_hdr hdr;
	struct vss_istream_cmd_attach_vocproc_t mvm_attach_cvp_handle;
} __attribute__((packed));

struct mvm_detach_vocproc_cmd {
	struct apr_hdr hdr;
	struct vss_istream_cmd_detach_vocproc_t mvm_detach_cvp_handle;
} __attribute__((packed));

struct mvm_create_passive_ctl_session_cmd {
	struct apr_hdr hdr;
} __attribute__((packed));

struct mvm_set_tty_mode_cmd {
	struct apr_hdr hdr;
	struct vss_istream_cmd_set_tty_mode_t tty_mode;
} __attribute__((packed));

/* TO CVS commands */
#define VSS_ISTREAM_CMD_CREATE_PASSIVE_CONTROL_SESSION	0x00011140
/**< Wait for APRV2_IBASIC_RSP_RESULT response. */

#define APRV2_IBASIC_CMD_DESTROY_SESSION		0x0001003C

#define VSS_ISTREAM_CMD_CACHE_CALIBRATION_DATA		0x000110FB

#define VSS_ISTREAM_CMD_SET_MUTE			0x00011022

struct vss_istream_cmd_create_passive_control_session_t {
	char name[20];
	/**<
	* A variable-sized stream name.
	*
	* The stream name size is the payload size minus the size of the other
	* fields.
	*/
} __attribute__((packed));
struct vss_istream_cmd_set_mute_t {
	uint16_t direction;
	/**<
	* 0 : TX only
	* 1 : RX only
	* 2 : TX and Rx
	*/
	uint16_t mute_flag;
	/**<
	* Mute, un-mute.
	*
	* 0 : Silence disable
	* 1 : Silence enable
	* 2 : CNG enable. Applicable to TX only. If set on RX behavior
	*     will be the same as 1
	*/
} __attribute__((packed));

struct cvs_create_passive_ctl_session_cmd {
	struct apr_hdr hdr;
	struct vss_istream_cmd_create_passive_control_session_t cvs_session;
} __attribute__((packed));

struct cvs_destroy_session_cmd {
	struct apr_hdr hdr;
} __attribute__((packed));

struct cvs_cache_calibration_data_cmd {
	struct apr_hdr hdr;
} __attribute__ ((packed));

struct cvs_set_mute_cmd {
	struct apr_hdr hdr;
	struct vss_istream_cmd_set_mute_t cvs_set_mute;
} __attribute__((packed));

/* TO CVP commands */

#define VSS_IVOCPROC_CMD_CREATE_FULL_CONTROL_SESSION	0x000100C3
/**< Wait for APRV2_IBASIC_RSP_RESULT response. */

#define APRV2_IBASIC_CMD_DESTROY_SESSION		0x0001003C

#define VSS_IVOCPROC_CMD_SET_DEVICE			0x000100C4

#define VSS_IVOCPROC_CMD_CACHE_CALIBRATION_DATA		0x000110E3

#define VSS_IVOCPROC_CMD_CACHE_VOLUME_CALIBRATION_TABLE	0x000110E4

#define VSS_IVOCPROC_CMD_SET_VP3_DATA			0x000110EB

#define VSS_IVOCPROC_CMD_SET_RX_VOLUME_INDEX		0x000110EE

#define VSS_IVOCPROC_CMD_ENABLE				0x000100C6
/**< No payload. Wait for APRV2_IBASIC_RSP_RESULT response. */

#define VSS_IVOCPROC_CMD_DISABLE			0x000110E1
/**< No payload. Wait for APRV2_IBASIC_RSP_RESULT response. */

#define VSS_IVOCPROC_TOPOLOGY_ID_NONE			0x00010F70
#define VSS_IVOCPROC_TOPOLOGY_ID_TX_SM_ECNS		0x00010F71
#define VSS_IVOCPROC_TOPOLOGY_ID_TX_DM_FLUENCE		0x00010F72

#define VSS_IVOCPROC_TOPOLOGY_ID_RX_DEFAULT		0x00010F77

#define VSS_NETWORK_ID_DEFAULT				0x00010037

struct vss_ivocproc_cmd_create_full_control_session_t {
	uint16_t direction;
	/**<
	*stream direction.
	* 0 : TX only
	* 1 : RX only
	* 2 : TX and RX
	* */
	uint32_t tx_port_id;
	/**<
	* * TX device port ID which vocproc will connect to. If not supplying a
	** port ID set to VSS_IVOCPROC_PORT_ID_NONE.
	* */
	uint32_t tx_topology_id;
	/**<
	 Tx leg topology ID. If not supplying a topology ID set to
	* VSS_IVOCPROC_TOPOLOGY_ID_NONE.
	*/
	uint32_t rx_port_id;
	/**<
	* * RX device port ID which vocproc will connect to. If not supplying a
	* * port ID set to VSS_IVOCPROC_PORT_ID_NONE.
	* */
	uint32_t rx_topology_id;
	/**<
	* Rx leg topology ID. If not supplying a topology ID set to
	* VSS_IVOCPROC_TOPOLOGY_ID_NONE.
	*/
	int32_t network_id;
	/**<
	* Network ID. (Refer to VSS_NETWORK_ID_XXX). If not supplying a network
	*ID set to VSS_NETWORK_ID_DEFAULT.
	*/
} __attribute__((packed));

struct vss_ivocproc_cmd_set_device_t {
	uint32_t tx_port_id;
	/**<
	* TX device port ID which vocproc will connect to.
	* VSS_IVOCPROC_PORT_ID_NONE means vocproc will not connect to any port.
	*/
	uint32_t tx_topology_id;
	/**<
	* TX leg topology ID.
	* VSS_IVOCPROC_TOPOLOGY_ID_NONE means vocproc does not contain any
	* pre/post-processing blocks and is pass-through.
	*/
	int32_t rx_port_id;
	/**<
	* RX device port ID which vocproc will connect to.
	* VSS_IVOCPROC_PORT_ID_NONE means vocproc will not connect to any port.
	*/
	uint32_t rx_topology_id;
	/**<
	* RX leg topology ID.
	* VSS_IVOCPROC_TOPOLOGY_ID_NONE means vocproc does not contain any
	* pre/post-processing blocks and is pass-through.
	*/
} __attribute__((packed));

struct vss_ivocproc_cmd_set_volume_index_t {
	uint16_t vol_index;
	/**<
	* Volume index utilized by the vocproc to index into the volume table
	* provided in VSS_IVOCPROC_CMD_CACHE_VOLUME_CALIBRATION_TABLE and set
	* volume on the VDSP.
	*/
} __attribute__((packed));

struct cvp_create_full_ctl_session_cmd {
	struct apr_hdr hdr;
	struct vss_ivocproc_cmd_create_full_control_session_t cvp_session;
} __attribute__ ((packed));

struct cvp_command {
	struct apr_hdr hdr;
} __attribute__((packed));

struct cvp_set_device_cmd {
	struct apr_hdr hdr;
	struct vss_ivocproc_cmd_set_device_t cvp_set_device;
} __attribute__ ((packed));

struct cvp_cache_calibration_data_cmd {
	struct apr_hdr hdr;
} __attribute__((packed));

struct cvp_cache_volume_calibration_table_cmd {
	struct apr_hdr hdr;
} __attribute__((packed));

struct cvp_set_vp3_data_cmd {
	struct apr_hdr hdr;
} __attribute__((packed));

struct cvp_set_rx_volume_index_cmd {
	struct apr_hdr hdr;
	struct vss_ivocproc_cmd_set_volume_index_t cvp_set_vol_idx;
} __attribute__((packed));

#endif
