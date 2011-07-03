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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/vcm_mm.h>
#include <linux/vcm.h>
#include <linux/vcm_types.h>
#include <linux/errno.h>
#include <linux/spinlock.h>

#include <asm/page.h>
#include <asm/sizes.h>

#ifdef CONFIG_SMMU
#include <mach/smmu_driver.h>
#endif

/* alloc_vm_area */
#include <linux/pfn.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>

/* may be temporary */
#include <linux/bootmem.h>

#include <asm/cacheflush.h>
#include <asm/mach/map.h>

#define BOOTMEM_SZ	SZ_32M
#define BOOTMEM_ALIGN	SZ_1M

#define CONT_SZ		SZ_8M
#define CONT_ALIGN	SZ_1M

#define ONE_TO_ONE_CHK 1

#define vcm_err(a, ...)							\
	pr_err("ERROR %s %i " a, __func__, __LINE__, ##__VA_ARGS__)

static void *bootmem;
static void *bootmem_cont;
static struct vcm *cont_vcm_id;
static struct phys_chunk *cont_phys_chunk;

DEFINE_SPINLOCK(vcmlock);

static int vcm_no_res(struct vcm *vcm)
{
	if (!vcm) {
		vcm_err("NULL vcm\n");
		goto fail;
	}

	return list_empty(&vcm->res_head);
fail:
	return -EINVAL;
}

static int vcm_no_assoc(struct vcm *vcm)
{
	if (!vcm) {
		vcm_err("NULL vcm\n");
		goto fail;
	}

	return list_empty(&vcm->assoc_head);
fail:
	return -EINVAL;
}

static int vcm_all_activated(struct vcm *vcm)
{
	struct avcm *avcm;

	if (!vcm) {
		vcm_err("NULL vcm\n");
		goto fail;
	}

	list_for_each_entry(avcm, &vcm->assoc_head, assoc_elm)
		if (!avcm->is_active)
			return 0;

	return 1;
fail:
	return -1;
}

static void vcm_destroy_common(struct vcm *vcm)
{
	if (!vcm) {
		vcm_err("NULL vcm\n");
		return;
	}

	memset(vcm, 0, sizeof(*vcm));
	kfree(vcm);
}

static struct vcm *vcm_create_common(void)
{
	struct vcm *vcm = 0;

	vcm = kzalloc(sizeof(*vcm), GFP_KERNEL);
	if (!vcm) {
		vcm_err("kzalloc(%i, GFP_KERNEL) ret 0\n",
			sizeof(*vcm));
		goto fail;
	}

	INIT_LIST_HEAD(&vcm->res_head);
	INIT_LIST_HEAD(&vcm->assoc_head);

	return vcm;

fail:
	return NULL;
}


static int vcm_create_pool(struct vcm *vcm, size_t start_addr, size_t len)
{
	int ret = 0;

	if (!vcm) {
		vcm_err("NULL vcm\n");
		goto fail;
	}

	vcm->start_addr = start_addr;
	vcm->len = len;

	vcm->pool = gen_pool_create(PAGE_SHIFT, -1);
	if (!vcm->pool) {
		vcm_err("gen_pool_create(%x, -1) ret 0\n", PAGE_SHIFT);
		goto fail;
	}

	ret = gen_pool_add(vcm->pool, start_addr, len, -1);
	if (ret) {
		vcm_err("gen_pool_add(%p, %p, %i, -1) ret %i\n", vcm->pool,
			(void *) start_addr, len, ret);
		goto fail2;
	}

	return 0;

fail2:
	gen_pool_destroy(vcm->pool);
fail:
	return -1;
}


static struct vcm *vcm_create_flagged(int flag, size_t start_addr, size_t len)
{
	int ret = 0;
	struct vcm *vcm = 0;

	vcm = vcm_create_common();
	if (!vcm) {
		vcm_err("NULL vcm\n");
		goto fail;
	}

	/* special one-to-one mapping case */
	if ((flag & ONE_TO_ONE_CHK) &&
	    bootmem_cont &&
	    __pa(bootmem_cont) &&
	    start_addr == __pa(bootmem_cont) &&
	    len == CONT_SZ) {
		vcm->type = VCM_ONE_TO_ONE;
	} else {
		ret = vcm_create_pool(vcm, start_addr, len);
		vcm->type = VCM_DEVICE;
	}

	if (ret) {
		vcm_err("vcm_create_pool(%p, %p, %i) ret %i\n", vcm,
			(void *) start_addr, len, ret);
		goto fail2;
	}

	return vcm;

fail2:
	vcm_destroy_common(vcm);
fail:
	return NULL;
}

struct vcm *vcm_create(size_t start_addr, size_t len)
{
	unsigned long flags;
	struct vcm *vcm;

	spin_lock_irqsave(&vcmlock, flags);
	vcm = vcm_create_flagged(ONE_TO_ONE_CHK, start_addr, len);
	spin_unlock_irqrestore(&vcmlock, flags);
	return vcm;
}


static int ext_vcm_id_valid(size_t ext_vcm_id)
{
	return ((ext_vcm_id == VCM_PREBUILT_KERNEL) ||
		(ext_vcm_id == VCM_PREBUILT_USER));
}


struct vcm *vcm_create_from_prebuilt(size_t ext_vcm_id)
{
	unsigned long flags;
	struct vcm *vcm = 0;

	spin_lock_irqsave(&vcmlock, flags);

	if (!ext_vcm_id_valid(ext_vcm_id)) {
		vcm_err("ext_vcm_id_valid(%i) ret 0\n", ext_vcm_id);
		goto fail;
	}

	vcm = vcm_create_common();
	if (!vcm) {
		vcm_err("NULL vcm\n");
		goto fail;
	}

	if (ext_vcm_id == VCM_PREBUILT_KERNEL)
		vcm->type = VCM_EXT_KERNEL;
	else if (ext_vcm_id == VCM_PREBUILT_USER)
		vcm->type = VCM_EXT_USER;
	else {
		vcm_err("UNREACHABLE ext_vcm_id is illegal\n");
		goto fail_free;
	}

	/* TODO: set kernel and userspace start_addr and len, if this
	 * makes sense */

	spin_unlock_irqrestore(&vcmlock, flags);
	return vcm;

fail_free:
	vcm_destroy_common(vcm);
fail:
	spin_unlock_irqrestore(&vcmlock, flags);
	return NULL;
}


struct vcm *vcm_clone(struct vcm *vcm_id)
{
	return 0;
}


/* No lock needed, vcm->start_addr is never updated after creation */
size_t vcm_get_start_addr(struct vcm *vcm)
{
	if (!vcm) {
		vcm_err("NULL vcm\n");
		return 1;
	}

	return vcm->start_addr;
}


/* No lock needed, vcm->len is never updated after creation */
size_t vcm_get_len(struct vcm *vcm)
{
	if (!vcm) {
		vcm_err("NULL vcm\n");
		return 0;
	}

	return vcm->len;
}


static int vcm_free_common_rule(struct vcm *vcm)
{
	int ret;

	if (!vcm) {
		vcm_err("NULL vcm\n");
		goto fail;
	}

	ret = vcm_no_res(vcm);
	if (!ret) {
		vcm_err("vcm_no_res(%p) ret 0\n", vcm);
		goto fail_busy;
	}

	if (ret == -EINVAL) {
		vcm_err("vcm_no_res(%p) ret -EINVAL\n", vcm);
		goto fail;
	}

	ret = vcm_no_assoc(vcm);
	if (!ret) {
		vcm_err("vcm_no_assoc(%p) ret 0\n", vcm);
		goto fail_busy;
	}

	if (ret == -EINVAL) {
		vcm_err("vcm_no_assoc(%p) ret -EINVAL\n", vcm);
		goto fail;
	}

	return 0;

fail_busy:
	return -EBUSY;
fail:
	return -EINVAL;
}


static int vcm_free_pool_rule(struct vcm *vcm)
{
	if (!vcm) {
		vcm_err("NULL vcm\n");
		goto fail;
	}

	/* A vcm always has a valid pool, don't free the vcm because
	   what we got is probably invalid.
	*/
	if (!vcm->pool) {
		vcm_err("NULL vcm->pool\n");
		goto fail;
	}

	return 0;

fail:
	return -EINVAL;
}


static void vcm_free_common(struct vcm *vcm)
{
	memset(vcm, 0, sizeof(*vcm));

	kfree(vcm);
}


static int vcm_free_pool(struct vcm *vcm)
{
	if (!vcm) {
		vcm_err("NULL vcm\n");
		goto fail;
	}

	gen_pool_destroy(vcm->pool);

	return 0;

fail:
	return -1;
}


static int __vcm_free(struct vcm *vcm)
{
	int ret;

	if (!vcm) {
		vcm_err("NULL vcm\n");
		goto fail;
	}

	ret = vcm_free_common_rule(vcm);
	if (ret != 0) {
		vcm_err("vcm_free_common_rule(%p) ret %i\n", vcm, ret);
		goto fail;
	}

	if (vcm->type == VCM_DEVICE) {
		ret = vcm_free_pool_rule(vcm);
		if (ret != 0) {
			vcm_err("vcm_free_pool_rule(%p) ret %i\n",
				(void *) vcm, ret);
			goto fail;
		}

		ret = vcm_free_pool(vcm);
		if (ret != 0) {
			vcm_err("vcm_free_pool(%p) ret %i", (void *) vcm, ret);
			goto fail;
		}
	}

	vcm_free_common(vcm);

	return 0;

fail:
	return -EINVAL;
}

int vcm_free(struct vcm *vcm)
{
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&vcmlock, flags);
	ret = __vcm_free(vcm);
	spin_unlock_irqrestore(&vcmlock, flags);

	return ret;
}


static struct res *__vcm_reserve(struct vcm *vcm, size_t len, uint32_t attr)
{
	struct res *res = NULL;
	int align_attr = 0;

	if (!vcm) {
		vcm_err("NULL vcm\n");
		goto fail;
	}

	if (len == 0) {
		vcm_err("len is 0\n");
		goto fail;
	}

	res = kzalloc(sizeof(*res), GFP_KERNEL);
	if (!res) {
		vcm_err("kzalloc(%i, GFP_KERNEL) ret 0", sizeof(*res));
		goto fail;
	}

	align_attr = (attr >> VCM_ALIGN_SHIFT) & VCM_ALIGN_MASK;

	if (align_attr >= 32) {
		vcm_err("Invalid alignment attribute: %d\n", align_attr);
		goto fail2;
	}

	INIT_LIST_HEAD(&res->res_elm);
	res->vcm_id = vcm;
	res->len = len;
	res->attr = attr;

	if (align_attr == 0) {
		if (len/SZ_1M)
			res->alignment_req = SZ_1M;
		else if (len/SZ_64K)
			res->alignment_req = SZ_64K;
		else
			res->alignment_req = SZ_4K;
	} else
		res->alignment_req = 1 << align_attr;

	res->aligned_len = res->alignment_req + len;

	switch (vcm->type) {
	case VCM_DEVICE:
		/* should always be not zero */
		if (!vcm->pool) {
			vcm_err("NULL vcm->pool\n");
			goto fail2;
		}

		res->ptr = gen_pool_alloc(vcm->pool, res->aligned_len);
		if (!res->ptr) {
			vcm_err("gen_pool_alloc(%p, %i) ret 0\n",
				vcm->pool, res->aligned_len);
			goto fail2;
		}

		/* Calculate alignment... this will all change anyway */
		res->aligned_ptr = res->ptr +
			(res->alignment_req -
			 (res->ptr & (res->alignment_req - 1)));

		break;
	case VCM_EXT_KERNEL:
		res->vm_area = alloc_vm_area(res->aligned_len);
		res->mapped = 0; /* be explicit */
		if (!res->vm_area) {
			vcm_err("NULL res->vm_area\n");
			goto fail2;
		}

		res->aligned_ptr = (size_t) res->vm_area->addr +
			(res->alignment_req -
			 ((size_t) res->vm_area->addr &
			  (res->alignment_req - 1)));

		break;
	case VCM_ONE_TO_ONE:
		break;
	default:
		vcm_err("%i is an invalid vcm->type\n", vcm->type);
		goto fail2;
	}

	list_add_tail(&res->res_elm, &vcm->res_head);

	return res;

fail2:
	kfree(res);
fail:
	return 0;
}


struct res *vcm_reserve(struct vcm *vcm, size_t len, uint32_t attr)
{
	unsigned long flags;
	struct res *res;

	spin_lock_irqsave(&vcmlock, flags);
	res = __vcm_reserve(vcm, len, attr);
	spin_unlock_irqrestore(&vcmlock, flags);

	return res;
}


struct res *vcm_reserve_at(enum memtarget_t memtarget, struct vcm* vcm,
		     size_t len, uint32_t attr)
{
	return 0;
}


/* No lock needed, res->vcm_id is never updated after creation */
struct vcm *vcm_get_vcm_from_res(struct res *res)
{
	if (!res) {
		vcm_err("NULL res\n");
		return 0;
	}

	return res->vcm_id;
}


static int __vcm_unreserve(struct res *res)
{
	struct vcm *vcm;

	if (!res) {
		vcm_err("NULL res\n");
		goto fail;
	}

	if (!res->vcm_id) {
		vcm_err("NULL res->vcm_id\n");
		goto fail;
	}

	vcm = res->vcm_id;
	if (!vcm) {
		vcm_err("NULL vcm\n");
		goto fail;
	}

	switch (vcm->type) {
	case VCM_DEVICE:
		if (!res->vcm_id->pool) {
			vcm_err("NULL (res->vcm_id))->pool\n");
			goto fail;
		}

		/* res->ptr could be zero, this isn't an error */
		gen_pool_free(res->vcm_id->pool, res->ptr,
			      res->aligned_len);
		break;
	case VCM_EXT_KERNEL:
		if (res->mapped) {
			vcm_err("res->mapped is true\n");
			goto fail;
		}

		/* This may take a little explaining.
		 * In the kernel vunmap will free res->vm_area
		 * so if we've called it then we shouldn't call
		 * free_vm_area(). If we've called it we set
		 * res->vm_area to 0.
		 */
		if (res->vm_area) {
			free_vm_area(res->vm_area);
			res->vm_area = 0;
		}

		break;
	case VCM_ONE_TO_ONE:
		break;
	default:
		vcm_err("%i is an invalid vcm->type\n", vcm->type);
		goto fail;
	}

	list_del(&res->res_elm);

	/* be extra careful by clearing the memory before freeing it */
	memset(res, 0, sizeof(*res));

	kfree(res);

	return 0;

fail:
	return -EINVAL;
}


int vcm_unreserve(struct res *res)
{
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&vcmlock, flags);
	ret = __vcm_unreserve(res);
	spin_unlock_irqrestore(&vcmlock, flags);

	return ret;
}


/* No lock needed, res->len is never updated after creation */
size_t vcm_get_res_len(struct res *res)
{
	if (!res) {
		vcm_err("res is 0\n");
		return 0;
	}

	return res->len;
}


int vcm_set_res_attr(struct res *res, uint32_t attr)
{
	return 0;
}


uint32_t vcm_get_res_attr(struct res *res)
{
	return 0;
}


size_t vcm_get_num_res(struct vcm *vcm)
{
	return 0;
}


struct res *vcm_get_next_res(struct vcm *vcm, struct res *res)
{
	return 0;
}


size_t vcm_res_copy(struct res *to, size_t to_off, struct res *from,
		    size_t from_off, size_t len)
{
	return 0;
}


size_t vcm_get_min_page_size(void)
{
	return PAGE_SIZE;
}


static int vcm_to_smmu_attr(uint32_t attr)
{
	int smmu_attr = 0;

	switch (attr & VCM_CACHE_POLICY) {
	case VCM_NOTCACHED:
		smmu_attr = VCM_DEV_ATTR_NONCACHED;
		break;
	case VCM_WB_WA:
		smmu_attr = VCM_DEV_ATTR_CACHED_WB_WA;
		smmu_attr |= VCM_DEV_ATTR_SH;
		break;
	case VCM_WB_NWA:
		smmu_attr = VCM_DEV_ATTR_CACHED_WB_NWA;
		smmu_attr |= VCM_DEV_ATTR_SH;
		break;
	case VCM_WT:
		smmu_attr = VCM_DEV_ATTR_CACHED_WT;
		smmu_attr |= VCM_DEV_ATTR_SH;
		break;
	default:
		return -1;
	}

	return smmu_attr;
}


static int vcm_process_chunk(size_t dev_id, unsigned long pa, unsigned long va,
			unsigned long len, unsigned int attr, int map)
{
	int ret;
	unsigned long map_len = SZ_4K;

	if (IS_ALIGNED(va, SZ_64K) && len >= SZ_64K)
		map_len = SZ_64K;

	if (IS_ALIGNED(va, SZ_1M) && len >= SZ_1M)
		map_len = SZ_1M;

	if (IS_ALIGNED(va, SZ_16M) && len >= SZ_16M)
		map_len = SZ_16M;

#ifdef VCM_PERF_DEBUG
	if (va & (len - 1))
		pr_warning("Warning! Suboptimal VCM mapping alignment "
			   "va = %p, len = %p. Expect TLB performance "
			   "degradation.\n", (void *) va, (void *) len);
#endif

	while (len) {
		if (va & (SZ_4K - 1)) {
			vcm_err("Tried to map w/ align < 4k! va = %08lx\n", va);
			goto fail;
		}

		if (map_len > len) {
			vcm_err("map_len = %lu, len = %lu, trying to overmap\n",
				 map_len, len);
			goto fail;
		}

		if (map)
			ret = smmu_map((struct smmu_dev *) dev_id, pa, va,
								map_len, attr);
		else
			ret = smmu_unmap((struct smmu_dev *) dev_id, va,
								map_len);
		if (ret) {
			vcm_err("smmu_map/unmap(%p, %p, %p, 0x%x, 0x%x) ret %i"
				"map = %d", (void *) dev_id, (void *) pa,
				(void *) va, (int) map_len, attr, ret, map);
			goto fail;
		}

		va += map_len;
		pa += map_len;
		len -= map_len;
	}
	return 0;
fail:
	return -1;
}

/* TBD if you vcm_back again what happens? */
int vcm_back(struct res *res, struct physmem *physmem)
{
	unsigned long flags;
	struct vcm *vcm;
	struct phys_chunk *chunk;
	size_t va = 0;
	int ret;
	int attr;

	spin_lock_irqsave(&vcmlock, flags);

	if (!res) {
		vcm_err("NULL res\n");
		goto fail;
	}

	vcm = res->vcm_id;
	if (!vcm) {
		vcm_err("NULL vcm\n");
		goto fail;
	}

	switch (vcm->type) {
	case VCM_DEVICE:
	case VCM_EXT_KERNEL: /* hack part 1 */
		attr = vcm_to_smmu_attr(res->attr);
		if (attr == -1) {
			vcm_err("Bad SMMU attr\n");
			goto fail;
		}
		break;
	default:
		attr = 0;
		break;
	}

	if (!physmem) {
		vcm_err("NULL physmem\n");
		goto fail;
	}

	if (res->len == 0) {
		vcm_err("res->len is 0\n");
		goto fail;
	}

	if (physmem->len == 0) {
		vcm_err("physmem->len is 0\n");
		goto fail;
	}

	if (res->len != physmem->len) {
		vcm_err("res->len (%i) != physmem->len (%i)\n",
			res->len, physmem->len);
		goto fail;
	}

	if (physmem->is_cont) {
		if (physmem->res == 0) {
			vcm_err("cont physmem->res is 0");
			goto fail;
		}
	} else {
		/* fail if no physmem */
		if (list_empty(&physmem->alloc_head.allocated)) {
			vcm_err("no allocated phys memory");
			goto fail;
		}
	}

	ret = vcm_no_assoc(res->vcm_id);
	if (ret == 1) {
		vcm_err("can't back un associated VCM\n");
		goto fail;
	}

	if (ret == -1) {
		vcm_err("vcm_no_assoc() ret -1\n");
		goto fail;
	}

	ret = vcm_all_activated(res->vcm_id);
	if (ret == 0) {
		vcm_err("can't back, not all associations are activated\n");
		goto fail_eagain;
	}

	if (ret == -1) {
		vcm_err("vcm_all_activated() ret -1\n");
		goto fail;
	}

	va = res->aligned_ptr;

	list_for_each_entry(chunk, &physmem->alloc_head.allocated,
			    allocated) {
		struct vcm *vcm = res->vcm_id;
		size_t chunk_size =  vcm_alloc_idx_to_size(chunk->size_idx);

		switch (vcm->type) {
		case VCM_DEVICE:
		{
#ifdef CONFIG_SMMU
			struct avcm *avcm;
			/* map all */
			list_for_each_entry(avcm, &vcm->assoc_head,
					    assoc_elm) {

				ret = vcm_process_chunk(avcm->dev_id, chunk->pa,
						      va, chunk_size, attr, 1);
				if (ret != 0) {
					vcm_err("vcm_back_chunk(%p, %p, %p,"
						" 0x%x, 0x%x)"
						" ret %i",
						(void *) avcm->dev_id,
						(void *) chunk->pa,
						(void *) va,
						(int) chunk_size, attr, ret);
					goto fail;
					/* TODO handle weird inter-map case */
				}
			}
			break;
#else
			vcm_err("No SMMU support - VCM_DEVICE not supported\n");
			goto fail;
#endif
		}

		case VCM_EXT_KERNEL:
		{
			unsigned int pages_in_chunk = chunk_size / PAGE_SIZE;
			unsigned long loc_va = va;
			unsigned long loc_pa = chunk->pa;

			const struct mem_type *mtype;

			/* TODO: get this based on MEMTYPE */
			mtype = get_mem_type(MT_DEVICE);
			if (!mtype) {
				vcm_err("mtype is 0\n");
				goto fail;
			}

			/* TODO: Map with the same chunk size */
			while (pages_in_chunk--) {
				ret = ioremap_page(loc_va,
						   loc_pa,
						   mtype);
				if (ret != 0) {
					vcm_err("ioremap_page(%p, %p, %p) ret"
						" %i", (void *) loc_va,
						(void *) loc_pa,
						(void *) mtype, ret);
					goto fail;
					/* TODO handle weird
					   inter-map case */
				}

				/* hack part 2 */
				/* we're changing the PT entry behind
				 * linux's back
				 */
				ret = cpu_set_attr(loc_va, PAGE_SIZE, attr);
				if (ret != 0) {
					vcm_err("cpu_set_attr(%p, %lu, %x)"
						"ret %i\n",
						(void *) loc_va, PAGE_SIZE,
						attr, ret);
					goto fail;
					/* TODO handle weird
					   inter-map case */
				}

				res->mapped = 1;

				loc_va += PAGE_SIZE;
				loc_pa += PAGE_SIZE;
			}

			flush_cache_vmap(va, loc_va);
			break;
		}
		case VCM_ONE_TO_ONE:
			va = chunk->pa;
			break;
		default:
			/* this should never happen */
			goto fail;
		}

		va += chunk_size;
		/* also add res to the allocated chunk list of refs */
	}

	/* note the reservation */
	res->physmem_id = physmem;

	spin_unlock_irqrestore(&vcmlock, flags);
	return 0;
fail_eagain:
	spin_unlock_irqrestore(&vcmlock, flags);
	return -EAGAIN;
fail:
	spin_unlock_irqrestore(&vcmlock, flags);
	return -EINVAL;
}


int vcm_unback(struct res *res)
{
	unsigned long flags;
	struct vcm *vcm;
	struct physmem *physmem;
	int ret;

	spin_lock_irqsave(&vcmlock, flags);

	if (!res)
		goto fail;

	vcm = res->vcm_id;
	if (!vcm) {
		vcm_err("NULL vcm\n");
		goto fail;
	}

	if (!res->physmem_id) {
		vcm_err("can't unback a non-backed reservation\n");
		goto fail;
	}

	physmem = res->physmem_id;
	if (!physmem) {
		vcm_err("physmem is NULL\n");
		goto fail;
	}

	if (list_empty(&physmem->alloc_head.allocated)) {
		vcm_err("physmem allocation is empty\n");
		goto fail;
	}

	ret = vcm_no_assoc(res->vcm_id);
	if (ret == 1) {
		vcm_err("can't unback a unassociated reservation\n");
		goto fail;
	}

	if (ret == -1) {
		vcm_err("vcm_no_assoc(%p) ret -1\n", (void *) res->vcm_id);
		goto fail;
	}

	ret = vcm_all_activated(res->vcm_id);
	if (ret == 0) {
		vcm_err("can't unback, not all associations are active\n");
		goto fail_eagain;
	}

	if (ret == -1) {
		vcm_err("vcm_all_activated(%p) ret -1\n", (void *) res->vcm_id);
		goto fail;
	}


	switch (vcm->type) {
	case VCM_EXT_KERNEL:
		if (!res->mapped) {
			vcm_err("can't unback an unmapped VCM_EXT_KERNEL"
				" VCM\n");
			goto fail;
		}

		/* vunmap free's vm_area */
		vunmap(res->vm_area->addr);
		res->vm_area = 0;

		res->mapped = 0;
		break;

	case VCM_DEVICE:
	{
#ifdef CONFIG_SMMU
		struct phys_chunk *chunk;
		size_t va = res->aligned_ptr;

		list_for_each_entry(chunk, &physmem->alloc_head.allocated,
				    allocated) {
			struct vcm *vcm = res->vcm_id;
			size_t chunk_size =
				vcm_alloc_idx_to_size(chunk->size_idx);
			struct avcm *avcm;

			/* un map all */
			list_for_each_entry(avcm, &vcm->assoc_head, assoc_elm) {
				ret = vcm_process_chunk(avcm->dev_id, 0, va,
							chunk_size, 0, 0);
				if (ret != 0) {
					vcm_err("vcm_unback_chunk(%p, %p, 0x%x)"
						" ret %i",
						(void *) avcm->dev_id,
						(void *) va,
						(int) chunk_size, ret);
					goto fail;
					/* TODO handle weird inter-unmap state*/
				}
			}
			va += chunk_size;
			/* may to a light unback, depending on the requested
			 * functionality
			 */
		}
#else
		vcm_err("No SMMU support - VCM_DEVICE memory not supported\n");
		goto fail;
#endif
		break;
	}

	case VCM_ONE_TO_ONE:
		break;
	default:
		/* this should never happen */
		goto fail;
	}

	/* clear the reservation */
	res->physmem_id = 0;

	spin_unlock_irqrestore(&vcmlock, flags);
	return 0;
fail_eagain:
	spin_unlock_irqrestore(&vcmlock, flags);
	return -EAGAIN;
fail:
	spin_unlock_irqrestore(&vcmlock, flags);
	return -EINVAL;
}


enum memtarget_t vcm_get_memtype_of_res(struct res *res)
{
	return VCM_INVALID;
}

static int vcm_free_max_munch_cont(struct phys_chunk *head)
{
	struct phys_chunk *chunk, *tmp;

	if (!head)
		return -1;

	list_for_each_entry_safe(chunk, tmp, &head->allocated,
				 allocated) {
		list_del_init(&chunk->allocated);
	}

	return 0;
}

static int vcm_alloc_max_munch_cont(size_t start_addr, size_t len,
				    struct phys_chunk *head)
{
	/* this function should always succeed, since it
	   parallels a VCM */

	int i, j;

	if (!head) {
		vcm_err("head is NULL in continuous map.\n");
		goto fail;
	}

	if (start_addr < __pa(bootmem_cont)) {
		vcm_err("phys start addr (%p) < base (%p)\n",
			(void *) start_addr, (void *) __pa(bootmem_cont));
		goto fail;
	}

	if ((start_addr + len) >= (__pa(bootmem_cont) + CONT_SZ)) {
		vcm_err("requested region (%p + %i) > "
			" available region (%p + %i)",
			(void *) start_addr, (int) len,
			(void *) __pa(bootmem_cont), CONT_SZ);
		goto fail;
	}

	i = (start_addr - __pa(bootmem_cont))/SZ_4K;

	for (j = 0; j < ARRAY_SIZE(chunk_sizes); ++j) {
		while (len/chunk_sizes[j]) {
			if (!list_empty(&cont_phys_chunk[i].allocated)) {
				vcm_err("chunk %i ( addr %p) already mapped\n",
					i, (void *) (start_addr +
						     (i*chunk_sizes[j])));
				goto fail_free;
			}
			list_add_tail(&cont_phys_chunk[i].allocated,
				      &head->allocated);
			cont_phys_chunk[i].size_idx = j;

			len -= chunk_sizes[j];
			i += chunk_sizes[j]/SZ_4K;
		}
	}

	if (len % SZ_4K) {
		if (!list_empty(&cont_phys_chunk[i].allocated)) {
			vcm_err("chunk %i (addr %p) already mapped\n",
				i, (void *) (start_addr + (i*SZ_4K)));
			goto fail_free;
		}
		len -= SZ_4K;
		list_add_tail(&cont_phys_chunk[i].allocated,
			      &head->allocated);

		i++;
	}

	return i;

fail_free:
	{
		struct phys_chunk *chunk, *tmp;
		/* just remove from list, if we're double alloc'ing
		   we don't want to stamp on the other guy */
		list_for_each_entry_safe(chunk, tmp, &head->allocated,
					 allocated) {
			list_del(&chunk->allocated);
		}
	}
fail:
	return 0;
}

struct physmem *vcm_phys_alloc(enum memtype_t memtype, size_t len,
			       uint32_t attr)
{
	unsigned long flags;
	int ret;
	struct physmem *physmem = NULL;
	int blocks_allocated;

	spin_lock_irqsave(&vcmlock, flags);

	physmem = kzalloc(sizeof(*physmem), GFP_KERNEL);
	if (!physmem) {
		vcm_err("physmem is NULL\n");
		goto fail;
	}

	physmem->memtype = memtype;
	physmem->len = len;
	physmem->attr = attr;

	INIT_LIST_HEAD(&physmem->alloc_head.allocated);

	if (attr & VCM_PHYS_CONT) {
		if (!cont_vcm_id) {
			vcm_err("cont_vcm_id is NULL\n");
			goto fail2;
		}

		physmem->is_cont = 1;

		/* TODO: get attributes */
		physmem->res = __vcm_reserve(cont_vcm_id, len, 0);
		if (physmem->res == 0) {
			vcm_err("contiguous space allocation failed\n");
			goto fail2;
		}

		/* if we're here we know we have memory, create
		   the shadow physmem links*/
		blocks_allocated =
			vcm_alloc_max_munch_cont(
				vcm_get_dev_addr(physmem->res),
				len,
				&physmem->alloc_head);

		if (blocks_allocated == 0) {
			vcm_err("shadow physmem allocation failed\n");
			goto fail3;
		}
	} else {
		blocks_allocated = vcm_alloc_max_munch(len,
						       &physmem->alloc_head);
		if (blocks_allocated == 0) {
			vcm_err("physical allocation failed:"
				" vcm_alloc_max_munch(%i, %p) ret 0\n",
				len, &physmem->alloc_head);
			goto fail2;
		}
	}

	spin_unlock_irqrestore(&vcmlock, flags);
	return physmem;

fail3:
	ret = __vcm_unreserve(physmem->res);
	if (ret != 0) {
		vcm_err("vcm_unreserve(%p) ret %i during cleanup",
			(void *) physmem->res, ret);
		spin_unlock_irqrestore(&vcmlock, flags);
		return 0;
	}
fail2:
	kfree(physmem);
fail:
	spin_unlock_irqrestore(&vcmlock, flags);
	return 0;
}


int vcm_phys_free(struct physmem *physmem)
{
	unsigned long flags;
	int ret;

	spin_lock_irqsave(&vcmlock, flags);

	if (!physmem) {
		vcm_err("physmem is NULL\n");
		goto fail;
	}

	if (physmem->is_cont) {
		if (physmem->res == 0) {
			vcm_err("contiguous reservation is NULL\n");
			goto fail;
		}

		ret = vcm_free_max_munch_cont(&physmem->alloc_head);
		if (ret != 0) {
			vcm_err("failed to free physical blocks:"
				" vcm_free_max_munch_cont(%p) ret %i\n",
				(void *) &physmem->alloc_head, ret);
			goto fail;
		}

		ret = __vcm_unreserve(physmem->res);
		if (ret != 0) {
			vcm_err("failed to free virtual blocks:"
				" vcm_unreserve(%p) ret %i\n",
				(void *) physmem->res, ret);
			goto fail;
		}

	} else {

		ret = vcm_alloc_free_blocks(&physmem->alloc_head);
		if (ret != 0) {
			vcm_err("failed to free physical blocks:"
				" vcm_alloc_free_blocks(%p) ret %i\n",
				(void *) &physmem->alloc_head, ret);
			goto fail;
		}
	}

	memset(physmem, 0, sizeof(*physmem));

	kfree(physmem);

	spin_unlock_irqrestore(&vcmlock, flags);
	return 0;

fail:
	spin_unlock_irqrestore(&vcmlock, flags);
	return -EINVAL;
}


struct avcm *vcm_assoc(struct vcm *vcm, size_t dev_id, uint32_t attr)
{
	unsigned long flags;
	struct avcm *avcm = NULL;

	spin_lock_irqsave(&vcmlock, flags);

	if (!vcm) {
		vcm_err("vcm is NULL\n");
		goto fail;
	}

	if (!dev_id) {
		vcm_err("dev_id is NULL\n");
		goto fail;
	}

	if (vcm->type == VCM_EXT_KERNEL && !list_empty(&vcm->assoc_head)) {
		vcm_err("only one device may be assocoated with a"
			" VCM_EXT_KERNEL\n");
		goto fail;
	}

	avcm = kzalloc(sizeof(*avcm), GFP_KERNEL);
	if (!avcm) {
		vcm_err("kzalloc(%i, GFP_KERNEL) ret NULL\n", sizeof(*avcm));
		goto fail;
	}

	avcm->dev_id = dev_id;

	avcm->vcm_id = vcm;
	avcm->attr = attr;
	avcm->is_active = 0;

	INIT_LIST_HEAD(&avcm->assoc_elm);
	list_add(&avcm->assoc_elm, &vcm->assoc_head);

	spin_unlock_irqrestore(&vcmlock, flags);
	return avcm;

fail:
	spin_unlock_irqrestore(&vcmlock, flags);
	return 0;
}


int vcm_deassoc(struct avcm *avcm)
{
	unsigned long flags;

	spin_lock_irqsave(&vcmlock, flags);

	if (!avcm) {
		vcm_err("avcm is NULL\n");
		goto fail;
	}

	if (list_empty(&avcm->assoc_elm)) {
		vcm_err("nothing to deassociate\n");
		goto fail;
	}

	if (avcm->is_active) {
		vcm_err("association still activated\n");
		goto fail_busy;
	}

	list_del(&avcm->assoc_elm);

	memset(avcm, 0, sizeof(*avcm));

	kfree(avcm);
	spin_unlock_irqrestore(&vcmlock, flags);
	return 0;
fail_busy:
	spin_unlock_irqrestore(&vcmlock, flags);
	return -EBUSY;
fail:
	spin_unlock_irqrestore(&vcmlock, flags);
	return -EINVAL;
}


int vcm_set_assoc_attr(struct avcm *avcm, uint32_t attr)
{
	return 0;
}


uint32_t vcm_get_assoc_attr(struct avcm *avcm)
{
	return 0;
}


int vcm_activate(struct avcm *avcm)
{
	unsigned long flags;
	struct vcm *vcm;

	spin_lock_irqsave(&vcmlock, flags);

	if (!avcm) {
		vcm_err("avcm is NULL\n");
		goto fail;
	}

	vcm = avcm->vcm_id;
	if (!vcm) {
		vcm_err("NULL vcm\n");
		goto fail;
	}

	if (!avcm->dev_id) {
		vcm_err("cannot activate without a device\n");
		goto fail_nodev;
	}

	if (avcm->is_active) {
		vcm_err("double activate\n");
		goto fail_busy;
	}

	if (vcm->type == VCM_DEVICE) {
#ifdef CONFIG_SMMU
		int ret = smmu_is_active((struct smmu_dev *) avcm->dev_id);
		if (ret == -1) {
			vcm_err("smmu_is_active(%p) ret -1\n",
				(void *) avcm->dev_id);
			goto fail_dev;
		}

		if (ret == 1) {
			vcm_err("SMMU is already active\n");
			goto fail_busy;
		}

		/* TODO, pmem check */
		ret = smmu_activate((struct smmu_dev *) avcm->dev_id);
		if (ret != 0) {
			vcm_err("smmu_activate(%p) ret %i"
				" SMMU failed to activate\n",
				(void *) avcm->dev_id, ret);
			goto fail_dev;
		}
#else
		vcm_err("No SMMU support - cannot activate/deactivate\n");
		goto fail_nodev;
#endif
	}

	avcm->is_active = 1;
	spin_unlock_irqrestore(&vcmlock, flags);
	return 0;

#ifdef CONFIG_SMMU
fail_dev:
	spin_unlock_irqrestore(&vcmlock, flags);
	return -1;
#endif
fail_busy:
	spin_unlock_irqrestore(&vcmlock, flags);
	return -EBUSY;
fail_nodev:
	spin_unlock_irqrestore(&vcmlock, flags);
	return -ENODEV;
fail:
	spin_unlock_irqrestore(&vcmlock, flags);
	return -EINVAL;
}


int vcm_deactivate(struct avcm *avcm)
{
	unsigned long flags;
	struct vcm *vcm;

	spin_lock_irqsave(&vcmlock, flags);

	if (!avcm)
		goto fail;

	vcm = avcm->vcm_id;
	if (!vcm) {
		vcm_err("NULL vcm\n");
		goto fail;
	}

	if (!avcm->dev_id) {
		vcm_err("cannot deactivate without a device\n");
		goto fail;
	}

	if (!avcm->is_active) {
		vcm_err("double deactivate\n");
		goto fail_nobusy;
	}

	if (vcm->type == VCM_DEVICE) {
#ifdef CONFIG_SMMU
		int ret = smmu_is_active((struct smmu_dev *) avcm->dev_id);
		if (ret == -1) {
			vcm_err("smmu_is_active(%p) ret %i\n",
				(void *) avcm->dev_id, ret);
			goto fail_dev;
		}

		if (ret == 0) {
			vcm_err("double SMMU deactivation\n");
			goto fail_nobusy;
		}

		/* TODO, pmem check */
		ret = smmu_deactivate((struct smmu_dev *) avcm->dev_id);
		if (ret != 0) {
			vcm_err("smmu_deactivate(%p) ret %i\n",
				(void *) avcm->dev_id, ret);
			goto fail_dev;
		}
#else
		vcm_err("No SMMU support - cannot activate/deactivate\n");
		goto fail;
#endif
	}

	avcm->is_active = 0;
	spin_unlock_irqrestore(&vcmlock, flags);
	return 0;
#ifdef CONFIG_SMMU
fail_dev:
	spin_unlock_irqrestore(&vcmlock, flags);
	return -1;
#endif
fail_nobusy:
	spin_unlock_irqrestore(&vcmlock, flags);
	return -ENOENT;
fail:
	spin_unlock_irqrestore(&vcmlock, flags);
	return -EINVAL;
}

struct bound *vcm_create_bound(struct vcm *vcm, size_t len)
{
	return 0;
}


int vcm_free_bound(struct bound *bound)
{
	return -1;
}


struct res *vcm_reserve_from_bound(struct bound *bound, size_t len,
				   uint32_t attr)
{
	return 0;
}


size_t vcm_get_bound_start_addr(struct bound *bound)
{
	return 0;
}


size_t vcm_get_bound_len(struct bound *bound)
{
	return 0;
}


struct physmem *vcm_map_phys_addr(size_t phys, size_t len)
{
	return 0;
}


size_t vcm_get_next_phys_addr(struct physmem *physmem, size_t phys, size_t *len)
{
	return 0;
}


size_t vcm_get_dev_addr(struct res *res)
{
	if (!res) {
		vcm_err("res is NULL\n");
		return 0;
	}

	return res->aligned_ptr;
}


struct res *vcm_get_res(size_t dev_addr, struct vcm *vcm)
{
	return 0;
}


size_t vcm_translate(size_t src_dev, struct vcm *src_vcm, struct vcm *dst_vcm)
{
	return 0;
}


size_t vcm_get_phys_num_res(size_t phys)
{
	return 0;
}


struct res *vcm_get_next_phys_res(size_t phys, struct res *res_id, size_t *len)
{
	return 0;
}


size_t vcm_get_pgtbl_pa(struct vcm *vcm)
{
	return 0;
}


/* No lock needed, smmu_translate has its own lock */
size_t vcm_dev_addr_to_phys_addr(size_t dev_id, size_t dev_addr)
{
#ifdef CONFIG_SMMU
	int ret;
	ret = smmu_translate((struct smmu_dev *) dev_id, dev_addr);
	if (ret == -1)
		vcm_err("smmu_translate(%p, %p) ret %i\n",
			(void *) dev_id, (void *) dev_addr, ret);

	return ret;
#else
	vcm_err("No support for SMMU - manual translation not supported\n");
	return -1;
#endif
}


/* No lock needed, bootmem_cont never changes after  */
size_t vcm_get_cont_memtype_pa(enum memtype_t memtype)
{
	if (memtype != VCM_MEMTYPE_0) {
		vcm_err("memtype != VCM_MEMTYPE_0\n");
		goto fail;
	}

	if (!bootmem_cont) {
		vcm_err("bootmem_cont 0\n");
		goto fail;
	}

	return (size_t) __pa(bootmem_cont);
fail:
	return 0;
}


/* No lock needed, constant */
size_t vcm_get_cont_memtype_len(enum memtype_t memtype)
{
	if (memtype != VCM_MEMTYPE_0) {
		vcm_err("memtype != VCM_MEMTYPE_0\n");
		return 0;
	}

	return CONT_SZ;
}

int vcm_hook(size_t dev_id, vcm_handler handler, void *data)
{
#ifdef CONFIG_SMMU
	int ret;

	ret = smmu_hook_irpt((struct smmu_dev *) dev_id, handler, data);
	if (ret != 0)
		vcm_err("smmu_hook_irpt(%p, %p, %p) ret %i\n", (void *) dev_id,
			(void *) handler, (void *) data, ret);

	return ret;
#else
	vcm_err("No support for SMMU - interrupts not supported\n");
	return -1;
#endif
}


size_t vcm_hw_ver(size_t dev_id)
{
	return 0;
}


static int vcm_cont_phys_chunk_init(void)
{
	int i;
	int cont_pa;

	if (!cont_phys_chunk) {
		vcm_err("cont_phys_chunk 0\n");
		goto fail;
	}

	if (!bootmem_cont) {
		vcm_err("bootmem_cont 0\n");
		goto fail;
	}

	cont_pa = (int) __pa(bootmem_cont);

	for (i = 0; i < CONT_SZ/PAGE_SIZE; ++i) {
		cont_phys_chunk[i].pa = (int) cont_pa; cont_pa += PAGE_SIZE;
		cont_phys_chunk[i].size_idx = IDX_4K;
		INIT_LIST_HEAD(&cont_phys_chunk[i].allocated);
	}

	return 0;

fail:
	return -1;
}


int vcm_sys_init(void)
{
	int ret;
	printk(KERN_INFO "VCM Initialization\n");
	if (!bootmem) {
		vcm_err("bootmem is 0\n");
		ret = -1;
		goto fail;
	}

	if (!bootmem_cont) {
		vcm_err("bootmem_cont is 0\n");
		ret = -1;
		goto fail;
	}

	ret = vcm_setup_tex_classes();
	if (ret != 0) {
		printk(KERN_INFO "Could not determine TEX attribute mapping\n");
		ret = -1;
		goto fail;
	}


	ret = vcm_alloc_init(__pa(bootmem));
	if (ret != 0) {
		vcm_err("vcm_alloc_init(%p) ret %i\n", (void *) __pa(bootmem),
			ret);
		ret = -1;
		goto fail;
	}

	cont_phys_chunk = kzalloc(sizeof(*cont_phys_chunk)*(CONT_SZ/PAGE_SIZE),
				  GFP_KERNEL);
	if (!cont_phys_chunk) {
		vcm_err("kzalloc(%lu, GFP_KERNEL) ret 0",
			sizeof(*cont_phys_chunk)*(CONT_SZ/PAGE_SIZE));
		goto fail_free;
	}

	/* the address and size will hit our special case unless we
	   pass an override */
	cont_vcm_id = vcm_create_flagged(0, __pa(bootmem_cont), CONT_SZ);
	if (cont_vcm_id == 0) {
		vcm_err("vcm_create_flagged(0, %p, %i) ret 0\n",
			(void *) __pa(bootmem_cont), CONT_SZ);
		ret = -1;
		goto fail_free2;
	}

	ret = vcm_cont_phys_chunk_init();
	if (ret != 0) {
		vcm_err("vcm_cont_phys_chunk_init() ret %i\n", ret);
		goto fail_free3;
	}

	printk(KERN_INFO "VCM Initialization OK\n");
	return 0;

fail_free3:
	ret = __vcm_free(cont_vcm_id);
	if (ret != 0) {
		vcm_err("vcm_free(%p) ret %i during failure path\n",
			(void *) cont_vcm_id, ret);
		return -1;
	}

fail_free2:
	kfree(cont_phys_chunk);
	cont_phys_chunk = 0;

fail_free:
	ret = vcm_alloc_destroy();
	if (ret != 0)
		vcm_err("vcm_alloc_destroy() ret %i during failure path\n",
			ret);

	ret = -1;
fail:
	return ret;
}


int vcm_sys_destroy(void)
{
	int ret = 0;

	if (!cont_phys_chunk) {
		vcm_err("cont_phys_chunk is 0\n");
		return -1;
	}

	if (!cont_vcm_id) {
		vcm_err("cont_vcm_id is 0\n");
		return -1;
	}

	ret = __vcm_free(cont_vcm_id);
	if (ret != 0) {
		vcm_err("vcm_free(%p) ret %i\n", (void *) cont_vcm_id, ret);
		return -1;
	}

	cont_vcm_id = 0;

	kfree(cont_phys_chunk);
	cont_phys_chunk = 0;

	ret = vcm_alloc_destroy();
	if (ret != 0) {
		vcm_err("vcm_alloc_destroy() ret %i\n", ret);
		return -1;
	}

	return ret;
}

int vcm_init(void)
{
	int ret;

	bootmem = __alloc_bootmem(BOOTMEM_SZ, BOOTMEM_ALIGN, 0);
	if (!bootmem) {
		vcm_err("segregated block pool alloc failed:"
			" __alloc_bootmem(%i, %i, 0)\n",
			BOOTMEM_SZ, BOOTMEM_ALIGN);
		goto fail;
	}

	bootmem_cont = __alloc_bootmem(CONT_SZ, CONT_ALIGN, 0);
	if (!bootmem_cont) {
		vcm_err("contiguous pool alloc failed:"
			" __alloc_bootmem(%i, %i, 0)\n",
			CONT_SZ, CONT_ALIGN);
		goto fail_free;
	}

	ret = vcm_sys_init();
	if (ret != 0) {
		vcm_err("vcm_sys_init() ret %i\n", ret);
		goto fail_free2;
	}

	return 0;

fail_free2:
	free_bootmem(__pa(bootmem_cont), CONT_SZ);
fail_free:
	free_bootmem(__pa(bootmem), BOOTMEM_SZ);
fail:
	return -1;
};

/* Useful for testing, and if VCM is ever unloaded */
void vcm_exit(void)
{
	int ret;

	if (!bootmem_cont) {
		vcm_err("bootmem_cont is 0\n");
		goto fail;
	}

	if (!bootmem) {
		vcm_err("bootmem is 0\n");
		goto fail;
	}

	ret = vcm_sys_destroy();
	if (ret != 0) {
		vcm_err("vcm_sys_destroy() ret %i\n", ret);
		goto fail;
	}

	free_bootmem(__pa(bootmem_cont), CONT_SZ);
	free_bootmem(__pa(bootmem), BOOTMEM_SZ);
fail:
	return;
}
early_initcall(vcm_init);
module_exit(vcm_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Zach Pfeffer <zpfeffer@codeaurora.org>");
