#include <stdio.h>
#include <stdlib.h>

#include "libsimparse.h"

u32 countAlloc = 0, countFree = 0;

u64 simMalloc(u32 size)
{
	countAlloc++;
	void *ptr = malloc(size);
	return (u64)(size_t)ptr;
}

void simFree(u64 location)
{
	countFree++;
	free((void *)(size_t)location);
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
		printf("Usage: sim_alloc_test <file>\n"
			"\t <file> is file from randgen\n");
		return 0;
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
