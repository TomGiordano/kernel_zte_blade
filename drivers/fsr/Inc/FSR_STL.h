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
 * @file      FSR_STL.h
 * @brief     This file contains global type definitions, global macros,
 *            prototypes of functions for FSR_STL operation.
 * @author    Jeong-Uk Kang
 * @date      17-JAN-2007
 * @remark
 * REVISION HISTORY
 * @n  17-JAN-2007 [Jeong-Uk Kang] : first writing
 *
 */


#ifndef _FSR_STL_H_
#define _FSR_STL_H_

/*****************************************************************************/
/* Common Constant definitions                                               */
/*****************************************************************************/
#define FSR_MAX_STL_DFRG_GROUP              (9)

/*****************************************************************************/
/* FSR_STL Format Options                                                    */
/*****************************************************************************/
#define FSR_STL_FORMAT_NONE                 (1 << 0)
#define FSR_STL_FORMAT_REMEMBER_ECNT        (1 << 1)
#define FSR_STL_FORMAT_USE_ECNT             (1 << 2)
#define FSR_STL_FORMAT_HOT_SPOT             (1 << 3)    /* \deprecated{not supported, but for backward-compatability} */
#define FSR_STL_FORMAT_SET_NUM_INITFREE     (1 << 4)    /* Set the number of initial free units */
#define FSR_STL_FORMAT_SET_MIN_META         (1 << 5)    /* Set the number of minimum meta units */
#define FSR_STL_FORMAT_SECURE               (1 << 6)

/*****************************************************************************/
/* FSR_STL Flags for Read/Write/Delete                                       */
/*****************************************************************************/
/* default */
#define FSR_STL_FLAG_DEFAULT                (0)

/* for FSR_STL_Write */
#define FSR_STL_FLAG_WRITE_HOT_DATA         (1 << 0)
#define FSR_STL_FLAG_WRITE_COLD_DATA        (1 << 1)
#define FSR_STL_FLAG_WRITE_NO_LSB_BACKUP    (1 << 2)    /* Not supported yet */
#define FSR_STL_FLAG_WRITE_NO_CONFIRM       (1 << 3)    /* Not supported yet */

/* for FSR_STL_Delete */
#define FSR_STL_FLAG_DELETE_SECURE          (1 << 4)

/* for FSR_STL_Read/Write/Delete */
#define FSR_STL_FLAG_USE_SM                 (1 << 5)    /* \deprecated{not supported, but for backward-compatability} */

/* for FSR_STL_Open */
#define FSR_STL_FLAG_OPEN_READONLY          (1 << 6)
#define FSR_STL_FLAG_OPEN_READWRITE         (1 << 7)

/* for FSR_STL_IOCTL_READ_ECNT command of FSR_STL_IOCtl */
#define FSR_STL_META_MARK                   (0x80000000)

/*****************************************************************************/
/* FSR_STL Return Codes                                                      */
/*****************************************************************************/
/** general return code */
#define FSR_STL_SUCCESS                     FSR_RETURN_VALUE(0, 0, 0x0000, 0x0000)
#define FSR_STL_CLEAN_PAGE                  FSR_RETURN_VALUE(0, 0, 0x0000, 0x0001)
#define FSR_STL_ERROR                       FSR_RETURN_VALUE(1, 0, 0x0000, 0x0000)
#define FSR_STL_CRITICAL_ERROR              FSR_RETURN_VALUE(1, 0, 0x0001, 0x0000)

/** for FSR_STL_Init */
#define FSR_STL_ALREADY_INITIALIZED         FSR_RETURN_VALUE(1, 0, 0x0000, 0x0001)

/** for FSR_STL_Open */
#define FSR_STL_UNFORMATTED                 FSR_RETURN_VALUE(1, 0, 0x0000, 0x0002)

/** for FSR_STL_Format/Open */
#define FSR_STL_OUT_OF_MEMORY               FSR_RETURN_VALUE(1, 0, 0x0001, 0x0001)
#define FSR_STL_META_BROKEN                 FSR_RETURN_VALUE(1, 0, 0x0001, 0x0002)
#define FSR_STL_PROHIBIT_FORMAT_BY_GWL      FSR_RETURN_VALUE(1, 0, 0x0001, 0x0003)

/** for FSR_STL_IOCtl */
#define FSR_STL_UNSUPPORTED_IOCTL           FSR_RETURN_VALUE(1, 0, 0x0002, 0x0000)

/* common return code */
#define FSR_STL_INVALID_PARAM               FSR_RETURN_VALUE(1, 0, 0x0003, 0x0000)
#define FSR_STL_INVALID_VOLUME_ID           FSR_RETURN_VALUE(1, 0, 0x0003, 0x0001)
#define FSR_STL_INVALID_PARTITION_ID        FSR_RETURN_VALUE(1, 0, 0x0003, 0x0002)
#define FSR_STL_INVALID_PARAM_OPMODE        FSR_RETURN_VALUE(1, 0, 0x0003, 0x0003)
#define FSR_STL_INVALID_PAGE                FSR_RETURN_VALUE(1, 0, 0x0003, 0x0004)
#define FSR_STL_INVALID_PARAM_FLAG          FSR_RETURN_VALUE(1, 0, 0x0003, 0x0005)

#define FSR_STL_PARTITION_ERROR             FSR_RETURN_VALUE(1, 0, 0x0004, 0x0000)
#define FSR_STL_PARTITION_NOT_OPENED        FSR_RETURN_VALUE(1, 0, 0x0004, 0x0001)
#define FSR_STL_PARTITION_ALREADY_OPENED    FSR_RETURN_VALUE(1, 0, 0x0004, 0x0002)
#define FSR_STL_PARTITION_TOO_MANY_OPENED   FSR_RETURN_VALUE(1, 0, 0x0004, 0x0003)
#define FSR_STL_PARTITION_READ_ONLY         FSR_RETURN_VALUE(1, 0, 0x0004, 0x0004)
#define FSR_STL_PARTITION_LOCKED            FSR_RETURN_VALUE(1, 0, 0x0004, 0x0005)

/* Semaphore error code */
#define FSR_STL_SEMAPHORE_ERROR             FSR_RETURN_VALUE(1, 0, 0x0005, 0x0000)
#define FSR_STL_ACQUIRE_SM_ERROR            FSR_RETURN_VALUE(1, 0, 0x0005, 0x0001)
#define FSR_STL_RELEASE_SM_ERROR            FSR_RETURN_VALUE(1, 0, 0x0005, 0x0002)

/* for FSR_STL_Read */
#define FSR_STL_USERDATA_ERROR              FSR_RETURN_VALUE(1, 0, 0x0006, 0x0000)

/* for write buffer */
#define FSR_STL_WB_IS_NULL                  FSR_RETURN_VALUE(1, 0, 0x0007, 0x0000)
#define FSR_STL_WB_IS_FULL                  FSR_RETURN_VALUE(1, 0, 0x0007, 0x0001)
#define FSR_STL_WB_IS_WORN_OUT              FSR_RETURN_VALUE(1, 0, 0x0007, 0x0002)

/* Internal error return code */
#define FSR_STL_ERR_NO_LOG                  FSR_RETURN_VALUE(1, 0, 0x0F00, 0x0000)
#define FSR_STL_ERR_LOGGRP                  FSR_RETURN_VALUE(1, 0, 0x0F00, 0x0001)
#define FSR_STL_ERR_NEW_LOGGRP              FSR_RETURN_VALUE(1, 0, 0x0F00, 0x0002)
#define FSR_STL_ERR_PMT                     FSR_RETURN_VALUE(1, 0, 0x0F00, 0x0003)
#define FSR_STL_ERR_PMT_NO_ENTRY            FSR_RETURN_VALUE(1, 0, 0x0F00, 0x0004)

#if defined (FSR_STL_FOR_PRE_PROGRAMMING)
/* For pre-programming */
#define FSR_STL_ALREADY_DEFRAGMENTED        FSR_RETURN_VALUE(1, 0, 0x0100, 0x0001)
#endif  /* defined (FSR_STL_FOR_PRE_PROGRAMMING) */

/*****************************************************************************/
/* STL IOCtl function codes                                                  */
/*****************************************************************************/

/*****************************************************************************/
/*  UINT32       nVol;                                                       */
/*  UINT32       nPartID;                                                    */
/*  UINT32       nBytesReturned;                                             */
/*  UINT32       nWearLevelThreshold;                                        */
/*                                                                           */
/*  nVol    = 0;                                                             */
/*  nPartID = FSR_PARTID_STL0;                                               */
/*                                                                           */
/*  FSR_STL_IOCtl  (nVol, nPartID, FSR_STL_IOCTL_GET_WL_THRESHOLD,           */
/*                  NULL, 0, (VOID *)                                        */
/*                  &nWearLevelThreshold, sizeof(nWearLevelThreshold),       */
/*                  &nBytesReturned);                                        */
/*****************************************************************************/
#define FSR_STL_IOCTL_GET_WL_THRESHOLD      FSR_IOCTL_CODE(FSR_MODULE_STL, 1,   \
                                                        FSR_METHOD_OUT_DIRECT,  \
                                                        FSR_READ_ACCESS)

/*****************************************************************************/
/*  UINT32       nVol;                                                       */
/*  UINT32       nPartID;                                                    */
/*  UINT32       nIdx;                                                       */
/*  UINT32       n1stVpn;                                                    */
/*  UINT32       nPgsPerUnit;                                                */
/*  UINT32       nBytesReturned;                                             */
/*  UINT32       n1stVun;                                                    */
/*  UINT32      *pnaECnt = NULL;                                             */
/*  UINT32       nNumOfECnt;                                                 */
/*  UINT32       nSizeOfArray;                                               */
/*  FSRPartEntry stPartEntry;                                                */
/*                                                                           */
/*  nVol    = 0;                                                             */
/*  nPartID = FSR_PARTID_STL0;                                               */
/*                                                                           */
/*  FSR_BML_LoadPIEntry(nVol, nPartID, &n1stVpn, &nPgsPerUnit, &stPartEntry);*/
/*  n1stVun = stPartEntry.n1stVun;                                           */
/*                                                                           */
/*  FSR_STL_IOCtl  (nVol, nPartID, FSR_STL_IOCTL_GET_ECNT_BUFFER_SIZE,       */
/*                  NULL, 0, (VOID *) &nNumOfECnt, sizeof(nNumOfECnt),       */
/*                  &nBytesReturned);                                        */
/*                                                                           */
/*  nSizeOfArray = nNumOfECnt * sizeof(UINT32);                              */
/*  pnaECnt      = (UINT32 *) FSR_OAM_Malloc(nSizeOfArray);                  */
/*                                                                           */
/*  FSR_STL_IOCtl  (nVol, nPartID, FSR_STL_IOCTL_READ_ECNT,                  */
/*                  NULL, 0, (VOID *) pnaECnt, nSizeOfArray,                 */
/*                  &nBytesReturned);                                        */
/*                                                                           */
/*  for (nIdx = 0; nIdx < nNumOfECnt; nIdx++)                                */
/*  {                                                                        */
/*      FSR_DBZ_DBGMOUT(FSR_DBZ_APP,                                         */
/*                      (TEXT("           Vun[%04d] : ECnt(%08d)\r\n"),      */
/*                      n1stVun + nIdx, pnaECnt[nIdx]));                     */
/*  }                                                                        */
/*                                                                           */
/*  FSR_OAM_Free(pnaECnt);                                                   */
/*****************************************************************************/
#define FSR_STL_IOCTL_READ_ECNT             FSR_IOCTL_CODE(FSR_MODULE_STL, 2,   \
                                                        FSR_METHOD_OUT_DIRECT,  \
                                                        FSR_READ_ACCESS)

#define FSR_STL_IOCTL_GET_ECNT_BUFFER_SIZE  FSR_IOCTL_CODE(FSR_MODULE_STL, 3,   \
                                                        FSR_METHOD_OUT_DIRECT,  \
                                                        FSR_READ_ACCESS)

/*****************************************************************************/
/*  UINT32       nVol;                                                       */
/*  UINT32       nPartID;                                                    */
/*  UINT32       nBytesReturned;                                             */
/*  UINT32       nNumOfLogScts;                                              */
/*                                                                           */
/*  nVol    = 0;                                                             */
/*  nPartID = FSR_PARTID_STL0;                                               */
/*                                                                           */
/*  FSR_STL_IOCtl  (nVol, nPartID, FSR_STL_IOCTL_LOG_SECTS,                  */
/*                  NULL, 0, (VOID *) &nNumOfLogScts, sizeof(nNumOfLogScts), */
/*                  &nBytesReturned);                                        */
/*****************************************************************************/
#define FSR_STL_IOCTL_LOG_SECTS             FSR_IOCTL_CODE(FSR_MODULE_STL, 4,   \
                                                        FSR_METHOD_OUT_DIRECT,  \
                                                        FSR_READ_ACCESS)

/*****************************************************************************/
/*  UINT32       nVol;                                                       */
/*  UINT32       nPartID;                                                    */
/*  UINT32       nBytesReturned;                                             */
/*  UINT32       nPageSize;                                                  */
/*                                                                           */
/*  nVol    = 0;                                                             */
/*  nPartID = FSR_PARTID_STL0;                                               */
/*                                                                           */
/*  FSR_STL_IOCtl  (nVol, nPartID, FSR_STL_IOCTL_PAGE_SIZE,                  */
/*                  NULL, 0, (VOID *) &nPageSize, sizeof(nPageSize),         */
/*                  &nBytesReturned);                                        */
/*****************************************************************************/
#define FSR_STL_IOCTL_PAGE_SIZE             FSR_IOCTL_CODE(FSR_MODULE_STL, 5,   \
                                                        FSR_METHOD_OUT_DIRECT,  \
                                                        FSR_READ_ACCESS)

/*****************************************************************************/
/*  UINT32          nVol;                                                    */
/*  UINT32          nPartID;                                                 */
/*  UINT32          nStlPartFlag;                                            */
/*                                                                           */
/*  nVol                = 0 or 1;                                            */
/*  nStlPartFlag        = FSR_STL_FLAG_OPEN_READONLY or                      */
/*                        FSR_STL_FLAG_OPEN_READWRITE or                     */
/*                        FSR_STL_FLAG_DEFAULT ;                             */
/*                                                                           */
/*  FSR_STL_IOCtl  (nVol, nPartID, FSR_STL_IOCTL_CHANGE_PART_ATTR,           */
/*                  (VOID *) &nStlPartFlag, sizeof(nStlPartFlag), NULL, 0,   */
/*                  NULL);                                                   */
/*****************************************************************************/
#define FSR_STL_IOCTL_CHANGE_PART_ATTR      FSR_IOCTL_CODE(FSR_MODULE_STL, 8,   \
                                                        FSR_METHOD_IN_DIRECT,   \
                                                        FSR_WRITE_ACCESS)

/*****************************************************************************/
/*  UINT32       nVol;                                                       */
/*  UINT32       nPartID;                                                    */
/*  UINT32       nIdx;                                                       */
/*  UINT32       n1stVpn;                                                    */
/*  UINT32       nPgsPerUnit;                                                */
/*  UINT32       nBytesReturned;                                             */
/*  UINT32       n1stVun;                                                    */
/*  FSRStlStats  stStlStats                                                  */
/*  UINT32       nNumOfECnt;                                                 */
/*  UINT32       nSizeOfArray;                                               */
/*  FSRPartEntry stPartEntry;                                                */
/*                                                                           */
/*  nVol    = 0;                                                             */
/*  nPartID = FSR_PARTID_STL0;                                               */
/*                                                                           */
/*  FSR_BML_LoadPIEntry(nVol, nPartID, &n1stVpn, &nPgsPerUnit, &stPartEntry);*/
/*  n1stVun = stPartEntry.n1stVun;                                           */
/*                                                                           */
/*  FSR_STL_IOCtl  (nVol, nPartID, FSR_STL_IOCTL_STATS_RESET,                */
/*                   NULL, 0, NULL, 0, &nBytesReturned);                     */
/*                                                                           */
/*  // Some Stl Operation                                                    */
/*                                                                           */
/*  FSR_STL_IOCtl  (nVol, nPartID, FSR_STL_IOCTL_STATS_GET,                  */
/*                   NULL, 0, &stStlStats, sizeof(stStlStats),               */
/*                         &nBytesReturned);                                 */
/*****************************************************************************/
#define FSR_STL_IOCTL_RESET_STATS           FSR_IOCTL_CODE(FSR_MODULE_STL, 9,   \
                                                        FSR_METHOD_OUT_DIRECT,  \
                                                        FSR_READ_ACCESS)

#define FSR_STL_IOCTL_GET_STATS             FSR_IOCTL_CODE(FSR_MODULE_STL, 10,  \
                                                        FSR_METHOD_OUT_DIRECT,  \
                                                        FSR_READ_ACCESS)

/*****************************************************************************/
/*  UINT32       nVol;                                                       */
/*  UINT32       nPartID;                                                    */
/*  UINT32       naWBInfo[2];                                                */
/*  UINT32       nTotalBlksOfWB;                                             */
/*  UINT32       nUsedBlksOfWB;                                              */
/*  UINT32       nBytesReturned;                                             */
/*                                                                           */
/*  nVol         = 0;                                                        */
/*  nPartID      = FSR_PARTID_STL0;                                          */
/*                                                                           */
/*  FSR_STL_IOCtl  (nVol, nPartID, FSR_STL_IOCTL_GET_WB_INFO                 */
/*                  NULL, 0, (VOID *) &nWBInfo[0], sizeof(UINT32) * 2,       */
/*                  &nBytesReturned);                                        */
/*                                                                           */
/*  nTotalBlksOfWB = naWBInfo[0];                                            */
/*  nUsedBlksOfWB  = naWBInfo[1];                                            */
/*                                                                           */
/*  FSR_STL_IOCtl  (nVol, nPartID, FSR_STL_IOCTL_FLUSH_WB                    */
/*                  (VOID *) &nUsedBlksOfWB, sizeof(UINT32), NULL, 0,        */
/*                  &nBytesReturned);                                        */
/*****************************************************************************/
#define FSR_STL_IOCTL_GET_WB_INFO           FSR_IOCTL_CODE(FSR_MODULE_STL, 11,  \
                                                        FSR_METHOD_IN_DIRECT,   \
                                                        FSR_WRITE_ACCESS)

#define FSR_STL_IOCTL_FLUSH_WB              FSR_IOCTL_CODE(FSR_MODULE_STL, 12,  \
                                                        FSR_METHOD_IN_DIRECT,   \
                                                        FSR_WRITE_ACCESS)

/*****************************************************************************/
/*  UINT32       nVol;                                                       */
/*  UINT32       nPartID;                                                    */
/*  UINT32       nNumOfBlk;                                                  */
/*                                                                           */
/*  nVol         = 0;                                                        */
/*  nPartID      = FSR_PARTID_STL0;                                          */
/*  nNumOfBlk    = 10;                                                       */
/*                                                                           */
/*  FSR_STL_IOCtl  (nVol, nPartID, FSR_STL_IOCTL_RUN_ER,                     */
/*                  (VOID *) &nNumOfBlk, sizeof(nNumOfBlk), NULL, 0,         */
/*                  NULL);                                                   */
/*****************************************************************************/
#define FSR_STL_IOCTL_RUN_ER                FSR_IOCTL_CODE(FSR_MODULE_STL, 13,  \
                                                        FSR_METHOD_IN_DIRECT,   \
                                                        FSR_WRITE_ACCESS)

/*****************************************************************************/
/* STL Data Structures                                                       */
/*****************************************************************************/

/**
 * @brief       Data structure for the parameter of FSR_STL_Format
 */
typedef struct
{
    UINT32      nOpt;               /**< FSR_STL_FORMAT_NONE                                /
                                         FSR_STL_FORMAT_REMEMBER_ECNT                       /
                                         FSR_STL_FORMAT_USE_ECNT                            /
                                         FSR_STL_FORMAT_SET_NUM_INITFREE                    /
                                         FSR_STL_FORMAT_SECURE                              */

    UINT32      nStartSctsOfHot;    /**< \deprecated{not supported, but for backward-compatability} */
    UINT32      nNumOfSctsOfHot;    /**< \deprecated{not supported, but for backward-compatability} */

    UINT32      nAvgECnt;           /**< If nOpt has FSR_STL_FORMAT_REMEMBER_ECNT,
                                         this value is used when we cannot retrieve the original erase count */

    UINT32      nNumOfECnt;         /**< If nOpt has FSR_STL_FORMAT_USE_ECNT,
                                         this value is used for the array size of erase count */

    UINT32     *pnECnt;             /**< If nOpt has FSR_STL_FORMAT_USE_ECNT,
                                         this value is used to set erase count of each unit  */

    UINT16      nNumOfInitFreeUnits;/**< If nOpt has FSR_STL_FORMAT_SET_NUM_INITFREE,
                                         this value is used for the number of initial free units, 
                                         which is the same as reserved unit in XSR          */

    UINT16      nNumOfMinMetaUnits; /**< If nOpt has FSR_STL_FORMAT_SET_MIN_META,
                                         this value is used for the minimum number of meta units */
} FSRStlFmtInfo;

/**
 * @brief       Data structure for the parameter of FSR_STL_Open
 */
typedef struct
{
    UINT32  nLogSctsPerPage;                /**< The number of sectors per a page           */
    UINT32  nLogSctsPerUnit;                /**< The number of sectors per a unit           */
    UINT32  nTotalLogScts;                  /**< The number of total logical sectors        */
    UINT32  nTotalLogUnits;                 /**< The number of total logical units          */
} FSRStlInfo;

/**
 * @brief       Data structure for the parameter of FSR_STL_IOCTL_GET_STATS
 */
typedef struct
{    
    UINT32          nZoneRdPgs;             /**< Zone read pages                            */
    UINT32          nZoneWrPgs;             /**< Zone write pages                           */

    UINT32          nWBRdPgs;               /**< WB read pages                              */
    UINT32          nWBWrPgs;               /**< WB write pages                             */

    UINT32          nLogECnt;               /**< Log block erase count                      */
    UINT32          nMetaECnt;              /**< Meta block erase count                     */
    UINT32          nWBECnt;                /**< WB block erase count                       */

    UINT32          nFreeLogCnt;            /**< Free log count (in log reclaim)            */
    UINT32          nCompactLogCnt;         /**< Compact log count (in log reclaim)         */
    UINT32          nMergeLogCnt;           /**< Merge log count (in log reclaim)           */
    UINT32          nRefreshLogCnt;         /**< Refresh log count (in log reclaim)         */
    UINT32          nReclaimMetaCnt;        /**< Meta reclaim count                         */ 

    UINT32          nLogPgmPgs;             /**< Normal log program pages                   */
    UINT32          nLogPgmPgsByReclaim;    /**< Log program pages by log reclaim           */
    UINT32          nLogPgmPgsByWearLevel;  /**< Log program pages by wear-leveling         */
    UINT32          nLogPgmPgsByWBFlush;    /**< Log program pages by WB flush              */

    UINT32          nCtxPgmPgs;             /**< Normal meta page program count             */
    UINT32          nCtxPgmPgsByReclaim;    /**< Meta program pages by meta reclaim         */

    UINT32          nLocalWearLevelCnt;     /**< Local wearleveling count                   */
    UINT32          nGlobalWearLevelCnt;    /**< Global wearleveling count                  */
    UINT32          nMetaWearLevelCnt;      /**< Meta wearleveling count                    */
} FSRStlStats;

#if defined(FSR_STL_FOR_PRE_PROGRAMMING)
/**
 * @brief       Data structure for the parameter of FSRStlDfrg
 */
typedef struct
{
    UINT32  nStartVun;      /**< Output : the start virtual unit number        */
    UINT32  nNumOfVun;      /**< Output : the number of virtual units          */
} FSRStlUnitGrp;

/**
 * @brief       Data structure for the parameter of FSR_STL_Defragment
 */
typedef struct
{
    UINT32          nNumGroup;
    FSRStlUnitGrp   staGroup[FSR_MAX_STL_DFRG_GROUP];
} FSRStlDfrg;

typedef VOID (*FSRStlPrgFp)(UINT32 nMaxStage, UINT32 nCurrentStage, INT8 *szStageDesc);

#endif  /* defined (FSR_STL_FOR_PRE_PROGRAMMING) */

/*****************************************************************************/
/* External Functions                                                        */
/*****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

PUBLIC INT32    FSR_STL_Init   (VOID);
PUBLIC INT32    FSR_STL_Format (UINT32          nVol,
                                UINT32          nPartID,
                                FSRStlFmtInfo  *pstStlFmt);
PUBLIC INT32    FSR_STL_Open   (UINT32          nVol,
                                UINT32          nPartID,
                                FSRStlInfo     *pstSTLInfo,
                                UINT32          nFlag);
PUBLIC INT32    FSR_STL_Close  (UINT32          nVol,
                                UINT32          nPartID);
PUBLIC INT32    FSR_STL_Read   (UINT32          nVol,
                                UINT32          nPartID,
                                UINT32          nLsn,
                                UINT32          nNumOfScts,
                                UINT8          *pBuf,
                                UINT32          nFlag);
PUBLIC INT32    FSR_STL_Write  (UINT32          nVol,
                                UINT32          nPartID,
                                UINT32          nLsn,
                                UINT32          nNumOfScts,
                                UINT8          *pBuf,
                                UINT32          nFlag);
PUBLIC INT32    FSR_STL_Delete (UINT32          nVol,
                                UINT32          nPartID,
                                UINT32          nLsn,
                                UINT32          nNumOfScts,
                                UINT32          nFlag);
PUBLIC INT32    FSR_STL_IOCtl  (UINT32          nVol,
                                UINT32          nPartID,
                                UINT32          nCode,
                                VOID           *pBufIn,
                                UINT32          nLenIn,
                                VOID           *pBufOut,
                                UINT32          nLenOut,
                                UINT32         *pBytesReturned);

#if defined (FSR_STL_FOR_PRE_PROGRAMMING)
PUBLIC INT32    FSR_STL_Defragment (UINT32      nVol,
                                    UINT32      nPartID,
                                    FSRStlDfrg *pstSTLDfra,
                                    FSRStlPrgFp pfProgress,
                                    UINT32      nFlag);
#endif  /* defined (FSR_STL_FOR_PRE_PROGRAMMING) */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _FSR_STL_H_ */
