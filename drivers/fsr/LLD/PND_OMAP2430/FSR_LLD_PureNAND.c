/**
 *   @mainpage   Flex Sector Remapper : LinuStoreIII_1.2.0_b035-FSR_1.2.1p1_b129_RC
 *
 *   @section Intro
 *       Flash Translation Layer for Flex-OneNAND and OneNAND
 *
 *    @section  Copyright
 *            COPYRIGHT. 2008-2009 SAMSUNG ELECTRONICS CO., LTD.
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
 * @file      FSR_LLD_PureNAND.c
 * @brief     This file implements Low Level Driver for PureNAND
 * @author    Jinho Yi, Jinhyuck Kim
 * @date      18-AUG-2009
 * @remark
 * REVISION HISTORY
 * @n  20-OCT-2008 [NamOh Hwang] : First writing
 * @n  07-JUL-2009 [JinHo Yi]    : Modify for PureNAND support
 * @n  24-AUG-2009 [JinHyuck Kim]: Modify for tiny support and statistics
 *
 */

/******************************************************************************/
/* Header file inclusions                                                     */
/******************************************************************************/
#define     FSR_NO_INCLUDE_BML_HEADER
#define     FSR_NO_INCLUDE_STL_HEADER

#include    <FSR.h>
#include    "FSR_LLD_PureNAND.h"

#include    "FSR_LLD_OMAP2430.h"

/******************************************************************************/
/*   Local Configurations                                                     */
/*                                                                            */
/* - FSR_LLD_STRICT_CHK        : To check parameters strictly                 */
/* - FSR_LLD_STATISTICS        : To accumulate statistics.                    */
/* - FSR_LLD_LOGGING_HISTORY   : To log history                               */
/******************************************************************************/
#define     FSR_LLD_STRICT_CHK
//#define     FSR_LLD_STATISTICS
#define     FSR_LLD_LOGGING_HISTORY

/* Controller setting */
//#define     FSR_LLD_ENABLE_CNTRLLER_INT
//#define     FSR_LLD_ENABLE_CNTRLLER_ECC

/******************************************************************************/
/* Local #defines                                                             */
/******************************************************************************/
#define     FSR_PND_MAX_DEVS                    (FSR_MAX_DEVS)

#define     FSR_PND_SUPPORTED_DIES              (1)

#define     FSR_PND_MAX_BADMARK                 (4)  /* the size of gnBadMarkValue*/
#define     FSR_PND_MAX_BBMMETA                 (2)  /* the size of gnBBMMetaValue*/

/* MACROs below are used for Deferred Check Operation                         */
#define     FSR_PND_PREOP_NONE                  (0x0000)
#define     FSR_PND_PREOP_READ                  (0x0001)
#define     FSR_PND_PREOP_CPBK_LOAD             (0x0002)
#define     FSR_PND_PREOP_CACHE_PGM             (0x0003)
#define     FSR_PND_PREOP_PROGRAM               (0x0004)
#define     FSR_PND_PREOP_CPBK_PGM              (0x0005)
#define     FSR_PND_PREOP_ERASE                 (0x0006)
#define     FSR_PND_PREOP_IOCTL                 (0x0007)
#define     FSR_PND_PREOP_HOTRESET              (0x0008)
#define     FSR_PND_PREOP_ADDRESS_NONE          (0xFFFF)
#define     FSR_PND_PREOP_FLAG_NONE             (FSR_LLD_FLAG_NONE)

#define     FSR_PND_BUSYWAIT_TIME_LIMIT         (0x100000)

#define     FSR_PND_FLUSHOP_CALLER_BASEBIT      (16)
#define     FSR_PND_FLUSHOP_CALLER_MASK         (0xFFFF0000)

#define     FSR_PND_BLOCK_SIZE_128KB            (1)
#define     FSR_PND_BLOCK_SIZE_256KB            (2)

#define     FSR_PND_VALID_BLK_MARK              (0xFFFF) /* x16 */

#define     FSR_PND_ECC_READ_DISTURBANCE        (2)

#define     FSR_PND_SPARE_ECC_AREA              (5)	/* x16 */

#define 	FSR_PND_X8_BUS_WIDTH                (8)
#define     FSR_PND_X16_BUS_WIDTH               (16)

/* MACROs below are used for the statistic                                   */
#define     FSR_PND_STAT_SLC_PGM                (0x00000001)
#define     FSR_PND_STAT_LSB_PGM                (0x00000002)
#define     FSR_PND_STAT_MSB_PGM                (0x00000003)
#define     FSR_PND_STAT_ERASE                  (0x00000004)
#define     FSR_PND_STAT_SLC_LOAD               (0x00000005)
#define     FSR_PND_STAT_MLC_LOAD               (0x00000006)
#define     FSR_PND_STAT_RD_TRANS               (0x00000007)
#define     FSR_PND_STAT_WR_TRANS               (0x00000008)
#define     FSR_PND_STAT_WR_CACHEBUSY           (0x00000009)
#define     FSR_PND_STAT_FLUSH                  (0x0000000A)

/* Command type for statistics */
#define     FSR_PND_STAT_NORMAL_CMD             (0x0)
#define     FSR_PND_STAT_PLOAD                  (0x1)
#define     FSR_PND_STAT_CACHE_PGM              (0x2)

#define     FSR_PND_CACHE_BUSY_TIME             (25) /* usec */
#define     FSR_PND_RD_SW_OH                    (5)  /* usec */
#define     FSR_PND_WR_SW_OH                    (10) /* usec */

/* In sync burst mode, reading from DataRAM has overhead */
#define     FSR_PND_RD_TRANS_SW_OH              (14) /* usec */


/* For General PureNAND LLD */
#define     FSR_PND_4CYCLES                     (4)
#define     FSR_PND_5CYCLES                     (5)
#define     INVALID_ADDRESS                     (0xFFFFFFFF)


/* Command code for NAND Controller */
#define     FSR_PND_SET_READ_PAGE               (0x0)
#define     FSR_PND_SET_PROGRAM_PAGE            (0x1)
#define     FSR_PND_SET_ERASE                   (0x2)
#define     FSR_PND_SET_RANDOM_DATA_INPUT       (0x3)
#define     FSR_PND_SET_RANDOM_DATA_OUTPUT      (0x4)
#define     FSR_PND_SET_RESET                   (0x5)
#define     FSR_PND_SET_READ_ID                 (0x6)
#define     FSR_PND_SET_READ_PGM_STATUS         (0x7)
#define     FSR_PND_SET_READ_ERASE_STATUS       (0x8)
#define     FSR_PND_SET_READ_INTERRUPT_STATUS   (0x9)
#define     FSR_PND_SET_ECC_ON                  (0xA)
#define     FSR_PND_SET_ECC_OFF                 (0xB)

#define     FSR_PND_BUSY                        (0x0)
#define     FSR_PND_READY                       (0x1)

#define     FSR_PND_OP_READ                     (0x0)
#define     FSR_PND_OP_PGM                      (0x1)
#define     FSR_PND_OP_ERASE                    (0x2)

#if defined(_FSR_LLD_OMAP2430_34xx_)
/* Controller command set */
#define OMAP2430_CMD_READ                            (0x00)
#define OMAP2430_CMD_READ_CONFIRM                    (0x30)
#define OMAP2430_CMD_READ_FOR_COPY_BACK              (0x35)
#define OMAP2430_CMD_READ_ID                         (0x90)
#define OMAP2430_CMD_RESET                           (0xFF)
#define OMAP2430_CMD_PAGE_PROGRAM                    (0x80)
#define OMAP2430_CMD_PAGE_PROGRAM_CONFIRM            (0x10)
#define OMAP2430_CMD_TWOPLANE_PAGE_PROGRAM           (0x81)
#define OMAP2430_CMD_TWOPLANE_PAGE_PROGRAM_CONFIRM   (0x11)
#define OMAP2430_CMD_BLOCK_ERASE                     (0x60)
#define OMAP2430_CMD_BLOCK_ERASE_CONFIRM             (0xD0)
#define OMAP2430_CMD_RANDOM_DATA_INPUT               (0x85)
#define OMAP2430_CMD_RANDOM_DATA_OUTPUT              (0x05)
#define OMAP2430_CMD_RANDOM_DATA_OUTPUT_CONFIRM      (0xE0)
#define OMAP2430_CMD_READ_STATUS                     (0x70)

/* Status Register Definition */
#define FSR_PND_STATUS_PGM_FAIL                      (0x01)
#define FSR_PND_STATUS_ERASE_FAIL                    (0x01)
#define FSR_PND_STATUS_READY                         (0x40)
#endif

#if !defined(FSR_LLD_ENABLE_CNTRLLER_ECC)
/* SW ECC for OMAP2430 */
#define OMAP2430_ECC_START_ADDRESS		             (6)

#define FSR_PND_ENABLE_ECC                           (0x0)
#define FSR_PND_DISABLE_ECC                          (0x1)

#define SWECC_E_ERROR                                (1)
#define SWECC_N_ERROR                                (0)
#define SWECC_C_ERROR                                (-1)
#define SWECC_U_ERROR                                (-2)
#define SWECC_R_ERROR                                (-3)
#endif


/******************************************************************************/
/* Local typedefs                                                             */
/******************************************************************************/
typedef struct
{
    UINT16          nMID;
    UINT16          nDID;
    
    UINT16          nNumOfBlks;
    
    UINT16          nNumOfDies;
    UINT16          nNumOfPlanes;
    
    UINT16          nSctsPerPg;
    UINT16          nSparePerSct;
    
    UINT16          nPgsPerBlk;
    
    UINT16          nRsvBlksInDev;
    
    UINT8           nBWidth;        /* Nand Organization X8 or X16   */
    UINT8           n5CycleDev;	    /* If 5 cycle device, this is 1
	                                   otherwise, this is 0          */
	                                   
    /* 
     * TrTime, TwTime of MLC are array of size 2
     * First  element is for LSB TLoadTime, TProgTime
     * SecPND element is for MLB TLoadTime, TProgTime
     * Use macro FSR_LLD_IDX_LSB_TIME, FSR_LLD_IDX_MSB_TIME
     */
    UINT32          nSLCTLoadTime;      /**< Typical Load     operation time  */    
    UINT32          nSLCTProgTime;      /**< Typical Program  operation time  */    
    UINT32          nTEraseTime;        /**< Typical Erase    operation time  */
} PureNANDSpec;

typedef struct
{
    UINT32          nShMemUseCnt;   
    
    /**< Previous operation data structure which can be shared among process  */
    UINT16          nPreOp[FSR_MAX_DIES];
    UINT16          nPreOpPbn[FSR_MAX_DIES];
    UINT16          nPreOpPgOffset[FSR_MAX_DIES];
    UINT32          nPreOpFlag[FSR_MAX_DIES];
    
    /**< Pseudo DataRAM. FIXME: The size is 4KB, It is difficult to allocate shared memory  */
    UINT8           pPseudoMainDataRAM[FSR_SECTOR_SIZE * FSR_MAX_PHY_SCTS];
    UINT8           pPseudoSpareDataRAM[FSR_SPARE_SIZE * FSR_MAX_PHY_SCTS];    

} PureNANDShMem;

typedef struct
{
    UINT32          nBaseAddr;
    BOOL32          bOpen;

    PureNANDSpec   *pstPNDSpec;
    
    UINT8           nSftPgsPerBlk;
    UINT8           nMskPgsPerBlk;    
    UINT16          nDDPSelBaseBit;

    UINT16          nFlushOpCaller;
    
    UINT16          nNumOfDies;

#if !defined(FSR_LLD_ENABLE_CNTRLLER_ECC)
    UINT32          nECCOption;    
#endif

    UINT32          nWrTranferTime;    /**< Write transfer time              */
    UINT32          nRdTranferTime;    /**< Read transfer time               */

    UINT32          nIntID;
} PureNANDCxt;

/**
 * @brief Data structure of OneNAND statistics
 */
typedef struct
{
    UINT32          nNumOfSLCLoads; /**< The number of times of Load  operations */
    UINT32          nNumOfMLCLoads;
    
    UINT32          nNumOfSLCPgms;  /**< The number of times of SLC programs     */    
    UINT32          nNumOfLSBPgms;  /**< The number of times of LSB programs     */
    UINT32          nNumOfMSBPgms;  /**< The number of times of MSB programs     */
    
    UINT32          nNumOfCacheBusy;/**< The number of times of Cache Busy       */
    UINT32          nNumOfErases;   /**< The number of times of Erase operations */

    UINT32          nNumOfRdTrans;  /**< The number of times of Read  Transfer   */
    UINT32          nNumOfWrTrans;  /**< The number of times of Write Transfer   */

    UINT32          nPreCmdOption[FSR_MAX_DIES]; /** Previous command option     */

    INT32           nIntLowTime[FSR_MAX_DIES];
                                    /**< MDP : 0 
                                         DDP : Previous operation time           */

    UINT32          nRdTransInBytes;/**< The number of bytes transfered in read  */
    UINT32          nWrTransInBytes;/**< The number of bytes transfered in write */
} PureNANDStat;

typedef struct
{
    UINT8           nMID;
    UINT8           nDID;
    UINT8           n3rdIDData;
    UINT8           n4thIDData;
    UINT8           n5thIDData;
    UINT8           nPad0;
    UINT16          nPad1;
} NANDIDData;

/******************************************************************************/
/* Global variable definitions                                                */
/******************************************************************************/
PRIVATE const UINT16    gnBBMMetaValue[FSR_PND_MAX_BBMMETA]   =
                            { 0xFFFF,
                              FSR_LLD_BBM_META_MARK
                            };

PRIVATE const UINT16    gnBadMarkValue[FSR_PND_MAX_BADMARK]   =
                            { 0xFFFF,         /* FSR_LLD_FLAG_WR_NOBADMARK    */
                              0x2222,         /* FSR_LLD_FLAG_WR_EBADMARK     */
                              0x4444,         /* FSR_LLD_FLAG_WR_WBADMARK     */
                              0x8888,         /* FSR_LLD_FLAG_WR_LBADMARK     */
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

PRIVATE PureNANDCxt    *gpstPNDCxt  [FSR_PND_MAX_DEVS];

#if defined(WITH_TINY_FSR)
extern  PureNANDShMem  *gpstPNDShMem[FSR_PND_MAX_DEVS];
#else
        PureNANDShMem  *gpstPNDShMem[FSR_PND_MAX_DEVS];
#endif

#if defined (FSR_LLD_STATISTICS)
PRIVATE PureNANDStat   *gpstPNDStat[FSR_PND_MAX_DEVS];
PRIVATE UINT32          gnElapsedTime;
PRIVATE UINT32          gnDevsInVol[FSR_MAX_VOLS] = {0, 0};
#endif /* #if defined (FSR_LLD_STATISTICS) */

#if defined(FSR_LLD_LOGGING_HISTORY)
volatile PureNANDOpLog gstPNDOpLog[FSR_MAX_DEVS] =
{
    {0, {0,}, {0,}, {0,},{0,}},
    {0, {0,}, {0,}, {0,},{0,}},
    {0, {0,}, {0,}, {0,},{0,}},
    {0, {0,}, {0,}, {0,},{0,}}
};
#endif



/******************************************************************************/
/* Local constant definitions                                                 */
/******************************************************************************/
PRIVATE const PureNANDSpec gastPNDSpec[] = {
/******************************************************************************/
/*    nMID,                                                                   */
/*        nDID,                                                               */
/*               nNumOfBlks                                                   */
/*                   nNumOfDies                                               */
/*                      nNumOfPlanes                                          */
/*                         nSctsPerPg                                         */
/*                            nSparePerSct                                    */
/*                               nPgsPerBlk                                   */
/*                                  nRsvBlksInDev                             */
/*                                     nBWidth                                */
/*                                        n5CycleDev                          */
/*                                           nSLCTLoadTime                    */
/*                                              nSLCTProgTime                 */
/*                                                  nTEraseTime               */
/******************************************************************************/
    /* KF92G16Q2X */ /* KF92G16Q2W */
    { 0xEC, 0xBA,   2048, 1, 1, 4, 16, 64, 40, 16, FSR_PND_5CYCLES, 40, 250, 2000},

    /* KF94G16Q2W */
    { 0xEC, 0xBC,   4096, 1, 1, 4, 16, 64, 80, 16, FSR_PND_5CYCLES, 40, 250, 2000},
        
    { 0x00, 0x00,      0, 0, 0, 0,  0,  0,  0,  0,               0,  0,  0,     0}
};
/******************************************************************************/
/* Local function prototypes                                                  */
/******************************************************************************/
PRIVATE VOID    _ReadSpare          (FSRSpareBuf   *pstDest, 
                                     UINT8         *pSrc8);

PRIVATE VOID    _WriteSpare         (PureNANDSpec  *pstPNDSpec,
                                     UINT8		   *pDest8,
                                     FSRSpareBuf   *pstSrc,
                                     UINT32         nFlag);                             
                             
PRIVATE INT32   _StrictChk          (UINT32         nDev,
                                     UINT32         nPbn,
                                     UINT32         nPgOffset);

PRIVATE INT32   _SetDeviceInfo      (UINT32         nDev);


PRIVATE INT32   _ReadPageBuffer     (UINT32         nDev,
                                     UINT32         nDie);

PRIVATE VOID    _WriteToPageBuffer  (UINT32         nDev,
                                     UINT32         nDie);

PRIVATE VOID	_SetNANDCtrllerCmd  (UINT32         nDev,
                                     UINT32         nDie,
                            		 UINT32         nRowAddress,
                            		 UINT32         nColAddress,
                            		 UINT32         nCommand);

PRIVATE INT32   _WaitPNDIntStat     (UINT32         nDev,
                                     UINT32         nDie,
                                     UINT32         nOp);
                                     
PRIVATE INT32   _GetOpStatus        (UINT32         nDev,
                                     UINT32         nDie, 
                                     UINT32         nOp);
                                     
PRIVATE VOID    _GetPNDID           (UINT32         nDev,
                                     NANDIDData    *pstNANDID);
                                     
PRIVATE VOID    _SetECC             (UINT32         nDev,
                                     UINT32         nDie,
                                     UINT32         nSetECC);


#if !defined(FSR_LLD_ENABLE_CNTRLLER_ECC)
PRIVATE VOID    _GenerateECC        (UINT32 nDev,
                                     UINT32 nDie);

PRIVATE INT32   _CorrectECCErr      (UINT32 nDev,
                                     UINT32 nDie);
                                     
PRIVATE VOID    _ECC_GenM           (UINT8         *pEcc,
                                     UINT32        *pBuf);
                                     
PRIVATE VOID    _ECC_GenS           (volatile   UINT16  *pEcc, 
                                                UINT8   *pBuf);
                                     
PRIVATE INT32   _ECC_CompM          (UINT8         *pEcc2,
                                     UINT8         *pBuf,
                                     UINT32         nSectNum);
                                     
PUBLIC INT32    _ECC_CompS          (UINT8         *pEcc2, 
                                     UINT8         *pBuf,
                                     UINT32         nSectNum);
#endif

#if defined (FSR_LLD_LOGGING_HISTORY)
PRIVATE VOID    _AddLog             (UINT32         nDev,
                                     UINT32         nDie);
#endif /* #if defined (FSR_LLD_LOGGING_HISTORY) */

PRIVATE VOID    _DumpCmdLog         (VOID);

PRIVATE VOID    _DumpSpareBuffer    (UINT32         nDev);

PRIVATE VOID    _DumpMainBuffer     (UINT32         nDev);

#if defined (FSR_LLD_STATISTICS)
PRIVATE VOID    _AddPNDStat         (UINT32       nDev,
                                     UINT32       nDie,
                                     UINT32       nType,
                                     UINT32       nBytes,
                                     UINT32       nCmdOption);
#endif /* #if defined (FSR_LLD_STATISTICS) */


/******************************************************************************/
/* Code Implementation                                                        */
/******************************************************************************/

/**
 * @brief           This function reads data from spare area of Pseudo DataRAM
 *
 * @param[out]      pstDest : pointer to the host buffer
 * @param[in]       pSrc    : pointer to the spare area of Pseudo DataRAM
 *
 * @return          none
 *
 * @author          Jinho Yi
 * @version         1.2.1
 * @remark          This function reads from spare area of 1 page.
 *
 */
PRIVATE VOID  _ReadSpare(FSRSpareBuf *pstDest, 
                         UINT8       *pSrc8)
{
    UINT16 *pSrc16;

    FSR_STACK_VAR;

    FSR_STACK_END;	

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[PND:IN ] ++%s(pstDest: 0x%08x, pSrc8: 0x%08x)\r\n"),
        __FSR_FUNC__, pstDest, pSrc8));

    FSR_ASSERT(((UINT32) pstDest & 0x01) == 0);
    
    pSrc16 = (UINT16 *)pSrc8;

    pstDest->pstSpareBufBase->nBadMark       = *pSrc16++;
    pstDest->pstSpareBufBase->nBMLMetaBase0  = *pSrc16++;
    
	pstDest->pstSpareBufBase->nSTLMetaBase0  = (*pSrc16++ << 16);	
	pSrc16 += FSR_PND_SPARE_ECC_AREA;	
	pstDest->pstSpareBufBase->nSTLMetaBase0 |= *pSrc16++;

    pstDest->pstSpareBufBase->nSTLMetaBase1  = (*pSrc16++ << 16);
	pstDest->pstSpareBufBase->nSTLMetaBase1 |= *pSrc16++;
	
    pSrc16 += FSR_PND_SPARE_ECC_AREA;
    
    pstDest->pstSpareBufBase->nSTLMetaBase2  = (*pSrc16++ << 16);
	pstDest->pstSpareBufBase->nSTLMetaBase2 |= *pSrc16++;
	
	if(pstDest->nNumOfMetaExt > 0)
	{
		FSR_ASSERT(pstDest->nNumOfMetaExt == 1);
		
		/* Initialize meta extention #0 */
		FSR_OAM_MEMSET(&pstDest->pstSTLMetaExt[0], 0xFF, sizeof(FSRSpareBufExt));
		
		pstDest->pstSTLMetaExt->nSTLMetaExt0     = (*pSrc16++ << 16);
	    pSrc16 += FSR_PND_SPARE_ECC_AREA;	    		
	    pstDest->pstSTLMetaExt->nSTLMetaExt0    |= *pSrc16++;

		pstDest->pstSTLMetaExt->nSTLMetaExt1     = (*pSrc16++ << 16);		
		pstDest->pstSTLMetaExt->nSTLMetaExt1    |= *pSrc16++;		
	}
		
    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[PND:OUT] --%s()\r\n"), __FSR_FUNC__));
}



/**
 * @brief           This function writes FSRSpareBuf into spare area of Pseudo DataRAM
 *
 * @param[in]       pstPNDSpec  : pointer to PND Spec structure
 * @param[in]       pDest8      : pointer to the spare area of Pseudo DataRAM
 * @param[in]       pstSrc      : pointer to the structure, FSRSpareBuf
 * @param[in]       nFlag       : whether to write spare buffer onto DataRAM or not.
 *
 * @return          none
 *
 * @author          Jinho Yi
 * @version         1.2.1
 * @remark          This function writes FSRSpareBuf over spare area of 1 page
 *
 */
PRIVATE VOID
_WriteSpare(PureNANDSpec   *pstPNDSpec,
            UINT8		   *pDest8,
            FSRSpareBuf    *pstSrc,
            UINT32          nFlag)
{    
    UINT16 			*pDest16;  

    FSR_STACK_VAR;

    FSR_STACK_END;	

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[PND:IN ] ++%s(pDest: 0x%08x, pstSrc: 0x%08x)\r\n"),
        __FSR_FUNC__, pDest, pstSrc));        
    
    FSR_OAM_MEMSET((VOID *) pDest8, 0xFF, pstPNDSpec->nSctsPerPg * FSR_SPARE_SIZE);

    if (nFlag & FSR_LLD_FLAG_USE_SPAREBUF)
    {   
        pDest16 = (UINT16 *)pDest8;
        
		*pDest16++ = pstSrc->pstSpareBufBase->nBadMark;  
        
        *pDest16++ = pstSrc->pstSpareBufBase->nBMLMetaBase0;

        *pDest16++ = (UINT16) (pstSrc->pstSpareBufBase->nSTLMetaBase0 >> 16);        
         pDest16  += FSR_PND_SPARE_ECC_AREA;
        *pDest16++ = (UINT16) (pstSrc->pstSpareBufBase->nSTLMetaBase0 & 0xFFFF);   
     
        *pDest16++ = (UINT16) (pstSrc->pstSpareBufBase->nSTLMetaBase1 >> 16);
        *pDest16++ = (UINT16) (pstSrc->pstSpareBufBase->nSTLMetaBase1 & 0xFFFF);  
        
         pDest16  += FSR_PND_SPARE_ECC_AREA;

        *pDest16++ = (UINT16) (pstSrc->pstSpareBufBase->nSTLMetaBase2 >> 16);
        *pDest16++ = (UINT16) (pstSrc->pstSpareBufBase->nSTLMetaBase2 & 0xFFFF);

		if(pstSrc->nNumOfMetaExt > 0)
		{
			FSR_ASSERT(pstSrc->nNumOfMetaExt == 1);
			
	        *pDest16++ = (UINT16) (pstSrc->pstSTLMetaExt->nSTLMetaExt0 >> 16);	        
	         pDest16  += FSR_PND_SPARE_ECC_AREA;
	        *pDest16++ = (UINT16) (pstSrc->pstSTLMetaExt->nSTLMetaExt0 & 0xFFFF);     

	        *pDest16++ = (UINT16) (pstSrc->pstSTLMetaExt->nSTLMetaExt1 >> 16);
	        *pDest16++ = (UINT16) (pstSrc->pstSTLMetaExt->nSTLMetaExt1 & 0xFFFF);
		}
    }
    
    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[PND:OUT] --%s()\r\n"), __FSR_FUNC__));
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
 * @author          Jinhyuck Kim
 * @version         1.2.1
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
    PureNANDCxt    *pstPNDCxt;
    PureNANDSpec   *pstPNDSpec;
    INT32           nLLDRe      = FSR_LLD_SUCCESS;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[FND:OUT] ++%s(nDev:%d, nPbn:%d, nPgOffset:%d)\r\n"),
        __FSR_FUNC__, nDev, nPbn, nPgOffset));

    do
    {
        /* Check Device Number */
        if (nDev >= FSR_PND_MAX_DEVS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[PND:ERR]   %s() / %d line\r\n"),
                __FSR_FUNC__, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Invalid Device Number (nDev = %d)\r\n"),
                nDev));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }

        pstPNDCxt  = gpstPNDCxt[nDev];
        pstPNDSpec = pstPNDCxt->pstPNDSpec;

        /* Check Device Open Flag */
        if (pstPNDCxt->bOpen == FALSE32)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[PND:ERR]   %s() / %d line\r\n"),
                __FSR_FUNC__, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Device is not opened\r\n")));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            pstPNDCxt->bOpen: 0x%08x\r\n"),
                pstPNDCxt->bOpen));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }

        /* Check Block Out of Bound */
        if (nPbn >= pstPNDSpec->nNumOfBlks)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[PND:ERR]   %s() / %d line\r\n"),
                __FSR_FUNC__, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Pbn:%d >= pstPNDSpec->nNumOfBlks:%d\r\n"),
                nPbn, pstPNDSpec->nNumOfBlks));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }
        
        /* 
         * Check nPgOffset Out of Bound
         * in case of SLC, pageOffset is lower than 64 
         */
        if (nPgOffset >= pstPNDSpec->nPgsPerBlk)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[PND:ERR]   %s() / %d line\r\n"),
                __FSR_FUNC__, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Pg #%d in Pbn (#%d) is invalid\r\n"),
                nPgOffset, nPbn));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            nPgsPerBlk = %d\r\n"),
                pstPNDSpec->nPgsPerBlk));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }
    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[PND:OUT] --%s()\r\n"), __FSR_FUNC__));

    return (nLLDRe);
}



/**
 * @brief           This function initializes PureNAND Device Driver.
 *
 * @param[in]       nFlag   : FSR_LLD_FLAG_NONE
 *
 * @return          FSR_LLD_SUCCESS
 * @return          FSR_LLD_ALREADY_INITIALIZED
 *
 * @author          Jinho Yi
 * @version         1.2.1
 * @remark          It initializes internal data structure
 *
 */
PUBLIC INT32
FSR_PND_Init(UINT32 nFlag)
{
            FsrVolParm       astPAParm[FSR_MAX_VOLS];
            FSRLowFuncTbl    astLFT[FSR_MAX_VOLS];
            FSRLowFuncTbl   *apstLFT[FSR_MAX_VOLS];
            PureNANDShMem   *pstPNDShMem;
    PRIVATE BOOL32           nInitFlg					= FALSE32;
            UINT32           nVol;
            UINT32           nPDev;
            UINT32           nIdx;
            UINT32           nDie;
            UINT32           nMemAllocType;
            UINT32           nMemoryChunkID;
            UINT32           nSharedMemoryUseCnt;
            INT32            nLLDRet   					= FSR_LLD_SUCCESS;
            INT32            nPAMRet   					= FSR_PAM_SUCCESS;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[PND:IN ] ++%s(nFlag:0x%08x)\r\n"),
        __FSR_FUNC__, nFlag));

    do
    {
        if (nInitFlg == TRUE32)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
                (TEXT("[PND:INF]   %s(nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
                (TEXT("            already initialized\r\n")));

            nLLDRet = FSR_LLD_ALREADY_INITIALIZED;
            break;
        }

        /* Local data structure */
        for (nPDev = 0; nPDev < FSR_PND_MAX_DEVS; nPDev++)
        {
            gpstPNDCxt[nPDev] = NULL;
        }

        nPAMRet  = FSR_PAM_GetPAParm(astPAParm);
        if (FSR_RETURN_MAJOR(nPAMRet) != FSR_PAM_SUCCESS)
        {
            nLLDRet = FSR_LLD_PAM_ACCESS_ERROR;
            break;
        }

        for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
        {
            FSR_OAM_MEMSET(&astLFT[nVol], 0x00, sizeof(FSRLowFuncTbl));
            apstLFT[nVol] = &astLFT[nVol];
        }

        nPAMRet = FSR_PAM_RegLFT(apstLFT);
        if (nPAMRet != FSR_PAM_SUCCESS)
        {
            nLLDRet = FSR_LLD_PAM_ACCESS_ERROR;
            break;
        }

        for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
        {
            /* 
             * Because BML calls LLD_Init() by volume,
             * FSR_PND_Init() only initializes shared memory of
             * corresponding volume
             */
            if ((apstLFT[nVol] == NULL) || apstLFT[nVol]->LLD_Init != FSR_PND_Init)
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


                pstPNDShMem = NULL;
                
#if !defined(WITH_TINY_FSR)
                gpstPNDShMem[nPDev] = (PureNANDShMem *) NULL;
#endif

                if(gpstPNDShMem[nPDev] == NULL)
                {
                    pstPNDShMem = (PureNANDShMem *) FSR_OAM_MallocExt(nMemoryChunkID,
                                                                      sizeof(PureNANDShMem),
                                                                      nMemAllocType);
                                                                      
                    if (pstPNDShMem == NULL)
                    {
                        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                            (TEXT("[PND:ERR]   %s(nFlag:0x%08x) / %d line\r\n"),
                            __FSR_FUNC__, nFlag, __LINE__));

                        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                            (TEXT("            pstPNDShMem is NULL\r\n")));

                        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                            (TEXT("            malloc failed!\r\n")));

                        nLLDRet = FSR_LLD_MALLOC_FAIL;
                        break;
                    }

                    if (((UINT32) pstPNDShMem & (0x03)) != 0)
                    {
                        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                            (TEXT("[PND:ERR]   %s(nFlag:0x%08x) / %d line\r\n"),
                            __FSR_FUNC__, nFlag, __LINE__));

                        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                            (TEXT("            pstPNDShMem is misaligned:0x%08x\r\n"),
                            pstPNDShMem));

                        nLLDRet = FSR_OAM_NOT_ALIGNED_MEMPTR;
                        break;
                    }

                    gpstPNDShMem[nPDev] = pstPNDShMem;

                    nSharedMemoryUseCnt = pstPNDShMem->nShMemUseCnt;
                    
                    /* PreOp init of single process */
                    if (astPAParm[nVol].bProcessorSynchronization == FALSE32)
                    {
                        /* Initialize shared memory used by LLD                       */
                        for (nDie = 0; nDie < FSR_MAX_DIES; nDie++)
                        {
                            pstPNDShMem->nPreOp[nDie]          = FSR_PND_PREOP_NONE;
                            pstPNDShMem->nPreOpPbn[nDie]       = FSR_PND_PREOP_ADDRESS_NONE;
                            pstPNDShMem->nPreOpPgOffset[nDie]  = FSR_PND_PREOP_ADDRESS_NONE;
                            pstPNDShMem->nPreOpFlag[nDie]      = FSR_PND_PREOP_FLAG_NONE;
                        }
                    }
                    /* PreOp init of dual process */
                    else
                    {
                        if ((nSharedMemoryUseCnt == 0) ||
                            (nSharedMemoryUseCnt == astPAParm[nVol].nSharedMemoryInitCycle))
                        {
                            pstPNDShMem->nShMemUseCnt = 0;
                            
                            /* Initialize shared memory used by LLD                       */
                            for (nDie = 0; nDie < FSR_MAX_DIES; nDie++)
                            {
                                pstPNDShMem->nPreOp[nDie]          = FSR_PND_PREOP_NONE;
                                pstPNDShMem->nPreOpPbn[nDie]       = FSR_PND_PREOP_ADDRESS_NONE;
                                pstPNDShMem->nPreOpPgOffset[nDie]  = FSR_PND_PREOP_ADDRESS_NONE;
                                pstPNDShMem->nPreOpFlag[nDie]      = FSR_PND_PREOP_FLAG_NONE;
                            }                            
                        }
                    }
                    
                    pstPNDShMem->nShMemUseCnt++;
                }
            } /* for (nIdx = 0; nIdx < astPAParm[nVol].nDevsInVol; nIdx++) */

            if (nLLDRet != FSR_LLD_SUCCESS)
            {
                break;
            }
        } /* for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++) */

        if (nLLDRet != FSR_LLD_SUCCESS)
        {
            break;
        }
        
        nInitFlg = TRUE32;

    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[PND:OUT] --%s() / nLLDRet : 0x%x\r\n"), __FSR_FUNC__, nLLDRet));

    return (nLLDRet);
}

/**
 * @brief           This function searches PureNAND device spec
 * @n               and initialize context structure.
 *
 * @param[in]       nDev    : Physical Device Number (0 ~ 3)  
 *
 * @return          FSR_LLD_SUCCESS
 * @return          FSR_LLD_OPEN_FAILURE
 * @return          FSR_LLD_PAM_ACCESS_ERROR 
 *
 * @author          Jinho Yi
 * @version         1.2.1
 * @remark
 *
 */
PRIVATE INT32
_SetDeviceInfo(UINT32 nDev)
{
    PureNANDSpec      *pstPNDSpec;
    PureNANDCxt       *pstPNDCxt;
    NANDIDData         stIDData;
    INT32              nLLDRet      = FSR_LLD_SUCCESS;    
    UINT32             nShift;
    UINT32             nCnt         = 0;
    UINT8              nMID; /* Manufacture ID */
    UINT8              nDID; /* Device ID      */
    
    FSR_STACK_VAR;

    FSR_STACK_END;
    
    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[PND:IN ] ++%s(nDev:%d)\r\n"),
                        __FSR_FUNC__, nDev));

    do
    {
	    pstPNDCxt = gpstPNDCxt[nDev];
        
        /* tWHR is min 60ns */
        _GetPNDID(nDev, &stIDData);        

        nMID = stIDData.nMID;
        nDID = stIDData.nDID;

        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
            (TEXT("            nDev=%d, nMID=0x%04x, nDID=0x%04x\r\n"),
            nDev, nMID, nDID));

        pstPNDCxt->pstPNDSpec = NULL;
        
        for (nCnt = 0; gastPNDSpec[nCnt].nMID != 0; nCnt++)
        {
            if ((nDID == gastPNDSpec[nCnt].nDID) &&
                (nMID == gastPNDSpec[nCnt].nMID))
            {
                pstPNDCxt->pstPNDSpec = (PureNANDSpec *) &gastPNDSpec[nCnt];
                break;
            }
            /* else continue */
        }

        if (pstPNDCxt->pstPNDSpec == NULL)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[PND:ERR]   %s(nDev:%d / %d line\r\n"),
                __FSR_FUNC__, nDev, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Unknown Device\r\n")));

            nLLDRet = FSR_LLD_OPEN_FAILURE;
            break;
        }

        pstPNDSpec = pstPNDCxt->pstPNDSpec;

        nShift = 0;
        while(pstPNDSpec->nPgsPerBlk != (1 << nShift) )
        {
            nShift++;
        }

        pstPNDCxt->nSftPgsPerBlk = nShift;
        pstPNDCxt->nMskPgsPerBlk = pstPNDSpec->nPgsPerBlk -1;

        /* 
         * When the device is DDP, BML does interleve between the chips
         * To prevent this, fix nNumOfDies as 1
         */
        pstPNDCxt->nNumOfDies = FSR_PND_SUPPORTED_DIES;
        
        nShift = 0;
        while ((pstPNDSpec->nNumOfBlks/pstPNDCxt->nNumOfDies) != (1 << nShift))
        {
            nShift++;
        }
        
        pstPNDCxt->nDDPSelBaseBit = nShift;
    } while(0);

    return (nLLDRet);
}



/**
 * @brief           This function opens PureNAND device driver 
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
 *
 * @author          Jinho Yi
 * @version         1.2.1
 * @remark
 *
 */
PUBLIC INT32
FSR_PND_Open(UINT32         nDev,
             VOID          *pParam,
             UINT32         nFlag)
{
            PureNANDCxt        *pstPNDCxt;                  
            FsrVolParm         *pstParm;
            UINT32              nIdx            = 0;
            UINT32              nMemoryChunkID;
            INT32               nLLDRet         = FSR_LLD_SUCCESS;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[PND:IN ] ++%s(nDev:%d,nFlag:0x%x)\r\n"),
        __FSR_FUNC__, nDev, nFlag));

    do
    {
    
#if defined (FSR_LLD_STRICT_CHK)
        /* Check device number */
        if (nDev >= FSR_PND_MAX_DEVS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[PND:ERR]   %s(nDev:%d, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Invalid Device Number (nDev = %d)\r\n"),
                nDev));

            nLLDRet = FSR_LLD_INVALID_PARAM;
            break;
        }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */
        
        if (pParam == NULL)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[PND:ERR]   %s(nDev:%d, pParam:0x%08x, nFlag:%d) / %d line\r\n"),
                __FSR_FUNC__, nDev, pParam, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            pParam is NULL\r\n")));

            nLLDRet = FSR_LLD_INVALID_PARAM;
            break;
        }

        pstParm     = (FsrVolParm *) pParam;
        pstPNDCxt   = gpstPNDCxt[nDev];

        if (pstPNDCxt == NULL)
        {
            nMemoryChunkID = nDev / (FSR_MAX_DEVS / FSR_MAX_VOLS);

            pstPNDCxt = (PureNANDCxt *) FSR_OAM_MallocExt(nMemoryChunkID,
                                                          sizeof(PureNANDCxt),
                                                          FSR_OAM_LOCAL_MEM);

            if (pstPNDCxt == NULL)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[PND:ERR]   %s(nDev:%d, pParam:0x%08x, nFlag:%d) / %d line\r\n"),
                    __FSR_FUNC__, nDev, pParam, nFlag, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            pstPNDCxt is NULL\r\n")));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            malloc failed!\r\n")));

                nLLDRet = FSR_LLD_MALLOC_FAIL;
                break;
            }

            if (((UINT32) pstPNDCxt & (0x03)) != 0)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[PND:ERR]   %s(nDev:%d, nFlag:0x%08x) / %d line\r\n"),
                    __FSR_FUNC__, nDev, nFlag, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            pstPNDCxt is misaligned:0x%08x\r\n"),
                    pstPNDCxt));

                nLLDRet = FSR_OAM_NOT_ALIGNED_MEMPTR;
                break;
            }

            gpstPNDCxt[nDev] = pstPNDCxt;

            FSR_OAM_MEMSET(pstPNDCxt, 0x00, sizeof(PureNANDCxt));

        } /* if (pstPNDCxt == NULL) */

#if defined(FSR_LLD_STATISTICS)
        gpstPNDStat[nDev] = (PureNANDStat *) FSR_OAM_Malloc(sizeof(PureNANDStat));

        if (gpstPNDStat[nDev] == NULL)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[PND:ERR]   %s(nDev:%d, pParam:0x%08x, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, pParam, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            gpstPNDStat[%d] is NULL\r\n"), nDev));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            malloc failed!\r\n")));

            nLLDRet = FSR_LLD_MALLOC_FAIL;
            break;
        }

        /* If the pointer has not 4-byte align address, return not-aligned pointer. */
        if (((UINT32) gpstPNDStat[nDev] & (sizeof(UINT32) - 1)) != 0)
        {
            FSR_DBZ_DBGMOUT(FSR_DBZ_ERROR, 
                               (TEXT("[PND:ERR]   gpstPNDStat[%d] is not aligned by 4-bytes\r\n"), nDev));
            nLLDRet = FSR_OAM_NOT_ALIGNED_MEMPTR;
            break;
        }

        FSR_OAM_MEMSET(gpstPNDStat[nDev], 0x00, sizeof(PureNANDStat));
#endif

        if (pstPNDCxt->bOpen == TRUE32)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
                (TEXT("[PND:INF]   %s(nDev:%d, pParam:0x%08x, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, pParam, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
                (TEXT("[PND:INF]   dev:%d is already open\r\n"), nDev));

            nLLDRet = FSR_LLD_ALREADY_OPEN;
            break;
        }

        /* Base address setting */
        nIdx = nDev & (FSR_MAX_DEVS/FSR_MAX_VOLS - 1);

        if (pstParm->nBaseAddr[nIdx] != FSR_PAM_NOT_MAPPED)
        {
            pstPNDCxt->nBaseAddr = pstParm->nBaseAddr[nIdx];

            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
                (TEXT("[PND:INF]   %s(nDev:%d, pParam:0x%08x, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, pParam, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
                (TEXT("            pstPNDCxt->nBaseAddr: 0x%08x\r\n"),
                pstPNDCxt->nBaseAddr));
        }
        else
        {
            pstPNDCxt->nBaseAddr = FSR_PAM_NOT_MAPPED;
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[PND:ERR]   %s(nDev:%d, pParam:0x%08x, nFlag:0x%08x / %d line\r\n"),
                __FSR_FUNC__, nDev, pParam, nFlag, __LINE__));
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            PureNAND is not mapped\r\n")));
            nLLDRet = FSR_LLD_INVALID_PARAM;
            break;
        }

        /* Set device info */
        nLLDRet = _SetDeviceInfo(nDev);
        if (FSR_RETURN_MAJOR(nLLDRet) != FSR_LLD_SUCCESS)
        {            
            break;
        }

        nIdx                    = nDev & (FSR_MAX_DEVS / FSR_MAX_VOLS -1);
        pstPNDCxt->nIntID       = pstParm->nIntID[nIdx];
        if ((pstPNDCxt->nIntID != FSR_INT_ID_NONE) && (pstPNDCxt->nIntID > FSR_INT_ID_NAND_7))
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[PND:INF]   nIntID is out of range(nIntID:%d)\r\n"), pstPNDCxt->nIntID));
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[PND:INF]   FSR_INT_ID_NAND_0(%d) <= nIntID <= FSR_INT_ID_NAND_7(%d)\r\n"),
                FSR_INT_ID_NAND_0, FSR_INT_ID_NAND_7));
            nLLDRet = FSR_LLD_INVALID_PARAM;            
            break;
        }

        FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF,
            (TEXT("[PND:INF]   pstPNDCxt->nIntID   :0x%04x / %d line\r\n"),
            pstPNDCxt->nIntID, __LINE__));
        
        pstPNDCxt->bOpen      = TRUE32;
        
#if defined (FSR_LLD_STATISTICS)
        gnDevsInVol[nDev / (FSR_MAX_DEVS / FSR_MAX_VOLS)]++;
#endif

    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[PND:OUT] --%s() / nLLDRet : 0x%x\r\n"), __FSR_FUNC__, nLLDRet));

    return (nLLDRet);
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
 * @author          Jinho Yi
 * @version         1.2.1
 * @remark
 *
 */
PUBLIC INT32
FSR_PND_Close(UINT32         nDev,
              UINT32         nFlag)
{
    PureNANDCxt      *pstPNDCxt;
    UINT32            nMemoryChunkID;
    INT32             nLLDRet           = FSR_LLD_SUCCESS;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[PND:IN ] ++%s(nDev:%d, nFlag:0x%08x)\r\n"),
        __FSR_FUNC__, nDev, nFlag));

    /* Note: Here LLD doesn't flush the previous operation, for BML flushes */
    
    do
    {

#if defined (FSR_LLD_STRICT_CHK)
        /* Check device number */
        if (nDev >= FSR_PND_MAX_DEVS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[PND:ERR]   %s(nDev:%d, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Invalid Device Number (nDev = %d)\r\n"),
                nDev));

            nLLDRet = FSR_LLD_INVALID_PARAM;
            break;
        }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */

        pstPNDCxt = gpstPNDCxt[nDev];

        if (pstPNDCxt != NULL)
        {
            pstPNDCxt->bOpen      = FALSE32;
            pstPNDCxt->pstPNDSpec = NULL;

            nMemoryChunkID = nDev / (FSR_MAX_DEVS / FSR_MAX_VOLS);

            FSR_OAM_FreeExt(nMemoryChunkID, pstPNDCxt, FSR_OAM_LOCAL_MEM);
            
            gpstPNDCxt[nDev] = NULL;
            
#if defined (FSR_LLD_STATISTICS)
            gnDevsInVol[nDev / (FSR_MAX_DEVS / FSR_MAX_VOLS)]--;
#endif

        }

#if defined(FSR_LLD_STATISTICS)
        if (gpstPNDStat[nDev] != NULL)
        {
            FSR_OAM_Free(gpstPNDStat[nDev]);
            gpstPNDStat[nDev] = NULL;
        }
#endif
        
    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[PND:OUT] --%s() / nLLDRet : 0x%x\r\n"), __FSR_FUNC__, nLLDRet));

    return (nLLDRet);
}



/**
 * @brief           This function reads 1 page from PureNAND
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
 * @return          FSR_LLD_PREV_2LV_READ_DISTURBANCE | {FSR_LLD_1STPLN_CURR_ERROR}
 * @n               previous read disturbance error return value
 *
 * @author          Jinho Yi
 * @version         1.2.1
 * @remark          One function call of FSR_PND_Read() loads 1 page from
 * @n               PureNAND and moves data from DataRAM to pMBuf, pSBuf.
 *
 */
PUBLIC INT32
FSR_PND_Read(UINT32         nDev,
             UINT32         nPbn,
             UINT32         nPgOffset,
             UINT8         *pMBuf,
             FSRSpareBuf   *pSBuf,
             UINT32         nFlag)
{
    INT32       nLLDRet = FSR_LLD_SUCCESS;
    UINT32      nLLDFlag;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[PND:IN ] ++%s()\r\n"), __FSR_FUNC__));

    do
    {
        nLLDFlag = ~FSR_LLD_FLAG_CMDIDX_MASK & nFlag;

        nLLDRet  = FSR_PND_ReadOptimal(nDev,
                                       nPbn, nPgOffset,
                                       pMBuf, pSBuf,
                                       FSR_LLD_FLAG_1X_LOAD | nLLDFlag);
        if (FSR_RETURN_MAJOR(nLLDRet) != FSR_LLD_SUCCESS)
        {
            break;
        }

        nLLDRet  = FSR_PND_ReadOptimal(nDev,
                                       nPbn, nPgOffset,
                                       pMBuf, pSBuf,
                                       FSR_LLD_FLAG_TRANSFER | nLLDFlag);
        if (FSR_RETURN_MAJOR(nLLDRet) != FSR_LLD_SUCCESS)
        {
            break;
        }
    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[PND:OUT] --%s() / nLLDRet : 0x%x\r\n"), __FSR_FUNC__, nLLDRet));

    return nLLDRet;
}



/**
 * @brief           This function reads 1 page from PureNAND
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
 * @return          FSR_LLD_PREV_2LV_READ_DISTURBANCE | {FSR_LLD_1STPLN_CURR_ERROR}
 * @n               previous read disturbance error return value
 *
 * @author          Jinho Yi
 * @version         1.2.1
 *
 */
PUBLIC INT32
FSR_PND_ReadOptimal(UINT32         nDev,
                    UINT32         nPbn,
                    UINT32         nPgOffset,
                    UINT8         *pMBufOrg,
                    FSRSpareBuf   *pSBufOrg,
                    UINT32         nFlag)
{
    PureNANDCxt      *pstPNDCxt;
    PureNANDSpec     *pstPNDSpec;
    PureNANDShMem    *pstPNDShMem;

    UINT32            nCmdIdx;

    /* Start sector offset from the start */
    UINT32            nStartOffset;

    /* End sector offset from the end     */
    UINT32            nEndOffset;
    UINT32            nDie;
    UINT32            nFlushOpCaller;
    UINT32            nRowAddress;
    UINT32            nColAddress      = 0;    
    INT32             nLLDRet          = FSR_LLD_SUCCESS;
    UINT8            *pMainDataRAM;
    UINT8            *pSpareDataRAM;
    
#if defined(FSR_LLD_STATISTICS)
    UINT32            nBytesTransferred = 0;
#endif

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
    (TEXT("[PND:IN ] ++%s(nDev:%d, nPbn:%d, nPgOffset:%d, pMBuf:0x%08x, pSBuf:0x%08x, nFlag:0x%08x)\r\n"),
    __FSR_FUNC__ , nDev, nPbn, nPgOffset, pMBufOrg, pSBufOrg, nFlag));

    do
    {

#if defined (FSR_LLD_STRICT_CHK)
        nLLDRet = _StrictChk(nDev, nPbn, nPgOffset);
        if (nLLDRet != FSR_LLD_SUCCESS)
        {
            break;
        }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */
    
        pstPNDCxt   = gpstPNDCxt[nDev];
        pstPNDSpec  = pstPNDCxt->pstPNDSpec;
        pstPNDShMem = gpstPNDShMem[nDev];
             
        nCmdIdx = (nFlag & FSR_LLD_FLAG_CMDIDX_MASK) >> FSR_LLD_FLAG_CMDIDX_BASEBIT;

        nDie    = nPbn >> pstPNDCxt->nDDPSelBaseBit;

        nFlushOpCaller = FSR_PND_PREOP_READ << FSR_PND_FLUSHOP_CALLER_BASEBIT;
        
        pMainDataRAM    = pstPNDShMem->pPseudoMainDataRAM;
        pSpareDataRAM   = pstPNDShMem->pPseudoSpareDataRAM;

        /* Transfer data */
        if (nFlag & FSR_LLD_FLAG_TRANSFER)
        {  
            if (pstPNDCxt->nNumOfDies == FSR_MAX_DIES)
            {
                FSR_PND_FlushOp(nDev,
                                nFlushOpCaller | ((nDie + 0x1) & 0x1),
                                nFlag | FSR_LLD_FLAG_REMAIN_PREOP_STAT);
            }

            nLLDRet = FSR_PND_FlushOp(nDev, nFlushOpCaller | nDie, nFlag);
           
            nStartOffset = (nFlag & FSR_LLD_FLAG_1ST_SCTOFFSET_MASK)
                            >> FSR_LLD_FLAG_1ST_SCTOFFSET_BASEBIT;

            nEndOffset   = (nFlag & FSR_LLD_FLAG_LAST_SCTOFFSET_MASK)
                            >> FSR_LLD_FLAG_LAST_SCTOFFSET_BASEBIT;

            if (pMBufOrg != NULL)
            {
                FSR_OAM_MEMCPY(pMBufOrg + nStartOffset * FSR_SECTOR_SIZE,
                               pMainDataRAM + nStartOffset * FSR_SECTOR_SIZE,
                               (pstPNDSpec->nSctsPerPg - nStartOffset - nEndOffset) * FSR_SECTOR_SIZE);
                               
#if defined (FSR_LLD_STATISTICS)
                nBytesTransferred +=  FSR_SECTOR_SIZE *
                    (pstPNDSpec->nSctsPerPg - nStartOffset - nEndOffset);
#endif /* #if defined (FSR_LLD_STATISTICS) */

            }        

            if ((pSBufOrg != NULL) && (nFlag & FSR_LLD_FLAG_USE_SPAREBUF))
            {
                if ((nFlag & FSR_LLD_FLAG_DUMP_MASK) == FSR_LLD_FLAG_DUMP_OFF)
                {
                    _ReadSpare(pSBufOrg,
                               pSpareDataRAM);
                }
                else
                {
                	FSR_OAM_MEMCPY(pSBufOrg + nStartOffset * FSR_SPARE_SIZE,
                                   pSpareDataRAM + nStartOffset * FSR_SPARE_SIZE,
                                  (pstPNDSpec->nSctsPerPg - nStartOffset - nEndOffset) * FSR_SPARE_SIZE);                    
                }
                
#if defined (FSR_LLD_STATISTICS)
                if ((nFlag & FSR_LLD_FLAG_DUMP_MASK) == FSR_LLD_FLAG_DUMP_OFF)
                {
                    nBytesTransferred += (FSR_SPARE_BUF_SIZE_2KB_PAGE);
                }
                else
                {
                    nBytesTransferred +=
                        pstPNDSpec->nSctsPerPg * pstPNDSpec->nSparePerSct;
                }
#endif /* #if defined (FSR_LLD_STATISTICS) */

            }
            
            pstPNDShMem->nPreOp[nDie] = FSR_PND_PREOP_NONE;
        } /* if (nFlag & FSR_LLD_FLAG_TRANSFER) */

        if ((nCmdIdx == FSR_LLD_FLAG_1X_LOAD) ||
            (nCmdIdx == FSR_LLD_FLAG_1X_PLOAD))
        {
            /* 
             * eNAND doesn't support read interleaving,
             * therefore, wait until interrupt status is ready for both die.
             * if device is DDP. 
             */
            if (pstPNDCxt->nNumOfDies == FSR_MAX_DIES)
            {
                FSR_PND_FlushOp(nDev,
                                nFlushOpCaller | ((nDie + 0x1) & 0x1),
                                nFlag | FSR_LLD_FLAG_REMAIN_PREOP_STAT);
            }
            
            FSR_PND_FlushOp(nDev, nFlushOpCaller | nDie, nFlag);

            nRowAddress  = nPbn << pstPNDCxt->nSftPgsPerBlk;
			nRowAddress |= (nPgOffset & pstPNDCxt->nMskPgsPerBlk);

            if ((nFlag & FSR_LLD_FLAG_ECC_MASK) == FSR_LLD_FLAG_ECC_ON)
            {
                _SetNANDCtrllerCmd(nDev, nDie, INVALID_ADDRESS, INVALID_ADDRESS, FSR_PND_SET_ECC_ON);
            }
            else
            {
                _SetNANDCtrllerCmd(nDev, nDie, INVALID_ADDRESS, INVALID_ADDRESS, FSR_PND_SET_ECC_OFF);
            }

            /* In case of non-blocking mode, interrupt should be enabled */
            if (nFlag & FSR_LLD_FLAG_INT_MASK)
            {
                FSR_OAM_ClrNEnableInt(pstPNDCxt->nIntID);
            }
            else
            {
                FSR_OAM_ClrNDisableInt(pstPNDCxt->nIntID);
            }
            
            _SetNANDCtrllerCmd(nDev, nDie, nRowAddress, nColAddress, FSR_PND_SET_READ_PAGE);

            pstPNDShMem->nPreOp[nDie] = FSR_PND_PREOP_READ;
            
#if defined (FSR_LLD_STATISTICS)
            _AddPNDStat(nDev, nDie, FSR_PND_STAT_RD_TRANS, nBytesTransferred, FSR_PND_STAT_PLOAD);
#endif /* #if defined (FSR_LLD_STATISTICS) */

        } /* if ((nCmdIdx == FSR_LLD_FLAG_1X_LOAD) | (nCmdIdx == FSR_LLD_FLAG_1X_PLOAD)) */

        pstPNDShMem->nPreOpPbn[nDie]      = (UINT16) nPbn;
        pstPNDShMem->nPreOpPgOffset[nDie] = (UINT16) nPgOffset;
        pstPNDShMem->nPreOpFlag[nDie]     = nFlag;
        
#if defined (FSR_LLD_LOGGING_HISTORY)
        _AddLog(nDev, nDie);
#endif

    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[PND:OUT] --%s() / nLLDRet : 0x%x\r\n"), __FSR_FUNC__, nLLDRet));

    return (nLLDRet);
}



/**
 * @brief           This function writes data into PureNAND
 *
 * @param[in]       nDev        : Physical Device Number (0 ~ 3)
 * @param[in]       nPbn        : Physical Block  Number
 * @param[in]       nPgOffset   : Page Offset within a block
 * @param[in]       pMBufOrg    : Memory buffer for main  array of NAND flash
 * @param[in]       pSBufOrg    : Memory buffer for spare array of NAND flash
 * @param[in]       nFlag       : Operation options such as ECC_ON, OFF
 *
 * @return          FSR_LLD_SUCCESS
 * @return          FSR_LLD_INVALID_PARAM
 * @return          FSR_LLD_PREV_WRITE_ERROR  | {FSR_LLD_1STPLN_CURR_ERROR}
 * @n               Error for normal program 
 *
 * @author          Jinho Yi
 * @version         1.2.1
 * @remark
 *
 */
PUBLIC INT32
FSR_PND_Write(UINT32       nDev,
              UINT32       nPbn,
              UINT32       nPgOffset,
              UINT8       *pMBufOrg,
              FSRSpareBuf *pSBufOrg,
              UINT32       nFlag)
{
    PureNANDCxt      *pstPNDCxt;
    PureNANDSpec     *pstPNDSpec;
    PureNANDShMem    *pstPNDShMem;
    UINT32            nBBMMetaIdx;
    UINT32            nBadMarkIdx;
    UINT32            nDie;
    UINT32            nFlushOpCaller;
    UINT32            nRowAddress;
    UINT32            nColAddress;
    UINT32            nMainSize;
    INT32             nLLDRet          = FSR_LLD_SUCCESS;
    UINT8            *pMainDataRAM;
    UINT8            *pSpareDataRAM;
    
#if defined(FSR_LLD_STATISTICS)
    UINT32            nBytesTransferred = 0;
#endif

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
    (TEXT("[PND:IN ] ++%s(nDev:%d, nPbn:%d, nPgOffset:%d, pMBufOrg: 0x%08x, pSBufOrg: 0x%08x, nFlag:0x%08x)\r\n"),
    __FSR_FUNC__, nDev, nPbn, nPgOffset, pMBufOrg, pSBufOrg, nFlag));

    do
    {

#if defined (FSR_LLD_STRICT_CHK)
        nLLDRet = _StrictChk(nDev, nPbn, nPgOffset);

        if (nLLDRet != FSR_LLD_SUCCESS)
        {
            break;
        }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */

        pstPNDCxt   = gpstPNDCxt[nDev];
        pstPNDSpec  = pstPNDCxt->pstPNDSpec;
        pstPNDShMem = gpstPNDShMem[nDev];

        nDie = nPbn >> pstPNDCxt->nDDPSelBaseBit;

        nFlushOpCaller = FSR_PND_PREOP_PROGRAM << FSR_PND_FLUSHOP_CALLER_BASEBIT;

        nLLDRet = FSR_PND_FlushOp(nDev, nFlushOpCaller | nDie, nFlag);
        if (nLLDRet != FSR_LLD_SUCCESS)
        {
            break;
        }

        nRowAddress  = nPbn << pstPNDCxt->nSftPgsPerBlk;
        nRowAddress |= (nPgOffset & pstPNDCxt->nMskPgsPerBlk);

        nMainSize = pstPNDSpec->nSctsPerPg * FSR_SECTOR_SIZE;
        
        nColAddress = 0;

        _SetNANDCtrllerCmd(nDev, nDie, nRowAddress, nColAddress, FSR_PND_SET_PROGRAM_PAGE);
            
        /* Default setting */
        pMainDataRAM    = pstPNDShMem->pPseudoMainDataRAM;
        pSpareDataRAM   = pstPNDShMem->pPseudoSpareDataRAM;

        /* Write Data into DataRAM */
        if (pMBufOrg != NULL)
        {            

#if defined (FSR_LLD_STATISTICS)
            nBytesTransferred += nMainSize;
#endif        

            FSR_OAM_MEMCPY(pMainDataRAM, pMBufOrg, nMainSize);
        }
        else if((pMBufOrg == NULL) && (pSBufOrg != NULL))
        {
            /* For bad handling */
            FSR_OAM_MEMSET(pMainDataRAM, 0xFF, nMainSize);
        }

        if (pSBufOrg != NULL)
        {
            if ((nFlag & FSR_LLD_FLAG_DUMP_MASK) == FSR_LLD_FLAG_DUMP_ON)
            {
                pSpareDataRAM = (UINT8 *) pSBufOrg;
            }
            else
            {
                /* 
                 * If FSR_LLD_FLAG_BBM_META_BLOCK of nFlag is set,
                 * write nBMLMetaBase0 of FSRSpareBufBase with 0xA5A5
                 */
                nBBMMetaIdx = (nFlag & FSR_LLD_FLAG_BBM_META_MASK) >>
                              FSR_LLD_FLAG_BBM_META_BASEBIT;
                              
                nBadMarkIdx = (nFlag & FSR_LLD_FLAG_BADMARK_MASK) >> 
                                    FSR_LLD_FLAG_BADMARK_BASEBIT;

                pSBufOrg->pstSpareBufBase->nBadMark      = gnBadMarkValue[nBadMarkIdx];
                pSBufOrg->pstSpareBufBase->nBMLMetaBase0 = gnBBMMetaValue[nBBMMetaIdx];

                /* 
                 * _WriteSpare does not care about bad mark which is written at
                 * the first word of sector 0 of the spare area 
                 * bad mark is written individually.
                 */                
                _WriteSpare(pstPNDSpec, pSpareDataRAM, pSBufOrg, nFlag);
            }
        }

        if (pMBufOrg != NULL && pSBufOrg == NULL)
        {
            if((nFlag & FSR_LLD_FLAG_BACKUP_DATA) != FSR_LLD_FLAG_BACKUP_DATA)
            {
            	FSR_OAM_MEMSET(pSpareDataRAM, 0xFF, pstPNDSpec->nSctsPerPg * FSR_SPARE_SIZE);
            }
        }
        
        if ((nFlag & FSR_LLD_FLAG_DUMP_MASK) == FSR_LLD_FLAG_DUMP_OFF)
        {
            /* 
             * Write bad mark of the block
             * bad mark is not written in _WriteSpare()
             */
            *((UINT16 *)pSpareDataRAM) = 
                gnBadMarkValue[(nFlag & FSR_LLD_FLAG_BADMARK_MASK) >> FSR_LLD_FLAG_BADMARK_BASEBIT];
        }

#if defined (FSR_LLD_STATISTICS)
        nBytesTransferred += (pstPNDSpec->nSctsPerPg * pstPNDSpec->nSparePerSct);
#endif /* #if defined (FSR_LLD_STATISTICS) */
        
        
        if ((nFlag & FSR_LLD_FLAG_ECC_MASK) == FSR_LLD_FLAG_ECC_ON)
        {
            _SetNANDCtrllerCmd(nDev, nDie, INVALID_ADDRESS, INVALID_ADDRESS, FSR_PND_SET_ECC_ON);
        }
        else
        {
            _SetNANDCtrllerCmd(nDev, nDie, INVALID_ADDRESS, INVALID_ADDRESS, FSR_PND_SET_ECC_OFF);
        }
                
        _WriteToPageBuffer(nDev, nDie);

#if defined (FSR_LLD_WAIT_ALLDIE_PGM_READY)
        /* 
         * In case of DDP, wait the other die ready.
         * When there is a cache program going on the other die,
         * setting the address register leads to the problem.
         * that is.. data under programming will be written to the newly set address.
         */
        if (pstPNDCxt->nNumOfDies == FSR_MAX_DIES)
        {
            FSR_PND_FlushOp(nDev,
                            nFlushOpCaller | ((nDie + 0x1) & 0x01),
                            nFlag | FSR_LLD_FLAG_REMAIN_PREOP_STAT);
        }
#endif

        /* In case of non-blocking mode, interrupt should be enabled */
        if (nFlag & FSR_LLD_FLAG_INT_MASK)
        {
            FSR_OAM_ClrNEnableInt(pstPNDCxt->nIntID);
        }
        else
        {
            FSR_OAM_ClrNDisableInt(pstPNDCxt->nIntID);
        }

        pstPNDShMem->nPreOp[nDie]         = FSR_PND_PREOP_PROGRAM;
        pstPNDShMem->nPreOpPbn[nDie]      = (UINT16) nPbn;
        pstPNDShMem->nPreOpPgOffset[nDie] = (UINT16) nPgOffset;
        pstPNDShMem->nPreOpFlag[nDie]     = nFlag;
        
#if defined (FSR_LLD_LOGGING_HISTORY)
        _AddLog(nDev, nDie);
#endif

#if defined (FSR_LLD_STATISTICS)
        _AddPNDStat(nDev, nDie, FSR_PND_STAT_WR_TRANS, nBytesTransferred, FSR_PND_STAT_CACHE_PGM);
        _AddPNDStat(nDev, nDie, FSR_PND_STAT_SLC_PGM, 0, FSR_PND_STAT_CACHE_PGM);
#endif /* #if defined (FSR_LLD_STATISTICS) */

    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[PND:OUT] --%s() / nLLDRet : 0x%x\r\n"), __FSR_FUNC__, nLLDRet));

    return (nLLDRet);
}



/**
 * @brief           This function erase a block of PureNAND
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
 *
 * @author          Jinho Yi
 * @version         1.2.1
 * @remark          As of now, supports only single plane, one block erase
 * @                Pbn is consecutive numbers within the device
 * @n               i.e. for DDP device, Pbn for 1st block in 2nd chip is not 0
 * @n               it is one more than the last block number in 1st chip
 *
 */
PUBLIC INT32
FSR_PND_Erase(UINT32  nDev,
              UINT32 *pnPbn,
              UINT32  nNumOfBlks,
              UINT32  nFlag)
{
    PureNANDCxt    *pstPNDCxt;
    PureNANDShMem  *pstPNDShMem;
    UINT32          nDie;
    UINT32          nPbn;    
    UINT32          nRowAddress;
    INT32           nLLDRet;

    do
    {
    
#if defined (FSR_LLD_STRICT_CHK)
        if (pnPbn == NULL)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[PND:ERR]   %s(nDev:%d, pnPbn:0x%08x, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, pnPbn, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            pnPbn is NULL\r\n")));
                
            nLLDRet = FSR_LLD_INVALID_PARAM;
            break;
        }
        
        nLLDRet = _StrictChk(nDev, *pnPbn, 0);
        if (nLLDRet != FSR_LLD_SUCCESS)
        {
            break;
        }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */

        pstPNDCxt   = gpstPNDCxt[nDev];
        pstPNDShMem = gpstPNDShMem[nDev];
        
        nPbn = *pnPbn;
        nDie = nPbn >> pstPNDCxt->nDDPSelBaseBit;

        nLLDRet = FSR_PND_FlushOp(nDev, nDie, nFlag);
        if (FSR_RETURN_MAJOR(nLLDRet) != FSR_LLD_SUCCESS)
        {
            break;
        }

        nRowAddress = nPbn << pstPNDCxt->nSftPgsPerBlk;

        _SetNANDCtrllerCmd(nDev, nDie, nRowAddress, INVALID_ADDRESS, FSR_PND_SET_ERASE);

        pstPNDShMem->nPreOp[nDie]         = FSR_PND_PREOP_ERASE;
        pstPNDShMem->nPreOpPbn[nDie]      = (UINT16) nPbn;
        pstPNDShMem->nPreOpPgOffset[nDie] = FSR_PND_PREOP_ADDRESS_NONE;
        pstPNDShMem->nPreOpFlag[nDie]     = nFlag;

#if defined (FSR_LLD_LOGGING_HISTORY)
        _AddLog(nDev, nDie);
#endif

#if defined (FSR_LLD_STATISTICS)
        _AddPNDStat(nDev, nDie, FSR_PND_STAT_ERASE, 0, FSR_PND_STAT_NORMAL_CMD);
#endif /* #if defined (FSR_LLD_STATISTICS) */

    } while(0);

    return (nLLDRet);
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
 * @return          FSR_LLD_PREV_2LV_READ_DISTURBANCE | {FSR_LLD_1STPLN_CURR_ERROR}
 * @return          FSR_LLD_PREV_WRITE_ERROR      | {FSR_LLD_1STPLN_PREV_ERROR}
 * @n               write error for previous write operation
 *
 * @author          Jinho Yi
 * @version         1.2.1
 * @remark          This function loads data from PureNAND to Pseudo DataRAM,
 * @n               and randomly writes data into the DataRAM
 * @n               and program back into PureNAND
 *
 */
PUBLIC INT32
FSR_PND_CopyBack(UINT32         nDev,
                 LLDCpBkArg    *pstCpArg,
                 UINT32         nFlag)
{
    PureNANDCxt      *pstPNDCxt;
    PureNANDSpec     *pstPNDSpec;
    PureNANDShMem    *pstPNDShMem;

    LLDRndInArg      *pstRIArg; /* Random in argument */

    UINT32            nCmdIdx;
    UINT32            nCnt;
    UINT32            nPgOffset;
    UINT32            nDie;
    UINT32            nPbn;
    UINT32            nRndOffset;
    UINT32            nFlushOpCaller;
    UINT32            nRowAddress;    

    BOOL32            bSpareBufRndIn;
    FSRSpareBufBase   stSpareBufBase;
    FSRSpareBuf       stSpareBuf;
    FSRSpareBufExt    stSpareBufExt[FSR_MAX_SPARE_BUF_EXT];

    INT32             nLLDRet           = FSR_LLD_SUCCESS;

    UINT8            *pMainDataRAM;
    UINT8            *pSpareDataRAM;

#if defined (FSR_LLD_STATISTICS)
    UINT32            nBytesTransferred = 0;
#endif /* #if defined (FSR_LLD_STATISTICS) */

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[PND:IN ] ++%s(nDev:%d, nFlag:0x%08x)\r\n"),
        __FSR_FUNC__, nDev, nFlag));

    do
    {

#if defined (FSR_LLD_STRICT_CHK)
        if (pstCpArg == NULL)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[PND:ERR]   %s(nDev:%d, pstCpArg:0x%08x, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, pstCpArg, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            pstCpArg is NULL\r\n")));

            nLLDRet = FSR_LLD_INVALID_PARAM;
            break;
        }
        
        /* Check Device Number */
        if (nDev >= FSR_PND_MAX_DEVS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[PND:ERR]   %s() / %d line\r\n"),
                __FSR_FUNC__, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Invalid Device Number (nDev = %d)\r\n"),
                nDev));

            nLLDRet = FSR_LLD_INVALID_PARAM;
            break;
        }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */

        pstPNDCxt   = gpstPNDCxt[nDev];
        pstPNDSpec  = pstPNDCxt->pstPNDSpec;
        pstPNDShMem = gpstPNDShMem[nDev];
        
        nCmdIdx = (nFlag & FSR_LLD_FLAG_CMDIDX_MASK) >> FSR_LLD_FLAG_CMDIDX_BASEBIT;

        pMainDataRAM  = pstPNDShMem->pPseudoMainDataRAM;
        pSpareDataRAM = pstPNDShMem->pPseudoSpareDataRAM;

        if (nCmdIdx == FSR_LLD_FLAG_1X_CPBK_LOAD)
        {
            FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_INF,
                (TEXT("[PND:INF]   %s(nDev:%d, nSrcPbn:%d, nSrcPg:%d, nFlag:0x%08x)\r\n"),
                __FSR_FUNC__, nDev, pstCpArg->nSrcPbn, pstCpArg->nSrcPgOffset, nFlag));

            /* 
             * Load phase of copyback() only checks the source block & page
             * offset. For BML does not fill the destination block & page
             * offset at this phase
             */
            nPbn      = pstCpArg->nSrcPbn;
            nPgOffset = pstCpArg->nSrcPgOffset;
        
#if defined (FSR_LLD_STRICT_CHK)
            nLLDRet = _StrictChk(nDev, nPbn, nPgOffset);
            if (nLLDRet != FSR_LLD_SUCCESS)
            {
                break;
            }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */

            nDie = nPbn >> pstPNDCxt->nDDPSelBaseBit;

            nFlushOpCaller = FSR_PND_PREOP_CPBK_LOAD << FSR_PND_FLUSHOP_CALLER_BASEBIT;

            /* 
             * eNAND doesn't support read interleaving,
             * therefore, wait until interrupt status is ready for both die.
             * if device is DDP. 
             */
            if (pstPNDCxt->nNumOfDies == FSR_MAX_DIES)
            {
                FSR_PND_FlushOp(nDev,
                                nFlushOpCaller | ((nDie + 0x1) & 0x1),
                                nFlag | FSR_LLD_FLAG_REMAIN_PREOP_STAT);
            }
            
            nLLDRet = FSR_PND_FlushOp(nDev, nFlushOpCaller | nDie, nFlag);
            if ((FSR_RETURN_MAJOR(nLLDRet) != FSR_LLD_SUCCESS) &&
                (FSR_RETURN_MAJOR(nLLDRet) != FSR_LLD_PREV_2LV_READ_DISTURBANCE))
            {
                break;
            }            

            nRowAddress  = nPbn << pstPNDCxt->nSftPgsPerBlk;
            nRowAddress |= (nPgOffset & pstPNDCxt->nMskPgsPerBlk);
            
            if ((nFlag & FSR_LLD_FLAG_ECC_MASK) == FSR_LLD_FLAG_ECC_ON)
            {
                _SetNANDCtrllerCmd(nDev, nDie, INVALID_ADDRESS, INVALID_ADDRESS, FSR_PND_SET_ECC_ON);
            }
            else
            {
                _SetNANDCtrllerCmd(nDev, nDie, INVALID_ADDRESS, INVALID_ADDRESS, FSR_PND_SET_ECC_OFF);
            }

            /* In case of non-blocking mode, interrupt should be enabled */
            if (nFlag & FSR_LLD_FLAG_INT_MASK)
            {
                FSR_OAM_ClrNEnableInt(pstPNDCxt->nIntID);
            }
            else
            {
                FSR_OAM_ClrNDisableInt(pstPNDCxt->nIntID);
            }

            _SetNANDCtrllerCmd(nDev, nDie, nRowAddress, 0, FSR_PND_SET_READ_PAGE);
            
            /* Store the type of previous operation for the deferred check */
            pstPNDShMem->nPreOp[nDie]         = FSR_PND_PREOP_CPBK_LOAD;
            pstPNDShMem->nPreOpPbn[nDie]      = (UINT16) nPbn;
            pstPNDShMem->nPreOpPgOffset[nDie] = (UINT16) nPgOffset;
            pstPNDShMem->nPreOpFlag[nDie]     = nFlag;
            
#if defined (FSR_LLD_LOGGING_HISTORY)
            _AddLog(nDev, nDie);
#endif
            
#if defined (FSR_LLD_STATISTICS)
            _AddPNDStat(nDev, nDie, FSR_PND_STAT_SLC_LOAD, 0, FSR_PND_STAT_NORMAL_CMD);
#endif /* #if defined (FSR_LLD_STATISTICS) */

        }
        else if (nCmdIdx == FSR_LLD_FLAG_1X_CPBK_PROGRAM)
        {
            FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_INF,
            (TEXT("[PND:INF]   %s(nDev:%d, nDstPbn:%d, nDstPg:%d, nRndInCnt:%d, nFlag:0x%08x)\r\n"),
            __FSR_FUNC__, nDev, pstCpArg->nDstPbn, pstCpArg->nDstPgOffset, pstCpArg->nRndInCnt, nFlag));

            nPbn      = pstCpArg->nDstPbn;
            nPgOffset = pstCpArg->nDstPgOffset;
        
#if defined (FSR_LLD_STRICT_CHK)
            nLLDRet = _StrictChk(nDev, nPbn, nPgOffset);
            if (nLLDRet != FSR_LLD_SUCCESS)
            {
                break;
            }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */

            nDie = nPbn >> pstPNDCxt->nDDPSelBaseBit;

            nFlushOpCaller = FSR_PND_PREOP_CPBK_PGM << FSR_PND_FLUSHOP_CALLER_BASEBIT;

            nLLDRet = FSR_PND_FlushOp(nDev, nFlushOpCaller | nDie, nFlag);

            if ((FSR_RETURN_MAJOR(nLLDRet) != FSR_LLD_SUCCESS) &&
                (FSR_RETURN_MAJOR(nLLDRet) != FSR_LLD_PREV_2LV_READ_DISTURBANCE))
            {
                break;
            }

            bSpareBufRndIn = FALSE32;
            for (nCnt = 0; nCnt < pstCpArg->nRndInCnt; nCnt++)
            {
                pstRIArg = pstCpArg->pstRndInArg + nCnt;
                if (pstRIArg->nOffset >= FSR_LLD_CPBK_SPARE)
                {
                    bSpareBufRndIn = TRUE32;

                    stSpareBuf.pstSpareBufBase  = &stSpareBufBase;
                    stSpareBuf.nNumOfMetaExt    = 1;
                    stSpareBuf.pstSTLMetaExt    = &stSpareBufExt[0];                   
                    _ReadSpare(&stSpareBuf,
                               pSpareDataRAM);
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
                        FSR_OAM_MEMCPY((UINT8 *) stSpareBuf.pstSpareBufBase + nRndOffset,
                                       (UINT8 *) pstRIArg->pBuf,
                                        pstRIArg->nNumOfBytes);                                     
                    }
                }
                else
                {
                    FSR_OAM_MEMCPY(pMainDataRAM + pstRIArg->nOffset,
                                   (UINT8 *) pstRIArg->pBuf,
                                   pstRIArg->nNumOfBytes);
                  
#if defined (FSR_LLD_STATISTICS)
                    nBytesTransferred += pstRIArg->nNumOfBytes;
#endif /* #if defined (FSR_LLD_STATISTICS) */                 

                }
            }

            if (bSpareBufRndIn == TRUE32)
            {
                _WriteSpare(pstPNDSpec,
                            pSpareDataRAM,
                            &stSpareBuf,
                            nFlag | FSR_LLD_FLAG_USE_SPAREBUF);

#if defined (FSR_LLD_STATISTICS)
                nBytesTransferred += FSR_SPARE_BUF_SIZE_2KB_PAGE;
#endif /* #if defined (FSR_LLD_STATISTICS) */

            }

            nRowAddress  = nPbn << pstPNDCxt->nSftPgsPerBlk;
            nRowAddress |= nPgOffset & pstPNDCxt->nMskPgsPerBlk;

            /* Set row & column address */
            _SetNANDCtrllerCmd(nDev, nDie, nRowAddress, 0, FSR_PND_SET_PROGRAM_PAGE);

            if ((nFlag & FSR_LLD_FLAG_ECC_MASK) == FSR_LLD_FLAG_ECC_ON)
            {
                _SetNANDCtrllerCmd(nDev, nDie, INVALID_ADDRESS, INVALID_ADDRESS, FSR_PND_SET_ECC_ON);
            }
            else
            {
                _SetNANDCtrllerCmd(nDev, nDie, INVALID_ADDRESS, INVALID_ADDRESS, FSR_PND_SET_ECC_OFF);
            }

            /* In case of non-blocking mode, interrupt should be enabled */
            if (nFlag & FSR_LLD_FLAG_INT_MASK)
            {
                FSR_OAM_ClrNEnableInt(pstPNDCxt->nIntID);
            }
            else
            {
                FSR_OAM_ClrNDisableInt(pstPNDCxt->nIntID);
            }

			_WriteToPageBuffer(nDev, nDie);
			
            /* Store the type of previous operation for the deferred check */
            pstPNDShMem->nPreOp[nDie]         = FSR_PND_PREOP_CPBK_PGM;
            pstPNDShMem->nPreOpPbn[nDie]      = (UINT16) nPbn;
            pstPNDShMem->nPreOpPgOffset[nDie] = (UINT16) nPgOffset;
            pstPNDShMem->nPreOpFlag[nDie]     = nFlag;
            
#if defined (FSR_LLD_STATISTICS)
            _AddPNDStat(nDev, nDie, FSR_PND_STAT_WR_TRANS, nBytesTransferred, FSR_PND_STAT_NORMAL_CMD);
            _AddPNDStat(nDev, nDie, FSR_PND_STAT_SLC_PGM, 0, FSR_PND_STAT_NORMAL_CMD);
#endif /* #if defined (FSR_LLD_STATISTICS) */

#if defined (FSR_LLD_LOGGING_HISTORY)
            _AddLog(nDev, nDie);
#endif

        }
        


    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[PND:OUT] --%s() / nLLDRet : 0x%x\r\n"), __FSR_FUNC__, nLLDRet));

    return (nLLDRet);
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
 * @author          Jinho Yi
 * @version         1.2.1
 * @remark          Pbn is consecutive numbers within the device
 * @n               i.e. for DDP device, Pbn for 1st block in 2nd chip is not 0
 * @n               it is one more than the last block number in 1st chip
 *
 */
PUBLIC INT32
FSR_PND_ChkBadBlk(UINT32         nDev,
                  UINT32         nPbn,
                  UINT32         nFlag)
{
    INT32             nLLDRet;
    UINT16            nDQ;
    PureNANDCxt      *pstPNDCxt;
    PureNANDShMem    *pstPNDShMem;
    UINT32            nDie;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[PND:IN ] ++%s(nDev:%d, nPbn:%d, nFlag:0x%08x)\r\n"),
        __FSR_FUNC__, nDev, nPbn, nFlag));

    do
    {

#if defined (FSR_LLD_STRICT_CHK)
        /* Check device number */
        nLLDRet = _StrictChk(nDev, nPbn, 0);
        if (nLLDRet != FSR_LLD_SUCCESS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[PND:ERR]   %s(nDev:%d, nPbn:%d, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, nPbn, nFlag, __LINE__));            
            break;
        }
#endif
 
        pstPNDCxt   = gpstPNDCxt[nDev];
        pstPNDShMem = gpstPNDShMem[nDev];
        nDie        = nPbn >> pstPNDCxt->nDDPSelBaseBit;

        nLLDRet = FSR_PND_ReadOptimal(nDev,  
                                      nPbn,                  
                                      (UINT32) 0,
                                      (UINT8 *) NULL,
                                      (FSRSpareBuf *) NULL,   
                                      (UINT32) (FSR_LLD_FLAG_ECC_OFF | FSR_LLD_FLAG_1X_LOAD));
        if (FSR_RETURN_MAJOR(nLLDRet) != FSR_LLD_SUCCESS)
        {
            break;
        }

        nLLDRet = FSR_PND_FlushOp(nDev, nDie, nFlag);
        if (FSR_RETURN_MAJOR(nLLDRet) != FSR_LLD_SUCCESS)
        {
            break;
        }
        
        nDQ = *((UINT16 *)pstPNDShMem->pPseudoSpareDataRAM);
       
        if (nDQ != FSR_PND_VALID_BLK_MARK)
        {        	
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR | FSR_DBZ_LLD_INF,
                            (TEXT("[PND:INF]   %s(nDev:%d, nPbn:%d, nFlag:0x%08x) / %d line\r\n"),
                            __FSR_FUNC__, nDev, nPbn, nFlag, __LINE__));


            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR | FSR_DBZ_LLD_INF,
                            (TEXT("            nPbn = %d is a bad block\r\n"), nPbn));

            nLLDRet = FSR_LLD_INIT_BADBLOCK | FSR_LLD_BAD_BLK_1STPLN;
        }
        else
        {
            FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_INF,
                            (TEXT("[PND:INF]   %s(nDev:%d, nPbn:%d, nFlag:0x%08x) / %d line\r\n"),
                            __FSR_FUNC__, nDev, nPbn, nFlag, __LINE__));

            FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_INF,
                            (TEXT("            nPbn = %d is a good block\r\n"), nPbn));

            nLLDRet = FSR_LLD_INIT_GOODBLOCK;
        }
        
#if defined (FSR_LLD_STATISTICS)
        _AddPNDStat(nDev, nDie, FSR_PND_STAT_SLC_LOAD, 0, FSR_PND_STAT_NORMAL_CMD);
#endif /* #if defined (FSR_LLD_STATISTICS) */

    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[PND:OUT] --%s() / nLLDRet : 0x%x\r\n"), __FSR_FUNC__, nLLDRet));

    return (nLLDRet);
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
 * @return          FSR_LLD_PREV_WRITE_ERROR | {FSR_LLD_1STPLN_CURR_ERROR}
 * @return          FSR_LLD_PREV_ERASE_ERROR | {FSR_LLD_1STPLN_CURR_ERROR}
 * @return          FSR_LLD_INVALID_PARAM
 *
 * @author          Jinho Yi
 * @version         1.2.1
 * @remark          This function completes previous operation,
 * @n               and returns error for previous operation
 * @n               After calling series of FSR_FND_Write() or FSR_FND_Erase() or
 * @n               FSR_FND_CopyBack(), FSR_FND_FlushOp() needs to be called.
 *
 */
PUBLIC INT32
FSR_PND_FlushOp(UINT32         nDev,
                UINT32         nDie,
                UINT32         nFlag)
{
            
    PureNANDCxt        *pstPNDCxt;
    PureNANDShMem      *pstPNDShMem;
    
    UINT32              nPrevOp;

#if !defined(FSR_OAM_RTLMSG_DISABLE)
    UINT32              nPrevPbn;
    UINT32              nPrevPgOffset;
#endif

    UINT32              nPrevFlag;    
    INT32               nLLDRet         = FSR_LLD_SUCCESS;    

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[PND:IN ] ++%s(nDev: %d, nDieIdx: %d, nFlag: 0x%08x)\r\n"),
        __FSR_FUNC__, nDev, nDie, nFlag));

    do
    {

#if defined (FSR_LLD_STRICT_CHK)
        /* Check device number */
        if (nDev >= FSR_PND_MAX_DEVS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("[PND:ERR]   %s(nDev:%d, nDie:%d, nFlag:0x%08x) / %d line\r\n"),
            __FSR_FUNC__, nDev, nDie, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("            Invalid Device Number (nDev = %d)\r\n"), nDev));

            nLLDRet = FSR_LLD_INVALID_PARAM;
            break;
        }
#endif

        pstPNDCxt   = gpstPNDCxt[nDev];
        pstPNDShMem = gpstPNDShMem[nDev];
        
        pstPNDCxt->nFlushOpCaller = (UINT16) (nDie >> FSR_PND_FLUSHOP_CALLER_BASEBIT);
        
        nDie = nDie & ~FSR_PND_FLUSHOP_CALLER_MASK;

        nPrevOp       = pstPNDShMem->nPreOp[nDie];
        
#if !defined(FSR_OAM_RTLMSG_DISABLE)
        nPrevPbn      = pstPNDShMem->nPreOpPbn[nDie];
        nPrevPgOffset = pstPNDShMem->nPreOpPgOffset[nDie];
#endif

        nPrevFlag     = pstPNDShMem->nPreOpFlag[nDie];

        switch (nPrevOp)
        {
        case FSR_PND_PREOP_NONE:
            /* DO NOT remove this code : 'case FSR_PND_PREOP_NONE:'
               for compiler optimization */
            break;

        case FSR_PND_PREOP_READ:
        case FSR_PND_PREOP_CPBK_LOAD:
            if(_WaitPNDIntStat(nDev, nDie, FSR_PND_OP_READ) == FSR_PND_BUSY)
            {
                nLLDRet = FSR_LLD_NO_RESPONSE;
                break;
            }
            
            nLLDRet = _GetOpStatus(nDev, nDie, FSR_PND_OP_READ);
            
            if((nPrevFlag & FSR_LLD_FLAG_ECC_ON) == FSR_LLD_FLAG_ECC_ON)
            {
                if (FSR_RETURN_MAJOR(nLLDRet) == FSR_LLD_PREV_READ_ERROR)
                {
                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                        (TEXT("[PND:ERR]   %s(nDev:%d, nDie:%d, nFlag:0x%08x) / %d line\r\n"),
                        __FSR_FUNC__, nDev, nDie, nFlag, __LINE__));

                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                        (TEXT("            pstPNDCxt->nFlushOpCaller : %d\r\n"),
                        pstPNDCxt->nFlushOpCaller));
                        
                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                        (TEXT("            ECC Status : Uncorrectable\r\n")));

                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                        (TEXT("            at nDev:%d, nPbn:%d, nPgOffset:%d, nFlag:0x%08x\r\n"),
                        nDev, nPrevPbn, nPrevPgOffset, nPrevFlag));
                        
                    _DumpMainBuffer(nDev);
                    _DumpSpareBuffer(nDev);
                }
                else if(FSR_RETURN_MAJOR(nLLDRet) == FSR_LLD_PREV_2LV_READ_DISTURBANCE)
                {
                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                        (TEXT("[PND:ERR]   %s(nDev:%d, nDie:%d, nFlag:0x%08x) / %d line\r\n"),
                        __FSR_FUNC__, nDev, nDie, nFlag, __LINE__));

                    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                        (TEXT("            pstPNDCxt->nFlushOpCaller : %d\r\n"),
                        pstPNDCxt->nFlushOpCaller));
                        
                    FSR_DBZ_RTLMOUT(FSR_DBZ_LLD_INF | FSR_DBZ_ERROR,
                        (TEXT("            2Lv read disturbance at nPbn:%d, nPgOffset:%d, nFlag:0x%08x\r\n"),
                        nPrevPbn, nPrevPgOffset, nPrevFlag));
                            
                    _DumpSpareBuffer(nDev);
                }
            }            
            break;

        case FSR_PND_PREOP_CACHE_PGM: 
        case FSR_PND_PREOP_PROGRAM:
        case FSR_PND_PREOP_CPBK_PGM:
            if(_WaitPNDIntStat(nDev, nDie, FSR_PND_OP_PGM) == FSR_PND_BUSY)
            {
                nLLDRet = FSR_LLD_NO_RESPONSE;
                break;
            }
                        
            nLLDRet = _GetOpStatus(nDev, nDie, FSR_PND_OP_PGM);
            
            if (nLLDRet != FSR_LLD_SUCCESS)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[PND:ERR]   %s(nDev:%d, nDie:%d, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, nDie, nFlag, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            pstPNDCxt->nFlushOpCaller : %d\r\n"),
                    pstPNDCxt->nFlushOpCaller));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            last write() call @ nDev = %d, nPbn = %d, nPgOffset = %d, nFlag:0x%08x\r\n"),
                    nDev, nPrevPbn, nPrevPgOffset, nPrevFlag));
            }
            break;
            
        case FSR_PND_PREOP_ERASE:        
            if(_WaitPNDIntStat(nDev, nDie, FSR_PND_OP_ERASE) == FSR_PND_BUSY)
            {
                nLLDRet = FSR_LLD_NO_RESPONSE;
                break;
            }
            
            /* Previous error check */
            nLLDRet = _GetOpStatus(nDev, nDie, FSR_PND_OP_ERASE);
            
            if (nLLDRet != FSR_LLD_SUCCESS)
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[PND:ERR]   %s(nDev:%d, nDie:%d, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, nDie, nFlag, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            pstPNDCxt->nFlushOpCaller : %d\r\n"),
                    pstPNDCxt->nFlushOpCaller));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            last Erase() call @ nDev = %d, nPbn = %d, nFlag:0x%08x\r\n"),
                    nDev, nPrevPbn, nPrevFlag));
            }
            break;

        default:
            /* Other IOCtl operations is ignored */            
            break;
        }
        
        if (!(nFlag & FSR_LLD_FLAG_REMAIN_PREOP_STAT))
        {
            pstPNDShMem->nPreOp[nDie] = FSR_PND_PREOP_NONE;
        }
        
#if defined (FSR_LLD_STATISTICS)
        _AddPNDStat(nDev, nDie, FSR_PND_STAT_FLUSH, 0, FSR_PND_STAT_NORMAL_CMD);
#endif        
        
    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[PND:OUT] --%s() / nLLDRet : 0x%x\r\n"), __FSR_FUNC__, nLLDRet));

    return (nLLDRet);
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
 * @author          Jinho Yi
 * @version         1.2.1
 * @remark          i.e. SLC block, the number of pages per block
 *
 */
PUBLIC INT32
FSR_PND_GetBlockInfo(UINT32         nDev,
                     UINT32         nPbn,
                     UINT32        *pBlockType,
                     UINT32        *pPgsPerBlk)
{
    PureNANDSpec     *pstPNDSpec;

    INT32             nLLDRet     = FSR_LLD_SUCCESS;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[PND:IN ] ++%s(nDev:%d, nPbn:%d)\r\n"),
        __FSR_FUNC__, nDev, nPbn));

    do
    {
    
#if defined (FSR_LLD_STRICT_CHK)
        nLLDRet = _StrictChk(nDev, nPbn, 0);

        if (nLLDRet != FSR_LLD_SUCCESS)
        {
            break;
        }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */

        pstPNDSpec = gpstPNDCxt[nDev]->pstPNDSpec;

        /* This block is SLC block */
        if (pBlockType != NULL)
        {
            *pBlockType = FSR_LLD_SLC_BLOCK;
        }
        
        if (pPgsPerBlk != NULL)
        {
            *pPgsPerBlk = pstPNDSpec->nPgsPerBlk;
        }

    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[PND:OUT] --%s() / nLLDRet : 0x%x\r\n"), __FSR_FUNC__, nLLDRet));

    return nLLDRet;
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
 * @author          Jinho Yi
 * @version         1.2.1
 * @remark
 *
 */
PUBLIC INT32
FSR_PND_GetDevSpec(UINT32         nDev,
                   FSRDevSpec    *pstDevSpec,
                   UINT32         nFlag)
{
    PureNANDCxt    *pstPNDCxt;
    PureNANDSpec   *pstPNDSpec;

    UINT32          nDieIdx;
    INT32           nLLDRet     = FSR_LLD_SUCCESS;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[PND:IN ] ++%s(nDev:%d,pstDevSpec:0x%x,nFlag:0x%08x)\r\n"),
        __FSR_FUNC__, nDev, pstDevSpec, nFlag));

    do
    {

#if defined (FSR_LLD_STRICT_CHK)
        /* Check device number */
        if (nDev >= FSR_PND_MAX_DEVS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[PND:ERR]   %s(nDev:%d, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Invalid Device Number (nDev = %d)\r\n"),
                nDev));

            nLLDRet = FSR_LLD_INVALID_PARAM;
            break;
        }

        if (pstDevSpec == NULL)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[PND:ERR]   %s(nDev:%d, pstDevSpec:0x%08x, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, pstDevSpec, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            nDev:%d pstDevSpec is NULL\r\n"), nDev));

            nLLDRet = FSR_LLD_OPEN_FAILURE;
            break;
        }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */

        pstPNDCxt  = gpstPNDCxt[nDev];
        pstPNDSpec = pstPNDCxt->pstPNDSpec;

        FSR_OAM_MEMSET(pstDevSpec, 0xFF, sizeof(FSRDevSpec));

        pstDevSpec->nNumOfBlks          = pstPNDSpec->nNumOfBlks;
        pstDevSpec->nNumOfPlanes        = pstPNDSpec->nNumOfPlanes;

        for (nDieIdx = 0; nDieIdx < pstPNDCxt->nNumOfDies; nDieIdx++)
        {
            pstDevSpec->nBlksForSLCArea[nDieIdx] =
                pstPNDSpec->nNumOfBlks / pstPNDCxt->nNumOfDies;
        }

        pstDevSpec->nSparePerSct        = pstPNDSpec->nSparePerSct;
        pstDevSpec->nSctsPerPG          = pstPNDSpec->nSctsPerPg;
        pstDevSpec->nNumOfBlksIn1stDie  =
            pstPNDSpec->nNumOfBlks / pstPNDCxt->nNumOfDies;

        pstDevSpec->nDID                = pstPNDSpec->nDID;

        pstDevSpec->nPgsPerBlkForSLC    = pstPNDSpec->nPgsPerBlk;
        pstDevSpec->nPgsPerBlkForMLC    = 0;
        pstDevSpec->nNumOfDies          = pstPNDCxt->nNumOfDies;
        pstDevSpec->nUserOTPScts        = 0;
        pstDevSpec->b1stBlkOTP          = FALSE32;
        pstDevSpec->nRsvBlksInDev       = pstPNDSpec->nRsvBlksInDev;
        pstDevSpec->pPairedPgMap        = NULL;
        pstDevSpec->pLSBPgMap           = NULL;
        pstDevSpec->nNANDType           = FSR_LLD_SLC_NAND;
        pstDevSpec->nPgBufToDataRAMTime = 0;
        pstDevSpec->bCachePgm           = FALSE32;
        pstDevSpec->nSLCTLoadTime       = 0;
        pstDevSpec->nMLCTLoadTime       = 0;
        pstDevSpec->nSLCTProgTime       = 0;
        pstDevSpec->nMLCTProgTime[0]    = 0;
        pstDevSpec->nMLCTProgTime[1]    = 0;
        pstDevSpec->nTEraseTime         = 0;

        /* Time for transfering 1 page in u sec */
        pstDevSpec->nWrTranferTime      = 0;
        pstDevSpec->nRdTranferTime      = 0;

        pstDevSpec->nSLCPECycle         = 50000;
        pstDevSpec->nMLCPECycle         = 10000;

    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[PND:OUT] --%s() / nLLDRet : 0x%x\r\n"), __FSR_FUNC__, nLLDRet));

    return (nLLDRet);
}



/**
 * @brief           This function reads data from DataRAM of Flex-OneNAND
 *
 * @param[in]       nDev    : Physical Device Number (0 ~ 3)
 * @param[out]      pMBuf   : Memory buffer for main  array of NAND flash
 * @param[out]      pSBuf   : Memory buffer for spare array of NAND flash
 * @n               nDieIdx : 0 is for 1st die for DDP device
 * @n                       : 1 is for 2nd die for DDP device
 * @param[in]       nFlag   :
 *
 * @return          FSR_LLD_SUCCESS
 *
 * @author          Jinhyuck Kim
 * @version         1.2.1
 * @remark          This function does not loads data from PureNAND
 * @n               it just reads data which lies on DataRAM
 *
 */
PUBLIC INT32
FSR_PND_GetPrevOpData(UINT32         nDev,
                      UINT8         *pMBuf,
                      FSRSpareBuf   *pSBuf,
                      UINT32         nDieIdx,
                      UINT32         nFlag)
{    
    PureNANDCxt        *pstPNDCxt;
    PureNANDShMem      *pstPNDShMem;
    PureNANDSpec       *pstPNDSpec;
    INT32               nLLDRe      = FSR_LLD_SUCCESS;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[PND:IN ] ++%s() / nDie : %d / nFlag : 0x%x\r\n"), __FSR_FUNC__, nDieIdx, nFlag));

    do
    {

#if defined (FSR_LLD_STRICT_CHK)
        /* Check device number */
        if (nDev >= FSR_PND_MAX_DEVS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[PND:ERR]   %s(nDev:%d, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Invalid Device Number (nDev = %d)\r\n"),
                nDev));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }

        /* nDie can be 0 (1st die) or 1 (2nd die) */
        if ((nDieIdx & 0xFFFE) != 0)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[PND:ERR]   %s(nDev:%d, nDie:%d, nFlag:0x%08x) / %d line\r\n"),
                __FSR_FUNC__, nDev, nDieIdx, nFlag, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Invalid nDie Number (nDie = %d)\r\n"),
                nDieIdx));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }
#endif

        pstPNDCxt   = gpstPNDCxt[nDev];
        pstPNDSpec  = pstPNDCxt->pstPNDSpec;
        pstPNDShMem = gpstPNDShMem[nDev];

        FSR_OAM_MEMCPY(pMBuf,
                       pstPNDShMem->pPseudoMainDataRAM,
                       pstPNDSpec->nSctsPerPg * FSR_SECTOR_SIZE);

        _ReadSpare(pSBuf,
                   pstPNDShMem->pPseudoSpareDataRAM);
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
 * @return          FSR_LLD_PI_PROGRAM_ERROR
 * @return          FSR_LLD_PREV_READ_ERROR  | {FSR_LLD_1STPLN_CURR_ERROR }
 *
 * @author          Jinho Yi
 * @version         1.2.1
 * @remark          OTP read, write is performed with FSR_FND_Write(), FSR_FND_ReadOptimal(),
 * @n               after OTP Access
 *
 */
PUBLIC INT32
FSR_PND_IOCtl(UINT32         nDev,
              UINT32         nCode,
              UINT8         *pBufI,
              UINT32         nLenI,
              UINT8         *pBufO,
              UINT32         nLenO,
              UINT32        *pByteRet)
{
    /* Used to lock, unlock, lock-tight */
    LLDProtectionArg *pLLDProtectionArg;
    PureNANDCxt      *pstPNDCxt;
    PureNANDShMem    *pstPNDShMem;             
    UINT32            nDie             = 0xFFFFFFFF;
    UINT32            nPbn;
    INT32             nLLDRet          = FSR_LLD_SUCCESS;
    UINT16            nWrProtectStat;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[PND:IN ] ++%s(nDev:%d, nCode:0x%08x)\r\n"),
        __FSR_FUNC__, nDev, nCode));

    do
    {

#if defined (FSR_LLD_STRICT_CHK)
        /* Check device number */
        if (nDev >= FSR_PND_MAX_DEVS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("[PND:ERR]   %s() / %d line\r\n"),
            __FSR_FUNC__, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("            Invalid Device Number (nDev = %d)\r\n"), nDev));

            nLLDRet = (FSR_LLD_INVALID_PARAM);
            break;
        }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */

        pstPNDCxt   = gpstPNDCxt[nDev];
        pstPNDShMem = gpstPNDShMem[nDev];

        /* Interrupt should be enabled in I/O Ctl */
        FSR_OAM_ClrNDisableInt(pstPNDCxt->nIntID);

        switch (nCode)
        {
        case FSR_LLD_IOCTL_OTP_ACCESS:        
            nDie = 0;
            
            if (pByteRet != NULL)
            {
                *pByteRet = (UINT32) 0;
            }
            break;

        case FSR_LLD_IOCTL_OTP_LOCK:
            nDie = 0;            
            if (pByteRet != NULL)
            {
                *pByteRet = (UINT32) 0;
            }
            break;

        case FSR_LLD_IOCTL_OTP_GET_INFO:
            nDie = 0;
            if (pByteRet != NULL)
            {
                *pByteRet = (UINT32) 0;
            }
            break;

        case FSR_LLD_IOCTL_LOCK_TIGHT:
            if ((pBufI == NULL) || (nLenI != sizeof(LLDProtectionArg)) ||
                (pBufO == NULL) || (nLenO != sizeof(UINT32)))
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[FND:ERR]   %s() / %d line\r\n"),
                    __FSR_FUNC__, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            invalid parameter pBufI = 0x%08x, nLenI = %d\r\n"), pBufI, nLenI));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            invalid parameter pBufO = 0x%08x, nLenO = %d\r\n"), pBufO, nLenO));

                nLLDRet = FSR_LLD_INVALID_PARAM;
                break;
            }

            pLLDProtectionArg = (LLDProtectionArg *) pBufI;

            nDie = pLLDProtectionArg->nStartBlk >> pstPNDCxt->nDDPSelBaseBit;

            FSR_ASSERT((nDie & ~0x1) == 0);
            
            if (pByteRet != NULL)
            {
                *pByteRet = (UINT32) 0;
            }
            break;

        case FSR_LLD_IOCTL_LOCK_BLOCK:
            if ((pBufI == NULL) || (nLenI != sizeof(LLDProtectionArg)) ||
                (pBufO == NULL) || (nLenO != sizeof(UINT32)))
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[FND:ERR]   %s() / %d line\r\n"),
                    __FSR_FUNC__, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            invalid parameter pBufI = 0x%08x, nLenI = %d\r\n"), pBufI, nLenI));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            invalid parameter pBufO = 0x%08x, nLenO = %d\r\n"), pBufO, nLenO));

                nLLDRet = FSR_LLD_INVALID_PARAM;
                break;
            }

            pLLDProtectionArg = (LLDProtectionArg *) pBufI;

            nDie = pLLDProtectionArg->nStartBlk >> pstPNDCxt->nDDPSelBaseBit;

            FSR_ASSERT((nDie & ~0x1) == 0);

            if (pByteRet != NULL)
            {
                *pByteRet = (UINT32) 0;
            }
            break;

        case FSR_LLD_IOCTL_UNLOCK_BLOCK:
            if ((pBufI == NULL) || (nLenI != sizeof(LLDProtectionArg)) ||
                (pBufO == NULL) || (nLenO != sizeof(UINT32)))
            {
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("[FND:ERR]   %s() / %d line\r\n"),
                    __FSR_FUNC__, __LINE__));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            invalid parameter pBufI = 0x%08x, nLenI = %d\r\n"), pBufI, nLenI));

                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("            invalid parameter pBufO = 0x%08x, nLenO = %d\r\n"), pBufO, nLenO));

                nLLDRet = FSR_LLD_INVALID_PARAM;
                break;
            }

            pLLDProtectionArg = (LLDProtectionArg *) pBufI;

            nDie = pLLDProtectionArg->nStartBlk >> pstPNDCxt->nDDPSelBaseBit;

            FSR_ASSERT((nDie & ~0x1) == 0);
            
            if (pByteRet != NULL)
            {
                *pByteRet = (UINT32) 0;
            }
            break;

        case FSR_LLD_IOCTL_UNLOCK_ALLBLK:
            if ((pBufI == NULL) || (nLenI != sizeof(LLDProtectionArg)) ||
               (pBufO == NULL) || (nLenO != sizeof(UINT32)))
            {
               FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                   (TEXT("[FND:ERR]   %s() / %d line\r\n"),
                   __FSR_FUNC__, __LINE__));

               FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                   (TEXT("            invalid parameter pBufI = 0x%08x, nLenI = %d\r\n"), pBufI, nLenI));

               FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                   (TEXT("            invalid parameter pBufO = 0x%08x, nLenO = %d\r\n"), pBufO, nLenO));

               nLLDRet = FSR_LLD_INVALID_PARAM;
               break;
            }            

            pLLDProtectionArg = (LLDProtectionArg *) pBufI;

            nDie = pLLDProtectionArg->nStartBlk >> pstPNDCxt->nDDPSelBaseBit;

            FSR_ASSERT((nDie & ~0x1) == 0);

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

                nLLDRet = FSR_LLD_INVALID_PARAM;
                break;
            }

            nPbn    = *(UINT32 *) pBufI;

            nDie = nPbn >> pstPNDCxt->nDDPSelBaseBit;

            FSR_ASSERT((nDie & ~0x1) == 0);

            nWrProtectStat = FSR_LLD_BLK_STAT_UNLOCKED;

            *(UINT32 *) pBufO = (UINT32) nWrProtectStat;
            
            if (pByteRet != NULL)
            {
                *pByteRet = (UINT32) sizeof(UINT32);
            }

            nLLDRet    = FSR_LLD_SUCCESS;
            break;

        case FSR_LLD_IOCTL_HOT_RESET:            
            if (pByteRet != NULL)
            {
                *pByteRet = (UINT32) 0;
            }
            break;

        case FSR_LLD_IOCTL_CORE_RESET:            
            if (pByteRet != NULL)
            {
                *pByteRet = (UINT32) 0;
            }
            break;

        default:
            nLLDRet = FSR_LLD_IOCTL_NOT_SUPPORT;
            break;
        }

        if ((nCode == FSR_LLD_IOCTL_HOT_RESET) || (nCode == FSR_LLD_IOCTL_CORE_RESET))
        {
            for (nDie = 0; nDie < pstPNDCxt->nNumOfDies; nDie++)
            {
                pstPNDShMem->nPreOp[nDie]         = FSR_PND_PREOP_IOCTL;
                pstPNDShMem->nPreOpPbn[nDie]      = FSR_PND_PREOP_ADDRESS_NONE;
                pstPNDShMem->nPreOpPgOffset[nDie] = FSR_PND_PREOP_ADDRESS_NONE;
                pstPNDShMem->nPreOpFlag[nDie]     = FSR_PND_PREOP_FLAG_NONE;
            }
        }
        else if ((nLLDRet != FSR_LLD_INVALID_PARAM) && (nLLDRet != FSR_LLD_IOCTL_NOT_SUPPORT))
        {
            FSR_ASSERT(nDie != 0xFFFFFFFF);

            pstPNDShMem->nPreOp[nDie]         = FSR_PND_PREOP_IOCTL;
            pstPNDShMem->nPreOpPbn[nDie]      = FSR_PND_PREOP_ADDRESS_NONE;
            pstPNDShMem->nPreOpPgOffset[nDie] = FSR_PND_PREOP_ADDRESS_NONE;
            pstPNDShMem->nPreOpFlag[nDie]     = FSR_PND_PREOP_FLAG_NONE;
        }
    } while (0);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[PND:OUT] --%s() / nLLDRet : 0x%x\r\n"), __FSR_FUNC__, nLLDRet));

    return (nLLDRet);
}



PRIVATE 
VOID _GetPNDID(UINT32         nDev,
               NANDIDData    *pstNANDID)
{
                UINT32      nCS;
    volatile    UINT32      nCnt = 5;
    volatile    UINT16     *pTgt16;
    
    nCS = nDev;

    GPMC_NAND_COMMAND_(nCS) = OMAP2430_CMD_READ_ID;            
    GPMC_NAND_ADDRESS_(nCS) = 0x00;
        
    /* Wait 30ns to get status data */
    while (nCnt--);
        
    pTgt16 = (volatile UINT16 *) (GPMC_NAND_DATA_(nCS));
    
    pstNANDID->nMID         = *(UINT8 *)pTgt16;
    pstNANDID->nDID         = *(UINT8 *)pTgt16;
    pstNANDID->n3rdIDData   = *(UINT8 *)pTgt16;
    pstNANDID->n4thIDData   = *(UINT8 *)pTgt16;
    pstNANDID->n5thIDData   = *(UINT8 *)pTgt16;
}



PRIVATE INT32 _GetOpStatus(UINT32         nDev,
                           UINT32         nDie, 
                           UINT32         nOp)
{
                UINT32  nCS;
                UINT8   nStatus;
                INT32   nRe         = FSR_LLD_SUCCESS;    
    volatile    INT32   nCnt        = 5;
    volatile    UINT16 *pTgt16;
    
    nCS         = nDev;
    
    switch (nOp)
    {
    case FSR_PND_OP_READ:
        /* 
         * Copy data to psuedo DataRAM rather than user buffer
         * Although this operation decreases performance, 
         * it maintains the semantic FlushOP function.
         */
        nRe = _ReadPageBuffer(nDev,
                              nDie);
        break;
    case FSR_PND_OP_PGM:
        GPMC_NAND_COMMAND_(nCS) = OMAP2430_CMD_READ_STATUS;
        
        /* Wait 100ns to get status data */
        while (nCnt--);        
            
        pTgt16  = (volatile UINT16 *) (GPMC_NAND_DATA_(nCS));        
        nStatus = (*(UINT8 *)pTgt16);        
        nRe     = (nStatus & FSR_PND_STATUS_PGM_FAIL)? 
                  FSR_LLD_1STPLN_CURR_ERROR|FSR_LLD_PREV_WRITE_ERROR :
                  FSR_LLD_SUCCESS;
        break;
    case FSR_PND_OP_ERASE:
        GPMC_NAND_COMMAND_(nCS) = OMAP2430_CMD_READ_STATUS;
        
        /* Wait 100ns to get status data */
        while (nCnt--);        
            
        pTgt16  = (volatile UINT16 *) (GPMC_NAND_DATA_(nCS));        
        nStatus = (*(UINT8 *)pTgt16);
        nRe     = (nStatus & FSR_PND_STATUS_ERASE_FAIL)? 
                  FSR_LLD_1STPLN_CURR_ERROR|FSR_LLD_PREV_ERASE_ERROR :
                  FSR_LLD_SUCCESS;
        break;    
    }
    
    return nRe;
}



PRIVATE INT32   _WaitPNDIntStat     (UINT32         nDev,
                                     UINT32         nDie,
                                     UINT32         nOp)
{
             UINT32     nCS;
             INT32      nTimeLimit          = FSR_PND_BUSYWAIT_TIME_LIMIT;
             INT32      nReadyBusyStatus    = FSR_PND_BUSY;
    volatile INT32      nCnt                = 5;
    volatile UINT16    *pTgt16;
    
    nCS = nDev;
    
    GPMC_NAND_COMMAND_(nCS) = OMAP2430_CMD_READ_STATUS;
    
    /* Wait 100ns to get status data */
    while (nCnt--);
    
    do
    {
    
#if !defined(FSR_LLD_ENABLE_CNTRLLER_INT)        
        pTgt16 = (volatile UINT16 *) (GPMC_NAND_DATA_(nCS));
        
        if(*(UINT8 *)pTgt16 & FSR_PND_STATUS_READY)
        {
            nReadyBusyStatus = FSR_PND_READY;            
        }
#else	
        /* Read GPMC NAND status register */
        if (!(GPMC_STATUS & NAND_FLASH_STATUS_BUSY_MASK_(nCS)))
        {
            nReadyBusyStatus = FSR_PND_READY;            
        }
#endif

        if (--nTimeLimit == 0)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,                                     
                (TEXT("[PND:ERR]   busy wait time-out %s(), %d line\r\n"),
                __FSR_FUNC__, __LINE__));
            FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
                (TEXT("[PND:OUT] --%s()\r\n"), __FSR_FUNC__));
            _DumpCmdLog();            
        }                                                                      
    }while (nReadyBusyStatus == FSR_PND_BUSY);

    return nReadyBusyStatus;
}



/**
 * @brief           This function is used for pure NAND
 *                  this function is used only for H/W ECC not for S/W ECC
 *
 * @return          FSR_PAM_SUCCESS
 *
 * @author          JinHo Yi
 * @version         1.0.0
 *
 */
PRIVATE VOID
_SetECC(UINT32 nDev,
        UINT32 nDie,
        UINT32 nSetECC)
{
        
#if !defined(FSR_LLD_ENABLE_CNTRLLER_ECC)
    FSR_STACK_VAR;

    FSR_STACK_END;
    
    gpstPNDCxt[nDev]->nECCOption = nSetECC;
#endif
}



/**
 * @brief           This function is used for pure NAND
 *                  this function read data from NAND flash
 *
 * @return          FSR_PAM_SUCCESS
 *
 * @author          JinHo Yi
 * @version         1.2.1
 *
 */
PRIVATE 
INT32   _ReadPageBuffer(UINT32  nDev,
                        UINT32  nDie)
{
    volatile UINT16        *pMBuf16;
    volatile UINT16        *pSBuf16;    
    volatile UINT16        *pTgt16;
    volatile UINT8         *pMBuf8;
    volatile UINT8         *pSBuf8;    
    volatile UINT8         *pTgt8;
             UINT8         *pMBuf;
             UINT8         *pSBuf;
             UINT32	        nCnt;
             UINT32         nCS;             
             INT32          nNumOfTransfer;
             INT32          nLLDRe      = FSR_LLD_SUCCESS;             
             PureNANDCxt   *pstPNDCxt;
             PureNANDSpec  *pstPNDSpec;
             PureNANDShMem *pstPNDShMem;    
             
    FSR_STACK_VAR;

    FSR_STACK_END;

    nCS         = nDev;
    pstPNDCxt   = gpstPNDCxt[nDev];
    pstPNDSpec  = pstPNDCxt->pstPNDSpec;
    pstPNDShMem = gpstPNDShMem[nDev];
    
    pMBuf = pstPNDShMem->pPseudoMainDataRAM;
    pSBuf = pstPNDShMem->pPseudoSpareDataRAM;
    
    _SetNANDCtrllerCmd(nDev, nDie, INVALID_ADDRESS, 0, FSR_PND_SET_READ_PAGE);
   
    if(pstPNDSpec->nBWidth == 16)
    {
        pTgt16 = (volatile UINT16 *)(GPMC_NAND_DATA_(nCS));    
        
        nNumOfTransfer = pstPNDSpec->nSctsPerPg * FSR_SECTOR_SIZE / sizeof(UINT16);
        
        pMBuf16 = (volatile UINT16 *)pMBuf;

        for (nCnt = nNumOfTransfer; nCnt > 0; nCnt--)
        {
            *pMBuf16++ = *pTgt16;            
        }
        
        nNumOfTransfer = pstPNDSpec->nSctsPerPg * FSR_SPARE_SIZE / sizeof(UINT16);
        
        pSBuf16 = (volatile UINT16 *)pSBuf;

        for (nCnt = nNumOfTransfer; nCnt > 0; nCnt--)
        {
            *pSBuf16++ = *pTgt16;
        }
    }
    else if(pstPNDSpec->nBWidth == 8)
    {
        pTgt8 = (volatile UINT8 *)(GPMC_NAND_DATA_(nCS));    
        
        nNumOfTransfer = pstPNDSpec->nSctsPerPg * FSR_SECTOR_SIZE / sizeof(UINT8);
        
        pMBuf8 = (volatile UINT8 *)pMBuf;

        for (nCnt = nNumOfTransfer; nCnt > 0; nCnt--)
        {
            *pMBuf8++ = *pTgt8;
        }
        
        nNumOfTransfer = pstPNDSpec->nSctsPerPg * FSR_SPARE_SIZE / sizeof(UINT8);
        
        pSBuf8 = (volatile UINT8 *)pSBuf;

        for (nCnt = nNumOfTransfer; nCnt > 0; nCnt--)
        {
            *pSBuf8++ = *pTgt8;
        }
    }
    
#if !defined(FSR_LLD_ENABLE_CNTRLLER_ECC)
    if(pstPNDCxt->nECCOption == FSR_PND_ENABLE_ECC)
    {
        nLLDRe = _CorrectECCErr(nDev, nDie);
    }
#endif
    
    return nLLDRe;
}

#if !defined(FSR_LLD_ENABLE_CNTRLLER_ECC)
/**
 * @brief           This function corrects data
 *
 * @return          FSR_LLD_SUCCESS
 * @return          FSR_LLD_PREV_READ_ERROR
 * @return          FSR_LLD_PREV_2LV_READ_DISTURBANCE
 *
 * @author          JinHyuck Kim
 * @version         1.2.1
 *
 */
PRIVATE
INT32 _CorrectECCErr(UINT32 nDev,
                     UINT32 nDie)
{
    UINT32         nEccM;
    UINT16         nEccS;             
    INT32          nECCRet;
    UINT32         nCnt;    
    UINT8         *pMBuf;
    UINT8         *pSBuf;
    PureNANDCxt   *pstPNDCxt;
    PureNANDSpec  *pstPNDSpec;
    PureNANDShMem *pstPNDShMem;
    INT32          nLLDRe       = FSR_LLD_SUCCESS;
    
    pstPNDCxt   = gpstPNDCxt[nDev];
    pstPNDSpec  = pstPNDCxt->pstPNDSpec;
    pstPNDShMem = gpstPNDShMem[nDev];
    
    pMBuf = pstPNDShMem->pPseudoMainDataRAM;
    pSBuf = pstPNDShMem->pPseudoSpareDataRAM; 
    	    
	for(nCnt = 0; nCnt < pstPNDSpec->nSctsPerPg && (nLLDRe == FSR_LLD_SUCCESS); nCnt++)
	{
		nEccM = (UINT32)pSBuf[FSR_SPARE_SIZE * nCnt + OMAP2430_ECC_START_ADDRESS + 0]
			  | (UINT32)pSBuf[FSR_SPARE_SIZE * nCnt + OMAP2430_ECC_START_ADDRESS + 1] << 8
			  | (UINT32)pSBuf[FSR_SPARE_SIZE * nCnt + OMAP2430_ECC_START_ADDRESS + 2] << 16;			

		nECCRet = _ECC_CompM((UINT8 *)&nEccM, pMBuf + FSR_SECTOR_SIZE * nCnt, nCnt);
		
		switch (nECCRet)
		{
		    case SWECC_N_ERROR:		    
		        /* no op */
		        break;
		    case SWECC_R_ERROR:
		    case SWECC_C_ERROR:		    
		    case SWECC_E_ERROR:
		        nLLDRe = FSR_LLD_PREV_2LV_READ_DISTURBANCE |
		                 FSR_LLD_1STPLN_CURR_ERROR;
		        break;
		    case SWECC_U_ERROR:		    
		        nLLDRe = FSR_LLD_PREV_READ_ERROR | 
		                 FSR_LLD_1STPLN_CURR_ERROR;
		        break;
		}
	}    	

	for(nCnt = 0; nCnt < pstPNDSpec->nSctsPerPg && (nLLDRe == FSR_LLD_SUCCESS); nCnt++)
	{
		nEccS = (UINT16)pSBuf[FSR_SPARE_SIZE * nCnt + OMAP2430_ECC_START_ADDRESS + 3]
			  | (UINT16)pSBuf[FSR_SPARE_SIZE * nCnt + OMAP2430_ECC_START_ADDRESS + 4] << 8;

		nECCRet = _ECC_CompS((UINT8 *)&nEccS, pSBuf + FSR_SPARE_SIZE * nCnt, nCnt);    

		switch (nECCRet)
		{		    
		    case SWECC_N_ERROR:
		        /* no op */
		        break;
		    case SWECC_R_ERROR:
		    case SWECC_C_ERROR:
		    case SWECC_E_ERROR:
		        nLLDRe = FSR_LLD_PREV_2LV_READ_DISTURBANCE |
		                 FSR_LLD_1STPLN_CURR_ERROR;
		        break;		    
		    case SWECC_U_ERROR:		    
		        nLLDRe = FSR_LLD_PREV_READ_ERROR | 
		                 FSR_LLD_1STPLN_CURR_ERROR;
		        break;
		}
	}
		
	return (nLLDRe);
}
#endif

/**
 * @brief           This function writes data to NAND page buffer
 *
 * @return          None
 *
 * @author          JinHo Yi
 * @version         1.2.1
 *
 */
PRIVATE 
VOID _WriteToPageBuffer(UINT32     nDev,
                        UINT32     nDie)
{
                PureNANDCxt    *pstPNDCxt;
                PureNANDSpec   *pstPNDSpec;
                PureNANDShMem  *pstPNDShMem;
    volatile    UINT16         *pMBuf16;
    volatile    UINT16         *pSBuf16;
    volatile    UINT16         *pTgt16;
    volatile    UINT8          *pTgt8;
                UINT8          *pMBuf;
                UINT8          *pSBuf;    
                UINT32	        nCnt;
                UINT32          nCS;                
                UINT32          nNumOfTransfer;
    volatile    UINT32          nTmp            = 0;                
    
    FSR_STACK_VAR;

    FSR_STACK_END;
    
	nCS = nDev;

    pstPNDCxt   = gpstPNDCxt[nDev];
    pstPNDSpec  = pstPNDCxt->pstPNDSpec;
    pstPNDShMem = gpstPNDShMem[nDev];
    
    pMBuf = pstPNDShMem->pPseudoMainDataRAM; 
    pSBuf = pstPNDShMem->pPseudoSpareDataRAM;
    
#if !defined(FSR_LLD_ENABLE_CTRLLER_ECC)
    if(pstPNDCxt->nECCOption == FSR_PND_ENABLE_ECC)
    {
        _GenerateECC(nDev, nDie);
    }
#endif
        
    if(pstPNDSpec->nBWidth == 16)
    {
        pTgt16  = (volatile UINT16 *)GPMC_NAND_DATA_(nCS);    
        
        nNumOfTransfer = pstPNDSpec->nSctsPerPg * FSR_SECTOR_SIZE / sizeof(UINT16) ;
        
        pMBuf16 = (volatile UINT16 *)pMBuf;
        pSBuf16 = (volatile UINT16 *)pSBuf;
        
        for (nCnt = nNumOfTransfer; nCnt > 0; nCnt--)
        {            
            *pTgt16 = *pMBuf16++;
            nTmp = 1;
            while(nTmp--);
        }
        
        nNumOfTransfer = pstPNDSpec->nSctsPerPg * FSR_SPARE_SIZE / sizeof(UINT16);
        
        for (nCnt = nNumOfTransfer; nCnt > 0; nCnt--)
        {            
            *pTgt16 = *pSBuf16++;
        }
    }
    else if(pstPNDSpec->nBWidth == 8)
    {
        pTgt8 = (volatile UINT8 *)GPMC_NAND_DATA_(nCS);
        
        nNumOfTransfer = pstPNDSpec->nSctsPerPg * FSR_SECTOR_SIZE / sizeof(UINT8);
        
        for (nCnt = nNumOfTransfer; nCnt > 0; nCnt--)
        {
            *pTgt8 = *pMBuf++;            
            nTmp = 1;
            while(nTmp--);
        }
        
        nNumOfTransfer = pstPNDSpec->nSctsPerPg * FSR_SPARE_SIZE / sizeof(UINT8);
        
        for (nCnt = nNumOfTransfer; nCnt > 0; nCnt--)
        {
            *pTgt8 = *pSBuf++;            
        }
    }

    GPMC_NAND_COMMAND_(nCS) = OMAP2430_CMD_PAGE_PROGRAM_CONFIRM;
}

#if !defined(FSR_LLD_ENABLE_CNTRLLER_ECC)
/**
 * @brief           This function is used for generating SW ECC for PureNAND
 *
 * @return          None
 *
 * @author          JinHo Yi
 * @version         1.2.1
 *
 */
PRIVATE
VOID   _GenerateECC(UINT32 nDev,
                    UINT32 nDie)
{
    PureNANDCxt    *pstPNDCxt;
    PureNANDSpec   *pstPNDSpec;
    PureNANDShMem  *pstPNDShMem;  
    UINT8          *pMBuf;
    UINT8          *pSBuf;    
    UINT32	        nCnt;                
    UINT32	        nEccM;
    UINT16	        nEccS;    
    
    FSR_STACK_VAR;

    FSR_STACK_END;

    pstPNDCxt   = gpstPNDCxt[nDev];
    pstPNDSpec  = pstPNDCxt->pstPNDSpec;
    pstPNDShMem = gpstPNDShMem[nDev];
    
    pMBuf = pstPNDShMem->pPseudoMainDataRAM; 
    pSBuf = pstPNDShMem->pPseudoSpareDataRAM;
    
	for(nCnt = 0; nCnt < pstPNDSpec->nSctsPerPg; nCnt++)
	{
		_ECC_GenM((UINT8 *)&nEccM, (UINT32 *)(pMBuf + FSR_SECTOR_SIZE * nCnt));    

		pSBuf[FSR_SPARE_SIZE * nCnt + OMAP2430_ECC_START_ADDRESS + 0] = nEccM & 0xFF;
		pSBuf[FSR_SPARE_SIZE * nCnt + OMAP2430_ECC_START_ADDRESS + 1] = (nEccM >> 8) & 0xFF;
		pSBuf[FSR_SPARE_SIZE * nCnt + OMAP2430_ECC_START_ADDRESS + 2] = (nEccM >> 16) & 0xFF;		
	}

	for(nCnt = 0; nCnt < pstPNDSpec->nSctsPerPg; nCnt++)
	{
		_ECC_GenS((UINT16 *)&nEccS, pSBuf + FSR_SPARE_SIZE * nCnt);    

		pSBuf[FSR_SPARE_SIZE * nCnt + OMAP2430_ECC_START_ADDRESS + 3] = nEccS & 0xFF;
		pSBuf[FSR_SPARE_SIZE * nCnt + OMAP2430_ECC_START_ADDRESS + 4] = (nEccS >> 8) & 0xFF;
	}	
}
#endif
           
/**
 * @brief           This function is used for pure NAND
 *                  this function set address and command for NAND flash
 *
 * @return          FSR_PAM_SUCCESS
 *
 * @author          JinHo Yi
 * @version         1.2.1
 *
 */
PRIVATE  VOID
_SetNANDCtrllerCmd (UINT32 nDev,
                    UINT32 nDie,
                    UINT32 nRowAddress,
                    UINT32 nColAddress,
                    UINT32 nCommand)
{

#if defined(_FSR_LLD_OMAP2430_34xx_)
	UINT32	nCS;
	UINT32  nCycle;
    
    nCS = nDev;
    
    nCycle = gpstPNDCxt[nDev]->pstPNDSpec->n5CycleDev;
    
    switch (nCommand)
    {
        case FSR_PND_SET_READ_PAGE:
            GPMC_NAND_COMMAND_(nCS) = OMAP2430_CMD_READ;
			
			if(nRowAddress != INVALID_ADDRESS)
			{
            	GPMC_NAND_ADDRESS_(nCS) = (UINT16)(nColAddress & 0x00FF);
            	GPMC_NAND_ADDRESS_(nCS) = (UINT16)((nColAddress >> 8) & 0x00FF);
            	GPMC_NAND_ADDRESS_(nCS) = (UINT16)(nRowAddress & 0x00FF);
            	GPMC_NAND_ADDRESS_(nCS) = (UINT16)((nRowAddress >> 8) & 0x00FF);
            	
            	if(nCycle == FSR_PND_5CYCLES)
            	{
                	GPMC_NAND_ADDRESS_(nCS) = (UINT16)((nRowAddress >> 16) & 0x00FF);
            	}

            	GPMC_NAND_COMMAND_(nCS) = OMAP2430_CMD_READ_CONFIRM;
			}
            break;

        case FSR_PND_SET_PROGRAM_PAGE:
            GPMC_NAND_COMMAND_(nCS) = OMAP2430_CMD_PAGE_PROGRAM;

            GPMC_NAND_ADDRESS_(nCS) = (UINT16)(nColAddress & 0x00FF);
            GPMC_NAND_ADDRESS_(nCS) = (UINT16)((nColAddress >> 8) & 0x00FF);
            GPMC_NAND_ADDRESS_(nCS) = (UINT16)(nRowAddress & 0x00FF);
            GPMC_NAND_ADDRESS_(nCS) = (UINT16)((nRowAddress >> 8) & 0x00FF);
            
            if(nCycle == FSR_PND_5CYCLES)
            {
                GPMC_NAND_ADDRESS_(nCS) = (UINT16)((nRowAddress >> 16) & 0x00FF);                
            }
            /* tADL is Min 100ns */
            break;

        case FSR_PND_SET_ERASE:
            GPMC_NAND_COMMAND_(nCS) = OMAP2430_CMD_BLOCK_ERASE;
            GPMC_NAND_ADDRESS_(nCS) = (UINT16)(nRowAddress & 0x00FF);
            GPMC_NAND_ADDRESS_(nCS) = (UINT16)((nRowAddress >> 8) & 0x00FF);
            
            if(nCycle == FSR_PND_5CYCLES)
            {
                GPMC_NAND_ADDRESS_(nCS) = (UINT16)((nRowAddress >> 16) & 0x00FF);                
            }
            
            GPMC_NAND_COMMAND_(nCS) = OMAP2430_CMD_BLOCK_ERASE_CONFIRM;
            break;
            
        case FSR_PND_SET_RANDOM_DATA_INPUT:
            GPMC_NAND_COMMAND_(nCS) = OMAP2430_CMD_RANDOM_DATA_INPUT;
            GPMC_NAND_ADDRESS_(nCS) = (UINT16)(nColAddress & 0x00FF);
            GPMC_NAND_ADDRESS_(nCS) = (UINT16)((nColAddress >> 8) & 0x00FF);
            /* tADL is Min 100ns */
            break;

        case FSR_PND_SET_RANDOM_DATA_OUTPUT:
            GPMC_NAND_COMMAND_(nCS) = OMAP2430_CMD_RANDOM_DATA_OUTPUT;

            GPMC_NAND_ADDRESS_(nCS) = (UINT16)(nColAddress & 0x00FF);
            GPMC_NAND_ADDRESS_(nCS) = (UINT16)((nColAddress >> 8) & 0x00FF);

            GPMC_NAND_COMMAND_(nCS) = OMAP2430_CMD_RANDOM_DATA_OUTPUT_CONFIRM;
            break;

        case FSR_PND_SET_RESET:
            GPMC_NAND_COMMAND_(nCS) = OMAP2430_CMD_RESET;
            break;        
            
        case FSR_PND_SET_READ_ID:
            GPMC_NAND_COMMAND_(nCS) = OMAP2430_CMD_READ_ID;            
            GPMC_NAND_ADDRESS_(nCS) = 0x00;
            break;
        
        case FSR_PND_SET_READ_PGM_STATUS:
        case FSR_PND_SET_READ_ERASE_STATUS:
        case FSR_PND_SET_READ_INTERRUPT_STATUS:
            GPMC_NAND_COMMAND_(nCS) = OMAP2430_CMD_READ_STATUS;
            break;
        
        case FSR_PND_SET_ECC_ON:            
            _SetECC(nDev, nDie, FSR_PND_ENABLE_ECC);
            break;
            
        case FSR_PND_SET_ECC_OFF:
            _SetECC(nDev, nDie, FSR_PND_DISABLE_ECC);
            break;
        
        default:
            break;
    }
#endif
}

#if !defined(FSR_LLD_ENABLE_CTRLLER_ECC)
/**
 * @brief          This function generates 3 byte ECC for 512 byte data. 
 *                 (Software ECC)
 *
 * @param[in]      pEcc     : The memory location for given ECC val 
 * @param[in]      pBuf     : The memory location for given data
 *
 * @return         None
 *
 * @author         JinHo Yi
 * @version        1.2.1
 * @remark
 *
 */
PRIVATE VOID
_ECC_GenM(UINT8 *pEcc, UINT32 *pBuf)
{
    UINT32  nCnt;
    UINT32  nTmp;
    UINT32  nEcc = 0;
    UINT32  nCol;
    UINT32  nRow = 0;
    UINT32  nCol4 = 0, nCol2 = 0, nCol1 = 0, nColT = 0;
    UINT32  nXorT = 0;
    UINT32 *pDat32 = pBuf;  

    FSR_STACK_VAR;

    FSR_STACK_END;
    
    for(nCnt = 0; nCnt < 16; nCnt++)
    {
        nCol = *pDat32++; 
        nTmp = *pDat32++; nCol ^= nTmp; nCol1 ^= nTmp;
        nTmp = *pDat32++; nCol ^= nTmp;                nCol2 ^= nTmp;
        nTmp = *pDat32++; nCol ^= nTmp; nCol1 ^= nTmp; nCol2 ^= nTmp;
        nTmp = *pDat32++; nCol ^= nTmp;                               nCol4 ^= nTmp;
        nTmp = *pDat32++; nCol ^= nTmp; nCol1 ^= nTmp;                nCol4 ^= nTmp;
        nTmp = *pDat32++; nCol ^= nTmp;                nCol2 ^= nTmp; nCol4 ^= nTmp;
        nTmp = *pDat32++; nCol ^= nTmp; nCol1 ^= nTmp; nCol2 ^= nTmp; nCol4 ^= nTmp;

        nColT ^= nCol;

        nTmp = (nCol  >> 16) ^ nCol;
        nTmp = (nTmp  >>  8) ^ nTmp;
        nTmp = (nTmp  >>  4) ^ nTmp;
        nTmp = ((nTmp >>  2) ^ nTmp) & 0x03;

        if ((nTmp == 0x01) || (nTmp == 0x02))
        {
            nRow  ^= nCnt;
            nXorT ^= 0x01;
        }
    }

    nTmp  = (nCol4 >> 16) ^ nCol4;              /********/
    nTmp  = (nTmp  >>  8) ^ nTmp;               /*      */
    nTmp  = (nTmp  <<  4) ^ nTmp;               /* p128 */
    nTmp  = (nTmp  <<  2) ^ nTmp;               /*      */
    nEcc |= ((nTmp <<  1) ^ nTmp) & 0x80;       /********/
                                                              
    nTmp  = (nCol2 >> 16) ^ nCol2;              /********/
    nTmp  = (nTmp  >>  8) ^ nTmp;               /*      */
    nTmp  = (nTmp  <<  4) ^ nTmp;               /* p64  */
    nTmp  = (nTmp  >>  2) ^ nTmp;               /*      */
    nEcc |= ((nTmp <<  1) ^ nTmp) & 0x20;       /********/
                                                              
    nTmp  = (nCol1 >> 16) ^ nCol1;              /********/
    nTmp  = (nTmp  >>  8) ^ nTmp;               /*      */
    nTmp  = (nTmp  >>  4) ^ nTmp;               /* p32  */
    nTmp  = (nTmp  <<  2) ^ nTmp;               /*      */
    nEcc |= ((nTmp <<  1) ^ nTmp) & 0x08;       /********/
                                                              
    nTmp  = (nColT & 0xFFFF0000);               /********/
    nTmp  = (nTmp  >> 16);                      /*      */
    nTmp  = (nTmp  >>  8) ^ nTmp;               /* p16  */
    nTmp  = (nTmp  >>  4) ^ nTmp;               /*      */
    nTmp  = (nTmp  >>  2) ^ nTmp;               /*      */
    nEcc |= ((nTmp <<  1) ^ nTmp) & 0x02;       /********/
                                                              
    nTmp  = (nColT & 0xFF00FF00);               /********/
    nTmp  = (nTmp  << 16) ^ nTmp;               /*      */
    nTmp  = (nTmp  >>  8);                      /*  p8  */
    nTmp  = (nTmp  <<  4) ^ nTmp;               /*      */
    nTmp  = (nTmp  <<  2) ^ nTmp;               /*      */
    nEcc |= ((nTmp <<  1) ^ nTmp) & 0x800000;   /********/
                                                              
    nTmp  = (nColT & 0xF0F0F0F0);               /********/
    nTmp  = (nTmp  << 16) ^ nTmp;               /*      */
    nTmp  = (nTmp  >>  8) ^ nTmp;               /*  p4  */
    nTmp  = (nTmp  >>  2) ^ nTmp;               /*      */
    nEcc |= ((nTmp <<  1) ^ nTmp) & 0x200000;   /********/
                                                              
    nTmp  = (nColT & 0xCCCCCCCC);               /********/
    nTmp  = (nTmp  << 16) ^ nTmp;               /*      */
    nTmp  = (nTmp  >>  8) ^ nTmp;               /*  p2  */
    nTmp  = (nTmp  >>  4) ^ nTmp;               /*      */
    nEcc |= ((nTmp <<  1) ^ nTmp) & 0x80000;    /********/
                                                              
    nTmp  = (nColT & 0xAAAAAAAA);               /********/
    nTmp  = (nTmp  << 16) ^ nTmp;               /*      */
    nTmp  = (nTmp  >>  8) ^ nTmp;               /*  p1  */
    nTmp  = (nTmp  >>  4) ^ nTmp;               /*      */
    nTmp  = (nTmp  >>  2) ^ nTmp;               /*      */
    nEcc |= (nTmp  & 0x20000);                  /********/
                                                              
    nEcc |= (nRow  & 0x01) <<  9;               /* p256 */
    nEcc |= (nRow  & 0x02) << 10;               /* p512 */
    nEcc |= (nRow  & 0x04) << 11;               /* p1024*/
    nEcc |= (nRow  & 0x08) << 12;               /* p2048*/

    if (nXorT)
    {
        nEcc |= (nEcc ^ 0x00AAAAAA) >> 1;
    }
    else
    {
        nEcc |= (nEcc >> 1);
    }

    nEcc = ~nEcc;

	*(pEcc + 2) = (UINT8)(((nEcc & 0x00F00000) >> 20) | (nEcc << 4));
	*(pEcc + 1) = (UINT8)(nEcc >> 12);
	*(pEcc + 0) = (UINT8)(nEcc >> 4);
    
}

/**
 * @brief          This function corrects and detects data bit error for main area data 
 *                 (512B size data)
 *
 * @param[in]     *pEcc2    : The memory location for given ECC val2 (Target) 
 * @param[in]     *pBuf     : The memory location for given data     (Target)
 * @param[in]      nCnt     : Sector number
 *
 * @return         SWECC_N_ERROR    : No error
 * @return         SWECC_E_ERROR    : Ecc Value error
 * @return         SWECC_C_ERROR    : Correctable Error (One bit data error)
 * @return         SWECC_U_ERROR    : Uncorrectable Error (More than Two bit data error)
 * @return         SWECC_R_ERROR    : Correctable Error (One bit data error) by Read Disturbance
 *
 * @author         JinHo Yi
 * @version        1.0.0
 * @remark
 *
 */
PRIVATE INT32
_ECC_CompM(UINT8 *pEcc2, UINT8 *pBuf, UINT32 nSectNum)
{
    UINT8  *pEcc1;
    UINT32  nEccComp = 0, nEccSum = 0;
    UINT32  nEBit    = 0;
    UINT32  nEByte   = 0;
    UINT32  nXorT1   = 0, nXorT2 = 0;
    UINT32  nCnt;
    INT32	nErr;
    
	UINT32 nEccFN901;
	UINT32 nEccFN902;
	UINT32 nEcc1;
	UINT32 nEcc2;
		
    FSR_STACK_VAR;
    
    FSR_STACK_END;

    if (pBuf == NULL)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                       (TEXT("[PND:ERR] ++%s : Invalid buffer pointer \r\n"), __FSR_FUNC__));

        return FSR_LLD_INVALID_PARAM;
    }
		
    _ECC_GenM ((UINT8 *)&nEccFN901, (UINT32 *)pBuf);     

    /* line up the ECC bit array */
    pEcc1     = (UINT8 *)&nEccFN901;        
	nEccFN902 = (pEcc2[2] << 16) | (pEcc2[1] << 8) | (pEcc2[0]);
		
	nEcc1 = ((nEccFN901 << 4) & 0x00FFFFF0) | ((nEccFN901 >> 20) & 0x0000000F);
	nEcc2 = ((nEccFN902 << 4) & 0x00FFFFF0) | ((nEccFN902 >> 20) & 0x0000000F);
		
    FSR_DBZ_DBGMOUT (FSR_DBZ_LLD_IF, 
                    (TEXT("[ECC:MSG] nEcc1 = %x nEccFN901 = %x \n"), nEcc1, nEccFN901));
    FSR_DBZ_DBGMOUT (FSR_DBZ_LLD_IF, 
                    (TEXT("[ECC:MSG] nEcc2 = %x nEccFN902 = %x \n"), nEcc2, nEccFN902));
                         
    if (nEcc1 == nEcc2)
    {
        return (SWECC_N_ERROR);
    }                         

	*(pEcc1 + 2) = (UINT8)(nEcc1 >> 16);
	*(pEcc1 + 1) = (UINT8)(nEcc1 >> 8);
	*(pEcc1 + 0) = (UINT8)(nEcc1);
	*(pEcc2 + 2) = (UINT8)(nEcc2 >> 16);
	*(pEcc2 + 1) = (UINT8)(nEcc2 >> 8);
	*(pEcc2 + 0) = (UINT8)(nEcc2);
	
    for (nCnt = 0; nCnt < 2; nCnt++)
    {
        nXorT1 ^= (((*pEcc1) >> nCnt) & 0x01);
        nXorT2 ^= (((*pEcc2) >> nCnt) & 0x01);
    }

    for (nCnt = 0; nCnt < 3; nCnt++)
    {
        nEccComp |= ((~pEcc1[nCnt] ^ ~pEcc2[nCnt]) << (nCnt * 8));
    }
    
    for (nCnt = 0; nCnt < 24; nCnt++) 
    {
        nEccSum += ((nEccComp >> nCnt) & 0x01);
    }
            
    switch (nEccSum) 
    {
        case 0 :
            FSR_DBZ_DBGMOUT (FSR_DBZ_LLD_IF, 
                             (TEXT("[ECC:MSG] No Error for Main\n")));
            return (SWECC_N_ERROR);

        case 1 :
            FSR_DBZ_RTLMOUT (FSR_DBZ_LLD_IF | FSR_DBZ_ERROR, 
                             (TEXT("[ECC:MSG] 1 bit error in ECC parity occurs. But data is correct.\r\n")));
            return (SWECC_E_ERROR);

        case 12 :
            if (nXorT1 != nXorT2)
            {
                FSR_DBZ_RTLMOUT (FSR_DBZ_LLD_IF, 
                                 (TEXT("[ECC:MSG] Correctable ECC Error Occurs for Main\n")));

                nEByte  = ((nEccComp >>  7) & 0x100) +
                          ((nEccComp >>  6) & 0x80) + ((nEccComp >>  5) & 0x40) +
                          ((nEccComp >>  4) & 0x20) + ((nEccComp >>  3) & 0x10) +
                          ((nEccComp >>  2) & 0x08) + ((nEccComp >>  1) & 0x04) +
                          (nEccComp & 0x02)         + ((nEccComp >> 23) & 0x01);
                nEBit   = (UINT8)(((nEccComp >> 19) & 0x04) +
                          ((nEccComp >> 18) & 0x02) + ((nEccComp >> 17) & 0x01));

                FSR_DBZ_RTLMOUT (FSR_DBZ_LLD_IF, 
                                 (TEXT("[ECC:MSG] ECC Position : %dth byte, %dth bit\n"), nEByte, nEBit));
                
                nErr = SWECC_C_ERROR;
                
                if (pBuf != NULL)
                {
                    FSR_DBZ_DBGMOUT (FSR_DBZ_LLD_IF, 
                                     (TEXT("[ECC:MSG]  Corrupted : 0x%02xth \n"), pBuf[nEByte]));

                    pBuf[nEByte] = (UINT8)(pBuf[nEByte] ^ (1 << nEBit));

                    FSR_DBZ_RTLMOUT (FSR_DBZ_LLD_IF, 
                                     (TEXT("[ECC:MSG]  Corrected : 0x%02xth \n"), pBuf[nEByte]));
                    
                    if ((pBuf[nEByte] & (1 << nEBit))!=0)
                    {
                    	nErr = SWECC_R_ERROR;
                        FSR_DBZ_RTLMOUT (FSR_DBZ_LLD_IF | FSR_DBZ_ERROR, 
                            (TEXT("[ECC:MSG]  %d sector : One bit error by read disturbance \r\n"), nSectNum));
                   	}
                }
                
                return nErr;
            }

        default :
            FSR_DBZ_RTLMOUT (FSR_DBZ_LLD_IF | FSR_DBZ_ERROR, 
                             (TEXT("[ECC:MSG]  Ecc Error : %d sector,  %s, %d \r\n"), nSectNum, __FSR_FUNC__, __LINE__));

            FSR_DBZ_RTLMOUT (FSR_DBZ_LLD_IF | FSR_DBZ_ERROR, 
                             (TEXT("[ECC:MSG]  Uncorrectable ECC Error Occurs for Main\n")));
            return (SWECC_U_ERROR);
    }
}



/**
 * @brief          This function generates 3 byte ECC for 6 byte data. 
 *                 (Software ECC)
 *
 * @param[in]      pEcc     : The memory location for given ECC val 
 * @param[in]      pBuf     : The memory location for given data
 *
 * @return         none
 *
 * @author         JeongWook Moon
 * @version        1.0.0
 * @remark
 *
 */
PRIVATE VOID
_ECC_GenS (volatile UINT16 *pEcc, 
                    UINT8  *pBuf)
{
    UINT32  nTmp;
    UINT32  nEcc  = 0;
    UINT32  nXorT = 0;
    UINT32  nP8   = 0;
    UINT32  nP16  = 0;
    UINT32  nP32  = 0;
    UINT8  *pDat8;

    FSR_STACK_VAR;

    FSR_STACK_END;

    pDat8 = pBuf;

    nXorT = (UINT8)(pDat8[0] ^ pDat8[1] ^ pDat8[2] ^ pDat8[3] ^ pDat8[4] ^ pDat8[5] ^ pDat8[6] ^ pDat8[7]);

    nP8   = (UINT8)(pDat8[1] ^ pDat8[3] ^ pDat8[5] ^ pDat8[7]);
    nP16  = (UINT8)(pDat8[2] ^ pDat8[3] ^ pDat8[6] ^ pDat8[7]);
    nP32  = (UINT8)(pDat8[4] ^ pDat8[5] ^ pDat8[6] ^ pDat8[7]);

    
    /*
     * <Generate ECC code of spare data about 6 byte>
     *
     * +-------+-------+-------+-------+-------+-------+-------+-------+-------+
     * |       | I/O 7 | I/O 6 | I/O 5 | I/O 4 | I/O 3 | I/O 2 | I/O 1 | I/O 0 |
     * +-------+-------+-------+-------+-------+-------+-------+-------+-------+
     * S_ECC 0 |  P8   |  P8'  |  P4   |  P4'  |  P2   |  P2'  |  P1   |  P1'  |
     * +-------+-------------------------------+-------+-------+-------+-------+
     * S_ECC 1 |            (reserved)         | P32   | P32'  | P16   | P16'  | 
     * +-------+-------------------------------+-------+-------+-------+-------+
     */
    /* 
     * Column bit set
     */
    nTmp  = (nP32  <<  4) ^ nP32;                   /********/
    nTmp  = (nTmp  <<  2) ^ nTmp;                   /* p32  */
    nTmp  = (nTmp  <<  1) ^ nTmp;                   /*      */
    nEcc |= (nTmp  <<  4) & 0x800;                  /********/

    nTmp  = (nP16  <<  4) ^ nP16;                   /********/
    nTmp  = (nTmp  <<  2) ^ nTmp;                   /* p16  */
    nTmp  = (nTmp  <<  1) ^ nTmp;                   /*      */
    nEcc |= (nTmp  <<  2) & 0x200;                  /********/

    nTmp  = (nP8   <<  4) ^ nP8;                    /********/
    nTmp  = (nTmp  <<  2) ^ nTmp;                   /*  p8  */
    nEcc |= ((nTmp <<  1) ^ nTmp) & 0x80;           /********/

    /* 
     * Row bit set 
     */
    nTmp  = (nXorT   & 0xF0);                       /********/
    nTmp ^= (nTmp    >> 2);                         /*  P4  */
    nEcc |= ((nTmp   << 1) ^ nTmp) & 0x20;          /********/

    nTmp  = (nXorT   & 0xCC);                       /********/
    nTmp ^= (nTmp    >> 4);                         /*  p2  */
    //nTmp  = (nTmp    << 2);                         /*      */
    nEcc |= ((nTmp   << 1) ^ nTmp) & 0x08;          /********/
                     
    nTmp  = (nXorT   & 0xAA);                       /********/
    nTmp ^= (nTmp    >> 4);                         /*  p1  */
    nTmp ^= (nTmp    >> 2);                         /*      */
    nEcc |= (nTmp    & 0x02);                       /********/


    nXorT ^= (nXorT  >> 4);                         /********/
    nXorT ^= (nXorT  >> 2);                         /*nXorT */
    nXorT  = (UINT8)(((nXorT >> 1) ^ nXorT) & 0x01);/********/

    if (nXorT)
    {
        nEcc |= (nEcc ^ 0xAAAA) >> 1;
    }
    else
    {
        nEcc |= nEcc >> 1;
    }

    /* It is policy of samsung elec. to save invert value */
    nEcc = (UINT16) (~nEcc | 0xF000);

    *(UINT16 *)pEcc = (UINT16)(nEcc);
}

/**
 * @brief          This function corrects and detects data bit error for spare area data 
 *                 (16B size data)
 *
 * @param[in]     *pEcc2    : The memory location for given ECC val2 (Target) 
 * @param[in]     *pBuf     : The memory location for given data     (Target)
 * @param[in]      nSectNum : Sector number
 *
 * @return         SWECC_N_ERROR    : No error
 * @return         SWECC_E_ERROR    : Ecc Value error
 * @return         SWECC_C_ERROR    : Correctable Error (One bit data error)
 * @return         SWECC_U_ERROR    : Uncorrectable Error (More than Two bit data error)
 * @return         SWECC_R_ERROR    : Correctable Error (One bit data error) by Read Disturbance
 *
 * @author         JeongWook Moon
 * @version        1.0.0
 * @remark
 *
 */
PUBLIC INT32 
_ECC_CompS (UINT8  *pEcc2, 
            UINT8  *pBuf,
            UINT32  nSectNum)
{
    UINT8  *pEcc1;
    UINT32  nEccComp = 0; 
    UINT32  nEccSum  = 0;
    UINT32  nEBit    = 0;
    UINT32  nEByte   = 0;
    UINT32  nXorT1   = 0;
    UINT32  nXorT2   = 0;
    INT32   nErr;
    
#if defined (LOOP_FOR_ECC)
    UINT32  nCnt;
#endif

    UINT16 nEccFN901;
    UINT16 nEccFN902;
    UINT16 nEcc1;
    UINT16 nEcc2;

    FSR_STACK_VAR;

    FSR_STACK_END;

    if (pBuf == NULL)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("[OND:ERR] ++%s : Invalid buffer pointer \r\n"), __FSR_FUNC__));

        return FSR_LLD_INVALID_PARAM;
    }

    _ECC_GenS (&nEccFN901, pBuf);

    /*
     * <ECC code of spare data about 6 byte>
     *
     * +-------+-------+-------+-------+-------+-------+-------+-------+-------+
     * |       | I/O 7 | I/O 6 | I/O 5 | I/O 4 | I/O 3 | I/O 2 | I/O 1 | I/O 0 |
     * +-------+-------+-------+-------+-------+-------+-------+-------+-------+
     * S_ECC 0 |  P8   |  P8'  |  P4   |  P4'  |  P2   |  P2'  |  P1   |  P1'  |
     * +-------+-------------------------------+-------+-------+-------+-------+
     * S_ECC 1 |            (reserved)         | P32   | P32'  | P16   | P16'  | 
     * +-------+-------------------------------+-------+-------+-------+-------+
     */
    /* line up the ECC bit array */
    pEcc1     = (UINT8 *) &nEccFN901;
    nEccFN902 = (pEcc2[1] << 8) | (pEcc2[0]);

    nEcc1 = nEccFN901 & 0x0FFF;
    nEcc2 = nEccFN902 & 0x0FFF;

    FSR_DBZ_DBGMOUT (FSR_DBZ_LLD_IF, 
                     (TEXT("[ECC:MSG] nEcc1 = %x nEccFN901 = %x \n"), nEcc1, nEccFN901));
    FSR_DBZ_DBGMOUT (FSR_DBZ_LLD_IF, 
                     (TEXT("[ECC:MSG] nEcc2 = %x nEccFN902 = %x \n"), nEcc2, nEccFN902));


    if (nEcc1 == nEcc2)
    {
        return (SWECC_N_ERROR);
    }

#if defined (LOOP_FOR_ECC)
    for (nCnt = 0; nCnt < 2; nCnt++)
    {
        nXorT1 ^= (((*pEcc1) >> nCnt) & 0x01);
        nXorT2 ^= (((*pEcc2) >> nCnt) & 0x01);
    }
#else
    nXorT1 ^= ((*pEcc1) & 0x01);
    nXorT1 ^= (((*pEcc1) >> 1) & 0x01);
    nXorT2 ^= ((*pEcc2) & 0x01);
    nXorT2 ^= (((*pEcc2) >> 1) & 0x01);
#endif

#if defined (LOOP_FOR_ECC)
    /* compare written data with read data */
    for (nCnt = 0; nCnt < 2; nCnt++)
    {
        nEccComp |= ((~pEcc1[nCnt] ^ ~pEcc2[nCnt]) << (nCnt * 8));
    }
#else
    nEccComp |=  (~pEcc1[0] ^ ~pEcc2[0]);
    nEccComp |= ((UINT16)(~pEcc1[1] ^ ~pEcc2[1]) << 8);
#endif

#if defined (LOOP_FOR_ECC)
    /* find the number of '1' */
    for(nCnt = 0; nCnt < 12; nCnt++) 
    {
        nEccSum += ((nEccComp >> nCnt) & 0x01);
    }
#else
    nEccSum += ( nEccComp        & 0x01);
    nEccSum += ((nEccComp >>  1) & 0x01);
    nEccSum += ((nEccComp >>  2) & 0x01);
    nEccSum += ((nEccComp >>  3) & 0x01);
    nEccSum += ((nEccComp >>  4) & 0x01);
    nEccSum += ((nEccComp >>  5) & 0x01);
    nEccSum += ((nEccComp >>  6) & 0x01);
    nEccSum += ((nEccComp >>  7) & 0x01);
    nEccSum += ((nEccComp >>  8) & 0x01);
    nEccSum += ((nEccComp >>  9) & 0x01);
    nEccSum += ((nEccComp >> 10) & 0x01);
    nEccSum += ((nEccComp >> 11) & 0x01);
#endif


    switch (nEccSum) 
    {
        case 0 :
            FSR_DBZ_DBGMOUT (FSR_DBZ_LLD_IF, 
                             (TEXT("[ECC:MSG] No Error for Spare\n")));
            return (SWECC_N_ERROR);

        case 1 :
            FSR_DBZ_RTLMOUT (FSR_DBZ_LLD_IF | FSR_DBZ_ERROR, 
                             (TEXT("[ECC:MSG] 1 bit error in ECC parity occurs. But data is correct.\r\n")));
            return (SWECC_E_ERROR);

        case 6 :
            if (nXorT1 != nXorT2)
            {
                FSR_DBZ_DBGMOUT (FSR_DBZ_LLD_IF, 
                                 (TEXT("[ECC:MSG] Correctable ECC Error Occurs for Spare\r\n")));

                nEByte  = (UINT8) ((nEccComp >> 9) & 0x04) + 
                          ((nEccComp >> 8) & 0x02) + ((nEccComp >>  7) & 0x01);
                nEBit   = (UINT8)(((nEccComp >>  3) & 0x04) +
                          ((nEccComp >>  2) & 0x02) + ((nEccComp >>  1) & 0x01));

                FSR_DBZ_DBGMOUT (FSR_DBZ_LLD_IF, 
                                 (TEXT("[ECC:MSG] ECC Position : %dth byte, %dth bit\r\n"), nEByte, nEBit));
                
                nErr = SWECC_C_ERROR;
                
                if (pBuf != NULL)
                {
                    FSR_DBZ_DBGMOUT (FSR_DBZ_LLD_IF, 
                                     (TEXT("[ECC:MSG]  Corrupted : 0x%02xth \r\n"), pBuf[nEByte]));

                    pBuf[nEByte]  = (UINT8)(pBuf[nEByte] ^ (1 << nEBit));

                    FSR_DBZ_DBGMOUT (FSR_DBZ_LLD_IF, 
                                     (TEXT("[ECC:MSG]  Corrected : 0x%02xth \r\n"), pBuf[nEByte]));
                    
                    if ((pBuf[nEByte] & (1 << nEBit))!=0)
                    {
                        nErr = SWECC_R_ERROR;
                        FSR_DBZ_DBGMOUT (FSR_DBZ_LLD_IF | FSR_DBZ_ERROR, 
                            (TEXT("[ECC:MSG]  %d sector : One bit error by read disturbance \r\n"), nSectNum));
                    }
                }
                return nErr;
            }
            else
            {
                FSR_DBZ_RTLMOUT (FSR_DBZ_LLD_IF | FSR_DBZ_ERROR, 
                             (TEXT("[ECC:MSG]  Ecc Error : %d sector,  %s, %d \r\n"), nSectNum, __FSR_FUNC__, __LINE__));

                FSR_DBZ_RTLMOUT (FSR_DBZ_LLD_IF | FSR_DBZ_ERROR, 
                                 (TEXT("[ECC:MSG]  Uncorrectable ECC Error Occurs for Spare\r\n")));
                return (SWECC_U_ERROR);
            }

        default :
            FSR_DBZ_RTLMOUT (FSR_DBZ_LLD_IF | FSR_DBZ_ERROR, 
                             (TEXT("[ECC:MSG]  Ecc Error : %d sector,  %s, %d \r\n"), nSectNum, __FSR_FUNC__, __LINE__));
        
            FSR_DBZ_RTLMOUT (FSR_DBZ_LLD_IF | FSR_DBZ_ERROR, 
                             (TEXT("[ECC:MSG]  Uncorrectable ECC Error Occurs for Spare\r\n")));
            return (SWECC_U_ERROR);
    }   
}
#endif


/**
 * @brief           This function initialize the structure for LLD statistics
 *
 * @return          FSR_LLD_SUCCESS
 *
 * @author          Jinhyuck Kim
 * @version         1.2.1
 * @remark
 *
 */
PUBLIC INT32
FSR_PND_InitLLDStat(VOID)
{    

#if defined (FSR_LLD_STATISTICS)
    PureNANDStat   *pstStat;
    UINT32          nVolIdx;
    UINT32          nDevIdx;
    UINT32          nDieIdx;
    
    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG, 
                   (TEXT("[PND:IN ] ++%s\r\n"), 
                   __FSR_FUNC__));
                   
    gnElapsedTime = 0;

    for (nVolIdx = 0; nVolIdx < FSR_MAX_VOLS; nVolIdx++)
    {
        for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVolIdx]; nDevIdx++)
        {
            pstStat = gpstPNDStat[nVolIdx * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx];

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
                pstStat->nPreCmdOption[nDieIdx] = FSR_PND_STAT_NORMAL_CMD;
                pstStat->nIntLowTime[nDieIdx]   = 0;
            }
        }
    }
                   
    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[PND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, FSR_LLD_SUCCESS));        
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
 * @author          Jinhyuck Kim
 * @version         1.2.1
 * @remark
 *
 */
PUBLIC INT32
FSR_PND_GetStat(FSRLLDStat    *pstStat)
{
#if defined (FSR_LLD_STATISTICS)
    PureNANDCxt    *pstPNDCxt;
    PureNANDSpec   *pstPNDSpec;
    PureNANDStat   *pstPNDStat;
    UINT32          nVolIdx;
    UINT32          nDevIdx;
    UINT32          nTotalTime;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG, 
                   (TEXT("[PND:IN ] ++%s\r\n"), 
                   __FSR_FUNC__));
                   
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
                pstPNDStat = gpstPNDStat[nVolIdx * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx];

                pstStat->nSLCPgms        += pstPNDStat->nNumOfSLCPgms;
                pstStat->nErases         += pstPNDStat->nNumOfErases;
                pstStat->nSLCLoads       += pstPNDStat->nNumOfSLCLoads;
                pstStat->nRdTrans        += pstPNDStat->nNumOfRdTrans;
                pstStat->nWrTrans        += pstPNDStat->nNumOfWrTrans;
                pstStat->nCacheBusy      += pstPNDStat->nNumOfCacheBusy;
                pstStat->nRdTransInBytes += pstPNDStat->nRdTransInBytes;
                pstStat->nWrTransInBytes += pstPNDStat->nWrTransInBytes;

                pstPNDCxt     = gpstPNDCxt[nDevIdx];
                pstPNDSpec    = pstPNDCxt->pstPNDSpec;

                nTotalTime   += pstStat->nSLCLoads          * pstPNDSpec->nSLCTLoadTime;
                nTotalTime   += pstStat->nSLCPgms           * pstPNDSpec->nSLCTProgTime;
                nTotalTime   += pstStat->nErases            * pstPNDSpec->nTEraseTime;

                /* pstPNDCxt->nRdTranferTime, pstPNDCxt->nWrTranferTime is time for transfering 2 bytes */
                nTotalTime   += pstStat->nRdTransInBytes    * pstPNDCxt->nRdTranferTime / 2 / 1000;
                nTotalTime   += pstStat->nWrTransInBytes    * pstPNDCxt->nWrTranferTime / 2 / 1000;
                nTotalTime   += pstStat->nCacheBusy         * FSR_PND_CACHE_BUSY_TIME;
            }
        }
    } while (0);
                   
    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[PND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, FSR_LLD_SUCCESS));
    return gnElapsedTime;
#else
    FSR_OAM_MEMSET(pstStat, 0x00, sizeof(FSRLLDStat));
    return 0;
#endif

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
 * @author          Jinhyuck Kim
 * @version         1.2.1
 * @remark
 *
 */
PUBLIC INT32
FSR_PND_GetNANDCtrllerInfo(UINT32             nDev,
                           LLDPlatformInfo   *pLLDPltInfo)
{
    INT32           nLLDRe = FSR_LLD_SUCCESS;
    PureNANDCxt    *pstPNDCxt;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG, 
                   (TEXT("[PND:IN ] ++%s(nDev:%d)\r\n"), 
                   __FSR_FUNC__, nDev));
                   
    do
    {
    
#if defined (FSR_LLD_STRICT_CHK)
        if (pLLDPltInfo == NULL)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[PND:ERR]   %s() / %d line\r\n"),
                __FSR_FUNC__, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            pLLDPltInfo is NULL\r\n")));

            nLLDRe = FSR_LLD_INVALID_PARAM;
            break;
        }

        /* Check device number */
        if (nDev >= FSR_PND_MAX_DEVS)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("[PND:ERR]   %s() / %d line\r\n"),
            __FSR_FUNC__, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("            Invalid Device Number (nDev = %d)\r\n"), nDev));

            nLLDRe = (FSR_LLD_INVALID_PARAM);
            break;
        }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */    

        pstPNDCxt = gpstPNDCxt[nDev];

#if defined (FSR_LLD_STRICT_CHK)
        /* Check Device Open Flag */
        if (pstPNDCxt->bOpen == FALSE32)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("[PND:ERR]   %s() / %d line\r\n"),
                __FSR_FUNC__, __LINE__));

            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("            Device is not opened\r\n")));

            nLLDRe = FSR_LLD_OPEN_FAILURE;
            break;
        }
#endif /* #if defined (FSR_LLD_STRICT_CHK) */

        FSR_OAM_MEMSET(pLLDPltInfo, 0x00, sizeof(LLDPlatformInfo));

        /* Type of Device : FSR_LLD_SLC_ONENAND */
        pLLDPltInfo->nType            = FSR_LLD_SLC_NAND;
        /* Address of command register          */
        pLLDPltInfo->nAddrOfCmdReg    = (UINT32) NULL;
        /* Address of address register          */
        pLLDPltInfo->nAddrOfAdrReg    = (UINT32) NULL;
        /* Address of register for reading ID   */
        pLLDPltInfo->nAddrOfReadIDReg = (UINT32) NULL;
        /* Address of status register           */
        pLLDPltInfo->nAddrOfStatusReg = (UINT32) NULL;
        /* Command of reading Device ID         */
        pLLDPltInfo->nCmdOfReadID     = (UINT32) OMAP2430_CMD_READ_ID;
        /* Command of read page                 */
        pLLDPltInfo->nCmdOfReadPage   = (UINT32) OMAP2430_CMD_READ;
        /* Command of read status               */
        pLLDPltInfo->nCmdOfReadStatus = (UINT32) OMAP2430_CMD_READ_STATUS;
        /* Mask value for Ready or Busy status  */
        pLLDPltInfo->nMaskOfRnB       = (UINT32) 0x40;
    } while(0);
    
    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_IF | FSR_DBZ_LLD_LOG,
        (TEXT("[PND:OUT] --%s() / nLLDRe : 0x%x\r\n"), __FSR_FUNC__, nLLDRe));
                   
    return nLLDRe;
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
             PureNANDShMem *pstPNDShMem;
    volatile PureNANDOpLog *pstPNDOpLog;

    UINT32 nLogHead;

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG ,
        (TEXT("[PND:IN ] ++%s(nDie: %d)\r\n"), __FSR_FUNC__, nDie));

    pstPNDShMem = gpstPNDShMem[nDev];

    pstPNDOpLog = &gstPNDOpLog[nDev];

    nLogHead = pstPNDOpLog->nLogHead;

    pstPNDOpLog->nLogOp       [nLogHead]  = pstPNDShMem->nPreOp[nDie];
    pstPNDOpLog->nLogPbn      [nLogHead]  = pstPNDShMem->nPreOpPbn[nDie];
    pstPNDOpLog->nLogPgOffset [nLogHead]  = pstPNDShMem->nPreOpPgOffset[nDie];
    pstPNDOpLog->nLogFlag     [nLogHead]  = pstPNDShMem->nPreOpFlag[nDie];

    pstPNDOpLog->nLogHead = (nLogHead + 1) & (FSR_PND_MAX_LOG -1);

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
        (TEXT("[PND:OUT] --%s()\r\n"), __FSR_FUNC__));
}
#endif /* #if defined (FSR_LLD_LOGGING_HISTORY) */



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
                PureNANDCxt        *pstPNDCxt       = NULL;
                PureNANDShMem      *pstPNDShMem     = NULL;
                UINT32              nDev;
                
#if defined(FSR_LLD_LOGGING_HISTORY)
    volatile    PureNANDOpLog      *pstPNDOpLog     = NULL;
                UINT32              nIdx;
                UINT32              nLogHead;
                UINT32              nPreOpIdx;
#endif

    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG | FSR_DBZ_ERROR,
        (TEXT("[PND:IN ] ++%s()\r\n\r\n"), __FSR_FUNC__));

    for (nDev = 0; nDev < FSR_PND_MAX_DEVS; nDev++)
    {
        pstPNDCxt  = gpstPNDCxt[nDev];

        if (pstPNDCxt == NULL)
        {
            continue;
        }

        if (pstPNDCxt->bOpen == FALSE32)
        {
            continue;
        }

        pstPNDShMem = gpstPNDShMem[nDev];

        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR | FSR_DBZ_LLD_INF,
            (TEXT("[PND:INF]   pstPNDCxt->nFlushOpCaller : %d\r\n"),
            pstPNDCxt->nFlushOpCaller));

#if defined (FSR_LLD_LOGGING_HISTORY)
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("[PND:INF]   start printing nLog      : nDev[%d]\r\n"), nDev));

        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("%5s  %7s  %10s  %10s  %10s\r\n"), TEXT("nLog"),
            TEXT("preOp"), TEXT("prePbn"), TEXT("prePg"), TEXT("preFlag")));

        pstPNDOpLog = &gstPNDOpLog[nDev];

        nLogHead = pstPNDOpLog->nLogHead;
        for (nIdx = 0; nIdx < FSR_PND_MAX_LOG; nIdx++)
        {
            nPreOpIdx = pstPNDOpLog->nLogOp[nLogHead];
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                (TEXT("%5d| %7s, 0x%08x, 0x%08x, 0x%08x\r\n"),
                nLogHead, gpszLogPreOp[nPreOpIdx], pstPNDOpLog->nLogPbn[nLogHead],
                pstPNDOpLog->nLogPgOffset[nLogHead], pstPNDOpLog->nLogFlag[nLogHead]));

            nLogHead = (nLogHead + 1) & (FSR_PND_MAX_LOG -1);
        }

        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("[PND:   ]   end   printing nLog      : nDev[%d]\r\n"), nDev));
#endif /* #if defined (FSR_LLD_LOGGING_HISTORY) */

        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("\r\n[PND:INF]   start printing PureNANDCxt: nDev[%d]\r\n"), nDev));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("            nPreOp           : [0x%08x, 0x%08x]\r\n"),
            pstPNDShMem->nPreOp[0], pstPNDShMem->nPreOp[1]));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("            nPreOpPbn        : [0x%08x, 0x%08x]\r\n"),
            pstPNDShMem->nPreOpPbn[0], pstPNDShMem->nPreOpPbn[1]));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("            nPreOpPgOffset   : [0x%08x, 0x%08x]\r\n"),
            pstPNDShMem->nPreOpPgOffset[0], pstPNDShMem->nPreOpPgOffset[1]));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("            nPreOpFlag       : [0x%08x, 0x%08x]\r\n"),
            pstPNDShMem->nPreOpFlag[0], pstPNDShMem->nPreOpFlag[1]));
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("            end   printing PureNANDCxt: nDev[%d]\r\n\r\n"), nDev));
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG | FSR_DBZ_ERROR, (TEXT("[PND:OUT] --%s()\r\n"), __FSR_FUNC__));
#endif /* #if !defined(FSR_OAM_RTLMSG_DISABLE) */
}


/**
 * @brief           This function prints pseudo spare DATARAM
 *
 * @param[in]       nDev        : Physical Device Number (0 ~ 3) 
 *
 * @return          none
 *
 * @author          Jinhyuck Kim
 * @version         1.2.1
 * @remark
 *
 */
PRIVATE VOID
_DumpSpareBuffer(UINT32 nDev)
{
#if !defined(FSR_OAM_RTLMSG_DISABLE)
    UINT32          nSctIdx;
    UINT32          nIdx;    
    UINT16          nValue;
    PureNANDCxt    *pstPNDCxt;
    PureNANDShMem  *pstPNDShMem;
    UINT8          *pSpareDataRAM;
    UINT32          nSpareSizePerSct;
    UINT32          nDieIdx;
    
    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
    (TEXT("[PND:IN ] ++%s(nDev:%d)\r\n"),
    __FSR_FUNC__, nDev));
    
    pstPNDCxt        = gpstPNDCxt[nDev];
    pstPNDShMem      = gpstPNDShMem[nDev];
    pSpareDataRAM    = pstPNDShMem->pPseudoSpareDataRAM;
    nSpareSizePerSct = pstPNDCxt->pstPNDSpec->nSparePerSct;
        
    for (nDieIdx = 0; nDieIdx < pstPNDCxt->nNumOfDies; nDieIdx++)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("          Dump Spare Buffer [die:%d]\r\n"), nDieIdx));
        for (nSctIdx = 0; nSctIdx < pstPNDCxt->pstPNDSpec->nSctsPerPg; nSctIdx++)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("[%02d] "), nSctIdx));
            for (nIdx = 0; nIdx < nSpareSizePerSct / sizeof(UINT16); nIdx++)
            {
                nValue = *((UINT16 *)(pSpareDataRAM + nSctIdx * nSpareSizePerSct)
                            + nIdx);
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("%04x "), nValue));
            }
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("\r\n")));
        }
    }
    
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("\r\n")));

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[PND:OUT] --%s()\r\n"), __FSR_FUNC__));
#endif

}


/**
 * @brief           This function prints pseudo main DATARAM
 *
 * @param[in]       nDev        : Physical Device Number (0 ~ 3) 
 *
 * @return          none
 *
 * @author          Jinhyuck Kim
 * @version         1.2.1
 * @remark
 *
 */
PRIVATE VOID
_DumpMainBuffer(UINT32 nDev)
{
#if !defined(FSR_OAM_RTLMSG_DISABLE)
    UINT32          nDataIdx;
    UINT32          nIdx;    
    UINT16          nValue;
    PureNANDCxt    *pstPNDCxt;
    PureNANDShMem  *pstPNDShMem;
    UINT8          *pMainDataRAM;    
    UINT32          nDieIdx;
    UINT32          nPageSize;
    
    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG,
    (TEXT("[PND:IN ] ++%s(nDev:%d)\r\n"),
    __FSR_FUNC__, nDev));
    
    pstPNDCxt        = gpstPNDCxt[nDev];
    pstPNDShMem      = gpstPNDShMem[nDev];
    pMainDataRAM     = pstPNDShMem->pPseudoMainDataRAM;
    nPageSize        = pstPNDCxt->pstPNDSpec->nSctsPerPg * FSR_SECTOR_SIZE;    
        
    for (nDieIdx = 0; nDieIdx < pstPNDCxt->nNumOfDies; nDieIdx++)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("          Dump Main Buffer [die:%d]\r\n"), nDieIdx));
        for (nDataIdx = 0; nDataIdx < nPageSize / (16 * sizeof(UINT16)); nDataIdx++)
        {
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("%04x: "), nDataIdx * 16));
            for (nIdx = 0; nIdx < 16; nIdx++)
            {
                nValue = *((UINT16 *)(pMainDataRAM) 
                            + nDataIdx * 16 + nIdx);
                FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
                    (TEXT("%04x "), nValue));
            }
            FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("\r\n")));
        }
    }
    
    FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR, (TEXT("\r\n")));

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[PND:OUT] --%s()\r\n"), __FSR_FUNC__));
#endif

}



#if defined (FSR_LLD_STATISTICS)
/**
 * @brief          This function is called within the LLD function
 *                 to total the busy time of the device
 *
 * @param[in]      nDev         : Physical Device Number (0 ~ 7)
 * @param[in]      nDieNum      : Physical Die  Number
 * @param[in]      nType        : FSR_PND_STAT_SLC_PGM
 *                                FSR_PND_STAT_LSB_PGM
 *                                FSR_PND_STAT_MSB_PGM
 *                                FSR_PND_STAT_ERASE
 *                                FSR_PND_STAT_LOAD
 *                                FSR_PND_STAT_RD_TRANS
 *                                FSR_PND_STAT_WR_TRANS
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
_AddPNDStat(UINT32  nDevNum,
            UINT32  nDieNum,
            UINT32  nType,
            UINT32  nBytes,
            UINT32  nCmdOption)
{
    PureNANDCxt  *pstPNDCxt  = gpstPNDCxt[nDevNum];
    PureNANDSpec *pstPNDSpec = pstPNDCxt->pstPNDSpec;
    PureNANDStat *pstPNDStat = gpstPNDStat[nDevNum];
    
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

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[PND:IN ] ++%s\r\n"), __FSR_FUNC__));

    pstPNDStat  = gpstPNDStat[nDevNum];
    nIntLowTime = pstPNDStat->nIntLowTime[nDieNum];

    switch (nType)
    {
    case FSR_PND_STAT_SLC_PGM:
        /* Add the number of times of SLC program */
        pstPNDStat->nNumOfSLCPgms++;
        
        if (nIntLowTime > 0)
        {
            /* Wait INT for previous operation */
            gnElapsedTime += nIntLowTime;
            
            for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
            {
                for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVol]; nDevIdx++)
                {
                    nPDevIdx   = nVol * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx;
                    nNumOfDies = gpstPNDCxt[nPDevIdx]->nNumOfDies;

                    for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                    {
                        if (gpstPNDStat[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                        {
                            gpstPNDStat[nPDevIdx]->nIntLowTime[nDieIdx] -= nIntLowTime;
                        }
                    }
                }
            }
        }

        /* 
         * If nCmdOption is CACHE, transfering time can be hided
         * store & use it later 
         */
        pstPNDStat->nPreCmdOption[nDieNum] = nCmdOption;
        pstPNDStat->nIntLowTime[nDieNum]   = FSR_PND_WR_SW_OH + pstPNDSpec->nSLCTProgTime;

        if (nCmdOption == FSR_PND_STAT_CACHE_PGM)
        {
            pstPNDStat->nNumOfCacheBusy++;
        }
        break;

    case FSR_PND_STAT_ERASE:
        pstPNDStat->nNumOfErases++;
        
        if (nIntLowTime > 0)
        {
            /* Wait INT for previous operation */
            gnElapsedTime += nIntLowTime;
            
            for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
            {
                for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVol]; nDevIdx++)
                {
                    nPDevIdx   = nVol * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx;
                    nNumOfDies = gpstPNDCxt[nPDevIdx]->nNumOfDies;

                    for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                    {
                        if (gpstPNDStat[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                        {
                            gpstPNDStat[nPDevIdx]->nIntLowTime[nDieIdx] -= nIntLowTime;
                        }
                    }
                }
            }
        }

        pstPNDStat->nIntLowTime[nDieNum] = pstPNDSpec->nTEraseTime;
        break;

    case FSR_PND_STAT_SLC_LOAD:
        pstPNDStat->nNumOfSLCLoads++;

        if(nIntLowTime > 0)
        {
            /* Wait INT for previous operation */
            gnElapsedTime += nIntLowTime;
            
            for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
            {
                for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVol]; nDevIdx++)
                {
                    nPDevIdx   = nVol * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx;
                    nNumOfDies = gpstPNDCxt[nPDevIdx]->nNumOfDies;

                    for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                    {
                        if (gpstPNDStat[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                        {
                            gpstPNDStat[nPDevIdx]->nIntLowTime[nDieIdx] -= nIntLowTime;
                        }
                    }
                }
            }
        }
        pstPNDStat->nIntLowTime[nDieNum] = FSR_PND_RD_SW_OH + pstPNDSpec->nSLCTLoadTime;
        
        pstPNDStat->nPreCmdOption[nDieNum] = nCmdOption;
        break;

    case FSR_PND_STAT_RD_TRANS:
        pstPNDStat->nNumOfRdTrans++;
        pstPNDStat->nRdTransInBytes  += nBytes;
        nTransferTime = nBytes * pstPNDCxt->nRdTranferTime / 2 / 1000;

        if (nBytes > 0)
        {
            /* Add s/w overhead */
            nTransferTime += FSR_PND_RD_TRANS_SW_OH;
        }

        if ((nCmdOption != FSR_PND_STAT_PLOAD) && (nIntLowTime > 0))
        {
            gnElapsedTime += nIntLowTime;

            for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
            {
                for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVol]; nDevIdx++)
                {
                    nPDevIdx   = nVol * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx;
                    nNumOfDies = gpstPNDCxt[nPDevIdx]->nNumOfDies;

                    for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                    {
                        if (gpstPNDStat[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                        {
                            gpstPNDStat[nPDevIdx]->nIntLowTime[nDieIdx] -= /* sw overhead + */nIntLowTime;
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
                nNumOfDies = gpstPNDCxt[nPDevIdx]->nNumOfDies;

                for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                {
                    if (gpstPNDStat[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                    {
                        gpstPNDStat[nPDevIdx]->nIntLowTime[nDieIdx] -= nTransferTime;
                    }
                }
            }
        }

        break;

    case FSR_PND_STAT_WR_TRANS:
        pstPNDStat->nNumOfWrTrans++;
        pstPNDStat->nWrTransInBytes  += nBytes;
        nTransferTime = nBytes * pstPNDCxt->nWrTranferTime / 2 / 1000;

        /* Cache operation can hide transfer time */
        if((pstPNDStat->nPreCmdOption[nDieNum] == FSR_PND_STAT_CACHE_PGM) && (nIntLowTime >= 0))
        {
            gnElapsedTime  += nTransferTime;

            for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
            {
                for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVol]; nDevIdx++)
                {
                    nPDevIdx   = nVol * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx;
                    nNumOfDies = gpstPNDCxt[nPDevIdx]->nNumOfDies;

                    for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                    {
                        if (gpstPNDStat[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                        {
                            gpstPNDStat[nPDevIdx]->nIntLowTime[nDieIdx] -= nTransferTime;
                        }
                    }
                }
            }

            if(nIntLowTime < nTransferTime)
            {
                /* Only some of transfer time can be hided */
                pstPNDStat->nIntLowTime[nDieNum] = 0;
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
                        nNumOfDies = gpstPNDCxt[nPDevIdx]->nNumOfDies;

                        for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                        {
                            if (gpstPNDStat[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                            {
                                gpstPNDStat[nPDevIdx]->nIntLowTime[nDieIdx] -= nIntLowTime;
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
                    nNumOfDies = gpstPNDCxt[nPDevIdx]->nNumOfDies;

                    for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                    {
                        if (gpstPNDStat[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                        {
                            gpstPNDStat[nPDevIdx]->nIntLowTime[nDieIdx] -= nTransferTime;
                        }
                    }
                }
            }

            pstPNDStat->nIntLowTime[nDieNum] = 0;
        }
        break;

    case FSR_PND_STAT_FLUSH:
        if (nIntLowTime > 0)
        {
            /* Wait INT for previous operation */
            gnElapsedTime += nIntLowTime;
            
            for (nVol = 0; nVol < FSR_MAX_VOLS; nVol++)
            {
                for (nDevIdx = 0; nDevIdx < gnDevsInVol[nVol]; nDevIdx++)
                {
                    nPDevIdx   = nVol * FSR_MAX_DEVS / FSR_MAX_VOLS + nDevIdx;
                    nNumOfDies = gpstPNDCxt[nPDevIdx]->nNumOfDies;

                    for (nDieIdx = 0; nDieIdx < nNumOfDies; nDieIdx++)
                    {
                        if (gpstPNDStat[nPDevIdx]->nIntLowTime[nDieIdx] > 0)
                        {
                            gpstPNDStat[nPDevIdx]->nIntLowTime[nDieIdx] -= nIntLowTime;
                        }
                    }
                }
            }

            pstPNDStat->nIntLowTime[nDieNum] = 0;
        }
        break;

    default:
        FSR_ASSERT(0);
        break;
    }

    FSR_DBZ_DBGMOUT(FSR_DBZ_LLD_LOG, (TEXT("[PND:OUT] --%s\r\n"), __FSR_FUNC__));
}
#endif /* #if defined (FSR_LLD_STATISTICS) */
