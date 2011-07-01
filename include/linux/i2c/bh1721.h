/******************************************************************/
/*head file for bh1721.c*/


/*******************************************************************
Geschichte:
Wenn               Wer          Was                               Tag
2010-08-13    wangzy     create    


******************************************************************/
// ioctl numbers
#define ALS_IOCTL_MAGIC        	0XCF
#define ALS_IOCTL_PW_ON       	_IO(ALS_IOCTL_MAGIC, 1)
#define ALS_IOCTL_PW_OFF      	_IO(ALS_IOCTL_MAGIC, 2)
#define ALS_IOCTL_DATA     	_IOR(ALS_IOCTL_MAGIC, 3, short)
#define ALS_IOCTL_CALIBRATE	_IO(ALS_IOCTL_MAGIC, 4)
#define ALS_IOCTL_CONFIG_GET   	_IOR(ALS_IOCTL_MAGIC, 5, struct als_cfg)
#define ALS_IOCTL_CONFIG_SET		_IOW(ALS_IOCTL_MAGIC, 6, struct als_cfg)


#define u32 unsigned int 
#define u16 unsigned short  
#define u8   unsigned char 

//device configuration
struct als_cfg {
	u32	calibrate_target;
 	//u8	filter_history;
	//u8	filter_count;
	u8	gain;
};


