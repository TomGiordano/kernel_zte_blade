/**
-----------------------------------------------------------------------------
Copyright (c) 2010~ ZTE Corporation, Incorporated. 
All Rights Reserved. ZTE Proprietary and Confidential.
-----------------------------------------------------------------------------
 *
 * @Filename: modemctl.c
 * Description: the realization file for the modemctl driver, some structures and include files etc.
 * Others:   
 * Version:    1.0
 * Author:     wangxiaowei
 */

/*===========================================================================

                        EDIT HISTORY FOR MODULE


  when            who  what, where, why
  ----------   ---   -----------------------------------------------------------
  2010-06-24  hyj     check idle state of tx and rx before close uart clock		ZTE_MODMEMCTL_HYJ_20100624
  2010-06-18  hyj     modify the debug code.							ZTE_MODMEMCTL_HYJ_20100618
  2010-06-10  wxw  debug cann't sleep when forbid GSM modem                   WANGXIAOWEI_WAKELOCK_20100610_01
  2010-06-04  wxw  add the interface for geting modem state                         WANGXIAOWEI_MODEM_STATE_20100604_01
  2010-05-25  wxw  debug the uart error data transmission                            WANGXIAOWEI_DEBUG_UART_20100525_01
  2010-05-24  wxw  add exceptional protection and clean some log                  WANGXIAOWEI_EP_20100524_01
  2010-05-20  wxw  change the wakelock timer to 5s to receive AT - RING        WANGXIAOWEI_PM_20100520_02
  2010-05-20  wxw  to debug the modem cann't sleep when AP suspend            WANGXIAOWEI_PM_20100520_01
  2010-05-19  wxw  add the log of the GPIO states to debug                           WANGXIAOWEI_GPIOSTATE_20100519_02
  2010-05-19  wxw  add the close & open UART clock to debug                        WANGXIAOWEI_UARTCLOCK_20100519_01

  2010-05-04  hyj    close the modem before system power off  or restart           HUANGYANJUN_PM_20100504,
  2010-04-27  wxw  add the suspend & resume function                                  WANGXIAOWEI_PM_0427,
  2010-04-16  wxw  using delay 25ms version when send AT to modem               WANGXIAOWEI_25MS_20100416
  2010-04-09  wxw  validate ready pin feature by timer                                    WANGXIAOWEI_READY_20100409
  2010-03-28  wxw  change the reset function, 
                            because of there is a not gate on the line of gsm reset         WANGXIAOWEI_RESET_20100331
  2010-03-28  wxw  add the g2c_ready gsm response interface                         WANGXIAOWEI_G2C_READY_RESP_20100330
  2010-03-28  wxw  add the send data notify interface
  2010-03-08  wxw  create
===========================================================================*/

/*===========================================================================
						 INCLUDE FILES FOR MODULE
===========================================================================*/

#include <linux/reboot.h>

#include "modemctl.h"	/* modemctl module definitions */

/*=========================================================================*/
/*                                MACROS                                   */
/*=========================================================================*/
#define VERSION		"1.0"

/* state variable names and bit positions */
#define MODEMCTL_MODEM_WAKE_RESP	0x01
#define MODEMCTL_SEND_DATA	0x02
#define MODEMCTL_ALLOW_SLEEP	0x04

/* 1 second timeout */
#define TX_TIMER_INTERVAL	1
#define MODEMCTL_MODEM_RESP_POLL_TIME (2*HZ)
#define MODEMCTL_WAIT_MODEMRESP_TIMEOUT -1

//WANGXIAOWEI_PM_20100520_02, begin
#define UART_CLOCK_ON	    1
#define UART_CLOCK_OFF	    0
//#define MODEMCTL_AFTERIRQ_WAKELOCK_TIME (HZ/2)
#define MODEMCTL_AFTERIRQ_WAKELOCK_TIME (HZ*5)
//WANGXIAOWEI_PM_20100520_02, end

/*=========================================================================*/
/*                            Debug Definitions                          */
/*=========================================================================*/
//ZTE_MODMEMCTL_HYJ_20100618, begin, add the debug code
enum {
	MODEMCTL_DEBUG_INFO = 1U << 0,
	MODEMCTL_DEBUG_WARNING = 1U << 2,
	MODEMCTL_DEBUG_ERR = 1U << 1,
};

static int modemctl_debug_mask = MODEMCTL_DEBUG_ERR;
module_param_named(
	debug_mask, modemctl_debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP
);

#define MODEMCTL_INFO(fmt, arg...) \
	do { \
		if ((MODEMCTL_DEBUG_INFO) & modemctl_debug_mask) \
			printk(KERN_INFO "Modemctl: %s: " fmt "\n", __func__  , ## arg); \
	} while (0)
	
#define MODEMCTL_WAR(fmt, arg...) \
	do { \
		if ((MODEMCTL_DEBUG_WARNING) & modemctl_debug_mask) \
			printk(KERN_WARNING "Modemctl: %s: " fmt "\n", __func__  , ## arg); \
	} while (0)
	
	#define MODEMCTL_ERR(fmt, arg...) \
	do { \
		if ((MODEMCTL_DEBUG_ERR) & modemctl_debug_mask) \
			printk(KERN_ERR "Modemctl: %s: " fmt "\n", __func__  , ## arg); \
	} while (0)
//ZTE_MODMEMCTL_HYJ_20100618, end, add the debug code
/*=========================================================================*/
/*                               CONSTANTS                                 */
/*=========================================================================*/

/*
 * Global variables
 */
/** Global state flags */
static unsigned long flags;

/** Tasklet to respond to change in hostwake line */
//static struct tasklet_struct hostwake_task;

/** Transmission timer */
static struct timer_list ready_timer;

/** Resp single timer */
//static struct timer_list respsignal_timer;

/** Lock for state transitions */
static spinlock_t rw_lock;


/* global pointer to a single modemctl device. */
static struct modemctl_device_info *p_mdi;

/*=========================================================================*/
/*                           LOCAL FUNCITONS DECLARATIONS                         */
/*=========================================================================*/
static ssize_t modemctl_show_cmd(struct device_driver *driver, char *buf);
static ssize_t modemctl_show_select(struct device_driver *driver, char *buf);
static ssize_t modemctl_store_select(struct device_driver *driver,
	const char *buf, size_t count);
static ssize_t modemctl_store_cmd(struct device_driver *driver,
	const char *buf, size_t count);
static ssize_t modemctl_show_modem_state(struct device_driver *driver, char *buf);

//WANGXIAOWEI_MODEM_STATE_20100604_01, begin
static ssize_t modemctl_show_audio_sw_state(struct device_driver *driver, char *buf);
static ssize_t modemctl_show_at_sw_state(struct device_driver *driver, char *buf);
static ssize_t modemctl_show_board_state(struct device_driver *driver, char *buf);
//WANGXIAOWEI_MODEM_STATE_20100604_01, end

static int __init modemctl_probe(struct platform_device *pdev);
static void modemctl_hostwake_task(unsigned long data);

//WANGXIAOWEI_PM_0427, begin, add just for suspend resume
static int modemctl_suspend(struct platform_device *pdev, pm_message_t state);
static int modemctl_resume(struct platform_device *pdev);
//WANGXIAOWEI_PM_0427, end, add just for suspend resume

static int modemctl_pre_write2uart(void);

//WANGXIAOWEI_UARTCLOCK_20100519_01, begin
static void hsuart_power(int on);
//WANGXIAOWEI_UARTCLOCK_20100519_01, end

//WANGXIAOWEI_GPIOSTATE_20100519_02, begin
static void modemctl_showgpiostate(void);
//WANGXIAOWEI_GPIOSTATE_20100519_02, end

/*=========================================================================*/
/*                               TYPEDEFS & STRUCTS                                 */
/*=========================================================================*/
/*
 * tasklet declare
 */
DECLARE_TASKLET(hostwake_task, modemctl_hostwake_task, 0);

/*
 * Sysfs properties for the app control
 */
static DRIVER_ATTR(select, S_IRWXUGO, modemctl_show_select, modemctl_store_select);	//can read write execute
static DRIVER_ATTR(cmd, S_IRWXUGO, modemctl_show_cmd, modemctl_store_cmd); 	//can read write execute
static DRIVER_ATTR(modem_state, S_IRUGO, modemctl_show_modem_state, NULL);	 //can read only
//WANGXIAOWEI_MODEM_STATE_20100604_01, begin
static DRIVER_ATTR(audio_sw_state, S_IRUGO, modemctl_show_audio_sw_state, NULL);	 //can read only
static DRIVER_ATTR(at_sw_state, S_IRUGO, modemctl_show_at_sw_state, NULL);	 //can read only
static DRIVER_ATTR(board_state, S_IRUGO, modemctl_show_board_state, NULL);	 //can read only
//WANGXIAOWEI_MODEM_STATE_20100604_01, end

/*
 * modemctl_driver declare
 */
static struct platform_driver modemctl_driver = {
	.probe = modemctl_probe,
	//.remove = modemctl_remove,
       //WANGXIAOWEI_PM_0427, begin, add just for suspend resume
	.suspend = modemctl_suspend,
	.resume = modemctl_resume,
       //WANGXIAOWEI_PM_0427, end, add just for suspend resume
	.driver = {
		.name = "msm_modemctl",
		.owner = THIS_MODULE,
	},
};

/*=========================================================================*/
/*                           EXTERN DATA DECLARATIONS                             */
/*=========================================================================*/
extern struct modemctl_ops modem_ifxgsm_ops;


/*=========================================================================*/
/*                           LOCAL FUNCITONS REALIZATION                             */
/*=========================================================================*/

/**********************************************************************
* Function:       modemclt_init_sysfs_info(void)
* Description:    init the sysfs interface variables of modemctl device
* Input:            void
* Output:         
* Return:          
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/03/11     V1.0      wxw       create
**********************************************************************/
static void modemclt_init_sysfs_info(void)
{
    p_mdi->sysfs_info->select = 0;
    p_mdi->sysfs_info->cmd = 0;
    p_mdi->sysfs_info->modem_state= 0;
}

/**********************************************************************
* Function:        modemclt_create_driver_sysfs(struct platform_driver *drv)
* Description:    to create the sysfs interface for modemctl driver
* Input:            struct platform_driver *drv
* Output:         
* Return:           0 sucess, !0 error
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/03/11     V1.0      wxw       create
**********************************************************************/
static int modemclt_create_driver_sysfs(struct platform_driver *drv)
{
    int error = 0;
    
    MODEMCTL_INFO("++");
    error  = driver_create_file(&drv->driver, &driver_attr_select);
    error |= driver_create_file(&drv->driver, &driver_attr_cmd);
    error |= driver_create_file(&drv->driver, &driver_attr_modem_state);
    
//WANGXIAOWEI_MODEM_STATE_20100604_01, begin
    error |= driver_create_file(&drv->driver, &driver_attr_audio_sw_state);
    error |= driver_create_file(&drv->driver, &driver_attr_at_sw_state);
    error |= driver_create_file(&drv->driver, &driver_attr_board_state);
//WANGXIAOWEI_MODEM_STATE_20100604_01, end
    
    MODEMCTL_INFO("--");
    return error;
}

/**********************************************************************
* Function:        modemclt_remove_driver_sysfs(struct platform_driver *drv)
* Description:    to remove the sysfs interface for modemctl driver
* Input:            struct platform_driver *drv
* Output:         
* Return:           none
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/03/11     V1.0      wxw       create
**********************************************************************/
static void modemclt_remove_driver_sysfs(struct platform_driver *drv)
{
    MODEMCTL_INFO("++");
    driver_remove_file(&drv->driver, &driver_attr_select);
    driver_remove_file(&drv->driver, &driver_attr_cmd);
    driver_remove_file(&drv->driver, &driver_attr_modem_state);

//WANGXIAOWEI_MODEM_STATE_20100604_01, begin
    driver_remove_file(&drv->driver, &driver_attr_audio_sw_state);
    driver_remove_file(&drv->driver, &driver_attr_at_sw_state);
    driver_remove_file(&drv->driver, &driver_attr_board_state);
//WANGXIAOWEI_MODEM_STATE_20100604_01, end
    
    MODEMCTL_INFO("--");
}


/**********************************************************************
* Function:       the following function to operate the select property of sysfs interface
* Description:    to read or write the select property ---- driver_attr_select
* Input:            struct platform_driver *drv
* Output:         
* Return:           none
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/03/11     V1.0      wxw       create
**********************************************************************/
static ssize_t modemctl_show_select(struct device_driver *driver, char *buf)
{
    MODEMCTL_INFO("enter the modemctl_show_select. ");

    return snprintf(buf, PAGE_SIZE, "%u\n", p_mdi->sysfs_info->select);
}


static ssize_t modemctl_store_select(struct device_driver *driver,
	const char *buf, size_t count)
{
    char *p = (char *)buf;

    p_mdi->sysfs_info->select = simple_strtol(p, NULL, 10);
    
    MODEMCTL_INFO("++ the cmd char buff = %s, cmd code = %u\n",buf, p_mdi->sysfs_info->select);

    switch(p_mdi->sysfs_info->select)
    {
        case MODEMCTL_IFX_GSM_MODEM:
            p_mdi->ops = &modem_ifxgsm_ops;
            break;
        default:
            p_mdi->ops = &modem_ifxgsm_ops;
            break;
    }
     return strnlen(buf, count);
}


/**********************************************************************
* Function:       the following function to operate the cmd property of sysfs interface
* Description:    to read or write the select property ---- driver_attr_cmd
* Input:            struct platform_driver *drv
* Output:         char* buf
* Return:           none
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/03/11     V1.0      wxw       create
**********************************************************************/
static ssize_t modemctl_show_cmd(struct device_driver *driver, char *buf)
{
    MODEMCTL_INFO("++ ");

    return snprintf(buf, PAGE_SIZE, "%u\n", p_mdi->sysfs_info->cmd);
}

static ssize_t modemctl_store_cmd(struct device_driver *driver,
	const char *buf, size_t count)
{
    int ret = 0;
    
    char *p = (char *)buf;
    p_mdi->sysfs_info->cmd = (unsigned int)simple_strtol(p, NULL, 10);
    
    MODEMCTL_INFO("++ the count = %d, the cmd char buff = %s, cmd code = %u\n",count,buf, p_mdi->sysfs_info->cmd);

    switch(p_mdi->sysfs_info->cmd)
    {
        case MODEMCTL_CMD_OPEN_MODEM:        	       //open modem
            ret = p_mdi->ops->open_modem();
            break;
            
        case MODEMCTL_CMD_CLOSE_MODEM:			//close modem
            //WANGXIAOWEI_EP_20100524_01, begin
            del_timer(&ready_timer);
            clear_bit(MODEMCTL_SEND_DATA, &flags);
			//WANGXIAOWEI_WAKELOCK_20100610_01,begin
		 	wake_unlock(&p_mdi->wake_lock);
			//WANGXIAOWEI_WAKELOCK_20100610_01,end
            //WANGXIAOWEI_EP_20100524_01, end
            ret = p_mdi->ops->close_modem();
            break;
            
        case MODEMCTL_CMD_RESET_MODEM:			//reset modem
            //WANGXIAOWEI_EP_20100524_01, begin
            del_timer(&ready_timer);
            clear_bit(MODEMCTL_SEND_DATA, &flags);
            //WANGXIAOWEI_EP_20100524_01, end
            ret = p_mdi->ops->reset_modem();
            break;
            
        case MODEMCTL_CMD_WAKE_MODEM:			//wake modem
            ret = p_mdi->ops->wake_modem();
            break;
            
        case MODEMCTL_CMD_SENDDATE2MODEM:		//send data to modem by uart dm2
            ret = modemctl_pre_write2uart();
            break;	
            
        case MODEMCTL_CMD_AT_DISCONNECT:			//switch atcom to LOW, make ap uart disconnect directly to ifx modem uart, 
            ret = p_mdi->ops->switch_atcom( GPIO_LOW_LEVEL);
            break;
            
        case MODEMCTL_CMD_AT_CONNECT:			//switch atcom to HIGH, make ap uart connect directly to AP uart, 
            ret = p_mdi->ops->switch_atcom( GPIO_HIGH_LEVEL );
            break;
           
        case MODEMCTL_CMD_AUDIO2AP:                       //switch audiocom to LOW, make audio channel to AP
            ret = p_mdi->ops->switch_audiocom( GPIO_LOW_LEVEL);
            break;
        case MODEMCTL_CMD_AUDIO2MODEM:                 //switch audiocom to HIGH, make audio channel to IFX modem
            ret = p_mdi->ops->switch_audiocom( GPIO_HIGH_LEVEL );
            break;
            
//WANGXIAOWEI_25MS_20100416, begin

        //the follow just for test the modem sleep 10/11
        case MODEMCTL_CMD_READYTEST_SET_C2GIRQ_LOW:    
            gpio_set_value(MSM_GPIO_C2G_IRQ, GPIO_LOW_LEVEL);
            ret = gpio_get_value(MSM_GPIO_G2C_READY);
            MODEMCTL_INFO("@@ after pull down MSM_GPIO_C2G_IRQ , the state MSM_GPIO_G2C_READY =%d  \n",ret);
            break;
            
        case MODEMCTL_CMD_READYTEST_SET_C2GIRQ_HIGH:                  
            ret = p_mdi->ops->wake_modem();
            MODEMCTL_INFO("@@ after pull up MSM_GPIO_C2G_IRQ to wake up the modem, the state MSM_GPIO_G2C_READY =%d  \n",ret);
            break;

//WANGXIAOWEI_25MS_20100416, end    
	 default:
            MODEMCTL_ERR(" don't support the cmd --- %d, please check your input cmd.\n", p_mdi->sysfs_info->cmd);
            break;

    }

    //here if some error occurs, return the ret, else return the strnlen(buf, count)
    if( ret != 0 )
    {
         MODEMCTL_ERR(" the cmd --- %d, execute error \n", p_mdi->sysfs_info->cmd);
    }


    //WANGXIAOWEI_GPIOSTATE_20100519_02, begin
    //modemctl_showgpiostate();
    //WANGXIAOWEI_GPIOSTATE_20100519_02, end

    MODEMCTL_INFO("-- the return = %d\n",strnlen(buf, count));
	
    //if return is not count 
    return strnlen(buf, count);
    
}

/**********************************************************************
* Function:       the following function to operate the modem_state property of sysfs interface
* Description:    to read or write the select property ---- driver_attr_modem_state
* Input:            struct platform_driver *drv
* Output:         
* Return:           none
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/03/11     V1.0      wxw       create
**********************************************************************/
static ssize_t modemctl_show_modem_state(struct device_driver *driver, char *buf)
{
    unsigned int workingstate = 0;

    //the sleepstate don't support in the IFX modem.
    //sleepstate = p_mdi->ops->get_sleepstate();//sleepstate
    workingstate = p_mdi->ops->get_workingstate();//workingstate

    //WANGXIAOWEI_MODEM_STATE_20100604_01, begin
    return snprintf(buf, PAGE_SIZE, "%u\n", workingstate);
    //WANGXIAOWEI_MODEM_STATE_20100604_01, end
}


//WANGXIAOWEI_MODEM_STATE_20100604_01, begin
/**********************************************************************
* Function:       the following function to operate the audio_sw_state property of sysfs interface
* Description:    to read or write the select property ---- driver_attr_audio_sw_state
* Input:            struct platform_driver *drv
* Output:         
* Return:           the state of audio_sw
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/06/04     V1.0      wxw       create
**********************************************************************/
static ssize_t modemctl_show_audio_sw_state(struct device_driver *driver, char *buf)
{
    unsigned int audio_sw_state = 0;
    MODEMCTL_INFO("++ ");
    audio_sw_state = p_mdi->ops->get_gpiostate( MODEMCTL_GPIO_AUDIO_SW );
    MODEMCTL_INFO("-- ");
    return snprintf(buf, PAGE_SIZE, "%u\n", audio_sw_state);
}
/**********************************************************************
* Function:       the following function to operate the at_sw_state property of sysfs interface
* Description:    to read or write the select property ---- driver_attr_at_sw_state
* Input:            struct platform_driver *drv
* Output:         
* Return:           the state of at_sw
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/06/04     V1.0      wxw       create
**********************************************************************/
static ssize_t modemctl_show_at_sw_state(struct device_driver *driver, char *buf)
{
    unsigned int at_sw_state = 0;
    at_sw_state = p_mdi->ops->get_gpiostate( MODEMCTL_GPIO_AT_SW );
    return snprintf(buf, PAGE_SIZE, "%u\n", at_sw_state);
}
/**********************************************************************
* Function:       the following function to operate the board_state property of sysfs interface
* Description:    to read or write the select property ---- driver_attr_board_state
* Input:            struct platform_driver *drv
* Output:         
* Return:           the state of board_state
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/06/04     V1.0      wxw       create
**********************************************************************/
static ssize_t modemctl_show_board_state(struct device_driver *driver, char *buf)
{
    unsigned int board_state = 0;
    board_state = p_mdi->ops->get_gpiostate( MODEMCTL_GPIO_BOARD_STATE );
    return snprintf(buf, PAGE_SIZE, "%u\n", board_state);
}
//WANGXIAOWEI_MODEM_STATE_20100604_01, end


//WANGXIAOWEI_GPIOSTATE_20100519_02, begin
/**********************************************************************
* Function:       modemctl_showgpiostate
* Description:    to show the states of interrelated GPIO
* Input:           void
* Output:         void
* Return:           none
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/05/19     V1.0      wxw       create
**********************************************************************/
static void modemctl_showgpiostate(void)
{
    //unsigned long gpio_states = 0;
    unsigned int gpio_states = 0;
    MODEMCTL_INFO("++");
    /*
    if( gpio_get_value(MSM_GPIO_C2G_IRQ) )
    {
        set_bit( 0x01, &gpio_states);
    }
    if( gpio_get_value(MSM_GPIO_C2G_READY) )
    {
        set_bit( 0x02, &gpio_states);
    }
    if( gpio_get_value(MSM_GPIO_GSM_KEY_ON) )
    {
        set_bit( 0x04, &gpio_states);
    }
    if( gpio_get_value(MSM_GPIO_GSM_RESET) )
    {
        set_bit( 0x08, &gpio_states);
    }

    if( gpio_get_value(MSM_GPIO_G2C_IRQ) )
    {
        set_bit( 0x10, &gpio_states);
    }
    if( gpio_get_value(MSM_GPIO_G2C_READY) )
    {
        set_bit( 0x20, &gpio_states);
    }
    if( gpio_get_value(MSM_GPIO_GSM_AT_SW) )
    {
        set_bit( 0x40, &gpio_states);
    }
    if( gpio_get_value(MSM_GPIO_GSM_WORKING) )
    {
        set_bit( 0x80, &gpio_states);
    }
    */
    MODEMCTL_INFO("@@ the gpio_states = %x \n", gpio_states);
    MODEMCTL_INFO("\n @@ MSM_GPIO_C2G_IRQ = %d \n @@ MSM_GPIO_C2G_READY =%d \n \
    @@ MSM_GPIO_GSM_KEY_ON = %d  \n @@ MSM_GPIO_GSM_RESET= %d \n @@ MSM_GPIO_G2C_IRQ =%d \n \
    @@ MSM_GPIO_G2C_READY = %d  \n @@ MSM_GPIO_GSM_AT_SW = %d \n @@ MSM_GPIO_GSM_WORKING = %d \n", \
    gpio_get_value(MSM_GPIO_C2G_IRQ), gpio_get_value(MSM_GPIO_C2G_READY), gpio_get_value(MSM_GPIO_GSM_KEY_ON), \
    gpio_get_value(MSM_GPIO_GSM_RESET), gpio_get_value(MSM_GPIO_G2C_IRQ), gpio_get_value(MSM_GPIO_G2C_READY), \
    gpio_get_value(MSM_GPIO_GSM_AT_SW), gpio_get_value(MSM_GPIO_GSM_WORKING));
    
    MODEMCTL_INFO("--");
    
}
//WANGXIAOWEI_GPIOSTATE_20100519_02, end


//WANGXIAOWEI_READY_20100409, begin
/**********************************************************************
* Function:       modemctl_wakeup_modem
* Description:    to wake up the modem and wait for the g2c_ready response
* Input:           void
* Output:         ret : 0,the g2c_ready response OK
                            MODEMCTL_WAIT_MODEMRESP_TIMEOUT: wait queue time out
* Return:           none
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/03/11     V1.0      wxw       create
**********************************************************************/
//WANGXIAOWEI_READY_20100409, first version using the interrupt mechanism, begin
#if 0
static int modemctl_wakeup_modem(void)
{
    int ret = 0;
    MODEMCTL_INFO("++");

	//if (test_bit(MODEMCTL_ALLOW_SLEEP, &flags)) {
	
		//here forbid the ap to sleep
		wake_lock(&p_mdi->wake_lock);
                
            //WANGXIAOWEI_G2C_READY_RESP_20100330, begin, here enable the g2c_ready interrupt

            //fllow the spec, here should wake up the modem before send data using uart dm2
            p_mdi->ops->wake_modem();
            
            clear_bit(MODEMCTL_MODEM_WAKE_RESP, &flags);
            //wait_event_interruptible_timeout the function returns 0 if the @timeout elapsed, 
            //-ERESTARTSYS if it was interrupted by a signal, and the remaining jiffies otherwise
            //if the condition evaluated to true before the timeout elapsed.
            MODEMCTL_INFO("@@ befor wait_event_interruptible_timeout function");
            
            ret = wait_event_interruptible_timeout(p_mdi->event_wait, test_bit(MODEMCTL_MODEM_WAKE_RESP, &flags), MODEMCTL_MODEM_RESP_POLL_TIME);
            MODEMCTL_INFO("@@ after wait_event_interruptible_timeout function, the ret = %d \n", ret);
            if( ret == 0 )      //ret = 0 ----the @timeout elapsed
            {
                ret = MODEMCTL_WAIT_MODEMRESP_TIMEOUT;
            }
            //the remaining jiffies otherwise if the condition evaluated to true before the timeout elapsed.
            else if( (ret > 0) && ( ret <= MODEMCTL_MODEM_RESP_POLL_TIME ) )           
            {
                ret = 0;        //means OK, so, the cmd store return zhe character of write into cmd--sysfs
            }
            //the other is -ERESTARTSYS, -512, it was interrupted by a signal
            
            //WANGXIAOWEI_G2C_READY_RESP_20100330, begin, here enable the g2c_ready interrupt            
            
            MODEMCTL_INFO("@@ after wake the modem and mod_timer start the ready_timer \n");
		/* Start the timer to judge whether uart dm2 fifo empty */
		mod_timer(&ready_timer, jiffies + (TX_TIMER_INTERVAL * HZ));
		//clear_bit(MODEMCTL_ALLOW_SLEEP, &flags);
		/*Activating UART */
		//hsuart_power(1);
	//}

    MODEMCTL_INFO("--");
    return ret;

    
}
#endif
//WANGXIAOWEI_READY_20100409, first version using the interrupt mechanism, end

//WANGXIAOWEI_READY_20100409, Second version using the timer check mechanism, begin
#if 0
static int modemctl_wakeup_modem(void)
{
    int ret = 0;
    MODEMCTL_INFO("++");

            //fllow the spec, here should wake up the modem before send data using uart dm2
            // the return is gpio_get_value(MSM_GPIO_G2C_READY)
            if(GPIO_HIGH_LEVEL  != (p_mdi->ops->wake_modem()))
            {

                // after wake up modem if the MSM_GPIO_G2C_READY is 0, means 
                // the MSM_GPIO_G2C_READY will be pull up after about 10ms
                MODEMCTL_INFO("@@ after wake the modem and the G2C_Ready is Low, so start the wait_event_interruptible_timeout flow \n");
                
                clear_bit(MODEMCTL_MODEM_WAKE_RESP, &flags);

                 /* Start the timer to judge whether response signal */
                MODEMCTL_INFO("@@ befor mod_timer function to start the timer");
               
        	    mod_timer(&respsignal_timer, jiffies + msecs_to_jiffies(10));

                                
                //wait_event_interruptible_timeout the function returns 0 if the @timeout elapsed, 
                //-ERESTARTSYS if it was interrupted by a signal, and the remaining jiffies otherwise
                //if the condition evaluated to true before the timeout elapsed.
                MODEMCTL_INFO("@@ befor wait_event_interruptible_timeout function");
                ret = wait_event_interruptible_timeout(p_mdi->event_wait, test_bit(MODEMCTL_MODEM_WAKE_RESP, &flags), MODEMCTL_MODEM_RESP_POLL_TIME);
                
                MODEMCTL_INFO("@@ after wait_event_interruptible_timeout function, the ret = %d \n", ret);
                if( ret == 0 )      //ret = 0 ----the @timeout elapsed
                {
                    ret = MODEMCTL_WAIT_MODEMRESP_TIMEOUT;
                }
                //the remaining jiffies otherwise if the condition evaluated to true before the timeout elapsed.
                else if( (ret > 0) && ( ret <= MODEMCTL_MODEM_RESP_POLL_TIME ) )           
                {
                    ret = 0;        //means OK, so, the cmd store return zhe character of write into cmd--sysfs
                }
                //the other is -ERESTARTSYS, -512, it was interrupted by a signal


            }
            else
            {
                // after wake up modem if the MSM_GPIO_G2C_READY is 1, means 
                // the MSM_GPIO_G2C_READY have already been pulled up.
               MODEMCTL_INFO("@@ after wake the modem and the G2C_Ready is high, so the modem is already wake \n");   
            }
    
    MODEMCTL_INFO("--");
    return ret;
}


static void modemctl_respsignal_timer_expire(unsigned long data)
{
    int resp_signal = 0;
    static int timer_num = 0;

    MODEMCTL_INFO("++");
    
    resp_signal = p_mdi->ops->get_respsignal();

    if ((resp_signal == GPIO_HIGH_LEVEL) && ( timer_num < MAX_RESPSIGNAL_TIMERNUM ))
    {
        //the uart dm1 have sent the data over, so here allow to sleep
        MODEMCTL_INFO(" the dm1 empty, the data has been send over, can go to sleep...");
        //set_bit(MODEMCTL_ALLOW_SLEEP, &flags);
            /* wake up the wait queue */
        MODEMCTL_INFO(" set_bit MODEMCTL_MODEM_WAKE_RESP, and then call wake_up_interruptible");
        set_bit(MODEMCTL_MODEM_WAKE_RESP, &flags);
        
        timer_num = 0;    
        wake_up_interruptible(&p_mdi->event_wait);

    }
    else if ( (resp_signal != GPIO_HIGH_LEVEL) && ( timer_num < MAX_RESPSIGNAL_TIMERNUM ) )
    {
        /* Start the timer to judge whether response signal */
        MODEMCTL_INFO(" after the first 10ms timer_expire, the G2C_READY is Low, timer_num = %d ", timer_num);
        mod_timer(&respsignal_timer, jiffies + msecs_to_jiffies(1));
        timer_num++;
    }
    else
    {
        MODEMCTL_INFO("** Error, the G2C_READY still is Low, so GIVE UP, timer_num = %d ", timer_num);
        timer_num = 0;   
    }
    MODEMCTL_INFO("--");
    
}
#endif
//WANGXIAOWEI_READY_20100409, Second version using the timer check mechanism, begin
//WANGXIAOWEI_READY_20100409, end


//WANGXIAOWEI_25MS_20100416, begin
/**********************************************************************
* Function:       modemctl_wakeup_modem
* Description:    to wake up the modem and wait for 25MS, now using 25ms version, don't use the ready signal
* Input:           void
* Output:         ret : 0 wake up modem ok, !0 wake up modem error
* Return:           none
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/03/11     V1.0      wxw       create
**********************************************************************/
static int modemctl_wakeup_modem(void)
{
    int ret = 0;
    MODEMCTL_INFO("++");

    if( !(test_bit(MODEMCTL_SEND_DATA, &flags)) )
    {
        MODEMCTL_INFO("@@ have not set the MODEMCTL_SEND_DATA, and the MSM_GPIO_C2G_IRQ = %d  \n", gpio_get_value(MSM_GPIO_C2G_IRQ));
        ret = p_mdi->ops->wake_modem();
        delay(25);
        /* data passing by */
        set_bit(MODEMCTL_SEND_DATA, &flags);


        //WANGXIAOWEI_PM_20100520_01, begin
        //here forbid the ap to sleep
	 wake_lock(&p_mdi->wake_lock);
        //WANGXIAOWEI_PM_20100520_01, end
        
        //WANGXIAOWEI_UARTCLOCK_20100519_01, begin
        hsuart_power(UART_CLOCK_ON);
        //WANGXIAOWEI_UARTCLOCK_20100519_01, end
        
        MODEMCTL_INFO("@@ after wake the modem and set the MODEMCTL_SEND_DATA, mod_timer start the ready_timer \n");
    }
    else
    {
        MODEMCTL_INFO("@@ Have set the MODEMCTL_SEND_DATA, and the MSM_GPIO_C2G_IRQ = %d  \n", gpio_get_value(MSM_GPIO_C2G_IRQ));
        MODEMCTL_INFO("@@ so here don't need to send IRQ to wake up the modem.\n");
    }

    /* Start the timer to judge whether uart dm2 fifo empty */
    mod_timer(&ready_timer, jiffies + (TX_TIMER_INTERVAL * HZ));
        
    MODEMCTL_INFO("--");
    return ret;
}
//WANGXIAOWEI_25MS_20100416, end


/**********************************************************************
* Function:       modemctl_ready_timer_expire
* Description:    to check the data send over then close 
* Input:           void
* Output:         ret : 0
* Return:           none
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/03/11     V1.0      wxw       create
**********************************************************************/
static void modemctl_ready_timer_expire(unsigned long data)
{
    //unsigned long irq_flags;
    unsigned int uart_state = 0;
    int ret = 0;
    
    MODEMCTL_INFO("++");
    //during the period of check msm_hs_dm1_tx_empty, should irqsave.
    //spin_lock_irqsave(&rw_lock, irq_flags);

    uart_state = msm_hs_dm1_tx_empty();
    if (uart_state == TIOCSER_TEMT)
    {
        //the uart dm1 have sent the data over, so here allow to sleep
        MODEMCTL_INFO("@@ the dm1 empty, the data has been send over, can go to sleep...");

        //WANGXIAOWEI_25MS_20100416, begin
        MODEMCTL_INFO("@@ and clean the MODEMCTL_SEND_DATA bit.");
        clear_bit(MODEMCTL_SEND_DATA, &flags);
        
        //set_bit(MODEMCTL_ALLOW_SLEEP, &flags);
        ret = p_mdi->ops->notify_modem(MODEMCTL_NOTIFY_SENDDATA_READY);
        if(ret)
        {
             MODEMCTL_ERR(" ** Error: the notify_modem operation fail.");
             goto fail_return;
        }

        //WANGXIAOWEI_PM_20100520_01, begin
        //WANGXIAOWEI_DEBUG_UART_20100525_01, begin
        #if 0
        /*Deactivating UART */
	  hsuart_power(UART_CLOCK_OFF);
	  /* UART clk is not turned off immediately. Release
		* wakelock after 500 ms.
		*/
	  wake_lock_timeout(&p_mdi->wake_lock, HZ / 2);
         #endif
	  wake_unlock(&p_mdi->wake_lock);
        //WANGXIAOWEI_DEBUG_UART_20100525_01, end
        //WANGXIAOWEI_PM_20100520_01, end
        
        
        //WANGXIAOWEI_GPIOSTATE_20100519_02, begin
        //modemctl_showgpiostate();
        //WANGXIAOWEI_GPIOSTATE_20100519_02, end

        //WANGXIAOWEI_25MS_20100416, end
    }
    else if( uart_state == MODEMCTL_UARTDM1_UNINIT )
    {
        MODEMCTL_ERR("** Error: the dm1 not init...");
        goto fail_return;
    }
    else            //the data have not been sent over
    {
        mod_timer(&ready_timer, jiffies + (TX_TIMER_INTERVAL * HZ));
    }
    
    //spin_unlock_irqrestore(&rw_lock, irq_flags);
    MODEMCTL_INFO("--");
    
fail_return:
    //spin_unlock_irqrestore(&rw_lock, irq_flags);
    MODEMCTL_INFO("--");
    
}

/**********************************************************************
* Function:       modemctl_pre_write2uart
* Description:    Operations before write to uart dm2 in Modemctl 
* Input:           void
* Output:         
* Return:           none
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/03/27     V1.0      wxw       create
**********************************************************************/
static int modemctl_pre_write2uart(void)
{
    //unsigned long irq_flags;
    int ret = 0;
    MODEMCTL_INFO("++");
    
    //spin_lock_irqsave(&rw_lock, irq_flags);
    /* the IFX GSM modem need to be waked up before AP send AT*/
    ret = modemctl_wakeup_modem();
    
    //spin_unlock_irqrestore(&rw_lock, irq_flags);
    
    MODEMCTL_INFO("--");
    return ret;
}


/**********************************************************************
* Function:       hsuart_power(int on)
* Description:    power on or shut down the uart DM1
* Input:           on: power on or shut down
* Output:         
* Return:           none
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/03/27     V1.0      wxw       create
**********************************************************************/
static void hsuart_power(int on)
{
    struct uart_port * dm1_port = NULL;
    static int swith_flag=-1;// 1 :on  0: close   -1:init
    MODEMCTL_INFO("++");
    MODEMCTL_INFO("the swich_flag = %d", swith_flag);
    if(on != swith_flag)
    {
        swith_flag=on;
        MODEMCTL_WAR("TTTT ytc:%s uart clock",on?"open":"close");
        dm1_port = msm_hs_dm1_getuartport();
        if( dm1_port != NULL)
        {
            if (on)
            {
                msm_hs_request_clock_on(dm1_port);
                msm_hs_set_mctrl(dm1_port, TIOCM_RTS);
            }
            else
            {
                msm_hs_set_mctrl(dm1_port, 0);
                msm_hs_request_clock_off(dm1_port);
            }
        }
        else
        {
            MODEMCTL_ERR("** Error: get the uart port of dm1\n");
        }
    }
    MODEMCTL_INFO("--");
}


//WANGXIAOWEI_READY_20100409, begin
#if 0
/**********************************************************************
* Function:       modemctl_modemresp_isr
* Description:    the ISR for ready signal IRQ, will call this function. 
                       Now using 25ms version don't support ready signal.
* Input:            int irq, void *dev_id
* Output:         
* Return:           none
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/03/11     V1.0      wxw       create
**********************************************************************/
static irqreturn_t modemctl_modemresp_isr(int irq, void *dev_id)
{

    MODEMCTL_INFO("++");
    gpio_clear_detect_status(p_mdi->gpio_info->g2c_ready);

    /* wake up the wait queue */
    MODEMCTL_INFO(" set_bit MODEMCTL_MODEM_WAKE_RESP, and then call wake_up_interruptible");
    set_bit(MODEMCTL_MODEM_WAKE_RESP, &flags);
    
    wake_up_interruptible(&p_mdi->event_wait);
    
    MODEMCTL_INFO("--");    
    return IRQ_HANDLED;
}
#endif
//WANGXIAOWEI_READY_20100409, end

/**********************************************************************
* Function:       modemctl_hostwake_isr
* Description:    when the gsm modem wake up ap, will call this function
* Input:            int irq,  struct platform_driver *drv
* Output:         
* Return:           none
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/03/11     V1.0      wxw       create
**********************************************************************/
static irqreturn_t modemctl_hostwake_isr(int irq, void *dev_id)
{
    gpio_clear_detect_status(p_mdi->gpio_info->g2c_irq);

    /* schedule a tasklet to handle the change in the host wake line */
    tasklet_schedule(&hostwake_task);
    return IRQ_HANDLED;
}

/**********************************************************************
* Function:       modemctl_hostwake_task
* Description:    this is the ISR function, when the gsm modem wake up ap, 
* Input:            unsigned long data
* Output:         
* Return:           none
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/03/11     V1.0      wxw       create
**********************************************************************/
static void modemctl_hostwake_task(unsigned long data)
{
    MODEMCTL_INFO(" ++");
    
    //after modem wakeup AP using GPIO42, will enter this function
    //here keep wakelock 500 ms to prevent reenter to sleep quickly
    //WANGXIAOWEI_PM_0427,begin
    MODEMCTL_INFO("@@ call wake_lock_timeout to set a 5s keep wakelock 5s");
    wake_lock_timeout(&p_mdi->wake_lock, MODEMCTL_AFTERIRQ_WAKELOCK_TIME);
    //WANGXIAOWEI_PM_0427,end

    //WANGXIAOWEI_UARTCLOCK_20100519_01, begin
    MODEMCTL_INFO(" TTTT: before call hsuart_power to open the uart clock\n");
    hsuart_power(UART_CLOCK_ON);
    MODEMCTL_INFO("TTTT: after call hsuart_power to open the uart clock\n");
    //WANGXIAOWEI_UARTCLOCK_20100519_01, end
    
    MODEMCTL_INFO(" --");
}


//WANGXIAOWEI_PM_0427, begin, add just for suspend resume
/**********************************************************************
* Function:       modemctl_suspend
* Description:    when AP suspend will enter this function
* Input:           
* Output:         
* Return:           none
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/04/27     V1.0      wxw       create
**********************************************************************/
static int modemctl_suspend(struct platform_device *pdev, pm_message_t state)
{
    int  ret = 0;
    MODEMCTL_INFO(" ++");


    //WANGXIAOWEI_UARTCLOCK_20100519_01, begin
     //ZTE_MODMEMCTL_HYJ_20100624  add check the ilde state before close uart clock       
     if(msm_hs_dm1_tx_empty() == TIOCSER_TEMT &&  //tx stop
		gpio_get_value(MSM_GPIO_G2C_IRQ) == GPIO_LOW_LEVEL)//rx stop
	{
		MODEMCTL_INFO("@@ notify_modem(MODEMCTL_NOTIFY_APSUSPEND) set the MSM_GPIO_C2G_READY LOW");

		ret = p_mdi->ops->notify_modem(MODEMCTL_NOTIFY_APSUSPEND);
		if(!ret)
		{
			MODEMCTL_INFO(" TTTT: before call the hsuart_power to close uart clock.\n");		
			hsuart_power(UART_CLOCK_OFF);
			MODEMCTL_INFO(" TTTT: after call the hsuart_power to close uart clock.\n");
		}
		else
		{
			 MODEMCTL_ERR(" ** Error, the notify_modem operation fail.");
		}
	}

   //ZTE_MODMEMCTL_HYJ_20100624 add check the idle state before close uart clock      
    //WANGXIAOWEI_UARTCLOCK_20100519_01, end	

    //WANGXIAOWEI_GPIOSTATE_20100519_02, begin
    modemctl_showgpiostate();
    //WANGXIAOWEI_GPIOSTATE_20100519_02, end

    MODEMCTL_INFO(" --");
    return ret;
}

/**********************************************************************
* Function:       modemctl_resume
* Description:    when AP resume will enter this function
* Input:           
* Output:         
* Return:           none
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/04/27     V1.0      wxw       create
**********************************************************************/
static int modemctl_resume(struct platform_device *pdev)
{
    int  ret = 0;
    MODEMCTL_INFO(" ++");
    
    MODEMCTL_INFO("@@ notify_modem(MODEMCTL_NOTIFY_APRESUME) set the MSM_GPIO_C2G_READY HIGH");

    ret = p_mdi->ops->notify_modem(MODEMCTL_NOTIFY_APRESUME);
    if(ret)
    {
        MODEMCTL_ERR(" ** Error: the notify_modem operation fail.");
    }
    //hsuart_power(1);
    
    //WANGXIAOWEI_GPIOSTATE_20100519_02, begin
    modemctl_showgpiostate();
   //WANGXIAOWEI_GPIOSTATE_20100519_02, end
    
    MODEMCTL_INFO(" --");
    return ret;
}
//WANGXIAOWEI_PM_0427, end, add just for suspend resume

/**********************************************************************
* Function:       modemctl_probe
* Description:    when the driver match the device, will enter this function
                       to get resource and init main struct p_mdi here.
* Input:            struct platform_device *pdev
* Output:         
* Return:           none
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/03/11     V1.0      wxw       create
**********************************************************************/
static int __init modemctl_probe(struct platform_device *pdev)
{
	int ret;
	struct resource *res;

	MODEMCTL_INFO(" ++");

      /*allocate memory for the main struct. The memory is set to zero*/
	p_mdi = kzalloc(sizeof(struct modemctl_device_info), GFP_KERNEL);
	if (!p_mdi){
		return -ENOMEM;
      }
	p_mdi->gpio_info = kzalloc(sizeof(struct modemctl_device_gpio_info), GFP_KERNEL);
	if (!p_mdi->gpio_info ){
		ret = -ENOMEM;
		goto free_mdi_device_only;
      }

	p_mdi->sysfs_info = kzalloc(sizeof(struct modemctl_sysfs_info), GFP_KERNEL);
	if (!p_mdi->sysfs_info ){
		ret = -ENOMEM;
		goto free_mdi_gpio_info;
      }

       /* init waitqueue head */
      init_waitqueue_head(&p_mdi->event_wait);


      /* get the modemctl device resources */
      //the gsm modem open pin
	res = platform_get_resource_byname(pdev, IORESOURCE_IO,"GSM_KEY_ON");
	if (!res) {
		MODEMCTL_ERR(" ** Error:couldn't find GSM_KEY_ON gpio\n");
		ret = -ENODEV;
		goto free_mdi;
	}
	p_mdi->gpio_info->gsm_key_on= res->start;

	MODEMCTL_INFO("@@ the gsm modem open pin \n");
    
    /*
	ret = gpio_request(p_mdi->gpio_info->gsm_key_on, "GSM_KEY_ON");
	if (ret)
		goto free_gsm_ken_on;
*/
      //the reset pin
	res = platform_get_resource_byname(pdev, IORESOURCE_IO,"GSM_RESET");
	if (!res) {
		MODEMCTL_ERR(" ** Error: couldn't find GSM_RESET gpio\n");
		ret = -ENODEV;
		goto free_mdi;
	}
	p_mdi->gpio_info->gsm_reset= res->start;


	MODEMCTL_INFO("@@ the gsm the reset pin \n");

    
      //the modem to ap irq wakeup pin, and this pin also used as gpio to up or down
    	res = platform_get_resource_byname(pdev, IORESOURCE_IO,"G2C_IRQ_IO");
	if (!res) {
		MODEMCTL_ERR(" ** Error: couldn't find GSM_RESET gpio\n");
		ret = -ENODEV;
		goto free_mdi;
	}
	p_mdi->gpio_info->g2c_irq_io= res->start;

	MODEMCTL_INFO("@@ the modem to ap irq wakeup pin, \n");

     //the modem to ap irq wakeup pin, 
    	res = platform_get_resource_byname(pdev, IORESOURCE_IRQ,"G2C_IRQ");
	if (!res) {
		MODEMCTL_ERR(" ** Error:couldn't find G2C_IRQ gpio\n");
		ret = -ENODEV;
		goto free_mdi;
	}
	p_mdi->gpio_info->g2c_irq= res->start;

	MODEMCTL_INFO("@@ the modem to ap irq wakeup pin,  \n");

       //GPIO_75, to interrupt gsm for ready, and show the state of ap sleep.
    	res = platform_get_resource_byname(pdev, IORESOURCE_IO,"C2G_READY");
	if (!res) {
		MODEMCTL_ERR(" ** Error: couldn't find C2G_READY gpio\n");
		ret = -ENODEV;
		goto free_mdi;
	}
	p_mdi->gpio_info->c2g_ready= res->start;

       //GPIO_74, to interrupt gsm for wake up
    	res = platform_get_resource_byname(pdev, IORESOURCE_IO,"C2G_IRQ");
	if (!res) {
		MODEMCTL_ERR(" ** Error: couldn't find C2G_IRQ gpio\n");
		ret = -ENODEV;
		goto free_mdi;
	}
	p_mdi->gpio_info->c2g_irq= res->start;

      //GPIO_78, here just usded by gsm to show the state of modem sleep.
    	res = platform_get_resource_byname(pdev, IORESOURCE_IO,"G2C_READY_IO");
	if (!res) {
		MODEMCTL_ERR(" ** Error: couldn't find G2C_READY gpio\n");
		ret = -ENODEV;
		goto free_mdi;
	}
	p_mdi->gpio_info->g2c_ready_io= res->start;

	MODEMCTL_INFO("@@ p_mdi->gpio_info->g2c_ready_io = %d  \n", p_mdi->gpio_info->g2c_ready_io);
	MODEMCTL_INFO("@@ p_mdi->gpio_info->g2c_irq = %d  \n", p_mdi->gpio_info->g2c_irq);
	
      //ZTE_MODMECTL_WXW_G2C_RESP_20100330, begin, add the G2C_READY IRQ signal
      //GPIO_78, here just usded by gsm to show the state of modem sleep.
    	res = platform_get_resource_byname(pdev, IORESOURCE_IRQ,"G2C_READY");
	if (!res) {
		MODEMCTL_ERR(" ** Error: couldn't find G2C_READY gpio as irq gpio\n");
		ret = -ENODEV;
		goto free_mdi;
	}
	p_mdi->gpio_info->g2c_ready= res->start;

//WANGXIAOWEI_READY_20100409, begin
/*
	MODEMCTL_INFO("@@ p_mdi->gpio_info->g2c_ready = %d  \n", p_mdi->gpio_info->g2c_ready);
	ret = request_irq(p_mdi->gpio_info->g2c_ready, modemctl_modemresp_isr, IRQF_DISABLED |IRQF_TRIGGER_RISING,"modemctl modemresp",NULL);
	if (ret  < 0) {
		MODEMCTL_ERR("Couldn't acquire modemctl_modemresp g2c_ready IRQ");
		return ret;
	}
    
	MODEMCTL_INFO("@@ request_irq(p_mdi->gpio_info->g2c_ready, ,  \n");

*/
//WANGXIAOWEI_READY_20100409, end
    
      //ZTE_MODMECTL_WXW_G2C_RESP_20100330, end, add the G2C_READY IRQ signal


      //init wake_lock 
	wake_lock_init(&p_mdi->wake_lock, WAKE_LOCK_SUSPEND, "modemctl");

      /* register the irq*/
      //the IRQF_TRIGGER_FALLING in the Interrupt.h
      //IRQF_DISABLED - keep irqs disabled when calling the action handler
	ret = request_irq(p_mdi->gpio_info->g2c_irq, modemctl_hostwake_isr, IRQF_DISABLED |IRQF_TRIGGER_RISING,"modemctl hostwake",NULL);
	if (ret  < 0) {
		MODEMCTL_ERR(" ** Error: Couldn't acquire MODEMCTL_HOST_WAKE -- G2C_IRQ");
		goto free_mdi;
	}
    
	MODEMCTL_INFO("@@ request_irq(p_mdi->gpio_info->g2c_irq, ,  \n");
    
	ret = enable_irq_wake(p_mdi->gpio_info->g2c_irq);
	if (ret < 0) {
		MODEMCTL_ERR(" ** Error: Couldn't enable MODEMCTL_HOST_WAKE -- G2C_IRQ as wakeup interrupt");
		free_irq(p_mdi->gpio_info->g2c_irq, NULL);
		goto free_mdi;
	}

      //WANGXIAOWEI_PM_0427, begin, add just for suspend resume
	MODEMCTL_INFO(" @@ set the p_mdi->ops = &modem_ifxgsm_ops, for suspend resume init ");
      p_mdi->ops = &modem_ifxgsm_ops;
       //WANGXIAOWEI_PM_0427, end, add just for suspend resume 
       
	MODEMCTL_INFO(" --");
    
	return 0;

free_mdi:
      kfree(p_mdi->sysfs_info);
free_mdi_gpio_info:
      kfree(p_mdi->gpio_info);
free_mdi_device_only:
      kfree(p_mdi);
	 p_mdi->ops =NULL;
	return ret;

}

/*HUANGYANJUN_PM_20100504 begin*/
/**********************************************************************
* Function:       mainctl_reboot_call
* Description:   close the modem  before system power off or resart
* Input:            
* Output:         
* Return:           none
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/05/04     V1.0      hyj       create
**********************************************************************/
static int mainctl_reboot_call
	(struct notifier_block *this, unsigned long code, void *_cmd)
{
	int ret = -1;
	
	if ((code == SYS_POWER_OFF) ||(code == SYS_RESTART)) {
		ret = p_mdi->ops->close_modem();
	}
	
       printk(KERN_INFO "[HYJ@MODEM] close modem:  %s\n",ret?"fail !!!":"success !!!");
	
	return NOTIFY_DONE;
}

/*
 * notifier_block declare
 */
static struct notifier_block mainctl_reboot_notifier = {
	.notifier_call = mainctl_reboot_call,
};
/*HUANGYANJUN_PM_20100504 end*/

/**********************************************************************
* Function:       modemctl_init
* Description:    Initializes the module.
* Input:            
* Output:         return On success, 0. On error, -1, and <code>errno</code> is set
* Return:           none
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/03/11     V1.0      wxw       create
**********************************************************************/
static int __init modemctl_init(void)
{
	int retval;

	MODEMCTL_INFO("enter modemctl_init Driver Ver %s ++", VERSION);


      /* register the modemctl driver */
	retval = platform_driver_register(&modemctl_driver);
	if (retval)
      {   
		return retval;
       }
    
	/* sysfs interface create */
	retval = modemclt_create_driver_sysfs(&modemctl_driver);
	if (retval)
      {   
		return retval;
       }
    
      /* init the sysfs interface variables of modemctl */
      modemclt_init_sysfs_info();
          
       /* clear all status bits */
	flags = 0;

	/* Initialize spinlock. */
	spin_lock_init(&rw_lock);


	/* Initialize timer */
	init_timer(&ready_timer);
	ready_timer.function = modemctl_ready_timer_expire;
	ready_timer.data = 0;
	
/*HUANGYANJUN_PM_20100504 */
	register_reboot_notifier(&mainctl_reboot_notifier);

//WANGXIAOWEI_25MS_20100416
#if 0
	/* Initialize timer */
	init_timer(&respsignal_timer);
	respsignal_timer.function = modemctl_respsignal_timer_expire;
	respsignal_timer.data = 0;
#endif
//WANGXIAOWEI_25MS_20100416
       
	MODEMCTL_INFO(" modemctl_init Driver Ver %s --", VERSION);

	return 0;

}


/**********************************************************************
* Function:       modemctl_exit
* Description:    exit the module.
* Input:            
* Output:         
* Return:           none
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/03/11     V1.0      wxw       create
**********************************************************************/
static void __exit modemctl_exit(void)
{
	/* assert other operations */

       /* unregister notifier interface */
	//modemctl_register_notifier(&modemctl_event_nblock);
	wake_lock_destroy(&p_mdi->wake_lock);
	if (disable_irq_wake(p_mdi->gpio_info->g2c_irq))
      {   
          MODEMCTL_ERR(" ** Error: Couldn't disable hostwake IRQ wakeup mode \n");
      }
       free_irq(p_mdi->gpio_info->g2c_irq, NULL);
    
	del_timer(&ready_timer);
       /* clear all status bits */
	flags = 0;
       
      /* unregister notifier interface */
       modemclt_remove_driver_sysfs(&modemctl_driver);

       /* unregister the modemctl driver */
	platform_driver_unregister(&modemctl_driver);
	   
	/*HUANGYANJUN_PM_20100504 */
       unregister_reboot_notifier(&mainctl_reboot_notifier);
	   
      kfree(p_mdi->gpio_info);
      kfree(p_mdi->sysfs_info);
      kfree(p_mdi);
}



module_init(modemctl_init);
module_exit(modemctl_exit);

MODULE_DESCRIPTION("Modem Control Mode Driver ver %s. Create by wangxiaowei@ZTE " VERSION);
#ifdef MODULE_LICENSE
MODULE_LICENSE("GPL");
#endif


