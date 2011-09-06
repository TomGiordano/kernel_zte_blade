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

#ifndef _TLSF_H_
#define _TLSF_H_

typedef void* (get_memory)(size_t bytes);
typedef void (put_memory)(void *ptr);

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
extern void *tlsf_create_memory_pool(const char *name,
					get_memory get_mem,
					put_memory put_mem,
					size_t init_size,
					size_t max_size,
					size_t grow_size);
/**
 * tlsf_destory_memory_pool - cleanup given pool
 * @mem_pool: Pool to be destroyed
 *
 * Data structures associated with pool are freed.
 * All memory allocated from pool must be freed before
 * destorying it.
 */
extern void tlsf_destroy_memory_pool(void *mem_pool);

/**
 * tlsf_malloc - allocate memory from given pool
 * @size: no. of bytes
 * @mem_pool: pool to allocate from
 */
extern void *tlsf_malloc(size_t size, void *mem_pool);

/**
 * tlsf_calloc - allocate and zero-out memory from given pool
 * @size: no. of bytes
 * @mem_pool: pool to allocate from
 */
extern void *tlsf_calloc(size_t nelem, size_t elem_size, void *mem_pool);

/**
 * tlsf_free - free memory from given pool
 * @ptr: address of memory to be freed
 * @mem_pool: pool to free from
 */
extern void tlsf_free(void *ptr, void *mem_pool);

/**
 * tlsf_get_used_size - get memory currently used by given pool
 *
 * Used memory includes stored data + metadata + internal fragmentation
 */
extern size_t tlsf_get_used_size(void *mem_pool);

/**
 * tlsf_get_total_size - get total memory currently allocated for given pool
 *
 * This is the total memory currently allocated for this pool which includes
 * used size + free size.
 *
 * (Total - Used) is good indicator of memory efficiency of allocator.
 */
extern size_t tlsf_get_total_size(void *mem_pool);

#endif
