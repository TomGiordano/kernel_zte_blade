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
 * @file      FSR_DBG.h
 * @brief     This file contains debug zone, stack measurement, etc
 * @author    SongHo Yoon
 * @date      11-JAN-2007
 * @remark
 * REVISION HISTORY
 * @n  11-JAN-2007 [SongHo Yoon] : first writing
 *
 */

#ifndef _FSR_DBG_H_
#define _FSR_DBG_H_

#ifdef __cplusplus
extern "C" {                                    /* C declarations in C++     */
#endif

/*****************************************************************************/
/* LLD debug zone mask                                                       */
/*****************************************************************************/
#define     FSR_DBZ_LLD_IF                      0x00000001
#define     FSR_DBZ_LLD_INF                     0x00000002
#define     FSR_DBZ_LLD_LOG                     0x00000004
#define     FSR_DBZ_LLD_RSV1                    0x00000008
#define     FSR_DBZ_LLD_ALLZONE                 (FSR_DBZ_LLD_IF  | \
                                                 FSR_DBZ_LLD_INF | \
                                                 FSR_DBZ_LLD_LOG)

/*****************************************************************************/
/* BML debug zone mask                                                       */
/*****************************************************************************/
#define     FSR_DBZ_BML_IF                      0x00000010
#define     FSR_DBZ_BML_BBM                     0x00000020
#define     FSR_DBZ_BML_LOG                     0x00000040
#define     FSR_DBZ_BML_INF                     0x00000080
#define     FSR_DBZ_BML_ALLZONE                 (FSR_DBZ_BML_IF  | \
                                                 FSR_DBZ_BML_BBM | \
                                                 FSR_DBZ_BML_INF | \
                                                 FSR_DBZ_BML_LOG)

/*****************************************************************************/
/* STL debug zone mask                                                       */
/*****************************************************************************/
#define     FSR_DBZ_STL_IF                      0x00000100
#define     FSR_DBZ_STL_WBM                     0x00000200
#define     FSR_DBZ_STL_OPN                     0x00000400
#define     FSR_DBZ_STL_MDM                     0x00000800
#define     FSR_DBZ_STL_VUM                     0x00001000
#define     FSR_DBZ_STL_WEARLV                  0x00002000
#define     FSR_DBZ_STL_INF                     0x00004000
#define     FSR_DBZ_STL_LOG                     0x00008000
#define     FSR_DBZ_STL_ALLZONE                 (FSR_DBZ_STL_IF     | \
                                                 FSR_DBZ_STL_WBM    | \
                                                 FSR_DBZ_STL_OPN    | \
                                                 FSR_DBZ_STL_MDM    | \
                                                 FSR_DBZ_STL_VUM    | \
                                                 FSR_DBZ_STL_WEARLV | \
                                                 FSR_DBZ_STL_INF    | \
                                                 FSR_DBZ_STL_LOG)

/*****************************************************************************/
/* disk driver layer debug zone mask                                         */
/*****************************************************************************/
#if defined(FSR_SYMOS_OAM)
#define     FSR_DBZ_MD                          0x00010000
#define     FSR_DBZ_MD_PI                       0x00020000
#define     FSR_DBZ_MD_CAPS                     0x00040000
#define     FSR_DBZ_MD_INFO                     0x00080000
#define     FSR_DBZ_MD_ROFSINFO                 0x00100000
#define     FSR_DBZ_MD_ALLZONE                 (FSR_DBZ_MD      | \
                                                FSR_DBZ_MD_PI   | \
                                                FSR_DBZ_MD_CAPS | \
                                                FSR_DBZ_MD_INFO | \
                                                FSR_DBZ_MD_ROFSINFO)
#elif defined(FSR_WINCE_OAM)

#elif defined(FSR_LINUX_OAM)

#endif

/*****************************************************************************/
/* common debug zone mask                                                    */
/*****************************************************************************/
#define     FSR_DBZ_APP                         0x10000000
#define     FSR_DBZ_INF                         0x20000000
#define     FSR_DBZ_WARN                        0x40000000
#define     FSR_DBZ_ERROR                       0x80000000
#define     FSR_DBZ_ALL_ENABLE                  0xFFFFFFFF
#define     FSR_DBZ_ALL_DISABLE                 0x00000000

/*****************************************************************************/
/* default debug zone mask                                                   */
/*****************************************************************************/
#define     FSR_DBZ_DEFAULT                     FSR_DBZ_ERROR


/*****************************************************************************/
/* the message out macros                                                    */
/*****************************************************************************/
#define     FSR_DBGMASK                         (gnFSRDbgZoneMask)

#if defined(FSR_OAM_RTLMSG_DISABLE)
#define     FSR_DBZ_RTLMOUT(mask, x)            {if(FSR_DBZ_DEFAULT & (mask)) FSR_RTL_PRINT(x);}
#else
#define     FSR_DBZ_RTLMOUT(mask, x)            {if(FSR_DBGMASK & (FSR_DBZ_DEFAULT | (mask))) FSR_RTL_PRINT(x);}
#endif

#if defined(FSR_OAM_DBGMSG_ENABLE)
#define     FSR_DBZ_DBGMOUT(mask, x)            {if(FSR_DBGMASK & (mask)) FSR_DBG_PRINT(x);}
#else
#define     FSR_DBZ_DBGMOUT(mask, x)
#endif

/*****************************************************************************/
/* macro for measuring stack depth                                           */
/*****************************************************************************/
#if defined(FSR_DBG_STACKUSAGE)
    #define FSR_INIT_STACKDEPTH                 FSR_DBG_InitStackDepth()
    #define FSR_STACK_VAR                       UINT32 nStackVar
    #define FSR_STACK_BEGIN                     FSR_DBG_RecordStackStart(&nStackVar, (UINT8 *) __FUNCTION__)
    #define FSR_STACK_END                       FSR_DBG_RecordStackEnd(&nStackVar, (UINT8 *) __FUNCTION__)
    #define FSR_GET_STACKUSAGE(a1,a2,stackdep)  FSR_DBG_GetStackUsage(a1,a2,stackdep)
#else /* FSR_DBG_STACKUSAGE */
    #define FSR_INIT_STACKDEPTH                 {}
    #define FSR_STACK_VAR                       {}
    #define FSR_STACK_BEGIN                     {}
    #define FSR_STACK_END                       {}
    #define FSR_GET_STACKUSAGE(a1,a2,stactdep)  {}
#endif /* FSR_DBG_STACKUSAGE */

#define FSR_DBG_BEGIN_TIMER                     FSR_OAM_StartTimer()
#define FSR_DBG_END_TIMER                       FSR_OAM_StopTimer();    \
                                                FSR_DBZ_RTLMOUT(FSR_DBZ_DEFAULT, (TEXT("[DBG:   ] --%s() : 0x%08x\r\n"), __FSR_FUNC__, bFind))

/* macros for BML volume dump */
#define     FSR_DUMP_HDR_SIG                    "FSRDHSIG"
#define     FSR_DUMP_HDR_SIG_SIZE               (8)
#define     FSR_DUMP_HDR_VER                    (UINT32) (0x01000000)
#define     FSR_SPARE_SIZE                      (16)
#define     FSR_MAX_PAGE_SIZE                   (FSR_MAX_PHY_SCTS * FSR_MAX_PLANES * \
                                                (FSR_SECTOR_SIZE  + FSR_SPARE_SIZE))
#define     FSR_MAX_DUMP_DEVS                   (FSR_MAX_DEVS / FSR_MAX_VOLS)
#define     FSR_MAX_DUMP_DIES                   (FSR_MAX_DIES)
#define     FSR_MAX_DUMP_RSVS                   (2)
#define     FSR_MAX_DUMP_RCB                    (FSR_MAX_BAD_BLKS / 2)
#define     FSR_MAX_DUMP_PARTITION_ENTRY        ((512 - 32)/16 + FSR_MAX_DUMP_RSVS) + \
                                                (FSR_MAX_DUMP_RCB)
#define     FSR_MAX_DUMP_DATA_TYPE              (5)

/* Block type for FSRDumpPEntry */
#define     FSR_DUMP_SLC_BLOCK                  (0x00000000)
#define     FSR_DUMP_MLC_BLOCK                  (0x00000001)

/*****************************************************************************/
/* Swapping macro definition                                                 */
/*****************************************************************************/
#if defined(FSR_SUPPORT_BE_IMAGE_FOR_FLEXIA) || defined(FSR_SUPPORT_BE_IMAGE_BE_PLATFORM)


#define         ENDIAN_SWAP16(X)        { \
                                            FSR_ASSERT(sizeof(X) == 2); \
                                            ((X) = ((((X) & 0xFF00) >> 8) | (((X) & 0x00FF) << 8))); \
                                        }

#define         ENDIAN_SWAP32(X)        { \
                                            FSR_ASSERT(sizeof(X) == 4); \
                                            ( (X) = ((((X) & 0xFF000000) >> 24) | \
                                                 (((X) & 0x00FF0000) >> 8)   | \
                                                 (((X) & 0x0000FF00) << 8)   | \
                                                 (((X) & 0x000000FF) << 24)) ); \
                                        }
#endif  /* defined(FSR_SUPPORT_BE_IMAGE_FOR_FLEXIA) || defined(FSR_SUPPORT_BE_IMAGE_BE_PLATFORM) */


/*****************************************************************************/
/* #pragma options                                                           */
/*****************************************************************************/
#if defined(_WIN32)
#pragma warning (disable : 4127)        /* conditional expression is constant*/
#pragma warning (disable : 4100)        /* unreferenced formal parameter    */
#endif

/**
 *  @brief      parameter of FSR_PAM_GetPAParm()
 */
typedef struct
{
    UINT32  nBaseAddr[FSR_MAX_DEVS/FSR_MAX_VOLS];
                                    /**< the base address for accessing NAND 
                                         device                              */
    UINT32  nEndianType;            /**< Endian Type                         */
    UINT32  nPad;                   /**< reserved area                       */

    UINT32  nDevsInVol;             /**< number of devices in the volume     */

    VOID   *pExInfo;                /**< For Device Extension.
                                         For Extra Information of Device,
                                         data structure can be mapped.       */
} FSRDumpVolParm;

/**
 * @brief  Device structure for image dump 
 */
typedef struct
{
    UINT16      nNumOfBlks;     /**< # of blocks in NAND device         */
    UINT16      nNumOfPlanes;   /**< # of planes in die                 */

    UINT16      nSparePerSct;   /**< # of bytes of spare of a sector    */
    UINT16      nSctsPerPG;     /**< # of sectors per page              */

    UINT32      nPgsPerBlkForSLC;/**< # of pages per block in SLC area  */
    UINT32      nPgsPerBlkForMLC;/**< # of pages per block in MLC area  */

    UINT16      nNumOfDies;     /**< # of dies in NAND device           */
    UINT16      nNANDType;      /**< NAND Type of device                */

    UINT16      nDID;           /**< Device ID                          */

    UINT16      nPad;           /**< Pad                                */
} FSRDumpDevCxt;


/**
 * @brief  Fraction entry structure for image dump
 */
typedef struct
{
    UINT32      n1stPbn;        /**< 1st physical block number              */
    UINT32      nNumOfPbn;      /**< the number of blocks to dump           */
    UINT32      nBlkType;       /**< Block type (SLC/MLC)                   */
    UINT32      nPEOffset;      /**< Dump PE start offset in a die          */
    UINT32      nPESize;        /**< Dump partition Entry size in a die     */
} FSRDumpPEntry;

/**
 * @brief  BML volume dump header for die
 */
typedef struct
{
    UINT32          nDieOffset;         /**< Die offset in a file                   */
    UINT32          nFlagCellOffset;    /**< Flag Cell offset in a file             */
    UINT32          nFlagCellSize;      /**< Flag Cell size in a file               */
    UINT32          nOTPBlkOffset;      /**< OTP block offset in a file             */
    UINT32          nOTPBlkSize;        /**< OTP block size in a file               */
    UINT32          nPIBlkOffset;       /**< PI block offset in a file              */
    UINT32          nPIBlkSize;         /**< PI block size in a file                */

    UINT32          nDataAreaOffset;    /**< Data area offset in a file             */
    UINT32          nDataAreaSize;      /**< Data area size  in a file              */
    UINT32          nNumOfPEntry;       /**< The number of dump partition entries   */
    FSRDumpPEntry   stPEntry[FSR_MAX_DUMP_PARTITION_ENTRY]; /**< Data structure
                                                    for dump partition entries      */

    UINT32          nLsbDataAreaOffset; /**< Lsb data area offset in a file         */
    UINT32          nLsbDataAreaSize;   /**< Lsb data area offset in a file         */
    UINT32          nRcvdLsbPgs;        /**< the number of pnLsbData array          */
} FSRDieDumpHdr;

/**
 * @brief  BML volume dump header for device
 * @remark BML volume dump image structure
 * @n
 * @n  +----------------------+
 * @n  |    FSRVolDumpHdr     |
 * @n  +----------------------+  <<------+
 * @n  |       Flag Cell      |          |
 * @n  +----------------------+          |
 * @n  |      OTP block       |          |
 * @n  +----------------------+          |
 * @n  |       PI block       |          |
 * @n  +----------------------+          |
 * @n  |                      |          +--> for each die
 * @n  |                      |          |
 * @n  |       Data area      |          |
 * @n  |                      |          |
 * @n  |                      |          |
 * @n  +----------------------+  <<------+
 * @n
 */

/**
 * @brief  BML volume dump header
 */
typedef struct
{
    UINT8           nHeaderSig[FSR_DUMP_HDR_SIG_SIZE];  /**<  signature     */
    UINT16          nBlksForSLCArea[FSR_MAX_DUMP_DEVS][FSR_MAX_DUMP_DIES]; /**< # of blks for SLC area */

    UINT32          nHeaderVerCode; /**< BML dump header version            */
    UINT32          nFSRVerCode;    /**< FSR version                        */
    UINT32          nNumOfDevs;     /**< the number of devices              */
    UINT32          nDiesPerDev;    /**< the number of dies per device      */
    FSRDumpVolParm  stVolParm;      /**< FSR volume param                   */

    FSRDumpDevCxt   stDevCxt;       /**< Device Context for dump            */
    UINT32          nDevCxtSize;    /**< Device Context size                */
    FSRDieDumpHdr   stDieDumpHdr[FSR_MAX_DUMP_DEVS][FSR_MAX_DUMP_DIES];/**< BML die dump header*/
} FSRVolDumpHdr;

/*****************************************************************************/
/* exported variables                                                        */
/*****************************************************************************/
extern volatile UINT32 gnFSRDbgZoneMask;

/*****************************************************************************/
/* exported function prototype                                               */
/*****************************************************************************/
UINT32  FSR_DBG_GetDbgZoneMask      (VOID);
VOID    FSR_DBG_SetDbgZoneMask      (UINT32  nMask);
VOID    FSR_DBG_UnsetDbgZoneMask    (UINT32  nMask);
VOID    FSR_DBG_ResetDbgZoneMask    (VOID);
VOID    FSR_DBG_SetAllDbgZoneMask   (VOID);
VOID    FSR_DBG_UnsetAllDbgZoneMask (VOID);

#if defined(FSR_DBG_STACKUSAGE)
VOID    FSR_DBG_InitStackDepth   (VOID);
VOID    FSR_DBG_RecordStackStart (VOID   *pnStartAddress, UINT8  *pFuncName);
VOID    FSR_DBG_RecordStackEnd   (VOID   *pnEndAddress,   UINT8  *pFuncName);
VOID    FSR_DBG_GetStackUsage    (UINT8  *p1stFuncName,   UINT8  *pLastFuncName, UINT32 *pnStackDepth);
#endif /* FSR_DBG_STACKUSAGE */





#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* #ifdef _FSR_DBG_H_ */
