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
 * @version	LinuStoreIII_1.2.0_b035-FSR_1.2.1p1_b129_RC
 * @file	drivers/fsr/stl_blkdev.c
 * @brief	This file is STL I/O part which supports linux kernel 2.6
 *		It provides (un)registering block devices and reqeust handler
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <FSR.h>
#include <FSR_OAM.h>

#include "fsr_base.h"

#define DEVICE_NAME		"stl"
#define MAJOR_NR		BLK_DEVICE_STL

extern VOID memcpy32(VOID *pDst, VOID *pSrc, UINT32 nSize);

static DECLARE_MUTEX(stl_list_mutex);
static LIST_HEAD(stl_list); 


#if defined(FSR_DUP_BUFFER)
#define FSR_DUP_MAX_SECTOR	128
#define FSR_DUP_SECTOR_SIZE	512
#define FSR_DUP_BUFFER_SIZE	(FSR_DUP_MAX_SECTOR * FSR_DUP_SECTOR_SIZE / sizeof(UINT32))

static UINT32	xBuffer[FSR_DUP_BUFFER_SIZE];
static UINT8*	yBuffer = (UINT8 *) xBuffer;

#ifdef __LINUSTORE_INTERNAL_IO_PARTTERN__
extern void stl_count_layer(unsigned long sector, int num_sectors);
#endif

/**
 * copy and transfer to the STL
 * @param volume	volume(device) number
 * @param part_id	partition id
 * @param sector	start sector number
 * @param nsect		number of sectors
 * @param buf		pointer to the data
*/
static int _fsr_dup_write(u32 volume, u32 part_id, unsigned long sector, 
			unsigned long nsect, char *buf)
{
	int ret;
	unsigned long sector_to_write;
	unsigned long buffer_length;

	// write nsect of sectors 
	sector_to_write = FSR_DUP_MAX_SECTOR;
	buffer_length = FSR_DUP_MAX_SECTOR << 9;

	while (nsect > 0)
	{
		if (nsect < FSR_DUP_MAX_SECTOR)
		{
			sector_to_write = nsect;
			buffer_length = nsect << 9;
		}

		memcpy32(yBuffer, buf, buffer_length);
		ret = FSR_STL_Write(volume, part_id, sector, sector_to_write, 
					yBuffer, FSR_STL_FLAG_USE_SM);
		if (FSR_STL_SUCCESS != ret)
		{
			ERRPRINTK("stl: transfer error=%x\n", ret);
			return -1;
		}

		nsect -= sector_to_write;
		sector += sector_to_write;
		buf += buffer_length;
	}

	return FSR_STL_SUCCESS;
}

#endif // end of #if defined(FSR_DUP_BUFFER)

/**
 * transfer data from STL to block device
 * @param volume 	volume(device) number 
 * @param partno 	partition number
 * @param req           request to perfrom
 * @return		0 on success, ErrorNo on failure 
 * @remark		You can find ErrorNo on driver/fsr/Inc/STL.h
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
static int stl_transfer(u32 volume, u32 partno, const struct request *req, u32 data_len)
#else
static int stl_transfer(u32 volume, u32 partno, const struct request *req)
#endif
{
	unsigned long sector, nsect;
	char *buf;
	u32 part_id;
	int ret;

	DEBUG(DL3,"STL[I]: volume(%d), partno(%d)\n", volume, partno);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
	sector = blk_rq_pos(req);
	nsect = data_len >> 9;
#else
	sector = req->sector;
	nsect = req->current_nr_sectors;
#endif
	buf = req->buffer;

	if (!blk_fs_request(req))
	{
		ERRPRINTK("Invalid Request type from file system\n");
		return 0;
	}

	part_id = fsr_part_id(fsr_get_part_spec(volume), partno);

#ifdef	__LINUSTORE_INTERNAL_BLOCK_IO__	
	if (por_enable == 1)
		return 0;
#endif
	switch (rq_data_dir(req)) 
	{
		case READ:
			FSR_DOWN(&fsr_mutex);
			ret = FSR_STL_Read(volume, part_id, sector, nsect, buf, FSR_STL_FLAG_USE_SM);
			FSR_UP(&fsr_mutex);
			break;
	
		case WRITE:
			FSR_DOWN(&fsr_mutex);
	
#if defined(FSR_DUP_BUFFER)
			ret = _fsr_dup_write(volume, part_id, sector, nsect, buf);
#else
			ret = FSR_STL_Write(volume, part_id, sector, nsect, buf, FSR_STL_FLAG_USE_SM);
#endif
	


			FSR_UP(&fsr_mutex);
			break;
		
		default:
			ERRPRINTK("Unknown request 0x%x\n", (u32) rq_data_dir(req));
			return -1;
	}

	/* I/O error */
	if (ret != FSR_STL_SUCCESS) 
	{
		ERRPRINTK("STL: fsr block device transfer error =%x\n", ret);
		return 0;
	}
#if defined(CONFIG_LINUSTOREIII_DEBUG) && defined(CONFIG_PROC_FS)
	stl_count_iostat(nsect, rq_data_dir(req));
#endif

	DEBUG(DL3,"STL[O]: volume(%d), partno(%d)\n", volume, partno);

	return 1;
}

/**
 * request function
 * @param rq		reqeust queue
 * @return		none
 * @remark		req->bio->bi_private:  private fsr requests from RFS
 */
static void stl_request(struct request_queue *rq)
{
	u32 minor, volume, partno;
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

	DEBUG(DL3,"STL[I]\n");

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
#if LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 25)
		ret = 0;
#endif
		trans_ret = 1;
		minor = dev->gd->first_minor;
		volume = fsr_vol(minor);
		partno = fsr_part(minor);
		if (fsr_is_whole_dev(partno))
			goto end_req;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
		len = blk_rq_cur_bytes(req);
		if(blk_rq_cur_sectors(req) != blk_rq_sectors(req))
		{
			blk_rq_map_sg(rq, req, dev->sg);
			len = dev->sg->length;
			if (len > blk_rq_bytes(req))
			{
				len = blk_rq_bytes(req);
			}
		}
		trans_ret = stl_transfer(volume, partno, req, len);

#else
		if(req->current_nr_sectors != req->nr_sectors) 
		{
			blk_rq_map_sg(rq, req, dev->sg);
			req->current_nr_sectors = dev->sg->length >> SECTOR_BITS;
			if(req->current_nr_sectors > req->nr_sectors)
			{
				req->current_nr_sectors = req->nr_sectors;
			}
		}
		trans_ret = stl_transfer(volume, partno, req);
#endif
end_req:
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
		if(!ret) 
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

	DEBUG(DL3,"STL[O]\n");
}

/**
 * Delete the disk
 * @param dev		structure for block device in kernel 2.6
 * @return		none
 */
static void stl_del_disk(struct fsr_dev *dev)
{
	DEBUG(DL3,"STL[I]\n");

	if (dev->gd) 
	{
		del_gendisk(dev->gd);
		put_disk(dev->gd);
	}
	else
	{
		ERRPRINTK("No gendisk in DEV\n");
	}
	kfree(dev->sg);
	if (dev->queue) 
		blk_cleanup_queue(dev->queue);
	list_del(&dev->list);
	kfree(dev);

	DEBUG(DL3,"STL[O]\n");

	return;
}

/**
 * Register the each partition as a sigle disk
 * @param volume	bundle of same deivces 
 * @param partno	partition number
 * @return		0 on success, -ENOMEM on failure
 */
static int stl_add_disk(u32 volume, u32 partno)
{
	u32 minor, sizes;
	stl_info_t *stl_info;
	struct fsr_dev *dev;

	DEBUG(DL3,"STL[I]: volume(%d), partno(%d)", volume, partno);

	dev = kmalloc(sizeof(struct fsr_dev), GFP_KERNEL);
	/* memory error */
	if (!dev)
	{
		ERRPRINTK("STL: fsr_dev malloc fail\n");
		return -ENOMEM;
	}
	memset(dev, 0, sizeof(struct fsr_dev));

	spin_lock_init(&dev->lock);
	INIT_LIST_HEAD(&dev->list);
	down(&stl_list_mutex);
	list_add(&dev->list, &stl_list);
	up(&stl_list_mutex);

	/* init queue */
	dev->queue = blk_init_queue(stl_request, &dev->lock);
	dev->queue->queuedata = dev;
	dev->req = NULL;

	/* alloc scatterlist */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
	dev->sg = kmalloc(sizeof(struct scatterlist) * dev->queue->limits.max_phys_segments, GFP_KERNEL);
#else
	dev->sg = kmalloc(sizeof(struct scatterlist) * dev->queue->max_phys_segments, GFP_KERNEL);
#endif
	if(!dev->sg) 
	{
		kfree(dev);
		ERRPRINTK("STL: Scatter gather list malloc fail\n");
		return -ENOMEM;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 31)
	memset(dev->sg, 0, sizeof(struct scatterlist) * dev->queue->limits.max_phys_segments);
#else
	memset(dev->sg, 0, sizeof(struct scatterlist) * dev->queue->max_phys_segments);
#endif
	/* Each GBBM2 partition is a physical disk which has one partition */
	dev->gd = alloc_disk(1);
	/* memory error */
	if (!dev->gd) 
	{
		kfree(dev->sg);
		kfree(dev);
		ERRPRINTK("STL: Gendisk malloc fail\n");
		return -ENOMEM;
	}

	minor = fsr_minor(volume, partno);

	dev->gd->major = MAJOR_NR;
	dev->gd->first_minor = minor;
	dev->gd->fops = stl_get_block_device_operations();
	dev->gd->queue = dev->queue;
	snprintf(dev->gd->disk_name, 32, "%s%d", DEVICE_NAME, minor);
	/* setup block device parameter array */
	stl_info = fsr_get_stl_info(volume, partno);
	sizes = fsr_stl_sectors_nr(stl_info);
	set_capacity(dev->gd, sizes);

	add_disk(dev->gd);

	DEBUG(DL3,"STL[O]: volume(%d), partno(%d)", volume, partno);

	return 0;
}

/**
 * update the block device parameter
 * @param minor				minor number to update device
 * @param blkdev_size		it contains the size of all block-devices by 1KB
 * @param blkdev_blksize	it contains the size of logical block
 * @return					0 on success, -1 on failure
 */
int stl_update_blkdev_param(u32 minor, u32 blkdev_size, u32 blkdev_blksize)
{
	struct fsr_dev *dev;
	struct gendisk *gd = NULL;
	struct list_head *this, *next;
	unsigned int nparts = 0;
	FSRPartI *pi;
	u32 volume, partno;

	DEBUG(DL3,"STL[I]\n");

	down(&stl_list_mutex);
	list_for_each_safe(this, next, &stl_list) 
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
			} 
			else if (dev->gd->first_minor >
					(nparts + (volume << PARTITION_BITS))) 
			{
				/* update for removed disk */
				stl_del_disk(dev);
			}
		}
	}
	up(&stl_list_mutex);

	if (!gd) 
	{
		volume = fsr_vol(minor);
		partno = fsr_part(minor);
		if (stl_add_disk(volume, partno)) 
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

	DEBUG(DL3,"STL[O]\n");

	return 0;
}

/**
 * clean the block device
 * @param first_minor		first minor number of volume device
 * @param nparts			a number of partitions of volume device
 * @return					0 on success, -1 on failure
 */
void stl_blkdev_clean(u32 first_minor, u32 nparts)
{
	struct fsr_dev *dev;
	struct list_head *this, *next;

	DEBUG(DL3,"STL[I]\n");

	down(&stl_list_mutex);
	list_for_each_safe(this, next, &stl_list) 
	{
		dev = list_entry(this, struct fsr_dev, list);
		if ((dev->gd->first_minor < (first_minor + nparts)) &&
				(dev->gd->first_minor >= first_minor))
		{
			stl_del_disk(dev);
		}
	}
	up(&stl_list_mutex);

	DEBUG(DL3,"STL[O]\n");
}

/**
 * free device node
 * @return		none
 */
static void stl_blkdev_free(void)
{
	struct fsr_dev *dev;
	struct list_head *this, *next;

	down(&stl_list_mutex);
	list_for_each_safe(this, next, &stl_list) 
	{
		dev = list_entry(this, struct fsr_dev, list);
		stl_del_disk(dev);
	}
	up(&stl_list_mutex);
}

/**
 * create STL block devices as many as partitions
 * @return	0 on success
 * @pre 	If the device is not formatted as GBBM2, this is meanless
 */
static int stl_blkdev_create(void)
{
	u32 i, j, partno;
	int ret;
	FSRPartI *pi;

	DEBUG(DL3,"STL[I]\n");

	for (i = 0; i < FSR_MAX_VOLUMES; i++) 
	{
		ret = fsr_update_vol_spec(i);
		if (ret) 
		{
			ERRPRINTK("fsr_update_vol_spec Error\r\n");
			continue;
		}

		pi = fsr_get_part_spec(i);

		/*
		 * which are better auto or static?
		 */
		for (j = 0; j < MAX_FLASH_PARTITIONS; j++) 
		{
			partno = j;
			/* No more partition */
			if (partno >= fsr_parts_nr(pi) 
				|| fsr_part_id(pi, j) >= FSR_PARTID_DUMMY0)
			{
				break;
			}

			ret = stl_add_disk(i, partno);
			if (ret)
			{
				ERRPRINTK("stl_add_disk() fail\n");
				return ret;
			}
			/* keep going*/
		}
	}

	DEBUG(DL3,"STL[O]\n");

	return 0;
}

/**
 * initialize the stl devices
 * @return		0 on success
 */
int __init stl_blkdev_init(void)
{

	DEBUG(DL3,"STL[I]\n");

	if (register_blkdev(MAJOR_NR, DEVICE_NAME)) 
	{
		ERRPRINTK("unable to get major number %d\n", MAJOR_NR);
		return -EBUSY;
	}

	if (stl_blkdev_create()) 
	{
		unregister_blkdev(MAJOR_NR, DEVICE_NAME);
		ERRPRINTK("stl_blkdev_create error\n");
		return -ENOMEM;
	}

	DEBUG(DL3,"STL[O]\n");

	return 0;
}

/**
 * exit the stl devices, it will cleanup all block parameter about stl devices
 * @return		none
 */
void __exit stl_blkdev_exit(void)
{
	DEBUG(DL3,"STL[I]\n");

	stl_blkdev_free();
	unregister_blkdev(MAJOR_NR, DEVICE_NAME);

	DEBUG(DL3,"STL[O]\n");
}

MODULE_LICENSE("Samsung Proprietary");
MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("The kernel 2.6 block device interface for STL");
