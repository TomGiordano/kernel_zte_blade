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
 * @file      FSR.h
 * @brief     This file contains global type definition, global macro, 
 *              FSR version macro and build time.
 * @author    SongHo Yoon
 * @date      10-JAN-2007
 * @remark
 * REVISION HISTORY
 * @n  10-JAN-2007 [SongHo Yoon] : first writing
 * @n  01-FEB-2008 [MinYoung Kim] : version macro is modified for FSR v1.0.1
 *
 */

#ifndef _FSR_H_
#define _FSR_H_

#ifdef __cplusplus
extern "C" {                                    /* C declarations in C++     */
#endif

/******************************************************************************/
/*   Global macro configurations                                              */
/*                                                                            */
/* - FSR_NO_BASIC_TYPES        : to disable FSR basic types                   */
/*                               The header file which has the same type def  */
/*                               and FSR.h included in the same source file,  */
/*                               to avoid the duplicated typedef error,       */
/*                               define this macro before FSR.h               */
/* - FSR_NO_INCLUDE_BML_HEADER : to remove including "FSR_BML.h in this file  */
/*   FSR_NO_INCLUDE_STL_HEADER : to remove including "FSR_STL.h in this file  */
/******************************************************************************/

/*****************************************************************************/
/* Basic type defines                                                        */
/*****************************************************************************/
#if !defined(FSR_NO_BASIC_TYPES)

typedef     unsigned int                        UINT32;
typedef     int                                 INT32;
typedef     unsigned short                      UINT16;
typedef     short                               INT16;
typedef     unsigned char                       UINT8;
typedef     signed char                         INT8;

#endif /* FSR_NO_BASIC_TYPES */

#if !defined(FSR_WINCE_OAM)
#ifndef     VOID
#ifdef      _WIN32
typedef     void                                VOID;
#else
#define     VOID                                void
#endif
#endif
#endif      /* FSR_WINCE_OAM */

typedef     UINT32                              BOOL32;
typedef     UINT32                              SM32;

/*****************************************************************************/
/* Basic Constants                                                           */
/*****************************************************************************/
#ifndef     FALSE32
#define     FALSE32                             (BOOL32) 0
#endif

#ifndef     TRUE32
#define     TRUE32                              (BOOL32) 1
#endif

#ifndef     PUBLIC
#define     PUBLIC
#endif

#ifndef     PRIVATE
#define     PRIVATE                             static
#endif

/*****************************************************************************/
/* FSR static configuration parameters                                       */
/*****************************************************************************/
#define     FSR_MAX_VOLS                        (2)
#define     FSR_MAX_DEVS                        (4)
#define     FSR_DEVS_PER_VOL                    (FSR_MAX_DEVS / FSR_MAX_VOLS)
#define     FSR_MAX_DIES                        (2) /** # of dies of dev      */
#define     FSR_MAX_WAYS                        (2)
#define     FSR_MAX_PLANES                      (2)
#define     FSR_MAX_PHY_PGS                     128
#define     FSR_MAX_VIR_PGS                     (FSR_MAX_WAYS * FSR_MAX_PHY_PGS)
#define     FSR_MAX_PHY_SCTS                    (8)
#define     FSR_MAX_VIR_SCTS                    (FSR_MAX_PLANES * FSR_MAX_PHY_SCTS)
#define     FSR_MAX_BAD_BLKS                    (820) /* the maximum number of bad blocks */
#define     FSR_SECTOR_SIZE                     (512)
#define     FSR_SPARE_SIZE                      (16)

/*****************************************************************************/
/* Return value MACRO definition                                             */
/*                                                                           */
/* bit 31      /  1 bit  / err / if return value is error, 1. otherwise, 0   */
/* bit 30 ~ 28 /  3 bits / lay / STL:0, BML:1, LLD:2, PAM:3, OAM:4           */
/* bit 27 ~ 16 / 12 bits / major error value (0x000 ~ 0xFFF)                 */
/* bit 16 ~ 00 / 16 bits / minor error value (0x0000 ~ 0xFFFF)               */
/*****************************************************************************/
#define     FSR_RETURN_MAJOR(err)           (INT32)((err) & 0xFFFF0000)
#define     FSR_RETURN_MINOR(err)           (INT32)((err) & 0x0000FFFF)
#define     FSR_RETURN_VALUE(err, lay, maj, min)                             \
                                            (INT32)(((UINT32)((err) & 0x00000001) << 31) | \
                                            ((UINT32)((lay) & 0x00000007) << 28) | \
                                            ((UINT32)((maj) & 0x00000FFF) << 16) | \
                                            (UINT32)((min) & 0x0000FFFF))

/*****************************************************************************/
/* Module field of FSR_IOCTL_CODE                                            */
/*****************************************************************************/
#define     FSR_MODULE_STL                      0   /** sector translation layer */
#define     FSR_MODULE_BML                      1   /** block management laye r  */
#define     FSR_MODULE_LLD                      2   /** low level driver         */

/*****************************************************************************/
/* Method field of FSR_IOCTL_CODE                                            */
/*****************************************************************************/
#define     FSR_METHOD_BUFFERED                 0
#define     FSR_METHOD_IN_DIRECT                1
#define     FSR_METHOD_OUT_DIRECT               2
#define     FSR_METHOD_INOUT_DIRECT             3

/*****************************************************************************/
/* Access field of FSR_IOCTL_CODE                                            */
/*****************************************************************************/
#define     FSR_ANY_ACCESS                      0
#define     FSR_READ_ACCESS                     1
#define     FSR_WRITE_ACCESS                    2

/*****************************************************************************/
/* Macro definition for defining IOCTL control codes.                        */
/*                                                                           */
/* bit 31 ~ 18 / 14 bit  / Module field                                      */
/* bit 17 ~  4 / 14 bits / Function field                                    */
/* bit  3 ~  2 /  2 bits / Method field                                      */
/* bit  1 ~  0 /  2 bits / Access field                                      */
/*****************************************************************************/
#define     FSR_IOCTL_CODE(Module, Function, Method, Access)    (           \
               ((Module   & 0x3FFF) << 18 |                                 \
                (Function & 0x3FFF) <<  4 |                                 \
                (Method   & 0x0003) <<  2 |                                 \
                (Access   & 0x0003) <<  0)                                  \
)

/*****************************************************************************/
/* FSR version macro                                                         */
/*****************************************************************************/
#define     FSR_VER_MAJOR           1       /** FSR major version            */
#define     FSR_VER_MINOR1          2       /** FSR minor1 version           */
#define     FSR_VER_MINOR2          1       /** FSR minor2 version           */
#define     FSR_VER_PATCHLEVEL      1       /** FSR patch version            */
#define     FSR_BUILD_NUMBER        139     /** FSR build number             */

#define     FSR_VER_PREFIX          "FSR_"  /** FSR version prefix           */
#define     FSR_VERSION_RCX         "RTM"   /** FSR version RC{X} or RTM     */
/** FSR version code                                                         */
#define     FSR_VERSION_CODE(major, minor1, minor2, patchlevel) \
            ((major << 24) | (minor1 << 16) | (minor2 << 8) | patchlevel)

/** FSR version string                                                       */
#if (FSR_VER_PATCHLEVEL > 0)
#define     FSR_VERSION_STR(major, minor1, minor2, patchlevel, buildnum) \
            #major "." #minor1 "." #minor2 "p" #patchlevel "_" "b" #buildnum "_" FSR_VERSION_RCX
#else
#define     FSR_VERSION_STR(major, minor1, minor2, patchlevel, buildnum) \
            #major "." #minor1 "." #minor2 "_" "b" #buildnum "_" FSR_VERSION_RCX
#endif /* #if (FSR_VER_PATCHLEVEL > 0) */

/** FSR version variable declares                                            */
#define     FSR_VERSION(maj, min1, min2, plevel, bldnum) \
            const int   gnFSRVerCode = FSR_VERSION_CODE(maj, min1, min2, plevel);   \
            const char *gpaFSRVerStr = FSR_VER_PREFIX \
                           FSR_VERSION_STR (maj, min1, min2, plevel, bldnum);
                           
/*****************************************************************************/
/** FSR build time macro                                                     */
/*****************************************************************************/
#define     FSR_BUILD_TIME          __DATE__ " " __TIME__

/*****************************************************************************/
/* exported function prototype                                               */
/*****************************************************************************/
UINT8  *FSR_Version              (UINT32 *pnVerCode);
UINT32  FSR_VersionCode          (VOID);
UINT32  FSR_BuildNumber          (VOID);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/*****************************************************************************/
/* Include exported header files                                             */
/*****************************************************************************/
#include "FSR_OAM.h"
#include "FSR_LLD.h"

#if !defined(FSR_NO_INCLUDE_BML_HEADER)
#include "FSR_BML.h"
#endif

#if !defined(FSR_NO_INCLUDE_STL_HEADER)

#if defined(FSR_NO_INCLUDE_BML_HEADER)
#error  FSR_NO_INCLUDE_BML_HEADER macro should not be defined!!
#endif

#include "FSR_STL.h"

#endif

#include "FSR_PAM.h"
#include "FSR_DBG.h"
#include "FSR_PartitionID.h"

#endif /* #ifdef _FSR_H_ */
