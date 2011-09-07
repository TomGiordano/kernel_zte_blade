#include <asm/errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../alloc_stress/simprofile/libsimparse.h"
#include "../../allocators/xvmalloc_defrag/xvmalloc.h"

#define WASTAGE_PERC_THRESHOLD	5

PoolID pool;
int statsEnabled;
u32 countAlloc = 0, countFree = 0;
u64 dataSize;

void *locTable;

typedef union {
	u64 addr64;
	struct {
		u32 pageNum;
		u32 offset;
	};
} Location;

static void showStats(PoolID pool)
{
	if (!statsEnabled) {
		return;
	}
	
	printf("%llu\n", xvGetTotalSize(pool) >> 10);
}

void CompactPool(PoolID pool)
{
	u32 wastagePerc;

	if (!dataSize) {
		return;
	}
	
	wastagePerc = (xvGetTotalSize(pool) - dataSize) * 100 / dataSize;

	if (wastagePerc >= WASTAGE_PERC_THRESHOLD) {
		printf("w=%u\t", wastagePerc);
		xvCompactMemPool(pool);
	}
}

u64 simMallocWithBackRef(u32 size, u32 backRef)
{
	int error;
	Location loc;

	countAlloc++;

	error = xvMallocCompact(pool, size, backRef, &loc.pageNum, &loc.offset);
	if (error) {
		return 0;
	}
	dataSize += size + sizeof(backRef);

	CompactPool(pool);
	showStats(pool);

	return loc.addr64;
}

void simFree(u64 location)
{
	void *obj;
	Location loc;

	countFree++;
	loc.addr64 = location;

	obj = (char *)xvMapPage(loc.pageNum) + loc.offset;
	dataSize -= xvGetObjectSize(obj);

	xvFree(pool, loc.pageNum, loc.offset);

	CompactPool(pool);
	showStats(pool);
}

void UpdateBackRefCallback(u32 backRef)
{
	return;
}

void simEnd(void)
{
	xvDestroyMemPool(pool);
	return;
}

int main(int argc, char **argv)
{
	char *fileName;
	void *handle;
	

	if (argc < 2 || argc > 3) {
		printf("Usage: xvmalloc_defrag_stress <file> [-stats]\n"
			"\t <file> is file from randgen\n");
		return -EINVAL;
	}
	
	fileName = argv[1];

	if ((argc > 2) && !strncmp(argv[2], "-stats", 6)) {
		statsEnabled = 1;
	}

	pool = xvCreateMemPool(UpdateBackRefCallback);
	if (pool == INVALID_POOL_ID) {
		printf("Failed to create xv mempool\n");
		return -ENOMEM;
	}

	handle = simGetHandle();
	setSimFile(handle, fileName);
	setSimCallbacks(handle, 0, simFree, simEnd);
	setSimCallbackMallocWithBackRef(handle, simMallocWithBackRef);
	locTable = simGetTable();
	simStart(handle);
	simPutHandle(handle);

	printf("# end: countAlloc=%u, countFree=%u\n", countAlloc, countFree);

	return 0;
}
