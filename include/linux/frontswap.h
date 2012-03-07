#ifndef _LINUX_FRONTSWAP_H
#define _LINUX_FRONTSWAP_H

#include <linux/swap.h>
#include <linux/mm.h>

struct frontswap_ops {
	void (*init)(unsigned);
	int (*put_page)(unsigned, pgoff_t, struct page *);
	int (*get_page)(unsigned, pgoff_t, struct page *);
	void (*flush_page)(unsigned, pgoff_t);
	void (*flush_area)(unsigned);
};

extern int frontswap_enabled;
extern struct frontswap_ops
	frontswap_register_ops(struct frontswap_ops *ops);
extern void frontswap_shrink(unsigned long);
extern unsigned long frontswap_curr_pages(void);

extern void frontswap_init(unsigned type);
extern int __frontswap_put_page(struct page *page);
extern int __frontswap_get_page(struct page *page);
extern void __frontswap_flush_page(unsigned, pgoff_t);
extern void __frontswap_flush_area(unsigned);

#ifndef CONFIG_FRONTSWAP
/* all inline routines become no-ops and all externs are ignored */
#define frontswap_enabled (0)
#endif

static inline int frontswap_test(struct swap_info_struct *sis, pgoff_t offset)
{
	int ret = 0;

	if (frontswap_enabled && sis->frontswap_map)
		ret = test_bit(offset % BITS_PER_LONG,
			&sis->frontswap_map[offset/BITS_PER_LONG]);
	return ret;
}

static inline void frontswap_set(struct swap_info_struct *sis, pgoff_t offset)
{
	if (frontswap_enabled && sis->frontswap_map)
		set_bit(offset % BITS_PER_LONG,
			&sis->frontswap_map[offset/BITS_PER_LONG]);
}

static inline void frontswap_clear(struct swap_info_struct *sis, pgoff_t offset)
{
	if (frontswap_enabled && sis->frontswap_map)
		clear_bit(offset % BITS_PER_LONG,
			&sis->frontswap_map[offset/BITS_PER_LONG]);
}

static inline int frontswap_put_page(struct page *page)
{
	int ret = -1;

	if (frontswap_enabled)
		ret = __frontswap_put_page(page);
	return ret;
}

static inline int frontswap_get_page(struct page *page)
{
	int ret = -1;

	if (frontswap_enabled)
		ret = __frontswap_get_page(page);
	return ret;
}

static inline void frontswap_flush_page(unsigned type, pgoff_t offset)
{
	if (frontswap_enabled)
		__frontswap_flush_page(type, offset);
}

static inline void frontswap_flush_area(unsigned type)
{
	if (frontswap_enabled)
		__frontswap_flush_area(type);
}

#endif /* _LINUX_FRONTSWAP_H */

