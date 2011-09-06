/*
 * Minimal hashtable testing
 */

#include <stdio.h>

#include "../hashtable.h"

#define PASS(fmt,arg...) printf("PASS: " fmt "\n",##arg)
#define FAIL(fmt,arg...) printf("FAIL: " fmt "\n",##arg)

/* sample keys */
u64 K1[2] = { 0xaaaaaaaaaaaaaaaaULL, 0xaaaaaaaaaaaaaaaaULL };
u64 K2[2] = { 0xabcdabcdabcdabcdULL, 0xabcdabcdabcdabcdULL };
u64 K3[3] = { 0x1234567890abcdefULL, 0x1234567890abcdefULL };

struct hash_table HT;
struct hash_item ITEM1, ITEM2, ITEM3, ITEM4;

void init_item(struct hash_item *item, u64 *key)
{
	INIT_HLIST_NODE(&item->head);
	item->key[0] = key[0];
	item->key[1] = key[1];
	
	item->compress_len = 0;
	item->count = 0;
}

static inline unsigned int key_isequal_128(u64 *key1, u64 *key2)
{
	return ((key1[0] == key2[0]) && (key1[1] == key2[1]));
}

int main()
{
	int ret, failed, testno;
	struct hash_item *item;

	/* small hash table size - more stress! */
	if ((ret = ht_create(&HT, 16)))	/* hashtable with 2^4 entries */
		goto out_alloc_table;

	// test1
#if 0
	failed = 1;
	testno = 1;
	init_item(&ITEM1, K1);
	if ((ret = ht_insert_item(&HT, &ITEM1)))
		goto out;
	ret = ht_find_item(&HT, K1, &item);
	if (ret)
		FAIL("test1: item not found");
	else {
		if (key_isequal_128(item->key, K1)) {
			PASS("test1");
			failed = 0;
		} else {
			FAIL("test1: wrong item returned");
		}
	}
#endif
	// test2
	failed = 1;
	testno = 2;
	init_item(&ITEM1, K1);
	init_item(&ITEM2, K2);
	init_item(&ITEM3, K3);
	init_item(&ITEM4, K1);
	ret = ht_insert_item(&HT, &ITEM1);
	ret = ht_insert_item(&HT, &ITEM2);
	ret = ht_insert_item(&HT, &ITEM3);
	ret = ht_insert_item(&HT, &ITEM4);
	if (ret)
		goto out;
	ret = ht_find_item(&HT, K1, &item);
	printf("k1 item: %p\n", item);
	ret = ht_find_item(&HT, K2, &item);
	ret = ht_find_item(&HT, K3, &item);
	ret = ht_find_item(&HT, K1, &item);
	printf("k1 item: %p\n", item);
	ret = ht_remove_item(&HT, item);
	if (ret)
		goto out;
	printf("check\n");
	ret = ht_find_item(&HT, K1, &item);
	printf("ret=%d\n", ret);
	printf("k1 item: %p\n", item);
	ret = ht_remove_item(&HT, item);
	ret = ht_find_item(&HT, K1, &item);
	printf("ret=%d\n", ret);

	if (!ret)
		goto out;
	failed = 0;
	PASS("test2");

out:
	ht_destroy(&HT);

out_alloc_table:
	if (failed)
		printf("\t\n *** failed at test: %d\n", testno);
	else
		printf("\t\n *** ALL tests passed!\n");

	return ret;
}
