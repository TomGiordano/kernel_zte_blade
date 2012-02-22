/******************************************************************
*File Name: 	ISL29026.c 	                                           *
*Description:	Linux device driver for ISL ambient light and    *
*			proximity sensors.                                          *                                
*******************************************************************
Geschichte:	                                                                        
Wenn               Wer          Was                                                                        Tag
2011-10-15    wanglg     create
                                   
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
#include <linux/i2c/PS_ALS_common.h>
#include <linux/input.h>
#include <linux/miscdevice.h>


#define ISL_INT_GPIO 42
#define ISL_TAG        "[ISL]"

// device name/id/address/counts
#define ISL_DEVICE_NAME		"ISL"
#define ISL_DEVICE_ID                "isl29026"
#define ISL_ID_NAME_SIZE		10

#define ISL_MAX_NUM_DEVICES		3
#define ISL_MAX_DEVICE_REGS		10


//#define RD_CHIPID		0x0
#define ISL_CFG_REG		0x1
#define ISL_PROX_EN_Bit    0x80 //1 is enable
#define ISL_PROX_SLP_Bit   0x70 //011 means sleep 100ms
#define ISL_PROX_DR_Bit    0x08 // 0 is 110mA, 1 is 220mA
#define ISL_ALS_EN_Bit     0x04 //1 is enable
#define ISL_ALS_RANGE_Bit  0x02 //0 is low range, 1 is high range
#define ISL_ALSIR_MODE_Bit 0x01 //0 is ALS mode, 1 is IR spectrum mode
#define ISL_SENSOR_All_Off 0x00
#define ISL_ALS_On         (ISL_ALS_EN_Bit | ISL_ALS_RANGE_Bit)
#define ISL_ALS_mask       (ISL_ALS_EN_Bit | ISL_ALS_RANGE_Bit | ISL_ALSIR_MODE_Bit)
//#define ISL_PROX_On        (0xD0) //101 50ms 110 100ms 
#define ISL_PROX_mask      (ISL_PROX_EN_Bit | ISL_PROX_SLP_Bit | ISL_PROX_DR_Bit)
#if defined(CONFIG_MACH_BLADE)
#define ISL_PROX_On        (0xD0) //101 50ms 110 100ms 
#elif defined(CONFIG_MACH_RACER2)
#define ISL_PROX_On        (0xD0) //101 50ms 110 100ms 
#elif defined(CONFIG_MACH_SKATE)
#define ISL_PROX_On        (0xD8) //101 50ms 8 220mA 
#elif defined(CONFIG_MACH_ROAMER)
#define ISL_PROX_On        (0xD0) //101 50ms 110 100ms 
#elif defined(CONFIG_MACH_BLADE2)
#define ISL_PROX_On        (0xD0) //101 50ms 110 100ms 
#else
#define ISL_PROX_On        (0xD0) //101 50ms 110 100ms 
#endif



#define ISL_INT_REG		0x2
#define ISL_INT_PROX_Bit      0x80 //read value is 1 means interrupt event occurred, 0 means no interrupt or cleared
#define ISL_INT_PROX_PRST_Bit 0x60 //00 is 1 c, 01 is 4 c, 10 is 8 c, 11 is 16 c
#define ISL_INT_ALS_Bit       0x08 //read value is 1 means interrupt event occurred, 0 means no interrupt or cleared
#define ISL_INT_ALS_PRST_Bit  0x06 //00 is 1 c, 01 is 4 c, 10 is 8 c, 11 is 16 c
#define ISL_INT_CTRL_Bit      0x01 //0 is PROX or ALS, 1 is PROX and ALS
#define ISL_INT_All_Off       0x00
#define ISL_ALS_INT_On         (0x00) // 4times over/ 4 means 8times over
#define ISL_ALS_INT_mask       (0x0E)
#define ISL_PROX_INT_On        (0x20)  // 4times over
#define ISL_PROX_INT_mask      (0xE0)

#define ISL_PROX_TH_L	0x3
#define ISL_PROX_TH_H	0x4

#define ISL_ALSIR_TH_L	0x5
#define ISL_ALSIR_TH_HL	0x6
#define ISL_ALSIR_TH_H	0x7

#define ISL_PROX_DATA	0x8

#define ISL_ALSIR_DATA_L	0x9
#define ISL_ALSIR_DATA_H	0xa

// lux constants
#define	ISL_MAX_LUX			65535000
//#define ISL_SCALE_MILLILUX		3
#define ISL_FILTER_DEPTH		3
#define THRES_LO_TO_HI_RATIO  4/5

// forward declarations
static int ISL_probe(struct i2c_client *clientp, const struct i2c_device_id *idp);
static int ISL_remove(struct i2c_client *client);
static int ISL_open(struct inode *inode, struct file *file);
static int ISL_release(struct inode *inode, struct file *file);
static int ISL_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);
static int ISL_read(struct file *file, char *buf, size_t count, loff_t *ppos);
static int ISL_write(struct file *file, const char *buf, size_t count, loff_t *ppos);
static loff_t ISL_llseek(struct file *file, loff_t offset, int orig);
static int ISL_get_lux(void);
static int ISL_device_name(unsigned char *bufp, char **device_name);
static int ISL_prox_poll(struct PS_ALS_prox_info *prxp);

static void do_ISL_work(struct work_struct *w);
static void ISL_report_value(int mask);
static int calc_distance(int value);
static int enable_light_and_proximity(int mask);	
static int light_on=0;  
static int prox_on = 0;

enum ISL_chip_type {
	TSL2771 = 0,
	TMD2771,	
};

struct alsprox_data {
	struct input_dev *input_dev;
};

static struct alsprox_data *light;
static struct alsprox_data *proximity;
// first device number
static dev_t ISL_dev_number;

// class structure for this device
struct class *ISL_class;

// module device table
static struct i2c_device_id ISL_idtable[] = {
        {ISL_DEVICE_ID, 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, ISL_idtable);


// client and device
struct i2c_client *ISL_my_clientp;
struct i2c_client *ISL_bad_clientp[ISL_MAX_NUM_DEVICES];
//static int num_bad = 0;
//static int device_found = 0;

// driver definition
static struct i2c_driver ISL_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "isl29026",
	 },
	.id_table = ISL_idtable,
	.probe = ISL_probe,
	.remove = __devexit_p(ISL_remove),
};


struct ISL_intr_data {
    int int_gpio;
    int irq;
};

// per-device data
struct ISL_data {
	struct i2c_client *client;
	struct cdev cdev;
	unsigned int addr;
	char ISL_id;
	char ISL_name[ISL_ID_NAME_SIZE];
	struct semaphore update_lock;
	char valid;
	unsigned long last_updated;
    	struct ISL_intr_data *pdata;
	struct work_struct  ISL_work;
	enum ISL_chip_type chip_type;
	struct mutex proximity_calibrating;
} *ISL_datap;

static struct ISL_intr_data ISL_irq= {
    .int_gpio = ISL_INT_GPIO,
    .irq = MSM_GPIO_TO_INT(ISL_INT_GPIO),
};


// file operations
static struct file_operations ISL_fops = {
	.owner = THIS_MODULE,
	.open = ISL_open,
	.release = ISL_release,
	.read = ISL_read,
	.write = ISL_write,
	.llseek = ISL_llseek,
	.ioctl = ISL_ioctl,
};

// device configuration
struct PS_ALS_cfg *ISL_cfgp;
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

static u16 prox_threshold_hi_param = 1023; 
static u16 prox_threshold_lo_param = 818;
static u8 prox_int_time_param = 0xF6;
static u8 prox_adc_time_param = 0xFF;
static u8 prox_wait_time_param = 0xFF;
static u8 prox_intr_filter_param = 0x00;
static u8 prox_config_param = 0x00;

static u8 prox_pulse_cnt_param = 0x10;
static u8 prox_gain_param = 0x20;

// device reg init values
u8 ISL_triton_reg_init[16] = {0x00,0xFF,0XFF,0XFF,0X00,0X00,0XFF,0XFF,0X00,0X00,0XFF,0XFF,0X00,0X00,0X00,0X00};

//
static u16 als_intr_threshold_hi_param = 0;
static u16 als_intr_threshold_lo_param = 0;
int ISL_g_nlux = 0;

		

// prox info
struct PS_ALS_prox_info ISL_prox_cal_info[20];
struct PS_ALS_prox_info ISL_prox_cur_info;
struct PS_ALS_prox_info *ISL_prox_cur_infop = &ISL_prox_cur_info;


static int device_released = 0;
static u16 sat_als = 0;
static u16 sat_prox = 0;

// lux time scale
struct time_scale_factor  {
	u16	numerator;
	u16	denominator;
	u16	saturation;
};
struct time_scale_factor ISL_TritonTime = {1, 0, 0};
struct time_scale_factor *ISL_lux_timep = &ISL_TritonTime;

// gain table
u8 ISL_triton_gain_table[] = {1, 8, 16, 120};

// lux data
struct lux_data {
	u16	ratio;
	u16	clear;
	u16	ir;
};
struct lux_data ISL_TritonFN_lux_data[] = {
        { 9830,  8320,  15360 },
        { 12452, 10554, 22797 },
        { 14746, 6234,  11430 },
        { 17695, 3968,  6400  },
        { 0,     0,     0     }
};
struct lux_data *ISL_lux_tablep = ISL_TritonFN_lux_data;
static int lux_history[ISL_FILTER_DEPTH] = {-ENODATA, -ENODATA, -ENODATA};

//prox data
struct ISL_prox_data {
	u16	ratio;
	u16	hi;
	u16	lo;
};
struct ISL_prox_data ISL_prox_data[] = {
        { 1,  50,  20 },
        { 3, 20, 16 },
        { 6, 18, 14 },
        { 10, 16,  16 },      
        { 0,  0,   0 }
};
struct ISL_prox_data *ISL_prox_tablep = ISL_prox_data;


/* ----------------------*
* config gpio for intr utility        *
*-----------------------*/
int ISL_config_int_gpio(int int_gpio)
{
    int rc=0;
    uint32_t  gpio_config_data = GPIO_CFG(int_gpio,  0, GPIO_INPUT, GPIO_PULL_UP, GPIO_2MA);

    rc = gpio_request(int_gpio, "gpio_sensor");
    if (rc) {
        printk(ISL_TAG "%s: gpio_request(%#x)=%d\n",
                __func__, int_gpio, rc);
        return rc;
    }

    rc = gpio_tlmm_config(gpio_config_data, GPIO_ENABLE);
    if (rc) {
        printk(ISL_TAG "%s: gpio_tlmm_config(%#x)=%d\n",
                __func__, gpio_config_data, rc);
        return rc;
    }

    mdelay(1);

    rc = gpio_direction_input(int_gpio);
    if (rc) {
        printk(ISL_TAG "%s: gpio_direction_input(%#x)=%d\n",
                __func__, int_gpio, rc);
        return rc;
    }

    return 0;
}

/* ----------------------*
* ISL interrupt function         *
*-----------------------*/
static irqreturn_t ISL_interrupt(int irq, void *data)
{
    //printk(ISL_TAG "ISL_interrupt\n");	
    disable_irq_nosync(ISL_datap->pdata->irq);
    schedule_work(&ISL_datap->ISL_work);
    
    return IRQ_HANDLED;
}

static void do_ISL_work(struct work_struct *w)
{

    int ret =0;	
    //int prx_hi, prx_lo;
    u16 status = 0;	
    status = i2c_smbus_read_byte_data(ISL_datap->client,ISL_INT_REG);//add error ctl
    //printk(ISL_TAG "ISL_interrupt status = %x\n",status);	
    if(status<0)
	goto read_reg_fail;
   
    //als interrupt
    if(((status & ISL_INT_ALS_Bit) != 0)/*&&((status & ISL_INT_PROX_Bit)==0)*/)
    {
	 status = status & (~ISL_INT_ALS_Bit);
	 //printk(ISL_TAG "ISL_interrupt als change status = %d\n",status);	
	 ISL_g_nlux = ISL_get_lux();
     ISL_report_value(0);
    }
	
    //prox interrupt

//	if(((status & ISL_INT_PROX_Bit)!=0)||ISL_prox_cur_infop->prox_event)
	if((status & ISL_INT_PROX_Bit)!=0)
    {
    status = status & (~ISL_INT_PROX_Bit);
	//printk(ISL_TAG "ISL_interrupt prox change status = %d\n",status);	
	if((ret = ISL_prox_poll(ISL_prox_cur_infop))<0)
	{
		printk(KERN_CRIT "ISL: get prox poll  failed in  ISL interrupt()\n");  	

	}
	//printk(ISL_TAG "prox_data = %d  prox_threshold_hi = %d  prox_threshold_lo = %d \n",ISL_prox_cur_infop->prox_data,ISL_cfgp->prox_threshold_hi,ISL_cfgp->prox_threshold_lo);		
	if(ISL_prox_cur_infop->prox_data > ISL_cfgp->prox_threshold_hi)       
	 {           
	 	if ((ret = (i2c_smbus_write_byte_data(ISL_datap->client, ISL_PROX_TH_H, 0x01))) < 0) {        	        
			pr_crit(ISL_TAG "i2c_smbus_write_byte() to ISL_TRITON_PRX_MAXTHRESHLO\n");                	
			}           
/*	 	if ((ret = (i2c_smbus_write_byte_data(ISL_datap->client, ISL_PROX_TH_L, 0x00))) < 0) {        	        
			pr_crit(ISL_TAG "i2c_smbus_write_byte() to ISL_TRITON_PRX_MAXTHRESHLO\n");                	
			}           
*/
		ISL_prox_cur_infop->prox_event = 1;           
	}        
	else if(ISL_prox_cur_infop->prox_data < ISL_cfgp->prox_threshold_lo)        
	{            
		if ((ret = (i2c_smbus_write_byte_data(ISL_datap->client, ISL_PROX_TH_H,ISL_cfgp->prox_threshold_hi))) < 0) {    	       
			pr_crit(ISL_TAG "i2c_smbus_write_byte() to ISL_TRITON_PRX_MAXTHRESHLO\n");            	
			}             
/*		if ((ret = (i2c_smbus_write_byte_data(ISL_datap->client, ISL_PROX_TH_L,0x00))) < 0) {    	    
			pr_crit(ISL_TAG "i2c_smbus_write_byte() to ISL_TRITON_PRX_MINTHRESHLO\n");            
			}
*/			
		ISL_prox_cur_infop->prox_event = 0;             
		//pr_crit(ISL_TAG "screen on:ISL_prox_cur_infop->prox_data=%d\n",ISL_prox_cur_infop->prox_data);                     
	}   
   /*  if ((ret = (i2c_smbus_write_byte_data(ISL_datap->client, ISL_INT_REG, status))) < 0) 
	 {
   		printk(KERN_CRIT "ISL: i2c_smbus_write_byte failed in clear als interrupt\n");
	 }*/
	ISL_report_value(1);
    }
	
read_reg_fail:
	//clear intr  for als intr case, as prox intr  already be cleared in prox_poll	
     if ((ret = (i2c_smbus_write_byte_data(ISL_datap->client, ISL_INT_REG, status))) < 0) 
	 {
   		printk(KERN_CRIT "ISL: i2c_smbus_write_byte failed in clear als interrupt\n");
	 }
    enable_irq(ISL_datap->pdata->irq);
    return;
}	
		
/*calc_distance, using prox value*/
static int calc_distance(int value)
{
	int temp=0;
	if(ISL_prox_cur_infop->prox_event == 1)
		temp=0;
	else if(ISL_prox_cur_infop->prox_event == 0)
		temp=5;
	return temp;
}

/*report als and prox data to input sub system, 
for data poll utility in hardware\alsprox.c
*/
static void ISL_report_value(int mask)
{
	struct PS_ALS_prox_info *val = ISL_prox_cur_infop;
	int lux_val=ISL_g_nlux;
	int  dist;
	lux_val=(lux_val<=1)? lux_val:lux_val*10;	

	if (mask==0) {
		input_report_abs(light->input_dev, ABS_MISC, lux_val);
//[sensor wlg 20110729]ALS thereshold modify add log
 		printk(KERN_CRIT "ISL: als_interrupt lux_val(%d)=ISL_g_nlux(%d) * 9 \n", lux_val, ISL_g_nlux);
		input_sync(light->input_dev);
	}

	if (mask==1) {
		dist=calc_distance(val->prox_data);
		//input_report_als(alsprox->input_dev, ALS_PROX, val->prox_clear);
		input_report_abs(proximity->input_dev, ABS_DISTANCE, dist);
		//input_report_als(alsprox->input_dev, ALS_PROX, val->prox_event);
		printk(KERN_CRIT "ISL: prox_interrupt =%d, distance=%d\n",  val->prox_data,dist);
		input_sync(proximity->input_dev);
	}

	//input_sync(alsprox->input_dev);	
	//enable_irq(ISL_datap->pdata->irq);
}



/* ------------*
* driver init        *
*------------*/
static int __init ISL_init(void) {
	int ret = 0;
	//struct i2c_adapter *my_adap;
	printk(KERN_ERR "ISL: comes into ISL_init\n");
	if ((ret = (alloc_chrdev_region(&ISL_dev_number, 0, ISL_MAX_NUM_DEVICES, ISL_DEVICE_NAME))) < 0) {
		printk(KERN_ERR "ISL: alloc_chrdev_region() failed in ISL_init()\n");
                return (ret);
	}
        ISL_class = class_create(THIS_MODULE, ISL_DEVICE_NAME);
        ISL_datap = kmalloc(sizeof(struct ISL_data), GFP_KERNEL);
        if (!ISL_datap) {
		printk(KERN_ERR "ISL: kmalloc for struct ISL_data failed in ISL_init()\n");
                return -ENOMEM;
	}
        memset(ISL_datap, 0, sizeof(struct ISL_data));
        cdev_init(&ISL_datap->cdev, &ISL_fops);
        ISL_datap->cdev.owner = THIS_MODULE;
        if ((ret = (cdev_add(&ISL_datap->cdev, ISL_dev_number, 1))) < 0) {
		printk(KERN_ERR "ISL: cdev_add() failed in ISL_init()\n");
                return (ret);
	}
	//device_create(ISL_class, NULL, MKDEV(MAJOR(ISL_dev_number), 0), &ISL_driver ,"ISL");
        ret = i2c_add_driver(&ISL_driver);
	if(ret){
		printk(KERN_ERR "ISL: i2c_add_driver() failed in ISL_init(),%d\n",ret);
                return (ret);
	}
    	//pr_crit(ISL_TAG "%s:%d\n",__func__,ret);
        return (ret);
}



// driver exit
static void __exit ISL_exit(void) {
/*	if (ISL_my_clientp)
		i2c_unregister_device(ISL_my_clientp);
	*/
        i2c_del_driver(&ISL_driver);
        unregister_chrdev_region(ISL_dev_number, ISL_MAX_NUM_DEVICES);
	device_destroy(ISL_class, MKDEV(MAJOR(ISL_dev_number), 0));
	cdev_del(&ISL_datap->cdev);
	class_destroy(ISL_class);
	mutex_destroy(&ISL_datap->proximity_calibrating);
        kfree(ISL_datap);
}


//***************************************************
/*static struct file_operations ISL_device_fops = {
	.owner = THIS_MODULE,
	.open = ISL_open,
	.release = ISL_release,
	.ioctl = ISL_ioctl,
};


static struct miscdevice ISL_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "ISL",
	.fops = &ISL_device_fops,
};
*/

// client probe
static int ISL_probe(struct i2c_client *clientp, const struct i2c_device_id *idp) {
	int ret = 0;
	int i = 0;
	unsigned char buf[ISL_MAX_DEVICE_REGS];
	char *device_name;

//	if (device_found)
//		return -ENODEV;
	if (!i2c_check_functionality(clientp->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		printk(KERN_ERR "ISL: ISL_probe() - i2c smbus byte data functions unsupported\n");
		return -EOPNOTSUPP;
		}
    if (!i2c_check_functionality(clientp->adapter, I2C_FUNC_SMBUS_WORD_DATA)) {
		printk(KERN_ERR "ISL: ISL_probe() - i2c smbus word data functions unsupported\n");
        }
    if (!i2c_check_functionality(clientp->adapter, I2C_FUNC_SMBUS_BLOCK_DATA)) {
		printk(KERN_ERR "ISL: ISL_probe() - i2c smbus block data functions unsupported\n");
        }
	ISL_datap->client = clientp;
	i2c_set_clientdata(clientp, ISL_datap);
	
    for(i = 1; i < ISL_MAX_DEVICE_REGS; i++) {
		//buf[i] = i2c_smbus_read_byte_data(clientp, i);
		buf[i] = i2c_smbus_read_byte_data(ISL_datap->client, i);
		if(buf[i] < 0)
		{
			printk(KERN_ERR "ISL: i2c_smbus_read_byte_data(%d) failed in ISL_probe()\n",i);
		}
		else
		{
			//printk(KERN_ERR "ISL: i2c_smbus_read_byte_data(%d) = %d in ISL_probe()\n", i, buf[i]);
	}	
	}	
	//check device ID "tritonFN"
	if ((ret = ISL_device_name(buf, &device_name)) == 0) {
		printk(KERN_ERR "ISL: chip id that was read found mismatched by ISL_device_name(), in ISL_probe()\n");
 		return -ENODEV;
        }
	if (strcmp(device_name, ISL_DEVICE_ID)) {
		printk(KERN_ERR "ISL: chip id that was read does not match expected id in ISL_probe()\n");
		return -ENODEV;
        }
	else{
		printk(KERN_ERR "ISL: chip id of %s that was read matches expected id in ISL_probe()\n", device_name);
		//device_found = 1;
	}
	device_create(ISL_class, NULL, MKDEV(MAJOR(ISL_dev_number), 0), &ISL_driver ,"ISL");

	if ((ret = (i2c_smbus_write_byte_data(ISL_datap->client,ISL_ALSIR_TH_L,0) < 0))) {
		printk(KERN_ERR "ISL: i2c_smbus_write_byte() to control reg failed in ISL_probe()\n");
		return(ret);
        }
	if ((ret = (i2c_smbus_write_byte_data(ISL_datap->client,ISL_ALSIR_TH_HL,0) < 0))) {
		printk(KERN_ERR "ISL: i2c_smbus_write_byte() to control reg failed in ISL_probe()\n");
		return(ret);
        }
	if ((ret = (i2c_smbus_write_byte_data(ISL_datap->client,ISL_ALSIR_TH_H,0) < 0))) {
		printk(KERN_ERR "ISL: i2c_smbus_write_byte() to control reg failed in ISL_probe()\n");
		return(ret);
        }

	if ((ret = (i2c_smbus_write_byte_data(ISL_datap->client,ISL_CFG_REG,ISL_SENSOR_All_Off) < 0))) {
		printk(KERN_ERR "ISL: i2c_smbus_write_byte() to control reg failed in ISL_probe()\n");
		return(ret);
        }
	
	if ((ret = (i2c_smbus_write_byte_data(ISL_datap->client,ISL_INT_REG,ISL_INT_All_Off) < 0))) {
		printk(KERN_ERR "ISL: i2c_smbus_write_byte() to control reg failed in ISL_probe()\n");
		return(ret);
        }
	
	INIT_WORK(&ISL_datap->ISL_work, do_ISL_work); 
	mutex_init(&ISL_datap->proximity_calibrating);
	ISL_datap->pdata = &ISL_irq;
	if(clientp->irq){		
		ISL_datap->pdata->irq=ISL_datap->client->irq;
		ISL_datap->pdata->int_gpio=INT_TO_MSM_GPIO(ISL_datap->pdata->irq);
		}
	printk(KERN_CRIT "ISL use gpio %d\n",ISL_datap->pdata->int_gpio);
    	ret=ISL_config_int_gpio(ISL_datap->pdata->int_gpio);
    	if (ret) {
		printk(KERN_CRIT "ISL configure int_gpio%d failed\n",
                ISL_datap->pdata->int_gpio);
        	return ret;
    	}

    	ret = request_irq(ISL_datap->pdata->irq, ISL_interrupt, IRQF_TRIGGER_FALLING, 
			ISL_datap->ISL_name, ISL_prox_cur_infop);
    	if (ret) {
		printk(KERN_CRIT "ISL request interrupt failed\n");
        	return ret;
    	}
	
	strlcpy(clientp->name, ISL_DEVICE_ID, I2C_NAME_SIZE);
	strlcpy(ISL_datap->ISL_name, ISL_DEVICE_ID, ISL_ID_NAME_SIZE);
	ISL_datap->valid = 0;
	
	init_MUTEX(&ISL_datap->update_lock);
	if (!(ISL_cfgp = kmalloc(sizeof(struct PS_ALS_cfg), GFP_KERNEL))) {
		printk(KERN_ERR "ISL: kmalloc for struct PS_ALS_cfg failed in ISL_probe()\n");
		return -ENOMEM;
	}	

	//update settings
	//this should behind ISL_device_name	
	ISL_cfgp->calibrate_target = calibrate_target_param;
	ISL_cfgp->als_time = als_time_param;
	ISL_cfgp->scale_factor = scale_factor_param;
	ISL_cfgp->gain_trim = gain_trim_param;
	ISL_cfgp->filter_history = filter_history_param;
	ISL_cfgp->filter_count = filter_count_param;
	ISL_cfgp->gain = gain_param;
	ISL_cfgp->prox_threshold_hi = prox_threshold_hi_param;
	ISL_cfgp->prox_threshold_lo = prox_threshold_lo_param;
	ISL_cfgp->prox_int_time = prox_int_time_param;
	ISL_cfgp->prox_adc_time = prox_adc_time_param;
	ISL_cfgp->prox_wait_time = prox_wait_time_param;
	ISL_cfgp->prox_intr_filter = prox_intr_filter_param;
	ISL_cfgp->prox_config = prox_config_param;
	ISL_cfgp->prox_pulse_cnt = prox_pulse_cnt_param;
	ISL_cfgp->prox_gain = prox_gain_param;
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
		printk(KERN_ERR "ISL_probe: Failed to allocate light input device\n");
		goto exit_input_dev_alloc_failed;
	}
	proximity->input_dev = input_allocate_device();
	if (!proximity->input_dev) {
		ret = -ENOMEM;
		printk(KERN_ERR "ISL_probe: Failed to allocate prox input device\n");
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
		printk("ISL_probe: Unable to register input device: %s\n", light->input_dev->name);
		goto exit_input_register_device_failed;
	}
	ret = input_register_device(proximity->input_dev);
	if (ret) {
		printk("ISL_probe: Unable to register input device: %s\n", proximity->input_dev->name);
		goto exit_input_register_device_failed;
	}
	
	return (ret);
//exit_ISL_device_register_failed:	
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
static int __devexit ISL_remove(struct i2c_client *client) {
	int ret = 0;

	return (ret);
}

// open
static int ISL_open(struct inode *inode, struct file *file) {
	struct ISL_data *ISL_datap;
	int ret = 0;
	device_released = 0;
	ISL_datap = container_of(inode->i_cdev, struct ISL_data, cdev);
	if (strcmp(ISL_datap->ISL_name, ISL_DEVICE_ID) != 0) {
		printk(KERN_ERR "ISL: device name incorrect during ISL_open(), get %s\n", ISL_datap->ISL_name);
		ret = -ENODEV;
	}
	return (ret);
}

// release
static int ISL_release(struct inode *inode, struct file *file) {
	struct ISL_data *ISL_datap;
	int ret = 0;

	device_released = 1;
	prox_on = 0;
	//printk(KERN_CRIT "[ISL]:%s\n", __func__);

	//prox_history_hi = 0;
	//prox_history_hi = 0;
	ISL_datap = container_of(inode->i_cdev, struct ISL_data, cdev);
	if (strcmp(ISL_datap->ISL_name, ISL_DEVICE_ID) != 0) {
		printk(KERN_ERR "ISL: device name incorrect during ISL_release(), get %s\n", ISL_datap->ISL_name);
		ret = -ENODEV;
	}
	return (ret);
}

// read
static int ISL_read(struct file *file, char *buf, size_t count, loff_t *ppos) {
	struct ISL_data *ISL_datap;
	u8 i = 0, xfrd = 0, reg = 0;
	u8 my_buf[ISL_MAX_DEVICE_REGS];
	int ret = 0;

	//if (prox_on)
		//del_timer(&prox_poll_timer);
        if ((*ppos < 0) || (*ppos >= ISL_MAX_DEVICE_REGS)  || (count > ISL_MAX_DEVICE_REGS)) {
		printk(KERN_ERR "ISL: reg limit check failed in ISL_read()\n");
		return -EINVAL;
	}
	reg = (u8)*ppos;
	ISL_datap = container_of(file->f_dentry->d_inode->i_cdev, struct ISL_data, cdev);
	while (xfrd < count) {
      		my_buf[i++] = i2c_smbus_read_byte_data(ISL_datap->client,reg);
			reg++;
			xfrd++;
        }
        if ((ret = copy_to_user(buf, my_buf, xfrd))){
		printk(KERN_ERR "ISL: copy_to_user failed in ISL_read()\n");
		return -ENODATA;
	}
	//if (prox_on)
		//ISL_prox_poll_timer_start();
        return ((int)xfrd);
}

// write
static int ISL_write(struct file *file, const char *buf, size_t count, loff_t *ppos) {
	struct ISL_data *ISL_datap;
	u8 i = 0, xfrd = 0, reg = 0;
	u8 my_buf[ISL_MAX_DEVICE_REGS];
	int ret = 0;

	//if (prox_on)
		//del_timer(&prox_poll_timer);
        if ((*ppos < 0) || (*ppos >= ISL_MAX_DEVICE_REGS) || ((*ppos + count) > ISL_MAX_DEVICE_REGS)) {
		printk(KERN_ERR "ISL: reg limit check failed in ISL_write()\n");
		return -EINVAL;
	}
	reg = (u8)*ppos;
        if ((ret =  copy_from_user(my_buf, buf, count))) {
		printk(KERN_ERR "ISL: copy_to_user failed in ISL_write()\n");
 		return -ENODATA;
	}
        ISL_datap = container_of(file->f_dentry->d_inode->i_cdev, struct ISL_data, cdev);
        while (xfrd < count) {
                if ((ret = (i2c_smbus_write_byte_data(ISL_datap->client, reg, my_buf[i++]))) < 0) {
                        printk(KERN_ERR "ISL: i2c_smbus_write_byte_data failed in ISL_write()\n");
                        return (ret);
                }
                reg++;
                xfrd++;
        }
	//if (prox_on)
		//ISL_prox_poll_timer_start();
        return ((int)xfrd);
}

// llseek
static loff_t ISL_llseek(struct file *file, loff_t offset, int orig) {
	int ret = 0;
	loff_t new_pos = 0;

	//if (prox_on)
		//del_timer(&prox_poll_timer);
	if ((offset >= ISL_MAX_DEVICE_REGS) || (orig < 0) || (orig > 1)) {
                printk(KERN_ERR "ISL: offset param limit or origin limit check failed in ISL_llseek()\n");
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
	if ((new_pos < 0) || (new_pos >= ISL_MAX_DEVICE_REGS) || (ret < 0)) {
		printk(KERN_ERR "ISL: new offset limit or origin limit check failed in ISL_llseek()\n");
		return -EINVAL;
	}
	file->f_pos = new_pos;
       // if (prox_on)
              //  ISL_prox_poll_timer_start();
	return new_pos;
}

/*enable_light_and_proximity, mask values' indication*/
/*10 : light on*/
/*01 : prox on*/
/*20 : light off*/
/*02 : prox off*/
static int enable_light_and_proximity(int mask)
{
	u8 ret = 0, reg_val = 0; //itime = 0;

	if(mask==0x10) /*10 : light on*/
	{
		pr_crit(ISL_TAG "light on\n");
		if ((reg_val = (i2c_smbus_read_byte_data(ISL_datap->client,ISL_INT_REG))) < 0) {
			printk(KERN_ERR "ISL: i2c_smbus_read_byte_data failed in ioctl prox_on\n");
                        return (reg_val);
		}
		reg_val = (reg_val & (~ISL_ALS_INT_mask)) | ISL_ALS_INT_On;
		if ((ret = (i2c_smbus_write_byte_data(ISL_datap->client, ISL_INT_REG, reg_val))) < 0) {
			printk(KERN_ERR "ISL: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
                        return (ret);
		}
		
		reg_val = als_intr_threshold_lo_param & 0x00FF;
		if ((ret = (i2c_smbus_write_byte_data(ISL_datap->client, ISL_ALSIR_TH_L, reg_val))) < 0) {
			printk(KERN_ERR "ISL: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
                        return (ret);
		}
		reg_val = ((als_intr_threshold_lo_param >> 8) & 0x000F) | ((als_intr_threshold_hi_param << 4) & 0x00F0) ;			
		if ((ret = (i2c_smbus_write_byte_data(ISL_datap->client, ISL_ALSIR_TH_HL, reg_val))) < 0) {
			printk(KERN_ERR "ISL: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
                        return (ret);
		}	
		reg_val = (als_intr_threshold_hi_param >> 4) & 0x00FF;			
		if ((ret = (i2c_smbus_write_byte_data(ISL_datap->client, ISL_ALSIR_TH_H, reg_val))) < 0) {
			printk(KERN_ERR "ISL: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
                        return (ret);
		}
		
		if ((reg_val = (i2c_smbus_read_byte_data(ISL_datap->client,ISL_CFG_REG))) < 0) {
			printk(KERN_ERR "ISL: i2c_smbus_read_byte_data failed in ioctl prox_on\n");
                	return (reg_val);
		}
		reg_val = (reg_val & (~ISL_ALS_mask)) | ISL_ALS_On;
		if ((ret = (i2c_smbus_write_byte_data(ISL_datap->client, ISL_CFG_REG, reg_val))) < 0) {
			printk(KERN_ERR "ISL: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
        	        return (ret);
		}
	}	
	if(mask==0x01) /*01 : prox on*/
	{
		pr_crit(ISL_TAG "prox on\n");
	    if ((reg_val = (i2c_smbus_read_byte_data(ISL_datap->client,ISL_INT_REG))) < 0) {
			printk(KERN_ERR "ISL: i2c_smbus_read_byte_data failed in ioctl prox_on\n");
            return (reg_val);
		}
		reg_val = (reg_val & (~(ISL_PROX_INT_mask ))) | ISL_PROX_INT_On;
		if ((ret = (i2c_smbus_write_byte_data(ISL_datap->client, ISL_INT_REG, reg_val))) < 0) {
            printk(KERN_ERR "ISL: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
            return (ret);
		}

//		reg_val = ISL_cfgp->prox_threshold_lo & 0x00FF;
		if ((ret = (i2c_smbus_write_byte_data(ISL_datap->client, ISL_PROX_TH_L, 0x00))) < 0) {
           printk(KERN_ERR "ISL: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
           return (ret);
		}
		reg_val = ISL_cfgp->prox_threshold_hi & 0x00FF;			
		if ((ret = (i2c_smbus_write_byte_data(ISL_datap->client, ISL_PROX_TH_H, reg_val))) < 0) {
            printk(KERN_ERR "ISL: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
            return (ret);
		}			 

		if ((reg_val = (i2c_smbus_read_byte_data(ISL_datap->client,ISL_CFG_REG))) < 0) {
			printk(KERN_ERR "ISL: i2c_smbus_read_byte_data failed in ioctl prox_on\n");
      		return (reg_val);
		}
		reg_val = (reg_val & (~ISL_PROX_mask)) | ISL_PROX_On;
		if ((ret = (i2c_smbus_write_byte_data(ISL_datap->client, ISL_CFG_REG, reg_val))) < 0) {
			printk(KERN_ERR "ISL: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
            return (ret);
		}
	}	
	if(mask==0x20) /*20 : light off*/
	{
		pr_crit(ISL_TAG "light off\n");
		reg_val = 0x00;
		if ((ret = (i2c_smbus_write_byte_data(ISL_datap->client, ISL_ALSIR_TH_L, reg_val))) < 0) {
			printk(KERN_ERR "ISL: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
            return (ret);
        }
		reg_val = 0xF0;
		if ((ret = (i2c_smbus_write_byte_data(ISL_datap->client, ISL_ALSIR_TH_HL, reg_val))) < 0) {
			printk(KERN_ERR "ISL: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
            return (ret);
		}
		reg_val = 0xFF;			
		if ((ret = (i2c_smbus_write_byte_data(ISL_datap->client, ISL_ALSIR_TH_H, reg_val))) < 0) {
			printk(KERN_ERR "ISL: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
            return (ret);
		}

		if ((reg_val = (i2c_smbus_read_byte_data(ISL_datap->client,ISL_INT_REG))) < 0) {
			printk(KERN_ERR "ISL: i2c_smbus_read_byte_data failed in ioctl prox_on\n");
            return (reg_val);
		}
		reg_val = (reg_val & (~ISL_ALS_INT_mask));
		if ((ret = (i2c_smbus_write_byte_data(ISL_datap->client, ISL_INT_REG, reg_val))) < 0) {
			printk(KERN_ERR "ISL: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
            return (ret);
		}

		if ((reg_val = (i2c_smbus_read_byte_data(ISL_datap->client,ISL_CFG_REG))) < 0) {
			printk(KERN_ERR "ISL: i2c_smbus_read_byte_data failed in ioctl prox_on\n");
            return (reg_val);
		}
		reg_val = (reg_val & (~ISL_ALS_mask)) ;
		if ((ret = (i2c_smbus_write_byte_data(ISL_datap->client, ISL_CFG_REG, reg_val))) < 0) {
			printk(KERN_ERR "ISL: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
            return (ret);
        }
	}
	if(mask==0x02) /*02 : prox off*/
	{
		pr_crit(ISL_TAG "prox off\n");
		if ((reg_val = (i2c_smbus_read_byte_data(ISL_datap->client,ISL_CFG_REG))) < 0) {
			printk(KERN_ERR "ISL: i2c_smbus_read_byte_data failed in ioctl prox_on\n");
            return (reg_val);
		}
		reg_val = (reg_val & (~ISL_PROX_mask));
		if ((ret = (i2c_smbus_write_byte_data(ISL_datap->client, ISL_CFG_REG, reg_val))) < 0) {
			printk(KERN_ERR "ISL: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
            return (ret);
		}

		if ((reg_val = (i2c_smbus_read_byte_data(ISL_datap->client,ISL_INT_REG))) < 0) {
			printk(KERN_ERR "ISL: i2c_smbus_read_byte_data failed in ioctl prox_on\n");
            return (reg_val);
        }
		reg_val = (reg_val & (~ISL_PROX_INT_mask));
		if ((ret = (i2c_smbus_write_byte_data(ISL_datap->client, ISL_INT_REG, reg_val))) < 0) {
			printk(KERN_ERR "ISL: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
            return (ret);
        }
	}
	return ret;
}

// ioctls
static int ISL_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg) {
	struct ISL_data *ISL_datap;
	int prox_sum = 0, prox_mean = 0, prox_max = 0;
	int lux_val = 0, ret = 0, i = 0;
	u8 reg_val = 0;
	u16 ratio;
	struct ISL_prox_data *prox_pt;

	ISL_datap = container_of(inode->i_cdev, struct ISL_data, cdev);
//[sensor wlg 20110729]ALS thereshold modify add log
//printk(KERN_ERR "ISL_wlg: ISL_ioctl() cmd=%d\n", cmd);
	switch (cmd) {
		case PS_ALS_IOCTL_ALS_ON:
			if(light_on)
			{
			    pr_crit(ISL_TAG "PS_ALS_IOCTL_ALS_ON: light already on.\n");
			    break;
			}
			ret=enable_light_and_proximity(0x10);
			if(ret>=0)
			{
				light_on=1;	
      			pr_crit(ISL_TAG "PS_ALS_IOCTL_ALS_ON,lux=%d\n", ISL_g_nlux);
			}			
			return (ret);
			break;
        case PS_ALS_IOCTL_ALS_OFF:	
			if(light_on == 0)
			{
			    pr_crit(ISL_TAG "PS_ALS_IOCTL_ALS_OFF: light already off.\n");
			    break;
			}

            for (i = 0; i < ISL_FILTER_DEPTH; i++)
                lux_history[i] = -ENODATA;
			ret=enable_light_and_proximity(0x20);
			if(ret>=0)
			{
				light_on=0;
				//ISL_g_nlux=0;
            	pr_crit(ISL_TAG"PS_ALS_IOCTL_ALS_OFF\n");
			}			
			return (ret);
            break;
		case PS_ALS_IOCTL_ALS_DATA:
			if(light_on == 0)
			{
			    pr_crit(ISL_TAG "PS_ALS_IOCTL_ALS_DATA: read als data when light off.\n");
			}

    		reg_val = i2c_smbus_read_byte_data(ISL_datap->client,ISL_ALSIR_DATA_L);
			if(reg_val < 0)
			{
				return ret;
            }
			ret = reg_val;
			reg_val = i2c_smbus_read_byte_data(ISL_datap->client,ISL_ALSIR_DATA_H);
			if(reg_val < 0)
			{
				return reg_val;
            }
			lux_val = (ret & 0xf0) + ((reg_val & 0xf) << 8);
			return lux_val;
			break;
		case PS_ALS_IOCTL_ALS_CALIBRATE:

			break;
			
		case PS_ALS_IOCTL_CONFIG_GET:
			ret = copy_to_user((struct PS_ALS_cfg *)arg, ISL_cfgp, sizeof(struct PS_ALS_cfg));
			if (ret) {
				printk(KERN_ERR "ISL: copy_to_user failed in ioctl config_get\n");
				return -ENODATA;
			}
			return (ret);
			break;
        case PS_ALS_IOCTL_CONFIG_SET:
            ret = copy_from_user(ISL_cfgp, (struct PS_ALS_cfg *)arg, sizeof(struct PS_ALS_cfg));
			if (ret) {
				printk(KERN_ERR "ISL: copy_from_user failed in ioctl config_set\n");
                return -ENODATA;
			}
  			return (ret);
            break;
		case PS_ALS_IOCTL_PROX_ON:				
			if(prox_on)
			{
			    pr_crit(ISL_TAG "PS_ALS_IOCTL_PROX_ON: prox already on.\n");
				break;
			}

    		ISL_prox_cur_infop->prox_event = 0;
            ISL_prox_cur_infop->prox_clear = 0;
            ISL_prox_cur_infop->prox_data = 0;
			ret=enable_light_and_proximity(0x01);
			if(ret>=0)
			{
				prox_on = 1;	
            	pr_crit(ISL_TAG "PS_ALS_IOCTL_PROX_ON\n");
			}	
			return ret;	
			break;
        case PS_ALS_IOCTL_PROX_OFF:
			if(prox_on == 0)
			{
			    pr_crit(ISL_TAG "PS_ALS_IOCTL_PROX_OFF: prox already off.\n");
				break;
			}

    		ret=enable_light_and_proximity(0x02);
			if(ret>=0)
			{
				prox_on = 0;	
            	pr_crit(ISL_TAG"PS_ALS_IOCTL_PROX_OFF\n");
			}	
			return ret;	
			break;
		case PS_ALS_IOCTL_PROX_DATA:
            ret = copy_to_user((struct PS_ALS_prox_info *)arg, ISL_prox_cur_infop, sizeof(struct PS_ALS_prox_info));
            if (ret) {
                     printk(KERN_ERR "ISL: copy_to_user failed in ioctl prox_data\n");
                     return -ENODATA;
            }
            return (ret);
            break;
        case PS_ALS_IOCTL_PROX_EVENT:
 			return (ISL_prox_cur_infop->prox_event);
            break;
		case PS_ALS_IOCTL_PROX_CALIBRATE:
			if (!prox_on)			
			 {
				printk(KERN_ERR "ISL: ioctl prox_calibrate was called before ioctl prox_on was called\n");
				return -EPERM;
			}
			mutex_lock(&ISL_datap->proximity_calibrating);
				prox_sum = 0;
				prox_max = 0;
				for (i = 0; i < 20; i++) {
	                        if ((ret = ISL_prox_poll(&ISL_prox_cal_info[i])) < 0) {
        	                        printk(KERN_ERR "ISL: call to prox_poll failed in ioctl prox_calibrate\n");
						mutex_unlock(&ISL_datap->proximity_calibrating);			
                	                return (ret);
                        		}					
				pr_crit(ISL_TAG "prox calibrate poll prox[%d] = %d\n",i,ISL_prox_cal_info[i].prox_data); 			
					prox_sum += ISL_prox_cal_info[i].prox_data;
					if (ISL_prox_cal_info[i].prox_data > prox_max)
						prox_max = ISL_prox_cal_info[i].prox_data;
				mdelay(100);
				}
				prox_mean = prox_sum/20;
			ratio = 10*prox_mean/sat_prox;
			for (prox_pt = ISL_prox_tablep; prox_pt->ratio && prox_pt->ratio <= ratio; prox_pt++);
        		if(!prox_pt->ratio)
                	return -1;
			 
			ISL_cfgp->prox_threshold_hi = (prox_mean*prox_pt->hi)/10;
			if (ISL_cfgp->prox_threshold_hi <= ((sat_prox*3)/100)) {				
				ISL_cfgp->prox_threshold_hi = ((sat_prox*8)/100);
			}
			ISL_cfgp->prox_threshold_lo = ISL_cfgp->prox_threshold_hi*THRES_LO_TO_HI_RATIO;								
			//write back threshold
			reg_val = 0;//ISL_cfgp->prox_threshold_lo & 0x00FF;
			if ((ret = (i2c_smbus_write_byte_data(ISL_datap->client, ISL_PROX_TH_L, reg_val))) < 0) {
				printk(KERN_ERR "ISL: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
                		return (ret);
            		}
			reg_val = ISL_cfgp->prox_threshold_hi & 0x00FF;			
			if ((ret = (i2c_smbus_write_byte_data(ISL_datap->client, ISL_PROX_TH_H, reg_val))) < 0) {
				printk(KERN_ERR "ISL: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
                		return (ret);
            		}
			mutex_unlock(&ISL_datap->proximity_calibrating);	
			pr_crit(KERN_ERR "ISL prox_cal_threshold_hi=%d,prox_cal_threshold_lo=%d\n",ISL_cfgp->prox_threshold_hi,ISL_cfgp->prox_threshold_lo);
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
static int ISL_get_lux(void) 
{
        u16 raw_clear = 0, reg_val=0;
	u32 lux = 0;
//	u32 ratio = 0;
//	u8 dev_gain = 0;
//	struct lux_data *p;
	int ret = 0;
	u8 chdata[2];
	int i = 0;
    	pr_crit(ISL_TAG "ISL start to calc lux value\n");

	for (i = 0; i < 2; i++) {
       	chdata[i] = i2c_smbus_read_byte_data(ISL_datap->client,ISL_ALSIR_DATA_L+i);
	}
	
	raw_clear = ((chdata[1]&0xF)<< 8)|chdata[0];

	als_intr_threshold_hi_param = raw_clear + raw_clear/5;
    als_intr_threshold_lo_param = raw_clear - raw_clear/5;

	//update threshold 
    	printk(ISL_TAG "als_intr_threshold_hi_param=%x,als_intr_threshold_lo_param=%x\n",als_intr_threshold_hi_param,als_intr_threshold_lo_param);
			reg_val = als_intr_threshold_lo_param & 0x00FF;
    	//printk(ISL_TAG "ISL_ALSIR_TH_L = %x\n",reg_val);
			if ((ret = (i2c_smbus_write_byte_data(ISL_datap->client, ISL_ALSIR_TH_L, reg_val))) < 0) {
				printk(KERN_ERR "ISL: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
                return (ret);
			}
			reg_val = ((als_intr_threshold_lo_param >> 8) & 0x000F) | ((als_intr_threshold_hi_param << 4) & 0x00F0) ;			
    	//printk(ISL_TAG "ISL_ALSIR_TH_HL = %x\n",reg_val);
			if ((ret = (i2c_smbus_write_byte_data(ISL_datap->client, ISL_ALSIR_TH_HL, reg_val))) < 0) {
				printk(KERN_ERR "ISL: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
                return (ret);
        	}
			reg_val = (als_intr_threshold_hi_param >> 4) & 0x00FF;			
    	//printk(ISL_TAG "ISL_ALSIR_TH_H = %x\n",reg_val);
			if ((ret = (i2c_smbus_write_byte_data(ISL_datap->client, ISL_ALSIR_TH_H, reg_val))) < 0) {
				printk(KERN_ERR "ISL: i2c_smbus_write_byte_data failed in ioctl prox_on\n");
                return (ret);
        	}
	
	lux = raw_clear;
	return(lux);
}


// verify device
static int ISL_device_name(unsigned char *bufp, char **device_name) {
		ISL_datap->chip_type=TMD2771;
/*		if( (bufp[0x12]&0xf0)!=0x00)
			return(0);
		if(bufp[0x10]|bufp[0x1a]|bufp[0x1b]|bufp[0x1c]|bufp[0x1d]|bufp[0x1e])
			return(0);
  		if(bufp[0x13]&0x0c)
			return(0);
*/			
		*device_name="isl29026";
		return(1);
}

// proximity poll
static int ISL_prox_poll(struct PS_ALS_prox_info *prxp) {
	u8 prox_data;

	prox_data = i2c_smbus_read_byte_data(ISL_datap->client,ISL_PROX_DATA);   
	prxp->prox_data = prox_data;

	return (prox_data);
	}
// start prox poll timer
/*
static void ISL_prox_poll_timer_start(void) {
        init_timer(&prox_poll_timer);
        prox_poll_timer.expires = jiffies + (HZ/10);
        prox_poll_timer.function = ISL_prox_poll_timer_func;
        add_timer(&prox_poll_timer);

        return;
}*/


MODULE_AUTHOR("ZTE Software");
MODULE_DESCRIPTION("ISL ambient light and proximity sensor driver");
MODULE_LICENSE("GPL");

module_init(ISL_init);
module_exit(ISL_exit);

