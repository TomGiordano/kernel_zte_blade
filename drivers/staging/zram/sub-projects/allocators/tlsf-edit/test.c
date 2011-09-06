#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tlsf.h"

#define M(x)	(x << 20)
#define K(x)	(x << 10)

#define INIT_SIZE	K(64)
#define MAX_SIZE	M(64)
#define GROW_SIZE	K(64)

static void* get_mem(size_t bytes)
{
	return calloc(bytes, 1);
}

static void put_mem(void * addr)
{
	free(addr);
}

int main(void)
{
	void *mem, *pool;
	pool = tlsf_create_memory_pool(get_mem, put_mem,
				INIT_SIZE, MAX_SIZE, GROW_SIZE);
	mem = tlsf_malloc(4033, pool);
	if (mem == NULL) {
		printf("test: error allocating memory\n");
		return 0;
	}
	memset(mem, 0x0, 4033);
	tlsf_free(mem, pool);
	tlsf_destroy_memory_pool(pool);
	return 0;
}
