/**
 *   @mainpage   Flex Sector Remapper : LinuStoreIII_1.2.0_b035-FSR_1.2.1p1_b129_RC
 *
 *   @section Intro
 *       Flash Translation Layer for Flex-OneNAND and OneNAND
 *    
 *    @section  Copyright
 *            COPYRIGHT. 2007-2008 SAMSUNG ELECTRONICS CO., LTD.               
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
 * @file      FSR_DBG_Stack.c
 * @brief     This file contains the routine for checking the stack usage.
 * @author    SongHo Yoon
 * @date      10-JAN-2007
 * @remark
 * REVISION HISTORY
 * @n  10-JAN-2007 [SongHo Yoon] : modified from stack usage code of XSR
 *
 */
 
/*
  [NOTE]
  In order to measure the stack depth, insert FSR_DBG_STACKUSAGE macro to compiler option
*/

#if defined(FSR_DBG_STACKUSAGE)

#define     FSR_NO_INCLUDE_BML_HEADER
#define     FSR_NO_INCLUDE_STL_HEADER

#include    "FSR.h"

/* maximum number of function name for measuring stack usage */
#define     MAX_STACK_FUNCNAME      100

PRIVATE INT32   gnMaxStackDepth = 0;
PRIVATE INT32   gnCurStackDepth = 0;
PRIVATE INT32   gnStackStartTmp;
PRIVATE INT32   gnStackEndTmp;
PRIVATE UINT8   gszStackStartMsg[MAX_STACK_FUNCNAME];
PRIVATE UINT8   gszStackEndMsg[MAX_STACK_FUNCNAME];
PRIVATE UINT8   gszStackStartMsgTmp[MAX_STACK_FUNCNAME];
PRIVATE UINT8   gszStackEndMsgTmp[MAX_STACK_FUNCNAME];
PRIVATE BOOL32  gbRecordStart = FALSE32;

/**
 * @brief           This function initializes the global variable for measuring stack depth
 *
 * @return          none
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 *
 */
PUBLIC VOID
FSR_DBG_InitStackDepth(VOID)
{
    gnMaxStackDepth    = 0;
    gnCurStackDepth = 0;
    gbRecordStart   = FALSE32;

    FSR_OAM_MEMSET(gszStackStartMsg,    0x00, MAX_STACK_FUNCNAME);
    FSR_OAM_MEMSET(gszStackEndMsg,      0x00, MAX_STACK_FUNCNAME);
    FSR_OAM_MEMSET(gszStackStartMsgTmp, 0x00, MAX_STACK_FUNCNAME);
    FSR_OAM_MEMSET(gszStackEndMsgTmp,   0x00, MAX_STACK_FUNCNAME);
}

/**
 * @brief           This function stores the given stack pointer and the given function
 * @n               name into the global variable to record the start of stack
 *
 * @param[in]       *pnStartAddress : stack start address
 * @param[in]       *pFuncName      : function name
 *
 * @return          none
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 *
 */
PUBLIC VOID
FSR_DBG_RecordStackStart(VOID   *pnStartAddress,
                         UINT8  *pFuncName)
{
    gnStackStartTmp = (UINT32) pnStartAddress - sizeof(UINT32);

    FSR_OAM_MEMCPY(gszStackStartMsgTmp, pFuncName, MAX_STACK_FUNCNAME);

    gszStackStartMsgTmp[MAX_STACK_FUNCNAME - 1] = 0;

    gbRecordStart = TRUE32;
}

/**
 * @brief           This function stores the given stack pointer and the given function
 * @n               name into the global variable to record the end of stack
 *
 * @param[in]       *pnEndAddress   : stack end address
 * @param[in]       *pFuncName      : function name
 *
 * @return          none
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 *
 */
PUBLIC VOID
FSR_DBG_RecordStackEnd(VOID   *pnEndAddress,
                       UINT8  *pFuncName)
{
    if (gbRecordStart == FALSE32)
        return;

    gnStackEndTmp = (UINT32) pnEndAddress + sizeof(UINT32);

    FSR_OAM_MEMCPY(gszStackEndMsgTmp, pFuncName, MAX_STACK_FUNCNAME);
    gszStackEndMsgTmp[MAX_STACK_FUNCNAME - 1] = 0;

    gnCurStackDepth = (gnStackStartTmp - gnStackEndTmp);

    if (gnMaxStackDepth < (gnStackStartTmp - gnStackEndTmp))
    {
        /* update stack depth */
        gnMaxStackDepth = (gnStackStartTmp - gnStackEndTmp);

        /* update the start function name of new stack depth */
        FSR_OAM_MEMCPY(gszStackStartMsg, gszStackStartMsgTmp, MAX_STACK_FUNCNAME);
        gszStackStartMsg[MAX_STACK_FUNCNAME - 1] = 0;

        /* update the end function name of new stack depth */
        FSR_OAM_MEMCPY(gszStackEndMsg, gszStackEndMsgTmp, MAX_STACK_FUNCNAME);
        gszStackEndMsg[MAX_STACK_FUNCNAME - 1] = 0;
            
        FSR_OAM_DbgMsg((VOID *) "[MSC:   ] New stack depth! : %s -- %s: %5d bytes\r\n",
            gszStackStartMsgTmp, gszStackEndMsgTmp, gnMaxStackDepth);
    }
}

/**
 * @brief           This function get the maximum stack depth with the 1st function name
 * @n               and the last function name
 *
 * @param[out]      p1stFuncName   : 1st function name
 * @param[out]      pLastFuncName  : last function name
 * @param[out]      pnStackDepth   : stack depth
 *
 * @return          none
 *
 * @author          SongHo Yoon
 * @version         1.0.0
 *
 */
PUBLIC VOID
FSR_DBG_GetStackUsage(UINT8  *p1stFuncName,
                      UINT8  *pLastFuncName,
                      UINT32 *pnStackDepth)
{
    if (p1stFuncName != NULL)
    {
        FSR_OAM_MEMCPY(p1stFuncName, gszStackStartMsg, MAX_STACK_FUNCNAME);
        p1stFuncName[MAX_STACK_FUNCNAME - 1] = 0;
    }

    if (pLastFuncName != NULL)
    {
        FSR_OAM_MEMCPY(pLastFuncName, gszStackEndMsg, MAX_STACK_FUNCNAME);
        pLastFuncName[MAX_STACK_FUNCNAME - 1] = 0;
    }

    if (pnStackDepth != NULL)
    {
        *pnStackDepth = gnMaxStackDepth;
    }

    FSR_OAM_DbgMsg((VOID *) "[MSC:   ] Maximum stack depth = %5d bytes\r\n", gnMaxStackDepth);
    FSR_OAM_DbgMsg((VOID *) "[MSC:   ] %s -- %s\r\n", gszStackStartMsg, gszStackEndMsg);
}

#endif /* FSR_DBG_STACKUSAGE */

