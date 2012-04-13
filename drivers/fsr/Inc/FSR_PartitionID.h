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
 * @file      FSR_PartitionID.h
 * @brief     This file contains FSR Partition IDs.
 * @n         User can add user's FSR Partition IDs.
 * @author    SongHo Yoon
 * @date      12-JAN-2007
 * @remark
 * REVISION HISTORY
 * @n  12-JAN-2007 [SongHo Yoon] : first writing
 *
 */

#ifndef _FSR_PARTITION_ID_H_
#define _FSR_PARTITION_ID_H_
/*****************************************************************************/
/* Partition ID of FSR_BML_LoadPIEntry()                                     */
/*****************************************************************************/
#define     FSR_PARTID_MASK                 0xFFFF  /* Partition ID MASK     */
#define     FSR_PARTID_BML0                 0       /* Partition ID 0        */
#define     FSR_PARTID_BML1                 1       /* Partition ID 1        */
#define     FSR_PARTID_BML2                 2       /* Partition ID 2        */
#define     FSR_PARTID_BML3                 3       /* Partition ID 3        */
#define     FSR_PARTID_BML4                 4       /* Partition ID 4        */
#define     FSR_PARTID_BML5                 5       /* Partition ID 5        */
#define     FSR_PARTID_BML6                 6       /* Partition ID 6        */
#define     FSR_PARTID_BML7                 7       /* Partition ID 7        */
#define     FSR_PARTID_BML8                 8       /* Partition ID 8        */
#define     FSR_PARTID_BML9                 9       /* Partition ID 9        */
#define     FSR_PARTID_BML10                10      /* Partition ID 10       */
#define     FSR_PARTID_BML11                11      /* Partition ID 11       */
#define     FSR_PARTID_BML12                12      /* Partition ID 12       */
#define     FSR_PARTID_BML13                13      /* Partition ID 13       */
#define     FSR_PARTID_BML14                14      /* Partition ID 14       */
#define     FSR_PARTID_BML15                15      /* Partition ID 15       */
#define     FSR_PARTID_BML16                16      /* Partition ID 16       */
#define     FSR_PARTID_BML17                17      /* Partition ID 17       */
#define     FSR_PARTID_BML18                18      /* Partition ID 18       */
#define     FSR_PARTID_BML19                19      /* Partition ID 19       */
#define     FSR_PARTID_BML20                20      /* Partition ID 20       */
#define     FSR_PARTID_BML21                21      /* Partition ID 21       */
#define     FSR_PARTID_BML22                22      /* Partition ID 22       */
#define     FSR_PARTID_BML23                23      /* Partition ID 23       */
#define     FSR_PARTID_BML24                24      /* Partition ID 24       */
#define     FSR_PARTID_BML25                25      /* Partition ID 25       */
#define     FSR_PARTID_BML26                26      /* Partition ID 26       */
#define     FSR_PARTID_BML27                27      /* Partition ID 27       */
#define     FSR_PARTID_BML28                28      /* Partition ID 28       */
#define     FSR_PARTID_BML29                29      /* Partition ID 29       */
#define     FSR_PARTID_BML30                30      /* Partition ID 30       */
#define     FSR_PARTID_BML31                31      /* Partition ID 31       */
#define     FSR_PARTID_BML32                32      /* Partition ID 32       */
#define     FSR_PARTID_BML33                33      /* Partition ID 33       */
#define     FSR_PARTID_BML34                34      /* Partition ID 34       */
#define     FSR_PARTID_BML35                35      /* Partition ID 35       */


/* Partition IDs for STL */
#define     FSR_PARTID_STL0                 (FSR_PARTID_BML20)
#define     FSR_PARTID_STL1                 (FSR_PARTID_BML21)
#define     FSR_PARTID_STL2                 (FSR_PARTID_BML22)
#define     FSR_PARTID_STL3                 (FSR_PARTID_BML23)
#define     FSR_PARTID_STL4                 (FSR_PARTID_BML24)
#define     FSR_PARTID_STL5                 (FSR_PARTID_BML25)
#define     FSR_PARTID_STL6                 (FSR_PARTID_BML26)
#define     FSR_PARTID_STL7                 (FSR_PARTID_BML27)
#define     FSR_PARTID_STL8                 (FSR_PARTID_BML28)
#define     FSR_PARTID_STL9                 (FSR_PARTID_BML29)
#define     FSR_PARTID_STL10                (FSR_PARTID_BML30)
#define     FSR_PARTID_STL11                (FSR_PARTID_BML31)
#define     FSR_PARTID_STL12                (FSR_PARTID_BML32)
#define     FSR_PARTID_STL13                (FSR_PARTID_BML33)
#define     FSR_PARTID_STL14                (FSR_PARTID_BML34)
#define     FSR_PARTID_STL15                (FSR_PARTID_BML35)

#define     FSR_MAX_STL_PARTITIONS          (FSR_PARTID_STL15 - FSR_PARTID_STL0 + 1)


/* 
 * Dummy Partition IDs
 * DO NOT use these IDs. these IDs are used internally 
 */
#define     FSR_PARTID_DUMMY0               (0xFFD0)
#define     FSR_PARTID_DUMMY1               (0xFFD1)
#define     FSR_PARTID_DUMMY2               (0xFFD2) 
#define     FSR_PARTID_DUMMY3               (0xFFD3) 
#define     FSR_PARTID_DUMMY4               (0xFFD4)
#define     FSR_PARTID_DUMMY5               (0xFFD5) 
#define     FSR_PARTID_DUMMY6               (0xFFD6) 
#define     FSR_PARTID_DUMMY7               (0xFFD7) 
#define     FSR_PARTID_DUMMY8               (0xFFD8) 
#define     FSR_PARTID_DUMMY9               (0xFFD9) 
#define     FSR_PARTID_DUMMY10              (0xFFDA)
#define     FSR_PARTID_DUMMY11              (0xFFDB)
#define     FSR_PARTID_DUMMY12              (0xFFDC)
#define     FSR_PARTID_DUMMY13              (0xFFDD)
#define     FSR_PARTID_DUMMY14              (0xFFDE)
#define     FSR_PARTID_DUMMY15              (0xFFDF)
#define     FSR_PARTID_DUMMY16              (0xFFF0)
#define     FSR_PARTID_DUMMY17              (0xFFF1)
#define     FSR_PARTID_DUMMY18              (0xFFF2)
#define     FSR_PARTID_DUMMY19              (0xFFF3)
#define     FSR_PARTID_DUMMY20              (0xFFF4)
#define     FSR_PARTID_DUMMY21              (0xFFF5)
#define     FSR_PARTID_DUMMY22              (0xFFF6)
#define     FSR_PARTID_DUMMY23              (0xFFF7)
#define     FSR_PARTID_DUMMY24              (0xFFF8)
#define     FSR_PARTID_DUMMY25              (0xFFF9)
#define     FSR_PARTID_DUMMY26              (0xFFFA)
#define     FSR_PARTID_DUMMY27              (0xFFFB)
#define     FSR_PARTID_DUMMY28              (0xFFFC)
#define     FSR_PARTID_DUMMY29              (0xFFFD)


/*****************************************************************************/
/* User can customize FSR PARTITION ID using                                 */
/* - FSR_PARTID_BMLxx & FSR_PARTID_STLxx                                     */
/*****************************************************************************/
#define     FSR_PARTID_BOOTLOADER           (FSR_PARTID_BML0) /* NAND bootloader */

#if defined(FSR_WINCE_OAM)

#define     FSR_PARTID_NBLS                 FSR_PARTID_BML0
#define     FSR_PARTID_EBOOT                FSR_PARTID_BML1
#define     FSR_PARTID_IPL                  FSR_PARTID_BML2
#define     FSR_PARTID_EBOOTCFG             FSR_PARTID_BML3
#define     FSR_PARTID_RESERVED             FSR_PARTID_BML4

#define     FSR_PARTID_OS                   FSR_PARTID_STL0
#define     FSR_PARTID_FS0                  FSR_PARTID_STL1
#define     FSR_PARTID_FS1                  FSR_PARTID_STL2

#elif defined(FSR_SYMOS_OAM)

#define     FSR_PARTID_COPIEDOS             FSR_PARTID_BML3
#define     FSR_PARTID_DEMANDONOS           FSR_PARTID_BML4
#define     FSR_PARTID_FILESYSTEM           FSR_PARTID_STL0
#define     FSR_PARTID_FILESYSTEM1          FSR_PARTID_STL1
#define     FSR_PARTID_FILESYSTEM2          FSR_PARTID_STL2
#define     FSR_PARTID_FILESYSTEM3          FSR_PARTID_STL3
#define     FSR_PARTID_FILESYSTEM4          FSR_PARTID_STL4
#define     FSR_PARTID_FILESYSTEM5          FSR_PARTID_STL5
#define     FSR_PARTID_FILESYSTEM6          FSR_PARTID_STL6
#define     FSR_PARTID_FILESYSTEM7          FSR_PARTID_STL7

#elif defined(FSR_LINUX_OAM)

#define     FSR_PARTID_UBOOT                FSR_PARTID_BML1
#define     FSR_PARTID_UBOOT_PARAMETER      FSR_PARTID_BML2
#define     FSR_PARTID_COPIEDOS             FSR_PARTID_BML3
#define     FSR_PARTID_CRAMFS               FSR_PARTID_BML4
#define     FSR_PARTID_FATFS0               FSR_PARTID_STL0
#define     FSR_PARTID_FATFS1               FSR_PARTID_STL1

#else /* RTOS or WIN32 */

#define     FSR_PARTID_COPIEDOS             FSR_PARTID_BML3  /* OS image copied from NAND 
                                                                flash memory to RAM        */
#define     FSR_PARTID_DEMANDONOS           FSR_PARTID_BML4  /* OS image loaded on demand  */
#define     FSR_PARTID_FILESYSTEM           FSR_PARTID_STL0
#define     FSR_PARTID_FILESYSTEM1          FSR_PARTID_STL1
#define     FSR_PARTID_FILESYSTEM2          FSR_PARTID_STL2
#define     FSR_PARTID_FILESYSTEM3          FSR_PARTID_STL3
#define     FSR_PARTID_FILESYSTEM4          FSR_PARTID_STL4
#define     FSR_PARTID_FILESYSTEM5          FSR_PARTID_STL5
#define     FSR_PARTID_FILESYSTEM6          FSR_PARTID_STL6
#define     FSR_PARTID_FILESYSTEM7          FSR_PARTID_STL7

#endif

#endif  /* _FSR_PARTITION_ID_H_ */
