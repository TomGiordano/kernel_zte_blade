/*============================================================*/
/*                            (Ambient Light Sensor)bh1721.c                                                        */
/*============================================================*/

/*******************************************************************
Geschichte:
Wenn               Wer          Was                                                                        Tag
2010-08-13    wangzy     create for bh1721 ambient light sensor     


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
#include <linux/i2c/bh1721.h>
#include <linux/input.h>
#include <linux/miscdevice.h>
#include <linux/workqueue.h>

#define ALS_TAG    "[als]"

#define DEBUG 0

//device name/id/address
#define ALS_DEVICE_NAME 		"taos"
#define ALS_DEVICE_ID       		"bh1721"
#define ALS_ID_NAME_SIZE		10
#define ALS_DEVICE_ADDR1		0x23<<1
#define ALS_MAX_DEVICE_REGS		32
#define ALS_MAX_NUM_DEVICES     3

//device command
#define ALS_CMD_PWN_OFF   0x00
#define ALS_CMD_PWN_ON    0x01
#define ALS_CMD_RESET    0x07
#define ALS_CMD_HRES     0x12
#define ALS_CMD_LRES     0x13
#define ALS_CMD_MEASURE_HI   0x49
#define ALS_CMD_MEASURE_LO  0x6C  //set measurement time 100%	

//overflow times before every input event
#define ALS_LUX_INT_FILTER   2 

// lux constants
//#define TAOS_MAX_LUX			65535000
//#define TAOS_SCALE_MILLILUX		3
#define TAOS_FILTER_DEPTH		3

// forward declarations
static int als_probe(struct i2c_client *clientp, const struct i2c_device_id *idp);
static int als_remove(struct i2c_client *client);
static int als_open(struct inode *inode, struct file *file);
static int als_release(struct inode *inode, struct file *file);
static int als_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);
static int als_read(struct file *file, char *buf, size_t count, loff_t *ppos);
static int als_write(struct file *file, const char *buf, size_t count, loff_t *ppos);
static loff_t als_llseek(struct file *file, loff_t offset, int orig);
static void als_add_lux_filter(int lux);
static int als_get_lux_filter(void);
static int als_lux_poll(void);
static void do_als_work(struct work_struct *w);
static void als_lux_poll_timer_func(unsigned long param);
static void als_lux_poll_timer_start(void);
static void als_report_value(int val);
static int bh1721_regs_read(struct i2c_client * client, char * buf, int count);
static int bh1721_regs_write(struct i2c_client * client,const char * buf, int count);
//input data structure
struct alsprox_data {
	struct input_dev *input_dev;
};

static struct alsprox_data *light;

// first device number
static dev_t als_dev;

// class structure for this device
struct class *als_class;

// module device table
static struct i2c_device_id als_idtable[] = {
        {ALS_DEVICE_ID, 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, als_idtable);


// driver definition
static struct i2c_driver als_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "bh1721",
	 },
	.id_table = als_idtable,
	.probe = als_probe,
	.remove = __devexit_p(als_remove),
};

// per-device data
struct als_data {
	struct i2c_client *client;
	struct cdev cdev;
	unsigned int addr;
	char als_id;
	char als_name[ALS_ID_NAME_SIZE];
	struct semaphore update_lock;
	char valid;
	struct workqueue_struct *workqueue;
	struct delayed_work als_work;
} *als_datap;


// file operations
static struct file_operations als_fops = {
	.owner = THIS_MODULE,
	.open = als_open,
	.release = als_release,
	.read = als_read,
	.write = als_write,
	.llseek = als_llseek,
	.ioctl = als_ioctl,
};
	
//static values	 declaration
static struct timer_list lux_poll_timer;
struct als_cfg *als_cfgp;
static int light_on=0;  
static int thres_cont=0;
static int glux=0;
static int device_released = 0;
static int lux_history[TAOS_FILTER_DEPTH] = {-ENODATA, -ENODATA, -ENODATA};
u8 als_reg_cmd[6] = {ALS_CMD_PWN_OFF,ALS_CMD_PWN_ON,ALS_CMD_RESET,ALS_CMD_HRES,ALS_CMD_MEASURE_HI,ALS_CMD_MEASURE_LO};




/**********************************************************/
/*driver init------------------------------------------------*/
static int __init als_init(void) {
	int ret = 0;	
	printk(KERN_ERR "BH1721: comes into als_init\n");
	
	if ((ret = (alloc_chrdev_region(&als_dev, 0, ALS_MAX_NUM_DEVICES, ALS_DEVICE_NAME))) < 0) {
		printk(KERN_ERR "BH1721: alloc_chrdev_region() failed in bh1721_init()\n");
                return (ret);
	}
	
    	als_class = class_create(THIS_MODULE, ALS_DEVICE_NAME);
    	als_datap = kmalloc(sizeof(struct als_data), GFP_KERNEL);
    	if (!als_datap){
		printk(KERN_ERR "BH1721: kmalloc for struct als_data failed in als_init()\n");
       	return -ENOMEM;
	}
    	memset(als_datap, 0, sizeof(struct als_data));
    	cdev_init(&als_datap->cdev, &als_fops);
    	als_datap->cdev.owner = THIS_MODULE;
   	if ((ret = (cdev_add(&als_datap->cdev, als_dev, 1))) < 0) {
		printk(KERN_ERR "BH1721: cdev_add() failed in als_init()\n");
        	return (ret);
	}
	device_create(als_class, NULL, MKDEV(MAJOR(als_dev), 0), &als_driver ,"taos");
	
    	ret = i2c_add_driver(&als_driver);
	if(ret){
		printk(KERN_ERR "BH1721: i2c_add_driver() failed in als_init(),%d\n",ret);
        return (ret);
	}

	/*als workqueue for i2c read*/
	/*ZTE_BH7121_001 start*/
	als_datap->workqueue = create_singlethread_workqueue("als_workqueue");
	if (NULL == als_datap->workqueue) 
	{
		printk(KERN_ERR "bh1721: workqueue created fail");
		return -1;
	}
	INIT_DELAYED_WORK(&als_datap->als_work, do_als_work);
	init_timer(&lux_poll_timer);
	/*ZTE_BH7121_001 end*/
    	return (ret);
}


// driver exit---------------------------------------------------------------
static void __exit als_exit(void) {
   	i2c_del_driver(&als_driver);
    	unregister_chrdev_region(als_dev, ALS_MAX_NUM_DEVICES);
	device_destroy(als_class, MKDEV(MAJOR(als_dev), 0));
	cdev_del(&als_datap->cdev);
	class_destroy(als_class);
   	kfree(als_datap);
}

/******************************************************/
/*als_probe                                                                                        */
/******************************************************/
static int als_probe(struct i2c_client *clientp, const struct i2c_device_id *idp) {
	int ret = 0;
	//int i = 0;
	//unsigned char buf[ALS_MAX_DEVICE_REGS];
	//char *device_name;
	printk(KERN_ERR "BH1721: als_probe()\n");
	if (!i2c_check_functionality(clientp->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		printk(KERN_ERR "BH1721: taos_probe() - i2c smbus byte data functions unsupported\n");
		return -EOPNOTSUPP;
		}
	als_datap->client = clientp;
	i2c_set_clientdata(clientp, als_datap);
	
	if((ret = (i2c_smbus_write_byte(clientp, ALS_CMD_PWN_OFF))) < 0) {
		printk(KERN_ERR "BH1721: i2c_smbus_write_byte() to reg failed in als_probe()\n");
		return(ret);
        }
	
	strlcpy(clientp->name, ALS_DEVICE_ID, I2C_NAME_SIZE);
	strlcpy(als_datap->als_name, ALS_DEVICE_ID, ALS_ID_NAME_SIZE);
	als_datap->valid = 0;
	
	if (!(als_cfgp = kmalloc(sizeof(struct als_cfg), GFP_KERNEL))) {
		printk(KERN_ERR "BH1721: kmalloc for struct als_cfg failed in als_probe()\n");
		return -ENOMEM;
	}

	light = kzalloc(sizeof(struct alsprox_data), GFP_KERNEL);
	if (!light) {
		ret = -ENOMEM;
		goto exit_alloc_data_failed;
	}

	light->input_dev = input_allocate_device();
	if (!light->input_dev) {
		ret = -ENOMEM;
		printk(KERN_ERR "als_probe: Failed to allocate input device\n");
		goto exit_input_dev_alloc_failed;
	}

	/* lux */
	set_bit(EV_ABS, light->input_dev->evbit);
	input_set_abs_params(light->input_dev,  ABS_MISC, 0, 65535, 0, 0);

	light->input_dev->name = "light";
	ret = input_register_device(light->input_dev);
	if (ret) {
		printk("als_probe: Unable to register input device: %s\n", light->input_dev->name);
		goto exit_input_register_device_failed;
	}	
	return (ret);
	
exit_input_register_device_failed:
	input_free_device(light->input_dev);
	
exit_input_dev_alloc_failed:
exit_alloc_data_failed:
	kfree(light);
	return (ret);		
}


// client remove---------------------------------------------------
static int __devexit als_remove(struct i2c_client *client) {
	int ret = 0;
	return (ret);
}

// open-----------------------------------------------------------
static int als_open(struct inode *inode, struct file *file) {
	struct als_data *als_datap;
	int ret = 0;

	device_released = 0;
    	printk(KERN_CRIT "[als]:%s\n", __func__);
	als_datap = container_of(inode->i_cdev, struct als_data, cdev);
	if (strcmp(als_datap->als_name, ALS_DEVICE_ID) != 0) {
		printk(KERN_ERR "BH1721: device name incorrect during als_open(), get %s\n", als_datap->als_name);
		ret = -ENODEV;
	}
	return (ret);
}

// release------------------------------------------------------------
static int als_release(struct inode *inode, struct file *file) {
	struct als_data *als_datap;
	int ret = 0;

	device_released = 1;
	printk(KERN_CRIT "[als]:%s\n", __func__);

	als_datap = container_of(inode->i_cdev, struct als_data, cdev);
	if (strcmp(als_datap->als_name, ALS_DEVICE_ID) != 0) {
		printk(KERN_ERR "BH1721: device name incorrect during taos_release(), get %s\n", als_datap->als_name);
		ret = -ENODEV;
	}
	return (ret);
}

/******************************************************/
/*read and write bh1721 chip need no offset, so modify it when use*/
/******************************************************/
// read-----------------------------------------------------------------------------
static int als_read(struct file *file, char *buf, size_t count, loff_t *ppos) {
	struct als_data *als_datap;
	u8 i = 0, xfrd = 0, reg = 0;
	u8 my_buf[ALS_MAX_DEVICE_REGS];
	int ret = 0;

    if ((*ppos < 0) || (*ppos >= ALS_MAX_DEVICE_REGS)  || (count > ALS_MAX_DEVICE_REGS)) {
		printk(KERN_ERR "BH1721: reg limit check failed in als_read()\n");
		return -EINVAL;
	}
	reg = (u8)*ppos;
	als_datap = container_of(file->f_dentry->d_inode->i_cdev, struct als_data, cdev);
	while (xfrd < count) {
			if ((ret = (i2c_smbus_write_byte(als_datap->client, (reg)))) < 0) {
			printk(KERN_ERR "BH1721: i2c_smbus_write_byte failed in als_read()\n");
			return (ret);
			}
      		my_buf[i++] = i2c_smbus_read_byte(als_datap->client);
			reg++;
			xfrd++;
        }
        if ((ret = copy_to_user(buf, my_buf, xfrd))){
		printk(KERN_ERR "BH1721: copy_to_user failed in als_read()\n");
		return -ENODATA;
	}
	return ((int)xfrd);
}

// write---------------------------------------------------------------------------------------
static int als_write(struct file *file, const char *buf, size_t count, loff_t *ppos) {
	struct als_data *als_datap;
	u8 i = 0, xfrd = 0, reg = 0;
	u8 my_buf[ALS_MAX_DEVICE_REGS];
	int ret = 0;

    if ((*ppos < 0) || (*ppos >= ALS_MAX_DEVICE_REGS) || ((*ppos + count) > ALS_MAX_DEVICE_REGS)) {
		printk(KERN_ERR "BH1721: reg limit check failed in als_write()\n");
		return -EINVAL;
	}
	reg = (u8)*ppos;
        if ((ret =  copy_from_user(my_buf, buf, count))) {
		printk(KERN_ERR "BH1721: copy_to_user failed in als_write()\n");
 		return -ENODATA;
	}
    als_datap = container_of(file->f_dentry->d_inode->i_cdev, struct als_data, cdev);
        while (xfrd < count) {
                if ((ret = (i2c_smbus_write_byte_data(als_datap->client, (reg), my_buf[i++]))) < 0) {
                        printk(KERN_ERR "BH1721: i2c_smbus_write_byte_data failed in als_write()\n");
                        return (ret);
                }
                reg++;
                xfrd++;
        }
        return ((int)xfrd);
}

// llseek---------------------------------------------------------------------
static loff_t als_llseek(struct file *file, loff_t offset, int orig) {
	int ret = 0;
	loff_t new_pos = 0;

	if ((offset >= ALS_MAX_DEVICE_REGS) || (orig < 0) || (orig > 1)) {
                printk(KERN_ERR "BH1721: offset param limit or origin limit check failed in als_llseek()\n");
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
	if ((new_pos < 0) || (new_pos >= ALS_MAX_DEVICE_REGS) || (ret < 0)) {
		printk(KERN_ERR "BH1721: new offset limit or origin limit check failed in als_llseek()\n");
		return -EINVAL;
	}
	file->f_pos = new_pos;

	return new_pos;
}

/*******************************************/
/*als_ioctl*/
/*******************************************/
static int als_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg) {
	struct als_data *als_datap;
	int lux_val = 0, ret = 0, i = 0;
	u16 gain_val = 0;

	als_datap = container_of(inode->i_cdev, struct als_data, cdev);
	switch(cmd) 
	{		
		case ALS_IOCTL_PW_ON:			
			for (i = 0; i < TAOS_FILTER_DEPTH; i++)
				lux_history[i] = -ENODATA;
                        if ((ret = (bh1721_regs_write(als_datap->client, &als_reg_cmd[1],1))) < 0) {
                                printk(KERN_ERR "BH1721: i2c_smbus_write_byte failed in ioctl als_on\n");
                                return (ret);
                        }
                        if ((ret = (bh1721_regs_write(als_datap->client, &als_reg_cmd[4],1))) < 0) {
                                printk(KERN_ERR "BH1721: i2c_smbus_write_byte failed in ioctl als_on\n");
                                return (ret);
                        }
			if ((ret = (bh1721_regs_write(als_datap->client, &als_reg_cmd[5],1))) < 0) {
                                printk(KERN_ERR "BH1721: i2c_smbus_write_byte failed in ioctl als_on\n");
                                return (ret);
			}			
                        if ((ret = (bh1721_regs_write(als_datap->client, &als_reg_cmd[3],1))) < 0) {
                                printk(KERN_ERR "BH1721: i2c_smbus_write_byte failed in ioctl als_on\n");
                                return (ret);
                        }
			msleep(120);			
                        als_lux_poll_timer_start();
			light_on=1;	
			printk(KERN_ERR "BH1721: light_on\n");
			return (ret);
			break;
			
                case ALS_IOCTL_PW_OFF:	
			del_timer(&lux_poll_timer);

			for (i = 0; i < TAOS_FILTER_DEPTH; i++)
                                lux_history[i] = -ENODATA;
                        if ((ret = (bh1721_regs_write(als_datap->client, &als_reg_cmd[0],1))) < 0) {
                                printk(KERN_ERR "BH1721: i2c_smbus_write_byte failed in ioctl als_off\n");
                                return (ret);
                        }
                        
			light_on=0;	
			//printk(KERN_ERR "BH1721: light_off\n");
			return (ret);
                	break;
					
		case ALS_IOCTL_DATA:                
			return (glux);
			break;
			
		case ALS_IOCTL_CALIBRATE:                     
                        if ((lux_val = als_lux_poll()) < 0) {
                                printk(KERN_ERR "BH1721: call to als_lux_poll() returned error %d in ioctl als_calibrate\n", lux_val);
	                        return (lux_val);
			}
			gain_val = (u16)((als_cfgp->calibrate_target) /lux_val);
			als_cfgp->gain = (int)gain_val;		
			return ((int)gain_val);
			break;
			
		case ALS_IOCTL_CONFIG_GET:
			ret = copy_to_user((struct als_cfg *)arg, als_cfgp, sizeof(struct als_cfg));
			if (ret) {
				printk(KERN_ERR "BH1721: copy_to_user failed in ioctl config_get\n");
				return -ENODATA;
			}
			return (ret);
			break;
			
                case ALS_IOCTL_CONFIG_SET:
                        ret = copy_from_user(als_cfgp, (struct als_cfg *)arg, sizeof(struct als_cfg));
			if (ret) {
				printk(KERN_ERR "BH1721: copy_from_user failed in ioctl config_set\n");
                                return -ENODATA;
			}
                	break;

		case ALS_IOCTL_GET_ENABLED:
			return put_user(light_on, (unsigned long __user *)arg);
			break;		
					
		default:
			return -EINVAL;
			break;
	}
	return (ret);
}


//report als data to input sub system,  for data poll utility in hal
static void als_report_value(int val)
{
	int lux_val=val;
	
	input_report_abs(light->input_dev, ABS_MISC, lux_val);
#if DEBUG	
	printk(KERN_INFO "BH1721: als_report_value =%d\n",  lux_val);
#endif	
	input_sync(light->input_dev);		
}

/*******************************************/
/*als_add_lux_filter: add value to histrory samples     */
/*******************************************/
static void als_add_lux_filter(int lux)
{
        lux_history[2] = lux_history[1];
        lux_history[1] = lux_history[0];
        lux_history[0] = lux;
}

/*******************************************/
/*als_get_lux_filter: get the middle value of  samples */
/*******************************************/
static int als_get_lux_filter(void)
{
	static u8 middle[] = {1,0,2,0,0,2,0,1};
        int index= 0;
        if( lux_history[0] > lux_history[1] ) index += 4;
        if( lux_history[1] > lux_history[2] ) index += 2;
        if( lux_history[0] > lux_history[2] ) index++;
        return(lux_history[middle[index]]);
}



/*******************************************/
/*light intensity lux poll*/
/*******************************************/
static int als_lux_poll(void) 
{
	u8 reg[2];
	int ret=0;
#if DEBUG	
	pr_crit(ALS_TAG "bh1721 start to calc lux value\n");
#endif	
	ret=bh1721_regs_read(als_datap->client, reg, 2);
	if (ret<0){		
        	printk(KERN_INFO "BH1721: i2c_smbus_read_word failed in als_get_lux()\n");
		return ret;	
        } 
#if DEBUG	
        printk(KERN_ERR "BH1721: als_lux_poll start to poll lux value, lux=%d_%d, glux=%d\n", reg[0],reg[1],glux);
#endif	        
	ret=((reg[0]<<8)&0xff00)|reg[1];
	ret=ret*8;
#if DEBUG	
	printk(KERN_ERR "BH1721: lux=%d\n", ret);
#endif		
	return ret;

}

/*******************************************/
/*light work*/
/*******************************************/
/*ZTE_BH7121_001 start*/
static void do_als_work(struct work_struct *w)
{
	int temp=0;
	int lux_temp=0;
	lux_temp=als_lux_poll();
	temp=lux_temp-glux;
	if(abs(temp)>(glux/5)){
#if DEBUG		
		pr_crit(ALS_TAG "bh1721 lux value over threshold\n");
#endif
		thres_cont++;
		als_add_lux_filter(lux_temp);
	}
	else{
		thres_cont=0;
	}
	
	if(thres_cont==ALS_LUX_INT_FILTER)
	{
		thres_cont=0;
		glux=als_get_lux_filter();
		als_report_value(glux);
	
	}	
			
}
/*ZTE_BH7121_001 end*/

//-----------------------------------------------------------------
static int bh1721_regs_read(struct i2c_client * client, char * buf, int count)
{
	int	ret = 0;

	if (count != i2c_master_recv(client, buf, count)) 
		ret = -EIO;

	return ret;
}


//---------------------------------------------------------------------
static int bh1721_regs_write(struct i2c_client * client,const char * buf, int count)
{
	int	ret = 0;

	if (count != i2c_master_send(client, buf, count)) 
		ret = -EIO;

	return ret;
}


//lux poll timer function
static void als_lux_poll_timer_func(unsigned long param) {

	if (!device_released) {
		if (NULL == als_datap->workqueue) {
			return ;
		}
		queue_delayed_work(als_datap->workqueue, &als_datap->als_work, 0);		
		als_lux_poll_timer_start();
	}
	return;
}

// start lux poll timer
static void als_lux_poll_timer_start(void) {
        lux_poll_timer.expires = jiffies + (HZ/5);
        lux_poll_timer.function = als_lux_poll_timer_func;
        add_timer(&lux_poll_timer);
        return;
}


MODULE_AUTHOR("Wangzy - ZTE Corporation");
MODULE_DESCRIPTION("BH1721 ambient light sensor driver");
MODULE_LICENSE("GPL");

module_init(als_init);
module_exit(als_exit);

