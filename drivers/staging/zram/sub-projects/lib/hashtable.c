#include <ctype.h>
#include <error.h>
#include <stdio.h>
#include <stdlib.h>

#include "hashtable.h"
#include "internal.h"

/* ---
 * Helper functions
 */
static inline u64 hash_64(u64 val, unsigned int bits)
{
	u64 hash = val;

	/*  Sigh, gcc can't optimise this alone like it does for 32 bits. */
	u64 n = hash;
	n <<= 18;
	hash -= n;
	n <<= 33;
	hash -= n;
	n <<= 3;
	hash += n;
	n <<= 3;
	hash -= n;
	n <<= 4;
	hash += n;
	n <<= 2;
	hash += n;

	/* High bits are more random, so use them. */
	return hash >> (64 - bits);
}

static inline u64 hash_128(u64 *val, unsigned int bits)
{
	return ((hash_64(val[0], bits / 2) << (bits / 2))
		+ hash_64(val[1], bits / 2));
}

static inline unsigned int key_isequal_128(u64 *key1, u64 *key2)
{
	return ((key1[0] == key2[0]) && (key1[1] == key2[1]));
}

static struct hlist_node *ht_find_key(struct hash_table *ht,
					u64 *key)
{
	struct hash_item *entry;
	struct hlist_head *h_list;
	struct hlist_node *list;
	u64 hashed_key;

	hashed_key = hash_128(key, ht->order);
	h_list = &ht->table[hashed_key];
	hlist_for_each(list, h_list) {
		entry = hlist_entry(list, struct hash_item, head);
		if (key_isequal_128(entry->key, key))
			return list;
	}
	return NULL;
}

/* ---
 * Interface
 */

int ht_find_item(struct hash_table *ht, u64 *key,
		struct hash_item **item)
{
	struct hlist_node *list;

	list = ht_find_key(ht, key);
	if (!list)
		return -1;

	*item = hlist_entry(list, struct hash_item, head);
	return 0;
}

/*
 * Assumes 'item' is not already present. Make sure this is
 * true using ht_find_item() first. Otherwise there will
 * be items with duplicate keys.
 */
int ht_insert_item(struct hash_table *ht, struct hash_item *item)
{
	struct hlist_head *h_list;
	u64 hashed_key;

	hashed_key = hash_128(item->key, ht->order);
	//printf("ht_insert: key(idx)=%llu\n", hashed_key);
	h_list = &ht->table[hashed_key];
	hlist_add_head(&item->head, h_list);

	ht->count_items++;
	return 0;
}

int ht_remove_item(struct hash_table *ht, struct hash_item *item)
{
	hlist_del_init(&item->head);
	ht->count_items--;
	return 0;
}

int ht_create(struct hash_table *ht, unsigned int order)
{
	unsigned int i;

	ht->size = 1 << order;
	ht->order = order;
	ht->count_items = 0;
	ht->table = NULL;
	ht->table = malloc(ht->size * sizeof(*ht->table));
	if (!ht->table) {
		printf("Error allocating hash table\n");
		return -1;
	}

	for (i = 0; i < ht->size; i++) {
		INIT_HLIST_HEAD(&ht->table[i]);
	}

	pthread_mutex_init(&ht->lock, NULL);
	return 0;
}

void ht_destroy(struct hash_table *ht)
{
	if (ht == NULL)
		return;
	free(ht->table);
	ht->table = NULL;
}
