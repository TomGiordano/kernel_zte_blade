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
 * @file        FSR_LLD_OMAP2430.h
 * @brief       This file contain the Platform Adaptation Modules for OMAP2430
 * @author      JinHo Yi, JinHyuck Kim
 * @date        15-SEP-2009
 * @remark
 * REVISION HISTORY
 * @n   28-JAN-2008 [JinHo Yi] 	   : First writing
 * @n   15-SEP-2009 [JinHyuck Kim] : Update for FSR LLD
 *
 */

#ifndef _FSR_LLD_OMAP2430_34xx_
#define _FSR_LLD_OMAP2430_34xx_



/*****************************************************************************/
/* NAND Controller  Masking value Definitions                                */
/*****************************************************************************/
#define GPMC_nCS0                           0
#define GPMC_nCS1                           1
#define GPMC_nCS2                           2
#define GPMC_nCS3                           3
#define GPMC_nCS4                           4
#define GPMC_nCS5                           5
#define GPMC_nCS6                           6
#define GPMC_nCS7                           7

#define WAITx_ACTIVE_HIGH                   0x1
#define NAND_FLASH_LIKE_DEVICES             0x1
#define WAIT_INPUT_PIN_IS_WAIT_(x)          (x)
#define NAND_FLASH_STATUS_BUSY_MASK_(x)     (1 << (x + 8))

/*****************************************************************************/
/* NAND Controller Register Address Definitions                              */
/*****************************************************************************/
#define GPMC_BASE               0x6E000000

#define GPMC_SYSCONFIG          *(volatile UINT32 *)(GPMC_BASE + 0x010)
#define GPMC_SYSSTATUS          *(volatile UINT32 *)(GPMC_BASE + 0x014)
#define GPMC_IRQSTATUS          *(volatile UINT32 *)(GPMC_BASE + 0x018)
#define GPMC_IRQENABLE          *(volatile UINT32 *)(GPMC_BASE + 0x01C)
#define GPMC_TIMEOUT_CONTROL    *(volatile UINT32 *)(GPMC_BASE + 0x040)
#define GPMC_ERR_ADDRESS        *(volatile UINT32 *)(GPMC_BASE + 0x044)
#define GPMC_ERR_TYPE           *(volatile UINT32 *)(GPMC_BASE + 0x048)
#define GPMC_CONFIG             *(volatile UINT32 *)(GPMC_BASE + 0x050)
#define GPMC_STATUS             *(volatile UINT32 *)(GPMC_BASE + 0x054)

#define GPMC_CONFIG1_(x)        *(volatile UINT32 *)(GPMC_BASE + 0x060 + (0x30 * x))
#define GPMC_CONFIG2_(x)        *(volatile UINT32 *)(GPMC_BASE + 0x064 + (0x30 * x))
#define GPMC_CONFIG3_(x)        *(volatile UINT32 *)(GPMC_BASE + 0x068 + (0x30 * x))
#define GPMC_CONFIG4_(x)        *(volatile UINT32 *)(GPMC_BASE + 0x06C + (0x30 * x))
#define GPMC_CONFIG5_(x)        *(volatile UINT32 *)(GPMC_BASE + 0x070 + (0x30 * x))
#define GPMC_CONFIG6_(x)        *(volatile UINT32 *)(GPMC_BASE + 0x074 + (0x30 * x))
#define GPMC_CONFIG7_(x)        *(volatile UINT32 *)(GPMC_BASE + 0x078 + (0x30 * x))

#define GPMC_NAND_COMMAND_(x)   *(volatile UINT16 *)(GPMC_BASE + 0x07C + (0x30 * x))
#define GPMC_NAND_ADDRESS_(x)   *(volatile UINT16 *)(GPMC_BASE + 0x080 + (0x30 * x))
#define GPMC_NAND_DATA_(x)      (GPMC_BASE + 0x084 + (0x30 * x))

#define GPMC_PREFETCH_CONFIG1   *(volatile UINT32 *)(GPMC_BASE + 0x1E0)
#define GPMC_PREFETCH_CONFIG2   *(volatile UINT32 *)(GPMC_BASE + 0x1E4)
#define GPMC_PREFETCH_CONTROL   *(volatile UINT32 *)(GPMC_BASE + 0x1EC)
#define GPMC_PREFETCH_STATUS    *(volatile UINT32 *)(GPMC_BASE + 0x1F0)

#define GPMC_ECC_CONFIG         *(volatile UINT32 *)(GPMC_BASE + 0x1F4)
#define GPMC_ECC_CONTROL        *(volatile UINT32 *)(GPMC_BASE + 0x1F8)
#define GPMC_ECC_SIZE_CONFIG    *(volatile UINT32 *)(GPMC_BASE + 0x1FC)

#ifdef  _OMAP3430_
#define GPMC_BCH_RESULT0_(x)    *(volatile UINT32 *)(GPMC_BASE + 0x240 + (0x10 * x))
#define GPMC_BCH_RESULT1_(x)    *(volatile UINT32 *)(GPMC_BASE + 0x244 + (0x10 * x))
#define GPMC_BCH_RESULT2_(x)    *(volatile UINT32 *)(GPMC_BASE + 0x248 + (0x10 * x))
#define GPMC_BCH_RESULT3_(x)    *(volatile UINT32 *)(GPMC_BASE + 0x24c + (0x10 * x))
#define GPMC_BCH_SWDATA         *(volatile UINT32 *)(GPMC_BASE + 0x2D0)
#endif
#endif /* #define _FSR_PAM_OMAP2430_34xx_ */

