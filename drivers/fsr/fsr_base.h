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
 * @file	drivers/fsr/fsr_base.h
 * @brief	The most commom part and some inline functions to mainipulate
 *		the FSR instance (volume specification, partition table)
 *
 */

#ifndef _FSR_BASE_H_
#define _FSR_BASE_H_

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/version.h>
#include <linux/fsr_if.h>
#include "debug.h"

#include <FSR.h>
//#include <FsrTypes.h>
#include <FSR_OAM.h>
#include <FSR_PAM.h>
#include <FSR_STL.h>
#include <FSR_BML.h>
#include <FSR_LLD.h>
#if defined(CONFIG_PM)
#if (defined(CONFIG_TINY_FSR) && defined(CONFIG_RFS_FSR_MODULE)) || defined(CONFIG_RFS_FSR)
#include <../Core/BML/FSR_BML_Types.h>
#include <../Core/BML/FSR_BML_BIFCommon.h>
#endif
#endif

#define SECTOR_SIZE             512
#define SECTOR_BITS             9
#define OOB_SIZE		16
#define OOB_BITS		4
#define SECTOR_MASK             MASK(SECTOR_BITS)
#define MAX_LEN_PARTITIONS	(sizeof(FSRPartI))

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2, 6, 15)
#define BLK_DEF_MAX_SECTORS MAX_SECTORS
#endif

#ifdef CONFIG_PROC_FS
	extern struct proc_dir_entry *fsr_proc_dir;
#endif

#define FSR_PROC_DIR		"LinuStoreIII"
#define FSR_PROC_BMLINFO	"bmlinfo"
#define FSR_PROC_STLINFO	"stlinfo"

extern struct semaphore fsr_mutex;
extern int (*sec_stl_delete)(dev_t dev, u32 start, u32 nums, u32 b_size);

FSRVolSpec *fsr_get_vol_spec(u32 volume);
FSRPartI   *fsr_get_part_spec(u32 volume);

static inline unsigned int fsr_minor(unsigned int vol, unsigned int part)
{
	return ((vol << PARTITION_BITS) + (part + 1));
}

static inline unsigned int fsr_vol(unsigned int minor)
{
	return (minor >> PARTITION_BITS);
}

/*Get partition*/
static inline unsigned int fsr_part_spu(FSRVolSpec *volume, FSRPartI *pi, u32 partno)
{
	if(pi->stPEntry[partno].nAttr & FSR_BML_PI_ATTR_SLC)
	{
		return (volume->nSctsPerPg * volume->nPgsPerSLCUnit);
	}
	else
	{
		return (volume->nSctsPerPg * volume->nPgsPerMLCUnit);
	}
}

static inline unsigned int fsr_part_ppu(FSRVolSpec *volume, FSRPartI *pi, u32 partno)
{
	if(pi->stPEntry[partno].nAttr & FSR_BML_PI_ATTR_SLC)
	{
		return volume->nPgsPerSLCUnit;
	}
	else
	{
		return volume->nPgsPerMLCUnit;
	}
}

static inline unsigned int fsr_part(unsigned int minor)
{
	return (minor & PARTITION_MASK) - 1;
}

static inline unsigned int fsr_is_whole_dev(unsigned int part_no)
{
	return (part_no >> PARTITION_BITS);
}

static inline unsigned int fsr_parts_nr(FSRPartI *pt)
{
	return (pt->nNumOfPartEntry);
}

static inline FSRPartEntry *fsr_part_entry(FSRPartI *pt, unsigned int no)
{
	return &(pt->stPEntry[no]);
}

static inline unsigned int fsr_part_units_nr(FSRPartI *pt, unsigned int no)
{
	return (pt->stPEntry[no].nNumOfUnits);
}

static inline unsigned int fsr_part_id(FSRPartI *pt, unsigned int no)
{
	return (pt->stPEntry[no].nID);
}

static inline unsigned int fsr_part_attr(FSRPartI *pt, unsigned int no)
{
	return (pt->stPEntry[no].nAttr);
}

static inline unsigned int fsr_part_start(FSRPartI *pt, unsigned int no)
{
	return (pt->stPEntry[no].n1stVun);
}

static inline unsigned int fsr_part_size(FSRPartI *pt, unsigned int no)
{
	return (pt->stPEntry[no].nNumOfUnits << SECTOR_BITS);
}

/*Get volume*/
static inline unsigned int fsr_vol_spp(FSRVolSpec *volume)
{
	return (volume->nSctsPerPg);
}

static inline unsigned int fsr_vol_sectors_nr(u32 volume)
{
	FSRVolSpec *vs;
	FSRPartI *pi;
	FSRPartEntry pe;
	u32 n1stVpn, nPgsPerUnit;
	
	vs = fsr_get_vol_spec(volume);
	pi = fsr_get_part_spec(volume);
	pe = pi->stPEntry[pi->nNumOfPartEntry - 1];

	if(FSR_BML_GetVirUnitInfo(volume, 
		fsr_part_start(pi, pi->nNumOfPartEntry - 1), 
		&n1stVpn, &nPgsPerUnit) != FSR_BML_SUCCESS)
	{
		ERRPRINTK("FSR_BML_GetVirUnitInfo FAIL\r\n");
		return -1;
	}

	return (n1stVpn + pe.nNumOfUnits * nPgsPerUnit) * vs->nSctsPerPg;
}

static inline unsigned int fsr_vol_pages_nr(u32 volume)
{
	FSRPartI *pi;
	FSRPartEntry pe;
	u32 n1stVpn, nPgsPerUnit;
	
	pi = fsr_get_part_spec(volume);
	pe = pi->stPEntry[pi->nNumOfPartEntry - 1];

	if(FSR_BML_GetVirUnitInfo(volume, 
		fsr_part_start(pi, pi->nNumOfPartEntry - 1), 
		&n1stVpn, &nPgsPerUnit) != FSR_BML_SUCCESS)
	{
		ERRPRINTK("FSR_BML_GetVirUnitInfo FAIL\r\n");
		return -1; 
	}

	return n1stVpn + (pe.nNumOfUnits * nPgsPerUnit);
}

static inline unsigned int fsr_vol_unitsize(u32 volume, u32 partno)
{
	FSRVolSpec *vs;
	FSRPartI *pi;
	
	vs = fsr_get_vol_spec(volume);
	pi = fsr_get_part_spec(volume);
	
	if (fsr_is_whole_dev(partno))
	{
		return (vs->nSctsPerPg * vs->nPgsPerSLCUnit * 2)
			<< SECTOR_BITS;

	}
	else if	(pi->stPEntry[partno].nAttr & FSR_BML_PI_ATTR_MLC)
	{
		return (vs->nSctsPerPg * vs->nPgsPerMLCUnit) 
				<< SECTOR_BITS;
	}
	
	else
	{
		return (vs->nSctsPerPg * vs->nPgsPerSLCUnit) 
			<< SECTOR_BITS;
	}
}

static inline unsigned int fsr_vol_unit_nr(FSRVolSpec *volume)
{
	 return (volume->nNumOfUsUnits);
}

int bml_block_init(void);
void bml_block_exit(void);

FSRLowFuncTbl* fsr_get_low_tbl(u32 volume);

int fsr_init_partition(u32 volume);
int fsr_update_vol_spec(u32 volume);
int fsr_write_partitions(u32 volume, BML_PARTTAB_T *parttab);
int fsr_read_partitions(u32 volume, BML_PARTTAB_T *parttab);
struct block_device_operations *bml_get_block_device_operations(void);
int bml_blkdev_init(void);
void bml_blkdev_exit(void);
void fsr_register_stl_ioctl(int (*func) (u32, u32, u32, u32));
void fsr_unregister_stl_ioctl(void);

int bml_update_blkdev_param(u32 minor, u32 blkdev_size, u32 blkdev_blksize);
int stl_update_blkdev_param(u32 minor, u32 blkdev_size, u32 blkdev_blksize);
stl_info_t *fsr_get_stl_info(u32 volume, u32 partno);
struct block_device_operations *stl_get_block_device_operations(void);
void stl_blkdev_clean(u32 first_minor, u32 nparts);
int stl_blkdev_init(void);
void stl_blkdev_exit(void);

/* These macros will be use for iostat */
#if defined(CONFIG_LINUSTOREIII_DEBUG) && defined(CONFIG_PROC_FS)
void stl_count_iostat(int num_sectors, int rw);
void bml_count_iostat(int num_sectors, int rw);
#else
#define stl_count_iostat(...)		do { } while (0)
#define bml_count_iostat(...)		do { } while (0)
#endif /* CONFIG_LINUSTOREIII_DEBUG */

static inline unsigned int fsr_stl_sectors_nr(stl_info_t *ssp)
{
	return (ssp->total_sectors);
}

static inline unsigned int fsr_stl_page_size(stl_info_t *ssp)
{
	return (ssp->page_size);
}

/* kernel 2.6 */
#include <linux/version.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 0)
#include <linux/blkdev.h>

struct fsr_dev 
{
	struct request		*req;
	struct list_head	list;
	int			size;
	spinlock_t		lock;
	struct request_queue	*queue;
	struct gendisk          *gd;
	int			dev_id;
	struct scatterlist	*sg;
};
#else
/* Kernel 2.4 */
#ifndef __user
#define __user
#endif
#endif

#endif	/* _FSR_BASE_H_ */

