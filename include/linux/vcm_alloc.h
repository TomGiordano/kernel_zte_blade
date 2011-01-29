/* Copyright (c) 2010, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of Code Aurora Forum, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef VCM_ALLOC_H
#define VCM_ALLOC_H

#include <linux/list.h>

#define NUM_CHUNK_SIZES 3

enum chunk_size_idx {
	IDX_1M = 0,
	IDX_64K,
	IDX_4K
};

struct phys_chunk {
	struct list_head list;
	struct list_head allocated; /* used to record is allocated */

	struct list_head refers_to;

	/* TODO: change to unsigned long */
	int pa;
	int size_idx;
};

int vcm_alloc_get_mem_size(void);
int vcm_alloc_blocks_avail(enum chunk_size_idx idx);
int vcm_alloc_get_num_chunks(void);
int vcm_alloc_all_blocks_avail(void);
int vcm_alloc_count_allocated(void);
void vcm_alloc_print_list(int just_allocated);
int vcm_alloc_idx_to_size(int idx);
int vcm_alloc_destroy(void);
int vcm_alloc_init(unsigned int set_base_pa);
int vcm_alloc_free_blocks(struct phys_chunk *alloc_head);
int vcm_alloc_num_blocks(int num,
			 enum chunk_size_idx idx, /* chunk size */
			 struct phys_chunk *alloc_head);
int vcm_alloc_max_munch(int len,
			struct phys_chunk *alloc_head);

#endif /* VCM_ALLOC_H */
