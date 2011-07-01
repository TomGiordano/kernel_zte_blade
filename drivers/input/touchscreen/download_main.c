// filename: main.c
#include "issp_revision.h"
#ifdef PROJECT_REV_1
/* Copyright 2006-2009, Cypress Semiconductor Corporation.
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
//---------------------------------------------------------------------------*/

/* ############################################################################
   ###################  CRITICAL PROJECT CONSTRAINTS   ########################
   ############################################################################ 

   ISSP programming can only occur within a temperature range of 5C to 50C.
   - This project is written without temperature compensation and using
     programming pulse-widths that match those used by programmers such as the
     Mini-Prog and the ISSP Programmer.
     This means that the die temperature of the PSoC device cannot be outside
     of the above temperature range.
     If a wider temperature range is required, contact your Cypress Semi-
     conductor FAE or sales person for assistance.

   - TMA300 uses an internal regulator for the CPU.  This means that the IMO trim
     values are independent of Vdd.

   - This program supports Indium only.  Other PSoC devices are not supported 
     in this project.
	 
   ############################################################################ 
   ##########################################################################*/

  
/* (((((((((((((((((((((((((((((((((((((())))))))))))))))))))))))))))))))))))))
 PSoC In-System Serial Programming (ISSP) Template
 This PSoC Project is designed to be used as a template for designs that
 require PSoC ISSP Functions.
 
 This project is based on the AN2026 series of Application Notes. That app
 note should be referenced before any modifications to this project are made.
 
 The subroutines and files were created in such a way as to allow easy cut & 
 paste as needed. There are no customer-specific functions in this project. 
 This demo of the code utilizes a PSoC as the Host.
 
 Some of the subroutines could be merged, or otherwise reduced, but they have
 been written as independently as possible so that the specific steps involved
 within each function can easily be seen. By merging things, some code-space 
 savings could be realized. 
 
 As is, and with all features enabled, the project consumes approximately 3500
 bytes of code space, and 19-Bytes of RAM (not including stack usage). The
 Block-Verify requires a 64-Byte buffer for read-back verification. This same
 buffer could be used to hold the (actual) incoming program data.
 
 Please refer to the compiler-directives file "directives.h" to see the various
 features.
 
 The pin used in this project are assigned as shown below. The HOST pins are 
 arbitrary and any 3 pins could be used (the masks used to control the pins 
 must be changed). The TARGET pins cannot be changed, these are fixed function
 pins on the PSoC. 
 The PWR pin is used to provide power to the target device if power cycle
 programming mode is used. The compiler directive RESET_MODE in ISSP_directives.h
 is used to select the programming mode. This pin could control the enable on
 a voltage regulator, or could control the gate of a FET that is used to turn
 the power to the PSoC on.
 The TP pin is a Test Point pin that can be used signal from the host processor
 that the program has completed certain tasks. Predefined test points are
 included that can be used to observe the timing for bulk erasing, block 
 programming and security programming.
 
      SIGNAL  HOST  TARGET
      ---------------------
      SDATA   P1.0   P1.0
      SCLK    P1.1   P1.1
      XRES    P2.0   XRES
      PWR     P2.1   Vdd 
      TP      P0.7   n/a
 
 For test & demonstration, this project generates the program data internally. 
 It does not take-in the data from an external source such as I2C, UART, SPI,
 etc. However, the program was written in such a way to be portable into such
 designs. The spirit of this project was to keep it stripped to the minimum 
 functions required to do the ISSP functions only, thereby making a portable 
 framework for integration with other projects.

 The high-level functions have been written in C in order to be portable to
 other processors. The low-level functions that are processor dependent, such 
 as toggling pins and implementing specific delays, are all found in the file
 ISSP_Drive_Routines.c. These functions must be converted to equivalent
 functions for the HOST processor.  Care must be taken to meet the timing 
 requirements when converting to a new processor. ISSP timing information can
 be found in Application Note AN2026.  All of the sections of this program
 that need to be modified for the host processor have "PROCESSOR_SPECIFIC" in
 the comments. By performing a "Find in files" using "PROCESSOR_SPECIFIC" these
 sections can easily be identified.

 The variables in this project use Hungarian notation. Hungarian prepends a
 lower case letter to each variable that identifies the variable type. The
 prefixes used in this program are defined below:
  b = byte length variable, signed char and unsigned char
  i = 2-byte length variable, signed int and unsigned int
  f = byte length variable used as a flag (TRUE = 0, FALSE != 0)
  ab = an array of byte length variables


 After this program has been ported to the desired host processor the timing
 of the signals must be confirmed.  The maximum SCLK frequency must be checked
 as well as the timing of the bulk erase, block write and security write
 pulses.
 
 The maximum SCLK frequency for the target device can be found in the device 
 datasheet under AC Programming Specifications with a Symbol of "Fsclk".
 An oscilloscope should be used to make sure that no half-cycles (the high 
 time or the low time) are shorter than the half-period of the maximum
 freqency. In other words, if the maximum SCLK frequency is 8MHz, there can be
 no high or low pulses shorter than 1/(2*8MHz), or 62.5 nsec.

 The test point (TP) functions, enabled by the define USE_TP, provide an output
 from the host processor that brackets the timing of the internal bulk erase,
 block write and security write programming pulses. An oscilloscope, along with
 break points in the PSoC ICE Debugger should be used to verify the timing of 
 the programming.  The Application Note, "Host-Sourced Serial Programming"
 explains how to do these measurements and should be consulted for the expected
 timing of the erase and program pulses.

 ############################################################################
   ############################################################################ 

(((((((((((((((((((((((((((((((((((((()))))))))))))))))))))))))))))))))))))) */


/*----------------------------------------------------------------------------
//                               C main line
//----------------------------------------------------------------------------
*/
/*wly delet*/
//#include <m8c.h>        // part specific constants and macros
//#include "PSoCAPI.h"    // PSoC API definitions for all User Modules


// ------ Declarations Associated with ISSP Files & Routines -------
//     Add these to your project as needed.
#include "issp_extern.h"
#include "issp_directives.h"
#include "issp_defs.h"
#include "issp_errors.h"
#include <linux/kernel.h>
#include <asm/uaccess.h>

#include <linux/blkdev.h>
#include <linux/completion.h>
#include <linux/dcache.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fcntl.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/kref.h>
#include <linux/kthread.h>
#include <linux/limits.h>
#include <linux/rwsem.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/freezer.h>
#include <linux/utsname.h>

#include <linux/usb/ch9.h>
#include <linux/usb/gadget.h>

/* ------------------------------------------------------------------------- */

unsigned char bBankCounter;
unsigned int  iBlockCounter;
unsigned int  iChecksumData;
unsigned int  iChecksumTarget;
int update_flag = 0;

/* ========================================================================= */
// ErrorTrap()
// Return is not valid from main for PSOC, so this ErrorTrap routine is used.
// For some systems returning an error code will work best. For those, the 
// calls to ErrorTrap() should be replaced with a return(bErrorNumber). For
// other systems another method of reporting an error could be added to this
// function -- such as reporting over a communcations port.
/* ========================================================================= */
void ErrorTrap(unsigned char bErrorNumber)
{
#ifndef RESET_MODE
    // Set all pins to highZ to avoid back powering the PSoC through the GPIO
    // protection diodes.
    SetSCLKHiZ();   
    SetSDATAHiZ();
    // If Power Cycle programming, turn off the target
    //RemoveTargetVDD();
#endif
     //while (1);
     update_flag = 1;
     printk("ErrorTrap:go to error!\n");
     //return ;
}

/* ========================================================================= */
/* MAIN LOOP                                                                 */
/* Based on the diagram in the AN2026                                        */
/* ========================================================================= */

//firmware transfer

//#define count 280


void download_firmware_main(char* filename)
{
    struct file     *filp;
    struct inode    *inode = NULL;
    int         length, remaining,count;
    mm_segment_t    oldfs;
    
    char *fw_buf = NULL, *buffer;
    update_flag = 0;
    oldfs = get_fs();
    set_fs(KERNEL_DS);

    filp = filp_open(filename, O_RDONLY, S_IRUSR);
    if ( IS_ERR(filp) ) {
        printk("%s: file %s filp_open error\n", __FUNCTION__, filename);
        return;
    }
    if (!filp->f_op) {
        printk("%s: File Operation Method Error\n", __FUNCTION__);
        return;
    }
    inode = filp->f_path.dentry->d_inode;


    if (!inode) {
        printk("%s: Get inode from filp failed\n", __FUNCTION__);
        filp_close(filp, NULL);
        return;
    }

    printk("%s file offset opsition: %xh\n", __FUNCTION__, (unsigned)filp->f_pos);

    length = i_size_read(inode->i_mapping->host);
    if (length == 0) {
        printk("%s: Try to get file size error\n", __FUNCTION__);
        goto Transfer_DONE;
    }
    printk("%s: length=%d\n", __FUNCTION__, length);
		fw_buf = (char*)kmalloc((length+1), GFP_KERNEL);
    if (fw_buf == NULL) {
        printk("%s: kernel memory alloc error\n", __FUNCTION__);
        filp_close(filp, NULL);
        return;
    }
	if (filp->f_op->read(filp, fw_buf, length, &filp->f_pos) != length) {
        printk("%s: file read error\n", __FUNCTION__);
        goto Transfer_DONE;
    }
    // >>>> ISSP Programming Starts Here <<<<
    
    // Initialize the Host & Target for ISSP operations
    fIsError = fPowerCycleInitializeTargetForISSP();
    if (fIsError) {
		printk("power on failed!\n");
        ErrorTrap(fIsError);
        return;

    }
    printk("power on success!\n");
    // Run the SiliconID Verification, and proceed according to result.

    fIsError = fVerifySiliconID();
    if (fIsError ) { 
    	printk("SiliconID Verification failed!\n");
        ErrorTrap(fIsError);
        return;
    }
     printk("SiliconID Verification success!\n");
   
     

//-------------------------------------------------------------------------------------------------------
// xch: the function call below will erase one block (128bytes) of flash
//      in the target device.  The block number to be erased is passed in 
//      as a parameter
//	fEraseBlock(5);  // arbitrarily used the 5th block for debugging purposes
//-------------------------------------------------------------------------------------------------------

	fIsError = fEraseTarget();
    if (fIsError ) {
        ErrorTrap(fIsError);
	        return;
    }
	 printk("erase one block success!\n");

   
    //==============================================================//
    // Program Flash blocks with predetermined data. In the final application
    // this data should come from the HEX output of PSoC Designer.
    //firmware_transfer(tgt_fw);
    remaining = length;
		buffer = fw_buf;
    iChecksumData = 0;     // Calculte the device checksum as you go
    for (bBankCounter=0; bBankCounter<NUM_BANKS; bBankCounter++)		
    {
    	for (iBlockCounter=0; iBlockCounter<BLOCKS_PER_BANK; iBlockCounter++) {
		 	count = (remaining > TARGET_DATABUFF_LEN)? TARGET_DATABUFF_LEN : remaining;
	      	LoadProgramData(bBankCounter, (unsigned char)iBlockCounter, buffer, count);			// This loads the host with test data, not the DUT
		    remaining = remaining - count;
		    buffer = &fw_buf[count * (iBlockCounter+1)];
		    iChecksumData += iLoadTarget();											// This loads the DUT
		            																		
		    #ifdef USE_TP
		    SetTPHigh();    // Only used of Test Points are enabled 
		    #endif
				fIsError = fProgramTargetBlock(bBankCounter,(unsigned char)iBlockCounter);
		   	if (fIsError ) {
		    ErrorTrap(fIsError);
		    return;
		    }
		    #ifdef USE_TP
			SetTPLow();    // Only used of Test Points are enabled  
			#endif			
	  		fIsError = fReadStatus();
	    	if (fIsError ) { 										// READ-STATUS after PROGRAM-AND-VERIFY
	        	ErrorTrap(fIsError);
	        	return;
	    	}
     	}
	
	}       	    
    printk("Program Flash blocks with predetermined data success.\n");
    //=======================================================//
    // Doing Verify
	// Verify included for completeness in case host desires to do a stand-alone verify at a later date.

	remaining = length;
	buffer = fw_buf;
    for (bBankCounter=0; bBankCounter<NUM_BANKS; bBankCounter++)
    {
        for (iBlockCounter=0; iBlockCounter<BLOCKS_PER_BANK; iBlockCounter++) {
	        //LoadProgramData(bBankCounter, (unsigned char) iBlockCounter,buffer,count);
	      	count = (remaining > TARGET_DATABUFF_LEN)? TARGET_DATABUFF_LEN : remaining;
	      	LoadProgramData(bBankCounter, (unsigned char)iBlockCounter, buffer, count);			// This loads the host with test data, not the DUT
		    remaining = remaining - count;
		    buffer = &fw_buf[count * (iBlockCounter+1)];
		    fIsError = fVerifySetup(bBankCounter,(unsigned char)iBlockCounter);
	        if (fIsError ) {
				printk("fVerifySetup err =%d\n",fIsError);
	            ErrorTrap(fIsError);	
	            return;
	        }
	        fIsError = fReadStatus();
			
			if (fIsError ) { 
				printk("fReadStatus err =%d\n",fIsError);
	    		ErrorTrap(fIsError);				
	    		return;			
			}
			fIsError = fReadByteLoop();
			if (fIsError ) {
				printk("fReadByteLoop err =%d\n",fIsError);
				ErrorTrap(fIsError);
				return;
			}
        }
    }

	printk("Doing Verify success.\n");
//-------------------------------------------------------------------------------------------------------
// xch: the function call below will erase one block (128bytes) of flash
//      in the target device.  The block number to be erased is passed in 
//      as a parameter
//	fEraseBlock(5);  // arbitrarily used the 5th block for debugging purposes
//-------------------------------------------------------------------------------------------------------

	//=======================================================//
    // Program security data into target PSoC. In the final application this 
    // data should come from the HEX output of PSoC Designer.
    for (bBankCounter=0; bBankCounter<NUM_BANKS; bBankCounter++)
    {

        // Load one bank of security data from hex file into buffer
        fIsError = fLoadSecurityData(bBankCounter);
        if (fIsError ) {
            ErrorTrap(fIsError);
            return;
        }

		// Secure one bank of the target flash
		fIsError = fSecureTargetFlash();
        if (fIsError ) {
                ErrorTrap(fIsError);
            return;
        }
    }
    printk("set securitydata success\n");
    //==============================================================//
    //Do VERIFY-SECURITY after SECURE    
    //Load one bank of security data from hex file into buffer
    //loads abTargetDataOUT[] with security data that was used in secure bit stream
    fIsError = fLoadSecurityData(bBankCounter);
	if (fIsError ) {
	    ErrorTrap(fIsError);
	    return;
	 }
    
	//if (fIsError == fVerifySecurity()) {
		//ErrorTrap(fIsError);
	//}
	fIsError = fVerifySecurity();
	if (fIsError) {
		ErrorTrap(fIsError);
			return;
	}
	
    //=======================================================//     
    // Doing Checksum after VERIFY-SECURITY
    iChecksumTarget = 0;
    for (bBankCounter=0; bBankCounter<NUM_BANKS; bBankCounter++)
    {
    	fIsError = fAccTargetBankChecksum(&iChecksumTarget);
        if (fIsError ) {
            ErrorTrap(fIsError);
            return;
        }
    }
	
    if (iChecksumTarget != (iChecksumData & 0xFFFF)){
        ErrorTrap(VERIFY_ERROR);
        return;
    }
	printk("checksum success!\n");
    printk("download firmware success!\n");
    if (fIsError)update_flag=1;
    else update_flag = 2;
    // *** SUCCESS *** 
    // At this point, the Target has been successfully Initialize, ID-Checked,
    // Bulk-Erased, Block-Loaded, Block-Programmed, Block-Verified, and Device-
    // Checksum Verified.

    // You may want to restart Your Target PSoC Here.
    //ReStartTarget();//wly delete
  

Transfer_DONE:
    kfree(fw_buf);
    filp_close(filp, NULL);
    set_fs(oldfs);

} 
// end of main()

#endif  //(PROJECT_REV_)
// end of file main.c