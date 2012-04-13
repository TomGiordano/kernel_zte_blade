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
 * @file	drivers/fsr/bml_block.c
 * @brief	This file is BML common part which supports the raw interface
 *		It provides block device operations and utilities
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/bitops.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>

#include <asm/errno.h>
#include <asm/uaccess.h>

#include "fsr_base.h"

/**
 * Delcaration for getting a STL's ioctl
 */
static int (*stl_ioctl_func)(u32 volume, u32 partno, u32 cmd, u32 arg) = NULL;

/**
 * update the information about whole partitions on linux blkdev
 * @param volume	volume number
 * @return		0 on success, -1 on failure
 */
static int bml_update_whole_blkdev(u32 volume)
{
	int ret = 0;
	u32 i, minor, partsize, pagesize, blocksize;
	FSRVolSpec *vs = fsr_get_vol_spec(volume);
	FSRPartI *pi = fsr_get_part_spec(volume);

	/* update the block size */
	for (i = 0 ; i < pi->nNumOfPartEntry ; i++) 
	{
		minor = fsr_minor(volume, i);
		blocksize = fsr_vol_unitsize(volume, i) >> 10U; /* by 1KB */
		partsize = pi->stPEntry[i].nNumOfUnits * blocksize; /* by 1KB */
		pagesize = fsr_vol_spp(vs) << 9U;/* by Byte*/
		ret = bml_update_blkdev_param(minor, partsize, pagesize);
		if ( 0 != ret)
		{
			ERRPRINTK("bml_update_blkdev_param function fail");
		}
	}
	
	return ret;
}

/**
 * read/write partition to device from user's request
 * @param cmd			BML_SET_PART_TAB
 * @param volume		a volume number
 * @param *user_parts		the pointer of partition information came from user
 * @return			0 on success, otherwise on failure
 */
static int bml_rw_parts_from_user(u32 cmd, u32 volume, BML_PARTTAB_T *user_parts)
{
	int ret = -1;
	BML_PARTTAB_T *kparts;
	
	kparts = (BML_PARTTAB_T *) kmalloc(sizeof(BML_PARTTAB_T), GFP_KERNEL);
	if (!kparts)
	{
		ERRPRINTK("kmalloc error");
		return -ENOMEM;
	}

	memset(kparts, 0, sizeof(BML_PARTTAB_T));

	ret = (int) copy_from_user((char *)kparts, (char *)user_parts,
			sizeof(BML_PARTTAB_T));
	/* memory error */
	if (ret) 
	{
		ERRPRINTK("copy_from_user error");
		ret = -EFAULT;
		goto end_io;
	}

	ret = fsr_write_partitions(volume, kparts);
	/* I/O error */
	if (ret != FSR_BML_SUCCESS) 
	{
		ERRPRINTK("fsr_write_partition error");
		ret = -EIO;
		goto end_io;
	}

	ret = bml_update_whole_blkdev(volume);

end_io:
	kfree(kparts);
	return ret;
}


/**
 * dump/restore devcie at the request of user
 * @param cmd		BML_DUMP/BML_RESTORE, otherwise error
 * @param volume	a volume number
 * @param partno	a partition number
 * @param user_page	the pointer of page information and buffer
 * @return			0 on success, otherwise on failure
 */
static int bml_dump_from_user(u32 cmd, u32 volume, u32 partno, PAGEINFO_T *user_page)
{
	int ret = 0;
	unsigned int page;
	PAGEINFO_T *kpage;
	u32 nPgsPerUnit = 0, n1stVpn = 0;
	FSRVolSpec *vs = fsr_get_vol_spec(volume);
	FSRPartI *pi = fsr_get_part_spec(volume);

	kpage = (PAGEINFO_T *)kmalloc(sizeof(PAGEINFO_T), GFP_KERNEL);
	/* memory error */
	if (!kpage)
	{
		ERRPRINTK("kmalloc error error");
		return -ENOMEM;
	}
	
	do
	{
		ret = (int) copy_from_user(kpage, user_page, sizeof(PAGEINFO_T));
		/* memory error */
		if (ret) 
		{
			ERRPRINTK("copy_to_user error");
			ret = -EFAULT;
			break;
		}

		page = kpage->offset;

		if (!fsr_is_whole_dev(partno)) 
		{
			if (FSR_BML_GetVirUnitInfo(volume, fsr_part_start(pi, partno),
						&n1stVpn, &nPgsPerUnit) != FSR_BML_SUCCESS) 
			{
				ERRPRINTK("FSR_BML_GetVirUnitInfo FAIL");
				ret = -EIO;
				break;
			}

			/* out-of-range input */
			if (page >= (fsr_part_units_nr(pi, partno) * 
							fsr_part_ppu(vs, pi, partno))) 
			{
				ERRPRINTK("Out of range input value");
				ret = -EIO;
				break;
			}
		} 
		else 
		{
			/* out-of-range input */
			if (page >= fsr_vol_pages_nr(volume)) 
			{
				ERRPRINTK("Out of range input value");
				ret = -EIO;
				break;
			}
		}

		if (cmd == BML_DUMP) 
		{
			ret = FSR_BML_Read(volume, n1stVpn + page, 1, kpage->mbuf,
								NULL, 
								FSR_BML_FLAG_ECC_ON);
			/* I/O error */
			if (ret != FSR_BML_SUCCESS)
			{
				ERRPRINTK("ret: %x offset: %x page: %d",
							ret, kpage->offset, page);
				ret = -EIO;
				break;
			}
			ret = (int) copy_to_user(user_page, kpage, sizeof(PAGEINFO_T));
			/* memory error */
			if (ret)
			{
				ERRPRINTK("copy_to_user error");
				ret = -EFAULT;
				break;
			}
		}
		else if (cmd == BML_RESTORE) 
		{
			if(likely(!fsr_is_whole_dev(partno)))
			{
				/* unit start */
				if ((page & (fsr_part_ppu(vs, pi, partno) - 1)) == 0U)
				{
					u32 unit = fsr_part_start(pi, partno) +
						(page / fsr_part_ppu(vs, pi, partno));
					ret = FSR_BML_Erase(volume, &unit, 1, FSR_BML_FLAG_NONE);
					/* I/O error */
					if (ret != FSR_BML_SUCCESS)
					{
						ERRPRINTK("FSR_BML_Erase Error [0x%08x]", ret);
						ret = -EIO;
						break;
					}
				}
			}
			else
			{
				if (page == 0)
				{
					u32 start_unit = 0, end_unit = fsr_vol_unit_nr(vs);

					while (start_unit < end_unit)
					{
						ret = FSR_BML_Erase(volume, &start_unit, 1,
											FSR_BML_FLAG_NONE);
						/* I/O error */
						if (ret != FSR_BML_SUCCESS)
						{
							ERRPRINTK("FSR_BML_Erase Fail [0x%08x]", ret);
							return -EIO;
						}

						++start_unit;
					}
				}
			}

			ret = FSR_BML_Write(volume, n1stVpn + page, 1, kpage->mbuf,
					NULL, FSR_BML_FLAG_ECC_ON);
			/* I/O error */
			if (ret != FSR_BML_SUCCESS)
			{
				ERRPRINTK("FSR_BML_Write Error. offset: %x page: %d [0x%08x]",
						kpage->offset, page, ret);
				ret = -EIO;
				break;
			}
		}
		else /*unknown command*/
		{
			ret = -EINVAL;
			break;
		}

		ret = 0;
	} while(0);

	kfree(kpage);
	return ret;
}

/**
 * dump/restore devcie at the request of user
 * @param cmd		LLD_READ/LLD_WRITE, otherwise error
 * @param volume	a volume number
 * @param user_page	the pointer of page information and buffer
 * @return			0 on success, otherwise on failure
 */
static int lld_operation_from_user(u32 cmd, u32 volume, PAGEINFO_T *user_page)
{
	int ret;
	FSRDevSpec stDevSpec;
	PAGEINFO_T *kpage;
	FSRLowFuncTbl *gpstFSRLFT;
	gpstFSRLFT = fsr_get_low_tbl(volume);

	ret = gpstFSRLFT->LLD_GetDevSpec(0, &stDevSpec, 
												FSR_LLD_FLAG_NONE);
	if(ret != FSR_LLD_SUCCESS)
	{
		ERRPRINTK("LLD_GetDevSpec Fail [0x%08x]", ret);
		return -EIO;
	}

	kpage = (PAGEINFO_T *)kmalloc(sizeof(PAGEINFO_T), 
										GFP_KERNEL);
	if (kpage == NULL)
	{
		ERRPRINTK("kmalloc Fail [kmalloc return value : 0x%x]", ret);
		return -ENOMEM;
	}

	do
	{
		ret = (int) copy_from_user
					(kpage, user_page, sizeof(PAGEINFO_T));
		if (ret)
		{
			ret = -EFAULT;
			break;
		}
		if (cmd == LLD_BOOT_WRITE)
		{
			if (kpage->offset == 0)
			{
				int boot_blk = 0;

				ret = gpstFSRLFT->LLD_Erase(0, &boot_blk, 1, 
											FSR_LLD_FLAG_NONE);
				if (ret != FSR_LLD_SUCCESS)
				{
					ERRPRINTK("LLD_Erase Fail [0x%08x]", ret);
					ret = -EIO;
						break;
				}

				ret = gpstFSRLFT->LLD_FlushOp(0, 0, FSR_LLD_FLAG_NONE);
				if (ret != FSR_LLD_SUCCESS)
				{
					ERRPRINTK("LLD_FlushOp Fail [0x%08x]", ret);
					ret = -EIO;
					break;
				}
			}
			ret = gpstFSRLFT->LLD_Write(0, 0, kpage->offset,
										kpage->mbuf, NULL,
										FSR_LLD_FLAG_1X_PROGRAM |
										FSR_LLD_FLAG_ECC_ON);
			if (ret != FSR_LLD_SUCCESS)
			{
				ERRPRINTK("LLD_Write Fail [0x%08x]", ret);
				ret = -EIO;
				break;
			}

			ret = gpstFSRLFT->LLD_FlushOp(0, 0, FSR_LLD_FLAG_NONE);
			if (ret != FSR_LLD_SUCCESS)
			{
				ERRPRINTK("LLD_FlushOp Fail [0x%08x]", ret);
				ret = -EIO;
				break;
			}
		}
		else if (cmd == LLD_BOOT_READ)
		{
			ret = gpstFSRLFT->LLD_Read(0, 0, kpage->offset,
										kpage->mbuf, NULL,
										FSR_LLD_FLAG_TRANSFER |
										FSR_LLD_FLAG_ECC_ON);
			if (ret != FSR_LLD_SUCCESS)
			{
				ERRPRINTK("LLD_Read Fail [0x%08x]", ret);
				ret = -EIO;
				break;
			}

			ret = gpstFSRLFT->LLD_FlushOp(0, 0, FSR_LLD_FLAG_NONE);
			if (ret != FSR_LLD_SUCCESS)
			{
				ERRPRINTK("LLD_FlushOp Fail [0x%08x]", ret);
				ret = -EIO;
				break;
			}

			ret = (int) copy_to_user((char *)user_page, (char *)kpage,
										sizeof(PAGEINFO_T));
			/* memory error */
			if (ret)
			{
				ret = -EFAULT;
				break;
			}
		}

		ret = 0;
	} while(0);

	kfree(kpage);
	return ret;
}

static int
fsr_dump_from_user(u32 cmd, u32 volume, u32 partno, BML_DUMP_T *user_page)
{
	u8 *pBuf = 0;
	int ret = 0;
	u32 nDumpOrder, nPartIdx;
	u32 nNumOfBytes;
	FSRPartI *pi;
	FSRVolSpec *vs;
	BML_DUMP_T *kpage;
	FSRDumpPEList pstPEList;

	kpage = (BML_DUMP_T *) kmalloc(sizeof(BML_DUMP_T), GFP_KERNEL);
	if (kpage == NULL)
	{
		ERRPRINTK("kmalloc Fail");
		return -ENOMEM;
	}

	vs = fsr_get_vol_spec(volume);
	pi = fsr_get_part_spec(volume);

	do
	{
		ret = (int) copy_from_user((char *)kpage, (char *)user_page, sizeof(BML_DUMP_T));
		if (ret)
		{
			ret = -EFAULT;
			break;
		}
		
		pBuf = FSR_OAM_Malloc(vs->nSizeOfDumpBuf << 1);
		if (pBuf == NULL)
		{
			ERRPRINTK("FSR_OAM_Malloc Fail");
			ret = -EIO;
			break;
		}

		nDumpOrder = kpage->status;
		FSR_OAM_MEMSET((void *) &pstPEList, 0x00, sizeof(FSRDumpPEList));
		pstPEList.nNumOfPEntry = 0;
	
		for(nPartIdx = 0; nPartIdx < pi->nNumOfPartEntry ; nPartIdx++)
		{
			if (kpage->dump_parts[nPartIdx] == 1)
			{
				FSR_OAM_MEMCPY(&(pstPEList.stPEntry[pstPEList.nNumOfPEntry++]), &(pi->stPEntry[nPartIdx]), sizeof(FSRPartEntry));
			}
		}
	
		nNumOfBytes = 0;
		FSR_OAM_MEMSET(pBuf, 0x00, vs->nSizeOfDumpBuf);
		ret = FSR_BML_Dump(volume, FSR_DUMP_PARTITION, nDumpOrder, 
							&pstPEList,	pBuf, (u32 *) &nNumOfBytes);
		if ((ret != FSR_BML_DUMP_COMPLETE) && 
			(ret != FSR_BML_DUMP_INCOMPLETE))
		{
			ERRPRINTK("FSR_DUMP_PARTITION Fail [0x%08x]", 
						ret);
			ret = 1;
			break;
		}

		kpage->dump_size = nNumOfBytes;
		kpage->status = ret;

		ret = (int) copy_to_user((char *)user_page, (char *)kpage, sizeof(BML_DUMP_T));
		if (ret)
		{
			ERRPRINTK("copy_to_user");
			ret = -EFAULT;
			break;
		}

		ret = (int) copy_to_user((char *)user_page->dump_pBuf, (char *)pBuf, nNumOfBytes);
		if (ret)
		{
			ERRPRINTK("copy_to_user");
			break;
		}

		ret = 0;
	} while (0);

	if (pBuf)
	{
		FSR_OAM_Free(pBuf);
	}
	if (kpage)
	{
		kfree(kpage);
	}
	return ret;
}

/**
 * BML block open operation
 * @param inode		block device inode
 * @param file		block device file
 * @return			0 on success, otherwise on failure
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 28)
static int bml_block_open(struct block_device *bdev, fmode_t mode)
{
	u32 volume, minor;
	int ret;

	minor = MINOR(bdev->bd_dev);
#else
static int bml_block_open(struct inode *inode, struct file *file)
{
	u32 volume, minor;
	int ret;
	
	minor = MINOR(inode->i_rdev);
#endif
	volume = fsr_vol(minor);
	
	DEBUG(DL3,"BML[I]: volume(%d), minor(%d)", volume, minor);

	if (volume >= FSR_MAX_VOLUMES)
	{
		ERRPRINTK("Invalid volume");
		return -ENODEV;
	}
	
	ret = FSR_BML_Open(volume, FSR_BML_FLAG_NONE);
	
	if (ret != FSR_BML_SUCCESS) 
	{
		ERRPRINTK("BML: FSR_BML_Open error[0x%08x]", ret);
		return -ENODEV;
	}
	
	DEBUG(DL3,"BML[o]: volume(%d), minor(%d)", volume, minor);

	return 0;
}

/**
 * bml_block_release - BML block release operation
 * @param inode	block device inode
 * @param file	block device file
 * @return		0 on success
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 28)
static int bml_block_release(struct gendisk *disk, fmode_t mode)
{
	u32 volume, minor;
	minor = disk->first_minor;
#else
static int bml_block_release(struct inode *inode, struct file *file)
{
	u32 volume, minor;
	
	minor = MINOR(inode->i_rdev);
#endif
	volume = fsr_vol(minor);
	
	DEBUG(DL3,"BML[I]: volume(%d), minor(%d)", volume, minor);

	FSR_BML_Close(volume, FSR_BML_FLAG_NONE);
	
	DEBUG(DL3,"BML[O]: volume(%d), minor(%d)", volume, minor);
	return 0;
}

/**
 * otp_operation_from_user - BML block release operation
 * @param cmd	IOCTL command - ref. include/linux/fsr_if.h
 * @param volume	a volume number
 * @param user_page	user page information
 * @return		0 on success
 */
static int otp_operation_from_user(u32 cmd, u32 volume, OTP_PAGEINFO_T *user_page)
{
	s32 ret = 0;
	u32 page;
	u32 n1stVpn = 0;
	OTP_PAGEINFO_T *kpage;
	FSRVolSpec *vs;
	vs = fsr_get_vol_spec(volume);

	DEBUG(DL3, "IN IOCtl Command : 0x%x\n", cmd);

	/* OTP_PAGEINFO_T allocation */
	kpage = (OTP_PAGEINFO_T *)kmalloc(sizeof(OTP_PAGEINFO_T), GFP_KERNEL);
	if (kpage == NULL)
	{
		ERRPRINTK("kmalloc fail for OTP_PAGEINFO_T structure");
		return -ENOMEM;
	}

	do
	{
		ret = (int) copy_from_user(kpage, user_page, sizeof(OTP_PAGEINFO_T));
		/* memory error */
		if (ret)
		{
			ERRPRINTK("copy_to_user error");
			ret = -EFAULT;
			break;
		}

		page = kpage->offset;

		if (cmd == BML_OTP_READ)
		{
			/* OTP Block Read */
			ret = FSR_BML_OTPRead(volume, n1stVpn + page, 1, kpage->mbuf, NULL, FSR_BML_FLAG_NONE);
			/* I/O error */
			if (ret != FSR_BML_SUCCESS)
			{
				ERRPRINTK("ret: %x offset: %x page: %d", ret, kpage->offset, page);
				ret = -EIO;
				break;
			}

			ret = (int) copy_to_user(user_page, kpage, sizeof(OTP_PAGEINFO_T));
			/* memory error */
			if (ret)
			{
				ERRPRINTK("copy_to_user error");
				ret = -EFAULT;
				break;
			}
		}
		else if(cmd == BML_OTP_WRITE)
		{
			/* OTP Block Write */
			ret = FSR_BML_OTPWrite(volume, n1stVpn + page, 1, kpage->mbuf, NULL, FSR_BML_FLAG_NONE);
			/* I/O error */
			if (ret != FSR_BML_SUCCESS)
			{
				ERRPRINTK("ret: %x offset: %x page: %d", ret, kpage->offset, page);
				ret = -EIO;
				break;
			}
		}
		else
		{
			ERRPRINTK("Invalid IOCtl Command. [cmd : 0x%x]", cmd);
			ret = -EIO;
		}
	}
	while(0);

	DEBUG(DL3, "OUT IOCtl Command : 0x%x\n", cmd);

	return ret;
}

/**
 * BML raw block I/O control
 * @param inode	block device inode
 * @param file	block device file
 * @param cmd	IOCTL command - ref. include/linux/fsr_if.h
 * @param arg	arguemnt from user
 * @return		0 on success, otherwise on failure
 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 28)
static int bml_block_ioctl(struct block_device *bdev, fmode_t mode,
			unsigned cmd, unsigned long arg)
{
	u32 minor = MINOR(bdev->bd_dev);
#else
static int bml_block_ioctl(struct inode *inode, struct file *file,
							unsigned cmd, unsigned long arg)
{
	u32 minor = MINOR(inode->i_rdev);
#endif
	u32 volume = fsr_vol(minor);
	u32 partno = fsr_part(minor);
	int ret = 0;
	FSRVolSpec *vs;
	FSRPartI *pi;
	
	vs = fsr_get_vol_spec(volume);
	pi = fsr_get_part_spec(volume);
	
	DEBUG(DL3,"volume(%d), minor(%d), cmd(%x)\n", volume, minor, cmd);

	switch (cmd) 
	{
		case BML_DUMP:
		case BML_RESTORE:
		{
			DEBUG(DL3, "IN BML_DUMP : 0x%x\n", cmd);
			return bml_dump_from_user(cmd, volume, partno, (PAGEINFO_T *)arg);
		}
		case LLD_GET_DEV_INFO:
		{
			FSRLowFuncTbl *gpstFSRLFT;
			FSRDevSpec stDevSpec;
			LLD_DEVINFO_T info;
			DEBUG(DL3, "IN LLD_GET_DEV_INFO : 0x%x\n", cmd);
			if (!fsr_is_whole_dev(partno))
			{
				return -EINVAL;
			}

			gpstFSRLFT = fsr_get_low_tbl(volume);
			if (gpstFSRLFT == NULL)
			{
				ERRPRINTK("fsr_get_low_tbl Fail");
				return -EACCES;
			}

			ret = gpstFSRLFT->LLD_GetDevSpec(0, &stDevSpec, FSR_LLD_FLAG_NONE);
			if (ret != FSR_LLD_SUCCESS)
			{
				ERRPRINTK("LLD_GetDevSpec Fail [0x%08x]", ret);
				return -EIO;
			}
			
			info.msize_page = stDevSpec.nSctsPerPG << SECTOR_BITS;
			info.pages_blk = stDevSpec.nPgsPerBlkForSLC;
			info.msize_blk = stDevSpec.nPgsPerBlkForSLC * info.msize_page;

			DEBUG(DL3, "OUT LLD_GET_DEV_INFO : 0x%x\n", cmd);
			return copy_to_user((char *) arg, (char *) &info, sizeof (LLD_DEVINFO_T));
		}

		case LLD_BOOT_WRITE:
		case LLD_BOOT_READ:
		{
			DEBUG(DL3, "IN LLD_BOOT_WRITE : 0x%x\n", cmd);
			lld_operation_from_user(cmd, volume, (PAGEINFO_T *)arg);
			DEBUG(DL3, "OUT LLD_BOOT_WRITE : 0x%x\n", cmd);
			return 0;
		}

		case BML_SET_PART_TAB:
		{
			ret = bml_rw_parts_from_user(cmd, volume, (BML_PARTTAB_T *)arg);
			if (ret)
			{
				ERRPRINTK("bml_rw_parts_from_user function error [ret=%d]", ret);
				return -EINVAL;
			}

			ret = fsr_update_vol_spec(volume);
			if (ret)
			{
				ERRPRINTK("fsr_update_vol_spec FAIL");
				return -EINVAL;
			}

			return ret;
		}
	
		case BML_UNLOCK_ALL:
		{
			u32 len;
	
			ret = FSR_BML_IOCtl(volume, FSR_BML_IOCTL_UNLOCK_WHOLEAREA,
								NULL, 0, NULL, 0, &len);
			/* I/O error */
			if (ret != FSR_BML_SUCCESS)
			{
				ERRPRINTK("FSR_BML_IOCTL_UNLOCK_WHOLEAREA Fail [0x%08x]", ret);
				return -EIO;
			}
	
			return 0;
		}

		case BML_ERASE:
		{
			u32 end_unit, start_unit = 0;
			DEBUG(DL3, "IN BML_ERASE : 0x%x\n", cmd);
			
			if (fsr_is_whole_dev(partno))
			{
				end_unit = fsr_vol_unit_nr(vs);
			}
			else
			{
				start_unit = fsr_part_start(pi, partno);
				end_unit = start_unit + fsr_part_units_nr(pi, partno);
			}
			
			while(start_unit < end_unit)
			{
				ret = FSR_BML_Erase(volume, &start_unit, 1, 
									FSR_BML_FLAG_NONE);
				/* I/O error */
				if (ret != FSR_BML_SUCCESS)
				{
					ERRPRINTK("FSR_BML_Erase Fail : %x, volume : %d, start unit : %d", ret, volume, start_unit);
					return -EIO;
				}
	
				++start_unit;
			}
			
			DEBUG(DL3, "OUT BML_ERASE : 0x%x\n", cmd);
			return 0;
		}

		case BML_GET_MAJOR_NUMBER:
		{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 28)
			u32 major_nr = MAJOR(bdev->bd_dev);
#else
			u32 major_nr = MAJOR(inode->i_rdev);
#endif
			DEBUG(DL3, "IN BML_GET_MAJOR_NUMBER : 0x%x\n", cmd);

			ret = put_user(major_nr, (u32 *)arg);
			/* memory error */
			if (ret)
			{
				ERRPRINTK("copy_to_user error : %x\n", ret);
				ret = -EFAULT;
				break;
			}

			DEBUG(DL3, "OUT BML_GET_MAJOR_NUMBER : 0x%x\n", cmd);
			return 0;
		}

		case BML_GET_MINOR_NUMBER:
		{
			u32 minor_nr = minor;
			if (minor_nr > MAX_FLASH_PARTITIONS)
			{
				minor_nr = minor - (0x1 << PARTITION_BITS);
			} 
			DEBUG(DL3, "IN BML_GET_MINOR_NUMBER : 0x%x\n", cmd);

			ret = put_user(minor_nr, (u32 *)arg);		
			/* memory error */
			if (ret)
			{
				ERRPRINTK("copy_to_user error : %x\n", ret);
				ret = -EFAULT;
				break;
			}

			DEBUG(DL3, "OUT BML_GET_MINOR_NUMBER : 0x%x\n", cmd);
			return 0;
		}

		case BML_GET_PART_ATTR:
		{
			u32 attr = 0;

			if (fsr_is_whole_dev(partno))
			{
				return -EACCES;
			}

			attr = fsr_part_attr(pi, partno);
			ret = copy_to_user((char *) arg, (char *) &attr, sizeof (u32));

			return ret;
		}

		case BML_SET_PART_ATTR:
		{
			u32 attr = 0, len;
			FSRChangePA stChangePA;

			if (fsr_is_whole_dev(partno))
			{
				return -EACCES;
			}


			ret = copy_from_user((char *) &attr, (char *) arg, sizeof (u32));
			if (ret)
			{
				ERRPRINTK("ret = %x", ret);
				return ret;
			}

			stChangePA.nPartID = fsr_part_id(pi, partno);
			stChangePA.nNewAttr = attr;
			ret = FSR_BML_IOCtl(volume, FSR_BML_IOCTL_CHANGE_PART_ATTR,
					(UINT8 *) &stChangePA, sizeof(FSRChangePA), NULL, 0, &len);
			if (ret != FSR_BML_SUCCESS)
			{
				ERRPRINTK("ret = %x", ret);
				return -EIO;
			}

			ret = fsr_update_vol_spec(volume);
			if (ret)
			{
				ERRPRINTK("ret = %x", ret);
				return -EIO;
			}

			return 0;
		}

		case BML_OTP_READ:
		case BML_OTP_WRITE:
		{
			DEBUG(DL3, "IN OTP Read / Write function call : 0x%x\n", cmd);
			ret = otp_operation_from_user(cmd, volume, (OTP_PAGEINFO_T *)arg);
			if (ret && (ret != FSR_BML_SUCCESS))
			{
				ERRPRINTK("OTP Operation error. [ret : 0x%x\n", ret);
				return -EIO;
			}

			DEBUG(DL3, "OUT OTP Read / Write function call : 0x%x\n", cmd);
			return 0;
		}

		case BML_OTP_INFO:
		{
			OTP_PAGEINFO_T *kpage;
			u32 len;

			DEBUG(DL3, "IN OTP Info command : 0x%x\n", cmd);

			/* OTP_PAGEINFO_T allocation */
			kpage = (OTP_PAGEINFO_T *)kmalloc(sizeof(OTP_PAGEINFO_T), GFP_KERNEL);
			if (kpage == NULL)
			{
				ERRPRINTK("kmalloc fail for OTP_PAGEINFO_T structure\n");
				return -ENOMEM;
			}

			memset(kpage, 0x00, sizeof(OTP_PAGEINFO_T));

			ret = copy_from_user(kpage, (OTP_PAGEINFO_T *)arg, sizeof(OTP_PAGEINFO_T));
			if (ret)
			{
				ERRPRINTK(" copy from user error [ret = %x]", ret);
				return ret;
			}

			ret = FSR_BML_IOCtl(volume, FSR_BML_IOCTL_GET_OTP_INFO,
						NULL, 0, (UINT8*) &kpage->lock_flag, sizeof(kpage->lock_flag), &len);
			if (ret != FSR_BML_SUCCESS)
			{
				ERRPRINTK(" FSR_BML_INCtl function error [ret = %x]", ret);
				return ret;
			}

			DEBUG(DL1, "OTP Lock Info = %x\n", kpage->lock_flag);
			ret = copy_to_user((OTP_PAGEINFO_T *)arg, kpage, sizeof(OTP_PAGEINFO_T));
			if (ret)
			{
				ERRPRINTK(" copy to user error [ret = %x]", ret);
				return ret;
			}

			kfree(kpage);

			DEBUG(DL3, "OUT OTP Info command : 0x%x\n", cmd);
			return 0;
		}

		case BML_OTP_LOCK:
		{
			OTP_PAGEINFO_T *kpage;
			u32 OTP_Flags = 0;
			u32 len;

			DEBUG(DL3, "IN OTP Lock command : 0x%x\n", cmd);

			/* OTP_PAGEINFO_T allocation */
			kpage = (OTP_PAGEINFO_T *)kmalloc(sizeof(OTP_PAGEINFO_T), GFP_KERNEL);
			if (kpage == NULL)
			{
				ERRPRINTK("kmalloc fail for OTP_PAGEINFO_T structure\n");
				return -ENOMEM;
			}

			ret = copy_from_user((UINT8 *) kpage, (UINT8 *) arg, sizeof(OTP_PAGEINFO_T));
			if (ret)
			{
				ERRPRINTK(" copy from user error [ret = %x]", ret);
				return ret;
			}

			if (kpage->lock_flag == FSR_BML_OTP_LOCK_1ST_BLK)
			{
				/* first OTP Block flag */
				DEBUG(DL0, "Fist OTP Block flag\n");
				OTP_Flags |= FSR_BML_OTP_LOCK_1ST_BLK;
			}
			else if (kpage->lock_flag == FSR_BML_OTP_LOCK_OTP_BLK)
			{
				/* OTP Block flag */
				DEBUG(DL0, "OTP Block flag\n");
				OTP_Flags |= FSR_BML_OTP_LOCK_OTP_BLK;
			}
			else if (kpage->lock_flag == (FSR_BML_OTP_LOCK_OTP_BLK | FSR_BML_OTP_LOCK_1ST_BLK))
			{
				/* OTP Block flag */
				DEBUG(DL0, "OTP Block and First OTP Block flag\n");
				OTP_Flags |= FSR_BML_OTP_LOCK_1ST_BLK;
				OTP_Flags |= FSR_BML_OTP_LOCK_OTP_BLK;
			}
			else
			{
				DEBUG(DL0, "None\n");
			}

			ret = FSR_BML_IOCtl(volume, FSR_BML_IOCTL_OTP_LOCK,
						(UINT8 *) &OTP_Flags, sizeof(OTP_Flags), NULL, 0, &len);
			if (ret != FSR_BML_SUCCESS)
			{
				ERRPRINTK("FSR: FSR_BML_IOCtl OTP Lock Error [ret : 0x%x]\n", ret);
				return -EIO;
			}

			kfree(kpage);
			DEBUG(DL3, "OUT OTP Lock command : 0x%x\n", cmd);
			return 0;
		}

		case BML_FSR_DUMP:
		{
			DEBUG(DL3, "IN FSR_DUMP : 0x%x\n", cmd);
			DEBUG(DL3, "OUT FSR_DUMP : 0x%x\n", cmd);
			return fsr_dump_from_user(cmd, volume, partno, (BML_DUMP_T *)arg);
		}

		case STL_GET_MINOR_NUMBER:
		case STL_GET_NUM_OF_SECTORS:
		case STL_FORMAT:
		case FSR_IS_WHOLE_DEV:
		case FSR_GET_PART_INFO:	
		case FSR_GET_PART_TAB:
		case FSR_GET_LS_VERSION:
		{
			if ( cmd == STL_FORMAT )
			{
				DEBUG(DL3, "IN STL_FORMAT : 0x%x\n", cmd);
				if (fsr_is_whole_dev(partno))
				{
					return -EACCES;
				}

			}
			DEBUG(DL3, "IN stl_ioctl_func call : 0x%x\n", cmd);
			if (stl_ioctl_func == NULL) 
			{
				ERRPRINTK("FSR: Need STL Module\n");
				return -EINVAL;
			}

			ret = stl_ioctl_func(volume, partno, cmd, arg);
			/* I/O error */
			if (ret)
				ERRPRINTK("STL ioctl function error. [0x%08x]", ret);
			DEBUG(DL3, "OUT stl_ioctl_func call : 0x%x\n", cmd);
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
 * Register STL ioctl to do STL formating
 * @param func	function pointer to use STL ioctl
 * @return		none
 */
void fsr_register_stl_ioctl(int (*func) (u32, u32, u32, u32))
{
	stl_ioctl_func = func;
}

/**
 * Unregister STL ioctl
 * @return	none
 */
void fsr_unregister_stl_ioctl(void)
{
	stl_ioctl_func = NULL;
}

/**
 * FSR common block device operations
 */
static struct block_device_operations bml_block_fops = 
{
	.owner		= THIS_MODULE,
	.open		= bml_block_open,
	.release	= bml_block_release,
	.ioctl		= bml_block_ioctl,
};

/**
 * bml_get_block_device_ops
 * @return	FSR common block device operations
 */
struct block_device_operations *bml_get_block_device_operations(void)
{
	return &bml_block_fops;
}

#if defined(CONFIG_LINUSTOREIII_DEBUG) && defined(CONFIG_PROC_FS)
static unsigned int (*bml_iostat)[IO_DIRECTION];

void bml_count_iostat(int num_sectors, int rw)
{
	if (num_sectors > BLK_DEF_MAX_SECTORS)
	{
		num_sectors = BLK_DEF_MAX_SECTORS;
	}
	if (rw) /* write? */
	{
		rw = 1;
	}
	/* 
	 * Save value to "num_sectors - 1" array 
	 * because array size is from 0 to BLK_DEF_MAX_SECTORS - 1
	*/
	bml_iostat[num_sectors - 1][rw]++; 
}

static int bml_show_iostat(char *page, char **start, off_t off,
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
			if (bml_iostat[j][i] == 0)
			{
				continue;
			}
			/*
			 * "num_sectors - 1" array saved io count for num_sector.
			 */
			len += snprintf(page + len, PAGE_SIZE - len, "[%d: %d] ", j + 1, bml_iostat[j][i]);
		}
		len += snprintf(page + len, PAGE_SIZE - len, "\n");
	}
	
	len += snprintf(page + len, PAGE_SIZE - len, "\n");
	/* NOTE: This page is limited PAGE_SIZE - 80 */
	*eof = 1;
	return len;
}

int bml_reset_iostat(struct file *file, const char *buffer,
						unsigned long count, void *data) 
{
	memset(bml_iostat, 0, sizeof(bml_iostat[IO_DIRECTION]) * BLK_DEF_MAX_SECTORS);
	ERRPRINTK("BML iostat is reseted!!!\n");
	return count;
}

#if !defined(FSR_OAM_RTLMSG_DISABLE) || defined(FSR_OAM_DBGMSG_ENABLE)
static int fsr_set_debug_message(struct file *file, const char *buffer,
						unsigned long count, void *data) 
{
	char dbg_level[20];
	int ret;

	ret = copy_from_user(dbg_level, buffer, count);
	/* memory error */
	if (ret) 
	{
		ERRPRINTK("copy_from_user error\n");
		return -EFAULT;
	}

	dbg_level[count] = '\0';
	if (dbg_level[count - 1] == '\n')
	{
		dbg_level[count - 1] = '\0';
	}

	if (!strcmp(dbg_level, "0"))
	{
		FSR_DBG_UnsetAllDbgZoneMask();
		DEBUG(DL0,"FSR DbgMsg Disable\r\n");
	}
	else if (!strcmp(dbg_level, "1"))
	{
		FSR_DBG_SetDbgZoneMask(FSR_DBZ_DEFAULT);
		DEBUG(DL0,"FSR DbgMsg Default Enable\r\n");
	}
	else if (!strcmp(dbg_level, "2"))
	{
		FSR_DBG_UnsetAllDbgZoneMask();
		FSR_DBG_SetDbgZoneMask(FSR_DBZ_ALL_ENABLE);
		DEBUG(DL0,"FSR DbgMsg All Enable\r\n");
	}
	else
	{
		DEBUG(DL0,"Usage : echo [0 | 1 | 2] > /proc/fsr/dbgmsg\r\n");
		DEBUG(DL0,"        cat > /proc/LinuStoreIII/dbgmsg\r\n");
	}

	return count;
}

static int fsr_get_debug_message(char *page, char **start, off_t off,
							int count, int *eof, void *data)
{
	unsigned int len = 0;

	if (FSR_DBG_GetDbgZoneMask() == 0)
	{
		len += snprintf(page + len, PAGE_SIZE - len, "DbgMsg disable\r\n");
	}
	else if (FSR_DBG_GetDbgZoneMask() == FSR_DBZ_DEFAULT)
	{
		len += snprintf(page + len, PAGE_SIZE - len, 
						"DbgMsg Default Enable\r\n");
	}
	else if (FSR_DBG_GetDbgZoneMask() == FSR_DBZ_ALL_ENABLE)
	{
		len += snprintf(page + len, PAGE_SIZE - len, 
						"DbgMsg All Enable\r\n");
	}
	else
	{
		len += snprintf(page + len, PAGE_SIZE - len, 
						"DbgMsg Enable : 0x%08X\r\n", 
						FSR_DBG_GetDbgZoneMask());
	}

	return len;
}
#endif
#endif

/**
 * BML block module init
 * @return	0 on success
 */
int __init bml_block_init(void)
{

#if defined(CONFIG_LINUSTOREIII_DEBUG) && defined(CONFIG_PROC_FS)
	struct proc_dir_entry *bmlinfo_entry = NULL;
	struct proc_dir_entry *dbgmsg_entry = NULL;

	bmlinfo_entry = create_proc_entry(BML_IOSTAT_PROC_NAME,
			S_IFREG | S_IWUSR | S_IRUGO,  fsr_proc_dir);
	if (bmlinfo_entry) 
	{
		bmlinfo_entry->read_proc = bml_show_iostat;
		bmlinfo_entry->write_proc = bml_reset_iostat;
	}

	bml_iostat = kmalloc(sizeof(bml_iostat[IO_DIRECTION]) * BLK_DEF_MAX_SECTORS, GFP_KERNEL);
	if (bml_iostat == NULL)
	{
		ERRPRINTK("bml_iostat kmalloc error\n");
		return -EIO;
	}
	memset(bml_iostat, 0, sizeof(bml_iostat[IO_DIRECTION]) * BLK_DEF_MAX_SECTORS);

#if !defined(FSR_OAM_RTLMSG_DISABLE) || defined(FSR_OAM_DBGMSG_ENABLE)
	dbgmsg_entry = create_proc_entry(FSR_DBGMSG_PROC_NAME,
			S_IFREG | S_IWUSR | S_IRUGO,  fsr_proc_dir);
	if (dbgmsg_entry)
	{
		dbgmsg_entry->read_proc = fsr_get_debug_message;
		dbgmsg_entry->write_proc = fsr_set_debug_message;
	}
#endif 
#endif

	if (0 == bml_blkdev_init())
	{
		printk("BML: Registered BML Driver.\n");
	}
	else
	{
		ERRPRINTK("BML: FSR: Couldn't register BML Driver.\n");
		return 1;
	}

	return 0;
}

/**
 * BML block module exit
 */
void __exit bml_block_exit(void)
{
	stl_ioctl_func = NULL;
#if defined(CONFIG_LINUSTOREIII_DEBUG) && defined(CONFIG_PROC_FS)
	remove_proc_entry(BML_IOSTAT_PROC_NAME, fsr_proc_dir);
#if !defined(FSR_OAM_RTLMSG_DISABLE) || defined(FSR_OAM_DBGMSG_ENABLE)
	remove_proc_entry(FSR_DBGMSG_PROC_NAME, fsr_proc_dir);
#endif
#endif

	bml_blkdev_exit();
}

