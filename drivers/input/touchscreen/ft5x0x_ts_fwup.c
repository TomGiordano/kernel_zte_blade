#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>
#include "ft5x0x_ts_new.h"
#include <linux/fb.h>
#include <asm/uaccess.h>
#include <linux/fs.h>

static struct i2c_client *ftc_i2c_client;

static unsigned char CTPM_FW[]=
{
#if defined ( CONFIG_MACH_ROAMER)
#if defined (CONFIG_TOUCHSCREEN_MXT224_N760)
#include "ft_Ver10_20110705_036_9475_app_roamer.i" //ZTE_TS_XYM_20110711

#else
#include "Ver11_20110802_036_9475_Only4P736T_app.i"

#endif
#elif defined (CONFIG_MACH_RACER2)
#include "Ver17_20110721_028_P728T_app.i"
#elif defined (CONFIG_MACH_SKATE)
#include "Ver11_20110808_043_9485_skate_app.i"
#else
#endif
};


//#ifdef CONFIG_SUPPORT_FTS_CTP_UPG
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




//#define FTS_NULL		x0
//#define FTS_TRUE		x01
//#define FTS_FALSE		0x0
/*
void msleep(FTS_WORD  w_ms)
{
    //platform related, please implement this function
    msleep( w_ms );
}
*/
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


static int Fts_i2c_read(struct i2c_client *client, uint8_t reg, u8 * buf, int count) 
{
	int ret;
	struct i2c_msg msg[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = 1,
			.buf = &reg,
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = count,
			.buf = buf,
    }
	};

	ret = i2c_transfer(client->adapter, msg, 2);
  if ( ret != 2 )
    {
      dev_err(&client->dev, "Fts_i2c_read FAILED: read %d bytes from reg %d\n", count, reg);
      return -1;
    }
	return 0;
}


static int Fts_i2c_write(struct i2c_client *client, int reg, u8 data)
{
	int ret;
	struct i2c_msg msg;
  u8 buf[2];
  buf[0] = reg;
  buf[1] = data;
	msg.addr = client->addr;
	msg.flags = client->flags & I2C_M_TEN;
	msg.len = 2;
	msg.buf = (char *)buf;

	ret = i2c_transfer(client->adapter, &msg, 1);
	if ( ret != 1 )
    {
        dev_err(&client->dev, "Fts_i2c_write FAILED: writing to reg %d\n", reg);
        return -1;
    }

	return 0;
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
#define    FTS_SETTING_BUF_LEN      128

static int ft5x0x_GetFwSize(char * firmware_name)
{
	struct file* pfile = NULL;
	struct inode *inode;
	unsigned long magic; 
	off_t fsize = 0; 
	char filepath[128];

	memset(filepath, 0, sizeof(filepath));
	sprintf(filepath, "/sdcard/%s", firmware_name);
	pr_info("filepath=%s\n", filepath);
	if(NULL == pfile){
		pfile = filp_open(filepath, O_RDONLY, 0);
	}
	if(IS_ERR(pfile)){
		pr_err("error occured while opening file %s.\n", filepath);
		return -1;
	}

	inode=pfile->f_dentry->d_inode; 
	magic=inode->i_sb->s_magic;
	fsize=inode->i_size; 

	filp_close(pfile, NULL);

	return fsize;
}
static int ft5x0x_ReadFw(char * firmware_name, unsigned char * firmware_buf)
{
	struct file* pfile = NULL;
	struct inode *inode;
	unsigned long magic; 
	off_t fsize; 
	char filepath[128];
	loff_t pos;
	mm_segment_t old_fs;

	memset(filepath, 0, sizeof(filepath));
	sprintf(filepath, "/sdcard/%s", firmware_name);
	pr_info("filepath=%s\n", filepath);
	if(NULL == pfile){
		pfile = filp_open(filepath, O_RDONLY, 0);
	}
	if(IS_ERR(pfile)){
		pr_err("error occured while opening file %s.\n", filepath);
		return -1;
		}
	inode=pfile->f_dentry->d_inode; 
	magic=inode->i_sb->s_magic;
	fsize=inode->i_size; 
	//char * buf;
	old_fs = get_fs();
	set_fs(KERNEL_DS);
	pos = 0;

	vfs_read(pfile, firmware_buf, fsize, &pos);

	filp_close(pfile, NULL);
	set_fs(old_fs);
	return 0;
}

static unsigned char fts_ctpm_get_boot_vid(struct i2c_client *client)
{
    FTS_BYTE reg_val[2] = {0};
    unsigned char buf[FTS_SETTING_BUF_LEN];
	FTS_BYTE  auc_i2c_write_buf[10];
	int i=0;

    /*********Step 1:Reset  CTPM *****/
    printk("Step 1: Reset CTPM test\n");
    Fts_i2c_write(client,0xfc,0xaa);
    msleep(50);
    Fts_i2c_write(client,0xfc,0x55);
    msleep(40);

    /*********Step 2:Enter upgrade mode *****/
	printk("Step 2: Enter update mode. \n");
	auc_i2c_write_buf[0] = 0x55;
	auc_i2c_write_buf[1] = 0xaa;
	i2c_master_send(client, auc_i2c_write_buf, 2);

    /*********Step 3:check READ-ID***********************/
    //send the opration head
    do{
        if(i > 3)
            return 0; 
        cmd_write(client,0x90,0x00,0x00,0x00,4);//get the CTPM ID
        byte_read(client,reg_val,2);
        i++;
        printk("Step 3: CTPM ID,ID1 = 0x%x,ID2 = 0x%x\n",reg_val[0],reg_val[1]);
    }while(reg_val[0] != 0x79 || reg_val[1] != 0x03);

	// read BootLoader Version
    cmd_write(client,0xcd,0x0,0x00,0x00,1);
    byte_read(client,reg_val,1);
    printk("bootloader version = 0x%x\n", reg_val[0]);
	if ( reg_val[0] == 0xcd )
	{
		printk("The bootloader version is Old");
		buf[4] = 0x57;		// Invalid Vid,Just support Goworld's TP
	}
	else
	{   /* --------- read current project setting  ---------- */
	    //set read start address
	    buf[0] = 0x3;
	    buf[1] = 0x0;
	    buf[2] = 0x78;
	    buf[3] = 0x0;
	    byte_write(client,buf, 4);
	    byte_read(client,buf, FTS_SETTING_BUF_LEN);
	    
	    printk("[FTS] old setting: uc_i2c_addr = 0x%x, uc_io_voltage = %d, uc_panel_factory_id = 0x%x\n",
	        buf[0],  buf[2], buf[4]);
		if ( buf[4] == 0xFF )
			buf[4] = 0x57;		// Old Boot,Invalid Vid,Just support Goworld's TP
	}
	
    /********* reset the new FW***********************/
    cmd_write(client,0x07,0x00,0x00,0x00,1);

    msleep(200);

	return buf[4];
}


static unsigned char fts_ctpm_get_fw_vid(FTS_BYTE* pbt_buf, FTS_DWRD dw_lenth)
{
    unsigned char ui_sz;

    if (dw_lenth > 2)
    {
    	ui_sz = pbt_buf[dw_lenth - 2] + pbt_buf[dw_lenth - 1];
		if ( ui_sz == 0xFF )
			return 0x57;		// Old Version *.i file,Just support GoWorld's TP
		else
	        return pbt_buf[dw_lenth - 1];
    }
    else
    {
        //TBD, error handling?
        return 0xff; //default value
    }
}


static unsigned char fts_ctpm_get_fw_ver(FTS_BYTE* pbt_buf, FTS_DWRD dw_lenth)
{
    if (dw_lenth > 2)
    {
        return pbt_buf[dw_lenth - 2];
    }
    else
    {
        //TBD, error handling?
        return 0xff; //default value
    }
}

static int fts_ctpm_selfcal(struct i2c_client *client)
{
	//adjust begin
	u8 buf; 
	int ret = 0, retry = 0;

	msleep(2000);
	Fts_i2c_write(client, 0x00, 0x40);
	msleep(20);
	ret = Fts_i2c_read(client, 0x00, &buf, 1); 
	retry = 3;
	while(retry-- >0) 
	{	
		if(0x40 == buf)
			break;
		if(0x40 != buf)
		{	
			pr_info("%s: Enter test mode failed, retry = %d!\n", __func__, retry);
			Fts_i2c_write(client, 0x00, 0x40);
			msleep(20);
			ret = Fts_i2c_read(client, 0x00, &buf, 1); 
		}
	}	
	
	msleep(3000);
	Fts_i2c_write(client, 0x4D, 0x4);
	msleep(10000);
	Fts_i2c_read(client, 0x4D, &buf, 1); 
	retry=5;
	while(retry-- >0) 
	{	
		if(0x50 == buf)
			break;
		if(0x50 != buf)
		{	
			pr_info("%s value of register 0x4D is %d.\n", __func__, buf);
			Fts_i2c_read(client, 0x4D, &buf, 1); 
			if(buf == 0x30)
				pr_info("%s: Calibration failed! Retry!\n", __func__);
			msleep(2000);
		}	
	
	}	
	
	pr_info("%s value of register 0x4D is %d\n", __func__, buf);
	
	Fts_i2c_write(client, 0x00, 0x0);
	//adjust end
	return 0;
}		

static E_UPGRADE_ERR_TYPE  fts_ctpm_write_fw(struct i2c_client *client,FTS_BYTE* pbt_buf, FTS_DWRD dw_lenth)
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
    Fts_i2c_write(client,0xfc,0xaa);
    msleep(50);
    Fts_i2c_write(client,0xfc,0x55);
    printk("Step 1: Reset CTPM test\n");

    msleep(40);

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
    msleep(1500);
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
        msleep(FTS_PACKET_LENGTH/6 + 1);
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
        msleep(20);
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
        msleep(20);
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



int fts_ctpm_update_fw(struct i2c_client *client,FTS_BYTE* pbt_buf, FTS_DWRD dw_lenth)
{
	int ret;
	char crtfwVer;
	char crtfwVid;

	printk("********Fts Upgrade Start********\n");

	// 1. check vid
	ret = Fts_i2c_read(client, FT5X0X_REG_FT5201ID, &crtfwVid,1);
	if ( crtfwVid == FT5X0X_REG_FT5201ID )
	{
		printk("CTPM fw is corruptted!\n");
		crtfwVid = fts_ctpm_get_boot_vid(client);
		printk("vid from boot = 0x%x\n",crtfwVid);
		// Old Bootloader(<0x14) can't get the correct Vid.
		// So,must be Upgrade by manually if fw is corruptted.
		if ( crtfwVid < 0x42/* || crtfwVid == 0xFF*/ ){
			printk("Invalid  vid!\n");
			return -1;
		}
	}
	printk("Current fw vid=0x%x\n",crtfwVid);
	ret = fts_ctpm_get_fw_vid(pbt_buf,dw_lenth);
	printk("fw vid from fw file = 0x%x\n", ret);
	if ( ret != crtfwVid ){
		printk("vid dismatch !\n");
		return 0;
	}

	// 2. check fw ver
	ret = Fts_i2c_read(client, FT5X0X_REG_FIRMID, &crtfwVer,1);
	pr_info("Current fw verion = 0x%x.\n", crtfwVer);
	ret = fts_ctpm_get_fw_ver(pbt_buf,dw_lenth);
	printk("fw ver from fw file = 0x%x\n", ret);
//	if ( (ret <= crtfwVer) && (crtfwVer != FT5X0X_REG_FIRMID) )
//		return 0;

	// 3. update
	fts_ctpm_write_fw(client,pbt_buf, dw_lenth);
	//fts_ctpm_write_fw_with_i_file(ts->client);//Update the CTPM firmware if need

	// 4. self calibration
	ret = fts_ctpm_selfcal(client);

	//	ret = Fts_i2c_read(update_client, FT5X0X_REG_FIRMID, &fwVer,1) ;//ZTE_TS_XYM_20110830
	//	printk("%s: New Fts FW ID read ID = 0x%x,ret = 0x%x\n", __FUNCTION__, fwVer, ret);//ZTE_TS_XYM_20110830
		
	return 0;
}


static int fts_ctpm_write_fw_with_app_file(struct i2c_client *client,char * firmware_name)
{
  	FTS_BYTE*     pbt_buf = 0;//FTS_NULL;
   	int i_ret; //u8 fwver;
   	int fwsize;


	fwsize = ft5x0x_GetFwSize(firmware_name);
   	if(fwsize <= 0)
   	{
   		pr_err("%s ERROR:Get firmware size failed\n", __FUNCTION__);
		return -1;
   	}
	
    //   FW upgrade begin
  	 pbt_buf = (unsigned char *) kmalloc(fwsize+1,GFP_ATOMIC);
	if(ft5x0x_ReadFw(firmware_name, pbt_buf))
    {
       	pr_err("%s() - ERROR: request_firmware failed\n", __FUNCTION__);
        kfree(pbt_buf);
		return -1;
    }

   	i_ret =  fts_ctpm_update_fw(client, pbt_buf, fwsize);
   	if (i_ret != 0)	{
       	pr_err("%s() - ERROR:[FTS] upgrade failed i_ret = %d.\n",__FUNCTION__,  i_ret);
       //error handling ...
       //TBD
   	} else 	{
       	pr_info("[FTS] upgrade successfully.\n");
//		if(Fts_i2c_read(update_client, FT5X0X_REG_FIRMID, &fwver,1)>=0)
	//		pr_info("the new fw ver is 0x%02x\n", fwver);
//			fts_ctpm_selfcal();  //start auto CLB
   	}
	kfree(pbt_buf);
   	return i_ret;
}


static ssize_t ft5x0x_fwupgradeapp_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	/* place holder for future use */
    return -EPERM;
}

//upgrade from app.bin
static ssize_t ft5x0x_fwupgradeapp_store(struct device *dev,
					struct device_attribute *attr,
						const char *buf, size_t count)
{
	//struct Fts_ts_data *data = NULL;

	//struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	//data = (struct Fts_ts_data *) i2c_get_clientdata( client );
//	ssize_t num_read_chars = 0;
	char fwname[128];
	struct i2c_client *client = ftc_i2c_client;
	//struct i2c_client *client2 = container_of(dev, struct i2c_client, dev);
//	printk("xiayc: client=%p, client2=%p\n", client, client2);

	memset(fwname, 0, sizeof(fwname));
	sprintf(fwname, "%s", buf);
	fwname[count-1] = '\0';

	//mutex_lock(&data->device_mode_mutex);
	disable_irq(client->irq);

	printk("xiayc~~~2.client =%p\n",client);
	fts_ctpm_write_fw_with_app_file(client,fwname);
	
	enable_irq(client->irq);

//error_return:
	//mutex_unlock(&data->device_mode_mutex);

	return count;
}

/* sysfs */
//static DEVICE_ATTR(rawbase, S_IRUGO|S_IWUSR, ft5x0x_rawbase_show, ft5x0x_rawbase_store);
//static DEVICE_ATTR(ftstpfwver, S_IRUGO|S_IWUSR, ft5x0x_tpfwver_show, ft5x0x_tpfwver_store);
//upgrade from *.i
//static DEVICE_ATTR(ftsfwupdate, S_IRUGO|S_IWUSR, ft5x0x_fwupdate_show, ft5x0x_fwupdate_store);
//static DEVICE_ATTR(ftstprwreg, S_IRUGO|S_IWUSR, ft5x0x_tprwreg_show, ft5x0x_tprwreg_store);
//upgrade from app.bin 
static DEVICE_ATTR(ftsfwupgradeapp, S_IRUGO|S_IWUSR, ft5x0x_fwupgradeapp_show, ft5x0x_fwupgradeapp_store);
//static DEVICE_ATTR(ftsrawdatashow, S_IRUGO|S_IWUSR, ft5x0x_rawdata_show, ft5x0x_rawdata_store);

/*
static struct attribute *ft5x0x_attributes[] = {
	//&dev_attr_ftstpfwver.attr,
	//&dev_attr_ftsfwupdate.attr,
	//&dev_attr_ftstprwreg.attr,
	&dev_attr_ftsfwupgradeapp.attr,
	//ev_attr_ftsrawdatashow.attr,
	NULL
};

static struct attribute_group ft5x0x_attribute_group = {
	.attrs = ft5x0x_attributes
};
*/

extern struct kobject *firmware_kobj;

int Ft5x0x_fwupdate(struct i2c_client *client)
{
	return fts_ctpm_update_fw(client,CTPM_FW,sizeof(CTPM_FW));
}

int Ft5x0x_fwupdate_init(struct i2c_client *client)
{
	int ret;
	struct kobject * fts_fw_kobj=NULL;

	fts_fw_kobj = kobject_get(firmware_kobj);
	if (fts_fw_kobj == NULL) {
		fts_fw_kobj = kobject_create_and_add("firmware", NULL);
		if (fts_fw_kobj == NULL) {
			pr_err("%s: subsystem_register failed\n", __func__);
			ret = -ENOMEM;
			return ret;
		}
	}
 
	ret=sysfs_create_file(fts_fw_kobj, &dev_attr_ftsfwupgradeapp.attr);
	if (ret) {
		pr_err("%s: sysfs_create_file failed\n", __func__);
		return ret;
	}
	ftc_i2c_client = client;
	printk("%s, xiayc: client=%p ftc_i2c_client=%p\n",__func__,client,ftc_i2c_client);

	pr_info("%s:fts firmware update init succeed!\n", __func__);
	return 0;


}

int Ft5x0x_fwupdate_deinit(struct i2c_client *client)
{
	struct kobject * fts_fw_kobj=NULL;

	fts_fw_kobj = kobject_get(firmware_kobj);
	if ( !firmware_kobj ){
		printk("%s: error get kobject\n", __func__);
		return -1;
	}
	
	sysfs_remove_file(firmware_kobj, &dev_attr_ftsfwupgradeapp.attr);
	//	kobject_del(virtual_key_kobj);

	return 0;
}

//#endif

