// filename: ISSP_Extern.h
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
#ifndef INC_ISSP_EXTERN
#define INC_ISSP_EXTERN

#include "issp_directives.h"
extern void ErrorTrap(unsigned char bErrorNumber);
extern void download_firmware_main(char* filename);  //wly add
extern signed char fXRESInitializeTargetForISSP(void);
extern signed char fPowerCycleInitializeTargetForISSP(void);
extern signed char fEraseTarget(void);
extern unsigned int iLoadTarget(void);
extern void ReStartTarget(void);
extern signed char fVerifySiliconID(void);
extern signed char fAccTargetBankChecksum(unsigned int*);
extern void SetBankNumber(unsigned char);
extern signed char fProgramTargetBlock(unsigned char, unsigned char);
extern signed char fVerifyTargetBlock(unsigned char, unsigned char);
extern signed char fVerifySetup(unsigned char, unsigned char);	//PTJ: VERIFY-SETUP
extern signed char fReadByteLoop(void);							//PTJ: read bytes after VERIFY-SETUP
extern signed char fSecureTargetFlash(void);					
extern signed char fEraseBlock(unsigned char bBlockNumber);

extern signed char fReadStatus(void);									//PTJ: READ-STATUS
						
extern signed char fReadWriteSetup(void);								//PTJ: READ-WRITE-SETUP

extern signed char fVerifySecurity(void);

//extern void InitTargetTestData(void);
extern void InitTargetTestData(unsigned char bBlockNum, unsigned char bBankNum,const char *buffer, unsigned long count);//wly add
extern void LoadArrayWithSecurityData(unsigned char, unsigned char, unsigned char);

//extern void LoadProgramData(unsigned char, unsigned char);
extern void LoadProgramData(unsigned char bBlockNum, unsigned char bBankNum, const char *buffer, unsigned long count);
extern signed char fLoadSecurityData(unsigned char);
extern void Delay(unsigned char);
extern unsigned char fSDATACheck(void);
extern void SCLKHigh(void);
extern void SCLKLow(void);
#ifndef RESET_MODE  //only needed when power cycle mode 
  extern void SetSCLKHiZ(void);
#endif 
extern void SetSCLKStrong(void);
extern void SetSDATAHigh(void);
extern void SetSDATALow(void);
extern void SetSDATAHiZ(void);
extern void SetSDATAStrong(void);
extern void AssertXRES(void);
extern void DeassertXRES(void);
extern void SetXRESStrong(void);
extern void ApplyTargetVDD(void);
extern void RemoveTargetVDD(void);
extern void SetTargetVDDStrong(void);

extern unsigned char   fIsError;

#ifdef USE_TP
extern void InitTP(void);
extern void SetTPHigh(void);
extern void SetTPLow(void);
extern void ToggleTP(void);
#endif

#endif  //(INC_ISSP_EXTERN)
#endif  //(PROJECT_REV_)
//end of file ISSP_Extern.h