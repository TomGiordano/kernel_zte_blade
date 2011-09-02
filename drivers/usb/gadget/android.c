/*
 * Gadget Driver for Android
 *
 * Copyright (C) 2008 Google, Inc.
 * Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
 * Author: Mike Lockwood <lockwood@android.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/* #define DEBUG */
/* #define VERBOSE_DEBUG */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>

#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/utsname.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/debugfs.h>
#include <linux/types.h>

#include <linux/usb/android_composite.h>
#include <linux/usb/ch9.h>
#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>

#include "gadget_chips.h"
#include "u_serial.h"
#include <linux/miscdevice.h>
#include <linux/wakelock.h>

/*
 * Kbuild is not very cooperative with respect to linking separately
 * compiled library objects into one module.  So for now we won't use
 * separate compilation ... ensuring init/exit sections work to shrink
 * the runtime footprint, and giving us at least some parts of what
 * a "gcc --combine ... part1.c part2.c part3.c ... " build would.
 */
#include "usbstring.c"
#include "config.c"
#include "epautoconf.c"
#include "composite.c"

MODULE_AUTHOR("Mike Lockwood");
MODULE_DESCRIPTION("Android Composite USB Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

/* product id */
static u16 global_product_id;
static int android_set_pid(const char *val, struct kernel_param *kp);
static int android_get_pid(char *buffer, struct kernel_param *kp);
module_param_call(product_id, android_set_pid, android_get_pid,
		  &global_product_id, 0664);
MODULE_PARM_DESC(product_id, "USB device product id");

static const char longname[] = "Gadget Android";

/* Default vendor and product IDs, overridden by platform data */
#define VENDOR_ID		0x18D1
#define PRODUCT_ID		0x0001
#if 0
#ifdef pr_debug
#undef pr_debug
#define pr_debug(fmt, ...)			\
	printk(fmt, ##__VA_ARGS__)
#endif
#endif

#if 1
/*COMMON*/
#define PRODUCT_ID_DIAG_MODEM_NMEA_MS_ADB          		0x1350
#define   PRODUCT_ID_ALL_INTERFACE    PRODUCT_ID_DIAG_MODEM_NMEA_MS_ADB

#define PRODUCT_ID_MS_ADB                      			0x1351
#define PRODUCT_ID_ADB                             			0x1352
#define PRODUCT_ID_MS                               			0x1353
#define PRODUCT_ID_MODEM_MS_ADB         		0x1354
#define PRODUCT_ID_MODEM_MS                 			0x1355

#define PRODUCT_ID_MS_CDROM                 			0x0083

#define PRODUCT_ID_DIAG_NMEA_MODEM   		0x0111
#define PRODUCT_ID_DIAG                           			0x0112	

/*froyo RNDIS*/
#define PRODUCT_ID_RNDIS_MS                 			0x1364
#define PRODUCT_ID_RNDIS_MS_ADB             		0x1364
#define PRODUCT_ID_RNDIS             				0x1365
#define PRODUCT_ID_RNDIS_ADB					0x1373

/*n600 pcui*/
#define PRODUCT_ID_DIAG_MODEM_NEMA_MS_ADB_AT   	0x1366
#define PRODUCT_ID_DIAG_MODEM_NEMA_MS_AT   		0x1367

/*DELL project*/
#define PRODUCT_ID_DIAG_MODEM_NMEA_MS_ADB_DELL          	0x1368
#define PRODUCT_ID_MS_ADB_DELL                      	0x1369
#define PRODUCT_ID_MS_DELL                               		0x1370
#define PRODUCT_ID_MODEM_MS_ADB_DELL         	0x1371
#define PRODUCT_ID_MODEM_MS_DELL                 	0x1372
#endif

//ruanmeisi_20100712 for cdrom auto install driver
static unsigned short g_enable_cdrom = 0;
struct usb_ex_work
{
	struct workqueue_struct *workqueue;
	int    enable_switch;
	int    enable_linux_switch;
	int switch_pid;
	int has_switch;
	int cur_pid;
	int linux_pid;
	struct delayed_work switch_work;
	struct delayed_work linux_switch_work;
	struct delayed_work plug_work;
	spinlock_t lock;
	struct wake_lock	wlock;
};

struct usb_ex_work global_usbwork = {0};
static int create_usb_work_queue(void);
static int destroy_usb_work_queue(void);
static void usb_plug_work(struct work_struct *w);
static void usb_switch_work(struct work_struct *w);
static void usb_switch_os_work(struct work_struct *w);
static void clear_switch_flag(void);
static int usb_cdrom_is_enable(void);
static int is_cdrom_enabled_after_switch(void);
struct android_usb_product *android_validate_product_id(unsigned short pid);
//end

struct android_usb {
	struct class		*class;
	struct list_head functions;
	int registered_function_count;
	atomic_t  function_count;
	
};
struct android_usb g_android_usb = {.functions = LIST_HEAD_INIT(g_android_usb.functions),};

struct android_dev {
	struct usb_composite_dev *cdev;
	struct usb_configuration *config;
	int num_products;
	struct android_usb_product *products;
	int num_functions;
	char **functions;

	int product_id;
	int version;
	int bind_num_functions;
	char **bind_functions;
	//struct android_usb *p ;
	struct mutex lock;
};

static atomic_t adb_enable_excl;

int is_adb_enabled(void)
{
	return 1 == atomic_read(&adb_enable_excl);
}

static struct android_dev *_android_dev;

static int current_pid(void)
{
	return _android_dev?_android_dev->product_id:0;
}

int support_assoc_desc(void)
{
	return current_pid() == PRODUCT_ID_RNDIS ? 0 : 1;
}


#define MAX_STR_LEN		64
/* string IDs are assigned dynamically */

#define STRING_MANUFACTURER_IDX		0
#define STRING_PRODUCT_IDX		1
#define STRING_SERIAL_IDX		2

static char serial_number[MAX_STR_LEN];

static struct kparam_string kps = {
        .string                 = serial_number,
        .maxlen                 = MAX_STR_LEN,
};

static int android_set_sn(const char *kmessage, struct kernel_param *kp);
module_param_call(serial_number, android_set_sn, param_get_string,
		  &kps, 0664);

MODULE_PARM_DESC(serial_number, "SerialNumber string");

/* String Table */
static struct usb_string strings_dev[] = {
	/* These dummy values should be overridden by platform data */
	[STRING_MANUFACTURER_IDX].s = "Android",
	[STRING_PRODUCT_IDX].s = "Android",
	[STRING_SERIAL_IDX].s = "0123456789ABCDEF",
	{  }			/* end of list */
};

static struct usb_gadget_strings stringtab_dev = {
	.language	= 0x0409,	/* en-us */
	.strings	= strings_dev,
};

static struct usb_gadget_strings *dev_strings[] = {
	&stringtab_dev,
	NULL,
};

static struct usb_device_descriptor device_desc = {
	.bLength              = sizeof(device_desc),
	.bDescriptorType      = USB_DT_DEVICE,
	.bcdUSB               = __constant_cpu_to_le16(0x0200),
	.bDeviceClass         = USB_CLASS_PER_INTERFACE,
	.idVendor             = __constant_cpu_to_le16(VENDOR_ID),
	.idProduct            = __constant_cpu_to_le16(PRODUCT_ID),
	.bcdDevice            = __constant_cpu_to_le16(0xffff),
	.bNumConfigurations   = 1,
};

static struct usb_otg_descriptor otg_descriptor = {
	.bLength =		sizeof otg_descriptor,
	.bDescriptorType =	USB_DT_OTG,
	.bmAttributes =		USB_OTG_SRP | USB_OTG_HNP,
	.bcdOTG               = __constant_cpu_to_le16(0x0200),
};

static const struct usb_descriptor_header *otg_desc[] = {
	(struct usb_descriptor_header *) &otg_descriptor,
	NULL,
};

//static struct list_head _functions = LIST_HEAD_INIT(_functions);
//static int _registered_function_count = 0;
void android_register_function(struct android_usb_function *f);

/* ruanmeisi for ftm */
//ruanmeisi_20100715 nv
enum usb_opt_nv_item
{
	NV_BACK_LIGHT_I=77,//nv77 used for config/store usb mode
	NV_FTM_MODE_I = 453// FTM mode
};
enum usb_opt_nv_type
{
	NV_READ=0,
        NV_WRITE
};
int
msm_hsusb_get_set_usb_conf_nv_value(uint32_t nv_item,uint32_t value,uint32_t is_write);
int get_ftm_from_tag(void);

#define NV_WRITE_SUCCESS 10

static int ftm_mode = 0;
static int pid_from_nv = 0;

static int is_ftm_mode(void)
{
	return !!ftm_mode;
}
static void set_ftm_mode(int i)
{
	ftm_mode = i;
	return ;
}
static int is_pid_configed_from_nv(void)
{
	return !!pid_from_nv;
}
static void set_pid_from_nv(int i)
{
	pid_from_nv = i;
	return ;
}

static int get_nv(void)
{
	return msm_hsusb_get_set_usb_conf_nv_value(NV_BACK_LIGHT_I,0,NV_READ);
}
static int set_nv(int nv)
{
	int r = msm_hsusb_get_set_usb_conf_nv_value(NV_BACK_LIGHT_I,nv,NV_WRITE);
	return (r == NV_WRITE_SUCCESS)? 0:-1;
}
static int config_pid_from_nv(void)
{
	int i = 0;
	printk("usb: %s, %d\n", __FUNCTION__, __LINE__);
	if (is_ftm_mode()) {
		return 0;
	}
	i = get_nv();
	//if nv77 == 4,  config usb pid from nv
	set_pid_from_nv(i == 4? 1:0);
	if (is_pid_configed_from_nv()) {
		printk(KERN_ERR"config pid from nv\n");
	}
	return is_pid_configed_from_nv();
}
static int config_ftm_from_tag(void)
{
	printk("usb: %s, %d\n", __FUNCTION__, __LINE__);
	if (is_ftm_mode()) {
		return 0;
	}

	set_ftm_mode(get_ftm_from_tag());

	printk("usb: %s, %d: ftm_mode %s\n",
	       __FUNCTION__, __LINE__,
	       is_ftm_mode()?"enable":"disable");
	if (is_ftm_mode()) {
		//product_id = PRODUCT_ID_DIAG;
	}
	return 0;
}
static ssize_t msm_hsusb_set_pidnv(struct device *dev,
                                   struct device_attribute *attr,
                                   const char *buf, size_t size)
{
	int value;
	sscanf(buf, "%d", &value);
	set_nv(value);
	return size;
}

static ssize_t msm_hsusb_show_pidnv(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
	int i = 0;
	i = scnprintf(buf, PAGE_SIZE, "nv %d\n", get_nv());

	return i;
}

static ssize_t msm_hsusb_show_ftm(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	int i = 0;
	i = scnprintf(buf, PAGE_SIZE, "%s\n", is_ftm_mode()?"enable":"disable");
	return i;
}

static ssize_t msm_hsusb_show_pid_from_nv(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	int i = 0;
	config_pid_from_nv();
	i = scnprintf(buf, PAGE_SIZE, "%s\n", is_pid_configed_from_nv()?"enable":"disable");
	return i;
}


struct usb_parameters {
	char p_mass_vendor_name[64];
	char p_manufacturer_name[64];
	char p_product_name[64];
	char p_serialnumber[64];
	char cdrom_enable_after_switch;
};
struct usb_parameters zte_usb_parameters = {
	.p_mass_vendor_name = {"ZTE"},
	.p_manufacturer_name = {"ZTE Incorporated"},
	.p_product_name	   = {"ZTE HSUSB Device"},
	.p_serialnumber = {"ZTE_andorid"},
	.cdrom_enable_after_switch = 0,
};

static int is_cdrom_enabled_after_switch(void)
{
	return zte_usb_parameters.cdrom_enable_after_switch;
}
static void enable_cdrom_after_switch(int enable)
{
	zte_usb_parameters.cdrom_enable_after_switch = !!enable;
}

	
#define android_parameter_attr(parameter)                                                \
static void set_android_usb_##parameter(char * param)                                    \
{                                                                                        \
	strncpy(zte_usb_parameters.p_##parameter,			\
		param,                                                                   \
		sizeof(zte_usb_parameters.p_##parameter));                               \
    zte_usb_parameters.p_##parameter[sizeof(zte_usb_parameters.p_##parameter) - 1] = 0;  \
	return ;                                                                         \
}                                                                                        \
static char* get_android_usb_##parameter(void)                                           \
{                                                                                        \
	return zte_usb_parameters.p_##parameter;                                         \
}                                                                                        \
static ssize_t  show_android_usb_##parameter(struct device *dev,                         \
				struct device_attribute *attr, char *buf)                \
{                                                                                        \
	return sprintf(buf, "%s\n", get_android_usb_##parameter());                      \
}                                                                                        \
static ssize_t store_android_usb_##parameter(struct device *dev,                         \
					       struct device_attribute *attr,            \
					       const char* buf, size_t size)             \
{                                                                                        \
	set_android_usb_##parameter((char*)buf);		                         \
        return size;                                                                     \
}                                                                                        \
static DEVICE_ATTR(parameter, S_IRUGO | S_IWUSR, show_android_usb_##parameter,           \
		   store_android_usb_##parameter);                                      

android_parameter_attr(mass_vendor_name);
android_parameter_attr(manufacturer_name);
android_parameter_attr(product_name);
android_parameter_attr(serialnumber);

static int android_set_sn(const char *kmessage, struct kernel_param *kp)
{
        int len = strlen(kmessage);

        if (len >= MAX_STR_LEN) {
                pr_err("serial number string too long\n");
                return -ENOSPC;
        }

        strlcpy(serial_number, kmessage, MAX_STR_LEN);
        /* Chop out \n char as a result of echo */
        if (serial_number[len - 1] == '\n')
                serial_number[len - 1] = '\0';
	set_android_usb_serialnumber(serial_number);

        return 0;
}

static ssize_t msm_hsusb_show_exwork(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	int i = 0;
	struct usb_ex_work *p = &global_usbwork;
	i = scnprintf(buf, PAGE_SIZE,
				"auto switch: %s\nlinux switch: %s\nswitch_pid: 0x%x\ncur_pid: 0x%x\nlinux_pid: 0x%x\n",
		      p->enable_switch?"enable":"disable",
		      p->enable_linux_switch?"enable":"disable",
		      p->switch_pid,
		      p->cur_pid,
		      p->linux_pid);
	
	return i;
}
static ssize_t msm_hsusb_store_enable_switch(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t size)
{
     unsigned long enable;
      	if (!strict_strtoul(buf, 16, &enable)) {
		global_usbwork.enable_switch = enable;
	} 
	return size;
}


static ssize_t msm_hsusb_store_enable_linux_switch(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t size)
{
     unsigned long enable;
      	if (!strict_strtoul(buf, 16, &enable)) {
		global_usbwork.enable_linux_switch = enable;
	} 
	return size;
}

static ssize_t msm_hsusb_store_cur_pid(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf, size_t size)
{     
    
       unsigned long value;
	if (!strict_strtoul(buf, 16, &value)) {
      		printk(KERN_ERR"usb: cur_pid value=0x%x\n",(unsigned int)value);
		if(android_validate_product_id((unsigned short)value))
			global_usbwork.cur_pid=(unsigned int) value;
	}
	return size;
}


static ssize_t msm_hsusb_store_switch_pid(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf, size_t size)
{     
     
       unsigned long value;
	if (!strict_strtoul(buf, 16, &value)) {
      		printk(KERN_ERR"usb: switch_pid value=0x%x\n",(unsigned int)value);
		if(android_validate_product_id((u16)value))
			global_usbwork.switch_pid= (unsigned int)value;
	}
	
	return size;
}

static ssize_t msm_hsusb_store_linux_pid(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf, size_t size)
{     
   
       unsigned long value;
	if (!strict_strtoul(buf, 16, &value)) {
      		printk(KERN_ERR"usb: linux_pid value=0x%x\n",(unsigned int)value);
		if(android_validate_product_id((u16)value))
			global_usbwork.linux_pid= (unsigned int)value;
	}

	return size;
}

static ssize_t msm_hsusb_show_enable_cdrom(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	int i = 0;	
	i = scnprintf(buf, PAGE_SIZE, "is cdrom enabled after switch? [%s]\n",
		      is_cdrom_enabled_after_switch()?"enable":"disable");	
	return i;
}

static ssize_t msm_hsusb_store_enable_cdrom(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t size)
{
       unsigned long cdrom_enable = 0;
      	if (!strict_strtoul(buf, 16, &cdrom_enable)) {
		enable_cdrom_after_switch((cdrom_enable == 0x5656)? 1 : 0);
		pr_info("%s: Requested enable_cdrom = %d 0x%lx\n",
			__func__, g_enable_cdrom, cdrom_enable);	
	} else {
		pr_info("%s: strict_strtoul conversion failed, %s\n", __func__, buf);
	}
	return size;
}

static DEVICE_ATTR(exwork, 0664,
		   msm_hsusb_show_exwork, NULL);
static DEVICE_ATTR(enable_switch, 0664,
		   NULL, msm_hsusb_store_enable_switch);

static DEVICE_ATTR(linux_switch, 0664,
		   NULL, msm_hsusb_store_enable_linux_switch);
static DEVICE_ATTR(cur_pid, 0664,
		   NULL, msm_hsusb_store_cur_pid);
static DEVICE_ATTR(switch_pid, 0664,
		   NULL, msm_hsusb_store_switch_pid);
static DEVICE_ATTR(linux_pid, 0664,
		   NULL, msm_hsusb_store_linux_pid);
static DEVICE_ATTR(enable_cdrom, 0664,
		   msm_hsusb_show_enable_cdrom, msm_hsusb_store_enable_cdrom);


//ruanmeisi_20110110



static DEVICE_ATTR(pidnv, 0664,
                   msm_hsusb_show_pidnv, msm_hsusb_set_pidnv);
static DEVICE_ATTR(ftm_mode, 0664,
                   msm_hsusb_show_ftm, NULL);
static DEVICE_ATTR(pid_from_nv, 0664,
                   msm_hsusb_show_pid_from_nv, NULL);


static struct attribute *android_attrs[] = {
	&dev_attr_pidnv.attr,
	&dev_attr_ftm_mode.attr,
	&dev_attr_pid_from_nv.attr,
	&dev_attr_manufacturer_name.attr,
        &dev_attr_product_name.attr,
        &dev_attr_mass_vendor_name.attr,
	&dev_attr_serialnumber.attr,
	&dev_attr_enable_cdrom.attr,
	//for cdrom and auto install driver
	&dev_attr_exwork.attr,
	&dev_attr_cur_pid.attr,
	&dev_attr_switch_pid.attr,
	&dev_attr_linux_pid.attr,
	&dev_attr_linux_switch.attr,
	&dev_attr_enable_switch.attr,
	NULL,
};



static struct attribute_group android_attr_grp = {
	.name = "functions",
	.attrs = android_attrs,
};


/*  end */


//copy from composiste.c ruanmeisi 20110114

#ifdef CONFIG_USB_F_SERIAL

#include "u_serial.c"
#include "f_serial.c"

int fserial_nmea_bind_config(struct usb_configuration *c)
{
	return gser_bind_config(c, 1);
}

static struct android_usb_function nmea_function = {
	.name = "nmea",
	.bind_config = fserial_nmea_bind_config,
};

int fserial_at_bind_config(struct usb_configuration *c)
{
	return gser_bind_config(c, 2);
}

static struct android_usb_function at_function = {
	.name = "at",
	.bind_config = fserial_at_bind_config,
};


int fserial_modem_bind_config(struct usb_configuration *c)
{
	int ret;

	/* See if composite driver can allocate
	 * serial ports. But for now allocate
	 * two ports for modem and nmea.
	 */
	ret = gserial_setup(c->cdev->gadget, 3);
	if (ret)
		return ret;
	return gser_bind_config(c, 0);
}

int fserial_modem_unbind_config(struct usb_configuration *c)
{
	

	/* See if composite driver can allocate
	 * serial ports. But for now allocate
	 * two ports for modem and nmea.
	 */
	gserial_cleanup();
	return 0;
}


static struct android_usb_function modem_function = {
	.name = "modem",
	.bind_config = fserial_modem_bind_config,
	.unbind_config = fserial_modem_unbind_config,
};

#endif /* CONFIG_USB_F_SERIAL */

#ifdef CONFIG_USB_ANDROID_DIAG
#include "f_diag.c"

static struct usb_diag_ch *diag_ch = NULL;
static struct usb_diag_platform_data usb_diag_pdata = {
	.ch_name = DIAG_LEGACY,
};




static int diag_bind_config(struct usb_configuration *c)
{
	struct diag_context *dev = NULL;
/* set up diag channel */
	if (NULL == diag_ch) {
		printk(KERN_ERR"usb: %s %d: diag bind err\n",
		       __FUNCTION__, __LINE__);
		return -1;
	} else
		dev = container_of(diag_ch, struct diag_context, ch);
	return diag_function_add(c, dev);
}

static int diag_init(void)
{
	struct diag_context *dev = NULL;
	diag_ch = diag_setup(&usb_diag_pdata);
	if (IS_ERR(diag_ch))
		return PTR_ERR(diag_ch);
	dev = container_of(diag_ch, struct diag_context, ch);
	/* claim the channel for this USB interface */
	diag_ch->priv_usb = dev;
	return 0;
}

static int diag_deinit(void)
{
	if (NULL == diag_ch) {
		return 0;
	}
	diag_cleanup(diag_ch);
	diag_ch = NULL;
	return 0;
}

static int diag_unbind_config(struct usb_configuration *c)
{
	return 0;
}


static struct android_usb_function diag_function = { 
	.name = "diag",
	.bind_config = diag_bind_config,
	.unbind_config = diag_unbind_config,
};


#endif
/* mass storage  */
#ifdef CONFIG_USB_ANDROID_ADB
int  adb_init(void);
int adb_deinit(void);
int adb_bind_config(struct usb_configuration *c);

static struct android_usb_function adb_function = {
	.name = "adb",
	.bind_config = adb_bind_config,
};

#endif

#ifdef CONFIG_USB_ANDROID_RNDIS
#include "f_rndis.c"
#include "rndis.c"
#include "u_ether.c"

#undef DBG     /* u_ether.c has broken idea about macros */
#undef VDBG    /* so clean up after it */
#undef ERROR
#undef INFO

static u8 hostaddr[ETH_ALEN];

int rndis_function_bind_config(struct usb_configuration *c)
{
	int ret = 0;
	ret = rndis_bind_config(c, hostaddr);
	return ret;
}

int rndis_function_unbind_config(struct usb_configuration *c)
{
	return 0;
}

static struct android_usb_function rndis_function = {
	.name = "rndis",
	.bind_config = rndis_function_bind_config,
	.unbind_config = rndis_function_unbind_config,
};

static int  android_rndis_init(void)
{
	printk(KERN_INFO "f_rndis init\n");
	return 0;
}
static int  android_rndis_deinit(void)
{
  	gether_cleanup();
	return 0;
}

#endif


#ifdef CONFIG_USB_ANDROID_MASS_STORAGE 
#include "f_mass_storage.c"

static struct fsg_config fsg_cfg;

static int get_nuluns(void)
{
	if (usb_cdrom_is_enable() || is_cdrom_enabled_after_switch()) {
		return 2;
	}
	return 1;		
}

struct fsg_config* init_fsg_cfg(void)
{
	int i, nluns;
	nluns = get_nuluns();
	if (nluns > FSG_MAX_LUNS)
		nluns = FSG_MAX_LUNS;
	fsg_cfg.nluns = nluns;
	for (i = 0; i < nluns; i++)
		fsg_cfg.luns[i].removable = 1;

	fsg_cfg.vendor_name = (const char *)get_android_usb_mass_vendor_name();
	fsg_cfg.product_name = "Mass Storage";
	fsg_cfg.release = 0x0100;
	fsg_cfg.can_stall = 0;
	fsg_cfg.luns[0].cdrom = 0;
	fsg_cfg.luns[1].cdrom = 1;
	return &fsg_cfg;
}

static int mass_storage_bind_config(struct usb_configuration *c)
{
	struct fsg_common *common = fsg_common_init(NULL, c->cdev,
						    init_fsg_cfg());
	int rc = 0;
	if (IS_ERR(common))
		return -1;
	 memcpy(common->inquiry_string,
               get_android_usb_mass_vendor_name(), strlen(get_android_usb_mass_vendor_name()));
	memcpy(common->inquiry_string+8, "Mass storage", sizeof("Mass storage"));
	rc = fsg_add(c->cdev, c, common);
	fsg_common_put(common);
	return rc;
}

static struct android_usb_function mass_storage_function = { 
	.name = FUNCTION_NAME,
	.bind_config = mass_storage_bind_config,
};

#endif

/* mass storage end */

static void usb_function_set_enabled(struct android_usb_function *f, int enabled)
{
	f->disabled = !enabled;
	kobject_uevent(&f->dev->kobj, KOBJ_CHANGE);
}


static ssize_t enable_show(struct device *dev, struct device_attribute *attr,
			   char *buf)
{
	struct android_usb_function *f = dev_get_drvdata(dev);
	return sprintf(buf, "%d\n", !f->disabled);
}

static ssize_t enable_store(
	struct device *dev, struct device_attribute *attr,
	const char *buf, size_t size)
{
	//struct android_usb_function *f = dev_get_drvdata(dev);

	/*int value;*/

	/* sscanf(buf, "%d", &value); */
	/* if (driver->enable_function) */
	/* 	driver->enable_function(f, value); */
	/* else */
	/* 	usb_function_set_enabled(f, value); */

	return size;
}

static DEVICE_ATTR(enable, S_IRUGO | S_IWUSR, enable_show, enable_store);

static int
composite_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	struct android_usb_function *f = dev_get_drvdata(dev);

	if (!f) {
		/* this happens when the device is first created */
		return 0;
	}

	if (add_uevent_var(env, "FUNCTION=%s", f->name))
		return -ENOMEM;
	if (add_uevent_var(env, "ENABLED=%d", !f->disabled))
		return -ENOMEM;
	return 0;
}


int create_sysif(struct android_usb *driver)
{
	driver->class = class_create(THIS_MODULE, "usb_composite");
	if (IS_ERR(driver->class))
		return PTR_ERR(driver->class);
	driver->class->dev_uevent = composite_uevent;
	return 0;
}

void destroy_sysif(struct android_usb *driver)
{
	class_destroy(driver->class);
	driver->class = NULL;
}


int unregister_android_function(struct android_usb_function *f)
{
	//struct android_dev *dev = _android_dev;
	if (NULL == f) {
		printk(KERN_ERR"usb:%s%d unvaild function\n",
		       __FUNCTION__, __LINE__);
		return -1;
	}
	device_remove_file(f->dev, &dev_attr_enable);
	device_destroy(g_android_usb.class,
		       f->dev->devt);
	return 0;
}

int register_android_function(struct android_usb_function *f)
{
	int	value = -EINVAL;
	int index;
	
	printk(KERN_ERR "%s %d %s\n", __FUNCTION__, __LINE__, f->name);

	if (!g_android_usb.class) {
		create_sysif(&g_android_usb);
	}
	index = atomic_inc_return(&g_android_usb.function_count);
	f->dev = device_create(g_android_usb.class, NULL,
			       MKDEV(0, index), NULL, f->name);
	if (IS_ERR(f->dev)) {
		return PTR_ERR(f->dev);
	}

	value = device_create_file(f->dev, &dev_attr_enable);
	if (value < 0) {
		atomic_dec_return(&g_android_usb.function_count);
		device_destroy(g_android_usb.class, MKDEV(0, index));
		return value;
	}

	dev_set_drvdata(f->dev, f);
	f->disabled = 1;
	/* bind our functions if they have all registered
	 * and the main driver has bound.
	 */
	list_add_tail(&f->list, &g_android_usb.functions);
	g_android_usb.registered_function_count++;
	return 0;
}


static int unregister_android_all_function(void)
{
	//struct android_dev *dev = _android_dev;
	struct android_usb_function *f = NULL;
	while (!list_empty(&g_android_usb.functions)) {
		f = list_first_entry(&g_android_usb.functions,
				     struct android_usb_function, list);
		list_del(&f->list);
		unregister_android_function(f);
	}
	return 0;
	
}



static struct android_usb_function *get_function(const char *name)
{
	struct android_usb_function	*f;

	list_for_each_entry(f, &g_android_usb.functions, list) {
		if (!strcmp(name, f->name))
			return f;
	}
	return 0;
}


static int product_all_func_registered(char **functions, int num_function)
{
	char** pr_func = functions;
	int count = num_function;
	int i;
	for(i = 0; i < count; i++) {
		//one of the function in product has not been register
		if(!get_function(*pr_func++))
			return 0;
	}
	//all function in product has been register, should start product function bind
	return 1;
}



static void bind_functions(struct android_dev *dev)
{

	if (NULL == dev || NULL == dev->bind_functions) {
		printk(KERN_ERR"usb:err %s %d\n", __FUNCTION__, __LINE__);
		return ;
	}

	if (product_all_func_registered(dev->bind_functions,
					   dev->bind_num_functions)) {
		struct android_usb_function *f;
		char** functions = dev->bind_functions;	
		int i = 0;

		for(i = 0; i < dev->bind_num_functions; i++) {
			char* name = *functions++;
			printk(KERN_ERR "function %s in bind_functions\n", name);
			f = get_function(name);
			if(f) {
				f->bind_config(dev->config);
				usb_function_set_enabled(f, 1);
			} else {
				printk(KERN_ERR "usb: err %s %d: %s not found\n",
				       __FUNCTION__, __LINE__, name);
			}
		}
	} else {
		printk(KERN_ERR "usb: some function hasn't binded %s %d\n",
		       __FUNCTION__, __LINE__);
	}
}



static int android_bind_config(struct usb_configuration *c)
{
	struct android_dev *dev = _android_dev;

	printk(KERN_DEBUG "android_bind_config\n");
	dev->config = c;

	/* bind our functions if they have all registered */
	if (g_android_usb.registered_function_count >= dev->num_functions)
		bind_functions(dev);
	return 0;
}

static int android_setup_config(struct usb_configuration *c,
				const struct usb_ctrlrequest *ctrl);

static struct usb_configuration android_config_driver = {
	.label		= "android",
	.bind		= android_bind_config,
	.setup		= android_setup_config,
	.bConfigurationValue = 1,
	.bMaxPower	= 0xFA, /* 500ma */
};

static int android_setup_config(struct usb_configuration *c,
				const struct usb_ctrlrequest *ctrl)
{
	int i;
	int ret = -EOPNOTSUPP;

	for (i = 0; i < android_config_driver.next_interface_id; i++) {
		if (android_config_driver.interface[i]->setup) {
			ret = android_config_driver.interface[i]->setup(
				android_config_driver.interface[i], ctrl);
			if (ret >= 0)
				return ret;
		}
	}
	return ret;
}

static int is_function_enable(char *func)
{
	struct android_dev *dev = _android_dev;
	int i = 0;
	char** functions = NULL;
	int num_functions = 0;
	if (NULL == func || NULL == dev || NULL == dev->bind_functions) {
		return 0;
	}

	functions = dev->bind_functions;	
	num_functions = dev->bind_num_functions;
	
	for(i = 0; i < num_functions; i++) {
		char* name = *functions++;
		if (!strncmp(func, name, strlen(name))) {
			return 1;
		}
	}
	return 0;
}

int is_iad_enabled(void) {
	if(is_function_enable("rndis"))
		return 1;
	return 0;
}


static int android_bind(struct usb_composite_dev *cdev)
{
	struct android_dev *dev = _android_dev;
	struct usb_gadget	*gadget = cdev->gadget;
	int			gcnum, id, /*product_id, */ret;

	printk(KERN_INFO "android_bind\n");

	/* Allocate string descriptor numbers ... note that string
	 * contents can be overridden by the composite_dev glue.
	 */
	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_dev[STRING_MANUFACTURER_IDX].id = id;
	device_desc.iManufacturer = id;

	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_dev[STRING_PRODUCT_IDX].id = id;
	device_desc.iProduct = id;

	if (!is_ftm_mode() && !is_pid_configed_from_nv()) {
		id = usb_string_id(cdev);
		if (id < 0)
			return id;
		strings_dev[STRING_SERIAL_IDX].id = id;
		device_desc.iSerialNumber = id;
	} else {
		strings_dev[STRING_SERIAL_IDX].id = 0;
		device_desc.iSerialNumber = 0;
	}

	/* For switch composition */
	if (current_pid())
		device_desc.idProduct = __constant_cpu_to_le16(current_pid());
	printk(KERN_ERR"device_desc.product_id = %x\n_android_dev.product_id = %x\n", 
	       device_desc.idProduct,
	       current_pid());

	if (gadget_is_otg(cdev->gadget))
		android_config_driver.descriptors = otg_desc;

	if (!usb_gadget_set_selfpowered(gadget))
		android_config_driver.bmAttributes |= USB_CONFIG_ATT_SELFPOWER;
	
	if (gadget->ops->wakeup)
		android_config_driver.bmAttributes |= USB_CONFIG_ATT_WAKEUP;
	
	/* register our configuration */
	#ifdef CONFIG_USB_ANDROID_RNDIS
	/* set up network link layer */
        ret = gether_setup(cdev->gadget, hostaddr);
        if (ret && (ret != -EBUSY)) {
	  printk(KERN_ERR "usb: %s %d, gether_setup err\n",
		 __FUNCTION__, __LINE__);
	  return ret;
        }
	#endif

	ret = usb_add_config(cdev, &android_config_driver);
	if (ret) {
		printk(KERN_ERR "usb_add_config failed\n");
		return ret;
	}
	
	gcnum = usb_gadget_controller_number(gadget);
	if (gcnum >= 0)
		device_desc.bcdDevice = cpu_to_le16(0x0200 + gcnum);
	else {
		/* gadget zero is so simple (for now, no altsettings) that
		 * it SHOULD NOT have problems with bulk-capable hardware.
		 * so just warn about unrcognized controllers -- don't panic.
		 *
		 * things like configuration and altsetting numbering
		 * can need hardware-specific attention though.
		 */
		pr_warning("%s: controller '%s' not recognized\n",
			   longname, gadget->name);
		device_desc.bcdDevice = __constant_cpu_to_le16(0x9999);
	}

	usb_gadget_set_selfpowered(gadget);
	dev->cdev = cdev;

	if (is_iad_enabled()) {
                device_desc.bDeviceClass         = USB_CLASS_MISC;
                device_desc.bDeviceSubClass      = 0x02;
                device_desc.bDeviceProtocol      = 0x01;
                //ruanmeisi
                if (!support_assoc_desc()) {
                        device_desc.bDeviceClass         =  USB_CLASS_WIRELESS_CONTROLLER;
                        device_desc.bDeviceSubClass      = 0;
                        device_desc.bDeviceProtocol      = 0;
                }
                //end
        } else {
                device_desc.bDeviceClass         = USB_CLASS_PER_INTERFACE;
                device_desc.bDeviceSubClass      = 0;
                device_desc.bDeviceProtocol      = 0;
        }

#if 0	
	//product_id = get_product_id(dev);
	//device_desc.idProduct = __constant_cpu_to_le16(product_id);
#endif	
	cdev->desc.idProduct = device_desc.idProduct;

	return 0;
}

static int android_unbind(struct usb_composite_dev *cdev)
{
	//struct android_dev *dev = _android_dev;
	struct android_usb_function *f = NULL;
	struct list_head *l = NULL;
	printk(KERN_INFO"usb: %s %d unbind\n",
	       __FUNCTION__, __LINE__);
	list_for_each(l, &g_android_usb.functions) {
		f = list_entry(l, struct android_usb_function, list);
		if (!f->disabled) {
			printk(KERN_ERR"usb: %s %d unbind %s\n",
			       __FUNCTION__, __LINE__, f->name);
			if (f->unbind_config) {
				f->unbind_config(NULL);
			}
			usb_function_set_enabled(f, 0);
		}
	}
	return 0;
}

static struct usb_composite_driver android_usb_driver = {
	.name		= "android_usb",
	.dev		= &device_desc,
	.strings	= dev_strings,
	.bind		= android_bind,
	.unbind		= android_unbind,
	//.enable_function = android_enable_function,
};


#define dbg_err(fmt, ...)				\
        printk(KERN_ERR pr_fmt(fmt), ##__VA_ARGS__)

static void print_all_products(void)
{
	struct android_dev *dev = _android_dev;
	struct android_usb_product *fi;
	int i;
	if (NULL == dev) {
		dbg_err("usb: %s %d err\n", __FUNCTION__, __LINE__);
		return ;
	}
	dbg_err("num functions = %d\n", dev->num_products);
	for (i = 0; i < dev->num_products; i++) {
		fi = &dev->products[i];
		dbg_err("pid=0x%x\n", fi->product_id);		
	}
}
struct android_usb_product *android_validate_product_id(unsigned short pid)
{
	struct android_dev *dev = _android_dev;
	struct android_usb_product *fi;
	int i;
	
	for (i = 0; i < dev->num_products; i++) {
		fi = &dev->products[i];
		if (fi->product_id == pid || fi->adb_product_id == pid)
			return fi;
	}
	print_all_products();
	return NULL;
}


int android_set_bind_functions(u16 pid)
{
	struct android_usb_product *func = NULL;
	struct android_dev *dev = _android_dev;

	if (NULL == dev) {
		dbg_err("usb:%s: err %x\n", __func__, pid);
		return -EINVAL;
	}
	func = android_validate_product_id(pid);
	
	if (!func) {
		dbg_err("usb:%s: invalid product id %x\n", __func__, pid);
		return -EINVAL;
	}
		
	/* DisHonour adb users */

	if (is_adb_enabled() && func->adb_product_id) {
		dev->bind_functions = func->adb_functions;
		dev->bind_num_functions = func->adb_num_functions;
		dev->product_id = func->adb_product_id;
	} else {
		dev->bind_functions = func->functions;
		dev->bind_num_functions = func->num_functions;
		dev->product_id = func->product_id;
	}
	dbg_err("%s: product id = %x num functions = %x curr pid 0x%x\n",
		__func__,
		func->product_id,
		func->num_functions,
		dev->product_id);
	return 0;
}

static int rndis_mute_switch = 0;
int get_mute_switch(void)
{
	return rndis_mute_switch ;
}
EXPORT_SYMBOL(get_mute_switch);
void set_mute_switch(int mute)
{
	rndis_mute_switch = mute;
	return ;
}
EXPORT_SYMBOL(set_mute_switch);


static int android_switch_composition(u16 pid)
{

	int ret;
	ret = android_set_bind_functions(pid);
	if (ret < 0) {
		dbg_err("%s: %d: err android_set_bind_functions \n", __func__, __LINE__);
		return ret;
	}
	printk(KERN_ERR"%s: %d: composite unregister \n", __func__, __LINE__);
	if (is_iad_enabled()) {
	set_mute_switch(1);
	}
	usb_composite_unregister(&android_usb_driver);
	printk(KERN_ERR"%s: %d: composite register \n", __func__, __LINE__);
	ret = usb_composite_register(&android_usb_driver);
	return ret;
}


static int android_set_pid(const char *val, struct kernel_param *kp)
{
	int ret = 0;
	unsigned long tmp;

	dbg_err("got val = %s\n", val);
	
	ret = strict_strtoul(val, 16, &tmp);
	if (ret) {
		dbg_err("strict_strtoul failed\n");
		goto out;
	}
	dbg_err("pid is %d\n", (unsigned int)tmp);
	/* We come here even before android_probe, when product id
	 * is passed via kernel command line.
	 */
	/* dbg_err("verifying _android_dev\n"); */
	if (!_android_dev) {
		global_product_id = tmp;
		goto out;
	}

	mutex_lock(&_android_dev->lock);
	ret = android_switch_composition(tmp);
	mutex_unlock(&_android_dev->lock);
	//ruanmeisi_20100713 for auto swith usb mode
	clear_switch_flag(); 
	//end
out:
	return strlen(val);
}

static int android_get_pid(char *buffer, struct kernel_param *kp)
{
	int ret;

	mutex_lock(&_android_dev->lock);
	ret = scnprintf(buffer, PAGE_SIZE, "%x", current_pid());
	mutex_unlock(&_android_dev->lock);

	return ret;
}

void android_register_function(struct android_usb_function *f)
{

	/* struct android_dev *dev = _android_dev; */
	
	register_android_function(f);
	
	/* if (dev->config && */
	/*     g_android_usb.registered_function_count >= dev->num_functions) { */
	/* 	bind_functions(dev); */
	/* } */
	return ;
}

#ifdef CONFIG_DEBUG_FS
static int android_debugfs_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t android_debugfs_serialno_write(struct file *file, const char
					      __user *buf,	size_t count, loff_t *ppos)
{
	char str_buf[MAX_STR_LEN];

	if (count > MAX_STR_LEN)
		return -EFAULT;

	if (copy_from_user(str_buf, buf, count))
		return -EFAULT;

	memcpy(serial_number, str_buf, count);

	if (serial_number[count - 1] == '\n')
		serial_number[count - 1] = '\0';

	strings_dev[STRING_SERIAL_IDX].s = serial_number;

	return count;
}
const struct file_operations android_fops = {
	.open	= android_debugfs_open,
	.write	= android_debugfs_serialno_write,
};

struct dentry *android_debug_root;
struct dentry *android_debug_serialno;

static int android_debugfs_init(struct android_dev *dev)
{
	android_debug_root = debugfs_create_dir("android", NULL);
	if (!android_debug_root)
		return -ENOENT;

	android_debug_serialno = debugfs_create_file("serial_number", 0220,
						     android_debug_root, dev,
						     &android_fops);
	if (!android_debug_serialno) {
		debugfs_remove(android_debug_root);
		android_debug_root = NULL;
		return -ENOENT;
	}
	return 0;
}

static void android_debugfs_cleanup(void)
{
	debugfs_remove(android_debug_serialno);
	debugfs_remove(android_debug_root);
}
#endif
static int __init android_probe(struct platform_device *pdev)
{
	struct android_usb_platform_data *pdata = pdev->dev.platform_data;
	struct android_dev *dev = _android_dev;
	int result;

	printk(KERN_INFO "android_probe pdata: %p\n", pdata);

	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);

	result = pm_runtime_get(&pdev->dev);
	if (result < 0) {
		dev_err(&pdev->dev,
			"Runtime PM: Unable to wake up the device, rc = %d\n",
			result);
		return result;
	}

	if (pdata) {
		dev->products = pdata->products;
		dev->num_products = pdata->num_products;
		dev->functions = pdata->functions;
		dev->num_functions = pdata->num_functions;
		config_ftm_from_tag();


		if (pdata->vendor_id)
			device_desc.idVendor =
				__constant_cpu_to_le16(pdata->vendor_id);
		if (pdata->product_id) {
			dev->product_id = pdata->product_id;
			device_desc.idProduct =
				__constant_cpu_to_le16(pdata->product_id);
		}
		if (is_ftm_mode()) {
			dev->product_id = PRODUCT_ID_DIAG;
			printk(KERN_ERR"dev->product_id 0x%x\n", dev->product_id);
			device_desc.idProduct =
				__constant_cpu_to_le16(PRODUCT_ID_DIAG);
		}

		if (pdata->version)
			dev->version = pdata->version;

		if (pdata->product_name) {
			set_android_usb_product_name(pdata->product_name);
		}
		strings_dev[STRING_PRODUCT_IDX].s =
				get_android_usb_product_name();
		if (pdata->manufacturer_name) {
			set_android_usb_manufacturer_name(pdata->manufacturer_name);
		}
		strings_dev[STRING_MANUFACTURER_IDX].s =
			get_android_usb_manufacturer_name();
		if (pdata->serial_number) {
			set_android_usb_serialnumber(pdata->serial_number);
		}
		strings_dev[STRING_SERIAL_IDX].s =
			get_android_usb_serialnumber();
	}
#ifdef CONFIG_DEBUG_FS
	result = android_debugfs_init(dev);
	if (result)
		pr_info("%s: android_debugfs_init failed\n", __func__);
#endif
	result = sysfs_create_group(&pdev->dev.kobj, &android_attr_grp);
        if (result < 0) {
		pr_err("%s: Failed to create the sysfs entry \n", __func__);
		return result;
        }

#ifdef CONFIG_USB_ANDROID_MASS_STORAGE 
	register_android_function(&mass_storage_function);
#endif

#ifdef CONFIG_USB_ANDROID_DIAG
	diag_init();
	register_android_function(&diag_function);
#endif

#ifdef CONFIG_USB_F_SERIAL
	register_android_function(&modem_function);
	register_android_function(&nmea_function);
	register_android_function(&at_function);
#endif
#ifdef CONFIG_USB_ANDROID_ADB
	adb_init();
	register_android_function(&adb_function);
#endif

#ifdef CONFIG_USB_ANDROID_RNDIS
	android_rndis_init();
	register_android_function(&rndis_function);
#endif
	if (dev->product_id) {
		android_set_bind_functions(dev->product_id);
		return usb_composite_register(&android_usb_driver);
	} else
		return 0;

}

static int andr_runtime_suspend(struct device *dev)
{
	dev_dbg(dev, "pm_runtime: suspending...\n");
	return 0;
}

static int andr_runtime_resume(struct device *dev)
{
	dev_dbg(dev, "pm_runtime: resuming...\n");
	return 0;
}

static struct dev_pm_ops andr_dev_pm_ops = {
	.runtime_suspend = andr_runtime_suspend,
	.runtime_resume = andr_runtime_resume,
};

static struct platform_driver android_platform_driver = {
	.driver = { .name = "android_usb", .pm = &andr_dev_pm_ops},
};



static int adb_enable_open(struct inode *ip, struct file *fp)
{
	if (atomic_inc_return(&adb_enable_excl) != 1) {
		atomic_dec(&adb_enable_excl);
		return -EBUSY;
	}

	printk(KERN_INFO "enabling adb\n");
	//android_enable_function(&_adb_dev->function, 1);
	if (_android_dev && _android_dev->product_id) {
		mutex_lock(&_android_dev->lock);
		android_switch_composition(_android_dev->product_id);
		mutex_unlock(&_android_dev->lock);
	}
	return 0;
}

static int adb_enable_release(struct inode *ip, struct file *fp)
{
	printk(KERN_INFO "disabling adb\n");
	//android_enable_function(&_adb_dev->function, 0);

	atomic_dec(&adb_enable_excl);
	
	if (_android_dev && _android_dev->product_id) {
		mutex_lock(&_android_dev->lock);
		android_switch_composition(_android_dev->product_id);
		mutex_unlock(&_android_dev->lock);
	}

	return 0;
}

static const struct file_operations adb_enable_fops = {
	.owner =   THIS_MODULE,
	.open =    adb_enable_open,
	.release = adb_enable_release,
};

static struct miscdevice adb_enable_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "android_adb_enable",
	.fops = &adb_enable_fops,
};


static int __init init(void)
{
	struct android_dev *dev;
	int ret;

	printk(KERN_ERR "android init\n");
	ret = misc_register(&adb_enable_device);
	if (ret)
		return ret;
	
	
	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	//create_sysif(dev->p);
	create_usb_work_queue();

	/* set default values, which should be overridden by platform data */
	dev->product_id = 0;
	_android_dev = dev;
	mutex_init(&dev->lock);

	return platform_driver_probe(&android_platform_driver, android_probe);
}
module_init(init);

static void __exit cleanup(void)
{
#ifdef CONFIG_DEBUG_FS
	android_debugfs_cleanup();
#endif
	usb_composite_unregister(&android_usb_driver);
	platform_driver_unregister(&android_platform_driver);
	kfree(_android_dev);
	_android_dev = NULL;
	destroy_usb_work_queue();
	unregister_android_all_function();
	misc_deregister(&adb_enable_device);

	#ifdef CONFIG_USB_ANDROID_DIAG
	diag_deinit();
	#endif
        #ifdef CONFIG_USB_ANDROID_ADB
	adb_deinit();
	#endif
	#ifdef CONFIG_USB_ANDROID_RNDIS
	android_rndis_deinit();
#endif
}
module_exit(cleanup);

//ruanmeisi_20100712 add for switch usb mode



static int create_usb_work_queue(void)
{
	struct usb_ex_work *p = &global_usbwork;
	if (p->workqueue) {
		printk(KERN_ERR"usb:workqueue has created");
		return 0;
	}
	spin_lock_init(&p->lock);
	p->enable_switch = 1;
	p->enable_linux_switch = 0;
	p->switch_pid = PRODUCT_ID_MS_ADB;
	p->linux_pid = PRODUCT_ID_MS_ADB;
	p->cur_pid = PRODUCT_ID_MS_CDROM;
	p->has_switch = 0;
	p->workqueue = create_singlethread_workqueue("usb_workqueue");
	if (NULL == p->workqueue) {
		printk(KERN_ERR"usb:workqueue created fail");
		p->enable_switch = 0;
		return -1;
	}
	INIT_DELAYED_WORK(&p->switch_work, usb_switch_work);
	INIT_DELAYED_WORK(&p->linux_switch_work, usb_switch_os_work);
	INIT_DELAYED_WORK(&p->plug_work, usb_plug_work);
	wake_lock_init(&p->wlock,
		       WAKE_LOCK_SUSPEND, "usb_switch_wlock");
	return 0;
}



static int destroy_usb_work_queue(void)
{
	struct usb_ex_work *p = &global_usbwork;
	if (NULL != p->workqueue) {
		destroy_workqueue(p->workqueue);
		p->workqueue = NULL;
	}
	wake_lock_destroy(&p->wlock);
	memset(&global_usbwork, 0, sizeof(global_usbwork));
	return 0;
}

static void usb_plug_work(struct work_struct *w)
{
	unsigned long flags;
	int pid = 0;
	struct usb_ex_work *p = container_of(w, struct usb_ex_work, plug_work.work);

	if (!_android_dev) {

		printk(KERN_ERR"usb:%s: %d: _android_dev == NULL\n",
		       __FUNCTION__, __LINE__);
		return ;
	}
	
	spin_lock_irqsave(&p->lock, flags);
	if (!p->has_switch) {
		printk("usb:rms: %s %d: \n", __FUNCTION__, __LINE__);
		spin_unlock_irqrestore(&p->lock, flags);
		return ;
	}
	printk("usb:rms: %s %d: \n", __FUNCTION__, __LINE__);
	p->has_switch = 0;
	pid = p->cur_pid;
	spin_unlock_irqrestore(&p->lock, flags);
	//enable_cdrom(1);
	//DBG("plug work");
	printk("usb:rms %s:%d pid 0x%x cur_pid 0x%x\n",
	       __FUNCTION__, __LINE__, current_pid(), pid);

	mutex_lock(&_android_dev->lock);
	wake_lock(&p->wlock);
	android_switch_composition((unsigned short)pid);
	wake_unlock(&p->wlock);
	mutex_unlock(&_android_dev->lock);

	return ;
}

static void usb_switch_work(struct work_struct *w)
{
	struct usb_ex_work *p = container_of(w, struct usb_ex_work, switch_work.work);
	unsigned long flags;
	if (!_android_dev) {

		printk(KERN_ERR"usb:%s: %d: _android_dev == NULL\n",
		       __FUNCTION__, __LINE__);
		return ;
	}
	if (!p->enable_switch) {
		return ;
	}
	if (p->has_switch) {
		printk("usb:rms:%s %d: already switch pid 0x%x switch_pid 0x%x\n",
		       __FUNCTION__, __LINE__, current_pid(), p->switch_pid);
		return ;
	}
	spin_lock_irqsave(&p->lock, flags);
//	p->cur_pid = ui->composition->product_id;
	p->has_switch = 1;
	spin_unlock_irqrestore(&p->lock, flags);
//	DBG("auto switch usb mode");
	printk("usb:rms:%s %d: pid 0x%x switch_pid 0x%x\n",
	       __FUNCTION__, __LINE__, current_pid(), p->switch_pid);
	//enable_cdrom(0);

	mutex_lock(&_android_dev->lock);
	wake_lock(&p->wlock);
	android_switch_composition((unsigned short)p->switch_pid);
	wake_unlock(&p->wlock);
	mutex_unlock(&_android_dev->lock);

	return ;
}

static void usb_switch_os_work(struct work_struct *w)
{
	struct usb_ex_work *p =
		container_of(w, struct usb_ex_work, linux_switch_work.work);
	unsigned long flags;

	if (!_android_dev) {

		printk(KERN_ERR"usb:%s: %d: _android_dev == NULL\n",
		       __FUNCTION__, __LINE__);
		return ;
	}

	if (!p->enable_switch || !p->enable_linux_switch || p->has_switch) {
		//switch  or linux_switch are enable, or we has already switch,return direct
		printk("usb:rms:%s:%d, switch %s: linux switch %s: %s switch\n",
		       __FUNCTION__, __LINE__, p->enable_switch?"enable":"disable",
		       p->enable_linux_switch?"enable":"disable",
		       p->has_switch?"has":"has not");
		return ;
	}
	spin_lock_irqsave(&p->lock, flags);
//	p->cur_pid = ui->composition->product_id;
	p->has_switch = 1;
	spin_unlock_irqrestore(&p->lock, flags);
	printk("usb:rms:%s %d: pid 0x%x linux_pid 0x%x\n",
	       __FUNCTION__, __LINE__, current_pid(), p->linux_pid);

	mutex_lock(&_android_dev->lock);
	wake_lock(&p->wlock);
	android_switch_composition((unsigned short)p->linux_pid);
	wake_unlock(&p->wlock);
	mutex_unlock(&_android_dev->lock);

	return ;
}

void schedule_cdrom_stop(void)
{
	
	if (NULL == global_usbwork.workqueue) {
		return ;
	}
	queue_delayed_work(global_usbwork.workqueue, &global_usbwork.switch_work, HZ/10);

	return;
}
EXPORT_SYMBOL(schedule_cdrom_stop);
void schedule_linux_os(void)
{
	if (NULL == global_usbwork.workqueue) {
		return ;
	}
	queue_delayed_work(global_usbwork.workqueue,
			   &global_usbwork.linux_switch_work, 0);

	return;
}
EXPORT_SYMBOL(schedule_linux_os);

void schedule_usb_plug(void)
{
	
	if (NULL == global_usbwork.workqueue) {
		return ;
	}
	printk("usb:rms: %s %d: \n", __FUNCTION__, __LINE__);
	queue_delayed_work(global_usbwork.workqueue, &global_usbwork.plug_work, 0);

	return ;
}
EXPORT_SYMBOL(schedule_usb_plug);

static void clear_switch_flag(void)
{
	unsigned long flags;
	struct usb_ex_work *p = &global_usbwork;
	spin_lock_irqsave(&p->lock, flags);
	p->has_switch = 0;
	spin_unlock_irqrestore(&p->lock, flags);

	return ;
}


static int usb_cdrom_is_enable(void)
{
	return (PRODUCT_ID_MS_CDROM == current_pid()) ? 1:0;
}


int os_switch_is_enable(void)
{
	struct usb_ex_work *p = &global_usbwork;
	
	return usb_cdrom_is_enable()?(p->enable_linux_switch) : 0;
}
EXPORT_SYMBOL(os_switch_is_enable);
//end
