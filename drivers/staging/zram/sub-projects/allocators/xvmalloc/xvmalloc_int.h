/*
 * xvmalloc_int.c
 *
 * Copyright (C) 2008, 2009  Nitin Gupta
 *
 * This code is released using a dual license strategy: GPL/LGPL
 * You can choose the licence that better fits your requirements.
 *
 * Released under the terms of the GNU General Public License Version 2.0
 * Released under the terms of the GNU Lesser General Public License Version 2.1
 */

#ifndef _XVMALLOC_INT_H_
#define _XVMALLOC_INT_H_

#include "../../include/types.h"

#define ROUNDUP(x,y)	((x) + (y) / (y) * (y))
// Each individual bitmap is 32-bit
#define BITMAP_BITS	32
#define BITMAP_SHIFT	5
#define BITMAP_MASK	(BITMAP_BITS - 1)

// User configurable params

// Must be power of two
#define ALIGN_SHIFT	2
#define ALIGN		(1 << ALIGN_SHIFT)
#define ALIGN_MASK	(ALIGN - 1)

// This must be greater than sizeof(LinkFree)
#define MIN_ALLOC_SIZE	32
#define MAX_ALLOC_SIZE	3072
// Free lists are separated by FL_DELTA bytes
#define FL_DELTA_SHIFT	3
#define FL_DELTA	(1 << FL_DELTA_SHIFT)
#define FL_DELTA_MASK	(FL_DELTA - 1)
#define NUM_FREE_LISTS	((MAX_ALLOC_SIZE - MIN_ALLOC_SIZE) \
				/ FL_DELTA + 1)

#define MAX_FLI		(ROUNDUP(NUM_FREE_LISTS,32) / 32)

/* End of user params */

#define ROUNDUP_ALIGN(x)	(((x) + ALIGN_MASK) & ~ALIGN_MASK)

#define BLOCK_FREE	0x1
#define PREV_FREE	0x2
#define FLAGS_MASK	ALIGN_MASK
#define PREV_MASK	~FLAGS_MASK

typedef struct {
	u32 pageNum;
	u16 offset;
	u16 pad;
} FreeListEntry;

typedef struct {
	u32 prevPageNum;
	u32 nextPageNum;
	u16 prevOffset;
	u16 nextOffset;
} LinkFree;

typedef struct {
	union {
		// This common header must be ALIGN bytes
		u8 common[ALIGN];
		struct {
			u16 size;
			u16 prev;
		};
	};
	LinkFree link;
} BlockHeader;

typedef struct {
	u32 flBitmap;
	u32 slBitmap[MAX_FLI];	

	FreeListEntry FreeList[NUM_FREE_LISTS];
	
	// stats
#ifdef XV_STATS
	u64 currPages;
#endif
} Pool;

#endif
