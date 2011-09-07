#ifndef _ZRAM_COMPAT_H_
#define _ZRAM_COMPAT_H_

#include <linux/version.h>

/* Uncomment this if you are using swap free notify patch */
//#define CONFIG_SWAP_FREE_NOTIFY

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,35))
#ifndef CONFIG_SWAP_FREE_NOTIFY
#define CONFIG_SWAP_FREE_NOTIFY
#endif
#endif

#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,31))
#define blk_queue_physical_block_size(q, size) \
	blk_queue_hardsect_size(q, size)
#define blk_queue_logical_block_size(q, size)
#endif

#endif
