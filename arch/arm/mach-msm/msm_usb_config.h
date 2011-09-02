/*从board-xxx.c整理过来的配置相关代码*/
 /*
 ========================================================================================
when        who           what, where, why                                comment tag
----------  ----------  ----------------------------------------------  -----------------
2010-03-31  HML      delete unused micro                             none
2010-05-06  wangzy    add pid 1354 1355                              ZTE_USBCONFIG_001                           
2010-05-28  rms      modify serialnumber

*/
#include <mach/msm_hsusb.h>
#include <linux/usb/mass_storage_function.h>

#ifdef CONFIG_USB_FUNCTION
struct usb_mass_storage_platform_data usb_mass_storage_pdata = {
	.nluns          = 0x01,            
	.buf_size       = 16384,
	.vendor         = "ZTE",
	.product        = "Mass Storage",
	.release        = 0xffff,
};

struct usb_function_map usb_functions_map[] = {
	{"diag", 0},
	{"modem", 1},
	{"nmea", 2},
	{"mass_storage", 3},
	{"adb", 4},
        //{"ethernet", 5},
};

/* dynamic composition */
struct usb_composition usb_func_composition[] = {
	
	{
		.product_id     = 0x0112,
		.functions	    = 0x01, /* 000001 */
	},

	{
		.product_id     = 0x0111,
		.functions	    = 0x07, /* 000111 */
	},
	
	{
		.product_id     = 0x1355, /*ZTE_USBCONFIG_001*/
		.functions	    = 0x0A, /* 001010 */
	},

	{
		.product_id     = 0x1354, /*ZTE_USBCONFIG_001*/
		.functions	    = 0x1A, /* 011010 */
	},

	{
		.product_id     = 0x1353,
		.functions	    = 0x08, /* 001000: ms */
	},
	
	{
		.product_id     = 0x0083,
		.functions	    = 0x08, /* 001000: ms +cdrom*/
	},

	{
		.product_id     = 0x1352,
		.functions	    = 0x10, /* 010000 */
	},

	{
		.product_id     = 0x1351,
		.functions	    = 0x18, /* 011000 */
	},

	{
		.product_id     = 0x1350,
		.functions	    = 0x1F, /* 011111 */
	},

};
#endif

struct msm_hsusb_platform_data msm_hsusb_pdata = {
#ifdef CONFIG_USB_FUNCTION
	.version	= 0x0100,
	.phy_info	= (USB_PHY_INTEGRATED | USB_PHY_MODEL_65NM),
	.product_name       = "ZTE HSUSB Device",
	.manufacturer_name  = "ZTE Incorporated",

	.vendor_id          = 0x19d2,
	.serial_number      = "ZTE-HSUSB",

	.compositions	= usb_func_composition,
	.num_compositions = ARRAY_SIZE(usb_func_composition),
	.function_map   = usb_functions_map,
	.num_functions	= ARRAY_SIZE(usb_functions_map),
	.config_gpio    = NULL,
#endif
};
