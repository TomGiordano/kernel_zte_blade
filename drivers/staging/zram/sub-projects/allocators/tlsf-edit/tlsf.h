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
 */

#ifndef _TLSF_H_
#define _TLSF_H_

#include <sys/types.h>

typedef void* (get_memory)(size_t bytes);
typedef void (put_memory)(void *ptr);

extern void *tlsf_create_memory_pool(get_memory get_mem,
					put_memory put_mem,
					size_t init_size,
					size_t max_size,
					size_t grow_size);
extern void tlsf_destroy_memory_pool(void *mem_pool);
extern void *tlsf_malloc(size_t size, void *mem_pool);
extern void *tlsf_calloc(size_t nelem, size_t elem_size, void *mem_pool);
extern void tlsf_free(void *ptr, void *mem_pool);
extern size_t tlsf_get_used_size(void *mem_pool);
extern size_t tlsf_get_total_size(void *mem_pool);

#endif
