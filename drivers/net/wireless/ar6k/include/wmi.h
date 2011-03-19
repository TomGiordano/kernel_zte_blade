/*------------------------------------------------------------------------------ */
/* <copyright file="wmi.h" company="Atheros"> */
/*    Copyright (c) 2004-2008 Atheros Corporation.  All rights reserved. */
/*  */
/* This program is free software; you can redistribute it and/or modify */
/* it under the terms of the GNU General Public License version 2 as */
/* published by the Free Software Foundation; */
/* */
/* Software distributed under the License is distributed on an "AS */
/* IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or */
/* implied. See the License for the specific language governing */
/* rights and limitations under the License. */
/* */
/* */
/*------------------------------------------------------------------------------ */
/*============================================================================== */
/* Author(s): ="Atheros" */
/*============================================================================== */

/*
 * This file contains the definitions of the WMI protocol specified in the
 * Wireless Module Interface (WMI).  It includes definitions of all the
 * commands and events. Commands are messages from the host to the WM.
 * Events and Replies are messages from the WM to the host.
 *
 * Ownership of correctness in regards to WMI commands
 * belongs to the host driver and the WM is not required to validate
 * parameters for value, proper range, or any other checking.
 *
 */

#ifndef _WMI_H_
#define _WMI_H_

#ifndef ATH_TARGET
#include "athstartpack.h"
#endif

#include "wmix.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HTC_PROTOCOL_VERSION    0x0002
#define HTC_PROTOCOL_REVISION   0x0000

#define WMI_PROTOCOL_VERSION    0x0002
#define WMI_PROTOCOL_REVISION   0x0000

#define ATH_MAC_LEN             6               /* length of mac in bytes */
#define WMI_CMD_MAX_LEN         100
#define WMI_CONTROL_MSG_MAX_LEN     256
#define WMI_OPT_CONTROL_MSG_MAX_LEN 1536
#define IS_ETHERTYPE(_typeOrLen)        ((_typeOrLen) >= 0x0600)
#define RFC1042OUI      {0x00, 0x00, 0x00}

#define IP_ETHERTYPE 0x0800

#define WMI_IMPLICIT_PSTREAM 0xFF
#define WMI_MAX_THINSTREAM 15

struct host_app_area_s {
    A_UINT32 wmi_protocol_ver;
};

/*
 * Data Path
 */
typedef PREPACK struct {
    A_UINT8     dstMac[ATH_MAC_LEN];
    A_UINT8     srcMac[ATH_MAC_LEN];
    A_UINT16    typeOrLen;
} POSTPACK ATH_MAC_HDR;

typedef PREPACK struct {
    A_UINT8     dsap;
    A_UINT8     ssap;
    A_UINT8     cntl;
    A_UINT8     orgCode[3];
    A_UINT16    etherType;
} POSTPACK ATH_LLC_SNAP_HDR;

typedef enum {
    DATA_MSGTYPE = 0x0,
    CNTL_MSGTYPE,
    SYNC_MSGTYPE,
    OPT_MSGTYPE,
} WMI_MSG_TYPE;


typedef PREPACK struct {
    A_INT8      rssi;
    A_UINT8     info;            /* WMI_MSG_TYPE in lower 2 bits - b1b0 */
                                 /* UP in next 3 bits - b4b3b2 */
#define WMI_DATA_HDR_MSG_TYPE_MASK  0x03
#define WMI_DATA_HDR_MSG_TYPE_SHIFT 0
#define WMI_DATA_HDR_UP_MASK        0x07
#define WMI_DATA_HDR_UP_SHIFT       2
#define WMI_DATA_HDR_IS_MSG_TYPE(h, t)  (((h)->info & (WMI_DATA_HDR_MSG_TYPE_MASK)) == (t))
/* In AP mode, the same bit (b5) is used to indicate Power save state in
 * the Rx dir and More data bit state in the tx direction.
 */
#define WMI_DATA_HDR_PS_MASK 0x1
#define WMI_DATA_HDR_PS_SHIFT 5

#define WMI_DATA_HDR_MORE_MASK 0x1
#define WMI_DATA_HDR_MORE_SHIFT 5

#define WMI_DATA_HDR_SET_MORE_BIT(h) ((h)->info |= (WMI_DATA_HDR_MORE_MASK << WMI_DATA_HDR_MORE_SHIFT))

} POSTPACK WMI_DATA_HDR;


#define WMI_DATA_HDR_SET_MSG_TYPE(h, t) (h)->info = (((h)->info & ~(WMI_DATA_HDR_MSG_TYPE_MASK << WMI_DATA_HDR_MSG_TYPE_SHIFT)) | (t << WMI_DATA_HDR_MSG_TYPE_SHIFT))
#define WMI_DATA_HDR_SET_UP(h, p) (h)->info = (((h)->info & ~(WMI_DATA_HDR_UP_MASK << WMI_DATA_HDR_UP_SHIFT)) | (p << WMI_DATA_HDR_UP_SHIFT))

/*
 * Control Path
 */
typedef PREPACK struct {
    A_UINT16    commandId;
} POSTPACK WMI_CMD_HDR;        /* used for commands and events */

/*
 * List of Commnands
 */
typedef enum {
    WMI_CONNECT_CMDID           = 0x0001,
    WMI_RECONNECT_CMDID,
    WMI_DISCONNECT_CMDID,
    WMI_SYNCHRONIZE_CMDID,
    WMI_CREATE_PSTREAM_CMDID,
    WMI_DELETE_PSTREAM_CMDID,
    WMI_START_SCAN_CMDID,
    WMI_SET_SCAN_PARAMS_CMDID,
    WMI_SET_BSS_FILTER_CMDID,
    WMI_SET_PROBED_SSID_CMDID,               /* 10 */
    WMI_SET_LISTEN_INT_CMDID,
    WMI_SET_BMISS_TIME_CMDID,
    WMI_SET_DISC_TIMEOUT_CMDID,
    WMI_GET_CHANNEL_LIST_CMDID,
    WMI_SET_BEACON_INT_CMDID,
    WMI_GET_STATISTICS_CMDID,
    WMI_SET_CHANNEL_PARAMS_CMDID,
    WMI_SET_POWER_MODE_CMDID,
    WMI_SET_IBSS_PM_CAPS_CMDID,
    WMI_SET_POWER_PARAMS_CMDID,              /* 20 */
    WMI_SET_POWERSAVE_TIMERS_POLICY_CMDID,
    WMI_ADD_CIPHER_KEY_CMDID,
    WMI_DELETE_CIPHER_KEY_CMDID,
    WMI_ADD_KRK_CMDID,
    WMI_DELETE_KRK_CMDID,
    WMI_SET_PMKID_CMDID,
    WMI_SET_TX_PWR_CMDID,
    WMI_GET_TX_PWR_CMDID,
    WMI_SET_ASSOC_INFO_CMDID,
    WMI_ADD_BAD_AP_CMDID,                    /* 30 */
    WMI_DELETE_BAD_AP_CMDID,
    WMI_SET_TKIP_COUNTERMEASURES_CMDID,
    WMI_RSSI_THRESHOLD_PARAMS_CMDID,
    WMI_TARGET_ERROR_REPORT_BITMASK_CMDID,
    WMI_SET_ACCESS_PARAMS_CMDID,
    WMI_SET_RETRY_LIMITS_CMDID,
    WMI_SET_OPT_MODE_CMDID,
    WMI_OPT_TX_FRAME_CMDID,
    WMI_SET_VOICE_PKT_SIZE_CMDID,
    WMI_SET_MAX_SP_LEN_CMDID,                /* 40 */
    WMI_SET_ROAM_CTRL_CMDID,
    WMI_GET_ROAM_TBL_CMDID,
    WMI_GET_ROAM_DATA_CMDID,
    WMI_ENABLE_RM_CMDID,
    WMI_SET_MAX_OFFHOME_DURATION_CMDID,
    WMI_EXTENSION_CMDID,                        /* Non-wireless extensions */
    WMI_SNR_THRESHOLD_PARAMS_CMDID,
    WMI_LQ_THRESHOLD_PARAMS_CMDID,
    WMI_SET_LPREAMBLE_CMDID,
    WMI_SET_RTS_CMDID,                       /* 50 */
    WMI_CLR_RSSI_SNR_CMDID,
    WMI_SET_FIXRATES_CMDID,
    WMI_GET_FIXRATES_CMDID,
    WMI_SET_AUTH_MODE_CMDID,
    WMI_SET_REASSOC_MODE_CMDID,
    WMI_SET_WMM_CMDID,
    WMI_SET_WMM_TXOP_CMDID,
    WMI_TEST_CMDID,
    WMI_SET_BT_STATUS_CMDID,
    WMI_SET_BT_PARAMS_CMDID,                 /* 60 */

    WMI_SET_KEEPALIVE_CMDID,
    WMI_GET_KEEPALIVE_CMDID,
    WMI_SET_APPIE_CMDID,
    WMI_GET_APPIE_CMDID,
    WMI_SET_WSC_STATUS_CMDID,

    /* Wake on Wireless */
    WMI_SET_HOST_SLEEP_MODE_CMDID,
    WMI_SET_WOW_MODE_CMDID,
    WMI_GET_WOW_LIST_CMDID,
    WMI_ADD_WOW_PATTERN_CMDID,
    WMI_DEL_WOW_PATTERN_CMDID,               /* 70 */

    WMI_SET_FRAMERATES_CMDID,
    /*
     * Developer commands starts at 0xF000
     */
    WMI_SET_BITRATE_CMDID = 0xF000,
    WMI_GET_BITRATE_CMDID,
    WMI_SET_WHALPARAM_CMDID,


    /*Should add the new command to the tail for compatible with
     * etna.
     */
    WMI_SET_MAC_ADDRESS_CMDID,
    WMI_SET_AKMP_PARAMS_CMDID,
    WMI_SET_PMKID_LIST_CMDID,
    WMI_GET_PMKID_LIST_CMDID,
    WMI_ABORT_SCAN_CMDID,
    WMI_SET_TARGET_EVENT_REPORT_CMDID,

    /* Pyxis specific Command IDs */
    WMI_PYXIS_CONFIG_CMDID,
    WMI_PYXIS_OPERATION_CMDID,    /* 0xF00A */

    /*
     * AP mode commands
     */
    WMI_AP_HIDDEN_SSID_CMDID,
    WMI_AP_SET_NUM_STA_CMDID,
    WMI_AP_ACL_POLICY_CMDID,
    WMI_AP_ACL_MAC_LIST_CMDID,
    WMI_AP_CONFIG_COMMIT_CMDID,
    WMI_AP_SET_MLME_CMDID,
    WMI_AP_SET_PVB_CMDID,
    WMI_AP_CONN_INACT_CMDID,
    WMI_AP_PROT_SCAN_TIME_CMDID,
    WMI_SET_COUNTRY_CMDID,     /* 0xF014 */
    WMI_AP_SET_DTIM_CMDID,

    WMI_SET_IP_CMDID,
    WMI_SET_PARAMS_CMDID,
    WMI_SET_MCAST_FILTER_CMDID,
    WMI_DEL_MCAST_FILTER_CMDID
} WMI_COMMAND_ID;

/*
 * Frame Types
 */
typedef enum {
    WMI_FRAME_BEACON        =   0,
    WMI_FRAME_PROBE_REQ,
    WMI_FRAME_PROBE_RESP,
    WMI_FRAME_ASSOC_REQ,
    WMI_FRAME_ASSOC_RESP,
    WMI_NUM_MGMT_FRAME
} WMI_MGMT_FRAME_TYPE;

/*
 * Connect Command
 */
typedef enum {
    INFRA_NETWORK       = 0x01,
    ADHOC_NETWORK       = 0x02,
    ADHOC_CREATOR       = 0x04,
    OPT_NETWORK         = 0x08,
    AP_NETWORK          = 0x10,
} NETWORK_TYPE;

typedef enum {
    OPEN_AUTH           = 0x01,
    SHARED_AUTH         = 0x02,
    LEAP_AUTH           = 0x04,  /* different from IEEE_AUTH_MODE definitions */
} DOT11_AUTH_MODE;

typedef enum {
    NONE_AUTH           = 0x01,
    WPA_AUTH            = 0x02,
    WPA_PSK_AUTH        = 0x03,
    WPA2_AUTH           = 0x04,
    WPA2_PSK_AUTH       = 0x05,
    WPA_AUTH_CCKM       = 0x06,
    WPA2_AUTH_CCKM      = 0x07,
} AUTH_MODE;

typedef enum {
    NONE_CRYPT          = 0x01,
    WEP_CRYPT           = 0x02,
    TKIP_CRYPT          = 0x03,
    AES_CRYPT           = 0x04,
#ifdef WAPI_ENABLE
    WAPI_CRYPT          = 0x05,
#endif /* WAPI_ENABLE */
} CRYPTO_TYPE;

#ifdef WAPI_ENABLE
#define IW_ENCODE_ALG_SM4              0x20
#define IW_AUTH_WAPI_ENABLED           0x20
#endif /* WAPI_ENABLE */

#define WMI_MIN_CRYPTO_TYPE NONE_CRYPT
#define WMI_MAX_CRYPTO_TYPE (AES_CRYPT + 1)

#ifdef WAPI_ENABLE
#undef WMI_MAX_CRYPTO_TYPE
#define WMI_MAX_CRYPTO_TYPE (WAPI_CRYPT + 1)
#endif /* WAPI_ENABLE */


#define WMI_MIN_KEY_INDEX   0
#define WMI_MAX_KEY_INDEX   3

#ifdef WAPI_ENABLE
#undef WMI_MAX_KEY_INDEX
#define WMI_MAX_KEY_INDEX   7 /* wapi grpKey 0-3, prwKey 4-7 */
#endif /* WAPI_ENABLE */

#define WMI_MAX_KEY_LEN     32

#define WMI_MAX_SSID_LEN    32

typedef enum {
    CONNECT_ASSOC_POLICY_USER = 0x0001,
    CONNECT_SEND_REASSOC = 0x0002,
    CONNECT_IGNORE_WPAx_GROUP_CIPHER = 0x0004,
    CONNECT_PROFILE_MATCH_DONE = 0x0008,
    CONNECT_IGNORE_AAC_BEACON = 0x0010,
    CONNECT_CSA_FOLLOW_BSS = 0x0020,
    CONNECT_PYXIS_REMOTE = 0x0040,
} WMI_CONNECT_CTRL_FLAGS_BITS;

#define DEFAULT_CONNECT_CTRL_FLAGS    (CONNECT_CSA_FOLLOW_BSS)

typedef PREPACK struct {
    A_UINT8     networkType;
    A_UINT8     dot11AuthMode;
    A_UINT8     authMode;
    A_UINT8     pairwiseCryptoType;
    A_UINT8     pairwiseCryptoLen;
    A_UINT8     groupCryptoType;
    A_UINT8     groupCryptoLen;
    A_UINT8     ssidLength;
    A_UCHAR     ssid[WMI_MAX_SSID_LEN];
    A_UINT16    channel;
    A_UINT8     bssid[ATH_MAC_LEN];
    A_UINT32    ctrl_flags;
} POSTPACK WMI_CONNECT_CMD;

/*
 * WMI_RECONNECT_CMDID
 */
typedef PREPACK struct {
    A_UINT16    channel;                    /* hint */
    A_UINT8     bssid[ATH_MAC_LEN];         /* mandatory if set */
} POSTPACK WMI_RECONNECT_CMD;

/*
 * WMI_ADD_CIPHER_KEY_CMDID
 */
typedef enum {
    PAIRWISE_USAGE      = 0x00,
    GROUP_USAGE         = 0x01,
    TX_USAGE            = 0x02,     /* default Tx Key - Static WEP only */
} KEY_USAGE;

/*
 * Bit Flag
 * Bit 0 - Initialise TSC - default is Initialize
 */
#define KEY_OP_INIT_TSC       0x01
#define KEY_OP_INIT_RSC       0x02
#ifdef WAPI_ENABLE
#define KEY_OP_INIT_WAPIPN    0x10
#endif /* WAPI_ENABLE */

#define KEY_OP_INIT_VAL     0x03     /* Default Initialise the TSC & RSC */
#define KEY_OP_VALID_MASK   0x03

typedef PREPACK struct {
    A_UINT8    keyIndex;
    A_UINT8    keyType;
    A_UINT8    keyUsage;           /* KEY_USAGE */
    A_UINT8    keyLength;
    A_UINT8    keyRSC[8];          /* key replay sequence counter */
    A_UINT8    key[WMI_MAX_KEY_LEN];
    A_UINT8    key_op_ctrl;       /* Additional Key Control information */
    A_UINT8    key_macaddr[ATH_MAC_LEN];
} POSTPACK WMI_ADD_CIPHER_KEY_CMD;

/*
 * WMI_DELETE_CIPHER_KEY_CMDID
 */
typedef PREPACK struct {
    A_UINT8     keyIndex;
} POSTPACK WMI_DELETE_CIPHER_KEY_CMD;

#define WMI_KRK_LEN     16
/*
 * WMI_ADD_KRK_CMDID
 */
typedef PREPACK struct {
    A_UINT8     krk[WMI_KRK_LEN];
} POSTPACK WMI_ADD_KRK_CMD;

/*
 * WMI_SET_TKIP_COUNTERMEASURES_CMDID
 */
typedef enum {
    WMI_TKIP_CM_DISABLE = 0x0,
    WMI_TKIP_CM_ENABLE  = 0x1,
} WMI_TKIP_CM_CONTROL;

typedef PREPACK struct {
    A_UINT8  cm_en;                     /* WMI_TKIP_CM_CONTROL */
} POSTPACK WMI_SET_TKIP_COUNTERMEASURES_CMD;

/*
 * WMI_SET_PMKID_CMDID
 */

#define WMI_PMKID_LEN 16

typedef enum {
   PMKID_DISABLE = 0,
   PMKID_ENABLE  = 1,
} PMKID_ENABLE_FLG;

typedef PREPACK struct {
    A_UINT8     bssid[ATH_MAC_LEN];
    A_UINT8     enable;                 /* PMKID_ENABLE_FLG */
    A_UINT8     pmkid[WMI_PMKID_LEN];
} POSTPACK WMI_SET_PMKID_CMD;

/*
 * WMI_START_SCAN_CMD
 */
typedef enum {
    WMI_LONG_SCAN  = 0,
    WMI_SHORT_SCAN = 1,
    WMI_PYXIS_PAS_DSCVR = 0,
    WMI_PYXIS_ACT_DSCVR = 1,
} WMI_SCAN_TYPE;

typedef PREPACK struct {
    A_BOOL   forceFgScan;
    A_BOOL   isLegacy;        /* For Legacy Cisco AP compatibility */
    A_UINT32 homeDwellTime;   /* Maximum duration in the home channel(milliseconds) */
    A_UINT32 forceScanInterval;    /* Time interval between scans (milliseconds)*/
    A_UINT8  scanType;           /* WMI_SCAN_TYPE */
    A_UINT8  numChannels;            /* how many channels follow */
    A_UINT16 channelList[1];         /* channels in Mhz */
} POSTPACK WMI_START_SCAN_CMD;

/*
 * WMI_SET_SCAN_PARAMS_CMDID
 */
#define WMI_SHORTSCANRATIO_DEFAULT      3
typedef enum {
    CONNECT_SCAN_CTRL_FLAGS = 0x01,    /* set if can scan in the Connect cmd */
    SCAN_CONNECTED_CTRL_FLAGS = 0x02,  /* set if scan for the SSID it is */
                                       /* already connected to */
    ACTIVE_SCAN_CTRL_FLAGS = 0x04,     /* set if enable active scan */
    ROAM_SCAN_CTRL_FLAGS = 0x08,       /* set if enable roam scan when bmiss and lowrssi */
    REPORT_BSSINFO_CTRL_FLAGS = 0x10,   /* set if follows customer BSSINFO reporting rule */
    ENABLE_AUTO_CTRL_FLAGS = 0x20,      /* if disabled, target doesn't
                                          scan after a disconnect event  */
    ENABLE_SCAN_ABORT_EVENT = 0x40      /* Scan complete event with canceled status will be generated when a scan is prempted before it gets completed */

} WMI_SCAN_CTRL_FLAGS_BITS;

#define CAN_SCAN_IN_CONNECT(flags)      (flags & CONNECT_SCAN_CTRL_FLAGS)
#define CAN_SCAN_CONNECTED(flags)       (flags & SCAN_CONNECTED_CTRL_FLAGS)
#define ENABLE_ACTIVE_SCAN(flags)       (flags & ACTIVE_SCAN_CTRL_FLAGS)
#define ENABLE_ROAM_SCAN(flags)         (flags & ROAM_SCAN_CTRL_FLAGS)
#define CONFIG_REPORT_BSSINFO(flags)     (flags & REPORT_BSSINFO_CTRL_FLAGS)
#define IS_AUTO_SCAN_ENABLED(flags)      (flags & ENABLE_AUTO_CTRL_FLAGS)
#define SCAN_ABORT_EVENT_ENABLED(flags) (flags & ENABLE_SCAN_ABORT_EVENT)

#define DEFAULT_SCAN_CTRL_FLAGS         (CONNECT_SCAN_CTRL_FLAGS| SCAN_CONNECTED_CTRL_FLAGS| ACTIVE_SCAN_CTRL_FLAGS| ROAM_SCAN_CTRL_FLAGS | ENABLE_AUTO_CTRL_FLAGS)


typedef PREPACK struct {
    A_UINT16    fg_start_period;        /* seconds */
    A_UINT16    fg_end_period;          /* seconds */
    A_UINT16    bg_period;              /* seconds */
    A_UINT16    maxact_chdwell_time;    /* msec */
    A_UINT16    pas_chdwell_time;       /* msec */
    A_UINT8     shortScanRatio;         /* how many shorts scan for one long */
    A_UINT8     scanCtrlFlags;
    A_UINT16    minact_chdwell_time;    /* msec */
    A_UINT16    maxact_scan_per_ssid;   /* max active scans per ssid */
    A_UINT32    max_dfsch_act_time;  /* msecs */
} POSTPACK WMI_SCAN_PARAMS_CMD;

/*
 * WMI_SET_BSS_FILTER_CMDID
 */
typedef enum {
    NONE_BSS_FILTER = 0x0,              /* no beacons forwarded */
    ALL_BSS_FILTER,                     /* all beacons forwarded */
    PROFILE_FILTER,                     /* only beacons matching profile */
    ALL_BUT_PROFILE_FILTER,             /* all but beacons matching profile */
    CURRENT_BSS_FILTER,                 /* only beacons matching current BSS */
    ALL_BUT_BSS_FILTER,                 /* all but beacons matching BSS */
    PROBED_SSID_FILTER,                 /* beacons matching probed ssid */
    LAST_BSS_FILTER,                    /* marker only */
} WMI_BSS_FILTER;

typedef PREPACK struct {
    A_UINT8    bssFilter;                      /* see WMI_BSS_FILTER */
    A_UINT8    reserved1;                      /* For alignment */
    A_UINT16   reserved2;                      /* For alignment */
    A_UINT32   ieMask;
} POSTPACK WMI_BSS_FILTER_CMD;

/*
 * WMI_SET_PROBED_SSID_CMDID
 */
#define MAX_PROBED_SSID_INDEX   5

typedef enum {
    DISABLE_SSID_FLAG  = 0,                  /* disables entry */
    SPECIFIC_SSID_FLAG = 0x01,               /* probes specified ssid */
    ANY_SSID_FLAG      = 0x02,               /* probes for any ssid */
} WMI_SSID_FLAG;

typedef PREPACK struct {
    A_UINT8     entryIndex;                     /* 0 to MAX_PROBED_SSID_INDEX */
    A_UINT8     flag;                           /* WMI_SSID_FLG */
    A_UINT8     ssidLength;
    A_UINT8     ssid[32];
} POSTPACK WMI_PROBED_SSID_CMD;

/*
 * WMI_SET_LISTEN_INT_CMDID
 * The Listen interval is between 15 and 3000 TUs
 */
#define MIN_LISTEN_INTERVAL 15
#define MAX_LISTEN_INTERVAL 5000
#define MIN_LISTEN_BEACONS 1
#define MAX_LISTEN_BEACONS 50

typedef PREPACK struct {
    A_UINT16     listenInterval;
    A_UINT16     numBeacons;
} POSTPACK WMI_LISTEN_INT_CMD;

/*
 * WMI_SET_BEACON_INT_CMDID
 */
typedef PREPACK struct {
    A_UINT16     beaconInterval;
} POSTPACK WMI_BEACON_INT_CMD;

/*
 * WMI_SET_BMISS_TIME_CMDID
 * valid values are between 1000 and 5000 TUs
 */

#define MIN_BMISS_TIME     1000
#define MAX_BMISS_TIME     5000
#define MIN_BMISS_BEACONS  1
#define MAX_BMISS_BEACONS  50

typedef PREPACK struct {
    A_UINT16     bmissTime;
    A_UINT16     numBeacons;
} POSTPACK WMI_BMISS_TIME_CMD;

/*
 * WMI_SET_POWER_MODE_CMDID
 */
typedef enum {
    REC_POWER = 0x01,
    MAX_PERF_POWER,
} WMI_POWER_MODE;

typedef PREPACK struct {
    A_UINT8     powerMode;      /* WMI_POWER_MODE */
} POSTPACK WMI_POWER_MODE_CMD;

typedef PREPACK struct {
    A_INT8 status;      /* WMI_SET_PARAMS_REPLY */
} POSTPACK WMI_SET_PARAMS_REPLY;

typedef PREPACK struct {
    A_UINT32 opcode;
    A_UINT32 length;
    A_CHAR buffer[1];      /* WMI_SET_PARAMS */
} POSTPACK WMI_SET_PARAMS_CMD;

typedef PREPACK struct {
    A_UINT8 multicast_mac[ATH_MAC_LEN];      /* WMI_SET_MCAST_FILTER */
} POSTPACK WMI_SET_MCAST_FILTER_CMD;

/*
 * WMI_SET_POWER_PARAMS_CMDID
 */
typedef enum {
    IGNORE_DTIM = 0x01,
    NORMAL_DTIM = 0x02,
    STICK_DTIM  = 0x03,
} WMI_DTIM_POLICY;

typedef PREPACK struct {
    A_UINT16    idle_period;             /* msec */
    A_UINT16    pspoll_number;
    A_UINT16    dtim_policy;
} POSTPACK WMI_POWER_PARAMS_CMD;

typedef PREPACK struct {
    A_UINT8    power_saving;
    A_UINT8    ttl; /* number of beacon periods */
    A_UINT16   atim_windows;          /* msec */
    A_UINT16   timeout_value;         /* msec */
} POSTPACK WMI_IBSS_PM_CAPS_CMD;

/*
 * WMI_SET_POWERSAVE_TIMERS_POLICY_CMDID
 */
typedef enum {
    IGNORE_TIM_ALL_QUEUES_APSD = 0,
    PROCESS_TIM_ALL_QUEUES_APSD = 1,
    IGNORE_TIM_SIMULATED_APSD = 2,
    PROCESS_TIM_SIMULATED_APSD = 3,
} APSD_TIM_POLICY;

typedef PREPACK struct {
    A_UINT16    psPollTimeout;          /* msec */
    A_UINT16    triggerTimeout;         /* msec */
    A_UINT32    apsdTimPolicy;      /* TIM behavior with  ques APSD enabled. Default is IGNORE_TIM_ALL_QUEUES_APSD */
    A_UINT32    simulatedAPSDTimPolicy;      /* TIM behavior with  simulated APSD enabled. Default is PROCESS_TIM_SIMULATED_APSD */
} POSTPACK WMI_POWERSAVE_TIMERS_POLICY_CMD;

/*
 * WMI_SET_VOICE_PKT_SIZE_CMDID
 */
typedef PREPACK struct {
    A_UINT16    voicePktSize;
} POSTPACK WMI_SET_VOICE_PKT_SIZE_CMD;

/*
 * WMI_SET_MAX_SP_LEN_CMDID
 */
typedef enum {
    DELIVER_ALL_PKT = 0x0,
    DELIVER_2_PKT = 0x1,
    DELIVER_4_PKT = 0x2,
    DELIVER_6_PKT = 0x3,
} APSD_SP_LEN_TYPE;

typedef PREPACK struct {
    A_UINT8    maxSPLen;
} POSTPACK WMI_SET_MAX_SP_LEN_CMD;

/*
 * WMI_SET_DISC_TIMEOUT_CMDID
 */
typedef PREPACK struct {
    A_UINT8     disconnectTimeout;          /* seconds */
} POSTPACK WMI_DISC_TIMEOUT_CMD;

typedef enum {
    UPLINK_TRAFFIC = 0,
    DNLINK_TRAFFIC = 1,
    BIDIR_TRAFFIC = 2,
} DIR_TYPE;

typedef enum {
    DISABLE_FOR_THIS_AC = 0,
    ENABLE_FOR_THIS_AC  = 1,
    ENABLE_FOR_ALL_AC   = 2,
} VOICEPS_CAP_TYPE;

typedef enum {
    TRAFFIC_TYPE_APERIODIC = 0,
    TRAFFIC_TYPE_PERIODIC = 1,
}TRAFFIC_TYPE;

/*
 * WMI_SYNCHRONIZE_CMDID
 */
typedef PREPACK struct {
    A_UINT8 dataSyncMap;
} POSTPACK WMI_SYNC_CMD;

/*
 * WMI_CREATE_PSTREAM_CMDID
 */
typedef PREPACK struct {
    A_UINT32        minServiceInt;           /* in milli-sec */
    A_UINT32        maxServiceInt;           /* in milli-sec */
    A_UINT32        inactivityInt;           /* in milli-sec */
    A_UINT32        suspensionInt;           /* in milli-sec */
    A_UINT32        serviceStartTime;
    A_UINT32        minDataRate;             /* in bps */
    A_UINT32        meanDataRate;            /* in bps */
    A_UINT32        peakDataRate;            /* in bps */
    A_UINT32        maxBurstSize;
    A_UINT32        delayBound;
    A_UINT32        minPhyRate;              /* in bps */
    A_UINT32        sba;
    A_UINT32        mediumTime;
    A_UINT16        nominalMSDU;             /* in octects */
    A_UINT16        maxMSDU;                 /* in octects */
    A_UINT8         trafficClass;
    A_UINT8         trafficDirection;        /* DIR_TYPE */
    A_UINT8         rxQueueNum;
    A_UINT8         trafficType;             /* TRAFFIC_TYPE */
    A_UINT8         voicePSCapability;       /* VOICEPS_CAP_TYPE */
    A_UINT8         tsid;
    A_UINT8         userPriority;            /* 802.1D user priority */
    A_UINT8         nominalPHY;              /* nominal phy rate */
} POSTPACK WMI_CREATE_PSTREAM_CMD;

/*
 * WMI_DELETE_PSTREAM_CMDID
 */
typedef PREPACK struct {
    A_UINT8     txQueueNumber;
    A_UINT8     rxQueueNumber;
    A_UINT8     trafficDirection;
    A_UINT8     trafficClass;
    A_UINT8     tsid;
} POSTPACK WMI_DELETE_PSTREAM_CMD;

/*
 * WMI_SET_CHANNEL_PARAMS_CMDID
 */
typedef enum {
    WMI_11A_MODE  = 0x1,
    WMI_11G_MODE  = 0x2,
    WMI_11AG_MODE = 0x3,
    WMI_11B_MODE  = 0x4,
    WMI_11GONLY_MODE = 0x5,
} WMI_PHY_MODE;

#define WMI_MAX_CHANNELS        32

typedef PREPACK struct {
    A_UINT8     reserved1;
    A_UINT8     scanParam;              /* set if enable scan */
    A_UINT8     phyMode;                /* see WMI_PHY_MODE */
    A_UINT8     numChannels;            /* how many channels follow */
    A_UINT16    channelList[1];         /* channels in Mhz */
} POSTPACK WMI_CHANNEL_PARAMS_CMD;


/*
 *  WMI_RSSI_THRESHOLD_PARAMS_CMDID
 *  Setting the polltime to 0 would disable polling.
 *  Threshold values are in the ascending order, and should agree to:
 *  (lowThreshold_lowerVal < lowThreshold_upperVal < highThreshold_lowerVal
 *      < highThreshold_upperVal)
 */

typedef PREPACK struct WMI_RSSI_THRESHOLD_PARAMS{
    A_UINT32    pollTime;               /* Polling time as a factor of LI */
    A_INT16     thresholdAbove1_Val;          /* lowest of upper */
    A_INT16     thresholdAbove2_Val;
    A_INT16     thresholdAbove3_Val;
    A_INT16     thresholdAbove4_Val;
    A_INT16     thresholdAbove5_Val;
    A_INT16     thresholdAbove6_Val;          /* highest of upper */
    A_INT16     thresholdBelow1_Val;         /* lowest of bellow */
    A_INT16     thresholdBelow2_Val;
    A_INT16     thresholdBelow3_Val;
    A_INT16     thresholdBelow4_Val;
    A_INT16     thresholdBelow5_Val;
    A_INT16     thresholdBelow6_Val;         /* highest of bellow */
    A_UINT8     weight;                  /* "alpha" */
    A_UINT8     reserved[3];
} POSTPACK  WMI_RSSI_THRESHOLD_PARAMS_CMD;

/*
 *  WMI_SNR_THRESHOLD_PARAMS_CMDID
 *  Setting the polltime to 0 would disable polling.
 */

typedef PREPACK struct WMI_SNR_THRESHOLD_PARAMS{
    A_UINT32    pollTime;               /* Polling time as a factor of LI */
    A_UINT8     weight;                  /* "alpha" */
    A_UINT8     thresholdAbove1_Val;      /* lowest of uppper*/
    A_UINT8     thresholdAbove2_Val;
    A_UINT8     thresholdAbove3_Val;
    A_UINT8     thresholdAbove4_Val;      /* highest of upper */
    A_UINT8     thresholdBelow1_Val;     /* lowest of bellow */
    A_UINT8     thresholdBelow2_Val;
    A_UINT8     thresholdBelow3_Val;
    A_UINT8     thresholdBelow4_Val;     /* highest of bellow */
    A_UINT8     reserved[3];
} POSTPACK WMI_SNR_THRESHOLD_PARAMS_CMD;

/*
 *  WMI_LQ_THRESHOLD_PARAMS_CMDID
 */
typedef PREPACK struct WMI_LQ_THRESHOLD_PARAMS {
    A_UINT8     enable;
    A_UINT8     thresholdAbove1_Val;
    A_UINT8     thresholdAbove2_Val;
    A_UINT8     thresholdAbove3_Val;
    A_UINT8     thresholdAbove4_Val;
    A_UINT8     thresholdBelow1_Val;
    A_UINT8     thresholdBelow2_Val;
    A_UINT8     thresholdBelow3_Val;
    A_UINT8     thresholdBelow4_Val;
    A_UINT8     reserved[3];
} POSTPACK  WMI_LQ_THRESHOLD_PARAMS_CMD;

typedef enum {
    WMI_LPREAMBLE_DISABLED = 0,
    WMI_LPREAMBLE_ENABLED
} WMI_LPREAMBLE_STATUS;

typedef PREPACK struct {
    A_UINT8     status;
}POSTPACK WMI_SET_LPREAMBLE_CMD;

typedef PREPACK struct {
    A_UINT16    threshold;
}POSTPACK WMI_SET_RTS_CMD;

/*
 *  WMI_TARGET_ERROR_REPORT_BITMASK_CMDID
 *  Sets the error reporting event bitmask in target. Target clears it
 *  upon an error. Subsequent errors are counted, but not reported
 *  via event, unless the bitmask is set again.
 */
typedef PREPACK struct {
    A_UINT32    bitmask;
} POSTPACK  WMI_TARGET_ERROR_REPORT_BITMASK;

/*
 * WMI_SET_TX_PWR_CMDID
 */
typedef PREPACK struct {
    A_UINT8     dbM;                  /* in dbM units */
} POSTPACK WMI_SET_TX_PWR_CMD, WMI_TX_PWR_REPLY;

/*
 * WMI_SET_ASSOC_INFO_CMDID
 *
 * A maximum of 2 private IEs can be sent in the [Re]Assoc request.
 * A 3rd one, the CCX version IE can also be set from the host.
 */
#define WMI_MAX_ASSOC_INFO_TYPE    2
#define WMI_CCX_VER_IE             2 /* ieType to set CCX Version IE */

#define WMI_MAX_ASSOC_INFO_LEN     240

typedef PREPACK struct {
    A_UINT8     ieType;
    A_UINT8     bufferSize;
    A_UINT8     assocInfo[1];       /* up to WMI_MAX_ASSOC_INFO_LEN */
} POSTPACK WMI_SET_ASSOC_INFO_CMD;


/*
 * WMI_GET_TX_PWR_CMDID does not take any parameters
 */

/*
 * WMI_ADD_BAD_AP_CMDID
 */
#define WMI_MAX_BAD_AP_INDEX      1

typedef PREPACK struct {
    A_UINT8     badApIndex;         /* 0 to WMI_MAX_BAD_AP_INDEX */
    A_UINT8     bssid[ATH_MAC_LEN];
} POSTPACK WMI_ADD_BAD_AP_CMD;

/*
 * WMI_DELETE_BAD_AP_CMDID
 */
typedef PREPACK struct {
    A_UINT8     badApIndex;         /* 0 to WMI_MAX_BAD_AP_INDEX */
} POSTPACK WMI_DELETE_BAD_AP_CMD;

/*
 * WMI_SET_ACCESS_PARAMS_CMDID
 */
#define WMI_DEFAULT_TXOP_ACPARAM    0       /* implies one MSDU */
#define WMI_DEFAULT_ECWMIN_ACPARAM  4       /* corresponds to CWmin of 15 */
#define WMI_DEFAULT_ECWMAX_ACPARAM  10      /* corresponds to CWmax of 1023 */
#define WMI_MAX_CW_ACPARAM          15      /* maximum eCWmin or eCWmax */
#define WMI_DEFAULT_AIFSN_ACPARAM   2
#define WMI_MAX_AIFSN_ACPARAM       15
typedef PREPACK struct {
    A_UINT16 txop;                      /* in units of 32 usec */
    A_UINT8  eCWmin;
    A_UINT8  eCWmax;
    A_UINT8  aifsn;
} POSTPACK WMI_SET_ACCESS_PARAMS_CMD;


/*
 * WMI_SET_RETRY_LIMITS_CMDID
 *
 * This command is used to customize the number of retries the
 * wlan device will perform on a given frame.
 */
#define WMI_MIN_RETRIES 2
#define WMI_MAX_RETRIES 13
typedef enum {
    MGMT_FRAMETYPE    = 0,
    CONTROL_FRAMETYPE = 1,
    DATA_FRAMETYPE    = 2
} WMI_FRAMETYPE;

typedef PREPACK struct {
    A_UINT8 frameType;                      /* WMI_FRAMETYPE */
    A_UINT8 trafficClass;                   /* applies only to DATA_FRAMETYPE */
    A_UINT8 maxRetries;
    A_UINT8 enableNotify;
} POSTPACK WMI_SET_RETRY_LIMITS_CMD;

/*
 * WMI_SET_ROAM_CTRL_CMDID
 *
 * This command is used to influence the Roaming behaviour
 * Set the host biases of the BSSs before setting the roam mode as bias
 * based.
 */

/*
 * Different types of Roam Control
 */

typedef enum {
        WMI_FORCE_ROAM          = 1,      /* Roam to the specified BSSID */
        WMI_SET_ROAM_MODE       = 2,      /* default ,progd bias, no roam */
        WMI_SET_HOST_BIAS       = 3,     /* Set the Host Bias */
        WMI_SET_LOWRSSI_SCAN_PARAMS = 4, /* Set lowrssi Scan parameters */
} WMI_ROAM_CTRL_TYPE;

#define WMI_MIN_ROAM_CTRL_TYPE WMI_FORCE_ROAM
#define WMI_MAX_ROAM_CTRL_TYPE WMI_SET_LOWRSSI_SCAN_PARAMS

/*
 * ROAM MODES
 */

typedef enum {
        WMI_DEFAULT_ROAM_MODE   = 1,  /* RSSI based ROAM */
        WMI_HOST_BIAS_ROAM_MODE = 2, /* HOST BIAS based ROAM */
        WMI_LOCK_BSS_MODE  = 3  /* Lock to the Current BSS - no Roam */
} WMI_ROAM_MODE;

/*
 * BSS HOST BIAS INFO
 */

typedef PREPACK struct {
        A_UINT8 bssid[ATH_MAC_LEN];
        A_INT8  bias;
} POSTPACK WMI_BSS_BIAS;

typedef PREPACK struct {
        A_UINT8 numBss;
        WMI_BSS_BIAS bssBias[1];
} POSTPACK WMI_BSS_BIAS_INFO;

typedef PREPACK struct WMI_LOWRSSI_SCAN_PARAMS {
        A_UINT16 lowrssi_scan_period;
        A_INT16  lowrssi_scan_threshold;
        A_INT16  lowrssi_roam_threshold;
        A_UINT8  roam_rssi_floor;
        A_UINT8  reserved[1];              /* For alignment */
} POSTPACK WMI_LOWRSSI_SCAN_PARAMS;

typedef PREPACK struct {
    PREPACK union {
        A_UINT8 bssid[ATH_MAC_LEN]; /* WMI_FORCE_ROAM */
        A_UINT8 roamMode;           /* WMI_SET_ROAM_MODE  */
        WMI_BSS_BIAS_INFO bssBiasInfo; /* WMI_SET_HOST_BIAS */
        WMI_LOWRSSI_SCAN_PARAMS lrScanParams;
    } POSTPACK info;
    A_UINT8   roamCtrlType ;
} POSTPACK WMI_SET_ROAM_CTRL_CMD;

/*
 * WMI_ENABLE_RM_CMDID
 */
typedef PREPACK struct {
        A_BOOL enable_radio_measurements;
} POSTPACK WMI_ENABLE_RM_CMD;

/*
 * WMI_SET_MAX_OFFHOME_DURATION_CMDID
 */
typedef PREPACK struct {
        A_UINT8 max_offhome_duration;
} POSTPACK WMI_SET_MAX_OFFHOME_DURATION_CMD;

typedef PREPACK struct {
    A_UINT32 frequency;
    A_UINT8  threshold;
} POSTPACK WMI_SET_HB_CHALLENGE_RESP_PARAMS_CMD;

typedef enum {
    BT_STREAM_UNDEF = 0,
    BT_STREAM_SCO,             /* SCO stream */
    BT_STREAM_A2DP,            /* A2DP stream */
    BT_STREAM_SCAN,            /* BT Discovery or Page */
    BT_STREAM_ESCO,
    BT_STREAM_ALL,
    BT_STREAM_MAX
} BT_STREAM_TYPE;

typedef enum {
    BT_PARAM_SCO = 1,         /* SCO stream parameters */
    BT_PARAM_A2DP ,
    BT_PARAM_ANTENNA_CONFIG,
    BT_PARAM_COLOCATED_BT_DEVICE,
    BT_PARAM_ACLCOEX,
    BT_PARAM_11A_SEPARATE_ANT,
    BT_PARAM_MAX
} BT_PARAM_TYPE;

typedef enum {
    BT_PARAM_SCO_PSPOLL_LATENCY_ONE_FOURTH =1,
    BT_PARAM_SCO_PSPOLL_LATENCY_HALF,
    BT_PARAM_SCO_PSPOLL_LATENCY_THREE_FOURTH,
} BT_PARAMS_SCO_PSPOLL_LATENCY;

typedef enum {
    BT_PARAMS_SCO_STOMP_SCO_NEVER =1,
    BT_PARAMS_SCO_STOMP_SCO_ALWAYS,
    BT_PARAMS_SCO_STOMP_SCO_IN_LOWRSSI,
} BT_PARAMS_SCO_STOMP_RULES;

typedef enum {
    BT_STATUS_UNDEF = 0,
    BT_STATUS_START,
    BT_STATUS_STOP,
    BT_STATUS_RESUME,
    BT_STATUS_SUSPEND,
    BT_STATUS_SUSPEND_A2DP,
    BT_STATUS_SUSPEND_SCO,
    BT_STATUS_SUSPEND_ACL,
    BT_STATUS_SUSPEND_SCAN,
    BT_STATUS_MAX
} BT_STREAM_STATUS;

typedef PREPACK struct {
    A_UINT8 streamType;
    A_UINT8 status;
} POSTPACK WMI_SET_BT_STATUS_CMD;

typedef enum {
    BT_ANT_TYPE_UNDEF=0,
    BT_ANT_TYPE_DUAL,
    BT_ANT_TYPE_SPLITTER,
    BT_ANT_TYPE_SWITCH
} BT_ANT_FRONTEND_CONFIG;

typedef enum {
    BT_COLOCATED_DEV_BTS4020=0,
    BT_COLCATED_DEV_CSR ,
    BT_COLOCATED_DEV_VALKYRIE
} BT_COLOCATED_DEV_TYPE;

#define BT_SCO_ALLOW_CLOSE_RANGE_OPT    (1 << 0)
#define BT_SCO_FORCE_AWAKE_OPT          (1 << 1)
#define BT_SCO_SET_DEFAULT_OVERRIDE(flags)        ((flags) |= (1 << 2))
#define BT_SCO_GET_DEFAULT_OVERRIDE(flags)        (((flags) >> 2) & 0x1)
typedef PREPACK struct {
    A_UINT32 numScoCyclesForceTrigger;  /* Number SCO cycles after which
                                           force a pspoll. default = 10 */
    A_UINT32 dataResponseTimeout;       /* Timeout Waiting for Downlink pkt
                                           in response for ps-poll,
                                           default = 10 msecs */
    A_UINT32  stompScoRules;
    A_UINT32 scoOptFlags;               /* SCO Options Flags :
                                            bits:     meaning:
                                             0        Allow Close Range Optimization
                                             1        Force awake during close range
                                             2        If set use host supplied threshold
                                             3        Unused
                                             4..7     Unused
                                             8..15    Unused
                                             16..23   Unused
                                        */
    A_UINT32 p2lrpOptModeBound;          /*p2lrp=packetToLowRatePacketRatio. In opt mode
                                          minimum ratio required to continue in opt mode*/
    A_UINT32 p2lrpNonOptModeBound;       /*In Non opt mode minimum ratio required to switch
                                          to opt mode*/
    A_UINT8 stompDutyCyleVal;           /* Sco cycles to limit ps-poll queuing
                                           if stomped */
    A_UINT8 stompDutyCyleMaxVal;        /*firm ware increases stomp duty cycle
                                          gradually uptill this value on need basis*/
    A_UINT8 psPollLatencyFraction;      /* Fraction of idle
                                           period, within which
                                           additional ps-polls
                                           can be queued */
    A_UINT8 noSCOSlots;                 /* Number of SCO Tx/Rx slots.
                                           HVx, EV3, 2EV3 = 2 */
    A_UINT8 noIdleSlots;                /* Number of Bluetooth idle slots between
                                           consecutive SCO Tx/Rx slots
                                           HVx, EV3 = 4
                                           2EV3 = 10 */
    A_UINT8 reserved8;                   /* maintain word algnment*/
} POSTPACK BT_PARAMS_SCO;

#define BT_A2DP_ALLOW_CLOSE_RANGE_OPT  (1 << 0)
#define BT_A2DP_FORCE_AWAKE_OPT        (1 << 1)
#define BT_A2DP_SET_DEFAULT_OVERRIDE(flags)        ((flags) |= (1 << 2))
#define BT_A2DP_GET_DEFAULT_OVERRIDE(flags)        (((flags) >> 2) & 0x1)
typedef PREPACK struct {
    A_UINT32 a2dpWlanUsageLimit; /* MAX time firmware uses the medium for
                                    wlan, after it identifies the idle time
                                    default (30 msecs) */
    A_UINT32 a2dpBurstCntMin;   /* Minimum number of bluetooth data frames
                                   to replenish Wlan Usage  limit (default 3) */
    A_UINT32 a2dpDataRespTimeout;
    A_UINT32 a2dpOptFlags;      /* A2DP Option flags:
                                       bits:    meaning:
                                        0       Allow Close Range Optimization
                                        1       Force awake during close range
                                        2       If set use host threshold
                                        3       Unused
                                        4..7    Unused
                                        8..15   Unused
                                        16..23  Unused
                                 */
    A_UINT32 p2lrpOptModeBound;    /*p2lrp=packetToLowRatePacketRatio. In opt mode
                                  minimum ratio required to continue in opt mode*/
    A_UINT32 p2lrpNonOptModeBound; /*In Non opt mode minimum ratio required to switch
                                  to opt mode*/
    A_UINT16 reserved16;          /*maintain word alignment*/
    A_UINT8 isCoLocatedBtRoleMaster;
    A_UINT8 reserved8;           /*maintain word alignment*/
}POSTPACK BT_PARAMS_A2DP;

/* During BT ftp/ BT OPP or any another data based acl profile on bluetooth
   (non a2dp).*/
typedef PREPACK struct {
    A_UINT32 aclWlanMediumUsageTime;  /* Wlan usage time during Acl (non-a2dp)
                                       coexistence (default 30 msecs) */
    A_UINT32 aclBtMediumUsageTime;   /* Bt usage time during acl coexistence
                                       (default 30 msecs)*/
    A_UINT32 aclDataRespTimeout;
    A_UINT32 aclDetectTimeout;      /* ACL coexistence enabled if we get
                                       10 Pkts in X msec(default 100 msecs) */
    A_UINT32 aclmaxPktCnt;          /* No of ACL pkts to receive before
                                         enabling ACL coex */

}POSTPACK BT_PARAMS_ACLCOEX;

typedef PREPACK struct {
    PREPACK union {
        BT_PARAMS_SCO scoParams;
        BT_PARAMS_A2DP a2dpParams;
        BT_PARAMS_ACLCOEX  aclCoexParams;
        A_UINT8 antType;         /* 0 -Disabled (default)
                                     1 - BT_ANT_TYPE_DUAL
                                     2 - BT_ANT_TYPE_SPLITTER
                                     3 - BT_ANT_TYPE_SWITCH */
        A_UINT8 coLocatedBtDev;  /* 0 - BT_COLOCATED_DEV_BTS4020 (default)
                                     1 - BT_COLCATED_DEV_CSR
                                     2 - BT_COLOCATED_DEV_VALKYRIe
                                   */
    } POSTPACK info;
    A_UINT8 paramType ;
} POSTPACK WMI_SET_BT_PARAMS_CMD;

typedef enum {
    DISCONN_EVT_IN_RECONN = 0,  /* default */
    NO_DISCONN_EVT_IN_RECONN
} TARGET_EVENT_REPORT_CONFIG;

typedef PREPACK struct {
    A_UINT32 evtConfig;
} POSTPACK WMI_SET_TARGET_EVENT_REPORT_CMD;


/*
 * Command Replies
 */

/*
 * WMI_GET_CHANNEL_LIST_CMDID reply
 */
typedef PREPACK struct {
    A_UINT8     reserved1;
    A_UINT8     numChannels;            /* number of channels in reply */
    A_UINT16    channelList[1];         /* channel in Mhz */
} POSTPACK WMI_CHANNEL_LIST_REPLY;

typedef enum {
    A_SUCCEEDED = A_OK,
    A_FAILED_DELETE_STREAM_DOESNOT_EXIST=250,
    A_SUCCEEDED_MODIFY_STREAM=251,
    A_FAILED_INVALID_STREAM = 252,
    A_FAILED_MAX_THINSTREAMS = 253,
    A_FAILED_CREATE_REMOVE_PSTREAM_FIRST = 254,
} PSTREAM_REPLY_STATUS;

typedef PREPACK struct {
    A_UINT8     status;                 /* PSTREAM_REPLY_STATUS */
    A_UINT8     txQueueNumber;
    A_UINT8     rxQueueNumber;
    A_UINT8     trafficClass;
    A_UINT8     trafficDirection;       /* DIR_TYPE */
} POSTPACK WMI_CRE_PRIORITY_STREAM_REPLY;

typedef PREPACK struct {
    A_UINT8     status;                 /* PSTREAM_REPLY_STATUS */
    A_UINT8     txQueueNumber;
    A_UINT8     rxQueueNumber;
    A_UINT8     trafficDirection;       /* DIR_TYPE */
    A_UINT8     trafficClass;
} POSTPACK WMI_DEL_PRIORITY_STREAM_REPLY;

/*
 * List of Events (target to host)
 */
typedef enum {
    WMI_READY_EVENTID           = 0x1001,
    WMI_CONNECT_EVENTID,
    WMI_DISCONNECT_EVENTID,
    WMI_BSSINFO_EVENTID,
    WMI_CMDERROR_EVENTID,
    WMI_REGDOMAIN_EVENTID,
    WMI_PSTREAM_TIMEOUT_EVENTID,
    WMI_NEIGHBOR_REPORT_EVENTID,
    WMI_TKIP_MICERR_EVENTID,
    WMI_SCAN_COMPLETE_EVENTID,           /* 0x100a */
    WMI_REPORT_STATISTICS_EVENTID,
    WMI_RSSI_THRESHOLD_EVENTID,
    WMI_ERROR_REPORT_EVENTID,
    WMI_OPT_RX_FRAME_EVENTID,
    WMI_REPORT_ROAM_TBL_EVENTID,
    WMI_EXTENSION_EVENTID,
    WMI_CAC_EVENTID,
    WMI_SNR_THRESHOLD_EVENTID,
    WMI_LQ_THRESHOLD_EVENTID,
    WMI_TX_RETRY_ERR_EVENTID,            /* 0x1014 */
    WMI_REPORT_ROAM_DATA_EVENTID,
    WMI_TEST_EVENTID,
    WMI_APLIST_EVENTID,
    WMI_GET_WOW_LIST_EVENTID,
    WMI_GET_PMKID_LIST_EVENTID,
    WMI_CHANNEL_CHANGE_EVENTID,
    WMI_PEER_NODE_EVENTID,
    WMI_PSPOLL_EVENTID,
    WMI_DTIMEXPIRY_EVENTID,
    WMI_WLAN_VERSION_EVENTID,
    WMI_SET_PARAMS_REPLY_EVENTID,
    WMI_ACM_REJECT_EVENTID
} WMI_EVENT_ID;

typedef enum {
    WMI_11A_CAPABILITY   = 1,
    WMI_11G_CAPABILITY   = 2,
    WMI_11AG_CAPABILITY  = 3,
} WMI_PHY_CAPABILITY;

typedef PREPACK struct {
    A_UINT8     macaddr[ATH_MAC_LEN];
    A_UINT8     phyCapability;              /* WMI_PHY_CAPABILITY */
} POSTPACK WMI_READY_EVENT_1;

typedef PREPACK struct {
    A_UINT32    version;
    A_UINT8     macaddr[ATH_MAC_LEN];
    A_UINT8     phyCapability;              /* WMI_PHY_CAPABILITY */
} POSTPACK WMI_READY_EVENT_2;

#if defined(ATH_TARGET)
#ifdef AR6002_REV2
#define WMI_READY_EVENT WMI_READY_EVENT_1  /* AR6002_REV2 target code */
#else
#define WMI_READY_EVENT WMI_READY_EVENT_2  /* AR6002_REV4 and AR6001 */
#endif
#else
#define WMI_READY_EVENT WMI_READY_EVENT_2 /* host code */
#endif


/*
 * Connect Event
 */
typedef PREPACK struct {
    A_UINT16    channel;
    A_UINT8     bssid[ATH_MAC_LEN];
    A_UINT16    listenInterval;
    A_UINT16    beaconInterval;
    A_UINT32    networkType;
    A_UINT8     beaconIeLen;
    A_UINT8     assocReqLen;
    A_UINT8     assocRespLen;
    A_UINT8     assocInfo[1];
} POSTPACK WMI_CONNECT_EVENT;

/*
 * Disconnect Event
 */
typedef enum {
    NO_NETWORK_AVAIL   = 0x01,
    LOST_LINK          = 0x02,     /* bmiss */
    DISCONNECT_CMD     = 0x03,
    BSS_DISCONNECTED   = 0x04,
    AUTH_FAILED        = 0x05,
    ASSOC_FAILED       = 0x06,
    NO_RESOURCES_AVAIL = 0x07,
    CSERV_DISCONNECT   = 0x08,
    INVALID_PROFILE    = 0x0a,
    DOT11H_CHANNEL_SWITCH = 0x0b,
    PROFILE_MISMATCH        = 0x0c,
    PYXIS_VIRT_ADHOC_DISC   = 0x0d,
} WMI_DISCONNECT_REASON;

typedef PREPACK struct {
    A_UINT16    protocolReasonStatus;  /* reason code, see 802.11 spec. */
    A_UINT8     bssid[ATH_MAC_LEN];    /* set if known */
    A_UINT8     disconnectReason ;      /* see WMI_DISCONNECT_REASON */
    A_UINT8     assocRespLen;
    A_UINT8     assocInfo[1];
} POSTPACK WMI_DISCONNECT_EVENT;

/*
 * BSS Info Event.
 * Mechanism used to inform host of the presence and characteristic of
 * wireless networks present.  Consists of bss info header followed by
 * the beacon or probe-response frame body.  The 802.11 header is not included.
 */
typedef enum {
    BEACON_FTYPE = 0x1,
    PROBERESP_FTYPE,
    ACTION_MGMT_FTYPE,
    PROBEREQ_FTYPE,
} WMI_BI_FTYPE;

enum {
    BSS_ELEMID_CHANSWITCH = 0x01,
    BSS_ELEMID_ATHEROS = 0x02,
};

typedef PREPACK struct {
    A_UINT16    channel;
    A_UINT8     frameType;          /* see WMI_BI_FTYPE */
    A_UINT8     snr;
    A_INT16     rssi;
    A_UINT8     bssid[ATH_MAC_LEN];
    A_UINT32    ieMask;
} POSTPACK WMI_BSS_INFO_HDR;

/*
 * Command Error Event
 */
typedef enum {
    INVALID_PARAM  = 0x01,
    ILLEGAL_STATE  = 0x02,
    INTERNAL_ERROR = 0x03,
} WMI_ERROR_CODE;

typedef PREPACK struct {
    A_UINT16    commandId;
    A_UINT8     errorCode;
} POSTPACK WMI_CMD_ERROR_EVENT;

/*
 * New Regulatory Domain Event
 */
typedef PREPACK struct {
    A_UINT32    regDomain;
} POSTPACK WMI_REG_DOMAIN_EVENT;

typedef PREPACK struct {
    A_UINT8     txQueueNumber;
    A_UINT8     rxQueueNumber;
    A_UINT8     trafficDirection;
    A_UINT8     trafficClass;
} POSTPACK WMI_PSTREAM_TIMEOUT_EVENT;

typedef PREPACK struct {
    A_UINT8     reserve1;
    A_UINT8     reserve2;
    A_UINT8     reserve3;
    A_UINT8     trafficClass;
} POSTPACK WMI_ACM_REJECT_EVENT;

/*
 * The WMI_NEIGHBOR_REPORT Event is generated by the target to inform
 * the host of BSS's it has found that matches the current profile.
 * It can be used by the host to cache PMKs and/to initiate pre-authentication
 * if the BSS supports it.  The first bssid is always the current associated
 * BSS.
 * The bssid and bssFlags information repeats according to the number
 * or APs reported.
 */
typedef enum {
    WMI_DEFAULT_BSS_FLAGS   = 0x00,
    WMI_PREAUTH_CAPABLE_BSS = 0x01,
    WMI_PMKID_VALID_BSS     = 0x02,
} WMI_BSS_FLAGS;

typedef PREPACK struct {
    A_UINT8     bssid[ATH_MAC_LEN];
    A_UINT8     bssFlags;            /* see WMI_BSS_FLAGS */
} POSTPACK WMI_NEIGHBOR_INFO;

typedef PREPACK struct {
    A_INT8      numberOfAps;
    WMI_NEIGHBOR_INFO neighbor[1];
} POSTPACK WMI_NEIGHBOR_REPORT_EVENT;

/*
 * TKIP MIC Error Event
 */
typedef PREPACK struct {
    A_UINT8 keyid;
    A_UINT8 ismcast;
} POSTPACK WMI_TKIP_MICERR_EVENT;

/*
 * WMI_SCAN_COMPLETE_EVENTID - no parameters (old), staus parameter (new)
 */
typedef PREPACK struct {
    A_INT32 status;
} POSTPACK WMI_SCAN_COMPLETE_EVENT;

#define MAX_OPT_DATA_LEN 1400

/*
 * WMI_SET_ADHOC_BSSID_CMDID
 */
typedef PREPACK struct {
    A_UINT8     bssid[ATH_MAC_LEN];
} POSTPACK WMI_SET_ADHOC_BSSID_CMD;

/*
 * WMI_SET_OPT_MODE_CMDID
 */
typedef enum {
    SPECIAL_OFF,
    SPECIAL_ON,
    PYXIS_ADHOC_ON,
    PYXIS_ADHOC_OFF,
} OPT_MODE_TYPE;

typedef PREPACK struct {
    A_UINT8     optMode;
} POSTPACK WMI_SET_OPT_MODE_CMD;

/*
 * WMI_TX_OPT_FRAME_CMDID
 */
typedef enum {
    OPT_PROBE_REQ   = 0x01,
    OPT_PROBE_RESP  = 0x02,
    OPT_CPPP_START  = 0x03,
    OPT_CPPP_STOP   = 0x04,
} WMI_OPT_FTYPE;

typedef PREPACK struct {
    A_UINT16    optIEDataLen;
    A_UINT8     frmType;
    A_UINT8     dstAddr[ATH_MAC_LEN];
    A_UINT8     bssid[ATH_MAC_LEN];
    A_UINT8     reserved;               /* For alignment */
    A_UINT8     optIEData[1];
} POSTPACK WMI_OPT_TX_FRAME_CMD;

/*
 * Special frame receive Event.
 * Mechanism used to inform host of the receiption of the special frames.
 * Consists of special frame info header followed by special frame body.
 * The 802.11 header is not included.
 */
typedef PREPACK struct {
    A_UINT16    channel;
    A_UINT8     frameType;          /* see WMI_OPT_FTYPE */
    A_INT8      snr;
    A_UINT8     srcAddr[ATH_MAC_LEN];
    A_UINT8     bssid[ATH_MAC_LEN];
} POSTPACK WMI_OPT_RX_INFO_HDR;

/*
 * Reporting statistics.
 */
typedef PREPACK struct {
    A_UINT32   tx_packets;
    A_UINT32   tx_bytes;
    A_UINT32   tx_unicast_pkts;
    A_UINT32   tx_unicast_bytes;
    A_UINT32   tx_multicast_pkts;
    A_UINT32   tx_multicast_bytes;
    A_UINT32   tx_broadcast_pkts;
    A_UINT32   tx_broadcast_bytes;
    A_UINT32   tx_rts_success_cnt;
    A_UINT32   tx_packet_per_ac[4];
    A_UINT32   tx_errors_per_ac[4];

    A_UINT32   tx_errors;
    A_UINT32   tx_failed_cnt;
    A_UINT32   tx_retry_cnt;
    A_UINT32   tx_mult_retry_cnt;
    A_UINT32   tx_rts_fail_cnt;
    A_INT32    tx_unicast_rate;
}POSTPACK tx_stats_t;

typedef PREPACK struct {
    A_UINT32   rx_packets;
    A_UINT32   rx_bytes;
    A_UINT32   rx_unicast_pkts;
    A_UINT32   rx_unicast_bytes;
    A_UINT32   rx_multicast_pkts;
    A_UINT32   rx_multicast_bytes;
    A_UINT32   rx_broadcast_pkts;
    A_UINT32   rx_broadcast_bytes;
    A_UINT32   rx_fragment_pkt;

    A_UINT32   rx_errors;
    A_UINT32   rx_crcerr;
    A_UINT32   rx_key_cache_miss;
    A_UINT32   rx_decrypt_err;
    A_UINT32   rx_duplicate_frames;
    A_INT32    rx_unicast_rate;
}POSTPACK rx_stats_t;

typedef PREPACK struct {
    A_UINT32   tkip_local_mic_failure;
    A_UINT32   tkip_counter_measures_invoked;
    A_UINT32   tkip_replays;
    A_UINT32   tkip_format_errors;
    A_UINT32   ccmp_format_errors;
    A_UINT32   ccmp_replays;
}POSTPACK tkip_ccmp_stats_t;

typedef PREPACK struct {
    A_UINT32   power_save_failure_cnt;
}POSTPACK pm_stats_t;

typedef PREPACK struct {
    A_UINT32    cs_bmiss_cnt;
    A_UINT32    cs_lowRssi_cnt;
    A_UINT16    cs_connect_cnt;
    A_UINT16    cs_disconnect_cnt;
    A_INT16     cs_aveBeacon_rssi;
    A_UINT16    cs_roam_count;
    A_INT16     cs_rssi;
    A_UINT8     cs_snr;
    A_UINT8     cs_aveBeacon_snr;
    A_UINT8     cs_lastRoam_msec;
} POSTPACK cserv_stats_t;

typedef PREPACK struct {
    tx_stats_t          tx_stats;
    rx_stats_t          rx_stats;
    tkip_ccmp_stats_t   tkipCcmpStats;
}POSTPACK wlan_net_stats_t;

typedef PREPACK struct {
    A_UINT32    arp_received;
    A_UINT32    arp_matched;
    A_UINT32    arp_replied;
} POSTPACK arp_stats_t;

typedef PREPACK struct {
    A_UINT32    wow_num_pkts_dropped;
    A_UINT16    wow_num_events_discarded;
    A_UINT8     wow_num_host_pkt_wakeups;
    A_UINT8     wow_num_host_event_wakeups;
} POSTPACK wlan_wow_stats_t;

typedef PREPACK struct {
    A_UINT32            lqVal;
    A_INT32             noise_floor_calibation;
    pm_stats_t          pmStats;
    wlan_net_stats_t    txrxStats;
    wlan_wow_stats_t    wowStats;
    arp_stats_t         arpStats;
    cserv_stats_t       cservStats;
} POSTPACK WMI_TARGET_STATS;

/*
 * WMI_RSSI_THRESHOLD_EVENTID.
 * Indicate the RSSI events to host. Events are indicated when we breach a
 * thresold value.
 */
typedef enum{
    WMI_RSSI_THRESHOLD1_ABOVE = 0,
    WMI_RSSI_THRESHOLD2_ABOVE,
    WMI_RSSI_THRESHOLD3_ABOVE,
    WMI_RSSI_THRESHOLD4_ABOVE,
    WMI_RSSI_THRESHOLD5_ABOVE,
    WMI_RSSI_THRESHOLD6_ABOVE,
    WMI_RSSI_THRESHOLD1_BELOW,
    WMI_RSSI_THRESHOLD2_BELOW,
    WMI_RSSI_THRESHOLD3_BELOW,
    WMI_RSSI_THRESHOLD4_BELOW,
    WMI_RSSI_THRESHOLD5_BELOW,
    WMI_RSSI_THRESHOLD6_BELOW
}WMI_RSSI_THRESHOLD_VAL;

typedef PREPACK struct {
    A_INT16 rssi;
    A_UINT8 range;
}POSTPACK WMI_RSSI_THRESHOLD_EVENT;

/*
 *  WMI_ERROR_REPORT_EVENTID
 */
typedef enum{
    WMI_TARGET_PM_ERR_FAIL      = 0x00000001,
    WMI_TARGET_KEY_NOT_FOUND    = 0x00000002,
    WMI_TARGET_DECRYPTION_ERR   = 0x00000004,
    WMI_TARGET_BMISS            = 0x00000008,
    WMI_PSDISABLE_NODE_JOIN     = 0x00000010,
    WMI_TARGET_COM_ERR          = 0x00000020,
    WMI_TARGET_FATAL_ERR        = 0x00000040
} WMI_TARGET_ERROR_VAL;

typedef PREPACK struct {
    A_UINT32 errorVal;
}POSTPACK  WMI_TARGET_ERROR_REPORT_EVENT;

typedef PREPACK struct {
    A_UINT8 retrys;
}POSTPACK  WMI_TX_RETRY_ERR_EVENT;

typedef enum{
    WMI_SNR_THRESHOLD1_ABOVE = 1,
    WMI_SNR_THRESHOLD1_BELOW,
    WMI_SNR_THRESHOLD2_ABOVE,
    WMI_SNR_THRESHOLD2_BELOW,
    WMI_SNR_THRESHOLD3_ABOVE,
    WMI_SNR_THRESHOLD3_BELOW,
    WMI_SNR_THRESHOLD4_ABOVE,
    WMI_SNR_THRESHOLD4_BELOW
} WMI_SNR_THRESHOLD_VAL;

typedef PREPACK struct {
    A_UINT8 range;  /* WMI_SNR_THRESHOLD_VAL */
    A_UINT8 snr;
}POSTPACK  WMI_SNR_THRESHOLD_EVENT;

typedef enum{
    WMI_LQ_THRESHOLD1_ABOVE = 1,
    WMI_LQ_THRESHOLD1_BELOW,
    WMI_LQ_THRESHOLD2_ABOVE,
    WMI_LQ_THRESHOLD2_BELOW,
    WMI_LQ_THRESHOLD3_ABOVE,
    WMI_LQ_THRESHOLD3_BELOW,
    WMI_LQ_THRESHOLD4_ABOVE,
    WMI_LQ_THRESHOLD4_BELOW
} WMI_LQ_THRESHOLD_VAL;

typedef PREPACK struct {
    A_INT32 lq;
    A_UINT8 range;  /* WMI_LQ_THRESHOLD_VAL */
}POSTPACK  WMI_LQ_THRESHOLD_EVENT;
/*
 * WMI_REPORT_ROAM_TBL_EVENTID
 */
#define MAX_ROAM_TBL_CAND   5

typedef PREPACK struct {
    A_INT32 roam_util;
    A_UINT8 bssid[ATH_MAC_LEN];
    A_INT8  rssi;
    A_INT8  rssidt;
    A_INT8  last_rssi;
    A_INT8  util;
    A_INT8  bias;
    A_UINT8 reserved; /* For alignment */
} POSTPACK WMI_BSS_ROAM_INFO;


typedef PREPACK struct {
    A_UINT16  roamMode;
    A_UINT16  numEntries;
    WMI_BSS_ROAM_INFO bssRoamInfo[1];
} POSTPACK WMI_TARGET_ROAM_TBL;

/*
 *  WMI_CAC_EVENTID
 */
typedef enum {
    CAC_INDICATION_ADMISSION = 0x00,
    CAC_INDICATION_ADMISSION_RESP = 0x01,
    CAC_INDICATION_DELETE = 0x02,
    CAC_INDICATION_NO_RESP = 0x03,
}CAC_INDICATION;

#define WMM_TSPEC_IE_LEN   63

typedef PREPACK struct {
    A_UINT8 ac;
    A_UINT8 cac_indication;
    A_UINT8 statusCode;
    A_UINT8 tspecSuggestion[WMM_TSPEC_IE_LEN];
}POSTPACK  WMI_CAC_EVENT;

/*
 * WMI_APLIST_EVENTID
 */

typedef enum {
    APLIST_VER1 = 1,
} APLIST_VER;

typedef PREPACK struct {
    A_UINT8     bssid[ATH_MAC_LEN];
    A_UINT16    channel;
} POSTPACK  WMI_AP_INFO_V1;

typedef PREPACK union {
    WMI_AP_INFO_V1  apInfoV1;
} POSTPACK WMI_AP_INFO;

typedef PREPACK struct {
    A_UINT8     apListVer;
    A_UINT8     numAP;
    WMI_AP_INFO apList[1];
} POSTPACK WMI_APLIST_EVENT;

/*
 * developer commands
 */

/*
 * WMI_SET_BITRATE_CMDID
 *
 * Get bit rate cmd uses same definition as set bit rate cmd
 */
typedef enum {
    RATE_AUTO   = -1,
    RATE_1Mb    = 0,
    RATE_2Mb    = 1,
    RATE_5_5Mb  = 2,
    RATE_11Mb   = 3,
    RATE_6Mb    = 4,
    RATE_9Mb    = 5,
    RATE_12Mb   = 6,
    RATE_18Mb   = 7,
    RATE_24Mb   = 8,
    RATE_36Mb   = 9,
    RATE_48Mb   = 10,
    RATE_54Mb   = 11,
} WMI_BIT_RATE;

typedef PREPACK struct {
    A_INT8      rateIndex;          /* see WMI_BIT_RATE */
    A_INT8      mgmtRateIndex;
    A_INT8      ctlRateIndex;
} POSTPACK WMI_BIT_RATE_CMD;


typedef PREPACK struct {
    A_INT8      rateIndex;          /* see WMI_BIT_RATE */
} POSTPACK  WMI_BIT_RATE_REPLY;


/*
 * WMI_SET_FIXRATES_CMDID
 *
 * Get fix rates cmd uses same definition as set fix rates cmd
 */
typedef enum {
    FIX_RATE_1Mb    = 0x1,
    FIX_RATE_2Mb    = 0x2,
    FIX_RATE_5_5Mb  = 0x4,
    FIX_RATE_11Mb   = 0x8,
    FIX_RATE_6Mb    = 0x10,
    FIX_RATE_9Mb    = 0x20,
    FIX_RATE_12Mb   = 0x40,
    FIX_RATE_18Mb   = 0x80,
    FIX_RATE_24Mb   = 0x100,
    FIX_RATE_36Mb   = 0x200,
    FIX_RATE_48Mb   = 0x400,
    FIX_RATE_54Mb   = 0x800,
} WMI_FIX_RATES_MASK;

typedef PREPACK struct {
    A_UINT16      fixRateMask;          /* see WMI_BIT_RATE */
} POSTPACK WMI_FIX_RATES_CMD, WMI_FIX_RATES_REPLY;

typedef PREPACK struct {
    A_UINT8        bEnableMask;
    A_UINT8        frameType;               /*type and subtype*/
    A_UINT16      frameRateMask;          /* see WMI_BIT_RATE */
} POSTPACK WMI_FRAME_RATES_CMD, WMI_FRAME_RATES_REPLY;

/*
 * WMI_SET_RECONNECT_AUTH_MODE_CMDID
 *
 * Set authentication mode
 */
typedef enum {
    RECONN_DO_AUTH = 0x00,
    RECONN_NOT_AUTH = 0x01
} WMI_AUTH_MODE;

typedef PREPACK struct {
    A_UINT8 mode;
} POSTPACK WMI_SET_AUTH_MODE_CMD;

/*
 * WMI_SET_REASSOC_MODE_CMDID
 *
 * Set authentication mode
 */
typedef enum {
    REASSOC_DO_DISASSOC = 0x00,
    REASSOC_DONOT_DISASSOC = 0x01
} WMI_REASSOC_MODE;

typedef PREPACK struct {
    A_UINT8 mode;
}POSTPACK WMI_SET_REASSOC_MODE_CMD;

typedef enum {
    ROAM_DATA_TIME = 1,            /* Get The Roam Time Data */
} ROAM_DATA_TYPE;

typedef PREPACK struct {
    A_UINT32        disassoc_time;
    A_UINT32        no_txrx_time;
    A_UINT32        assoc_time;
    A_UINT32        allow_txrx_time;
    A_UINT32        last_data_txrx_time;
    A_UINT32        first_data_txrx_time;
    A_UINT8         disassoc_bssid[ATH_MAC_LEN];
    A_INT8          disassoc_bss_rssi;
    A_UINT8         assoc_bssid[ATH_MAC_LEN];
    A_INT8          assoc_bss_rssi;
} POSTPACK WMI_TARGET_ROAM_TIME;

typedef PREPACK struct {
    PREPACK union {
        WMI_TARGET_ROAM_TIME roamTime;
    } POSTPACK u;
    A_UINT8 roamDataType ;
} POSTPACK WMI_TARGET_ROAM_DATA;

typedef enum {
    WMI_WMM_DISABLED = 0,
    WMI_WMM_ENABLED
} WMI_WMM_STATUS;

typedef PREPACK struct {
    A_UINT8    status;
}POSTPACK WMI_SET_WMM_CMD;

typedef enum {
    WMI_TXOP_DISABLED = 0,
    WMI_TXOP_ENABLED
} WMI_TXOP_CFG;

typedef PREPACK struct {
    A_UINT8    txopEnable;
}POSTPACK WMI_SET_WMM_TXOP_CMD;

typedef PREPACK struct {
    A_UINT8 keepaliveInterval;
} POSTPACK WMI_SET_KEEPALIVE_CMD;

typedef PREPACK struct {
    A_BOOL configured;
    A_UINT8 keepaliveInterval;
} POSTPACK WMI_GET_KEEPALIVE_CMD;

/*
 * Add Application specified IE to a management frame
 */
#define WMI_MAX_IE_LEN  255

typedef PREPACK struct {
    A_UINT8 mgmtFrmType;  /* one of WMI_MGMT_FRAME_TYPE */
    A_UINT8 ieLen;    /* Length  of the IE that should be added to the MGMT frame */
    A_UINT8 ieInfo[1];
} POSTPACK WMI_SET_APPIE_CMD;

/*
 * Notify the WSC registration status to the target
 */
#define WSC_REG_ACTIVE     1
#define WSC_REG_INACTIVE   0
/* Generic Hal Interface for setting hal paramters. */
/* Add new Set HAL Param cmdIds here for newer params */
typedef enum {
   WHAL_SETCABTO_CMDID = 1,
}WHAL_CMDID;

typedef PREPACK struct {
    A_UINT8 cabTimeOut;
} POSTPACK WHAL_SETCABTO_PARAM;

typedef PREPACK struct {
    A_UINT8  whalCmdId;
    A_UINT8 data[1];
} POSTPACK WHAL_PARAMCMD;


#define WOW_MAX_FILTER_LISTS 1 /*4*/
#define WOW_MAX_FILTERS_PER_LIST 4
#define WOW_PATTERN_SIZE 64
#define WOW_MASK_SIZE 64

#define MAC_MAX_FILTERS_PER_LIST 4

typedef PREPACK struct {
    A_UINT8 wow_valid_filter;
    A_UINT8 wow_filter_id;
    A_UINT8 wow_filter_size;
    A_UINT8 wow_filter_offset;
    A_UINT8 wow_filter_mask[WOW_MASK_SIZE];
    A_UINT8 wow_filter_pattern[WOW_PATTERN_SIZE];
} POSTPACK WOW_FILTER;


typedef PREPACK struct {
    A_UINT8 wow_valid_list;
    A_UINT8 wow_list_id;
    A_UINT8 wow_num_filters;
    A_UINT8 wow_total_list_size;
    WOW_FILTER list[WOW_MAX_FILTERS_PER_LIST];
} POSTPACK WOW_FILTER_LIST;

typedef PREPACK struct {
    A_UINT8 valid_filter;
    A_UINT8 mac_addr[ATH_MAC_LEN];
} POSTPACK MAC_FILTER;


typedef PREPACK struct {
    A_UINT8 total_list_size;
    MAC_FILTER list[MAC_MAX_FILTERS_PER_LIST];
} POSTPACK MAC_FILTER_LIST;

#define MAX_IP_ADDRS  2
typedef PREPACK struct {
    A_UINT32 ips[MAX_IP_ADDRS];  /* IP in Network Byte Order */
} POSTPACK WMI_SET_IP_CMD;

typedef PREPACK struct {
    A_BOOL awake;
    A_BOOL asleep;
} POSTPACK WMI_SET_HOST_SLEEP_MODE_CMD;

typedef PREPACK struct {
    A_BOOL enable_wow;
} POSTPACK WMI_SET_WOW_MODE_CMD;

typedef PREPACK struct {
    A_UINT8 filter_list_id;
} POSTPACK WMI_GET_WOW_LIST_CMD;

/*
 * WMI_GET_WOW_LIST_CMD reply
 */
typedef PREPACK struct {
    A_UINT8     num_filters;     /* number of patterns in reply */
    A_UINT8     this_filter_num; /*  this is filter # x of total num_filters */
    A_UINT8     wow_mode;
    A_UINT8     host_mode;
    WOW_FILTER  wow_filters[1];
} POSTPACK WMI_GET_WOW_LIST_REPLY;

typedef PREPACK struct {
    A_UINT8 filter_list_id;
    A_UINT8 filter_size;
    A_UINT8 filter_offset;
    A_UINT8 filter[1];
} POSTPACK WMI_ADD_WOW_PATTERN_CMD;

typedef PREPACK struct {
    A_UINT16 filter_list_id;
    A_UINT16 filter_id;
} POSTPACK WMI_DEL_WOW_PATTERN_CMD;

typedef PREPACK struct {
    A_UINT8 macaddr[ATH_MAC_LEN];
} POSTPACK WMI_SET_MAC_ADDRESS_CMD;

/*
 * WMI_SET_AKMP_PARAMS_CMD
 */

#define WMI_AKMP_MULTI_PMKID_EN   0x000001

typedef PREPACK struct {
    A_UINT32    akmpInfo;
} POSTPACK WMI_SET_AKMP_PARAMS_CMD;

typedef PREPACK struct {
    A_UINT8 pmkid[WMI_PMKID_LEN];
} POSTPACK WMI_PMKID;

/*
 * WMI_SET_PMKID_LIST_CMD
 */
#define WMI_MAX_PMKID_CACHE   8

typedef PREPACK struct {
    A_UINT32    numPMKID;
    WMI_PMKID   pmkidList[WMI_MAX_PMKID_CACHE];
} POSTPACK WMI_SET_PMKID_LIST_CMD;

/*
 * WMI_GET_PMKID_LIST_CMD  Reply
 * Following the Number of PMKIDs is the list of PMKIDs
 */
typedef PREPACK struct {
    A_UINT32    numPMKID;
    A_UINT8     bssidList[ATH_MAC_LEN][1];
    WMI_PMKID   pmkidList[1];
} POSTPACK WMI_PMKID_LIST_REPLY;

typedef PREPACK struct {
    A_UINT16 oldChannel;
    A_UINT32 newChannel;
} POSTPACK WMI_CHANNEL_CHANGE_EVENT;

typedef PREPACK struct {
    A_UINT32 version;
} POSTPACK WMI_WLAN_VERSION_EVENT;

#define PEER_NODE_JOIN_EVENT 0x00
#define PEER_NODE_LEAVE_EVENT 0x01
#define PEER_FIRST_NODE_JOIN_EVENT 0x10
#define PEER_LAST_NODE_LEAVE_EVENT 0x11
typedef PREPACK struct {
    A_UINT8 eventCode;
    A_UINT8 peerMacAddr[ATH_MAC_LEN];
} POSTPACK WMI_PEER_NODE_EVENT;

#define IEEE80211_FRAME_TYPE_MGT          0x00
#define IEEE80211_FRAME_TYPE_CTL          0x04


typedef enum {
    WMI_PYXIS_GEN_PARAMS = 0,
    WMI_PYXIS_DSCVR_PARAMS,
    WMI_PYXIS_SET_TX_MODE,
} WMI_PYXIS_CONFIG_TYPE;

typedef PREPACK struct {
    A_UINT16   pyxisConfigType;  /* One of WMI_PYXIS_CONFIG_TYPE */
    A_UINT16   pyxisConfigLen;   /* Length in Bytes of Information that follows */
} POSTPACK WMI_PYXIS_CONFIG_HDR;

typedef PREPACK struct {
    WMI_PYXIS_CONFIG_HDR        hdr;
    A_UINT32                    dscvrWindow;
    A_UINT32                    dscvrInterval;
    A_UINT32                    dscvrLife;
    A_UINT32                    probeInterval;
    A_UINT32                    probePeriod;
    A_UINT16                    dscvrChannel;
} POSTPACK WMI_PYXIS_DSCVR_CONFIG;

typedef PREPACK struct {
    WMI_PYXIS_CONFIG_HDR        hdr;
    A_UINT32                    dataWindowSizeMin;
    A_UINT32                    dataWindowSizeMax;
    A_UINT8                     maxJoiners;
} POSTPACK WMI_PYXIS_GEN_CONFIG;

typedef PREPACK struct {
    WMI_PYXIS_CONFIG_HDR        hdr;
    A_BOOL                      mode;
} POSTPACK WMI_PYXIS_TX_MODE;

typedef enum {
    WMI_PYXIS_DISC_PEER = 0,
    WMI_PYXIS_JOIN_PEER,
} WMI_PYXIS_CMD_TYPE;

typedef PREPACK struct {
    A_UINT16  pyxisCmd;
    A_UINT16  pyxisCmdLen;  /* Length following this header */
} POSTPACK WMI_PYXIS_CMD_HDR;

typedef PREPACK struct {
    WMI_PYXIS_CMD_HDR   hdr;
    A_UINT8    peerMacAddr[ATH_MAC_LEN];
} POSTPACK WMI_PYXIS_DISCONNECT_CMD;

typedef PREPACK struct {
    WMI_PYXIS_CMD_HDR hdr;
    A_UINT32    ctrl_flags;             /* One of the Bits determines if it
                                         * is Virt Adhoc/the device is to
                                         * join a BSS */
    A_UINT16    channel;                /* Data Channel */
    A_UINT8     networkType;            /* network type */
    A_UINT8     dot11AuthMode;          /* OPEN_AUTH */
    A_UINT8     authMode;               /* NONE_AUTH */
    A_UINT8     pairwiseCryptoType;     /* One of NONE_CRYPT, AES_CRYPT */
    A_UINT8     pairwiseCryptoLen;      /* 0 since ADD_KEY passes the length */
    A_UINT8     groupCryptoType;        /* One of NONE_CRYPT, AES_CRYPT */
    A_UINT8     groupCryptoLen;         /* 0 since ADD_KEY passes the length */
    A_UINT8     peerMacAddr[ATH_MAC_LEN];  /* BSSID of peer network*/
    A_UINT8     nwBSSID[ATH_MAC_LEN];     /* BSSID of the Pyxis Adhoc Network */
} POSTPACK WMI_PYXIS_JOIN_CMD;

typedef PREPACK struct {
    WMI_PYXIS_CMD_HDR   hdr;
    A_BOOL      mode;
} POSTPACK WMI_PYXIS_SET_TX_MODE_CMD;

/*
 * ------- AP Mode definitions --------------
 */

/*
 * !!! Warning !!!
 * -Changing the following values needs compilation of both driver and firmware
 */
#define AP_MAX_NUM_STA          8
#define AP_ACL_SIZE             10
#define IEEE80211_MAX_IE        256
#define MCAST_AID               0xFF /* Spl. AID used to set DTIM flag in the beacons */
#define DEF_AP_COUNTRY_CODE     "US "
#define DEF_AP_WMODE_G          WMI_11G_MODE
#define DEF_AP_WMODE_AG         WMI_11AG_MODE
#define DEF_AP_DTIM             5
#define DEF_BEACON_INTERVAL     100

/* AP mode disconnect reasons */
#define AP_DISCONNECT_STA_LEFT      101
#define AP_DISCONNECT_FROM_HOST     102
#define AP_DISCONNECT_COMM_TIMEOUT  103

/*
 * Used with WMI_AP_HIDDEN_SSID_CMDID
 */
#define HIDDEN_SSID_FALSE   0
#define HIDDEN_SSID_TRUE    1
typedef PREPACK struct {
    A_UINT8     hidden_ssid;
} POSTPACK WMI_AP_HIDDEN_SSID_CMD;

/*
 * Used with WMI_AP_ACL_POLICY_CMDID
 */
#define AP_ACL_DISABLE          0x00
#define AP_ACL_ALLOW_MAC        0x01
#define AP_ACL_DENY_MAC         0x02
#define AP_ACL_RETAIN_LIST_MASK 0x80
typedef PREPACK struct {
    A_UINT8     policy;
} POSTPACK WMI_AP_ACL_POLICY_CMD;

/*
 * Used with WMI_AP_ACL_MAC_LIST_CMDID
 */
#define ADD_MAC_ADDR    1
#define DEL_MAC_ADDR    2
typedef PREPACK struct {
    A_UINT8     action;
    A_UINT8     index;
    A_UINT8     mac[ATH_MAC_LEN];
    A_UINT8     wildcard;
} POSTPACK WMI_AP_ACL_MAC_CMD;

typedef PREPACK struct {
    A_UINT16    index;
    A_UINT8     acl_mac[AP_ACL_SIZE][ATH_MAC_LEN];
    A_UINT8     wildcard[AP_ACL_SIZE];
    A_UINT8     policy;
} POSTPACK WMI_AP_ACL;

/*
 * Used with WMI_AP_SET_NUM_STA_CMDID
 */
typedef PREPACK struct {
    A_UINT8     num_sta;
} POSTPACK WMI_AP_SET_NUM_STA_CMD;

/*
 * Used with WMI_AP_SET_MLME_CMDID
 */
typedef PREPACK struct {
    A_UINT8    mac[ATH_MAC_LEN];
    A_UINT16   reason;              /* 802.11 reason code */
    A_UINT8    cmd;                 /* operation to perform */
#define WMI_AP_MLME_ASSOC       1   /* associate station */
#define WMI_AP_DISASSOC         2   /* disassociate station */
#define WMI_AP_DEAUTH           3   /* deauthenticate station */
#define WMI_AP_MLME_AUTHORIZE   4   /* authorize station */
#define WMI_AP_MLME_UNAUTHORIZE 5   /* unauthorize station */
} POSTPACK WMI_AP_SET_MLME_CMD;

typedef PREPACK struct {
    A_UINT32 period;
} POSTPACK WMI_AP_CONN_INACT_CMD;

typedef PREPACK struct {
    A_UINT32 period_min;
    A_UINT32 dwell_ms;
} POSTPACK WMI_AP_PROT_SCAN_TIME_CMD;

typedef PREPACK struct {
    A_BOOL flag;
    A_UINT16 aid;
} POSTPACK WMI_AP_SET_PVB_CMD;

#define WMI_DISABLE_REGULATORY_CODE "FF"

typedef PREPACK struct {
    A_UCHAR countryCode[3];
} POSTPACK WMI_SET_COUNTRY_CMD;

typedef PREPACK struct {
    A_UINT8 dtim;
} POSTPACK WMI_AP_SET_DTIM_CMD;

/* AP mode events */
/* WMI_PS_POLL_EVENT */
typedef PREPACK struct {
    A_UINT16 aid;
} POSTPACK WMI_PSPOLL_EVENT;

/*
 * End of AP mode definitions
 */

#ifndef ATH_TARGET
#include "athendpack.h"
#endif

#ifdef __cplusplus
}
#endif

#endif /* _WMI_H_ */
