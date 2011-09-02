/*
 * =====================================================================================
 *
 *       Filename:  lis302dl.c
 *
 *    Description:  ST. lis305de g-sensor  driver
 *
 *        Version:  1.0
 *        Created:  11/11/2009 11:01:10 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Huang pei
 *        Company:  ZTE 
 *
 * =====================================================================================
 */

/*histstory:
 when       who     what, where, why                                            comment tag
 --------   ----    ---------------------------------------------------    ----------------------------------
 2009-11-17    hp     change for new mooncake layout                            ZTE_SENSOR_HP_003
 2009-11-16    hp     change into ioctl interface                               ZTE_SENSOR_HP_002
 2009-12-15    lsz    modify wakeup and suspend                                 ZTE_SENSOR_LSZ_001
*/
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/input.h>
#include <linux/lis302dl.h>
#include <asm/gpio.h>
#include <linux/delay.h>
#include<linux/earlysuspend.h>

#define LIS302_TAG "[hp@gsensor]:"
static struct i2c_client *this_client;

struct lis302dl_data {
	struct early_suspend early_suspend;
};

static struct lis302dl_platform_data *lis302dl_pdata;


static int lis302dl_i2c_rx(char *buffer, int length)
{

	int retry;
	struct i2c_msg msgs[] = {
		{
		 .addr = this_client->addr,
		 .flags = 0,
		 .len = 1,
		 .buf = buffer,
		},
		{
		 .addr = this_client->addr,
		 .flags = I2C_M_RD,
		 .len = length,
		 .buf = buffer,
		 },
	};

	if(length>1){
		pr_err(LIS302_TAG "only one byte at a time\n");
		return -1;
	}

	for (retry = 0; retry <= 10; retry++) {
		if (i2c_transfer(this_client->adapter, msgs, 2) > 0)
			break;
		else
			mdelay(5);
	}
	if (retry > 10) {
		printk(LIS302_TAG "%s: retry over 10\n",__func__);
		return -EIO;
	}else

		return 0;
}

static int lis302dl_i2c_tx(char *buffer, int length)
{
	int retry;
	struct i2c_msg msg[] = {
		{
		 .addr = this_client->addr,
		 .flags = 0,
		 .len = length,
		 .buf = buffer,
		 },
	};

	if(length>2){
		pr_err(LIS302_TAG "only one byte at a time\n");
		return -1;
	}

	for (retry = 0; retry <= 10; retry++) {
		if(i2c_transfer(this_client->adapter, msg, 1) > 0)
			break;
		else
			mdelay(5);
	}
	if (retry > 10) {
		printk(KERN_ERR "%s: retry over 10\n", __func__);
		return -EIO;
	}else 
		return 0;
}

static int lis302dl_hw_init(struct lis302dl_platform_data *pdata)
{
	char buf[4];
	int ret;
	//power up
	buf[0] =  LIS302DL_REG_CTRL1;
	buf[1] =  0x47;
	ret = lis302dl_i2c_tx(buf, 2);
	if(ret<0)
		return -1;
	mdelay(5);

	//reload internal register and select data fliter
	buf[0] = LIS302DL_REG_CTRL2;
	buf[1] = 0x40;
	ret = lis302dl_i2c_tx(buf, 2);
	if(ret<0)
		return -1;

	return 0;
}

/*
static int lis302dl_set_range(char range)
{
	char buffer[2];
	int ret;
	buffer[0] = RANGE_BWIDTH_REG;
	ret = lis302dl_i2c_rx(buffer, 1);
	if (ret < 0)
		return -1;
	buffer[1] = (buffer[0]&0xe7)|range<<3;
	buffer[0] = RANGE_BWIDTH_REG;
	ret = lis302dl_i2c_tx(buffer, 2);

	return ret;
}
*/
/*
static int lis302dl_get_range(void)
{
	char buffer;
	int ret;
	buffer = RANGE_BWIDTH_REG;
	ret = lis302dl_i2c_rx(&buffer, 1);
	if (ret < 0)
		return -1;
	buffer = (buffer&0x18)>>3;
	return buffer;
}
*/
/*
static int lis302dl_reset_int(void)
{
	char buffer[2];
	int ret;
	buffer[0] = SMB150_CTRL_REG;
	ret = lis302dl_i2c_rx(buffer, 1);
	if (ret < 0)
		return -1;
	buffer[1] = (buffer[0]&0xbf)|0x40;
	buffer[0] = SMB150_CTRL_REG;
	ret = lis302dl_i2c_tx(buffer, 2);

	return ret;
}
*/
/* set  operation mode 0 = normal, 1 = sleep*/
static int lis302dl_set_mode(int  mode)
{
	char buffer[2];
	int ret;
	if(mode) {
		buffer[0] = LIS302DL_REG_CTRL1; 
		buffer[1] = 0x47;
		ret = lis302dl_i2c_tx(buffer, 2); 
		if(ret<0)
			return -1;

		buffer[0] = LIS302DL_REG_CTRL2;
		buffer[1] = 0x40;
		ret = lis302dl_i2c_tx(buffer, 2); 
		if(ret<0)
			return -1;

	}else {
		buffer[0] = LIS302DL_REG_CTRL1;
		buffer[1] = 0x00;
//ZTE_SENSOR_LSZ_001,lsz modify wakeup and suspend for g-sensor,Begin
#if 0
		ret = lis302dl_i2c_rx(buffer, 1);
#else
		ret = lis302dl_i2c_tx(buffer, 2); 
#endif
//ZTE_SENSOR_LSZ_001,lsz modify wakeup and suspend for g-sensor,End
		if(ret<0)
			return -1;
	}
	/*buffer[0] = LIS302DL_REG_CTRL1;*/
	/*ret = lis302dl_i2c_rx(buffer, 1, 1);*/
	/*if (ret < 0)*/
		/*return ret;*/
	/*buffer[1] = buffer[0] | (!!mode << 6);*/
	/*buffer[0] = LIS302DL_REG_CTRL1;*/
	/*ret = lis302dl_i2c_tx(buffer, 2, 1);*/
	/*if(!ret && mode)*/
		/*ret = lis302dl_hw_init(lis302dl_pdata);*/

	return ret;
}

static int  lis302dl_TransRBuff(short *buffer)
{
	char buf[4];
	int ret;

	/*buf[4] = LIS302DL_REG_CTRL1;*/
	/*ret = lis302dl_i2c_rx(&buf[4], 1);*/
	/*if(ret<0)*/
		/*return -1;*/
	/*buf[5] = LIS302DL_REG_CTRL2;*/
	/*ret = lis302dl_i2c_rx(&buf[5], 1);*/
	/*if(ret<0)*/
		/*return -1;*/
	/*buf[6] = LIS302DL_REG_CTRL3;*/
	/*ret = lis302dl_i2c_rx(&buf[6], 1);*/
	/*if(ret<0)*/
		/*return -1;*/

	buf[3] = LIS302DL_REG_STATUS;
	ret = lis302dl_i2c_rx(&buf[3], 1);
	if(ret<0)
		return -1;

	buf[0] = LIS302DL_REG_OUT_X;
	ret = lis302dl_i2c_rx(&buf[0], 1);
	if(ret<0)
		return -1;
	buf[1] = LIS302DL_REG_OUT_Y;
	ret = lis302dl_i2c_rx(&buf[1], 1);
	if(ret<0)
		return -1;

	buf[2] = LIS302DL_REG_OUT_Z;
	ret = lis302dl_i2c_rx(&buf[2], 1);
	if(ret<0)
		return -1;
//ZTE_SENSOR_HP_003 2009-11-17 for mooncake gsensor layout 
#if defined(CONFIG_MACH_MOONCAKE)
	buffer[0] = ((s8)buf[0])*18*(-1);
	buffer[1] = ((s8)buf[1])*18*(-1);
	buffer[2] = ((s8)buf[2])*18*(-1);
#elif defined(CONFIG_MACH_RAISE)
	buffer[0] = ((s8)buf[1])*18*(-1);
	buffer[1] = ((s8)buf[0])*18*(-1);
	buffer[2] = ((s8)buf[2])*18;
#elif defined(CONFIG_MACH_JOE)
	buffer[0] = ((s8)buf[1])*18;
	buffer[1] = ((s8)buf[0])*18;
	buffer[2] = ((s8)buf[2])*18;
#elif defined(CONFIG_MACH_BLADE)
#if 0
	buffer[0] = ((s8)buf[1])*18*(-1);
	buffer[1] = ((s8)buf[0])*18;
	buffer[2] = ((s8)buf[2])*18*(-1);//Froyo
#endif
	buffer[0] = ((s8)buf[2])*18;
	buffer[1] = ((s8)buf[1])*18*(-1);
	buffer[2] = ((s8)buf[0])*18*(-1);//Gingerbread
#elif defined(CONFIG_MACH_SMOOTH)
	buffer[0] = ((s8)buf[0])*18*(-1);
	buffer[1] = ((s8)buf[1])*18;
	buffer[2] = ((s8)buf[2])*18;
#elif defined(CONFIG_MACH_R750)
	buffer[0] = ((s8)buf[0])*18;
	buffer[1] = ((s8)buf[1])*18;
	buffer[2] = ((s8)buf[2])*18*(-1);
#elif defined(CONFIG_MACH_V9)
	buffer[0] = ((s8)buf[0])*18*(-1);
	buffer[1] = ((s8)buf[1])*18;
	buffer[2] = ((s8)buf[2])*18;  
#elif defined(CONFIG_MACH_AMIGO)
	buffer[0] = ((s8)buf[0])*18*(-1);
	buffer[1] = ((s8)buf[1])*18;
	buffer[2] = ((s8)buf[2])*18;
#elif defined(CONFIG_MACH_SKATE)
	buffer[0] = ((s8)buf[0])*18*(-1);
	buffer[1] = ((s8)buf[1])*18;
	buffer[2] = ((s8)buf[2])*18;  //Gingerbread
#elif defined(CONFIG_MACH_ROAMER)
	buffer[0] = ((s8)buf[1])*18*(-1);	
	buffer[1] = ((s8)buf[0])*18;	
	buffer[2] = ((s8)buf[2])*18*(-1);
#elif defined(CONFIG_MACH_NOVA)
	buffer[0] = ((s8)buf[0])*18;
	buffer[1] = ((s8)buf[1])*18;
	buffer[2] = ((s8)buf[2])*18*(-1);
#elif defined(CONFIG_MACH_SAILBOAT)	
	buffer[0] = ((s8)buf[0])*18;
	buffer[1] = ((s8)buf[1])*18;
	buffer[2] = ((s8)buf[2])*18*(-1);
#elif defined(CONFIG_MACH_TURIES)	
	buffer[0] = ((s8)buf[0])*18;
	buffer[1] = ((s8)buf[1])*18;
	buffer[2] = ((s8)buf[2])*18*(-1);
#elif defined(CONFIG_MACH_BLADE2)	
	buffer[0] = ((s8)buf[1])*18;
	buffer[1] = ((s8)buf[0])*18;
	buffer[2] = ((s8)buf[2])*18;
#else
	buffer[0] = ((s8)buf[1])*18*(-1);
	buffer[1] = ((s8)buf[0])*18*(-1);
	buffer[2] = ((s8)buf[2])*18;
#endif /* define CONFIG_MACH_MOONCAKE */

//ZTE_SENSOR_HP_003 end
	/*pr_info(LIS302_TAG "ctrl 1-3 status x y z is %x, %x, %x %x %d %d %d\n",*/
			/*buf[6], buf[5], buf[4], buf[3], buf[1], buf[0], buf[2]);*/
	/*pr_info(LIS302_TAG "x y z is: %d, %d ,%d \n", buffer[0], */
			/*buffer[1], buffer[2]);*/
	return 0;
}

static int lis302dl_open(struct inode *inode, struct file *file)
{
	return nonseekable_open(inode, file);
}

static int lis302dl_release(struct inode *inode, struct file *file)
{
	return 0;
}

static int lis302dl_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
	   unsigned long arg)
{

	void __user *argp = (void __user *)arg;

	char rwbuf[8];
	int ret = -1;
	short buf[8], temp;

	switch (cmd) {
	case LIS302DL_IOCTL_READ:
	case LIS302DL_IOCTL_WRITE:
	case LIS302DL_IOCTL_SET_MODE:
		if (copy_from_user(&rwbuf, argp, sizeof(rwbuf)))
			return -EFAULT;
		break;
	case LIS302DL_IOCTL_READ_ACCELERATION:
		if (copy_from_user(&buf, argp, sizeof(buf)))
			return -EFAULT;
		break;
	default:
		break;
	}

	switch (cmd) {
	case LIS302DL_IOCTL_INIT:
		ret = lis302dl_hw_init(lis302dl_pdata);
		if (ret < 0)
			return ret;
		break;
	case LIS302DL_IOCTL_READ:
		if (rwbuf[0] < 1)
			return -EINVAL;
		ret = lis302dl_i2c_rx(&rwbuf[1], rwbuf[0]);
		if (ret < 0)
			return ret;
		break;
	case LIS302DL_IOCTL_WRITE:
		if (rwbuf[0] < 2)
			return -EINVAL;
		ret = lis302dl_i2c_tx(&rwbuf[1], rwbuf[0]);
		if (ret < 0)
			return ret;
		break;
	case LIS302DL_IOCTL_READ_ACCELERATION:
		ret = lis302dl_TransRBuff(&buf[0]);
		if (ret < 0)
			return ret;
		break;
	case LIS302DL_IOCTL_SET_MODE:
		lis302dl_set_mode(rwbuf[0]);
		break;
	default:
		return -ENOTTY;
	}

	switch (cmd) {
	case LIS302DL_IOCTL_READ:
		if (copy_to_user(argp, &rwbuf, sizeof(rwbuf)))
			return -EFAULT;
		break;
	case LIS302DL_IOCTL_READ_ACCELERATION:
		if (copy_to_user(argp, &buf, sizeof(buf)))
			return -EFAULT;
		break;
	case LIS302DL_IOCTL_GET_INT:
		if (copy_to_user(argp, &temp, sizeof(temp)))
			return -EFAULT;
		break;
	default:
		break;
	}

	return 0;
}

static void lis302dl_early_suspend(struct early_suspend *handler)
{
	int ret = lis302dl_set_mode(0);
	if(!ret)
		pr_info(LIS302_TAG "lis302dl suspend\n");
	else
		pr_err(LIS302_TAG "lis302dl suspend  failed \n");
}


static void lis302dl_late_resume(struct early_suspend *handler)
{
	int ret = lis302dl_set_mode(1);

	if(!ret)
		pr_info(LIS302_TAG "lis302dl resume\n");
	else
		pr_err(LIS302_TAG "lis302dl resume failed \n");

}

static struct file_operations lis302dl_fops = {
	.owner = THIS_MODULE,
	.open = lis302dl_open,
	.release = lis302dl_release,
	.ioctl = lis302dl_ioctl,
};

static struct miscdevice lis302dl_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "lis302dl",
	.fops = &lis302dl_fops,
};

int lis302dl_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct lis302dl_data *lis302dl;
	int err = 0;
	char buf[4];

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}

	this_client = client;
	buf[0] = LIS302DL_REG_WHO_AM_I;
	err = lis302dl_i2c_rx(buf, 1);
	if(err<0) {
		pr_err(LIS302_TAG "i2c read error!\n");
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}
	pr_info(LIS302_TAG "device id is %d\n", buf[0]);
	if(buf[0]!= LIS302DL_WHO_AM_I_MAGIC) {
		pr_err(LIS302_TAG "device id not match!\n");
		err = -ENODEV;
		goto exit_check_functionality_failed;
	}

	lis302dl = kzalloc(sizeof(struct lis302dl_data), GFP_KERNEL);
	if (!lis302dl) {
		err = -ENOMEM;
		goto exit_alloc_data_failed;
	}

	i2c_set_clientdata(client, lis302dl);

	lis302dl_pdata = client->dev.platform_data;
	if (lis302dl_pdata == NULL) {
		pr_err(LIS302_TAG "lis302dl_init_client: platform data is NULL\n");
		goto exit_platform_data_null;
	}

	err = lis302dl_hw_init(lis302dl_pdata);
	if (err < 0) {
		pr_err(LIS302_TAG "lis302dl_probe: lis302dl_init failed\n");
		goto exit_init_failed;
	}

	err = misc_register(&lis302dl_device);
	if (err) {
		pr_err(LIS302_TAG "lis302dl_probe: device register failed\n");
		goto exit_misc_device_register_failed;
	}
	pr_err(LIS302_TAG "lis302dl_probe: lis302dl_device register OK\n");//add for test on 20100208
	lis302dl->early_suspend.suspend = lis302dl_early_suspend;
	lis302dl->early_suspend.resume = lis302dl_late_resume;
	register_early_suspend(&lis302dl->early_suspend);

	return 0;

exit_misc_device_register_failed:
exit_init_failed:
exit_platform_data_null:
	kfree(lis302dl);
exit_alloc_data_failed:
exit_check_functionality_failed:
	return err;
}

static int lis302dl_remove(struct i2c_client *client)
{
	struct lis302dl_data *lis302dl = i2c_get_clientdata(client);

	unregister_early_suspend(&lis302dl->early_suspend);
	misc_deregister(&lis302dl_device);
	i2c_release_client(client);
	kfree(lis302dl);

	return 0;
}

static const struct i2c_device_id lis302dl_id[] = {
	{ "lis302dl", 0 },
	{ }
};

static struct i2c_driver lis302dl_driver = {
	.probe = lis302dl_probe,
	.remove = lis302dl_remove,
	.id_table = lis302dl_id,
	.driver = {
		.name = "lis302dl"
	}
};

static int __init lis302dl_init(void)
{
	pr_info(LIS302_TAG "lis302dl G-sensor driver: init\n");
	return i2c_add_driver(&lis302dl_driver);
}

static void __exit lis302dl_exit(void)
{
	pr_info(LIS302_TAG "lis302dl G-sensor driver: exit\n");
	i2c_del_driver(&lis302dl_driver);
}

module_init(lis302dl_init);
module_exit(lis302dl_exit);
MODULE_LICENSE("GPL v2");
