/**
 *   @mainpage   Flex Sector Remapper : LinuStoreIII_1.2.0_b036-FSR_1.2.1p1_b139_RTM
 *
 *   @section Intro Intro
 *       Flash Translation Layer for Flex-OneNAND and OneNAND
 *   
 *     @MULTI_BEGIN@ @COPYRIGHT_DEFAULT
 *     @section Copyright COPYRIGHT_DEFAULT
 *            COPYRIGHT. SAMSUNG ELECTRONICS CO., LTD.
 *                                    ALL RIGHTS RESERVED
 *     Permission is hereby granted to licensees of Samsung Electronics Co., Ltd. products
 *     to use this computer program only in accordance 
 *     with the terms of the SAMSUNG FLASH MEMORY DRIVER SOFTWARE LICENSE AGREEMENT.
 *     @MULTI_END@
 *
 *     @MULTI_BEGIN@ @COPYRIGHT_GPL
 *     @section Copyright COPYRIGHT_GPL
 *            COPYRIGHT. SAMSUNG ELECTRONICS CO., LTD.
 *                                    ALL RIGHTS RESERVED
 *     This program is free software; you can redistribute it and/or modify it
 *     under the terms of the GNU General Public License version 2 
 *     as published by the Free Software Foundation.
 *     @MULTI_END@
 *
 *     @section Description
 *
 */

/**
 * @file      FSR_BML.h
 * @brief     This header contains global type definitions, global macros, 
 *            prototypes of functions for FSR_BML operation
 * @author    MinYoung Kim
 * @author    SuRyun Lee
 * @date      11-JAN-2007
 * @remark
 * REVISION HISTORY
 * @n  11-JAN-2007 [SuRyun Lee]  : first writing
 * @n  11-JAN-2007 [MinYoung Kim]: add definitions for partition
 *
 */

#ifndef _FSR_BML_H_
#define _FSR_BML_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/********************************************************************************/
/* Common Constant definitions                                                  */
/********************************************************************************/
#define     FSR_BML_MAX_PARTENTRY               ((512 - 32) / 16)
#define     FSR_BML_MAX_PARTSIG                 (8)
#define     FSR_BML_MAX_PARTRSV                 (16)
#define     FSR_BML_MAX_PIEXT_DATA              (504)

#define     FSR_UID_SIZE                        (16) /* the size of unique ID   */
#define     FSR_PAIRED_PGMAP_SIZE               (FSR_MAX_VIR_PGS)

/********************************************************************************/
/* Macro for FSR_BML_CopyBack()                                                 */
/********************************************************************************/
#define     FSR_BML_CPBK_INVAILD_DATA           (0xffff)
#define     FSR_BML_CPBK_SPARE_START_OFFSET     (0x4000)
#define     FSR_BML_CPBK_MAX_RNDINARG           (16)

/* Macro for FSR_BML_Dump() */
#define     FSR_BML_LOCK_ATTR_MASK              (FSR_BML_PI_ATTR_RW     | \
                                                 FSR_BML_PI_ATTR_RO     | \
                                                 FSR_BML_PI_ATTR_LOCK   | \
                                                 FSR_BML_PI_ATTR_LOCKTIGHTEN)

/********************************************************************************/
/* nFlag                                                                        */
/********************************************************************************/
/*
 * every exported LLD API has nFlag.
 * nFlag is 32 bit wide, and has its meaning like this.
 *
 * +---------------------------------------------------+----------------------+----+----+----+----+----+----+----+----+----+
 * | 31 30 29 28 27 26 25 24 | 23 22 21 20 19 18 17 16 | 15 14 13 12 11 10 09 | 08 | 07 | 06 | 05 | 04 | 03 | 02 | 01 | 00 |
 * + ----------------------- + ----------------------- + ---------------------+ -- + -- + -- + -- + -- + -- + -- + -- + -- +
 * |  The Sct offset value   | The Sct offset value    |                      |ERS |DIS | ECC| OTP| R1 |SBuf|Flus|    |FOR-|
 * |     for 1st page        |     for last page       |                      |REF |ERR |    |    |    |    |hOp |    |MAT |
 * +-------------------------+-------------------------+----------------------+----+----+----+----+----+----+----+----+----+
 * <<Description>>
 *   31~24: (in BML_Read) start sector offset (0 ~ 7)
 *   23~16: (in BML_Read) last sector offset from the end of page (0 ~ 7)
 *   15~08: not used
 *      08: (in BML_Read) FSR_BML_FLAG_RUNTIME_ERASE_REFRESH
 *      07: (in BML_Read) FSR_BML_FLAG_INFORM_DISTURBANCE_ERROR
 *      06: ECC ON/OFF
 *      05: Dump mode
 *      04: (in BML_Read) FSR_BML_FLAG_RELIABLE_LOAD
 *      03: FSR_BML_FLAG_USE_SPAREBUF
 *      02: (in BML_FlushOp) FSR_BML_FLAG_NORMAL_MODE or FSR_BML_FLAG_EMERGENCY_MODE
 *      01: not used
 *      00: (in BML_Format) FSR_BML_INIT_FORMAT or FSR_BML_REPARTITION
 */

/*****************************************************************************/
/* nFlag value list                                                          */
/*****************************************************************************/
/* Default flag                                                              */
#define     FSR_BML_FLAG_NONE                       (0x00000000)

/* nFlag for FSR_BML_Init   */
#define     FSR_BML_FORCED_INIT                     (0x00000001)

/* nFlag value for FSR_BML_Open                                             */
#define     FSR_BML_FLAG_REFRESH_PARTIAL            (0x00000000)  /* Refresh partially  */
#define     FSR_BML_FLAG_REFRESH_ALL                (0x00000001)  /* Refresh all blocks */

#define     FSR_BML_FLAG_DO_NOT_REFRESH             (0x00000008)  /* do not refresh      */

/* nFlag value for using spare buffer */
#define     FSR_BML_FLAG_USE_SPAREBUF               (0x00000008)

/* nFlag value for FSR_BML_OTPRead                                          */
#define     FSR_BML_FLAG_DUMP                       (0x00000020)

/* nFlag value for LSB RECOVERY Load (Flex-OneNAND) */
#define     FSR_BML_FLAG_LSB_RECOVERY_LOAD          (0x00000010)

/* nFlag value for informming read disturbance error */
#define     FSR_BML_FLAG_INFORM_DISTURBANCE_ERROR   (0x00000080)

/* nFlag value for forced erase refresh operation in FSR_BML_Read */
#define     FSR_BML_FLAG_FORCED_ERASE_REFRESH      (0x00000100)

/* nFlag value for ingnoring read disturbance */
#define     FSR_BML_FLAG_IGNORE_READ_DISTURBANCE    (0x00001000)

/* nFlag for ECC ON/OFF from STL*/
#define     FSR_BML_FLAG_ECC_BASEBIT                (6)
#define     FSR_BML_FLAG_ECC_MASK                   (0x00000040)
#define     FSR_BML_FLAG_ECC_ON                     (0x00000000)
#define     FSR_BML_FLAG_ECC_OFF                    (0x00000040)

/* nFlags for FSR_BML_FlushOp()                                              */
#define     FSR_BML_FLAG_NORMAL_MODE                (0x00000000)
/* This flag uses only for the dual core system */
#define     FSR_BML_FLAG_NO_SEMAPHORE               (0x00000001)
#define     FSR_BML_FLAG_EMERGENCY_MODE             (0x00000004)

#define     FSR_BML_FLAG_1ST_SCTOFFSET_MASK         (0xFF000000)
#define     FSR_BML_FLAG_LAST_SCTOFFSET_MASK        (0x00FF0000)
#define     FSR_BML_FLAG_1ST_SCTOFFSET_BASEBIT      (24)
#define     FSR_BML_FLAG_LAST_SCTOFFSET_BASEBIT     (16)
#define     FSR_BML_FLAG_MAKE_SCTOFFSET(off1, off2)     \
                                                    (((off1 << FSR_BML_FLAG_1ST_SCTOFFSET_BASEBIT)  & FSR_BML_FLAG_1ST_SCTOFFSET_MASK) | \
                                                    ((off2 << FSR_BML_FLAG_LAST_SCTOFFSET_BASEBIT) & FSR_BML_FLAG_LAST_SCTOFFSET_MASK))

#define     FSR_BML_FLAG_PRIORITY_MASK              (0x0000f000)
#define     FSR_BML_FLAG_PRIORITY_BASEBIT           (12)

/* nFlag value for FSR_BML_Format() */
#define     FSR_BML_INIT_FORMAT                     (0x00000000)
#define     FSR_BML_REPARTITION                     (0x00000001)
#define     FSR_BML_AUTO_ADJUST_PARTINFO            (0x00000010)

/* # of shift bits for dump of OTP block */
#define     FSR_BML_DUMP_OTP_MASK                   (0x000000ff)
#define     FSR_BML_DUMP_OTP_DIE_SHIFTBIT           (16)
#define     FSR_BML_DUMP_OTP_PDEV_SHIFTBIT          (24)

/* nFeature of FSRDevSpec can have values listed below */
#define     FSR_BML_FEATURE_NONE                    (0x0000)
#define     FSR_BML_FEATURE_LSB_RECOVERY_LOAD       (0x0001)

/* nFlag Value for Suspend/Resume */
#define     FSR_BML_FLAG_RESTORE_BLOCK_STATE        (0x00000001)

/* nFlag Value for FSR_BML_Close */
#define     FSR_BML_FLAG_CLOSE_ALL                  (0x00000001)

/****************************************************************************/
/* Some definitions for FSR_BML_Dump()                                      */
/****************************************************************************/
/* nDumpType for dump image type */
#define     FSR_DUMP_VOLUME                     (0x00000000)
#define     FSR_DUMP_PARTITION                  (0x00000001)
#define     FSR_DUMP_RESERVOIR_ONLY             (0x00000002)

/* nDumpOrder for dump sequence */
#define     FSR_BML_FLAG_DUMP_FIRST             (0x00000000)
#define     FSR_BML_FLAG_DUMP_CONTINUE          (0x00000001)

/****************************************************************/
/* Some definitions for FSR_BML_IOCtl()                         */
/****************************************************************/
/* nOPMode for nCode = FSR_BML_IOCTL_GET_OPMODE_STAT */
#define     FSR_BML_BLOCKING_MODE               (0x00000000)
#define     FSR_BML_NONBLOCKING_MODE            (0x00000001)

/* nOTPBlk for nCode = FSR_BML_IOCTL_GET_OTP_INFO, FSR_BML_IOCTL_OTP_LOCK */
#define     FSR_BML_OTP_LOCK_1ST_BLK            (0x1)
#define     FSR_BML_OTP_LOCK_OTP_BLK            (0x2)

/* nLockStat for nCode = FSR_BML_IOCTL_GET_OTP_INFO, FSR_BML_IOCTL_OTP_LOCK */
#define     FSR_BML_OTP_1ST_BLK_LOCKED          (0x01)
#define     FSR_BML_OTP_1ST_BLK_UNLKED          (0x02)
#define     FSR_BML_OTP_OTP_BLK_LOCKED          (0x10)
#define     FSR_BML_OTP_OTP_BLK_UNLKED          (0x20)

/* nFuncType for nCode =FSR_BML_IOCTL_SET_NONBLOCKING */
#define     FSR_BML_SET_NONBLOCKING_READ        (0x00000001)
#define     FSR_BML_SET_NONBLOCKING_WRITE       (0x00000002)
#define     FSR_BML_SET_NONBLOCKING_ERASE       (0x00000004)
#define     FSR_BML_SET_NONBLOCKING_COPYBACK    (0x00000008)

#define     FSR_BML_SET_NONBLOCKING_ALL         (FSR_BML_SET_NONBLOCKING_READ   | \
                                                 FSR_BML_SET_NONBLOCKING_WRITE  | \
                                                 FSR_BML_SET_NONBLOCKING_ERASE  | \
                                                 FSR_BML_SET_NONBLOCKING_COPYBACK)

#define     FSR_BML_SET_NONBLOCKING_READ_EXCEPT (FSR_BML_SET_NONBLOCKING_WRITE  | \
                                                 FSR_BML_SET_NONBLOCKING_ERASE  | \
                                                 FSR_BML_SET_NONBLOCKING_COPYBACK)

/*****************************************************************************/
/* Major Return value of FSR_BML_XXX()                                       */
/*****************************************************************************/
/* Common value for FSR_BML_XXX()*/
#define     FSR_BML_SUCCESS                     FSR_RETURN_VALUE(0, 01, 0x0000, 0x0000)
#define     FSR_BML_CRITICAL_ERROR              FSR_RETURN_VALUE(1, 01, 0x0001, 0x0000)
#define     FSR_BML_INVALID_PARAM               FSR_RETURN_VALUE(1, 01, 0x0002, 0x0000)
#define     FSR_BML_DEVICE_ACCESS_ERROR         FSR_RETURN_VALUE(1, 01, 0x0003, 0x0000)
#define     FSR_BML_VOLUME_NOT_OPENED           FSR_RETURN_VALUE(1, 01, 0x0004, 0x0000)
#define     FSR_BML_OAM_ACCESS_ERROR            FSR_RETURN_VALUE(1, 01, 0x0005, 0x0000)
#define     FSR_BML_PAM_ACCESS_ERROR            FSR_RETURN_VALUE(1, 01, 0x0006, 0x0000)
#define     FSR_BML_ACQUIRE_SM_ERROR            FSR_RETURN_VALUE(1, 01, 0x0007, 0x0000)
#define     FSR_BML_RELEASE_SM_ERROR            FSR_RETURN_VALUE(1, 01, 0x0008, 0x0000)

/* Value for FSR_BML_Init(), FSR_BML_Open(), FSR_BML_Format and FSR_BML_Close*/
#define     FSR_BML_ALREADY_INITIALIZED         FSR_RETURN_VALUE(1, 01, 0x0009, 0x0000)
#define     FSR_BML_ALREADY_OPENED              FSR_RETURN_VALUE(1, 01, 0x000A, 0x0000)
#define     FSR_BML_UNLOCK_ERROR                FSR_RETURN_VALUE(1, 01, 0x000B, 0x0000)

/* Value for FSR_BML_Read(), FSR_BML_Write(), FSR_BML_Erase() and FSR_BML_CopyBack() */
#define     FSR_BML_READ_ERROR                  FSR_RETURN_VALUE(1, 01, 0x000C, 0x0000)
//#define     FSR_BML_READ_DISTURBANCE_ERROR      FSR_RETURN_VALUE(1, 01, 0x000D, 0x0000)
#define     FSR_BML_ERASE_ERROR                 FSR_RETURN_VALUE(1, 01, 0x000E, 0x0000)
#define     FSR_BML_WRITE_ERROR                 FSR_RETURN_VALUE(1, 01, 0x000F, 0x0000)
#define     FSR_BML_WR_PROTECT_ERROR            FSR_RETURN_VALUE(1, 01, 0x0010, 0x0000)

/* Value for others*/
#define     FSR_BML_UNSUPPORTED_FUNCTION        FSR_RETURN_VALUE(1, 01, 0x0011, 0x0000)
#define     FSR_BML_NO_PIENTRY                  FSR_RETURN_VALUE(1, 01, 0x0012, 0x0000)
#define     FSR_BML_STORE_PIEXT_ERROR           FSR_RETURN_VALUE(1, 01, 0x0013, 0x0000)
#define     FSR_BML_MAKE_RSVR_ERROR             FSR_RETURN_VALUE(1, 01, 0x0014, 0x0000)
#define     FSR_BML_UNSUPPORTED_IOCTL           FSR_RETURN_VALUE(1, 01, 0x0015, 0x0000)
#define     FSR_BML_CANT_LOCK_FOREVER           FSR_RETURN_VALUE(1, 01, 0x0016, 0x0000)
#define     FSR_BML_CANT_UNLOCK_WHOLEAREA       FSR_RETURN_VALUE(1, 01, 0x0017, 0x0000)
#define     FSR_BML_VOLUME_ALREADY_LOCKTIGHT    FSR_RETURN_VALUE(1, 01, 0x0018, 0x0000)
#define     FSR_BML_CANT_CHANGE_PART_ATTR       FSR_RETURN_VALUE(1, 01, 0x0019, 0x0000)
#define     FSR_BML_CANT_LOCK_BLOCK             FSR_RETURN_VALUE(1, 01, 0x001A, 0x0000)
#define     FSR_BML_CANT_UNLOCK_BLOCK           FSR_RETURN_VALUE(1, 01, 0x001B, 0x0000)
#define     FSR_BML_ALREADY_OTP_LOCKED          FSR_RETURN_VALUE(1, 01, 0x001C, 0x0000)
#define     FSR_BML_CANT_FIND_PART_ID           FSR_RETURN_VALUE(1, 01, 0x001D, 0x0000)
#define     FSR_BML_CANT_GET_RETURN_VALUE       FSR_RETURN_VALUE(1, 01, 0x001E, 0x0000)
#define     FSR_BML_INVALID_PARTITION           FSR_RETURN_VALUE(1, 01, 0x001F, 0x0000)
#define     FSR_BML_CANT_ADJUST_PARTINFO        FSR_RETURN_VALUE(1, 01, 0x0020, 0x0000)
#define     FSR_BML_NO_FREE_PAGE                FSR_RETURN_VALUE(1, 01, 0x0021, 0x0000)
#define     FSR_BML_PARTIION_RANGE_IS_NOT_SAME  FSR_RETURN_VALUE(1, 01, 0x0022, 0x0000)

/*****************************************************************************/
/* Major Return value of FSR_BBM_XXX()                                       */
/*****************************************************************************/
#define     FSR_BML_MOUNT_FAILURE               FSR_RETURN_VALUE(1, 01, 0x0030, 0x0000)
#define     FSR_BML_CONSTRUCT_BMF_FAILURE       FSR_RETURN_VALUE(1, 01, 0x0031, 0x0000)
#define     FSR_BML_NO_LPCB                     FSR_RETURN_VALUE(1, 01, 0x0032, 0x0000)
#define     FSR_BML_NO_UPCB                     FSR_RETURN_VALUE(1, 01, 0x0033, 0x0000)
#define     FSR_BML_LOAD_PI_FAILURE             FSR_RETURN_VALUE(1, 01, 0x0034, 0x0000)
#define     FSR_BML_LOAD_UBMS_FAILURE           FSR_RETURN_VALUE(1, 01, 0x0035, 0x0000)
#define     FSR_BML_LOAD_LBMS_FAILURE           FSR_RETURN_VALUE(1, 01, 0x0036, 0x0000)
#define     FSR_BML_NO_RSV_BLK_POOL             FSR_RETURN_VALUE(1, 01, 0x0037, 0x0000)
#define     FSR_BML_MAKE_NEW_PCB_FAILURE        FSR_RETURN_VALUE(1, 01, 0x0038, 0x0000)
#define     FSR_BML_UPDATE_PIEXT_FAILURE        FSR_RETURN_VALUE(1, 01, 0x0039, 0x0000)
#define     FSR_BML_ERASE_REFRESH_FAILURE       FSR_RETURN_VALUE(1, 01, 0x003A, 0x0000)
#define     FSR_BML_INVALID_PCB_AGE             FSR_RETURN_VALUE(1, 01, 0x003B, 0x0000)
#define     FSR_BML_UPDATE_META_FAILURE         FSR_RETURN_VALUE(1, 01, 0x003C, 0x0000)
#define     FSR_BML_PI_ALREADY_LOCKED           FSR_RETURN_VALUE(1, 01, 0x003D, 0x0000)
#define     FSR_BML_TOO_MUCH_INIT_BAD_DEVICE    FSR_RETURN_VALUE(1, 01, 0x003E, 0x0000)

/*****************************************************************************/
/* Value for FSR_BML_Dump()                                                  */
/*****************************************************************************/
#define     FSR_BML_DUMP_INCOMPLETE             FSR_RETURN_VALUE(1, 01, 0x0050, 0x0000)
#define     FSR_BML_DUMP_COMPLETE               FSR_RETURN_VALUE(1, 01, 0x0051, 0x0000)

#define     FSR_BML_1LV_READ_DISTURBANCE_ERROR  FSR_RETURN_VALUE(1, 01, 0x0052, 0x0000)
#define     FSR_BML_2LV_READ_DISTURBANCE_ERROR  FSR_RETURN_VALUE(1, 01, 0x0053, 0x0000)

/*****************************************************************************/
/* Minor retun value of FSR_BML_Format()                                     */
/*****************************************************************************/
#define     FSR_BML_USE_MULTI_CORE              FSR_RETURN_VALUE(1, 01, 0x0000, 0x0001)

/*****************************************************************************/
/* Minor retun value of FSR_BML_READ_ERROR for FSR_BML_CopyBack              */
/*****************************************************************************/
#define     FSR_BML_READ_1WAY                   FSR_RETURN_VALUE(0, 01, 0x0000, 0x0000)
#define     FSR_BML_READ_2WAY                   FSR_RETURN_VALUE(0, 01, 0x0000, 0x0001)
#define     FSR_BML_READ_3WAY                   FSR_RETURN_VALUE(0, 01, 0x0000, 0x0002)
#define     FSR_BML_READ_4WAY                   FSR_RETURN_VALUE(0, 01, 0x0000, 0x0003)

/*****************************************************************************/
/* Minor retun value of FSR_BML_IOCtl(FSR_BML_IOCTL_OTP_LOCK)                */
/*****************************************************************************/
#define     FSR_BML_OTP_1STBLK                  FSR_RETURN_VALUE(0, 01, 0x0000, 0x0004)
#define     FSR_BML_OTP_BLK                     FSR_RETURN_VALUE(0, 01, 0x0000, 0x0005)

/****************************************************************************/
/* Minor retun value of FSR_BML_INVALID_PARTITION                           */
/****************************************************************************/
#define     FSR_BML_NO_PARTI                    FSR_RETURN_VALUE(0, 01, 0x0000, 0x0020)
#define     FSR_BML_UNSUPPORTED_PARTENTRY       FSR_RETURN_VALUE(0, 01, 0x0000, 0x0021)
#define     FSR_BML_DUPLICATE_PARTID            FSR_RETURN_VALUE(0, 01, 0x0000, 0x0022)
#define     FSR_BML_INVALID_GLOBAL_WL           FSR_RETURN_VALUE(0, 01, 0x0000, 0x0023)
#define     FSR_BML_INVALID_NAND_TYPE           FSR_RETURN_VALUE(0, 01, 0x0000, 0x0024)
#define     FSR_BML_COEXIST_STL_DPW_ATTR        FSR_RETURN_VALUE(0, 01, 0x0000, 0x0025)
#define     FSR_BML_INVALID_LOCK_ATTR           FSR_RETURN_VALUE(0, 01, 0x0000, 0x0026)
#define     FSR_BML_UNSUPPORTED_ATTR            FSR_RETURN_VALUE(1, 01, 0x0000, 0x0027)
#define     FSR_BML_NO_UNITS                    FSR_RETURN_VALUE(1, 01, 0x0000, 0x0028)
#define     FSR_BML_UNSUPPORTED_UNITS           FSR_RETURN_VALUE(1, 01, 0x0000, 0x0029)
#define     FSR_BML_OVERLAPPED_PARTITION        FSR_RETURN_VALUE(1, 01, 0x0000, 0x002A)
#define     FSR_BML_UNUSED_AREA                 FSR_RETURN_VALUE(1, 01, 0x0000, 0x002B)
#define     FSR_BML_NO_STL_PARTID               FSR_RETURN_VALUE(1, 01, 0x0000, 0x002C)

/****************************************************************************/
/* value of nAttr of FSRPartEntry structure                                 */
/*                                                                          */
/* All partition should have one among following 3 attributes.              */
/*              'FSR_BML_PI_ATTR_LOCKTIGHTEN + FSR_BML_PI_ATTR_RO'          */
/*              'FSR_BML_PI_ATTR_LOCK + FSR_BML_PI_ATTR_RO'                 */
/*              'FSR_BML_PI_ATTR_RO'                                        */
/*              'FSR_BML_PI_ATTR_RW'                                        */
/* The partition used by STL can have following attributes                  */
/*              'FSR_BML_PI_ATTR_GLOBAL_WL_GRP0'                            */
/*              'FSR_BML_PI_ATTR_GLOBAL_WL_GRP1'                            */
/* The partition used by STL should have following attribute                */
/*              'FSR_BML_PI_ATTR_STL'                                       */
/* The partition loaded by DPM on demand should have following attribute    */
/*              'FSR_BML_PI_ATTR_DPW'                                       */
/* The first partition booted up should have following attribute            */
/*              'FSR_BML_PI_ATTR_ENTRYPOINT'                                */
/* The partition loaded by boot-loader(BL) should have following attribute  */
/*              'FSR_BML_PI_ATTR_BOOTLOADING'                               */
/* The partition programed by NAND-writer should have following attribute   */
/*              'FSR_BML_PI_ATTR_PREWRITING'                                */
/* The partition which is made up of SLC blocks should have following       */
/* attribute                                                                */
/*              'BML_PI_ATTR_SLC'                                           */
/* The partition which is made up of MLC blocks should have following       */
/* attribute                                                                */
/*              'BML_PI_ATTR_MLC'                                           */
/*                                                                          */
/* The other configurations are invalid attributes.                         */
/****************************************************************************/
#define     FSR_BML_PI_ATTR_RW                  0x0001
#define     FSR_BML_PI_ATTR_RO                  0x0002
#define     FSR_BML_PI_ATTR_PROCESSOR_ID0       0x0004
#define     FSR_BML_PI_ATTR_PROCESSOR_ID1       0x0008
#define     FSR_BML_PI_ATTR_LOCK                0x0010
#define     FSR_BML_PI_ATTR_LOCKTIGHTEN         0x0020
#define     FSR_BML_PI_ATTR_STL_GLOBAL_WL_GRP0  0x0040
#define     FSR_BML_PI_ATTR_STL_GLOBAL_WL_GRP1  0x0080
#define     FSR_BML_PI_ATTR_STL                 0x0100
#define     FSR_BML_PI_ATTR_DPW                 0x0200
#define     FSR_BML_PI_ATTR_ENTRYPOINT          0x0400
#define     FSR_BML_PI_ATTR_STL_HOT             0x0800
#define     FSR_BML_PI_ATTR_SLC                 0x1000
#define     FSR_BML_PI_ATTR_MLC                 0x2000
#define     FSR_BML_PI_ATTR_BOOTLOADING         0x4000
#define     FSR_BML_PI_ATTR_PREWRITING          0x8000

/********************************************************************************/
/* FSR BML IO Control Code                                                      */
/********************************************************************************/

/********************************************************************************/
/* FSR_BML_IOCtl(nVol, FSR_BML_IOCTL_UNLOCK_WHOLEAREA, NULL, 0, NULL, 0, &nRet) */
/********************************************************************************/
#define     FSR_BML_IOCTL_UNLOCK_WHOLEAREA  FSR_IOCTL_CODE(FSR_MODULE_BML,      \
                                                           0x01,                \
                                                           FSR_METHOD_IN_DIRECT,\
                                                           FSR_WRITE_ACCESS)

/********************************************************************************/
/* FSR_BML_IOCtl(nVol, FSR_BML_IOCTL_LOCK_FOREVER, NULL, 0, NULL, 0, &nRet)     */
/********************************************************************************/
#define     FSR_BML_IOCTL_LOCK_FOREVER      FSR_IOCTL_CODE(FSR_MODULE_BML,      \
                                                           0x02,                \
                                                           FSR_METHOD_IN_DIRECT,\
                                                           FSR_WRITE_ACCESS)

/********************************************************************************/
/*  UINT32      nRet;                                                           */
/*  UINT32      nVol;                                                           */
/*  FSRChangePA    stChangePA;                                                  */
/*                                                                              */
/*  nVol                = 0 or 1;                                               */
/*  stChangePA.nPartID  = PARTITION_ID;                                         */
/*  stChangePA.nNewAttr = BML_PI_ATTR_RO;                                       */
/*                                                                              */
/*  FSR_BML_IOCtl(nVol, FSR_BML_IOCTL_CHANGE_PART_ATTR, (UINT8 *) &stChangePA,  */
/*            sizeof(ChangePA), NULL, 0, &nRet)                                 */
/********************************************************************************/
#define     FSR_BML_IOCTL_CHANGE_PART_ATTR  FSR_IOCTL_CODE(FSR_MODULE_BML,      \
                                                           0x03,                \
                                                           FSR_METHOD_IN_DIRECT,\
                                                           FSR_WRITE_ACCESS)

/********************************************************************************/
/*  PRIVATE BmlBMF     gstBmf0[FSR_MAX_BAD_BLKS];                               */
/*  PRIVATE BmlBMF     gstBmf1[FSR_MAX_BAD_BLKS];                               */
/*                                                                              */
/*  UINT32     nDevIdx;                                                         */
/*  UINT32     nDieIdx;                                                         */
/*  UINT32     nNumOfDevs;                                                      */
/*  UINT32     nRet;                                                            */
/*  FSRBMInfo  stBMInfo;                                                        */
/*                                                                              */
/*  stBMInfo.nNumOfDies    = 0;                                                 */
/*  stBMInfo.nNumOfBMFs[0] = 0;                                                 */
/*  stBMInfo.nNumOfBMFs[1] = 0;                                                 */
/*  stBMInfo.pstBMF[0]     = &gstBmf0[0];                                       */
/*  stBMInfo.pstBMF[1]     = &gstBmf1[0];                                       */
/*                                                                              */
/*  FSR_BML_IOCtl(nVol, FSR_BML_IOCTL_GET_BMI, (UINT8 *) &nDevIdx,              */
/*            sizeof(nDevIdx), (UINT8 *)&stBMInfo, sizeof(FSRBMInfo), &nRet)    */
/********************************************************************************/
#define     FSR_BML_IOCTL_GET_BMI           FSR_IOCTL_CODE(FSR_MODULE_BML,      \
                                                           0x04,                \
                                                           FSR_METHOD_BUFFERED, \
                                                           FSR_READ_ACCESS)

/********************************************************************************/
/*  UINT32  nWaitTime;                                                          */
/*  UINT32  nRet;                                                               */
/*  UINT32  nVol;                                                               */
/*                                                                              */
/*  nVol        = 0;                                                            */
/*  nWaitTime   = 50;                                                           */
/*                                                                              */
/*  FSR_BML_IOCtl(nVol, FSR_BML_IOCTL_SET_WTIME4ERR, (UINT8 *) nWaitTime,       */
/*                sizeof(nWaitTime), NULL, 0, &nRet)                            */
/********************************************************************************/
#define     FSR_BML_IOCTL_SET_WTIME4ERR     FSR_IOCTL_CODE(FSR_MODULE_BML,      \
                                                           0x05,                \
                                                           FSR_METHOD_IN_DIRECT,\
                                                           FSR_WRITE_ACCESS)

/********************************************************************************/
/*  UINT32      nVol;                                                           */
/*  UINT32      nLockState;                                                     */
/*                                                                              */
/*  nVol      = 0;                                                              */
/*                                                                              */
/*  FSR_BML_IOCtl(nVol, FSR_BML_IOCTL_GET_OTP_INFO,                             */
/*                NULL, 0, (UINT8 *) &nLockState, sizeof(nLockState), &nRet)    */
/*                                                                              */
/*  nLockState can have the following macros                                    */
/*  - FSR_BML_OTP_1ST_BLK_LOCKED                                                */
/*  - FSR_BML_OTP_1ST_BLK_UNLKED                                                */
/*  - FSR_BML_OTP_OTP_BLK_LOCKED                                                */
/*  - FSR_BML_OTP_OTP_BLK_UNLKED                                                */
/********************************************************************************/
#define     FSR_BML_IOCTL_GET_OTP_INFO      FSR_IOCTL_CODE(FSR_MODULE_BML,      \
                                                           0x06,                \
                                                           FSR_METHOD_IN_DIRECT,\
                                                           FSR_WRITE_ACCESS)

/********************************************************************************/
/*  UINT32      nVol;                                                           */
/*  UINT32      nLockState;                                                     */
/*                                                                              */
/*  nVol      = 0;                                                              */
/*                                                                              */
/*  FSR_BML_IOCtl(nVol, FSR_BML_IOCTL_OTP_LOCK    ,                             */
/*                (UINT8 *) &nLockState, sizeof(nLockState), NULL, 0, &nRet)    */
/********************************************************************************/
#define     FSR_BML_IOCTL_OTP_LOCK          FSR_IOCTL_CODE(FSR_MODULE_BML,      \
                                                           0x07,                \
                                                           FSR_METHOD_IN_DIRECT,\
                                                           FSR_WRITE_ACCESS)

/********************************************************************************/
/*  UINT32      nVol;                                                           */
/*  UINT32      nRet;                                                           */
/*  UINT32      nPDev;                                                          */
/*  FSRSLCBoundary stSLCBD;                                                     */
/*                                                                              */
/*  nVol        = 0;                                                            */
/*  nPDev       = 0;                                                            */
/*                                                                              */
/*  FSR_BML_IOCtl(nVol, FSR_BML_IOCTL_GET_SLC_BOUNDARY, (UINT8 *) &nPDev,       */
/*            sizeOf(nPDev), (UINT8 *) &stSLCBD, sizeof(FSRSLCBoundary), &nRet) */
/********************************************************************************/
#define     FSR_BML_IOCTL_GET_SLC_BOUNDARY  FSR_IOCTL_CODE(FSR_MODULE_BML,      \
                                                           0x08,                \
                                                           FSR_METHOD_OUT_DIRECT,\
                                                           FSR_READ_ACCESS)

/********************************************************************************/
/*  FSR_BML_IOCtl(nVol, FSR_BML_IOCTL_FIX_SLC_BOUNDARY, NULL, 0, NULL, 0, &nRet)*/
/********************************************************************************/
#define     FSR_BML_IOCTL_FIX_SLC_BOUNDARY  FSR_IOCTL_CODE(FSR_MODULE_BML,      \
                                                           0x09,                \
                                                           FSR_METHOD_IN_DIRECT,\
                                                           FSR_WRITE_ACCESS)

/********************************************************************************/
/*  UINT32      nVol;                                                           */
/*  UINT32      nRet;                                                           */
/*  UINT32      nOPMode;                                                        */
/*                                                                              */
/*  nVol        = 0;                                                            */
/*                                                                              */
/*  FSR_BML_IOCtl(nVol, FSR_BML_IOCTL_GET_OPMODE_STAT, NULL,                    */
/*                0, (UINT8 *) nOPMode, sizeof(nOPMode), &nRet)                 */
/********************************************************************************/
#define     FSR_BML_IOCTL_GET_OPMODE_STAT   FSR_IOCTL_CODE(FSR_MODULE_BML,      \
                                                           0x0A,                \
                                                           FSR_METHOD_IN_DIRECT,\
                                                           FSR_READ_ACCESS)

/********************************************************************************/
/*  UINT32      nVol;                                                           */
/*  UINT32      nRet;                                                           */
/*  UINT32      nFuncType;                                                      */
/*                                                                              */
/*  nVol        = 0;                                                            */
/*  nFuncType   = FSR_BML_SET_NONBLOCKING_READ_EXCEPT;                          */
/*                                                                              */
/*  FSR_BML_IOCtl(nVol, FSR_BML_IOCTL_SET_NONBLOCKING, &nFuncType,              */
/*                sizeof(nFuncType), NULL, 0, &nRet)                            */
/********************************************************************************/
#define     FSR_BML_IOCTL_SET_NONBLOCKING   FSR_IOCTL_CODE(FSR_MODULE_BML,      \
                                                           0x0B,                \
                                                           FSR_METHOD_OUT_DIRECT,\
                                                           FSR_WRITE_ACCESS)

/********************************************************************************/
/*  FSR_BML_IOCtl(nVol, FSR_BML_IOCTL_SET_BLOCKING, NULL, 0, NULL, 0, &nRet)    */
/********************************************************************************/
#define     FSR_BML_IOCTL_SET_BLOCKING      FSR_IOCTL_CODE(FSR_MODULE_BML,      \
                                                           0x0C,                \
                                                           FSR_METHOD_OUT_DIRECT,\
                                                           FSR_WRITE_ACCESS)

/********************************************************************************/
/*  FSR_BML_IOCtl(nVol, FSR_BML_IOCTL_GET_NUMOFERB, NULL, 0,                    */
/*               nCnt, sizeof(UINT32), &nRet)                                   */
/********************************************************************************/
#define     FSR_BML_IOCTL_GET_NUMOFERB      FSR_IOCTL_CODE(FSR_MODULE_BML,      \
                                                           0x0D,                \
                                                           FSR_METHOD_BUFFERED, \
                                                           FSR_READ_ACCESS)

/********************************************************************************/
/*  PRIVATE BmlBMF     gstBmf0[FSR_MAX_BAD_BLKS];                               */
/*  PRIVATE BmlBMF     gstBmf1[FSR_MAX_BAD_BLKS];                               */
/*  PRIVATE UINT32     gstRCB0[FSR_MAX_BAD_BLKS/2];                             */
/*  PRIVATE UINT32     gstRCB1[FSR_MAX_BAD_BLKS/2];                             */
/*                                                                              */
/*  UINT32     nDevIdx;                                                         */
/*  UINT32     nDieIdx;                                                         */
/*  UINT32     nNumOfDevs;                                                      */
/*  UINT32     nRet;                                                            */
/*  FSRBMInfo  stBMInfo;                                                        */
/*                                                                              */
/*  stBMInfo.nNumOfDies    = 0;                                                 */
/*  stBMInfo.nNumOfBMFs[0] = 0;                                                 */
/*  stBMInfo.nNumOfBMFs[1] = 0;                                                 */
/*  stBMInfo.pstBMF[0]     = &gstBmf0[0];                                       */
/*  stBMInfo.pstBMF[1]     = &gstBmf1[0];                                       */
/*  stBMInfo.pstRCB[0]     = &gstRCB0[0];                                       */
/*  stBMInfo.pstRCB[1]     = &gstRCB1[0];                                       */
/*                                                                              */
/*  FSR_BML_IOCtl(nVol, FSR_BML_IOCTL_GET_PAIRED_BMI, (UINT8 *) &nDevIdx,       */
/*            sizeof(nDevIdx), (UINT8 *)&stPairedBMInfo, sizeof(FSRPairedBMInfo), */
/*            &nRet)                                                            */
/********************************************************************************/
#define     FSR_BML_IOCTL_GET_PAIRED_BMI    FSR_IOCTL_CODE(FSR_MODULE_BML,      \
                                                           0x0E,                \
                                                           FSR_METHOD_BUFFERED, \
                                                           FSR_READ_ACCESS)

/**
 *  @brief      Data structure for device specification structure in Volume
 */
typedef struct
{
    UINT16       nPgsPerSLCUnit;      /**< # of pages per SLC Unit           */
    UINT16       nPgsPerMLCUnit;      /**< # of pages per MLC Unit           */

    UINT8        nSctsPerPg;          /**< # of sectors per page             */
    UINT8        nNumOfWays;          /**< # of ways                         */

    UINT16       nNumOfUsUnits;       /**< # of usable units                 */

    BOOL32       bVolOpen;            /**< volume open flag                 */

    UINT16       nSparePerSct;        /**< spare size per sector             */
    UINT8        nDiesPerDev;         /**< the number of dies per device     */
    UINT8        nPlnsPerDie;         /**< the number of planes per Die      */

    UINT16       nNANDType;           /**< NAND types
                                           - FSR_LLD_FLEX_ONENAND
                                           - FSR_LLD_SLC_NAND
                                           - FSR_LLD_MLC_NAND
                                           - FSR_LLD_SLC_ONENAND             */

    UINT16       nFeature;            /**< Feature by NAND type              */

    UINT32       nSLCTLoadTime;       /**< Typical SLC Load time             */
    UINT32       nMLCTLoadTime;       /**< Typical MLC Load time             */
    UINT32       nSLCTProgTime;       /**< Typical SLC Program time          */
    UINT32       nMLCTProgTime[2];    /**< Typical MLC Program time          */
    UINT32       nTEraseTime;         /**< Typical Erase time                */

    /* endurance information */
    UINT32       nSLCPECycle;         /**< program, erase cycle of SLC block */
    UINT32       nMLCPECycle;         /**< program, erase cycle of MLC block */

    BOOL32       b1stBlkOTP;          /**< First block OTP supported         */
    UINT32       nUserOTPScts;        /**< # of user sectors                 */

    UINT32       nSizeOfDumpBuf;      /**< Maximum size of buffer for image dump */

    UINT16       nRsvrBlksPerDie;     /**< the number of reserved blocks per die */
    UINT16       nPad;

    UINT8        nUID[FSR_LLD_UID_SIZE];  /**< Uniqure ID in OTP area            */
} FSRVolSpec;

/**
 *  @brief      Data structure for Random-in argument of FSR_BML_CopyBack
 */
typedef struct
{
    UINT16       nOffset;          /**< nOffset should be even number
                                       main area range  :          0 ~ 8190
                                       spare area range : 0x4000 + 0 ~ 0      */
    UINT16       nNumOfBytes;      /**< Random In Bytes                       */
    VOID        *pBuf;             /**< Data Buffer Pointer                   */
} BMLRndInArg;

/**
 *  @brief      Data structure for FSR_BML_CopyBack
 */
typedef struct
{
    UINT16       nSrcVun;          /**< Source Vun                           */
    UINT16       nDstVun;          /**< Dest. Vun                            */
    UINT16       nSrcPgOffset;     /**< Src. page offset in virtual unit     */
    UINT16       nDstPgOffset;     /**< Dest. page offset in virtual unit    */
    UINT32       nRndInCnt;        /**< Random In Count                      */
    BMLRndInArg *pstRndInArg;      /**< RndInArg Array pointer               */
} BMLCpBkArg;

/**
 *  @brief      Data structure for partition entry structure
 */
typedef struct
{
    UINT16       nID;           /**< partition entry ID                      */
    UINT16       nAttr;         /**< attribute                               */
    UINT16       n1stVun;       /**< 1st virtual unit number                 */
    UINT16       nNumOfUnits;   /**< number of units                         */
    UINT32       nLoadAddr;     /**< Image in this partition can be loaded
                                     to this RAM address when 'nAttr' has
                                     'FSR_BML_PI_ATTR_BOOTLOADING' flag      */
    UINT32       nReserved;     /**< Reserved                                */
} FSRPartEntry;

/**
 *  @brief      Data structure for partition information structure
 */
typedef struct
{
    UINT8         aSig[FSR_BML_MAX_PARTSIG];        /**< signature of partition 
                                                        information                 */
    UINT32        nVer;                             /**< version of partition 
                                                        information                 */
    UINT32        nNumOfPartEntry;                  /**< # of partition entry       */
    FSRPartEntry  stPEntry[FSR_BML_MAX_PARTENTRY];  /**< Array for FSRPartEntry     */
    
    UINT8         aRsv[FSR_BML_MAX_PARTRSV];        /**< reserved area (not used)   */
} FSRPartI;

/**
 *  @brief      Data structure for partition extension information 
 */
typedef struct
{
    UINT32       nID;           /**< nID of partition information extension  */
    UINT32       nSizeOfData;   /**< size of pData                           */
    UINT8        nData[FSR_BML_MAX_PIEXT_DATA];
} FSRPIExt;

/**
 *  @brief      Data structure for the input parameter of FSR_BML_IOCTL_CHANGE_PART_ATTR
 *  @remark     nCode: FSR_BML_IOCTL_CHANGE_PART_ATTR
 */
typedef struct
{
    UINT32      nPartID;        /**< nID to change partition attributes     */
    UINT32      nNewAttr;       /**< New partition attributes               */
} FSRChangePA;

/**
 *  @brief      Data structure for reservoir meta block information
 */
typedef struct
{
    UINT16      nUPCB;          /**< Unlocked Pool Control Block            */
    UINT16      nLPCB;          /**< Locked Pool Control Block              */
    UINT16      nTPCB;          /**< Temporary Pool Control Block           */
    UINT16      nREF;           /**< Erase Rerefresh Block                  */
    UINT16      n1stBlkOfRsv;   /**< 1st block number of reservoir          */
    UINT16      nLastBlkOfRsv;  /**< last block number of reservoir         */
} FSRRsvrInfo;

/**
 *  @brief      Data structure for the output parameter of FSR_BML_IOCTL_GET_SLC_BOUNDARY
 *  @remark     nCode: FSR_BML_IOCTL_GET_SLC_BOUNDARY
 */
typedef struct
{
    UINT32      nNumOfDies;                     /**< # of dies              */
    UINT32      nLastSLCBlkNum[FSR_MAX_DIES];   /**< The last SLC Blk number
                                                     by die index           */
    UINT32      nPILockValue[FSR_MAX_DIES];     /**< Lock value
                                                {FSR_LLD_IOCTL_LOCK_PI, FSR_LLD_IOCTL_UNLOCK_PI}*/
} FSRSLCBoundary;

/**
 *  @brief      Data structure for the paramter of FSR_BML_AdjustPartInfo
 */
typedef struct
{
    FSRVolSpec      stVolSpec;                  /**< volume spec             */
    FSRSLCBoundary  stSLCBoundary[FSR_MAX_DEVS];/**< SLC boundary            */
} FSRFlashInfo;

/**
 *  @brief      Data structure for partition Entry List for image dump
 */
typedef struct
{
    UINT32          nNumOfPEntry;               /**< the number of partition entries  */
    FSRPartEntry    stPEntry[FSR_BML_MAX_PARTENTRY];  /**< Array for FSRPartEntry     */
} FSRDumpPEList;

/**
 * @brief       Data structure for storing the info. about BMF(Block Map Field)
 */
typedef struct BMF_
{
    UINT16     nSbn;            /**< Semi physical Block Number              */
    UINT16     nRbn;            /**< Replaced Block Number in BMI structure
                                     Replaced Block offset in BMS structure  */
} BmlBMF;

/**
 *  @brief      Output data structures of FSR_BML_IOCtl()
 *  @remark     nCode: FSR_BML_IOCTL_GET_BMI
 */
typedef struct
{
    UINT32         nNumOfDies;                  /**< # of dies              */
    BmlBMF        *pstBMF[FSR_MAX_DIES];        /**< Array pointer to BMF   */
    UINT32         nNumOfBMFs[FSR_MAX_DIES];    /**< # of BMFs by die index */
    FSRRsvrInfo    stRsvInfo[FSR_MAX_DIES];
} FSRBMInfo;

/**
 *  @brief      Output data structures of FSR_BML_IOCtl()
 *  @remark     nCode: FSR_BML_IOCTL_GET_PAIRED_BMI
 */
typedef struct
{
    UINT32         nNumOfDies;                  /**< # of dies              */
    BmlBMF        *pstBMF[FSR_MAX_DIES];        /**< Array pointer to BMF   */
    UINT32         nNumOfBMFs[FSR_MAX_DIES];    /**< # of BMFs by die index */
    UINT16        *pstRCB[FSR_MAX_DIES];        /**< Array pointer to RCB   */
    UINT32         nNumOfRCBs[FSR_MAX_DIES];    /**< # of RCBs by die index */
    FSRRsvrInfo    stRsvInfo[FSR_MAX_DIES];
} FSRPairedBMInfo;


/*****************************************************************************/
/* exported function prototype of BML                                        */
/*****************************************************************************/

/*****************************************************************************/
/* APIs for pre-programming                                                  */
/*****************************************************************************/
#if !(defined(FSR_NBL2) || defined(TINY_FSR))
INT32   FSR_BML_Format           (UINT32        nVol,
                                  FSRPartI     *pstPartI,
                                  UINT32        nFlag);
INT32   FSR_BML_AdjustPartInfo   (UINT32        nVol,
                                  FSRPartI     *pstPartIIn,
                                  FSRPartI     *pstPartIOut,
                                  FSRFlashInfo *pstFlashInfo,
                                  UINT32        nFlag);
#endif

/*****************************************************************************/
/* Major APIs                                                                */
/*****************************************************************************/
INT32   FSR_BML_Init             (UINT32        nFlag);
INT32   FSR_BML_Open             (UINT32        nVol,
                                  UINT32        nFlag);

#if !defined(FSR_NBL2)
INT32   FSR_BML_Close            (UINT32        nVol,
                                  UINT32        nFlag);
#endif

INT32   FSR_BML_Read             (UINT32        nVol,
                                  UINT32        nVpn,
                                  UINT32        nNumOfPgs,
                                  UINT8        *pMBuf,
                                  FSRSpareBuf  *pSBuf,
                                  UINT32        nFlag);
INT32   FSR_BML_ReadScts         (UINT32        nVol,
                                  UINT32        nVpn,
                                  UINT32        n1stSecOff,
                                  UINT32        nNumOfScts,
                                  UINT8        *pBuf,
                                  FSRSpareBuf  *pSBuf,
                                  UINT32        nFlag);

#if !(defined(FSR_NBL2) || defined(TINY_FSR))
INT32   FSR_BML_Write            (UINT32        nVol,
                                  UINT32        nVpn,
                                  UINT32        nNumOfPgs,
                                  UINT8        *pMBuf,
                                  FSRSpareBuf  *pSBuf,
                                  UINT32        nFlag);
INT32   FSR_BML_Erase            (UINT32        nVol,
                                  UINT32       *nVun,
                                  UINT32        nNumOfUnits, 
                                  UINT32        nFlag);
INT32   FSR_BML_FlushOp          (UINT32        nVol,
                                  UINT32        nFlag);
#endif
INT32   FSR_BML_GetVolSpec       (UINT32        nVol,
                                  FSRVolSpec   *pstVolSpec,
                                  UINT32        nFlag);
INT32   FSR_BML_LoadPIEntry      (UINT32        nVol,
                                  UINT32        nID,
                                  UINT32       *pn1stVpn,
                                  UINT32       *pnPgsPerUnit,
                                  FSRPartEntry *pstPartEntry);
#if !defined(FSR_NBL2)

INT32   FSR_BML_EraseRefresh     (UINT32        nVol,
                                  UINT32        nNumOfUnit,
                                  UINT32        nFlag);
#endif


/*****************************************************************************/
/* APIs only used in DPM                                                     */
/*****************************************************************************/

#if !(defined(FSR_NBL2) || defined(TINY_FSR))
VOID    FSR_BML_AcquireSM        (UINT32        nVol);
VOID    FSR_BML_ReleaseSM        (UINT32        nVol);
#endif

/*****************************************************************************/
/* APIs only used in STL                                                     */
/*****************************************************************************/
#if !(defined(FSR_NBL2) || defined(TINY_FSR))
INT32   FSR_BML_CopyBack           (UINT32        nVol,
                                    BMLCpBkArg   *pstBMLCpArg[FSR_MAX_WAYS],
                                    UINT32        nFlag);
INT32   FSR_BML_GetPairedVPgOff    (UINT32        nVol,
                                    UINT32        nVPgOff);
INT32   FSR_BML_GetVPgOffOfLSBPg   (UINT32        nVol,
                                    UINT32        nLSBPgOff);
#endif

#if !defined(FSR_NBL2)
BOOL32  FSR_BML_GetPartAttrChg     (UINT32        nVol,
                                    UINT32        nPartID,
                                    UINT32        nFlag);
VOID    FSR_BML_SetPartAttrChg     (UINT32        nVol,
                                    UINT32        nPartID,
                                    BOOL32        bSet,
                                    UINT32        nFlag);
#endif
/*****************************************************************************/
/* API for Non-blocking I/O mode                                             */
/*****************************************************************************/
INT32   FSR_BML_ResumeOp         (UINT32        nVol);

/*****************************************************************************/
/* API for image dump                                                        */
/*****************************************************************************/
INT32   FSR_BML_Dump             (UINT32         nVol,
                                  UINT32         nDumpType,
                                  UINT32         nDumpOrder,
                                  FSRDumpPEList *pstPEList,
                                  UINT8         *pBuf,
                                  UINT32        *pBytesReturned);

INT32
FSR_BML_GetDumpSize              (UINT32    nVol,
                                  UINT32   *pnDumpSize);
/*****************************************************************************/
/* OTP APIs                                                                  */
/*****************************************************************************/
#if !(defined(FSR_NBL2) || defined(TINY_FSR))
INT32   FSR_BML_OTPRead          (UINT32        nVol,
                                  UINT32        nVpn,
                                  UINT32        nNumOfPgs,
                                  UINT8        *pMBuf,
                                  FSRSpareBuf  *pSBuf,
                                  UINT32        nFlag);
INT32   FSR_BML_OTPWrite         (UINT32        nVol,
                                  UINT32        nVpn,
                                  UINT32        nNumOfPgs,
                                  UINT8        *pMBuf,
                                  FSRSpareBuf  *pSBuf,
                                  UINT32        nFlag);
#endif

/*****************************************************************************/
/* Extra APIs                                                                */
/*****************************************************************************/
#if !(defined(FSR_NBL2) || defined(TINY_FSR))
INT32   FSR_BML_StorePIExt       (UINT32        nVol,
                                  FSRPIExt     *pPIExt,
                                  UINT32        nFlag);
INT32   FSR_BML_LoadPIExt        (UINT32        nVol,
                                  FSRPIExt     *pPIExt,
                                  UINT32        nFlag);
INT32   FSR_BML_IOCtl            (UINT32        nVol,
                                  UINT32        nCode,
                                  UINT8        *pBufIn,
                                  UINT32        nLenIn,
                                  UINT8        *pBufOut,
                                  UINT32        nLenOut,
                                  UINT32       *pBytesReturned);
#if defined(FSR_BML_RWEXT)
INT32   FSR_BML_ReadExt          (UINT32        nVol,
                                  UINT32        nVpn,
                                  UINT32        nNumOfPgs,
                                  UINT8        *pMBuf,
                                  UINT8        *pSBuf,
                                  UINT32        nFlag);
INT32   FSR_BML_WriteExt         (UINT32        nVol,
                                  UINT32        nVpn,
                                  UINT32        nNumOfPgs,
                                  UINT8        *pMBuf,
                                  UINT8        *pSBuf,
                                  UINT32        nFlag);
#endif /* #if defined(FSR_BML_RWEXT) */
#endif
INT32   FSR_BML_GetVirUnitInfo   (UINT32        nVol,
                                  UINT32        nVun,
                                  UINT32       *pn1stVpn,
                                  UINT32       *pnPgsPerUnit);
INT32   FSR_BML_GetFullPartI     (UINT32        nVol,
                                  FSRPartI     *pstPartI);

/*****************************************************************************/
/* APIs for Suspend/Resume                                                   */
/*****************************************************************************/
#if defined(FSR_BML_SUPPORT_SUSPEND_RESUME)
INT32   FSR_BML_Suspend         ( UINT32        nVol,
                                  UINT32        nFlag );
INT32   FSR_BML_Resume          ( UINT32        nVol,
                                  UINT32        nFlag );
#endif  /* #if defined(FSR_BML_SUPPORT_SUSPEND_RESUME) */

#ifdef __cplusplus
}
#endif
#endif  /* _FSR_BML_H_ */
