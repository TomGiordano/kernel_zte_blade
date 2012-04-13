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
 * @file      FSR_LLD_4K_ONDShared.c
 * @brief     This file declares the shared variable for supporting the linux
 * @author    NamOh Hwang
 * @date      1-Aug-2007
 * @remark
 * REVISION HISTORY
 * @n  1-Aug-2007 [NamOh Hwang] : first writing
 *
 */

/******************************************************************************/
/* Header file inclusions                                                     */
/******************************************************************************/
#define     FSR_NO_INCLUDE_BML_HEADER
#define     FSR_NO_INCLUDE_STL_HEADER

#include    "FSR.h"
#include    "FSR_LLD_4K_OneNAND.h"

#define     FSR_LLD_LOGGING_HISTORY

#if defined(FSR_ONENAND_EMULATOR)
#define     FSR_LLD_LOGGING_HISTORY
#endif

#if defined(FSR_NBL2)
#undef      FSR_LLD_LOGGING_HISTORY
#endif

/******************************************************************************/
/* Global variable definitions                                                */
/******************************************************************************/
#if defined(TINY_FSR)
volatile OneNAND4kSharedCxt gstOND4kSharedCxt[FSR_MAX_DEVS] =
{
    {{0, 0}, {FSR_LLD_SUCCESS, FSR_LLD_SUCCESS}, {FSR_OND_4K_PREOP_NONE, FSR_OND_4K_PREOP_NONE}, {FSR_OND_4K_PREOP_ADDRESS_NONE, FSR_OND_4K_PREOP_ADDRESS_NONE}, {FSR_OND_4K_PREOP_ADDRESS_NONE, FSR_OND_4K_PREOP_ADDRESS_NONE}, {FSR_LLD_FLAG_NONE, FSR_LLD_FLAG_NONE}, {0, 0},{{0,},}, {{0,},}},
    {{0, 0}, {FSR_LLD_SUCCESS, FSR_LLD_SUCCESS}, {FSR_OND_4K_PREOP_NONE, FSR_OND_4K_PREOP_NONE}, {FSR_OND_4K_PREOP_ADDRESS_NONE, FSR_OND_4K_PREOP_ADDRESS_NONE}, {FSR_OND_4K_PREOP_ADDRESS_NONE, FSR_OND_4K_PREOP_ADDRESS_NONE}, {FSR_LLD_FLAG_NONE, FSR_LLD_FLAG_NONE}, {0, 0},{{0,},}, {{0,},}},
    {{0, 0}, {FSR_LLD_SUCCESS, FSR_LLD_SUCCESS}, {FSR_OND_4K_PREOP_NONE, FSR_OND_4K_PREOP_NONE}, {FSR_OND_4K_PREOP_ADDRESS_NONE, FSR_OND_4K_PREOP_ADDRESS_NONE}, {FSR_OND_4K_PREOP_ADDRESS_NONE, FSR_OND_4K_PREOP_ADDRESS_NONE}, {FSR_LLD_FLAG_NONE, FSR_LLD_FLAG_NONE}, {0, 0},{{0,},}, {{0,},}},
    {{0, 0}, {FSR_LLD_SUCCESS, FSR_LLD_SUCCESS}, {FSR_OND_4K_PREOP_NONE, FSR_OND_4K_PREOP_NONE}, {FSR_OND_4K_PREOP_ADDRESS_NONE, FSR_OND_4K_PREOP_ADDRESS_NONE}, {FSR_OND_4K_PREOP_ADDRESS_NONE, FSR_OND_4K_PREOP_ADDRESS_NONE}, {FSR_LLD_FLAG_NONE, FSR_LLD_FLAG_NONE}, {0, 0},{{0,},}, {{0,},}}
};

#if defined(FSR_LINUX_OAM)
EXPORT_SYMBOL(gstOND4kSharedCxt);
#endif
#endif /* defined(TINY_FSR) */

#if defined(FSR_LLD_LOGGING_HISTORY)

#if defined(TINY_FSR) || !defined(FSR_LLD_HANDSHAKE_ERR_INF)

volatile OneNAND4kOpLog gstOND4kOpLog[FSR_MAX_DEVS] =
{
    {0, {0,}, {0,}, {0,},{0,}},
    {0, {0,}, {0,}, {0,},{0,}},
    {0, {0,}, {0,}, {0,},{0,}},
    {0, {0,}, {0,}, {0,},{0,}}
};

#if defined(FSR_LINUX_OAM)
EXPORT_SYMBOL(gstOND4kOpLog);
#endif

#endif

#endif  /* #if defined(FSR_LLD_LOGGING_HISTORY) */
