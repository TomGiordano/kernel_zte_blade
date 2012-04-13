/**
 *   @mainpage   Flex Sector Remapper : LinuStoreIII_1.2.0_b035-FSR_1.2.1p1_b129_RC
 *
 *   @section Intro
 *       Flash Translation Layer for Flex-OneNAND and OneNAND
 *    
 *    @section  Copyright
 *            COPYRIGHT. 2007 - 2008 SAMSUNG ELECTRONICS CO., LTD.               
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
 * @file      FSR_PAM_ApollonPLUS.c
 * @brief     This file contain the Platform Adaptation Modules for ApollonPLUS
 * @author    SongHo Yoon
 * @date      15-MAR-2007
 * @remark
 * REVISION HISTORY
 * @n  15-MAR-2007 [SongHo Yoon] : first writing
 *
 */

#include "FSR.h"

/*****************************************************************************/
/* [PAM customization]                                                       */
/* The following 5 parameters can be customized                              */
/*                                                                           */
/* - FSR_ONENAND_PHY_BASE_ADDR                                               */
/* - FSR_ENABLE_WRITE_DMA                                                    */
/* - FSR_ENABLE_READ_DMA                                                     */
/* - FSR_ENABLE_FLEXOND_LFT                                                  */
/* - FSR_ENABLE_ONENAND_LFT                                                  */
/*                                                                           */
/*****************************************************************************/
/**< if FSR_ENABLE_FLEXOND_LFT is defined, 
     Low level function table is linked with Flex-OneNAND LLD */
#define     FSR_ENABLE_FLEXOND_LFT

/**< if FSR_ENABLE_ONENAND_LFT is defined, 
     Low level function table is linked with OneNAND LLD */
//#define     FSR_ENABLE_ONENAND_LFT /* daeho kim */


#if defined(FSR_WINCE_OAM)

    //#include <oal_memory.h>
    #include <omap2420_irq.h>

    #define     FSR_ONENAND_PHY_BASE_ADDR           ONENAND_BASEADDR
    //#define     FSR_ONENAND_VIR_BASE_ADDR           ((UINT32)OALPAtoVA(ONENAND_BASEADDR,FALSE))
    #define     FSR_ONENAND_VIR_BASE_ADDR           0xA0000000

    /**< if FSR_ENABLE_WRITE_DMA is defined, write DMA is enabled */
    #undef      FSR_ENABLE_WRITE_DMA
    /**< if FSR_ENABLE_READ_DMA is defined, read DMA is enabled */
    #undef      FSR_ENABLE_READ_DMA

    #define     IRQ_GPIO_72                         (IRQ_GPIO_64 + 8 )

#elif defined(FSR_SYMOS_OAM)

    #include <..\..\..\MD\Src\d_mednand.h>
    #include <Shared_gpio.h>

    #define     FSR_ONENAND_PHY_BASE_ADDR           (0x00000000)

    /**< if FSR_ENABLE_WRITE_DMA is defined, write DMA is enabled */
    #undef      FSR_ENABLE_WRITE_DMA
    /**< if FSR_ENABLE_READ_DMA is defined, read DMA is enabled */
    #undef      FSR_ENABLE_READ_DMA

    const TUint KNandGpioInterrupt = 72;

    class TNandGpioInterrupt
    	{
    public:
    	/** Bind to a Gpio Pin */
    	TInt Bind(TInt aPinId);
    
    	static void NandInterrupt(TAny* aPtr, TOmapGpioPinHandle aHandle);
    
    	inline void Enable(TBool aEnable=ETrue) {iPinWrapper->EnableInterruptNotif(aEnable);}
        inline void AckInterruptNotif() {iPinWrapper->AckInterruptNotif();}
    private:
    	TOmapGpioPinHandle iPinHandle;
    	TOmapGpioInputPinWrapper* iPinWrapper;
    	};
    
    extern DMediaDriverNand* pMDNand;
    TNandGpioInterrupt* pNandGpioInterrupt;

#elif defined(FSR_LINUX_OAM)

    #if defined(TINY_FSR)
        /* Setting at Kernel configuration */
        #define     FSR_ONENAND_PHY_BASE_ADDR       CONFIG_TINY_FLASH_PHYS_ADDR
    #else
        #define     FSR_ONENAND_PHY_BASE_ADDR       CONFIG_FSR_FLASH_PHYS_ADDR
    #endif

    /**< if FSR_ENABLE_WRITE_DMA is defined, write DMA is enabled */
    #undef      FSR_ENABLE_WRITE_DMA
    /**< if FSR_ENABLE_READ_DMA is defined, read DMA is enabled */
    #undef      FSR_ENABLE_READ_DMA

#else /* RTOS (such as Nucleus) or OSLess */

    #include <stdarg.h>
    #include <stdio.h>
    #include "OMAP2420.h"
    
    #define     FSR_ONENAND_PHY_BASE_ADDR           (0x00100000)

    /**< if FSR_ENABLE_WRITE_DMA is defined, write DMA is enabled */
    #define     FSR_ENABLE_WRITE_DMA
    /**< if FSR_ENABLE_READ_DMA is defined, read DMA is enabled */
    #define     FSR_ENABLE_READ_DMA

    #define     INT_GPIO3_IRQ                       (31)
    #define     GPIO_8_OFFSET                       (8)
    #define     GPIO_72                             (1 << GPIO_8_OFFSET)     // gpio 72 -> gpio 8 (64+8)

#endif

#if defined(FSR_ENABLE_FLEXOND_LFT)
    #include "FSR_LLD_FlexOND.h"
#endif

#if defined(FSR_ENABLE_ONENAND_LFT)
    #include "FSR_LLD_OneNAND.h"
#endif

/*****************************************************************************/
/* Global variables definitions                                              */
/*****************************************************************************/

/*****************************************************************************/
/* Local #defines                                                            */
/*****************************************************************************/
#define     ONENAND_DID_FLEX        (2)
#define     ONENAND_DID_SLC         (0)
#define     ONENAND_DID_MLC         (1)
#define     ONENAND_DIE_REG_OFFSET  (0x0001e002)

#define     DBG_PRINT(x)            FSR_DBG_PRINT(x)
#define     RTL_PRINT(x)            FSR_RTL_PRINT(x)


/*****************************************************************************/
/* Local typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Static variables definitions                                              */
/*****************************************************************************/

PRIVATE FsrVolParm              gstFsrVolParm[FSR_MAX_VOLS];
PRIVATE BOOL32                  gbPAMInit                   = FALSE32;
PRIVATE BOOL32                  gbFlexOneNAND[FSR_MAX_VOLS] = {FALSE32, FALSE32};
PRIVATE BOOL32                  gbUseWriteDMA               = FALSE32;
PRIVATE BOOL32                  gbUseReadDMA                = FALSE32;
#if defined(FSR_ENABLE_ONENAND_LFT)
PRIVATE volatile OneNANDReg     *gpOneNANDReg               = (volatile OneNANDReg *) 0;
#elif defined(FSR_ENABLE_FLEXOND_LFT)
PRIVATE volatile FlexOneNANDReg *gpOneNANDReg               = (volatile FlexOneNANDReg *) 0;
#else
#error  Either FSR_ENABLE_FLEXOND_LFT or FSR_ENABLE_ONENAND_LFT should be defined
#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern  VOID    memcpy32 (VOID       *pDst,
                          VOID       *pSrc,
                          UINT32     nSize);

#if defined(FSR_WINCE_OAM)
extern  UINT32  CheckMMU (VOID);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

/*****************************************************************************/
/* Function Implementation                                                   */
/*****************************************************************************/

/**
 * @brief           This function initializes PAM
 *                  this function is called by FSR_BML_Init
 *
 * @return          FSR_PAM_SUCCESS
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 *
 */
PUBLIC INT32
FSR_PAM_Init(VOID)
{
    INT32   nRe = FSR_PAM_SUCCESS;
    UINT32  nONDVirBaseAddr;
#if defined(CONFIG_FSR_DUAL_VOLUME)
    UINT32  nONDVirBaseAddr_1;
#endif
    FSR_STACK_VAR;

    FSR_STACK_END;

    if (gbPAMInit == TRUE32)
    {
        return FSR_PAM_SUCCESS;
    }
    gbPAMInit     = TRUE32;

    RTL_PRINT((TEXT("[PAM:   ] ++%s\r\n"), __FSR_FUNC__));

    do
    {
#if defined(FSR_ENABLE_WRITE_DMA)
        gbUseWriteDMA = TRUE32;
#else
        gbUseWriteDMA = FALSE32;
#endif

#if defined(FSR_ENABLE_READ_DMA)
        gbUseReadDMA  = TRUE32;
#else
        gbUseReadDMA  = FALSE32;
#endif

#if defined(FSR_WINCE_OAM)
        /* For WCE/WM, FSR_OAM_Pa2Va does nothing. */
        if (((UINT32)CheckMMU()) & 0x01)
        {
            /* MMU is on. */
            nONDVirBaseAddr  = FSR_ONENAND_VIR_BASE_ADDR;
            RTL_PRINT((TEXT("[PAM:   ]   MMU is Enabled\r\n")));
        }
        else
        {
            nONDVirBaseAddr  = FSR_ONENAND_PHY_BASE_ADDR;
            RTL_PRINT((TEXT("[PAM:   ]   MMU is Disabed\r\n")));
        }
#else
        nONDVirBaseAddr  = FSR_OAM_Pa2Va(FSR_ONENAND_PHY_BASE_ADDR);
#if defined(CONFIG_FSR_DUAL_VOLUME)
        nONDVirBaseAddr_1  = FSR_OAM_Pa2Va(CONFIG_FSR_FLASH_PHYS_ADDR2);
#endif
#endif

        RTL_PRINT((TEXT("[PAM:   ]   OneNAND physical base address       : 0x%08x\r\n"), FSR_ONENAND_PHY_BASE_ADDR));
        RTL_PRINT((TEXT("[PAM:   ]   OneNAND virtual  base address       : 0x%08x\r\n"), nONDVirBaseAddr));

#if defined(FSR_ENABLE_ONENAND_LFT)
        gpOneNANDReg  = (volatile OneNANDReg *) nONDVirBaseAddr;
#elif defined(FSR_ENABLE_FLEXOND_LFT)
        gpOneNANDReg  = (volatile FlexOneNANDReg *) nONDVirBaseAddr;
#endif

        /* check manufacturer ID */
        if (gpOneNANDReg->nMID != 0x00ec)
        {
            /* ERROR : OneNAND address space is not mapped with address space of ARM core */
            RTL_PRINT((TEXT("[PAM:ERR] OneNAND address space is not mapped with address space of ARM core\r\n")));
            nRe = FSR_PAM_NAND_PROBE_FAILED;
            break;
        }

        /* check whether current attached OneNAND is Flex-OneNAND or OneNAND */
        if (((gpOneNANDReg->nDID >> 8) & 0x03) == ONENAND_DID_FLEX)
        {
            gbFlexOneNAND[0] = TRUE32;
            gbFlexOneNAND[1] = TRUE32;

            RTL_PRINT((TEXT("[PAM:   ]   Flex-OneNAND nMID=0x%2x : nDID=0x%02x\r\n"), 
                    gpOneNANDReg->nMID, gpOneNANDReg->nDID));
        }
        else
        {
            gbFlexOneNAND[0] = FALSE32;
            gbFlexOneNAND[1] = FALSE32;

            RTL_PRINT((TEXT("[PAM:   ]   OneNAND nMID=0x%2x : nDID=0x%02x\r\n"), 
                    gpOneNANDReg->nMID, gpOneNANDReg->nDID));
        }

        gstFsrVolParm[0].nBaseAddr[0] = nONDVirBaseAddr;
        gstFsrVolParm[0].nBaseAddr[1] = FSR_PAM_NOT_MAPPED;
        gstFsrVolParm[0].nIntID[0]    = FSR_INT_ID_NAND_0;
        gstFsrVolParm[0].nIntID[1]    = FSR_INT_ID_NONE;
        gstFsrVolParm[0].nDevsInVol   = 1;
        gstFsrVolParm[0].pExInfo      = NULL;

#if defined(CONFIG_FSR_DUAL_VOLUME)
        gstFsrVolParm[1].nBaseAddr[0] = nONDVirBaseAddr_1;
        gstFsrVolParm[1].nBaseAddr[1] = FSR_PAM_NOT_MAPPED;
        gstFsrVolParm[1].nIntID[0]    = FSR_INT_ID_NAND_1;
        gstFsrVolParm[1].nIntID[1]    = FSR_INT_ID_NONE;
        gstFsrVolParm[1].nDevsInVol   = 1;
        gstFsrVolParm[1].pExInfo      = NULL;
#else
        gstFsrVolParm[1].nBaseAddr[0] = FSR_PAM_NOT_MAPPED;
        gstFsrVolParm[1].nBaseAddr[1] = FSR_PAM_NOT_MAPPED;
        gstFsrVolParm[1].nIntID[0]    = FSR_INT_ID_NONE;
        gstFsrVolParm[1].nIntID[1]    = FSR_INT_ID_NONE;
        gstFsrVolParm[1].nDevsInVol   = 0;
#endif
 

    } while (0);

    RTL_PRINT((TEXT("[PAM:   ] --%s\r\n"), __FSR_FUNC__));

    return nRe;
}

/**
 * @brief           This function returns FSR volume parameter
 *                  this function is called by FSR_BML_Init
 *
 * @param[in]       stVolParm[FSR_MAX_VOLS] : FsrVolParm data structure array
 *
 * @return          FSR_PAM_SUCCESS
 * @return          FSR_PAM_NOT_INITIALIZED
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 *
 */
PUBLIC INT32
FSR_PAM_GetPAParm(FsrVolParm stVolParm[FSR_MAX_VOLS])
{
    FSR_STACK_VAR;

    FSR_STACK_END;

    if (gbPAMInit == FALSE32)
    {
        return FSR_PAM_NOT_INITIALIZED;
    }

    FSR_OAM_MEMCPY(&(stVolParm[0]), &gstFsrVolParm[0], sizeof(FsrVolParm));
    FSR_OAM_MEMCPY(&(stVolParm[1]), &gstFsrVolParm[1], sizeof(FsrVolParm));

    return FSR_PAM_SUCCESS;
}

/**
 * @brief           This function registers LLD function table
 *                  this function is called by FSR_BML_Open
 *
 * @param[in]      *pstLFT[FSR_MAX_VOLS] : pointer to FSRLowFuncTable data structure
 *
 * @return          none
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 *
 */
PUBLIC INT32
FSR_PAM_RegLFT(FSRLowFuncTbl  *pstLFT[FSR_MAX_VOLS])
{
    UINT32  nVolIdx;
    FSR_STACK_VAR;

    FSR_STACK_END;

    if (gbPAMInit == FALSE32)
    {
        return FSR_PAM_NOT_INITIALIZED;
    }

    if (gstFsrVolParm[0].nDevsInVol > 0)
    {
        nVolIdx = 0;

        if (gbFlexOneNAND[0] == TRUE32)
        {
#if defined(FSR_ENABLE_FLEXOND_LFT)
            pstLFT[nVolIdx]->LLD_Init               = FSR_FND_Init;
            pstLFT[nVolIdx]->LLD_Open               = FSR_FND_Open;
            pstLFT[nVolIdx]->LLD_Close              = FSR_FND_Close;
            pstLFT[nVolIdx]->LLD_Erase              = FSR_FND_Erase;
            pstLFT[nVolIdx]->LLD_ChkBadBlk          = FSR_FND_ChkBadBlk;
            pstLFT[nVolIdx]->LLD_FlushOp            = FSR_FND_FlushOp;
            pstLFT[nVolIdx]->LLD_GetDevSpec         = FSR_FND_GetDevSpec;
            pstLFT[nVolIdx]->LLD_Read               = FSR_FND_Read;
            pstLFT[nVolIdx]->LLD_ReadOptimal        = FSR_FND_ReadOptimal;
            pstLFT[nVolIdx]->LLD_Write              = FSR_FND_Write;
            pstLFT[nVolIdx]->LLD_CopyBack           = FSR_FND_CopyBack;
            pstLFT[nVolIdx]->LLD_GetPrevOpData      = FSR_FND_GetPrevOpData;
            pstLFT[nVolIdx]->LLD_IOCtl              = FSR_FND_IOCtl;
            pstLFT[nVolIdx]->LLD_InitLLDStat        = FSR_FND_InitLLDStat;
            pstLFT[nVolIdx]->LLD_GetStat            = FSR_FND_GetStat;
            pstLFT[nVolIdx]->LLD_GetBlockInfo       = FSR_FND_GetBlockInfo;
            pstLFT[nVolIdx]->LLD_GetNANDCtrllerInfo = FSR_FND_GetPlatformInfo;
#else
            RTL_PRINT((TEXT("[PAM:ERR] LowFuncTbl(FlexOneNAND) isn't linked : %s / line %d\r\n"), __FSR_FUNC__, __LINE__));
            return FSR_PAM_LFT_NOT_LINKED;
#endif
        }
        else
        {
#if defined(FSR_ENABLE_ONENAND_LFT)
            pstLFT[nVolIdx]->LLD_Init               = FSR_OND_Init;
            pstLFT[nVolIdx]->LLD_Open               = FSR_OND_Open;
            pstLFT[nVolIdx]->LLD_Close              = FSR_OND_Close;
            pstLFT[nVolIdx]->LLD_Erase              = FSR_OND_Erase;
            pstLFT[nVolIdx]->LLD_ChkBadBlk          = FSR_OND_ChkBadBlk;
            pstLFT[nVolIdx]->LLD_FlushOp            = FSR_OND_FlushOp;
            pstLFT[nVolIdx]->LLD_GetDevSpec         = FSR_OND_GetDevSpec;
            pstLFT[nVolIdx]->LLD_Read               = FSR_OND_Read;
            pstLFT[nVolIdx]->LLD_ReadOptimal        = FSR_OND_ReadOptimal;
            pstLFT[nVolIdx]->LLD_Write              = FSR_OND_Write;
            pstLFT[nVolIdx]->LLD_CopyBack           = FSR_OND_CopyBack;
            pstLFT[nVolIdx]->LLD_GetPrevOpData      = FSR_OND_GetPrevOpData;
            pstLFT[nVolIdx]->LLD_IOCtl              = FSR_OND_IOCtl;
            pstLFT[nVolIdx]->LLD_InitLLDStat        = FSR_OND_InitLLDStat;
            pstLFT[nVolIdx]->LLD_GetStat            = FSR_OND_GetStat;
            pstLFT[nVolIdx]->LLD_GetBlockInfo       = FSR_OND_GetBlockInfo;
            pstLFT[nVolIdx]->LLD_GetNANDCtrllerInfo = FSR_OND_GetNANDCtrllerInfo;
#else
            RTL_PRINT((TEXT("[PAM:ERR] LowFuncTbl(OneNAND) isn't linked : %s / line %d\r\n"), __FSR_FUNC__, __LINE__));
            return FSR_PAM_LFT_NOT_LINKED;
#endif
        }
    }

    if (gstFsrVolParm[1].nDevsInVol > 0)
    {
        nVolIdx = 1;

        if (gbFlexOneNAND[1] == TRUE32)
        {
#if defined(FSR_ENABLE_FLEXOND_LFT)
            pstLFT[nVolIdx]->LLD_Init               = FSR_FND_Init;
            pstLFT[nVolIdx]->LLD_Open               = FSR_FND_Open;
            pstLFT[nVolIdx]->LLD_Close              = FSR_FND_Close;
            pstLFT[nVolIdx]->LLD_Erase              = FSR_FND_Erase;
            pstLFT[nVolIdx]->LLD_ChkBadBlk          = FSR_FND_ChkBadBlk;
            pstLFT[nVolIdx]->LLD_FlushOp            = FSR_FND_FlushOp;
            pstLFT[nVolIdx]->LLD_GetDevSpec         = FSR_FND_GetDevSpec;
            pstLFT[nVolIdx]->LLD_Read               = FSR_FND_Read;
            pstLFT[nVolIdx]->LLD_ReadOptimal        = FSR_FND_ReadOptimal;
            pstLFT[nVolIdx]->LLD_Write              = FSR_FND_Write;
            pstLFT[nVolIdx]->LLD_CopyBack           = FSR_FND_CopyBack;
            pstLFT[nVolIdx]->LLD_GetPrevOpData      = FSR_FND_GetPrevOpData;
            pstLFT[nVolIdx]->LLD_IOCtl              = FSR_FND_IOCtl;
            pstLFT[nVolIdx]->LLD_InitLLDStat        = FSR_FND_InitLLDStat;
            pstLFT[nVolIdx]->LLD_GetStat            = FSR_FND_GetStat;
            pstLFT[nVolIdx]->LLD_GetBlockInfo       = FSR_FND_GetBlockInfo;
            pstLFT[nVolIdx]->LLD_GetNANDCtrllerInfo = FSR_FND_GetPlatformInfo;
#else
            RTL_PRINT((TEXT("[PAM:ERR] LowFuncTbl(FlexOneNAND) isn't linked : %s / line %d\r\n"), __FSR_FUNC__, __LINE__));
            return FSR_PAM_LFT_NOT_LINKED;
#endif
        }
        else
        {   
#if defined(FSR_ENABLE_ONENAND_LFT)
            pstLFT[nVolIdx]->LLD_Init               = FSR_OND_Init;
            pstLFT[nVolIdx]->LLD_Open               = FSR_OND_Open;
            pstLFT[nVolIdx]->LLD_Close              = FSR_OND_Close;
            pstLFT[nVolIdx]->LLD_Erase              = FSR_OND_Erase;
            pstLFT[nVolIdx]->LLD_ChkBadBlk          = FSR_OND_ChkBadBlk;
            pstLFT[nVolIdx]->LLD_FlushOp            = FSR_OND_FlushOp;
            pstLFT[nVolIdx]->LLD_GetDevSpec         = FSR_OND_GetDevSpec;
            pstLFT[nVolIdx]->LLD_Read               = FSR_OND_Read;
            pstLFT[nVolIdx]->LLD_ReadOptimal        = FSR_OND_ReadOptimal;
            pstLFT[nVolIdx]->LLD_Write              = FSR_OND_Write;
            pstLFT[nVolIdx]->LLD_CopyBack           = FSR_OND_CopyBack;
            pstLFT[nVolIdx]->LLD_GetPrevOpData      = FSR_OND_GetPrevOpData;
            pstLFT[nVolIdx]->LLD_IOCtl              = FSR_OND_IOCtl;
            pstLFT[nVolIdx]->LLD_InitLLDStat        = FSR_OND_InitLLDStat;
            pstLFT[nVolIdx]->LLD_GetStat            = FSR_OND_GetStat;
            pstLFT[nVolIdx]->LLD_GetBlockInfo       = FSR_OND_GetBlockInfo;
            pstLFT[nVolIdx]->LLD_GetNANDCtrllerInfo = FSR_OND_GetNANDCtrllerInfo;
#else
            RTL_PRINT((TEXT("[PAM:ERR] LowFuncTbl(OneNAND) isn't linked : %s / line %d\r\n"), __FSR_FUNC__, __LINE__));
            return FSR_PAM_LFT_NOT_LINKED;
#endif
        }
    }

    return FSR_PAM_SUCCESS;
}

/**
 * @brief           This function transfers data to NAND
 *
 * @param[in]      *pDst  : Destination array Pointer to be copied
 * @param[in]      *pSrc  : Source data allocated Pointer
 * @param[in]      *nSize : length to be transferred
 *
 * @return          none
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 * @remark          pDst / pSrc address should be aligned by 4 bytes.
 *                  DO NOT USE memcpy of standard library 
 *                  memcpy (RVDS2.2) can do wrong memory copy operation.
 *                  memcpy32 is optimized by using multiple load/store instruction
 *
 */
PUBLIC VOID
FSR_PAM_TransToNAND(volatile VOID *pDst,
                    VOID          *pSrc,
                    UINT32        nSize)
{
    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_ASSERT(((UINT32) pSrc & 0x03) == 0x00000000);
    FSR_ASSERT(((UINT32) pDst & 0x03) == 0x00000000);
    FSR_ASSERT(nSize > sizeof(UINT32));

    if (nSize >= (FSR_SECTOR_SIZE / 2))
    {
        if (gbUseWriteDMA == TRUE32)
        {
            FSR_OAM_WriteDMA((UINT32) pDst, (UINT32) pSrc, nSize);
        }
        else
        {
            memcpy32((void *) pDst, pSrc, nSize);
        }
    }
    else
    {
        memcpy32((void *) pDst, pSrc, nSize);
    }
}

/**
 * @brief           This function transfers data from NAND
 *
 * @param[in]      *pDst  : Destination array Pointer to be copied
 * @param[in]      *pSrc  : Source data allocated Pointer
 * @param[in]      *nSize : length to be transferred
 *
 * @return          none
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 * @remark          pDst / pSrc address should be aligned by 4 bytes
 *                  DO NOT USE memcpy of standard library 
 *                  memcpy (RVDS2.2) can do wrong memory copy operation
 *                  memcpy32 is optimized by using multiple load/store instruction
 *
 */
VOID
FSR_PAM_TransFromNAND(VOID          *pDst,
                      volatile VOID *pSrc,
                      UINT32         nSize)
{
    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_ASSERT(((UINT32) pSrc & 0x03) == 0x00000000);
    FSR_ASSERT(((UINT32) pDst & 0x03) == 0x00000000);
    FSR_ASSERT(nSize > sizeof(UINT32));

    if (nSize >= (FSR_SECTOR_SIZE / 2))
    {
        if (gbUseReadDMA == TRUE32)
        {
            FSR_OAM_ReadDMA((UINT32) pDst, (UINT32) pSrc, nSize);
        }
        else
        {
            memcpy32(pDst, (void *) pSrc, nSize);
        }
    }
    else
    {
        memcpy32(pDst, (void *) pSrc, nSize);
    }
}

/**
 * @brief           This function initializes the specified logical interrupt.
 *
 * @param[in]       nLogIntId : Logical interrupt id
 * 
 * @return          FSR_PAM_SUCCESS
 *
 * @author          SongHo Yoon
 *
 * @version         1.0.0
 *
 * @remark          this function is used to support non-blocking I/O feature of FSR
 *
 */
PUBLIC INT32
FSR_PAM_InitInt(UINT32 nLogIntId)
{
    FSR_STACK_VAR;
    FSR_STACK_END;

    switch (nLogIntId)
    {
        case FSR_INT_ID_NAND_0:
#if defined(FSR_SYMOS_OAM)
            if(!pNandGpioInterrupt)
            {
                pNandGpioInterrupt = new TNandGpioInterrupt;
                pNandGpioInterrupt->Bind(KNandGpioInterrupt);
            }
#elif defined(FSR_WINCE_OAM)

#elif defined(FSR_LINUX_OAM)

#else /* for RTOS or OSLess*/

            *(volatile UINT32 *) GPIO3_OE            |= GPIO_72;
            /* rising edge */
            *(volatile UINT32 *) GPIO3_RISINGDETECT  |= GPIO_72;
            /* set IRQ enable */
            *(volatile UINT32 *) GPIO3_SETIRQENABLE1 |= GPIO_72;

#endif /* #if defined(FSR_SYMOS_OAM) */
            break;
        default:
            break;
    }

    return FSR_PAM_SUCCESS;
}

/**
 * @brief           This function deinitializes the specified logical interrupt.
 *
 * @param[in]       nLogIntId : Logical interrupt id
 * 
 * @return          FSR_PAM_SUCCESS
 *
 * @author          SongHo Yoon
 *
 * @version         1.0.0
 *
 * @remark          this function is used to support non-blocking I/O feature of FSR
 *
 */
PUBLIC INT32
FSR_PAM_DeinitInt(UINT32 nLogIntId)
{
    FSR_STACK_VAR;
    FSR_STACK_END;

    switch (nLogIntId)
    {
        case FSR_INT_ID_NAND_0:
#if defined(FSR_SYMOS_OAM)
            if (pNandGpioInterrupt != NULL)
            {
                delete pNandGpioInterrupt;
                pNandGpioInterrupt = NULL;
            }
#elif defined(FSR_WINCE_OAM)

#elif defined(FSR_LINUX_OAM)

#else /* for RTOS or OSLess*/
            /* clear irq enable */
            *(volatile UINT32 *) GPIO3_CLEARIRQENABLE1 |= GPIO_72;
#endif /* #if defined(FSR_SYMOS_OAM) */
            break;
        default:
            break;
    }

    return FSR_PAM_SUCCESS;
}

/**
 * @brief           This function returns the physical interrupt ID from the logical interrupt ID
 *
 * @param[in]       nLogIntID : Logical interrupt id
 *
 * @return          physical interrupt ID
 *
 * @author          SongHo Yoon
 *
 * @version         1.0.0
 *
 */
UINT32
FSR_PAM_GetPhyIntID(UINT32  nLogIntID)
{
    UINT32 nPhyIntID = 0;

    switch (nLogIntID)
    {
    case FSR_INT_ID_NAND_0:

#if defined(FSR_SYMOS_OAM)
        nPhyIntID = KNandGpioInterrupt;
#elif defined(FSR_WINCE_OAM)
        nPhyIntID = IRQ_GPIO_72;
#elif defined(FSR_LINUX_OAM)

#else /* for RTOS or OSLess*/
        nPhyIntID = INT_GPIO3_IRQ;
#endif
        break;
    default:
        break;
    }

    return nPhyIntID;
}

/**
 * @brief           This function enables the specified interrupt.
 *
 * @param[in]       nLogIntID : Logical interrupt id
 *
 * @return          FSR_PAM_SUCCESS
 *
 * @author          Seunghyun Han
 *
 * @version         1.0.0
 *
 */
INT32
FSR_PAM_ClrNEnableInt(UINT32 nLogIntID)
{
    switch (nLogIntID)
    {
    case FSR_INT_ID_NAND_0:

#if defined(FSR_SYMOS_OAM)
        pNandGpioInterrupt->AckInterruptNotif(); /* Interrupt Clear   */
        pNandGpioInterrupt->Enable();            /* Interrupt Enable  */
#elif defined(FSR_WINCE_OAM)

#elif defined(FSR_LINUX_OAM)

#else /* for RTOS or OSLess*/

#endif
        break;
    default:
        break;
    }

    return FSR_PAM_SUCCESS;
}

/**
 * @brief           This function disables the specified interrupt.
 *
 * @param[in]       nLogIntID : Logical interrupt id
 *
 * @return          FSR_PAM_SUCCESS
 *
 * @author          Seunghyun Han
 *
 * @version         1.0.0
 *
 */
INT32
FSR_PAM_ClrNDisableInt(UINT32 nLogIntID)
{
    switch (nLogIntID)
    {
    case FSR_INT_ID_NAND_0:

#if defined(FSR_SYMOS_OAM)
        pNandGpioInterrupt->AckInterruptNotif(); /* Interrupt Clear   */
        pNandGpioInterrupt->Enable(EFalse);      /* Interrupt Disable */
#elif defined(FSR_WINCE_OAM)

#elif defined(FSR_LINUX_OAM)

#else /* for RTOS or OSLess*/

#endif
        break;
    default:
        break;
    }

    return FSR_PAM_SUCCESS;
}


#if defined(FSR_SYMOS_OAM)
/**
 * @brief           This function is used only for SymOS.
 *
 * @param[in]       aPtr
 * @param[in]       aHandle
 *
 * @return          none
 *
 * @author          WooYoungYang
 *
 * @version         1.0.0
 *
 */
void TNandGpioInterrupt::NandInterrupt(TAny* aPtr, TOmapGpioPinHandle aHandle)
{
    FSR_PAM_ClrNDisableInt(FSR_INT_ID_NAND_0);
	pMDNand->iNandIreqDfc.Add();
}

/**
 * @brief           This function is used only for SymOS.
 *
 * @param[in]       aPinId
 *
 * @return          TInt
 *
 * @author          WooYoungYang
 *
 * @version         1.0.0
 *
 */
TInt TNandGpioInterrupt::Bind(TInt aPinId)
{
    iPinHandle = OmapGpioPinMgr::AcquirePin(aPinId);

    if (iPinHandle == KOmapGpioInvalidHandle)
        return KErrBadHandle;

	OmapGpioPinMgr::SetPinAsInput(iPinHandle);

    iPinWrapper = new TOmapGpioInputPinWrapper(iPinHandle);
    if (!iPinWrapper)
    {
        return KErrNoMemory;
    }

    // Configure the Intr on the falling edge
    iPinWrapper->SetEventDetection(EOmapGpioEDTEdge, EOmapGpioEDSUp);

    // Enable module wake-up on this pin (interrupt is also enabled)
    iPinWrapper->EnableWakeUpNotif();

    // Disable interrupt generation on the input pin
    iPinWrapper->EnableInterruptNotif(EFalse);

    return iPinWrapper->BindIsr(NandInterrupt, this);
}
#endif
