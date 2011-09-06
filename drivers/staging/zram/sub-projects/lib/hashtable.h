#ifndef _HASH_H_
#define _HASH_H_

#include <pthread.h>

#include "list.h"
#include "more_types.h"

/* ---
 * Data structures
 */

struct hash_item {
	struct hlist_node head;
	u64 key[2];

	/* additional node data */
	u16 compress_len;
	u16 count;
};

struct hash_table {
	pthread_mutex_t lock;
	u32 order;	/* hash size = 2^order */
	u32 size;
	u32 count_items;
	struct hlist_head *table;
};


/* ---
 * Interface
 */

int ht_create(struct hash_table *ht, unsigned int order);
void ht_destroy(struct hash_table *ht);

int ht_find_item(struct hash_table *ht, u64 *key,
		struct hash_item **item);
int ht_insert_item(struct hash_table *ht, struct hash_item *item);
int ht_remove_item(struct hash_table *ht, struct hash_item *item);

#endif
