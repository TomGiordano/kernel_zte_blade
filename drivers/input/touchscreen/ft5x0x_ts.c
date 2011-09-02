/*drivers/input/keyboard/ft5x0x_ts.c
 *This file is used for FocalTech ft5x0x_ts touchscreen
 *
*/

/*
=======================================================================================================
When		Who	What,Where,Why		Comment			Tag
2011-04-20	zfj	add virtual key for P732A			ZFJ_TS_ZFJ_20110420     
2011-03-02	zfj	use create_singlethread_workqueue instead 	ZTE_TS_ZFJ_20110302 
2011-01-08	zfj	Create file			
*/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>
#include "ft5x0x_ts.h"
#include <mach/gpio.h>

#if defined(CONFIG_MACH_BLADE)//P729B touchscreen enable
#define GPIO_TOUCH_EN_OUT  31
#elif defined(CONFIG_MACH_R750)//R750 touchscreen enable
#define GPIO_TOUCH_EN_OUT  33
#elif defined(CONFIG_MACH_TURIES)
#define GPIO_TOUCH_EN_OUT  89

#else
#define GPIO_TOUCH_EN_OUT  31
#endif

/*ZTE_TS_ZFJ_20110215 begin*/
#if defined(CONFIG_MACH_TURIES)
#define GPIO_TOUCH_INT_WAKEUP	18
#else
#define GPIO_TOUCH_INT_WAKEUP	29
#endif
/*ZTE_TS_ZFJ_20110215 end*/

#define ABS_SINGLE_TAP	0x21	/* Major axis of touching ellipse */
#define ABS_TAP_HOLD	0x22	/* Minor axis (omit if circular) */
#define ABS_DOUBLE_TAP	0x23	/* Major axis of approaching ellipse */
#define ABS_EARLY_TAP	0x24	/* Minor axis (omit if circular) */
#define ABS_FLICK	0x25	/* Ellipse orientation */
#define ABS_PRESS	0x26	/* Major axis of touching ellipse */
#define ABS_PINCH 	0x27	/* Minor axis (omit if circular) */

#if defined(CONFIG_TOUCHSCREEN_VIRTUAL_KEYS)
#define virtualkeys virtualkeys.Fts-touchscreen
#if defined (CONFIG_MACH_RACER2)
static const char ts_keys_size[] = "0x01:217:40:340:60:34:0x01:139:120:340:60:34:0x01:158:200:340:60:34";
#else
//static const char ts_keys_size[] = "0x01:139:40:920:60:40:0x01:102:165:920:60:40:0x01:158:280:920:60:40:0x01:217:440:920:60:40";
static const char ts_keys_size[] = "0x01:139:30:520:50:80:0x01:102:110:520:60:80:0x01:158:200:520:60:80:0x01:217:300:520:60:80";
//static const char ts_keys_size[] = "0x01:139:40:115:80:54:0x01:102:180:115:80:54:0x01:158:300:115:80:54:0x01:217:440:115:80:54";
#endif
static ssize_t virtualkeys_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	sprintf(buf,"%s\n",ts_keys_size);
//	pr_info("%s\n",__FUNCTION__);
    return strlen(ts_keys_size)+2;
}
static DEVICE_ATTR(virtualkeys, 0444, virtualkeys_show, NULL);
extern struct kobject *android_touch_kobj;
static struct kobject * virtual_key_kobj;
static int ts_key_report_init(void)
{
	int ret;
	virtual_key_kobj = kobject_get(android_touch_kobj);
	if (virtual_key_kobj == NULL) {
		virtual_key_kobj = kobject_create_and_add("board_properties", NULL);
		if (virtual_key_kobj == NULL) {
			pr_err("%s: subsystem_register failed\n", __func__);
			ret = -ENOMEM;
			return ret;
		}
	}
 
	ret = sysfs_create_file(virtual_key_kobj, &dev_attr_virtualkeys.attr);
	if (ret) {
		pr_err("%s: sysfs_create_file failed\n", __func__);
		return ret;
	}
	pr_info("%s:virtual key init succeed!\n", __func__);
	return 0;
}
static void ts_key_report_deinit(void)
{
	sysfs_remove_file(virtual_key_kobj, &dev_attr_virtualkeys.attr);
	kobject_del(virtual_key_kobj);
}

#endif


static struct workqueue_struct *Fts_wq;
static struct i2c_driver Fts_ts_driver;

struct Fts_ts_data
{
	uint16_t addr;
	struct i2c_client *client;
	struct input_dev *input_dev;
	int use_irq;
	struct hrtimer timer;
	struct work_struct  work;
	uint16_t max[2];
	struct early_suspend early_suspend;
};
#ifdef CONFIG_MACH_SKATE
#define CONFIG_SUPPORT_FTS_CTP_UPG
#endif

#if defined(CONFIG_SUPPORT_FTS_CTP_UPG)

static struct i2c_client *update_client;
static int update_result_flag=0;
#endif
static int Fts_i2c_read(struct i2c_client *client, int reg, u8 * buf, int count)
{
    int rc;
    int ret = 0;

    buf[0] = reg;
    rc = i2c_master_send(client, buf, 1);
    if (rc != 1)
    {
        dev_err(&client->dev, "Fts_i2c_read FAILED: read of register %d\n", reg);
        ret = -1;
        goto tp_i2c_rd_exit;
    }
    rc = i2c_master_recv(client, buf, count);
    if (rc != count)
    {
        dev_err(&client->dev, "Fts_i2c_read FAILED: read %d bytes from reg %d\n", count, reg);
        ret = -1;
    }

  tp_i2c_rd_exit:
    return ret;
}
static int Fts_i2c_write(struct i2c_client *client, int reg, u8 data)
{
    u8 buf[2];
    int rc;
    int ret = 0;

    buf[0] = reg;
    buf[1] = data;
    rc = i2c_master_send(client, buf, 2);
    if (rc != 2)
    {
        dev_err(&client->dev, "Fts_i2c_write FAILED: writing to reg %d\n", reg);
        ret = -1;
    }
    return ret;
}

#ifdef CONFIG_SUPPORT_FTS_CTP_UPG

typedef enum
{
    ERR_OK,
    ERR_MODE,
    ERR_READID,
    ERR_ERASE,
    ERR_STATUS,
    ERR_ECC,
    ERR_DL_ERASE_FAIL,
    ERR_DL_PROGRAM_FAIL,
    ERR_DL_VERIFY_FAIL
}E_UPGRADE_ERR_TYPE;

typedef unsigned char         FTS_BYTE;     //8 bit
typedef unsigned short        FTS_WORD;    //16 bit
typedef unsigned int          FTS_DWRD;    //16 bit
typedef unsigned char         FTS_BOOL;    //8 bit

#define FTS_NULL                0x0
#define FTS_TRUE                0x01
#define FTS_FALSE              0x0

void delay_ms(FTS_WORD  w_ms)
{
    //platform related, please implement this function
    msleep( w_ms );
}

/*
[function]: 
    send a command to ctpm.
[parameters]:
    btcmd[in]        :command code;
    btPara1[in]    :parameter 1;    
    btPara2[in]    :parameter 2;    
    btPara3[in]    :parameter 3;    
    num[in]        :the valid input parameter numbers, if only command code needed and no parameters followed,then the num is 1;    
[return]:
    FTS_TRUE    :success;
    FTS_FALSE    :io fail;
*/
FTS_BOOL cmd_write(struct i2c_client *client,FTS_BYTE btcmd,FTS_BYTE btPara1,FTS_BYTE btPara2,FTS_BYTE btPara3,FTS_BYTE num)
{
    FTS_BYTE write_cmd[4] = {0};

    write_cmd[0] = btcmd;
    write_cmd[1] = btPara1;
    write_cmd[2] = btPara2;
    write_cmd[3] = btPara3;
    return i2c_master_send(client, write_cmd, num);
}

/*
[function]: 
    write data to ctpm , the destination address is 0.
[parameters]:
    pbt_buf[in]    :point to data buffer;
    bt_len[in]        :the data numbers;    
[return]:
    FTS_TRUE    :success;
    FTS_FALSE    :io fail;
*/
FTS_BOOL byte_write(struct i2c_client *client,FTS_BYTE* pbt_buf, FTS_DWRD dw_len)
{
    
    return i2c_master_send(client, pbt_buf, dw_len);
}

/*
[function]: 
    read out data from ctpm,the destination address is 0.
[parameters]:
    pbt_buf[out]    :point to data buffer;
    bt_len[in]        :the data numbers;    
[return]:
    FTS_TRUE    :success;
    FTS_FALSE    :io fail;
*/
FTS_BOOL byte_read(struct i2c_client *client,FTS_BYTE* pbt_buf, FTS_BYTE bt_len)
{
    return i2c_master_recv(client, pbt_buf, bt_len);
}


/*
[function]: 
    burn the FW to ctpm.
[parameters]:(ref. SPEC)
    pbt_buf[in]    :point to Head+FW ;
    dw_lenth[in]:the length of the FW + 6(the Head length);    
    bt_ecc[in]    :the ECC of the FW
[return]:
    ERR_OK        :no error;
    ERR_MODE    :fail to switch to UPDATE mode;
    ERR_READID    :read id fail;
    ERR_ERASE    :erase chip fail;
    ERR_STATUS    :status error;
    ERR_ECC        :ecc error.
*/


#define    FTS_PACKET_LENGTH        122//250//122//26/10/2

static unsigned char CTPM_FW[]=
{
#include "ft_app1.i"
};

E_UPGRADE_ERR_TYPE  fts_ctpm_fw_upgrade(struct i2c_client *client,FTS_BYTE* pbt_buf, FTS_DWRD dw_lenth)
{
//    FTS_BYTE cmd_len     = 0;
    FTS_BYTE reg_val[2] = {0};
    FTS_DWRD i = 0;
//    FTS_BYTE ecc = 0;

    FTS_DWRD  packet_number;
    FTS_DWRD  j;
    FTS_DWRD  temp;
    FTS_DWRD  lenght;
    FTS_BYTE  packet_buf[FTS_PACKET_LENGTH + 6];
    FTS_BYTE  auc_i2c_write_buf[10];
    FTS_BYTE bt_ecc;

    /*********Step 1:Reset  CTPM *****/
    /*write 0xaa to register 0xfc*/
    Fts_i2c_write(client,0xfc,0xaa);
    delay_ms(50);
     /*write 0x55 to register 0xfc*/
    Fts_i2c_write(client,0xfc,0x55);
    printk("Step 1: Reset CTPM test\n");

    delay_ms(40);

    /*********Step 2:Enter upgrade mode *****/
     auc_i2c_write_buf[0] = 0x55;
     auc_i2c_write_buf[1] = 0xaa;
     i2c_master_send(client, auc_i2c_write_buf, 2);
     printk("Step 2: Enter update mode. \n");

    /*********Step 3:check READ-ID***********************/
    /*send the opration head*/
    do{
        if(i > 3)
        {
            return ERR_READID; 
        }
        /*read out the CTPM ID*/
        
        cmd_write(client,0x90,0x00,0x00,0x00,4);
        byte_read(client,reg_val,2);
        i++;
        printk("Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",reg_val[0],reg_val[1]);
    }while(reg_val[0] != 0x79 || reg_val[1] != 0x03);

     /*********Step 4:erase app*******************************/
    cmd_write(client,0x61,0x00,0x00,0x00,1);
    delay_ms(1500);
    printk("Step 4: erase. \n");

    /*********Step 5:write firmware(FW) to ctpm flash*********/
    bt_ecc = 0;
    printk("Step 5: start upgrade. \n");
    dw_lenth = dw_lenth - 8;
    packet_number = (dw_lenth) / FTS_PACKET_LENGTH;
    packet_buf[0] = 0xbf;
    packet_buf[1] = 0x00;
    for (j=0;j<packet_number;j++)
    {
        temp = j * FTS_PACKET_LENGTH;
        packet_buf[2] = (FTS_BYTE)(temp>>8);
        packet_buf[3] = (FTS_BYTE)temp;
        lenght = FTS_PACKET_LENGTH;
        packet_buf[4] = (FTS_BYTE)(lenght>>8);
        packet_buf[5] = (FTS_BYTE)lenght;

        for (i=0;i<FTS_PACKET_LENGTH;i++)
        {
            packet_buf[6+i] = pbt_buf[j*FTS_PACKET_LENGTH + i]; 
            bt_ecc ^= packet_buf[6+i];
        }
        
        byte_write(client,&packet_buf[0],FTS_PACKET_LENGTH + 6);
        delay_ms(FTS_PACKET_LENGTH/6 + 1);
        if ((j * (FTS_PACKET_LENGTH + 6) % 1024) == 0)
        {
              printk("upgrade the 0x%x th byte.\n", ((unsigned int)j) * FTS_PACKET_LENGTH);
        }
    }

    if ((dw_lenth) % FTS_PACKET_LENGTH > 0)
    {
        temp = packet_number * FTS_PACKET_LENGTH;
        packet_buf[2] = (FTS_BYTE)(temp>>8);
        packet_buf[3] = (FTS_BYTE)temp;

        temp = (dw_lenth) % FTS_PACKET_LENGTH;
        packet_buf[4] = (FTS_BYTE)(temp>>8);
        packet_buf[5] = (FTS_BYTE)temp;

        for (i=0;i<temp;i++)
        {
            packet_buf[6+i] = pbt_buf[ packet_number*FTS_PACKET_LENGTH + i]; 
            bt_ecc ^= packet_buf[6+i];
        }

        byte_write(client,&packet_buf[0],temp+6);    
        delay_ms(20);
    }

    //send the last six byte
    for (i = 0; i<6; i++)
    {
        temp = 0x6ffa + i;
        packet_buf[2] = (FTS_BYTE)(temp>>8);
        packet_buf[3] = (FTS_BYTE)temp;
        temp =1;
        packet_buf[4] = (FTS_BYTE)(temp>>8);
        packet_buf[5] = (FTS_BYTE)temp;
        packet_buf[6] = pbt_buf[ dw_lenth + i]; 
        bt_ecc ^= packet_buf[6];

        byte_write(client,&packet_buf[0],7);  
        delay_ms(20);
    }

    /*********Step 6: read out checksum***********************/
    /*send the opration head*/
    cmd_write(client,0xcc,0x00,0x00,0x00,1);
    byte_read(client,reg_val,1);
    printk("Step 6:  ecc read 0x%x, new firmware 0x%x. \n", reg_val[0], bt_ecc);
    if(reg_val[0] != bt_ecc)
    {
        return ERR_ECC;
    }

    /*********Step 7: reset the new FW***********************/
    cmd_write(client,0x07,0x00,0x00,0x00,1);

    return ERR_OK;
}
#if 0
unsigned char fts_ctpm_get_upg_ver(void)
{
    unsigned int ui_sz;
    ui_sz = sizeof(CTPM_FW);
    if (ui_sz > 2)
    {
        return CTPM_FW[ui_sz - 2];
    }
    else
    {
        //TBD, error handling?
        return 0xff; //default value
    }
}
#endif

#endif

static int
proc_read_val(char *page, char **start, off_t off, int count, int *eof,
	  void *data)
{
	int len = 0;
	len += sprintf(page + len, "%s\n", "touchscreen module");
	len += sprintf(page + len, "name     : %s\n", "FocalTech");
	#if defined(CONFIG_MACH_R750)
	len += sprintf(page + len, "i2c address  : %x\n", 0x3E);
	#else
	len += sprintf(page + len, "i2c address  : 0x%x\n", 0x3E);
	#endif
	len += sprintf(page + len, "IC type    : %s\n", "FT5X06");
	len += sprintf(page + len, "module : %s\n", "FocalTech FT5x06");
	#ifdef CONFIG_SUPPORT_FTS_CTP_UPG
	  len += sprintf(page + len, "update flag : 0x%x\n", update_result_flag);
	#endif
	if (off + count >= len)
		*eof = 1;
	if (len < off)
		return 0;
	*start = page + off;
	return ((count < len - off) ? count : len - off);
}

static int proc_write_val(struct file *file, const char *buffer,
           unsigned long count, void *data)
{
		unsigned long val;
		sscanf(buffer, "%lu", &val);

#ifdef CONFIG_SUPPORT_FTS_CTP_UPG
	//if (  fts_ctpm_get_upg_ver() > update_ver)
	{
		printk("Fts Upgrade Start\n");
		update_result_flag=0;

		fts_ctpm_fw_upgrade(update_client,CTPM_FW, sizeof(CTPM_FW));
		//fts_ctpm_fw_upgrade_with_i_file(ts->client);//Update the CTPM firmware if need
		
		update_result_flag=2;
	}
#endif		
		return -EINVAL;
}

static void Fts_ts_work_func(struct work_struct *work)
{
	int ret, x, y, z, finger, w, x2, y2,w2,z2,finger2,pressure,pressure2;
	uint8_t buf[15];
	struct Fts_ts_data *ts = container_of(work, struct Fts_ts_data, work);


	ret = Fts_i2c_read(ts->client, 0x00, buf, 15); 
	if (ret < 0){
   		printk(KERN_ERR "Fts_ts_work_func: Fts_i2c_write failed, go to poweroff.\n");
	    	gpio_direction_output(GPIO_TOUCH_EN_OUT, 0);
	    	msleep(200);
	    	gpio_direction_output(GPIO_TOUCH_EN_OUT, 1);
	    	msleep(220);
	}
	else
	{
		//printk("Fts Touch Event: P1=%x,P2=%x,Points=%x\n",buf[3] ,buf[9],buf[2] );
		x = (uint16_t)((buf[3] & 0x0F)<<8 )| (uint16_t)buf[4];	
		y = (uint16_t)((buf[5] & 0x0F)<<8) | (uint16_t)buf[6];
		//pressure = ( (buf[3] & 0xF0) == 0x40 )?0:200;
		//w = 10;
		pressure = buf[7];
		w = buf[8];
		z = 1;
		finger = (buf[5] & 0xF0)>>4;
		
		x2 = (uint16_t)((buf[9] & 0x0F)<<8) | (uint16_t)buf[10];			
		y2 = (uint16_t)((buf[11] & 0x0F)<<8) | (uint16_t)buf[12];		
		//pressure2 = ( (buf[9] & 0xF0) == 0x40 )?0:200;
		//w2 = 10;
		pressure2 = buf[13];
		w2 = buf[14]; 
		z2 = 1;
		finger2 = (buf[2] & 0x07)>1?1:0;

		input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, pressure);
		input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 10);
		input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
		input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);
		input_mt_sync(ts->input_dev);

		if(finger2)
		{
			input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, pressure2);
			input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, 10);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x2);
			input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y2);
			input_mt_sync(ts->input_dev);
		}
			
		input_sync(ts->input_dev);
	}
	if (ts->use_irq)
		enable_irq(ts->client->irq);
}

static irqreturn_t Fts_ts_irq_handler(int irq, void *dev_id)
{
	struct Fts_ts_data *ts = dev_id;

	disable_irq_nosync(ts->client->irq);
	queue_work(Fts_wq, &ts->work);

	return IRQ_HANDLED;
}

static int Fts_ts_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int ret = 0;
	struct Fts_ts_data *ts;
	
	//ts = container_of(client, struct Fts_ts_data , client);
	ts = i2c_get_clientdata(client);
	disable_irq(client->irq);
	ret = cancel_work_sync(&ts->work);
	if(ret & ts->use_irq)
		enable_irq(client->irq);
	//flush_workqueue(ts->work);
	// ==set mode ==, 
	//ft5x0x_set_reg(FT5X0X_REG_PMODE, PMODE_HIBERNATE);
	Fts_i2c_write(client, FT5X0X_REG_PMODE, PMODE_HIBERNATE);
	gpio_direction_output(GPIO_TOUCH_INT_WAKEUP,1);
	return 0;
}

static int Fts_ts_resume(struct i2c_client *client)
{
	uint8_t buf,retry=0;

Fts_resume_start:	
	//gpio_direction_output(GPIO_TOUCH_EN_OUT,1);
	//gpio_direction_output(GPIO_TOUCH_INT_WAKEUP,1);
	//msleep(250);
	gpio_set_value(GPIO_TOUCH_INT_WAKEUP,0);
	msleep(5);
	gpio_set_value(GPIO_TOUCH_INT_WAKEUP,1);
	msleep(5);
	gpio_direction_input(GPIO_TOUCH_INT_WAKEUP);

	if ( Fts_i2c_read(client, FT5X0X_REG_FIRMID, &buf,1) < 0)
	{//I2C error read firmware ID
		printk("Fts FW ID read Error: retry=%x\n",retry);
		if ( ++retry < 3 )
			goto Fts_resume_start;
	}

	enable_irq(client->irq);
	
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void Fts_ts_early_suspend(struct early_suspend *h)
{
	struct Fts_ts_data *ts;
	
	ts = container_of(h, struct Fts_ts_data, early_suspend);
	Fts_ts_suspend(ts->client, PMSG_SUSPEND);
}

static void Fts_ts_late_resume(struct early_suspend *h)
{
	struct Fts_ts_data *ts;
	ts = container_of(h, struct Fts_ts_data, early_suspend);
	Fts_ts_resume(ts->client);
}
#endif

static int Fts_ts_probe(
	struct i2c_client *client, const struct i2c_device_id *id)
{
	struct Fts_ts_data *ts;
	int ret = 0;
	u8 fwVer;
	struct proc_dir_entry *dir, *refresh;
	u8 buf;
	
	ret = gpio_request(GPIO_TOUCH_EN_OUT, "touch voltage");
	if (ret)
	{	
		printk("gpio 31 request is error!\n");
		goto err_check_functionality_failed;
	}   

	gpio_direction_output(GPIO_TOUCH_EN_OUT, 1);
	msleep(250);
	
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
	{
		printk(KERN_ERR "Fts_ts_probe: need I2C_FUNC_I2C\n");
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}
	ts = kzalloc(sizeof(*ts), GFP_KERNEL);
	if (ts == NULL)
	{
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}

	INIT_WORK(&ts->work, Fts_ts_work_func);
	/*ZTE_TS_ZFJ_20110302 begin*/
	Fts_wq= create_singlethread_workqueue("Fts_wq");
	if(!Fts_wq)
	{
		ret = -ESRCH;
		pr_err("%s creare single thread workqueue failed!\n", __func__);
		goto err_create_singlethread;
	}
	/*ZTE_TS_ZFJ_20110302 end */
	ts->client = client;
	#ifdef CONFIG_SUPPORT_FTS_CTP_UPG
	update_client=client;
	#endif
	i2c_set_clientdata(client, ts);
	client->driver = &Fts_ts_driver;
	
	{
		int retry = 3;
		while (retry-- > 0)
		{
			ret = Fts_i2c_read(client, FT5X0X_REG_FIRMID, &buf,1);
			pr_info("wly: Fts_i2c_read, FT5X0X_REG_FIRMID = %d.\n", buf);
			if (0 == ret)
				break;
			msleep(10);
		}
		
		/*ZTE_TOUCH_WLY_005,@2009-12-19,begin*/
		if (retry < 0)
		{
			ret = -1;
			goto err_detect_failed;
		}
		/*ZTE_TOUCH_WLY_005,@2009-12-19,begin*/
	}
	
	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) {
		ret = -ENOMEM;
		printk(KERN_ERR "Fts_ts_probe: Failed to allocate input device\n");
		goto err_input_dev_alloc_failed;
	}
	
	ts->input_dev->name = "Fts-touchscreen";
	//ts->input_dev->phys = "Fts-touchscreen/input0";
	#if 0
	set_bit(EV_SYN, ts->input_dev->evbit);
	set_bit(EV_KEY, ts->input_dev->evbit);
	set_bit(BTN_TOUCH, ts->input_dev->keybit);
	set_bit(EV_ABS, ts->input_dev->evbit);
	set_bit(ABS_EARLY_TAP, ts->input_dev->absbit);
	set_bit(ABS_FLICK, ts->input_dev->absbit);
	set_bit(ABS_PRESS, ts->input_dev->absbit);
	set_bit(ABS_PINCH, ts->input_dev->absbit);
	set_bit(ABS_MT_TOUCH_MAJOR, ts->input_dev->absbit);
	set_bit(ABS_MT_POSITION_X, ts->input_dev->absbit);
	set_bit(ABS_MT_POSITION_Y, ts->input_dev->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, ts->input_dev->absbit);
	/*ZFJ_TS_ZFJ_20110420   begin*/  
	set_bit(KEY_MENU, ts->input_dev->keybit);
	set_bit(KEY_BACK, ts->input_dev->keybit);
	set_bit(KEY_HOME, ts->input_dev->keybit);
	set_bit(KEY_SEARCH, ts->input_dev->keybit);
	/*ZFJ_TS_ZFJ_20110420   end*/   
#endif	
	set_bit(EV_SYN, ts->input_dev->evbit);
	set_bit(EV_KEY, ts->input_dev->evbit);
	set_bit(BTN_TOUCH, ts->input_dev->keybit);
	set_bit(BTN_2, ts->input_dev->keybit);
	set_bit(EV_ABS, ts->input_dev->evbit);


	set_bit(KEY_HOME, ts->input_dev->keybit);
	set_bit(KEY_MENU, ts->input_dev->keybit);
	set_bit(KEY_BACK, ts->input_dev->keybit);
	set_bit(KEY_SEARCH, ts->input_dev->keybit);
	
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, SCREEN_MAX_X, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, SCREEN_MAX_Y, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 255, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_SINGLE_TAP, 0, 5, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_TAP_HOLD, 0, 5, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_EARLY_TAP, 0, 5, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_FLICK, 0, 5, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_PRESS, 0, 5, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_DOUBLE_TAP, 0, 5, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_PINCH, -255, 255, 0, 0);

	ret = input_register_device(ts->input_dev);
	if (ret)
	{
		printk(KERN_ERR "Fts_ts_probe: Unable to register %s input device\n", ts->input_dev->name);
		goto err_input_register_device_failed;
	}

   if (client->irq)
    {
        ret = request_irq(client->irq, Fts_ts_irq_handler, IRQF_TRIGGER_FALLING, "ft5x0x_ts", ts);
		if (ret == 0)
			ts->use_irq = 1;
		else
		{
			dev_err(&client->dev, "request_irq failed\n");
			goto err_input_request_irq_failed;
		}
	}
#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = Fts_ts_early_suspend;
	ts->early_suspend.resume = Fts_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif
	dir = proc_mkdir("touchscreen", NULL);
	refresh = create_proc_entry("ts_information", 0777, dir);
	if (refresh) {
		refresh->data		= NULL;
		refresh->read_proc  = proc_read_val;
		refresh->write_proc = proc_write_val;
	}
	printk(KERN_INFO "Fts_ts_probe: Start touchscreen %s in %s mode\n", ts->input_dev->name, ts->use_irq ? "interrupt" : "polling");

//Firmware Upgrade handle Start
	ret = Fts_i2c_read(ts->client, FT5X0X_REG_FIRMID, &fwVer,1) ;

	printk("Fts FW ID read ID = %x,ret = %x\n",fwVer,ret);
#if 0//def CONFIG_SUPPORT_FTS_CTP_UPG
	if (  fts_ctpm_get_upg_ver() > fwVer )
	{
		printk("Fts Upgrade Start\n");

		fts_ctpm_fw_upgrade(ts->client,CTPM_FW, sizeof(CTPM_FW));
		//fts_ctpm_fw_upgrade_with_i_file(ts->client);//Update the CTPM firmware if need
	}
#endif
//Firmware Upgrade handle End	
#if defined(CONFIG_TOUCHSCREEN_VIRTUAL_KEYS)
	ts_key_report_init();
#endif
	return 0;

err_input_request_irq_failed:
err_input_register_device_failed:
	input_free_device(ts->input_dev);

err_input_dev_alloc_failed:
err_detect_failed:
	kfree(ts);
	destroy_workqueue(Fts_wq);
err_create_singlethread: //ZTE_TS_ZFJ_20110302	
err_alloc_data_failed:
err_check_functionality_failed:
	gpio_free(GPIO_TOUCH_EN_OUT);
	return ret;
}

static int Fts_ts_remove(struct i2c_client *client)
{
	struct Fts_ts_data *ts = i2c_get_clientdata(client);
#if defined(CONFIG_TOUCHSCREEN_VIRTUAL_KEYS)
	ts_key_report_deinit();
#endif
	unregister_early_suspend(&ts->early_suspend);
	if (ts->use_irq)
		free_irq(client->irq, ts);
	else
		hrtimer_cancel(&ts->timer);
	input_unregister_device(ts->input_dev);
	kfree(ts);
	gpio_direction_output(GPIO_TOUCH_EN_OUT, 0);
	return 0;
}




static const struct i2c_device_id Fts_ts_id[] = {
	{ FT5X0X_NAME, 0 },
	{ }
};

static struct i2c_driver Fts_ts_driver = {
	.probe		= Fts_ts_probe,
	.remove		= Fts_ts_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= Fts_ts_suspend,
	.resume		= Fts_ts_resume,
#endif
	.id_table	= Fts_ts_id,
	.driver 	= {
		.name	= FT5X0X_NAME,
	},
};

static int __devinit Fts_ts_init(void)
{
	/*ZTE_TS_ZFJ_20110302 begin*/
	#if 0
	Fts_wq = create_rt_workqueue("Fts_wq");
	if (!Fts_wq)
		return -ENOMEM;
	#endif
	/*ZTE_TS_ZFJ_20110302 end*/
	return i2c_add_driver(&Fts_ts_driver);
}

static void __exit Fts_ts_exit(void)
{
	i2c_del_driver(&Fts_ts_driver);
	if (Fts_wq)
		destroy_workqueue(Fts_wq);
}

module_init(Fts_ts_init);
module_exit(Fts_ts_exit);

MODULE_DESCRIPTION("Fts Touchscreen Driver");
MODULE_LICENSE("GPL");
