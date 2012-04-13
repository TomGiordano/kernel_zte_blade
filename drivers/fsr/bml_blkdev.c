/*
 *---------------------------------------------------------------------------*
 *                                                                           *
 *               COPYRIGHT. SAMSUNG ELECTRONICS CO., LTD.                    *
 *                          ALL RIGHTS RESERVED                              *
 *                                                                           *
 *   Permission is hereby granted to licensees of Samsung Electronics Co.,   *
 *   Ltd. products to use this computer program only in accordance with the  *
 *   terms of the SAMSUNG FLASH MEMORY DRIVER SOFTWARE LICENSE AGREEMENT.    *
 *                                                                           *
 *---------------------------------------------------------------------------*
 */
/**
 * @version     LinuStoreIII_1.2.0_b035-FSR_1.2.1p1_b129_RC
 * @file        drivers/fsr/bml_block.c
 * @brief       This file is BML common part which supports the raw interface
 *              It provides block device operations and utilities
 *
 */


#include <linux/init.h>
#include <linux/fs.h>
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 15)
#include <linux/platform_device.h>
#else
#include <linux/device.h>
#endif

#include "fsr_base.h"

#define DEVICE_NAME		"bml"
#define MAJOR_NR		BLK_DEVICE_BML

#if defined(CONFIG_PM)
#include <FSR_BML.h>
#endif /* CONFIG_PM */

/**
 * list to keep track of each created block devices
 */
static DECLARE_MUTEX(bml_list_mutex);
static LIST_HEAD(bml_list);

/* These functions are called by tinybml when CONFIG_RFS_FSR is module */
#if defined(CONFIG_PM) && defined(CONFIG_TINY_FSR) && defined(CONFIG_RFS_FSR_MODULE)
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 15)
extern int (*bml_module_suspend)(struct platform_device *pdev, pm_message_t state);
extern int (*bml_module_resume)(struct platform_device *pdev);
#else
extern int (*bml_module_suspend)(struct device *dev, u32 state, u32 level);
extern int (*bml_module_resume)(struct device *dev, u32 level);
#endif
#endif /* CONFIG_PM && CONFIG_TINY_FSR && CONFIG_RFS_FSR_MODULE */

/**
 * transger data from BML to buffer cache
 * @param volume		: device number
 * @param partno		: 0~15: partition, other: whole device
 * @param req			: request description
 * @return			1 on success, 0 on failure
 *
 * It will erase a block before it do write the data
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
static int bml_transfer(u32 volume, u32 partno, const struct request *req, u32 data_len)
#else
static int bml_transfer(u32 volume, u32 partno, const struct request *req)
#endif
{
	unsigned long sector, nsect;
	char *buf;
	FSRVolSpec *vs;
	FSRPartI *ps;
	u32 nPgsPerUnit = 0, n1stVpn = 0, vun = 0, vsn = 0;
	u32 spu_shift, spp_shift, spp_mask;
	int ret;

	DEBUG(DL3,"BML[I] volume(%d), partno(%d)\n",volume, partno);

	if (!blk_fs_request(req))
	{
		ERRPRINTK("Invalid file system request\n");
		return 0;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
	sector = blk_rq_pos(req);
	nsect = data_len >> 9;
#else
	sector = req->sector;
	nsect = req->current_nr_sectors;
#endif
	buf = req->buffer;

	vs = fsr_get_vol_spec(volume);
	ps = fsr_get_part_spec(volume);
	spp_shift = ffs(vs->nSctsPerPg) - 1;
	spp_mask = vs->nSctsPerPg - 1;

	/**
	 *  Check the partition attr. and erase the partition. when command is WRITE
	 */

	// partial partition
	if(likely(!fsr_is_whole_dev(partno)))
	{
		if (FSR_BML_GetVirUnitInfo(volume, fsr_part_start(ps, partno),
					&n1stVpn, &nPgsPerUnit) != FSR_BML_SUCCESS)
		{
			ERRPRINTK("FSR_BML_GetVirUnitInfo FAIL\n");
			return -EIO;
		}
	
		// calculate virtual sector number
		vsn = (n1stVpn * vs->nSctsPerPg) + sector;
		// calculate sector per page shift variable
		spu_shift = ffs(nPgsPerUnit * vs->nSctsPerPg) - 1;

		// erase block if needed
		// only if command is WRITE and meet the first sector of unit
		if (unlikely(rq_data_dir(req) == WRITE &&
			((sector & MASK(spu_shift)) == 0 ||
			(sector >> spu_shift) != ((sector + nsect - 1) >> spu_shift))))
		{
			vun = fsr_part_start(ps, partno) + ((sector + nsect - 1) >> spu_shift);
			ret = FSR_BML_Erase(volume, &vun, 1, FSR_BML_FLAG_NONE);
			/* I/O error */
			if (ret != FSR_BML_SUCCESS) 
			{
				ERRPRINTK("BML: Erase error = %X\n", ret);
				return -EIO;
			}
		}
	}
	// whole deivce
	else
	{
		// If volume has Non-RW attribute partition, Can not write to.
		if (rq_data_dir(req) == WRITE)
		{
			u32 nPartIdx;

			for (nPartIdx = 0; nPartIdx < fsr_parts_nr(ps); ++nPartIdx)
			{
				if (!(ps->stPEntry[nPartIdx].nAttr & FSR_BML_PI_ATTR_RW))
				{
					ERRPRINTK("Protected device");
					return -EIO;
				}
			}
		}

		// only if command is WRITE and meet ther first sector of volume 
		if (rq_data_dir(req) == WRITE && sector == 0)
		{
			u32 start_unit = 0, end_unit = fsr_vol_unit_nr(vs);

			while (start_unit < end_unit)
			{
				ret = FSR_BML_Erase(volume, &start_unit, 1, 
									FSR_BML_FLAG_NONE);
				/* I/O error */
				if (ret != FSR_BML_SUCCESS)
				{
					ERRPRINTK("FSR_BML_Erase Fail : %x", ret);
					return -EIO;
				}

				++start_unit;
			}
		}
	}

	// Execute WRITE or READ command.
	switch (rq_data_dir(req)) 
	{
		case READ:
		/*
		 * If sector and nsect are aligned with vs->nSctsPerPg,
		 * you have to use a FSR_BML_Read() function using page unit,
		 * If not, use a FSR_BML_ReadScts() function using sector unit.
		 */
		if ((!(sector & spp_mask) && !(nsect & spp_mask))) 
		{
			ret = FSR_BML_Read(volume, n1stVpn + (sector >> spp_shift),
					nsect >> spp_shift, buf, NULL, FSR_BML_FLAG_ECC_ON);
		} 
		else 
		{
			ret = FSR_BML_ReadScts(volume, n1stVpn + (sector >> spp_shift),
					sector & spp_mask, nsect, buf, NULL, FSR_BML_FLAG_ECC_ON);
		}
		break;

		case WRITE:
			ret = FSR_BML_Write(volume, n1stVpn + (sector >> spp_shift), 
					nsect >> spp_shift, buf, NULL, FSR_BML_FLAG_ECC_ON);	
		break;

		default:
			ERRPRINTK("Unknown request 0x%x\n", (u32) rq_data_dir(req));
			return -EINVAL;
	}

	/* I/O error */
	if (ret != FSR_BML_SUCCESS) 
	{
		ERRPRINTK("BML: %s error = %X\n",
					(rq_data_dir(req) == READ) ? "read" : "write", ret);
		return -EIO;
	}

#if defined(CONFIG_LINUSTOREIII_DEBUG) && defined(CONFIG_PROC_FS)
	bml_count_iostat(nsect, rq_data_dir(req)); 
#endif

	DEBUG(DL3,"BML[O] volume(%d), partno(%d)\n",volume, partno);

	return 1;
}

/**
 * request function which is do read/write sector
 * @param rq	: request queue which is created by blk_init_queue()
 * @return				none
 */
static void bml_request(struct request_queue *rq)
{
	u32 minor, volume, partno, spp_mask;
	struct request *req;
	struct fsr_dev *dev;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 25)
	int ret;
#endif
	int trans_ret;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
	int error = 0;
	u32 len = 0;
#endif

	FSRVolSpec *vs;

	DEBUG(DL3,"BML[I]\n");

	dev = rq->queuedata;
	if (dev->req)
		return;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
	while ((dev->req = req = blk_peek_request(rq)) != NULL) 
#else
	while ((dev->req = req = elv_next_request(rq)) != NULL) 
#endif
	{
		spin_unlock_irq(rq->queue_lock);
		
		minor = dev->gd->first_minor;
		volume = fsr_vol(minor);
		partno = fsr_part(minor);
		vs = fsr_get_vol_spec(volume);
		spp_mask = vs->nSctsPerPg - 1;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
		len = blk_rq_cur_bytes(req);
		if ((rq_data_dir(req) == READ) && !( blk_rq_pos(req) & spp_mask) &&
			( blk_rq_cur_sectors(req) != blk_rq_sectors(req)))
		{
			blk_rq_map_sg(rq, req, dev->sg);
			if (!((dev->sg->length >> SECTOR_BITS) & 0x7))
			{
				len = dev->sg->length;
			}
			if (len > blk_rq_bytes(req))
			{
				len = blk_rq_bytes(req);
			}

		}
		trans_ret = bml_transfer(volume, partno, req, len);
#else
		if ((rq_data_dir(req) == READ) && !(req->sector & spp_mask) &&
			(req->current_nr_sectors != req->nr_sectors))
		{
			blk_rq_map_sg(rq, req, dev->sg);
			if (!((dev->sg->length >> SECTOR_BITS) & 0x7))
			{
				req->current_nr_sectors = dev->sg->length >> SECTOR_BITS;
			}
			if (req->current_nr_sectors > req->nr_sectors)
			{
				req->current_nr_sectors = req->nr_sectors;
			}
		}
		trans_ret = bml_transfer(volume, partno, req);
#endif		
		
		spin_lock_irq(rq->queue_lock);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31) 
		if (trans_ret)
		{
			error = 0;
		} else
		{
			error = -EIO;
		}
		/* don't need to check if request is finished */
		if (blk_rq_sectors(req) <= (len >> 9))
			list_del_init(&req->queuelist);
		__blk_end_request(req, error, len);

#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 25)
		req->hard_cur_sectors = req->current_nr_sectors;
		end_request(req, trans_ret);
#else
		ret = end_that_request_chunk(req, trans_ret, req->current_nr_sectors << SECTOR_BITS);
		if (!ret) 
		{
			add_disk_randomness(req->rq_disk);
			blkdev_dequeue_request(req);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 16)
			end_that_request_last(req, trans_ret);
#else
			end_that_request_last(req);
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 16) */
		}
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31) */
	}

	DEBUG(DL3,"BML[O]\n");
}

/**
 * add each partitions as disk
 * @param volume		a volume number
 * @param partno		a partition number
 * @return				0 on success, otherwise on error
 *
 */
static int bml_add_disk(u32 volume, u32 partno)
{
	u32 minor, sectors;
	struct fsr_dev *dev;
	FSRPartI *pi;
	
	DEBUG(DL3,"BML[I]: volume(%d), partno(%d)", volume, partno);

	dev = kmalloc(sizeof(struct fsr_dev), GFP_KERNEL);
	/* memory error */
	if (!dev)
	{
		ERRPRINTK("BML: fsr_dev malloc fail\n");
		return -ENOMEM;
	}
	memset(dev, 0, sizeof(struct fsr_dev));
	
	spin_lock_init(&dev->lock);
	INIT_LIST_HEAD(&dev->list);
	down(&bml_list_mutex);
	list_add(&dev->list, &bml_list);
	up(&bml_list_mutex);
	
	/* init queue */
	dev->queue = blk_init_queue(bml_request, &dev->lock);
	dev->queue->queuedata = dev;
	dev->req = NULL;

	/* alloc scatterlist */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
	dev->sg = kmalloc(sizeof(struct scatterlist) * dev->queue->limits.max_phys_segments, GFP_KERNEL);
#else
	dev->sg = kmalloc(sizeof(struct scatterlist) * dev->queue->max_phys_segments, GFP_KERNEL);
#endif	
	if (!dev->sg) 
	{
		kfree(dev);
		ERRPRINTK("BML: Scatter gather list malloc fail\r\n");
		return -ENOMEM;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
	memset(dev->sg, 0, sizeof(struct scatterlist) * dev->queue->limits.max_phys_segments);
#else
	memset(dev->sg, 0, sizeof(struct scatterlist) * dev->queue->max_phys_segments);
#endif	

	/* Each partition is a physical disk which has one partition */
	dev->gd = alloc_disk(1);
	/* memory error */
	if (!dev->gd) 
	{
		kfree(dev->sg);
		list_del(&dev->list);
		kfree(dev);
		ERRPRINTK("No gendisk in DEV\r\n");
		return -ENOMEM;
	}
	
	minor = fsr_minor(volume, partno);
	
	dev->gd->major = MAJOR_NR;
	dev->gd->first_minor = minor;
	dev->gd->fops = bml_get_block_device_operations();
	dev->gd->queue = dev->queue;
	
	pi = fsr_get_part_spec(volume);
	
	/* check minor number whether it is used for chip */
	if (minor & PARTITION_MASK) 
	{
		/* partition */
		snprintf(dev->gd->disk_name, 32, "%s%d", DEVICE_NAME, minor);
		sectors = (fsr_part_units_nr(pi, partno) *
					fsr_vol_unitsize(volume, partno)) >> 9;
	} 
	else 
	{
		/* chip */
		snprintf(dev->gd->disk_name, 32, "%s%d/%c", DEVICE_NAME,
					minor >> PARTITION_BITS, 'c');
		sectors = fsr_vol_sectors_nr(volume);
	}
	
	/* setup block device parameter array */
	set_capacity(dev->gd, sectors);
	
	add_disk(dev->gd);
	
	DEBUG(DL3,"BML[O]: volume(%d), partno(%d)", volume, partno);

	return 0;
}

/**
 * free all disk structure
 * @param *dev	fsr block device structure (ref. inlcude/linux/fsr/fsr_if.h)
 * @return		none
 *
 */
static void bml_del_disk(struct fsr_dev *dev)
{
	DEBUG(DL3,"BML[I]\n");

	if (dev->gd) 
	{
		del_gendisk(dev->gd);
		put_disk(dev->gd);
	}
	
	kfree(dev->sg);

	if (dev->queue)
	{
		blk_cleanup_queue(dev->queue);
	}

	list_del(&dev->list);
	kfree(dev);

	DEBUG(DL3,"BML[O]\n");
}

/**
 * update the block device parameter
 * @param minor				minor number to update device
 * @param blkdev_size			it contains the size of all block-devices by 1KB
 * @param blkdev_blksize		it contains the size of logical block
 * @return				0 on success, -1 on failure
 */
int bml_update_blkdev_param(u32 minor, u32 blkdev_size, u32 blkdev_blksize)
{
	struct fsr_dev *dev;
	struct gendisk *gd = NULL;
	struct list_head *this, *next;
	unsigned int nparts = 0;
	FSRPartI *pi;
	u32 volume, partno;
	
	down(&bml_list_mutex);
	list_for_each_safe(this, next, &bml_list) 
	{
		dev = list_entry(this, struct fsr_dev, list);
		if (dev && dev->gd && dev->gd->first_minor) 
		{
			volume = fsr_vol(dev->gd->first_minor);
			pi = fsr_get_part_spec(volume);
			nparts = fsr_parts_nr(pi);
			
			if (dev->gd->first_minor == minor) 
			{
				gd = dev->gd;
				break;
			} else if (dev->gd->first_minor >
						nparts + (volume << PARTITION_BITS)) 
			{
				/* update for removed disk */
				bml_del_disk(dev);
			}
		}
	}
	up(&bml_list_mutex);
	
	if (!gd) 
	{
		volume = fsr_vol(minor);
		partno = fsr_part(minor);
		if (bml_add_disk(volume, partno)) 
		{
			/* memory error */
			ERRPRINTK("gd updated failed, minor = %d\n", minor);
			return -1;
		}
	} 
	else 
	{
		/* blkdev_size use KB, set_capacity need numbers of sector */
		set_capacity(gd, blkdev_size << 1U);
	}
	
	return 0;
}


/**
 * bml_blkdev_create - create device node
 * @return	none
 */
static int bml_blkdev_create(void)
{
	u32 volume, partno, i;
	FSRPartI *pi;
	unsigned int nparts;
	int ret;
	
	DEBUG(DL3,"BML[I]\n");

	for (volume = 0; volume < FSR_MAX_VOLUMES; volume++) 
	{
		ret = FSR_BML_Open(volume, FSR_BML_FLAG_NONE);
		if (ret != FSR_BML_SUCCESS) 
		{
			ERRPRINTK("No such device or address, %d(0x%x)", volume, ret);
			continue;
		}

		ret = fsr_update_vol_spec(volume);
		if (ret) 
		{
			ERRPRINTK("fsr_update_vol_spec FAIL\r\n");
			FSR_BML_Close(volume, FSR_BML_FLAG_NONE);
			continue;
		}
		pi = fsr_get_part_spec(volume);
		nparts = fsr_parts_nr(pi);

		/*
		 * which is better auto or static?
		 */
		for (i = 0; i < FSR_BML_MAX_PARTENTRY + 1; i++) 
		{
			/* when partno has -1, it means whole device */
			partno = i - 1;
			/* there is no partition */
			if (i > nparts)
			{
				break;
			}
			
			ret = bml_add_disk(volume, partno);
			if (ret)
			{
				ERRPRINTK("bml_add_disk Fail : %d\r\n", ret);
				continue;
			}
		}
	}
	
	DEBUG(DL3,"BML[O]\n");

	return 0;
}

/**
 * free resource
 * @return				none
 * @remark bml_blkdev is the build-in code, so it will be never reached
 */
static void bml_blkdev_free(void)
{
	struct fsr_dev *dev;
	struct list_head *this, *next;
	
	DEBUG(DL3,"BML[I]\n");

	down(&bml_list_mutex);
	list_for_each_safe(this, next, &bml_list) 
	{
		dev = list_entry(this, struct fsr_dev, list);
		bml_del_disk(dev);
	}
	up(&bml_list_mutex);

	DEBUG(DL3,"BML[O]\n");
}

/*
 * suspend/resume is used below cases
 * Case 1: TinyFSR + FSR module
 * Case 2: FSR static
 */
#if defined(CONFIG_PM)
#if (defined(CONFIG_TINY_FSR) && defined(CONFIG_RFS_FSR_MODULE)) || defined(CONFIG_RFS_FSR)

/**
 * suspend the bml devices
 * @param dev           device structure
 * @param state         device power management state
 * @return              0 on success
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 15)
static int bml_suspend(struct platform_device *pdev, pm_message_t state)
#else
static int bml_suspend(struct device *dev, u32 state, u32 level)
#endif
{
	u32 ret = 0;
	u32 volume; 
	u32 nFlag = 0;		// for future use in FSR

	DEBUG(DL3,"BML[I]\n");

	/*
	 * Acquire BML Semaphore of all volumes
	 * to can not operate other operations in FlexOneNAND
	 */

	down(&bml_list_mutex);


	for (volume = 0; volume < FSR_MAX_VOLUMES; volume++) 
	{
		ret = FSR_BML_Suspend(volume, nFlag);
		if (ret != FSR_BML_SUCCESS)
		{
			ERRPRINTK("Can not execute FSR_BML_Suspend function [vol : %d, ret : 0x%x]\n", volume, ret);
			up(&bml_list_mutex);
			return -ENODEV;
		}
	}


	DEBUG(DL3,"BML[O]\n");

	return 0;
}

/**
 * resume the bml devices
 * @param dev           device structure
 * @return              0 on success
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 15)
static int bml_resume(struct platform_device *pdev)
#else
static int bml_resume(struct device *dev, u32 level)
#endif
{
	u32 volume, ret;

	DEBUG(DL3,"BML[I]\n");

	for (volume = 0; volume < FSR_MAX_VOLUMES; volume++) 
	{
		ret = FSR_BML_Resume(volume, FSR_BML_FLAG_RESTORE_BLOCK_STATE);
		if (ret != FSR_BML_SUCCESS) 
		{
			ERRPRINTK("Can not execute FSR_BML_Resume function [vol : %d, ret : 0x%x]\n",
				 volume, ret);
			return -ENODEV;
		}
	}


	up(&bml_list_mutex);

	DEBUG(DL3,"BML[O]\n");

	return 0;
}
#endif /* (CONFIG_RFS_FSR || CONFIG_RFS_FSR_MODULE) && CONFIG_TINY_FSR */
#endif /* CONFIG_PM */



/**
 * create device node, it will scan every chips and partitions
 * @return		0 on success, otherwise on error
 */
int __init bml_blkdev_init(void)
{
	DEBUG(DL3,"BML[I]\n");

	if (register_blkdev(MAJOR_NR, DEVICE_NAME)) 
	{
		ERRPRINTK("raw: unable to get major %d\n", MAJOR_NR);
		return -EAGAIN;
	}
	
	if (bml_blkdev_create()) 
	{
		unregister_blkdev(MAJOR_NR, DEVICE_NAME);
		ERRPRINTK("Can't created the BML block device\n");
		return -ENOMEM;
	}


	/* Register function pointer when CONFIG_RFS_FSR is module */
#if defined(CONFIG_PM) && defined(CONFIG_TINY_FSR) && defined(CONFIG_RFS_FSR_MODULE)
	bml_module_suspend = bml_suspend;
	bml_module_resume = bml_resume;
#endif
	
	DEBUG(DL3,"BML[O]\n");

	return 0;
}

/**
 * initialize the bml devices
 * @return				0 on success, otherwise on failure
 */
void __exit bml_blkdev_exit(void)
{
	int ret;
	unsigned int volume;

	DEBUG(DL3,"BML[I]\n");

	/*
	 * This FSR_BML_Close called at bml_blkdev_create() 
	 * It is called by the module exit
	 */
	for (volume = 0; volume < FSR_MAX_VOLUMES; volume++)
	{
		ret = FSR_BML_Close(volume, FSR_BML_FLAG_NONE);
		if (ret != FSR_BML_SUCCESS)
		{
			DEBUG(DL0,"Volume(%d) has not opened.\n", volume);
		}
	}


	bml_blkdev_free();
	unregister_blkdev(MAJOR_NR, DEVICE_NAME);

	/* Unregister function pointer when CONFIG_RFS_FSR is module */
#if defined(CONFIG_PM) && defined(CONFIG_TINY_FSR) && defined(CONFIG_RFS_FSR_MODULE)
	bml_module_suspend = NULL;
	bml_module_resume = NULL;
#endif

	DEBUG(DL3,"BML[O]\n");
}

MODULE_LICENSE("Samsung Proprietary");
MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("The kernel 2.6 block device interface for BML");

