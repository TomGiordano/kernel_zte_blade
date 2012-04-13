/**
 *   @mainpage   Flex Sector Remapper : LinuStoreIII_1.2.0_b035-FSR_1.2.1p1_b129_RC
 *
 *   @section Intro
 *       Flash Translation Layer for Flex-OneNAND and OneNAND
 *    
 *    @section  Copyright
 *            COPYRIGHT. 2003-2007 SAMSUNG ELECTRONICS CO., LTD.               
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
 * @file      FSR_PAM_Memcpy.c
 * @brief     memcpy32
 * @author    SongHo Yoon
 * @date      21-NOV-2007
 * @remark
 * REVISION HISTORY
 * @n  21-NOV-2007 [SongHo Yoon] : first writing
 *
 */

#include "FSR.h"

/*****************************************************************************/
/* Function Implementation                                                   */
/*****************************************************************************/

/**
 * @brief           memcpy
 *
 * @return          none
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 *
 */
PUBLIC VOID
memcpy32 (VOID       *pDst,
          VOID       *pSrc,
          UINT32     nSize)
{
    UINT32   nIdx;
    UINT32  *pSrc32;
    UINT32  *pDst32;
    UINT32   nSize32;


    pSrc32  = (UINT32 *)(pSrc);
    pDst32  = (UINT32 *)(pDst);
    nSize32 = nSize / sizeof (UINT32);

    for(nIdx = 0; nIdx < nSize32; nIdx++)
    {
        pDst32[nIdx] = pSrc32[nIdx];
    }
}
