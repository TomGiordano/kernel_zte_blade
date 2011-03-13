//------------------------------------------------------------------------------
// <copyright file="hif_internal.h" company="Atheros">
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
// internal header file for hif layer
//
// Author(s): ="Atheros"
//==============================================================================
#include "a_config.h"
#include "ctsystem.h"
#include "sdio_busdriver.h"
#include "_sdio_defs.h"
#include "sdio_lib.h"
#include "athdefs.h"
#include "a_types.h"
#include "a_osapi.h"
#include "hif.h"

#define MANUFACTURER_ID_AR6001_BASE        0x100
#define MANUFACTURER_ID_AR6002_BASE        0x200
#define MANUFACTURER_ID_AR6003_BASE        0x300
#define FUNCTION_CLASS                     0x0
#define MANUFACTURER_CODE                  0x271

#define BUS_REQUEST_MAX_NUM                64

#define SDIO_CLOCK_FREQUENCY_DEFAULT       25000000
#define SDWLAN_ENABLE_DISABLE_TIMEOUT      20
#define FLAGS_CARD_ENAB                    0x02
#define FLAGS_CARD_IRQ_UNMSK               0x04

#define HIF_MBOX_BLOCK_SIZE                128
#define HIF_MBOX_BASE_ADDR                 0x800
#define HIF_MBOX_WIDTH                     0x800
#define HIF_MBOX0_BLOCK_SIZE               1
#define HIF_MBOX1_BLOCK_SIZE               HIF_MBOX_BLOCK_SIZE
#define HIF_MBOX2_BLOCK_SIZE               HIF_MBOX_BLOCK_SIZE
#define HIF_MBOX3_BLOCK_SIZE               HIF_MBOX_BLOCK_SIZE

#define HIF_MBOX_START_ADDR(mbox)                        \
    HIF_MBOX_BASE_ADDR + mbox * HIF_MBOX_WIDTH

#define HIF_MBOX_END_ADDR(mbox)	                         \
    HIF_MBOX_START_ADDR(mbox) + HIF_MBOX_WIDTH - 1

struct hif_device {
    SDDEVICE *handle;
    void     *claimedContext;
    HTC_CALLBACKS htcCallbacks;
    OSKERNEL_HELPER insert_helper;
    BOOL  helper_started;
};

typedef struct target_function_context {
    SDFUNCTION           function; /* function description of the bus driver */
    OS_SEMAPHORE         instanceSem; /* instance lock. Unused */
    SDLIST               instanceList; /* list of instances. Unused */
} TARGET_FUNCTION_CONTEXT;

typedef struct bus_request {
    struct bus_request *next;
    SDREQUEST *request;
    void *context;
    HIF_DEVICE *hifDevice;
} BUS_REQUEST;

BOOL
hifDeviceInserted(SDFUNCTION *function, SDDEVICE *device);

void
hifDeviceRemoved(SDFUNCTION *function, SDDEVICE *device);

SDREQUEST *
hifAllocateDeviceRequest(SDDEVICE *device);

void
hifFreeDeviceRequest(SDREQUEST *request);

void
hifRWCompletionHandler(SDREQUEST *request);

void
hifIRQHandler(void *context);

HIF_DEVICE *
addHifDevice(SDDEVICE *handle);

HIF_DEVICE *
getHifDevice(SDDEVICE *handle);

void
delHifDevice(SDDEVICE *handle);
