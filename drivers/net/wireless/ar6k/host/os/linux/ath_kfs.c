#include <linux/sched.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/version.h>

#include "ath_kfs.h"

#define EEPROM_SZ	0x300

ATH_OS_FD ath_OSFileOpen(char *pPath,  int flag, int mode)
{
	struct file	*filePtr;
		
	filePtr = filp_open(pPath, flag, mode);
	if (IS_ERR(filePtr)) {
		DBGPRINT(DEBUG_ERROR, ("%s(): Error %ld opening %s\n", __FUNCTION__, -PTR_ERR(filePtr), pPath));
	}

	return (ATH_OS_FD)filePtr;
}

int ath_OSFileClose(ATH_OS_FD osfd)
{
	filp_close(osfd, NULL);
	return 0;
}

void ath_OSFileSeek(ATH_OS_FD osfd, int offset)
{
	osfd->f_pos = offset;
}

int ath_OSFileRead(ATH_OS_FD osfd, char *pDataPtr, int readLen)
{
	if (osfd->f_op && osfd->f_op->read) {
		return osfd->f_op->read(osfd,  pDataPtr, readLen, &osfd->f_pos);
	} else {
		DBGPRINT(DEBUG_ERROR, ("no file read method\n"));
		return -1;
	}	
}

int ath_OSFileWrite(ATH_OS_FD osfd, char *pDataPtr, int writeLen)
{
	return osfd->f_op->write(osfd, pDataPtr, (size_t)writeLen, &osfd->f_pos);
}

void ath_OSFSInfoChange(ATH_OS_FS_INFO *pOSFSInfo, BOOLEAN bSet)
{
	if (bSet) {
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29)
		pOSFSInfo->fsuid= current->fsuid;
		pOSFSInfo->fsgid = current->fsgid;
		current->fsuid = current->fsgid = 0;
#else
		pOSFSInfo->fsuid = current_fsuid();
		pOSFSInfo->fsgid = current_fsgid();		
#endif
		pOSFSInfo->fs = get_fs();
		set_fs(KERNEL_DS);
	} else {
		set_fs(pOSFSInfo->fs);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,29)
		current->fsuid = pOSFSInfo->fsuid;
		current->fsgid = pOSFSInfo->fsgid;
#endif
	}
}

int ar6k_saveEepromFile(char *fname, char *buf, int len)
{
	int status = 0;	
	int retval;
	int				writeLength = 0;
	ATH_OS_FS_INFO	osFSInfo;
	ATH_OS_FD		srcf;
	
	if (!fname || !buf || len <= 0)
		return -1;

	ath_OSFSInfoChange(&osFSInfo, TRUE);
	
	do {
		srcf = ath_OSFileOpen(fname, O_WRONLY|O_CREAT|O_TRUNC, 0777);
		if (IS_FILE_OPEN_ERR(srcf)) {
			DBGPRINT(DEBUG_ERROR, ("%s - Error opening file %s\n", __FUNCTION__, fname));
			status = -1;
			break;
		}

		writeLength = ath_OSFileWrite(srcf, buf, len);

		if (writeLength != len) {			
			DBGPRINT(DEBUG_ERROR, ("%s - Error writing file %s\n", __FUNCTION__, fname));
			status = -1;
		}

		break;
		
	} while(TRUE);
	
	if (IS_FILE_OPEN_ERR(srcf))
		;
	else {
		retval = ath_OSFileClose(srcf);
		if (retval)	{
			DBGPRINT(DEBUG_ERROR, ("--> Error %d closing %s\n", -retval, fname));
			status = retval;
		}
	}
	
	ath_OSFSInfoChange(&osFSInfo, FALSE);
	
	return status;
		
}

int ar6k_eepromfileExist(char *fname)
{
	ATH_OS_FS_INFO	osFSInfo;	
	ATH_OS_FD		srcf;
	int status = 0;
	
	if (!fname)
		return -1;
	
	ath_OSFSInfoChange(&osFSInfo, TRUE);

	srcf = ath_OSFileOpen(fname, O_RDONLY, 0);
	
	if (IS_FILE_OPEN_ERR(srcf)) {
		DBGPRINT(DEBUG_ERROR, ("%s - Error opening file %s\n", __FUNCTION__, fname));
		status = 0; /*file not exist*/
	} else {
		status = 1; /*file exist*/
	}

	if (IS_FILE_OPEN_ERR(srcf))
	;
	else {
		ath_OSFileClose(srcf);
	}
	
	ath_OSFSInfoChange(&osFSInfo, FALSE);
	
	return status;		
	
}

static void ath_CalEepromChecksum(unsigned char *buf, unsigned short *checksum)
{
    int i, len;
    unsigned short ckm;
    unsigned short *s = (unsigned short *)buf;

    ckm = 0;
    len = EEPROM_SZ;

    for (i = 0; i < len/2; i = i + 2) {
            ckm ^= s[i]^s[i+1];
    }

	*checksum = ckm;
}

int ar6k_isEepromFileValid(char *fname)
{
	ATH_OS_FS_INFO	osFSInfo;	
	ATH_OS_FD		srcf;

	unsigned short checksum;
	unsigned char buf[EEPROM_SZ] = {0};
	int status = 1;	/*valid*/
	int FileLength;

	if (!fname)
		return -1;
	
	ath_OSFSInfoChange(&osFSInfo, TRUE);
	srcf = ath_OSFileOpen(fname, O_RDONLY, 0);

	do {
		
		srcf = ath_OSFileOpen(fname, O_RDONLY, 0);
	
		if (IS_FILE_OPEN_ERR(srcf)) 
		{
			DBGPRINT(DEBUG_ERROR, ("%s - Error opening file %s\n", __FUNCTION__, fname));
			status = 0; /*invalid*/
			break;
		}
		
		FileLength = ath_OSFileRead(srcf, buf, EEPROM_SZ);
		
		if (FileLength != EEPROM_SZ) {
			status = 0; /*invalid*/
			break;
		} else {
			ath_CalEepromChecksum(buf, &checksum);
			if (checksum != 0xffff)
				status = 0; /*invalid*/
		}	
		break;		
	} while(0);
	
	if (IS_FILE_OPEN_ERR(srcf))
		;
	else {
		ath_OSFileClose(srcf);
	}
	
	ath_OSFSInfoChange(&osFSInfo, FALSE);
	
	return status;
		
}

