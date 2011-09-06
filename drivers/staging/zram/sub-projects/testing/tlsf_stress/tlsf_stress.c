#include <asm/errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../alloc_stress/simprofile/libsimparse.h"
#include "../../allocators/tlsf-edit/tlsf.h"

#define PAGE_SIZE 4096
#define PAGE_MASK (PAGE_SIZE - 1)

void *pool;
int statsEnabled;
u32 countAlloc = 0, countFree = 0;

typedef union {
	u64 addr64;
	struct {
		u32 pageNum;
		u32 offset;
	};
} Location;

static void showStats(void *pool)
{
	if (statsEnabled) {
		printf("%zu\n", tlsf_get_total_size(pool) >> 10);
	}
}

u64 simMalloc(u32 size)
{
	void *ptr;

	countAlloc++;

	ptr = tlsf_malloc(size, pool);
	if (!ptr) {
		return 0;
	}

	showStats(pool);

	return (u64)((size_t)(ptr));
}

void simFree(u64 location)
{
	void *ptr = (void *)((size_t)location);

	countFree++;

	tlsf_free(ptr, pool);

	showStats(pool);
}

void simEnd(void)
{
	tlsf_destroy_memory_pool(pool);
	return;
}

void* (tlsfGetMem)(size_t bytes)
{
	int error;
	void *ptr;
	size_t size;

	size = (bytes + PAGE_MASK) & ~PAGE_MASK;
	error = posix_memalign(&ptr, PAGE_SIZE, size);
	if (error) {
		return NULL;
	}

	return ptr;
}

void (tlsfPutMem)(void *ptr)
{
	free(ptr);	
}

int main(int argc, char **argv)
{
	char *fileName;
	void *handle;

	if (argc < 2 || argc > 3) {
		printf("Usage: tlsf_stress <file> [-stats]\n"
			"\t <file> is file from randgen\n");
		return -EINVAL;
	}
	
	fileName = argv[1];

	if ((argc > 2) && !strncmp(argv[2], "-stats", 6)) {
		statsEnabled = 1;
	}

	pool = tlsf_create_memory_pool(tlsfGetMem, tlsfPutMem,
					PAGE_SIZE, 0, PAGE_SIZE);
	if (!pool) {
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
