#include <stdio.h>
#include <stdlib.h>

#include "../../allocators/tlsf-copy/tlsf.h"
#include "../../swap_replay/sr_parse/libsr_parse.h"

#define PAGE_SIZE	4096
#define MAX_SWAP_SIZE	(64 * 1024 * 1024)
#define MAX_TABLE_SIZE	(MAX_SWAP_SIZE / PAGE_SIZE)
#define MAX_POOL_SIZE	(64 * 1024 * 1024)
#define KB(BYTES)	(BYTES >> 10)

static void *pool;
static struct {
	void *addr;
	unsigned short len;
} *page_table;

void event_read(unsigned long index)
{
}

void event_write(unsigned long index, unsigned long len)
{
	if (page_table[index].addr)
		free_ex(page_table[index].addr, pool);

	page_table[index].addr = malloc_ex(len, pool);
	if (page_table[index].addr == NULL) {
		printf("Error allocating for index=%lu, len=%lu\n", index, len);
		return;
	}
	page_table[index].len = len;
	printf("%u\n", KB(get_used_size(pool)));
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
}

int main(int argc, char **argv)
{
	int ret;
	size_t pool_size;
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

	pool = malloc(MAX_POOL_SIZE);
	if (pool == NULL) {
		printf("Error allocating memory pool.\n");
		goto out;
	}

	pool_size = init_memory_pool(MAX_POOL_SIZE, pool);
	printf("# total pool size: %u\n", pool_size);

	page_table = calloc(MAX_TABLE_SIZE, sizeof(*page_table));
	if (page_table == NULL) {
		printf("Error allocating page table structure.\n");
		goto out_pgtbl;
	}

	set_sr_callbacks(event_read, event_write, event_replay_end);
	
	sr_start();

	free(page_table);

out_pgtbl:
	destroy_memory_pool(pool);
	free(pool);

out:
	return 0;
}

