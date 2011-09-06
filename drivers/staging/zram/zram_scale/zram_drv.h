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

#ifndef _ZRAM_DRV_H_
#define _ZRAM_DRV_H_

#include <linux/mutex.h>

#include "u64_stats_sync.h"
#include "sub-projects/allocators/xvmalloc-kmod/xvmalloc.h"

/*
 * Some arbitrary value. This is just to catch
 * invalid value for num_devices module parameter.
 */
static const unsigned max_num_devices = 32;

/*-- Configurable parameters */

/* Default zram disk size: 25% of total RAM */
static const unsigned default_disksize_perc_ram = 25;

/*
 * Pages that compress to size greater than this are stored
 * uncompressed in memory.
 */
static const unsigned max_zpage_size = PAGE_SIZE / 8 * 7;

/*
 * NOTE: max_zpage_size must be less than or equal to XV_MAX_ALLOC_SIZE
 * otherwise, xv_malloc() would always return failure.
 */

/*-- End of configurable params */

#define SECTOR_SHIFT		9
#define SECTOR_SIZE		(1 << SECTOR_SHIFT)
#define SECTORS_PER_PAGE_SHIFT	(PAGE_SHIFT - SECTOR_SHIFT)
#define SECTORS_PER_PAGE	(1 << SECTORS_PER_PAGE_SHIFT)

/*
 * Maintains swap slot to compressed object mapping.
 */
struct table {
	phys_addr_t addr;	/* location of [compressed] object */
};

enum zram_stats_index {
	ZRAM_STAT_COMPR_SIZE,	/* compressed size of pages stored */
	ZRAM_STAT_NUM_READS,	/* failed + successful */
	ZRAM_STAT_NUM_WRITES,	/* --do-- */
	ZRAM_STAT_INVALID_IO,	/* non-page-aligned I/O requests */
	ZRAM_STAT_NOTIFY_FREE,	/* no. of swap slot free notifications */
	ZRAM_STAT_DISCARD,	/* no. of block discard requests */
	ZRAM_STAT_PAGES_ZERO,	/* no. of zero filled pages */
	ZRAM_STAT_PAGES_STORED,	/* no. of pages currently stored */
	ZRAM_STAT_PAGES_EXPAND,	/* no. of incompressible pages */
	ZRAM_STAT_NSTATS,
};

struct zram_stats_cpu {
	s64 count[ZRAM_STAT_NSTATS];
	struct u64_stats_sync syncp;
};

struct zram {
	struct xv_pool *mem_pool;
	struct table *table;
	struct request_queue *queue;
	struct gendisk *disk;
	unsigned int init_done;
	/* Prevent concurrent execution of device init and reset */
	struct mutex init_lock;
	/*
	 * This is the limit on amount of *uncompressed* worth of data
	 * we can store in a disk (in bytes).
	 */
	u64 disksize;
	struct zram_stats_cpu *stats;	/* percpu stats */
};

extern struct zram *devices;
extern unsigned int num_devices;
#ifdef CONFIG_SYSFS
extern struct attribute_group zram_disk_attr_group;
#endif

extern int zram_init_device(struct zram *zram);
extern void zram_reset_device(struct zram *zram);

#endif
