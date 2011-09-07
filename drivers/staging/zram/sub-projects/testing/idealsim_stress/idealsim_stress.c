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
	countAlloc++;

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

	if (argc != 2) {
		printf("Usage: idealsim_alloc_test <file>\n"
			"\t <file> is file from randgen\n");
		return -EINVAL;
	}
	
	fileName = argv[1];

	handle = simGetHandle();
	setSimFile(handle, fileName);
	setSimCallbacks(handle, simMalloc, simFree, simEnd);
	simStart(handle);
	simPutHandle(handle);

	printf("# end: countAlloc=%u, countFree=%u\n", countAlloc, countFree);

	return 0;
}
