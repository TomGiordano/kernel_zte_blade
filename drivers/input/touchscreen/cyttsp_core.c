/* Source for:
 * Cypress TrueTouch(TM) Standard Product touchscreen driver.
 * drivers/input/touchscreen/cyttsp_core.c
 *
 * Copyright (C) 2009-2011 Cypress Semiconductor, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, and only version 2, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * Cypress reserves the right to make changes without further notice
 * to the materials described herein. Cypress does not assume any
 * liability arising out of the application described herein.
 *
 * Contact Cypress Semiconductor at www.cypress.com
 *
 */

#include <linux/delay.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/byteorder/generic.h>
#include <linux/bitops.h>
#include <linux/earlysuspend.h>
#include <linux/cyttsp.h>
#include <linux/ctype.h>
#include "cyttsp_core.h"

#define DBG(x)
#define DBG2(x)
#define DBG3(x)

/* rely on kernel input.h to define Multi-Touch capability */
#ifndef ABS_MT_TRACKING_ID
/* define only if not defined already by system; */
/* value based on linux kernel 2.6.30.10 */
#define ABS_MT_TRACKING_ID (ABS_MT_BLOB_ID + 1)
#endif /* ABS_MT_TRACKING_ID */

#define	TOUCHSCREEN_TIMEOUT (msecs_to_jiffies(28))
/* Bootloader File 0 offset */
#define CY_BL_FILE0       0x00
/* Bootloader command directive */
#define CY_BL_CMD         0xFF
/* Bootloader Enter Loader mode */
#define CY_BL_ENTER       0x38
/* Bootloader Write a Block */
#define CY_BL_WRITE_BLK   0x39
/* Bootloader Terminate Loader mode */
#define CY_BL_TERMINATE   0x3B
/* Bootloader Exit and Verify Checksum command */
#define CY_BL_EXIT        0xA5
/* Bootloader default keys */
#define CY_BL_KEY0 0
#define CY_BL_KEY1 1
#define CY_BL_KEY2 2
#define CY_BL_KEY3 3
#define CY_BL_KEY4 4
#define CY_BL_KEY5 5
#define CY_BL_KEY6 6
#define CY_BL_KEY7 7

#define CY_DIFF(m, n)               ((m) != (n))
#define GET_NUM_TOUCHES(x)          ((x) & 0x0F)
#define GET_TOUCH1_ID(x)            (((x) & 0xF0) >> 4)
#define GET_TOUCH2_ID(x)            ((x) & 0x0F)
#define GET_TOUCH3_ID(x)            (((x) & 0xF0) >> 4)
#define GET_TOUCH4_ID(x)            ((x) & 0x0F)
#define IS_LARGE_AREA(x)            (((x) & 0x10) >> 4)
#define IS_BAD_PKT(x)               ((x) & 0x20)
#define FLIP_DATA_FLAG              0x01
#define REVERSE_X_FLAG              0x02
#define REVERSE_Y_FLAG              0x04
#define FLIP_DATA(flags)            ((flags) & FLIP_DATA_FLAG)
#define REVERSE_X(flags)            ((flags) & REVERSE_X_FLAG)
#define REVERSE_Y(flags)            ((flags) & REVERSE_Y_FLAG)
#define FLIP_XY(x, y)      {typeof(x) tmp; tmp = (x); (x) = (y); (y) = tmp; }
#define INVERT_X(x, xmax)           ((xmax) - (x))
#define INVERT_Y(y, ymax)           ((ymax) - (y))
#define SET_HSTMODE(reg, mode)      ((reg) & (mode))
#define GET_HSTMODE(reg)            ((reg & 0x70) >> 4)
#define GET_BOOTLOADERMODE(reg)     ((reg & 0x10) >> 4)

/* Watchdog timeout to check if device is reset and no interrupts running */
#define CY_WDG_TIMEOUT              msecs_to_jiffies(3000)
#define CY_MODE_ERROR(ps, hst_mode, tt_mode) \
	((ps == CY_ACTIVE_STATE && GET_HSTMODE(hst_mode) != CY_OPERATE_MODE) ||\
	GET_BOOTLOADERMODE(tt_mode))

/* maximum number of concurrent ST track IDs */
#define CY_NUM_ST_TCH_ID            2
/* maximum number of concurrent MT track IDs */
#define CY_NUM_MT_TCH_ID            4
/* maximum number of track IDs */
#define CY_NUM_TRK_ID               16
/*
 * maximum number of concurrent touches
 * (only CY_NUM_MT_TCH_ID have coord data)
 */
#define CY_MAX_TCH                  10
/*
 * maximum number of touch reports with
 * current touches=0 before performing Driver reset
 */
#define CY_MAX_NTCH                 10

#define CY_NTCH                     0 /* lift off */
#define CY_TCH                      1 /* touch down */
#define CY_ST_FNGR1_IDX             0
#define CY_ST_FNGR2_IDX             1
#define CY_MT_TCH1_IDX              0
#define CY_MT_TCH2_IDX              1
#define CY_MT_TCH3_IDX              2
#define CY_MT_TCH4_IDX              3
#define CY_XPOS                     0
#define CY_YPOS                     1
#define CY_IGNR_TCH               (-1)
#define CY_SMALL_TOOL_WIDTH         10
#define CY_LARGE_TOOL_WIDTH         255
#define CY_REG_BASE                 0x00
#define CY_REG_GEST_SET             0x1E
#define CY_REG_SCN_TYP              0x1C
#define CY_REG_ACT_INTRVL           0x1D
#define CY_REG_TCH_TMOUT            (CY_REG_ACT_INTRVL+1)
#define CY_REG_LP_INTRVL            (CY_REG_TCH_TMOUT+1)
#define CY_SOFT_RESET               (1 << 0)
#define CY_DEEP_SLEEP               (1 << 1)
#define CY_LOW_POWER                (1 << 2)
#define CY_MAXZ                     255
#define CY_OK                       0
#define CY_INIT                     1
#define CY_DELAY_DFLT               20 /* ms */
#define CY_DELAY_MAX                (500/CY_DELAY_DFLT) /* half second */
#define CY_DELAY_SYSINFO            20 /* ms */
#define CY_DELAY_BL                 300
#define CY_DELAY_DNLOAD             100 /* ms */
#define CY_HNDSHK_BIT               0x80
/* device mode bits */
#define CY_OPERATE_MODE             0x00
#define CY_SYSINFO_MODE             0x10
/* power mode select bits */
#define CY_SOFT_RESET_MODE          0x01 /* return to Bootloader mode */
#define CY_DEEP_SLEEP_MODE          0x02
#define CY_LOW_POWER_MODE           0x04
#define CY_NUM_KEY                  8

/* TrueTouch Standard Product Gen3 (Txx3xx) interface definition */
struct cyttsp_xydata {
	u8 hst_mode;
	u8 tt_mode;
	u8 tt_stat;
	u16 x1 __attribute__ ((packed));
	u16 y1 __attribute__ ((packed));
	u8 z1;
	u8 touch12_id;
	u16 x2 __attribute__ ((packed));
	u16 y2 __attribute__ ((packed));
	u8 z2;
	u8 gest_cnt;
	u8 gest_id;
	u16 x3 __attribute__ ((packed));
	u16 y3 __attribute__ ((packed));
	u8 z3;
	u8 touch34_id;
	u16 x4 __attribute__ ((packed));
	u16 y4 __attribute__ ((packed));
	u8 z4;
	u8 tt_undef[3];
	u8 gest_set;
	u8 tt_reserved;
};

struct cyttsp_xydata_gen2 {
	u8 hst_mode;
	u8 tt_mode;
	u8 tt_stat;
	u16 x1 __attribute__ ((packed));
	u16 y1 __attribute__ ((packed));
	u8 z1;
	u8 evnt_idx;
	u16 x2 __attribute__ ((packed));
	u16 y2 __attribute__ ((packed));
	u8 tt_undef1;
	u8 gest_cnt;
	u8 gest_id;
	u8 tt_undef[14];
	u8 gest_set;
	u8 tt_reserved;
};

/* TrueTouch Standard Product Gen2 (Txx2xx) interface definition */
enum cyttsp_gen2_std {
	CY_GEN2_NOTOUCH = 0x03, /* Both touches removed */
	CY_GEN2_GHOST =   0x02, /* ghost */
	CY_GEN2_2TOUCH =  0x03, /* 2 touch; no ghost */
	CY_GEN2_1TOUCH =  0x01, /* 1 touch only */
	CY_GEN2_TOUCH2 =  0x01, /* 1st touch removed; 2nd touch remains */
};

/* TTSP System Information interface definition */
struct cyttsp_sysinfo_data {
	u8 hst_mode;
	u8 mfg_stat;
	u8 mfg_cmd;
	u8 cid[3];
	u8 tt_undef1;
	u8 uid[8];
	u8 bl_verh;
	u8 bl_verl;
	u8 tts_verh;
	u8 tts_verl;
	u8 app_idh;
	u8 app_idl;
	u8 app_verh;
	u8 app_verl;
	u8 tt_undef[5];
	u8 scn_typ;	/* Gen3 only: scan type [0:Mutual, 1:Self] */
	u8 act_intrvl;
	u8 tch_tmout;
	u8 lp_intrvl;
};

/* TTSP Bootloader Register Map interface definition */
#define CY_BL_CHKSUM_OK 0x01
struct cyttsp_bootloader_data {
	u8 bl_file;
	u8 bl_status;
	u8 bl_error;
	u8 blver_hi;
	u8 blver_lo;
	u8 bld_blver_hi;
	u8 bld_blver_lo;
	u8 ttspver_hi;
	u8 ttspver_lo;
	u8 appid_hi;
	u8 appid_lo;
	u8 appver_hi;
	u8 appver_lo;
	u8 cid_0;
	u8 cid_1;
	u8 cid_2;
};

#define cyttsp_wake_data cyttsp_xydata

struct cyttsp {
	struct device *pdev;
	int irq;
	struct input_dev *input;
	struct work_struct work;
	struct timer_list timer;
	struct mutex mutex;
	struct early_suspend early_suspend;
	char phys[32];
	struct cyttsp_platform_data *platform_data;
	struct cyttsp_bootloader_data bl_data;
	struct cyttsp_sysinfo_data sysinfo_data;
	u8 num_prv_st_tch;
	u16 act_trk[CY_NUM_TRK_ID];
	u16 prv_mt_tch[CY_NUM_MT_TCH_ID];
	u16 prv_st_tch[CY_NUM_ST_TCH_ID];
	u16 prv_mt_pos[CY_NUM_TRK_ID][2];
	struct cyttsp_bus_ops *bus_ops;
	unsigned fw_loader_mode:1;
	unsigned suspended:1;
	struct timer_list to_timer;
	bool to_timeout;
	struct completion int_running;
	bool bl_ready;
	u8 reg_id;
	u8 ntch_count;
};

struct cyttsp_track_data {
	u8 prv_tch;
	u8 cur_tch;
	u16 tmp_trk[CY_NUM_MT_TCH_ID];
	u16 snd_trk[CY_NUM_MT_TCH_ID];
	u16 cur_trk[CY_NUM_TRK_ID];
	u16 cur_st_tch[CY_NUM_ST_TCH_ID];
	u16 cur_mt_tch[CY_NUM_MT_TCH_ID];
	/* if NOT CY_USE_TRACKING_ID then only */
	/* uses CY_NUM_MT_TCH_ID positions */
	u16 cur_mt_pos[CY_NUM_TRK_ID][2];
	/* if NOT CY_USE_TRACKING_ID then only */
	/* uses CY_NUM_MT_TCH_ID positions */
	u8 cur_mt_z[CY_NUM_TRK_ID];
	u8 tool_width;
	u16 st_x1;
	u16 st_y1;
	u8 st_z1;
	u16 st_x2;
	u16 st_y2;
	u8 st_z2;
};

static const u8 bl_cmd[] = {
	CY_BL_FILE0, CY_BL_CMD, CY_BL_EXIT,
	CY_BL_KEY0, CY_BL_KEY1, CY_BL_KEY2,
	CY_BL_KEY3, CY_BL_KEY4, CY_BL_KEY5,
	CY_BL_KEY6, CY_BL_KEY7
};

#define LOCK(m) do { \
	DBG(printk(KERN_INFO "%s: lock\n", __func__);) \
	mutex_lock(&(m)); \
} while (0);

#define UNLOCK(m) do { \
	DBG(printk(KERN_INFO "%s: unlock\n", __func__);) \
	mutex_unlock(&(m)); \
} while (0);

DBG(
static void print_data_block(const char *func, u8 command,
			u8 length, void *data)
{
	char buf[1024];
	unsigned buf_len = sizeof(buf);
	char *p = buf;
	int i;
	int l;

	l = snprintf(p, buf_len, "cmd 0x%x: ", command);
	buf_len -= l;
	p += l;
	for (i = 0; i < length && buf_len; i++, p += l, buf_len -= l)
		l = snprintf(p, buf_len, "%02x ", *((char *)data + i));
	printk(KERN_DEBUG "%s: %s\n", func, buf);
})

static int cyttsp_soft_reset(struct cyttsp *ts);
static int cyttsp_set_operational_mode(struct cyttsp *ts);
static int cyttsp_exit_bl_mode(struct cyttsp *ts);
static int cyttsp_power_on(struct cyttsp *ts);
static void cyttsp_init_tch(struct cyttsp *ts)
{
	/* init the touch structures */
	ts->num_prv_st_tch = CY_NTCH;
	memset(ts->act_trk, CY_NTCH, sizeof(ts->act_trk));
	memset(ts->prv_mt_pos, CY_NTCH, sizeof(ts->prv_mt_pos));
	memset(ts->prv_mt_tch, CY_IGNR_TCH, sizeof(ts->prv_mt_tch));
	memset(ts->prv_st_tch, CY_IGNR_TCH, sizeof(ts->prv_st_tch));
	ts->ntch_count = 0;
}

static u8 ttsp_convert_gen2(u8 cur_tch, struct cyttsp_xydata *pxy_data)
{
	struct cyttsp_xydata_gen2 *pxy_data_gen2;
	pxy_data_gen2 = (struct cyttsp_xydata_gen2 *)(pxy_data);

	if (pxy_data_gen2->evnt_idx == CY_GEN2_NOTOUCH) {
		cur_tch = 0;
	} else if (cur_tch == CY_GEN2_GHOST) {
		/* stuff artificial track ID1 and ID2 */
		pxy_data->touch12_id = 0x12;//xym
		pxy_data->z1 = CY_MAXZ;//xym
		pxy_data->z2 = CY_MAXZ;//xym
		//cur_tch = 0;//xym
	} else if (cur_tch == CY_GEN2_2TOUCH) {
		/* stuff artificial track ID1 and ID2 */
		pxy_data->touch12_id = 0x12;
		pxy_data->z1 = CY_MAXZ;
		pxy_data->z2 = CY_MAXZ;
		cur_tch--; /* 2 touches */
	} else if (cur_tch == CY_GEN2_1TOUCH) {
		/* stuff artificial track ID1 and ID2 */
		pxy_data->touch12_id = 0x12;
		pxy_data->z1 = CY_MAXZ;
		pxy_data->z2 = CY_NTCH;
		if (pxy_data_gen2->evnt_idx == CY_GEN2_TOUCH2) {
			/* push touch 2 data into touch1
			 * (first finger up; second finger down) */
			/* stuff artificial track ID1 for touch2 info */
			pxy_data->touch12_id = 0x20;
			/* stuff touch 1 with touch 2 coordinate data */
			pxy_data->x1 = pxy_data->x2;
			pxy_data->y1 = pxy_data->y2;
		}
	} else {
		cur_tch = 0;
	}
	return cur_tch;
}

static int ttsp_read_block_data(struct cyttsp *ts, u8 command,
	u8 length, void *buf)
{
	int rc;
	int tries;
	DBG(printk(KERN_INFO"%s: Enter\n", __func__);)

	if (!buf || !length) {
		printk(KERN_ERR "%s: Error, buf:%s len:%u\n",
				__func__, !buf ? "NULL" : "OK", length);
		return -EIO;
	}

	for (tries = 0, rc = -1; tries < CY_NUM_RETRY && (rc < 0); tries++) {
		rc = ts->bus_ops->read(ts->bus_ops, command, length, buf);
		if (rc)
			msleep(CY_DELAY_DFLT);
	}

	if (rc < 0)
		printk(KERN_ERR "%s: error %d\n", __func__, rc);
	DBG(print_data_block(__func__, command, length, buf);)
	return rc;
}

static int ttsp_write_block_data(struct cyttsp *ts, u8 command,
	u8 length, void *buf)
{
	int rc;
	int tries;
	DBG(printk(KERN_INFO"%s: Enter\n", __func__);)

	if (!buf || !length) {
		printk(KERN_ERR "%s: Error, buf:%s len:%u\n",
				__func__, !buf ? "NULL" : "OK", length);
		return -EIO;
	}

	for (tries = 0, rc = -1; tries < CY_NUM_RETRY && (rc < 0); tries++) {
		rc = ts->bus_ops->write(ts->bus_ops, command, length, buf);
		if (rc)
			msleep(CY_DELAY_DFLT);
	}

	if (rc < 0)
		printk(KERN_ERR "%s: error %d\n", __func__, rc);
	DBG(print_data_block(__func__, command, length, buf);)
	return rc;
}

static int ttsp_tch_ext(struct cyttsp *ts, void *buf)
{
	int rc;
	DBG(printk(KERN_INFO"%s: Enter\n", __func__);)

	if (!buf) {
		printk(KERN_ERR "%s: Error, buf:%s\n",
				__func__, !buf ? "NULL" : "OK");
		return -EIO;
	}
	rc = ts->bus_ops->ext(ts->bus_ops, buf);
	if (rc < 0)
		printk(KERN_ERR "%s: error %d\n", __func__, rc);
	return rc;
}

/* ************************************************************************
 * The cyttsp_xy_worker function reads the XY coordinates and sends them to
 * the input layer.  It is called from the Touch Interrupt.
 * *************************************************************************/
static int cyttsp_inlist(u16 prev_track[], u8 cur_trk_id, u8 *prev_loc,
	u8 num_touches)
{
	u8 id = 0;

	DBG(printk(KERN_INFO"%s: IN p[%d]=%d c=%d n=%d loc=%d\n",
		__func__, id, prev_track[id], cur_trk_id,
		num_touches, *prev_loc);)

	for (*prev_loc = CY_IGNR_TCH; id < num_touches; id++) {
		DBG(printk(KERN_INFO"%s: p[%d]=%d c=%d n=%d loc=%d\n",
			__func__, id, prev_track[id], cur_trk_id,
				num_touches, *prev_loc);)
		if (prev_track[id] == cur_trk_id) {
			*prev_loc = id;
			break;
		}
	}
	DBG(printk(KERN_INFO"%s: OUT p[%d]=%d c=%d n=%d loc=%d\n", __func__,
		id, prev_track[id], cur_trk_id, num_touches, *prev_loc);)

	return *prev_loc < CY_NUM_TRK_ID;
}

static int cyttsp_next_avail_inlist(u16 cur_trk[], u8 *new_loc,
	u8 num_touches)
{
	u8 id = 0;
	DBG(printk(KERN_INFO"%s: Enter\n", __func__);)

	for (*new_loc = CY_IGNR_TCH; id < num_touches; id++) {
		if (cur_trk[id] > CY_NUM_TRK_ID) {
			*new_loc = id;
			break;
		}
	}
	return *new_loc < CY_NUM_TRK_ID;
}

static void handle_single_touch(struct cyttsp_xydata *xy,
	struct cyttsp_track_data *t, struct cyttsp *ts)
{
	u8 id;
	u8 use_trk_id = ts->platform_data->use_trk_id;

	DBG(printk(KERN_INFO"%s: ST STEP 0 - ST1 ID=%d  ST2 ID=%d\n",
		__func__, t->cur_st_tch[CY_ST_FNGR1_IDX],
		t->cur_st_tch[CY_ST_FNGR2_IDX]);)

	if (t->cur_st_tch[CY_ST_FNGR1_IDX] > CY_NUM_TRK_ID) {
		/* reassign finger 1 and 2 positions to new tracks */
		if (t->cur_tch > 0) {
			/* reassign st finger1 */
			if (use_trk_id) {
				id = CY_MT_TCH1_IDX;
				t->cur_st_tch[CY_ST_FNGR1_IDX] =
							t->cur_mt_tch[id];
			} else {
				id = GET_TOUCH1_ID(xy->touch12_id);
				t->cur_st_tch[CY_ST_FNGR1_IDX] = id;
			}
			t->st_x1 = t->cur_mt_pos[id][CY_XPOS];
			t->st_y1 = t->cur_mt_pos[id][CY_YPOS];
			t->st_z1 = t->cur_mt_z[id];

			DBG(printk(KERN_INFO"%s: ST STEP 1 - ST1 ID=%3d\n",
				__func__, t->cur_st_tch[CY_ST_FNGR1_IDX]);)

			if ((t->cur_tch > 1) &&
				(t->cur_st_tch[CY_ST_FNGR2_IDX] >
				CY_NUM_TRK_ID)) {
				/* reassign st finger2 */
				if (use_trk_id) {
					id = CY_MT_TCH2_IDX;
					t->cur_st_tch[CY_ST_FNGR2_IDX] =
						t->cur_mt_tch[id];
				} else {
					id = GET_TOUCH2_ID(xy->touch12_id);
					t->cur_st_tch[CY_ST_FNGR2_IDX] = id;
				}
				t->st_x2 = t->cur_mt_pos[id][CY_XPOS];
				t->st_y2 = t->cur_mt_pos[id][CY_YPOS];
				t->st_z2 = t->cur_mt_z[id];

				DBG(
				printk(KERN_INFO"%s: ST STEP 2 - ST2 ID=%3d\n",
				__func__, t->cur_st_tch[CY_ST_FNGR2_IDX]);)
			}
		}
	} else if (t->cur_st_tch[CY_ST_FNGR2_IDX] > CY_NUM_TRK_ID) {
		if (t->cur_tch > 1) {
			/* reassign st finger2 */
			if (use_trk_id) {
				/* reassign st finger2 */
				id = CY_MT_TCH2_IDX;
				t->cur_st_tch[CY_ST_FNGR2_IDX] =
							t->cur_mt_tch[id];
			} else {
				/* reassign st finger2 */
				id = GET_TOUCH2_ID(xy->touch12_id);
				t->cur_st_tch[CY_ST_FNGR2_IDX] = id;
			}
			t->st_x2 = t->cur_mt_pos[id][CY_XPOS];
			t->st_y2 = t->cur_mt_pos[id][CY_YPOS];
			t->st_z2 = t->cur_mt_z[id];

			DBG(printk(KERN_INFO"%s: ST STEP 3 - ST2 ID=%3d\n",
				   __func__, t->cur_st_tch[CY_ST_FNGR2_IDX]);)
		}
	}
	/* if the 1st touch is missing and there is a 2nd touch,
	 * then set the 1st touch to 2nd touch and terminate 2nd touch
	 */
	if ((t->cur_st_tch[CY_ST_FNGR1_IDX] > CY_NUM_TRK_ID) &&
			(t->cur_st_tch[CY_ST_FNGR2_IDX] < CY_NUM_TRK_ID)) {
		t->st_x1 = t->st_x2;
		t->st_y1 = t->st_y2;
		t->st_z1 = t->st_z2;
		t->cur_st_tch[CY_ST_FNGR1_IDX] = t->cur_st_tch[CY_ST_FNGR2_IDX];
		t->cur_st_tch[CY_ST_FNGR2_IDX] = CY_IGNR_TCH;
	}
	/* if the 2nd touch ends up equal to the 1st touch,
	 * then just report a single touch */
	if (t->cur_st_tch[CY_ST_FNGR1_IDX] == t->cur_st_tch[CY_ST_FNGR2_IDX])
		t->cur_st_tch[CY_ST_FNGR2_IDX] = CY_IGNR_TCH;

	/* set Single Touch current event signals */
	if (t->cur_st_tch[CY_ST_FNGR1_IDX] < CY_NUM_TRK_ID) {
		input_report_abs(ts->input, ABS_X, t->st_x1);
		input_report_abs(ts->input, ABS_Y, t->st_y1);
		input_report_abs(ts->input, ABS_PRESSURE, t->st_z1);
		input_report_key(ts->input, BTN_TOUCH, CY_TCH);
		input_report_abs(ts->input, ABS_TOOL_WIDTH, t->tool_width);

		DBG2(printk(KERN_INFO"%s:ST->F1:%3d X:%3d Y:%3d Z:%3d\n",
			   __func__, t->cur_st_tch[CY_ST_FNGR1_IDX],
			   t->st_x1, t->st_y1, t->st_z1);)

		if (t->cur_st_tch[CY_ST_FNGR2_IDX] < CY_NUM_TRK_ID) {
			input_report_key(ts->input, BTN_2, CY_TCH);
			input_report_abs(ts->input, ABS_HAT0X, t->st_x2);
			input_report_abs(ts->input, ABS_HAT0Y, t->st_y2);

			DBG2(printk(KERN_INFO
				"%s:ST->F2:%3d X:%3d Y:%3d Z:%3d\n",
				__func__, t->cur_st_tch[CY_ST_FNGR2_IDX],
				t->st_x2, t->st_y2, t->st_z2);)
		} else {
			input_report_key(ts->input, BTN_2, CY_NTCH);
		}
	} else {
		input_report_abs(ts->input, ABS_PRESSURE, CY_NTCH);
		input_report_key(ts->input, BTN_TOUCH, CY_NTCH);
		input_report_key(ts->input, BTN_2, CY_NTCH);
	}
	/* update platform data for the current single touch info */
	ts->prv_st_tch[CY_ST_FNGR1_IDX] = t->cur_st_tch[CY_ST_FNGR1_IDX];
	ts->prv_st_tch[CY_ST_FNGR2_IDX] = t->cur_st_tch[CY_ST_FNGR2_IDX];

}

static void handle_multi_touch(struct cyttsp_track_data *t, struct cyttsp *ts)
{

	u8 id;
	u8 i, loc;
	void (*mt_sync_func)(struct input_dev *) = ts->platform_data->mt_sync;

	if (!ts->platform_data->use_trk_id)
		goto no_track_id;

	/* terminate any previous touch where the track
	 * is missing from the current event */
	for (id = 0; id < CY_NUM_TRK_ID; id++) {
		if ((ts->act_trk[id] == CY_NTCH) || (t->cur_trk[id] != CY_NTCH))
			continue;

		input_report_abs(ts->input, ABS_MT_TRACKING_ID, id);
		input_report_abs(ts->input, ABS_MT_TOUCH_MAJOR, CY_NTCH);
		input_report_abs(ts->input, ABS_MT_WIDTH_MAJOR, t->tool_width);
		input_report_abs(ts->input, ABS_MT_POSITION_X,
					ts->prv_mt_pos[id][CY_XPOS]);
		input_report_abs(ts->input, ABS_MT_POSITION_Y,
					ts->prv_mt_pos[id][CY_YPOS]);
		if (mt_sync_func)
			mt_sync_func(ts->input);
		ts->act_trk[id] = CY_NTCH;
		ts->prv_mt_pos[id][CY_XPOS] = 0;
		ts->prv_mt_pos[id][CY_YPOS] = 0;
	}
	/* set Multi-Touch current event signals */
	for (id = 0; id < CY_NUM_MT_TCH_ID; id++) {
		if (t->cur_mt_tch[id] >= CY_NUM_TRK_ID)
			continue;

		input_report_abs(ts->input, ABS_MT_TRACKING_ID,
						t->cur_mt_tch[id]);
		input_report_abs(ts->input, ABS_MT_TOUCH_MAJOR,
						t->cur_mt_z[id]);
		input_report_abs(ts->input, ABS_MT_WIDTH_MAJOR,
						t->tool_width);
		input_report_abs(ts->input, ABS_MT_POSITION_X,
						t->cur_mt_pos[id][CY_XPOS]);
		input_report_abs(ts->input, ABS_MT_POSITION_Y,
						t->cur_mt_pos[id][CY_YPOS]);
		if (mt_sync_func)
			mt_sync_func(ts->input);

		ts->act_trk[id] = CY_TCH;
		ts->prv_mt_pos[id][CY_XPOS] = t->cur_mt_pos[id][CY_XPOS];
		ts->prv_mt_pos[id][CY_YPOS] = t->cur_mt_pos[id][CY_YPOS];
	}
	return;
no_track_id:

	/* set temporary track array elements to voids */
	memset(t->tmp_trk, CY_IGNR_TCH, sizeof(t->tmp_trk));
	memset(t->snd_trk, CY_IGNR_TCH, sizeof(t->snd_trk));

	/* get what is currently active */
	for (i = id = 0; id < CY_NUM_TRK_ID && i < CY_NUM_MT_TCH_ID; id++) {
		if (t->cur_trk[id] == CY_TCH) {
			/* only incr counter if track found */
			t->tmp_trk[i] = id;
			i++;
		}
	}
	DBG(printk(KERN_INFO"%s: T1: t0=%d, t1=%d, t2=%d, t3=%d\n", __func__,
					t->tmp_trk[0], t->tmp_trk[1],
					t->tmp_trk[2], t->tmp_trk[3]);)
	DBG(printk(KERN_INFO"%s: T1: p0=%d, p1=%d, p2=%d, p3=%d\n", __func__,
					ts->prv_mt_tch[0], ts->prv_mt_tch[1],
					ts->prv_mt_tch[2], ts->prv_mt_tch[3]);)

	/* pack in still active previous touches */
	for (id = t->prv_tch = 0; id < CY_NUM_MT_TCH_ID; id++) {
		if (t->tmp_trk[id] >= CY_NUM_TRK_ID)
			continue;

		if (cyttsp_inlist(ts->prv_mt_tch, t->tmp_trk[id], &loc,
							CY_NUM_MT_TCH_ID)) {
			loc &= CY_NUM_MT_TCH_ID - 1;
			t->snd_trk[loc] = t->tmp_trk[id];
			t->prv_tch++;
			DBG(printk(KERN_INFO"%s: in list s[%d]=%d "
					"t[%d]=%d, loc=%d p=%d\n", __func__,
					loc, t->snd_trk[loc],
					id, t->tmp_trk[id],
					loc, t->prv_tch);)
		} else {
			DBG(printk(KERN_INFO"%s: is not in list s[%d]=%d"
					" t[%d]=%d loc=%d\n", __func__,
					id, t->snd_trk[id],
					id, t->tmp_trk[id],
					loc);)
		}
	}
	DBG(printk(KERN_INFO"%s: S1: s0=%d, s1=%d, s2=%d, s3=%d p=%d\n",
		   __func__,
		   t->snd_trk[0], t->snd_trk[1], t->snd_trk[2],
		   t->snd_trk[3], t->prv_tch);)

	/* pack in new touches */
	for (id = 0; id < CY_NUM_MT_TCH_ID; id++) {
		if (t->tmp_trk[id] >= CY_NUM_TRK_ID)
			continue;

		if (!cyttsp_inlist(t->snd_trk, t->tmp_trk[id], &loc,
							CY_NUM_MT_TCH_ID)) {

			DBG(
			printk(KERN_INFO"%s: not in list t[%d]=%d, loc=%d\n",
				   __func__,
				   id, t->tmp_trk[id], loc);)

			if (cyttsp_next_avail_inlist(t->snd_trk, &loc,
							CY_NUM_MT_TCH_ID)) {
				loc &= CY_NUM_MT_TCH_ID - 1;
				t->snd_trk[loc] = t->tmp_trk[id];
				DBG(printk(KERN_INFO "%s: put in list s[%d]=%d"
					" t[%d]=%d\n", __func__,
					loc,
					t->snd_trk[loc], id, t->tmp_trk[id]);
				    )
			}
		} else {
			DBG(printk(KERN_INFO"%s: is in list s[%d]=%d "
				"t[%d]=%d loc=%d\n", __func__,
				id, t->snd_trk[id], id, t->tmp_trk[id], loc);)
		}
	}
	DBG(printk(KERN_INFO"%s: S2: s0=%d, s1=%d, s2=%d, s3=%d\n", __func__,
			t->snd_trk[0], t->snd_trk[1],
			t->snd_trk[2], t->snd_trk[3]);)

	/* sync motion event signals for each current touch */
	for (id = 0; id < CY_NUM_MT_TCH_ID; id++) {
		/* z will either be 0 (NOTOUCH) or
		 * some pressure (TOUCH)
		 */
		DBG(printk(KERN_INFO "%s: MT0 prev[%d]=%d "
				"temp[%d]=%d send[%d]=%d\n",
				__func__, id, ts->prv_mt_tch[id],
				id, t->tmp_trk[id], id, t->snd_trk[id]);)

		if (t->snd_trk[id] < CY_NUM_TRK_ID) {
			input_report_abs(ts->input, ABS_MT_TOUCH_MAJOR,
					t->cur_mt_z[t->snd_trk[id]]);
			input_report_abs(ts->input, ABS_MT_WIDTH_MAJOR,
					t->tool_width);
			input_report_abs(ts->input, ABS_MT_POSITION_X,
					t->cur_mt_pos[t->snd_trk[id]][CY_XPOS]);
			input_report_abs(ts->input, ABS_MT_POSITION_Y,
					t->cur_mt_pos[t->snd_trk[id]][CY_YPOS]);
			if (mt_sync_func)
				mt_sync_func(ts->input);

			DBG2(printk(KERN_INFO"%s: MT1 -> TID:"
				"%3d X:%3d  Y:%3d  Z:%3d\n", __func__,
				t->snd_trk[id],
				t->cur_mt_pos[t->snd_trk[id]][CY_XPOS],
				t->cur_mt_pos[t->snd_trk[id]][CY_YPOS],
				t->cur_mt_z[t->snd_trk[id]]);)

		} else if (ts->prv_mt_tch[id] < CY_NUM_TRK_ID) {
			/* void out this touch */
			input_report_abs(ts->input, ABS_MT_TOUCH_MAJOR,
							CY_NTCH);
			input_report_abs(ts->input, ABS_MT_WIDTH_MAJOR,
							t->tool_width);
			input_report_abs(ts->input, ABS_MT_POSITION_X,
				ts->prv_mt_pos[ts->prv_mt_tch[id]][CY_XPOS]);
			input_report_abs(ts->input, ABS_MT_POSITION_Y,
				ts->prv_mt_pos[ts->prv_mt_tch[id]][CY_YPOS]);

			if (mt_sync_func)
				mt_sync_func(ts->input);

			DBG2(printk(KERN_INFO"%s: "
				"MT2->TID:%2d X:%3d Y:%3d Z:%3d liftoff-sent\n",
				__func__, ts->prv_mt_tch[id],
				ts->prv_mt_pos[ts->prv_mt_tch[id]][CY_XPOS],
				ts->prv_mt_pos[ts->prv_mt_tch[id]][CY_YPOS],
				CY_NTCH);)
		} else {
			/* do not stuff any signals for this
			 * previously and currently void touches
			 */
			DBG(printk(KERN_INFO"%s: "
				"MT3->send[%d]=%d - No touch - NOT sent\n",
				__func__, id, t->snd_trk[id]);)
		}
	}

	/* save current posted tracks to
	 * previous track memory */
	for (id = 0; id < CY_NUM_MT_TCH_ID; id++) {
		ts->prv_mt_tch[id] = t->snd_trk[id];
		if (t->snd_trk[id] < CY_NUM_TRK_ID) {
			ts->prv_mt_pos[t->snd_trk[id]][CY_XPOS] =
					t->cur_mt_pos[t->snd_trk[id]][CY_XPOS];
			ts->prv_mt_pos[t->snd_trk[id]][CY_YPOS] =
					t->cur_mt_pos[t->snd_trk[id]][CY_YPOS];
			DBG(printk(KERN_INFO"%s: "
				"MT4->TID:%2d X:%3d Y:%3d Z:%3d save for prv\n",
				__func__, t->snd_trk[id],
				ts->prv_mt_pos[t->snd_trk[id]][CY_XPOS],
				ts->prv_mt_pos[t->snd_trk[id]][CY_YPOS],
				CY_NTCH);)
		}
	}
	memset(ts->act_trk, CY_NTCH, sizeof(ts->act_trk));
	for (id = 0; id < CY_NUM_MT_TCH_ID; id++) {
		if (t->snd_trk[id] < CY_NUM_TRK_ID)
			ts->act_trk[t->snd_trk[id]] = CY_TCH;
	}
}

static void cyttsp_xy_worker(struct cyttsp *ts)
{
	struct cyttsp_xydata xy_data;
	u8 id, tilt, rev_x, rev_y;
	struct cyttsp_track_data trc;
	s32 retval;

	DBG(printk(KERN_INFO "%s: Enter\n", __func__);)
	/* get event data from CYTTSP device */
	retval = ttsp_read_block_data(ts, CY_REG_BASE,
				      sizeof(xy_data), &xy_data);

	/*printk("xym: xy_data.host_mode=0x%x \n", xy_data.hst_mode);
	printk("xym: xy_data.tt_mode=0x%x\n", xy_data.tt_mode);
	printk("xym: xy_data.tt_stat=0x%x\n", xy_data.tt_stat);
	printk("xym: xy_data.x1=0x%x\n", xy_data.x1);
	printk("xym: xy_data.y1=0x%x\n", xy_data.y1);
	printk("xym: xy_data.x2=0x%x\n", xy_data.x2);
	printk("xym: xy_data.y2=0x%x\n", xy_data.y2);*/
	
	if (retval < 0) {
		printk(KERN_ERR "%s: Error, fail to read device on host interface bus\n",
			__func__);
		goto exit_xy_worker;
	}

	if (CY_MODE_ERROR(ts->platform_data->power_state,
		xy_data.hst_mode, xy_data.tt_mode)) {
		/* TTSP device has switched to non-operational mode */
		printk(KERN_ERR "%s: Error, bad mode ps=%d hm=%02X tm=%02X\n",
			__func__, ts->platform_data->power_state,
			xy_data.hst_mode, xy_data.tt_mode);
		retval = cyttsp_power_on(ts);
		if (retval < 0)
			printk(KERN_ERR "%s: Error, power on fail\n", __func__);
		goto exit_xy_worker;
	}

	/* touch extension handling */
	retval = ttsp_tch_ext(ts, &xy_data);

	if (retval < 0) {
		printk(KERN_ERR "%s: Error, touch extension handling\n",
			__func__);
		goto exit_xy_worker;
	} else if (retval > 0) {
		DBG(printk(KERN_INFO "%s: Touch extension handled\n",
			__func__);)
		goto exit_xy_worker;
	}

	/* provide flow control handshake */
	if (ts->irq) {
		if (ts->platform_data->use_hndshk) {
			u8 cmd;
			cmd = xy_data.hst_mode & CY_HNDSHK_BIT ?
				xy_data.hst_mode & ~CY_HNDSHK_BIT :
				xy_data.hst_mode | CY_HNDSHK_BIT;
			retval = ttsp_write_block_data(ts, CY_REG_BASE,
						       sizeof(cmd), (u8 *)&cmd);
		}
	}
	trc.cur_tch = GET_NUM_TOUCHES(xy_data.tt_stat);
	//printk("xym: trc.cur_tch=0x%x\n", trc.cur_tch);

	if (IS_LARGE_AREA(xy_data.tt_stat) == 1) {
		/* terminate all active tracks */
		trc.cur_tch = CY_NTCH;
		DBG(printk(KERN_INFO "%s: Large area detected\n",
			__func__);)
	} else if (trc.cur_tch > CY_MAX_TCH) {
		/* terminate all active tracks */
		trc.cur_tch = CY_NTCH;
		DBG(printk(KERN_INFO "%s: Num touch error detected\n",
			__func__);)
	} else if (IS_BAD_PKT(xy_data.tt_mode)) {
		/* terminate all active tracks */
		trc.cur_tch = CY_NTCH;
		DBG(printk(KERN_INFO "%s: Invalid buffer detected\n",
			__func__);)
	}

	/* set tool size */
	trc.tool_width = CY_SMALL_TOOL_WIDTH;

	if (ts->platform_data->gen == CY_GEN2) {
		/* translate Gen2 interface data into comparable Gen3 data */
		trc.cur_tch = ttsp_convert_gen2(trc.cur_tch, &xy_data);
	}

	/* clear current active track ID array and count previous touches */
	for (id = 0, trc.prv_tch = CY_NTCH; id < CY_NUM_TRK_ID; id++) {
		trc.cur_trk[id] = CY_NTCH;
		trc.prv_tch += ts->act_trk[id];
	}

	/*
	 * send no events if there were no
	 * previous touches and no new touches
	 */
	if ((trc.prv_tch == CY_NTCH) && ((trc.cur_tch == CY_NTCH) ||
				(trc.cur_tch > CY_MAX_TCH))) {
		if (++ts->ntch_count > CY_MAX_NTCH) {
			/* TTSP device has a stuck operational mode */
			printk(KERN_ERR "%s: Error, stuck no-touch ct=%d\n",
				__func__, trc.cur_tch);
			retval = cyttsp_power_on(ts);
			if (retval < 0)
				printk(KERN_ERR "%s: Error, power on fail\n",
					__func__);
			goto exit_xy_worker;
		}
	} else
		ts->ntch_count = 0;

	DBG(printk(KERN_INFO "%s: prev=%d  curr=%d\n", __func__,
		   trc.prv_tch, trc.cur_tch);)

	/* clear current single-touch array */
	memset(trc.cur_st_tch, CY_IGNR_TCH, sizeof(trc.cur_st_tch));

	/* clear single touch positions */
	trc.st_x1 = trc.st_y1 = trc.st_z1 =
			trc.st_x2 = trc.st_y2 = trc.st_z2 = CY_NTCH;

	/* clear current multi-touch arrays */
	memset(trc.cur_mt_tch, CY_IGNR_TCH, sizeof(trc.cur_mt_tch));
	memset(trc.cur_mt_pos, CY_NTCH, sizeof(trc.cur_mt_pos));
	memset(trc.cur_mt_z, CY_NTCH, sizeof(trc.cur_mt_z));

	DBG(
	if (trc.cur_tch) {
		unsigned i;
		u8 *pdata = (u8 *)&xy_data;

		printk(KERN_INFO "%s: TTSP data_pack: ", __func__);
		for (i = 0; i < sizeof(struct cyttsp_xydata); i++)
			printk(KERN_INFO "[%d]=0x%x ", i, pdata[i]);
		printk(KERN_INFO "\n");
	})

	/* Determine if display is tilted */
	tilt = !!FLIP_DATA(ts->platform_data->flags);
	/* Check for switch in origin */
	rev_x = !!REVERSE_X(ts->platform_data->flags);
	rev_y = !!REVERSE_Y(ts->platform_data->flags);

	/* process the touches */
	switch (trc.cur_tch) {
	case 4:
		xy_data.x4 = be16_to_cpu(xy_data.x4);
		xy_data.y4 = be16_to_cpu(xy_data.y4);
		if (tilt)
			FLIP_XY(xy_data.x4, xy_data.y4);

		if (rev_x)
			xy_data.x4 = INVERT_X(xy_data.x4,
					ts->platform_data->maxx);
		if (rev_y)
			xy_data.y4 = INVERT_X(xy_data.y4,
					ts->platform_data->maxy);

		id = GET_TOUCH4_ID(xy_data.touch34_id);
		if (ts->platform_data->use_trk_id) {
			trc.cur_mt_pos[CY_MT_TCH4_IDX][CY_XPOS] = xy_data.x4;
			trc.cur_mt_pos[CY_MT_TCH4_IDX][CY_YPOS] = xy_data.y4;
			trc.cur_mt_z[CY_MT_TCH4_IDX] = xy_data.z4;
		} else {
			trc.cur_mt_pos[id][CY_XPOS] = xy_data.x4;
			trc.cur_mt_pos[id][CY_YPOS] = xy_data.y4;
			trc.cur_mt_z[id] = xy_data.z4;
		}
		trc.cur_mt_tch[CY_MT_TCH4_IDX] = id;
		trc.cur_trk[id] = CY_TCH;
		if (ts->prv_st_tch[CY_ST_FNGR1_IDX] <	CY_NUM_TRK_ID) {
			if (ts->prv_st_tch[CY_ST_FNGR1_IDX] == id) {
				trc.st_x1 = xy_data.x4;
				trc.st_y1 = xy_data.y4;
				trc.st_z1 = xy_data.z4;
				trc.cur_st_tch[CY_ST_FNGR1_IDX] = id;
			} else if (ts->prv_st_tch[CY_ST_FNGR2_IDX] == id) {
				trc.st_x2 = xy_data.x4;
				trc.st_y2 = xy_data.y4;
				trc.st_z2 = xy_data.z4;
				trc.cur_st_tch[CY_ST_FNGR2_IDX] = id;
			}
		}
		DBG(printk(KERN_INFO"%s: 4th XYZ:% 3d,% 3d,% 3d  ID:% 2d\n\n",
				__func__, xy_data.x4, xy_data.y4, xy_data.z4,
				(xy_data.touch34_id & 0x0F));)

		/* do not break */
	case 3:
		xy_data.x3 = be16_to_cpu(xy_data.x3);
		xy_data.y3 = be16_to_cpu(xy_data.y3);
		if (tilt)
			FLIP_XY(xy_data.x3, xy_data.y3);

		if (rev_x)
			xy_data.x3 = INVERT_X(xy_data.x3,
					ts->platform_data->maxx);
		if (rev_y)
			xy_data.y3 = INVERT_X(xy_data.y3,
					ts->platform_data->maxy);

		id = GET_TOUCH3_ID(xy_data.touch34_id);
		if (ts->platform_data->use_trk_id) {
			trc.cur_mt_pos[CY_MT_TCH3_IDX][CY_XPOS] = xy_data.x3;
			trc.cur_mt_pos[CY_MT_TCH3_IDX][CY_YPOS] = xy_data.y3;
			trc.cur_mt_z[CY_MT_TCH3_IDX] = xy_data.z3;
		} else {
			trc.cur_mt_pos[id][CY_XPOS] = xy_data.x3;
			trc.cur_mt_pos[id][CY_YPOS] = xy_data.y3;
			trc.cur_mt_z[id] = xy_data.z3;
		}
		trc.cur_mt_tch[CY_MT_TCH3_IDX] = id;
		trc.cur_trk[id] = CY_TCH;
		if (ts->prv_st_tch[CY_ST_FNGR1_IDX] < CY_NUM_TRK_ID) {
			if (ts->prv_st_tch[CY_ST_FNGR1_IDX] == id) {
				trc.st_x1 = xy_data.x3;
				trc.st_y1 = xy_data.y3;
				trc.st_z1 = xy_data.z3;
				trc.cur_st_tch[CY_ST_FNGR1_IDX] = id;
			} else if (ts->prv_st_tch[CY_ST_FNGR2_IDX] == id) {
				trc.st_x2 = xy_data.x3;
				trc.st_y2 = xy_data.y3;
				trc.st_z2 = xy_data.z3;
				trc.cur_st_tch[CY_ST_FNGR2_IDX] = id;
			}
		}
		DBG(printk(KERN_INFO"%s: 3rd XYZ:% 3d,% 3d,% 3d  ID:% 2d\n",
			__func__, xy_data.x3, xy_data.y3, xy_data.z3,
			((xy_data.touch34_id >> 4) & 0x0F));)

		/* do not break */
	case 2:
		xy_data.x2 = be16_to_cpu(xy_data.x2);
		xy_data.y2 = be16_to_cpu(xy_data.y2);
		if (tilt)
			FLIP_XY(xy_data.x2, xy_data.y2);

		if (rev_x)
			xy_data.x2 = INVERT_X(xy_data.x2,
					ts->platform_data->maxx);
		if (rev_y)
			xy_data.y2 = INVERT_X(xy_data.y2,
					ts->platform_data->maxy);
		id = GET_TOUCH2_ID(xy_data.touch12_id);
		if (ts->platform_data->use_trk_id) {
			trc.cur_mt_pos[CY_MT_TCH2_IDX][CY_XPOS] = xy_data.x2;
			trc.cur_mt_pos[CY_MT_TCH2_IDX][CY_YPOS] = xy_data.y2;
			trc.cur_mt_z[CY_MT_TCH2_IDX] = xy_data.z2;
		} else {
			trc.cur_mt_pos[id][CY_XPOS] = xy_data.x2;
			trc.cur_mt_pos[id][CY_YPOS] = xy_data.y2;
			trc.cur_mt_z[id] = xy_data.z2;
		}
		trc.cur_mt_tch[CY_MT_TCH2_IDX] = id;
		trc.cur_trk[id] = CY_TCH;
		if (ts->prv_st_tch[CY_ST_FNGR1_IDX] <	CY_NUM_TRK_ID) {
			if (ts->prv_st_tch[CY_ST_FNGR1_IDX] == id) {
				trc.st_x1 = xy_data.x2;
				trc.st_y1 = xy_data.y2;
				trc.st_z1 = xy_data.z2;
				trc.cur_st_tch[CY_ST_FNGR1_IDX] = id;
			} else if (ts->prv_st_tch[CY_ST_FNGR2_IDX] == id) {
				trc.st_x2 = xy_data.x2;
				trc.st_y2 = xy_data.y2;
				trc.st_z2 = xy_data.z2;
				trc.cur_st_tch[CY_ST_FNGR2_IDX] = id;
			}
		}
		DBG(printk(KERN_INFO"%s: 2nd XYZ:% 3d,% 3d,% 3d  ID:% 2d\n",
				__func__, xy_data.x2, xy_data.y2, xy_data.z2,
				(xy_data.touch12_id & 0x0F));)

		/* do not break */
	case 1:
		xy_data.x1 = be16_to_cpu(xy_data.x1);
		xy_data.y1 = be16_to_cpu(xy_data.y1);
		if (tilt)
			FLIP_XY(xy_data.x1, xy_data.y1);

		if (rev_x)
			xy_data.x1 = INVERT_X(xy_data.x1,
					ts->platform_data->maxx);
		if (rev_y)
			xy_data.y1 = INVERT_X(xy_data.y1,
					ts->platform_data->maxy);

		id = GET_TOUCH1_ID(xy_data.touch12_id);
		if (ts->platform_data->use_trk_id) {
			trc.cur_mt_pos[CY_MT_TCH1_IDX][CY_XPOS] = xy_data.x1;
			trc.cur_mt_pos[CY_MT_TCH1_IDX][CY_YPOS] = xy_data.y1;
			trc.cur_mt_z[CY_MT_TCH1_IDX] = xy_data.z1;
		} else {
			trc.cur_mt_pos[id][CY_XPOS] = xy_data.x1;
			trc.cur_mt_pos[id][CY_YPOS] = xy_data.y1;
			trc.cur_mt_z[id] = xy_data.z1;
		}
		trc.cur_mt_tch[CY_MT_TCH1_IDX] = id;
		trc.cur_trk[id] = CY_TCH;
		if (ts->prv_st_tch[CY_ST_FNGR1_IDX] <	CY_NUM_TRK_ID) {
			if (ts->prv_st_tch[CY_ST_FNGR1_IDX] == id) {
				trc.st_x1 = xy_data.x1;
				trc.st_y1 = xy_data.y1;
				trc.st_z1 = xy_data.z1;
				trc.cur_st_tch[CY_ST_FNGR1_IDX] = id;
			} else if (ts->prv_st_tch[CY_ST_FNGR2_IDX] == id) {
				trc.st_x2 = xy_data.x1;
				trc.st_y2 = xy_data.y1;
				trc.st_z2 = xy_data.z1;
				trc.cur_st_tch[CY_ST_FNGR2_IDX] = id;
			}
		}
		DBG(printk(KERN_INFO"%s: S1st XYZ:% 3d,% 3d,% 3d  ID:% 2d\n",
				__func__, xy_data.x1, xy_data.y1, xy_data.z1,
				((xy_data.touch12_id >> 4) & 0x0F));)

		break;
	case 0:
	default:
		break;
	}

	if (ts->platform_data->use_st)
		handle_single_touch(&xy_data, &trc, ts);

	if (ts->platform_data->use_mt)
		handle_multi_touch(&trc, ts);

	/* handle gestures */
	if (ts->platform_data->use_gestures && xy_data.gest_id) {
		input_report_key(ts->input, BTN_3, CY_TCH);
		input_report_abs(ts->input, ABS_HAT1X, xy_data.gest_id);
		input_report_abs(ts->input, ABS_HAT1Y, xy_data.gest_cnt);
	}

	/* signal the view motion event */
	input_sync(ts->input);

	/* update platform data for the current multi-touch information */
	memcpy(ts->act_trk, trc.cur_trk, CY_NUM_TRK_ID);

exit_xy_worker:
	/* reset the watchdog */
	mod_timer(&ts->timer, jiffies + CY_WDG_TIMEOUT);
	DBG(printk(KERN_INFO"%s: finished.\n", __func__);)
	return;
}

static irqreturn_t cyttsp_irq(int irq, void *handle)
{
	struct cyttsp *ts = (struct cyttsp *)handle;

	DBG(printk(KERN_INFO"%s: Got IRQ!\n", __func__);)
	switch (ts->platform_data->power_state) {
	case CY_BL_STATE:
	case CY_SYSINFO_STATE:
		complete(&ts->int_running);
		break;
	case CY_LDR_STATE:
		ts->bl_ready = true;
		break;
	case CY_ACTIVE_STATE:
		cyttsp_xy_worker(ts);
		break;
	case CY_READY_STATE:
	default:
		break;
	}

	return IRQ_HANDLED;
}

/* Timer function used as watchdog */
static void cyttsp_timer(unsigned long handle)
{
	struct cyttsp *ts = (struct cyttsp *)handle;

	DBG(printk(KERN_INFO"%s: Watchdog timer event\n", __func__);)
	if (!work_pending(&ts->work))
		schedule_work(&ts->work);
	return;
}
/*
 ************************************************************************
 * Probe initialization functions
 ************************************************************************
 */
static int cyttsp_load_bl_regs(struct cyttsp *ts)
{
	int retval;

	DBG(printk(KERN_INFO"%s: Enter\n", __func__);)

	memset(&ts->bl_data, 0, sizeof(ts->bl_data));

	retval =  ttsp_read_block_data(ts, CY_REG_BASE,
				sizeof(ts->bl_data), &(ts->bl_data));

	if (retval < 0) {
		DBG2(printk(KERN_INFO "%s: Failed reading block data, err:%d\n",
			__func__, retval);)
		goto fail;
	}

	DBG({
	      int i;
	      u8 *bl_data = (u8 *)&(ts->bl_data);
	      for (i = 0; i < sizeof(struct cyttsp_bootloader_data); i++)
			printk(KERN_INFO "%s bl_data[%d]=0x%x\n",
				__func__, i, bl_data[i]);
	})
	DBG2(printk(KERN_INFO "%s: bl f=%02X s=%02X e=%02X\n",
		__func__,
		ts->bl_data.bl_file,
		ts->bl_data.bl_status,
		ts->bl_data.bl_error);)

	return 0;
fail:
	return retval;
}

static bool cyttsp_bl_app_valid(struct cyttsp *ts)
{
	int retval;

	ts->bl_data.bl_status = 0x00;

	retval = cyttsp_load_bl_regs(ts);

	if (retval < 0)
		return false;

	if (ts->bl_data.bl_status == 0x11) {
		printk(KERN_INFO "%s: App found; normal boot\n", __func__);
		return true;
	} else if (ts->bl_data.bl_status == 0x10) {
		printk(KERN_INFO "%s: NO APP; load firmware!!\n", __func__);
		return false;
	} else {
		if (ts->bl_data.bl_file == CY_OPERATE_MODE) {
			int invalid_op_mode_status;
			invalid_op_mode_status = ts->bl_data.bl_status & 0x3f;
			if (invalid_op_mode_status)
				return false;
			else {
				if (ts->platform_data->power_state ==
					CY_ACTIVE_STATE)
					printk(KERN_INFO "%s: Operational\n",
						__func__);
				else
					printk(KERN_INFO "%s: No bootloader\n",
						__func__);
			}
		}
		return true;
	}
}

static int cyttsp_exit_bl_mode(struct cyttsp *ts)
{
	int retval;
	int tries = 0;

	DBG(printk(KERN_INFO"%s: Enter\n", __func__);)

	/* check if in bootloader mode;
	 * if in operational mode then just return without fail
	 */
	cyttsp_load_bl_regs(ts);
	if (!GET_BOOTLOADERMODE(ts->bl_data.bl_status))
		return 0;

	retval = ttsp_write_block_data(ts, CY_REG_BASE, sizeof(bl_cmd),
				       (void *)bl_cmd);
	if (retval < 0) {
		printk(KERN_ERR "%s: Failed writing block data, err:%d\n",
			__func__, retval);
		goto fail;
	}
	do {
		msleep(20);
		cyttsp_load_bl_regs(ts);
	} while (GET_BOOTLOADERMODE(ts->bl_data.bl_status) && tries++ < 10);

	DBG2(printk(KERN_INFO "%s: read tries=%d\n", __func__, tries);)

	DBG(printk(KERN_INFO"%s: Exit "
				"f=%02X s=%02X e=%02X\n",
				__func__,
				ts->bl_data.bl_file, ts->bl_data.bl_status,
				ts->bl_data.bl_error);)

	return 0;
fail:
	return retval;
}

static int cyttsp_set_sysinfo_mode(struct cyttsp *ts)
{
	int retval;
	u8 cmd = CY_SYSINFO_MODE;

	DBG(printk(KERN_INFO"%s: Enter\n", __func__);)
	memset(&(ts->sysinfo_data), 0, sizeof(struct cyttsp_sysinfo_data));

	/* switch to sysinfo mode */
	retval = ttsp_write_block_data(ts, CY_REG_BASE, sizeof(cmd), &cmd);
	if (retval < 0) {
		printk(KERN_ERR "%s: Failed writing block data, err:%d\n",
			__func__, retval);
		goto exit_sysinfo_mode;
	}

	if (!(retval < 0)) {
		/* wait for interrupt to set ready completion */
		ts->platform_data->power_state = CY_SYSINFO_STATE;
		INIT_COMPLETION(ts->int_running);
		retval = wait_for_completion_interruptible_timeout(
			&ts->int_running,
			msecs_to_jiffies(CY_DELAY_DFLT * CY_DELAY_MAX));
		ts->platform_data->power_state = CY_READY_STATE;
	}

	if (retval < 0) {
		ts->platform_data->power_state = CY_IDLE_STATE;
		printk(KERN_ERR "%s: reset timeout fail (ret=%d)\n",
			__func__, retval);
	}

exit_sysinfo_mode:
	/* read sysinfo registers */
	retval = ttsp_read_block_data(ts, CY_REG_BASE,
		sizeof(ts->sysinfo_data), &(ts->sysinfo_data));

	DBG(printk(KERN_INFO"%s:SI2: hst_mode=0x%02X mfg_cmd=0x%02X "
		"mfg_stat=0x%02X\n", __func__, ts->sysinfo_data.hst_mode,
		ts->sysinfo_data.mfg_cmd,
		ts->sysinfo_data.mfg_stat);)

	DBG(printk(KERN_INFO"%s:SI2: bl_ver=0x%02X%02X\n",
		__func__, ts->sysinfo_data.bl_verh, ts->sysinfo_data.bl_verl);)

	DBG(printk(KERN_INFO"%s:SI2: sysinfo act_intrvl=0x%02X "
		"tch_tmout=0x%02X lp_intrvl=0x%02X\n",
		__func__, ts->sysinfo_data.act_intrvl,
		ts->sysinfo_data.tch_tmout,
		ts->sysinfo_data.lp_intrvl);)

	printk(KERN_INFO"%s:SI2:tts_ver=%02X%02X app_id=%02X%02X "
		"app_ver=%02X%02X c_id=%02X%02X%02X "
		"u_id=%02X%02X%02X%02X%02X%02X%02X%02X\n",
		__func__,
		ts->sysinfo_data.tts_verh, ts->sysinfo_data.tts_verl,
		ts->sysinfo_data.app_idh, ts->sysinfo_data.app_idl,
		ts->sysinfo_data.app_verh, ts->sysinfo_data.app_verl,
		ts->sysinfo_data.cid[0], ts->sysinfo_data.cid[1],
		ts->sysinfo_data.cid[2],
		ts->sysinfo_data.uid[0], ts->sysinfo_data.uid[1],
		ts->sysinfo_data.uid[2], ts->sysinfo_data.uid[3],
		ts->sysinfo_data.uid[4], ts->sysinfo_data.uid[5],
		ts->sysinfo_data.uid[6], ts->sysinfo_data.uid[7]);

	return retval;
}

static int cyttsp_set_sysinfo_regs(struct cyttsp *ts)
{
	int retval = 0;

	if (ts->platform_data->scn_typ != CY_SCN_TYP_DFLT) {
		u8 scn_typ = ts->platform_data->scn_typ;

		retval = ttsp_write_block_data(ts,
				CY_REG_SCN_TYP,
				sizeof(scn_typ), &scn_typ);
	}

	if (retval < 0)
		return retval;

	if ((ts->platform_data->act_intrvl != CY_ACT_INTRVL_DFLT) ||
		(ts->platform_data->tch_tmout != CY_TCH_TMOUT_DFLT) ||
		(ts->platform_data->lp_intrvl != CY_LP_INTRVL_DFLT)) {

		u8 intrvl_ray[3];

		intrvl_ray[0] = ts->platform_data->act_intrvl;
		intrvl_ray[1] = ts->platform_data->tch_tmout;
		intrvl_ray[2] = ts->platform_data->lp_intrvl;

		/* set intrvl registers */
		retval = ttsp_write_block_data(ts,
				CY_REG_ACT_INTRVL,
				sizeof(intrvl_ray), intrvl_ray);

		msleep(CY_DELAY_SYSINFO);
	}

	return retval;
}

static int cyttsp_set_operational_mode(struct cyttsp *ts)
{
	int retval;
	int tries;
	u8 cmd = CY_OPERATE_MODE;
	u8 gest_default;
	struct cyttsp_xydata xy_data;

	DBG(printk(KERN_INFO"%s: Enter\n", __func__);)

	retval = ttsp_write_block_data(ts, CY_REG_BASE, sizeof(cmd), &cmd);
	if (retval < 0) {
		printk(KERN_ERR "%s: Failed writing block data, err:%d\n",
			__func__, retval);
		goto write_block_data_fail;
	}

	/* wait for TTSP Device to complete switch to Operational mode */
	msleep(20);

	tries = 0;
	gest_default =
		CY_GEST_GRP1 + CY_GEST_GRP2 +
		CY_GEST_GRP3 + CY_GEST_GRP4 +
		CY_ACT_DIST_DFLT;
	tries = 0;
	do {
		msleep(CY_DELAY_DFLT);
		memset(&xy_data, 0, sizeof(xy_data));
		retval = ttsp_read_block_data(ts, CY_REG_BASE,
			sizeof(struct cyttsp_xydata), &xy_data);
	} while (!((retval == 0) &&
		((xy_data.gest_set & CY_ACT_DIST_BITS) ==
		(CY_ACT_DIST_DFLT & CY_ACT_DIST_BITS))) &&
		(tries++ < CY_DELAY_MAX));

	DBG2(printk(KERN_INFO "%s: read tries=%d\n", __func__, tries);)

	return 0;
write_block_data_fail:
	return retval;
}

static int cyttsp_soft_reset(struct cyttsp *ts)
{
	int retval;
	u8 cmd = CY_SOFT_RESET_MODE;

	DBG(printk(KERN_INFO"%s: Enter\n", __func__);)

	/* reset TTSP Device back to bootloader mode */
	ts->platform_data->power_state = CY_BL_STATE;
	retval = ttsp_write_block_data(ts, CY_REG_BASE, sizeof(cmd), &cmd);

	if (!(retval < 0)) {
		/* wait for interrupt to set ready completion */
		INIT_COMPLETION(ts->int_running);
		retval = wait_for_completion_interruptible_timeout(
			&ts->int_running,
			msecs_to_jiffies(CY_DELAY_DFLT * CY_DELAY_MAX));
	}

	if (retval < 0) {
		ts->platform_data->power_state = CY_IDLE_STATE;
		printk(KERN_ERR "%s: reset timeout fail (ret=%d)\n",
			__func__, retval);
	}

	if (retval > 0)
		retval = 0;

	return retval;
}

static int cyttsp_gesture_setup(struct cyttsp *ts)
{
	int retval;
	u8 gesture_setup;

	DBG(printk(KERN_INFO"%s: Init gesture; active distance setup\n",
		__func__);)

	gesture_setup = ts->platform_data->gest_set;
	retval = ttsp_write_block_data(ts, CY_REG_GEST_SET,
		sizeof(gesture_setup), &gesture_setup);

	return retval;
}

#ifdef CY_INCLUDE_LOAD_RECS
#include "cyttsp_ldr.h"
#else
static bool cyttsp_bl_status(struct cyttsp *ts)
{
	DBG(printk(KERN_INFO"%s: Enter\n", __func__);)
	return ((ts->bl_data.bl_status == 0x10) ||
		(ts->bl_data.bl_status == 0x11));
}

static int cyttsp_loader(struct cyttsp *ts)
{
	void *fptr = cyttsp_bl_status;	/* kill warning */

	if (ts) {
		DBG(printk(KERN_INFO"%s: Enter\n", __func__);)
		DBG(printk(KERN_INFO"%s: Exit\n", __func__);)
	}

	if (!fptr)
		return -EIO;
	else
		return 0;
}
#endif

static int cyttsp_power_on(struct cyttsp *ts)
{
	int retval = 0;

	DBG(printk(KERN_INFO"%s: Enter\n", __func__);)

	/* (re-)start watchdog */
	mod_timer(&ts->timer, jiffies + CY_WDG_TIMEOUT);

	cyttsp_init_tch(ts);
	retval = cyttsp_soft_reset(ts);
	DBG2(printk(KERN_INFO"%s: after softreset r=%d\n", __func__, retval);)
	if (retval < 0)
		goto bypass;

	if (ts->platform_data->use_load_file)
		retval = cyttsp_loader(ts);

	if (!cyttsp_bl_app_valid(ts)) {
		retval = 1;
		goto bypass;
	}

	retval = cyttsp_exit_bl_mode(ts);
	DBG2(printk(KERN_INFO"%s: after exit bl r=%d\n", __func__, retval);)

	if (retval < 0)
		goto bypass;

	/* switch to System Information mode to read */
	/* versions and set interval registers */
	ts->platform_data->power_state = CY_READY_STATE;
	retval = cyttsp_set_sysinfo_mode(ts);
	if (retval < 0)
		goto bypass;

	retval = cyttsp_set_sysinfo_regs(ts);
	if (retval < 0)
		goto bypass;

	/* switch back to Operational mode */
	DBG2(printk(KERN_INFO"%s: switch back to operational mode\n",
		__func__);)
	retval = cyttsp_set_operational_mode(ts);
	if (retval < 0)
		goto bypass;

	/* init gesture setup; required for active distance */
	cyttsp_gesture_setup(ts);

bypass:
	if (retval)
		ts->platform_data->power_state = CY_IDLE_STATE;
	else
		ts->platform_data->power_state = CY_ACTIVE_STATE;

	printk(KERN_INFO"%s: Power state is %s\n",
		__func__, (ts->platform_data->power_state == CY_ACTIVE_STATE) ?
		"ACTIVE" : "IDLE");
	return retval;
}

static void cyttsp_check_bl(struct work_struct *work)
{
	struct cyttsp *ts = container_of(work, struct cyttsp, work);
	s32 retval;
	struct cyttsp_xydata xy_data;

	retval = ttsp_read_block_data(ts, CY_REG_BASE, 2, &xy_data);
	if (retval < 0) {
		printk(KERN_ERR "%s: Error, fail to read device\n", __func__);
		goto reserve_next;
	}

	if (CY_MODE_ERROR(ts->platform_data->power_state,
		xy_data.hst_mode, xy_data.tt_mode)) {
		printk(KERN_ERR "%s: Error, mode error detected "
			"on watchdog timeout ps=%d mode=%02X bl_mode=%02X\n",
			__func__, ts->platform_data->power_state,
			xy_data.hst_mode, xy_data.tt_mode);
		retval = cyttsp_power_on(ts);
		if (retval < 0)
			printk(KERN_ERR "%s: Error, power on fail\n", __func__);
	}

reserve_next:
	mod_timer(&ts->timer, jiffies + CY_WDG_TIMEOUT);
}

static int cyttsp_resume(struct cyttsp *ts)
{
	int retval = 0;
	struct cyttsp_xydata xydata;

	DBG(printk(KERN_INFO"%s: Enter\n", __func__);)
	if (ts->platform_data->use_sleep && (ts->platform_data->power_state !=
		CY_ACTIVE_STATE)) {
		if (ts->platform_data->wakeup) {
			retval = ts->platform_data->wakeup();
			if (retval < 0)
				printk(KERN_ERR "%s: Error, wakeup failed!\n",
					__func__);
		} else {
			printk(KERN_ERR "%s: Error, wakeup not implemented "
				"(check board file).\n", __func__);
			retval = -ENOSYS;
		}
		if (!(retval < 0)) {
			retval = ttsp_read_block_data(ts, CY_REG_BASE,
				sizeof(xydata), &xydata);
			if (!(retval < 0) && !GET_HSTMODE(xydata.hst_mode))
				ts->platform_data->power_state =
					CY_ACTIVE_STATE;
		}
	}
	DBG(printk(KERN_INFO"%s: Wake Up %s\n", __func__,
		(retval < 0) ? "FAIL" : "PASS");)
	return retval;
}

static int cyttsp_suspend(struct cyttsp *ts)
{
	u8 sleep_mode = 0;
	int retval = 0;

	DBG(printk(KERN_INFO"%s: Enter\n", __func__);)
	if (ts->platform_data->use_sleep &&
			(ts->platform_data->power_state == CY_ACTIVE_STATE)) {
		sleep_mode = CY_DEEP_SLEEP_MODE;
		retval = ttsp_write_block_data(ts,
			CY_REG_BASE, sizeof(sleep_mode), &sleep_mode);
		if (!(retval < 0))
			ts->platform_data->power_state = CY_SLEEP_STATE;
	}
	DBG(printk(KERN_INFO"%s: Sleep Power state is %s\n", __func__,
		(ts->platform_data->power_state == CY_ACTIVE_STATE) ?
		"ACTIVE" :
		((ts->platform_data->power_state == CY_SLEEP_STATE) ?
		"SLEEP" : "LOW POWER"));)
	return retval;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void cyttsp_ts_early_suspend(struct early_suspend *h)
{
	struct cyttsp *ts = container_of(h, struct cyttsp, early_suspend);

	DBG(printk(KERN_INFO"%s: Enter\n", __func__);)
	LOCK(ts->mutex);
	if (!ts->fw_loader_mode) {
		disable_irq_nosync(ts->irq);
		ts->suspended = 1;
		/* kill watchdog */
		del_timer(&ts->timer);
		cancel_work_sync(&ts->work);
		cyttsp_suspend(ts);
	}
	UNLOCK(ts->mutex);
}

static void cyttsp_ts_late_resume(struct early_suspend *h)
{
	struct cyttsp *ts = container_of(h, struct cyttsp, early_suspend);

	DBG(printk(KERN_INFO"%s: Enter\n", __func__);)
	LOCK(ts->mutex);
	if (!ts->fw_loader_mode && ts->suspended) {
		ts->suspended = 0;
		if (cyttsp_resume(ts) < 0)
			printk(KERN_ERR "%s: Error, cyttsp_resume.\n",
				__func__);
		enable_irq(ts->irq);
		/* resume watchdog */
		mod_timer(&ts->timer, jiffies + CY_WDG_TIMEOUT);
	}
	UNLOCK(ts->mutex);
}
#endif

static int cyttsp_wr_reg(struct cyttsp *ts, u8 reg_id, u8 reg_data)
{

	DBG(printk(KERN_INFO"%s: Enter\n", __func__);)

	return ttsp_write_block_data(ts,
		CY_REG_BASE + reg_id, sizeof(u8), &reg_data);
}

static int cyttsp_rd_reg(struct cyttsp *ts, u8 reg_id, u8 *reg_data)
{
	DBG(printk(KERN_INFO"%s: Enter\n", __func__);)
	return ttsp_read_block_data(ts,
		CY_REG_BASE + reg_id, sizeof(u8), reg_data);
}

static ssize_t firmware_write(struct file *pFile, struct kobject *kobj,
				struct bin_attribute *bin_attr,
				char *buf, loff_t pos, size_t size)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct cyttsp *ts = dev_get_drvdata(dev);
	LOCK(ts->mutex);
	if (!ts->fw_loader_mode) {
		unsigned short val = *(unsigned short *)buf;
		u8 reg_data = val & 0xff;
		ts->reg_id = (val & 0xff00) >> 8;
		if (!(ts->reg_id & 0x80)) {
			/* write user specified operational register */
			if (ts->reg_id == 0x00) {
				switch (GET_HSTMODE(reg_data)) {
				case GET_HSTMODE(CY_OPERATE_MODE):
					cyttsp_wr_reg(ts, ts->reg_id, reg_data);
					ts->platform_data->power_state =
						CY_ACTIVE_STATE;
					enable_irq(ts->irq);
					mod_timer(&ts->timer, CY_WDG_TIMEOUT);
					printk(KERN_INFO "%s: "
						"Switch to Operational Mode "
						"ps=%d\n", __func__,
						ts->platform_data->power_state);
					break;
				case GET_HSTMODE(CY_SYSINFO_MODE):
					ts->platform_data->power_state =
						CY_READY_STATE;
					disable_irq_nosync(ts->irq);
					/* kill watchdog */
					del_timer(&ts->timer);
					cancel_work_sync(&ts->work);
					cyttsp_wr_reg(ts, ts->reg_id, reg_data);
					printk(KERN_INFO "%s: "
						"Switch to SysInfo Mode "
						"ps=%d\n", __func__,
						ts->platform_data->power_state);
					break;
				default:
					cyttsp_wr_reg(ts, ts->reg_id, reg_data);
					break;
				}
			} else
				cyttsp_wr_reg(ts, ts->reg_id, reg_data);
			printk(KERN_INFO "%s: "
				"write(reg=%02X(%d) dat=0x%02X(%d))\n",
				__func__,
				ts->reg_id & ~0x80, ts->reg_id & ~0x80,
				reg_data,  reg_data);
		} else {
			/* save user specified operational read register */
			DBG2(printk(KERN_INFO "%s: read(r=0x%02X)\n",
				__func__, ts->reg_id);)
		}
	} else {
		int retval = 0;
		int tries = 0;
		DBG({
			char str[128];
			char *p = str;
			int i;
			for (i = 0; i < size; i++, p += 2)
				sprintf(p, "%02x", (unsigned char)buf[i]);
			printk(KERN_DEBUG "%s: size %d, pos %ld payload %s\n",
			       __func__, size, (long)pos, str);
		})
		do {
			retval = ttsp_write_block_data(ts,
				CY_REG_BASE, size, buf);
			if (retval < 0)
				msleep(500);
		} while ((retval < 0) && (tries++ < 10));
	}
	UNLOCK(ts->mutex);
	return size;
}

static ssize_t firmware_read(struct file *pFile, struct kobject *kobj,
	struct bin_attribute *ba,
	char *buf, loff_t pos, size_t size)
{
	int count = 0;
	u8 reg_data;
	struct device *dev = container_of(kobj, struct device, kobj);
	struct cyttsp *ts = dev_get_drvdata(dev);

	DBG2(printk(KERN_INFO"%s: Enter (mode=%d)\n",
		__func__, ts->fw_loader_mode);)

	LOCK(ts->mutex);
	if (!ts->fw_loader_mode) {
		/* read user specified operational register */
		cyttsp_rd_reg(ts, ts->reg_id & ~0x80, &reg_data);
		*(unsigned short *)buf = reg_data << 8;
		count = sizeof(unsigned short);
		printk(KERN_INFO "%s: read(reg=%02X(%d) dat=0x%02X(%d))\n",
			__func__, ts->reg_id & ~0x80, ts->reg_id & ~0x80,
			reg_data,  reg_data);
	} else {
		int retval = 0;
		int tries = 0;

		do {
			retval = cyttsp_load_bl_regs(ts);
			if (retval < 0)
				msleep(500);
		} while ((retval < 0) && (tries++ < 10));

		if (retval < 0) {
			printk(KERN_ERR "%s: error reading status\n", __func__);
			count = 0;
		} else {
			*(unsigned short *)buf = ts->bl_data.bl_status << 8 |
				ts->bl_data.bl_error;
			count = sizeof(unsigned short);
		}

		DBG2(printk(KERN_INFO
			"%s:bl_f=0x%02X bl_s=0x%02X bl_e=0x%02X\n",
			__func__,
			ts->bl_data.bl_file,
			ts->bl_data.bl_status,
			ts->bl_data.bl_error);)
	}
	UNLOCK(ts->mutex);
	return count;
}

static struct bin_attribute cyttsp_firmware = {
	.attr = {
		.name = "firmware",
		.mode = 0644,
	},
	.size = 128,
	.read = firmware_read,
	.write = firmware_write,
};

static ssize_t attr_fwloader_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct cyttsp *ts = dev_get_drvdata(dev);
	return sprintf(buf, "0x%02X%02X 0x%02X%02X 0x%02X%02X 0x%02X%02X%02X\n",
		ts->sysinfo_data.tts_verh, ts->sysinfo_data.tts_verl,
		ts->sysinfo_data.app_idh, ts->sysinfo_data.app_idl,
		ts->sysinfo_data.app_verh, ts->sysinfo_data.app_verl,
		ts->sysinfo_data.cid[0], ts->sysinfo_data.cid[1],
		ts->sysinfo_data.cid[2]);
}

static ssize_t attr_fwloader_store(struct device *dev,
				struct device_attribute *attr,
				const char *buf, size_t size)
{
	char *p;
	int ret;
	struct cyttsp *ts = dev_get_drvdata(dev);
	unsigned val = simple_strtoul(buf, &p, 10);

	ret = p - buf;
	if (*p && isspace(*p))
		ret++;
	DBG2(printk(KERN_DEBUG "%s: %u\n", __func__, val);)

	LOCK(ts->mutex)
	if (val == 3) {
		sysfs_remove_bin_file(&dev->kobj, &cyttsp_firmware);
		DBG2(printk(KERN_INFO "%s: FW loader closed for reg r/w\n",
			__func__);)
	} else if (val == 2) {
		if (sysfs_create_bin_file(&dev->kobj, &cyttsp_firmware))
			printk(KERN_ERR "%s: unable to create file\n",
				__func__);
		DBG2(printk(KERN_INFO "%s: FW loader opened for reg r/w\n",
			__func__);)
		if (ts->suspended) {
			cyttsp_resume(ts);
			ts->suspended = 0;
			enable_irq(ts->irq);
			/* resume watchdog */
			mod_timer(&ts->timer, jiffies + CY_WDG_TIMEOUT);
		}
	} else if ((val == 1) && !ts->fw_loader_mode) {
		ts->fw_loader_mode = 1;
		if (ts->suspended) {
			cyttsp_resume(ts);
			ts->suspended = 0;
			enable_irq(ts->irq);
			/* resume watchdog */
			mod_timer(&ts->timer, jiffies + CY_WDG_TIMEOUT);
		}
		disable_irq_nosync(ts->irq);
		/* kill watchdog */
		del_timer(&ts->timer);
		cancel_work_sync(&ts->work);
		if (sysfs_create_bin_file(&dev->kobj, &cyttsp_firmware))
			printk(KERN_ERR "%s: unable to create file\n",
				__func__);
		DBG2(printk(KERN_INFO
			"%s: FW loader opened for start load: ps=%d mode=%d\n",
			__func__,
			ts->platform_data->power_state, ts->fw_loader_mode);)
		cyttsp_soft_reset(ts);
		printk(KERN_INFO "%s: FW loader started.\n", __func__);
		ts->platform_data->power_state = CY_LDR_STATE;
	} else if (!val && ts->fw_loader_mode) {
		sysfs_remove_bin_file(&dev->kobj, &cyttsp_firmware);
		ts->fw_loader_mode = 0;
		printk(KERN_INFO "%s: FW loader finished.\n", __func__);
		enable_irq(ts->irq);
		ret = cyttsp_power_on(ts);
		if (ret < 0)
			printk(KERN_ERR "%s: Error, power on fail\n", __func__);
		/* resume watchdog */
		mod_timer(&ts->timer, jiffies + CY_WDG_TIMEOUT);
	}
	UNLOCK(ts->mutex);
	return  ret == size ? ret : -EINVAL;
}

static struct device_attribute fwloader =
	__ATTR(fwloader, 0644, attr_fwloader_show, attr_fwloader_store);

void *cyttsp_core_init(struct cyttsp_bus_ops *bus_ops, struct device *pdev)
{
	struct input_dev *input_device;
	struct cyttsp *ts;
	int retval = 0;

	DBG(printk(KERN_INFO"%s: Enter\n", __func__);)

	ts = kzalloc(sizeof(*ts), GFP_KERNEL);
	if (ts == NULL) {
		printk(KERN_ERR "%s: Error, kzalloc\n", __func__);
		goto error_alloc_data_failed;
	}
	mutex_init(&ts->mutex);
	ts->pdev = pdev;
	ts->platform_data = pdev->platform_data;
	ts->bus_ops = bus_ops;
	init_completion(&ts->int_running);

	if (ts->platform_data->init)
		retval = ts->platform_data->init(1);
	if (retval) {
		printk(KERN_ERR "%s: platform init failed! \n", __func__);
		goto error_init;
	}

	/* Create the input device and register it. */
	input_device = input_allocate_device();
	if (!input_device) {
		retval = -ENOMEM;
		printk(KERN_ERR "%s: Error, failed to allocate input device\n",
			__func__);
		goto error_input_allocate_device;
	}

	ts->input = input_device;
	input_device->name = ts->platform_data->name;
	input_device->phys = ts->phys;
	input_device->dev.parent = ts->pdev;

	/* enable interrupts */
	ts->irq = gpio_to_irq(ts->platform_data->irq_gpio);
	retval = request_threaded_irq(ts->irq, NULL, cyttsp_irq,
		IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
		ts->input->name, ts);
	if (retval < 0) {
		printk(KERN_ERR "%s: IRQ request failed r=%d\n",
			__func__, retval);
		ts->platform_data->power_state = CY_INVALID_STATE;
		goto error_set_irq;
	}
	/* setup watchdog */
	INIT_WORK(&ts->work, cyttsp_check_bl);
	setup_timer(&ts->timer, cyttsp_timer, (unsigned long) ts);

	DBG(printk(KERN_INFO "%s: call power_on\n", __func__);)
	retval = cyttsp_power_on(ts);
	if (retval < 0) {
		printk(KERN_ERR "%s: Error, power on failed! \n", __func__);
		goto error_power_on;
	}

	set_bit(EV_SYN, input_device->evbit);
	set_bit(EV_KEY, input_device->evbit);
	set_bit(EV_ABS, input_device->evbit);
	set_bit(BTN_TOUCH, input_device->keybit);
	set_bit(BTN_2, input_device->keybit);
	if (ts->platform_data->use_gestures)
		set_bit(BTN_3, input_device->keybit);

	input_set_abs_params(input_device, ABS_X, 0, ts->platform_data->maxx,
			     0, 0);
	input_set_abs_params(input_device, ABS_Y, 0, ts->platform_data->maxy,
			     0, 0);
	input_set_abs_params(input_device, ABS_TOOL_WIDTH, 0,
			     CY_LARGE_TOOL_WIDTH, 0, 0);
	input_set_abs_params(input_device, ABS_PRESSURE, 0, CY_MAXZ, 0, 0);
	input_set_abs_params(input_device, ABS_HAT0X, 0,
			     ts->platform_data->maxx, 0, 0);
	input_set_abs_params(input_device, ABS_HAT0Y, 0,
			     ts->platform_data->maxy, 0, 0);
	if (ts->platform_data->use_gestures) {
		input_set_abs_params(input_device, ABS_HAT1X, 0, CY_MAXZ,
				     0, 0);
		input_set_abs_params(input_device, ABS_HAT1Y, 0, CY_MAXZ,
				     0, 0);
	}
	if (ts->platform_data->use_mt) {
		input_set_abs_params(input_device, ABS_MT_POSITION_X, 0,
				     ts->platform_data->maxx, 0, 0);
		input_set_abs_params(input_device, ABS_MT_POSITION_Y, 0,
				     ts->platform_data->maxy, 0, 0);
		input_set_abs_params(input_device, ABS_MT_TOUCH_MAJOR, 0,
				     CY_MAXZ, 0, 0);
		input_set_abs_params(input_device, ABS_MT_WIDTH_MAJOR, 0,
				     CY_LARGE_TOOL_WIDTH, 0, 0);
		if (ts->platform_data->use_trk_id)
			input_set_abs_params(input_device, ABS_MT_TRACKING_ID,
					0, CY_NUM_TRK_ID, 0, 0);
	}

	if (ts->platform_data->use_virtual_keys)
		input_set_capability(input_device, EV_KEY, KEY_PROG1);

	retval = input_register_device(input_device);
	if (retval) {
		printk(KERN_ERR "%s: Error, failed to register input device\n",
			__func__);
		goto error_input_register_device;
	}
	DBG(printk(KERN_INFO "%s: Registered input device %s\n",
		   __func__, input_device->name);)

#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = cyttsp_ts_early_suspend;
	ts->early_suspend.resume = cyttsp_ts_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif
	retval = device_create_file(pdev, &fwloader);
	if (retval) {
		printk(KERN_ERR "%s: Error, could not create attribute\n",
			__func__);
		goto device_create_error;
	}
	dev_set_drvdata(pdev, ts);

	return ts;

device_create_error:
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ts->early_suspend);
#endif
error_input_register_device:
	input_unregister_device(input_device);
error_power_on:
	if (ts->irq >= 0)
		free_irq(ts->irq, ts);
error_set_irq:
	input_free_device(input_device);
error_input_allocate_device:
	if (ts->platform_data->init)
		ts->platform_data->init(0);
error_init:
	kfree(ts);
error_alloc_data_failed:
	return NULL;
}

/* registered in driver struct */
void cyttsp_core_release(void *handle)
{
	struct cyttsp *ts = handle;

	DBG(printk(KERN_INFO"%s: Enter\n", __func__);)
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ts->early_suspend);
#endif
	del_timer_sync(&ts->timer);
	cancel_work_sync(&ts->work);
	free_irq(ts->irq, ts);
	input_unregister_device(ts->input);
	input_free_device(ts->input);
	if (ts->platform_data->init)
		ts->platform_data->init(0);
	kfree(ts);
}
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Cypress TrueTouch(R) Standard touchscreen driver core");
MODULE_AUTHOR("Cypress");

