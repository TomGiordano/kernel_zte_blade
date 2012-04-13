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
 * @file        drivers/fsr/fsr_bench.c
 * @brief       benchmark program to analyze fsr (BML and STL)
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>

#include "fsr_base.h"

#ifndef MSEC_PER_SEC
#define MSEC_PER_SEC 1000L
#endif
#ifndef USEC_PER_SEC
#define USEC_PER_SEC 1000000L
#endif
#define FLOAT_POSITION 1000
#define BML	0
#define STL	1

/**
 * data to input get_performance()
 */
struct performance_input 
{
	u32 volume;
	u32 part_id;
	u32 part_first_sector;
	u32 part_end_sector;
	u32 part_first_page;
	u32 part_end_page;
	u32 dev_kind;
	char *buf;
};

/**
 * module parameter  
 */
static u32 major = 0; /* major number */
static u32 minor = 0; /* minor number */
static u32 sectors = 0; /* number of sectors */
static u32 rw = 2; /* 0: read, 1: write, otehrs; read/write */
static u32 size = 0; /* size for operation*/

module_param(major, int, 0644);
module_param(minor, int, 0644);
module_param(sectors, int, 0644);
module_param(rw, int, 0644);
module_param(size, int, 0644);

/**
 * calibrate_performance - calibrate a performance of operation
 * @param start_time		time when operation start
 * @param stop_time		time when operation stop
 * @param size_kbytes		size of transmission data (kbytes)
 * @return			MBytes per Second (size / interval)
 */
static u32 calibrate_performance(struct timeval start_time, struct timeval stop_time, u32 size_kbytes)
{
	u32 interval_msec, result;

	if (stop_time.tv_usec < start_time.tv_usec) 
	{
		stop_time.tv_sec -= (start_time.tv_sec + 1);
		stop_time.tv_usec += (USEC_PER_SEC - start_time.tv_usec);
	} 
	else 
	{
		stop_time.tv_sec -= start_time.tv_sec;
		stop_time.tv_usec -= start_time.tv_usec;
	}
	
	interval_msec = (stop_time.tv_sec * MSEC_PER_SEC) + (stop_time.tv_usec / MSEC_PER_SEC);
	result = (size_kbytes * FLOAT_POSITION) / interval_msec;
	result = (result * MSEC_PER_SEC) / 1024;
	
	return result;
}

/**
 * get_performance - measure a performance of operation
 * @param dev_input	input values to do operation
 * @param write		0: read operation, 1: write operation
 * @return		MBytes per Second (size / interval);
 */
static u32 get_performance(struct performance_input dev_input, int write)
{
	struct timeval start_time, stop_time;
	u32 current_sector = 0, datasize_kbytes = 0;
	u32 current_page = 0;
	u32 pages;
	int ret = 0;

	if (dev_input.dev_kind == BML) 
	{
		INT32 (*FSR_BML_Operation)(UINT32 nVol, UINT32 nVsn, UINT32 nNumOfScts,
				UINT8 *pMBuf, FSRSpareBuf *pSBuf, UINT32 nFlag) = NULL;
		if (write) 
		{
			FSR_BML_Operation = FSR_BML_Write;
		}
		else 
		{
			FSR_BML_Operation = FSR_BML_Read;
		}
		
		if (write && 
				sectors < fsr_get_vol_spec(dev_input.volume)->nSctsPerPg)
		{
			return 0;
		}
		pages = sectors / fsr_get_vol_spec(dev_input.volume)->nSctsPerPg;

		do_gettimeofday(&start_time);
		for (current_page = dev_input.part_first_page; 
			current_page + (pages - 1) <= dev_input.part_end_page;
				current_page += pages) 
		{
			ret = (*FSR_BML_Operation)(dev_input.volume, current_page, 
					pages, dev_input.buf, NULL, FSR_BML_FLAG_ECC_ON);
			if (ret != FSR_BML_SUCCESS) 
			{
				printk("FSR BML: %s transfer error = %x\n", write ? "WRITE" : "READ", ret);
				return 0;
			}
		}
		do_gettimeofday(&stop_time);
		datasize_kbytes = (current_page - dev_input.part_first_page) << 2;
	} 
	else if (dev_input.dev_kind == STL) 
	{
		INT32 (*FSR_STL_Operation)(UINT32 nVol, UINT32 nPartID, UINT32 nLsn,
				UINT32 nNumOfScts, UINT8 *pBuf, UINT32 nFlag) = NULL;
		if (write)
		{
			FSR_STL_Operation = FSR_STL_Write;
		}
		else 
		{
			FSR_STL_Operation = FSR_STL_Read;
		}

		FSR_DOWN(&fsr_mutex);
		do_gettimeofday(&start_time);
		
		for (current_sector = dev_input.part_first_sector; 
			current_sector + (sectors - 1) <= dev_input.part_end_sector;
				current_sector += sectors) 
		{
			ret = (*FSR_STL_Operation)(dev_input.volume, dev_input.part_id, 
					current_sector, sectors, dev_input.buf, FSR_STL_FLAG_USE_SM);
			if (ret != FSR_STL_SUCCESS) 
			{
				printk("stl: %s transfer error = %x\n", write ? "WRITE" : "READ", ret);
				printk("stl: partition id = %d, current sector = %d\n", 
					dev_input.part_id, current_sector);
				
				return 0;
			}
		}
		
		do_gettimeofday(&stop_time);
		FSR_UP(&fsr_mutex);
		datasize_kbytes = (current_sector - dev_input.part_first_sector) >> 1;
	}

	return calibrate_performance(start_time, stop_time, datasize_kbytes);
}

/**
 * check_sectors - check error of sectors module parameter
 * return	0 on success
 */
static int check_sectors(void)
{
	u32 match_count , shift_count;
	for (match_count = 0, shift_count = 0; shift_count < 8; shift_count++) 
	{
		if (sectors & (1 << shift_count))
		{
			match_count++;
		}
	}

	if (match_count != 1)
	{
		return 1; /* invalid sectors */
	}

	return 0; /* sectors = 2, 4, 8, 16, 32, 64, 128 */
}

/**
 * fsr benchmark module init
 * @return      0 on success
 */
static int __init fsr_bench_init(void)
{
	u32 part_no;
	u32 part_first_page, part_first_unit, part_end_unit, current_unit;
	u32 read_performance, write_performance;
	u32 stl_sectors, stl_len, ppu = 0;
	u32 ret;
	FSRPartI *ps;
	FSRStlInfo info;
	struct performance_input dev_input;

	/* check error of module parameter */
	if (major == 0 || ((major != BLK_DEVICE_BML) && 
				(major != BLK_DEVICE_STL))) 
	{
		printk("Fail: Can't use major %d\n"
			"insert major number(BML:137, STL:138)\n"
			"Usage: insmod fsr_bench.o major=? minor=? sectors=num\n", major);
		return -EINVAL;
	} 
	else if (minor == 0 || minor > 16) 
	{
		printk("Fail: Can't use minor %d\n"
			"insert minor number(partition: 1 - 16)\n"
			"Usage: insmod fsr_bench.o major=? minor=? sectors=num\n", minor);
		return -EINVAL;
	} 
	else if (sectors == 0 || sectors == 1 || check_sectors()) 
	{
		printk("Fail: Can't use sector number %d\n"
			"insert sector number(sector number: 2, 4, 8, 16, 32, 64, 128 \n"
			"Usage: insmod fsr_bench.o major=? minor=? sectors=num\n", sectors);
		return -EINVAL;
	}

	dev_input.volume = fsr_vol(minor);
	part_no = fsr_part(minor);

	ps = fsr_get_part_spec(dev_input.volume);

	dev_input.buf = kmalloc((sectors * SECTOR_SIZE), GFP_KERNEL);

	if (!dev_input.buf)
	{
		printk("[%d] %s kmalloc FAIL\r\n", __LINE__, __func__);
		return -ENOMEM;
	}
	
	memset(dev_input.buf, 0xa5, sectors * SECTOR_SIZE);
	printk("benchmarking [%s(major=%d) part %d(minor=%d)] with %d sector size\n", 
	major == 137 ? "BML" : "STL", major, part_no, minor, sectors);

	if (major ==  BLK_DEVICE_BML) /* BML */
	{
		dev_input.dev_kind = BML;
		dev_input.part_id = (u32)NULL;
		ret = FSR_BML_Open(dev_input.volume, FSR_BML_FLAG_NONE);
		if (ret != FSR_BML_SUCCESS) 
		{
	           	printk(KERN_ERR "BML: open error = %d\n", ret);
			return -ENODEV;
		}

		ret = FSR_BML_GetVirUnitInfo(dev_input.volume, fsr_part_start(ps, part_no), &part_first_page, &ppu);
		if (ret != FSR_BML_SUCCESS)
		{
			printk("FSR_BML_GetVirUnitInfo FAIL : 0x%08X\r\n", ret);
			return -EIO;
		}
		part_first_unit = fsr_part_start(ps, part_no);

		part_end_unit = part_first_unit + fsr_part_units_nr(ps, part_no) - 1;

		for (current_unit = part_first_unit; current_unit <= part_end_unit; current_unit++) 
		{
			ret = FSR_BML_Erase(dev_input.volume, &current_unit, 1, FSR_BML_FLAG_NONE);

			if (ret != FSR_BML_SUCCESS) 
			{
				printk("erase error = %x\n", ret);
				return -EIO;
			}
		}

		dev_input.part_first_page = part_first_page;

		dev_input.part_end_page = part_first_page 
			+ fsr_part_units_nr(ps, part_no) 
			* ppu - 1;

		if (rw != READ) 
		{
			write_performance = get_performance(dev_input, WRITE);
			printk("write: %d.%dMB/s\n",
				write_performance / FLOAT_POSITION,
					write_performance % FLOAT_POSITION);
		}
		if (rw != WRITE) 
		{
			read_performance = get_performance(dev_input, READ);
			printk("read: %d.%dMB/s\n",
				read_performance / FLOAT_POSITION,
					read_performance % FLOAT_POSITION);
		}

		FSR_BML_Close(dev_input.volume, FSR_BML_FLAG_NONE);	
	} 

	else if(major == BLK_DEVICE_STL) /* STL */
	{
		dev_input.dev_kind = STL;
		dev_input.part_id = fsr_part_id(ps, part_no);

		if (rw != READ) 
		{
			FSR_DOWN(&fsr_mutex);
			ret = FSR_STL_Open(dev_input.volume, dev_input.part_id, &info, 
								FSR_STL_FLAG_DEFAULT);
			FSR_UP(&fsr_mutex);

			if (ret == FSR_STL_PARTITION_ALREADY_OPENED) 
			{
				printk("FSR STL: Device is busy\n");
				printk("FSR STL: If you had execute only write benchmark,"
						" you have to execute read benchmark.\n");
				return -EBUSY;
			} 
			else if (ret == FSR_STL_UNFORMATTED) 
			{
				printk("FSR STL: Device is unformatted\n");
				return -ENXIO;
			} 
			else if (ret != FSR_STL_SUCCESS) 
			{
				printk("FSR STL: Out of device\n");
				return -ENXIO;
			}
		}

		FSR_DOWN(&fsr_mutex);
		ret = FSR_STL_IOCtl(dev_input.volume, dev_input.part_id, FSR_STL_IOCTL_LOG_SECTS, NULL, sizeof(u32), &stl_sectors, sizeof(u32), &stl_len);
		FSR_UP(&fsr_mutex);
		if (ret != FSR_STL_SUCCESS) 
		{
			printk("FSR STL: Ioctl error\n");
			printk("FSR STL: If you want to execute only read benchmark,"
					" you have to execute write benchmark before that.\n");
			return -EINVAL;
		}
		/* first sector of stl partition is always zero */
		dev_input.part_first_sector = 0; 
		dev_input.part_end_sector = size ? size : stl_sectors - 1;

		if (rw != READ) 
		{
			write_performance = get_performance(dev_input, WRITE);
			printk("write: %d.%03dMB/s\n",
				write_performance / FLOAT_POSITION,
					write_performance % FLOAT_POSITION);
		}
	
		/* To remove Sam table effect after STL_Write */	
		FSR_DOWN(&fsr_mutex);
		FSR_STL_Close(dev_input.volume, dev_input.part_id);

		ret = FSR_STL_Open(dev_input.volume, dev_input.part_id, &info, 
							FSR_STL_FLAG_DEFAULT);
		FSR_UP(&fsr_mutex);
		if (ret == FSR_STL_PARTITION_ALREADY_OPENED) 
		{
			printk("STL: Device is busy\n");
			return -EBUSY;
		} 
		else if (ret == FSR_STL_UNFORMATTED) 
		{
			printk("STL: Device is unformatted\n");
			return -ENXIO;
		} 
		else if (ret != FSR_STL_SUCCESS) 
		{
			printk("STL: Out of device\n");
			return -ENXIO;
		}

		if (rw != WRITE) 
		{
			read_performance = get_performance(dev_input, READ);
			printk("read: %d.%03dMB/s\n",
				read_performance / FLOAT_POSITION,
					read_performance % FLOAT_POSITION);
		}

		FSR_DOWN(&fsr_mutex);
		FSR_STL_Close(dev_input.volume, dev_input.part_id);
		FSR_UP(&fsr_mutex);
	}

	if(dev_input.buf)
	{
		kfree(dev_input.buf);
	}
	return 0;
}

/**
 * fsr benchmark module exit
 * @return              none
 */
static void __exit fsr_bench_cleanup(void)
{
}

module_init(fsr_bench_init);
module_exit(fsr_bench_cleanup);

MODULE_LICENSE("Samsung Proprietary");
MODULE_AUTHOR("Samsung Electronics");
MODULE_DESCRIPTION("benchmark program to analyze fsr");
