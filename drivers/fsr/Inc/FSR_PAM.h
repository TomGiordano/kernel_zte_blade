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
 * @file      FSR_PAM.h
 * @brief     This file contains the definition and protypes of exported
 *              functions for Platform Adaptation Module of FSR.
 * @author    SongHo Yoon
 * @date      10-JAN-2007
 * @remark
 * REVISION HISTORY
 * @n  10-JAN-2007 [SongHo Yoon] : first writing
 *
 */

#ifndef _FSR_PAM_H_
#define _FSR_PAM_H_

/*****************************************************************************/
/* Definitions of PAM                                                        */
/*****************************************************************************/

#define     FlexOneNAND_SYSCONFIG_VAL  (0xC0F0)
#define     ASYNC_READ_MASK    ((FlexOneNAND_SYSCONFIG_VAL & 0x8000) ? 0 : 0x10)
#define     ASYNC_WRITE_MASK   ((FlexOneNAND_SYSCONFIG_VAL & 0x2)    ? 0 : 0x30)

#define     FSR_PAM_NOT_MAPPED          0xFFFFFFFF  /* Device not mapped 
                                                      in platform memory map */

#define     FSR_PAM_PROCESSOR_ID0       1           /* Processor ID0 */
#define     FSR_PAM_PROCESSOR_ID1       2           /* Processor ID1 */

/*****************************************************************************/
/* Major Return value of FSR_PAM_XXX()                                       */
/*****************************************************************************/
/* Common value for FSR_PAM_XXX()*/
#define     FSR_PAM_SUCCESS             FSR_RETURN_VALUE(0, 0x3, 0x0000, 0x0000)
#define     FSR_PAM_CRITICAL_ERROR      FSR_RETURN_VALUE(1, 0x3, 0x0001, 0x0000)
#define     FSR_PAM_NOT_INITIALIZED     FSR_RETURN_VALUE(1, 0x3, 0x0002, 0x0000)
#define     FSR_PAM_NAND_PROBE_FAILED   FSR_RETURN_VALUE(1, 0x3, 0x0003, 0x0000)
#define     FSR_PAM_LFT_NOT_LINKED      FSR_RETURN_VALUE(1, 0x3, 0x0004, 0x0000)

/**
 *  @brief      parameter of FSR_PAM_GetPAParm()
 */
typedef struct
{
    UINT32  nBaseAddr[FSR_MAX_DEVS/FSR_MAX_VOLS];
                                    /**< the base address for accessing NAND 
                                         device                              */
    UINT32  nIntID[FSR_MAX_DEVS/FSR_MAX_VOLS];
                                    /**< interrupt ID for non-blocking I/O
                                         FSR_INT_ID_NAND_0~7, refer FSR_OAM.h */
    UINT32  nDevsInVol;             /**< number of devices in the volume     */
    BOOL32  bProcessorSynchronization;/**< indicates whether synchronization
                                          is needed or not */
    UINT32  nSharedMemoryBase;      /**< base address for shared memory      */
    UINT32  nSharedMemorySize;      /**< the size of shared memory           */
    UINT32  nSharedMemoryInitCycle; /**< the cycle for initialization of shared memory.
                                         generally, this value is same as the number of Boot loaders(BLs).
                                         --------------------------------------------------------------
                                         open-count-scheme needs open() to be paired with close().
                                         But, this scheme can not be used in dual core system
                                         because BLs can not call close().
                                         Instead of open-count-scheme, the dual core system uses
                                         this value to identify the point of shared memory initialization.
                                         ------------------------------------------------------------- */
    UINT32  nMemoryChunkID;         /**< the memory chunck ID
                                         generally, nMemoryChunkID is same as
                                         the volume number                  */
    UINT32  nProcessorID;           /**< the processor ID
                                         prevent the illegal flash operation
                                         from accessing of the other processor */
    VOID   *pExInfo;                /**< For Device Extension.
                                         For Extra Information of Device,
                                         data structure can be mapped.       */
} FsrVolParm;

/*****************************************************************************/
/* Exported Function Prototype of PAM                                        */
/*****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

INT32   FSR_PAM_Init           (VOID);
VOID    FSR_PAM_InitNANDController      (VOID);

INT32   FSR_PAM_GetPAParm      (FsrVolParm      stVolParm[FSR_MAX_VOLS]);
INT32   FSR_PAM_RegLFT         (FSRLowFuncTbl  *pstLFT[FSR_MAX_VOLS]);

/*****************************************************************************/
/* APIs to do read, write of OneNAND register                                */
/*****************************************************************************/
UINT16  FSR_PAM_ReadOneNANDRegister     (UINT32   nAddr);
VOID    FSR_PAM_WriteToOneNANDRegister  (UINT32   nAddr,
                                         UINT16   nValue);

/*****************************************************************************/
/* APIs to do read, write of OneNAND DataRAM                                 */
/*****************************************************************************/
UINT16  FSR_PAM_Read2BFromDataRAM       (UINT32   nAddr);
VOID    FSR_PAM_Write2BToDataRAM        (UINT32   nAddr,
                                         UINT16   nValue);

/*****************************************************************************/
/* APIs to optimize NAND transfer performance (read, write of DataRAM)       */
/*****************************************************************************/
VOID    FSR_PAM_TransToNAND    (volatile VOID  *pDst, 
                                         VOID  *pSrc, 
                                         UINT32 nSize);
VOID    FSR_PAM_TransFromNAND  (         VOID  *pDst, 
                                volatile VOID  *pSrc, 
                                         UINT32 nSize);

/*****************************************************************************/
/* APIs to support non-blocking I/O feature                                  */
/*****************************************************************************/
INT32   FSR_PAM_InitInt        (UINT32   nLogIntId);
INT32   FSR_PAM_DeinitInt      (UINT32   nLogIntId);
UINT32  FSR_PAM_GetPhyIntID    (UINT32   nLogIntID);
INT32   FSR_PAM_ClrNEnableInt  (UINT32   nLogIntID);
INT32   FSR_PAM_ClrNDisableInt (UINT32   nLogIntID);

/*****************************************************************************/
/* APIs to support daul core feature                                  */
/*****************************************************************************/
BOOL32  FSR_PAM_CreateSL(UINT32  *pHandle, UINT32  nLayer);
BOOL32  FSR_PAM_AcquireSL(UINT32  nHandle, UINT32  nLayer);
BOOL32  FSR_PAM_ReleaseSL(UINT32  nHandle, UINT32  nLayer);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* _FSR_PAM_H_ */
