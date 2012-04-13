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
 * @file	drivers/fsr/stl_block.c
 * @brief	This file is STL common part to adopt FSR in linux
 *		It provides block device operations
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/hdreg.h>
#include <linux/proc_fs.h>

#include <asm/errno.h>
#include <asm/uaccess.h>

#include "fsr_base.h"

static unsigned int stl_open_count[FSR_MAX_VOLUMES][MAX_FLASH_PARTITIONS] = {{0,},};

#define MAJOR_NR	BLK_DEVICE_BML

static u32 		STL_start_sector;
static u32 		STL_nsectors;
static u32 		STL_nTotal_sectors;

static int stl_setup_spec(u32 volume, u32 partno, stl_info_t *ssp)
{
	u32 tmp, len, part_id;
	int ret;

	DEBUG(DL3,"STL[I]: volume(%d), partno(%d)\n",volume, partno);

	part_id = fsr_part_id(fsr_get_part_spec(volume), partno);

	FSR_DOWN(&fsr_mutex);
	ret = FSR_STL_IOCtl(volume, part_id, FSR_STL_IOCTL_LOG_SECTS, NULL, 
			sizeof(u32), &tmp, sizeof(u32), &len); 
	FSR_UP(&fsr_mutex);

	/* I/O error */	
	if (ret != FSR_STL_SUCCESS) 
	{
		ERRPRINTK("FSR_STL_IOCtl error[0x%08x]\n", ret);
		return -EINVAL;
	}

	DEBUG(DL2,"volume(%d), partno(%d) total_sectors(%d)", volume, partno, tmp);
	ssp->total_sectors = tmp;

	FSR_DOWN(&fsr_mutex);
	ret = FSR_STL_IOCtl(volume, part_id, FSR_STL_IOCTL_PAGE_SIZE, NULL, 
			sizeof(u32), &tmp, sizeof(u32), &len); 
	FSR_UP(&fsr_mutex);

	/* I/O error */
	if (ret != FSR_STL_SUCCESS) 
	{
		ERRPRINTK("FSR_STL_IOCtl error[0x%08x]\n", ret);
		return -EINVAL;
	}

	DEBUG(DL2,"volume(%d), partno(%d) page_size(%d)", volume, partno, tmp);
	ssp->page_size = tmp;
	DEBUG(DL2,"total_sectors(%d), page_size(%d)",ssp->total_sectors, ssp->page_size);

	DEBUG(DL3,"STL[O]: volume(%d), partno(%d)\n",volume, partno);

	return 0;
}

/**
 * stl_do_ioctl - ioctl to format the device
 * @param volume        volume number
 * @param partno        partition number
 * @param cmd           IOCTL CMD for STL (include/linux/fsr_if.h)
 * @param arg           argument from user space
 * @return              0 on success, otherwise on failure
 */
static int stl_do_ioctl(u32 volume, u32 partno, u32 cmd, u32 arg)
{
	u32 part_id;
	int ret;
	
	FSRStlInfo info;
	stl_info_t *stl;
	FSRVolSpec *vs;
	FSRPartI *pi;
	vs = fsr_get_vol_spec(volume);
        pi = fsr_get_part_spec(volume);

	DEBUG(DL3,"STL[I]: volume(%d), partno(%d), cmd(%x)\n",volume, partno, cmd);
	
	part_id = fsr_part_id(fsr_get_part_spec(volume), partno);

	switch (cmd) 
	{

		case STL_FORMAT:
		{
			u32 minor;				///< minor number
			FSRStlFmtInfo STLFmt;			///< stl format info passed to stl layer
			STL_FORMAT_INFO_T * stUsrSTLInfo;	///< temp structure

			stUsrSTLInfo = (STL_FORMAT_INFO_T *) kmalloc(sizeof(STL_FORMAT_INFO_T), GFP_KERNEL);
			if (!stUsrSTLInfo)
			{
				ERRPRINTK("kmalloc error\n");
				return -ENOMEM;
			}
			memset(stUsrSTLInfo, 0, sizeof(STL_FORMAT_INFO_T));

			if ( copy_from_user((char *)stUsrSTLInfo, (char *)arg, sizeof(STL_FORMAT_INFO_T)))
			{
				ERRPRINTK("Can not copry from user memory to stUsrSTLInfo.");
				return -EIO;
			}

			
			STLFmt.nOpt = FSR_STL_FORMAT_REMEMBER_ECNT;
			STLFmt.nAvgECnt = 0;
			STLFmt.nNumOfECnt = 0;
			STLFmt.pnECnt = NULL;
			
			// support for FSR_1.2.0 above
			if (stUsrSTLInfo->nNumOfInitFreeUnits > 1)
			{
				STLFmt.nOpt |= FSR_STL_FORMAT_SET_NUM_INITFREE;
				STLFmt.nNumOfInitFreeUnits = stUsrSTLInfo->nNumOfInitFreeUnits;
			}
			DEBUG(DL2,"STLFmt.nNumOfInitFreeUnits : %d\n", STLFmt.nNumOfInitFreeUnits);

			FSR_DOWN(&fsr_mutex);
			ret = FSR_STL_Format(volume, part_id, &STLFmt);
			FSR_UP(&fsr_mutex);
			
			if (ret != FSR_STL_SUCCESS)
			{
				ERRPRINTK("FSR_STL_Format Error in STL_FORMAT. [0x%08x]", ret);
				return -EIO;
			}

			/* update the STL instance */
			FSR_DOWN(&fsr_mutex);
			ret = FSR_STL_Open(volume, part_id, &info, FSR_STL_FLAG_DEFAULT);
			FSR_UP(&fsr_mutex);
			
			DEBUG(DL2,"STL_Open: SPU: %d, SECTS:%d",info.nLogSctsPerUnit , info.nTotalLogScts);
			if (ret != FSR_STL_SUCCESS)
			{
				ERRPRINTK("FSR_STL_Open Error in STL_FORMAT. [0x%08x]", ret);
				return -EIO;
			}
			
			stl = fsr_get_stl_info(volume, partno);
			stl_setup_spec(volume, partno, stl);
			STL_nTotal_sectors = fsr_stl_sectors_nr(stl);
			stUsrSTLInfo->nTotalSectors = STL_nTotal_sectors;
 
			ret = (int) copy_to_user((char *)arg, (char *)stUsrSTLInfo, sizeof(STL_FORMAT_INFO_T));
			if( ret < 0)
			{
				ERRPRINTK("Can not put stl total sectors to user variable [0x%08x]", ret);
				return -EIO;
			}
			
			
			FSR_DOWN(&fsr_mutex);
			ret = FSR_STL_Close(volume, part_id);
			FSR_UP(&fsr_mutex);

			if (ret != FSR_STL_SUCCESS)
			{
				ERRPRINTK("FSR_STL_Close Error in STL_FORMAT. [0x%08x]", ret);
			}

			minor = fsr_minor(volume, partno);
			ret = stl_update_blkdev_param(minor, fsr_stl_sectors_nr(stl) >> 0x1,
					fsr_stl_page_size(stl));
			if (ret != FSR_STL_SUCCESS)
			{
				ERRPRINTK("Can not update stl blkdev parameters. [0x%08x]", ret);
				return -ENODEV;
			}

			return ret;
		}

		case FSR_IS_WHOLE_DEV:
		{
			/* if (ret == 0) => partial device, else => whole partition */
			ret = fsr_is_whole_dev(partno);
			DEBUG(DL1, "fsr_is_whole_dev return value is [0x%x]\n", ret);
			return ret;
		}

		case FSR_GET_LS_VERSION:
		{
			ret = (int) copy_to_user((char *)((VERSION_CHECK_T *)arg)->driver_version, 
						(char *)&LINUSTOREIII_VERSION_STRING, 
						strlen(LINUSTOREIII_VERSION_STRING));

			return ret;
		}

		case FSR_GET_PART_TAB:
		{
			BML_PARTTAB_T *kparts;
			kparts = (BML_PARTTAB_T *) kmalloc(sizeof(BML_PARTTAB_T), GFP_KERNEL);
			if (!kparts)
			{
				ERRPRINTK("kmalloc error\n");
				return -ENOMEM;
			}
			memset(kparts, 0, sizeof(BML_PARTTAB_T));

			fsr_read_partitions(volume, kparts);

			ret = (int) copy_to_user((char *)arg, (char *)kparts,
								sizeof(BML_PARTTAB_T));
			/* memory error */
			if (ret < 0)
			{
				ERRPRINTK("copy_to_user error [0x%08x]", ret);
				kfree(kparts);
				return -EIO;
			}

			ret = fsr_update_vol_spec(volume);
			if (ret)
			{
				ERRPRINTK("fsr_update_vol_spec FAIL\r\n");
				kfree(kparts);
				return -EINVAL;
			}
			
			kfree(kparts);
			return ret;
		}

		case FSR_GET_PART_INFO:
		{
			BML_DEVINFO_T info;

			DEBUG(DL3, "IN FSR_GET_PART_INFO : 0x%x\n", cmd);
			if (fsr_is_whole_dev(partno))
			{
				info.num_units = fsr_vol_unit_nr(vs);
			}
			else
			{
				info.num_units = fsr_part_units_nr(pi, partno);
			}

			info.phy_unit_size = fsr_vol_unitsize(volume, partno);
			info.dump_size = vs->nSizeOfDumpBuf << 1;
			info.page_msize = fsr_vol_spp(vs) << SECTOR_BITS;

			DEBUG(DL3, "OUT FSR_GET_PART_INFO : 0x%x\n", cmd);
			return copy_to_user((char *) arg, (char *) &info,
					sizeof (BML_DEVINFO_T));
		}

		/* out-of-range input */
		default:
			return -EINVAL;
	}
		
	DEBUG(DL3,"STL[O]: volume(%d), partno(%d), cmd(%x)\n",volume, partno, cmd);

	return 0;

}

/**
 * STL raw block I/O control
 * @param inode block device inode
 * @param file	block device file
 * @param cmd	IOCTL command - ref. include/linux/fsr_if.h
 * @param arg	argument from user
 * @return	0 on success, otherwise on failure
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 28)
static int stl_block_ioctl(struct block_device *bdev, fmode_t mode,
			u32 cmd, unsigned long arg)
{
	u32 minor = MINOR(bdev->bd_dev);
#else
static int stl_block_ioctl(struct inode *inode, struct file *file, 
							unsigned cmd, unsigned long arg)
{
	u32 minor = MINOR(inode->i_rdev);
#endif
	u32 volume = fsr_vol(minor);
	u32 partno = fsr_part(minor);
	u32 part_id;
	int ret = 0;
	stl_info_t *stl;

	part_id = fsr_part_id(fsr_get_part_spec(volume), partno);

	DEBUG(DL3,"volume(%d), minor(%d), cmd(%x)\n", volume, minor, cmd);

	switch (cmd)
	{
		case STL_GET_NUM_OF_SECTORS:
		{
			DEBUG(DL2,"STL_GET_NUM_OF_SECTORS");
			stl = fsr_get_stl_info(volume, partno);
			stl_setup_spec(volume, partno, stl);
			STL_nTotal_sectors  = fsr_stl_sectors_nr(stl);

			DEBUG(DL2,"STL_nTotal_sectors : %d", STL_nTotal_sectors);

			return put_user(STL_nTotal_sectors, (u32 *)arg);
		}

		case STL_DUMP:
		{
			u8 *pBuf;

			// if read data exceed partition size,
			// read data size is set not to exceed partition size. 
			STL_nsectors = (MAX_STL_OPERATION_BUFFER_SIZE >> SECTOR_BITS);
			DEBUG(DL1,"MAX_STL_OPERATION_BUFFER_SIZE : %d, SECTOR_BITS : %d, STL_nsectors : %d\n", 
					MAX_STL_OPERATION_BUFFER_SIZE, SECTOR_BITS, STL_nsectors);
			if (STL_nTotal_sectors < (STL_start_sector + STL_nsectors))
			{
				STL_nsectors = STL_nTotal_sectors - STL_start_sector;
			}

			DEBUG(DL1,"part_id : %d, STL_start_sector : %d, STL_nsectors : %d\n", 
					part_id, STL_start_sector, STL_nsectors);

			if (STL_nsectors == 0)
			{
				ERRPRINTK("Can not access address\n");
				return -EFAULT;
			}

			// allocate buffer
			pBuf = kmalloc(MAX_STL_OPERATION_BUFFER_SIZE, GFP_KERNEL);
			if (pBuf == NULL)
			{
				ERRPRINTK("pBuf memory is not allocated\n");
				return -ENOMEM;
			}

			memset(pBuf, 0xFF, MAX_STL_OPERATION_BUFFER_SIZE);

			FSR_DOWN(&fsr_mutex);
			ret = FSR_STL_Read(volume, part_id, STL_start_sector, STL_nsectors, pBuf, FSR_STL_FLAG_DEFAULT);
			FSR_UP(&fsr_mutex);
			if ( ret != FSR_STL_SUCCESS )
			{
				ERRPRINTK("STL_Read Error in STL_DUMP [0x%08x]", ret);
				kfree(pBuf);
				return -EIO;
			}

			// copy to user memory
			ret = copy_to_user((char *)arg, (char *)pBuf, MAX_STL_OPERATION_BUFFER_SIZE);

			if (0 > ret)
			{
				ERRPRINTK("copy_to_user Error in STL_DUMP [0x%08x]", ret);
				kfree(pBuf);
				return -EIO;
			}

			STL_start_sector += STL_nsectors;

			kfree(pBuf);

			return FSR_STL_SUCCESS;
		}

		case STL_RESTORE:
		{
			u8 *pBuf;
	
			// if written data exceed partition size,
			// written data size is set not to exceed partition size.
			STL_nsectors = (MAX_STL_OPERATION_BUFFER_SIZE >> SECTOR_BITS);
			if (STL_nTotal_sectors < STL_start_sector + STL_nsectors)
			{
				STL_nsectors = STL_nTotal_sectors - STL_start_sector;
			}

			if (STL_nsectors == 0)
			{
				ERRPRINTK("Can not access address\n");
				return FSR_STL_SUCCESS;
			}

			// allocate buffer
			pBuf = kmalloc(MAX_STL_OPERATION_BUFFER_SIZE, GFP_KERNEL);
			if (pBuf == NULL)
			{
			    ERRPRINTK("pBuf memory is not allocated");
			    return -ENOMEM;
			}

			// copy from user memory
			if (copy_from_user((char *)pBuf, (char *)arg,
								MAX_STL_OPERATION_BUFFER_SIZE))
			{
				ERRPRINTK("copy_from_user Error in STL_RESTORE\n");
				kfree(pBuf);
				return -EIO;
			}

			FSR_DOWN(&fsr_mutex);
			ret = FSR_STL_Write(volume, part_id, STL_start_sector, STL_nsectors, pBuf, FSR_STL_FLAG_DEFAULT);
			FSR_UP(&fsr_mutex);
			if ( 0 > ret )
			{
				ERRPRINTK("STL_Write Error in STL_RESTORE. [0x%08x]\n", ret);
				kfree(pBuf);
				return -EIO;
			}

			STL_start_sector += STL_nsectors;

			kfree(pBuf);
			
			return FSR_STL_SUCCESS;
		}

		case STL_INIT_VARIABLES:
		{
			STL_start_sector = 0;
			STL_nsectors = 0;
			STL_nTotal_sectors = 0;

			return FSR_STL_SUCCESS;

		}

		case STL_CHANGE_PART_ATTR:
		{
			u32 nFlags;
			FSRChangePA *kparts;

			kparts = (FSRChangePA *) kmalloc(sizeof(FSRChangePA), GFP_KERNEL);
			if (!kparts)
			{
				ERRPRINTK("Can not alloc memory in STL_CHANGE_PART_ATTR cmd\n");
				return -ENOMEM;
			}
			memset(kparts, 0x0, sizeof(FSRChangePA));

			ret = (int) copy_from_user((char *)kparts, (char *)arg,
					sizeof(FSRChangePA));
			if (kparts->nNewAttr == FSR_BML_PI_ATTR_RW)
			{
				DEBUG(DL0, "FSR_STL_FLAG_OPEN_READWRITE\n");
				nFlags = FSR_STL_FLAG_OPEN_READWRITE;
			}
			else if (kparts->nNewAttr == FSR_BML_PI_ATTR_RO)
			{
				DEBUG(DL0, "FSR_STL_FLAG_OPEN_READONLY\n");
				nFlags = FSR_STL_FLAG_OPEN_READONLY;
			}
			else
			{
				ERRPRINTK("Invalid ATTR [New attribute : 0x%08x", kparts->nNewAttr);
				return -EIO;
			}
			DEBUG(DL2, "kparts->nPartID : %d, nFlags : 0x%08x\n", kparts->nPartID, nFlags);
			ret = FSR_STL_IOCtl(volume,
					kparts->nPartID,
					FSR_STL_IOCTL_CHANGE_PART_ATTR,
					&nFlags,
					sizeof(u32),
					NULL,
					0,
					NULL);

			/* I/O error */
			if (ret != FSR_STL_SUCCESS)
			{
				ERRPRINTK("FSR_STL_CHANGE_PART_ATTR Fail : 0x%x", ret);
				return -EIO;
			}

			ret = fsr_update_vol_spec(volume);
			if (ret)
			{
				ERRPRINTK("fsr_update_vol_spec FAIL\r\n");
				return -EINVAL;
			}

			return ret;
		}


		case STL_GET_MAJOR_NUMBER: 
		{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 28)
			u32 major_nr = MAJOR(bdev->bd_dev);
#else
			u32 major_nr = MAJOR(inode->i_rdev);
#endif
			DEBUG(DL3, "IN STL_GET_MAJOR_NUMBER : 0x%x", cmd);

			ret = put_user(major_nr, (u32 *)arg);
			/* memory error */
			if (ret)
			{
				ERRPRINTK("copy_to_user error [0x%08x]", ret);
				ret = -EFAULT;
				break;
			}

			DEBUG(DL3, "OUT STL_GET_MAJOR_NUMBER : 0x%x", cmd);
			return 0;
		}

		case STL_GET_MINOR_NUMBER:
		{
			u32 minor_nr = minor;
			if (minor_nr > MAX_FLASH_PARTITIONS)
			{
				minor_nr = minor - (0x1 << PARTITION_BITS);
			}
			DEBUG(DL3, "IN STL_GET_MINOR_NUMBER : 0x%x", cmd);
			
			ret = put_user(minor_nr, (u32 *)arg);
			/* memory error */
			if (ret)
			{
				ERRPRINTK("copy_to_user error [0x%08x]", ret);
				ret = -EFAULT;
				break;
			}

			DEBUG(DL3, "OUT BML_GET_MINOR_NUMBER : 0x%x", cmd);
			return 0;
		}

		case FSR_GET_LS_VERSION:
		case FSR_GET_PART_INFO:
		case FSR_GET_PART_TAB:
		case FSR_IS_WHOLE_DEV:
		{
			DEBUG(DL3, "volume : %d, partno : %d, command : 0x%0x", volume, partno, cmd);
			ret = stl_do_ioctl(volume, partno, cmd, arg);			
			return ret; 
		}

		
		/* out-of-range input */
		default:
		{
			return -EINVAL;
		}
	}

	return 0;
}

/**
 * STL block open interface
 * @return		0 on success, otherwise on failure
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 28)
static int stl_block_open(struct block_device *bdev, fmode_t mode)
{
	u32 volume, minor, partno, part_id;
	u32 ret;
	FSRStlInfo info;
	DECLARE_TIMER;

	minor = MINOR(bdev->bd_dev);
#else
static int stl_block_open(struct inode *inode, struct file *file)
{
	u32 volume, minor, partno, part_id;
	u32 ret;
	FSRStlInfo info;
	DECLARE_TIMER;

	minor = MINOR(inode->i_rdev);
#endif
	volume = fsr_vol(minor);
	partno = fsr_part(minor);

	DEBUG(DL3,"STL[I]: volume(%d), partno(%d)\n",volume, partno);

	if (volume >= FSR_MAX_VOLUMES)
	{
		ERRPRINTK("out of the volume number\n");
		return -ENODEV;
	}
	
	part_id = fsr_part_id(fsr_get_part_spec(volume), partno);

	if (fsr_is_whole_dev(partno))
	{
		ERRPRINTK("Invalid partition number\n");
		return -EINVAL;
	}

	START_TIMER();

	FSR_DOWN(&fsr_mutex);
	ret = FSR_STL_Open(volume, part_id, &info, FSR_STL_FLAG_DEFAULT);
	FSR_UP(&fsr_mutex);

	STOP_TIMER("STL_Open");
	if (ret == FSR_STL_PARTITION_ALREADY_OPENED) 
	{
		stl_open_count[volume][partno + 1]++;
		return 0;
	} 
	else if(ret != FSR_STL_SUCCESS) 
	{
		ERRPRINTK("STL: open error = %x\n", ret);
		return -EINVAL;
	}

	do 
	{
		stl_info_t *stl = fsr_get_stl_info(volume, partno);

		START_TIMER();
		stl_setup_spec(volume, partno, stl);
		STOP_TIMER("stl_setup_spec:");

		stl_update_blkdev_param(minor,
				fsr_stl_sectors_nr(stl) >> 0x1, 
				fsr_stl_page_size(stl));
	} while (0);

	DEBUG(DL3,"STL[O]: volume(%d), partno(%d)\n",volume, partno);

	return 0;
}

/**
 * STL block release interface
 * @return		0 on success, otherwise on failure
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 28)
static int stl_block_release(struct gendisk *disk, fmode_t mode)
{
	u32 volume, minor, partno, part_id;
	int ret;

	minor = disk->first_minor;
#else
static int stl_block_release(struct inode *inode, struct file *file)
{
	u32 volume, minor, partno, part_id;
	int ret;

	minor = MINOR(inode->i_rdev);
#endif
	volume = fsr_vol(minor);
	partno = fsr_part(minor);

	DEBUG(DL3,"STL[I]: volume(%d), partno(%d)\n",volume, partno);

	part_id = fsr_part_id(fsr_get_part_spec(volume), partno);

	if (fsr_is_whole_dev(partno))
		return -EINVAL;

	if (stl_open_count[volume][partno + 1] > 0) 
	{
		stl_open_count[volume][partno + 1]--;
		return 0;
	}

	FSR_DOWN(&fsr_mutex);
	ret = FSR_STL_Close(volume, part_id);
	FSR_UP(&fsr_mutex);

	if (ret != FSR_STL_SUCCESS) 
	{
		ERRPRINTK("FSR_STL_Close error[0x%08x]\n", ret);
		return -ENODEV;
	}

	DEBUG(DL3,"STL[O]: volume(%d), partno(%d)\n",volume, partno);

	return 0;
}

/**
 * STL common block device operations
 */
static struct block_device_operations stl_block_fops = 
{
	.owner		= THIS_MODULE,
	.open		= stl_block_open,
	.release	= stl_block_release,
	.ioctl		= stl_block_ioctl,
};

/**
 * return block device operations
 * @return	: STL common block device operations
 */
struct block_device_operations *stl_get_block_device_operations(void)
{
	return &stl_block_fops;
}

/**
 * Remove unnecessary STL map
 * @param dev		major, minor
 * @param start		start unit to delete
 * @param nums		numbers of unit to delete
 * @param b_size	unit size
 * @return		0 on success, otherwise on failure
 */
static int stl_delete(dev_t dev, u32 start, u32 nums, u32 b_size)
{
	u32 volume, partno, part_id, count;
	u32 minor = MINOR(dev);
	int ret;

	volume = fsr_vol(minor);
	partno = fsr_part(minor);

	DEBUG(DL3,"STL[I]: volume(%d), partno(%d)\n",volume, partno);

	count = b_size >> SECTOR_BITS;
	
	start = start * count;
	nums = nums * count;

	part_id = fsr_part_id(fsr_get_part_spec(volume), partno);

	FSR_DOWN(&fsr_mutex);
	ret = FSR_STL_Delete(volume, part_id, start, nums, FSR_STL_FLAG_USE_SM);
	FSR_UP(&fsr_mutex);

	DEBUG(DL2,"@: %d, %d - 0x%08x", start, nums, ret);

	/* I/O error */
	if (ret != FSR_STL_SUCCESS) 
	{
		ERRPRINTK("FSR_STL_Delete error[0x%08x]\n", ret);
		return -1;
	}

	DEBUG(DL3,"STL[O]: volume(%d), partno(%d)\n",volume, partno);

	return 0;
}

#if defined(CONFIG_LINUSTOREIII_DEBUG) && defined(CONFIG_PROC_FS)

/**
 * Open the STL device as default option
 * @param volume        volume number
 * @param partno        partition number
 * @return      0 on success, ohter on failure
 */
static int stl_open(u32 volume, u32 partno)
{
	int ret;
	u32 part_id;
	FSRStlInfo info;
	FSRPartI *pi = fsr_get_part_spec(volume);
	part_id = fsr_part_id(pi, partno);
	
	DEBUG(DL3,"STL[I]: volume(%d), partno(%d)\n",volume, partno);

	FSR_DOWN(&fsr_mutex);
	ret = FSR_STL_Open(volume, part_id, &info, FSR_STL_FLAG_DEFAULT);
	FSR_UP(&fsr_mutex);
	
	if (ret == FSR_STL_PARTITION_ALREADY_OPENED) 
	{
		DEBUG(DL0,"STL: Device is busy\n");
	} 
	else if (ret == FSR_STL_UNFORMATTED) 
	{
		DEBUG(DL0,"STL: Device is unformatted\n");
	} 
	else if (ret != FSR_STL_SUCCESS) 
	{
		DEBUG(DL0,"STL: Out of device : %X\n", ret);
	}
	
	DEBUG(DL3,"STL[O]: volume(%d), partno(%d)\n",volume, partno);

	return ret;
}

#include <linux/seq_file.h>

struct stl_ecount_info 
{
	int volume;
	int partno;
	int unit;
	int open;
};

static struct stl_ecount_info stl_proc_ecount = {-1, -1, 0, 0};

#define SET_STL_PROC_VAL(v, p, o) \
{								    \
	stl_proc_ecount.volume = v;     \
	stl_proc_ecount.partno = p;     \
	stl_proc_ecount.open = o;       \
	stl_proc_ecount.unit = 0;      \
}

/**
 * Close the STL device
 * @param volume        volume number
 * @param partno        partition number
 * @return      0 on success, ohter on failure
 */
static int stl_close(u32 volume, u32 partno)
{
	int ret;
	u32 part_id;
	FSRPartI *pi;
	
	DEBUG(DL3,"STL[I]: volume(%d), partno(%d)\n",volume, partno);

	pi = fsr_get_part_spec(volume);
	part_id = fsr_part_id(pi, partno);
	
	FSR_DOWN(&fsr_mutex);
	ret = FSR_STL_Close(volume, part_id);
	FSR_UP(&fsr_mutex);
	
	DEBUG(DL3,"STL[O]: volume(%d), partno(%d)\n",volume, partno);

	return ret;
}

/**
 * get a volume and partition number to show ecount
 * @param file          unused
 * @param buffer        user buffer
 * @param count         data len
 * @param data          unused
 */
#define MAX_TEMP_LEN 20
static ssize_t proc_stl_write(struct file *file, const char *buffer,
								size_t count, loff_t *ppos)
{
	char kbuf[MAX_TEMP_LEN + 1], *start, *end;
	int volume, partno, ret;
	
	if (count > MAX_TEMP_LEN) 
	{
		/* out-of-range input */
		ERRPRINTK("out of range input\n");
		return -EINVAL;
	}
	if (copy_from_user(&kbuf, buffer, count)) 
	{
		/* memory error */
		ERRPRINTK("Copy from user error\n");
		return -EFAULT;
	}
	kbuf[MAX_TEMP_LEN] = '\0';
	
	start = kbuf;
	end = strchr(kbuf, ' ');
	if (!end)
	{
		ERRPRINTK("stchar error\n");
		return -EINVAL;
	}
	
	volume = simple_strtol(start, &end, 10);
	start = end + 1;
	end = &kbuf[MAX_TEMP_LEN];
	partno = simple_strtol(start, &end, 10);
	partno --;
	
	if (volume >= FSR_MAX_VOLUMES || partno >= FSR_BML_MAX_PARTENTRY) 
	{
		/* out-of-range input */
		ERRPRINTK("out of device\n");
		return -EINVAL;
	}
	
	ret = stl_open(volume, partno);
	if (ret != FSR_STL_SUCCESS) 
	{
		SET_STL_PROC_VAL(-1, -1, 1);
		return count;
	}
	
	stl_close(volume, partno);
	SET_STL_PROC_VAL(volume, partno, 0);
	
	/* Find the cache in the chain of caches. */
	return count;
}

/**
 * initialize the sequentail file
 * @param m     sequential file
 * @param pos   the count for loop
 * @return      the pointer for first entry, NULL on error
 */
static void *stl_seq_start(struct seq_file *m, loff_t *pos)
{
	int ret, i;
	u32 part_id, nr_unit, *erase_unit, len;
	FSRPartI *pi;

	if (stl_proc_ecount.volume < 0 || stl_proc_ecount.partno < 0)
	{
		ERRPRINTK("out of input value\n");
		return NULL;
	}
	
	if (stl_proc_ecount.volume  >= FSR_MAX_VOLUMES ||
			stl_proc_ecount.partno >= MAX_FLASH_PARTITIONS)
	{	
		ERRPRINTK("out of input value\n");
		return NULL;
	}
	
	if (!stl_proc_ecount.open) 
	{
		if (stl_open(stl_proc_ecount.volume, stl_proc_ecount.partno))
		{
			ERRPRINTK("out of input value\n");
			return NULL;
		}
		stl_proc_ecount.open = 1;
		DEBUG(DL0,"%d th partition\nUnit  Erase Count\n", (stl_proc_ecount.partno + 1) & PARTITION_MASK);
	}

	pi = fsr_get_part_spec(stl_proc_ecount.volume);
	part_id = fsr_part_id(pi, stl_proc_ecount.partno);
	nr_unit = fsr_part_units_nr(pi, stl_proc_ecount.partno);
	erase_unit = FSR_OAM_Malloc(sizeof(u32) * nr_unit);

	do
	{
		FSR_DOWN(&fsr_mutex);
		ret = FSR_STL_IOCtl(stl_proc_ecount.volume, part_id, FSR_STL_IOCTL_READ_ECNT,
								NULL, 0,
								erase_unit, sizeof(u32) * nr_unit,
								&len);
		FSR_UP(&fsr_mutex);
		if (ret != FSR_STL_SUCCESS)
		{
			break;
		}

		for(i = 0; i < nr_unit; i++)
		{
			DEBUG(DL0,"%4d\t%4d\n", i, erase_unit[i] & (~FSR_STL_META_MARK));
		}
	} while(0);

	if(erase_unit)
	{
		FSR_OAM_Free(erase_unit);
	}

	stl_close(stl_proc_ecount.volume, stl_proc_ecount.partno);
	SET_STL_PROC_VAL(-1, -1, 0);

	return NULL;
}


/**
 * cleanup sequentail file
 * @param m		sequential file
 * @param p		entry
 * @return		NULL
 */
static void stl_seq_stop(struct seq_file *m, void *p)
{
	return;
}

/**
 * stlinfo_op
 */
static struct seq_operations stlinfo_op = 
{
	.start		= stl_seq_start,
	.stop		= stl_seq_stop,
};

static int proc_stl_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &stlinfo_op);
}

static struct file_operations proc_stl_operations = 
{
	.llseek		= seq_lseek,
	.read		= seq_read,
	.write		= proc_stl_write,
	.open		= proc_stl_open,
	.release	= seq_release,
};

static unsigned int (*stl_iostat)[IO_DIRECTION];


void stl_count_iostat(int num_sectors, int rw)
{
	if (rw) /* write? */
	{
		rw = 1;
	}
	if (num_sectors > BLK_DEF_MAX_SECTORS)
		num_sectors = BLK_DEF_MAX_SECTORS;
	/*
	 * Save value to "num_sectors - 1" array
	 * because array size is from 0 to BLK_DEF_MAX_SECTORS - 1
	 */
	stl_iostat[num_sectors - 1][rw] ++;
}

static int stl_show_iostat(char *page, char **start, off_t off,
						int count, int *eof, void *data)
{
	int i, j;
	unsigned int len = 0;
	
	for (i = 0; i < IO_DIRECTION; i++) 
	{
		len += snprintf(page + len, PAGE_SIZE - len, "* %s [number of sectors: counts]\n ",
						i ? "WRITE": "READ");
		for (j = 0; j < BLK_DEF_MAX_SECTORS; j++) 
		{
			if (stl_iostat[j][i] == 0)
			{
				continue;
			}
			/*
			 * "num_sectors - 1" array saved io count for num_sector.
			 */
			len += snprintf(page + len, PAGE_SIZE - len, "[%d: %d] ", j + 1, stl_iostat[j][i]);
		}
		len += snprintf(page + len, PAGE_SIZE - len, "\n");
	}
	

	len += snprintf(page + len, PAGE_SIZE - len, "\n");
	/* NOTE: This page is limited PAGE_SIZE - 80 */
	*eof = 1;
	return len;
}

static int stl_reset_iostat(struct file *file, const char *buffer,
				unsigned long count, void *data) 
{
	memset(stl_iostat, 0, sizeof(stl_iostat[IO_DIRECTION]) * BLK_DEF_MAX_SECTORS);


	DEBUG(DL2,"STL iostat is reseted!!!\n");
	return count;
}
#endif /* defined(CONFIG_LINUSTOREIII_DEBUG) && defined(CONFIG_PROC_FS) */

/**
 * STL block module init
 * @return	0 on success
 */
static int __init stl_block_init(void)
{
	int ret;

	DECLARE_TIMER;
	START_TIMER();

	DEBUG(DL3,"STL[I]\n");

	if((ret = FSR_STL_Init()) != FSR_STL_SUCCESS)
	{
		ERRPRINTK("FSR_STL_Init error[0x%08x]\n", ret);
		return -EINVAL;
	}

	STOP_TIMER("STL_Init");
	fsr_register_stl_ioctl(stl_do_ioctl);

#if defined(CONFIG_LINUSTOREIII_DEBUG) && defined(CONFIG_PROC_FS)
	do 
	{
		struct proc_dir_entry *entry;
		struct proc_dir_entry *stlinfo_entry = NULL;
		
		stlinfo_entry = create_proc_entry(STL_IOSTAT_PROC_NAME,
						S_IFREG | S_IWUSR | S_IRUGO,  fsr_proc_dir);
		if (stlinfo_entry) 
		{
			stlinfo_entry->read_proc = stl_show_iostat;
			stlinfo_entry->write_proc = stl_reset_iostat;
		}
		
		entry = create_proc_entry(FSR_PROC_STLINFO, 0, fsr_proc_dir);
		if (entry)
		{
			entry->proc_fops = &proc_stl_operations;
		}

		stl_iostat = kmalloc(sizeof(stl_iostat[IO_DIRECTION]) * BLK_DEF_MAX_SECTORS, GFP_KERNEL);
		if (stl_iostat == NULL)

		{
			ERRPRINTK("stl_iostat kmalloc error\n");
			return -EIO;
		}
		memset(stl_iostat, 0, sizeof(stl_iostat[IO_DIRECTION]) * BLK_DEF_MAX_SECTORS);

	} while (0);
#endif

	/* Register Block device by 2.4 or 2.6 IF*/
	if (stl_blkdev_init() == 0)
	{
		printk("FSR: Registered STL Driver.\n");
	}
	else
	{
		ERRPRINTK("FSR: Couldn't register STL Driver.\n");
	}

	sec_stl_delete = stl_delete;

	DEBUG(DL3,"STL[O]\n");

	return 0;
}

/**
 * STL block module exit
 * @return 		none
 */
static void __exit stl_cleanup(void)
{
	DEBUG(DL3,"STL[I]\n");

	fsr_unregister_stl_ioctl();

#if defined(CONFIG_LINUSTOREIII_DEBUG) && defined(CONFIG_PROC_FS)
	remove_proc_entry(STL_IOSTAT_PROC_NAME, fsr_proc_dir);
	remove_proc_entry(FSR_PROC_STLINFO, fsr_proc_dir);
#endif

	sec_stl_delete = NULL;
	stl_blkdev_exit();

	DEBUG(DL3,"STL[O]\n");
}

module_init(stl_block_init);
module_exit(stl_cleanup);

/**
 * STL symbols
 */
EXPORT_SYMBOL(FSR_STL_Open);
EXPORT_SYMBOL(FSR_STL_Close);
EXPORT_SYMBOL(FSR_STL_Write);
EXPORT_SYMBOL(FSR_STL_Read);
EXPORT_SYMBOL(FSR_STL_Delete);
EXPORT_SYMBOL(FSR_STL_IOCtl);

MODULE_LICENSE("Samsung Proprietary");
MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("STL common block device layer");

