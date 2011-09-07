#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "../../lib/hashtable.h"
#include "../../swap_replay/sr_parse/libsr_parse.h"

#define PAGE_SIZE	4096
#define MAX_SWAP_SIZE	(512 * 1024 * 1024)
#define MAX_TABLE_SIZE	(MAX_SWAP_SIZE / PAGE_SIZE)
#define KB(BYTES)	(BYTES >> 10)
#define MAX_FILES	10

struct stats {
	ulong nr_swap_pages;	/* no. of pages currently in swap */
	ulong nr_swap_pages_ps;	/* total pages when page-sharing is used */
	ulong mem_compress;	/* memory used if swap pages are compressed */
	ulong mem_compress_and_ps;	/* page-sharing is used and these
					unique pages are compressed */
};

/* table[idx] -> swap sector 'idx' */
//static void **table;

/* Hash table of pages in swap */
static struct hash_table htable;

/* updated at every write event */
static struct stats stats;

static pthread_key_t thread_key_table;
static pthread_mutex_t event_write_lock;

static void init_hash_item(struct hash_item *item, u16 clen, u64 *key)
{
	INIT_HLIST_NODE(&item->head);
	item->key[0] = key[0];
	item->key[1] = key[1];
	item->compress_len = clen;
	item->count = 1;
}

/* Assumes hash lock is acquired */
static inline void get_item(struct hash_item *item)
{
	item->count++;	
}

/* Assumes hash lock is acquired */
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
	void **table;
	struct hash_item *item;

	table = pthread_getspecific(thread_key_table);
	if (table == NULL) {
		printf("Failed to get thread specific data\n");
		return;
	}

	pthread_mutex_lock(&htable.lock);
	for (i = 1; i < MAX_TABLE_SIZE; i++) {
		item = (struct hash_item *)table[i];
		if (item) {
			stats.nr_swap_pages--;
			stats.mem_compress -= item->compress_len;
			put_item(item);
		}
	}
	pthread_mutex_unlock(&htable.lock);
}

void event_read(unsigned long index)
{
#if 0
	void *table;
	table = pthread_getspecific(thread_key_table);
	printf("R %p\n", table);
#endif
}

/* serialized */
void event_write(unsigned long index, unsigned long len,
		const unsigned char *hash)
{
	int ret;
	void **table;

	pthread_mutex_lock(&event_write_lock);

	table = pthread_getspecific(thread_key_table);
	if (table == NULL) {
		printf("Failed to get thread specific data\n");
		return;
	}
	//printf("W %p\n", table);

	struct hash_item *item = (struct hash_item *)(table[index]);

	/* Remove page already at location 'index' (if any) */
	if (item) {
		stats.nr_swap_pages--;
		stats.mem_compress -= item->compress_len;
		pthread_mutex_lock(&htable.lock);
		put_item(item);
		pthread_mutex_unlock(&htable.lock);
	}

	/* Check if page with same hash already exists */
	pthread_mutex_lock(&htable.lock);
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
	pthread_mutex_unlock(&htable.lock);

	stats.mem_compress += item->compress_len;
	stats.nr_swap_pages++;
	table[index] = item;

	//printf("# (KB) <none> <page_share> <compress> <ps_and_compress>\n");
	printf("%lu %lu %lu %lu\n",
		stats.nr_swap_pages * 4,
		stats.nr_swap_pages_ps * 4,
		KB(stats.mem_compress),
		KB(stats.mem_compress_and_ps));

	pthread_mutex_unlock(&event_write_lock);
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

static void *threadfn(void *arg)
{
	int ret;
	char *fname;
	void *sr_handle, **table;

	fname = arg;

	table = calloc(MAX_TABLE_SIZE, sizeof(*table));
	if (table == NULL) {
		printf("Error allocating page table structure\n");
		goto out;
	}

	//printf("hello world!\n");
	pthread_setspecific(thread_key_table, table);

	sr_handle = sr_gethandle();

	ret = set_sr_file(sr_handle, fname);
	if (ret < 0) {
		printf("Error setting replay data file\n");
		goto out;
	}

	set_sr_callbacks(sr_handle, event_read, event_write, event_replay_end);

	sr_start(sr_handle);

	sr_puthandle(sr_handle);

	free_table_entries(table);
	free(table);
out:
	return NULL;
}

static void usage(void)
{
	printf("Usage: cps_test <num_files> <file 1> <file 2> .... <file n>\n");
	printf("\t<file> is swap replay dump file from sr_collect\n");
	exit(0);
}

int main(int argc, char **argv)
{
	int ret, i;
	unsigned long num_files;
	pthread_t *threads;

	if (argc < 2)
		usage();

	num_files = argc - 1;
	if (num_files > MAX_FILES) {
		printf("Too many files: %lu\n", num_files);
		return 0;
	}

	threads = calloc(num_files, sizeof(*threads));
	if (threads == NULL) {
		printf("Error allocating threads array\n");
		return 0;
	}

	pthread_key_create(&thread_key_table, NULL);

	ret = ht_create(&htable, 16);
	if (ret) {
		printf("Error allocating hash table\n");
		goto out;
	}


	for (i = 0; i < num_files; i++) {
		ret = pthread_create(&threads[i], NULL, threadfn, argv[1 + i]);
		if (ret) {
			printf("Error creating thread: %d\n", i);
			goto out_thread;
		}
	}

	for (i = 0; i < num_files; i++)
		pthread_join(threads[i], NULL);

out_thread:
	ht_destroy(&htable);

out:
	pthread_key_delete(thread_key_table);
	free(threads);
	return ret;
}
