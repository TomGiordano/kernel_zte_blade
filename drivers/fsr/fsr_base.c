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
 *  @version    LinuStoreIII_1.2.0_b035-FSR_1.2.1p1_b129_RC
 *  @file       drivers/fsr/fsr_base.c
 *  @brief      This file is a basement for FSR adoption. It povides
 *              partition management, proc inteface, contexts management
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <linux/proc_fs.h>
#include "fsr_base.h"
#include "FSR_LLD_4K_OneNAND.h"


extern  VOID    memcpy32 (VOID       *pDst,
                          VOID       *pSrc,
                          UINT32     nSize);

/* This is the contexts to keep the specification of volume */
static FSRLowFuncTbl *gpstFSRLFT[FSR_MAX_VOLS];

static FSRVolSpec      vol_spec[FSR_MAX_VOLUMES];
static FSRPartI        part_spec[FSR_MAX_VOLUMES];
static stl_info_t stl_info_list[FSR_MAX_VOLUMES * MAX_FLASH_PARTITIONS];

struct proc_dir_entry *fsr_proc_dir = NULL;
EXPORT_SYMBOL(fsr_proc_dir);

extern struct FlexONDShMem *gpstFNDShMem;
extern struct OneNANDShMem *gpstONDShMem;
extern struct OneNAND4kShMem *gpstOND4kShMem;


#ifndef CONFIG_TINY_FSR
	int (*sec_stl_delete)(dev_t dev, u32 start, u32 nums, u32 b_size) = NULL;
	EXPORT_SYMBOL(sec_stl_delete);
#endif

/* To protect fsr operations, this semaphore lock fsr codes */
DECLARE_MUTEX(fsr_mutex);

FSRLowFuncTbl *fsr_get_low_tbl(u32 volume)
{
	int nVol, ret = 0;

	for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
	{
		if (gpstFSRLFT[nVol] == NULL)
		{
			gpstFSRLFT[nVol] = (FSRLowFuncTbl *) FSR_OAM_Malloc
							(sizeof(FSRLowFuncTbl));
			if (gpstFSRLFT[nVol] == NULL)
			{
				ERRPRINTK("FSR_OAM_Malloc Fail : %x", ret);
				return NULL;
			}
		}
	}

	ret = FSR_PAM_RegLFT(gpstFSRLFT);
	if (ret != FSR_PAM_SUCCESS)
	{
		ERRPRINTK("FSR_PAM_RegLFT Fail : %x", ret);
	}

	return gpstFSRLFT[volume];
}

/**
 * fsr_get_vol_spec - get a volume instance
 * @param volume        : a volume number
 */
FSRVolSpec *fsr_get_vol_spec(u32 volume)
{
	return &vol_spec[volume];
}

/**
 * fsr_get_part_spec - get a partition instance
 * @param volume        : a volume number
 */

FSRPartI *fsr_get_part_spec(u32 volume)
{
	return &part_spec[volume];
}

/**
 * fsr_get_vol_spec - get a STL info instance
 * @param volume        : a volume number
 & @param partno        : a partition number
 */
stl_info_t *fsr_get_stl_info(u32 volume, u32 partno)
{
	return (stl_info_list + (volume * MAX_FLASH_PARTITIONS) + partno);
}

/**
 * fsr_update_vol_spec - update volume & partition instance from the device
 * @param volume        : a volume number
 */
int fsr_update_vol_spec(u32 volume)
{
	int error;
	FSRVolSpec vs, *gstvs;
	FSRPartI pi, *gstpi;
	
	gstvs = fsr_get_vol_spec(volume);
	gstpi = fsr_get_part_spec(volume);
	
	memset(&vs, 0x00, sizeof(FSRVolSpec));
	memset(&pi, 0x00, sizeof(FSRPartI));
	
	error = FSR_BML_GetVolSpec(volume, &vs, FSR_BML_FLAG_NONE);
	/* I/O error */
	if (error != FSR_BML_SUCCESS)
	{
		ERRPRINTK("BML: FSR_BML_GetVolSpec error[0x%08x]\n", error);
		return -1;
	}

	error = FSR_BML_GetFullPartI(volume, &pi);
	/* I/O error */
	if (error != FSR_BML_SUCCESS)
	{
		ERRPRINTK("BML: FSR_BML_GetFullPartI error[0x%08x]\n", error);
		return -1;
	}

	memcpy(gstvs, &vs, sizeof(FSRVolSpec));
	memcpy(gstpi, &pi, sizeof(FSRPartI));
	
	return 0;
}

/**
 * fsr_read_partitions - read partition table into the device
 * @param volume        : a volume number
 * @param parttab       : buffer to store the partition table
 */
int fsr_read_partitions(u32 volume, BML_PARTTAB_T *parttab)
{
	int error;
	u32 partno;
	FSRPartI *pi;
	
	pi = fsr_get_part_spec(volume);
	
	/*could't find vaild part table*/
	if (!pi->nNumOfPartEntry) 
	{
		error = fsr_update_vol_spec(volume);
		/* I/O error */
		if (error) /*never action, because of low-formatting*/
		{
			DEBUG(DL2,"error(%x)", error);
		}
	}
	
	DEBUG(DL2,"pi->nNumOfPartEntry: %d", pi->nNumOfPartEntry);
	parttab->num_parts =  (int) pi->nNumOfPartEntry;
	for (partno = 0; partno < pi->nNumOfPartEntry; partno++) 
	{
		parttab->part_size[partno]      = (int) pi->stPEntry[partno].nNumOfUnits;
		parttab->part_id[partno]        = (int) pi->stPEntry[partno].nID;
		parttab->part_attr[partno]      = (int) pi->stPEntry[partno].nAttr;
		parttab->part_addr[partno]      = (int) pi->stPEntry[partno].nLoadAddr;
	}
	
	return 0;
}

/**
 * fsr_write_partitions - write partition table into the device
 * @param volume        : a volume number
 * @param parttab       : buffer to store the partition table
 */
int fsr_write_partitions(u32 volume, BML_PARTTAB_T *parttab)
{
	u32 partno, sum_blks;
	FSRPartI *pi, *core;
	int error, ret;
	
	DEBUG(DL3,"BML[I] volume(%d)\n",volume);

	if (parttab->num_parts > MAX_FLASH_PARTITIONS) 
	{
		/* out-of-range input */
		ERRPRINTK("BML: Too many partitions (%d > %d)",
				parttab->num_parts, MAX_FLASH_PARTITIONS);
		return -1;
	}
	
	pi = (FSRPartI *) kmalloc(sizeof(FSRPartI), GFP_KERNEL);
	/* memory error */
	if (!pi)
	{
		ERRPRINTK("FSRPartI kmalloc fail\n");
		return -ENOMEM;
	}
	memset(pi, 0, sizeof(FSRPartI));
	
	/* Don't need check return value */
	/* Alread Opened FSR_BML_Open from BML block device creating time */
	FSR_BML_Close(volume, FSR_BML_FLAG_NONE);
	
	pi->nNumOfPartEntry = (u32) parttab->num_parts;
	
	/*covert from user parttab*/
	for (partno = 0, sum_blks = 0; partno < (u32) parttab->num_parts; partno++) 
	{
		pi->stPEntry[partno].n1stVun = sum_blks;
		pi->stPEntry[partno].nNumOfUnits = (u32) parttab->part_size[partno];
		pi->stPEntry[partno].nID = (u32) parttab->part_id[partno];
		pi->stPEntry[partno].nAttr = (u32) parttab->part_attr[partno];
		pi->stPEntry[partno].nLoadAddr = (u32) parttab->part_addr[partno];
		sum_blks += pi->stPEntry[partno].nNumOfUnits;
		
		DEBUG(DL2,"start: %d, size: %d id : %d",
				pi->stPEntry[partno].n1stVun,
				pi->stPEntry[partno].nNumOfUnits,
				pi->stPEntry[partno].nID);
	}
	
	error = FSR_BML_Format(volume, pi, FSR_BML_REPARTITION | FSR_BML_AUTO_ADJUST_PARTINFO);
	
	if (error == FSR_BML_ALREADY_OPENED)
	{
		/* Don't need the return value, This function will decrease the open count */
		FSR_BML_Close(volume, FSR_BML_FLAG_NONE);

		error = FSR_BML_Format(volume, pi, FSR_BML_REPARTITION | FSR_BML_AUTO_ADJUST_PARTINFO);
		if (error != FSR_BML_SUCCESS) 
		{
			ERRPRINTK("FSR_BML_Format function fail[0x%08X]\n", error);
			goto out;
		}

		error = FSR_BML_Open(volume, FSR_BML_FLAG_NONE);
		if(error != FSR_BML_SUCCESS)
		{
			ERRPRINTK("BML: FSR_BML_Open fuction fail[0x%08x]\n", error);
			goto out;
		}
	}
	else if (error != FSR_BML_SUCCESS)
	{
		ERRPRINTK("FSR_BML_Format function fail[0x%08x]\n", error);
		goto out;
	}
	
	/* get part_spec to update from user's partition table */
	core = fsr_get_part_spec(volume);
	
	/*update in-core partition table*/
	memcpy(core, pi, sizeof(FSRPartI));
out:
	/* Don't need the return value. 
	 * Because FSR_BML_Open is already opened after FSR_BML_Format.
	 */
	ret = FSR_BML_Open(volume, FSR_BML_FLAG_NONE);
	if (ret != FSR_BML_SUCCESS)
	{
		ERRPRINTK("Can not open the volume : %d\n", volume);
		kfree(pi);
		return ret;
	}
	kfree(pi);

	DEBUG(DL3,"BML[O] volume(%d)\n",volume);

	return error;
}


#if defined(CONFIG_LINUSTOREIII_DEBUG) && defined(CONFIG_PROC_FS)
/**
 * fsr_proc_output - Display the partition information
 * @param buf           buffer to write data
 * @return              none
 *
 * If you use the CONFIG_LINUSTOREIII_DEBUG, it will print the chip id
 */
static int fsr_proc_output(char *buf)
{
	int nchips;
	int ret;
	unsigned int len = 0;
	u32 nVersion;
	u32 start, size, block_nr;
	u32 i, j, nIdx, num_bad, index;
	FSRBMInfo bmap;
	FSRPartI *pi;
	FSRVolSpec *vs;
	PRIVATE BmlBMF gstBmf0[FSR_MAX_BAD_BLKS];
	PRIVATE BmlBMF gstBmf1[FSR_MAX_BAD_BLKS];
	
	nchips = FSR_MAX_VOLUMES;
	
	len += snprintf(buf + len, PAGE_SIZE - len, "FSR VERSION: %s\r\n", FSR_Version(&nVersion));
	len += snprintf(buf + len, PAGE_SIZE - len, "minor       position           size     units       id\n");
	
	for (i = 0; i < nchips; i++) 
	{
		vs = fsr_get_vol_spec(i);
		/*no more device*/
		if (vs->nPgsPerSLCUnit == 0)
		        continue;
		pi = fsr_get_part_spec(i);
		
		for (j = 0; j < fsr_parts_nr(pi); j++) 
		{
			start = fsr_part_start(pi, j) * fsr_vol_unitsize(i, j);
			size = fsr_part_units_nr(pi, j) * fsr_vol_unitsize(i, j);
			block_nr = fsr_part_units_nr(pi, j);
			
			len += snprintf(buf + len, PAGE_SIZE - len, "%4u: 0x%08x-0x%08x 0x%08x %6d %8d\n",
					        fsr_minor(i, j),
					        start, start + size, size, block_nr,
					        fsr_part_id(pi, j));
		}
	}
	
	for (i = 0; i < nchips; i++) 
	{
		index = 0;
		bmap.nNumOfDies = 0;
		bmap.nNumOfBMFs[0] = 0;
		bmap.nNumOfBMFs[1] = 0;
		bmap.pstBMF[0] = &gstBmf0[0];
		bmap.pstBMF[1] = &gstBmf1[0];
		
		ret = FSR_BML_IOCtl(i, FSR_BML_IOCTL_GET_BMI, (u8 *) &index, sizeof(index),
		        (u8 *) &bmap, sizeof(FSRBMInfo), &num_bad);
		if (ret != FSR_BML_SUCCESS)
		{
			continue;
		}
		
		for (j  = 0; j < bmap.nNumOfDies; j++) 
		{
			len += snprintf(buf + len, PAGE_SIZE - len, "\n(%d)(%d) bad mapping information\n", i, j);
			len += snprintf(buf + len, PAGE_SIZE - len, "  No   BadUnit   RsvUnit\n");
			
			for(nIdx = 0; nIdx < bmap.nNumOfBMFs[j]; nIdx++)
			{
				len += snprintf(buf + len, PAGE_SIZE - len, "%4d: %8d  %8d \n", 
					nIdx, bmap.pstBMF[j][nIdx].nSbn, bmap.pstBMF[j][nIdx].nRbn);
			}
		}
		
	}
	
	return len;
}

/**
 * bml_read_proc - FSR kernel proc interface
 */
static int bml_read_proc(char *page, char **start, off_t off,
                int count, int *eof, void *data)
{
	int len = fsr_proc_output(page);
	if (len <= off + count)
	{
		*eof = 1;
	}
	*start = page + off;
	len -= off;
	if (len > count)
	{
		len = count;
	}
	if (len < 0) 
	{
		len = 0;
	}
	return len;
}


#endif  /* CONFIG_LINUSTOREIII_DEBUG && CONFIG_PROC_FS */

/**
 * fsr_init - [Init] initalize the fsr
 */
static int __init fsr_init(void)
{
	int error;
	DECLARE_TIMER;
	/*initialize global array*/
	START_TIMER();
	
	FSR_OAM_MEMSET(vol_spec, 0x00, sizeof(FSRVolSpec) * FSR_MAX_VOLUMES);
	FSR_OAM_MEMSET(part_spec, 0x00, sizeof(FSRPartI) * FSR_MAX_VOLUMES);

#if defined(FSR_OAM_ALL_DBGMSG)
	FSR_DBG_SetDbgZoneMask(FSR_DBZ_ALL_ENABLE);
#endif
	
	error = FSR_BML_Init(FSR_BML_FLAG_NONE);
	STOP_TIMER("BML_Init");
	
	if (error != FSR_BML_SUCCESS && error != FSR_BML_ALREADY_INITIALIZED) 
	{
		ERRPRINTK("FSR_BML_Init: error (%x)\n", error);
		ERRPRINTK("Check the PAM module\n");
		return -ENXIO;
	}

#if defined(CONFIG_LINUSTOREIII_DEBUG) && defined(CONFIG_PROC_FS)
	/* make proc directory */
	fsr_proc_dir = proc_mkdir(FSR_PROC_DIR, NULL);
	if (!fsr_proc_dir)
	{
		ERRPRINTK("Can't Create LinuStoreIII proc dir\n");
	        return -EINVAL;
	}
	/* make proc entry and link the read function*/
	create_proc_read_entry(FSR_PROC_BMLINFO, 0, fsr_proc_dir, 
							bml_read_proc, NULL);


#endif

	/* call init bml block device */
	error = bml_block_init();
	if (error)
	{
		ERRPRINTK("BML: bml_block_init error (ret:%x)\n", error);
		return -ENXIO;
	}
	
	return 0;
}

/**
 * fsr_exit - exit all and clear proc entry
 */
static void __exit fsr_exit(void)
{
	bml_block_exit();

#if defined(CONFIG_LINUSTOREIII_DEBUG) && defined(CONFIG_PROC_FS)
	remove_proc_entry(FSR_PROC_BMLINFO, fsr_proc_dir);


	if (fsr_proc_dir)
	{
		remove_proc_entry(FSR_PROC_DIR, NULL);
	}
#endif
}

module_init(fsr_init);
module_exit(fsr_exit);

/* BASE */
EXPORT_SYMBOL(fsr_mutex);
EXPORT_SYMBOL(fsr_get_part_spec);
EXPORT_SYMBOL(fsr_get_vol_spec);
EXPORT_SYMBOL(fsr_get_stl_info);
EXPORT_SYMBOL(fsr_update_vol_spec);

/* OAM */
EXPORT_SYMBOL(FSR_OAM_Malloc);
EXPORT_SYMBOL(FSR_OAM_Free);
EXPORT_SYMBOL(FSR_OAM_DbgMsg);
EXPORT_SYMBOL(FSR_OAM_AcquireSM);
EXPORT_SYMBOL(FSR_OAM_ReleaseSM);
EXPORT_SYMBOL(FSR_OAM_CreateSM);
EXPORT_SYMBOL(FSR_OAM_DestroySM);

/* PAM */
EXPORT_SYMBOL(memcpy32);

/* BML */
EXPORT_SYMBOL(FSR_BML_Init);
EXPORT_SYMBOL(FSR_BML_Open);
EXPORT_SYMBOL(FSR_BML_Close);
EXPORT_SYMBOL(FSR_BML_Read);
EXPORT_SYMBOL(FSR_BML_GetVolSpec);
EXPORT_SYMBOL(FSR_BML_GetVirUnitInfo);
EXPORT_SYMBOL(FSR_BML_LoadPIEntry);
EXPORT_SYMBOL(FSR_BML_Write);
EXPORT_SYMBOL(FSR_BML_CopyBack);
EXPORT_SYMBOL(FSR_BML_Erase);
EXPORT_SYMBOL(FSR_BML_IOCtl);
EXPORT_SYMBOL(FSR_BML_FlushOp);
EXPORT_SYMBOL(FSR_BML_GetPairedVPgOff);
EXPORT_SYMBOL(FSR_BML_GetVPgOffOfLSBPg);
EXPORT_SYMBOL(FSR_BML_GetPartAttrChg);
EXPORT_SYMBOL(FSR_BML_SetPartAttrChg);

/* Added by SEC_TN */
EXPORT_SYMBOL(FSR_BML_GetFullPartI);
EXPORT_SYMBOL(FSR_BML_AcquireSM);
EXPORT_SYMBOL(FSR_BML_ReleaseSM);
EXPORT_SYMBOL(FSR_PAM_GetPAParm);
EXPORT_SYMBOL(FSR_OND_4K_Read);

/* STL */
EXPORT_SYMBOL(fsr_register_stl_ioctl);
EXPORT_SYMBOL(fsr_unregister_stl_ioctl);
EXPORT_SYMBOL(fsr_read_partitions);

/* Misc */
EXPORT_SYMBOL(FSR_VersionCode);
EXPORT_SYMBOL(FSR_DBG_SetDbgZoneMask);
EXPORT_SYMBOL(gnFSRDbgZoneMask);

MODULE_LICENSE("Samsung Proprietary");
MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("FSR common device layer");

