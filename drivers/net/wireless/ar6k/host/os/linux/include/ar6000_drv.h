/*------------------------------------------------------------------------------ */
/* <copyright file="ar6000_drv.h" company="Atheros"> */
/*    Copyright (c) 2004-2009 Atheros Corporation.  All rights reserved. */
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

#ifndef _AR6000_H_
#define _AR6000_H_

#include <linux/version.h>


#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,17)
#include <linux/config.h>
#else
#include <linux/autoconf.h>
#endif
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/skbuff.h>
#include <linux/if_ether.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <net/iw_handler.h>
#include <linux/if_arp.h>
#include <linux/ip.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,26)
#include <asm/semaphore.h>
#else
#include <linux/semaphore.h>
#endif
#include <linux/wireless.h>
#include <linux/module.h>
#include <asm/io.h>

#include <a_config.h>
#include <athdefs.h>
#include "a_types.h"
#include "a_osapi.h"
#include "htc_api.h"
#include "wmi.h"
#include "a_drv.h"
#include "bmi.h"
#include <ieee80211.h>
#include <ieee80211_ioctl.h>
#include <wlan_api.h>
#include <wmi_api.h>
#include "gpio_api.h"
#include "gpio.h"
#include <host_version.h>
#include <linux/rtnetlink.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
#include <asm/uaccess.h>
#else
#include <linux/init.h>
#include <linux/moduleparam.h>
#endif
#include "AR6002/hw/mbox_host_reg.h"
#include "AR6002/hw/mbox_reg.h"
#include "AR6002/hw/rtc_reg.h"
#include "ar6000_api.h"
#ifdef CONFIG_HOST_TCMD_SUPPORT
#include <testcmd.h>
#endif

#include "targaddrs.h"
#include "dbglog_api.h"
#include "ar6000_diag.h"
#include "common_drv.h"
#include "roaming.h"

#ifndef  __dev_put
#define  __dev_put(dev) dev_put(dev)
#endif


#ifdef USER_KEYS

#define USER_SAVEDKEYS_STAT_INIT     0
#define USER_SAVEDKEYS_STAT_RUN      1

/* TODO this needs to move into the AR_SOFTC struct */
struct USER_SAVEDKEYS {
    struct ieee80211req_key   ucast_ik;
    struct ieee80211req_key   bcast_ik;
    CRYPTO_TYPE               keyType;
    A_BOOL                    keyOk;
};
#endif

#define DBG_INFO        0x00000001
#define DBG_ERROR       0x00000002
#define DBG_WARNING     0x00000004
#define DBG_SDIO        0x00000008
#define DBG_HIF         0x00000010
#define DBG_HTC         0x00000020
#define DBG_WMI         0x00000040
#define DBG_WMI2        0x00000080
#define DBG_DRIVER      0x00000100

#define DBG_DEFAULTS    (DBG_ERROR|DBG_WARNING)


#ifdef DEBUG
#define AR_DEBUG_PRINTF(args...)        if (debugdriver) A_PRINTF(args);
#define AR_DEBUG2_PRINTF(args...)        if (debugdriver >= 2) A_PRINTF(args);
extern int debugdriver;
#else
#define AR_DEBUG_PRINTF(args...)
#define AR_DEBUG2_PRINTF(args...)
#endif

A_STATUS ar6000_ReadRegDiag(HIF_DEVICE *hifDevice, A_UINT32 *address, A_UINT32 *data);
A_STATUS ar6000_WriteRegDiag(HIF_DEVICE *hifDevice, A_UINT32 *address, A_UINT32 *data);

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_AR6000                        1
#define AR6000_MAX_RX_BUFFERS             16
#define AR6000_BUFFER_SIZE                1664
#define AR6000_TX_TIMEOUT                 10
#define AR6000_ETH_ADDR_LEN               6
#define AR6000_MAX_ENDPOINTS              4
#define MAX_NODE_NUM                      15
#define MAX_COOKIE_NUM                    150
#define AR6000_HB_CHALLENGE_RESP_FREQ_DEFAULT        1
#define AR6000_HB_CHALLENGE_RESP_MISS_THRES_DEFAULT  1
#define A_DISCONNECT_TIMER_INTERVAL       5 * 1000

enum {
    DRV_HB_CHALLENGE = 0,
    APP_HB_CHALLENGE
};

/* HTC RAW streams */
typedef enum _HTC_RAW_STREAM_ID {
    HTC_RAW_STREAM_NOT_MAPPED = -1,
    HTC_RAW_STREAM_0 = 0,
    HTC_RAW_STREAM_1 = 1,
    HTC_RAW_STREAM_2 = 2,
    HTC_RAW_STREAM_3 = 3,
    HTC_RAW_STREAM_NUM_MAX
} HTC_RAW_STREAM_ID;

#define RAW_HTC_READ_BUFFERS_NUM    4
#define RAW_HTC_WRITE_BUFFERS_NUM   4

#define HTC_RAW_BUFFER_SIZE  1664

typedef struct {
    int currPtr;
    int length;
    unsigned char data[HTC_RAW_BUFFER_SIZE];
    HTC_PACKET    HTCPacket;
} raw_htc_buffer;

#ifdef CONFIG_HOST_TCMD_SUPPORT
/*
 *  add TCMD_MODE besides wmi and bypasswmi
 *  in TCMD_MODE, only few TCMD releated wmi commands
 *  counld be hanlder
 */
enum {
    AR6000_WMI_MODE = 0,
    AR6000_BYPASS_MODE,
    AR6000_TCMD_MODE,
    AR6000_WLAN_MODE
};
#endif /* CONFIG_HOST_TCMD_SUPPORT */

struct ar_wep_key {
    A_UINT8                 arKeyIndex;
    A_UINT8                 arKeyLen;
    A_UINT8                 arKey[64];
} ;

struct ar_node_mapping {
    A_UINT8                 macAddress[6];
    A_UINT8                 epId;
    A_UINT8                 txPending;
};

struct ar_cookie {
    A_UINT32               arc_bp[2];    /* Must be first field */
    HTC_PACKET             HtcPkt;       /* HTC packet wrapper */
    struct ar_cookie *arc_list_next;
};

struct ar_hb_chlng_resp {
    A_TIMER                 timer;
    A_UINT32                frequency;
    A_UINT32                seqNum;
    A_BOOL                  outstanding;
    A_UINT8                 missCnt;
    A_UINT8                 missThres;
};

/* Per STA data, used in AP mode */
/*TODO: All this should move to OS independent dir */

#define STA_PWR_MGMT_MASK 0x1
#define STA_PWR_MGMT_SHIFT 0x0
#define STA_PWR_MGMT_AWAKE 0x0
#define STA_PWR_MGMT_SLEEP 0x1

#define STA_SET_PWR_SLEEP(sta) (sta->flags |= (STA_PWR_MGMT_MASK << STA_PWR_MGMT_SHIFT))
#define STA_CLR_PWR_SLEEP(sta) (sta->flags &= ~(STA_PWR_MGMT_MASK << STA_PWR_MGMT_SHIFT))
#define STA_IS_PWR_SLEEP(sta) ((sta->flags >> STA_PWR_MGMT_SHIFT) & STA_PWR_MGMT_MASK)

#define STA_PS_POLLED_MASK 0x1
#define STA_PS_POLLED_SHIFT 0x1
#define STA_SET_PS_POLLED(sta) (sta->flags |= (STA_PS_POLLED_MASK << STA_PS_POLLED_SHIFT))
#define STA_CLR_PS_POLLED(sta) (sta->flags &= ~(STA_PS_POLLED_MASK << STA_PS_POLLED_SHIFT))
#define STA_IS_PS_POLLED(sta) (sta->flags & (STA_PS_POLLED_MASK << STA_PS_POLLED_SHIFT))

typedef struct {
    A_UINT16                flags;
    A_UINT8                 mac[ATH_MAC_LEN];
    A_UINT8                 aid;
    A_UINT8                 wpa_ie[IEEE80211_MAX_IE];
    A_NETBUF_QUEUE_T        psq;    /* power save q */
    A_MUTEX_T               psqLock;
} sta_t;

typedef struct ar6_raw_htc {
    HTC_ENDPOINT_ID         arRaw2EpMapping[HTC_RAW_STREAM_NUM_MAX];
    HTC_RAW_STREAM_ID       arEp2RawMapping[ENDPOINT_MAX];
    struct semaphore        raw_htc_read_sem[HTC_RAW_STREAM_NUM_MAX];
    struct semaphore        raw_htc_write_sem[HTC_RAW_STREAM_NUM_MAX];
    wait_queue_head_t       raw_htc_read_queue[HTC_RAW_STREAM_NUM_MAX];
    wait_queue_head_t       raw_htc_write_queue[HTC_RAW_STREAM_NUM_MAX];
    raw_htc_buffer          raw_htc_read_buffer[HTC_RAW_STREAM_NUM_MAX][RAW_HTC_READ_BUFFERS_NUM];
    raw_htc_buffer          raw_htc_write_buffer[HTC_RAW_STREAM_NUM_MAX][RAW_HTC_WRITE_BUFFERS_NUM];
    A_BOOL                  write_buffer_available[HTC_RAW_STREAM_NUM_MAX];
    A_BOOL                  read_buffer_available[HTC_RAW_STREAM_NUM_MAX];
} AR_RAW_HTC_T;

typedef struct ar6_softc {
    struct net_device       *arNetDev;    /* net_device pointer */
    void                    *arWmi;
    int                     arTxPending[ENDPOINT_MAX];
    int                     arTotalTxDataPending;
    A_UINT8                 arNumDataEndPts;
    A_BOOL                  arWmiEnabled;
    A_BOOL                  arWmiReady;
    A_BOOL                  arConnected;
    HTC_HANDLE              arHtcTarget;
    void                    *arHifDevice;
    spinlock_t              arLock;
    struct semaphore        arSem;
    int                     arRxBuffers[ENDPOINT_MAX];
    int                     arSsidLen;
    u_char                  arSsid[32];
    A_UINT8                 arNextMode;
    A_UINT8                 arNetworkType;
    A_UINT8                 arDot11AuthMode;
    A_UINT8                 arAuthMode;
    A_UINT8                 arPrevCrypto;
    A_UINT8                 arPairwiseCrypto;
    A_UINT8                 arPairwiseCryptoLen;
    A_UINT8                 arGroupCrypto;
    A_UINT8                 arGroupCryptoLen;
    A_UINT8                 arDefTxKeyIndex;
    struct ar_wep_key       arWepKeyList[WMI_MAX_KEY_INDEX + 1];
    A_UINT8                 arBssid[6];
    A_UINT8                 arReqBssid[6];
    A_UINT16                arChannelHint;
    A_UINT16                arBssChannel;
    A_UINT16                arListenInterval;
    struct ar6000_version   arVersion;
    A_UINT32                arTargetType;
    A_INT16                 arRssi;
    A_UINT8                 arTxPwr;
    A_BOOL                  arTxPwrSet;
    A_INT32                 arBitRate;
    struct net_device_stats arNetStats;
    struct iw_statistics    arIwStats;
    A_INT8                  arNumChannels;
    A_UINT16                arChannelList[32];
    A_UINT32                arRegCode;
    A_BOOL                  statsUpdatePending;
    TARGET_STATS            arTargetStats;
    A_INT8                  arMaxRetries;
    A_UINT8                 arPhyCapability;
#ifdef CONFIG_HOST_TCMD_SUPPORT
    A_UINT8                 tcmdRxReport;
    A_UINT32                tcmdRxTotalPkt;
    A_INT32                 tcmdRxRssi;
    A_UINT32                tcmdPm;
   A_UINT32                 arTargetMode;
    A_UINT32                tcmdRxcrcErrPkt;
    A_UINT32                tcmdRxsecErrPkt;
    A_UINT32                tcmdRxNoiseFloor;
#endif
    AR6000_WLAN_STATE       arWlanState;
    struct ar_node_mapping  arNodeMap[MAX_NODE_NUM];
    A_UINT8                 arIbssPsEnable;
    A_UINT8                 arNodeNum;
    A_UINT8                 arNexEpId;
    struct ar_cookie        *arCookieList;
    A_UINT16                arRateMask;
    A_UINT8                 arSkipScan;
    A_UINT16                arBeaconInterval;
    A_BOOL                  arConnectPending;
    A_BOOL                  arWmmEnabled;
    struct ar_hb_chlng_resp arHBChallengeResp;
    A_UINT8                 arKeepaliveConfigured;
    A_UINT32                arMgmtFilter;
    HTC_ENDPOINT_ID         arAc2EpMapping[WMM_NUM_AC];
    A_BOOL                  arAcStreamActive[WMM_NUM_AC];
    A_UINT8                 arAcStreamPriMap[WMM_NUM_AC];
    A_UINT8                 arHiAcStreamActivePri;
    A_UINT8                 arEp2AcMapping[ENDPOINT_MAX];
    HTC_ENDPOINT_ID         arControlEp;
#ifdef HTC_RAW_INTERFACE
    AR_RAW_HTC_T            *arRawHtc;
#endif
    A_BOOL                  arNetQueueStopped;
    A_BOOL                  arRawIfInit;
    int                     arDeviceIndex;
    COMMON_CREDIT_STATE_INFO arCreditStateInfo;
    A_BOOL                  arWMIControlEpFull;
    A_BOOL                  dbgLogFetchInProgress;
    A_UCHAR                 log_buffer[DBGLOG_HOST_LOG_BUFFER_SIZE];
    A_UINT32                log_cnt;
    A_UINT32                dbglog_init_done;
    A_UINT32                arConnectCtrlFlags;
#ifdef USER_KEYS
    A_INT32                 user_savedkeys_stat;
    A_UINT32                user_key_ctrl;
    struct USER_SAVEDKEYS   user_saved_keys;
#endif
    A_UINT32                scan_triggered;
    USER_RSSI_THOLD rssi_map[12];
    A_UINT16                ap_profile_flag;    /* AP mode */
    WMI_AP_ACL              g_acl;              /* AP mode */
    sta_t                   sta_list[AP_MAX_NUM_STA]; /* AP mode */
    A_UINT8                 sta_list_index;     /* AP mode */
    struct ieee80211req_key ap_mode_bkey;           /* AP mode */
    A_NETBUF_QUEUE_T        mcastpsq;    /* power save q for Mcast frames */
    A_MUTEX_T               mcastpsqLock;
    A_BOOL                  DTIMExpired; /* flag to indicate DTIM expired */
    A_UINT8                 intra_bss;   /* enable/disable intra bss data forward */
    A_BOOL                  bIsDestroyProgress; /* flag to indicate ar6k destroy is in progress */
    A_TIMER                 disconnect_timer;
    A_UINT8                 ap_hidden_ssid;
    A_UINT8                 country_code[3];
    A_UINT8                 ap_wmode;
    A_UINT8                 ap_dtim_period;
    A_UINT16                ap_beacon_interval;
    A_UINT16                arRTS;
#if CONFIG_PM
    A_UINT16                arOsPowerCtrl;
    A_UINT16                arWowState;
    struct notifier_block   notify_pm;
#endif
    WMI_SCAN_PARAMS_CMD scParams;
} AR_SOFTC_T;

#define ATH_DHCP_PKT_SIZE             342
#define ATH_DHCP_OPCODE_MSG_TYPE      53
#define ATH_DHCP_MSG_TYPE_LEN         1

#define ATH_DHCP_DISCOVER             1
#define ATH_DHCP_REQUEST              3
#define ATH_DHCP_ACK                  5

#define ATH_DHCP_INVALID_MSG          99
#define A_DHCP_TIMER_INTERVAL       10 * 1000

typedef PREPACK struct ether2_hdr {
    A_UINT8     destMAC[ATH_MAC_LEN];
    A_UINT8     srcMAC[ATH_MAC_LEN];
    A_UINT16    etherType;
} POSTPACK ETHERII_HDR;

typedef PREPACK struct ip_hdr {
    A_UINT8     version_HdrLength;     /* [Version (4 bits)][Header Length (4 bits)] */
    A_UINT8     TOS;
    A_UINT16    totalLength;
    A_UINT16    identification;
    A_UINT16    flags_FragmentOffset;  /* [Flags (3bits)][Fragment Offset 13 bits] */
    A_UINT8     TTL;
    A_UINT8     protocol;
    A_UINT16    headerCheckSum;
    A_UINT32    sourceIP;
    A_UINT32    destIP;
} POSTPACK IP_HDR;

typedef PREPACK struct udp_hdr{
    A_UINT16 sourcePort;
    A_UINT16 destPort;
    A_UINT16 length;
    A_UINT16 checkSum;
} POSTPACK UDP_HDR;

typedef PREPACK struct dhcp_msg{

    A_UINT8     opCode;
    A_UINT8     hwAddressType;
    A_UINT8     hwAddressLength;
    A_UINT8     hops;
    A_UINT32    transactionID;
    A_UINT16    seconds;
    A_UINT16    flags;
    A_UINT32    clientIPAddress;       
    A_UINT32    yourIPAddress;        
    A_UINT32    serverIPAddress;       
    A_UINT32    relayIPAddress;        
    A_UINT16    clientHwAddress[8];     
    A_UINT8     bootP_Data[192];
    A_UINT8     magicCookie[4];
} POSTPACK DHCP_MSG;

typedef PREPACK struct dhcp_packet{
    ETHERII_HDR  ether2Hdr;
    IP_HDR       ipHdr;
    UDP_HDR      udpHdr;
    DHCP_MSG     dhcpMsg;
    A_UINT8      dhcpOptions[256];
} POSTPACK DHCP_PACKET;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
/* Looks like we need this for 2.4 kernels */
static inline void *netdev_priv(struct net_device *dev)
{
    return(dev->priv);
}
#endif

#define arAc2EndpointID(ar,ac)          (ar)->arAc2EpMapping[(ac)]
#define arSetAc2EndpointIDMap(ar,ac,ep)  \
{  (ar)->arAc2EpMapping[(ac)] = (ep); \
   (ar)->arEp2AcMapping[(ep)] = (ac); }
#define arEndpoint2Ac(ar,ep)           (ar)->arEp2AcMapping[(ep)]

#define arRawIfEnabled(ar) (ar)->arRawIfInit
#define arRawStream2EndpointID(ar,raw)          (ar)->arRawHtc->arRaw2EpMapping[(raw)]
#define arSetRawStream2EndpointIDMap(ar,raw,ep)  \
{  (ar)->arRawHtc->arRaw2EpMapping[(raw)] = (ep); \
   (ar)->arRawHtc->arEp2RawMapping[(ep)] = (raw); }
#define arEndpoint2RawStreamID(ar,ep)           (ar)->arRawHtc->arEp2RawMapping[(ep)]

struct ar_giwscan_param {
    char    *current_ev;
    char    *end_buf;
    A_UINT32 bytes_needed;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
    struct iw_request_info *info;
#endif
};

#define AR6000_STAT_INC(ar, stat)       (ar->arNetStats.stat++)

#define AR6000_SPIN_LOCK(lock, param)   do {                            \
    if (irqs_disabled()) {                                              \
        AR_DEBUG_PRINTF("IRQs disabled:AR6000_LOCK\n");                 \
    }                                                                   \
    spin_lock_bh(lock);                                                 \
} while (0)

#define AR6000_SPIN_UNLOCK(lock, param) do {                            \
    if (irqs_disabled()) {                                              \
        AR_DEBUG_PRINTF("IRQs disabled: AR6000_UNLOCK\n");              \
    }                                                                   \
    spin_unlock_bh(lock);                                               \
} while (0)

int ar6000_ioctl(struct net_device *dev, struct ifreq *rq, int cmd);
int ar6000_ioctl_dispatcher(struct net_device *dev, struct ifreq *rq, int cmd);
void ar6000_gpio_init(void);
void ar6000_init_profile_info(AR_SOFTC_T *ar);
void ar6000_install_static_wep_keys(AR_SOFTC_T *ar);
int ar6000_init(struct net_device *dev);
int ar6000_dbglog_get_debug_logs(AR_SOFTC_T *ar);
void ar6000_TxDataCleanup(AR_SOFTC_T *ar);

#ifdef HTC_RAW_INTERFACE

#ifndef __user
#define __user
#endif

int ar6000_htc_raw_open(AR_SOFTC_T *ar);
int ar6000_htc_raw_close(AR_SOFTC_T *ar);
ssize_t ar6000_htc_raw_read(AR_SOFTC_T *ar,
                            HTC_RAW_STREAM_ID StreamID,
                            char __user *buffer, size_t count);
ssize_t ar6000_htc_raw_write(AR_SOFTC_T *ar,
                             HTC_RAW_STREAM_ID StreamID,
                             char __user *buffer, size_t count);

#endif /* HTC_RAW_INTERFACE */

/* AP mode */
/*TODO: These routines should be moved to a file that is common across OS */
sta_t *
ieee80211_find_conn(AR_SOFTC_T *ar, A_UINT8 *node_addr);

sta_t *
ieee80211_find_conn_for_aid(AR_SOFTC_T *ar, A_UINT8 aid);

A_UINT8
remove_sta(AR_SOFTC_T *ar, A_UINT8 *mac, A_UINT16 reason);

#ifdef __cplusplus
}
#endif

#endif /* _AR6000_H_ */
