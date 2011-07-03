// filename: ISSP_Defs.h
#include "issp_revision.h"
#ifdef PROJECT_REV_1
// Copyright 2006-2009, Cypress Semiconductor Corporation.
//
// This software is owned by Cypress Semiconductor Corporation (Cypress)
// and is protected by and subject to worldwide patent protection (United
// States and foreign), United States copyright laws and international 
// treaty provisions. Cypress hereby grants to licensee a personal, 
// non-exclusive, non-transferable license to copy, use, modify, create 
// derivative works of, and compile the Cypress Source Code and derivative 
// works for the sole purpose of creating custom software in support of 
// licensee product to be used only in conjunction with a Cypress integrated 
// circuit as specified in the applicable agreement. Any reproduction, 
// modification, translation, compilation, or representation of this 
// software except as specified above is prohibited without the express 
// written permission of Cypress.
//
// Disclaimer: CYPRESS MAKES NO WARRANTY OF ANY KIND,EXPRESS OR IMPLIED, 
// WITH REGARD TO THIS MATERIAL, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
// Cypress reserves the right to make changes without further notice to the
// materials described herein. Cypress does not assume any liability arising
// out of the application or use of any product or circuit described herein.
// Cypress does not authorize its products for use as critical components in
// life-support systems where a malfunction or failure may reasonably be
// expected to result in significant injury to the user. The inclusion of
// Cypress� product in a life-support systems application implies that the
// manufacturer assumes all risk of such use and in doing so indemnifies
// Cypress against all charges.
//
// Use may be limited by and subject to the applicable Cypress software
// license agreement.
//
//--------------------------------------------------------------------------
#ifndef INC_ISSP_DEFS
#define INC_ISSP_DEFS

#include "issp_directives.h"

//  flash block is 128 bytes. Note Block-Verify Uses 64-Bytes of RAM
#define TARGET_DATABUFF_LEN    128  

#if 1 //wly
#define SCLK_GPIO		60
#define SDATA_GPIO		61
#define VDD_GPIO		31
#endif			
											
// The number of Flash blocks in each part is defined here. This is used in
// main programming loop when programming and verifying the blocks.

#ifdef CY8CTMA300
    #define NUM_BANKS                     1
    #define BLOCKS_PER_BANK             256
    #define SECURITY_BYTES_PER_BANK      64
#endif
#ifdef CY8CTMA301D
    #define NUM_BANKS                     1
    #define BLOCKS_PER_BANK             128
    #define SECURITY_BYTES_PER_BANK      64
#endif
#ifdef CY8CTMA300E
    #define NUM_BANKS                     1
    #define BLOCKS_PER_BANK             256
    #define SECURITY_BYTES_PER_BANK      64
	#define ACTIVE_LOW_XRES
#endif
#ifdef CY8CTMA301E
    #define NUM_BANKS                     1
    #define BLOCKS_PER_BANK             128
    #define SECURITY_BYTES_PER_BANK      64
	#define ACTIVE_LOW_XRES
#endif

#endif //(INC_ISSP_DEFS)
#endif //(PROJECT_REV_)
//end of file ISSP_Defs.h