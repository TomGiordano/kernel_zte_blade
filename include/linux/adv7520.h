/* Copyright (c) 2010, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *     * Neither the name of Code Aurora Forum, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef _ADV7520_H_
#define _ADV7520_H_
#define ADV7520_DRV_NAME 		"adv7520"

#define HDMI_XRES			1280
#define HDMI_YRES			720
#define HDMI_PIXCLOCK_MAX		74250
#define ADV7520_EDIDI2CSLAVEADDRESS   	0xA0

#define DEBUG  0

/* Configure the 20-bit 'N' used with the CTS to
regenerate the audio clock in the receiver
Pixel clock: 74.25 Mhz, Audio sampling: 44.1 Khz -> N
value = 6272 */
#define ADV7520_AUDIO_CTS_20BIT_N   6272

/* HDMI EDID Length  */
#define HDMI_EDID_MAX_LENGTH    256

/* HDMI EDID DTDs  */
#define HDMI_EDID_MAX_DTDS      4

/*  EDID - Extended Display ID Data structs  */

/*  Video Descriptor Block  */
struct hdmi_edid_dtd_video {
	u8   pixel_clock[2];          /* 54-55 */
	u8   horiz_active;            /* 56 */
	u8   horiz_blanking;          /* 57 */
	u8   horiz_high;              /* 58 */
	u8   vert_active;             /* 59 */
	u8   vert_blanking;           /* 60 */
	u8   vert_high;               /* 61 */
	u8   horiz_sync_offset;       /* 62 */
	u8   horiz_sync_pulse;        /* 63 */
	u8   vert_sync_pulse;         /* 64 */
	u8   sync_pulse_high;         /* 65 */
	u8   horiz_image_size;        /* 66 */
	u8   vert_image_size;	      /* 67 */
	u8   image_size_high;         /* 68 */
	u8   horiz_border;            /* 69 */
	u8   vert_border;             /* 70 */
	u8   misc_settings;           /* 71 */
} ;

/*  EDID structure  */
struct hdmi_edid {			/* Bytes */
	u8   edid_header[8];            /* 00-07 */
	u8   manufacturerID[2];      	/* 08-09 */
	u8   product_id[2];           	/* 10-11 */
	u8   serial_number[4];        	/* 12-15 */
	u8   week_manufactured;       	/* 16 */
	u8   year_manufactured;       	/* 17 */
	u8   edid_version;            	/* 18 */
	u8   edid_revision;           	/* 19 */

	u8   video_in_definition;      	/* 20 */
	u8   max_horiz_image_size;      /* 21 */
	u8   max_vert_image_size;       /* 22 */
	u8   display_gamma;           	/* 23 */
	u8   power_features;          	/* 24 */
	u8   chroma_info[10];         	/* 25-34 */
	u8   timing_1;                	/* 35 */
	u8   timing_2;               	/* 36 */
	u8   timing_3;              	/* 37 */
	u8   std_timings[16];         	/* 38-53 */

	struct hdmi_edid_dtd_video dtd[4];   /* 54-125 */

	u8   extension_edid;          	/* 126 */
	u8   checksum;               	/* 127 */

	u8   extension_tag;           	/* 00 (extensions follow EDID) */
	u8   extention_rev;           	/* 01 */
	u8   offset_dtd;              	/* 02 */
	u8   num_dtd;                 	/* 03 */

	u8   data_block[123];         	/* 04 - 126 */
	u8   extension_checksum;      	/* 127 */
} ;

#endif
