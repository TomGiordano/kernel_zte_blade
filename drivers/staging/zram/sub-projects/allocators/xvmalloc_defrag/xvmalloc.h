/*
 * xvmalloc.h
 *
 * Copyright (C) 2008, 2009  Nitin Gupta
 *
 * This code is released using a dual license strategy: GPL/LGPL
 * You can choose the licence that better fits your requirements.
 *
 * Released under the terms of the GNU General Public License Version 2.0
 * Released under the terms of the GNU Lesser General Public License Version 2.1
 */

#ifndef _XVMALLOC_H_
#define _XVMALLOC_H_

#include <stddef.h>
#include "../../include/types.h"

//#include "xvmalloc_int.h"

#define PAGE_SHIFT	12
#define PAGE_SIZE	(1 << PAGE_SHIFT)
#define PAGE_MASK	(PAGE_SIZE - 1)

#define INVALID_POOL_ID	NULL
#define INVALID_PGNUM	((u32)(-1))

typedef struct Pool * PoolID;
typedef void (xvUpdateBackRefFn)(u32);

PoolID xvCreateMemPool(xvUpdateBackRefFn *updateBackRefFn);
void xvDestroyMemPool(PoolID poolID);

int xvMalloc(PoolID poolID, u32 size, u32 *pageNum, u32 *offset);
int xvMallocCompact(PoolID pool, u32 size, u32 backRef,
			u32 *pageNum, u32 *offset);
void xvFree(PoolID poolID, u32 pageNum, u32 offset);
void xvCompactMemPool(PoolID poolID);

void *xvMapPage(u32 pageNum);
void xvUnmapPage(u32 pageNum);

u32 xvGetObjectSize(void *obj);
u64 xvGetTotalSize(PoolID poolID);

#endif

