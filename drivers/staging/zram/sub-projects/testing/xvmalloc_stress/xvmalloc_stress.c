#include <asm/errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../alloc_stress/simprofile/libsimparse.h"
#include "../../allocators/xvmalloc/xvmalloc.h"

PoolID pool;
int statsEnabled;
u32 countAlloc = 0, countFree = 0;

typedef union {
	u64 addr64;
	struct {
		u32 pageNum;
		u32 offset;
	};
} Location;

static void showStats(PoolID pool)
{
	if (statsEnabled) {
		printf("%llu\n", xvGetTotalSize(pool) >> 10);
	}
}

u64 simMalloc(u32 size)
{
	int error;
	Location loc;

	countAlloc++;

	error = xvMalloc(pool, size, &loc.pageNum, &loc.offset);
	if (error) {
		return 0;
	}

	showStats(pool);

	return loc.addr64;
}

void simFree(u64 location)
{
	Location loc;

	countFree++;
	loc.addr64 = location;

	xvFree(pool, loc.pageNum, loc.offset);

	showStats(pool);
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
		printf("Usage: xvmalloc_stress <file> [-stats]\n"
			"\t <file> is file from randgen\n");
		return -EINVAL;
	}
	
	fileName = argv[1];

	if ((argc > 2) && !strncmp(argv[2], "-stats", 6)) {
		statsEnabled = 1;
	}

	pool = xvCreateMemPool();
	if (pool == INVALID_POOL_ID) {
		printf("Failed to create xv mempool\n");
		return -ENOMEM;
	}

	handle = simGetHandle();
	setSimFile(handle, fileName);
	setSimCallbacks(handle, simMalloc, simFree, simEnd);
	simStart(handle);
	simPutHandle(handle);

	printf("# end: countAlloc=%u, countFree=%u\n", countAlloc, countFree);

	return 0;
}
