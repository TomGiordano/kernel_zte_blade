/**
-----------------------------------------------------------------------------
Copyright (c) 2010~ ZTE Corporation, Incorporated. 
All Rights Reserved. ZTE Proprietary and Confidential.
-----------------------------------------------------------------------------
 *
 * @Filename: modemctl_ifxgsm.c
 * Description: the file for the modemctl driver, do the operation on the device of IFX gsm modem.
 * Others:     
 * Version:    1.0
 * Author:     wangxiaowei
 */


/*===========================================================================

                        EDIT HISTORY FOR MODULE


  when            who  what, where, why
  ----------   ---   -----------------------------------------------------------
  2010-07-20  hyj  modify the gpio config for pm .flag: ZTE_MODMEMCTL_HYJ_20100720
  2010-06-12  hyj  modify the debug code.flag: ZTE_MODMEMCTL_HYJ_20100618
  2010-06-04  hyj  to add delay during the reset and close   HUANGYANJUN_IFX_MODEM_20100603_01
  2010-05-20  wxw  to check the working state before close & open  WANGXIAOWEI_PM_20100520_02
  2010-05-20  wxw  to debug the modem cann't sleep when AP suspend   WANGXIAOWEI_PM_20100520_01
  2010-04-20  wxw  to debug the working high after gsm reset    WANGXIAOWEI_RESET_20100420
  2010-04-09  wxw  validate ready pin feature by timer              WANGXIAOWEI_READY_20100409
  2010-04-07  wxw  pull down the AT_SW pin to make the uart only connect to usb 10 pin    WANGXIAOWEI_ATSW_DOWN_001
  2010-03-28  wxw  change the reset function, 
                            because of there is a not gate on the line of gsm reset              WANGXIAOWEI_RESET_20100331
  2010-03-08  wxw  create
===========================================================================*/

/*===========================================================================
						 INCLUDE FILES FOR MODULE
===========================================================================*/

#include <modemctl.h>	/* kernel module definitions */

/*=========================================================================*/
/*                               CONSTANTS                                 */
/*=========================================================================*/


/*=========================================================================*/
/*                                MACROS                                   */
/*=========================================================================*/
#define MODEMCTL_OPENMODEM_MAX_TIME   3
#define MODEM_WORKING   1
#define MODEM_UN_WORKING   0

#define MODEM_SLEEPING   1
#define MODEM_WAKEING   0

//#define MODEMCTL_FORRF_VERSION

/*=========================================================================*/
/*                            Debug Definitions                          */
/*=========================================================================*/
//ZTE_MODMEMCTL_HYJ_20100618, begin, add the debug code
enum {
	IFX_DEBUG_INFO = 1U << 0,
	IFX_DEBUG_WARNING = 1U << 2,
	IFX_DEBUG_ERR = 1U << 1,
};

static int ifx_debug_mask = IFX_DEBUG_ERR;
module_param_named(
	debug_mask, ifx_debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP
);

#define IFX_INFO(fmt, arg...) \
	do { \
		if ((IFX_DEBUG_INFO) & ifx_debug_mask) \
			printk(KERN_INFO "ifxgsm: %s: " fmt "\n", __func__  , ## arg); \
	} while (0)
	
#define IFX_WAR(fmt, arg...) \
	do { \
		if ((IFX_DEBUG_WARNING) & ifx_debug_mask) \
			printk(KERN_WARNING "ifxgsm: %s: " fmt "\n", __func__  , ## arg); \
	} while (0)
	
	#define IFX_ERR(fmt, arg...) \
	do { \
		if ((IFX_DEBUG_ERR) & ifx_debug_mask) \
			printk(KERN_ERR "ifxgsm: %s: " fmt "\n", __func__  , ## arg); \
	} while (0)
	//ZTE_MODMEMCTL_HYJ_20100618, end, add the debug code
/*=========================================================================*/
/*                               TYPEDEFS & STRUCTS                                 */
/*=========================================================================*/

/*=========================================================================*/
/*                           DATA DECLARATIONS                             */
/*=========================================================================*/

/* GSM AT_SW pin */
static unsigned modemctl_config_atswitch = GPIO_CFG(MSM_GPIO_GSM_AT_SW, 0, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_2MA);
/* GSM <--> AP AUDIO_SW pin */
static unsigned modemctl_config_audioswitch = GPIO_CFG(MSM_GPIO_GSM_AT_SW, 0, GPIO_OUTPUT, GPIO_PULL_UP, GPIO_2MA);



/*=========================================================================*/
/*                           LOCAL FUNCITONS DECLARATIONS                         */
/*=========================================================================*/

static int ifxgsm_init(void);
//WANGXIAOWEI_MODEM_STATE_20100604_01, begin
//static int ifxgsm_get_sleepstate(void);
static int ifxgsm_get_gpiostate(unsigned int type);
//WANGXIAOWEI_MODEM_STATE_20100604_01, end
static int ifxgsm_get_workingstate(void);
static int ifxgsm_open_modem(void);
static int ifxgsm_reset_modem(void);
static int ifxgsm_close_modem(void);
static int ifxgsm_wake_modem(void);
static int ifxgsm_notify_modem(unsigned int type);
static int ifxgsm_switch_atcom(unsigned int on);
static int ifxgsm_get_respsignal(void);
static int ifxgsm_switch_audiocom(unsigned int on);


/*=========================================================================*/
/*                           LOCAL FUNCITONS REALIZATION                        */
/*=========================================================================*/

/**********************************************************************
* Function:       ifxgsm_poweron()
* Description:    To set GSM_KEY_ON OUT to high or low to power on the ifxgsm modem.
* Input:
* Output:
* Return:none
* Others:        
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/03/16     V1.0     wxw       create
**********************************************************************/
static int ifxgsm_poweron(void)
{
    IFX_INFO("++");

    gpio_set_value(MSM_GPIO_GSM_KEY_ON, GPIO_HIGH_LEVEL);
    gpio_set_value(MSM_GPIO_C2G_IRQ, GPIO_LOW_LEVEL);
    delay(1000);
    gpio_set_value(MSM_GPIO_GSM_KEY_ON, GPIO_LOW_LEVEL);
    
    IFX_INFO("--");
    return 0;
}

/**********************************************************************
* Function:       ifxgsm_shutdown()
* Description:    To set GSM_KEY_ON to high 1s and set low to shut down the ifxgsm modem.
* Input:
* Output:
* Return:none
* Others:        
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/03/16     V1.0     wxw       create
**********************************************************************/
static int ifxgsm_shutdown(void)
{
    IFX_INFO("++");

    gpio_set_value(MSM_GPIO_GSM_KEY_ON, GPIO_HIGH_LEVEL);
    delay(1000);
    gpio_set_value(MSM_GPIO_GSM_KEY_ON, GPIO_LOW_LEVEL);

    IFX_INFO("--");
    return 0;				    
}    

/**********************************************************************
* Function:       ifxgsm_get_gpio_status
* Description:    get the status of gpio selected
* Input:           gpio_id: the number of the GPIO
* Output:
* Return:none
* Others:        
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/03/16     V1.0     wxw       create
**********************************************************************/
static int ifxgsm_get_gpio_status(unsigned int gpio_id)
{
	return (gpio_get_value(gpio_id));
}

/**********************************************************************
* Function:       init the ifx GSM modem
* Description:    to init the ifx GSM modem
* Input:            void
* Output:         0  success, !0 fail
* Return:           none
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/03/15     V1.0      wxw       create
**********************************************************************/
static int ifxgsm_init(void)
{
    int rc = 0;
    //int ret = 0;
    IFX_INFO("++");

    /* GSM AT_SW pin */
   rc = gpio_tlmm_config(modemctl_config_atswitch ,GPIO_ENABLE);
    if (rc) {
        printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",__func__, MSM_GPIO_GSM_AT_SW, rc);
        return -EIO;
    }
    rc = gpio_request(MSM_GPIO_GSM_AT_SW, "modemctl");
    if(rc)
    {
        printk(KERN_ERR "gpio_request: %d failed!\n", MSM_GPIO_GSM_AT_SW);
        return rc;
    }

    /* GSM AUDIO_SW pin */
    rc = gpio_tlmm_config(modemctl_config_audioswitch ,GPIO_ENABLE);
    if (rc) {
        printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",__func__, MSM_GPIO_GSM_AUDIO_SW, rc);
        return -EIO;
    }
    rc = gpio_request(MSM_GPIO_GSM_AUDIO_SW, "modemctl");
    if(rc)
    {
        printk(KERN_ERR "gpio_request: %d failed!\n", MSM_GPIO_GSM_AT_SW);
        return rc;
    }

    //WANGXIAOWEI_ATSW_DOWN_001, begin
#ifdef MODEMCTL_FORRF_VERSION

    IFX_INFO("@@ set the ifx GSM AT_SW low to enable the uart port driectly connect to usb pins not AP.\n");
    gpio_set_value(MSM_GPIO_GSM_AT_SW, GPIO_LOW_LEVEL);
    
#else

    IFX_INFO("@@ set the ifx GSM AT_SW high to enable the uart port driectly connect to modem uart.\n");
    gpio_set_value(MSM_GPIO_GSM_AT_SW, GPIO_HIGH_LEVEL);
    
#endif
    //WANGXIAOWEI_ATSW_DOWN_001, end

    IFX_INFO("@@ call ifxgsm_open_modem to power on the ifx modem, before ifxgsm_open_modem.\n");
    rc = ifxgsm_open_modem();
    if( rc == MODEMCTL_OPENMODEM_MAX_TIME )
    {
       IFX_INFO("** Error call ifxgsm_open_modem to power on the ifx modem failed.\n");
       return -1;
    }
    IFX_INFO("@@ call ifxgsm_open_modem to power on the ifx modem, after ifxgsm_open_modem.\n");


    IFX_INFO("@@ set the ifx GSM AUDIO_SW low to make audio channel to AP.\n");
    gpio_set_value(MSM_GPIO_GSM_AUDIO_SW, GPIO_LOW_LEVEL);

    IFX_INFO("@@ set the ifx GSM MSM_GPIO_C2G_READY high to indicate AP wake.\n");
    gpio_set_value(MSM_GPIO_C2G_READY, GPIO_HIGH_LEVEL);

    IFX_INFO("--");
    return 0;
}

/**********************************************************************
* Function:       open the ifx GSM modem
* Description:    to open the ifx GSM modem
* Input:            void
* Output:         0  success, !0 fail
* Return:           none
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/03/15     V1.0      wxw       create
**********************************************************************/
static int ifxgsm_open_modem(void)
{
	int openCount = 0;
    
    //WANGXIAOWEI_PM_20100520_02,begin
    if(ifxgsm_get_workingstate() == MODEM_WORKING)
    {
         IFX_WAR(" ** : the MSM_GPIO_GSM_WORKING already HIGH. \n");
         //WANGXIAOWEI_MODEM_STATE_20100604_01, begin
         ifxgsm_reset_modem();
         //WANGXIAOWEI_MODEM_STATE_20100604_01, end
	   return 0;
    }
    //WANGXIAOWEI_PM_20100520_02,end
    
	do {
		ifxgsm_poweron();
		openCount++;
		IFX_WAR(" try %d times\n", openCount);
     delay(1000);
	} while ((ifxgsm_get_workingstate() == MODEM_UN_WORKING) &&
		 (openCount < MODEMCTL_OPENMODEM_MAX_TIME));
	
    if(ifxgsm_get_workingstate() == MODEM_WORKING)
    {
        //ZTE_MODMEMCTL_HYJ_20100720, , modify the gpio config for pm
        gpio_set_value(MSM_GPIO_GSM_AT_SW, GPIO_HIGH_LEVEL);
        return 0;
    }
     else
    {	
	   return openCount;
    }
    IFX_WAR(" after ifxgsm_open_modem, the workingstate = %d, have tried %d times\n",ifxgsm_get_workingstate(), openCount);
}

//WANGXIAOWEI_MODEM_STATE_20100604_01, begin
/**********************************************************************
* Function:       ifxgsm_get_gpiostate
* Description:    get the state of gpio 
* Input:            void
* Output:         gpio_state
* Return:           none
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/03/15     V1.0      wxw       create
* 2010/06/04     V1.0      wxw       create
**********************************************************************/
static int ifxgsm_get_gpiostate(unsigned int type)
{
    int gpiostate = 0;
    IFX_INFO("++ ");
    switch( type )
    {
        case MODEMCTL_GPIO_AUDIO_SW:        //audio_sw
            gpiostate = ifxgsm_get_gpio_status(MSM_GPIO_GSM_AUDIO_SW);
            break;
        case MODEMCTL_GPIO_AT_SW:            //at_sw
            gpiostate = ifxgsm_get_gpio_status(MSM_GPIO_GSM_AT_SW);
            break;
        case MODEMCTL_GPIO_BOARD_STATE:        //board state
            gpiostate = ifxgsm_get_gpio_status(MSM_GPIO_BOARD_TYPE);
            break;
    }
    
    IFX_INFO("-- ");
    return gpiostate;
}
//WANGXIAOWEI_MODEM_STATE_20100604_01, end

/**********************************************************************
* Function:       ifxgsm_get_workingstate
* Description:    get the state of ifx GSM modem working pin
* Input:            void
* Output:         the state of MSM_GPIO_GSM_WORKING
* Return:           none
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/03/15     V1.0      wxw       create
**********************************************************************/
static int ifxgsm_get_workingstate(void)
{
    IFX_INFO("++");
    IFX_INFO("the gsm working state = %d",ifxgsm_get_gpio_status(MSM_GPIO_GSM_WORKING) );
    IFX_INFO("--");
    return ifxgsm_get_gpio_status(MSM_GPIO_GSM_WORKING);
}


/**********************************************************************
* Function:       ifxgsm_get_respsignal
* Description:    get the state of ifx GSM modem G2C_READY pin for the Ready signal version 
* Input:            void
* Output:         the state of MSM_GPIO_GSM_WORKING
* Return:           none
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/04/06     V1.0      wxw       create
**********************************************************************/
static int ifxgsm_get_respsignal(void)
{
    int ret = 0;
    IFX_INFO("++");
    ret = ifxgsm_get_gpio_status(MSM_GPIO_G2C_READY);
    IFX_INFO("--");
    return ret;
}



/**********************************************************************
* Function:       ifxgsm_reset_modem
* Description:    to reset the ifx GSM modem
* Input:            void
* Output:         0  success, !0 fail
* Return:           none
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/03/15     V1.0      wxw       create
**********************************************************************/
static int ifxgsm_reset_modem(void)
{
    //HUANGYANJUN_IFX_MODEM_20100603_01 begin
    int openCount = 0;
    int ret = -1;
    //HUANGYANJUN_IFX_MODEM_20100603_01 end
    
    IFX_INFO("++");
    
    //WANGXIAOWEI_RESET_20100331, begin, there is a not gate on the line of gsm reset 

    /*
    gpio_set_value(MSM_GPIO_GSM_RESET, GPIO_LOW_LEVEL);
    mdelay(20);
    gpio_set_value(MSM_GPIO_GSM_RESET, GPIO_HIGH_LEVEL);
    */
    
    gpio_set_value(MSM_GPIO_GSM_RESET, GPIO_HIGH_LEVEL);
    delay(20);
    gpio_set_value(MSM_GPIO_GSM_RESET, GPIO_LOW_LEVEL);
    
    gpio_set_value(MSM_GPIO_GSM_KEY_ON, GPIO_HIGH_LEVEL);
    gpio_set_value(MSM_GPIO_C2G_IRQ, GPIO_HIGH_LEVEL);
    delay(1000);
    gpio_set_value(MSM_GPIO_GSM_KEY_ON, GPIO_LOW_LEVEL);

    //WANGXIAOWEI_RESET_20100331, begin, there is a not gate on the line of gsm reset 
     //WANGXIAOWEI_RESET_20100420, begin, to debug the working high after gsm reset 
    gpio_set_value(MSM_GPIO_C2G_IRQ, GPIO_LOW_LEVEL);
    //add delay(1000), to prevent APP reset the modem frequently.
    delay(1000);
    //WANGXIAOWEI_RESET_20100420, end, to debug the working high after gsm reset
     
    //HUANGYANJUN_IFX_MODEM_20100603_01 begin
 	  do {
		  openCount++;
		  IFX_INFO(" reset MODEM  try %d times\n", openCount);
		  //after 1s to detect the working state
      delay(500);
    } while ((ifxgsm_get_workingstate() == MODEM_UN_WORKING) &&
		  (openCount < MODEMCTL_OPENMODEM_MAX_TIME));

    if(ifxgsm_get_workingstate() == MODEM_WORKING) 
    {
        //ZTE_MODMEMCTL_HYJ_20100720, , modify the gpio config for pm
        gpio_set_value(MSM_GPIO_GSM_AT_SW, GPIO_HIGH_LEVEL);
        ret = 0;
    }
    else
        ret = openCount; 
    //HUANGYANJUN_IFX_MODEM_20100603_01 end    
   
    IFX_WAR(" after ifxgsm_reset_modem, the workingstate = %d, have tried %d times\n",ifxgsm_get_workingstate(), openCount);
    
    IFX_INFO("--"); 
    return ret;   

}


/**********************************************************************
* Function:       ifxgsm_close_modem
* Description:    to close the ifx GSM modem
* Input:            void
* Output:         0  success, !0 fail
* Return:           none
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/03/15     V1.0      wxw       create
**********************************************************************/
static int ifxgsm_close_modem(void)
{
	int openCount = 0;
      IFX_INFO("++");
      
      IFX_INFO("@@ before close the modem, set the MSM_GPIO_C2G_IRQ to low++\n ");
      gpio_set_value(MSM_GPIO_C2G_IRQ, GPIO_LOW_LEVEL);
      IFX_INFO("@@ before close the modem, set the MSM_GPIO_C2G_IRQ to low--\n ");      

      //WANGXIAOWEI_PM_20100520_02,begin
      if(ifxgsm_get_workingstate() != MODEM_WORKING)
      {
         IFX_WAR(" ** : the MSM_GPIO_GSM_WORKING already LOW. \n");
	   return 0;
      }
      //WANGXIAOWEI_PM_20100520_02,end
    
	do {
		ifxgsm_shutdown();
		openCount++;
		//after 1s to detect the working state
		//HUANGYANJUN_IFX_MODEM_20100603_01: delay time fix :1000ms ->2000ms 
        delay(2000);
	} while ((ifxgsm_get_workingstate() == MODEM_WORKING) &&
		 (openCount < MODEMCTL_OPENMODEM_MAX_TIME));

    if(ifxgsm_get_workingstate() == MODEM_UN_WORKING)
    {
        //ZTE_MODMEMCTL_HYJ_20100720, , modify the gpio config for pm
        gpio_set_value(MSM_GPIO_GSM_AT_SW, GPIO_LOW_LEVEL);
        return 0;
     }
    else
        return openCount;
    
    IFX_WAR(" after ifxgsm_close_modem, the workingstate = %d, have tried %d times\n",ifxgsm_get_workingstate(), openCount);
    IFX_INFO("--");
}


/**********************************************************************
* Function:       ifxgsm_wake_modem
* Description:    to init the ifx GSM modem
* Input:            void
* Output:         0  success, !0 fail
* Return:           none
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/03/15     V1.0      wxw       create
**********************************************************************/
static int ifxgsm_wake_modem(void)
{
    int ret = 0;
    IFX_INFO("++");
    //set c2g_irq high, the mdelay 25ms realized in modemctl.c
    gpio_set_value(MSM_GPIO_C2G_IRQ, GPIO_HIGH_LEVEL);
    
    //WANGXIAOWEI_25MS_20100416, begin
    /*
    //WANGXIAOWEI_READY_20100409, begin
    ret = gpio_get_value(MSM_GPIO_G2C_READY);
    //WANGXIAOWEI_READY_20100409, end
    */
    //WANGXIAOWEI_25MS_20100416, end
    
    IFX_INFO("--");
    return ret;
}


/**********************************************************************
* Function:       ifxgsm_notify_modem
* Description:    to nofify the ifx GSM modem
* Input:            unsigned int type: the types of notifier form AP
* Output:         0  success, !0 fail
* Return:           none
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/03/15     V1.0      wxw       create
**********************************************************************/
static int ifxgsm_notify_modem(unsigned int type)
{
    IFX_INFO("++");
    
    switch( type)
    {
        case MODEMCTL_NOTIFY_SENDDATA_READY:        //send data read notify
            if(ifxgsm_get_gpio_status(MSM_GPIO_C2G_IRQ) == GPIO_HIGH_LEVEL)
            {
                IFX_INFO(" here the MSM_GPIO_C2G_IRQ is high, so pull it down\n");
                gpio_set_value(MSM_GPIO_C2G_IRQ, GPIO_LOW_LEVEL);
            }
            else
            {
                IFX_ERR(" ** Error, here the MSM_GPIO_C2G_IRQ is already LOW \n");
                return -1;
            }
            break;
            
        case MODEMCTL_NOTIFY_APSUSPEND:         //AP suspend notify
            if(ifxgsm_get_gpio_status(MSM_GPIO_C2G_READY) == GPIO_HIGH_LEVEL)
            {
                IFX_INFO(" here the MSM_GPIO_C2G_READY is high, so pul it down\n");
                gpio_set_value(MSM_GPIO_C2G_READY, GPIO_LOW_LEVEL);
            }
            else
            {
                IFX_ERR(" **Error, here the MSM_GPIO_C2G_READY is already LOW\n");
                return 0;
            }

            //WANGXIAOWEI_PM_20100520_01, begin
            //add a protection mechanism make the MSM_GPIO_C2G_IRQ must LOW when suspend
            if(ifxgsm_get_gpio_status(MSM_GPIO_C2G_IRQ) == GPIO_HIGH_LEVEL)
            {
                IFX_INFO(" NOTE: here the MSM_GPIO_C2G_IRQ is high, so pul it down\n");
                gpio_set_value(MSM_GPIO_C2G_IRQ, GPIO_LOW_LEVEL);
            }
            //WANGXIAOWEI_PM_20100520_01, end
            
            break;
            
        case MODEMCTL_NOTIFY_APRESUME:        //AP resume notify
            if(ifxgsm_get_gpio_status(MSM_GPIO_C2G_READY) == GPIO_LOW_LEVEL)
            {
                IFX_INFO(" here the MSM_GPIO_C2G_READY is LOW, so pull it up\n");
                gpio_set_value(MSM_GPIO_C2G_READY, GPIO_HIGH_LEVEL);
            }
            else
            {
                IFX_ERR(" **Error, here the MSM_GPIO_C2G_READY is already HIGH \n");
                return 0;
            }
            break;

    }
    
    IFX_INFO("--");
    return 0;
}


/**********************************************************************
* Function:       ifxgsm_switch_atcom
* Description:    to init the ifx GSM modem
* Input:            void
* Output:         0  success, !0 fail
* Return:           none
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/03/15     V1.0      wxw       create
**********************************************************************/
static int ifxgsm_switch_atcom(unsigned int on)
{
    IFX_INFO("++");
    
    switch( on )
    {
        case MODEMCTL_ATSW_DISCONNECT:        
            IFX_INFO("@@ set the ifx GSM AT_SW low to enable the uart port driectly connect to usb pins not AP.\n");
            gpio_set_value(MSM_GPIO_GSM_AT_SW, GPIO_LOW_LEVEL);
            break;
            
        case MODEMCTL_ATSW_CONNECT:       
            IFX_INFO("@@ set the ifx GSM AT_SW high to enable the uart port driectly connect to modem uart.\n");
            gpio_set_value(MSM_GPIO_GSM_AT_SW, GPIO_HIGH_LEVEL);
            break;
            
        default:
            IFX_ERR("@@ don't support the on cmd --- %d, please check your input cmd.\n", on);
            break;
    }
    
    IFX_INFO("--");
    return 0;
}

/**********************************************************************
* Function:       ifxgsm_switch_audiocom
* Description:    to switch the audio channel to AP or IFX Modem
* Input:            void
* Output:         0  success, !0 fail
* Return:           none
* Others:           
* Modify Date    Version    Author    Modification
* ----------------------------------------------------
* 2010/03/15     V1.0      wxw       create
**********************************************************************/
static int ifxgsm_switch_audiocom(unsigned int on)
{
    IFX_INFO("++");
    
    switch( on )
    {
        case MODEMCTL_AUDIOSW_AUDIO2AP :        
            IFX_INFO("@@ set the GSM AUDIO_SW low to make audio channel to AP.\n");
            gpio_set_value(MSM_GPIO_GSM_AUDIO_SW, GPIO_LOW_LEVEL);
            break;
            
        case MODEMCTL_AUDIOSW_AUDIO2MODEM:       
            IFX_INFO("@@ set the GSM AUDIO_SW high to make audio channel to IFX modem.\n");
            gpio_set_value(MSM_GPIO_GSM_AUDIO_SW, GPIO_HIGH_LEVEL);
            break;
            
        default:
            IFX_INFO("@@ don't support the on cmd --- %d, please check your input cmd.\n", on);
            break;
    }
    
    IFX_INFO("--");
    return 0;
}



/*
 * modemctl_ops realization for the IFX GSM Modem
 */
struct modemctl_ops modem_ifxgsm_ops = {
	.init = ifxgsm_init,
        //WANGXIAOWEI_MODEM_STATE_20100604_01, begin
	//.get_sleepstate = ifxgsm_get_sleepstate,
	.get_gpiostate = ifxgsm_get_gpiostate,
	//WANGXIAOWEI_MODEM_STATE_20100604_01, end
	.get_workingstate = ifxgsm_get_workingstate,
	.open_modem = ifxgsm_open_modem,
	.reset_modem = ifxgsm_reset_modem,
	.close_modem = ifxgsm_close_modem,
	.wake_modem = ifxgsm_wake_modem,
	.notify_modem = ifxgsm_notify_modem,
	.switch_atcom = ifxgsm_switch_atcom,
	.get_respsignal = ifxgsm_get_respsignal,
	.switch_audiocom = ifxgsm_switch_audiocom,
};

//here make the ifx gsm initialization
module_init(ifxgsm_init);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("wangxiaowei");
EXPORT_SYMBOL(modem_ifxgsm_ops);

