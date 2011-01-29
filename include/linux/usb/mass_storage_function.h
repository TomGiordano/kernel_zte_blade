/*
 *  Switch class driver
 *
 * Copyright (C) 2008 Google, Inc.
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

#ifndef __LINUX_USB_MASS_STORAGE_FUNCTION_H__
#define __LINUX_USB_MASS_STORAGE_FUNCTION_H__

/* platform data for USB mass storage function driver */
struct usb_mass_storage_platform_data {
	/* number of logical units */
	int         nluns;
	
	/* buffer size for bulk transfers */
	u32			buf_size;
	
	/* values for UMS DO_INQUIRY command */
	const char  *vendor;
	const char  *product;
	int			release;
};

#endif /* __LINUX_USB_MASS_STORAGE_FUNCTION_H__ */
