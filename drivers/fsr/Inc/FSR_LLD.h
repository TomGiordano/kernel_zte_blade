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
 * @file      FSR_LLD.h
 * @brief     Flash Low level Device Driver header file
 * @author    NamOh Hwang
 * @date      15-JAN-2007
 * @remark
 * REVISION HISTORY
 * @n  10-JAN-2007 [SongHo Yoon] : first writing
 * @n  15-JAN-2007 [NamOh Hwang] : update flag/return value
 *
 */

#ifndef _FSR_LLD_H_
#define _FSR_LLD_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/*****************************************************************************/
/* Data Structures                                                           */
/*****************************************************************************/

/*****************************************************************************
  - Spare Buffer Physical Layout
  2KB page : SpareBufferBase(16B) + SpareBufferExt(16B) / 2
  4KB page : SpareBufferBase(16B) + SpareBufferExt(16B) + SpareBufferExt(16B)
*****************************************************************************/


#define     FSR_SPARE_BUF_BML_BASE_SIZE         (4)
#define     FSR_SPARE_BUF_STL_BASE_SIZE         (12)
#define     FSR_SPARE_BUF_BASE_SIZE             (FSR_SPARE_BUF_BML_BASE_SIZE + FSR_SPARE_BUF_STL_BASE_SIZE)
#define     FSR_SPARE_BUF_EXT_SIZE              (16)
#define     FSR_MAX_SPARE_BUF_EXT               (2)
#define     FSR_SPARE_BUF_SIZE_2KB_PAGE         (FSR_SPARE_BUF_BASE_SIZE + FSR_SPARE_BUF_EXT_SIZE * 1)
#define     FSR_SPARE_BUF_SIZE_4KB_PAGE         (FSR_SPARE_BUF_BASE_SIZE + FSR_SPARE_BUF_EXT_SIZE * 2)
#define     FSR_PAGE_SIZE_PER_SPARE_BUF_EXT     (2048)
#define     FSR_LLD_SPARE_BUF_SIZE_2KB_PAGE     (FSR_SPARE_BUF_BASE_SIZE + (FSR_SPARE_BUF_EXT_SIZE >> 1))
#define     FSR_LLD_UID_SIZE                    (16)

/**
 * @brief   data structure for spare area base
 */
typedef struct
{
    UINT16          nBadMark;       /**< 2B / Bad Mark        : Read Only   */
    UINT16          nBMLMetaBase0;  /**< 2B / BML meta base 0 : Read Only   */

    UINT32          nSTLMetaBase0;  /**< 4B / STL meta base 0 : R/W         */
    UINT32          nSTLMetaBase1;  /**< 4B / STL meta base 1 : R/W         */
    UINT32          nSTLMetaBase2;  /**< 4B / STL meta base 2 : R/W         */
} FSRSpareBufBase;

/**
 * @brief   data structure for spare area extension
 */
typedef struct
{
    UINT32  nSTLMetaExt0;       /**<  4B  / STL meta extension 0            */
    UINT32  nSTLMetaExt1;       /**<  4B  / STL meta extension 1            */
    UINT32  nSTLMetaExt2;       /**<  4B  / STL meta extension 2            */
    UINT32  nSTLMetaExt3;       /**<  4B  / STL meta extension 3            */
} FSRSpareBufExt;

/**
 * @brief   data structure for spare area
            spare allocation size : 32 Bytes (16B + 16B) for 2KB page
            spare allocation size : 48 Bytes (16B + 32B) for 4KB page
 */
typedef struct
{
    FSRSpareBufBase *pstSpareBufBase;/** base element of Spare buffer       */

    UINT32           nNumOfMetaExt;  /**<    / the number of extened data   */
    FSRSpareBufExt  *pstSTLMetaExt;  /**< 16B * page size / 512 / STL meta ext
                                         pstSTLMetaExt[0] for 2KB / 4KB page
                                         pstSTLMetaExt[1] for 4KB page only */
} FSRSpareBuf;


/**
 * @brief data structure which can be obtained by FSR_LLD_GetDevSpec function call
 */
typedef struct
{
            UINT16      nNumOfBlks;     /**< # of blocks in NAND device      */
            UINT16      nNumOfPlanes;   /**< # of planes in die              */
            UINT16      nBlksForSLCArea[FSR_MAX_DIES];/**< # of blks for SLC
                                                             area            */


            UINT16      nSparePerSct;   /**< # of bytes of spare of a sector */
            UINT16      nSctsPerPG;     /**< # of sectors per page           */
            UINT16      nNumOfBlksIn1stDie; /**< # of blocks in 1st Die      */
            UINT16      nDID;           /**< Device ID                       */

            UINT32      nPgsPerBlkForSLC;/**< # of pages per block in SLC area*/
            UINT32      nPgsPerBlkForMLC;/**< # of pages per block in MLC area*/

            UINT16      nNumOfDies;     /**< # of dies in NAND device        */
            UINT16      nUserOTPScts;   /**< # of user sectors               */

            BOOL32      b1stBlkOTP;     /**< support 1st block OTP or not    */

            UINT16      nRsvBlksInDev;  /**< # of total bad blocks (init + run)*/
            UINT16      nPad0; 

    const   UINT8      *pPairedPgMap;   /**< paired page mapping information */
    const   UINT8      *pLSBPgMap;      /**< array of LSB pages              */

            UINT16      nNANDType;      /**< NAND types
                                           - FSR_LLD_FLEX_ONENAND
                                           - FSR_LLD_SLC_NAND
                                           - FSR_LLD_MLC_NAND
                                           - FSR_LLD_SLC_ONENAND             */
            UINT16      nPgBufToDataRAMTime;/**< time for moving data of 1 page
                                                 from Page Buffer to DataRAM */

            BOOL32      bCachePgm;      /**< supports cache program          */

            /* TrTime, TwTime of MLC are array of size 2
             * first  element is for LSB TLoadTime, TProgTime
             * second element is for MLB TLoadTime, TProgTime
             * use macro FSR_LLD_IDX_LSB_TIME, FSR_LLD_IDX_MSB_TIME
             */
            UINT32      nSLCTLoadTime; /**< Typical Load     operation time  */
            UINT32      nMLCTLoadTime; /**< Typical Load     operation time  */
            UINT32      nSLCTProgTime; /**< Typical Program  operation time  */
            UINT32      nMLCTProgTime[2];/**< Typical Program  operation time*/
            UINT32      nTEraseTime;   /**< Typical Erase    operation time  */
            UINT32      nWrTranferTime;/**< write transfer time              */
            UINT32      nRdTranferTime;/**< read transfer time               */

            /* endurance information                                         */
            UINT32      nSLCPECycle;   /**< program, erase cycle of SLC block*/
            UINT32      nMLCPECycle;   /**< program, erase cycle of MLC block*/

            UINT8       nUID[FSR_LLD_UID_SIZE]; /**< Unique ID info. about OTP block  */

} FSRDevSpec;

/**
 * @brief  data structure for copy-back random-in arguement
 */
typedef struct
{
    UINT16    nOffset;          /**< nOffset should be even number
                                     main area range  :          0 ~ 8190
                                     spare area range : 0x4000   0 ~ 0       */
    UINT16    nNumOfBytes;      /**< Random In Bytes                         */
    VOID     *pBuf;             /**< Data Buffer Pointer                     */
} LLDRndInArg;

/**
 * @brief  data structure for copy-back arguement
 */
typedef struct
{
    UINT16       nSrcPbn;       /**< Copy Back Source Vun                    */
    UINT16       nSrcPgOffset;
    UINT16       nDstPbn;       /**< Copy Back Dest.  Vun                    */
    UINT16       nDstPgOffset;
    UINT32       nRndInCnt;     /**< Random In Count                         */
    LLDRndInArg *pstRndInArg;   /**< RndInArg Array pointer                  */
} LLDCpBkArg;

/**
 * @brief  LLD_IOCtl parameter for FSR_LLD_IOCTL_PI_WRITE/ FSR_LLD_IOCTL_PI_READ Code
 * @remark in case of Flex-OneNAND, read/write partition information with LLD_IOCtl
 *         LLD_IOCtl takes nCode as FSR_LLD_IOCTL_PI_WRITE/ FSR_LLD_IOCTL_PI_READ
 *         pBufI as pointer to LLDPIArg
*/
typedef struct
{   
    UINT16 nEndOfSLC;    /**< the end block number of SLC Area                 */
    UINT16 nPad;         /**< padding for 32 bit alignment                     */
    UINT32 nPILockValue; /**< {FSR_LLD_IOCTL_LOCK_PI, FSR_LLD_IOCTL_UNLOCK_PI} */
} LLDPIArg;

/**
 * @brief  LLD_IOCtl parameter for FSR_LLD_IOCTL_LOCK_TIGHT/
 * @n      FSR_LLD_IOCTL_LOCK_BLOCK/FSR_LLD_IOCTL_UNLOCK_BLOCK
 * @remark in case of   FSR_LLD_IOCTL_LOCK_TIGHT, 
 *                      FSR_LLD_IOCTL_LOCK_BLOCK, 
 *                      FSR_LLD_IOCTL_UNLOCK_BLOCK, 
 *                      use LLDProtectionArg
*/
typedef struct
{
    UINT32 nStartBlk;   /**< start block number                              */
    UINT32 nBlks;       /**< the number of blocks                            */
} LLDProtectionArg;

/*
   in case of   FSR_LLD_IOCTL_UNLOCK_ALLBLK,
                FSR_LLD_IOCTL_GET_LOCK_STAT,
                FSR_LLD_IOCTL_CORE_RESET,
                FSR_LLD_IOCTL_HOT_RESET,
                FSR_LLD_IOCTL_PI_READ,

                use UINT32 variable for pBufI
*/

/**
 * @brief  LLD_GetNANDCtrllerInfo parameter
 */
typedef struct
{
    UINT32  nType;
    UINT32  nAddrOfCmdReg;      /**< Address of command register             */
    UINT32  nAddrOfAdrReg;      /**< Address of address register             */
    UINT32  nAddrOfReadIDReg;   /**< Address of register for reading ID      */
    UINT32  nAddrOfStatusReg;   /**< Address of status register              */
    UINT32  nCmdOfReadID;       /**< Command of reading Device ID            */
    UINT32  nCmdOfReadPage;     /**< Command of read page                    */
    UINT32  nCmdOfReadStatus;   /**< Command of read status                  */
    UINT32  nMaskOfRnB;         /**< Mask value for Ready or Busy status     */
} LLDPlatformInfo;

/**
 * @brief  LLD_GetStat parameter
 * @remark this data structure is used to get the statistics for the whole device
 */
typedef struct
{
    UINT32  nSLCLoads;  /**< the number of times of SLC Load                 */
    UINT32  nMLCLoads;  /**< the number of times of MSB Load                 */

    UINT32  nSLCPgms;   /**< the number of times of SLC programs             */
    UINT32  nLSBPgms;   /**< the number of times of LSB programs             */
    UINT32  nMSBPgms;   /**< the number of times of MSB programs             */

    UINT32  nCacheBusy; /**< the number of times of cache busy               */
    UINT32  nErases;    /**< the number of times of Erase                    */

    UINT32  nRdTrans;   /**< the number of times of read transfer            */
    UINT32  nWrTrans;   /**< the number of times of write transfer           */

    UINT32  nRdTransInBytes; /**< the number of bytes transfered             */
    UINT32  nWrTransInBytes; /**< the number of bytes transfered             */
} FSRLLDStat;

/*****************************************************************************/
/* Flags                                                                     */
/*****************************************************************************/
/*
 * every exported LLD API has nFlag.
 * nFlag is 32 bit wide, and has its meaning like this.
 *
 *  << nFlag BIT ALLOCATION >>
 *  +-----------------------+-----------------------+--------+--+--+-----+-----+--+--+--+--+--+--------+
 *  |31 30 29 28 27 26 25 24|23 22 21 20 19 18 17 16|15 14 13|12 11| 10  |09 08|07|06|05|04|03|02 01 00|
 *  +-----------------------+-----------------------+--+--+--+--+--+-----+-----+--+--+--+--+--+--------+
 *  |  start sector offset  |   last sector offset  |NB|NB|RM|RD|Ba|bUse | BAD |Tr| E| I| B| D| COMMAND|
 *  |                       |                       |  |  |  |O |ck|Spare| MARK|an| C| N| B| M|  INDEX |
 *  |                       |                       |ST|CT|ST|  |Up|Buf  |     |s | C| T| M| P|        |
 *  +-----------------------+-----------------------+--+--+--+--+--+-----+-----+--+--+--+--+--+--------+
 *
 *   31~24: (in LLD_Read) start sector offset (0 ~ 7)
 *   23~16: (in LLD_Read) last sector offset from the end of page (0 ~ 7)
 *      15: (in FSR_BML_NonBlkMgr.c) Start non-blocking operation in same operation type
 *      14: (in FSR_BML_NonBlkMgr.c) Continue start non-blocking operation in same operation type
 *      13: refer to a comment on  FSR_LLD_FLAG_REMAIN_PREOP_STAT
 *      12: tiny FSR use only
 *      11: (in LLD_Write) Back up data for Bad block handling
 *      10: (in LLD_Write) write spare buffer onto DataRAM
 *   09~08: (in LLD_Write) BAD MARK
 *      07: (in LLD_Read) Transfer ON/OFF
 *      06: ECC ON/OFF
 *      05: Interrupt ON/OFF
 *      04: (in LLD_Write) this bit indicates the requested block is a BBM meta block
 *      03: Dump ON/OFF
 *   02~00: Command Index
 */

/* nFlag of FSR_XXX_FlushOp() : if this flag is set, 
   DO NOT clear previos operation to NONE. 
   And At the next FSR_XXX_FlushOp call, FSR_XXX_FlushOp() should return the previous error.*/
#define     FSR_LLD_FLAG_REMAIN_PREOP_STAT  (0x00002000)

/* to support tiny FSR                                                       */
#define     FSR_LLD_FLAG_READ_ONLY          (0x00001000)

/* select DataRAM by write operation when the input buffer is null           */
#define     FSR_LLD_FLAG_BACKUP_DATA        (0x00000800)

/* if pSBuf is not NULL & this bit is set, write spare buffer onto DataRAM
 * if pSBuf is not NULL & this bit is unset, fill DataRAM with 0xFF
 */
#define     FSR_LLD_FLAG_USE_SPAREBUF       (0x00000400)

/* macros for controling bad mark                                            */
#define     FSR_LLD_FLAG_BADMARK_BASEBIT    (8)
#define     FSR_LLD_FLAG_BADMARK_MASK       (0x00000300)
/* no bad mark flag                                                          */
#define     FSR_LLD_FLAG_WR_NOBADMARK       (0x00000000)
/* bad mark flag by erase error                                              */
#define     FSR_LLD_FLAG_WR_EBADMARK        (0x00000100)
/* bad mark flag by write error                                              */
#define     FSR_LLD_FLAG_WR_WBADMARK        (0x00000200)
/* bad mark flag by block protection error                                   */
#define     FSR_LLD_FLAG_WR_LBADMARK        (0x00000300)

#define     FSR_LLD_FLAG_TRANSFER           (0x00000080)

#define     FSR_LLD_FLAG_ECC_BASEBIT        (6)
#define     FSR_LLD_FLAG_ECC_MASK           (0x00000040)
#define     FSR_LLD_FLAG_ECC_ON             (0x00000000)
#define     FSR_LLD_FLAG_ECC_OFF            (0x00000040)

/* blocking, non blocking mode                                               */
#define     FSR_LLD_FLAG_INT_BASEBIT        (5)
#define     FSR_LLD_FLAG_INT_MASK           (0x00000020)
#define     FSR_LLD_FLAG_ENABLE_INT         (0x00000020)
#define     FSR_LLD_FLAG_DISABLE_INT        (0x00000000)

/* macros for controling nBlkType,nRsvType of FSRSpareBuf structure          */
#define     FSR_LLD_FLAG_BBM_META_BASEBIT   (4)
#define     FSR_LLD_FLAG_BBM_META_MASK      (0x00000010)
#define     FSR_LLD_FLAG_BBM_META_BLOCK     (0x00000010)


#define     FSR_LLD_FLAG_CMDIDX_BASEBIT     (0)
#define     FSR_LLD_FLAG_CMDIDX_MASK        (0x00000007)

/* macros for dump datas to DRAM */
#define     FSR_LLD_FLAG_DUMP_MASK          (0x00000008)
#define     FSR_LLD_FLAG_DUMP_ON            (0x00000008)
#define     FSR_LLD_FLAG_DUMP_OFF           (0x00000000)

/* bits between 02 and 00 of nFlag represents command index.                 */
#define     FSR_LLD_FLAG_NO_LOADCMD         (0x00000000)
#define     FSR_LLD_FLAG_1X_LOAD            (0x00000001)
#define     FSR_LLD_FLAG_1X_PLOAD           (0x00000002)
#define     FSR_LLD_FLAG_2X_LOAD            (0x00000003)
#define     FSR_LLD_FLAG_2X_PLOAD           (0x00000004)
#define     FSR_LLD_FLAG_LSB_RECOVERY_LOAD  (0x00000005)

#define     FSR_LLD_FLAG_1X_PROGRAM         (0x00000000)
#define     FSR_LLD_FLAG_1X_CACHEPGM        (0x00000001)
#define     FSR_LLD_FLAG_2X_PROGRAM         (0x00000002)
#define     FSR_LLD_FLAG_2X_CACHEPGM        (0x00000003)
#define     FSR_LLD_FLAG_OTP_PROGRAM        (0x00000004)

#define     FSR_LLD_FLAG_1X_ERASE           (0x00000000)
#define     FSR_LLD_FLAG_2X_ERASE           (0x00000001)

#define     FSR_LLD_FLAG_NO_CPBK_CMD        (0x00000000)
#define     FSR_LLD_FLAG_1X_CPBK_LOAD       (0x00000001)
#define     FSR_LLD_FLAG_1X_CPBK_PROGRAM    (0x00000002)
#define     FSR_LLD_FLAG_2X_CPBK_LOAD       (0x00000003)
#define     FSR_LLD_FLAG_2X_CPBK_PROGRAM    (0x00000004)

#define     FSR_LLD_FLAG_1X_CHK_BADBLOCK    (0x00000001)
#define     FSR_LLD_FLAG_2X_CHK_BADBLOCK    (0x00000002)

#define     FSR_LLD_FLAG_1X_OPERATION       (0x00000001)
#define     FSR_LLD_FLAG_2X_OPERATION       (0x00000002)

/* 
 * in case of calling APIs like LLD_Init(),
 *                              LLD_Open(),
 *                              LLD_Close(),
 *                              LLD_ChkBadBlk(),
 *                              LLD_GetDevSpec(),
 *                              LLD_ReadScts(),
 * nFlag is FSR_LLD_FLAG_NONE
 */
#define     FSR_LLD_FLAG_NONE               (0x00000000)

#define     FSR_LLD_CPBK_SPARE              (0x4000)

#define     FSR_LLD_FLAG_1ST_SCTOFFSET_MASK     (0xFF000000)
#define     FSR_LLD_FLAG_LAST_SCTOFFSET_MASK    (0x00FF0000)
#define     FSR_LLD_FLAG_1ST_SCTOFFSET_BASEBIT  (24)
#define     FSR_LLD_FLAG_LAST_SCTOFFSET_BASEBIT (16)
#define     FSR_LLD_FLAG_MAKE_SCTOFFSET(off1, off2)     \
    (((off1 << FSR_LLD_FLAG_1ST_SCTOFFSET_BASEBIT)  & FSR_LLD_FLAG_1ST_SCTOFFSET_MASK) | \
     ((off2 << FSR_LLD_FLAG_LAST_SCTOFFSET_BASEBIT) & FSR_LLD_FLAG_LAST_SCTOFFSET_MASK))

/* write protection status register                                          */
#define     FSR_LLD_BLK_STAT_MASK           (0x0007)
#define     FSR_LLD_BLK_STAT_UNLOCKED       (0x0004)
#define     FSR_LLD_BLK_STAT_LOCKED         (0x0002)
#define     FSR_LLD_BLK_STAT_LOCKED_TIGHT   (0x0001)

/* nNANDType of FSRDevSpec can have values listed below                      */
#define     FSR_LLD_FLEX_ONENAND            (0x0000)
#define     FSR_LLD_SLC_NAND                (0x0001)
#define     FSR_LLD_MLC_NAND                (0x0002)
#define     FSR_LLD_SLC_ONENAND             (0x0003)

/* index to nMLCTrTime[], nMLCTwTime[]                                       */
#define     FSR_LLD_IDX_LSB_TIME            (0x0)
#define     FSR_LLD_IDX_MSB_TIME            (0x1)


#define     FSR_LLD_BBM_META_MARK           (0xA5A5)

/*****************************************************************************/
/* LLD Return Values                                                         */
/*****************************************************************************/
#define     FSR_LLD_IOCTL_NOT_SUPPORT           FSR_RETURN_VALUE(0, 0x2, 0x0003, 0x0000)
#define     FSR_LLD_INIT_BADBLOCK               FSR_RETURN_VALUE(0, 0x2, 0x0002, 0x0000)
#define     FSR_LLD_INIT_GOODBLOCK              FSR_RETURN_VALUE(0, 0x2, 0x0001, 0x0000)

#define     FSR_LLD_SUCCESS                     FSR_RETURN_VALUE(0, 0x2, 0x0000, 0x0000)

#define     FSR_LLD_PREV_READ_ERROR             FSR_RETURN_VALUE(1, 0x2, 0x0001, 0x0000)
#define     FSR_LLD_ALREADY_INITIALIZED         FSR_RETURN_VALUE(1, 0x2, 0x0003, 0x0000)

#define     FSR_LLD_PREV_WRITE_ERROR            FSR_RETURN_VALUE(1, 0x2, 0x0004, 0x0000)
#define     FSR_LLD_PREV_ERASE_ERROR            FSR_RETURN_VALUE(1, 0x2, 0x0005, 0x0000)
#define     FSR_LLD_PREV_WR_PROTECT_ERROR       FSR_RETURN_VALUE(1, 0x2, 0x0006, 0x0000)

#define     FSR_LLD_INVALID_PARAM               FSR_RETURN_VALUE(1, 0x2, 0x0007, 0x0000)

#define     FSR_LLD_INIT_FAILURE                FSR_RETURN_VALUE(1, 0x2, 0x0008, 0x0000)
#define     FSR_LLD_OPEN_FAILURE                FSR_RETURN_VALUE(1, 0x2, 0x0009, 0x0000)

#define     FSR_LLD_WR_PROTECT_ERROR            FSR_RETURN_VALUE(1, 0x2, 0x000A, 0x0000)
#define     FSR_LLD_NO_UNLOCKED_BLK             FSR_RETURN_VALUE(1, 0x2, 0x000B, 0x0000)

#define     FSR_LLD_NO_RESPONSE                 FSR_RETURN_VALUE(1, 0x2, 0x000C, 0x0000)
#define     FSR_LLD_OAM_ACCESS_ERROR            FSR_RETURN_VALUE(1, 0x2, 0x000D, 0x0000)
#define     FSR_LLD_PAM_ACCESS_ERROR            FSR_RETURN_VALUE(1, 0x2, 0x000E, 0x0000)

#define     FSR_LLD_UNLOCKED_BLK_NOT_EXIST      FSR_RETURN_VALUE(1, 0x2, 0x000F, 0x0000)

/* when do PI erase or program, PI block must be unlocked                    */
#define     FSR_LLD_PI_PROGRAM_ERROR            FSR_RETURN_VALUE(1, 0x2, 0x0010, 0x0000)
#define     FSR_LLD_PI_ERASE_ERROR              FSR_RETURN_VALUE(1, 0x2, 0x0011, 0x0000)

#define     FSR_LLD_BLK_PROTECTION_ERROR        FSR_RETURN_VALUE(1, 0x2, 0x0012, 0x0000)

#define     FSR_LLD_OTP_ALREADY_LOCKED          FSR_RETURN_VALUE(1, 0x2, 0x0013, 0x0000)
#define     FSR_LLD_ALREADY_OPEN                FSR_RETURN_VALUE(1, 0X2, 0x0014, 0x0000)
#define     FSR_LLD_MALLOC_FAIL                 FSR_RETURN_VALUE(1, 0x2, 0x0015, 0x0000)

#define     FSR_LLD_INVALID_BLOCK_STATE         FSR_RETURN_VALUE(1, 0x2, 0x0016, 0x0000)

#define     FSR_LLD_PI_READ_ERROR               FSR_RETURN_VALUE(1, 0x2, 0x0017, 0x0000)

#define     FSR_LLD_PREV_1LV_READ_DISTURBANCE   FSR_RETURN_VALUE(1, 0x2, 0x0018, 0x0000)
#define     FSR_LLD_PREV_2LV_READ_DISTURBANCE   FSR_RETURN_VALUE(1, 0x2, 0x0019, 0x0000)


/*****************************************************************************/
/* Minor Retun Value for following major error values                        */
/* - FSR_LLD_PREV_WRITE_ERROR                                                */
/* - FSR_LLD_PREV_ERASE_ERROR                                                */
/* - FSR_LLD_PREV_READ_DISTURBANCE                                           */
/* - FSR_LLD_PREV_READ_ERROR                                                 */
/*****************************************************************************/
#define     FSR_LLD_1STPLN_PREV_ERROR           (0x00000001)
#define     FSR_LLD_1STPLN_CURR_ERROR           (0x00000002)
#define     FSR_LLD_2NDPLN_PREV_ERROR           (0x00000004)
#define     FSR_LLD_2NDPLN_CURR_ERROR           (0x00000008)

/*****************************************************************************/
/* Minor Return Values for Major return value of FSR_LLD_INIT_BADBLOCK       */
/*****************************************************************************/
#define     FSR_LLD_BAD_BLK_1STPLN              (0x00000001)
#define     FSR_LLD_BAD_BLK_2NDPLN              (0x00000002)

/*****************************************************************************/
/* LLD IO Ctrl Code                                                          */
/*****************************************************************************/
#define     FSR_LLD_IOCTL_PI_READ               FSR_IOCTL_CODE(FSR_MODULE_LLD,         \
                                                               0x00,                   \
                                                               FSR_METHOD_INOUT_DIRECT,\
                                                               FSR_READ_ACCESS)

/* FSR_LLD_IOCTL_PI_WRITE does folowing in sequence
 *                        first  erase PI block,
 *                        second program PI block,
 *                        then   update it
 */
#define     FSR_LLD_IOCTL_PI_WRITE              FSR_IOCTL_CODE(FSR_MODULE_LLD,         \
                                                               0x01,                   \
                                                               FSR_METHOD_INOUT_DIRECT,\
                                                               FSR_WRITE_ACCESS)

#define     FSR_LLD_IOCTL_OTP_LOCK              FSR_IOCTL_CODE(FSR_MODULE_LLD,         \
                                                               0x05,                   \
                                                               FSR_METHOD_INOUT_DIRECT,\
                                                               FSR_WRITE_ACCESS)

#define     FSR_LLD_IOCTL_OTP_GET_INFO          FSR_IOCTL_CODE(FSR_MODULE_LLD,         \
                                                               0x06,                   \
                                                               FSR_METHOD_INOUT_DIRECT,\
                                                               FSR_READ_ACCESS)

#define     FSR_LLD_IOCTL_LOCK_TIGHT             FSR_IOCTL_CODE(FSR_MODULE_LLD,         \
                                                               0x07,                   \
                                                               FSR_METHOD_INOUT_DIRECT,\
                                                               FSR_WRITE_ACCESS)

#define     FSR_LLD_IOCTL_LOCK_BLOCK            FSR_IOCTL_CODE(FSR_MODULE_LLD,         \
                                                               0x08,                   \
                                                               FSR_METHOD_INOUT_DIRECT,\
                                                               FSR_WRITE_ACCESS)

#define     FSR_LLD_IOCTL_GET_LOCK_STAT         FSR_IOCTL_CODE(FSR_MODULE_LLD,         \
                                                              0x09,                   \
                                                               FSR_METHOD_INOUT_DIRECT,\
                                                               FSR_READ_ACCESS)

#define     FSR_LLD_IOCTL_UNLOCK_BLOCK          FSR_IOCTL_CODE(FSR_MODULE_LLD,         \
                                                               0x0A,                   \
                                                               FSR_METHOD_INOUT_DIRECT,\
                                                               FSR_ANY_ACCESS)

#define     FSR_LLD_IOCTL_UNLOCK_ALLBLK         FSR_IOCTL_CODE(FSR_MODULE_LLD,         \
                                                               0x0B,                   \
                                                               FSR_METHOD_INOUT_DIRECT,\
                                                               FSR_ANY_ACCESS)

#define     FSR_LLD_IOCTL_HOT_RESET             FSR_IOCTL_CODE(FSR_MODULE_LLD,         \
                                                              0x0C,                   \
                                                               FSR_METHOD_INOUT_DIRECT,\
                                                               FSR_ANY_ACCESS)

#define     FSR_LLD_IOCTL_CORE_RESET            FSR_IOCTL_CODE(FSR_MODULE_LLD,         \
                                                               0x0D,                   \
                                                               FSR_METHOD_INOUT_DIRECT,\
                                                               FSR_ANY_ACCESS)

#define     FSR_LLD_IOCTL_OTP_ACCESS            FSR_IOCTL_CODE(FSR_MODULE_LLD,         \
                                                               0x0E,                   \
                                                               FSR_METHOD_INOUT_DIRECT,\
                                                               FSR_ANY_ACCESS)

#define     FSR_LLD_IOCTL_PI_ACCESS             FSR_IOCTL_CODE(FSR_MODULE_LLD,         \
                                                               0x0F,                   \
                                                               FSR_METHOD_INOUT_DIRECT,\
                                                               FSR_ANY_ACCESS)

#define     FSR_LLD_IOCTL_SYS_CONF_RECOVERY     FSR_IOCTL_CODE(FSR_MODULE_LLD,         \
                                                               0x10,                   \
                                                               FSR_METHOD_IN_DIRECT,   \
                                                               FSR_ANY_ACCESS)

/* FSR_LLD_IOCTL_PI_READ/WRITE takes pointer to LLDPIArg as pBufO, pBufI
 * a member of LLDPIArg takes one of followings
 */
#define     FSR_LLD_IOCTL_LOCK_PI               (0x4C4F434B) /* ascii for LOCK       */
#define     FSR_LLD_IOCTL_UNLOCK_PI             (0x554E4C4B) /* ascii for UNLK       */

/* FSR_LLD_IOCTL_OTP_LOCK takes combination of folowings as pBufI            */
#define     FSR_LLD_OTP_LOCK_1ST_BLK            (0x00000001)
#define     FSR_LLD_OTP_LOCK_OTP_BLK            (0x00000002)

/* return value of FSR_LLD_IOCTL_OTP_GET_INFO                                */
#define     FSR_LLD_OTP_1ST_BLK_LOCKED          (0x00000001)
#define     FSR_LLD_OTP_1ST_BLK_UNLKED          (0x00000002)
#define     FSR_LLD_OTP_OTP_BLK_LOCKED          (0x00000010)
#define     FSR_LLD_OTP_OTP_BLK_UNLKED          (0x00000020)

/* parameter[out], pBlockType of LLD_GetBlockInfo() takes one of these       */
#define     FSR_LLD_SLC_BLOCK                   (0x00000001)
#define     FSR_LLD_MLC_BLOCK                   (0x00000002)

/*****************************************************************************/
/** LLD Function Table Data Structures                                       */
/*****************************************************************************/
typedef struct
{
    INT32   (*LLD_Init)                 (UINT32             nFlag);
    INT32   (*LLD_Open)                 (UINT32             nDev,
                                         VOID              *pParam,
                                         UINT32             nFlag);
    INT32   (*LLD_Close)                (UINT32             nDev,
                                         UINT32             nFlag);
    INT32   (*LLD_Erase)                (UINT32             nDev,
                                         UINT32            *pnPbn,
                                         UINT32             nNumOfBlks,
                                         UINT32             nFlag);
    INT32   (*LLD_ChkBadBlk)            (UINT32             nDev,
                                         UINT32             nPbn,
                                         UINT32             nFlag);
    INT32   (*LLD_FlushOp)              (UINT32             nDev,
                                         UINT32             nDieIdx,
                                         UINT32             nFlag);
    INT32   (*LLD_GetDevSpec)           (UINT32             nDev,
                                         FSRDevSpec        *pstDevSpec,
                                         UINT32             nFlag);
    INT32   (*LLD_Read)                 (UINT32             nDev, 
                                         UINT32             nPbn,
                                         UINT32             nPgOffset,
                                         UINT8             *pMBuf,
                                         FSRSpareBuf       *pSBuf,
                                         UINT32             nFlag);
    INT32   (*LLD_ReadOptimal)          (UINT32             nDev, 
                                         UINT32             nPbn,
                                         UINT32             nPgOffset,
                                         UINT8             *pMBuf,
                                         FSRSpareBuf       *pSBuf,
                                         UINT32             nFlag);
    INT32   (*LLD_Write)                (UINT32             nDev,
                                         UINT32             nPbn,
                                         UINT32             nPgOffset,
                                         UINT8             *pMBuf,
                                         FSRSpareBuf       *pSBuf,
                                         UINT32             nFlag);
    INT32   (*LLD_CopyBack)             (UINT32             nDev,
                                         LLDCpBkArg        *pstCpArg,
                                         UINT32             nFlag);
    INT32   (*LLD_GetPrevOpData)        (UINT32             nDev,
                                         UINT8             *pMBuf,
                                         FSRSpareBuf       *pSBuf,
                                         UINT32             nDieIdx,
                                         UINT32             nFlag);
    INT32   (*LLD_IOCtl)                (UINT32             nDev,
                                         UINT32             nCode,
                                         UINT8             *pBufI,
                                         UINT32             nLenI,
                                         UINT8             *pBufO,
                                         UINT32             nLenO,
                                         UINT32            *pByteRet);
    INT32   (*LLD_InitLLDStat)          (VOID);
    INT32   (*LLD_GetStat)              (FSRLLDStat        *pStat);
    INT32   (*LLD_GetBlockInfo)         (UINT32             nDev,
                                         UINT32             nPbn,
                                         UINT32            *pBlockType,
                                         UINT32            *pPgsPerBlk);
    INT32   (*LLD_GetNANDCtrllerInfo)   (UINT32             nDev,
                                         LLDPlatformInfo   *pLLDPltInfo);
} FSRLowFuncTbl;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _FSR_LLD_H_ */

