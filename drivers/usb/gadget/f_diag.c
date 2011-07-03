/* drivers/usb/gadget/f_diag.c
 *Diag Function Device - Route ARM9 and ARM11 DIAG messages
 *between HOST and DEVICE.
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2008-2009, Code Aurora Forum. All rights reserved.
 * Author: Brian Swetland <swetland@google.com>
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
#include <linux/kernel.h>
#include <linux/platform_device.h>

#include <mach/usbdiag.h>
#include <mach/rpc_hsusb.h>

#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>
#include <linux/workqueue.h>

#define WRITE_COMPLETE 0
#define READ_COMPLETE  0
#define TRUE  1
#define FALSE 0

static DEFINE_SPINLOCK(dev_lock);

static struct usb_interface_descriptor intf_desc = {
	.bLength            =	sizeof intf_desc,
	.bDescriptorType    =	USB_DT_INTERFACE,
	.bNumEndpoints      =	2,
	.bInterfaceClass    =	0xFF,
	.bInterfaceSubClass =	0xFF,
	.bInterfaceProtocol =	0xFF,
};

static struct usb_endpoint_descriptor hs_bulk_in_desc = {
	.bLength 			=	USB_DT_ENDPOINT_SIZE,
	.bDescriptorType 	=	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes 		=	USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize 	=	__constant_cpu_to_le16(512),
	.bInterval 			=	0,
};
static struct usb_endpoint_descriptor fs_bulk_in_desc = {
	.bLength          =	USB_DT_ENDPOINT_SIZE,
	.bDescriptorType  =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes     =	USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize   = __constant_cpu_to_le16(64),
	.bInterval        =	0,
};

static struct usb_endpoint_descriptor hs_bulk_out_desc = {
	.bLength          =	USB_DT_ENDPOINT_SIZE,
	.bDescriptorType  =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes     =	USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize   = __constant_cpu_to_le16(512),
	.bInterval        =	0,
};

static struct usb_endpoint_descriptor fs_bulk_out_desc = {
	.bLength          =	USB_DT_ENDPOINT_SIZE,
	.bDescriptorType  =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_OUT,
	.bmAttributes     =	USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize   = __constant_cpu_to_le16(64),
	.bInterval        =	0,
};

static struct usb_descriptor_header *fs_diag_desc[] = {
	(struct usb_descriptor_header *) &intf_desc,
	(struct usb_descriptor_header *) &fs_bulk_in_desc,
	(struct usb_descriptor_header *) &fs_bulk_out_desc,
	NULL,
	};
static struct usb_descriptor_header *hs_diag_desc[] = {
	(struct usb_descriptor_header *) &intf_desc,
	(struct usb_descriptor_header *) &hs_bulk_in_desc,
	(struct usb_descriptor_header *) &hs_bulk_out_desc,
	NULL,
};
/* list of requests */
struct diag_req_entry {
	struct list_head re_entry;
	struct usb_request *usb_req;
	void *diag_request;
};

struct diag_context {
	struct usb_function function;
	struct usb_ep *out;
	struct usb_ep *in;
	struct usb_endpoint_descriptor  *in_desc;
	struct usb_endpoint_descriptor  *out_desc;
	/* linked list of read requets*/
	struct list_head dev_read_req_list;
	/* linked list of write requets*/
	struct list_head dev_write_req_list;
	struct diag_operations *operations;
	struct work_struct diag_work;
	unsigned diag_configured;
	unsigned char i_serial_number;
	char *serial_number;
	unsigned short  product_id;
};

static void usb_config_work_func(struct work_struct *);
static struct diag_context _context;
static void diag_write_complete(struct usb_ep *,
		struct usb_request *);
static struct diag_req_entry *diag_alloc_req_entry(struct usb_ep *,
		unsigned len, gfp_t);
static void diag_free_req_entry(struct usb_ep *, struct diag_req_entry *);
static void diag_read_complete(struct usb_ep *, struct usb_request *);

static inline struct diag_context *func_to_dev(struct usb_function *f)
{
	return container_of(f, struct diag_context, function);
}

static void diag_function_unbind(struct usb_configuration *c,
		struct usb_function *f)
{
	struct diag_context *ctxt = func_to_dev(f);

	if (!ctxt)
		return;
	if (gadget_is_dualspeed(c->cdev->gadget))
		usb_free_descriptors(f->hs_descriptors);
	usb_free_descriptors(f->descriptors);
}
static void usb_config_work_func(struct work_struct *work)
{
	struct diag_context *ctxt = &_context;
	if ((ctxt->operations) &&
		(ctxt->operations->diag_connect))
			ctxt->operations->diag_connect();
	/*send serial number to A9 sw download, only if serial_number
	* is not null and i_serial_number is non-zero
	*/
	if (ctxt->serial_number && ctxt->i_serial_number) {
		msm_hsusb_is_serial_num_null(FALSE);
		msm_hsusb_send_serial_number(ctxt->serial_number);
	} else
		msm_hsusb_is_serial_num_null(TRUE);
	/* Send product ID to A9 for software download*/
	if (ctxt->product_id)
		msm_hsusb_send_productID(ctxt->product_id);
}
static int diag_function_bind(struct usb_configuration *c,
		struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct diag_context *ctxt = func_to_dev(f);
	struct usb_ep      *ep;
	int status = -ENODEV;

	if (!ctxt)
		return status;

	intf_desc.bInterfaceNumber =  usb_interface_id(c, f);

	ep = usb_ep_autoconfig(cdev->gadget, &fs_bulk_in_desc);
	ctxt->in = ep;
	ep->driver_data = cdev;

	ep = usb_ep_autoconfig(cdev->gadget, &fs_bulk_out_desc);
	ctxt->out = ep;
	ep->driver_data = cdev;

	/* copy descriptors, and track endpoint copies */
	f->descriptors = usb_copy_descriptors(fs_diag_desc);
	if (!f->descriptors)
		goto fail;

	if (gadget_is_dualspeed(c->cdev->gadget)) {
		hs_bulk_in_desc.bEndpointAddress =
				fs_bulk_in_desc.bEndpointAddress;
		hs_bulk_out_desc.bEndpointAddress =
				fs_bulk_out_desc.bEndpointAddress;

		/* copy descriptors, and track endpoint copies */
		f->hs_descriptors = usb_copy_descriptors(hs_diag_desc);
	}
	return 0;
fail:
	if (ctxt->out)
		ctxt->out->driver_data = NULL;
	if (ctxt->in)
		ctxt->in->driver_data = NULL;
	return status;

}
static int diag_function_set_alt(struct usb_function *f,
		unsigned intf, unsigned alt)
{
	struct diag_context  *dev = func_to_dev(f);
	struct usb_composite_dev *cdev = f->config->cdev;
	unsigned long flags;
	int status = -ENODEV;

	if (!dev)
		return status;

	dev->in_desc = ep_choose(cdev->gadget,
			&hs_bulk_in_desc, &fs_bulk_in_desc);
	dev->out_desc = ep_choose(cdev->gadget,
			&hs_bulk_out_desc, &fs_bulk_in_desc);
	usb_ep_enable(dev->in, dev->in_desc);
	usb_ep_enable(dev->out, dev->out_desc);
	dev->i_serial_number = cdev->desc.iSerialNumber;
	dev->product_id   = cdev->desc.idProduct;
	schedule_work(&dev->diag_work);

	spin_lock_irqsave(&dev_lock , flags);
	dev->diag_configured = 1;
	spin_unlock_irqrestore(&dev_lock , flags);

	return 0;
}
static void diag_function_disable(struct usb_function *f)
{
	struct diag_context  *dev = func_to_dev(f);
	unsigned long flags;

	printk(KERN_INFO "diag_function_disable\n");

	spin_lock_irqsave(&dev_lock , flags);
	dev->diag_configured = 0;
	spin_unlock_irqrestore(&dev_lock , flags);

	if (dev->in) {
		usb_ep_fifo_flush(dev->in);
		usb_ep_disable(dev->in);
		dev->in->driver_data = NULL;
	}
	if (dev->out) {
		usb_ep_fifo_flush(dev->out);
		usb_ep_disable(dev->out);
		dev->out->driver_data = NULL;
	}
	if ((dev->operations) &&
		(dev->operations->diag_disconnect))
			dev->operations->diag_disconnect();
}
int diag_usb_register(struct diag_operations *func)
{
	struct diag_context *ctxt = &_context;
	unsigned long flags;
	int connected;

	if (func == NULL) {
		printk(KERN_ERR "%s:registering"
				"diag char operations NULL\n", __func__);
		return -1;
	}
	ctxt->operations = func;
	spin_lock_irqsave(&dev_lock , flags);
	connected = ctxt->diag_configured;
	spin_unlock_irqrestore(&dev_lock , flags);

	if (connected)
		if ((ctxt->operations) &&
			(ctxt->operations->diag_connect))
				ctxt->operations->diag_connect();
	return 0;
}
EXPORT_SYMBOL(diag_usb_register);

int diag_usb_unregister(void)
{
	struct diag_context *ctxt = &_context;

	ctxt->operations = NULL;
	return 0;
}
EXPORT_SYMBOL(diag_usb_unregister);

int diag_open(int num_req)
{
	struct diag_context *ctxt = &_context;
	struct diag_req_entry *write_entry;
	struct diag_req_entry *read_entry;
	int i = 0;

	for (i = 0; i < num_req; i++) {
		write_entry = diag_alloc_req_entry(ctxt->in, 0, GFP_KERNEL);
		if (write_entry) {
			write_entry->usb_req->complete = diag_write_complete;
			list_add(&write_entry->re_entry,
					&ctxt->dev_write_req_list);
		} else
			goto write_error;
	}

	for (i = 0; i < num_req ; i++) {
		read_entry = diag_alloc_req_entry(ctxt->out, 0 , GFP_KERNEL);
		if (read_entry) {
			read_entry->usb_req->complete = diag_read_complete;
			list_add(&read_entry->re_entry ,
					&ctxt->dev_read_req_list);
		} else
			goto read_error;
		}
	return 0;
read_error:
	printk(KERN_ERR "%s:read requests allocation failure\n", __func__);
	while (!list_empty(&ctxt->dev_read_req_list)) {
		read_entry = list_entry(ctxt->dev_read_req_list.next,
				struct diag_req_entry, re_entry);
		list_del(&read_entry->re_entry);
		diag_free_req_entry(ctxt->out, read_entry);
	}
write_error:
	printk(KERN_ERR "%s: write requests allocation failure\n", __func__);
	while (!list_empty(&ctxt->dev_write_req_list)) {
		write_entry = list_entry(ctxt->dev_write_req_list.next,
				struct diag_req_entry, re_entry);
		list_del(&write_entry->re_entry);
		diag_free_req_entry(ctxt->in, write_entry);
	}
	return -ENOMEM;
}
EXPORT_SYMBOL(diag_open);

void diag_close(void)
{
	struct diag_context *ctxt = &_context;
	struct diag_req_entry *req_entry;
	/* free write requests */

	while (!list_empty(&ctxt->dev_write_req_list)) {
		req_entry = list_entry(ctxt->dev_write_req_list.next,
				struct diag_req_entry, re_entry);
		list_del(&req_entry->re_entry);
		diag_free_req_entry(ctxt->in, req_entry);
	}

	/* free read requests */
	while (!list_empty(&ctxt->dev_read_req_list)) {
		req_entry = list_entry(ctxt->dev_read_req_list.next,
				struct diag_req_entry, re_entry);
		list_del(&req_entry->re_entry);
		diag_free_req_entry(ctxt->out, req_entry);
	}
	return;
}
EXPORT_SYMBOL(diag_close);

static void diag_free_req_entry(struct usb_ep *ep,
		struct diag_req_entry *req)
{
	if (req) {
		if (ep && req->usb_req)
			usb_ep_free_request(ep, req->usb_req);
		kfree(req);
	}
}

static struct diag_req_entry *diag_alloc_req_entry(struct usb_ep *ep,
		unsigned len, gfp_t kmalloc_flags)
{
	struct diag_req_entry *req;

	req = kmalloc(sizeof(struct diag_req_entry), kmalloc_flags);
	if (req == NULL)
		return NULL;

	req->usb_req  =  usb_ep_alloc_request(ep, GFP_KERNEL);
	if (req->usb_req == NULL) {
		kfree(req);
		return NULL;
	}
	req->usb_req->context = req;
	return req;
}

int diag_read(struct diag_request *d_req)
{
	unsigned long flags;
	struct usb_request *req = NULL;
	struct diag_req_entry *req_entry = NULL;
	struct diag_context *ctxt = &_context;


	spin_lock_irqsave(&dev_lock , flags);
	if (!ctxt->diag_configured) {
		spin_unlock_irqrestore(&dev_lock , flags);
		return -EIO;
	}
	if (!list_empty(&ctxt->dev_read_req_list)) {
		req_entry = list_entry(ctxt->dev_read_req_list.next ,
				struct diag_req_entry , re_entry);
		req_entry->diag_request = d_req;
		req = req_entry->usb_req;
		list_del(&req_entry->re_entry);
	}
	spin_unlock_irqrestore(&dev_lock , flags);
	if (req) {
		req->buf = d_req->buf;
		req->length = d_req->length;
		if (usb_ep_queue(ctxt->out, req, GFP_ATOMIC)) {
			/* If error add the link to the linked list again. */
			spin_lock_irqsave(&dev_lock , flags);
			list_add_tail(&req_entry->re_entry ,
					&ctxt->dev_read_req_list);
			spin_unlock_irqrestore(&dev_lock , flags);
			printk(KERN_ERR "%s:can't queue request\n", __func__);
			return -EIO;
		}
	} else {
		printk(KERN_ERR
				"%s:no requests avialable\n", __func__);
		return -EIO;
	}
	return 0;
}
EXPORT_SYMBOL(diag_read);

int diag_write(struct diag_request *d_req)
{
	unsigned long flags;
	struct usb_request *req = NULL;
	struct diag_req_entry *req_entry = NULL;
	struct diag_context *ctxt = &_context;

	spin_lock_irqsave(&dev_lock , flags);
	if (!ctxt->diag_configured) {
		spin_unlock_irqrestore(&dev_lock , flags);
		return -EIO;
	}
	if (!list_empty(&ctxt->dev_write_req_list)) {
		req_entry = list_entry(ctxt->dev_write_req_list.next ,
				struct diag_req_entry , re_entry);
		req_entry->diag_request = d_req;
		req = req_entry->usb_req;
		list_del(&req_entry->re_entry);
	}
	spin_unlock_irqrestore(&dev_lock, flags);
	if (req) {
		req->buf = d_req->buf;
		req->length = d_req->length;
		if (usb_ep_queue(ctxt->in, req, GFP_ATOMIC)) {
			/* If error add the link to linked list again*/
			spin_lock_irqsave(&dev_lock, flags);
			list_add_tail(&req_entry->re_entry ,
					&ctxt->dev_write_req_list);
			spin_unlock_irqrestore(&dev_lock, flags);
			printk(KERN_ERR "%s: cannot queue"
					" read request\n", __func__);
			return -EIO;
		}
	} else {
		printk(KERN_ERR	"%s: no requests available\n", __func__);
		return -EIO;
	}
	return 0;
}
EXPORT_SYMBOL(diag_write);

static void diag_write_complete(struct usb_ep *ep ,
		struct usb_request *req)
{
	struct diag_context *ctxt = &_context;
	struct diag_req_entry *diag_req = req->context;
	struct diag_request *d_req = (struct diag_request *)
						diag_req->diag_request;
	unsigned long flags;

	if (ctxt == NULL) {
		printk(KERN_ERR "%s: requesting"
				"NULL device pointer\n", __func__);
		return;
	}
	if (req->status == WRITE_COMPLETE) {
		if ((req->length >= ep->maxpacket) &&
				((req->length % ep->maxpacket) == 0)) {
			req->length = 0;
			/* req->device = ctxt; */
			d_req->actual = req->actual;
			d_req->status = req->status;
			/* Queue zero length packet */
			usb_ep_queue(ctxt->in, req, GFP_ATOMIC);
			return;
		}
	}
	spin_lock_irqsave(&dev_lock, flags);
	list_add_tail(&diag_req->re_entry ,
			&ctxt->dev_write_req_list);
	if (req->length != 0) {
		d_req->actual = req->actual;
		d_req->status = req->status;
	}
	spin_unlock_irqrestore(&dev_lock , flags);
	if ((ctxt->operations) &&
		(ctxt->operations->diag_char_write_complete))
			ctxt->operations->diag_char_write_complete(
				d_req);
}
static void diag_read_complete(struct usb_ep *ep ,
		struct usb_request *req)
{
	 struct diag_context *ctxt = &_context;
	 struct diag_req_entry *diag_req = req->context;
	 struct diag_request *d_req = (struct diag_request *)
							diag_req->diag_request;
	 unsigned long flags;

	if (ctxt == NULL) {
		printk(KERN_ERR "%s: requesting"
				"NULL device pointer\n", __func__);
		return;
	}
	spin_lock_irqsave(&dev_lock, flags);
	list_add_tail(&diag_req->re_entry ,
			&ctxt->dev_read_req_list);
	d_req->actual = req->actual;
	d_req->status = req->status;
	spin_unlock_irqrestore(&dev_lock, flags);
	if ((ctxt->operations) &&
		(ctxt->operations->diag_char_read_complete))
			ctxt->operations->diag_char_read_complete(
				d_req);
}
int diag_function_add(struct usb_configuration *c,
				char *serial_number)
{
	struct diag_context *dev = &_context;
	int ret;

	printk(KERN_INFO "%s\n", __func__);
	dev->function.name = "diag";
	dev->function.descriptors = fs_diag_desc;
	dev->function.hs_descriptors = hs_diag_desc;
	dev->function.bind = diag_function_bind;
	dev->function.unbind = diag_function_unbind;
	dev->function.set_alt = diag_function_set_alt;
	dev->function.disable = diag_function_disable;
	dev->serial_number    = serial_number;
	INIT_LIST_HEAD(&dev->dev_read_req_list);
	INIT_LIST_HEAD(&dev->dev_write_req_list);
	INIT_WORK(&dev->diag_work, usb_config_work_func);
	ret = usb_add_function(c, &dev->function);
	if (ret)
		goto err1;
	return 0;
err1:
	printk(KERN_ERR "diag gadget driver failed to initialize\n");
	return ret;
}
