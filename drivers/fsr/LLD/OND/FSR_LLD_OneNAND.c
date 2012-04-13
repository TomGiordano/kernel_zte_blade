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
 * @file      FSR_LLD_OneNAND.c
 * @brief     This is Low level driver of OneNAND
 * @author    JeongWook Moon
 * @date      19-MAR-2007
 * @remark
 * REVISION HISTORY
 * @n  19-MAR-2007 [JeongWook Moon] : first writing
 * @n  13-MAY-2009 [Kyungho Shin]   : Support tinyFSR in dual core
 */


/*****************************************************************************/
/* Header file inclusions                                                    */
/*****************************************************************************/
#define     FSR_NO_INCLUDE_BML_HEADER
#define     FSR_NO_INCLUDE_STL_HEADER

#include    "FSR.h"
#include    "FSR_LLD_OneNAND.h"
#include    "FSR_LLD_SWEcc.h"

/********************************************************************************/
/*   Local Configurations                                                       */
/*                                                                              */
/*   FSR_LLD_STRICT_CHK           : To check parameters strictly                */
/*   FSR_LLD_STATISTICS           : To collect LLD statistics                   */
/*   FSR_LLD_USE_CACHE_PGM        : To support cache program                    */
/********************************************************************************/
#define     FSR_LLD_STRICT_CHK
#undef      FSR_LLD_STATISTICS
#define     FSR_LLD_USE_CACHE_PGM
#define     FSR_LLD_LOGGING_HISTORY

#if defined(_WIN32)
#define     FSR_LLD_STATISTICS
#undef     FSR_LLD_STATISTICS
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
#include    "..\Inc\FSR_FOE_Interface.h"
#endif /* #if defined(FSR_ONENAND_EMULATOR) */

/*****************************************************************************/
/* Local #defines                                                            */
/*****************************************************************************/

#define     FSR_OND_MAX_DEVS                    FSR_MAX_DEVS

/* OneNAND register : bit information */
#define     FSR_OND_SYSCONF1_RM_BASEBIT         (15)        /* sys config 1 register       */
#define     FSR_OND_SYSCONF1_BRWL_BASEBIT       (12)        /* sys config 1 register       */
#define     FSR_OND_SYSCONF1_BL_BASEBIT         (9)         /* sys config 1 register       */
#define     FSR_OND_SYSCONF1_ECC_BASEBIT        (8)         /* sys config 1 register       */
#define     FSR_OND_SYSCONF1_RDY_POL_BASEBIT    (7)         /* sys config 1 register       */
#define     FSR_OND_SYSCONF1_INT_POL_BASEBIT    (6)         /* sys config 1 register       */
#define     FSR_OND_SYSCONF1_IOBE_BASEBIT       (5)         /* sys config 1 register       */
#define     FSR_OND_SYSCONF1_RDYCONF_BASEBIT    (4)         /* sys config 1 register       */
#define     FSR_OND_SYSCONF1_HF_BASEBIT         (2)         /* sys config 1 register       */
#define     FSR_OND_SYSCONF1_WM_BASEBIT         (1)         /* sys config 1 register       */

#define     FSR_OND_INT_READY                   (0x8000)    /* interrupt status register   */
#define     FSR_OND_INT_RI_READY                (0x0080)
#define     FSR_OND_INT_WI_READY                (0x0040)
#define     FSR_OND_INT_EI_READY                (0x0020)
#define     FSR_OND_INT_CLEAR                   (0x0000)
#define     FSR_OND_DBS_BASEBIT                 (15)        /* start address 2 register    */
#define     FSR_OND_DFS_BASEBIT                 (15)        /* start address 1 register    */
#define     FSR_OND_STATUS_ERROR                (0x0400)    /* controller status register  */
#define     FSR_OND_STATUS_ERROR_PREV1          (0x0010)    /* controller status register  */
#define     FSR_OND_STATUS_ERROR_CURR1          (0x0008)    /* controller status register  */
#define     FSR_OND_STATUS_ERROR_PREV2          (0x0004)    /* controller status register  */
#define     FSR_OND_STATUS_ERROR_CURR2          (0x0002)    /* controller status register  */
#define     FSR_OND_ECC_NOERROR                 (0x0000)    /* ECC status register 1,2,3,4 */
#define     FSR_OND_ECC_UNCORRECTABLE           (0xAAAA)    /* ECC status register 1,2,3,4 */
#define     FSR_OND_DDP_CHIP                    (0x0008)    /* Device ID register          */
#define     FSR_OND_CONF1_RM_MASK               (0x8000)    /* config1 Reg. read mode*/

/* The size of array whose element can be put into command register */
#define     FSR_OND_MAX_PGMCMD                  (5)         /* the size of gnPgmCmdArray   */
#define     FSR_OND_MAX_LOADCMD                 (4)         /* the size of gnLoadCmdArray  */
#define     FSR_OND_MAX_ERASECMD                (2)         /* the size of gnEraseCmdArray */
#define     FSR_OND_MAX_CPBKCMD                 (5)         /* the size of gnCpBkCmdArray  */
#define     FSR_OND_MAX_BADMARK                 (4)         /* the size of gnBadMarkValue  */
#define     FSR_OND_MAX_BBMMETA                 (2)         /* the size of gnBBMMetaValue  */
#define     FSR_OND_MAX_ECC_STATUS_REG          (9)         /* # of ECC status registers   */

/* Register masking values */
#define     FSR_OND_MASK_DFS                    (0x8000)
#define     FSR_OND_MASK_DBS                    (0x8000)
#define     FSR_OND_MASK_OTPBL                  (0x0020)
#define     FSR_OND_MASK_OTPL                   (0x0040)
#define     FSR_OND_MASK_OTP                    (FSR_OND_MASK_OTPBL | FSR_OND_MASK_OTPL)
#define     FSR_OND_MASK_DID                    (0xFFF8)    /* Device ID Register    */
#define     FSR_OND_MASK_DID_VCC                (0x0003)    /* Device ID Register    */

/* Other size value */
#define     FSR_OND_SECTOR_SIZE                 (512)
#define     FSR_OND_SPARE_SIZE                  (16)
#define     FSR_OND_NUM_OF_BAD_MARK_PAGES       (2)

/* Bad Mark Configuration */
#define     FSR_OND_VALID_BLK_MARK              (0xFFFF)

/* Time limit configuration */
#define     FSR_OND_BUSYWAIT_TIME_LIMIT         (10000000)

#define     FSR_OND_WR_PROTECT_TIME_LIMIT       (25000000)

/* Transfering data between DataRAM & host DRAM depends on DMA & sync mode.
 * Because DMA & sync burst needs some preparation,
 * when the size of transferring data is small,
 * it's better to use load, store command of ARM processor.
 * Confer _ReadMain(), _WriteMain().
 * FSR_OND_MIN_BULK_TRANS_SIZE stands for minimum transfering size.
 */
#define     FSR_OND_MIN_BULK_TRANS_SIZE         (16 * 2)

#define     FSR_OND_OTP_LOCK_MASK               (0xFF)
#define     FSR_OND_LOCK_OTP_BLOCK              (0xFC)
#define     FSR_OND_LOCK_1ST_BLOCK_OTP          (0xF3)
#define     FSR_OND_LOCK_BOTH_OTP               (0xF0)
#define     FSR_OND_OTP_PAGE_OFFSET             (0)
#define     FSR_OND_LOCK_SPARE_BYTE_OFFSET      (14)

/* Hardware ECC of OneNAND use 4 bytes of spare area                    */
#define     FSR_OND_SPARE_HW_ECC_AREA           (5)

/* Hardware ECC of OneNAND use five UINT16 (2 bytes) of spare area      */
#define     FSR_OND_SPARE_SW_ECC_AREA           (2)

#define     FSR_OND_SPARE_USER_AREA             (6)

/* Register shift or mask value */
#define     FSR_OND_FPA_SHIFT                   (2)
#define     FSR_OND_FSA_MASK                    (3)
#define     FSR_OND_BSA_SHIFT                   (8)
#define     FSR_OND_OTP_SHIFT                   (5)
#define     FSR_OND_OTP_LOCK                    (2)

/* Buffer Sector Count specifies the number of sectors to read               */
#define     FSR_OND_BSC00                       (0)
#define     FSR_OND_BSC01                       (1)             /* Using for UID */

/* Buffer Sector Address specifies the sector address in the internal BootRAM and DataRAM */
#define     FSR_OND_MDRAM0_SIZE                 (2048)
#define     FSR_OND_SDRAM0_SIZE                 (64)
#define     FSR_OND_BSA1000                     (0x8)
#define     FSR_OND_BSA1100                     (0xC)

#define     FSR_OND_STARTADDR1_DEFAULT          (0x0000)
#define     FSR_OND_STARTADDR2_DEFAULT          (0x0000)
#define     FSR_OND_STARTADDR8_DEFAULT          (0x0000)

/* OTP set to spare area */
#define     FSR_OND_LOCK_BOTH                   (0xF0)
#define     FSR_OND_LOCK_1STBLK                 (0xF3)
#define     FSR_OND_LOCK_OTP                    (0xFC)

/* OneNAND command which is written to Command Register                    */
#define     FSR_OND_CMD_1X_LOAD                 (0x0000)
#define     FSR_OND_CMD_1X_PROGRAM              (0x0080)
#define     FSR_OND_CMD_2X_PROGRAM              (0x007D)
#define     FSR_OND_CMD_2X_CACHEPGM             (0x007F)
#define     FSR_OND_CMD_ERASE                   (0x0094)
#define     FSR_OND_CMD_OTPACCESS               (0x0065)
#define     FSR_OND_CMD_ALLBLK_UNLOCK           (0x0027)
#define     FSR_OND_CMD_HOT_RESET               (0x00F3)
#define     FSR_OND_CMD_CORE_RESET              (0x00F0)

/* 
 * 14th bit of Controller Status Register (F240h) shows
 * whether the host is programming/erasing a locked block of the NAND Flash Array
 * OneNAND does not use this bit, and it is fixed to zero.
 */
#define     FSR_OND_LOCK_STATE                  (0x4000)

/* 
 * With the help of emulator, LLD can run without the OneNAND
 * in that case, define FSR_ONENAND_EMULATOR
 */
#if defined (FSR_ONENAND_EMULATOR)

    #define     OND_WRITE(nAddr, nDQ)                   {FSR_FOE_Write((UINT32)&(nAddr), nDQ);}
    #define     OND_READ(nAddr)                         ((UINT16) FSR_FOE_Read((UINT32)&(nAddr)))
    #define     OND_SET(nAddr, nDQ)                     {FSR_FOE_Write((UINT32)&(nAddr), (UINT16) FSR_FOE_Read((UINT32)&(nAddr)) | nDQ);}
    #define     OND_CLR(nAddr, nDQ)                     {FSR_FOE_Write((UINT32)&(nAddr), (UINT16) FSR_FOE_Read((UINT32)&(nAddr)) & nDQ);}

    #define     TRANSFER_TO_NAND(pDst, pSrc, nSize)                            \
                    FSR_FOE_TransferToDataRAM(pDst, pSrc, nSize)

    /* nSize is the number of bytes to transfer                               */
    #define     TRANSFER_FROM_NAND(pDst, pSrc, nSize)                          \
                    FSR_FOE_TransferFromDataRAM(pDst, pSrc, nSize)

    /* Macro below is for emulation purpose */
    #define     OND_MEMSET_DATARAM(pDst, nVal, nSize)                          \
                    FSR_FOE_MemsetDataRAM((pDst), (nVal), (nSize))

#else /* #if defined (FSR_ONENAND_EMULATOR) */

    #define     OND_WRITE(nAddr, nDQ)                   {nAddr  = nDQ;}
    #define     OND_READ(nAddr)                         (nAddr        )
    #define     OND_SET(nAddr, nDQ)                     {nAddr  = (nAddr | nDQ);}
    #define     OND_CLR(nAddr, nDQ)                     {nAddr  = (nAddr & nDQ);}

    /* nSize is the number of bytes to transfer */
    #define     TRANSFER_TO_NAND(pDst, pSrc, nSize)     FSR_PAM_TransToNAND(pDst, pSrc, nSize)
    #define     TRANSFER_FROM_NAND(pDst, pSrc, nSize)   FSR_PAM_TransFromNAND(pDst, pSrc, nSize)

    /* macro below is for emulation purpose
     * in real target environment, it's same as memset()
     */
    #define     OND_MEMSET_DATARAM(pDst, nVal, nSize)                          \
                    FSR_OAM_MEMSET((pDst), (nVal), (nSize))

#endif /* #if defined (FSR_ONENAND_EMULATOR) */

#define WAIT_OND_INT_STAT(x, a)                                                                     \
            {INT32 nTimeLimit = FSR_OND_BUSYWAIT_TIME_LIMIT;                                        \
            OND_READ(x->nCtrlStat);                                                                 \
            OND_READ(x->nCtrlStat);                                                                 \
            OND_READ(x->nCtrlStat);                                                                 \
            OND_READ(x->nCtrlStat);                                                                 \
            OND_READ(x->nCtrlStat);                                                                 \
            OND_READ(x->nCtrlStat);                                                                 \
            OND_READ(x->nCtrlStat);                                                                 \
            OND_READ(x->nCtrlStat);                                                                 \
            OND_READ(x->nCtrlStat);                                                                 \
            OND_READ(x->nCtrlStat);                                                                 \
            OND_READ(x->nCtrlStat);                                                                 \
            OND_READ(x->nCtrlStat);                                                                 \
            while ((OND_READ(x->nInt) & a) != (UINT16) a)                                            \
            {                                                                                       \
                if (--nTimeLimit == 0)                                                              \
                {                                                                                   \
                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR , (TEXT("[OND:ERR]   busy wait time-out / %s,%d\r\n"), \
                    __FSR_FUNC__, __LINE__));                                                       \
                    _DumpRegisters(x);                                                              \
                    _DumpCmdLog();                                                     \
                    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,                               \
                    (TEXT("[OND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, FSR_LLD_NO_RESPONSE));\
                    return (FSR_LLD_NO_RESPONSE);                                                   \
                }                                                                                   \
            }}

/* In order to wait 3s, call OND_READ(x->nInt) 12 times */
#define WAIT_OND_WR_PROTECT_STAT(x, a, b, c)                                   \
    {INT32 nTimeLimit = FSR_OND_WR_PROTECT_TIME_LIMIT;                         \
    while ((OND_READ(x->nWrProtectStat) & (a)) != (UINT16) (a))          \
    {                                                                          \
        if (--nTimeLimit == 0)                                                 \
        {                                                                      \
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,                                     \
                (TEXT("[OND:ERR] WR protect busy wait time-out %s(), %d line\r\n"),\
                __FSR_FUNC__, __LINE__));                                      \
            _DumpRegisters(x);                                                 \
            _DumpSpareBuffer(x, b, c);                                   \
            _DumpCmdLog();                                                     \
            FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,                  \
                (TEXT("[OND:OUT] --%s()\r\n"), __FSR_FUNC__));                 \
            return (FSR_LLD_NO_RESPONSE);                                      \
        }                                                                      \
    }}

/* MACROs below are used for the statistic                                   */
#define FSR_OND_STAT_SLC_PGM                    (0x00000001)
#define FSR_OND_STAT_LSB_PGM                    (0x00000002)
#define FSR_OND_STAT_MSB_PGM                    (0x00000003)
#define FSR_OND_STAT_ERASE                      (0x00000004)
#define FSR_OND_STAT_SLC_LOAD                   (0x00000005)
#define FSR_OND_STAT_MLC_LOAD                   (0x00000006)
#define FSR_OND_STAT_RD_TRANS                   (0x00000007)
#define FSR_OND_STAT_WR_TRANS                   (0x00000008)
#define FSR_OND_STAT_WR_CACHEBUSY               (0x00000009)
#define FSR_OND_STAT_FLUSH                      (0x0000000A)

/* Command type for statistics */
#define FSR_OND_STAT_NORMAL_CMD                 (0x0)
#define FSR_OND_STAT_PLOAD                      (0x1)
#define FSR_OND_STAT_CACHE_PGM                  (0x2)

#define FSR_OND_CACHE_BUSY_TIME                 (25) /* usec */
#define FSR_OND_RD_SW_OH                        (5)  /* usec */
#define FSR_OND_WR_SW_OH                        (10) /* usec */

/* In sync burst mode, reading from DataRAM has overhead */
#define FSR_OND_RD_TRANS_SW_OH                  (14) /* usec */

/* Device ID for 3G DDP OneNAND       */
#define FSR_OND_4G_DDP_DID                      (0x005C)
#define FSR_OND_2G_DDP_DID                      (0x0048)
#define FSR_OND_3G_DDP_DID                      (0x0052)

/* Signature value about UniqueID */
#define FSR_OND_SIGNATURE_IN_OTP                (0xAAAA5555)
#define FSR_OND_NUM_OF_UID                      (8)
#define FSR_OND_NUM_OF_SIG                      (4)

#define FSR_OND_NULL_VBN                        (0xFFFF)

/*****************************************************************************/
/* Local typedefs                                                            */
/*****************************************************************************/
/**
 * @brief data structure of OneNAND specification
 */
typedef struct
{
        UINT16      nMID;               /**< manufacturer ID                     */
        UINT16      nDID;               /**< device ID                           */
        
        UINT16      nNumOfBlks;         /**< the number of blocks                */
        UINT16      nNumOfBlksIn1stDie; /**< the number of blocks in first die   */
        UINT16      nNumOfPlanes;       /**< the number of planes                */
        UINT16      nSparePerSct;       /**< # of bytes of spare per sector      */

        UINT16      nSctsPerPG;         /**< the number of sectors per page      */
        UINT16      nNumOfRsvrInSLC;    /**< # of bad blocks in SLC area         */
        UINT32      nPgsPerBlkForSLC;   /**< # of pages per block in SLC area    */
        
        UINT16      nNumOfDies;         /**< # of dies in NAND device            */
        UINT16      nUserOTPScts;       /**< # of user sectors                   */
        BOOL32      b1stBlkOTP;         /**< support 1st block OTP or not        */
        
        /* TrTime, TwTime of MLC are array of size 2
         * first  element is for LSB TLoadTime, TProgTime
         * second element is for MLB TLoadTime, TProgTime
         * use macro FSR_LLD_IDX_LSB_TIME, FSR_LLD_IDX_MSB_TIME
         */
        UINT32      nSLCTLoadTime;      /**< Typical Load     operation time     */
        UINT32      nSLCTProgTime;      /**< Typical Program  operation time     */
        UINT32      nTEraseTime;        /**< Typical Erase    operation time     */
    
        /* endurance information                                                 */
        UINT32      nSLCPECycle;        /**< program, erase cycle of SLC block   */

const   UINT16      nECC2LvRdDist;      /**< ECC 1 bit error                  */
        UINT16      nPad;
        
} OneNANDSpec;

/** @brief   Shared data structure for communication in Dual Core             */
typedef struct
{
    UINT32  nShMemUseCnt;

    /**< Previous operation data structure which can be shared among process  */
    UINT16  nPreOp[FSR_MAX_DIES];
    UINT16  nPreOpPbn[FSR_MAX_DIES];
    UINT16  nPreOpPgOffset[FSR_MAX_DIES];
    UINT32  nPreOpFlag[FSR_MAX_DIES];
} OneNANDShMem;


/**
 * @brief data structure of OneNAND LLD context for each device number
 */
typedef struct
{
    UINT32       nBaseAddr;             /**< the base address of OneNAND    */
    BOOL32       bOpen;                 /**< open flag : TRUE32 or FALSE32       */

    UINT8        nDDPSelSft;            /**< the shift value of DDP selection    */
    UINT8        nFPASelSft;            /**< the shift value of FPA selection    */
    UINT16       nFBAMask;              /**< the mask of Flash Block Address     */

    UINT8        nFSAMask;              /**< the mask of Flash Sector Address    */
    UINT16       nBSASelSft;            /**< the shift value of BSA selection    */

    BOOL32       bIsPreCmdCache[FSR_MAX_DIES];

    UINT16       nBlksForSLCArea[FSR_MAX_DIES];/**< # of blocks for SLC area     */

    UINT32       nWrTranferTime;        /**< write transfer time                 */
    UINT32       nRdTranferTime;        /**< read transfer time                  */

    UINT32       nECCRes[2];            /**< ECC result of 2 plane               */
    UINT32       nPreUseBuf[FSR_MAX_DIES]; 
                                        /**< previous used DataRAM for each die  */
    UINT8        nUID[FSR_LLD_UID_SIZE];/**< Unique ID info. about OTP block     */

    UINT16       nSysConf1;             /**< when opening a device save the value
                                           of system configuration 1 register.
                                           restore this value after hot reset    */
    UINT16       nUnlockedPbn;          /**< store pbn using for OTP program     */

    UINT32       nIntID;                /**< interrupt ID : non-blocking I/O     */

    UINT8       *pDataBuf4CpBk;         /**< DataBuffer for copyback 
                                            accessing 3G DDP                     */
    UINT8       *pTempBuffer;           /**< buffer for temporary use            */

    OneNANDSpec *pstONDSpec;            /**< pointer to OneNANDSpec              */

} OneNANDCxt;

/**
 * @brief Data structure of OneNAND statistics
 */
typedef struct
{
    UINT32      nNumOfSLCLoads; /**< the number of times of Load  operations */
    
    UINT32      nNumOfSLCPgms;  /**< the number of times of SLC programs     */
    
    UINT32      nNumOfCacheBusy;/**< the number of times of Cache Busy       */
    UINT32      nNumOfErases;   /**< the number of times of Erase operations */

    UINT32      nNumOfRdTrans;  /**< the number of times of Read  Transfer   */
    UINT32      nNumOfWrTrans;  /**< the number of times of Write Transfer   */

    UINT32      nPreCmdOption[FSR_MAX_DIES]; /** previous command option     */

    INT32       nIntLowTime[FSR_MAX_DIES];
                                /**< MDP : 0 
                                     DDP : previous operation time           */

    UINT32      nRdTransInBytes;/**< the number of bytes transfered in read  */
    UINT32      nWrTransInBytes;/**< the number of bytes transfered in write */

    UINT32      nNumOfMLCLoads;
    UINT32      nNumOfLSBPgms;  /**< the number of times of LSB programs     */
    UINT32      nNumOfMSBPgms;  /**< the number of times of MSB programs     */
} OneNANDStat;


/*****************************************************************************/
/* Global variable definitions                                               */
/*****************************************************************************/

/* 
 * gnPgmCmdArray contains the actual COMMAND for the program
 * the size of array can be changed for the future extention
 */
PRIVATE const UINT16    gnPgmCmdArray[FSR_OND_MAX_PGMCMD] =
                            { FSR_OND_CMD_1X_PROGRAM,   /* FSR_LLD_FLAG_1X_PROGRAM      */
                              FSR_OND_CMD_1X_PROGRAM,   /* FSR_LLD_FLAG_1X_CACHEPGM     */
                              FSR_OND_CMD_2X_PROGRAM,   /* FSR_LLD_FLAG_2X_PROGRAM      */
                              FSR_OND_CMD_2X_CACHEPGM,  /* FSR_LLD_FLAG_2X_CACHEPGM     */
                              FSR_OND_CMD_1X_PROGRAM,   /* FSR_LLD_FLAG_OTP_PROGRAM     */
                            };

/* gnLoadCmdArray contains the actual COMMAND for the load
 * the size of array can be changed for the future extention
 */
PRIVATE const UINT16    gnLoadCmdArray[FSR_OND_MAX_LOADCMD] =
                            { 0xFFFF,              /* FSR_LLD_FLAG_NO_LOADCMD      */
                              FSR_OND_CMD_1X_LOAD, /* FSR_LLD_FLAG_1X_LOAD         */
                              FSR_OND_CMD_1X_LOAD, /* FSR_LLD_FLAG_1X_PLOAD        */
                              FSR_OND_CMD_1X_LOAD, /* FSR_LLD_FLAG_2X_LOAD         */
                                                   /* FSR_LLD_FLAG_2X_PLOAD        */
                            };

/* gnEraseCmdArray contains the actual COMMAND for the erase
 * the size of array can be changed for the future extention
 */
PRIVATE const UINT16    gnEraseCmdArray[FSR_OND_MAX_ERASECMD] = 
                            { FSR_OND_CMD_ERASE, /* FSR_LLD_FLAG_1X_ERASE        */
                              FSR_OND_CMD_ERASE, /* FSR_LLD_FLAG_2X_ERASE        */
                            };

/* gnBadMarkValue contains the actual 2 byte value which is programmed into the first 2 byte of spare area
 * the size of array can be changed for the future extention
 */
PRIVATE const UINT16    gnBadMarkValue[FSR_OND_MAX_BADMARK] = 
                            { 0xFFFF, /* FSR_LLD_FLAG_WR_NOBADMARK    */
                              0x2222, /* FSR_LLD_FLAG_WR_EBADMARK     */
                              0x4444, /* FSR_LLD_FLAG_WR_WBADMARK     */
                              0x8888, /* FSR_LLD_FLAG_WR_LBADMARK     */
                            };

/* gnCpBkCmdArray contains the actual COMMAND for the copyback
 * the size of array can be changed for the future extention
 */
PRIVATE const UINT16    gnCpBkCmdArray[FSR_OND_MAX_CPBKCMD] = 
                            { 0xFFFF,
                              FSR_OND_CMD_1X_LOAD,    /* FSR_LLD_FLAG_1X_CPBK_LOAD    */
                              FSR_OND_CMD_1X_PROGRAM, /* FSR_LLD_FLAG_1X_CPBK_PROGRAM */
                              FSR_OND_CMD_1X_LOAD,    /* FSR_LLD_FLAG_2X_CPBK_LOAD    */
                              FSR_OND_CMD_2X_PROGRAM, /* FSR_LLD_FLAG_2X_CPBK_PROGRAM */
                                                      /* FSR_LLD_FLAG_4X_CPBK_LOAD    */
                                                      /* FSR_LLD_FLAG_4X_CPBK_PROGRAM */
                            };

/* nBlkType of FSRSpareBuf can have 0x00 or 0xFF */
PRIVATE const UINT16    gnBBMMetaValue[FSR_OND_MAX_BBMMETA] = 
                            { 0xFFFF,
                              FSR_LLD_BBM_META_MARK
                            };

#if defined (FSR_LLD_LOGGING_HISTORY) && !defined(FSR_OAM_RTLMSG_DISABLE)
PRIVATE const UINT8    *gpszLogPreOp[] =
                            { (UINT8 *) "NONE ",
                              (UINT8 *) "READ ",
                              (UINT8 *) "1XPGM",
                              (UINT8 *) "2XPGM",
                              (UINT8 *) "ERASE",
                              (UINT8 *) "IOCTL",
                              (UINT8 *) "CHPGM",
                            };
#endif

PRIVATE OneNANDCxt     *gpstONDCxt[FSR_OND_MAX_DEVS];

#if !defined(WITH_TINY_FSR)
OneNANDShMem *gpstONDShMem[FSR_OND_MAX_DEVS];
#endif

#if defined (WITH_TINY_FSR)
extern OneNANDShMem *gpstONDShMem[FSR_OND_MAX_DEVS];
#endif

#if defined (FSR_LLD_STATISTICS)
PRIVATE OneNANDStat    *gpstONDStat[FSR_OND_MAX_DEVS];
PRIVATE UINT32          gnElapsedTime;
PRIVATE UINT32          gnDevsInVol[FSR_MAX_VOLS] = {0, 0};
#endif /* #if defined (FSR_LLD_STATISTICS) */

#if defined(FSR_LLD_LOGGING_HISTORY)
volatile OneNANDOpLog gstONDOpLog[FSR_MAX_DEVS] =
{
    {0, {0,}, {0,}, {0,},{0,}},
    {0, {0,}, {0,}, {0,},{0,}},
    {0, {0,}, {0,}, {0,},{0,}},
    {0, {0,}, {0,}, {0,},{0,}}
};
#endif

/*****************************************************************************/
/* Extern variable declarations                                              */
/*****************************************************************************/

/*****************************************************************************/
/* Local constant definitions                                                */
/*****************************************************************************/

PRIVATE const OneNANDSpec gstONDSpec[] = {
/********************************************************************************************/
/* 1. nMID                                                                                  */
/* 2.         nDID                                                                          */
/* 3.                 nNumOfBlks                                                            */
/* 4.                       nNumOfBlksIn1stDie                                              */
/* 5.                             nNumOfPlanes                                              */
/* 6.                                nSparePerSct                                           */
/* 7.                                    nSctsPerPG                                         */
/* 8.                                       nNumOfRsvrInSLC                                 */
/* 9.                                           nPgsPerBlkForSLC                            */
/* 10.                                              nNumOfDies                              */
/* 11.                                                 nUserOTPScts                         */
/* 12.                                                     b1stBlkOTP                       */
/* 13.                                                             nSLCTLoadTime            */
/* 14.                                                                 nSLCTProgTime        */
/* 15.                                                                      nTEraseTime     */
/* 16.                                                                           nSLCPECycle*/
/********************************************************************************************/
    /* KFG1G16Q2M */
    { 0x00EC, 0x0030, 1024, 1024, 1, 16, 4, 20, 64, 1, 50, TRUE32, 30, 220, 2000, 100000, 0x5555, 0},

    /* KFG1G16U2D (OneNAND 1G D-die)*/
    { 0x00EC, 0x0035, 1024, 1024, 1, 16, 4, 20, 64, 1, 50, TRUE32, 30, 250, 2000, 100000, 0x5555, 0},

    /* KFM1G16Q2D (OneNAND MUX 1G D-die)*/
    { 0x00EC, 0x0030, 1024, 1024, 1, 16, 4, 20, 64, 1, 50, TRUE32, 30, 250, 2000, 100000, 0x5555, 0},

    /* KFG1G16Q2D (OneNAND 1G D-die)*/
    { 0x00EC, 0x0034, 1024, 1024, 1, 16, 4, 20, 64, 1, 50, TRUE32, 30, 250, 2000, 100000, 0x5555, 0},

    /* KFG2G16Q2M */
    { 0x00EC, 0x0044, 2048, 2048, 2, 16, 4, 40, 64, 1, 50, TRUE32, 30, 220, 2000, 100000, 0x5555, 0},

    /* KFH4G16Q2M */
    { 0x00EC, 0x005C, 4096, 2048, 2, 16, 4, 80, 64, 2, 50, TRUE32, 30, 220, 2000, 100000, 0x5555, 0},

    /* KFM2G16Q2M */
    { 0x00EC, 0x0040, 2048, 2048, 2, 16, 4, 40, 64, 1, 50, TRUE32, 30, 220, 2000, 100000, 0x5555, 0},

    /* KFN2G16Q2A */
    { 0x00EC, 0x0048, 2048, 1024, 1, 16, 4, 40, 64, 2, 50, TRUE32, 30, 220, 2000, 100000, 0x5555, 0},

    /* KFN4G16Q2M */
    { 0x00EC, 0x0058, 4096, 2048, 2, 16, 4, 80, 64, 2, 50, TRUE32, 30, 220, 2000, 100000, 0x5555, 0},

    /* KFM4G16Q2M */
    { 0x00EC, 0x0050, 4096, 4096, 2, 16, 4, 80, 64, 1, 50, TRUE32, 30, 220, 2000, 100000, 0x5555, 0},

    {      0,      0,    0,   0,  0,  0, 0,  0,  0, 0,  0, FALSE32, 0,   0,    0,      0, 0x0000, 0},
};

/*****************************************************************************/
/* Local function prototypes                                                 */
/*****************************************************************************/
PRIVATE VOID    _WriteMain        (volatile UINT8       *pDest,
                                            UINT8       *pSrc,
                                            UINT32       nSize);

PRIVATE VOID    _WriteSpare       (volatile UINT8       *pDest,
                                            FSRSpareBuf *pstSrc,
                                            UINT32       nFlag);

PRIVATE VOID    _ReadMain         (         UINT8       *pDest,
                                   volatile UINT8       *pSrc,
                                            UINT32       nSize,
                                            UINT8       *pTempBuffer);

PRIVATE INT32   _ReadSpare        (         FSRSpareBuf *pstDest,
                                   volatile UINT8       *pSrc,
                                            UINT32       nFlag);

PRIVATE INT32   _StrictChk        (UINT32       nDev,
                                   UINT32       nPbn,
                                   UINT32       nPgOffset);

PRIVATE INT32   _LockTightBlocks  (UINT32       nDev, 
                                   UINT32       nSbn,             /* Start block number   */
                                   UINT32       nBlks,            /* The number of blocks */
                                   UINT32      *pnErrPbn);

PRIVATE INT32   _LockBlocks       (UINT32       nDev, 
                                   UINT32       nSbn,             /* Start block number   */
                                   UINT32       nBlks,            /* The number of blocks */
                                   UINT32      *pnErrPbn);

PRIVATE INT32   _UnLockBlocks     (UINT32       nDev, 
                                   UINT32       nSbn,             /* Start block number   */
                                   UINT32       nBlks,            /* The number of blocks */
                                   UINT32      *pnErrPbn);

PRIVATE INT32   _GetUniqueID      (UINT32       nDev,
                                   OneNANDCxt  *pstONDCxt,
                                   UINT32       nFlag);           

PRIVATE VOID    _CopyToAnotherDataRAM   (UINT32       nDev,
                                         LLDCpBkArg  *pstCpArg);

PRIVATE VOID    _CalcTransferTime (UINT32       nDev, 
                                   UINT32      *pnWordRdCycle, 
                                   UINT32      *pnWordWrCycle);

PRIVATE BOOL32  _SearchUnlockedPbn(UINT32    nDev,
                                   UINT32    nDie,
                                   UINT16   *nPbn);

#if defined (FSR_LLD_STATISTICS)
PRIVATE VOID    _AddONDStat       (UINT32       nDev,
                                   UINT32       nDie,
                                   UINT32       nType,
                                   UINT32       nBytes,
                                   UINT32       nCmdOption);

#endif /* #if defined (FSR_LLD_STATISTICS) */

PRIVATE VOID    _DumpRegisters    (volatile OneNANDReg    *pstReg);
PRIVATE VOID    _DumpSpareBuffer  (volatile OneNANDReg    *pstReg,
                                            UINT32         nDev,
                                            UINT32         nDie);
PRIVATE VOID    _DumpCmdLog       (VOID);

#if defined (FSR_LLD_LOGGING_HISTORY)
PRIVATE VOID    _AddLog           (UINT32       nDev,
                                   UINT32       nDie);
#endif /* #if defined (FSR_LLD_LOGGING_HISTORY)                               */

/*****************************************************************************/
/* Code Implementation                                                       */
/*****************************************************************************/

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
                OneNANDCxt         *pstONDCxt       = NULL;
                OneNANDShMem       *pstONDShMem     = NULL;

                UINT32              nDev;

#if defined(FSR_LLD_LOGGING_HISTORY)
    volatile    OneNANDOpLog       *pstONDOpLog     = NULL;
                UINT32              nIdx;
                UINT32              nLogHead;
                UINT32              nPreOpIdx;
#endif

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG | FSR_DBZ_ERROR,
        (TEXT("[OND:IN ] ++%s()\r\n\r\n"), __FSR_FUNC__));

    for (nDev = 0; nDev < FSR_MAX_DEVS; nDev++)
    {
        pstONDCxt  = gpstONDCxt[nDev];

        if (pstONDCxt == NULL)
        {
            continue;
        }

        if (pstONDCxt->bOpen == FALSE32)
        {
            continue;
        }

        pstONDShMem = gpstONDShMem[nDev];

#if defined (FSR_LLD_LOGGING_HISTORY)
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("[OND:INF]   start printing nLog      : nDev[%d]\r\n"), nDev));

        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("%5s  %7s  %10s  %10s  %10s\r\n"), TEXT("nLog"),
            TEXT("preOp"), TEXT("prePbn"), TEXT("prePg"), TEXT("preFlag")));

        pstONDOpLog = &gstONDOpLog[nDev];

        nLogHead = pstONDOpLog->nLogHead;
        for (nIdx = 0; nIdx < FSR_OND_MAX_LOG; nIdx++)
        {
            nPreOpIdx = pstONDOpLog->nLogOp[nLogHead];
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("%5d| %7s, 0x%08x, 0x%08x, 0x%08x\r\n"),
                nLogHead, gpszLogPreOp[nPreOpIdx], pstONDOpLog->nLogPbn[nLogHead],
                pstONDOpLog->nLogPgOffset[nLogHead], pstONDOpLog->nLogFlag[nLogHead]));

            nLogHead = (nLogHead + 1) & (FSR_OND_MAX_LOG -1);
        }

        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("[OND:   ]   end   printing nLog      : nDev[%d]\r\n"), nDev));
#endif /* #if defined (FSR_LLD_LOGGING_HISTORY) */

        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("\r\n[OND:INF]   start printing OneNANDCxt: nDev[%d]\r\n"), nDev));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("            nPreOp           : [0x%08x, 0x%08x]\r\n"),
            pstONDShMem->nPreOp[0], pstONDShMem->nPreOp[1]));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("            nPreOpPbn        : [0x%08x, 0x%08x]\r\n"),
            pstONDShMem->nPreOpPbn[0], pstONDShMem->nPreOpPbn[1]));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("            nPreOpPgOffset   : [0x%08x, 0x%08x]\r\n"),
            pstONDShMem->nPreOpPgOffset[0], pstONDShMem->nPreOpPgOffset[1]));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("            nPreOpFlag       : [0x%08x, 0x%08x]\r\n"),
            pstONDShMem->nPreOpFlag[0], pstONDShMem->nPreOpFlag[1]));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("            end   printing OneNANDCxt: nDev[%d]\r\n\r\n"), nDev));

    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG | FSR_DBZ_ERROR, (TEXT("[OND:OUT] --%s()\r\n"), __FSR_FUNC__));
#endif /* #if !defined(FSR_OAM_RTLMSG_DISABLE) */
}

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
             OneNANDShMem *pstONDShMem;
    volatile OneNANDOpLog *pstONDOpLog;

    UINT32 nLogHead;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG ,
        (TEXT("[OND:IN ] ++%s(nDie: %d)\r\n"), __FSR_FUNC__, nDie));

    pstONDShMem = gpstONDShMem[nDev];

    pstONDOpLog = &gstONDOpLog[nDev];

    nLogHead = pstONDOpLog->nLogHead;

    pstONDOpLog->nLogOp       [nLogHead]  = pstONDShMem->nPreOp[nDie];
    pstONDOpLog->nLogPbn      [nLogHead]  = pstONDShMem->nPreOpPbn[nDie];
    pstONDOpLog->nLogPgOffset [nLogHead]  = pstONDShMem->nPreOpPgOffset[nDie];
    pstONDOpLog->nLogFlag     [nLogHead]  = pstONDShMem->nPreOpFlag[nDie];

    pstONDOpLog->nLogHead = (nLogHead + 1) & (FSR_OND_MAX_LOG -1);


    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s()\r\n"), __FSR_FUNC__));
}
#endif /* #if defined (FSR_LLD_LOGGING_HISTORY) */


/**
 * @brief          This function initializes OneNAND
 *
 * @param[in]      nFlag        : FSR_LLD_FLAG_NONE
 *
 * @return         FSR_LLD_SUCCESS
 * @return         FSR_LLD_INIT_FAILURE
 * @return         FSR_LLD_ALREADY_INITIALIZED
 *
 * @author         NamOh Hwang / JeongWook Moon
 * @version        1.0.0
 * @remark         constructioin and initialization of internal data structure
 *
 */
PUBLIC INT32
FSR_OND_Init(UINT32 nFlag)
{
            FsrVolParm       astPAParm[FSR_MAX_VOLS];
            FSRLowFuncTbl    astLFT[FSR_MAX_VOLS];
            FSRLowFuncTbl   *apstLFT[FSR_MAX_VOLS];
            OneNANDShMem    *pstONDShMem;

    PRIVATE BOOL32           nInitFlg = FALSE32;
            UINT32           nMemoryChunkID;
            UINT32           nPDev;
            UINT32           nVol;
            UINT32           nMemAllocType;
            UINT32           nIdx;
            UINT32           nSharedMemoryUseCnt;
            UINT32           nDie;
            
            INT32            nLLDRe   = FSR_LLD_SUCCESS;
            INT32            nPAMRe   = FSR_PAM_SUCCESS;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:IN ] ++%s(nFlag:0x%08x)\r\n"), __FSR_FUNC__, nFlag));
    do
    {
        if (nInitFlg == TRUE32)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
                (TEXT("[OND:INF]   %s(nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
                (TEXT("            LLD is already initialized\r\n")));

            nLLDRe = FSR_LLD_ALREADY_INITIALIZED;
            break;
        }

        /* Local data structure */
        for (nPDev = 0; nPDev < FSR_OND_MAX_DEVS; nPDev++)
        {
            gpstONDCxt[nPDev] = NULL;
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
             * FSR_OND_Init() only initializes shared memory of 
             * corresponding volume
             */
            if ((apstLFT[nVol] == NULL) || apstLFT[nVol]->LLD_Init != FSR_OND_Init)
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

                pstONDShMem = NULL;

#if !defined(WITH_TINY_FSR)
                gpstONDShMem[nPDev] = (OneNANDShMem *) NULL;
#endif

                if (gpstONDShMem[nPDev] == NULL)
                {

                    pstONDShMem = (OneNANDShMem *) FSR_OAM_MallocExt(nMemoryChunkID,
                                                                     sizeof(OneNANDShMem),
                                                                     nMemAllocType);

                    if (pstONDShMem == NULL)
                    {
                        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                            (TEXT("[OND:ERR]   %s(nFlag:0x%08x) / %d line\r\n"),
                            __FSR_FUNC__, nFlag, __LINE__));

                        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                            (TEXT("            pstONDShMem is NULL\r\n")));

                        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                            (TEXT("            malloc failed!\r\n")));

                        nLLDRe = FSR_LLD_MALLOC_FAIL;
                        break;
                    }

                    if (((UINT32) pstONDShMem & (0x03)) != 0)
                    {
                        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                            (TEXT("[OND:ERR]   %s(nFlag:0x%08x) / %d line\r\n"),
                            __FSR_FUNC__, nFlag, __LINE__));

                        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                            (TEXT("            pstONDShMem is misaligned:0x%08x\r\n"),
                            pstONDShMem));

                        nLLDRe = FSR_OAM_NOT_ALIGNED_MEMPTR;
                        break;
                    }

                    gpstONDShMem[nPDev] = pstONDShMem;
                    nSharedMemoryUseCnt = pstONDShMem->nShMemUseCnt;

                    /* PreOp init of single process */
                    if (astPAParm[nVol].bProcessorSynchronization == FALSE32)
                    {
                       /* Initialize shared memory used by LLD */
                        for (nDie = 0; nDie < FSR_MAX_DIES; nDie++)
                        {
                            pstONDShMem->nPreOp[nDie]          = FSR_OND_PREOP_NONE;
                            pstONDShMem->nPreOpPbn[nDie]       = FSR_OND_PREOP_ADDRESS_NONE;
                            pstONDShMem->nPreOpPgOffset[nDie]  = FSR_OND_PREOP_ADDRESS_NONE;
                            pstONDShMem->nPreOpFlag[nDie]      = FSR_OND_PREOP_FLAG_NONE;
                        }
                    }
                    /* PreOp init of dual process */
                    else
                    {
                        if ((nSharedMemoryUseCnt ==0) ||
                            (nSharedMemoryUseCnt == astPAParm[nVol].nSharedMemoryInitCycle))
                        {
                            pstONDShMem->nShMemUseCnt = 0;

                            /* Initialize shared memory used by LLD */
                            for (nDie = 0; nDie < FSR_MAX_DIES; nDie++)
                            {
                                pstONDShMem->nPreOp[nDie]          = FSR_OND_PREOP_NONE;
                                pstONDShMem->nPreOpPbn[nDie]       = FSR_OND_PREOP_ADDRESS_NONE;
                                pstONDShMem->nPreOpPgOffset[nDie]  = FSR_OND_PREOP_ADDRESS_NONE;
                                pstONDShMem->nPreOpFlag[nDie]      = FSR_OND_PREOP_FLAG_NONE;
                            }
                        }
                    }

                    pstONDShMem->nShMemUseCnt++;
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

        nInitFlg = TRUE32;
    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return (nLLDRe);
}


/**
 * @brief          This function opens OneNAND device driver
 *
 * @param[in]      nDev         : Physical Device Number (0 ~ 7)
 * @param[in]      pParam       : pointer to structure for configuration
 * @param[in]      nFlag        : FSR_LLD_FLAG_NONE
 *
 * @return         FSR_LLD_SUCCESS  
 * @return         FSR_LLD_OPEN_FAILURE
 * @return         FSR_LLD_INVALID_PARAM
 *
 * @author         NamOh Hwang / JeongWook Moon
 * @version        1.0.0
 * @remark
 *
 */
PUBLIC INT32
FSR_OND_Open(UINT32  nDev,
             VOID   *pParam,
             UINT32  nFlag)
{
             OneNANDCxt        *pstONDCxt;
             OneNANDSpec       *pstONDSpec;
    volatile OneNANDReg        *pstFOReg;
             FsrVolParm        *pstParm;
             UINT32             nCnt;
             UINT32             nIdx;
             UINT32             nDie;             
             INT32              nLLDRe              = FSR_LLD_SUCCESS;
             UINT32             nRdTransferTime;
             UINT32             nWrTransferTime;
             UINT16             nMID;
             UINT16             nDID;

#if !defined(FSR_OAM_RTLMSG_DISABLE)
             UINT16             nVID;
#endif
             UINT16             nDID2               = 0;
             UINT16             nShifted;
             UINT16             nNumOfBlksIn2ndDie;
             UINT32             nMemoryChunkID;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG, 
        (TEXT("[OND:IN ] ++%s(nDev:%d,nFlag:0x%08x)\r\n"), 
        __FSR_FUNC__, nFlag));

    do
    {

#if defined (FSR_LLD_STRICT_CHK)
        /* Check device number */
        if (nDev >= FSR_OND_MAX_DEVS)
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
            FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, 
                           (TEXT("[OND:IN ]   (OPEN)pParam is NULL\r\n")));
            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }

        pstParm     = (FsrVolParm *) pParam;
        pstONDCxt   = gpstONDCxt[nDev];

        if (pstONDCxt == NULL)
        {
            nMemoryChunkID = nDev / (FSR_MAX_DEVS / FSR_MAX_VOLS);

            pstONDCxt = (OneNANDCxt *)
                FSR_OAM_MallocExt(nMemoryChunkID, sizeof(OneNANDCxt), FSR_OAM_LOCAL_MEM);

            if (pstONDCxt == NULL)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[OND:ERR]   %s(nDev:%d, pParam:0x%08x, nFlag:0x%08x) / %d line\r\n"),
                    __FSR_FUNC__, nDev, pParam, nFlag, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            pstONDCxt is NULL\r\n")));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            malloc failed!\r\n")));

                nLLDRe = FSR_LLD_MALLOC_FAIL;
                break;
            }

            /* If the pointer has not 4-byte align address, return not-aligned pointer. */
            if (((UINT32) gpstONDCxt[nDev] & (sizeof(UINT32) - 1)) != 0)
            {
                FSR_DBZ_DBGMOUT(FSR_DBZ_ERROR, 
                                   (TEXT("[OND:ERR]   gpstONDCxt[%d] is not aligned by 4-bytes\r\n"), nDev));
                nLLDRe = FSR_OAM_NOT_ALIGNED_MEMPTR;
                break;
            }

            gpstONDCxt[nDev] = pstONDCxt;

            FSR_OAM_MEMSET(pstONDCxt, 0x00, sizeof(OneNANDCxt));
        }

#if defined(FSR_LLD_STATISTICS)
        gpstONDStat[nDev] = (OneNANDStat *) FSR_OAM_Malloc(sizeof(OneNANDStat));

        if (gpstONDStat[nDev] == NULL)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s(nDev:%d, pParam:0x%08x, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, pParam, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            gpstONDStat[%d] is NULL\r\n"), nDev));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            malloc failed!\r\n")));

            nLLDRe = FSR_LLD_MALLOC_FAIL;
            break;
        }

        /* If the pointer has not 4-byte align address, return not-aligned pointer. */
        if (((UINT32) gpstONDStat[nDev] & (sizeof(UINT32) - 1)) != 0)
        {
            FSR_DBZ_DBGMOUT(FSR_DBZ_ERROR, 
                               (TEXT("[OND:ERR]   gpstONDStat[%d] is not aligned by 4-bytes\r\n"), nDev));
            nLLDRe = FSR_OAM_NOT_ALIGNED_MEMPTR;
            break;
        }

        FSR_OAM_MEMSET(gpstONDStat[nDev], 0x00, sizeof(OneNANDStat));
#endif

        if (pstONDCxt->bOpen == TRUE32)
        {
            FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, 
                           (TEXT("[OND:IN ]   (OPEN)dev:%d already opened\r\n"), nDev));
            nLLDRe = FSR_LLD_ALREADY_OPEN;
            break;
        }

        /* Base address setting */
        nIdx = nDev & (FSR_MAX_DEVS / FSR_MAX_VOLS -1);

        if (pstParm->nBaseAddr[nIdx] != FSR_PAM_NOT_MAPPED)
        {
            pstONDCxt->nBaseAddr = pstParm->nBaseAddr[nIdx];

            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
                (TEXT("[OND:INF]   %s(nDev:%d, pParam:0x%08x, nFlag:0x%08x)\r\n"),
                __FSR_FUNC__, nDev, pParam, nFlag));

            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
                (TEXT("            pstONDCxt->nBaseAddr: 0x%08x\r\n"),
                pstONDCxt->nBaseAddr));
        }
        else
        {
            pstONDCxt->nBaseAddr = FSR_PAM_NOT_MAPPED;

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s(nDev:%d, pParam:0x%08x, nFlag:0x%08x / %d line\r\n"),
                __FSR_FUNC__, nDev, pParam, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            OneNAND is not mapped\r\n")));
            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }

        pstFOReg = (volatile OneNANDReg *) pstONDCxt->nBaseAddr;

        /* Set DBS to die 0 */
        OND_WRITE(pstFOReg->nStartAddr2, (UINT16) 0x00);

        nMID = OND_READ(pstFOReg->nMID);
        nDID = OND_READ(pstFOReg->nDID);

#if !defined(FSR_OAM_RTLMSG_DISABLE)
        nVID = OND_READ(pstFOReg->nVerID);
#endif

        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
            (TEXT("            nDev=%d, nMID=0x%04x, nDID=0x%04x, nVID=0x%04x\r\n"),
            nDev, nMID, nDID, nVID));
        
        pstONDCxt->pDataBuf4CpBk = NULL;
        pstONDCxt->pTempBuffer   = NULL;

        if ((nDID & FSR_OND_DDP_CHIP) != 0)
        {
            /* Set DBS to die 1 */
            OND_WRITE(pstFOReg->nStartAddr2, (UINT16) FSR_OND_MASK_DBS);

            nDID2 = OND_READ(pstFOReg->nDID);

            /* Check if this device is 3G DDP */
            if ((nDID == FSR_OND_4G_DDP_DID) && (nDID2 == FSR_OND_2G_DDP_DID))
            {
                nDID                    = (FSR_OND_4G_DDP_DID + FSR_OND_2G_DDP_DID) / 2;
                nMemoryChunkID          = nDev / (FSR_MAX_DEVS / FSR_MAX_VOLS);
                pstONDCxt->pDataBuf4CpBk = (UINT8 *) FSR_OAM_MallocExt(nMemoryChunkID,
                                                                       (FSR_OND_SECTOR_SIZE + FSR_OND_SPARE_SIZE) * 8,
                                                                       FSR_OAM_LOCAL_MEM);

                /* If the pointer has not 4-byte align address, return not-aligned pointer. */
                if (((UINT32) pstONDCxt->pDataBuf4CpBk & (sizeof(UINT32) - 1)) != 0)
                {
                    FSR_DBZ_DBGMOUT(FSR_DBZ_ERROR, 
                           (TEXT("[OND:ERR]   pDataBuf4CpBk is not aligned by 4-bytes\r\n")));
                    return FSR_OAM_NOT_ALIGNED_MEMPTR;
                }
            }

            /* Set DBS to die 0*/
            OND_WRITE(pstFOReg->nStartAddr2, (UINT16) 0x0000);
        }
        
        /* Find current device */
        for (nCnt = 0; gstONDSpec[nCnt].nMID != 0; nCnt++)
        {
            if ((nMID == gstONDSpec[nCnt].nMID) &&
                ((nDID & FSR_OND_MASK_DID) == (gstONDSpec[nCnt].nDID & FSR_OND_MASK_DID)))
            {                
                pstONDCxt->pstONDSpec = (OneNANDSpec *) &gstONDSpec[nCnt];
                break;
            }
            /* else continue */
        }

        if (pstONDCxt->pstONDSpec == NULL)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s(nDev:%d, pParam:0x%08x, nFlag:0x%08x / %d line\r\n"),
                __FSR_FUNC__, nDev, pParam, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Unknown Device\r\n")));

            nLLDRe = FSR_LLD_OPEN_FAILURE;
            break;
        }

        pstONDSpec = pstONDCxt->pstONDSpec;

        /*
         * Start Address8 Register 
         *     | 15 | 14 | 13 | 12 | 11 | 10 |  9 |  8 |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
         *     |=======================================|=============================|=========|
         *     |            Reserved                   |          FPA                |   FSA   |
         */
        pstONDCxt->nFPASelSft = FSR_OND_FPA_SHIFT;
        pstONDCxt->nFSAMask   = FSR_OND_FSA_MASK;

        /*
         * Start Buffer Register 
         *     | 15 | 14 | 13 | 12 | 11 | 10 |  9 |  8 |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
         *     |===================|===================|=============================|=========|
         *     |    Reserved       |       BSA         |        Reserved             |   BSC   |
         */
        pstONDCxt->nBSASelSft = FSR_OND_BSA_SHIFT;

        /* 
         * The mask of Flash Block Address : all bit is set to '1'
         * If each device has different number of blocks in case of DDP, 
         * you shoud be considered condition about FBA mask.
         */
        nNumOfBlksIn2ndDie = pstONDSpec->nNumOfBlks - pstONDSpec->nNumOfBlksIn1stDie;

        if ((nNumOfBlksIn2ndDie != 0) &&
            (pstONDSpec->nNumOfBlksIn1stDie != nNumOfBlksIn2ndDie))
        {
            pstONDCxt->nFBAMask   = pstONDSpec->nNumOfBlksIn1stDie - 1;
        }
        else
        {
            pstONDCxt->nFBAMask   = (pstONDSpec->nNumOfBlks / pstONDSpec->nNumOfDies) - 1;
        }

        /* Calculate nDDPSelSft */
        pstONDCxt->nDDPSelSft   = 0;
        nShifted                = pstONDCxt->nFBAMask << 1;
        while((nShifted & FSR_OND_MASK_DFS) != FSR_OND_MASK_DFS)
        {
            pstONDCxt->nDDPSelSft++;
            nShifted <<= 1;
        }

        /* Read PI Allocation Info */
        for (nDie = 0; nDie < pstONDSpec->nNumOfDies; nDie++)
        {
            pstONDCxt->nBlksForSLCArea[nDie] = pstONDSpec->nNumOfBlksIn1stDie;

            if (nDie != 0)
            {
                pstONDCxt->nBlksForSLCArea[nDie] = nNumOfBlksIn2ndDie;
            }

            pstONDCxt->nPreUseBuf[nDie] = 0;
        }

        /* 
         * HOT-RESET initializes the value of register file, which includes 
         * system configuration 1 register.
         * for device to function properly, save the value of system
         * configuration 1 register at open time & restore it after HOT-RESET
         */
        pstONDCxt->nSysConf1       =  OND_READ(pstFOReg->nSysConf1);

        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
            (TEXT("[OND:INF]   pstONDCxt->nSysConf1:0x%04x / %d line\r\n"),
            pstONDCxt->nSysConf1, __LINE__));

        nMemoryChunkID          = nDev / (FSR_MAX_DEVS / FSR_MAX_VOLS);
        pstONDCxt->pTempBuffer  =
            (UINT8 *) FSR_OAM_MallocExt(nMemoryChunkID, 
                                        pstONDSpec->nSctsPerPG * (FSR_SECTOR_SIZE + FSR_SPARE_SIZE),
                                        FSR_OAM_LOCAL_MEM);

        /* If the pointer has not 4-byte align address, return not-aligned pointer. */
        if (((UINT32) pstONDCxt->pTempBuffer & (sizeof(UINT32) - 1)) != 0)
        {
            FSR_DBZ_DBGMOUT(FSR_DBZ_ERROR, 
                   (TEXT("[OND:ERR]   pTempBuffer is not aligned by 4-bytes\r\n")));
            return FSR_OAM_NOT_ALIGNED_MEMPTR;
        }

        if (pstONDCxt->pTempBuffer == NULL)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s(nDev:%d, pParam:0x%08x, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, pParam, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            malloc failed!\r\n")));

            nLLDRe = FSR_LLD_MALLOC_FAIL;
            break;
        }

        /* Time for transfering 16 bits between host & DataRAM of OneNAND */
        _CalcTransferTime(nDev, &nRdTransferTime, &nWrTransferTime);

        pstONDCxt->nRdTranferTime  = nRdTransferTime; /* nano second base */
        pstONDCxt->nWrTranferTime  = nWrTransferTime; /* nano second base */

        pstONDCxt->nIntID          = pstParm->nIntID[nIdx];

        if ((pstONDCxt->nIntID != FSR_INT_ID_NONE) && (pstONDCxt->nIntID > FSR_INT_ID_NAND_7))
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:INF]   nIntID is out of range(nIntID:%d)\r\n"), pstONDCxt->nIntID));
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:INF]   FSR_INT_ID_NAND_0(%d) <= nIntID <= FSR_INT_ID_NAND_7(%d)\r\n"), 
                FSR_INT_ID_NAND_0, FSR_INT_ID_NAND_7));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }

        /* Init UID Buffer */
        FSR_OAM_MEMSET(pstONDCxt->nUID, 0xFF, FSR_LLD_UID_SIZE);

        FSR_OAM_MEMSET(pstONDCxt->pTempBuffer, 0xFF, pstONDSpec->nSctsPerPG * FSR_SECTOR_SIZE);

        pstONDCxt->bOpen      = TRUE32;

#if defined (FSR_LLD_STATISTICS)
        gnDevsInVol[nDev / (FSR_MAX_DEVS / FSR_MAX_VOLS)]++;
#endif

    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return (nLLDRe);
}

/**
 * @brief          This function closes OneNAND device driver
 *
 * @param[in]      nDev         : Physical Device Number (0 ~ 7)
 * @param[in]      nFlag        : FSR_LLD_FLAG_NONE
 *
 * @return         FSR_LLD_SUCCESS
 * @return         FSR_LLD_INVALID_PARAM
 *
 * @author         NamOh Hwang / JeongWook Moon
 * @version        1.0.0
 * @remark
 *
 */
PUBLIC INT32
FSR_OND_Close(UINT32 nDev,
              UINT32 nFlag)
{
    OneNANDCxt       *pstONDCxt;
    UINT32            nMemoryChunkID;

#if defined (FSR_LLD_STATISTICS)
    UINT32            nVol;
#endif

    INT32             nLLDRe     = FSR_LLD_SUCCESS;

    FSR_STACK_VAR;

    FSR_STACK_END;
    
    FSR_ASSERT(nFlag == FSR_LLD_FLAG_NONE);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG, 
        (TEXT("[OND:IN ] ++%s(nDev:%d,nFlag:0x%08x)\r\n"), __FSR_FUNC__, nDev, nFlag));

    /* Here LLD doesn't flush the previous operation, for BML flushes */
    do
    {

#if defined (FSR_LLD_STRICT_CHK)
        /* Check device number */
        if (nDev >= FSR_OND_MAX_DEVS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR , 
                           (TEXT("[OND:ERR]   Invalid Device Number (nDev = %d)\r\n"), nDev));
            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */

        pstONDCxt  = gpstONDCxt[nDev];

        if (pstONDCxt != NULL)
        {
            pstONDCxt->bOpen      = FALSE32;
            pstONDCxt->pstONDSpec = NULL;

            nMemoryChunkID = nDev / (FSR_MAX_DEVS / FSR_MAX_VOLS);

            if (pstONDCxt->pTempBuffer != NULL)
            {            
                FSR_OAM_FreeExt(nMemoryChunkID, gpstONDCxt[nDev]->pTempBuffer, FSR_OAM_LOCAL_MEM);
                pstONDCxt->pTempBuffer = NULL;
            }

            if (pstONDCxt->pDataBuf4CpBk != NULL)
            {            
                FSR_OAM_FreeExt(nMemoryChunkID, gpstONDCxt[nDev]->pDataBuf4CpBk, FSR_OAM_LOCAL_MEM);
                pstONDCxt->pDataBuf4CpBk = NULL;
            }

            FSR_OAM_FreeExt(nMemoryChunkID, pstONDCxt, FSR_OAM_LOCAL_MEM);
            gpstONDCxt[nDev] = NULL;

#if defined (FSR_LLD_STATISTICS)
            nVol = nDev / (FSR_MAX_DEVS / FSR_MAX_VOLS);
            gnDevsInVol[nVol]--;
#endif

        }

#if defined(FSR_LLD_STATISTICS)
        if (gpstONDStat[nDev] != NULL)
        {
            FSR_OAM_Free(gpstONDStat[nDev]);
            gpstONDStat[nDev] = NULL;
        }
#endif

    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return (nLLDRe);
}

/**
 * @brief          This function writes data into main area of DataRAM of NAND flash
 *
 * @param[in]      pDest        : pointer to the main area of DataRAM
 * @param[in]      pSrc         : pointer to the buffer
 * @param[in]      nSize        : the number of bytes to write
 *
 * @return          VOID
 *
 * @author          NamOh Hwang
 * @version         1.0.0
 * @remark
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

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[OND:IN ] ++%s\r\n"), __FSR_FUNC__));
    
    pTgt16 = (volatile UINT16 *) pDest;

    if ((((UINT32) pSrc & 0x01) != 0) || ((nSize & 0x3) != 0))
    {
        /* Copy it by word, so loop it for nSize/2 */
        nSize  = nSize >> 1;
        for (nCnt = 0; nCnt < nSize; nCnt++)
        {              
            nReg   = *pSrc++;
            nReg  |= *pSrc++ << 8;
            OND_WRITE(*pTgt16++, nReg);
        }
    }
    else
    {
        /* nSize is the number of bytes to transfer */
        TRANSFER_TO_NAND(pDest, pSrc, nSize);
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[OND:OUT] --%s\r\n"), __FSR_FUNC__));
}


/**
 * @brief           This function writes FSRSpareBuf into spare area of DataRAM
 *
 * @param[in]       pDest        : pointer to the spare area of DataRAM
 * @param[in]       pstSrc       : pointer to the structure, FSRSpareBuf
 * @param[in]       nFlag        : Operation options such as ECC_ON, OFF
 *
 * @return          VOID
 *
 * @author          NamOh Hwang / JeongWook Moon
 * @version         1.0.0
 * @remark          
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
             UINT8    anSpareBuf[FSR_LLD_SPARE_BUF_SIZE_2KB_PAGE];

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[OND:IN ] ++%s\r\n"), __FSR_FUNC__));

    /* When pstSrc->nNumOfMetaExt is 0, fill the rest of spare area with 0xFF */
    FSR_OAM_MEMSET(&anSpareBuf[0], 0xFF, sizeof(anSpareBuf));

    /* Spare buffer extension size is 8B in 2K page */
    if (nFlag & FSR_LLD_FLAG_USE_SPAREBUF)
    {
        FSR_OAM_MEMCPY(&anSpareBuf[0], pstSrc->pstSpareBufBase, FSR_SPARE_BUF_BASE_SIZE);

        FSR_ASSERT(pstSrc->nNumOfMetaExt < 2);

        for (nMetaIdx = 0; nMetaIdx < pstSrc->nNumOfMetaExt; nMetaIdx++)
        {
            FSR_OAM_MEMCPY(&anSpareBuf[FSR_SPARE_BUF_BASE_SIZE + nMetaIdx * FSR_SPARE_BUF_EXT_SIZE],
                           &pstSrc->pstSTLMetaExt[nMetaIdx],
                           (FSR_SPARE_BUF_EXT_SIZE >> 1));
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

    pSrc16 = (UINT16 *) &anSpareBuf[0];
    pTgt16 = (volatile UINT16 *) pDest;

    FSR_ASSERT(((UINT32) pSrc16 & 0x01) == 0);

    /* If nNumofMetaExt value is 2, idx should be 4. because loop should operate 4 times. */
    nTwoSctIdx = (FSR_LLD_SPARE_BUF_SIZE_2KB_PAGE / FSR_OND_SPARE_USER_AREA) >> 1;

    do
    {
        nSctIdx = (FSR_LLD_SPARE_BUF_SIZE_2KB_PAGE / FSR_OND_SPARE_USER_AREA) >> 1;

        /* ECC generation */
        if (bEccOn == TRUE32) 
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
            OND_WRITE(*pTgt16++, *pSrc16++);
            OND_WRITE(*pTgt16++, *pSrc16++);
            OND_WRITE(*pTgt16++, *pSrc16++);

            if (bEccOn == TRUE32)
            {
                /* 
                 * You must access the NAND array by unit of word.
                 * maybe not, it should be fallen in error for exception.
                 * Hardware Ecc area uses 5 bytes space, for if you want to 
                 * use software Ecc area by next byte, it occurs 2 times to read
                 * NAND array about MSB and LSB area.
                 */               
                if (nSctIdx == 2)
                {                    
                    nEcc_MSB = OND_READ(*pTgt16);
                    nEcc_MSB = (nEcc_MSB & 0xFF00) | (nGenValue & 0x00FF);
                    OND_WRITE(*pTgt16++,       nEcc_MSB);
                }
                else
                {
                    nEcc_LSB = OND_READ(*pTgt16); 
                    nEcc_LSB = (nEcc_LSB & 0xFF00) | ((nGenValue >> 8) & 0x00FF);
                    OND_WRITE(*pTgt16++,       nEcc_LSB);
                }

                /* Set address to location for SWEcc area */
                pTgt16 += FSR_OND_SPARE_HW_ECC_AREA - 1;
            }
            else /* if (bEccOn == TRUE32) */
            {
                pTgt16 += FSR_OND_SPARE_HW_ECC_AREA;
            }
        }while (--nSctIdx > 0);

    } while (--nTwoSctIdx > 0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[OND:OUT] --%s\r\n"), __FSR_FUNC__));
}



PRIVATE VOID
_GenSWECCOfDumpSpare(UINT8 *pSBuf, UINT32 nNumOfSct)
{
    UINT32  nECCDataIdx     = 0;
    UINT8   nECCData[8];
    UINT16  nSWECC;
    UINT32  nSctIdx;    

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[OND:IN ] ++%s\r\n"), __FSR_FUNC__));

    for (nSctIdx = 0; nSctIdx < nNumOfSct; nSctIdx++)
    {
        /* Get data in 0, 1, 4, 5th byte */
        nECCData[nECCDataIdx++] = pSBuf[nSctIdx * FSR_SPARE_SIZE + 0];
        nECCData[nECCDataIdx++] = pSBuf[nSctIdx * FSR_SPARE_SIZE + 1];
        nECCData[nECCDataIdx++] = pSBuf[nSctIdx * FSR_SPARE_SIZE + 4];
        nECCData[nECCDataIdx++] = pSBuf[nSctIdx * FSR_SPARE_SIZE + 5];        

        /* 1 SW ECC is generated every 2 sectors */
        if ((nSctIdx & 0x1) == 0x1)
        {
            FSR_OND_ECC_GenS(&nSWECC, &nECCData[0]);
            pSBuf[(nSctIdx - 1) * FSR_SPARE_SIZE + 6]   = (UINT8) (nSWECC & 0x00FF);
            pSBuf[nSctIdx * FSR_SPARE_SIZE + 6]         = (UINT8) ((nSWECC >> 8) & 0x00FF);

            nECCDataIdx = 0 ;
        }
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[OND:OUT] --%s\r\n"), __FSR_FUNC__));
}

/**
 * @brief          This function writes data into NAND flash by dual buffering
 *
 * @param[in]      nDev         : Physical Device Number (0 ~ 7)
 * @param[in]      nPbn         : Physical Block  Number
 * @param[in]      nPgOffset    : Page Offset within a block
 * @param[in]      pMBuf        : Memory buffer for main  array of NAND flash
 * @param[in]      pSBuf        : Memory buffer for spare array of NAND flash
 * @param[in]      nFlag        : Operation options such as ECC_ON, OFF
 *
 * @return         FSR_LLD_SUCCESS
 * @return         FSR_LLD_INVALID_PARAM
 * @return         FSR_LLD_PREV_WRITE_ERROR  |
 *                 {FSR_LLD_1STPLN_PREV_ERROR | FSR_LLD_2NDPLN_PREV_ERROR}
 * @n              previous write error return value
 * @return         FSR_LLD_WR_PROTECT_ERROR |
 *                 {FSR_LLD_1STPLN_CURR_ERROR | FSR_LLD_2NDPLN_CURR_ERROR}
 * @n              write protetion error return value
 * @return         FSR_LLD_UNLOCKED_BLK_NOT_EXIST
 * @n              return value not to be able to find unlocked pbn for OTP program
 *
 * @author          NamOh Hwang / JeongWook Moon / Jinhyuck Kim
 * @version         1.2.0
 * @remark          
 *
 */
PUBLIC INT32
FSR_OND_Write(UINT32       nDev,
              UINT32       nPbn,
              UINT32       nPgOffset,
              UINT8       *pMBuf,
              FSRSpareBuf *pSBuf,
              UINT32       nFlag)
{
             OneNANDCxt       *pstONDCxt;
             OneNANDShMem     *pstONDShMem;             
             OneNANDSpec      *pstONDSpec;
    volatile OneNANDReg       *pstFOReg;             
             UINT32            nCmdIdx;
             UINT32            nDie;
             UINT32            nBBMMetaIdx;
             UINT32            nBadMarkIdx;

#if defined (FSR_LLD_STATISTICS)
             UINT32            nBytesTrasferred     = 0;
#endif /* #if defined (FSR_LLD_STATISTICS) */

             UINT32            nPairedPbn;
             INT32             nLLDRe               = FSR_LLD_SUCCESS;
             UINT16            nBSA                 = 0;

             /* For double buffering */
             UINT32            nPreUseBuf           = 0;
             FSRSpareBuf       stTSBuf;             
             UINT32            nSysConf1;
             UINT16            nWrProtectStat;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG, 
                    (TEXT("[OND:IN ] ++%s(nDev:%d, nPbn:%d, nPgOffset:%d, nFlag:0x%08x)\r\n"), 
                    __FSR_FUNC__, nDev, nPbn, nPgOffset, nFlag));

    do
    {

#if defined (FSR_LLD_STRICT_CHK)
        if ((nLLDRe = _StrictChk(nDev, nPbn, nPgOffset)) != FSR_LLD_SUCCESS)
        {
            break;
        }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */

        pstONDCxt   = gpstONDCxt[nDev];
        pstONDShMem = gpstONDShMem[nDev];
        pstONDSpec  = pstONDCxt->pstONDSpec;
        pstFOReg    = (volatile OneNANDReg *) pstONDCxt->nBaseAddr;

        nCmdIdx = (nFlag & FSR_LLD_FLAG_CMDIDX_MASK) >> FSR_LLD_FLAG_CMDIDX_BASEBIT;

        /* Check if this request is OTP Program */
        if (nCmdIdx == FSR_LLD_FLAG_OTP_PROGRAM)
        {
            /* PBN = (unlocked PBN) */
            nPbn = pstONDCxt->nUnlockedPbn;

            if(nPbn == FSR_OND_NULL_VBN)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_INF,
                        (TEXT("[OND:ERR]   %s() / %d line\r\n"),
                        __FSR_FUNC__, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_INF,
                    (TEXT("            Cannot find unlocked pbn in dev (%d).\r\n \
                                       OTP program in 2k page OneNAND requires the pbn of an unlocked block.\r\n"), 
                     nDev));

                nLLDRe = FSR_LLD_UNLOCKED_BLK_NOT_EXIST;
                break;
            }
        }

        nDie = (nPbn >> (FSR_OND_DFS_BASEBIT - pstONDCxt->nDDPSelSft)) & 0x1;

        /* Set DBS                         */
        OND_WRITE(pstFOReg->nStartAddr2, 
                  ((UINT16) nPbn << pstONDCxt->nDDPSelSft) & FSR_OND_MASK_DBS);

        /* If 2x operation goes on, interrupt bit should be set after write program is over */
        if (((nCmdIdx & FSR_LLD_FLAG_CMDIDX_MASK) == FSR_LLD_FLAG_2X_PROGRAM) || 
            ((nCmdIdx & FSR_LLD_FLAG_CMDIDX_MASK) == FSR_LLD_FLAG_2X_CACHEPGM))
        {
            nLLDRe = FSR_OND_FlushOp(nDev,
                                     nDie,
                                     nFlag);
            if (nLLDRe != FSR_LLD_SUCCESS)
            {
                break;
            }

            /* Reset nPreUseBuf to point start block address of DataRAM */
            pstONDCxt->nPreUseBuf[nDie] = 0;
        }

        /*
         * < nPreUseBuf decide previous used DataRAM for each die >
         * Double buffering makes a confusing condition to used DataRAM.
         * So, you have to make a decision which buffer uses for current data and
         * used for previous data.
         * ##This buffer is going to use for current buffer##
         */
        nPreUseBuf = pstONDCxt->nPreUseBuf[nDie];

        /* Check whether source buffer is empty or not */
        if (pMBuf != NULL)
        {
            /* When write program is under way, BSA to DataRAM is always set to 1000 */
            _WriteMain(&(pstFOReg->nDataMB00[0]) + 
                        (nPreUseBuf & 1) * 
                        FSR_OND_MDRAM0_SIZE, 
                       pMBuf, 
                       pstONDSpec->nSctsPerPG * FSR_OND_SECTOR_SIZE);

#if defined (FSR_LLD_STATISTICS)
            nBytesTrasferred += pstONDSpec->nSctsPerPG * FSR_OND_SECTOR_SIZE;
#endif /* #if defined (FSR_LLD_STATISTICS) */

            /* Consider about 2xPGM */
            if (((nCmdIdx & FSR_LLD_FLAG_CMDIDX_MASK) == FSR_LLD_FLAG_2X_PROGRAM) || 
                ((nCmdIdx & FSR_LLD_FLAG_CMDIDX_MASK) == FSR_LLD_FLAG_2X_CACHEPGM))
            {
                /* Use second buffer - DataRAM10 */
                _WriteMain(&(pstFOReg->nDataMB00[0]) + 
                            ((nPreUseBuf + 1) & 1) * 
                            FSR_OND_MDRAM0_SIZE, 
                           pMBuf + pstONDSpec->nSctsPerPG * FSR_OND_SECTOR_SIZE, 
                           pstONDSpec->nSctsPerPG * FSR_OND_SECTOR_SIZE);

#if defined (FSR_LLD_STATISTICS)
                nBytesTrasferred += pstONDSpec->nSctsPerPG * FSR_OND_SECTOR_SIZE;
#endif /* #if defined (FSR_LLD_STATISTICS) */
            }
        }

        if ((pMBuf == NULL) && (pSBuf != NULL))
        {
            if((nFlag & FSR_LLD_FLAG_BACKUP_DATA) != FSR_LLD_FLAG_BACKUP_DATA)
            {
                /* Write Main DataRAM */
                FSR_OAM_MEMSET(pstONDCxt->pTempBuffer, 0xFF, pstONDSpec->nSctsPerPG * FSR_SECTOR_SIZE);
                
                TRANSFER_TO_NAND((void *)(&(pstFOReg->nDataMB00[0]) + 
                                 (nPreUseBuf & 1) * FSR_OND_MDRAM0_SIZE),
                                 pstONDCxt->pTempBuffer,
                                 sizeof(pstFOReg->nDataMB00) * pstONDSpec->nSctsPerPG);
                                    
                /* Consider about 2xPGM */
                if (((nCmdIdx & FSR_LLD_FLAG_CMDIDX_MASK) == FSR_LLD_FLAG_2X_PROGRAM) || 
                    ((nCmdIdx & FSR_LLD_FLAG_CMDIDX_MASK) == FSR_LLD_FLAG_2X_CACHEPGM))
                {
                    /*
                     *  When 2xPGM is accessed, spare buffer is written only one area.
                     *  So if you do not want to fail operation, 
                     *  you will write same data into paired page.
                     */                     
                     TRANSFER_TO_NAND((void *)(&(pstFOReg->nDataMB00[0]) + 
                                      ((nPreUseBuf + 1) & 1) * FSR_OND_MDRAM0_SIZE),
                                      pstONDCxt->pTempBuffer,
                                      sizeof(pstFOReg->nDataMB00) * pstONDSpec->nSctsPerPG);
                                    
                }
            }
        }

        if ((pMBuf != NULL) && (pSBuf == NULL))
        {
            if((nFlag & FSR_LLD_FLAG_BACKUP_DATA) != FSR_LLD_FLAG_BACKUP_DATA)
            {
                FSR_OAM_MEMSET(pstONDCxt->pTempBuffer, 0xFF, pstONDSpec->nSctsPerPG * FSR_SPARE_SIZE);
                
                TRANSFER_TO_NAND((void *)(&(pstFOReg->nDataSB00[0]) + 
                                 (nPreUseBuf & 1) * FSR_OND_SDRAM0_SIZE),
                                 pstONDCxt->pTempBuffer,
                                 FSR_SPARE_SIZE * pstONDSpec->nSctsPerPG);
                                    
                /* Consider about 2xPGM */
                if (((nCmdIdx & FSR_LLD_FLAG_CMDIDX_MASK) == FSR_LLD_FLAG_2X_PROGRAM) || 
                    ((nCmdIdx & FSR_LLD_FLAG_CMDIDX_MASK) == FSR_LLD_FLAG_2X_CACHEPGM))
                {
                    /*
                     *  When 2xPGM is accessed, spare buffer is written only one area.
                     *  so, if you do not want to fail operation, you will write same data into paired page.
                     */
                    TRANSFER_TO_NAND((void *)(&(pstFOReg->nDataSB00[0]) + 
                                     ((nPreUseBuf + 1) & 1) * FSR_OND_SDRAM0_SIZE),
                                     pstONDCxt->pTempBuffer,
                                     FSR_SPARE_SIZE * pstONDSpec->nSctsPerPG);
                }
            }
        }

        if (pSBuf != NULL)
        {                    
            /* If Dump flag is '1', all data in spare should be transfered to DataRAM */
            if ((nFlag & FSR_LLD_FLAG_DUMP_MASK) == FSR_LLD_FLAG_DUMP_ON)
            {
                if ((nFlag & FSR_LLD_FLAG_ECC_MASK) == FSR_LLD_FLAG_ECC_ON)
                {
                    _GenSWECCOfDumpSpare((UINT8 *) pSBuf,
                                         pstONDSpec->nSctsPerPG);
                }

                _WriteMain(&(pstFOReg->nDataSB00[0]) + 
                             (nPreUseBuf & 1) * FSR_OND_SDRAM0_SIZE,
                           (UINT8 *) pSBuf,
                           pstONDSpec->nSctsPerPG * FSR_OND_SPARE_SIZE);
            }
            else
            {
                /* 
                 * If FSR_LLD_FLAG_BBM_META_BLOCK (13th bit) of nFlag is set, 
                 * write nBlkType of FSRSpareBuf with 0x00 else write with 0xFF
                 */
                nBBMMetaIdx = (nFlag & FSR_LLD_FLAG_BBM_META_MASK) >> 
                              FSR_LLD_FLAG_BBM_META_BASEBIT;

                /* FSR_OND_MAX_BADMARK : 0 (0xFFFF), 1 (0x2222), 2 (0x4444), 3 (0x8888) */
                nBadMarkIdx = (nFlag & FSR_LLD_FLAG_BADMARK_MASK) >> 
                                    FSR_LLD_FLAG_BADMARK_BASEBIT;
                                
                /* Do not delete this code which writes bad mark  */
                pSBuf->pstSpareBufBase->nBadMark      = gnBadMarkValue[nBadMarkIdx];
                pSBuf->pstSpareBufBase->nBMLMetaBase0 = gnBBMMetaValue[nBBMMetaIdx];

                stTSBuf.pstSpareBufBase = pSBuf->pstSpareBufBase;

                /* Meta Extension for spare buffer should be decided */
                if (pSBuf->nNumOfMetaExt != 0)
                {
                    stTSBuf.nNumOfMetaExt = 1;
                    stTSBuf.pstSTLMetaExt = &pSBuf->pstSTLMetaExt[0];
                }
                else
                {
                    stTSBuf.nNumOfMetaExt = 0;
                }
                                
                _WriteSpare(&(pstFOReg->nDataSB00[0]) + 
                             (nPreUseBuf & 1) * FSR_OND_SDRAM0_SIZE, 
                            &stTSBuf, nFlag);
            }
            
#if defined (FSR_LLD_STATISTICS)
            nBytesTrasferred += (FSR_SPARE_BUF_BASE_SIZE + FSR_SPARE_BUF_EXT_SIZE);
#endif /* #if defined (FSR_LLD_STATISTICS) */

            /* Consider about 2xPGM */
            if (((nCmdIdx & FSR_LLD_FLAG_CMDIDX_MASK) == FSR_LLD_FLAG_2X_PROGRAM) || 
                ((nCmdIdx & FSR_LLD_FLAG_CMDIDX_MASK) == FSR_LLD_FLAG_2X_CACHEPGM))
            {
                /* If Dump flag is '1', all data in spare should be transfered to DataRAM */
                if ((nFlag & FSR_LLD_FLAG_DUMP_MASK) == FSR_LLD_FLAG_DUMP_ON)
                {
                    if ((nFlag & FSR_LLD_FLAG_ECC_MASK) == FSR_LLD_FLAG_ECC_ON)
                    {
                        _GenSWECCOfDumpSpare(((UINT8 *) pSBuf) + pstONDSpec->nSparePerSct,
                                    pstONDSpec->nSctsPerPG);
                    }

                    _WriteMain(&(pstFOReg->nDataSB00[0]) + 
                                 ((nPreUseBuf + 1)& 1) * FSR_OND_SDRAM0_SIZE,
                                 (UINT8 *) pSBuf + pstONDSpec->nSparePerSct,
                                 pstONDSpec->nSctsPerPG * FSR_OND_SPARE_SIZE);
                }
                else
                {   
                    /* Meta Extension for spare buffer should be decided */
                    if (pSBuf->nNumOfMetaExt != 0)
                    {
                        stTSBuf.nNumOfMetaExt = 1;
                        stTSBuf.pstSTLMetaExt = &pSBuf->pstSTLMetaExt[1];
                    }
                    else
                    {
                        stTSBuf.nNumOfMetaExt = 0;
                    }
                                        
                    /*
                     *  When 2xPGM is accessed, spare buffer is written only one area.
                     *  so, if you do not want to fail operation, you will write same data into paired page.
                     */
                    _WriteSpare(&(pstFOReg->nDataSB00[0]) + 
                                 ((nPreUseBuf + 1) & 1) * FSR_OND_SDRAM0_SIZE, 
                                &stTSBuf, nFlag);
                }

#if defined (FSR_LLD_STATISTICS)
                nBytesTrasferred += FSR_SPARE_BUF_EXT_SIZE;
#endif /* #if defined (FSR_LLD_STATISTICS) */

            }
        }

        /* 
         * Interrupt status register should be set before writing operation in NAND array finishes
         * Transfer occurs while data is written in NAND array.
         * But 2x operation executes checking about interrupt bit, 
         * so this waiting code is unnecessary
         */
        if ((nCmdIdx == FSR_LLD_FLAG_1X_PROGRAM) || 
            (nCmdIdx == FSR_LLD_FLAG_1X_CACHEPGM))
        {
            nLLDRe = FSR_OND_FlushOp(nDev, nDie, nFlag);
            if (nLLDRe != FSR_LLD_SUCCESS)
            {
                break;
            }
        }

        /* In case of 2 plane program, write protection has to be checked about second plane */
        if ((nCmdIdx == FSR_LLD_FLAG_2X_PROGRAM) || 
            (nCmdIdx == FSR_LLD_FLAG_2X_CACHEPGM))
        {
            /* Set paired block number */
            nPairedPbn = nPbn + 1;

            /* Set DFS, FBA                    */
            OND_WRITE(pstFOReg->nStartAddr1, 
                      (((UINT16) nPairedPbn << pstONDCxt->nDDPSelSft) & FSR_OND_MASK_DFS) | 
                       ((UINT16) nPairedPbn & pstONDCxt->nFBAMask));

            nWrProtectStat = OND_READ(pstFOReg->nWrProtectStat);

            if ((nWrProtectStat & FSR_LLD_BLK_STAT_MASK) != 
                FSR_LLD_BLK_STAT_UNLOCKED)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR , 
                               (TEXT("[OND:ERR]   Pbn is write protected\r\n")));

                _DumpRegisters(pstFOReg);

                nLLDRe |= (FSR_LLD_WR_PROTECT_ERROR | 
                           FSR_LLD_2NDPLN_CURR_ERROR);
            }
        }

        /* Set DFS, FBA                    */
        OND_WRITE(pstFOReg->nStartAddr1, 
                  (((UINT16) nPbn << pstONDCxt->nDDPSelSft) & FSR_OND_MASK_DFS) | 
                   ((UINT16) nPbn & pstONDCxt->nFBAMask));

        if (nCmdIdx != FSR_LLD_FLAG_OTP_PROGRAM)
        {
            /* Write protection can be checked when DBS & FBA are set */
            nWrProtectStat = OND_READ(pstFOReg->nWrProtectStat);
            if ((nWrProtectStat & FSR_LLD_BLK_STAT_MASK) != 
                FSR_LLD_BLK_STAT_UNLOCKED)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR , 
                               (TEXT("[OND:ERR]   Pbn is write protected\r\n")));

                _DumpRegisters(pstFOReg);

                nLLDRe = (FSR_LLD_WR_PROTECT_ERROR | 
                          FSR_LLD_1STPLN_CURR_ERROR);
            }
        }

        /* if the write protection error is detected, break do-while(0) loop */
        if (nLLDRe & (FSR_LLD_1STPLN_CURR_ERROR | FSR_LLD_2NDPLN_CURR_ERROR))
        {
            break;
        }

        /* Set Start Page Address (FPA)    */
        OND_WRITE(pstFOReg->nStartAddr8, 
                 (UINT16) nPgOffset << pstONDCxt->nFPASelSft);

        /* If Buffer has NULL value, DataRAM should not be changed */
        if ((pMBuf == NULL) && (pSBuf == NULL))
        {
            if (!(nFlag & FSR_LLD_FLAG_BACKUP_DATA))
            {
                FSR_DBZ_DBGMOUT(FSR_DBZ_ERROR, 
                               (TEXT("[OND:ERR] BackUp Flag is not found")));
                nLLDRe = FSR_LLD_INVALID_PARAM;
                break;
            }

            /* 
             * This situation should be occured on 1x operation. 
             * If the operation is 2x, buffer pointer should not be changed.
             */
            if ((nCmdIdx == FSR_LLD_FLAG_1X_PROGRAM) || 
                (nCmdIdx == FSR_LLD_FLAG_1X_CACHEPGM))
            {
                pstONDCxt->nPreUseBuf[nDie] += 1;
                nPreUseBuf = pstONDCxt->nPreUseBuf[nDie];
            }

            /* 
             * Set Start Buffer Register (BSA & BSC) 
             * BSA should be considered to use each buffer - DataRAM00 and DataRAM10 
             */
            nBSA = FSR_OND_BSA1000 | (((UINT16) nPreUseBuf & 1) << 2);
            
            /* Set to using number of sectors */
            OND_WRITE(pstFOReg->nStartBuf, 
                     (nBSA << pstONDCxt->nBSASelSft) | FSR_OND_BSC00);
        }
        else /* Normal case */
        {
            /* Set Start Buffer Register (BSA & BSC) */
            /* BSA should be considered to use each buffer - DataRAM00 and DataRAM10 */
            nBSA = FSR_OND_BSA1000 | (((UINT16) nPreUseBuf & 1) << 2);

            /* Set to using number of sectors */
            OND_WRITE(pstFOReg->nStartBuf, 
                     (nBSA << pstONDCxt->nBSASelSft) | FSR_OND_BSC00);

            /*
             * < nPreUseBuf decide previous used DataRAM for each die >
             * Double buffering makes a confusing condition to used DataRAM
             * so, you have to make a decision which buffer uses for current data and
             *     used for previous data.
             * ##This buffer is going to use for previous buffer##
             * ##   Data for plus one is next using buffer      ##
             */
            pstONDCxt->nPreUseBuf[nDie] += 1;
        }

        if ((nFlag & FSR_LLD_FLAG_ECC_MASK) == FSR_LLD_FLAG_ECC_ON)
        {
            /* 
             * System configuration 1 register has ECC bit which is 9th.
             * bit 0 : with correction, bit 1 : without correction
             */
            nSysConf1 = OND_READ(pstFOReg->nSysConf1);
            if (nSysConf1 & 0x0100)
            {
                OND_CLR(pstFOReg->nSysConf1, ~0x0100);
            }
        }
        else
        {
            OND_SET(pstFOReg->nSysConf1,  0x0100);
        }

        /* In case of non-blocking mode, interrupt should be enabled */
        if (nFlag & FSR_LLD_FLAG_INT_MASK)
        {
            FSR_OAM_ClrNEnableInt(pstONDCxt->nIntID);
        }
        else
        {
            FSR_OAM_ClrNDisableInt(pstONDCxt->nIntID);
        }

        /* Issue command */
        OND_WRITE(pstFOReg->nCmd, (UINT16) gnPgmCmdArray[nCmdIdx]);

        /* Save the information for deffered check */
#if defined (FSR_LLD_USE_CACHE_PGM)
        if (nCmdIdx == FSR_LLD_FLAG_2X_CACHEPGM)
        {
            pstONDShMem->nPreOp[nDie]       = FSR_OND_PREOP_CACHE_PGM;
            pstONDCxt->bIsPreCmdCache[nDie] = TRUE32;
        }
        else
        {
            if (pstONDCxt->bIsPreCmdCache[nDie] == TRUE32)
            {
                pstONDShMem->nPreOp[nDie] = FSR_OND_PREOP_CACHE_PGM;
            }
            else
            {
                if (pstONDSpec->nNumOfPlanes == FSR_MAX_PLANES)
                {
                    pstONDShMem->nPreOp[nDie] = FSR_OND_PREOP_2X_WRITE;
                }
                else
                {
                    pstONDShMem->nPreOp[nDie] = FSR_OND_PREOP_1X_WRITE;
                }
            }

            pstONDCxt->bIsPreCmdCache[nDie] = FALSE32;
        }
#else
        if ((nCmdIdx == FSR_LLD_FLAG_2X_PROGRAM) || 
            (nCmdIdx == FSR_LLD_FLAG_2X_CACHEPGM))
        {
            pstONDShMem->nPreOp[nDie]         = FSR_OND_PREOP_2X_WRITE;
        }
        else
        {
            pstONDShMem->nPreOp[nDie]         = FSR_OND_PREOP_1X_WRITE;
        }
#endif /* #if defined (FSR_LLD_USE_CACHE_PGM) */

        pstONDShMem->nPreOpPbn[nDie]      = (UINT16)nPbn;
        pstONDShMem->nPreOpPgOffset[nDie] = (UINT16)nPgOffset;
        pstONDShMem->nPreOpFlag[nDie]     = nFlag;

#if defined (FSR_LLD_LOGGING_HISTORY)
        _AddLog(nDev, nDie);
#endif

#if defined (FSR_LLD_STATISTICS)
        _AddONDStat(nDev, nDie, FSR_OND_STAT_WR_TRANS, nBytesTrasferred, FSR_OND_STAT_CACHE_PGM);
        _AddONDStat(nDev, nDie, FSR_OND_STAT_SLC_PGM, 0, FSR_OND_STAT_CACHE_PGM);
#endif /* #if defined (FSR_LLD_STATISTICS) */
    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return (nLLDRe);
}



/**
 * @brief          This function reads data from main area of DataRAM
 *
 * @param[out]     pDest        : pointer to the buffer
 * @param[in]      pstSrc       : pointer to the main area of DataRAM
 * @param[in]      nSize        : # of bytes to read
 * @param[in]      pTempBuffer: temporary buffer for reading misaligned data
 *
 * @return         VOID
 *
 * @author          NamOh Hwang / JeongWook Moon
 * @version         1.0.0
 * @remark          
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

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[OND:IN ] ++%s\r\n"), __FSR_FUNC__));

    /* if pDest is not 4 byte aligned. */
    if ((((UINT32) pDest & 0x3) != 0) || (nSize < FSR_OND_MIN_BULK_TRANS_SIZE))
    {
        TRANSFER_FROM_NAND(pTempBuffer, pSrc, nSize);
        FSR_OAM_MEMCPY(pDest, pTempBuffer, nSize);
    }
    else /* when pDest is 4 byte aligned */
    {
        TRANSFER_FROM_NAND(pDest, pSrc, nSize);
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[OND:OUT] --%s\r\n"), __FSR_FUNC__));
}


/**
 * @breif          this function reads data from spare area of DataRAM
 *
 * @param[out]     pstDest      : pointer to the host buffer
 * @param[in]      pSrc         : pointer to the spare area of DataRAM
 * @param[in]      nFlag        : Operation options such as ECC_ON, OFF
 *
 * @return         VOID
 *
 * @author         NamOh Hwang / JeongWook Moon
 * @version        1.0.0
 * @remark         
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
             UINT8   nBuf[8];
             UINT8   anDummyBuf[8];
             UINT8   nSWEccR[2];
             UINT16  nTemp16;
             UINT32  nIdx;
             BOOL32  bEccOn;
             UINT8  *apDataForCorrection[4] = {NULL, }; 

             INT32   nLLDRe     = FSR_LLD_SUCCESS;
             INT32   nRe        = 0;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[OND:IN ] ++%s\r\n"), __FSR_FUNC__));

    /* If number of MetaExt is 0, dest address is changed to dummy buffer point*/
    FSR_OAM_MEMSET(&anDummyBuf[0], 0xFF, sizeof(anDummyBuf));

    pSrc16  = (volatile UINT16 *) pSrc;
    pDest16 = (UINT16 *) pstDest->pstSpareBufBase; 

    FSR_ASSERT(((UINT32) pstDest & 0x01) == 0);

    if ((nFlag & FSR_LLD_FLAG_ECC_MASK) == FSR_LLD_FLAG_ECC_ON)
    {
        bEccOn = TRUE32;
    }
    else
    {
        bEccOn = FALSE32;
    }

    /* The value of nSctIdx has 2 or 4, which decides loop count for read */
    nTwoSctIdx = (FSR_LLD_SPARE_BUF_SIZE_2KB_PAGE / FSR_OND_SPARE_USER_AREA) >> 1;

    /* MetaExt is positioned on 3th sector */
    nExtSctPos = nTwoSctIdx - 1;

    do
    {
        nByteAcs = FSR_OND_SPARE_USER_AREA + FSR_OND_SPARE_SW_ECC_AREA;

        /* MetaExt is positioned on 5th byte of 3th sector */
        nExtBytePos = nByteAcs - 2;

        nIdx = 0;

        do
        {
            if (nTwoSctIdx == nExtSctPos && nByteAcs == nExtBytePos)
            {
                /* If number of meta extension is 0, the area is filled all '0xFF' */
                if (pstDest->nNumOfMetaExt != 0)
                {
                    FSR_OAM_MEMSET(&pstDest->pstSTLMetaExt[0], 0xFF, sizeof(FSRSpareBufExt));
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
                    nTemp16    = OND_READ(*pSrc16++);

                    if (nByteAcs == 5)
                    {
                        nSWEccR[0] = (UINT8) (nTemp16 & 0x00FF);
                    }
                    else /* nByteAcs == 1 */
                    {
                        nSWEccR[1] = (UINT8) (nTemp16 & 0x00FF);
                    }
                   /* Skip H/W Ecc area */
                    pSrc16 += FSR_OND_SPARE_HW_ECC_AREA - 1;
                }
                else /* if (bEccOn == TRUE32) */
                {
                    pSrc16    += FSR_OND_SPARE_HW_ECC_AREA; /* Skip 8 bytes for hardware ECC */ 
                }
            }
            else
            {
                 nReg      = OND_READ(*pSrc16++);

                 /* Byte position is set */
                 if ((nByteAcs % 4) != 3)
                 {
                     apDataForCorrection[nIdx >> 1] = (UINT8 *) pDest16;
                     nBuf[nIdx++] = (UINT8)(nReg);
                     nBuf[nIdx++] = (UINT8)(nReg >> 8);
                 }

                *pDest16++ = nReg;
            }
        } while(--nByteAcs > 0);
        
        if (bEccOn == TRUE32)
        {
            nRe = FSR_OND_ECC_CompS (&nSWEccR[0], &nBuf[0], nTwoSctIdx);

            /* If uncorrectable error is occured, it should be returned though existing other errors  */
            if (nRe == FSR_OND_SWECC_N_ERROR)
            {   
                if (!((nLLDRe & FSR_LLD_PREV_READ_ERROR) == FSR_LLD_PREV_READ_ERROR))
                {
                    nLLDRe = FSR_LLD_SUCCESS;
                }
            }
            else if (nRe == FSR_OND_SWECC_U_ERROR)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_INF , (TEXT("[OND:ERR]   Uncorrectable read error occurs\r\n")));

                nLLDRe = FSR_LLD_PREV_READ_ERROR | FSR_LLD_1STPLN_CURR_ERROR;
            }
            else if ((nRe == FSR_OND_SWECC_C_ERROR) ||
                     (nRe == FSR_OND_SWECC_R_ERROR))
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_INF , (TEXT("[OND:INF]   1 bit error occurs and corrected.\r\n")));
                
                /* Copy data into destination */
                *(apDataForCorrection[0])    = nBuf[0];
                *(apDataForCorrection[0]+1)  = nBuf[1];
                *(apDataForCorrection[1])    = nBuf[2];
                *(apDataForCorrection[1]+1)  = nBuf[3];
                *(apDataForCorrection[2])    = nBuf[4];
                *(apDataForCorrection[2]+1)  = nBuf[5];
                *(apDataForCorrection[3])    = nBuf[6];
                *(apDataForCorrection[3]+1)  = nBuf[7];
                
                if (!((nLLDRe & FSR_LLD_PREV_READ_ERROR) == FSR_LLD_PREV_READ_ERROR))
                {
                    nLLDRe = FSR_LLD_PREV_2LV_READ_DISTURBANCE | FSR_LLD_1STPLN_CURR_ERROR;
                }
            }
        }
    } while (--nTwoSctIdx > 0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[OND:OUT] --%s (nRe=%x)\r\n"), __FSR_FUNC__, nRe));

    return (nLLDRe);
}

/**
 * @brief          This function reads all datas in spare area
 *
 * @param[out]     pstDest      : pointer to the host buffer
 * @param[in]      pSrc         : pointer to the spare area of DataRAM
 * @param[in]      nSize        : the number of bytes to write
 *
 * @return         VOID
 *
 * @author         JeongWook Moon
 * @version        1.0.0
 * @remark         
 *
 */
PRIVATE VOID 
_DumpSpare(         UINT8  *pDest,
           volatile UINT8  *pSrc,
                    UINT32  nSize)
{
                UINT32  nCnt;
                UINT16  nReg;
    volatile    UINT16 *pSrc16; 

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[OND:IN ] ++%s\r\n"), __FSR_FUNC__));

    pSrc16  = (volatile UINT16 *)pSrc;

    /* Copy it by word, so divide it by 2 */
    nSize = (nSize >> 1);

    for (nCnt = 0; nCnt < nSize; nCnt++)
    {
         nReg    = OND_READ(*pSrc16++);
        *pDest++ = (UINT8)(nReg);
        *pDest++ = (UINT8)(nReg >> 8);
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[OND:OUT] --%s\r\n"), __FSR_FUNC__));
}

PRIVATE INT32
_CheckSWECCOfDumpSpare(UINT8 *pSBuf, UINT32 nNumOfSct)
{
    UINT32  nECCDataIdx     = 0;
    UINT8   nECCData[8];
    UINT8   nSWECC[2];
    UINT32  nSctIdx;
    INT32   nRe;
    INT32   nLLDRe          = FSR_LLD_SUCCESS;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[OND:IN ] ++%s\r\n"), __FSR_FUNC__));

    for (nSctIdx = 0; nSctIdx < nNumOfSct; nSctIdx++)
    {
        /* Get data in 0, 1, 4, 5th byte */
        nECCData[nECCDataIdx++] = pSBuf[nSctIdx * FSR_SPARE_SIZE + 0];
        nECCData[nECCDataIdx++] = pSBuf[nSctIdx * FSR_SPARE_SIZE + 1];
        nECCData[nECCDataIdx++] = pSBuf[nSctIdx * FSR_SPARE_SIZE + 4];
        nECCData[nECCDataIdx++] = pSBuf[nSctIdx * FSR_SPARE_SIZE + 5];
        nSWECC[nSctIdx & 0x1]   = pSBuf[nSctIdx * FSR_SPARE_SIZE + 6];

        /* If current sector is 1st or 3rd sector */
        if ((nSctIdx & 0x1) == 0x1)
        {
            nRe = FSR_OND_ECC_CompS(&nSWECC[0], &nECCData[0], nSctIdx >> 1);

            /* If uncorrectable error is occured, it should be returned though existing other errors  */
            if (nRe == FSR_OND_SWECC_N_ERROR)
            {   
                if (!((nLLDRe & FSR_LLD_PREV_READ_ERROR) == FSR_LLD_PREV_READ_ERROR))
                {
                    nLLDRe = FSR_LLD_SUCCESS;
                }
            }
            else if (nRe == FSR_OND_SWECC_U_ERROR)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_INF , (TEXT("[OND:ERR]   Uncorrectable read error occurs\r\n")));

                nLLDRe = FSR_LLD_PREV_READ_ERROR | FSR_LLD_1STPLN_CURR_ERROR;
            }
            else if ((nRe == FSR_OND_SWECC_C_ERROR) ||
                     (nRe == FSR_OND_SWECC_R_ERROR))
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_INF , (TEXT("[OND:INF]   1 bit error occurs and corrected.\r\n")));
                
                pSBuf[(nSctIdx - 1) * FSR_SPARE_SIZE + 0]   = nECCData[0];
                pSBuf[(nSctIdx - 1) * FSR_SPARE_SIZE + 1]   = nECCData[1];
                pSBuf[(nSctIdx - 1) * FSR_SPARE_SIZE + 4]   = nECCData[2];
                pSBuf[(nSctIdx - 1) * FSR_SPARE_SIZE + 5]   = nECCData[3];
                pSBuf[nSctIdx * FSR_SPARE_SIZE + 0]         = nECCData[4];                 
                pSBuf[nSctIdx * FSR_SPARE_SIZE + 1]         = nECCData[5];
                pSBuf[nSctIdx * FSR_SPARE_SIZE + 4]         = nECCData[6];
                pSBuf[nSctIdx * FSR_SPARE_SIZE + 5]         = nECCData[7];   

                if (!((nLLDRe & FSR_LLD_PREV_READ_ERROR) == FSR_LLD_PREV_READ_ERROR))
                {
                    nLLDRe = FSR_LLD_PREV_2LV_READ_DISTURBANCE | FSR_LLD_1STPLN_CURR_ERROR;
                }
            }

            nECCDataIdx = 0;
        }        
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[OND:OUT] --%s\r\n"), __FSR_FUNC__));

    return nLLDRe;
}
/**
 * @brief          This function reads data from NAND flash
 *
 * @param[in]      nDev         : Physical Device Number (0 ~ 7)
 * @param[in]      nPbn         : Physical Block  Number
 * @param[in]      nPgOffset    : Page Offset within a block
 * @param[out]     pMBuf        : Memory buffer for main  array of NAND flash
 * @param[out]     pSBuf        : Memory buffer for spare array of NAND flash
 * @param[in]      nFlag        : Operation options such as ECC_ON, OFF
 *
 * @return         FSR_LLD_SUCCESS
 * @return         FSR_LLD_INVALID_PARAM
 * @return         FSR_LLD_PREV_READ_ERROR  |
 *                 {FSR_LLD_1STPLN_CURR_ERROR | FSR_LLD_2NDPLN_CURR_ERROR}
 * @n              previous read error return value
 * @return         FSR_LLD_PREV_READ_DISTURBANCE |
 *                 {FSR_LLD_1STPLN_CURR_ERROR | FSR_LLD_2NDPLN_CURR_ERROR}
 * @n              previous read disturbance error return value
 *
 * @author         NamOh Hwang / JeongWook Moon
 * @version        1.0.0
 * @remark         
 *
 */
PUBLIC INT32
FSR_OND_Read(UINT32       nDev,
             UINT32       nPbn,
             UINT32       nPgOffset,
             UINT8       *pMBuf,
             FSRSpareBuf *pSBuf,
             UINT32       nFlag)
{
    INT32           nLLDRe     = FSR_LLD_SUCCESS;
    UINT32          nLLDFlag;
    
    do
    {
        nLLDFlag = ~FSR_LLD_FLAG_CMDIDX_MASK & nFlag;

        /* This operation makes a sequence for transferring data about loaded one */
        nLLDRe = FSR_OND_ReadOptimal(nDev, nPbn, nPgOffset, pMBuf, pSBuf, FSR_LLD_FLAG_1X_LOAD | nLLDFlag);
        if (FSR_RETURN_MAJOR(nLLDRe) != FSR_LLD_SUCCESS)
        {
            break;
        }

        nLLDRe = FSR_OND_ReadOptimal(nDev, nPbn, nPgOffset, pMBuf, pSBuf, FSR_LLD_FLAG_TRANSFER | nLLDFlag);
        if (FSR_RETURN_MAJOR(nLLDRe) != FSR_LLD_SUCCESS)
        {
            break;
        }
    } while (0);

    return (nLLDRe);
}

/**
 * @brief          This function reads data from NAND flash by dual buffering
 *
 * @param[in]      nDev         : Physical Device Number (0 ~ 7)
 * @param[in]      nPbn         : Physical Block  Number
 * @param[in]      nPgOffset    : Page Offset within a block
 * @param[out]     pMBuf        : Memory buffer for main  array of NAND flash
 * @param[out]     pSBuf        : Memory buffer for spare array of NAND flash
 * @param[in]      nFlag        : Operation options such as ECC_ON, OFF
 *
 * @return         FSR_LLD_SUCCESS
 * @return         FSR_LLD_INVALID_PARAM
 * @return         FSR_LLD_PREV_READ_ERROR  |
 *                 {FSR_LLD_1STPLN_CURR_ERROR | FSR_LLD_2NDPLN_CURR_ERROR}
 * @n              previous read error return value
 * @return         FSR_LLD_PREV_READ_DISTURBANCE |
 *                 {FSR_LLD_1STPLN_CURR_ERROR | FSR_LLD_2NDPLN_CURR_ERROR}
 * @n              previous read disturbance error return value
 *
 * @author         NamOh Hwang / JeongWook Moon
 * @version        1.0.0
 * @remark         
 *
 */
PUBLIC INT32
FSR_OND_ReadOptimal(UINT32       nDev,
                    UINT32       nPbn,
                    UINT32       nPgOffset,
                    UINT8       *pMBuf,
                    FSRSpareBuf *pSBuf,
                    UINT32       nFlag)
{
             OneNANDCxt    *pstONDCxt;
             OneNANDShMem  *pstONDShMem;
             OneNANDSpec   *pstONDSpec;
    volatile OneNANDReg    *pstFOReg;
             UINT32         nCmdIdx;
             UINT32         nDie;
             UINT32         nStartOffset; /* Start sector offset from the start */
             UINT32         nEndOffset;   /* End sector offset from the end     */

#if defined (FSR_LLD_STATISTICS)
             UINT32         nBytesTrasferred    = 0;
#endif

             INT32          nLLDRe              = FSR_LLD_SUCCESS;
             INT32          nSpareRe            = FSR_LLD_SUCCESS;
             UINT16         nBSA;
             UINT32         nPreUseBuf          = 0;
             UINT32         nSysConf1;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
                    (TEXT("[OND:IN ] ++%s(nDev:%d, nPbn:%d, nPgOffset:%d, nFlag:0x%08x)\r\n"), 
                    __FSR_FUNC__ , nDev, nPbn, nPgOffset, nFlag));

    do
    {

#if defined (FSR_LLD_STRICT_CHK)
        nLLDRe = _StrictChk(nDev, nPbn, nPgOffset);
        if (nLLDRe != FSR_LLD_SUCCESS)
        {
            break;
        }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */

        pstONDCxt   = gpstONDCxt[nDev];
        pstONDShMem = gpstONDShMem[nDev];
        pstONDSpec  = pstONDCxt->pstONDSpec;
        pstFOReg    = (volatile OneNANDReg *) pstONDCxt->nBaseAddr;

        nCmdIdx = (nFlag & FSR_LLD_FLAG_CMDIDX_MASK) >> FSR_LLD_FLAG_CMDIDX_BASEBIT;
        nDie    = (nPbn >> (FSR_OND_DFS_BASEBIT - pstONDCxt->nDDPSelSft)) & 0x1;

        /*  
         * Read operation has two parts, Load and transfer.
         * Transfer means dump to data to DRAM from DataRAM, 
         * Load dumps data to DataRAM from NAND array.
         */
        if ((nCmdIdx == FSR_LLD_FLAG_1X_LOAD) || (nCmdIdx == FSR_LLD_FLAG_1X_PLOAD))
        {
            /* OneNAND doesn't support read interleaving,
               therefore, wait until interrupt status is ready for both die. 
               if device is DDP. */
            if (pstONDSpec->nNumOfDies == FSR_MAX_DIES)
            {
                /* 
                 * Ignore read error of the other die
                 * that read error is detected when that die is accessed 
                 */
                FSR_OND_FlushOp(nDev,
                                (nDie + 0x1) & 0x01,
                                nFlag | FSR_LLD_FLAG_REMAIN_PREOP_STAT);
            }
            nLLDRe = FSR_OND_FlushOp(nDev,
                                     nDie,
                                     nFlag);

            /* Set DBS */
            OND_WRITE(pstFOReg->nStartAddr2, 
                     ((UINT16) nPbn << pstONDCxt->nDDPSelSft) & FSR_OND_MASK_DBS);

            /* Set DFS & FBA */
            OND_WRITE(pstFOReg->nStartAddr1, 
                      (((UINT16) nPbn << pstONDCxt->nDDPSelSft) & FSR_OND_MASK_DFS) |
                       ((UINT16) nPbn & pstONDCxt->nFBAMask));

            /* Set FPA (Flash Page Address) */
            OND_WRITE(pstFOReg->nStartAddr8, 
                     (UINT16) (nPgOffset << pstONDCxt->nFPASelSft));

            /*
             * < nPreUseBuf decide previous used DataRAM for each die >
             * ## Load operation has not 2x load, so Double buffering is not occur
             * ## but pre-load demands double buffering.
             * so, you have to make a decision which buffer uses for current data and
             *     used for previous data.
             * ## This buffer is going to use for current buffer ##
             */
            if (nCmdIdx == FSR_LLD_FLAG_1X_PLOAD)
            {
                pstONDCxt->nPreUseBuf[nDie] += 1;
            }

            /* 
             * Set BSA (Buffer Sector Address) & BSC (Buffer Sector Count) 
             * BSA has to be set to 1000 
             */
            nBSA = FSR_OND_BSA1000 | (((UINT16) pstONDCxt->nPreUseBuf[nDie] & 1) << 2);

            OND_WRITE(pstFOReg->nStartBuf, 
                     (nBSA << pstONDCxt->nBSASelSft) | FSR_OND_BSC00);

            /* Set System Configuration1 Reg (ECC On or Off) */
            if ((nFlag & FSR_LLD_FLAG_ECC_MASK) == FSR_LLD_FLAG_ECC_ON)
            {
                nSysConf1 = OND_READ(pstFOReg->nSysConf1);
                if (nSysConf1 & 0x0100)
                {
                    OND_CLR(pstFOReg->nSysConf1, ~0x0100);
                }
            }
            else
            {
                OND_SET(pstFOReg->nSysConf1,  0x0100);
            }

            /* In case of non-blocking mode, interrupt should be enabled */
            if (nFlag & FSR_LLD_FLAG_INT_MASK)
            {
                FSR_OAM_ClrNEnableInt(pstONDCxt->nIntID);
            }
            else
            {
                FSR_OAM_ClrNDisableInt(pstONDCxt->nIntID);
            }

            /* Issue command (nCmdIdx : 0 (0xFFFF), 1 (0x0000), 2 (0x0000)) */
            OND_WRITE(pstFOReg->nCmd, (UINT16) gnLoadCmdArray[nCmdIdx]);

#if defined (FSR_LLD_STATISTICS)
            if (nCmdIdx == FSR_LLD_FLAG_1X_PLOAD)
            {
                _AddONDStat(nDev, nDie, FSR_OND_STAT_SLC_LOAD, 0, FSR_OND_STAT_PLOAD);
            }
            else
            {
                _AddONDStat(nDev, nDie, FSR_OND_STAT_SLC_LOAD, 0, FSR_OND_STAT_NORMAL_CMD);
            }
#endif /* #if defined (FSR_LLD_STATISTICS) */
        }
        
        /* Transfer data loaded by the previous load command */
        if (nFlag & FSR_LLD_FLAG_TRANSFER)
        {
            /* If PLOAD is accessed, nPreUseBuf should be changed to prevent previous data */
            if (nCmdIdx == FSR_LLD_FLAG_1X_PLOAD)
            {
                /* 
                 * If the command is PLoad & Tranfer, you must check the pre-using buffer 
                 * Trnasfer should be occured previous data. so, you must change BSA.
                 */
                nPreUseBuf  = pstONDCxt->nPreUseBuf[nDie] + 1;
            }
            else
            {
                /* 
                 * OneNAND doesn't support read interleaving.
                 * Therefore, wait until interrupt status is ready for both die,
                 * if device is DDP. 
                 */
                if (pstONDSpec->nNumOfDies == FSR_MAX_DIES)
                {
                    /* Ignore read error of the other die */
                    FSR_OND_FlushOp(nDev,
                                    (nDie + 0x1) & 0x01,
                                    nFlag | FSR_LLD_FLAG_REMAIN_PREOP_STAT);
                }

                nLLDRe = FSR_OND_FlushOp(nDev,
                                         nDie,
                                         nFlag);

                /* Set DBS */
                OND_WRITE(pstFOReg->nStartAddr2, (UINT16)(nDie << FSR_OND_DBS_BASEBIT));
            }

            /* If transfer only, */
            if (nCmdIdx == FSR_LLD_FLAG_NO_LOADCMD)
            {
                nPreUseBuf = pstONDCxt->nPreUseBuf[nDie];
            }

            /* Check whether source buffer is empty or not */
            if (pMBuf != NULL)
            {
                /* 
                 * By extracting start & end offset from nFlag,
                 * _ReadMain() can load continuous sectors within a page.
                 */
                nStartOffset = (nFlag & FSR_LLD_FLAG_1ST_SCTOFFSET_MASK) >> 
                                FSR_LLD_FLAG_1ST_SCTOFFSET_BASEBIT;
                nEndOffset   = (nFlag & FSR_LLD_FLAG_LAST_SCTOFFSET_MASK) >> 
                                FSR_LLD_FLAG_LAST_SCTOFFSET_BASEBIT;

                _ReadMain(pMBuf + nStartOffset * FSR_OND_SECTOR_SIZE,
                         &(pstFOReg->nDataMB00[0]) + 
                          (nPreUseBuf & 1) * FSR_OND_MDRAM0_SIZE + 
                          nStartOffset * FSR_OND_SECTOR_SIZE,
                         (pstONDSpec->nSctsPerPG - nStartOffset - nEndOffset) * 
                          FSR_OND_SECTOR_SIZE,
                          (UINT8 *) pstONDCxt->pTempBuffer);

#if defined (FSR_LLD_STATISTICS)
                nBytesTrasferred += (pstONDSpec->nSctsPerPG - nStartOffset - nEndOffset) * FSR_OND_SECTOR_SIZE;
#endif /* #if defined (FSR_LLD_STATISTICS) */

            }

            if ((pSBuf != NULL) && (nFlag & FSR_LLD_FLAG_USE_SPAREBUF))
            {
                /* If Dump flag is '1', all data in spare should be transfered to DRAM in host */
                if ((nFlag & FSR_LLD_FLAG_DUMP_MASK) == FSR_LLD_FLAG_DUMP_ON)
                {
                    _DumpSpare((UINT8 *)pSBuf, 
                               &(pstFOReg->nDataSB00[0]) + (nPreUseBuf & 1) * FSR_OND_SDRAM0_SIZE,
                               pstONDSpec->nSctsPerPG * FSR_OND_SPARE_SIZE);

                    if ((nFlag & FSR_LLD_FLAG_ECC_MASK) == FSR_LLD_FLAG_ECC_ON)
                    {
                        /* SW ECC Check */
                        nSpareRe = _CheckSWECCOfDumpSpare((UINT8 *)pSBuf, pstONDSpec->nSctsPerPG);
                        if (nSpareRe != FSR_LLD_SUCCESS)
                        {
                            if (!((nLLDRe & FSR_LLD_PREV_READ_ERROR) == FSR_LLD_PREV_READ_ERROR))
                            {
                                nLLDRe = nSpareRe;
                            }
                        }
                    }
                }
                else /* FSR_LLD_FLAG_DUMP_OFF */
                {
                    nSpareRe = _ReadSpare((FSRSpareBuf *) pSBuf, 
                                         &(pstFOReg->nDataSB00[0]) + 
                                         (nPreUseBuf & 1) * 
                                         FSR_OND_SDRAM0_SIZE,
                                         nFlag);
                    if (nSpareRe != FSR_LLD_SUCCESS)
                    {
                        if (!((nLLDRe & FSR_LLD_PREV_READ_ERROR) == FSR_LLD_PREV_READ_ERROR))
                        {
                            nLLDRe = nSpareRe;
                        }
                    }
                }
#if defined (FSR_LLD_STATISTICS)
                nBytesTrasferred += sizeof(FSRSpareBuf);
#endif /* #if defined (FSR_LLD_STATISTICS) */
            }
        }

        /*
         * If transfer only operation, nPreOp[nDie] should be FSR_OND_PREOP_NONE.
         * ----------------------------------------------------------------------
         * FSR_OND_FlushOp() wait read interrupt if nPreOp is FSR_OND_PREOP_READ.
         * But, if this function only has FSR_LLD_FLAG_TRANSFER, there is no read interrupt.
         * Therefore, In order to avoid infinite loop, nPreOp is FSR_OND_PREOP_NONE.
         * ----------------------------------------------------------------------
         */
        if ((nCmdIdx == FSR_LLD_FLAG_NO_LOADCMD) &&
            (nFlag & FSR_LLD_FLAG_TRANSFER))
        {
            pstONDShMem->nPreOp[nDie]         = FSR_OND_PREOP_NONE;
        }
        else
        {
            pstONDShMem->nPreOp[nDie]         = FSR_OND_PREOP_READ;
        }

        pstONDShMem->nPreOpPbn[nDie]      = (UINT16)nPbn;
        pstONDShMem->nPreOpPgOffset[nDie] = (UINT16)nPgOffset;
        pstONDShMem->nPreOpFlag[nDie]     = nFlag;

#if defined (FSR_LLD_LOGGING_HISTORY)
        _AddLog(nDev, nDie);
#endif

#if defined (FSR_LLD_STATISTICS)
        _AddONDStat(nDev, nDie, FSR_OND_STAT_RD_TRANS, nBytesTrasferred, FSR_OND_STAT_PLOAD);
#endif /* #if defined (FSR_LLD_STATISTICS) */

    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s() / nRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return (nLLDRe);
}


/**
 * @brief          This function reads data from NAND flash
 *
 * @param[in]      nDev         : Physical Device Number (0 ~ 7)
 * @param[out]     pMBuf        : Memory buffer for main  array of NAND flash
 * @param[out]     pSBuf        : Memory buffer for spare array of NAND flash
 *                 nDieIdx      : 0 is for 1st die
 *                              : 1 is for 2nd die
 * @param[in]      nFlag        : FSR_LLD_FLAG_NONE
 *
 * @return         FSR_LLD_SUCCESS
 *
 * @author         NamOh Hwang / JeongWook Moon
 * @version        1.0.0
 * @remark         
 *
 */
PUBLIC INT32
FSR_OND_GetPrevOpData(UINT32       nDev,
                      UINT8       *pMBuf,
                      FSRSpareBuf *pSBuf,
                      UINT32       nDie,
                      UINT32       nFlag)
{
             OneNANDCxt    *pstONDCxt;
             OneNANDSpec   *pstONDSpec;
    volatile OneNANDReg    *pstFOReg;
             UINT32         nPreUseBuf  = 0;
             INT32          nLLDRe      = FSR_LLD_SUCCESS;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:IN ] ++%s(nDev:%d,nDie:%d,nFlag:0x%08x\r\n"),
        __FSR_FUNC__, nDev, nDie, nFlag));

    do
    {

#if defined (FSR_LLD_STRICT_CHK)
        /* Check device number */
        if (nDev >= FSR_OND_MAX_DEVS)
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
        pstONDCxt  = gpstONDCxt[nDev];
        pstONDSpec = pstONDCxt->pstONDSpec;
        pstFOReg   = (volatile OneNANDReg *) pstONDCxt->nBaseAddr;

        /* Set DBS */
        OND_WRITE(pstFOReg->nStartAddr2, (UINT16)(nDie << FSR_OND_DBS_BASEBIT));

        /* 
         * You must check whether the operation is 1x or 2x.
         * if the operation is 2x, 4k data is read to buffer.
         */
        if ((nFlag & FSR_LLD_FLAG_CMDIDX_MASK) == FSR_LLD_FLAG_1X_OPERATION)
        {
            nPreUseBuf = pstONDCxt->nPreUseBuf[nDie];
            nPreUseBuf += 1;

            _ReadMain (pMBuf, 
                       (volatile UINT8 *) (&pstFOReg->nDataMB00[0] + 
                                          (nPreUseBuf & 1) * FSR_OND_MDRAM0_SIZE), 
                       pstONDSpec->nSctsPerPG * FSR_OND_SECTOR_SIZE,
                       (UINT8 *) pstONDCxt->pTempBuffer);
            nLLDRe = _ReadSpare(pSBuf, 
                                (volatile UINT8 *) (&pstFOReg->nDataSB00[0] + 
                                                    (nPreUseBuf & 1) * FSR_OND_SDRAM0_SIZE), nFlag);
        }

        if ((nFlag & FSR_LLD_FLAG_CMDIDX_MASK) == FSR_LLD_FLAG_2X_OPERATION)
        {
            _ReadMain (pMBuf, 
                       (volatile UINT8 *) &pstFOReg->nDataMB00[0], 
                       pstONDSpec->nSctsPerPG * FSR_OND_SECTOR_SIZE * 2,
                       (UINT8 *) pstONDCxt->pTempBuffer);
            nLLDRe = _ReadSpare(pSBuf, 
                                (volatile UINT8 *) &pstFOReg->nDataSB00[0], nFlag);
        }
    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return (nLLDRe);
}


/**
 * @brief          This function reads data from NAND flash, program with random data input
 *
 * @param[in]      nDev         : Physical Device Number (0 ~ 7)
 * @param[in]      pstCpArg     : pointer to the structure LLDCpBkArg 
 * @param[in]      nFlag        : FSR_LLD_FLAG_1X_CPBK_LOAD
 * @n                             FSR_LLD_FLAG_1X_CPBK_PROGRAM
 *
 * @return         FSR_LLD_SUCCESS
 * @return         FSR_LLD_INVALID_PARAM
 * @return         FSR_LLD_PREV_READ_ERROR |
 *                 {FSR_LLD_1STPLN_CURR_ERROR | FSR_LLD_2NDPLN_CURR_ERROR}
 * @n              previous read error return value
 * @return         FSR_LLD_PREV_WRITE_ERROR  |
 *                 {FSR_LLD_1STPLN_PREV_ERROR | FSR_LLD_2NDPLN_PREV_ERROR}
 * @n              previous write error return value
 * @return         FSR_LLD_WR_PROTECT_ERROR |
 *                 {FSR_LLD_1STPLN_CURR_ERROR | FSR_LLD_2NDPLN_CURR_ERROR}
 * @n              write protetion error return value
 *
 * @author         NamOh Hwang / JeongWook Moon
 * @version        1.0.0
 * @remark         
 *
 */
PUBLIC INT32
FSR_OND_CopyBack(UINT32      nDev,
                 LLDCpBkArg *pstCpArg,
                 UINT32      nFlag)
{
             OneNANDCxt       *pstONDCxt;
             OneNANDShMem     *pstONDShMem;
             OneNANDSpec      *pstONDSpec;
    volatile OneNANDReg       *pstFOReg;
             LLDRndInArg      *pstRIArg; /* Random in argument */
             UINT32            nCmdIdx;
             UINT32            nCnt;
             UINT32            nPgOffset;

#if defined (FSR_LLD_STATISTICS)
             UINT32            nBytesTrasferred     = 0;
#endif /* #if defined (FSR_LLD_STATISTICS) */

             INT32             nLLDRe               = FSR_LLD_SUCCESS;
             UINT32            nDie                 = 0;
             UINT32            nDstDie              = 0;
             UINT32            nSrcDie              = 0;
             UINT16            nPbn;
             UINT16            nPairedPbn;
             UINT16            nECCRes; /* ECC result */
             UINT16            nBSA                 = 0;
             UINT32            nPreUseBuf           = 0;
             FSRSpareBuf       stSBuf4RndIn;
             FSRSpareBufBase   stSpareBufBase;
             FSRSpareBufExt    stSBuf4Ext[FSR_MAX_SPARE_BUF_EXT];
             UINT8             nSBuf4RndIn[FSR_SPARE_BUF_SIZE_4KB_PAGE];
             UINT16            nOffSet;
             BOOL32            bIsRndIn4Spare       = FALSE32;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG, 
                    (TEXT("[OND:IN ] ++%s(nDev:%d, nFlag:0x%08x)\r\n"),
                    __FSR_FUNC__, nDev, nFlag));

    do
    {

#if defined (FSR_LLD_STRICT_CHK)
        /* Check device number */
        if (nDev >= FSR_OND_MAX_DEVS)
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
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR , (TEXT("[OND:ERR]  (CPBACK)pstCpArg is NULL\r\n")));
            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */

        pstONDCxt    = gpstONDCxt[nDev];
        pstONDShMem  = gpstONDShMem[nDev];
        pstONDSpec   = pstONDCxt->pstONDSpec;
        pstFOReg     = (volatile OneNANDReg *) pstONDCxt->nBaseAddr;

        if (((nFlag & FSR_LLD_FLAG_CMDIDX_MASK) == FSR_LLD_FLAG_1X_CPBK_LOAD) ||
            ((nFlag & FSR_LLD_FLAG_CMDIDX_MASK) == FSR_LLD_FLAG_2X_CPBK_LOAD))
        {
            FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_INF,
                (TEXT("[OND:INF]   %s(nDev:%d, nSrcPbn:%d, nSrcPg:%d, nFlag:0x%08x)\r\n"),
                __FSR_FUNC__, nDev, pstCpArg->nSrcPbn, pstCpArg->nSrcPgOffset, nFlag));

            /* 
             * Load phase of copyback() only checks the source block & page offset,
             * for BML does not fill the destination block & page offset at this phase
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

            nDie = (nPbn >> (FSR_OND_DFS_BASEBIT - pstONDCxt->nDDPSelSft)) & 0x1;

            /* 
             * OneNAND doesn't support read interleaving,
             * therefore, wait until interrupt status is ready for both die
             * if device is DDP. 
             */
            if (pstONDSpec->nNumOfDies == FSR_MAX_DIES)
            {
                /* Ignore error of the other die */
                FSR_OND_FlushOp(nDev,
                                (nDie + 1) & 1,
                                nFlag | FSR_LLD_FLAG_REMAIN_PREOP_STAT);
            }

            nLLDRe = FSR_OND_FlushOp(nDev,
                                     nDie,
                                     nFlag);
            if (nLLDRe != FSR_LLD_SUCCESS)
            {
                break;
            }

            /* Set DBS */
            OND_WRITE(pstFOReg->nStartAddr2, 
                     ((UINT16) (nDie & 1) << FSR_OND_DBS_BASEBIT) & FSR_OND_MASK_DBS) ;

            /* Set DFS & FBA                 */
            OND_WRITE(pstFOReg->nStartAddr1, 
                      (((nPbn << pstONDCxt->nDDPSelSft) & FSR_OND_MASK_DFS) | 
                       (nPbn & pstONDCxt->nFBAMask)));

            /* Set FPA (Flash Page Address)  */
            OND_WRITE(pstFOReg->nStartAddr8, 
                     (UINT16) nPgOffset << pstONDCxt->nFPASelSft);

            /* If 2x opertion is accessed, whole DataRAMs should be selected  */
            if ((nFlag & FSR_LLD_FLAG_CMDIDX_MASK) == FSR_LLD_FLAG_2X_CPBK_LOAD)
            {
                nPreUseBuf = 0;
            }
            else
            {
                nPreUseBuf = pstONDCxt->nPreUseBuf[nDie];
                if (pstONDSpec->nNumOfPlanes ==  FSR_MAX_PLANES)
                {
                    pstONDCxt->nPreUseBuf[nDie] += 1;
                }
            }

            /* Set BSC (Buffer Sector Count) */
            nBSA = FSR_OND_BSA1000 | (((UINT16) nPreUseBuf & 1) << 2);
            OND_WRITE(pstFOReg->nStartBuf, 
                     (nBSA << pstONDCxt->nBSASelSft) | FSR_OND_BSC00);

            /* 
             * Set System Configuration1 Reg (ECC On) 
             * pstONDCxt->nECCValue[1] has ECC on value
             */
            if ((nFlag & FSR_LLD_FLAG_ECC_MASK) == FSR_LLD_FLAG_ECC_ON)
            {
                OND_CLR(pstFOReg->nSysConf1, ~0x0100);
            }
            else
            {
                OND_SET(pstFOReg->nSysConf1,  0x0100);
            }

            if ((nFlag & FSR_LLD_FLAG_CMDIDX_MASK) != FSR_LLD_FLAG_2X_CPBK_LOAD)
            {
                /* In case of non-blocking mode, interrupt should be enabled */
                if (nFlag & FSR_LLD_FLAG_INT_MASK)
                {
                    FSR_OAM_ClrNEnableInt(pstONDCxt->nIntID);
                }
                else
                {
                    FSR_OAM_ClrNDisableInt(pstONDCxt->nIntID);
                }
            }

            nCmdIdx = (nFlag & FSR_LLD_FLAG_CMDIDX_MASK) >> 
                      FSR_LLD_FLAG_CMDIDX_BASEBIT;

            OND_WRITE(pstFOReg->nCmd, (UINT16) gnCpBkCmdArray[nCmdIdx]);

            /* In case of 2x copy back  */
            if ((nFlag & FSR_LLD_FLAG_CMDIDX_MASK) == FSR_LLD_FLAG_2X_CPBK_LOAD)
            {
                /* Check whether protection is released or not */
                WAIT_OND_INT_STAT(pstFOReg, FSR_OND_INT_RI_READY);

                /* Error check routine about 1st load */
                if ((nFlag & FSR_LLD_FLAG_ECC_MASK) == FSR_LLD_FLAG_ECC_ON)
                {
                    nECCRes = OND_READ(pstFOReg->nEccStat);

                    if (nECCRes & pstONDSpec->nECC2LvRdDist)
                    {
                        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
                            (TEXT("[OND:INF]   %s() / %d line\r\n"),
                            __FSR_FUNC__, __LINE__));

                        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
                            (TEXT("            read disturbance at nPbn:%d, nPgOffset:%d\r\n"),
                            pstONDShMem->nPreOpPbn[nDie], pstONDShMem->nPreOpPgOffset[nDie]));

                        nLLDRe = FSR_LLD_PREV_2LV_READ_DISTURBANCE | 
                                 FSR_LLD_1STPLN_CURR_ERROR;

                        /* Register status is saved ECC register */
                        pstONDCxt->nECCRes[0] = nLLDRe;
                    }
                    else if (nECCRes & FSR_OND_ECC_UNCORRECTABLE)
                    {
                        nLLDRe = FSR_LLD_PREV_READ_ERROR | 
                                 FSR_LLD_1STPLN_CURR_ERROR;

                        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
                            (TEXT("[OND:INF]   %s() / %d line\r\n"),
                            __FSR_FUNC__, __LINE__));

                        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
                            (TEXT("            read error at nPbn:%d, nPgOffset:%d\r\n"),
                            pstONDShMem->nPreOpPbn[nDie], pstONDShMem->nPreOpPgOffset[nDie]));

                        _DumpRegisters(pstFOReg);
                        _DumpSpareBuffer(pstFOReg, nDev, nDie);

                        /* Register status is saved ECC register */
                        pstONDCxt->nECCRes[0] = nLLDRe;
                    }
                }

                /* Set paired block number */
                nPairedPbn = nPbn + 1;

                /* Set DFS, FBA */
                OND_WRITE(pstFOReg->nStartAddr1, 
                          (((UINT16) nPairedPbn << pstONDCxt->nDDPSelSft) & FSR_OND_MASK_DFS) | 
                           ((UINT16) nPairedPbn & pstONDCxt->nFBAMask));

                /* Set FPA (Flash Page Address)  */
                OND_WRITE(pstFOReg->nStartAddr8, 
                         (UINT16) nPgOffset << pstONDCxt->nFPASelSft);

                /* Set BSC (Buffer Sector Count) */
                /* BSA has to be set to 1100 */
                nBSA = FSR_OND_BSA1100;
                OND_WRITE(pstFOReg->nStartBuf, 
                         (nBSA << pstONDCxt->nBSASelSft) | FSR_OND_BSC00);

                /* In case of non-blocking mode, interrupt should be enabled */
                if (nFlag & FSR_LLD_FLAG_INT_MASK)
                {
                    FSR_OAM_ClrNEnableInt(pstONDCxt->nIntID);
                }
                else
                {
                    FSR_OAM_ClrNDisableInt(pstONDCxt->nIntID);
                }

                /* Command : FSR_LLD_FLAG_2X_CPBK_LOAD */
                OND_WRITE(pstFOReg->nCmd, (UINT16) gnCpBkCmdArray[nCmdIdx]);
            }

            pstONDShMem->nPreOp[nDie]         = FSR_OND_PREOP_READ;
            pstONDShMem->nPreOpPbn[nDie]      = (UINT16) nPbn;
            pstONDShMem->nPreOpPgOffset[nDie] = (UINT16) nPgOffset;


#if defined (FSR_LLD_LOGGING_HISTORY)
            _AddLog(nDev, nDie);
#endif

#if defined (FSR_LLD_STATISTICS)

            _AddONDStat(nDev, nDie, FSR_OND_STAT_SLC_LOAD, 0, FSR_OND_STAT_NORMAL_CMD);

#endif /* #if defined (FSR_LLD_STATISTICS) */
        }
        else if (((nFlag & FSR_LLD_FLAG_CMDIDX_MASK) == FSR_LLD_FLAG_1X_CPBK_PROGRAM) || 
                 ((nFlag & FSR_LLD_FLAG_CMDIDX_MASK) == FSR_LLD_FLAG_2X_CPBK_PROGRAM))
        {

            /* Flush operation for unpaired 1st plane block of 2X program */
            if (((nFlag & FSR_LLD_FLAG_CMDIDX_MASK) == FSR_LLD_FLAG_1X_CPBK_PROGRAM) &&
                (pstONDSpec->nNumOfPlanes ==  FSR_MAX_PLANES))
            {
                nLLDRe = FSR_OND_FlushOp(nDev,
                                         nDie,
                                         nFlag);
                if (nLLDRe != FSR_LLD_SUCCESS)
                {
                    break;
                }
            }

            FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_INF,
            (TEXT("[OND:INF]   %s(nDev:%d, nDstPbn:%d, nDstPg:%d, nRndInCnt:%d, nFlag:0x%08x)\r\n"),
            __FSR_FUNC__, nDev, pstCpArg->nDstPbn, pstCpArg->nDstPgOffset, pstCpArg->nRndInCnt, nFlag));

            /* 
             * If SrcBlk is in another block with DstBlk, 
             * you should be placed data into DataRAM of DstBlk`s.
             * This situation is valid about Dual Die Package.
             */
            if ((pstONDSpec->nNumOfDies > 1) && (pstONDSpec->nDID == FSR_OND_3G_DDP_DID))
            {
                nSrcDie = (pstCpArg->nSrcPbn >> (FSR_OND_DFS_BASEBIT - pstONDCxt->nDDPSelSft)) & 0x1;
                nDstDie = (pstCpArg->nDstPbn >> (FSR_OND_DFS_BASEBIT - pstONDCxt->nDDPSelSft)) & 0x1;

                if (nSrcDie != nDstDie)
                {
                    _CopyToAnotherDataRAM(nDev, pstCpArg);
                }
            }

            nPbn      = pstCpArg->nDstPbn;
            nPgOffset = pstCpArg->nDstPgOffset;

            nDie = (nPbn >> (FSR_OND_DFS_BASEBIT - pstONDCxt->nDDPSelSft)) & 0x1;

#if defined (FSR_LLD_STRICT_CHK)
            if ((nLLDRe = _StrictChk(nDev, nPbn, nPgOffset)) != FSR_LLD_SUCCESS)
            {
                break;
            }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */

            /* Set DBS                      */
            OND_WRITE(pstFOReg->nStartAddr2, 
                     (nPbn << pstONDCxt->nDDPSelSft) & FSR_OND_MASK_DBS);

            if (pstONDShMem->nPreOp[nDie] == FSR_OND_PREOP_READ)
            {
                /* Wait until device is ready   */
                WAIT_OND_INT_STAT(pstFOReg, FSR_OND_INT_RI_READY);
            }

            if ((nFlag & FSR_LLD_FLAG_CMDIDX_MASK) == FSR_LLD_FLAG_2X_CPBK_PROGRAM)
            {
                /* If 2x opertion is accessed, whole DataRAMs should be selected */
                nPreUseBuf = 0;
            }
            else
            {
                /*
                 * < nPreUseBuf decide previous used DataRAM for each die >
                 * Double buffering makes a confusing condition to used DataRAM
                 * so, you have to make a decision which buffer uses for current data and
                 *     used for previous data.
                 * ## This buffer is going to use for previous buffer and using buffer
                 * ## copyback with random in data is occured same BSA
                 */

                nPreUseBuf = pstONDCxt->nPreUseBuf[nDie];
                pstONDCxt->nPreUseBuf[nDie] += 1;
            }

            /* Set BSC (Buffer Sector Count) */
            nBSA = FSR_OND_BSA1000 | (((UINT16) nPreUseBuf & 1) << 2);
            OND_WRITE(pstFOReg->nStartBuf, 
                     (nBSA << pstONDCxt->nBSASelSft) | FSR_OND_BSC00);
            
            /* If the spare buffer random-in is avaible, preload spare buffer */
            for (nCnt = 0; nCnt < pstCpArg->nRndInCnt; nCnt++)
            {
                pstRIArg = pstCpArg->pstRndInArg + nCnt;

                /* In case copyback of spare area is requested */
                if (pstRIArg->nOffset >= FSR_LLD_CPBK_SPARE)
                {
                    /* 
                     * Read spare for random in data to spare area.
                     * Random in data to spare is not a single data to whole area.
                     * Data can be devided many times, so, you should be check the offset
                     * But spare structure is too complex, for you can write random in data
                     * on temperary buffer.
                     */
                    stSBuf4RndIn.pstSpareBufBase = &stSpareBufBase;
                    stSBuf4RndIn.nNumOfMetaExt = 1;
                    stSBuf4RndIn.pstSTLMetaExt = &stSBuf4Ext[0];

                    nLLDRe = _ReadSpare(&stSBuf4RndIn, 
                                        &pstFOReg->nDataSB00[0] + (nPreUseBuf & 1) * FSR_OND_SDRAM0_SIZE, 
                                        nFlag);

                    /* Copy to structure buffer from data array which is occured data random in */
                    FSR_OAM_MEMCPY(&nSBuf4RndIn[0],
                                   stSBuf4RndIn.pstSpareBufBase,
                                   FSR_SPARE_BUF_BASE_SIZE);
                    FSR_OAM_MEMCPY(&nSBuf4RndIn[FSR_SPARE_BUF_BASE_SIZE],
                                   stSBuf4RndIn.pstSTLMetaExt,
                                   FSR_SPARE_BUF_EXT_SIZE);

                    if ((nFlag & FSR_LLD_FLAG_CMDIDX_MASK) == FSR_LLD_FLAG_2X_CPBK_PROGRAM)
                    {
                        stSBuf4RndIn.pstSpareBufBase = &stSpareBufBase;
                        stSBuf4RndIn.nNumOfMetaExt = 1;
                        stSBuf4RndIn.pstSTLMetaExt = &stSBuf4Ext[1];

                        /* Get the data into temperary buffer */
                        nLLDRe = _ReadSpare(&stSBuf4RndIn, 
                                            &pstFOReg->nDataSB10[0], 
                                            nFlag);

                        /*
                         * It makes easy to calculate the offset of random-in data 
                         * that Ext1 copies to second Base. Then you can keep going the sequence
                         * without using any 'if' context when you write to random-in data.
                         *
                         * |  16B |  16B |  16B |  16B |  ==> |  16B |  16B |  16B |  16B |
                         * | Base | Ext0 | Base | Ext1 |      | Base | Ext0 | Ext1 | Ext1 |
                         */
                        FSR_OAM_MEMCPY(&nSBuf4RndIn[FSR_SPARE_BUF_BASE_SIZE + FSR_SPARE_BUF_EXT_SIZE],
                                        stSBuf4RndIn.pstSTLMetaExt,
                                        FSR_SPARE_BUF_EXT_SIZE);

                    }
                    bIsRndIn4Spare = TRUE32;
                    break;
                } /* if (pstRIArg->nOffset >= FSR_LLD_CPBK_SPARE) */
            } /* for (nCnt = 0; nCnt < pstCpArg->nRndInCnt; nCnt++) */

            /* 2 plane copy back is considered */
            for (nCnt = 0; nCnt < pstCpArg->nRndInCnt; nCnt++)
            {
                pstRIArg = pstCpArg->pstRndInArg + nCnt;

                /* In case copyback of spare area is requested */
                if (pstRIArg->nOffset >= FSR_LLD_CPBK_SPARE)
                {
                    /* nOffSet means a location which is far from start location of Spare */
                    nOffSet = pstRIArg->nOffset - FSR_LLD_CPBK_SPARE;

                    FSR_OAM_MEMCPY(&nSBuf4RndIn[0] + nOffSet, pstRIArg->pBuf, pstRIArg->nNumOfBytes);
                }
                else
                {
                    _WriteMain(&pstFOReg->nDataMB00[0] + 
                               (nPreUseBuf & 1) * FSR_OND_MDRAM0_SIZE + 
                               pstRIArg->nOffset, 
                              (UINT8 *) pstRIArg->pBuf, 
                              pstRIArg->nNumOfBytes);


#if defined (FSR_LLD_STATISTICS)
                    nBytesTrasferred += pstRIArg->nNumOfBytes;
#endif /* #if defined (FSR_LLD_STATISTICS) */
                }
            } /* for (nCnt = 0; nCnt < pstCpArg->nRndInCnt; nCnt++) */

            if (bIsRndIn4Spare == TRUE32)
            {
                /* Restore pstSTLMetaExt pointer */
                stSBuf4RndIn.pstSpareBufBase = &stSpareBufBase;
                stSBuf4RndIn.nNumOfMetaExt = 1;
                stSBuf4RndIn.pstSTLMetaExt = &stSBuf4Ext[0];

                /* Copy to structure buffer from data array which is occured data random in */
                FSR_OAM_MEMCPY(stSBuf4RndIn.pstSpareBufBase,
                               &nSBuf4RndIn[0],
                               FSR_SPARE_BUF_BASE_SIZE);
                FSR_OAM_MEMCPY(stSBuf4RndIn.pstSTLMetaExt,
                               &nSBuf4RndIn[FSR_SPARE_BUF_BASE_SIZE],
                               FSR_SPARE_BUF_EXT_SIZE);

                _WriteSpare(&pstFOReg->nDataSB00[0] + (nPreUseBuf & 1) * FSR_OND_SDRAM0_SIZE, 
                            &stSBuf4RndIn, 
                            FSR_LLD_FLAG_USE_SPAREBUF | FSR_LLD_FLAG_ECC_ON);

#if defined (FSR_LLD_STATISTICS)
                nBytesTrasferred += (FSR_SPARE_BUF_BASE_SIZE + FSR_SPARE_BUF_EXT_SIZE);
#endif /* #if defined (FSR_LLD_STATISTICS) */

                if ((nFlag & FSR_LLD_FLAG_CMDIDX_MASK) == FSR_LLD_FLAG_2X_CPBK_PROGRAM)
                {
                    /* Restore pstSTLMetaExt pointer */
                    stSBuf4RndIn.pstSpareBufBase = &stSpareBufBase;
                    stSBuf4RndIn.nNumOfMetaExt = 1;
                    stSBuf4RndIn.pstSTLMetaExt = &stSBuf4Ext[1];

                    FSR_OAM_MEMCPY(stSBuf4RndIn.pstSTLMetaExt,
                                   &nSBuf4RndIn[FSR_SPARE_BUF_BASE_SIZE + FSR_SPARE_BUF_EXT_SIZE],
                                   FSR_SPARE_BUF_EXT_SIZE);

                    /* 2x operation wants to load 4k */
                    _WriteSpare(&pstFOReg->nDataSB10[0], 
                                &stSBuf4RndIn, 
                                FSR_LLD_FLAG_USE_SPAREBUF | FSR_LLD_FLAG_ECC_ON);

#if defined (FSR_LLD_STATISTICS)
                    nBytesTrasferred += (FSR_SPARE_BUF_BASE_SIZE + FSR_SPARE_BUF_EXT_SIZE);
#endif /* #if defined (FSR_LLD_STATISTICS) */

                }
            }

            /* 
             * If the previous operation is write, 
             * check busy status, after do random-in operation. 
             */
            if ((pstONDShMem->nPreOp[nDie] == FSR_OND_PREOP_1X_WRITE) || 
                (pstONDShMem->nPreOp[nDie] == FSR_OND_PREOP_2X_WRITE))
            {
                /* Wait until device is ready   */
                WAIT_OND_INT_STAT(pstFOReg, FSR_OND_INT_WI_READY);
            }

            if ((nFlag & FSR_LLD_FLAG_CMDIDX_MASK) == FSR_LLD_FLAG_2X_CPBK_PROGRAM)
            {
                /* Set paired block number */
                nPairedPbn = nPbn + 1;

                /* Set DFS, FBA                    */
                OND_WRITE(pstFOReg->nStartAddr1, 
                          (((UINT16) nPairedPbn << pstONDCxt->nDDPSelSft) & FSR_OND_MASK_DFS) | 
                           ((UINT16) nPairedPbn & pstONDCxt->nFBAMask));

                /* Set FPA (Flash Page Address)  */
                OND_WRITE(pstFOReg->nStartAddr8, 
                          (UINT16) nPgOffset << pstONDCxt->nFPASelSft);

                /* Error check */
                if ((nFlag & FSR_LLD_FLAG_ECC_MASK) == FSR_LLD_FLAG_ECC_ON)
                {
                    nECCRes = OND_READ(pstFOReg->nEccStat);

                    if (nECCRes & pstONDSpec->nECC2LvRdDist)
                    {
                        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
                            (TEXT("[OND:INF]   %s() / %d line\r\n"),
                            __FSR_FUNC__, __LINE__));

                        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
                            (TEXT("            read disturbance at nPbn:%d, nPgOffset:%d\r\n"),
                            pstONDShMem->nPreOpPbn[nDie], pstONDShMem->nPreOpPgOffset[nDie]));

                        pstONDCxt->nECCRes[1] = (UINT32) (FSR_LLD_PREV_2LV_READ_DISTURBANCE | 
                                                          FSR_LLD_1STPLN_CURR_ERROR);

                        nLLDRe = pstONDCxt->nECCRes[0] | pstONDCxt->nECCRes[1];
                    }
                    else if (nECCRes & FSR_OND_ECC_UNCORRECTABLE)
                    {
                        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
                            (TEXT("[OND:INF]   %s() / %d line\r\n"),
                            __FSR_FUNC__, __LINE__));

                        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
                            (TEXT("            read error at nPbn:%d, nPgOffset:%d\r\n"),
                            pstONDShMem->nPreOpPbn[nDie], pstONDShMem->nPreOpPgOffset[nDie]));

                        _DumpRegisters(pstFOReg);
                        _DumpSpareBuffer(pstFOReg, nDev, nDie);

                        pstONDCxt->nECCRes[1] = FSR_OND_ECC_UNCORRECTABLE | 
                                                FSR_LLD_1STPLN_CURR_ERROR;

                        nLLDRe = pstONDCxt->nECCRes[0] | pstONDCxt->nECCRes[1];
                    }
                }
            } /* if ((nFlag & FSR_LLD_FLAG_CMDIDX_MASK) == FSR_LLD_FLAG_2X_CPBK_PROGRAM) */

            /* Set DFS & FBA                */
            OND_WRITE(pstFOReg->nStartAddr1, 
                      (((nPbn << pstONDCxt->nDDPSelSft) & FSR_OND_MASK_DFS) | 
                       (nPbn & pstONDCxt->nFBAMask)));

            /* Write protection can be checked at this point */
            if ((OND_READ(pstFOReg->nWrProtectStat) & FSR_LLD_BLK_STAT_MASK) != FSR_LLD_BLK_STAT_UNLOCKED)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR , 
                               (TEXT("[OND:ERR]   Pbn is write protected\r\n")));

                _DumpRegisters(pstFOReg);

                nLLDRe = (FSR_LLD_WR_PROTECT_ERROR | FSR_LLD_1STPLN_CURR_ERROR);
                break;
            }

            /* Set FPA (Flash Page Address)  */
            OND_WRITE(pstFOReg->nStartAddr8, 
                     (UINT16) nPgOffset << pstONDCxt->nFPASelSft);

            /* BSA is already set in Cpbk_load operation */

            /* Error check */
            if ((nFlag & FSR_LLD_FLAG_ECC_MASK) == FSR_LLD_FLAG_ECC_ON)
            {
                nECCRes = OND_READ(pstFOReg->nEccStat);

                if (nECCRes & pstONDSpec->nECC2LvRdDist)
                {
                    FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
                        (TEXT("[OND:INF]   %s() / %d line\r\n"),
                        __FSR_FUNC__, __LINE__));

                    FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
                        (TEXT("            read disturbance at nPbn:%d, nPgOffset:%d\r\n"),
                        pstONDShMem->nPreOpPbn[nDie], pstONDShMem->nPreOpPgOffset[nDie]));

                    pstONDCxt->nECCRes[0] = (UINT32) (FSR_LLD_PREV_2LV_READ_DISTURBANCE | 
                                                      FSR_LLD_1STPLN_CURR_ERROR);

                    nLLDRe = pstONDCxt->nECCRes[0];
                }
                else if (nECCRes & FSR_OND_ECC_UNCORRECTABLE)
                {
                    pstONDCxt->nECCRes[0] = (UINT32) (FSR_OND_ECC_UNCORRECTABLE | 
                                                      FSR_LLD_1STPLN_CURR_ERROR);

                    nLLDRe = pstONDCxt->nECCRes[0];
                }
            }

            /* 
             * System configuration 1 register has ECC bit which is 9th.
             * bit 0 : with correction, bit 1 : without correction
             */
            if ((nFlag & FSR_LLD_FLAG_ECC_MASK) == FSR_LLD_FLAG_ECC_ON)
            {
                OND_CLR(pstFOReg->nSysConf1, ~0x0100);
            }
            else
            {
                OND_SET(pstFOReg->nSysConf1,  0x0100);
            }


            /* In case of non-blocking mode, interrupt should be enabled */
            if (nFlag & FSR_LLD_FLAG_INT_MASK)
            {
                FSR_OAM_ClrNEnableInt(pstONDCxt->nIntID);
            }
            else
            {
                FSR_OAM_ClrNDisableInt(pstONDCxt->nIntID);
            }

            nCmdIdx = (nFlag & FSR_LLD_FLAG_CMDIDX_MASK) >> 
                      FSR_LLD_FLAG_CMDIDX_BASEBIT;

            OND_WRITE(pstFOReg->nCmd, (UINT16) gnCpBkCmdArray[nCmdIdx]);

            /* Save the information for deffered check */
            pstONDShMem->nPreOp[nDie]         = FSR_OND_PREOP_1X_WRITE;
            pstONDShMem->nPreOpPbn[nDie]      = (UINT16) nPbn;
            pstONDShMem->nPreOpPgOffset[nDie] = (UINT16) nPgOffset;
            pstONDShMem->nPreOpFlag[nDie]     = nFlag;

            if ((nFlag & FSR_LLD_FLAG_CMDIDX_MASK) == FSR_LLD_FLAG_2X_CPBK_PROGRAM)
            {
                pstONDShMem->nPreOp[nDie]         = FSR_OND_PREOP_2X_WRITE;
            }

#if defined (FSR_LLD_LOGGING_HISTORY)
            _AddLog(nDev, nDie);
#endif

#if defined (FSR_LLD_STATISTICS)

            _AddONDStat(nDev, nDie, FSR_OND_STAT_WR_TRANS, nBytesTrasferred, FSR_OND_STAT_NORMAL_CMD);
            _AddONDStat(nDev, nDie, FSR_OND_STAT_SLC_PGM, 0, FSR_OND_STAT_NORMAL_CMD);

#endif /* #if defined (FSR_LLD_STATISTICS) */

        }

    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return (nLLDRe);
}


/**
 * @brief          This function erase block in NAND flash
 *
 * @param[in]      nDev         : Physical Device Number (0 ~ 7)
 * @param[in]      pnPbn        : array of blocks, not necessarilly consecutive
 * @n                             which will be supported in the future
 * @param[in]      nFlag        : FSR_LLD_FLAG_1X_ERASE
 * @n                             FSR_LLD_FLAG_2X_ERASE
 *
 * @return         FSR_LLD_SUCCESS
 * @return         FSR_LLD_INVALID_PARAM
 * @return         FSR_LLD_PREV_ERASE_ERROR | {FSR_LLD_1STPLN_CURR_ERROR}
 * @n              previous erase error
 * @return         FSR_LLD_WR_PROTECT_ERROR | {FSR_LLD_1STPLN_CURR_ERROR}
 * @n              previous erase protection error
 *
 * @author         NamOh Hwang / JeongWook Moon
 * @version        1.0.0
 * @remark         v1.0.0 supports only single plane, one block erase
 *
 */
PUBLIC INT32
FSR_OND_Erase(UINT32  nDev,
              UINT32 *pnPbn,
              UINT32  nNumOfBlks,
              UINT32  nFlag)
{
             OneNANDCxt       *pstONDCxt;
             OneNANDShMem     *pstONDShMem;
             OneNANDSpec      *pstONDSpec;
    volatile OneNANDReg       *pstFOReg;
             UINT32            nCmdIdx;
             INT32             nLLDRe     = FSR_LLD_SUCCESS;
             UINT32            nDie;
             UINT16            nPbn       = (UINT16) *pnPbn;
             UINT16            nWrProtectStat;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG, 
        (TEXT("[OND:IN ] ++%s(nDev:%d, nPbn:%d nNumOfBlks:%d, nFlag:0x%08x\r\n"),
                    __FSR_FUNC__, nDev, pnPbn[0], nNumOfBlks, nFlag));

    do
    {

#if defined (FSR_LLD_STRICT_CHK)
        /* Check device number */
        if (nDev >= FSR_OND_MAX_DEVS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                            (TEXT("[OND:ERR]   %s(nDev:%d, *pnPbn:%d, nNumOfBlks:%d, nFlag:0x%08x) / %d line\r\n"),
                            __FSR_FUNC__, nDev, *pnPbn, nNumOfBlks, nFlag, __LINE__));
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR , 
                            (TEXT("[OND:ERR]   Invalid Device Number (nDev = %d)\r\n"), nDev));
            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */
        
        pstONDCxt   = gpstONDCxt[nDev];
        pstONDShMem = gpstONDShMem[nDev];
        pstONDSpec  = pstONDCxt->pstONDSpec;
        pstFOReg    = (volatile OneNANDReg *) pstONDCxt->nBaseAddr;

#if defined (FSR_LLD_STRICT_CHK)
        /* 
         * Function parameter pnPbn is defined as an array to support multi block erase.
         * though multi block erase is not supported yet.
         * nNumOfBlks can have a value of 1 for now.
         */
        if (nNumOfBlks > 1)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR , 
                           (TEXT("[OND:ERR]   nNumOfBlks is more than one block\r\n")));
            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }

        if (nPbn >= pstONDSpec->nNumOfBlks)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR , 
                           (TEXT("[OND:ERR]   Pbn is higher than upper bound\r\n")));
            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */

        nDie = nPbn >> (FSR_OND_DBS_BASEBIT - pstONDCxt->nDDPSelSft);

        if (pstONDSpec->nNumOfDies == FSR_MAX_DIES)
        {
               FSR_OND_FlushOp(nDev,
                              ((nDie + 0x1) & 0x01),
                              nFlag);
        }
        
        nLLDRe = FSR_OND_FlushOp(nDev, nDie, nFlag);
        
        if (nLLDRe != FSR_LLD_SUCCESS)
        {
            break;
        }

        /* Set DBS                        */
        OND_WRITE(pstFOReg->nStartAddr2, 
                 (nPbn << pstONDCxt->nDDPSelSft) & FSR_OND_MASK_DBS) ;
                 
        /* Set DFS, FBA                   */
        OND_WRITE(pstFOReg->nStartAddr1, 
                 ((nPbn << pstONDCxt->nDDPSelSft) & FSR_OND_MASK_DFS) | 
                  (nPbn & pstONDCxt->nFBAMask));
                  
        /* Write protection can be checked at this point */
        nWrProtectStat = OND_READ(pstFOReg->nWrProtectStat);
        if ((nWrProtectStat & FSR_LLD_BLK_STAT_MASK) != FSR_LLD_BLK_STAT_UNLOCKED)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR , 
                           (TEXT("[OND:ERR]   Pbn is write protected\r\n")));

            _DumpRegisters(pstFOReg);

            nLLDRe = (FSR_LLD_WR_PROTECT_ERROR | FSR_LLD_1STPLN_CURR_ERROR);
            break;
        }

        nCmdIdx = (nFlag & FSR_LLD_FLAG_CMDIDX_MASK) >> 
                  FSR_LLD_FLAG_CMDIDX_BASEBIT;

        /* In case of non-blocking mode, interrupt should be enabled */
        if (nFlag & FSR_LLD_FLAG_INT_MASK)
        {
            FSR_OAM_ClrNEnableInt(pstONDCxt->nIntID);
        }
        else
        {
            FSR_OAM_ClrNDisableInt(pstONDCxt->nIntID);
        }

        OND_WRITE(pstFOReg->nCmd, (UINT16) gnEraseCmdArray[nCmdIdx]);

        /* Save the information for deffered check */
        pstONDShMem->nPreOp[nDie]         = FSR_OND_PREOP_ERASE;
        pstONDShMem->nPreOpPbn[nDie]      = (UINT16) nPbn;
        pstONDShMem->nPreOpPgOffset[nDie] = FSR_OND_PREOP_ADDRESS_NONE;
        pstONDShMem->nPreOpFlag[nDie]     = nFlag;
        pstONDShMem->nPreOpPgOffset[nDie] = 0;

#if defined (FSR_LLD_LOGGING_HISTORY)
        _AddLog(nDev, nDie);
#endif

#if defined (FSR_LLD_STATISTICS)
        _AddONDStat(nDev, nDie, FSR_OND_STAT_ERASE, 0, FSR_OND_STAT_NORMAL_CMD);
#endif /* #if defined (FSR_LLD_STATISTICS) */


    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return (nLLDRe);
}


/**
 * @brief          This function checks a bad block
 *
 * @param[in]      nDev         : Physical Device Number (0 ~ 7)
 * @param[in]      nPbn         : physical block number
 * @param[in]      nFlag        : FSR_LLD_FLAG_1X_CHK_BADBLOCK
 *                                FSR_LLD_FLAG_2X_CHK_BADBLOCK
 *
 * @return         FSR_LLD_INIT_GOODBLOCK
 * @return         FSR_LLD_INIT_BADBLOCK |
 * @n              {FSR_LLD_BAD_BLK_1STPLN, FSR_LLD_BAD_BLK_2NDPLN}
 * @return         FSR_LLD_INVALID_PARAM
 *
 * @author         NamOh Hwang / JeongWook Moon / Jinhyuck Kim
 * @version        1.2.0
 * @remark         
 *
 */
PUBLIC INT32
FSR_OND_ChkBadBlk(UINT32 nDev,
                  UINT32 nPbn,
                  UINT32 nFlag)
{
             OneNANDCxt       *pstONDCxt;
    volatile OneNANDReg       *pstFOReg;
             INT32             nLLDRe;
             INT32             nLLDMinorRe;
             UINT32            nDie;
             UINT16            nDQ;
             UINT32            nPpn;
             UINT32            nCurPbn;
             UINT32            nNumOfPlane;
             UINT32            nPlane;
             BOOL32            bIsBadBlk;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG, 
                    (TEXT("[OND:IN ] ++%s(nDev:%d, nPbn:%d, nFlag:0x%08x)\r\n"), 
                    __FSR_FUNC__, nDev, nPbn, nFlag));

    do
    {

#if defined (FSR_LLD_STRICT_CHK)        
        if (_StrictChk(nDev, nPbn, 0) != FSR_LLD_SUCCESS)
        {            
            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }
#endif

        pstONDCxt  = gpstONDCxt[nDev];
        pstFOReg   = (volatile OneNANDReg *) pstONDCxt->nBaseAddr;

#if defined (FSR_LLD_STRICT_CHK)        
        if ((nFlag & FSR_LLD_FLAG_CMDIDX_MASK) == FSR_LLD_FLAG_2X_CHK_BADBLOCK)            
        {
            if (pstONDCxt->pstONDSpec->nNumOfPlanes != 2)       
            {
                /* Verify that the device to be checked should be 2-plane device */
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR , 
                                (TEXT("[OND:ERR]   Invalid flag (nFlag = %x). \
                                      This flag is used only in 2-plane device\r\n"), nFlag));
                nLLDRe = FSR_LLD_INVALID_PARAM;
                break;
            }
            else if ((nPbn & 1) != 0)
            {
                /* Verify that the block number to be checked should be even in case of 2-plane device */
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR , 
                                (TEXT("[OND:ERR]   Invalid Block Number (nPbn = %d).\
                                      Block Number is even when this flag is used\r\n"), nPbn));
                nLLDRe = FSR_LLD_INVALID_PARAM;
                break;
            }            
        }
#endif

        nDie = (nPbn >> (FSR_OND_DFS_BASEBIT - pstONDCxt->nDDPSelSft)) & 0x1;

        if ((nFlag & FSR_LLD_FLAG_CMDIDX_MASK) == FSR_LLD_FLAG_1X_CHK_BADBLOCK)
        {
            nNumOfPlane = 1;
        }
        else if ((nFlag & FSR_LLD_FLAG_CMDIDX_MASK) == FSR_LLD_FLAG_2X_CHK_BADBLOCK)
        {
            nNumOfPlane = 2;
        }
        else
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR , 
                            (TEXT("[OND:ERR]   Invalid Flag (%x)\r\n"), nFlag));
            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }

        nLLDMinorRe = 0;

        /* Read bad mark and check it */
        for (nPlane = 0; nPlane < nNumOfPlane; nPlane++)
        {   
            nCurPbn     = nPbn + nPlane;
            bIsBadBlk   = FALSE32;

            /* Check "FFFF" at the 1st word of sector 0 of spare area in 1st and 2nd page */
            for (nPpn = 0; nPpn < FSR_OND_NUM_OF_BAD_MARK_PAGES; nPpn++)
            {
                FSR_OND_ReadOptimal(nDev,
                                    nCurPbn,
                                    nPpn,
                                    NULL,
                                    NULL,
                                    (FSR_LLD_FLAG_ECC_OFF | FSR_LLD_FLAG_1X_LOAD));
                

                FSR_OND_ReadOptimal(nDev,
                                    nCurPbn,
                                    nPpn,
                                    NULL,
                                    NULL,
                                    (FSR_LLD_FLAG_ECC_OFF | FSR_LLD_FLAG_TRANSFER));

                /* Read bad mark from spare area in DataRAM directly */
                nDQ = OND_READ(*(volatile UINT16 *) (&pstFOReg->nDataSB00[0] + 
                                    (pstONDCxt->nPreUseBuf[nDie] & 1) * 
                                    FSR_OND_SDRAM0_SIZE));
                
                /* Simply check if this bad information is valid block mark in 2K page OneNAND */
                if (nDQ != (UINT16) FSR_OND_VALID_BLK_MARK)
                {
                    bIsBadBlk = TRUE32;
                }
                else
                {
                    bIsBadBlk = FALSE32;
                    break;
                }
            }
            
            if (bIsBadBlk == TRUE32)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                               (TEXT("[OND:   ]   (CHK BB) nPbn = %d is a bad block\r\n"), nCurPbn));

                nLLDMinorRe |= (nPlane == 0) ? FSR_LLD_BAD_BLK_1STPLN : FSR_LLD_BAD_BLK_2NDPLN;
            }
            else
            {
                FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, 
                               (TEXT("[OND:   ]   (CHK BB) nPbn = %d is a good block\r\n"), nCurPbn));
            }
        }
                
        nLLDRe = (nLLDMinorRe != 0) ? (FSR_LLD_INIT_BADBLOCK | nLLDMinorRe) : FSR_LLD_INIT_GOODBLOCK;

#if defined (FSR_LLD_STATISTICS)
        _AddONDStat(nDev, nDie, FSR_OND_STAT_SLC_LOAD, 0, FSR_OND_STAT_NORMAL_CMD);
#endif /* #if defined (FSR_LLD_STATISTICS) */

    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return (nLLDRe);
}

/**
 * @brief          After calling series of FSR_OND_Write(), FSR_OND_Erase(), FSR_OND_CopyBack(),
 * @n              FSR_OND_FlushOp is called. 
 *
 * @param[in]      nDev         : Physical Device Number (0 ~ 7)
 *                 nDieIdx      : 0 is for 1st die
 *                              : 1 is for 2nd die
 * @param[in]      nFlag        : FSR_LLD_FLAG_NONE or FSR_LLD_FLAG_REMAIN_PREOP_STAT
 *
 * @return         FSR_LLD_SUCCESS
 * @return         FSR_LLD_PREV_WRITE_ERROR | 
 * @n              {FSR_LLD_1STPLN_PREV_ERROR, FSR_LLD_1STPLN_CURR_ERROR, FSR_LLD_2NDPLN_PREV_ERROR, FSR_LLD_2NDPLN_CURR_ERROR}
 * @return         FSR_LLD_PREV_ERASE_ERROR | 
 * @n              {FSR_LLD_1STPLN_CURR_ERROR | FSR_LLD_2NDPLN_CURR_ERROR}
 * @return         FSR_LLD_INVALID_PARAM
 *
 * @author         NamOh Hwang / JeongWook Moon
 * @version        1.0.0
 * @remark         this function completes previous operation, and returns previous error.
 *
 */
PUBLIC INT32
FSR_OND_FlushOp(UINT32 nDev,
                UINT32 nDieIdx,
                UINT32 nFlag)
{
             OneNANDCxt       *pstONDCxt;
             OneNANDShMem     *pstONDShMem;
             OneNANDSpec      *pstONDSpec;
    volatile OneNANDReg       *pstFOReg;
             INT32             nLLDRe       = FSR_LLD_SUCCESS;
             UINT32            nPrevOp;

#if !defined(FSR_OAM_RTLMSG_DISABLE)
             UINT32            nPrevPbn;
             UINT32            nPrevPgOffset;
#endif

             UINT32            nPrevFlag;
             UINT32            nMasterInt   = 0;
             UINT16            nECCRes;
             UINT16            nCtrlStat;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG, 
                    (TEXT("[OND:IN ] ++%s(nDev: %d, nDieIdx: %d, nFlag: 0x%08x)\r\n"), 
                    __FSR_FUNC__, nDev, nDieIdx, nFlag));

    do
    {

#if defined (FSR_LLD_STRICT_CHK)
        /* Check device number */
        if (nDev >= FSR_OND_MAX_DEVS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                            (TEXT("[OND:ERR]   %s(nDev:%d, nDie:%d, nFlag:0x%08x) / %d line\r\n"),
                            __FSR_FUNC__, nDev, nDieIdx, nFlag, __LINE__));
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR , 
                           (TEXT("[OND:ERR]   Invalid Device Number (nDev = %d)\r\n"), nDev));
            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }
#endif

        pstONDCxt   = gpstONDCxt[nDev];
        pstONDShMem = gpstONDShMem[nDev];
        pstONDSpec  = pstONDCxt->pstONDSpec;
        pstFOReg    = (volatile OneNANDReg *) pstONDCxt->nBaseAddr;

        nPrevOp       = pstONDShMem->nPreOp[nDieIdx];
        nPrevFlag     = pstONDShMem->nPreOpFlag[nDieIdx];

#if !defined(FSR_OAM_RTLMSG_DISABLE)
        nPrevPbn      = pstONDShMem->nPreOpPbn[nDieIdx];
        nPrevPgOffset = pstONDShMem->nPreOpPgOffset[nDieIdx];
#endif

        /* Set DBS */
        OND_WRITE(pstFOReg->nStartAddr2, 
                 (UINT16)(nDieIdx << FSR_OND_DBS_BASEBIT));


        switch (nPrevOp)
        {
        case FSR_OND_PREOP_NONE:
            /* 
             * DO NOT remove this code : 'case FSR_OND_PREOP_NONE:'
             * for compiler optimization 
             */
            break;

        case FSR_OND_PREOP_READ:

            WAIT_OND_INT_STAT(pstFOReg, FSR_OND_INT_RI_READY);

            if ((nPrevFlag & FSR_LLD_FLAG_ECC_MASK) == FSR_LLD_FLAG_ECC_ON)
            {
                nECCRes = OND_READ(pstFOReg->nEccStat);

                if (nECCRes & FSR_OND_ECC_UNCORRECTABLE)
                {
                    nLLDRe = FSR_LLD_PREV_READ_ERROR | 
                             FSR_LLD_1STPLN_CURR_ERROR;

                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[OND:ERR]   %s(nDev:%d, nDie:%d, nFlag:0x%08x) / %d line\r\n"),
                    __FSR_FUNC__, nDev, nDieIdx, nFlag, __LINE__));

                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                        (TEXT("            prev Op:%d @ nDev:%d, nPbn:%d, nPgOffset:%d, nFlag:0x%08x\r\n"),
                        nPrevOp, nDev, nPrevPbn, nPrevPgOffset, nPrevFlag));

                    FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
                        (TEXT("            read error at nPbn:%d, nPgOffset:%d\r\n"),
                        pstONDShMem->nPreOpPbn[nDieIdx], pstONDShMem->nPreOpPgOffset[nDieIdx]));

                    _DumpRegisters(pstFOReg);
                    _DumpSpareBuffer(pstFOReg, nDev, nDieIdx);
                }
                else if (nECCRes & pstONDSpec->nECC2LvRdDist)
                {
                    nLLDRe = FSR_LLD_PREV_2LV_READ_DISTURBANCE | 
                             FSR_LLD_1STPLN_CURR_ERROR;

                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[OND:INF]   %s(nDev:%d, nDie:%d, nFlag:0x%08x) / %d line\r\n"),
                    __FSR_FUNC__, nDev, nDieIdx, nFlag, __LINE__));

                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                        (TEXT("            prev Op:%d @ nDev:%d, nPbn:%d, nPgOffset:%d, nFlag:0x%08x\r\n"),
                        nPrevOp, nDev, nPrevPbn, nPrevPgOffset, nPrevFlag));

                    FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
                        (TEXT("            read disturbance at nPbn:%d, nPgOffset:%d\r\n"),
                        pstONDShMem->nPreOpPbn[nDieIdx], pstONDShMem->nPreOpPgOffset[nDieIdx]));

                    _DumpRegisters(pstFOReg);
                    _DumpSpareBuffer(pstFOReg, nDev, nDieIdx);
                }
            }
            break;

        case FSR_OND_PREOP_1X_WRITE:

            /* 
             * Wait for the device ready
             * Macro below is for OneNAND
             * it polls the INT bit, and if INT bit doesn't go up, it returns FSR_LLD_NO_RESPONSE
             */
            WAIT_OND_INT_STAT(pstFOReg, FSR_OND_INT_WI_READY);

            /* 
             * Previous error check
             * in case of Cache Program, Error bit shows the accumulative error status of Cache Program
             * so if an error occurs this bit stays as fail 
             */
            nCtrlStat = OND_READ(pstFOReg->nCtrlStat);

            if ((nCtrlStat & FSR_OND_STATUS_ERROR) == FSR_OND_STATUS_ERROR)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s(nDev:%d, nDie:%d, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, nDieIdx, nFlag, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            prev Op:%d @ nDev:%d, nPbn:%d, nPgOffset:%d, nFlag:0x%08x\r\n"),
                    nPrevOp, nDev, nPrevPbn, nPrevPgOffset, nPrevFlag));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR , 
                               (TEXT("[OND:ERR]   write error (nDieIdx = %d, line = %d)\r\n"), nDieIdx, __LINE__));

                nLLDRe = FSR_LLD_PREV_WRITE_ERROR;

                /* 
                 * There is not 1xCache program in OneNAND Device
                 * So, you need not to consider cache operation at 1x program.
                 * Just return minor error by 1 plane current bit.
                 */
                nLLDRe |= FSR_LLD_1STPLN_CURR_ERROR;

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR , 
                               (TEXT("[OND:ERR]   previous write error: previous - 1st Plane (nDieIdx = %d, line = %d)\r\n"), nDieIdx, __LINE__));

                _DumpRegisters(pstFOReg);
            }
            break;

        case FSR_OND_PREOP_CACHE_PGM:
        case FSR_OND_PREOP_2X_WRITE:
            /* 
             * Wait for the device ready
             * Macro below is for OneNAND
             * it polls the INT bit, and if INT bit doesn't go up, it returns FSR_LLD_NO_RESPONSE
             */
            if (nPrevOp == FSR_OND_PREOP_CACHE_PGM)
            {
                /* Master INT */
                nMasterInt = FSR_OND_INT_READY; 
            }
            else
            {
                nMasterInt = 0;
            }

            WAIT_OND_INT_STAT(pstFOReg, (nMasterInt | FSR_OND_INT_WI_READY));

            /* 
             * Previous error check
             * in case of Cache Program, Error bit shows the accumulative error status of Cache Program
             * so if an error occurs this bit stays as fail 
             */
            nCtrlStat = OND_READ(pstFOReg->nCtrlStat);

            if ((nCtrlStat & FSR_OND_STATUS_ERROR) == FSR_OND_STATUS_ERROR)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s(nDev:%d, nDie:%d, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, nDieIdx, nFlag, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            prev Op:%d @ nDev:%d, nPbn:%d, nPgOffset:%d, nFlag:0x%08x\r\n"),
                    nPrevOp, nDev, nPrevPbn, nPrevPgOffset, nPrevFlag));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR , 
                               (TEXT("[OND:ERR]   write error (nDieIdx = %d, line = %d)\r\n"), nDieIdx, __LINE__));

                nLLDRe = FSR_LLD_PREV_WRITE_ERROR;

                /* 
                 * If previous bit of controller status register is set, 
                 * Cache Program needs to be restarted from the page that occurs an error
                 * so here, returns previous bit of status register
                 */
                if (nCtrlStat & FSR_OND_STATUS_ERROR_PREV1)
                {
                    nLLDRe |= FSR_LLD_1STPLN_PREV_ERROR;

                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR , 
                                   (TEXT("[OND:ERR]   previous write error: previous - 1st Plane (nDieIdx = %d, line = %d)\r\n"), nDieIdx, __LINE__));
                }

                if (nCtrlStat & FSR_OND_STATUS_ERROR_CURR1)
                {
                    nLLDRe |= FSR_LLD_1STPLN_CURR_ERROR;

                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR , 
                                   (TEXT("[OND:ERR]   previous write error: current - 1st Plane (nDieIdx = %d, line = %d)\r\n"), nDieIdx, __LINE__));
                }

                if (nCtrlStat & FSR_OND_STATUS_ERROR_PREV2)
                {
                    nLLDRe |= FSR_LLD_2NDPLN_PREV_ERROR;

                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR , 
                                   (TEXT("[OND:ERR]   previous write error: previous - 2nd Plane (nDieIdx = %d, line = %d)\r\n"), nDieIdx, __LINE__));
                }

                if (nCtrlStat & FSR_OND_STATUS_ERROR_CURR2)
                {
                    nLLDRe |= FSR_LLD_2NDPLN_CURR_ERROR;

                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR , 
                                   (TEXT("[OND:ERR]   previous write error: current - 2nd Plane (nDieIdx = %d, line = %d)\r\n"), nDieIdx, __LINE__));
                }

                _DumpRegisters(pstFOReg);
            }
            break;

        case FSR_OND_PREOP_ERASE:
            /* 
             * Wait for the device ready
             * Macro below is for OneNAND
             * it polls the INT bit, and if INT bit doesn't go up, it returns FSR_LLD_NO_RESPONSE
             */
            WAIT_OND_INT_STAT(pstFOReg, FSR_OND_INT_EI_READY);

            nCtrlStat = OND_READ(pstFOReg->nCtrlStat);

            /* Previous error check */
            if ((nCtrlStat & FSR_OND_STATUS_ERROR) == FSR_OND_STATUS_ERROR)
            {
                nLLDRe = (FSR_LLD_PREV_ERASE_ERROR | FSR_LLD_1STPLN_CURR_ERROR);

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s(nDev:%d, nDie:%d, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, nDieIdx, nFlag, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            prev Op:%d @ nDev:%d, nPbn:%d, nPgOffset:%d, nFlag:0x%08x\r\n"),
                    nPrevOp, nDev, nPrevPbn, nPrevPgOffset, nPrevFlag));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR , 
                    (TEXT("[OND:ERR]   previous erase error (nDieIdx = %d, line = %d)\r\n"), nDieIdx, __LINE__));

                _DumpRegisters(pstFOReg);
            }
            break;

        default:
            WAIT_OND_INT_STAT(pstFOReg, FSR_OND_INT_READY);
            break;
        }

        if ((nFlag & FSR_LLD_FLAG_REMAIN_PREOP_STAT) != FSR_LLD_FLAG_REMAIN_PREOP_STAT)
        {
            pstONDShMem->nPreOp[nDieIdx] = FSR_OND_PREOP_NONE;
        }

#if defined (FSR_LLD_STATISTICS)
        _AddONDStat(nDev, nDieIdx, FSR_OND_STAT_FLUSH, 0, FSR_OND_STAT_NORMAL_CMD);
#endif
    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return (nLLDRe);
}


/**
 * @brief          This function reports device information to upper layer.
 *
 * @param[in]      nDev         : Physical Device Number (0 ~ 7)
 * @param[out]     pstDevSpec   : pointer to the device spec
 * @param[in]      nFlag        : FSR_LLD_FLAG_NONE
 *
 * @return         FSR_LLD_SUCCESS
 * @return         FSR_LLD_INVALID_PARAM
 *
 * @author         NamOh Hwang / JeongWook Moon
 * @version        1.0.0
 * @remark
 *
 */
PUBLIC INT32
FSR_OND_GetDevSpec(UINT32      nDev,
                   FSRDevSpec *pstDevSpec,
                   UINT32      nFlag)
{
    OneNANDSpec *pstONDSpec;
    OneNANDCxt  *pstONDCxt;
    INT32        nLLDRe         = FSR_LLD_SUCCESS;
    UINT32       nDieIdx;
    UINT32       nDie           = 0;
    UINT32       nTINYFlag      = FSR_LLD_FLAG_NONE;    /* LLD flag for TINY FSR    */

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG, 
                   (TEXT("[OND:IN ] ++%s(nDev:%d )\r\n"), __FSR_FUNC__, nDev));

    do 
    {

#if defined (FSR_LLD_STRICT_CHK)
        /* Check device number */
        if (nDev >= FSR_OND_MAX_DEVS)
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

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR , 
                           (TEXT("[OND:ERR]   (GetDevSpec)parameter pstDevSpec is NULL\r\n")));
            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */

        pstONDSpec = gpstONDCxt[nDev]->pstONDSpec;
        pstONDCxt  = gpstONDCxt[nDev];

        FSR_OAM_MEMSET(pstDevSpec, 0x00, sizeof(FSRDevSpec));

        pstDevSpec->nNumOfBlks                      = pstONDSpec->nNumOfBlks;
        pstDevSpec->nNumOfPlanes                    = pstONDSpec->nNumOfPlanes;
        pstDevSpec->nNumOfBlksIn1stDie              = pstONDSpec->nNumOfBlksIn1stDie;
        pstDevSpec->nDID                            = pstONDSpec->nDID;

        for (nDieIdx = 0; nDieIdx < pstONDSpec->nNumOfDies; nDieIdx++)
        {
            pstDevSpec->nBlksForSLCArea[nDieIdx]    = gpstONDCxt[nDev]->nBlksForSLCArea[nDieIdx];
        }

        pstDevSpec->nSctsPerPG                      = pstONDSpec->nSctsPerPG;
        pstDevSpec->nSparePerSct                    = pstONDSpec->nSparePerSct;
        pstDevSpec->nPgsPerBlkForSLC                = pstONDSpec->nPgsPerBlkForSLC;
        pstDevSpec->nPgsPerBlkForMLC                = 0;
        pstDevSpec->nNumOfDies                      = pstONDSpec->nNumOfDies;

        if (pstONDSpec->nDID == FSR_OND_3G_DDP_DID)
        {
            pstDevSpec->nNumOfDies = 1;
        }

        pstDevSpec->nUserOTPScts                    = pstONDSpec->nUserOTPScts;
        pstDevSpec->b1stBlkOTP                      = pstONDSpec->b1stBlkOTP;
        pstDevSpec->nRsvBlksInDev                   = pstONDSpec->nNumOfRsvrInSLC;
        pstDevSpec->pPairedPgMap                    = NULL;
        pstDevSpec->nNANDType                       = FSR_LLD_SLC_ONENAND;
        pstDevSpec->bCachePgm                       = TRUE32;   /* Always TRUE32 */
        pstDevSpec->nSLCTLoadTime                   = pstONDSpec->nSLCTLoadTime;
        pstDevSpec->nMLCTLoadTime                   = 0;
        pstDevSpec->nSLCTProgTime                   = pstONDSpec->nSLCTProgTime;
        pstDevSpec->nMLCTProgTime[0]                = 0;
        pstDevSpec->nMLCTProgTime[1]                = 0;
        pstDevSpec->nTEraseTime                     = pstONDSpec->nTEraseTime;

        /* Time for transfering 1 page in usec */
        pstDevSpec->nWrTranferTime = (pstONDSpec->nSctsPerPG * 
                                      (FSR_OND_SECTOR_SIZE + pstONDSpec->nSparePerSct)) * 
                                     pstONDCxt->nWrTranferTime / 2 / 1000;
        pstDevSpec->nRdTranferTime = (pstONDSpec->nSctsPerPG * 
                                      (FSR_OND_SECTOR_SIZE + pstONDSpec->nSparePerSct)) * 
                                     pstONDCxt->nRdTranferTime / 2 / 1000;

        pstDevSpec->nSLCPECycle    = pstONDSpec->nSLCPECycle;

        if (pstONDSpec->nNumOfDies == FSR_MAX_DIES)
        {
               FSR_OND_FlushOp(nDev,
                          ((nDie + 0x1) & 0x01),
                          FSR_LLD_FLAG_NONE | nTINYFlag);
        }

        nLLDRe = FSR_OND_FlushOp(nDev, nDie, FSR_LLD_FLAG_NONE | nTINYFlag);

        /* Get UID */
        _GetUniqueID(nDev, pstONDCxt, nFlag);

        FSR_OAM_MEMCPY(&pstDevSpec->nUID[0], &pstONDCxt->nUID[0] , sizeof(UINT8) * 16);
    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return (nLLDRe);
}



/**
 * @brief          This function locks block tight
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
_LockTightBlocks(UINT32  nDev, 
                 UINT32  nSbn, 
                 UINT32  nBlks,
                 UINT32 *pnErrPbn)
{
             OneNANDCxt       *pstONDCxt    = gpstONDCxt[nDev];
             OneNANDShMem     *pstONDShMem  = gpstONDShMem[nDev];
    volatile OneNANDReg       *pstFOReg     = (volatile OneNANDReg *) pstONDCxt->nBaseAddr;
             INT32             nLLDRe       = FSR_LLD_SUCCESS;
             UINT32            nDieIdx;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[OND:IN ] ++%s(nDev : %d, Start BlkNum : %d, NumOfBlks : %d) \r\n"),
                    __FSR_FUNC__, nDev, nSbn, nBlks));

    while (nBlks > 0)
    {
        nDieIdx                    = (nSbn >> (FSR_OND_DFS_BASEBIT - pstONDCxt->nDDPSelSft)) & 0x1;
        pstONDShMem->nPreOp[nDieIdx]          = FSR_OND_PREOP_IOCTL;
        pstONDShMem->nPreOpPbn[nDieIdx]       = (UINT16) nSbn;
        pstONDShMem->nPreOpPgOffset[nDieIdx]  = 0;

        /* Set DBS                         */
        OND_WRITE(pstFOReg->nStartAddr2, 
                 ((UINT16) nSbn << pstONDCxt->nDDPSelSft) & FSR_OND_MASK_DBS) ;

        /* Set DFS & FBA */
        OND_WRITE(pstFOReg->nStartAddr1, 
                 (((UINT16) nSbn << pstONDCxt->nDDPSelSft) & FSR_OND_MASK_DFS) | 
                  ((UINT16) nSbn & pstONDCxt->nFBAMask));

        /* The block should be locked first to lock blocks tight */
        if ((OND_READ(pstFOReg->nWrProtectStat) & FSR_LLD_BLK_STAT_MASK) != FSR_LLD_BLK_STAT_LOCKED)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR , 
                           (TEXT("[OND:ERR]   Lock a block first before lock tight a block\r\n")));

            _DumpRegisters(pstFOReg);

            nLLDRe =  FSR_LLD_INVALID_BLOCK_STATE;
            break;
        }

        OND_WRITE(pstFOReg->nStartBlkAddr, ((UINT16) nSbn & pstONDCxt->nFBAMask));

        /* 0x002A is a command for Locking a block tight */
        OND_WRITE(pstFOReg->nCmd, 0x002C);

        WAIT_OND_INT_STAT       (pstFOReg, FSR_OND_INT_READY);

        /* Check CtrlStat register */
        if (OND_READ(pstFOReg->nCtrlStat) & FSR_OND_STATUS_ERROR)
        {
            *pnErrPbn = nSbn;

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s() / %d line\r\n"),
                __FSR_FUNC__, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Pbn #%d is fail lock tight\r\n"), nSbn));

            _DumpRegisters(pstFOReg);
            _DumpSpareBuffer(pstFOReg, nDev, nDieIdx);

            nLLDRe = FSR_LLD_INVALID_BLOCK_STATE;
            break;
        }

        /* set DFS, FBA                    */
        OND_WRITE(pstFOReg->nStartAddr1, 
                  (((UINT16) nSbn << pstONDCxt->nDDPSelSft) & FSR_OND_MASK_DFS) | 
                   ((UINT16) nSbn & pstONDCxt->nFBAMask));

        /* Wait WR_PROTECT_STAT */
        WAIT_OND_WR_PROTECT_STAT(pstFOReg, FSR_LLD_BLK_STAT_LOCKED_TIGHT, nDev, nDieIdx);

        nBlks--;
        nSbn++;
    }
  
    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[OND:OUT] --%s\r\n"), __FSR_FUNC__));

    return (nLLDRe);
}

/**
 * @brief          This function locks block
 *
 * @param[in]      nDev         : Physical Device Number (0 ~ 7)
 * @param[in]      nSbn         : start block number
 * @param[in]      nBlks        : the number of blocks to lock tight
 * @param[out]     pnErrorPbn   : physical block number where
 * @n                             command (unlock block) fails.
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
_LockBlocks(UINT32  nDev, 
            UINT32  nSbn, 
            UINT32  nBlks,
            UINT32 *pnErrPbn)
{
             OneNANDCxt       *pstONDCxt    = gpstONDCxt[nDev];
             OneNANDShMem     *pstONDShMem  = gpstONDShMem[nDev];
    volatile OneNANDReg       *pstFOReg     = (volatile OneNANDReg *) pstONDCxt->nBaseAddr;
             UINT16            nLockStat;
             INT32             nLLDRe       = FSR_LLD_SUCCESS;
             UINT32            nDieIdx;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[OND:IN ] ++%s(nDev : %d, Start BlkNum : %d, NumOfBlks : %d) \r\n"),
                    __FSR_FUNC__, nDev, nSbn, nBlks));

    while (nBlks > 0)
    {
        nDieIdx                    = (nSbn >> (FSR_OND_DFS_BASEBIT - pstONDCxt->nDDPSelSft)) & 0x1;
        pstONDShMem->nPreOp[nDieIdx]          = FSR_OND_PREOP_IOCTL;
        pstONDShMem->nPreOpPbn[nDieIdx]       = (UINT16) nSbn;
        pstONDShMem->nPreOpPgOffset[nDieIdx]  = 0;

        /* Set DBS                         */
        OND_WRITE(pstFOReg->nStartAddr2, 
                 ((UINT16) nSbn << pstONDCxt->nDDPSelSft) & FSR_OND_MASK_DBS) ;

        /* Set DFS & FBA */
        OND_WRITE(pstFOReg->nStartAddr1, 
                 (((UINT16) nSbn << pstONDCxt->nDDPSelSft) & FSR_OND_MASK_DFS) | 
                  ((UINT16) nSbn & pstONDCxt->nFBAMask));

        OND_WRITE(pstFOReg->nStartBlkAddr, ((UINT16) nSbn & pstONDCxt->nFBAMask));

        nLockStat = OND_READ(pstFOReg->nWrProtectStat) & FSR_LLD_BLK_STAT_MASK;

        /* If the block is locked tight, then it cannot go to locked state */
        if (nLockStat == FSR_LLD_BLK_STAT_LOCKED_TIGHT)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s() / %d line\r\n"),
                __FSR_FUNC__, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Pbn #%d is locked tight\r\n"), nSbn));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            locked-tight block cannot go to locked state\r\n")));

            _DumpRegisters(pstFOReg);

            nLLDRe = FSR_LLD_INVALID_BLOCK_STATE;
            break;
        }

        /* 0x002A is a command for Locking a block */
        OND_WRITE(pstFOReg->nCmd, 0x002A);

        WAIT_OND_INT_STAT(pstFOReg, FSR_OND_INT_READY);

        /* Check CtrlStat register */
        if (OND_READ(pstFOReg->nCtrlStat) & FSR_OND_STATUS_ERROR)
        {
            *pnErrPbn = nSbn;

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s() / %d line\r\n"),
                __FSR_FUNC__, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Pbn #%d is fail lock tight\r\n"), nSbn));

            _DumpRegisters(pstFOReg);
            _DumpSpareBuffer(pstFOReg, nDev, nDieIdx);

            nLLDRe = FSR_LLD_INVALID_BLOCK_STATE;
            break;
        }

        /* Set DFS, FBA                    */
        OND_WRITE(pstFOReg->nStartAddr1, 
                  (((UINT16) nSbn << pstONDCxt->nDDPSelSft) & FSR_OND_MASK_DFS) | 
                   ((UINT16) nSbn & pstONDCxt->nFBAMask));

        /* Wait WR_PROTECT_STAT */
        WAIT_OND_WR_PROTECT_STAT(pstFOReg, FSR_LLD_BLK_STAT_LOCKED, nDev, nDieIdx);

        nBlks--;
        nSbn++;
    }
  
    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[OND:OUT] --%s\r\n"), __FSR_FUNC__));

    return (nLLDRe);
}

/**
 * @brief          This function unlocks block
 *
 * @param[in]      nDev         : Physical Device Number (0 ~ 7)
 * @param[in]      nSbn         : start block number
 * @param[in]      nBlks        : the number of blocks to lock tight
 * @param[out]     pnErrorPbn   : physical block number where
 * @n                             command (unlock block) fails.
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
_UnLockBlocks(UINT32  nDev, 
              UINT32  nSbn, 
              UINT32  nBlks,
              UINT32 *pnErrPbn)
{
             OneNANDCxt       *pstONDCxt    = gpstONDCxt[nDev];
             OneNANDShMem     *pstONDShMem  = gpstONDShMem[nDev];
    volatile OneNANDReg       *pstFOReg     = (volatile OneNANDReg *) pstONDCxt->nBaseAddr;
             UINT16            nLockStat;
             INT32             nLLDRe       = FSR_LLD_SUCCESS;
             UINT32            nDieIdx;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[OND:IN ] ++%s(nDev : %d, Start BlkNum : %d, NumOfBlks : %d) \r\n"),
                    __FSR_FUNC__, nDev, nSbn, nBlks));
    
    while (nBlks > 0)
    {
        nDieIdx                    = (nSbn >> (FSR_OND_DFS_BASEBIT - pstONDCxt->nDDPSelSft)) & 0x1;
        pstONDShMem->nPreOp[nDieIdx]          = FSR_OND_PREOP_IOCTL;
        pstONDShMem->nPreOpPbn[nDieIdx]       = (UINT16) nSbn;
        pstONDShMem->nPreOpPgOffset[nDieIdx]  = 0;

        /* Set DBS                         */
        OND_WRITE(pstFOReg->nStartAddr2, 
                 ((UINT16) nSbn << pstONDCxt->nDDPSelSft) & FSR_OND_MASK_DBS);

        /* Set DFS & FBA */
        OND_WRITE(pstFOReg->nStartAddr1, 
                 (((UINT16) nSbn << pstONDCxt->nDDPSelSft) & FSR_OND_MASK_DFS) | 
                  ((UINT16) nSbn & pstONDCxt->nFBAMask));

        OND_WRITE(pstFOReg->nStartBlkAddr, ((UINT16) nSbn & pstONDCxt->nFBAMask));

                nLockStat = OND_READ(pstFOReg->nWrProtectStat) & FSR_LLD_BLK_STAT_MASK;

        /* If the block is locked tightly, then it cannot go to locked state */
        if (nLockStat == FSR_LLD_BLK_STAT_LOCKED_TIGHT)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s() / %d line\r\n"),
                __FSR_FUNC__, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Pbn #%d is locked tight\r\n"), nSbn));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            locked-tight block cannot go to unlocked state\r\n")));

            _DumpRegisters(pstFOReg);

            nLLDRe = FSR_LLD_INVALID_BLOCK_STATE;
            break;
        }

        /* 0x002A is a command for UnLocking a block */
        OND_WRITE(pstFOReg->nCmd, 0x0023);

        WAIT_OND_INT_STAT(pstFOReg, FSR_OND_INT_READY);

        /* Check CtrlStat register */
        if (OND_READ(pstFOReg->nCtrlStat) & FSR_OND_STATUS_ERROR)
        {
            *pnErrPbn = nSbn;

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s() / %d line\r\n"),
                __FSR_FUNC__, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Pbn #%d is fail lock tight\r\n"), nSbn));

            _DumpRegisters(pstFOReg);
            _DumpSpareBuffer(pstFOReg, nDev, nDieIdx);

            nLLDRe = FSR_LLD_INVALID_BLOCK_STATE;
            break;
        }

        /* Set DFS, FBA                    */
        OND_WRITE(pstFOReg->nStartAddr1, 
                  (((UINT16) nSbn << pstONDCxt->nDDPSelSft) & FSR_OND_MASK_DFS) | 
                   ((UINT16) nSbn & pstONDCxt->nFBAMask));

        /* Wait WR_PROTECT_STAT */
        WAIT_OND_WR_PROTECT_STAT(pstFOReg, FSR_LLD_BLK_STAT_UNLOCKED, nDev, nDieIdx);

        nBlks--;
        nSbn++;
    }
  
    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[OND:OUT] --%s\r\n"), __FSR_FUNC__));

    return (nLLDRe);
}



/**
 * @brief           This function searches an unlocked block and returns the pbn
 *
 * @param[in]       nDev        : Physical Device Number
 * @param[in]       nDie        : Physical Device Die
 * @param[out]     *pnPbn       : unlocked block pbn
 *
 * @return          TRUE32      : found
  * @n              FALSE32     : not found
 *
 * @author          Jinhyuck Kim
 * @version         1.2.1
 * @remark
 *
 */
PRIVATE
BOOL32 _SearchUnlockedPbn(UINT32    nDev,
                          UINT32    nDie,
                          UINT16   *pnPbn)
{
             OneNANDCxt       *pstONDCxt;             
             OneNANDSpec      *pstONDSpec;
    volatile OneNANDReg       *pstFOReg;   
             UINT32            nIdx;
             UINT32            nWrProtectStat;    

    pstONDCxt   = gpstONDCxt[nDev];    
    pstONDSpec  = pstONDCxt->pstONDSpec;
    pstFOReg    = (volatile OneNANDReg *) pstONDCxt->nBaseAddr;
    
    /* Set DBS                         */
    OND_WRITE(pstFOReg->nStartAddr2, 
              (UINT16) (nDie << pstONDCxt->nDDPSelSft));

    /* Backward search */
    nIdx = pstONDSpec->nNumOfBlksIn1stDie - 1;

    do
    {
        /* Set DFS, FBA                    */
        OND_WRITE(pstFOReg->nStartAddr1, 
                  (((UINT16) nIdx << pstONDCxt->nDDPSelSft) & FSR_OND_MASK_DFS) | 
                   ((UINT16) nIdx & pstONDCxt->nFBAMask));

        nWrProtectStat = OND_READ(pstFOReg->nWrProtectStat);

        if ((nWrProtectStat & FSR_LLD_BLK_STAT_MASK) == FSR_LLD_BLK_STAT_UNLOCKED) 
        {
            *pnPbn = (UINT16)nIdx;
            return TRUE32;
        }
    }while(nIdx-- > 0);

    return FALSE32;
}

/**
 * @brief           This function locks the OTP block, 1st OTP block
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
            OneNANDCxt         *pstONDCxt  = gpstONDCxt[nDev];
            OneNANDSpec        *pstONDSpec = pstONDCxt->pstONDSpec;
   volatile OneNANDReg         *pstFOReg   = (volatile OneNANDReg *) pstONDCxt->nBaseAddr;
            UINT32              nBytesRet;
            UINT32              nLockState;
            UINT32              nDie;
            UINT32              nPbn;
            INT32               nLLDRe     = FSR_LLD_SUCCESS;
            UINT16              nLockValue0;
            UINT16              nLockValue1;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[OND:IN ] ++%s(nDev:%d, nLockValue:0x%04x)\r\n"),
        __FSR_FUNC__, nDev, nLockValue));

    do
    {
        nLLDRe = FSR_OND_IOCtl(nDev, FSR_LLD_IOCTL_OTP_GET_INFO,
                                NULL, 0,
                                (UINT8 *) &nLockState, sizeof(nLockState),
                                &nBytesRet);
        if (nLLDRe != FSR_LLD_SUCCESS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   FSR_OND_IOCtl(nCode:FSR_LLD_IOCTL_OTP_GET_INFO) / %s(nDev:%d) / %d line\r\n"),
                __FSR_FUNC__, nDev, __LINE__));
            break;
        }

        if ((nLockValue & FSR_OND_OTP_LOCK_MASK) == FSR_OND_LOCK_1ST_BLOCK_OTP)
        {
            /* 
             * It's an Error to lock 1st block as OTP
             * when this device doesn't use 1st block as OTP
             */
            if ((pstONDSpec->b1stBlkOTP) == FALSE32)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[OND:ERR]   FSR_OND_IOCtl(nCode:FSR_LLD_IOCTL_OTP_LOCK) / %s(nDev:%d) / %d line\r\n"),
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
                nLLDRe = FSR_LLD_OTP_ALREADY_LOCKED;
                break;
            }
        }
        
        /* Die 0's OTP is only valid */
        nDie = 0;
        
        nLLDRe = FSR_OND_IOCtl(nDev, FSR_LLD_IOCTL_OTP_ACCESS,
                                (UINT8 *) &nDie, sizeof(nDie),
                                NULL, 0,
                                &nBytesRet);
        if (nLLDRe != FSR_LLD_SUCCESS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   FSR_OND_IOCtl(nCode:FSR_LLD_IOCTL_OTP_ACCESS) / %s(nDev:%d) / %d line\r\n"),
                __FSR_FUNC__, nDev, __LINE__));
            break;
        }

        nPbn = pstONDCxt->nUnlockedPbn;

        FSR_OND_ReadOptimal(nDev,
                            nPbn,
                            FSR_OND_OTP_PAGE_OFFSET,
                            NULL,
                            NULL,
                            FSR_LLD_FLAG_1X_LOAD);

        nLLDRe = FSR_OND_ReadOptimal(nDev,
                                     nPbn,
                                     FSR_OND_OTP_PAGE_OFFSET,
                                     NULL,
                                     NULL,
                                     FSR_LLD_FLAG_TRANSFER);
        if (nLLDRe != FSR_LLD_SUCCESS)
        {
            break;
        }

        /* 
         * Modify both of spare area 
         * because we don't known whether FSR_OND_Write() uses nDataSB00 or nDataSB10 
         */
        nLockValue0 = OND_READ(*(UINT16 *) &pstFOReg->nDataSB00[FSR_OND_LOCK_SPARE_BYTE_OFFSET]);
        nLockValue0 = nLockValue0 & (UINT16) (0xFF00 | nLockValue);
        OND_WRITE(*(UINT16 *) &pstFOReg->nDataSB00[FSR_OND_LOCK_SPARE_BYTE_OFFSET], nLockValue0);

        nLockValue1 = OND_READ(*(UINT16 *) &pstFOReg->nDataSB10[FSR_OND_LOCK_SPARE_BYTE_OFFSET]);
        nLockValue1 = nLockValue1 & (UINT16) (0xFF00 | nLockValue);
        OND_WRITE(*(UINT16 *) &pstFOReg->nDataSB10[FSR_OND_LOCK_SPARE_BYTE_OFFSET], nLockValue1);

        FSR_OAM_MEMSET(pstONDCxt->pTempBuffer,
                       0xFF,
                       pstONDSpec->nSctsPerPG * FSR_SECTOR_SIZE);

        /* Program lock field */
        FSR_OND_Write(nDev,
                      nPbn,
                      FSR_OND_OTP_PAGE_OFFSET,
                      (UINT8 *) pstONDCxt->pTempBuffer,
                      NULL,
                      FSR_LLD_FLAG_OTP_PROGRAM | FSR_LLD_FLAG_BACKUP_DATA);

        nLLDRe = FSR_OND_FlushOp(nDev, 0, FSR_LLD_FLAG_NONE);

        /* Reset the bit to '0xFF' */
        nLockValue0 = OND_READ(*(UINT16 *) &pstFOReg->nDataSB00[FSR_OND_LOCK_SPARE_BYTE_OFFSET]);
        nLockValue0 |= 0xFF;
        OND_WRITE(*(UINT16 *) &pstFOReg->nDataSB00[FSR_OND_LOCK_SPARE_BYTE_OFFSET], nLockValue0);

        nLockValue1 = OND_READ(*(UINT16 *) &pstFOReg->nDataSB10[FSR_OND_LOCK_SPARE_BYTE_OFFSET]);
        nLockValue1 |= 0xFF;
        OND_WRITE(*(UINT16 *) &pstFOReg->nDataSB10[FSR_OND_LOCK_SPARE_BYTE_OFFSET], nLockValue1);


        if (nLLDRe != FSR_LLD_SUCCESS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s(nDev:%d, nLockValue:0x%04x) / %d line\r\n"),
                __FSR_FUNC__, nDev, nLockValue, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Program Error while locking OTP\r\n")));

            _DumpRegisters(pstFOReg);

            break;
        }

    } while (0);

    /* Exit the OTP block */
    FSR_OND_IOCtl(nDev,                         /*  nDev      : Physical Device Number (0 ~ 7)   */
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
 * @brief          This function does OTP operation, reset, write protection.
 * @n              OTP read, write is performed on FSR_OND_Write(), FSR_OND_Read() function
 *
 * @param[in]      nDev         : Physical Device Number (0 ~ 7)
 * @param[in]      nCode        : IO Control Command
 * @n                             FSR_LLD_IOCTL_OTP_LOCK
 * @n                             in case IOCTL does with OTP protection, pBufI indicates 1st OTP or OTP block or both
 * @n                             FSR_LLD_IOCTL_OTP_GET_INFO
 * @n                             FSR_LLD_IOCTL_LOCK_TIGHT
 * @n                             FSR_LLD_IOCTL_LOCK_BLOCK
 * @n                             FSR_LLD_IOCTL_GET_LOCK_STAT
 * @n                             FSR_LLD_IOCTL_UNLOCK_BLOCK
 * @n                             FSR_LLD_IOCTL_UNLOCK_ALLBLK
 * @n                             FSR_LLD_IOCTL_HOT_RESET
 * @n                             FSR_LLD_IOCTL_CORE_RESET
 * @param[in]      pBufI        : Input Buffer pointer
 * @param[in]      nLenI        : Length of Input Buffer
 * @param[out]     pBufO        : Output Buffer pointer
 * @param[out]     nLenO        : Length of Output Buffer
 * @param[out]     pByteRet     : The number of bytes (length) of Output Buffer as the result of function call
 *
 * @return         FSR_LLD_SUCCESS
 * @return         FSR_LLD_INVALID_PARAM
 * @return         FSR_LLD_IOCTL_NOT_SUPPORT
 * @return         FSR_LLD_WRITE_ERROR | FSR_LLD_1STPLN_CURR_ERROR
 * @return         FSR_LLD_ERASE_ERROR | FSR_LLD_1STPLN_CURR_ERROR
 * @return         FSR_LLD_WR_PROTECT_ERROR | FSR_LLD_1STPLN_CURR_ERROR
 * @return         FSR_LLD_UNLOCKED_BLK_NOT_EXIST
 * @return         FSR_LLD_PI_PROGRAM_ERROR
 *
 * @author         NamOh Hwang / JeongWook Moon
 * @version        1.0.0
 * @remark
 *
 */
PUBLIC INT32
FSR_OND_IOCtl(UINT32  nDev,
              UINT32  nCode,
              UINT8  *pBufI,
              UINT32  nLenI,
              UINT8  *pBufO,
              UINT32  nLenO,
              UINT32 *pByteRet)
{
             LLDProtectionArg *pLLDProtectionArg; /* Used to lock, unlock, lock-tight */
             OneNANDCxt       *pstONDCxt;
             OneNANDShMem     *pstONDShMem;
             OneNANDSpec      *pstONDSpec;
    volatile OneNANDReg       *pstFOReg;
             UINT32            nLockValue;
             UINT32            nLockType;
             UINT32            nDie         = 0;
             UINT32            nDieIdx      = 0;
             UINT32            nRegVal;
             UINT32            nErrPbn      = 0;
             INT32             nLLDRe       = FSR_LLD_SUCCESS;
             UINT16            nPbn         = 0;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG, 
                   (TEXT("[OND:IN ] ++%s(nDev:%d, nCode:0x%x)\r\n"), __FSR_FUNC__, nDev, nCode));

    do
    {

#if defined (FSR_LLD_STRICT_CHK)
        /* Check device number */
        if (nDev >= FSR_OND_MAX_DEVS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR , 
                           (TEXT("[OND:ERR]   Invalid Device Number (nDev = %d)\r\n"), nDev));
            nLLDRe = (FSR_LLD_INVALID_PARAM);
            break;
        }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */

        pstONDCxt    = gpstONDCxt[nDev];
        pstONDShMem  = gpstONDShMem[nDev];
        pstONDSpec   = pstONDCxt->pstONDSpec;
        pstFOReg     = (volatile OneNANDReg *) pstONDCxt->nBaseAddr;
        
        /* Interrupt should be enabled in I/O Ctl */
        FSR_OAM_ClrNDisableInt(pstONDCxt->nIntID);

        switch (nCode)
        {
        case FSR_LLD_IOCTL_OTP_ACCESS:
            nDieIdx                    = *(UINT32 *) pBufI;
            pstONDShMem->nPreOp[nDieIdx] = FSR_OND_PREOP_IOCTL;

            if(_SearchUnlockedPbn(nDev, nDieIdx, &pstONDCxt->nUnlockedPbn) == FALSE32)
            {
                pstONDCxt->nUnlockedPbn = FSR_OND_NULL_VBN;
            }

            /* 
             * Current version has only one OTP area not only MDP but also DDP.
             * 0th die has OTP area. so, if the version is updated, argument should be
             * changed to follow the argument - pBufI : means die number.
             */
            OND_WRITE(pstFOReg->nStartAddr2 , (UINT16) nDieIdx << FSR_OND_DBS_BASEBIT);
            OND_WRITE(pstFOReg->nStartAddr1 , (UINT16) nDieIdx << FSR_OND_DFS_BASEBIT);
            
            OND_WRITE(pstFOReg->nInt, FSR_OND_INT_CLEAR);

            /* Issue command : OTP Access */
            OND_WRITE(pstFOReg->nCmd, FSR_OND_CMD_OTPACCESS);

            WAIT_OND_INT_STAT(pstFOReg, FSR_OND_INT_READY);

            break;

        /* OTP lock of 1st OTP or OTP block or both can be determined by dereferencing pBufI */
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

            /* Lock 1st block OTP & OTP block */
            if (nLockType == (FSR_LLD_OTP_LOCK_1ST_BLK | FSR_LLD_OTP_LOCK_OTP_BLK))
            {
                nLockValue = FSR_OND_LOCK_BOTH_OTP;
            }
            /* Lock 1st block OTP only */
            else if (nLockType == FSR_LLD_OTP_LOCK_1ST_BLK)
            {
                nLockValue = FSR_OND_LOCK_1ST_BLOCK_OTP;
            }
            /* Lock OTP block only */
            else if (nLockType == FSR_LLD_OTP_LOCK_OTP_BLK)
            {
                nLockValue = FSR_OND_LOCK_OTP_BLOCK;
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
            pstONDShMem->nPreOp[nDie] = FSR_OND_PREOP_IOCTL;

            if (pByteRet != NULL)
            {
                *pByteRet = (UINT32) 0;
            }
            break;

        case FSR_LLD_IOCTL_OTP_GET_INFO:
            /* Set DBS                         */
            nPbn = 0;
            OND_WRITE(pstFOReg->nStartAddr2, 
                      ((UINT16) nPbn << pstONDCxt->nDDPSelSft) & FSR_OND_MASK_DBS);
            
            nRegVal = (UINT32) ((OND_READ(pstFOReg->nCtrlStat) & FSR_OND_MASK_OTP) >> FSR_OND_OTP_SHIFT);

            /*
             * Controller Status Register 
             *     | 15 | 14 | 13 | 12 | 11 | 10 |  9 |  8 |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
             *     |====|====|====|====|====|====|====|====|====|====|====|====|====|====|====|====|
             *     |OnGo|Lock|Load|Prog|Erse| Err| Sus| Rsv|RSTB|OTP |OTP |Prv1|Cur1|Prv2|Cur2| TO |
             *                                                   (l)  (BL)      
             */
            if (nRegVal == 0x01)
            {
                *(UINT32 *) pBufO = FSR_LLD_OTP_OTP_BLK_UNLKED | 
                                    FSR_LLD_OTP_1ST_BLK_LOCKED;
            }
            else if (nRegVal == 0x02)
            {
                *(UINT32 *) pBufO = FSR_LLD_OTP_OTP_BLK_LOCKED | 
                                    FSR_LLD_OTP_1ST_BLK_UNLKED;
            }
            else if (nRegVal == 0x03)
            {
                *(UINT32 *) pBufO = FSR_LLD_OTP_OTP_BLK_LOCKED | 
                                    FSR_LLD_OTP_1ST_BLK_LOCKED;
            }
            else
            {
                *(UINT32 *) pBufO = FSR_LLD_OTP_OTP_BLK_UNLKED | 
                                    FSR_LLD_OTP_1ST_BLK_UNLKED;
            }
            
            nLLDRe    = FSR_LLD_SUCCESS;
            break;

        case FSR_LLD_IOCTL_LOCK_TIGHT:
            if ((pBufI == NULL) || (nLenI != sizeof(LLDProtectionArg)) ||
                (pBufO == NULL) || (nLenO != sizeof(nErrPbn)))
            {
                nLLDRe = FSR_LLD_INVALID_PARAM;
                break;
            }

            pLLDProtectionArg   = (LLDProtectionArg *) pBufI;
            nLLDRe              = _LockTightBlocks(nDev, 
                                                   pLLDProtectionArg->nStartBlk, 
                                                   pLLDProtectionArg->nBlks,
                                                   &nErrPbn);
         
            if (nLLDRe == FSR_LLD_BLK_PROTECTION_ERROR)
            {
                *(UINT32 *) pBufO = nErrPbn;

                if (pByteRet != NULL)
                {
                    *pByteRet = sizeof(nErrPbn);
                }
            }
            else
            {
                if (pByteRet != NULL)
                {
                    *pByteRet           = (UINT32)0;
                }
            }
            break;

        case FSR_LLD_IOCTL_LOCK_BLOCK:
            if ((pBufI == NULL) || (nLenI != sizeof(LLDProtectionArg)) ||
                (pBufO == NULL) || (nLenO != sizeof(nErrPbn)))
            {
                nLLDRe = FSR_LLD_INVALID_PARAM;
                break;
            }

            pLLDProtectionArg   = (LLDProtectionArg *) pBufI;

            nLLDRe              = _LockBlocks(nDev, 
                                              pLLDProtectionArg->nStartBlk, 
                                              pLLDProtectionArg->nBlks,
                                              &nErrPbn);
         
            if (nLLDRe == FSR_LLD_BLK_PROTECTION_ERROR)
            {
                *(UINT32 *) pBufO = nErrPbn;

                if (pByteRet != NULL)
                {
                    *pByteRet = sizeof(nErrPbn);
                }
            }
            else
            {
                if (pByteRet != NULL)
                {
                    *pByteRet = (UINT32)0;
                }
            }
            break;

        case FSR_LLD_IOCTL_UNLOCK_BLOCK:
            if ((pBufI == NULL) || (nLenI != sizeof(LLDProtectionArg)) ||
                (pBufO == NULL) || (nLenO != sizeof(nErrPbn)))
            {
                nLLDRe = FSR_LLD_INVALID_PARAM;
                break;
            }

            pLLDProtectionArg   = (LLDProtectionArg *) pBufI;

            nLLDRe              = _UnLockBlocks(nDev, 
                                                pLLDProtectionArg->nStartBlk, 
                                                pLLDProtectionArg->nBlks,
                                                &nErrPbn);

            if (nLLDRe == FSR_LLD_BLK_PROTECTION_ERROR)
            {
                *(UINT32 *) pBufO = nErrPbn;

                if (pByteRet != NULL)
                {
                    *pByteRet = sizeof(nErrPbn);
                }
            }
            else
            {
                if (pByteRet != NULL)
                {
                    *pByteRet           = (UINT32)0;
                }
            }
            break;

        case FSR_LLD_IOCTL_UNLOCK_ALLBLK:
            if (nLenI != sizeof(UINT32))
            {
                nLLDRe = FSR_LLD_INVALID_PARAM;
                break;
            }

            /* If 1st block is used to OTP block, and is locked, then error should be returned */
            if (pstONDSpec->b1stBlkOTP == TRUE32)
            {
                /* OTP_BL value can be found by Control Status Register */
                nRegVal = (UINT32) ((OND_READ(pstFOReg->nCtrlStat) & FSR_OND_MASK_OTP) >> FSR_OND_OTP_SHIFT);

                if ((nRegVal & FSR_OND_MASK_OTPBL) == FSR_OND_MASK_OTPBL)
                {
                    nLLDRe = FSR_LLD_WR_PROTECT_ERROR;
                    break;
                }
            }

            nPbn = (UINT16) *(UINT32 *) pBufI;

            nDie                                = (nPbn >> (FSR_OND_DFS_BASEBIT - pstONDCxt->nDDPSelSft)) & 0x1;
            pstONDShMem->nPreOp[nDie]           = FSR_OND_PREOP_IOCTL;
            pstONDShMem->nPreOpPbn[nDie]        = nPbn;
            pstONDShMem->nPreOpPgOffset[nDie]   = 0;

            OND_WRITE(pstFOReg->nStartAddr2 , 
                     (nPbn << pstONDCxt->nDDPSelSft) & FSR_OND_MASK_DBS);
            OND_WRITE(pstFOReg->nStartAddr1 , 
                     (nPbn << pstONDCxt->nDDPSelSft) & FSR_OND_MASK_DFS);
            OND_WRITE(pstFOReg->nStartBlkAddr, nPbn & pstONDCxt->nFBAMask);
            
            /* 
             * Write 0 into INT bit of Interrupt Register
             * this is added, because there is a ceratain amount of delay before INT bit goes to low (busy state)
             * so manully write INT bit into low (busy state)
             */
            OND_WRITE(pstFOReg->nInt, (UINT16) FSR_OND_INT_CLEAR);
            
            OND_WRITE(pstFOReg->nCmd, FSR_OND_CMD_ALLBLK_UNLOCK);

            WAIT_OND_INT_STAT(pstFOReg, FSR_OND_INT_READY);

            if (OND_READ(pstFOReg->nCtrlStat) & FSR_OND_STATUS_ERROR)
            {
                nLLDRe = FSR_LLD_BLK_PROTECTION_ERROR;

                if (pByteRet != NULL)
                {
                    *pByteRet = (UINT32) 0;
                }
            }

            break;

        case FSR_LLD_IOCTL_GET_LOCK_STAT:
            if ((nLenI != sizeof(UINT32)) || (pBufO == NULL) || (nLenO < sizeof(UINT32)))
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR , 
                               (TEXT("[OND:ERR]   (IO_CTRL)invalid parameter pBufO = 0x#010x, nLenO = %d\r\n"), 
                               pBufO, nLenO));
                nLLDRe = FSR_LLD_INVALID_PARAM;
                break;
            }

            nPbn    = (UINT16) *(UINT32 *) pBufI;

            nDieIdx                      = (nPbn >> (FSR_OND_DFS_BASEBIT - pstONDCxt->nDDPSelSft)) & 0x1;
            pstONDShMem->nPreOp[nDieIdx] = FSR_OND_PREOP_IOCTL;

            /* Set DBS       */
            OND_WRITE(pstFOReg->nStartAddr2, 
                     ((nPbn << pstONDCxt->nDDPSelSft) & FSR_OND_MASK_DBS)) ;

            /* Set DFS & FBA */
            OND_WRITE(pstFOReg->nStartAddr1, 
                     (((nPbn << pstONDCxt->nDDPSelSft) & FSR_OND_MASK_DFS) | 
                       (nPbn & pstONDCxt->nFBAMask)));

            nRegVal = (UINT32) OND_READ(pstFOReg->nWrProtectStat);

            *(UINT32 *) pBufO = nRegVal;

            *pByteRet = (UINT32) sizeof(UINT32);
            nLLDRe    = FSR_LLD_SUCCESS;
            break;

        case FSR_LLD_IOCTL_HOT_RESET:
#if 0
            /* 
             * Write 0 into INT bit of Interrupt Register
             * this is added, because there is a ceratain amount of delay before INT bit goes to low (busy state)
             * so manully write INT bit into low (busy state)
             */
            OND_WRITE(pstFOReg->nInt, (UINT16) FSR_OND_INT_CLEAR);

            OND_WRITE(pstFOReg->nCmd, FSR_OND_CMD_HOT_RESET);

            WAIT_OND_INT_STAT(pstFOReg, FSR_OND_INT_READY);

            OND_WRITE(pstFOReg->nSysConf1, pstONDCxt->nSysConf1);
#endif

            if (pByteRet != NULL)
            {
                *pByteRet = (UINT32) 0;
            }
            break;

        case FSR_LLD_IOCTL_CORE_RESET:
            /* 
             * Write 0 into INT bit of Interrupt Register
             * this is added, because there is a ceratain amount of delay before INT bit goes to low (busy state)
             * so manully write INT bit into low (busy state)
             */
            OND_WRITE(pstFOReg->nInt, (UINT16) FSR_OND_INT_CLEAR);

            OND_WRITE(pstFOReg->nCmd, FSR_OND_CMD_CORE_RESET);

            WAIT_OND_INT_STAT(pstFOReg, FSR_OND_INT_READY);
            break;

        default:
            nLLDRe = FSR_LLD_IOCTL_NOT_SUPPORT;
            break;
        }

        if ((nLLDRe != FSR_LLD_INVALID_PARAM) && (nLLDRe != FSR_LLD_IOCTL_NOT_SUPPORT))
        {
            FSR_ASSERT(nDie != 0xFFFFFFFF);

            pstONDShMem->nPreOp[nDie]         = FSR_OND_PREOP_IOCTL;
            pstONDShMem->nPreOpPbn[nDie]      = FSR_OND_PREOP_ADDRESS_NONE;
            pstONDShMem->nPreOpPgOffset[nDie] = FSR_OND_PREOP_ADDRESS_NONE;
            pstONDShMem->nPreOpFlag[nDie]     = FSR_OND_PREOP_FLAG_NONE;
        }

    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG, 
        (TEXT("[OND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return (nLLDRe);
}

/**
 * @brief          This function checks the validity of parameter
 *
 * @param[in]      nDev         : Physical Device Number (0 ~ 7)
 * @param[in]      nPbn         : Physical Block  Number
 * @param[in]      nPgOffset    : Page Offset within a block
 * @param[in]      pstONDCxt    : gpstONDCxt[nDev]
 * @param[in]      pstONDSpec   : gpstONDCxt[nDev]->pstONDSpec
 *
 * @return         FSR_LLD_SUCCESS
 * @return         FSR_LLD_INVALID_PARAM
 *
 * @author         NamOh Hwang / JeongWook Moon
 * @version        1.0.0
 * @remark
 *
 */
PRIVATE INT32
_StrictChk(UINT32       nDev,
           UINT32       nPbn,
           UINT32       nPgOffset)
{
    INT32           nLLDRe     = FSR_LLD_SUCCESS;
    OneNANDCxt     *pstONDCxt  = NULL;
    OneNANDSpec    *pstONDSpec = NULL;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, 
                   (TEXT("[OND:IN ] ++%s(nDev:%d, nPbn:%d, nPgOffset:%d)\r\n"), 
                   __FSR_FUNC__, nDev, nPbn, nPgOffset));

    do
    {
        /* Check Device Number */
        if (nDev >= FSR_OND_MAX_DEVS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR , 
                           (TEXT("[OND:ERR]   Invalid Device Number (nDev = %d)\r\n"), nDev));
            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }

        pstONDCxt  = gpstONDCxt[nDev];        

        /* Check Device Open Flag */
        if (pstONDCxt->bOpen == FALSE32)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s() / %d line\r\n"),
                __FSR_FUNC__, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR , 
                           (TEXT("[OND:ERR]   Device is not opened\r\n")));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            pstONDCxt->bOpen: 0x%08x\r\n"),
                pstONDCxt->bOpen));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }

        pstONDSpec = pstONDCxt->pstONDSpec;

        /* Check Block Out of Bound */
        if (nPbn >= (UINT32) pstONDSpec->nNumOfBlks)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[OND:ERR]   %s() / %d line\r\n"),
                __FSR_FUNC__, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Pbn:%d >= pstONDSpec->nNumOfBlks:%d\r\n"),
                nPbn, pstONDSpec->nNumOfBlks));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }
        else
        {
            /*
             * Check nPgOffset Out of Bound                 
             * In case of SLC, pageOffset is lower than 64  
             */
            if (nPgOffset >= pstONDSpec->nPgsPerBlkForSLC)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[OND:ERR]   %s() / %d line\r\n"),
                    __FSR_FUNC__, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            Pg #%d in SLC(Pbn #%d) is invalid\r\n"),
                    nPgOffset, nPbn));
                
                nLLDRe = FSR_LLD_INVALID_PARAM;
                break;
            }
        }

    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, 
                   (TEXT("[OND:OUT] --%s\r\n"), __FSR_FUNC__));

    return (nLLDRe);
}


#if defined (FSR_LLD_STATISTICS)

/**
 * @brief          This function is called within the LLD function
 *                 to total the busy time of the device
 *
 * @param[in]      nDev         : Physical Device Number (0 ~ 7)
 * @param[in]      nDieNum      : Physical Die  Number
 * @param[in]      nType        : FSR_OND_STAT_SLC_PGM
 *                                FSR_OND_STAT_LSB_PGM
 *                                FSR_OND_STAT_MSB_PGM
 *                                FSR_OND_STAT_ERASE
 *                                FSR_OND_STAT_LOAD
 *                                FSR_OND_STAT_RD_TRANS
 *                                FSR_OND_STAT_WR_TRANS
 * @param[in]      nBytes       : the number of bytes to transfer from/to DataRAM
 * @param[in]      nCmdOption   : command option such cache, superload
 *                                which can hide transfer time
 *
 *
 * @return         VOID
 *
 * @since          since v1.0.0
 *
 */
PRIVATE VOID
_AddONDStat(UINT32  nDevNum,
            UINT32  nDieNum,
            UINT32  nType,
            UINT32  nBytes,
            UINT32  nCmdOption)
{
    OneNANDCxt  *pstONDCxt  = gpstONDCxt[nDevNum];
    OneNANDSpec *pstONDSpec = pstONDCxt->pstONDSpec;
    OneNANDStat *pstONDStat = gpstONDStat[nDevNum];
    
    UINT32       nVol;
    UINT32       nDevIdx; /* Device index within a volume (0~4) */
    UINT32       nPDevIdx;/* Physical device index (0~7)        */
    UINT32       nDieIdx;
    UINT32       nNumOfDies;
    
    /* The duration of Interrupt Low when command is issued */
    INT32        nIntLowTime;
    INT32        nTransferTime;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[OND:IN ] ++%s\r\n"), __FSR_FUNC__));

    pstONDStat  = gpstONDStat[nDevNum];
    nIntLowTime = pstONDStat->nIntLowTime[nDieNum];

    switch (nType)
    {
    case FSR_OND_STAT_SLC_PGM:
        /* Add the number of times of SLC program */
        pstONDStat->nNumOfSLCPgms++;
        
        if (nIntLowTime > 0)
        {
            /* Wait INT for previous operation */
            gnElapsedTime += nIntLowTime;
            
            for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
            {
                for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVol]; nDevIdx++)
                {
                    nPDevIdx   = nVol * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx;
                    nNumOfDies = gpstONDCxt[nPDevIdx]->pstONDSpec->nNumOfDies;

                    for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                    {
                        if (gpstONDStat[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                        {
                            gpstONDStat[nPDevIdx]->nIntLowTime[nDieIdx] -= nIntLowTime;
                        }
                    }
                }
            }
        }

        /* 
         * If nCmdOption is CACHE, transfering time can be hided
         * store & use it later 
         */
        pstONDStat->nPreCmdOption[nDieNum] = nCmdOption;
        pstONDStat->nIntLowTime[nDieNum]   = FSR_OND_WR_SW_OH + pstONDSpec->nSLCTProgTime;

        if (nCmdOption == FSR_OND_STAT_CACHE_PGM)
        {
            pstONDStat->nNumOfCacheBusy++;
        }
        break;

    case FSR_OND_STAT_ERASE:
        pstONDStat->nNumOfErases++;
        
        if (nIntLowTime > 0)
        {
            /* Wait INT for previous operation */
            gnElapsedTime += nIntLowTime;
            
            for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
            {
                for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVol]; nDevIdx++)
                {
                    nPDevIdx   = nVol * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx;
                    nNumOfDies = gpstONDCxt[nPDevIdx]->pstONDSpec->nNumOfDies;

                    for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                    {
                        if (gpstONDStat[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                        {
                            gpstONDStat[nPDevIdx]->nIntLowTime[nDieIdx] -= nIntLowTime;
                        }
                    }
                }
            }
        }

        pstONDStat->nIntLowTime[nDieNum] = pstONDSpec->nTEraseTime;
        break;

    case FSR_OND_STAT_SLC_LOAD:
        pstONDStat->nNumOfSLCLoads++;

        if(nIntLowTime > 0)
        {
            /* Wait INT for previous operation */
            gnElapsedTime += nIntLowTime;
            
            for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
            {
                for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVol]; nDevIdx++)
                {
                    nPDevIdx   = nVol * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx;
                    nNumOfDies = gpstONDCxt[nPDevIdx]->pstONDSpec->nNumOfDies;

                    for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                    {
                        if (gpstONDStat[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                        {
                            gpstONDStat[nPDevIdx]->nIntLowTime[nDieIdx] -= nIntLowTime;
                        }
                    }
                }
            }
        }
        pstONDStat->nIntLowTime[nDieNum] = FSR_OND_RD_SW_OH + pstONDSpec->nSLCTLoadTime;
        
        pstONDStat->nPreCmdOption[nDieNum] = nCmdOption;
        break;

    case FSR_OND_STAT_RD_TRANS:
        pstONDStat->nNumOfRdTrans++;
        pstONDStat->nRdTransInBytes  += nBytes;
        nTransferTime = nBytes * pstONDCxt->nRdTranferTime / 2 / 1000;

        if (nBytes > 0)
        {
            /* Add s/w overhead */
            nTransferTime += FSR_OND_RD_TRANS_SW_OH;
        }

        if ((nCmdOption != FSR_OND_STAT_PLOAD) && (nIntLowTime > 0))
        {
            gnElapsedTime += nIntLowTime;

            for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
            {
                for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVol]; nDevIdx++)
                {
                    nPDevIdx   = nVol * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx;
                    nNumOfDies = gpstONDCxt[nPDevIdx]->pstONDSpec->nNumOfDies;

                    for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                    {
                        if (gpstONDStat[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                        {
                            gpstONDStat[nPDevIdx]->nIntLowTime[nDieIdx] -= /* sw overhead + */nIntLowTime;
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
                nNumOfDies = gpstONDCxt[nPDevIdx]->pstONDSpec->nNumOfDies;

                for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                {
                    if (gpstONDStat[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                    {
                        gpstONDStat[nPDevIdx]->nIntLowTime[nDieIdx] -= nTransferTime;
                    }
                }
            }
        }

        break;

    case FSR_OND_STAT_WR_TRANS:
        pstONDStat->nNumOfWrTrans++;
        pstONDStat->nWrTransInBytes  += nBytes;
        nTransferTime = nBytes * pstONDCxt->nWrTranferTime / 2 / 1000;

        /* Cache operation can hide transfer time */
        if((pstONDStat->nPreCmdOption[nDieNum] == FSR_OND_STAT_CACHE_PGM) && (nIntLowTime >= 0))
        {
            gnElapsedTime  += nTransferTime;

            for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
            {
                for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVol]; nDevIdx++)
                {
                    nPDevIdx   = nVol * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx;
                    nNumOfDies = gpstONDCxt[nPDevIdx]->pstONDSpec->nNumOfDies;

                    for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                    {
                        if (gpstONDStat[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                        {
                            gpstONDStat[nPDevIdx]->nIntLowTime[nDieIdx] -= nTransferTime;
                        }
                    }
                }
            }

            if(nIntLowTime < nTransferTime)
            {
                /* Only some of transfer time can be hided */
                pstONDStat->nIntLowTime[nDieNum] = 0;
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
                        nNumOfDies = gpstONDCxt[nPDevIdx]->pstONDSpec->nNumOfDies;

                        for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                        {
                            if (gpstONDStat[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                            {
                                gpstONDStat[nPDevIdx]->nIntLowTime[nDieIdx] -= nIntLowTime;
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
                    nNumOfDies = gpstONDCxt[nPDevIdx]->pstONDSpec->nNumOfDies;

                    for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                    {
                        if (gpstONDStat[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                        {
                            gpstONDStat[nPDevIdx]->nIntLowTime[nDieIdx] -= nTransferTime;
                        }
                    }
                }
            }

            pstONDStat->nIntLowTime[nDieNum] = 0;
        }
        break;

    case FSR_OND_STAT_FLUSH:
        if (nIntLowTime > 0)
        {
            /* Wait INT for previous operation */
            gnElapsedTime += nIntLowTime;
            
            for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
            {
                for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVol]; nDevIdx++)
                {
                    nPDevIdx   = nVol * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx;
                    nNumOfDies = gpstONDCxt[nPDevIdx]->pstONDSpec->nNumOfDies;

                    for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                    {
                        if (gpstONDStat[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                        {
                            gpstONDStat[nPDevIdx]->nIntLowTime[nDieIdx] -= nIntLowTime;
                        }
                    }
                }
            }

            pstONDStat->nIntLowTime[nDieNum] = 0;
        }
        break;

    default:
        FSR_ASSERT(0);
        break;
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[OND:OUT] --%s\r\n"), __FSR_FUNC__));
}
#endif /* #if defined (FSR_LLD_STATISTICS) */



/**
 * @brief          This function initialize the structure for LLD statistics
 *
 * @param[in]      VOID
 *
 * @return         FSR_LLD_SUCCESS
 *
 * @author         NamOh Hwang / JeongWook Moon
 * @version        1.0.0
 * @remark
 *
 */
PUBLIC INT32
FSR_OND_InitLLDStat(VOID)
{
#if defined (FSR_LLD_STATISTICS)
    OneNANDStat    *pstStat;
    UINT32          nVolIdx;
    UINT32          nDevIdx;
    UINT32          nDieIdx;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:IN ] ++%s\r\n"), __FSR_FUNC__));

    gnElapsedTime = 0;

     for (nVolIdx = 0; nVolIdx < FSR_MAX_VOLS; nVolIdx++)
    {
        for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVolIdx]; nDevIdx++)
        {
            pstStat = gpstONDStat[nVolIdx * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx];

            pstStat->nNumOfSLCLoads    = 0;
            pstStat->nNumOfSLCPgms     = 0;
            pstStat->nNumOfCacheBusy   = 0;
            pstStat->nNumOfErases      = 0;
            pstStat->nNumOfRdTrans     = 0;
            pstStat->nNumOfWrTrans     = 0;
            pstStat->nRdTransInBytes   = 0;
            pstStat->nWrTransInBytes   = 0;
            pstStat->nNumOfMLCLoads    = 0;
            pstStat->nNumOfLSBPgms     = 0;
            pstStat->nNumOfMSBPgms     = 0;

            for (nDieIdx = 0; nDieIdx < FSR_MAX_DIES; nDieIdx++)
            {
                pstStat->nPreCmdOption[nDieIdx] = FSR_OND_STAT_NORMAL_CMD;
                pstStat->nIntLowTime[nDieIdx]   = 0;
            }
        }
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG, (TEXT("[OND:OUT] --%s()\r\n"), __FSR_FUNC__));
#endif

    return FSR_LLD_SUCCESS;
}



/**
 * @brief          This function initialize the structure for LLD statistics
 *
 * @param[out]     pstStat : the pointer to the structure, FSRLLDStat
 *
 * @return         total busy time of whole device
 *
 * @author         NamOh Hwang / JeongWook Moon
 * @version        1.0.0
 * @remark
 *
 */
PUBLIC INT32
FSR_OND_GetStat(FSRLLDStat *pstStat)
{
#if defined (FSR_LLD_STATISTICS)
    OneNANDCxt  *pstONDCxt;
    OneNANDSpec *pstONDSpec;
    OneNANDStat *pstONDStat;
    UINT32       nVolIdx;
    UINT32       nDevIdx;
    UINT32       nTotalTime;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:IN ] ++%s()\r\n"), __FSR_FUNC__));

    do
    {
        if (pstStat == NULL)
        {
            break;
        }

        FSR_OAM_MEMSET(pstStat, 0x00, sizeof(FSRLLDStat));

        nTotalTime    = 0;

        for (nVolIdx = 0; nVolIdx < FSR_MAX_VOLS; nVolIdx++)
        {
            for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVolIdx]; nDevIdx++)
            {
                pstONDStat = gpstONDStat[nVolIdx * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx];

                pstStat->nSLCPgms        += pstONDStat->nNumOfSLCPgms;
                pstStat->nErases         += pstONDStat->nNumOfErases;
                pstStat->nSLCLoads       += pstONDStat->nNumOfSLCLoads;
                pstStat->nRdTrans        += pstONDStat->nNumOfRdTrans;
                pstStat->nWrTrans        += pstONDStat->nNumOfWrTrans;
                pstStat->nCacheBusy      += pstONDStat->nNumOfCacheBusy;
                pstStat->nRdTransInBytes += pstONDStat->nRdTransInBytes;
                pstStat->nWrTransInBytes += pstONDStat->nWrTransInBytes;

                pstONDCxt     = gpstONDCxt[nDevIdx];
                pstONDSpec    = pstONDCxt->pstONDSpec;

                nTotalTime   += pstStat->nSLCLoads          * pstONDSpec->nSLCTLoadTime;
                nTotalTime   += pstStat->nSLCPgms           * pstONDSpec->nSLCTProgTime;
                nTotalTime   += pstStat->nErases            * pstONDSpec->nTEraseTime;

                /* pstONDCxt->nRdTranferTime, pstONDCxt->nWrTranferTime is time for transfering 2 bytes */
                nTotalTime   += pstStat->nRdTransInBytes    * pstONDCxt->nRdTranferTime / 2 / 1000;
                nTotalTime   += pstStat->nWrTransInBytes    * pstONDCxt->nWrTranferTime / 2 / 1000;
                nTotalTime   += pstStat->nCacheBusy         * FSR_OND_CACHE_BUSY_TIME;
            }
        }
    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[OND:OUT] --%s() / gnElapsedTime : %d\r\n"), 
        __FSR_FUNC__, gnElapsedTime));

    return gnElapsedTime;
#else
    FSR_OAM_MEMSET(pstStat, 0x00, sizeof(FSRLLDStat));
    return 0;
#endif /* #if defined (FSR_LLD_STATISTICS) */
}



/**
 * @brief          This function gets block tybpe and pages per blk.
 *
 * @param[in]      nDev        : Physical Device Number (0 ~ 7)
 * @param[in]      nPbn        : block number
 * @param[out]     pnType      : block type (MLC / SLC)
 * @param[out]     pnPgsPerBlk : pages per block
 *
 * @return         FSR_LLD_SUCCESS
 *
 * @author         NamOh Hwang / JeongWook Moon
 * @version        1.0.0
 * @remark
 *
 */
PUBLIC INT32
FSR_OND_GetBlockInfo (UINT32  nDev,
                      UINT32  nPbn,
                      UINT32 *pnType,
                      UINT32 *pnPgsPerBlk)
{
    OneNANDCxt     *pstONDCxt;
    OneNANDSpec    *pstONDSpec;
    
    INT32          nLLDRe = FSR_LLD_SUCCESS;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG, 
                   (TEXT("[OND:IN ] ++%s(nDev:%d, nPbn:%d)\r\n"), 
                   __FSR_FUNC__, nDev, nPbn));

    do
    {
#if defined (FSR_LLD_STRICT_CHK)
        if ((nLLDRe = _StrictChk(nDev, nPbn, 0)) != FSR_LLD_SUCCESS)
        {
            break;
        }
#endif
        pstONDCxt  = gpstONDCxt[nDev];
        pstONDSpec = pstONDCxt->pstONDSpec;

        /* Check whether the second parameter is valid or not */
        if (nPbn >= (UINT32) pstONDSpec->nNumOfBlks)
        {
            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }

        /* Every block is SLC NAND block */
        if (pnType != NULL)
        {
            *pnType         = FSR_LLD_SLC_BLOCK;
        }

        if (pnPgsPerBlk != NULL) 
        {
            *pnPgsPerBlk    = pstONDSpec->nPgsPerBlkForSLC;
        }
    } while(0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG, 
        (TEXT("[OND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));

    return (nLLDRe);
}

/**
 * @brief          This function gets NAND controller information.
 *
 * @param[in]      nDev        : Physical Device Number (0 ~ 7)
 * @param[in]      pLLDPltInfo : Platform information (we need manufacturer ID)
 *
 * @return         FSR_LLD_SUCCESS
 *
 * @author         NamOh Hwang / JeongWook Moon
 * @version        1.0.0
 * @remark
 *
 */
PUBLIC INT32
FSR_OND_GetNANDCtrllerInfo(UINT32           nDev,
                           LLDPlatformInfo *pLLDPltInfo)
{
             OneNANDCxt    *pstONDCxt       = gpstONDCxt[nDev];
    volatile OneNANDReg    *pstFOReg        = (volatile OneNANDReg *) pstONDCxt->nBaseAddr;
             INT32          nLLDRe          = FSR_LLD_SUCCESS;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG, 
                   (TEXT("[OND:IN ] ++%s(nDev:%d)\r\n"), 
                   __FSR_FUNC__, nDev));

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
        if (nDev >= FSR_OND_MAX_DEVS)
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

        pstONDCxt   = gpstONDCxt[nDev];
        pstFOReg    = (volatile OneNANDReg *) pstONDCxt->nBaseAddr;

#if defined (FSR_LLD_STRICT_CHK)
        /* Check Device Open Flag */
        if (pstONDCxt->bOpen == FALSE32)
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

        /* Type of Device : OneNAND = 0         */
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
        (TEXT("[OND:OUT] --%s() / nLLDRe : 0x%x\r\n"), 
                   __FSR_FUNC__, nLLDRe));

    return (nLLDRe);
}

/**
 * @brief          This function sets same data into another DataRAM.
 *
 * @param[in]      nFlag       : It is decided that which data is copied to which DataRAM 
 *                               according to the value is a poistive or a negative number.
 *
 * @return         None
 *
 * @author         JeongWook Moon
 * @version        1.0.0
 * @remark
 *
 */
PRIVATE VOID 
_CopyToAnotherDataRAM(UINT32        nDev,
                      LLDCpBkArg   *pstCpArg)
{
             OneNANDCxt       *pstONDCxt  = gpstONDCxt[nDev];
             OneNANDSpec      *pstONDSpec = pstONDCxt->pstONDSpec;
    volatile OneNANDReg       *pstFOReg   = (volatile OneNANDReg *) pstONDCxt->nBaseAddr;

    /* Set DBS about source die        */
    OND_WRITE(pstFOReg->nStartAddr2, 
              ((UINT16) pstCpArg->nSrcPbn << pstONDCxt->nDDPSelSft) & FSR_OND_MASK_DBS);


    /* Read to pDataBuf for copy data in first die */
    _ReadMain(pstONDCxt->pDataBuf4CpBk, 
              &(pstFOReg->nDataMB00[0]), 
              pstONDSpec->nSctsPerPG * FSR_OND_SECTOR_SIZE,
              (UINT8 *) pstONDCxt->pTempBuffer);

    _ReadSpare((FSRSpareBuf *) (pstONDCxt->pDataBuf4CpBk + 
                                (pstONDSpec->nSctsPerPG * FSR_OND_SECTOR_SIZE)),
               &(pstFOReg->nDataSB00[0]), FSR_LLD_FLAG_ECC_OFF);

    /* Set DBS about destination die   */
    OND_WRITE(pstFOReg->nStartAddr2, 
              ((UINT16) pstCpArg->nDstPbn << pstONDCxt->nDDPSelSft) & FSR_OND_MASK_DBS);

    /* Write to another DataRAM */
    _WriteMain(&(pstFOReg->nDataMB00[0]), 
               pstONDCxt->pDataBuf4CpBk, 
               pstONDSpec->nSctsPerPG * FSR_OND_SECTOR_SIZE);

    _WriteSpare(&(pstFOReg->nDataSB00[0]), 
                (FSRSpareBuf *) (pstONDCxt->pDataBuf4CpBk + 
                                (pstONDSpec->nSctsPerPG * FSR_OND_SECTOR_SIZE)),
                FSR_LLD_FLAG_ECC_OFF);

}

/**
 * @brief           This function calculates transfer time for word (2 bytes)
 * @n               between host & OneNAND
 *
 * @param[in]       nDev        : Physical Device Number (0 ~ 7)
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
_CalcTransferTime(UINT32 nDev, UINT32 *pnWordRdCycle, UINT32 *pnWordWrCycle)
{
             OneNANDCxt       *pstONDCxt  = gpstONDCxt[nDev];
             OneNANDSpec      *pstONDSpec = pstONDCxt->pstONDSpec;
    volatile OneNANDReg       *pstFOReg   = (volatile OneNANDReg *) pstONDCxt->nBaseAddr;

             UINT32             nPgIdx;
             UINT32             nNumOfPgs = 1;
             UINT32             nPageSize;
             /* Theoretical read cycle for word (2 byte) from OneNAND to host */
             UINT32             nTheoreticalRdCycle;

             UINT32             nWordRdCycle;
             UINT32             nWordWrCycle;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[OND:IN ] ++%s\r\n"), __FSR_FUNC__));


    nPageSize = pstONDSpec->nSctsPerPG * FSR_SECTOR_SIZE;

    /* Calculate write cycle for word */
    FSR_OAM_MEMSET(pstONDCxt->pTempBuffer, 
                   0x55, 
                   nPageSize);

#if !defined(FSR_OAM_RTLMSG_DISABLE)
    FSR_OAM_StartTimer();
#endif

    for (nPgIdx = 0; nPgIdx < nNumOfPgs; nPgIdx++)
    {
        TRANSFER_TO_NAND(pstFOReg->nDataMB00,
                         (UINT8 *) pstONDCxt->pTempBuffer, 
                         nPageSize);
    }

#if !defined(FSR_OAM_RTLMSG_DISABLE)
    FSR_OAM_StopTimer();
#endif

#if !defined(FSR_OAM_RTLMSG_DISABLE)
    /* nWordWrCycle is based on nano second */
    nWordWrCycle   = 2 * 1000 * FSR_OAM_GetElapsedTime() / (nNumOfPgs * nPageSize);
#else
    nWordWrCycle   = 0;
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

    /* Calculate read cycle for word */
    FSR_OAM_MEMSET(pstONDCxt->pTempBuffer, 
                   0xFF, 
                   pstONDSpec->nSctsPerPG * FSR_SECTOR_SIZE);

#if !defined(FSR_OAM_RTLMSG_DISABLE)
    FSR_OAM_StartTimer();
#endif

    for (nPgIdx = 0; nPgIdx < nNumOfPgs; nPgIdx++)
    {
        TRANSFER_FROM_NAND((UINT8 *) pstONDCxt->pTempBuffer, 
                           pstFOReg->nDataMB00, 
                           nPageSize);
    }

#if !defined(FSR_OAM_RTLMSG_DISABLE)
    FSR_OAM_StopTimer();
#endif

    /* Time for transfering 16 bits depends on bRM (read mode) */
    nTheoreticalRdCycle = (OND_READ(pstFOReg->nSysConf1) & FSR_OND_CONF1_SYNC_READ) ? 15 : 76;

#if !defined(FSR_OAM_RTLMSG_DISABLE)
    /* nByteRdCycle is based on nano second */
    nWordRdCycle   = 2 * 1000 * FSR_OAM_GetElapsedTime() / (nNumOfPgs * nPageSize);
#else
    nWordRdCycle   =  0;
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

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[OND:OUT] --%s\r\n"), __FSR_FUNC__));
}

/**
 * @brief           This function gets the unique ID from OneNAND OTP
 *
 * @param[in]       nDev        : Physical Device Number (0 ~ 7)
 * @param[in]       *pstONDCxt  : pointer to OneNANDCxt data structure
 *
 * @return          FSR_LLD_SUCCESS
 *
 * @author          JeongWook Moon
 * @version         1.0.0
 * @remark
 *
 */
PRIVATE INT32
_GetUniqueID(UINT32         nDev,
             OneNANDCxt    *pstONDCxt,
             UINT32         nFlag)
{
    OneNANDSpec    *pstONDSpec;
    UINT32          nDie = 0;
    UINT32          nPbn;
    UINT32          nPgOffset;
    UINT32          nByteRet;
    UINT32          nIdx;
    UINT32          nCnt;
    UINT32          nSizeOfUID;
    UINT32          nSparePerSct;
    INT32           nLLDRe          = FSR_LLD_SUCCESS;
    INT32           nRet            = 0;
    BOOL32          bValidUID;
    UINT8          *pUIDArea        = NULL;
    UINT8          *pSignature      = NULL;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF,
        (TEXT("[OND:IN ] ++%s(nDev:%d, nDie:%d)\r\n"),
        __FSR_FUNC__, nDev, nDie));

    do
    {
        /* Set OTP access mode */
        nLLDRe = FSR_OND_IOCtl(nDev,                     /*  nDev      : Physical Device Number (0 ~ 7)   */
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

        FSR_ASSERT(pstONDCxt != NULL);
        FSR_ASSERT(pstONDCxt->pstONDSpec != NULL);

        pstONDSpec   = pstONDCxt->pstONDSpec;
        nSparePerSct = pstONDSpec->nSparePerSct;

        FSR_OAM_MEMSET(pstONDCxt->pTempBuffer, 0x00,
            pstONDSpec->nSctsPerPG * (FSR_SECTOR_SIZE + nSparePerSct));

        /* Unique ID is located at page 60 of OTP block */
        nPgOffset   = 60;
        nPbn        = 0;

        pUIDArea    = pstONDCxt->pTempBuffer;
        pSignature  = pstONDCxt->pTempBuffer + (pstONDSpec->nSctsPerPG * FSR_SECTOR_SIZE);

        nLLDRe = FSR_OND_Read(nDev, nPbn, nPgOffset,
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
        nSizeOfUID = (FSR_SECTOR_SIZE / FSR_OND_NUM_OF_UID) / 2;

        for (nCnt = 0; nCnt < FSR_OND_NUM_OF_UID; nCnt++)
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
            pUIDArea += (FSR_SECTOR_SIZE / FSR_OND_NUM_OF_UID);
        }

        if (bValidUID == FALSE32)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF, (TEXT("[OND:ERR]   Invalid UID\r\n")));
            break;
        }

        /* Get Unique ID */
        for (nIdx = 0; nIdx < FSR_LLD_UID_SIZE; nIdx++)
        {
            pstONDCxt->nUID[nIdx] = pUIDArea[2 * nIdx];
        }

    } while (0);

    /* Exit the OTP block */
    nLLDRe = FSR_OND_IOCtl(nDev,                         /*  nDev      : Physical Device Number (0 ~ 7)   */
                           FSR_LLD_IOCTL_CORE_RESET,     /*  nCode     : IO Control Command               */
                           NULL,                         /* *pBufI     : Input Buffer pointer             */
                           0,                            /*  nLenI     : Length of Input Buffer           */
                           NULL,                         /* *pBufO     : Output Buffer pointer            */
                           0,                            /*  nLenO     : Length of Output Buffer          */
                           &nByteRet);                   /* *pByteRet  : The number of bytes (length) of 
                                                                         Output Buffer as the result of 
                                                                         function call                    */

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF,
        (TEXT("[OND:OUT] --%s()\r\n"), __FSR_FUNC__));

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
_DumpRegisters(volatile OneNANDReg *pstReg)
{
    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
    (TEXT("[OND:IN ] ++%s(pstReg:0x%08x)\r\n"),
    __FSR_FUNC__, pstReg));

    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("\r\nstart dumping registers. \r\n")));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Manufacturer ID Reg."),
        &pstReg->nMID, OND_READ(pstReg->nMID)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Device ID Reg."),
        &pstReg->nDID, OND_READ(pstReg->nDID)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Version ID Reg."),
        &pstReg->nVerID, OND_READ(pstReg->nVerID)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, 
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Data Buffer Size Reg."),
        &pstReg->nDataBufSize, OND_READ(pstReg->nDataBufSize)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Boot Buffer Size Reg."),
        &pstReg->nBootBufSize, OND_READ(pstReg->nBootBufSize)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Amount of buffers Reg."),
        &pstReg->nBufAmount, OND_READ(pstReg->nBufAmount)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Technology Reg."),
        &pstReg->nTech, OND_READ(pstReg->nTech)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Start address 1 Reg."),
        &pstReg->nStartAddr1, OND_READ(pstReg->nStartAddr1)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Start address 2 Reg."),
        &pstReg->nStartAddr2, OND_READ(pstReg->nStartAddr2)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Start address 8 Reg."),
        &pstReg->nStartAddr8, OND_READ(pstReg->nStartAddr8)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Start Buffer Reg."),
        &pstReg->nStartBuf, OND_READ(pstReg->nStartBuf)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Command Reg."),
        &pstReg->nCmd, OND_READ(pstReg->nCmd)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("System Conf1 Reg."),
        &pstReg->nSysConf1, OND_READ(pstReg->nSysConf1)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Controller Status Reg."),
        &pstReg->nCtrlStat, OND_READ(pstReg->nCtrlStat)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Interrupt Reg."),
        &pstReg->nInt, OND_READ(pstReg->nInt)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Start Block Address Reg."),
        &pstReg->nStartBlkAddr, OND_READ(pstReg->nStartBlkAddr)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("Write Protection Reg."),
        &pstReg->nWrProtectStat, OND_READ(pstReg->nWrProtectStat)));
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("ECC Status Reg."),
        &pstReg->nEccStat, OND_READ(pstReg->nEccStat)));

    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("ECC Result(1st/Main)  Reg"),
        &pstReg->nEccResult[0], OND_READ(pstReg->nEccResult[0])));

    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("ECC Result(1st/Spare) Reg"),
        &pstReg->nEccResult[1], OND_READ(pstReg->nEccResult[1])));

    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("ECC Result(2nd/Main)  Reg"),
        &pstReg->nEccResult[2], OND_READ(pstReg->nEccResult[2])));

    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("ECC Result(2nd/Spare) Reg"),
        &pstReg->nEccResult[3], OND_READ(pstReg->nEccResult[3])));

    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("ECC Result(3rd/Main)  Reg"),
        &pstReg->nEccResult[4], OND_READ(pstReg->nEccResult[4])));

    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("ECC Result(3rd/Spare) Reg"),
        &pstReg->nEccResult[5], OND_READ(pstReg->nEccResult[5])));

    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("ECC Result(4th/Main)  Reg"),
        &pstReg->nEccResult[6], OND_READ(pstReg->nEccResult[6])));

    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("%-25s @ 0x%08X: 0x%04X\r\n"), TEXT("ECC Result(4th/Spare) Reg"),
        &pstReg->nEccResult[7], OND_READ(pstReg->nEccResult[7])));


    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
        (TEXT("end dumping registers. \r\n\r\n")));


    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[OND:OUT] --%s\r\n"), __FSR_FUNC__));
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
_DumpSpareBuffer(volatile OneNANDReg *pstReg,
                 UINT32   nDev,
                 UINT32   nDie)
{
#if !defined(FSR_OAM_RTLMSG_DISABLE)
    OneNANDCxt       *pstONDCxt  = gpstONDCxt[nDev];

    UINT32      nSctIdx;
    UINT32      nIdx;
    UINT32      nDataBufSize;
    UINT32      nSctsPerDataBuf;
    UINT16      nValue;
    UINT16     *pnDataSB00;

    UINT32      nPreUseBuf;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
    (TEXT("[OND:IN ] ++%s(pstReg:0x%08x)\r\n"),
    __FSR_FUNC__, pstReg));

    nPreUseBuf = pstONDCxt->nPreUseBuf[nDie];

    nDataBufSize    = OND_READ(pstReg->nDataBufSize);
    nSctsPerDataBuf = nDataBufSize / FSR_SECTOR_SIZE;
    pnDataSB00      = (UINT16 *) pstReg->nDataSB00;

    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("          Dump Spare Buffer\r\n")));
    for (nSctIdx = 0; nSctIdx < nSctsPerDataBuf; nSctIdx++)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[%02d] "), nSctIdx));
        for (nIdx = 0; nIdx < 8; nIdx++)
        {
            nValue = (UINT16) OND_READ(pnDataSB00[nSctIdx * 8 + nIdx  + 
                                       (nPreUseBuf & 1) * 
                                       (FSR_OND_SDRAM0_SIZE / 2)]);
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("%04x "), nValue));
        }
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("\r\n")));
    }

    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("\r\n")));

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[OND:OUT] --%s\r\n"), __FSR_FUNC__));
#endif
}
