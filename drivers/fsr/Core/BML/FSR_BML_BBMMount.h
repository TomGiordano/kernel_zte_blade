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
 * @file      FSR_BML_BBMMount.h
 * @brief     This file contains the definition and prototypes of exported
 * @n         functions for Bad Block Manager 
 * @author    MinYoung Kim
 * @date      15-JAN-2007
 * @remark
 * REVISION HISTORY
 * @n  15-JAN-2007 [MinYoung Kim] : first writing
 *
 */ 

#ifndef     _FSR_BML_BBMMOUNT_H_
#define     _FSR_BML_BBMMOUNT_H_

/*****************************************************************************/
/* exported function prototype of Bad Block Manager                          */
/*****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

INT32  FSR_BBM_Mount                (BmlVolCxt   *pstVol,
                                     BmlDevCxt   *pstDev, 
                                     FSRPartI    *pstPI,
                                     FSRPIExt    *pstPExt);
INT32  FSR_BBM_UnlockRsvr           (BmlVolCxt   *pstVol,
                                     BmlDevCxt   *pstDev,
                                     UINT32       nDieIdx);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _FSR_BML_BBMMOUNT_H_ */
