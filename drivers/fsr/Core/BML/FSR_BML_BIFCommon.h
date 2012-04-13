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
 *  @file       FSR_BML_BIFCommon.h
 *  @brief      This file consists of common FSR_BML functions
 *  @author     SuRuyn Lee
 *  @date       15-JAN-2007
 *  @remark
 *  REVISION HISTORY
 *  @n  15-JAN-2007 [SuRyun Lee] : first writing
 *  @n  31-MAY-2007 [SuRyun Lee] : seperate original FSR_BML_Interface file
 *
 */
#ifndef _FSR_BML_BIFCOMMON_H_
#define _FSR_BML_BIFCOMMON_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/****************************************************************************/
/*Common Constant definitions                                               */
/****************************************************************************/
/* Previous error mask and base bit for _HandlePrevWrErr, _HandlePrevErErr  */
#define     BML_1STPLN_ERROR                    (FSR_LLD_1STPLN_PREV_ERROR | FSR_LLD_1STPLN_CURR_ERROR)
#define     BML_2NDPLN_ERROR                    (FSR_LLD_2NDPLN_PREV_ERROR | FSR_LLD_2NDPLN_CURR_ERROR)
#define     BML_NEXTPREV_ERROR                  (FSR_LLD_1STPLN_PREV_ERROR | FSR_LLD_2NDPLN_PREV_ERROR)
#define     BML_PREV_ERROR                      (FSR_LLD_1STPLN_CURR_ERROR | FSR_LLD_2NDPLN_CURR_ERROR)

/* 1X CPYBACK ERROR of unpaired block in 2 plane */
#define     BML_CPYBACK_1X_ERROR                (0x10)

#define     BML_SHIFT_BIT_FOR_DUMMY_PGM         (1)

/* # of PreOp logs */
#define     BML_NUM_OF_PREOPLOG                 (2)

/* Mask for lock or locktighten attributes */
#define     BML_LOCK_ATTR_MASK                  (FSR_BML_PI_ATTR_LOCK | FSR_BML_PI_ATTR_LOCKTIGHTEN)

/* # of LLD_CopyBack per page*/
#define     BML_NUM_OF_LLD_CPBK_PER_PAGE                (2)
#define     BML_NUM_OF_CPBK_ERASEREFESH                 (1)

/* # of LLD_Read func. to read one page */
#define     BML_NUM_OF_LLD_READ_PER_PAGE                (2)

/* Mask for NAND type of partition attributes */
#define     BML_PART_ATTR_NANDTYPE_MASK         (0x00003000)

/* Mask for BML format flag */
#define     BML_FORMAT_FLAG_MASK                (0x0000000F)

/* Mask for _Open() to check interrupt ID */
#define     BML_MAX_INT_ID                      (FSR_INT_ID_NAND_7)
/**
 *  @brief      Macros
 */
#if defined(BML_CHK_VOLUME_VALIDATION)
#define CHK_VOL_RANGE(Vol)                                \
        {                                                 \
            if (Vol >= FSR_MAX_VOLS)                      \
           {                                              \
                return FSR_BML_INVALID_PARAM;             \
            }                                             \
        }

#define CHK_VOL_OPEN(VolOpen)                             \
        {                                                 \
            if (VolOpen != TRUE32)                        \
            {                                             \
                return FSR_BML_VOLUME_NOT_OPENED;         \
            }                                             \
        }
#else
#define CHK_VOL_RANGE(Vol)
#define CHK_VOL_OPEN(VolOpen)
#endif

#define CHK_VOL_POINTER(pstVol)                                 \
        {                                                       \
            if ((pstVol == NULL) || (pstVol->nNumOfDev == 0))   \
            {                                                   \
                return FSR_BML_PAM_ACCESS_ERROR;                \
            }                                                   \
        }

/**
 *  @brief      Static function prototypes
 */
PUBLIC BOOL32      *_GetPartAttrChgHdl  (UINT32   nVol,
                                         UINT32   nPartID);
PUBLIC BmlVolCxt   *_GetVolCxt          (UINT32         nVol);
PUBLIC BmlDevCxt   *_GetDevCxt          (UINT32         nDev);
PUBLIC BmlShCxt    *_GetShCxt           (VOID);
PUBLIC VOID         _StoreVolCxt        (UINT32         nVol,
                                         BmlVolCxt     *pstVol);
PUBLIC FSRLowFuncTbl *_GetLFT           (UINT32         nVol);
PUBLIC INT32        _InitBIF            (FsrVolParm     stPAM[FSR_MAX_VOLS],
                                         UINT32         nFlag);
PUBLIC BOOL32       _IsOpenedDev        (UINT32         nPDev);
PUBLIC BOOL32       _ChkLLDSpecValidity (BmlVolCxt     *pstVol,
                                         FSRDevSpec    *pstLLDSpec);
PUBLIC BOOL32       _InitLFT            (UINT32         nVol);
PUBLIC INT32        _Open               (UINT32         nVol);
PUBLIC INT32        _Close              (UINT32         nVol);
PUBLIC INT32        _MountRsvr          (UINT32         nVol,
                                         FSRPartI      *pstLoadingPI,
                                         FSRPIExt      *pstLoadingPExt);
PUBLIC UINT32       _GetShfValue        (UINT32         nValue);
PUBLIC VOID         _GetPBN             (UINT32         nSbn,
                                         BmlVolCxt     *pstVol,
                                         BmlDieCxt     *pstDie);
PUBLIC BOOL32       _GetPartID          (BmlVolCxt     *pstVol,
                                         UINT32         nVun,
                                         UINT32        *pPartID);
PUBLIC BOOL32       _StorePrevPIRet     (BmlVolCxt     *pstVol,
                                         BmlDieCxt     *pstDie,
                                         INT32          nBMLRe,
                                         UINT32         nCurPartID);
PUBLIC INT32        _GetPIRet           (BmlVolCxt     *pstVol,
                                         BmlDieCxt     *pstDie,
                                         UINT32         nVun,
                                         INT32          nBMLRe);
PUBLIC INT32        _SetBlkState        (UINT32         nVol,
                                         UINT32         nVun,
                                         UINT32         nNumOfUnits,
                                         UINT32         nCode);

PUBLIC INT32        _HandlePrevError    (UINT32         nVol,
                                         UINT32         nPDev,
                                         UINT32         nDieIdx,
                                         INT32          nLLDRe);
PUBLIC INT32        _HandlePrevWrErr    (UINT32         nVol,
                                         UINT32         nPDev,
                                         UINT32         nDieIdx,
                                         INT32          nMinorErr,
                                         BOOL32         bOn);
PUBLIC INT32        _HandlePrevErErr    (UINT32         nVol,
                                         UINT32         nPDev,
                                         UINT32         nDieIdx,
                                         INT32          nMinorErr);
PUBLIC INT32        _HandleLockErr      (BmlVolCxt     *pstVol,
                                         UINT32         nPDev,
                                         UINT32         nDieIdx,
                                         UINT32         nErrPbn);
PUBLIC INT32        _GetOTPInfo         (UINT32         nVol,
                                         UINT32        *pnLockstat);
PUBLIC VOID         _PrintPartI         (UINT32         nVol,
                                         FSRPartI      *pstPartI);


PUBLIC INT32        _ProcessEraseRefresh(UINT32         nVol,
                                         UINT32         nFlag);

PUBLIC VOID    _FreeLocalMem    (UINT32         nVol,
                                 FsrVolParm    *pstPAM);
PUBLIC INT32   _AllocSharedMem  (UINT32         nVol,
                                 FsrVolParm    *pstPAM);
PUBLIC INT32   _AllocMem        (UINT32         nVol,
                                 FsrVolParm     stPAM[FSR_MAX_VOLS],
                                 UINT32         nMemType);
PUBLIC UINT32  _GetLockLayer    (UINT32         nVol);
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _FSR_BML_BIFCOMMON_H_ */
