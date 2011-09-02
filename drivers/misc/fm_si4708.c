/* drivers/misc/fm_si4708.c
 *
 * Copyright (C) 2009 ZTE, corporation.
 * Author:	zhu.yufei@zte.com.cn
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*histstory:
 when               who            what, where, why                                comment tag
 --------        ----          ----------------------            -------------------------------
2010-5-17 chenjun  fix CRDB00480912                         ZTE_Audio_CJ_100511

 2009-10-12    feng.yuao     		add si4708 driver                    ZTE_SI4708_F.YUAO_001

 2009-11-03    feng.yuao     	add/remove suspend and resume function           ZTE_SI4708_F.YUAO_002

 2009-11-04    feng.yuao     	modify si4708 suspend and resume            ZTE_SI4708_F.YUAO_003
 								interrelated code

 2009-11-06    feng.yuao          modify for getting frequency            ZTE_SI4708_F.YUAO_004

 2009-12-05    feng.yuao   to release free memory when errors happened yet     ZTE_SI4708_F.YUAO_005
*/

/******************************  INCLUDE FILES******************************/
#include <linux/module.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <asm/io.h>
/*	ZTE_SI4708_F.YUAO_002 2009-11-03	*/
#include <linux/earlysuspend.h>
/*end, ZTE_SI4708_F.YUAO_002 2009-11-03	*/
#include <asm/system.h>
#include <asm/mach-types.h>
#include <asm/uaccess.h>
#include <asm/gpio.h>
#include <mach/fm_si4708.h>

/**************************************************************************/

#define   	FM_DBG(x...)   	printk(KERN_DEBUG"[FM]"x)
#define   	FM_INFO(x...)   	printk(KERN_INFO"[FM]"x)
#define 		FYA_TAG        "[FYA@FM_SI4708]"

struct fm_dev {
	int   initialized;   			/* device opend times */
    	char   band;				/* cached band */		
    	char   track;				/* cached track */
    	char   status_stereo;		/* cached stereo status */
	char  space;				/* cached space */
   	int    frequency; 			/* cached frequency */
    	int    channel;			/* cached channel */
	char  seek_direction;		/* cached seek direction */
    	char  vol;         			/* cached vol */          	  		      											                	 						                                   											                	  		
    	char regs[32];             		/* buffer for reading */	 		 			
	struct i2c_client *client;
	spinlock_t lock;
};

static struct fm_dev *fm_si4708_dev;

static int si4708_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int si4708_remove(struct i2c_client *client);

/**********************************************************************
 * Function:    si4708_regs_read(struct i2c_client * client, char * buf, int count)
 * Description: This function read buff by i2c bus
 * Input:        client: the I2C client to be operated,
 		       buf: save regs to native
 		       count: how many registers could be readed
 * Output:      none
 * Return:      0 = success, negative = failure
 * Others:      
 * Modify Date    Version    Author        Modification
 * ----------------------------------------------------
 * 2009/11/11       1.0      fengyuao        modify
 **********************************************************************/
static int si4708_regs_read(struct i2c_client * client, char * buf, int count)
{
	int	ret = 0;

	if (count != i2c_master_recv(client, buf, count)) 
		ret = -EIO;

	return ret;
}


/**********************************************************************
 * Function:    si4708_regs_write(struct i2c_client * client, char * buf, int count)
 * Description: This function write buff to register by i2c bus
 * Input:        client: the I2C client to be operated,
 		       buf: save regs to native
 		       count: how many registers could be readed
 * Output:      none
 * Return:      0 = success, negative = failure
 * Others:      
 * Modify Date    Version    Author        Modification
 * ----------------------------------------------------
 * 2009/11/11       1.0      fengyuao        modify
 **********************************************************************/
static int si4708_regs_write(struct i2c_client * client,const char * buf, int count)
{
	int	ret = 0;

	if (count != i2c_master_send(client, buf, count)) 
		ret = -EIO;

	return ret;
}


/**********************************************************************/

#define int_to_char(m)  ((char)(m&0xFF))

/**********************************************************************
 * Function:    si4708_WR(char *in_data, int num, int isInit)
 * Description: This function write or read register,when isInt = 1,delay some time for writing
 * Input:        in_data: read/write buffer,
 		       num: how many registers could be readed or writed
 		       isInt: 0 = read,	1 = write	
 * Output:      none
 * Return:      0 = success, negative = failure
 * Others:      
 * Modify Date    Version    Author        Modification
 * ----------------------------------------------------
 * 2009/11/11       1.0      fengyuao        modify
 **********************************************************************/
static int si4708_WR(char *in_data, int num, int isInit)
{
	int res;

	res = si4708_regs_write(fm_si4708_dev->client, in_data, num);
   	if (res < 0)
		fm_si4708_dev->initialized = 0;
	 
   	/* from powerdown to powerup*/
	if(isInit)
		mdelay(SI4708_POWERUP_DELAY);
	
	return res;
}


/**********************************************************************/
/*three modes :	init,	wakeup and standby*/
static char fm_init[] = {0x40,0x01,0x00,0x00,0xC0,0x04,0x0A,0x10};
static char fm_wakeup[] = {0x40,0x01};
static char fm_standby[] = {0x00,0x41};

#define si4708_init2normal(void)	si4708_WR(fm_init,sizeof(fm_init),1)
#define si4708_standby2normal(void)	si4708_WR(fm_wakeup,sizeof(fm_wakeup),0)
#define si4708_normal2standby(void)	si4708_WR(fm_standby,sizeof(fm_standby),0)

/**********************************************************************
 * Function:    si4708_read_all_regs(void)
 * Description: This function read all registers
 * Input:        none
 * Output:      none
 * Return:      0 = success, negative = failure
 * Others:      
 * Modify Date    Version    Author        Modification
 * ----------------------------------------------------
 * 2009/11/11       1.0      fengyuao        modify
 **********************************************************************/
static int si4708_read_all_regs(void)
{
	int res = 0;
	
    	res = si4708_regs_read(fm_si4708_dev->client, &fm_si4708_dev->regs[FM_REG_RD_START], sizeof(fm_si4708_dev->regs));
    	if (res < 0)
		fm_si4708_dev->initialized = 0;

	return res;
}


/**********************************************************************
 * Function:    si4708_get_vol(void)
 * Description: This function  gets volume from register
 * Input:        none
 * Output:      volume value
 * Return:      volume value
 * Others:      
 * Modify Date    Version    Author        Modification
 * ----------------------------------------------------
 * 2009/11/11       1.0      fengyuao        modify
 **********************************************************************/
static int si4708_get_vol(void)
{
	fm_si4708_dev->vol = fm_si4708_dev->regs[FM_REG_SYSCONFIG2+1] & 0X0F;
	return fm_si4708_dev->vol;
}


/**********************************************************************
 * Function:    si4708_set_vol(char vol)
 * Description: This function  sets volume to register
 * Input:         volume value
 * Output:       none
 * Return:       0 = success, negative = failure
 * Others:      
 * Modify Date    Version    Author        Modification
 * ----------------------------------------------------
 * 2009/11/11       1.0      fengyuao        modify
 **********************************************************************/
static int si4708_set_vol(char vol)
{
	int res;
	int vol_value;

       fm_si4708_dev->vol = vol & 0xF;
       vol_value = (int)fm_si4708_dev->vol ;
	   
    	if(vol_value < FM_VOL_MUTE)
		fm_si4708_dev->vol = int_to_char(FM_VOL_MUTE);
		
	if(vol_value > FM_VOL_MAX) 
   	       fm_si4708_dev->vol = int_to_char(FM_VOL_MAX);
		
    	fm_si4708_dev->regs[FM_REG_SYSCONFIG2+1] &= ~REG_VOL_MASK; 
	fm_si4708_dev->regs[FM_REG_SYSCONFIG2+1] |=  fm_si4708_dev->vol; 
      
	/*write  vol  to register*/
    	res = si4708_regs_write(fm_si4708_dev->client, &fm_si4708_dev->regs[FM_REG_WR_START], 8);
	if(res < 0)
		fm_si4708_dev->initialized = 0;

    	return res;
}


/**********************************************************************
 * Function:    si4708_get_vol(void)
 * Description: This function  gets band from register
 * Input:        none
 * Output:      band value
 * Return:      band value
 * Others:      
 * Modify Date    Version    Author        Modification
 * ----------------------------------------------------
 * 2009/11/11       1.0      fengyuao        modify
 **********************************************************************/
static int si4708_get_band(void)
{  
	fm_si4708_dev->band = (fm_si4708_dev->regs[FM_REG_SYSCONFIG2+1] & 0XC0)>>6;
	return fm_si4708_dev->band;
}


/**********************************************************************
 * Function:    si4708_set_vol(char vol)
 * Description: This function  sets band to register
 * Input:         band value
 * Output:       none
 * Return:       0 = success, negative = failure
 * Others:      
 * Modify Date    Version    Author        Modification
 * ----------------------------------------------------
 * 2009/11/11       1.0      fengyuao        modify
 **********************************************************************/
static int si4708_set_band(char band)
{
	int res = 0;
	int band_value;
	
       fm_si4708_dev->band = band & 0x3;
       band_value =(int)fm_si4708_dev->band;
	   
    	if(band_value < FM_BAND_GENERIC )
		fm_si4708_dev->band = FM_BAND_GENERIC;
		
	if(band_value > FM_BAND_JAPAN) 
   	       fm_si4708_dev->band = FM_BAND_JAPAN;
	   
	fm_si4708_dev->regs[FM_REG_SYSCONFIG2+1] &= ~REG_BAND_MASK; 
	fm_si4708_dev->regs[FM_REG_SYSCONFIG2+1] |= fm_si4708_dev->band <<6; 

	
	/*write  band  to register*/
    	res = si4708_regs_write(fm_si4708_dev->client, &fm_si4708_dev->regs[FM_REG_WR_START], 8);
	if(res < 0)
		fm_si4708_dev->initialized = 0;
		
    	return res;
}


/**********************************************************************
 * Function:    si4708_get_space(void)
 * Description: This function  gets space from register
 * Input:        none
 * Output:      space value
 * Return:      space value
 * Others:      
 * Modify Date    Version    Author        Modification
 * ----------------------------------------------------
 * 2009/11/11       1.0      fengyuao        modify
 **********************************************************************/
static int si4708_get_space(void)
{
	fm_si4708_dev->space = (fm_si4708_dev->regs[FM_REG_SYSCONFIG2+1] & 0X30)>>4;
	return fm_si4708_dev->space;
}


/**********************************************************************
 * Function:    si4708_set_space(char vol)
 * Description: This function  sets space to register
 * Input:         space value
 * Output:       none
 * Return:       0 = success, negative = failure
 * Others:      
 * Modify Date    Version    Author        Modification
 * ----------------------------------------------------
 * 2009/11/11       1.0      fengyuao        modify
 **********************************************************************/
static int si4708_set_space(char space)
{
	int res = 0;
	int space_value;
	
       fm_si4708_dev->space = space & 0x3;
	space_value = fm_si4708_dev->space;
	   
    	if(space_value < FM_SPACE_200K )
		fm_si4708_dev->space = FM_SPACE_200K;
		
	if(space_value > FM_SPACE_50K) 
   	       fm_si4708_dev->space = FM_SPACE_50K;
	   
	fm_si4708_dev->regs[FM_REG_SYSCONFIG2+1] &= ~REG_SPACE_MASK; 
	fm_si4708_dev->regs[FM_REG_SYSCONFIG2+1] |= fm_si4708_dev->space <<4; 
	
	/*write  space  to register*/
    	res = si4708_regs_write(fm_si4708_dev->client, &fm_si4708_dev->regs[FM_REG_WR_START], 8);
	if(res < 0)
		fm_si4708_dev->initialized = 0;
		
    	return res;
}


/**********************************************************************
 * Function:    si4708_get_track(void)
 * Description: This function  gets track status from register
 * Input:        none
 * Output:      track value
 * Return:      track value
 * Others:      
 * Modify Date    Version    Author        Modification
 * ----------------------------------------------------
 * 2009/11/11       1.0      fengyuao        modify
 **********************************************************************/
static int si4708_get_track(void) 
{ 
 	fm_si4708_dev->track = fm_si4708_dev->regs[FM_REG_POWERCFG] & 0X20;
	return fm_si4708_dev->track;
}


/**********************************************************************
 * Function:    si4708_set_track(char track)
 * Description: This function  sets track status to register
 * Input:         track value
 * Output:       none
 * Return:       0 = success, negative = failure
 * Others:      
 * Modify Date    Version    Author        Modification
 * ----------------------------------------------------
 * 2009/11/11       1.0      fengyuao        modify
 **********************************************************************/
static int si4708_set_track(char track)
{
	int res = 0;

       fm_si4708_dev->track = track & 0x1;
	   
	fm_si4708_dev->regs[FM_REG_POWERCFG] &= ~REG_TRACK_MASK; 
	fm_si4708_dev->regs[FM_REG_POWERCFG] |= fm_si4708_dev->track <<5; 
	
	/*write  vol  to register*/
    	res = si4708_regs_write(fm_si4708_dev->client, &fm_si4708_dev->regs[FM_REG_WR_START], 1);
	if(res < 0)
			fm_si4708_dev->initialized = 0;
		
  return res;
}


/**********************************************************************
 * Function:    channel2freq(int channel)
 * Description: This function change channel value to frequency
 * Input:         channel value
 * Output:       frequency
 * Return:       frequency
 * Others:      
 * Modify Date    Version    Author        Modification
 * ----------------------------------------------------
 * 2009/11/11       1.0      fengyuao        modify
 **********************************************************************/
static int channel2freq(int channel)
{
	int	freq = 0;
	int	space;
	int	space_status = si4708_get_space();

	switch(space_status){
		case FM_SPACE_200K:
			space = 20;
			break;
		case FM_SPACE_100K:
			space = 10;
			break;
		case FM_SPACE_50K:
			space = 5;
			break;
		default:
		       space = 10;
	}

	if(fm_si4708_dev->band == FM_BAND_GENERIC)
		freq = space * channel + 8750;/* US &  EUROPE 87.50MHZ */
	else
		freq = space * channel + 7600;/* JAPAN 76.00MHZ */

	return freq;
	
}


/**********************************************************************
 * Function:    freq2channel(int frequency)
 * Description: This function change  frequency to channel value
 * Input:         frequency
 * Output:       channel value
 * Return:       channel value
 * Others:      
 * Modify Date    Version    Author        Modification
 * ----------------------------------------------------
 * 2009/11/11       1.0      fengyuao        modify
 **********************************************************************/
static int freq2channel(int frequency)
{
	int   space;
	int   channel;
	int 	space_status = si4708_get_space();

	switch(space_status){
		case FM_SPACE_200K:
			space = 20;
			break;
		case FM_SPACE_100K:
			space = 10;
			break;
		case FM_SPACE_50K:
			space = 5;
			break;
		default:
		       space = 10;
	}
	
	if(fm_si4708_dev->band == FM_BAND_GENERIC)
		channel = (frequency - 8750)/space;
	else  /*JAPAN BAND*/
		channel = (frequency -7600)/space;
	
       return channel;
}


/**********************************************************************
 * Function:    si4708_get_frequency(void)
 * Description: This function  gets current frequency  from register
 * Input:        none
 * Output:      current frequency
 * Return:      current frequency
 * Others:      
 * Modify Date    Version    Author        Modification
 * ----------------------------------------------------
 * 2009/11/11       1.0      fengyuao        modify
 **********************************************************************/
/*ZTE_SI4708_F.YUAO_004 2009-11-06*/	
static int si4708_get_frequency(void)
{
	int res = 0;
	int high,low,channel;

	res = si4708_regs_read(fm_si4708_dev->client, 
			&fm_si4708_dev->regs[FM_REG_RD_START], 4); 
	if(res <0)
	{
		fm_si4708_dev->initialized = 0;
    		return res;
	}

	high = ((int)(fm_si4708_dev->regs[FM_REG_READCHAN] & 0x03))<<8;
	low = (int)fm_si4708_dev->regs[FM_REG_READCHAN+1];

	channel = high + low;

	return channel2freq(channel);

}
/*end, ZTE_SI4708_F.YUAO_004 2009-11-06*/


/**********************************************************************
 * Function:    si4708_set_frequency(int frequency)
 * Description: This function  gets current frequency  from register
 * Input:        frequency
 * Output:      none
 * Return:      0 = success, negative = failure
 * Others:      
 * Modify Date    Version    Author        Modification
 * ----------------------------------------------------
 * 2009/11/11       1.0      fengyuao        modify
 **********************************************************************/
static int si4708_set_frequency(int frequency)
{
  	int res = 0;

    	/* Check STC bit to see whether the device is busy now. */
  	res = si4708_regs_read(fm_si4708_dev->client, &fm_si4708_dev->regs[FM_REG_RD_START], 4); 
	if(res <0)
	{
		fm_si4708_dev->initialized = 0;
    		return res;
	}
	/* do not need to check it ,because  each S/T operation  will clear STC*/
	if( (fm_si4708_dev->regs[FM_REG_STATUSRSSI] & REG_STC_MASK) == REG_STC_MASK)
		return -EBUSY;
	
	/*write frequency to register*/   
       fm_si4708_dev->frequency = frequency;
	if(fm_si4708_dev->band == FM_BAND_GENERIC)
	{
		if(frequency > 10800)
			fm_si4708_dev->frequency = 10800;
		if(frequency < 8750)
			fm_si4708_dev->frequency = 8750;
	}
	else if(fm_si4708_dev->band == FM_BAND_JAPAN_WIDE)
	{
		if(frequency > 10800)
			fm_si4708_dev->frequency = 10800;
		if(frequency < 7600)
			fm_si4708_dev->frequency = 7600;	
	}
	else
	{
		if(frequency > 9000)
			fm_si4708_dev->frequency = 9000;
		if(frequency < 7600)
			fm_si4708_dev->frequency = 7600;	
	}
	
       fm_si4708_dev->channel = freq2channel(frequency);
	
    	fm_si4708_dev->regs[FM_REG_CHANNEL+1] = int_to_char(fm_si4708_dev->channel);
	fm_si4708_dev->regs[FM_REG_CHANNEL] = REG_TUNE_MASK|int_to_char(fm_si4708_dev->channel >>8);

    	res = si4708_regs_write(fm_si4708_dev->client, &fm_si4708_dev->regs[FM_REG_WR_START], 4); //start tune.
	if(res <0)
	{
		fm_si4708_dev->initialized = 0;
    		return res;
	}

	mdelay(SI4708_SEEK_TUNE_DELAY);    /*seek time*/
	   
	/*check STC status*/
       res = si4708_regs_read(fm_si4708_dev->client, &fm_si4708_dev->regs[FM_REG_RD_START], 4); 
	if(res <0)
	{
		fm_si4708_dev->initialized = 0;
    		return res;
	}

       //no matter what's the STC status, we should shold stop tunning here.
	fm_si4708_dev->regs[FM_REG_CHANNEL] &= ~REG_TUNE_MASK;
    	res = si4708_regs_write(fm_si4708_dev->client, &fm_si4708_dev->regs[FM_REG_WR_START], 3); 
	if(res < 0)
	{
	       fm_si4708_dev->initialized = 0;
		return res;
	}

	/*check whether tune complete*/
	if( REG_STC_MASK & fm_si4708_dev->regs[FM_REG_STATUSRSSI])
	{	
		//clear STC
		fm_si4708_dev->regs[FM_REG_POWERCFG] = 0x40;
		fm_si4708_dev->regs[FM_REG_CHANNEL] &= ~REG_TUNE_MASK; 
		si4708_regs_write(fm_si4708_dev->client, &fm_si4708_dev->regs[FM_REG_POWERCFG], 3); 

		return res;
	 }
	else
	{
		//clear STC
		fm_si4708_dev->regs[FM_REG_POWERCFG] = 0x40;
		fm_si4708_dev->regs[FM_REG_CHANNEL] &= ~REG_TUNE_MASK; 
		si4708_regs_write(fm_si4708_dev->client, &fm_si4708_dev->regs[FM_REG_POWERCFG], 3); 

		return -EFAULT;
	}
	
}


/**********************************************************************
 * Function:    si4708_seek(int dir, int *p_frequency)
 * Description: This function seeks the channels
 * Input:        dir: direction requested
 * Output:      p_frequency:seeked frequency
 * Return:      0 = success, negative = failure
 * Others:      
 * Modify Date    Version    Author        Modification
 * ----------------------------------------------------
 * 2009/11/11       1.0      fengyuao        modify
 **********************************************************************/
static int si4708_seek(int dir, int *p_frequency)
{
    	int res = 0;
	int seeked_channel;

       res = si4708_regs_read(fm_si4708_dev->client, &fm_si4708_dev->regs[FM_REG_RD_START], 24);
	if(res <0)
	{
		fm_si4708_dev->initialized = 0;
    		return res;
	}
	/* Check STC bit to see whether the device is busy now.*/
	if( (fm_si4708_dev->regs[FM_REG_STATUSRSSI] & REG_STC_MASK) == REG_STC_MASK)
		return -EBUSY;

	/*now we check SEEK and SEEKUP bits ,and must set SKMODE  !*/
	if(dir == SEEKDOWN)
	{
		fm_si4708_dev->regs[FM_REG_POWERCFG] &= ~REG_SEEKUP_MASK;
		fm_si4708_dev->regs[FM_REG_POWERCFG] |= REG_SEEK_MASK;
	}
	if(dir == SEEKUP)
    		 fm_si4708_dev->regs[FM_REG_POWERCFG] |= REG_SEEKUP_MASK|REG_SEEK_MASK; 

	fm_si4708_dev->regs[FM_REG_POWERCFG] &= ~REG_SKMODE_MASK;
	/*write seek information to register!*/
   	res = si4708_regs_write(fm_si4708_dev->client, &fm_si4708_dev->regs[FM_REG_WR_START], 1);//start seek
	if(res <0 )
	{
		 fm_si4708_dev->initialized = 0;
    		 return res;
	}
	
	mdelay(SI4708_SEEK_TUNE_DELAY);    /*seek time*/
	   
	/*check it is whether seek complete*/
       res = si4708_regs_read(fm_si4708_dev->client, &fm_si4708_dev->regs[FM_REG_RD_START], 4); //read statusrssi
	if(res <0)
	{
		fm_si4708_dev->initialized = 0;
    		return res;
	}

        /*no matter what's the STC status, we should shold stop seeking here.*/
	fm_si4708_dev->regs[FM_REG_CHANNEL] &= ~REG_TUNE_MASK;
    	res = si4708_regs_write(fm_si4708_dev->client, &fm_si4708_dev->regs[FM_REG_WR_START], 3); 
	if(res < 0)
	{
	       fm_si4708_dev->initialized = 0;
		return res;
	}
	/*check seek whether complete .if it has completed,return seeked_channal to user programme */
	if( REG_STC_MASK & fm_si4708_dev->regs[FM_REG_STATUSRSSI])
	{	
		if( REG_SF_MASK & fm_si4708_dev->regs[FM_REG_STATUSRSSI])
		{
			*p_frequency = channel2freq(fm_si4708_dev->channel);	
		}
		else
		{
			seeked_channel = (int) fm_si4708_dev->regs[FM_REG_READCHAN+1]+(int)((fm_si4708_dev->regs[FM_REG_READCHAN]& 0x3) <<8);
			*p_frequency = channel2freq(seeked_channel);
		}
		//clear STC and SF
		fm_si4708_dev->regs[FM_REG_POWERCFG] = 0x40;
		fm_si4708_dev->regs[FM_REG_CHANNEL] &= ~REG_TUNE_MASK; 
		si4708_regs_write(fm_si4708_dev->client, &fm_si4708_dev->regs[FM_REG_POWERCFG], 3); 

		 return res;
	 }
	else
	{	
		//clear STC and SF
		fm_si4708_dev->regs[FM_REG_POWERCFG] = 0x40;
		fm_si4708_dev->regs[FM_REG_CHANNEL] &= ~REG_TUNE_MASK; 
		si4708_regs_write(fm_si4708_dev->client, &fm_si4708_dev->regs[FM_REG_POWERCFG], 3); 
		
		return -EBUSY;
	}
}


/**********************************************************************
 * Function:    si4708_open(struct inode *inode, struct file *filp)
 * Description: This function is called when the device file is opened
 * Input:       inode: the commonly used inode struct
 *                 filp: the commonly used file struct
 * Output:      none
 * Return:      0 = success, negative = failure
 * Others:      
 * Modify Date    Version    Author        Modification
 * ----------------------------------------------------
 * 2009/11/11       1.0      fengyuao        modify
 **********************************************************************/
/* ZTE_Audio_CJ_100511, chenjun, 2010-5-11, start */
extern int snd_hph_amp_ctl(uint32_t on);

static int si4708_open(struct inode *inode, struct file *filp)
{
    	int res;
	int sdio_state;

	snd_hph_amp_ctl(0);
    
	/*checkout if the device is open*/
	if(fm_si4708_dev->initialized)
	{
	    	res = si4708_standby2normal();
	}
	else
       {
		spin_lock_init(&fm_si4708_dev->lock);
		gpio_direction_output(FM_RESET_GPIO,0);
		mdelay(SI4708_RESET_DELAY);
		
		spin_lock_irq(&fm_si4708_dev->lock);
		sdio_state = gpio_get_value(FM_I2C_SDA_GPIO);
		gpio_direction_output(FM_I2C_SDA_GPIO, 0); 
		udelay(2);
		gpio_direction_output(FM_RESET_GPIO,1);
		udelay(2);
		gpio_direction_output(FM_I2C_SDA_GPIO,sdio_state); 
		spin_unlock_irq(&fm_si4708_dev->lock);

		res = si4708_init2normal();
		if( res > 0)
		      fm_si4708_dev->initialized++;
	}

	snd_hph_amp_ctl(1);

       if(res < 0)
	   	return res;

	res = si4708_read_all_regs();
/*setting interfaces,here we get init status,application could change it after init*/
	si4708_get_band();
	si4708_get_vol();
	si4708_get_space();
	si4708_get_track();
	si4708_get_frequency();
/*end of getting status*/
	return res < 0 ? res : 0;
}
/* ZTE_Audio_CJ_100511, chenjun, 2010-5-11, end */

/**********************************************************************
 * Function:    si4708_release(struct inode *inode, struct file *filp)
 * Description: This function is called when the device file is closed
 * Input:       inode: the commonly used inode struct
 *              filp: the commonly used file struct
 * Output:      none
 * Return:      0 = success, negative = failure
 * Others:      
 * Modify Date    Version    Author        Modification
 * ----------------------------------------------------
 * 2009/11/11       1.0      fengyuao        modify
 **********************************************************************/
static int si4708_release(struct inode *inode, struct file *filp)
{
	int res;
	/*set close status,flag = 0*/
	fm_si4708_dev->initialized = 0;
	/* Make sure the device is power down. */
	res = si4708_normal2standby();

	return res < 0 ? res : 0;
}


/**********************************************************************
 * Function:    si4702_ioctl(struct inode *inode, struct file *filp,
 *                       unsigned int cmd, unsigned long arg)
 * Description: This function is called when an ioctl system call is
 *              issued on the device file
 * Input:       inode: the commonly used inode struct
 *              filp: the commonly used file struct
 *              cmd: IOCTL command
 *              arg: IOCTL argument
 * Output:      none
 * Return:      0 = success, negative = failure
 * Others:      
 * Modify Date    Version    Author        Modification
 * ----------------------------------------------------
 * 2009/11/11       1.0      fengyuao        modify
 **********************************************************************/
static int si4708_ioctl(struct inode *inode, struct file *filp,
                     unsigned int cmd, unsigned long arg)
{
    	int res = 0;
	int user_data;
	int user_value[2];
    	
	/*kernel-space communicates with user-space */
    	switch (cmd) 
	{
		case	FM_INIT2NORMAL:
			res = si4708_init2normal(); 
		break;
		
		case	FM_NORMAL2STANDBY:
			res = si4708_normal2standby(); 
		break;
		
		case	FM_STANDBY2NORMAL:
			res = si4708_standby2normal();		
		break;
	
		case 	FM_TUNE:
			if (get_user(user_data,(int *)arg))
			{
	  		     res = -EFAULT;
				goto exit;
    		       }
	        	res = si4708_set_frequency(user_data);
	        break;

		 case	 FM_SEEK:
			if (copy_from_user(user_value,(int *)arg, 2*sizeof(int)))
			{
	   		    res = -EFAULT;
			    goto exit;
       		}
	       	 res = si4708_seek(user_value[0], &user_value[1]);
			 if(res < 0)
			 	goto exit;
			 if (copy_to_user((int *)arg,user_value, 2*sizeof(int)))
	       	  {
	            	       res = -EFAULT;
			       goto exit;
	       	  }
	        break;
		
		 case 	FM_GET_VOL:
	       	  if (put_user((int)si4708_get_vol(), (int*)arg))
	       	  {
	            	       res = -EFAULT;
			       goto exit;
	       	  }
	        break;

		 case 	FM_SET_VOL:
			if (get_user(user_data,(int *)arg))
			{
	  		       res = -EFAULT;
				goto exit;
    		       }
	       	 res = si4708_set_vol(int_to_char(user_data)); 
	        break;

		 case 	FM_GET_BAND:
	       	 if (put_user((int)si4708_get_band(), (int*)arg))
	       	  {
	            	       res = -EFAULT;
			       goto exit;
	       	  }
		 break;
				
	    	 case 	FM_SET_BAND:	
			if (get_user(user_data,(int *)arg))
			{
	  		       res = -EFAULT;
				goto exit;
    		       }
			res = si4708_set_band(int_to_char(user_data));
	        break;
			 
		 case 	FM_GET_SPACE:
	       	 if (put_user((int)si4708_get_space(), (int*)arg))
	       	  {
	            	       res = -EFAULT;
			       goto exit;
	       	  }
	        break;

		 case 	FM_SET_SPACE:
			if (get_user(user_data,(int *)arg))
			{
	  		       res = -EFAULT;
				goto exit;
    		       }
	        	res = si4708_set_space(int_to_char(user_data));
	        break;

		 case	FM_GET_AUDIOTRACK:
	       	 if (put_user((int)si4708_get_track(), (int *)arg))
	       	  {
	            	       res = -EFAULT;
			       goto exit;
	       	  }
		 break;

		 case	FM_SET_AUDIOTRACK:
			if (get_user(user_data,(int *)arg))
			{
	  		     res = -EFAULT;
				goto exit;
    		       }
			res = si4708_set_track(int_to_char(user_data));
		 break;

		 case	FM_GET_CURRENTFREQ:
	       	 if (put_user(si4708_get_frequency(), (int *)arg))
	       	  {
	            	       res = -EFAULT;
			       goto exit;
	       	  }
		 break;
		 
	       default:
	              return  -ENOTTY;
      }

exit:		
   return res < 0 ?  res : 0;

}


/**********************************************************************/

static struct file_operations si4708_fops = {
	 .owner 	= THIS_MODULE,
        .ioctl =	si4708_ioctl,
        .open = si4708_open,
        .release = si4708_release,
};

/**********************************************************************/

static struct miscdevice si4708_device = {
	.minor 	=  MISC_DYNAMIC_MINOR,
	.name 	=  "si4708",
	.fops 	= &si4708_fops,
};


/**********************************************************************/

static int si4708_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int res = 0;
	
	/*check if i2c communicated with FM successfully*/
     	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))	
		return -EIO;

	fm_si4708_dev = kzalloc(sizeof(struct fm_dev), GFP_KERNEL);
	if (!fm_si4708_dev){ 
		res = - ENOMEM;
		goto init_exit;
	}

	fm_si4708_dev->initialized = 0;
      	
	/* Register a misc device */
	res = misc_register(&si4708_device);
	if(res)
		goto out;
	
	FM_INFO(FYA_TAG"register fm_si4708 device successful!\n");
	fm_si4708_dev->client = client;
	
	return 0;
out:
	printk(KERN_ERR"%s:Driver Initialisation failed\n",__FILE__);
init_exit:
	/*	ZTE_SI4708_F.YUAO_005 2009-12-05	*/
	kfree(fm_si4708_dev);
	/*end,ZTE_SI4708_F.YUAO_005 2009-12-05	*/
	return res;
}


/**********************************************************************
 * Function:    si4708_remove(void)
 * Description: This function is called when the driver module is
 *              removed
 * Input:       none
 * Output:      none
 * Return:      0 = success, negative = failure
 * Others:      
 * Modify Date    Version    Author        Modification
 * ----------------------------------------------------
 * 2009/11/11       1.0      fengyuao        modify
 **********************************************************************/
static int si4708_remove(struct i2c_client *client)
{
/*	ZTE_SI4708_F.YUAO_005 2009-12-05	*/
	misc_deregister(&si4708_device);
/*end,ZTE_SI4708_F.YUAO_005 2009-12-05	*/
	i2c_release_client(client);
	return 0;
}

/**********************************************************************/

static const struct i2c_device_id si4708_id[] = { 
	{ "si4708", 0 },
	{},
};

/**********************************************************************/

struct i2c_driver si4708_driver = {
	.probe  = si4708_probe,
	.remove = si4708_remove,
	.driver = {
		.name = "si4708",
		.owner = THIS_MODULE,
	},
	.id_table   = si4708_id,
};

/**********************************************************************
 * Function:    i2c_si4708_init(void)
 * Description: This function is called when the driver module is
 *              initialized
 * Input:       none
 * Output:      none
 * Return:      0 = success, negative = failure
 * Others:      
 * Modify Date    Version    Author        Modification
 * ----------------------------------------------------
 * 2009/11/11       1.0      fengyuao        modify
 **********************************************************************/
int __init i2c_si4708_init(void)
{
	int res;

	res = i2c_add_driver(&si4708_driver);
	if (res) 
		FM_DBG(FYA_TAG"fm_si4708 add driver failed\n");
	FM_INFO(FYA_TAG"fm_si4708 add driver successful\n");
	
	return 0;
} 

/**********************************************************************
 * Function:    i2c_si4708_exit(void)
 * Description: This function is called when the driver module is
 *              removed
 * Input:       none
 * Output:      none
 * Return:      0 = success, negative = failure
 * Others:      
 * Modify Date    Version    Author        Modification
 * ----------------------------------------------------
 * 2009/11/11       1.0      fengyuao        modify
 **********************************************************************/
void __exit  i2c_si4708_exit(void)
{
/*	ZTE_SI4708_F.YUAO_005 2009-12-05	*/
	pr_info(FYA_TAG"fm_si4708 driver exit!\n");	
	i2c_del_driver(&si4708_driver);
/*	ZTE_SI4708_F.YUAO_005 2009-12-05	*/
}

/**********************************************************************/

MODULE_LICENSE("GPL");
MODULE_VERSION("2.0");
MODULE_AUTHOR("feng.yuao@zte.com.cn");
MODULE_DESCRIPTION("Silicon Laboratories'4708 semiconductor driver");
module_init(i2c_si4708_init);
module_exit(i2c_si4708_exit);
