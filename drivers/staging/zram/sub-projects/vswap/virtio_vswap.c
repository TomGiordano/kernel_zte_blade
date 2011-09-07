/*
 * Virtio vswap driver.
 *
 *  Copyright 2009 Nitin Gupta <ngupta@vflare.org>
 */

#define KMSG_COMPONENT "vswap"
#define pr_fmt(fmt) KMSG_COMPONENT ": " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/blkdev.h>
#include <linux/buffer_head.h>
#include <linux/crypto.h>
#include <linux/debugfs.h>
#include <linux/device.h>
#include <linux/genhd.h>
#include <linux/mutex.h>
#include <linux/relay.h>
#include <linux/scatterlist.h>
#include <linux/swap.h>

#include <linux/virtio.h>
#include <linux/virtio_config.h>

#include "virtio_vswap.h"

#define VSWAP_SECTOR_SHIFT	9
#define VSWAP_SECTOR_SIZE	(1 << VSWAP_SECTOR_SHIFT)
#define VSWAP_SECTORS_PER_PAGE	(PAGE_SIZE / VSWAP_SECTOR_SIZE)

/* Module param */
static char *backing_swap;

/* Globals */
static int major;
size_t disk_size;
struct block_device *backing_bdev;

struct virtio_vswap
{
	struct virtio_device *vdev;
	struct virtqueue *vq;

	struct block_device *bdev;
	struct gendisk *disk;
};

struct vswap_req
{
	struct virtio_vswap_outhdr out_hdr;
	struct virtio_vswap_inhdr in_hdr;
	
	struct completion acked;
};

static struct block_device_operations vswap_devops = {
	.owner = THIS_MODULE,
};

static struct virtio_device_id id_table[] = {
	{ VIRTIO_ID_VSWAP, VIRTIO_DEV_ANY_ID },
	{ 0 },
};

static void vswap_ack(struct virtqueue *vq)
{
	struct vswap_req *vsr;
	unsigned int len;
	
	//pr_info("vswap_ack called\n");

	vsr = vq->vq_ops->get_buf(vq, &len);
	if (vsr)
		complete(&vsr->acked);
}

static int try_swap_to_host(struct request_queue *queue, struct bio *bio)
{
	unsigned long out = 0, in = 0;
	struct scatterlist sg[3];
	struct virtio_vswap *vswap = queue->queuedata;
	struct virtqueue *vq = vswap->vq;
	struct vswap_req *vsr;

	vsr = kzalloc(sizeof(*vsr), GFP_ATOMIC);

	vsr->out_hdr.type = 0;
	vsr->out_hdr.sector = bio->bi_sector;

	sg_set_buf(&sg[out++], &vsr->out_hdr, sizeof(vsr->out_hdr));
	sg_set_page(&sg[out], bio->bi_io_vec[0].bv_page, PAGE_SIZE, 0);

	if (bio_data_dir(bio) == WRITE) {
		vsr->out_hdr.type |= VIRTIO_VSWAP_T_OUT;
		out++;
	} else {
		vsr->out_hdr.type |= VIRTIO_VSWAP_T_IN;
		in++;		
	}
	
	vsr->in_hdr.status = 0;
	sg_set_buf(&sg[out + in++], &vsr->in_hdr, sizeof(vsr->in_hdr));
	
	init_completion(&vsr->acked);
	
	if (vq->vq_ops->add_buf(vq, sg, out, in, vsr) < 0) {
		kfree(vsr);
		pr_err("Error adding buffer\n");
		return -ENOMEM;
	}
	vq->vq_ops->kick(vq);
	
	wait_for_completion(&vsr->acked);
	kfree(vsr);

	/* FIXME: for now always return error */
	return -ENOMEM;
}

static int vswap_make_request(struct request_queue *queue, struct bio *bio)
{
	int ret;
#if 0
	pr_debug("bio: dir=%lu, sector=%lu\n", bio_data_dir(bio),
				(unsigned long)(bio->bi_sector));
#endif
	if (unlikely(bio->bi_sector & (VSWAP_SECTORS_PER_PAGE - 1)
			|| bio->bi_vcnt != 1
			|| bio->bi_size != PAGE_SIZE
			|| bio->bi_io_vec[0].bv_offset != 0)) {
		/*
		 * This is not a swap in/out request.
		 * So, don't collect any data - just fwd to backing device.
		 */
		goto fwd_request;
	}
	
	ret = try_swap_to_host(queue, bio);
	if (!ret) {
		bio_endio(bio, 0);
		return 0;
	}

	if (bio_data_dir(bio) == WRITE) {
		/* TODO */
	} else {
		/* TODO */
	}

fwd_request:
	bio->bi_bdev = backing_bdev;
	return 1;
}

static int setup_backing_device(void)
{
	int ret = 0;
	struct inode *inode;
	struct file *swap_file;
	struct address_space *mapping;

	pr_info("backing_swap=%s\n", backing_swap);

	if (backing_swap == NULL) {
		pr_err("backing_dev param missing\n");
		ret = -EINVAL;
		goto bad_param;
	}

	swap_file = filp_open(backing_swap, O_RDWR | O_LARGEFILE, 0);

	if (IS_ERR(swap_file)) {
		pr_err("Error open backing device file: %s\n", backing_swap);
		ret = -EINVAL;
		goto bad_param;
	}

	mapping = swap_file->f_mapping;
	inode = mapping->host;

	if (S_ISBLK(inode->i_mode)) {
		backing_bdev = I_BDEV(inode);
		ret = bd_claim(backing_bdev, setup_backing_device);
		if (ret < 0) {
			backing_bdev = NULL;
			ret = -EINVAL;
			goto bad_param;
		}
	} else if (S_ISREG(inode->i_mode)) {
		backing_bdev = inode->i_sb->s_bdev;
		if (IS_SWAPFILE(inode)) {
			ret = -EINVAL;
			goto bad_param;
		}
	}

	disk_size = i_size_read(inode) >> PAGE_SHIFT;

	ret = 0;
	goto out;

bad_param:
	if (backing_bdev)
		bd_release(backing_bdev);

out:
	return ret;
}

static int setup_vswap_device(struct virtio_vswap *vswap)
{
	int ret;

	vswap->disk = alloc_disk(1);
	if (vswap->disk == NULL) {
		pr_err("Error allocating disk structure\n");
		ret = -ENOMEM;
		goto out;
	}

	vswap->disk->major = major;
	vswap->disk->first_minor = 0;
	vswap->disk->fops = &vswap_devops;
	sprintf(vswap->disk->disk_name, "%s", "vswap");

	vswap->disk->queue = blk_alloc_queue(GFP_KERNEL);
	if (vswap->disk->queue == NULL) {
		pr_err("Cannot register disk queue\n");
		ret = -EFAULT;
		goto out_put_disk;
	}
	vswap->disk->queue->queuedata = vswap;

	blk_queue_make_request(vswap->disk->queue, vswap_make_request);

	set_capacity(vswap->disk,
		disk_size << (PAGE_SHIFT - VSWAP_SECTOR_SHIFT));
	add_disk(vswap->disk);

	return 0;

out_put_disk:
	put_disk(vswap->disk);
out:
	return ret;
}

static int vswap_probe(struct virtio_device *vdev)
{
	int ret;
	struct virtio_vswap *vswap;

	pr_info("probe called\n");

	vdev->priv = vswap = kzalloc(sizeof(*vswap), GFP_KERNEL);
	if (!vswap) {
		ret = -ENOMEM;
		goto out;
	}
	vswap->vdev = vdev;

	/* Dummy driver. Just one virtqueue. */
	vswap->vq = vdev->config->find_vq(vdev, 0, vswap_ack);
	if (IS_ERR(vswap->vq)) {
		ret = PTR_ERR(vswap->vq);
		goto out_free_vswap;
	}
	
	ret = setup_vswap_device(vswap);
	if (ret < 0)
		goto out_free_vq;

	return 0;

out_free_vq:
	vdev->config->del_vq(vswap->vq);
out_free_vswap:
	kfree(vswap);
out:
	return ret;
}

static void __devexit vswap_remove(struct virtio_device *vdev)
{
	struct virtio_vswap *vswap = vdev->priv;
	pr_info("remove called\n");
	
	/* Stop all virtqueues */
	vdev->config->reset(vdev);
	
	del_gendisk(vswap->disk);
	blk_cleanup_queue(vswap->disk->queue);
	put_disk(vswap->disk);
	vdev->config->del_vq(vswap->vq);
	kfree(vswap);
}

static void vswap_changed(struct virtio_device *vdev)
{
	pr_info("changed called\n");
}

static struct virtio_driver virtio_vswap = {
	.feature_table = NULL,
	.feature_table_size = 0,
	.driver.name =	KBUILD_MODNAME,
	.driver.owner = THIS_MODULE,
	.id_table = id_table,
	.probe = vswap_probe,
	.remove = __devexit_p(vswap_remove),
	.config_changed = vswap_changed,
};

static int __init vswap_init(void)
{
	int ret;
	
	ret = setup_backing_device();
	if (ret)
		goto out;

	major = register_blkdev(0, "vswap");
	if (major < 0) {
		ret = major;
		goto out_free_backing;
	}
	
	ret = register_virtio_driver(&virtio_vswap);
	if (ret < 0)
		goto out_free_bdev;

	return 0;

out_free_bdev:
	unregister_blkdev(major, "vswap");
out_free_backing:
	bd_release(backing_bdev);
out:
	return ret;
}

static void __exit vswap_exit(void)
{
	pr_info("exit called\n");

	bd_release(backing_bdev);
	unregister_blkdev(major, "vswap");	
	unregister_virtio_driver(&virtio_vswap);
}

module_param(backing_swap, charp, 0);
MODULE_PARM_DESC(backing_swap, "Backing swap partition");

module_init(vswap_init);
module_exit(vswap_exit);

MODULE_DEVICE_TABLE(virtio, id_table);
MODULE_DESCRIPTION("Virtio vswap driver");
MODULE_LICENSE("GPL");
