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
 *  @file       FSR_STL_Config.h
 *  @brief      This file defines configuration of Rainbow FTL.
 *  @author     Wonmoon Cheon
 *  @date       02-MAR-2007
 *  @remark
 *  REVISION HISTORY
 *  @n  02-MAR-2007 [Wonmoon Cheon] : first writing
 *  @n  29-JAN-2008 [MinYoung Kim] : dead code elimination
 *
 */

#ifndef __FSR_STL_CONFIG_H__
#define __FSR_STL_CONFIG_H__

/*****************************************************************************/
/* Header file inclusions                                                    */
/*****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/*****************************************************************************/
/* Macro                                                                     */
/*****************************************************************************/

/*****************************************************************************/
/* Compile Options                                                           */ 
/*****************************************************************************/
#if defined(FSR_ONENAND_EMULATOR)
#define OP_STL_DEBUG_CODE                   (1)
#else
#define OP_STL_DEBUG_CODE                   (0)
#endif

/**
 * @brief Option for data wear-leveling
 */
#define OP_SUPPORT_DATA_WEAR_LEVELING                   (1)

/**
 * @brief Option for meta wear-leveling
 */
#define OP_SUPPORT_META_WEAR_LEVELING                   (1)

/**
 * @brief Option for zone to zone global wear-leveling
 */
#define OP_SUPPORT_GLOBAL_WEAR_LEVELING                 (1)

/**
 * @brief Option for remember erase count during format
 */
#define OP_SUPPORT_REMEMBER_ECNT                        (1)

/**
 * @brief Option for LSB pages only mode in MLC NAND.
 */
#define OP_SUPPORT_MLC_LSB_ONLY                         (0)

/**
 * @brief Option for statistical information
 */
#define OP_SUPPORT_STATISTICS_INFO                      (0)

/**
 * @brief Option for page delete operation
 */
#define OP_SUPPORT_PAGE_DELETE                          (1)

/**
 * @brief Option for wait to complete of MSB page write
 */
#define OP_SUPPORT_MSB_PAGE_WAIT                        (0)

/**
 * @brief Option for enabling write buffer (for Hot/Cold management)
 */
#define OP_SUPPORT_WRITE_BUFFER                         (1)

/**
* @brief Option for whether the run-time Active Log Scan is supported or not
*/
#define OP_SUPPORT_RUNTIME_PMT_BUILDING                 (1)

/**
* @brief Option for writing mapping information and clear the meta block for reducing next open time
*/
#define OP_SUPPORT_CLOSE_TIME_UPDATE_FOR_FASTER_OPEN    (1)

/**
* @brief Option for sorting the free block list
*/
#define OP_SUPPORT_SORTED_FREE_BLOCK_LIST               (1)

/**
* @brief Option for run-time erase refresh
*/
#define OP_SUPPORT_RUNTIME_ERASE_REFRESH                (1)


//------------------------------------------------------------------------------
// STL Constants Setting

/**
 * @brief Maximum number of volumes
 */
#define MAX_NUM_VOLUME                      (FSR_MAX_VOLS)

/**
 * @brief Maximum number of clusters
 */
#define MAX_NUM_CLUSTERS                    (FSR_MAX_VOLS * FSR_MAX_STL_PARTITIONS)

/**
 * @brief Number of bytes per sector
 */
#define BYTES_PER_SECTOR                    (512)

/**
 * @brief Number of bytes per sector
 */
#define BYTES_SECTOR_SHIFT                  (9)     /* 512 Bytes */

/**
 * @brief Maximum number of STL zone
 */
#define MAX_ZONE                            (8)

/**
 * @brief Maximum number of root blocks
 * @remark If this definition value changes, backward-compatability will be broken.
 */
#define MAX_ROOT_BLKS                       (2)

/**
 * @brief Maximum number of STL meta blocks, the number of them is 2% of all unit except root units.
 * @remark When the number of zone unit is larger than 4800, the zone has maximum meta units.
 */
#define MAX_META_BLKS                       (96)

/**
 * @brief Minimum number of STL meta blocks
 * @remark Until 1.1.0, this value is 16 but this value changes to 3 since 1.2.0
 */
#define MIN_META_BLKS                       (3) 

/**
 * @brief Minimum number of STL initial free blocks
 * @remark Until 1.1.0, this value is 16 but this value changes to 3 since 1.2.0
 */
#define MIN_INIT_FREE_BLKS                  (2)

/**
 * @brief Maximum number of log groups per one PMT meta page
 */
#define MAX_LOGGRP_PER_PMT                  (16)

/**
 * @brief ATL mapping parameter : N (number of data blocks per group)
 */
#define NUM_DBLKS_PER_GRP                   (1)

/**
 * @brief Mapping parameter : N (bit shift)
 */
#define NUM_DBLKS_PER_GRP_SHIFT             (0)

/**
 * @brief Mapping parameter : k (max. number of log blocks per group)
 * @n      Note that the 'k' should be equal or less than the number of 
 * @n      total log blocks. (k can not exceed 256)
 */
#define LBLKS_PER_GRP_2K_FORMAT             (8)
#define LBLKS_PER_GRP_4K_FORMAT             (8)
#define LBLKS_PER_GRP_8K_FORMAT             (16)
#define LBLKS_PER_GRP_16K_FORMAT            (16)
#define MAX_LBLKS_PER_GRP                   (16)

/**
 * @brief Total number of active log blocks
 */
#define MAX_ACTIVE_LBLKS                    (8)

/**
* @brief Ratio of meta blocks
*/
#define META_BLOCK_RATIO                    (20)    // 2% (x/1000)

/**
 * @brief Number of bytes of page valid (delete) bitmap table
 */
#define BMT_PVB_SIZE                        (32)    // bytes = (MAX_PAGES_PER_SBLK / 8)

/**
 * @brief Active log group memory pool size ( >= MAX_ACTIVE_LBLKS )
 */
#define ACTIVE_LOG_GRP_POOL_SIZE            (MAX_ACTIVE_LBLKS)

/**
 * @brief Inactive log group memory pool size
 */
#define INACTIVE_LOG_GRP_POOL_SIZE          (16)

/**
 * @brief The default number of initial free blocks. 
 */
#define DEFAULT_INIT_FREE_BLKS              (MAX_ACTIVE_LBLKS + 2)

/**
 * @brief Wear-leveling trigger threshold value (difference of block erase count)
 */
#define DEFAULT_WEARLEVEL_THRESHOLD         (1000)

/**
 * @brief Decay period of two-level LRU (for hot/cold data management)
 */
#define DEFAULT_DECAY_PERIOD                (100)

/**
 * @brief NAND sync period of read count table (for runtime erase refresh)
 */
#define DEFAULT_RCT_SYNC_PERIOD             (10)

/**
 * @brief Root info buffer size
 */
#define ROOT_INFO_BUF_SIZE                  (2048)
#define ROOT_INFO_BUF_BITMAP                (0x0000000F)
#define ROOT_INFO_BUF_PADDING               (1)

/**
 * @brief Number of data blocks per one LA (Local Area)
 */
#define META_2KB_DBLKS_PER_LA               (128)
#define META_4KB_DBLKS_PER_LA               (512)
#define META_8KB_DBLKS_PER_LA               (512)
#define META_16KB_DBLKS_PER_LA              (2048)

/**
 * @brief Number of log groups for PMT wear-leveling (2nd round)
 */
#define DEFAULT_PMT_EC_GRP_SIZE             (8)
#define DEFAULT_PMT_EC_GRP_SIZE_SHIFT       (3)

/**
 * @brief Maximum number of VFL parameters per one cluster
 */
#define MAX_VFL_PARAMS                      (8)

/**
 * @brief Maximum number of extended VFL parameters per one cluster
 */
#define MAX_VFLEXT_PARAMS                   (8)

/**
 * @brief Maximum number of meta pages(BMT or PMT, excluding DH) in the latest meta unit  
 * @n     after STL_Close() was called 
 */
#define MAX_META_PAGES_FOR_NEW_ALLOCATION   (8)

/**
 * @brief Maximum number of erase refresh candidate list
 */
#define MAX_ER_CANDIDATES                   (32)

/**
 * @brief Maximum number of entries of read count reference table
 */
#define MAX_RCRT_ENTRY                      (5)

#ifdef __cplusplus
}
#endif /* __cplusplus */

/*****************************************************************************/
/* Type defines                                                              */
/*****************************************************************************/

/*****************************************************************************/
/* Constant definitions                                                      */
/*****************************************************************************/

/*****************************************************************************/
/* Exported variable declarations                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Exported function prototype                                               */
/*****************************************************************************/

#endif /* __FSR_STL_CONFIG_H__ */
