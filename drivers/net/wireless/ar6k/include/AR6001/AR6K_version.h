//------------------------------------------------------------------------------
// <copyright file="AR6K_version.h" company="Atheros">
//    Copyright (c) 2004-2007 Atheros Corporation.  All rights reserved.
// 
// The software source and binaries included in this development package are
// licensed, not sold. You, or your company, received the package under one
// or more license agreements. The rights granted to you are specifically
// listed in these license agreement(s). All other rights remain with Atheros
// Communications, Inc., its subsidiaries, or the respective owner including
// those listed on the included copyright notices.  Distribution of any
// portion of this package must be in strict compliance with the license
// agreement(s) terms.
// </copyright>
// 
// <summary>
// 	Wifi driver for AR6002
// </summary>
//
//------------------------------------------------------------------------------
//==============================================================================
// Author(s): ="Atheros"
//==============================================================================
#define __VER_MAJOR_ 2
#define __VER_MINOR_ 2 
#define __VER_PATCH_ 0

  
/* The makear6ksdk script (used for release builds) modifies the following line. */
#define __BUILD_NUMBER_ 53


/* Format of the version number. */
#define VER_MAJOR_BIT_OFFSET		28
#define VER_MINOR_BIT_OFFSET		24
#define VER_PATCH_BIT_OFFSET		16
#define VER_BUILD_NUM_BIT_OFFSET	0


/* 
 * The version has the following format:
 * Bits 28-31: Major version
 * Bits 24-27: Minor version
 * Bits 16-23: Patch version
 * Bits 0-15:  Build number (automatically generated during build process ) 
 * E.g. Build 1.1.3.7 would be represented as 0x11030007.
 *  
 * DO NOT split the following macro into multiple lines as this may confuse the build scripts. 
 */
#define AR6K_SW_VERSION 	( ( __VER_MAJOR_ << VER_MAJOR_BIT_OFFSET ) + ( __VER_MINOR_ << VER_MINOR_BIT_OFFSET ) + ( __VER_PATCH_ << VER_PATCH_BIT_OFFSET ) + ( __BUILD_NUMBER_ << VER_BUILD_NUM_BIT_OFFSET ) )


