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
 * @file      FSR_BML_Types.h
 * @brief     This header defines Data types which are shared
 *            by all BML submodules
 * @author    MinYoung Kim
 * @author    SuRyun Lee
 * @date      11-JAN-2007
 * @remark
 * REVISION HISTORY
 * @n  11-JAN-2007 [SuRyun Lee]  : first writing
 * @n  11-JAN-2007 [MinYoung Kim]: add data structures for reservoir
 *
 */

#ifndef _FSR_BML_TYPES_H_
#define _FSR_BML_TYPES_H_

/****************************************************************************/
/*Common Constant definitions                                               */
/****************************************************************************/
#define     FSR_FIRST_DIE                           (0)
#define     FSR_SECOND_DIE                          (1)
#define     DEVS_PER_VOL                            (FSR_MAX_DEVS/FSR_MAX_VOLS)
#define     DEVS_PER_VOL_SHIFT                      (1)

/*****************************************************************************/
/* nOpType of PreOpLog structure                                             */
/*****************************************************************************/
#define     BML_PRELOG_NOP                          (UINT16) (0)
#define     BML_PRELOG_WRITE                        (UINT16) (1)
#define     BML_PRELOG_ERASE                        (UINT16) (2)
#define     BML_PRELOG_READ                         (UINT16) (4)

/*****************************************************************************/
/* maximum number of uncorrectable read error                                */
/*****************************************************************************/
#define     BML_MAX_UNCORRECTABLE_ERR               (4)

/*****************************************************************************/
/* maximum number of blocks in Erase Refresh List                            */
/*****************************************************************************/
#define     BML_MAX_ERL_ITEM                        (127)

/*****************************************************************************/
/* the number of blocks per bad unit (pBUMap)                                */
/*****************************************************************************/
#define     BML_BLKS_PER_BADUNIT                    (32)
#define     BML_SFT_BLKS_PER_BADUNIT                (5)

/*****************************************************************************/
/* number of meta blocks in Reservoir                                        */
/*****************************************************************************/
#define     BML_RSV_PCB_BLKS        (3)             /* TPCB + LPCB + UPCB    */
#define     BML_RSV_REF_BLKS        (1)             /* REF block             */
#define     BML_RSV_META_BLKS       (BML_RSV_PCB_BLKS + BML_RSV_REF_BLKS)

/*****************************************************************************/
/* Types for reservoir depending on cell type                                */
/*****************************************************************************/
#define     BML_SLC_RESERVOIR               (0x00000001)
#define     BML_MLC_RESERVOIR               (0x00000002)
#define     BML_HYBRID_RESERVOIR            (0x00000003)        /* SLC + MLC */

/*****************************************************************************/
/* nFlag of FSR_BBM_UpdateERL                                                */
/*****************************************************************************/
#define     BML_FLAG_ERL_UPDATE         (0x00000001) /* update list in RAM    */
#define     BML_FLAG_ERL_PROGRAM        (0x00000002) /* program list to Flash */
#define     BML_FLAG_ERL_DELETE         (0x00000003) /* delete list in RAM    */
#define     BML_FLAG_ERL_FORCED_PROGRAM (0x00000004) /* program list by force */
#define     BML_FLAG_ERL_UPCB_DELETE    (0x00000005) /* delete list at UPCB   */

/*****************************************************************************/
/* nFlag of FSR_BBM_RefreshByErase                                           */
/*****************************************************************************/
#define     BML_FLAG_REFRESH_PARTIAL    (0x00000001)  /* refresh partially   */
#define     BML_FLAG_REFRESH_ALL        (0x00000002)  /* refresh all blocks  */
#define     BML_FLAG_NOTICE_READ_ERROR  (0x00000004)  /* notice read error   */
#define     BML_FLAG_REFRESH_USER       (0x00000008)  /* user's request      */

/*****************************************************************************/
/* nFlag of FSR_BBM_HandleBadBlk                                             */
/*****************************************************************************/
/* Flags for error type */
#define     BML_HANDLE_WRITE_ERROR      (0x00000001)
#define     BML_HANDLE_ERASE_ERROR      (0x00000002)

/* Flags to recover corrupted LSB paired page for previous write error */
#define     BML_RECOVER_PREV_ERROR      (0x00010000)

/*****************************************************************************/
/* nFlag of FSR_BBM_RestorePrevData & FSR_BBM_BackupPrevData                 */
/*****************************************************************************/
#define     BML_FLAG_NEXT_PREV_DATA     (0x00000001)
#define     BML_FLAG_PREV_DATA          (0x00000002)

/*****************************************************************************/
/* nFlag of FSR_BBM_GetBMI                                                          */
/*****************************************************************************/
#define     BML_FLAG_GET_BMI            (0x00000001)
#define     BML_FLAG_GET_PAIRED_BMI     (0x00000002)
/**
  * @brief  Data structure for storing the address
  */
typedef struct
{
    UINT32         nPDev;           /**< Physical device number            */
    UINT32         nPgOffset;       /**< Page offset                       */
    UINT32         nDieIdx;         /**< Die Index                         */
    UINT32         nRdFlag;         /**< flag of previous LLD operation    */
    UINT8         *pMBuf;           /**< main buffer pointer               */
    UINT8         *pExtraMBuf;      /**< extra main buffer pointer         */
    FSRSpareBuf   *pSBuf;           /**< spare buffer pointer              */
    FSRSpareBuf    stExtraSBuf;     /**< extra spare buffer pointer        */
} BmlAddrLog;

/**
 * @brief  Data structure for storing the info. about the previous operation
 */
typedef struct
{
    UINT16         nOpType;           /**< operation type                    */
    UINT16         nSbn;              /**< semi-physical blk number          */
    UINT32         nPgOffset;         /**< page offset (SLC:0~63 /MLC:0~127) */
    UINT32         nFlag;             /**< flag of operation                 */
} BmlPreOpLog;

/**
 * @brief  Data structure for storing the info. about block and progress
 */
typedef struct
{
    UINT16      nSbn;               /**< Sbn of a block to be refresed       */
    UINT16      nProgInfo;          /**< progress info of erase refreshing   */
} BmlERBlkInfo;

/**
 * @brief  Data structure for storing the info. about ERL (Erase Refresh List)
 */
typedef struct
{
    UINT16          nCnt;               /**< count of blocks to be refreshed    */
    UINT16          nRsv;
    BmlERBlkInfo    astERBlkInfo[BML_MAX_ERL_ITEM]; /**< info of blocks to be refreshed*/
} BmlERL;

/**
 * @brief  Data structure for storing the info. about BMI(Block Map Page)
 */
typedef struct
{
    UINT16     nNumOfBMFs;       /**< the number of BMFs                      */
    UINT16     nNumOfRCBs;       /**< the number of reservoir candidate blocks*/
} BmlBMI;

/**
 * @brief  Data structure for fast BMF look-up
 */
typedef struct
{
    UINT16     n1stBMFIdx;      /**< 1st BMF index                           */
    UINT16     nNumOfBMFs;      /**< the number of BMFs in BadUnit           */
} BmlBadUnit;

/**
 * @brief  Shared data structure for the information about Reservoir
 */
typedef struct
{
    UINT16      nUPCBSbn;        /**< Unlockable Pool Control Block number    */
    UINT16      nLPCBSbn;        /**< tightly Lockable Pool Control Block number*/

    UINT16      nTPCBSbn;        /**< temporary Pool Control Block number     */
    UINT16      nREFSbn;         /**< REF block number                        */

    UINT32      nGlobalPCBAge;   /**< Global age of latest PCB                */
    UINT32      nUPcbAge;        /**< Age of latest UPCB                      */
    UINT32      nLPcbAge;        /**< Age of latest LPCB                      */

    UINT16      nNextUPCBPgOff;  /**< next page offset in current UPCB        */
    UINT16      nNextLPCBPgOff;  /**< next page offset in current LPCB        */
    UINT16      nNextREFPgOff;   /**< next page offset in current REF block   */
    
    UINT8       nFirstUpdateUPCB; /**< initial update flag for softprogram   */
    UINT8       nFirstUpdateLPCB; /**< initial update flag for softprogram   */

    UINT32      nERLCntInRAM;    /**< number of ERL list in RAM(not programmed)*/

    BOOL32      bKeepLPCB;     /**< Is LPCB data from the latest programmed page?*/
    BOOL32      bKeepUPCB;     /**< Is UPCB data from the latest programmed page?*/
} BmlReservoirSh;

/**
 * @brief  Data structure for the information about Reservoir
 */
typedef struct
{
    /* Sorts by ascending power of stBMI.pstBMF[x]->nSbn */
    BmlBMI      *pstBMI;        /**< Block Map Page                          */
    BmlBMF      *pstBMF;        /**< Block Map Field                         */
    UINT16      *pstRCB;        /**< Reserved Candidate Block                */
    UINT8       *pBABitMap;     /**< Block Allocation Bit Map in Reservoir   */
    UINT32       nNumOfBUMap;   /**< the number of BUMaps                    */
    BmlBadUnit  *pBUMap;        /**< Bad Unit Map in User Area
                                      This field is used 
                                      to decrease a loop-up time of BMF     */
    BmlERL      *pstERL;        /**< Erase Refresh List                     */

    UINT32      n1stSbnOfRsvr;  /**< first block index of reservoir         */
    UINT32      nLastSbnOfRsvr; /**< Last block index of reservoir          */
    UINT32      n1stSbnOfMLC;   /**< 1stSbn of MLC area for F-OneNAND(S+M)  */
    UINT32      nRsvrType;      /**< SLC only, MLC only, SLC+MLC type       */
    UINT32      nNumOfRsvrInSLC; /**< # of Rsv blocks for SLC               */
    UINT32      nNumOfRsvrInMLC; /**< # of Rsv blocks for MLC               */

    UINT8         *pMBuf;        /**< main buffer pointer                   */
    FSRSpareBuf   *pSBuf;        /**< spare buffer pointer                  */
} BmlReservoir;

/**
 * @brief   Non-blocking Operation Context
 */
typedef struct
{
    UINT32      nIntBit;                /**< Interrupt bit                  */
    UINT32      nStartWay;              /**< Start Way                      */
} BmlNBCxt;

/**
 * @brief  typedefs for Die Context
 */
typedef struct
{
    BmlPreOpLog    *pstPreOp;                   /**< previous Op-Cxt                    */
    UINT8          *pPreOpMBuf;                 /**< main buffer pointer for PreOp      */
    FSRSpareBuf    *pPreOpSBuf;                 /**< spare buffer pointer for PreOp     */

    BmlPreOpLog    *pstNextPreOp;               /**< next previous Op-Cxt               */
    UINT8          *pNextPreOpMBuf;             /**< main buffer pointer for NextPreOp  */
    FSRSpareBuf    *pNextPreOpSBuf;             /**< spare buffer pointer for NextPreOp */

    BmlReservoirSh *pstRsvSh;                   /**< shared reservoir structure         */
    BmlReservoir   *pstRsv;                     /**< reservoir structure                */

    UINT32          nCntRDErr;                  /**< # of uncorrrectable read error     */

    UINT16          nNumOfLLDOp;                /**< if nNumOfLLDOp = 0, 2X-operation
                                                     if not, 1X-Operation               */

    UINT16          nPrevPartID;                /**< partition ID for the previous Op.  */
    UINT32         *pnRetOfPartI;               /**< pointer array for return value
                                                                    of each partitions  */

    BOOL32          bPrevErr;                   /**< TRUE32 : previous error generated dummy program
                                                     FALSE32: No previous error     */
    INT32           nPrevErrRet;                /**< Error value of dummy program   */

    UINT16          nCurPbn[FSR_MAX_PLANES];    /**< array of Pbn                   */
    UINT16          nCurSbn[FSR_MAX_PLANES];    /**< array of Sbn                   */

    UINT8          *pMBuf;                      /**< main buffer pointer            */
    FSRSpareBuf    *pSBuf;                      /**< spare buffer pointer           */
} BmlDieCxt;

/**
 * @brief  typedefs for Device Context
 */

typedef struct
{
    UINT16      nDevNo;                      /**< physical device number     */
    UINT16      nRsvBits;                    /**< reserved bits to align a   */
                                             /**< structure                  */
    UINT32      nOTPStatus;                  /**< status of OTP bloc         */

    UINT32      nNumOfSLCBlksInDie[FSR_MAX_DIES];
                                             /**< # of blocks for SLC area   */
    BmlDieCxt  *pstDie[FSR_MAX_DIES];        /**< die context pointer        */
} BmlDevCxt;

/**
 * @brief  typedefs for Dump Log Context
 */
typedef struct
{
    UINT16      nPDev;          /**< physical device number                 */
    UINT16      nDieIdx;        /**< Die Index                              */
    UINT32      nSeqOrder;      /**< current sequent order                  */
    BOOL32      bOTPBlk;        /**< TRUE32 : Dump OTP Block
                                     FALSE32: No OTP Block/Be already dumped*/
    BOOL32      bPIBlk;         /**< TRUE32 : Dump PI Block
                                     FALSE32: No PI Block/Be already dumped */
    BOOL32      bDataBlk;       /**< TRUE32 : Dump Data block
                                     FALSE32: Be already dumped             */
    UINT32      nPEntryNum;     /**< Current dump partition entry number    */
    UINT32      nPbn;           /**< physical block number                  */
} BmlDumpLog;

/**
 * @brief  Data structure for storng the info. about volume information
 */
typedef struct
{
        BOOL32     *pbUseSharedMemory;    /**< Is a shared memory used?         */
        UINT32     *pnSharedOpenCnt;      /**< open count for shared memory     */

        UINT32      n1stVpnOfMLC;         /**< The first Vpn of MLC Blk         */
        UINT32      nSftDDP;              /**< If a device is DDP, nDDPFlag = 1 */
                                          /**< If not, nDDPFlag = 0             */
        UINT32      nDDPMask;             /**< If a device is DDP, nDDPMask = 1 */
                                          /**< If not, nDDPMask = 0             */
        UINT32      nNumOfDev;            /**< # of devices in volume           */
        UINT32      nNumOfPlane;          /**< # of planes in device            */
        UINT32      nNumOfDieInDev;       /**< # of dies in device              */
        UINT32      nNumOfBlksInDie;      /**< # of blocks in a chip            */
        UINT32      nNumOfBlksInDev;      /**< # of blocks in a device          */
        UINT32      nNumOfWays;           /**< # of ways in volume              */
        UINT32      nNumOfSLCUnit;        /**< # of SLC Units                   */
        UINT32      nNumOfUsBlks;         /**< # of usable blks in volume       */
        UINT32      nNumOfPgsInSLCBlk;    /**< # of pages in SLC Blk            */
        UINT32      nNumOfPgsInMLCBlk;    /**< # of pages in MLC Blk            */
        UINT32      nLastVpn;             /**< the last vpn                     */
        UINT32      nLastUnit;            /**< the last unit number             */
        UINT32      nSctsPerPg;           /**< # of scts in page                */

        UINT32      nMaskPDev;            /**< Mask for PDev                    */
        UINT32      nMaskWays;            /**< Mask for ways                    */
        UINT32      nMaskSLCPgs;          /**< Mask for page of SLC blk         */
        UINT32      nMaskMLCPgs;          /**< Mask for page of MLC blk         */
        UINT32      nSftNumOfBlksInDie;   /**< # of shift bits for blks in die  */
        UINT32      nSftNumOfWays;        /**< # of shift bits for ways         */
        UINT32      nSftNumOfDev;         /**< # of shift bits for device       */
        UINT32      nSftNumOfPln;         /**< # of shift bits for plane        */
        UINT32      nSftSLC;              /**< # of shift bits for SLC blks     */
        UINT32      nSftMLC;              /**< # of shift bits for MLC blks     */

        UINT32      bVolOpen;             /**< volume open flag                 */
        UINT32      nOpenCnt;             /**< volume open count                */

        UINT32      nSizeOfVPage;         /**< The size of virtual page(main only)*/
        UINT32      nSizeOfPage;          /**< the size of physical page (main only)*/
        UINT32      nSparePerSct;         /**< the size of spare area per sector*/
        BOOL32      bPreProgram;          /**< pre-programming en-dis/able      */
        BOOL32      bOTPEmul;             /**< OTP emulation ?                  */
        BOOL32      bOTPEnable;           /**< OTP mode is enabled ?            */
        BOOL32      b1stBlkOTP;           /**< support 1st block OTP or not     */

  const UINT8      *pPairedPgMap;         /**< paired page mapping information  */
  const UINT8      *pLSBPgMap;            /**< array of the numbers of LSB pages*/
        SM32        nLockHandle;          /**< handle # for lock (PAM/OAM lock) */
        UINT32      nNBEvent;             /**< handle # for event 
                                                (Only non-blocking mode)        */

        UINT32      nNumOfRsvrBlks;       /**< # of reservoir blocks            */          

        BOOL32      bCachedProgram;       /**< Flag for cached program operation*/
        BOOL32      bNonBlkMode;          /**< Flag for NonBlocking Mode        */

        UINT16      nNANDType;            /**< NAND types                       */

        UINT16      nPgBufToDataRAMTime;  /**< time for moving data of 1 page
                                                    from Page Buffer to DataRAM */
        UINT32      nSLCTLoadTime;        /**< Typical Load     operation time  */
        UINT32      nMLCTLoadTime;        /**< Typical Load     operation time  */
        UINT32      nSLCTProgTime;        /**< Typical Program  operation time  */
        UINT32      nMLCTProgTime[2];     /**< Typical Program  operation time  */
        UINT32      nTEraseTime;          /**< Typical Erase    operation time  */
        UINT32      nWrTranferTime;       /**< write transfer time              */
        UINT32      nRdTranferTime;       /**< read transfer time               */

        /* endurance information */
        UINT32      nSLCPECycle;        /**< program, erase cycle of SLC block   */
        UINT32      nMLCPECycle;        /**< program, erase cycle of MLC block   */

        UINT8       nUID[FSR_LLD_UID_SIZE]; /**< Unique ID info. about OTP block  */
        
        UINT16      nDID;               /**< Device ID                        */
        UINT16      nUserOTPScts;       /**< # of user sectors               */

        FSRPartI    *pstPartI;             /**< partition information            */
        FSRPIExt    *pPIExt;               /**< partition information extension  */
        BmlAddrLog  *pstAddr[FSR_MAX_WAYS];/**< address log                      */
        BmlNBCxt    *pstNBCxt;             /**< Non-blocking Op Context pointer  */
        BmlDumpLog  *pstDumpLog;           /**< Dump Log Context pointer         */

        BOOL32      (*AcquireLock)             (SM32           nHandle,
                                                UINT32         nLayer);
        BOOL32      (*ReleaseLock)             (SM32           nHandle,
                                                UINT32         nLayer);

        INT32       (*LLD_Init)                (UINT32         nFlag);
        INT32       (*LLD_Open)                (UINT32         nDev,
                                                VOID          *pParam,
                                                UINT32         nFlag);
        INT32       (*LLD_Close)               (UINT32         nDev,
                                                UINT32         nFlag);
        INT32       (*LLD_Erase)               (UINT32         nDev,
                                                UINT32        *pnPbn,
                                                UINT32         nNumOfBlks,
                                                UINT32         nFlag);
        INT32       (*LLD_ChkBadBlk)           (UINT32         nDev,
                                                UINT32         nPbn,
                                                UINT32         nFlag);
        INT32       (*LLD_FlushOp)             (UINT32         nDev,
                                                UINT32         nDieIdx,
                                                UINT32         nFlag);
        INT32       (*LLD_GetDevSpec)          (UINT32         nDev,
                                                FSRDevSpec    *pstDevSpec,
                                                UINT32         nFlag);
        INT32       (*LLD_Read)                (UINT32         nDev, 
                                                UINT32         nPbn,
                                                UINT32         nPgOffset,
                                                UINT8         *pMBuf,
                                                FSRSpareBuf   *pSBuf,
                                                UINT32         nFlag);
        INT32       (*LLD_Write)               (UINT32         nDev, 
                                                UINT32         nPbn,
                                                UINT32         nPgOffset,
                                                UINT8         *pMBuf,
                                                FSRSpareBuf   *pSBuf,
                                                UINT32         nFlag);
        INT32       (*LLD_CopyBack)            (UINT32         nDev,
                                                LLDCpBkArg    *pstCpArg,
                                                UINT32         nFlag);
        INT32       (*LLD_GetPrevOpData)       (UINT32         nDev,
                                                UINT8         *pMBuf,
                                                FSRSpareBuf   *pSBuf,
                                                UINT32         nDieIdx,
                                                UINT32         nFlag);
        INT32       (*LLD_IOCtl)               (UINT32         nDev,
                                                UINT32         nCode,
                                                UINT8         *pBufI,
                                                UINT32         nLenI,
                                                UINT8         *pBufO,
                                                UINT32         nLenO,
                                                UINT32        *pByteRet);
        INT32       (*LLD_InitLLDStat)         (VOID);
        INT32       (*LLD_GetStat)             (FSRLLDStat    *pStat);
        INT32       (*LLD_GetBlockInfo)        (UINT32         nDev,
                                                UINT32         nPbn,
                                                UINT32        *pBlockType,
                                                UINT32        *pPgsPerBlk);
        INT32       (*LLD_GetNANDCtrllerInfo)  (UINT32             nDev,
                                                LLDPlatformInfo   *pLLDPltInfo);
} BmlVolCxt;

/*****************************************************************************/
/** BML Read Function Table Data Structures                                  */
/*****************************************************************************/

/**
 * @brief  typedefs for Shared Context
 */
typedef struct
{
    INT32   (*HandlePrevError) (UINT32 nVol, UINT32 nPDev, UINT32 nDieIdx, INT32 nLLDRe);
    INT32   (*UpdateERL)       (BmlVolCxt *pstVol, BmlDevCxt *pstDev, UINT32 nDieIdx, UINT32 nOrgSbn,UINT32 nFlag);
}BmlShCxt;

#endif /* _FSR_BML_TYPES_H_ */
