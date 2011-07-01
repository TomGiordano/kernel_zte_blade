/* Copyright (c) 2008-2009, Code Aurora Forum. All rights reserved.
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

#ifndef DIAGCHAR_H
#define DIAGCHAR_H

#include <linux/init.h>
#include <linux/module.h>
#include <linux/mempool.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <mach/msm_smd.h>
#include <asm/atomic.h>

/* Size of the USB buffers used for read and write*/
#define USB_MAX_OUT_BUF 4096
#define USB_MAX_IN_BUF  8192
#define MAX_BUF_SIZE  	32768
/* Size of the buffer used for deframing a packet
  reveived from the PC tool*/
#define HDLC_MAX 4096
#define HDLC_OUT_BUF_SIZE	8192
#define POOL_TYPE_COPY		1
#define POOL_TYPE_HDLC		0
#define POOL_TYPE_USB_STRUCT	2
#define MODEM_DATA 		1
#define QDSP_DATA  		2
#define APPS_DATA  		3
#define NON_SMD_CONTEXT	0
#define SMD_CONTEXT		1
/* Number of maximum USB requests that the USB layer should handle at
   one time. */
#define MAX_DIAG_USB_REQUESTS 12
#define MSG_MASK_SIZE 8000
#define LOG_MASK_SIZE 1000
#define EVENT_MASK_SIZE 1000
#define REG_TABLE_SIZE 25
#define PKT_SIZE 4096

#define APPEND_DEBUG(ch) \
do {							\
	diag_debug_buf[diag_debug_buf_idx] = ch; \
	(diag_debug_buf_idx < 1023) ? \
	(diag_debug_buf_idx++) : (diag_debug_buf_idx = 0); \
} while (0)

struct diag_master_table {
	uint16_t cmd_code;
	uint16_t subsys_id;
	uint16_t cmd_code_lo;
	uint16_t cmd_code_hi;
	int process_id;
};

struct diag_write_device {
	void *buf;
	int length;
};

struct diagchar_dev {

	/* State for the char driver */
	unsigned int major;
	unsigned int minor_start;
	int num;
	struct cdev *cdev;
	char *name;
	int dropped_count;
	struct class *diagchar_class;
	int ref_count;
	struct mutex diagchar_mutex;
	wait_queue_head_t wait_q;
	int *client_map;
	int *data_ready;
	int num_clients;
	struct diag_write_device *buf_tbl;

	/* Memory pool parameters */
	unsigned int itemsize;
	unsigned int poolsize;
	unsigned int itemsize_hdlc;
	unsigned int poolsize_hdlc;
	unsigned int itemsize_usb_struct;
	unsigned int poolsize_usb_struct;
	unsigned int debug_flag;
	/* State for the mempool for the char driver */
	mempool_t *diagpool;
	mempool_t *diag_hdlc_pool;
	mempool_t *diag_usb_struct_pool;
	struct mutex diagmem_mutex;
	int count;
	int count_hdlc_pool;
	int count_usb_struct_pool;
	int used;

	/* State for diag forwarding */
	unsigned char *usb_buf_in;
	unsigned char *usb_buf_in_qdsp;
	unsigned char *usb_buf_out;
	smd_channel_t *ch;
	smd_channel_t *chqdsp;
	int in_busy;
	int in_busy_qdsp;
	int read_len;
	unsigned char *hdlc_buf;
	unsigned hdlc_count;
	unsigned hdlc_escape;
	int usb_connected;
	struct workqueue_struct *diag_wq;
	struct work_struct diag_read_work;
	struct work_struct diag_drain_work;
	struct work_struct diag_read_smd_work;
	struct work_struct diag_read_smd_qdsp_work;
	uint8_t *msg_masks;
	uint8_t *log_masks;
	uint8_t *event_masks;
	struct diag_master_table *table;
	uint8_t *pkt_buf;
	int pkt_length;
	struct diag_request *usb_write_ptr;
	struct diag_request *usb_read_ptr;
	struct diag_request *usb_write_ptr_svc;
	struct diag_request *usb_write_ptr_qdsp;
	int logging_mode;
	int logging_process_id;
};

extern struct diagchar_dev *driver;
#endif
