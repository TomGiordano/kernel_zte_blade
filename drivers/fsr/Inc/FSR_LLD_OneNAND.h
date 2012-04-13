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
 * @file      FSR_LLD_OneNAND.h
 * @brief     This is Low level driver of OneNAND
 * @author    JeongWook Moon
 * @date      19-MAR-2007
 * @remark
 * REVISION HISTORY
 * @n  05-JAN-2007 [SongHo Yoon]    : first writing
 * @n  19-MAR-2007 [JeongWook Moon] : modify to OneNAND specification
 *                                    (from FSR_LLD_FlexOND.h)
 *
 */

#ifndef _FSR_ONENAND_LLD_H_
#define _FSR_ONENAND_LLD_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** external memory address MAP                                              */
typedef struct
{
    volatile UINT8    nBootM0[512];
    volatile UINT8    nBootM1[512];
    volatile UINT8    nDataMB00[512];
    volatile UINT8    nDataMB01[512];
    volatile UINT8    nDataMB02[512];
    volatile UINT8    nDataMB03[512];
    volatile UINT8    nDataMB10[512];
    volatile UINT8    nDataMB11[512];
    volatile UINT8    nDataMB12[512];
    volatile UINT8    nDataMB13[512];

    volatile UINT8    nRsv1[59 * 1024];

    volatile UINT8    nBootS0[16];
    volatile UINT8    nBootS1[16];
    volatile UINT8    nDataSB00[16];
    volatile UINT8    nDataSB01[16];
    volatile UINT8    nDataSB02[16];
    volatile UINT8    nDataSB03[16];
    volatile UINT8    nDataSB10[16];
    volatile UINT8    nDataSB11[16];
    volatile UINT8    nDataSB12[16];
    volatile UINT8    nDataSB13[16];

    volatile UINT8    nRsv2[8032];
    volatile UINT8    nRsv3[24 * 1024];
    volatile UINT8    nRsv4[8 * 1024];
    volatile UINT8    nRsv5[16 * 1024];

    volatile UINT16   nMID;             /* offset : 0x1E000 */
    volatile UINT16   nDID;             /* offset : 0x1E002 */
    volatile UINT16   nVerID;           /* offset : 0x1E004 */
    volatile UINT16   nDataBufSize;     /* offset : 0x1E006 */
    volatile UINT16   nBootBufSize;     /* offset : 0x1E008 */
    volatile UINT16   nBufAmount;       /* offset : 0x1E00a */
    volatile UINT16   nTech;            /* offset : 0x1E00c */

    volatile UINT8    nRsv6[0x1F2];

    volatile UINT16   nStartAddr1;      /* offset : 0x1E200 */
    volatile UINT16   nStartAddr2;      /* offset : 0x1E202 */
    volatile UINT16   nStartAddr3;      /* offset : 0x1E204 */
    volatile UINT16   nStartAddr4;      /* offset : 0x1E206 */
    volatile UINT16   nStartAddr5;      /* offset : 0x1E208 */
    volatile UINT16   nStartAddr6;      /* offset : 0x1E20a */
    volatile UINT16   nStartAddr7;      /* offset : 0x1E20c */
    volatile UINT16   nStartAddr8;      /* offset : 0x1E20e */

    volatile UINT8    nRsv7[0x1F0];

    volatile UINT16   nStartBuf;        /* offset : 0x1E400 */

    volatile UINT8    nRsv8[0xE];
    volatile UINT8    nRsv9[0x30];

    volatile UINT16   nCmd;             /* offset : 0x1E440 */
    volatile UINT16   nSysConf1;        /* offset : 0x1E442 */

    volatile UINT8    nRsv10[0x1C];
    volatile UINT8    nRsv11[0x20];

    volatile UINT16   nCtrlStat;        /* offset : 0x1E480 */
    volatile UINT16   nInt;             /* offset : 0x1E482 */

    volatile UINT8    nRsv12[0x14];

    volatile UINT16   nStartBlkAddr;    /* offset : 0x1E498 */
    volatile UINT16   nRsv13;           /* offset : 0x1E49A */
    volatile UINT16   nWrProtectStat;   /* offset : 0x1E49C */

    volatile UINT8    nRsv14[0x1962];

    volatile UINT16   nEccStat;         /* offset : 0x1FE00 */
    volatile UINT16   nEccResult[8];    /* offset : 0x1FE02 ~ 0x1FE10 */

    volatile UINT8    nRsv15[0x1EE];
} OneNANDReg;

#define FSR_OND_MAX_LOG                         (32)

/**
 * @brief   data structure for logging LLD operations.
 */
typedef struct
{
    volatile UINT32   nLogHead;
    volatile UINT16   nLogOp      [FSR_OND_MAX_LOG];
    volatile UINT16   nLogPbn     [FSR_OND_MAX_LOG];
    volatile UINT16   nLogPgOffset[FSR_OND_MAX_LOG];
    volatile UINT32   nLogFlag    [FSR_OND_MAX_LOG];
} OneNANDOpLog;

extern volatile OneNANDOpLog        gstONDOpLog[FSR_MAX_DEVS];

/*****************************************************************************/
/* OneNAND System Configureation1 Register Values                            */
/*****************************************************************************/
#define     FSR_OND_CONF1_SYNC_READ             (UINT16) (0x8000)
#define     FSR_OND_CONF1_ASYNC_READ            (UINT16) (0x0000)

#define     FSR_OND_CONF1_BST_RD_LATENCY_3      (UINT16) (0x3000)     /*   min   */
#define     FSR_OND_CONF1_BST_RD_LATENCY_4      (UINT16) (0x4000)     /* default */
#define     FSR_OND_CONF1_BST_RD_LATENCY_5      (UINT16) (0x5000)
#define     FSR_OND_CONF1_BST_RD_LATENCY_6      (UINT16) (0x6000)
#define     FSR_OND_CONF1_BST_RD_LATENCY_7      (UINT16) (0x7000)

#define     FSR_OND_CONF1_BST_LENGTH_CONT       (UINT16) (0x0000)     /* default */
#define     FSR_OND_CONF1_BST_LENGTH_4WD        (UINT16) (0x0200)
#define     FSR_OND_CONF1_BST_LENGTH_8WD        (UINT16) (0x0400)
#define     FSR_OND_CONF1_BST_LENGTH_16WD       (UINT16) (0x0600)
#define     FSR_OND_CONF1_BST_LENGTH_32WD       (UINT16) (0x0800)     /* N/A on spare */
#define     FSR_OND_CONF1_BST_LENGTH_1KWD       (UINT16) (0x0A00)     /* N/A on spare, sync. burst block read only */

#define     FSR_OND_CONF1_ECC_ON                (UINT16) (0xFEFF)
#define     FSR_OND_CONF1_ECC_OFF               (UINT16) (0x0100)     /* (~CONF1_ECC_ON) */

#define     FSR_OND_CONF1_RDY_POLAR             (UINT16) (0x0080)
#define     FSR_OND_CONF1_INT_POLAR             (UINT16) (0x0040)
#define     FSR_OND_CONF1_IOBE_ENABLE           (UINT16) (0x0020)

#define     FSR_OND_CONF1_BWPS_UNLOCKED         (UINT16) (0x0001)

#define     FSR_OND_CONF1_HF_ON                 (UINT16) (0x0004)
#define     FSR_OND_CONF1_HF_OFF                (UINT16) (0xFFFB)
#define     FSR_OND_CONF1_RDY_CONF              (UINT16) (0x0010)

/* MACROs below are used for Deferred Check Operation                         */
#define     FSR_OND_PREOP_NONE                  (0x0000)
#define     FSR_OND_PREOP_READ                  (0x0001)
#define     FSR_OND_PREOP_1X_WRITE              (0x0002)
#define     FSR_OND_PREOP_2X_WRITE              (0x0003)
#define     FSR_OND_PREOP_ERASE                 (0x0004)
#define     FSR_OND_PREOP_IOCTL                 (0x0005)
#define     FSR_OND_PREOP_CACHE_PGM             (0x0006)
#define     FSR_OND_PREOP_ADDRESS_NONE          (0xFFFF)
#define     FSR_OND_PREOP_FLAG_NONE             FSR_LLD_FLAG_NONE

/*****************************************************************************/
/* exported common APIs                                                      */
/*****************************************************************************/
INT32   FSR_OND_Init                (UINT32             nFlag);
INT32   FSR_OND_Open                (UINT32             nDev,
                                     VOID              *pParam,
                                     UINT32             nFlag);
INT32   FSR_OND_Close               (UINT32             nDev,
                                     UINT32             nFlag);
INT32   FSR_OND_Erase               (UINT32             nDev,
                                     UINT32            *pnPbn,
                                     UINT32             nNumOfBlks,
                                     UINT32             nFlag);
INT32   FSR_OND_ChkBadBlk           (UINT32             nDev,
                                     UINT32             nPbn,
                                     UINT32             nFlag);
INT32   FSR_OND_FlushOp             (UINT32             nDev,
                                     UINT32             nDieIdx,
                                     UINT32             nFlag);
INT32   FSR_OND_GetDevSpec          (UINT32             nDev,
                                     FSRDevSpec        *pstDevSpec,
                                     UINT32             nFlag);
INT32   FSR_OND_Read                (UINT32             nDev, 
                                     UINT32             nPbn,
                                     UINT32             nPgOffset,
                                     UINT8             *pMBuf,
                                     FSRSpareBuf       *pSBuf,
                                     UINT32             nFlag);
INT32   FSR_OND_ReadOptimal         (UINT32             nDev, 
                                     UINT32             nPbn,
                                     UINT32             nPgOffset,
                                     UINT8             *pMBuf,
                                     FSRSpareBuf       *pSBuf,
                                     UINT32             nFlag);
INT32   FSR_OND_Write               (UINT32             nDev, 
                                     UINT32             nPbn,
                                     UINT32             nPgOffset,
                                     UINT8             *pMBuf,
                                     FSRSpareBuf       *pSBuf,
                                     UINT32             nFlag);
INT32   FSR_OND_CopyBack            (UINT32             nDev,
                                     LLDCpBkArg        *pstCpArg,
                                     UINT32             nFlag);
INT32   FSR_OND_GetPrevOpData       (UINT32             nDev,
                                     UINT8             *pMBuf,
                                     FSRSpareBuf       *pSBuf,
                                     UINT32             nDieIdx,
                                     UINT32             nFlag);
INT32   FSR_OND_IOCtl               (UINT32             nDev,
                                     UINT32             nCode,
                                     UINT8             *pBufI,
                                     UINT32             nLenI,
                                     UINT8             *pBufO,
                                     UINT32             nLenO,
                                     UINT32            *pByteRet);
INT32   FSR_OND_InitLLDStat         (VOID);
INT32   FSR_OND_GetStat             (FSRLLDStat        *pstStat);
INT32   FSR_OND_GetBlockInfo        (UINT32             nDev,
                                     UINT32             nPbn,
                                     UINT32            *pnType,
                                     UINT32            *pnPgsPerBlk);
INT32   FSR_OND_GetNANDCtrllerInfo  (UINT32             nDev,
                                     LLDPlatformInfo   *pLLDPltInfo);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _FSR_ONENAND_LLD_H_ */
