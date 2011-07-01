/* Copyright (c) 2010, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/vcm_alloc.h>
#include <linux/string.h>
#include <asm/sizes.h>

/* Amount of memory managed by VCM */
#define TOTAL_MEM_SIZE SZ_32M

static unsigned int base_pa = 0x80000000;
int basicalloc_init;

int chunk_sizes[NUM_CHUNK_SIZES] = {SZ_1M, SZ_64K, SZ_4K};
int init_num_chunks[] = {
	(TOTAL_MEM_SIZE/2) / SZ_1M,
	(TOTAL_MEM_SIZE/4) / SZ_64K,
	(TOTAL_MEM_SIZE/4) / SZ_4K
};
#define LAST_SZ() (ARRAY_SIZE(chunk_sizes) - 1)

#define vcm_alloc_err(a, ...)						\
	pr_err("ERROR %s %i " a, __func__, __LINE__, ##__VA_ARGS__)

struct phys_chunk_head {
	struct list_head head;
	int num;
};

struct phys_mem {
	struct phys_chunk_head heads[ARRAY_SIZE(chunk_sizes)];
} phys_mem;

static int is_allocated(struct list_head *allocated)
{
	/* This should not happen under normal conditions */
	if (!allocated) {
		vcm_alloc_err("no allocated\n");
		return 0;
	}

	if (!basicalloc_init) {
		vcm_alloc_err("no basicalloc_init\n");
		return 0;
	}
	return !list_empty(allocated);
}

static int count_allocated_size(enum chunk_size_idx idx)
{
	int cnt = 0;
	struct phys_chunk *chunk, *tmp;

	if (!basicalloc_init) {
		vcm_alloc_err("no basicalloc_init\n");
		return 0;
	}

	list_for_each_entry_safe(chunk, tmp,
				 &phys_mem.heads[idx].head, list) {
		if (is_allocated(&chunk->allocated))
			cnt++;
	}

	return cnt;
}


int vcm_alloc_get_mem_size(void)
{
	return TOTAL_MEM_SIZE;
}
EXPORT_SYMBOL(vcm_alloc_get_mem_size);


int vcm_alloc_blocks_avail(enum chunk_size_idx idx)
{
	if (!basicalloc_init) {
		vcm_alloc_err("no basicalloc_init\n");
		return 0;
	}

	return phys_mem.heads[idx].num;
}
EXPORT_SYMBOL(vcm_alloc_blocks_avail);


int vcm_alloc_get_num_chunks(void)
{
	return ARRAY_SIZE(chunk_sizes);
}
EXPORT_SYMBOL(vcm_alloc_get_num_chunks);


int vcm_alloc_all_blocks_avail(void)
{
	int i;
	int cnt = 0;

	if (!basicalloc_init) {
		vcm_alloc_err("no basicalloc_init\n");
		return 0;
	}

	for (i = 0; i < ARRAY_SIZE(chunk_sizes); ++i)
		cnt += vcm_alloc_blocks_avail(i);
	return cnt;
}
EXPORT_SYMBOL(vcm_alloc_all_blocks_avail);


int vcm_alloc_count_allocated(void)
{
	int i;
	int cnt = 0;

	if (!basicalloc_init) {
		vcm_alloc_err("no basicalloc_init\n");
		return 0;
	}

	for (i = 0; i < ARRAY_SIZE(chunk_sizes); ++i)
		cnt += count_allocated_size(i);
	return cnt;
}
EXPORT_SYMBOL(vcm_alloc_count_allocated);


void vcm_alloc_print_list(int just_allocated)
{
	int i;
	struct phys_chunk *chunk, *tmp;

	if (!basicalloc_init) {
		vcm_alloc_err("no basicalloc_init\n");
		return;
	}

	for (i = 0; i < ARRAY_SIZE(chunk_sizes); ++i) {
		if (list_empty(&phys_mem.heads[i].head))
			continue;
		list_for_each_entry_safe(chunk, tmp,
					 &phys_mem.heads[i].head, list) {
			if (just_allocated && !is_allocated(&chunk->allocated))
				continue;

			printk(KERN_INFO "pa = %#x, size = %#x\n",
			       chunk->pa, chunk_sizes[chunk->size_idx]);
		}
	}
}
EXPORT_SYMBOL(vcm_alloc_print_list);


int vcm_alloc_idx_to_size(int idx)
{
	return chunk_sizes[idx];
}
EXPORT_SYMBOL(vcm_alloc_idx_to_size);


int vcm_alloc_destroy(void)
{
	int i;
	struct phys_chunk *chunk, *tmp;

	if (!basicalloc_init) {
		vcm_alloc_err("no basicalloc_init\n");
		return -1;
	}

	/* can't destroy a space that has allocations */
	if (vcm_alloc_count_allocated()) {
		vcm_alloc_err("allocations still present\n");
		return -1;
	}
	for (i = 0; i < ARRAY_SIZE(chunk_sizes); ++i) {

		if (list_empty(&phys_mem.heads[i].head))
			continue;
		list_for_each_entry_safe(chunk, tmp,
					 &phys_mem.heads[i].head, list) {
			list_del(&chunk->list);
			memset(chunk, 0, sizeof(*chunk));
			kfree(chunk);
		}
	}

	basicalloc_init = 0;

	return 0;
}
EXPORT_SYMBOL(vcm_alloc_destroy);


int vcm_alloc_init(unsigned int set_base_pa)
{
	int i = 0, j = 0;
	struct phys_chunk *chunk;
	int pa;

	if (set_base_pa)
		base_pa = set_base_pa;

	pa = base_pa;

	/* no double inits */
	if (basicalloc_init) {
		vcm_alloc_err("double basicalloc_init\n");
		BUG();
		return -1;
	}

	/* separate out to ensure good cleanup */
	for (i = 0; i < ARRAY_SIZE(chunk_sizes); ++i) {
		INIT_LIST_HEAD(&phys_mem.heads[i].head);
		phys_mem.heads[i].num = 0;
	}

	for (i = 0; i < ARRAY_SIZE(chunk_sizes); ++i) {
		for (j = 0; j < init_num_chunks[i]; ++j) {
			chunk = kzalloc(sizeof(*chunk), GFP_KERNEL);
			if (!chunk) {
				vcm_alloc_err("null chunk\n");
				goto fail;
			}
			chunk->pa = pa; pa += chunk_sizes[i];
			chunk->size_idx = i;
			INIT_LIST_HEAD(&chunk->allocated);
			list_add_tail(&chunk->list, &phys_mem.heads[i].head);
			phys_mem.heads[i].num++;
		}
	}

	basicalloc_init = 1;
	return 0;
fail:
	vcm_alloc_destroy();
	return -1;
}
EXPORT_SYMBOL(vcm_alloc_init);


int vcm_alloc_free_blocks(struct phys_chunk *alloc_head)
{
	struct phys_chunk *chunk, *tmp;

	if (!basicalloc_init) {
		vcm_alloc_err("no basicalloc_init\n");
		goto fail;
	}

	if (!alloc_head) {
		vcm_alloc_err("no alloc_head\n");
		goto fail;
	}

	list_for_each_entry_safe(chunk, tmp, &alloc_head->allocated,
				 allocated) {
		list_del_init(&chunk->allocated);
		phys_mem.heads[chunk->size_idx].num++;
	}

	return 0;
fail:
	return -1;
}
EXPORT_SYMBOL(vcm_alloc_free_blocks);


int vcm_alloc_num_blocks(int num,
			 enum chunk_size_idx idx, /* chunk size */
			 struct phys_chunk *alloc_head)
{
	struct phys_chunk *chunk;
	int num_allocated = 0;

	if (!basicalloc_init) {
		vcm_alloc_err("no basicalloc_init\n");
		goto fail;
	}

	if (!alloc_head) {
		vcm_alloc_err("no alloc_head\n");
		goto fail;
	}

	if (list_empty(&phys_mem.heads[idx].head)) {
		vcm_alloc_err("list is empty\n");
		goto fail;
	}

	if (vcm_alloc_blocks_avail(idx) < num) {
		vcm_alloc_err("not enough blocks? num=%d\n", num);
		goto fail;
	}

	list_for_each_entry(chunk, &phys_mem.heads[idx].head, list) {
		if (num_allocated == num)
			break;
		if (is_allocated(&chunk->allocated))
			continue;

		list_add_tail(&chunk->allocated, &alloc_head->allocated);
		phys_mem.heads[idx].num--;
		num_allocated++;
	}
	return num_allocated;
fail:
	return 0;
}
EXPORT_SYMBOL(vcm_alloc_num_blocks);


int vcm_alloc_max_munch(int len,
			struct phys_chunk *alloc_head)
{
	int i;

	int blocks_req = 0;
	int block_residual = 0;
	int blocks_allocated = 0;

	int ba = 0;

	if (!basicalloc_init) {
		vcm_alloc_err("basicalloc_init is 0\n");
		goto fail;
	}

	if (!alloc_head) {
		vcm_alloc_err("alloc_head is NULL\n");
		goto fail;
	}

	for (i = 0; i < ARRAY_SIZE(chunk_sizes); ++i) {
		blocks_req = len / chunk_sizes[i];
		block_residual = len % chunk_sizes[i];

		len = block_residual; /* len left */
		if (blocks_req) {
			int blocks_available = 0;
			int blocks_diff = 0;
			int bytes_diff = 0;

			blocks_available = vcm_alloc_blocks_avail(i);
			if (blocks_available < blocks_req) {
				blocks_diff =
					(blocks_req - blocks_available);
				bytes_diff =
					blocks_diff * chunk_sizes[i];

				/* add back in the rest */
				len += bytes_diff;
			} else {
				/* got all the blocks I need */
				blocks_available =
					(blocks_available > blocks_req)
					? blocks_req : blocks_available;
			}

			ba = vcm_alloc_num_blocks(blocks_available, i,
						  alloc_head);

			if (ba != blocks_available) {
				vcm_alloc_err("blocks allocated (%i) !="
					      " blocks_available (%i):"
					      " chunk size = %#x,"
					      " alloc_head = %p\n",
					      ba, blocks_available,
					      i, (void *) alloc_head);
				goto fail;
			}
			blocks_allocated += blocks_available;
		}
	}

	if (len) {
		int blocks_available = 0;

		blocks_available = vcm_alloc_blocks_avail(LAST_SZ());

		if (blocks_available > 1) {
			ba = vcm_alloc_num_blocks(1, LAST_SZ(), alloc_head);
			if (ba != 1) {
				vcm_alloc_err("blocks allocated (%i) !="
					      " blocks_available (%i):"
					      " chunk size = %#x,"
					      " alloc_head = %p\n",
					      ba, 1,
					      LAST_SZ(),
					      (void *) alloc_head);
				goto fail;
			}
			blocks_allocated += 1;
		} else {
			vcm_alloc_err("blocks_available (%#x) <= 1\n",
				      blocks_available);
			goto fail;
		}
	}

	return blocks_allocated;
fail:
	vcm_alloc_free_blocks(alloc_head);
	return 0;
}
EXPORT_SYMBOL(vcm_alloc_max_munch);
