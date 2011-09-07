/*
 * xvmalloc.c
 *
 * Copyright (C) 2008, 2009  Nitin Gupta
 *
 * This code is released using a dual license strategy: GPL/LGPL
 * You can choose the licence that better fits your requirements.
 *
 * Released under the terms of the GNU General Public License Version 2.0
 * Released under the terms of the GNU Lesser General Public License Version 2.1
 */

#include <asm-generic/errno-base.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "xvmalloc_int.h"
#include "xvmalloc.h"

#define unlikely(_exp) __builtin_expect((_exp), 0)

/**
 * (taken from Linux kernel)
 *
 * fls - find last set bit in word
 * @x: the word to search
 *
 * This is defined in a similar way as the libc and compiler builtin
 * ffs, but returns the position of the most significant set bit.
 *
 * fls(value) returns 0 if value is 0 or the position of the last
 * set bit if value is nonzero. The last (most significant) bit is
 * at position 32.
 */
static inline int fls(int x)
{
        int r;
        asm("bsrl %1,%0\n\t"
            "jnz 1f\n\t"
            "movl $-1,%0\n"
            "1:" : "=r" (r) : "rm" (x));
        return r + 1;
}

#ifdef XV_STATS
static void statInc(u64 *value)
{
	*value = *value + 1;
}

static void statDec(u64 *value)
{
	*value = *value - 1;
}
#else
#define statInc(x) do { } while(0)
#define statDec(x) do { } while(0)
#endif

static void SetBit(u32 *value, u32 bitIdx)
{
	*value |= (u32)(1 << bitIdx);
}

static void ClearBit(u32 *value, u32 bitIdx)
{
	*value &= (u32)(~(1 << bitIdx));
}

static u32 IsBlockFree(BlockHeader *block)
{
	return (block->prev & BLOCK_FREE);
}

static u32 IsPrevBlockFree(BlockHeader *block)
{
	return (block->prev & PREV_FREE);
}

static void SetBlockFree(BlockHeader *block)
{
	block->prev |= BLOCK_FREE;
}

static void SetBlockPrevFree(BlockHeader *block)
{
	block->prev |= PREV_FREE;
}

static void SetBlockUsed(BlockHeader *block)
{
	block->prev &= ~BLOCK_FREE;
}

static void SetBlockPrevUsed(BlockHeader *block)
{
	block->prev &= ~PREV_FREE;
}

static u16 GetBlockPrev(BlockHeader *block)
{
	return (block->prev & PREV_MASK);
}

static void SetBlockPrev(BlockHeader *block, u16 newOffset)
{
	block->prev = newOffset | (block->prev & FLAGS_MASK);
}

// Get index of free list containing blocks of
// maximum size which is <= given size
static u32 GetIndexForInsert(u32 size)
{
	size = size > MAX_ALLOC_SIZE ? MAX_ALLOC_SIZE : size;
	size &= ~FL_DELTA_MASK;
	return ((size - MIN_ALLOC_SIZE) >> FL_DELTA_SHIFT);
}

// Get index of free list having blocks of size >= requested size
static u32 GetIndex(u32 size)
{
	size = (size + FL_DELTA_MASK) & ~FL_DELTA_MASK; 
	return ((size - MIN_ALLOC_SIZE) >> FL_DELTA_SHIFT);
}

static u32 FindBlock(Pool *pool, u32 size, u32 *pageNum, u32 *offset)
{
	u32 flBitmap, slBitmap;
	u32 flIndex, slIndex, slBitStart;

	if (!pool->flBitmap) {
		return 0;
	}

	if (unlikely(size < MIN_ALLOC_SIZE)) {
		size = MIN_ALLOC_SIZE;
	}

	slIndex = GetIndex(size);
	slBitmap = pool->slBitmap[slIndex >> BITMAP_SHIFT];
	slBitStart = slIndex & BITMAP_MASK;
	
	if (slBitmap & (1 << slBitStart)) {
		*pageNum = pool->FreeList[slIndex].pageNum;
		*offset = pool->FreeList[slIndex].offset;
		return slIndex;
	}

	slBitStart++;
	slBitmap >>= slBitStart;
	
	if ((slBitStart != BITMAP_BITS) && slBitmap) {
		slIndex += ffs(slBitmap);
		*pageNum = pool->FreeList[slIndex].pageNum;
		*offset = pool->FreeList[slIndex].offset;
		return slIndex;
	}
	
	flIndex = slIndex >> BITMAP_SHIFT;
	
	flBitmap = (pool->flBitmap) >> (flIndex + 1);	
	if (!flBitmap) {
		return 0;
	}
	flIndex += ffs(flBitmap);
	slBitmap = pool->slBitmap[flIndex];
	slIndex = (flIndex << BITMAP_SHIFT) + ffs(slBitmap) - 1;
	*pageNum = pool->FreeList[slIndex].pageNum;
	*offset = pool->FreeList[slIndex].offset;

	return slIndex;
}

static u32 AllocPage(void)
{
	int error;
	void *page;
	
	error = posix_memalign(&page, PAGE_SIZE, PAGE_SIZE);
	if (unlikely(error)) {
		return INVALID_PGNUM;
	}
	return (u32)((size_t)(page) >> PAGE_SHIFT);
}

static void FreePage(u32 pageNum)
{
	void *page;
	
	page = (void *)((size_t)(pageNum) << PAGE_SHIFT);
	free(page);
}

static void InsertBlock(Pool *pool, u32 pageNum, u32 offset,
			BlockHeader *block)
{
	u32 flIndex, slIndex;
	BlockHeader *nextBlock;
	
	slIndex = GetIndexForInsert(block->size);
	flIndex = slIndex >> BITMAP_SHIFT;
	
	block->link.prevPageNum = INVALID_PGNUM;
	block->link.prevOffset = 0;
	block->link.nextPageNum = pool->FreeList[slIndex].pageNum;
	block->link.nextOffset = pool->FreeList[slIndex].offset;
	pool->FreeList[slIndex].pageNum = pageNum;
	pool->FreeList[slIndex].offset = offset;
	
	if (block->link.nextPageNum != INVALID_PGNUM) {
		nextBlock = (BlockHeader *)
				((char *)xvMapPage(block->link.nextPageNum) +
				block->link.nextOffset);
		nextBlock->link.prevPageNum = pageNum;
		nextBlock->link.prevOffset = offset;
	}
	
	SetBit(&pool->slBitmap[flIndex], slIndex & BITMAP_MASK);
	SetBit(&pool->flBitmap, flIndex);
}

static void RemoveBlockHead(Pool *pool, BlockHeader *block,
			    u32 slIndex)
{
	BlockHeader *tmpBlock;
	u32 flIndex = slIndex >> BITMAP_SHIFT;
	
	pool->FreeList[slIndex].pageNum = block->link.nextPageNum;
	pool->FreeList[slIndex].offset = block->link.nextOffset;
	block->link.prevPageNum = INVALID_PGNUM;
	block->link.prevOffset = 0;
	
	if (pool->FreeList[slIndex].pageNum == INVALID_PGNUM) {
		ClearBit(&pool->slBitmap[flIndex], slIndex & BITMAP_MASK);
		if (!pool->slBitmap[flIndex]) {
			ClearBit(&pool->flBitmap, flIndex);
		}
	} else {
		// Debug
		tmpBlock = (BlockHeader *)
			((char *)xvMapPage(pool->FreeList[slIndex].pageNum) +
				pool->FreeList[slIndex].offset);
		tmpBlock->link.prevPageNum = INVALID_PGNUM;
		tmpBlock->link.prevOffset = 0;
	}
	
}

static void RemoveBlock(Pool *pool, u32 pageNum, u32 offset,
			BlockHeader *block, u32 slIndex)
{
	u32 flIndex;
	BlockHeader *tmpBlock;
	
	if (pool->FreeList[slIndex].pageNum == pageNum
	   && pool->FreeList[slIndex].offset == offset) {
		RemoveBlockHead(pool, block, slIndex);
		return;
	}
	
	flIndex = slIndex >> BITMAP_SHIFT;
	
	if (block->link.prevPageNum != INVALID_PGNUM) {
		tmpBlock = (BlockHeader *)
				((char *)xvMapPage(block->link.prevPageNum) +
					block->link.prevOffset);
		tmpBlock->link.nextPageNum = block->link.nextPageNum;
		tmpBlock->link.nextOffset = block->link.nextOffset;
	}
	
	if (block->link.nextPageNum != INVALID_PGNUM) {
		tmpBlock = (BlockHeader *)
				((char *)xvMapPage(block->link.nextPageNum) +
					block->link.nextOffset);
		tmpBlock->link.prevPageNum = block->link.prevPageNum;
		tmpBlock->link.prevOffset = block->link.prevOffset;
	}
	
	return;
}

static int GrowPool(Pool *pool)
{
	u32 pageNum;
	BlockHeader *block;

	pageNum = AllocPage();
	if (unlikely(pageNum == INVALID_PGNUM)) {
		return -ENOMEM;
	}
	statInc(&pool->currPages);
	
	block = xvMapPage(pageNum);
	
	block->size = PAGE_SIZE - ALIGN;
	SetBlockFree(block);
	SetBlockPrevUsed(block);
	SetBlockPrev(block, 0);
	InsertBlock(pool, pageNum, 0, block);
	
	return 0;
}

#if 0
static void shouldCompact(Pool *pool)
{
	u64 actual, total, wastage;

#define WASTAGE_THRESHOLD_PERC	10
	actual = xvGetDataSizeBytes(pool);
	total = xvGetTotalSizeBytes(pool);

	wastagePerc = ((total - actual) * 100) / actual;
	if (wastage > WASTAGE_THRESHOLD_PERC) {
		return 1;
	}

	return 0;
}
#endif

PoolID xvCreateMemPool(xvUpdateBackRefFn *updateBackRefFn)
{
	int i, error;
	void *ovhdMem;
	Pool *pool;
	u32 poolOvhd;

	poolOvhd = ROUNDUP(sizeof(Pool), PAGE_SIZE);
	error = posix_memalign(&ovhdMem, PAGE_SIZE, poolOvhd);
	if (error) {
		return INVALID_POOL_ID;
	}
	pool = ovhdMem;
	memset(pool, 0, poolOvhd);

	for (i = 0; i < NUM_FREE_LISTS; i++) {
		pool->FreeList[i].pageNum = INVALID_PGNUM;
	}

	pool->updateBackRefFn = updateBackRefFn;

	return (PoolID)pool;
}

void xvDestroyMemPool(PoolID poolID)
{
	Pool *pool = (Pool *)poolID;
	free(pool);
}

int xvMalloc(PoolID poolID, u32 size, u32 *pageNum, u32 *offset)
{
	int error;
	u32 index, tmpSize, origSize, tmpOffset;
	BlockHeader *block, *tmpBlock = NULL;
	Pool *pool = (Pool *)poolID;
	
	*pageNum = INVALID_PGNUM;
	*offset = 0;
	origSize = size;

	if (unlikely(!size || size > MAX_ALLOC_SIZE)) {
		return -ENOMEM;
	}

	if (unlikely(size < MIN_ALLOC_SIZE)) {
		size = MIN_ALLOC_SIZE;
	} else {
		size = ROUNDUP_ALIGN(size);
	}
	
	index = FindBlock(pool, size, pageNum, offset);
	
	if (*pageNum == INVALID_PGNUM) {
		error = GrowPool(pool);
		if (unlikely(error)) {
			return -ENOMEM;
		}
		index = FindBlock(pool, size, pageNum, offset);
	}

	if (*pageNum == INVALID_PGNUM) {
		return -ENOMEM;
	}

	block = (BlockHeader *)((char *)xvMapPage(*pageNum) + *offset);

	RemoveBlockHead(pool, block, index);

	// Split the block if required
	tmpOffset = *offset + size + ALIGN;
	tmpSize = block->size - size;
	tmpBlock = (BlockHeader *)((char *)block + size + ALIGN);
	if (tmpSize) {
		tmpBlock->size = tmpSize - ALIGN;
		SetBlockFree(tmpBlock);
		SetBlockPrevUsed(tmpBlock);
		
		SetBlockPrev(tmpBlock, *offset);
		if (tmpBlock->size >= MIN_ALLOC_SIZE) {
			InsertBlock(pool, *pageNum, tmpOffset, tmpBlock);
		}

		if (tmpOffset + ALIGN + tmpBlock->size < PAGE_SIZE) {
			tmpBlock = (BlockHeader *)((char *)tmpBlock
						+ tmpBlock->size + ALIGN);

			SetBlockPrev(tmpBlock, tmpOffset);
		}
	} else {
		// this block is exact fit
		if (tmpOffset < PAGE_SIZE) {
			SetBlockPrevUsed(tmpBlock);
		}
	}
	
	block->size = origSize;
	SetBlockUsed(block);

	*offset += ALIGN;
	
	return 0;
}

int xvMallocCompact(PoolID pool, u32 size, u32 backRef,
			u32 *pageNum, u32 *offset)
{
	int ret;
	u32 *backRefPtr;
	
	size += sizeof(backRef);
	ret = xvMalloc(pool, size, pageNum, offset);
	if (ret) {
		return ret;
	}

	backRefPtr = (u32 *)((char *)xvMapPage(*pageNum) + *offset);
	*backRefPtr = backRef;
	//*offset += sizeof(backRef);

	return 0;	
}

void xvFree(PoolID poolID, u32 pageNum, u32 offset)
{
	void *page;
	BlockHeader *block, *tmpBlock;
	Pool *pool = (Pool *)poolID;

	page = xvMapPage(pageNum);
	offset -= ALIGN;
	block = (BlockHeader *)((char *)page + offset);
	
	if (unlikely(block->size < MIN_ALLOC_SIZE)) {
		block->size = MIN_ALLOC_SIZE;
	} else {
		block->size = ROUNDUP_ALIGN(block->size);
	}
	
	tmpBlock = (BlockHeader *)((char *)block + block->size + ALIGN);
	if (offset + block->size + ALIGN == PAGE_SIZE) {
		tmpBlock = NULL;
	}

	if (tmpBlock && IsBlockFree(tmpBlock)) {
		// we do not keep blocks smaller than this in any free list
		if (tmpBlock->size >= MIN_ALLOC_SIZE) {
			RemoveBlock(pool, pageNum,
				    offset + block->size + ALIGN, tmpBlock,
				    GetIndexForInsert(tmpBlock->size));
		}
		block->size += tmpBlock->size + ALIGN;
	}

	if (IsPrevBlockFree(block)) {
		tmpBlock = (BlockHeader *)((char *)(page) +
				GetBlockPrev(block));
		offset = offset - tmpBlock->size - ALIGN;
		
		if (tmpBlock->size >= MIN_ALLOC_SIZE) {
			RemoveBlock(pool, pageNum, offset, tmpBlock,
				    GetIndexForInsert(tmpBlock->size));
		}
		tmpBlock->size += block->size + ALIGN;
		block = tmpBlock;
	}

	if (block->size == PAGE_SIZE - ALIGN) {
		FreePage(pageNum);
		statDec(&pool->currPages);
		return;
	}

	SetBlockFree(block);
	InsertBlock(pool, pageNum, offset, block);

	if (offset + block->size < PAGE_SIZE - ALIGN) {
		tmpBlock = (BlockHeader *)((char *)(block) + block->size +
								ALIGN);
		SetBlockPrevFree(tmpBlock);
		SetBlockPrev(tmpBlock, offset);
	}

	return;
}

void xvCompactMemPool(PoolID poolID)
{
	int fli, sli;
	u32 slBitmap, countLive = 0, blkSize, pageOffset;
	Pool *pool = (Pool *)poolID;
	BlockHeader *pageStart, *block;

	fli = fls(pool->flBitmap);
	if (unlikely(!fli)) {
		return;
	}

	fli--;
	sli = fli << BITMAP_SHIFT; 
	printf("fli=%d\t", fli);
	slBitmap = pool->slBitmap[fli];
	sli += fls(slBitmap) - 1;
	
	printf("sli=%d\n", sli);

	// Start iterating from first block in this page
	pageStart = xvMapPage(pool->FreeList[sli].pageNum);
	pageOffset = 0;

	block = pageStart;
	printf("\n--------------\n");

	do {
		if (unlikely(block->size < MIN_ALLOC_SIZE)) {
			blkSize = MIN_ALLOC_SIZE;
		} else {
			blkSize = ROUNDUP_ALIGN(block->size);
		}

		printf("block->size=%u, blkSize=%u\n", block->size, blkSize);

		if (!IsBlockFree(block)) {
			countLive++;
			printf("countLive=%u\n", countLive);
		}
		printf("\n====================\n");

		pageOffset += blkSize + ALIGN;
		printf("offset=%u\n", pageOffset);
		block = (BlockHeader *)((char *)pageStart + pageOffset);
		if (pageOffset > PAGE_SIZE) {
			printf("######################################################################\n");
			//exit(0);
		}
	} while (pageOffset < PAGE_SIZE);
	printf("-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n");

	return;
}

void *xvMapPage(u32 pageNum)
{
        return (void *)((size_t)(pageNum << PAGE_SHIFT));
}

void xvUnmapPage(u32 pageNum)
{
        return;
}

u32 xvGetObjectSize(void *obj)
{
	BlockHeader *blk;

	blk = (BlockHeader *)((char *)(obj) - ALIGN);
	return blk->size;
}

// Returns total memory used by allocator (bytes)
u64 xvGetTotalSize(PoolID poolID)
{
	Pool *pool = (Pool *)poolID;

#ifdef XV_STATS
	return (pool->currPages << PAGE_SHIFT);
#else
	return 0;
#endif
}
