// filename: ISSP_Errors.h
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
#ifndef INC_ISSP_ERRORS
#define INC_ISSP_ERRORS

// The following are defines for error messages from the ISSP program.
#define PASS           0
// PASS is used to indicate that a function completed successfully.
#define ERROR         -1
// ERROR is a generic failure used within lower level functions before the
// error is reported.  This should not be seen as an error that is reported
// from main.
#define INIT_ERROR     1
// INIT_ERROR means a step in chip initialization failed.
#define SiID_ERROR     2
// SiID_ERROR means that the Silicon ID check failed. This happens if the
// target part does not match the device type that the ISSP program is
// configured for.
#define ERASE_ERROR    3
// ERASE_ERROR means that the bulk erase step failed. 
#define BLOCK_ERROR    4
// BLOCK_ERROR means that a step in programming a Flash block or the verify
// of the block failed.
#define VERIFY_ERROR   5
// VERIFY_ERROR means that the checksum verification failed.
#define SECURITY_ERROR 6
// SECURITY_ERROR means that the write of the security information failed.
#define STATUS_ERROR 7


#endif  //(INC_ISSP_ERRORS)
#endif  //(PROJECT_REV_)
//end of file ISSP_Errors.h