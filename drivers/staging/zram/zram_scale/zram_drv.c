/*
 * Compressed RAM block device
 *
 * Copyright (C) 2008, 2009, 2010  Nitin Gupta
 *
 * This code is released using a dual license strategy: BSD/GPL
 * You can choose the licence that better fits your requirements.
 *
 * Released under the terms of 3-clause BSD License
 * Released under the terms of GNU General Public License Version 2.0
 *
 * Project home: http://compcache.googlecode.com/
 */

#define KMSG_COMPONENT "zram"
#define pr_fmt(fmt) KMSG_COMPONENT ": " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/bitops.h>
#include <linux/blkdev.h>
#include <linux/buffer_head.h>
#include <linux/cpu.h>
#include <linux/device.h>
#include <linux/genhd.h>
#include <linux/highmem.h>
#include <linux/notifier.h>
#include <linux/slab.h>
#include <linux/lzo.h>
#include <linux/string.h>
#include <linux/vmalloc.h>

#include "compat.h"
#include "zram_drv.h"

static DEFINE_PER_CPU(unsigned char *, compress_buffer);
static DEFINE_PER_CPU(unsigned char *, compress_workmem);

/* Globals */
static int zram_major;
struct zram *devices;

/* Module params (documentation at end) */
unsigned int num_devices;

/*
 * We do not allocate any memory for zero-filled pages.
 * Rather, we simply mark them in corresponding table
 * entry by setting this bit.
 */
#define ZRAM_ZERO_PAGE_MARK_BIT		(1 << 0)

static void zram_add_stat(struct zram *zram,
			enum zram_stats_index idx, s64 val)
{
	struct zram_stats_cpu *stats;

	preempt_disable();
	stats = __this_cpu_ptr(zram->stats);
	u64_stats_update_begin(&stats->syncp);
	stats->count[idx] += val;
	u64_stats_update_end(&stats->syncp);
	preempt_enable();
}

static void zram_inc_stat(struct zram *zram, enum zram_stats_index idx)
{
	zram_add_stat(zram, idx, 1);
}

static void zram_dec_stat(struct zram *zram, enum zram_stats_index idx)
{
	zram_add_stat(zram, idx, -1);
}

static int page_zero_filled(void *ptr)
{
	unsigned int pos;
	unsigned long *page;

	page = (unsigned long *)ptr;

	for (pos = 0; pos != PAGE_SIZE / sizeof(*page); pos++) {
		if (page[pos])
			return 0;
	}

	return 1;
}

static int zram_is_zero_page(struct zram *zram, u32 index)
{
	phys_addr_t addr = zram->table[index].addr;

	return addr & ZRAM_ZERO_PAGE_MARK_BIT;
}

static void zram_set_zero_page(struct zram *zram, u32 index)
{
	zram->table[index].addr |= ZRAM_ZERO_PAGE_MARK_BIT;
}

static void zram_clear_zero_page(struct zram *zram, u32 index)
{
	zram->table[index].addr &= ~ZRAM_ZERO_PAGE_MARK_BIT;
}

static void zram_find_obj(struct zram *zram, u32 index, struct page **page,
			u32 *offset)
{
	phys_addr_t addr = zram->table[index].addr;

	if (!addr) {
		*page = NULL;
		*offset = 0;
		return;
	}

	*page = pfn_to_page(addr >> PAGE_SHIFT);
	*offset = addr & ~PAGE_MASK;
}

static void zram_insert_obj(struct zram *zram, u32 index, struct page *page,
			u32 offset)
{
	phys_addr_t addr;

	addr = page_to_pfn(page) << PAGE_SHIFT;
	addr |= (offset & ~PAGE_MASK);

	zram->table[index].addr = addr;
}

static u64 zram_default_disksize(void)
{
	u64 disksize;

	disksize = default_disksize_perc_ram *
			(totalram_pages / 100);
	disksize = (disksize << PAGE_SHIFT) & PAGE_MASK;

	return disksize;
}

static void zram_free_page(struct zram *zram, size_t index)
{
	int zlen;
	void *obj;
	u32 offset;
	struct page *page;

	/*
	 * No memory is allocated for zero filled pages.
	 * Simply clear corresponding table entry.
	 */
	if (zram_is_zero_page(zram, index)) {
		zram_clear_zero_page(zram, index);
		zram_dec_stat(zram, ZRAM_STAT_PAGES_ZERO);
		return;
	}

	zram_find_obj(zram, index, &page, &offset);
	if (!page)
		return;

	/* Uncompressed pages cosume whole page, so offset is zero */
	if (unlikely(!offset)) {
		zlen = PAGE_SIZE;
		__free_page(page);
		zram_dec_stat(zram, ZRAM_STAT_PAGES_EXPAND);
		goto out;
	}

	obj = kmap_atomic(page, KM_USER0) + offset;
	zlen = xv_get_object_size(obj);
	kunmap_atomic(obj, KM_USER0);

	xv_free(zram->mem_pool, page, offset);

out:
	zram_add_stat(zram, ZRAM_STAT_COMPR_SIZE, -zlen);
	zram_dec_stat(zram, ZRAM_STAT_PAGES_STORED);

	zram->table[index].addr = 0;
}

static void handle_zero_page(struct page *page)
{
	void *user_mem;

	user_mem = kmap_atomic(page, KM_USER0);
	memset(user_mem, 0, PAGE_SIZE);
	kunmap_atomic(user_mem, KM_USER0);

	flush_dcache_page(page);
}

static void handle_uncompressed_page(struct zram *zram,
				struct page *bio_page, u32 index)
{
	u32 zoffset;
	struct page *zpage;
	unsigned char *bio_mem, *zmem;

	zram_find_obj(zram, index, &zpage, &zoffset);
	BUG_ON(zoffset);

	bio_mem = kmap_atomic(bio_page, KM_USER0);
	zmem = kmap_atomic(zpage, KM_USER1);

	memcpy(bio_mem, zmem, PAGE_SIZE);
	kunmap_atomic(bio_mem, KM_USER0);
	kunmap_atomic(zmem, KM_USER1);

	flush_dcache_page(bio_page);
}

static int zram_read(struct zram *zram, struct bio *bio)
{
	int i;
	u32 index;
	struct bio_vec *bvec;

	if (unlikely(!zram->init_done)) {
		set_bit(BIO_UPTODATE, &bio->bi_flags);
		bio_endio(bio, 0);
		return 0;
	}

	zram_inc_stat(zram, ZRAM_STAT_NUM_READS);
	index = bio->bi_sector >> SECTORS_PER_PAGE_SHIFT;

	bio_for_each_segment(bvec, bio, i) {
		int ret;
		size_t zlen;
		u32 zoffset;
		struct page *bio_page, *zpage;
		unsigned char *bio_mem, *zmem;

		bio_page = bvec->bv_page;

		if (zram_is_zero_page(zram, index)) {
			handle_zero_page(bio_page);
			continue;
		}

		zram_find_obj(zram, index, &zpage, &zoffset);

		/* Requested page is not present in compressed area */
		if (unlikely(!zpage)) {
			pr_debug("Read before write on swap device: "
				"sector=%lu, size=%u",
				(ulong)(bio->bi_sector), bio->bi_size);
			/* Do nothing */
			continue;
		}

		/* Page is stored uncompressed since it's incompressible */
		if (unlikely(!zoffset)) {
			handle_uncompressed_page(zram, bio_page, index);
			continue;
		}

		bio_mem = kmap_atomic(bio_page, KM_USER0);
		zlen = PAGE_SIZE;

		zmem = kmap_atomic(zpage, KM_USER1) + zoffset;

		ret = lzo1x_decompress_safe(zmem, xv_get_object_size(zmem),
					bio_mem, &zlen);

		kunmap_atomic(bio_mem, KM_USER0);
		kunmap_atomic(zmem, KM_USER1);

		/* This should NEVER happen - return bio error if it does! */
		if (unlikely(ret != LZO_E_OK)) {
			pr_err("Decompression failed! err=%d, page=%u\n",
				ret, index);
			goto out;
		}

		flush_dcache_page(bio_page);
		index++;
	}

	set_bit(BIO_UPTODATE, &bio->bi_flags);
	bio_endio(bio, 0);
	return 0;

out:
	bio_io_error(bio);
	return 0;
}

static int zram_write(struct zram *zram, struct bio *bio)
{
	int i, ret;
	u32 index;
	struct bio_vec *bvec;

	if (unlikely(!zram->init_done)) {
		ret = zram_init_device(zram);
		if (ret)
			goto out;
	}

	zram_inc_stat(zram, ZRAM_STAT_NUM_WRITES);
	index = bio->bi_sector >> SECTORS_PER_PAGE_SHIFT;

	bio_for_each_segment(bvec, bio, i) {
		size_t zlen;
		u32 zoffset;
		struct page *bio_page, *zpage;
		unsigned char *zbuffer, *zworkmem;
		unsigned char *bio_mem, *zmem, *src;

		bio_page = bvec->bv_page;

		/*
		 * System overwrites unused sectors. Free memory associated
		 * with this sector now (if used).
		 */
		zram_free_page(zram, index);

		preempt_disable();
		zbuffer = __get_cpu_var(compress_buffer);
		zworkmem = __get_cpu_var(compress_workmem);
		if (unlikely(!zbuffer || !zworkmem)) {
			preempt_enable();
			goto out;
		}

		src = zbuffer;
		bio_mem = kmap_atomic(bio_page, KM_USER0);
		if (page_zero_filled(bio_mem)) {
			kunmap_atomic(bio_mem, KM_USER0);
			preempt_enable();
			zram_inc_stat(zram, ZRAM_STAT_PAGES_ZERO);
			zram_set_zero_page(zram, index);
			continue;
		}

		ret = lzo1x_1_compress(bio_mem, PAGE_SIZE, src, &zlen,
					zworkmem);

		kunmap_atomic(bio_mem, KM_USER0);

		if (unlikely(ret != LZO_E_OK)) {
			preempt_enable();
			pr_err("Compression failed! err=%d\n", ret);
			goto out;
		}

		 /* Page is incompressible. Store it as-is (uncompressed) */
		if (unlikely(zlen > max_zpage_size)) {
			zlen = PAGE_SIZE;
			zpage = alloc_page(GFP_NOWAIT | __GFP_HIGHMEM);
			if (unlikely(!zpage)) {
				preempt_enable();
				pr_info("Error allocating memory for "
					"incompressible page: %u\n", index);
				goto out;
			}

			zoffset = 0;
			zram_inc_stat(zram, ZRAM_STAT_PAGES_EXPAND);
			src = kmap_atomic(zpage, KM_USER0);
			goto memstore;
		}

		if (xv_malloc(zram->mem_pool, zlen, &zpage, &zoffset,
				GFP_NOWAIT | __GFP_HIGHMEM)) {
			preempt_enable();
			pr_info("Error allocating memory for compressed "
				"page: %u, size=%zu\n", index, zlen);
			goto out;
		}

memstore:
		zmem = kmap_atomic(zpage, KM_USER1) + zoffset;

		memcpy(zmem, src, zlen);
		kunmap_atomic(zmem, KM_USER1);
		preempt_enable();

		if (unlikely(!zoffset))
			kunmap_atomic(src, KM_USER0);

		/* Update stats */
		zram_add_stat(zram, ZRAM_STAT_COMPR_SIZE, zlen);
		zram_inc_stat(zram, ZRAM_STAT_PAGES_STORED);

		zram_insert_obj(zram, index, zpage, zoffset);
		index++;
	}

	set_bit(BIO_UPTODATE, &bio->bi_flags);
	bio_endio(bio, 0);
	return 0;

out:
	bio_io_error(bio);
	return 0;
}

static void zram_discard(struct zram *zram, struct bio *bio)
{
	size_t bytes = bio->bi_size;
	sector_t sector = bio->bi_sector;

	if (unlikely(!zram->init_done))
		goto out;

	zram_inc_stat(zram, ZRAM_STAT_DISCARD);

	while (bytes >= PAGE_SIZE) {
		zram_free_page(zram, sector >> SECTORS_PER_PAGE_SHIFT);
		sector += PAGE_SIZE >> SECTOR_SHIFT;
		bytes -= PAGE_SIZE;
	}

out:
	set_bit(BIO_UPTODATE, &bio->bi_flags);
	bio_endio(bio, 0);
}

/*
 * Check if request is within bounds and page aligned.
 */
static inline int valid_io_request(struct zram *zram, struct bio *bio)
{
	int ret = 1;

	if (unlikely(
		(bio->bi_sector >= (zram->disksize >> SECTOR_SHIFT)) ||
		(bio->bi_sector & (SECTORS_PER_PAGE - 1)) ||
		(bio->bi_size & (PAGE_SIZE - 1)))) {

		ret = 0;	/* invalid I/O */
		if (zram->init_done)
			zram_inc_stat(zram, ZRAM_STAT_INVALID_IO);
	}

	/* I/O request is valid */
	return ret;
}

/*
 * Handler function for all zram I/O requests.
 */
static int zram_make_request(struct request_queue *queue, struct bio *bio)
{
	int ret = 0;
	struct zram *zram = queue->queuedata;

	if (unlikely(!valid_io_request(zram, bio))) {
		bio_io_error(bio);
		return 0;
	}

	if (unlikely(bio_rw_flagged(bio, BIO_RW_DISCARD))) {
		zram_discard(zram, bio);
		return 0;
	}

	switch (bio_data_dir(bio)) {
	case READ:
		ret = zram_read(zram, bio);
		break;

	case WRITE:
		ret = zram_write(zram, bio);
		break;
	}

	return ret;
}

void zram_reset_device(struct zram *zram)
{
	size_t index;

	mutex_lock(&zram->init_lock);
	zram->init_done = 0;

	/* Free all pages that are still in this zram device */
	for (index = 0; index < zram->disksize >> PAGE_SHIFT; index++)
		zram_free_page(zram, index);

	vfree(zram->table);
	zram->table = NULL;

	xv_destroy_pool(zram->mem_pool);
	zram->mem_pool = NULL;

	/* Reset stats */
	memset(&zram->stats, 0, sizeof(zram->stats));

	zram->disksize = zram_default_disksize();
	mutex_unlock(&zram->init_lock);
}

int zram_init_device(struct zram *zram)
{
	int ret;
	size_t num_pages;

	mutex_lock(&zram->init_lock);

	if (zram->init_done) {
		mutex_unlock(&zram->init_lock);
		return 0;
	}

	if (!zram->disksize)
		zram->disksize = zram_default_disksize();

	num_pages = zram->disksize >> PAGE_SHIFT;
	zram->table = vmalloc(num_pages * sizeof(*zram->table));
	if (!zram->table) {
		pr_err("Error allocating zram address table\n");
		/* To prevent accessing table entries during cleanup */
		zram->disksize = 0;
		ret = -ENOMEM;
		goto fail;
	}
	memset(zram->table, 0, num_pages * sizeof(*zram->table));

	set_capacity(zram->disk, zram->disksize >> SECTOR_SHIFT);

	/* zram devices sort of resembles non-rotational disks */
	queue_flag_set_unlocked(QUEUE_FLAG_NONROT, zram->disk->queue);

	zram->stats = alloc_percpu(struct zram_stats_cpu);
	if (!zram->stats) {
		pr_err("Error allocating percpu stats\n");
		ret = -ENOMEM;
		goto fail;
	}

	zram->mem_pool = xv_create_pool();
	if (!zram->mem_pool) {
		pr_err("Error creating memory pool\n");
		ret = -ENOMEM;
		goto fail;
	}

	zram->init_done = 1;
	mutex_unlock(&zram->init_lock);

	pr_debug("Initialization done!\n");
	return 0;

fail:
	mutex_unlock(&zram->init_lock);
	zram_reset_device(zram);

	pr_err("Initialization failed: err=%d\n", ret);
	return ret;
}

#if defined(CONFIG_SWAP_FREE_NOTIFY)
static void zram_slot_free_notify(struct block_device *bdev,
				unsigned long index)
{
	struct zram *zram;

	zram = bdev->bd_disk->private_data;
	zram_free_page(zram, index);
	zram_inc_stat(zram, ZRAM_STAT_NOTIFY_FREE);
}
#endif

static const struct block_device_operations zram_devops = {
#if defined(CONFIG_SWAP_FREE_NOTIFY)
	.swap_slot_free_notify = zram_slot_free_notify,
#endif
	.owner = THIS_MODULE
};

static int create_device(struct zram *zram, int device_id)
{
	int ret = 0;

	mutex_init(&zram->init_lock);

	zram->queue = blk_alloc_queue(GFP_KERNEL);
	if (!zram->queue) {
		pr_err("Error allocating disk queue for device %d\n",
			device_id);
		ret = -ENOMEM;
		goto out;
	}

	blk_queue_make_request(zram->queue, zram_make_request);
	zram->queue->queuedata = zram;

	 /* gendisk structure */
	zram->disk = alloc_disk(1);
	if (!zram->disk) {
		blk_cleanup_queue(zram->queue);
		pr_warning("Error allocating disk structure for device %d\n",
			device_id);
		ret = -ENOMEM;
		goto out;
	}

	zram->disk->major = zram_major;
	zram->disk->first_minor = device_id;
	zram->disk->fops = &zram_devops;
	zram->disk->queue = zram->queue;
	zram->disk->private_data = zram;
	snprintf(zram->disk->disk_name, 16, "zram%d", device_id);

	/* Custom size can be set using syfs (/sys/block/zram<id>/disksize) */
	zram->disksize = zram_default_disksize();
	set_capacity(zram->disk, zram->disksize >> SECTOR_SHIFT);

	/*
	 * To ensure that we always get PAGE_SIZE aligned
	 * and n*PAGE_SIZED sized I/O requests.
	 */
	blk_queue_physical_block_size(zram->disk->queue, PAGE_SIZE);
	blk_queue_logical_block_size(zram->disk->queue, PAGE_SIZE);
	blk_queue_io_min(zram->disk->queue, PAGE_SIZE);
	blk_queue_io_opt(zram->disk->queue, PAGE_SIZE);

	zram->disk->queue->limits.discard_granularity = PAGE_SIZE;
	zram->disk->queue->limits.max_discard_sectors = UINT_MAX;
	zram->disk->queue->limits.discard_zeroes_data = 1;
	queue_flag_set_unlocked(QUEUE_FLAG_DISCARD, zram->queue);

	add_disk(zram->disk);

#ifdef CONFIG_SYSFS
	ret = sysfs_create_group(&disk_to_dev(zram->disk)->kobj,
				&zram_disk_attr_group);
	if (ret < 0) {
		pr_warning("Error creating sysfs group");
		goto out;
	}
#endif

	zram->init_done = 0;

out:
	return ret;
}

static void destroy_device(struct zram *zram)
{
#ifdef CONFIG_SYSFS
	sysfs_remove_group(&disk_to_dev(zram->disk)->kobj,
			&zram_disk_attr_group);
#endif

	if (zram->disk) {
		del_gendisk(zram->disk);
		put_disk(zram->disk);
	}

	if (zram->queue)
		blk_cleanup_queue(zram->queue);
}

/*
 * Callback for CPU hotplug events. Allocates percpu compression buffers.
 */
static int zram_cpu_notify(struct notifier_block *nb, unsigned long action,
			void *pcpu)
{
	int cpu = (long)pcpu;

	switch (action) {
	case CPU_UP_PREPARE:
		per_cpu(compress_buffer, cpu) = (void *)__get_free_pages(
					GFP_KERNEL | __GFP_ZERO, 1);
		per_cpu(compress_workmem, cpu) = kzalloc(
					LZO1X_MEM_COMPRESS, GFP_KERNEL);

		break;
	case CPU_DEAD:
	case CPU_UP_CANCELED:
		free_pages((unsigned long)(per_cpu(compress_buffer, cpu)), 1);
		per_cpu(compress_buffer, cpu) = NULL;

		kfree(per_cpu(compress_buffer, cpu));
		per_cpu(compress_buffer, cpu) = NULL;

		break;
	default:
		break;
	}

	return NOTIFY_OK;
}

static struct notifier_block zram_cpu_nb = {
	.notifier_call = zram_cpu_notify
};

static int __init zram_init(void)
{
	int ret, dev_id;
	unsigned int cpu;

	if (num_devices > max_num_devices) {
		pr_warning("Invalid value for num_devices: %u\n",
				num_devices);
		ret = -EINVAL;
		goto out;
	}

	ret = register_cpu_notifier(&zram_cpu_nb);
	if (ret)
		goto out;

	for_each_online_cpu(cpu) {
		void *pcpu = (void *)(long)cpu;
		zram_cpu_notify(&zram_cpu_nb, CPU_UP_PREPARE, pcpu);
		if (!per_cpu(compress_buffer, cpu) ||
					!per_cpu(compress_workmem, cpu)) {
			ret = -ENOMEM;
			goto out;
		}
	}

	zram_major = register_blkdev(0, "zram");
	if (zram_major <= 0) {
		pr_warning("Unable to get major number\n");
		ret = -EBUSY;
		goto out;
	}

	if (!num_devices) {
		pr_info("num_devices not specified. Using default: 1\n");
		num_devices = 1;
	}

	/* Allocate the device array and initialize each one */
	pr_debug("Creating %u devices ...\n", num_devices);
	devices = kzalloc(num_devices * sizeof(struct zram), GFP_KERNEL);
	if (!devices) {
		ret = -ENOMEM;
		goto unregister;
	}

	for (dev_id = 0; dev_id < num_devices; dev_id++) {
		ret = create_device(&devices[dev_id], dev_id);
		if (ret)
			goto free_devices;
	}

	return 0;

free_devices:
	while (dev_id)
		destroy_device(&devices[--dev_id]);
unregister:
	unregister_blkdev(zram_major, "zram");
out:
	for_each_online_cpu(cpu) {
		void *pcpu = (void *)(long)cpu;
		zram_cpu_notify(&zram_cpu_nb, CPU_UP_CANCELED, pcpu);
	}

	return ret;
}

static void __exit zram_exit(void)
{
	int i;
	unsigned int cpu;
	struct zram *zram;

	for (i = 0; i < num_devices; i++) {
		zram = &devices[i];

		destroy_device(zram);
		if (zram->init_done)
			zram_reset_device(zram);
	}

	unregister_blkdev(zram_major, "zram");

	for_each_online_cpu(cpu) {
		void *pcpu = (void *)(long)cpu;
		zram_cpu_notify(&zram_cpu_nb, CPU_UP_CANCELED, pcpu);
	}

	unregister_cpu_notifier(&zram_cpu_nb);

	kfree(devices);
	pr_debug("Cleanup done!\n");
}

module_param(num_devices, uint, 0);
MODULE_PARM_DESC(num_devices, "Number of zram devices");

module_init(zram_init);
module_exit(zram_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Nitin Gupta <ngupta@vflare.org>");
MODULE_DESCRIPTION("Compressed RAM Block Device");
