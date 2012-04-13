/**
 *   @mainpage   Flex Sector Remapper : LinuStoreIII_1.2.0_b035-FSR_1.2.1p1_b129_RC
 *
 *   @section Intro
 *       Flash Translation Layer for Flex-OneNAND and OneNAND
 *    
 *    @section  Copyright
 *            COPYRIGHT. 2007-2009 SAMSUNG ELECTRONICS CO., LTD.               
 *                            ALL RIGHTS RESERVED                              
 *                                                                             
 *     Permission is hereby granted to licensees of Samsung Electronics        
 *     Co., Ltd. products to use or abstract this computer program for the     
 *     sole purpose of implementing a product based on Samsung                 
 *     Electronics Co., Ltd. products. No other rights to reproduce, use,      
 *     or disseminate this computer program, whether in part or in whole,      
 *     are granted.                                                            
 *                                                                             
 *     Samsung Electronics Co., Ltd. makes no representation or warranties     
 *     with respect to the performance of this computer program, and           
 *     specifically disclaims any responsibility for any damages,              
 *     special or consequential, connected with the use of this program.       
 *
 *     @section Description
 *
 */

/**
 * @file      FSR_LLD_4K_OneNAND.c
 * @brief     This file implements Low Level Driver for OneNAND 4K device
 * @author    NamOh Hwang
 * @author    SongHo Yoon
 * @date      10-JAN-2007
 * @remark
 * REVISION HISTORY
 * @n  10-JAN-2007 [NamOh Hwang] : first writing
 * @n  29-AUG-2008 [KyungHo Shin] : 42nm OneNAND 4K support
 *
 */

/******************************************************************************/
/* Header file inclusions                                                     */
/******************************************************************************/
#define     FSR_NO_INCLUDE_BML_HEADER
#define     FSR_NO_INCLUDE_STL_HEADER

#include    "FSR.h"
#include    "FSR_LLD_4K_OneNAND.h"
#include    "FSR_LLD_SWEcc.h"

/******************************************************************************/
/*   Local Configurations                                                     */
/*                                                                            */
/* - FSR_LLD_BIG_ENDIAN        : to support big endianess                     */
/* - FSR_LLD_STRICT_CHK        : to check parameters strictly                 */
/* - FSR_ONENAND_EMULATOR      : to use 4K OneNAND emulator                   */
/*                               instead of real device (for debugging only)  */
/* - FSR_FLEXIA                : to build with Flexia (FSR image tool)        */
/* - FSR_LLD_STATISTICS        : to accumulate statistics.                    */
/* - FSR_LLD_LOGGING_HISTORY   : to log history                               */
/* - FSR_LLD_HANDSHAKE_ERR_INF : to support TINY FSR                          */
/******************************************************************************/

//#define     FSR_LLD_BIG_ENDIAN
#define     FSR_LLD_STRICT_CHK
//#define     FSR_LLD_STATISTICS
//#define     FSR_LLD_LOGGING_HISTORY
#define     FSR_LLD_USE_CACHE_PGM
//#define     FSR_LLD_WAIT_ALLDIE_PGM_READY
#define     FSR_LLD_USE_SUPER_LOAD
//#define     FSR_LLD_WAIT_WR_PROTECT_STAT
//#define     FSR_LLD_ENABLE_DEBUG_PORT
//#define     FSR_LLD_PE_TEST

#if defined(FSR_BIG_ENDIAN)
#define     FSR_LLD_BIG_ENDIAN
#endif

#if defined(FSR_ONENAND_EMULATOR)
#define     FSR_LLD_STATISTICS
#define     FSR_LLD_LOGGING_HISTORY
#endif

#if defined(FSR_NBL2)
#undef      FSR_LLD_STATISTICS
#undef      FSR_LLD_LOGGING_HISTORY
#endif

#if defined(FSR_ONENAND_EMULATOR)
#include    "FSR_FOE_Interface.h"
#endif /* #if defined(FSR_ONENAND_EMULATOR) */

/******************************************************************************/
/* Local #defines                                                             */
/******************************************************************************/

#define     FSR_OND_4K_MAX_DEVS                (FSR_MAX_DEVS)
#define     FSR_OND_4K_1ST_DIE                 (0)
#define     FSR_OND_4K_2ND_DIE                 (1)

#define     FSR_OND_4K_DID_MASK                (0xFFF8) /* Device ID Register    */
#define     FSR_OND_4K_DID_VCC_MASK            (0x0003) /* Device ID Register    */
#define     FSR_OND_4K_DID_VCC_33V             (0x0001) /* Device ID Register    */
#define     FSR_OND_4K_STEPPING_ID_MASK        (0x000F) /* Version ID Register   */
#define     FSR_OND_4K_CS_VERSION              (0x0001) /* cs version not support
                                                        Erasing of PI Block   */
#define     FSR_OND_4K_INT_MASTER_READY        (0x8000) /* interrupt status reg. */
#define     FSR_OND_4K_INT_READ_READY          (0x0080)
#define     FSR_OND_4K_INT_WRITE_READY         (0x0040)
#define     FSR_OND_4K_INT_ERASE_READY         (0x0020)
#define     FSR_OND_4K_INT_RESET_READY         (0x0010)
#define     FSR_OND_4K_DFS_BASEBIT             (15)     /* start address 1 reg.  */
#define     FSR_OND_4K_DFS_MASK                (0x8000)
#define     FSR_OND_4K_DBS_BASEBIT             (15)     /* start address 2 reg.  */
#define     FSR_OND_4K_DBS_MASK                (0x8000)
#define     FSR_OND_4K_STATUS_ERROR            (0x0400) /* controller status reg.*/
#define     FSR_OND_4K_PREV_CACHEPGM_ERROR     (0x0004) /* controller status reg.*/
#define     FSR_OND_4K_CURR_CACHEPGM_ERROR     (0x0002) /* controller status reg.*/

#if defined(FSR_PE_TEST)
#define     FSR_OND_4K_ECC_READ_DISTURBANCE    (0x0F0F) /* ECC 1~4 bit error     */
#else
#define     FSR_OND_4K_ECC_READ_DISTURBANCE    (0x0E0E) /* ECC 2~4 bit error     */
#endif


#define     FSR_OND_4K_ECC_UNCORRECTABLE       (0x1010) /* ECC status reg.1,2,3,4*/

/* the size of array whose element can be put into command register           */
#define     FSR_OND_4K_MAX_PGMCMD              (2)  /* the size of gnPgmCmdArray */
#define     FSR_OND_4K_MAX_LOADCMD             (6)  /* the size of gnLoadCmdArray*/
#define     FSR_OND_4K_MAX_ERASECMD            (1)  /* the size of gnEraseCmdArra*/
#define     FSR_OND_4K_MAX_CPBKCMD             (3)  /* the size of gnCpBkCmdArray*/

#define     FSR_OND_4K_MAX_BADMARK             (4)  /* the size of gnBadMarkValue*/
#define     FSR_OND_4K_MAX_BBMMETA             (2)  /* the size of gnBBMMetaValue*/


#define     FSR_OND_4K_MAX_ECC_STATUS_REG      (4)  /* # of ECC status registers */


#define     FSR_OND_4K_VALID_BLK_MARK          (0xFFFF)


#define     FSR_OND_4K_SECTOR_SIZE             (FSR_SECTOR_SIZE)


/* hardware ECC of 4K OneNAND use five UINT16 (9 bytes) of spare area      */
#define     FSR_OND_4K_SPARE_HW_ECC_AREA       (5)

/* hardware ECC of 4K OneNAND use five UINT16 (2 bytes) of spare area      */
#define     FSR_OND_4K_SPARE_SW_ECC_AREA       (2)

/* out of 16 bytes in spare area of 1 sector, LLD use 6 bytes                 */
#define     FSR_OND_4K_SPARE_USER_AREA         (6)

#define     FSR_OND_4K_BUSYWAIT_TIME_LIMIT     (10000000)

/* Write protect time out count */
#define     FSR_OND_4K_WR_PROTECT_TIME_LIMIT   (25000000)

/* Transfering data between DataRAM & host DRAM depends on DMA & sync mode.
 * Because DMA & sync burst needs some preparation,
 * when the size of transferring data is small,
 * it's better to use load, store command of ARM processor.
 * Confer _ReadMain(), _WriteMain().
 * FSR_OND_4K_MIN_BULK_TRANS_SIZE stands for minimum transfering size.
 */
#define     FSR_OND_4K_MIN_BULK_TRANS_SIZE     (16 * 2)

/* Buffer Sector Count specifies the number of sectors to load
 * LLD does not support sector-based load.
 * Instead, LLD always load 1 page, and selectively transfer sectors.
 */
#define     FSR_OND_4K_START_BUF_DEFAULT       (0x0800)

#define     FSR_OND_4K_OTP_LOCK_MASK           (0x00FF)
#define     FSR_OND_4K_LOCK_OTP_BLOCK          (0x00FC)
#define     FSR_OND_4K_LOCK_1ST_BLOCK_OTP      (0x00F3)
#define     FSR_OND_4K_LOCK_BOTH_OTP           (0x00F0)
#define     FSR_OND_4K_OTP_PAGE_OFFSET         (49)

/* controller status register                                                 */
#define     FSR_OND_4K_CTLSTAT_OTP_MASK        (0x0060)
#define     FSR_OND_4K_CTLSTAT_OTP_BLK_LOCKED  (0x0040)
#define     FSR_OND_4K_CTLSTAT_1ST_OTP_LOCKED  (0x0020)


/* 4K OneNAND command which is written to Command Register                  */
#define     FSR_OND_4K_CMD_LOAD                (0x0000)
#define     FSR_OND_4K_CMD_SUPERLOAD           (0x0003)
#define     FSR_OND_4K_CMD_PROGRAM             (0x0080)
#define     FSR_OND_4K_CMD_CACHEPGM            (0x007F)
#define     FSR_OND_4K_CMD_UNLOCK_BLOCK        (0x0023)
#define     FSR_OND_4K_CMD_LOCK_BLOCK          (0x002A)
#define     FSR_OND_4K_CMD_LOCKTIGHT_BLOCK     (0x002C)
#define     FSR_OND_4K_CMD_UNLOCK_ALLBLOCK     (0x0027)
#define     FSR_OND_4K_CMD_ERASE               (0x0094)
#define     FSR_OND_4K_CMD_RST_NFCORE          (0x00F0)
#define     FSR_OND_4K_CMD_HOT_RESET           (0x00F3)
#define     FSR_OND_4K_CMD_OTP_ACCESS          (0x0065)

/* 14th bit of Controller Status Register (F240h) of OneNAND shows
 * whether host is programming/erasing a locked block of the NAND Flash Array
 * 4K OneNAND does not use this bit, and it is fixed to zero.
 */
#define     FSR_OND_4K_LOCK_STATE              (0x4000)

/* with the help of emulator, LLD can run without the 4K OneNAND
 * in that case, define FSR_ONENAND_EMULATOR
 */
#if defined (FSR_ONENAND_EMULATOR)

    #define     OND_4K_WRITE(nAddr, nDQ)                                          \
                   {FSR_FOE_Write((UINT32)&(nAddr), nDQ);}

    #define     OND_4K_READ(nAddr)                                                \
                   ((UINT16) FSR_FOE_Read((UINT32)&(nAddr)))

    #define     OND_4K_SET(nAddr, nDQ)                                            \
                   {FSR_FOE_Write((UINT32)&(nAddr),                            \
                   (UINT16) FSR_FOE_Read((UINT32)&(nAddr)) | nDQ);}

    #define     OND_4K_CLR(nAddr, nDQ)                                            \
                   {FSR_FOE_Write((UINT32)&(nAddr),                            \
                   (UINT16) FSR_FOE_Read((UINT32)&(nAddr)) & nDQ);}

    #define     TRANSFER_TO_NAND(pDst, pSrc, nSize)                            \
                    FSR_FOE_TransferToDataRAM(pDst, pSrc, nSize)

    /* nSize is the number of bytes to transfer                               */
    #define     TRANSFER_FROM_NAND(pDst, pSrc, nSize)                          \
                    FSR_FOE_TransferFromDataRAM(pDst, pSrc, nSize)

    /* macro below is for emulation purpose */
    #define     OND_4K_MEMSET_DATARAM(pDst, nVal, nSize)                          \
                    FSR_FOE_MemsetDataRAM((pDst), (nVal), (nSize))

#elif defined (FSR_FLEXIA)

    #define     OND_4K_WRITE(nAddr, nDQ)                                          \
                   {FSRDLL_FOE_Write((UINT32)&(nAddr), nDQ);}

    #define     OND_4K_READ(nAddr)                                                \
                   ((UINT16) FSRDLL_FOE_Read((UINT32)&(nAddr)))

    #define     OND_4K_SET(nAddr, nDQ)                                            \
                   {FSRDLL_FOE_Write((UINT32)&(nAddr),                            \
                   (UINT16) FSRDLL_FOE_Read((UINT32)&(nAddr)) | nDQ);}

    #define     OND_4K_CLR(nAddr, nDQ)                                            \
                   {FSRDLL_FOE_Write((UINT32)&(nAddr),                            \
                   (UINT16) FSRDLL_FOE_Read((UINT32)&(nAddr)) & nDQ);}

    #define     TRANSFER_TO_NAND(pDst, pSrc, nSize)                            \
                    FSRDLL_FOE_TransferToDataRAM(pDst, pSrc, nSize)

    /* nSize is the number of bytes to transfer                               */
    #define     TRANSFER_FROM_NAND(pDst, pSrc, nSize)                          \
                    FSRDLL_FOE_TransferFromDataRAM(pDst, pSrc, nSize)

    /* macro below is for emulation purpose */
    #define     OND_4K_MEMSET_DATARAM(pDst, nVal, nSize)                          \
                    FSRDLL_FOE_MemsetDataRAM((pDst), (nVal), (nSize))

#elif defined (FSR_MSM7200)
    /* onenand controller does word access, but MSM7200 controller does 4byte access */

    #define     OND_4K_WRITE(nAddr, nDQ)   FSR_PAM_WriteToOneNANDRegister((UINT32)&nAddr, nDQ)

    #define     OND_4K_READ(nAddr)         FSR_PAM_ReadOneNANDRegister((UINT32)&nAddr)

    #define     OND_4K_SET(nAddr, nDQ)                                           \
    {                                                                             \
        UINT32 nData;                                                             \
        nData = OND_4K_READ(nAddr);                                              \
        OND_4K_WRITE(nAddr, (nData | nDQ));                                      \
    }

    #define     OND_4K_CLR(nAddr, nDQ)                                           \
    {                                                                             \
        UINT32 nData;                                                             \
        nData = OND_4K_READ(nAddr);                                              \
        OND_4K_WRITE(nAddr, (nData & nDQ));                                      \
    }

    /* nSize is the number of bytes to transfer                                  */
    #define     TRANSFER_TO_NAND(pDst, pSrc, nSize)                               \
                    FSR_PAM_TransToNAND(pDst, pSrc, nSize)

    #define     TRANSFER_FROM_NAND(pDst, pSrc, nSize)                             \
                    FSR_PAM_TransFromNAND(pDst, pSrc, nSize)

    #define     OND_MEMSET_DATARAM(pDst, nVal, nSize)                          \
                    FSR_OAM_MEMSET((pDst), (nVal), (nSize))                    
#elif defined (FSR_DENALI_CONTROLLER)

    #define     OND_4K_WRITE(nAddr, nDQ)   FSR_PAM_WriteToDenaliRegister((UINT32)&nAddr, nDQ)
    #define     OND_4K_READ(nAddr)         FSR_PAM_ReadDenaliRegister((UINT32)&nAddr)

    #define     OND_4K_SET(nAddr, nDQ)     OND_4K_WRITE(nAddr, OND_4K_READ(nAddr) | nDQ)
    #define     OND_4K_CLR(nAddr, nDQ)     OND_4K_WRITE(nAddr, OND_4K_READ(nAddr) & nDQ)
    
    /* nSize is the number of bytes to transfer                               */
    #define     TRANSFER_TO_NAND(pDst, pSrc, nSize)                            \
                    FSR_PAM_TransToNAND(pDst, pSrc, nSize)
    #define     TRANSFER_FROM_NAND(pDst, pSrc, nSize)                          \
                    FSR_PAM_TransFromNAND(pDst, pSrc, nSize)
  
    #define     OND_MEMSET_DATARAM(pDst, nVal, nSize)                          \
                    FSR_OAM_MEMSET((void*)((UINT32)(pDst) | FSR_PAM_GetBase00Address()), (nVal), (nSize))
                    
#else /* #if defined (FSR_ONENAND_EMULATOR)                                   */

    #define     OND_4K_WRITE(nAddr, nDQ)   {nAddr  = nDQ;}
    #define     OND_4K_READ(nAddr)         (nAddr        )
    #define     OND_4K_SET(nAddr, nDQ)     {nAddr  = (nAddr | nDQ);}
    #define     OND_4K_CLR(nAddr, nDQ)     {nAddr  = (nAddr & nDQ);}

    /* nSize is the number of bytes to transfer                               */
    #define     TRANSFER_TO_NAND(pDst, pSrc, nSize)                            \
                    FSR_PAM_TransToNAND(pDst, pSrc, nSize)

    #define     TRANSFER_FROM_NAND(pDst, pSrc, nSize)                          \
                    FSR_PAM_TransFromNAND(pDst, pSrc, nSize)

    /* macro below is for emulation purpose
     * in real target environment, it's same as memset()
     */
    #define     OND_4K_MEMSET_DATARAM(pDst, nVal, nSize)                          \
                    FSR_OAM_MEMSET((pDst), (nVal), (nSize))

#endif /* #if defined (FSR_ONENAND_EMULATOR)                                  */

/* In order to wait 200ns, call OND_4K_READ(x->nInt) 12 times */
#define WAIT_OND_4K_INT_STAT(x, a)                                                \
    {INT32 nTimeLimit = FSR_OND_4K_BUSYWAIT_TIME_LIMIT;                           \
    OND_4K_READ(x->nInt);                                                         \
    OND_4K_READ(x->nInt);                                                         \
    OND_4K_READ(x->nInt);                                                         \
    OND_4K_READ(x->nInt);                                                         \
    OND_4K_READ(x->nInt);                                                         \
    OND_4K_READ(x->nInt);                                                         \
    OND_4K_READ(x->nInt);                                                         \
    OND_4K_READ(x->nInt);                                                         \
    OND_4K_READ(x->nInt);                                                         \
    OND_4K_READ(x->nInt);                                                         \
    while ((OND_4K_READ(x->nInt) & (a)) != (UINT16)(a))                           \
    {                                                                          \
        if (--nTimeLimit == 0)                                                 \
        {                                                                      \
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,                                     \
                (TEXT("[OND:ERR]   busy wait time-out %s(), %d line\r\n"),     \
                __FSR_FUNC__, __LINE__));                                      \
            _DumpRegisters(x);                                                 \
            _DumpSpareBuffer(x);                                               \
            _DumpCmdLog();                                                     \
            FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,                  \
                (TEXT("[OND:OUT] --%s()\r\n"), __FSR_FUNC__));                 \
            return (FSR_LLD_NO_RESPONSE);                                      \
        }                                                                      \
    }}

/* In order to wait 3s, call OND_4K_READ(x->nInt) 12 times */
#define WAIT_OND_4K_WR_PROTECT_STAT(x, a)                                         \
    {INT32 nTimeLimit = FSR_OND_4K_WR_PROTECT_TIME_LIMIT;                         \
    while ((OND_4K_READ(pstFOReg->nWrProtectStat) & (a)) != (UINT16)(a))          \
    {                                                                          \
        if (--nTimeLimit == 0)                                                 \
        {                                                                      \
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,                                     \
                (TEXT("[OND:ERR] WR protect busy wait time-out %s(), %d line\r\n"),\
                __FSR_FUNC__, __LINE__));                                      \
            _DumpRegisters(x);                                                 \
            _DumpSpareBuffer(x);                                               \
            _DumpCmdLog();                                                     \
            FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,                  \
                (TEXT("[OND:OUT] --%s()\r\n"), __FSR_FUNC__));                 \
            return (FSR_LLD_NO_RESPONSE);                                      \
        }                                                                      \
    }}


/* MACROs below are used for the statistic                                    */
#define FSR_OND_4K_STAT_SLC_PGM                    (0x00000001)
#define FSR_OND_4K_STAT_LSB_PGM                    (0x00000002)
#define FSR_OND_4K_STAT_MSB_PGM                    (0x00000003)
#define FSR_OND_4K_STAT_ERASE                      (0x00000004)
#define FSR_OND_4K_STAT_SLC_LOAD                   (0x00000005)
#define FSR_OND_4K_STAT_MLC_LOAD                   (0x00000006)
#define FSR_OND_4K_STAT_RD_TRANS                   (0x00000007)
#define FSR_OND_4K_STAT_WR_TRANS                   (0x00000008)
#define FSR_OND_4K_STAT_WR_CACHEBUSY               (0x00000009)
#define FSR_OND_4K_STAT_FLUSH                      (0x0000000A)

/* command type for statistics                                                */
#define FSR_OND_4K_STAT_NORMAL_CMD                 (0x0)
#define FSR_OND_4K_STAT_PLOAD                      (0x1)
#define FSR_OND_4K_STAT_CACHE_PGM                  (0x2)

#define FSR_OND_4K_CACHE_BUSY_TIME                 (25) /* in usec               */
#define FSR_OND_4K_PAGEBUF_TO_DATARAM_TIME         (25) /* in usec               */
/* SoftWare OverHead                                                          */
#define FSR_OND_4K_RD_SW_OH                        (0)  /* in usec               */
#define FSR_OND_4K_WR_SW_OH                        (0)  /* in usec               */

/* signature value about UniqueID */
#define FSR_OND_4K_NUM_OF_UID                      (8)
#define FSR_OND_4K_NUM_OF_SIG                      (4)

#define FSR_OND_4K_FLUSHOP_CALLER_BASEBIT          (16)
#define FSR_OND_4K_FLUSHOP_CALLER_MASK             (0xFFFF0000)

/******************************************************************************/
/* Local typedefs                                                             */
/******************************************************************************/
/** data structure of 4K OneNAND specification                              */
typedef struct
{
        UINT16      nMID;               /**< manufacturer ID                  */
        UINT16      nDID;               /**< bits 0~2 of nDID are
                                             masked with FSR_OND_4K_DID_MASK     */

        UINT16      nGEN;               /**< process information              */
        UINT16      nNumOfBlks;         /**< the number of blocks             */

        UINT16      nNumOfDies;         /**< # of dies in NAND device         */
        UINT16      nNumOfPlanes;       /**< the number of planes             */

        UINT16      nSctsPerPG;         /**< the number of sectors per page   */
        UINT16      nSparePerSct;       /**< # of bytes of spare of a sector  */

        UINT32      nPgsPerBlkForSLC;   /**< # of pages per block in SLC area */
        
        BOOL32      b1stBlkOTP;         /**< support 1st block OTP or not     */

        UINT16      nUserOTPScts;       /**< # of user sectors                */
        UINT16      nRsvBlksInDev;      /**< # of total bad blocks(init + run)*/

        UINT32      nSLCTLoadTime;      /**< Typical Load     operation time  */
        UINT32      nSLCTProgTime;      /**< Typical Program  operation time  */
        UINT32      nTEraseTime;        /**< Typical Erase    operation time  */

        /* endurance information                                              */
        UINT32      nSLCPECycle;        /**< program, erase cycle of SLC block*/

} OneNAND4kSpec;

/** @brief   shared data structure for communication in Dual Core             */
typedef struct
{
    UINT32  nShMemUseCnt;

    /**< previous operation data structure which can be shared among process  */
    UINT16  nPreOp[FSR_MAX_DIES];
    UINT16  nPreOpPbn[FSR_MAX_DIES];
    UINT16  nPreOpPgOffset[FSR_MAX_DIES];
    UINT32  nPreOpFlag[FSR_MAX_DIES];
} OneNAND4kShMem;

/** data structure of 4K OneNAND LLD context for each device number         */
typedef struct
{
    UINT32       nBaseAddr;             /**< the base address of 4K OneNAND */
    BOOL32       bOpen;                 /**< open flag : TRUE32 or FALSE32    */

    OneNAND4kSpec *pstOND4kSpec;            /**< pointer to OneNAND4kSpec           */

    UINT16       nFBAMask;              /**< the mask of Flash Block Address  */
    UINT8        nFPASelSft;            /**< the shift value of FPA selection */
    UINT8        nDDPSelSft;            /**< the shift value of DDP selection */

    UINT16       nSysConf1;             /**< when opening a device save value
                                           of system configuration 1 register.
                                           restore this value after hot reset */
    UINT16       nFlushOpCaller;

    BOOL32       bCachePgm;             /**< supports cache program           */

    BOOL32       bIsPreCmdCache[FSR_MAX_DIES];

    UINT16       nBlksForSLCArea[FSR_MAX_DIES];/**< # of blocks for SLC area  */

    UINT8       *pSpareBuffer;          /**< can cover all spare area of 1 pg */

    UINT8       *pTempBuffer;           /**< buffer for temporary use. 
                                             is used for just allocation/free */

    UINT32       nWrTranferTime;        /**< write transfer time              */
    UINT32       nRdTranferTime;        /**< read transfer time               */

    UINT32       nIntID;                /**< interrupt ID : non-blocking I/O  */
    UINT8        nUID[FSR_LLD_UID_SIZE];/**< Unique ID info. about OTP block  */

#if defined (FSR_LLD_STATISTICS)
    UINT32       nNumOfSLCLoads; /**< the number of times of Load  operations */
    UINT32       nNumOfMLCLoads; /**< the number of times of Load  operations */

    UINT32       nNumOfSLCPgms;  /**< the number of times of SLC programs     */
    UINT32       nNumOfLSBPgms;  /**< the number of times of LSB programs     */
    UINT32       nNumOfMSBPgms;  /**< the number of times of MSB programs     */

    UINT32       nNumOfCacheBusy;/**< the number of times of Cache Busy       */
    UINT32       nNumOfErases;   /**< the number of times of Erase operations */

    UINT32       nNumOfRdTrans;  /**< the number of times of Read  Transfer   */
    UINT32       nNumOfWrTrans;  /**< the number of times of Write Transfer   */

    UINT32       nPreCmdOption[FSR_MAX_DIES]; /** previous command option     */

    INT32        nIntLowTime[FSR_MAX_DIES];
                                 /**< MDP : 0 
                                      DDP : previous operation time           */

    UINT32       nRdTransInBytes;/**< the number of bytes transfered in read  */
    UINT32       nWrTransInBytes;/**< the number of bytes transfered in write */
#endif /* #if defined (FSR_LLD_STATISTICS)                                    */

} OneNAND4kCxt;



/******************************************************************************/
/* Global variable definitions                                                */
/******************************************************************************/

/* gnPgmCmdArray contains the actual COMMAND for the program
 * the size of array can be changed for the future extention
 * if device does not support Cache Program, LLD_Open() fills it with
 * normal program command
 */
PRIVATE       UINT16    gnPgmCmdArray[FSR_OND_4K_MAX_PGMCMD] =
                            { FSR_OND_4K_CMD_PROGRAM, /* FSR_LLD_FLAG_1X_PROGRAM */
                              FSR_OND_4K_CMD_CACHEPGM,/* FSR_LLD_FLAG_1X_CACHEPGM*/
                                                   /* FSR_LLD_FLAG_2X_PROGRAM */
                                                   /* FSR_LLD_FLAG_2X_CACHEPGM*/
                            };


/* gnLoadCmdArray contains the actual COMMAND for the load
 * the size of array can be changed for the future extention
 * gnLoadCmdArray is not a const type, so value can change
 */
PRIVATE       UINT16    gnLoadCmdArray[FSR_OND_4K_MAX_LOADCMD] =
                            { 0xFFFF,           /* FSR_LLD_FLAG_NO_LOADCMD    */
                              FSR_OND_4K_CMD_LOAD, /* FSR_LLD_FLAG_1X_LOAD       */
                              FSR_OND_4K_CMD_SUPERLOAD, /* FSR_LLD_FLAG_1X_PLOAD */
                              0xFFFF,           /* FSR_LLD_FLAG_2X_LOAD       */
                              0xFFFF,           /* FSR_LLD_FLAG_2X_PLOAD      */
                            };

/* gnEraseCmdArray contains the actual COMMAND for the erase
 * the size of array can be changed for the future extention
 */
PRIVATE const UINT16    gnEraseCmdArray[FSR_OND_4K_MAX_ERASECMD] =
                            { FSR_OND_4K_CMD_ERASE,/* FSR_LLD_FLAG_1X_ERASE      */
                                                /* FSR_LLD_FLAG_2X_ERASE      */
                            };

/* gnBadMarkValue contains the actual 2 byte value 
 * which is programmed into the first 2 byte of spare area
 * the size of array can be changed for the future extention
 */
PRIVATE const UINT16    gnBadMarkValue[FSR_OND_4K_MAX_BADMARK]   =
                            { 0xFFFF,         /* FSR_LLD_FLAG_WR_NOBADMARK    */
                              0x2222,         /* FSR_LLD_FLAG_WR_EBADMARK     */
                              0x4444,         /* FSR_LLD_FLAG_WR_WBADMARK     */
                              0x8888,         /* FSR_LLD_FLAG_WR_LBADMARK     */
                            };

/* gnCpBkCmdArray contains the actual COMMAND for the copyback
 * the size of array can be changed for the future extention
 */
PRIVATE const UINT16    gnCpBkCmdArray[FSR_OND_4K_MAX_CPBKCMD]   =
                            { 0xFFFF,
                              FSR_OND_4K_CMD_LOAD, /* FSR_LLD_FLAG_1X_CPBK_LOAD  */
                              FSR_OND_4K_CMD_PROGRAM,/*FSR_LLD_FLAG_1X_CPBK_PROGR*/
                                              /* FSR_LLD_FLAG_2X_CPBK_LOAD    */
                                              /* FSR_LLD_FLAG_2X_CPBK_PROGRAM */
                                              /* FSR_LLD_FLAG_4X_CPBK_LOAD    */
                                              /* FSR_LLD_FLAG_4X_CPBK_PROGRAM */
                            };

PRIVATE const UINT16    gnBBMMetaValue[FSR_OND_4K_MAX_BBMMETA]   =
                            { 0xFFFF,
                              FSR_LLD_BBM_META_MARK
                            };

#if defined (FSR_LLD_LOGGING_HISTORY) && !defined(FSR_OAM_RTLMSG_DISABLE)
PRIVATE const UINT8    *gpszLogPreOp[] =
                            { (UINT8 *) "NONE ",
                              (UINT8 *) "READ ",
                              (UINT8 *) "CLOAD",
                              (UINT8 *) "CAPGM",
                              (UINT8 *) "PROG ",
                              (UINT8 *) "CBPGM",
                              (UINT8 *) "ERASE",
                              (UINT8 *) "IOCTL",
                              (UINT8 *) "HTRST",
                            };
#endif

PRIVATE OneNAND4kCxt     *gpstOND4kCxt[FSR_OND_4K_MAX_DEVS];

PRIVATE OneNAND4kShMem *gpstOND4kShMem[FSR_OND_4K_MAX_DEVS];
PRIVATE UINT16          gstSpare4kBuffer[(FSR_MAX_PHY_SCTS * FSR_SPARE_SIZE) /2];

#if defined (FSR_LLD_STATISTICS)
PRIVATE UINT32          gnElapsedTime;
PRIVATE UINT32          gnDevsInVol[FSR_MAX_VOLS] = {0, 0};
#endif /* #if defined (FSR_LLD_STATISTICS)                                    */


/* FSR_OND_4K_Open() maps this function pointer to _ReadOptWithSLoad() or
 * _ReadOptWithNonSLoad() according to sync mode
 */
PRIVATE INT32 (*gpfReadOptimal)(UINT32         nDev,
                                UINT32         nPbn,
                                UINT32         nPgOffset,
                                UINT8         *pMBuf,
                                FSRSpareBuf   *pSBuf,
                                UINT32         nFlag);

/******************************************************************************/
/* Extern variable declarations                                               */
/******************************************************************************/

#if defined(FSR_LLD_PE_TEST)
PUBLIC UINT16 gnECCStat0;
PUBLIC UINT16 gnECCStat1;
PUBLIC UINT16 gnECCStat2;
PUBLIC UINT16 gnECCStat3;
PUBLIC UINT32 gnPbn;
PUBLIC UINT32 gnPgOffset;
#endif

PRIVATE const OneNAND4kSpec gstOND4kSpec[] = {
/**********************************************************************************************************************/
/* 1. nMID                                                                                                            */
/* 2.         nDID                                                                                                    */
/* 3.                 nGEN                                                                                            */
/* 4.                    nNumOfBlks                                                                                   */
/* 5 .                         nNumOfDies                                                                             */
/* 6.                             nNumOfPlanes                                                                        */
/* 7.                                nSctsPerPG                                                                       */
/* 8                                    nSparePerSct                                                                  */
/* 9.                                       nPgsPerBlkForSLC                                                          */
/* 10.                                               b1stBlkOTP                                                       */
/* 11.                                                       nUserOTPScts                                             */
/* 12.                                                           nRsvBlksInDev                                        */
/* 13.                                                                nSLCTLoadTime                                   */
/* 14.                                                                    nSLCTProgTime                               */
/* 15.                                                                          nTEraseTime                           */
/* 16                                                                                  nSLCPECycle                    */
/**********************************************************************************************************************/
    /* 4Gb */
    { 0x00EC, 0x0050, 0, 2048, 1, 1, 8, 16, 64, TRUE32, 50, 40, 45, 240, 500, 50000},

    /* 8Gb DDP */
    { 0x00EC, 0x0068, 0, 4096, 2, 1, 8, 16, 64, TRUE32, 50, 80, 45, 240, 500, 50000},

    {      0,      0, 0,    0, 0, 0, 0,  0, 0,  FALSE32,  0,  0, 0,   0,   0,     0},
};

/******************************************************************************/
/* Local function prototypes                                                  */
/******************************************************************************/
PRIVATE INT32 _ReadOptWithNonSLoad (UINT32       nDev,
                                    UINT32       nPbn,
                                    UINT32       nPgOffset,
                                    UINT8       *pMBuf,
                                    FSRSpareBuf *pSBuf,
                                    UINT32       nFlag);

PRIVATE INT32 _ReadOptWithSLoad    (UINT32       nDev,
                                    UINT32       nPbn,
                                    UINT32       nPgOffset,
                                    UINT8       *pMBuf,
                                    FSRSpareBuf *pSBuf,
                                    UINT32       nFlag);

PRIVATE VOID  _ReadMain            (UINT8       *pDest,
                           volatile UINT8       *pSrc,
                                    UINT32       nSize,
                                    UINT8       *pTempBuffer);

PRIVATE INT32  _ReadSpare           (FSRSpareBuf *pstDest,
                           volatile UINT8       *pSrc,
                                    UINT32       nFlag);

PRIVATE VOID  _WriteMain(  volatile UINT8       *pDest,
                                    UINT8       *pSrc,
                                    UINT32       nSize);

PRIVATE VOID  _WriteSpare( volatile UINT8       *pDest,
                                    FSRSpareBuf *pstSrc,
                                    UINT32       nFlag);

PRIVATE INT32 _ControlLockBlk      (UINT32  nDev, 
                                    UINT32  nPbn, 
                                    UINT32  nBlks,
                                    UINT32  nLockTypeCMD,
                                    UINT32 *pnErrPbn);

PRIVATE INT32 _LockOTP             (UINT32       nDev,
                                    UINT32       nLockValue);

PRIVATE INT32 _StrictChk           (UINT32       nDev,
                                    UINT32       nPbn,
                                    UINT32       nPgOffset);

PRIVATE VOID  _DumpRegisters       (volatile OneNAND4kReg *pstReg);
PRIVATE VOID  _DumpSpareBuffer     (volatile OneNAND4kReg *pstReg);
PRIVATE VOID  _DumpCmdLog          (VOID);

PRIVATE VOID  _CalcTransferTime    (UINT32       nDev,
                                    UINT16       nSysConf1Reg,
                                    UINT32      *pnReadCycle,
                                    UINT32      *pnWriteCycle);

PRIVATE INT32 _GetUniqueID         (UINT32       nDev,
                                    OneNAND4kCxt  *pstOND4kCxt,
                                    UINT32       nFlag);

#if defined (FSR_LLD_STATISTICS)
PRIVATE VOID  _AddOND4kStatLoad      (UINT32       nDev,
                                    UINT32       nDie,
                                    UINT32       nPbn,
                                    UINT32       nCmdOption);

PRIVATE VOID  _AddOND4kStatPgm       (UINT32       nDev,
                                    UINT32       nDie,
                                    UINT32       nPbn,
                                    UINT32       nPg,
                                    UINT32       nCmdOption);

PRIVATE VOID  _AddOND4kStat          (UINT32       nDev,
                                    UINT32       nDie,
                                    UINT32       nType,
                                    UINT32       nBytes,
                                    UINT32       nCmdOption);

#endif /* #if defined (FSR_LLD_STATISTICS)                                    */

#if defined (FSR_LLD_LOGGING_HISTORY)
PRIVATE VOID  _AddLog              (UINT32       nDev,
                                    UINT32       nDie);
#endif /* #if defined (FSR_LLD_LOGGING_HISTORY)                               */


#if defined (FSR_LLD_HANDSHAKE_ERR_INF)
PRIVATE VOID  _SaveErrorCxt        (UINT32       nDev,
                                    UINT32       nDie,
                                    OneNAND4kCxt  *pstOND4kCxt,
                                    INT32        nLLDRe);

PRIVATE INT32 _CheckErrorCxt       (UINT32       nDev,
                                    UINT32       nDie,
                                    OneNAND4kCxt  *pstOND4kCxt,
                           volatile OneNAND4kReg *pstFOReg);

#endif /* #if defined (FSR_LLD_HANDSHAKE_ERR_INF)                             */

/******************************************************************************/
/* Code Implementation                                                        */
/******************************************************************************/

/**
 * @brief           This function initializes 4K OneNAND Device Driver.
 *
 * @param[in]       nFlag   :
 *
 * @return          FSR_LLD_SUCCESS
 * @return          FSR_LLD_ALREADY_INITIALIZED
 *
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark          it initialize internal data structure
 *
 */
PUBLIC INT32
FSR_OND_4K_Init(UINT32 nFlag)
{
            FsrVolParm       astPAParm[FSR_MAX_VOLS];
            FSRLowFuncTbl    astLFT[FSR_MAX_VOLS];
            FSRLowFuncTbl   *apstLFT[FSR_MAX_VOLS];
            OneNAND4kShMem    *pstOND4kShMem;

    PRIVATE BOOL32           nInitFlg = FALSE32;
            UINT32           nVol;
            UINT32           nPDev;
            UINT32           nIdx;
            UINT32           nDie;
            UINT32           nMemAllocType;
            UINT32           nMemoryChunkID;
            UINT32           nSharedMemoryUseCnt;

            INT32            nLLDRe   = FSR_LLD_SUCCESS;
            INT32            nPAMRe   = FSR_PAM_SUCCESS;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:IN ] ++%s(nFlag:0x%08x)\r\n"),
        __FSR_FUNC__, nFlag));

    do
    {
        if (nInitFlg == TRUE32)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
                (TEXT("[OND:INF]   %s(nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
                (TEXT("            already initialized\r\n")));

            nLLDRe = FSR_LLD_ALREADY_INITIALIZED;
            break;
        }

        /* local data structure */
        for (nPDev = 0; nPDev < FSR_OND_4K_MAX_DEVS; nPDev++)
        {
            gpstOND4kCxt[nPDev] = NULL;
        }


        nPAMRe  = FSR_PAM_GetPAParm(astPAParm);
        if (nPAMRe != FSR_PAM_SUCCESS)
        {
            nLLDRe = FSR_LLD_PAM_ACCESS_ERROR;
            break;
        }

        for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
        {
            FSR_OAM_MEMSET(&astLFT[nVol], 0x00, sizeof(FSRLowFuncTbl));
            apstLFT[nVol] = &astLFT[nVol];
        }

        nPAMRe = FSR_PAM_RegLFT(apstLFT);
        if (nPAMRe != FSR_PAM_SUCCESS)
        {
            nLLDRe = FSR_LLD_PAM_ACCESS_ERROR;
            break;
        }

        for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
        {
            /* because BML calls LLD_Init() by volume,
             * FSR_OND_4K_Init() only initializes shared memory of 
             * corresponding volume
             */
            if ((apstLFT[nVol] == NULL) || apstLFT[nVol]->LLD_Init != FSR_OND_4K_Init)
            {
                continue;
            }

            if (astPAParm[nVol].bProcessorSynchronization == TRUE32)
            {
                nMemAllocType = FSR_OAM_SHARED_MEM;
            }
            else
            {
                nMemAllocType = FSR_OAM_LOCAL_MEM;
            }


            nMemoryChunkID = astPAParm[nVol].nMemoryChunkID;

            for (nIdx = 0; nIdx < astPAParm[nVol].nDevsInVol; nIdx++)
            {
                nPDev = nVol * (FSR_MAX_DEVS / FSR_MAX_VOLS) + nIdx;

                pstOND4kShMem = NULL;

                pstOND4kShMem = (OneNAND4kShMem *) FSR_OAM_MallocExt(nMemoryChunkID,
                                                                 sizeof(OneNAND4kShMem),
                                                                 nMemAllocType);

                if (pstOND4kShMem == NULL)
                {
                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                        (TEXT("[OND:ERR]   %s(nFlag:0x%08x) / %d line\r\n"),
                        __FSR_FUNC__, nFlag, __LINE__));

                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                        (TEXT("            pstOND4kShMem is NULL\r\n")));

                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                        (TEXT("            malloc failed!\r\n")));

                    nLLDRe = FSR_LLD_MALLOC_FAIL;
                    break;
                }

                if (((UINT32) pstOND4kShMem & (0x03)) != 0)
                {
                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                        (TEXT("[OND:ERR]   %s(nFlag:0x%08x) / %d line\r\n"),
                        __FSR_FUNC__, nFlag, __LINE__));

                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                        (TEXT("            pstOND4kShMem is misaligned:0x%08x\r\n"),
                        pstOND4kShMem));

                    nLLDRe = FSR_OAM_NOT_ALIGNED_MEMPTR;
                    break;
                }

                gpstOND4kShMem[nPDev] = pstOND4kShMem;

                nSharedMemoryUseCnt = pstOND4kShMem->nShMemUseCnt;

                /* PreOp init of single process */
                if (astPAParm[nVol].bProcessorSynchronization == FALSE32)
                {
                   /* initialize shared memory used by LLD                       */
                    for (nDie = 0; nDie < FSR_MAX_DIES; nDie++)
                    {
                        pstOND4kShMem->nPreOp[nDie]          = FSR_OND_4K_PREOP_NONE;
                        pstOND4kShMem->nPreOpPbn[nDie]       = FSR_OND_4K_PREOP_ADDRESS_NONE;
                        pstOND4kShMem->nPreOpPgOffset[nDie]  = FSR_OND_4K_PREOP_ADDRESS_NONE;
                        pstOND4kShMem->nPreOpFlag[nDie]      = FSR_OND_4K_PREOP_FLAG_NONE;
                    }
                }
                /* PreOp init of dual process */
                else
                {
                    if ((nSharedMemoryUseCnt ==0) ||
                        (nSharedMemoryUseCnt == astPAParm[nVol].nSharedMemoryInitCycle))
                    {
                        pstOND4kShMem->nShMemUseCnt = 0;
                       /* initialize shared memory used by LLD                       */
                        for (nDie = 0; nDie < FSR_MAX_DIES; nDie++)
                        {
                            pstOND4kShMem->nPreOp[nDie]          = FSR_OND_4K_PREOP_NONE;
                            pstOND4kShMem->nPreOpPbn[nDie]       = FSR_OND_4K_PREOP_ADDRESS_NONE;
                            pstOND4kShMem->nPreOpPgOffset[nDie]  = FSR_OND_4K_PREOP_ADDRESS_NONE;
                            pstOND4kShMem->nPreOpFlag[nDie]      = FSR_OND_4K_PREOP_FLAG_NONE;
                        }
                    }
                }

                pstOND4kShMem->nShMemUseCnt++;
            } /* for (nIdx = 0; nIdx < astPAParm[nVol].nDevsInVol; nIdx++) */

            if (nLLDRe != FSR_LLD_SUCCESS)
            {
                break;
            }
        } /* for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++) */

        if (nLLDRe != FSR_LLD_SUCCESS)
        {
            break;
        }

        nInitFlg = TRUE32;

        gpfReadOptimal = NULL;
        /* initialize controller */
        FSR_PAM_InitNANDController();

    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return (nLLDRe);
}



/**
 * @brief           this function opens 4K OneNAND device driver 
 *
 * @param[in]       nDev    : Physical Device Number (0 ~ 3)
 * @param[in]       pParam  : pointer to structure for configuration
 * @param[in]       nFlag   :
 *
 * @return          FSR_LLD_SUCCESS
 * @return          FSR_LLD_OPEN_FAILURE
 * @return          FSR_LLD_INVALID_PARAM
 * @return          FSR_LLD_MALLOC_FAIL
 * @return          FSR_LLD_ALREADY_OPEN
 * @return          FSR_OAM_NOT_ALIGNED_MEMPTR
 * @return          FSR_LLD_PREV_READ_ERROR  | {FSR_LLD_1STPLN_CURR_ERROR }
 *
 * @author          NamOh Hwang / Kyungho Shin
 * @version         1.1.0
 * @remark
 *
 */
PUBLIC INT32
FSR_OND_4K_Open(UINT32  nDev,
                VOID   *pParam,
                UINT32  nFlag)
{
             OneNAND4kCxt        *pstOND4kCxt;
             OneNAND4kSpec       *pstOND4kSpec;
    volatile OneNAND4kReg    *pstFOReg;

             FsrVolParm        *pstParm    = (FsrVolParm *) pParam;

             UINT32             nCnt               = 0;
             UINT32             nIdx               = 0;
             UINT32             nRdTransferTime = 0;
             UINT32             nWrTransferTime = 0;
             UINT32             nBytesPerPage   = 0;
             UINT32             nSpareBytesPerPage = 0;
             UINT32             nMemoryChunkID;

#if !defined(FSR_OAM_RTLMSG_DISABLE)
#if defined(FSR_LLD_HANDSHAKE_ERR_INF)
             UINT32             bHandshakeErrInfo = TRUE32;
#else
             UINT32             bHandshakeErrInfo = FALSE32;
#endif
#if defined(TINY_FSR)
             BOOL32             bTinyFSR = TRUE32;
#else
             BOOL32             bTinyFSR = FALSE32;
#endif
#endif
             INT32              nLLDRe     = FSR_LLD_SUCCESS;

             UINT16             nMID; /* manufacture ID */
             UINT16             nDID; /* device ID      */
             UINT16             nVID; /* version ID     */
             UINT16             nShifted;

    FSR_STACK_VAR;

    FSR_STACK_END;



    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:IN ] ++%s(nDev:%d,nFlag:0x%x)\r\n"), 
        __FSR_FUNC__, nDev, nFlag));

    do
    {
#if defined (FSR_LLD_STRICT_CHK)
        /* check device number */
        if (nDev >= FSR_OND_4K_MAX_DEVS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s(nDev:%d, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Invalid Device Number (nDev = %d)\r\n"),
                nDev));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */

        if (pParam == NULL)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s(nDev:%d, pParam:0x%08x, nFlag:%d) / %d line\r\n"),
                __FSR_FUNC__, nDev, pParam, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            pParam is NULL\r\n")));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }

        pstOND4kCxt = gpstOND4kCxt[nDev];

        if (pstOND4kCxt == NULL)
        {
            nMemoryChunkID = nDev / (FSR_MAX_DEVS / FSR_MAX_VOLS);

            pstOND4kCxt = (OneNAND4kCxt *)
                FSR_OAM_MallocExt(nMemoryChunkID, sizeof(OneNAND4kCxt), FSR_OAM_LOCAL_MEM);

            if (pstOND4kCxt == NULL)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[OND:ERR]   %s(nDev:%d, pParam:0x%08x, nFlag:%d) / %d line\r\n"),
                    __FSR_FUNC__, nDev, pParam, nFlag, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            pstOND4kCxt is NULL\r\n")));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            malloc failed!\r\n")));

                nLLDRe = FSR_LLD_MALLOC_FAIL;
                break;
            }

            if (((UINT32) pstOND4kCxt & (0x03)) != 0)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[OND:ERR]   %s(nDev:%d, nFlag:0x%08x) / %d line\r\n"),
                    __FSR_FUNC__, nDev, nFlag, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            pstOND4kCxt is misaligned:0x%08x\r\n"),
                    pstOND4kCxt));

                nLLDRe = FSR_OAM_NOT_ALIGNED_MEMPTR;
                break;
            }

            gpstOND4kCxt[nDev] = pstOND4kCxt;

            FSR_OAM_MEMSET(pstOND4kCxt, 0x00, sizeof(OneNAND4kCxt));

        } /* if (pstOND4kCxt == NULL) */

        if (pstOND4kCxt->bOpen == TRUE32)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
                (TEXT("[OND:INF]   %s(nDev:%d, pParam:0x%08x, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, pParam, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
                (TEXT("[OND:INF]   dev:%d is already open\r\n"), nDev));

            nLLDRe = FSR_LLD_ALREADY_OPEN;
            break;
        }


        /* base address setting */
        nIdx = nDev & (FSR_MAX_DEVS / FSR_MAX_VOLS -1);

        if (pstParm->nBaseAddr[nIdx] != FSR_PAM_NOT_MAPPED)
        {
            pstOND4kCxt->nBaseAddr = pstParm->nBaseAddr[nIdx];

            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
                (TEXT("[OND:INF]   %s(nDev:%d, pParam:0x%08x, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, pParam, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
                (TEXT("            pstOND4kCxt->nBaseAddr: 0x%08x\r\n"),
                pstOND4kCxt->nBaseAddr));
        }
        else
        {
            pstOND4kCxt->nBaseAddr = FSR_PAM_NOT_MAPPED;
        }

        pstFOReg = (volatile OneNAND4kReg *) pstOND4kCxt->nBaseAddr;

        nMID = OND_4K_READ(pstFOReg->nMID);
        nDID = OND_4K_READ(pstFOReg->nDID);
        nVID = OND_4K_READ(pstFOReg->nVerID);

        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
            (TEXT("            nDev=%d, nMID=0x%04x, nDID=0x%04x, nVID=0x%04x\r\n"),
            nDev, nMID, nDID, nVID));


        /* find corresponding OneNAND4kSpec from gstOND4kSpec[]
         * by looking at bits 3~15. ignore bits 0~2
         */
        pstOND4kCxt->pstOND4kSpec = NULL;
        for (nCnt = 0; gstOND4kSpec[nCnt].nMID != 0; nCnt++)
        {
            if (((nDID & FSR_OND_4K_DID_MASK) == (gstOND4kSpec[nCnt].nDID & FSR_OND_4K_DID_MASK)) &&
                (nMID == gstOND4kSpec[nCnt].nMID))
            {
                pstOND4kCxt->pstOND4kSpec = (OneNAND4kSpec *) &gstOND4kSpec[nCnt];
                break;
            }
            /* else continue */
        }

        if (pstOND4kCxt->pstOND4kSpec == NULL)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s(nDev:%d, pParam:0x%08x, nFlag:0x%08x / %d line\r\n"),
                __FSR_FUNC__, nDev, pParam, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Unknown Device\r\n")));

            nLLDRe = FSR_LLD_OPEN_FAILURE;
            break;
        }

        pstOND4kSpec = pstOND4kCxt->pstOND4kSpec;

        pstOND4kCxt->nFBAMask   = pstOND4kSpec->nNumOfBlks / pstOND4kSpec->nNumOfDies -1;
        pstOND4kCxt->nFPASelSft = 2;/* offset of FPA in Start Address8 register */

        /* calculate nDDPSelSft */
        pstOND4kCxt->nDDPSelSft = 0;
        nShifted = pstOND4kCxt->nFBAMask << 1;
        while((nShifted & FSR_OND_4K_DFS_MASK) != FSR_OND_4K_DFS_MASK)
        {
            pstOND4kCxt->nDDPSelSft++;
            nShifted <<= 1;
        }

        /* HOT-RESET initializes the value of register file, which includes 
         * system configuration 1 register.
         * for device to function properly, save the value of system
         * configuration 1 register at open time & restore it after HOT-RESET
         */
        pstOND4kCxt->nSysConf1    =  OND_4K_READ(pstFOReg->nSysConf1);

        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
            (TEXT("[OND:INF]   pstOND4kCxt->nSysConf1:0x%04x / %d line\r\n"),
            pstOND4kCxt->nSysConf1, __LINE__));



        pstOND4kCxt->nFlushOpCaller = FSR_OND_4K_PREOP_NONE;



        /* previous operation information */
        for (nCnt = 0; nCnt < FSR_MAX_DIES; nCnt++)
        {
            pstOND4kCxt->bIsPreCmdCache[nCnt]  = FALSE32;
        }

        /* read PI Allocation Info */
        for (nCnt = 0; nCnt < pstOND4kSpec->nNumOfDies; nCnt++)
        {
            pstOND4kCxt->nBlksForSLCArea[nCnt] = pstOND4kSpec->nNumOfBlks / pstOND4kSpec->nNumOfDies;

            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
                (TEXT("[OND:INF]   pstOND4kCxt->nBlksForSLCArea[%d]:%d / %d line\r\n"),
                nCnt, pstOND4kCxt->nBlksForSLCArea[nCnt], __LINE__));
        }

        nSpareBytesPerPage = pstOND4kSpec->nSctsPerPG * pstOND4kSpec->nSparePerSct;
        nMemoryChunkID     = nDev / (FSR_MAX_DEVS / FSR_MAX_VOLS);
        pstOND4kCxt->pSpareBuffer =
            (UINT8 *) FSR_OAM_MallocExt(nMemoryChunkID, nSpareBytesPerPage, FSR_OAM_LOCAL_MEM);

        if (pstOND4kCxt->pSpareBuffer == NULL)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s(nDev:%d, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            malloc failed!\r\n")));

            nLLDRe = FSR_LLD_MALLOC_FAIL;
            break;
        }

        if (((UINT32) pstOND4kCxt->pSpareBuffer & (0x03)) != 0)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s(nDev:%d, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            pstOND4kCxt->pSpareBuffer is misaligned:0x%08x\r\n"),
                pstOND4kCxt->pSpareBuffer));

            nLLDRe = FSR_OAM_NOT_ALIGNED_MEMPTR;
            break;
        }

        FSR_OAM_MEMSET(pstOND4kCxt->pSpareBuffer, 0xFF, nSpareBytesPerPage);



        /* nBytesPerPage is 4224 byte */
        nBytesPerPage = pstOND4kSpec->nSctsPerPG *
            (FSR_OND_4K_SECTOR_SIZE + pstOND4kSpec->nSparePerSct);

        nMemoryChunkID = nDev / (FSR_MAX_DEVS / FSR_MAX_VOLS);

        pstOND4kCxt->pTempBuffer =
            (UINT8 *) FSR_OAM_MallocExt(nMemoryChunkID, nBytesPerPage, FSR_OAM_LOCAL_MEM);

        if (pstOND4kCxt->pTempBuffer == NULL)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s(nDev:%d, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            malloc failed!\r\n")));

            nLLDRe = FSR_LLD_MALLOC_FAIL;
            break;
        }

        if (((UINT32) pstOND4kCxt->pTempBuffer & (0x03)) != 0)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s(nDev:%d, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            pstOND4kCxt->pTempBuffer is misaligned:0x%08x\r\n"),
                pstOND4kCxt->pTempBuffer));

            nLLDRe = FSR_OAM_NOT_ALIGNED_MEMPTR;
            break;
        }

        FSR_OAM_MEMSET(pstOND4kCxt->pTempBuffer, 0x00, nBytesPerPage);

        /* time for transfering 16 bits between host & DataRAM of OneNAND */
        _CalcTransferTime(nDev, pstOND4kCxt->nSysConf1,
                          &nRdTransferTime, &nWrTransferTime);

        pstOND4kCxt->nRdTranferTime  = nRdTransferTime; /* nano second base */
        pstOND4kCxt->nWrTranferTime  = nWrTransferTime; /* nano second base */



        nIdx                    = nDev & (FSR_MAX_DEVS / FSR_MAX_VOLS -1);
        pstOND4kCxt->nIntID       = pstParm->nIntID[nIdx];
        if ((pstOND4kCxt->nIntID != FSR_INT_ID_NONE) && (pstOND4kCxt->nIntID > FSR_INT_ID_NAND_7))
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:INF]   nIntID is out of range(nIntID:%d)\r\n"), pstOND4kCxt->nIntID));
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:INF]   FSR_INT_ID_NAND_0(%d) <= nIntID <= FSR_INT_ID_NAND_7(%d)\r\n"), 
                FSR_INT_ID_NAND_0, FSR_INT_ID_NAND_7));
            break;
        }

        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
            (TEXT("[OND:INF]   pstOND4kCxt->nIntID   :0x%04x / %d line\r\n"),
            pstOND4kCxt->nIntID, __LINE__));



        if (pstOND4kCxt->nSysConf1 & FSR_OND_4K_CONF1_SYNC_READ)
        {
            gpfReadOptimal            = _ReadOptWithSLoad;
        }
        else
        {
            gpfReadOptimal            = _ReadOptWithNonSLoad;

            /* superload does not work in Ayncronous mode.
             * so, treat FSR_LLD_FLAG_1X_PLOAD as normal load in Async mode.
             */
            gnLoadCmdArray[FSR_LLD_FLAG_1X_PLOAD] = 0x0000;
        }

        FSR_ASSERT(gpfReadOptimal != NULL);

        /* device of 3.3V does not support cache program */
        if ((nDID & FSR_OND_4K_DID_VCC_MASK) == FSR_OND_4K_DID_VCC_33V)
        {
            pstOND4kCxt->bCachePgm                    = FALSE32;
            gnPgmCmdArray[FSR_LLD_FLAG_1X_CACHEPGM] = 0x0080;
        }
        else
        {
            pstOND4kCxt->bCachePgm                    = TRUE32;
        }

#if !defined(FSR_LLD_USE_CACHE_PGM)
        pstOND4kCxt->bCachePgm                    = FALSE32;
        gnPgmCmdArray[FSR_LLD_FLAG_1X_CACHEPGM] = 0x0080;
#endif

#if !defined(FSR_LLD_USE_SUPER_LOAD)
        gpfReadOptimal                          = _ReadOptWithNonSLoad;
        gnLoadCmdArray[FSR_LLD_FLAG_1X_PLOAD]   = 0x0000;
#endif

#if defined (FSR_LLD_STATISTICS)
        gnDevsInVol[nDev / (FSR_MAX_DEVS / FSR_MAX_VOLS)]++;
#endif

        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_LOG,
            (TEXT("[OND:INF]   1X_CACHEPGM CMD : 0x%04x\r\n"), 
            gnPgmCmdArray[FSR_LLD_FLAG_1X_CACHEPGM]));

        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_LOG,
            (TEXT("[OND:INF]   1X_PLOAD    CMD : 0x%04x\r\n"), 
            gnLoadCmdArray[FSR_LLD_FLAG_1X_PLOAD]));

        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_LOG,
            (TEXT("[OND:INF]   %s / bHandshakeErrInfo=%d\r\n"), 
            (bTinyFSR == TRUE32) ? TEXT("TINY_FSR") : TEXT("FSR"), bHandshakeErrInfo));

#if defined(FSR_LLD_ENABLE_DEBUG_PORT)
        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_LOG,
            (TEXT("[OND:INF]   debug port addr : byte order:0x%x / word order:0x%x\r\n"),
            (UINT32) (&pstFOReg->nDebugPort), (UINT32) (&pstFOReg->nDebugPort) / 2));

        OND_4K_WRITE(pstFOReg->nDebugPort, 0x4321);
#endif

        pstOND4kCxt->bOpen      = TRUE32;

    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return (nLLDRe);
}



/**
 * @brief           This function closes 4K OneNAND device driver
 *
 * @param[in]       nDev        : Physical Device Number (0 ~ 3)
 * @param[in]       nFlag       :
 *
 * @return          FSR_LLD_SUCCESS
 * @return          FSR_LLD_INVALID_PARAM
 *
 * @author          NamOh Hwang / Kyungho Shin
 * @version         1.1.0
 * @remark
 *
 */
PUBLIC INT32
FSR_OND_4K_Close(UINT32 nDev,
                 UINT32 nFlag)
{
    OneNAND4kCxt       *pstOND4kCxt;

#if defined (FSR_LLD_STATISTICS)
    UINT32            nVol;
#endif
    UINT32            nMemoryChunkID;


    INT32             nLLDRe     = FSR_LLD_SUCCESS;

    FSR_STACK_VAR;

    FSR_STACK_END;



    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:IN ] ++%s(nDev:%d, nFlag:0x%08x)\r\n"),
        __FSR_FUNC__, nDev, nFlag));

    /* here LLD doesn't flush the previous operation, for BML flushes */
    do
    {

#if defined (FSR_LLD_STRICT_CHK)
        /* check device number */
        if (nDev >= FSR_OND_4K_MAX_DEVS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s(nDev:%d, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Invalid Device Number (nDev = %d)\r\n"),
                nDev));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */

        pstOND4kCxt = gpstOND4kCxt[nDev];

        if (pstOND4kCxt != NULL)
        {
            pstOND4kCxt->bOpen      = FALSE32;
            pstOND4kCxt->pstOND4kSpec = NULL;

            nMemoryChunkID = nDev / (FSR_MAX_DEVS / FSR_MAX_VOLS);

            FSR_OAM_FreeExt(nMemoryChunkID, pstOND4kCxt->pTempBuffer, FSR_OAM_LOCAL_MEM);
            FSR_OAM_FreeExt(nMemoryChunkID, pstOND4kCxt->pSpareBuffer, FSR_OAM_LOCAL_MEM);
            FSR_OAM_FreeExt(nMemoryChunkID, pstOND4kCxt, FSR_OAM_LOCAL_MEM);
            gpstOND4kCxt[nDev] = NULL;

#if defined (FSR_LLD_STATISTICS)
            nVol = nDev / (FSR_MAX_DEVS / FSR_MAX_VOLS);
            gnDevsInVol[nVol]--;
#endif
        }
    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return (nLLDRe);
}



/**
 * @brief           This function reads 1 page from 4K OneNAND
 *
 * @param[in]       nDev        : Physical Device Number (0 ~ 3)
 * @param[in]       nPbn        : Physical Block  Number
 * @param[in]       nPgOffset   : Page Offset within a block
 * @param[out]      pMBuf       : Memory buffer for main  array of NAND flash
 * @param[out]      pSBuf       : Memory buffer for spare array of NAND flash
 * @param[in]       nFlag       : Operation options such as ECC_ON, OFF
 *
 * @return          FSR_LLD_SUCCESS
 * @return          FSR_LLD_INVALID_PARAM
 * @return          FSR_LLD_PREV_READ_ERROR       | {FSR_LLD_1STPLN_CURR_ERROR}
 * @n               previous read error return value
 * @return          FSR_LLD_PREV_READ_DISTURBANCE | {FSR_LLD_1STPLN_CURR_ERROR}
 * @n               previous read disturbance error return value
 *
 * @author          NamOh Hwang / Kyungho Shin
 * @version         1.1.0
 * @remark          one function call of FSR_OND_4K_Read() loads 1 page from
 * @n               4K OneNAND and moves data from DataRAM to pMBuf, pSBuf.
 * @n               In DDP environment, 
 * @n               this version of read does not make use of load time.
 * @n               ( which is busy wait time, let's say it be 45us )
 * @n               instead, it waits till DataRAM gets filled.
 * @n               as a result, it does not give the best performance.
 * @n               To make use of die interleaving, use FSR_OND_4K_ReadOptimal()
 *
 */
PUBLIC INT32
FSR_OND_4K_Read(UINT32         nDev,
                UINT32         nPbn,
                UINT32         nPgOffset,
                UINT8         *pMBuf,
                FSRSpareBuf   *pSBuf,
                UINT32         nFlag)
{
    INT32       nLLDRe = FSR_LLD_SUCCESS;
    UINT32      nLLDFlag;

    FSR_STACK_VAR;

    FSR_STACK_END;



    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:IN ] ++%s()\r\n"), __FSR_FUNC__));

    do
    {
        nLLDFlag = ~FSR_LLD_FLAG_CMDIDX_MASK & nFlag;

        nLLDRe = FSR_OND_4K_ReadOptimal(nDev,
                                     nPbn, nPgOffset,
                                     pMBuf, pSBuf,
                                     FSR_LLD_FLAG_1X_LOAD | nLLDFlag);

        if (FSR_RETURN_MAJOR(nLLDRe) != FSR_LLD_SUCCESS)
        {
            break;
        }

        nLLDRe = FSR_OND_4K_ReadOptimal(nDev,
                                     nPbn, nPgOffset,
                                     pMBuf, pSBuf,
                                     FSR_LLD_FLAG_TRANSFER | nLLDFlag);

        if (FSR_RETURN_MAJOR(nLLDRe) != FSR_LLD_SUCCESS)
        {
            break;
        }
    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return nLLDRe;
}



/**
 * @brief           This function reads 1 page from 4K OneNAND
 *
 * @param[in]       nDev        : Physical Device Number (0 ~ )
 * @param[in]       nPbn        : Physical Block  Number
 * @param[in]       nPgOffset   : Page Offset within a block
 * @param[out]      pMBuf       : Memory buffer for main  array of NAND flash
 * @param[out]      pSBuf       : Memory buffer for spare array of NAND flash
 * @param[in]       nFlag       : Operation options such as ECC_ON, OFF
 *
 * @return          FSR_LLD_SUCCESS
 * @return          FSR_LLD_INVALID_PARAM
 * @return          FSR_LLD_PREV_READ_ERROR       | {FSR_LLD_1STPLN_CURR_ERROR}
 * @n               previous read error return value
 * @return          FSR_LLD_PREV_READ_DISTURBANCE | {FSR_LLD_1STPLN_CURR_ERROR}
 * @n               previous read disturbance error return value
 *
 * @author          NamOh Hwang / Kyungho Shin
 * @version         1.1.0
 * @remark          LLD_Open() links gpfReadOptimal to _ReadOptimalWithSLoad() or
 * @n               _ReadOptimalWithNonSLoad(), which depends on whether target
 * @n               supports syncronous read or not.
 * @n               because, superload needs data transferred in sync mode.
 * @n               _ReadOptimalWithSLoad() does not work in Async mode.
 *
 */
PUBLIC INT32
FSR_OND_4K_ReadOptimal(UINT32         nDev,
                       UINT32         nPbn,
                       UINT32         nPgOffset,
                       UINT8         *pMBuf,
                       FSRSpareBuf   *pSBuf,
                       UINT32         nFlag)
{
    INT32 nLLDRe = FSR_LLD_SUCCESS;

    FSR_STACK_VAR;

    FSR_STACK_END;


    do
    {
#if defined (FSR_LLD_STRICT_CHK)
        nLLDRe = _StrictChk(nDev, nPbn, nPgOffset);

        if (nLLDRe != FSR_LLD_SUCCESS)
        {
            break;
        }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */

        nLLDRe = gpfReadOptimal(nDev, nPbn, nPgOffset, pMBuf, pSBuf, nFlag);

    } while (0);

    /* FSR_OND_4K_Open() assigns appropriate function pointer to gpfReadOptimal.
     * it depends on the Read Mode (whether it supports syncronous read or not).
     */
    return nLLDRe;
}



/**
 * @brief           this function reads data from 4K OneNAND
 *
 * @param[in]       nDev        : Physical Device Number (0 ~ 3)
 * @param[in]       nPbn        : Physical Block  Number
 * @param[in]       nPgOffset   : Page Offset within a block
 * @param[out]      pMBuf       : Memory buffer for main  array of NAND flash
 * @param[out]      pSBuf       : Memory buffer for spare array of NAND flash
 * @param[in]       nFlag       : Operation options such as ECC_ON, OFF
 *
 * @return          FSR_LLD_SUCCESS
 * @return          FSR_LLD_PREV_READ_ERROR       | {FSR_LLD_1STPLN_CURR_ERROR}
 * @n               previous read error return value
 * @return          FSR_LLD_PREV_READ_DISTURBANCE | {FSR_LLD_1STPLN_CURR_ERROR}
 * @n               previous read disturbance error return value
 *
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark          use of PLOAD flag issue superload command.
 *
 */
PRIVATE INT32
_ReadOptWithSLoad(UINT32       nDev,
                  UINT32       nPbn,
                  UINT32       nPgOffset,
                  UINT8       *pMBuf,
                  FSRSpareBuf *pSBuf,
                  UINT32       nFlag)
{
             OneNAND4kCxt       *pstOND4kCxt  = gpstOND4kCxt[nDev];
             OneNAND4kSpec      *pstOND4kSpec = pstOND4kCxt->pstOND4kSpec;
             OneNAND4kShMem     *pstOND4kShMem = gpstOND4kShMem[nDev];
    volatile OneNAND4kReg   *pstFOReg   = (volatile OneNAND4kReg *) pstOND4kCxt->nBaseAddr;

             UINT32            nCmdIdx;

             /* start sector offset from the start */
             UINT32            nStartOffset;
             /* end sector offset from the end */
             UINT32            nEndOffset;
             UINT32            nDie;
             UINT32            nFlushOpCaller;
#if defined (FSR_LLD_STATISTICS)
             UINT32            nBytesTransferred = 0;
#endif /* #if defined (FSR_LLD_STATISTICS) */

             INT32             nLLDRe     = FSR_LLD_SUCCESS;
             INT32             nSpareRe   = FSR_LLD_SUCCESS;

             UINT16            nSysConf1;


    FSR_STACK_VAR;

    FSR_STACK_END;



    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
    (TEXT("[OND:IN ] ++%s(nDev:%d, nPbn:%d, nPgOffset:%d, pMBuf:0x%08x, pSBuf:0x%08x, nFlag:0x%08x)\r\n"),
    __FSR_FUNC__, nDev, nPbn, nPgOffset, pMBuf, pSBuf, nFlag));

    do
    {
        nCmdIdx = (nFlag & FSR_LLD_FLAG_CMDIDX_MASK) >> FSR_LLD_FLAG_CMDIDX_BASEBIT;

        nDie    = nPbn >> (FSR_OND_4K_DFS_BASEBIT - pstOND4kCxt->nDDPSelSft);

        nFlushOpCaller = FSR_OND_4K_PREOP_READ << FSR_OND_4K_FLUSHOP_CALLER_BASEBIT;

        if ((nCmdIdx == FSR_LLD_FLAG_1X_LOAD) ||
            (nCmdIdx == FSR_LLD_FLAG_1X_PLOAD))
        {

            /* 4K OneNAND doesn't support read interleaving,
               therefore, wait until interrupt status is ready for both die.
               if device is DDP. */
            if (pstOND4kSpec->nNumOfDies == FSR_MAX_DIES)
            {
                FSR_OND_4K_FlushOp(nDev,
                                nFlushOpCaller | ((nDie + 0x1) & 0x01),
                                nFlag | FSR_LLD_FLAG_REMAIN_PREOP_STAT);
            }

            nLLDRe = FSR_OND_4K_FlushOp(nDev, nFlushOpCaller | nDie, nFlag);

            /* set DBS */
            OND_4K_WRITE(pstFOReg->nStartAddr2, (UINT16) (nDie << FSR_OND_4K_DBS_BASEBIT));

            /* set DFS & FBA */
            OND_4K_WRITE(pstFOReg->nStartAddr1,
                (UINT16) ((nDie << FSR_OND_4K_DFS_BASEBIT) |
                          (nPbn & pstOND4kCxt->nFBAMask)));

            /* set FPA (Flash Page Address) */
            OND_4K_WRITE(pstFOReg->nStartAddr8,
                (UINT16) (nPgOffset << pstOND4kCxt->nFPASelSft));

            /* set Start Buffer Register */
            OND_4K_WRITE(pstFOReg->nStartBuf, FSR_OND_4K_START_BUF_DEFAULT);


            if ((nFlag & FSR_LLD_FLAG_ECC_MASK) == FSR_LLD_FLAG_ECC_ON)
            {
                nSysConf1 = OND_4K_READ(pstFOReg->nSysConf1);
                /* if ECC is disabled, enable */
                if (nSysConf1 & FSR_OND_4K_CONF1_ECC_OFF)
                {
                    OND_4K_CLR(pstFOReg->nSysConf1, FSR_OND_4K_CONF1_ECC_ON);
                }
            }
            else
            {
                OND_4K_SET(pstFOReg->nSysConf1, FSR_OND_4K_CONF1_ECC_OFF);
            }

            /* in case of non-blocking mode, interrupt should be enabled      */
            if (nFlag & FSR_LLD_FLAG_INT_MASK)
            {
                FSR_OAM_ClrNEnableInt(pstOND4kCxt->nIntID);
            }
            else
            {
                FSR_OAM_ClrNDisableInt(pstOND4kCxt->nIntID);
            }

            OND_4K_WRITE(pstFOReg->nCmd, (UINT16) gnLoadCmdArray[nCmdIdx]);

#if defined (FSR_LLD_STATISTICS)

            if (gnLoadCmdArray[nCmdIdx] == FSR_OND_4K_CMD_SUPERLOAD)
            {
                _AddOND4kStatLoad(nDev,
                                nDie,
                                nPbn,
                                FSR_OND_4K_STAT_PLOAD);
            }
            else
            {
                _AddOND4kStatLoad(nDev,
                                nDie,
                                nPbn,
                                FSR_OND_4K_STAT_NORMAL_CMD);
            }

#endif /* #if defined (FSR_LLD_STATISTICS) */
        } /* if ((nCmdIdx == FSR_LLD_FLAG_1X_LOAD) | (nCmdIdx == FSR_LLD_FLAG_1X_PLOAD)) */

        /* transfer data */
        if (nFlag & FSR_LLD_FLAG_TRANSFER)
        {
            if (nCmdIdx == FSR_LLD_FLAG_NO_LOADCMD)
            {
                /* 4K OneNAND doesn't support read interleaving,
                   therefore, wait until interrupt status is ready for both die. 
                   if device is DDP. */
                if (pstOND4kSpec->nNumOfDies == FSR_MAX_DIES)
                {
                    FSR_OND_4K_FlushOp(nDev,
                                    nFlushOpCaller | ((nDie + 0x1) & 0x1),
                                    nFlag | FSR_LLD_FLAG_REMAIN_PREOP_STAT);
                }
                nLLDRe = FSR_OND_4K_FlushOp(nDev, nFlushOpCaller | nDie, nFlag);

                /* set DBS */
                OND_4K_WRITE(pstFOReg->nStartAddr2,
                    (UINT16) (nDie << FSR_OND_4K_DBS_BASEBIT));
            }


            if (pMBuf != NULL)
            {
                /* _ReadMain() can load continuous sectors within a page. */
                nStartOffset = (nFlag & FSR_LLD_FLAG_1ST_SCTOFFSET_MASK)
                                >> FSR_LLD_FLAG_1ST_SCTOFFSET_BASEBIT;

                nEndOffset   = (nFlag & FSR_LLD_FLAG_LAST_SCTOFFSET_MASK)
                                >> FSR_LLD_FLAG_LAST_SCTOFFSET_BASEBIT;

                _ReadMain(pMBuf + nStartOffset * FSR_OND_4K_SECTOR_SIZE,
                    &(pstFOReg->nDataMB00[0]) + nStartOffset * FSR_OND_4K_SECTOR_SIZE,
                    (pstOND4kSpec->nSctsPerPG - nStartOffset - nEndOffset) * FSR_OND_4K_SECTOR_SIZE,
                    pstOND4kCxt->pTempBuffer);

#if defined (FSR_LLD_STATISTICS)
                nBytesTransferred +=  FSR_OND_4K_SECTOR_SIZE *
                    (pstOND4kSpec->nSctsPerPG - nStartOffset - nEndOffset);
#endif /* #if defined (FSR_LLD_STATISTICS) */
            }

            if ((pSBuf != NULL) && (nFlag & FSR_LLD_FLAG_USE_SPAREBUF))
            {
                if ((nFlag & FSR_LLD_FLAG_DUMP_MASK) == FSR_LLD_FLAG_DUMP_OFF)
                {
                    nSpareRe = _ReadSpare((FSRSpareBuf *) pSBuf, &(pstFOReg->nDataSB00[0]), nFlag);

                    if (nSpareRe != FSR_LLD_SUCCESS)
                    {
                        if (!((nLLDRe & FSR_LLD_PREV_READ_ERROR) == FSR_LLD_PREV_READ_ERROR))
                        {
                            nLLDRe = nSpareRe;
                        }
                    }
                }
                else
                {
                    /* when dumpping NAND Image,
                     * _ReadMain() reads the whole spare area
                     */
                    _ReadMain((UINT8 *) pSBuf,
                        &(pstFOReg->nDataSB00[0]),
                        pstOND4kSpec->nSctsPerPG * pstOND4kSpec->nSparePerSct,
                        pstOND4kCxt->pTempBuffer);
                }

#if defined (FSR_LLD_STATISTICS)
                if ((nFlag & FSR_LLD_FLAG_DUMP_MASK) == FSR_LLD_FLAG_DUMP_OFF)
                {
                    nBytesTransferred += (FSR_SPARE_BUF_SIZE_4KB_PAGE);
                }
                else
                {
                    nBytesTransferred +=
                        pstOND4kSpec->nSctsPerPG * pstOND4kSpec->nSparePerSct;
                }
#endif /* #if defined (FSR_LLD_STATISTICS) */
            }
            

            /******************CAUTION ***************************************
             *       COMPILER MUST NOT REMOVE THIS CODE                      *
             *****************************************************************
             */
            if (nCmdIdx == FSR_LLD_FLAG_1X_PLOAD)
            {
                /* Read at least 4 bytes to the end of the spare area, 
                 * Or Superload will not work properly.
                 * 2 lines below trigger the load from page buffer to DataRAM
                 * [[ CAUTION ]]
                 * SHOULD NOT be issued at the address of &pstFOReg->nDataSB13[0x0e]
                 */

#if defined(FSR_ONENAND_EMULATOR)
                /* Read at least 4 bytes to the end of the spare area, 
                 * Or Superload will not work properly.
                 * 2 lines below trigger the load from page buffer to DataRAM
                 */
                *(UINT16 *) &(pstOND4kCxt->pTempBuffer[0]) = OND_4K_READ(*(UINT16 *) &pstFOReg->nDataSB13[0xC]);
                *(UINT16 *) &(pstOND4kCxt->pTempBuffer[2]) = OND_4K_READ(*(UINT16 *) &pstFOReg->nDataSB13[0xE]);
#elif defined (FSR_MSM7200)
                *(UINT32 *) &(pstOND4kCxt->pTempBuffer[0]) = OND_4K_READ(*(UINT16 *) &pstFOReg->nDataSB13[0xC]);
#else
                *(UINT32 *) &(pstOND4kCxt->pTempBuffer[0]) = *(UINT32 *) &pstFOReg->nDataSB13[0x0C];
#endif
            }


#if defined (FSR_LLD_STATISTICS)
            if (nCmdIdx == FSR_LLD_FLAG_1X_PLOAD)
            {
                _AddOND4kStat(nDev,
                            nDie,
                            FSR_OND_4K_STAT_RD_TRANS,
                            nBytesTransferred,
                            FSR_OND_4K_STAT_PLOAD);
            }
            else
            {
                _AddOND4kStat(nDev,
                            nDie,
                            FSR_OND_4K_STAT_RD_TRANS,
                            nBytesTransferred,
                            FSR_OND_4K_STAT_NORMAL_CMD);
            }
#endif /* #if defined (FSR_LLD_STATISTICS) */

        } /* if (nFlag & FSR_LLD_FLAG_TRANSFER) */

    } while (0);

    /*
       if transfer only operation, nPreOp[nDie] should be FSR_OND_PREOP_NONE.
       ----------------------------------------------------------------------
       FSR_OND_4K_FlushOp() wait read interrupt if nPreOp is FSR_OND_4K_PREOP_READ.
       But, if this function only has FSR_LLD_FLAG_TRANSFER, there is no read interrupt.
       Therefore, In order to avoid infinite loop, nPreOp is FSR_OND_4K_PREOP_NONE.
       ----------------------------------------------------------------------
    */
    if ((nCmdIdx == FSR_LLD_FLAG_NO_LOADCMD) &&
        (nFlag & FSR_LLD_FLAG_TRANSFER))
    {
        pstOND4kShMem->nPreOp[nDie]         = FSR_OND_4K_PREOP_NONE;
    }
    else
    {
        pstOND4kShMem->nPreOp[nDie]         = FSR_OND_4K_PREOP_READ;
    }

    pstOND4kShMem->nPreOpPbn[nDie]      = (UINT16) nPbn;
    pstOND4kShMem->nPreOpPgOffset[nDie] = (UINT16) nPgOffset;
    pstOND4kShMem->nPreOpFlag[nDie]     = nFlag;

#if defined (FSR_LLD_LOGGING_HISTORY)
    _AddLog(nDev, nDie);
#endif

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));


    return (nLLDRe);
}



/**
 * @brief           This function is for Asyncronous mode Read
 *
 * @param[in]       nDev        : Physical Device Number (0 ~ 3)
 * @param[in]       nPbn        : Physical Block  Number
 * @param[in]       nPgOffset   : Page Offset within a block
 * @param[out]      pMBuf       : Memory buffer for main  array of NAND flash
 * @param[out]      pSBuf       : Memory buffer for spare array of NAND flash
 * @param[in]       nFlag       : Operation options such as ECC_ON, OFF
 *
 * @return          FSR_LLD_SUCCESS
 * @return          FSR_LLD_PREV_READ_ERROR       | {FSR_LLD_1STPLN_CURR_ERROR}
 * @n               previous read error return value
 * @return          FSR_LLD_PREV_READ_DISTURBANCE | {FSR_LLD_1STPLN_CURR_ERROR}
 * @n               previous read disturbance error return value
 *
 * @author          NamOh Hwang / Kyungho Shin
 * @version         1.1.0
 * @remark          This function does not make use of SUPERLOAD command
 * @n               instead it treats PLOAD flag with normal load command
 *
 */
PRIVATE INT32
_ReadOptWithNonSLoad(UINT32        nDev,
                     UINT32        nPbn,
                     UINT32        nPgOffset,
                     UINT8        *pMBuf,
                     FSRSpareBuf  *pSBuf,
                     UINT32        nFlag)
{
             OneNAND4kCxt       *pstOND4kCxt  = gpstOND4kCxt[nDev];
             OneNAND4kSpec      *pstOND4kSpec = pstOND4kCxt->pstOND4kSpec;
             OneNAND4kShMem     *pstOND4kShMem= gpstOND4kShMem[nDev];
    volatile OneNAND4kReg   *pstFOReg   = (volatile OneNAND4kReg *) pstOND4kCxt->nBaseAddr;

             UINT32            nCmdIdx;
             /* start sector offset from the start */
             UINT32            nStartOffset;
             /* end sector offset from the end     */
             UINT32            nEndOffset;
             UINT32            nDie;
             UINT32            nFlushOpCaller;

#if defined (FSR_LLD_STATISTICS)
             UINT32            nBytesTransferred = 0;
#endif /* #if defined (FSR_LLD_STATISTICS) */

             INT32             nLLDRe     = FSR_LLD_SUCCESS;
             INT32             nSpareRe   = FSR_LLD_SUCCESS;
             UINT16            nSysConf1;


    FSR_STACK_VAR;

    FSR_STACK_END;



    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
    (TEXT("[OND:IN ] ++%s(nDev:%d, nPbn:%d, nPgOffset:%d, pMBuf:0x%08x, pSBuf:0x%08x, nFlag:0x%08x)\r\n"),
    __FSR_FUNC__ , nDev, nPbn, nPgOffset, pMBuf, pSBuf, nFlag));

    do
    {
        nCmdIdx = (nFlag & FSR_LLD_FLAG_CMDIDX_MASK) 
                    >> FSR_LLD_FLAG_CMDIDX_BASEBIT;

        nDie    = nPbn >> (FSR_OND_4K_DFS_BASEBIT - pstOND4kCxt->nDDPSelSft);

        nFlushOpCaller = FSR_OND_4K_PREOP_READ << FSR_OND_4K_FLUSHOP_CALLER_BASEBIT;

        /* transfer data */
        if (nFlag & FSR_LLD_FLAG_TRANSFER)
        {
            /* OneNAND doesn't support read interleaving,
               therefore, wait until interrupt status is ready for both die. 
               if device is DDP. */
            if (pstOND4kSpec->nNumOfDies == FSR_MAX_DIES)
            {
                FSR_OND_4K_FlushOp(nDev,
                                nFlushOpCaller | ((nDie + 0x1) & 0x1),
                                nFlag | FSR_LLD_FLAG_REMAIN_PREOP_STAT);
            }
            nLLDRe = FSR_OND_4K_FlushOp(nDev, nFlushOpCaller | nDie, nFlag);


            /* set DBS */
            OND_4K_WRITE(pstFOReg->nStartAddr2, (UINT16) (nDie << FSR_OND_4K_DBS_BASEBIT));

            if (pMBuf != NULL)
            {
                /* _ReadMain() can load continuous sectors within a page.*/
                nStartOffset = (nFlag & FSR_LLD_FLAG_1ST_SCTOFFSET_MASK)
                                >> FSR_LLD_FLAG_1ST_SCTOFFSET_BASEBIT;

                nEndOffset   = (nFlag & FSR_LLD_FLAG_LAST_SCTOFFSET_MASK)
                                >> FSR_LLD_FLAG_LAST_SCTOFFSET_BASEBIT;

                _ReadMain(pMBuf + nStartOffset * FSR_OND_4K_SECTOR_SIZE,
                &(pstFOReg->nDataMB00[0]) + nStartOffset * FSR_OND_4K_SECTOR_SIZE,
                (pstOND4kSpec->nSctsPerPG - nStartOffset - nEndOffset) * FSR_OND_4K_SECTOR_SIZE,
                pstOND4kCxt->pTempBuffer);

#if defined (FSR_LLD_STATISTICS)
                nBytesTransferred +=  FSR_OND_4K_SECTOR_SIZE *
                    (pstOND4kSpec->nSctsPerPG - nStartOffset - nEndOffset);
#endif /* #if defined (FSR_LLD_STATISTICS) */
            }

            if ((pSBuf != NULL) && (nFlag & FSR_LLD_FLAG_USE_SPAREBUF))
            {
                if ((nFlag & FSR_LLD_FLAG_DUMP_MASK) == FSR_LLD_FLAG_DUMP_OFF)
                {
                    nSpareRe = _ReadSpare((FSRSpareBuf *) pSBuf, &(pstFOReg->nDataSB00[0]), nFlag);

                    if (nSpareRe != FSR_LLD_SUCCESS)
                    {
                        if (!((nLLDRe & FSR_LLD_PREV_READ_ERROR) == FSR_LLD_PREV_READ_ERROR))
                        {
                            nLLDRe = nSpareRe;
                        }
                    }
                }
                else
                {
                    /* when dumpping NAND Image,
                     * _ReadMain() reads the whole spare area
                     */
                    _ReadMain((UINT8 *) pSBuf,
                        &(pstFOReg->nDataSB00[0]),
                        pstOND4kSpec->nSctsPerPG * pstOND4kSpec->nSparePerSct,
                        pstOND4kCxt->pTempBuffer);
                }

#if defined (FSR_LLD_STATISTICS)
                if ((nFlag & FSR_LLD_FLAG_DUMP_MASK) == FSR_LLD_FLAG_DUMP_OFF)
                {
                    nBytesTransferred += (FSR_SPARE_BUF_SIZE_4KB_PAGE);
                }
                else
                {
                    nBytesTransferred +=
                        pstOND4kSpec->nSctsPerPG * pstOND4kSpec->nSparePerSct;
                }
#endif /* #if defined (FSR_LLD_STATISTICS) */
            }


#if defined (FSR_LLD_STATISTICS)
            _AddOND4kStat(nDev,
                        nDie,
                        FSR_OND_4K_STAT_RD_TRANS,
                        nBytesTransferred,
                        FSR_OND_4K_STAT_NORMAL_CMD);

#endif /* #if defined (FSR_LLD_STATISTICS) */

            pstOND4kShMem->nPreOp[nDie]         = FSR_OND_4K_PREOP_NONE;

        } /* if (nFlag & FSR_LLD_FLAG_TRANSFER) */


        if ((nCmdIdx == FSR_LLD_FLAG_1X_LOAD) ||
            (nCmdIdx == FSR_LLD_FLAG_1X_PLOAD))
        {
            /* OneNAND doesn't support read interleaving,
               therefore, wait until interrupt status is ready for both die. 
               if device is DDP. */
            if (pstOND4kSpec->nNumOfDies == FSR_MAX_DIES)
            {
                FSR_OND_4K_FlushOp(nDev,
                                nFlushOpCaller | ((nDie + 0x1) & 0x1),
                                nFlag | FSR_LLD_FLAG_REMAIN_PREOP_STAT);
            }
            FSR_OND_4K_FlushOp(nDev, nFlushOpCaller | nDie, nFlag);

            /* set DBS */
            OND_4K_WRITE(pstFOReg->nStartAddr2,
                (UINT16) (nDie << FSR_OND_4K_DBS_BASEBIT));

            /* set DFS & FBA */
            OND_4K_WRITE(pstFOReg->nStartAddr1,
                (UINT16) ((nDie << FSR_OND_4K_DFS_BASEBIT) | (nPbn & pstOND4kCxt->nFBAMask)));

            /* set FPA (Flash Page Address) */
            OND_4K_WRITE(pstFOReg->nStartAddr8,
                      (UINT16) (nPgOffset << pstOND4kCxt->nFPASelSft));

            /* set Start Buffer Register */
            OND_4K_WRITE(pstFOReg->nStartBuf, FSR_OND_4K_START_BUF_DEFAULT);


            if ((nFlag & FSR_LLD_FLAG_ECC_MASK) == FSR_LLD_FLAG_ECC_ON)
            {
                nSysConf1 = OND_4K_READ(pstFOReg->nSysConf1);
                if (nSysConf1 & 0x0100)
                {
                    OND_4K_CLR(pstFOReg->nSysConf1, FSR_OND_4K_CONF1_ECC_ON);
                }
            }
            else
            {
                OND_4K_SET(pstFOReg->nSysConf1, FSR_OND_4K_CONF1_ECC_OFF);
            }


            /* in case of non-blocking mode, interrupt should be enabled */
            if (nFlag & FSR_LLD_FLAG_INT_MASK)
            {
                FSR_OAM_ClrNEnableInt(pstOND4kCxt->nIntID);
            }
            else
            {
                FSR_OAM_ClrNDisableInt(pstOND4kCxt->nIntID);
            }


            /* because SUPERLOAD is not working in Asynchronous Mode
             * this version issue a normal load command (0x0000) for PLOAD flag
             */
            OND_4K_WRITE(pstFOReg->nCmd, (UINT16) gnLoadCmdArray[nCmdIdx]);


#if defined (FSR_LLD_STATISTICS)

            _AddOND4kStatLoad(nDev,
                            nDie,
                            nPbn,
                            FSR_OND_4K_STAT_NORMAL_CMD);

#endif /* #if defined (FSR_LLD_STATISTICS) */

            pstOND4kShMem->nPreOp[nDie]         = FSR_OND_4K_PREOP_READ;

        } /* if ((nCmdIdx == FSR_LLD_FLAG_1X_LOAD) | (nCmdIdx == FSR_LLD_FLAG_1X_PLOAD)) */

        pstOND4kShMem->nPreOpPbn[nDie]      = (UINT16) nPbn;
        pstOND4kShMem->nPreOpPgOffset[nDie] = (UINT16) nPgOffset;
        pstOND4kShMem->nPreOpFlag[nDie]     = nFlag;

#if defined (FSR_LLD_LOGGING_HISTORY)
        _AddLog(nDev, nDie);
#endif
    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return (nLLDRe);
}



/**
 * @brief           this function reads data from main area of DataRAM
 *
 * @param[out]      pDest   : pointer to the buffer
 * @param[in]       pstSrc  : pointer to the main area of DataRAM
 * @param[in]       nSize   : # of bytes to read
 * @param[in]       pTempBuffer: temporary buffer for reading misaligned data
 *
 * @return          none
 *
 * @author          NamOh Hwang / Kyungho Shin
 * @version         1.1.0
 * @remark          pTempBuffer is 4-byte aligned.
 * @n               the caller of _ReadMain() provides enough space for holding
 * @n               nSize bytes of data.
 *
 */
PRIVATE VOID
_ReadMain(         UINT8  *pDest,
          volatile UINT8  *pSrc,
                   UINT32  nSize,
                   UINT8  *pTempBuffer)
{
    FSR_STACK_VAR;

    FSR_STACK_END;


    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[OND:IN ] ++%s(pDest: 0x%08x, pSrc: 0x%08x, pTempBuffer: 0x%08x)\r\n"),
        __FSR_FUNC__, pDest, pSrc, pTempBuffer));

    /* if pDest is not 4 byte aligned. */
    if ((((UINT32) pDest & 0x3) != 0) || (nSize < FSR_OND_4K_MIN_BULK_TRANS_SIZE))
    {
        TRANSFER_FROM_NAND(pTempBuffer, pSrc, nSize);
        FSR_OAM_MEMCPY(pDest, pTempBuffer, nSize);
    }
    else /* when pDest is 4 byte aligned */
    {
        TRANSFER_FROM_NAND(pDest, pSrc, nSize);
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s()\r\n"), __FSR_FUNC__));
}



/**
 * @brief           this function reads data from spare area of DataRAM
 *
 * @param[out]      pstDest : pointer to the host buffer
 * @param[in]       pSrc    : pointer to the spare area of DataRAM
 *
 * @return          none
 *
 * @author          KyungHo Shin
 * @version         1.1.0
 * @remark          this function reads from spare area of 1 page.
 *
 */
PRIVATE INT32
_ReadSpare(         FSRSpareBuf *pstDest,
           volatile UINT8       *pSrc,
                    UINT32       nFlag)
{
    volatile UINT16 *pSrc16;
             UINT16 *pDest16;
             UINT16  nReg;

             UINT32  nByteAcs;
             UINT32  nTwoSctIdx;
             UINT32  nExtSctPos;
             UINT32  nExtBytePos;

#if defined (FSR_LLD_BIG_ENDIAN)
             UINT8  *pDest8 = (UINT8  *)pstDest;             
#endif
             UINT8   nBuf[8];
             UINT8   anDummyBuf[8];
             UINT8   nSWEccR[2];
             UINT16  nTemp16;
             UINT32  nIdx;
             BOOL32  bEccOn     = FALSE32;
             UINT8  *apDataForCorrection[4] = { NULL,}; 

             INT32   nLLDRe     = FSR_LLD_SUCCESS;
             INT32   nRe        = 0;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[OND:IN ] ++%s\r\n"), __FSR_FUNC__));

    FSR_OAM_MEMSET(&gstSpare4kBuffer[0], 0xFF, sizeof(gstSpare4kBuffer));
    TRANSFER_FROM_NAND(&gstSpare4kBuffer[0], pSrc, sizeof(gstSpare4kBuffer));

    /* If number of MetaExt is 0, dest address is changed to dummy buffer point*/
    FSR_OAM_MEMSET(&anDummyBuf[0], 0xFF, sizeof(anDummyBuf));

    pSrc16  = (UINT16 *) &gstSpare4kBuffer[0];
    pDest16 = (UINT16 *) pstDest->pstSpareBufBase;

    FSR_ASSERT(((UINT32) pstDest & 0x01) == 0);

    if ((nFlag & FSR_LLD_FLAG_ECC_MASK) == FSR_LLD_FLAG_ECC_ON)
    {
        bEccOn = TRUE32;
    }

    /* The value of nSctIdx is 4, because S/W Ecc of all sectors shoud be calculated. */
    nTwoSctIdx = ((FSR_SPARE_BUF_SIZE_4KB_PAGE / FSR_OND_4K_SPARE_USER_AREA) >> 2) + pstDest->nNumOfMetaExt;

    /* MetaExt is positioned on 3th sector */
    nExtSctPos = nTwoSctIdx - 1;

    do
    {
        nByteAcs = FSR_OND_4K_SPARE_USER_AREA +  FSR_OND_4K_SPARE_SW_ECC_AREA;

        /* MetaExt is positioned on 5th byte of 3th sector */
        nExtBytePos = nByteAcs - 2;
        nIdx     = 0;

        do
        {
            if (nTwoSctIdx == nExtSctPos && nByteAcs == nExtBytePos)
            {
                /* if number of meta extension is 0, the area is filled all '0xFF' */
                if (pstDest->nNumOfMetaExt != 0)
                {
                    pDest16 = (UINT16 *) &pstDest->pstSTLMetaExt[0];
                }
                else
                {
                    pDest16 = (UINT16 *) &anDummyBuf[0];
                }
            }

            if (nByteAcs % 4 == 1) /* S/W ECC area */
            {
                if (bEccOn == TRUE32)
                {        
              /* Calculated data about 6byte in spare area  */
#if defined (FSR_LLD_BIG_ENDIAN)
                    nReg       = *pSrc16++;
                    nSWEccR[0] = (UINT8)(nReg >> 8);

                    nReg       = *pSrc16++;
                    nSWEccR[1] = (UINT8)(nReg);

#else /* #if defined (FSR_LLD_BIG_ENDIAN) */

                    nTemp16    = *pSrc16++;

                    if (nByteAcs == 5)
                    {
                        nSWEccR[0] = (UINT8) (nTemp16 & 0x00FF);
                    }
                    else /* nByteAcs == 1 */
                    {
                        nSWEccR[1] = (UINT8) (nTemp16 & 0x00FF);
                    }
                   /* Skip H/W Ecc area */
                    pSrc16 += FSR_OND_4K_SPARE_HW_ECC_AREA - 1;
#endif /* #if defined (FSR_LLD_BIG_ENDIAN) */
                }
                else /* if (bEccOn == TRUE32) */
                {
                    pSrc16    += FSR_OND_4K_SPARE_HW_ECC_AREA; /* skip 8 bytes for hardware ECC */ 
                }
            }
            else
            {
#if defined (FSR_LLD_BIG_ENDIAN)
                 nReg      = *pSrc16++;
                *pDest8++  = (UINT8)(nReg >> 8);
                *pDest8++  = (UINT8)(nReg);

#else /* #if defined (FSR_LLD_BIG_ENDIAN) */
                 nReg      = *pSrc16++;
                 /* byte position is set */
                 
                 if ((nByteAcs % 4) != 3)
                 {
                     apDataForCorrection[nIdx >> 1] = (UINT8 *) pDest16;
                     nBuf[nIdx++] = (UINT8)(nReg);
                     nBuf[nIdx++] = (UINT8)(nReg >> 8);                        
                 }

                *pDest16++ = nReg;
#endif /* #if defined (FSR_LLD_BIG_ENDIAN) */
            }
        }while(--nByteAcs > 0);


       if (bEccOn == TRUE32)
       {
           nRe = FSR_OND_ECC_CompS (&nSWEccR[0], &nBuf[0], nTwoSctIdx);

            /* If uncorrectable error is occured, it should be returned though existing other errors  */
            if (nRe == FSR_OND_SWECC_N_ERROR)
            {
                if (nLLDRe != (FSR_LLD_PREV_READ_ERROR | FSR_LLD_1STPLN_CURR_ERROR))
                {
                    nLLDRe = FSR_LLD_SUCCESS;
                }
            }
            else if (nRe == FSR_OND_SWECC_U_ERROR)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_INF , (TEXT("[OND:ERR]   uncorrectable read error occurs\r\n")));

                nLLDRe = FSR_LLD_PREV_READ_ERROR | FSR_LLD_1STPLN_CURR_ERROR;
            }
            else if ((nRe == FSR_OND_SWECC_C_ERROR) ||
                     (nRe == FSR_OND_SWECC_R_ERROR))
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_INF , (TEXT("[OND:INF]   1 bit error occurs and corrected.\r\n")));
                
                /* Copy data into destination */
#if defined(FSR_LLD_BIG_ENDIAN)
                *(apDataForCorrection[0])    = nBuf[0];
                *(apDataForCorrection[0]+1)  = nBuf[1];
                *(apDataForCorrection[1])    = nBuf[2];
                *(apDataForCorrection[1]+1)  = nBuf[3];
                *(apDataForCorrection[2])    = nBuf[4];
                *(apDataForCorrection[2]+1)  = nBuf[5];
                *(apDataForCorrection[3])    = nBuf[6];
                *(apDataForCorrection[3]+1)  = nBuf[7];
#else
                *(apDataForCorrection[0])    = nBuf[0];
                *(apDataForCorrection[0]+1)  = nBuf[1];
                *(apDataForCorrection[1])    = nBuf[2];
                *(apDataForCorrection[1]+1)  = nBuf[3];
                *(apDataForCorrection[2])    = nBuf[4];
                *(apDataForCorrection[2]+1)  = nBuf[5];
                *(apDataForCorrection[3])    = nBuf[6];
                *(apDataForCorrection[3]+1)  = nBuf[7];
#endif

                if (nLLDRe != (FSR_LLD_PREV_READ_ERROR | FSR_LLD_1STPLN_CURR_ERROR))
                {
                    nLLDRe = FSR_LLD_PREV_READ_DISTURBANCE | FSR_LLD_1STPLN_CURR_ERROR;
                }
            }
       }
    } while (--nTwoSctIdx > 0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[OND:OUT] --%s (nRe=%x)\r\n"), __FSR_FUNC__, nRe));

    return (nLLDRe);
}



/**
 * @brief           this function writes data into 4K OneNAND
 *
 * @param[in]       nDev        : Physical Device Number (0 ~ 3)
 * @param[in]       nPbn        : Physical Block  Number
 * @param[in]       nPgOffset   : Page Offset within a block
 * @param[in]       pMBuf       : Memory buffer for main  array of NAND flash
 * @param[in]       pSBuf       : Memory buffer for spare array of NAND flash
 * @param[in]       nFlag       : Operation options such as ECC_ON, OFF
 *
 * @return          FSR_LLD_SUCCESS
 * @return          FSR_LLD_INVALID_PARAM
 * @return          FSR_LLD_PREV_WRITE_ERROR  | {FSR_LLD_1STPLN_CURR_ERROR}
 * @n               error for normal program
 * @return          FSR_LLD_PREV_WRITE_ERROR  | {FSR_LLD_1STPLN_PREV_ERROR}
 * @n               error for cache program
 * @return          FSR_LLD_WR_PROTECT_ERROR  | {FSR_LLD_1STPLN_CURR_ERROR}
 * @n               write protetion error return value
 *
 * @author          NamOh Hwang / Kyungho Shin
 * @version         1.1.0
 * @remark
 *
 */
PUBLIC INT32
FSR_OND_4K_Write(UINT32       nDev,
                 UINT32       nPbn,
                 UINT32       nPgOffset,
                 UINT8       *pMBuf,
                 FSRSpareBuf *pSBuf,
                 UINT32       nFlag)
{
             OneNAND4kCxt       *pstOND4kCxt  = NULL;
             OneNAND4kSpec      *pstOND4kSpec = NULL;
             OneNAND4kShMem     *pstOND4kShMem= NULL;
    volatile OneNAND4kReg   *pstFOReg   = NULL;

#if defined (FSR_LLD_HANDSHAKE_ERR_INF)
    volatile OneNAND4kSharedCxt *pstOND4kSharedCxt = NULL;
#endif

             UINT32            nCmdIdx;
             UINT32            nBBMMetaIdx;
             UINT32            nBadMarkIdx;
             UINT32            nDie;
             UINT32            nFlushOpCaller;
#if defined (FSR_LLD_STATISTICS)
             UINT32            nBytesTransferred = 0;
#endif /* #if defined (FSR_LLD_STATISTICS) */

             INT32             nLLDRe     = FSR_LLD_SUCCESS;

             UINT16            nWrProtectStat;

    FSR_STACK_VAR;

    FSR_STACK_END;



    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
    (TEXT("[OND:IN ] ++%s(nDev:%d, nPbn:%d, nPgOffset:%d, pMBuf: 0x%08x, pSBuf: 0x%08x, nFlag:0x%08x)\r\n"),
    __FSR_FUNC__, nDev, nPbn, nPgOffset, pMBuf, pSBuf, nFlag));

    do
    {

#if defined (FSR_LLD_STRICT_CHK)
        nLLDRe = _StrictChk(nDev, nPbn, nPgOffset);

        if (nLLDRe != FSR_LLD_SUCCESS)
        {
            break;
        }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */

        pstOND4kCxt  = gpstOND4kCxt[nDev];
        pstOND4kShMem  = gpstOND4kShMem[nDev];
        pstOND4kSpec = pstOND4kCxt->pstOND4kSpec;
        pstFOReg   = (volatile OneNAND4kReg *) pstOND4kCxt->nBaseAddr;

        nCmdIdx = (nFlag & FSR_LLD_FLAG_CMDIDX_MASK) >> FSR_LLD_FLAG_CMDIDX_BASEBIT;

        nDie = (nPbn >> (FSR_OND_4K_DFS_BASEBIT - pstOND4kCxt->nDDPSelSft));

        nFlushOpCaller = FSR_OND_4K_PREOP_PROGRAM << FSR_OND_4K_FLUSHOP_CALLER_BASEBIT;

        nLLDRe = FSR_OND_4K_FlushOp(nDev, nFlushOpCaller | nDie, nFlag);
        if (nLLDRe != FSR_LLD_SUCCESS)
        {
            break;
        }

        /* set DBS */
        OND_4K_WRITE(pstFOReg->nStartAddr2,
            (UINT16) (nDie << FSR_OND_4K_DBS_BASEBIT));


        /* Write Data Into DataRAM */
        if (pMBuf != NULL)
        {
            _WriteMain(&(pstFOReg->nDataMB00[0]),
                       pMBuf,
                       pstOND4kSpec->nSctsPerPG * FSR_OND_4K_SECTOR_SIZE);

#if defined (FSR_LLD_STATISTICS)
            nBytesTransferred += pstOND4kSpec->nSctsPerPG * FSR_OND_4K_SECTOR_SIZE;
#endif /* #if defined (FSR_LLD_STATISTICS) */
        }

        if (pMBuf == NULL && pSBuf != NULL)
        {
            if((nFlag & FSR_LLD_FLAG_BACKUP_DATA) != FSR_LLD_FLAG_BACKUP_DATA)
            {
                OND_MEMSET_DATARAM(pstOND4kCxt->pTempBuffer, 0xFF, sizeof(pstFOReg->nDataMB00) * pstOND4kSpec->nSctsPerPG);
                TRANSFER_TO_NAND((VOID *)pstFOReg->nDataMB00,
                                   pstOND4kCxt->pTempBuffer,
                                   sizeof(pstFOReg->nDataMB00) * pstOND4kSpec->nSctsPerPG);
            }
        }

        if (pSBuf != NULL)
        {
            if ((nFlag & FSR_LLD_FLAG_DUMP_MASK) == FSR_LLD_FLAG_DUMP_ON)
            {
                _WriteMain(&(pstFOReg->nDataSB00[0]),
                           (UINT8 *) pSBuf,
                           pstOND4kSpec->nSparePerSct * pstOND4kSpec->nSctsPerPG);
            }
            else
            {
                /* if FSR_LLD_FLAG_BBM_META_BLOCK of nFlag is set,
                 * write nBMLMetaBase0 of FSRSpareBuf with 0xA5A5
                 */
                nBBMMetaIdx = (nFlag & FSR_LLD_FLAG_BBM_META_MASK) >>
                              FSR_LLD_FLAG_BBM_META_BASEBIT;

                /* FSR_OND_MAX_BADMARK : 0 (0xFFFF), 1 (0x2222), 2 (0x4444), 3 (0x8888) */
                nBadMarkIdx = (nFlag & FSR_LLD_FLAG_BADMARK_MASK) >> 
                                    FSR_LLD_FLAG_BADMARK_BASEBIT;

                /* Do not delete this code which writes bad mark  */
                pSBuf->pstSpareBufBase->nBadMark      = gnBadMarkValue[nBadMarkIdx];
                pSBuf->pstSpareBufBase->nBMLMetaBase0 = gnBBMMetaValue[nBBMMetaIdx];

                /* _WriteSpare does not care about bad mark which is written at
                 * the first word of sector 0 of the spare area 
                 * bad mark is written individually.
                 */
                _WriteSpare(&(pstFOReg->nDataSB00[0]), pSBuf, nFlag);
            }

#if defined (FSR_LLD_STATISTICS)
            if ((nFlag & FSR_LLD_FLAG_DUMP_MASK) == FSR_LLD_FLAG_DUMP_OFF)
            {
                nBytesTransferred += (FSR_SPARE_BUF_SIZE_4KB_PAGE);
            }
            else
            {
                nBytesTransferred +=
                    pstOND4kSpec->nSctsPerPG * pstOND4kSpec->nSparePerSct;
            }
#endif /* #if defined (FSR_LLD_STATISTICS) */
        }

        if (pMBuf != NULL && pSBuf == NULL)
        {
            if((nFlag & FSR_LLD_FLAG_BACKUP_DATA) != FSR_LLD_FLAG_BACKUP_DATA)
            {
                OND_MEMSET_DATARAM(pstOND4kCxt->pTempBuffer,
                                        0xFF,
                                        sizeof(pstFOReg->nDataSB00) * pstOND4kSpec->nSctsPerPG);
                TRANSFER_TO_NAND((VOID *)pstFOReg->nDataSB00,
                                       pstOND4kCxt->pTempBuffer,
                                       sizeof(pstFOReg->nDataSB00) * pstOND4kSpec->nSctsPerPG);                    
            }
        }

        if ((nFlag & FSR_LLD_FLAG_DUMP_MASK) == FSR_LLD_FLAG_DUMP_OFF)
        {
            /* write bad mark of the block
             * bad mark is not written in _WriteSpare()
             */
            OND_4K_WRITE(*(volatile UINT16 *) &pstFOReg->nDataSB00[0],
                gnBadMarkValue[(nFlag & FSR_LLD_FLAG_BADMARK_MASK) >> FSR_LLD_FLAG_BADMARK_BASEBIT]);
        }


#if defined (FSR_LLD_WAIT_ALLDIE_PGM_READY)
        /* In case of DDP, wait the other die ready.
         * When there is a cache program going on the other die, 
         * setting the address register leads to the problem.
         * that is.. data under programming will be written to the newly set address.
         */
        if (pstOND4kSpec->nNumOfDies == FSR_MAX_DIES)
        {
            FSR_OND_4K_FlushOp(nDev,
                            nFlushOpCaller | ((nDie + 0x1) & 0x01),
                            nFlag | FSR_LLD_FLAG_REMAIN_PREOP_STAT);
        }        
#endif

        /* set DFS, FBA */
        OND_4K_WRITE(pstFOReg->nStartAddr1,
        (UINT16)((nDie << FSR_OND_4K_DFS_BASEBIT) | (nPbn & pstOND4kCxt->nFBAMask)));

#if defined(FSR_LLD_WAIT_WR_PROTECT_STAT)
        /* wait until lock bit is set */
        do
        {
            nWrProtectStat = OND_4K_READ(pstFOReg->nWrProtectStat);
        } while (nWrProtectStat == (UINT16) 0x0000);
#else
        nWrProtectStat = OND_4K_READ(pstFOReg->nWrProtectStat);
#endif
        /******************CAUTION ***************************************
         * when cache program (0x007F) is performed on a locked block    *
         * 4K OneNAND automatically resets itself                      *
         * so makes sure that cache program command is not issued on a   *
         * locked block                                                  *
         *****************************************************************
         */
        /* write protection can be checked when DBS & FBA is set */
        if ((nWrProtectStat & FSR_LLD_BLK_STAT_MASK) != FSR_LLD_BLK_STAT_UNLOCKED)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s() / %d line \r\n"),
                __FSR_FUNC__, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Pbn #%d, Pg #%d is write protected\r\n"),
                nPbn, nPgOffset));

            _DumpRegisters(pstFOReg);
            _DumpSpareBuffer(pstFOReg);

            nLLDRe = (FSR_LLD_WR_PROTECT_ERROR | FSR_LLD_1STPLN_CURR_ERROR);
            break;
        }

        /* set Start Page Address (FPA) */
        OND_4K_WRITE(pstFOReg->nStartAddr8,
                  (UINT16) (nPgOffset << pstOND4kCxt->nFPASelSft));

        /* set Start Buffer Register */
        OND_4K_WRITE(pstFOReg->nStartBuf, FSR_OND_4K_START_BUF_DEFAULT);

        if ((nFlag & FSR_LLD_FLAG_ECC_MASK) == FSR_LLD_FLAG_ECC_ON)
        {
            OND_4K_CLR(pstFOReg->nSysConf1, FSR_OND_4K_CONF1_ECC_ON);
        }
        else
        {
            OND_4K_SET(pstFOReg->nSysConf1, FSR_OND_4K_CONF1_ECC_OFF);
        }

        /* in case of non-blocking mode, interrupt should be enabled */
        if (nFlag & FSR_LLD_FLAG_INT_MASK)
        {
            FSR_OAM_ClrNEnableInt(pstOND4kCxt->nIntID);
        }
        else
        {
            FSR_OAM_ClrNDisableInt(pstOND4kCxt->nIntID);
        }

        /* issue command */
        OND_4K_WRITE(pstFOReg->nCmd, (UINT16) gnPgmCmdArray[nCmdIdx]);

#if defined(FSR_LLD_USE_CACHE_PGM)
        if (gnPgmCmdArray[nCmdIdx] == FSR_OND_4K_CMD_CACHEPGM)
        {
            pstOND4kShMem->nPreOp[nDie] = FSR_OND_4K_PREOP_CACHE_PGM;
            pstOND4kCxt->bIsPreCmdCache[nDie] = TRUE32;
        }
        else
        {
            if (pstOND4kCxt->bIsPreCmdCache[nDie] == TRUE32)
            {
                pstOND4kShMem->nPreOp[nDie] = FSR_OND_4K_PREOP_CACHE_PGM;
            }
            else
            {
                pstOND4kShMem->nPreOp[nDie] = FSR_OND_4K_PREOP_PROGRAM;
            }

            pstOND4kCxt->bIsPreCmdCache[nDie] = FALSE32;
        }
#else
        pstOND4kShMem->nPreOp[nDie] = FSR_OND_4K_PREOP_PROGRAM;
#endif  /* #if defined(FSR_LLD_USE_CACHE_PGM) */

        pstOND4kShMem->nPreOpPbn[nDie]      = (UINT16) nPbn;
        pstOND4kShMem->nPreOpPgOffset[nDie] = (UINT16) nPgOffset;
        pstOND4kShMem->nPreOpFlag[nDie]     = nFlag;

#if defined (FSR_LLD_HANDSHAKE_ERR_INF)
        pstOND4kSharedCxt = &gstOND4kSharedCxt[nDev];
        pstOND4kSharedCxt->nPrevFSRMode[nDie]     = nFlag & FSR_LLD_FLAG_READ_ONLY;
        pstOND4kSharedCxt->nHostLLDOp[nDie]       = pstOND4kShMem->nPreOp[nDie];
        pstOND4kSharedCxt->nHostLLDPbn[nDie]      = (UINT16) nPbn;
        pstOND4kSharedCxt->nHostLLDPgOffset[nDie] = (UINT16) nPgOffset;
        pstOND4kSharedCxt->nHostLLDFlag[nDie]     = nFlag;
#endif

#if defined (FSR_LLD_LOGGING_HISTORY)
        _AddLog(nDev, nDie);
#endif

#if defined (FSR_LLD_STATISTICS)
        if (gnPgmCmdArray[nCmdIdx] == FSR_OND_4K_CMD_CACHEPGM)
        {
            _AddOND4kStat(nDev,
                        nDie,
                        FSR_OND_4K_STAT_WR_TRANS,
                        nBytesTransferred,
                        FSR_OND_4K_STAT_CACHE_PGM);

            _AddOND4kStatPgm(nDev,
                        nDie,
                        nPbn,
                        nPgOffset,
                        FSR_OND_4K_STAT_CACHE_PGM);
        }
        else
        {
            _AddOND4kStat(nDev,
                        nDie,
                        FSR_OND_4K_STAT_WR_TRANS,
                        nBytesTransferred,
                        FSR_OND_4K_STAT_NORMAL_CMD);

            _AddOND4kStatPgm(nDev,
                        nDie,
                        nPbn,
                        nPgOffset,
                        FSR_OND_4K_STAT_NORMAL_CMD);
        }
#endif /* #if defined (FSR_LLD_STATISTICS) */
    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));
    
    return (nLLDRe);
}



/**
 * @brief           this function writes data into main area of DataRAM
 *
 * @param[in]       pDest   : pointer to the main area of DataRAM
 * @param[in]       pSrc    : pointer to the buffer
 * @param[in]       nSize   : the number of bytes to write
 *
 * @return          none
 *
 * @author          NamOh Hwang / Kyungho Shin
 * @version         1.1.0
 * @remark          pDest is a pointer to main DataRAM
 * @n               nSize is the number of bytes to write
 *
 */
PRIVATE VOID
_WriteMain(volatile UINT8  *pDest,
                    UINT8  *pSrc,
                    UINT32  nSize)
{
             UINT32  nCnt;
    volatile UINT16 *pTgt16;
             UINT16  nReg;

    OneNAND4kCxt    *pstOND4kCxt = NULL;

    FSR_STACK_VAR;

    FSR_STACK_END;

    pstOND4kCxt = gpstOND4kCxt[0];

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[OND:IN ] ++%s(pDest: 0x%08x, pSrc: 0x%08x, nSize: %d)\r\n"),
        __FSR_FUNC__, pDest, pSrc, nSize));

    

    /* if nSize is not multiple of 4, memcpy is not guaranteed.
     * although here only checks that nSize is multiple of 4, 
     * nSize should not be an odd.
     */
#if defined (FSR_MSM7200)
    if ( ((nSize & 0x3) != 0) && (((UINT32) pSrc & 0x03) != 0) && (nSize >=FSR_OND_4K_MIN_BULK_TRANS_SIZE) )
    {
        FSR_OAM_MEMCPY(pstOND4kCxt->pTempBuffer, pSrc, nSize); 
        TRANSFER_TO_NAND(pDest, pstOND4kCxt->pTempBuffer, nSize); 
    }
    else if ( ((nSize & 0x3) != 0) || (((UINT32) pSrc & 0x03) != 0) || (nSize < FSR_OND_4K_MIN_BULK_TRANS_SIZE) )
#else
    if (((nSize & 0x3) != 0) ||
        (((UINT32) pSrc & 0x3) != 0) ||
        (nSize < FSR_OND_4K_MIN_BULK_TRANS_SIZE))
#endif
    {
        pTgt16 = (volatile UINT16 *) pDest;

        /* copy it by word, so loop it for nSize/2 */
        nSize  = nSize >> 1;
        for (nCnt = 0; nCnt < nSize; nCnt++)
        {
#if defined (FSR_LLD_BIG_ENDIAN)
            nReg   = *pSrc++ << 8;
            nReg  |= *pSrc++;
#else /* #if defined (FSR_LLD_BIG_ENDIAN) */
            nReg   = *pSrc++;
            nReg  |= *pSrc++ << 8;
#endif /* #if defined (FSR_LLD_BIG_ENDIAN) */
            OND_4K_WRITE(*pTgt16++, nReg);
        }

    }
    else
    {
        /* nSize is the number of bytes to transfer */
        TRANSFER_TO_NAND(pDest, pSrc, nSize);
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s()\r\n"), __FSR_FUNC__));
}


/**
 * @brief           this function writes FSRSpareBuf into spare area of DataRAM
 *
 * @param[in]       pDest   : pointer to the spare area of DataRAM
 * @param[in]       pstSrc  : pointer to the structure, FSRSpareBuf
 * @param[in]       nFlag   : whether to write spare buffer onto DataRAM or not.
 *
 * @return          none
 *
 * @author          Kyungho Shin
 * @version         1.1.0
 * @remark          this function writes FSRSpareBuf over spare area of 1 page
 *
 */
PRIVATE VOID
_WriteSpare(volatile UINT8       *pDest,
                     FSRSpareBuf *pstSrc,
                     UINT32       nFlag)
{
             UINT32   nSctIdx;
             UINT32   nTwoSctIdx;
             UINT16  *pSrc16;
    volatile UINT16  *pTgt16;
             UINT32   nMetaIdx;

             UINT8    nBuf[8];
             UINT16  *pLsn16 = NULL;
             UINT16   nGenValue;
             UINT16   nEcc_LSB;
             UINT16   nEcc_MSB;

             UINT32   bEccOn     = FALSE32;
             UINT8    anSpareBuf[FSR_SPARE_BUF_SIZE_4KB_PAGE];
             UINT16  *pTempBuffer16;

#if defined (FSR_LLD_BIG_ENDIAN)
             UINT16   nReg;
             UINT8   *pSrc4Ecc8;
             UINT8   *pSrc8;
#endif /* #if defined (FSR_LLD_BIG_ENDIAN) */

    FSR_STACK_VAR;

    FSR_STACK_END;

    /* when pstSrc->nNumOfMetaExt is 0, fill the rest of spare area with 0xFF */
    OND_MEMSET_DATARAM(&anSpareBuf[0], 0xFF, sizeof(anSpareBuf));

    if (nFlag & FSR_LLD_FLAG_USE_SPAREBUF)
    {
        FSR_OAM_MEMCPY(&anSpareBuf[0], pstSrc->pstSpareBufBase, FSR_SPARE_BUF_BASE_SIZE);
        
        for (nMetaIdx = 0; nMetaIdx < pstSrc->nNumOfMetaExt; nMetaIdx++)
        {
            FSR_OAM_MEMCPY(&anSpareBuf[FSR_SPARE_BUF_BASE_SIZE + nMetaIdx * FSR_SPARE_BUF_EXT_SIZE],
                           &pstSrc->pstSTLMetaExt[nMetaIdx],
                           FSR_SPARE_BUF_EXT_SIZE);
        }
    }

    if ((nFlag & FSR_LLD_FLAG_ECC_MASK) == FSR_LLD_FLAG_ECC_ON)
    {
        bEccOn = TRUE32;
    }
    else
    {
        bEccOn = FALSE32;
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[OND:IN ] ++%s\r\n"), __FSR_FUNC__));

    OND_MEMSET_DATARAM(&gstSpare4kBuffer[0], 0xFF, sizeof(gstSpare4kBuffer));
    pTempBuffer16 = &gstSpare4kBuffer[0];

#if defined (FSR_LLD_BIG_ENDIAN)
    pSrc8  = &anSpareBuf[0];
#endif

    pSrc16 = (UINT16 *) &anSpareBuf[0];
    pTgt16 = (volatile UINT16 *) pDest;

    FSR_ASSERT(((UINT32) pSrc16 & 0x01) == 0);

    /* If nNumofMetaExt value is 2, idx should be 4. because loop should operate 4 times. */
    nTwoSctIdx = ((FSR_SPARE_BUF_SIZE_4KB_PAGE / FSR_OND_4K_SPARE_USER_AREA) >> 1);

    do
    {
        nSctIdx = ((FSR_SPARE_BUF_SIZE_4KB_PAGE / FSR_OND_4K_SPARE_USER_AREA) >> 2);

        if (bEccOn == TRUE32) //ECC generation
        {
            pLsn16 = pSrc16;

        /* There`s no loop for guarantee the performance */
            nBuf[0] = (UINT8) *pLsn16;
            nBuf[1] = (UINT8) (*pLsn16++ >> 8);
            pLsn16++;
            nBuf[2] = (UINT8) *pLsn16;
            nBuf[3] = (UINT8) (*pLsn16++ >> 8);
            nBuf[4] = (UINT8) *pLsn16;
            nBuf[5] = (UINT8) (*pLsn16++ >> 8);
            pLsn16++;
            nBuf[6] = (UINT8) *pLsn16;
            nBuf[7] = (UINT8) (*pLsn16++ >> 8);

            /* ECC spaces for spare data */
            FSR_OND_ECC_GenS (&nGenValue, &nBuf[0]);
        }

        do
        {
#if defined (FSR_LLD_BIG_ENDIAN)
            nReg  = *pSrc8++ << 8;
            nReg |= *pSrc8++;
            *pTempBuffer16++ = nReg;

            nReg  = *pSrc8++ << 8;
            nReg |= *pSrc8++;
            *pTempBuffer16++ = nReg;

            nReg  = *pSrc8++ << 8;
            nReg |= *pSrc8++;
            *pTempBuffer16++ = nReg;

            nReg  = *pSrc8++ << 8;
            nReg |= *pSrc8++;
            *pTempBuffer16++ = nReg;
#else /* #if defined (FSR_LLD_BIG_ENDIAN) */
            *pTempBuffer16++ = *pSrc16++;
            *pTempBuffer16++ = *pSrc16++;
            *pTempBuffer16++ = *pSrc16++;
#endif /* #if defined (FSR_LLD_BIG_ENDIAN) */

            if (bEccOn == TRUE32)
            {
                /* 
                 * You must access the NAND array by unit of word.
                 * maybe not, it should be fallen in error for exception.
                 * Hardware Ecc area uses 5 bytes space, for if you want to 
                 * use software Ecc area by next byte, it occur 2 times to read
                 * NAND array about MSB and LSB area.
                 */
                /* H/W ECC spaces */

#if defined (FSR_LLD_BIG_ENDIAN)
                nEcc_MSB  = OND_4K_READ(*pTmpTgt++);
                nEcc_MSB = (nEcc_MSB >> 8) | (UINT16) (nEcc_MSB << 8);

                nEcc_LSB  = OND_4K_READ(*pTmpTgt++);
                nEcc_LSB = (nEcc_MSB >> 8) | (UINT16) (nEcc_MSB << 8);

                pSrc4Ecc8   = (UINT8 *) &nEcc_MSB;

                nReg  = *pSrc4Ecc8++ << 8;
                nReg |= *pSrc4Ecc8++;
                *pTempBuffer16++ = nReg;

                pSrc4Ecc8   = (UINT8 *) &nEcc_LSB;

                nReg  = *pSrc4Ecc8++ << 8;
                nReg |= *pSrc4Ecc8++;
                *pTempBuffer16++ = nReg;
#else /* #if defined (FSR_LLD_BIG_ENDIAN) */
                if (nSctIdx == 2)
                {
                    nEcc_MSB = OND_4K_READ(*pTgt16);
                    nEcc_MSB = (nEcc_MSB & 0xFF00) | (nGenValue & 0x00FF);
                    *pTempBuffer16++ = nEcc_MSB;
                }
                else
                {
                    nEcc_LSB = OND_4K_READ(*pTgt16); 
                    nEcc_LSB = (nEcc_LSB & 0xFF00) | ((nGenValue >> 8) & 0x00FF);
                    *pTempBuffer16++ = nEcc_LSB;
                }
#endif /* #if defined (FSR_LLD_BIG_ENDIAN) */

                /* set address to location for SWEcc area */
                pTempBuffer16 += FSR_OND_4K_SPARE_HW_ECC_AREA -1;
            }
            else /* if (bEccOn == TRUE32) */
            {
                pTempBuffer16 += FSR_OND_4K_SPARE_HW_ECC_AREA;
            }
        }while (--nSctIdx > 0);

    } while (--nTwoSctIdx > 0);

    TRANSFER_TO_NAND(pDest, &gstSpare4kBuffer[0], sizeof(gstSpare4kBuffer));

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[OND:OUT] --%s\r\n"), __FSR_FUNC__));
}





/**
 * @brief           this function erase a block of 4K OneNAND
 *
 * @param[in]       nDev        : Physical Device Number (0 ~ 3)
 * @param[in]       pnPbn       : array of blocks, not necessarilly consecutive.
 * @n                             multi block erase will be supported in the future
 * @param[in]       nFlag       : FSR_LLD_FLAG_1X_ERASE
 *
 * @return          FSR_LLD_SUCCESS
 * @return          FSR_LLD_INVALID_PARAM
 * @return          FSR_LLD_PREV_ERASE_ERROR | {FSR_LLD_1STPLN_CURR_ERROR}
 * @n               erase error for previous erase operation
 * @return          FSR_LLD_WR_PROTECT_ERROR | {FSR_LLD_1STPLN_CURR_ERROR}
 * @n               protection error for previous erase operation
 *
 * @author          NamOh Hwang / Kyungho Shin
 * @version         1.1.0
 * @remark          as of now, supports only single plane, one block erase
 * @                Pbn is consecutive numbers within the device
 * @n               i.e. for DDP device, Pbn for 1st block in 2nd chip is not 0
 * @n               it is one more than the last block number in 1st chip
 *
 */
PUBLIC INT32
FSR_OND_4K_Erase(UINT32  nDev,
                 UINT32 *pnPbn,
                 UINT32  nNumOfBlks,
                 UINT32  nFlag)
{
             OneNAND4kCxt       *pstOND4kCxt  = NULL;
             OneNAND4kSpec      *pstOND4kSpec = NULL;
             OneNAND4kShMem     *pstOND4kShMem= NULL;
    volatile OneNAND4kReg   *pstFOReg   = NULL;

#if defined (FSR_LLD_HANDSHAKE_ERR_INF)
    volatile OneNAND4kSharedCxt *pstOND4kSharedCxt;
#endif

             UINT32            nCmdIdx;
             UINT32            nDie;
             UINT32            nPbn       = *pnPbn;
             UINT32            nFlushOpCaller;

             INT32             nLLDRe     = FSR_LLD_SUCCESS;
             UINT16            nWrProtectStat;


    FSR_STACK_VAR;

    FSR_STACK_END;


    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:IN ] ++%s(nDev:%d, *pnPbn:%d, nNumOfBlks:%d)\r\n"),
        __FSR_FUNC__, nDev, *pnPbn, nNumOfBlks));

    do
    {
#if defined (FSR_LLD_STRICT_CHK)
        /* check device number */
        if (nDev >= FSR_OND_4K_MAX_DEVS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("[OND:ERR]   %s(nDev:%d, *pnPbn:%d, nNumOfBlks:%d, nFlag:0x%08x) / %d line\r\n"),
            __FSR_FUNC__, nDev, *pnPbn, nNumOfBlks, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("            Invalid Device Number (nDev = %d)\r\n"), nDev));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }
#endif

        pstOND4kCxt  = gpstOND4kCxt[nDev];
        pstOND4kShMem= gpstOND4kShMem[nDev];
        pstOND4kSpec = pstOND4kCxt->pstOND4kSpec;
        pstFOReg   = (volatile OneNAND4kReg *) pstOND4kCxt->nBaseAddr;

        /* to support multi block erase,
         * function parameter pnPbn is defined as an array
         * though multi block erase is not supported yet.
         * nNumOfBlks can have a value of 1 for now.
         */
        FSR_ASSERT(nNumOfBlks == 1);

#if defined (FSR_LLD_STRICT_CHK)
        if (nPbn >= pstOND4kSpec->nNumOfBlks)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("[OND:ERR]   %s(nDev:%d, *pnPbn:%d, nNumOfBlks:%d, nFlag:0x%08x) / %d line\r\n"),
            __FSR_FUNC__, nDev, *pnPbn, nNumOfBlks, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Pbn:%d is invalid\r\n"), nPbn));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */


        nDie = nPbn >> (FSR_OND_4K_DFS_BASEBIT - pstOND4kCxt->nDDPSelSft);

        nFlushOpCaller = FSR_OND_4K_PREOP_ERASE << FSR_OND_4K_FLUSHOP_CALLER_BASEBIT;

        nLLDRe = FSR_OND_4K_FlushOp(nDev, nFlushOpCaller | nDie, nFlag);
        if (nLLDRe != FSR_LLD_SUCCESS)
        {
            break;
        }

        /* WARNING : DO NOT change address setting sequence 
           1. set DBS 
           2. set FBA */

        /* set DBS */
        OND_4K_WRITE(pstFOReg->nStartAddr2,
            (UINT16) (nDie << FSR_OND_4K_DBS_BASEBIT));
            
        /* set DFS, FBA */
        OND_4K_WRITE(pstFOReg->nStartAddr1,
        (UINT16)((nDie << FSR_OND_4K_DFS_BASEBIT) | (nPbn & pstOND4kCxt->nFBAMask)));

        

#if defined(FSR_LLD_WAIT_WR_PROTECT_STAT)
        /* wait until lock bit is set */
        do
        {
            nWrProtectStat = OND_4K_READ(pstFOReg->nWrProtectStat);
        } while (nWrProtectStat == (UINT16) 0x0000);
#else
        nWrProtectStat = OND_4K_READ(pstFOReg->nWrProtectStat);
#endif
        /* write protection can be checked at this point */
        if ((nWrProtectStat & FSR_LLD_BLK_STAT_MASK) != FSR_LLD_BLK_STAT_UNLOCKED)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("[OND:ERR]   %s(nDev:%d, *pnPbn:%d, nNumOfBlks:%d, nFlag:0x%08x) / %d line\r\n"),
            __FSR_FUNC__, nDev, *pnPbn, nNumOfBlks, nFlag, __LINE__));


            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Pbn #%d is write protected\r\n"), nPbn));

            _DumpRegisters(pstFOReg);
            _DumpSpareBuffer(pstFOReg);

            nLLDRe = (FSR_LLD_WR_PROTECT_ERROR | FSR_LLD_1STPLN_CURR_ERROR);
            break;
        }

        /* in case of non-blocking mode, interrupt should be enabled */
        if (nFlag & FSR_LLD_FLAG_INT_MASK)
        {
            FSR_OAM_ClrNEnableInt(pstOND4kCxt->nIntID);
        }
        else
        {
            FSR_OAM_ClrNDisableInt(pstOND4kCxt->nIntID);
        }

        nCmdIdx = (nFlag & FSR_LLD_FLAG_CMDIDX_MASK) >> FSR_LLD_FLAG_CMDIDX_BASEBIT;
        OND_4K_WRITE(pstFOReg->nCmd, (UINT16) gnEraseCmdArray[nCmdIdx]);


        pstOND4kShMem->nPreOp[nDie]                 = FSR_OND_4K_PREOP_ERASE;
        pstOND4kShMem->nPreOpPbn[nDie]              = (UINT16) nPbn;
        pstOND4kShMem->nPreOpPgOffset[nDie]         = FSR_OND_4K_PREOP_ADDRESS_NONE;
        pstOND4kShMem->nPreOpFlag[nDie]             = nFlag;

#if defined (FSR_LLD_HANDSHAKE_ERR_INF)
        pstOND4kSharedCxt = &gstOND4kSharedCxt[nDev];
        pstOND4kSharedCxt->nPrevFSRMode[nDie]     = nFlag & FSR_LLD_FLAG_READ_ONLY;
        pstOND4kSharedCxt->nHostLLDOp[nDie]       = FSR_OND_4K_PREOP_ERASE;
        pstOND4kSharedCxt->nHostLLDPbn[nDie]      = (UINT16) nPbn;
        pstOND4kSharedCxt->nHostLLDPgOffset[nDie] = FSR_OND_4K_PREOP_ADDRESS_NONE;
        pstOND4kSharedCxt->nHostLLDFlag[nDie]     = nFlag;
#endif

#if defined (FSR_LLD_LOGGING_HISTORY)
        _AddLog(nDev, nDie);
#endif

#if defined (FSR_LLD_STATISTICS)
        _AddOND4kStat(nDev,
                    nDie,
                    FSR_OND_4K_STAT_ERASE,
                    0,
                    FSR_OND_4K_STAT_NORMAL_CMD);
#endif /* #if defined (FSR_LLD_STATISTICS) */

    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return (nLLDRe);
}



/**
 * @brief           this function copybacks 1 page.
 *
 * @param[in]       nDev        : Physical Device Number (0 ~ 3)
 * @param[in]       pstCpArg    : pointer to the structure LLDCpBkArg 
 * @param[in]       nFlag       : FSR_LLD_FLAG_1X_CPBK_LOAD,
 * @n                             FSR_LLD_FLAG_1X_CPBK_PROGRAM
 *
 * @return          FSR_LLD_SUCCESS
 * @return          FSR_LLD_INVALID_PARAM
 * @return          FSR_LLD_PREV_READ_ERROR       | {FSR_LLD_1STPLN_CURR_ERROR}
 * @n               uncorrectable read error for previous read operation
 * @return          FSR_LLD_PREV_READ_DISTURBANCE | {FSR_LLD_1STPLN_CURR_ERROR}
 * @return          FSR_LLD_PREV_WRITE_ERROR      | {FSR_LLD_1STPLN_PREV_ERROR}
 * @n               write error for previous write operation
 * @return          FSR_LLD_WR_PROTECT_ERROR      | {FSR_LLD_1STPLN_CURR_ERROR}
 * @n               write protetion error return value
 *
 * @author          NamOh Hwang / Kyungho Shin
 * @version         1.1.0
 * @remark          this function loads data from 4K OneNAND,
 * @n               and randomly writes data into DataRAM of 4K OneNAND
 * @n               and program back into 4K OneNAND
 *
 */
PUBLIC INT32
FSR_OND_4K_CopyBack(UINT32      nDev,
                    LLDCpBkArg *pstCpArg,
                    UINT32      nFlag)
{
             OneNAND4kCxt       *pstOND4kCxt  = NULL;
             OneNAND4kSpec      *pstOND4kSpec = NULL;
             OneNAND4kShMem     *pstOND4kShMem= NULL;
#if defined (FSR_LLD_HANDSHAKE_ERR_INF)
    volatile OneNAND4kSharedCxt *pstOND4kSharedCxt = NULL;
#endif
    volatile OneNAND4kReg   *pstFOReg   = NULL;

             LLDRndInArg      *pstRIArg; /* random in argument */

             UINT32            nCmdIdx;
             UINT32            nCnt;
             UINT32            nPgOffset  = 0;
             UINT32            nDie       = 0;
             UINT32            nPbn       = 0;
             UINT32            nRndOffset = 0;
             UINT32            nFlushOpCaller;
             BOOL32            bSpareBufRndIn;

             FSRSpareBuf       stSpareBuf;
             FSRSpareBufBase   stSpareBufBase;
             FSRSpareBufExt    stSpareBufExt[FSR_MAX_SPARE_BUF_EXT];

#if defined (FSR_LLD_STATISTICS)
             UINT32            nBytesTransferred = 0;
#endif /* #if defined (FSR_LLD_STATISTICS) */

             INT32             nLLDRe     = FSR_LLD_SUCCESS;
             UINT16            nSysConf1;
             UINT16            nWrProtectStat;

    FSR_STACK_VAR;

    FSR_STACK_END;



    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:IN ] ++%s(nDev:%d, nFlag:0x%08x)\r\n"),
        __FSR_FUNC__, nDev, nFlag));

    do
    {
        FSR_OAM_MEMSET(&stSpareBuf, 0x00, sizeof(FSRSpareBuf));

#if defined (FSR_LLD_STRICT_CHK)
        /* check device number */
        if (nDev >= FSR_OND_4K_MAX_DEVS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s(nDev:%d, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Invalid Device Number (nDev = %d)\r\n"),
                nDev));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }

        if (pstCpArg == NULL)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s(nDev:%d, pstCpArg:0x%08x, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, pstCpArg, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            pstCpArg is NULL\r\n")));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */

        pstOND4kCxt  = gpstOND4kCxt[nDev];
        pstOND4kShMem= gpstOND4kShMem[nDev];
        pstOND4kSpec = pstOND4kCxt->pstOND4kSpec;
        pstFOReg   = (volatile OneNAND4kReg *) pstOND4kCxt->nBaseAddr;


        nCmdIdx = (nFlag & FSR_LLD_FLAG_CMDIDX_MASK) >> FSR_LLD_FLAG_CMDIDX_BASEBIT;

        if (nCmdIdx == FSR_LLD_FLAG_1X_CPBK_LOAD)
        {
            FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_INF,
                (TEXT("[OND:INF]   %s(nDev:%d, nSrcPbn:%d, nSrcPg:%d, nFlag:0x%08x)\r\n"),
                __FSR_FUNC__, nDev, pstCpArg->nSrcPbn, pstCpArg->nSrcPgOffset, nFlag));

            /* load phase of copyback() only checks the source block & page
             * offset. For BML does not fill the destination block & page
             * offset at this phase
             */
            nPbn      = pstCpArg->nSrcPbn;
            nPgOffset = pstCpArg->nSrcPgOffset;


#if defined (FSR_LLD_STRICT_CHK)
            nLLDRe = _StrictChk(nDev, nPbn, nPgOffset);

            if (nLLDRe != FSR_LLD_SUCCESS)
            {
                break;
            }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */

            nDie = nPbn >> (FSR_OND_4K_DFS_BASEBIT - pstOND4kCxt->nDDPSelSft);

            nFlushOpCaller = FSR_OND_4K_PREOP_CPBK_LOAD << FSR_OND_4K_FLUSHOP_CALLER_BASEBIT;

            /* OneNAND doesn't support read interleaving,
               therefore, wait until interrupt status is ready for both die. 
               if device is DDP. */
            if (pstOND4kSpec->nNumOfDies == FSR_MAX_DIES)
            {
                FSR_OND_4K_FlushOp(nDev,
                                nFlushOpCaller | ((nDie + 0x1) & 0x1),
                                nFlag | FSR_LLD_FLAG_REMAIN_PREOP_STAT);
            }

            nLLDRe = FSR_OND_4K_FlushOp(nDev, nFlushOpCaller | nDie, nFlag);
            if ((FSR_RETURN_MAJOR(nLLDRe) != FSR_LLD_SUCCESS) &&
                (FSR_RETURN_MAJOR(nLLDRe) != FSR_LLD_PREV_READ_DISTURBANCE))
            {
                break;
            }

            
            /* set DBS */
            OND_4K_WRITE(pstFOReg->nStartAddr2,
                (UINT16) (nDie << FSR_OND_4K_DBS_BASEBIT));
                
            /* set DFS & FBA */
            OND_4K_WRITE(pstFOReg->nStartAddr1,
                (UINT16) ((nDie << FSR_OND_4K_DFS_BASEBIT) | (nPbn & pstOND4kCxt->nFBAMask)));

            /* set FPA (Flash Page Address) */
            OND_4K_WRITE(pstFOReg->nStartAddr8,
                      (UINT16) nPgOffset << pstOND4kCxt->nFPASelSft);

            /* set Start Buffer Register */
            OND_4K_WRITE(pstFOReg->nStartBuf, FSR_OND_4K_START_BUF_DEFAULT);

            if ((nFlag & FSR_LLD_FLAG_ECC_MASK) == FSR_LLD_FLAG_ECC_ON)
            {
                nSysConf1 = OND_4K_READ(pstFOReg->nSysConf1);
                if (nSysConf1 & FSR_OND_4K_CONF1_ECC_OFF)
                {
                    OND_4K_CLR(pstFOReg->nSysConf1, FSR_OND_4K_CONF1_ECC_ON);
                }
            }
            else
            {
                OND_4K_SET(pstFOReg->nSysConf1, FSR_OND_4K_CONF1_ECC_OFF);
            }


            /* in case of non-blocking mode, interrupt should be enabled */
            if (nFlag & FSR_LLD_FLAG_INT_MASK)
            {
                FSR_OAM_ClrNEnableInt(pstOND4kCxt->nIntID);
            }
            else
            {
                FSR_OAM_ClrNDisableInt(pstOND4kCxt->nIntID);
            }


            OND_4K_WRITE(pstFOReg->nCmd, (UINT16) gnCpBkCmdArray[nCmdIdx]);


#if defined (FSR_LLD_STATISTICS)

            _AddOND4kStatLoad(nDev,
                            nDie,
                            nPbn,
                            FSR_OND_4K_STAT_NORMAL_CMD);

#endif /* #if defined (FSR_LLD_STATISTICS) */

            pstOND4kShMem->nPreOp[nDie]         = FSR_OND_4K_PREOP_CPBK_LOAD;
        }
        else if (nCmdIdx == FSR_LLD_FLAG_1X_CPBK_PROGRAM)
        {
            FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_INF,
            (TEXT("[OND:INF]   %s(nDev:%d, nDstPbn:%d, nDstPg:%d, nRndInCnt:%d, nFlag:0x%08x)\r\n"),
            __FSR_FUNC__, nDev, pstCpArg->nDstPbn, pstCpArg->nDstPgOffset, pstCpArg->nRndInCnt, nFlag));

            nPbn      = pstCpArg->nDstPbn;
            nPgOffset = pstCpArg->nDstPgOffset;

            nDie = nPbn >> (FSR_OND_4K_DFS_BASEBIT - pstOND4kCxt->nDDPSelSft);

#if defined (FSR_LLD_STRICT_CHK)
            nLLDRe = _StrictChk(nDev, nPbn, nPgOffset);

            if (nLLDRe != FSR_LLD_SUCCESS)
            {
                break;
            }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */

            nFlushOpCaller = FSR_OND_4K_PREOP_CPBK_PGM << FSR_OND_4K_FLUSHOP_CALLER_BASEBIT;

            nLLDRe = FSR_OND_4K_FlushOp(nDev, nFlushOpCaller | nDie, nFlag);

            if ((FSR_RETURN_MAJOR(nLLDRe) != FSR_LLD_SUCCESS) &&
                (FSR_RETURN_MAJOR(nLLDRe) != FSR_LLD_PREV_READ_DISTURBANCE))
            {
                break;
            }

            /* set DBS */
            OND_4K_WRITE(pstFOReg->nStartAddr2,
                (UINT16) (nDie << FSR_OND_4K_DBS_BASEBIT));


            bSpareBufRndIn = FALSE32;
            for (nCnt = 0; nCnt < pstCpArg->nRndInCnt; nCnt++)
            {
                pstRIArg = pstCpArg->pstRndInArg + nCnt;
                if (pstRIArg->nOffset >= FSR_LLD_CPBK_SPARE)
                {
                    bSpareBufRndIn = TRUE32;

                    stSpareBuf.pstSpareBufBase = &stSpareBufBase;
                    stSpareBuf.nNumOfMetaExt = 2;
                    stSpareBuf.pstSTLMetaExt = &stSpareBufExt[0];

                    nLLDRe = _ReadSpare(&stSpareBuf, &pstFOReg->nDataSB00[0], nFlag);

                    break;
                }
            }

            for (nCnt = 0; nCnt < pstCpArg->nRndInCnt; nCnt++)
            {
                pstRIArg = pstCpArg->pstRndInArg + nCnt;

                /* in case copyback of spare area is requested */
                if (pstRIArg->nOffset >= FSR_LLD_CPBK_SPARE)
                {
                    nRndOffset = pstRIArg->nOffset - FSR_LLD_CPBK_SPARE;

                    if (nRndOffset >= (FSR_SPARE_BUF_BASE_SIZE))
                    {
                        /* random-in to FSRSpareBufExt[] */
                        nRndOffset -= (FSR_SPARE_BUF_BASE_SIZE * 1);

                        FSR_ASSERT((pstRIArg->nNumOfBytes + nRndOffset) <= (FSR_SPARE_BUF_EXT_SIZE * FSR_MAX_SPARE_BUF_EXT));

                        /* random-in to FSRSpareBuf */
                        FSR_OAM_MEMCPY((UINT8 *) &(stSpareBuf.pstSTLMetaExt[0]) + nRndOffset,
                                       (UINT8 *) pstRIArg->pBuf,
                                        pstRIArg->nNumOfBytes);
                    }
                    else
                    {
                        FSR_ASSERT((pstRIArg->nNumOfBytes + nRndOffset) <= FSR_SPARE_BUF_BASE_SIZE);

                        /* random-in to FSRSpareBuf */
                        FSR_OAM_MEMCPY((UINT8 *)(stSpareBuf.pstSpareBufBase) + nRndOffset,
                                       (UINT8 *) pstRIArg->pBuf,
                                        pstRIArg->nNumOfBytes);
                    }
#if defined (FSR_LLD_STATISTICS)
                    nBytesTransferred += pstRIArg->nNumOfBytes;
#endif /* #if defined (FSR_LLD_STATISTICS) */
                }
                else
                {
                    _WriteMain(&pstFOReg->nDataMB00[0] + pstRIArg->nOffset,
                               (UINT8 *) pstRIArg->pBuf,
                               pstRIArg->nNumOfBytes);

#if defined (FSR_LLD_STATISTICS)
                    nBytesTransferred += pstRIArg->nNumOfBytes;
#endif /* #if defined (FSR_LLD_STATISTICS) */
                }
            }

            if (bSpareBufRndIn == TRUE32)
            {
                _WriteSpare(&pstFOReg->nDataSB00[0],
                            &stSpareBuf,
                            nFlag | FSR_LLD_FLAG_USE_SPAREBUF);
            }

            /* set DFS & FBA */
            OND_4K_WRITE(pstFOReg->nStartAddr1,
                (UINT16) ((nDie << FSR_OND_4K_DFS_BASEBIT) | (nPbn & pstOND4kCxt->nFBAMask)));

#if defined(FSR_LLD_WAIT_WR_PROTECT_STAT)
            do
            {
                nWrProtectStat = OND_4K_READ(pstFOReg->nWrProtectStat);
            } while (nWrProtectStat == (UINT16) 0x0000);
#else
            nWrProtectStat = OND_4K_READ(pstFOReg->nWrProtectStat);
#endif

            /* write protection can be checked at this point */
            if ((nWrProtectStat & FSR_LLD_BLK_STAT_MASK) != FSR_LLD_BLK_STAT_UNLOCKED)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[OND:ERR]   %s(nDev:%d, nDstPbn:%d, nDstPg:%d, nFlag:0x%08x) / %d line\r\n"),
                    __FSR_FUNC__, nDev, pstCpArg->nDstPbn,
                    pstCpArg->nDstPgOffset, nFlag,  __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            Pbn:%d is write protected\r\n"), nPbn));

                _DumpRegisters(pstFOReg);
                _DumpSpareBuffer(pstFOReg);

                nLLDRe = (FSR_LLD_WR_PROTECT_ERROR | FSR_LLD_1STPLN_CURR_ERROR);
                break;
            }

            /* set FPA (Flash Page Address) */
            OND_4K_WRITE(pstFOReg->nStartAddr8,
                (UINT16) nPgOffset << pstOND4kCxt->nFPASelSft);

            /* set Start Buffer Register */
            OND_4K_WRITE(pstFOReg->nStartBuf, FSR_OND_4K_START_BUF_DEFAULT);

            if ((nFlag & FSR_LLD_FLAG_ECC_MASK) == FSR_LLD_FLAG_ECC_ON)
            {
                OND_4K_CLR(pstFOReg->nSysConf1, FSR_OND_4K_CONF1_ECC_ON);
            }
            else
            {
                OND_4K_SET(pstFOReg->nSysConf1, FSR_OND_4K_CONF1_ECC_OFF);
            }

            /* in case of non-blocking mode, interrupt should be enabled */
            if (nFlag & FSR_LLD_FLAG_INT_MASK)
            {
                FSR_OAM_ClrNEnableInt(pstOND4kCxt->nIntID);
            }
            else
            {
                FSR_OAM_ClrNDisableInt(pstOND4kCxt->nIntID);
            }


            OND_4K_WRITE(pstFOReg->nCmd, (UINT16) gnCpBkCmdArray[nCmdIdx]);


#if defined (FSR_LLD_STATISTICS)
            _AddOND4kStat(nDev,
                        nDie,
                        FSR_OND_4K_STAT_WR_TRANS,
                        nBytesTransferred,
                        FSR_OND_4K_STAT_NORMAL_CMD);

            _AddOND4kStatPgm(nDev,
                        nDie,
                        nPbn,
                        nPgOffset,
                        FSR_OND_4K_STAT_NORMAL_CMD);
#endif /* #if defined (FSR_LLD_STATISTICS) */

            /* store the type of previous operation for the deferred check */
            pstOND4kShMem->nPreOp[nDie]                 = FSR_OND_4K_PREOP_CPBK_PGM;

#if defined (FSR_LLD_HANDSHAKE_ERR_INF)
            pstOND4kSharedCxt = &gstOND4kSharedCxt[nDev];
            pstOND4kSharedCxt->nPrevFSRMode[nDie]     = nFlag & FSR_LLD_FLAG_READ_ONLY;
            pstOND4kSharedCxt->nHostLLDOp[nDie]       = FSR_OND_4K_PREOP_CPBK_PGM;

            pstOND4kSharedCxt->nHostLLDPbn[nDie]      = (UINT16) nPbn;
            pstOND4kSharedCxt->nHostLLDPgOffset[nDie] = (UINT16) nPgOffset;
            pstOND4kSharedCxt->nHostLLDFlag[nDie]     = nFlag;
#endif
        }

        pstOND4kShMem->nPreOpPbn[nDie]      = (UINT16) nPbn;
        pstOND4kShMem->nPreOpPgOffset[nDie] = (UINT16) nPgOffset;
        pstOND4kShMem->nPreOpFlag[nDie]     = nFlag;

#if defined (FSR_LLD_LOGGING_HISTORY)
        _AddLog(nDev, nDie);
#endif
    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return (nLLDRe);
}



/**
 * @brief           This function checks whether block is bad or not
 *
 * @param[in]       nDev        : Physical Device Number (0 ~ 3)
 * @param[in]       nPbn        : physical block number
 * @param[in]       nFlag       : FSR_LLD_FLAG_1X_CHK_BADBLOCK
 *
 * @return          FSR_LLD_INIT_GOODBLOCK
 * @return          FSR_LLD_INIT_BADBLOCK | {FSR_LLD_BAD_BLK_1STPLN}
 * @return          FSR_LLD_INVALID_PARAM
 *
 * @author          NamOh Hwang / Kyungho Shin
 * @version         1.1.0
 * @remark          Pbn is consecutive numbers within the device
 * @n               i.e. for DDP device, Pbn for 1st block in 2nd chip is not 0
 * @n               it is one more than the last block number in 1st chip
 *
 */
PUBLIC INT32
FSR_OND_4K_ChkBadBlk(UINT32 nDev,
                     UINT32 nPbn,
                     UINT32 nFlag)
{
             OneNAND4kCxt       *pstOND4kCxt  = NULL;
    volatile OneNAND4kReg   *pstFOReg   = NULL;

             INT32             nLLDRe = FSR_LLD_INIT_GOODBLOCK;
             UINT16            nDQ;

    FSR_STACK_VAR;

    FSR_STACK_END;



    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:IN ] ++%s(nDev:%d, nPbn:%d, nFlag:0x%08x)\r\n"),
        __FSR_FUNC__, nDev, nPbn, nFlag));

    do
    {

#if defined (FSR_LLD_STRICT_CHK)
        /* check device number */
        if (nDev >= FSR_OND_4K_MAX_DEVS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s(nDev:%d, nPbn:%d, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, nPbn, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Invalid Device Number (nDev = %d)\r\n"),
                nDev));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }
#endif

        pstOND4kCxt  = gpstOND4kCxt[nDev];
        pstFOReg   = (volatile OneNAND4kReg *) pstOND4kCxt->nBaseAddr;

        nLLDRe = FSR_OND_4K_ReadOptimal(nDev,   /* Device Number                          */
                      nPbn,                  /* Physical Block Number                  */
                      (UINT32) 0,            /* page offset to be read                 */
                      (UINT8 *) NULL,        /* Buffer pointer for Main area           */
                      (FSRSpareBuf*) NULL,   /* Buffer pointer for Spare area          */
                      (UINT32) (FSR_LLD_FLAG_ECC_OFF | FSR_LLD_FLAG_1X_LOAD));/* flag  */

        if (FSR_RETURN_MAJOR(nLLDRe) == FSR_LLD_INVALID_PARAM)
        {
            break;
        }

        nLLDRe = FSR_OND_4K_ReadOptimal(nDev,   /* Device Number                          */
                      nPbn,                  /* Physical Block Number                  */
                      (UINT32) 0,            /* page offset to be read                 */
                      (UINT8 *) NULL,        /* Buffer pointer for Main area           */
                      (FSRSpareBuf *) NULL,  /* Buffer pointer for Spare area          */
                      (UINT32) (FSR_LLD_FLAG_ECC_OFF | FSR_LLD_FLAG_TRANSFER));/* flag */

        if (FSR_RETURN_MAJOR(nLLDRe) == FSR_LLD_INVALID_PARAM)
        {
            break;
        }

        nDQ = OND_4K_READ(*(volatile UINT16 *) &pstFOReg->nDataSB00[0]);

        if (nDQ != (UINT16) FSR_OND_4K_VALID_BLK_MARK)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR | FSR_DBZ_LLD_INF,
            (TEXT("[OND:INF]   %s(nDev:%d, nPbn:%d, nFlag:0x%08x) / %d line\r\n"),
            __FSR_FUNC__, nDev, nPbn, nFlag, __LINE__));


            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR | FSR_DBZ_LLD_INF,
            (TEXT("            nPbn = %d is a bad block\r\n"), nPbn));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR | FSR_DBZ_LLD_INF,
            (TEXT("            bad mark: 0x%04x\r\n"), nDQ));

            nLLDRe = FSR_LLD_INIT_BADBLOCK | FSR_LLD_BAD_BLK_1STPLN;
        }
        else
        {
            FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_INF,
            (TEXT("[OND:INF]   %s(nDev:%d, nPbn:%d, nFlag:0x%08x) / %d line\r\n"),
            __FSR_FUNC__, nDev, nPbn, nFlag, __LINE__));

            FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_INF,
            (TEXT("            nPbn = %d is a good block\r\n"), nPbn));

            nLLDRe = FSR_LLD_INIT_GOODBLOCK;
        }

    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return (nLLDRe);
}

/**
 * @brief           this function flush previous operation
 *
 * @param[in]       nDev         : Physical Device Number (0 ~ 3)
 * @param[in]       nDie         : 0 is for 1st die
 * @n                            : 1 is for 2nd die
 * @param[in]       nFlag        :
 *
 * @return          FSR_LLD_SUCCESS
 * @return          FSR_LLD_PREV_WRITE_ERROR | {FSR_LLD_1STPLN_PREV_ERROR | FSR_LLD_1STPLN_CURR_ERROR}
 * @return          FSR_LLD_PREV_ERASE_ERROR | {FSR_LLD_1STPLN_CURR_ERROR}
 * @return          FSR_LLD_INVALID_PARAM
 *
 * @author          NamOh Hwang / Kyungho Shin
 * @version         1.1.0
 * @remark          this function completes previous operation,
 * @n               and returns error for previous operation
 * @n               After calling series of FSR_OND_4K_Write() or FSR_OND_4K_Erase() or
 * @n               FSR_OND_4K_CopyBack(), FSR_OND_4K_FlushOp() needs to be called.
 *
 */
PUBLIC INT32
FSR_OND_4K_FlushOp(UINT32 nDev,
                   UINT32 nDie,
                   UINT32 nFlag)
{
             OneNAND4kCxt       *pstOND4kCxt  = NULL;
             OneNAND4kShMem     *pstOND4kShMem= NULL;
    volatile OneNAND4kReg   *pstFOReg   = NULL;
#if defined (FSR_LLD_HANDSHAKE_ERR_INF)
    volatile OneNAND4kSharedCxt *pstOND4kSharedCxt;
#endif

             UINT32            nIdx;
             UINT32            nPrevOp;
#if !defined(FSR_OAM_RTLMSG_DISABLE)
             UINT32            nPrevPbn;
             UINT32            nPrevPgOffset;
#endif
             UINT32            nPrevFlag;


              INT32            nLLDRe     = FSR_LLD_SUCCESS;
             UINT16            nMasterInt;
             UINT16            nECCRes; /* ECC result */

    FSR_STACK_VAR;

    FSR_STACK_END;



    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:IN ] ++%s(nDev: %d, nDieIdx: %d, nFlag: 0x%08x)\r\n"),
        __FSR_FUNC__, nDev, nDie, nFlag));

    do
    {

#if defined (FSR_LLD_STRICT_CHK)
        /* check device number */
        if (nDev >= FSR_OND_4K_MAX_DEVS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("[OND:ERR]   %s(nDev:%d, nDie:%d, nFlag:0x%08x) / %d line\r\n"),
            __FSR_FUNC__, nDev, nDie, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("            Invalid Device Number (nDev = %d)\r\n"), nDev));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }
#endif

        pstOND4kCxt  = gpstOND4kCxt[nDev];
        pstOND4kShMem= gpstOND4kShMem[nDev];
        pstFOReg   = (volatile OneNAND4kReg *) pstOND4kCxt->nBaseAddr;

        pstOND4kCxt->nFlushOpCaller = (UINT16) (nDie >> FSR_OND_4K_FLUSHOP_CALLER_BASEBIT);
        nDie = nDie & ~FSR_OND_4K_FLUSHOP_CALLER_MASK;

        nPrevOp       = pstOND4kShMem->nPreOp[nDie];
#if !defined(FSR_OAM_RTLMSG_DISABLE)
        nPrevPbn      = pstOND4kShMem->nPreOpPbn[nDie];
        nPrevPgOffset = pstOND4kShMem->nPreOpPgOffset[nDie];
#endif
        nPrevFlag     = pstOND4kShMem->nPreOpFlag[nDie];



#if defined (FSR_LLD_HANDSHAKE_ERR_INF)

        pstOND4kSharedCxt = &gstOND4kSharedCxt[nDev];

        /* mode has changed between Tiny FSR & normal FSR (linux only) */
        if ((nFlag & FSR_LLD_FLAG_READ_ONLY) !=
            (pstOND4kSharedCxt->nPrevFSRMode[nDie] & FSR_LLD_FLAG_READ_ONLY))
        {
            if (nFlag & FSR_LLD_FLAG_READ_ONLY)
            {
                nPrevOp       = pstOND4kSharedCxt->nHostLLDOp[nDie];
#if !defined(FSR_OAM_RTLMSG_DISABLE)
                nPrevPbn      = pstOND4kSharedCxt->nHostLLDPbn[nDie];
                nPrevPgOffset = pstOND4kSharedCxt->nHostLLDPgOffset[nDie];
#endif
                nPrevFlag     = pstOND4kSharedCxt->nHostLLDFlag[nDie];
            }
            /* if Erase, Pgm Error was saved in Tiny FSR path,
             * restore the error context, and return the error
             */
            else /* if (!(nFlag & FSR_LLD_FLAG_READ_ONLY)) */
            {
                nLLDRe = _CheckErrorCxt(nDev, nDie, pstOND4kCxt, pstFOReg);
                nPrevOp = FSR_OND_4K_PREOP_NONE;
            }
        }

#endif


        /* set DBS */
        OND_4K_WRITE(pstFOReg->nStartAddr2, (UINT16) (nDie << FSR_OND_4K_DBS_BASEBIT));


        switch (nPrevOp)
        {
        case FSR_OND_4K_PREOP_NONE:
            /* DO NOT remove this code : 'case FSR_OND_PREOP_NONE:'
               for compiler optimization */
            break;

        case FSR_OND_4K_PREOP_READ:
        case FSR_OND_4K_PREOP_CPBK_LOAD:
            WAIT_OND_4K_INT_STAT(pstFOReg, FSR_OND_4K_INT_READ_READY);

            if ((nPrevFlag & FSR_LLD_FLAG_ECC_MASK) == FSR_LLD_FLAG_ECC_ON)
            {
                /* uncorrectable read error */
                if (OND_4K_READ(pstFOReg->nCtrlStat) & FSR_OND_4K_STATUS_ERROR)
                {
                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                        (TEXT("[OND:ERR]   %s(nDev:%d, nDie:%d, nFlag:0x%08x) / %d line\r\n"),
                        __FSR_FUNC__, nDev, nDie, nFlag, __LINE__));

                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                        (TEXT("            pstOND4kCxt->nFlushOpCaller : %d\r\n"),
                        pstOND4kCxt->nFlushOpCaller));

                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                        (TEXT("            ECC Status : Uncorrectable, CtrlReg=0x%04x\r\n"),
                        OND_4K_READ(pstFOReg->nCtrlStat)));

                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                        (TEXT("            at nDev:%d, nPbn:%d, nPgOffset:%d, nFlag:0x%08x\r\n"),
                        nDev, nPrevPbn, nPrevPgOffset, nPrevFlag));

                    _DumpRegisters(pstFOReg);
                    _DumpSpareBuffer(pstFOReg);

                    nLLDRe = (FSR_LLD_PREV_READ_ERROR |
                              FSR_LLD_1STPLN_CURR_ERROR);
                }
                /* correctable error */
                else
                {
                    nIdx = FSR_OND_4K_MAX_ECC_STATUS_REG - 1;
                    do
                    {
                        nECCRes = OND_4K_READ(pstFOReg->nEccStat[nIdx]);

                        if (nECCRes & FSR_OND_4K_ECC_READ_DISTURBANCE)
                        {
                            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
                                (TEXT("[OND:INF]   %s(nDev:%d, nDie:%d, nFlag:0x%08x) / %d line\r\n"),
                                __FSR_FUNC__, nDev, nDie, nFlag, __LINE__));

                            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
                                (TEXT("            pstOND4kCxt->nFlushOpCaller : %d\r\n"),
                                pstOND4kCxt->nFlushOpCaller));

                            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
                                (TEXT("            read disturbance at nPbn:%d, nPgOffset:%d, nFlag:0x%08x\r\n"),
                                nPrevPbn, nPrevPgOffset, nPrevFlag));

                            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
                                (TEXT("            Detected at ECC Register[%d]:0x%04x\r\n"),
                                nIdx, OND_4K_READ(pstFOReg->nEccStat[nIdx])));

                            _DumpRegisters(pstFOReg);
                            _DumpSpareBuffer(pstFOReg);

#if defined(FSR_LLD_PE_TEST)
                            gnECCStat0 = OND_4K_READ(pstFOReg->nEccStat[0]);
                            gnECCStat1 = OND_4K_READ(pstFOReg->nEccStat[1]);
                            gnECCStat2 = OND_4K_READ(pstFOReg->nEccStat[2]);
                            gnECCStat3 = OND_4K_READ(pstFOReg->nEccStat[3]);
                            
                            gnPbn     = nPrevPbn;
                            gnPgOffset = nPrevPgOffset;
#endif

                            nLLDRe = FSR_LLD_PREV_READ_DISTURBANCE |
                                     FSR_LLD_1STPLN_CURR_ERROR;
                            break;
                        }
                        /* else case
                         * no error, 1 ~ 2 bit error
                         */
                    } while (nIdx-- > 0);
                }
            }

            break;

        case FSR_OND_4K_PREOP_PROGRAM:
        case FSR_OND_4K_PREOP_CACHE_PGM:
        case FSR_OND_4K_PREOP_CPBK_PGM:
            /* if the the current operation is cache program operation or 
                                               interleave cache program,
               master interrupt & write interrupt bit should be checked.
               even though the last command is program command.
               -----------------------------------------------------------
               cache program operation is as follows
               1. cache_pgm
               2. cache_pgm
               3. program
               
            */
            if (nPrevOp == FSR_OND_4K_PREOP_CACHE_PGM)
            {
                nMasterInt = FSR_OND_4K_INT_MASTER_READY;
            }
            else
            {
                nMasterInt = 0;
            }

            WAIT_OND_4K_INT_STAT(pstFOReg, nMasterInt | FSR_OND_4K_INT_WRITE_READY);

            /* previous error check
             * in case of Cache Program, Error bit shows the accumulative
             * error status of Cache Program
             * so if an error occurs this bit stays as fail 
             */
            if (OND_4K_READ(pstFOReg->nCtrlStat) & FSR_OND_4K_STATUS_ERROR)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s(nDev:%d, nDie:%d, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, nDie, nFlag, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            pstOND4kCxt->nFlushOpCaller : %d\r\n"),
                    pstOND4kCxt->nFlushOpCaller));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            write() error: CtrlReg=0x%04x\r\n"),
                    OND_4K_READ(pstFOReg->nCtrlStat) ));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            last write() call @ nDev = %d, nPbn = %d, nPgOffset = %d, nFlag:0x%08x\r\n"),
                    nDev, nPrevPbn, nPrevPgOffset, nPrevFlag));

                _DumpRegisters(pstFOReg);
                _DumpSpareBuffer(pstFOReg);

                nLLDRe = FSR_LLD_PREV_WRITE_ERROR;

                if (!(OND_4K_READ(pstFOReg->nCtrlStat) &
                      (FSR_OND_4K_CURR_CACHEPGM_ERROR | FSR_OND_4K_PREV_CACHEPGM_ERROR)))
                {
                    nLLDRe |= FSR_LLD_1STPLN_CURR_ERROR;
                }

                if (OND_4K_READ(pstFOReg->nCtrlStat) & FSR_OND_4K_CURR_CACHEPGM_ERROR)
                {
                    nLLDRe |= FSR_LLD_1STPLN_CURR_ERROR;
                }

                if (OND_4K_READ(pstFOReg->nCtrlStat) & FSR_OND_4K_PREV_CACHEPGM_ERROR)
                {
                    nLLDRe |= FSR_LLD_1STPLN_PREV_ERROR;
                }
            }

            break;

        case FSR_OND_4K_PREOP_ERASE:

            WAIT_OND_4K_INT_STAT(pstFOReg, FSR_OND_4K_INT_ERASE_READY);

            /* previous error check */
            if ((OND_4K_READ(pstFOReg->nCtrlStat) & FSR_OND_4K_STATUS_ERROR) ==
                FSR_OND_4K_STATUS_ERROR)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s(nDev:%d, nDie:%d, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, nDie, nFlag, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            pstOND4kCxt->nFlushOpCaller : %d\r\n"),
                    pstOND4kCxt->nFlushOpCaller));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            erase error: CtrlReg=0x%04x\r\n"),
                    OND_4K_READ(pstFOReg->nCtrlStat) ));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            last Erase() call @ nDev = %d, nPbn = %d, nFlag:0x%08x\r\n"),
                    nDev, nPrevPbn, nPrevFlag));

                _DumpRegisters(pstFOReg);
                _DumpSpareBuffer(pstFOReg);

                nLLDRe = (FSR_LLD_PREV_ERASE_ERROR | FSR_LLD_1STPLN_CURR_ERROR);
            }
            break;

        default:
            WAIT_OND_4K_INT_STAT(pstFOReg, FSR_OND_4K_INT_MASTER_READY);
            break;
        }

#if defined (FSR_LLD_HANDSHAKE_ERR_INF)
        /* when this code is running for Tiny FSR and
         * there is an error for Erase, Pgm, save the error
         */
        if ((nFlag & FSR_LLD_FLAG_READ_ONLY) && (nLLDRe != FSR_LLD_SUCCESS))
        {
            _SaveErrorCxt(nDev, nDie, pstOND4kCxt, nLLDRe);

            nLLDRe = FSR_LLD_SUCCESS;
        }
#endif

#if defined (FSR_LLD_STATISTICS)
    _AddOND4kStat(nDev, nDie, FSR_OND_4K_STAT_FLUSH, 0, FSR_OND_4K_STAT_NORMAL_CMD);
#endif

    } while (0);


    if (FSR_RETURN_MAJOR(nLLDRe) != FSR_LLD_INVALID_PARAM)
    {
        if (!(nFlag & FSR_LLD_FLAG_REMAIN_PREOP_STAT))
        {
            pstOND4kShMem->nPreOp[nDie] = FSR_OND_4K_PREOP_NONE;
        }

#if defined (FSR_LLD_HANDSHAKE_ERR_INF)
        if (!(nFlag & FSR_LLD_FLAG_READ_ONLY))
        {
            gstOND4kSharedCxt[nDev].nHostLLDOp[nDie] = FSR_OND_4K_PREOP_NONE;
        }

        /* save the mode (Tiny FSR or FSR) */
        gstOND4kSharedCxt[nDev].nPrevFSRMode[nDie] = nFlag & FSR_LLD_FLAG_READ_ONLY;
#endif
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return (nLLDRe);
}



/**
 * @brief           this function provides block information.
 *
 * @param[in]       nDev        : Physical Device Number
 * @param[in]       nPbn        : Physical Block  Number
 * @param[out]      pBlockType  : whether nPbn is SLC block or MLC block
 * @param[out]      pPgsPerBlk  : the number of pages per block
 *
 * @return          FSR_LLD_SUCCESS
 * @n               FSR_LLD_INVALID_PARAM
 *
 * @author          NamOh Hwang / Kyungho Shin
 * @version         1.1.0
 * @remark          i.e. SLC or MLC block, the number of pages per block
 *
 */
PUBLIC INT32
FSR_OND_4K_GetBlockInfo(UINT32     nDev,
                        UINT32     nPbn,
                        UINT32    *pBlockType,
                        UINT32    *pPgsPerBlk)
{
    OneNAND4kCxt       *pstOND4kCxt  = NULL;
    OneNAND4kSpec      *pstOND4kSpec = NULL;

    UINT32            nDie;
    INT32             nLLDRe     = FSR_LLD_SUCCESS;

    FSR_STACK_VAR;

    FSR_STACK_END;



    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:IN ] ++%s(nDev:%d, nPbn:%d)\r\n"),
        __FSR_FUNC__, nDev, nPbn));

    do
    {
#if defined (FSR_LLD_STRICT_CHK)
        nLLDRe = _StrictChk(nDev, nPbn, 0);

        if (nLLDRe != FSR_LLD_SUCCESS)
        {
            break;
        }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */

        pstOND4kCxt  = gpstOND4kCxt[nDev];
        pstOND4kSpec = pstOND4kCxt->pstOND4kSpec;


        nDie = nPbn >> (FSR_OND_4K_DFS_BASEBIT - pstOND4kCxt->nDDPSelSft);

        if ((nPbn & pstOND4kCxt->nFBAMask) < pstOND4kCxt->nBlksForSLCArea[nDie])
        {
            /* this block is SLC block */
            if (pBlockType != NULL)
            {
                *pBlockType = FSR_LLD_SLC_BLOCK;
            }
            if (pPgsPerBlk != NULL)
            {
                *pPgsPerBlk = pstOND4kSpec->nPgsPerBlkForSLC;
            }
        }
    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return nLLDRe;
}



/**
 * @brief           This function reports device information to upper layer.
 *
 * @param[in]       nDev        : Physical Device Number (0 ~ 3)
 * @param[out]      pstDevSpec  : pointer to the device spec
 * @param[in]       nFlag       :
 *
 * @return         FSR_LLD_SUCCESS
 * @return         FSR_LLD_OPEN_FAILURE
 *
 * @author          NamOh Hwang / Kyungho Shin
 * @version         1.1.0
 * @remark
 *
 */
PUBLIC INT32
FSR_OND_4K_GetDevSpec(UINT32      nDev,
                      FSRDevSpec *pstDevSpec,
                      UINT32      nFlag)
{
                OneNAND4kCxt     *pstOND4kCxt  = NULL;
                OneNAND4kSpec    *pstOND4kSpec = NULL;
       volatile OneNAND4kReg *pstFOReg   = NULL;


                UINT32          nDieIdx;
                INT32           nLLDRe     = FSR_LLD_SUCCESS;

    FSR_STACK_VAR;

    FSR_STACK_END;



    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:IN ] ++%s(nDev:%d,pstDevSpec:0x%x,nFlag:0x%08x)\r\n"), 
        __FSR_FUNC__, nDev, pstDevSpec, nFlag));

    do 
    {

#if defined (FSR_LLD_STRICT_CHK)
        /* check device number */
        if (nDev >= FSR_OND_4K_MAX_DEVS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s(nDev:%d, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Invalid Device Number (nDev = %d)\r\n"),
                nDev));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }

        if (pstDevSpec == NULL)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s(nDev:%d, pstDevSpec:0x%08x, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, pstDevSpec, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            nDev:%d pstDevSpec is NULL\r\n"), nDev));

            nLLDRe = FSR_LLD_OPEN_FAILURE;
            break;
        }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */

        pstOND4kCxt  = gpstOND4kCxt[nDev];
        pstOND4kSpec = pstOND4kCxt->pstOND4kSpec;

        FSR_OAM_MEMSET(pstDevSpec, 0xFF, sizeof(FSRDevSpec));

        pstDevSpec->nNumOfBlks          = pstOND4kSpec->nNumOfBlks;
        pstDevSpec->nNumOfPlanes        = pstOND4kSpec->nNumOfPlanes;

        for (nDieIdx = 0; nDieIdx < pstOND4kSpec->nNumOfDies; nDieIdx++)
        {
            pstDevSpec->nBlksForSLCArea[nDieIdx] =
                pstOND4kCxt->nBlksForSLCArea[nDieIdx];
        }

        pstDevSpec->nSparePerSct        = pstOND4kSpec->nSparePerSct;
        pstDevSpec->nSctsPerPG          = pstOND4kSpec->nSctsPerPG;
        pstDevSpec->nNumOfBlksIn1stDie  =
            pstOND4kSpec->nNumOfBlks / pstOND4kSpec->nNumOfDies;

        /* read DID from register file.
         * because DID from pstOND4kSpec->nDID is masked with FSR_OND_4K_DID_MASK
         */
        pstFOReg = (volatile OneNAND4kReg *) pstOND4kCxt->nBaseAddr;
        pstDevSpec->nDID                = OND_4K_READ(pstFOReg->nDID);

        pstDevSpec->nPgsPerBlkForSLC    = pstOND4kSpec->nPgsPerBlkForSLC;
        pstDevSpec->nPgsPerBlkForMLC    = 0;
        pstDevSpec->nNumOfDies          = pstOND4kSpec->nNumOfDies;
        pstDevSpec->nUserOTPScts        = pstOND4kSpec->nUserOTPScts;
        pstDevSpec->b1stBlkOTP          = pstOND4kSpec->b1stBlkOTP;
        pstDevSpec->nRsvBlksInDev       = pstOND4kSpec->nRsvBlksInDev;
        pstDevSpec->pPairedPgMap        = NULL;

        pstDevSpec->nNANDType           = FSR_LLD_SLC_ONENAND;
        pstDevSpec->nPgBufToDataRAMTime = FSR_OND_4K_PAGEBUF_TO_DATARAM_TIME;

        pstDevSpec->bCachePgm           = pstOND4kCxt->bCachePgm;


        pstDevSpec->nSLCTLoadTime       = pstOND4kSpec->nSLCTLoadTime;
        pstDevSpec->nMLCTLoadTime       = 0;
        pstDevSpec->nSLCTProgTime       = pstOND4kSpec->nSLCTProgTime;
        pstDevSpec->nMLCTProgTime[0]    = 0;
        pstDevSpec->nMLCTProgTime[1]    = 0;
        pstDevSpec->nTEraseTime         = pstOND4kSpec->nTEraseTime;

        /* time for transfering 1 page in u sec */
        pstDevSpec->nWrTranferTime =
            (pstOND4kSpec->nSctsPerPG * FSR_OND_4K_SECTOR_SIZE + sizeof(FSRSpareBuf))
            * pstOND4kCxt->nWrTranferTime / 2 / 1000;

        pstDevSpec->nRdTranferTime =
            (pstOND4kSpec->nSctsPerPG * FSR_OND_4K_SECTOR_SIZE + sizeof(FSRSpareBuf))
            * pstOND4kCxt->nRdTranferTime / 2 / 1000;

        pstDevSpec->nSLCPECycle         = pstOND4kSpec->nSLCPECycle;
        pstDevSpec->nMLCPECycle         = 0;
        

        /* get UID from OTP block */
        nLLDRe = _GetUniqueID(nDev, pstOND4kCxt, nFlag);

        FSR_OAM_MEMCPY(&pstDevSpec->nUID[0], &pstOND4kCxt->nUID[0], FSR_LLD_UID_SIZE);

    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return (nLLDRe);
}



/**
 * @brief           this function provides access information 
 *
 * @param[in]       nDev        : Physical Device Number
 * @param[out]      pLLDPltInfo : structure for platform information.
 *
 * @return          FSR_LLD_SUCCESS
 * @n               FSR_LLD_INVALID_PARAM
 * @n               FSR_LLD_OPEN_FAILURE
 *
 * @author          NamOh Hwang / Kyungho Shin
 * @version         1.1.0
 * @remark
 *
 */
PUBLIC INT32
FSR_OND_4K_GetPlatformInfo(UINT32           nDev,
                           LLDPlatformInfo *pLLDPltInfo)
{
            OneNAND4kCxt     *pstOND4kCxt = NULL;
   volatile OneNAND4kReg *pstFOReg  = NULL;


            INT32           nLLDRe    = FSR_LLD_SUCCESS;


    FSR_STACK_VAR;

    FSR_STACK_END;



    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:IN ] ++%s(nDev:%d,pLLDPltInfo:0x%x)\r\n"), __FSR_FUNC__, nDev, pLLDPltInfo));

    do
    {
        if (pLLDPltInfo == NULL)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s() / %d line\r\n"),
                __FSR_FUNC__, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            pLLDPltInfo is NULL\r\n")));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }

#if defined (FSR_LLD_STRICT_CHK)
        /* check Device Number */
        if (nDev >= FSR_OND_4K_MAX_DEVS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s() / %d line \r\n"),
                __FSR_FUNC__, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Invalid Device Number (nDev = %d)\r\n"),
                nDev));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }
#endif

        pstOND4kCxt = gpstOND4kCxt[nDev];
        pstFOReg  = (volatile OneNAND4kReg *) pstOND4kCxt->nBaseAddr;


#if defined (FSR_LLD_STRICT_CHK)
        /* check Device Open Flag */
        if (pstOND4kCxt->bOpen == FALSE32)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s() / %d line\r\n"),
                __FSR_FUNC__, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Device is not opened\r\n")));

            nLLDRe = FSR_LLD_OPEN_FAILURE;
            break;
        }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */



        FSR_OAM_MEMSET(pLLDPltInfo, 0x00, sizeof(LLDPlatformInfo));

        /* Type of Device : 4K OneNAND = 0      */
        pLLDPltInfo->nType            = 0;
        /* Address of command register          */
        pLLDPltInfo->nAddrOfCmdReg    = (UINT32) &pstFOReg->nCmd;
        /* Address of address register          */
        pLLDPltInfo->nAddrOfAdrReg    = (UINT32) &pstFOReg->nStartAddr1;
        /* Address of register for reading ID   */
        pLLDPltInfo->nAddrOfReadIDReg = (UINT32) &pstFOReg->nMID;
        /* Address of status register           */
        pLLDPltInfo->nAddrOfStatusReg = (UINT32) &pstFOReg->nCtrlStat;
        /* Command of reading Device ID         */
        pLLDPltInfo->nCmdOfReadID     = (UINT32) NULL;
        /* Command of read page                 */
        pLLDPltInfo->nCmdOfReadPage   = (UINT32) NULL;
        /* Command of read status               */
        pLLDPltInfo->nCmdOfReadStatus = (UINT32) NULL;
        /* Mask value for Ready or Busy status  */
        pLLDPltInfo->nMaskOfRnB       = (UINT32) NULL;

    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return nLLDRe;
}



/**
 * @brief           this function reads data from DataRAM of 4K OneNAND
 *
 * @param[in]       nDev    : Physical Device Number (0 ~ 3)
 * @param[out]      pMBuf   : Memory buffer for main  array of NAND flash
 * @param[out]      pSBuf   : Memory buffer for spare array of NAND flash
 * @n               nDie    : 0 is for 1st die for DDP device
 * @n                       : 1 is for 2nd die for DDP device
 * @param[in]       nFlag   :
 *
 * @return          FSR_LLD_SUCCESS
 *
 * @author          NamOh Hwang / Kyungho Shin
 * @version         1.1.0
 * @remark          this function does not loads data from 4K OneNAND
 * @n               it just reads data which lies on DataRAM
 *
 */
PUBLIC INT32
FSR_OND_4K_GetPrevOpData(UINT32       nDev,
                         UINT8       *pMBuf,
                         FSRSpareBuf *pSBuf,
                         UINT32       nDie,
                         UINT32       nFlag)
{
             OneNAND4kCxt       *pstOND4kCxt  = NULL;
             OneNAND4kSpec      *pstOND4kSpec = NULL;
    volatile OneNAND4kReg   *pstFOReg   = NULL;

    INT32    nLLDRe = FSR_LLD_SUCCESS;

    FSR_STACK_VAR;

    FSR_STACK_END;



    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:IN ] ++%s() / nDie : %d / nFlag : 0x%x\r\n"), __FSR_FUNC__, nDie, nFlag));

    do
    {
#if defined (FSR_LLD_STRICT_CHK)
        /* check device number */
        if (nDev >= FSR_OND_4K_MAX_DEVS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s(nDev:%d, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Invalid Device Number (nDev = %d)\r\n"),
                nDev));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }

        /* nDie can be 0 (1st die) or 1 (2nd die) */
        if ((nDie & 0xFFFE) != 0)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s(nDev:%d, nDie:%d, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, nDie, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Invalid nDie Number (nDie = %d)\r\n"),
                nDie));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }
#endif

        pstOND4kCxt  = gpstOND4kCxt[nDev];
        pstOND4kSpec = pstOND4kCxt->pstOND4kSpec;
        pstFOReg   = (volatile OneNAND4kReg *) pstOND4kCxt->nBaseAddr;

        OND_4K_WRITE(pstFOReg->nStartAddr2, (UINT16) (nDie << FSR_OND_4K_DBS_BASEBIT));

        _ReadMain (pMBuf,
                   (volatile UINT8 *) &pstFOReg->nDataMB00[0],
                   pstOND4kSpec->nSctsPerPG * FSR_OND_4K_SECTOR_SIZE,
                   pstOND4kCxt->pTempBuffer);

        nLLDRe = _ReadSpare(pSBuf,
                           (volatile UINT8 *) &pstFOReg->nDataSB00[0],
                           nFlag);
    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s()\r\n"), __FSR_FUNC__));

    return nLLDRe;
}



/**
 * @brief           This function does PI operation, OTP operation,
 * @n               reset, write protection. 
 *
 * @param[in]       nDev        : Physical Device Number (0 ~ 3)
 * @param[in]       nCode       : IO Control Command
 * @n                             FSR_LLD_IOCTL_OTP_ACCESS
 * @n                             FSR_LLD_IOCTL_OTP_LOCK
 * @n                             in case IOCTL does with OTP protection,
 * @n                             pBufI indicates 1st OTP or OTP block or both
 * @n                             FSR_LLD_IOCTL_GET_OTPINFO
 * @n                             FSR_LLD_IOCTL_LOCK_TIGHT
 * @n                             FSR_LLD_IOCTL_LOCK_BLOCK
 * @n                             FSR_LLD_IOCTL_UNLOCK_BLOCK
 * @n                             FSR_LLD_IOCTL_UNLOCK_ALLBLK
 * @n                             FSR_LLD_IOCTL_GET_LOCK_STAT
 * @n                             FSR_LLD_IOCTL_HOT_RESET
 * @n                             FSR_LLD_IOCTL_CORE_RESET
 * @param[in]       pBufI       : Input Buffer pointer
 * @param[in]       nLenI       : Length of Input Buffer
 * @param[out]      pBufO       : Output Buffer pointer
 * @param[out]      nLenO       : Length of Output Buffer
 * @param[out]      pByteRet    : The number of bytes (length) of Output Buffer
 * @n                             as the result of function call
 *
 * @return          FSR_LLD_SUCCESS
 * @return          FSR_LLD_INVALID_PARAM
 * @return          FSR_LLD_IOCTL_NOT_SUPPORT
 * @return          FSR_LLD_WRITE_ERROR      | {FSR_LLD_1STPLN_CURR_ERROR}
 * @return          FSR_LLD_ERASE_ERROR      | {FSR_LLD_1STPLN_CURR_ERROR}
 * @return          FSR_LLD_WR_PROTECT_ERROR | {FSR_LLD_1STPLN_CURR_ERROR}
 * @return          FSR_LLD_PI_PROGRAM_ERROR
 * @return          FSR_LLD_PREV_READ_ERROR  | {FSR_LLD_1STPLN_CURR_ERROR }
 *
 * @author          NamOh Hwang / KyungHo Shin
 * @version         1.1.0
 * @remark          OTP read, write is performed with FSR_OND_4K_Write(), FSR_OND_4K_ReadOptimal(),
 * @n               after OTP Access
 *
 */
PUBLIC INT32
FSR_OND_4K_IOCtl(UINT32  nDev,
                 UINT32  nCode,
                 UINT8  *pBufI,
                 UINT32  nLenI,
                 UINT8  *pBufO,
                 UINT32  nLenO,
                 UINT32 *pByteRet)
{
              /* used to lock, unlock, lock-tight */
             LLDProtectionArg *pLLDProtectionArg;

             OneNAND4kCxt       *pstOND4kCxt  = NULL;
             OneNAND4kSpec      *pstOND4kSpec = NULL;
             OneNAND4kShMem     *pstOND4kShMem= NULL;
    volatile OneNAND4kReg   *pstFOReg   = NULL;

    volatile UINT32            nLockType;
             UINT32            nDie      = 0xFFFFFFFF;
             UINT32            nPbn;
             UINT32            nErrorPbn = 0;

             INT32             nLLDRe    = FSR_LLD_SUCCESS;

             UINT32            nLockValue;
             UINT16            nLockState;
             UINT16            nWrProtectStat;

    FSR_STACK_VAR;

    FSR_STACK_END;



    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:IN ] ++%s(nDev:%d, nCode:0x%08x)\r\n"),
        __FSR_FUNC__, nDev, nCode));

    do
    {

#if defined (FSR_LLD_STRICT_CHK)
        /* check device number */
        if (nDev >= FSR_OND_4K_MAX_DEVS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("[OND:ERR]   %s() / %d line\r\n"),
            __FSR_FUNC__, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("            Invalid Device Number (nDev = %d)\r\n"), nDev));

            nLLDRe = (FSR_LLD_INVALID_PARAM);
            break;
        }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */

        pstOND4kCxt  = gpstOND4kCxt[nDev];
        pstOND4kShMem= gpstOND4kShMem[nDev];
        pstOND4kSpec = pstOND4kCxt->pstOND4kSpec;
        pstFOReg   = (volatile OneNAND4kReg *) pstOND4kCxt->nBaseAddr;

        /* interrupt should be enabled in I/O Ctl */
        FSR_OAM_ClrNDisableInt(pstOND4kCxt->nIntID);

        switch (nCode)
        {
        case FSR_LLD_IOCTL_OTP_ACCESS:

            if ((pBufI == NULL) || (nLenI != sizeof(UINT32)))
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[OND:ERR]   %s() / %d line\r\n"),
                    __FSR_FUNC__, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            invalid parameter pBufI = 0x%08x, nLenI = %d\r\n"), pBufI, nLenI));

                nLLDRe = FSR_LLD_INVALID_PARAM;
                break;
            }

            nDie = *(UINT32 *) pBufI;

            FSR_ASSERT((nDie & ~0x1) == 0);


            OND_4K_WRITE(pstFOReg->nStartAddr2,
                (UINT16) (nDie << FSR_OND_4K_DBS_BASEBIT));
                
            OND_4K_WRITE(pstFOReg->nStartAddr1,
                (UINT16) (nDie << FSR_OND_4K_DFS_BASEBIT));
            
            
            /* issue OTP Access command */
            OND_4K_WRITE(pstFOReg->nCmd, FSR_OND_4K_CMD_OTP_ACCESS);


            WAIT_OND_4K_INT_STAT(pstFOReg, FSR_OND_4K_INT_MASTER_READY);

            if (pByteRet != NULL)
            {
                *pByteRet = (UINT32) 0;
            }
            break;


        case FSR_LLD_IOCTL_OTP_LOCK:
            if ((pBufI == NULL) || (nLenI != sizeof(UINT32)))
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[OND:ERR]   %s() / %d line\r\n"),
                    __FSR_FUNC__, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            invalid parameter pBufI = 0x%08x, nLenI = %d\r\n"), pBufI, nLenI));

                nLLDRe = FSR_LLD_INVALID_PARAM;
                break;
            }

            nLockType = *(UINT32 *) pBufI;

            /* lock 1st block OTP & OTP block */
            if (nLockType == (FSR_LLD_OTP_LOCK_1ST_BLK | FSR_LLD_OTP_LOCK_OTP_BLK))
            {
                nLockValue = FSR_OND_4K_LOCK_BOTH_OTP;
            }
            /* lock 1st block OTP only */
            else if (nLockType == FSR_LLD_OTP_LOCK_1ST_BLK)
            {
                nLockValue = FSR_OND_4K_LOCK_1ST_BLOCK_OTP;
            }
            /* lock OTP block only */
            else if (nLockType == FSR_LLD_OTP_LOCK_OTP_BLK)
            {
                nLockValue = FSR_OND_4K_LOCK_OTP_BLOCK;
            }
            else
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[OND:ERR]   %s() / %d line\r\n"),
                    __FSR_FUNC__, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            invalid parameter pBufI = 0x%08x, nLenI = %d\r\n"), pBufI, nLenI));

                nLLDRe = FSR_LLD_INVALID_PARAM;
                break;
            }

            nLLDRe = _LockOTP(nDev, nLockValue);

            nDie = 0;

            if (pByteRet != NULL)
            {
                *pByteRet = (UINT32) 0;
            }
            break;


        case FSR_LLD_IOCTL_OTP_GET_INFO:

            if ((pBufO == NULL) || (nLenO != sizeof(UINT32)))
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[OND:ERR]   %s() / %d line\r\n"),
                    __FSR_FUNC__, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            invalid parameter pBufO = 0x%08x, nLenO = %d\r\n"), pBufO, nLenO));

                nLLDRe = FSR_LLD_INVALID_PARAM;
                break;
            }

            nDie = 0;

            OND_4K_WRITE(pstFOReg->nStartAddr2,
                (UINT16) (nDie << FSR_OND_4K_DBS_BASEBIT));

            nLockType  = 0;

            nLockState = OND_4K_READ(pstFOReg->nCtrlStat);

            if (nLockState & FSR_OND_4K_CTLSTAT_1ST_OTP_LOCKED)
            {
                nLockType |= FSR_LLD_OTP_1ST_BLK_LOCKED;
            }
            else
            {
                nLockType |= FSR_LLD_OTP_1ST_BLK_UNLKED;
            }

            if (nLockState & FSR_OND_4K_CTLSTAT_OTP_BLK_LOCKED)
            {
                nLockType |= FSR_LLD_OTP_OTP_BLK_LOCKED;
            }
            else
            {
                nLockType |= FSR_LLD_OTP_OTP_BLK_UNLKED;
            }

            *(UINT32 *) pBufO = nLockType;
            nLenO             = sizeof(nLockType);

            nDie = 0;

            if (pByteRet != NULL)
            {
                *pByteRet = (UINT32) 0;
            }
            break;

        case FSR_LLD_IOCTL_LOCK_TIGHT:

            if ((pBufI == NULL) || (nLenI != sizeof(LLDProtectionArg)) ||
                (pBufO == NULL) || (nLenO != sizeof(nErrorPbn)))
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[OND:ERR]   %s() / %d line\r\n"),
                    __FSR_FUNC__, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            invalid parameter pBufI = 0x%08x, nLenI = %d\r\n"), pBufI, nLenI));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            invalid parameter pBufO = 0x%08x, nLenO = %d\r\n"), pBufO, nLenO));

                nLLDRe = FSR_LLD_INVALID_PARAM;
                break;
            }

            pLLDProtectionArg = (LLDProtectionArg *) pBufI;

            nDie = pLLDProtectionArg->nStartBlk >>
                (FSR_OND_4K_DFS_BASEBIT - pstOND4kCxt->nDDPSelSft);

            FSR_ASSERT((nDie & ~0x1) == 0);

            nLLDRe = _ControlLockBlk(nDev,
                                     pLLDProtectionArg->nStartBlk,
                                     pLLDProtectionArg->nBlks,
                                     (UINT32)FSR_OND_4K_CMD_LOCKTIGHT_BLOCK,
                                     &nErrorPbn);

            if (nLLDRe == FSR_LLD_INVALID_BLOCK_STATE)
            {
                *(UINT32 *) pBufO = nErrorPbn;

                if (pByteRet != NULL)
                {
                    *pByteRet = sizeof(nErrorPbn);
                }
            }
            else
            {
                if (pByteRet != NULL)
                {
                    *pByteRet = (UINT32) 0;
                }
            }
            break;

        case FSR_LLD_IOCTL_LOCK_BLOCK:
            if ((pBufI == NULL) || (nLenI != sizeof(LLDProtectionArg)) ||
                (pBufO == NULL) || (nLenO != sizeof(nErrorPbn)))
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[OND:ERR]   %s() / %d line\r\n"),
                    __FSR_FUNC__, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            invalid parameter pBufI = 0x%08x, nLenI = %d\r\n"), pBufI, nLenI));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            invalid parameter pBufO = 0x%08x, nLenO = %d\r\n"), pBufO, nLenO));

                nLLDRe = FSR_LLD_INVALID_PARAM;
                break;
            }

            pLLDProtectionArg   = (LLDProtectionArg *) pBufI;

            nDie = pLLDProtectionArg->nStartBlk >>
                (FSR_OND_4K_DFS_BASEBIT - pstOND4kCxt->nDDPSelSft);

            FSR_ASSERT((nDie & ~0x1) == 0);

            nLLDRe = _ControlLockBlk(nDev,
                                     pLLDProtectionArg->nStartBlk,
                                     pLLDProtectionArg->nBlks,
                                     FSR_OND_4K_CMD_LOCK_BLOCK,
                                     &nErrorPbn);
            if (nLLDRe == FSR_LLD_INVALID_BLOCK_STATE)
            {
                *(UINT32 *) pBufO = nErrorPbn;

                if (pByteRet != NULL)
                {
                    *pByteRet = sizeof(nErrorPbn);
                }
            }
            else
            {
                if (pByteRet != NULL)
                {
                    *pByteRet = (UINT32) 0;
                }
            }
            break;

        case FSR_LLD_IOCTL_UNLOCK_BLOCK:
            if ((pBufI == NULL) || (nLenI != sizeof(LLDProtectionArg)) ||
                (pBufO == NULL) || (nLenO != sizeof(nErrorPbn)))
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[OND:ERR]   %s() / %d line\r\n"),
                    __FSR_FUNC__, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            invalid parameter pBufI = 0x%08x, nLenI = %d\r\n"), pBufI, nLenI));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            invalid parameter pBufO = 0x%08x, nLenO = %d\r\n"), pBufO, nLenO));

                nLLDRe = FSR_LLD_INVALID_PARAM;
                break;
            }

            pLLDProtectionArg   = (LLDProtectionArg *) pBufI;

            nDie = pLLDProtectionArg->nStartBlk >> 
                (FSR_OND_4K_DFS_BASEBIT - pstOND4kCxt->nDDPSelSft);

            FSR_ASSERT((nDie & ~0x1) == 0);

            nLLDRe = _ControlLockBlk(nDev,
                                     pLLDProtectionArg->nStartBlk,
                                     pLLDProtectionArg->nBlks,
                                     FSR_OND_4K_CMD_UNLOCK_BLOCK,
                                     &nErrorPbn);
            if (nLLDRe == FSR_LLD_INVALID_BLOCK_STATE)
            {
                *(UINT32 *) pBufO = nErrorPbn;

                if (pByteRet != NULL)
                {
                    *pByteRet = sizeof(nErrorPbn);
                }
            }
            else
            {
                if (pByteRet != NULL)
                {
                    *pByteRet = (UINT32) 0;
                }
            }

            break;

        case FSR_LLD_IOCTL_UNLOCK_ALLBLK:
            if ((pBufI == NULL) || (nLenI != sizeof(UINT32)))
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[OND:ERR]   %s() / %d line\r\n"),
                    __FSR_FUNC__, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            invalid parameter pBufI = 0x%08x, nLenI = %d\r\n"), pBufI, nLenI));

                nLLDRe = FSR_LLD_INVALID_PARAM;
                break;
            }

            nPbn = *(UINT32 *) pBufI;

            nDie = nPbn >> (FSR_OND_4K_DFS_BASEBIT - pstOND4kCxt->nDDPSelSft);

            FSR_ASSERT((nDie & ~0x1) == 0);

            OND_4K_WRITE(pstFOReg->nStartAddr2,
                (UINT16) ((nPbn << pstOND4kCxt->nDDPSelSft) & FSR_OND_4K_DBS_MASK));

            OND_4K_WRITE(pstFOReg->nStartAddr1,
                (UINT16) ((nPbn << pstOND4kCxt->nDDPSelSft) & FSR_OND_4K_DFS_MASK));

            OND_4K_WRITE(pstFOReg->nStartBlkAddr,
                (UINT16) (nPbn & pstOND4kCxt->nFBAMask));


            OND_4K_WRITE(pstFOReg->nCmd, FSR_OND_4K_CMD_UNLOCK_ALLBLOCK);


            WAIT_OND_4K_INT_STAT(pstFOReg, FSR_OND_4K_INT_MASTER_READY);

            if (pByteRet != NULL)
            {
                *pByteRet = (UINT32) 0;
            }

            break;

        case FSR_LLD_IOCTL_GET_LOCK_STAT:

            if ((pBufI == NULL) || (nLenI != sizeof(UINT32)) ||
                (pBufO == NULL) || (nLenO != sizeof(UINT32)))
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[OND:ERR]   %s() / %d line\r\n"),
                    __FSR_FUNC__, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            invalid parameter pBufO = 0x%08x, nLenO = %d\r\n"), pBufO, nLenO));

                nLLDRe = FSR_LLD_INVALID_PARAM;
                break;
            }

            nPbn    = *(UINT32 *) pBufI;

            nDie = nPbn >> (FSR_OND_4K_DFS_BASEBIT - pstOND4kCxt->nDDPSelSft);

            FSR_ASSERT((nDie & ~0x1) == 0);

            /* set DBS       */
            OND_4K_WRITE(pstFOReg->nStartAddr2,
                (UINT16) ((nPbn << pstOND4kCxt->nDDPSelSft) & FSR_OND_4K_DBS_MASK)) ;

            /* set DFS & FBA */
            OND_4K_WRITE(pstFOReg->nStartAddr1,
                (UINT16) (((nPbn << pstOND4kCxt->nDDPSelSft) & FSR_OND_4K_DFS_MASK) | (nPbn & pstOND4kCxt->nFBAMask)));

#if defined(FSR_LLD_WAIT_WR_PROTECT_STAT)
            do
            {
                nWrProtectStat = OND_4K_READ(pstFOReg->nWrProtectStat);
            } while (nWrProtectStat == (UINT16) 0x0000);
#else
            nWrProtectStat = OND_4K_READ(pstFOReg->nWrProtectStat);
#endif

            FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_INF,
                (TEXT("[OND:INF]   nDev: %d, nPbn: %d has lock status 0x%04x\r\n"),
                nDev, nPbn, nWrProtectStat));

            *(UINT32 *) pBufO = (UINT32) nWrProtectStat;

            if (pByteRet != NULL)
            {
                *pByteRet = (UINT32) sizeof(UINT32);
            }

            nLLDRe    = FSR_LLD_SUCCESS;
            break;

        case FSR_LLD_IOCTL_HOT_RESET:

            OND_4K_WRITE(pstFOReg->nCmd, FSR_OND_4K_CMD_HOT_RESET);

            WAIT_OND_4K_INT_STAT(pstFOReg, FSR_OND_4K_INT_RESET_READY);

            /* hot reset initialize the nSysConf1 register */
            OND_4K_WRITE(pstFOReg->nSysConf1, pstOND4kCxt->nSysConf1);

            if (pByteRet != NULL)
            {
                *pByteRet = (UINT32) 0;
            }
            break;

        case FSR_LLD_IOCTL_CORE_RESET:

            OND_4K_WRITE(pstFOReg->nCmd, FSR_OND_4K_CMD_RST_NFCORE);

            WAIT_OND_4K_INT_STAT(pstFOReg, FSR_OND_4K_INT_RESET_READY);

            if (pByteRet != NULL)
            {
                *pByteRet = (UINT32) 0;
            }
            break;

        default:
            nLLDRe = FSR_LLD_IOCTL_NOT_SUPPORT;
            break;
        }

        if ((nCode == FSR_LLD_IOCTL_HOT_RESET) || (nCode == FSR_LLD_IOCTL_CORE_RESET))
        {
            for (nDie = 0; nDie < pstOND4kSpec->nNumOfDies; nDie++)
            {
                pstOND4kShMem->nPreOp[nDie]         = FSR_OND_4K_PREOP_IOCTL;
                pstOND4kShMem->nPreOpPbn[nDie]      = FSR_OND_4K_PREOP_ADDRESS_NONE;
                pstOND4kShMem->nPreOpPgOffset[nDie] = FSR_OND_4K_PREOP_ADDRESS_NONE;
                pstOND4kShMem->nPreOpFlag[nDie]     = FSR_OND_4K_PREOP_FLAG_NONE;
            }
        }
        else if ((nLLDRe != FSR_LLD_INVALID_PARAM) && (nLLDRe != FSR_LLD_IOCTL_NOT_SUPPORT))
        {
            FSR_ASSERT(nDie != 0xFFFFFFFF);

            pstOND4kShMem->nPreOp[nDie]         = FSR_OND_4K_PREOP_IOCTL;
            pstOND4kShMem->nPreOpPbn[nDie]      = FSR_OND_4K_PREOP_ADDRESS_NONE;
            pstOND4kShMem->nPreOpPgOffset[nDie] = FSR_OND_4K_PREOP_ADDRESS_NONE;
            pstOND4kShMem->nPreOpFlag[nDie]     = FSR_OND_4K_PREOP_FLAG_NONE;
        }

    } while (0);


    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return (nLLDRe);
}



/**
 * @brief          This function controls lock/unlock/lock-tight block
 *
 * @param[in]      nDev         : Physical Device Number (0 ~ 7)
 * @param[in]      nSbn         : start block number
 * @param[in]      nBlks        : the number of blocks to lock tight
 * @param[out]     pnErrorPbn   : physical block number where
 * @n                             command (lock tight) fails.
 *
 * @return         FSR_LLD_SUCCESS
 * @return         FSR_LLD_BLK_PROTECTION_ERROR
 *
 * @author         NamOh Hwang / JeongWook Moon
 * @version        1.0.0
 * @remark
 *
 */
PRIVATE INT32 
_ControlLockBlk(UINT32  nDev, 
                UINT32  nPbn, 
                UINT32  nBlks,
                UINT32  nLockTypeCMD,
                UINT32 *pnErrPbn)
{
             OneNAND4kCxt       *pstOND4kCxt  = gpstOND4kCxt[nDev];
    volatile OneNAND4kReg   *pstFOReg   = (volatile OneNAND4kReg *) pstOND4kCxt->nBaseAddr;

             UINT32            nDie;
             UINT32            nFBA;
             INT32             nLLDRe     = FSR_LLD_SUCCESS;

             UINT16            nLockStat;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[OND:IN ] ++%s(nDev: %d, nPbn: %d, nBlks: %d)\r\n"),
        __FSR_FUNC__, nDev, nPbn, nBlks));

    while (nBlks > 0)
    {
        nDie = nPbn >> (FSR_OND_4K_DFS_BASEBIT - pstOND4kCxt->nDDPSelSft);
        nFBA = nPbn & pstOND4kCxt->nFBAMask;

        /* set DBS */
        OND_4K_WRITE(pstFOReg->nStartAddr2, (UINT16) (nDie << FSR_OND_4K_DBS_BASEBIT));

        /* set DFS, FBA */
        OND_4K_WRITE(pstFOReg->nStartAddr1,
            (UINT16) ((nDie << FSR_OND_4K_DFS_BASEBIT) | nFBA));

#if defined(FSR_LLD_WAIT_WR_PROTECT_STAT)
        do
        {
            nLockStat = OND_4K_READ(pstFOReg->nWrProtectStat);
        } while (nLockStat == (UINT16) 0x0000);
#else
        nLockStat = OND_4K_READ(pstFOReg->nWrProtectStat);
#endif

        nLockStat = nLockStat & FSR_LLD_BLK_STAT_MASK;

        /* to lock blocks tight, the block should be locked first */

        switch (nLockTypeCMD)
        {
        case FSR_OND_4K_CMD_LOCKTIGHT_BLOCK:
            if (nLockStat == FSR_LLD_BLK_STAT_UNLOCKED)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[OND:ERR]   %s() / %d line\r\n"),
                    __FSR_FUNC__, __LINE__));
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            unlocked blk cannot be locked tight\r\n")));
                nLLDRe = FSR_LLD_INVALID_BLOCK_STATE;
            }
            break;
        case FSR_OND_4K_CMD_LOCK_BLOCK:
        case FSR_OND_4K_CMD_UNLOCK_BLOCK:
            if (nLockStat == FSR_LLD_BLK_STAT_LOCKED_TIGHT)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[OND:ERR]   %s() / %d line\r\n"),
                    __FSR_FUNC__, __LINE__));
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            lock-tight blk cannot be unlocked or locked\r\n")));
                nLLDRe = FSR_LLD_INVALID_BLOCK_STATE;
            }
            break;
        default:
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s() / %d line\r\n"),
                __FSR_FUNC__, __LINE__));
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Unknown command 0x%x\r\n"), nLockTypeCMD));
            nLLDRe = FSR_LLD_INVALID_BLOCK_STATE;
            break;
        }

        if (!(nLLDRe == FSR_LLD_SUCCESS))
        {
            *pnErrPbn = nPbn;
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Pbn #%d is unlocked\r\n"), nPbn));
            _DumpRegisters(pstFOReg);
            _DumpSpareBuffer(pstFOReg);
            break;
        }

        OND_4K_WRITE(pstFOReg->nStartBlkAddr,(UINT16) nFBA);

        /* a command */
        OND_4K_WRITE(pstFOReg->nCmd, (UINT16)nLockTypeCMD);

        WAIT_OND_4K_INT_STAT(pstFOReg, FSR_OND_4K_INT_MASTER_READY);

        /* Check CtrlStat register */
        if (OND_4K_READ(pstFOReg->nCtrlStat) & FSR_OND_4K_STATUS_ERROR)
        {
            *pnErrPbn = nPbn;

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s() / %d line\r\n"),
                __FSR_FUNC__, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Pbn #%d is fail lock tight\r\n"), nPbn));

            _DumpRegisters(pstFOReg);
            _DumpSpareBuffer(pstFOReg);

            nLLDRe = FSR_LLD_INVALID_BLOCK_STATE;
            break;
        }

        /* set DFS, FBA */
        OND_4K_WRITE(pstFOReg->nStartAddr1,
            (UINT16) ((nDie << FSR_OND_4K_DFS_BASEBIT) | nFBA));

        /* Wait WR_PROTECT_STAT */
        switch (nLockTypeCMD)
        {
        case FSR_OND_4K_CMD_LOCKTIGHT_BLOCK:
            WAIT_OND_4K_WR_PROTECT_STAT(pstFOReg, FSR_LLD_BLK_STAT_LOCKED_TIGHT);
            break;
        case FSR_OND_4K_CMD_LOCK_BLOCK:
            WAIT_OND_4K_WR_PROTECT_STAT(pstFOReg, FSR_LLD_BLK_STAT_LOCKED);
            break;
        case FSR_OND_4K_CMD_UNLOCK_BLOCK:
            WAIT_OND_4K_WR_PROTECT_STAT(pstFOReg, FSR_LLD_BLK_STAT_UNLOCKED);
            break;
        default:
            break;
        }

        FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_INF,
            (TEXT("[OND:INF]   nDev: %d, nPbn: %d is locked tight\r\n"),
            nDev, nPbn));

        nBlks--;
        nPbn++;
    }
  
    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s()\r\n"), __FSR_FUNC__));

    return (nLLDRe);
}


/**
 * @brief           this function locks the OTP block, 1st OTP block
 *
 * @param[in]       nDev        : Physical Device Number
 * @param[in]       nLockValue  : this is programmed into 1st word of sector0 of
 * @n                             main of page0 in the OTP block
 *
 * @return          FSR_LLD_SUCCESS
 * @n               FSR_LLD_INVALID_PARAM
 * @n               FSR_LLD_OTP_ALREADY_LOCKED
 *
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark
 *
 */
PRIVATE INT32
_LockOTP(UINT32 nDev,
         UINT32 nLockValue)
{
            OneNAND4kCxt         *pstOND4kCxt  = gpstOND4kCxt[nDev];
            OneNAND4kSpec        *pstOND4kSpec = pstOND4kCxt->pstOND4kSpec;
   volatile OneNAND4kReg     *pstFOReg   = (volatile OneNAND4kReg *) pstOND4kCxt->nBaseAddr;

            UINT32              nBytesRet;
            UINT32              nLockState;
            UINT32              nPbn;
            UINT32              nFlushOpCaller;

            INT32               nLLDRe     = FSR_LLD_SUCCESS;



    FSR_STACK_VAR;

    FSR_STACK_END;



    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[OND:IN ] ++%s(nDev:%d, nLockValue:0x%04x)\r\n"),
        __FSR_FUNC__, nDev, nLockValue));

    do
    {
        nLLDRe = FSR_OND_4K_IOCtl(nDev, FSR_LLD_IOCTL_OTP_GET_INFO,
                                NULL, 0,
                                (UINT8 *) &nLockState, sizeof(nLockState),
                                &nBytesRet);
        if (nLLDRe != FSR_LLD_SUCCESS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   FSR_OND_4K_IOCtl(nCode:FSR_LLD_IOCTL_OTP_GET_INFO) / %s(nDev:%d) / %d line\r\n"),
                __FSR_FUNC__, nDev, __LINE__));
            break;
        }

        if ((nLockValue & FSR_OND_4K_OTP_LOCK_MASK) == FSR_OND_4K_LOCK_1ST_BLOCK_OTP)
        {
            /* it's an Error to lock 1st block as OTP
             * when this device doesn't use 1st block as OTP
             */
            if ((pstOND4kSpec->b1stBlkOTP) == FALSE32)
            {
                nLLDRe = FSR_LLD_INVALID_PARAM;
                break;
            }

            /* when OTP block is locked,
             * 1st block cannot be locked as OTP
             */
            if (nLockState & FSR_LLD_OTP_OTP_BLK_LOCKED)
            {
                nLLDRe = FSR_LLD_OTP_ALREADY_LOCKED;
                break;
            }
        }

        nPbn = 0;

        /* the size of nPbn should be 4 bytes */
        FSR_ASSERT(sizeof(nPbn) == sizeof(UINT32));

        nLLDRe = FSR_OND_4K_IOCtl(nDev, FSR_LLD_IOCTL_OTP_ACCESS,
                                (UINT8 *) &nPbn, sizeof(nPbn),
                                NULL, 0,
                                &nBytesRet);
        if (nLLDRe != FSR_LLD_SUCCESS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   FSR_OND_4K_IOCtl(nCode:FSR_LLD_IOCTL_OTP_ACCESS) / %s(nDev:%d) / %d line\r\n"),
                __FSR_FUNC__, nDev, __LINE__));
            break;
        }

        FSR_OND_4K_ReadOptimal(nDev,
                            nPbn,
                            FSR_OND_4K_OTP_PAGE_OFFSET,
                            NULL,
                            NULL,
                            FSR_LLD_FLAG_1X_LOAD);

        nLLDRe = FSR_OND_4K_ReadOptimal(nDev,
                                     nPbn,
                                     FSR_OND_4K_OTP_PAGE_OFFSET,
                                     NULL,
                                     NULL,
                                     FSR_LLD_FLAG_TRANSFER);
        if (nLLDRe != FSR_LLD_SUCCESS)
        {
            break;
        }

        nLockValue = OND_4K_READ(*(UINT16 *) pstFOReg->nDataMB10) & (UINT16) nLockValue;
        OND_4K_WRITE(*(UINT16 *) pstFOReg->nDataMB10, (UINT16) nLockValue);

        FSR_OND_4K_Write(nDev, nPbn, FSR_OND_4K_OTP_PAGE_OFFSET,
                      NULL, NULL, FSR_LLD_FLAG_1X_PROGRAM);

        nFlushOpCaller = FSR_OND_4K_PREOP_IOCTL << FSR_OND_4K_FLUSHOP_CALLER_BASEBIT;

        nLLDRe = FSR_OND_4K_FlushOp(nDev, nFlushOpCaller | 0, FSR_LLD_FLAG_NONE);

        if (nLLDRe != FSR_LLD_SUCCESS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s(nDev:%d, nLockValue:0x%04x) / %d line\r\n"),
                __FSR_FUNC__, nDev, nLockValue, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Program Error while locking OTP\r\n")));

            _DumpRegisters(pstFOReg);
            _DumpSpareBuffer(pstFOReg);

            break;
        }

    } while (0);

    /* exit the OTP block */
    FSR_OND_4K_IOCtl(nDev,                         /*  nDev      : Physical Device Number (0 ~ 7)   */
                  FSR_LLD_IOCTL_CORE_RESET,     /*  nCode     : IO Control Command               */
                  NULL,                         /* *pBufI     : Input Buffer pointer             */
                  0,                            /*  nLenI     : Length of Input Buffer           */
                  NULL,                         /* *pBufO     : Output Buffer pointer            */
                  0,                            /*  nLenO     : Length of Output Buffer          */
                  &nBytesRet);                  /* *pByteRet  : The number of bytes (length) of 
                                                                Output Buffer as the result of 
                                                                function call                    */

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s()\r\n"), __FSR_FUNC__));

    return nLLDRe;
}



/**
 * @brief           this function checks the validity of parameter
 *
 * @param[in]       nDev        : Physical Device Number (0 ~ 3)
 * @param[in]       nPbn        : Physical Block  Number
 * @param[in]       nPgOffset   : Page Offset within a block
 * @param[in]       pstOND4kCxt   : gpstOND4kCxt[nDev]
 * @param[in]       pstOND4kSpec  : gpstOND4kCxt[nDev]->pstOND4kSpec
 *
 * @return          FSR_LLD_SUCCESS
 * @return          FSR_LLD_INVALID_PARAM
 *
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark          Pbn is consecutive numbers within the device
 * @n               i.e. for DDP device, Pbn for 1st block in 2nd chip is not 0
 * @n               it is one more than the last block number in 1st chip
 *
 */
PRIVATE INT32
_StrictChk(UINT32       nDev,
           UINT32       nPbn,
           UINT32       nPgOffset)
{
    OneNAND4kCxt     *pstOND4kCxt  = NULL;
    OneNAND4kSpec    *pstOND4kSpec = NULL;

    UINT32          nDie;

    INT32           nLLDRe = FSR_LLD_SUCCESS;

    FSR_STACK_VAR;

    FSR_STACK_END;


    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] ++%s(nDev:%d, nPbn:%d, nPgOffset:%d)\r\n"),
        __FSR_FUNC__, nDev, nPbn, nPgOffset));

    do
    {
        /* check Device Number */
        if (nDev >= FSR_OND_4K_MAX_DEVS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s() / %d line\r\n"),
                __FSR_FUNC__, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Invalid Device Number (nDev = %d)\r\n"),
                nDev));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }

        pstOND4kCxt  = gpstOND4kCxt[nDev];
        pstOND4kSpec = pstOND4kCxt->pstOND4kSpec;

        nDie       = nPbn >> (FSR_OND_4K_DFS_BASEBIT - pstOND4kCxt->nDDPSelSft);

        /* check Device Open Flag */
        if (pstOND4kCxt->bOpen == FALSE32)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s() / %d line\r\n"),
                __FSR_FUNC__, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Device is not opened\r\n")));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            pstOND4kCxt->bOpen: 0x%08x\r\n"),
                pstOND4kCxt->bOpen));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }

        /* check Block Out of Bound */
        if (nPbn >= pstOND4kSpec->nNumOfBlks)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s() / %d line\r\n"),
                __FSR_FUNC__, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Pbn:%d >= pstOND4kSpec->nNumOfBlks:%d\r\n"),
                nPbn, pstOND4kSpec->nNumOfBlks));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }
        else
        {
            /* check nPgOffset Out of Bound                */
            /* in case of SLC, pageOffset is lower than 64 */
            if (nPgOffset >= pstOND4kSpec->nPgsPerBlkForSLC)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[OND:ERR]   %s() / %d line\r\n"),
                    __FSR_FUNC__, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            Pg #%d in SLC(Pbn #%d) is invalid\r\n"),
                    nPgOffset, nPbn));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            nBlksForSLCArea[%d] = %d\r\n"),
                    nDie, pstOND4kCxt->nBlksForSLCArea[nDie]));

                nLLDRe = FSR_LLD_INVALID_PARAM;
                break;
            }
        }

    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s()\r\n"), __FSR_FUNC__));

    return (nLLDRe);
}



/**
 * @brief           This function prints the contents register file
 *
 * @param[in]       pstReg      : pointer to structure OneNAND4kReg.
 *
 * @return          none
 *
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark
 *
 */
PRIVATE VOID
_DumpRegisters(volatile OneNAND4kReg *pstReg)
{
    UINT32      nDiesPerDev;
    UINT16      nDID;
    UINT16      nStartAddr2;
    UINT16      nDBS;
    FSR_STACK_VAR;

    FSR_STACK_END;


    nDID        = OND_4K_READ(pstReg->nDID);
    nDiesPerDev = ((nDID >> 3) & 1) + 1;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
    (TEXT("[OND:IN ] ++%s(pstReg:0x%08x)\r\n"),
    __FSR_FUNC__, pstReg));

    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("\r\nstart dumping registers. \r\n")));
#if defined (TINY_FSR)
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[OND:   ]   this is TINY_FSR\r\n")));
#endif
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Manufacturer ID Reg."),
        &pstReg->nMID, OND_4K_READ(pstReg->nMID)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Device ID Reg."),
        &pstReg->nDID, OND_4K_READ(pstReg->nDID)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Version ID Reg."),
        &pstReg->nVerID, OND_4K_READ(pstReg->nVerID)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, 
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Data Buffer Size Reg."),
        &pstReg->nDataBufSize, OND_4K_READ(pstReg->nDataBufSize)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Boot Buffer Size Reg."),
        &pstReg->nBootBufSize, OND_4K_READ(pstReg->nBootBufSize)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Amount of buffers Reg."),
        &pstReg->nBufAmount, OND_4K_READ(pstReg->nBufAmount)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Technology Reg."),
        &pstReg->nTech, OND_4K_READ(pstReg->nTech)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Start address 1 Reg."),
        &pstReg->nStartAddr1, OND_4K_READ(pstReg->nStartAddr1)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Start address 2 Reg."),
        &pstReg->nStartAddr2, OND_4K_READ(pstReg->nStartAddr2)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Start address 8 Reg."),
        &pstReg->nStartAddr8, OND_4K_READ(pstReg->nStartAddr8)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Start Buffer Reg."),
        &pstReg->nStartBuf, OND_4K_READ(pstReg->nStartBuf)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Command Reg."),
        &pstReg->nCmd, OND_4K_READ(pstReg->nCmd)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("System Conf1 Reg."),
        &pstReg->nSysConf1, OND_4K_READ(pstReg->nSysConf1)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Controller Status Reg."),
        &pstReg->nCtrlStat, OND_4K_READ(pstReg->nCtrlStat)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Interrupt Reg."),
        &pstReg->nInt, OND_4K_READ(pstReg->nInt)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Start Block Address Reg."),
        &pstReg->nStartBlkAddr, OND_4K_READ(pstReg->nStartBlkAddr)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Write Protection Reg."),
        &pstReg->nWrProtectStat, OND_4K_READ(pstReg->nWrProtectStat)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("ECC Status 1 Reg."),
        &pstReg->nEccStat[0], OND_4K_READ(pstReg->nEccStat[0])));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("ECC Status 2 Reg."),
        &pstReg->nEccStat[1], OND_4K_READ(pstReg->nEccStat[1])));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("ECC Status 3 Reg."),
        &pstReg->nEccStat[2], OND_4K_READ(pstReg->nEccStat[2])));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("ECC Status 4 Reg."),
        &pstReg->nEccStat[3], OND_4K_READ(pstReg->nEccStat[3])));

    if (nDiesPerDev == 2)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("other die register dump\r\n")));

        /* backup DBS */
        nStartAddr2 = OND_4K_READ(pstReg->nStartAddr2);
        nDBS        = nStartAddr2 >> FSR_OND_4K_DBS_BASEBIT;
        nDBS        = (nDBS + 1) & 1;

        /* set DBS */
        OND_4K_WRITE(pstReg->nStartAddr2,
            (UINT16) (nDBS << FSR_OND_4K_DBS_BASEBIT));

        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Start address 1 Reg."),
            &pstReg->nStartAddr1, OND_4K_READ(pstReg->nStartAddr1)));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Start address 2 Reg."),
            &pstReg->nStartAddr2, OND_4K_READ(pstReg->nStartAddr2)));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Start address 8 Reg."),
            &pstReg->nStartAddr8, OND_4K_READ(pstReg->nStartAddr8)));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Controller Status Reg."),
            &pstReg->nCtrlStat, OND_4K_READ(pstReg->nCtrlStat)));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Interrupt Reg."),
            &pstReg->nInt, OND_4K_READ(pstReg->nInt)));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Write Protection Reg."),
            &pstReg->nWrProtectStat, OND_4K_READ(pstReg->nWrProtectStat)));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("ECC Status 1 Reg."),
            &pstReg->nEccStat[0], OND_4K_READ(pstReg->nEccStat[0])));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("ECC Status 2 Reg."),
            &pstReg->nEccStat[1], OND_4K_READ(pstReg->nEccStat[1])));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("ECC Status 3 Reg."),
            &pstReg->nEccStat[2], OND_4K_READ(pstReg->nEccStat[2])));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("ECC Status 4 Reg."),
            &pstReg->nEccStat[3], OND_4K_READ(pstReg->nEccStat[3])));

        /* restore DBS */
        OND_4K_WRITE(pstReg->nStartAddr2,
            (UINT16) (nStartAddr2));
    }

    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("end dumping registers. \r\n\r\n")));


    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[OND:OUT] --%s\r\n"), __FSR_FUNC__));
}

/**
 * @brief           This function prints spare buffer
 *
 * @param[in]       pstReg      : pointer to structure OneNAND4kReg.
 *
 * @return          none
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 * @remark
 *
 */
PRIVATE VOID
_DumpSpareBuffer(volatile OneNAND4kReg *pstReg)
{
#if !defined(FSR_OAM_RTLMSG_DISABLE)
    UINT32      nSctIdx;
    UINT32      nIdx;
    UINT32      nDieIdx;
    UINT32      nDataBufSize;
    UINT32      nSctsPerDataBuf;
    UINT32      nDiesPerDev;
    UINT16     *pnDataSB00;
    UINT16      nDID;
    UINT16      nStartAddr2;
    UINT16      nValue;
    FSR_STACK_VAR;

    FSR_STACK_END;

    nDID        = OND_4K_READ(pstReg->nDID);
    nDiesPerDev = ((nDID >> 3) & 1) + 1;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
    (TEXT("[OND:IN ] ++%s(pstReg:0x%08x)\r\n"),
    __FSR_FUNC__, pstReg));

    /* backup DBS */
    nStartAddr2 = OND_4K_READ(pstReg->nStartAddr2);

    nDataBufSize    = OND_4K_READ(pstReg->nDataBufSize) * 2;
    nSctsPerDataBuf = nDataBufSize / FSR_SECTOR_SIZE;
    pnDataSB00      = (UINT16 *) pstReg->nDataSB00;

    for (nDieIdx = 0; nDieIdx < nDiesPerDev; nDieIdx++)
    {
        /* set DBS */
        OND_4K_WRITE(pstReg->nStartAddr2,
            (UINT16) (nDieIdx << FSR_OND_4K_DBS_BASEBIT));

        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("          Dump Spare Buffer [die:%d]\r\n"), nDieIdx));
        for (nSctIdx = 0; nSctIdx < nSctsPerDataBuf; nSctIdx++)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[%02d] "), nSctIdx));
            for (nIdx = 0; nIdx < 8; nIdx++)
            {
                nValue = (UINT16) OND_4K_READ(pnDataSB00[nSctIdx * 8 + nIdx]);
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("%04x "), nValue));
            }
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("\r\n")));
        }
    }

    /* restore DBS */
    OND_4K_WRITE(pstReg->nStartAddr2,
        (UINT16) (nStartAddr2));

    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("\r\n")));

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[OND:OUT] --%s()\r\n"), __FSR_FUNC__));
#endif
}

/**
 * @brief           This function prints error context of TimeOut error
 * @n               when using Tiny FSR and FSR at the same time
 *
 * @param[in]       none
 *
 * @return          none
 *
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark
 *
 */
PRIVATE VOID
_DumpCmdLog(VOID)
{
#if !defined(FSR_OAM_RTLMSG_DISABLE)
#if defined (FSR_LLD_HANDSHAKE_ERR_INF)
    volatile    OneNAND4kSharedCxt   *pstOND4kSharedCxt = NULL;
#endif
                OneNAND4kCxt         *pstOND4kCxt       = NULL;
                OneNAND4kShMem       *pstOND4kShMem     = NULL;

                UINT32              nDev;

#if defined(FSR_LLD_LOGGING_HISTORY)
    volatile    OneNAND4kOpLog       *pstOND4kOpLog     = NULL;
                UINT32              nIdx;
                UINT32              nLogHead;
                UINT32              nPreOpIdx;
#endif

    FSR_STACK_VAR;

    FSR_STACK_END;



    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG | FSR_DBZ_ERROR,
        (TEXT("[OND:IN ] ++%s()\r\n\r\n"), __FSR_FUNC__));

#if defined (TINY_FSR)
    FSR_DBZ_DBGMOUT(FSR_DBZ_ERROR, (TEXT("[OND:   ]   this is TINY_FSR\r\n")));
#endif

    for (nDev = 0; nDev < FSR_MAX_DEVS; nDev++)
    {
        pstOND4kCxt  = gpstOND4kCxt[nDev];

        if (pstOND4kCxt == NULL)
        {
            continue;
        }

        if (pstOND4kCxt->bOpen == FALSE32)
        {
            continue;
        }

        pstOND4kShMem = gpstOND4kShMem[nDev];

        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR | FSR_DBZ_LLD_INF,
            (TEXT("[OND:INF]   pstOND4kCxt->nFlushOpCaller : %d\r\n"),
            pstOND4kCxt->nFlushOpCaller));


#if defined (FSR_LLD_LOGGING_HISTORY)
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("[OND:INF]   start printing nLog      : nDev[%d]\r\n"), nDev));

        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("%5s  %7s  %10s  %10s  %10s\r\n"), TEXT("nLog"),
            TEXT("preOp"), TEXT("prePbn"), TEXT("prePg"), TEXT("preFlag")));

        pstOND4kOpLog = &gstOND4kOpLog[nDev];

        nLogHead = pstOND4kOpLog->nLogHead;
        for (nIdx = 0; nIdx < FSR_4K_OND_MAX_LOG; nIdx++)
        {
            nPreOpIdx = pstOND4kOpLog->nLogOp[nLogHead];
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("%5d| %7s, 0x%08x, 0x%08x, 0x%08x\r\n"),
                nLogHead, gpszLogPreOp[nPreOpIdx], pstOND4kOpLog->nLogPbn[nLogHead],
                pstOND4kOpLog->nLogPgOffset[nLogHead], pstOND4kOpLog->nLogFlag[nLogHead]));

            nLogHead = (nLogHead + 1) & (FSR_4K_OND_MAX_LOG -1);
        }

        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("[OND:   ]   end   printing nLog      : nDev[%d]\r\n"), nDev));
#endif /* #if defined (FSR_LLD_LOGGING_HISTORY) */

        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("\r\n[OND:INF]   start printing OneNAND4kCxt: nDev[%d]\r\n"), nDev));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("            nPreOp           : [0x%08x, 0x%08x]\r\n"),
            pstOND4kShMem->nPreOp[0], pstOND4kShMem->nPreOp[1]));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("            nPreOpPbn        : [0x%08x, 0x%08x]\r\n"),
            pstOND4kShMem->nPreOpPbn[0], pstOND4kShMem->nPreOpPbn[1]));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("            nPreOpPgOffset   : [0x%08x, 0x%08x]\r\n"),
            pstOND4kShMem->nPreOpPgOffset[0], pstOND4kShMem->nPreOpPgOffset[1]));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("            nPreOpFlag       : [0x%08x, 0x%08x]\r\n"),
            pstOND4kShMem->nPreOpFlag[0], pstOND4kShMem->nPreOpFlag[1]));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("            end   printing OneNAND4kCxt: nDev[%d]\r\n\r\n"), nDev));


#if defined (FSR_LLD_HANDSHAKE_ERR_INF)

        pstOND4kSharedCxt = &gstOND4kSharedCxt[nDev];

        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("[OND:INF]   start printing SharedCxt: nDev[%d]\r\n"), nDev));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("[OND:   ]   nPrevFSRMode     : [0x%08x, 0x%08x]\r\n"),
            pstOND4kSharedCxt->nPrevFSRMode[0], pstOND4kSharedCxt->nPrevFSRMode[1]));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("[OND:   ]   nHostLLDRe       : [0x%08x, 0x%08x]\r\n"),
            pstOND4kSharedCxt->nHostLLDRe[0], pstOND4kSharedCxt->nHostLLDRe[1]));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("[OND:   ]   nHostLLDOp       : [0x%08x, 0x%08x]\r\n"),
            pstOND4kSharedCxt->nHostLLDOp[0], pstOND4kSharedCxt->nHostLLDOp[1]));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("[OND:   ]   nHostLLDPbn      : [0x%08x, 0x%08x]\r\n"),
            pstOND4kSharedCxt->nHostLLDPbn[0], pstOND4kSharedCxt->nHostLLDPbn[1]));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("[OND:   ]   nHostLLDPgOffset : [0x%08x, 0x%08x]\r\n"),
            pstOND4kSharedCxt->nHostLLDPgOffset[0], pstOND4kSharedCxt->nHostLLDPgOffset[1]));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("[OND:   ]   nHostLLDFlag     : [0x%08x, 0x%08x]\r\n"),
            pstOND4kSharedCxt->nHostLLDFlag[0], pstOND4kSharedCxt->nHostLLDFlag[1]));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("[OND:   ]   end   printing SharedCxt: nDev[%d]\r\n"), nDev));
#endif

    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG | FSR_DBZ_ERROR, (TEXT("[OND:OUT] --%s()\r\n"), __FSR_FUNC__));
#endif /* #if !defined(FSR_OAM_RTLMSG_DISABLE) */
}


/**
 * @brief           This function calculates transfer time for word (2 bytes)
 * @n               between host & OneNAND
 *
 * @param[in]       nDev         : Physical Device Number (0 ~ 3)
 * @param[in]       nSysConfig1  : read mode (sync or async) depends on
 *                                 the SysConfig1 Register
 * @param[out]      pnWordRdCycle: read  cycle (nano second) for word.
 * @param[out]      pnWordWrCycle: write cycle (nano second) for word.
 *
 * @return          none
 *
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark
 *
 */
PRIVATE VOID
_CalcTransferTime(UINT32  nDev,
                  UINT16  nSysConf1Reg,
                  UINT32 *pnWordRdCycle,
                  UINT32 *pnWordWrCycle)
{
             OneNAND4kCxt        *pstOND4kCxt  = gpstOND4kCxt[nDev];
             OneNAND4kSpec       *pstOND4kSpec = pstOND4kCxt->pstOND4kSpec;
    volatile OneNAND4kReg    *pstFOReg   = (volatile OneNAND4kReg *) pstOND4kCxt->nBaseAddr;

             UINT32             nPgIdx;
             UINT32             nNumOfPgs = 1;
             UINT32             nPageSize;
             /* theoretical read cycle for word (2 byte) from OneNAND to host */
             UINT32             nTheoreticalRdCycle;

             UINT32             nWordRdCycle;
             UINT32             nWordWrCycle;

    FSR_STACK_VAR;

    FSR_STACK_END;


    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[OND:IN ] ++%s(nDev:%d)\r\n"), __FSR_FUNC__, nDev));

    /* nPageSize is 4096 byte */
    nPageSize = pstOND4kSpec->nSctsPerPG * FSR_OND_4K_SECTOR_SIZE;

    /* calculate write cycle for word */
    /* here 0x55 has no meaning */
    FSR_OAM_MEMSET(pstOND4kCxt->pTempBuffer, 0x55, nPageSize);

#if !defined(FSR_OAM_RTLMSG_DISABLE)
    FSR_OAM_StartTimer();
#endif

    for (nPgIdx = 0; nPgIdx < nNumOfPgs; nPgIdx++)
    {
        TRANSFER_TO_NAND(pstFOReg->nDataMB00,
                         pstOND4kCxt->pTempBuffer,
                         nPageSize);
    }

#if !defined(FSR_OAM_RTLMSG_DISABLE)
    FSR_OAM_StopTimer();
#endif

#if !defined(FSR_OAM_RTLMSG_DISABLE)
    /* nWordWrCycle is based on nano second
     * FSR_OAM_GetElapsedTime() returns elapsed time in usec
     */
    nWordWrCycle = 2 * 1000 * FSR_OAM_GetElapsedTime() / (nNumOfPgs * nPageSize);
#else
    nWordWrCycle = 0;
#endif

    if (nWordWrCycle < 70)
    {
        *pnWordWrCycle = nWordWrCycle = 70;

        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
            (TEXT("[OND:INF]   assuming Word Write Cycle: %d nano second\r\n"),
            nWordWrCycle));
    }
    else
    {
        *pnWordWrCycle = nWordWrCycle;

        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
            (TEXT("[OND:INF]   Transfered %d bytes to DataRAM in %d usec\r\n"),
            nNumOfPgs * nPageSize, 
            FSR_OAM_GetElapsedTime()));

        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
            (TEXT("            calculated Word Write Cycle: %d nano second\r\n"),
            nWordWrCycle));
    }

    /* calculate read cycle for word */
    FSR_OAM_MEMSET(pstOND4kCxt->pTempBuffer, 0xFF, nPageSize);

#if !defined(FSR_OAM_RTLMSG_DISABLE)
    FSR_OAM_StartTimer();
#endif

    for (nPgIdx = 0; nPgIdx < nNumOfPgs; nPgIdx++)
    {
        TRANSFER_FROM_NAND(pstOND4kCxt->pTempBuffer,
                           pstFOReg->nDataMB00,
                           nPageSize);
    }

#if !defined(FSR_OAM_RTLMSG_DISABLE)
    FSR_OAM_StopTimer();
#endif

    /* time for transfering 16 bits depends on bRM (read mode) */
    nTheoreticalRdCycle = (nSysConf1Reg & FSR_OND_4K_CONF1_SYNC_READ) ? 25 : 76;

#if !defined(FSR_OAM_RTLMSG_DISABLE)
    /* nByteRdCycle is based on nano second
     * FSR_OAM_GetElapsedTime() returns elapsed time in usec
     */
    nWordRdCycle = 2 * 1000 * FSR_OAM_GetElapsedTime() / (nNumOfPgs * nPageSize);
#else
    nWordRdCycle = 0;
#endif

    if (nWordRdCycle < nTheoreticalRdCycle)
    {
        *pnWordRdCycle = nWordRdCycle = nTheoreticalRdCycle;

        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
            (TEXT("[OND:INF]   assuming Word Read  Cycle: %d nano second\r\n"),
            nWordRdCycle));
    }
    else
    {
        *pnWordRdCycle = nWordRdCycle;

        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
            (TEXT("[OND:INF]   Transfered %d bytes from DataRAM in %d usec\r\n"),
            nNumOfPgs * nPageSize,
            FSR_OAM_GetElapsedTime()));

        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
            (TEXT("            calculated Word Read  Cycle: %d nano second\r\n"),
            nWordRdCycle));
    }


    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s(nDev:%d)\r\n"), __FSR_FUNC__, nDev));
}



#if defined (FSR_LLD_STATISTICS)
/**
 * @brief           this function is called within the LLD function
 * @n               to total the busy time of the device
 *
 * @param[in]       nDev        : Physical Device Number (0 ~ 3)
 * @param[in]       nDie        : die(chip) number (0 or 1) for DDP device
 * @param[in]       nType       : FSR_OND_4K_STAT_SLC_PGM
 * @n                             FSR_OND_4K_STAT_LSB_PGM
 * @n                             FSR_OND_4K_STAT_MSB_PGM
 * @n                             FSR_OND_4K_STAT_ERASE
 * @n                             FSR_OND_4K_STAT_SLC_LOAD
 * @n                             FSR_OND_4K_STAT_MLC_LOAD
 * @n                             FSR_OND_4K_STAT_RD_TRANS
 * @n                             FSR_OND_4K_STAT_WR_TRANS
 * @n                             FSR_OND_4K_STAT_FLUSH
 * @param[in]       nBytes      : the number of bytes to transfer from/to DataRAM
 * @param[in]       nCmdOption  : command option such cache, superload
 * @n                             which can hide transfer time
 *
 *
 * @return          none
 *
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark
 *
 */
PRIVATE VOID
_AddOND4kStat(UINT32  nDev,
            UINT32  nDie,
            UINT32  nType,
            UINT32  nBytes,
            UINT32  nCmdOption)
{
    OneNAND4kCxt  *pstOND4kCxt  = gpstOND4kCxt[nDev];
    OneNAND4kSpec *pstOND4kSpec = pstOND4kCxt->pstOND4kSpec;

    UINT32       nVol;
    UINT32       nDevIdx; /* device index within a volume (0~4) */
    UINT32       nPDevIdx;/* physical device index (0~7)        */
    UINT32       nDieIdx;
    UINT32       nNumOfDies;

    /* the duration of Interrupt Low after command is issued */
    INT32        nIntLowTime;
    INT32        nTransferTime;

    FSR_STACK_VAR;

    FSR_STACK_END;



    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[OND:IN ] ++%s()\r\n"), __FSR_FUNC__));

    nIntLowTime = pstOND4kCxt->nIntLowTime[nDie];

    switch (nType)
    {
    case FSR_OND_4K_STAT_SLC_PGM:
        pstOND4kCxt->nNumOfSLCPgms++;
        
        if (nIntLowTime > 0)
        {
            /* wait INT for previous operation */
            gnElapsedTime += nIntLowTime;
            
            for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
            {
                for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVol]; nDevIdx++)
                {
                    nPDevIdx   = nVol * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx;
                    nNumOfDies = gpstOND4kCxt[nPDevIdx]->pstOND4kSpec->nNumOfDies;

                    for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                    {
                        if (gpstOND4kCxt[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                        {
                            gpstOND4kCxt[nPDevIdx]->nIntLowTime[nDieIdx] -= nIntLowTime;
                        }
                    }
                }
            }
        }

        /* if nCmdOption is CACHE, transfering time can be hided */
        pstOND4kCxt->nPreCmdOption[nDie]  = nCmdOption;
        pstOND4kCxt->nIntLowTime[nDie]    = FSR_OND_4K_WR_SW_OH + pstOND4kSpec->nSLCTProgTime;

        if (nCmdOption == FSR_OND_4K_STAT_CACHE_PGM)
        {
            pstOND4kCxt->nNumOfCacheBusy++;
        }
        break;
    
    case FSR_OND_4K_STAT_ERASE:
        pstOND4kCxt->nNumOfErases++;
        
        if (nIntLowTime > 0)
        {
            /* wait INT for previous operation */
            gnElapsedTime += nIntLowTime;
            
            for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
            {
                for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVol]; nDevIdx++)
                {
                    nPDevIdx   = nVol * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx;
                    nNumOfDies = gpstOND4kCxt[nPDevIdx]->pstOND4kSpec->nNumOfDies;

                    for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                    {
                        if (gpstOND4kCxt[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                        {
                            gpstOND4kCxt[nPDevIdx]->nIntLowTime[nDieIdx] -= nIntLowTime;
                        }
                    }
                }
            }
        }

        pstOND4kCxt->nIntLowTime[nDie] = pstOND4kSpec->nTEraseTime;
        break;

    case FSR_OND_4K_STAT_SLC_LOAD:
        pstOND4kCxt->nNumOfSLCLoads++;

        if(nIntLowTime > 0)
        {
            /* wait INT for previous operation */
            gnElapsedTime += nIntLowTime;
            
            for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
            {
                for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVol]; nDevIdx++)
                {
                    nPDevIdx   = nVol * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx;
                    nNumOfDies = gpstOND4kCxt[nPDevIdx]->pstOND4kSpec->nNumOfDies;

                    for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                    {
                        if (gpstOND4kCxt[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                        {
                            gpstOND4kCxt[nPDevIdx]->nIntLowTime[nDieIdx] -= nIntLowTime;
                        }
                    }
                }
            }
        }

        if (nCmdOption == FSR_OND_4K_STAT_PLOAD)
        {
            pstOND4kCxt->nIntLowTime[nDie] = FSR_OND_4K_RD_SW_OH + 
                pstOND4kSpec->nSLCTLoadTime - FSR_OND_4K_PAGEBUF_TO_DATARAM_TIME;
        }
        else
        {
            pstOND4kCxt->nIntLowTime[nDie] = FSR_OND_4K_RD_SW_OH + 
                                               pstOND4kSpec->nSLCTLoadTime;
        }
        
        pstOND4kCxt->nPreCmdOption[nDie] = nCmdOption;
        break;

    case FSR_OND_4K_STAT_RD_TRANS:
        pstOND4kCxt->nNumOfRdTrans++;
        pstOND4kCxt->nRdTransInBytes  += nBytes;
        nTransferTime = nBytes * pstOND4kCxt->nRdTranferTime / 2 / 1000;


        if ((nCmdOption != FSR_OND_4K_STAT_PLOAD) && (nIntLowTime > 0))
        {
            gnElapsedTime += nIntLowTime;

            for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
            {
                for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVol]; nDevIdx++)
                {
                    nPDevIdx   = nVol * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx;
                    nNumOfDies = gpstOND4kCxt[nPDevIdx]->pstOND4kSpec->nNumOfDies;

                    for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                    {
                        if (gpstOND4kCxt[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                        {
                            gpstOND4kCxt[nPDevIdx]->nIntLowTime[nDieIdx] -= nIntLowTime;
                        }
                    }
                }
            }
        }

        gnElapsedTime += nTransferTime;

        for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
        {
            for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVol]; nDevIdx++)
            {
                nPDevIdx   = nVol * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx;
                nNumOfDies = gpstOND4kCxt[nPDevIdx]->pstOND4kSpec->nNumOfDies;

                for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                {
                    if (gpstOND4kCxt[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                    {
                        gpstOND4kCxt[nPDevIdx]->nIntLowTime[nDieIdx] -= nTransferTime;
                    }
                }
            }
        }

        if (nCmdOption == FSR_OND_4K_STAT_PLOAD)
        {
            pstOND4kCxt->nIntLowTime[nDie] = FSR_OND_4K_PAGEBUF_TO_DATARAM_TIME;
        }
        else
        {
            pstOND4kCxt->nIntLowTime[nDie] = 0;
        }

        break;

    case FSR_OND_4K_STAT_WR_TRANS:
        pstOND4kCxt->nNumOfWrTrans++;
        pstOND4kCxt->nWrTransInBytes  += nBytes;
        nTransferTime   = nBytes * pstOND4kCxt->nWrTranferTime / 2 / 1000;

        /* cache operation can hide transfer time */
        if((pstOND4kCxt->nPreCmdOption[nDie] == FSR_OND_4K_STAT_CACHE_PGM) && (nIntLowTime >= 0))
        {
            gnElapsedTime  += nTransferTime;

            for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
            {
                for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVol]; nDevIdx++)
                {
                    nPDevIdx   = nVol * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx;
                    nNumOfDies = gpstOND4kCxt[nPDevIdx]->pstOND4kSpec->nNumOfDies;

                    for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                    {
                        if (gpstOND4kCxt[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                        {
                            gpstOND4kCxt[nPDevIdx]->nIntLowTime[nDieIdx] -= nTransferTime;
                        }
                    }
                }
            }

            if(nIntLowTime < nTransferTime)
            {
                pstOND4kCxt->nIntLowTime[nDie] = 0;
            }
        }
        else /* transfer time cannot be hided */
        {
            if(nIntLowTime > 0)
            {
                gnElapsedTime += nIntLowTime;

                for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
                {
                    for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVol]; nDevIdx++)
                    {
                        nPDevIdx   = nVol * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx;
                        nNumOfDies = gpstOND4kCxt[nPDevIdx]->pstOND4kSpec->nNumOfDies;

                        for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                        {
                            if (gpstOND4kCxt[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                            {
                                gpstOND4kCxt[nPDevIdx]->nIntLowTime[nDieIdx] -= nIntLowTime;
                            }
                        }
                    }
                }
            }

            gnElapsedTime  += nTransferTime;

            for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
            {
                for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVol]; nDevIdx++)
                {
                    nPDevIdx   = nVol * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx;
                    nNumOfDies = gpstOND4kCxt[nPDevIdx]->pstOND4kSpec->nNumOfDies;

                    for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                    {
                        if (gpstOND4kCxt[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                        {
                            gpstOND4kCxt[nPDevIdx]->nIntLowTime[nDieIdx] -= nTransferTime;
                        }
                    }
                }
            }

            pstOND4kCxt->nIntLowTime[nDie] = 0;
        }


        break;

    case FSR_OND_4K_STAT_FLUSH:

        if (nIntLowTime > 0)
        {
            /* wait INT for previous operation */
            gnElapsedTime += nIntLowTime;
            
            for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
            {
                for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVol]; nDevIdx++)
                {
                    nPDevIdx   = nVol * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx;
                    nNumOfDies = gpstOND4kCxt[nPDevIdx]->pstOND4kSpec->nNumOfDies;

                    for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                    {
                        if (gpstOND4kCxt[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                        {
                            gpstOND4kCxt[nPDevIdx]->nIntLowTime[nDieIdx] -= nIntLowTime;
                        }
                    }
                }
            }

            pstOND4kCxt->nIntLowTime[nDie] = 0;
        }
        break;

    default:
        break;
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s()\r\n"), __FSR_FUNC__));
}


/**
 * @brief          this function is same as _AddOND4kStat().
 * @n              but in the needs of classifying SLC
 *
 * @param[in]       nDev        : Physical Device Number (0 ~ 3)
 * @param[in]       nDie        : die(chip) number (0 or 1) for DDP device
 * @param[in]       nPbn        : Physical block  Number
 * @param[in]       nPage       : page offset within the block
 * @param[in]       nCmdOption  : command option such as cache, superload
 * @n                             which hide transfer time.
 *
 * @return          none
 *
 * @author          NamOh Hwang / KyungHo Shin
 * @version         1.1.0
 * @remark          Pbn is consecutive numbers within the device
 * @n               i.e. for DDP device, Pbn for 1st block in 2nd chip is not 0
 * @n               it is one more than the last block number in 1st chip
 *
 */
PRIVATE VOID
_AddOND4kStatPgm(UINT32   nDev,
               UINT32   nDie,
               UINT32   nPbn,
               UINT32   nPage,
               UINT32   nCmdOption)
{
    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[OND:IN ] ++%s()\r\n"), __FSR_FUNC__));

    /* program is performed on SLC block */
    _AddOND4kStat(nDev, nDie, FSR_OND_4K_STAT_SLC_PGM, 0, nCmdOption);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s()\r\n"), __FSR_FUNC__));
}


/**
 * @brief           this function is same as _AddOND4kStat().
 * @n               but in need of classifying SLC, MLC load
 *
 * @param[in]       nDev        : Physical Device Number (0 ~ 3)
 * @param[in]       nDie        : die(chip) number (0 or 1) for DDP device
 * @param[in]       nPbn        : Physical block  Number
 * @param[in]       nCmdOption  : command option such as cache, superload
 * @n                             which hides transfer time.
 *
 * @return          none
 *
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark          Pbn is consecutive numbers within the device
 * @n               i.e. for DDP device, Pbn for 1st block in 2nd chip is not 0
 * @n               it is one more than the last block number in 1st chip
 *
 */
PRIVATE VOID
_AddOND4kStatLoad(UINT32   nDev,
                UINT32   nDie,
                UINT32   nPbn,
                UINT32   nCmdOption)
{
    OneNAND4kCxt  *pstOND4kCxt  = gpstOND4kCxt[nDev];

    FSR_STACK_VAR;

    FSR_STACK_END;



    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[OND:IN ] ++%s()\r\n"), __FSR_FUNC__));

    if (pstOND4kCxt->nBlksForSLCArea[nDie] > (nPbn & pstOND4kCxt->nFBAMask))
    {
        /* operation is performed on SLC block */
        _AddOND4kStat(nDev, nDie, FSR_OND_4K_STAT_SLC_LOAD, 0, nCmdOption);

    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s()\r\n"), __FSR_FUNC__));
}
#endif /* #if defined (FSR_LLD_STATISTICS) */

#if defined (FSR_LLD_LOGGING_HISTORY)
/**
 * @brief           This function leave a trace for the LLD function call
 *
 * @param[in]       nDev        : Physical Device Number (0 ~ 3)
 * @param[in]       nDie        : Die(Chip) index
 *
 * @return          none
 *
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark
 *
 */
VOID
_AddLog(UINT32 nDev, UINT32 nDie)
{
             OneNAND4kShMem *pstOND4kShMem;
    volatile OneNAND4kOpLog *pstOND4kOpLog;

    UINT32 nLogHead;

    FSR_STACK_VAR;

    FSR_STACK_END;



    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG ,
        (TEXT("[OND:IN ] ++%s(nDie: %d)\r\n"), __FSR_FUNC__, nDie));

    pstOND4kShMem = gpstOND4kShMem[nDev];

    pstOND4kOpLog = &gstOND4kOpLog[nDev];

    nLogHead = pstOND4kOpLog->nLogHead;

    pstOND4kOpLog->nLogOp       [nLogHead]  = pstOND4kShMem->nPreOp[nDie];
    pstOND4kOpLog->nLogPbn      [nLogHead]  = pstOND4kShMem->nPreOpPbn[nDie];
    pstOND4kOpLog->nLogPgOffset [nLogHead]  = pstOND4kShMem->nPreOpPgOffset[nDie];
    pstOND4kOpLog->nLogFlag     [nLogHead]  = pstOND4kShMem->nPreOpFlag[nDie];

    pstOND4kOpLog->nLogHead = (nLogHead + 1) & (FSR_4K_OND_MAX_LOG -1);


    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s()\r\n"), __FSR_FUNC__));
}
#endif /* #if defined (FSR_LLD_LOGGING_HISTORY) */


/**
 * @brief           this function initialize the structure for LLD statistics
 *
 * @return          FSR_LLD_SUCCESS
 *
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark
 *
 */
PUBLIC INT32
FSR_OND_4K_InitLLDStat(VOID)
{
#if defined (FSR_LLD_STATISTICS)
    OneNAND4kCxt *pstOND4kCxt;

    UINT32      nVolIdx;
    UINT32      nDevIdx;
    UINT32      nDieIdx;

    FSR_STACK_VAR;

    FSR_STACK_END;



    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:IN ] ++%s()\r\n"), __FSR_FUNC__));

    gnElapsedTime = 0;

    for (nVolIdx = 0; nVolIdx < FSR_MAX_VOLS; nVolIdx++)
    {
        for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVolIdx]; nDevIdx++)
        {

            pstOND4kCxt = gpstOND4kCxt[nVolIdx * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx];

            pstOND4kCxt->nNumOfSLCLoads = 0;

            pstOND4kCxt->nNumOfSLCLoads = 0;
            pstOND4kCxt->nNumOfMLCLoads = 0;

            pstOND4kCxt->nNumOfSLCPgms  = 0;
            pstOND4kCxt->nNumOfLSBPgms  = 0;
            pstOND4kCxt->nNumOfMSBPgms  = 0;

            pstOND4kCxt->nNumOfCacheBusy= 0;
            pstOND4kCxt->nNumOfErases   = 0;

            pstOND4kCxt->nNumOfRdTrans  = 0;
            pstOND4kCxt->nNumOfWrTrans  = 0;

            pstOND4kCxt->nRdTransInBytes= 0;
            pstOND4kCxt->nWrTransInBytes= 0;

            for (nDieIdx = 0; nDieIdx < FSR_MAX_DIES; nDieIdx++)
            {
                pstOND4kCxt->nPreCmdOption[nDieIdx] = FSR_OND_4K_STAT_NORMAL_CMD;
                pstOND4kCxt->nIntLowTime[nDieIdx]   = 0;
            }
        }
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s()\r\n"), __FSR_FUNC__));
#endif
    return FSR_LLD_SUCCESS;
}


/**
 * @brief           this function totals the time device consumed.
 *
 * @param[out]      pstStat : the pointer to the structure, FSRLLDStat
 *
 * @return          total busy time of whole device
 *
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark
 *
 */
PUBLIC INT32
FSR_OND_4K_GetStat(FSRLLDStat *pstStat)
{
#if defined (FSR_LLD_STATISTICS)
    OneNAND4kCxt  *pstOND4kCxt;
    OneNAND4kSpec *pstOND4kSpec;

    UINT32       nDevIdx;
    UINT32       nTotalTime;
    UINT32       nVolIdx;

    FSR_STACK_VAR;

    FSR_STACK_END;



    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:IN ] ++%s()\r\n"), __FSR_FUNC__));

    FSR_OAM_MEMSET(pstStat, 0x00, sizeof(FSRLLDStat));

    nTotalTime    = 0;

    for (nVolIdx = 0; nVolIdx < FSR_MAX_VOLS; nVolIdx++)
    {
        for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVolIdx]; nDevIdx++)
        {
            pstOND4kCxt = gpstOND4kCxt[nVolIdx * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx];

            pstStat->nSLCPgms        += pstOND4kCxt->nNumOfSLCPgms;
            pstStat->nLSBPgms        += pstOND4kCxt->nNumOfLSBPgms;
            pstStat->nMSBPgms        += pstOND4kCxt->nNumOfMSBPgms;

            pstStat->nErases         += pstOND4kCxt->nNumOfErases;

            pstStat->nSLCLoads       += pstOND4kCxt->nNumOfSLCLoads;
            pstStat->nMLCLoads       += pstOND4kCxt->nNumOfMLCLoads;

            pstStat->nRdTrans        += pstOND4kCxt->nNumOfRdTrans;
            pstStat->nWrTrans        += pstOND4kCxt->nNumOfWrTrans;

            pstStat->nCacheBusy      += pstOND4kCxt->nNumOfCacheBusy;

            pstStat->nRdTransInBytes += pstOND4kCxt->nRdTransInBytes;
            pstStat->nWrTransInBytes += pstOND4kCxt->nWrTransInBytes;


            pstOND4kSpec    = pstOND4kCxt->pstOND4kSpec;

            /* nTotaltime can be compared with gnElapsedTime */
            nTotalTime   += pstStat->nSLCLoads * pstOND4kSpec->nSLCTLoadTime;

            nTotalTime   += pstStat->nSLCPgms  * pstOND4kSpec->nSLCTProgTime;

            nTotalTime   += pstStat->nErases   * pstOND4kSpec->nTEraseTime;

            /* pstOND4kCxt->nRdTranferTime, pstOND4kCxt->nWrTranferTime is time for transfering 2 bytes
             * which is nano second base
             * to get a time in micro second, divide it by 1000
             */
            nTotalTime   += pstStat->nRdTransInBytes * pstOND4kCxt->nRdTranferTime / 2 / 1000;
            nTotalTime   += pstStat->nWrTransInBytes * pstOND4kCxt->nWrTranferTime / 2 / 1000;
            nTotalTime   += pstStat->nCacheBusy      * FSR_OND_4K_CACHE_BUSY_TIME;

        }
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s()\r\n"), __FSR_FUNC__));

    return gnElapsedTime;
#else
    FSR_OAM_MEMSET(pstStat, 0x00, sizeof(FSRLLDStat));
    return 0;
#endif /* #if defined (FSR_LLD_STATISTICS) */
}

#if defined (FSR_LLD_HANDSHAKE_ERR_INF)
/**
 * @brief           this function saves the contents of DataRAM & ctrl register
 *
 * @param[in]       nDev        : Physical Device Number (0 ~ 3)
 * @param[in]       nDie        : die(chip) number (0 or 1) for DDP device
 * @param[in]       pstOND4kCxt   : pointer to OneNAND4kCxt
 * @param[in]       nLLDRe      : error type
 *
 * @return          none
 *
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark
 *
 */
PRIVATE VOID
_SaveErrorCxt(            UINT32           nDev,
                          UINT32           nDie,
                          OneNAND4kCxt      *pstOND4kCxt,
                          INT32            nLLDRe)
{
                OneNAND4kSpec      *pstOND4kSpec;
    volatile    OneNAND4kSharedCxt *pstOND4kSharedCxt;

    volatile    OneNAND4kReg   *pstFOReg = (volatile OneNAND4kReg *) pstOND4kCxt->nBaseAddr;

    FSR_STACK_VAR;

    FSR_STACK_END;



    FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_LOG | FSR_DBZ_ERROR,
        (TEXT("[OND:IN ] ++%s(nDev: %d, nDie:%d, nLLDRe:0x%08x)\r\n"),
        __FSR_FUNC__, nDev, nDie, nLLDRe));


    pstOND4kSharedCxt = &gstOND4kSharedCxt[nDev];

    /* save the error returned */
    pstOND4kSharedCxt->nHostLLDRe[nDie]  = nLLDRe;

    /* to print debug message */
    pstOND4kSharedCxt->nErrCtrlReg[nDie] = OND_4K_READ(pstFOReg->nCtrlStat);

    FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
        (TEXT("[OND:INF]   pstOND4kSharedCxt->nHostLLDRe[%d] = 0x%08x\r\n"),
        pstOND4kSharedCxt->nHostLLDRe[nDie]));



    pstOND4kSpec   = pstOND4kCxt->pstOND4kSpec;

    if (FSR_RETURN_MAJOR(nLLDRe) == FSR_LLD_PREV_WRITE_ERROR)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
            (TEXT("[OND:INF]   saving the error context for write error\r\n")));

        /* save DataRAM for bad block handling */
        TRANSFER_FROM_NAND((UINT8 *) &pstOND4kSharedCxt->nErrMainDataRAM[nDie][0],
                           &pstFOReg->nDataMB00[0],
                           pstOND4kSpec->nSctsPerPG * FSR_OND_4K_SECTOR_SIZE);

        TRANSFER_FROM_NAND((UINT8 *) &pstOND4kSharedCxt->nErrSpareDataRAM[nDie][0],
                           &pstFOReg->nDataSB00[0],
                           pstOND4kSpec->nSctsPerPG * pstOND4kSpec->nSparePerSct);
    }

    if (FSR_RETURN_MAJOR(nLLDRe) == FSR_LLD_PREV_ERASE_ERROR)
    {
        /* do nothing for erase error */
        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
            (TEXT("[OND:INF]   saving the error context for erase error\r\n")));
    }

    FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_LOG | FSR_DBZ_ERROR,
        (TEXT("[OND:OUT] --%s()\r\n"), __FSR_FUNC__));
}



/**
 * @brief           this function restores the contents of DataRAM & ctrl register
 *
 * @param[in]       nDev        : Physical Device Number (0 ~ 3)
 * @param[in]       nDie        : die(chip) number (0 or 1) for DDP device
 * @param[in]       pstOND4kCxt   : pointer to OneNAND4kCxt
 * @param[in]       pstFOReg    : pointer to OneNAND4kReg
 *
 * @return          FSR_LLD_SUCCESS
 * @return          FSR_LLD_PREV_WRITE_ERROR  | {FSR_LLD_1STPLN_CURR_ERROR}
 * @n               error for normal program
 * @return          FSR_LLD_PREV_WRITE_ERROR  | {FSR_LLD_1STPLN_PREV_ERROR}
 * @n               error for cache program
 * @return          FSR_LLD_PREV_ERASE_ERROR  | {FSR_LLD_1STPLN_CURR_ERROR}
 * @n               erase error for previous erase operation
 *
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark
 *
 */


PRIVATE INT32
_CheckErrorCxt(              UINT32           nDev,
                             UINT32           nDie,
                             OneNAND4kCxt      *pstOND4kCxt,
                 volatile    OneNAND4kReg  *pstFOReg)
{
                OneNAND4kSpec      *pstOND4kSpec;
                OneNAND4kShMem     *pstOND4kShMem = gpstOND4kShMem[nDev];
    volatile    OneNAND4kSharedCxt *pstOND4kSharedCxt;

    INT32             nLLDRe;

    FSR_STACK_VAR;

    FSR_STACK_END;



    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[OND:IN ] ++%s(nDev:%d, nDie:%d)\r\n"),
        __FSR_FUNC__, nDev, nDie));

    pstOND4kSharedCxt = &gstOND4kSharedCxt[nDev];

    nLLDRe          = pstOND4kSharedCxt->nHostLLDRe[nDie];

    if (FSR_RETURN_MAJOR(nLLDRe) == FSR_LLD_PREV_WRITE_ERROR)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
            (TEXT("            CtrlReg=0x%04x\r\n"),
            pstOND4kSharedCxt->nErrCtrlReg[nDie]));

        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
            (TEXT("            prev Op:%d @ nDev:%d, nPbn:%d, nPgOffset:%d, nFlag:0x%08x\r\n"),
            pstOND4kShMem->nPreOp[nDie], nDev, pstOND4kShMem->nPreOpPbn[nDie],
            pstOND4kShMem->nPreOpPgOffset[nDie], pstOND4kShMem->nPreOpFlag[nDie]));

        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
            (TEXT("[OND:INF]   restoring the error cxt for write error\r\n")));

        pstOND4kSpec = pstOND4kCxt->pstOND4kSpec;

        TRANSFER_TO_NAND(&pstFOReg->nDataMB00[0],
                         (UINT8 *) &pstOND4kSharedCxt->nErrMainDataRAM[nDie][0],
                         pstOND4kSpec->nSctsPerPG * FSR_OND_4K_SECTOR_SIZE);

        TRANSFER_TO_NAND(&pstFOReg->nDataSB00[0],
                         (UINT8 *) &pstOND4kSharedCxt->nErrSpareDataRAM[nDie][0],
                         pstOND4kSpec->nSctsPerPG * pstOND4kSpec->nSparePerSct);
    }

    if (FSR_RETURN_MAJOR(nLLDRe) == FSR_LLD_PREV_ERASE_ERROR)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
            (TEXT("[OND:INF]   restoring the error cxt for erase error\r\n")));

        /* do nothing */
    }

    pstOND4kSharedCxt->nHostLLDRe[nDie] = FSR_LLD_SUCCESS;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s()\r\n"), __FSR_FUNC__));

    return nLLDRe;
}
#endif



/**
 * @brief           This function gets the unique ID from OTP Block
 *
 * @param[in]       nDev        : Physical Device Number (0 ~ 7)
 * @param[in]       pstOND4kCxt   : pointer to the structure OneNAND4kCxt
 * @param[in]       nFlag       : bypass nFlag from GetDevSpec()
 *
 * @return          FSR_LLD_SUCCESS
 *
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark
 *
 */
PRIVATE INT32
_GetUniqueID(UINT32         nDev,
             OneNAND4kCxt    *pstOND4kCxt,
             UINT32         nFlag)
{
#if defined (FSR_LLD_HANDSHAKE_ERR_INF)
    volatile    OneNAND4kSharedCxt *pstOND4kSharedCxt = NULL;
#endif
                OneNAND4kSpec *pstOND4kSpec;

    UINT32       nDie = 0;
    UINT32       nPbn;
    UINT32       nPgOffset;
    UINT32       nByteRet;
    UINT32       nIdx;
    UINT32       nCnt;
    UINT32       nSizeOfUID;
    UINT32       nSparePerSct;

    INT32        nLLDRe = FSR_LLD_SUCCESS;
    INT32        nRet   = 0;

    BOOL32       bValidUID;

    UINT8       *pUIDArea   = NULL;
    UINT8       *pSignature = NULL;

    FSR_STACK_VAR;

    FSR_STACK_END;



    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF,
        (TEXT("[OND:IN ] ++%s(nDev:%d, nDie:%d)\r\n"),
        __FSR_FUNC__, nDev, nDie));


    do
    {
        /* set OTP access mode */
        nLLDRe = FSR_OND_4K_IOCtl(nDev,                     /*  nDev      : Physical Device Number (0 ~ 7)   */
                               FSR_LLD_IOCTL_OTP_ACCESS, /*  nCode     : IO Control Command               */
                               (UINT8 *) &nDie,          /* *pBufI     : Input Buffer pointer             */
                               sizeof(nDie),             /*  nLenI     : Length of Input Buffer           */
                               NULL,                     /* *pBufO     : Output Buffer pointer            */
                               0,                        /*  nLenO     : Length of Output Buffer          */
                               &nByteRet);               /* *pByteRet  : The number of bytes (length) of 
                                                                         Output Buffer as the result of 
                                                                         function call                    */

        if (FSR_RETURN_MAJOR(nLLDRe) != FSR_LLD_SUCCESS)
        {
            break;
        }


        FSR_ASSERT(pstOND4kCxt != NULL);
        FSR_ASSERT(pstOND4kCxt->pstOND4kSpec != NULL);

        pstOND4kSpec   = pstOND4kCxt->pstOND4kSpec;
        nSparePerSct = pstOND4kSpec->nSparePerSct;

        FSR_OAM_MEMSET(pstOND4kCxt->pTempBuffer, 0x00,
            pstOND4kSpec->nSctsPerPG * (FSR_SECTOR_SIZE + nSparePerSct));


        /* Unique ID is located at page 60 of OTP block */
        nPgOffset = 60;
        nPbn      = 0;

        pUIDArea   = pstOND4kCxt->pTempBuffer;
        pSignature = pstOND4kCxt->pTempBuffer + (pstOND4kSpec->nSctsPerPG * FSR_SECTOR_SIZE);

        nLLDRe = FSR_OND_4K_Read(nDev, nPbn, nPgOffset,
                              pUIDArea, (FSRSpareBuf *) pSignature,
                              FSR_LLD_FLAG_DUMP_ON      | FSR_LLD_FLAG_ECC_OFF |
                              FSR_LLD_FLAG_USE_SPAREBUF | nFlag);

        if (FSR_RETURN_MAJOR(nLLDRe) != FSR_LLD_SUCCESS)
        {
            break;
        }

        /*
         *   ##UniqueID Verification sequence##
         * STEP 1. check 16 bytes data in spare area.
         *         - the set must be checked minimum 1set out of 4sets
         *         - 1set has a value that '0x5555AAAA(4B)' is repeated four times
         * STEP 2. check complement data in data area
         *         - the set must be checked minimum 1set out of 8sets
         */
        /* STEP 1 : check signature */
        /* | Spare0 (16B) | Spare1 (16B) | Spare2 (16B) | Spare3 (16B) |
         * |--------------+--------------+--------------+--------------|
         * |  0xAAAA5555  |                All 0xFFFF                  |
         */

        bValidUID = FALSE32;
        for (nIdx = 0; nIdx < nSparePerSct; nIdx += 4)
        {
            if ((pSignature[nIdx] == 0x55) && (pSignature[nIdx + 2] == 0xAA))
            {
                bValidUID = TRUE32;
                break;
            }
        }

        if (bValidUID != TRUE32)
        {
            break;
        }

        /* STEP 2 : check complenent data */
        /* |   Unique ID (512B)  | Reserved (512B) | Reserved (512B) | Reserved (512B) |
         * |---------------------+-----------------+-----------------+-----------------|
         * | ID(32B) & Cplmt(32B)|                    All 0xFFFF                       |
         */
        bValidUID = FALSE32;

        /* nSizeOfUID is 32 byte */
        nSizeOfUID = (FSR_SECTOR_SIZE / FSR_OND_4K_NUM_OF_UID) / 2;

        for (nCnt = 0; nCnt < FSR_OND_4K_NUM_OF_UID; nCnt++)
        {
            for (nIdx = 0; nIdx < nSizeOfUID; nIdx++)
            {
                pUIDArea[nSizeOfUID + nIdx] = (UINT8) ~pUIDArea[nSizeOfUID + nIdx];
            }

            nRet = FSR_OAM_MEMCMP(pUIDArea, pUIDArea + nSizeOfUID, nSizeOfUID);

            if (nRet == 0)
            {
                bValidUID = TRUE32;
                break;
            }

            /* skip 64 byte */
            pUIDArea += (FSR_SECTOR_SIZE / FSR_OND_4K_NUM_OF_UID);
        }

        if (bValidUID == FALSE32)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF, (TEXT("[OND:ERR]   Invalid UID\r\n")));
            break;
        }

        /* Get Unique ID */
        for (nIdx = 0; nIdx < FSR_LLD_UID_SIZE; nIdx++)
        {
            pstOND4kCxt->nUID[nIdx] = pUIDArea[2 * nIdx];
        }

    } while (0);

    /* exit the OTP block */
    nLLDRe = FSR_OND_4K_IOCtl(nDev,                         /*  nDev      : Physical Device Number (0 ~ 7)   */
                           FSR_LLD_IOCTL_CORE_RESET,     /*  nCode     : IO Control Command               */
                           NULL,                         /* *pBufI     : Input Buffer pointer             */
                           0,                            /*  nLenI     : Length of Input Buffer           */
                           NULL,                         /* *pBufO     : Output Buffer pointer            */
                           0,                            /*  nLenO     : Length of Output Buffer          */
                           &nByteRet);                   /* *pByteRet  : The number of bytes (length) of 
                                                                         Output Buffer as the result of 
                                                                         function call                    */

#if defined (FSR_LLD_HANDSHAKE_ERR_INF)

    pstOND4kSharedCxt = &gstOND4kSharedCxt[nDev];
    pstOND4kSharedCxt->nPrevFSRMode[nDie]     = nFlag & FSR_LLD_FLAG_READ_ONLY;
    pstOND4kSharedCxt->nHostLLDOp[nDie]       = FSR_OND_4K_PREOP_IOCTL;

    pstOND4kSharedCxt->nHostLLDPbn[nDie]      = FSR_OND_4K_PREOP_ADDRESS_NONE;
    pstOND4kSharedCxt->nHostLLDPgOffset[nDie] = FSR_OND_4K_PREOP_ADDRESS_NONE;
    pstOND4kSharedCxt->nHostLLDFlag[nDie]     = nFlag;
#endif

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF,
        (TEXT("[OND:OUT] --%s()\r\n"), __FSR_FUNC__));

    return (nLLDRe);
}
