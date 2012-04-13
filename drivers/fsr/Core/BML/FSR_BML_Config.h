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
 * @file      FSR_BML_Config.h
 * @brief     This file sets configuration parameters for FSR BML functions
 * @author    SuRyun Lee
 * @date      11-JAN-2007
 * @remark
 * REVISION HISTORY
 * @n  11-JAN-2007 [SuRyun Lee]  : first writing
 *
 */
#ifndef _FSR_BML_CONFIG_H_
#define _FSR_BML_CONFIG_H_

/****************************************************************************/
/* Common Constant definitions                                              */
/****************************************************************************/
/* OTP mode mark in LSN offset of spare of 1st page of 1st block */
#define     FSR_BML_REAL_OTP_MODE_MARK          (0xFF)
#define     FSR_BML_OTP_EMUL_MODE_MARK          (0x0F)

/* # of pages per semaphore acquire/release cycle */
#define     FSR_BML_PGS_PER_SM_CYCLE            (0x80)

/* Kernel Lock-up time for NonBlocking mode */
#define     FSR_BML_KERNEL_LOCKUP_TIME          (20)

/* number of blocks to be refreshed at open time */
#define     FSR_BML_MAX_PROCESSABLE_ERL_CNT     (16)

/****************************************************************************/
/* En-/Dis-able checking whether the given volume is valid or not.          */
/* If BML_CHK_VOLUME_VALIDATION is undefined,                               */
/* BML doesn't check the validation of the given volume.                    */
/****************************************************************************/
#if defined(FSR_NBL2)
#undef      BML_CHK_VOLUME_VALIDATION
#else
#define     BML_CHK_VOLUME_VALIDATION
#endif

/****************************************************************************/
/* En-/Dis-able checking whether the given page or block are valid or not.  */
/* If BML_CHK_PARAMETER_VALIDATION is undefined,                            */
/* BML doesn't check the validation of the given page or block.             */
/****************************************************************************/
#if defined(FSR_NBL2)
#undef      BML_CHK_PARAMETER_VALIDATION
#else
#define     BML_CHK_PARAMETER_VALIDATION
#endif
/****************************************************************************/
/* Before erase command is issued, wait until both of die is ready          */
/* in FSR_BML_Erase()                                                       */
/****************************************************************************/
#define     FSR_BML_WAIT_OPPOSITE_DIE

#endif /* _FSR_BML_CONFIG_H_ */
