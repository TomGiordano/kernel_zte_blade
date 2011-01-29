// filename: ISSP_Vectors.h
#include "issp_revision.h"
#ifdef PROJECT_REV_1
// Copyright 2006-2009, Cypress Semiconductor Corporation.
//read_write_setup
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
#ifndef INC_ISSP_VECTORS
#define INC_ISSP_VECTORS

#include "issp_directives.h"

unsigned char target_status00_v = 0x00;		// Status = 00 means Success, the SROM function did what it was supposed to 
unsigned char target_status01_v = 0x01;		// Status = 01 means that function is not allowed because of block level protection, for test with verify_setup (VERIFY-SETUP)
unsigned char target_status03_v = 0x03;		// Status = 03 is fatal error, SROM halted
unsigned char target_status04_v = 0x04;		// Status = 04 means that ___ for test with ___ (PROGRAM-AND-VERIFY) 
unsigned char target_status06_v = 0x06;		// Status = 06 means that Calibrate1 failed, for test with id_setup_1 (ID-SETUP-1)

// ----------------------------------------------------------------------------
// Modifying these tables is NOT recommendended. Doing so will all but
// guarantee an ISSP error, unless updated vectors have been recommended or
// provided by Cypress Semiconductor.
// ----------------------------------------------------------------------------
#ifdef CY8CTMA300_36LQXI
    unsigned char target_id_v[] = {0x05, 0x71};     //ID for CY8CTMA300_36LQXI
#endif
#ifdef CY8CTMA300_48LTXI
    unsigned char target_id_v[] = {0x05, 0x72};     //ID for CY8CTMA300_48LTXI
#endif
#ifdef CY8CTMA300_49FNXI
    unsigned char target_id_v[] = {0x05, 0x73};     //ID for CY8CTMA300_49FNXI
#endif
#ifdef CY8CTMA300B_36LQXI
    unsigned char target_id_v[] = {0x05, 0x74};     //ID for CY8CTMA300B_36LQXI
#endif
#ifdef CY8CTMA301D_36LQXI
    unsigned char target_id_v[] = {0x05, 0x77};     //ID for CY8CTMA301D_36LQXI
#endif
#ifdef CY8CTMA301D_48LTXI
    unsigned char target_id_v[] = {0x05, 0x78};     //ID for CY8CTMA301D_48LTXI
#endif
#ifdef CY8CTMA300D_36LQXI
    unsigned char target_id_v[] = {0x05, 0x79};     //ID for CY8CTMA300D_36LQXI
#endif
#ifdef CY8CTMA300D_48LTXI
    unsigned char target_id_v[] = {0x05, 0x80};     //ID for CY8CTMA300D_48LTXI
#endif
#ifdef CY8CTMA300D_49FNXI
    unsigned char target_id_v[] = {0x05, 0x81};     //ID for CY8CTMA300D_49FNXI
#endif
#ifdef CY8CTMA301E_36LQXI
    unsigned char target_id_v[] = {0x05, 0x85};     //ID for CY8CTMA301E_36LQXI
#endif
#ifdef CY8CTMA301E_48LTXI
    unsigned char target_id_v[] = {0x05, 0x86};     //ID for CY8CTMA301E_48LTXI
#endif
#ifdef CY8CTMA300E_36LQXI
    unsigned char target_id_v[] = {0x05, 0x82};     //ID for CY8CTMA300E_36LQXI
#endif
#ifdef CY8CTMA300E_48LTXI
    unsigned char target_id_v[] = {0x05, 0x83};     //ID for CY8CTMA300E_48LTXI
#endif
#ifdef CY8CTMA300E_49FNXI
    unsigned char target_id_v[] = {0x05, 0x84};     //ID for CY8CTMA300E_49FNXI
#endif

// ----------------- CY8CTMA300 Checksum Vectors ------------------------------
// Modifying these tables is NOT recommendended. Doing so will all but
// guarantee an ISSP error, unless updated vectors have been recommended or
// provided by Cypress Semiconductor.
// ----------------------------------------------------------------------------
    const unsigned int num_bits_checksum = 418;
    const unsigned char checksum_v[] =
    {
		0xDE, 0xE2, 0x1F, 0x7F, 0x02, 0x7D, 0xC4, 0x09,
		0xF7, 0x00, 0x1F, 0x9F, 0x07, 0x5E, 0x7C, 0x81,
		0xF9, 0xF4, 0x01, 0xF7, 0xF0, 0x07, 0xDC, 0x40,
		0x1F, 0x70, 0x01, 0xFD, 0xEE, 0x01, 0xF7, 0xA0,
		0x1F, 0xDE, 0xA0, 0x1F, 0x7B, 0x00, 0x7D, 0xE0,
		0x0F, 0xF7, 0xC0, 0x07, 0xDF, 0x28, 0x1F, 0x7D,
		0x18, 0x7D, 0xFE, 0x25, 0xC0
    };

// -------------------- CY8CTMA300 Program Block Vectors ----------------------
// Modifying these tables is NOT recommendended. Doing so will all but
// guarantee an ISSP error, unless updated vectors have been recommended or
// provided by Cypress Semiconductor.
// ----------------------------------------------------------------------------
    const unsigned int num_bits_program_block = 440;
    const unsigned char program_block[] =
    {
		0xDE, 0xE2, 0x1F, 0x7F, 0x02, 0x7D, 0xC4, 0x09,
		0xF7, 0x00, 0x1F, 0x9F, 0x07, 0x5E, 0x7C, 0x81,
		0xF9, 0xF7, 0x01, 0xF7, 0xF0, 0x07, 0xDC, 0x40,
		0x1F, 0x70, 0x01, 0xFD, 0xEE, 0x01, 0xF6, 0xA0,
		0x0F, 0xDE, 0x80, 0x7F, 0x7A, 0x80, 0x7D, 0xEC,
		0x01, 0xF7, 0x80, 0x57, 0xDF, 0x00, 0x1F, 0x7C,
		0xA0, 0x7D, 0xF4, 0x61, 0xF7, 0xF8, 0x97
    };

// ----------------------- CCY8CTMA300 Init-1 --------------------------------
// Modifying these tables is NOT recommendended. Doing so will all but
// guarantee an ISSP error, unless updated vectors have been recommended or
// provided by Cypress Semiconductor.
// ----------------------------------------------------------------------------
    const unsigned int num_bits_id_setup_1 = 616;
    const unsigned char id_setup_1[] =
    {
		0xCA, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x0D, 0xEE, 0x21, 0xF7, 0xF0, 0x27, 0xDC, 0x40,
		0x9F, 0x70, 0x01, 0xFD, 0xEE, 0x01, 0xE7, 0xC1,
		0xD7, 0x9F, 0x20, 0x7E, 0x3F, 0x9D, 0x78, 0xF6,
		0x21, 0xF7, 0xB8, 0x87, 0xDF, 0xC0, 0x1F, 0x71,
		0x00, 0x7D, 0xC0, 0x07, 0xF7, 0xB8, 0x07, 0xDE,
		0x80, 0x7F, 0x7A, 0x80, 0x7D, 0xEC, 0x01, 0xF7,
		0x80, 0x4F, 0xDF, 0x00, 0x1F, 0x7C, 0xA0, 0x7D,
		0xF4, 0x61, 0xF7, 0xF8, 0x97
    };
// ------------ General PSoC Vectors (for TMG300, TST300, TMA300)--------------
// Modifying these tables is NOT recommendended. Doing so will all but
// guarantee an ISSP error, unless updated vectors have been recommended or
// provided by Cypress Semiconductor.
// ----------------------------------------------------------------------------

	const unsigned int num_bits_id_setup_2 = 418;
	const unsigned char id_setup_2[] =
    {
		0xDE, 0xE2, 0x1F, 0x7F, 0x02, 0x7D, 0xC4, 0x09,
		0xF7, 0x00, 0x1F, 0x9F, 0x07, 0x5E, 0x7C, 0x81,
		0xF9, 0xF4, 0x01, 0xF7, 0xF0, 0x07, 0xDC, 0x40, 
		0x1F, 0x70, 0x01, 0xFD, 0xEE, 0x01, 0xF7, 0xA0,
		0x1F, 0xDE, 0xA0, 0x1F, 0x7B, 0x00, 0x7D, 0xE0,
		0x0D, 0xF7, 0xC0, 0x07, 0xDF, 0x28, 0x1F, 0x7D,
		0x18, 0x7D, 0xFE, 0x25, 0xC0			
	};

// ------------ General PSoC Vectors (for TMG300, TST300, TMA300)--------------
// Modifying these tables is NOT recommendended. Doing so will all but
// guarantee an ISSP error, unless updated vectors have been recommended or
// provided by Cypress Semiconductor.
// ----------------------------------------------------------------------------
	const unsigned int num_bits_erase_block = 396;
	const unsigned char erase_block[] =
    {
		0b11011110, 0b11100010, 0b00011111, 0b01111111, 0b00000010, 0b01111101, 0b11000100, 0b00001001, 
		0b11110111, 0b00000000, 0b00011111, 0b10011111, 0b00000111, 0b01011110, 0b01111100, 0b10000101, 
		0b11111101, 0b11111100, 0b00000001, 0b11110111, 0b00010000, 0b00000111, 0b11011100, 0b00000000, 
		0b01111111, 0b01111011, 0b10000000, 0b01111101, 0b11100000, 0b00000111, 0b11110111, 0b10100000, 
		0b00011111, 0b11011110, 0b10100000, 0b00011111, 0b01111011, 0b00000100, 0b01111101, 0b11110000, 
		0b00000001, 0b11110111, 0b11001001, 0b10000111, 0b11011111, 0b01001000, 0b00011111, 0b01111111, 
		0b10001001, 0b01110000
	};

	const unsigned int num_bits_tsync_enable = 110;
    const unsigned char tsync_enable[] =
    {
	    0xDE, 0xE2, 0x1F, 0x7F, 0x02, 0x7D, 0xC4, 0x09, 
        0xF7, 0x00, 0x1F, 0xDE, 0xE0, 0x1C 	
	
//		0b11011110, 0b11100010, 0b00011111, 0b01111111, 0b00000010, 0b01111101, 0b11000100, 0b00001001,
//		0b11110111, 0b00000000, 0b00011111, 0b11011110, 0b11100000, 0b00011100
	};
	const unsigned int num_bits_tsync_disable = 110;
	const unsigned char tsync_disable[] =
	{			
	    0xDE, 0xE2, 0x1F, 0x71, 0x00, 0x7D, 0xFC, 0x01, 
        0xF7, 0x00, 0x1F, 0xDE, 0xE0, 0x1C 

//			0b11011110, 0b11100010, 0b00011111, 0b01110001, 0b00000000, 0b01111101, 0b11111100, 0b00000001, 
//			0b11110111, 0b00000000, 0b00011111, 0b11011110, 0b11100000, 0b00011100
	};

	const unsigned int num_bits_set_block_num = 33;		
    const unsigned char set_block_num[] =
	{
	    0xDE, 0xE0, 0x1E, 0x7D, 0x00, 0x70 
		
//		    0b11011110, 0b11100000, 0b00011110, 0b01111101, 0b00000000, 0b01110000
    };
    const unsigned int num_bits_set_block_num_end = 3;		//PTJ: this selects the first three bits of set_block_num_end
    const unsigned char set_block_num_end = 0xE0;

	const unsigned int num_bits_read_write_setup = 66;		//PTJ:
    const unsigned char read_write_setup[] =
    {
        0xDE, 0xF0, 0x1F, 0x78, 0x00, 0x7D, 0xA0, 0x03, 
        0xC0 

//			0b11011110, 0b11110000, 0b00011111, 0b01111000, 0b00000000, 0b01111101, 0b10100000, 0b00000011,
//			0b11000000
	};

	const unsigned int num_bits_my_verify_setup = 440;
    const unsigned char verify_setup[] =
    {
		0xDE, 0xE2, 0x1F, 0x7F, 0x02, 0x7D, 0xC4, 0x09,
		0xF7, 0x00, 0x1F, 0x9F, 0x07, 0x5E, 0x7C, 0x81,
		0xF9, 0xF7, 0x01, 0xF7, 0xF0, 0x07, 0xDC, 0x40,
		0x1F, 0x70, 0x01, 0xFD, 0xEE, 0x01, 0xF6, 0xA8,
		0x0F, 0xDE, 0x80, 0x7F, 0x7A, 0x80, 0x7D, 0xEC,
		0x01, 0xF7, 0x80, 0x0F, 0xDF, 0x00, 0x1F, 0x7C,
		0xA0, 0x7D, 0xF4, 0x61, 0xF7, 0xF8, 0x97
		
//			0b11011110, 0b11100010, 0b00011111, 0b01111111, 0b00000010, 0b01111101, 0b11000100, 0b00001001, 
//			0b11110111, 0b00000000, 0b00011111, 0b10011111, 0b00000111, 0b01011110, 0b01111100, 0b10000001, 
//			0b11111001, 0b11110111, 0b00000001, 0b11110111, 0b11110000, 0b00000111, 0b11011100, 0b01000000, 
//			0b00011111, 0b01110000, 0b00000001, 0b11111101, 0b11101110, 0b00000001, 0b11110110, 0b10101000, 
//			0b00001111, 0b11011110, 0b10000000, 0b01111111, 0b01111010, 0b10000000, 0b01111101, 0b11101100, 
//			0b00000001, 0b11110111, 0b10000000, 0b00001111, 0b11011111, 0b00000000, 0b00011111, 0b01111100, 
//			0b10100000, 0b01111101, 0b11110100, 0b01100001, 0b11110111, 0b11111000, 0b10010111 			
    };

	const unsigned int num_bits_erase = 396;		
    const unsigned char erase[] =
	{
	    0xDE, 0xE2, 0x1F, 0x7F, 0x02, 0x7D, 0xC4, 0x09, 
        0xF7, 0x00, 0x1F, 0x9F, 0x07, 0x5E, 0x7C, 0x85, 
        0xFD, 0xFC, 0x01, 0xF7, 0x10, 0x07, 0xDC, 0x00, 
        0x7F, 0x7B, 0x80, 0x7D, 0xE0, 0x0B, 0xF7, 0xA0, 
        0x1F, 0xDE, 0xA0, 0x1F, 0x7B, 0x04, 0x7D, 0xF0, 
        0x01, 0xF7, 0xC9, 0x87, 0xDF, 0x48, 0x1F, 0x7F, 
        0x89, 0x70
	
//			0b11011110, 0b11100010, 0b00011111, 0b01111111, 0b00000010, 0b01111101, 0b11000100, 0b00001001,
//			0b11110111, 0b00000000, 0b00011111, 0b10011111, 0b00000111, 0b01011110, 0b01111100, 0b10000101, 
//			0b11111101, 0b11111100, 0b00000001, 0b11110111, 0b00010000, 0b00000111, 0b11011100, 0b00000000, 
//			0b01111111, 0b01111011, 0b10000000, 0b01111101, 0b11100000, 0b00001011, 0b11110111, 0b10100000, 
//			0b00011111, 0b11011110, 0b10100000, 0b00011111, 0b01111011, 0b00000100, 0b01111101, 0b11110000, 
//			0b00000001, 0b11110111, 0b11001001, 0b10000111, 0b11011111, 0b01001000, 0b00011111, 0b01111111, 
//			0b10001001, 0b01110000
    };

	const unsigned int num_bits_secure = 440;		//PTJ: secure with TSYNC Enable and Disable
    const unsigned char secure[] =
	{
			0b11011110, 0b11100010, 0b00011111, 0b01111111, 0b00000010, 0b01111101, 0b11000100, 0b00001001, 
			0b11110111, 0b00000000, 0b00011111, 0b10011111, 0b00000111, 0b01011110, 0b01111100, 0b10000001, 
			0b11111001, 0b11110111, 0b00000001, 0b11110111, 0b11110000, 0b00000111, 0b11011100, 0b01000000, 
			0b00011111, 0b01110000, 0b00000001, 0b11111101, 0b11101110, 0b00000001, 0b11110110, 0b10100000, 
			0b00001111, 0b11011110, 0b10000000, 0b01111111, 0b01111010, 0b10000000, 0b01111101, 0b11101100, 
			0b00000001, 0b11110111, 0b10000000, 0b00100111, 0b11011111, 0b00000000, 0b00011111, 0b01111100, 
			0b10100000, 0b01111101, 0b11110100, 0b01100001, 0b11110111, 0b11111000, 0b10010111
	};

    const unsigned int num_bits_checksum_setup = 418;		//PTJ: Checksum with TSYNC Enable and Disable
    const unsigned char checksum_setup[] =
    {
		
			0b11011110, 0b11100010, 0b00011111, 0b01111111, 0b00000010, 0b01111101, 0b11000100, 0b00001001,
			0b11110111, 0b00000000, 0b00011111, 0b10011111, 0b00000111, 0b01011110, 0b01111100, 0b10000001, 
			0b11111001, 0b11110100, 0b00000001, 0b11110111, 0b11110000, 0b00000111, 0b11011100, 0b01000000, 
			0b00011111, 0b01110000, 0b00000001, 0b11111101, 0b11101110, 0b00000001, 0b11110111, 0b10100000,
			0b00011111, 0b11011110, 0b10100000, 0b00011111, 0b01111011, 0b00000000, 0b01111101, 0b11100000, 
			0b00001111, 0b11110111, 0b11000000, 0b00000111, 0b11011111, 0b00101000, 0b00011111, 0b01111101, 
			0b00011000, 0b01111101, 0b11111110, 0b00100101, 0b11000000
			
    };

    const unsigned int num_bits_program_and_verify = 440;		//PTJ: length of program_block[], not including zero padding at end
    const unsigned char program_and_verify[] =
    {
		0b11011110, 0b11100010, 0b00011111, 0b01111111, 0b00000010, 0b01111101, 0b11000100, 0b00001001, 
		0b11110111, 0b00000000, 0b00011111, 0b10011111, 0b00000111, 0b01011110, 0b01111100, 0b10000001, 
		0b11111001, 0b11110111, 0b00000001, 0b11110111, 0b11110000, 0b00000111, 0b11011100, 0b01000000, 
		0b00011111, 0b01110000, 0b00000001, 0b11111101, 0b11101110, 0b00000001, 0b11110110, 0b10100000, 
		0b00001111, 0b11011110, 0b10000000, 0b01111111, 0b01111010, 0b10000000, 0b01111101, 0b11101100, 
		0b00000001, 0b11110111, 0b10000000, 0b01010111, 0b11011111, 0b00000000, 0b00011111, 0b01111100, 
		0b10100000, 0b01111101, 0b11110100, 0b01100001, 0b11110111, 0b11111000, 0b10010111
		
    };

    const unsigned int num_bits_verify_security = 440; //462;	//484
    const unsigned char verify_security[] =
    {
       
//		0b11011110, 0b11100010, 0b00011111, 0b01111111, 0b00000010, 0b01111101, 0b11000100, 0b00001001, 
//		0b11110111, 0b00000000, 0b00011111, 0b10011111, 0b00000111, 0b01011110, 0b01111100, 0b10000001, 
//		0b11111001, 0b11110100, 0b00000001, 0b11100111, 0b11011100, 0b00000111, 0b11011111, 0b11000000, 
//		0b00011111, 0b01110001, 0b00000000, 0b01111101, 0b11000000, 0b00000111, 0b11110111, 0b10111000, 
//		0b00000111, 0b11011010, 0b10000000, 0b00011111, 0b01111010, 0b00000001, 0b11111101, 0b11101010, 
//		0b00000001, 0b11110111, 0b10110000, 0b00000111, 0b11011110, 0b00000010, 0b01111111, 0b01111100, 
//		0b00000000, 0b01111101, 0b11110010, 0b10000001, 0b11110111, 0b11010001, 0b10000111, 0b11011111, 
//		0b11100010, 0b01011100
		
		0b11011110, 0b11100010, 0b00011111, 0b01111111, 0b00000010, 0b01111101, 0b11000100, 0b00001001, 
		0b11110111, 0b00000000, 0b00011111, 0b10011111, 0b00000111, 0b01011110, 0b01111100, 0b10000001, 
		0b11111001, 0b11110100, 0b00000001, 0b11100111, 0b11011100, 0b00000111, 0b11011111, 0b11000000, 
		0b00011111, 0b01110001, 0b00000000, 0b01111101, 0b11000000, 0b00000111, 0b11110111, 0b10111000, 
		0b00000111, 0b11011110, 0b10000000, 0b01111111, 0b01111010, 0b10000000, 0b01111101, 0b11101100, 
		0b00000001, 0b11110111, 0b10000000, 0b10011111, 0b11011111, 0b00000000, 0b00011111, 0b01111100, 
		0b10100000, 0b01111101, 0b11110100, 0b01100001, 0b11110111, 0b11111000, 0b10010111
		
    };
 


    const unsigned char read_id_v[] =
    {
		0xBF, 0x00, 0xDF, 0x90, 0x00, 0xFE, 0x60, 0xFF, 0x00
//		0b10111111,0b00000000,0b11011111,0b10010000,0b00000000,0b11111110,0b0110000,0b11111111,0b00000000
    };
            
    const unsigned char    write_byte_start = 0x90;			//this is set to SRAM 0x80
	const unsigned char    write_byte_end = 0xE0;

    const unsigned char    set_block_number[] = {0x9F, 0x40, 0xE0};
    const unsigned char    set_block_number_end = 0xE0;

    const unsigned char    num_bits_wait_and_poll_end = 40;
    const unsigned char    wait_and_poll_end[] = 
    {  
        0x00, 0x00, 0x00, 0x00, 0x00 
    };    // forty '0's per the spec             
                
    const unsigned char read_checksum_v[] = 
    {  
        0xBF, 0x20, 0xDF, 0x80, 0x80
                
//          0b10111111, 0b00100000,0b11011111,0b10000000,0b10000000 
    };
    
    const unsigned char read_byte_v[] = 
    {  
        0xB0, 0x80 
                
//          0b10110000, 0b10000000 
    };    

#endif  //(INC_ISSP_VECTORS)
#endif  //(PROJECT_REV_)
//end of file ISSP_Vectors.h