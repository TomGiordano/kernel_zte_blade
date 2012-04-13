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
 * @file      FSR_LLD_4K_OneNAND.h
 * @brief     declarations of exported functions and register file
 * @author    NamOh Hwang
 * @date      05-JAN-2007
 * @remark
 * REVISION HISTORY
 * @n  05-JAN-2007 [NamOh Hwang] : first writing
 *
 */


#ifndef _FSR_4K_ONENAND_LLD_H_
#define _FSR_4K_ONENAND_LLD_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief external memory address MAP
 */
typedef struct
{
    volatile UINT8    nBootM0[512];     /* offset : 0x00000 */
    volatile UINT8    nBootM1[512];     /* offset : 0x00200 */
    volatile UINT8    nDataMB00[512];   /* offset : 0x00400 */
    volatile UINT8    nDataMB01[512];   /* offset : 0x00600 */
    volatile UINT8    nDataMB02[512];   /* offset : 0x00800 */
    volatile UINT8    nDataMB03[512];   /* offset : 0x00A00 */
    volatile UINT8    nDataMB10[512];   /* offset : 0x00C00 */
    volatile UINT8    nDataMB11[512];   /* offset : 0x00E00 */
    volatile UINT8    nDataMB12[512];   /* offset : 0x01000 */
    volatile UINT8    nDataMB13[512];   /* offset : 0x01200 */

    volatile UINT8    nRsv1[59 * 1024]; /* offset : 0x01400 ~ 0x0FFFE */

    volatile UINT8    nBootS0[16];      /* offset : 0x10000 ~ 0x1000E */
    volatile UINT8    nBootS1[16];      /* offset : 0x10010 ~ 0x1001E */
    volatile UINT8    nDataSB00[16];    /* offset : 0x10020 ~ 0x1002E */
    volatile UINT8    nDataSB01[16];    /* offset : 0x10030 ~ 0x1003E */
    volatile UINT8    nDataSB02[16];    /* offset : 0x10040 ~ 0x1004E */
    volatile UINT8    nDataSB03[16];    /* offset : 0x10050 ~ 0x1005E */
    volatile UINT8    nDataSB10[16];    /* offset : 0x10060 ~ 0x1006E */
    volatile UINT8    nDataSB11[16];    /* offset : 0x10070 ~ 0x1007E */
    volatile UINT8    nDataSB12[16];    /* offset : 0x10080 ~ 0x1008E */
    volatile UINT8    nDataSB13[16];    /* offset : 0x10090 ~ 0x1009E */

    volatile UINT8    nRsv2[8032];      /* offset : 0x100a0 ~ 0x11FFE */
    volatile UINT8    nRsv3[24 * 1024]; /* offset : 0x12000 ~ 0x17FFE */
    volatile UINT8    nRsv4[8 * 1024];  /* offset : 0x18000 ~ 0x19FFE */
    volatile UINT8    nRsv5[16 * 1024]; /* offset : 0x1A000 ~ 0x1DFFE */

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

    volatile UINT16   nEccStat[4];      /* offset : 0x1FE00 ~ 0x1FE06 */

    volatile UINT8    nRsv15[0x1F6];    /* offset : 0x1FE08 ~ 0x1FFFC */
    volatile UINT16   nDebugPort;       /* offset : 0x1FFFE */
} OneNAND4kReg;

#define FSR_4K_OND_MAX_LOG                         (32)
/**
 * @brief   data structure for logging LLD operations.
 */
typedef struct
{
    volatile UINT32   nLogHead;
    volatile UINT16   nLogOp      [FSR_4K_OND_MAX_LOG];
    volatile UINT16   nLogPbn     [FSR_4K_OND_MAX_LOG];
    volatile UINT16   nLogPgOffset[FSR_4K_OND_MAX_LOG];
    volatile UINT32   nLogFlag    [FSR_4K_OND_MAX_LOG];
} OneNAND4kOpLog;

/*****************************************************************************/
/* Extern variable declarations                                              */
/*****************************************************************************/
extern volatile OneNAND4kOpLog        gstOND4kOpLog[FSR_MAX_DEVS];

/*****************************************************************************/
/* 4K OneNAND System Configureation1 Register Values                       */
/*****************************************************************************/
#define     FSR_OND_4K_CONF1_SYNC_READ             (UINT16) (0x8000)
#define     FSR_OND_4K_CONF1_ASYNC_READ            (UINT16) (0x0000)

#define     FSR_OND_4K_CONF1_BST_RD_LATENCY_3      (UINT16) (0x3000)     /*   min   */
#define     FSR_OND_4K_CONF1_BST_RD_LATENCY_4      (UINT16) (0x4000)     /* default */
#define     FSR_OND_4K_CONF1_BST_RD_LATENCY_5      (UINT16) (0x5000)
#define     FSR_OND_4K_CONF1_BST_RD_LATENCY_6      (UINT16) (0x6000)
#define     FSR_OND_4K_CONF1_BST_RD_LATENCY_7      (UINT16) (0x7000)

#define     FSR_OND_4K_CONF1_BST_LENGTH_CONT       (UINT16) (0x0000)     /* default */
#define     FSR_OND_4K_CONF1_BST_LENGTH_4WD        (UINT16) (0x0200)
#define     FSR_OND_4K_CONF1_BST_LENGTH_8WD        (UINT16) (0x0400)
#define     FSR_OND_4K_CONF1_BST_LENGTH_16WD       (UINT16) (0x0600)
#define     FSR_OND_4K_CONF1_BST_LENGTH_32WD       (UINT16) (0x0800)     /* N/A on spare */
#define     FSR_OND_4K_CONF1_BST_LENGTH_1KWD       (UINT16) (0x0A00)     /* N/A on spare, sync. burst block read only */

#define     FSR_OND_4K_CONF1_ECC_ON                (UINT16) (0xFEFF)
#define     FSR_OND_4K_CONF1_ECC_OFF               (UINT16) (0x0100)     /* (~FSR_OND_4K_CONF1_ECC_ON) */

#define     FSR_OND_4K_CONF1_RDY_POLAR             (UINT16) (0x0080)
#define     FSR_OND_4K_CONF1_INT_POLAR             (UINT16) (0x0040)
#define     FSR_OND_4K_CONF1_IOBE_ENABLE           (UINT16) (0x0020)

#define     FSR_OND_4K_CONF1_BWPS_UNLOCKED         (UINT16) (0x0001)

#define     FSR_OND_4K_CONF1_HF_ON                 (UINT16) (0x0004)
#define     FSR_OND_4K_CONF1_HF_OFF                (UINT16) (0xFFFB)
#define     FSR_OND_4K_CONF1_RDY_CONF              (UINT16) (0x0010)


/* MACROs below are used for Deferred Check Operation                         */
#define     FSR_OND_4K_PREOP_NONE                  (0x0000)
#define     FSR_OND_4K_PREOP_READ                  (0x0001)
#define     FSR_OND_4K_PREOP_CPBK_LOAD             (0x0002)
#define     FSR_OND_4K_PREOP_CACHE_PGM             (0x0003)
#define     FSR_OND_4K_PREOP_PROGRAM               (0x0004)
#define     FSR_OND_4K_PREOP_CPBK_PGM              (0x0005)
#define     FSR_OND_4K_PREOP_ERASE                 (0x0006)
#define     FSR_OND_4K_PREOP_IOCTL                 (0x0007)
#define     FSR_OND_4K_PREOP_HOTRESET              (0x0008)
#define     FSR_OND_4K_PREOP_ADDRESS_NONE          (0xFFFF)
#define     FSR_OND_4K_PREOP_FLAG_NONE             FSR_LLD_FLAG_NONE
/*****************************************************************************/
/* exported common APIs                                                      */
/*****************************************************************************/
INT32   FSR_OND_4K_Init            (UINT32         nFlag);
INT32   FSR_OND_4K_Open            (UINT32         nDev,
                                 VOID          *pParam,
                                 UINT32         nFlag);
INT32   FSR_OND_4K_Close           (UINT32         nDev,
                                 UINT32         nFlag);
INT32   FSR_OND_4K_Read            (UINT32         nDev,
                                 UINT32         nPbn,
                                 UINT32         nPgOffset,
                                 UINT8         *pMBuf,
                                 FSRSpareBuf   *pSBuf,
                                 UINT32         nFlag);
INT32   FSR_OND_4K_ReadOptimal     (UINT32         nDev,
                                 UINT32         nPbn,
                                 UINT32         nPgOffset,
                                 UINT8         *pMBuf,
                                 FSRSpareBuf   *pSBuf,
                                 UINT32         nFlag);
INT32   FSR_OND_4K_Write           (UINT32         nDev, 
                                 UINT32         nPbn,
                                 UINT32         nPgOffset,
                                 UINT8         *pMBuf,
                                 FSRSpareBuf   *pSBuf,
                                 UINT32         nFlag);
INT32   FSR_OND_4K_Erase           (UINT32         nDev,
                                 UINT32        *pnPbn,
                                 UINT32         nNumOfBlks,
                                 UINT32         nFlag);
INT32   FSR_OND_4K_CopyBack        (UINT32         nDev,
                                 LLDCpBkArg    *pstCpArg,
                                 UINT32         nFlag);
INT32   FSR_OND_4K_ChkBadBlk       (UINT32         nDev,
                                 UINT32         nPbn,
                                 UINT32         nFlag);
INT32   FSR_OND_4K_FlushOp         (UINT32         nDev,
                                 UINT32         nDieIdx,
                                 UINT32         nFlag);
INT32   FSR_OND_4K_GetBlockInfo    (UINT32         nDev,
                                 UINT32         nPbn,
                                 UINT32        *pBlockType,
                                 UINT32        *pPgsPerBlk);
INT32   FSR_OND_4K_GetDevSpec      (UINT32         nDev,
                                 FSRDevSpec    *pstDevSpec,
                                 UINT32         nFlag);
INT32   FSR_OND_4K_GetPlatformInfo (UINT32         nDev,
                               LLDPlatformInfo *pLLDPltInfo);
INT32   FSR_OND_4K_GetPrevOpData   (UINT32         nDev,
                                 UINT8         *pMBuf,
                                 FSRSpareBuf   *pSBuf,
                                 UINT32         nDieIdx,
                                 UINT32         nFlag);
INT32   FSR_OND_4K_IOCtl           (UINT32         nDev,
                                 UINT32         nCode,
                                 UINT8         *pBufI,
                                 UINT32         nLenI,
                                 UINT8         *pBufO,
                                 UINT32         nLenO,
                                 UINT32        *pByteRet);
INT32   FSR_OND_4K_InitLLDStat     (VOID);
INT32   FSR_OND_4K_GetStat         (FSRLLDStat    *pstStat);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _FSR_FONENAND_LLD_H_ */
