/*
 *    (C) Copyright  2009, ZTE Corporation
*/

#ifndef __SI4708_H__
#define __SI4708_H__



/******************************  MACROS DEFINE******************************/
#define      FM_RESET_GPIO					3
#define      FM_I2C_SDA_GPIO				93

#define      SI4708_RESET_DELAY				110
#define      SI4708_POWERUP_DELAY			110 
#define      SI4708_POWERDOWN_DELAY         110 
#define      SI4708_SEEK_DELAY				600 
#define      SI4708_TUNE_DELAY				200
/****************************INTERNAL VARIBLE******************************/

#define      SEEKUP  			1		/* seek up*/
#define      SEEKDOWN 		0		/* seek down*/

#define      FM_TRACK_STEREO 0
#define      FM_TRACK_MONO   1

#define      FM_BAND_GENERIC   0
#define      FM_BAND_JAPAN_WIDE    1
#define      FM_BAND_JAPAN           2
#define      FM_BAND_RESERVED     3

#define      FM_SPACE_200K    0
#define      FM_SPACE_100K    1
#define      FM_SPACE_50K      2


#define REG_VOL_MASK  0x0F
#define REG_BAND_MASK  0xC0
#define REG_SPACE_MASK 0x50
#define REG_TRACK_MASK 0x20
#define REG_STC_MASK  0x40
#define REG_SF_MASK  0x20
#define REG_ST_MASK   0x01
#define REG_TUNE_MASK 0x80
#define REG_SEEK_MASK 0x01
#define REG_SEEKUP_MASK 0x02
#define REG_SKMODE_MASK 0x04

#define FM_VOL_MUTE    0/* volume min.= 0*/
#define FM_VOL_MAX      15/* volume max.=15*/


#define FM_REG_STATUSRSSI 0
#define FM_REG_READCHAN     2

#define FM_REG_DEVICEID 12
#define FM_REG_CHIPID     14
#define FM_REG_POWERCFG 16
#define FM_REG_CHANNEL 18
#define FM_REG_SYSCONFIG1 20
#define FM_REG_SYSCONFIG2 22
#define FM_REG_SYSCONFIG3 24

#define FM_REG_WR_START   FM_REG_POWERCFG
#define FM_REG_RD_START   FM_REG_STATUSRSSI

/******************************************************************************/
/* Magic # */
#define FM_IOC_MAGIC  'k'

/* Interfaces */

#define FM_INIT2NORMAL                 _IO(FM_IOC_MAGIC, 1)/*for test*/
#define FM_NORMAL2STANDBY         _IO(FM_IOC_MAGIC, 2)/*for test*/
#define FM_STANDBY2NORMAL         _IO(FM_IOC_MAGIC, 3)/*for test*/

#define FM_TUNE                              _IOW(FM_IOC_MAGIC, 4, int)
#define FM_SEEK        		                _IOW(FM_IOC_MAGIC, 5, int[2])
//#define FM_AUTOSEEK   		 	   _IOR(FM_IOC_MAGIC, 6)
#define FM_GET_VOL    		  	   _IOR(FM_IOC_MAGIC, 7, int)
#define FM_SET_VOL    		          _IOW(FM_IOC_MAGIC, 8,int)
//#define FM_VOL_UP      		          _IO(FM_IOC_MAGIC, 9)
//#define FM_VOL_DOWN  		  	    _IO(FM_IOC_MAGIC, 10)
/* Reserved interface*/
#define FM_GET_BAND   		  		_IOR(FM_IOC_MAGIC, 11, int)
#define FM_SET_BAND     	   		       _IOW(FM_IOC_MAGIC, 12, int)
#define FM_GET_SPACE    	   		       _IOR(FM_IOC_MAGIC, 13, int)
#define FM_SET_SPACE    	  		       _IOW(FM_IOC_MAGIC, 14, int)
#define FM_GET_AUDIOTRACK    	       _IOR(FM_IOC_MAGIC, 15, int)
#define FM_SET_AUDIOTRACK    	       _IOW(FM_IOC_MAGIC, 16, int)
#define FM_GET_CURRENTFREQ		_IOR(FM_IOC_MAGIC, 17, int)
/*for test*/
#define FM_DISPLAY_REG    			 _IO(FM_IOC_MAGIC, 18)      

/******************************************************************************/
#endif /* __SI4708_H__ */

