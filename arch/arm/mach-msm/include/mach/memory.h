/* arch/arm/mach-msm/include/mach/memory.h
 *
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __ASM_ARCH_MEMORY_H
#define __ASM_ARCH_MEMORY_H

/* physical offset of RAM */
#define PHYS_OFFSET UL(CONFIG_PHYS_OFFSET)

#define MAX_PHYSMEM_BITS 32
#define SECTION_SIZE_BITS 28

#define HAS_ARCH_IO_REMAP_PFN_RANGE

#ifndef __ASSEMBLY__
void *alloc_bootmem_aligned(unsigned long size, unsigned long alignment);
unsigned long allocate_contiguous_ebi_nomap(unsigned long, unsigned long);
void clean_and_invalidate_caches(unsigned long, unsigned long, unsigned long);
void clean_caches(unsigned long, unsigned long, unsigned long);
void invalidate_caches(unsigned long, unsigned long, unsigned long);
int platform_physical_remove_pages(unsigned long, unsigned long);
int platform_physical_active_pages(unsigned long, unsigned long);
int platform_physical_low_power_pages(unsigned long, unsigned long);

#ifdef CONFIG_ARCH_MSM_ARM11
void write_to_strongly_ordered_memory(void);
void map_page_strongly_ordered(void);
#endif

#ifdef CONFIG_CACHE_L2X0
extern void l2x0_cache_sync(void);
#define finish_arch_switch(prev)     do { l2x0_cache_sync(); } while (0)
#endif

#endif

#if defined CONFIG_ARCH_MSM_SCORPION || defined CONFIG_ARCH_MSM_SCORPIONMP
#define arch_has_speculative_dfetch()	1
#endif

#endif

/* these correspond to values known by the modem */
#define MEMORY_DEEP_POWERDOWN	0
#define MEMORY_SELF_REFRESH	1
#define MEMORY_ACTIVE		2

#define NPA_MEMORY_NODE_NAME	"/mem/apps/ddr_dpd"
