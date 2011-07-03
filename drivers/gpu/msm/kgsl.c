/* Copyright (c) 2008-2010, Code Aurora Forum. All rights reserved.
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
 *
 */
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <mach/clk.h>
#include <mach/dal_axi.h>
#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/android_pmem.h>
#include <linux/pm_qos_params.h>
#include <linux/highmem.h>
#include <linux/vmalloc.h>
#include <asm/cacheflush.h>
#include <linux/notifier.h>

#include <linux/delay.h>
#include <asm/atomic.h>
#include <mach/internal_power_rail.h>

#include "kgsl.h"
#include "kgsl_yamato.h"
#include "kgsl_g12.h"
#include "kgsl_cmdstream.h"

#include "kgsl_log.h"
#include "kgsl_drm.h"

#define KGSL_MAX_PRESERVED_BUFFERS		10
#define KGSL_MAX_SIZE_OF_PRESERVED_BUFFER	0x10000

static void kgsl_put_phys_file(struct file *file);

static int kgsl_runpending(struct kgsl_device *device)
{
	if (device->flags & KGSL_FLAGS_INITIALIZED) {
		kgsl_cmdstream_memqueue_drain(device);
	}

	return KGSL_SUCCESS;
}

static void kgsl_runpending_all(void)
{
	struct kgsl_device *device;
	int i;

	for (i = 0; i < KGSL_DEVICE_MAX; i++) {
		device = kgsl_driver.devp[i];
		if (device != NULL)
			kgsl_runpending(device);
	}
	return;
}

#ifdef CONFIG_MSM_KGSL_MMU
static long kgsl_cache_range_op(unsigned long addr, int size,
					unsigned int flags)
{
#ifdef CONFIG_OUTER_CACHE
	unsigned long end;
#endif
	BUG_ON(addr & (KGSL_PAGESIZE - 1));
	BUG_ON(size & (KGSL_PAGESIZE - 1));

	if (flags & KGSL_MEMFLAGS_CACHE_FLUSH)
		dmac_flush_range((const void *)addr,
				(const void *)(addr + size));
	else
		if (flags & KGSL_MEMFLAGS_CACHE_CLEAN)
			dmac_clean_range((const void *)addr,
					(const void *)(addr + size));
		else if (flags & KGSL_MEMFLAGS_CACHE_INV)
			dmac_inv_range((const void *)addr,
					(const void *)(addr + size));

#ifdef CONFIG_OUTER_CACHE
	for (end = addr; end < (addr + size); end += KGSL_PAGESIZE) {
		unsigned long physaddr;
		if (flags & KGSL_MEMFLAGS_VMALLOC_MEM)
			physaddr = page_to_phys(vmalloc_to_page((void *) end));
		else
			if (flags & KGSL_MEMFLAGS_HOSTADDR) {
				physaddr = kgsl_virtaddr_to_physaddr(end);
				if (!physaddr) {
					KGSL_MEM_ERR
					("Unable to find physaddr for "
					"address: %x\n", (unsigned int)end);
					return -EINVAL;
				}
			} else
				return -EINVAL;

		if (flags & KGSL_MEMFLAGS_CACHE_FLUSH)
			outer_flush_range(physaddr, physaddr + KGSL_PAGESIZE);
		else
			if (flags & KGSL_MEMFLAGS_CACHE_CLEAN)
				outer_clean_range(physaddr,
					physaddr + KGSL_PAGESIZE);
			else if (flags & KGSL_MEMFLAGS_CACHE_INV)
				outer_inv_range(physaddr,
					physaddr + KGSL_PAGESIZE);
	}
#endif
	return 0;
}

static long kgsl_clean_cache_all(struct kgsl_file_private *private)
{
	int result = 0;
	struct kgsl_mem_entry *entry = NULL;

	kgsl_runpending_all();

	list_for_each_entry(entry, &private->mem_list, list) {
		if (KGSL_MEMFLAGS_CACHE_MASK & entry->memdesc.priv) {
			result =
			    kgsl_cache_range_op((unsigned long)entry->
						   memdesc.hostptr,
						   entry->memdesc.size,
							entry->memdesc.priv);
			if (result)
				goto done;
		}
	}
done:
	return result;
}
#endif /*CONFIG_MSM_KGSL_MMU*/

/*this is used for logging, so that we can call the dev_printk
 functions without export struct kgsl_driver everywhere*/
struct device *kgsl_driver_getdevnode(void)
{
	BUG_ON(kgsl_driver.pdev == NULL);
	return &kgsl_driver.pdev->dev;
}

struct kgsl_device *kgsl_get_device(int dev_idx)
{
	BUG_ON(dev_idx >= KGSL_DEVICE_MAX || dev_idx < KGSL_DEVICE_YAMATO);
	return kgsl_driver.devp[dev_idx];
}

int kgsl_register_ts_notifier(struct kgsl_device *device,
			      struct notifier_block *nb)
{
	BUG_ON(device == NULL);
	return atomic_notifier_chain_register(&device->ts_notifier_list,
					      nb);
}

int kgsl_unregister_ts_notifier(struct kgsl_device *device,
				struct notifier_block *nb)
{
	BUG_ON(device == NULL);
	return atomic_notifier_chain_unregister(&device->ts_notifier_list,
						nb);
}

int kgsl_regread(struct kgsl_device *device, unsigned int offsetwords,
			unsigned int *value)
{
	int status = -ENXIO;
	if (device->ftbl.device_regread)
		status = device->ftbl.device_regread(device, offsetwords,
					value);

	return status;
}

int kgsl_regwrite(struct kgsl_device *device, unsigned int offsetwords,
			unsigned int value)
{
	int status = -ENXIO;
	if (device->ftbl.device_regwrite)
		status = device->ftbl.device_regwrite(device, offsetwords,
					value);

	return status;
}

int kgsl_setstate(struct kgsl_device *device, uint32_t flags)
{
	int status = -ENXIO;

	if (flags && device->ftbl.device_setstate) {
		status = device->ftbl.device_setstate(device, flags);
		device->mmu.tlb_flags &= ~flags;
	} else
		status = 0;

	return status;
}

int kgsl_idle(struct kgsl_device *device, unsigned int timeout)
{
	int status = -ENXIO;

	if (device->ftbl.device_idle)
		status = device->ftbl.device_idle(device, timeout);

	return status;
}


int kgsl_pwrctrl(unsigned int pwrflag)
{
	switch (pwrflag) {
	case KGSL_PWRFLAGS_YAMATO_CLK_OFF:
		if (kgsl_driver.power_flags & KGSL_PWRFLAGS_YAMATO_CLK_ON) {
			if (kgsl_driver.yamato_grp_pclk)
				clk_disable(kgsl_driver.yamato_grp_pclk);

			clk_disable(kgsl_driver.yamato_grp_clk);
			if (kgsl_driver.imem_clk != NULL)
				clk_disable(kgsl_driver.imem_clk);
			if (kgsl_driver.clk_freq[KGSL_3D_MIN_FREQ])
				clk_set_min_rate(kgsl_driver.yamato_grp_src_clk,
					kgsl_driver.clk_freq[KGSL_3D_MIN_FREQ]);
			if (kgsl_driver.clk_freq[KGSL_AXI_HIGH_3D])
				pm_qos_update_requirement(
					PM_QOS_SYSTEM_BUS_FREQ,
					"kgsl_3d", PM_QOS_DEFAULT_VALUE);
			kgsl_driver.power_flags &=
					~(KGSL_PWRFLAGS_YAMATO_CLK_ON);
			kgsl_driver.power_flags |= KGSL_PWRFLAGS_YAMATO_CLK_OFF;
		}
		return KGSL_SUCCESS;
	case KGSL_PWRFLAGS_YAMATO_CLK_ON:
		if (kgsl_driver.power_flags & KGSL_PWRFLAGS_YAMATO_CLK_OFF) {
			if (kgsl_driver.clk_freq[KGSL_AXI_HIGH_3D])
				pm_qos_update_requirement(
					PM_QOS_SYSTEM_BUS_FREQ, "kgsl_3d",
					kgsl_driver.clk_freq[KGSL_AXI_HIGH_3D]);
			if (kgsl_driver.clk_freq[KGSL_3D_MAX_FREQ])
				clk_set_min_rate(kgsl_driver.yamato_grp_src_clk,
					kgsl_driver.clk_freq[KGSL_3D_MAX_FREQ]);
			if (kgsl_driver.yamato_grp_pclk)
				clk_enable(kgsl_driver.yamato_grp_pclk);
			clk_enable(kgsl_driver.yamato_grp_clk);
			if (kgsl_driver.imem_clk != NULL)
				clk_enable(kgsl_driver.imem_clk);

			kgsl_driver.power_flags &=
				~(KGSL_PWRFLAGS_YAMATO_CLK_OFF);
			kgsl_driver.power_flags |= KGSL_PWRFLAGS_YAMATO_CLK_ON;
		}
		return KGSL_SUCCESS;
	case KGSL_PWRFLAGS_G12_CLK_OFF:
		if (kgsl_driver.power_flags & KGSL_PWRFLAGS_G12_CLK_ON) {
			if (kgsl_driver.g12_grp_pclk)
				clk_disable(kgsl_driver.g12_grp_pclk);
			if (kgsl_driver.g12_grp_clk != NULL) {
				clk_disable(kgsl_driver.g12_grp_clk);
				if (kgsl_driver.clk_freq[KGSL_2D_MIN_FREQ])
					clk_set_min_rate(
					kgsl_driver.g12_grp_clk,
					kgsl_driver.clk_freq[KGSL_2D_MIN_FREQ]);
			}
			if (kgsl_driver.clk_freq[KGSL_AXI_HIGH_2D])
				pm_qos_update_requirement(
					PM_QOS_SYSTEM_BUS_FREQ,
					"kgsl_2d", PM_QOS_DEFAULT_VALUE);
			kgsl_driver.power_flags &= ~(KGSL_PWRFLAGS_G12_CLK_ON);
			kgsl_driver.power_flags |= KGSL_PWRFLAGS_G12_CLK_OFF;
		}
		return KGSL_SUCCESS;
	case KGSL_PWRFLAGS_G12_CLK_ON:
		if (kgsl_driver.power_flags & KGSL_PWRFLAGS_G12_CLK_OFF) {
			if (kgsl_driver.clk_freq[KGSL_AXI_HIGH_2D])
				pm_qos_update_requirement(
					PM_QOS_SYSTEM_BUS_FREQ, "kgsl_2d",
					kgsl_driver.clk_freq[KGSL_AXI_HIGH_2D]);
			if (kgsl_driver.g12_grp_pclk)
				clk_enable(kgsl_driver.g12_grp_pclk);
			if (kgsl_driver.g12_grp_clk != NULL) {
				if (kgsl_driver.clk_freq[KGSL_2D_MAX_FREQ])
					clk_set_min_rate(
					kgsl_driver.g12_grp_clk,
					kgsl_driver.clk_freq[KGSL_2D_MAX_FREQ]);
				clk_enable(kgsl_driver.g12_grp_clk);
			}

			kgsl_driver.power_flags &= ~(KGSL_PWRFLAGS_G12_CLK_OFF);
			kgsl_driver.power_flags |= KGSL_PWRFLAGS_G12_CLK_ON;
		}
		return KGSL_SUCCESS;
	case KGSL_PWRFLAGS_YAMATO_POWER_OFF:
		if (kgsl_driver.power_flags & KGSL_PWRFLAGS_YAMATO_POWER_ON) {
			internal_pwr_rail_ctl(PWR_RAIL_GRP_CLK, KGSL_FALSE);
			internal_pwr_rail_mode(PWR_RAIL_GRP_CLK,
					PWR_RAIL_CTL_AUTO);
			kgsl_driver.power_flags &=
					~(KGSL_PWRFLAGS_YAMATO_POWER_ON);
			kgsl_driver.power_flags |=
					KGSL_PWRFLAGS_YAMATO_POWER_OFF;
		}
		return KGSL_SUCCESS;
	case KGSL_PWRFLAGS_YAMATO_POWER_ON:
		if (kgsl_driver.power_flags & KGSL_PWRFLAGS_YAMATO_POWER_OFF) {
			internal_pwr_rail_mode(PWR_RAIL_GRP_CLK,
					PWR_RAIL_CTL_MANUAL);
			internal_pwr_rail_ctl(PWR_RAIL_GRP_CLK, KGSL_TRUE);
			kgsl_driver.power_flags &=
					~(KGSL_PWRFLAGS_YAMATO_POWER_OFF);
			kgsl_driver.power_flags |=
					KGSL_PWRFLAGS_YAMATO_POWER_ON;
		}
		return KGSL_SUCCESS;
	case KGSL_PWRFLAGS_G12_POWER_OFF:
		if (kgsl_driver.power_flags & KGSL_PWRFLAGS_G12_POWER_ON) {
			internal_pwr_rail_ctl(PWR_RAIL_GRP_2D_CLK, KGSL_FALSE);
			internal_pwr_rail_mode(PWR_RAIL_GRP_2D_CLK,
					PWR_RAIL_CTL_AUTO);
			kgsl_driver.power_flags &=
					~(KGSL_PWRFLAGS_G12_POWER_ON);
			kgsl_driver.power_flags |= KGSL_PWRFLAGS_G12_POWER_OFF;
		}
		return KGSL_SUCCESS;
	case KGSL_PWRFLAGS_G12_POWER_ON:
		if (kgsl_driver.power_flags & KGSL_PWRFLAGS_G12_POWER_OFF) {
			internal_pwr_rail_mode(PWR_RAIL_GRP_2D_CLK,
					PWR_RAIL_CTL_MANUAL);
			internal_pwr_rail_ctl(PWR_RAIL_GRP_2D_CLK, KGSL_TRUE);
			kgsl_driver.power_flags &=
					~(KGSL_PWRFLAGS_G12_POWER_OFF);
			kgsl_driver.power_flags |= KGSL_PWRFLAGS_G12_POWER_ON;
		}
		return KGSL_SUCCESS;
	case KGSL_PWRFLAGS_YAMATO_IRQ_ON:
		if (kgsl_driver.power_flags & KGSL_PWRFLAGS_YAMATO_IRQ_OFF) {
			enable_irq(kgsl_driver.yamato_interrupt_num);
			kgsl_driver.power_flags &=
				~(KGSL_PWRFLAGS_YAMATO_IRQ_OFF);
			kgsl_driver.power_flags |= KGSL_PWRFLAGS_YAMATO_IRQ_ON;
		}

		return KGSL_SUCCESS;
	case KGSL_PWRFLAGS_YAMATO_IRQ_OFF:
		if (kgsl_driver.power_flags & KGSL_PWRFLAGS_YAMATO_IRQ_ON) {
			disable_irq(kgsl_driver.yamato_interrupt_num);
			kgsl_driver.power_flags &=
				~(KGSL_PWRFLAGS_YAMATO_IRQ_ON);
			kgsl_driver.power_flags |= KGSL_PWRFLAGS_YAMATO_IRQ_OFF;
		}
		return KGSL_SUCCESS;
	case KGSL_PWRFLAGS_G12_IRQ_ON:
		if (kgsl_driver.power_flags & KGSL_PWRFLAGS_G12_IRQ_OFF) {
			enable_irq(kgsl_driver.g12_interrupt_num);
			kgsl_driver.power_flags &= ~(KGSL_PWRFLAGS_G12_IRQ_OFF);
			kgsl_driver.power_flags |= KGSL_PWRFLAGS_G12_IRQ_ON;
		}

		return KGSL_SUCCESS;
	case KGSL_PWRFLAGS_G12_IRQ_OFF:
		if (kgsl_driver.power_flags & KGSL_PWRFLAGS_G12_IRQ_ON) {
			disable_irq(kgsl_driver.g12_interrupt_num);
			kgsl_driver.power_flags &= ~(KGSL_PWRFLAGS_G12_IRQ_ON);
			kgsl_driver.power_flags |= KGSL_PWRFLAGS_G12_IRQ_OFF;
		}
		return KGSL_SUCCESS;
	default:
		return KGSL_FAILURE;
	}
}

/*Suspend function*/
static int kgsl_suspend(struct platform_device *dev, pm_message_t state)
{
	int i;
	struct kgsl_device *device;

	mutex_lock(&kgsl_driver.mutex);
	if (kgsl_driver.power_flags != 0) {
		for (i = 0; i < KGSL_DEVICE_MAX; i++) {
			device = kgsl_driver.devp[i];
			if (device && device->hwaccess_blocked == KGSL_FALSE)
				device->ftbl.device_suspend(device);
		}

		kgsl_driver.is_suspended = KGSL_TRUE;
	}
	mutex_unlock(&kgsl_driver.mutex);
	return KGSL_SUCCESS;
}

/*Resume function*/
static int kgsl_resume(struct platform_device *dev)
{
	int i;
	struct kgsl_device *device;

	mutex_lock(&kgsl_driver.mutex);
	if (kgsl_driver.power_flags != 0) {
		for (i = 0; i < KGSL_DEVICE_MAX; i++) {
			device = kgsl_driver.devp[i];
			if (device != NULL)
				device->ftbl.device_wake(device);
		}

		kgsl_driver.is_suspended = KGSL_FALSE;
	}
	mutex_unlock(&kgsl_driver.mutex);
	return KGSL_SUCCESS;
}

/* file operations */
static struct kgsl_file_private *
kgsl_get_process_private(struct kgsl_device_private *cur_dev_priv)
{
	struct kgsl_device_private *dev_priv;
	struct kgsl_file_private *private;

	list_for_each_entry(dev_priv, &kgsl_driver.dev_priv_list, list) {
		if ((dev_priv->pid == task_pid_nr(current)) &&
					(dev_priv != cur_dev_priv)) {
			private = dev_priv->process_priv;
			private->refcnt++;
			return private;
		}
	}

	/* no existing process private found for this dev_priv, create one */
	private = kzalloc(sizeof(struct kgsl_file_private), GFP_KERNEL);
	if (private == NULL)
		KGSL_DRV_ERR("Error: cannot allocate process private data\n");
	else
		private->refcnt = 1;
	return private;
}

static int
kgsl_init_process_private(struct kgsl_device *device,
					struct kgsl_file_private *private)
{
	int result = 0;

       /* only initialize it once */
	if (private->refcnt != 1)
		return result;

	INIT_LIST_HEAD(&private->mem_list);
	INIT_LIST_HEAD(&private->preserve_entry_list);
	private->preserve_list_size = 0;

	/*NOTE: this must happen after first_open */
#ifdef CONFIG_MSM_KGSL_MMU
#ifdef CONFIG_KGSL_PER_PROCESS_PAGE_TABLE
	private->pagetable =
		kgsl_mmu_getpagetable(&device->mmu, task_pid_nr(current));
	if (private->pagetable == NULL)
		return -ENOMEM;
	result = kgsl_yamato_setup_pt(device, private->pagetable);
	if (result) {
		kgsl_mmu_putpagetable(private->pagetable);
		private->pagetable = NULL;
		result = -EINVAL;
	}
#else
	private->pagetable = device->mmu.hwpagetable;
#endif
	private->vmalloc_size = 0;
#endif
	return result;
}

static void kgsl_cleanup_process_private(struct kgsl_device *device,
					struct kgsl_file_private *private)
{
	struct kgsl_mem_entry *entry, *entry_tmp;

	list_for_each_entry_safe(entry, entry_tmp, &private->mem_list, list)
		kgsl_remove_mem_entry(entry, false);

	entry = NULL;
	entry_tmp = NULL;
	list_for_each_entry_safe(entry, entry_tmp,
			&private->preserve_entry_list, list)
		kgsl_remove_mem_entry(entry, false);

	if (private->pagetable != NULL) {
#ifdef CONFIG_KGSL_PER_PROCESS_PAGE_TABLE
		kgsl_yamato_cleanup_pt(device, private->pagetable);
		kgsl_mmu_putpagetable(private->pagetable);
#endif
		private->pagetable = NULL;
	}
	return;
}

static void kgsl_put_process_private(struct kgsl_device *device,
					struct kgsl_file_private *private)
{
	if (private->refcnt-- == 1) {
		kgsl_cleanup_process_private(device, private);
		kfree(private);
	}
}

static int kgsl_release(struct inode *inodep, struct file *filep)
{
	int result = 0;
	unsigned int i;
	struct kgsl_device_private *dev_priv = NULL;
	struct kgsl_file_private *private = NULL;
	struct kgsl_device *device;

	device = kgsl_driver.devp[iminor(inodep)];
	BUG_ON(device == NULL);

	KGSL_PRE_HWACCESS();

	dev_priv = (struct kgsl_device_private *) filep->private_data;
	BUG_ON(dev_priv == NULL);
	BUG_ON(device != dev_priv->device);
	private = dev_priv->process_priv;
	BUG_ON(private == NULL);
	filep->private_data = NULL;
	list_del(&dev_priv->list);

	for (i = 0; i < KGSL_CONTEXT_MAX; i++) {
		if (dev_priv->ctxt_id_mask & (1 << i)) {
			device->ftbl.device_drawctxt_destroy(device, i);
		}
	}

	kgsl_put_process_private(device, private);

	if (atomic_dec_return(&device->open_count) == -1) {
		KGSL_DRV_VDBG("last_release\n");
		result = device->ftbl.device_last_release_locked(device);
	}

	KGSL_POST_HWACCESS();
	kfree(dev_priv);

	return result;
}

static int kgsl_open(struct inode *inodep, struct file *filep)
{
	int result = 0;
	struct kgsl_device_private *dev_priv;
	struct kgsl_device *device;
	unsigned int minor = iminor(inodep);

	KGSL_DRV_DBG("file %p pid %d\n", filep, task_pid_nr(current));

	BUG_ON(minor >= KGSL_DEVICE_MAX || minor < KGSL_DEVICE_YAMATO);
	device = kgsl_driver.devp[minor];
	BUG_ON(device == NULL);

	if (filep->f_flags & O_EXCL) {
		KGSL_DRV_ERR("O_EXCL not allowed\n");
		return -EBUSY;
	}

	dev_priv = kzalloc(sizeof(struct kgsl_device_private), GFP_KERNEL);
	if (dev_priv == NULL) {
		KGSL_DRV_ERR("cannot allocate device private data\n");
		result = -ENOMEM;
		goto done;
	}

	mutex_lock(&kgsl_driver.mutex);

	dev_priv->ctxt_id_mask = 0;
	dev_priv->device = device;
	dev_priv->pid = task_pid_nr(current);
	dev_priv->device->id = minor;
	filep->private_data = dev_priv;

	list_add(&dev_priv->list, &kgsl_driver.dev_priv_list);

	/* Get file (per process) private struct */
	dev_priv->process_priv = kgsl_get_process_private(dev_priv);
	if (dev_priv->process_priv ==  NULL) {
		KGSL_DRV_ERR("cannot allocate or find file private data\n");
		result = -ENOMEM;
		goto done;
	}

	if (atomic_inc_and_test(&device->open_count)) {
		result = device->ftbl.device_first_open_locked(device);
		if (result != 0) {
			KGSL_DRV_ERR("device_first_open() failed, minor=%d\n",
					minor);
			goto done;
		}
	}

	result = kgsl_init_process_private(device, dev_priv->process_priv);

done:
	mutex_unlock(&kgsl_driver.mutex);
	if (result != 0)
		kgsl_release(inodep, filep);
	return result;
}

/*call with driver locked */
static struct kgsl_mem_entry *
kgsl_sharedmem_find(struct kgsl_file_private *private, unsigned int gpuaddr)
{
	struct kgsl_mem_entry *entry = NULL, *result = NULL;

	BUG_ON(private == NULL);

	list_for_each_entry(entry, &private->mem_list, list) {
		if (entry->memdesc.gpuaddr == gpuaddr) {
			result = entry;
			break;
		}
	}
	return result;
}

/*call with driver locked */
struct kgsl_mem_entry *
kgsl_sharedmem_find_region(struct kgsl_file_private *private,
				unsigned int gpuaddr,
				size_t size)
{
	struct kgsl_mem_entry *entry = NULL, *result = NULL;

	BUG_ON(private == NULL);

	list_for_each_entry(entry, &private->mem_list, list) {
		if (gpuaddr >= entry->memdesc.gpuaddr &&
		    ((gpuaddr + size) <=
			(entry->memdesc.gpuaddr + entry->memdesc.size))) {
			result = entry;
			break;
		}
	}

	return result;
}

/*call all ioctl sub functions with driver locked*/
static long kgsl_ioctl_device_getproperty(struct kgsl_device_private *dev_priv,
					 void __user *arg)
{
	int result = 0;
	struct kgsl_device_getproperty param;

	if (copy_from_user(&param, arg, sizeof(param))) {
		result = -EFAULT;
		goto done;
	}
	result = dev_priv->device->ftbl.device_getproperty(dev_priv->device,
					 param.type,
					 param.value, param.sizebytes);
done:
	return result;
}

static long kgsl_ioctl_device_regread(struct kgsl_device_private *dev_priv,
				     void __user *arg)
{
	int result = 0;
	struct kgsl_device_regread param;

	if (copy_from_user(&param, arg, sizeof(param))) {
		result = -EFAULT;
		goto done;
	}
	result = dev_priv->device->ftbl.device_regread(dev_priv->device,
						param.offsetwords,
						&param.value);

	if (result != 0)
		goto done;

	if (copy_to_user(arg, &param, sizeof(param))) {
		result = -EFAULT;
		goto done;
	}
done:
	return result;
}


static long kgsl_ioctl_device_waittimestamp(struct kgsl_device_private
						*dev_priv, void __user *arg)
{
	int result = 0;
	struct kgsl_device_waittimestamp param;

	if (copy_from_user(&param, arg, sizeof(param))) {
		result = -EFAULT;
		goto done;
	}

	/* Don't wait forever, set a max value for now */
	if (param.timeout == -1)
		param.timeout = 10 * MSEC_PER_SEC;
	result = dev_priv->device->ftbl.device_waittimestamp(dev_priv->device,
				     param.timestamp,
				     param.timeout);

	kgsl_runpending(dev_priv->device);
done:
	return result;
}

static long kgsl_ioctl_rb_issueibcmds(struct kgsl_device_private *dev_priv,
				     void __user *arg)
{
	int result = 0;
	struct kgsl_ringbuffer_issueibcmds param;

	if (copy_from_user(&param, arg, sizeof(param))) {
		result = -EFAULT;
		goto done;
	}

	if ((dev_priv->ctxt_id_mask & 1 << param.drawctxt_id) == 0) {
		result = -EINVAL;
		KGSL_DRV_ERR("invalid drawctxt drawctxt_id %d\n",
				      param.drawctxt_id);
		goto done;
	}

	if (kgsl_sharedmem_find_region(dev_priv->process_priv,
				param.ibaddr,
				param.sizedwords*sizeof(uint32_t)) == NULL) {
		KGSL_DRV_ERR("invalid cmd buffer ibaddr %08x " \
					"sizedwords %d\n",
					param.ibaddr, param.sizedwords);
		result = -EINVAL;
		goto done;
	}

	result = dev_priv->device->ftbl.device_issueibcmds(dev_priv,
					     param.drawctxt_id,
					     param.ibaddr,
					     param.sizedwords,
					     &param.timestamp,
					     param.flags);

	if (result != 0)
		goto done;

	if (copy_to_user(arg, &param, sizeof(param))) {
		result = -EFAULT;
		goto done;
	}
done:
	return result;
}

static long kgsl_ioctl_cmdstream_readtimestamp(struct kgsl_device_private
						*dev_priv, void __user *arg)
{
	int result = 0;
	struct kgsl_cmdstream_readtimestamp param;

	if (copy_from_user(&param, arg, sizeof(param))) {
		result = -EFAULT;
		goto done;
	}

	param.timestamp = dev_priv->device->ftbl.device_cmdstream_readtimestamp
							(dev_priv->device,
							param.type);

	if (result != 0)
		goto done;

	if (copy_to_user(arg, &param, sizeof(param))) {
		result = -EFAULT;
		goto done;
	}
done:
	return result;
}

static long kgsl_ioctl_cmdstream_freememontimestamp(struct kgsl_device_private
						*dev_priv, void __user *arg)
{
	int result = 0;
	struct kgsl_cmdstream_freememontimestamp param;
	struct kgsl_mem_entry *entry = NULL;

	if (copy_from_user(&param, arg, sizeof(param))) {
		result = -EFAULT;
		goto done;
	}

	entry = kgsl_sharedmem_find(dev_priv->process_priv, param.gpuaddr);
	if (entry == NULL) {
		KGSL_DRV_ERR("invalid gpuaddr %08x\n", param.gpuaddr);
		result = -EINVAL;
		goto done;
	}
#ifdef CONFIG_MSM_KGSL_MMU
	if (entry->memdesc.priv & KGSL_MEMFLAGS_VMALLOC_MEM)
		entry->memdesc.priv &= ~KGSL_MEMFLAGS_CACHE_MASK;
#endif
	result = kgsl_cmdstream_freememontimestamp(dev_priv->device,
							entry,
							param.timestamp,
							param.type);

	kgsl_runpending(dev_priv->device);
done:
	return result;
}

static long kgsl_ioctl_drawctxt_create(struct kgsl_device_private *dev_priv,
				      void __user *arg)
{
	int result = 0;
	struct kgsl_drawctxt_create param;

	if (copy_from_user(&param, arg, sizeof(param))) {
		result = -EFAULT;
		goto done;
	}

	result = dev_priv->device->ftbl.device_drawctxt_create(dev_priv,
					param.flags,
					&param.drawctxt_id);
	if (result != 0)
		goto done;

	if (copy_to_user(arg, &param, sizeof(param))) {
		result = -EFAULT;
		goto done;
	}

	dev_priv->ctxt_id_mask |= 1 << param.drawctxt_id;

done:
	return result;
}

static long kgsl_ioctl_drawctxt_destroy(struct kgsl_device_private *dev_priv,
				       void __user *arg)
{
	int result = 0;
	struct kgsl_drawctxt_destroy param;

	if (copy_from_user(&param, arg, sizeof(param))) {
		result = -EFAULT;
		goto done;
	}

	if ((dev_priv->ctxt_id_mask & 1 << param.drawctxt_id) == 0) {
		result = -EINVAL;
		goto done;
	}

	result = dev_priv->device->ftbl.device_drawctxt_destroy(
							dev_priv->device,
							param.drawctxt_id);
	if (result == 0)
		dev_priv->ctxt_id_mask &= ~(1 << param.drawctxt_id);

done:
	return result;
}

void kgsl_remove_mem_entry(struct kgsl_mem_entry *entry, bool preserve)
{
	/* If allocation is vmalloc and preserve is requested then save
	* the allocation in a free list to be used later instead of
	* freeing it here */
	if (KGSL_MEMFLAGS_VMALLOC_MEM & entry->memdesc.priv &&
		preserve &&
		entry->priv->preserve_list_size < KGSL_MAX_PRESERVED_BUFFERS &&
		entry->memdesc.size <= KGSL_MAX_SIZE_OF_PRESERVED_BUFFER) {
		if (entry->free_list.prev) {
			list_del(&entry->free_list);
			entry->free_list.prev = NULL;
		}
		if (entry->list.prev) {
			list_del(&entry->list);
			entry->list.prev = NULL;
		}
		list_add(&entry->list, &entry->priv->preserve_entry_list);
		entry->priv->preserve_list_size++;
		return;
	}
	kgsl_mmu_unmap(entry->memdesc.pagetable,
			entry->memdesc.gpuaddr & KGSL_PAGEMASK,
			entry->memdesc.size);
	if (KGSL_MEMFLAGS_VMALLOC_MEM & entry->memdesc.priv) {
		vfree((void *)entry->memdesc.physaddr);
		entry->priv->vmalloc_size -= entry->memdesc.size;
	} else
		kgsl_put_phys_file(entry->pmem_file);

	/* remove the entry from list and free_list if it exists */
	if (entry->list.prev)
		list_del(&entry->list);
	if (entry->free_list.prev)
		list_del(&entry->free_list);

	kfree(entry);

}

static long kgsl_ioctl_sharedmem_free(struct kgsl_file_private *private,
				     void __user *arg)
{
	int result = 0;
	struct kgsl_sharedmem_free param;
	struct kgsl_mem_entry *entry = NULL;

	if (copy_from_user(&param, arg, sizeof(param))) {
		result = -EFAULT;
		goto done;
	}
	entry = kgsl_sharedmem_find(private, param.gpuaddr);

	if (entry == NULL) {
		KGSL_DRV_ERR("invalid gpuaddr %08x\n", param.gpuaddr);
		result = -EINVAL;
		goto done;
	}

	kgsl_remove_mem_entry(entry, false);
done:
	return result;
}

#ifdef CONFIG_MSM_KGSL_MMU
static long kgsl_ioctl_sharedmem_from_vmalloc(struct kgsl_file_private *private,
					      void __user *arg)
{
	int result = 0, len, found = 0;
	struct kgsl_sharedmem_from_vmalloc param;
	struct kgsl_mem_entry *entry = NULL, *entry_tmp = NULL;
	void *vmalloc_area;
	struct vm_area_struct *vma;

	if (copy_from_user(&param, arg, sizeof(param))) {
		result = -EFAULT;
		goto error;
	}

	if (!param.hostptr) {
		KGSL_DRV_ERR
		    ("Invalid host pointer of malloc passed: param.hostptr "
		     "%08x\n", param.hostptr);
		result = -EINVAL;
		goto error;
	}

	vma = find_vma(current->mm, param.hostptr);
	if (!vma) {
		KGSL_MEM_ERR("Could not find vma for address %x\n",
			     param.hostptr);
		result = -EINVAL;
		goto error;
	}
	len = vma->vm_end - vma->vm_start;
	if (vma->vm_pgoff || !KGSL_IS_PAGE_ALIGNED(len) ||
	    !KGSL_IS_PAGE_ALIGNED(vma->vm_start)) {
		KGSL_MEM_ERR
		("kgsl vmalloc mapping must be at offset 0 and page aligned\n");
		result = -EINVAL;
		goto error;
	}
	if (vma->vm_start != param.hostptr) {
		KGSL_MEM_ERR
		    ("vma start address is not equal to mmap address\n");
		result = -EINVAL;
		goto error;
	}

	if ((private->vmalloc_size + len) > KGSL_GRAPHICS_MEMORY_LOW_WATERMARK
	    && !param.force_no_low_watermark) {
		result = -ENOMEM;
		goto error;
	}

	list_for_each_entry_safe(entry, entry_tmp,
				&private->preserve_entry_list, list) {
		if (entry->memdesc.size == len) {
			list_del(&entry->list);
			found = 1;
			break;
		}
	}

	if (!found) {
		entry = kzalloc(sizeof(struct kgsl_mem_entry), GFP_KERNEL);
		if (entry == NULL) {
			result = -ENOMEM;
			goto error;
		}

		/* allocate memory and map it to user space */
		vmalloc_area = vmalloc_user(len);
		if (!vmalloc_area) {
			KGSL_MEM_ERR("vmalloc failed\n");
			result = -ENOMEM;
			goto error_free_entry;
		}
		kgsl_cache_range_op((unsigned int)vmalloc_area, len,
			KGSL_MEMFLAGS_CACHE_INV | KGSL_MEMFLAGS_VMALLOC_MEM);

		result =
		    kgsl_mmu_map(private->pagetable,
			(unsigned long)vmalloc_area, len,
			GSL_PT_PAGE_RV | GSL_PT_PAGE_WV,
			&entry->memdesc.gpuaddr, KGSL_MEMFLAGS_ALIGN4K |
						KGSL_MEMFLAGS_VMALLOC_MEM);
		if (result != 0)
			goto error_free_vmalloc;

		entry->memdesc.pagetable = private->pagetable;
		entry->memdesc.size = len;
		entry->memdesc.priv = KGSL_MEMFLAGS_VMALLOC_MEM |
			    KGSL_MEMFLAGS_CACHE_CLEAN;
		entry->memdesc.physaddr = (unsigned long)vmalloc_area;
		entry->priv = private;
		private->vmalloc_size += len;

	} else {
		KGSL_MEM_INFO("Reusing memory entry: %x, size: %x\n",
				(unsigned int)entry, entry->memdesc.size);
		entry->priv->preserve_list_size--;
		vmalloc_area = (void *)entry->memdesc.physaddr;
	}

	if (!kgsl_cache_enable)
		vma->vm_page_prot = pgprot_writecombine(vma->vm_page_prot);

	result = remap_vmalloc_range(vma, vmalloc_area, 0);
	if (result) {
		KGSL_MEM_ERR("remap_vmalloc_range returned %d\n", result);
		goto error_unmap_entry;
	}

	entry->memdesc.hostptr = (void *)param.hostptr;

	param.gpuaddr = entry->memdesc.gpuaddr;

	if (copy_to_user(arg, &param, sizeof(param))) {
		result = -EFAULT;
		goto error_unmap_entry;
	}
	list_add(&entry->list, &private->mem_list);

	return 0;

error_unmap_entry:
	kgsl_mmu_unmap(private->pagetable, entry->memdesc.gpuaddr,
		       entry->memdesc.size);

error_free_vmalloc:
	vfree(vmalloc_area);

error_free_entry:
	kfree(entry);

error:
	return result;
}
#endif /*CONFIG_MSM_KGSL_MMU*/

static int kgsl_get_phys_file(int fd, unsigned long *start, unsigned long *len,
			      struct file **filep)
{
	struct file *fbfile;
	int put_needed;
	unsigned long vstart = 0;
	int ret = 0;
	dev_t rdev;
	struct fb_info *info;

	*filep = NULL;
	if (!get_pmem_file(fd, start, &vstart, len, filep))
		return 0;

	fbfile = fget_light(fd, &put_needed);
	if (fbfile == NULL)
		return -1;

	rdev = fbfile->f_dentry->d_inode->i_rdev;
	info = MAJOR(rdev) == FB_MAJOR ? registered_fb[MINOR(rdev)] : NULL;
	if (info) {
		*start = info->fix.smem_start;
		*len = info->fix.smem_len;
		ret = 0;
	} else
		ret = -1;
	fput_light(fbfile, put_needed);

	return ret;
}

static void kgsl_put_phys_file(struct file *file)
{
	KGSL_DRV_DBG("put phys file %p\n", file);
	if (file)
		put_pmem_file(file);
}

static int kgsl_ioctl_sharedmem_from_pmem(struct kgsl_file_private *private,
						void __user *arg)
{
	int result = 0;
	struct kgsl_sharedmem_from_pmem param;
	struct kgsl_mem_entry *entry = NULL;
	unsigned long start = 0, len = 0;
	struct file *pmem_file = NULL;

	if (copy_from_user(&param, arg, sizeof(param))) {
		result = -EFAULT;
		goto error;
	}

	if (kgsl_get_phys_file(param.pmem_fd, &start, &len, &pmem_file)) {
		result = -EINVAL;
		goto error;

	} else {
		if (!param.len)
			param.len = len;

		if (param.offset + param.len > len) {
			KGSL_DRV_ERR("%s: region too large "
					"0x%x + 0x%x >= 0x%lx\n",
				     __func__, param.offset, param.len, len);
			result = -EINVAL;
			goto error_put_pmem;
		}
	}

	KGSL_MEM_INFO("get phys file %p start 0x%lx len 0x%lx\n",
		      pmem_file, start, len);
	KGSL_DRV_DBG("locked phys file %p\n", pmem_file);

	entry = kzalloc(sizeof(*entry), GFP_KERNEL);
	if (entry == NULL) {
		result = -ENOMEM;
		goto error_put_pmem;
	}

	entry->pmem_file = pmem_file;

	entry->memdesc.pagetable = private->pagetable;

	/* Any MMU mapped memory must have a length in multiple of PAGESIZE */
	entry->memdesc.size = ALIGN(param.len, PAGE_SIZE);
	/*we shouldn't need to write here from kernel mode */
	entry->memdesc.hostptr = NULL;
	/* ensure that MMU mappings are at page boundary */
	entry->memdesc.physaddr = start + (param.offset & KGSL_PAGEMASK);
	result = kgsl_mmu_map(private->pagetable, entry->memdesc.physaddr,
			entry->memdesc.size, GSL_PT_PAGE_RV | GSL_PT_PAGE_WV,
			&entry->memdesc.gpuaddr,
			KGSL_MEMFLAGS_ALIGN4K | KGSL_MEMFLAGS_CONPHYS);
	if (result)
		goto error_free_entry;

	/* If the offset is not at 4K boundary then add the correct offset
	 * value to gpuaddr */
	entry->memdesc.gpuaddr += (param.offset & ~KGSL_PAGEMASK);
	param.gpuaddr = entry->memdesc.gpuaddr;

	if (copy_to_user(arg, &param, sizeof(param))) {
		result = -EFAULT;
		goto error_unmap_entry;
	}
	list_add(&entry->list, &private->mem_list);
	return result;

error_unmap_entry:
	kgsl_mmu_unmap(entry->memdesc.pagetable,
			entry->memdesc.gpuaddr & KGSL_PAGEMASK,
			entry->memdesc.size);
error_free_entry:
	kfree(entry);

error_put_pmem:
	kgsl_put_phys_file(pmem_file);

error:
	return result;
}

#ifdef CONFIG_MSM_KGSL_MMU
/*This function flushes a graphics memory allocation from CPU cache
 *when caching is enabled with MMU*/
static long kgsl_ioctl_sharedmem_flush_cache(struct kgsl_file_private *private,
				       void __user *arg)
{
	int result = 0;
	struct kgsl_mem_entry *entry;
	struct kgsl_sharedmem_free param;

	if (copy_from_user(&param, arg, sizeof(param))) {
		result = -EFAULT;
		goto done;
	}

	entry = kgsl_sharedmem_find(private, param.gpuaddr);
	if (!entry) {
		KGSL_DRV_ERR("invalid gpuaddr %08x\n", param.gpuaddr);
		result = -EINVAL;
		goto done;
	}
	result = kgsl_cache_range_op((unsigned long)entry->memdesc.hostptr,
					entry->memdesc.size,
				KGSL_MEMFLAGS_CACHE_CLEAN |
				KGSL_MEMFLAGS_HOSTADDR);
	/* Mark memory as being flushed so we don't flush it again */
	entry->memdesc.priv &= ~KGSL_MEMFLAGS_CACHE_MASK;
done:
	return result;
}
#endif /*CONFIG_MSM_KGSL_MMU*/

static long kgsl_ioctl(struct file *filep, unsigned int cmd, unsigned long arg)
{
	int result = 0;
	struct kgsl_device_private *dev_priv = filep->private_data;
	struct inode *inodep = filep->f_path.dentry->d_inode;
	struct kgsl_device *device;

	BUG_ON(dev_priv == NULL);
	device = kgsl_driver.devp[iminor(inodep)];
	BUG_ON(device == NULL);
	BUG_ON(device != dev_priv->device);

	KGSL_DRV_VDBG("filep %p cmd 0x%08x arg 0x%08lx\n", filep, cmd, arg);

	KGSL_PRE_HWACCESS();
	switch (cmd) {

	case IOCTL_KGSL_DEVICE_GETPROPERTY:
		result =
		    kgsl_ioctl_device_getproperty(dev_priv, (void __user *)arg);
		break;

	case IOCTL_KGSL_DEVICE_REGREAD:
		result =
		    kgsl_ioctl_device_regread(dev_priv, (void __user *)arg);
		break;

	case IOCTL_KGSL_DEVICE_WAITTIMESTAMP:
		result = kgsl_ioctl_device_waittimestamp(dev_priv,
							(void __user *)arg);
		/* order reads to the buffer written to by the GPU */
		rmb();
		break;

	case IOCTL_KGSL_RINGBUFFER_ISSUEIBCMDS:
#ifdef CONFIG_MSM_KGSL_MMU
		if (kgsl_cache_enable)
			kgsl_clean_cache_all(dev_priv->process_priv);
#endif
#ifdef CONFIG_MSM_KGSL_DRM
		kgsl_gpu_mem_flush(DRM_KGSL_GEM_CACHE_OP_TO_DEV);
#endif
		result =
		    kgsl_ioctl_rb_issueibcmds(dev_priv, (void __user *)arg);
#ifdef CONFIG_MSM_KGSL_DRM
		kgsl_gpu_mem_flush(DRM_KGSL_GEM_CACHE_OP_FROM_DEV);
#endif
		break;

	case IOCTL_KGSL_CMDSTREAM_READTIMESTAMP:
		result =
		    kgsl_ioctl_cmdstream_readtimestamp(dev_priv,
							(void __user *)arg);
		break;

	case IOCTL_KGSL_CMDSTREAM_FREEMEMONTIMESTAMP:
		result =
		    kgsl_ioctl_cmdstream_freememontimestamp(dev_priv,
						    (void __user *)arg);
		break;

	case IOCTL_KGSL_DRAWCTXT_CREATE:
		result = kgsl_ioctl_drawctxt_create(dev_priv,
							(void __user *)arg);
		break;

	case IOCTL_KGSL_DRAWCTXT_DESTROY:
		result =
		    kgsl_ioctl_drawctxt_destroy(dev_priv, (void __user *)arg);
		break;

	case IOCTL_KGSL_SHAREDMEM_FREE:
		result = kgsl_ioctl_sharedmem_free(dev_priv->process_priv,
							(void __user *)arg);
		break;

#ifdef CONFIG_MSM_KGSL_MMU
	case IOCTL_KGSL_SHAREDMEM_FROM_VMALLOC:
		kgsl_runpending_all();
		result = kgsl_ioctl_sharedmem_from_vmalloc(
							dev_priv->process_priv,
							   (void __user *)arg);
		break;

	case IOCTL_KGSL_SHAREDMEM_FLUSH_CACHE:
		if (kgsl_cache_enable)
			result =
			    kgsl_ioctl_sharedmem_flush_cache(
							dev_priv->process_priv,
						       (void __user *)arg);
		break;
#endif
	case IOCTL_KGSL_SHAREDMEM_FROM_PMEM:
		kgsl_runpending_all();
		result = kgsl_ioctl_sharedmem_from_pmem(dev_priv->process_priv,
							(void __user *)arg);
		break;



	default:
		/* call into device specific ioctls */
		result = device->ftbl.device_ioctl(dev_priv, cmd, arg);
		break;
	}
	KGSL_POST_HWACCESS();
	KGSL_DRV_VDBG("result %d\n", result);
	return result;
}

static int kgsl_mmap(struct file *file, struct vm_area_struct *vma)
{
	int result = 0;
	struct kgsl_memdesc *memdesc = NULL;
	unsigned long vma_size = vma->vm_end - vma->vm_start;
	unsigned long vma_offset = vma->vm_pgoff << PAGE_SHIFT;
	struct inode *inodep = file->f_path.dentry->d_inode;
	struct kgsl_device *device;

	device = kgsl_driver.devp[iminor(inodep)];
	BUG_ON(device == NULL);

	mutex_lock(&kgsl_driver.mutex);

	/*allow device memstore to be mapped read only */
	if (vma_offset == device->memstore.physaddr) {
		if (vma->vm_flags & VM_WRITE) {
			result = -EPERM;
			goto done;
		}
		memdesc = &device->memstore;
	} else {
		result = -EINVAL;
		goto done;
	}

	if (memdesc->size != vma_size) {
		KGSL_MEM_ERR("file %p bad size %ld, should be %d\n",
			file, vma_size, memdesc->size);
		result = -EINVAL;
		goto done;
	}
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	result = remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff,
				vma_size, vma->vm_page_prot);
	if (result != 0) {
		KGSL_MEM_ERR("remap_pfn_range returned %d\n",
				result);
		goto done;
	}
done:
	mutex_unlock(&kgsl_driver.mutex);
	return result;
}

static const struct file_operations kgsl_fops = {
	.owner = THIS_MODULE,
	.release = kgsl_release,
	.open = kgsl_open,
	.mmap = kgsl_mmap,
	.unlocked_ioctl = kgsl_ioctl,
};


struct kgsl_driver kgsl_driver = {
	.mutex = __MUTEX_INITIALIZER(kgsl_driver.mutex),
};

static void kgsl_device_unregister(void)
{
	int j;

	for (j = 0; j < kgsl_driver.num_devs; j++) {
		device_destroy(kgsl_driver.class,
					MKDEV(MAJOR(kgsl_driver.dev_num), j));
	}

	class_destroy(kgsl_driver.class);
	cdev_del(&kgsl_driver.cdev);
	unregister_chrdev_region(kgsl_driver.dev_num, kgsl_driver.num_devs);
}

static void kgsl_driver_cleanup(void)
{

	if (kgsl_driver.yamato_interrupt_num > 0) {
		if (kgsl_driver.yamato_have_irq) {
			free_irq(kgsl_driver.yamato_interrupt_num, NULL);
			kgsl_driver.yamato_have_irq = 0;
		}
		kgsl_driver.yamato_interrupt_num = 0;
	}

	if (kgsl_driver.g12_interrupt_num > 0) {
		if (kgsl_driver.g12_have_irq) {
			free_irq(kgsl_driver.g12_interrupt_num, NULL);
			kgsl_driver.g12_have_irq = 0;
		}
		kgsl_driver.g12_interrupt_num = 0;
	}

	pm_qos_remove_requirement(PM_QOS_SYSTEM_BUS_FREQ, "kgsl_3d");

	if (kgsl_driver.yamato_grp_pclk) {
		clk_put(kgsl_driver.yamato_grp_pclk);
		kgsl_driver.yamato_grp_pclk = NULL;
	}

	/* shutdown memory apertures */
	kgsl_sharedmem_close(&kgsl_driver.shmem);

	if (kgsl_driver.yamato_grp_clk) {
		clk_put(kgsl_driver.yamato_grp_clk);
		kgsl_driver.yamato_grp_clk = NULL;
	}

	if (kgsl_driver.imem_clk != NULL) {
		clk_put(kgsl_driver.imem_clk);
		kgsl_driver.imem_clk = NULL;
	}

	if (kgsl_driver.g12_grp_pclk) {
		clk_put(kgsl_driver.g12_grp_pclk);
		kgsl_driver.g12_grp_pclk = NULL;
	}

	if (kgsl_driver.g12_grp_clk) {
		clk_put(kgsl_driver.g12_grp_clk);
		kgsl_driver.g12_grp_clk = NULL;
		pm_qos_remove_requirement(PM_QOS_SYSTEM_BUS_FREQ, "kgsl_2d");
	}

	kgsl_driver.pdev = NULL;
	kgsl_driver.power_flags = 0;

}

static int kgsl_add_device(int dev_idx)
{
	struct kgsl_device *device;
	int err = KGSL_FAILURE;

	/* init fields which kgsl_open() will use */
	if (dev_idx == KGSL_DEVICE_YAMATO) {
		device = kgsl_get_yamato_generic_device();
		if (device == NULL)
			goto done;
		kgsl_driver.devp[KGSL_DEVICE_YAMATO] = device;
		err = kgsl_yamato_getfunctable(&device->ftbl);
	} else if (dev_idx == KGSL_DEVICE_G12) {
		device = kgsl_get_g12_generic_device();
		if (device == NULL)
			goto done;
		kgsl_driver.devp[KGSL_DEVICE_G12] = device;
		err = kgsl_g12_getfunctable(&device->ftbl);
	}
	if (err != KGSL_SUCCESS) {
		KGSL_DRV_ERR("kgsl_*_getfunctable() failed dev_idx=%d\n",
					dev_idx);
		goto done;
	}
	err = KGSL_SUCCESS;
	atomic_set(&device->open_count, -1);

 done:
	return err;
}

static int kgsl_register_dev(int num_devs)
{
	int err;
	int j, i;
	char device_str[sizeof(DRIVER_NAME) + 5];
	dev_t dev;

	/* alloc major and minor device numbers */
	err = alloc_chrdev_region(&kgsl_driver.dev_num, 0, num_devs,
				DRIVER_NAME);
	if (err < 0) {
		KGSL_DRV_ERR("alloc_chrdev_region failed err = %d\n", err);
		return err;
	}

	/* create sysfs entries */
	kgsl_driver.class = class_create(THIS_MODULE, CLASS_NAME);
	if (IS_ERR(kgsl_driver.class)) {
		err = PTR_ERR(kgsl_driver.class);
		KGSL_DRV_ERR("failed to create class %s", CLASS_NAME);
		goto error_class_create;
	}

	for (i = 0; i < num_devs; i++) {
		if (i == KGSL_DEVICE_YAMATO)
			snprintf(device_str, sizeof(device_str), "%s-3d0",
				DRIVER_NAME);
		else if (i == KGSL_DEVICE_G12)
			snprintf(device_str, sizeof(device_str), "%s-2d0",
				DRIVER_NAME);
		dev = MKDEV(MAJOR(kgsl_driver.dev_num), i);
		kgsl_driver.base_dev[i] = device_create(kgsl_driver.class,
						&kgsl_driver.pdev->dev,
						dev, NULL, device_str);
		if (IS_ERR(kgsl_driver.base_dev[i])) {
			err = PTR_ERR(kgsl_driver.base_dev[i]);
			KGSL_DRV_ERR("device_create failed err=%d\n", err);
			for (j = 0; j < i; j++)
				device_destroy(kgsl_driver.class,
					MKDEV(MAJOR(kgsl_driver.dev_num), j));
			goto error_device_create;
		}
	}

	/* add char dev(s) */
	cdev_init(&kgsl_driver.cdev, &kgsl_fops);
	kgsl_driver.cdev.owner = THIS_MODULE;
	kgsl_driver.cdev.ops = &kgsl_fops;
	err = cdev_add(&kgsl_driver.cdev, MKDEV(MAJOR(kgsl_driver.dev_num), 0),
			num_devs);
	if (err) {
		KGSL_DRV_ERR("kgsl: cdev_add() failed, dev_num= %d,"
				" result= %d\n", kgsl_driver.dev_num, err);
		goto error_cdev_add;
	}

	KGSL_DRV_DBG("register_dev successful, cdev.dev=0x%x\n",
						kgsl_driver.cdev.dev);
	return err;

error_cdev_add:
	for (j = 0; j < num_devs; j++) {
		device_destroy(kgsl_driver.class,
					MKDEV(MAJOR(kgsl_driver.dev_num), j));
	}
error_device_create:
	class_destroy(kgsl_driver.class);
error_class_create:
	unregister_chrdev_region(kgsl_driver.dev_num, num_devs);

	return err;
}

static int __devinit kgsl_platform_probe(struct platform_device *pdev)
{
	int i, result = 0;
	struct clk *clk, *grp_clk;
	struct resource *res = NULL;
	struct kgsl_platform_data *pdata = NULL;

	kgsl_debug_init();

	for (i = 0; i < KGSL_DEVICE_MAX; i++) {
		kgsl_driver.base_dev[i] = NULL;
		kgsl_driver.devp[i] = NULL;
	}
	kgsl_driver.num_devs = 0;
	INIT_LIST_HEAD(&kgsl_driver.dev_priv_list);
	/*acquire clocks */
	BUG_ON(kgsl_driver.yamato_grp_clk != NULL);
	BUG_ON(kgsl_driver.g12_grp_clk != NULL);

	kgsl_driver.pdev = pdev;
	pdata = pdev->dev.platform_data;

	clk = clk_get(&pdev->dev, "grp_pclk");
	if (IS_ERR(clk))
		clk = NULL;
	kgsl_driver.yamato_grp_pclk = clk;

	clk = clk_get(&pdev->dev, pdata->grp3d_clk_name);
	if (IS_ERR(clk)) {
		result = PTR_ERR(clk);
		KGSL_DRV_ERR("clk_get(%s) returned %d\n", pdata->grp3d_clk_name,
			result);
		goto done;
	}
	kgsl_driver.yamato_grp_clk = grp_clk = clk;

	clk = clk_get(&pdev->dev, "grp_src_clk");
	if (IS_ERR(clk)) {
		clk = grp_clk; /* Fallback to slave */
	}
	kgsl_driver.yamato_grp_src_clk = clk;


	/* put the AXI bus into asynchronous mode with the graphics cores */
	if (pdata != NULL) {
		if ((pdata->set_grp3d_async != NULL) &&
			(pdata->max_grp3d_freq) &&
			(!pdata->set_grp3d_async()))
			clk_set_min_rate(clk, pdata->max_grp3d_freq);
	}

	if (pdata->imem_clk_name != NULL) {
		clk = clk_get(&pdev->dev, pdata->imem_clk_name);
		if (IS_ERR(clk)) {
			result = PTR_ERR(clk);
			KGSL_DRV_ERR("clk_get(%s) returned %d\n",
				pdata->imem_clk_name, result);
			goto done;
		}
		kgsl_driver.imem_clk = clk;
	}

#ifdef CONFIG_MSM_KGSL_2D
	clk = clk_get(&pdev->dev, "grp_2d_pclk");
	if (IS_ERR(clk))
		clk = NULL;
	kgsl_driver.g12_grp_pclk = clk;

	if (pdata->grp2d_clk_name != NULL) {
		clk = clk_get(&pdev->dev, pdata->grp2d_clk_name);
		if (IS_ERR(clk)) {
			clk = NULL;
			result = PTR_ERR(clk);
			KGSL_DRV_ERR("clk_get(%s) returned %d\n",
				pdata->grp2d_clk_name, result);
		}
	} else {
		clk = NULL;
	}
	kgsl_driver.g12_grp_clk = clk;
#else
	kgsl_driver.g12_grp_clk = NULL;
	kgsl_driver.g12_grp_pclk = NULL;
#endif

	if (pdata != NULL && clk != NULL) {
		if ((pdata->set_grp2d_async != NULL) &&
			(pdata->max_grp2d_freq) &&
			(!pdata->set_grp2d_async()))
			clk_set_min_rate(clk, pdata->max_grp2d_freq);
	}

	kgsl_driver.power_flags = 0;

	if (pdata) {
		kgsl_driver.clk_freq[KGSL_AXI_HIGH_3D] = pdata->high_axi_3d;
		kgsl_driver.clk_freq[KGSL_AXI_HIGH_2D] = pdata->high_axi_2d;
		kgsl_driver.clk_freq[KGSL_2D_MIN_FREQ] = pdata->min_grp2d_freq;
		kgsl_driver.clk_freq[KGSL_2D_MAX_FREQ] = pdata->max_grp2d_freq;
		kgsl_driver.clk_freq[KGSL_3D_MIN_FREQ] = pdata->min_grp3d_freq;
		kgsl_driver.clk_freq[KGSL_3D_MAX_FREQ] = pdata->max_grp3d_freq;
	}

	pm_qos_add_requirement(PM_QOS_SYSTEM_BUS_FREQ, "kgsl_3d",
				PM_QOS_DEFAULT_VALUE);

	/* acquire yamato device and init only the fields required for open */
	if (kgsl_add_device(KGSL_DEVICE_YAMATO) == KGSL_FAILURE) {
		result = -EINVAL;
		goto done;
	} else {
		kgsl_driver.num_devs++;
	}

	/*acquire yamato interrupt */
	kgsl_driver.yamato_interrupt_num =
			platform_get_irq_byname(pdev, "kgsl_yamato_irq");

	if (kgsl_driver.yamato_interrupt_num <= 0) {
		KGSL_DRV_ERR("platform_get_irq_byname() returned %d\n",
			       kgsl_driver.yamato_interrupt_num);
		result = -EINVAL;
		goto done;
	}
	result = request_irq(kgsl_driver.yamato_interrupt_num, kgsl_yamato_isr,
				IRQF_TRIGGER_HIGH, DRIVER_NAME,
				kgsl_driver.devp[KGSL_DEVICE_YAMATO]);
	if (result) {
		KGSL_DRV_ERR("request_irq(%d) returned %d\n",
			      kgsl_driver.yamato_interrupt_num, result);
		goto done;
	}
	kgsl_driver.yamato_have_irq = 1;
	disable_irq(kgsl_driver.yamato_interrupt_num);

	if (kgsl_driver.g12_grp_clk) {
		/* acquire g12 device and init dev struct fields */
		if (kgsl_add_device(KGSL_DEVICE_G12) == KGSL_FAILURE) {
			result = -EINVAL;
			goto done;
		} else {
			kgsl_driver.num_devs++;
		}

		/*acquire g12 interrupt */
		kgsl_driver.g12_interrupt_num =
			platform_get_irq_byname(pdev, "kgsl_g12_irq");

		if (kgsl_driver.g12_interrupt_num <= 0) {
			KGSL_DRV_ERR("platform_get_irq_byname() returned %d\n",
						kgsl_driver.g12_interrupt_num);
			result = -EINVAL;
			goto done;
		}
		result = request_irq(kgsl_driver.g12_interrupt_num,
					kgsl_g12_isr,
					IRQF_TRIGGER_HIGH,
					DRIVER_NAME,
					kgsl_driver.devp[KGSL_DEVICE_G12]);
		if (result) {
			KGSL_DRV_ERR("request_irq(%d) returned %d\n",
				      kgsl_driver.g12_interrupt_num, result);
			goto done;
		}
		kgsl_driver.g12_have_irq = 1;
		disable_irq(kgsl_driver.g12_interrupt_num);

		/* g12 config */
		pm_qos_add_requirement(PM_QOS_SYSTEM_BUS_FREQ, "kgsl_2d",
				PM_QOS_DEFAULT_VALUE);
		result = kgsl_g12_config(&kgsl_driver.g12_config, pdev);
		if (result != 0) {
			KGSL_DRV_ERR("kgsl_g12_config returned error=%d\n",
				      result);
			goto done;
		}
	}

	/* yamato config */
	result = kgsl_yamato_config(&kgsl_driver.yamato_config, pdev);
	if (result != 0)
		goto done;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM,
					   "kgsl_phys_memory");
	if (res == NULL) {
		result = -EINVAL;
		goto done;
	}

	kgsl_driver.shmem.physbase = res->start;
	kgsl_driver.shmem.size = resource_size(res);

	/* init memory apertures */
	result = kgsl_sharedmem_init(&kgsl_driver.shmem);
	if (result != 0)
		goto done;

	result = kgsl_drm_init(pdev);

	INIT_LIST_HEAD(&kgsl_driver.pagetable_list);
	mutex_init(&kgsl_driver.pt_mutex);

done:
	if (result)
		kgsl_driver_cleanup();
	else
		result = kgsl_register_dev(kgsl_driver.num_devs);

	if (!result)
		KGSL_DRV_DBG("platform probe successful, numdevs=%d\n",
						kgsl_driver.num_devs);
	return result;
}

static int kgsl_platform_remove(struct platform_device *pdev)
{

	kgsl_driver_cleanup();
	kgsl_drm_exit();
	kgsl_device_unregister();

	return 0;
}

static struct platform_driver kgsl_platform_driver = {
	.probe = kgsl_platform_probe,
	.remove = __devexit_p(kgsl_platform_remove),
	.suspend = kgsl_suspend,
	.resume = kgsl_resume,
	.driver = {
		.owner = THIS_MODULE,
		.name = DRIVER_NAME
	}
};

static int __init kgsl_mod_init(void)
{
	return platform_driver_register(&kgsl_platform_driver);
}

static void __exit kgsl_mod_exit(void)
{
	platform_driver_unregister(&kgsl_platform_driver);
}

#ifdef MODULE
module_init(kgsl_mod_init);
#else
late_initcall(kgsl_mod_init);
#endif
module_exit(kgsl_mod_exit);

MODULE_DESCRIPTION("Graphics driver for QSD8x50, MSM7x27, and MSM7x30");
MODULE_VERSION("1.1");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:kgsl");
