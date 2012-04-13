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
 *  @file       FSR_STL_Types.h
 *  @brief      Definition for ATL data structures.
 *  @author     Wonmoon Cheon
 *  @date       02-MAR-2007
 *  @remark
 *  REVISION HISTORY
 *  @n  02-MAR-2007 [Wonmoon Cheon] : first writing
 *  @n  14-MAY-2009 [Seunghyun Han] : re-design for v1.2.0 
 *
 */

#ifndef __FSR_STL_TYPES_H__
#define __FSR_STL_TYPES_H__

/*****************************************************************************/
/* Header file inclusions                                                    */
/*****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus*/


/*****************************************************************************/
/* Macro                                                                     */
/*****************************************************************************/
/* STL meta format version */
#define STL_META_VER_2K         0x12000200  /**< V1.2.0 2K format               */
#define STL_META_VER_4K         0x12000400  /**< V1.2.0 4K format               */
#define STL_META_VER_8K         0x12000800  /**< V1.2.0 8K format               */
#define STL_META_VER_16K        0x12001000  /**< V1.2.0 16K format              */

/* STL device type */
#define STL_DEVTYPE_ONENAND     0x00        /**< OneNAND                        */
#define STL_DEVTYPE_FLEXONENAND 0x01        /**< Flex-OneNAND                   */

/* STL device mode */
#define STL_DEVMODE_SLC         0x00        /**< SLC mode                       */
#define STL_DEVMODE_MLC         0x01        /**< MLC mode                       */

/* STL device process technology */
#define STL_DEVPROC_NULL        0x00
#define STL_DEVPROC_63NM        0x01        /**< 63nm technology                */
#define STL_DEVPROC_55nm        0x02        /**< 55nm technology                */
#define STL_DEVPROC_42NM        0x03        /**< 42nm technology                */

/* Log block state (Log.nState) */
#define LS_FREE                 0x00        /**< Free state                     */
#define LS_ALLOC                0x01        /**< Allocated state                */
#define LS_SEQ                  0x02        /**< Sequential state               */
#define LS_RANDOM               0x03        /**< Random state                   */
#define LS_GARBAGE              0x04        /**< Garbage log state              */

#define LS_STATES_BIT_MASK      0x0F        /**< Lower 4bits : state            */
#define LS_ACTIVE_SHIFT         0x04        /**< Active state (bit shift)       */
#define LS_MLC_FAST_MODE_SHIFT  0x05        /**< MLC fast mode flag (bit shift) */

/* Page type field */
#define TF_LOG                  0x01        /**< Log page                       */
#define TF_MERGED_LOG           0x02        /**< Log page moved by merge - It had been BU */
#define TF_META                 0x04        /**< Meta page                      */

/* Page info field for meta pages */
#define MT_ROOT                 0x01        /**< Root meta page                 */
#define MT_HEADER               0x02        /**< Directory header meta page     */
#define MT_BMT                  0x04        /**< BMT meta page                  */
#define MT_PMT                  0x08        /**< PMT meta page                  */
#define MT_RCT                  0x10        /**< RCT meta page                  */

/* Page info field for log pages */
#define MIDLSB                  0x01        /**< LSB page & MID                 */
#define ENDLSB                  0x02        /**< LSB page & END                 */
#define MIDMSB                  0x04        /**< MSB page & MID                 */
#define ENDMSB                  0x08        /**< MSB page & END                 */

/* NULL values */
#define NULL_DGN                (0xFFFF)
#define NULL_VBN                (0xFFFF)
#define NULL_POFFSET            (0xFFFF)
#define NULL_VPN                (0xFFFFFFFF)
#define NULL_LOGIDX             (0xFF)
#define NULL_BLKIDX             (0xFFFF)
#define NULL_METAIDX            (0xFFFFFFFF)

/* Partition status flag */
#define FSR_STL_FLAG_RW_PARTITION       (1 << 0)
#define FSR_STL_FLAG_RO_PARTITION       (1 << 1)
#define FSR_STL_FLAG_LOCK_PARTITION     (1 << 2)

#if defined(FSR_SUPPORT_BE_IMAGE_FOR_FLEXIA)
/* Operation type for endian-change */
#define STL_OP_READ             (0x0)
#define STL_OP_WRITE            (0x1)

/* Meta type to be swapped according endian-type */
#define PG_ROOT                 0x01        /**< Endian swap Root               */
#define PG_HEADER               0x02        /**< Endian swap directory header   */
#define PG_BMT                  0x03        /**< Endian swap BMT                */
#define PG_PMT                  0x04        /**< Endian swap PMT                */
#define PG_RCT                  0x05        /**< Endian swap RCT                */
#define PG_CTX                  0x06        /**< Endian swap context            */
#define PG_BMTCTX               0x07        /**< Endian swap BMT + context      */
#define PG_PMTCTX               0x08        /**< Endian swap PMT + context      */
#define PG_GRP                  0x09        /**< Endian swap LogGroup           */
#define PG_WB_HEADER            0x10        /**< Endian swap Write Buffer Hdr   */

/* Spare type to be swapped according endian-type */
#define SP_VFL                  0x01        /**< VFL Parameter with Ext         */
#define SP_VFLEXT               0x02        /**< VFL Ext Param only             */
#define SP_SBUF                 0x03        /**< FSRSpareBuf only               */
#define SP_WB_SBUF_HDR          0x04        /**< FSRSpareBuf for Write Buffer   */
#define SP_WB_SBUF_DAT          0x05        /**< FSRSpareBuf for Write Buffer   */
#define SP_WB_SBUF_DEL          0x06        /**< FSRSpareBuf for Write Buffer   */
#define SP_WB_METAEXT           0x07        /**< Meta Ext for Write Buffer      */
#endif  /* defined(FSR_SUPPORT_BE_IMAGE_FOR_FLEXIA) */


/****************************************************************************/
/* Global data structure                                                    */
/****************************************************************************/
/**
 * @brief     NAND device attribute structure
 */
typedef struct 
{
    UINT16          nDeviceType;            /**< Type of device                             */
    UINT16          nDeviceMode;            /**< Mode of device                             */
    UINT16          nDeviceProcess;         /**< Process of device                          */
    UINT16          nPadding;

    UINT32          nNumWays;               /**< Number of ways                             */
    UINT32          nPagesPerSBlk;          /**< Count of pages per super block             */
    UINT32          nSecPerSBlk;            /**< Number of sectors per super block          */
    UINT32          nSecPerVPg;             /**< Number of sectors per virtual page         */
    UINT32          nBytesPerVPg;           /**< Bytes per virtual page (main)              */
    UINT32          nFullSBitmapPerVPg;     /**< Full sector bitmap per virtual page        */

    UINT32          nNumWaysShift;
    UINT32          nSecPerVPgShift;
    UINT32          nPagesPerSbShift;

} STLDevInfo;


/**
 *  @brief  Zone information data structure
 */
typedef struct
{
    BADDR           nStartVbn;              /**< Start VBN of zone                          */
    UINT16          nNumTotalBlks;          /**< Number total blocks in one zone            */
    UINT16          nNumMetaBlks;           /**< Number of meta blocks                      */
    UINT8           nNumDirHdrPgs;          /**< Number of directory header pages           */
    UINT8           nNumRCTPgs;             /**< Number of RC meta pages (for RER)          */

    UINT16          nMaxPMTDirEntry;        /**< Max number of entries in PMT dir           */
    UINT16          nNumLogGrpPerPMT;       /**< Number of log groups per PMT               */

    UINT16          nNumFBlks;              /**< Total number of free blocks                */
    UINT16          nNumUserBlks;           /**< Total number of user blocks                */

    UINT32          nNumUserScts;           /**< Number of total user sectors               */
    UINT32          nNumUserPgs;            /**< Number of total user pages                 */

    UINT16          nNumLA;                 /**< Total number of LA                         */
    UINT16          nDBlksPerLA;            /**< Number of managed data blocks per one LA   */
    UINT32          nDBlksPerLAShift;       /**< Data blocks per LA (bit shift)             */

    BADDR           aMetaVbnList[MAX_META_BLKS];
                                            /**< Meta block VBN list                        */
} STLZoneInfo;


/**
 *  @brief  Root information data structure
 */
typedef struct
{
    UINT8           aSignature[8];          /**< STL signature                              */
    UINT32          nVersion;               /**< STL version                                */

    UINT32          nAge_High;              /**< Header age (high 32bits)                   */
    UINT32          nAge_Low;               /**< Header age (low  32bits)                   */

    UINT32          nMetaVersion;           /**< Meta format version                        */

    BADDR           nRootStartVbn;          /**< Root block start VBN                       */
    UINT16          nRootCPOffs;            /**< Current root page offset                   */

    UINT16          nNumZone;               /**< Number of zone                             */
    UINT16          nNumRootBlks;           /**< Number of root blocks                      */

    UINT16          nN;                     /**< Mapping constant 'N'                       */
    UINT16          nK;                     /**< Mapping constant 'k'                       */

    UINT16          nNShift;                /**< 'N' bit shift                              */
    UINT16          nKShift;                /**< 'k' bit shift                              */

    UINT32          aRootBlkEC[MAX_ROOT_BLKS];
                                            /**< Root block erase count info                */
    STLZoneInfo     aZone[MAX_ZONE];        /**< STLZoneInfo array of STL                   */

} STLRootInfo;


/**
 *  @brief  Meta block location information for Root block
 */
typedef struct
{
    STLRootInfo     stRootInfo;
    UINT8           aPadding[ROOT_INFO_BUF_SIZE - sizeof(STLRootInfo)];

} STLRootInfoBuf;


/**
 *  @brief  Meta format layout information
 */
typedef struct
{
    /* Buffer size configuration */    
    UINT32          nBMTBufSize;            /**< BMT buffer size                            */
    UINT32          nCtxBufSize;            /**< Context info buffer size                   */
    UINT32          nPMTBufSize;            /**< PMT buffer size                            */

    /* Sector bitmap configuration */
    UINT32          nDirHdrSBM;             /**< Directory header sector bitmap             */
    UINT32          nBMTCtxSBM;             /**< BMT+CTX sector bitmap                      */
    UINT32          nCtxPMTSBM;             /**< CTX+PMT sector bitmap                      */
    UINT32          nBMTSBM;                /**< BMT sector bitmap                          */
    UINT32          nPMTSBM;                /**< PMT sector bitmap                          */

    /* Meta data size */    
    UINT32          nPagesPerLGMT;          /**< Number of pages per log group PMT          */
    UINT32          nBytesPerLogGrp;        /**< Log group size                             */
    UINT32          nMaxFreeSlots;          /**< Maximum number of slots in free list       */

} STLMetaLayout;


/**
 *  @brief  Zero bit count confirmation structure
 */
typedef struct 
{
    UINT32          nZBC;                   /**< Zero bit count                             */
    UINT32          nInvZBC;                /**< Inverted zero bit count                    */

} STLZbcCfm;


/**
 *  @brief  Directory header structure (fixed size members)
 */
typedef struct
{
    /* Header age information */
    UINT32          nAge_High;              /**< Header age (high 32bits)                   */
    UINT32          nAge_Low;               /**< Header age (low 32bits)                    */

    /* Meta block compaction victim information */
    UINT32          nVictimIdx;             /**< Compaction victim block index              */

    /* Current working meta block information */
    UINT16          nHeaderIdx;             /**< Current latest map header index            */
    POFFSET         nCPOffs;                /**< Current clean page offset (RAM)            */

    /* Header index & context page index for meta compaction */
    UINT16          nLatestBlkIdx;          /**< Block index of the latest CTX page         */
    UINT16          nLatestPOffs;           /**< Page offset of the latest CTX page         */

    /* Idle meta block list */
    UINT16          nIdleBoListHead;        /**< Head index of idle meta block list         */
    UINT16          nNumOfIdleBlk;          /**< Number of idle meta blocks                 */

    /* PMT directory management information */
    UINT16          nMinECPMTIdx;           /**< Index which has minimum erase count        */
    UINT16          nPMTEntryCnt;           /**< Log group entry count in astPMTDir[]       */

} STLDirHeaderFm;


/**
 *  @brief  PMT directory entry
 */
typedef struct
{
    /* nDgn : PMT ratio < 100%, nCost : PMT ratio = 100% */
    UINT16          nCost;                  /**< Compaction cost                            */
    POFFSET         nPOffs;                 /**< Meta page offset                           */

} STLPMTDirEntry;


/**
 *  @brief  PMT wear-leveling group
 */
typedef struct
{
    BADDR           nDgn;                   /**< Data group number                          */
    BADDR           nMinVbn;                /**< Minimum EC blocks' VBN                     */
    UINT32          nMinEC;                 /**< Minimum EC in this group                   */

} STLPMTWLGrp;


/**
 *  @brief  Directory header structure  (RAM manipulation)
 */
typedef struct
{
    /* Variable size members */
    UINT16         *pIdleBoList;            /**< Idle block offset list of meta blocks      */
    UINT16         *pValidPgCnt;            /**< Valid page count of each meta block        */
    UINT32         *pMetaBlksEC;            /**< Erase count of meta blocks                 */
    POFFSET        *pBMTDir;                /**< BMT location directory                     */
    POFFSET        *pRCTDir;                /**< RCT location directory (for RER)           */
    UINT8          *pPMTMergeFlags;         /**< Merge status flag of a log group           */
                                            /* 0 bit : merge -> free block generation (X)   */
                                            /* 1 bit : merge -> free block generation (O)   */
    STLPMTDirEntry *pstPMTDir;              /**< PMT location directory                     */
    STLPMTWLGrp    *pstPMTWLGrp;            /**< PMT wear-leveling group information        */

    STLDirHeaderFm *pstFm;                  /**< Pointer to fixed size members              */

    UINT8          *pBuf;                   /**< Directory header buffer pointer            */
    UINT32          nBufSize;               /**< Buffer size                                */

} STLDirHdrHdl;


/**
 *  @brief  Context information structure (fixed size members)
 */
typedef struct
{
    /* Age information */
    UINT32          nAge_High;              /**< Context age (high 32bits)                  */
    UINT32          nAge_Low;               /**< Context age (low 32bits)                   */

    /* POR Info : Merged log block information for PMTDir update */
    BADDR           nMergedDgn;             /**< Merged DGN: Debugging info                 */
    BADDR           nMergedVbn;             /**< Not used                                   */
    UINT16          nMetaReclaimFlag;       /**< Meta reclaim flag : TRUE(1)/FALSE(0)       */
    BADDR           nMergeVictimDgn;        /**< Merge victim group number during GC/RGC    */
    UINT16          nMergeVictimFlag;       /**< Merge victim group flag : TRUE(1)/FALSE(0) */

    /* POR Info : Modified cost for PMTDir update */
    BADDR           nModifiedDgn;           /**< The DGN of which cost is modified          */
    UINT16          nModifiedCost;          /**< The modified cost                          */

    /* Active log information */
    UINT16          nNumLBlks;              /**< Current number of active log blocks        */

    /* Free block list information */
    UINT16          nNumFBlks;              /**< Current number of free blocks              */
    UINT16          nFreeListHead;          /**< Free block list head position              */

    /* Idle data block information */
    UINT32          nNumIBlks;              /**< Number of idle data block                  */
    
    /* Data wear-leveling information */
    UINT16          nMinECLan;              /**< LAN of minimum EC data block               */

    /* Zone to zone wear-leveling information */
    UINT16          nZoneWLMark;            /**< Zone wear-leveling start/end mark          */
    UINT16          nZone1Idx;              /**< Zone target1 index                         */
    UINT16          nZone2Idx;              /**< Zone target2 index                         */
    BADDR           nZone1Vbn;              /**< Zone target1 VBN                           */
    BADDR           nZone2Vbn;              /**< Zone target2 VBN                           */
    UINT32          nZone1EC;               /**< Zone target1 erase count                   */
    UINT32          nZone2EC;               /**< Zone target2 erase count                   */

    /* PMT wear-leveling information (POR) */
    UINT16          nMinECPMTIdx;           /**< Mimimum EC PMT WL group index              */
    UINT16          nUpdatedPMTWLGrpIdx;    /**< Updated PMT WL group index                 */
    STLPMTWLGrp     stUpdatedPMTWLGrp;      /**< Updated PMT WL group                       */

} STLCtxInfoFm;


/**
 *  @brief  Active Log Entry structure
 */
typedef struct
{
    BADDR           nDgn;                   /**< Data block group number                    */
    BADDR           nVbn;                   /**< Virtual block number                       */

} STLActLogEntry;


/**
 *  @brief  Context Information structure  (RAM manipulation)
 */
typedef struct
{
    /* Fixed size members */
    STLCtxInfoFm   *pstFm;                  /**< Pointer to fixed size members              */

    /* Variable size members */
    STLActLogEntry *pstActLogList;          /**< Active log list                            */
    BADDR          *pFreeList;              /**< Free blocks list                           */
    UINT32         *pFBlksEC;               /**< Erase count list of free blocks            */
    UINT32         *pMinEC;                 /**< Minimum erase count of each LA             */
    UINT16         *pMinECIdx;              /**< Minimum EC block index of each LA          */

    /* Zero bit count confirmation */
    STLZbcCfm      *pstCfm;                 /**< ZBC confirmation                           */
    UINT32          nCfmBufSize;            /**< Target buffer size in ZBC calculation      */

    /* I/O buffer memory */
    UINT8          *pBuf;                   /**< Context info buffer pointer                */
    UINT32          nBufSize;               /**< Buffer size                                */

} STLCtxInfoHdl;

/**
 *  @brief  BMT(Block Mapping Table) entry structure
 */
typedef struct
{
    BADDR           nVbn;                   /**< Virtual block number                       */

#if defined(__GNUC__)
} __attribute ((packed)) STLBMTEntry;
#else
} STLBMTEntry;
#endif


/**
 *  @brief  BMT data structure (fixed size members)
 */
typedef struct
{
    UINT16          nLan;                   /**< LAN (Local Area Number)                    */
    UINT16          nInvLan;                /**< Inverted LAN                               */

} STLBMTFm;


/**
 *  @brief  BMT data structure (RAM manipulation)
 */
typedef struct
{
    /* Fixed size members */
    STLBMTFm       *pstFm;                  /**< Pointer to fixed size members              */

    /* Variable size memebers */
    STLBMTEntry    *pMapTbl;                /**< Data block mapping unit                    */
    UINT32         *pDBlksEC;               /**< Data block erase count table               */
    UINT8          *pGBlkFlags;             /**< garbage flag bitstream                     */

    /* Zero bit count confirmation */
    STLZbcCfm      *pstCfm;                 /**< ZBC confirmation                           */
    UINT32          nCfmBufSize;            /**< Target buffer size in ZBC calculation      */

    /* I/O buffer memory */
    UINT8          *pBuf;                   /**< BMT buffer pointer                         */
    UINT32          nBufSize;               /**< Buffer size                                */

} STLBMTHdl;


/**
 *  @brief  Log block structure (fixed size members).
 */
typedef struct
{
    UINT8           nState;                 /**< Flag, log block state                      */
    
    UINT8           nSelfIdx;               /**< Self index in its log group                */
    UINT8           nPrevIdx;               /**< Previous log index (doubly linked list)    */
    UINT8           nNextIdx;               /**< Next log index (doubly linked list)        */
    
    BADDR           nVbn;                   /**< Log virtual block number                   */
    POFFSET         nLastLpo;               /**< Lastly programmed LPN Offset               */

    POFFSET         nCPOffs;                /**< Clean page offset                          */
    UINT16          nLSBOffs;               /**< LSB page offset in MLC fast mode           */

    UINT16          nNOP;                   /**< NOP of current programming page            */
    UINT16          nCSOffs;                /**< Clean sector offset                        */
                                   
    UINT32          nEC;                    /**< Erase count                                */

} STLLog;


/**
 *  @brief  Log group structure (fixed size members)
 */
typedef struct
{
    BADDR           nDgn;                   /**< Data group number                          */
    UINT16          nMinVPgLogIdx;          /**< Index of log which has minimum valid pages */

    UINT8           nState;                 /**< Log group state                            */
    UINT8           nNumLogs;               /**< Number of logs in this log group           */
    UINT8           nHeadIdx;               /**< Log list head index                        */
    UINT8           nTailIdx;               /**< Log list tail index                        */

    UINT16          nMinVbn;                /**< VBN of log which has minimum erase count   */
    UINT16          nMinIdx;                /**< Index of log which has minimum erase count */
    UINT32          nMinEC;                 /**< Minimum EC                                 */

} STLLogGrpFm;


/**
 *  @brief  Log group structure (RAM manipulation)
 */
typedef struct _LGHdl
{
    /* Fixed size members */
    STLLogGrpFm    *pstFm;                  /**< Log group fixed size members               */

    /* Variable size members */
    STLLog         *pstLogList;             /**< Log list                                   */
    BADDR          *pLbnList;               /**< LBN list in each log block                 */
    UINT16         *pNumDBlks;              /**< Number of mapped data blocks in each log   */
    UINT16         *pLogVPgCnt;             /**< Number of valid pages in each log          */
    POFFSET        *pMapTbl;                /**< Log group page mapping table               */

    /* Zero bit count confirmation */
    STLZbcCfm      *pstCfm;                 /**< ZBC confirmation                           */
    UINT32          nCfmBufSize;            /**< Target buffer size in ZBC calculation      */

    /* I/O buffer memory */
    UINT8          *pBuf;                   /**< Log group buffer pointer                   */
    UINT32          nBufSize;               /**< Buffer size                                */

    struct _LGHdl  *pPrev;                  /**< Previous link                              */
    struct _LGHdl  *pNext;                  /**< Next link                                  */

} STLLogGrpHdl;


/**
 *  @brief  PMT structure to load/store in meta page
 */
typedef struct
{
    UINT32          nNumLogGrps;            /**< Number of log groups                       */

    STLLogGrpHdl    astLogGrps[MAX_LOGGRP_PER_PMT];
                                            /**< Pointer array of log group                 */

    /* I/O buffer memory */
    UINT8          *pBuf;                   /**< PMT buffer pointer                         */
    UINT32          nBufSize;               /**< Buffer size                                */

} STLPMTHdl;


/**
 *  @brief  Log group list structure
 */
typedef struct
{
    UINT32          nNumLogGrps;            /**< Current number of log groups in use        */
    STLLogGrpHdl   *pstHead;                /**< Log group head pointer                     */
    STLLogGrpHdl   *pstTail;                /**< Log group tail pointer                     */

} STLLogGrpList;


#if (OP_SUPPORT_STATISTICS_INFO == 1)
/** 
 *  @brief  Statistics information
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

} STLStats;
#endif  /* (OP_SUPPORT_STATISTICS_INFO == 1) */


/**
 *  @brief  Deleted context structure
 */
typedef struct
{
    BADDR           nDelPrevDgn;            /**< Latest deleted DGN                         */
    BADDR           nDelPrevLan;            /**< Latest deleted LAN                         */
    STLLogGrpHdl   *pstDelLogGrpHdl;        /**< Latest deleted log group pointer           */
    PADDR           nDelLpn;                /**< Latest deleted LPN                         */
    UINT32          nDelSBitmap;            /**< Latest deleted pages' sector bitmap        */

} STLDelCtxObj;


/**
*  @brief  External environment variables
*/
typedef struct
{
    UINT32          nECDiffThreshold;       /**< Wear-leveling EC diff threshold            */
    UINT32          nStartVpn;              /**< Cluster start VPN                          */
    UINT32          nStartVbn;              /**< Cluster start VBN                          */

    STLDevInfo      stDevInfo;              /**< Cluster device info                        */
} STLClstEnvVar;


/**
 *  @brief  Zone global object type
 */
typedef struct
{
    BOOL32          bOpened;                /**< Open or not                                */

    UINT32          nVolID;                 /**< Volume ID                                  */
    UINT32          nPart;                  /**< Partition Index, not exact ID              */

    UINT32          nClstID;                /**< Cluster ID                                 */
    UINT32          nZoneID;                /**< Zone ID                                    */

    STLDevInfo     *pstDevInfo;             /**< Device information                         */

    STLMetaLayout  *pstML;                  /**< Meta layout format                         */

    STLRootInfo    *pstRI;                  /**< Root info                                  */
    STLZoneInfo    *pstZI;                  /**< Zone info                                  */

    STLDirHdrHdl   *pstDirHdrHdl;           /**< Directory header                           */
    UINT8          *pDirHdrBuf;             /**< Directory header buffer                    */

    STLCtxInfoHdl  *pstCtxHdl;              /**< Context info                               */
    STLBMTHdl      *pstBMTHdl;              /**< BMT                                        */
    STLPMTHdl      *pstPMTHdl;              /**< PMT                                        */

    UINT8          *pMetaPgBuf;             /**< Meta page buffer                           */
    UINT8          *pTempMetaPgBuf;         /**< Temporary meta page buffer                 */
    UINT8          *pTempValidPgScanBuf;    /**< Temporary valid page scan buffer           */

    STLLogGrpList  *pstActLogGrpList;       /**< Active log group list                      */
    STLLogGrpHdl   *pstActLogGrpPool;       /**< Active log group handle pool               */
    UINT8          *pActLogGrpPoolBuf;      /**< Active log group memory pool               */
    STLLogGrpList  *pstInaLogGrpCache;      /**< Inactive log group cache                   */
    STLLogGrpHdl   *pstInaLogGrpPool;       /**< Inactive log group handle pool             */
    UINT8          *pInaLogGrpPoolBuf;      /**< Inactive log group memory pool             */

    UINT8          *pGCScanBitmap;          /**< LA updated flag bitmap for GC              */

#if (OP_SUPPORT_PAGE_DELETE == 1)
    STLDelCtxObj   *pstDelCtxObj;           /**< Deleted info                               */
#endif

    UINT8          *pFullBMTBuf;            /**< Full BMT buffer                            */

#if (OP_SUPPORT_STATISTICS_INFO == 1)
    STLStats       *pstStats;               /**< Statistical information                    */
#endif

#if (OP_SUPPORT_RUNTIME_PMT_BUILDING == 1)
    BOOL32          bActLogScanCompleted;   /**< Active log scanning is completed or not    */
    BADDR          *pInitActLogDgnList;     /**< DGN list of initial active log group       */
#endif

#if (OP_SUPPORT_RUNTIME_ERASE_REFRESH == 1)
    UINT32         *pnTotalAccRC;           /**< Total accumulated RC after last NAND sync  */

    UINT32         *pnCurrRCBuf;            /**< Buffer for RC                              */
    UINT32         *pnPrevRCBuf;            /**< Buffer for RC in NAND                      */

    UINT32          n1stERListOff;          /**< Last clean offset of 1st LV ER             */
    BADDR          *p1stERBlkDgnList;       /**< DGN list of 1st LV ER                      */
    BADDR          *p1stERBlkIdxList;       /**< Log or Meta index list of 1st LV ER        */

    UINT32          n2ndERListOff;          /**< Last clean offset of 2nd LV ER             */
    BADDR          *p2ndERBlkDgnList;       /**< DGN list of 2nd LV ER                      */
    BADDR          *p2ndERBlkIdxList;       /**< Log or Meta index list of 2nd LV ER        */

    BOOL32         *pbStoreRCT;             /**< Flags whether storing RCT is needed or not */
#endif

    UINT32          nPORWrTimer;            /**< POR timer for reordering write timer       */
    UINT32          nWrTimer;               /**< Write timer for recording timestamp        */

} STLZoneObj;


/**
 *  @brief  Top level cluster global object type
 */
typedef struct
{
    UINT32          nVolID;                 /**< Volume ID                                  */

    STLRootInfoBuf  stRootInfoBuf;          /**< Root information                           */

    STLMetaLayout   stML;                   /**< Meta page layout info                      */

    STLZoneObj      stZoneObj[MAX_ZONE];    /**< Zone objects array                         */

    STLDevInfo     *pstDevInfo;             /**< Device Info                                */

    UINT32          nVFLParamCnt;           /**< VFL parameter in use count                 */
    VFLParam        astParamPool[MAX_VFL_PARAMS];
                                            /**< VFL parameter pool                         */

    UINT32          nVFLExtCnt;             /**< Extended parameter busy count              */
    VFLExtParam     astExtParamPool[MAX_VFLEXT_PARAMS];         
                                            /**< Extended VFL parameter pool                */

    UINT8          *pTempPgBuf;             /**< Temporary page buffer                      */

    UINT32          nMaxWriteReq;           /**< The maximum BML_Write request              */

    STLClstEnvVar  *pstEnvVar;              /**< External environment variables             */

    FSRSpareBuf     staSBuf[FSR_MAX_WAYS];  /**< FSR spare data pointers                    */
    FSRSpareBufBase staSBase[FSR_MAX_WAYS]; /**< FSR base spare data                        */
    FSRSpareBufExt  staSExt[FSR_MAX_SPARE_BUF_EXT];
                                            /**< FSR extended spare data                    */

    BMLCpBkArg     *pstBMLCpBk[FSR_MAX_WAYS];
                                            /**< FSR BML copyback pointer                   */
    BMLCpBkArg      staBMLCpBk[FSR_MAX_WAYS];
                                            /**< FSR BML copyback arguments                 */
    BMLRndInArg     staBMLRndIn[3];         /**< FSR BML Random-in arguments                */

    BOOL32          bTransBegin;            /**< Does transaction begin?                    */

#if (OP_SUPPORT_MSB_PAGE_WAIT == 1)
    BOOL32          baMSBProg[FSR_MAX_WAYS];/**< Is previous program MSB?                   */
#endif

} STLClstObj;


#if (OP_SUPPORT_WRITE_BUFFER == 1)
/**
 * @brief  Write buffer object type
 */
typedef struct
{
    /* WB device spec */
    UINT32          nNumWays;
    UINT32          nPgsPerBlk;
    UINT32          nPgsPerBlkSft;
    UINT32          nSctsPerPg;
    UINT32          nSctsPerPgSft;

    /* WB partition information */
    BADDR           nStartVbn;              /**< Start VBN of WB                            */
    UINT16          nNumTotalBlks;          /**< # of total blocks in WB                    */
    PADDR           nStartVpn;              /**< Start VPN of WB                            */
    PADDR           nNumTotalPgs;           /**< # of total pages in WB                     */

    /* WB mother partition information */
    UINT32          nNumTotalBlksM;         /**< # of total blocks in mother                */
    UINT32          nNumTotalPgsM;          /**< # of total pages in mother                 */

    /* WB offset pointers */
    BADDR           nHeadBlkOffs;
    BADDR           nTailBlkOffs;
    POFFSET         nHeadPgOffs;
    POFFSET         nTailPgOffs;            /**< Not used. Just for size alignment          */

    /* Header age */
    UINT32          nHeaderAge;

    /* Flag for CloseWB */
    BOOL32          bDirty;

    /* Mapping information */
    POFFSET        *pIndexT;
    POFFSET        *pCollisionT;
    PADDR          *pPgMapT;
    UINT8          *pPgBitmap;

    /* Deleted page list */
    UINT32          nDeletedListOffs;
    PADDR          *pDeletedList;

#if (OP_SUPPORT_RUNTIME_ERASE_REFRESH == 1)
    /* Reclaim block list */
    UINT32          nReclaimListOffs;
    BADDR          *pReclaimList;
#endif

    /* Temporary buffer */
    UINT8           *pMainBuf;
    FSRSpareBuf     *pstSpareBuf;
    FSRSpareBufBase *pstSpareBufBase;
    FSRSpareBufExt  *pstSpareBufExt;

} STLWBObj;
#endif  /* (OP_SUPPORT_WRITE_BUFFER == 1) */

/**
 *  @brief  User level partition information object type
 */
typedef struct _STLPart
{
    UINT32          nVolID;                 /**< Volume ID                                  */
    UINT32          nPart;                  /**< Partition Index, not exact ID              */

    BOOL32          nOpenCnt;               /**< The count of open                          */
    UINT32          nOpenFlag;              /**< Is the read only partition ?               */
    BOOL32          nInitOpenFlag;          /**< The initial flag                           */

    SM32            nSM;                    /**< handle ID for semaphore                    */

    UINT32          nClstID;                /**< The cluster ID of the partition            */
    UINT32          nZoneID;                /**< The Zone ID of the partition               */

    UINT32          nNumUnit;               /**< The # of unit of the partition             */
    UINT32          nNumPart;               /**< The # of partitions (in same GWL group)    */

    struct _STLPart *pst1stPart;            /**< The part ID of the 1st zone                */

    STLClstEnvVar   stClstEV;               /**< Environment variables                      */

#if (OP_SUPPORT_WRITE_BUFFER == 1)
    STLWBObj       *pstWBObj;               /**< Write buffer                               */

    UINT32         *pLRUT;                  /**< Two level LRU table                        */
    UINT32          nLRUTSize;              /**< Size of LRU table                          */
    UINT32          nDecayCnt;              /**< Write count for decaying                   */
    UINT32          nWriteCnt;              /**< Write count                                */
    UINT32          nHitCnt;                /**< Write count to be hit                      */
    UINT32          nLSBPos;                /**< LSB bits position in LRU table             */
#endif

} STLPartObj;


#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /* __FSR_STL_TYPES_H__*/
