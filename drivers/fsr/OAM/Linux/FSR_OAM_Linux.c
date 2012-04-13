/*
   %COPYRIGHT%
 */

/**
 * @version     LinuStoreIII_1.2.0_b035-FSR_1.2.1p1_b129_RC
 * @file        drivers/tfsr/OAM/FSR_OAM_Linux.c
 * @brief       This file contain the OS Adaptation Modules for Linux
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/highmem.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/fsr_if.h>

#include <asm/io.h>
#if defined(CONFIG_ARM)
#include <asm/sizes.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 27)
#include <mach/hardware.h>
#else
#include <asm/hardware.h>
#endif /* LINUX_VERSION_CODE */
#endif /* CONFIG_ARM */


/* FSR include file */
#include    "FSR.h"

/*****************************************************************************/
/* Global variables definitions                                              */
/*****************************************************************************/

/*****************************************************************************/
/* Local #defines                                                            */
/*****************************************************************************/
#ifndef CONFIG_ARM
    #define SZ_128K                         0x00020000
#endif

/*****************************************************************************/
/* Local typedefs                                                            */
/*****************************************************************************/

/*****************************************************************************/
/* Local constant definitions                                                */
/*****************************************************************************/

/*****************************************************************************/
/* Static variables definitions                                              */
/*****************************************************************************/
#if defined(WITH_TINY_FSR)

extern  struct  semaphore fsr_sem[FSR_OAM_MAX_SEMAPHORES];
extern  int     semaphore_use[FSR_OAM_MAX_SEMAPHORES];
extern  int     semaphore_use_bml;
extern  int     semaphore_fsr_bml_handle;

#else

struct  semaphore fsr_sem[FSR_OAM_MAX_SEMAPHORES];
int     semaphore_use[FSR_OAM_MAX_SEMAPHORES] = {0,};
int     semaphore_use_bml = 0;
int     semaphore_fsr_bml_handle = 0;
EXPORT_SYMBOL(fsr_sem);
EXPORT_SYMBOL(semaphore_use);
EXPORT_SYMBOL(semaphore_use_bml);
EXPORT_SYMBOL(semaphore_fsr_bml_handle);

#endif

PRIVATE UINT32  gnFSRHeapUsage    = 0;
PRIVATE UINT32  gnFSRNumOfMemReqs = 0;

// defined for Linux timer
static struct timeval start;
static struct timeval stop;

#if defined(FSR_USE_DUAL_CORE)
PRIVATE     UINT32          gnShMemBaseAddress[FSR_MAX_VOLS]    = {0x01FFA000,0};
PRIVATE     UINT32          gnShMemMaxSize[FSR_MAX_VOLS]        = {0x5000,0};
PRIVATE     UINT32          gnSMallocPtr[FSR_MAX_VOLS]          = {0,0};

#define SHARED_MEMORY_RESET     (0x2)
#endif

/*****************************************************************************/
/* Static function prototypes                                                */
/*****************************************************************************/

/*****************************************************************************/
/* External variables definitions                                            */
/*****************************************************************************/
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef __cplusplus
}
#endif /* __cplusplus */

/*****************************************************************************/
/* External function prototypes                                              */
/*****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern  VOID    memcpy32 (VOID       *pDst,
                          VOID       *pSrc,
                          UINT32     nSize);

#ifdef __cplusplus
}
#endif /* __cplusplus */

/*****************************************************************************/
/* Function Implementation                                                   */
/*****************************************************************************/

static inline int
free_vmem(void *addr)
{
    if (((u32)addr >= VMALLOC_START) && ((u32)addr <= VMALLOC_END))
    {
        vfree(addr);
        return 0;
    }

    return -1;
}

static inline void
*alloc_vmem(unsigned long size)
{
    return vmalloc(size);
}

/**
 * @brief           This function initializes OAM
 *
 * @return          none
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 *
 * @remark          FSR_PAM_Init() should be called before FSR_OAM_Init() is called.
 *                  this function is called by FSR_BML_Init()
 */
PUBLIC INT32
FSR_OAM_Init(VOID)
{
    FSR_STACK_VAR;

    FSR_STACK_END;

    FSR_OAM_InitMemStat();

    return FSR_OAM_SUCCESS;
}

/**
 *
 * @brief           This function initializes  memory allocation statistics.
 *
 * @return          none
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 *
 */
PUBLIC VOID
FSR_OAM_InitMemStat(VOID)
{
    FSR_STACK_VAR;

    FSR_STACK_END;

    gnFSRHeapUsage    = 0;
    gnFSRNumOfMemReqs = 0;
}

/**
 * @brief           This function gets memory statistics
 *
 * @param[out]      pnHeapUsage    : the heap usage
 * @param[out]      pnNumOfMemReqs : the number of memory allocation requests
 *
 * @return          none
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 * @remark          this function is used for getting memory usages
 *
 */
PUBLIC VOID
FSR_OAM_GetMemStat(UINT32  *pnHeapUsage,
                   UINT32  *pnNumOfMemReqs)
{
    FSR_STACK_VAR;

    FSR_STACK_END;

    *pnHeapUsage    = gnFSRHeapUsage;
    *pnNumOfMemReqs = gnFSRNumOfMemReqs;

    FSR_RTL_PRINT((TEXT("[OAM:   ] HeapUsage : %d / NumOfMemReqs : %d\r\n"), gnFSRHeapUsage, gnFSRNumOfMemReqs));
}

/**
 * @brief           This function sets malloc reset pointer
 *
 * @return          none
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 * @remark          internal debug purpose ONLY
 *
 */
PUBLIC VOID
FSR_OAM_SetMResetPoint(UINT32 nMemChunkID)
{
    FSR_STACK_VAR;

    FSR_STACK_END;
}

/**
 * @brief           This function resets malloc pointer
 *
 * @param[in]       bReset    : if TRUE32, Malloc pointer is set as 0
 * 
 * @return          none
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 * @remark          If there is no malloc/free API, use this API instead of FSR_OAM_Free()
 *                  to free all allocated memory. internal debug purpose ONLY
 *
 */
PUBLIC VOID
FSR_OAM_ResetMalloc(UINT32  nMemChunkID,
		    BOOL32  bReset)
{
    FSR_STACK_VAR;

    FSR_STACK_END;
}

/**
 * @brief           This function allocates memory
 *
 * @param[in]       nSize    : Size to be allocated
 * 
 * @return          Pointer of allocated memory
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 *
 */
PUBLIC VOID *
FSR_OAM_Malloc(UINT32 nSize)
{
    void *nPtr;
    FSR_STACK_VAR;

    FSR_STACK_END;
    
    if (nSize > SZ_128K)
        nPtr = alloc_vmem(nSize);
    else
        nPtr = kmalloc(nSize, GFP_KERNEL);

    if (nPtr == NULL)
    {
        printk("%s Fail : nPtr : NULL\r\n", (char *) __FSR_FUNC__);
    }

    return nPtr;
}

/**
 * @brief           This function allocates memory
 *
 * @param[in]       nSize    : Size to be allocated
 * @param[in]       nMemType : Memory type to be allocated
 *
 * @return          Pointer of allocated memory
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 *
 */
PUBLIC VOID*
FSR_OAM_MallocExt(UINT32    nPDev,
                  UINT32    nSize,
                  UINT32    nMemType)
{

#if defined(FSR_USE_DUAL_CORE)
    UINT32  nAlignSize;
#endif
    UINT32  nVol;

    nVol = nPDev / FSR_MAX_VOLS;

    if (nMemType == FSR_OAM_LOCAL_MEM)
    {
        return FSR_OAM_Malloc(nSize);
    }
    else /* nMemType == FSR_OAM_SHARED_MEM */
    {
#if defined(FSR_USE_DUAL_CORE)
        nAlignSize = nSize / sizeof(UINT32);
        if (nSize % sizeof(UINT32))
        {
            nAlignSize++;
        }

        gnSMallocPtr[nVol] += nAlignSize;
        if (gnSMallocPtr[nVol] > gnShMemMaxSize[nVol])
        {
            return NULL;
        }

        return (VOID *) (gnShMemBaseAddress[nVol] + (gnSMallocPtr[nVol] - nAlignSize) * 4);
#else
        return NULL;
#endif
    }
}

/**
 * @brief           This function frees memory
 *
 * @param[in]      *pMem     : Pointer to be free
 * @param[in]       nMemType : Memory type to be free
 *
 * @return         none
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 *
 */
PUBLIC VOID
FSR_OAM_FreeExt(UINT32      nPDev,
                VOID       *pMem,
                UINT32      nMemType)
{
    UINT32  nVol;

    nVol = nPDev / FSR_MAX_VOLS;

    if (nMemType == FSR_OAM_LOCAL_MEM)
    {
        FSR_OAM_Free(pMem);
    }
    else /* nMemType == FSR_OAM_SHARED_MEM */
    {
#if defined(FSR_USE_DUAL_CORE)
        gnSMallocPtr[nVol] = SHARED_MEMORY_RESET;
#endif
    }
}

/**
 * @brief           This function frees memory
 *
 * @param[in]      *pMem : Pointer to be free
 * 
 * @return         none
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 *
 */
PUBLIC VOID
FSR_OAM_Free(VOID  *pMem)
{
    FSR_STACK_VAR;

    FSR_STACK_END;
    
    if (free_vmem(pMem))
        kfree(pMem);
}

/**
 * @brief           This function creates semaphore object.
 *
 * @param[out]     *pHandle : Handle of semaphore
 * @param[in]       nLayer  : 1 : FSR_OAM_SM_TYPE_BDD
 *                            2 : FSR_OAM_SM_TYPE_STL
 *                            3 : FSR_OAM_SM_TYPE_BML_VOL_0
 *                            4 : FSR_OAM_SM_TYPE_LLD
 *                            5 : FSR_OAM_SM_TYPE_BML_VOL_1
 * 
 * @return          TRUE32   : this function creates semaphore successfully
 * @return          FALSE32  : fail
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 * @remark          An initial count of semaphore object is 1.
 *                  An maximum count of semaphore object is 1.
 *
 */
PUBLIC BOOL32
FSR_OAM_CreateSM(SM32   *pHandle,
                 UINT32  nLayer)
{
    BOOL32      bRe = TRUE32;
    int         sem_count = 0;
    FSR_STACK_VAR;

    FSR_STACK_END;


    do
    {
        if ((nLayer == FSR_OAM_SM_TYPE_BML_VOL_0)
	   || (nLayer == FSR_OAM_SM_TYPE_BML_VOL_1))
        {
            if (semaphore_use_bml++ > 0)
            {
                *pHandle = semaphore_fsr_bml_handle;
                break;
            }
        }
    
        for (sem_count = 0; sem_count < FSR_OAM_MAX_SEMAPHORES; sem_count++)
        {
            /* find semaphore */
            if (semaphore_use[sem_count] == 0)
            {
                semaphore_use[sem_count] = 1;
                break;
            }
        }
    
        /* can't find semaphore */
        if (sem_count == FSR_OAM_MAX_SEMAPHORES)
        {
            bRe = FALSE32;
            break;
        }
    
        *pHandle = sem_count;
        if ((nLayer == FSR_OAM_SM_TYPE_BML_VOL_0)
	   || (nLayer == FSR_OAM_SM_TYPE_BML_VOL_1))
        {
            semaphore_fsr_bml_handle = sem_count;
        }

        sema_init(&fsr_sem[sem_count], 1);

    } while (0);       

    return bRe;
}

/**
 * @brief           This function destroys semaphore.
 *
 * @param[in]       nHandle : Handle of semaphore to be destroyed
 * @param[in]       nLayer  : 1 : FSR_OAM_SM_TYPE_BDD
 *                            2 : FSR_OAM_SM_TYPE_STL
 *                            3 : FSR_OAM_SM_TYPE_BML_VOL_0
 *                            4 : FSR_OAM_SM_TYPE_LLD
 *                            5 : FSR_OAM_SM_TYPE_BML_VOL_1
 * 
 * @return          TRUE32   : this function destroys semaphore successfully
 * @return          FALSE32  : fail
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 *
 */
PUBLIC BOOL32
FSR_OAM_DestroySM(SM32        nHandle,
                  UINT32      nLayer)
{
    FSR_STACK_VAR;

    FSR_STACK_END;

    if (semaphore_use[nHandle] != 1)
    {
        return FALSE32;
    }

    if ((nLayer == FSR_OAM_SM_TYPE_BML_VOL_0)
	|| (nLayer == FSR_OAM_SM_TYPE_BML_VOL_1))
    {
        if (--semaphore_use_bml != 0)
        {
            return TRUE32;
        }
    }

    semaphore_use[nHandle] = 0;

    return TRUE32;
}

/**
 * @brief          This function acquires semaphore.
 *
 * @param[in]       nHandle : Handle of semaphore to be acquired
 * @param[in]       nLayer  : 1 : FSR_OAM_SM_TYPE_BDD
 *                            2 : FSR_OAM_SM_TYPE_STL
 *                            3 : FSR_OAM_SM_TYPE_BML_VOL_0
 *                            4 : FSR_OAM_SM_TYPE_LLD
 *                            5 : FSR_OAM_SM_TYPE_BML_VOL_1
 * 
 * @return         TRUE32   : this function acquires semaphore successfully
 * @return         FALSE32  : fail
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 *
 */
PUBLIC BOOL32
FSR_OAM_AcquireSM(SM32        nHandle,
                  UINT32      nLayer)
{
    FSR_STACK_VAR;

    FSR_STACK_END;

    if (semaphore_use[nHandle] != 1)
    {
        return FALSE32;
    }

    /* acquire the lock for critical section */
    down(&fsr_sem[nHandle]);

    return TRUE32;
}

/**
 * @brief           This function releases semaphore.
 *
 * @param[in]       nHandle : Handle of semaphore to be released
 * @param[in]       nLayer  : 1 : FSR_OAM_SM_TYPE_BDD
 *                            2 : FSR_OAM_SM_TYPE_STL
 *                            3 : FSR_OAM_SM_TYPE_BML_VOL_0
 *                            4 : FSR_OAM_SM_TYPE_LLD
 *                            5 : FSR_OAM_SM_TYPE_BML_VOL_1
 *
 * @return          TRUE32   : this function releases semaphore successfully
 * @return          FALSE32  : fail
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 *
 */
PUBLIC BOOL32
FSR_OAM_ReleaseSM(SM32        nHandle,
                  UINT32      nLayer)
{
    FSR_STACK_VAR;

    FSR_STACK_END;

    if (semaphore_use[nHandle] != 1)
    {
        return FALSE32;
    }
   
    up(&fsr_sem[nHandle]);

    return TRUE32;
}

/**
 * @brief           This function prints debug message
 *
 * @param[in]       *pFmt : NULL-terminated string to be printed
 *
 * @return          none
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 * @remark          This function is used when OS can not support debug print API
 *
 */
static UINT8   gaStr[1024 * 8];
 
PUBLIC VOID
FSR_OAM_DbgMsg(VOID  *pFmt, ...)
{
    va_list ap;
/*
    DO NOT use     FSR_STACK_VAR/FSR_STACK_END macro
*/

    va_start(ap, pFmt);
    vsnprintf((char *) gaStr, (size_t) sizeof(gaStr), (char *) pFmt, ap);
    
    printk(gaStr);
    
    va_end(ap);
}

/**
 * @brief           This function gets virtual address for NAND device physical address
 *
 * @param[in]       nPAddr : physical address of NAND device
 *
 * @return          Virtual address of NAND device
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 *
 */
PUBLIC UINT32
FSR_OAM_Pa2Va(UINT32 nPAddr)
{
    unsigned long ioaddr;
    FSR_STACK_VAR;

    FSR_STACK_END;

    ioaddr = (unsigned long) ioremap(nPAddr, SZ_128K);
    return ioaddr;
}

/**
 * @brief           This function waits N msec
 *
 * @param[in]       nNMSec : msec time for waiting
 *
 * @return          none
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 *
 */
PUBLIC VOID
FSR_OAM_WaitNMSec(UINT32 nNMSec)
{
    FSR_STACK_VAR;

    FSR_STACK_END;

    /* should be implemented */
    mdelay(4);
}

/**
 * @brief           This function is called in _IsROPartition function
 *
 * @return          TRUE32  : lock mechanism is used
 * @return          FALSE32 : lock mechanism isn't used
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 *
 */
PUBLIC BOOL32
FSR_OAM_GetROLockFlag(VOID)
{
    FSR_STACK_VAR;

    FSR_STACK_END;
#if defined(FSR_OAM_NO_USE_LOCK_MECHANISM)
    return  FALSE32;
#else
    return  TRUE32;
#endif
}

/**
 * @brief           This function initializes the specified logical interrupt.
 *
 * @param[in]       nLogIntId : logical interrupt ID
 *
 * @return          FSR_OAM_SUCCESS
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 * @remark          this function is used to support non-blocking I/O feature of FSR
 *
 */
PUBLIC INT32
FSR_OAM_InitInt(UINT32 nLogIntId)
{
    UINT32      nPhyIntId;
    INT32       nRe = FSR_OAM_SUCCESS;

    FSR_STACK_VAR;

    FSR_STACK_END;

    nPhyIntId = FSR_PAM_GetPhyIntID(nLogIntId);

    switch (nLogIntId)
    {
        case FSR_INT_ID_NAND_0:        
            /* should be implemented */

            /* set interrupt controller registers */

            /* register ISR for the logical interrupt */

            /* Clear and Disable the logical interrupt */
            FSR_OAM_ClrNDisableInt(nLogIntId);

            break;

        default:
            break;
    }

    return nRe;
}

/**
 * @brief           This function deinitializes the specified logical interrupt.
 *
 * @param[in]       nLogIntId : logical interrupt ID
 *
 * @return          FSR_OAM_SUCCESS
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 * @remark          this function is used to support non-blocking I/O feature of FSR
 *
 */
PUBLIC INT32
FSR_OAM_DeinitInt(UINT32 nLogIntId)
{
    UINT32      nPhyIntId;
    INT32       nRe = FSR_OAM_SUCCESS;

    FSR_STACK_VAR;

    FSR_STACK_END;

    nPhyIntId = FSR_PAM_GetPhyIntID(nLogIntId);

    switch (nLogIntId)
    {
        case FSR_INT_ID_NAND_0:
            /* should be implemented */

            /* set interrupt controller registers */

            /* register ISR for the logical interrupt */

            /* Clear and Disable the logical interrupt */
            FSR_OAM_ClrNDisableInt(nLogIntId);

            break;

        default:
            break;
    }

    return nRe;
}

/**
 * @brief           This function clears/disables the specified interrupt.
 *
 * @param[in]       nLogIntId : logical interrupt ID
 *
 * @return          FSR_OAM_SUCCESS
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 * @remark          this function is used to support non-blocking I/O feature of FSR
 *
 */
PUBLIC INT32
FSR_OAM_ClrNDisableInt(UINT32  nLogIntId)
{
    UINT32      nPhyIntId;
    FSR_STACK_VAR;

    FSR_STACK_END;

    nPhyIntId = FSR_PAM_GetPhyIntID(nLogIntId);

    switch (nLogIntId)
    {
        case FSR_INT_ID_NAND_0:
            /* should be implemented */
        
            /* disable Interrupt */

            break;

        default:
            break;
    }

    return FSR_OAM_SUCCESS;
}

/**
 * @brief           This function clears/enables the specified interrupt.
 *
 * @param[in]       nLogIntId : logical interrupt ID
 *
 * @return          FSR_OAM_SUCCESS
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 * @remark          this function is used to support non-blocking I/O feature of FSR
 *
 */
PUBLIC INT32
FSR_OAM_ClrNEnableInt(UINT32  nLogIntId)
{
    UINT32      nPhyIntId;
    FSR_STACK_VAR;

    FSR_STACK_END;

    nPhyIntId = FSR_PAM_GetPhyIntID(nLogIntId);

    switch (nLogIntId)
    {
        case FSR_INT_ID_NAND_0:
            /* should be implemented */
        
            /* enable interrupt */
            break;

        default:
            break;
    }

    return FSR_OAM_SUCCESS;
}

/**
 * @brief           This function creates the event.
 *
 * @param[out]      *pHandle : event handle
 * 
 * @return          TRUE32 or FALSE32
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 *
 * @remark          this function is used to support non-blocking I/O feature of FSR
 *
 */
PUBLIC BOOL32
FSR_OAM_CreateEvent(UINT32    *pHandle)
{
    /* should be implemented */
    return TRUE32;
}

/**
 * @brief           This function deletes the event.
 *
 * @param[in]       nHandle : event handle
 * 
 * @return          TRUE32 or FALSE32
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 *
 * @remark          this function is used to support non-blocking I/O feature of FSR
 *
 */
PUBLIC BOOL32
FSR_OAM_DeleteEvent(UINT32     nHandle)
{
    /* should be implemented */
    return TRUE32;
}

/**
 * @brief           This function sends the event.
 *
 * @param[in]       nHandle : event handle
 * 
 * @return          TRUE32 or FALSE32
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 *
 * @remark          this function is used to support non-blocking I/O feature of FSR
 *
 */
PUBLIC BOOL32
FSR_OAM_SendEvent(UINT32     nHandle)
{
    /* should be implemented */
    return TRUE32;
}

/**
 * @brief           This function receives the event.
 *
 * @param[in]       nHandle : event handle
 * 
 * @return          TRUE32 or FALSE32
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 *
 * @remark          this function is used to support non-blocking I/O feature of FSR
 *
 */
PUBLIC BOOL32
FSR_OAM_ReceiveEvent(UINT32     nHandle)
{
    /* should be implemented */
    return TRUE32;
}

/**
 * @brief           This function starts timer
 *
 * @return          none
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 * @remark          
 *
 */
PUBLIC VOID
FSR_OAM_StartTimer(VOID)
{
    FSR_STACK_VAR;

    FSR_STACK_END;
    
    /* should be implemented */
    do {
	do_gettimeofday(&start);
    } while (0);
}

/**
 * @brief           This function stops timer
 *
 * @return          none
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 * @remark          
 *
 */
PUBLIC VOID
FSR_OAM_StopTimer(VOID)
{
    FSR_STACK_VAR;

    FSR_STACK_END;
    
    /* should be implemented */
}

/**
 * @brief           This function get the elapsed time (usec)
 *
 * @return          the elapsed time (usec)
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 * @remark          if this function isn't implemented, it should return 0.
 *
 */
PUBLIC UINT32
FSR_OAM_GetElapsedTime(VOID)
{
    FSR_STACK_VAR;

    FSR_STACK_END;

    /* should be implemented */
    do_gettimeofday(&stop);
    if (stop.tv_usec < start.tv_usec)
    {
	return stop.tv_usec += (1000000U - start.tv_usec);
    }
    else
    {
	return stop.tv_usec -= start.tv_usec;
    }

    return -1;
}

/**
 * @brief           This function initializes OMAP2420 DMA channel 1
 *
 * @return          FSR_OAM_SUCCESS;
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 *
 * @remark          FSR_OAM_InitDMA() is called after FSR_OAM_Init() is called in FSR_BML_Init()
 */
PUBLIC INT32
FSR_OAM_InitDMA(VOID)
{
    FSR_STACK_VAR;

    FSR_STACK_END;

    /* initializes DMA registers */

    return FSR_OAM_SUCCESS;
}

/**
 * @brief           This function do read operation by DMA
 *
 * @param[in]       nVirDstAddr : virtual destination address
 * @param[in]       nVirSrcAddr : virtual source address
 * @param[in]       nSize       : size to be invalidated
 *
 * @return          FSR_OAM_SUCCESS
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 *
 */
PUBLIC INT32
FSR_OAM_ReadDMA(UINT32     nVirDstAddr,
                UINT32     nVirSrcAddr,
                UINT32     nSize)
{
    FSR_STACK_VAR;

    FSR_STACK_END;

    memcpy32((void *) nVirDstAddr, (void *) nVirSrcAddr, nSize);

    return FSR_OAM_SUCCESS;
}

/**
 * @brief           This function do write operation by DMA
 *
 * @param[in]       nVirDstAddr : virtual destination address
 * @param[in]       nVirSrcAddr : virtual source address
 * @param[in]       nSize       : size to be invalidated
 *
 * @return          FSR_OAM_SUCCESS
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 *
 */
PUBLIC INT32
FSR_OAM_WriteDMA(UINT32     nVirDstAddr,
                 UINT32     nVirSrcAddr,
                 UINT32     nSize)
{
    FSR_STACK_VAR;

    FSR_STACK_END;

    memcpy32((void *) nVirDstAddr, (void *) nVirSrcAddr, nSize);

    return FSR_OAM_SUCCESS;
}
