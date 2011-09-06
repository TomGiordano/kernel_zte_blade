#include <stdio.h>
#include <stdlib.h>

#include "../../allocators/tlsf-edit/tlsf.h"
#include "../../swap_replay/sr_parse/libsr_parse.h"

#define K(x)		(x << 10)
#define M(x)		(x << 20)

#define PAGE_SIZE	K(4)
#define MAX_SWAP_SIZE	M(64)
#define MAX_TABLE_SIZE	(MAX_SWAP_SIZE / PAGE_SIZE)

#define INIT_POOL_SIZE	K(32)
#define GROW_POOL_SIZE	K(32)
#define MAX_POOL_SIZE	M(64)

static void *pool;
static struct {
	void *addr;
	unsigned short len;
} *page_table;

static struct {
	size_t num_free;
	size_t num_alloc;
	size_t num_sysfree;
	size_t num_sysalloc;
} stats;

void *get_mem(size_t size)
{
	stats.num_sysalloc++;
	return malloc(size);
}

void put_mem(void *ptr)
{
	stats.num_sysfree++;
	free(ptr);
}

void event_read(unsigned long index)
{
}

void event_write(unsigned long index, unsigned long len)
{
	if (page_table[index].addr) {
		stats.num_free++;
		tlsf_free(page_table[index].addr, pool);
	}

	stats.num_alloc++;
	page_table[index].addr = tlsf_malloc(len, pool);
	if (page_table[index].addr == NULL) {
		printf("Error allocating for index=%lu, len=%lu\n", index, len);
		return;
	}
	page_table[index].len = len;
	// printf("%u\n", tlsf_get_used_size(pool) >> 10);
	//printf("%u\n", tlsf_get_total_size(pool) >> 10);
}

void dump_stats(void)
{
	printf("# num_free:	%u\n", stats.num_free);
	printf("# num_alloc:	%u\n", stats.num_alloc);
	printf("# num_sysalloc:	%u\n", stats.num_sysalloc);
	printf("# num_sysfree:	%u\n", stats.num_sysfree);
}

/*
 * Arg:
 *	0: Success
 *	>0: Error: partial read
 *	<0: Error: file read error
 */
void event_replay_end(int err)
{
	printf("# replay end: err=%d\n", err);
	dump_stats();
}

int main(int argc, char **argv)
{
	int ret;
	char *fname;

	if (argc < 2) {
		printf("Usage: tlsf_test <file>\n");
		printf("\t<file> is swap replay dump file from sr_collect\n");
		goto out;
	}

	fname = argv[1];	
	ret = set_sr_file(fname);
	if (ret < 0) {
		printf("Error setting replay data file.\n");
		goto out;
	}

	pool = tlsf_create_memory_pool(get_mem, put_mem,
			INIT_POOL_SIZE, MAX_POOL_SIZE, GROW_POOL_SIZE);
	if (pool == NULL) {
		printf("Error creating memory pool.\n");
		goto out;
	}

	page_table = calloc(MAX_TABLE_SIZE, sizeof(*page_table));
	if (page_table == NULL) {
		printf("Error allocating page table structure.\n");
		goto out_pgtbl;
	}

	set_sr_callbacks(event_read, event_write, event_replay_end);
	
	sr_start();

	free(page_table);

out_pgtbl:
	tlsf_destroy_memory_pool(pool);

out:
	return 0;
}

