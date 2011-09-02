/**
-----------------------------------------------------------------------------
Copyright (c) 2010~ ZTE Corporation, Incorporated. 
All Rights Reserved. ZTE Proprietary and Confidential.
-----------------------------------------------------------------------------
 *
 * @Filename: modemctl.h
 * Description: the head file for the modemctl driver, some structures and include files etc.
 * Others:     
 * Version:    1.0
 * Author:     wangxiaowei
 */


/*===========================================================================

                        EDIT HISTORY FOR MODULE


  when            who  what, where, why
  ----------   ---   -----------------------------------------------------------
  2010-06-12  hyj  modify the debug code.flag: ZTE_MODMEMCTL_HYJ_20100618
  2010-03-08  wxw  create
===========================================================================*/

#ifndef	MODEMCTL_H
#define	MODEMCTL_H

/*===========================================================================
						 INCLUDE FILES FOR MODULE
===========================================================================*/

#include <linux/module.h>	/* kernel module definitions */
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/notifier.h>
#include <linux/proc_fs.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>

#include <mach/gpio.h>                  //for the gpio config
#include <linux/delay.h>

#include <linux/irq.h>
#include <linux/param.h>
#include <linux/bitops.h>
#include <linux/termios.h>
#include <linux/wakelock.h>
#include <mach/gpio.h>
#include <linux/serial_core.h>           //for the uart_port
#include <mach/msm_serial_hs.h>

#include <asm/types.h>



/*=========================================================================*/
/*                               CONSTANTS                                 */
/*=========================================================================*/
//the cmd type for the modemctl driver module
enum modemctl_cmd_type {
	MODEMCTL_CMD_OPEN_MODEM = 1,
	MODEMCTL_CMD_CLOSE_MODEM,
	MODEMCTL_CMD_RESET_MODEM,
	MODEMCTL_CMD_WAKE_MODEM,
	MODEMCTL_CMD_SENDDATE2MODEM,
	MODEMCTL_CMD_AT_DISCONNECT,
	MODEMCTL_CMD_AT_CONNECT,
	MODEMCTL_CMD_AUDIO2AP,
	MODEMCTL_CMD_AUDIO2MODEM,
	MODEMCTL_CMD_READYTEST_SET_C2GIRQ_LOW,
	MODEMCTL_CMD_READYTEST_SET_C2GIRQ_HIGH,
	MODEMCTL_CMD_TYPE_COUNT,
};

//the different types to select
enum modemctl_select_type {
	MODEMCTL_IFX_GSM_MODEM = 1,
	MODEMCTL_ALL_MODEM,
};

//for ap to notify modem
enum modemctl_notify_cmd_type {
      MODEMCTL_NOTIFY_SENDDATA_READY = 1,
      MODEMCTL_NOTIFY_APSUSPEND,
      MODEMCTL_NOTIFY_APRESUME,
      MODEMCTL_NOTIFY_ALL_CMD,
};

//for control the audio SW
enum modemctl_audiosw_cmd_type {
      MODEMCTL_AUDIOSW_AUDIO2AP = 0,
      MODEMCTL_AUDIOSW_AUDIO2MODEM,
      MODEMCTL_AUDIOSW_ALL_CMD,
};

//for control the AT SW
enum modemctl_atsw_cmd_type {
      MODEMCTL_ATSW_DISCONNECT = 0,
      MODEMCTL_ATSW_CONNECT,
      MODEMCTL_ATSW_ALL_CMD,
};

//WANGXIAOWEI_MODEM_STATE_20100604_01, begin
//for some gpio state 
enum modemctl_gpio_state_type {
      MODEMCTL_GPIO_AUDIO_SW = 1,
      MODEMCTL_GPIO_AT_SW,
      MODEMCTL_GPIO_BOARD_STATE,
      MODEMCTL_GPIO_ALL_TYPE,
};
//WANGXIAOWEI_MODEM_STATE_20100604_01, end

/*=========================================================================*/
/*                                MACROS                                   */
/*=========================================================================*/
//the gpio resource
#define MSM_GPIO_UARTDM2_RX	    21
#define MSM_GPIO_UARTDM2_TX	    108
#define MSM_GPIO_UARTDM2_RTS	    19
#define MSM_GPIO_UARTDM2_CTS	    20

#define MSM_GPIO_GSM_KEY_ON	    72
#define MSM_GPIO_GSM_RESET	    73
#define MSM_GPIO_G2C_IRQ	          42
#define MSM_GPIO_C2G_READY	    75
#define MSM_GPIO_C2G_IRQ	          74
#define MSM_GPIO_G2C_READY	    78
#define MSM_GPIO_GSM_WORKING	    16
#define MSM_GPIO_GSM_AUDIO_SW	    79

#define MSM_GPIO_GSM_AT_SW	    80
//WANGXIAOWEI_MODEM_STATE_20100604_01, begin
#define MSM_GPIO_BOARD_TYPE	    110
//WANGXIAOWEI_MODEM_STATE_20100604_01, end

#define GPIO_HIGH_LEVEL	    1
#define GPIO_LOW_LEVEL	    0

#define GET_RESPSIGNAL	    1
#define SET_RESPSIGNAL	    2

#define MAX_RESPSIGNAL_TIMERNUM		20

//ZTE_MODMEMCTL_HYJ_20100618, begin, modify the debug code
#if 0
//for the debug
//WANGXIAOWEI_EP_20100524_01, begin
//#define MODEMCTL_INFO(fmt, arg...) printk(KERN_INFO "Modemctl: %s: " fmt "\n", __func__  , ## arg)
#define MODEMCTL_INFO(fmt, arg...) 
//WANGXIAOWEI_EP_20100524_01, end

#define MODEMCTL_ERR(fmt, arg...)  printk(KERN_ERR "Modemctl: %s: " fmt "\n" , __func__ , ## arg)
#define MODEMCTL_DBG(fmt, arg...)  pr_debug("Modemctl: %s: " fmt "\n" , __func__ , ## arg)
#endif 
//ZTE_MODMEMCTL_HYJ_20100618, end, modify the debug code

/* delay, which can schedule*/
#if 0
#define delay(n) \
  do { \
    unsigned long expiry = jiffies + n; \
	  do { \
		  set_current_state(TASK_INTERRUPTIBLE); \
		  schedule_timeout(n); \
	  } while( time_before(jiffies, expiry)); \
  } while(0)
#endif
#define delay(n) mdelay(n)

/*=========================================================================*/
/*                               TYPEDEFS & STRUCTS                                 */
/*=========================================================================*/

/** the modemctl device gpio resource info  */
struct modemctl_device_gpio_info {
	unsigned gsm_key_on;		
	unsigned gsm_reset;
    	unsigned g2c_irq_io;       //also used as gpio to up or down
	unsigned g2c_irq;           //to wake gsm modem
	unsigned c2g_ready;
	unsigned c2g_irq;
    	unsigned g2c_ready_io;
	unsigned g2c_ready;
	unsigned gsm_working;	
};

/** the modemctl device operations for ifx_gsm, ... different modems */
struct modemctl_ops {
	int (*init) (void);
      //WANGXIAOWEI_MODEM_STATE_20100604_01, begin
      	//int (*get_sleepstate)(void);
	int (*get_gpiostate)(unsigned int type);
      //WANGXIAOWEI_MODEM_STATE_20100604_01, begin
	int (*get_workingstate)(void);
	int (*open_modem)(void);	
	int (*reset_modem)(void);	
	int (*close_modem)(void);	
	int (*wake_modem)(void);
    	int (*switch_atcom)(unsigned int on);
    	int (*switch_audiocom)(unsigned int on);
	int (*notify_modem)(unsigned int type);	
	int (*get_respsignal)(void);
};

/** the sysfs interface of modemctl device  */
struct modemctl_sysfs_info {
	unsigned int select;		
	unsigned int cmd;
    	unsigned int modem_state;       //the modem state
};

/** the modemctl device info struct */
struct modemctl_device_info {
        struct modemctl_device_gpio_info *gpio_info;	//modemctl用的GPIO资源
        //struct uart_port *uport;			// modemctl用到的串口资源
        struct modemctl_sysfs_info *sysfs_info;
        struct wake_lock wake_lock;
        struct modemctl_ops *ops;		//Modem操作API的封装接口
        unsigned int modem_state;		//Modem 状态
        unsigned int modem_type;		//所使用的Modem 类型
        wait_queue_head_t event_wait;
};

#endif  /*MODEMCTL_H*/

