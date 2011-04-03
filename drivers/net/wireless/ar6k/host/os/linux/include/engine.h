/*
 * athloader.h - definitions for atheros firmware loader.
 *
 * Firmware image format:
 *
 * unsigned char 0xFF                //  "magic"
 * unsigned char version             //  fw loader / tool version, not fw version major/minor 4 bits each
 * char hostname[variable]           //  zero delimited hostname where the image was built
 * unsigned char month               //
 * unsigned char day                 //  Date and time when the image was built
 * unsigned char year                //  year is a number of years after 2008, i.e. 2008 is coided as 0
 * unsigned char hour                //
 * unsigned char min                 //
 * // the rest are optional blocks that may be repeated and/or mixed
 * unsigned char instruction
 *   unsigned char argument[4]       // optional constant, mask or offset
 *   unsigned char image[variable]   // optional loadable image
 * // repeated as necessary
 * unsigned char crc32[4]            // inverted crc32
 *
 */
#ifndef ATHLOADER_IS_SEEN
#define ATHLOADER_IS_SEEN

#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)
/* ATHENV */
#ifndef ANDROID_ENV
#define FW_AUTOLOAD
#endif
/* ATHENV */
/* Uncomment the following to build standalone application for testing 
 
#define UNIT_TEST
#define syserr printf
#define sysprint printf
#include "crc32.h"
#include <stdio.h>
 */

#ifndef UNIT_TEST
#if defined(__linux__) || defined(LINUX)
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/crc32.h>
#include <linux/firmware.h>
#define syserr(arg...) printk( KERN_ERR arg)
#define sysprint(arg...) printk( KERN_INFO arg)
#endif /* __linux__ */
#endif /* UNIT_TEST */

#define VERSION (0x01)

/* Opcodes */
#define RLoad (0x10)
#define Ror   (0x20)
#define Rand  (0x30)
#define Add   (0x40)
#define Rstor (0x50)
#define Shift (0x60)
#define Nneg  (0x70)
#define Trr   (0x80)
#define Trw   (0x90)
#define Trx   (0xA0)
#define Exit  (0xB0)
#define Cmp   (0xC0)
#define Ldprn (0xD0)
#define Jump  (0xE0)

extern unsigned int get_target_reg( unsigned address, void *ar );
extern int write_target_reg( unsigned address, unsigned value, void *ar );
extern int execute_on_target( unsigned int address, unsigned int arg, void *ar );
extern int load_binary(unsigned int addr, unsigned char *data, void *ar );
extern void bmidone( void *ar );
#endif /* KERNEL 2.6 only */
#endif /* ATHLOADER_IS_SEEN */
