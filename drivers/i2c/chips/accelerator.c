/*
 * ===================================================================================================
 *
 *	Filename:  accelerator.c
 *
 * 	Description:  ADI 345/346 && ST.35DE/33DE/302DL Three-Axis Digital Accelerometers (I2C Interface)
 *
 *	Version:   1.0
 *	Created:   11/03/2010 15:30:10 AM
 *	Revision:  none
 *	Compiler:  gcc
 *
 * 	Author:    Feng Yuao
 *	Company:   ZTE Corporation, China
 *
 * ===================================================================================================
 */

#include <linux/device.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <asm/gpio.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/i2c.h>
#include <linux/workqueue.h>
#include <linux/miscdevice.h>
#include <linux/earlysuspend.h>
#include <linux/lis302dl.h>
#include <linux/adxl34x.h>

#define  LIS302_TAG 		"[lis302dl@gsensor]:"
#define  ADXL34X_TAG  		"[adxl34x@gsensor]"
static char buffer_store = '0';
static int device_flag;
static struct i2c_client *this_client;

struct lis302dl_data {
	struct early_suspend early_suspend;
};

struct adxl34x_data {
	struct early_suspend early_suspend;
};


static struct lis302dl_platform_data *lis302dl_pdata;

static struct lis302dl_platform_data gsensor = {
	.gpio_intr1 =  84,
	.gpio_intr2 =  85,
	.scale      =  2 ,
	.int_active_low = 1,
};


/* ADXL345/6 Register Map */
#define DEVID		0x00	/* R   Device ID */
#define THRESH_TAP	0x1D	/* R/W Tap threshold */
#define OFSX		0x1E	/* R/W X-axis offset */
#define OFSY		0x1F	/* R/W Y-axis offset */
#define OFSZ		0x20	/* R/W Z-axis offset */
#define DUR			0x21	/* R/W Tap duration */
#define LATENT		0x22	/* R/W Tap latency */
#define WINDOW		0x23	/* R/W Tap window */
#define THRESH_ACT	0x24	/* R/W Activity threshold */
#define THRESH_INACT	0x25	/* R/W Inactivity threshold */
#define TIME_INACT	0x26	/* R/W Inactivity time */
#define ACT_INACT_CTL	0x27	/* R/W Axis enable control for activity and */
				/* inactivity detection */
#define THRESH_FF	0x28	/* R/W Free-fall threshold */
#define TIME_FF		0x29	/* R/W Free-fall time */
#define TAP_AXES	0x2A	/* R/W Axis control for tap/double tap */
#define ACT_TAP_STATUS	0x2B	/* R   Source of tap/double tap */
#define BW_RATE		0x2C	/* R/W Data rate and power mode control */
#define POWER_CTL	0x2D	/* R/W Power saving features control */
#define INT_ENABLE	0x2E	/* R/W Interrupt enable control */
#define INT_MAP		0x2F	/* R/W Interrupt mapping control */
#define INT_SOURCE	0x30	/* R   Source of interrupts */
#define DATA_FORMAT	0x31	/* R/W Data format control */
#define DATAX0		0x32	/* R   X-Axis Data 0 */
#define DATAX1		0x33	/* R   X-Axis Data 1 */
#define DATAY0		0x34	/* R   Y-Axis Data 0 */
#define DATAY1		0x35	/* R   Y-Axis Data 1 */
#define DATAZ0		0x36	/* R   Z-Axis Data 0 */
#define DATAZ1		0x37	/* R   Z-Axis Data 1 */
#define FIFO_CTL	0x38	/* R/W FIFO control */
#define FIFO_STATUS	0x39	/* R   FIFO status */
#define TAP_SIGN	0x3A	/* R   Sign and source for tap/double tap */
/* Orientation ADXL346 only */
#define ORIENT_CONF	0x3B	/* R/W Orientation configuration */
#define ORIENT		0x3C	/* R   Orientation status */

/* DEVIDs */
#define ID_ADXL345	0xE5
#define ID_ADXL346	0xE6

/* INT_ENABLE/INT_MAP/INT_SOURCE Bits */
#define DATA_READY	(1 << 7)
#define SINGLE_TAP	(1 << 6)
#define DOUBLE_TAP	(1 << 5)
#define ACTIVITY	(1 << 4)
#define INACTIVITY	(1 << 3)
#define FREE_FALL	(1 << 2)
#define WATERMARK	(1 << 1)
#define OVERRUN		(1 << 0)

/* ACT_INACT_CONTROL Bits */
#define ACT_ACDC	(1 << 7)
#define ACT_X_EN	(1 << 6)
#define ACT_Y_EN	(1 << 5)
#define ACT_Z_EN	(1 << 4)
#define INACT_ACDC	(1 << 3)
#define INACT_X_EN	(1 << 2)
#define INACT_Y_EN	(1 << 1)
#define INACT_Z_EN	(1 << 0)

/* TAP_AXES Bits */
#define SUPPRESS	(1 << 3)
#define TAP_X_EN	(1 << 2)
#define TAP_Y_EN	(1 << 1)
#define TAP_Z_EN	(1 << 0)

/* ACT_TAP_STATUS Bits */
#define ACT_X_SRC	(1 << 6)
#define ACT_Y_SRC	(1 << 5)
#define ACT_Z_SRC	(1 << 4)
#define ASLEEP		(1 << 3)
#define TAP_X_SRC	(1 << 2)
#define TAP_Y_SRC	(1 << 1)
#define TAP_Z_SRC	(1 << 0)

/* BW_RATE Bits */
#define LOW_POWER	(1 << 4)
#define RATE(x)		((x) & 0xF)

/* POWER_CTL Bits */
#define PCTL_LINK			(1 << 5)
#define PCTL_AUTO_SLEEP 	(1 << 4)
#define PCTL_MEASURE		(1 << 3)
#define PCTL_SLEEP			(1 << 2)
#define PCTL_WAKEUP(x)		((x) & 0x3)
#define PCTL_STANDBY        0X00

/* DATA_FORMAT Bits */
#define SELF_TEST			(1 << 7)
#define SPI					(1 << 6)
#define INT_INVERT			(1 << 5)
#define FULL_RES			(1 << 3)
#define JUSTIFY				(1 << 2)
#define RANGE(x)			((x) & 0x3)
#define RANGE_PM_2g			0
#define RANGE_PM_4g			1
#define RANGE_PM_8g			2
#define RANGE_PM_16g		3

/*
 * Maximum value our axis may get in full res mode for the input device
 * (signed 13 bits)
 */
#define ADXL_FULLRES_MAX_VAL 4096

/*
 * Maximum value our axis may get in fixed res mode for the input device
 * (signed 10 bits)
 */
#define ADXL_FIXEDRES_MAX_VAL 512

/* FIFO_CTL Bits */
#define FIFO_MODE(x)	(((x) & 0x3) << 6)
#define FIFO_BYPASS	0
#define FIFO_FIFO	1
#define FIFO_STREAM	2
#define FIFO_TRIGGER	3
#define TRIGGER		(1 << 5)
#define SAMPLES(x)	((x) & 0x1F)

/* FIFO_STATUS Bits */
#define FIFO_TRIG	(1 << 7)
#define ENTRIES(x)	((x) & 0x3F)

/* TAP_SIGN Bits ADXL346 only */
#define XSIGN		(1 << 6)
#define YSIGN		(1 << 5)
#define ZSIGN		(1 << 4)
#define XTAP		(1 << 3)
#define YTAP		(1 << 2)
#define ZTAP		(1 << 1)

/* ORIENT_CONF ADXL346 only */
#define ORIENT_DEADZONE(x)	(((x) & 0x7) << 4)
#define ORIENT_DIVISOR(x)	((x) & 0x7)

/* ORIENT ADXL346 only */
#define ADXL346_2D_VALID		(1 << 6)
#define ADXL346_2D_ORIENT(x)		(((x) & 0x3) >> 4)
#define ADXL346_3D_VALID		(1 << 3)
#define ADXL346_3D_ORIENT(x)		((x) & 0x7)
#define ADXL346_2D_PORTRAIT_POS		0	/* +X */
#define ADXL346_2D_PORTRAIT_NEG		1	/* -X */
#define ADXL346_2D_LANDSCAPE_POS	2	/* +Y */
#define ADXL346_2D_LANDSCAPE_NEG	3	/* -Y */

#define ADXL346_3D_FRONT		3	/* +X */
#define ADXL346_3D_BACK			4	/* -X */
#define ADXL346_3D_RIGHT		2	/* +Y */
#define ADXL346_3D_LEFT			5	/* -Y */
#define ADXL346_3D_TOP			1	/* +Z */
#define ADXL346_3D_BOTTOM		6	/* -Z */

#undef ADXL_DEBUG

#define AC_READ(ac, reg)	((ac)->read((ac)->bus, reg))
#define AC_WRITE(ac, reg, val)	((ac)->write((ac)->bus, reg, val))

#define bus_device		struct i2c_client

struct axis_triple {
	int x;
	int y;
	int z;
};

struct adxl34x {
	bus_device *bus;
	struct input_dev *input;
	struct work_struct work;
	struct mutex mutex;	/* reentrant protection for struct */
	struct adxl34x_platform_data pdata;
	struct axis_triple swcal;
	struct axis_triple hwcal;
	struct axis_triple saved;
	char phys[32];
	unsigned disabled:1;	/* P: mutex */
	unsigned opened:1;	/* P: mutex */
	unsigned fifo_delay:1;
	unsigned model;
	unsigned int_mask;

	int (*read) (bus_device *, unsigned char);
	int (*read_block) (bus_device *, unsigned char, int, unsigned char *);
	int (*write) (bus_device *, unsigned char, unsigned char);
};

static const struct adxl34x_platform_data adxl34x_default_init = {
	.tap_threshold = 35,
	.tap_duration = 3,
	.tap_latency = 20,
	.tap_window = 20,
	.tap_axis_control = ADXL_TAP_X_EN | ADXL_TAP_Y_EN | ADXL_TAP_Z_EN,
	.act_axis_control = 0xFF,
	.activity_threshold = 6,
	.inactivity_threshold = 4,
	.inactivity_time = 3,
	.free_fall_threshold = 8,
	.free_fall_time = 0x20,
	.data_rate = 8,
	.data_range = ADXL_FULL_RES,

	.ev_type = EV_ABS,
	.ev_code_x = ABS_X,	/* EV_REL */
	.ev_code_y = ABS_Y,	/* EV_REL */
	.ev_code_z = ABS_Z,	/* EV_REL */

	.ev_code_tap_x = BTN_TOUCH,	/* EV_KEY */
	.ev_code_tap_y = BTN_TOUCH,	/* EV_KEY */
	.ev_code_tap_z = BTN_TOUCH,	/* EV_KEY */
	.power_mode = ADXL_AUTO_SLEEP | ADXL_LINK,
	.fifo_mode = FIFO_STREAM,
	.watermark = 0,
};

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
//ZTE_SENSOR_LSZ_001, modify wakeup and suspend for g-sensor,Begin
#if 0
		ret = lis302dl_i2c_rx(buffer, 1);
#else
		ret = lis302dl_i2c_tx(buffer, 2); 
#endif
//ZTE_SENSOR_LSZ_001, modify wakeup and suspend for g-sensor,End
		if(ret<0){
			printk(LIS302_TAG"%s call lis302dl_i2c_tx error", __func__);
			return -1;
		}
	}

	return ret;
}

static int  lis302dl_TransRBuff(short *buffer)
{
	char buf[4];
	int ret;

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
	buffer[0] = ((s8)buf[1])*18*(-1);
	buffer[1] = ((s8)buf[0])*18;
	buffer[2] = ((s8)buf[2])*18*(-1);
#elif defined(CONFIG_MACH_NOVA)
	buffer[0] = ((s8)buf[1])*18*(-1);
	buffer[1] = ((s8)buf[0])*18;
	buffer[2] = ((s8)buf[2])*18*(-1);
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
#elif defined(CONFIG_MACH_ROAMER)
	buffer[0] = ((s8)buf[1])*18*(-1);
	buffer[1] = ((s8)buf[0])*18;
	buffer[2] = ((s8)buf[2])*18*(-1);
#else
	buffer[0] = ((s8)buf[1])*18*(-1);
	buffer[1] = ((s8)buf[0])*18*(-1);
	buffer[2] = ((s8)buf[2])*18;
#endif /* define CONFIG_MACH_MOONCAKE */

	return 0;
}

static int adxl34x_i2c_read(struct i2c_client *client, unsigned char reg)
{
	int retry,ret;
 	unsigned char buffer[2];
	
	struct i2c_msg msgs[] = {
		{
		 .addr = client->addr,
		 .flags = 0,
		 .len = 1,
		 .buf = buffer,
		},
		{
		 .addr = client->addr,
		 .flags = I2C_M_RD,
		 .len = 1,
		 .buf = buffer,
		 },
	};

	buffer[0] = reg;
	
	for (retry = 0; retry <= 10; retry++) {
		ret = i2c_transfer(client->adapter, msgs, 2);
		if ( ret > 0)
			break;
		else{
			mdelay(5);
			printk(ADXL34X_TAG"adxl34x_i2c_read-->i2c_transfer ret = %d\n",ret);
		}
	}
	if (retry > 10) {
		printk(ADXL34X_TAG "%s: retry over 10\n",__func__);
		return -EIO;
	}else{
		buffer_store = buffer[0];
		return 0;
	}
}

static int adxl34x_i2c_write(struct i2c_client *client,
				   unsigned char reg, unsigned char val)
{
	int retry;
	unsigned char buffer[2];

	struct i2c_msg msg[] = {
		{
		 .addr = client->addr,
		 .flags = 0,
		 .len = 2,
		 .buf = buffer,
		 },
	};
	
	buffer[0] = reg;
 	buffer[1] = val;

	for (retry = 0; retry <= 10; retry++) {
		if(i2c_transfer(client->adapter, msg, 1) > 0)
			break;
		else
			mdelay(5);
	}
	if (retry > 10) {
		printk(ADXL34X_TAG "%s: retry over 10\n", __func__);
		return -EIO;
	}else 
		return 0;
}

static int adxl34x_i2c_master_read_block_data(struct i2c_client *client,
					      unsigned char reg, int count,
					      unsigned char *buf)
{
	int ret;

	ret = i2c_master_send(client, &reg, 1);
	if (ret < 0)
		return ret;
	
	ret = i2c_master_recv(client, buf, count);
	if (ret < 0)
		return ret;
	if (ret != count)
		return -EIO;

	return 0;
}

static int adxl34x_hw_init(void)
{
	int ret;
	
	//Data rate and power model control
	ret = adxl34x_i2c_write(this_client,BW_RATE,0X0B);
	if(ret < 0)
		goto exit;
	mdelay(5);
	
	//Power save features control
	ret = adxl34x_i2c_write(this_client,POWER_CTL,0X08);
	if(ret < 0)
		goto exit;
	mdelay(5);

	//Data format control
	ret = adxl34x_i2c_write(this_client,DATA_FORMAT,0X00);
	if(ret < 0)
		goto exit;

	printk(ADXL34X_TAG"adxl34x_hw_init success\n");
	return 0;
	
exit:
	printk(ADXL34X_TAG"adxl34x_hw_init fail\n");
	return ret;
}

static int adxl34x_get_triple(short *buffer)
{
	short buf[3];
	int ret;

	ret = adxl34x_i2c_master_read_block_data(this_client, DATAX0, DATAZ1 - DATAX0 + 1,
		       (unsigned char *)buf);
	if(ret < 0){
		printk(ADXL34X_TAG"%s function read initial data error\n",__func__);
		return ret;
	}
	
#if defined(CONFIG_MACH_MOONCAKE)
	buffer[0] = (s16) le16_to_cpu(buf[0])*(-1)*4;
	buffer[1] = (s16) le16_to_cpu(buf[1])*(-1)*4;
	buffer[2] = (s16) le16_to_cpu(buf[2])*(-1)*4;
#elif defined(CONFIG_MACH_BLADE)
	buffer[0] = (s16) le16_to_cpu(buf[1])*(-1)*4;
	buffer[1] = (s16) le16_to_cpu(buf[0])*4;
	buffer[2] = (s16) le16_to_cpu(buf[2])*(-1)*4;
#elif defined(CONFIG_MACH_NOVA)
	buffer[0] = (s16) le16_to_cpu(buf[1])*(-1)*4;
	buffer[1] = (s16) le16_to_cpu(buf[0])*4;
	buffer[2] = (s16) le16_to_cpu(buf[2])*(-1)*4;
#elif defined(CONFIG_MACH_V9)
	buffer[0] = (s16) le16_to_cpu(buf[1])*(-1)*4;
	buffer[1] = (s16) le16_to_cpu(buf[0])*(-1)*4;
	buffer[2] = (s16) le16_to_cpu(buf[2])*4;
#else
	buffer[0] = (s16) le16_to_cpu(buf[0])*(-1)*4;
	buffer[1] = (s16) le16_to_cpu(buf[1])*(-1)*4;
	buffer[2] = (s16) le16_to_cpu(buf[2])*(-1)*4;
#endif  //default configration

	return 0;
}

static void adxl34x_disable(struct adxl34x *ac)
{
	mutex_lock(&ac->mutex);
	if (!ac->disabled && ac->opened) {
		ac->disabled = 1;
		/*
		 * A '0' places the ADXL34x into standby mode
		 * with minimum power consumption.
		 */
		AC_WRITE(ac, POWER_CTL, 0);
	}
	mutex_unlock(&ac->mutex);
}

static void adxl34x_enable(struct adxl34x *ac)
{
	mutex_lock(&ac->mutex);
	if (ac->disabled && ac->opened) {
		AC_WRITE(ac, POWER_CTL, ac->pdata.power_mode | PCTL_MEASURE);
		ac->disabled = 0;
	}
	mutex_unlock(&ac->mutex);
}


static int adxl34x_set_mode(short mode)
{
	int ret = 0;
	
	if(mode) {
		ret = adxl34x_i2c_write(this_client, POWER_CTL, PCTL_MEASURE);
		if(ret < 0)
			printk(ADXL34X_TAG"%s call adxl34x_i2c_write error",__func__);
	}else {
		ret = adxl34x_i2c_write(this_client, POWER_CTL, PCTL_STANDBY);
		if(ret < 0)
			printk(ADXL34X_TAG"%s call adxl34x_i2c_write error", __func__);
		}
	return ret;

}

static ssize_t adxl34x_disable_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct adxl34x *ac = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", ac->disabled);
}

static ssize_t adxl34x_disable_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct adxl34x *ac = dev_get_drvdata(dev);
	unsigned long val;
	int error;

	error = strict_strtoul(buf, 10, &val);
	if (error)
		return error;

	if (val)
		adxl34x_disable(ac);
	else
		adxl34x_enable(ac);

	return count;
}

static DEVICE_ATTR(disable, 0664, adxl34x_disable_show, adxl34x_disable_store);

static ssize_t adxl34x_calibrate_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	struct adxl34x *ac = dev_get_drvdata(dev);
	ssize_t count;

	mutex_lock(&ac->mutex);
	count = sprintf(buf, "%d,%d,%d\n", ac->hwcal.x * 4 + ac->swcal.x,
			ac->hwcal.y * 4 + ac->swcal.y,
			ac->hwcal.z * 4 + ac->swcal.z);
	mutex_unlock(&ac->mutex);

	return count;
}

static ssize_t adxl34x_calibrate_store(struct device *dev,
				       struct device_attribute *attr,
				       const char *buf, size_t count)
{
	struct adxl34x *ac = dev_get_drvdata(dev);

	/*
	 * Hardware offset calibration has a resolution of 15.6 mg/LSB.
	 * We use HW calibration and handle the remaining bits in SW. (4mg/LSB)
	 */

	mutex_lock(&ac->mutex);
	ac->hwcal.x -= (ac->saved.x / 4);
	ac->swcal.x = ac->saved.x % 4;

	ac->hwcal.y -= (ac->saved.y / 4);
	ac->swcal.y = ac->saved.y % 4;

	ac->hwcal.z -= (ac->saved.z / 4);
	ac->swcal.z = ac->saved.z % 4;

	AC_WRITE(ac, OFSX, (s8) ac->hwcal.x);
	AC_WRITE(ac, OFSY, (s8) ac->hwcal.y);
	AC_WRITE(ac, OFSZ, (s8) ac->hwcal.z);
	mutex_unlock(&ac->mutex);

	return count;
}

static DEVICE_ATTR(calibrate, 0664, adxl34x_calibrate_show,
		   adxl34x_calibrate_store);

static ssize_t adxl34x_rate_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct adxl34x *ac = dev_get_drvdata(dev);
	ssize_t count;

	mutex_lock(&ac->mutex);
	count = sprintf(buf, "%u\n", RATE(ac->pdata.data_rate));
	mutex_unlock(&ac->mutex);

	return count;
}

static ssize_t adxl34x_rate_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct adxl34x *ac = dev_get_drvdata(dev);
	unsigned long val;
	int error;

	mutex_lock(&ac->mutex);
	error = strict_strtoul(buf, 10, &val);
	if (error)
		return error;

	ac->pdata.data_rate = RATE(val);

	AC_WRITE(ac, BW_RATE, ac->pdata.data_rate |
		 (ac->pdata.low_power_mode ? LOW_POWER : 0));
	mutex_unlock(&ac->mutex);

	return count;
}

static DEVICE_ATTR(rate, 0664, adxl34x_rate_show, adxl34x_rate_store);

static ssize_t adxl34x_autosleep_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct adxl34x *ac = dev_get_drvdata(dev);
	ssize_t count;

	mutex_lock(&ac->mutex);
	count = sprintf(buf, "%u\n", ac->pdata.power_mode &
		(PCTL_AUTO_SLEEP | PCTL_LINK) ? 1 : 0);
	mutex_unlock(&ac->mutex);

	return count;
}

static ssize_t adxl34x_autosleep_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct adxl34x *ac = dev_get_drvdata(dev);
	unsigned long val;
	int error;

	mutex_lock(&ac->mutex);
	error = strict_strtoul(buf, 10, &val);
	if (error)
		return error;

	if (val)
		ac->pdata.power_mode |= (PCTL_AUTO_SLEEP | PCTL_LINK);
	else
		ac->pdata.power_mode &= ~(PCTL_AUTO_SLEEP | PCTL_LINK);

	if (!ac->disabled && ac->opened)
		AC_WRITE(ac, POWER_CTL, ac->pdata.power_mode | PCTL_MEASURE);

	mutex_unlock(&ac->mutex);

	return count;
}

static DEVICE_ATTR(autosleep, 0664, adxl34x_autosleep_show,
		   adxl34x_autosleep_store);

#ifdef ADXL_DEBUG
static ssize_t adxl34x_write_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct adxl34x *ac = dev_get_drvdata(dev);
	unsigned long val;
	int error;

	/*
	 * This allows basic ADXL register write access for debug purposes.
	 */
	mutex_lock(&ac->mutex);
	error = strict_strtoul(buf, 16, &val);
	if (error)
		return error;

	AC_WRITE(ac, val >> 8, val & 0xFF);
	mutex_unlock(&ac->mutex);

	return count;
}

static DEVICE_ATTR(write, 0664, NULL, adxl34x_write_store);
#endif

static struct attribute *adxl34x_attributes[] = {
	&dev_attr_disable.attr,
	&dev_attr_calibrate.attr,
	&dev_attr_rate.attr,
	&dev_attr_autosleep.attr,
#ifdef ADXL_DEBUG
	&dev_attr_write.attr,
#endif
	NULL
};

static const struct attribute_group adxl34x_attr_group = {
	.attrs = adxl34x_attributes,
};

static int __devinit adxl34x_initialize(bus_device *bus, struct adxl34x *ac)
{
	int ret, err ;
	//struct input_dev *input_dev;
	struct adxl34x_platform_data *devpd = bus->dev.platform_data;
	struct adxl34x_platform_data *pdata;

	unsigned char revid = 0X00;

	if (!devpd) {
		dev_dbg(&bus->dev,
			"No platfrom data: Using default initialization\n");
		devpd = (struct adxl34x_platform_data *)&adxl34x_default_init;
	}

	memcpy(&ac->pdata, devpd, sizeof(ac->pdata));
	pdata = &ac->pdata;
	
	ac->disabled = 1;
	mutex_init(&ac->mutex);

	ret = ac->read(bus, DEVID);
	if(ret < 0)
		printk(ADXL34X_TAG"i2c read chipid fail,ret = %d\n",ret);
	else{
		revid = buffer_store; /// check return result
		printk(ADXL34X_TAG"i2c read success,revid =%d\n",revid);
	}

	switch (revid) {
	case ID_ADXL345:
		ac->model = 345;
		break;
	case ID_ADXL346:
		ac->model = 346;
		break;
	default:
		printk(ADXL34X_TAG"Read revid = %x ,Failed to probe\n",revid);
		err = -ENODEV;
		goto err_free_mem;
	}

	if (pdata->ev_code_ff) {
		ac->int_mask = FREE_FALL;
		//__set_bit(pdata->ev_code_ff, input_dev->keybit);
	}

	if (pdata->ev_code_act_inactivity){
		//__set_bit(pdata->ev_code_act_inactivity, input_dev->keybit);
	}

	ac->int_mask |= ACTIVITY | INACTIVITY;

	if (pdata->watermark) {
		ac->int_mask |= WATERMARK;
		if (!FIFO_MODE(pdata->fifo_mode))
			pdata->fifo_mode |= FIFO_STREAM;
	} else {
		ac->int_mask |= DATA_READY;
	}

	if (pdata->tap_axis_control & (TAP_X_EN | TAP_Y_EN | TAP_Z_EN))
		ac->int_mask |= SINGLE_TAP | DOUBLE_TAP;

	if (FIFO_MODE(pdata->fifo_mode) == FIFO_BYPASS)
		ac->fifo_delay = 0;

	ac->write(bus, POWER_CTL, 0);
#if 0
	err = request_irq(bus->irq, adxl34x_irq,
			  IRQF_TRIGGER_HIGH, bus->dev.driver->name, ac);

	if (err) {
		dev_err(&bus->dev, "irq %d busy?\n", bus->irq);
		goto err_free_mem;
	}

	err = sysfs_create_group(&bus->dev.kobj, &adxl34x_attr_group);
	if (err)
		goto err_free_irq;

	err = input_register_device(input_dev);
	if (err)
		goto err_remove_attr;
#endif

	AC_WRITE(ac, THRESH_TAP, pdata->tap_threshold);
	AC_WRITE(ac, OFSX, pdata->x_axis_offset);
	ac->hwcal.x = pdata->x_axis_offset;
	AC_WRITE(ac, OFSY, pdata->y_axis_offset);
	ac->hwcal.y = pdata->y_axis_offset;
	AC_WRITE(ac, OFSZ, pdata->z_axis_offset);
	ac->hwcal.z = pdata->z_axis_offset;
	AC_WRITE(ac, THRESH_TAP, pdata->tap_threshold);
	AC_WRITE(ac, DUR, pdata->tap_duration);
	AC_WRITE(ac, LATENT, pdata->tap_latency);
	AC_WRITE(ac, WINDOW, pdata->tap_window);
	AC_WRITE(ac, THRESH_ACT, pdata->activity_threshold);
	AC_WRITE(ac, THRESH_INACT, pdata->inactivity_threshold);
	AC_WRITE(ac, TIME_INACT, pdata->inactivity_time);
	AC_WRITE(ac, THRESH_FF, pdata->free_fall_threshold);
	AC_WRITE(ac, TIME_FF, pdata->free_fall_time);
	AC_WRITE(ac, TAP_AXES, pdata->tap_axis_control);
	AC_WRITE(ac, ACT_INACT_CTL, pdata->act_axis_control);
	AC_WRITE(ac, BW_RATE, RATE(ac->pdata.data_rate) |
		 (pdata->low_power_mode ? LOW_POWER : 0));
	AC_WRITE(ac, DATA_FORMAT, pdata->data_range);
	AC_WRITE(ac, FIFO_CTL, FIFO_MODE(pdata->fifo_mode) |
		 SAMPLES(pdata->watermark));
	AC_WRITE(ac, INT_MAP, 0);	/* Map all INTs to INT1 */
	AC_WRITE(ac, INT_ENABLE, ac->int_mask | OVERRUN);

	pdata->power_mode &= (PCTL_AUTO_SLEEP | PCTL_LINK);
#if 0
	dev_info(&bus->dev, "ADXL%d accelerometer, irq %d\n",
	//	 ac->model, bus->irq);
#endif
	return 0;
#if 0
err_remove_attr:
	sysfs_remove_group(&bus->dev.kobj, &adxl34x_attr_group);
err_free_irq:
	free_irq(bus->irq, ac);
#endif

err_free_mem:
	//input_free_device(input_dev);
	return err;
}

static int adxl34x_open(struct inode *inode, struct file *file)
{
	return nonseekable_open(inode, file);
}


static int lis302dl_open(struct inode *inode, struct file *file)
{
	return nonseekable_open(inode, file);
}

static int adxl34x_release(struct inode *inode, struct file *file)
{
	/*Do not add here fot file operation*/
	//device_flag = -1; 
	return 0;
}

static int lis302dl_release(struct inode *inode, struct file *file)
{
	/*Do not add here fot file operation*/
	//device_flag = -1;
	return 0;
}

static int adxl34x_ioctl(struct inode *inode, struct file *file, unsigned int cmd,
	   unsigned long arg)
{

	void __user *argp = (void __user *)arg;

	char rwbuf[8];
	int ret = -1;
	short buf[8], temp;

	switch (cmd) {
	case ADXL34X_IOCTL_READ:
	case ADXL34X_IOCTL_WRITE:
	case ADXL34X_IOCTL_SET_MODE:
		if (copy_from_user(&rwbuf, argp, sizeof(rwbuf)))
			return -EFAULT;
		break;
	case ADXL34X_IOCTL_READ_ACCELERATION:
		if (copy_from_user(&buf, argp, sizeof(buf)))
			return -EFAULT;
		break;
	default:
		break;
	}

	switch (cmd) {
	case ADXL34X_IOCTL_INIT:
		ret = adxl34x_hw_init();
		if (ret < 0){
			printk(ADXL34X_TAG"%s ,ADXL34X_IOCTL_INIT return %d\n",__func__,ret);
			return ret;
		}
		break;
	case ADXL34X_IOCTL_READ:
		if (rwbuf[0] < 1)
			return -EINVAL;
		//ret = lis302dl_i2c_rx(&rwbuf[1], rwbuf[0]);
		if (ret < 0)
			return ret;
		break;
	case ADXL34X_IOCTL_WRITE:
		if (rwbuf[0] < 2)
			return -EINVAL;
		//ret = lis302dl_i2c_tx(&rwbuf[1], rwbuf[0]);
		if (ret < 0)
			return ret;
		break;
	case ADXL34X_IOCTL_READ_ACCELERATION:
		ret = adxl34x_get_triple(&buf[0]);
		if (ret < 0)
			return ret;
		break;
	case ADXL34X_IOCTL_SET_MODE:
		adxl34x_set_mode(rwbuf[0]);
		break;
	default:
		return -ENOTTY;
	}

	switch (cmd) {
	case ADXL34X_IOCTL_READ:
		if (copy_to_user(argp, &rwbuf, sizeof(rwbuf)))
			return -EFAULT;
		break;
	case ADXL34X_IOCTL_READ_ACCELERATION:
		if (copy_to_user(argp, &buf, sizeof(buf)))
			return -EFAULT;
		break;
	case ADXL34X_IOCTL_GET_INT:
		if (copy_to_user(argp, &temp, sizeof(temp)))
			return -EFAULT;
		break;
	default:
		break;
	}

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

static void adxl34x_early_suspend(struct early_suspend *handler)
{
	int ret = adxl34x_set_mode(0);
	if(!ret)
		pr_info(ADXL34X_TAG "adxl34x suspend\n");
	else
		pr_err(ADXL34X_TAG "adxl34x suspend failed \n");
}

static void lis302dl_early_suspend(struct early_suspend *handler)
{
	int ret = lis302dl_set_mode(0);
	if(!ret)
		pr_info(LIS302_TAG "lis302dl suspend\n");
	else
		pr_err(LIS302_TAG "lis302dl suspend  failed \n");
}

static void adxl34x_late_resume(struct early_suspend *handler)
{
	int ret = adxl34x_set_mode(1);

	if(!ret)
		pr_info(ADXL34X_TAG "adxl34x resume\n");
	else
		pr_err(ADXL34X_TAG "adxl34x resume failed \n");

}

static void lis302dl_late_resume(struct early_suspend *handler)
{
	int ret = lis302dl_set_mode(1);

	if(!ret)
		pr_info(LIS302_TAG "lis302dl resume\n");
	else
		pr_err(LIS302_TAG "lis302dl resume failed \n");

}

static struct file_operations adxl34x_fops = {
	.owner = THIS_MODULE,
	.open = adxl34x_open,
	.release = adxl34x_release,
	.ioctl = adxl34x_ioctl,
};

static struct miscdevice adxl34x_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "adxl34x",
	.fops = &adxl34x_fops,
};

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

static int  accel_i2c_probe(struct i2c_client *client,
				       const struct i2c_device_id *id)
{
	struct adxl34x *ac;
	struct adxl34x_data *adxl34x_data;
	struct lis302dl_data *lis302dl;
	int ret;
	unsigned char buf[2];
	int chipid = 0;;
	

	device_flag = -1; //int device_flag, no device was handle
	ret = i2c_check_functionality(client->adapter, I2C_FUNC_I2C);
	if (!ret) {
		dev_err(&client->dev, "I2C_FUNC_I2C not Supported\n");
		goto probe_failed;
	}
	
	printk("[gsensor]Start to debug gsensor driver in %s\n",__func__);

	this_client = client;
#if 0
	ret = i2c_master_recv(client, buf, 1); //now .start to check
	if (ret < 0){
		printk(ADXL34X_TAG"i2c_master_recv call error in %s ,ret = %d\n",__func__,ret);
		goto probe_failed;
	}else{
		printk(ADXL34X_TAG"i2c_master_recv call sucess in %s ,chipid = %d\n",__func__,buf[0]);
	}
	chipid = buf[0];
#endif
	ret = adxl34x_i2c_read(client, DEVID);
	if(ret < 0)
		pr_err(ADXL34X_TAG"adxl34x_i2c_read chipid fail in %s,ret = %d\n",__func__,ret);
	else{
		chipid = buffer_store; 
		pr_err("[fya@gsensor]i2c read success, ret = %x\n",chipid);
	}
	
	if((ID_ADXL345==chipid) || (ID_ADXL346==chipid)){
		device_flag = 1;
		pr_err(LIS302_TAG "use ADI gsenosr chip ,chipid = %x!\n",chipid);
	}else{
		/*the second is from ST*/
		buf[0] = LIS302DL_REG_WHO_AM_I;
#if defined(CONFIG_MATCH_V9)
	//ZTE_FYA_ADD_20101119 start +++++++++++++++++++
		ret = lis302dl_i2c_rx(buf, 1);
		if(ret < 0) {
			pr_err(LIS302_TAG "i2c-bus read error,at %s!\n",__func__);
			goto probe_failed;
		}

		device_flag = 0;
	//ZTE_FYA_ADD_20101119 end	--------------------	
		pr_err(LIS302_TAG"Do not to cheackout chipid for lis33de!\n");
#else
		ret = lis302dl_i2c_rx(buf, 1);
		if(ret < 0) {
			pr_err(LIS302_TAG "Checkout ST chips,i2c read error!\n");
			goto probe_failed;
		}
		else{
			pr_info(LIS302_TAG "Device id is %d\n", buf[0]);
			if(buf[0]!= LIS302DL_WHO_AM_I_MAGIC) {
				pr_err(LIS302_TAG "Device id not match,it is not from ST.\n");
				goto probe_failed;
			}
		
			device_flag = 0;
			pr_err(LIS302_TAG "Use ST gsenosr chip!\n");
		}			
#endif
	}
	if(1 == device_flag){
		ac = kzalloc(sizeof(struct adxl34x), GFP_KERNEL);
		if (!ac){
			kfree(ac);
		    goto probe_failed;
		}
		ac->bus = client;
		i2c_set_clientdata(client, ac);
		ac->read_block = adxl34x_i2c_master_read_block_data;
		ac->read = adxl34x_i2c_read;
		ac->write = adxl34x_i2c_write;

		ret = adxl34x_initialize(client, ac);
		if (ret) {
			i2c_set_clientdata(client, NULL);
			kfree(ac);
			goto probe_failed;
		}

		adxl34x_data = kzalloc(sizeof(struct adxl34x_data), GFP_KERNEL);
		if (!adxl34x_data) {
			kfree(adxl34x_data);
		    goto probe_failed;
		}
	
		ret = misc_register(&adxl34x_device);
		if (ret) {
			pr_err(ADXL34X_TAG "%s: adxl34x_device register failed\n",__func__);
		    goto probe_failed;
		}else
			pr_err(ADXL34X_TAG "%s: adxl34x_device register OK\n",__func__);
		adxl34x_data->early_suspend.suspend = adxl34x_early_suspend;
		adxl34x_data->early_suspend.resume = adxl34x_late_resume;
		register_early_suspend(&adxl34x_data->early_suspend);

		return 0;
	}else if(0 == device_flag){
		lis302dl = kzalloc(sizeof(struct lis302dl_data), GFP_KERNEL);
		if (!lis302dl) {
			kfree(lis302dl);
		    goto probe_failed;
		}
		
		i2c_set_clientdata(client, lis302dl);
		//lis302dl_pdata = client->dev.platform_data;
		lis302dl_pdata = &gsensor;

		if (lis302dl_pdata == NULL) {
			pr_err(LIS302_TAG "lis302dl_init_client: platform data is NULL\n");
			kfree(lis302dl_pdata);
		}
		
		ret = lis302dl_hw_init(lis302dl_pdata);
		if (ret < 0) 
			pr_err(LIS302_TAG "lis302dl_probe: lis302dl_init failed\n");
		
		ret = misc_register(&lis302dl_device);
		if (ret) {
			pr_err(LIS302_TAG "lis302dl_probe: device register failed\n");
		    goto probe_failed;
		}else
			pr_err(LIS302_TAG "lis302dl_probe: lis302dl_device register OK\n");
		lis302dl->early_suspend.suspend = lis302dl_early_suspend;
		lis302dl->early_suspend.resume = lis302dl_late_resume;
		register_early_suspend(&lis302dl->early_suspend);

		return 0;
	}else{
		printk("[gsensor]Do not have any gsensor-chip in our product\n");
		goto probe_failed;
	}
probe_failed:
	return -ENOMEM;
}

static int  accel_i2c_remove(struct i2c_client *client)
{
	struct adxl34x *ac;
	struct adxl34x_data *adxl34x_data;
	struct lis302dl_data *lis302dl;

	if(1 == device_flag){
		ac = dev_get_drvdata(&client->dev);
		adxl34x_data = i2c_get_clientdata(client);
		unregister_early_suspend(&adxl34x_data->early_suspend);
		misc_deregister(&adxl34x_device);
		i2c_set_clientdata(client, NULL);
		kfree(adxl34x_data);
		kfree(ac);
	}else if(0 == device_flag){
		lis302dl = i2c_get_clientdata(client);
		unregister_early_suspend(&lis302dl->early_suspend);
		misc_deregister(&lis302dl_device);
		i2c_set_clientdata(client, NULL);
		kfree(lis302dl);
	}else{
		printk("No chip could be closed\n ");
	}

	device_flag = -1;
	return 0;
}


static const struct i2c_device_id accel_id[] = {
	{ "accelerator", 0 },
	{ }
};

static struct i2c_driver accel_driver = {
	.probe = accel_i2c_probe,
	.remove = accel_i2c_remove,
	.id_table = accel_id,
	.driver = {
		.name = "accelerator"
	}
};

static int __init accel_i2c_init(void)
{
	pr_info("[@gsensor]G-sensor driver: init\n");
	return i2c_add_driver(&accel_driver);
}

module_init(accel_i2c_init);

static void __exit accel_i2c_exit(void)
{
	pr_info("[@gsensor]G-sensor driver: exit\n");
	i2c_del_driver(&accel_driver);
}

module_exit(accel_i2c_exit);

MODULE_AUTHOR("feng.yuao@zte.com.cn>");
MODULE_DESCRIPTION("ADI345/346 && ST LIS302DL Three-Axis Digital Accelerometer Driver ");
MODULE_LICENSE("GPL");
