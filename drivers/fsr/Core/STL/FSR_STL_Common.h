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
 *  @file       FSR_STL_Common.h
 *  @brief      This header file defines functions in STL.
 *  @author     Wonmoon Cheon
 *  @date       02-MAR-2007
 *  @remark
 *  REVISION HISTORY
 *  @n  02-MAR-2007 [Wonmoon Cheon] : first writing
 *  @n  26-JUN-2007 [Wonmoon Cheon] : re-design
 *  @n  29-JAN-2008 [MinYoung Kim] : dead code elimination
 *  @n  29-JAN-2008 [MinYoung Kim] : dead code elimination
 *
 */

#ifndef __FSR_STL_COMMON_H__
#define __FSR_STL_COMMON_H__

/*****************************************************************************/
/* Header file inclusions                                                    */
/*****************************************************************************/
#if defined (FSR_POR_USING_LONGJMP)
#include "FSR_FOE_Interface.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus*/


/*****************************************************************************/
/* Macro                                                                     */
/*****************************************************************************/
#define GET_META_PAGE_SIZE(_METAVER)            (((_METAVER) >> 8) & 0x000000FF)

#define GET_REMAINDER(_DIV, _BITSHIFT)          ((_DIV) & ((1 << (_BITSHIFT)) - 1))

/* 
* Get CRC32 Spare Index to support one CRC32 per two sectors
* 2KB : nIdx(0,2)      ==> nSpareIdx(0,1)
* 4KB : nIdx(0,2,4,6)  ==> nSpareIdx(0,1,4,5)
* ex) 2 ==> ((0>>2) >> 2) + ((1 & 3) >> 1) ==> 1
* ex) 6 ==> ((6>>2) >> 2) + ((6 & 3) >> 1) ==> 5
*/
#define GET_CRC32_SPARE_IDX(nIdx)               (((nIdx) >> 2) << 2) + (((nIdx) & 3) >> 1)

/* For Flexia to support big-endian image */
#if defined(FSR_SUPPORT_BE_IMAGE_FOR_FLEXIA)
#define ENDIAN_SWAP_MAIN_FOR_TOOL(pstObject, pMBuf, nPgType)       FSR_STL_EndianSwapSTLMetaInfo((pstObject), (pMBuf), (nPgType))
#define ENDIAN_SWAP_SPARE_FOR_TOOL(pBuf, nSpType)                  FSR_STL_EndianSwapSpare((pBuf), (nSpType))
#else
#define ENDIAN_SWAP_MAIN_FOR_TOOL(pstZone, pMBuf, nPgType)         
#define ENDIAN_SWAP_SPARE_FOR_TOOL(pBuf, nSpType)              
#endif  /* defined(FSR_SUPPORT_BE_IMAGE_FOR_FLEXIA) */


/*****************************************************************************/
/* Type defines                                                              */
/*****************************************************************************/

/*****************************************************************************/
/* Constant definitions                                                      */
/*****************************************************************************/
#define FSR_STL_SPARE_EXT_MAX_SIZE  (8)
#define FSR_STL_CPBK_SPARE_BASE     (FSR_LLD_CPBK_SPARE      + FSR_SPARE_BUF_BML_BASE_SIZE)
#define FSR_STL_CPBK_SPARE_EXT      (FSR_STL_CPBK_SPARE_BASE + FSR_SPARE_BUF_STL_BASE_SIZE)


/*****************************************************************************/
/* Exported variable declarations                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Global variable definition                                                */
/*****************************************************************************/
extern PUBLIC STLPartObj      gstSTLPartObj[FSR_MAX_VOLS][FSR_MAX_STL_PARTITIONS];
extern PUBLIC STLClstObj     *gpstSTLClstObj[MAX_NUM_CLUSTERS];

extern PUBLIC const UINT32 	  CRC32_Table[4][256];


/*****************************************************************************/
/* Exported function prototype                                               */
/*****************************************************************************/

/*---------------------------------------------------------------------------*/
/* STL Cluster & Zone interface functions                                    */

PUBLIC INT32            FSR_STL_InitCluster        (UINT32          nClstID,
                                                    UINT32          nVolID,
                                                    UINT32          nPart);

PUBLIC INT32            FSR_STL_FormatCluster      (UINT32          nClstID,
                                                    UINT16          nStartVbn,
                                                    UINT32          nNumZones,
                                                    UINT16         *pnBlksPerZone,
                                                    UINT16         *pnWBBlksPerZone,
                                                    FSRStlFmtInfo  *pstFmt);

PUBLIC INT32            FSR_STL_OpenCluster        (UINT32          nClstID,
                                                    UINT32          nFlag);

PUBLIC INT32            FSR_STL_CloseCluster       (UINT32          nClstID);

PUBLIC INT32            FSR_STL_CloseZone          (UINT32          nClstID,
                                                    UINT32          nZoneID);

PUBLIC INT32            FSR_STL_ReadZone           (UINT32          nClstID,
                                                    UINT32          nZoneID,
                                                    UINT32          nLsn,
                                                    UINT32          nNumOfScts,
                                                    UINT8          *pBuf,
                                                    UINT32          nFlag);

PUBLIC INT32            FSR_STL_WriteZone          (UINT32          nClstID,
                                                    UINT32          nZoneID,
                                                    UINT32          nLsn,
                                                    UINT32          nNumOfScts,
                                                    UINT8          *pBuf,
                                                    UINT32          nFlag);

#if (OP_SUPPORT_PAGE_DELETE == 1)
PUBLIC INT32            FSR_STL_DeleteZone         (UINT32          nClstID,
                                                    UINT32          nZoneID,
                                                    UINT32          nLsn,
                                                    UINT32          nNumOfScts,
                                                    UINT32          nFlag);
#endif  /* OP_SUPPORT_PAGE_DELETE == 1      */


/*---------------------------------------------------------------------------*/
/* FSR_STL_Init.c                                                            */

PUBLIC VOID             FSR_STL_InitVFLParamPool   (STLClstObj     *pstClst);

PUBLIC VOID             FSR_STL_SetDirHdrHdl       (STLZoneObj     *pstZone,
                                                    STLDirHdrHdl   *pstDH,
                                                    UINT8          *pBuf,
                                                    UINT32          nBufSize);

PUBLIC VOID             FSR_STL_SetCtxInfoHdl      (STLZoneObj     *pstZone,
                                                    STLCtxInfoHdl  *pstCtx,
                                                    UINT8          *pBuf,
                                                    UINT32          nBufSize);

PUBLIC VOID             FSR_STL_SetBMTHdl          (STLZoneObj     *pstZone,
                                                    STLBMTHdl      *pstBMT,
                                                    UINT8          *pBuf,
                                                    UINT32          nBufSize);

PUBLIC VOID             FSR_STL_SetLogGrpHdl       (STLZoneObj     *pstZone,
                                                    STLLogGrpHdl   *pstLogGrp,
                                                    UINT8          *pBuf,
                                                    UINT32          nBufSize);

PUBLIC VOID             FSR_STL_SetPMTHdl          (STLZoneObj     *pstZone,
                                                    STLPMTHdl      *pstPMT,
                                                    UINT8          *pBuf,
                                                    UINT32          nBufSize);

PUBLIC VOID             FSR_STL_InitDirHdr         (STLZoneObj     *pstZone,
                                                    STLDirHdrHdl   *pstDH);

PUBLIC VOID             FSR_STL_InitCtx            (STLZoneObj     *pstZone,
                                                    STLCtxInfoHdl  *pstCtx);

PUBLIC VOID             FSR_STL_InitBMT            (STLZoneObj     *pstZone,
                                                    STLBMTHdl      *pstBMT,
                                                    BADDR           nLan);

PUBLIC VOID             FSR_STL_InitDelCtx         (STLDelCtxObj   *pstDelCtx);

PUBLIC VOID             FSR_STL_InitLog            (STLZoneObj     *pstZone,
                                                    STLLogGrpHdl   *pstLogGrp,
                                                    STLLog         *pstLog);

PUBLIC VOID             FSR_STL_InitLogGrp         (STLZoneObj     *pstZone, 
                                                    STLLogGrpHdl   *pstLogGrp);

PUBLIC VOID             FSR_STL_InitLogGrpList     (STLLogGrpList  *pstLogGrpList);

PUBLIC INT32            FSR_STL_InitRootInfo       (STLDevInfo     *pstDev,
                                                    STLRootInfo    *pstRootInfo);

PUBLIC INT32            FSR_STL_InitMetaLayout     (STLDevInfo     *pstDev,
                                                    STLRootInfo    *pstRI,
                                                    STLMetaLayout  *pstML);


/*---------------------------------------------------------------------------*/
/* FSR_STL_ZoneMgr.c                                                         */

PUBLIC VOID             FSR_STL_ResetZoneObj       (UINT32          nVolID,
                                                    STLDevInfo     *pstDevInfo,
                                                    STLRootInfo    *pstRI,
                                                    STLMetaLayout  *pstML,
                                                    STLZoneObj     *pstZone);

PUBLIC INT32            FSR_STL_AllocZoneMem       (STLZoneObj     *pstZone);

PUBLIC VOID             FSR_STL_FreeZoneMem        (STLZoneObj     *pstZone);

PUBLIC VOID             FSR_STL_InitZoneInfo       (STLZoneInfo    *pstZI);

PUBLIC INT32            FSR_STL_SetupZoneInfo      (STLClstObj     *pstClst,
                                                    STLZoneObj     *pstZone,
                                                    UINT16          nStartVbn,
                                                    UINT16          nNumBlks,
                                                    UINT16          nNumWBBlks,
                                                    UINT16          nNumInitFBlks,                                                     
                                                    UINT16          nNumMinMetaBlks);

PUBLIC VOID             FSR_STL_InitZone           (STLZoneObj     *pstZone);


/*---------------------------------------------------------------------------*/
/* FSR_STL_Format.c                                                          */


/*---------------------------------------------------------------------------*/
/* FSR_STL_Open.c                                                            */


/*---------------------------------------------------------------------------*/
/* FSR_STL_Read.c                                                            */


/*---------------------------------------------------------------------------*/
/* FSR_STL_Write.c                                                           */

PUBLIC INT32            FSR_STL_Convert_ExtParam   (STLZoneObj     *pstZone,
                                                    VFLParam       *pstParam);

PUBLIC VOID             FSR_STL_InitCRCs           (STLZoneObj     *pstZone,
                                                    VFLParam       *pstParam);

PUBLIC VOID             FSR_STL_ComputeCRCs        (STLZoneObj     *pstZone,
                                                    VFLParam       *pstSrcParam,
                                                    VFLParam       *pstDstParam,
                                                    BOOL32          bCRCValid);

PUBLIC VOID             FSR_STL_SetLogSData123     (STLZoneObj     *pstZone,
                                                    PADDR           nLpn,
                                                    PADDR           nDstVpn,
                                                    VFLParam       *pstParam,
                                                    BOOL32          bConfirmPage);

PUBLIC INT32            FSR_STL_FlashModiCopybackCRC
                                                   (STLZoneObj     *pstZone,
                                                    PADDR           nSrcVpn,
                                                    PADDR           nDstVpn,
                                                    VFLParam       *pstParam,
                                                    UINT32          nSrcEC);

PUBLIC INT32            FSR_STL_GetLogToWrite      (STLZoneObj     *pstZone,
                                                    PADDR           nLpn,
                                                    STLLogGrpHdl   *pstTargetGrp,
                                                    STLLog        **pstTargetLog,
                                                    UINT32         *pnNumAvailPgs);


/*---------------------------------------------------------------------------*/
/* FSR_STL_Delete.c                                                          */

PUBLIC INT32            FSR_STL_StoreDeletedInfo   (STLZoneObj     *pstZone);


/*---------------------------------------------------------------------------*/
/* FSR_STL_MetaMgr.c                                                         */

PUBLIC INT32            FSR_STL_StoreRootInfo      (STLClstObj     *pstClst);

PUBLIC INT32            FSR_STL_StoreDirHeader     (STLZoneObj     *pstZone,
                                                    BOOL32          bEnableMetaWL, 
                                                    BOOL32         *pbWearLevel,
                                                    UINT32         *pnVictimIdx);

PUBLIC INT32            FSR_STL_ProgramHeader      (STLZoneObj     *pstZone,
                                                    BADDR           nHeaderVbn);

PUBLIC INT32            FSR_STL_ReadHeader         (STLZoneObj     *pstZone,
                                                    BADDR           nHeaderVbn);

PUBLIC INT32            FSR_STL_StoreBMTCtx        (STLZoneObj     *pstZone,
                                                    BOOL32          bEnableMetaWL);

PUBLIC INT32            FSR_STL_LoadBMT            (STLZoneObj     *pstZone,
                                                    BADDR           nLan,
                                                    BOOL32          bOpenFlag);

PUBLIC INT32            FSR_STL_StorePMTCtx        (STLZoneObj     *pstZone,
                                                    STLLogGrpHdl   *pstLogGrp,
                                                    BOOL32          bEnableMetaWL);

PUBLIC INT32            FSR_STL_LoadPMT            (STLZoneObj     *pstZone,
                                                    BADDR           nDgn,
                                                    POFFSET         nMetaPOffs,
                                                    STLLogGrpHdl  **pstLogGrp,
                                                    BOOL32          bOpenFlag);

#if (OP_SUPPORT_RUNTIME_ERASE_REFRESH == 1)
PUBLIC INT32            FSR_STL_StoreRCT           (STLZoneObj     *pstZone,
                                                    UINT32          nRCTOff,
                                                    BOOL32          bEnableMetaWL);

PUBLIC INT32            FSR_STL_LoadRCT            (STLZoneObj     *pstZone,
                                                    UINT32          nRCTOff,
                                                    POFFSET         nMetaPOffs);
#endif  /* (OP_SUPPORT_RUNTIME_ERASE_REFRESH == 1) */

PUBLIC INT32            FSR_STL_CompactMeta        (STLZoneObj     *pstZone,
                                                    UINT32          nSrcIdx);

PUBLIC INT32            FSR_STL_SelectVictimMeta   (STLZoneObj     *pstZone,
                                                    UINT32         *pnVictimIdx);

PUBLIC INT32            FSR_STL_ReclaimMeta        (STLZoneObj     *pstZone,
                                                    UINT32          nVictimIdx);

PUBLIC INT32            FSR_STL_ReserveMetaPgs     (STLZoneObj     *pstZone, 
                                                    UINT16          nReqPgs,
                                                    BOOL32          bEnableMetaWL);


/*---------------------------------------------------------------------------*/
/* FSR_STL_MappingMgr.c                                                      */

PUBLIC POFFSET          FSR_STL_GetWayCPOffset     (STLDevInfo     *pstDev,
                                                    POFFSET         nLogCPOffset,
                                                    PADDR           nLpn);

PUBLIC POFFSET          FSR_STL_SearchMetaPg       (STLZoneObj     *pstZone,
                                                    BADDR           nDgn);

PUBLIC VOID             FSR_STL_UpdatePMTDir       (STLZoneObj     *pstZone,
                                                    BADDR           nDgn,
                                                    POFFSET         nMetaPOffs,
                                                    UINT32          nMinEC,
                                                    BADDR           nMinVbn,
                                                    BOOL32          bOpenFlag);

PUBLIC VOID             FSR_STL_UpdatePMTDirMinECGrp
                                                   (STLZoneObj     *pstZone,
                                                    STLDirHdrHdl   *pstDH);

PUBLIC INT32            FSR_STL_CheckMergeVictimGrp(STLZoneObj     *pstZone,
                                                    STLLogGrpHdl   *pstLogGrp,
                                                    BOOL32          bOpenFlag,
                                                    BOOL32         *pbVictim);

PUBLIC BOOL32           FSR_STL_IsMergeVictimGrp   (STLZoneObj     *pstZone,
                                                    BADDR           nDgn);

PUBLIC VOID             FSR_STL_SetMergeVictimGrp  (STLZoneObj     *pstZone,
                                                    BADDR           nDgn,
                                                    BOOL32          bSet);

PUBLIC BOOL32           FSR_STL_SearchMergeVictimGrp
                                                   (STLZoneObj     *pstZone,
                                                    BADDR          *pnDgn);

PUBLIC STLLogGrpHdl*    FSR_STL_SearchLogGrp       (STLLogGrpList  *pstLogGrpList,
                                                    BADDR           nDgn);

PUBLIC STLLogGrpHdl*    FSR_STL_AllocNewLogGrp     (STLZoneObj     *pstZone,
                                                    STLLogGrpList  *pstLogGrpList);

PUBLIC VOID             FSR_STL_AddLogGrp          (STLLogGrpList  *pstLogGrpList,
                                                    STLLogGrpHdl   *pstLogGrp);

PUBLIC VOID             FSR_STL_MoveLogGrp2Head    (STLLogGrpList  *pstLogGrpList,
                                                    STLLogGrpHdl   *pstLogGrp);

PUBLIC VOID             FSR_STL_RemoveLogGrp       (STLZoneObj     *pstZone,
                                                    STLLogGrpList  *pstLogGrpList,
                                                    STLLogGrpHdl   *pstLogGrp);

PUBLIC STLLogGrpHdl*    FSR_STL_SelectReplacedLogGrp
                                                   (STLLogGrpList  *pstLogGrpList,
                                                    BADDR           nSelfDgn);

PUBLIC STLLog*          FSR_STL_AddNewLog          (STLZoneObj     *pstZone,
                                                    STLLogGrpHdl   *pstLogGrp,
                                                    BADDR           nLbn,
                                                    BADDR           nVbn,
                                                    UINT32          nEC);

PUBLIC BOOL32           FSR_STL_SearchLog          (STLLogGrpHdl   *pstLogGrp,
                                                    BADDR           nVbn,
                                                    STLLog        **pstLog);

PUBLIC VOID             FSR_STL_RemoveLog          (STLZoneObj     *pstZone,
                                                    STLLogGrpHdl   *pstLogGrp,
                                                    STLLog         *pstLog);

PUBLIC STLLog*          FSR_STL_SelectReplacedLog  (STLLogGrpHdl   *pstLogGrp,
                                                    BOOL32          bActiveLog);

PUBLIC VOID             FSR_STL_ChangeLogState     (STLZoneObj     *pstZone,
                                                    STLLogGrpHdl   *pstLogGrp,
                                                    STLLog         *pstLog,
                                                    PADDR           nLpn);

PUBLIC STLLog*          FSR_STL_FindAvailLog       (STLZoneObj     *pstZone,
                                                    STLLogGrpHdl   *pstLogGrp,
                                                    PADDR           nLpn,
                                                    UINT32         *pnNumAvailPgs);

PUBLIC VOID             FSR_STL_AddActLogList      (STLZoneObj     *pstZone,
                                                    BADDR           nDgn,
                                                    STLLog         *pstLog);

PUBLIC VOID             FSR_STL_RemoveActLogList   (STLZoneObj     *pstZone,
                                                    BADDR           nDgn, 
                                                    STLLog         *pstLog);

PUBLIC INT32            FSR_STL_MakeRoomForActLogGrpList    
                                                   (STLZoneObj     *pstZone,
                                                    BADDR           nSelfDgn,
                                                    BOOL32          bEntireLogs);

PUBLIC STLLogGrpHdl*    FSR_STL_NewActLogGrp       (STLZoneObj     *pstZone,
                                                    BADDR           nDgn);

PUBLIC BOOL32           FSR_STL_CheckFullInaLogs   (STLLogGrpHdl   *pstLogGrp);

PUBLIC VOID             FSR_STL_InsertIntoInaLogGrpCache    
                                                   (STLZoneObj     *pstZone,
                                                    STLLogGrpHdl   *pstLogGrp);

PUBLIC INT32            FSR_STL_GetLpn2Vpn         (STLZoneObj     *pstZone,
                                                    PADDR           nLpn,
                                                    BOOL32         *pbDeleted,
                                                    PADDR          *pnVpn,
                                                    BADDR          *pnDgn,
                                                    UINT8          *pnLogIdx,
                                                    UINT32         *pnEC);

PUBLIC PADDR            FSR_STL_GetLogCPN          (STLZoneObj     *pstZone,
                                                    STLLog         *pstLog,
                                                    PADDR           nLpn);

PUBLIC VOID             FSR_STL_UpdatePMT          (STLZoneObj     *pstZone,
                                                    STLLogGrpHdl   *pstLogGrp,
                                                    STLLog         *pstLog,
                                                    PADDR           nLpn,
                                                    PADDR           nVpn);

PUBLIC VOID             FSR_STL_UpdatePMTCost      (STLZoneObj     *pstZone,
                                                    STLLogGrpHdl   *pstLogGrp);

#if (OP_SUPPORT_CLOSE_TIME_UPDATE_FOR_FASTER_OPEN == 1)
PUBLIC VOID             FSR_STL_FlushAllMetaInfo   (STLZoneObj     *pstZone);
#endif  /* (OP_SUPPORT_CLOSE_TIME_UPDATE_FOR_FASTER_OPEN == 1) */


/*---------------------------------------------------------------------------*/
/* FSR_STL_FreeBlkMgr.c                                                      */

PUBLIC BOOL32           FSR_STL_CheckFullFreeList  (STLZoneObj     *pstZone);

PUBLIC INT32            FSR_STL_AddFreeList        (STLZoneObj     *pstZone,
                                                    BADDR           nVbn,
                                                    UINT32          nEC);

PUBLIC INT32            FSR_STL_ReserveFreeBlks    (STLZoneObj     *pstZone,
                                                    BADDR           nDgn,
                                                    UINT32          nNumRsvdFBlks,
                                                    UINT32         *pnNumRsvd);

PUBLIC INT32            FSR_STL_ReserveYFreeBlks   (STLZoneObj     *pstZone,
                                                    BADDR           nDgn,
                                                    UINT32          nNumRsvdFBlks,
                                                    UINT32         *pnNumRsvd);

PUBLIC INT32            FSR_STL_GetFreeBlk         (STLZoneObj     *pstZone,
                                                    BADDR          *pnVbn,
                                                    UINT32         *pnEC);


/*---------------------------------------------------------------------------*/
/* FSR_STL_MergeMgr.c                                                        */

PUBLIC INT32            FSR_STL_StoreMapInfo       (STLZoneObj     *pstZone,
                                                    STLLogGrpHdl   *pstLogGrp,
                                                    STLLog         *pstLog);

PUBLIC INT32            FSR_STL_CompactLog         (STLZoneObj     *pstZone,
                                                    BADDR           nSelfDgn,
                                                    STLLogGrpHdl   *pstLogGrp,
                                                    UINT8           nVictimLogIdx,
                                                    UINT8           nDstLogIdx);

PUBLIC INT32            FSR_STL_MergeLog           (STLZoneObj     *pstZone,
                                                    STLLogGrpHdl   *pstLogGrp,
                                                    UINT8           nVictimLogIdx1,
                                                    UINT8           nVictimLogIdx2);

PUBLIC INT32            FSR_STL_RefreshLog         (STLZoneObj     *pstZone,
                                                    BADDR           nSelfDgn,
                                                    STLLogGrpHdl   *pstLogGrp,
                                                    UINT8           nVictimLogIdx);

PUBLIC INT32            FSR_STL_FreeLog            (STLZoneObj     *pstZone,
                                                    STLLogGrpHdl   *pstLogGrp,
                                                    UINT8           nVictimLogIdx);

PUBLIC INT32            FSR_STL_SelectVictimLog    (STLZoneObj     *pstZone,
                                                    BADDR           nSelfDgn,
                                                    BADDR          *pnVictimDgn,
                                                    UINT8          *pnVictimLogIdx1,
                                                    UINT8          *pnVictimLogIdx2,
                                                    UINT8          *pnDstLogIdx,
                                                    STLLogGrpHdl  **pstLogGrp,
                                                    UINT8          *pnReclaimType);

PUBLIC INT32            FSR_STL_ReclaimLog         (STLZoneObj     *pstZone,
                                                    BADDR           nSelfDgn,
                                                    BADDR           nVictimDgn,
                                                    UINT8           nVictimLogIdx);


/*---------------------------------------------------------------------------*/
/* FSR_STL_WearLevelMgr.c                                                    */

PUBLIC VOID             FSR_STL_UpdateBMTMinEC     (STLZoneObj     *pstZone);

PUBLIC INT32            FSR_STL_UpdatePMTMinEC     (STLZoneObj     *pstZone,
                                                    STLLogGrpHdl   *pstLogGrp,
                                                    UINT32          nRemovalVbn);

PUBLIC INT32            FSR_STL_WearLevelFreeBlk   (STLZoneObj     *pstZone,
                                                    UINT32          nNumBlks,
                                                    BADDR           nRcvDgn);

PUBLIC INT32            FSR_STL_RecoverGWLInfo     (STLZoneObj     *pstZone);

PUBLIC INT32            FSR_STL_GetAllBlksEC       (STLZoneObj     *pstZone,
                                                    UINT32         *paBlkEC,
                                                    UINT32          nNumBlks);


/*-------------------------------------------------------------------------------*/
/* FSR_STL_GCMgr.c                                                               */

PUBLIC INT32            FSR_STL_GC                 (STLZoneObj     *pstZone);

PUBLIC INT32            FSR_STL_ReverseGC          (STLZoneObj     *pstZone,
                                                    UINT32          nFreeSlots);

PUBLIC INT32            FSR_STL_UpdateGCScanBitmap (STLZoneObj     *pstZone,
                                                    BADDR           nLan,
                                                    BOOL32          bSet);


/*---------------------------------------------------------------------------*/
/* FSR_STL_FlashIO.c                                                         */

PUBLIC INT32            FSR_STL_FlashRead          (STLZoneObj     *pstZone,
                                                    UINT32          nVpn,
                                                    VFLParam       *pstParam,
                                                    BOOL32          bCheckReadDisturb,
                                                    UINT32          nEC);

PUBLIC INT32            FSR_STL_FlashCheckRead     (STLZoneObj     *pstZone,
                                                    UINT32          nVpn,
                                                    VFLParam       *pstParam,
                                                    UINT32          nRetryCnt,
                                                    BOOL32          bReadRefresh);

PUBLIC INT32            FSR_STL_FlashReliableRead  (STLZoneObj     *pstZone,
                                                    UINT32          nVpn,
                                                    VFLParam       *pstParam);

PUBLIC INT32            FSR_STL_FlashProgram       (STLZoneObj     *pstZone,
                                                    UINT32          nVpn,
                                                    VFLParam       *pstParam);

PUBLIC INT32            FSR_STL_FlashErase         (STLZoneObj     *pstZone,
                                                    BADDR           nVbn,
                                                    BOOL32          bStoreRCT);

PUBLIC INT32            FSR_STL_FlashCopyback      (STLZoneObj     *pstZone,
                                                    UINT32          nSrcVpn,
                                                    UINT32          nDstVpn,
                                                    UINT32          nSBitmap);

PUBLIC INT32            FSR_STL_FlashModiCopyback  (STLZoneObj     *pstZone,
                                                    UINT32          nSrcVpn,
                                                    UINT32          nDstVpn,
                                                    VFLParam       *pstParam,
                                                    BOOL32          bCheckReadDisturb,
                                                    UINT32          nSrcEC);

PUBLIC BOOL32           FSR_STL_FlashIsLSBPage     (STLZoneObj     *pstZone,
                                                    UINT32          nVpn);

PUBLIC BOOL32           FSR_STL_FlashIsCleanPage   (STLZoneObj     *pstZone,
                                                    VFLParam       *pstParam);


/*---------------------------------------------------------------------------*/
/* FSR_STL_Common.c                                                          */

PUBLIC STLPartObj*      FSR_STL_GetPartObj         (UINT32          nVol,
                                                    UINT32          nPartID);

PUBLIC STLClstObj*      FSR_STL_GetClstObj         (UINT32          nClstID);

PUBLIC VFLParam*        FSR_STL_AllocVFLParam      (STLZoneObj     *pstZone);

PUBLIC VFLExtParam*     FSR_STL_AllocVFLExtParam   (STLZoneObj     *pstZone);

PUBLIC VOID             FSR_STL_FreeVFLParam       (STLZoneObj     *pstZone,
                                                    VFLParam       *pstVFLParam);

PUBLIC VOID             FSR_STL_FreeVFLExtParam    (STLZoneObj     *pstZone,
                                                    VFLExtParam    *pstVFLExtParam);

PUBLIC SBITMAP          FSR_STL_MarkSBitmap        (SBITMAP         nSrcBitmap);

PUBLIC SBITMAP          FSR_STL_SetSBitmap         (UINT32          nLsn,
                                                    UINT32          nSectors,
                                                    UINT32          nSecPerVPg);

PUBLIC INT32            FSR_STL_CheckArguments     (UINT32          nClstID,
                                                    UINT32          nZoneID,
                                                    UINT32          nLsn,
                                                    UINT32          nNumOfScts,
                                                    UINT8          *pBuf);

PUBLIC UINT32           FSR_STL_GetShiftBit        (UINT32          nVal);

PUBLIC UINT32           FSR_STL_GetZBC             (UINT8          *pBuf,
                                                    UINT32          nBufSize);

PUBLIC UINT32           FSR_STL_CalcCRC32          (const UINT8    *pBuf,
                                                    UINT32          nSize);

PUBLIC UINT16           FSR_STL_SetPTF             (STLZoneObj     *pstZone,
                                                    UINT8           nPageType, 
                                                    UINT8           nPageInfo);

PUBLIC VOID             FSR_STL_SetRndInArg        (VFLExtParam    *pExtParam,
                                                    UINT16          nOffset,
                                                    UINT16          nNumOfBytes,
                                                    VOID           *pBuf,
                                                    UINT16          nIdx);

PUBLIC VOID             FSR_STL_SetTimestamp       (STLZoneObj     *pstZone, 
                                                    VFLExtParam    *pstExtParam, 
                                                    BOOL32          bCalledByMeta);

PUBLIC INT32            FSR_STL_LockPartition      (UINT32          nClstID);

PUBLIC INT32            FSR_STL_ChangePartAttr     (STLPartObj     *pstSTLPartObj,
                                                    UINT32          nAttr);

PUBLIC UINT32           FSR_STL_GetWLThreshold     (UINT16          nDeviceType,
                                                    UINT16          nDeviceMode,
                                                    UINT16          nDeviceProcess);

#if (OP_SUPPORT_RUNTIME_ERASE_REFRESH == 1)
PUBLIC VOID             FSR_STL_AddERList          (STLZoneObj     *pstZone,
                                                    BADDR           nDgn,
                                                    BADDR           nIdx,
                                                    BOOL32          b1stER);

PUBLIC VOID             FSR_STL_RemoveERList       (STLZoneObj     *pstZone,
                                                    BADDR           nDgn,
                                                    BADDR           nIdx);

PUBLIC VOID             FSR_STL_SetRC              (STLZoneObj     *pstZone,
                                                    BADDR           nVbn,
                                                    UINT32          nRC,
                                                    BOOL32          bFromWB,
                                                    UINT32         *pnPrevRC,
                                                    UINT32         *pnCurrRC);

PUBLIC VOID             FSR_STL_ResetRC            (STLZoneObj     *pstZone,
                                                    BADDR           nVbn,
                                                    BOOL32          bFromWB,
                                                    UINT32         *pnPrevRC,
                                                    UINT32         *pnCurrRC);

PUBLIC VOID             FSR_STL_GetRCR             (UINT16          nDeviceType,
                                                    UINT16          nDeviceMode,
                                                    UINT16          nDeviceProcess,
                                                    UINT32          nEC,
                                                    UINT32         *pnRC1LV,
                                                    UINT32         *pnRC2LV);

PUBLIC VOID             FSR_STL_PerformRER         (STLZoneObj     *pstZone,
                                                    UINT32          nReqERBlkNum,
                                                    UINT32         *pnERBlkNum,
                                                    BOOL32          b1stER);
#endif  /* (OP_SUPPORT_RUNTIME_ERASE_REFRESH == 1) */

#if (OP_STL_DEBUG_CODE == 1)
PUBLIC VOID             FSR_STL_DBG_CheckValidity  (STLZoneObj     *pstZone);

PUBLIC VOID             FSR_STL_DBG_CheckValidPgCnt(STLZoneObj     *pstZone);

PUBLIC VOID             FSR_STL_DBG_CheckMergeVictim
                                                   (STLZoneObj     *pstZone);

PUBLIC VOID             FSR_STL_DBG_CheckWLGrp     (STLZoneObj     *pstZone);

PUBLIC VOID             FSR_STL_DBG_PMTHistoryLog  (STLZoneObj     *pstZone,
                                                    BADDR           nDgn);

PUBLIC VOID             FSR_STL_GetPTF             (STLZoneObj     *pstZone,
                                                    BADDR          nVbn);

PUBLIC BOOL32           FSR_STL_GetVbn             (STLZoneObj     *pstZone,
                                                    BADDR          nVbn,
                                                    UINT32         *pnBlkCnt);
#endif  /* (OP_STL_DEBUG_CODE == 1) */

#if defined(FSR_SUPPORT_BE_IMAGE_FOR_FLEXIA)
PUBLIC VOID             FSR_STL_EndianSwapSTLMetaInfo
                                                   (VOID           *pstObject,
                                                    UINT8          *pMBuf,
                                                    UINT32          nPgType);

PUBLIC VOID             FSR_STL_EndianSwapSpare    (VOID           *pBuf,
                                                    UINT32          nSpType);
#endif  /* defined(FSR_SUPPORT_BE_IMAGE_FOR_FLEXIA) */


/*---------------------------------------------------------------------------*/
/* FSR_STL_Open.c                                                            */

#if (OP_SUPPORT_RUNTIME_PMT_BUILDING == 1)
PUBLIC INT32            FSR_STL_ScanActLogBlock    (STLZoneObj     *pstZone,
                                                    BADDR           nDgn);
#endif  /* (OP_SUPPORT_RUNTIME_PMT_BUILDING == 1) */


/*---------------------------------------------------------------------------*/
/* FSR_STL_WriteBufferMgr.c                                                  */

#if (OP_SUPPORT_WRITE_BUFFER == 1)
PUBLIC INT32            FSR_STL_OpenWB             (UINT32          nVol,
                                                    UINT32          nPart,
                                                    UINT32          nFlag);

PUBLIC INT32            FSR_STL_CloseWB            (UINT32          nVol,
                                                    UINT32          nPart);

PUBLIC INT32            FSR_STL_FormatWB           (UINT32          nVol,
                                                    UINT32          nPart,
                                                    UINT16         *pnNumOfWBBlks);

PUBLIC INT32            FSR_STL_ReadWB             (UINT32          nVol,
                                                    UINT32          nPart,
                                                    UINT32          nLsn,
                                                    UINT32          nNumOfScts,
                                                    UINT8          *pBuf,
                                                    UINT32          nFlag);

PUBLIC INT32            FSR_STL_WriteWB            (UINT32          nVol,
                                                    UINT32          nPart,
                                                    UINT32          nLsn,
                                                    UINT32          nNumOfScts,
                                                    UINT8          *pBuf,
                                                    UINT32          nFlag);

PUBLIC INT32            FSR_STL_DeleteWB           (UINT32          nVol,
                                                    UINT32          nPart,
                                                    UINT32          nLsn,
                                                    UINT32          nNumOfScts,
                                                    BOOL32          bSyncMode,
                                                    UINT32          nFlag);

PUBLIC INT32            FSR_STL_IsInWB             (UINT32          nVol,
                                                    UINT32          nPart,
                                                    UINT32          nLsn,
                                                    UINT32          nNumOfScts,
                                                    UINT32         *pnNumOfSctsInWB);

PUBLIC INT32            FSR_STL_FlushWB            (UINT32          nVol,
                                                    UINT32          nPart,
                                                    UINT32          nNumOfBlks,
                                                    UINT32          nFlag);

PUBLIC INT32            FSR_STL_GetWBInfo          (UINT32          nVol,
                                                    UINT32          nPart,
                                                    UINT32         *pnTotalBlks,
                                                    UINT32         *pnUsedBlks);
#endif  /* (OP_SUPPORT_WRITE_BUFFER == 1) */


#ifdef __cplusplus
}
#endif /* __cplusplus*/

#endif /* __FSR_STL_COMMON_H__*/
