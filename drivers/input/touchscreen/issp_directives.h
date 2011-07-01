// filename: ISSP_Directives.h
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
//-----------------------------------------------------------------------------

// --------------------- Compiler Directives ----------------------------------
#ifndef INC_ISSP_DIRECTIVES
#define INC_ISSP_DIRECTIVES

// This directive will enable a Genral Purpose test-point on P1.7
// It can be toggled as needed to measure timing, execution, etc...
// A "Test Point" sets a GPIO pin of the host processor high or low. This GPIO
// pin can be observed with an oscilloscope to verify the timing of key
// programming steps. TPs have been added in main() that set Port 0, pin 1
// high during bulk erase, during each block write and during security write.
// The timing of these programming steps should be verified as correct as part
// of the validation process of the final program.
#define USE_TP
#ifdef USE_TP
#undef USE_TP
#endif

// ****************************************************************************
// **************** USER ATTENTION REQUIRED: PROGRAMMING MODE *****************
// ****************************************************************************
// This directive selects whether code that uses reset programming mode or code
// that uses power cycle programming is use. Reset programming mode uses the
// external reset pin (XRES) to enter programming mode. Power cycle programming
// mode uses the power-on reset to enter programming mode.
// Applying signals to various pins on the target device must be done in a 
// deliberate order when using power cycle mode. Otherwise, high signals to GPIO
// pins on the target will power the PSoC through the protection diodes.

//#define RESET_MODE

// ****************************************************************************
// ****************** USER ATTENTION REQUIRED: TARGET PSOC ********************
// ****************************************************************************
// The directives below enable support for various PSoC devices. The root part
// number to be programmed should be un-commented so that its value becomes
// defined.  All other devices should be commented out.
// Select one device to be supported below:


// **** CY8CTMA30xx devices ****
//#define CY8CTMA300_36LQXI
//#define CY8CTMA300_48LTXI
//#define CY8CTMA300_49FNXI
//#define CY8CTMA300B_36LQXI
//#define CY8CTMA301D_36LQXI
//#define CY8CTMA301D_48LTXI
//#define CY8CTMA300D_36LQXI
//#define CY8CTMA300D_48LTXI
//#define CY8CTMA300D_49FNXI
//#define CY8CTMA301E_36LQXI
//#define CY8CTMA301E_48LTXI
#define CY8CTMA300E_36LQXI
//#define CY8CTMA300E_48LTXI
//#define CY8CTMA300E_49FNXI

//-----------------------------------------------------------------------------
// This section sets the Family that has been selected. These are used to 
// simplify other conditional compilation blocks.
//-----------------------------------------------------------------------------

#ifdef CY8CTMA300_36LQXI
    #define CY8CTMA300
#endif 
#ifdef CY8CTMA300_48LTXI
    #define CY8CTMA300
#endif
#ifdef CY8CTMA300_49FNXI
    #define CY8CTMA300
#endif
#ifdef CY8CTMA300B_36LQXI
    #define CY8CTMA300
#endif
#ifdef CY8CTMA300D_36LQXI
    #define CY8CTMA300
#endif
#ifdef CY8CTMA300D_48LTXI
    #define CY8CTMA300
#endif
#ifdef CY8CTMA300D_49FNXI
    #define CY8CTMA300
#endif
#ifdef CY8CTMA301D_36LQXI
    #define CY8CTMA301D
#endif
#ifdef CY8CTMA301D_48LTXI
    #define CY8CTMA301D
#endif
#ifdef CY8CTMA301E_36LQXI
    #define CY8CTMA301E
#endif
#ifdef CY8CTMA301E_48LTXI
    #define CY8CTMA301E
#endif
#ifdef CY8CTMA300E_36LQXI
    #define CY8CTMA300E
#endif
#ifdef CY8CTMA300E_48LTXI
    #define CY8CTMA300E
#endif
#ifdef CY8CTMA300E_49FNXI
    #define CY8CTMA300E
#endif


// ----------------------------------------------------------------------------
#endif  //(INC_ISSP_DIRECTIVES)
#endif  //(PROJECT_REV_)
//end of file ISSP_Directives.h