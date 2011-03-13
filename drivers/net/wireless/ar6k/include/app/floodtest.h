//------------------------------------------------------------------------------
// <copyright file="floodtest.h" company="Atheros">
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

/* Floodtest Application Message Interface */

#define FLOOD_TX 0 /* From Host to Target */
#define FLOOD_RX 1 /* From Target to Host */

struct floodtest_control_s {
    A_UINT32 direction;        /* FLOOD_RX or FLOOD_TX */
    A_UINT32 message_size;     /* in bytes, for SEND/RX only */
    A_UINT32 duration;         /* in seconds, for SEND/RX only */
};

