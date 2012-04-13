/**
 *   @mainpage   Flex Sector Remapper : LinuStoreIII_1.2.0_b036-FSR_1.2.1p1_b139_RTM
 *
 *   @section Intro Intro
 *       Flash Translation Layer for Flex-OneNAND and OneNAND
 *   
 *     @MULTI_BEGIN@ @COPYRIGHT_DEFAULT
 *     @section Copyright COPYRIGHT_DEFAULT
 *            COPYRIGHT. SAMSUNG ELECTRONICS CO., LTD.
 *                                    ALL RIGHTS RESERVED
 *     Permission is hereby granted to licensees of Samsung Electronics Co., Ltd. products
 *     to use this computer program only in accordance 
 *     with the terms of the SAMSUNG FLASH MEMORY DRIVER SOFTWARE LICENSE AGREEMENT.
 *     @MULTI_END@
 *
 *     @MULTI_BEGIN@ @COPYRIGHT_GPL
 *     @section Copyright COPYRIGHT_GPL
 *            COPYRIGHT. SAMSUNG ELECTRONICS CO., LTD.
 *                                    ALL RIGHTS RESERVED
 *     This program is free software; you can redistribute it and/or modify it
 *     under the terms of the GNU General Public License version 2 
 *     as published by the Free Software Foundation.
 *     @MULTI_END@
 *
 *     @section Description
 *
 */

/**
 * @file      FSR_OAM.h
 * @brief     This file contains the definition and protypes of exported
 *              functions for OS Adaptation Module.
 * @author    SongHo Yoon
 * @date      10-JAN-2007
 * @remark
 * REVISION HISTORY
 * @n  10-JAN-2007 [SongHo Yoon] : first writing
 *
 */

#ifndef _FSR_OAM_H_
#define _FSR_OAM_H_

/*****************************************************************************/
/* OS Selection                                                              */
/* DO NOT DEFINE FSR_WINCE_OAM or FSR_SYMOS_OAM or in This Header File       */
/*   FSR_WINCE_OAM  : the definition for WindowsCE / Windows Mobile          */
/*   FSR_SYMOS_OAM  : the definition for SymbianOS (EKA2)                    */
/*   FSR_WIN32_OAM  : the definition for Win32                               */
/*   FSR_LINUX_OAM  : the definition for Linux                               */
/*                                                                           */
/*****************************************************************************/

#if defined(FSR_WINCE_OAM)

    #include <windows.h>

    #if defined(FSR_OAM_RTLMSG_DISABLE)
    #define FSR_RTL_PRINT(x)
    #else
    #define FSR_RTL_PRINT(x)        RETAILMSG(1, x)
    #endif /* FSR_OAM_RTLMSG_DISABLE */

    #if defined(FSR_OAM_DBGMSG_ENABLE)
    #define FSR_DBG_PRINT(x)        RETAILMSG(1, x)
    #endif /* FSR_OAM_DBGMSG_ENABLE */

    #define     FSR_OAM_MEMCPY(a, b, c)                 memcpy((a), (b), (c))
    #define     FSR_OAM_MEMSET(a, b, c)                 memset((a), (b), (c))
    #define     FSR_OAM_MEMCMP(a, b, c)                 memcmp((a), (b), (c))

#elif defined(FSR_SYMOS_OAM)

    #include <locmedia.h>
    #include <platform.h>
    
    #define TEXT(x)                 (x)

    #define     FSR_OAM_MEMCPY(a, b, c)                 memcpy((TAny*)(a), (const TAny*)(b), (unsigned int)c)
    #define     FSR_OAM_MEMSET(a, b, c)                 memset((TAny*)(a),(TInt)(b), (unsigned int)(c))
    #define     FSR_OAM_MEMCMP(a, b, c)                 FSR_OAM_Memcmp((a), (b), (c))

    #if defined(FSR_OAM_RTLMSG_DISABLE)
    #define FSR_RTL_PRINT(x)
    #else
    #define FSR_RTL_PRINT(x)        Kern::Printf x
    #endif /* FSR_OAM_RTLMSG_DISABLE */

    #if defined(FSR_OAM_DBGMSG_ENABLE)
        #define FSR_DBG_PRINT(x)        Kern::Printf x
    #endif /* FSR_OAM_DBGMSG_ENABLE */

#elif defined(FSR_LINUX_OAM)

    #include <linux/kernel.h>
    #include <linux/module.h>
    #include <linux/string.h>    

    #if defined(FSR_OAM_RTLMSG_DISABLE)
    #define FSR_RTL_PRINT(x)
    #else
    #define FSR_RTL_PRINT(x)        FSR_OAM_DbgMsg x
    #endif /* FSR_OAM_RTLMSG_DISABLE */

    #if defined(FSR_OAM_DBGMSG_ENABLE)
    #define FSR_DBG_PRINT(x)        FSR_OAM_DbgMsg x
    #else
    #define FSR_DBG_PRINT(x)
    #endif /* FSR_OAM_DBGMSG_ENABLE */

    #ifndef   TEXT
    #define TEXT(x)                 (VOID *) (x)
    #endif

    #include <linux/string.h>
    #define     FSR_OAM_MEMCPY(a, b, c)                 memcpy((a), (b), (c))
    #define     FSR_OAM_MEMSET(a, b, c)                 memset((a), (b), (c))
    #define     FSR_OAM_MEMCMP(a, b, c)                 memcmp((a), (b), (c))

#else /* other case */

    #define FSR_RTL_PRINT(x)        FSR_OAM_DbgMsg x
   
    #if defined(FSR_OAM_DBGMSG_ENABLE)
    #define FSR_DBG_PRINT(x)        FSR_OAM_DbgMsg x
    #else
    #define FSR_DBG_PRINT(x)
    #endif /* FSR_OAM_DBGMSG_ENABLE */

    #ifdef   TEXT
    #undef   TEXT
    #endif
    #define TEXT(x)                 (VOID *) (x)

    #define     FSR_OAM_MEMCPY(a, b, c)                 FSR_OAM_Memcpy((a), (b), (c))
    #define     FSR_OAM_MEMSET(a, b, c)                 FSR_OAM_Memset((a), (b), (c))
    #define     FSR_OAM_MEMCMP(a, b, c)                 FSR_OAM_Memcmp((a), (b), (c))
#endif

/*****************************************************************************/
/* Major Return value of FSR_OAM_XXX()                                       */
/*****************************************************************************/
/* Common value for FSR_OAM_XXX() */
#define     FSR_OAM_SUCCESS             FSR_RETURN_VALUE(0, 0x4, 0x0000, 0x0000)
#define     FSR_OAM_CRITICAL_ERROR      FSR_RETURN_VALUE(1, 0x4, 0x0001, 0x0000)
#define     FSR_OAM_NOT_INITIALIZED     FSR_RETURN_VALUE(1, 0x4, 0x0002, 0x0000)
#define     FSR_OAM_TIMEOUT             FSR_RETURN_VALUE(1, 0x4, 0x0003, 0x0000)
#define     FSR_OAM_INIT_INT_ERROR      FSR_RETURN_VALUE(1, 0x4, 0x0004, 0x0000)
#define     FSR_OAM_DEINIT_INT_ERROR    FSR_RETURN_VALUE(1, 0x4, 0x0005, 0x0000)
#define     FSR_OAM_NOT_ALIGNED_MEMPTR  FSR_RETURN_VALUE(1, 0x4, 0x0006, 0x0000)
#define     FSR_OAM_MALLOC_FAIL         FSR_RETURN_VALUE(1, 0x4, 0x0007, 0x0000)

/*****************************************************************************/
/* nLayer parameter value of FSR_OAM_CreateSM()                              */
/*****************************************************************************/
#define     FSR_OAM_SM_TYPE_BDD         (UINT32) (1)
#define     FSR_OAM_SM_TYPE_STL         (UINT32) (2)
#define     FSR_OAM_SM_TYPE_BML         (UINT32) (3)
#define     FSR_OAM_SM_TYPE_LLD         (UINT32) (4)
#define     FSR_OAM_SM_TYPE_BML_VOL_0   (UINT32) (3)
#define     FSR_OAM_SM_TYPE_BML_VOL_1   (UINT32) (5)

/*****************************************************************************/
/* NULL #defines                                                             */
/*   If Each OAM header file defines NULL, following define is not used.     */
/*****************************************************************************/
#ifndef     NULL
#ifdef      __cplusplus
#define     NULL                0
#else
#define     NULL                ((void *)0)
#endif
#endif

/*****************************************************************************/
/* __FUNCTION__ / __FILE__ macro                                             */
/*****************************************************************************/
#ifndef __FUNCTION__
    #if (__CC_ARM == 1)
        #define __FUNCTION__ __func__   /* for ARM ADS1.2 */
    #elif defined(__linux__)
        /* Linux supports __FUNCTION__ */
    #else
        #define __FUNCTION__ "[__FUNCTION__]"
    #endif
#endif

#define __FSR_FUNC__        TEXT(__FUNCTION__)
#define __FSR_FILE__        TEXT(__FILE__)

/****************************************************************************/
/* assertion macro                                                          */
/****************************************************************************/
#if defined(FSR_ASSERT)
    #undef  FSR_ASSERT
    #define FSR_ASSERT(f)   {if ((f) == 0) \
                            {FSR_RTL_PRINT((TEXT("\n FSR/Assertion Failed : %s,line:%d,func:%s\n"), (const unsigned char *) __FSR_FILE__, __LINE__, (const unsigned char *) __FSR_FUNC__));\
                            FSR_RTL_PRINT((TEXT("\n<log P1=\"100\" P2=\"POR\" P3=\"1\" P4=\"mammoth auto power off\" />\n")));\
                            while (1);}}
#else
    #define FSR_ASSERT(f)
#endif

/****************************************************************************/
/* mammoth macro                                                            */
/****************************************************************************/
#if defined(FSR_MAMMOTH_POWEROFF)
    #undef  FSR_MAMMOTH_POWEROFF
    #define FSR_MAMMOTH_POWEROFF()  {FSR_RTL_PRINT((TEXT("\nMammoth auto power off protocol : %s,line:%d,func:%s\n"), (const unsigned char *) __FSR_FILE__, __LINE__, (const unsigned char *) __FSR_FUNC__));\
                                     FSR_RTL_PRINT((TEXT("\n<log P1=\"100\" P2=\"POR\" P3=\"1\" P4=\"mammoth auto power off\" />\n")));}
#else
    #define  FSR_MAMMOTH_POWEROFF()
#endif

/*****************************************************************************/
/* Interrupt ID                                                              */
/*****************************************************************************/
#define     FSR_INT_ID_NAND_0   (0)     /** Interrupt ID : 1st NAND          */
#define     FSR_INT_ID_NAND_1   (1)     /** Interrupt ID : 2nd NAND          */
#define     FSR_INT_ID_NAND_2   (2)     /** Interrupt ID : 3rd NAND          */
#define     FSR_INT_ID_NAND_3   (3)     /** Interrupt ID : 4th NAND          */
#define     FSR_INT_ID_NAND_4   (4)     /** Interrupt ID : 5th NAND          */
#define     FSR_INT_ID_NAND_5   (5)     /** Interrupt ID : 6th NAND          */
#define     FSR_INT_ID_NAND_6   (6)     /** Interrupt ID : 7th NAND          */
#define     FSR_INT_ID_NAND_7   (7)     /** Interrupt ID : 8th NAND          */
#define     FSR_INT_ID_NONE     (0xFFFF)/** no interrupt ID                  */

/*****************************************************************************/
/* semaphore macro                                                           */
/*****************************************************************************/
#define     FSR_OAM_MAX_SEMAPHORES              (FSR_MAX_VOLS + FSR_MAX_STL_PARTITIONS * FSR_MAX_VOLS)
#define     FSR_OAM_MAX_EVENTS                  (FSR_INT_ID_NAND_7 + 1)

/*****************************************************************************/
/* memory type macro                                                         */
/*****************************************************************************/
#define     FSR_OAM_LOCAL_MEM                   (0x00000000)
#define     FSR_OAM_SHARED_MEM                  (0x00000001)

/*****************************************************************************/
/* heap memory chunk macro                                                   */
/*****************************************************************************/
#define     FSR_MAX_HEAP_MEM_CHUNKS             (FSR_MAX_VOLS)
#define     FSR_HEAP_MEM_CHUNK_0                (0) /** MemChunk ID: volume 0*/
#define     FSR_HEAP_MEM_CHUNK_1                (1) /** MemChunk ID: volume 1*/

/*****************************************************************************/
/* Miscellaneous Functions Wrappers                                          */
/*****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*****************************************************************************/
/* exported function prototype of OAM                                        */
/*****************************************************************************/
INT32    FSR_OAM_Init                   (VOID);
VOID     FSR_OAM_InitSharedMemory       (VOID);

/*****************************************************************************/
/* APIs for memory control                                                   */
/*****************************************************************************/
VOID     FSR_OAM_InitMemStat            (VOID);
VOID     FSR_OAM_GetMemStat             (UINT32     *pnHeapUsage,
                                         UINT32     *pnNumOfMemReqs);
VOID     FSR_OAM_SetMResetPoint         (UINT32      nMemChunkID);
VOID     FSR_OAM_ResetMalloc            (UINT32      nMemChunkID,
                                         BOOL32      bReset);

VOID    *FSR_OAM_Malloc                 (UINT32      nSize);
VOID    *FSR_OAM_MallocExt              (UINT32      nMemChunkID,
                                         UINT32      nSize,
                                         UINT32      nMemType);
VOID     FSR_OAM_Free                   (VOID       *pMem);
VOID     FSR_OAM_FreeExt                (UINT32      nMemChunkID,
                                         VOID       *pMem,
                                         UINT32      nMemType);
INT32    FSR_OAM_Memcmp                 (VOID       *pSrc,
                                         VOID       *pDst,
                                         UINT32      nLen);
VOID     FSR_OAM_Memcpy                 (VOID       *pDst,
                                         VOID       *pSrc,
                                         UINT32      nLen);
VOID     FSR_OAM_Memset                 (VOID       *pDst,
                                         UINT8       nData,
                                         UINT32      nLen);
/*****************************************************************************/
/* APIs for semaphore control                                                */
/*****************************************************************************/
BOOL32   FSR_OAM_CreateSM               (SM32       *pHandle,
                                         UINT32      nLayer);
BOOL32   FSR_OAM_DestroySM              (SM32        nHandle,
                                         UINT32      nLayer);
BOOL32   FSR_OAM_AcquireSM              (SM32        nHandle,
                                         UINT32      nLayer);
BOOL32   FSR_OAM_ReleaseSM              (SM32        nHandle,
                                         UINT32      nLayer);
UINT32   FSR_OAM_Pa2Va                  (UINT32      nPAddr);

/*****************************************************************************/
/* APIs to support non-blocking I/O feature                                  */
/*****************************************************************************/
INT32    FSR_OAM_InitInt                (UINT32     nLogIntId);
INT32    FSR_OAM_DeinitInt              (UINT32     nLogIntId);
INT32    FSR_OAM_ClrNDisableInt         (UINT32     nLogIntId);
INT32    FSR_OAM_ClrNEnableInt          (UINT32     nLogIntId);

BOOL32   FSR_OAM_CreateEvent            (UINT32    *pnHandle);
BOOL32   FSR_OAM_DeleteEvent            (UINT32     nHandle);
BOOL32   FSR_OAM_SendEvent              (UINT32     nHandle);
BOOL32   FSR_OAM_ReceiveEvent           (UINT32     nHandle);

/*****************************************************************************/
/* APIs for DMA                                                              */
/*****************************************************************************/
INT32    FSR_OAM_InitDMA                (VOID);
INT32    FSR_OAM_ReadDMA                (UINT32     nVirDstAddr,
                                         UINT32     nVirSrcAddr,
                                         UINT32     nSize);
INT32    FSR_OAM_WriteDMA               (UINT32     nVirDstAddr,
                                         UINT32     nVirSrcAddr,
                                         UINT32     nSize);

/*****************************************************************************/
/* APIs for Timer (ONLY for performance measurement)                         */
/*****************************************************************************/
VOID     FSR_OAM_WaitNMSec              (UINT32 nNMSec);
VOID     FSR_OAM_StartTimer             (VOID);
VOID     FSR_OAM_StopTimer              (VOID);
UINT32   FSR_OAM_GetElapsedTime         (VOID);

/*****************************************************************************/
/* extra APIs                                                                */
/*****************************************************************************/
VOID     FSR_OAM_DbgMsg                 (VOID *pStr, ...);
BOOL32   FSR_OAM_GetROLockFlag          (VOID);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* _FSR_OAM_H_ */
