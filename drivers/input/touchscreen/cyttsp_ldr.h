/*
 * Source for:
 * Cypress TrueTouch(TM) Standard Product touchscreen driver.
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
/*
 ************************************************************************
 * Compiled image bootloader functions
 ************************************************************************
 */
#include "cyttsp_fw.h"
#define CY_BL_PAGE_SIZE		16
#define CY_BL_NUM_PAGES		5
#define CY_MAX_DATA_LEN		(CY_BL_PAGE_SIZE * 2)

/* Timeout timer */
static int cyttsp_check_polling(struct cyttsp *ts)
{
	return ts->platform_data->use_timer;
}

static void cyttsp_to_timer(unsigned long handle)
{
	struct cyttsp *ts = (struct cyttsp *)handle;

	DBG(printk(KERN_INFO"%s: TTSP timeout timer event!\n", __func__);)
	ts->to_timeout = true;
	return;
}

static void cyttsp_setup_to_timer(struct cyttsp *ts)
{
	DBG(printk(KERN_INFO"%s: Enter\n", __func__);)
	setup_timer(&ts->to_timer, cyttsp_to_timer, (unsigned long) ts);
}

static void cyttsp_kill_to_timer(struct cyttsp *ts)
{
	DBG(printk(KERN_INFO"%s: Enter\n", __func__);)
	del_timer(&ts->to_timer);
}

static void cyttsp_start_to_timer(struct cyttsp *ts, int ms)
{
	DBG(printk(KERN_INFO"%s: Enter\n", __func__);)
	ts->to_timeout = false;
	mod_timer(&ts->to_timer, jiffies + ms);
}

static bool cyttsp_timeout(struct cyttsp *ts)
{
	if (cyttsp_check_polling(ts))
		return false;
	else
		return ts->to_timeout;
}

static void cyttsp_set_bl_ready(struct cyttsp *ts, bool set)
{
	DBG(printk(KERN_INFO"%s: Enter\n", __func__);)
	ts->bl_ready = set;
	DBG(printk(KERN_INFO"%s: bl_ready=%d\n", __func__, (int)ts->bl_ready);)
}

static bool cyttsp_check_bl_ready(struct cyttsp *ts)
{
	if (cyttsp_check_polling(ts))
		return true;
	else
		return ts->bl_ready;
}

static bool cyttsp_bl_err_status(struct cyttsp *ts)
{
	DBG(printk(KERN_INFO"%s: Enter\n", __func__);)
	return (((ts->bl_data.bl_status == 0x10) &&
		(ts->bl_data.bl_error == 0x20)) ||
		((ts->bl_data.bl_status == 0x11) &&
		(ts->bl_data.bl_error == 0x20)));
}

static bool cyttsp_wait_bl_ready(struct cyttsp *ts,
	int pre_delay, int loop_delay, int max_try,
	bool (*done)(struct cyttsp *ts))
{
	int tries;
	bool rdy = false, tmo = false;

	DBG(printk(KERN_INFO"%s: Enter\n", __func__);)
	DBG(printk(KERN_INFO"%s: pre-dly=%d loop-dly=%d, max-try=%d\n",
		__func__, pre_delay, loop_delay, max_try);)

	tries = 0;
	ts->bl_data.bl_file = 0;
	ts->bl_data.bl_status = 0;
	ts->bl_data.bl_error = 0;
	if (cyttsp_check_polling(ts)) {
		msleep(pre_delay);
		do {
			msleep(abs(loop_delay));
			cyttsp_load_bl_regs(ts);
		} while (!done(ts) &&
			tries++ < max_try);
		DBG(printk(KERN_INFO"%s: polling mode tries=%d\n",
			__func__, tries);)
	} else {
		cyttsp_start_to_timer(ts, abs(loop_delay) * max_try);
		while (!rdy && !tmo) {
			rdy = cyttsp_check_bl_ready(ts);
			tmo = cyttsp_timeout(ts);
			if (loop_delay < 0)
				udelay(abs(loop_delay));
			else
				msleep(abs(loop_delay));
			tries++;
		}
		DBG2(printk(KERN_INFO"%s: irq mode tries=%d rdy=%d tmo=%d\n",
			__func__, tries, (int)rdy, (int)tmo);)
		cyttsp_load_bl_regs(ts);
	}

	if (tries >= max_try || tmo)
		return true;	/* timeout */
	else
		return false;
}

static int cyttsp_wr_blk_chunks(struct cyttsp *ts, u8 cmd,
	u8 length, const u8 *values)
{
	int retval = 0;
	int block = 1;
	bool timeout;

	u8 dataray[CY_MAX_DATA_LEN];

	DBG(printk(KERN_INFO"%s: Enter\n", __func__);)

	/* first page already includes the bl page offset */
	memcpy(dataray, values, CY_BL_PAGE_SIZE + 1);
	cyttsp_set_bl_ready(ts, false);
	retval = ttsp_write_block_data(ts, cmd, CY_BL_PAGE_SIZE + 1, dataray);
	values += CY_BL_PAGE_SIZE + 1;
	length -= CY_BL_PAGE_SIZE + 1;
	if (retval)
		return retval;

	/* remaining blocks require bl page offset stuffing */
	while (length && (block < CY_BL_NUM_PAGES) && !(retval < 0)) {
		dataray[0] = CY_BL_PAGE_SIZE * block;
		timeout = cyttsp_wait_bl_ready(ts,
			1, -100, 100, cyttsp_bl_err_status);
		if (timeout)
			return -EIO;
		memcpy(&dataray[1], values, length >= CY_BL_PAGE_SIZE ?
			CY_BL_PAGE_SIZE : length);
		cyttsp_set_bl_ready(ts, false);
		retval = ttsp_write_block_data(ts, cmd,
			length >= CY_BL_PAGE_SIZE ?
			CY_BL_PAGE_SIZE + 1 : length + 1, dataray);
		values += CY_BL_PAGE_SIZE;
		length = length >= CY_BL_PAGE_SIZE ?
			length - CY_BL_PAGE_SIZE : 0;
		block++;
	}

	return retval;
}

static int cyttsp_load_app(struct cyttsp *ts)
{
	int retval = 0;
	int rec;
	bool timeout;

	DBG(printk(KERN_INFO"%s: Enter\n", __func__);)

	printk(KERN_INFO "%s: "
		"load file - tver=0x%02X%02X a_id=0x%02X%02X aver=0x%02X%02X\n",
		__func__,
		cyttsp_fw_tts_verh, cyttsp_fw_tts_verl,
		cyttsp_fw_app_idh, cyttsp_fw_app_idl,
		cyttsp_fw_app_verh, cyttsp_fw_app_verl);

	/* download new TTSP Application to the Bootloader */
	rec = 0;

	/* send bootload initiation command */
	printk(KERN_INFO"%s: Send BL Enter\n", __func__);
	cyttsp_set_bl_ready(ts, false);
	retval = ttsp_write_block_data(ts, CY_REG_BASE,
		cyttsp_fw[rec].Length, cyttsp_fw[rec].Block);
	rec++;
	if (retval)
		return retval;
	timeout = cyttsp_wait_bl_ready(ts, 1, 100, 100, cyttsp_bl_err_status);
	DBG(printk(KERN_INFO "%s: BL ENTER f=%02X s=%02X e=%02X t=%d\n",
		__func__,
		ts->bl_data.bl_file, ts->bl_data.bl_status,
		ts->bl_data.bl_error, timeout);)
	if (timeout)
		goto loader_exit;

	/* send bootload firmware load blocks */
	printk(KERN_INFO"%s: Send BL Blocks\n", __func__);
	while (cyttsp_fw[rec].Command == CY_BL_WRITE_BLK) {
		DBG2(printk(KERN_INFO "%s:"
			"BL DNLD Rec=% 3d Len=% 3d Addr=%04X\n",
			__func__,
			cyttsp_fw[rec].Record, cyttsp_fw[rec].Length,
			cyttsp_fw[rec].Address);
		)
		retval = cyttsp_wr_blk_chunks(ts, CY_REG_BASE,
			cyttsp_fw[rec].Length, cyttsp_fw[rec].Block);
		if (retval < 0) {
			DBG(printk(KERN_INFO "%s:"
				"BL fail Rec=%3d retval=%d\n",
				__func__,
				cyttsp_fw[rec].Record, retval);
			)
			break;
		} else {
			cyttsp_wait_bl_ready(ts, 10, 1, 1000,
				cyttsp_bl_err_status);
			DBG(printk(KERN_INFO "%s: BL _LOAD "
				"f=%02X s=%02X e=%02X\n",
				__func__,
				ts->bl_data.bl_file, ts->bl_data.bl_status,
				ts->bl_data.bl_error);)
		}
		rec++;
	}
	if (retval < 0)
		goto loader_exit;

	/* send bootload terminate command */
	printk(KERN_INFO"%s: Send BL Terminate\n", __func__);
	cyttsp_set_bl_ready(ts, false);
	retval = ttsp_write_block_data(ts, CY_REG_BASE,
		cyttsp_fw[rec].Length, cyttsp_fw[rec].Block);
	if (retval < 0)
		goto loader_exit;
	else
		cyttsp_wait_bl_ready(ts, 1, 100, 100, cyttsp_bl_err_status);

loader_exit:
	/* reset TTSP Device back to bootloader mode */
	retval = cyttsp_soft_reset(ts);

	return retval;
}

static int cyttsp_loader(struct cyttsp *ts)
{
	int retval;

	DBG(printk(KERN_INFO"%s: Enter\n", __func__);)

	retval = cyttsp_load_bl_regs(ts);
	if (retval < 0)
		return retval;

	printk(KERN_INFO "%s:"
		"blttsp=0x%02X%02X flttsp=0x%02X%02X force=%d\n",
		__func__,
		ts->bl_data.ttspver_hi, ts->bl_data.ttspver_lo,
		cyttsp_fw_tts_verh, cyttsp_fw_tts_verl,
		ts->platform_data->use_force_fw_update);
	printk(KERN_INFO "%s:"
		"blappid=0x%02X%02X flappid=0x%02X%02X\n",
		__func__,
		ts->bl_data.appid_hi, ts->bl_data.appid_lo,
		cyttsp_fw_app_idh, cyttsp_fw_app_idl);
	printk(KERN_INFO "%s:"
		"blappver=0x%02X%02X flappver=0x%02X%02X\n",
		__func__,
		ts->bl_data.appver_hi, ts->bl_data.appver_lo,
		cyttsp_fw_app_verh, cyttsp_fw_app_verl);
	printk(KERN_INFO "%s:"
		"blcid=0x%02X%02X%02X flcid=0x%02X%02X%02X\n",
		__func__,
		ts->bl_data.cid_0, ts->bl_data.cid_1, ts->bl_data.cid_2,
		cyttsp_fw_cid_0, cyttsp_fw_cid_1, cyttsp_fw_cid_2);

	if (CY_DIFF(ts->bl_data.ttspver_hi, cyttsp_fw_tts_verh)  ||
		CY_DIFF(ts->bl_data.ttspver_lo, cyttsp_fw_tts_verl)  ||
		CY_DIFF(ts->bl_data.appid_hi, cyttsp_fw_app_idh)  ||
		CY_DIFF(ts->bl_data.appid_lo, cyttsp_fw_app_idl)  ||
		CY_DIFF(ts->bl_data.appver_hi, cyttsp_fw_app_verh)  ||
		CY_DIFF(ts->bl_data.appver_lo, cyttsp_fw_app_verl)  ||
		CY_DIFF(ts->bl_data.cid_0, cyttsp_fw_cid_0)  ||
		CY_DIFF(ts->bl_data.cid_1, cyttsp_fw_cid_1)  ||
		CY_DIFF(ts->bl_data.cid_2, cyttsp_fw_cid_2)  ||
		ts->platform_data->use_force_fw_update) {
		/* load new app into TTSP Device */
		cyttsp_setup_to_timer(ts);
		ts->platform_data->power_state = CY_LDR_STATE;
		retval = cyttsp_load_app(ts);
		cyttsp_kill_to_timer(ts);

	} else {
		/* firmware file is a match with firmware in the TTSP device */
		DBG(printk(KERN_INFO "%s: FW matches - no loader\n", __func__);)
	}

	if (retval < 0)
		return retval;

	return retval;
}

