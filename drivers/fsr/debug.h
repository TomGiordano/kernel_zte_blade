/*
 *---------------------------------------------------------------------------*
 *                                                                           *
 *               COPYRIGHT. SAMSUNG ELECTRONICS CO., LTD.                    *
 *                          ALL RIGHTS RESERVED                              *
 *                                                                           *
 *   Permission is hereby granted to licensees of Samsung Electronics Co.,   *
 *   Ltd. products to use this computer program only in accordance with the  *
 *   terms of the SAMSUNG FLASH MEMORY DRIVER SOFTWARE LICENSE AGREEMENT.    *
 *                                                                           *
 *---------------------------------------------------------------------------*
*/
/**
 * @version	LinuStoreIII_1.2.0_b035-FSR_1.2.1p1_b129_RC
 * @file	drivers/fsr/debug.h
 * @brief	Debug macro, time measure macro
 *
 */

#ifndef _FSR_DEBUG_H_
#define _FSR_DEBUG_H_

#define MAX_NUMBER_SECTORS	16
#define IO_DIRECTION		2
#define STL_IOSTAT_PROC_NAME	"stl-iostat"
#define BML_IOSTAT_PROC_NAME	"bml-iostat"
#define FSR_DBGMSG_PROC_NAME	"dbgmsg"

#ifdef FSR_TIMER
#define DECLARE_TIMER	struct timeval start, stop
#define START_TIMER()	do { do_gettimeofday(&start); } while (0)
#define STOP_TIMER(name)					\
do {								\
	do_gettimeofday(&stop);					\
	if (stop.tv_usec < start.tv_usec) {			\
		stop.tv_sec -= (start.tv_sec + 1);		\
		stop.tv_usec += (1000000U - start.tv_usec);	\
	} else {						\
		stop.tv_sec -= start.tv_sec;			\
		stop.tv_usec -= start.tv_usec;			\
	}							\
	printk("STOP: %s: %d.%06d",				\
	 name, (int) (stop.tv_sec), (int) (stop.tv_usec));	\
} while (0)

#else
#define DECLARE_TIMER		do { } while (0)
#define START_TIMER()		do { } while (0)
#define STOP_TIMER(name)	do { } while (0)
#endif

/* These symbols will be use the user-level*/
/*
 * for debugging
 */
#define DL0     (0)     /* Quiet   */
#define DL1     (1)     /* Audible */
#define DL2     (2)     /* Loud    */
#define DL3     (3)     /* Noisy   */

#define _LINUSTOREIII_DEBUG_
#ifdef _LINUSTOREIII_DEBUG_
#define DEBUG(n, format, args...)					\
do {									\
	if (n <= CONFIG_LINUSTOREIII_DEBUG_VERBOSE) {			\
		printk("%s[%d]: " format "\n",				\
			 __func__, __LINE__, ##args);			\
	}								\
} while(0)			
#else
#define DEBUG(...)		do { } while (0)
#endif /*_LINUSTOREIII_DEBUG_ */

#define ERRPRINTK(format, args...)					\
do {									\
	printk("LinuStoreIII_NERR: %s[%d] " format "\n", __func__, __LINE__, ##args);	\
} while (0)


/* This macro for print request descriptor */
//#define RFS_FSR_PRINT_REQ
#ifdef RFS_FSR_PRINT_REQ
#define print_request(req)						\
do {									\
	printk("cmd: %d, sector[%d], nr_sectors[%d], nr_segment[%d] :0x%08x\n",\
		req->cmd, req->sector, req->nr_sectors,			\
		req->nr_segments, req->buffer);				\
} while (0)
#else
#define print_request(...)	do { } while (0)
#endif

#define FSR_DOWN(x)			down(x)
#define FSR_UP(x)			up(x)

typedef void (*fsr_reset_t)(void);

struct fsr_platform {
	fsr_reset_t	reset;
};

#ifdef CONFIG_PROC_FS
extern struct proc_dir_entry *fsr_proc_dir;
#endif

#endif	/* _FSR_DEBUG_H_ */
