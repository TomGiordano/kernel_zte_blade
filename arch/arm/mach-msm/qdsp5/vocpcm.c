/* 
 * A call recording/answering module for msm72xx devices 
 * which do not have a vocpcm driver built-in.
 * Should be put into arch/arm/mach-msm/qdsp5.
 *
 * pcm audio input device
 *
 * Copyright (C) 2008 HTC Corporation
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>

/* we have to include slab since .34 onward with intention for kfree() and friends*/
#include <linux/slab.h>

#include <asm/atomic.h>
#include <asm/ioctls.h>
#include <mach/msm_adsp.h>
#include <mach/msm_rpcrouter.h>

#if 0
#include <mach/msm_rpc_version.h>
#else

/* Please either check these values against AMSS_VERSION,
   or include a suitable header file if you have it.
   i.e look into arch/arm/mach-msm/qdsp5/snd.c
----->>>*/

#define RPC_SND_VERS    0x00020001
#define RPC_SND_PROG    0x30000002
#define RPC_SND_CB_PROG 0x31000002

#define VOCPCM_REGISTER_PCM_INPUT_CLIENT_PROC  24
#define VOCPCM_REGISTER_PCM_OUTPUT_CLIENT_PROC 25

/* <<<----- */
#endif



#define VOC_PCM_CLIENT_OUTPUT_PTR 2
#define VOC_PCM_CLIENT_INPUT_PTR 3
#define MSM_RPC_ENABLE_RECEIVE (0x10000)

#define FRAME_NUM		(12)
#define FRAME_SIZE		(160)
#define BUFFER_NUM		(2)
#define BUFFER_SIZE		(FRAME_NUM * FRAME_SIZE)

#define RPC_TYPE_REQUEST	0
#define RPC_TYPE_REPLY		1

#define RPC_SND_PROG    	0x30000002
#define RPC_SND_CB_PROG 	0x31000002

#define RPC_COMMON_HDR_SZ  	(sizeof(uint32_t) * 2)
#define RPC_REQUEST_HDR_SZ 	(sizeof(struct rpc_request_hdr))
#define RPC_REPLY_HDR_SZ   	(sizeof(uint32_t) * 3)
#define RPC_REPLY_SZ       	(sizeof(uint32_t) * 6)

#define VOC_PCM_INTERFACE_RX_OUTPUT    (0)
#define VOC_PCM_INTERFACE_RX_INPUT     (1)
#define VOC_PCM_INTERFACE_TX_OUTPUT    (2)
#define VOC_PCM_INTERFACE_TX_INPUT     (3)

#define VOC_PCM_CLIENT_OUTPUT_PTR	2
#define VOC_PCM_CLIENT_INPUT_PTR	3

#define VOC_PCM_DATA_STATUS_AVAILABLE     (0)/* Data available for PCM input */
#define VOC_PCM_DATA_STATUS_UNAVAILABLE   (1)  /* Data not available        */
#define VOC_PCM_DATA_STATUS_MAX           (2)

#define VOCPCM_IOCTL_MAGIC 'v'
#define VOCPCM_REGISTER_CLIENT		_IOW(VOCPCM_IOCTL_MAGIC, 0, unsigned)
#define VOCPCM_UNREGISTER_CLIENT	_IOW(VOCPCM_IOCTL_MAGIC, 1, unsigned)


struct buffer {
	uint16_t data[BUFFER_SIZE];
	uint32_t index;
};

struct voc_ctxt {
	struct miscdevice misc;
	struct buffer buf[BUFFER_NUM];
	struct mutex lock;

	spinlock_t dsp_lock;
	wait_queue_head_t wait;
	wait_queue_head_t last_write_wait;

	uint32_t head;
	uint32_t tail;
	uint32_t count;
	int opened;
	int intr;
	int client;
	int final_input;
	uint8_t *s_ptr;
};

struct voc_rpc {
	struct msm_rpc_endpoint *ept;
	struct task_struct *task;
	struct mutex lock;
};

struct vocpcm_register_client_msg {
	struct rpc_request_hdr hdr;
	uint32_t intr;
	uint32_t client_func;
};

struct vocpcm_input_callback_msg {
	uint32_t rep[6];
	uint32_t type;
	uint32_t size;
	uint32_t buf[FRAME_SIZE];
};

static struct voc_rpc the_voc;
static struct vocpcm_input_callback_msg msg;

static struct voc_ctxt *voc_minor_to_ctxt(unsigned n);
static struct voc_ctxt *voc_intr_to_ctxt(unsigned n);
static void   get_vocpcm_data(const uint32_t *, uint32_t, uint32_t);
static void   put_vocpcm_data(uint32_t *, uint32_t, uint32_t, struct msm_rpc_endpoint *, uint32_t);

static void rpc_ack(struct msm_rpc_endpoint *ept, uint32_t xid)
{
        uint32_t rep[6];

        rep[0] = cpu_to_be32(xid);
        rep[1] = cpu_to_be32(1);
        rep[2] = cpu_to_be32(RPCMSG_REPLYSTAT_ACCEPTED);
        rep[3] = cpu_to_be32(RPC_ACCEPTSTAT_SUCCESS);
        rep[4] = 0;
        rep[5] = 0;

        msm_rpc_write(ept, rep, sizeof(rep));
}

static void rpc_in_ack(struct msm_rpc_endpoint *ept, uint32_t xid)
{
        int rc;
        msg.rep[0] = cpu_to_be32(xid);
        msg.rep[1] = cpu_to_be32(1);
        msg.rep[2] = cpu_to_be32(RPCMSG_REPLYSTAT_ACCEPTED);
        msg.rep[3] = cpu_to_be32(RPC_ACCEPTSTAT_SUCCESS);
        msg.rep[4] = cpu_to_be32(0);
        msg.rep[5] = cpu_to_be32(0);
        msg.type = cpu_to_be32(0);
        msg.size = cpu_to_be32(FRAME_SIZE);

        rc = msm_rpc_write(ept, &msg, sizeof(msg));
        if (rc < 0)
                pr_info("rpc_in_ack %d %d\n", rc, sizeof(msg));
}

static void process_rpc_request(uint32_t proc, uint32_t xid,
                                void *data, int len)
{
        uint32_t *x = data;
        uint32_t cb_id, data_len, pointer;

        if (proc == VOC_PCM_CLIENT_INPUT_PTR) {
                cb_id = be32_to_cpu(*x++);
                pointer = be32_to_cpu(*x++);
                data_len = be32_to_cpu(*x++);
                put_vocpcm_data(x, cb_id, data_len, the_voc.ept, xid);
                rpc_in_ack(the_voc.ept, xid);
        } else if (proc == VOC_PCM_CLIENT_OUTPUT_PTR) {
                cb_id = be32_to_cpu(*x++);
                data_len = be32_to_cpu(*x++);
                get_vocpcm_data(x, cb_id, data_len);
                rpc_ack(the_voc.ept, xid);
        } else {
                pr_err("voc_client: unknown rpc proc %d\n", proc);
                rpc_ack(the_voc.ept, xid);
        }
}

static int voc_rpc_thread(void *d)
{
        struct rpc_request_hdr *hdr = NULL;
        uint32_t type;
        int len;

        pr_info("voc_rpc_thread() start\n");
        while (!kthread_should_stop() && the_voc.ept != NULL) {
                if (hdr != NULL) {
                        kfree(hdr);
                        hdr = NULL;
                }
                len = msm_rpc_read(the_voc.ept, (void **) &hdr, -1, -1);
                if (len < 0) {
                        pr_err("voc: rpc read failed (%d)\n", len);
                        break;
                }
                if (len < RPC_COMMON_HDR_SZ)
                        continue;

                type = be32_to_cpu(hdr->type);

                if (type == RPC_TYPE_REPLY) {
                        struct rpc_reply_hdr *rep = (void *) hdr;
                        uint32_t status;
                        if (len < RPC_REPLY_HDR_SZ)
                                continue;
                        status = be32_to_cpu(rep->reply_stat);
                        if (status == RPCMSG_REPLYSTAT_ACCEPTED) {
                                status = be32_to_cpu(rep->
                                                data.acc_hdr.accept_stat);
                                pr_info("voc: rpc_reply status %d\n", status);
                        } else {
                                pr_info("voc: rpc_reply denied!\n");
                        }
                        /* process reply */
                        continue;
                }

                if (len < RPC_REQUEST_HDR_SZ)
                        continue;

                process_rpc_request(be32_to_cpu(hdr->procedure),
                                    be32_to_cpu(hdr->xid),
                                    (void *) (hdr + 1),
                                    len - sizeof(*hdr));

        }

        pr_info("voc_rpc_thread() exit\n");
        if (hdr != NULL) {
                kfree(hdr);
                hdr = NULL;
        }
        return 0;
}

static void put_vocpcm_data(uint32_t *pcm_data, uint32_t cb_id, uint32_t len,
		struct msm_rpc_endpoint *ept, uint32_t xid)
{
	struct voc_ctxt *ctxt = voc_intr_to_ctxt(cb_id);

	unsigned long flags;
	uint32_t buf_index;
	uint32_t data_index;
	struct buffer *frame;
	uint16_t *data;
	int i = 0;

	memset(msg.buf , 0 , FRAME_SIZE);

	if (ctxt == NULL)
		return;

	if (ctxt->count > 0) {

		if (len != FRAME_SIZE) {
			pr_err("len error\n");
			return;
		}
		spin_lock_irqsave(&ctxt->dsp_lock, flags);

		buf_index = ctxt->tail;
		frame = &ctxt->buf[buf_index];
		data_index = frame->index * FRAME_SIZE;
		data = &frame->data[data_index];

		for (i = 0; i < FRAME_SIZE; i++)
			msg.buf[i] = cpu_to_be32(*(data+i));

		if (++frame->index >= FRAME_NUM) {
			frame->index = 0;
			ctxt->tail = (ctxt->tail + 1) & (BUFFER_NUM - 1);
			if (ctxt->head == ctxt->tail) {
				ctxt->head = (ctxt->head + 1) &
						(BUFFER_NUM - 1);
				if (!ctxt->final_input) {
					pr_err(" when head=tail,"
						" head= %d, tail=%d \n",
						ctxt->head, ctxt->tail);
				} else {
					ctxt->count--;
				}
			} else
				ctxt->count--;

			spin_unlock_irqrestore(&ctxt->dsp_lock, flags);
			wake_up_interruptible(&ctxt->wait);
			wake_up_interruptible(&ctxt->last_write_wait);
		} else
			spin_unlock_irqrestore(&ctxt->dsp_lock, flags);
	} else {
		pr_err("rpc miss data and return \n");
	}

}

static void get_vocpcm_data(const uint32_t *pcm_data, uint32_t cb_id, uint32_t len)
{
	struct voc_ctxt *ctxt = voc_intr_to_ctxt(cb_id);
	unsigned long flags;
	uint32_t buf_index;
	uint32_t data_index;
	struct buffer *frame;
	uint16_t *data;
	int i = 0;

	if (ctxt == NULL)
		return;

	buf_index = ctxt->head;
	frame = &ctxt->buf[buf_index];

	if (len != FRAME_SIZE) {
		pr_err("len error\n");
		return;
	}
	spin_lock_irqsave(&ctxt->dsp_lock, flags);
	data_index = frame->index * FRAME_SIZE;
	data = &frame->data[data_index];

	for (i = 0; i < FRAME_SIZE; i++) *(data+i) = be32_to_cpu(*(pcm_data+i));

	if (++frame->index == FRAME_NUM) {
		frame->index = 0;
		ctxt->head = (ctxt->head + 1) & (BUFFER_NUM - 1);
		if (ctxt->head == ctxt->tail) {
			ctxt->tail = (ctxt->tail + 1) & (BUFFER_NUM - 1);
			pr_err(" when head=tail, head= %d, tail=%d \n",
						ctxt->head, ctxt->tail);
		}
		else
			ctxt->count++;
		spin_unlock_irqrestore(&ctxt->dsp_lock, flags);
		wake_up_interruptible(&ctxt->wait);
	} else
		spin_unlock_irqrestore(&ctxt->dsp_lock, flags);
}


static int voc_register_client(struct voc_ctxt *ctxt)
{
	int rc;

	struct vocpcm_register_client_msg msg;
	msg.intr = cpu_to_be32(ctxt->intr);
	msg.client_func = cpu_to_be32(ctxt->intr);

	if (ctxt->intr % 2) {
		msm_rpc_setup_req(&msg.hdr, RPC_SND_PROG, RPC_SND_VERS,
				VOCPCM_REGISTER_PCM_INPUT_CLIENT_PROC);
	} else {
		msm_rpc_setup_req(&msg.hdr, RPC_SND_PROG, RPC_SND_VERS,
				VOCPCM_REGISTER_PCM_OUTPUT_CLIENT_PROC);
	}

	rc = msm_rpc_write(the_voc.ept, &msg, sizeof(msg));
	if (rc < 0) {
		pr_err("%s: %d failed\n", __func__, ctxt->intr);
	} else {
		pr_info("%s: %d success\n", __func__, ctxt->intr);
		ctxt->client = 1;
	}

	return rc;
}

static int voc_unregister_client(struct voc_ctxt *ctxt)
{
	int rc;

	struct vocpcm_register_client_msg msg;
	msg.intr = cpu_to_be32(ctxt->intr);
	msg.client_func = cpu_to_be32(0xffffffff);

	if (ctxt->intr % 2) {
		msm_rpc_setup_req(&msg.hdr, RPC_SND_PROG, RPC_SND_VERS,
				VOCPCM_REGISTER_PCM_INPUT_CLIENT_PROC);
	} else {
		msm_rpc_setup_req(&msg.hdr, RPC_SND_PROG, RPC_SND_VERS,
				VOCPCM_REGISTER_PCM_OUTPUT_CLIENT_PROC);
	}

	rc = msm_rpc_write(the_voc.ept, &msg, sizeof(msg));
	if (rc < 0) {
		pr_err("%s: %d failed\n", __func__, ctxt->intr);
	} else {
		pr_info("%s: %d success\n", __func__, ctxt->intr);
		ctxt->client = 0;
	}
	return rc;
}


static long vocpcm_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct voc_ctxt *ctxt = file->private_data;
	struct buffer *frame;
	unsigned long flags;
	uint32_t index;
	uint32_t data_index;
	uint32_t len = 0;
	uint8_t *dest;
	int rc = 0;

	mutex_lock(&ctxt->lock);
	switch (cmd) {
	case VOCPCM_REGISTER_CLIENT:
		rc = voc_register_client(ctxt);
		break;

	case VOCPCM_UNREGISTER_CLIENT:
		if (ctxt->intr % 2) {
			if (ctxt->s_ptr) {
				index = ctxt->head;
				frame = &ctxt->buf[index];
				data_index = FRAME_NUM * FRAME_SIZE - 1;
				dest = (uint8_t *)&frame->data[data_index] + 1;
				len = dest - ctxt->s_ptr + 1;

				memset(ctxt->s_ptr, 0, len);

				spin_lock_irqsave(&ctxt->dsp_lock, flags);
				frame->index = 0;
				ctxt->head = (ctxt->head + 1) &
						 (BUFFER_NUM - 1);
				ctxt->count++;
				ctxt->final_input = 1;
				spin_unlock_irqrestore(&ctxt->dsp_lock, flags);

				rc = wait_event_interruptible_timeout(
						ctxt->last_write_wait,
						ctxt->count == 0,
						5 * HZ);
				if (rc < 0)
					break;
			}
		} else {
			pr_info("voc: stopping vocpcm_read()\n");
			ctxt->final_input = 1;
			wake_up_interruptible(&ctxt->wait);
		}
		if (ctxt->client)
			rc = voc_unregister_client(ctxt);
		else
			pr_err("voc: no %d client to unregister.", ctxt->intr);
		break;

	default:
		pr_err("voc: unknown command.\n");
		rc = -EINVAL;
		break;
	}
	mutex_unlock(&ctxt->lock);

	return rc;
}

static ssize_t vocpcm_read(struct file *file, char __user *buf,
				size_t count, loff_t *pos)
{
	struct voc_ctxt *ctxt = file->private_data;
	struct buffer *frame;
	const char __user *start = buf;
	unsigned long flags;
	uint32_t index;
	uint32_t data_index;
	uint32_t len = 0;
	uint32_t actual_len = 0;
	uint8_t *dest;
	int rc = 0;

	if (ctxt->intr % 2) {
		return -EFAULT;
	}

	while (count > 0) {

		rc = wait_event_interruptible_timeout(ctxt->wait,
						 ctxt->count > 0 || ctxt->final_input == 1,
						 5 * HZ);
		if (!rc) rc = -ETIMEDOUT;
		if (ctxt->final_input == 1) rc = -EFAULT;

		if (rc < 0)
			break;

		index = ctxt->tail;
		frame = &ctxt->buf[index];

		if (ctxt->s_ptr == NULL)
			ctxt->s_ptr = (uint8_t *)&frame->data;

		data_index = FRAME_NUM * FRAME_SIZE - 1;
		dest = (uint8_t *)&frame->data[data_index] + 1;

		len = dest - ctxt->s_ptr + 1;

		actual_len = (len > count) ? count : len;
		if (copy_to_user(buf, ctxt->s_ptr, actual_len)) {
		rc = -EFAULT;
		break;
		}

		spin_lock_irqsave(&ctxt->dsp_lock, flags);
		if (index != ctxt->tail) {
			/* overrun -- data is invalid
			    and we need to retry */
			spin_unlock_irqrestore(&ctxt->dsp_lock, flags);
			continue;
		}

		if (len > count) {
			ctxt->s_ptr += count;
		} else {
			frame->index = 0;
			ctxt->tail = (ctxt->tail + 1) & (BUFFER_NUM - 1);
			ctxt->count--;
			ctxt->s_ptr = 0;
		}
		spin_unlock_irqrestore(&ctxt->dsp_lock, flags);
		buf += actual_len;
		count -= actual_len;
	}

	if (buf > start)
		return buf - start;

	return rc;
}

static ssize_t vocpcm_write(struct file *file, const char __user *buf,
				   size_t count, loff_t *pos)
{
	struct voc_ctxt *ctxt = file->private_data;
	struct buffer *frame;
	const char __user *start = buf;
	unsigned long flags;
	uint32_t index;
	uint32_t data_index;
	uint32_t len = 0;
	uint32_t actual_len = 0;
	uint8_t *dest;
	int rc = 0;

	if ((ctxt->intr % 2) == 0) {
		return -EFAULT;
	}

	while (count > 0) {
		rc = wait_event_interruptible_timeout(ctxt->wait,
					ctxt->count < BUFFER_NUM,
					5 * HZ);
		if (!rc)
			rc = -ETIMEDOUT;

		if (rc < 0)
			break;

		index = ctxt->head;
		frame = &ctxt->buf[index];

		if (ctxt->s_ptr == NULL)
			ctxt->s_ptr = (uint8_t *)&frame->data;

		data_index = FRAME_NUM * FRAME_SIZE - 1;
		dest = (uint8_t *)&frame->data[data_index] + 1;

		len = dest - ctxt->s_ptr + 1;

		actual_len = (len > count) ? count : len;
		if (copy_from_user(ctxt->s_ptr, buf, actual_len)) {
		rc = -EFAULT;
		break;
		}

		spin_lock_irqsave(&ctxt->dsp_lock, flags);
		if (index != ctxt->head) {
			/* overrun -- data is invalid
			    and we need to retry */
			spin_unlock_irqrestore(&ctxt->dsp_lock, flags);
			continue;
		}

		if (len > count) {
			ctxt->s_ptr += count;
		} else {
			frame->index = 0;
			ctxt->head = (ctxt->head + 1) & (BUFFER_NUM - 1);
			ctxt->count++;
			ctxt->s_ptr = 0;
		}
		spin_unlock_irqrestore(&ctxt->dsp_lock, flags);
		buf += actual_len;
		count -= actual_len;
	}

	if (buf > start)
		return buf - start;

	return rc;
}

static int vocpcm_release(struct inode *inode, struct file *file)
{
	struct voc_ctxt *ctxt = file->private_data;
	mutex_lock(&ctxt->lock);
	if (ctxt->client)
		voc_unregister_client(ctxt);
	ctxt->opened = 0;
	mutex_unlock(&ctxt->lock);
	return 0;
}

static int vocpcm_open(struct inode *inode, struct file *file)
{
	struct voc_ctxt *ctxt = voc_minor_to_ctxt(MINOR(inode->i_rdev));

	if (!ctxt) {
		pr_err("voc: unknown voc misc %d\n", MINOR(inode->i_rdev));
		return -ENODEV;
	}

	mutex_lock(&the_voc.lock);
	if (the_voc.task == NULL) {
		if (the_voc.ept == NULL) 
			the_voc.ept = msm_rpc_connect_compatible(RPC_SND_PROG, RPC_SND_VERS,
					MSM_RPC_UNINTERRUPTIBLE | MSM_RPC_ENABLE_RECEIVE);
		if (IS_ERR(the_voc.ept)) {
			the_voc.ept = NULL;
			pr_err("voc: failed to connect voc svc\n");
			mutex_unlock(&the_voc.lock);
			return -ENODEV;
		}
		the_voc.task = kthread_run(voc_rpc_thread, NULL, "voc_rpc_thread");
		if (IS_ERR(the_voc.task)) {
			the_voc.task = NULL;
			if (the_voc.ept) msm_rpc_close(the_voc.ept);
			the_voc.ept = NULL;
			mutex_unlock(&the_voc.lock);
			return -EFAULT;
		}
	}
	mutex_unlock(&the_voc.lock);

	mutex_lock(&ctxt->lock);
	if (ctxt->opened) {
		pr_err("vocpcm already opened.\n");
		mutex_unlock(&ctxt->lock);
		return -EBUSY;
	}

	file->private_data = ctxt;
	ctxt->head = 0;
	ctxt->tail = 0;
	ctxt->client = 0;

	memset(ctxt->buf[0].data, 0, sizeof(ctxt->buf[0].data));
	memset(ctxt->buf[1].data, 0, sizeof(ctxt->buf[1].data));
	ctxt->buf[0].index = 0;
	ctxt->buf[1].index = 0;
	ctxt->opened = 1;
	ctxt->count = 0;
	ctxt->s_ptr = 0;
	ctxt->final_input = 0;

	mutex_unlock(&ctxt->lock);
	return 0;
}

static struct file_operations vocpcm_fops = {
	.owner		= THIS_MODULE,
	.open		= vocpcm_open,
	.release	= vocpcm_release,
	.read		= vocpcm_read,
	.write		= vocpcm_write,
	.unlocked_ioctl	= vocpcm_ioctl,
};

static struct voc_ctxt vocpcm3 = {
	.misc = {
		.minor	= MISC_DYNAMIC_MINOR,
		.name	= "voc_tx_playback",
		.fops	= &vocpcm_fops,
	}
};

static struct voc_ctxt vocpcm2 = {
	.misc = {
		.minor	= MISC_DYNAMIC_MINOR,
		.name	= "voc_tx_record",
		.fops	= &vocpcm_fops,
	}
};

static struct voc_ctxt vocpcm1 = {
	.misc = {
		.minor	= MISC_DYNAMIC_MINOR,
		.name	= "voc_rx_playback",
		.fops	= &vocpcm_fops,
	}
};

static struct voc_ctxt vocpcm0 = {
	.misc = {
		.minor	= MISC_DYNAMIC_MINOR,
		.name	= "voc_rx_record",
		.fops	= &vocpcm_fops,
	}
};

void voc_ctxt_init(struct voc_ctxt *ctxt, unsigned n)
{
	mutex_init(&ctxt->lock);
	spin_lock_init(&ctxt->dsp_lock);
	init_waitqueue_head(&ctxt->wait);
	init_waitqueue_head(&ctxt->last_write_wait);
	ctxt->intr = n;
}

static struct voc_ctxt *voc_intr_to_ctxt(unsigned n)
{
	if (n == vocpcm0.intr)
		return &vocpcm0;
	if (n == vocpcm1.intr)
		return &vocpcm1;
	if (n == vocpcm2.intr)
		return &vocpcm2;
	if (n == vocpcm3.intr)
		return &vocpcm3;
	return 0;
}

static struct voc_ctxt *voc_minor_to_ctxt(unsigned n)
{
	if (n == vocpcm0.misc.minor)
		return &vocpcm0;
	if (n == vocpcm1.misc.minor)
		return &vocpcm1;
	if (n == vocpcm2.misc.minor)
		return &vocpcm2;
	if (n == vocpcm3.misc.minor)
		return &vocpcm3;
	return 0;
}

static int __init vocpcm_init(void)
{
	int rc;

	voc_ctxt_init(&vocpcm0, 0);
	voc_ctxt_init(&vocpcm1, 1);
	voc_ctxt_init(&vocpcm2, 2);
	voc_ctxt_init(&vocpcm3, 3);

	mutex_init(&the_voc.lock);
	the_voc.ept = NULL;
	the_voc.task = NULL;

	rc = misc_register(&vocpcm0.misc);
	if (rc == 0)
		rc = misc_register(&vocpcm1.misc);
	if (rc == 0)
		rc = misc_register(&vocpcm2.misc);
	if (rc == 0)
		rc = misc_register(&vocpcm3.misc);
	return rc;
}

static void shut_voc(struct voc_ctxt *ctx) 
{
	mutex_lock(&ctx->lock);

        if(ctx->client) voc_unregister_client(ctx);
        misc_deregister(&ctx->misc);

        mutex_unlock(&ctx->lock);
}

static void vocpcm_exit(void) 
{
	mutex_lock(&the_voc.lock);
	if(the_voc.ept != NULL) msm_rpc_close(the_voc.ept);
	the_voc.ept = NULL;
	shut_voc(&vocpcm0);
	shut_voc(&vocpcm1);
	shut_voc(&vocpcm2);
	shut_voc(&vocpcm3);

	mutex_unlock(&the_voc.lock);
}

module_init(vocpcm_init);
module_exit(vocpcm_exit);

MODULE_DESCRIPTION("Incall recording pcm driver");
MODULE_LICENSE("GPL v2");
