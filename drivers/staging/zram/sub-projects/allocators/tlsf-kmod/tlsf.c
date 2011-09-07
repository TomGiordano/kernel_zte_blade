/*
 * Two Levels Segregate Fit memory allocator (TLSF)
 * Version 2.3.2
 *
 * Written by Miguel Masmano Tello <mimastel@doctor.upv.es>
 *
 * Thanks to Ismael Ripoll for his suggestions and reviews
 *
 * Copyright (C) 2007, 2006, 2005, 2004
 *
 * This code is released using a dual license strategy: GPL/LGPL
 * You can choose the licence that better fits your requirements.
 *
 * Released under the terms of the GNU General Public License Version 2.0
 * Released under the terms of the GNU Lesser General Public License Version 2.1
 *
 * This is kernel port of TLSF allocator.
 * Original code can be found at: http://rtportal.upv.es/rtmalloc/
 * 	- Nitin Gupta (nitingupta910@gmail.com)
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/string.h>

#include "compat.h"
#include "tlsf_int.h"
#include "tlsf.h"

static spinlock_t pool_list_lock;
static struct list_head pool_list_head;

#if defined(CONFIG_TLSF_STATS)
static struct proc_dir_entry *proc;

/* Print in format similar to /proc/slabinfo -- easy to awk */
static void print_tlsfinfo_header(struct seq_file *tlsf)
{
	seq_puts(tlsf, "# Size in kB\n");
	seq_puts(tlsf, "# name         <init_size> <max_size> <grow_size>"
			" <used_size> <total_size> <extra>");
#if defined(CONFIG_TLSF_STATS)
	seq_puts(tlsf, " <peak_used> <peak_total> <peak_extra>"
		" <count_alloc> <count_free> <count_region_alloc>"
		" <count_region_free> <failed_alloc>");
#endif
#if defined(CONFIG_TLSF_DEBUG)
	seq_puts(tlsf, " <valid>");
#endif

	seq_putc(tlsf, '\n');
}

/* Get pool no. (*pos) from pool list */
static void *tlsf_start(struct seq_file *tlsf, loff_t *pos)
{
	struct list_head *lp;
	loff_t p = *pos;

	spin_lock(&pool_list_lock);
	if (!p)
		return SEQ_START_TOKEN;

	list_for_each(lp, &pool_list_head) {
		if (!--p)
			return lp;
	}

	return NULL;
}

/* Get pool next to the one given by tlsf_start()/previous tlsf_next() */
static void *tlsf_next(struct seq_file *tlsf, void *v, loff_t *pos)
{
	struct list_head *lp;

	if (v == SEQ_START_TOKEN)
		lp = &pool_list_head;
	else
		lp = v;

	lp = lp->next;
	if (lp == &pool_list_head)
		return NULL;

	++*pos;
	return lp;
}

static void tlsf_stop(struct seq_file *tlsf, void *v)
{
	spin_unlock(&pool_list_lock);
}

/* Display stats for pool given by tlsf_next() */
static int tlsf_show(struct seq_file *tlsf, void *v)
{
	struct pool *pool;
	size_t used, total;
	if (v == SEQ_START_TOKEN) {
		print_tlsfinfo_header(tlsf);
		return 0;
	}

	pool = list_entry(v, struct pool, list);
	used = tlsf_get_used_size(pool);
	total = tlsf_get_total_size(pool);
#define K(x)	((x) >> 10)
	seq_printf(tlsf, "%-16s %6zu %6zu %6zu %6zu %6zu %6zu",
			pool->name,
			K(pool->init_size),
			K(pool->max_size),
			K(pool->grow_size),
			K(used),
			K(total),
			K(total - used));

#if defined(CONFIG_TLSF_STATS)
	seq_printf(tlsf, " %6zu %6zu %6zu %6zu %6zu %6zu %6zu %6zu",
			K(pool->peak_used),
			K(pool->peak_total),
			K(pool->peak_extra),
			pool->count_alloc,
			pool->count_free,
			pool->count_region_alloc,
			pool->count_region_free,
			pool->count_failed_alloc);
#endif

#if defined(CONFIG_TLSF_DEBUG)
	seq_printf(tlsf, " %u", pool->valid);
#endif

	seq_putc(tlsf, '\n');
	return 0;
}

static struct seq_operations tlsfinfo_op = {
	.start = tlsf_start,
	.next = tlsf_next,
	.stop = tlsf_stop,
	.show = tlsf_show,
};

static int tlsfinfo_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &tlsfinfo_op);
}

static const struct file_operations proc_tlsfinfo_operations = {
	.open		= tlsfinfo_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release,
};
#endif	/* CONFIG_TLSF_STATS */

/*
 * Helping functions
 */

/**
 * Returns indexes (fl, sl) of the list used to serve request of size r
 */
static inline void MAPPING_SEARCH(size_t *r, int *fl, int *sl)
{
	int t;

	if (*r < SMALL_BLOCK) {
		*fl = 0;
		*sl = *r / (SMALL_BLOCK / MAX_SLI);
	} else {
		t = (1 << (fls(*r) - 1 - MAX_LOG2_SLI)) - 1;
		*r = *r + t;
		*fl = fls(*r) - 1;
		*sl = (*r >> (*fl - MAX_LOG2_SLI)) - MAX_SLI;
		*fl -= FLI_OFFSET;
		/*if ((*fl -= FLI_OFFSET) < 0) // FL will be always >0!
		 *fl = *sl = 0;
		 */
		*r &= ~t;
	}
}

/**
 * Returns indexes (fl, sl) which is used as starting point to search
 * for a block of size r. It also rounds up requested size(r) to the
 * next list.
 */
static inline void MAPPING_INSERT(size_t r, int *fl, int *sl)
{
	if (r < SMALL_BLOCK) {
		*fl = 0;
		*sl = r / (SMALL_BLOCK / MAX_SLI);
	} else {
		*fl = fls(r) - 1;
		*sl = (r >> (*fl - MAX_LOG2_SLI)) - MAX_SLI;
		*fl -= FLI_OFFSET;
	}
}

/**
 * Returns first block from a list that hold blocks larger than or
 * equal to the one pointed by the indexes (fl, sl)
 */
static inline struct bhdr *FIND_SUITABLE_BLOCK(struct pool *p, int *fl,
						int *sl)
{
	u32 tmp = p->sl_bitmap[*fl] & (~0 << *sl);
	struct bhdr *b = NULL;

	if (tmp) {
		*sl = ffs(tmp) - 1;
		b = p->matrix[*fl][*sl];
	} else {
		*fl = ffs(p->fl_bitmap & (~0 << (*fl + 1))) - 1;
		if (*fl > 0) {		/* likely */
			*sl = ffs(p->sl_bitmap[*fl]) - 1;
			b = p->matrix[*fl][*sl];
		}
	}
	return b;
}

/**
 * Remove first free block(b) from free list with indexes (fl, sl).
 */
static inline void EXTRACT_BLOCK_HDR(struct bhdr *b, struct pool *p, int fl,
					int sl)
{
	p->matrix[fl][sl] = b->ptr.free_ptr.next;
	if (p->matrix[fl][sl])
		p->matrix[fl][sl]->ptr.free_ptr.prev = NULL;
	else {
		clear_bit(sl, (void *)&p->sl_bitmap[fl]);
		if(!p->sl_bitmap[fl])
			clear_bit (fl, (void *)&p->fl_bitmap);
	}
	b->ptr.free_ptr = (struct free_ptr) {NULL, NULL};
}

/**
 * Removes block(b) from free list with indexes (fl, sl)
 */
static inline void EXTRACT_BLOCK(struct bhdr *b, struct pool *p, int fl,
					int sl)
{
	if (b->ptr.free_ptr.next)
		b->ptr.free_ptr.next->ptr.free_ptr.prev =
					b->ptr.free_ptr.prev;
	if (b->ptr.free_ptr.prev)
		b->ptr.free_ptr.prev->ptr.free_ptr.next =
					b->ptr.free_ptr.next;
	if (p->matrix[fl][sl] == b) {
		p->matrix[fl][sl] = b->ptr.free_ptr.next;
		if (!p->matrix[fl][sl]) {
			clear_bit(sl, (void *)&p->sl_bitmap[fl]);
			if (!p->sl_bitmap[fl])
				clear_bit (fl, (void *)&p->fl_bitmap);
		}
	}
	b->ptr.free_ptr = (struct free_ptr) {NULL, NULL};
}

/**
 * Insert block(b) in free list with indexes (fl, sl)
 */
static inline void INSERT_BLOCK(struct bhdr *b, struct pool *p, int fl, int sl)
{
	b->ptr.free_ptr = (struct free_ptr) {NULL, p->matrix[fl][sl]};
	if (p->matrix[fl][sl])
		p->matrix[fl][sl]->ptr.free_ptr.prev = b;
	p->matrix[fl][sl] = b;
	set_bit(sl, (void *)&p->sl_bitmap[fl]);
	set_bit(fl, (void *)&p->fl_bitmap);
}

/**
 * Region is a virtually contiguous memory region and Pool is
 * collection of such regions
 */
static inline void ADD_REGION(void *region, size_t region_size,
					struct pool *pool)
{
	int fl, sl;
	struct bhdr *b, *lb;

	b = (struct bhdr *)(region);
	b->prev_hdr = NULL;
	b->size = ROUNDDOWN_SIZE(region_size - 2 * BHDR_OVERHEAD)
						| FREE_BLOCK | PREV_USED;
	MAPPING_INSERT(b->size & BLOCK_SIZE_MASK, &fl, &sl);
	INSERT_BLOCK(b, pool, fl, sl);
	/* The sentinel block: allows us to know when we're in the last block */
	lb = GET_NEXT_BLOCK(b->ptr.buffer, b->size & BLOCK_SIZE_MASK);
	lb->prev_hdr = b;
	lb->size = 0 | USED_BLOCK | PREV_FREE;
	pool->used_size += BHDR_OVERHEAD; /* only sentinel block is "used" */
	pool->num_regions++;
	stat_inc(pool->count_region_alloc);
}

/*
 * Allocator code start
 */

/**
 * tlsf_create_memory_pool - create dynamic memory pool
 * @name: name of the pool
 * @get_mem: callback function used to expand pool
 * @put_mem: callback function used to shrink pool
 * @init_size: inital pool size (in bytes)
 * @max_size: maximum pool size (in bytes) - set this as 0 for no limit
 * @grow_size: amount of memory (in bytes) added to pool whenever required
 *
 * All size values are rounded up to next page boundary.
 */
void *tlsf_create_memory_pool(const char *name,
			get_memory get_mem,
			put_memory put_mem,
			size_t init_size,
			size_t max_size,
			size_t grow_size)
{
	struct pool *pool;
	void *region;
#if defined(CONFIG_TLSF_STATS)
	size_t used, total;
#endif

	if (max_size)	
		BUG_ON(max_size < init_size);

	pool = get_mem(ROUNDUP_SIZE(sizeof(*pool)));
	if (pool == NULL)
		goto out;
	memset(pool, 0, ROUNDUP_SIZE(sizeof(*pool)));

	/* Round to next page boundary */
	init_size = ROUNDUP_PAGE(init_size);
	max_size = ROUNDUP_PAGE(max_size);
	grow_size = ROUNDUP_PAGE(grow_size);
	pr_info(T "pool: %p, init_size=%zu, max_size=%zu, grow_size=%zu\n",
			pool, init_size, max_size, grow_size);

	/* pool global overhead not included in used size */
	pool->used_size = 0;

	pool->init_size = init_size;
	pool->max_size = max_size;
	pool->grow_size = grow_size;
	pool->get_mem = get_mem;
	pool->put_mem = put_mem;
	strncpy(pool->name, name, MAX_POOL_NAME_LEN);
	pool->name[MAX_POOL_NAME_LEN - 1] = '\0';
#if defined(CONFIG_TLSF_DEBUG)
	pool->valid = 1;
#endif
	region = get_mem(init_size);
	if (region == NULL)
		goto out_region;
	ADD_REGION(region, init_size, pool);
	pool->init_region = region;

	spin_lock_init(&pool->lock);

	spin_lock(&pool_list_lock);
	list_add_tail(&pool->list, &pool_list_head);
	spin_unlock(&pool_list_lock);

	/* Pool created: update stats */
	stat_set(used, tlsf_get_used_size(pool));
	stat_set(total, tlsf_get_total_size(pool));
	stat_inc(pool->count_alloc);
	stat_setmax(pool->peak_extra, total - used);
	stat_setmax(pool->peak_used, used);
	stat_setmax(pool->peak_total, total);

	return pool;

out_region:
	put_mem(pool);

out:
	return NULL;
}
EXPORT_SYMBOL_GPL(tlsf_create_memory_pool);

/**
 * tlsf_get_used_size - get memory currently used by given pool
 *
 * Used memory includes stored data + metadata + internal fragmentation
 */
size_t tlsf_get_used_size(void *mem_pool)
{
	struct pool *pool = (struct pool *)mem_pool;
	return pool->used_size;
}
EXPORT_SYMBOL_GPL(tlsf_get_used_size);

/**
 * tlsf_get_total_size - get total memory currently allocated for given pool
 *
 * This is the total memory currently allocated for this pool which includes
 * used size + free size.
 *
 * (Total - Used) is good indicator of memory efficiency of allocator.
 */
size_t tlsf_get_total_size(void *mem_pool)
{
	size_t total;
	struct pool *pool = (struct pool *)mem_pool;
	total = ROUNDUP_SIZE(sizeof(*pool))
		+ pool->init_size
		+ (pool->num_regions - 1) * pool->grow_size;
	return total;
}
EXPORT_SYMBOL_GPL(tlsf_get_total_size);

/**
 * tlsf_destory_memory_pool - cleanup given pool
 * @mem_pool: Pool to be destroyed
 *
 * Data structures associated with pool are freed.
 * All memory allocated from pool must be freed before
 * destorying it.
 */
void tlsf_destroy_memory_pool(void *mem_pool) 
{
	struct pool *pool;

	if (mem_pool == NULL)
		return;

	pool = (struct pool *)mem_pool;

	/* User is destorying without ever allocating from this pool */
	if (tlsf_get_used_size(pool) == BHDR_OVERHEAD) {
		pool->put_mem(pool->init_region);
		pool->used_size -= BHDR_OVERHEAD;
		stat_inc(pool->count_region_free);
	}

	/* Check for memory leaks in this pool */
	if (tlsf_get_used_size(pool)) {
		pr_warning(T "memory leak in pool: %s (%p). "
			"%zu bytes still in use.\n",
			pool->name, pool, tlsf_get_used_size(pool));

#if defined(CONFIG_TLSF_DEBUG)
		pool->valid = 0;
		/* Invalid pools stay in list for debugging purpose */
		return;
#endif
	}
	spin_lock(&pool_list_lock);
	list_del_init(&pool->list);
	spin_unlock(&pool_list_lock);
	pool->put_mem(pool);
}
EXPORT_SYMBOL_GPL(tlsf_destroy_memory_pool);

/**
 * tlsf_malloc - allocate memory from given pool
 * @size: no. of bytes
 * @mem_pool: pool to allocate from
 */
void *tlsf_malloc(size_t size, void *mem_pool)
{
	struct pool *pool = (struct pool *)mem_pool;
	struct bhdr *b, *b2, *next_b, *region;
	int fl, sl;
	size_t tmp_size;
#if defined(CONFIG_TLSF_STATS)
	size_t used, total;
#endif

#if defined(CONFIG_TLSF_DEBUG)
	unsigned int retries = 0;
#endif

	size = (size < MIN_BLOCK_SIZE) ? MIN_BLOCK_SIZE : ROUNDUP_SIZE(size);
	/* Rounding up the requested size and calculating fl and sl */

	spin_lock(&pool->lock);
retry_find:
	MAPPING_SEARCH(&size, &fl, &sl);

	/* Searching a free block */
	if (!(b = FIND_SUITABLE_BLOCK(pool, &fl, &sl))) {
#if defined(CONFIG_TLSF_DEBUG)
		/*
		 * This can happen if there are too many users
		 * allocating from this pool simultaneously.
		 */
		if (unlikely(retries == MAX_RETRY_EXPAND))
			goto out_locked;
		retries++;
#endif
		/* Not found */
		if (size > (pool->grow_size - 2 * BHDR_OVERHEAD))
			goto out_locked;
		if (pool->max_size && (pool->init_size +
				pool->num_regions * pool->grow_size
				> pool->max_size))
			goto out_locked;
		spin_unlock(&pool->lock);
		if ((region = pool->get_mem(pool->grow_size)) == NULL)
			goto out;
		spin_lock(&pool->lock);
		ADD_REGION(region, pool->grow_size, pool);
		goto retry_find;
	}
	EXTRACT_BLOCK_HDR(b, pool, fl, sl);

	/*-- found: */
	next_b = GET_NEXT_BLOCK(b->ptr.buffer, b->size & BLOCK_SIZE_MASK);
	/* Should the block be split? */
	tmp_size = (b->size & BLOCK_SIZE_MASK) - size;
	if (tmp_size >= sizeof(struct bhdr) ) {
		tmp_size -= BHDR_OVERHEAD;
		b2 = GET_NEXT_BLOCK(b->ptr.buffer, size);

		b2->size = tmp_size | FREE_BLOCK | PREV_USED;
		b2->prev_hdr = b;

		next_b->prev_hdr = b2;

		MAPPING_INSERT(tmp_size, &fl, &sl);
		INSERT_BLOCK(b2, pool, fl, sl);

		b->size = size | (b->size & PREV_STATE);
	} else {
		next_b->size &= (~PREV_FREE);
		b->size &= (~FREE_BLOCK);	/* Now it's used */
	}

	pool->used_size += (b->size & BLOCK_SIZE_MASK) + BHDR_OVERHEAD;

	/* Successful alloc: update stats. */
	stat_set(used, tlsf_get_used_size(pool));
	stat_set(total, tlsf_get_total_size(pool));
	stat_inc(pool->count_alloc);
	stat_setmax(pool->peak_extra, total - used);
	stat_setmax(pool->peak_used, used);
	stat_setmax(pool->peak_total, total);

	spin_unlock(&pool->lock);
	return (void *)b->ptr.buffer;

	/* Failed alloc */
out_locked:
	spin_unlock(&pool->lock);

out:
	stat_inc(pool->count_failed_alloc);
	return NULL;
}
EXPORT_SYMBOL_GPL(tlsf_malloc);

/**
 * tlsf_free - free memory from given pool
 * @ptr: address of memory to be freed
 * @mem_pool: pool to free from
 */
void tlsf_free(void *ptr, void *mem_pool)
{
	struct pool *pool = (struct pool *)mem_pool;
	struct bhdr *b, *tmp_b;
	int fl = 0, sl = 0;
#if defined(CONFIG_TLSF_STATS)
	size_t used, total;
#endif
	if (unlikely(ptr == NULL))
		return;

	b = (struct bhdr *) ((char *) ptr - BHDR_OVERHEAD);

	spin_lock(&pool->lock);
	b->size |= FREE_BLOCK;
	pool->used_size -= (b->size & BLOCK_SIZE_MASK) + BHDR_OVERHEAD;
	b->ptr.free_ptr = (struct free_ptr) { NULL, NULL};
	tmp_b = GET_NEXT_BLOCK(b->ptr.buffer, b->size & BLOCK_SIZE_MASK);
	if (tmp_b->size & FREE_BLOCK) {
		MAPPING_INSERT(tmp_b->size & BLOCK_SIZE_MASK, &fl, &sl);
		EXTRACT_BLOCK(tmp_b, pool, fl, sl);
		b->size += (tmp_b->size & BLOCK_SIZE_MASK) + BHDR_OVERHEAD;
	}
	if (b->size & PREV_FREE) {
		tmp_b = b->prev_hdr;
		MAPPING_INSERT(tmp_b->size & BLOCK_SIZE_MASK, &fl, &sl);
		EXTRACT_BLOCK(tmp_b, pool, fl, sl);
		tmp_b->size += (b->size & BLOCK_SIZE_MASK) + BHDR_OVERHEAD;
		b = tmp_b;
	}
	tmp_b = GET_NEXT_BLOCK(b->ptr.buffer, b->size & BLOCK_SIZE_MASK);
	tmp_b->prev_hdr = b;

	MAPPING_INSERT(b->size & BLOCK_SIZE_MASK, &fl, &sl);

	if ((b->prev_hdr == NULL) && ((tmp_b->size & BLOCK_SIZE_MASK) == 0)) {
		pool->put_mem(b);
		pool->num_regions--;
		pool->used_size -= BHDR_OVERHEAD; /* sentinel block header */
		stat_inc(pool->count_region_free);
		goto out;
	}
	
	INSERT_BLOCK(b, pool, fl, sl);

	tmp_b->size |= PREV_FREE;
	tmp_b->prev_hdr = b;
out:
	/* Update stats */
	stat_set(used, tlsf_get_used_size(pool));
	stat_set(total, tlsf_get_total_size(pool));
	stat_inc(pool->count_free);
	stat_setmax(pool->peak_extra, total - used);
	stat_setmax(pool->peak_used, used);
	stat_setmax(pool->peak_total, total);

	spin_unlock(&pool->lock);
}
EXPORT_SYMBOL_GPL(tlsf_free);

/**
 * tlsf_calloc - allocate and zero-out memory from given pool
 * @size: no. of bytes
 * @mem_pool: pool to allocate from
 */
void *tlsf_calloc(size_t nelem, size_t elem_size, void *mem_pool)
{
	void *ptr;

	if (nelem == 0 || elem_size == 0)
		return NULL;

	if ((ptr = tlsf_malloc(nelem * elem_size, mem_pool)) == NULL)
		return NULL;
	memset(ptr, 0, nelem * elem_size);

	return ptr;
}
EXPORT_SYMBOL_GPL(tlsf_calloc);

static int __init tlsf_init(void)
{
	INIT_LIST_HEAD(&pool_list_head);
	spin_lock_init(&pool_list_lock);
#if defined(CONFIG_TLSF_STATS)
	proc = create_proc_entry("tlsfinfo", S_IRUGO, NULL);
	if (proc)
		proc->proc_fops = &proc_tlsfinfo_operations;
	else
		pr_warning(T "error creating proc entry\n");
#endif
	return 0;
}

static void __exit tlsf_exit(void)
{
#if defined(CONFIG_TLSF_STATS)
	if (proc)
		remove_proc_entry("tlsfinfo", proc->parent);
#endif
	return;
}

module_init(tlsf_init);
module_exit(tlsf_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Nitin Gupta <nitingupta910@gmail.com>");
MODULE_DESCRIPTION("TLSF Memory Allocator");
