#include <linux/input/SynaUpgrade.h>
#include <linux/input/synaptics_i2c_rmi.h>
#include <linux/i2c.h>
#include <mach/gpio.h>
#include <linux/delay.h>

#include <linux/file.h>


unsigned char Firmware_Image[16000];  // make smaller and dynamic
unsigned char Config_Image[16000];  // make smaller and dynamic

unsigned char page_data[200];
unsigned char status;
unsigned long firmware_imgsize;
unsigned char firmware_imgver;
unsigned long config_imgsize;
unsigned long filesize;
unsigned int synaptics_bootload_id;

unsigned char *firmware_imgdata;
unsigned char *config_imgdata;
unsigned short firmware_blocksize;
unsigned short firmware_blockcount;
unsigned short config_blocksize;
unsigned short config_blockcount;

unsigned long firmware_imgchecksum;

static const unsigned char f34_reflash_cmd_firmware_crc   = 0x01;
static const unsigned char f34_reflash_cmd_firmware_write = 0x02;
static const unsigned char f34_reflash_cmd_erase_all      = 0x03;
static const unsigned char f34_reflash_cmd_config_read    = 0x05;
static const unsigned char f34_reflash_cmd_config_write   = 0x06;
static const unsigned char f34_reflash_cmd_config_erase   = 0x07;
static const unsigned char f34_reflash_cmd_enable        = 0x0f;
static const unsigned char f34_reflash_cmd_normal_result  = 0x80; 

unsigned short f01_RMI_CommandBase;
unsigned short f01_RMI_DataBase;
unsigned short f01_RMI_QueryBase;
unsigned short f01_RMI_IntStatus;


unsigned int synaptics_bootload_imgid;
unsigned short f34_reflash_datareg;
unsigned short f34_reflash_blocknum;
unsigned short f34_reflash_blockdata;
unsigned short f34_reflash_query_bootid;

unsigned short f34_reflash_query_flashpropertyquery;//!!!!Important!!!!determine how to calculate F34 register map layout	
unsigned short f34_reflash_query_firmwareblocksize;
unsigned short f34_reflash_query_firmwareblockcount;
unsigned short f34_reflash_query_configblocksize;
unsigned short f34_reflash_query_configblockcount;

unsigned short f34_reflash_flashcontrol;
unsigned short f34_reflash_blocknum;
unsigned short f34_reflash_blockdata;

bool  flash_prog_on_startup;
bool  unconfigured;

//following functions defined in kernel/drivers/input/touchscreen/synaptics_i2c_rmi.c
//extern int RMI4ReadBootloadID(struct i2c_client *client);
//extern int synaptics_get_pgt_f34(struct i2c_client *client);

struct rmi4_function_descriptor f34_func_des;
struct rmi4_function_descriptor f01_func_des;

bool synaptics_read_flash_property(struct i2c_client *client)
{
	int ret = 0;
	ret = i2c_smbus_read_i2c_block_data(client, f34_reflash_query_flashpropertyquery, 1, &page_data[0]);
	if(0 > ret){
		pr_err("%s: i2c read flash property error!\n", __func__);
		return -1;
	}

	pr_info("Flash Property Query = 0x%x\n", page_data[0]);
	return ((page_data[0] & 0x01) == 0x01);
}

//Func34 has 2 models, we should make sure which model the TP used
void synaptics_set_flash_addr(struct i2c_client *client)
{
	if (synaptics_read_flash_property(client)){
		pr_info("Flash Property  1\n");
		f34_reflash_flashcontrol = f34_func_des.data_base + firmware_blocksize + 2;
		f34_reflash_blocknum = f34_func_des.data_base;
		f34_reflash_blockdata = f34_func_des.data_base + 2;
	}else{
		f34_reflash_flashcontrol = f34_func_des.data_base;
		f34_reflash_blocknum = f34_func_des.data_base + 1;
		f34_reflash_blockdata = f34_func_des.data_base + 3;
	}
}

int synaptics_get_pgt_f34(struct i2c_client *client)
{
#if 0
	int ret = 0;
	ret = i2c_smbus_read_i2c_block_data(client, PDT_P00_F34_FLASH_QUERY_BASE_ADDR, sizeof(struct rmi4_function_descriptor),(uint8_t *)&f34_func_des);
	if(0 > ret){
		pr_err("%s: i2c read F34 pgt error!\n", __func__);
		return -1;
	}else{
		//这个得到的值和寄存器表上的不一致差了一位
		pr_info("%s: F34 data: query base address:%d data base address:%d .\n", __func__, f34_func_des.query_base,f34_func_des.data_base);
	}
	
	ret = i2c_smbus_read_i2c_block_data(client, PDT_P00_F01_FLASH_QUERY_BASE_ADDR, sizeof(struct rmi4_function_descriptor),(uint8_t *)&f01_func_des);
	if(0 > ret){
		pr_err("%s: i2c read F01 pgt error!\n", __func__);
		return -1;
	}else{
		//同上
		pr_info("%s: F01 data: query base address:%d data base:%d .\n", __func__, f01_func_des.query_base,f01_func_des.data_base);
	}
#else
int ret=0;
struct rmi4_function_descriptor Buffer;
unsigned short uAddress;

f01_func_des.function_number= 0;
f34_func_des.function_number = 0;
//m_BaseAddresses.m_ID = 0xff;

for(uAddress = 0xe9; uAddress > 10; uAddress -= sizeof(struct rmi4_function_descriptor))
{
  ret = i2c_smbus_read_i2c_block_data(client, uAddress, sizeof(Buffer),(uint8_t *)&Buffer);
  if(0 > ret){
	  pr_err("%s: i2c read F34 pgt error!\n", __func__);
	  return -1;
  	}else
  	{
  		pr_info("%s: data: query base address:%d data base address:%d .\n", __func__, Buffer.query_base,Buffer.data_base);	
  	}

  switch(Buffer.function_number)
  {
	case 0x34:
	  f34_func_des= Buffer;
	  break;
	case 0x01:
	  f01_func_des = Buffer;
	  break;
  }

  if(Buffer.function_number == 0)
  {
	break;
  }
  else
  {
	printk("Function $%02x found.\n", Buffer.function_number);
  }
}

#endif
    //实际上f01的赋值没有看到有什么用处
    f01_RMI_DataBase= f01_func_des.data_base;
    f01_RMI_IntStatus = f01_func_des.data_base + 1;
    f01_RMI_CommandBase = f01_func_des.cmd_base;
    f01_RMI_QueryBase = f01_func_des.query_base;	
  
	f34_reflash_datareg = f34_func_des.data_base;
	f34_reflash_blocknum = f34_func_des.data_base;
	f34_reflash_blockdata = f34_func_des.data_base + 2;
	
	f34_reflash_query_bootid = f34_func_des.query_base;
	f34_reflash_query_flashpropertyquery = f34_func_des.query_base + 2;	
	f34_reflash_query_firmwareblocksize = f34_func_des.query_base + 3;
	f34_reflash_query_firmwareblockcount = f34_func_des.query_base + 5;
	f34_reflash_query_configblocksize = f34_func_des.query_base + 3;
	f34_reflash_query_configblockcount = f34_func_des.query_base + 7;
	
  	synaptics_set_flash_addr(client);
	return 0;
}

int synaptics_read_bootload_id(struct i2c_client *client)
{
	int ret = 0;
	char data[2];

	ret = i2c_smbus_read_i2c_block_data(client, f34_reflash_query_bootid, 2, data);
	if(0 > ret){
		pr_err("%s: i2c read bootload id error!\n", __func__);
		return -1;
	}
	
	synaptics_bootload_id = (unsigned int)data[0] + (unsigned int)data[1]*0x100;
	printk("%s bootload id: %d,DATA[0]:0X%x,DATA[1]:0X%x", __func__, synaptics_bootload_id,data[0],data[1]);

	return 0;
}

int synaptics_write_bootload_id(struct i2c_client *client)
{
	int ret = 0;
	unsigned char data[2];
  
	data[0] = synaptics_bootload_id%0x100;
	data[1] = synaptics_bootload_id/0x100;
	printk(" write synaptics_bootload_id = %d,data[0]=0x%x,data[1]=0x%x \n",synaptics_bootload_id,data[0],data[1]);
	ret = i2c_smbus_write_i2c_block_data(client, f34_reflash_blockdata, 2,data);
  	if(0 != ret){
		pr_err("%s: i2c write bootload id error!\n", __func__);
		return -1;
	}
	return 0;
}





//Check wether the TP support F34 and try to enter bootload mode
void synaptics_write_page(struct i2c_client *client)
{
	int ret = 0;
	unsigned char upage = 0x00;
	unsigned char f01_rmi_data[2];
	unsigned char status;

	//wirte 0x00 to Page Select register
	//ret = i2c_smbus_write_byte_data(client, 0xff, upage);
	ret = i2c_smbus_write_i2c_block_data(client, 0xff, 1, &upage);
	if(0 > ret){
		pr_err("%s: i2c read flash property error!\n", __func__);
		return;
	}
	
	do{
		ret = i2c_smbus_read_i2c_block_data(client, 0x00, 1, &status);
		//ret = i2c_smbus_read_i2c_block_data(client, f01_func_des.data_base, 1, &status);
		if(0 > ret){
			pr_err("%s: i2c read f34 flash data0 error!\n", __func__);
			return ;
		}

		if(status & 0x40){
			flash_prog_on_startup = true;
		}

		if(status & 0x80){
			unconfigured = true;
			break;
		}

		pr_info("%s: Status is 0x%x\n", __func__, status);
	} while(status & 0x40);

	if(flash_prog_on_startup && ! unconfigured){
		pr_info("%s: Bootloader running!\n", __func__);
	}
	else if(unconfigured){
		pr_info("%s: UI running\n", __func__);
	}

	synaptics_get_pgt_f34(client);

	if(f34_func_des.function_number == 0){
		pr_err("%s: Func 34 is not supported\n", __func__);
	}

	pr_info("Func 34 addresses Control base:$%02x Query base: $%02x.\n", f34_func_des.ctrl_base, f34_func_des.query_base);

	if(f01_func_des.function_number == 0){
		pr_err("%s: Func 01 is not supported, its members will be set here!\n", __func__);
		f01_func_des.function_number = 0x01;
		f01_func_des.data_base = 0;
	}
	pr_info("Func 01 addresses Control base:$%02x Query base: $%02x.\n", f01_func_des.ctrl_base, f01_func_des.query_base);

	// Get device status
	ret = i2c_smbus_read_i2c_block_data(client, f01_func_des.data_base, sizeof(f01_rmi_data), &f01_rmi_data[0]);
	if(0 > ret){
		pr_err("%s: i2c read f01_func_des.data_base error!\n", __func__);
		return ;
	}

	// Check Device Status
	pr_info("%s: Configured: %s\n", __func__, f01_rmi_data[0] & 0x80 ? "false" : "true");
	pr_info("%s: FlashProg:  %s\n", __func__, f01_rmi_data[0] & 0x40 ? "true" : "false");
	pr_info("%s: StatusCode: 0x%x \n", __func__, f01_rmi_data[0] & 0x0f );
}

int synaptics_enable_flash_command(struct i2c_client *client)
{
  return i2c_smbus_write_i2c_block_data(client, f34_reflash_flashcontrol, 1, (unsigned char *)&f34_reflash_cmd_enable);
}

void synaptics_enable_flash(struct i2c_client *client)
{
	unsigned char data[2]={0,0};
	int ret = 0;
	int count = 0;

	// Read bootload ID
	ret = synaptics_read_bootload_id(client);
	if(0 != ret){
		pr_err("%s: Read bootload err!\n", __func__);
		return ;
	}
	
	// Write bootID to block data registers
	ret = synaptics_write_bootload_id(client);
	if(0 != ret){
		pr_err("%s: Write bootload err!\n", __func__);
		return ;
	}
	
	do {
		ret = i2c_smbus_read_i2c_block_data(client, f34_reflash_flashcontrol, 1, (uint8_t *)&page_data[0]);

		// To deal with ASIC physic address error from cdciapi lib when device is busy and not available for read
		if(0>ret && count < 300)
		{
		  count++;
		  page_data[0] = 0;
		  continue;
		}

		// Clear the attention assertion by reading the interrupt status register
		ret = i2c_smbus_read_i2c_block_data(client, f01_func_des.data_base, 1, &status);
		if(0 > ret){
			pr_err("%s: Read Func34 flashcontrol err!\n", __func__);
			return ;
		}

	} while(((page_data[0] & 0x0f) != 0x00) && (count <= 300));

	// Issue Enable flash command
	ret = synaptics_enable_flash_command(client);
	if(0 != ret){
		pr_err("%s: Enable flash failed!\n", __func__);
		return ;
	}

	//!!Here missing this function because function in dll is not open source!!!
	//!!Just use sleep instead!!!
	//RMI4WaitATTN();
	msleep(2000);
	
	synaptics_get_pgt_f34(client);
	
	do
	{
	ret = i2c_smbus_read_i2c_block_data(client, f34_reflash_flashcontrol, 1, &data[0]);
	printk("%s:Func34 data register bit 7: 0x%x FOR ENSURE PRO_ENABLE\n", __func__, data[0]);
	msleep(10);
	} while((data[0]&0x80)==0);
	
	#if 0
	do
	{
		ret = i2c_smbus_write_i2c_block_data(client, f01_func_des.data_base+1, 1, &data[0]);
		printk("%s:Func01 INTER register bit 0: 0x%x FOR FLASH INT BE 1\n", __func__, data[0]);
	} while((data[0] & 0x01)==0);	

	// Read F01 Status flash prog, ensure the 6th bit is '0'
		ret = i2c_smbus_write_i2c_block_data(client, f34_reflash_flashcontrol, 1, &data[0]);
		printk("%s:Func34 data register bit 7: 0x%x FOR ENSURE PRO_ENABLE\n", __func__, data[0]);
		
	// Read F01 Status flash prog, ensure the 6th bit is '0'
	do
	{
		ret = i2c_smbus_write_i2c_block_data(client, f01_func_des.data_base, 1, &data[0]);
		printk("%s:Func01 data register bit 6: 0x%x\n", __func__, data[0]);
	} while((data[0] & 0x40)!= 0);
#endif

}


unsigned long synaptics_extract_long_from_header(const unsigned char* SynaImage)  // Endian agnostic
{
  return((unsigned long)SynaImage[0] +
         (unsigned long)SynaImage[1]*0x100 +
         (unsigned long)SynaImage[2]*0x10000 +
         (unsigned long)SynaImage[3]*0x1000000);
}

void synaptics_read_config_info(struct i2c_client *client)
{
  unsigned char data[2];
  int ret;
  //ret = SynaReadRegister(m_uF34ReflashQuery_ConfigBlockSize, &uData[0], 2);
  ret = i2c_smbus_read_i2c_block_data(client,f34_reflash_query_configblocksize,2,data);
  if(0 > ret){
	pr_err("%s: Read config block size err!\n", __func__);
	return;
  }


  config_blocksize = data[0] | (data[1] << 8);

  //m_ret = SynaReadRegister(m_uF34ReflashQuery_ConfigBlockCount, &uData[0], 2);
  ret = i2c_smbus_read_i2c_block_data(client,f34_reflash_query_configblockcount,2,data);
	  if(0 > ret){
		pr_err("%s: Read config block count err!\n", __func__);
		return;
	  }


  config_blockcount = data[0] | (data[1] << 8);
  config_imgsize = config_blocksize*config_blockcount;
}

void synaptics_read_firmware_info(struct i2c_client *client)
{
	unsigned char data[2];
	int ret;

	ret = i2c_smbus_read_i2c_block_data(client, f34_reflash_query_firmwareblocksize, 2, data);
	if(0 > ret){
		pr_err("%s: Read firmware block size err!\n", __func__);
		return;
	}

	firmware_blocksize = data[0] | (data[1] << 8);
	
	ret = i2c_smbus_read_i2c_block_data(client, f34_reflash_query_firmwareblockcount, 2, data);
	if(0 > ret){
		pr_err("%s: Read firmware block count err!\n", __func__);
		return ;
	}

	firmware_blockcount = data[0] | (data[1] << 8);
	firmware_imgsize = firmware_blockcount * firmware_blocksize;

	pr_info("%s: Firmware block size:%d ,firmware block count: %d\n", __func__, firmware_blocksize, firmware_blockcount);
}


void synaptics_cal_checksum(unsigned short * data, unsigned short len, unsigned long * dataBlock)
{
  unsigned long temp = *data++;
  unsigned long sum1;
  unsigned long sum2;

  *dataBlock = 0xffffffff;

  sum1 = *dataBlock & 0xFFFF;
  sum2 = *dataBlock >> 16;

  while (len--)
  {
    sum1 += temp;
    sum2 += sum1;
    sum1 = (sum1 & 0xffff) + (sum1 >> 16);
    sum2 = (sum2 & 0xffff) + (sum2 >> 16);
  }

  *dataBlock = sum2 << 16 | sum1;
}

unsigned short synaptics_get_frimware_size(void)
{
  return firmware_blocksize * firmware_blockcount;
}

unsigned short synaptics_get_config_size(void)
{
  return config_blocksize * config_blockcount;
}

//Go to a endless circule if config or firmware sizes dismatch here.
void synaptics_read_firmware_header(struct i2c_client *client)
{
	unsigned long check_sum;
	//unsigned char data;
	int ret;
	
	filesize = sizeof(SynaFirmware) -1;

	pr_info("\n%s:Scanning SynaFirmware[], header file - len = %ld \n\n", __func__, filesize);

	check_sum = synaptics_extract_long_from_header(&(SynaFirmware[0]));
	synaptics_bootload_imgid = (unsigned int)SynaFirmware[4] + (unsigned int)SynaFirmware[5]*0x100;
	firmware_imgver = SynaFirmware[7];
	firmware_imgsize = synaptics_extract_long_from_header(&(SynaFirmware[8]));
	config_imgsize = synaptics_extract_long_from_header(&(SynaFirmware[12]));
	
	pr_info("%s: Target = %s, ", __func__, &SynaFirmware[16]);
	pr_info("Cksum = 0x%ld, Id = 0x%d, Ver = %d, FwSize = 0x%ld, ConfigSize = 0x%ld \n",
	check_sum, synaptics_bootload_imgid, firmware_imgver, firmware_imgsize, config_imgsize);

	 // Determine firmware organization - read firmware block size and firmware size
	synaptics_read_firmware_info(client);  

	synaptics_cal_checksum((unsigned short*)&(SynaFirmware[4]), (unsigned short)(filesize-4)>>1,
						&firmware_imgchecksum);

	if (filesize != (0x100+firmware_imgsize+config_imgsize))
	{
		pr_err("%s: Error--SynaFirmware[] size = %ld, expected %ld\n", __func__, filesize, (0x100+firmware_imgsize+config_imgsize));
		while(1);
	}

	if (firmware_imgsize != synaptics_get_frimware_size())
	{
		pr_err("%s: Firmware image size verfication failed!\n", __func__);
		pr_err("\tsize in image %ld did not match device size %d\n", firmware_imgsize, synaptics_get_frimware_size());
		while(1);
	}

	if (config_imgsize != synaptics_get_config_size())
	{
		pr_err("%s: Configuration size verfication failed!\n", __func__);
		pr_err("\tsize in image %ld did not match device size %d\n", config_imgsize, synaptics_get_config_size());
		while(1);
	}

	firmware_imgdata=(unsigned char *)((&SynaFirmware[0])+0x100);

	// memcpy(m_firmwareImgData, (&SynaFirmware[0])+0x100, firmware_imgsize);
	memcpy(config_imgdata,   (&SynaFirmware[0])+0x100+firmware_imgsize, config_imgsize);

	ret = i2c_smbus_read_i2c_block_data(client, f34_reflash_flashcontrol, 1, &page_data[0]);
	if(0 > ret){
		pr_err("%s: Read Func34 flash control err!\n", __func__);
		return ;
	}
	return;
}

bool synaptics_validate_bootload_id(unsigned short bootloadID, struct i2c_client *client)
{
	int ret = 0;
	pr_info("%s: called!\n", __func__);
	ret = synaptics_read_bootload_id(client);
	if(0 != ret){
		pr_err("%s: read bootload id err!\n", __func__);
		return false;
	}
	
	pr_info("Bootload ID of device: %d, input bootID: %d\n", synaptics_bootload_id, bootloadID);

	// check bootload ID against the value found in firmware--but only for image file format version 0
	return firmware_imgver != 0 || bootloadID == synaptics_bootload_id;
}

int synaptics_issue_erase_cmd(unsigned char *command, struct i2c_client *client)
{
  int ret;
  // command = 3 - erase all; command = 7 - erase config
  ret = i2c_smbus_write_i2c_block_data(client, f34_reflash_flashcontrol, 1, command);

  return ret;
}

int synaptics_flash_firmware_write(struct i2c_client *client)
{
	unsigned char *buf_firmware_data = firmware_imgdata;
	unsigned char data[2];
	int ret = 0;
	unsigned short blocknum;
	pr_info("Synaptics Flash Firmware startshere!\n");

	for(blocknum = 0; blocknum < firmware_blockcount; ++blocknum){
		data[0] = blocknum & 0xff;
		data[1] = (blocknum & 0xff00) >> 8;

		// Write Block Number
		ret = i2c_smbus_write_i2c_block_data(client, f34_reflash_blocknum, 2, data);
		if(0 > ret){
			pr_err("%s: Write f34 reflash blocknum id err, count:%d!\n", __func__, blocknum);
			return -1;
		}

		// Write Data Block
		ret = i2c_smbus_write_i2c_block_data(client, f34_reflash_blockdata, firmware_blocksize, buf_firmware_data);
		if(0 > ret){
			pr_err("%s: Write f34 reflash block data err, count:%d!\n", __func__, blocknum);
			return -1;
		}

		// Move to next data block
		buf_firmware_data += firmware_blocksize;

		// Issue Write Firmware Block command
		//m_bAttenAsserted = false;
		data[0] = 2;
		ret = i2c_smbus_write_i2c_block_data(client, f34_reflash_flashcontrol, 1, data);
		if(0 > ret){
			pr_err("%s: Write f34 reflash flash control err, count:%d!\n", __func__, blocknum);
			return -1;
		}
		// Wait ATTN. Read Flash Command register and check error
		//RMI4WaitATTN();
		//msleep(200);
		do
		{
		
		msleep(10);
		ret = i2c_smbus_read_i2c_block_data(client, f34_reflash_flashcontrol, 1, &data[0]);
		printk("%s:Func34 data register bit 7: 0x%x FOR ENSURE PRO_ENABLE\n", __func__, data[0]);
		} while((data[0]&0x80)==0);

	}
	pr_info("%s:Flash Firmware done!\n", __func__);

	return 0;
}

void synaptics_prog_firmware(struct i2c_client *client)
{
	int ret;
	unsigned char data[1];

	if ( !synaptics_validate_bootload_id(synaptics_bootload_imgid, client) )
	{
	pr_err("%s: Validate bootload id failed!\n", __func__);
	}

	// Write bootID to data block register
	ret = synaptics_write_bootload_id(client);
	if(0 != ret){
		pr_err("%s: Write bootload id err!\n", __func__);
		return ;
	}
	// Issue the firmware and configuration erase command
	data[0]=3;
	ret = synaptics_issue_erase_cmd(&data[0], client);
	if(0 != ret){
		pr_err("%s: Issue erase cmd err!\n", __func__);
		return ;
	}
	
	//RMI4WaitATTN();
	msleep(200);
	do
	{
	ret = i2c_smbus_read_i2c_block_data(client, f34_reflash_flashcontrol, 1, &data[0]);
	printk("%s:Func34 data register bit 7: 0x%x FOR ENSURE PRO_ENABLE\n", __func__, data[0]);
	msleep(10);
	} while((data[0]&0x80)==0);

	// Write firmware image
	ret = synaptics_flash_firmware_write(client);
	if(0 != ret){
		pr_err("%s: Flash firmware write err!\n", __func__);
		return ;
	}
}

int synaptics_issue_flash_ctl_cmd(struct i2c_client *client, unsigned char *command)
{
	int ret;
	ret = i2c_smbus_write_byte_data(client, f34_reflash_flashcontrol, *command);

	return ret;
}

void synaptics_prog_configuration(struct i2c_client *client)
{
	int ret = 0;
	unsigned char data[2];
	unsigned char *pdata = config_imgdata;
	unsigned short blocknum;
	for(blocknum = 0; blocknum < config_blockcount; blocknum++){
		data[0] =  blocknum%0x100;
		data[1] =  blocknum/0x100;

		// Write Configuration Block Number
		ret = i2c_smbus_write_i2c_block_data(client, f34_reflash_blocknum, 2, &data[0]);
		if(0 > ret){
			pr_err("%s: Write f34 reflash blocknum err, count:%d.\n", __func__, blocknum);
			return ;
		}

		// Write Data Block
		ret = i2c_smbus_write_i2c_block_data(client, f34_reflash_blockdata, config_blocksize, pdata);
		if(0 > ret){
			pr_err("%s: Write Func34 reflash blockdata, count:%d.\n", __func__, blocknum);
			return ;
		}

		pdata += config_blocksize;

		// Issue Write Configuration Block command to flash command register
		//m_bAttenAsserted = false;
		data[0] = f34_reflash_cmd_config_write;

		ret = synaptics_issue_flash_ctl_cmd(client, &data[0]);
		if(0 != ret){
			pr_err("%s: Flash firmware write err!\n", __func__);
			return ;
		}

		// Wait for ATTN
		//RMI4WaitATTN();
		//msleep(200);
		do
		{
		msleep(10);
		ret = i2c_smbus_read_i2c_block_data(client, f34_reflash_flashcontrol, 1, &data[0]);
		if(0>ret)printk("synaptics_prog_configuration read i2c failed\n");
		printk("%s:Func34 data register bit 7: 0x%x FOR ENSURE PRO_ENABLE\n", __func__, data[0]);

		} while((data[0]&0x80)==0);

	}
}

void synaptics_reset_device(struct i2c_client *client)
{
	int ret = 0;
	unsigned char data[1];

	data[0] = 1;
	ret = i2c_smbus_write_i2c_block_data(client, f01_func_des.cmd_base, 1, &data[0]);
	if(0 > ret){
		pr_err("%s: Write reset command to Func01 command base error.\n", __func__);
		return ;
	}
}

int synaptics_disenable_flash(struct i2c_client *client)
{
	int ret = 0;
	unsigned char data[2];
	unsigned int errcount = 0;

	// Issue a reset command
	synaptics_reset_device(client);
	msleep(200);


	// Wait for ATTN to be asserted to see if device is in idle state
#if 0
	if (!SynaWaitForATTN(300))
	{
	RMI4CheckIfFatalError(EErrorTimeout);
	}
#endif
	msleep(300);

	do {
		ret = i2c_smbus_read_i2c_block_data(client, f34_reflash_flashcontrol, 1, &page_data[0]);

		// To work around the physical address error from control bridge
		if(ret && errcount < 300)
		{
		  errcount++;
		  page_data[0] = 0;
		  continue;
		}
		pr_info("%s: RMI4WaitATTN after errorCount loop, error count=%d\n", __func__, errcount);
	} while(((page_data[0] & 0x0f) != 0x00) && (errcount <= 300));

	//if errcount > 300, quit here
	if(0 > ret){
		pr_err("%s: Write Func34 reflash flash control error.\n", __func__);
		return -1;
	}

	// Clear the attention assertion by reading the interrupt status register
	ret = i2c_smbus_read_i2c_block_data(client, f01_func_des.data_base+1, 1, &status);
	if(0 > ret){
		pr_err("%s: Write Func34 reflash flash control error.\n", __func__);
		return -1;
	}

	// Read F01 Status flash prog, ensure the 6th bit is '0'
	do
	{
		ret = i2c_smbus_read_i2c_block_data(client, f01_func_des.data_base, 1, &data[0]);
		printk("%s:Func01 data register bit 6: 0x%x\n", __func__, data[0]);
	} while((data[0] & 0x40)!= 0);

	// With a new flash image the page description table could change
	synaptics_get_pgt_f34(client);
	

	return 0;//ESuccess;
}

//函数功能为触摸屏通断电
void syna_power_on_off(struct i2c_client *client,int on_off)
{
	//
	//int(* power)(int on);
	//power=client->dev.platform_data;
	//power(on_off);
	gpio_direction_output(31, on_off);
}
unsigned short i2c_address;
void syna_i2c_init(struct i2c_client *client)
{
	//i2c_address=client->addr;//temp
	syna_power_on_off(client,0);
	msleep(200);
	syna_power_on_off(client,1);
	msleep(200);
	//RMI4WritePage();
	synaptics_write_page(client);
}
void syna_rmi4_init(struct i2c_client *client)
{
/*
	// Set up blockSize and blockCount for UI and config
	RMI4ReadConfigInfo();
	RMI4ReadFirmwareInfo();
	
	// Allocate arrays
	m_firmwareImgData = &FirmwareImage[0];	// new unsigned char [GetFirmwareSize()];
	m_configImgData = &ConfigImage[0];		// new unsigned char [GetConfigSize()];
*/
	synaptics_read_config_info(client);
	synaptics_read_firmware_info(client);
	firmware_imgdata=&Firmware_Image[0];
	config_imgdata = &Config_Image[0];
}
#if 0
void Synaptics_Convert_FirmwareImage_To_CHeaderFile(void)
{
	struct file * pFile;
	int c;
	long int count;
	pFile=open("/etc/image",O_RDWR);
	if(NULL==pFile)
		printk(" %s :open image file failed\n",__func__);

	do{
      c = fgetc (pFile);

      if (c != EOF)
        //fprintf(hFile, "0x%02x, ",c);
        SynaFirmware[count]=c;
      else 
	  	//fprintf(hFile, " 0xFF }; \n\n");
	  	SynaFirmware[count]=0xff;

	  count++;

      if (c != EOF)
        printf("0x%02x, ",c);
      else printf(" 0xFFFF }; \n\n");


    } while (c != EOF);

}
#endif

void ioctol(struct i2c_client *client)
{
	//原函数中的初始化，暂时看来没有什么用处
	//RMI4FuncsConstructor();
	
	//初始化i2c等
	//RMI4RMIInit(protocolType, (unsigned char)i2c_address, attn, byte_delay, bit_rate, time_out);
	//RMI4Init();
	syna_i2c_init(client);
	syna_rmi4_init(client);

	synaptics_set_flash_addr(client);
	//前面准备工作完毕，开始programming流程
	synaptics_enable_flash(client);

	//这个函数功能暂时不要所需的数组已经用其他方法取得
	//SynaConvertFirmwareImageToCHeaderFile(ImgFile);  // this is just a utility to generate a header file.

	synaptics_read_firmware_header(client);
	synaptics_prog_firmware(client);
	synaptics_prog_configuration(client);

	synaptics_disenable_flash(client);
	
	//这里建议重启触摸屏
	//EndControlBridge();
	syna_power_on_off(client,0);
	msleep(500);
	syna_power_on_off(client,1);
}


