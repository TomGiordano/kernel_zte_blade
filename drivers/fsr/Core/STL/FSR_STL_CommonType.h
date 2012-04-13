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
 *  @file       FSR_STL_CommonType.h
 *  @brief      Definition for common data types.
 *  @author     Wonmoon Cheon
 *  @date       01-OCT-2006
 *  @remark
 *  REVISION HISTORY
 *  @n  01-OCT-2006 [Wonmoon Cheon] : first writing
 *  @n  29-JAN-2008 [MinYoung Kim] : dead code elimination
 *
 */

#ifndef __FSR_STL_COMMON_TYPE_H__
#define __FSR_STL_COMMON_TYPE_H__

/*****************************************************************************/
/* Header file inclusions                                                    */
/*****************************************************************************/

/*****************************************************************************/
/* Macro                                                                     */
/*****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif  /*__cplusplus*/

/*****************************************************************************/
/* Type defines                                                              */
/*****************************************************************************/
/**
 * @brief block address type
 */
typedef     UINT16      BADDR;

/**
 * @brief page address type
 */
typedef     UINT32      PADDR;

/**
 * @brief page offset in a super block
 */
typedef     UINT16      POFFSET;

/**
 * @brief sector bitmap type
 */
typedef     UINT32      SBITMAP;

/**
* @brief Maximum number of random in command per one VFL request
*/
#define MAX_RND_IN_ARGS                     (32)

/**
* @brief maximum number of sectors per virtual page 
*/
#define MAX_SECTORS_PER_VPAGE               (32)

#define STL_2KB_PG                          (4)             /* 2KB page device   */
#define STL_4KB_PG                          (8)             /* 4KB page device   */

/**
 * @brief   Random In argument
 */
typedef struct
{
    UINT16          nOffset;            /**< nOffset should be even number     */
    UINT16          nNumOfBytes;        /**< Random In Bytes                   */
    VOID            *pBuf;              /**< Data Buffer Pointer               */
} VFLRndInArg;

/**
 * @brief   Extended VFL parameters
 */
typedef struct
{
    UINT32          aExtSData[MAX_SECTORS_PER_VPAGE];
    VFLRndInArg     astRndIn[MAX_RND_IN_ARGS];
    UINT16          nNumExtSData;   /**< actual number of extended spare data   */
    UINT16          nNumRndIn;      /**< actual number of random in command     */
} VFLExtParam;

/**
 * @brief   This structure is VFL parameter.
 */
typedef struct
{
    UINT8          *pData;          /**< Buffer pointer                         */
    UINT32          nLpn;           /**< Logical page number                    */
    BOOL32          bUserData;      /**< Is pData user data?                    */
    BOOL32          bPgSizeBuf;     /**< Is pData aligned to page size          */
    BOOL32          bSpare;         /**< If bSpare is true, spare data should be written or read */
    UINT32          nSData1;        /**< first spare data                       */
    UINT32          nSData2;        /**< second spare data                      */
    UINT32          nSData3;        /**< third spare data                       */
    SBITMAP         nBitmap;        /**< sector bitmap                          */
    UINT32          nNumOfPgs;      /**< number of request pages                */
    VFLExtParam    *pExtParam;      /**< pointer to extended parameters         */
} VFLParam;


/****************************************************************************/
/* Platform dependent memory management functions definition                */
/****************************************************************************/
#define     FSR_STL_MEM_DRAM            (0)
#define     FSR_STL_MEM_SRAM            (1)
#define     FSR_STL_MEM_CACHEABLE       (0)
#define     FSR_STL_MEM_NONCACHEABLE    (1)

/**
 * @brief x = mem_size, 
 *        y = FSR_STL_MEM_CACHABLE/FSR_STL_MEM_NONCACHEABLE 
 *            (NAND write buffer should be noncachable)
 *        z = FSR_STL_MEM_DRAM/FSR_STL_MEM_SRAM 
 *            (Data structures such as handlers should be assigned in SRAM for fast access)
 */
#define     FSR_STL_MALLOC(x, y, z)     FSR_OAM_Malloc(x)

#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/
/* Constant definitions                                                      */
/*****************************************************************************/

/*****************************************************************************/
/* Exported variable declarations                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Exported function prototype                                               */
/*****************************************************************************/

#endif /* __FSR_STL_COMMON_TYPE_H__ */
