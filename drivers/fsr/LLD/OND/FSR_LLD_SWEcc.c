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
 * @file      FSR_LLD_SWEcc.c
 * @brief     SW Error Correction Code (Hamming Algorithm)
 * @author    JeongWook Moon
 * @date      08-MAY-2007
 * @remark
 * REVISION HISTORY
 * @n  27-DEC-2002 [Kwangyoon Lee]  : first writing
 * @n  15-JUL-2003 [SeWook Na]      : code modification
 * @n  10-AUG-2003 [Janghwan Kim]   : add codes
 * @n  11-AUG-2003 [Janghwan Kim]   : code optimization
 * @n  15-AUG-2003 [Janghwan Kim]   : bug fix
 * @n  02-OCT-2003 [Janghwan Kim]   : reorganization
 * @n  13-NOV-2003 [Chang JongBaek] : Rewriting with new algorithm
 * @n  26-NOV-2003 [JangHwan Kim]   : Reoranization
 * @n  27-NOV-2003 [JangHwan Kim]   : Add Gen, Comp function for Spare Array
 * @n  31-JAN-2006 [ByoungYoung Ahn]: added SWECC_R_ERROR for read disturbance
 * @n  31-JAN-2007 [Younwon Park]   : modified debug message
 * @n  08-MAY-2007 [JeongWook Moon] : delete pre-code and add ECC parity code of 6bytes
 *
 */


/*****************************************************************************/
/* Header file inclusions                                                    */
/*****************************************************************************/
#define     FSR_NO_INCLUDE_BML_HEADER
#define     FSR_NO_INCLUDE_STL_HEADER

#include    "FSR.h"

#include    "FSR_LLD_OneNAND.h"
#include    "FSR_LLD_SWEcc.h"

/*****************************************************************************/
/* Function Redirection                                                      */
/*****************************************************************************/

/*****************************************************************************/
/* Local Configuration                                                       */
/*****************************************************************************/
#undef  LOOP_FOR_ECC

/*****************************************************************************/
/* Local Function Declarations                                               */
/*****************************************************************************/

/**
 * @brief          This function generates 3 byte ECC for 6 byte data. 
 *                 (Software ECC)
 *
 * @param[in]      pEcc     : The memory location for given ECC val 
 * @param[in]      pBuf     : The memory location for given data
 *
 * @return         none
 *
 * @author         JeongWook Moon
 * @version        1.0.0
 * @remark
 *
 */
PUBLIC VOID
FSR_OND_ECC_GenS (volatile UINT16 *pEcc, 
                           UINT8  *pBuf)
{
    UINT32  nTmp;
    UINT32  nEcc  = 0;
    UINT32  nXorT = 0;
    UINT32  nP8   = 0;
    UINT32  nP16  = 0;
    UINT32  nP32  = 0;
    UINT8  *pDat8;

    FSR_STACK_VAR;

    FSR_STACK_END;

    pDat8 = pBuf;

    nXorT = (UINT8)(pDat8[0] ^ pDat8[1] ^ pDat8[2] ^ pDat8[3] ^ pDat8[4] ^ pDat8[5] ^ pDat8[6] ^ pDat8[7]);

    nP8   = (UINT8)(pDat8[1] ^ pDat8[3] ^ pDat8[5] ^ pDat8[7]);
    nP16  = (UINT8)(pDat8[2] ^ pDat8[3] ^ pDat8[6] ^ pDat8[7]);
    nP32  = (UINT8)(pDat8[4] ^ pDat8[5] ^ pDat8[6] ^ pDat8[7]);

    
    /*
     * <Generate ECC code of spare data about 6 byte>
     *
     * +-------+-------+-------+-------+-------+-------+-------+-------+-------+
     * |       | I/O 7 | I/O 6 | I/O 5 | I/O 4 | I/O 3 | I/O 2 | I/O 1 | I/O 0 |
     * +-------+-------+-------+-------+-------+-------+-------+-------+-------+
     * S_ECC 0 |  P8   |  P8'  |  P4   |  P4'  |  P2   |  P2'  |  P1   |  P1'  |
     * +-------+-------------------------------+-------+-------+-------+-------+
     * S_ECC 1 |            (reserved)         | P32   | P32'  | P16   | P16'  | 
     * +-------+-------------------------------+-------+-------+-------+-------+
     */
    /* 
     * Column bit set
     */
    nTmp  = (nP32  <<  4) ^ nP32;                   /********/
    nTmp  = (nTmp  <<  2) ^ nTmp;                   /* p32  */
    nTmp  = (nTmp  <<  1) ^ nTmp;                   /*      */
    nEcc |= (nTmp  <<  4) & 0x800;                  /********/

    nTmp  = (nP16  <<  4) ^ nP16;                   /********/
    nTmp  = (nTmp  <<  2) ^ nTmp;                   /* p16  */
    nTmp  = (nTmp  <<  1) ^ nTmp;                   /*      */
    nEcc |= (nTmp  <<  2) & 0x200;                  /********/

    nTmp  = (nP8   <<  4) ^ nP8;                    /********/
    nTmp  = (nTmp  <<  2) ^ nTmp;                   /*  p8  */
    nEcc |= ((nTmp <<  1) ^ nTmp) & 0x80;           /********/

    /* 
     * Row bit set 
     */
    nTmp  = (nXorT   & 0xF0);                       /********/
    nTmp ^= (nTmp    >> 2);                         /*  P4  */
    nEcc |= ((nTmp   << 1) ^ nTmp) & 0x20;          /********/

    nTmp  = (nXorT   & 0xCC);                       /********/
    nTmp ^= (nTmp    >> 4);                         /*  p2  */
    //nTmp  = (nTmp    << 2);                         /*      */
    nEcc |= ((nTmp   << 1) ^ nTmp) & 0x08;          /********/
                     
    nTmp  = (nXorT   & 0xAA);                       /********/
    nTmp ^= (nTmp    >> 4);                         /*  p1  */
    nTmp ^= (nTmp    >> 2);                         /*      */
    nEcc |= (nTmp    & 0x02);                       /********/


    nXorT ^= (nXorT  >> 4);                         /********/
    nXorT ^= (nXorT  >> 2);                         /*nXorT */
    nXorT  = (UINT8)(((nXorT >> 1) ^ nXorT) & 0x01);/********/

    if (nXorT)
    {
        nEcc |= (nEcc ^ 0xAAAA) >> 1;
    }
    else
    {
        nEcc |= nEcc >> 1;
    }

    /* It is policy of samsung elec. to save invert value */
    nEcc = (UINT16) (~nEcc | 0xF000);

    *(UINT16 *)pEcc = (UINT16)(nEcc);

}

/**
 * @brief          This function corrects and detects data bit error for spare area data 
 *                 (16B size data)
 *
 * @param[in]     *pEcc2    : The memory location for given ECC val2 (Target) 
 * @param[in]     *pBuf     : The memory location for given data     (Target)
 * @param[in]      nCnt     : Sector number
 *
 * @return         SWECC_N_ERROR    : No error
 * @return         SWECC_E_ERROR    : Ecc Value error
 * @return         SWECC_C_ERROR    : Correctable Error (One bit data error)
 * @return         SWECC_U_ERROR    : Uncorrectable Error (More than Two bit data error)
 * @return         SWECC_R_ERROR    : Correctable Error (One bit data error) by Read Disturbance
 *
 * @author         JeongWook Moon
 * @version        1.0.0
 * @remark
 *
 */
PUBLIC INT32 
FSR_OND_ECC_CompS (UINT8  *pEcc2, 
                   UINT8  *pBuf,
                   UINT32  nSectNum)
{
    UINT8  *pEcc1;
    UINT32  nEccComp = 0; 
    UINT32  nEccSum  = 0;
    UINT32  nEBit    = 0;
    UINT32  nEByte   = 0;
    UINT32  nXorT1   = 0;
    UINT32  nXorT2   = 0;
    INT32   nErr;
#if defined (LOOP_FOR_ECC)
    UINT32  nCnt;
#endif

    UINT16 nEccFN901;
    UINT16 nEccFN902;
    UINT16 nEcc1;
    UINT16 nEcc2;

    FSR_STACK_VAR;

    FSR_STACK_END;

    if (pBuf == NULL)
    {
        FSR_DBZ_RTLMOUT(FSR_DBZ_ERROR,
            (TEXT("[OND:ERR] ++%s : Invalid buffer pointer \r\n"), __FSR_FUNC__));

        return FSR_LLD_INVALID_PARAM;
    }

    FSR_OND_ECC_GenS (&nEccFN901, pBuf);

    /*
     * <ECC code of spare data about 6 byte>
     *
     * +-------+-------+-------+-------+-------+-------+-------+-------+-------+
     * |       | I/O 7 | I/O 6 | I/O 5 | I/O 4 | I/O 3 | I/O 2 | I/O 1 | I/O 0 |
     * +-------+-------+-------+-------+-------+-------+-------+-------+-------+
     * S_ECC 0 |  P8   |  P8'  |  P4   |  P4'  |  P2   |  P2'  |  P1   |  P1'  |
     * +-------+-------------------------------+-------+-------+-------+-------+
     * S_ECC 1 |            (reserved)         | P32   | P32'  | P16   | P16'  | 
     * +-------+-------------------------------+-------+-------+-------+-------+
     */
    /* line up the ECC bit array */
    pEcc1     = (UINT8 *) &nEccFN901;
    nEccFN902 = (pEcc2[1] << 8) | (pEcc2[0]);

    nEcc1 = nEccFN901 & 0x0FFF;
    nEcc2 = nEccFN902 & 0x0FFF;

    FSR_DBZ_DBGMOUT (FSR_DBZ_LLD_IF, 
                     (TEXT("[ECC:MSG] nEcc1 = %x nEccFN901 = %x \n"), nEcc1, nEccFN901));
    FSR_DBZ_DBGMOUT (FSR_DBZ_LLD_IF, 
                     (TEXT("[ECC:MSG] nEcc2 = %x nEccFN902 = %x \n"), nEcc2, nEccFN902));


    if (nEcc1 == nEcc2)
    {
        return (FSR_OND_SWECC_N_ERROR);
    }

#if defined (LOOP_FOR_ECC)
    for (nCnt = 0; nCnt < 2; nCnt++)
    {
        nXorT1 ^= (((*pEcc1) >> nCnt) & 0x01);
        nXorT2 ^= (((*pEcc2) >> nCnt) & 0x01);
    }
#else
    nXorT1 ^= ((*pEcc1) & 0x01);
    nXorT1 ^= (((*pEcc1) >> 1) & 0x01);
    nXorT2 ^= ((*pEcc2) & 0x01);
    nXorT2 ^= (((*pEcc2) >> 1) & 0x01);
#endif

#if defined (LOOP_FOR_ECC)
    /* compare written data with read data */
    for (nCnt = 0; nCnt < 2; nCnt++)
    {
        nEccComp |= ((~pEcc1[nCnt] ^ ~pEcc2[nCnt]) << (nCnt * 8));
    }
#else
    nEccComp |=  (~pEcc1[0] ^ ~pEcc2[0]);
    nEccComp |= ((UINT16)(~pEcc1[1] ^ ~pEcc2[1]) << 8);
#endif

#if defined (LOOP_FOR_ECC)
    /* find the number of '1' */
    for(nCnt = 0; nCnt < 12; nCnt++) 
    {
        nEccSum += ((nEccComp >> nCnt) & 0x01);
    }
#else
    nEccSum += ( nEccComp        & 0x01);
    nEccSum += ((nEccComp >>  1) & 0x01);
    nEccSum += ((nEccComp >>  2) & 0x01);
    nEccSum += ((nEccComp >>  3) & 0x01);
    nEccSum += ((nEccComp >>  4) & 0x01);
    nEccSum += ((nEccComp >>  5) & 0x01);
    nEccSum += ((nEccComp >>  6) & 0x01);
    nEccSum += ((nEccComp >>  7) & 0x01);
    nEccSum += ((nEccComp >>  8) & 0x01);
    nEccSum += ((nEccComp >>  9) & 0x01);
    nEccSum += ((nEccComp >> 10) & 0x01);
    nEccSum += ((nEccComp >> 11) & 0x01);
#endif


    switch (nEccSum) 
    {
        case 0 :
            FSR_DBZ_DBGMOUT (FSR_DBZ_LLD_IF, 
                             (TEXT("[ECC:MSG] No Error for Spare\n")));
            return (FSR_OND_SWECC_N_ERROR);

        case 1 :
            FSR_DBZ_RTLMOUT (FSR_DBZ_LLD_IF | FSR_DBZ_ERROR, 
                             (TEXT("[ECC:MSG] Ecc Errr for Spare\n")));
            return (FSR_OND_SWECC_E_ERROR);

        case 6 :
            if (nXorT1 != nXorT2)
            {
                FSR_DBZ_DBGMOUT (FSR_DBZ_LLD_IF, 
                                 (TEXT("[ECC:MSG] Correctable ECC Error Occurs for Spare\n")));

                nEByte  = (UINT8) ((nEccComp >> 9) & 0x04) + 
                          ((nEccComp >> 8) & 0x02) + ((nEccComp >>  7) & 0x01);
                nEBit   = (UINT8)(((nEccComp >>  3) & 0x04) +
                          ((nEccComp >>  2) & 0x02) + ((nEccComp >>  1) & 0x01));

                FSR_DBZ_DBGMOUT (FSR_DBZ_LLD_IF, 
                                 (TEXT("[ECC:MSG] ECC Position : %dth byte, %dth bit\n"), nEByte, nEBit));
                
                nErr = FSR_OND_SWECC_C_ERROR;
                
                if (pBuf != NULL)
                {
                    FSR_DBZ_DBGMOUT (FSR_DBZ_LLD_IF, 
                                     (TEXT("[ECC:MSG]  Corrupted : 0x%02xth \n"), pBuf[nEByte]));

                    pBuf[nEByte]  = (UINT8)(pBuf[nEByte] ^ (1 << nEBit));

                    FSR_DBZ_DBGMOUT (FSR_DBZ_LLD_IF, 
                                     (TEXT("[ECC:MSG]  Corrected : 0x%02xth \n"), pBuf[nEByte]));
                    
                    if ((pBuf[nEByte] & (1 << nEBit))!=0)
                    {
                        nErr = FSR_OND_SWECC_R_ERROR;
                        FSR_DBZ_DBGMOUT (FSR_DBZ_LLD_IF | FSR_DBZ_ERROR, 
                            (TEXT("[ECC:MSG]  %d sector : One bit error by read disturbance \r\n"), nSectNum));
                    }
                }
                return nErr;
            }
            else
            {
                FSR_DBZ_RTLMOUT (FSR_DBZ_LLD_IF | FSR_DBZ_ERROR, 
                             (TEXT("[ECC:MSG]  Ecc Error : %d sector,  %s, %d \r\n"), nSectNum, __FSR_FUNC__, __LINE__));

                FSR_DBZ_RTLMOUT (FSR_DBZ_LLD_IF | FSR_DBZ_ERROR, 
                                 (TEXT("[ECC:MSG]  Uncorrectable ECC Error Occurs for Spare\n")));
                return (FSR_OND_SWECC_U_ERROR);
            }

        default :
            FSR_DBZ_RTLMOUT (FSR_DBZ_LLD_IF | FSR_DBZ_ERROR, 
                             (TEXT("[ECC:MSG]  Ecc Error : %d sector,  %s, %d \r\n"), nSectNum, __FSR_FUNC__, __LINE__));
        
            FSR_DBZ_RTLMOUT (FSR_DBZ_LLD_IF | FSR_DBZ_ERROR, 
                             (TEXT("[ECC:MSG]  Uncorrectable ECC Error Occurs for Spare\n")));
            return (FSR_OND_SWECC_U_ERROR);
    }   
}
