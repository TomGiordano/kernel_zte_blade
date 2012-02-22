/******************************************************************************
 * isl29026.h - Linux kernel module for Intersil ISL29028 ambient light sensor
 *		and proximity sensor
 *
 * Copyright 2008-2010 Intersil Inc..
 *
 ********************************************************/

#ifndef __ISL29026_H__
#define __ISL29026_H__

// ioctl numbers
#define ISL_IOCTL_MAGIC        	0XCF
#define ISL_IOCTL_ALS_ON       	_IO(ISL_IOCTL_MAGIC, 1)
#define ISL_IOCTL_ALS_OFF      	_IO(ISL_IOCTL_MAGIC, 2)
#define ISL_IOCTL_ALS_DATA     	_IOR(ISL_IOCTL_MAGIC, 3, short)
#define ISL_IOCTL_ALS_CALIBRATE	_IO(ISL_IOCTL_MAGIC, 4)
#define ISL_IOCTL_CONFIG_GET   	_IOR(ISL_IOCTL_MAGIC, 5, struct ISL_cfg)
#define ISL_IOCTL_CONFIG_SET		_IOW(ISL_IOCTL_MAGIC, 6, struct ISL_cfg)
#define ISL_IOCTL_PROX_ON		_IO(ISL_IOCTL_MAGIC, 7)
#define ISL_IOCTL_PROX_OFF		_IO(ISL_IOCTL_MAGIC, 8)
#define ISL_IOCTL_PROX_DATA		_IOR(ISL_IOCTL_MAGIC, 9, struct ISL_prox_info)
#define ISL_IOCTL_PROX_EVENT           _IO(ISL_IOCTL_MAGIC, 10)
#define ISL_IOCTL_PROX_CALIBRATE	_IO(ISL_IOCTL_MAGIC, 11)
#define ISL_IOCTL_PROX_GET_ENABLED   	_IOR(ISL_IOCTL_MAGIC, 12, int*)
#define ISL_IOCTL_ALS_GET_ENABLED   	_IOR(ISL_IOCTL_MAGIC, 13, int*)
//
//#define u32 unsigned int 
//#define u16 unsigned short  
//#define u8   unsigned char 

// device configuration
struct ISL_cfg {
	u32	calibrate_target;
	u16	als_time;
	u16	scale_factor;
	u16	gain_trim;
 	u8	filter_history;
	u8	filter_count;
	u8	gain;
	u16	prox_threshold_hi;
	u16	prox_threshold_lo;
	u8	prox_int_time;
	u8	prox_adc_time;
	u8	prox_wait_time;
	u8	prox_intr_filter;
	u8	prox_config;
	u8	prox_pulse_cnt;
	u8	prox_gain;
};

// proximity data
struct ISL_prox_info {
        u16 prox_clear;
        u16 prox_data;
        int prox_event;
};

#endif

