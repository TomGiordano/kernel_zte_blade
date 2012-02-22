/******************************************************************
*File Name: 	ONA3301.c 	                                           *
*Description:	Linux device driver for ONA ambient light and    *
*			proximity sensors.                                          *                                
*******************************************************************
Geschichte:	                                                                        
Wenn               Wer          Was                                                                        Tag
2011-11-15         wanglg     create
                                   
******************************************************************/
// includes
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/hwmon.h>
#include <linux/timer.h>
#include <asm/uaccess.h>
#include <asm/errno.h>
#include <asm/delay.h>
#include <linux/irq.h>
#include <asm/gpio.h>
#include <linux/i2c/PS_ALS_common.h> //copy to linux/i2c
#include <linux/input.h>
#include <linux/miscdevice.h>


#define ONA_INT_GPIO 42
#define ONA_TAG        "[ONA]"

// device name/id/address/counts
#define ONA_DEVICE_NAME		"ona3301"
#define ONA_DEVICE_ID                "ONA"
#define ONA_ID_NAME_SIZE		10

#define ONA_MAX_NUM_DEVICES		3

/*************************************************/
// Register Definitions
#define NAOCHIP_REG_NUM 24
#define NOACHIP_VENDOR      0x0001
/* PART_ID */
#define NOACHIP_PART		0x90  /* needs to be set to the correct part id */
#define NOACHIP_PART_MASK	0xf0   
#define NOACHIP_REV_MASK	0x0f
#define NOACHIP_REV_SHIFT	0
#define NOACHIP_REV_0		0x00

/* Operating modes for both */
#define NOACHIP_STANDBY		0x00
#define NOACHIP_PS_ONESHOT	0x01
#define NOACHIP_PS_REPEAT	0x02

#define NOACHIP_TRIG_MEAS	0x01


/* Interrupt control */
#define NOACHIP_INT_LEDS_INT_HI	(1 << 1)
#define NOACHIP_INT_LEDS_INT_LO	(1 << 0)
#define NOACHIP_INT_ALS_INT_HI	(1 << 2) 
#define NOACHIP_INT_ALS_INT_LO  (1 << 3)
#define NOACHIP_INT_EXTERNAL    (1 << 4)

#define NOACHIP_DISABLE		0
#define NOACHIP_ENABLE		3

enum noa_regs {
	 NOACHIP_PART_ID,
	 NOACHIP_RESET,
	 NOACHIP_INT_CONFIG,
	 NOACHIP_PS_LED_CURRENT,
	 NOACHIP_PS_FILTER_CONFIG,
	 NOACHIP_PS_CONFIG,
	 NOACHIP_PS_TH_UP_MSB,
	 NOACHIP_PS_TH_UP_LSB,
	 NOACHIP_PS_TH_LO_MSB,
	 NOACHIP_PS_TH_LO_LSB,
	 NOACHIP_PS_MEAS_INTERVAL,
	 NOACHIP_PS_CONTROL,
	 NOACHIP_ALS_TH_UP_MSB,
	 NOACHIP_ALS_TH_UP_LSB,
	 NOACHIP_ALS_TH_LO_MSB,
	 NOACHIP_ALS_TH_LO_LSB,
	 NOACHIP_ALS_CONFIG,
	 NOACHIP_ALS_MEAS_INTERVAL,
	 NOACHIP_ALS_CONTROL,
	 NOACHIP_INTERRUPT,
	 NOACHIP_PS_DATA_MSB,
	 NOACHIP_PS_DATA_LSB,
	 NOACHIP_ALS_DATA_MSB,
	 NOACHIP_ALS_DATA_LSB
};

static const char noa_regmap[]={
	 [NOACHIP_PART_ID] = 0x00, /* Part number and revision ID */
	 [NOACHIP_RESET] = 0x01, /* Reset */
	 [NOACHIP_INT_CONFIG] = 0x02, /* Interrupt pin function control settings */
	 [NOACHIP_PS_LED_CURRENT] = 0x0F, /* Set PS LED Current */
	 [NOACHIP_PS_FILTER_CONFIG] = 0x14, /* Sets N and M setting for PS fileter */
	 [NOACHIP_PS_CONFIG] = 0x15, /* PS Range and intergration time */
	 [NOACHIP_PS_TH_UP_MSB] = 0x10, /* PS interrupt upper threshold, MSB */
	 [NOACHIP_PS_TH_UP_LSB] = 0x11, /* PS interrupt upper threshold, LSB */
	 [NOACHIP_PS_TH_LO_MSB] = 0x12, /* PS interrupt lower threshold, MSB */
	 [NOACHIP_PS_TH_LO_LSB] = 0x13, /* PS interrupt lower threshold, LSB */
	 [NOACHIP_PS_MEAS_INTERVAL] = 0x16, /* PS meas. interval at stand alone mode */
	 [NOACHIP_PS_CONTROL] = 0x17, /* PS operation mode control 0x02 = PS_Repeat, 0x01=PS_OneShot */
	 [NOACHIP_ALS_TH_UP_MSB] = 0x20, /* ALS upper threshold MSB */
	 [NOACHIP_ALS_TH_UP_LSB] = 0x21, /* ALS upper threshold LSB */
	 [NOACHIP_ALS_TH_LO_MSB] = 0x22, /* ALS lower threshold MSB */
	 [NOACHIP_ALS_TH_LO_LSB] = 0x23, /* ALS lower threshold LSB */
	 [NOACHIP_ALS_CONFIG] = 0x25, /* ALS intergration time */
	 [NOACHIP_ALS_MEAS_INTERVAL] = 0x26, /* ALS meas. interval at stand alone mode */
	 [NOACHIP_ALS_CONTROL] = 0x27, /* ALS operation mode control */
	 [NOACHIP_INTERRUPT] = 0x40, /* Interrupt status */
	 [NOACHIP_PS_DATA_MSB] = 0x41, /* PS DATA MSB */
	 [NOACHIP_PS_DATA_LSB] = 0x42, /* PS DATA LSB */
	 [NOACHIP_ALS_DATA_MSB] = 0x43, /* ALS DATA high byte */
	 [NOACHIP_ALS_DATA_LSB] = 0x44 /* ALS DATA low byte */
};

/* led_max_curr is a safetylimit for IR leds */
#define NOA3402_LED_5mA   0
#define NOA3402_LED_10mA  1
#define NOA3402_LED_15mA  2
#define NOA3402_LED_20mA  3
#define NOA3402_LED_25mA  4
#define NOA3402_LED_30mA  5
#define NOA3402_LED_35mA  6
#define NOA3402_LED_40mA  7
#define NOA3402_LED_45mA  8
#define NOA3402_LED_50mA  9
#define NOA3402_LED_55mA  10
#define NOA3402_LED_60mA  11
#define NOA3402_LED_65mA  12
#define NOA3402_LED_70mA  13
#define NOA3402_LED_75mA  14
#define NOA3402_LED_80mA  15
#define NOA3402_LED_85mA  16
#define NOA3402_LED_90mA  17
#define NOA3402_LED_95mA  18
#define NOA3402_LED_100mA 19
#define NOA3402_LED_105mA 20
#define NOA3402_LED_110mA 21
#define NOA3402_LED_115mA 22
#define NOA3402_LED_120mA 23
#define NOA3402_LED_125mA 24
#define NOA3402_LED_130mA 25
#define NOA3402_LED_135mA 26
#define NOA3402_LED_140mA 27
#define NOA3402_LED_145mA 28
#define NOA3402_LED_150mA 29
#define NOA3402_LED_155mA 30
#define NOA3402_LED_160mA 31
//#define NOA3402_INT_CONFIG    0x01 // interrupt pin active till cleared, active high
#define NOA3402_INT_CONFIG    0x00 // interrupt pin active till cleared, active low
#define NOA3402_PWM_SENS      0x00 // Standard: 0x00=standard, 0x01=1/2, 0x10=1/4, 0x11=1/8
#define NOA3402_PWM_RES       0x01 // 8bit: 0x00=7bit, 0x01=7bit, 0x10=9bit, 0x11=10bit 
#define NOA3402_PWM_ENABLE    0x00 // not enabled.
#define NOA3402_PWM_POLARITY  0x01 // Positive: 0x00=neg, 0x01=pos
#define NOA3402_PWM_TYPE      0x01 // log: 0x00=lin,0x01=log
#define NOA3402_ALS_HYST_TRIGGER (1<<4) // Upper: 0x00=Lowwer, 0x01=Upper
#define NOA3402_ALS_HYST_ENABLE  0x00 // Disabled 0x00=Disabled, 0x01=Enabled
#define NOA3402_ALS_INTEGRATION_TIME  0X04 //100ms: 0x000=6.25ms, 0x001=12.5ms,0x010=25ms,0x011=50ms,0x100=200ms,0x110=400,0x111=800ms
#define NOA3402_ALS_THRES_LO	0
#define NOA3402_ALS_THRES_UP	65535
#define NOA3402_ALS_CONTROL   0x02  // Repeat Mode 0x01=One Shot
#define NOA3402_ALS_INTERVAL  500 // Valid values 0 - 3150 in steps of 50.

#define NOA3402_PS_FILTER_M 1 // 1 - 15
#define NOA3402_PS_FILTER_N (1<<4) // 1 - 15
#define NOA3402_PS_INTEGRATION_TIME 0x20 // 300us: 0x00=75us, 0x01=150us, 0x10=300us, 0x11=600us
#define NOA3402_PS_HYST_TRIGGER (1<<4) // Upper: 0x00=Lowwer, 0x01=Upper
#define NOA3402_PS_HYST_ENABLE  0x00 // Disabled 0x00=Disabled, 0x01=Enabled
#define NOA3402_PS_THRES_LO	1200
#define NOA3402_PS_THRES_UP	1500
#define NOA3402_PS_INTERVAL  100 // 100ms: Valid values 0 - 3150 in steps of 50.
#define NOA3402_PS_LED_CURRENT 0x10 // 85ma  NOA3402_PS_LED_CURRENT * 5 + 5
#define NOA3402_PS_MODE NOACHIP_PS_REPEAT 

/*************************************************/

// lux constants
#define	ONA_MAX_LUX			65535000
//#define ONA_SCALE_MILLILUX		3
#define ONA_FILTER_DEPTH		3
#define THRES_LO_TO_HI_RATIO  4/5

// forward declarations
static int ONA_probe(struct i2c_client *clientp, const struct i2c_device_id *idp);
static int ONA_remove(struct i2c_client *client);
static int ONA_open(struct inode *inode, struct file *file);
static int ONA_release(struct inode *inode, struct file *file);
static int ONA_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);
static int ONA_read(struct file *file, char *buf, size_t count, loff_t *ppos);
static int ONA_write(struct file *file, const char *buf, size_t count, loff_t *ppos);
static loff_t ONA_llseek(struct file *file, loff_t offset, int orig);
static int ONA_get_lux(void);
static int ONA_device_name(unsigned char *bufp, char **device_name);
static int ONA_prox_poll(struct PS_ALS_prox_info *prxp);

static void do_ONA_work(struct work_struct *w);
static void ONA_report_value(int mask);
static int calc_distance(int value);
static int enable_light_and_proximity(int mask);	
static int light_on=0;  
static int prox_on = 0;

enum ONA_chip_type {
	ONA3301 = 0,
	ONA3302,	
};

struct alsprox_data {
	struct input_dev *input_dev;
};

static struct alsprox_data *light;
static struct alsprox_data *proximity;
// first device number
static dev_t ONA_dev_number;

// class structure for this device
struct class *ONA_class;

// module device table
static struct i2c_device_id ONA_idtable[] = {
        {ONA_DEVICE_ID, 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, ONA_idtable);


// client and device
struct i2c_client *ONA_my_clientp;
struct i2c_client *ONA_bad_clientp[ONA_MAX_NUM_DEVICES];
//static int num_bad = 0;
//static int device_found = 0;

// driver definition
static struct i2c_driver ONA_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "ona3301",
	 },
	.id_table = ONA_idtable,
	.probe = ONA_probe,
	.remove = __devexit_p(ONA_remove),
};


struct ONA_intr_data {
    int int_gpio;
    int irq;
};

// per-device data
struct ONA_data {
	struct i2c_client *client;
	struct cdev cdev;
	unsigned int addr;
	char ONA_id;
	char ONA_name[ONA_ID_NAME_SIZE];
	struct semaphore update_lock;
	char valid;
	unsigned long last_updated;
    	struct ONA_intr_data *pdata;
	struct work_struct  ONA_work;
	enum ONA_chip_type chip_type;
	struct mutex proximity_calibrating;
} *ONA_datap;

static struct ONA_intr_data ONA_irq= {
    .int_gpio = ONA_INT_GPIO,
    .irq = MSM_GPIO_TO_INT(ONA_INT_GPIO),
};


// file operations
static struct file_operations ONA_fops = {
	.owner = THIS_MODULE,
	.open = ONA_open,
	.release = ONA_release,
	.read = ONA_read,
	.write = ONA_write,
	.llseek = ONA_llseek,
	.ioctl = ONA_ioctl,
};

// device configuration
struct PS_ALS_cfg *ONA_cfgp;
static u32 calibrate_target_param = 300000;
static u16 als_time_param = 27;
static u16 scale_factor_param = 1;
static u8 filter_history_param = 3;
static u8 filter_count_param = 1;
static u8 gain_param = 1;

#if defined(CONFIG_MACH_BLADE)
static u16 gain_trim_param = 25;
#elif defined(CONFIG_MACH_RACER2)
static u16 gain_trim_param = 50;
#elif defined(CONFIG_MACH_SKATE)
static u16 gain_trim_param = 240;
#elif defined(CONFIG_MACH_ROAMER)
static u16 gain_trim_param = 5;
#elif defined(CONFIG_MACH_BLADE2)
static u16 gain_trim_param = 100;
#else
static u16 gain_trim_param = 25; //this value is set according to specific device
#endif

static u16 prox_threshold_hi_param = 1500; 
static u16 prox_threshold_lo_param = 1200;
static u8 prox_int_time_param = 0xF6;
static u8 prox_adc_time_param = 0xFF;
static u8 prox_wait_time_param = 0xFF;
static u8 prox_intr_filter_param = 0x00;
static u8 prox_config_param = 0x00;

static u8 prox_pulse_cnt_param = 0x10;
static u8 prox_gain_param = 0x20;

// device reg init values
//u8 ONA_triton_reg_init[16] = {0x00,0xFF,0XFF,0XFF,0X00,0X00,0XFF,0XFF,0X00,0X00,0XFF,0XFF,0X00,0X00,0X00,0X00};

//
static u16 als_intr_threshold_hi_param = 0;
static u16 als_intr_threshold_lo_param = 0;
int ONA_g_nlux = 0;

		

// prox info
struct PS_ALS_prox_info ONA_prox_cal_info[20];
struct PS_ALS_prox_info ONA_prox_cur_info;
struct PS_ALS_prox_info *ONA_prox_cur_infop = &ONA_prox_cur_info;


static int device_released = 0;
static u16 sat_als = 0;
static u16 sat_prox = 0;

// lux time scale
struct time_scale_factor  {
	u16	numerator;
	u16	denominator;
	u16	saturation;
};
struct time_scale_factor ONA_TritonTime = {1, 0, 0};
struct time_scale_factor *ONA_lux_timep = &ONA_TritonTime;

// gain table
u8 ONA_triton_gain_table[] = {1, 8, 16, 120};

// lux data
struct lux_data {
	u16	ratio;
	u16	clear;
	u16	ir;
};
struct lux_data ONA_TritonFN_lux_data[] = {
        { 9830,  8320,  15360 },
        { 12452, 10554, 22797 },
        { 14746, 6234,  11430 },
        { 17695, 3968,  6400  },
        { 0,     0,     0     }
};
struct lux_data *ONA_lux_tablep = ONA_TritonFN_lux_data;
static int lux_history[ONA_FILTER_DEPTH] = {-ENODATA, -ENODATA, -ENODATA};

//prox data
struct ONA_prox_data {
	u16	ratio;
	u16	hi;
	u16	lo;
};
struct ONA_prox_data ONA_prox_data[] = {
        { 1,  22,  20 },
        { 3, 20, 16 },
        { 6, 18, 14 },
        { 10, 16,  16 },      
        { 0,  0,   0 }
};
struct ONA_prox_data *ONA_prox_tablep = ONA_prox_data;


/* ----------------------*
* config gpio for intr utility        *
*-----------------------*/
int ONA_config_int_gpio(int int_gpio)
{
    int rc=0;
    uint32_t  gpio_config_data = GPIO_CFG(int_gpio,  0, GPIO_INPUT, GPIO_PULL_UP, GPIO_2MA);

    rc = gpio_request(int_gpio, "gpio_sensor");
    if (rc) {
        printk(ONA_TAG "%s: gpio_request(%#x)=%d\n",
                __func__, int_gpio, rc);
        return rc;
    }

    rc = gpio_tlmm_config(gpio_config_data, GPIO_ENABLE);
    if (rc) {
        printk(ONA_TAG "%s: gpio_tlmm_config(%#x)=%d\n",
                __func__, gpio_config_data, rc);
        return rc;
    }

    mdelay(1);

    rc = gpio_direction_input(int_gpio);
    if (rc) {
        printk(ONA_TAG "%s: gpio_direction_input(%#x)=%d\n",
                __func__, int_gpio, rc);
        return rc;
    }

    return 0;
}

/* ----------------------*
* ONA interrupt function         *
*-----------------------*/
static irqreturn_t ONA_interrupt(int irq, void *data)
{
    //printk(ONA_TAG "ONA_interrupt\n");	
    disable_irq_nosync(ONA_datap->pdata->irq);
    schedule_work(&ONA_datap->ONA_work);
    
    return IRQ_HANDLED;
}

static void do_ONA_work(struct work_struct *w)
{

    int ret =0;	
    //int prx_hi, prx_lo;
    u16 status = 0;	
    status = i2c_smbus_read_byte_data(ONA_datap->client,noa_regmap[NOACHIP_INTERRUPT]);//add error ctl
    //printk(ONA_TAG "ONA_interrupt status = %x\n",status);	
    if(status<0)
	goto read_reg_fail;
   
    //als interrupt
    if(((status & (NOACHIP_INT_ALS_INT_HI|NOACHIP_INT_ALS_INT_LO)) != 0)/*&&((status & NOACHIP_INT_EXTERNAL) != 0)*/)
    {
	 printk(ONA_TAG "ONA_interrupt als change status = %d\n",status);	
	 ONA_g_nlux = ONA_get_lux();
     ONA_report_value(0);
    }
	
    //prox interrupt
	if((status & (NOACHIP_INT_LEDS_INT_HI|NOACHIP_INT_LEDS_INT_LO))!=0)
    {
	printk(ONA_TAG "ONA_interrupt prox change status = %d\n",status);	
	if((ret = ONA_prox_poll(ONA_prox_cur_infop))<0)
	{
		printk(KERN_CRIT "ONA: get prox poll  failed in  ONA interrupt()\n");  	
	    goto read_reg_fail;
	}
	//printk(ONA_TAG "prox_data = %d  prox_threshold_hi = %d  prox_threshold_lo = %d \n",ONA_prox_cur_infop->prox_data,ONA_cfgp->prox_threshold_hi,ONA_cfgp->prox_threshold_lo);		
	if(ONA_prox_cur_infop->prox_data > ONA_cfgp->prox_threshold_hi)       
	 {           
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_PS_TH_UP_MSB], (ONA_prox_cur_infop->prox_data >> 8)&0xFF))) < 0) {        	        
			pr_crit(ONA_TAG "i2c_smbus_write_byte() to ONA_TRITON_PRX_MAXTHRESHLO\n");                	
			}           
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_PS_TH_UP_LSB], (ONA_prox_cur_infop->prox_data)&0xFF))) < 0) {        	        
			pr_crit(ONA_TAG "i2c_smbus_write_byte() to ONA_TRITON_PRX_MAXTHRESHLO\n");                	
			}           
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_PS_TH_LO_MSB], (ONA_cfgp->prox_threshold_lo >> 8)&0xFF))) < 0) {        	        
			pr_crit(ONA_TAG "i2c_smbus_write_byte() to ONA_TRITON_PRX_MAXTHRESHLO\n");                	
			}           
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_PS_TH_LO_LSB], (ONA_cfgp->prox_threshold_lo)&0xFF))) < 0) {        	        
			pr_crit(ONA_TAG "i2c_smbus_write_byte() to ONA_TRITON_PRX_MAXTHRESHLO\n");                	
			}           

		ONA_prox_cur_infop->prox_event = 1;           
	}        
	else if(ONA_prox_cur_infop->prox_data < ONA_cfgp->prox_threshold_lo)        
	{            
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_PS_TH_UP_MSB], (ONA_cfgp->prox_threshold_hi >> 8)&0xFF))) < 0) {        	        
			pr_crit(ONA_TAG "i2c_smbus_write_byte() to ONA_TRITON_PRX_MAXTHRESHLO\n");                	
			}           
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_PS_TH_UP_LSB], (ONA_cfgp->prox_threshold_hi)&0xFF))) < 0) {        	        
			pr_crit(ONA_TAG "i2c_smbus_write_byte() to ONA_TRITON_PRX_MAXTHRESHLO\n");                	
			}           
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_PS_TH_LO_MSB], (ONA_prox_cur_infop->prox_data >> 8)&0xFF))) < 0) {        	        
			pr_crit(ONA_TAG "i2c_smbus_write_byte() to ONA_TRITON_PRX_MAXTHRESHLO\n");                	
			}           
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_PS_TH_LO_LSB], (ONA_prox_cur_infop->prox_data)&0xFF))) < 0) {        	        
			pr_crit(ONA_TAG "i2c_smbus_write_byte() to ONA_TRITON_PRX_MAXTHRESHLO\n");                	
			}           
		ONA_prox_cur_infop->prox_event = 0;             
		//pr_crit(ONA_TAG "screen on:ONA_prox_cur_infop->prox_data=%d\n",ONA_prox_cur_infop->prox_data);                     
	}   
	ONA_report_value(1);
    }
	
read_reg_fail:
    enable_irq(ONA_datap->pdata->irq);
    return;
}	

static int noa3402_ps_intervals_to_reg_value(int interval)
{
	int i;
	int ret = 0;
	int calc_interval;

	for (i = 0; i < 256; i++) {
		calc_interval = 5 * i;
		if (calc_interval == interval) {
			ret = i;
			break;
		}
	}
	if (i == 256)
		ret = -EINVAL;
	return ret;
}

static int noa3402_als_intervals_to_reg_value(int interval)
{
	int i;
	int ret = 0;
	int calc_interval;

	for (i = 0; i < 64; i++) {
		calc_interval = 50 * i;
		if (calc_interval == interval) {
			ret = i;
			break;
		}
	}
	if (i == 64)
		ret = -EINVAL;
	return ret;
}
		
/*calc_distance, using prox value*/
static int calc_distance(int value)
{
	int temp=0;
	if(ONA_prox_cur_infop->prox_event == 1)
		temp=0;
	else if(ONA_prox_cur_infop->prox_event == 0)
		temp=5;
	return temp;
}

/*report als and prox data to input sub system, 
for data poll utility in hardware\alsprox.c
*/
static void ONA_report_value(int mask)
{
	struct PS_ALS_prox_info *val = ONA_prox_cur_infop;
	int lux_val=ONA_g_nlux;
	int  dist;
	lux_val=(lux_val<=1)? lux_val:lux_val*10;	

	if (mask==0) {
		input_report_abs(light->input_dev, ABS_MISC, lux_val);
//[sensor wlg 20110729]ALS thereshold modify add log
 		printk(KERN_CRIT "ONA: als_interrupt lux_val(%d)=ONA_g_nlux(%d) * 9 \n", lux_val, ONA_g_nlux);
		input_sync(light->input_dev);
	}

	if (mask==1) {
		dist=calc_distance(val->prox_data);
		//input_report_als(alsprox->input_dev, ALS_PROX, val->prox_clear);
		input_report_abs(proximity->input_dev, ABS_DISTANCE, dist);
		//input_report_als(alsprox->input_dev, ALS_PROX, val->prox_event);
		printk(KERN_CRIT "ONA: prox_interrupt =%d, distance=%d\n",  val->prox_data,dist);
		input_sync(proximity->input_dev);
	}

	//input_sync(alsprox->input_dev);	
	//enable_irq(ONA_datap->pdata->irq);
}



/* ------------*
* driver init        *
*------------*/
static int __init ONA_init(void) {
	int ret = 0;
	//struct i2c_adapter *my_adap;
	printk(KERN_ERR "ONA: comes into ONA_init\n");
	if ((ret = (alloc_chrdev_region(&ONA_dev_number, 0, ONA_MAX_NUM_DEVICES, ONA_DEVICE_NAME))) < 0) {
		printk(KERN_ERR "ONA: alloc_chrdev_region() failed in ONA_init()\n");
                return (ret);
	}
        ONA_class = class_create(THIS_MODULE, ONA_DEVICE_NAME);
        ONA_datap = kmalloc(sizeof(struct ONA_data), GFP_KERNEL);
        if (!ONA_datap) {
		printk(KERN_ERR "ONA: kmalloc for struct ONA_data failed in ONA_init()\n");
                return -ENOMEM;
	}
        memset(ONA_datap, 0, sizeof(struct ONA_data));
        cdev_init(&ONA_datap->cdev, &ONA_fops);
        ONA_datap->cdev.owner = THIS_MODULE;
        if ((ret = (cdev_add(&ONA_datap->cdev, ONA_dev_number, 1))) < 0) {
		printk(KERN_ERR "ONA: cdev_add() failed in ONA_init()\n");
                return (ret);
	}
	//device_create(ONA_class, NULL, MKDEV(MAJOR(ONA_dev_number), 0), &ONA_driver ,"ONA");
        ret = i2c_add_driver(&ONA_driver);
	if(ret){
		printk(KERN_ERR "ONA: i2c_add_driver() failed in ONA_init(),%d\n",ret);
                return (ret);
	}
    	//pr_crit(ONA_TAG "%s:%d\n",__func__,ret);
        return (ret);
}



// driver exit
static void __exit ONA_exit(void) {
/*	if (ONA_my_clientp)
		i2c_unregister_device(ONA_my_clientp);
	*/
        i2c_del_driver(&ONA_driver);
        unregister_chrdev_region(ONA_dev_number, ONA_MAX_NUM_DEVICES);
	device_destroy(ONA_class, MKDEV(MAJOR(ONA_dev_number), 0));
	cdev_del(&ONA_datap->cdev);
	class_destroy(ONA_class);
	mutex_destroy(&ONA_datap->proximity_calibrating);
        kfree(ONA_datap);
}


//***************************************************
/*static struct file_operations ONA_device_fops = {
	.owner = THIS_MODULE,
	.open = ONA_open,
	.release = ONA_release,
	.ioctl = ONA_ioctl,
};


static struct miscdevice ONA_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "ONA",
	.fops = &ONA_device_fops,
};
*/

// client probe
static int ONA_probe(struct i2c_client *clientp, const struct i2c_device_id *idp) {
	int ret = 0;
	int i = 0;
	unsigned char buf[NAOCHIP_REG_NUM];
	char *device_name;

//	if (device_found)
//		return -ENODEV;
	if (!i2c_check_functionality(clientp->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		printk(KERN_ERR "ONA: ONA_probe() - i2c smbus byte data functions unsupported\n");
		return -EOPNOTSUPP;
		}
    if (!i2c_check_functionality(clientp->adapter, I2C_FUNC_SMBUS_WORD_DATA)) {
		printk(KERN_ERR "ONA: ONA_probe() - i2c smbus word data functions unsupported\n");
        }
    if (!i2c_check_functionality(clientp->adapter, I2C_FUNC_SMBUS_BLOCK_DATA)) {
		printk(KERN_ERR "ONA: ONA_probe() - i2c smbus block data functions unsupported\n");
        }
	ONA_datap->client = clientp;
	i2c_set_clientdata(clientp, ONA_datap);
	
    for(i = 0; i < NAOCHIP_REG_NUM; i++) {
		buf[i] = i2c_smbus_read_byte_data(ONA_datap->client, noa_regmap[i]);
		if(buf[i] < 0)
		{
			printk(KERN_ERR "ONA: i2c_smbus_read_byte_data(%d) failed in ONA_probe()\n",noa_regmap[i]);
		}
		else
		{
			printk(KERN_ERR "ONA: i2c_smbus_read_byte_data(%d) = %d in ONA_probe()\n", noa_regmap[i], buf[i]);
	}	
	}	
	//check device ID "tritonFN"
	if ((ret = ONA_device_name(buf, &device_name)) == 0) {
		printk(KERN_ERR "ONA: chip id that was read found mismatched by ONA_device_name(), in ONA_probe()\n");
 		return -ENODEV;
        }
	if (strcmp(device_name, ONA_DEVICE_ID)) {
		printk(KERN_ERR "ONA: chip id that was read does not match expected id in ONA_probe()\n");
		return -ENODEV;
        }
	else{
		printk(KERN_ERR "ONA: chip id of %s that was read matches expected id in ONA_probe()\n", device_name);
		//device_found = 1;
	}
	device_create(ONA_class, NULL, MKDEV(MAJOR(ONA_dev_number), 0), &ONA_driver ,"ONA");

		printk(KERN_ERR "ONA: ONA_probe() init regs.\n");
//	noa3402_reset(chip, 0x01);
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_RESET], 0x01))) < 0) {        	        
			pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_RESET],0x01);                	
            return(ret);
			}           
//init PS
//	noa3402_ps_mode(chip, NOACHIP_STANDBY);
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_PS_CONTROL], NOACHIP_STANDBY))) < 0) {        	        
			pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_PS_CONTROL],NOACHIP_STANDBY);                	
            return(ret);
			}           
//	noa3402_set_filter_m(chip, NOA3402_PS_FILTER_M);
//	noa3402_set_filter_n(chip, NOA3402_PS_FILTER_N);
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_PS_FILTER_CONFIG], NOA3402_PS_FILTER_M|NOA3402_PS_FILTER_N))) < 0) {        	        
			pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_PS_FILTER_CONFIG],NOA3402_PS_FILTER_M|NOA3402_PS_FILTER_N);                	
            return(ret);
			}           
//	noa3402_set_integ_time(chip, NOA3402_PS_INTEGRATION_TIME);
//	noa3402_set_hyst_trigger(chip, NOA3402_PS_HYST_TRIGGER);
//	noa3402_set_hyst_enable(chip,  NOA3402_PS_HYST_ENABLE);
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_PS_CONFIG], NOA3402_PS_HYST_ENABLE|NOA3402_PS_HYST_TRIGGER|NOA3402_PS_INTEGRATION_TIME))) < 0) {        	        
			pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_PS_CONFIG],NOA3402_PS_HYST_ENABLE|NOA3402_PS_HYST_TRIGGER|NOA3402_PS_INTEGRATION_TIME);                	
            return(ret);
			}           
//	noa3402_ps_set_threshold(chip,
//					NOA3402_PS_THRES_UP,
//					NOA3402_PS_THRES_LO);
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_PS_TH_UP_MSB], (ONA_cfgp->prox_threshold_hi >> 8)&0xFF))) < 0) {        	        
			pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_PS_TH_UP_MSB],(ONA_cfgp->prox_threshold_hi >> 8)&0xFF);                	
            return(ret);
			}           
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_PS_TH_UP_LSB], (ONA_cfgp->prox_threshold_hi)&0xFF))) < 0) {        	        
			pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_PS_TH_UP_LSB],(ONA_cfgp->prox_threshold_hi)&0xFF);                	
            return(ret);
			}           
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_PS_TH_LO_MSB], (ONA_cfgp->prox_threshold_lo >> 8)&0xFF))) < 0) {        	        
			pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_PS_TH_LO_MSB],(ONA_cfgp->prox_threshold_lo >> 8)&0xFF);                	
            return(ret);
			}           
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_PS_TH_LO_LSB], (ONA_cfgp->prox_threshold_lo)&0xFF))) < 0) {        	        
			pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_PS_TH_LO_LSB],(ONA_cfgp->prox_threshold_lo)&0xFF);                	
            return(ret);
			}           
//	noa3402_led_cfg(chip, NOA3402_PS_LED_CURRENT);
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_PS_LED_CURRENT], NOA3402_PS_LED_CURRENT))) < 0) {        	        
			pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_PS_LED_CURRENT],NOA3402_PS_LED_CURRENT);                	
            return(ret);
			}           
//	noa3402_ps_interval(chip, NOA3402_PS_INTERVAL);
	 	if ((ret = noa3402_ps_intervals_to_reg_value(NOA3402_PS_INTERVAL)) < 0) {
			pr_crit(ONA_TAG "error: noa3402_ps_intervals_to_reg_value(%d)\n",NOA3402_PS_INTERVAL);                	
            return(ret);
			}           
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_PS_MEAS_INTERVAL], ret))) < 0) {        	        
			pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_PS_MEAS_INTERVAL],noa3402_ps_intervals_to_reg_value(NOA3402_PS_INTERVAL));                	
            return(ret);
			}           

//init ALS
//	noa3402_als_mode(chip, NOACHIP_STANDBY);
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_ALS_CONTROL], NOACHIP_STANDBY))) < 0) {        	        
			pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_ALS_CONTROL],NOACHIP_STANDBY);                	
            return(ret);
			}           
//	noa3402_als_set_threshold(chip,
//					NOA3402_ALS_THRES_UP,
//					NOA3402_ALS_THRES_LO);
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_ALS_TH_UP_MSB], 0))) < 0) {        	        
			pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_ALS_TH_UP_MSB],0);                	
            return(ret);
			}           
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_ALS_TH_UP_LSB], 0))) < 0) {        	        
			pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_ALS_TH_UP_LSB],0);                	
            return(ret);
			}           
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_ALS_TH_LO_MSB], 0))) < 0) {        	        
			pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_ALS_TH_LO_MSB],0);                	
            return(ret);
			}           
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_ALS_TH_LO_LSB], 0))) < 0) {        	        
			pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_ALS_TH_LO_LSB],0);                	
            return(ret);
			}           
//	noa3402_als_interval(chip, NOA3402_ALS_INTERVAL);
	 	if ((ret = noa3402_als_intervals_to_reg_value(NOA3402_ALS_INTERVAL)) < 0) {
			pr_crit(ONA_TAG "error: noa3402_als_intervals_to_reg_value(%d)\n",NOA3402_ALS_INTERVAL);                	
            return(ret);
			}           
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_ALS_MEAS_INTERVAL], ret))) < 0) {        	        
			pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_ALS_MEAS_INTERVAL],noa3402_als_intervals_to_reg_value(NOA3402_ALS_INTERVAL));                	
            return(ret);
			}           
//	noa3402_set_als_hyst_trigger(chip, NOA3402_ALS_HYST_TRIGGER);
//	noa3402_set_als_hyst_enable(chip, NOA3402_ALS_HYST_ENABLE);
//	noa3402_set_als_integ_time(chip, NOA3402_ALS_INTEGRATION_TIME);
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_ALS_CONFIG], NOA3402_ALS_HYST_ENABLE|NOA3402_ALS_HYST_TRIGGER|NOA3402_ALS_INTEGRATION_TIME))) < 0) {        	        
			pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_ALS_CONFIG],NOA3402_ALS_HYST_ENABLE|NOA3402_ALS_HYST_TRIGGER|NOA3402_ALS_INTEGRATION_TIME);                	
            return(ret);
			}           

//	noa3402_interrupt_config(chip, NOA3402_INT_CONFIG);
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_INT_CONFIG], NOA3402_INT_CONFIG))) < 0) {        	        
			pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_INT_CONFIG],NOA3402_INT_CONFIG);                	
            return(ret);
			}           
	INIT_WORK(&ONA_datap->ONA_work, do_ONA_work); 
	mutex_init(&ONA_datap->proximity_calibrating);
	ONA_datap->pdata = &ONA_irq;
	if(clientp->irq){		
		ONA_datap->pdata->irq=ONA_datap->client->irq;
		ONA_datap->pdata->int_gpio=INT_TO_MSM_GPIO(ONA_datap->pdata->irq);
		}
	printk(KERN_CRIT "ONA use gpio %d\n",ONA_datap->pdata->int_gpio);
    	ret=ONA_config_int_gpio(ONA_datap->pdata->int_gpio);
    	if (ret) {
		printk(KERN_CRIT "ONA configure int_gpio%d failed\n",
                ONA_datap->pdata->int_gpio);
        	return ret;
    	}

    	ret = request_irq(ONA_datap->pdata->irq, ONA_interrupt, IRQF_TRIGGER_FALLING, 
			ONA_datap->ONA_name, ONA_prox_cur_infop);
    	if (ret) {
		printk(KERN_CRIT "ONA request interrupt failed\n");
        	return ret;
    	}
	
	strlcpy(clientp->name, ONA_DEVICE_ID, I2C_NAME_SIZE);
	strlcpy(ONA_datap->ONA_name, ONA_DEVICE_ID, ONA_ID_NAME_SIZE);
	ONA_datap->valid = 0;
	
	init_MUTEX(&ONA_datap->update_lock);
	if (!(ONA_cfgp = kmalloc(sizeof(struct PS_ALS_cfg), GFP_KERNEL))) {
		printk(KERN_ERR "ONA: kmalloc for struct PS_ALS_cfg failed in ONA_probe()\n");
		return -ENOMEM;
	}	

	//update settings
	//this should behind ONA_device_name	
	ONA_cfgp->calibrate_target = calibrate_target_param;
	ONA_cfgp->als_time = als_time_param;
	ONA_cfgp->scale_factor = scale_factor_param;
	ONA_cfgp->gain_trim = gain_trim_param;
	ONA_cfgp->filter_history = filter_history_param;
	ONA_cfgp->filter_count = filter_count_param;
	ONA_cfgp->gain = gain_param;
	ONA_cfgp->prox_threshold_hi = prox_threshold_hi_param;
	ONA_cfgp->prox_threshold_lo = prox_threshold_lo_param;
	ONA_cfgp->prox_int_time = prox_int_time_param;
	ONA_cfgp->prox_adc_time = prox_adc_time_param;
	ONA_cfgp->prox_wait_time = prox_wait_time_param;
	ONA_cfgp->prox_intr_filter = prox_intr_filter_param;
	ONA_cfgp->prox_config = prox_config_param;
	ONA_cfgp->prox_pulse_cnt = prox_pulse_cnt_param;
	ONA_cfgp->prox_gain = prox_gain_param;
	sat_als = 0xFFF;
	sat_prox = 0xFF;

	light = kzalloc(sizeof(struct alsprox_data), GFP_KERNEL);
	if (!light) {
		ret = -ENOMEM;
		goto exit_alloc_data_failed;
	}
	proximity= kzalloc(sizeof(struct alsprox_data), GFP_KERNEL);
	if (!proximity) {
		ret = -ENOMEM;
		goto exit_alloc_data_failed;
	}

	light->input_dev = input_allocate_device();
	if (!light->input_dev) {
		ret = -ENOMEM;
		printk(KERN_ERR "ONA_probe: Failed to allocate light input device\n");
		goto exit_input_dev_alloc_failed;
	}
	proximity->input_dev = input_allocate_device();
	if (!proximity->input_dev) {
		ret = -ENOMEM;
		printk(KERN_ERR "ONA_probe: Failed to allocate prox input device\n");
		goto exit_input_dev_alloc_failed;
	}
	
	/* lux */
	set_bit(EV_ABS, light->input_dev->evbit);
	input_set_abs_params(light->input_dev,  ABS_MISC, 0, 65535, 0, 0);
	light->input_dev->name = "light";
	/* prox */
	set_bit(EV_ABS, proximity->input_dev->evbit);
	input_set_abs_params(proximity->input_dev, ABS_DISTANCE, 0, 65535, 0, 0);
	proximity->input_dev->name = "proximity";
	
	ret = input_register_device(light->input_dev);
	if (ret) {
		printk("ONA_probe: Unable to register input device: %s\n", light->input_dev->name);
		goto exit_input_register_device_failed;
	}
	ret = input_register_device(proximity->input_dev);
	if (ret) {
		printk("ONA_probe: Unable to register input device: %s\n", proximity->input_dev->name);
		goto exit_input_register_device_failed;
	}
	
	return (ret);
//exit_ONA_device_register_failed:	
exit_input_register_device_failed:
	if(light->input_dev)
		input_free_device(light->input_dev);
	if(proximity->input_dev)
		input_free_device(proximity->input_dev);
exit_input_dev_alloc_failed:
exit_alloc_data_failed:
	if(light)
		kfree(light);
	if(proximity)
		kfree(proximity);
	return (ret);		
}


// client remove
static int __devexit ONA_remove(struct i2c_client *client) {
	int ret = 0;

	return (ret);
}

// open
static int ONA_open(struct inode *inode, struct file *file) {
	struct ONA_data *ONA_datap;
	int ret = 0;
	device_released = 0;
	ONA_datap = container_of(inode->i_cdev, struct ONA_data, cdev);
	if (strcmp(ONA_datap->ONA_name, ONA_DEVICE_ID) != 0) {
		printk(KERN_ERR "ONA: device name incorrect during ONA_open(), get %s\n", ONA_datap->ONA_name);
		ret = -ENODEV;
	}
	return (ret);
}

// release
static int ONA_release(struct inode *inode, struct file *file) {
	struct ONA_data *ONA_datap;
	int ret = 0;

	device_released = 1;
	prox_on = 0;
	//printk(KERN_CRIT "[ONA]:%s\n", __func__);

	//prox_history_hi = 0;
	//prox_history_hi = 0;
	ONA_datap = container_of(inode->i_cdev, struct ONA_data, cdev);
	if (strcmp(ONA_datap->ONA_name, ONA_DEVICE_ID) != 0) {
		printk(KERN_ERR "ONA: device name incorrect during ONA_release(), get %s\n", ONA_datap->ONA_name);
		ret = -ENODEV;
	}
	return (ret);
}

// read
static int ONA_read(struct file *file, char *buf, size_t count, loff_t *ppos) {
	struct ONA_data *ONA_datap;
	u8 i = 0, xfrd = 0, reg = 0;
	u8 my_buf[NAOCHIP_REG_NUM];
	int ret = 0;

	//if (prox_on)
		//del_timer(&prox_poll_timer);
        if ((*ppos < 0) || (*ppos >= NAOCHIP_REG_NUM)  || (count > NAOCHIP_REG_NUM)) {
		printk(KERN_ERR "ONA: reg limit check failed in ONA_read()\n");
		return -EINVAL;
	}
	reg = (u8)*ppos;
	ONA_datap = container_of(file->f_dentry->d_inode->i_cdev, struct ONA_data, cdev);
	while (xfrd < count) {
      		my_buf[i++] = i2c_smbus_read_byte_data(ONA_datap->client,noa_regmap[reg]);
			reg++;
			xfrd++;
        }
        if ((ret = copy_to_user(buf, my_buf, xfrd))){
		printk(KERN_ERR "ONA: copy_to_user failed in ONA_read()\n");
		return -ENODATA;
	}
	//if (prox_on)
		//ONA_prox_poll_timer_start();
        return ((int)xfrd);
}

// write
static int ONA_write(struct file *file, const char *buf, size_t count, loff_t *ppos) {
	struct ONA_data *ONA_datap;
	u8 i = 0, xfrd = 0, reg = 0;
	u8 my_buf[NAOCHIP_REG_NUM];
	int ret = 0;

	//if (prox_on)
		//del_timer(&prox_poll_timer);
        if ((*ppos < 0) || (*ppos >= NAOCHIP_REG_NUM) || ((*ppos + count) > NAOCHIP_REG_NUM)) {
		printk(KERN_ERR "ONA: reg limit check failed in ONA_write()\n");
		return -EINVAL;
	}
	reg = (u8)*ppos;
        if ((ret =  copy_from_user(my_buf, buf, count))) {
		printk(KERN_ERR "ONA: copy_to_user failed in ONA_write()\n");
 		return -ENODATA;
	}
        ONA_datap = container_of(file->f_dentry->d_inode->i_cdev, struct ONA_data, cdev);
        while (xfrd < count) {
                if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[reg], my_buf[i++]))) < 0) {
                        printk(KERN_ERR "ONA: i2c_smbus_write_byte_data failed in ONA_write()\n");
                        return (ret);
                }
                reg++;
                xfrd++;
        }
	//if (prox_on)
		//ONA_prox_poll_timer_start();
        return ((int)xfrd);
}

// llseek
static loff_t ONA_llseek(struct file *file, loff_t offset, int orig) {
	int ret = 0;
	loff_t new_pos = 0;

	//if (prox_on)
		//del_timer(&prox_poll_timer);
	if ((offset >= NAOCHIP_REG_NUM) || (orig < 0) || (orig > 1)) {
                printk(KERN_ERR "ONA: offset param limit or origin limit check failed in ONA_llseek()\n");
                return -EINVAL;
        }
        switch (orig) {
        	case 0:
                	new_pos = offset;
                	break;
        	case 1:
                	new_pos = file->f_pos + offset;
	                break;
		default:
			return -EINVAL;
			break;
	}
	if ((new_pos < 0) || (new_pos >= NAOCHIP_REG_NUM) || (ret < 0)) {
		printk(KERN_ERR "ONA: new offset limit or origin limit check failed in ONA_llseek()\n");
		return -EINVAL;
	}
	file->f_pos = new_pos;
       // if (prox_on)
              //  ONA_prox_poll_timer_start();
	return new_pos;
}

/*enable_light_and_proximity, mask values' indication*/
/*10 : light on*/
/*01 : prox on*/
/*20 : light off*/
/*02 : prox off*/
static int enable_light_and_proximity(int mask)
{
	u8 ret = 0;// reg_val = 0 //itime = 0;

	if(mask==0x10) /*10 : light on*/
	{
		pr_crit(ONA_TAG "light on\n");
//	noa3402_als_set_threshold(chip,
//					NOA3402_ALS_THRES_UP,
//					NOA3402_ALS_THRES_LO);
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_ALS_TH_UP_MSB], 0))) < 0) {        	        
			pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_ALS_TH_UP_MSB],0);                	
            return(ret);
			}           
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_ALS_TH_UP_LSB], 0))) < 0) {        	        
			pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_ALS_TH_UP_LSB],0);                	
            return(ret);
			}           
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_ALS_TH_LO_MSB], 0))) < 0) {        	        
			pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_ALS_TH_LO_MSB],0);                	
            return(ret);
			}           
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_ALS_TH_LO_LSB], 0))) < 0) {        	        
			pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_ALS_TH_LO_LSB],0);                	
            return(ret);
			}           
//	noa3402_als_mode(chip, NOACHIP_PS_REPEAT);
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_ALS_CONTROL], NOACHIP_PS_REPEAT))) < 0) {        	        
			pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_ALS_CONTROL],NOACHIP_PS_REPEAT);                	
            return(ret);
			} 
	}	
	if(mask==0x01) /*01 : prox on*/
	{
		pr_crit(ONA_TAG "prox on\n");
//	noa3402_ps_set_threshold(chip,
//					NOA3402_PS_THRES_UP,
//					NOA3402_PS_THRES_LO);
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_PS_TH_UP_MSB], (ONA_cfgp->prox_threshold_hi >> 8)&0xFF))) < 0) {        	        
			pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_PS_TH_UP_MSB],(ONA_cfgp->prox_threshold_hi >> 8)&0xFF);                	
            return(ret);
			}           
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_PS_TH_UP_LSB], (ONA_cfgp->prox_threshold_hi)&0xFF))) < 0) {        	        
			pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_PS_TH_UP_LSB],(ONA_cfgp->prox_threshold_hi)&0xFF);                	
            return(ret);
			}           
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_PS_TH_LO_MSB], (ONA_cfgp->prox_threshold_lo >> 8)&0xFF))) < 0) {        	        
			pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_PS_TH_LO_MSB],(ONA_cfgp->prox_threshold_lo >> 8)&0xFF);                	
            return(ret);
			}           
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_PS_TH_LO_LSB], (ONA_cfgp->prox_threshold_lo)&0xFF))) < 0) {        	        
			pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_PS_TH_LO_LSB],(ONA_cfgp->prox_threshold_lo)&0xFF);                	
            return(ret);
			}           
//	noa3402_ps_mode(chip, NOACHIP_PS_REPEAT);
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_PS_CONTROL], NOACHIP_PS_REPEAT))) < 0) {        	        
			pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_PS_CONTROL],NOACHIP_PS_REPEAT);                	
            return(ret);
			} 
	}	
	if(mask==0x20) /*20 : light off*/
	{
		pr_crit(ONA_TAG "light off\n");
//	noa3402_als_mode(chip, NOACHIP_STANDBY);
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_ALS_CONTROL], NOACHIP_STANDBY))) < 0) {        	        
			pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_ALS_CONTROL],NOACHIP_STANDBY);                	
            return(ret);
			} 
	}
	if(mask==0x02) /*02 : prox off*/
	{
		pr_crit(ONA_TAG "prox off\n");
//	noa3402_ps_mode(chip, NOACHIP_STANDBY);
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_PS_CONTROL], NOACHIP_STANDBY))) < 0) {        	        
			pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_PS_CONTROL],NOACHIP_STANDBY);                	
            return(ret);
			} 
	}
	return ret;
}

// ioctls
static int ONA_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg) {
	struct ONA_data *ONA_datap;
	int prox_sum = 0, prox_mean = 0, prox_max = 0;
	int lux_val = 0, ret = 0, i = 0;
	u8 reg_val = 0;
	u16 ratio;
	struct ONA_prox_data *prox_pt;

	ONA_datap = container_of(inode->i_cdev, struct ONA_data, cdev);
//[sensor wlg 20110729]ALS thereshold modify add log
//printk(KERN_ERR "ONA_wlg: ONA_ioctl() cmd=%d\n", cmd);
	switch (cmd) {
		case PS_ALS_IOCTL_ALS_ON:
			if(light_on)
			{
			    pr_crit(ONA_TAG "PS_ALS_IOCTL_ALS_ON: light already on.\n");
			    break;
			}
			ret=enable_light_and_proximity(0x10);
			if(ret>=0)
			{
				light_on=1;	
      			pr_crit(ONA_TAG "PS_ALS_IOCTL_ALS_ON,lux=%d\n", ONA_g_nlux);
			}			
			return (ret);
			break;
        case PS_ALS_IOCTL_ALS_OFF:	
			if(light_on == 0)
			{
			    pr_crit(ONA_TAG "PS_ALS_IOCTL_ALS_OFF: light already off.\n");
			    break;
			}

            for (i = 0; i < ONA_FILTER_DEPTH; i++)
                lux_history[i] = -ENODATA;
			ret=enable_light_and_proximity(0x20);
			if(ret>=0)
			{
				light_on=0;
				//ONA_g_nlux=0;
            	pr_crit(ONA_TAG"PS_ALS_IOCTL_ALS_OFF\n");
			}			
			return (ret);
            break;
		case PS_ALS_IOCTL_ALS_DATA:
			if(light_on == 0)
			{
			    pr_crit(ONA_TAG "PS_ALS_IOCTL_ALS_DATA: read als data when light off.\n");
			}

    		ret = i2c_smbus_read_byte_data(ONA_datap->client,noa_regmap[NOACHIP_ALS_DATA_MSB]);
			if(ret < 0)
			{
				return ret;
            }
			reg_val = ret;
			ret = i2c_smbus_read_byte_data(ONA_datap->client,noa_regmap[NOACHIP_ALS_DATA_LSB]);
			if(ret < 0)
			{
				return ret;
            }
			lux_val = (ret & 0xFF) | ((reg_val & 0xFF) << 8);
			return lux_val;
			break;
		case PS_ALS_IOCTL_ALS_CALIBRATE:

			break;
			
		case PS_ALS_IOCTL_CONFIG_GET:
			ret = copy_to_user((struct PS_ALS_cfg *)arg, ONA_cfgp, sizeof(struct PS_ALS_cfg));
			if (ret) {
				printk(KERN_ERR "ONA: copy_to_user failed in ioctl config_get\n");
				return -ENODATA;
			}
			return (ret);
			break;
        case PS_ALS_IOCTL_CONFIG_SET:
            ret = copy_from_user(ONA_cfgp, (struct PS_ALS_cfg *)arg, sizeof(struct PS_ALS_cfg));
			if (ret) {
				printk(KERN_ERR "ONA: copy_from_user failed in ioctl config_set\n");
                return -ENODATA;
			}
  			return (ret);
            break;
		case PS_ALS_IOCTL_PROX_ON:				
			if(prox_on)
			{
			    pr_crit(ONA_TAG "PS_ALS_IOCTL_PROX_ON: prox already on.\n");
				break;
			}

    		ONA_prox_cur_infop->prox_event = 0;
            ONA_prox_cur_infop->prox_clear = 0;
            ONA_prox_cur_infop->prox_data = 0;
			ret=enable_light_and_proximity(0x01);
			if(ret>=0)
			{
				prox_on = 1;	
            	pr_crit(ONA_TAG "PS_ALS_IOCTL_PROX_ON\n");
			}	
			return ret;	
			break;
        case PS_ALS_IOCTL_PROX_OFF:
			if(prox_on == 0)
			{
			    pr_crit(ONA_TAG "PS_ALS_IOCTL_PROX_OFF: prox already off.\n");
				break;
			}

    		ret=enable_light_and_proximity(0x02);
			if(ret>=0)
			{
				prox_on = 0;	
            	pr_crit(ONA_TAG"PS_ALS_IOCTL_PROX_OFF\n");
			}	
			return ret;	
			break;
		case PS_ALS_IOCTL_PROX_DATA:
            ret = copy_to_user((struct PS_ALS_prox_info *)arg, ONA_prox_cur_infop, sizeof(struct PS_ALS_prox_info));
            if (ret) {
                     printk(KERN_ERR "ONA: copy_to_user failed in ioctl prox_data\n");
                     return -ENODATA;
            }
            return (ret);
            break;
        case PS_ALS_IOCTL_PROX_EVENT:
 			return (ONA_prox_cur_infop->prox_event);
            break;
		case PS_ALS_IOCTL_PROX_CALIBRATE:
			if (!prox_on)			
			 {
				printk(KERN_ERR "ONA: ioctl prox_calibrate was called before ioctl prox_on was called\n");
				return -EPERM;
			}
			mutex_lock(&ONA_datap->proximity_calibrating);
				prox_sum = 0;
				prox_max = 0;
				for (i = 0; i < 20; i++) {
	                        if ((ret = ONA_prox_poll(&ONA_prox_cal_info[i])) < 0) {
        	                        printk(KERN_ERR "ONA: call to prox_poll failed in ioctl prox_calibrate\n");
						mutex_unlock(&ONA_datap->proximity_calibrating);			
                	                return (ret);
                        		}					
				pr_crit(ONA_TAG "prox calibrate poll prox[%d] = %d\n",i,ONA_prox_cal_info[i].prox_data); 			
					prox_sum += ONA_prox_cal_info[i].prox_data;
					if (ONA_prox_cal_info[i].prox_data > prox_max)
						prox_max = ONA_prox_cal_info[i].prox_data;
				mdelay(100);
				}
				prox_mean = prox_sum/20;
			ratio = 10*prox_mean/sat_prox;
			for (prox_pt = ONA_prox_tablep; prox_pt->ratio && prox_pt->ratio <= ratio; prox_pt++);
        		if(!prox_pt->ratio)
                	return -1;
			 
			ONA_cfgp->prox_threshold_hi = (prox_mean*prox_pt->hi)/10;
				ONA_cfgp->prox_threshold_lo = ONA_cfgp->prox_threshold_hi*THRES_LO_TO_HI_RATIO;					
				
			if (ONA_cfgp->prox_threshold_lo < ((sat_prox*15)/100)) {				
				ONA_cfgp->prox_threshold_hi = ((sat_prox*20)/100);
				ONA_cfgp->prox_threshold_lo = (ONA_cfgp->prox_threshold_hi *THRES_LO_TO_HI_RATIO);
			}
			
			//write back threshold
//	noa3402_ps_set_threshold(chip,
//					NOA3402_PS_THRES_UP,
//					NOA3402_PS_THRES_LO);
			if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_PS_TH_UP_MSB], (ONA_cfgp->prox_threshold_hi >> 8)&0xFF))) < 0) {        	        
				pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_PS_TH_UP_MSB],(ONA_cfgp->prox_threshold_hi >> 8)&0xFF);                	
				return(ret);
				}           
			if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_PS_TH_UP_LSB], (ONA_cfgp->prox_threshold_hi)&0xFF))) < 0) {        	        
				pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_PS_TH_UP_LSB],(ONA_cfgp->prox_threshold_hi)&0xFF);                	
				return(ret);
				}           
			if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_PS_TH_LO_MSB], (ONA_cfgp->prox_threshold_lo >> 8)&0xFF))) < 0) {        	        
				pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_PS_TH_LO_MSB],(ONA_cfgp->prox_threshold_lo >> 8)&0xFF);                	
				return(ret);
				}           
			if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_PS_TH_LO_LSB], (ONA_cfgp->prox_threshold_lo)&0xFF))) < 0) {        	        
				pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_PS_TH_LO_LSB],(ONA_cfgp->prox_threshold_lo)&0xFF);                	
				return(ret);
				}           
			mutex_unlock(&ONA_datap->proximity_calibrating);	
			pr_crit(KERN_ERR "ONA prox_cal_threshold_hi=%d,prox_cal_threshold_lo=%d\n",ONA_cfgp->prox_threshold_hi,ONA_cfgp->prox_threshold_lo);
			break;
					
		case PS_ALS_IOCTL_PROX_GET_ENABLED:
			return put_user(prox_on, (unsigned long __user *)arg);
			break;	
		case PS_ALS_IOCTL_ALS_GET_ENABLED:
			return put_user(light_on, (unsigned long __user *)arg);
			break;
			
		default:
			return -EINVAL;
			break;
	}
	return (ret);
}

// read and calculate lux value
static int ONA_get_lux(void) 
{
        u16 raw_clear = 0, reg_val=0;
	u32 lux = 0;
//	u32 ratio = 0;
//	u8 dev_gain = 0;
//	struct lux_data *p;
	int ret = 0;

//	int i = 0;
    	pr_crit(ONA_TAG "ONA start to calc lux value\n");

   	ret = i2c_smbus_read_byte_data(ONA_datap->client,noa_regmap[NOACHIP_ALS_DATA_MSB]);
	if(ret < 0)
	{
		return ret;
    }
	reg_val = ret;
	ret = i2c_smbus_read_byte_data(ONA_datap->client,noa_regmap[NOACHIP_ALS_DATA_LSB]);
	if(ret < 0)
	{
		return ret;
    }
	raw_clear = (ret & 0xFF) | ((reg_val & 0xFF) << 8);

	als_intr_threshold_hi_param = raw_clear + raw_clear/5;
    als_intr_threshold_lo_param = raw_clear - raw_clear/5;

	//update threshold 
    	printk(ONA_TAG "als_intr_threshold_hi_param=%x,als_intr_threshold_lo_param=%x\n",als_intr_threshold_hi_param,als_intr_threshold_lo_param);
//	noa3402_als_set_threshold(chip,
//					als_intr_threshold_hi_param,
//					als_intr_threshold_lo_param);
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_ALS_TH_UP_MSB], (als_intr_threshold_hi_param>>8)&0xFF))) < 0) {        	        
			pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_ALS_TH_UP_MSB],(als_intr_threshold_hi_param>>8)&0xFF);                	
            return(ret);
			}           
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_ALS_TH_UP_LSB], (als_intr_threshold_hi_param)&0xFF))) < 0) {        	        
			pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_ALS_TH_UP_LSB],(als_intr_threshold_hi_param)&0xFF);                	
            return(ret);
			}           
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_ALS_TH_LO_MSB], (als_intr_threshold_lo_param>>8)&0xFF))) < 0) {        	        
			pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_ALS_TH_LO_MSB],(als_intr_threshold_lo_param>>8)&0xFF);                	
            return(ret);
			}           
	 	if ((ret = (i2c_smbus_write_byte_data(ONA_datap->client, noa_regmap[NOACHIP_ALS_TH_LO_LSB], (als_intr_threshold_lo_param)&0xFF))) < 0) {        	        
			pr_crit(ONA_TAG "error: i2c_smbus_write_byte(%d) val %d\n",noa_regmap[NOACHIP_ALS_TH_LO_LSB],(als_intr_threshold_lo_param>>8)&0xFF);                	
            return(ret);
			}           	
	lux = raw_clear;
	return(lux);
}


// verify device
static int ONA_device_name(unsigned char *bufp, char **device_name) {
		ONA_datap->chip_type=ONA3301;
/*		if( (bufp[0x12]&0xf0)!=0x00)
			return(0);
		if(bufp[0x10]|bufp[0x1a]|bufp[0x1b]|bufp[0x1c]|bufp[0x1d]|bufp[0x1e])
			return(0);
  		if(bufp[0x13]&0x0c)
			return(0);
*/			
        if( (bufp[NOACHIP_PART_ID]&NOACHIP_PART_MASK)!=NOACHIP_PART)
		    return(0);
		*device_name="ONA3301";
		return(1);
}

// proximity poll
static int ONA_prox_poll(struct PS_ALS_prox_info *prxp) {
	u8 prox_data;
    int ret,reg_val;
	
   	ret = i2c_smbus_read_byte_data(ONA_datap->client,noa_regmap[NOACHIP_PS_DATA_MSB]);
	if(ret < 0)
	{
		return ret;
    }
	reg_val = ret;
	ret = i2c_smbus_read_byte_data(ONA_datap->client,noa_regmap[NOACHIP_PS_DATA_LSB]);
	if(ret < 0)
	{
		return ret;
    }
	prox_data = (ret & 0xFF) | ((reg_val & 0xFF) << 8);
	prxp->prox_data = prox_data;

	return (prox_data);
	}
// start prox poll timer
/*
static void ONA_prox_poll_timer_start(void) {
        init_timer(&prox_poll_timer);
        prox_poll_timer.expires = jiffies + (HZ/10);
        prox_poll_timer.function = ONA_prox_poll_timer_func;
        add_timer(&prox_poll_timer);

        return;
}*/


MODULE_AUTHOR("ZTE Software");
MODULE_DESCRIPTION("ONA ambient light and proximity sensor driver");
MODULE_LICENSE("GPL");

module_init(ONA_init);
module_exit(ONA_exit);

