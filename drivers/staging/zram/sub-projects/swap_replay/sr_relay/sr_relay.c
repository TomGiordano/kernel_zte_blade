/*
 * Relay swap events and data to userspace
 *
 * (C) Nitin Gupta
 *
 */

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
#include <asm/string.h>

#include "compat.h"
#include "sr_relay.h"
#include "../../compression/lzo-kmod/lzo.h"

#define RELAY_BUFFER_SIZE	PAGE_SIZE
#define RELAY_NUM_BUFFERS	16

#define SR_SECTOR_SHIFT		9
#define SR_SECTOR_SIZE		(1 << SR_SECTOR_SHIFT)
#define SR_SECTORS_PER_PAGE	(PAGE_SIZE / SR_SECTOR_SIZE)

/* things required to get md5 hash of a page */
struct hash_req {
	struct scatterlist sg[1];
	struct crypto_hash *tfm;	// transform
	struct hash_desc desc;
};

static struct hash_req hash_req;

int get_md5_hash_page(struct page *page, char *outbuf);

struct sr_dev_info {
	struct rchan *relay;
	struct block_device *b_dev, *sr_dev;
	struct gendisk *disk;
	struct mutex lock;
	void *cdata, *cworkmem;	// temp buffers for compressor
	unsigned long drop_count;
	int old_block_size;
	int disk_size;
};

static struct block_device_operations sr_devops = {
	.owner = THIS_MODULE,
};

static unsigned int first_write_done;
static char *backing_dev;	// Module param
static struct sr_dev_info sr;
static struct sr_info sr_info;

static int __init sr_relay_init(void);

static int sr_subbuf_start_callback(struct rchan_buf *buf, void *subbuf,
				void *prev_subbuf, unsigned int prev_padding)
{
	if (relay_buf_full(buf)) {
#if 0
		pr_debug("Dropped event count=%lu\n", ++sr.drop_count);
#endif
		return 0;
	}

	return 1;
}

static struct dentry *sr_create_buf_file_callback(const char *filename,
							struct dentry *parent,
							int mode,
							struct rchan_buf *buf,
							int *is_global)
{
	*is_global = 1;
	return debugfs_create_file(filename, mode, parent, buf,
					&relay_file_operations);
}

static int sr_remove_buf_file_callback(struct dentry *dentry)
{
	debugfs_remove(dentry);
	return 0;
}

static struct rchan_callbacks sr_relay_callbacks = {
	.subbuf_start		= sr_subbuf_start_callback,
	.create_buf_file	= sr_create_buf_file_callback,
	.remove_buf_file	= sr_remove_buf_file_callback,
};

static int sr_make_request(struct request_queue *queue, struct bio *bio)
{
	int ret;
	struct page *page;

#if 0
	pr_debug("Fwd request: dir=%lu, sector=%lu\n",
				bio_data_dir(bio),
				(unsigned long)(bio->bi_sector));
#endif
	if (unlikely(bio->bi_sector & (SR_SECTORS_PER_PAGE - 1)
			|| bio->bi_vcnt != 1
			|| bio->bi_size != PAGE_SIZE
			|| bio->bi_io_vec[0].bv_offset != 0)) {
		/*
		 * This is not swap in/out request.
		 * So, don't collect any data - just fwd to backing device.
		 */
		goto fwd_request;
	}
	
	mutex_lock(&sr.lock);

	// sector no. is always multiple of SR_SECTORS_PER_PAGE
	sr_info.page_rw = ((bio->bi_sector / SR_SECTORS_PER_PAGE) << 1)
				| bio_data_dir(bio);
	sr_info.clen = 0;

	if (bio_data_dir(bio) == WRITE) {
		if (unlikely(!first_write_done))
			first_write_done = 1;

		page = bio->bi_io_vec[0].bv_page;
		/* Record compressed length */
		ret = lzo1x_1_compress(page_address(page), PAGE_SIZE,
					sr.cdata, &sr_info.clen,
					sr.cworkmem);

		/* Should never happen! */
		if (ret != LZO_E_OK)
			pr_info("Error compressing page!\n");

		/* Record MD5 hash: 128-bit */
		ret = get_md5_hash_page(page, &sr_info.hash[0]);
		if (ret)
			pr_info("MD5 hash failed!\n");	
	} else {
		/*
		 * Read on swap device before any write in meaningless.
		 * As soon as module is loaded, first few sectors are read -
		 * We don't want to record these spurious reads.
		 */
		if (!first_write_done) {
			mutex_unlock(&sr.lock);
			goto fwd_request;
		}
	}

	__relay_write(sr.relay, &sr_info, sizeof(struct sr_info));

	mutex_unlock(&sr.lock);

fwd_request:
	bio->bi_bdev = sr.b_dev;
	return 1;
}

static int setup_backing_device(void)
{
	int error = 0;
	struct inode *inode;
	struct file *swap_file;
	struct address_space *mapping;

	pr_info("backing_dev=%s\n", backing_dev);

	if (backing_dev == NULL) {
		pr_err("backing_dev param missing\n");
		error = -EINVAL;
		goto bad_param;
	}

	swap_file = filp_open(backing_dev, O_RDWR | O_LARGEFILE, 0);

	if (IS_ERR(swap_file)) {
		pr_err("Error open backing device file: %s\n", backing_dev);
		error = -EINVAL;
		goto bad_param;
	}

	mapping = swap_file->f_mapping;
	inode = mapping->host;

	if (S_ISBLK(inode->i_mode)) {
		sr.b_dev = I_BDEV(inode);
		error = bd_claim(sr.b_dev, sr_relay_init);
		if (error < 0) {
			sr.b_dev = NULL;
			error = -EINVAL;
			goto bad_param;
		}
		sr.old_block_size = block_size(sr.b_dev);
		error = set_blocksize(sr.b_dev, PAGE_SIZE);
		if (error < 0)
			goto bad_param;
	} else if (S_ISREG(inode->i_mode)) {
		sr.b_dev = inode->i_sb->s_bdev;
		if (IS_SWAPFILE(inode)) {
			error = -EBUSY;
			goto bad_param;
		}
	}

	sr.disk_size = i_size_read(inode) >> PAGE_SHIFT;

	error = 0;
	goto out;

bad_param:
	if (sr.b_dev) {
		set_blocksize(sr.b_dev, sr.old_block_size);
		bd_release(sr.b_dev);
	}

out:
	return error;
}

static int setup_sr_device(void)
{
	int error;

	sr.disk = alloc_disk(1);
	if (sr.disk == NULL) {
		pr_err("Error allocating disk structure\n");
		error = -ENOMEM;
		goto out;
	}

	sr.disk->first_minor = 0;
	sr.disk->fops = &sr_devops;
	sprintf(sr.disk->disk_name, "%s", "sr_relay");

	sr.disk->major = register_blkdev(0, sr.disk->disk_name);
	if (sr.disk->major < 0) {
		pr_err("Cannot register block device\n");
		error = -EFAULT;
		goto out;
	}

	sr.disk->queue = blk_alloc_queue(GFP_KERNEL);
	if (sr.disk->queue == NULL) {
		pr_err("Cannot register disk queue\n");
		error = -EFAULT;
		goto out;
	}

	blk_queue_make_request(sr.disk->queue, sr_make_request);
	set_capacity(sr.disk, sr.disk_size << (PAGE_SHIFT - SR_SECTOR_SHIFT));
	add_disk(sr.disk);

	error = 0;
	goto out;

out:
	return error;
}

static int setup_sr_relay(void)
{
	sr.relay = relay_open("sr_relay", NULL, RELAY_BUFFER_SIZE,
			RELAY_NUM_BUFFERS, &sr_relay_callbacks, NULL);
	if (!sr.relay) {
		pr_err("Error creating relay channel\n");
		return -ENOMEM;
	}

	return 0;
}

static int init_hash(void)
{
	hash_req.tfm = crypto_alloc_hash("md5", 0, 0);
	if (IS_ERR(hash_req.tfm)) {
		printk("failed to load transform for md5: %ld\n",
						PTR_ERR(hash_req.tfm));
		return -ENOMEM;
	}

	hash_req.desc.tfm = hash_req.tfm;
	hash_req.desc.flags = 0;

	return 0;
}

static int __init sr_relay_init(void)
{
	int error;

	mutex_init(&sr.lock);

	// Temp memory for compressor
	sr.cdata = kmalloc(2 * PAGE_SIZE, GFP_KERNEL);
	sr.cworkmem = vmalloc(LZO1X_1_MEM_COMPRESS);
	if (sr.cdata == NULL || sr.cworkmem == NULL) {
		error = -ENOMEM;
		goto fail_alloc;
	}

	error = setup_backing_device();
	if (error)
		goto out;

	error = setup_sr_device();
	if (error)
		goto fail_sr_dev;

	error = setup_sr_relay();
	if (error)
		goto fail_relay;

	error = init_hash();
	if (error)
		goto fail_hash_init;

	error = 0;
	goto out;

fail_hash_init:
fail_relay:
	unregister_blkdev(sr.disk->major, sr.disk->disk_name);
	del_gendisk(sr.disk);
	
fail_sr_dev:
	set_blocksize(sr.b_dev, sr.old_block_size);
	bd_release(sr.b_dev);

fail_alloc:
	if (sr.cdata)
		kfree(sr.cdata);
	if (sr.cworkmem)
		vfree(sr.cworkmem);

out:
	return error;
}

static void __exit sr_relay_exit(void)
{
	set_blocksize(sr.b_dev, sr.old_block_size);
	bd_release(sr.b_dev);

	unregister_blkdev(sr.disk->major, sr.disk->disk_name);
	del_gendisk(sr.disk);

	relay_close(sr.relay);

	crypto_free_hash(hash_req.tfm);
	
	kfree(sr.cdata);
	vfree(sr.cworkmem);
}

/*
 * MD5 sum is 128-bit, so outbuf is assumed to be atleast this big
 */
int get_md5_hash_page(struct page *page, char *outbuf)
{
	int ret;
	struct scatterlist sg[1];
	sg_init_one(&sg[0], page_address(page), PAGE_SIZE);

	ret = crypto_hash_digest(&hash_req.desc, sg, PAGE_SIZE, outbuf);
	return ret;
}

module_param(backing_dev, charp, 0);
MODULE_PARM_DESC(backing_dev, "Swap partition");

module_init(sr_relay_init);
module_exit(sr_relay_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nitin Gupta <nitingupta910@gmail.com>");
MODULE_DESCRIPTION("Relay swap events and data to userspace");
