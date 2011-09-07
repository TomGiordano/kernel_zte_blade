#include <asm/errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../alloc_stress/simprofile/libsimparse.h"

int statsEnabled;
u32 countAlloc = 0, countFree = 0;
u64 totalSize = 0;

u64 simMalloc(u32 size)
{
	u32 sizeShift, blockSize;

	countAlloc++;

#define PAGE_SHIFT	12
#define MAX_SIZE	2048
	// blockSize: [PAGE_SIZE/128, PAGE_SIZE/2] => [32b, 2048b]
	for (sizeShift = 7; sizeShift >= 1; sizeShift--)  {
		blockSize = 1 << (PAGE_SHIFT - sizeShift);
		if (size <= blockSize) {
			size = blockSize;
			break;
		}
	}
	totalSize += size;

        printf("%llu\n", totalSize >> 10);

        // Store size in location field
	return size;
}

void simFree(u64 location)
{
        countFree++;

        // For this "ideal" allocator, we store
        // obj size in location field
        totalSize -= location;
        printf("%llu\n", totalSize >> 10);
}

void simEnd(void)
{
	return;
}

int main(int argc, char **argv)
{
	char *fileName;
	void *handle;

	if (argc < 2 || argc > 3) {
		printf("Usage: ideal_qh_stress <file> [-stats]\n"
			"\t <file> is file from randgen\n");
		return -EINVAL;
	}
	
	fileName = argv[1];

	if ((argc > 2) && !strncmp(argv[2], "-stats", 6)) {
		statsEnabled = 1;
	}

	handle = simGetHandle();
	setSimFile(handle, fileName);
	setSimCallbacks(handle, simMalloc, simFree, simEnd);
	simStart(handle);
	simPutHandle(handle);

	printf("# end: countAlloc=%u, countFree=%u\n", countAlloc, countFree);

	return 0;
}

