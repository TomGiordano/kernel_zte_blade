/**
 *   @mainpage   Flex Sector Remapper : LinuStoreIII_1.2.0_b035-FSR_1.2.1p1_b129_RC
 *
 *   @section Intro
 *       Flash Translation Layer for Flex-OneNAND and OneNAND
 *    
 *   @section  Copyright
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
 *   @section Description
 *
 */

/**
 * @file      FSR_LLD_FlexOND.c
 * @brief     This file implements Low Level Driver for Flex-OneNAND
 * @author    NamOh Hwang
 * @author    SongHo Yoon
 * @date      10-JAN-2007
 * @remark
 * REVISION HISTORY
 * @n  10-JAN-2007 [NamOh Hwang]  : first writing
 * @n  13-MAY-2009 [Kyungho Shin] : Support tinyFSR in dual core
 */

/******************************************************************************/
/* Header file inclusions                                                     */
/******************************************************************************/
#define     FSR_NO_INCLUDE_BML_HEADER
#define     FSR_NO_INCLUDE_STL_HEADER

#include    "FSR.h"
#include    "FSR_LLD_FlexOND.h"

/******************************************************************************/
/*   Local Configurations                                                     */
/*                                                                            */
/* - FSR_LLD_STRICT_CHK             : To check parameters strictly            */
/* - FSR_LLD_STATISTICS             : To accumulate statistics.               */
/* - FSR_LLD_LOGGING_HISTORY        : To log history                          */
/* - FSR_LLD_USE_CACHE_PGM          : To support cache program                */
/* - FSR_LLD_WAIT_ALLDIE_PGM_READY  : To wait until all program are finished  */
/* - FSR_LLD_USE_SUPER_LOAD         : To support super load                   */
/* - FSR_LLD_WAIT_WR_PROTECT_STAT   : To wait for setting WR Protect register */
/* - FSR_LLD_ENABLE_DEBUG_PORT      : To enable debug port                    */
/* - FSR_LLD_READ_PRELOADED_DATA    : To re-use preload data                  */
/* - FSR_LLD_RUNTIME_ERASE_REFRESH  : To support runtime erase refresh        */
/******************************************************************************/
#define     FSR_LLD_STRICT_CHK
//#define     FSR_LLD_STATISTICS
#define     FSR_LLD_LOGGING_HISTORY
#define     FSR_LLD_USE_CACHE_PGM
//#define     FSR_LLD_WAIT_ALLDIE_PGM_READY
#define     FSR_LLD_USE_SUPER_LOAD
//#define     FSR_LLD_WAIT_WR_PROTECT_STAT
//#define     FSR_LLD_ENABLE_DEBUG_PORT
#define     FSR_LLD_READ_PRELOADED_DATA
//#define     FSR_LLD_RUNTIME_ERASE_REFRESH

#if defined(FSR_LLD_RUNTIME_ERASE_REFRESH)
/* Read count for erase refresh */
#define    FSR_RUNTIME_ER_MAX_READ_CNT        30000
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
#define     FSR_FND_MAX_DEVS                (FSR_MAX_DEVS)
#define     FSR_FND_1ST_DIE                 (0)
#define     FSR_FND_2ND_DIE                 (1)

#define     FSR_FND_DID_MASK                (0xFFF8) /* Device ID Register    */
#define     FSR_FND_DID_VCC_MASK            (0x0003) /* Device ID Register    */
#define     FSR_FND_DID_VCC_33V             (0x0001) /* Device ID Register    */
#define     FSR_FND_STEPPING_ID_MASK        (0x000F) /* Version ID Register   */
#define     FSR_FND_CS_VERSION              (0x0001) /* CS version not support
                                                        Erasing of PI Block   */
#define     FSR_FND_INT_MASTER_READY        (0x8000) /* Interrupt status reg. */
#define     FSR_FND_INT_READ_READY          (0x0080)
#define     FSR_FND_INT_WRITE_READY         (0x0040)
#define     FSR_FND_INT_ERASE_READY         (0x0020)
#define     FSR_FND_INT_RESET_READY         (0x0010)
#define     FSR_FND_DFS_BASEBIT             (15)     /* Start address 1 reg.  */
#define     FSR_FND_DFS_MASK                (0x8000)
#define     FSR_FND_DBS_BASEBIT             (15)     /* Start address 2 reg.  */
#define     FSR_FND_DBS_MASK                (0x8000)
#define     FSR_FND_STATUS_ERROR            (0x0400) /* Controller status reg.*/
#define     FSR_FND_PREV_CACHEPGM_ERROR     (0x0004) /* Controller status reg.*/
#define     FSR_FND_CURR_CACHEPGM_ERROR     (0x0002) /* Controller status reg.*/

#define     FSR_FND_ECC_UNCORRECTABLE       (0x1010) /* ECC status reg.1,2,3,4*/

/* The size of array whose element can be put into command register           */
#define     FSR_FND_MAX_PGMCMD              (5)  /* The size of gnPgmCmdArray */
#define     FSR_FND_MAX_LOADCMD             (6)  /* The size of gnLoadCmdArray*/
#define     FSR_FND_MAX_ERASECMD            (1)  /* The size of gnEraseCmdArra*/
#define     FSR_FND_MAX_CPBKCMD             (3)  /* The size of gnCpBkCmdArray*/

#define     FSR_FND_MAX_BADMARK             (4)  /* The size of gnBadMarkValue*/
#define     FSR_FND_MAX_BBMMETA             (2)  /* The size of gnBBMMetaValue*/


#define     FSR_FND_MAX_ECC_STATUS_REG      (4)  /* # of ECC status registers */

#define     FSR_FND_NUM_OF_BAD_MARK_PAGES   (2)
#define     FSR_FND_VALID_BLK_MARK          (0xFFFF)


#define     FSR_FND_SECTOR_SIZE             (FSR_SECTOR_SIZE)

/* Hardware ECC of Flex-OneNAND use five UINT16 (10 bytes) of spare area      */
#define     FSR_FND_SPARE_HW_ECC_AREA       (5)


/* Out of 16 bytes in spare area of 1 sector, LLD use 6 bytes                 */
#define     FSR_FND_SPARE_USER_AREA         (6)

#define     FSR_FND_BUSYWAIT_TIME_LIMIT     (10000000)

/* Write protect time out count */
#define     FSR_FND_WR_PROTECT_TIME_LIMIT   (25000000)

/* 
 * Transfering data between DataRAM & host DRAM depends on DMA & sync mode.
 * Because DMA & sync burst needs some preparation,
 * when the size of transferring data is small,
 * it's better to use load, store command of ARM processor.
 * Confer _ReadMain(), _WriteMain().
 * FSR_FND_MIN_BULK_TRANS_SIZE stands for minimum transfering size.
 */
#define     FSR_FND_MIN_BULK_TRANS_SIZE     (16 * 2)

/* 
 * Buffer Sector Count specifies the number of sectors to load
 * LLD does not support sector-based load.
 * Instead, LLD always load 1 page, and selectively transfer sectors.
 */
#define     FSR_FND_START_BUF_DEFAULT       (0x0800)

#define     FSR_FND_PI_LOCK                 (0x3F00)
#define     FSR_FND_PI_NOLOCK               (0xFF00)
#define     FSR_FND_PI_RSV_MASK             (0xFFFF)

#define     FSR_FND_PI_LOCK_MASK            (0xC000)

#define     FSR_FND_OTP_LOCK_MASK           (0x00FF)
#define     FSR_FND_LOCK_OTP_BLOCK          (0x00FC)
#define     FSR_FND_LOCK_1ST_BLOCK_OTP      (0x00F3)
#define     FSR_FND_LOCK_BOTH_OTP           (0x00F0)
#define     FSR_FND_OTP_PAGE_OFFSET         (49)

/* Controller status register                                                 */
#define     FSR_FND_CTLSTAT_OTP_MASK        (0x0060)
#define     FSR_FND_CTLSTAT_OTP_BLK_LOCKED  (0x0040)
#define     FSR_FND_CTLSTAT_1ST_OTP_LOCKED  (0x0020)

/* Flex-OneNAND command which is written to Command Register                  */
#define     FSR_FND_CMD_LOAD                (0x0000)
#define     FSR_FND_CMD_SUPERLOAD           (0x0003)
#define     FSR_FND_CMD_UPDATE_PI           (0x0005)
#define     FSR_FND_CMD_PROGRAM             (0x0080)
#define     FSR_FND_CMD_CACHEPGM            (0x007F)
#define     FSR_FND_CMD_UNLOCK_BLOCK        (0x0023)
#define     FSR_FND_CMD_LOCK_BLOCK          (0x002A)
#define     FSR_FND_CMD_LOCKTIGHT_BLOCK     (0x002C)
#define     FSR_FND_CMD_UNLOCK_ALLBLOCK     (0x0027)
#define     FSR_FND_CMD_ERASE               (0x0094)
#define     FSR_FND_CMD_RST_NFCORE          (0x00F0)
#define     FSR_FND_CMD_HOT_RESET           (0x00F3)
#define     FSR_FND_CMD_OTP_ACCESS          (0x0065)
#define     FSR_FND_CMD_ACCESS_PI           (0x0066)

/* 
 * 14th bit of Controller Status Register (F240h) of OneNAND shows
 * whether host is programming/erasing a locked block of the NAND Flash Array
 * Flex-OneNAND does not use this bit, and it is fixed to zero.
 */
#define     FSR_FND_LOCK_STATE              (0x4000)

/* 
 * With the help of emulator, LLD can run without the Flex-OneNAND
 * in that case, define FSR_ONENAND_EMULATOR
 */
#if defined (FSR_ONENAND_EMULATOR)

    #define     FND_WRITE(nAddr, nDQ)                                          \
                   {FSR_FOE_Write((UINT32)&(nAddr), nDQ);}

    #define     FND_READ(nAddr)                                                \
                   ((UINT16) FSR_FOE_Read((UINT32)&(nAddr)))

    #define     FND_SET(nAddr, nDQ)                                            \
                   {FSR_FOE_Write((UINT32)&(nAddr),                            \
                   (UINT16) FSR_FOE_Read((UINT32)&(nAddr)) | nDQ);}

    #define     FND_CLR(nAddr, nDQ)                                            \
                   {FSR_FOE_Write((UINT32)&(nAddr),                            \
                   (UINT16) FSR_FOE_Read((UINT32)&(nAddr)) & nDQ);}

    #define     TRANSFER_TO_NAND(pDst, pSrc, nSize)                            \
                    FSR_FOE_TransferToDataRAM(pDst, pSrc, nSize)

    /* nSize is the number of bytes to transfer                               */
    #define     TRANSFER_FROM_NAND(pDst, pSrc, nSize)                          \
                    FSR_FOE_TransferFromDataRAM(pDst, pSrc, nSize)

    /* Macro below is for emulation purpose */
    #define     FND_MEMSET_DATARAM(pDst, nVal, nSize)                          \
                    FSR_FOE_MemsetDataRAM((pDst), (nVal), (nSize))

#else /* #if defined (FSR_ONENAND_EMULATOR)                                   */

    #define     FND_WRITE(nAddr, nDQ)   {nAddr  = nDQ;}
    #define     FND_READ(nAddr)         (nAddr        )
    #define     FND_SET(nAddr, nDQ)     {nAddr  = (nAddr | nDQ);}
    #define     FND_CLR(nAddr, nDQ)     {nAddr  = (nAddr & nDQ);}

    /* nSize is the number of bytes to transfer                               */
    #define     TRANSFER_TO_NAND(pDst, pSrc, nSize)                            \
                    FSR_PAM_TransToNAND(pDst, pSrc, nSize)

    #define     TRANSFER_FROM_NAND(pDst, pSrc, nSize)                          \
                    FSR_PAM_TransFromNAND(pDst, pSrc, nSize)

    /* macro below is for emulation purpose
     * in real target environment, it's same as memset()
     */
    #define     FND_MEMSET_DATARAM(pDst, nVal, nSize)                          \
                    FSR_OAM_MEMSET((pDst), (nVal), (nSize))

#endif /* #if defined (FSR_ONENAND_EMULATOR)                                  */

/* In order to wait 200ns, call FND_READ(x->nInt) 12 times */
#define WAIT_FND_INT_STAT(x, a)                                                \
    {INT32 nTimeLimit = FSR_FND_BUSYWAIT_TIME_LIMIT;                           \
    FND_READ(x->nInt);                                                         \
    FND_READ(x->nInt);                                                         \
    FND_READ(x->nInt);                                                         \
    FND_READ(x->nInt);                                                         \
    FND_READ(x->nInt);                                                         \
    FND_READ(x->nInt);                                                         \
    FND_READ(x->nInt);                                                         \
    FND_READ(x->nInt);                                                         \
    FND_READ(x->nInt);                                                         \
    FND_READ(x->nInt);                                                         \
    while ((FND_READ(x->nInt) & (a)) != (UINT16)(a))                           \
    {                                                                          \
        if (--nTimeLimit == 0)                                                 \
        {                                                                      \
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,                                     \
                (TEXT("[FND:ERR]   busy wait time-out %s(), %d line\r\n"),     \
                __FSR_FUNC__, __LINE__));                                      \
            _DumpRegisters(x);                                                 \
            _DumpSpareBuffer(x);                                               \
            _DumpCmdLog();                                                     \
            FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,                  \
                (TEXT("[FND:OUT] --%s()\r\n"), __FSR_FUNC__));                 \
            return (FSR_LLD_NO_RESPONSE);                                      \
        }                                                                      \
    }}

/* In order to wait 3s, call FND_READ(x->nInt) 12 times */
#define WAIT_FND_WR_PROTECT_STAT(x, a)                                         \
    {INT32 nTimeLimit = FSR_FND_WR_PROTECT_TIME_LIMIT;                         \
    while ((FND_READ(x->nWrProtectStat) & (a)) != (UINT16)(a))          \
    {                                                                          \
        if (--nTimeLimit == 0)                                                 \
        {                                                                      \
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,                                     \
                (TEXT("[FND:ERR] WR protect busy wait time-out %s(), %d line\r\n"),\
                __FSR_FUNC__, __LINE__));                                      \
            _DumpRegisters(x);                                                 \
            _DumpSpareBuffer(x);                                               \
            _DumpCmdLog();                                                     \
            FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,                  \
                (TEXT("[FND:OUT] --%s()\r\n"), __FSR_FUNC__));                 \
            return (FSR_LLD_NO_RESPONSE);                                      \
        }                                                                      \
    }}


/* MACROs below are used for the statistic                                    */
#define FSR_FND_STAT_SLC_PGM                    (0x00000001)
#define FSR_FND_STAT_LSB_PGM                    (0x00000002)
#define FSR_FND_STAT_MSB_PGM                    (0x00000003)
#define FSR_FND_STAT_ERASE                      (0x00000004)
#define FSR_FND_STAT_SLC_LOAD                   (0x00000005)
#define FSR_FND_STAT_MLC_LOAD                   (0x00000006)
#define FSR_FND_STAT_RD_TRANS                   (0x00000007)
#define FSR_FND_STAT_WR_TRANS                   (0x00000008)
#define FSR_FND_STAT_WR_CACHEBUSY               (0x00000009)
#define FSR_FND_STAT_FLUSH                      (0x0000000A)

/* Command type for statistics                                                */
#define FSR_FND_STAT_NORMAL_CMD                 (0x0)
#define FSR_FND_STAT_PLOAD                      (0x1)
#define FSR_FND_STAT_CACHE_PGM                  (0x2)

#define FSR_FND_CACHE_BUSY_TIME                 (25) /* in usec               */
#define FSR_FND_PAGEBUF_TO_DATARAM_TIME         (25) /* in usec               */

/* SoftWare overHead                                                          */
#define FSR_FND_RD_SW_OH                        (0)  /* in usec               */
#define FSR_FND_WR_SW_OH                        (0)  /* in usec               */

/* Signature value about UniqueID */
#define FSR_FND_NUM_OF_UID                      (8)
#define FSR_FND_NUM_OF_SIG                      (4)

#define FSR_FND_FLUSHOP_CALLER_BASEBIT          (16)
#define FSR_FND_FLUSHOP_CALLER_MASK             (0xFFFF0000)

/******************************************************************************/
/* Local typedefs                                                             */
/******************************************************************************/
/** Data structure of Flex-OneNAND specification                              */
typedef struct
{
        UINT16      nMID;               /**< manufacturer ID                  */
        UINT16      nDID;               /**< bits 0~2 of nDID are
                                             masked with FSR_FND_DID_MASK     */

        UINT16      nGEN;               /**< process information              */
        UINT16      nNumOfBlks;         /**< the number of blocks             */

        UINT16      nNumOfDies;         /**< # of dies in NAND device         */
        UINT16      nNumOfPlanes;       /**< the number of planes             */

        UINT16      nSctsPerPG;         /**< the number of sectors per page   */
        UINT16      nSparePerSct;       /**< # of bytes of spare of a sector  */

        UINT32      nPgsPerBlkForSLC;   /**< # of pages per block in SLC area */
        UINT32      nPgsPerBlkForMLC;   /**< # of pages per block in MLC area */

        BOOL32      b1stBlkOTP;         /**< support 1st block OTP or not     */

        UINT16      nUserOTPScts;       /**< # of user sectors                */
        UINT16      nRsvBlksInDev;      /**< # of total bad blocks(init + run)*/

const   UINT8      *pPairedPgMap;       /**< paired page mapping information  */
const   UINT8      *pLSBPgMap;          /**< array of LSB pages               */

        /* 
         * TrTime, TwTime of MLC are array of size 2
         * First  element is for LSB TLoadTime, TProgTime
         * Second element is for MLB TLoadTime, TProgTime
         * Use macro FSR_LLD_IDX_LSB_TIME, FSR_LLD_IDX_MSB_TIME
         */
        UINT32      nSLCTLoadTime;      /**< Typical Load     operation time  */
        UINT32      nMLCTLoadTime;      /**< Typical Load     operation time  */
        UINT32      nSLCTProgTime;      /**< Typical Program  operation time  */
        UINT32      nMLCTProgTime[2];   /**< Typical Program  operation time  */
        UINT32      nTEraseTime;        /**< Typical Erase    operation time  */

        /* Endurance information                                              */
        UINT32      nSLCPECycle;        /**< program, erase cycle of SLC block*/
        UINT32      nMLCPECycle;        /**< program, erase cycle of MLC block*/

const   UINT16      nECC1LvRdDistOnSLC; /**< ECC 3-4 bit error in SLC         */
const   UINT16      nECC2LvRdDistOnSLC; /**< ECC 2 bit error in SLC           */
const   UINT16      nECC1LvRdDistOnMLC; /**< ECC 3-4 bit error in MLC         */
const   UINT16      nECC2LvRdDistOnMLC; /**< ECC 2 bit error in MLC           */

} FlexONDSpec;

/** @brief   Shared data structure for communication in Dual Core             */
typedef struct
{
    UINT32  nShMemUseCnt;

    /**< Previous operation data structure which can be shared among process  */
    UINT16  nPreOp[FSR_MAX_DIES];
    UINT16  nPreOpPbn[FSR_MAX_DIES];
    UINT16  nPreOpPgOffset[FSR_MAX_DIES];
    UINT32  nPreOpFlag[FSR_MAX_DIES];

#if defined (FSR_LLD_READ_PRELOADED_DATA)
    UINT32  bIsPreCmdLoad[FSR_MAX_DIES];
#endif

} FlexONDShMem;

/** Data structure of Flex-OneNAND LLD context for each device number         */
typedef struct
{
    UINT32       nBaseAddr;             /**< The base address of Flex-OneNAND */
    BOOL32       bOpen;                 /**< Open flag : TRUE32 or FALSE32    */

    FlexONDSpec *pstFNDSpec;            /**< pointer to FlexONDSpec           */

    UINT16       nFBAMask;              /**< The mask of Flash Block Address  */
    UINT8        nFPASelSft;            /**< The shift value of FPA selection */
    UINT8        nDDPSelSft;            /**< The shift value of DDP selection */

    UINT16       nSysConf1;             /**< When opening a device save value
                                           of system configuration 1 register.
                                           Restore this value after hot reset */
    UINT16       nFlushOpCaller;

    BOOL32       bCachePgm;             /**< Supports cache program           */

    BOOL32       bIsPreCmdCache[FSR_MAX_DIES];

    UINT16       nBlksForSLCArea[FSR_MAX_DIES];/**< # of blocks for SLC area  */

    UINT8       *pSpareBuffer;          /**< Can cover all spare area of 1 pg */

    UINT8       *pTempBuffer;           /**< Buffer for temporary use. 
                                             is used for just allocation/free */

    UINT32       nWrTranferTime;        /**< Write transfer time              */
    UINT32       nRdTranferTime;        /**< Read transfer time               */

    UINT32       nIntID;                /**< Interrupt ID : non-blocking I/O  */
    UINT8        nUID[FSR_LLD_UID_SIZE];/**< Unique ID info. about OTP block  */

#if defined (FSR_LLD_STATISTICS)
    UINT32       nNumOfSLCLoads; /**< The number of times of Load  operations */
    UINT32       nNumOfMLCLoads; /**< The number of times of Load  operations */

    UINT32       nNumOfSLCPgms;  /**< The number of times of SLC programs     */
    UINT32       nNumOfLSBPgms;  /**< The number of times of LSB programs     */
    UINT32       nNumOfMSBPgms;  /**< The number of times of MSB programs     */

    UINT32       nNumOfCacheBusy;/**< The number of times of Cache Busy       */
    UINT32       nNumOfErases;   /**< The number of times of Erase operations */

    UINT32       nNumOfRdTrans;  /**< The number of times of Read  Transfer   */
    UINT32       nNumOfWrTrans;  /**< The number of times of Write Transfer   */

    UINT32       nPreCmdOption[FSR_MAX_DIES]; /** Previous command option     */

    INT32        nIntLowTime[FSR_MAX_DIES];
                                 /**< MDP : 0 
                                      DDP : Previous operation time           */

    UINT32       nRdTransInBytes;/**< The number of bytes transfered in read  */
    UINT32       nWrTransInBytes;/**< The number of bytes transfered in write */
#endif /* #if defined (FSR_LLD_STATISTICS)                                    */

#if defined (FSR_LLD_RUNTIME_ERASE_REFRESH)
    UINT32      *pReadCnt;
#endif

} FlexONDCxt;

/******************************************************************************/
/* Global variable definitions                                                */
/******************************************************************************/

/* 
 * gnPgmCmdArray contains the actual COMMAND for the program
 * The size of array can be changed for the future extention
 * If device does not support Cache Program, LLD_Open() fills it with
 * normal program command
 */
PRIVATE       UINT16    gnPgmCmdArray[FSR_FND_MAX_PGMCMD] =
                            { FSR_FND_CMD_PROGRAM, /* FSR_LLD_FLAG_1X_PROGRAM */
                              FSR_FND_CMD_CACHEPGM,/* FSR_LLD_FLAG_1X_CACHEPGM*/
                              0xFFFF,              /* FSR_LLD_FLAG_2X_PROGRAM */
                              0xFFFF,              /* FSR_LLD_FLAG_2X_CACHEPGM*/
                              FSR_FND_CMD_PROGRAM, /* FSR_LLD_FLAG_OTP_PROGRAM*/
                            };

/* 
 * gnLoadCmdArray contains the actual COMMAND for the load
 * The size of array can be changed for the future extention
 * gnLoadCmdArray is not a const type, so value can change
 */
PRIVATE       UINT16    gnLoadCmdArray[FSR_FND_MAX_LOADCMD] =
                            { 0xFFFF,           /* FSR_LLD_FLAG_NO_LOADCMD    */
                              FSR_FND_CMD_LOAD, /* FSR_LLD_FLAG_1X_LOAD       */
                              FSR_FND_CMD_SUPERLOAD, /* FSR_LLD_FLAG_1X_PLOAD */
                              0xFFFF,           /* FSR_LLD_FLAG_2X_LOAD       */
                              0xFFFF,           /* FSR_LLD_FLAG_2X_PLOAD      */
                              0x0005,       /* FSR_LLD_FLAG_LSB_RECOVERY_LOAD */
                            };

/* 
 * gnEraseCmdArray contains the actual COMMAND for the erase
 * The size of array can be changed for the future extention
 */
PRIVATE const UINT16    gnEraseCmdArray[FSR_FND_MAX_ERASECMD] =
                            { FSR_FND_CMD_ERASE,/* FSR_LLD_FLAG_1X_ERASE      */
                                                /* FSR_LLD_FLAG_2X_ERASE      */
                            };

/* 
 * gnBadMarkValue contains the actual 2 byte value 
 * which is programmed into the first 2 byte of spare area
 * the size of array can be changed for the future extention
 */
PRIVATE const UINT16    gnBadMarkValue[FSR_FND_MAX_BADMARK]   =
                            { 0xFFFF,         /* FSR_LLD_FLAG_WR_NOBADMARK    */
                              0x2222,         /* FSR_LLD_FLAG_WR_EBADMARK     */
                              0x4444,         /* FSR_LLD_FLAG_WR_WBADMARK     */
                              0x8888,         /* FSR_LLD_FLAG_WR_LBADMARK     */
                            };

/* 
 * gnCpBkCmdArray contains the actual COMMAND for the copyback
 * the size of array can be changed for the future extention
 */
PRIVATE const UINT16    gnCpBkCmdArray[FSR_FND_MAX_CPBKCMD]   =
                            { 0xFFFF,
                              FSR_FND_CMD_LOAD, /* FSR_LLD_FLAG_1X_CPBK_LOAD  */
                              FSR_FND_CMD_PROGRAM,/*FSR_LLD_FLAG_1X_CPBK_PROGR*/
                                              /* FSR_LLD_FLAG_2X_CPBK_LOAD    */
                                              /* FSR_LLD_FLAG_2X_CPBK_PROGRAM */
                                              /* FSR_LLD_FLAG_4X_CPBK_LOAD    */
                                              /* FSR_LLD_FLAG_4X_CPBK_PROGRAM */
                            };

/* 
 * gnBBMMetaValue contains the Meta mark
 */
PRIVATE const UINT16    gnBBMMetaValue[FSR_FND_MAX_BBMMETA]   =
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

PRIVATE FlexONDCxt     *gpstFNDCxt[FSR_FND_MAX_DEVS];

#if !defined(WITH_TINY_FSR)
FlexONDShMem *gpstFNDShMem[FSR_FND_MAX_DEVS];
#endif

#if defined (WITH_TINY_FSR)
extern FlexONDShMem *gpstFNDShMem[FSR_FND_MAX_DEVS];
#endif

#if defined (FSR_LLD_STATISTICS)
PRIVATE UINT32          gnElapsedTime;
PRIVATE UINT32          gnDevsInVol[FSR_MAX_VOLS] = {0, 0};
#endif /* #if defined (FSR_LLD_STATISTICS)                                    */

#if defined(FSR_LLD_LOGGING_HISTORY)
volatile FlexONDOpLog gstFNDOpLog[FSR_MAX_DEVS] =
{
    {0, {0,}, {0,}, {0,},{0,}},
    {0, {0,}, {0,}, {0,},{0,}},
    {0, {0,}, {0,}, {0,},{0,}},
    {0, {0,}, {0,}, {0,},{0,}}
};
#endif

/* 
 * FSR_FND_Open() maps this function pointer to _ReadOptWithSLoad() or
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

/******************************************************************************/
/* Local constant definitions                                                 */
/******************************************************************************/
/* 
 * In case of MLC partition, when Program, Cache Program, Interleave Cache
 * Program, Copyback with random datain operations are abnormally aborted,
 * not only page data under program but also paired page may be demaged
 */
PRIVATE const UINT8 gnPairPgMap[] = 
{
    0x04, 0x05, 0x08, 0x09, 0x00, 0x01, 0x0C, 0x0D,
    0x02, 0x03, 0x10, 0x11, 0x06, 0x07, 0x14, 0x15,
    0x0A, 0x0B, 0x18, 0x19, 0x0E, 0x0F, 0x1C, 0x1D,
    0x12, 0x13, 0x20, 0x21, 0x16, 0x17, 0x24, 0x25,
    0x1A, 0x1B, 0x28, 0x29, 0x1E, 0x1F, 0x2C, 0x2D,
    0x22, 0x23, 0x30, 0x31, 0x26, 0x27, 0x34, 0x35,
    0x2A, 0x2B, 0x38, 0x39, 0x2E, 0x2F, 0x3C, 0x3D,
    0x32, 0x33, 0x40, 0x41, 0x36, 0x37, 0x44, 0x45,
    0x3A, 0x3B, 0x48, 0x49, 0x3E, 0x3F, 0x4C, 0x4D,
    0x42, 0x43, 0x50, 0x51, 0x46, 0x47, 0x54, 0x55,
    0x4A, 0x4B, 0x58, 0x59, 0x4E, 0x4F, 0x5C, 0x5D,
    0x52, 0x53, 0x60, 0x61, 0x56, 0x57, 0x64, 0x65,
    0x5A, 0x5B, 0x68, 0x69, 0x5E, 0x5F, 0x6C, 0x6D,
    0x62, 0x63, 0x70, 0x71, 0x66, 0x67, 0x74, 0x75,
    0x6A, 0x6B, 0x78, 0x79, 0x6E, 0x6F, 0x7C, 0x7D,
    0x72, 0x73, 0x7E, 0x7F, 0x76, 0x77, 0x7A, 0x7B
};

PRIVATE const UINT8 gnLSBPgs[] =
{
    0x00, 0x01, 0x02, 0x03, 0x06, 0x07, 0x0A, 0x0B,
    0x0E, 0x0F, 0x12, 0x13, 0x16, 0x17, 0x1A, 0x1B,
    0x1E, 0x1F, 0x22, 0x23, 0x26, 0x27, 0x2A, 0x2B,
    0x2E, 0x2F, 0x32, 0x33, 0x36, 0x37, 0x3A, 0x3B,
    0x3E, 0x3F, 0x42, 0x43, 0x46, 0x47, 0x4A, 0x4B,
    0x4E, 0x4F, 0x52, 0x53, 0x56, 0x57, 0x5A, 0x5B,
    0x5E, 0x5F, 0x62, 0x63, 0x66, 0x67, 0x6A, 0x6B,
    0x6E, 0x6F, 0x72, 0x73, 0x76, 0x77, 0x7A, 0x7B
};

PRIVATE const FlexONDSpec gstFNDSpec[] = {
/*************************************************************************************************************************************************************************/
/* 1. nMID                                                                                                                                                               */
/* 2.         nDID                                                                                                                                                       */
/* 3.                 nGEN                                                                                                                                               */
/* 4.                    nNumOfBlks                                                                                                                                      */
/* 5 .                         nNumOfDies                                                                                                                                */
/* 6.                             nNumOfPlanes                                                                                                                           */
/* 7.                                nSctsPerPG                                                                                                                          */
/* 8                                    nSparePerSct                                                                                                                     */
/* 9.                                       nPgsPerBlkForSLC                                                                                                             */
/* 10                                           nPgsPerBlkForMLC                                                                                                         */
/* 11.                                               b1stBlkOTP                                                                                                          */
/* 12.                                                       nUserOTPScts                                                                                                */
/* 13.                                                           nRsvBlksInDev                                                                                           */
/* 14.                                                               pPairedPgMap                                                                                        */
/* 15                                                                             pnLSBPgs                                                                               */
/* 16                                                                                       nSLCTLoadTime                                                                */
/* 17.                                                                                          nMLCTLoadTime                                                            */
/* 18.                                                                                              nSLCTProgTime                                                        */
/* 19.                                                                                                    nMLCTProgTime[0]                                               */
/* 20.                                                                                                         nMLCTProgTime[1]                                          */
/* 21.                                                                                                                nTEraseTime                                        */
/* 22                                                                                                                      nSLCPECycle                                   */
/* 23                                                                                                                             nMLCPECycle                            */
/* 24                                                                                                                                    nSizeOfRdDistTbl                */
/*************************************************************************************************************************************************************************/

    /*
     * Guideline1: Mux/Demux & 1.8V/3.3V is not registered as a separate device 
     * Guideline2: MDP/DDP would be better to be registered at once 
     * Guideline3: Be careful of missing comma 
     */

    /* 4Gb */
    { 0x00EC, 0x0250, 0, 1024, 1, 1, 8, 16, 64, 128, TRUE32, 50,  26, gnPairPgMap, gnLSBPgs, 45, 50, 240, {240, 1760}, 500, 50000, 10000, 0x0202, 0x0C0C, 0x0202, 0x0C0C},

    /* 8Gb DDP */
    { 0x00EC, 0x0268, 0, 2048, 2, 1, 8, 16, 64, 128, TRUE32, 50,  52, gnPairPgMap, gnLSBPgs, 45, 50, 240, {240, 1760}, 500, 50000, 10000, 0x0202, 0x0C0C, 0x0202, 0x0C0C},

    /* 8Gb */
    { 0x00EC, 0x0260, 0, 2048, 1, 1, 8, 16, 64, 128, TRUE32, 50,  52, gnPairPgMap, gnLSBPgs, 56, 67, 335, {335, 2465}, 500, 50000,  5000, 0x0202, 0x0C0C, 0x0202, 0x0C0C},

    /* 16Gb DDP */
    { 0x00EC, 0x0278, 0, 4096, 2, 1, 8, 16, 64, 128, TRUE32, 50, 104, gnPairPgMap, gnLSBPgs, 56, 67, 335, {335, 2465}, 500, 50000,  5000, 0x0202, 0x0C0C, 0x0202, 0x0C0C},
    
    {      0,      0, 0,    0, 0, 0, 0,  0, 0,  0,  FALSE32,  0,   0,        NULL,        0,  0,  0,   0, {  0,    0},   0,     0,     0,      0,      0,      0,      0},
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

PRIVATE VOID  _ReadSpare           (FSRSpareBuf *pstDest,
                           volatile UINT8       *pSrc);

PRIVATE VOID  _WriteMain(  volatile UINT8       *pDest,
                                    UINT8       *pSrc,
                                    UINT32       nSize);

PRIVATE VOID  _WriteSpare( volatile UINT8       *pDest,
                                    FSRSpareBuf *pstSrc,
                                    UINT32       nFlag);

PRIVATE INT32 _ReadPI              (UINT32       nDev,
                                    UINT32       nDieIdx,
                                    UINT32      *pnEndOfSLC,
                                    BOOL32      *pbPILocked);

PRIVATE INT32 _WritePI             (UINT32       nDev,
                                    UINT32       nDieIdx,
                                    UINT32       nPIValue);

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

PRIVATE VOID  _DumpRegisters       (volatile FlexOneNANDReg *pstReg);
PRIVATE VOID  _DumpSpareBuffer     (volatile FlexOneNANDReg *pstReg);
PRIVATE VOID  _DumpCmdLog          (VOID);

PRIVATE VOID  _CalcTransferTime    (UINT32       nDev,
                                    UINT16       nSysConf1Reg,
                                    UINT32      *pnReadCycle,
                                    UINT32      *pnWriteCycle);

PRIVATE INT32 _GetUniqueID         (UINT32       nDev,
                                    FlexONDCxt  *pstFNDCxt,
                                    UINT32       nFlag);

#if defined (FSR_LLD_STATISTICS)
PRIVATE VOID  _AddFNDStatLoad      (UINT32       nDev,
                                    UINT32       nDie,
                                    UINT32       nPbn,
                                    UINT32       nCmdOption);

PRIVATE VOID  _AddFNDStatPgm       (UINT32       nDev,
                                    UINT32       nDie,
                                    UINT32       nPbn,
                                    UINT32       nPg,
                                    UINT32       nCmdOption);

PRIVATE VOID  _AddFNDStat          (UINT32       nDev,
                                    UINT32       nDie,
                                    UINT32       nType,
                                    UINT32       nBytes,
                                    UINT32       nCmdOption);

#endif /* #if defined (FSR_LLD_STATISTICS)                                    */

#if defined (FSR_LLD_LOGGING_HISTORY)
PRIVATE VOID  _AddLog              (UINT32       nDev,
                                    UINT32       nDie);
#endif /* #if defined (FSR_LLD_LOGGING_HISTORY)                               */

/******************************************************************************/
/* Code Implementation                                                        */
/******************************************************************************/

/**
 * @brief           This function initializes Flex-OneNAND Device Driver.
 *
 * @param[in]       nFlag   : FSR_LLD_FLAG_NONE
 *
 * @return          FSR_LLD_SUCCESS
 * @return          FSR_LLD_ALREADY_INITIALIZED
 *
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark          It initialize internal data structure
 *
 */
PUBLIC INT32
FSR_FND_Init(UINT32 nFlag)
{
            FsrVolParm       astPAParm[FSR_MAX_VOLS];
            FSRLowFuncTbl    astLFT[FSR_MAX_VOLS];
            FSRLowFuncTbl   *apstLFT[FSR_MAX_VOLS];
            FlexONDShMem    *pstFNDShMem;

    PRIVATE BOOL32           nInitFlg                   = FALSE32;
            UINT32           nVol;
            UINT32           nPDev;
            UINT32           nIdx;
            UINT32           nDie;
            UINT32           nMemAllocType;
            UINT32           nMemoryChunkID;
            UINT32           nSharedMemoryUseCnt;

            INT32            nLLDRe                     = FSR_LLD_SUCCESS;
            INT32            nPAMRe                     = FSR_PAM_SUCCESS;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[FND:IN ] ++%s(nFlag:0x%08x)\r\n"),
        __FSR_FUNC__, nFlag));

    do
    {
        if (nInitFlg == TRUE32)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
                (TEXT("[FND:INF]   %s(nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
                (TEXT("            LLD is already initialized\r\n")));

            nLLDRe = FSR_LLD_ALREADY_INITIALIZED;
            break;
        }

        /* Initialize the pointers of context data structure */
        for (nPDev = 0; nPDev < FSR_FND_MAX_DEVS; nPDev++)
        {
            gpstFNDCxt[nPDev] = NULL;
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
            /* 
             * Because BML calls LLD_Init() by volume,
             * FSR_FND_Init() only initializes shared memory of 
             * corresponding volume
             */
            if ((apstLFT[nVol] == NULL) || apstLFT[nVol]->LLD_Init != FSR_FND_Init)
            {
                continue;
            }

            /* Check if FSR is on separate processors (OS) or dual drivers */
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

                pstFNDShMem = NULL;

#if !defined(WITH_TINY_FSR)
                /* Initialize the pointer of shared memory */
                gpstFNDShMem[nPDev] = (FlexONDShMem *) NULL;
#endif

                if(gpstFNDShMem[nPDev] == NULL)
                {
                    pstFNDShMem = (FlexONDShMem *) FSR_OAM_MallocExt(nMemoryChunkID,
                                                                     sizeof(FlexONDShMem),
                                                                     nMemAllocType);

                    if (pstFNDShMem == NULL)
                    {
                        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                            (TEXT("[FND:ERR]   %s(nFlag:0x%08x) / %d line\r\n"),
                            __FSR_FUNC__, nFlag, __LINE__));

                        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                            (TEXT("            pstFNDShMem is NULL\r\n")));

                        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                            (TEXT("            Malloc fails!\r\n")));

                        nLLDRe = FSR_LLD_MALLOC_FAIL;
                        break;
                    }

                    if (((UINT32) pstFNDShMem & (0x03)) != 0)
                    {
                        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                            (TEXT("[FND:ERR]   %s(nFlag:0x%08x) / %d line\r\n"),
                            __FSR_FUNC__, nFlag, __LINE__));

                        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                            (TEXT("            pstFNDShMem is misaligned:0x%08x\r\n"),
                            pstFNDShMem));

                        nLLDRe = FSR_OAM_NOT_ALIGNED_MEMPTR;
                        break;
                    }

                    gpstFNDShMem[nPDev] = pstFNDShMem;

                    /* PreOp init of single process */
                    if (astPAParm[nVol].bProcessorSynchronization == FALSE32)
                    {
                        /* Initialize shared memory used by LLD */
                        for (nDie = 0; nDie < FSR_MAX_DIES; nDie++)
                        {
                            pstFNDShMem->nPreOp[nDie]          = FSR_FND_PREOP_NONE;
                            pstFNDShMem->nPreOpPbn[nDie]       = FSR_FND_PREOP_ADDRESS_NONE;
                            pstFNDShMem->nPreOpPgOffset[nDie]  = FSR_FND_PREOP_ADDRESS_NONE;
                            pstFNDShMem->nPreOpFlag[nDie]      = FSR_FND_PREOP_FLAG_NONE;

#if defined (FSR_LLD_READ_PRELOADED_DATA)
                            pstFNDShMem->bIsPreCmdLoad[nDie]   = FALSE32;
#endif

                        }
                    }
                    /* PreOp init of dual process */
                    else
                    {
                        /* Initialize shared memory */
                        nSharedMemoryUseCnt = pstFNDShMem->nShMemUseCnt;

                        if ((nSharedMemoryUseCnt == 0) ||
                            (nSharedMemoryUseCnt == astPAParm[nVol].nSharedMemoryInitCycle))
                        {
                            pstFNDShMem->nShMemUseCnt = 0;

                            /* Initialize shared memory used by LLD */
                            for (nDie = 0; nDie < FSR_MAX_DIES; nDie++)
                            {
                                pstFNDShMem->nPreOp[nDie]          = FSR_FND_PREOP_NONE;
                                pstFNDShMem->nPreOpPbn[nDie]       = FSR_FND_PREOP_ADDRESS_NONE;
                                pstFNDShMem->nPreOpPgOffset[nDie]  = FSR_FND_PREOP_ADDRESS_NONE;
                                pstFNDShMem->nPreOpFlag[nDie]      = FSR_FND_PREOP_FLAG_NONE;

#if defined (FSR_LLD_READ_PRELOADED_DATA)
                                pstFNDShMem->bIsPreCmdLoad[nDie]   = FALSE32;
#endif

                            }
                        }
                    }

                    pstFNDShMem->nShMemUseCnt++;
                } /* for (nIdx = 0; nIdx < astPAParm[nVol].nDevsInVol; nIdx++) */
            }

            if (nLLDRe != FSR_LLD_SUCCESS)
            {
                break;
            }
        } /* for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++) */

        if (nLLDRe != FSR_LLD_SUCCESS)
        {
            break;
        }
        
        gpfReadOptimal = NULL;

        nInitFlg = TRUE32;

    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[FND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return (nLLDRe);
}



/**
 * @brief           This function opens Flex-OneNAND device driver 
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
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark
 *
 */
PUBLIC INT32
FSR_FND_Open(UINT32  nDev,
             VOID   *pParam,
             UINT32  nFlag)
{
             FlexONDCxt        *pstFNDCxt;
             FlexONDSpec       *pstFNDSpec;
    volatile FlexOneNANDReg    *pstFOReg;

             FsrVolParm        *pstParm;

             UINT32             nCnt;
             UINT32             nIdx;
             UINT32             nEndBlkNumOfSLC     = 0;
             UINT32             nRdTransferTime     = 0;
             UINT32             nWrTransferTime     = 0;
             UINT32             nBytesPerPage;
             UINT32             nSpareBytesPerPage;
             UINT32             nMemoryChunkID;
             INT32              nLLDRe              = FSR_LLD_SUCCESS;
             UINT16             nMID;               /* Manufacture ID */
             UINT16             nDID;               /* Device ID      */
             UINT16             nVID;               /* Version ID     */
             UINT16             nShifted;
             UINT32             nDie                = 0;
             UINT32             nTINYFlag           = FSR_LLD_FLAG_NONE;    /* LLD flag for TINY FSR    */

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[FND:IN ] ++%s(nDev:%d,nFlag:0x%08x)\r\n"), 
        __FSR_FUNC__, nDev, nFlag));

    do
    {

#if defined (FSR_LLD_STRICT_CHK)
        /* Check device number */
        if (nDev >= FSR_FND_MAX_DEVS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[FND:ERR]   %s(nDev:%d, nFlag:0x%08x) / %d line\r\n"),
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
                (TEXT("[FND:ERR]   %s(nDev:%d, pParam:0x%08x, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, pParam, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            pParam is NULL\r\n")));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }

        pstParm     = (FsrVolParm *) pParam;
        pstFNDCxt = gpstFNDCxt[nDev];

        if (pstFNDCxt == NULL)
        {
            nMemoryChunkID = nDev / (FSR_MAX_DEVS / FSR_MAX_VOLS);

            pstFNDCxt = (FlexONDCxt *)
                FSR_OAM_MallocExt(nMemoryChunkID, sizeof(FlexONDCxt), FSR_OAM_LOCAL_MEM);

            if (pstFNDCxt == NULL)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[FND:ERR]   %s(nDev:%d, pParam:0x%08x, nFlag:0x%08x) / %d line\r\n"),
                    __FSR_FUNC__, nDev, pParam, nFlag, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            pstFNDCxt is NULL\r\n")));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            malloc failed!\r\n")));

                nLLDRe = FSR_LLD_MALLOC_FAIL;
                break;
            }

            if (((UINT32) pstFNDCxt & (0x03)) != 0)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[FND:ERR]   %s(nDev:%d, nFlag:0x%08x) / %d line\r\n"),
                    __FSR_FUNC__, nDev, nFlag, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            pstFNDCxt is misaligned:0x%08x\r\n"),
                    pstFNDCxt));

                nLLDRe = FSR_OAM_NOT_ALIGNED_MEMPTR;
                break;
            }

            gpstFNDCxt[nDev] = pstFNDCxt;

            FSR_OAM_MEMSET(pstFNDCxt, 0x00, sizeof(FlexONDCxt));
        } /* if (pstFNDCxt == NULL) */

        if (pstFNDCxt->bOpen == TRUE32)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
                (TEXT("[FND:INF]   %s(nDev:%d, pParam:0x%08x, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, pParam, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
                (TEXT("[FND:INF]   dev:%d is already open\r\n"), nDev));

            nLLDRe = FSR_LLD_ALREADY_OPEN;
            break;
        }

        /* Set base address */
        nIdx = nDev & (FSR_MAX_DEVS / FSR_MAX_VOLS - 1);

        if (pstParm->nBaseAddr[nIdx] != FSR_PAM_NOT_MAPPED)
        {
            pstFNDCxt->nBaseAddr = pstParm->nBaseAddr[nIdx];

            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
                (TEXT("[FND:INF]   %s(nDev:%d, pParam:0x%08x, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, pParam, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
                (TEXT("            pstFNDCxt->nBaseAddr: 0x%08x\r\n"),
                pstFNDCxt->nBaseAddr));
        }
        else
        {
            pstFNDCxt->nBaseAddr = FSR_PAM_NOT_MAPPED;
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[FND:ERR]   %s(nDev:%d, pParam:0x%08x, nFlag:0x%08x / %d line\r\n"),
                __FSR_FUNC__, nDev, pParam, nFlag, __LINE__));
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            OneNAND is not mapped\r\n")));
            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }

        pstFOReg = (volatile FlexOneNANDReg *) pstFNDCxt->nBaseAddr;

        /* Display device inforamtion */
        nMID = FND_READ(pstFOReg->nMID);
        nDID = FND_READ(pstFOReg->nDID);
        nVID = FND_READ(pstFOReg->nVerID);

        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
            (TEXT("            nDev=%d, nMID=0x%04x, nDID=0x%04x, nVID=0x%04x\r\n"),
            nDev, nMID, nDID, nVID));

        /* 
         * Find corresponding FlexONDSpec from gstFNDSpec[]
         * by looking at bits 3~15. ignore bits 0~2
         */
        pstFNDCxt->pstFNDSpec = NULL;

        for (nCnt = 0; gstFNDSpec[nCnt].nMID != 0; nCnt++)
        {
            if (((nDID & FSR_FND_DID_MASK) == (gstFNDSpec[nCnt].nDID & FSR_FND_DID_MASK)) && 
                (nMID == gstFNDSpec[nCnt].nMID))
            {
                pstFNDCxt->pstFNDSpec = (FlexONDSpec *) &gstFNDSpec[nCnt];
                break;
            }
            /* else continue */
        }

        if (pstFNDCxt->pstFNDSpec == NULL)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[FND:ERR]   %s(nDev:%d, pParam:0x%08x, nFlag:0x%08x / %d line\r\n"),
                __FSR_FUNC__, nDev, pParam, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Unknown Device\r\n")));

            nLLDRe = FSR_LLD_OPEN_FAILURE;
            break;
        }

        pstFNDSpec = pstFNDCxt->pstFNDSpec;

        pstFNDCxt->nFBAMask   = pstFNDSpec->nNumOfBlks / pstFNDSpec->nNumOfDies -1;

        /* Offset of FPA in Start Address8 register */
        pstFNDCxt->nFPASelSft = 2;

        /* Calculate nDDPSelSft */
        pstFNDCxt->nDDPSelSft = 0;
        nShifted = pstFNDCxt->nFBAMask << 1;
        while((nShifted & FSR_FND_DFS_MASK) != FSR_FND_DFS_MASK)
        {
            pstFNDCxt->nDDPSelSft++;
            nShifted <<= 1;
        }

        /* 
         * HOT-RESET initializes the value of register file, which includes 
         * system configuration 1 register.
         * For device to function properly, save the value of system
         * configuration 1 register at open time & restore it after HOT-RESET
         */
        pstFNDCxt->nSysConf1    =  FND_READ(pstFOReg->nSysConf1);

        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
            (TEXT("[FND:INF]   pstFNDCxt->nSysConf1:0x%04x / %d line\r\n"),
            pstFNDCxt->nSysConf1, __LINE__));

        pstFNDCxt->nFlushOpCaller = FSR_FND_PREOP_NONE;

        /* Initialize previous operation information */
        for (nCnt = 0; nCnt < FSR_MAX_DIES; nCnt++)
        {
            pstFNDCxt->bIsPreCmdCache[nCnt]  = FALSE32;
        }

        /* Flush every die to read PI information */
        if (pstFNDSpec->nNumOfDies == FSR_MAX_DIES)
        {
               FSR_FND_FlushOp(nDev,
                          ((nDie + 0x1) & 0x01),
                          FSR_LLD_FLAG_NONE | nTINYFlag);
        }

        nLLDRe = FSR_FND_FlushOp(nDev, nDie, FSR_LLD_FLAG_NONE | nTINYFlag);

        /* Read PI Allocation Info */
        for (nCnt = 0; nCnt < pstFNDSpec->nNumOfDies; nCnt++)
        {
            /* 
             * Ignore return value of _ReadPI()
             * LLD_Open() succeed in the presence of the PI read error
             */
            _ReadPI(nDev, nCnt, &nEndBlkNumOfSLC, NULL);

            pstFNDCxt->nBlksForSLCArea[nCnt] = (UINT16) nEndBlkNumOfSLC + 1;

            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
                (TEXT("[FND:INF]   pstFNDCxt->nBlksForSLCArea[%d]:%d / %d line\r\n"),
                nCnt, pstFNDCxt->nBlksForSLCArea[nCnt], __LINE__));
        }

        nSpareBytesPerPage  = pstFNDSpec->nSctsPerPG * pstFNDSpec->nSparePerSct;
        nMemoryChunkID     = nDev / (FSR_MAX_DEVS / FSR_MAX_VOLS);
        pstFNDCxt->pSpareBuffer =
            (UINT8 *) FSR_OAM_MallocExt(nMemoryChunkID, nSpareBytesPerPage, FSR_OAM_LOCAL_MEM);

        if (pstFNDCxt->pSpareBuffer == NULL)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[FND:ERR]   %s(nDev:%d, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            malloc failed!\r\n")));

            nLLDRe = FSR_LLD_MALLOC_FAIL;
            break;
        }

        if (((UINT32) pstFNDCxt->pSpareBuffer & (0x03)) != 0)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[FND:ERR]   %s(nDev:%d, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            pstFNDCxt->pSpareBuffer is misaligned:0x%08x\r\n"),
                pstFNDCxt->pSpareBuffer));

            nLLDRe = FSR_OAM_NOT_ALIGNED_MEMPTR;
            break;
        }

        FSR_OAM_MEMSET(pstFNDCxt->pSpareBuffer, 0xFF, nSpareBytesPerPage);

        /* nBytesPerPage is 4224 byte */
        nBytesPerPage = pstFNDSpec->nSctsPerPG *
                        (FSR_FND_SECTOR_SIZE + pstFNDSpec->nSparePerSct);
        nMemoryChunkID = nDev / (FSR_MAX_DEVS / FSR_MAX_VOLS);

        pstFNDCxt->pTempBuffer =
            (UINT8 *) FSR_OAM_MallocExt(nMemoryChunkID, nBytesPerPage, FSR_OAM_LOCAL_MEM);

        if (pstFNDCxt->pTempBuffer == NULL)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                            (TEXT("[FND:ERR]   %s(nDev:%d, nFlag:0x%08x) / %d line\r\n"),
                            __FSR_FUNC__, nDev, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                            (TEXT("            malloc failed!\r\n")));

            nLLDRe = FSR_LLD_MALLOC_FAIL;
            break;
        }

        if (((UINT32) pstFNDCxt->pTempBuffer & (0x03)) != 0)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                            (TEXT("[FND:ERR]   %s(nDev:%d, nFlag:0x%08x) / %d line\r\n"),
                            __FSR_FUNC__, nDev, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                            (TEXT("            pstFNDCxt->pTempBuffer is misaligned:0x%08x\r\n"),
                            pstFNDCxt->pTempBuffer));

            nLLDRe = FSR_OAM_NOT_ALIGNED_MEMPTR;
            break;
        }

        FSR_OAM_MEMSET(pstFNDCxt->pTempBuffer, 0x00, nBytesPerPage);

        /* Calculate time for transfering 16 bits between host & DataRAM of OneNAND */
        _CalcTransferTime(nDev, pstFNDCxt->nSysConf1,
                          &nRdTransferTime, &nWrTransferTime);

        pstFNDCxt->nRdTranferTime  = nRdTransferTime; /* Nano second base */
        pstFNDCxt->nWrTranferTime  = nWrTransferTime; /* Nano second base */

        nIdx                    = nDev & (FSR_MAX_DEVS / FSR_MAX_VOLS -1);
        pstFNDCxt->nIntID       = pstParm->nIntID[nIdx];

        if ((pstFNDCxt->nIntID != FSR_INT_ID_NONE) && (pstFNDCxt->nIntID > FSR_INT_ID_NAND_7))
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[FND:INF]   nIntID is out of range(nIntID:%d)\r\n"), pstFNDCxt->nIntID));
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[FND:INF]   FSR_INT_ID_NAND_0(%d) <= nIntID <= FSR_INT_ID_NAND_7(%d)\r\n"), 
                FSR_INT_ID_NAND_0, FSR_INT_ID_NAND_7));
            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }

        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
            (TEXT("[FND:INF]   pstFNDCxt->nIntID   :0x%04x / %d line\r\n"),
            pstFNDCxt->nIntID, __LINE__));

#if defined (FSR_LLD_RUNTIME_ERASE_REFRESH)
        pstFNDCxt->pReadCnt = (UINT32 *) FSR_OAM_MallocExt(nMemoryChunkID,
                                                           pstFNDSpec->nNumOfBlks * sizeof(UINT32),
                                                           FSR_OAM_LOCAL_MEM);

        FSR_OAM_MEMSET(pstFNDCxt->pReadCnt, 0x00, pstFNDSpec->nNumOfBlks * sizeof(UINT32));
#endif

#if defined(FSR_LLD_USE_SUPER_LOAD)
        /* Configure superload */
        if (pstFNDCxt->nSysConf1 & FSR_FND_CONF1_SYNC_READ)
        {
            gpfReadOptimal            = _ReadOptWithSLoad;
        }
        else
        {
            gpfReadOptimal            = _ReadOptWithNonSLoad;

            /* 
             * Superload does not work in Ayncronous mode.
             * so, treat FSR_LLD_FLAG_1X_PLOAD as normal load in Async mode.
             */
            gnLoadCmdArray[FSR_LLD_FLAG_1X_PLOAD] = FSR_FND_CMD_LOAD;
        }
#else
        gpfReadOptimal                          = _ReadOptWithNonSLoad;
        gnLoadCmdArray[FSR_LLD_FLAG_1X_PLOAD]   = FSR_FND_CMD_LOAD;
#endif

#if defined(FSR_LLD_USE_CACHE_PGM)
        /* Device of 3.3V does not support cache program */
        if ((nDID & FSR_FND_DID_VCC_MASK) == FSR_FND_DID_VCC_33V)
        {
            pstFNDCxt->bCachePgm                    = FALSE32;
            gnPgmCmdArray[FSR_LLD_FLAG_1X_CACHEPGM] = FSR_FND_CMD_PROGRAM;
        }
        else
        {
            pstFNDCxt->bCachePgm                    = TRUE32;
        }
#else
        pstFNDCxt->bCachePgm                    = FALSE32;
        gnPgmCmdArray[FSR_LLD_FLAG_1X_CACHEPGM] = FSR_FND_CMD_PROGRAM;
#endif

#if defined (FSR_LLD_STATISTICS)
        gnDevsInVol[nDev / (FSR_MAX_DEVS / FSR_MAX_VOLS)]++;
#endif

        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_LOG,
            (TEXT("[FND:INF]   1X_CACHEPGM CMD : 0x%04x\r\n"), 
            gnPgmCmdArray[FSR_LLD_FLAG_1X_CACHEPGM]));

        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_LOG,
            (TEXT("[FND:INF]   1X_PLOAD    CMD : 0x%04x\r\n"), 
            gnLoadCmdArray[FSR_LLD_FLAG_1X_PLOAD]));

#if defined(FSR_LLD_ENABLE_DEBUG_PORT)
        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_LOG,
            (TEXT("[FND:INF]   debug port addr : byte order:0x%x / word order:0x%x\r\n"),
            (UINT32) (&pstFOReg->nDebugPort), (UINT32) (&pstFOReg->nDebugPort) / 2));

        FND_WRITE(pstFOReg->nDebugPort, 0x4321);
#endif

        pstFNDCxt->bOpen      = TRUE32;

    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[FND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return (nLLDRe);
}



/**
 * @brief           This function closes Flex-OneNAND device driver
 *
 * @param[in]       nDev        : Physical Device Number (0 ~ 3)
 * @param[in]       nFlag       :
 *
 * @return          FSR_LLD_SUCCESS
 * @return          FSR_LLD_INVALID_PARAM
 *
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark
 *
 */
PUBLIC INT32
FSR_FND_Close(UINT32 nDev,
              UINT32 nFlag)
{
    FlexONDCxt       *pstFNDCxt;

#if defined (FSR_LLD_STATISTICS)
    UINT32            nVol;
#endif

    UINT32            nMemoryChunkID;
    INT32             nLLDRe            = FSR_LLD_SUCCESS;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[FND:IN ] ++%s(nDev:%d, nFlag:0x%08x)\r\n"),
        __FSR_FUNC__, nDev, nFlag));

    /* Here LLD doesn't flush the previous operation, for BML flushes */
    do
    {

#if defined (FSR_LLD_STRICT_CHK)
        /* Check device number */
        if (nDev >= FSR_FND_MAX_DEVS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[FND:ERR]   %s(nDev:%d, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Invalid Device Number (nDev = %d)\r\n"),
                nDev));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */

        pstFNDCxt = gpstFNDCxt[nDev];

        if (pstFNDCxt != NULL)
        {
            pstFNDCxt->bOpen      = FALSE32;
            pstFNDCxt->pstFNDSpec = NULL;

            nMemoryChunkID = nDev / (FSR_MAX_DEVS / FSR_MAX_VOLS);

            FSR_OAM_FreeExt(nMemoryChunkID, pstFNDCxt->pTempBuffer, FSR_OAM_LOCAL_MEM);
            FSR_OAM_FreeExt(nMemoryChunkID, pstFNDCxt->pSpareBuffer, FSR_OAM_LOCAL_MEM);

#if defined (FSR_LLD_RUNTIME_ERASE_REFRESH)
            FSR_OAM_FreeExt(nMemoryChunkID, pstFNDCxt->pReadCnt, FSR_OAM_LOCAL_MEM);
#endif

            FSR_OAM_FreeExt(nMemoryChunkID, pstFNDCxt, FSR_OAM_LOCAL_MEM);
            gpstFNDCxt[nDev] = NULL;

#if defined (FSR_LLD_STATISTICS)
            nVol = nDev / (FSR_MAX_DEVS / FSR_MAX_VOLS);
            gnDevsInVol[nVol]--;
#endif

        }
    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[FND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return (nLLDRe);
}



/**
 * @brief           This function reads 1 page from Flex-OneNAND
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
 * @return          FSR_LLD_PREV_1LV_READ_DISTURBANCE | {FSR_LLD_1STPLN_CURR_ERROR}
 * @return          FSR_LLD_PREV_2LV_READ_DISTURBANCE | {FSR_LLD_1STPLN_CURR_ERROR}
 * @n               previous read disturbance error return value
 *
 * @author          NamOh Hwang
 * @version         1.2.0
 * @remark          One function call of FSR_FND_Read() loads 1 page from
 * @n               Flex-OneNAND and moves data from DataRAM to pMBuf, pSBuf.
 * @n               In DDP environment, 
 * @n               this version of read does not make use of load time.
 * @n               ( which is busy wait time, let's say it be 45us )
 * @n               instead, it waits till DataRAM gets filled.
 * @n               as a result, it does not give the best performance.
 * @n               To make use of die interleaving, use FSR_FND_ReadOptimal()
 *
 */
PUBLIC INT32
FSR_FND_Read(UINT32         nDev,
             UINT32         nPbn,
             UINT32         nPgOffset,
             UINT8         *pMBuf,
             FSRSpareBuf   *pSBuf,
             UINT32         nFlag)
{
    INT32       nLLDRe      = FSR_LLD_SUCCESS;
    UINT32      nLLDFlag;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[FND:IN ] ++%s()\r\n"), __FSR_FUNC__));

    do
    {
        nLLDFlag = ~FSR_LLD_FLAG_CMDIDX_MASK & nFlag;

        nLLDRe = FSR_FND_ReadOptimal(nDev,
                                     nPbn, nPgOffset,
                                     pMBuf, pSBuf,
                                     FSR_LLD_FLAG_1X_LOAD | nLLDFlag);
        if (FSR_RETURN_MAJOR(nLLDRe) != FSR_LLD_SUCCESS)
        {
            break;
        }

        nLLDRe = FSR_FND_ReadOptimal(nDev,
                                     nPbn, nPgOffset,
                                     pMBuf, pSBuf,
                                     FSR_LLD_FLAG_TRANSFER | nLLDFlag);

        if (FSR_RETURN_MAJOR(nLLDRe) != FSR_LLD_SUCCESS)
        {
            break;
        }
    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[FND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return nLLDRe;
}



/**
 * @brief           This function reads 1 page from Flex-OneNAND
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
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark          LLD_Open() links gpfReadOptimal to _ReadOptimalWithSLoad() or
 * @n               _ReadOptimalWithNonSLoad(), which depends on whether target
 * @n               supports syncronous read or not.
 * @n               because, superload needs data transferred in sync mode.
 * @n               _ReadOptimalWithSLoad() does not work in Async mode.
 *
 */
PUBLIC INT32
FSR_FND_ReadOptimal(UINT32         nDev,
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

    /* FSR_FND_Open() assigns appropriate function pointer to gpfReadOptimal.
     * it depends on the Read Mode (whether it supports syncronous read or not).
     */
    return nLLDRe;
}



/**
 * @brief           This function reads data from Flex-OneNAND
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
             FlexONDCxt       *pstFNDCxt        = gpstFNDCxt[nDev];
             FlexONDSpec      *pstFNDSpec       = pstFNDCxt->pstFNDSpec;
             FlexONDShMem     *pstFNDShMem      = gpstFNDShMem[nDev];
    volatile FlexOneNANDReg   *pstFOReg         = (volatile FlexOneNANDReg *) pstFNDCxt->nBaseAddr;
             UINT32            nCmdIdx;

             /* Start sector offset from the start */
             UINT32            nStartOffset;

             /* End sector offset from the end */
             UINT32            nEndOffset;
             UINT32            nDie;
             UINT32            nFlushOpCaller;

#if defined (FSR_LLD_STATISTICS)
             UINT32            nBytesTransferred = 0;
#endif /* #if defined (FSR_LLD_STATISTICS) */

             INT32             nLLDRe     = FSR_LLD_SUCCESS;
             UINT16            nSysConf1;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
    (TEXT("[FND:IN ] ++%s(nDev:%d, nPbn:%d, nPgOffset:%d, pMBuf:0x%08x, pSBuf:0x%08x, nFlag:0x%08x)\r\n"),
    __FSR_FUNC__, nDev, nPbn, nPgOffset, pMBuf, pSBuf, nFlag));

    do
    {
        nCmdIdx = (nFlag & FSR_LLD_FLAG_CMDIDX_MASK) >> FSR_LLD_FLAG_CMDIDX_BASEBIT;

        nDie    = nPbn >> (FSR_FND_DFS_BASEBIT - pstFNDCxt->nDDPSelSft);

        nFlushOpCaller = FSR_FND_PREOP_READ << FSR_FND_FLUSHOP_CALLER_BASEBIT;

        if ((nCmdIdx == FSR_LLD_FLAG_1X_LOAD) ||
            (nCmdIdx == FSR_LLD_FLAG_1X_PLOAD)||
            (nCmdIdx == FSR_LLD_FLAG_LSB_RECOVERY_LOAD))
        {

            /* Flex-OneNAND doesn't support read interleaving,
               therefore, wait until interrupt status is ready for both die.
               if device is DDP. */
            if (pstFNDSpec->nNumOfDies == FSR_MAX_DIES)
            {
                FSR_FND_FlushOp(nDev,
                                nFlushOpCaller | ((nDie + 0x1) & 0x01),
                                nFlag | FSR_LLD_FLAG_REMAIN_PREOP_STAT);
            }

            nLLDRe = FSR_FND_FlushOp(nDev, nFlushOpCaller | nDie, nFlag);

            /* Set DBS */
            FND_WRITE(pstFOReg->nStartAddr2, (UINT16) (nDie << FSR_FND_DBS_BASEBIT));

            /* Set DFS & FBA */
            FND_WRITE(pstFOReg->nStartAddr1,
                (UINT16) ((nDie << FSR_FND_DFS_BASEBIT) |
                          (nPbn & pstFNDCxt->nFBAMask)));

            /* Set FPA (Flash Page Address) */
            FND_WRITE(pstFOReg->nStartAddr8,
                (UINT16) (nPgOffset << pstFNDCxt->nFPASelSft));

            /* Set Start Buffer Register */
            FND_WRITE(pstFOReg->nStartBuf, FSR_FND_START_BUF_DEFAULT);


            if ((nFlag & FSR_LLD_FLAG_ECC_MASK) == FSR_LLD_FLAG_ECC_ON)
            {
                nSysConf1 = FND_READ(pstFOReg->nSysConf1);

                /* If ECC is disabled, enable */
                if (nSysConf1 & FSR_FND_CONF1_ECC_OFF)
                {
                    FND_CLR(pstFOReg->nSysConf1, FSR_FND_CONF1_ECC_ON);
                }
            }
            else
            {
                FND_SET(pstFOReg->nSysConf1, FSR_FND_CONF1_ECC_OFF);
            }

            /* In case of non-blocking mode, interrupt should be enabled */
            if (nFlag & FSR_LLD_FLAG_INT_MASK)
            {
                FSR_OAM_ClrNEnableInt(pstFNDCxt->nIntID);
            }
            else
            {
                FSR_OAM_ClrNDisableInt(pstFNDCxt->nIntID);
            }

#if defined (FSR_LLD_READ_PRELOADED_DATA)            
            if ((pstFNDShMem->bIsPreCmdLoad[nDie]   == TRUE32) &&
                (pstFNDShMem->nPreOpPgOffset[nDie]  == nPgOffset) &&
                (pstFNDShMem->nPreOpPbn[nDie]       == nPbn))
            {
                /* LLD does not issue load command */
            }
            else
#endif
            {
                /* Issue command */
                FND_WRITE(pstFOReg->nCmd, (UINT16) gnLoadCmdArray[nCmdIdx]);

#if defined (FSR_LLD_RUNTIME_ERASE_REFRESH)
                (pstFNDCxt->pReadCnt[nPbn])++;
#endif

#if defined (FSR_LLD_READ_PRELOADED_DATA)
                pstFNDShMem->bIsPreCmdLoad[nDie] = TRUE32;
#endif

#if defined (FSR_LLD_STATISTICS)
                if (gnLoadCmdArray[nCmdIdx] == FSR_FND_CMD_SUPERLOAD)
                {
                    _AddFNDStatLoad(nDev,
                                    nDie,
                                    nPbn,
                                    FSR_FND_STAT_PLOAD);
                }
                else
                {
                    _AddFNDStatLoad(nDev,
                                    nDie,
                                    nPbn,
                                    FSR_FND_STAT_NORMAL_CMD);
                }
#endif /* #if defined (FSR_LLD_STATISTICS) */

            }
        } /* if ((nCmdIdx == FSR_LLD_FLAG_1X_LOAD) | (nCmdIdx == FSR_LLD_FLAG_1X_PLOAD)) */

        /* Transfer data */ 
        if (nFlag & FSR_LLD_FLAG_TRANSFER)
        {
            if (nCmdIdx == FSR_LLD_FLAG_NO_LOADCMD)
            {
                /* 
                 * Flex-OneNAND doesn't support read interleaving,
                 * therefore, wait until interrupt status is ready for both die
                 * if device is DDP. 
                 */
                if (pstFNDSpec->nNumOfDies == FSR_MAX_DIES)
                {
                    FSR_FND_FlushOp(nDev,
                                    nFlushOpCaller | ((nDie + 0x1) & 0x1),
                                    nFlag | FSR_LLD_FLAG_REMAIN_PREOP_STAT);
                }
                nLLDRe = FSR_FND_FlushOp(nDev, nFlushOpCaller | nDie, nFlag);

                /* Set DBS */
                FND_WRITE(pstFOReg->nStartAddr2,
                    (UINT16) (nDie << FSR_FND_DBS_BASEBIT));
            }

            if (pMBuf != NULL)
            {
                /* _ReadMain() can load continuous sectors within a page. */
                nStartOffset = (nFlag & FSR_LLD_FLAG_1ST_SCTOFFSET_MASK)
                                >> FSR_LLD_FLAG_1ST_SCTOFFSET_BASEBIT;

                nEndOffset   = (nFlag & FSR_LLD_FLAG_LAST_SCTOFFSET_MASK)
                                >> FSR_LLD_FLAG_LAST_SCTOFFSET_BASEBIT;

                _ReadMain(pMBuf + nStartOffset * FSR_FND_SECTOR_SIZE,
                    &(pstFOReg->nDataMB00[0]) + nStartOffset * FSR_FND_SECTOR_SIZE,
                    (pstFNDSpec->nSctsPerPG - nStartOffset - nEndOffset) * FSR_FND_SECTOR_SIZE,
                    pstFNDCxt->pTempBuffer);

#if defined (FSR_LLD_STATISTICS)
                nBytesTransferred +=  FSR_FND_SECTOR_SIZE *
                    (pstFNDSpec->nSctsPerPG - nStartOffset - nEndOffset);
#endif /* #if defined (FSR_LLD_STATISTICS) */

            }

            if ((pSBuf != NULL) && (nFlag & FSR_LLD_FLAG_USE_SPAREBUF))
            {
                if ((nFlag & FSR_LLD_FLAG_DUMP_MASK) == FSR_LLD_FLAG_DUMP_OFF)
                {
                    _ReadSpare((FSRSpareBuf *) pSBuf, &(pstFOReg->nDataSB00[0]));
                }
                else
                {
                    /* 
                     * When dumpping NAND Image,
                     * _ReadMain() reads the whole spare area
                     */
                    _ReadMain((UINT8 *) pSBuf,
                        &(pstFOReg->nDataSB00[0]),
                        pstFNDSpec->nSctsPerPG * pstFNDSpec->nSparePerSct,
                        pstFNDCxt->pTempBuffer);
                }

#if defined (FSR_LLD_STATISTICS)
                if ((nFlag & FSR_LLD_FLAG_DUMP_MASK) == FSR_LLD_FLAG_DUMP_OFF)
                {
                    nBytesTransferred += (FSR_SPARE_BUF_SIZE_4KB_PAGE);
                }
                else
                {
                    nBytesTransferred +=
                        pstFNDSpec->nSctsPerPG * pstFNDSpec->nSparePerSct;
                }
#endif /* #if defined (FSR_LLD_STATISTICS) */

            }
            
            /*
             * *****************CAUTION ***************************************
             * *      COMPILER MUST NOT REMOVE THIS CODE                      *
             * ****************************************************************
             */
            if (nCmdIdx == FSR_LLD_FLAG_1X_PLOAD) 
            {
                /* 
                 * Read at least 4 bytes to the end of the spare area.
                 * Otherwise, superload will not work properly.
                 * 2 lines below trigger the load from page buffer to DataRAM
                 * [[ CAUTION ]]
                 * SHOULD NOT be issued at the address of &pstFOReg->nDataSB13[0x0e]
                 */

#if defined(FSR_ONENAND_EMULATOR)                
                *(UINT16 *) &(pstFNDCxt->pTempBuffer[0]) = FND_READ(*(UINT16 *) &pstFOReg->nDataSB13[0xC]);
                *(UINT16 *) &(pstFNDCxt->pTempBuffer[2]) = FND_READ(*(UINT16 *) &pstFOReg->nDataSB13[0xE]);
#else
                *(UINT32 *) &(pstFNDCxt->pTempBuffer[0]) = *(UINT32 *) &pstFOReg->nDataSB13[0x0C];
#endif
            }

#if defined (FSR_LLD_STATISTICS)
            if (nCmdIdx == FSR_LLD_FLAG_1X_PLOAD)
            {
                _AddFNDStat(nDev,
                            nDie,
                            FSR_FND_STAT_RD_TRANS,
                            nBytesTransferred,
                            FSR_FND_STAT_PLOAD);
            }
            else
            {
                _AddFNDStat(nDev,
                            nDie,
                            FSR_FND_STAT_RD_TRANS,
                            nBytesTransferred,
                            FSR_FND_STAT_NORMAL_CMD);
            }
#endif /* #if defined (FSR_LLD_STATISTICS) */

        } /* if (nFlag & FSR_LLD_FLAG_TRANSFER) */

    } while (0);

    /*
     * If transfer only operation, nPreOp[nDie] should be FSR_OND_PREOP_NONE.
     * ----------------------------------------------------------------------
     * FSR_FND_FlushOp() wait read interrupt if nPreOp is FSR_FND_PREOP_READ.
     * But if this function only has FSR_LLD_FLAG_TRANSFER, there is no read interrupt.
     * Therefore, In order to avoid infinite loop, nPreOp is FSR_FND_PREOP_NONE.
     * ----------------------------------------------------------------------
     */
    if ((nCmdIdx == FSR_LLD_FLAG_NO_LOADCMD) &&
        (nFlag & FSR_LLD_FLAG_TRANSFER))
    {
        pstFNDShMem->nPreOp[nDie]         = FSR_FND_PREOP_NONE;
    }
    else
    {
        pstFNDShMem->nPreOp[nDie]         = FSR_FND_PREOP_READ;

#if defined (FSR_LLD_READ_PRELOADED_DATA)        
        pstFNDShMem->nPreOpPbn[nDie]      = (UINT16) nPbn;
        pstFNDShMem->nPreOpPgOffset[nDie] = (UINT16) nPgOffset;
#endif

    }

#if !defined (FSR_LLD_READ_PRELOADED_DATA)
    pstFNDShMem->nPreOpPbn[nDie]      = (UINT16) nPbn;
    pstFNDShMem->nPreOpPgOffset[nDie] = (UINT16) nPgOffset;
#endif

    pstFNDShMem->nPreOpFlag[nDie]     = nFlag;

#if defined (FSR_LLD_LOGGING_HISTORY)
    _AddLog(nDev, nDie);
#endif

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[FND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

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
 * @author          NamOh Hwang
 * @version         1.0.0
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
             FlexONDCxt       *pstFNDCxt            = gpstFNDCxt[nDev];
             FlexONDSpec      *pstFNDSpec           = pstFNDCxt->pstFNDSpec;
             FlexONDShMem     *pstFNDShMem          = gpstFNDShMem[nDev];
    volatile FlexOneNANDReg   *pstFOReg             = (volatile FlexOneNANDReg *) pstFNDCxt->nBaseAddr;
             UINT32            nCmdIdx;

             /* start sector offset from the start */
             UINT32            nStartOffset;

             /* end sector offset from the end     */
             UINT32            nEndOffset;
             UINT32            nDie;
             UINT32            nFlushOpCaller;

#if defined (FSR_LLD_STATISTICS)
             UINT32            nBytesTransferred    = 0;
#endif /* #if defined (FSR_LLD_STATISTICS) */

             INT32             nLLDRe               = FSR_LLD_SUCCESS;
             UINT16            nSysConf1;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
    (TEXT("[FND:IN ] ++%s(nDev:%d, nPbn:%d, nPgOffset:%d, pMBuf:0x%08x, pSBuf:0x%08x, nFlag:0x%08x)\r\n"),
    __FSR_FUNC__ , nDev, nPbn, nPgOffset, pMBuf, pSBuf, nFlag));

    do
    {
        nCmdIdx = (nFlag & FSR_LLD_FLAG_CMDIDX_MASK) 
                    >> FSR_LLD_FLAG_CMDIDX_BASEBIT;

        nDie    = nPbn >> (FSR_FND_DFS_BASEBIT - pstFNDCxt->nDDPSelSft);

        nFlushOpCaller = FSR_FND_PREOP_READ << FSR_FND_FLUSHOP_CALLER_BASEBIT;

        /* Transfer data */
        if (nFlag & FSR_LLD_FLAG_TRANSFER)
        {
            /* 
             * OneNAND doesn't support read interleaving.
             * Therefore, wait until interrupt status is ready for both die if device is DDP. 
             */
            if (pstFNDSpec->nNumOfDies == FSR_MAX_DIES)
            {
                FSR_FND_FlushOp(nDev,
                                nFlushOpCaller | ((nDie + 0x1) & 0x1),
                                nFlag | FSR_LLD_FLAG_REMAIN_PREOP_STAT);
            }

            nLLDRe = FSR_FND_FlushOp(nDev, nFlushOpCaller | nDie, nFlag);

            /* Set DBS */
            FND_WRITE(pstFOReg->nStartAddr2, (UINT16) (nDie << FSR_FND_DBS_BASEBIT));

            if (pMBuf != NULL)
            {
                /* _ReadMain() can load continuous sectors within a page. */
                nStartOffset = (nFlag & FSR_LLD_FLAG_1ST_SCTOFFSET_MASK)
                                >> FSR_LLD_FLAG_1ST_SCTOFFSET_BASEBIT;

                nEndOffset   = (nFlag & FSR_LLD_FLAG_LAST_SCTOFFSET_MASK)
                                >> FSR_LLD_FLAG_LAST_SCTOFFSET_BASEBIT;

                _ReadMain(pMBuf + nStartOffset * FSR_FND_SECTOR_SIZE,
                &(pstFOReg->nDataMB00[0]) + nStartOffset * FSR_FND_SECTOR_SIZE,
                (pstFNDSpec->nSctsPerPG - nStartOffset - nEndOffset) * FSR_FND_SECTOR_SIZE,
                pstFNDCxt->pTempBuffer);

#if defined (FSR_LLD_STATISTICS)
                nBytesTransferred +=  FSR_FND_SECTOR_SIZE *
                    (pstFNDSpec->nSctsPerPG - nStartOffset - nEndOffset);
#endif /* #if defined (FSR_LLD_STATISTICS) */

            }

            if ((pSBuf != NULL) && (nFlag & FSR_LLD_FLAG_USE_SPAREBUF))
            {
                if ((nFlag & FSR_LLD_FLAG_DUMP_MASK) == FSR_LLD_FLAG_DUMP_OFF)
                {
                    _ReadSpare((FSRSpareBuf *) pSBuf, &(pstFOReg->nDataSB00[0]));
                }
                else
                {
                    /* 
                     * When dumpping NAND Image,
                     * _ReadMain() reads the whole spare area
                     */
                    _ReadMain((UINT8 *) pSBuf,
                        &(pstFOReg->nDataSB00[0]),
                        pstFNDSpec->nSctsPerPG * pstFNDSpec->nSparePerSct,
                        pstFNDCxt->pTempBuffer);
                }

#if defined (FSR_LLD_STATISTICS)
                if ((nFlag & FSR_LLD_FLAG_DUMP_MASK) == FSR_LLD_FLAG_DUMP_OFF)
                {
                    nBytesTransferred += (FSR_SPARE_BUF_SIZE_4KB_PAGE);
                }
                else
                {
                    nBytesTransferred +=
                        pstFNDSpec->nSctsPerPG * pstFNDSpec->nSparePerSct;
                }
#endif /* #if defined (FSR_LLD_STATISTICS) */

            }

#if defined (FSR_LLD_STATISTICS)
            _AddFNDStat(nDev,
                        nDie,
                        FSR_FND_STAT_RD_TRANS,
                        nBytesTransferred,
                        FSR_FND_STAT_NORMAL_CMD);

#endif /* #if defined (FSR_LLD_STATISTICS) */

            pstFNDShMem->nPreOp[nDie]         = FSR_FND_PREOP_NONE;

        } /* if (nFlag & FSR_LLD_FLAG_TRANSFER) */

        if ((nCmdIdx == FSR_LLD_FLAG_1X_LOAD) ||
            (nCmdIdx == FSR_LLD_FLAG_1X_PLOAD) ||
            (nCmdIdx == FSR_LLD_FLAG_LSB_RECOVERY_LOAD))
        {
            /* 
             * OneNAND doesn't support read interleaving.
             * Therefore, wait until interrupt status is ready for both die. if device is DDP. 
             */             
            if (pstFNDSpec->nNumOfDies == FSR_MAX_DIES)
            {
                FSR_FND_FlushOp(nDev,
                                nFlushOpCaller | ((nDie + 0x1) & 0x1),
                                nFlag | FSR_LLD_FLAG_REMAIN_PREOP_STAT);
            }

            FSR_FND_FlushOp(nDev, nFlushOpCaller | nDie, nFlag);

            /* Set DBS */
            FND_WRITE(pstFOReg->nStartAddr2,
                (UINT16) (nDie << FSR_FND_DBS_BASEBIT));

            /* Set DFS & FBA */
            FND_WRITE(pstFOReg->nStartAddr1,
                (UINT16) ((nDie << FSR_FND_DFS_BASEBIT) | (nPbn & pstFNDCxt->nFBAMask)));

            /* Set FPA (Flash Page Address) */
            FND_WRITE(pstFOReg->nStartAddr8,
                      (UINT16) (nPgOffset << pstFNDCxt->nFPASelSft));

            /* Set Start Buffer Register */
            FND_WRITE(pstFOReg->nStartBuf, FSR_FND_START_BUF_DEFAULT);

            if ((nFlag & FSR_LLD_FLAG_ECC_MASK) == FSR_LLD_FLAG_ECC_ON)
            {
                nSysConf1 = FND_READ(pstFOReg->nSysConf1);

                if (nSysConf1 & FSR_FND_CONF1_ECC_OFF)
                {
                    FND_CLR(pstFOReg->nSysConf1, FSR_FND_CONF1_ECC_ON);
                }
            }
            else
            {
                FND_SET(pstFOReg->nSysConf1, FSR_FND_CONF1_ECC_OFF);
            }

            /* In case of non-blocking mode, interrupt should be enabled */
            if (nFlag & FSR_LLD_FLAG_INT_MASK)
            {
                FSR_OAM_ClrNEnableInt(pstFNDCxt->nIntID);
            }
            else
            {
                FSR_OAM_ClrNDisableInt(pstFNDCxt->nIntID);
            }

#if defined (FSR_LLD_READ_PRELOADED_DATA)
            if ((pstFNDShMem->bIsPreCmdLoad[nDie]   == TRUE32) &&
                (pstFNDShMem->nPreOpPgOffset[nDie]  == nPgOffset) &&
                (pstFNDShMem->nPreOpPbn[nDie]       == nPbn))
            {
                /* LLD does not issue load command */
            }
            else
#endif
            {
                /* 
                 * Because SUPERLOAD is not working in Asynchronous Mode,
                 * In this version, issue a normal load command (0x0000) for PLOAD flag
                 */
                FND_WRITE(pstFOReg->nCmd, (UINT16) gnLoadCmdArray[nCmdIdx]);

                pstFNDShMem->nPreOpPbn[nDie]      = (UINT16) nPbn;
                pstFNDShMem->nPreOpPgOffset[nDie] = (UINT16) nPgOffset;

#if defined (FSR_LLD_READ_PRELOADED_DATA)
                pstFNDShMem->bIsPreCmdLoad[nDie] = TRUE32;
#endif

#if defined (FSR_LLD_RUNTIME_ERASE_REFRESH)
                (pstFNDCxt->pReadCnt[nPbn])++;
#endif

            }

#if defined (FSR_LLD_STATISTICS)
            _AddFNDStatLoad(nDev,
                            nDie,
                            nPbn,
                            FSR_FND_STAT_NORMAL_CMD);
#endif /* #if defined (FSR_LLD_STATISTICS) */

            pstFNDShMem->nPreOp[nDie]         = FSR_FND_PREOP_READ;

        } /* if ((nCmdIdx == FSR_LLD_FLAG_1X_LOAD) | (nCmdIdx == FSR_LLD_FLAG_1X_PLOAD)) */

        pstFNDShMem->nPreOpFlag[nDie]     = nFlag;

#if defined (FSR_LLD_LOGGING_HISTORY)
        _AddLog(nDev, nDie);
#endif

    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[FND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return (nLLDRe);
}



/**
 * @brief           This function reads data from main area of DataRAM
 *
 * @param[out]      pDest   : pointer to the buffer
 * @param[in]       pstSrc  : pointer to the main area of DataRAM
 * @param[in]       nSize   : # of bytes to read
 * @param[in]       pTempBuffer: temporary buffer for reading misaligned data
 *
 * @return          none
 *
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark          pTempBuffer is 4-byte aligned.
 * @n               The caller of _ReadMain() provides enough space for holding
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
        (TEXT("[FND:IN ] ++%s(pDest: 0x%08x, pSrc: 0x%08x, pTempBuffer: 0x%08x)\r\n"),
        __FSR_FUNC__, pDest, pSrc, pTempBuffer));

    /* If pDest is not 4 byte aligned. */
    if ((((UINT32) pDest & 0x3) != 0) || (nSize < FSR_FND_MIN_BULK_TRANS_SIZE))
    {
        TRANSFER_FROM_NAND(pTempBuffer, pSrc, nSize);
        FSR_OAM_MEMCPY(pDest, pTempBuffer, nSize);
    }
    else /* When pDest is 4 byte aligned */
    {
        TRANSFER_FROM_NAND(pDest, pSrc, nSize);
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[FND:OUT] --%s()\r\n"), __FSR_FUNC__));
}



/**
 * @brief           This function reads data from spare area of DataRAM
 *
 * @param[out]      pstDest : pointer to the host buffer
 * @param[in]       pSrc    : pointer to the spare area of DataRAM
 *
 * @return          none
 *
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark          This function reads from spare area of 1 page.
 *
 */
PRIVATE VOID
_ReadSpare(         FSRSpareBuf *pstDest,
           volatile UINT8       *pSrc)
{
             UINT32  nIdx;
             UINT32  nIdx1;
             UINT32  nSrcOffset;

    volatile UINT16 *pSrc16;
             UINT16 *pDest16;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[FND:IN ] ++%s(pstDest: 0x%08x, pSrc: 0x%08x)\r\n"),
        __FSR_FUNC__, pstDest, pSrc));

    pSrc16  = (volatile UINT16 *) pSrc;
    pDest16 = (UINT16 *) pstDest->pstSpareBufBase;

    FSR_ASSERT(((UINT32) pstDest & 0x01) == 0);

    nSrcOffset = 0;

    /* Read spare base */
    for (nIdx = 0; nIdx < (FSR_SPARE_BUF_BASE_SIZE / sizeof(UINT16)); nIdx++)
    {
        *pDest16++ = FND_READ(*pSrc16++);

        if (++nSrcOffset == 0x3)
        {
            nSrcOffset = 0;
            /* Skip 10 bytes for H/W ECC area */
            pSrc16 += FSR_FND_SPARE_HW_ECC_AREA;
        }
    }

    for (nIdx1 = 0; nIdx1 < pstDest->nNumOfMetaExt; nIdx1++)
    {
        pDest16 = (UINT16 *) &(pstDest->pstSTLMetaExt[nIdx1]);

        for (nIdx = 0; nIdx < (FSR_SPARE_BUF_EXT_SIZE / sizeof(UINT16)); nIdx++)
        {
            *pDest16++ = FND_READ(*pSrc16++);

            if (++nSrcOffset == 0x3)
            {
                nSrcOffset = 0;

                /* Skip 10 bytes for H/W ECC area */
                pSrc16 += FSR_FND_SPARE_HW_ECC_AREA;
            }
        }
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[FND:OUT] --%s()\r\n"), __FSR_FUNC__));
}



/**
 * @brief           This function writes data into Flex-OneNAND
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
 * @n               Error for normal program
 * @return          FSR_LLD_PREV_WRITE_ERROR  | {FSR_LLD_1STPLN_PREV_ERROR}
 * @n               Error for cache program
 * @return          FSR_LLD_WR_PROTECT_ERROR  | {FSR_LLD_1STPLN_CURR_ERROR}
 * @n               Write protetion error return value
 *
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark
 *
 */
PUBLIC INT32
FSR_FND_Write(UINT32       nDev,
              UINT32       nPbn,
              UINT32       nPgOffset,
              UINT8       *pMBuf,
              FSRSpareBuf *pSBuf,
              UINT32       nFlag)
{
             FlexONDCxt       *pstFNDCxt;
             FlexONDSpec      *pstFNDSpec;
             FlexONDShMem     *pstFNDShMem;
    volatile FlexOneNANDReg   *pstFOReg;
             UINT32            nCmdIdx;
             UINT32            nBBMMetaIdx;
             UINT32            nDie;
             UINT32            nFlushOpCaller;

#if defined (FSR_LLD_STATISTICS)
             UINT32            nBytesTransferred    = 0;
#endif /* #if defined (FSR_LLD_STATISTICS) */

             INT32             nLLDRe               = FSR_LLD_SUCCESS;
             UINT16            nWrProtectStat;             

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
    (TEXT("[FND:IN ] ++%s(nDev:%d, nPbn:%d, nPgOffset:%d, pMBuf: 0x%08x, pSBuf: 0x%08x, nFlag:0x%08x)\r\n"),
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

        pstFNDCxt   = gpstFNDCxt[nDev];
        pstFNDShMem = gpstFNDShMem[nDev];
        pstFNDSpec  = pstFNDCxt->pstFNDSpec;
        pstFOReg    = (volatile FlexOneNANDReg *) pstFNDCxt->nBaseAddr;

        nCmdIdx         = (nFlag & FSR_LLD_FLAG_CMDIDX_MASK) >> FSR_LLD_FLAG_CMDIDX_BASEBIT;

        /* Check if this request is OTP Program */
        if (nCmdIdx == FSR_LLD_FLAG_OTP_PROGRAM)
        {
            /* PBN = 0 */
            nPbn            = 0;
        }

        nDie            = (nPbn >> (FSR_FND_DFS_BASEBIT - pstFNDCxt->nDDPSelSft));
        nFlushOpCaller  = FSR_FND_PREOP_PROGRAM << FSR_FND_FLUSHOP_CALLER_BASEBIT;

        nLLDRe = FSR_FND_FlushOp(nDev, nFlushOpCaller | nDie, nFlag);
        if (nLLDRe != FSR_LLD_SUCCESS)
        {
            break;
        }

        /* Set DBS */
        FND_WRITE(pstFOReg->nStartAddr2,
            (UINT16) (nDie << FSR_FND_DBS_BASEBIT));

        /* Write Data Into DataRAM */
        if (pMBuf != NULL)
        {
            _WriteMain(&(pstFOReg->nDataMB00[0]),
                       pMBuf,
                       pstFNDSpec->nSctsPerPG * FSR_FND_SECTOR_SIZE);

#if defined (FSR_LLD_STATISTICS)
            nBytesTransferred += pstFNDSpec->nSctsPerPG * FSR_FND_SECTOR_SIZE;
#endif /* #if defined (FSR_LLD_STATISTICS) */
        }

        if (pMBuf == NULL && pSBuf != NULL)
        {
            if((nFlag & FSR_LLD_FLAG_BACKUP_DATA) != FSR_LLD_FLAG_BACKUP_DATA)
            {
                FSR_OAM_MEMSET(pstFNDCxt->pTempBuffer, 0xFF, FSR_SECTOR_SIZE * pstFNDSpec->nSctsPerPG);
            
                TRANSFER_TO_NAND(pstFOReg->nDataMB00,
                                 pstFNDCxt->pTempBuffer,
                                 FSR_SECTOR_SIZE * pstFNDSpec->nSctsPerPG);
            }
        }

        if (pSBuf != NULL)
        {
            if ((nFlag & FSR_LLD_FLAG_DUMP_MASK) == FSR_LLD_FLAG_DUMP_ON)
            {
                _WriteMain(&(pstFOReg->nDataSB00[0]),
                           (UINT8 *) pSBuf,
                           pstFNDSpec->nSparePerSct * pstFNDSpec->nSctsPerPG);
            }
            else
            {
                /* 
                 * If FSR_LLD_FLAG_BBM_META_BLOCK of nFlag is set,
                 * write nBMLMetaBase0 of FSRSpareBuf with 0xA5A5
                 */
                nBBMMetaIdx = (nFlag & FSR_LLD_FLAG_BBM_META_MASK) >>
                              FSR_LLD_FLAG_BBM_META_BASEBIT;

                pSBuf->pstSpareBufBase->nBMLMetaBase0 = gnBBMMetaValue[nBBMMetaIdx];

                /* 
                 * _WriteSpare does not care about bad mark which is written at
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
                    pstFNDSpec->nSctsPerPG * pstFNDSpec->nSparePerSct;
            }
#endif /* #if defined (FSR_LLD_STATISTICS) */

        }

        if (pMBuf != NULL && pSBuf == NULL)
        {
            if((nFlag & FSR_LLD_FLAG_BACKUP_DATA) != FSR_LLD_FLAG_BACKUP_DATA)
            {
                FSR_OAM_MEMSET(pstFNDCxt->pTempBuffer, 0xFF, FSR_SECTOR_SIZE * pstFNDSpec->nSctsPerPG);
                
                TRANSFER_TO_NAND(pstFOReg->nDataSB00,
                                 pstFNDCxt->pTempBuffer,
                                 FSR_SPARE_SIZE * pstFNDSpec->nSctsPerPG);
            }
        }

        if ((nFlag & FSR_LLD_FLAG_DUMP_MASK) == FSR_LLD_FLAG_DUMP_OFF)
        {
            /* 
             * Write bad mark of the block
             * bad mark is not written in _WriteSpare()
             */
            FND_WRITE(*(volatile UINT16 *) &pstFOReg->nDataSB00[0],
                gnBadMarkValue[(nFlag & FSR_LLD_FLAG_BADMARK_MASK) >> FSR_LLD_FLAG_BADMARK_BASEBIT]);
        }

#if defined (FSR_LLD_WAIT_ALLDIE_PGM_READY)
        /* 
         * In case of DDP, wait the other die ready.
         * When there is a cache program going on the other die, 
         * setting the address register leads to the problem.
         * That is, data under programming will be written to the newly set address.
         */
        if (pstFNDSpec->nNumOfDies == FSR_MAX_DIES)
        {
            FSR_FND_FlushOp(nDev,
                            nFlushOpCaller | ((nDie + 0x1) & 0x01),
                            nFlag | FSR_LLD_FLAG_REMAIN_PREOP_STAT);
        }
#endif

        /* set DFS, FBA */
        FND_WRITE(pstFOReg->nStartAddr1,
        (UINT16)((nDie << FSR_FND_DFS_BASEBIT) | (nPbn & pstFNDCxt->nFBAMask)));

        if (nCmdIdx != FSR_LLD_FLAG_OTP_PROGRAM)
        {

#if defined(FSR_LLD_WAIT_WR_PROTECT_STAT)
            /* Wait until lock bit is set */
            do
            {
                nWrProtectStat = FND_READ(pstFOReg->nWrProtectStat);
            } while (nWrProtectStat == (UINT16) 0x0000);
#else
            nWrProtectStat = FND_READ(pstFOReg->nWrProtectStat);
#endif
            
            /******************CAUTION ***************************************
             * When cache program (0x007F) is performed on a locked block    *
             * Flex-OneNAND automatically resets itself                      *
             * so makes sure that cache program command is not issued on a   *
             * locked block                                                  *
             *****************************************************************
             */

            /* Write protection can be checked when DBS & FBA is set */
            if ((nWrProtectStat & FSR_LLD_BLK_STAT_MASK) != FSR_LLD_BLK_STAT_UNLOCKED)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[FND:ERR]   %s() / %d line \r\n"),
                    __FSR_FUNC__, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            Pbn #%d, Pg #%d is write protected\r\n"),
                    nPbn, nPgOffset));

                _DumpRegisters(pstFOReg);
                _DumpSpareBuffer(pstFOReg);

                nLLDRe = (FSR_LLD_WR_PROTECT_ERROR | FSR_LLD_1STPLN_CURR_ERROR);
                break;
            }
        }

        /* Set Start Page Address (FPA) */
        FND_WRITE(pstFOReg->nStartAddr8,
                  (UINT16) (nPgOffset << pstFNDCxt->nFPASelSft));

        /* Set Start Buffer Register */
        FND_WRITE(pstFOReg->nStartBuf, FSR_FND_START_BUF_DEFAULT);

        if ((nFlag & FSR_LLD_FLAG_ECC_MASK) == FSR_LLD_FLAG_ECC_ON)
        {
            FND_CLR(pstFOReg->nSysConf1, FSR_FND_CONF1_ECC_ON);
        }
        else
        {
            FND_SET(pstFOReg->nSysConf1, FSR_FND_CONF1_ECC_OFF);
        }

        /* In case of non-blocking mode, interrupt should be enabled */
        if (nFlag & FSR_LLD_FLAG_INT_MASK)
        {
            FSR_OAM_ClrNEnableInt(pstFNDCxt->nIntID);
        }
        else
        {
            FSR_OAM_ClrNDisableInt(pstFNDCxt->nIntID);
        }

        /* Issue command */
        FND_WRITE(pstFOReg->nCmd, (UINT16) gnPgmCmdArray[nCmdIdx]);

#if defined (FSR_LLD_READ_PRELOADED_DATA)
        pstFNDShMem->bIsPreCmdLoad[nDie] = FALSE32;
#endif

#if defined(FSR_LLD_USE_CACHE_PGM)
        if (gnPgmCmdArray[nCmdIdx] == FSR_FND_CMD_CACHEPGM)
        {
            pstFNDShMem->nPreOp[nDie] = FSR_FND_PREOP_CACHE_PGM;
            pstFNDCxt->bIsPreCmdCache[nDie] = TRUE32;
        }
        else
        {
            if (pstFNDCxt->bIsPreCmdCache[nDie] == TRUE32)
            {
                pstFNDShMem->nPreOp[nDie] = FSR_FND_PREOP_CACHE_PGM;
            }
            else
            {
                pstFNDShMem->nPreOp[nDie] = FSR_FND_PREOP_PROGRAM;
            }

            pstFNDCxt->bIsPreCmdCache[nDie] = FALSE32;
        }
#else
        pstFNDShMem->nPreOp[nDie] = FSR_FND_PREOP_PROGRAM;
#endif  /* #if defined(FSR_LLD_USE_CACHE_PGM) */

        pstFNDShMem->nPreOpPbn[nDie]      = (UINT16) nPbn;
        pstFNDShMem->nPreOpPgOffset[nDie] = (UINT16) nPgOffset;
        pstFNDShMem->nPreOpFlag[nDie]     = nFlag;

#if defined (FSR_LLD_LOGGING_HISTORY)
        _AddLog(nDev, nDie);
#endif

#if defined (FSR_LLD_STATISTICS)
        if (gnPgmCmdArray[nCmdIdx] == FSR_FND_CMD_CACHEPGM)
        {
            _AddFNDStat(nDev,
                        nDie,
                        FSR_FND_STAT_WR_TRANS,
                        nBytesTransferred,
                        FSR_FND_STAT_CACHE_PGM);

            _AddFNDStatPgm(nDev,
                        nDie,
                        nPbn,
                        nPgOffset,
                        FSR_FND_STAT_CACHE_PGM);
        }
        else
        {
            _AddFNDStat(nDev,
                        nDie,
                        FSR_FND_STAT_WR_TRANS,
                        nBytesTransferred,
                        FSR_FND_STAT_NORMAL_CMD);

            _AddFNDStatPgm(nDev,
                        nDie,
                        nPbn,
                        nPgOffset,
                        FSR_FND_STAT_NORMAL_CMD);
        }
#endif /* #if defined (FSR_LLD_STATISTICS) */
    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[FND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));
    
    return (nLLDRe);
}



/**
 * @brief           This function writes data into main area of DataRAM
 *
 * @param[in]       pDest   : pointer to the main area of DataRAM
 * @param[in]       pSrc    : pointer to the buffer
 * @param[in]       nSize   : the number of bytes to write
 *
 * @return          none
 *
 * @author          NamOh Hwang
 * @version         1.0.0
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

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[FND:IN ] ++%s(pDest: 0x%08x, pSrc: 0x%08x, nSize: %d)\r\n"),
        __FSR_FUNC__, pDest, pSrc, nSize));

    /* 
     * If nSize is not multiple of 4, memcpy is not guaranteed.
     * Although here only checks that nSize is multiple of 4, 
     * nSize should not be an odd.
     */
    if (((nSize & 0x3) != 0) ||
        (((UINT32) pSrc & 0x3) != 0) ||
        (nSize < FSR_FND_MIN_BULK_TRANS_SIZE))
    {
        pTgt16 = (volatile UINT16 *) pDest;

        /* Copy it by word, so loop it for nSize/2 */
        nSize  = nSize >> 1;
        for (nCnt = 0; nCnt < nSize; nCnt++)
        {
            nReg   = *pSrc++;
            nReg  |= *pSrc++ << 8;
            FND_WRITE(*pTgt16++, nReg);
        }
    }
    else
    {
        /* nSize is the number of bytes to transfer */
        TRANSFER_TO_NAND(pDest, pSrc, nSize);
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[FND:OUT] --%s()\r\n"), __FSR_FUNC__));
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
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark          this function writes FSRSpareBuf over spare area of 1 page
 *
 */
PRIVATE VOID
_WriteSpare(volatile UINT8       *pDest,
                     FSRSpareBuf *pstSrc,
                     UINT32       nFlag)
{
             UINT32   nNumOfSct;
             UINT32   nMetaIdx;
             UINT16  *pSrc16;
    volatile UINT16  *pTgt16;
             UINT8    anSpareBuf[FSR_SPARE_BUF_BASE_SIZE + 2 * sizeof(FSRSpareBufExt)];

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[FND:IN ] ++%s(pDest: 0x%08x, pstSrc: 0x%08x)\r\n"),
        __FSR_FUNC__, pDest, pstSrc));

    /* When pstSrc->nNumOfMetaExt is 0, fill the rest of spare area with 0xFF */
    FSR_OAM_MEMSET(&anSpareBuf[0], 0xFF, sizeof(anSpareBuf));

    if (nFlag & FSR_LLD_FLAG_USE_SPAREBUF)
    {
        FSR_OAM_MEMCPY(&anSpareBuf[0], &pstSrc->pstSpareBufBase[0], FSR_SPARE_BUF_BASE_SIZE);

        for (nMetaIdx = 0; nMetaIdx < pstSrc->nNumOfMetaExt; nMetaIdx++)
        {
            FSR_OAM_MEMCPY(&anSpareBuf[FSR_SPARE_BUF_BASE_SIZE + nMetaIdx * FSR_SPARE_BUF_EXT_SIZE],
                           &pstSrc->pstSTLMetaExt[nMetaIdx],
                           FSR_SPARE_BUF_EXT_SIZE);
        }
    }

    pSrc16 = (UINT16 *) &anSpareBuf[0];
    pTgt16 = (volatile UINT16 *) pDest;

    FSR_ASSERT(((UINT32) pSrc16 & 0x01) == 0);

    nNumOfSct = sizeof(anSpareBuf) / FSR_FND_SPARE_USER_AREA;

    do
    {
        /* Unable to use sync write to write data in spare area */
        FND_WRITE(*pTgt16++, *pSrc16++);
        FND_WRITE(*pTgt16++, *pSrc16++);
        FND_WRITE(*pTgt16++, *pSrc16++);

        /* Fill 0xFFFF on the the rest of spare except user-used spare area */
        FND_WRITE(*pTgt16++,  0xFFFF);
        FND_WRITE(*pTgt16++,  0xFFFF);
        FND_WRITE(*pTgt16++,  0xFFFF);
        FND_WRITE(*pTgt16++,  0xFFFF);
        FND_WRITE(*pTgt16++,  0xFFFF);        
    } while (--nNumOfSct > 0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[FND:OUT] --%s()\r\n"), __FSR_FUNC__));
}

/**
 * @brief           This function erase a block of Flex-OneNAND
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
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark          as of now, supports only single plane, one block erase
 * @                Pbn is consecutive numbers within the device
 * @n               i.e. for DDP device, Pbn for 1st block in 2nd chip is not 0
 * @n               it is one more than the last block number in 1st chip
 *
 */
PUBLIC INT32
FSR_FND_Erase(UINT32  nDev,
              UINT32 *pnPbn,
              UINT32  nNumOfBlks,
              UINT32  nFlag)
{
             FlexONDCxt       *pstFNDCxt;
             FlexONDSpec      *pstFNDSpec;
             FlexONDShMem     *pstFNDShMem;
    volatile FlexOneNANDReg   *pstFOReg;
             UINT32            nCmdIdx;
             UINT32            nDie;
             UINT32            nPbn       = *pnPbn;
             UINT32            nFlushOpCaller;
             INT32             nLLDRe     = FSR_LLD_SUCCESS;
             UINT16            nWrProtectStat;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[FND:IN ] ++%s(nDev:%d, *pnPbn:%d, nNumOfBlks:%d)\r\n"),
        __FSR_FUNC__, nDev, *pnPbn, nNumOfBlks));

    do
    {
#if defined (FSR_LLD_STRICT_CHK)
        /* Check device number */
        if (nDev >= FSR_FND_MAX_DEVS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("[FND:ERR]   %s(nDev:%d, *pnPbn:%d, nNumOfBlks:%d, nFlag:0x%08x) / %d line\r\n"),
            __FSR_FUNC__, nDev, *pnPbn, nNumOfBlks, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("            Invalid Device Number (nDev = %d)\r\n"), nDev));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }
#endif

        pstFNDCxt   = gpstFNDCxt[nDev];
        pstFNDShMem = gpstFNDShMem[nDev];
        pstFNDSpec  = pstFNDCxt->pstFNDSpec;
        pstFOReg    = (volatile FlexOneNANDReg *) pstFNDCxt->nBaseAddr;

        /* To support multi block erase,
         * function parameter pnPbn is defined as an array
         * though multi block erase is not supported yet.
         * nNumOfBlks can have a value of 1 for now.
         */
        FSR_ASSERT(nNumOfBlks == 1);

#if defined (FSR_LLD_STRICT_CHK)
        if (nPbn >= pstFNDSpec->nNumOfBlks)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("[FND:ERR]   %s(nDev:%d, *pnPbn:%d, nNumOfBlks:%d, nFlag:0x%08x) / %d line\r\n"),
            __FSR_FUNC__, nDev, *pnPbn, nNumOfBlks, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Pbn:%d is invalid\r\n"), nPbn));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */

        nDie = nPbn >> (FSR_FND_DFS_BASEBIT - pstFNDCxt->nDDPSelSft);

        nFlushOpCaller = FSR_FND_PREOP_ERASE << FSR_FND_FLUSHOP_CALLER_BASEBIT;

        nLLDRe = FSR_FND_FlushOp(nDev, nFlushOpCaller | nDie, nFlag);
        if (nLLDRe != FSR_LLD_SUCCESS)
        {
            break;
        }

        /* WARNING : DO NOT change address setting sequence 
           1. set DBS 
           2. set FBA */ 

        /* Set DBS */
        FND_WRITE(pstFOReg->nStartAddr2,
            (UINT16) (nDie << FSR_FND_DBS_BASEBIT));

        /* Set DFS, FBA */
        FND_WRITE(pstFOReg->nStartAddr1,
        (UINT16)((nDie << FSR_FND_DFS_BASEBIT) | (nPbn & pstFNDCxt->nFBAMask)));

#if defined(FSR_LLD_WAIT_WR_PROTECT_STAT)
        /* Wait until lock bit is set */
        do
        {
            nWrProtectStat = FND_READ(pstFOReg->nWrProtectStat);
        } while (nWrProtectStat == (UINT16) 0x0000);
#else
        nWrProtectStat = FND_READ(pstFOReg->nWrProtectStat);
#endif
        /* Write protection can be checked at this point */
        if ((nWrProtectStat & FSR_LLD_BLK_STAT_MASK) != FSR_LLD_BLK_STAT_UNLOCKED)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("[FND:ERR]   %s(nDev:%d, *pnPbn:%d, nNumOfBlks:%d, nFlag:0x%08x) / %d line\r\n"),
            __FSR_FUNC__, nDev, *pnPbn, nNumOfBlks, nFlag, __LINE__));


            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Pbn #%d is write protected\r\n"), nPbn));

            _DumpRegisters(pstFOReg);
            _DumpSpareBuffer(pstFOReg);

            nLLDRe = (FSR_LLD_WR_PROTECT_ERROR | FSR_LLD_1STPLN_CURR_ERROR);
            break;
        }

        /* In case of non-blocking mode, interrupt should be enabled */
        if (nFlag & FSR_LLD_FLAG_INT_MASK)
        {
            FSR_OAM_ClrNEnableInt(pstFNDCxt->nIntID);
        }
        else
        {
            FSR_OAM_ClrNDisableInt(pstFNDCxt->nIntID);
        }

        nCmdIdx = (nFlag & FSR_LLD_FLAG_CMDIDX_MASK) >> FSR_LLD_FLAG_CMDIDX_BASEBIT;
        FND_WRITE(pstFOReg->nCmd, (UINT16) gnEraseCmdArray[nCmdIdx]);

#if defined (FSR_LLD_RUNTIME_ERASE_REFRESH)
        pstFNDCxt->pReadCnt[nPbn] = 0x0;
#endif

#if defined (FSR_LLD_READ_PRELOADED_DATA)
        pstFNDShMem->bIsPreCmdLoad[nDie] = FALSE32;
#endif

        pstFNDShMem->nPreOp[nDie]                 = FSR_FND_PREOP_ERASE;
        pstFNDShMem->nPreOpPbn[nDie]              = (UINT16) nPbn;
        pstFNDShMem->nPreOpPgOffset[nDie]         = FSR_FND_PREOP_ADDRESS_NONE;
        pstFNDShMem->nPreOpFlag[nDie]             = nFlag;

#if defined (FSR_LLD_LOGGING_HISTORY)
        _AddLog(nDev, nDie);
#endif

#if defined (FSR_LLD_STATISTICS)
        _AddFNDStat(nDev,
                    nDie,
                    FSR_FND_STAT_ERASE,
                    0,
                    FSR_FND_STAT_NORMAL_CMD);
#endif /* #if defined (FSR_LLD_STATISTICS) */

    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[FND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return (nLLDRe);
}



/**
 * @brief           This function copybacks 1 page.
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
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark          this function loads data from Flex-OneNAND,
 * @n               and randomly writes data into DataRAM of Flex-OneNAND
 * @n               and program back into Flex-OneNAND
 *
 */
PUBLIC INT32
FSR_FND_CopyBack(UINT32      nDev,
                 LLDCpBkArg *pstCpArg,
                 UINT32      nFlag)
{
             FlexONDCxt       *pstFNDCxt;
             FlexONDSpec      *pstFNDSpec;
             FlexONDShMem     *pstFNDShMem;
    volatile FlexOneNANDReg   *pstFOReg;

             LLDRndInArg      *pstRIArg; /* Random-in argument */

             UINT32            nCmdIdx;
             UINT32            nCnt;
             UINT32            nPgOffset    = 0;
             UINT32            nDie         = 0;
             UINT32            nPbn         = 0;
             UINT32            nRndOffset   = 0;
             UINT32            nFlushOpCaller;
             BOOL32            bSpareBufRndIn;

             FSRSpareBuf       stSpareBuf;
             FSRSpareBufBase   stSpareBufBase;
             FSRSpareBufExt    stSpareBufExt[FSR_MAX_SPARE_BUF_EXT];

#if defined (FSR_LLD_STATISTICS)
             UINT32            nBytesTransferred = 0;
#endif /* #if defined (FSR_LLD_STATISTICS) */

             INT32             nLLDRe       = FSR_LLD_SUCCESS;
             UINT16            nSysConf1;
             UINT16            nWrProtectStat;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[FND:IN ] ++%s(nDev:%d, nFlag:0x%08x)\r\n"),
        __FSR_FUNC__, nDev, nFlag));

    do
    {
        

#if defined (FSR_LLD_STRICT_CHK)
        /* check device number */
        if (nDev >= FSR_FND_MAX_DEVS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[FND:ERR]   %s(nDev:%d, nFlag:0x%08x) / %d line\r\n"),
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
                (TEXT("[FND:ERR]   %s(nDev:%d, pstCpArg:0x%08x, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, pstCpArg, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            pstCpArg is NULL\r\n")));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */

        pstFNDCxt  = gpstFNDCxt[nDev];
        pstFNDShMem= gpstFNDShMem[nDev];
        pstFNDSpec = pstFNDCxt->pstFNDSpec;
        pstFOReg   = (volatile FlexOneNANDReg *) pstFNDCxt->nBaseAddr;

        nCmdIdx = (nFlag & FSR_LLD_FLAG_CMDIDX_MASK) >> FSR_LLD_FLAG_CMDIDX_BASEBIT;

        FSR_OAM_MEMSET(&stSpareBuf, 0x00, sizeof(FSRSpareBuf));

        if (nCmdIdx == FSR_LLD_FLAG_1X_CPBK_LOAD)
        {
            FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_INF,
                (TEXT("[FND:INF]   %s(nDev:%d, nSrcPbn:%d, nSrcPg:%d, nFlag:0x%08x)\r\n"),
                __FSR_FUNC__, nDev, pstCpArg->nSrcPbn, pstCpArg->nSrcPgOffset, nFlag));

            /* 
             * Load phase of copyback() only checks the source block & page
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

            nDie = nPbn >> (FSR_FND_DFS_BASEBIT - pstFNDCxt->nDDPSelSft);

            nFlushOpCaller = FSR_FND_PREOP_CPBK_LOAD << FSR_FND_FLUSHOP_CALLER_BASEBIT;

            /* 
             * OneNAND doesn't support read interleaving,
             * therefore, wait until interrupt status is ready for both die. 
             * if device is DDP. 
             */
            if (pstFNDSpec->nNumOfDies == FSR_MAX_DIES)
            {
                FSR_FND_FlushOp(nDev,
                                nFlushOpCaller | ((nDie + 0x1) & 0x1),
                                nFlag | FSR_LLD_FLAG_REMAIN_PREOP_STAT);
            }

            nLLDRe = FSR_FND_FlushOp(nDev, nFlushOpCaller | nDie, nFlag);
            if ((FSR_RETURN_MAJOR(nLLDRe) != FSR_LLD_SUCCESS) &&
                (FSR_RETURN_MAJOR(nLLDRe) != FSR_LLD_PREV_2LV_READ_DISTURBANCE) &&
                (FSR_RETURN_MAJOR(nLLDRe) != FSR_LLD_PREV_1LV_READ_DISTURBANCE))
            {
                break;
            }

            /* Set DBS */
            FND_WRITE(pstFOReg->nStartAddr2,
                (UINT16) (nDie << FSR_FND_DBS_BASEBIT));

            /* Set DFS & FBA */
            FND_WRITE(pstFOReg->nStartAddr1,
                (UINT16) ((nDie << FSR_FND_DFS_BASEBIT) | (nPbn & pstFNDCxt->nFBAMask)));

            /* Set FPA (Flash Page Address) */
            FND_WRITE(pstFOReg->nStartAddr8,
                      (UINT16) nPgOffset << pstFNDCxt->nFPASelSft);

            /* set Start Buffer Register */
            FND_WRITE(pstFOReg->nStartBuf, FSR_FND_START_BUF_DEFAULT);

            if ((nFlag & FSR_LLD_FLAG_ECC_MASK) == FSR_LLD_FLAG_ECC_ON)
            {
                nSysConf1 = FND_READ(pstFOReg->nSysConf1);
                if (nSysConf1 & FSR_FND_CONF1_ECC_OFF)
                {
                    FND_CLR(pstFOReg->nSysConf1, FSR_FND_CONF1_ECC_ON);
                }
            }
            else
            {
                FND_SET(pstFOReg->nSysConf1, FSR_FND_CONF1_ECC_OFF);
            }

            /* In case of non-blocking mode, interrupt should be enabled */
            if (nFlag & FSR_LLD_FLAG_INT_MASK)
            {
                FSR_OAM_ClrNEnableInt(pstFNDCxt->nIntID);
            }
            else
            {
                FSR_OAM_ClrNDisableInt(pstFNDCxt->nIntID);
            }

            FND_WRITE(pstFOReg->nCmd, (UINT16) gnCpBkCmdArray[nCmdIdx]);

#if defined (FSR_LLD_READ_PRELOADED_DATA)
            pstFNDShMem->bIsPreCmdLoad[nDie]  = FALSE32;
#endif

#if defined (FSR_LLD_STATISTICS)

            _AddFNDStatLoad(nDev,
                            nDie,
                            nPbn,
                            FSR_FND_STAT_NORMAL_CMD);

#endif /* #if defined (FSR_LLD_STATISTICS) */

            pstFNDShMem->nPreOp[nDie]         = FSR_FND_PREOP_CPBK_LOAD;
        }
        else if (nCmdIdx == FSR_LLD_FLAG_1X_CPBK_PROGRAM)
        {
            FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_INF,
            (TEXT("[FND:INF]   %s(nDev:%d, nDstPbn:%d, nDstPg:%d, nRndInCnt:%d, nFlag:0x%08x)\r\n"),
            __FSR_FUNC__, nDev, pstCpArg->nDstPbn, pstCpArg->nDstPgOffset, pstCpArg->nRndInCnt, nFlag));

            nPbn      = pstCpArg->nDstPbn;
            nPgOffset = pstCpArg->nDstPgOffset;

            nDie = nPbn >> (FSR_FND_DFS_BASEBIT - pstFNDCxt->nDDPSelSft);

#if defined (FSR_LLD_STRICT_CHK)
            nLLDRe = _StrictChk(nDev, nPbn, nPgOffset);

            if (nLLDRe != FSR_LLD_SUCCESS)
            {
                break;
            }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */

            nFlushOpCaller = FSR_FND_PREOP_CPBK_PGM << FSR_FND_FLUSHOP_CALLER_BASEBIT;

            nLLDRe = FSR_FND_FlushOp(nDev, nFlushOpCaller | nDie, nFlag);

            if ((FSR_RETURN_MAJOR(nLLDRe) != FSR_LLD_SUCCESS) &&
                (FSR_RETURN_MAJOR(nLLDRe) != FSR_LLD_PREV_2LV_READ_DISTURBANCE) &&
                (FSR_RETURN_MAJOR(nLLDRe) != FSR_LLD_PREV_1LV_READ_DISTURBANCE))
            {
                break;
            }

            /* Set DBS */
            FND_WRITE(pstFOReg->nStartAddr2,
                (UINT16) (nDie << FSR_FND_DBS_BASEBIT));

            bSpareBufRndIn = FALSE32;

            for (nCnt = 0; nCnt < pstCpArg->nRndInCnt; nCnt++)
            {
                pstRIArg = pstCpArg->pstRndInArg + nCnt;
                if (pstRIArg->nOffset >= FSR_LLD_CPBK_SPARE)
                {
                    bSpareBufRndIn = TRUE32;

                    stSpareBuf.pstSpareBufBase  = &stSpareBufBase;
                    stSpareBuf.nNumOfMetaExt    = 2;
                    stSpareBuf.pstSTLMetaExt    = &stSpareBufExt[0];

                    _ReadSpare(&stSpareBuf, &pstFOReg->nDataSB00[0]);

                    break;
                }
            }

            for (nCnt = 0; nCnt < pstCpArg->nRndInCnt; nCnt++)
            {
                pstRIArg = pstCpArg->pstRndInArg + nCnt;

                /* In case copyback of spare area is requested */
                if (pstRIArg->nOffset >= FSR_LLD_CPBK_SPARE)
                {
                    nRndOffset = pstRIArg->nOffset - FSR_LLD_CPBK_SPARE;

                    if (nRndOffset >= (FSR_SPARE_BUF_BASE_SIZE))
                    {
                        /* Random-in to FSRSpareBufExt[] */
                        nRndOffset -= (FSR_SPARE_BUF_BASE_SIZE * 1);

                        FSR_ASSERT((pstRIArg->nNumOfBytes + nRndOffset) <= (FSR_SPARE_BUF_EXT_SIZE * FSR_MAX_SPARE_BUF_EXT));

                        /* Random-in to FSRSpareBuf */
                        FSR_OAM_MEMCPY((UINT8 *) &(stSpareBuf.pstSTLMetaExt[0]) + nRndOffset,
                                       (UINT8 *) pstRIArg->pBuf,
                                        pstRIArg->nNumOfBytes);
                    }
                    else
                    {
                        FSR_ASSERT((pstRIArg->nNumOfBytes + nRndOffset) <= FSR_SPARE_BUF_BASE_SIZE);

                        /* Random-in to FSRSpareBuf */
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

            /* Set DFS & FBA */
            FND_WRITE(pstFOReg->nStartAddr1,
                (UINT16) ((nDie << FSR_FND_DFS_BASEBIT) | (nPbn & pstFNDCxt->nFBAMask)));

#if defined(FSR_LLD_WAIT_WR_PROTECT_STAT)
            do
            {
                nWrProtectStat = FND_READ(pstFOReg->nWrProtectStat);
            } while (nWrProtectStat == (UINT16) 0x0000);
#else
            nWrProtectStat = FND_READ(pstFOReg->nWrProtectStat);
#endif

            /* Write protection can be checked at this point */
            if ((nWrProtectStat & FSR_LLD_BLK_STAT_MASK) != FSR_LLD_BLK_STAT_UNLOCKED)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[FND:ERR]   %s(nDev:%d, nDstPbn:%d, nDstPg:%d, nFlag:0x%08x) / %d line\r\n"),
                    __FSR_FUNC__, nDev, pstCpArg->nDstPbn,
                    pstCpArg->nDstPgOffset, nFlag,  __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            Pbn:%d is write protected\r\n"), nPbn));

                _DumpRegisters(pstFOReg);
                _DumpSpareBuffer(pstFOReg);

                nLLDRe = (FSR_LLD_WR_PROTECT_ERROR | FSR_LLD_1STPLN_CURR_ERROR);
                break;
            }

            /* Set FPA (Flash Page Address) */
            FND_WRITE(pstFOReg->nStartAddr8,
                (UINT16) nPgOffset << pstFNDCxt->nFPASelSft);

            /* Set Start Buffer Register */
            FND_WRITE(pstFOReg->nStartBuf, FSR_FND_START_BUF_DEFAULT);

            if ((nFlag & FSR_LLD_FLAG_ECC_MASK) == FSR_LLD_FLAG_ECC_ON)
            {
                FND_CLR(pstFOReg->nSysConf1, FSR_FND_CONF1_ECC_ON);
            }
            else
            {
                FND_SET(pstFOReg->nSysConf1, FSR_FND_CONF1_ECC_OFF);
            }

            /* In case of non-blocking mode, interrupt should be enabled */
            if (nFlag & FSR_LLD_FLAG_INT_MASK)
            {
                FSR_OAM_ClrNEnableInt(pstFNDCxt->nIntID);
            }
            else
            {
                FSR_OAM_ClrNDisableInt(pstFNDCxt->nIntID);
            }


            FND_WRITE(pstFOReg->nCmd, (UINT16) gnCpBkCmdArray[nCmdIdx]);

#if defined (FSR_LLD_READ_PRELOADED_DATA)
            pstFNDShMem->bIsPreCmdLoad[nDie] = FALSE32;
#endif

#if defined (FSR_LLD_STATISTICS)
            _AddFNDStat(nDev,
                        nDie,
                        FSR_FND_STAT_WR_TRANS,
                        nBytesTransferred,
                        FSR_FND_STAT_NORMAL_CMD);

            _AddFNDStatPgm(nDev,
                        nDie,
                        nPbn,
                        nPgOffset,
                        FSR_FND_STAT_NORMAL_CMD);
#endif /* #if defined (FSR_LLD_STATISTICS) */

            /* Store the type of previous operation for the deferred check */
            pstFNDShMem->nPreOp[nDie]                 = FSR_FND_PREOP_CPBK_PGM;
        }

        pstFNDShMem->nPreOpPbn[nDie]      = (UINT16) nPbn;
        pstFNDShMem->nPreOpPgOffset[nDie] = (UINT16) nPgOffset;
        pstFNDShMem->nPreOpFlag[nDie]     = nFlag;

#if defined (FSR_LLD_LOGGING_HISTORY)
        _AddLog(nDev, nDie);
#endif
    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[FND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

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
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark          Pbn is consecutive numbers within the device
 * @n               i.e. for DDP device, Pbn for 1st block in 2nd chip is not 0
 * @n               it is one more than the last block number in 1st chip
 *
 */
PUBLIC INT32
FSR_FND_ChkBadBlk(UINT32 nDev,
                  UINT32 nPbn,
                  UINT32 nFlag)
{
             FlexONDCxt       *pstFNDCxt;
    volatile FlexOneNANDReg   *pstFOReg;
             INT32             nLLDRe       = FSR_LLD_INIT_GOODBLOCK;
             UINT16            nDQ;
             UINT32            nPageOffset;
             BOOL32            bIsBadBlk;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[FND:IN ] ++%s(nDev:%d, nPbn:%d, nFlag:0x%08x)\r\n"),
        __FSR_FUNC__, nDev, nPbn, nFlag));

    do
    {

#if defined (FSR_LLD_STRICT_CHK)
        /* Check device number */
        if (_StrictChk(nDev, nPbn, 0) != FSR_LLD_SUCCESS)
        {
            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }
#endif

        pstFNDCxt   = gpstFNDCxt[nDev];
        pstFOReg    = (volatile FlexOneNANDReg *) pstFNDCxt->nBaseAddr;
        bIsBadBlk   = FALSE32;

        for (nPageOffset = 0; nPageOffset < FSR_FND_NUM_OF_BAD_MARK_PAGES; nPageOffset++)
        {
            /* No need to check the return value (ignore the possibility of read error) */
            FSR_FND_ReadOptimal(nDev,
                                nPbn,
                                nPageOffset,
                                NULL,
                                NULL,
                                (FSR_LLD_FLAG_ECC_OFF | FSR_LLD_FLAG_1X_LOAD));

            FSR_FND_ReadOptimal(nDev,
                                nPbn,
                                nPageOffset,
                                NULL,
                                NULL,
                                (FSR_LLD_FLAG_ECC_OFF | FSR_LLD_FLAG_TRANSFER));

            nDQ = FND_READ(*(volatile UINT16 *) &pstFOReg->nDataSB00[0]);

            /* Simply check if this bad information is valid block mark in Flex-OneNAND */
            if (nDQ != (UINT16) FSR_FND_VALID_BLK_MARK)
            {
                bIsBadBlk = TRUE32;
            }
            else
            {
                bIsBadBlk = FALSE32;
                break;
            }
        }
        
        if(bIsBadBlk == TRUE32)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR | FSR_DBZ_LLD_INF,
            (TEXT("[FND:INF]   %s(nDev:%d, nPbn:%d, nFlag:0x%08x) / %d line\r\n"),
            __FSR_FUNC__, nDev, nPbn, nFlag, __LINE__));


            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR | FSR_DBZ_LLD_INF,
            (TEXT("            nPbn = %d is a bad block\r\n"), nPbn));

            nLLDRe = FSR_LLD_INIT_BADBLOCK | FSR_LLD_BAD_BLK_1STPLN;
        }
        else
        {
            FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_INF,
            (TEXT("[FND:INF]   %s(nDev:%d, nPbn:%d, nFlag:0x%08x) / %d line\r\n"),
            __FSR_FUNC__, nDev, nPbn, nFlag, __LINE__));

            FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_INF,
            (TEXT("            nPbn = %d is a good block\r\n"), nPbn));

            nLLDRe = FSR_LLD_INIT_GOODBLOCK;
        }

    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[FND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return (nLLDRe);
}

/**
 * @brief           This function flush previous operation
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
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark          This function completes previous operation,
 * @n               and returns error for previous operation
 * @n               After calling series of FSR_FND_Write() or FSR_FND_Erase() or
 * @n               FSR_FND_CopyBack(), FSR_FND_FlushOp() needs to be called.
 *
 */
PUBLIC INT32
FSR_FND_FlushOp(UINT32 nDev,
                UINT32 nDie,
                UINT32 nFlag)
{
             FlexONDCxt       *pstFNDCxt;
             FlexONDShMem     *pstFNDShMem;
             FlexONDSpec      *pstFNDSpec;
    volatile FlexOneNANDReg   *pstFOReg;
             UINT32            nIdx;
             UINT32            nPrevOp;
             UINT32            nPrevPbn;

#if !defined(FSR_OAM_RTLMSG_DISABLE)
             UINT32            nPrevPgOffset;
#endif
             UINT32            nPrevFlag;
             UINT32            nBlockType;
             INT32             nLLDRe       = FSR_LLD_SUCCESS;
             UINT16            nMasterInt;
             UINT16            nECCRes; /* ECC result */
             UINT16            nECC1LvRdDist;
             UINT16            nECC2LvRdDist;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[FND:IN ] ++%s(nDev: %d, nDieIdx: %d, nFlag: 0x%08x)\r\n"),
        __FSR_FUNC__, nDev, nDie, nFlag));

    do
    {

#if defined (FSR_LLD_STRICT_CHK)
        /* Check device number */
        if (nDev >= FSR_FND_MAX_DEVS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("[FND:ERR]   %s(nDev:%d, nDie:%d, nFlag:0x%08x) / %d line\r\n"),
            __FSR_FUNC__, nDev, nDie, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("            Invalid Device Number (nDev = %d)\r\n"), nDev));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }
#endif

        pstFNDCxt  = gpstFNDCxt[nDev];
        pstFNDShMem= gpstFNDShMem[nDev];
        pstFNDSpec = gpstFNDCxt[nDev]->pstFNDSpec;
        pstFOReg   = (volatile FlexOneNANDReg *) pstFNDCxt->nBaseAddr;

        pstFNDCxt->nFlushOpCaller = (UINT16) (nDie >> FSR_FND_FLUSHOP_CALLER_BASEBIT);
        nDie = nDie & ~FSR_FND_FLUSHOP_CALLER_MASK;

        nPrevOp       = pstFNDShMem->nPreOp[nDie];
        nPrevPbn      = pstFNDShMem->nPreOpPbn[nDie];

#if !defined(FSR_OAM_RTLMSG_DISABLE)
        nPrevPgOffset = pstFNDShMem->nPreOpPgOffset[nDie];
#endif
        nPrevFlag     = pstFNDShMem->nPreOpFlag[nDie];

        /* Set DBS */
        FND_WRITE(pstFOReg->nStartAddr2, (UINT16) (nDie << FSR_FND_DBS_BASEBIT));

        switch (nPrevOp)
        {
        case FSR_FND_PREOP_NONE:
            /* DO NOT remove this code : 'case FSR_OND_PREOP_NONE:'
               for compiler optimization */
            break;

        case FSR_FND_PREOP_READ:
        case FSR_FND_PREOP_CPBK_LOAD:
            WAIT_FND_INT_STAT(pstFOReg, FSR_FND_INT_READ_READY);

            if ((nPrevFlag & FSR_LLD_FLAG_ECC_MASK) == FSR_LLD_FLAG_ECC_ON)
            {
                /* Uncorrectable read error */
                if ((FND_READ(pstFOReg->nCtrlStat) & FSR_FND_STATUS_ERROR) ==
                    FSR_FND_STATUS_ERROR)
                {
                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                        (TEXT("[FND:ERR]   %s(nDev:%d, nDie:%d, nFlag:0x%08x) / %d line\r\n"),
                        __FSR_FUNC__, nDev, nDie, nFlag, __LINE__));

                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                        (TEXT("            pstFNDCxt->nFlushOpCaller : %d\r\n"),
                        pstFNDCxt->nFlushOpCaller));

                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                        (TEXT("            ECC Status : Uncorrectable, CtrlReg=0x%04x\r\n"),
                        FND_READ(pstFOReg->nCtrlStat)));

                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                        (TEXT("            at nDev:%d, nPbn:%d, nPgOffset:%d, nFlag:0x%08x\r\n"),
                        nDev, nPrevPbn, nPrevPgOffset, nPrevFlag));

                    _DumpRegisters(pstFOReg);
                    _DumpSpareBuffer(pstFOReg);

#if defined (FSR_LLD_READ_PRELOADED_DATA)
                    pstFNDShMem->bIsPreCmdLoad[nDie] = FALSE32;
#endif

                    nLLDRe = (FSR_LLD_PREV_READ_ERROR |
                              FSR_LLD_1STPLN_CURR_ERROR);
                }
                /* Correctable error */
                else
                {                    
                    FSR_FND_GetBlockInfo(nDev, nPrevPbn, &nBlockType, NULL);
                    if (nBlockType == FSR_LLD_SLC_BLOCK)
                    {
                        nECC1LvRdDist = pstFNDSpec->nECC1LvRdDistOnSLC;
                        nECC2LvRdDist = pstFNDSpec->nECC2LvRdDistOnSLC;
                    }
                    else
                    {
                        nECC1LvRdDist = pstFNDSpec->nECC1LvRdDistOnMLC;
                        nECC2LvRdDist = pstFNDSpec->nECC2LvRdDistOnMLC;
                    }
                    
                    nIdx = FSR_FND_MAX_ECC_STATUS_REG - 1;

                    do
                    {
                        nECCRes = FND_READ(pstFOReg->nEccStat[nIdx]);

                        if (nECCRes & nECC2LvRdDist)
                        {
                            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
                                (TEXT("[FND:INF]   %s(nDev:%d, nDie:%d, nFlag:0x%08x) / %d line\r\n"),
                                __FSR_FUNC__, nDev, nDie, nFlag, __LINE__));

                            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
                                (TEXT("            pstFNDCxt->nFlushOpCaller : %d\r\n"),
                                pstFNDCxt->nFlushOpCaller));

                            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
                                (TEXT("            2Lv read disturbance at nPbn:%d, nPgOffset:%d, nFlag:0x%08x\r\n"),
                                nPrevPbn, nPrevPgOffset, nPrevFlag));

                            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
                                (TEXT("            Detected at ECC Register[%d]:0x%04x\r\n"),
                                nIdx, nECCRes));

                            _DumpRegisters(pstFOReg);
                            _DumpSpareBuffer(pstFOReg);

#if defined(FSR_LLD_PE_TEST)
                            gnECCStat0 = FND_READ(pstFOReg->nEccStat[0]);
                            gnECCStat1 = FND_READ(pstFOReg->nEccStat[1]);
                            gnECCStat2 = FND_READ(pstFOReg->nEccStat[2]);
                            gnECCStat3 = FND_READ(pstFOReg->nEccStat[3]);
                            
                            gnPbn      = nPrevPbn;
                            gnPgOffset = nPrevPgOffset;
#endif

                            nLLDRe = FSR_LLD_PREV_2LV_READ_DISTURBANCE |
                                     FSR_LLD_1STPLN_CURR_ERROR;
                            break;
                        }      
                        else if (nECCRes & nECC1LvRdDist)
                        {
                            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
                                (TEXT("[FND:INF]   %s(nDev:%d, nDie:%d, nFlag:0x%08x) / %d line\r\n"),
                                __FSR_FUNC__, nDev, nDie, nFlag, __LINE__));

                            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
                                (TEXT("            pstFNDCxt->nFlushOpCaller : %d\r\n"),
                                pstFNDCxt->nFlushOpCaller));

                            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
                                (TEXT("            1Lv read disturbance at nPbn:%d, nPgOffset:%d, nFlag:0x%08x\r\n"),
                                nPrevPbn, nPrevPgOffset, nPrevFlag));

                            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
                                (TEXT("            Detected at ECC Register[%d]:0x%04x\r\n"),
                                nIdx, nECCRes));

#if defined(FSR_LLD_PE_TEST)
                            gnECCStat0 = FND_READ(pstFOReg->nEccStat[0]);
                            gnECCStat1 = FND_READ(pstFOReg->nEccStat[1]);
                            gnECCStat2 = FND_READ(pstFOReg->nEccStat[2]);
                            gnECCStat3 = FND_READ(pstFOReg->nEccStat[3]);
                            
                            gnPbn      = nPrevPbn;
                            gnPgOffset = nPrevPgOffset;
#endif

                            nLLDRe = FSR_LLD_PREV_1LV_READ_DISTURBANCE |
                                     FSR_LLD_1STPLN_CURR_ERROR;                            
                        }
                    } while (nIdx-- > 0);
                }
            }

#if defined (FSR_LLD_RUNTIME_ERASE_REFRESH)
            if (pstFNDCxt->pReadCnt[pstFNDShMem->nPreOpPbn[nDie]] >= FSR_RUNTIME_ER_MAX_READ_CNT)
            {
                nLLDRe = FSR_LLD_PREV_2LV_READ_DISTURBANCE | FSR_LLD_1STPLN_CURR_ERROR;
            }
#endif
            break;

        case FSR_FND_PREOP_PROGRAM:
        case FSR_FND_PREOP_CACHE_PGM:
        case FSR_FND_PREOP_CPBK_PGM:
            /*
             * If the the current operation is cache program operation or 
             *                                  interleave cache program,
             * master interrupt & write interrupt bit should be checked
             * even though the last command is program command.
             * -----------------------------------------------------------
             * Cache program operation is as follows
             * 1. cache_pgm
             * 2. cache_pgm
             * 3. program               
             */
            if (nPrevOp == FSR_FND_PREOP_CACHE_PGM)
            {
                nMasterInt = FSR_FND_INT_MASTER_READY;
            }
            else
            {
                nMasterInt = 0;
            }

            WAIT_FND_INT_STAT(pstFOReg, nMasterInt | FSR_FND_INT_WRITE_READY);

            /* 
             * Previous error check
             * in case of Cache Program, Error bit shows the accumulative
             * error status of Cache Program
             * so if an error occurs this bit stays as fail 
             */
            if ((FND_READ(pstFOReg->nCtrlStat) & FSR_FND_STATUS_ERROR) == 
                FSR_FND_STATUS_ERROR)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[FND:ERR]   %s(nDev:%d, nDie:%d, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, nDie, nFlag, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            pstFNDCxt->nFlushOpCaller : %d\r\n"),
                    pstFNDCxt->nFlushOpCaller));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            write() error: CtrlReg=0x%04x\r\n"),
                    FND_READ(pstFOReg->nCtrlStat) ));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            last write() call @ nDev = %d, nPbn = %d, nPgOffset = %d, nFlag:0x%08x\r\n"),
                    nDev, nPrevPbn, nPrevPgOffset, nPrevFlag));

                _DumpRegisters(pstFOReg);
                _DumpSpareBuffer(pstFOReg);

                nLLDRe = FSR_LLD_PREV_WRITE_ERROR;

                if (!(FND_READ(pstFOReg->nCtrlStat) &
                      (FSR_FND_CURR_CACHEPGM_ERROR | FSR_FND_PREV_CACHEPGM_ERROR)))
                {
                    nLLDRe |= FSR_LLD_1STPLN_CURR_ERROR;
                }

                if (FND_READ(pstFOReg->nCtrlStat) & FSR_FND_CURR_CACHEPGM_ERROR)
                {
                    nLLDRe |= FSR_LLD_1STPLN_CURR_ERROR;
                }

                if (FND_READ(pstFOReg->nCtrlStat) & FSR_FND_PREV_CACHEPGM_ERROR)
                {
                    nLLDRe |= FSR_LLD_1STPLN_PREV_ERROR;
                }
            }

            break;

        case FSR_FND_PREOP_ERASE:

            WAIT_FND_INT_STAT(pstFOReg, FSR_FND_INT_ERASE_READY);

            /* Previous error check */
            if ((FND_READ(pstFOReg->nCtrlStat) & FSR_FND_STATUS_ERROR) ==
                FSR_FND_STATUS_ERROR)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[FND:ERR]   %s(nDev:%d, nDie:%d, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, nDie, nFlag, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            pstFNDCxt->nFlushOpCaller : %d\r\n"),
                    pstFNDCxt->nFlushOpCaller));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            erase error: CtrlReg=0x%04x\r\n"),
                    FND_READ(pstFOReg->nCtrlStat) ));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            last Erase() call @ nDev = %d, nPbn = %d, nFlag:0x%08x\r\n"),
                    nDev, nPrevPbn, nPrevFlag));

                _DumpRegisters(pstFOReg);
                _DumpSpareBuffer(pstFOReg);

                nLLDRe = (FSR_LLD_PREV_ERASE_ERROR | FSR_LLD_1STPLN_CURR_ERROR);
            }
            break;

        default:
            WAIT_FND_INT_STAT(pstFOReg, FSR_FND_INT_MASTER_READY);
            break;
        }

        if (!(nFlag & FSR_LLD_FLAG_REMAIN_PREOP_STAT))
        {
            pstFNDShMem->nPreOp[nDie] = FSR_FND_PREOP_NONE;
        }

#if defined (FSR_LLD_STATISTICS)
        _AddFNDStat(nDev, nDie, FSR_FND_STAT_FLUSH, 0, FSR_FND_STAT_NORMAL_CMD);
#endif

    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[FND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return (nLLDRe);
}

/**
 * @brief           This function provides block information.
 *
 * @param[in]       nDev        : Physical Device Number
 * @param[in]       nPbn        : Physical Block  Number
 * @param[out]      pBlockType  : whether nPbn is SLC block or MLC block
 * @param[out]      pPgsPerBlk  : the number of pages per block
 *
 * @return          FSR_LLD_SUCCESS
 * @n               FSR_LLD_INVALID_PARAM
 *
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark          i.e. SLC or MLC block, the number of pages per block
 *
 */
PUBLIC INT32
FSR_FND_GetBlockInfo(UINT32     nDev,
                     UINT32     nPbn,
                     UINT32    *pBlockType,
                     UINT32    *pPgsPerBlk)
{
    FlexONDCxt       *pstFNDCxt;
    FlexONDSpec      *pstFNDSpec;
    UINT32            nDie;
    INT32             nLLDRe     = FSR_LLD_SUCCESS;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[FND:IN ] ++%s(nDev:%d, nPbn:%d)\r\n"),
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

        pstFNDCxt  = gpstFNDCxt[nDev];
        pstFNDSpec = pstFNDCxt->pstFNDSpec;

        nDie = nPbn >> (FSR_FND_DFS_BASEBIT - pstFNDCxt->nDDPSelSft);

        if ((nPbn & pstFNDCxt->nFBAMask) < pstFNDCxt->nBlksForSLCArea[nDie])
        {
            /* This block is SLC block */
            if (pBlockType != NULL)
            {
                *pBlockType = FSR_LLD_SLC_BLOCK;
            }
            if (pPgsPerBlk != NULL)
            {
                *pPgsPerBlk = pstFNDSpec->nPgsPerBlkForSLC;
            }
        }
        else
        {
            /* This block is MLC block */
            if (pBlockType != NULL)
            {
                *pBlockType = FSR_LLD_MLC_BLOCK;
            }
            if (pPgsPerBlk != NULL)
            {
                *pPgsPerBlk = pstFNDSpec->nPgsPerBlkForMLC;
            }
        }

    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[FND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return nLLDRe;
}



/**
 * @brief           This function reports device information to upper layer.
 *
 * @param[in]       nDev        : Physical Device Number (0 ~ 3)
 * @param[out]      pstDevSpec  : pointer to the device spec
 * @param[in]       nFlag       :
 *
 * @return          FSR_LLD_SUCCESS
 * @return          FSR_LLD_OPEN_FAILURE
 *
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark
 *
 */
PUBLIC INT32
FSR_FND_GetDevSpec(UINT32      nDev,
                   FSRDevSpec *pstDevSpec,
                   UINT32      nFlag)
{
                FlexONDCxt     *pstFNDCxt;
                FlexONDSpec    *pstFNDSpec;
       volatile FlexOneNANDReg *pstFOReg;
                UINT32          nDieIdx;
                UINT32          nDie        = 0;
                UINT32          nTINYFlag   = FSR_LLD_FLAG_NONE;    /* LLD flag for TINY FSR    */
                INT32           nLLDRe      = FSR_LLD_SUCCESS;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[FND:IN ] ++%s(nDev:%d,pstDevSpec:0x%x,nFlag:0x%08x)\r\n"), 
        __FSR_FUNC__, nDev, pstDevSpec, nFlag));

    do 
    {

#if defined (FSR_LLD_STRICT_CHK)
        /* Check device number */
        if (nDev >= FSR_FND_MAX_DEVS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[FND:ERR]   %s(nDev:%d, nFlag:0x%08x) / %d line\r\n"),
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
                (TEXT("[FND:ERR]   %s(nDev:%d, pstDevSpec:0x%08x, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, pstDevSpec, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            nDev:%d pstDevSpec is NULL\r\n"), nDev));

            nLLDRe = FSR_LLD_OPEN_FAILURE;
            break;
        }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */

        pstFNDCxt  = gpstFNDCxt[nDev];
        pstFNDSpec = pstFNDCxt->pstFNDSpec;

        FSR_OAM_MEMSET(pstDevSpec, 0xFF, sizeof(FSRDevSpec));

        pstDevSpec->nNumOfBlks          = pstFNDSpec->nNumOfBlks;
        pstDevSpec->nNumOfPlanes        = pstFNDSpec->nNumOfPlanes;

        for (nDieIdx = 0; nDieIdx < pstFNDSpec->nNumOfDies; nDieIdx++)
        {
            pstDevSpec->nBlksForSLCArea[nDieIdx] =
                pstFNDCxt->nBlksForSLCArea[nDieIdx];
        }

        pstDevSpec->nSparePerSct        = pstFNDSpec->nSparePerSct;
        pstDevSpec->nSctsPerPG          = pstFNDSpec->nSctsPerPG;
        pstDevSpec->nNumOfBlksIn1stDie  =
            pstFNDSpec->nNumOfBlks / pstFNDSpec->nNumOfDies;

        /* 
         * Read DID from register file
         * because DID from pstFNDSpec->nDID is masked with FSR_FND_DID_MASK
         */
        pstFOReg = (volatile FlexOneNANDReg *) pstFNDCxt->nBaseAddr;
        pstDevSpec->nDID                = FND_READ(pstFOReg->nDID);

        pstDevSpec->nPgsPerBlkForSLC    = pstFNDSpec->nPgsPerBlkForSLC;
        pstDevSpec->nPgsPerBlkForMLC    = pstFNDSpec->nPgsPerBlkForMLC;
        pstDevSpec->nNumOfDies          = pstFNDSpec->nNumOfDies;
        pstDevSpec->nUserOTPScts        = pstFNDSpec->nUserOTPScts;
        pstDevSpec->b1stBlkOTP          = pstFNDSpec->b1stBlkOTP;
        pstDevSpec->nRsvBlksInDev       = pstFNDSpec->nRsvBlksInDev;
        pstDevSpec->pPairedPgMap        = pstFNDSpec->pPairedPgMap;
        pstDevSpec->pLSBPgMap           = pstFNDSpec->pLSBPgMap;
        pstDevSpec->nNANDType           = FSR_LLD_FLEX_ONENAND;
        pstDevSpec->nPgBufToDataRAMTime = FSR_FND_PAGEBUF_TO_DATARAM_TIME;
        pstDevSpec->bCachePgm           = pstFNDCxt->bCachePgm;
        pstDevSpec->nSLCTLoadTime       = pstFNDSpec->nSLCTLoadTime;
        pstDevSpec->nMLCTLoadTime       = pstFNDSpec->nMLCTLoadTime;
        pstDevSpec->nSLCTProgTime       = pstFNDSpec->nSLCTProgTime;
        pstDevSpec->nMLCTProgTime[0]    = pstFNDSpec->nMLCTProgTime[0];
        pstDevSpec->nMLCTProgTime[1]    = pstFNDSpec->nMLCTProgTime[1];
        pstDevSpec->nTEraseTime         = pstFNDSpec->nTEraseTime;

        /* Time for transfering 1 page in usec */
        pstDevSpec->nWrTranferTime      = (pstFNDSpec->nSctsPerPG * 
                                           (FSR_FND_SECTOR_SIZE + pstFNDSpec->nSparePerSct)) *
                                          pstFNDCxt->nWrTranferTime / 2 / 1000;

        pstDevSpec->nRdTranferTime      = (pstFNDSpec->nSctsPerPG * 
                                           (FSR_FND_SECTOR_SIZE + pstFNDSpec->nSparePerSct)) *
                                          pstFNDCxt->nRdTranferTime / 2 / 1000;
        pstDevSpec->nSLCPECycle         = pstFNDSpec->nSLCPECycle;
        pstDevSpec->nMLCPECycle         = pstFNDSpec->nMLCPECycle;

        if (pstFNDSpec->nNumOfDies == FSR_MAX_DIES)
        {
               FSR_FND_FlushOp(nDev,
                          ((nDie + 0x1) & 0x01),
                          FSR_LLD_FLAG_NONE | nTINYFlag);
        }

        nLLDRe = FSR_FND_FlushOp(nDev, nDie, FSR_LLD_FLAG_NONE | nTINYFlag);

        /* Get UID from OTP block */
        _GetUniqueID(nDev, pstFNDCxt, nFlag);

        FSR_OAM_MEMCPY(&pstDevSpec->nUID[0], &pstFNDCxt->nUID[0], FSR_LLD_UID_SIZE);

    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[FND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return (nLLDRe);
}



/**
 * @brief           This function provides access information 
 *
 * @param[in]       nDev        : Physical Device Number
 * @param[out]      pLLDPltInfo : structure for platform information.
 *
 * @return          FSR_LLD_SUCCESS
 * @n               FSR_LLD_INVALID_PARAM
 * @n               FSR_LLD_OPEN_FAILURE
 *
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark
 *
 */
PUBLIC INT32
FSR_FND_GetPlatformInfo(UINT32           nDev,
                        LLDPlatformInfo *pLLDPltInfo)
{
            FlexONDCxt     *pstFNDCxt;
   volatile FlexOneNANDReg *pstFOReg;
            INT32           nLLDRe    = FSR_LLD_SUCCESS;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[FND:IN ] ++%s(nDev:%d,pLLDPltInfo:0x%x)\r\n"), __FSR_FUNC__, nDev, pLLDPltInfo));

    do
    {
        if (pLLDPltInfo == NULL)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[FND:ERR]   %s() / %d line\r\n"),
                __FSR_FUNC__, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            pLLDPltInfo is NULL\r\n")));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }

#if defined (FSR_LLD_STRICT_CHK)
        /* Check Device Number */
        if (nDev >= FSR_FND_MAX_DEVS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[FND:ERR]   %s() / %d line \r\n"),
                __FSR_FUNC__, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Invalid Device Number (nDev = %d)\r\n"),
                nDev));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }
#endif

        pstFNDCxt = gpstFNDCxt[nDev];
        pstFOReg  = (volatile FlexOneNANDReg *) pstFNDCxt->nBaseAddr;

#if defined (FSR_LLD_STRICT_CHK)
        /* Check Device Open Flag */
        if (pstFNDCxt->bOpen == FALSE32)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[FND:ERR]   %s() / %d line\r\n"),
                __FSR_FUNC__, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Device is not opened\r\n")));

            nLLDRe = FSR_LLD_OPEN_FAILURE;
            break;
        }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */

        FSR_OAM_MEMSET(pLLDPltInfo, 0x00, sizeof(LLDPlatformInfo));

        /* Type of Device : FlexOneNAND = 0     */
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
        (TEXT("[FND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return nLLDRe;
}



/**
 * @brief           This function reads data from DataRAM of Flex-OneNAND
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
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark          this function does not loads data from Flex-OneNAND
 * @n               it just reads data which lies on DataRAM
 *
 */
PUBLIC INT32
FSR_FND_GetPrevOpData(UINT32       nDev,
                      UINT8       *pMBuf,
                      FSRSpareBuf *pSBuf,
                      UINT32       nDie,
                      UINT32       nFlag)
{
             FlexONDCxt       *pstFNDCxt;
             FlexONDSpec      *pstFNDSpec;
    volatile FlexOneNANDReg   *pstFOReg;
             INT32    nLLDRe = FSR_LLD_SUCCESS;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[FND:IN ] ++%s() / nDie : %d / nFlag : 0x%x\r\n"), __FSR_FUNC__, nDie, nFlag));

    do
    {

#if defined (FSR_LLD_STRICT_CHK)
        /* Check device number */
        if (nDev >= FSR_FND_MAX_DEVS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[FND:ERR]   %s(nDev:%d, nFlag:0x%08x) / %d line\r\n"),
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
                (TEXT("[FND:ERR]   %s(nDev:%d, nDie:%d, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, nDie, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Invalid nDie Number (nDie = %d)\r\n"),
                nDie));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }
#endif

        pstFNDCxt  = gpstFNDCxt[nDev];
        pstFNDSpec = pstFNDCxt->pstFNDSpec;
        pstFOReg   = (volatile FlexOneNANDReg *) pstFNDCxt->nBaseAddr;

        FND_WRITE(pstFOReg->nStartAddr2, (UINT16) (nDie << FSR_FND_DBS_BASEBIT));

        _ReadMain (pMBuf,
                   (volatile UINT8 *) &pstFOReg->nDataMB00[0],
                   pstFNDSpec->nSctsPerPG * FSR_FND_SECTOR_SIZE,
                   pstFNDCxt->pTempBuffer);

        _ReadSpare(pSBuf,
                   (volatile UINT8 *) &pstFOReg->nDataSB00[0]);
    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[FND:OUT] --%s()\r\n"), __FSR_FUNC__));

    return nLLDRe;
}



/**
 * @brief           This function does PI operation, OTP operation,
 * @n               reset, write protection. 
 *
 * @param[in]       nDev        : Physical Device Number (0 ~ 3)
 * @param[in]       nCode       : IO Control Command
 * @n                             FSR_LLD_IOCTL_PI_ACCESS
 * @n                             FSR_LLD_IOCTL_PI_READ
 * @n                             FSR_LLD_IOCTL_PI_WRITE
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
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark          OTP read, write is performed with FSR_FND_Write(), FSR_FND_ReadOptimal(),
 * @n               after OTP Access
 *
 */
PUBLIC INT32
FSR_FND_IOCtl(UINT32  nDev,
              UINT32  nCode,
              UINT8  *pBufI,
              UINT32  nLenI,
              UINT8  *pBufO,
              UINT32  nLenO,
              UINT32 *pByteRet)
{
             /* First word of PI block contains partition information */
             LLDPIArg         *pLLDPIArg; /* PI Write Argument        */

              /* Used to lock, unlock, lock-tight */
             LLDProtectionArg *pLLDProtectionArg;

             FlexONDCxt       *pstFNDCxt;
             FlexONDSpec      *pstFNDSpec;
             FlexONDShMem     *pstFNDShMem;
    volatile FlexOneNANDReg   *pstFOReg;
    volatile UINT32            nLockType;
             UINT32            nDie         = 0xFFFFFFFF;
             UINT32            nEndOfSLC;
             UINT32            nPILockValue;
             UINT32            nPbn;
             UINT32            nErrorPbn    = 0;
             UINT32            nRSVofPI;
             INT32             nLLDRe       = FSR_LLD_SUCCESS;
             BOOL32            bPILocked;
             UINT32            nLockValue;
             UINT16            nLockState;
             UINT16            nWrProtectStat;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[FND:IN ] ++%s(nDev:%d, nCode:0x%08x)\r\n"),
        __FSR_FUNC__, nDev, nCode));

    do
    {

#if defined (FSR_LLD_STRICT_CHK)
        /* Check device number */
        if (nDev >= FSR_FND_MAX_DEVS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("[FND:ERR]   %s() / %d line\r\n"),
            __FSR_FUNC__, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("            Invalid Device Number (nDev = %d)\r\n"), nDev));

            nLLDRe = (FSR_LLD_INVALID_PARAM);
            break;
        }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */

        pstFNDCxt  = gpstFNDCxt[nDev];
        pstFNDShMem= gpstFNDShMem[nDev];
        pstFNDSpec = pstFNDCxt->pstFNDSpec;
        pstFOReg   = (volatile FlexOneNANDReg *) pstFNDCxt->nBaseAddr;

        /* Interrupt should be enabled in I/O Ctl */
        FSR_OAM_ClrNDisableInt(pstFNDCxt->nIntID);

        switch (nCode)
        {
        case FSR_LLD_IOCTL_PI_ACCESS:
            if ((pBufI == NULL) || (nLenI != sizeof(UINT32)))
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[FND:ERR]   %s() / %d line\r\n"),
                    __FSR_FUNC__, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            invalid parameter pBufI = 0x%08x, nLenI = %d\r\n"), pBufI, nLenI));

                nLLDRe = FSR_LLD_INVALID_PARAM;
                break;
            }

            nDie = *(UINT32 *) pBufI;

            FSR_ASSERT((nDie & ~0x1) == 0);
          
            FND_WRITE(pstFOReg->nStartAddr2,
                (UINT16) (nDie << FSR_FND_DBS_BASEBIT));

            FND_WRITE(pstFOReg->nStartAddr1,
                (UINT16) (nDie << FSR_FND_DFS_BASEBIT));

            /* Issue PI Access command */
            FND_WRITE(pstFOReg->nCmd, FSR_FND_CMD_ACCESS_PI);

            WAIT_FND_INT_STAT(pstFOReg, FSR_FND_INT_MASTER_READY);

            if (pByteRet != NULL)
            {
                *pByteRet = (UINT32) 0;
            }            
            break;

        case FSR_LLD_IOCTL_PI_READ:
            if ((pBufI == NULL) || (nLenI != sizeof(UINT32)) ||
                (pBufO == NULL) || (nLenO != sizeof(LLDPIArg)))
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[FND:ERR]   %s() / %d line\r\n"),
                    __FSR_FUNC__, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            invalid parameter\r\n")));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            pBufI = 0x%08x, nLenI = %d, pBufO = 0x%08x, nLenO = %d\r\n"),
                    pBufI, nLenI, pBufO, nLenO));

                nLLDRe = FSR_LLD_INVALID_PARAM;
                break;
            }

            nDie = *(UINT32 *) pBufI;

            FSR_ASSERT((nDie & ~0x1) == 0);

            nLLDRe = _ReadPI(nDev, nDie, &nEndOfSLC, &bPILocked);

            pLLDPIArg = (LLDPIArg *) pBufO;


            pLLDPIArg->nEndOfSLC    =
                (UINT16) (nDie * (pstFNDSpec->nNumOfBlks / pstFNDSpec->nNumOfDies) + nEndOfSLC);

            pLLDPIArg->nPad         = 0;

            pLLDPIArg->nPILockValue =
                bPILocked ? FSR_LLD_IOCTL_LOCK_PI : FSR_LLD_IOCTL_UNLOCK_PI;

            if (pByteRet != NULL)
            {
                *pByteRet     = sizeof(LLDPIArg);
            }
            break;

        case FSR_LLD_IOCTL_PI_WRITE:

            if ((pBufI == NULL) || (nLenI != sizeof(LLDPIArg)))
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[FND:ERR]   %s() / %d line\r\n"),
                    __FSR_FUNC__, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            invalid parameter pBufI = 0x%08x, nLenI = %d\r\n"), pBufI, nLenI));

                nLLDRe = FSR_LLD_INVALID_PARAM;
                break;
            }

            pLLDPIArg = (LLDPIArg *) pBufI;

            nEndOfSLC = (pLLDPIArg->nEndOfSLC & pstFNDCxt->nFBAMask);
            nRSVofPI  = FSR_FND_PI_RSV_MASK ^ pstFNDCxt->nFBAMask;
            nDie      =  pLLDPIArg->nEndOfSLC >> (FSR_FND_DFS_BASEBIT - pstFNDCxt->nDDPSelSft);

            FSR_ASSERT((nDie & ~0x1) == 0);

            nPILockValue = pLLDPIArg->nPILockValue;

            if (nPILockValue == FSR_LLD_IOCTL_LOCK_PI)
            {
                nLLDRe = _WritePI(nDev, nDie, (FSR_FND_PI_LOCK & nRSVofPI) | nEndOfSLC);
            }
            else if (nPILockValue == FSR_LLD_IOCTL_UNLOCK_PI)
            {
                nLLDRe = _WritePI(nDev, nDie, (FSR_FND_PI_NOLOCK & nRSVofPI) | nEndOfSLC);
            }
            else
            {
                nLLDRe = FSR_LLD_INVALID_PARAM;
            }

            if (pByteRet != NULL)
            {
                *pByteRet = (UINT32) 0;
            }
            break;

        case FSR_LLD_IOCTL_OTP_ACCESS:
            if ((pBufI == NULL) || (nLenI != sizeof(UINT32)))
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[FND:ERR]   %s() / %d line\r\n"),
                    __FSR_FUNC__, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            invalid parameter pBufI = 0x%08x, nLenI = %d\r\n"), pBufI, nLenI));

                nLLDRe = FSR_LLD_INVALID_PARAM;
                break;
            }

            nDie = *(UINT32 *) pBufI;

            FSR_ASSERT((nDie & ~0x1) == 0);

            FND_WRITE(pstFOReg->nStartAddr2,
                (UINT16) (nDie << FSR_FND_DBS_BASEBIT));

            FND_WRITE(pstFOReg->nStartAddr1,
                (UINT16) (nDie << FSR_FND_DFS_BASEBIT));

            /* Issue OTP Access command */
            FND_WRITE(pstFOReg->nCmd, FSR_FND_CMD_OTP_ACCESS);

            WAIT_FND_INT_STAT(pstFOReg, FSR_FND_INT_MASTER_READY);

            if (pByteRet != NULL)
            {
                *pByteRet = (UINT32) 0;
            }
            break;

        case FSR_LLD_IOCTL_OTP_LOCK:
            if ((pBufI == NULL) || (nLenI != sizeof(UINT32)))
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[FND:ERR]   %s() / %d line\r\n"),
                    __FSR_FUNC__, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            invalid parameter pBufI = 0x%08x, nLenI = %d\r\n"), pBufI, nLenI));

                nLLDRe = FSR_LLD_INVALID_PARAM;
                break;
            }

            nLockType = *(UINT32 *) pBufI;

            /* Lock 1st block OTP & OTP block */
            if (nLockType == (FSR_LLD_OTP_LOCK_1ST_BLK | FSR_LLD_OTP_LOCK_OTP_BLK))
            {
                nLockValue = FSR_FND_LOCK_BOTH_OTP;
            }
            /* Lock 1st block OTP only */
            else if (nLockType == FSR_LLD_OTP_LOCK_1ST_BLK)
            {
                nLockValue = FSR_FND_LOCK_1ST_BLOCK_OTP;
            }
            /* Lock OTP block only */
            else if (nLockType == FSR_LLD_OTP_LOCK_OTP_BLK)
            {
                nLockValue = FSR_FND_LOCK_OTP_BLOCK;
            }
            else
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[FND:ERR]   %s() / %d line\r\n"),
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
                    (TEXT("[FND:ERR]   %s() / %d line\r\n"),
                    __FSR_FUNC__, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            invalid parameter pBufO = 0x%08x, nLenO = %d\r\n"), pBufO, nLenO));

                nLLDRe = FSR_LLD_INVALID_PARAM;
                break;
            }

            nDie = 0;

            FND_WRITE(pstFOReg->nStartAddr2,
                (UINT16) (nDie << FSR_FND_DBS_BASEBIT));

            nLockType  = 0;

            nLockState = FND_READ(pstFOReg->nCtrlStat);

            if (nLockState & FSR_FND_CTLSTAT_1ST_OTP_LOCKED)
            {
                nLockType |= FSR_LLD_OTP_1ST_BLK_LOCKED;
            }
            else
            {
                nLockType |= FSR_LLD_OTP_1ST_BLK_UNLKED;
            }

            if (nLockState & FSR_FND_CTLSTAT_OTP_BLK_LOCKED)
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
                    (TEXT("[FND:ERR]   %s() / %d line\r\n"),
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
                (FSR_FND_DFS_BASEBIT - pstFNDCxt->nDDPSelSft);

            FSR_ASSERT((nDie & ~0x1) == 0);

            nLLDRe = _ControlLockBlk(nDev,
                                     pLLDProtectionArg->nStartBlk,
                                     pLLDProtectionArg->nBlks,
                                     (UINT32)FSR_FND_CMD_LOCKTIGHT_BLOCK,
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
                    (TEXT("[FND:ERR]   %s() / %d line\r\n"),
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
                (FSR_FND_DFS_BASEBIT - pstFNDCxt->nDDPSelSft);

            FSR_ASSERT((nDie & ~0x1) == 0);

            nLLDRe = _ControlLockBlk(nDev,
                                     pLLDProtectionArg->nStartBlk,
                                     pLLDProtectionArg->nBlks,
                                     FSR_FND_CMD_LOCK_BLOCK,
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
                    (TEXT("[FND:ERR]   %s() / %d line\r\n"),
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
                (FSR_FND_DFS_BASEBIT - pstFNDCxt->nDDPSelSft);

            FSR_ASSERT((nDie & ~0x1) == 0);

            nLLDRe = _ControlLockBlk(nDev,
                                     pLLDProtectionArg->nStartBlk,
                                     pLLDProtectionArg->nBlks,
                                     FSR_FND_CMD_UNLOCK_BLOCK,
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
                    (TEXT("[FND:ERR]   %s() / %d line\r\n"),
                    __FSR_FUNC__, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            invalid parameter pBufI = 0x%08x, nLenI = %d\r\n"), pBufI, nLenI));

                nLLDRe = FSR_LLD_INVALID_PARAM;
                break;
            }

            nPbn = *(UINT32 *) pBufI;

            nDie = nPbn >> (FSR_FND_DFS_BASEBIT - pstFNDCxt->nDDPSelSft);

            FSR_ASSERT((nDie & ~0x1) == 0);

            FND_WRITE(pstFOReg->nStartAddr2,
                (UINT16) ((nPbn << pstFNDCxt->nDDPSelSft) & FSR_FND_DBS_MASK));

            FND_WRITE(pstFOReg->nStartAddr1,
                (UINT16) ((nPbn << pstFNDCxt->nDDPSelSft) & FSR_FND_DFS_MASK));

            FND_WRITE(pstFOReg->nStartBlkAddr,
                (UINT16) (nPbn & pstFNDCxt->nFBAMask));

            FND_WRITE(pstFOReg->nCmd, FSR_FND_CMD_UNLOCK_ALLBLOCK);

            WAIT_FND_INT_STAT(pstFOReg, FSR_FND_INT_MASTER_READY);

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
                    (TEXT("[FND:ERR]   %s() / %d line\r\n"),
                    __FSR_FUNC__, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            invalid parameter pBufO = 0x%08x, nLenO = %d\r\n"), pBufO, nLenO));

                nLLDRe = FSR_LLD_INVALID_PARAM;
                break;
            }

            nPbn    = *(UINT32 *) pBufI;

            nDie = nPbn >> (FSR_FND_DFS_BASEBIT - pstFNDCxt->nDDPSelSft);

            FSR_ASSERT((nDie & ~0x1) == 0);

            /* Set DBS       */
            FND_WRITE(pstFOReg->nStartAddr2,
                (UINT16) ((nPbn << pstFNDCxt->nDDPSelSft) & FSR_FND_DBS_MASK)) ;

            /* Set DFS & FBA */
            FND_WRITE(pstFOReg->nStartAddr1,
                (UINT16) (((nPbn << pstFNDCxt->nDDPSelSft) & FSR_FND_DFS_MASK) | (nPbn & pstFNDCxt->nFBAMask)));

#if defined(FSR_LLD_WAIT_WR_PROTECT_STAT)
            do
            {
                nWrProtectStat = FND_READ(pstFOReg->nWrProtectStat);
            } while (nWrProtectStat == (UINT16) 0x0000);
#else
            nWrProtectStat = FND_READ(pstFOReg->nWrProtectStat);
#endif

            FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_INF,
                (TEXT("[FND:INF]   nDev: %d, nPbn: %d has lock status 0x%04x\r\n"),
                nDev, nPbn, nWrProtectStat));

            *(UINT32 *) pBufO = (UINT32) nWrProtectStat;

            if (pByteRet != NULL)
            {
                *pByteRet = (UINT32) sizeof(UINT32);
            }

            nLLDRe    = FSR_LLD_SUCCESS;
            break;

        case FSR_LLD_IOCTL_HOT_RESET:
#if 0
            FND_WRITE(pstFOReg->nCmd, FSR_FND_CMD_HOT_RESET);

            WAIT_FND_INT_STAT(pstFOReg, FSR_FND_INT_RESET_READY);

            /* Hot reset initializes the nSysConf1 register */
            FND_WRITE(pstFOReg->nSysConf1, pstFNDCxt->nSysConf1);
#endif
            if (pByteRet != NULL)
            {
                *pByteRet = (UINT32) 0;
            }
            break;

        case FSR_LLD_IOCTL_CORE_RESET:

            FND_WRITE(pstFOReg->nCmd, FSR_FND_CMD_RST_NFCORE);

            WAIT_FND_INT_STAT(pstFOReg, FSR_FND_INT_RESET_READY);

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
            /* Initialize shared memory */
            for (nDie = 0; nDie < pstFNDSpec->nNumOfDies; nDie++)
            {
                pstFNDShMem->nPreOp[nDie]         = FSR_FND_PREOP_IOCTL;
                pstFNDShMem->nPreOpPbn[nDie]      = FSR_FND_PREOP_ADDRESS_NONE;
                pstFNDShMem->nPreOpPgOffset[nDie] = FSR_FND_PREOP_ADDRESS_NONE;
                pstFNDShMem->nPreOpFlag[nDie]     = FSR_FND_PREOP_FLAG_NONE;

#if defined (FSR_LLD_READ_PRELOADED_DATA)
                pstFNDShMem->bIsPreCmdLoad[nDie] = FALSE32;
#endif

            }
        }
        else if ((nLLDRe != FSR_LLD_INVALID_PARAM) && (nLLDRe != FSR_LLD_IOCTL_NOT_SUPPORT))
        {
            FSR_ASSERT(nDie != 0xFFFFFFFF);

            /* Initialize shared memory */
            pstFNDShMem->nPreOp[nDie]         = FSR_FND_PREOP_IOCTL;
            pstFNDShMem->nPreOpPbn[nDie]      = FSR_FND_PREOP_ADDRESS_NONE;
            pstFNDShMem->nPreOpPgOffset[nDie] = FSR_FND_PREOP_ADDRESS_NONE;
            pstFNDShMem->nPreOpFlag[nDie]     = FSR_FND_PREOP_FLAG_NONE;

#if defined (FSR_LLD_READ_PRELOADED_DATA)
            pstFNDShMem->bIsPreCmdLoad[nDie] = FALSE32;
#endif

        }
    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[FND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return (nLLDRe);
}



/**
 * @brief           This function Erase, Program & Updates PI allocation info.
 *
 * @param[in]       nDev        : Physical Device Number (0 ~ 3)
 * @param[in]       nDie        : die(chip) number (0 or 1) for DDP device
 * @param[out]     *pnEndOfSLC  : end block # of SLC area
 * @param[out]     *pbPILocked  : whether PI block is locked or not.
 *
 * @return          FSR_LLD_SUCCESS
 * @return          FSR_LLD_NO_RESPONSE
 * @return          FSR_LLD_PI_READ_ERROR
 *
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark          EndOfSLC can be differnt by die(chip) for DDP device
 *
 */
PRIVATE INT32
_ReadPI(UINT32   nDev,
        UINT32   nDie,
        UINT32  *pnEndOfSLC,
        BOOL32  *pbPILocked)
{
             FlexONDCxt       *pstFNDCxt  = gpstFNDCxt[nDev];
    volatile FlexOneNANDReg   *pstFOReg   = (volatile FlexOneNANDReg *) pstFNDCxt->nBaseAddr;

             INT32             nLLDRe     = FSR_LLD_SUCCESS;
             UINT32            nIdx;
             UINT32            nRSVofPI;
             UINT16            nPIValue;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[FND:IN ] ++%s(nDev:%d, nDie:%d)\r\n"),
        __FSR_FUNC__, nDev, nDie));

    do
    {
        FSR_ASSERT((nDie & ~0x1) == 0)

        FSR_ASSERT(pnEndOfSLC != NULL);

        /* Wait for each die ready, play for safety */
        for (nIdx = 0; nIdx < pstFNDCxt->pstFNDSpec->nNumOfDies; nIdx++)
        {
            /* Select DataRAM for DDP */
            FND_WRITE(pstFOReg->nStartAddr2, (UINT16)(nIdx << FSR_FND_DBS_BASEBIT));

            /* Wait for INT register low to high transition */
            WAIT_FND_INT_STAT(pstFOReg, FSR_FND_INT_MASTER_READY);
        }

        /* Select DataRAM for DDP */
        FND_WRITE(pstFOReg->nStartAddr2, (UINT16)(nDie << FSR_FND_DBS_BASEBIT));

        /* Write 'DFS, FBA' of Flash (FBA could be omitted or any address) */
        FND_WRITE(pstFOReg->nStartAddr1, (UINT16)(nDie << FSR_FND_DFS_BASEBIT));

        /* Write 'PI Access' command */
        FND_WRITE(pstFOReg->nCmd, FSR_FND_CMD_ACCESS_PI);

        /* Wait for INT register low to high transition */
        WAIT_FND_INT_STAT(pstFOReg, FSR_FND_INT_MASTER_READY);

        /* Write 'FPA, FSA' of Flash */
        FND_WRITE(pstFOReg->nStartAddr8, 0x0 << pstFNDCxt->nFPASelSft);

        /* Write 'BSA, BSC' of DataRAM */
        FND_WRITE(pstFOReg->nStartBuf, FSR_FND_START_BUF_DEFAULT);

        /* ECC off */
        FND_SET(pstFOReg->nSysConf1, FSR_FND_CONF1_ECC_OFF);

        /* Write 'Load' Command */
        FND_WRITE(pstFOReg->nCmd, FSR_FND_CMD_LOAD);

        /* Wait for INT register low to high transition */
        WAIT_FND_INT_STAT(pstFOReg, FSR_FND_INT_READ_READY);

        /* Can not check ECC status becuase of ECC off */

        /* Host reads data from DataRAM */
        nPIValue = FND_READ(*(UINT16 *) &pstFOReg->nDataMB00[0]);

        *pnEndOfSLC = (UINT32) (nPIValue & pstFNDCxt->nFBAMask);
        nRSVofPI = FSR_FND_PI_RSV_MASK ^ pstFNDCxt->nFBAMask;

        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
            (TEXT("[FND:INF]   %s(nDev:%d, nDie:%d, *pnEndOfSLC:%d) / %d line\r\n"),
            __FSR_FUNC__, nDev, nDie, *pnEndOfSLC, __LINE__));

        if (pbPILocked != NULL)
        {
            if ((nPIValue & FSR_FND_PI_LOCK_MASK) == (UINT16)(nRSVofPI & FSR_FND_PI_LOCK & FSR_FND_PI_LOCK_MASK))
            {
                *pbPILocked = TRUE32;
            }
            else if ((nPIValue & FSR_FND_PI_LOCK_MASK) == (UINT16)(FSR_FND_PI_NOLOCK & nRSVofPI & FSR_FND_PI_LOCK_MASK))
            {
                *pbPILocked = FALSE32;
            }
            else
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[FND:ERR]   nDie: %d, undefined PI value: 0x%04x / %d line\r\n"),
                    nDie, nPIValue, __LINE__));
            }

            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
                (TEXT("[FND:INF]   %s(nDev:%d, nDie:%d, *pbPILocked:%d) / %d line\r\n"),
                __FSR_FUNC__, nDev, nDie, *pbPILocked, __LINE__));
        }
    } while (0);

    /* Do NAND Core reset */
    FND_WRITE(pstFOReg->nCmd, FSR_FND_CMD_RST_NFCORE);

    WAIT_FND_INT_STAT(pstFOReg, FSR_FND_INT_RESET_READY);

#if defined (FSR_LLD_READ_PRELOADED_DATA)
    gpstFNDShMem[nDev]->bIsPreCmdLoad[nDie] = FALSE32;
#endif

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[FND:OUT] --%s()\r\n"), __FSR_FUNC__));

    return (nLLDRe);
}

/**
 * @brief           This function Erase, Program & Updates PI allocation info.
 *
 * @param[in]       nDev        : Physical Device Number (0 ~ 3)
 * @param[in]       nDie        : die(chip) number (0 or 1) for DDP device
 * @param[in]       nPIValue    : this value is written to PI block
 * @n                             this value does not equals to the end block # of SLC area
 *
 * @return          FSR_LLD_SUCCESS
 * @return          FSR_LLD_PI_ERASE_ERROR
 * @return          FSR_LLD_PI_PROGRAM_ERROR
 *
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark
 *
 */
PRIVATE INT32
_WritePI(UINT32  nDev,
         UINT32  nDie,
         UINT32  nPIValue)
{
             FlexONDCxt       *pstFNDCxt  = gpstFNDCxt[nDev];
             FlexONDSpec      *pstFNDSpec = pstFNDCxt->pstFNDSpec;
    volatile FlexOneNANDReg   *pstFOReg   = (volatile FlexOneNANDReg *) pstFNDCxt->nBaseAddr;

             INT32             nLLDRe     = FSR_LLD_SUCCESS;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[FND:IN ] ++%s(nDev:%d, nDie:%d, nPIValue:0x%04x)\r\n"),
        __FSR_FUNC__, nDev, nDie, nPIValue));

    do
    {
        /*********************************************************************/
        /*                Step 1: PI Block Access mode                       */
        /*********************************************************************/

        /* Select DataRAM for DDP */
        FND_WRITE(pstFOReg->nStartAddr2, (UINT16) (nDie << FSR_FND_DBS_BASEBIT));

        /* Write 'DFS, FBA' of Flash (FBA could be omitted or any address) */
        FND_WRITE(pstFOReg->nStartAddr1, (UINT16) (nDie << FSR_FND_DFS_BASEBIT));

        /* Write 'PI Access' Command */
        FND_WRITE(pstFOReg->nCmd, FSR_FND_CMD_ACCESS_PI);

        /* Wait for INT register low to high transition */
        WAIT_FND_INT_STAT(pstFOReg, FSR_FND_INT_MASTER_READY);

        /*********************************************************************/
        /*                Step 2: PI Block Erase                             */
        /*********************************************************************/
        /* Write 'FBA' of Flash (FBA must be 0x0000h) */
        FND_WRITE(pstFOReg->nStartAddr1, (UINT16) (nDie << FSR_FND_DFS_BASEBIT));

        /* Write Erase command */
        FND_WRITE(pstFOReg->nCmd, FSR_FND_CMD_ERASE);

        /* Wait for INT register low to high transition */
        WAIT_FND_INT_STAT(pstFOReg, FSR_FND_INT_ERASE_READY);

        if ((FND_READ(pstFOReg->nCtrlStat) & FSR_FND_STATUS_ERROR) ==
            FSR_FND_STATUS_ERROR)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[FND:ERR]   %s(nDev:%d, nDie:%d, nPIValue:0x%04x) / %d line\r\n"),
                __FSR_FUNC__, nDev, nDie, nPIValue, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            PI Erase Error, Ctrl Reg:0x%04x\r\n"),
                FND_READ(pstFOReg->nCtrlStat)));

            _DumpRegisters(pstFOReg);
            _DumpSpareBuffer(pstFOReg);

            /* if PI block is locked, error bit of ctrl status reg. is set */
            nLLDRe = FSR_LLD_PI_ERASE_ERROR;
            break;
        }

        /*********************************************************************/
        /*                Step 3: PI Program                                 */
        /*********************************************************************/
        /* Write Data into DataRAM */
        FSR_OAM_MEMSET(pstFNDCxt->pTempBuffer, 0xFF, FSR_SECTOR_SIZE * pstFNDSpec->nSctsPerPG);
        
        TRANSFER_TO_NAND(pstFOReg->nDataMB00,
                         pstFNDCxt->pTempBuffer,
                         FSR_SECTOR_SIZE * pstFNDSpec->nSctsPerPG);
                      
        TRANSFER_TO_NAND(pstFOReg->nDataSB00,
                         pstFNDCxt->pTempBuffer,
                         FSR_SPARE_SIZE * pstFNDSpec->nSctsPerPG);
                                          
        /* Write Partition Information */
        FND_WRITE(*(volatile UINT16 *) &pstFOReg->nDataMB00[0],
                                       (UINT16) nPIValue);

        /* Write 'DFS, FBA' of Flash (FBA must be 0x0000h) */
        FND_WRITE(pstFOReg->nStartAddr1, (UINT16) (nDie << FSR_FND_DFS_BASEBIT));

        /* Write 'FPA, FSA' of Flash (FPA must be 00h and FSA must be 00h) */
        FND_WRITE(pstFOReg->nStartAddr8, 0x0 << pstFNDCxt->nFPASelSft);

        /* Write 'BSA, BSC' of DataRAM (BSA must be 1000b and BSC must be 000b) */
        FND_WRITE(pstFOReg->nStartBuf, FSR_FND_START_BUF_DEFAULT);

        /* ECC off */
        FND_SET(pstFOReg->nSysConf1, FSR_FND_CONF1_ECC_OFF);

        /* Write Program Command */
        FND_WRITE(pstFOReg->nCmd, FSR_FND_CMD_PROGRAM);

        /* Wait for INT register low to high transition */
        WAIT_FND_INT_STAT(pstFOReg, FSR_FND_INT_WRITE_READY);

        /* Check the result of PI program */
        if ((FND_READ(pstFOReg->nCtrlStat) & FSR_FND_STATUS_ERROR) ==
            FSR_FND_STATUS_ERROR)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[FND:ERR]   %s(nDev:%d, nDie:%d, nPIValue:0x%04x) / %d line\r\n"),
                __FSR_FUNC__, nDev, nDie, nPIValue, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            PI Program Error, CtrlReg=0x%04x\r\n"),
                FND_READ(pstFOReg->nCtrlStat)));

            _DumpRegisters(pstFOReg);
            _DumpSpareBuffer(pstFOReg);

            nLLDRe = (FSR_LLD_PI_PROGRAM_ERROR);
            break;
        }

        pstFNDCxt->nBlksForSLCArea[nDie] =
            (pstFNDCxt->nFBAMask & (UINT16) nPIValue) + 1;

        /*********************************************************************/
        /*                Step 3: PI Block Update                            */
        /*********************************************************************/
        /* Write 'DFS, FBA' of Flash (FBA must be 0x0000h) */
        FND_WRITE(pstFOReg->nStartAddr1, (UINT16)(nDie << FSR_FND_DFS_BASEBIT));

        /* Write 'BSA, BSC' of DataRAM (BSA must be 1000b and BSC must be 000b) */
        FND_WRITE(pstFOReg->nStartBuf, FSR_FND_START_BUF_DEFAULT);

        /* Write 'FPA, FSA' of Flash (FPA must be 00h and FSA must be 00h) */
        FND_WRITE(pstFOReg->nStartAddr8, 0x0 << pstFNDCxt->nFPASelSft);

        /* Write Update PI command */
        FND_WRITE(pstFOReg->nCmd, FSR_FND_CMD_UPDATE_PI);

        /* Wait for INT register low to high transition */
        WAIT_FND_INT_STAT(pstFOReg, FSR_FND_INT_MASTER_READY);
    } while (0);

    /*********************************************************************/
    /*                Step 4: NAND Flash Reset                           */
    /*********************************************************************/
    FND_WRITE(pstFOReg->nCmd, FSR_FND_CMD_RST_NFCORE);

    WAIT_FND_INT_STAT(pstFOReg, FSR_FND_INT_RESET_READY);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[FND:OUT] --%s()\r\n"), __FSR_FUNC__));

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
             FlexONDCxt       *pstFNDCxt  = gpstFNDCxt[nDev];
    volatile FlexOneNANDReg   *pstFOReg   = (volatile FlexOneNANDReg *) pstFNDCxt->nBaseAddr;
             UINT32            nDie;
             UINT32            nFBA;
             INT32             nLLDRe     = FSR_LLD_SUCCESS;
             UINT16            nLockStat;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[FND:IN ] ++%s(nDev: %d, nPbn: %d, nBlks: %d)\r\n"),
        __FSR_FUNC__, nDev, nPbn, nBlks));

    while (nBlks > 0)
    {
        nDie = nPbn >> (FSR_FND_DFS_BASEBIT - pstFNDCxt->nDDPSelSft);
        nFBA = nPbn & pstFNDCxt->nFBAMask;

        /* Set DBS */
        FND_WRITE(pstFOReg->nStartAddr2, (UINT16) (nDie << FSR_FND_DBS_BASEBIT));

        /* Set DFS, FBA */
        FND_WRITE(pstFOReg->nStartAddr1,
            (UINT16) ((nDie << FSR_FND_DFS_BASEBIT) | nFBA));

#if defined(FSR_LLD_WAIT_WR_PROTECT_STAT)
        do
        {
            nLockStat = FND_READ(pstFOReg->nWrProtectStat);
        } while (nLockStat == (UINT16) 0x0000);
#else
        nLockStat = FND_READ(pstFOReg->nWrProtectStat);
#endif

        nLockStat = nLockStat & FSR_LLD_BLK_STAT_MASK;

        switch (nLockTypeCMD)
        {
        case FSR_FND_CMD_LOCKTIGHT_BLOCK:
            /* To lock blocks tight, the block should be locked first */
            if (nLockStat == FSR_LLD_BLK_STAT_UNLOCKED)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[FND:ERR]   %s() / %d line\r\n"),
                    __FSR_FUNC__, __LINE__));
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            unlocked blk cannot be locked tight\r\n")));
                nLLDRe = FSR_LLD_INVALID_BLOCK_STATE;
            }
            break;
        case FSR_FND_CMD_LOCK_BLOCK:
        case FSR_FND_CMD_UNLOCK_BLOCK:
            if (nLockStat == FSR_LLD_BLK_STAT_LOCKED_TIGHT)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[FND:ERR]   %s() / %d line\r\n"),
                    __FSR_FUNC__, __LINE__));
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            lock-tight blk cannot be unlocked or locked\r\n")));
                nLLDRe = FSR_LLD_INVALID_BLOCK_STATE;
            }
            break;
        default:
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[FND:ERR]   %s() / %d line\r\n"),
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

        FND_WRITE(pstFOReg->nStartBlkAddr,(UINT16) nFBA);

        /* Issue a command */
        FND_WRITE(pstFOReg->nCmd, (UINT16)nLockTypeCMD);

        WAIT_FND_INT_STAT(pstFOReg, FSR_FND_INT_MASTER_READY);

        /* Check CtrlStat register */
        if ((FND_READ(pstFOReg->nCtrlStat) & FSR_FND_STATUS_ERROR) ==
            FSR_FND_STATUS_ERROR)
        {
            *pnErrPbn = nPbn;

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[FND:ERR]   %s() / %d line\r\n"),
                __FSR_FUNC__, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Pbn #%d is fail lock tight\r\n"), nPbn));

            _DumpRegisters(pstFOReg);
            _DumpSpareBuffer(pstFOReg);

            nLLDRe = FSR_LLD_INVALID_BLOCK_STATE;
            break;
        }

        /* Set DFS, FBA */
        FND_WRITE(pstFOReg->nStartAddr1,
            (UINT16) ((nDie << FSR_FND_DFS_BASEBIT) | nFBA));

        /* Wait for WR_PROTECT_STAT */
        switch (nLockTypeCMD)
        {
        case FSR_FND_CMD_LOCKTIGHT_BLOCK:
            WAIT_FND_WR_PROTECT_STAT(pstFOReg, FSR_LLD_BLK_STAT_LOCKED_TIGHT);
            break;
        case FSR_FND_CMD_LOCK_BLOCK:
            WAIT_FND_WR_PROTECT_STAT(pstFOReg, FSR_LLD_BLK_STAT_LOCKED);
            break;
        case FSR_FND_CMD_UNLOCK_BLOCK:
            WAIT_FND_WR_PROTECT_STAT(pstFOReg, FSR_LLD_BLK_STAT_UNLOCKED);
            break;
        default:
            break;
        }

        FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_INF,
            (TEXT("[FND:INF]   nDev: %d, nPbn: %d is locked tight\r\n"),
            nDev, nPbn));

        nBlks--;
        nPbn++;
    }
  
    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[FND:OUT] --%s()\r\n"), __FSR_FUNC__));

    return (nLLDRe);
}



/**
 * @brief           This function locks the OTP block, 1st OTP block
 *
 * @param[in]       nDev        : Physical Device Number
 * @param[in]       nLockValue  : This is programmed into 1st word of sector0 of
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
            FlexONDCxt         *pstFNDCxt  = gpstFNDCxt[nDev];
            FlexONDSpec        *pstFNDSpec = pstFNDCxt->pstFNDSpec;
   volatile FlexOneNANDReg     *pstFOReg   = (volatile FlexOneNANDReg *) pstFNDCxt->nBaseAddr;
            UINT32              nBytesRet;
            UINT32              nLockState;
            UINT32              nPbn;
            UINT32              nFlushOpCaller;
            INT32               nLLDRe     = FSR_LLD_SUCCESS;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[FND:IN ] ++%s(nDev:%d, nLockValue:0x%04x)\r\n"),
        __FSR_FUNC__, nDev, nLockValue));

    do
    {
        nLLDRe = FSR_FND_IOCtl(nDev, FSR_LLD_IOCTL_OTP_GET_INFO,
                                NULL, 0,
                                (UINT8 *) &nLockState, sizeof(nLockState),
                                &nBytesRet);
        if (nLLDRe != FSR_LLD_SUCCESS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[FND:ERR]   FSR_FND_IOCtl(nCode:FSR_LLD_IOCTL_OTP_LOCK) / %s(nDev:%d) / %d line\r\n"),
                __FSR_FUNC__, nDev, __LINE__));
            break;
        }

        if ((nLockValue & FSR_FND_OTP_LOCK_MASK) == FSR_FND_LOCK_1ST_BLOCK_OTP)
        {
            /* 
             * It's an error to lock 1st block as OTP
             * when this device doesn't support 1st block as OTP
             */
            if ((pstFNDSpec->b1stBlkOTP) == FALSE32)
            {                
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[FND:ERR]   FSR_FND_IOCtl(nCode:FSR_LLD_IOCTL_OTP_LOCK) / %s(nDev:%d) / %d line\r\n"),
                        __FSR_FUNC__, nDev, __LINE__));
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            This device does not support 1st OTP.\r\n")));
                nLLDRe = FSR_LLD_INVALID_PARAM;
                break;
            }

            /* 
             * When OTP block is locked,
             * 1st block cannot be locked as OTP
             */
            if (nLockState & FSR_LLD_OTP_OTP_BLK_LOCKED)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[FND:ERR]   FSR_FND_IOCtl(nCode:FSR_LLD_IOCTL_OTP_LOCK) / %s(nDev:%d) / %d line\r\n"),
                        __FSR_FUNC__, nDev, __LINE__));
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                        (TEXT("            1st OTP cannot be locked because OTP is already locked.\r\n")));

                nLLDRe = FSR_LLD_OTP_ALREADY_LOCKED;
                break;
            }
        }

        nPbn = 0;

        /* The size of nPbn should be 4 bytes */
        FSR_ASSERT(sizeof(nPbn) == sizeof(UINT32));

        /* Enter OTP access mode */
        nLLDRe = FSR_FND_IOCtl(nDev, 
                               FSR_LLD_IOCTL_OTP_ACCESS,
                               (UINT8 *) &nPbn, sizeof(nPbn),
                               NULL, 
                               0,
                               &nBytesRet);
        if (nLLDRe != FSR_LLD_SUCCESS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[FND:ERR]   FSR_FND_IOCtl(nCode:FSR_LLD_IOCTL_OTP_ACCESS) / %s(nDev:%d) / %d line\r\n"),
                __FSR_FUNC__, nDev, __LINE__));
            break;
        }

        /* Read OTP lock data */
        FSR_FND_ReadOptimal(nDev,
                            nPbn,
                            FSR_FND_OTP_PAGE_OFFSET,
                            NULL,
                            NULL,
                            FSR_LLD_FLAG_1X_LOAD);

        nLLDRe = FSR_FND_ReadOptimal(nDev,
                                     nPbn,
                                     FSR_FND_OTP_PAGE_OFFSET,
                                     NULL,
                                     NULL,
                                     FSR_LLD_FLAG_TRANSFER);
        if (nLLDRe != FSR_LLD_SUCCESS)
        {
            break;
        }

        /* Set OTP lock */
        nLockValue = FND_READ(*(UINT16 *) pstFOReg->nDataMB10) & (UINT16) nLockValue;
        
        FND_WRITE(*(UINT16 *) pstFOReg->nDataMB10, (UINT16) nLockValue);

        /* Program */
        FSR_FND_Write(nDev, nPbn, FSR_FND_OTP_PAGE_OFFSET,
                      NULL, NULL, FSR_LLD_FLAG_OTP_PROGRAM);

        nFlushOpCaller = FSR_FND_PREOP_IOCTL << FSR_FND_FLUSHOP_CALLER_BASEBIT;

        nLLDRe = FSR_FND_FlushOp(nDev, nFlushOpCaller | 0, FSR_LLD_FLAG_NONE);
        if (nLLDRe != FSR_LLD_SUCCESS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[FND:ERR]   %s(nDev:%d, nLockValue:0x%04x) / %d line\r\n"),
                __FSR_FUNC__, nDev, nLockValue, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Program Error while locking OTP\r\n")));

            _DumpRegisters(pstFOReg);
            _DumpSpareBuffer(pstFOReg);
            break;
        }
    } while (0);

    /* Exit the OTP access mode */
    FSR_FND_IOCtl(nDev,                         /*  nDev      : Physical Device Number (0 ~ 7)   */
                  FSR_LLD_IOCTL_CORE_RESET,     /*  nCode     : IO Control Command               */
                  NULL,                         /* *pBufI     : Input Buffer pointer             */
                  0,                            /*  nLenI     : Length of Input Buffer           */
                  NULL,                         /* *pBufO     : Output Buffer pointer            */
                  0,                            /*  nLenO     : Length of Output Buffer          */
                  &nBytesRet);                  /* *pByteRet  : The number of bytes (length) of 
                                                                Output Buffer as the result of 
                                                                function call                    */

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[FND:OUT] --%s()\r\n"), __FSR_FUNC__));

    return nLLDRe;
}



/**
 * @brief           This function checks the validity of parameter
 *
 * @param[in]       nDev        : Physical Device Number (0 ~ 3)
 * @param[in]       nPbn        : Physical Block  Number
 * @param[in]       nPgOffset   : Page Offset within a block 
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
    FlexONDCxt     *pstFNDCxt;
    FlexONDSpec    *pstFNDSpec;
    UINT32          nDie;
    INT32           nLLDRe      = FSR_LLD_SUCCESS;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[FND:IN ] ++%s(nDev:%d, nPbn:%d, nPgOffset:%d)\r\n"),
        __FSR_FUNC__, nDev, nPbn, nPgOffset));

    do
    {
        /* Check Device Number */
        if (nDev >= FSR_FND_MAX_DEVS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[FND:ERR]   %s() / %d line\r\n"),
                __FSR_FUNC__, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Invalid Device Number (nDev = %d)\r\n"),
                nDev));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }

        pstFNDCxt  = gpstFNDCxt[nDev];
        pstFNDSpec = pstFNDCxt->pstFNDSpec;

        nDie       = nPbn >> (FSR_FND_DFS_BASEBIT - pstFNDCxt->nDDPSelSft);

        /* Check Device Open Flag */
        if (pstFNDCxt->bOpen == FALSE32)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[FND:ERR]   %s() / %d line\r\n"),
                __FSR_FUNC__, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Device is not opened\r\n")));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            pstFNDCxt->bOpen: 0x%08x\r\n"),
                pstFNDCxt->bOpen));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }

        /* Check Block Out of Bound */
        if (nPbn >= pstFNDSpec->nNumOfBlks)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[FND:ERR]   %s() / %d line\r\n"),
                __FSR_FUNC__, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Pbn:%d >= pstFNDSpec->nNumOfBlks:%d\r\n"),
                nPbn, pstFNDSpec->nNumOfBlks));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }
        else if ((nPbn & pstFNDCxt->nFBAMask) >= pstFNDCxt->nBlksForSLCArea[nDie])
        {
            /* Check if nPgOffset is out of Bound           */
            /* in case of MLC, pageOffset is lower than 128 */
            if (nPgOffset >= pstFNDSpec->nPgsPerBlkForMLC)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[FND:ERR]   %s() / %d line\r\n"),
                    __FSR_FUNC__, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            Pg #%d in MLC(Pbn #%d) is invalid\r\n"),
                    nPgOffset, nPbn));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            nBlksForSLCArea[%d] = %d\r\n"),
                    nDie, pstFNDCxt->nBlksForSLCArea[nDie]));

                nLLDRe = FSR_LLD_INVALID_PARAM;
                break;
            }
        }
        else
        {
            /* Check if nPgOffset is out of Bound           */
            /* in case of MLC, pageOffset is lower than 128 */
            if (nPgOffset >= pstFNDSpec->nPgsPerBlkForSLC)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[FND:ERR]   %s() / %d line\r\n"),
                    __FSR_FUNC__, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            Pg #%d in SLC(Pbn #%d) is invalid\r\n"),
                    nPgOffset, nPbn));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            nBlksForSLCArea[%d] = %d\r\n"),
                    nDie, pstFNDCxt->nBlksForSLCArea[nDie]));

                nLLDRe = FSR_LLD_INVALID_PARAM;
                break;
            }
        }
    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[FND:OUT] --%s()\r\n"), __FSR_FUNC__));

    return (nLLDRe);
}



/**
 * @brief           This function prints the contents register file
 *
 * @param[in]       pstReg      : pointer to structure FlexOneNANDReg.
 *
 * @return          none
 *
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark
 *
 */
PRIVATE VOID
_DumpRegisters(volatile FlexOneNANDReg *pstReg)
{
    UINT32      nDiesPerDev;
    UINT16      nDID;
    UINT16      nStartAddr2;
    UINT16      nDBS;

    FSR_STACK_VAR;

    FSR_STACK_END;

    nDID        = FND_READ(pstReg->nDID);
    nDiesPerDev = ((nDID >> 3) & 1) + 1;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
    (TEXT("[FND:IN ] ++%s(pstReg:0x%08x)\r\n"),
    __FSR_FUNC__, pstReg));

    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("\r\nstart dumping registers. \r\n")));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Manufacturer ID Reg."),
        &pstReg->nMID, FND_READ(pstReg->nMID)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Device ID Reg."),
        &pstReg->nDID, FND_READ(pstReg->nDID)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Version ID Reg."),
        &pstReg->nVerID, FND_READ(pstReg->nVerID)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, 
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Data Buffer Size Reg."),
        &pstReg->nDataBufSize, FND_READ(pstReg->nDataBufSize)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Boot Buffer Size Reg."),
        &pstReg->nBootBufSize, FND_READ(pstReg->nBootBufSize)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Amount of buffers Reg."),
        &pstReg->nBufAmount, FND_READ(pstReg->nBufAmount)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Technology Reg."),
        &pstReg->nTech, FND_READ(pstReg->nTech)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Start address 1 Reg."),
        &pstReg->nStartAddr1, FND_READ(pstReg->nStartAddr1)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Start address 2 Reg."),
        &pstReg->nStartAddr2, FND_READ(pstReg->nStartAddr2)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Start address 8 Reg."),
        &pstReg->nStartAddr8, FND_READ(pstReg->nStartAddr8)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Start Buffer Reg."),
        &pstReg->nStartBuf, FND_READ(pstReg->nStartBuf)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Command Reg."),
        &pstReg->nCmd, FND_READ(pstReg->nCmd)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("System Conf1 Reg."),
        &pstReg->nSysConf1, FND_READ(pstReg->nSysConf1)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Controller Status Reg."),
        &pstReg->nCtrlStat, FND_READ(pstReg->nCtrlStat)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Interrupt Reg."),
        &pstReg->nInt, FND_READ(pstReg->nInt)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Start Block Address Reg."),
        &pstReg->nStartBlkAddr, FND_READ(pstReg->nStartBlkAddr)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Write Protection Reg."),
        &pstReg->nWrProtectStat, FND_READ(pstReg->nWrProtectStat)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("ECC Status 1 Reg."),
        &pstReg->nEccStat[0], FND_READ(pstReg->nEccStat[0])));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("ECC Status 2 Reg."),
        &pstReg->nEccStat[1], FND_READ(pstReg->nEccStat[1])));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("ECC Status 3 Reg."),
        &pstReg->nEccStat[2], FND_READ(pstReg->nEccStat[2])));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("ECC Status 4 Reg."),
        &pstReg->nEccStat[3], FND_READ(pstReg->nEccStat[3])));

    if (nDiesPerDev == 2)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("other die register dump\r\n")));

        /* backup DBS */
        nStartAddr2 = FND_READ(pstReg->nStartAddr2);
        nDBS        = nStartAddr2 >> FSR_FND_DBS_BASEBIT;
        nDBS        = (nDBS + 1) & 1;

        /* set DBS */
        FND_WRITE(pstReg->nStartAddr2,
            (UINT16) (nDBS << FSR_FND_DBS_BASEBIT));

        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Start address 1 Reg."),
            &pstReg->nStartAddr1, FND_READ(pstReg->nStartAddr1)));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Start address 2 Reg."),
            &pstReg->nStartAddr2, FND_READ(pstReg->nStartAddr2)));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Start address 8 Reg."),
            &pstReg->nStartAddr8, FND_READ(pstReg->nStartAddr8)));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Controller Status Reg."),
            &pstReg->nCtrlStat, FND_READ(pstReg->nCtrlStat)));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Interrupt Reg."),
            &pstReg->nInt, FND_READ(pstReg->nInt)));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Write Protection Reg."),
            &pstReg->nWrProtectStat, FND_READ(pstReg->nWrProtectStat)));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("ECC Status 1 Reg."),
            &pstReg->nEccStat[0], FND_READ(pstReg->nEccStat[0])));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("ECC Status 2 Reg."),
            &pstReg->nEccStat[1], FND_READ(pstReg->nEccStat[1])));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("ECC Status 3 Reg."),
            &pstReg->nEccStat[2], FND_READ(pstReg->nEccStat[2])));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("ECC Status 4 Reg."),
            &pstReg->nEccStat[3], FND_READ(pstReg->nEccStat[3])));

        /* Restore DBS */
        FND_WRITE(pstReg->nStartAddr2,
            (UINT16) (nStartAddr2));
    }

    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("end dumping registers. \r\n\r\n")));


    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[FND:OUT] --%s()\r\n"), __FSR_FUNC__));
}

/**
 * @brief           This function prints spare buffer
 *
 * @param[in]       pstReg      : pointer to structure FlexOneNANDReg.
 *
 * @return          none
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 * @remark
 *
 */
PRIVATE VOID
_DumpSpareBuffer(volatile FlexOneNANDReg *pstReg)
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

    nDID        = FND_READ(pstReg->nDID);
    nDiesPerDev = ((nDID >> 3) & 1) + 1;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
    (TEXT("[FND:IN ] ++%s(pstReg:0x%08x)\r\n"),
    __FSR_FUNC__, pstReg));

    /* Backup DBS */
    nStartAddr2 = FND_READ(pstReg->nStartAddr2);

    nDataBufSize    = FND_READ(pstReg->nDataBufSize) * 2;
    nSctsPerDataBuf = nDataBufSize / FSR_SECTOR_SIZE;
    pnDataSB00      = (UINT16 *) pstReg->nDataSB00;

    for (nDieIdx = 0; nDieIdx < nDiesPerDev; nDieIdx++)
    {
        /* Set DBS */
        FND_WRITE(pstReg->nStartAddr2,
            (UINT16) (nDieIdx << FSR_FND_DBS_BASEBIT));

        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("          Dump Spare Buffer [die:%d]\r\n"), nDieIdx));
        for (nSctIdx = 0; nSctIdx < nSctsPerDataBuf; nSctIdx++)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[%02d] "), nSctIdx));
            for (nIdx = 0; nIdx < 8; nIdx++)
            {
                nValue = (UINT16) FND_READ(pnDataSB00[nSctIdx * 8 + nIdx]);
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("%04x "), nValue));
            }
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("\r\n")));
        }
    }

    /* Restore DBS */
    FND_WRITE(pstReg->nStartAddr2,
        (UINT16) (nStartAddr2));

    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("\r\n")));

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[FND:OUT] --%s()\r\n"), __FSR_FUNC__));
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
                FlexONDCxt         *pstFNDCxt       = NULL;
                FlexONDShMem       *pstFNDShMem     = NULL;
                UINT32              nDev;
#if defined(FSR_LLD_LOGGING_HISTORY)
    volatile    FlexONDOpLog       *pstFNDOpLog     = NULL;
                UINT32              nIdx;
                UINT32              nLogHead;
                UINT32              nPreOpIdx;
#endif

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG | FSR_DBZ_ERROR,
        (TEXT("[FND:IN ] ++%s()\r\n\r\n"), __FSR_FUNC__));

    for (nDev = 0; nDev < FSR_MAX_DEVS; nDev++)
    {
        pstFNDCxt  = gpstFNDCxt[nDev];

        if (pstFNDCxt == NULL)
        {
            continue;
        }

        if (pstFNDCxt->bOpen == FALSE32)
        {
            continue;
        }

        pstFNDShMem = gpstFNDShMem[nDev];

        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR | FSR_DBZ_LLD_INF,
            (TEXT("[FND:INF]   pstFNDCxt->nFlushOpCaller : %d\r\n"),
            pstFNDCxt->nFlushOpCaller));

#if defined (FSR_LLD_LOGGING_HISTORY)
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("[FND:INF]   start printing nLog      : nDev[%d]\r\n"), nDev));

        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("%5s  %7s  %10s  %10s  %10s\r\n"), TEXT("nLog"),
            TEXT("preOp"), TEXT("prePbn"), TEXT("prePg"), TEXT("preFlag")));

        pstFNDOpLog = &gstFNDOpLog[nDev];

        nLogHead = pstFNDOpLog->nLogHead;
        for (nIdx = 0; nIdx < FSR_FND_MAX_LOG; nIdx++)
        {
            nPreOpIdx = pstFNDOpLog->nLogOp[nLogHead];
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("%5d| %7s, 0x%08x, 0x%08x, 0x%08x\r\n"),
                nLogHead, gpszLogPreOp[nPreOpIdx], pstFNDOpLog->nLogPbn[nLogHead],
                pstFNDOpLog->nLogPgOffset[nLogHead], pstFNDOpLog->nLogFlag[nLogHead]));

            nLogHead = (nLogHead + 1) & (FSR_FND_MAX_LOG -1);
        }

        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("[FND:   ]   end   printing nLog      : nDev[%d]\r\n"), nDev));
#endif /* #if defined (FSR_LLD_LOGGING_HISTORY) */

        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("\r\n[FND:INF]   start printing FlexONDCxt: nDev[%d]\r\n"), nDev));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("            nPreOp           : [0x%08x, 0x%08x]\r\n"),
            pstFNDShMem->nPreOp[0], pstFNDShMem->nPreOp[1]));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("            nPreOpPbn        : [0x%08x, 0x%08x]\r\n"),
            pstFNDShMem->nPreOpPbn[0], pstFNDShMem->nPreOpPbn[1]));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("            nPreOpPgOffset   : [0x%08x, 0x%08x]\r\n"),
            pstFNDShMem->nPreOpPgOffset[0], pstFNDShMem->nPreOpPgOffset[1]));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("            nPreOpFlag       : [0x%08x, 0x%08x]\r\n"),
            pstFNDShMem->nPreOpFlag[0], pstFNDShMem->nPreOpFlag[1]));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("            end   printing FlexONDCxt: nDev[%d]\r\n\r\n"), nDev));
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG | FSR_DBZ_ERROR, (TEXT("[FND:OUT] --%s()\r\n"), __FSR_FUNC__));
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
             FlexONDCxt        *pstFNDCxt  = gpstFNDCxt[nDev];
             FlexONDSpec       *pstFNDSpec = pstFNDCxt->pstFNDSpec;
    volatile FlexOneNANDReg    *pstFOReg   = (volatile FlexOneNANDReg *) pstFNDCxt->nBaseAddr;
             UINT32             nPgIdx;
             UINT32             nNumOfPgs = 1;
             UINT32             nPageSize;

             /* Theoretical read cycle for word (2 byte) from OneNAND to host */
             UINT32             nTheoreticalRdCycle;

             UINT32             nWordRdCycle;
             UINT32             nWordWrCycle;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[FND:IN ] ++%s(nDev:%d)\r\n"), __FSR_FUNC__, nDev));

    /* nPageSize is 4096 byte */
    nPageSize = pstFNDSpec->nSctsPerPG * FSR_FND_SECTOR_SIZE;

    /* 
     * Calculate write cycle for word 
     * Here 0x55 has no meaning 
     */
    FSR_OAM_MEMSET(pstFNDCxt->pTempBuffer, 0x55, nPageSize);

#if !defined(FSR_OAM_RTLMSG_DISABLE)
    FSR_OAM_StartTimer();
#endif

    for (nPgIdx = 0; nPgIdx < nNumOfPgs; nPgIdx++)
    {
        TRANSFER_TO_NAND(pstFOReg->nDataMB00,
                         pstFNDCxt->pTempBuffer,
                         nPageSize);
    }

#if !defined(FSR_OAM_RTLMSG_DISABLE)
    FSR_OAM_StopTimer();
#endif

#if !defined(FSR_OAM_RTLMSG_DISABLE)
    /* 
     * nWordWrCycle is based on nano second
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
            (TEXT("[FND:INF]   assuming Word Write Cycle: %d nano second\r\n"),
            nWordWrCycle));
    }
    else
    {
        *pnWordWrCycle = nWordWrCycle;

        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
            (TEXT("[FND:INF]   Transfered %d bytes to DataRAM in %d usec\r\n"),
            nNumOfPgs * nPageSize, 
            FSR_OAM_GetElapsedTime()));

        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
            (TEXT("            calculated Word Write Cycle: %d nano second\r\n"),
            nWordWrCycle));
    }

    /* Calculate read cycle for word */
    FSR_OAM_MEMSET(pstFNDCxt->pTempBuffer, 0xFF, nPageSize);

#if !defined(FSR_OAM_RTLMSG_DISABLE)
    FSR_OAM_StartTimer();
#endif

    for (nPgIdx = 0; nPgIdx < nNumOfPgs; nPgIdx++)
    {
        TRANSFER_FROM_NAND(pstFNDCxt->pTempBuffer,
                           pstFOReg->nDataMB00,
                           nPageSize);
    }

#if !defined(FSR_OAM_RTLMSG_DISABLE)
    FSR_OAM_StopTimer();
#endif

    /* Time for transfering 16 bits depends on bRM (read mode) */
    nTheoreticalRdCycle = (nSysConf1Reg & FSR_FND_CONF1_SYNC_READ) ? 25 : 76;

#if !defined(FSR_OAM_RTLMSG_DISABLE)
    /* 
     * nByteRdCycle is based on nano second
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
            (TEXT("[FND:INF]   assuming Word Read  Cycle: %d nano second\r\n"),
            nWordRdCycle));
    }
    else
    {
        *pnWordRdCycle = nWordRdCycle;

        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
            (TEXT("[FND:INF]   Transfered %d bytes from DataRAM in %d usec\r\n"),
            nNumOfPgs * nPageSize,
            FSR_OAM_GetElapsedTime()));

        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
            (TEXT("            calculated Word Read  Cycle: %d nano second\r\n"),
            nWordRdCycle));
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[FND:OUT] --%s(nDev:%d)\r\n"), __FSR_FUNC__, nDev));
}



#if defined (FSR_LLD_STATISTICS)
/**
 * @brief           This function is called within the LLD function
 * @n               to total the busy time of the device
 *
 * @param[in]       nDev        : Physical Device Number (0 ~ 3)
 * @param[in]       nDie        : die(chip) number (0 or 1) for DDP device
 * @param[in]       nType       : FSR_FND_STAT_SLC_PGM
 * @n                             FSR_FND_STAT_LSB_PGM
 * @n                             FSR_FND_STAT_MSB_PGM
 * @n                             FSR_FND_STAT_ERASE
 * @n                             FSR_FND_STAT_SLC_LOAD
 * @n                             FSR_FND_STAT_MLC_LOAD
 * @n                             FSR_FND_STAT_RD_TRANS
 * @n                             FSR_FND_STAT_WR_TRANS
 * @n                             FSR_FND_STAT_FLUSH
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
_AddFNDStat(UINT32  nDev,
            UINT32  nDie,
            UINT32  nType,
            UINT32  nBytes,
            UINT32  nCmdOption)
{
    FlexONDCxt  *pstFNDCxt  = gpstFNDCxt[nDev];
    FlexONDSpec *pstFNDSpec = pstFNDCxt->pstFNDSpec;
    UINT32       nVol;
    UINT32       nDevIdx; /* Device index within a volume (0~4) */
    UINT32       nPDevIdx;/* Physical device index (0~7)        */
    UINT32       nDieIdx;
    UINT32       nNumOfDies;

    /* The duration of Interrupt Low after command is issued */
    INT32        nIntLowTime;
    INT32        nTransferTime;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[FND:IN ] ++%s()\r\n"), __FSR_FUNC__));

    nIntLowTime = pstFNDCxt->nIntLowTime[nDie];

    switch (nType)
    {
    case FSR_FND_STAT_SLC_PGM:
        pstFNDCxt->nNumOfSLCPgms++;
        
        if (nIntLowTime > 0)
        {
            /* Wait INT for previous operation */
            gnElapsedTime += nIntLowTime;
            
            for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
            {
                for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVol]; nDevIdx++)
                {
                    nPDevIdx   = nVol * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx;
                    nNumOfDies = gpstFNDCxt[nPDevIdx]->pstFNDSpec->nNumOfDies;

                    for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                    {
                        if (gpstFNDCxt[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                        {
                            gpstFNDCxt[nPDevIdx]->nIntLowTime[nDieIdx] -= nIntLowTime;
                        }
                    }
                }
            }
        }

        /* If nCmdOption is CACHE, transfering time can be hided */
        pstFNDCxt->nPreCmdOption[nDie]  = nCmdOption;
        pstFNDCxt->nIntLowTime[nDie]    = FSR_FND_WR_SW_OH + pstFNDSpec->nSLCTProgTime;

        if (nCmdOption == FSR_FND_STAT_CACHE_PGM)
        {
            pstFNDCxt->nNumOfCacheBusy++;
        }
        break;

    case FSR_FND_STAT_LSB_PGM:
        pstFNDCxt->nNumOfLSBPgms++;
        
        if (nIntLowTime > 0)
        {
            /* Wait INT for previous operation */
            gnElapsedTime += nIntLowTime;
            
            for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
            {
                for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVol]; nDevIdx++)
                {
                    nPDevIdx   = nVol * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx;
                    nNumOfDies = gpstFNDCxt[nPDevIdx]->pstFNDSpec->nNumOfDies;

                    for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                    {
                        if (gpstFNDCxt[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                        {
                            gpstFNDCxt[nPDevIdx]->nIntLowTime[nDieIdx] -= nIntLowTime;
                        }
                    }
                }
            }
        }

        /* If nCmdOption is CACHE, transfering time can be hided */
        pstFNDCxt->nPreCmdOption[nDie] = nCmdOption;
        pstFNDCxt->nIntLowTime[nDie]   = FSR_FND_WR_SW_OH + pstFNDSpec->nMLCTProgTime[0];

        if (nCmdOption == FSR_FND_STAT_CACHE_PGM)
        {
            pstFNDCxt->nNumOfCacheBusy++;
        }
        break;

    case FSR_FND_STAT_MSB_PGM:
        pstFNDCxt->nNumOfMSBPgms++;
        
        if (nIntLowTime > 0)
        {
            /* Wait INT for previous operation */
            gnElapsedTime += nIntLowTime;
            
            for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
            {
                for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVol]; nDevIdx++)
                {
                    nPDevIdx   = nVol * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx;
                    nNumOfDies = gpstFNDCxt[nPDevIdx]->pstFNDSpec->nNumOfDies;

                    for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                    {
                        if (gpstFNDCxt[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                        {
                            gpstFNDCxt[nPDevIdx]->nIntLowTime[nDieIdx] -= nIntLowTime;
                        }
                    }
                }
            }
        }

        /* If nCmdOption is CACHE, transfering time can be hided */
        pstFNDCxt->nPreCmdOption[nDie] = nCmdOption;
        pstFNDCxt->nIntLowTime[nDie]   = FSR_FND_WR_SW_OH + pstFNDSpec->nMLCTProgTime[1];

        if (nCmdOption == FSR_FND_STAT_CACHE_PGM)
        {
            pstFNDCxt->nNumOfCacheBusy++;
        }
        break;

    case FSR_FND_STAT_ERASE:
        pstFNDCxt->nNumOfErases++;
        
        if (nIntLowTime > 0)
        {
            /* Wait INT for previous operation */
            gnElapsedTime += nIntLowTime;
            
            for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
            {
                for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVol]; nDevIdx++)
                {
                    nPDevIdx   = nVol * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx;
                    nNumOfDies = gpstFNDCxt[nPDevIdx]->pstFNDSpec->nNumOfDies;

                    for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                    {
                        if (gpstFNDCxt[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                        {
                            gpstFNDCxt[nPDevIdx]->nIntLowTime[nDieIdx] -= nIntLowTime;
                        }
                    }
                }
            }
        }

        pstFNDCxt->nIntLowTime[nDie] = pstFNDSpec->nTEraseTime;
        break;

    case FSR_FND_STAT_SLC_LOAD:
        pstFNDCxt->nNumOfSLCLoads++;

        if(nIntLowTime > 0)
        {
            /* Wait INT for previous operation */
            gnElapsedTime += nIntLowTime;
            
            for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
            {
                for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVol]; nDevIdx++)
                {
                    nPDevIdx   = nVol * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx;
                    nNumOfDies = gpstFNDCxt[nPDevIdx]->pstFNDSpec->nNumOfDies;

                    for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                    {
                        if (gpstFNDCxt[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                        {
                            gpstFNDCxt[nPDevIdx]->nIntLowTime[nDieIdx] -= nIntLowTime;
                        }
                    }
                }
            }
        }

        if (nCmdOption == FSR_FND_STAT_PLOAD)
        {
            pstFNDCxt->nIntLowTime[nDie] = FSR_FND_RD_SW_OH + 
                pstFNDSpec->nSLCTLoadTime - FSR_FND_PAGEBUF_TO_DATARAM_TIME;
        }
        else
        {
            pstFNDCxt->nIntLowTime[nDie] = FSR_FND_RD_SW_OH + 
                                               pstFNDSpec->nSLCTLoadTime;
        }
        
        pstFNDCxt->nPreCmdOption[nDie] = nCmdOption;
        break;

    case FSR_FND_STAT_MLC_LOAD:
        pstFNDCxt->nNumOfMLCLoads++;

        if(nIntLowTime > 0)
        {
            /* Wait INT for previous operation */
            gnElapsedTime += nIntLowTime;
            
            for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
            {
                for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVol]; nDevIdx++)
                {
                    nPDevIdx   = nVol * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx;
                    nNumOfDies = gpstFNDCxt[nPDevIdx]->pstFNDSpec->nNumOfDies;

                    for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                    {
                        if (gpstFNDCxt[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                        {
                            gpstFNDCxt[nPDevIdx]->nIntLowTime[nDieIdx] -= nIntLowTime;
                        }
                    }
                }
            }
        }

        if (nCmdOption == FSR_FND_STAT_PLOAD)
        {
            pstFNDCxt->nIntLowTime[nDie] = FSR_FND_RD_SW_OH + 
                pstFNDSpec->nMLCTLoadTime - FSR_FND_PAGEBUF_TO_DATARAM_TIME;
        }
        else
        {
            pstFNDCxt->nIntLowTime[nDie] = FSR_FND_RD_SW_OH + 
                                               pstFNDSpec->nMLCTLoadTime;
        }

        pstFNDCxt->nPreCmdOption[nDie] = nCmdOption;
        break;

    case FSR_FND_STAT_RD_TRANS:
        pstFNDCxt->nNumOfRdTrans++;
        pstFNDCxt->nRdTransInBytes  += nBytes;
        nTransferTime = nBytes * pstFNDCxt->nRdTranferTime / 2 / 1000;

        if ((nCmdOption != FSR_FND_STAT_PLOAD) && (nIntLowTime > 0))
        {
            gnElapsedTime += nIntLowTime;

            for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
            {
                for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVol]; nDevIdx++)
                {
                    nPDevIdx   = nVol * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx;
                    nNumOfDies = gpstFNDCxt[nPDevIdx]->pstFNDSpec->nNumOfDies;

                    for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                    {
                        if (gpstFNDCxt[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                        {
                            gpstFNDCxt[nPDevIdx]->nIntLowTime[nDieIdx] -= nIntLowTime;
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
                nNumOfDies = gpstFNDCxt[nPDevIdx]->pstFNDSpec->nNumOfDies;

                for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                {
                    if (gpstFNDCxt[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                    {
                        gpstFNDCxt[nPDevIdx]->nIntLowTime[nDieIdx] -= nTransferTime;
                    }
                }
            }
        }

        if (nCmdOption == FSR_FND_STAT_PLOAD)
        {
            pstFNDCxt->nIntLowTime[nDie] = FSR_FND_PAGEBUF_TO_DATARAM_TIME;
        }
        else
        {
            pstFNDCxt->nIntLowTime[nDie] = 0;
        }

        break;

    case FSR_FND_STAT_WR_TRANS:
        pstFNDCxt->nNumOfWrTrans++;
        pstFNDCxt->nWrTransInBytes  += nBytes;
        nTransferTime   = nBytes * pstFNDCxt->nWrTranferTime / 2 / 1000;

        /* Cache operation can hide transfer time */
        if((pstFNDCxt->nPreCmdOption[nDie] == FSR_FND_STAT_CACHE_PGM) && (nIntLowTime >= 0))
        {
            gnElapsedTime  += nTransferTime;

            for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
            {
                for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVol]; nDevIdx++)
                {
                    nPDevIdx   = nVol * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx;
                    nNumOfDies = gpstFNDCxt[nPDevIdx]->pstFNDSpec->nNumOfDies;

                    for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                    {
                        if (gpstFNDCxt[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                        {
                            gpstFNDCxt[nPDevIdx]->nIntLowTime[nDieIdx] -= nTransferTime;
                        }
                    }
                }
            }

            if(nIntLowTime < nTransferTime)
            {
                pstFNDCxt->nIntLowTime[nDie] = 0;
            }
        }
        else /* Transfer time cannot be hided */
        {
            if(nIntLowTime > 0)
            {
                gnElapsedTime += nIntLowTime;

                for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
                {
                    for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVol]; nDevIdx++)
                    {
                        nPDevIdx   = nVol * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx;
                        nNumOfDies = gpstFNDCxt[nPDevIdx]->pstFNDSpec->nNumOfDies;

                        for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                        {
                            if (gpstFNDCxt[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                            {
                                gpstFNDCxt[nPDevIdx]->nIntLowTime[nDieIdx] -= nIntLowTime;
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
                    nNumOfDies = gpstFNDCxt[nPDevIdx]->pstFNDSpec->nNumOfDies;

                    for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                    {
                        if (gpstFNDCxt[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                        {
                            gpstFNDCxt[nPDevIdx]->nIntLowTime[nDieIdx] -= nTransferTime;
                        }
                    }
                }
            }

            pstFNDCxt->nIntLowTime[nDie] = 0;
        }
        break;

    case FSR_FND_STAT_FLUSH:
        if (nIntLowTime > 0)
        {
            /* Wait INT for previous operation */
            gnElapsedTime += nIntLowTime;
            
            for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
            {
                for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVol]; nDevIdx++)
                {
                    nPDevIdx   = nVol * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx;
                    nNumOfDies = gpstFNDCxt[nPDevIdx]->pstFNDSpec->nNumOfDies;

                    for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                    {
                        if (gpstFNDCxt[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                        {
                            gpstFNDCxt[nPDevIdx]->nIntLowTime[nDieIdx] -= nIntLowTime;
                        }
                    }
                }
            }

            pstFNDCxt->nIntLowTime[nDie] = 0;
        }
        break;

    default:
        break;
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[FND:OUT] --%s()\r\n"), __FSR_FUNC__));
}


/**
 * @brief           This function is same as _AddFNDStat().
 * @n               But in the needs of classifying SLC, LSB, MSB programming
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
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark          Pbn is consecutive numbers within the device
 * @n               i.e. for DDP device, Pbn for 1st block in 2nd chip is not 0
 * @n               It is one more than the last block number in 1st chip
 *
 */
PRIVATE VOID
_AddFNDStatPgm(UINT32   nDev,
               UINT32   nDie,
               UINT32   nPbn,
               UINT32   nPage,
               UINT32   nCmdOption)
{
    FlexONDCxt  *pstFNDCxt  = gpstFNDCxt[nDev];
    FlexONDSpec *pstFNDSpec = pstFNDCxt->pstFNDSpec;
    
    UINT32       nBlkOffset;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[FND:IN ] ++%s()\r\n"), __FSR_FUNC__));

    /* Block offset within the die */
    nBlkOffset  = nPbn - nDie * pstFNDSpec->nNumOfBlks / pstFNDSpec->nNumOfDies;

    if (pstFNDCxt->nBlksForSLCArea[nDie] <= nBlkOffset)
    {
        /* Program is performed on MLC block */
        if (nPage < gnPairPgMap[nPage])
        {
            _AddFNDStat(nDev, nDie, FSR_FND_STAT_LSB_PGM, 0, nCmdOption);
        }
        else
        {
            _AddFNDStat(nDev, nDie, FSR_FND_STAT_MSB_PGM, 0, nCmdOption);
        }
    }
    else
    {
        /* Program is performed on SLC block */
        _AddFNDStat(nDev, nDie, FSR_FND_STAT_SLC_PGM, 0, nCmdOption);
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[FND:OUT] --%s()\r\n"), __FSR_FUNC__));
}


/**
 * @brief           This function is same as _AddFNDStat().
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
_AddFNDStatLoad(UINT32   nDev,
                UINT32   nDie,
                UINT32   nPbn,
                UINT32   nCmdOption)
{
    FlexONDCxt  *pstFNDCxt  = gpstFNDCxt[nDev];

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[FND:IN ] ++%s()\r\n"), __FSR_FUNC__));

    if (pstFNDCxt->nBlksForSLCArea[nDie] <= (nPbn & pstFNDCxt->nFBAMask))
    {
        /* Operation is performed on MLC block */
        _AddFNDStat(nDev, nDie, FSR_FND_STAT_MLC_LOAD, 0, nCmdOption);

    }
    else
    {
        /* Operation is performed on SLC block */
        _AddFNDStat(nDev, nDie, FSR_FND_STAT_SLC_LOAD, 0, nCmdOption);
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[FND:OUT] --%s()\r\n"), __FSR_FUNC__));
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
             FlexONDShMem *pstFNDShMem;
    volatile FlexONDOpLog *pstFNDOpLog;

    UINT32 nLogHead;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG ,
        (TEXT("[FND:IN ] ++%s(nDie: %d)\r\n"), __FSR_FUNC__, nDie));

    pstFNDShMem = gpstFNDShMem[nDev];

    pstFNDOpLog = &gstFNDOpLog[nDev];

    nLogHead = pstFNDOpLog->nLogHead;

    pstFNDOpLog->nLogOp       [nLogHead]  = pstFNDShMem->nPreOp[nDie];
    pstFNDOpLog->nLogPbn      [nLogHead]  = pstFNDShMem->nPreOpPbn[nDie];
    pstFNDOpLog->nLogPgOffset [nLogHead]  = pstFNDShMem->nPreOpPgOffset[nDie];
    pstFNDOpLog->nLogFlag     [nLogHead]  = pstFNDShMem->nPreOpFlag[nDie];

    pstFNDOpLog->nLogHead = (nLogHead + 1) & (FSR_FND_MAX_LOG -1);


    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[FND:OUT] --%s()\r\n"), __FSR_FUNC__));
}
#endif /* #if defined (FSR_LLD_LOGGING_HISTORY) */



/**
 * @brief           This function initialize the structure for LLD statistics
 *
 * @return          FSR_LLD_SUCCESS
 *
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark
 *
 */
PUBLIC INT32
FSR_FND_InitLLDStat(VOID)
{
#if defined (FSR_LLD_STATISTICS)
    FlexONDCxt *pstFNDCxt;
    UINT32      nVolIdx;
    UINT32      nDevIdx;
    UINT32      nDieIdx;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[FND:IN ] ++%s()\r\n"), __FSR_FUNC__));

    gnElapsedTime = 0;

    for (nVolIdx = 0; nVolIdx < FSR_MAX_VOLS; nVolIdx++)
    {
        for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVolIdx]; nDevIdx++)
        {
            pstFNDCxt = gpstFNDCxt[nVolIdx * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx];

            pstFNDCxt->nNumOfSLCLoads = 0;
            pstFNDCxt->nNumOfSLCLoads = 0;
            pstFNDCxt->nNumOfMLCLoads = 0;

            pstFNDCxt->nNumOfSLCPgms  = 0;
            pstFNDCxt->nNumOfLSBPgms  = 0;
            pstFNDCxt->nNumOfMSBPgms  = 0;

            pstFNDCxt->nNumOfCacheBusy= 0;
            pstFNDCxt->nNumOfErases   = 0;

            pstFNDCxt->nNumOfRdTrans  = 0;
            pstFNDCxt->nNumOfWrTrans  = 0;

            pstFNDCxt->nRdTransInBytes= 0;
            pstFNDCxt->nWrTransInBytes= 0;

            for (nDieIdx = 0; nDieIdx < FSR_MAX_DIES; nDieIdx++)
            {
                pstFNDCxt->nPreCmdOption[nDieIdx] = FSR_FND_STAT_NORMAL_CMD;
                pstFNDCxt->nIntLowTime[nDieIdx]   = 0;
            }
        }
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[FND:OUT] --%s()\r\n"), __FSR_FUNC__));
#endif

    return FSR_LLD_SUCCESS;
}



/**
 * @brief           This function totals the time device consumed.
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
FSR_FND_GetStat(FSRLLDStat *pstStat)
{
#if defined (FSR_LLD_STATISTICS)
    FlexONDCxt  *pstFNDCxt;
    FlexONDSpec *pstFNDSpec;
    UINT32       nDevIdx;
    UINT32       nTotalTime;
    UINT32       nVolIdx;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[FND:IN ] ++%s()\r\n"), __FSR_FUNC__));

    do
    {
        if (pstStat == NULL)
        {
            break;
        }

        FSR_OAM_MEMSET(pstStat, 0x00, sizeof(FSRLLDStat));

        nTotalTime = 0;

        for (nVolIdx = 0; nVolIdx < FSR_MAX_VOLS; nVolIdx++)
        {
            for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVolIdx]; nDevIdx++)
            {
                pstFNDCxt = gpstFNDCxt[nVolIdx * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx];

                pstStat->nSLCPgms        += pstFNDCxt->nNumOfSLCPgms;
                pstStat->nLSBPgms        += pstFNDCxt->nNumOfLSBPgms;
                pstStat->nMSBPgms        += pstFNDCxt->nNumOfMSBPgms;
                pstStat->nErases         += pstFNDCxt->nNumOfErases;
                pstStat->nSLCLoads       += pstFNDCxt->nNumOfSLCLoads;
                pstStat->nMLCLoads       += pstFNDCxt->nNumOfMLCLoads;
                pstStat->nRdTrans        += pstFNDCxt->nNumOfRdTrans;
                pstStat->nWrTrans        += pstFNDCxt->nNumOfWrTrans;
                pstStat->nCacheBusy      += pstFNDCxt->nNumOfCacheBusy;
                pstStat->nRdTransInBytes += pstFNDCxt->nRdTransInBytes;
                pstStat->nWrTransInBytes += pstFNDCxt->nWrTransInBytes;

                pstFNDSpec    = pstFNDCxt->pstFNDSpec;

                /* nTotaltime can be compared with gnElapsedTime */
                nTotalTime   += pstStat->nSLCLoads * pstFNDSpec->nSLCTLoadTime;
                nTotalTime   += pstStat->nMLCLoads * pstFNDSpec->nMLCTLoadTime;
                nTotalTime   += pstStat->nSLCPgms  * pstFNDSpec->nSLCTProgTime;
                nTotalTime   += pstStat->nLSBPgms  * pstFNDSpec->nMLCTProgTime[0];
                nTotalTime   += pstStat->nMSBPgms  * pstFNDSpec->nMLCTProgTime[1];
                nTotalTime   += pstStat->nErases   * pstFNDSpec->nTEraseTime;

                /* 
                 * pstFNDCxt->nRdTranferTime, pstFNDCxt->nWrTranferTime is time for transfering 2 bytes
                 * which is nano second base to get a time in micro second, divide it by 1000
                 */
                nTotalTime   += pstStat->nRdTransInBytes * pstFNDCxt->nRdTranferTime / 2 / 1000;
                nTotalTime   += pstStat->nWrTransInBytes * pstFNDCxt->nWrTranferTime / 2 / 1000;
                nTotalTime   += pstStat->nCacheBusy      * FSR_FND_CACHE_BUSY_TIME;
            }
        } 
    }while (0) ;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[FND:OUT] --%s()\r\n"), __FSR_FUNC__));

    return gnElapsedTime;
#else
    FSR_OAM_MEMSET(pstStat, 0x00, sizeof(FSRLLDStat));
    return 0;
#endif /* #if defined (FSR_LLD_STATISTICS) */
}


/**
 * @brief           This function gets the unique ID from OTP Block
 *
 * @param[in]       nDev        : Physical Device Number (0 ~ 7)
 * @param[in]       pstFNDCxt   : pointer to the structure FlexONDCxt
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
             FlexONDCxt    *pstFNDCxt,
             UINT32         nFlag)
{
    FlexONDSpec    *pstFNDSpec;
    UINT32          nDie        = 0;
    UINT32          nPbn;
    UINT32          nPgOffset;
    UINT32          nByteRet;
    UINT32          nIdx;
    UINT32          nCnt;
    UINT32          nSizeOfUID;
    UINT32          nSparePerSct;
    INT32           nLLDRe      = FSR_LLD_SUCCESS;
    INT32           nRet        = 0;
    BOOL32          bValidUID;
    UINT8          *pUIDArea   = NULL;
    UINT8          *pSignature = NULL;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF,
        (TEXT("[FND:IN ] ++%s(nDev:%d, nDie:%d)\r\n"),
        __FSR_FUNC__, nDev, nDie));


    do
    {
        /* Set OTP access mode */
        nLLDRe = FSR_FND_IOCtl(nDev,                     
                               FSR_LLD_IOCTL_OTP_ACCESS,
                               (UINT8 *) &nDie,
                               sizeof(nDie),
                               NULL,
                               0,
                               &nByteRet);
        if (FSR_RETURN_MAJOR(nLLDRe) != FSR_LLD_SUCCESS)
        {
            break;
        }

        FSR_ASSERT(pstFNDCxt != NULL);
        FSR_ASSERT(pstFNDCxt->pstFNDSpec != NULL);

        pstFNDSpec   = pstFNDCxt->pstFNDSpec;
        nSparePerSct = pstFNDSpec->nSparePerSct;

        FSR_OAM_MEMSET(pstFNDCxt->pTempBuffer, 0x00,
            pstFNDSpec->nSctsPerPG * (FSR_SECTOR_SIZE + nSparePerSct));

        /* Unique ID is located at page 60 of OTP block */
        nPgOffset = 60;
        nPbn      = 0;

        pUIDArea   = pstFNDCxt->pTempBuffer;
        pSignature = pstFNDCxt->pTempBuffer + (pstFNDSpec->nSctsPerPG * FSR_SECTOR_SIZE);

        nLLDRe = FSR_FND_Read(nDev, nPbn, nPgOffset,
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

        /* STEP 1 : check signature 
         *
         * | Spare0 (16B) | Spare1 (16B) | Spare2 (16B) | Spare3 (16B) |
         * |--------------+--------------+--------------+--------------|
         * |  0xAAAA5555  |                All 0xFFFF                  |
         *
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

        /* 
         * STEP 2 : check complenent data 
         * 
         * |   Unique ID (512B)  | Reserved (512B) | Reserved (512B) | Reserved (512B) |
         * |---------------------+-----------------+-----------------+-----------------|
         * | ID(32B) & Cplmt(32B)|                    All 0xFFFF                       |
         */
        bValidUID = FALSE32;

        /* nSizeOfUID is 32 byte */
        nSizeOfUID = (FSR_SECTOR_SIZE / FSR_FND_NUM_OF_UID) / 2;

        for (nCnt = 0; nCnt < FSR_FND_NUM_OF_UID; nCnt++)
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

            /* Skip 64 byte */
            pUIDArea += (FSR_SECTOR_SIZE / FSR_FND_NUM_OF_UID);
        }

        if (bValidUID == FALSE32)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF, (TEXT("[FND:ERR]   Invalid UID\r\n")));
            break;
        }

        /* Get Unique ID */
        for (nIdx = 0; nIdx < FSR_LLD_UID_SIZE; nIdx++)
        {
            pstFNDCxt->nUID[nIdx] = pUIDArea[2 * nIdx];
        }

    } while (0);

    /* Exit the OTP block */
    nLLDRe = FSR_FND_IOCtl(nDev,
                           FSR_LLD_IOCTL_CORE_RESET,
                           NULL,
                           0,
                           NULL,
                           0,
                           &nByteRet);

#if defined (FSR_LLD_READ_PRELOADED_DATA)
    gpstFNDShMem[nDev]->bIsPreCmdLoad[nDie] = FALSE32;
#endif

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF,
        (TEXT("[FND:OUT] --%s()\r\n"), __FSR_FUNC__));

    return (nLLDRe);
}

