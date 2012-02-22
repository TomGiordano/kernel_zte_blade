/*when       who     what, where, why
--------   ----    ---------------------------------------------------
2011.03.09  lianghouxing     LHX_PM_20110309_01 FOR CRDB00622074 modify some RPC ID for WakeupReason
2010.09.16  zhengchao     add addtional wakeup info,//ZTE_ZHENFCHAO_20100916
2010.04.02  huangyanjun   add notification user when sleep or wakeup . flag:ZTE_HYJ_ADD_NOTIFY_USER_WHEN_SUSPEND
2010.01.26  huangyanjun   add map prog id to name . flag:ZTE_HYJ_ADD_MAP_PROG_ID_TO_NAME
2010.01.26  huangyanjun   parse wakeup information. flag:ZTE_HYJ_PARSE_WAKEUP_INFO
2010.01.22  huangyanjun   add wakeup information to log_kernel. flag:HYJ_ADD_WAKEUP_INFO_TO_LOG
--------   ----    ---------------------------------------------------
*/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/string.h>

#include <linux/notifier.h>
#include <linux/suspend.h>
#include <linux/miscdevice.h>
#include <linux/device.h>
#include <linux/rtc.h>

#define PM_MINOR_DEV 150
#define  RESUME_STATE (0x0)
#define  SUSPEND_STATE (0x1)
#define WAKE_INFO_RECORD_NR  20
#define DEM_MAX_PORT_NAME_LEN (20)

#define PMDPRINTK(fmt, args...)						\
	printk(KERN_INFO "WAKEUP INFO" fmt "", ## args)
struct msm_pm_smem_t {
	uint32_t sleep_time;
	uint32_t irq_mask;
	uint32_t resources_used;
	uint32_t reserved1;
	uint32_t wakeup_reason;
	uint32_t pending_irqs;
	uint32_t rpc_prog;
	uint32_t rpc_proc;
	char     smd_port_name[DEM_MAX_PORT_NAME_LEN];
	uint32_t reserved2;
};

struct pm_monitor {
	int pm_state_changed;
	int pm_state;
	int info_index;
	char wakeup_info[WAKE_INFO_RECORD_NR][200];
  struct semaphore sem;
  wait_queue_head_t rqueue;
};
/*huangyanjun 2010-04-02 begin ZTE_HYJ_ADD_NOTIFY_USER_WHEN_SUSPEND*/
static int pm_monitor_open(struct inode *inodp,  struct file *filp);
static ssize_t pm_monitor_read(struct file *filp,  char __user *buff,  size_t count, loff_t *f_pos);
//static ssize_t pm_monitor_write(struct file *filp, const char __user *buff, size_t count, loff_t *f_pos);
static unsigned int pm_monitor_poll(struct file *filp,  struct poll_table_struct *p);
static int pm_monitor_release(struct inode *inodp,  struct file *filp);

static struct file_operations pm_monitor_fops = {
	.open = pm_monitor_open,
	.read = pm_monitor_read,
	.write = NULL,
	.poll = pm_monitor_poll,
	.release = pm_monitor_release,
};
static struct miscdevice pm_monitor_device = {
	.minor		= PM_MINOR_DEV,
	.name		= "pm_monitor",
	.fops		= &pm_monitor_fops,
};
/*huangyanjun 2010-04-02 end ZTE_HYJ_ADD_NOTIFY_USER_WHEN_SUSPEND*/
static struct pm_monitor *dev = NULL;
extern struct msm_pm_smem_t * get_msm_pm_smem_data(void);

/*
 * pm_monitor_state_show
 */
static ssize_t pm_monitor_state_show(struct device *devp, struct device_attribute *attr, char *buf)
{
	char * echo = dev->pm_state?"Suspend\n":"Resume\n";
	return sprintf(buf, "%s\n", echo);
}
static DEVICE_ATTR(pm_state, S_IRUGO, pm_monitor_state_show, NULL);

/*
 * pm_monitor_wakeup_info_show
 */
static ssize_t pm_monitor_wakeup_info_show(struct device *devp, struct device_attribute *attr, char *buf)
{
	char * echo = buf;
	int i ;
	
	for (i = 0; i < WAKE_INFO_RECORD_NR; i++) {
		if (strlen(&(dev->wakeup_info[i][0])) >0) {
			echo += sprintf(echo, "%s\n", dev->wakeup_info[i]);
		}
	}
	
	return echo-buf;
}
static DEVICE_ATTR(wakeup_info, S_IRUGO, pm_monitor_wakeup_info_show, NULL);

#define ZTE_GET_AMSS_SLEEP_TIME
//ZTE_POWER_ZENGHUIPENG_20110212 add
#ifdef ZTE_GET_AMSS_SLEEP_TIME
#include "../../arch/arm/mach-msm/proc_comm.h"
#define ZTE_PROC_COMM_CMD3_RECORD_GET_AMSS_SLEEPTIME 8
static ssize_t pm_monitor_amss_sleep_time_show(struct device *devp, struct device_attribute *attr, char *buf)
{
	char * echo = buf;	
	unsigned msleeptimeinsecs=0;
	unsigned subcmd=ZTE_PROC_COMM_CMD3_RECORD_GET_AMSS_SLEEPTIME;
	msm_proc_comm(PCOM_CUSTOMER_CMD3, &msleeptimeinsecs, &subcmd);

	echo += sprintf(echo, "%d", msleeptimeinsecs);
	
	return echo-buf;
}
static DEVICE_ATTR(amss_sleep_time, S_IRUGO, pm_monitor_amss_sleep_time_show, NULL);
#endif
//ZTE_POWER_ZENGHUIPENG_20110212 add end


#define ZTE_FEATURE_GET_CHIP_VERSION_APP	//LHX_PM_20110525_01 add code to get chip version for APP
#ifdef ZTE_FEATURE_GET_CHIP_VERSION_APP
#include "../../arch/arm/mach-msm/proc_comm.h"
#define ZTE_PROC_COMM_CMD3_GET_CHIP_VERSION 10
#define ZTE_PROC_COMM_CMD3_GET_HW_MSM_VERSION 11
static ssize_t pm_monitor_chip_version_show(struct device *devp, struct device_attribute *attr, char *buf)
{
	char * echo = buf;	
	unsigned chip_version=0;
	unsigned subcmd=ZTE_PROC_COMM_CMD3_GET_CHIP_VERSION;
	msm_proc_comm(PCOM_CUSTOMER_CMD3, &chip_version, &subcmd);

	echo += sprintf(echo, "%d", chip_version);
	
	return echo-buf;
}
static DEVICE_ATTR(chip_version, S_IRUGO, pm_monitor_chip_version_show, NULL);

static ssize_t pm_monitor_hw_msm_version_show(struct device *devp, struct device_attribute *attr, char *buf)
{
	char * echo = buf;	
	unsigned hw_msm_version=0;
	unsigned subcmd=ZTE_PROC_COMM_CMD3_GET_HW_MSM_VERSION;
	msm_proc_comm(PCOM_CUSTOMER_CMD3, &hw_msm_version, &subcmd);

	echo += sprintf(echo, "%x", hw_msm_version);
	
	return echo-buf;
}
static DEVICE_ATTR(hw_msm_version, S_IRUGO, pm_monitor_hw_msm_version_show, NULL);
#endif	//end ZTE_FEATURE_GET_CHIP_VERSION_APP




/*BEGIN LHX_PM_20110328_01 add code to record how long  modem keeps awake while app sleep*/
unsigned pm_modem_sleep_time_get(void)
{
	unsigned msleeptimeinsecs = 0;
	#ifdef ZTE_GET_AMSS_SLEEP_TIME
	unsigned subcmd=ZTE_PROC_COMM_CMD3_RECORD_GET_AMSS_SLEEPTIME;
	msm_proc_comm(PCOM_CUSTOMER_CMD3, &msleeptimeinsecs, &subcmd);
	#endif
	return msleeptimeinsecs;
}
/*END LHX_PM_20110328_01 add code to record how long  modem keeps awake while app sleep*/



/*ZTE_HYJ_PARSE_WAKEUP_INFO  2010.0126  begin*/

#define ID_WAKEUP_REASON	0
#define ID_WAKEUP_PROG		1
#define ID_PM_MONITOR_MAX 	2

struct map_id_to_name {
	uint32_t val;
	char *str;
};

struct map_id_to_name_tbl {
	struct map_id_to_name *map;
	int size;
};

static struct map_id_to_name pm_monitor_wakeup_reason[] = {
	{ 0x00000040, "OTHER" },
	{ 0x00000020, "RESET" },
	{ 0x00000010, "ALARM" },
	{ 0x00000008, "TIMER" },
	{ 0x00000004, "GPIO" },
	{ 0x00000002, "INT" },
	{ 0x00000001, "RPC" },
	{ 0x00000000, "NONE" },
};


static struct map_id_to_name pm_monitor_oncrpc_prog[] = {
	{ 0x30000000, "CM" },
	{ 0x30000001, "DB" },
	{ 0x30000002, "SND" },
	{ 0x30000003, "WMS" },
	{ 0x30000004, "PDSM" },
	{ 0x30000005, "MISC_MODEM_APIS" },
	{ 0x30000006, "MISC_APPS_APIS" },
	{ 0x30000007, "JOYST" },
	{ 0x30000008, "VJOY" },
	{ 0x30000009, "JOYSTC" },
	{ 0x3000000a, "ADSPRTOSATOM" },
	{ 0x3000000b, "ADSPRTOSMTOA" },
	{ 0x3000000c, "I2C" },
	{ 0x3000000d, "TIME_REMOTE" },
	{ 0x3000000e, "NV" },
	{ 0x3000000f, "CLKRGM_SEC" },
	{ 0x30000010, "RDEVMAP" },
	{ 0x30000011, "FS_RAPI" },
	{ 0x30000012, "PBMLIB" },
	{ 0x30000013, "AUDMGR" },
	{ 0x30000014, "MVS" },
	{ 0x30000015, "DOG_KEEPALIVE" },
	{ 0x30000016, "GSDI_EXP" },
	{ 0x30000017, "AUTH" },
	{ 0x30000018, "NVRUIMI" },
	{ 0x30000019, "MMGSDILIB" },
	{ 0x3000001a, "CHARGER" },
	{ 0x3000001b, "UIM" },
	{ 0x3000001C, "ONCRPCTEST" },
	{ 0x3000001d, "PDSM_ATL" },
	{ 0x3000001e, "FS_XMOUNT" },
	{ 0x3000001f, "SECUTIL " },
	{ 0x30000020, "MCCMEID" },
	{ 0x30000021, "PM_STROBE_FLASH" },
	{ 0x30000022, "DS707_EXTIF" },
	{ 0x30000023, "SMD BRIDGE_MODEM" },
	{ 0x30000024, "SMD PORT_MGR" },
	{ 0x30000025, "BUS_PERF" },
	{ 0x30000026, "BUS_MON" },
	{ 0x30000027, "MC" },
	{ 0x30000028, "MCCAP" },
	{ 0x30000029, "MCCDMA" },
	{ 0x3000002a, "MCCDS" },
	{ 0x3000002b, "MCCSCH" },
	{ 0x3000002c, "MCCSRID" },
	{ 0x3000002d, "SNM" },
	{ 0x3000002e, "MCCSYOBJ" },
	{ 0x3000002f, "DS707_APIS" },
	{ 0x30000030, "DS_MP_SHIM_APPS_ASYNC" },
	{ 0x30000031, "DSRLP_APIS" },
	{ 0x30000032, "RLP_APIS" },
	{ 0x30000033, "DS_MP_SHIM_MODEM" },
	{ 0x30000034, "DSHDR_APIS" },
	{ 0x30000035, "DSHDR_MDM_APIS" },
	{ 0x30000036, "DS_MP_SHIM_APPS" },
	{ 0x30000037, "HDRMC_APIS" },
	{ 0x30000038, "SMD_BRIDGE_MTOA" },
	{ 0x30000039, "SMD_BRIDGE_ATOM" },
	{ 0x3000003a, "DPMAPP_OTG" },
	{ 0x3000003b, "DIAG" },
	{ 0x3000003c, "GSTK_EXP" },
	{ 0x3000003d, "DSBC_MDM_APIS" },
	{ 0x3000003e, "HDRMRLP_MDM_APIS" },
	{ 0x3000003f, "HDRMRLP_APPS_APIS" },
	{ 0x30000040, "HDRMC_MRLP_APIS" },
	{ 0x30000041, "PDCOMM_APP_API" },
	{ 0x30000042, "DSAT_APIS" },
	{ 0x30000043, "MISC_RF_APIS" },
	{ 0x30000044, "CMIPAPP" },
	{ 0x30000045, "DSMP_UMTS_MODEM_APIS" },
	{ 0x30000046, "DSMP_UMTS_APPS_APIS" },
	{ 0x30000047, "DSUCSDMPSHIM" },
	{ 0x30000048, "TIME_REMOTE_ATOM" },
	{ 0x3000004a, "SD" },
	{ 0x3000004b, "MMOC" },
	{ 0x3000004c, "WLAN_ADP_FTM" },
	{ 0x3000004d, "WLAN_CP_CM" },
	{ 0x3000004e, "FTM_WLAN" },
	{ 0x3000004f, "SDCC_CPRM" },
	{ 0x30000050, "CPRMINTERFACE" },
	{ 0x30000051, "DATA_ON_MODEM_MTOA_APIS" },
	{ 0x30000052, "DATA_ON_APPS_ATOM_APIS" },
	{ 0x30000053, "MISC_MODEM_APIS_NONWINMOB" },
	{ 0x30000054, "MISC_APPS_APIS_NONWINMOB" },
	{ 0x30000055, "PMEM_REMOTE" },
	{ 0x30000056, "TCXOMGR" },
	{ 0x30000057, "DSUCSDAPPIF_APIS" },
	{ 0x30000058, "BT" },
	{ 0x30000059, "PD_COMMS_API" },
	{ 0x3000005a, "PD_COMMS_CLIENT_API" },
	{ 0x3000005b, "PDAPI" },
	{ 0x3000005c, "LSA_SUPL_DSM" },
	{ 0x3000005d, "TIME_REMOTE_MTOA" },
	{ 0x3000005e, "FTM_BT" },
	{ 0X3000005f, "DSUCSDAPPIF_APIS" },
	{ 0X30000060, "PMAPP_GEN" },
	{ 0X30000061, "PM_LIB" },
	{ 0X30000062, "KEYPAD" },
	{ 0X30000063, "HSU_APP_APIS" },
	{ 0X30000064, "HSU_MDM_APIS" },
	{ 0X30000065, "ADIE_ADC_REMOTE_ATOM " },
	{ 0X30000066, "TLMM_REMOTE_ATOM" },
	{ 0X30000067, "UI_CALLCTRL" },
	{ 0X30000068, "UIUTILS" },
	{ 0X30000069, "PRL" },
	{ 0X3000006a, "HW" },
	{ 0X3000006b, "OEM_RAPI" },
	{ 0X3000006c, "WMSPM" },
	{ 0X3000006d, "BTPF" },
	{ 0X3000006e, "CLKRGM_SYNC_EVENT" },
	{ 0X3000006f, "USB_APPS_RPC" },
	{ 0X30000070, "USB_MODEM_RPC" },
	{ 0X30000071, "ADC" },
	{ 0X30000072, "CAMERAREMOTED" },
	{ 0X30000073, "SECAPIREMOTED" },
	{ 0X30000074, "DSATAPI" },
	{ 0X30000075, "CLKCTL_RPC" },
	{ 0X30000076, "BREWAPPCOORD" },
	{ 0X30000077, "ALTENVSHELL" },
	{ 0X30000078, "WLAN_TRP_UTILS" },
	{ 0X30000079, "GPIO_RPC" },
	{ 0X3000007a, "PING_RPC" },
	{ 0X3000007b, "DSC_DCM_API" },
	{ 0X3000007c, "L1_DS" },
	{ 0X3000007d, "QCHATPK_APIS" },
	{ 0X3000007e, "GPS_API" },
	{ 0X3000007f, "OSS_RRCASN_REMOTE" },
	{ 0X30000080, "PMAPP_OTG_REMOTE" },
	{ 0X30000081, "PING_MDM_RPC" },
	{ 0X30000082, "PING_KERNEL_RPC" },
	{ 0X30000083, "TIMETICK" },
	{ 0X30000084, "WM_BTHCI_FTM " },
	{ 0X30000085, "WM_BT_PF" },
	{ 0X30000086, "IPA_IPC_APIS" },
	{ 0X30000087, "UKCC_IPC_APIS" },
	{ 0X30000088, "CMIPSMS " },
	{ 0X30000089, "VBATT_REMOTE" },
	{ 0X3000008a, "MFPAL" },
	{ 0X3000008b, "DSUMTSPDPREG" },
	{ 0X3000008c, "LOC_API GPS CB" },//LHX_PM_20110309_01 add for GB CRDB00622074 form loc_api_rpc.h according svc_register_auto_apiversions
	{ 0X30000091, "HS_REM" },//LHX_PM_20110309_01 add for GB form hs_rem_rpc.h according svc_register_auto_apiversions
	{ 0X3000fe00, "RESTART_DAEMON NUMBER 0" },
	{ 0X3000fe01, "RESTART_DAEMON NUMBER 1" },
	{ 0X3000feff, "RESTART_DAEMON NUMBER 255" },
	{ 0X3000fffe, "BACKWARDS_COMPATIBILITY_IN_RPC_CLNT_LOOKUP" },
	{ 0X3000ffff, "RPC_ROUTER_SERVER_PROGRAM" },
	/*ZTE_HYJ_ADD_MAP_PROG_ID_TO_NAME  02/02/2001 begin*/
	{ 0x31000000, "CM CB" },
	{ 0x31000001, "DB CB" },
	{ 0x31000002, "SND CB" },
	{ 0x31000003, "WMS CB" },
	{ 0x31000004, "PDSM CB" },
	{ 0x31000005, "MISC_MODEM_APIS CB" },
	{ 0x31000006, "MISC_APPS_APIS CB" },
	{ 0x31000007, "JOYST CB" },
	{ 0x31000008, "VJOY CB" },
	{ 0x31000009, "JOYSTC CB" },
	{ 0x3100000a, "ADSPRTOSATOM CB" },
	{ 0x3100000b, "ADSPRTOSMTOA CB" },
	{ 0x3100000c, "I2C CB" },
	{ 0x3100000d, "TIME_REMOTE CB" },
	{ 0x3100000e, "NV CB" },
	{ 0x3100000f, "CLKRGM_SEC CB" },
	{ 0x31000010, "RDEVMAP CB" },
	{ 0x31000011, "FS_RAPI CB" },
	{ 0x31000012, "PBMLIB CB" },
	{ 0x31000013, "AUDMGR CB" },
	{ 0x31000014, "MVS CB" },
	{ 0x31000015, "DOG_KEEPALIVE CB" },
	{ 0x31000016, "GSDI_EXP CB" },
	{ 0x31000017, "AUTH CB" },
	{ 0x31000018, "NVRUIMI CB" },
	{ 0x31000019, "MMGSDILIB CB" },
	{ 0x3100001a, "CHARGER CB" },
	{ 0x3100001b, "UIM CB" },
	{ 0x3100001C, "ONCRPCTEST CB" },
	{ 0x3100001d, "PDSM_ATL CB" },
	{ 0x3100001e, "FS_XMOUNT CB" },
	{ 0x3100001f, "SECUTIL CB" },
	{ 0x31000020, "MCCMEID" },
	{ 0x31000021, "PM_STROBE_FLASH CB" },
	{ 0x31000022, "DS707_EXTIF CB" },
	{ 0x31000023, "SMD BRIDGE_MODEM CB" },
	{ 0x31000024, "SMD PORT_MGR CB" },
	{ 0x31000025, "BUS_PERF CB" },
	{ 0x31000026, "BUS_MON CB" },
	{ 0x31000027, "MC CB" },
	{ 0x31000028, "MCCAP CB" },
	{ 0x31000029, "MCCDMA CB" },
	{ 0x3100002a, "MCCDS CB" },
	{ 0x3100002b, "MCCSCH CB" },
	{ 0x3100002c, "MCCSRID CB" },
	{ 0x3100002d, "SNM CB" },
	{ 0x3100002e, "MCCSYOBJ CB" },
	{ 0x3100002f, "DS707_APIS CB" },
	{ 0x31000030, "DS_MP_SHIM_APPS_ASYNC CB" },
	{ 0x31000031, "DSRLP_APIS CB" },
	{ 0x31000032, "RLP_APIS CB" },
	{ 0x31000033, "DS_MP_SHIM_MODEM CB" },
	{ 0x31000034, "DSHDR_APIS CB" },
	{ 0x31000035, "DSHDR_MDM_APIS CB" },
	{ 0x31000036, "DS_MP_SHIM_APPS CB" },
	{ 0x31000037, "HDRMC_APIS CB" },
	{ 0x31000038, "SMD_BRIDGE_MTOA CB" },
	{ 0x31000039, "SMD_BRIDGE_ATOM CB" },
	{ 0x3100003a, "DPMAPP_OTG CB" },
	{ 0x3100003b, "DIAG CB" },
	{ 0x3100003c, "GSTK_EXP CB" },
	{ 0x3100003d, "DSBC_MDM_APIS CB" },
	{ 0x3100003e, "HDRMRLP_MDM_APIS CB" },
	{ 0x3100003f, "HDRMRLP_APPS_APIS CB" },
	{ 0x31000040, "HDRMC_MRLP_APIS CB" },
	{ 0x31000041, "PDCOMM_APP_API CB" },
	{ 0x31000042, "DSAT_APIS CB" },
	{ 0x31000043, "MISC_RF_APIS CB" },
	{ 0x31000044, "CMIPAPP CB" },
	{ 0x31000045, "DSMP_UMTS_MODEM_APIS CB" },
	{ 0x31000046, "DSMP_UMTS_APPS_APIS CB" },
	{ 0x31000047, "DSUCSDMPSHIM CB" },
	{ 0x31000048, "TIME_REMOTE_ATOM CB" },
	{ 0x3100004a, "SD CB" },
	{ 0x3100004b, "MMOC CB" },
	{ 0x3100004c, "WLAN_ADP_FTM CB" },
	{ 0x3100004d, "WLAN_CP_CM CB" },
	{ 0x3100004e, "FTM_WLAN CB" },
	{ 0x3100004f, "SDCC_CPRM CB" },
	{ 0x31000050, "CPRMINTERFACE CB" },
	{ 0x31000051, "DATA_ON_MODEM_MTOA_APIS CB" },
	{ 0x31000052, "DATA_ON_APPS_ATOM_APIS CB" },
	{ 0x31000053, "MISC_APIS_NONWINMOB CB" },
	{ 0x31000054, "MISC_APPS_APIS_NONWINMOB CB" },
	{ 0x31000055, "PMEM_REMOTE CB" },
	{ 0x31000056, "TCXOMGR CB" },
	{ 0x31000057, "DSUCSDAPPIF_APIS CB" },
	{ 0x31000058, "BT CB" },
	{ 0x31000059, "PD_COMMS_API CB" },
	{ 0x3100005a, "PD_COMMS_CLIENT_API CB" },
	{ 0x3100005b, "PDAPI CB" },
	{ 0x3100005c, "LSA_SUPL_DSM CB" },
	{ 0x3100005d, "TIME_REMOTE_MTOA CB" },
	{ 0x3100005e, "FTM_BT CB" },
	{ 0X3100005f, "DSUCSDAPPIF_APIS CB" },
	{ 0X31000060, "PMAPP_GEN CB" },
	{ 0X31000061, "PM_LIB CB" },
	{ 0X31000062, "KEYPAD CB" },
	{ 0X31000063, "HSU_APP_APIS CB" },
	{ 0X31000064, "HSU_MDM_APIS CB" },
	{ 0X31000065, "ADIE_ADC_REMOTE_ATOM CB" },
	{ 0X31000066, "TLMM_REMOTE_ATOM CB" },
	{ 0X31000067, "UI_CALLCTRL CB" },
	{ 0X31000068, "UIUTILS CB" },
	{ 0X31000069, "PRL CB" },
	{ 0X3100006a, "HW CB" },
	{ 0X3100006b, "OEM_RAPI CB" },
	{ 0X3100006c, "WMSPM CB" },
	{ 0X3100006d, "BTPF CB" },
	{ 0X3100006e, "CLKRGM_SYNC_EVENT CB" },
	{ 0X3100006f, "USB_APPS_RPC CB" },
	{ 0X31000070, "USB_MODEM_RPC CB" },
	{ 0X31000071, "ADC CB" },
	{ 0X31000072, "CAMERAREMOTED CB" },
	{ 0X31000073, "SECAPIREMOTED CB" },
	{ 0X31000074, "DSATAPI CB" },
	{ 0X31000075, "CLKCTL_RPC CB" },
	{ 0X31000076, "BREWAPPCOORD CB" },
	{ 0X31000077, "ALTENVSHELL CB" },
	{ 0X31000078, "WLAN_TRP_UTILS CB" },
	{ 0X31000079, "GPIO_RPC CB" },
	{ 0X3100007a, "PING_RPC CB" },
	{ 0X3100007b, "DSC_DCM_API CB" },
	{ 0X3100007c, "L1_DS CB" },
	{ 0X3100007d, "QCHATPK_APIS CB" },
	{ 0X3100007e, "GPS_API CB" },
	{ 0X3100007f, "OSS_RRCASN_REMOTE CB" },
	{ 0X31000080, "PMAPP_OTG_REMOTE CB" },
	{ 0X31000081, "PING_MDM_RPC CB" },
	{ 0X31000082, "PING_KERNEL_RPC CB" },
	{ 0X31000083, "TIMETICK CB" },
	{ 0X31000084, "WM_BTHCI_FTM CB" },
	{ 0X31000085, "WM_BT_PF CB" },
	{ 0X31000086, "IPA_IPC_APIS CB" },
	{ 0X31000087, "UKCC_IPC_APIS CB" },
	{ 0X31000088, "CMIPSMS CB" },
	{ 0X31000089, "VBATT_REMOTE CB" },
	{ 0X3100008a, "MFPAL CB" },
	{ 0X31000091, "HS_REM CB" },//LHX_PM_20110309_01 add for GB form hs_rem_rpc.h according svc_register_auto
	{ 0X3100008b, "DSUMTSPDPREG CB" },
	{ 0X3100fe00, "RESTART_DAEMON NUMBER 0 CB" },
	{ 0X3100fe01, "RESTART_DAEMON NUMBER 1 CB" },
	{ 0X3100feff, "RESTART_DAEMON NUMBER 255 CB" },
	{ 0X3100fffe, "BACKWARDS_COMPATIBILITY_IN_RPC_CLNT_LOOKUP CB" },
	{ 0X3100ffff, "RPC_ROUTER_SERVER_PROGRAM CB" },
		/*ZTE_HYJ_ADD_MAP_PROG_ID_TO_NAME 02/02/2001 end*/
};


static struct map_id_to_name_tbl pm_monitor_tbl[] = {
	{ pm_monitor_wakeup_reason, ARRAY_SIZE(pm_monitor_wakeup_reason) },
	{ pm_monitor_oncrpc_prog, ARRAY_SIZE(pm_monitor_oncrpc_prog) },
};

 /*
 * pm_monitor_find_name
 */
 char *pm_parse_wakeup_reason(uint32_t id, uint32_t val)
{
	int ii;
	int size;
	 struct map_id_to_name_tbl *tbl_item;

	if(id >= ID_PM_MONITOR_MAX )
		return 0;
	
	tbl_item = &pm_monitor_tbl[id];
	size = tbl_item->size;
	for(ii = 0; ii < size; ii++) {
		if (tbl_item->map[ii].val == val) {
		return tbl_item->map[ii].str;
		}
	}
	return 0;
}
/*ZTE_HYJ_PARSE_WAKEUP_INFO  2010.0126  end*/

 /*
 * get_current_time
 */
static int get_current_time(char *str_time)
{
	struct timespec ts;
	struct rtc_time tm;
	int len = 0;
	
	getnstimeofday(&ts);
	rtc_time_to_tm(ts.tv_sec, &tm);
	len = sprintf(str_time, "%d-%02d-%02d %02d:%02d:%02d <UTC> ", 
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec);
	
	return len;
}

/*
 * pm_monitor_save_info
 */
static int pm_monitor_save_info(int pm_state,char *buf)
{
	char *p_buf = buf;
	int len = 0;
	
	if (pm_state) {
 		p_buf += get_current_time(p_buf);
		p_buf+= sprintf(p_buf, "%s","suspend");
		*p_buf = '\0';
		len = p_buf - buf;
		return len;
	} else {
	//ZTE_ZHENFCHAO_20100916
		printk("[PM] additional wakeup info:\n");
		printk("\twakeupreason:0x%x\n\trpc_prog:0x%x\n\trpc_proc:0x%x\n",get_msm_pm_smem_data()->wakeup_reason,
																get_msm_pm_smem_data()->rpc_prog,
																get_msm_pm_smem_data()->rpc_proc
																);
	 	p_buf += get_current_time(p_buf);
		p_buf+= sprintf(p_buf, "%s\n","resume");
		p_buf+= sprintf(p_buf, "WakeupReason:%s\tRpcProg:%s\tRpcProc:0x%x\tSmdPortName:%s\t  GpioInfo:0x%x\n",
			/*ZTE_HYJ_PARSE_WAKEUP_INFO  2010.0126  begin*/			
			pm_parse_wakeup_reason(ID_WAKEUP_REASON,get_msm_pm_smem_data()->wakeup_reason),
			pm_parse_wakeup_reason(ID_WAKEUP_PROG,get_msm_pm_smem_data()->rpc_prog),
			/*ZTE_HYJ_PARSE_WAKEUP_INFO  2010.0126  end*/				
			get_msm_pm_smem_data()->rpc_proc,
			get_msm_pm_smem_data()->smd_port_name,
			get_msm_pm_smem_data()->reserved2);
		*p_buf = '\0';
		len = p_buf - buf;
		return len;
	}

}

/*
 * pm_state_changed
 */
static void pm_state_changed(struct pm_monitor *sdev, int state)
{
	char event_string[17];
	if (sdev->pm_state != state) {
		if (down_interruptible(&dev->sem))
			goto err;
			
	  sdev->pm_state_changed = 1;
		sdev->pm_state = state;
		wake_up_interruptible(&dev->rqueue);
		if(sdev->info_index >= WAKE_INFO_RECORD_NR) {
			sdev->info_index = 0;
		}
		up(&dev->sem);
		
		pm_monitor_save_info(state,&sdev->wakeup_info[sdev->info_index++][0]);
		
		/*ZTE_HYJ_ADD_WAKEUP_INFO_TO_LOG  01/22/10 begin*/
		if(sdev->pm_state == RESUME_STATE) {
			PMDPRINTK("%s\n",&sdev->wakeup_info[sdev->info_index - 1][0]);
			sprintf(event_string, "pm_state=resume");
			//kobject_uevent_env(&(pm_monitor_device.this_device->kobj), KOBJ_CHANGE,envp);
		} else if(sdev->pm_state == SUSPEND_STATE) {
			sprintf(event_string, "pm_state=suspend");
			//kobject_uevent_env(&(pm_monitor_device.this_device->kobj), KOBJ_CHANGE,envp);
			set_current_state(TASK_UNINTERRUPTIBLE);
			schedule_timeout(HZ/20);
		}
		/*ZTE_HYJ_ADD_WAKEUP_INFO_TO_LOG  01/22/10 end*/

err:
        return ;
	}
}

/*
 * pm_suspend_notifier
 */
static int pm_suspend_notifier(struct notifier_block *nb,
				unsigned long event,
				void *dummy)
{
	switch (event) {
		case PM_SUSPEND_PREPARE:
			printk("PM_SUSPEND_PREPARE\n");		
			pm_state_changed(dev,SUSPEND_STATE);
			return NOTIFY_OK;
		case PM_POST_SUSPEND:
			printk("PM_POST_SUSPEND\n");		
			pm_state_changed(dev,RESUME_STATE);
			return NOTIFY_OK;
		default:
			return NOTIFY_DONE;
	}
}

static struct notifier_block pm_monitor_suspend_notifier = {
	.notifier_call = pm_suspend_notifier,
};

/*huangyanjun 2010-04-02 begin ZTE_HYJ_ADD_NOTIFY_USER_WHEN_SUSPEND*/
static int pm_monitor_open(struct inode *inodp, struct file *filp)
{
	printk("%s exit\n",__FUNCTION__);
        return 0;
}

static ssize_t pm_monitor_read(struct file *filp, char __user *buff, size_t count, loff_t *f_pos)
{
	int ret = 0;
	
	printk("%s enter\n",__FUNCTION__);
	
   if (wait_event_interruptible(dev->rqueue, dev->pm_state_changed))
		goto err;

   if (down_interruptible(&dev->sem))
		goto err;

   if (copy_to_user(buff, &dev->pm_state, sizeof(dev->pm_state))) {
			ret = -EFAULT;
   } else {
			ret = sizeof(dev->pm_state);
   }
		dev->pm_state_changed = 0;
	up(&dev->sem);
	printk("%s exit\n",__FUNCTION__);        
	return ret;
	
err:
        return -ERESTARTSYS;
}


static unsigned int pm_monitor_poll(struct file *filp, struct poll_table_struct *p)
{
	unsigned int mask = 0;

	if (down_interruptible(&dev->sem))
		goto err;

	poll_wait(filp, &dev->rqueue, p);

	if(dev->pm_state_changed)	{
		mask |= POLLIN | POLLRDNORM;
	}
	up(&dev->sem);
   return mask;
		
err:
	return -ERESTARTSYS;
}

static int pm_monitor_release(struct inode *inodp, struct file *filp)
{
	printk("%s enter\n",__FUNCTION__);
	return 0;
}
/*huangyanjun 2010-04-02 end  ZTE_HYJ_ADD_NOTIFY_USER_WHEN_SUSPEND*/
/*
 * pm_monitor_init
 */
static int __init pm_monitor_init(void)
{
	int ret = -1;

	printk("%s enter\n",__FUNCTION__);

	dev = kmalloc(sizeof(struct pm_monitor), GFP_KERNEL);
	if (!dev) {
		ret = -ENOMEM;
		goto out;
	}

	memset(dev, '\0', sizeof(struct pm_monitor));
	init_MUTEX(&dev->sem);
	init_waitqueue_head(&dev->rqueue);

	ret = misc_register(&pm_monitor_device);
	if (ret)
	    goto err;
	
	ret = register_pm_notifier(&pm_monitor_suspend_notifier);
	if (ret)
		goto out_unregister;        

	ret = device_create_file(pm_monitor_device.this_device,&dev_attr_pm_state);
	ret += device_create_file(pm_monitor_device.this_device, &dev_attr_wakeup_info);
	ret += device_create_file(pm_monitor_device.this_device, &dev_attr_amss_sleep_time);//ZTE_POWER_ZENGHUIPENG_20110212 add
	
#ifdef ZTE_FEATURE_GET_CHIP_VERSION_APP		//LHX_PM_20110525_01 add code to get chip version for APP
	ret += device_create_file(pm_monitor_device.this_device, &dev_attr_chip_version);
	ret += device_create_file(pm_monitor_device.this_device, &dev_attr_hw_msm_version);
#endif
	if (ret)
		goto out_unregister;

	return 0;

out_unregister: 
	misc_deregister(&pm_monitor_device);
 err:
        kfree(dev);
out:
        printk("%s exit\n",__FUNCTION__);
        return ret;
}


/*
 *pm_monitor_exit
 */
static void __exit pm_monitor_exit(void)
{
	printk("%s enter\n",__FUNCTION__);
	
	unregister_pm_notifier(&pm_monitor_suspend_notifier);
	misc_deregister(&pm_monitor_device);
	kfree(dev);
	
	printk("%s exit\n",__FUNCTION__);
}

MODULE_LICENSE("GPL");

module_init(pm_monitor_init);
module_exit(pm_monitor_exit);
