#include <stdio.h>
#include <stdlib.h>

#include "../../lib/hashtable.h"
#include "../../swap_replay/sr_parse/libsr_parse.h"

#define PAGE_SIZE	4096
#define MAX_SWAP_SIZE	(1500 * 1024 * 1024)
#define MAX_TABLE_SIZE	(MAX_SWAP_SIZE / PAGE_SIZE)
#define KB(BYTES)	(BYTES >> 10)

struct stats {
	ulong nr_swap_pages;	/* no. of pages currently in swap */
	ulong nr_swap_pages_ps;	/* total pages when page-sharing is used */
	ulong mem_compress;	/* memory used if swap pages are compressed */
	ulong mem_compress_and_ps;	/* page-sharing is used and these
					unique pages are compressed */
};

/* table[idx] -> swap sector 'idx' */
static void **table;

/* Hash table of pages in swap */
struct hash_table htable;

/* updated at every write event */
struct stats stats;

static void init_hash_item(struct hash_item *item, u16 clen, u64 *key)
{
	INIT_HLIST_NODE(&item->head);
	item->key[0] = key[0];
	item->key[1] = key[1];
	item->compress_len = clen;
	item->count = 1;
}

static inline void get_item(struct hash_item *item)
{
	item->count++;	
}

static inline void put_item(struct hash_item *item)
{
	int ret;

	item->count--;
	if (item->count == 0) {
		ret = ht_remove_item(&htable, item);
		if (ret) {
			printf("Error with hash_remove()\n");
			return;
		}
		stats.nr_swap_pages_ps--;
		stats.mem_compress_and_ps -= item->compress_len;
		free(item);
	}
}

static void free_table_entries()
{
	unsigned long i;
	struct hash_item *item;

	for (i = 1; i < MAX_TABLE_SIZE; i++) {
		item = (struct hash_item *)table[i];
		if (item)
			put_item(item); 
	}
}

void event_read(unsigned long index)
{
	// test
	//printf("R %lu\n", index);
}

void event_write(unsigned long index, unsigned long len, unsigned char *hash)
{
	int ret;
	struct hash_item *item = (struct hash_item *)(table[index]);

	// test
	//printf("W %lu %lu\n", index, len);

	/* Remove page already at location 'index' (if any) */
	if (item) {
		//printf("overwrite\n");
		stats.nr_swap_pages--;
		stats.mem_compress -= item->compress_len;
		put_item(item);
	}

	/* Check if page with same hash already exists */
	ret = ht_find_item(&htable, (u64 *)hash, &item);
	if (ret) {	/* not found - its unique */
		item = calloc(1, sizeof(*item));
		if (item == NULL) {
			printf("Error allocating hash item\n");
			return;
		}
		init_hash_item(item, len, (u64 *)hash);
		ret = ht_insert_item(&htable, item);
		stats.nr_swap_pages_ps++;
		stats.mem_compress_and_ps += item->compress_len;
	} else {
		get_item(item);
	}

	stats.mem_compress += item->compress_len;
	stats.nr_swap_pages++;
	table[index] = item;

	//printf("# (KB) <none> <page_share> <compress> <ps_and_compress>\n");
	printf("%lu %lu %lu %lu\n",
		stats.nr_swap_pages * 4,
		stats.nr_swap_pages_ps * 4,
		KB(stats.mem_compress),
		KB(stats.mem_compress_and_ps));
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
	printf("# summary:\n");
	printf("# \tnr_swap_pages: %lu\n", stats.nr_swap_pages);
	printf("# \tnr_swap_pages_ps: %lu\n", stats.nr_swap_pages_ps);
}

int main(int argc, char **argv)
{
	int ret;
	char *fname;

	if (argc < 2) {
		printf("Usage: cps_test <file>\n");
		printf("\t<file> is swap replay dump file from sr_collect\n");
		goto out;
	}

	fname = argv[1];	
	ret = set_sr_file(fname);
	if (ret < 0) {
		printf("Error setting replay data file\n");
		goto out;
	}

	table = calloc(MAX_TABLE_SIZE, sizeof(*table));
	if (table == NULL) {
		printf("Error allocating page table structure\n");
		goto out;
	}

	ret = ht_create(&htable, 16);
	if (ret) {
		printf("Error allocating hash table\n");
		goto out_ht;
	}

	set_sr_callbacks(event_read, event_write, event_replay_end);
	
	sr_start();

	free_table_entries();
	ht_destroy(&htable);

out_ht:
	free(table);

out:
	return ret;
}
