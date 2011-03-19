/*
 * Copyright (c) 2006 Atheros Communications, Inc.
 * All rights reserved.
 * 
 *
 * 
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
 * 
 */
/*history 
 * when        who      what why                       tag
 * 2010-01-06   hp     fix CQ431832 and CQ430643      ZTE_WIFI_HP_007
 * */

#include "ar6000_drv.h"
#include "htc.h"
#include <linux/fs.h>

#include "AR6002/hw2.0/hw/apb_map.h"
#include "AR6002/hw2.0/hw/gpio_reg.h"
#include "AR6002/hw2.0/hw/rtc_reg.h"
#include "AR6002/hw2.0/hw/si_reg.h"

//
// defines
//

#define MAX_FILENAME 1023
#define EEPROM_WAIT_LIMIT 16 

#define HOST_INTEREST_ITEM_ADDRESS(item)          \
        (AR6002_HOST_INTEREST_ITEM_ADDRESS(item))

#define EEPROM_SZ 768

#define ATH_MAC_LEN                         6
#define ATH_SOFT_MAC_TMP_BUF_LEN            64
unsigned char mac_addr[ATH_MAC_LEN];
unsigned char soft_mac_tmp_buf[ATH_SOFT_MAC_TMP_BUF_LEN];


//ZTE_WIFI_HP_007 
#ifdef SAVE_EEPROM_TO_FILE
#define SAVED_EEPROM_FILE "/data/misc/wifi/athSavedEeprom.bin"
extern int ar6k_saveEepromFile(char *fname, char *buf, int len);
extern int ar6k_eepromfileExist(char *fname);
extern int ar6k_isEepromFileValid(char *fname);
#endif 
//ZTE_WIFI_HP_007 end 

//
// static variables
//

static A_UCHAR eeprom_data[EEPROM_SZ];
static A_UINT32 sys_sleep_reg;
static HIF_DEVICE *p_bmi_device;

//
// Functions
//

// return 0: fail, 1: success
static int
wmic_ether_aton(const char *orig, A_UINT8 *eth)
{
  const char *bufp;
  int i;

  i = 0;
  for(bufp = orig; *bufp != '\0'; ++bufp) {
    unsigned int val;
    unsigned char c;

    c = *bufp++;

    if (c >= '0' && c <= '9') val = c - '0';
    else if (c >= 'a' && c <= 'f') val = c - 'a' + 10;
    else if (c >= 'A' && c <= 'F') val = c - 'A' + 10;
    else {
        printk("%s: MAC value is invalid\n", __FUNCTION__);
        break;
    }

    val <<= 4;
    c = *bufp++;
    if (c >= '0' && c <= '9') val |= c - '0';
    else if (c >= 'a' && c <= 'f') val |= c - 'a' + 10;
    else if (c >= 'A' && c <= 'F') val |= c - 'A' + 10;
    else {
        printk("%s: MAC value is invalid\n", __FUNCTION__);
        break;
    }

    eth[i] = (unsigned char) (val & 0377);
    if(++i == ATH_MAC_LEN) {
	    /* That's it.  Any trailing junk? */
        c = *bufp;
	    if ((c != '\0') && (c != 0x20) && (c != 0x0a) && (c != 0x0d)) {
            printk("wmic_ether_aton end char is not correct [%c:0x%x]\n", c, c);
		    return 0;
	    }
	    return 1;
    }
    c = *bufp;
    if (c != ':') {
        printk("wmic_ether_aton: c[%c:0x%x] is not :\n", c,c);
	    break;
    }
  }
  return 0;
}

static void
update_mac(unsigned char* eeprom, int size, unsigned char* macaddr)
{
	int i;
	A_UINT16* ptr = (A_UINT16*)(eeprom+4);
	A_UINT16  checksum = 0;

	memcpy(eeprom+10,macaddr,6);

	*ptr = 0;
	ptr = (A_UINT16*)eeprom;

	for (i=0; i<size; i+=2) {
		checksum ^= *ptr++;
	}
	checksum = ~checksum;

	ptr = (A_UINT16*)(eeprom+4);
	*ptr = checksum;
	return;
}

/* ATHENV V7.2 +++ */
static void
update_regCode(unsigned char* eeprom, int size, int regcode)
{
	int i;
	A_UINT16* ptr = (A_UINT16*)(eeprom+4);
	A_UINT16  checksum = 0;

	// Change Region Code
	AR_DEBUG_PRINTF("AR6K: Original Region Code= 0x%02x%02x\n", *(eeprom+9), *(eeprom+8));
	*(eeprom+8)= (unsigned char)(regcode&0xFF);
	*(eeprom+9)= (unsigned char)((regcode>>8)&0xFF);

	// checksum recalculation
	*ptr = 0;
	ptr = (A_UINT16*)eeprom;

	for (i=0; i<size; i+=2) {
		checksum ^= *ptr++;
	}
	checksum = ~checksum;

	ptr = (A_UINT16*)(eeprom+4);
	*ptr = checksum;
	return;
}
/* ATHENV V7.2 --- */

/* Read a Target register and return its value. */
inline void
BMI_read_reg(A_UINT32 address, A_UINT32 *pvalue)
{
    BMIReadSOCRegister(p_bmi_device, address, pvalue);
}

/* Write a value to a Target register. */
inline void
BMI_write_reg(A_UINT32 address, A_UINT32 value)
{
    BMIWriteSOCRegister(p_bmi_device, address, value);
}

/* Read Target memory word and return its value. */
inline void
BMI_read_mem(A_UINT32 address, A_UINT32 *pvalue)
{
    BMIReadMemory(p_bmi_device, address, (A_UCHAR*)(pvalue), 4);
}

/* Write a word to a Target memory. */
inline void
BMI_write_mem(A_UINT32 address, A_UINT8 *p_data, A_UINT32 sz)
{
    BMIWriteMemory(p_bmi_device, address, (A_UCHAR*)(p_data), sz); 
}

/*
 * Enable and configure the Target's Serial Interface
 * so we can access the EEPROM.
 */
static void
enable_SI(HIF_DEVICE *p_device)
{
    A_UINT32 regval;

    AR_DEBUG_PRINTF("%s\n", __FUNCTION__);

    p_bmi_device = p_device;

    BMI_read_reg(RTC_BASE_ADDRESS+SYSTEM_SLEEP_OFFSET, &sys_sleep_reg);
    BMI_write_reg(RTC_BASE_ADDRESS+SYSTEM_SLEEP_OFFSET, SYSTEM_SLEEP_DISABLE_SET(1)); //disable system sleep temporarily

    BMI_read_reg(RTC_BASE_ADDRESS+CLOCK_CONTROL_OFFSET, &regval);
    regval &= ~CLOCK_CONTROL_SI0_CLK_MASK;
    BMI_write_reg(RTC_BASE_ADDRESS+CLOCK_CONTROL_OFFSET, regval);

    BMI_read_reg(RTC_BASE_ADDRESS+RESET_CONTROL_OFFSET, &regval);
    regval &= ~RESET_CONTROL_SI0_RST_MASK;
    BMI_write_reg(RTC_BASE_ADDRESS+RESET_CONTROL_OFFSET, regval);


    BMI_read_reg(GPIO_BASE_ADDRESS+GPIO_PIN0_OFFSET, &regval);
    regval &= ~GPIO_PIN0_CONFIG_MASK;
    BMI_write_reg(GPIO_BASE_ADDRESS+GPIO_PIN0_OFFSET, regval);

    BMI_read_reg(GPIO_BASE_ADDRESS+GPIO_PIN1_OFFSET, &regval);
    regval &= ~GPIO_PIN1_CONFIG_MASK;
    BMI_write_reg(GPIO_BASE_ADDRESS+GPIO_PIN1_OFFSET, regval);

    /* SI_CONFIG = 0x500a6; */
    regval =    SI_CONFIG_BIDIR_OD_DATA_SET(1)  |
                SI_CONFIG_I2C_SET(1)            |
                SI_CONFIG_POS_SAMPLE_SET(1)     |
                SI_CONFIG_INACTIVE_CLK_SET(1)   |
                SI_CONFIG_INACTIVE_DATA_SET(1)   |
                SI_CONFIG_DIVIDER_SET(6);
    BMI_write_reg(SI_BASE_ADDRESS+SI_CONFIG_OFFSET, regval);
    
}

static void
disable_SI(void)
{
    A_UINT32 regval;
    
    AR_DEBUG_PRINTF("%s\n", __FUNCTION__);

    BMI_write_reg(RTC_BASE_ADDRESS+RESET_CONTROL_OFFSET, RESET_CONTROL_SI0_RST_MASK);
    BMI_read_reg(RTC_BASE_ADDRESS+CLOCK_CONTROL_OFFSET, &regval);
    regval |= CLOCK_CONTROL_SI0_CLK_MASK;
    BMI_write_reg(RTC_BASE_ADDRESS+CLOCK_CONTROL_OFFSET, regval);//Gate SI0 clock
    BMI_write_reg(RTC_BASE_ADDRESS+SYSTEM_SLEEP_OFFSET, sys_sleep_reg); //restore system sleep setting
}

/*
 * Tell the Target to start an 8-byte read from EEPROM,
 * putting the results in Target RX_DATA registers.
 */
static void
request_8byte_read(int offset)
{
    A_UINT32 regval;

//    printk("%s: request_8byte_read from offset 0x%x\n", __FUNCTION__, offset);

    
    /* SI_TX_DATA0 = read from offset */
        regval =(0xa1<<16)|
                ((offset & 0xff)<<8)    |
                (0xa0 | ((offset & 0xff00)>>7));
    
        BMI_write_reg(SI_BASE_ADDRESS+SI_TX_DATA0_OFFSET, regval);

        regval = SI_CS_START_SET(1)      |
                SI_CS_RX_CNT_SET(8)     |
                SI_CS_TX_CNT_SET(3);
        BMI_write_reg(SI_BASE_ADDRESS+SI_CS_OFFSET, regval);
}

/*
 * Tell the Target to start a 4-byte write to EEPROM,
 * writing values from Target TX_DATA registers.
 */
static void
request_4byte_write(int offset, A_UINT32 data)
{
    A_UINT32 regval;

    printk("%s: request_4byte_write (0x%x) to offset 0x%x\n", __FUNCTION__, data, offset);

        /* SI_TX_DATA0 = write data to offset */
        regval =    ((data & 0xffff) <<16)    |
                ((offset & 0xff)<<8)    |
                (0xa0 | ((offset & 0xff00)>>7));
        BMI_write_reg(SI_BASE_ADDRESS+SI_TX_DATA0_OFFSET, regval);

        regval =    data >> 16;
        BMI_write_reg(SI_BASE_ADDRESS+SI_TX_DATA1_OFFSET, regval);

        regval =    SI_CS_START_SET(1)      |
                SI_CS_RX_CNT_SET(0)     |
                SI_CS_TX_CNT_SET(6);
        BMI_write_reg(SI_BASE_ADDRESS+SI_CS_OFFSET, regval);
}

/*
 * Check whether or not an EEPROM request that was started
 * earlier has completed yet.
 */
static A_BOOL
request_in_progress(void)
{
    A_UINT32 regval;

    /* Wait for DONE_INT in SI_CS */
    BMI_read_reg(SI_BASE_ADDRESS+SI_CS_OFFSET, &regval);

//    printk("%s: request in progress SI_CS=0x%x\n", __FUNCTION__, regval);
    if (regval & SI_CS_DONE_ERR_MASK) {
        printk("%s: EEPROM signaled ERROR (0x%x)\n", __FUNCTION__, regval);
    }

    return (!(regval & SI_CS_DONE_INT_MASK));
}

/*
 * try to detect the type of EEPROM,16bit address or 8bit address
 */

static void eeprom_type_detect(void)
{
    A_UINT32 regval;
    A_UINT8 i = 0;

    request_8byte_read(0x100);
   /* Wait for DONE_INT in SI_CS */
    do{
        BMI_read_reg(SI_BASE_ADDRESS+SI_CS_OFFSET, &regval);
        if (regval & SI_CS_DONE_ERR_MASK) {
            printk("%s: ERROR : address type was wrongly set\n", __FUNCTION__);     
            break;
        }
        if (i++ == EEPROM_WAIT_LIMIT) {
            printk("%s: EEPROM not responding\n", __FUNCTION__);
        }
    } while(!(regval & SI_CS_DONE_INT_MASK));
}

/*
 * Extract the results of a completed EEPROM Read request
 * and return them to the caller.
 */
inline void
read_8byte_results(A_UINT32 *data)
{
    /* Read SI_RX_DATA0 and SI_RX_DATA1 */
    BMI_read_reg(SI_BASE_ADDRESS+SI_RX_DATA0_OFFSET, &data[0]);
    BMI_read_reg(SI_BASE_ADDRESS+SI_RX_DATA1_OFFSET, &data[1]);
}


/*
 * Wait for a previously started command to complete.
 * Timeout if the command is takes "too long".
 */
static void
wait_for_eeprom_completion(void)
{
    int i=0;

    while (request_in_progress()) {
        if (i++ == EEPROM_WAIT_LIMIT) {
            printk("%s: EEPROM not responding\n", __FUNCTION__);
        }
    }
}

/*
 * High-level function which starts an 8-byte read,
 * waits for it to complete, and returns the result.
 */
static void
fetch_8bytes(int offset, A_UINT32 *data)
{
    request_8byte_read(offset);
    wait_for_eeprom_completion();
    read_8byte_results(data);

    /* Clear any pending intr */
    BMI_write_reg(SI_BASE_ADDRESS+SI_CS_OFFSET, SI_CS_DONE_INT_MASK);
}

/*
 * High-level function which starts a 4-byte write,
 * and waits for it to complete.
 */
inline void
commit_4bytes(int offset, A_UINT32 data)
{
    request_4byte_write(offset, data);
    wait_for_eeprom_completion();
}

/* ATHENV */
#ifdef ANDROID_ENV
static int read_eeprom_from_file(char *fake_file)
{
	/*
	 * Transfer from file to Target RAM.
	 * Fetch source data from file.
	 */
	mm_segment_t		oldfs;
	struct file		*filp;
	struct inode		*inode = NULL;
	int			length;

	/* open file */
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	filp = filp_open(fake_file, O_RDONLY, S_IRUSR);

	if (IS_ERR(filp)) {
		printk("%s: file %s filp_open error\n", __FUNCTION__, fake_file);
		set_fs(oldfs);
		return -1;
	}

	if (!filp->f_op) {
		printk("%s: File Operation Method Error\n", __FUNCTION__);
		filp_close(filp, NULL);
		set_fs(oldfs);
		return -1;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20)
	inode = filp->f_path.dentry->d_inode;
#else
	inode = filp->f_dentry->d_inode;
#endif

	if (!inode) {
		printk("%s: Get inode from filp failed\n", __FUNCTION__);
		filp_close(filp, NULL);
		set_fs(oldfs);
		return -1;
	}

	AR_DEBUG_PRINTF("%s file offset opsition: %xh\n", __FUNCTION__, (unsigned)filp->f_pos);

	/* file's size */
	length = i_size_read(inode->i_mapping->host);
	AR_DEBUG_PRINTF("%s: length=%d\n", __FUNCTION__, length);
	if (length != EEPROM_SZ) {
		printk("%s: The file's size is not as expected\n", __FUNCTION__);
		filp_close(filp, NULL);
		set_fs(oldfs);
		return -1;
	}

	/* read data */
	if (filp->f_op->read(filp, eeprom_data, length, &filp->f_pos) != length) {
		printk("%s: file read error\n", __FUNCTION__);
		filp_close(filp, NULL);
		set_fs(oldfs);
		return -1;
	}

	/* read data out successfully */
	filp_close(filp, NULL);
	set_fs(oldfs);
	return 0;
}

static int read_mac_addr_from_file(char *p_mac)
{
	mm_segment_t		oldfs;
	struct file		*filp;
	struct inode		*inode = NULL;
	int			length;

	/* open file */
	oldfs = get_fs();
	set_fs(KERNEL_DS);
	filp = filp_open(p_mac, O_RDONLY, S_IRUSR);

	AR_DEBUG_PRINTF("%s try to open file %s\n", __FUNCTION__, p_mac);

	if (IS_ERR(filp)) {
		printk("%s: file %s filp_open error\n", __FUNCTION__, p_mac);
		set_fs(oldfs);
		return -1;
	}

	if (!filp->f_op) {
		printk("%s: File Operation Method Error\n", __FUNCTION__);
		filp_close(filp, NULL);
		set_fs(oldfs);
		return -1;
	}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,20)
	inode = filp->f_path.dentry->d_inode;
#else
	inode = filp->f_dentry->d_inode;
#endif
	if (!inode) {
		printk("%s: Get inode from filp failed\n", __FUNCTION__);
		filp_close(filp, NULL);
		set_fs(oldfs);
		return -1;
	}

	AR_DEBUG_PRINTF("%s file offset opsition: %xh\n", __FUNCTION__, (unsigned)filp->f_pos);

	/* file's size */
	length = i_size_read(inode->i_mapping->host);
	AR_DEBUG_PRINTF("%s: length=%d\n", __FUNCTION__, length);
	if (length > ATH_SOFT_MAC_TMP_BUF_LEN) {
		printk("%s: MAC file's size is not as expected\n", __FUNCTION__);
		filp_close(filp, NULL);
		set_fs(oldfs);
		return -1;
	}

	/* read data */
	if (filp->f_op->read(filp, soft_mac_tmp_buf, length, &filp->f_pos) != length) {
		printk("%s: file read error\n", __FUNCTION__);
		filp_close(filp, NULL);
		set_fs(oldfs);
		return -1;
	}

#if 0
	/* the data we just read */
	printk("%s: mac address from the file:\n", __FUNCTION__);
	for (i = 0; i < length; i++)
		printk("[%c(0x%x)],", soft_mac_tmp_buf[i], soft_mac_tmp_buf[i]);
	printk("\n");
#endif

	/* read data out successfully */
	filp_close(filp, NULL);
	set_fs(oldfs);

	/* convert mac address */
	if (!wmic_ether_aton(soft_mac_tmp_buf, mac_addr)) {
		printk("%s: convert mac value fail\n", __FUNCTION__);
		return -1;
	}

#if 0
	/* the converted mac address */
	printk("%s: the converted mac value\n", __FUNCTION__);
	for (i = 0; i < ATH_MAC_LEN; i++)
		printk("[0x%x],", mac_addr[i]);
	printk("\n");
#endif
	return 0;
}

void eeprom_ar6000_transfer(HIF_DEVICE *device, char *fake_file, char *p_mac, int regcode)
{
	A_UINT32 first_word;
	A_UINT32 board_data_addr;
	int i;
	int is_read_eeprom_from_file = 0;
//ZTE_WIFI_HP_007 
#ifdef SAVE_EEPROM_TO_FILE
	int is_eeprom_file_saved = 0;
	int ret;	
#endif
//ZTE_WIFI_HP_007 end 
	AR_DEBUG_PRINTF("%s: Enter\n", __FUNCTION__);

	enable_SI(device);

#ifdef SAVE_EEPROM_TO_FILE
    AR_DEBUG_PRINTF("%s: checking eeprom file %s\n", __FUNCTION__, SAVED_EEPROM_FILE);
    if ( ar6k_eepromfileExist(SAVED_EEPROM_FILE) == 1) {
		if (ar6k_isEepromFileValid(SAVED_EEPROM_FILE) == 1) 
			is_eeprom_file_saved = 1;
		else
			AR_DEBUG_PRINTF("%s: invalid eeprom file %s checksum\n", __FUNCTION__, SAVED_EEPROM_FILE);
	}
	if (is_eeprom_file_saved) {
		AR_DEBUG_PRINTF("%s: Valid eeprom file %s was saved already\n", __FUNCTION__, SAVED_EEPROM_FILE);
		fake_file = SAVED_EEPROM_FILE;	
	} else 
		 AR_DEBUG_PRINTF("%s: eeprom file not exist\n", __FUNCTION__);

#endif

	if (fake_file) {
		if ( read_eeprom_from_file(fake_file) == 0 ) // success
			is_read_eeprom_from_file = 1;	
	} 

	if (!is_read_eeprom_from_file) {

		/*
		 * Read from EEPROM to file OR transfer from EEPROM to Target RAM.
		 * Fetch EEPROM_SZ Bytes of Board Data, 8 bytes at a time.
		 */

		eeprom_type_detect();
		fetch_8bytes(0, (A_UINT32 *)(&eeprom_data[0]));

		/* Check the first word of EEPROM for validity */
		first_word = *((A_UINT32 *)eeprom_data);

		if ((first_word == 0) || (first_word == 0xffffffff)) {
			printk("Did not find EEPROM with valid Board Data.\n");
		}

		for (i = 8; i < EEPROM_SZ; i += 8) {
			fetch_8bytes(i, (A_UINT32 *)(&eeprom_data[i]));
		}
//ZTE_WIFI_HP_007 
#ifdef SAVE_EEPROM_TO_FILE
		if (!is_eeprom_file_saved) {
			AR_DEBUG_PRINTF("%s: saving hardware eeprom data to %s\n", __FUNCTION__, SAVED_EEPROM_FILE);
			ret = ar6k_saveEepromFile(SAVED_EEPROM_FILE, eeprom_data, EEPROM_SZ);
			if (!ret) {
				 AR_DEBUG_PRINTF("%s: saved hardware eeprom data to [%s] successfully\n", 
                         __FUNCTION__, SAVED_EEPROM_FILE);
			} else {
				 AR_DEBUG_PRINTF("%s: failed to save %s\n", __FUNCTION__, SAVED_EEPROM_FILE);
			}	
		}
#endif
//ZTE_WIFI_HP_007 
	}

	if (p_mac) {
		if (read_mac_addr_from_file(p_mac) != 0) {
			printk("%s: read_mac_addr_from_file failed\n", __FUNCTION__);
			p_mac = NULL;
		}
	}


	/* Determine where in Target RAM to write Board Data */
	BMI_read_mem( HOST_INTEREST_ITEM_ADDRESS(hi_board_data), &board_data_addr);
	if (board_data_addr == 0) {
		printk("hi_board_data is zero\n");
	}

	if (regcode != 0) {
		printk("update_regCode : 0x%x\n", regcode);  
		update_regCode(eeprom_data, EEPROM_SZ, regcode);
	}

	/* Update MAC address in RAM */
	if (p_mac) {
		update_mac(eeprom_data, EEPROM_SZ, mac_addr);
	}
#if 0
	/* mac address in eeprom array */
	printk("%s: mac values in eeprom array\n", __FUNCTION__);
	for (i = 10; i < 10 + 6; i++)
		printk("[0x%x],", eeprom_data[i]);
	printk("\n");
#endif

	/* Write EEPROM data to Target RAM */
	BMI_write_mem(board_data_addr, ((A_UINT8 *)eeprom_data), EEPROM_SZ);

	/* Record the fact that Board Data IS initialized */
	{
		A_UINT32 one = 1;
		BMI_write_mem(HOST_INTEREST_ITEM_ADDRESS(hi_board_data_initialized),
				(A_UINT8 *)&one, sizeof(A_UINT32));
	}

	disable_SI();
}
#endif /* ANDROID_ENV */
/* ATHENV */

