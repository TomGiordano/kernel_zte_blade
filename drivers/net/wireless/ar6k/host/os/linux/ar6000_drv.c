/*
 *
 * Copyright (c) 2004-2009 Atheros Communications Inc.
 * All rights reserved.
 *
 * 
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// Software distributed under the License is distributed on an "AS
// IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
// implied. See the License for the specific language governing
// rights and limitations under the License.
//
//
 *
 */

/*
 * This driver is a pseudo ethernet driver to access the Atheros AR6000
 * WLAN Device
 */
/* history
 *     when         who         what                                 tag 
 *   2010-01-18     hp       update arConnected flags when sending ZTE_WIFI_HP_012
 *   		              wmi_disconnect_cmd 
 *   2010-01-11     hp     decrease wifi netlink event report      ZTE_WIFI_HP_008
 *   2010-01-11     hp      delete remaining timer when            ZTE_WIFI_HP_010
 *   				 ar6000_destroy
 *   2010-01-11     hp      completely disable polling for wifi    ZTE_WIFI_HP_009
 *   			     except for module_init and module_cleanup
 *   2009-12-29     hp     add wifi scan complete report           ZTE_WIFI_HP_006
 *   				to speed up wifi scan
 *   2009-12-28     hp    safely remove wifi from sdio2            ZTE_WIFI_HP_005
 *                         when close wifi and fix wifi wlan state
 *   2009-12-21     hp        change polling for cmd7              ZTE_WIFI_HP_002 
 */
static const char athId[] __attribute__ ((unused)) = "$Id: //depot/sw/releases/olca2.2/host/os/linux/ar6000_drv.c#36 $";

#define SOFTMAC_USED

/* ATHENV */
#ifdef ANDROID_ENV
#include <linux/fs.h>
#include <linux/mmc/sdio_func.h>
#include <linux/wakelock.h>
#ifdef CONFIG_PM
#include <linux/platform_device.h>
#include <linux/inetdevice.h>
enum {
	WLAN_PWR_CTRL_CUT_PWR = 1,
	WLAN_PWR_CTRL_DEEP_SLEEP,
	WLAN_PWR_CTRL_WOW
};
#if 0 /* pseudo code */
extern void msmsdcc_disable_wlan_slot(void);
extern void msmsdcc_enable_wlan_slot(void);
extern unsigned int msmsdcc_wlan_pwr_ctrl;
static unsigned int host_asleep;
#endif
#endif /* CONFIG_PM */
#endif /* ANDROID_ENV */
/* ATHENV */

#include "ar6000_drv.h"
#include "htc.h"
#include "engine.h"
#include "wmi_filter_linux.h"

/*#include "/home/huangpei/proj/svn/7x27-4735/boot/kernel/arch/arm/mach-msm/include/mach/vreg.h"*/
#include <mach/vreg.h>

#define IS_MAC_NULL(mac) (mac[0]==0 && mac[1]==0 && mac[2]==0 && mac[3]==0 && mac[4]==0 && mac[5]==0)
#define IS_MAC_BCAST(mac) (*mac==0xff)

MODULE_LICENSE("GPL and additional rights");

#ifndef REORG_APTC_HEURISTICS
#undef ADAPTIVE_POWER_THROUGHPUT_CONTROL
#endif /* REORG_APTC_HEURISTICS */

#define MMC_POLLING_FILE  "/sys/devices/platform/msm_sdcc.2/polling" 
#ifdef ADAPTIVE_POWER_THROUGHPUT_CONTROL
#define APTC_TRAFFIC_SAMPLING_INTERVAL     100  /* msec */
#define APTC_UPPER_THROUGHPUT_THRESHOLD    3000 /* Kbps */
#define APTC_LOWER_THROUGHPUT_THRESHOLD    2000 /* Kbps */

typedef struct aptc_traffic_record {
    A_BOOL timerScheduled;
    struct timeval samplingTS;
    unsigned long bytesReceived;
    unsigned long bytesTransmitted;
} APTC_TRAFFIC_RECORD;

A_TIMER aptcTimer;
APTC_TRAFFIC_RECORD aptcTR;
#endif /* ADAPTIVE_POWER_THROUGHPUT_CONTROL */

unsigned int processDot11Hdr = 0;

/* ATHENV */
#ifdef ANDROID_ENV

/****************************************
 * variables for only Android case
 ****************************************/
int bmienable = 1;
int work_mode = 0;
int dev_removed = 0;
int chan_num = 0;
const char *def_ifname = "wlan0";
struct wake_lock ar6k_init_wake_lock;
static struct net_device *pending_devs[MAX_AR6000];

char *fm_path = NULL;
char *tgt_fw = "/system/wifi/athwlan.bin.z77";
char *tgt_patch = "/system/wifi/data.patch.hw2_0.bin";
char *tcmd_fw = "/system/wifi/athtcmd_ram.bin";
char *art_fw = "/system/wifi/device.bin";
char *eeprom_bin = "/system/wifi/eeprom.bin";
char *eeprom_data = "/system/wifi/eeprom.data";


#ifdef REGION_CODE_FILE_USED
char *reg_file = "/system/wifi/reg_code";
#else
char *reg_file = NULL;
#endif

#ifdef SOFTMAC_USED
char *softmac_file = "/system/wifi/softmac";
#else
char *softmac_file = NULL;
#endif

#ifdef EEPROM_FILE_USED
char *eeprom_file = "/system/wifi/calData_ar6102_15dBm.bin";
//char *eeprom_file = "/system/wifi/fakeBoardData_AR6002.bin";
#else
char *eeprom_file = NULL; 
#endif

int refClock = 26000000;
//int refClock =   19200000;
int regCode = 0x0;
//int regCode = 0x4067;

#else /* ! ANDROID_ENV */
int bmienable = 0;
#endif /* ANDROID_ENV */
/* ATHENV */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
char ifname[IFNAMSIZ] = {0,};
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0) */

int fwloadenable = 0;
unsigned int bypasswmi = 0;
unsigned int debuglevel = 0;
int tspecCompliance = ATHEROS_COMPLIANCE;
unsigned int busspeedlow = 0;
unsigned int onebitmode = 0;
unsigned int skipflash = 0;
unsigned int wmitimeout = 2;
unsigned int wlanNodeCaching = 1;
unsigned int enableuartprint = 0;
unsigned int logWmiRawMsgs = 0;
unsigned int enabletimerwar = 0;
unsigned int fwmode = 1;
unsigned int mbox_yield_limit = 99;
int reduce_credit_dribble = 1 + HTC_CONNECT_FLAGS_THRESHOLD_LEVEL_ONE_HALF;
int allow_trace_signal = 0;
#ifdef CONFIG_HOST_TCMD_SUPPORT
unsigned int testmode =0;
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
module_param_string(ifname, ifname, sizeof(ifname), 0644);
module_param(fwloadenable, int, 0644);
module_param(bmienable, int, 0644);
module_param(bypasswmi, int, 0644);
module_param(debuglevel, int, 0644);
module_param(tspecCompliance, int, 0644);
module_param(onebitmode, int, 0644);
module_param(busspeedlow, int, 0644);
module_param(skipflash, int, 0644);
module_param(wmitimeout, int, 0644);
module_param(wlanNodeCaching, int, 0644);
module_param(logWmiRawMsgs, int, 0644);
module_param(enableuartprint, int, 0644);
module_param(enabletimerwar, int, 0644);
module_param(fwmode, int, 0644);
module_param(mbox_yield_limit, int, 0644);
module_param(reduce_credit_dribble, int, 0644);
module_param(allow_trace_signal, int, 0644);
module_param(processDot11Hdr, int, 0644);
/* ATHENV */
#ifdef ANDROID_ENV
module_param(work_mode, int, 0644);
module_param(tgt_fw, charp, S_IRUGO);
module_param(tgt_patch, charp, S_IRUGO);
module_param(eeprom_bin, charp, S_IRUGO);
module_param(eeprom_data, charp, S_IRUGO);
module_param(softmac_file, charp, S_IRUGO);
module_param(eeprom_file, charp, S_IRUGO);
module_param(fm_path, charp,S_IRUGO);
module_param(refClock, int, 0644);
module_param(regCode, int, 0644);
module_param(chan_num, int, 0644);
#endif
/* ATHENV */
#ifdef CONFIG_HOST_TCMD_SUPPORT
module_param(testmode, int, 0644);
#endif
#else

#define __user
/* for linux 2.4 and lower */
MODULE_PARM(bmienable,"i");
MODULE_PARM(fwloadenable,"i");
MODULE_PARM(bypasswmi,"i");
MODULE_PARM(debuglevel, "i");
MODULE_PARM(onebitmode,"i");
MODULE_PARM(busspeedlow, "i");
MODULE_PARM(skipflash, "i");
MODULE_PARM(wmitimeout, "i");
MODULE_PARM(wlanNodeCaching, "i");
MODULE_PARM(enableuartprint,"i");
MODULE_PARM(logWmiRawMsgs, "i");
MODULE_PARM(enabletimerwar,"i");
MODULE_PARM(fwmode,"i");
MODULE_PARM(mbox_yield_limit,"i");
MODULE_PARM(reduce_credit_dribble,"i");
MODULE_PARM(allow_trace_signal,"i");
MODULE_PARM(processDot11Hdr,"i");
#ifdef CONFIG_HOST_TCMD_SUPPORT
MODULE_PARM(testmode, "i");
#endif
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,10)
/* in 2.6.10 and later this is now a pointer to a uint */
unsigned int _mboxnum = HTC_MAILBOX_NUM_MAX;
#define mboxnum &_mboxnum
#else
unsigned int mboxnum = HTC_MAILBOX_NUM_MAX;
#endif

#ifdef DEBUG
A_UINT32 g_dbg_flags = DBG_DEFAULTS;
unsigned int debugflags = 0;
int debugdriver = 1;
unsigned int debughtc = 128;
unsigned int debugbmi = 1;
unsigned int debughif = 2;
unsigned int txcreditsavailable[HTC_MAILBOX_NUM_MAX] = {0};
unsigned int txcreditsconsumed[HTC_MAILBOX_NUM_MAX] = {0};
unsigned int txcreditintrenable[HTC_MAILBOX_NUM_MAX] = {0};
unsigned int txcreditintrenableaggregate[HTC_MAILBOX_NUM_MAX] = {0};

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
module_param(debugflags, int, 0644);
module_param(debugdriver, int, 0644);
module_param(debughtc, int, 0644);
module_param(debugbmi, int, 0644);
module_param(debughif, int, 0644);
module_param_array(txcreditsavailable, int, mboxnum, 0644);
module_param_array(txcreditsconsumed, int, mboxnum, 0644);
module_param_array(txcreditintrenable, int, mboxnum, 0644);
module_param_array(txcreditintrenableaggregate, int, mboxnum, 0644);
#else
/* linux 2.4 and lower */
MODULE_PARM(debugflags,"i");
MODULE_PARM(debugdriver, "i");
MODULE_PARM(debughtc, "i");
MODULE_PARM(debugbmi, "i");
MODULE_PARM(debughif, "i");
MODULE_PARM(txcreditsavailable, "0-3i");
MODULE_PARM(txcreditsconsumed, "0-3i");
MODULE_PARM(txcreditintrenable, "0-3i");
MODULE_PARM(txcreditintrenableaggregate, "0-3i");
#endif

#endif /* DEBUG */

/* ATHENV */
#ifdef ANDROID_ENV
unsigned int resetok = 0;
#else
unsigned int resetok = 1;
#endif
/* ATHENV */
unsigned int tx_attempt[HTC_MAILBOX_NUM_MAX] = {0};
unsigned int tx_post[HTC_MAILBOX_NUM_MAX] = {0};
unsigned int tx_complete[HTC_MAILBOX_NUM_MAX] = {0};
unsigned int hifBusRequestNumMax = 40;
unsigned int war23838_disabled = 0;
#ifdef ADAPTIVE_POWER_THROUGHPUT_CONTROL
unsigned int enableAPTCHeuristics = 1;
#endif /* ADAPTIVE_POWER_THROUGHPUT_CONTROL */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
module_param_array(tx_attempt, int, mboxnum, 0644);
module_param_array(tx_post, int, mboxnum, 0644);
module_param_array(tx_complete, int, mboxnum, 0644);
module_param(hifBusRequestNumMax, int, 0644);
module_param(war23838_disabled, int, 0644);
module_param(resetok, int, 0644);
#ifdef ADAPTIVE_POWER_THROUGHPUT_CONTROL
module_param(enableAPTCHeuristics, int, 0644);
#endif /* ADAPTIVE_POWER_THROUGHPUT_CONTROL */
#else
MODULE_PARM(tx_attempt, "0-3i");
MODULE_PARM(tx_post, "0-3i");
MODULE_PARM(tx_complete, "0-3i");
MODULE_PARM(hifBusRequestNumMax, "i");
MODULE_PARM(war23838_disabled, "i");
MODULE_PARM(resetok, "i");
#ifdef ADAPTIVE_POWER_THROUGHPUT_CONTROL
MODULE_PARM(enableAPTCHeuristics, "i");
#endif /* ADAPTIVE_POWER_THROUGHPUT_CONTROL */
#endif

#ifdef BLOCK_TX_PATH_FLAG
int blocktx = 0;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
module_param(blocktx, int, 0644);
#else
MODULE_PARM(blocktx, "i");
#endif
#endif /* BLOCK_TX_PATH_FLAG */


int reconnect_flag = 0;

char tgt_fw_path[100];
char tgt_patch_path[100];
char eeprom_bin_path[100];
char eeprom_data_path[100];

static void ar6000_enable_mmc_polling(int enable);  
/* Function declarations */
static int ar6000_init_module(void);
static void ar6000_cleanup_module(void);

int ar6000_init(struct net_device *dev);
static int ar6000_open(struct net_device *dev);
static int ar6000_close(struct net_device *dev);
static void ar6000_init_control_info(AR_SOFTC_T *ar);
static int ar6000_data_tx(struct sk_buff *skb, struct net_device *dev);

static void ar6000_destroy(struct net_device *dev, unsigned int unregister);
static void ar6000_detect_error(unsigned long ptr);
static struct net_device_stats *ar6000_get_stats(struct net_device *dev);
static struct iw_statistics *ar6000_get_iwstats(struct net_device * dev);

static void disconnect_timer_handler(unsigned long ptr);

/*
 * HTC service connection handlers
 */
static A_STATUS ar6000_avail_ev(void *context, void *hif_handle);

static A_STATUS ar6000_unavail_ev(void *context, void *hif_handle);

static void ar6000_target_failure(void *Instance, A_STATUS Status);

static void ar6000_rx(void *Context, HTC_PACKET *pPacket);

static void ar6000_rx_refill(void *Context,HTC_ENDPOINT_ID Endpoint);

static void ar6000_tx_complete(void *Context, HTC_PACKET *pPacket);

static HTC_SEND_FULL_ACTION ar6000_tx_queue_full(void *Context, HTC_PACKET *pPacket);

static void deliver_frames_to_nw_stack(struct sk_buff *skb);

/*
 * Static variables
 */

static struct net_device *ar6000_devices[MAX_AR6000];
extern struct iw_handler_def ath_iw_handler_def;
DECLARE_WAIT_QUEUE_HEAD(arEvent);
DECLARE_WAIT_QUEUE_HEAD(ar6000_scan_queue);
static void ar6000_cookie_init(AR_SOFTC_T *ar);
static void ar6000_cookie_cleanup(AR_SOFTC_T *ar);
static void ar6000_free_cookie(AR_SOFTC_T *ar, struct ar_cookie * cookie);
static struct ar_cookie *ar6000_alloc_cookie(AR_SOFTC_T *ar);

#ifdef USER_KEYS
static A_STATUS ar6000_reinstall_keys(AR_SOFTC_T *ar,A_UINT8 key_op_ctrl);
#endif


static struct ar_cookie s_ar_cookie_mem[MAX_COOKIE_NUM];

#define HOST_INTEREST_ITEM_ADDRESS(ar, item) \
        (((ar)->arTargetType == TARGET_TYPE_AR6001) ? AR6001_HOST_INTEREST_ITEM_ADDRESS(item) : \
        (((ar)->arTargetType == TARGET_TYPE_AR6002) ? AR6002_HOST_INTEREST_ITEM_ADDRESS(item) : \
        (((ar)->arTargetType == TARGET_TYPE_AR6003) ? AR6003_HOST_INTEREST_ITEM_ADDRESS(item) : 0)))


#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
static struct net_device_ops ar6000_netdev_ops = {
    .ndo_init               = NULL,
    .ndo_open               = ar6000_open,
    .ndo_stop               = ar6000_close,
    .ndo_get_stats          = ar6000_get_stats,
    .ndo_do_ioctl           = ar6000_ioctl,
    .ndo_start_xmit         = ar6000_data_tx,
};
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29) */

/* Debug log support */

/*
 * Flag to govern whether the debug logs should be parsed in the kernel
 * or reported to the application.
 */
#define REPORT_DEBUG_LOGS_TO_APP

A_STATUS
ar6000_set_host_app_area(AR_SOFTC_T *ar)
{
    A_UINT32 address, data;
    struct host_app_area_s host_app_area;

    /* Fetch the address of the host_app_area_s instance in the host interest area */
    address = TARG_VTOP(ar->arTargetType, HOST_INTEREST_ITEM_ADDRESS(ar, hi_app_host_interest));
    if (ar6000_ReadRegDiag(ar->arHifDevice, &address, &data) != A_OK) {
        return A_ERROR;
    }
    address = TARG_VTOP(ar->arTargetType, data);
    host_app_area.wmi_protocol_ver = WMI_PROTOCOL_VERSION;
    if (ar6000_WriteDataDiag(ar->arHifDevice, address,
                             (A_UCHAR *)&host_app_area,
                             sizeof(struct host_app_area_s)) != A_OK)
    {
        return A_ERROR;
    }

    return A_OK;
}

A_UINT32
dbglog_get_debug_hdr_ptr(AR_SOFTC_T *ar)
{
    A_UINT32 param;
    A_UINT32 address;
    A_STATUS status;

    address = TARG_VTOP(ar->arTargetType, HOST_INTEREST_ITEM_ADDRESS(ar, hi_dbglog_hdr));
    if ((status = ar6000_ReadDataDiag(ar->arHifDevice, address,
                                      (A_UCHAR *)&param, 4)) != A_OK)
    {
        param = 0;
    }

    return param;
}

/*
 * The dbglog module has been initialized. Its ok to access the relevant
 * data stuctures over the diagnostic window.
 */
void
ar6000_dbglog_init_done(AR_SOFTC_T *ar)
{
    ar->dbglog_init_done = TRUE;
}

A_UINT32
dbglog_get_debug_fragment(A_INT8 *datap, A_UINT32 len, A_UINT32 limit)
{
    A_INT32 *buffer;
    A_UINT32 count;
    A_UINT32 numargs;
    A_UINT32 length;
    A_UINT32 fraglen;

    count = fraglen = 0;
    buffer = (A_INT32 *)datap;
    length = (limit >> 2);

    if (len <= limit) {
        fraglen = len;
    } else {
        while (count < length) {
            numargs = DBGLOG_GET_NUMARGS(buffer[count]);
            fraglen = (count << 2);
            count += numargs + 1;
        }
    }

    return fraglen;
}

void
dbglog_parse_debug_logs(A_INT8 *datap, A_UINT32 len)
{
    A_INT32 *buffer;
    A_UINT32 count;
    A_UINT32 timestamp;
    A_UINT32 debugid;
    A_UINT32 moduleid;
    A_UINT32 numargs;
    A_UINT32 length;

    count = 0;
    buffer = (A_INT32 *)datap;
    length = (len >> 2);
    while (count < length) {
        debugid = DBGLOG_GET_DBGID(buffer[count]);
        moduleid = DBGLOG_GET_MODULEID(buffer[count]);
        numargs = DBGLOG_GET_NUMARGS(buffer[count]);
        timestamp = DBGLOG_GET_TIMESTAMP(buffer[count]);
        switch (numargs) {
            case 0:
            AR_DEBUG_PRINTF("%d %d (%d)\n", moduleid, debugid, timestamp);
            break;

            case 1:
            AR_DEBUG_PRINTF("%d %d (%d): 0x%x\n", moduleid, debugid,
                            timestamp, buffer[count+1]);
            break;

            case 2:
            AR_DEBUG_PRINTF("%d %d (%d): 0x%x, 0x%x\n", moduleid, debugid,
                            timestamp, buffer[count+1], buffer[count+2]);
            break;

            default:
            AR_DEBUG_PRINTF("Invalid args: %d\n", numargs);
        }
        count += numargs + 1;
    }
}

int
ar6000_dbglog_get_debug_logs(AR_SOFTC_T *ar)
{
    struct dbglog_hdr_s debug_hdr;
    struct dbglog_buf_s debug_buf;
    A_UINT32 address;
    A_UINT32 length;
    A_UINT32 dropped;
    A_UINT32 firstbuf;
    A_UINT32 debug_hdr_ptr;

    if (!ar->dbglog_init_done) return A_ERROR;


    AR6000_SPIN_LOCK(&ar->arLock, 0);

    if (ar->dbgLogFetchInProgress) {
        AR6000_SPIN_UNLOCK(&ar->arLock, 0);
        return A_EBUSY;
    }

        /* block out others */
    ar->dbgLogFetchInProgress = TRUE;

    AR6000_SPIN_UNLOCK(&ar->arLock, 0);

    debug_hdr_ptr = dbglog_get_debug_hdr_ptr(ar);
    printk("debug_hdr_ptr: 0x%x\n", debug_hdr_ptr);

    /* Get the contents of the ring buffer */
    if (debug_hdr_ptr) {
        address = TARG_VTOP(ar->arTargetType, debug_hdr_ptr);
        length = sizeof(struct dbglog_hdr_s);
        ar6000_ReadDataDiag(ar->arHifDevice, address,
                            (A_UCHAR *)&debug_hdr, length);
        address = TARG_VTOP(ar->arTargetType, (A_UINT32)debug_hdr.dbuf);
        firstbuf = address;
        dropped = debug_hdr.dropped;
        length = sizeof(struct dbglog_buf_s);
        ar6000_ReadDataDiag(ar->arHifDevice, address,
                            (A_UCHAR *)&debug_buf, length);

        do {
            address = TARG_VTOP(ar->arTargetType, (A_UINT32)debug_buf.buffer);
            length = debug_buf.length;
            if ((length) && (debug_buf.length <= debug_buf.bufsize)) {
                /* Rewind the index if it is about to overrun the buffer */
                if (ar->log_cnt > (DBGLOG_HOST_LOG_BUFFER_SIZE - length)) {
                    ar->log_cnt = 0;
                }
                if(A_OK != ar6000_ReadDataDiag(ar->arHifDevice, address,
                                    (A_UCHAR *)&ar->log_buffer[ar->log_cnt], length))
                {
                    break;
                }
                ar6000_dbglog_event(ar, dropped, &ar->log_buffer[ar->log_cnt], length);
                ar->log_cnt += length;
            } else {
                AR_DEBUG_PRINTF("Length: %d (Total size: %d)\n",
                                debug_buf.length, debug_buf.bufsize);
            }

            address = TARG_VTOP(ar->arTargetType, (A_UINT32)debug_buf.next);
            length = sizeof(struct dbglog_buf_s);
            if(A_OK != ar6000_ReadDataDiag(ar->arHifDevice, address,
                                (A_UCHAR *)&debug_buf, length))
            {
                break;
            }

        } while (address != firstbuf);
    }

    ar->dbgLogFetchInProgress = FALSE;

    return A_OK;
}

void
ar6000_dbglog_event(AR_SOFTC_T *ar, A_UINT32 dropped,
                    A_INT8 *buffer, A_UINT32 length)
{
#ifdef REPORT_DEBUG_LOGS_TO_APP
    #define MAX_WIRELESS_EVENT_SIZE 252
    /*
     * Break it up into chunks of MAX_WIRELESS_EVENT_SIZE bytes of messages.
     * There seems to be a limitation on the length of message that could be
     * transmitted to the user app via this mechanism.
     */
    A_UINT32 send, sent;

    sent = 0;
    send = dbglog_get_debug_fragment(&buffer[sent], length - sent,
                                     MAX_WIRELESS_EVENT_SIZE);
    while (send) {
        ar6000_send_event_to_app(ar, WMIX_DBGLOG_EVENTID, &buffer[sent], send);
        sent += send;
        send = dbglog_get_debug_fragment(&buffer[sent], length - sent,
                                         MAX_WIRELESS_EVENT_SIZE);
    }
#else
    AR_DEBUG_PRINTF("Dropped logs: 0x%x\nDebug info length: %d\n",
                    dropped, length);

    /* Interpret the debug logs */
    dbglog_parse_debug_logs(buffer, length);
#endif /* REPORT_DEBUG_LOGS_TO_APP */
}

/* ATHENV */
#if defined(ANDROID_ENV) && defined(CONFIG_PM)
#if 0 /* pseudo code */
static void ar6k_send_asleep_event_to_app(AR_SOFTC_T *ar, A_BOOL asleep)
{
	char buf[128];
	union iwreq_data wrqu;

	snprintf(buf, sizeof(buf), "HOST_ASLEEP=%s", asleep ? "asleep" : "awake");
	A_MEMZERO(&wrqu, sizeof(wrqu));
	wrqu.data.length = strlen(buf);
	wireless_send_event(ar->arNetDev, IWEVCUSTOM, &wrqu, buf);
}

static void ar6000_wow_suspend(void)
{
	int i;
	A_BOOL wowMode = TRUE;

#define ANDROID_WOW_LIST_ID 1
	for (i = 0; i < MAX_AR6000; i++) {
		AR_SOFTC_T *ar;

		if (ar6000_devices[i] == NULL) 
			continue;

		ar = (AR_SOFTC_T*)netdev_priv(ar6000_devices[i]);

		if (ar->arNetworkType == AP_NETWORK)
			continue;

		wowMode &= ((ar->arConnected == TRUE) & ar->arWmiReady);

		if (wowMode) {

			/* Setup WoW for unicast & Aarp request for our own IP
			   disable background scan. Set listen interval into 1000 TUs
			   Enable keepliave for 110 seconds
			   */
			struct in_ifaddr **ifap = NULL;
			struct in_ifaddr *ifa = NULL;
			struct in_device *in_dev;
			A_UINT8 macMask[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
			A_STATUS status;
			WMI_ADD_WOW_PATTERN_CMD addWowCmd = { .filter = { 0 } };
			WMI_DEL_WOW_PATTERN_CMD delWowCmd;                              
			WMI_SET_HOST_SLEEP_MODE_CMD hostSleepMode = {FALSE, TRUE};
			WMI_SET_WOW_MODE_CMD wowMode = { .enable_wow = TRUE };

			printk("Setup for WoW mode\n");
			ar6000_TxDataCleanup(ar); /* IMPORTANT, otherwise there will be 11mA after listen interval as 1000*/

#if 0 /* we don't do it if the power consumption is already good enough. */
			if (wmi_listeninterval_cmd(ar->arWmi, 1000, 0) == A_OK) {
				AR6000_SPIN_LOCK(&ar->arLock, 0);
				ar->arListenInterval = 1000;
				AR6000_SPIN_UNLOCK(&ar->arLock, 0);
			}                   
#endif

			wmi_set_keepalive_cmd(ar->arWmi, 110); /* keepalive otherwise, we will be disconnected*/
			status = wmi_scanparams_cmd(ar->arWmi, 0,0,0xffff,0,0,0,0,0,0,0);            
			wmi_set_wow_mode_cmd(ar->arWmi, &wowMode);        

			/* clear up our WoW pattern first */
			delWowCmd.filter_list_id = ANDROID_WOW_LIST_ID;
			delWowCmd.filter_id = 0;
			wmi_del_wow_pattern_cmd(ar->arWmi, &delWowCmd);

			/* setup unicast packet pattern for WoW */
			if (ar->arNetDev->dev_addr[1]) {
				addWowCmd.filter_list_id = ANDROID_WOW_LIST_ID;
				addWowCmd.filter_size = 6; /* MAC address */
				addWowCmd.filter_offset = 2;         
				status = wmi_add_wow_pattern_cmd(ar->arWmi, &addWowCmd, ar->arNetDev->dev_addr, macMask, addWowCmd.filter_size);
			}

			/* setup ARP request for our own IP */
			if ((in_dev = __in_dev_get_rtnl(ar->arNetDev)) != NULL) {
				for (ifap = &in_dev->ifa_list; (ifa = *ifap) != NULL; ifap = &ifa->ifa_next) {
					if (!strcmp(ar->arNetDev->name, ifa->ifa_label)) {
						break; /* found */
					}
				}
			}        
			if (ifa && ifa->ifa_local) {
				WMI_SET_IP_CMD ipCmd;
				memset(&ipCmd, 0, sizeof(ipCmd));
				ipCmd.ips[0] = ifa->ifa_local;
				status = wmi_set_ip_cmd(ar->arWmi, &ipCmd);
			}
			ar6k_send_asleep_event_to_app(ar, TRUE);        
			host_asleep = 1;
			wmi_set_host_sleep_mode_cmd(ar->arWmi, &hostSleepMode);

		} else {
			printk("Not allowed to go to WOW at this moment.\n");
		}
	}
}

static void ar6000_wow_resume(void)
{
	int i;

	for (i = 0; i < MAX_AR6000; i++) {
		AR_SOFTC_T *ar;

		if (ar6000_devices[i] == NULL) 
			continue;

		ar = (AR_SOFTC_T*)netdev_priv(ar6000_devices[i]);

		if (host_asleep) {
			WMI_SET_HOST_SLEEP_MODE_CMD hostSleepMode = {TRUE, FALSE};
			wmi_set_host_sleep_mode_cmd(ar->arWmi, &hostSleepMode);        
			wmi_scanparams_cmd(ar->arWmi, 0,0,60,0,0,0,0,0,0,0);
			wmi_set_keepalive_cmd(ar->arWmi, 0);

			printk("WOW resume\n");
#if 0 /* we don't do it if the power consumption is already good enough. */
			if (wmi_listeninterval_cmd(ar->arWmi, 100, 0) == A_OK) {
				AR6000_SPIN_LOCK(&ar->arLock, 0);
				ar->arListenInterval = 100;
				AR6000_SPIN_UNLOCK(&ar->arLock, 0);
			}   
#endif
			ar6k_send_asleep_event_to_app(ar, FALSE);
			host_asleep = 0;
		} else {
			printk("Something is strange for WOW resume.\n");
		}
	}
}
#endif /* pseudo code */

static void ar6000_pwr_on(void)
{
	/*
	 * sample code of powerup sequence:
	 *

	int ret = 0;

	// for 2.6V
	ret = pm_vreg_set_level(WLAN_2_6V_ID, 2600);
	if (ret) {
		printk( KERN_ERR "ar6000: 2.6V set level failed\n");
	}

	ret = pm_vote_vreg_switch_pc(WLAN_2_6V_ID, 0);
	if (ret) {
		printk( KERN_ERR " SDIO Qualcomm HCD: 2.6V switch pc failed\n");
	}

	// for 1.8V
	ret = pm_vreg_set_level(WLAN_1_8V_ID, 1800);
	if (ret) {
		printk( KERN_ERR " SDIO Qualcomm HCD: 1.8V set level failed\n");
	}

	ret = pm_vote_vreg_switch_pc(WLAN_1_8V_ID, 0);
	if (ret) {
		printk( KERN_ERR " SDIO Qualcomm HCD: 1.8V switch pc failed\n");
	}

	// for 1.2V
	gpio_direction_output(WLAN_1_2V_ID, 1);

	// back to off state
	gpio_direction_output(WLAN_CHIP_PWD_L_ID, 0);
	mdelay(1);
	gpio_direction_output(WLAN_CHIP_PWD_L_ID, 1);
	mdelay(40);

	 *
	 * the end of the sampe code.
	 */
	pr_info("[hp@wifi]: ar6000_pwr_on()\n");
	gpio_direction_output(18, 1);
}

static void ar6000_pwr_down(void)
{
	pr_info("[hp@wifi]: ar6000_pwr_down()\n");
	gpio_direction_output(18, 0);
	/* we can power down the chip here */
}

static int ar6000_suspend(struct platform_device *dev, pm_message_t state)
{
#if 0 /* pseudo code */
	if (msmsdcc_wlan_pwr_ctrl == WLAN_PWR_CTRL_CUT_PWR) {
		/*
		 * Because the host controller driver will
		 * be informed for the suspend event first,
		 * the cut-power suspend stuff has finished
		 * and we can power down the chip now.
		*/
		ar6000_pwr_down();
	}
	if (msmsdcc_wlan_pwr_ctrl == WLAN_PWR_CTRL_WOW) {
		ar6000_wow_suspend();
		msmsdcc_disable_wlan_slot();
	}
#endif
	pr_info("[hp@wifi]: ar6000_suspend()\n");
	ar6000_pwr_down();
	return 0;
}

static int ar6000_resume(struct platform_device *dev)
{
#if 0 /* pseudo code */
	if (msmsdcc_wlan_pwr_ctrl == WLAN_PWR_CTRL_CUT_PWR) {
		/*
		 * Because we will be informed for the resume
		 * event earlier than the host controller driver,
		 * we can power on the chip first by ourselves.
		 * So the host controller driver can finish the
		 * remaining cut-power resume stuff.
		 */
		ar6000_pwr_on();
	}
	if (msmsdcc_wlan_pwr_ctrl == WLAN_PWR_CTRL_WOW) {
		msmsdcc_enable_wlan_slot();
		ar6000_wow_resume();
	}
#endif
	pr_info("[hp@wifi]: ar6000_resume()\n");
	ar6000_pwr_on();
	return 0;
}

static int ar6000_probe(struct platform_device *pdev)
{
	/*ar6000_pwr_on();*/
	return 0;
}

static int ar6000_remove(struct platform_device *pdev)
{
	/*ar6000_pwr_down();*/
	return 0;
}

static struct platform_driver ar6000_device = {
	.probe		= ar6000_probe,
	.remove		= ar6000_remove,
	.suspend	= ar6000_suspend,
	.resume		= ar6000_resume,
	.driver		= {
			.name = "wlan_ar6000",
	},
};
#endif /* ANDROID_ENV && CONFIG_PM */
/* ATHENV */

static int __init
ar6000_init_module(void)
{
    static int probed = 0;
    A_STATUS status;
    OSDRV_CALLBACKS osdrvCallbacks;

/* ATHENV */
#ifdef ANDROID_ENV

#if defined(ANDROID_ENV) && defined(FORCE_TCMD_MODE)
    work_mode = 1;
#endif

#if defined(ANDROID_ENV) && defined(FORCE_ART_MODE)
    work_mode = 2;
#endif

    if (work_mode == 1) {
        AR_DEBUG_PRINTF("TCMD mode.\n");
        testmode = 1;
        tgt_fw = tcmd_fw;
    } else if (work_mode == 2) {
        AR_DEBUG_PRINTF("ART mode.\n");
        enableuartprint = 1;
        resetok = 0;
        bypasswmi = 1;
        tgt_fw = art_fw;
    }else {
        AR_DEBUG_PRINTF("Normal WIFI mode.\n");
        if (fm_path) {
            strcat(tgt_fw_path,fm_path);
            tgt_fw=strcat(tgt_fw_path,"/target/AR6002/hw2.0/bin/athwlan.bin.z77");
            strcat(tgt_patch_path,fm_path);
            tgt_patch=strcat(tgt_patch_path,"/target/AR6002/hw2.0/bin/data.patch.hw2_0.bin");
            strcat(eeprom_bin_path,fm_path);
            eeprom_bin=strcat(eeprom_bin_path,"/target/AR6002/hw2.0/bin/eeprom.bin");
            strcat(eeprom_data_path,fm_path);
            eeprom_data=strcat(eeprom_data_path,"/target/AR6002/hw2.0/bin/eeprom.data");
        }
    }
#endif
/* ATHENV */

    A_MEMZERO(&osdrvCallbacks,sizeof(osdrvCallbacks));
    osdrvCallbacks.deviceInsertedHandler = ar6000_avail_ev;
    osdrvCallbacks.deviceRemovedHandler = ar6000_unavail_ev;

#ifdef DEBUG
    /* Set the debug flags if specified at load time */
    if(debugflags != 0)
    {
        g_dbg_flags = debugflags;
    }
#endif

    if (probed) {
        return -ENODEV;
    }
    probed++;

#ifdef ADAPTIVE_POWER_THROUGHPUT_CONTROL
    memset(&aptcTR, 0, sizeof(APTC_TRAFFIC_RECORD));
#endif /* ADAPTIVE_POWER_THROUGHPUT_CONTROL */

#ifdef CONFIG_HOST_GPIO_SUPPORT
    ar6000_gpio_init();
#endif /* CONFIG_HOST_GPIO_SUPPORT */
    //ZTE_WIFI_HP_009 
    //let sdcc detect ONLY ONCE 
    //PS: we only TEMPORARILY enable polling on init_module and 
    //module_clean_up 
    ar6000_enable_mmc_polling(1);
    pr_info("[hp@wifi]:enable polling\n");

    ar6000_enable_mmc_polling(0);
    pr_info("[hp@wifi]:disable  polling\n");
    //ZTE_WIFI_HP_009 end 

/* ATHENV */
#ifdef ANDROID_ENV
    wake_lock_init(&ar6k_init_wake_lock, WAKE_LOCK_SUSPEND, "ar6k_init");
#endif
/* ATHENV */

    status = HIFInit(&osdrvCallbacks);
    if(status != A_OK)
        return -ENODEV;

/* ATHENV */
#if defined(ANDROID_ENV) && defined(CONFIG_PM)
    if (platform_driver_register(&ar6000_device))
        printk("ar6000: fail to register the driver.\n");
#endif
/* ATHENV */

    return 0;
}

static void __exit
ar6000_cleanup_module(void)
{
    int i = 0, ret = 0;
    struct net_device *ar6000_netdev;
    struct vreg *vreg;
    int rc;

#ifdef ADAPTIVE_POWER_THROUGHPUT_CONTROL
    /* Delete the Adaptive Power Control timer */
    if (timer_pending(&aptcTimer)) {
        del_timer_sync(&aptcTimer);
    }
#endif /* ADAPTIVE_POWER_THROUGHPUT_CONTROL */

    for (i=0; i < MAX_AR6000; i++) {
        if (ar6000_devices[i] != NULL) {
            ar6000_netdev = ar6000_devices[i];
            ar6000_devices[i] = NULL;
/* ATHENV */
#ifdef ANDROID_ENV
            printk("AR6K: ar6000_cleanup_module ar6000_destroy [%d]\n", i);
#endif
/* ATHENV */
            ar6000_destroy(ar6000_netdev, 1);
        }
    }

    HIFShutDownDevice(NULL);

/* ATHENV */
#ifdef ANDROID_ENV
    wake_lock_destroy(&ar6k_init_wake_lock);
#ifdef CONFIG_PM
    platform_driver_unregister(&ar6000_device);
#endif
#endif
/* ATHENV */

    rc = gpio_request(18, "wlan_chip_pwd");
    if(!rc)
    {
        gpio_direction_output(18, 0);
        /*mdelay(30);*/
    }
    else
    {
        printk(KERN_ERR "gpio_request:%d failed\n", rc);
    }
    gpio_free(18);

    //pull down 18 
    vreg = vreg_get(0, "wlan");
    ret = vreg_disable(vreg);
    if(ret)
    {
        printk(KERN_ERR"verg_disable failed\n");
    }
    else
    {
        printk(KERN_ERR"verg_disable succeed\n");
    }

    AR_DEBUG_PRINTF("ar6000_cleanup: success\n");

    //ZTE_WIFI_HP_005 2009-12-28
    /*here temporarily enable  then disable sdio 2 polling for offer sdio host 2 
     * a chance to remove card */
    ar6000_enable_mmc_polling(1);
    pr_info("[hp@wifi]:enable polling\n");

    ar6000_enable_mmc_polling(0);
    pr_info("[hp@wifi]:disable  polling\n");
    //ZTE_WIFI_HP_005 end 
}

#ifdef ADAPTIVE_POWER_THROUGHPUT_CONTROL
void
aptcTimerHandler(unsigned long arg)
{
    A_UINT32 numbytes;
    A_UINT32 throughput;
    AR_SOFTC_T *ar;
    A_STATUS status;

    ar = (AR_SOFTC_T *)arg;
    A_ASSERT(ar != NULL);
    A_ASSERT(!timer_pending(&aptcTimer));

    AR6000_SPIN_LOCK(&ar->arLock, 0);

    /* Get the number of bytes transferred */
    numbytes = aptcTR.bytesTransmitted + aptcTR.bytesReceived;
    aptcTR.bytesTransmitted = aptcTR.bytesReceived = 0;

    /* Calculate and decide based on throughput thresholds */
    throughput = ((numbytes * 8)/APTC_TRAFFIC_SAMPLING_INTERVAL); /* Kbps */
    if (throughput < APTC_LOWER_THROUGHPUT_THRESHOLD) {
        /* Enable Sleep and delete the timer */
        A_ASSERT(ar->arWmiReady == TRUE);
        AR6000_SPIN_UNLOCK(&ar->arLock, 0);
        status = wmi_powermode_cmd(ar->arWmi, REC_POWER);
        AR6000_SPIN_LOCK(&ar->arLock, 0);
        A_ASSERT(status == A_OK);
        aptcTR.timerScheduled = FALSE;
    } else {
        A_TIMEOUT_MS(&aptcTimer, APTC_TRAFFIC_SAMPLING_INTERVAL, 0);
    }

    AR6000_SPIN_UNLOCK(&ar->arLock, 0);
}
#endif /* ADAPTIVE_POWER_THROUGHPUT_CONTROL */

#define MAX_BUF (8*1024)
/* #define A_ROUND_UP(x, y) ((((x)+((y)-1))/(y))*(y))  */ /* already defined in wmi_api.h */

/* ATHENV */
#ifdef ANDROID_ENV

extern void eeprom_ar6000_transfer(HIF_DEVICE *device, char *fake_file, char *p_mac, int regcode);

char * fw_buf;
static void firmware_transfer(HIF_DEVICE *device, char* filename, A_UINT32 address, A_BOOL isCompressed)
{
    struct file     *filp;
    struct inode    *inode = NULL;
    int         length, remaining;
    int         length1;
    A_STATUS        ret;
    mm_segment_t    oldfs;


    AR_DEBUG_PRINTF("%s: Enter, filename=%s\n", __FUNCTION__, filename);

    // Open file
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
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20)
    inode = filp->f_path.dentry->d_inode;
#else
    inode = filp->f_dentry->d_inode;
#endif

    if (!inode) {
        printk("%s: Get inode from filp failed\n", __FUNCTION__);
        filp_close(filp, NULL);
        return;
    }

    AR_DEBUG_PRINTF("%s file offset opsition: %xh\n", __FUNCTION__, (unsigned)filp->f_pos);

    fw_buf = (char*)kmalloc((MAX_BUF+12), GFP_KERNEL);
    if (fw_buf == NULL) {
        printk("%s: kernel memory alloc error\n", __FUNCTION__);
        filp_close(filp, NULL);
        return;
    }

    length = i_size_read(inode->i_mapping->host);
    if (length == 0) {
        printk("%s: Try to get file size error\n", __FUNCTION__);
        goto Transfer_DONE;
    }
    AR_DEBUG_PRINTF("%s: length=%d, address=0x%x\n", __FUNCTION__, length, address);

    if (isCompressed) {
        ret = BMILZStreamStart(device, address);
        if (ret != A_OK) {
            printk("%s: BMILZStreamStart failed, ret=%d\n", __FUNCTION__, ret);
            goto Transfer_DONE;
        }
    }

    remaining = length;

    while (remaining>0) {
        length = (remaining > MAX_BUF)? MAX_BUF : remaining;

        if (isCompressed) {
            ((A_UINT32 *)fw_buf)[((length-1)/4)] = 0;
        }

        if (filp->f_op->read(filp, fw_buf, length, &filp->f_pos) != length) {
            printk("%s: file read error, remaining=%d\n", __FUNCTION__, remaining);
            goto Transfer_DONE;
        }

        length1 =  A_ROUND_UP(length, 4);

        if (isCompressed) {
            AR_DEBUG_PRINTF("%s: BMILZData: len=%d, org_len=%d\n", __FUNCTION__, length1, length);
            ret = BMILZData(device, fw_buf, length1);
            if (ret != A_OK) {
                printk("%s: BMILZData failed, ret=%d\n", __FUNCTION__, ret);
                goto Transfer_DONE;
            }
        } else {
            ret = BMIWriteMemory(device, address, fw_buf, length1);
            if (ret != A_OK) {
                printk("%s: BMIWriteMemory failed, ret=%d\n", __FUNCTION__, ret);
                goto Transfer_DONE;
            }
        }

        remaining -= length;
        address += length;
    }

Transfer_DONE:
    kfree(fw_buf);
    filp_close(filp, NULL);
    set_fs(oldfs);
}

#ifdef REGION_CODE_FILE_USED
void get_reg_code_from_file(char *reg_file, int *val_ptr)
{
	mm_segment_t		oldfs;
	struct file		*filp;
	struct inode		*inode = NULL;
	int			length;

	/* open file */
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	filp = filp_open(reg_file, O_RDONLY, S_IRUSR);

	if (IS_ERR(filp)) {
		set_fs(oldfs);
		return;
	}

	if (!filp->f_op) {
		printk("%s: File Operation Method Error\n", __func__);
		filp_close(filp, NULL);
		set_fs(oldfs);
		return;
	}

	inode = filp->f_path.dentry->d_inode;
	if (!inode) {
		printk("%s: Get inode from filp failed\n", __func__);
		filp_close(filp, NULL);
		set_fs(oldfs);
		return;
	}

	/* read data */
	length = i_size_read(inode->i_mapping->host);
	if ((length < 2) || (filp->f_op->read(filp, (void*)val_ptr, 2, &filp->f_pos) != 2)) {
		printk("%s: file read error, length=[0x%x]\n", __func__, length);
		filp_close(filp, NULL);
		set_fs(oldfs);
		return;
	}

	/* read data out successfully */
	filp_close(filp, NULL);
	set_fs(oldfs);
}
#endif /* REGION_CODE_FILE_USED */
#endif /* ANDROID_ENV */
/* ATHENV */

#ifdef FW_AUTOLOAD
extern int fwengine( unsigned char *img, int size, void *ar);
extern int ar6k_reg_preload( int reg, unsigned int value );

/* Linux driver dependent utilities for fwengine */
int load_binary( unsigned int addr, unsigned char *cp, void *arg )
{
    int size = 0;
    int adv  = 5;
    AR_SOFTC_T *ar;

    ar = (AR_SOFTC_T *)arg;

    cp++;
    size |= ( *cp & 0xFF );       cp++;
    size |= ( *cp & 0xFF ) <<  8; cp++;
    size |= ( *cp & 0xFF ) << 16; cp++;
    size |= ( *cp & 0xFF ) << 24; cp++;

    if (BMIWriteMemory(ar->arHifDevice, addr, cp, size) != A_OK)
        return(-1);

    adv += size;
    return(adv);
}

int execute_on_target( unsigned int address, unsigned int parm, void *arg )
{
    int ret ;
    AR_SOFTC_T *ar;

    ar = (AR_SOFTC_T *)arg;
    ret = BMIExecute(ar->arHifDevice, address, &parm);

    return(ret);
}

unsigned int get_target_reg( unsigned address, void *arg )
{
    int ret ;
    AR_SOFTC_T *ar;

    ar = (AR_SOFTC_T *)arg;
    if (BMIReadMemory(ar->arHifDevice, address, (A_UCHAR *)&ret, 4)!= A_OK) {
        /* And what am I supposed to do? */;
        return( -1 );
    }
    return( ret );

}

int write_target_reg( unsigned address, unsigned value, void *arg )
{
    AR_SOFTC_T *ar;

    ar = (AR_SOFTC_T *)arg;
    if (BMIWriteMemory(ar->arHifDevice, address, (A_UCHAR *)&value, 4) != A_OK)
        return(-1);
    return(0);
}

void bmidone( void *arg )
{
    AR_SOFTC_T *ar;

    ar = (AR_SOFTC_T *)arg;
    BMIDone(ar->arHifDevice);
}
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,21)
static struct device ar6kfwdev = {
        .bus_id    = "sdio0",
};
#endif
#endif /* FW_AUTOLOAD */

/*
 * HTC Event handlers
 */
static A_STATUS
ar6000_avail_ev(void *context, void *hif_handle)
{
    int i;
    struct net_device *dev;
    void *ar_netif;
    AR_SOFTC_T *ar;
    int device_index = 0;
    A_UINT32 param;
    HTC_INIT_INFO  htcInfo;

    AR_DEBUG_PRINTF("ar6000_available\n");

    for (i=0; i < MAX_AR6000; i++) {
        if (ar6000_devices[i] == NULL) {
            break;
        }
    }

    if (i == MAX_AR6000) {
        AR_DEBUG_PRINTF("ar6000_available: max devices reached\n");
        return A_ERROR;
    }

    /* Save this. It gives a bit better readability especially since */
    /* we use another local "i" variable below.                      */
    device_index = i;

/* ATHENV */
#ifdef ANDROID_ENV
    if (dev_removed && pending_devs[device_index]) {
        printk("%s: Device inserted again\n", __func__);
        dev = pending_devs[device_index];
        pending_devs[device_index] = NULL;
    } else {
        dev = alloc_etherdev(sizeof(AR_SOFTC_T));
        if (dev == NULL) {
            AR_DEBUG_PRINTF("ar6000_available: can't alloc etherdev\n");
            return A_ERROR;
        }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
        if (def_ifname[0] != '\0') 
            strcpy(ifname, def_ifname);

        if (ifname[0])
        {
            strcpy(dev->name, ifname);
            AR_DEBUG_PRINTF("AR6K: dev->name=%s\n", dev->name);
        }
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0) */

#ifdef SET_MODULE_OWNER
        SET_MODULE_OWNER(dev);
#endif
        ether_setup(dev);
    }

#else
    dev = alloc_etherdev(sizeof(AR_SOFTC_T));
    if (dev == NULL) {
        AR_DEBUG_PRINTF("ar6000_available: can't alloc etherdev\n");
        return A_ERROR;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    if (ifname[0])
    {
        strcpy(dev->name, ifname);
    }
#endif /* LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0) */

#ifdef SET_MODULE_OWNER
    SET_MODULE_OWNER(dev);
#endif
    ether_setup(dev);
#endif
/* ATHENV */

    ar_netif = netdev_priv(dev);
    if (ar_netif == NULL) {
        printk(KERN_CRIT "ar6000_available: Could not allocate memory\n");
        return A_ERROR;
    }

    A_MEMZERO(ar_netif, sizeof(AR_SOFTC_T));

    ar                       = (AR_SOFTC_T *)ar_netif;
    ar->arNetDev             = dev;
    ar->arHifDevice          = hif_handle;
    ar->arWlanState          = WLAN_ENABLED;
    ar->arDeviceIndex        = device_index;

    A_INIT_TIMER(&ar->arHBChallengeResp.timer, ar6000_detect_error, dev);
    ar->arHBChallengeResp.seqNum = 0;
    ar->arHBChallengeResp.outstanding = FALSE;
    ar->arHBChallengeResp.missCnt = 0;
    ar->arHBChallengeResp.frequency = AR6000_HB_CHALLENGE_RESP_FREQ_DEFAULT;
    ar->arHBChallengeResp.missThres = AR6000_HB_CHALLENGE_RESP_MISS_THRES_DEFAULT;

    ar6000_init_control_info(ar);
    init_waitqueue_head(&arEvent);
    sema_init(&ar->arSem, 1);
    ar->bIsDestroyProgress = FALSE;

#ifdef ADAPTIVE_POWER_THROUGHPUT_CONTROL
    A_INIT_TIMER(&aptcTimer, aptcTimerHandler, ar);
#endif /* ADAPTIVE_POWER_THROUGHPUT_CONTROL */

    A_INIT_TIMER(&ar->disconnect_timer, disconnect_timer_handler, dev);

    /*
     * If requested, perform some magic which requires no cooperation from
     * the Target.  It causes the Target to ignore flash and execute to the
     * OS from ROM.
     *
     * This is intended to support recovery from a corrupted flash on Targets
     * that support flash.
     */
    if (skipflash)
    {
        AR_DEBUG_PRINTF("Skip flash feature not supported\n");
        return A_ERROR;
    }

    BMIInit();
    {
        struct bmi_target_info targ_info;

        if (BMIGetTargetInfo(ar->arHifDevice, &targ_info) != A_OK) {
            return A_ERROR;
        }

        ar->arVersion.target_ver = targ_info.target_ver;
        ar->arTargetType = targ_info.target_type;

            /* do any target-specific preparation that can be done through BMI */
        if (ar6000_prepare_target(ar->arHifDevice,
                                  targ_info.target_type,
                                  targ_info.target_ver) != A_OK) {
            return A_ERROR;
        }

    }

    if (enableuartprint) {
        param = 1;
        if (BMIWriteMemory(ar->arHifDevice,
                           HOST_INTEREST_ITEM_ADDRESS(ar, hi_serial_enable),
                           (A_UCHAR *)&param,
                           4)!= A_OK)
        {
             AR_DEBUG_PRINTF("BMIWriteMemory for enableuartprint failed \n");
             return A_ERROR;
        }
        AR_DEBUG_PRINTF("Serial console prints enabled\n");
    }

    /* Tell target which HTC version it is used*/
    param = HTC_PROTOCOL_VERSION;
    if (BMIWriteMemory(ar->arHifDevice,
                       HOST_INTEREST_ITEM_ADDRESS(ar, hi_app_host_interest),
                       (A_UCHAR *)&param,
                       4)!= A_OK)
    {
         AR_DEBUG_PRINTF("BMIWriteMemory for htc version failed \n");
         return A_ERROR;
    }

#ifdef CONFIG_HOST_TCMD_SUPPORT
    if(testmode) {
        ar->arTargetMode = AR6000_TCMD_MODE;
    }else {
        ar->arTargetMode = AR6000_WLAN_MODE;
    }
#endif
    if (enabletimerwar) {
        A_UINT32 param;

        if (BMIReadMemory(ar->arHifDevice,
            HOST_INTEREST_ITEM_ADDRESS(ar, hi_option_flag),
            (A_UCHAR *)&param,
            4)!= A_OK)
        {
            AR_DEBUG_PRINTF("BMIReadMemory for enabletimerwar failed \n");
            return A_ERROR;
        }

        param |= HI_OPTION_TIMER_WAR;

        if (BMIWriteMemory(ar->arHifDevice,
            HOST_INTEREST_ITEM_ADDRESS(ar, hi_option_flag),
            (A_UCHAR *)&param,
            4) != A_OK)
        {
            AR_DEBUG_PRINTF("BMIWriteMemory for enabletimerwar failed \n");
            return A_ERROR;
        }
        AR_DEBUG_PRINTF("Timer WAR enabled\n");
    }

    /* set the firmware mode to STA/IBSS/AP */
    {
        A_UINT32 param;

        if (BMIReadMemory(ar->arHifDevice,
            HOST_INTEREST_ITEM_ADDRESS(ar, hi_option_flag),
            (A_UCHAR *)&param,
            4)!= A_OK)
        {
            AR_DEBUG_PRINTF("BMIReadMemory for setting fwmode failed \n");
            return A_ERROR;
        }

        param |= (fwmode << HI_OPTION_FW_MODE_SHIFT);

        if (BMIWriteMemory(ar->arHifDevice,
            HOST_INTEREST_ITEM_ADDRESS(ar, hi_option_flag),
            (A_UCHAR *)&param,
            4) != A_OK)
        {
            AR_DEBUG_PRINTF("BMIWriteMemory for setting fwmode failed \n");
            return A_ERROR;
        }
        AR_DEBUG_PRINTF("Firmware mode set\n");
    }

    if (processDot11Hdr) {
        A_UINT32 param;

        if (BMIReadMemory(ar->arHifDevice,
            HOST_INTEREST_ITEM_ADDRESS(ar, hi_option_flag),
            (A_UCHAR *)&param,
            4)!= A_OK)
        {
            AR_DEBUG_PRINTF("BMIReadMemory for processDot11Hdr failed \n");
            return A_ERROR;
        }

        param |= HI_OPTION_RELAY_DOT11_HDR;

        if (BMIWriteMemory(ar->arHifDevice,
            HOST_INTEREST_ITEM_ADDRESS(ar, hi_option_flag),
            (A_UCHAR *)&param,
            4) != A_OK)
        {
            AR_DEBUG_PRINTF("BMIWriteMemory for processDot11Hdr failed \n");
            return A_ERROR;
        }
        AR_DEBUG_PRINTF("processDot11Hdr enabled\n");
    }


    // No need to reserve RAM space for patch as olca/dragon is flash based
    if (ar->arTargetType == TARGET_TYPE_AR6001) {
        param = 0;
        if (BMIWriteMemory(ar->arHifDevice,
            HOST_INTEREST_ITEM_ADDRESS(ar, hi_end_RAM_reserve_sz),
            (A_UCHAR *)&param,
            4) != A_OK)
        {
            AR_DEBUG_PRINTF("BMIWriteMemory for hi_end_RAM_reserve_sz failed \n");
            return A_ERROR;
        }
    }


        /* since BMIInit is called in the driver layer, we have to set the block
         * size here for the target */

    if (A_FAILED(ar6000_set_htc_params(ar->arHifDevice,
                                       ar->arTargetType,
                                       mbox_yield_limit,
                                       0 /* use default number of control buffers */
                                       ))) {
        return A_ERROR;
    }

    A_MEMZERO(&htcInfo,sizeof(htcInfo));
    htcInfo.pContext = ar;
    htcInfo.TargetFailure = ar6000_target_failure;

    ar->arHtcTarget = HTCCreate(ar->arHifDevice,&htcInfo);

    if (ar->arHtcTarget == NULL) {
        return A_ERROR;
    }

    spin_lock_init(&ar->arLock);

    /* Don't install the init function if BMI is requested */
    if(!bmienable)
    {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29)
        dev->init = ar6000_init;
#else
        ar6000_netdev_ops.ndo_init = ar6000_init;
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29) */
    } else {
        AR_DEBUG_PRINTF(" BMI enabled \n");
    }


#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29)
    dev->open = &ar6000_open;
    dev->stop = &ar6000_close;
    dev->hard_start_xmit = &ar6000_data_tx;
    dev->get_stats = &ar6000_get_stats;

    /* dev->tx_timeout = ar6000_tx_timeout; */
    dev->do_ioctl = &ar6000_ioctl;
#else
    dev->netdev_ops = &ar6000_netdev_ops;
#endif /* LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29) */
    dev->watchdog_timeo = AR6000_TX_TIMEOUT;
    dev->wireless_handlers = &ath_iw_handler_def;

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
    dev->get_wireless_stats = ar6000_get_iwstats; /*Displayed via proc fs */
#else
    ath_iw_handler_def.get_wireless_stats = ar6000_get_iwstats; /*Displayed via proc fs */
#endif

    if (processDot11Hdr) {
        dev->hard_header_len = sizeof(struct ieee80211_qosframe) + sizeof(ATH_LLC_SNAP_HDR) + sizeof(WMI_DATA_HDR) + HTC_HEADER_LEN;
    } else {
        /*
         * We need the OS to provide us with more headroom in order to
         * perform dix to 802.3, WMI header encap, and the HTC header
         */
        dev->hard_header_len = ETH_HLEN + sizeof(ATH_LLC_SNAP_HDR) +
            sizeof(WMI_DATA_HDR) + HTC_HEADER_LEN;
    }

/* ATHENV */
#ifdef ANDROID_ENV
    if (dev_removed) {
        printk("Not need to register device again\n");
        dev_removed = 0;
    } else {
        /* This runs the init function */
        if (register_netdev(dev)) {
            AR_DEBUG_PRINTF("ar6000_avail: register_netdev failed\n");
            printk("AR6K: ar6000_avail: register_netdev failed ar6000_destroy\n");
            ar6000_destroy(dev, 0);
            return A_ERROR;
        }
    }
#else
    /* This runs the init function */
    if (register_netdev(dev)) {
        AR_DEBUG_PRINTF("ar6000_avail: register_netdev failed\n");
        ar6000_destroy(dev, 0);
        return A_ERROR;
    }
#endif
/* ATHENV */

    HIFClaimDevice(ar->arHifDevice, ar);

    /* We only register the device in the global list if we succeed. */
    /* If the device is in the global list, it will be destroyed     */
    /* when the module is unloaded.                                  */
    ar6000_devices[device_index] = dev;

    AR_DEBUG_PRINTF("ar6000_avail: name=%s hifdevice=0x%x, dev=0x%x (%d), ar=0x%x\n",
                    dev->name, (A_UINT32)ar->arHifDevice, (A_UINT32)dev, device_index,
                    (A_UINT32)ar);
/* ATHENV */
#ifdef ANDROID_ENV
    if (tgt_fw != NULL) {
        A_UINT32 value, old_options, old_sleep;
        A_STATUS ret;

        /* temporarily disable system sleep */
        value = 0;
        BMIReadSOCRegister(ar->arHifDevice, 0x180c0, &value);
        old_options = value;
        value |= 0x08;
        BMIWriteSOCRegister(ar->arHifDevice, 0x180c0, value);
        value = 0;
        BMIReadSOCRegister(ar->arHifDevice, 0x40c4, &value);
        old_sleep = value;
        value |= 0x01;
        BMIWriteSOCRegister(ar->arHifDevice, 0x40c4, value);
        AR_DEBUG_PRINTF("old options [%d] old sleep [%d]\n", old_options, old_sleep);

        /* run at 40/44MHz by default */
        value = 0;
        BMIWriteSOCRegister(ar->arHifDevice, 0x4020, value);

        /* Set hi_refclk_hz */
        AR_DEBUG_PRINTF("Set hi_refclk_hz : Ref Clock=%d\n", refClock);	
        BMIWriteSOCRegister(ar->arHifDevice, 0x500478, refClock);

        /* use internal clock? */
        BMIReadSOCRegister(ar->arHifDevice, 0x50047c, &value);
        if (value == 0) {
            printk("use internal clock\n");
            value = 0x100000;
            BMIWriteSOCRegister(ar->arHifDevice, 0x40e0, value);
        }

#ifdef REGION_CODE_FILE_USED
        /* get region code from a file? */
        if (reg_file != NULL)
            get_reg_code_from_file(reg_file, &regCode);
#endif

        /* eeprom */
        /*
         * Change to use the mechanism as Olca 2.1 : eeprom -> Host -> FW
         * With this mechanism, though the data needs to move to the host side first.
         * But we can change the eeprom data at the driver side
         */
#if 1 /* The way Olca 2.1 used */

#ifdef EEPROM_FILE_USED  /* eeprom file -> Host -> FW */
        AR_DEBUG_PRINTF("AR6000: download eeprom from a file\n");
#else  /* eeprom -> Host -> FW */
        AR_DEBUG_PRINTF("AR6000: eeprom transfer by HOST\n");
#endif
        eeprom_ar6000_transfer(ar->arHifDevice, eeprom_file, softmac_file, regCode);

#else /* The way Olca 2.2 used originally: eeprom -> FW */
        AR_DEBUG_PRINTF("AR6000: eeprom transfer by TARGET\n");
        firmware_transfer(ar->arHifDevice, eeprom_data, 0x502070, FALSE);
        firmware_transfer(ar->arHifDevice, eeprom_bin, 0x5140f0, FALSE);
        value = 1;
        printk("AR6000: BMIExecute\n");
        BMIExecute(ar->arHifDevice, 0x9140f0, &value);
#endif
        AR_DEBUG_PRINTF("AR6000: BMISetAppStart\n");
        BMISetAppStart(ar->arHifDevice, 0x9140f0);

        /* enable HI_OPTION_TIMER_WAR */
        AR_DEBUG_PRINTF("AR6000: enable HI_OPTION_TIMER_WAR\n");
        value = 0;
        BMIReadSOCRegister(ar->arHifDevice, 0x500410, &value);
        value |= 0x01;
        BMIWriteSOCRegister(ar->arHifDevice, 0x500410, value);

        /* fw */
        AR_DEBUG_PRINTF("AR6000: firmware_transfer\n");
        if ((tgt_fw[strlen(tgt_fw) - 3] == 'z')
         && (tgt_fw[strlen(tgt_fw) - 2] == '7')
         && (tgt_fw[strlen(tgt_fw) - 1] == '7')) {
            firmware_transfer(ar->arHifDevice, tgt_fw, 0x502070, TRUE);
        } else {
            firmware_transfer(ar->arHifDevice, tgt_fw, 0x502070, FALSE);
        }

        /* WLAN patch DataSets */
        firmware_transfer(ar->arHifDevice, tgt_patch, 0x52d6d0, FALSE);
        BMIWriteSOCRegister(ar->arHifDevice, 0x500418, 0x52d6d0);

        /* restore system sleep */
        BMIWriteSOCRegister(ar->arHifDevice, 0x40c4, old_sleep);
        BMIWriteSOCRegister(ar->arHifDevice, 0x180c0, old_options);

        if (work_mode == 2) {
            /* art mode */
            BMIWriteSOCRegister(ar->arHifDevice, 0x500478, refClock);
            BMIWriteSOCRegister(ar->arHifDevice, 0x500458, 0x1);
            msleep(1000);
        } else {
            /* normal WIFI or TCMD mode, done */
            ret = ar6000_init(dev);
            if (ret!= A_OK) {
                printk("%s: ar6000_init failed, ret=%d\n", __FUNCTION__, ret);
            }

            if ((work_mode == 0) && (ret == A_OK)) {
                /* configure channel number? */
                if ((chan_num == 11) || (chan_num == 13) || (chan_num == 14)) {
                    int i;
                    A_UINT16 chan_list[16] = {0};

                    AR_DEBUG_PRINTF("AR6000: configure channel number [0x%x]\n", chan_num);
                    for (i = 0; i < chan_num; i++) {
                        /* channel 14 */
                        if (i == 14)
                            chan_list[i] = 2484;
                        /* channel 1~13 */
                        chan_list[i] = (2407 + ((i + 1) * 5));
                    }
                    if (wmi_set_channelParams_cmd(ar->arWmi, 1, WMI_11G_MODE, chan_num, chan_list) != A_OK)
                        printk("Fialed to configure channel number\n");
                }
                /* start a scan immediately if it's normal WIFI mode */
                if (wmi_bssfilter_cmd(ar->arWmi, ALL_BSS_FILTER, 0) != A_OK)
                    printk("Fialed to set filter\n");
                if (wmi_startscan_cmd(ar->arWmi, WMI_LONG_SCAN, FALSE, FALSE, 0, 0, 0, NULL) != A_OK)
                    printk("Failed to send scan cmd\n");
                /* configure disconnect timeout value */
                if (wmi_disctimeout_cmd(ar->arWmi, 3) != A_OK)
                    printk("Failed to disctimeout cmd\n");
            }
        }
    }
#endif /* ANDROID_ENV */
/* ATHENV */

#ifdef FW_AUTOLOAD
    if( fwloadenable ) {
        /* To compile firmware into driver the following struct should
         * be declared static, it's field 'data' initialised with ptr to
         * image and field 'size' with image size. 'request_firmware call
         * in that case should be bypassed. TODO: #ifdef that
         */
        const struct firmware *fw_entry;
        int                    ret;
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,21)
        if(request_firmware(&fw_entry, "ar6k_firmware", &ar6kfwdev)!=0) {
#else
        if(request_firmware(&fw_entry, "ar6k_firmware", &dev->dev)!=0) {
#endif
//        if(request_firmware(NULL, "ar6k_firmware", &dev->dev)!=0) {
            printk(KERN_ERR "ar6000_fwload: ar6k_firmware not available\n");
            ar6000_destroy(dev, 0);
            return A_ERROR;
        } else {
            ar6k_reg_preload( 14, ar->arTargetType );
            ar6k_reg_preload( 15, ar->arVersion.target_ver );
            ret = fwengine(fw_entry->data, fw_entry->size, (void *)ar);
            release_firmware(fw_entry);
            if( ret ) {
                printk(KERN_ERR "ar600_fwload: error loading firmware\n");
                ar6000_destroy(dev, 0);
                return A_ERROR;
            }
        }
    }
#endif /* FW_AUTOLOAD */

    return A_OK;
}

static void ar6000_target_failure(void *Instance, A_STATUS Status)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)Instance;
    WMI_TARGET_ERROR_REPORT_EVENT errEvent;
    static A_BOOL sip = FALSE;

    if (Status != A_OK) {

        printk(KERN_ERR "ar6000_target_failure: target asserted \n");

        if (timer_pending(&ar->arHBChallengeResp.timer)) {
            A_UNTIMEOUT(&ar->arHBChallengeResp.timer);
        }

        /* try dumping target assertion information (if any) */
        ar6000_dump_target_assert_info(ar->arHifDevice,ar->arTargetType);

        /*
         * Fetch the logs from the target via the diagnostic
         * window.
         */
        ar6000_dbglog_get_debug_logs(ar);

        /* Report the error only once */
        if (!sip) {
            sip = TRUE;
            errEvent.errorVal = WMI_TARGET_COM_ERR |
                                WMI_TARGET_FATAL_ERR;
            ar6000_send_event_to_app(ar, WMI_ERROR_REPORT_EVENTID,
                                     (A_UINT8 *)&errEvent,
                                     sizeof(WMI_TARGET_ERROR_REPORT_EVENT));
        }
    }
}

static A_STATUS
ar6000_unavail_ev(void *context, void *hif_handle)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)context;
    //ZTE_WIFI_HP_002
    /*ar6000_enable_mmc_polling(1);*/
    /*pr_info("hp: reenable polling at unavail\n");*/
    //ZTE_WIFI_HP_002 end 

/* ATHENV */
#ifdef ANDROID_ENV
    if (ar6000_devices[ar->arDeviceIndex]) {
        ar6000_devices[ar->arDeviceIndex] = NULL;
    } else {
        return A_OK;
    }

    dev_removed = 1;
    AR_DEBUG_PRINTF("AR6K: ar6000_unavail_ev : ar6000_destroy\n");
    /* avoid unregistering device and freeing memory */
    ar6000_destroy(ar->arNetDev, 2);
#else
    /* NULL out it's entry in the global list */
    ar6000_devices[ar->arDeviceIndex] = NULL;
    ar6000_destroy(ar->arNetDev, 1);
#endif
/* ATHENV */

    return A_OK;
}

/*
 * We need to differentiate between the surprise and planned removal of the
 * device because of the following consideration:
 * - In case of surprise removal, the hcd already frees up the pending
 *   for the device and hence there is no need to unregister the function
 *   driver inorder to get these requests. For planned removal, the function
 *   driver has to explictly unregister itself to have the hcd return all the
 *   pending requests before the data structures for the devices are freed up.
 *   Note that as per the current implementation, the function driver will
 *   end up releasing all the devices since there is no API to selectively
 *   release a particular device.
 * - Certain commands issued to the target can be skipped for surprise
 *   removal since they will anyway not go through.
 */
static void
ar6000_destroy(struct net_device *dev, unsigned int unregister)
{
    AR_SOFTC_T *ar;

    AR_DEBUG_PRINTF("+ar6000_destroy \n");

    if((dev == NULL) || ((ar = netdev_priv(dev)) == NULL))
    {
        AR_DEBUG_PRINTF("%s(): Failed to get device structure.\n", __func__);
        return;
    }

    ar->bIsDestroyProgress = TRUE;

    if (down_interruptible(&ar->arSem)) {
        AR_DEBUG_PRINTF("%s(): down_interruptible failed \n", __func__);
        return;
    }

    /* Stop the transmit queues */
    netif_stop_queue(dev);

    //ZTE_WIFI_HP_010  
    //delete remaining timer
    if (timer_pending(&ar->arHBChallengeResp.timer)) {
        A_UNTIMEOUT(&ar->arHBChallengeResp.timer);
    }
#ifdef ADAPTIVE_POWER_THROUGHPUT_CONTROL
    if (timer_pending(&aptcTimer)) {
        A_UNTIMEOUT(&aptcTimer);
    }
#endif
    if (timer_pending(&ar->disconnect_timer)) {
        A_UNTIMEOUT(&ar->disconnect_timer);
    }
    //ZTE_WIFI_HP_010 end 

    /* Disable the target and the interrupts associated with it */
    if (ar->arWmiReady == TRUE)
    {
        if (!bypasswmi)
        {
            if (ar->arConnected == TRUE || ar->arConnectPending == TRUE)
            {
                AR_DEBUG_PRINTF("%s(): Disconnect\n", __func__);
                AR6000_SPIN_LOCK(&ar->arLock, 0);
                ar6000_init_profile_info(ar);
                AR6000_SPIN_UNLOCK(&ar->arLock, 0);

                wmi_disconnect_cmd(ar->arWmi);
		//ZTE_WIFI_HP_012 
		//work around for disconnect cmd, we need to update the 
		//arConnected flag 
		ar->arConnected = FALSE;
		//ZTE_WIFI_HP_012 end

            }

/* ATHENV */
#ifdef ANDROID_ENV
            mdelay(500);
#else
            ar6000_dbglog_get_debug_logs(ar);
#endif
/* ATHENV */
            ar->arWmiReady  = FALSE;
            ar->arConnected = FALSE;
            ar->arConnectPending = FALSE;
            wmi_shutdown(ar->arWmi);
            ar->arWmiEnabled = FALSE;
            ar->arWmi = NULL;
	    //ZTE_WIFI_HP_005  2009-12-28
	    //fix wifi wlan state 
            ar->arWlanState = WLAN_DISABLED;
	    //ZTE_WIFI_HP_005 end 
#ifdef USER_KEYS
            ar->user_savedkeys_stat = USER_SAVEDKEYS_STAT_INIT;
            ar->user_key_ctrl      = 0;
#endif
        }

         AR_DEBUG_PRINTF("%s(): WMI stopped\n", __func__);
    }
    else
    {
        AR_DEBUG_PRINTF("%s(): WMI not ready 0x%08x 0x%08x\n",
            __func__, (unsigned int) ar, (unsigned int) ar->arWmi);

        /* Shut down WMI if we have started it */
        if(ar->arWmiEnabled == TRUE)
        {
            AR_DEBUG_PRINTF("%s(): Shut down WMI\n", __func__);
            wmi_shutdown(ar->arWmi);
            ar->arWmiEnabled = FALSE;
            ar->arWmi = NULL;
        }
    }

    if (ar->arHtcTarget != NULL) {
        AR_DEBUG_PRINTF(" Shuting down HTC .... \n");
        /* stop HTC */
        HTCStop(ar->arHtcTarget);
        /* destroy HTC */
        HTCDestroy(ar->arHtcTarget);
    }

    if (resetok) {
        /* try to reset the device if we can
         * The driver may have been configure NOT to reset the target during
         * a debug session */
        AR_DEBUG_PRINTF(" Attempting to reset target on instance destroy.... \n");
        if (ar->arHifDevice != NULL) {
            ar6000_reset_device(ar->arHifDevice, ar->arTargetType, TRUE);
        }
    } else {
        AR_DEBUG_PRINTF(" Host does not want target reset. \n");
    }

    if (ar->arHifDevice != NULL) {
        /*release the device so we do not get called back on remove incase we
         * we're explicity destroyed by module unload */
        HIFReleaseDevice(ar->arHifDevice);
        HIFShutDownDevice(ar->arHifDevice);
    }

       /* Done with cookies */
    ar6000_cookie_cleanup(ar);

    /* Cleanup BMI */
    BMIInit();

    /* Clear the tx counters */
    memset(tx_attempt, 0, sizeof(tx_attempt));
    memset(tx_post, 0, sizeof(tx_post));
    memset(tx_complete, 0, sizeof(tx_complete));

/* ATHENV */
#ifdef ANDROID_ENV

    /* Free up the device data structure */
    if (unregister == 0) {
        /* free memory only */
#ifndef free_netdev
        kfree(dev);
#else
        free_netdev(dev);
#endif
    } else if (unregister == 1) {
        /* unregister device and free memory */
        unregister_netdev(dev);
#ifndef free_netdev
        kfree(dev);
#else
        free_netdev(dev);
#endif
    } else {
        /* don't do anything */
        printk("Keep network device %d\n", ar->arDeviceIndex);
        pending_devs[ar->arDeviceIndex] = dev;
    }

#else /* ! ANDROID_ENV */

    /* Free up the device data structure */
    if( unregister )
        unregister_netdev(dev);
#ifndef free_netdev
    kfree(dev);
#else
    free_netdev(dev);
#endif

#endif /* ! ANDROID_ENV */
/* ATHENV */

    AR_DEBUG_PRINTF("-ar6000_destroy \n");
}

static void disconnect_timer_handler(unsigned long ptr)
{
    struct net_device *dev = (struct net_device *)ptr;
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);

    A_UNTIMEOUT(&ar->disconnect_timer);

    ar6000_init_profile_info(ar);
    wmi_disconnect_cmd(ar->arWmi);
    //ZTE_WIFI_HP_012 
    //work around for disconnect cmd, we need to update the 
    //arConnected flag 
    ar->arConnected = FALSE;
    //ZTE_WIFI_HP_012 end 
}

static void ar6000_detect_error(unsigned long ptr)
{
    struct net_device *dev = (struct net_device *)ptr;
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);
    WMI_TARGET_ERROR_REPORT_EVENT errEvent;

    AR6000_SPIN_LOCK(&ar->arLock, 0);

    if (ar->arHBChallengeResp.outstanding) {
        ar->arHBChallengeResp.missCnt++;
    } else {
        ar->arHBChallengeResp.missCnt = 0;
    }

    if (ar->arHBChallengeResp.missCnt > ar->arHBChallengeResp.missThres) {
        /* Send Error Detect event to the application layer and do not reschedule the error detection module timer */
        ar->arHBChallengeResp.missCnt = 0;
        ar->arHBChallengeResp.seqNum = 0;
        errEvent.errorVal = WMI_TARGET_COM_ERR | WMI_TARGET_FATAL_ERR;
        AR6000_SPIN_UNLOCK(&ar->arLock, 0);
        ar6000_send_event_to_app(ar, WMI_ERROR_REPORT_EVENTID,
                                 (A_UINT8 *)&errEvent,
                                 sizeof(WMI_TARGET_ERROR_REPORT_EVENT));
        return;
    }

    /* Generate the sequence number for the next challenge */
    ar->arHBChallengeResp.seqNum++;
    ar->arHBChallengeResp.outstanding = TRUE;

    AR6000_SPIN_UNLOCK(&ar->arLock, 0);

    /* Send the challenge on the control channel */
    if (wmi_get_challenge_resp_cmd(ar->arWmi, ar->arHBChallengeResp.seqNum, DRV_HB_CHALLENGE) != A_OK) {
        AR_DEBUG_PRINTF("Unable to send heart beat challenge\n");
    }


    /* Reschedule the timer for the next challenge */
    A_TIMEOUT_MS(&ar->arHBChallengeResp.timer, ar->arHBChallengeResp.frequency * 1000, 0);
}

void ar6000_init_profile_info(AR_SOFTC_T *ar)
{
    ar->arSsidLen            = 0;
    A_MEMZERO(ar->arSsid, sizeof(ar->arSsid));

    switch(fwmode) {
        case HI_OPTION_FW_MODE_IBSS:
            ar->arNetworkType = ar->arNextMode = ADHOC_NETWORK;
            break;
        case HI_OPTION_FW_MODE_BSS_STA:
            ar->arNetworkType = ar->arNextMode = INFRA_NETWORK;
            break;
        case HI_OPTION_FW_MODE_AP:
            ar->arNetworkType = ar->arNextMode = AP_NETWORK;
            break;
    }

    ar->arDot11AuthMode      = OPEN_AUTH;
    ar->arAuthMode           = NONE_AUTH;
    ar->arPairwiseCrypto     = NONE_CRYPT;
    ar->arPairwiseCryptoLen  = 0;
    ar->arGroupCrypto        = NONE_CRYPT;
    ar->arGroupCryptoLen     = 0;
    A_MEMZERO(ar->arWepKeyList, sizeof(ar->arWepKeyList));
    A_MEMZERO(ar->arReqBssid, sizeof(ar->arReqBssid));
    A_MEMZERO(ar->arBssid, sizeof(ar->arBssid));
    ar->arBssChannel = 0;
}

static void
ar6000_init_control_info(AR_SOFTC_T *ar)
{
    ar->arWmiEnabled         = FALSE;
    ar6000_init_profile_info(ar);
    ar->arDefTxKeyIndex      = 0;
    A_MEMZERO(ar->arWepKeyList, sizeof(ar->arWepKeyList));
    ar->arChannelHint        = 0;
    ar->arListenInterval     = MAX_LISTEN_INTERVAL;
    ar->arVersion.host_ver   = AR6K_SW_VERSION;
    ar->arRssi               = 0;
    ar->arTxPwr              = 0;
    ar->arTxPwrSet           = FALSE;
    ar->arSkipScan           = 0;
    ar->arBeaconInterval     = 0;
    ar->arBitRate            = 0;
    ar->arMaxRetries         = 0;
    ar->arWmmEnabled         = TRUE;
    ar->intra_bss            = 1;

    /* Initialize the AP mode state info */
    {
        A_UINT8 ctr;
        A_MEMZERO((A_UINT8 *)ar->sta_list, AP_MAX_NUM_STA * sizeof(sta_t));

        /* init the Mutexes */
        A_MUTEX_INIT(&ar->mcastpsqLock);

        /* Init the PS queues */
        for (ctr=0; ctr < AP_MAX_NUM_STA ; ctr++) {
            A_MUTEX_INIT(&ar->sta_list[ctr].psqLock);
            A_NETBUF_QUEUE_INIT(&ar->sta_list[ctr].psq);
        }

        ar->ap_profile_flag = 0;
        A_NETBUF_QUEUE_INIT(&ar->mcastpsq);

        A_MEMCPY(ar->ap_country_code, DEF_AP_COUNTRY_CODE, 3);
        ar->ap_wmode = DEF_AP_WMODE_G;
        ar->ap_dtim_period = DEF_AP_DTIM;
        ar->ap_beacon_interval = DEF_BEACON_INTERVAL;
    }
}

static int
ar6000_open(struct net_device *dev)
{
    unsigned long  flags;
    AR_SOFTC_T    *ar = (AR_SOFTC_T *)netdev_priv(dev);

    spin_lock_irqsave(&ar->arLock, flags);
    if( ar->arConnected || bypasswmi) {
        netif_carrier_on(dev);
        /* Wake up the queues */
        netif_wake_queue(dev);
    }
    else
        netif_carrier_off(dev);

    spin_unlock_irqrestore(&ar->arLock, flags);
    return 0;
}

static int
ar6000_close(struct net_device *dev)
{
    netif_stop_queue(dev);

    return 0;
}

/* connect to a service */
static A_STATUS ar6000_connectservice(AR_SOFTC_T               *ar,
                                      HTC_SERVICE_CONNECT_REQ  *pConnect,
                                      char                     *pDesc)
{
    A_STATUS                 status;
    HTC_SERVICE_CONNECT_RESP response;

    do {

        A_MEMZERO(&response,sizeof(response));

        status = HTCConnectService(ar->arHtcTarget,
                                   pConnect,
                                   &response);

        if (A_FAILED(status)) {
            AR_DEBUG_PRINTF(" Failed to connect to %s service status:%d \n",
                              pDesc, status);
            break;
        }
        switch (pConnect->ServiceID) {
            case WMI_CONTROL_SVC :
                if (ar->arWmiEnabled) {
                        /* set control endpoint for WMI use */
                    wmi_set_control_ep(ar->arWmi, response.Endpoint);
                }
                    /* save EP for fast lookup */
                ar->arControlEp = response.Endpoint;
                break;
            case WMI_DATA_BE_SVC :
                arSetAc2EndpointIDMap(ar, WMM_AC_BE, response.Endpoint);
                break;
            case WMI_DATA_BK_SVC :
                arSetAc2EndpointIDMap(ar, WMM_AC_BK, response.Endpoint);
                break;
            case WMI_DATA_VI_SVC :
                arSetAc2EndpointIDMap(ar, WMM_AC_VI, response.Endpoint);
                 break;
           case WMI_DATA_VO_SVC :
                arSetAc2EndpointIDMap(ar, WMM_AC_VO, response.Endpoint);
                break;
           default:
                AR_DEBUG_PRINTF("ServiceID not mapped %d\n", pConnect->ServiceID);
                status = A_EINVAL;
            break;
        }

    } while (FALSE);

    return status;
}

void ar6000_TxDataCleanup(AR_SOFTC_T *ar)
{
        /* flush all the data (non-control) streams
         * we only flush packets that are tagged as data, we leave any control packets that
         * were in the TX queues alone */
    HTCFlushEndpoint(ar->arHtcTarget,
                     arAc2EndpointID(ar, WMM_AC_BE),
                     AR6K_DATA_PKT_TAG);
    HTCFlushEndpoint(ar->arHtcTarget,
                     arAc2EndpointID(ar, WMM_AC_BK),
                     AR6K_DATA_PKT_TAG);
    HTCFlushEndpoint(ar->arHtcTarget,
                     arAc2EndpointID(ar, WMM_AC_VI),
                     AR6K_DATA_PKT_TAG);
    HTCFlushEndpoint(ar->arHtcTarget,
                     arAc2EndpointID(ar, WMM_AC_VO),
                     AR6K_DATA_PKT_TAG);
}

HTC_ENDPOINT_ID
ar6000_ac2_endpoint_id ( void * devt, A_UINT8 ac)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *) devt;
    return(arAc2EndpointID(ar, ac));
}

A_UINT8
ar6000_endpoint_id2_ac(void * devt, HTC_ENDPOINT_ID ep )
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *) devt;
    return(arEndpoint2Ac(ar, ep ));
}

/* This function does one time initialization for the lifetime of the device */
int ar6000_init(struct net_device *dev)
{
    AR_SOFTC_T *ar;
    A_STATUS    status;
    A_INT32     timeleft;

    if((ar = netdev_priv(dev)) == NULL)
    {
        return(-EIO);
    }

    /* Do we need to finish the BMI phase */
    if(BMIDone(ar->arHifDevice) != A_OK)
    {
        return -EIO;
    }

    if (!bypasswmi)
    {
#if 0 /* TBDXXX */
        if (ar->arVersion.host_ver != ar->arVersion.target_ver) {
            A_PRINTF("WARNING: Host version 0x%x does not match Target "
                    " version 0x%x!\n",
                    ar->arVersion.host_ver, ar->arVersion.target_ver);
        }
#endif

        /* Indicate that WMI is enabled (although not ready yet) */
        ar->arWmiEnabled = TRUE;
        if ((ar->arWmi = wmi_init((void *) ar)) == NULL)
        {
            AR_DEBUG_PRINTF("%s() Failed to initialize WMI.\n", __func__);
            return(-EIO);
        }

        AR_DEBUG_PRINTF("%s() Got WMI @ 0x%08x.\n", __func__,
            (unsigned int) ar->arWmi);
    }

    do {
        HTC_SERVICE_CONNECT_REQ connect;

            /* the reason we have to wait for the target here is that the driver layer
             * has to init BMI in order to set the host block size,
             */
        status = HTCWaitTarget(ar->arHtcTarget);

        if (A_FAILED(status)) {
            break;
        }

        A_MEMZERO(&connect,sizeof(connect));
            /* meta data is unused for now */
        connect.pMetaData = NULL;
        connect.MetaDataLength = 0;
            /* these fields are the same for all service endpoints */
        connect.EpCallbacks.pContext = ar;
        connect.EpCallbacks.EpTxComplete = ar6000_tx_complete;
        connect.EpCallbacks.EpRecv = ar6000_rx;
        connect.EpCallbacks.EpRecvRefill = ar6000_rx_refill;
        connect.EpCallbacks.EpSendFull = ar6000_tx_queue_full;
            /* set the max queue depth so that our ar6000_tx_queue_full handler gets called.
             * Linux has the peculiarity of not providing flow control between the
             * NIC and the network stack. There is no API to indicate that a TX packet
             * was sent which could provide some back pressure to the network stack.
             * Under linux you would have to wait till the network stack consumed all sk_buffs
             * before any back-flow kicked in. Which isn't very friendly.
             * So we have to manage this ourselves */
        connect.MaxSendQueueDepth = 32;

            /* connect to control service */
        connect.ServiceID = WMI_CONTROL_SVC;
        status = ar6000_connectservice(ar,
                                       &connect,
                                       "WMI CONTROL");
        if (A_FAILED(status)) {
            break;
        }

            /* for the remaining data services set the connection flag to reduce dribbling,
             * if configured to do so */
        if (reduce_credit_dribble) {
            connect.ConnectionFlags |= HTC_CONNECT_FLAGS_REDUCE_CREDIT_DRIBBLE;
            /* the credit dribble trigger threshold is (reduce_credit_dribble - 1) for a value
             * of 0-3 */
            connect.ConnectionFlags &= ~HTC_CONNECT_FLAGS_THRESHOLD_LEVEL_MASK;
            connect.ConnectionFlags |=
                        ((A_UINT16)reduce_credit_dribble - 1) & HTC_CONNECT_FLAGS_THRESHOLD_LEVEL_MASK;
        }
            /* connect to best-effort service */
        connect.ServiceID = WMI_DATA_BE_SVC;

        status = ar6000_connectservice(ar,
                                       &connect,
                                       "WMI DATA BE");
        if (A_FAILED(status)) {
            break;
        }

            /* connect to back-ground
             * map this to WMI LOW_PRI */
        connect.ServiceID = WMI_DATA_BK_SVC;
        status = ar6000_connectservice(ar,
                                       &connect,
                                       "WMI DATA BK");
        if (A_FAILED(status)) {
            break;
        }

            /* connect to Video service, map this to
             * to HI PRI */
        connect.ServiceID = WMI_DATA_VI_SVC;
        status = ar6000_connectservice(ar,
                                       &connect,
                                       "WMI DATA VI");
        if (A_FAILED(status)) {
            break;
        }

            /* connect to VO service, this is currently not
             * mapped to a WMI priority stream due to historical reasons.
             * WMI originally defined 3 priorities over 3 mailboxes
             * We can change this when WMI is reworked so that priorities are not
             * dependent on mailboxes */
        connect.ServiceID = WMI_DATA_VO_SVC;
        status = ar6000_connectservice(ar,
                                       &connect,
                                       "WMI DATA VO");
        if (A_FAILED(status)) {
            break;
        }

        A_ASSERT(arAc2EndpointID(ar,WMM_AC_BE) != 0);
        A_ASSERT(arAc2EndpointID(ar,WMM_AC_BK) != 0);
        A_ASSERT(arAc2EndpointID(ar,WMM_AC_VI) != 0);
        A_ASSERT(arAc2EndpointID(ar,WMM_AC_VO) != 0);

            /* setup access class priority mappings */
        ar->arAcStreamPriMap[WMM_AC_BK] = 0; /* lowest  */
        ar->arAcStreamPriMap[WMM_AC_BE] = 1; /*         */
        ar->arAcStreamPriMap[WMM_AC_VI] = 2; /*         */
        ar->arAcStreamPriMap[WMM_AC_VO] = 3; /* highest */

    } while (FALSE);

    if (A_FAILED(status)) {
        return (-EIO);
    }

    /*
     * give our connected endpoints some buffers
     */

    ar6000_rx_refill(ar, ar->arControlEp);
    ar6000_rx_refill(ar, arAc2EndpointID(ar,WMM_AC_BE));

    /*
     * We will post the receive buffers only for SPE or endpoint ping testing so we are
     * making it conditional on the 'bypasswmi' flag.
     */
    if (bypasswmi) {
        ar6000_rx_refill(ar,arAc2EndpointID(ar,WMM_AC_BK));
        ar6000_rx_refill(ar,arAc2EndpointID(ar,WMM_AC_VI));
        ar6000_rx_refill(ar,arAc2EndpointID(ar,WMM_AC_VO));
    }

        /* setup credit distribution */
    ar6000_setup_credit_dist(ar->arHtcTarget, &ar->arCreditStateInfo);

    /* Since cookies are used for HTC transports, they should be */
    /* initialized prior to enabling HTC.                        */
    ar6000_cookie_init(ar);

    /* start HTC */
    status = HTCStart(ar->arHtcTarget);

    if (status != A_OK) {
        if (ar->arWmiEnabled == TRUE) {
            wmi_shutdown(ar->arWmi);
            ar->arWmiEnabled = FALSE;
            ar->arWmi = NULL;
        }
        ar6000_cookie_cleanup(ar);
        return -EIO;
    }

    if (!bypasswmi) {
        /* Wait for Wmi event to be ready */
        timeleft = wait_event_interruptible_timeout(arEvent,
            (ar->arWmiReady == TRUE), wmitimeout * HZ);

        if(!timeleft || signal_pending(current))
        {
            AR_DEBUG_PRINTF("WMI is not ready or wait was interrupted\n");
            return -EIO;
        }

        AR_DEBUG_PRINTF("%s() WMI is ready\n", __func__);

        /* Communicate the wmi protocol verision to the target */
        if ((ar6000_set_host_app_area(ar)) != A_OK) {
            AR_DEBUG_PRINTF("Unable to set the host app area\n");
    }
    }

    ar->arNumDataEndPts = 1;

    if (bypasswmi) {
            /* for tests like endpoint ping, the MAC address needs to be non-zero otherwise
             * the data path through a raw socket is disabled */
        dev->dev_addr[0] = 0x00;
        dev->dev_addr[1] = 0x01;
        dev->dev_addr[2] = 0x02;
        dev->dev_addr[3] = 0xAA;
        dev->dev_addr[4] = 0xBB;
        dev->dev_addr[5] = 0xCC;
    }

    return(0);
}


void
ar6000_bitrate_rx(void *devt, A_INT32 rateKbps)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)devt;

    ar->arBitRate = rateKbps;
    wake_up(&arEvent);
}

void
ar6000_ratemask_rx(void *devt, A_UINT16 ratemask)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)devt;

    ar->arRateMask = ratemask;
    wake_up(&arEvent);
}

void
ar6000_txPwr_rx(void *devt, A_UINT8 txPwr)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)devt;

    ar->arTxPwr = txPwr;
    wake_up(&arEvent);
}


void
ar6000_channelList_rx(void *devt, A_INT8 numChan, A_UINT16 *chanList)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)devt;

    A_MEMCPY(ar->arChannelList, chanList, numChan * sizeof (A_UINT16));
    ar->arNumChannels = numChan;

    wake_up(&arEvent);
}

A_UINT8
ar6000_ibss_map_epid(struct sk_buff *skb, struct net_device *dev, A_UINT32 * mapNo)
{
    AR_SOFTC_T      *ar = (AR_SOFTC_T *)netdev_priv(dev);
    A_UINT8         *datap;
    ATH_MAC_HDR     *macHdr;
    A_UINT32         i, eptMap;

    (*mapNo) = 0;
    datap = A_NETBUF_DATA(skb);
    macHdr = (ATH_MAC_HDR *)(datap + sizeof(WMI_DATA_HDR));
    if (IEEE80211_IS_MULTICAST(macHdr->dstMac)) {
        return ENDPOINT_2;
    }

    eptMap = -1;
    for (i = 0; i < ar->arNodeNum; i ++) {
        if (IEEE80211_ADDR_EQ(macHdr->dstMac, ar->arNodeMap[i].macAddress)) {
            (*mapNo) = i + 1;
            ar->arNodeMap[i].txPending ++;
            return ar->arNodeMap[i].epId;
        }

        if ((eptMap == -1) && !ar->arNodeMap[i].txPending) {
            eptMap = i;
        }
    }

    if (eptMap == -1) {
        eptMap = ar->arNodeNum;
        ar->arNodeNum ++;
        A_ASSERT(ar->arNodeNum <= MAX_NODE_NUM);
    }

    A_MEMCPY(ar->arNodeMap[eptMap].macAddress, macHdr->dstMac, IEEE80211_ADDR_LEN);

    for (i = ENDPOINT_2; i <= ENDPOINT_5; i ++) {
        if (!ar->arTxPending[i]) {
            ar->arNodeMap[eptMap].epId = i;
            break;
        }
        // No free endpoint is available, start redistribution on the inuse endpoints.
        if (i == ENDPOINT_5) {
            ar->arNodeMap[eptMap].epId = ar->arNexEpId;
            ar->arNexEpId ++;
            if (ar->arNexEpId > ENDPOINT_5) {
                ar->arNexEpId = ENDPOINT_2;
            }
        }
    }

    (*mapNo) = eptMap + 1;
    ar->arNodeMap[eptMap].txPending ++;

    return ar->arNodeMap[eptMap].epId;
}

#ifdef DEBUG
static void ar6000_dump_skb(struct sk_buff *skb)
{
   u_char *ch;
   for (ch = A_NETBUF_DATA(skb);
        (A_UINT32)ch < ((A_UINT32)A_NETBUF_DATA(skb) +
        A_NETBUF_LEN(skb)); ch++)
    {
         AR_DEBUG_PRINTF("%2.2x ", *ch);
    }
    AR_DEBUG_PRINTF("\n");
}
#endif

static int
ar6000_data_tx(struct sk_buff *skb, struct net_device *dev)
{
#define AC_NOT_MAPPED   99
    AR_SOFTC_T        *ar = (AR_SOFTC_T *)netdev_priv(dev);
    A_UINT8            ac = AC_NOT_MAPPED;
    HTC_ENDPOINT_ID    eid = ENDPOINT_UNUSED;
    A_UINT32          mapNo = 0;
    int               len;
    struct ar_cookie *cookie;
    A_BOOL            checkAdHocPsMapping = FALSE,bMoreData = FALSE;

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,13)
    skb->list = NULL;
#endif

    AR_DEBUG2_PRINTF("ar6000_data_tx start - skb=0x%x, data=0x%x, len=0x%x\n",
                     (A_UINT32)skb, (A_UINT32)A_NETBUF_DATA(skb),
                     A_NETBUF_LEN(skb));

    /* If target is not associated */
    if( (!ar->arConnected && !bypasswmi)
#ifdef CONFIG_HOST_TCMD_SUPPORT
     /* TCMD doesnt support any data, free the buf and return */
    || (ar->arTargetMode == AR6000_TCMD_MODE)
#endif
                                            ) {
        A_NETBUF_FREE(skb);
        return 0;
    }

    do {

        if (ar->arWmiReady == FALSE && bypasswmi == 0) {
            break;
        }

#ifdef BLOCK_TX_PATH_FLAG
        if (blocktx) {
            break;
        }
#endif /* BLOCK_TX_PATH_FLAG */

        /* AP mode Power save processing */
        /* If the dst STA is in sleep state, queue the pkt in its PS queue */

        if (ar->arNetworkType == AP_NETWORK) {
            ATH_MAC_HDR *datap = (ATH_MAC_HDR *)A_NETBUF_DATA(skb);
            sta_t *conn = NULL;

            /* If the dstMac is a Multicast address & atleast one of the
             * associated STA is in PS mode, then queue the pkt to the
             * mcastq
             */
            if (IEEE80211_IS_MULTICAST(datap->dstMac)) {
                A_UINT8 ctr=0;
                A_BOOL qMcast=FALSE;

                for (ctr=0; ctr<AP_MAX_NUM_STA; ctr++) {
                    if (STA_IS_PWR_SLEEP((&ar->sta_list[ctr]))) {
                        qMcast = TRUE;
                    }
                }
                if(qMcast) {
                    /* If this transmit is not because of a Dtim Expiry q it */
                    if (ar->DTIMExpired == FALSE) {
                        A_BOOL isMcastqEmpty = FALSE;

                        A_MUTEX_LOCK(&ar->mcastpsqLock);
                        isMcastqEmpty = A_NETBUF_QUEUE_EMPTY(&ar->mcastpsq);
                        A_NETBUF_ENQUEUE(&ar->mcastpsq, skb);
                        A_MUTEX_UNLOCK(&ar->mcastpsqLock);

                        /* If this is the first Mcast pkt getting queued
                         * indicate to the target to set the BitmapControl LSB
                         * of the TIM IE.
                         */
                        if (isMcastqEmpty) {
                             wmi_set_pvb_cmd(ar->arWmi, MCAST_AID, 1);
                        }
                        return 0;
                    } else {
                     /* This transmit is because of Dtim expiry. Determine if
                      * MoreData bit has to be set.
                      */
                         A_MUTEX_LOCK(&ar->mcastpsqLock);
                         if(!A_NETBUF_QUEUE_EMPTY(&ar->mcastpsq)) {
                             bMoreData = TRUE;
                         }
                         A_MUTEX_UNLOCK(&ar->mcastpsqLock);
                    }
                }
            } else {
                conn = ieee80211_find_conn(ar, datap->dstMac);
                if (conn) {
                    if (STA_IS_PWR_SLEEP(conn)) {
                        /* If this transmit is not because of a PsPoll q it*/
                        if (!STA_IS_PS_POLLED(conn)) {
                            A_BOOL isPsqEmpty = FALSE;
                            /* Queue the frames if the STA is sleeping */
                            A_MUTEX_LOCK(&conn->psqLock);
                            isPsqEmpty = A_NETBUF_QUEUE_EMPTY(&conn->psq);
                            A_NETBUF_ENQUEUE(&conn->psq, skb);
                            A_MUTEX_UNLOCK(&conn->psqLock);

                            /* If this is the first pkt getting queued
                             * for this STA, update the PVB for this STA
                             */
                            if (isPsqEmpty) {
                                wmi_set_pvb_cmd(ar->arWmi, conn->aid, 1);
                            }

                            return 0;
                         } else {
                         /* This tx is because of a PsPoll. Determine if
                          * MoreData bit has to be set
                          */
                             A_MUTEX_LOCK(&conn->psqLock);
                             if (!A_NETBUF_QUEUE_EMPTY(&conn->psq)) {
                                 bMoreData = TRUE;
                             }
                             A_MUTEX_UNLOCK(&conn->psqLock);
                         }
                    }
                } else {

                    /* non existent STA. drop the frame */
                    A_NETBUF_FREE(skb);
                    return 0;
                }
            }
        }

        if (ar->arWmiEnabled) {
            if (A_NETBUF_HEADROOM(skb) < dev->hard_header_len) {
                struct sk_buff  *newbuf;

                /*
                 * We really should have gotten enough headroom but sometimes
                 * we still get packets with not enough headroom.  Copy the packet.
                 */
                len = A_NETBUF_LEN(skb);
                newbuf = A_NETBUF_ALLOC(len);
                if (newbuf == NULL) {
                    break;
                }
                A_NETBUF_PUT(newbuf, len);
                A_MEMCPY(A_NETBUF_DATA(newbuf), A_NETBUF_DATA(skb), len);
                A_NETBUF_FREE(skb);
                skb = newbuf;
                /* fall through and assemble header */
            }

            if (processDot11Hdr) {
                if (wmi_dot11_hdr_add(ar->arWmi,skb,ar->arNetworkType) != A_OK) {
                    AR_DEBUG_PRINTF("ar6000_data_tx-wmi_dot11_hdr_add failed\n");
                    break;
                }
            } else {
                if (wmi_dix_2_dot3(ar->arWmi, skb) != A_OK) {
                    AR_DEBUG_PRINTF("ar6000_data_tx - wmi_dix_2_dot3 failed\n");
                    break;
                }
            }

            if (wmi_data_hdr_add(ar->arWmi, skb, DATA_MSGTYPE, bMoreData) != A_OK) {
                AR_DEBUG_PRINTF("ar6000_data_tx - wmi_data_hdr_add failed\n");
                break;
            }

            if ((ar->arNetworkType == ADHOC_NETWORK) &&
                ar->arIbssPsEnable && ar->arConnected) {
                    /* flag to check adhoc mapping once we take the lock below: */
                checkAdHocPsMapping = TRUE;

            } else {
                    /* get the stream mapping */
                ac  =  wmi_implicit_create_pstream(ar->arWmi, skb, 0, ar->arWmmEnabled);
            }

        } else {
            struct iphdr    *ipHdr;
            /*
             * the endpoint is directly based on the TOS field in the IP
             * header **** only for testing ******
             */
            ipHdr = A_NETBUF_DATA(skb) + sizeof(ATH_MAC_HDR);
                /* here we map the TOS field to an access class, this is for
                 * the endpointping test application.  The application uses 0,1,2,3
                 * for the TOS field to emulate writing to mailboxes.  The number is
                 * used to map directly to an access class */
            ac = (ipHdr->tos >> 1) & 0x3;
        }

    } while (FALSE);

        /* did we succeed ? */
    if ((ac == AC_NOT_MAPPED) && !checkAdHocPsMapping) {
            /* cleanup and exit */
        A_NETBUF_FREE(skb);
        AR6000_STAT_INC(ar, tx_dropped);
        AR6000_STAT_INC(ar, tx_aborted_errors);
        return 0;
    }

    cookie = NULL;

        /* take the lock to protect driver data */
    AR6000_SPIN_LOCK(&ar->arLock, 0);

    do {

        if (checkAdHocPsMapping) {
            eid = ar6000_ibss_map_epid(skb, dev, &mapNo);
        }else {
            eid = arAc2EndpointID (ar, ac);
        }
            /* validate that the endpoint is connected */
        if (eid == 0 || eid == ENDPOINT_UNUSED ) {
            AR_DEBUG_PRINTF(" eid %d is NOT mapped!\n", eid);
            break;
        }
            /* allocate resource for this packet */
        cookie = ar6000_alloc_cookie(ar);

        if (cookie != NULL) {
                /* update counts while the lock is held */
            ar->arTxPending[eid]++;
            ar->arTotalTxDataPending++;
        }

    } while (FALSE);

    AR6000_SPIN_UNLOCK(&ar->arLock, 0);

    if (cookie != NULL) {
        cookie->arc_bp[0] = (A_UINT32)skb;
        cookie->arc_bp[1] = mapNo;
        SET_HTC_PACKET_INFO_TX(&cookie->HtcPkt,
                               cookie,
                               A_NETBUF_DATA(skb),
                               A_NETBUF_LEN(skb),
                               eid,
                               AR6K_DATA_PKT_TAG);

#ifdef DEBUG
        if (debugdriver >= 3) {
            ar6000_dump_skb(skb);
        }
#endif
            /* HTC interface is asynchronous, if this fails, cleanup will happen in
             * the ar6000_tx_complete callback */
        HTCSendPkt(ar->arHtcTarget, &cookie->HtcPkt);
    } else {
            /* no packet to send, cleanup */
        A_NETBUF_FREE(skb);
        AR6000_STAT_INC(ar, tx_dropped);
        AR6000_STAT_INC(ar, tx_aborted_errors);
    }

    return 0;
}

#ifdef ADAPTIVE_POWER_THROUGHPUT_CONTROL
static void
tvsub(register struct timeval *out, register struct timeval *in)
{
    if((out->tv_usec -= in->tv_usec) < 0) {
        out->tv_sec--;
        out->tv_usec += 1000000;
    }
    out->tv_sec -= in->tv_sec;
}

void
applyAPTCHeuristics(AR_SOFTC_T *ar)
{
    A_UINT32 duration;
    A_UINT32 numbytes;
    A_UINT32 throughput;
    struct timeval ts;
    A_STATUS status;

    AR6000_SPIN_LOCK(&ar->arLock, 0);

    if ((enableAPTCHeuristics) && (!aptcTR.timerScheduled)) {
        do_gettimeofday(&ts);
        tvsub(&ts, &aptcTR.samplingTS);
        duration = ts.tv_sec * 1000 + ts.tv_usec / 1000; /* ms */
        numbytes = aptcTR.bytesTransmitted + aptcTR.bytesReceived;

        if (duration > APTC_TRAFFIC_SAMPLING_INTERVAL) {
            /* Initialize the time stamp and byte count */
            aptcTR.bytesTransmitted = aptcTR.bytesReceived = 0;
            do_gettimeofday(&aptcTR.samplingTS);

            /* Calculate and decide based on throughput thresholds */
            throughput = ((numbytes * 8) / duration);
            if (throughput > APTC_UPPER_THROUGHPUT_THRESHOLD) {
                /* Disable Sleep and schedule a timer */
                A_ASSERT(ar->arWmiReady == TRUE);
                AR6000_SPIN_UNLOCK(&ar->arLock, 0);
                status = wmi_powermode_cmd(ar->arWmi, MAX_PERF_POWER);
                AR6000_SPIN_LOCK(&ar->arLock, 0);
                A_TIMEOUT_MS(&aptcTimer, APTC_TRAFFIC_SAMPLING_INTERVAL, 0);
                aptcTR.timerScheduled = TRUE;
            }
        }
    }

    AR6000_SPIN_UNLOCK(&ar->arLock, 0);
}
#endif /* ADAPTIVE_POWER_THROUGHPUT_CONTROL */

static HTC_SEND_FULL_ACTION ar6000_tx_queue_full(void *Context, HTC_PACKET *pPacket)
{
    AR_SOFTC_T     *ar = (AR_SOFTC_T *)Context;
    HTC_SEND_FULL_ACTION    action = HTC_SEND_FULL_KEEP;
    A_BOOL                  stopNet = FALSE;
    HTC_ENDPOINT_ID         Endpoint = HTC_GET_ENDPOINT_FROM_PKT(pPacket);

    do {

        if (bypasswmi) {
            /* for endpointping testing no other checks need to be made
             * we can however still allow the network to stop */
            stopNet = TRUE;
            break;
        }

        if (Endpoint == ar->arControlEp) {
                /* under normal WMI if this is getting full, then something is running rampant
                 * the host should not be exhausting the WMI queue with too many commands
                 * the only exception to this is during testing using endpointping */
            AR6000_SPIN_LOCK(&ar->arLock, 0);
                /* set flag to handle subsequent messages */
            ar->arWMIControlEpFull = TRUE;
            AR6000_SPIN_UNLOCK(&ar->arLock, 0);
            AR_DEBUG_PRINTF("WMI Control Endpoint is FULL!!! \n");
                /* no need to stop the network */
            stopNet = FALSE;
            break;
        }

        /* if we get here, we are dealing with data endpoints getting full */

        if (HTC_GET_TAG_FROM_PKT(pPacket) == AR6K_CONTROL_PKT_TAG) {
            /* don't drop control packets issued on ANY data endpoint */
            break;
        }

        if (ar->arNetworkType == ADHOC_NETWORK) {
            /* in adhoc mode, we cannot differentiate traffic priorities so there is no need to
             * continue, however we should stop the network */
            stopNet = TRUE;
            break;
        }

        if (ar->arAcStreamPriMap[arEndpoint2Ac(ar,Endpoint)] < ar->arHiAcStreamActivePri) {
                /* this stream's priority is less than the highest active priority, we
                 * give preference to the highest priority stream by directing
                 * HTC to drop the packet that overflowed */
            action = HTC_SEND_FULL_DROP;
                /* since we are dropping packets, no need to stop the network */
            stopNet = FALSE;
            break;
        }

    } while (FALSE);

    if (stopNet) {
        AR6000_SPIN_LOCK(&ar->arLock, 0);
        ar->arNetQueueStopped = TRUE;
        AR6000_SPIN_UNLOCK(&ar->arLock, 0);
        /* one of the data endpoints queues is getting full..need to stop network stack
         * the queue will resume in ar6000_tx_complete() */
        netif_stop_queue(ar->arNetDev);
    }

    return action;
}


static void
ar6000_tx_complete(void *Context, HTC_PACKET *pPacket)
{
    AR_SOFTC_T     *ar = (AR_SOFTC_T *)Context;
    void           *cookie = (void *)pPacket->pPktContext;
    struct sk_buff *skb = NULL;
    A_UINT32        mapNo = 0;
    A_STATUS        status;
    struct ar_cookie * ar_cookie;
    HTC_ENDPOINT_ID   eid;
    A_BOOL          wakeEvent = FALSE;

    status = pPacket->Status;
    ar_cookie = (struct ar_cookie *)cookie;
    skb = (struct sk_buff *)ar_cookie->arc_bp[0];
    eid = pPacket->Endpoint ;
    mapNo = ar_cookie->arc_bp[1];

    A_ASSERT(skb);
    A_ASSERT(pPacket->pBuffer == A_NETBUF_DATA(skb));

    if (A_SUCCESS(status)) {
        A_ASSERT(pPacket->ActualLength == A_NETBUF_LEN(skb));
    }

    AR_DEBUG2_PRINTF("ar6000_tx_complete skb=0x%x data=0x%x len=0x%x eid=%d ",
                     (A_UINT32)skb, (A_UINT32)pPacket->pBuffer,
                     pPacket->ActualLength,
                     eid);

        /* lock the driver as we update internal state */
    AR6000_SPIN_LOCK(&ar->arLock, 0);

    ar->arTxPending[eid]--;

    if ((eid  != ar->arControlEp) || bypasswmi) {
        ar->arTotalTxDataPending--;
    }

    if (eid == ar->arControlEp)
    {
        if (ar->arWMIControlEpFull) {
                /* since this packet completed, the WMI EP is no longer full */
            ar->arWMIControlEpFull = FALSE;
        }

        if (ar->arTxPending[eid] == 0) {
            wakeEvent = TRUE;
        }
    }

    if (A_FAILED(status)) {
        AR6000_STAT_INC(ar, tx_errors);
        if (status != A_NO_RESOURCE) {
            AR_DEBUG_PRINTF("%s() -TX ERROR, status: 0x%x\n", __func__,
                        status);
        }
    } else {
        AR_DEBUG2_PRINTF("OK\n");
        AR6000_STAT_INC(ar, tx_packets);
        ar->arNetStats.tx_bytes += A_NETBUF_LEN(skb);
#ifdef ADAPTIVE_POWER_THROUGHPUT_CONTROL
        aptcTR.bytesTransmitted += a_netbuf_to_len(skb);
        applyAPTCHeuristics(ar);
#endif /* ADAPTIVE_POWER_THROUGHPUT_CONTROL */
    }

    // TODO this needs to be looked at
    if ((ar->arNetworkType == ADHOC_NETWORK) && ar->arIbssPsEnable
        && (eid != ar->arControlEp) && mapNo)
    {
        mapNo --;
        ar->arNodeMap[mapNo].txPending --;

        if (!ar->arNodeMap[mapNo].txPending && (mapNo == (ar->arNodeNum - 1))) {
            A_UINT32 i;
            for (i = ar->arNodeNum; i > 0; i --) {
                if (!ar->arNodeMap[i - 1].txPending) {
                    A_MEMZERO(&ar->arNodeMap[i - 1], sizeof(struct ar_node_mapping));
                    ar->arNodeNum --;
                } else {
                    break;
                }
            }
        }
    }

    /* Freeing a cookie should not be contingent on either of */
    /* these flags, just if we have a cookie or not.           */
    /* Can we even get here without a cookie? Fix later.       */
    if (ar->arWmiReady == TRUE || (bypasswmi))
    {
        ar6000_free_cookie(ar, cookie);
    }

    if (ar->arNetQueueStopped) {
        ar->arNetQueueStopped = FALSE;
    }

    AR6000_SPIN_UNLOCK(&ar->arLock, 0);

    /* lock is released, we can freely call other kernel APIs */

    A_NETBUF_FREE(skb);

    if ((ar->arConnected == TRUE) || (bypasswmi)) {
        if (status != A_ECANCELED) {
                /* don't wake the queue if we are flushing, other wise it will just
                 * keep queueing packets, which will keep failing */
            netif_wake_queue(ar->arNetDev);
        }
    }

    if (wakeEvent) {
        wake_up(&arEvent);
    }

}

sta_t *
ieee80211_find_conn(AR_SOFTC_T *ar, A_UINT8 *node_addr)
{
    sta_t *conn = NULL;
    A_UINT8 i, max_conn;

    switch(ar->arNetworkType) {
        case AP_NETWORK:
            max_conn = AP_MAX_NUM_STA;
            break;
        default:
            max_conn=0;
            break;
    }

    for (i = 0; i < max_conn; i++) {
        if (IEEE80211_ADDR_EQ(node_addr, ar->sta_list[i].mac)) {
            conn = &ar->sta_list[i];
            break;
        }
    }

    return conn;
}

sta_t *ieee80211_find_conn_for_aid(AR_SOFTC_T *ar, A_UINT8 aid)
{
    sta_t *conn = NULL;
    A_UINT8 ctr;

    for (ctr = 0; ctr < AP_MAX_NUM_STA; ctr++) {
        if (ar->sta_list[ctr].aid == aid) {
            conn = &ar->sta_list[ctr];
            break;
        }
    }
    return conn;
}

/*
 * Receive event handler.  This is called by HTC when a packet is received
 */
int pktcount;
static void
ar6000_rx(void *Context, HTC_PACKET *pPacket)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)Context;
    struct sk_buff *skb = (struct sk_buff *)pPacket->pPktContext;
    int minHdrLen;
    A_STATUS        status = pPacket->Status;
    HTC_ENDPOINT_ID   ept = pPacket->Endpoint;

    A_ASSERT((status != A_OK) ||
             (pPacket->pBuffer == (A_NETBUF_DATA(skb) + HTC_HEADER_LEN)));

    AR_DEBUG2_PRINTF("ar6000_rx ar=0x%x eid=%d, skb=0x%x, data=0x%x, len=0x%x ",
                    (A_UINT32)ar, ept, (A_UINT32)skb, (A_UINT32)pPacket->pBuffer,
                    pPacket->ActualLength);
    if (status != A_OK) {
        AR_DEBUG2_PRINTF("ERR\n");
    } else {
        AR_DEBUG2_PRINTF("OK\n");
    }

        /* take lock to protect buffer counts
         * and adaptive power throughput state */
    AR6000_SPIN_LOCK(&ar->arLock, 0);

    ar->arRxBuffers[ept]--;

    if (A_SUCCESS(status)) {
        AR6000_STAT_INC(ar, rx_packets);
        ar->arNetStats.rx_bytes += pPacket->ActualLength;
#ifdef ADAPTIVE_POWER_THROUGHPUT_CONTROL
        aptcTR.bytesReceived += a_netbuf_to_len(skb);
        applyAPTCHeuristics(ar);
#endif /* ADAPTIVE_POWER_THROUGHPUT_CONTROL */

        A_NETBUF_PUT(skb, pPacket->ActualLength +  HTC_HEADER_LEN);
        A_NETBUF_PULL(skb, HTC_HEADER_LEN);

#ifdef DEBUG
        if (debugdriver >= 2) {
            ar6000_dump_skb(skb);
        }
#endif /* DEBUG */
    }

    AR6000_SPIN_UNLOCK(&ar->arLock, 0);

    skb->dev = ar->arNetDev;
    if (status != A_OK) {
        AR6000_STAT_INC(ar, rx_errors);
        A_NETBUF_FREE(skb);
    } else if (ar->arWmiEnabled == TRUE) {
        if (ept == ar->arControlEp) {
           /*
            * this is a wmi control msg
            */
            wmi_control_rx(ar->arWmi, skb);
        } else {
                /*
                 * this is a wmi data packet
                 */
                 // NWF

                if (processDot11Hdr) {
                    minHdrLen = sizeof(WMI_DATA_HDR) + sizeof(struct ieee80211_frame) + sizeof(ATH_LLC_SNAP_HDR);
                } else {
                    minHdrLen = sizeof (WMI_DATA_HDR) + sizeof(ATH_MAC_HDR) +
                          sizeof(ATH_LLC_SNAP_HDR);
                }

                /* In the case of AP mode we may receive NULL data frames
                 * that do not have LLC hdr. They are 16 bytes in size.
                 * Allow these frames in the AP mode.
                 */
                if (ar->arNetworkType != AP_NETWORK && ((pPacket->ActualLength < minHdrLen) ||
                    (pPacket->ActualLength > AR6000_BUFFER_SIZE)))
                {
                    /*
                     * packet is too short or too long
                     */
                    AR_DEBUG_PRINTF("TOO SHORT or TOO LONG\n");
                    AR6000_STAT_INC(ar, rx_errors);
                    AR6000_STAT_INC(ar, rx_length_errors);
                    A_NETBUF_FREE(skb);
                } else {
#if 0
                    /* Access RSSI values here */
                    AR_DEBUG_PRINTF("RSSI %d\n",
                        ((WMI_DATA_HDR *) A_NETBUF_DATA(skb))->rssi);
#endif
                    /* Get the Power save state of the STA */
                    if (ar->arNetworkType == AP_NETWORK) {
                        sta_t *conn = NULL;
                        A_UINT8 psState=0,prevPsState;
                        ATH_MAC_HDR *datap=NULL;

                        psState = (((WMI_DATA_HDR *)A_NETBUF_DATA(skb))->info
                                     >> WMI_DATA_HDR_PS_SHIFT) & WMI_DATA_HDR_PS_MASK;
                        datap = (ATH_MAC_HDR *)(A_NETBUF_DATA(skb)+sizeof(WMI_DATA_HDR));
                        conn = ieee80211_find_conn(ar, datap->srcMac);


                        if (conn) {
                            /* if there is a change in PS state of the STA,
                             * take appropriate steps.
                             * 1. If Sleep-->Awake, flush the psq for the STA
                             *    Clear the PVB for the STA.
                             * 2. If Awake-->Sleep, Starting queueing frames
                             * the STA.
                             */
                            prevPsState = STA_IS_PWR_SLEEP(conn);
                            if (psState) {
                                STA_SET_PWR_SLEEP(conn);
                            } else {
                                STA_CLR_PWR_SLEEP(conn);
                            }

                            if (prevPsState ^ STA_IS_PWR_SLEEP(conn)) {

                                if (!STA_IS_PWR_SLEEP(conn)) {

                                    A_MUTEX_LOCK(&conn->psqLock);
                                    while (!A_NETBUF_QUEUE_EMPTY(&conn->psq)) {
                                        struct sk_buff *skb=NULL;

                                        skb = A_NETBUF_DEQUEUE(&conn->psq);
                                        A_MUTEX_UNLOCK(&conn->psqLock);
                                        ar6000_data_tx(skb,ar->arNetDev);
                                        A_MUTEX_LOCK(&conn->psqLock);
                                    }
                                    A_MUTEX_UNLOCK(&conn->psqLock);
                                    /* Clear the PVB for this STA */
                                    wmi_set_pvb_cmd(ar->arWmi, conn->aid, 0);
                                }
                            }
                        } else {
                            /* This frame is from a STA that is not associated*/
                            A_ASSERT(FALSE);
                        }

                        /* Drop NULL data frames here */
                        if((pPacket->ActualLength < minHdrLen) ||
                                (pPacket->ActualLength > AR6000_BUFFER_SIZE)) {
                            A_NETBUF_FREE(skb);
                            goto refill;
                        }
                    }

                    wmi_data_hdr_remove(ar->arWmi, skb);
                    /* NWF: print the 802.11 hdr bytes */
                    if(processDot11Hdr) {
                        wmi_dot11_hdr_remove(ar->arWmi,skb);
                    } else {
                        wmi_dot3_2_dix(ar->arWmi, skb);
                    }

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,0)
                    /*
                     * extra push and memcpy, for eth_type_trans() of 2.4 kernel
                     * will pull out hard_header_len bytes of the skb.
                     */
                    A_NETBUF_PUSH(skb, sizeof(WMI_DATA_HDR) + sizeof(ATH_LLC_SNAP_HDR) + HTC_HEADER_LEN);
                    A_MEMCPY(A_NETBUF_DATA(skb), A_NETBUF_DATA(skb) + sizeof(WMI_DATA_HDR) +
                             sizeof(ATH_LLC_SNAP_HDR) + HTC_HEADER_LEN, sizeof(ATH_MAC_HDR));
#endif
                    if ((ar->arNetDev->flags & IFF_UP) == IFF_UP) {
                        if (ar->arNetworkType == AP_NETWORK) {
                            struct sk_buff *skb1 = NULL;
                            ATH_MAC_HDR *datap;

                            datap = (ATH_MAC_HDR *)A_NETBUF_DATA(skb);
                            if (IEEE80211_IS_MULTICAST(datap->dstMac)) {
                                /* Bcast/Mcast frames should be sent to the OS
                                 * stack as well as on the air.
                                 */
                                skb1 = skb_copy(skb,GFP_ATOMIC);
                            } else {
                                /* Search for a connected STA with dstMac as
                                 * the Mac address. If found send the frame to
                                 * it on the air else send the frame up the
                                 * stack
                                 */
                                sta_t *conn = NULL;
                                conn = ieee80211_find_conn(ar, datap->dstMac);

                                if (conn && ar->intra_bss) {
                                    skb1 = skb;
                                    skb = NULL;
                                } else if(conn && !ar->intra_bss) {
                                    A_NETBUF_FREE(skb);
                                    skb = NULL;
                                }
                            }
                            if (skb1) {
                                ar6000_data_tx(skb1, ar->arNetDev);
                            }
                        }
                    }
                    deliver_frames_to_nw_stack(skb);
                }
            }
    } else {
        deliver_frames_to_nw_stack(skb);
    }
refill:
    if (status != A_ECANCELED) {
        /*
         * HTC provides A_ECANCELED status when it doesn't want to be refilled
         * (probably due to a shutdown)
         */
        ar6000_rx_refill(Context, ept);
    }


}

static void
deliver_frames_to_nw_stack(struct sk_buff *skb)
{
    if(skb) {
        if ((skb->dev->flags & IFF_UP) == IFF_UP) {
            skb->protocol = eth_type_trans(skb, skb->dev);
            netif_rx(skb);
        } else {
            A_NETBUF_FREE(skb);
        }
    }
}


static void
ar6000_rx_refill(void *Context, HTC_ENDPOINT_ID Endpoint)
{
    AR_SOFTC_T  *ar = (AR_SOFTC_T *)Context;
    void        *osBuf;
    int         RxBuffers;
    int         buffersToRefill;
    HTC_PACKET  *pPacket;

    buffersToRefill = (int)AR6000_MAX_RX_BUFFERS -
                                    (int)ar->arRxBuffers[Endpoint];

    if (buffersToRefill <= 0) {
            /* fast return, nothing to fill */
        return;
    }

    AR_DEBUG2_PRINTF("ar6000_rx_refill: providing htc with %d buffers at eid=%d\n",
                    buffersToRefill, Endpoint);

    for (RxBuffers = 0; RxBuffers < buffersToRefill; RxBuffers++) {
        osBuf = A_NETBUF_ALLOC(AR6000_BUFFER_SIZE);
        if (NULL == osBuf) {
            break;
        }
            /* the HTC packet wrapper is at the head of the reserved area
             * in the skb */
        pPacket = (HTC_PACKET *)(A_NETBUF_HEAD(osBuf));
            /* set re-fill info */
        SET_HTC_PACKET_INFO_RX_REFILL(pPacket,osBuf,A_NETBUF_DATA(osBuf),AR6000_BUFFER_SIZE,Endpoint);
            /* add this packet */
        HTCAddReceivePkt(ar->arHtcTarget, pPacket);
    }

        /* update count */
    AR6000_SPIN_LOCK(&ar->arLock, 0);
    ar->arRxBuffers[Endpoint] += RxBuffers;
    AR6000_SPIN_UNLOCK(&ar->arLock, 0);
}

static struct net_device_stats *
ar6000_get_stats(struct net_device *dev)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);
    return &ar->arNetStats;
}

static struct iw_statistics *
ar6000_get_iwstats(struct net_device * dev)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)netdev_priv(dev);
    TARGET_STATS *pStats = &ar->arTargetStats;
    struct iw_statistics * pIwStats = &ar->arIwStats;

    if (ar->bIsDestroyProgress || ar->arWmiReady == FALSE)
    {
        pIwStats->status = 0;
        pIwStats->qual.qual = 0;
        pIwStats->qual.level =0;
        pIwStats->qual.noise = 0;
        pIwStats->discard.code =0;
        pIwStats->discard.retries=0;
        pIwStats->miss.beacon =0;
        return pIwStats;
    }
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
    /*
     * The in_atomic function is used to determine if the scheduling is
     * allowed in the current context or not. This was introduced in 2.6
     * From what I have read on the differences between 2.4 and 2.6, the
     * 2.4 kernel did not support preemption and so this check might not
     * be required for 2.4 kernels.
     */
    if (in_atomic())
    {
        wmi_get_stats_cmd(ar->arWmi);

        pIwStats->status = 1 ;
        pIwStats->qual.qual = pStats->cs_aveBeacon_rssi - 161;
        pIwStats->qual.level =pStats->cs_aveBeacon_rssi; /* noise is -95 dBm */
        pIwStats->qual.noise = pStats->noise_floor_calibation;
        pIwStats->discard.code = pStats->rx_decrypt_err;
        pIwStats->discard.retries = pStats->tx_retry_cnt;
        pIwStats->miss.beacon = pStats->cs_bmiss_cnt;
        return pIwStats;
    }
#endif /* LINUX_VERSION_CODE */

    if (down_interruptible(&ar->arSem)) {
        pIwStats->status = 0;
        return pIwStats;
    }

    if (ar->bIsDestroyProgress) {
        up(&ar->arSem);
        pIwStats->status = 0;
        return pIwStats;
    }

    ar->statsUpdatePending = TRUE;

    if(wmi_get_stats_cmd(ar->arWmi) != A_OK) {
        up(&ar->arSem);
        pIwStats->status = 0;
        return pIwStats;
    }

    wait_event_interruptible_timeout(arEvent, ar->statsUpdatePending == FALSE, wmitimeout * HZ);

    if (signal_pending(current)) {
        AR_DEBUG_PRINTF("ar6000 : WMI get stats timeout \n");
        up(&ar->arSem);
        pIwStats->status = 0;
        return pIwStats;
    }
    pIwStats->status = 1 ;
    pIwStats->qual.qual = pStats->cs_aveBeacon_rssi - 161;
    pIwStats->qual.level =pStats->cs_aveBeacon_rssi;  /* noise is -95 dBm */
    pIwStats->qual.noise = pStats->noise_floor_calibation;
    pIwStats->discard.code = pStats->rx_decrypt_err;
    pIwStats->discard.retries = pStats->tx_retry_cnt;
    pIwStats->miss.beacon = pStats->cs_bmiss_cnt;
    up(&ar->arSem);
    return pIwStats;
}

void
ar6000_ready_event(void *devt, A_UINT8 *datap, A_UINT8 phyCap, A_UINT32 vers)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)devt;
    struct net_device *dev = ar->arNetDev;

    ar->arWmiReady = TRUE;
    wake_up(&arEvent);
    A_MEMCPY(dev->dev_addr, datap, AR6000_ETH_ADDR_LEN);
    AR_DEBUG_PRINTF("mac address = %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x\n",
        dev->dev_addr[0], dev->dev_addr[1],
        dev->dev_addr[2], dev->dev_addr[3],
        dev->dev_addr[4], dev->dev_addr[5]);

    ar->arPhyCapability = phyCap;
    ar->arVersion.wlan_ver = vers;
}

A_UINT8
add_new_sta(AR_SOFTC_T *ar, A_UINT8 *mac, A_UINT16 aid, A_UINT8 *wpaie, A_UINT8 ielen)
{
    A_INT8    free_slot=-1, i;

    for(i=0; i < AP_MAX_NUM_STA; i++) {
        if(A_MEMCMP(ar->sta_list[i].mac, mac, ATH_MAC_LEN)==0) {
            /* it is already available */
            return 0;
        }

        if(!((1 << i) & ar->sta_list_index)) {
            free_slot = i;
            break;
        }
    }

    if(free_slot >= 0) {
        A_MEMCPY(ar->sta_list[free_slot].mac, mac, ATH_MAC_LEN);
        A_MEMCPY(ar->sta_list[free_slot].wpa_ie, wpaie, ielen);
        ar->sta_list[free_slot].aid = aid;
        ar->sta_list_index = ar->sta_list_index | (1 << free_slot);
        return 1;
    }
    return 0; /* not added */
}

void
ar6000_connect_event(AR_SOFTC_T *ar, A_UINT16 channel, A_UINT8 *bssid,
                     A_UINT16 listenInterval, A_UINT16 beaconInterval,
                     NETWORK_TYPE networkType, A_UINT8 beaconIeLen,
                     A_UINT8 assocReqLen, A_UINT8 assocRespLen,
                     A_UINT8 *assocInfo)
{
    union iwreq_data wrqu;
    int i, beacon_ie_pos, assoc_resp_ie_pos, assoc_req_ie_pos;
    static const char *tag1 = "ASSOCINFO(ReqIEs=";
    static const char *tag2 = "ASSOCRESPIE=";
    static const char *beaconIetag = "BEACONIE=";
    char buf[WMI_CONTROL_MSG_MAX_LEN * 2 + strlen(tag1) + 1];
    char *pos;
    A_UINT8 key_op_ctrl;
    unsigned long flags;

    if(ar->arNetworkType & AP_NETWORK) {
    A_PRINTF("NEW STA %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x "
            " aid=%d WPAIE=%d\n", bssid[0], bssid[1], bssid[2],
             bssid[3], bssid[4], bssid[5], channel, assocRespLen);
        add_new_sta(ar, bssid, channel /*aid*/,
            assocInfo /* WPA IE */, assocRespLen /* IE len */);

        /* Send event to application */
        A_MEMZERO(&wrqu, sizeof(wrqu));
        A_MEMCPY(wrqu.addr.sa_data, bssid, ATH_MAC_LEN);
        wireless_send_event(ar->arNetDev, IWEVREGISTERED, &wrqu, NULL);
        /* In case the queue is stopped when we switch modes, this will
         * wake it up
         */
        netif_wake_queue(ar->arNetDev);
        return;
    }

/* ATHENV */
# if 0 /* pseudo code */
#ifdef ANDROID_ENV
    msmsdcc_wlan_pwr_ctrl = WLAN_PWR_CTRL_WOW;
    printk("WLAN Connect: change to WOW.\n");
#endif
#endif
/* ATHENV */

    if (FALSE == ar->arConnected &&
        ((WPA_PSK_AUTH == ar->arAuthMode) || (WPA2_PSK_AUTH == ar->arAuthMode)))
    {
	AR_DEBUG_PRINTF("%s(), start disconnect timer %d\n", __func__, __LINE__);
        A_TIMEOUT_MS(&ar->disconnect_timer, A_DISCONNECT_TIMER_INTERVAL, 0);
    }

    A_MEMCPY(ar->arBssid, bssid, sizeof(ar->arBssid));
    ar->arBssChannel = channel;

    A_PRINTF("AR6000 connected event on freq %d ", channel);
    A_PRINTF("with bssid %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x "
            " listenInterval=%d, beaconInterval = %d, beaconIeLen = %d assocReqLen=%d"
            " assocRespLen =%d\n",
             bssid[0], bssid[1], bssid[2],
             bssid[3], bssid[4], bssid[5],
             listenInterval, beaconInterval,
             beaconIeLen, assocReqLen, assocRespLen);
    if (networkType & ADHOC_NETWORK) {
        if (networkType & ADHOC_CREATOR) {
            A_PRINTF("Network: Adhoc (Creator)\n");
        } else {
            A_PRINTF("Network: Adhoc (Joiner)\n");
        }
    } else {
        A_PRINTF("Network: Infrastructure\n");
    }

    if (beaconIeLen && (sizeof(buf) > (9 + beaconIeLen * 2))) {
        AR_DEBUG_PRINTF("\nBeaconIEs= ");

        beacon_ie_pos = 0;
        A_MEMZERO(buf, sizeof(buf));
        sprintf(buf, "%s", beaconIetag);
        pos = buf + 9;
        for (i = beacon_ie_pos; i < beacon_ie_pos + beaconIeLen; i++) {
            AR_DEBUG_PRINTF("%2.2x ", assocInfo[i]);
            sprintf(pos, "%2.2x", assocInfo[i]);
            pos += 2;
        }
        AR_DEBUG_PRINTF("\n");

        A_MEMZERO(&wrqu, sizeof(wrqu));
        wrqu.data.length = strlen(buf);
        wireless_send_event(ar->arNetDev, IWEVCUSTOM, &wrqu, buf);
    }

    if (assocRespLen && (sizeof(buf) > (12 + (assocRespLen * 2))))
    {
        assoc_resp_ie_pos = beaconIeLen + assocReqLen +
                            sizeof(A_UINT16)  +  /* capinfo*/
                            sizeof(A_UINT16)  +  /* status Code */
                            sizeof(A_UINT16)  ;  /* associd */
        A_MEMZERO(buf, sizeof(buf));
        sprintf(buf, "%s", tag2);
        pos = buf + 12;
        AR_DEBUG_PRINTF("\nAssocRespIEs= ");
        /*
         * The Association Response Frame w.o. the WLAN header is delivered to
         * the host, so skip over to the IEs
         */
        for (i = assoc_resp_ie_pos; i < assoc_resp_ie_pos + assocRespLen - 6; i++)
        {
            AR_DEBUG_PRINTF("%2.2x ", assocInfo[i]);
            sprintf(pos, "%2.2x", assocInfo[i]);
            pos += 2;
        }
        AR_DEBUG_PRINTF("\n");

        A_MEMZERO(&wrqu, sizeof(wrqu));
        wrqu.data.length = strlen(buf);
        wireless_send_event(ar->arNetDev, IWEVCUSTOM, &wrqu, buf);
    }

    if (assocReqLen && (sizeof(buf) > (17 + (assocReqLen * 2)))) {
        /*
         * assoc Request includes capability and listen interval. Skip these.
         */
        assoc_req_ie_pos =  beaconIeLen +
                            sizeof(A_UINT16)  +  /* capinfo*/
                            sizeof(A_UINT16);    /* listen interval */

        A_MEMZERO(buf, sizeof(buf));
        sprintf(buf, "%s", tag1);
        pos = buf + 17;
        AR_DEBUG_PRINTF("AssocReqIEs= ");
        for (i = assoc_req_ie_pos; i < assoc_req_ie_pos + assocReqLen - 4; i++) {
            AR_DEBUG_PRINTF("%2.2x ", assocInfo[i]);
            sprintf(pos, "%2.2x", assocInfo[i]);
            pos += 2;;
        }
        AR_DEBUG_PRINTF("\n");

        A_MEMZERO(&wrqu, sizeof(wrqu));
        wrqu.data.length = strlen(buf);
        wireless_send_event(ar->arNetDev, IWEVCUSTOM, &wrqu, buf);
    }

#ifdef USER_KEYS
    if (ar->user_savedkeys_stat == USER_SAVEDKEYS_STAT_RUN &&
        ar->user_saved_keys.keyOk == TRUE)
    {
        key_op_ctrl = KEY_OP_VALID_MASK & ~KEY_OP_INIT_TSC;

        if (ar->user_key_ctrl & AR6000_USER_SETKEYS_RSC_UNCHANGED) {
            key_op_ctrl &= ~KEY_OP_INIT_RSC;
        } else {
            key_op_ctrl |= KEY_OP_INIT_RSC;
        }
        ar6000_reinstall_keys(ar, key_op_ctrl);
    }
#endif /* USER_KEYS */

    netif_wake_queue(ar->arNetDev);

    if ((networkType & ADHOC_NETWORK)      &&
        (OPEN_AUTH == ar->arDot11AuthMode) &&
        (NONE_AUTH == ar->arAuthMode)      &&
        (WEP_CRYPT == ar->arPairwiseCrypto))
    {
        if (!ar->arConnected) {
            wmi_addKey_cmd(ar->arWmi,
                           ar->arDefTxKeyIndex,
                           WEP_CRYPT,
                           GROUP_USAGE | TX_USAGE,
                           ar->arWepKeyList[ar->arDefTxKeyIndex].arKeyLen,
                           NULL,
                           ar->arWepKeyList[ar->arDefTxKeyIndex].arKey, KEY_OP_INIT_VAL, NULL,
                           NO_SYNC_WMIFLAG);
        }
    }

    /* Update connect & link status atomically */
    spin_lock_irqsave(&ar->arLock, flags);
    ar->arConnected  = TRUE;
    ar->arConnectPending = FALSE;
    netif_carrier_on(ar->arNetDev);
    spin_unlock_irqrestore(&ar->arLock, flags);

    reconnect_flag = 0;

    A_MEMZERO(&wrqu, sizeof(wrqu));
    A_MEMCPY(wrqu.addr.sa_data, bssid, IEEE80211_ADDR_LEN);
    wrqu.addr.sa_family = ARPHRD_ETHER;
    wireless_send_event(ar->arNetDev, SIOCGIWAP, &wrqu, NULL);
    if ((ar->arNetworkType == ADHOC_NETWORK) && ar->arIbssPsEnable) {
        A_MEMZERO(ar->arNodeMap, sizeof(ar->arNodeMap));
        ar->arNodeNum = 0;
        ar->arNexEpId = ENDPOINT_2;
    }
}

void ar6000_set_numdataendpts(AR_SOFTC_T *ar, A_UINT32 num)
{
    A_ASSERT(num <= (HTC_MAILBOX_NUM_MAX - 1));
    ar->arNumDataEndPts = num;
}

void
sta_cleanup(AR_SOFTC_T *ar, A_UINT8 i)
{
    struct sk_buff *skb;
    
    /* empty the queued pkts in the PS queue if any */
    A_MUTEX_LOCK(&ar->sta_list[i].psqLock);
    while (!A_NETBUF_QUEUE_EMPTY(&ar->sta_list[i].psq)) {
        skb = A_NETBUF_DEQUEUE(&ar->sta_list[i].psq);
        A_NETBUF_FREE(skb);
    }
    A_MUTEX_UNLOCK(&ar->sta_list[i].psqLock);

    /* Zero out the state fields */
    A_MEMZERO(&ar->sta_list[i].mac, ATH_MAC_LEN);
    A_MEMZERO(&ar->sta_list[i].wpa_ie, IEEE80211_MAX_IE);
    ar->sta_list[i].aid = 0;
    ar->sta_list[i].flags = 0;

    ar->sta_list_index = ar->sta_list_index & ~(1 << i);
    
}

A_UINT8
remove_sta(AR_SOFTC_T *ar, A_UINT8 *mac, A_UINT16 reason)
{
    A_UINT8 i, removed=0;

    if(IS_MAC_NULL(mac)) {
        return removed;
    }

    if(IS_MAC_BCAST(mac)) {
        A_PRINTF("DEL ALL STA\n");
        for(i=0; i < AP_MAX_NUM_STA; i++) {
            if(!IS_MAC_NULL(ar->sta_list[i].mac)) {
                sta_cleanup(ar, i);
                removed = 1;
            }
        }
    } else {
        for(i=0; i < AP_MAX_NUM_STA; i++) {
            if(A_MEMCMP(ar->sta_list[i].mac, mac, ATH_MAC_LEN)==0) {
                A_PRINTF("DEL STA %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x "
                " aid=%d REASON=%d\n", mac[0], mac[1], mac[2],
                 mac[3], mac[4], mac[5], ar->sta_list[i].aid, reason);

                sta_cleanup(ar, i);
                removed = 1;
                break;
            }
        }
    }
    return removed;
}

void
ar6000_disconnect_event(AR_SOFTC_T *ar, A_UINT8 reason, A_UINT8 *bssid,
                        A_UINT8 assocRespLen, A_UINT8 *assocInfo, A_UINT16 protocolReasonStatus)
{
    A_UINT8 i;
    unsigned long flags;

    if(ar->arNetworkType & AP_NETWORK) {
        union iwreq_data wrqu;
        struct sk_buff *skb;

        if(!remove_sta(ar, bssid, protocolReasonStatus)) {
            return;
        }

        /* If there are no more associated STAs, empty the mcast PS q */
        if (ar->sta_list_index == 0) {
            A_MUTEX_LOCK(&ar->mcastpsqLock);
            while (!A_NETBUF_QUEUE_EMPTY(&ar->mcastpsq)) {
                skb = A_NETBUF_DEQUEUE(&ar->mcastpsq);
                A_NETBUF_FREE(skb);
            }
            A_MUTEX_UNLOCK(&ar->mcastpsqLock);

            /* Clear the LSB of the BitMapCtl field of the TIM IE */
            wmi_set_pvb_cmd(ar->arWmi, MCAST_AID, 0);
        }

        if(!IS_MAC_BCAST(bssid)) {
            /* Send event to application */
            A_MEMZERO(&wrqu, sizeof(wrqu));
            A_MEMCPY(wrqu.addr.sa_data, bssid, ATH_MAC_LEN);
            wireless_send_event(ar->arNetDev, IWEVEXPIRED, &wrqu, NULL);
        }
        return;
    }

    if (NO_NETWORK_AVAIL != reason)
    {
        union iwreq_data wrqu;
/* ATHENV */
        A_MEMCPY(wrqu.addr.sa_data, "\x00\x00\x00\x00\x00\x00", IEEE80211_ADDR_LEN);
        A_MEMZERO(&wrqu, sizeof(wrqu));
/* ATHENV */
        wrqu.addr.sa_family = ARPHRD_ETHER;

        /* Send disconnect event to supplicant */
        wireless_send_event(ar->arNetDev, SIOCGIWAP, &wrqu, NULL);
/* ATHENV */
# if 0 /* pseudo code */
#ifdef ANDROID_ENV
        msmsdcc_wlan_pwr_ctrl = WLAN_PWR_CTRL_CUT_PWR;
        printk("WLAN Connect: change to cut-power.\n");
#endif
#endif
/* ATHENV */
    }

    A_UNTIMEOUT(&ar->disconnect_timer);

    A_PRINTF("AR6000 disconnected");
    if (bssid[0] || bssid[1] || bssid[2] || bssid[3] || bssid[4] || bssid[5]) {
        A_PRINTF(" from %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x ",
                 bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
    }

    AR_DEBUG_PRINTF("\nDisconnect Reason is %d", reason);
    AR_DEBUG_PRINTF("\nProtocol Reason/Status Code is %d", protocolReasonStatus);
    AR_DEBUG_PRINTF("\nAssocResp Frame = %s",
                    assocRespLen ? " " : "NULL");
    for (i = 0; i < assocRespLen; i++) {
        if (!(i % 0x10)) {
            AR_DEBUG_PRINTF("\n");
        }
        AR_DEBUG_PRINTF("%2.2x ", assocInfo[i]);
    }
    AR_DEBUG_PRINTF("\n");
    /*
     * If the event is due to disconnect cmd from the host, only they the target
     * would stop trying to connect. Under any other condition, target would
     * keep trying to connect.
     *
     */
    if( reason == DISCONNECT_CMD)
    {
        ar->arConnectPending = FALSE;
    } else {
        ar->arConnectPending = TRUE;
        if (((reason == ASSOC_FAILED) && (protocolReasonStatus == 0x11)) ||
            ((reason == ASSOC_FAILED) && (protocolReasonStatus == 0x0) && (reconnect_flag == 1))) {
            ar->arConnected = TRUE;
            return;
        }
    }

    if (reason == NO_NETWORK_AVAIL)
    {
        bss_t *pWmiSsidnode = NULL;

        /* remove the current associated bssid node */
        wmi_free_node (ar->arWmi, bssid);

        /*
         * In case any other same SSID nodes are present
         * remove it, since those nodes also not available now
         */
        do
        {
            /*
             * Find the nodes based on SSID and remove it
             * NOTE :: This case will not work out for Hidden-SSID
             */
            pWmiSsidnode = wmi_find_Ssidnode (ar->arWmi, ar->arSsid, ar->arSsidLen, FALSE, TRUE);

            if (pWmiSsidnode)
            {
                wmi_free_node (ar->arWmi, pWmiSsidnode->ni_macaddr);
            }

        }while (pWmiSsidnode);

        ar6000_init_profile_info(ar);
        wmi_disconnect_cmd(ar->arWmi);
	//ZTE_WIFI_HP_012 
	//work around for disconnect cmd, we need to update the 
	//arConnected flag 
	ar->arConnected = FALSE;
	//ZTE_WIFI_HP_012 end 
    }

    /* Update connect & link status atomically */
    spin_lock_irqsave(&ar->arLock, flags);
    ar->arConnected = FALSE;
    netif_carrier_off(ar->arNetDev);
    spin_unlock_irqrestore(&ar->arLock, flags);

    if( (reason != CSERV_DISCONNECT) || (reconnect_flag != 1) ) {
        reconnect_flag = 0;
    }

#ifdef USER_KEYS
    if (reason != CSERV_DISCONNECT)
    {
        ar->user_savedkeys_stat = USER_SAVEDKEYS_STAT_INIT;
        ar->user_key_ctrl      = 0;
    }
#endif /* USER_KEYS */

    netif_stop_queue(ar->arNetDev);
    A_MEMZERO(ar->arBssid, sizeof(ar->arBssid));
    ar->arBssChannel = 0;
    ar->arBeaconInterval = 0;

    ar6000_TxDataCleanup(ar);
}

void
ar6000_regDomain_event(AR_SOFTC_T *ar, A_UINT32 regCode)
{
    A_PRINTF("AR6000 Reg Code = 0x%x\n", regCode);
    ar->arRegCode = regCode;
}

void
ar6000_neighborReport_event(AR_SOFTC_T *ar, int numAps, WMI_NEIGHBOR_INFO *info)
{
    static const char *tag = "PRE-AUTH";
    char buf[128];
    union iwreq_data wrqu;
    int i;

    AR_DEBUG_PRINTF("AR6000 Neighbor Report Event\n");
    for (i=0; i < numAps; info++, i++) {
        AR_DEBUG_PRINTF("bssid %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x ",
            info->bssid[0], info->bssid[1], info->bssid[2],
            info->bssid[3], info->bssid[4], info->bssid[5]);
        if (info->bssFlags & WMI_PREAUTH_CAPABLE_BSS) {
            AR_DEBUG_PRINTF("preauth-cap");
        }
        if (info->bssFlags & WMI_PMKID_VALID_BSS) {
            AR_DEBUG_PRINTF(" pmkid-valid\n");
            continue;           /* we skip bss if the pmkid is already valid */
        }
        AR_DEBUG_PRINTF("\n");
        snprintf(buf, sizeof(buf), "%s%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x",
                 tag,
                 info->bssid[0], info->bssid[1], info->bssid[2],
                 info->bssid[3], info->bssid[4], info->bssid[5],
                 i, info->bssFlags);
        A_MEMZERO(&wrqu, sizeof(wrqu));
        wrqu.data.length = strlen(buf);
	//ZTE_WIFI_HP_008
	//decrease event report
        /*wireless_send_event(ar->arNetDev, IWEVCUSTOM, &wrqu, buf);*/
	//ZTE_WIFI_HP_008 end 
    }
}

void
ar6000_tkip_micerr_event(AR_SOFTC_T *ar, A_UINT8 keyid, A_BOOL ismcast)
{
    static const char *tag = "MLME-MICHAELMICFAILURE.indication";
    char buf[128];
    union iwreq_data wrqu;

    /*
     * For AP case, keyid will have aid of STA which sent pkt with
     * MIC error. Use this aid to get MAC & send it to hostapd.
     */
    if (ar->arNetworkType == AP_NETWORK) {
        sta_t *s = ieee80211_find_conn_for_aid(ar, keyid);
        A_PRINTF("AP TKIP MIC error received from aid=%d\n", keyid);
        snprintf(buf,sizeof(buf), "%s addr=%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x",
            tag, s->mac[0],s->mac[1],s->mac[2],s->mac[3],s->mac[4],s->mac[5]);
    } else {
    A_PRINTF("AR6000 TKIP MIC error received for keyid %d %scast\n",
             keyid, ismcast ? "multi": "uni");
    snprintf(buf, sizeof(buf), "%s(keyid=%d %sicast)", tag, keyid,
             ismcast ? "mult" : "un");
    }

    memset(&wrqu, 0, sizeof(wrqu));
    wrqu.data.length = strlen(buf);
    wireless_send_event(ar->arNetDev, IWEVCUSTOM, &wrqu, buf);
}

void
ar6000_scanComplete_event(AR_SOFTC_T *ar, A_STATUS status)
{
	//ZTE_WIFI_HP_006 2009-12-29 for speed up wifi search ssid
    union iwreq_data wrqu;

    AR_DEBUG_PRINTF("AR6000 scan complete: %d\n", status);
    ar->scan_complete = 1;

    A_MEMZERO(&wrqu, sizeof(wrqu));
    wrqu.addr.sa_family = ARPHRD_ETHER;
    wireless_send_event(ar->arNetDev, SIOCGIWSCAN, &wrqu, NULL);
    //ZTE_WIFI_HP_006 end 

    wake_up_interruptible(&ar6000_scan_queue);
}

void
ar6000_targetStats_event(AR_SOFTC_T *ar,  WMI_TARGET_STATS *pTarget)
{
    TARGET_STATS *pStats = &ar->arTargetStats;
    A_UINT8 ac;

    A_PRINTF("AR6000 updating target stats\n");
    pStats->tx_packets          += pTarget->txrxStats.tx_stats.tx_packets;
    pStats->tx_bytes            += pTarget->txrxStats.tx_stats.tx_bytes;
    pStats->tx_unicast_pkts     += pTarget->txrxStats.tx_stats.tx_unicast_pkts;
    pStats->tx_unicast_bytes    += pTarget->txrxStats.tx_stats.tx_unicast_bytes;
    pStats->tx_multicast_pkts   += pTarget->txrxStats.tx_stats.tx_multicast_pkts;
    pStats->tx_multicast_bytes  += pTarget->txrxStats.tx_stats.tx_multicast_bytes;
    pStats->tx_broadcast_pkts   += pTarget->txrxStats.tx_stats.tx_broadcast_pkts;
    pStats->tx_broadcast_bytes  += pTarget->txrxStats.tx_stats.tx_broadcast_bytes;
    pStats->tx_rts_success_cnt  += pTarget->txrxStats.tx_stats.tx_rts_success_cnt;
    for(ac = 0; ac < WMM_NUM_AC; ac++)
        pStats->tx_packet_per_ac[ac] += pTarget->txrxStats.tx_stats.tx_packet_per_ac[ac];
    pStats->tx_errors           += pTarget->txrxStats.tx_stats.tx_errors;
    pStats->tx_failed_cnt       += pTarget->txrxStats.tx_stats.tx_failed_cnt;
    pStats->tx_retry_cnt        += pTarget->txrxStats.tx_stats.tx_retry_cnt;
    pStats->tx_mult_retry_cnt   += pTarget->txrxStats.tx_stats.tx_mult_retry_cnt;
    pStats->tx_rts_fail_cnt     += pTarget->txrxStats.tx_stats.tx_rts_fail_cnt;
    pStats->tx_unicast_rate      = wmi_get_rate(pTarget->txrxStats.tx_stats.tx_unicast_rate);

    pStats->rx_packets          += pTarget->txrxStats.rx_stats.rx_packets;
    pStats->rx_bytes            += pTarget->txrxStats.rx_stats.rx_bytes;
    pStats->rx_unicast_pkts     += pTarget->txrxStats.rx_stats.rx_unicast_pkts;
    pStats->rx_unicast_bytes    += pTarget->txrxStats.rx_stats.rx_unicast_bytes;
    pStats->rx_multicast_pkts   += pTarget->txrxStats.rx_stats.rx_multicast_pkts;
    pStats->rx_multicast_bytes  += pTarget->txrxStats.rx_stats.rx_multicast_bytes;
    pStats->rx_broadcast_pkts   += pTarget->txrxStats.rx_stats.rx_broadcast_pkts;
    pStats->rx_broadcast_bytes  += pTarget->txrxStats.rx_stats.rx_broadcast_bytes;
    pStats->rx_fragment_pkt     += pTarget->txrxStats.rx_stats.rx_fragment_pkt;
    pStats->rx_errors           += pTarget->txrxStats.rx_stats.rx_errors;
    pStats->rx_crcerr           += pTarget->txrxStats.rx_stats.rx_crcerr;
    pStats->rx_key_cache_miss   += pTarget->txrxStats.rx_stats.rx_key_cache_miss;
    pStats->rx_decrypt_err      += pTarget->txrxStats.rx_stats.rx_decrypt_err;
    pStats->rx_duplicate_frames += pTarget->txrxStats.rx_stats.rx_duplicate_frames;
    pStats->rx_unicast_rate      = wmi_get_rate(pTarget->txrxStats.rx_stats.rx_unicast_rate);


    pStats->tkip_local_mic_failure
                                += pTarget->txrxStats.tkipCcmpStats.tkip_local_mic_failure;
    pStats->tkip_counter_measures_invoked
                                += pTarget->txrxStats.tkipCcmpStats.tkip_counter_measures_invoked;
    pStats->tkip_replays        += pTarget->txrxStats.tkipCcmpStats.tkip_replays;
    pStats->tkip_format_errors  += pTarget->txrxStats.tkipCcmpStats.tkip_format_errors;
    pStats->ccmp_format_errors  += pTarget->txrxStats.tkipCcmpStats.ccmp_format_errors;
    pStats->ccmp_replays        += pTarget->txrxStats.tkipCcmpStats.ccmp_replays;


    pStats->power_save_failure_cnt += pTarget->pmStats.power_save_failure_cnt;
    pStats->noise_floor_calibation = pTarget->noise_floor_calibation;

    pStats->cs_bmiss_cnt        += pTarget->cservStats.cs_bmiss_cnt;
    pStats->cs_lowRssi_cnt      += pTarget->cservStats.cs_lowRssi_cnt;
    pStats->cs_connect_cnt      += pTarget->cservStats.cs_connect_cnt;
    pStats->cs_disconnect_cnt   += pTarget->cservStats.cs_disconnect_cnt;
    pStats->cs_aveBeacon_snr    = pTarget->cservStats.cs_aveBeacon_snr;
    pStats->cs_aveBeacon_rssi   = pTarget->cservStats.cs_aveBeacon_rssi;
    pStats->cs_lastRoam_msec    = pTarget->cservStats.cs_lastRoam_msec;
    pStats->cs_snr              = pTarget->cservStats.cs_snr;
    pStats->cs_rssi             = pTarget->cservStats.cs_rssi;

    pStats->lq_val              = pTarget->lqVal;

    pStats->wow_num_pkts_dropped += pTarget->wowStats.wow_num_pkts_dropped;
    pStats->wow_num_host_pkt_wakeups += pTarget->wowStats.wow_num_host_pkt_wakeups;
    pStats->wow_num_host_event_wakeups += pTarget->wowStats.wow_num_host_event_wakeups;
    pStats->wow_num_events_discarded += pTarget->wowStats.wow_num_events_discarded;

    pStats->arp_received += pTarget->arpStats.arp_received;
    pStats->arp_matched  += pTarget->arpStats.arp_matched;
    pStats->arp_replied  += pTarget->arpStats.arp_replied;

    ar->statsUpdatePending = FALSE;
    wake_up(&arEvent);
}

void
ar6000_rssiThreshold_event(AR_SOFTC_T *ar,  WMI_RSSI_THRESHOLD_VAL newThreshold, A_INT16 rssi)
{
    USER_RSSI_THOLD userRssiThold;

    /* Send an event to the app */
    userRssiThold.tag = ar->rssi_map[newThreshold].tag;
    userRssiThold.rssi = rssi + SIGNAL_QUALITY_NOISE_FLOOR;
    A_PRINTF("rssi Threshold range = %d tag = %d  rssi = %d\n", newThreshold,
             userRssiThold.tag, userRssiThold.rssi);

    ar6000_send_event_to_app(ar, WMI_RSSI_THRESHOLD_EVENTID,(A_UINT8 *)&userRssiThold, sizeof(USER_RSSI_THOLD));
}


void
ar6000_hbChallengeResp_event(AR_SOFTC_T *ar, A_UINT32 cookie, A_UINT32 source)
{
    if (source == APP_HB_CHALLENGE) {
        /* Report it to the app in case it wants a positive acknowledgement */
        ar6000_send_event_to_app(ar, WMIX_HB_CHALLENGE_RESP_EVENTID,
                                 (A_UINT8 *)&cookie, sizeof(cookie));
    } else {
        /* This would ignore the replys that come in after their due time */
        if (cookie == ar->arHBChallengeResp.seqNum) {
            ar->arHBChallengeResp.outstanding = FALSE;
        }
    }
}


void
ar6000_reportError_event(AR_SOFTC_T *ar, WMI_TARGET_ERROR_VAL errorVal)
{
    char    *errString[] = {
                [WMI_TARGET_PM_ERR_FAIL]    "WMI_TARGET_PM_ERR_FAIL",
                [WMI_TARGET_KEY_NOT_FOUND]  "WMI_TARGET_KEY_NOT_FOUND",
                [WMI_TARGET_DECRYPTION_ERR] "WMI_TARGET_DECRYPTION_ERR",
                [WMI_TARGET_BMISS]          "WMI_TARGET_BMISS",
                [WMI_PSDISABLE_NODE_JOIN]   "WMI_PSDISABLE_NODE_JOIN"
                };

    A_PRINTF("AR6000 Error on Target. Error = 0x%x\n", errorVal);

    /* One error is reported at a time, and errorval is a bitmask */
    if(errorVal & (errorVal - 1))
       return;

    A_PRINTF("AR6000 Error type = ");
    switch(errorVal)
    {
        case WMI_TARGET_PM_ERR_FAIL:
        case WMI_TARGET_KEY_NOT_FOUND:
        case WMI_TARGET_DECRYPTION_ERR:
        case WMI_TARGET_BMISS:
        case WMI_PSDISABLE_NODE_JOIN:
            A_PRINTF("%s\n", errString[errorVal]);
            break;
        default:
            A_PRINTF("INVALID\n");
            break;
    }

}


void
ar6000_cac_event(AR_SOFTC_T *ar, A_UINT8 ac, A_UINT8 cacIndication,
                 A_UINT8 statusCode, A_UINT8 *tspecSuggestion)
{
    WMM_TSPEC_IE    *tspecIe;

    /*
     * This is the TSPEC IE suggestion from AP.
     * Suggestion provided by AP under some error
     * cases, could be helpful for the host app.
     * Check documentation.
     */
    tspecIe = (WMM_TSPEC_IE *)tspecSuggestion;

    /*
     * What do we do, if we get TSPEC rejection? One thought
     * that comes to mind is implictly delete the pstream...
     */
    A_PRINTF("AR6000 CAC notification. "
                "AC = %d, cacIndication = 0x%x, statusCode = 0x%x\n",
                 ac, cacIndication, statusCode);
}

void
ar6000_channel_change_event(AR_SOFTC_T *ar, A_UINT16 oldChannel,
                            A_UINT16 newChannel)
{
    A_PRINTF("Channel Change notification\nOld Channel: %d, New Channel: %d\n",
             oldChannel, newChannel);
}

#define AR6000_PRINT_BSSID(_pBss)  do {     \
        A_PRINTF("%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x ",\
                 (_pBss)[0],(_pBss)[1],(_pBss)[2],(_pBss)[3],\
                 (_pBss)[4],(_pBss)[5]);  \
} while(0)

void
ar6000_roam_tbl_event(AR_SOFTC_T *ar, WMI_TARGET_ROAM_TBL *pTbl)
{
    A_UINT8 i;

    A_PRINTF("ROAM TABLE NO OF ENTRIES is %d ROAM MODE is %d\n",
              pTbl->numEntries, pTbl->roamMode);
    for (i= 0; i < pTbl->numEntries; i++) {
        A_PRINTF("[%d]bssid %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x ", i,
            pTbl->bssRoamInfo[i].bssid[0], pTbl->bssRoamInfo[i].bssid[1],
            pTbl->bssRoamInfo[i].bssid[2],
            pTbl->bssRoamInfo[i].bssid[3],
            pTbl->bssRoamInfo[i].bssid[4],
            pTbl->bssRoamInfo[i].bssid[5]);
        A_PRINTF("RSSI %d RSSIDT %d LAST RSSI %d UTIL %d ROAM_UTIL %d"
                 " BIAS %d\n",
            pTbl->bssRoamInfo[i].rssi,
            pTbl->bssRoamInfo[i].rssidt,
            pTbl->bssRoamInfo[i].last_rssi,
            pTbl->bssRoamInfo[i].util,
            pTbl->bssRoamInfo[i].roam_util,
            pTbl->bssRoamInfo[i].bias);
    }
}

void
ar6000_wow_list_event(struct ar6_softc *ar, A_UINT8 num_filters, WMI_GET_WOW_LIST_REPLY *wow_reply)
{
    A_UINT8 i,j;

    /*Each event now contains exactly one filter, see bug 26613*/
    A_PRINTF("WOW pattern %d of %d patterns\n", wow_reply->this_filter_num,                 wow_reply->num_filters);
    A_PRINTF("wow mode = %s host mode = %s\n",
            (wow_reply->wow_mode == 0? "disabled":"enabled"),
            (wow_reply->host_mode == 1 ? "awake":"asleep"));


    /*If there are no patterns, the reply will only contain generic
      WoW information. Pattern information will exist only if there are
      patterns present. Bug 26716*/

   /* If this event contains pattern information, display it*/
    if (wow_reply->this_filter_num) {
        i=0;
        A_PRINTF("id=%d size=%d offset=%d\n",
                    wow_reply->wow_filters[i].wow_filter_id,
                    wow_reply->wow_filters[i].wow_filter_size,
                    wow_reply->wow_filters[i].wow_filter_offset);
       A_PRINTF("wow pattern = ");
       for (j=0; j< wow_reply->wow_filters[i].wow_filter_size; j++) {
             A_PRINTF("%2.2x",wow_reply->wow_filters[i].wow_filter_pattern[j]);
        }

        A_PRINTF("\nwow mask = ");
        for (j=0; j< wow_reply->wow_filters[i].wow_filter_size; j++) {
            A_PRINTF("%2.2x",wow_reply->wow_filters[i].wow_filter_mask[j]);
        }
        A_PRINTF("\n");
    }
}

/*
 * Report the Roaming related data collected on the target
 */
void
ar6000_display_roam_time(WMI_TARGET_ROAM_TIME *p)
{
    A_PRINTF("Disconnect Data : BSSID: ");
    AR6000_PRINT_BSSID(p->disassoc_bssid);
    A_PRINTF(" RSSI %d DISASSOC Time %d NO_TXRX_TIME %d\n",
             p->disassoc_bss_rssi,p->disassoc_time,
             p->no_txrx_time);
    A_PRINTF("Connect Data: BSSID: ");
    AR6000_PRINT_BSSID(p->assoc_bssid);
    A_PRINTF(" RSSI %d ASSOC Time %d TXRX_TIME %d\n",
             p->assoc_bss_rssi,p->assoc_time,
             p->allow_txrx_time);
    A_PRINTF("Last Data Tx Time (b4 Disassoc) %d "\
             "First Data Tx Time (after Assoc) %d\n",
             p->last_data_txrx_time, p->first_data_txrx_time);
}

void
ar6000_roam_data_event(AR_SOFTC_T *ar, WMI_TARGET_ROAM_DATA *p)
{
    switch (p->roamDataType) {
        case ROAM_DATA_TIME:
            ar6000_display_roam_time(&p->u.roamTime);
            break;
        default:
            break;
    }
}

void
ar6000_bssInfo_event_rx(AR_SOFTC_T *ar, A_UINT8 *datap, int len)
{
    struct sk_buff *skb;
    WMI_BSS_INFO_HDR *bih = (WMI_BSS_INFO_HDR *)datap;


    if (!ar->arMgmtFilter) {
        return;
    }
    if (((ar->arMgmtFilter & IEEE80211_FILTER_TYPE_BEACON) &&
        (bih->frameType != BEACON_FTYPE))  ||
        ((ar->arMgmtFilter & IEEE80211_FILTER_TYPE_PROBE_RESP) &&
        (bih->frameType != PROBERESP_FTYPE)))
    {
        return;
    }

    if ((skb = A_NETBUF_ALLOC_RAW(len)) != NULL) {

        A_NETBUF_PUT(skb, len);
        A_MEMCPY(A_NETBUF_DATA(skb), datap, len);
        skb->dev = ar->arNetDev;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,22)
        A_MEMCPY(skb_mac_header(skb), A_NETBUF_DATA(skb), 6);
#else
        skb->mac.raw = A_NETBUF_DATA(skb);
#endif
        skb->ip_summed = CHECKSUM_NONE;
        skb->pkt_type = PACKET_OTHERHOST;
        skb->protocol = __constant_htons(0x0019);
        netif_rx(skb);
    }
}

A_UINT32 wmiSendCmdNum;

A_STATUS
ar6000_control_tx(void *devt, void *osbuf, HTC_ENDPOINT_ID eid)
{
    AR_SOFTC_T       *ar = (AR_SOFTC_T *)devt;
    A_STATUS         status = A_OK;
    struct ar_cookie *cookie = NULL;
    int i;

        /* take lock to protect ar6000_alloc_cookie() */
    AR6000_SPIN_LOCK(&ar->arLock, 0);

    do {

        AR_DEBUG2_PRINTF("ar_contrstatus = ol_tx: skb=0x%x, len=0x%x eid =%d\n",
                         (A_UINT32)osbuf, A_NETBUF_LEN(osbuf), eid);

        if (ar->arWMIControlEpFull && (eid == ar->arControlEp)) {
                /* control endpoint is full, don't allocate resources, we
                 * are just going to drop this packet */
            cookie = NULL;
            AR_DEBUG_PRINTF(" WMI Control EP full, dropping packet : 0x%X, len:%d \n",
                    (A_UINT32)osbuf, A_NETBUF_LEN(osbuf));
        } else {
            cookie = ar6000_alloc_cookie(ar);
        }

        if (cookie == NULL) {
            status = A_NO_MEMORY;
            break;
        }

        if(logWmiRawMsgs) {
            A_PRINTF("WMI cmd send, msgNo %d :", wmiSendCmdNum);
            for(i = 0; i < a_netbuf_to_len(osbuf); i++)
                A_PRINTF("%x ", ((A_UINT8 *)a_netbuf_to_data(osbuf))[i]);
            A_PRINTF("\n");
        }

        wmiSendCmdNum++;

    } while (FALSE);

    if (cookie != NULL) {
            /* got a structure to send it out on */
        ar->arTxPending[eid]++;

        if (eid != ar->arControlEp) {
            ar->arTotalTxDataPending++;
        }
    }

    AR6000_SPIN_UNLOCK(&ar->arLock, 0);

    if (cookie != NULL) {
        cookie->arc_bp[0] = (A_UINT32)osbuf;
        cookie->arc_bp[1] = 0;
        SET_HTC_PACKET_INFO_TX(&cookie->HtcPkt,
                               cookie,
                               A_NETBUF_DATA(osbuf),
                               A_NETBUF_LEN(osbuf),
                               eid,
                               AR6K_CONTROL_PKT_TAG);
            /* this interface is asynchronous, if there is an error, cleanup will happen in the
             * TX completion callback */
        HTCSendPkt(ar->arHtcTarget, &cookie->HtcPkt);
        status = A_OK;
    }

    return status;
}

/* indicate tx activity or inactivity on a WMI stream */
void ar6000_indicate_tx_activity(void *devt, A_UINT8 TrafficClass, A_BOOL Active)
{
    AR_SOFTC_T  *ar = (AR_SOFTC_T *)devt;
    HTC_ENDPOINT_ID eid ;
    int i;

    if (ar->arWmiEnabled) {
        eid = arAc2EndpointID(ar, TrafficClass);

        AR6000_SPIN_LOCK(&ar->arLock, 0);

        ar->arAcStreamActive[TrafficClass] = Active;

        if (Active) {
            /* when a stream goes active, keep track of the active stream with the highest priority */

            if (ar->arAcStreamPriMap[TrafficClass] > ar->arHiAcStreamActivePri) {
                    /* set the new highest active priority */
                ar->arHiAcStreamActivePri = ar->arAcStreamPriMap[TrafficClass];
            }

        } else {
            /* when a stream goes inactive, we may have to search for the next active stream
             * that is the highest priority */

            if (ar->arHiAcStreamActivePri == ar->arAcStreamPriMap[TrafficClass]) {

                /* the highest priority stream just went inactive */

                    /* reset and search for the "next" highest "active" priority stream */
                ar->arHiAcStreamActivePri = 0;
                for (i = 0; i < WMM_NUM_AC; i++) {
                    if (ar->arAcStreamActive[i]) {
                        if (ar->arAcStreamPriMap[i] > ar->arHiAcStreamActivePri) {
                            /* set the new highest active priority */
                            ar->arHiAcStreamActivePri = ar->arAcStreamPriMap[i];
                        }
                    }
                }
            }
        }

        AR6000_SPIN_UNLOCK(&ar->arLock, 0);

    } else {
            /* for mbox ping testing, the traffic class is mapped directly as a stream ID,
             * see handling of AR6000_XIOCTL_TRAFFIC_ACTIVITY_CHANGE in ioctl.c */
        eid = (HTC_ENDPOINT_ID)TrafficClass;
    }

        /* notify HTC, this may cause credit distribution changes */

    HTCIndicateActivityChange(ar->arHtcTarget,
                              eid,
                              Active);

}

module_init(ar6000_init_module);
module_exit(ar6000_cleanup_module);

/* Init cookie queue */
static void
ar6000_cookie_init(AR_SOFTC_T *ar)
{
    A_UINT32    i;

    ar->arCookieList = NULL;
    A_MEMZERO(s_ar_cookie_mem, sizeof(s_ar_cookie_mem));

    for (i = 0; i < MAX_COOKIE_NUM; i++) {
        ar6000_free_cookie(ar, &s_ar_cookie_mem[i]);
    }
}

/* cleanup cookie queue */
static void
ar6000_cookie_cleanup(AR_SOFTC_T *ar)
{
    /* It is gone .... */
    ar->arCookieList = NULL;
}

/* Init cookie queue */
static void
ar6000_free_cookie(AR_SOFTC_T *ar, struct ar_cookie * cookie)
{
    /* Insert first */
    A_ASSERT(ar != NULL);
    A_ASSERT(cookie != NULL);
    cookie->arc_list_next = ar->arCookieList;
    ar->arCookieList = cookie;
}

/* cleanup cookie queue */
static struct ar_cookie *
ar6000_alloc_cookie(AR_SOFTC_T  *ar)
{
    struct ar_cookie   *cookie;

    cookie = ar->arCookieList;
    if(cookie != NULL)
    {
        ar->arCookieList = cookie->arc_list_next;
    }

    return cookie;
}

#ifdef SEND_EVENT_TO_APP
/*
 * This function is used to send event which come from taget to
 * the application. The buf which send to application is include
 * the event ID and event content.
 */
#define EVENT_ID_LEN   2
void ar6000_send_event_to_app(AR_SOFTC_T *ar, A_UINT16 eventId,
                              A_UINT8 *datap, int len)
{

#if (WIRELESS_EXT >= 15)

/* note: IWEVCUSTOM only exists in wireless extensions after version 15 */

    char *buf;
    A_UINT16 size;
    union iwreq_data wrqu;

    size = len + EVENT_ID_LEN;

    if (size > IW_CUSTOM_MAX) {
        AR_DEBUG_PRINTF("WMI event ID : 0x%4.4X, len = %d too big for IWEVCUSTOM (max=%d) \n",
                eventId, size, IW_CUSTOM_MAX);
        return;
    }

    buf = A_MALLOC_NOWAIT(size);
    if (NULL == buf){
        AR_DEBUG_PRINTF("%s: failed to allocate %d bytes\n", __func__, size);
        return;
    }

    A_MEMZERO(buf, size);
    A_MEMCPY(buf, &eventId, EVENT_ID_LEN);
    A_MEMCPY(buf+EVENT_ID_LEN, datap, len);

    //AR_DEBUG_PRINTF("event ID = %d,len = %d\n",*(A_UINT16*)buf, size);
    A_MEMZERO(&wrqu, sizeof(wrqu));
    wrqu.data.length = size;
    wireless_send_event(ar->arNetDev, IWEVCUSTOM, &wrqu, buf);

    A_FREE(buf);
#endif


}

/*
 * This function is used to send events larger than 256 bytes
 * to the application. The buf which is sent to application
 * includes the event ID and event content.
 */
void ar6000_send_generic_event_to_app(AR_SOFTC_T *ar, A_UINT16 eventId,
                                      A_UINT8 *datap, int len)
{

#if (WIRELESS_EXT >= 18)

/* IWEVGENIE exists in wireless extensions version 18 onwards */

    char *buf;
    A_UINT16 size;
    union iwreq_data wrqu;

    size = len + EVENT_ID_LEN;

    if (size > IW_GENERIC_IE_MAX) {
        AR_DEBUG_PRINTF("WMI event ID : 0x%4.4X, len = %d too big for IWEVGENIE (max=%d) \n",
                        eventId, size, IW_GENERIC_IE_MAX);
        return;
    }

    buf = A_MALLOC_NOWAIT(size);
    if (NULL == buf){
        AR_DEBUG_PRINTF("%s: failed to allocate %d bytes\n", __func__, size);
        return;
    }

    A_MEMZERO(buf, size);
    A_MEMCPY(buf, &eventId, EVENT_ID_LEN);
    A_MEMCPY(buf+EVENT_ID_LEN, datap, len);

    A_MEMZERO(&wrqu, sizeof(wrqu));
    wrqu.data.length = size;
    wireless_send_event(ar->arNetDev, IWEVGENIE, &wrqu, buf);

    A_FREE(buf);

#endif /* (WIRELESS_EXT >= 18) */

}
#endif /* SEND_EVENT_TO_APP */


void
ar6000_tx_retry_err_event(void *devt)
{
    AR_DEBUG2_PRINTF("Tx retries reach maximum!\n");
}

void
ar6000_snrThresholdEvent_rx(void *devt, WMI_SNR_THRESHOLD_VAL newThreshold, A_UINT8 snr)
{
    WMI_SNR_THRESHOLD_EVENT event;
    AR_SOFTC_T *ar = (AR_SOFTC_T *)devt;

    event.range = newThreshold;
    event.snr = snr;

    ar6000_send_event_to_app(ar, WMI_SNR_THRESHOLD_EVENTID, (A_UINT8 *)&event,
                             sizeof(WMI_SNR_THRESHOLD_EVENT));
}

void
ar6000_lqThresholdEvent_rx(void *devt, WMI_LQ_THRESHOLD_VAL newThreshold, A_UINT8 lq)
{
    AR_DEBUG2_PRINTF("lq threshold range %d, lq %d\n", newThreshold, lq);
}



A_UINT32
a_copy_to_user(void *to, const void *from, A_UINT32 n)
{
    return(copy_to_user(to, from, n));
}

A_UINT32
a_copy_from_user(void *to, const void *from, A_UINT32 n)
{
    return(copy_from_user(to, from, n));
}


A_STATUS
ar6000_get_driver_cfg(struct net_device *dev,
                        A_UINT16 cfgParam,
                        void *result)
{

    A_STATUS    ret = 0;

    switch(cfgParam)
    {
        case AR6000_DRIVER_CFG_GET_WLANNODECACHING:
           *((A_UINT32 *)result) = wlanNodeCaching;
           break;
        case AR6000_DRIVER_CFG_LOG_RAW_WMI_MSGS:
           *((A_UINT32 *)result) = logWmiRawMsgs;
            break;
        default:
           ret = EINVAL;
           break;
    }

    return ret;
}

void
ar6000_keepalive_rx(void *devt, A_UINT8 configured)
{
    AR_SOFTC_T *ar = (AR_SOFTC_T *)devt;

    ar->arKeepaliveConfigured = configured;
    wake_up(&arEvent);
}

void
ar6000_pmkid_list_event(void *devt, A_UINT8 numPMKID, WMI_PMKID *pmkidList,
                        A_UINT8 *bssidList)
{
    A_UINT8 i, j;

    A_PRINTF("Number of Cached PMKIDs is %d\n", numPMKID);

    for (i = 0; i < numPMKID; i++) {
        A_PRINTF("\nBSSID %d ", i);
            for (j = 0; j < ATH_MAC_LEN; j++) {
                A_PRINTF("%2.2x", bssidList[j]);
            }
        bssidList += (ATH_MAC_LEN + WMI_PMKID_LEN);
        A_PRINTF("\nPMKID %d ", i);
            for (j = 0; j < WMI_PMKID_LEN; j++) {
                A_PRINTF("%2.2x", pmkidList->pmkid[j]);
            }
        pmkidList = (WMI_PMKID *)((A_UINT8 *)pmkidList + ATH_MAC_LEN +
                                  WMI_PMKID_LEN);
    }
}

void ar6000_pspoll_event(AR_SOFTC_T *ar,A_UINT8 aid)
{
    sta_t *conn=NULL;
    A_BOOL isPsqEmpty = FALSE;

    conn = ieee80211_find_conn_for_aid(ar, aid);

    /* If the PS q for this STA is not empty, dequeue and send a pkt from
     * the head of the q. Also update the More data bit in the WMI_DATA_HDR
     * if there are more pkts for this STA in the PS q. If there are no more
     * pkts for this STA, update the PVB for this STA.
     */
    A_MUTEX_LOCK(&conn->psqLock);
    isPsqEmpty  = A_NETBUF_QUEUE_EMPTY(&conn->psq);
    A_MUTEX_UNLOCK(&conn->psqLock);

    if (isPsqEmpty) {
        /* TODO:No buffered pkts for this STA. Send out a NULL data frame */
    } else {
        struct sk_buff *skb = NULL;

        A_MUTEX_LOCK(&conn->psqLock);
        skb = A_NETBUF_DEQUEUE(&conn->psq);
        A_MUTEX_UNLOCK(&conn->psqLock);
        /* Set the STA flag to PSPolled, so that the frame will go out */
        STA_SET_PS_POLLED(conn);
        ar6000_data_tx(skb, ar->arNetDev);
        STA_CLR_PS_POLLED(conn);

        /* Clear the PVB for this STA if the queue has become empty */
        A_MUTEX_LOCK(&conn->psqLock);
        isPsqEmpty  = A_NETBUF_QUEUE_EMPTY(&conn->psq);
        A_MUTEX_UNLOCK(&conn->psqLock);

        if (isPsqEmpty) {
            wmi_set_pvb_cmd(ar->arWmi, conn->aid, 0);
        }
    }
}

void ar6000_dtimexpiry_event(AR_SOFTC_T *ar)
{
    A_BOOL isMcastQueued = FALSE;
    struct sk_buff *skb = NULL;

    /* If there are no associated STAs, ignore the DTIM expiry event.
     * There can be potential race conditions where the last associated
     * STA may disconnect & before the host could clear the 'Indicate DTIM'
     * request to the firmware, the firmware would have just indicated a DTIM
     * expiry event. The race is between 'clear DTIM expiry cmd' going 
     * from the host to the firmware & the DTIM expiry event happening from
     * the firmware to the host. 
     */
    if (ar->sta_list_index == 0) {
        return;
    }

    A_MUTEX_LOCK(&ar->mcastpsqLock);
    isMcastQueued = A_NETBUF_QUEUE_EMPTY(&ar->mcastpsq);
    A_MUTEX_UNLOCK(&ar->mcastpsqLock);

    A_ASSERT(isMcastQueued == FALSE);

    /* Flush the mcast psq to the target */
    /* Set the STA flag to DTIMExpired, so that the frame will go out */
    ar->DTIMExpired = TRUE;

    A_MUTEX_LOCK(&ar->mcastpsqLock);
    while (!A_NETBUF_QUEUE_EMPTY(&ar->mcastpsq)) {
        skb = A_NETBUF_DEQUEUE(&ar->mcastpsq);
        A_MUTEX_UNLOCK(&ar->mcastpsqLock);

        ar6000_data_tx(skb, ar->arNetDev);

        A_MUTEX_LOCK(&ar->mcastpsqLock);
    }
    A_MUTEX_UNLOCK(&ar->mcastpsqLock);

    /* Reset the DTIMExpired flag back to 0 */
    ar->DTIMExpired = FALSE;

    /* Clear the LSB of the BitMapCtl field of the TIM IE */
    wmi_set_pvb_cmd(ar->arWmi, MCAST_AID, 0);
}

#ifdef USER_KEYS
static A_STATUS

ar6000_reinstall_keys(AR_SOFTC_T *ar, A_UINT8 key_op_ctrl)
{
    A_STATUS status = A_OK;
    struct ieee80211req_key *uik = &ar->user_saved_keys.ucast_ik;
    struct ieee80211req_key *bik = &ar->user_saved_keys.bcast_ik;
    CRYPTO_TYPE  keyType = ar->user_saved_keys.keyType;

    if (IEEE80211_CIPHER_CCKM_KRK != uik->ik_type) {
        if (NONE_CRYPT == keyType) {
            goto _reinstall_keys_out;
        }

        if (uik->ik_keylen) {
            status = wmi_addKey_cmd(ar->arWmi, uik->ik_keyix,
                    ar->user_saved_keys.keyType, PAIRWISE_USAGE,
                    uik->ik_keylen, (A_UINT8 *)&uik->ik_keyrsc,
                    uik->ik_keydata, key_op_ctrl, uik->ik_macaddr, SYNC_BEFORE_WMIFLAG);
        }

    } else {
        status = wmi_add_krk_cmd(ar->arWmi, uik->ik_keydata);
    }

    if (IEEE80211_CIPHER_CCKM_KRK != bik->ik_type) {
        if (NONE_CRYPT == keyType) {
            goto _reinstall_keys_out;
        }

        if (bik->ik_keylen) {
            status = wmi_addKey_cmd(ar->arWmi, bik->ik_keyix,
                    ar->user_saved_keys.keyType, GROUP_USAGE,
                    bik->ik_keylen, (A_UINT8 *)&bik->ik_keyrsc,
                    bik->ik_keydata, key_op_ctrl, bik->ik_macaddr, NO_SYNC_WMIFLAG);
        }
    } else {
        status = wmi_add_krk_cmd(ar->arWmi, bik->ik_keydata);
    }

_reinstall_keys_out:
    ar->user_savedkeys_stat = USER_SAVEDKEYS_STAT_INIT;
    ar->user_key_ctrl      = 0;

    return status;
}
#endif /* USER_KEYS */


void
ar6000_dset_open_req(
    void *context,
    A_UINT32 id,
    A_UINT32 targHandle,
    A_UINT32 targReplyFn,
    A_UINT32 targReplyArg)
{
}

void
ar6000_dset_close(
    void *context,
    A_UINT32 access_cookie)
{
    return;
}

void
ar6000_dset_data_req(
   void *context,
   A_UINT32 accessCookie,
   A_UINT32 offset,
   A_UINT32 length,
   A_UINT32 targBuf,
   A_UINT32 targReplyFn,
   A_UINT32 targReplyArg)
{
}

int
ar6000_ap_mode_profile_commit(struct ar6_softc *ar)
{
    WMI_CONNECT_CMD p;
    struct ieee80211req_key *ik;
    CRYPTO_TYPE keyType = NONE_CRYPT;

    /* No change in AP's profile configuration */
    if(ar->ap_profile_flag==0) {
        A_PRINTF("COMMIT: No change in profile!!!\n");
        return -ENODATA;
    }

    if(!ar->arSsidLen) {
        A_PRINTF("SSID not set!!!\n");
        return -ECHRNG;
    }

    if(!ar->arChannelHint) {
        /* Without channel info, can't start AP */
        A_PRINTF("Channel not set!!!\n");
        return -ECHRNG;
    }

    if(ar->arPairwiseCrypto != ar->arGroupCrypto) {
        A_PRINTF("Mixed cipher not supported in AP mode\n");
        return -EOPNOTSUPP;
    }

    switch(ar->arAuthMode) {
    case NONE_AUTH:
        if((ar->arPairwiseCrypto != NONE_CRYPT) &&
           (ar->arPairwiseCrypto != WEP_CRYPT)) {
            A_PRINTF("Cipher not supported in AP mode Open auth\n");
            return -EOPNOTSUPP;
        }
        break;
    case WPA_PSK_AUTH:
        if(ar->arPairwiseCrypto != TKIP_CRYPT) {
            A_PRINTF("Cipher not supported in AP mode WPA\n");
            return -EOPNOTSUPP;
        }
        break;
    case WPA2_PSK_AUTH:
        if(ar->arPairwiseCrypto != AES_CRYPT) {
            A_PRINTF("Cipher not supported in AP mode WPA2\n");
            return -EOPNOTSUPP;
        }
        break;
    case WPA_AUTH:
    case WPA2_AUTH:
    case WPA_AUTH_CCKM:
    case WPA2_AUTH_CCKM:
        A_PRINTF("This key mgmt type not supported in AP mode\n");
        return -EOPNOTSUPP;
    }

    /* Update the arNetworkType */
    ar->arNetworkType = ar->arNextMode;

    A_MEMZERO(&p,sizeof(p));
    p.ssidLength = ar->arSsidLen;
    A_MEMCPY(p.ssid,ar->arSsid,p.ssidLength);
    p.channel = ar->arChannelHint;
    p.networkType = ar->arNetworkType;

    p.dot11AuthMode = ar->arDot11AuthMode;
    p.authMode = ar->arAuthMode;
    p.pairwiseCryptoType = ar->arPairwiseCrypto;
    p.pairwiseCryptoLen = ar->arPairwiseCryptoLen;
    p.groupCryptoType = ar->arGroupCrypto;
    p.groupCryptoLen = ar->arGroupCryptoLen;
    p.ctrl_flags = ar->arConnectCtrlFlags;

    wmi_ap_profile_commit(ar->arWmi, &p);
    ar->arConnected = TRUE;
    ar->ap_profile_flag = 0;

    switch(ar->arAuthMode) {
    case NONE_AUTH:
        if(ar->arPairwiseCrypto == WEP_CRYPT) {
            ar6000_install_static_wep_keys(ar);
        }
        break;
    case WPA_PSK_AUTH:
    case WPA2_PSK_AUTH:
        ik = &ar->ap_mode_bkey;
        switch (ik->ik_type) {
            case IEEE80211_CIPHER_TKIP:
                keyType = TKIP_CRYPT;
                break;
            case IEEE80211_CIPHER_AES_CCM:
                keyType = AES_CRYPT;
                break;
            default:
                goto skip_key;
        }
        wmi_addKey_cmd(ar->arWmi, ik->ik_keyix, keyType, GROUP_USAGE,
                        ik->ik_keylen, (A_UINT8 *)&ik->ik_keyrsc,
                        ik->ik_keydata, KEY_OP_INIT_VAL, ik->ik_macaddr,
                        SYNC_BOTH_WMIFLAG);

        break;
    }

skip_key:
    return 0;
}

A_STATUS
ar6000_ap_mode_get_wpa_ie(struct ar6_softc *ar, struct ieee80211req_wpaie *wpaie)
{
    sta_t *conn = NULL;
    conn = ieee80211_find_conn(ar, wpaie->wpa_macaddr);

    A_MEMZERO(wpaie->wpa_ie, IEEE80211_MAX_IE);
    A_MEMZERO(wpaie->rsn_ie, IEEE80211_MAX_IE);

    if(conn) {
        A_MEMCPY(wpaie->wpa_ie, conn->wpa_ie, IEEE80211_MAX_IE);
    }

    return 0;
}

A_STATUS
is_iwioctl_allowed(A_UINT8 mode, A_UINT16 cmd)
{
    if(cmd >= SIOCSIWCOMMIT && cmd <= SIOCGIWPOWER) {
        cmd -= SIOCSIWCOMMIT;
        if(sioctl_filter[cmd] == 0xFF) return A_OK;
        if(sioctl_filter[cmd] & mode) return A_OK;
    } else if(cmd >= SIOCIWFIRSTPRIV && cmd <= (SIOCIWFIRSTPRIV+30)) {
        cmd -= SIOCIWFIRSTPRIV;
        if(pioctl_filter[cmd] == 0xFF) return A_OK;
        if(pioctl_filter[cmd] & mode) return A_OK;
    } else {
        return A_ERROR;
    }
    return A_ENOTSUP;
}

A_STATUS
is_xioctl_allowed(A_UINT8 mode, int cmd)
{
    if(sizeof(xioctl_filter)-1 < cmd) {
        A_PRINTF("Filter for this cmd=%d not defined\n",cmd);
        return 0;
    }
    if(xioctl_filter[cmd] == 0xFF) return A_OK;
    if(xioctl_filter[cmd] & mode) return A_OK;
    return A_ERROR;
}
void ar6000_peer_event(
    void *context,
    A_UINT8 eventCode,
    A_UINT8 *macAddr)
{
    A_UINT8 pos;

    for (pos=0;pos<6;pos++)
        printk("%02x: ",*(macAddr+pos));
    printk("\n");
}

static void ar6000_enable_mmc_polling(int enable) { 
	mm_segment_t        oldfs; 
	struct file     *filp; 
	int         length; 
	oldfs = get_fs(); 
	set_fs(KERNEL_DS); 
	do { 
		char buf[3]; 
		filp = filp_open(MMC_POLLING_FILE, O_RDWR, S_IRUSR); 
		if (IS_ERR(filp) || !filp->f_op) 
			break; 
		length = snprintf(buf, sizeof(buf), "%d\n", enable ? 1 : 0); 
		if (filp->f_op->write(filp, buf, length, &filp->f_pos) != length) { 
			printk("%s: file write error\n", __func__); 
			break; 
		} else { 
			printk("%s: set polling as %s\n", __func__, buf); 
		} 
	} while (0); 
	if (!IS_ERR(filp)) { 
		filp_close(filp, NULL); 
	} 
	set_fs(oldfs); 
} 
