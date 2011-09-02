/*
change record
20100926 	yintianci created
20100927	yintianci add code about treating bqfs and dffs file separately
20100930	yintianci enable it enable yintianci_bqfs_20100930_1
20100930	yintianci change to burn bqfs anytime yintianci_bqfs_20100930_2
20101014       yintianci fixed 3% problem;YINTIANCI_GAUGE_20101014
20101016       yintianci change to make sure init can get update_success flag;YINTIANCI_GAUGE_20101016
20101021       yintianci add add judging code about ti_gauge_td-NULL -pointer
20101025       yintianci sometimes the voltage from gauge is 175mV,that means the gauge is in exception, do something to fix it.
20101214	yintianci strstr() sometims cust much more time than our anticipation,so replace it .YINTIANCI_GAUGE_20101214
20110311    yintianci make sure IT be enabled after FW download YINTIANCI_GAUGE_IT_ENABLE_20110311
*/


#define ZTE_TI_GAUGE
#ifdef ZTE_TI_GAUGE
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/proc_fs.h>
#include <linux/platform_device.h>
#include <mach/gpio.h>
#include <linux/mutex.h>


#define GAUGE_UPDATE_FIRMWARE
#ifdef GAUGE_UPDATE_FIRMWARE
//#define GAUGE_UPDATE_TEST_PARSE //打开这个宏就只进行文件解析测试，不进行执行
#include <linux/kernel.h>
#include <asm/uaccess.h>

#include <linux/blkdev.h>
#include <linux/completion.h>
#include <linux/dcache.h>
#include <linux/device.h>
#include <linux/fcntl.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/kref.h>
#include <linux/kthread.h>
#include <linux/limits.h>
#include <linux/rwsem.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/string.h>
#include <linux/freezer.h>
#include <linux/utsname.h>
#include <linux/wakelock.h>
#endif

//定义标准功能寄存器地址宏
#define REGADDR_CNTL 		0x00//control
#define REGADDR_TEMP 		0x06//temperature
#define REGADDR_VOLT 		0x08//voltage
#define REGADDR_FLAGS 		0x0a//
#define REGADDR_NAC 		0x0c//norminal available capacity
#define REGADDR_FAC 		0x0e//full available capacity
#define REGADDR_RM 		0x10//remaining capacity
#define REGADDR_FCC 		0x12//full charge capacity
#define REGADDR_AI 			0x14//average current
#define REGADDR_TTE 		0x16//time to empty
#define REGADDR_MLI 		0x1e//max load current
#define REGADDR_MLTTE 		0x20//max load time to empty
#define REGADDR_CC 			0x2a//cycle count
#define REGADDR_SOC 		0x2c//state of charge

//定义扩展功能寄存器地址宏
#define REGADDR_DCAP 			0x3c//control
#define REGADDR_DFCLS 			0x3e//temperature
#define REGADDR_DFBLK 			0x3f//voltage
#define REGADDR_DFD 			0x40//
#define REGADDR_DFDCKS 		0x60//norminal available capacity
#define REGADDR_DFDCNTL 		0x61//full available capacity
#define REGADDR_DNAMELEN 		0x62//remaining capacity
#define REGADDR_DNAME 			0x0x63//full charge capacity
#define REGADDR_APPSTAT 		0x6a//average current


//定义control subcommands
#define CNTLSUB_CONTROL_STATUS 			0x0000
#define CNTLSUB_DEVICE_TYPE 				0x0001
#define CNTLSUB_FW_VERSION 				0x0002
#define CNTLSUB_HW_VERSION 				0x0003
#define CNTLSUB_DF_CHECKSUM  				0x0004
#define CNTLSUB_RESET_DATA 				0x0005
#define CNTLSUB_PREV_MACWRITE 			0x0007
#define CNTLSUB_CHEM_ID 					0x0008
#define CNTLSUB_BOARD_OFFSET 				0x0009
#define CNTLSUB_SET_HIBERNATE				0x0011
#define CNTLSUB_CLEAR_HIBERNATE			0x0012
#define CNTLSUB_SET_SLEEPp 				0x0013
#define CNTLSUB_CLEAR_SLEEPp 				0x0014
#define CNTLSUB_SEALED 						0x0020
#define CNTLSUB_IT_ENABLE 					0x0021
#define CNTLSUB_IF_CHECKSUM 				0x0022
#define CNTLSUB_CAL_MODE 					0x0040
#define CNTLSUB_RESET 						0x0041



struct TI_gauge_data
{
    uint16_t addr;//0x55
    struct i2c_client *client;
    int use_irq;
    struct hrtimer timer;
    struct workqueue_struct *ti_gauge_up_wq;//update workQ
    struct work_struct  work;
    uint16_t max[2];
};

static struct TI_gauge_data *ti_gauge_td=0;
static struct i2c_driver TI_gauge_driver;
static unsigned int gauge_initialized = 0;
static unsigned int gauge_fw_version= 0;
static unsigned int gauge_hw_version= 0;
static unsigned int gauge_cntl_status= 0;
static unsigned int gauge_flags= 0;
static struct mutex g_mutex;
static volatile unsigned int gauge_enable=1; // 1:enable bat read from gauge; 0:forbidden
static unsigned int smartupdate= 0;//当echo smartup时使能，之后更新固件的话会判断固件与文件版本，还会区别处理bqfs和dffs

#ifdef GAUGE_UPDATE_FIRMWARE
static volatile unsigned int gauge_update_state=0;// bit 1 : 1 during updating or 0 outupdate; bit 2: 1 already be the latest firmware; bit 3:1 update success or 0 update failed
static volatile int update_percent=0;// show the update progression
#define setbit(b) (gauge_update_state|=1<<(b))
#define clearbit(b) (gauge_update_state&=~(1<<(b)))
#define testbit(b) (gauge_update_state&(1<<(b)))
#endif

static int gauge_exit_update_mode(void);
static int gauge_enter_update_mode(void);
int gauge_cntlsub_read(int reg,unsigned int* readvalue);

/*
function:gauge_i2c_read
flag:用来说明是正常调用(0)还是update时调用(1)
*/
static int gauge_i2c_read(int reg, u8 * buf, int count)
{
    int rc;
    int ret = 0;
    struct i2c_client *client=0;

    if(ti_gauge_td==0)
    {
        printk("%s():%d,ti_gauge_td==0\n", __func__,__LINE__);
        return -1;
    }

    mutex_lock(&g_mutex);	//chenchongbao.2011.6.7	//chenchongbao.20110713_1 restore
    client=ti_gauge_td->client;
    buf[0] = reg;


    rc = i2c_master_send(client, buf, 1);
    if (rc != 1)
    {
        dev_err(&client->dev, "gauge_i2c_read FAILED: read of register 0x%x\n", reg);
        ret = -1;
        goto gauge_i2c_rd_exit;
    }
    rc = i2c_master_recv(client, buf, count);
    if (rc != count)
    {
        dev_err(&client->dev, "gauge_i2c_read FAILED: read %d bytes from reg 0x%x\n", count, reg);
        ret = -1;
    }

gauge_i2c_rd_exit:
    mutex_unlock(&g_mutex);
    return ret;
}

/*
fun:gauge_i2c_write
data[0]==reg
data[.]==data
*/
static int gauge_i2c_write(u8* data,int size)
{
    int rc;
    int ret = 0;
    struct i2c_client *client=0;
    if(ti_gauge_td==0)
    {
        printk("%s():%d,ti_gauge_td==0\n", __func__,__LINE__);
        return -1;
    }

    //mutex_lock(&g_mutex);	//chenchongbao.2011.6.7
    client=ti_gauge_td->client;

    rc = i2c_master_send(client, data, size);
    if (rc != size)
    {
        dev_err(&client->dev, "gauge_i2c_write FAILED: writing to reg %d; %d data has been written\n", data[0],rc);	//chenchongbao.20110713_1
        ret = -1;
    }
    //mutex_unlock(&g_mutex);
    return ret;
}


/*
function:gauge_mode
return: -1 communication failed; 0 work mode; 1 ROM mode;
*/
static int gauge_mode(void)
{
    unsigned int fw_version;
    unsigned int mode=-1;
    if(ti_gauge_td==0)
    {
        printk("%s():%d,ti_gauge_td==0\n", __func__,__LINE__);
        return -1;
    }
    ti_gauge_td->client->addr=0x55;
    if(!gauge_cntlsub_read( CNTLSUB_FW_VERSION,&fw_version))
    {
        mode=0;
        return mode;
    }

    ti_gauge_td->client->addr=0x0B;
    if(!gauge_cntlsub_read( CNTLSUB_FW_VERSION,&fw_version))
    {
        mode=1;
        return mode;
    }
    return mode;
}


/*
function:gauge_standard_read,read standard commands
return value: 0 ok; other err;
*/
static int gauge_standard_read(int reg,unsigned int* readvalue)
{
    u8 i2c_buf[16];
    int rc=-1;
    if(!testbit(0))
    {
        rc = gauge_i2c_read( reg, i2c_buf, 2);
        if(rc==0)
        {
            *readvalue=(i2c_buf[1]<<8)|i2c_buf[0];
        }
    }
    return  rc;
}

/*
function:bat_read_gauge,export to bat driver to read gauge reg
return value: 0 ok; other err;
*/
#define ABSM(a,b)	  ((a)>(b)?((a)-(b)):((b)-(a)))
int bat_read_gauge(int reg,unsigned int* readvalue)
{
    int rc=-1;
    //unsigned int voltage;
    static unsigned int normal_voltage=5000;
    static unsigned int normal_soc=1000;

    if(gauge_enable && (!testbit(0)) && gauge_initialized)//only when gauge_enabled and guage is not in update mode, bat can operate gauge!
    {
        /*
        由于P730A10电量计能检测到电池是否存在，当用电源供电时应该
        禁止电池驱动读取gauge，原因是电源供电会随意调节电压，而此时
        电量计就无法反映出那个电压应该的剩余容量，会干扰低电关机测试
        */
        if(!gauge_standard_read(REGADDR_FLAGS,&gauge_flags))
        {
            if(gauge_flags && gauge_flags!=0xff00)
            {
                if(!(gauge_flags&0x08))//电池不存在
                {
                    printk( "%s,battery not exist!\n", __func__);
                    rc=1;
                }
            }
        }
        if(rc==-1)//电池存在才允许读
        {
            rc=gauge_standard_read(reg,readvalue);
            if(rc==0)
            {
                if(REGADDR_VOLT==reg)
                {
                    if(*readvalue<3200 ||4300<*readvalue)
                {
                        printk( "%s,voltage=%u out of range! old_voltage is %u\n", __func__,*readvalue, normal_voltage);
                        //gauge_enable=0;//遇到了电压读回来175mv，容量1%,那就应该禁止电池来读了
                        if(normal_voltage >=3200 && normal_voltage<=4300)
                        {
                            printk( "%s,voltage replace by old value=%u\n", __func__,normal_voltage);
                            *readvalue=normal_voltage;
                        }
                }
                    else
                    {
                        if(ABSM(*readvalue,normal_voltage)>400)//400mv
                        {
                            printk( "%s,voltage change too much from %u to %u\n", __func__,normal_voltage,*readvalue);
                        }
                        normal_voltage=*readvalue;
                    }
                }
                else if(REGADDR_SOC==reg)  // 4v高电压出现0%的情况，应该是开机测开路电压时有电流造成的，如果没有保障2s的等待时间，在这里优化一下
                {//如果容量为0，则看看电压是否高于3.4v，高于则使用modem侧的容量
                    if(*readvalue > 100)
                    {
                        printk( "%s,soc=%u out of range\n", __func__,*readvalue);
                        if(normal_soc<=100)
                        {
                            printk( "%s,soc replace by old value=%u\n", __func__,normal_soc);
                            *readvalue=normal_soc;
                        }
                    }
                    else
                    {
                        if(ABSM(*readvalue,normal_soc)>5)//突然跳变了5%
                        {
                            printk( "%s,soc change too much from %u to %u\n", __func__,normal_soc,*readvalue);
			//*readvalue=normal_soc;		//chenchongbao.20110713_1
			if(normal_soc != 1000)			//chenchongbao.20110713_1		7.14 add
				*readvalue=normal_soc;
                        }//else						//chenchongbao.20110713_1		7.14: else will cause normal_soc == 1000 always! 1000(0x3E8) E8=232% !!!
                        normal_soc=*readvalue;
                    }
                        }
                    }
                }
            }
    else
    {
        printk( "%s, bat can't gauge!enable=%u,updating=%u,gauge_initialized=%u\n", __func__,gauge_enable,testbit(0),gauge_initialized);
        }
    if(rc)
    {
        printk( "%s, ===bat read gauge failed!\n", __func__);
    }
    return rc;
}

/*
function:gauge_cntlsub_read,read control subcommands
return value: 0 ok; other err;
*/
int gauge_cntlsub_read(int reg,unsigned int* readvalue)
{
    u8 i2c_buf[16];
    int rc=-1;
    if(!testbit(0))
    {

        //write cntl data
        i2c_buf[0]=REGADDR_CNTL;
        i2c_buf[1]=reg&0x00ff;//实践证明先写cntldata低8bit或者高8bit都没有问题，结果是一致的
        i2c_buf[2]=(reg&0xff00)>>8;
        //printk("%s():i2c write buf:%02x %02x %02x\n", __func__,i2c_buf[0],i2c_buf[1],i2c_buf[2]);
		mdelay(1);
	mutex_lock(&g_mutex);	//chenchongbao.2011.6.7
        rc = gauge_i2c_write( i2c_buf,3);
        if(rc)// -1 err; 0 ok;
        {
            printk("%s():i2c write failed!buf:%02x %02x %02x\n", __func__,i2c_buf[0],i2c_buf[1],i2c_buf[2]);
            return rc;
        }
	mdelay(2);

        //read result from cntl reg
        memset(i2c_buf,0,16);
	mutex_unlock(&g_mutex);	//chenchongbao.20110713_1
        rc=gauge_i2c_read( REGADDR_CNTL, i2c_buf, 2);
	//mutex_unlock(&g_mutex);	//chenchongbao.2011.6.7
        if(rc==0)
        {
            *readvalue=(i2c_buf[1]<<8)|i2c_buf[0];
        }
    }
    return  rc;
}

//yintianci_bqfs_20100930_1
/*
function:gauge_enabel_it, called only after updating in this .c file
flag:0 disable,1 enable
return value: 0 ok; other err;
*/
int gauge_enabel_it(int flag)
{
    unsigned int value=0;
    int rc=-1;
    printk("%s():enter\n", __func__);
    if(!testbit(0))//
    {
    #if 0
        //使能前先判断电池是否存在
        if(!gauge_standard_read(REGADDR_FLAGS,&gauge_flags))
        {
            if(gauge_flags && gauge_flags!=0xff00)
            {
                if(!(gauge_flags&0x08))//电池不存在
                {
                    rc=1;
                }
            }
        }
		#endif
        if(rc==-1)//电池存在才允许使能it
        {
            rc=gauge_cntlsub_read(CNTLSUB_IT_ENABLE,&value);
            if(rc==0)
            {
                printk("%s():success!rc:%d, value:%02x\n", __func__,rc,value);
            }
            else
            {
                printk("%s():failed!rc:%d, value:%02x\n", __func__,rc,value);
            }
        }
    }
    return rc;
}

//EXPORT(gauge_standard_read)//让msm_battery来读取各种电池信息

static ssize_t gauge_show_upmode(struct device_driver *driver, char *buf)
{
    int mode= -10;
    ssize_t rc=0;
    printk("%s():%d enter\n", __func__,__LINE__);
    if(testbit(0))
    {
        //communication ok, during update
        rc=snprintf(buf, PAGE_SIZE, "during updating\n");
    }
    else
    {
        mode=gauge_mode();
        switch (mode)
        {
        default:
        case -1:
            // communication failed
            rc=snprintf(buf, PAGE_SIZE, "communication failed!\n");
            break;
        case 0:
            //workmode
            rc=snprintf(buf, PAGE_SIZE, "work mode\n");
            break;
        case 1:
            //rom mode
            rc=snprintf(buf, PAGE_SIZE, "rom mode\n");
            break;
        }
    }
    return rc;//snprintf(buf, PAGE_SIZE, "upmode:%s \n", testbit(0)?"IN UPDATE":"OUT UPDATE");
}
/*
function:gauge_store_mode
usage:
*/
static ssize_t gauge_store_upmode(struct device_driver *driver,
                                  const char *buf, size_t count)
{
    char *p = (char *)buf;
    unsigned int mode= (unsigned int)simple_strtol(p, NULL, 16);

    printk("%s():enter,get mode:0x%08x from \"%s\"\n", __func__,mode,buf);
    if(mode ==0)
    {
        if(gauge_exit_update_mode())
        {
            printk("%s:  exit ROM Mode %s,l:%d \n", __func__,"failed",__LINE__);
        }
    }
    else if(mode ==1)//
    {
        if(!gauge_enter_update_mode())
        {
            printk("%s:  Enter ROM Mode %s,l:%d \n", __func__,"failed",__LINE__);
        }
    }
    printk("%s(): exit,line:%d\n", __func__,__LINE__);
    return strnlen(buf, count);
}


static ssize_t gauge_show_enable(struct device_driver *driver, char *buf)
{
    printk("%s():line:%d\n", __func__,__LINE__);
    return snprintf(buf, PAGE_SIZE, "%s bat driver read from gauge.\n", gauge_enable?"enable":"forbid");
}
/*
function:gauge_store_enable
usage:take disable for example,type "echo 0 > /sys/bus/i2c/drivers/ti-fuel-gauge/enable"
*/
static ssize_t gauge_store_enable(struct device_driver *driver,
                                  const char *buf, size_t count)
{
    char *p = (char *)buf;
    unsigned int enable= 1;
    enable=(unsigned int)simple_strtol(p, NULL, 10);

    printk("%s():enter,get enable:%d from \"%s\"\n", __func__,enable,buf);
    if(enable==0 ||enable==1)
    {
        gauge_enable=enable;
    }
    else
    {
        printk("%s():get err enable value:%d \n", __func__,enable);
    }
    printk("%s(): exit,line:%d\n", __func__,__LINE__);
    return strnlen(buf, count);
}


static ssize_t gauge_show_Control(struct device_driver *driver, char *buf)
{
    unsigned int readvalue=0;
    // int rc=gauge_standard_read(REGADDR_CNTL,&readvalue);
    int rc=gauge_cntlsub_read( CNTLSUB_CONTROL_STATUS,&readvalue);
    printk("%s(): rc:%d, readvalue:%d,line:%d\n", __func__,rc,readvalue,__LINE__);
    if(rc==0)
    {
        return snprintf(buf, PAGE_SIZE, "CNTL:0x%08x \n", readvalue);
    }
    else
    {
        return snprintf(buf, PAGE_SIZE, "read failed,rc=%d\n",rc);
    }
}
/*
function:gauge_store_Control
usage:take set_hibernate for example,type "echo 0x11 > /sys/bus/i2c/drivers/ti-fuel-gauge/Control"
*/
static ssize_t gauge_store_Control(struct device_driver *driver,
                                   const char *buf, size_t count)
{
    char *p = (char *)buf;
    u8 i2c_buf[16];
    unsigned int cntldata= (unsigned int)simple_strtol(p, NULL, 16);
    int rc=0;
    printk("%s():enter,get contldata:0x%04x from \"%s\"\n", __func__,cntldata,buf);
    if(!testbit(0))
    {
        if((cntldata <=0x9) //0~9
                ||((0x11<= cntldata) &&(cntldata <=0x14)) // 11 ~ 14
                ||((0x20<= cntldata) &&(cntldata <=0x22)) // 20 ~ 22
                ||((0x40<= cntldata) &&(cntldata <=0x41)) // 40 ~ 41
          )// right value
        {
            //go on
        }
        else
        {
            printk("%s():err contldata:0x%04x not support!\n", __func__,cntldata);
            return strnlen(buf, count);
        }

        //write cntl data
        i2c_buf[0]=REGADDR_CNTL;
        i2c_buf[1]=cntldata&0x00ff;//实践证明先写cntldata低8bit或者高8bit都没有问题，结果是一致的
        i2c_buf[2]=(cntldata&0xff00)>>8;
        printk("%s():i2c write buf:%02x %02x %02x\n", __func__,i2c_buf[0],i2c_buf[1],i2c_buf[2]);
        rc = gauge_i2c_write( i2c_buf,3);
        if(rc)// -1 err; 0 ok;
        {
            printk("%s():i2c write failed!buf:%02x %02x %02x\n", __func__,i2c_buf[0],i2c_buf[1],i2c_buf[2]);
            return strnlen(buf, count);
        }

#if 0//test
        //read result from cntl reg
        //但是每个cntl data返回的信息的长度目前还不清楚
        memset(i2c_buf,0,16);
        rc=gauge_i2c_read(ti_gauge_td->client, REGADDR_CNTL, i2c_buf, 16);
        printk("%s():CNTL:0x", __func__);
        rc=16;
        while(rc)
        {
            printk("%02x ", i2c_buf[rc-1]);
            rc--;
        }
        printk("\n");

#endif

    }

    printk("%s(): exit,line:%d\n", __func__,__LINE__);
    return strnlen(buf, count);
}


static ssize_t gauge_show_temperature(struct device_driver *driver, char *buf)
{
    unsigned int readvalue=0;
    int rc=gauge_standard_read(REGADDR_TEMP,&readvalue);
    printk("%s(): enter,line:%d\n", __func__,__LINE__);
    if(rc==0)
    {
        return snprintf(buf, PAGE_SIZE, "TEMP:%d ℃\n", (readvalue*10-27315)/100);
    }
    else
    {
        return snprintf(buf, PAGE_SIZE, "read failed,rc=%d\n",rc);
    }
    printk("%s(): exit,line:%d\n", __func__,__LINE__);
}
static ssize_t gauge_show_voltage(struct device_driver *driver, char *buf)
{
    unsigned int readvalue=0;
    int rc=gauge_standard_read(REGADDR_VOLT,&readvalue);
    printk("%s(): rc:%d, readvalue:%d,line:%d\n", __func__,rc,readvalue,__LINE__);
    if(rc==0)
    {
        return snprintf(buf, PAGE_SIZE, "VOLT:%u mV\n", readvalue);
    }
    else
    {
        return snprintf(buf, PAGE_SIZE, "read failed,rc=%d\n",rc);
    }
}
static ssize_t gauge_show_flag(struct device_driver *driver, char *buf)
{
    unsigned int readvalue=0;
    int rc=gauge_standard_read(REGADDR_FLAGS,&readvalue);
    printk("%s(): rc:%d, readvalue:%d,line:%d\n", __func__,rc,readvalue,__LINE__);
    if(rc==0)
    {
        return snprintf(buf, PAGE_SIZE, "FLAG:0x%04x\n", readvalue);
    }
    else
    {
        return snprintf(buf, PAGE_SIZE, "read failed,rc=%d\n",rc);
    }
}
static ssize_t gauge_show_NominalAvailableCapacity(struct device_driver *driver, char *buf)
{
    unsigned int readvalue=0;
    int rc=gauge_standard_read(REGADDR_NAC,&readvalue);
    printk("%s(): rc:%d, readvalue:%d,line:%d\n", __func__,rc,readvalue,__LINE__);
    if(rc==0)
    {
        return snprintf(buf, PAGE_SIZE, "NAC:%u mAh\n", readvalue);
    }
    else
    {
        return snprintf(buf, PAGE_SIZE, "read failed,rc=%d\n",rc);
    }
}
static ssize_t gauge_show_FullAvailableCapacity(struct device_driver *driver, char *buf)
{
    unsigned int readvalue=0;
    int rc=gauge_standard_read(REGADDR_FAC,&readvalue);
    printk("%s(): rc:%d, readvalue:%d,line:%d\n", __func__,rc,readvalue,__LINE__);
    if(rc==0)
    {
        return snprintf(buf, PAGE_SIZE, "FAC:%u mAh\n", readvalue);
    }
    else
    {
        return snprintf(buf, PAGE_SIZE, "read failed,rc=%d\n",rc);
    }
}
static ssize_t gauge_show_RemainingCapacity(struct device_driver *driver, char *buf)
{
    unsigned int readvalue=0;
    int rc=gauge_standard_read(REGADDR_RM,&readvalue);
    printk("%s(): rc:%d, readvalue:%d,line:%d\n", __func__,rc,readvalue,__LINE__);
    if(rc==0)
    {
        return snprintf(buf, PAGE_SIZE, "RM:%u mAh\n", readvalue);
    }
    else
    {
        return snprintf(buf, PAGE_SIZE, "read failed,rc=%d\n",rc);
    }
}
static ssize_t gauge_show_FullChargeCapacity(struct device_driver *driver, char *buf)
{
    unsigned int readvalue=0;
    int rc=gauge_standard_read(REGADDR_FCC,&readvalue);
    printk("%s(): rc:%d, readvalue:%d,line:%d\n", __func__,rc,readvalue,__LINE__);
    if(rc==0)
    {
        return snprintf(buf, PAGE_SIZE, "FCC:%u mAh\n", readvalue);
    }
    else
    {
        return snprintf(buf, PAGE_SIZE, "read failed,rc=%d\n",rc);
    }
}
static ssize_t gauge_show_current(struct device_driver *driver, char *buf)
{
    unsigned int readvalue=0;
    short int currentint=0;//电流可能是负值，表明在充电
    int rc=gauge_standard_read(REGADDR_AI,&readvalue);
    printk("%s(): rc:%d, readvalue:%d,line:%d\n", __func__,rc,readvalue,__LINE__);
    currentint=readvalue&0xffff;
    if(rc==0)
    {
        return snprintf(buf, PAGE_SIZE, "AI:%d mA\n", currentint);
    }
    else
    {
        return snprintf(buf, PAGE_SIZE, "read failed,rc=%d\n",rc);
    }
}
static ssize_t gauge_show_TimeToEmpty(struct device_driver *driver, char *buf)
{
    unsigned int readvalue=0;
    int rc=gauge_standard_read(REGADDR_TTE,&readvalue);
    printk("%s(): rc:%d, readvalue:%d,line:%d\n", __func__,rc,readvalue,__LINE__);
    if(rc==0)
    {
        return snprintf(buf, PAGE_SIZE, "TTE:%u min\n", readvalue);
    }
    else
    {
        return snprintf(buf, PAGE_SIZE, "read failed,rc=%d\n",rc);
    }
}
static ssize_t gauge_show_MaxLoadCurrent(struct device_driver *driver, char *buf)
{
    unsigned int readvalue=0;
    int rc=gauge_standard_read(REGADDR_MLI,&readvalue);
    printk("%s(): rc:%d, readvalue:%d,line:%d\n", __func__,rc,readvalue,__LINE__);
    if(rc==0)
    {
        return snprintf(buf, PAGE_SIZE, "MLI:%u mA\n", readvalue);
    }
    else
    {
        return snprintf(buf, PAGE_SIZE, "read failed,rc=%d\n",rc);
    }
}
static ssize_t gauge_show_MaxLoadTimeToEmpty(struct device_driver *driver, char *buf)
{
    unsigned int readvalue=0;
    int rc=gauge_standard_read(REGADDR_MLTTE,&readvalue);
    printk("%s(): rc:%d, readvalue:%d,line:%d\n", __func__,rc,readvalue,__LINE__);
    if(rc==0)
    {
        return snprintf(buf, PAGE_SIZE, "MLTTE:%u min\n", readvalue);
    }
    else
    {
        return snprintf(buf, PAGE_SIZE, "read failed,rc=%d\n",rc);
    }
}
static ssize_t gauge_show_CycleCount(struct device_driver *driver, char *buf)
{
    unsigned int readvalue=0;
    int rc=gauge_standard_read(REGADDR_CC,&readvalue);
    printk("%s(): rc:%d, readvalue:%d,line:%d\n", __func__,rc,readvalue,__LINE__);
    if(rc==0)
    {
        return snprintf(buf, PAGE_SIZE, "CC:%u times\n", readvalue);
    }
    else
    {
        return snprintf(buf, PAGE_SIZE, "read failed,rc=%d\n",rc);
    }
}
static ssize_t gauge_show_StateOfCharge(struct device_driver *driver, char *buf)
{
    unsigned int readvalue=0;
    int rc=gauge_standard_read(REGADDR_SOC,&readvalue);
    printk("%s(): rc:%d, readvalue:%d,line:%d\n", __func__,rc,readvalue,__LINE__);
    if(rc==0)
    {
        return snprintf(buf, PAGE_SIZE, "SOC:%u %%\n", readvalue);
    }
    else
    {
        return snprintf(buf, PAGE_SIZE, "read failed,rc=%d\n",rc);
    }
}
#if 0		/* chenchongbao.2011.7.8 for google CTS test */
static DRIVER_ATTR(enable, S_IRWXUGO, gauge_show_enable, gauge_store_enable);	//can read write execute
static DRIVER_ATTR(upmode, S_IRWXUGO, gauge_show_upmode, gauge_store_upmode);	//can read write execute
static DRIVER_ATTR(Control, S_IRWXUGO, gauge_show_Control, gauge_store_Control);	//can read write execute
#else
static DRIVER_ATTR(enable, S_IRWXU|S_IRWXG, gauge_show_enable, gauge_store_enable);	//can read write execute
static DRIVER_ATTR(upmode, S_IRWXU|S_IRWXG, gauge_show_upmode, gauge_store_upmode);	//can read write execute
static DRIVER_ATTR(Control, S_IRWXU|S_IRWXG, gauge_show_Control, gauge_store_Control);	//can read write execute
#endif
static DRIVER_ATTR(batTemperature, S_IRUGO, gauge_show_temperature, NULL); //显示实时电流
static DRIVER_ATTR(batVoltage, S_IRUGO, gauge_show_voltage, NULL);
static DRIVER_ATTR(Flags, S_IRUGO, gauge_show_flag, NULL);
static DRIVER_ATTR(batNominalAvailableCapacity, S_IRUGO, gauge_show_NominalAvailableCapacity, NULL);
static DRIVER_ATTR(batFullAvailableCapacity, S_IRUGO, gauge_show_FullAvailableCapacity, NULL);
static DRIVER_ATTR(batRemainingCapacity, S_IRUGO, gauge_show_RemainingCapacity, NULL);
static DRIVER_ATTR(batFullChargeCapacity, S_IRUGO, gauge_show_FullChargeCapacity, NULL);
static DRIVER_ATTR(batCurrent, S_IRUGO, gauge_show_current, NULL); //显示实时电流
static DRIVER_ATTR(batTimeToEmpty, S_IRUGO, gauge_show_TimeToEmpty, NULL);
static DRIVER_ATTR(batMaxLoadCurrent, S_IRUGO, gauge_show_MaxLoadCurrent, NULL);
static DRIVER_ATTR(batMaxLoadTimeToEmpty, S_IRUGO, gauge_show_MaxLoadTimeToEmpty, NULL);
static DRIVER_ATTR(batCycleCount, S_IRUGO, gauge_show_CycleCount, NULL);
static DRIVER_ATTR(batStateOfCharge, S_IRUGO, gauge_show_StateOfCharge, NULL);


#if 0
static void TI_gauge_work_func(struct work_struct *work)//处理低电告警中断
{
    // uint8_t buf[16];
    struct TI_gauge_data *pgauge_data = container_of(work, struct TI_gauge_data, work);
    /*
    读取gauge的iic，更新拔出电池等信息到smem里
    */
    enable_irq(pgauge_data->client->irq);
}


static int TI_gauge_irq_handler(int irq, void *dev_id)
{
    struct TI_gauge_data *pgauge_data = dev_id;
    /* printk("synaptics_ts_irq_handler\n"); */
    disable_irq_nosync(pgauge_data->client->irq);
    queue_work(pgauge_data->ti_gauge_wq, &pgauge_data->work);
    return IRQ_HANDLED;
}
#endif

#ifdef GAUGE_UPDATE_FIRMWARE//update relative
void ErrorTrap(unsigned char bErrorNumber)
{
#if 0
    // Set all pins to highZ to avoid back powering the PSoC through the GPIO
    // protection diodes.
    SetSCLKHiZ();
    SetSDATAHiZ();
    // If Power Cycle programming, turn off the target
    //RemoveTargetVDD();
#endif
    //while (1);
    printk("ErrorTrap:go to error!\n");
    //return ;
}

/*
fuction:ParseAValidateLine,like "W: 16 00 04"
in:pwalk,point to a string
out:cmd,1:w;2:R;3:c;4:x
out:data,return the parsed data behind the command string
in:size,the size of data buf
return:the usage of 'data[]' in byte ,0 indicate err
*/
int ParseAValidateLine(char** ppwalk,int* cmd,u8 *data,unsigned int size)
{
    int rc=0;
    //   u8 hexdata=0;
    char *tempstr=0;
    char *pwalk=*ppwalk;
    unsigned int *tempx=0;
#if 0

    printk("%s: enter,ppwalk :\n", __FUNCTION__);
    while(rc < 16)
    {
        printk("%02x ", pwalk[rc]);
        rc++;
    }
    rc=0;
    printk("\n");
#endif
#if 0//YINTIANCI_GAUGE_20101214
    tempstr=strstr(pwalk,": ");
#else
    tempstr=pwalk;
    while(!(((*tempstr) == ':') && ((*(tempstr+1)) == ' ')))
    {
        tempstr++;
	if( (tempstr - pwalk) > 12 )	//chenchongbao.20110713_1 it will cause dead loop! unsightned int max is 0xFFFFFFFF(4294967295) 10bit !
	{
		*ppwalk=tempstr;
		*cmd=0;
		return 0;
	}
    }
#endif
    if(tempstr)
    {
        pwalk=tempstr-1;
    }
    else
    {
        printk("%s: line:%d\n", __FUNCTION__,__LINE__);
        *ppwalk=pwalk+1;
        return rc;
    }
    //    printk("%s: line:%d\n", __FUNCTION__,__LINE__);
    switch (*pwalk)
    {
    case 'W':
        //W: I2CAddr RegAddr Byte0 Byte1 Byte2…
        //eg    W: 16 00 04
        *cmd=1;
        break;
    case 'R':
        //R: I2CAddr RegAddr NumBytes
        //R: AA 55 100
        *cmd=2;
        break;
    case 'C':
        //C: i2cAddr RegAddr Byte0 Byte1 Byte2
        //C: AA 55 AB CD EF 00
        *cmd=3;
        break;
    case 'X':
        //X: 200
        *cmd=4;
        break;
    default://err
        *cmd=0;
        break;
    }
    //    printk("%s: line:%d,cmd:%d\n", __FUNCTION__,__LINE__,*cmd);
    if((*cmd==1) || (*cmd==3))//
    {
        //      printk("%s: line:%d,cmd:%d\n", __FUNCTION__,__LINE__,*cmd);
        pwalk+=6;
        while(*pwalk != '\n')
        {
            if(0x30<=*pwalk && *pwalk <=0x39)
            {
                data[rc/2]|=(*pwalk-0x30)<<(((rc+1)%2)*4);
                rc++;
            }
            else if(0x41<=*pwalk && *pwalk <=0x46)
            {
                data[rc/2]|=(*pwalk-0x37)<<(((rc+1)%2)*4);
                rc++;
            }
            else if(*pwalk!=' ')
            {
                //err
            }
            pwalk++;
        }

    }
    else if(*cmd==4)
    {
        //     printk("%s: line:%d\n", __FUNCTION__,__LINE__);
        pwalk+=3;
        tempx=(unsigned int *)data;
        *((unsigned int *)data)= (unsigned int)simple_strtol(pwalk, NULL, 10);
        rc=8;
    }
    else if(*cmd==2)//R: AA 55 100
    {
        //      printk("%s: line:%d\n", __FUNCTION__,__LINE__);
        pwalk+=6;
        data[0]= (unsigned int)simple_strtol(pwalk, NULL, 16);
        tempx=(unsigned int *)(&data[1]);
        pwalk+=3;
        *((unsigned int *)(&data[1]))= (unsigned int)simple_strtol(pwalk, NULL, 10);
        rc=10;
    }
    *ppwalk=pwalk;
    return rc/2;
}
/*
function:parse_execute,pase *.bqfs file and execute the instruction
return value: 0:success; >0 :how many instructions have been executed
*/
#define linesizeMax 300
static int gauge_parse_execute(char *fw_buf,int length)
{

    int cmd=0;
    static int local_update_percent=0;
    int rtsize=0;
    int executNO=0;
    char* pwalk=fw_buf;

    u8 data_buf[linesizeMax];//receive ParseAValidateLine() parsed data
#ifndef GAUGE_UPDATE_TEST_PARSE

    u8 read_buf[linesizeMax];// read from i2c
    u8 RegAddr;//存储i2c寄存器地址
    int rc=0;
    unsigned int size=0;
#endif

    //unsigned int printcount=0;
    update_percent=0;
    printk("%s(): enter,line:%d\n", __func__,__LINE__);
    while(pwalk < (fw_buf+length))
    {
        //msleep(10);//仅仅为了测试
        memset(data_buf,0,linesizeMax);
        cmd=0;
        rtsize=ParseAValidateLine(&pwalk,&cmd,data_buf,linesizeMax);
        executNO+=(cmd?1:0);
        update_percent=(pwalk-fw_buf)*100/length;
        if(update_percent!=local_update_percent)//only for log
        {
            printk("%s():%d,percent:%d\n", __func__,__LINE__,update_percent);
            local_update_percent=update_percent;
        }
        if(cmd==0)//已经解析完毕，推出
        {
            setbit(2);
            printk("%s():%d,executNO:%d,goto exit\n", __func__,__LINE__,executNO);
            goto gauge_parse_execute_exit;
        }

#if 0
        printk("%s():parsedlinecount:%d,rtsize:%d,cmd:%d,data_buf:0x", __func__,executNO,rtsize,cmd);
        while(printcount < rtsize)
        {
            printk("%02x ", data_buf[printcount]);
            printcount++;
        }
        printcount=0;
        printk("\n");
#endif

#ifndef GAUGE_UPDATE_TEST_PARSE

        switch(cmd)
        {
        case 1://w
            //printk("%s():i2c write buf:%02x %02x %02x\n", __func__,i2c_buf[0],i2c_buf[1],i2c_buf[2]);
            rc = gauge_i2c_write( data_buf,rtsize);
            if(rc)
            {
                printk("%s:  i2c write %s \n", __func__,rc?"failed":"ok");
                return executNO-1;
            }
            break;
        case 2://r
            RegAddr=data_buf[0];
            size=*((unsigned int *)(&data_buf[1]));
            //memset(data_buf,0,linesizeMax);
            rc = gauge_i2c_read(RegAddr,read_buf,size);
            if(rc)
            {
                printk("%s:  i2c read %s \n", __func__,rc?"failed":"ok");
                return executNO-1;
            }
            break;
        case 3://c
            RegAddr=data_buf[0];
            size=rtsize-1;
            rc = gauge_i2c_read( RegAddr,read_buf,size);
            if(rc)
            {
                printk("%s:  i2c read %s \n", __func__,rc?"failed":"ok");
                return executNO-1;
            }
            if(strncmp(read_buf,&data_buf[1],size))//not match,so return
            {
                printk("%s:  compare failed!!!at %d line, %02x%02x%02x%02x\n", __func__,executNO,data_buf[1],data_buf[2],data_buf[3],data_buf[4]);
                return executNO-1;
            }
            break;
        case 4://x
            {
                if(*((unsigned int *)data_buf) <= 2)
                {
                    mdelay(*((unsigned int *)data_buf) );//cost 77s
                }
                else
                {
                    msleep(*((unsigned int *)data_buf));// cost 90s
                }
                break;
            }
        default:
            break;
        }
#endif

    }
gauge_parse_execute_exit:
    printk("%s(): exit,line:%d\n", __func__,__LINE__);
    return 0;
}

/*
function:gauge_enter_update_mode
return value: 0:failed  1:success
*/
static int gauge_enter_update_mode(void)
{
    char wtbuf[16];
    int rc=0;

    printk("%s(): enter,line:%d\n", __func__,__LINE__);
    //Command to Enter ROM Mode in I2C,slua541.pdf
    wtbuf[0]=REGADDR_CNTL;
    wtbuf[1]=0x00;
    wtbuf[2]=0x0f;
    if(ti_gauge_td==0)
    {
        printk("%s():%d,ti_gauge_td==0\n", __func__,__LINE__);
        return 0;
    }
    ti_gauge_td->client->addr=0x55;
    rc = gauge_i2c_write( wtbuf,3);
    if(rc)
    {
        printk( "%s:exit,Enter ROM Mode %s,l:%d \n", __func__,rc?"failed":"ok",__LINE__);
        ti_gauge_td->client->addr=0x0B; //the I2C addr is 0x0b in ROM mode
        return 0;
    }
    msleep(250);
    ti_gauge_td->client->addr=0x0B; //the I2C addr is 0x0b in ROM mode
    if (!i2c_check_functionality(ti_gauge_td->client->adapter, I2C_FUNC_I2C))
    {
        printk( "%s:exit,i2c_check_functionality failed\n",__func__);
        return 0;
    }
    printk( "%s(): exit,line:%d\n", __func__,__LINE__);
    return 1;
}

/*
function:gauge_exit_update_mode
return value: 1:failed  0:success
*/
static int gauge_exit_update_mode(void)
{
    char wtbuf[16];
    int rc=0;
    printk( "%s(): enter,line:%d\n", __func__,__LINE__);
    //ROM Mode Exit Sequence,slua541.pdf
    wtbuf[0]=REGADDR_CNTL;
    wtbuf[1]=0x0f;
    if(ti_gauge_td==0)
    {
        printk("%s():%d,ti_gauge_td==0\n", __func__,__LINE__);
        return 1;
    }
    ti_gauge_td->client->addr=0x0B;
    rc = gauge_i2c_write( wtbuf,2);
    if(rc)
    {
        printk( "%s:  exit ROM Mode %s,l:%d \n", __func__,rc?"failed":"ok",__LINE__);
        goto gauge_exit_update_mode_exit;
    }
    wtbuf[0]=0x64;
    wtbuf[1]=0x0f;
    rc = gauge_i2c_write(wtbuf,2);
    if(rc)
    {
        printk( "%s:  exit ROM Mode %s,l:%d \n", __func__,rc?"failed":"ok",__LINE__);
        goto gauge_exit_update_mode_exit;
    }
    wtbuf[0]=0x65;
    wtbuf[1]=0x00;
    rc = gauge_i2c_write( wtbuf,2);
    if(rc)
    {
        printk( "%s:  exit ROM Mode %s,l:%d \n", __func__,rc?"failed":"ok",__LINE__);
        goto gauge_exit_update_mode_exit;
    }
    ti_gauge_td->client->addr=0x55;//the I2C addr return to normal mode
    msleep(250);
    if (!i2c_check_functionality(ti_gauge_td->client->adapter, I2C_FUNC_I2C))
    {
        printk( "%s: exit, i2c_check_functionality failed\n",__func__);
        goto gauge_exit_update_mode_exit;
    }
    printk( "%s(): exit success,line:%d\n", __func__,__LINE__);
    return 0;
gauge_exit_update_mode_exit:
    ti_gauge_td->client->addr=0x55;//发现烧完后即便这个退出ROM mode过程异常了，但实际器件已经退出rom mode了，所以直接把地址改到正常地址
    printk( "%s(): exit failed,line:%d\n", __func__,__LINE__);
	   clearbit(0); // YINTIANCI_GAUGE_IT_ENABLE_20110311
    return rc;

}

/*
function:gauge_readfile
read from "filestr" file,return the file buf to fw_buf,the caller must free the fw_buf buffer.
return value:0 success;-1 failed
*/
int gauge_readfile(const char *filestr,char **fw_buf,int * length)
{
    struct file     *filp;
    struct inode    *inode = NULL;
    int tc=0;

    printk( "%s(): enter,line:%d\n", __func__,__LINE__);

    filp = filp_open(filestr, O_RDONLY, S_IRUSR);
    if ( IS_ERR(filp) )
    {
        printk("%s: file %s filp_open error\n", __FUNCTION__, filestr);
        goto gauge_readfile_exit;
    }
    if (!filp->f_op)
    {
        printk("%s: File Operation Method Error\n", __FUNCTION__);
        goto gauge_readfile_filp_open_exit;
    }
    inode = filp->f_path.dentry->d_inode;

    if (!inode)
    {
        printk("%s: Get inode from filp failed\n", __FUNCTION__);
        goto gauge_readfile_filp_open_exit;
    }

    printk("%s file offset opsition: %xh\n", __FUNCTION__, (unsigned)filp->f_pos);

    *length = i_size_read(inode->i_mapping->host);
    if (*length == 0)
    {
        printk("%s: Try to get file size error\n", __FUNCTION__);
        goto gauge_readfile_filp_open_exit;
    }
    printk("%s: length=%d\n", __FUNCTION__, *length);
    *fw_buf = (char*)kmalloc((*length+1), GFP_KERNEL);
    if (*fw_buf == NULL)
    {
        printk("%s: kernel memory alloc error\n", __FUNCTION__);
        goto gauge_readfile_filp_open_exit;
    }
    printk("%s: kernel memory alloc ok\n", __FUNCTION__);
    if (filp->f_op->read(filp, *fw_buf, *length, &filp->f_pos) != *length)
    {
        printk("%s: file read error\n", __FUNCTION__);

        printk("%s: filecontent :\n", __FUNCTION__);
        while(tc < 16)
        {
            printk("%02x ", *fw_buf[tc]);
            tc++;
        }
        printk("\n");

        goto gauge_readfile_kmalloc_exit;
    }
#if 0
    printk("%s: filecontent :\n", __FUNCTION__);
    while(tc < 16)
    {
        printk("%02x ", fw_buf[tc]);
        tc++;
    }
    printk("\n");
#endif

    printk( "%s(): read success! exit,line:%d\n", __func__,__LINE__);
    return 0;

gauge_readfile_kmalloc_exit:
    kfree(*fw_buf);
gauge_readfile_filp_open_exit:
    filp_close(filp, NULL);
gauge_readfile_exit:
    return -1;
}
//#define GAUGE_ENUM_BQFS
static void TI_gauge_update_work_func(struct work_struct *work)
{
    mm_segment_t    oldfs;//
    char *fw_buf = NULL;
    char * walkp;
    int length=0;
    int gaugemode=0;
    int rc=0;
    int i=0;
    struct wake_lock gauge_up_wake_lock;
    unsigned int firmwareVersion=0;//bqfs文件版本
    //#define BQFS "/data/local/ti_gauge.bqfs";
#define BQFS "/system/etc/ti_gauge.bqfs" // data and firmware
#define DFFS "/system/etc/ti_gauge.dffs"//only data


    printk( "%s(): enter,line:%d\n", __func__,__LINE__);

    oldfs = get_fs();//
    set_fs(KERNEL_DS);//

    gaugemode=gauge_mode();
    if(gaugemode==1)
    {
        smartupdate=0;
    }
    else if(gaugemode==-1)
    {
        printk( "%s():%d,gauge broken out!\n", __func__,__LINE__);
        goto set_fs_exit;
    }
mode_raise_point:
	#define retry_times 300
    if(smartupdate)
    {
        for(i=0;i<retry_times;i++)
        {
            if(gauge_cntlsub_read( CNTLSUB_FW_VERSION,&gauge_fw_version))//重新读一次，probe中读回来始终是0xff00
            {//failed
                gauge_fw_version=0;
                return; // 读取器件都失败，下载肯定无法进行，直接返回
            }
            else if(gauge_fw_version==0xff00)//读取值无效，重读三次
            {
                msleep(10);
                continue;
            }
            else// 读取值有效，跳出
            {
                break;
            }
        }
        if(i==retry_times)
        {
            smartupdate=0;
            goto mode_raise_point;
        }
        printk( "%s() line:%d,validate fw:%x\n", __func__,__LINE__,gauge_fw_version);
        if(gauge_fw_version&0xf000)
        {
            goto set_fs_exit;
        }
        printk( "%s() line:%d,validate fw:%x\n", __func__,__LINE__,gauge_fw_version);

        if(gauge_readfile(DFFS,&fw_buf,&length)==-1)//read failed
        {
            printk( "%s() line:%d,read data file failed\n", __func__,__LINE__);
            goto set_fs_exit;// 读文件都失败，下载肯定无法进行，直接返回
        }

        walkp=strstr(fw_buf,"FWV:");
        walkp+=4;
        firmwareVersion=(unsigned int)simple_strtol(walkp, &walkp, 16);
        printk( "%s() line:%d, device fwv:%x: file fwv:%x\n", __func__,__LINE__,gauge_fw_version,firmwareVersion);
#if 0

        if(gauge_fw_version == firmwareVersion)// 固件版本一致，则只更新数据文件
        {
            printk( "%s() line:%d, firmware already be the latest, ONLY update data!\n", __func__,__LINE__);
        }
        else if(gauge_fw_version > firmwareVersion)//如果器件固件版本号比系统中固件文件的版本号更新，则直接返回
        {
            setbit(1);
            printk( "%s() line:%d, device fwv newer than file fwv, just return\n", __func__,__LINE__);
            goto set_fs_exit;
        }
        else// 固件比文件旧，全部更新
        {
            printk( "%s() line:%d, device fwv older than file fwv, update firmware and data.\n", __func__,__LINE__);
            kfree(fw_buf);//free data file buf
            fw_buf=0;
            if(gauge_readfile(BQFS,&fw_buf,&length)==-1)//read failed
            {
                goto set_fs_exit;// 读文件都失败，下载肯定无法进行，直接返回
            }
            walkp=fw_buf;
            walkp=strstr(fw_buf,"FWV:");
            walkp+=4;
            firmwareVersion=(unsigned int)simple_strtol(walkp, &walkp, 16);
        }
#else
        if(gauge_fw_version == firmwareVersion)// 固件版本一致，则只更新数据文件
        {
            printk( "%s() line:%d, firmware already be the latest, ONLY update data!\n", __func__,__LINE__);
        }
        else// 固件不一致，全部更新
        {
            printk( "%s() line:%d, device fwv older than file fwv, update firmware and data.\n", __func__,__LINE__);
            kfree(fw_buf);//free data file buf
            fw_buf=0;
            if(gauge_readfile(BQFS,&fw_buf,&length)==-1)//read failed
            {
                goto set_fs_exit;// 读文件都失败，下载肯定无法进行，直接返回
            }
            walkp=fw_buf;
            walkp=strstr(fw_buf,"FWV:");
            walkp+=4;
            firmwareVersion=(unsigned int)simple_strtol(walkp, &walkp, 16);
        }
#endif

    }
    else
    {
        if(gauge_readfile(BQFS,&fw_buf,&length)==-1)//read failed
        {
            printk( "%s() line:%d,read data file failed\n", __func__,__LINE__);
            goto set_fs_exit;// 读文件都失败，下载肯定无法进行，直接返回
        }
        walkp=fw_buf;
        walkp=strstr(fw_buf,"FWV:");
        walkp+=4;
        firmwareVersion=(unsigned int)simple_strtol(walkp, &walkp, 16);
    }
    setbit(0);
    wake_lock_init(&gauge_up_wake_lock, WAKE_LOCK_SUSPEND, "ti-fuel-gauge-update");
    wake_lock(&gauge_up_wake_lock);
#ifndef GAUGE_UPDATE_TEST_PARSE

    if(gaugemode==0)
    {
        if(!gauge_enter_update_mode())
        {
            printk( "%s:  Enter ROM Mode %s,l:%d \n", __func__,"failed",__LINE__);
            goto proc_write_val_exit;
        }
    }
#endif

    rc=gauge_parse_execute(walkp,length);
    if(rc==0)
    {
        printk("download firmware success!before exit ROM mode\n");
    }
    else
    {
        printk("download firmware failed!%d line is executed\n",rc);
    }

#ifndef GAUGE_UPDATE_TEST_PARSE
#if 1//似乎烧写完了后就已经在正常模式了

    if(gauge_exit_update_mode())
    {
        printk( "%s:  exit ROM Mode %s,l:%d \n", __func__,"failed",__LINE__);
        mdelay(10);
        gauge_enabel_it(1);
        goto proc_write_val_exit;
    }
    mdelay(10);
    gauge_enabel_it(1);
    printk( "%s:  exit ROM Mode success,l:%d \n", __func__,__LINE__);
#endif
proc_write_val_exit:
#endif

    wake_unlock(&gauge_up_wake_lock);
    wake_lock_destroy(&gauge_up_wake_lock);
    clearbit(0);

    //kmalloc_exit:
    kfree(fw_buf);
set_fs_exit:
    set_fs(oldfs);
    setbit(2);
}


static int
gauge_proc_read_val(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    if(ti_gauge_td==0)
    {
        printk("%s():%d,ti_gauge_td==0\n", __func__,__LINE__);
        return 0;
    }
#if 1//YINTIANCI_GAUGE_20101016

    if(testbit(0))
    {
        len += sprintf(page + len, "uppercent:%d\n", update_percent);
    }
    else if(testbit(2))
    {
        printk( "%s():update_success!\n", __func__);
        len += sprintf(page + len, "update_success!\n");

        clearbit(2);
    }
    else if(testbit(1))
    {
        len += sprintf(page + len, "already be the latest firm ware,update abort!\n");
        clearbit(1);

    }
    else
    {
        if(gauge_cntlsub_read( CNTLSUB_CONTROL_STATUS,&gauge_cntl_status))
        {//failed
            gauge_cntl_status=0;
        }
        if(gauge_cntlsub_read( CNTLSUB_FW_VERSION,&gauge_fw_version))
        {//failed
            gauge_fw_version=0;
        }
        if(gauge_cntlsub_read( CNTLSUB_HW_VERSION,&gauge_hw_version))
        {//failed
            gauge_hw_version=0;
        }
        len += sprintf(page + len, "name: %s\n", "ti_fuel_gauge");
        len += sprintf(page + len, "i2c address: 0x%x\n", ti_gauge_td->client->addr);
        len += sprintf(page + len, "hardware version: 0x%02x\n", gauge_hw_version);
        len += sprintf(page + len, "firmware version: 0x%02x\n", gauge_fw_version);
        len += sprintf(page + len, "control status : 0x%02x\n", gauge_cntl_status);
    }

#else
    switch(gauge_update_state)
    {
    case 1://during updating
        len += sprintf(page + len, "uppercent:%d\n", update_percent);
        break;
    case 2:// open bqfs file failed;
        len += sprintf(page + len, "already be the latest firm ware,update abort!\n");
        gauge_update_state=0;
        break;
    case 3:// update success
        printk( "%s():update_success!\n", __func__);
        len += sprintf(page + len, "update_success!\n");
        gauge_update_state=0;
        break;
    case 0:
    default:
        if(gauge_cntlsub_read( CNTLSUB_CONTROL_STATUS,&gauge_cntl_status))
        {//failed
            gauge_cntl_status=0;
        }
        if(gauge_cntlsub_read( CNTLSUB_FW_VERSION,&gauge_fw_version))
        {//failed
            gauge_fw_version=0;
        }
        if(gauge_cntlsub_read( CNTLSUB_HW_VERSION,&gauge_hw_version))
        {//failed
            gauge_hw_version=0;
        }
        len += sprintf(page + len, "name: %s\n", "ti_fuel_gauge");
        len += sprintf(page + len, "i2c address: 0x%x\n", ti_gauge_td->client->addr);
        len += sprintf(page + len, "hardware version: 0x%02x\n", gauge_hw_version);
        len += sprintf(page + len, "firmware version: 0x%02x\n", gauge_fw_version);
        len += sprintf(page + len, "control status : 0x%02x\n", gauge_cntl_status);
        break;
    }
#endif
    //printk( "%s():%d,exit,rc str:%s\n", __func__,__LINE__,page);
    if (off + count >= len)
        *eof = 1;
    if (len < off)
        return 0;
    *start = page + off;
    return ((count < len - off) ? count : len - off);
}



static int gauge_proc_write_val(struct file *file, const char *buffer,
                                unsigned long count, void *data)
{
    //char *tgt_fw = "/system/etc/ti_gauge.bqfs";
    /*下载过程中i2c时钟要求低于100k，这个我们目前就是设置在这个频率上的
    可以从board_v9.c的msm_i2c_pdata定义时的.clk_freq = 100000看出来，同时可以从
    i2c-msm.c的prob函数中的 printk(KERN_INFO "msm_i2c_probe: clk_ctl %x, %d Hz\n",打印来印证。
    因此我们不需要专门进行配置来设置时钟。如果一定要设置就参考i2c-msm.c
    的方法来配置I2C_CLK_CTL寄存器。
    */

    printk( "%s():%d, enter,in buf:%s\n", __func__,__LINE__,buffer);

    if(ti_gauge_td==0)
    {
        printk("%s():%d,ti_gauge_td==0\n", __func__,__LINE__);
        return count;
    }
    if(strstr(buffer,"smartup"))//only echo up to this interface can trigger updating
    {
        smartupdate=1;
    }
    else if(strstr(buffer,"up"))
    {
        smartupdate=0;
    }
    else
    {
        printk( "%s(): cmd:%s not match \"up\" or \"smartup\"!\n", __func__,buffer);
        return count;
    }


    if(ti_gauge_td->ti_gauge_up_wq==NULL)
    {
        ti_gauge_td->ti_gauge_up_wq = create_singlethread_workqueue("ti_gauge_update");
        INIT_WORK(&ti_gauge_td->work, TI_gauge_update_work_func);
    }
    if(!testbit(0))//防止连续echo up来更新
    {
        queue_work(ti_gauge_td->ti_gauge_up_wq, &ti_gauge_td->work);//start update
    }
    return count;
}

#endif

/*
ti gauge初始化，设置初始的门限值，设置中断处理函数
*/
static int TI_gauge_probe(
    struct i2c_client *client, const struct i2c_device_id *id)
{
    struct TI_gauge_data *pgauge_data;
    //u8 buf[9];
    int ret = 0;
#ifdef GAUGE_UPDATE_FIRMWARE//update relative

    struct proc_dir_entry *dir,*refresh;//
#endif

    printk( "%s:  enter \n", __func__);

    //msleep(250);
    if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
    {
        printk( "%s:  i2c_check_functionality failed\n",__func__);
        ret = -ENODEV;
        goto err_check_functionality_failed;
    }
    pgauge_data = kzalloc(sizeof(*pgauge_data), GFP_KERNEL);
    if (pgauge_data == NULL)
    {
        ret = -ENOMEM;
        goto err_alloc_data_failed;
    }

    pgauge_data->client = client;
    i2c_set_clientdata(client, pgauge_data);
    client->driver = &TI_gauge_driver;

    ti_gauge_td = pgauge_data;
    mutex_init(&g_mutex);

    if(gauge_cntlsub_read( CNTLSUB_CONTROL_STATUS,&gauge_cntl_status))
    {//failed
        gauge_cntl_status=0;
    }
    if(gauge_cntlsub_read( CNTLSUB_FW_VERSION,&gauge_fw_version))
    {//failed
        gauge_fw_version=0;
    }
    if(gauge_cntlsub_read( CNTLSUB_HW_VERSION,&gauge_hw_version))
    {//failed
        gauge_hw_version=0;
    }
    printk( "%s: cntl:0x%02x, fw:0x%02x, hw:0x%02x,\n",__func__,gauge_cntl_status,gauge_fw_version,gauge_hw_version);

#if 0
    //ret = request_irq(client->irq, TI_gauge_irq_handler, IRQF_TRIGGER_HIGH, "TI_gauge_driver", pgauge_data);
    //if(ret == 0)
    if(1)
    {
        pgauge_data->use_irq = 1;
        buf[0]=REGADDR_CNTL;
        buf[1]=CNTLSUB_SET_HIBERNATE&0x00ff;
        buf[2]=CNTLSUB_SET_HIBERNATE&0xff00;
        ret = gauge_i2c_write(pgauge_data->client, buf,3);  /* enable hibernate mode*/
        printk( "%s:  enable hibernate mode %s \n", __func__,ret?"failed":"ok");
    }
    else
    {
        dev_err(&client->dev, "request_irq failed\n");
    }
#endif
#ifdef GAUGE_UPDATE_FIRMWARE//update relative
    dir = proc_mkdir("ti-fuel-gauge", NULL);
    refresh = create_proc_entry("gauge_interface", 0777, dir);
    if (refresh)
    {
        refresh->data		= NULL;
        refresh->read_proc  = gauge_proc_read_val;
        refresh->write_proc = gauge_proc_write_val;
    }
#endif

    printk( "%s:  exit ok \n", __func__);
    return 0;

    //err_detect_failed:
    //err_power_failed:
    kfree(pgauge_data);
err_alloc_data_failed:
err_check_functionality_failed:
    printk( "%s: exit failed \n", __func__);
    return ret;
}

static int TI_gauge_remove(struct i2c_client *client)
{
    struct TI_gauge_data *pgauge_data = i2c_get_clientdata(client);

    //free_irq(client->irq, pgauge_data);

    kfree(pgauge_data);
    return 0;
}

void  TI_gauge_shutdown(struct i2c_client * pclient)
{
    unsigned int regValue=0;
    printk( "%s:enter\n", __func__);
    //if(!gauge_cntlsub_read( CNTLSUB_SET_HIBERNATE,&regValue))
    if(!gauge_cntlsub_read( CNTLSUB_SET_SLEEPp,&regValue))		//chenchongbao.2011.6.7
    {
        if(!gauge_cntlsub_read( CNTLSUB_CONTROL_STATUS,&regValue))
        {
            printk("%s:set hibernate %s\n ", __func__,(regValue&(1<<6))?"ok":"failed");
        }
        else
        {
            printk("%s:set hibernate failed\n", __func__);
        }
    }
}

static const struct i2c_device_id TI_gauge_id[]=
    {
        { "ti-fuel-gauge",0
        },
        {},
    };

static struct i2c_driver TI_gauge_driver =
    {
        .probe		= TI_gauge_probe,
                  .remove      = TI_gauge_remove,
                                 .shutdown =TI_gauge_shutdown,
                                            .id_table    = TI_gauge_id,
                                                           .driver      = {
                                                                              .name   = "ti-fuel-gauge",
                                                                              .owner  = THIS_MODULE,
                                                                          },
                                                                      };


static int __init ti_gauge_init(void)
{
    int rc;
    printk( "%s:  enter \n", __func__);

    rc=i2c_add_driver(&TI_gauge_driver);
    if (rc < 0)
    {
        printk( "%s: add TI_gauge_driver Failed  rc=%d\n", __func__, rc);
        return rc;
    }

    rc   = driver_create_file(&TI_gauge_driver.driver, &driver_attr_Control);
    rc |= driver_create_file(&TI_gauge_driver.driver, &driver_attr_batTemperature);
    rc |= driver_create_file(&TI_gauge_driver.driver, &driver_attr_batVoltage);
    rc |= driver_create_file(&TI_gauge_driver.driver, &driver_attr_Flags);
    rc |= driver_create_file(&TI_gauge_driver.driver, &driver_attr_batNominalAvailableCapacity);
    rc |= driver_create_file(&TI_gauge_driver.driver, &driver_attr_batFullAvailableCapacity);
    rc |= driver_create_file(&TI_gauge_driver.driver, &driver_attr_batRemainingCapacity);
    rc |= driver_create_file(&TI_gauge_driver.driver, &driver_attr_batFullChargeCapacity);
    rc |= driver_create_file(&TI_gauge_driver.driver, &driver_attr_batCurrent);
    rc |= driver_create_file(&TI_gauge_driver.driver, &driver_attr_batTimeToEmpty);
    rc |= driver_create_file(&TI_gauge_driver.driver, &driver_attr_batMaxLoadCurrent);
    rc |= driver_create_file(&TI_gauge_driver.driver, &driver_attr_batMaxLoadTimeToEmpty);
    rc |= driver_create_file(&TI_gauge_driver.driver, &driver_attr_batCycleCount);
    rc |= driver_create_file(&TI_gauge_driver.driver, &driver_attr_batStateOfCharge);
    rc |= driver_create_file(&TI_gauge_driver.driver, &driver_attr_upmode);
    rc |= driver_create_file(&TI_gauge_driver.driver, &driver_attr_enable);
    if(rc )
    {
        i2c_del_driver(&TI_gauge_driver);
        return rc;
    }

    gauge_update_state=0;
    gauge_initialized = 1;

    printk( "%s:  exit ,enable=%d\n", __func__,gauge_enable);
    return 0;
}

static void __exit ti_gauge_exit(void)
{
    if(gauge_initialized)
        i2c_del_driver(&TI_gauge_driver);
    gauge_initialized = 0;

}

module_init(ti_gauge_init);
module_exit(ti_gauge_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("yintianci, ZTE, Inc.");
MODULE_DESCRIPTION("gauge driver for ti chipsets.");
MODULE_VERSION("1.0");
MODULE_ALIAS("platform:ti-fuel-gauge");

#endif

