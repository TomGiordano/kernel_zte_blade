/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/hrtimer.h>
#include <linux/clk.h>
#include <mach/hardware.h>
#include <linux/io.h>
#include <linux/debugfs.h>

#include <asm/system.h>
#include <asm/mach-types.h>
#include <linux/semaphore.h>
#include <linux/uaccess.h>

#include "mdp.h"
#include "msm_fb.h"
#include "mdp4.h"
#include "mddihosti.h"

#define MDP_DEBUG_BUF	2048

static uint32	mdp_offset;
static uint32	mdp_count;

static char	debug_buf[MDP_DEBUG_BUF];

/*
 * MDP4
 *
 */

static int mdp_offset_open(struct inode *inode, struct file *file)
{
	/* non-seekable */
	file->f_mode &= ~(FMODE_LSEEK | FMODE_PREAD | FMODE_PWRITE);
	return 0;
}

static int mdp_offset_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t mdp_offset_write(
	struct file *file,
	const char __user *buff,
	size_t count,
	loff_t *ppos)
{
	uint32 off, cnt;

	if (count > sizeof(debug_buf))
		return -EFAULT;

	if (copy_from_user(debug_buf, buff, count))
		return -EFAULT;

	debug_buf[count] = 0;	/* end of string */

	sscanf(debug_buf, "%x %d", &off, &cnt);

	if (cnt <= 0)
		cnt = 1;

	mdp_offset = off;
	mdp_count = cnt;

	printk(KERN_INFO "%s: offset=%x cnt=%d\n", __func__,
				mdp_offset, mdp_count);

	return count;
}

static ssize_t mdp_offset_read(
	struct file *file,
	char __user *buff,
	size_t count,
	loff_t *ppos)
{
	int len = 0;


	if (*ppos)
		return 0;	/* the end */

	len = snprintf(debug_buf, sizeof(debug_buf), "0x%08x %d\n",
					mdp_offset, mdp_count);

	if (copy_to_user(buff, debug_buf, len))
		return -EFAULT;

	if (len < 0)
		return 0;

	*ppos += len;	/* increase offset */

	return len;
}

static const struct file_operations mdp_off_fops = {
	.open = mdp_offset_open,
	.release = mdp_offset_release,
	.read = mdp_offset_read,
	.write = mdp_offset_write,
};

static int mdp_reg_open(struct inode *inode, struct file *file)
{
	/* non-seekable */
	file->f_mode &= ~(FMODE_LSEEK | FMODE_PREAD | FMODE_PWRITE);
	return 0;
}

static int mdp_reg_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t mdp_reg_write(
	struct file *file,
	const char __user *buff,
	size_t count,
	loff_t *ppos)
{
	uint32 off, data;
	int cnt;

	if (count > sizeof(debug_buf))
		return -EFAULT;

	if (copy_from_user(debug_buf, buff, count))
		return -EFAULT;

	debug_buf[count] = 0;	/* end of string */

	cnt = sscanf(debug_buf, "%x %x", &off, &data);

	mdp_pipe_ctrl(MDP_CMD_BLOCK, MDP_BLOCK_POWER_ON, FALSE);
	outpdw(MDP_BASE + off, data);
	mdp_pipe_ctrl(MDP_CMD_BLOCK, MDP_BLOCK_POWER_OFF, FALSE);

	printk(KERN_INFO "%s: addr=%x data=%x\n", __func__, off, data);

	return count;
}

static ssize_t mdp_reg_read(
	struct file *file,
	char __user *buff,
	size_t count,
	loff_t *ppos)
{
	int len = 0;
	uint32 data;
	int i, j, off, dlen, num;
	char *bp, *cp;
	int tot = 0;


	if (*ppos)
		return 0;	/* the end */

	j = 0;
	num = 0;
	bp = debug_buf;
	cp = MDP_BASE + mdp_offset;
	dlen = sizeof(debug_buf);
	mdp_pipe_ctrl(MDP_CMD_BLOCK, MDP_BLOCK_POWER_ON, FALSE);
	while (j++ < 8) {
		len = snprintf(bp, dlen, "0x%08x: ", (int)cp);
		tot += len;
		bp += len;
		dlen -= len;
		off = 0;
		i = 0;
		while (i++ < 4) {
			data = inpdw(cp + off);
			len = snprintf(bp, dlen, "%08x ", data);
			tot += len;
			bp += len;
			dlen -= len;
			off += 4;
			num++;
			if (num >= mdp_count)
				break;
		}
		*bp++ = '\n';
		tot++;
		cp += off;
		if (num >= mdp_count)
			break;
	}
	mdp_pipe_ctrl(MDP_CMD_BLOCK, MDP_BLOCK_POWER_OFF, FALSE);
	*bp = 0;
	tot++;

	if (copy_to_user(buff, debug_buf, tot))
		return -EFAULT;

	if (tot < 0)
		return 0;

	*ppos += tot;	/* increase offset */

	return tot;
}


static const struct file_operations mdp_reg_fops = {
	.open = mdp_reg_open,
	.release = mdp_reg_release,
	.read = mdp_reg_read,
	.write = mdp_reg_write,
};

static int mdp_stat_open(struct inode *inode, struct file *file)
{
	/* non-seekable */
	file->f_mode &= ~(FMODE_LSEEK | FMODE_PREAD | FMODE_PWRITE);
	return 0;
}

static int mdp_stat_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t mdp_stat_write(
	struct file *file,
	const char __user *buff,
	size_t count,
	loff_t *ppos)
{
	unsigned long flag;

	if (count > sizeof(debug_buf))
		return -EFAULT;

	spin_lock_irqsave(&mdp_spin_lock, flag);
	memset((char *)&mdp4_stat, 0 , sizeof(mdp4_stat));	/* reset */
	spin_unlock_irqrestore(&mdp_spin_lock, flag);

	return count;
}

static ssize_t mdp_stat_read(
	struct file *file,
	char __user *buff,
	size_t count,
	loff_t *ppos)
{
	int len = 0;
	int tot = 0;
	int dlen;
	char *bp;
	unsigned long flag;


	if (*ppos)
		return 0;	/* the end */

	bp = debug_buf;
	dlen = sizeof(debug_buf);

	spin_lock_irqsave(&mdp_spin_lock, flag);
	len = snprintf(bp, dlen, "intr_total:    %08lu\n",
					mdp4_stat.intr_tot);
	bp += len;
	dlen -= len;
	len = snprintf(bp, dlen, "intr_dma_p:    %08lu\n",
					mdp4_stat.intr_dma_p);
	bp += len;
	dlen -= len;
	len = snprintf(bp, dlen, "intr_dma_s:    %08lu\n",
					mdp4_stat.intr_dma_s);
	bp += len;
	dlen -= len;
	len = snprintf(bp, dlen, "intr_dma_e:    %08lu\n",
					mdp4_stat.intr_dma_e);
	bp += len;
	dlen -= len;
	len = snprintf(bp, dlen, "intr_overlay0: %08lu\n",
					mdp4_stat.intr_overlay0);
	bp += len;
	dlen -= len;
	len = snprintf(bp, dlen, "intr_overlay1: %08lu\n",
					mdp4_stat.intr_overlay1);
	bp += len;
	dlen -= len;
	len = snprintf(bp, dlen, "unerrun_primary:  %08lu\n",
					mdp4_stat.intr_underrun_p);
	bp += len;
	dlen -= len;
	len = snprintf(bp, dlen, "unerrun_external:  %08lu\n\n",
					mdp4_stat.intr_underrun_e);

	bp += len;
	dlen -= len;
	spin_unlock_irqrestore(&mdp_spin_lock, flag);

	len = snprintf(bp, dlen, "kickoff_mddi:      %08lu\n",
					mdp4_stat.kickoff_mddi);
	bp += len;
	dlen -= len;
	len = snprintf(bp, dlen, "kickoff_mddi_skip: %08lu\n",
					mdp4_stat.kickoff_mddi_skip);
	bp += len;
	dlen -= len;
	len = snprintf(bp, dlen, "kickoff_lcdc:      %08lu\n",
					mdp4_stat.kickoff_lcdc);
	bp += len;
	dlen -= len;
	len = snprintf(bp, dlen, "kickoff_dtv:       %08lu\n\n",
					mdp4_stat.kickoff_dtv);
	bp += len;
	dlen -= len;
	len = snprintf(bp, dlen, "overlay0_set:   %08lu\n",
					mdp4_stat.overlay_set[0]);
	bp += len;
	dlen -= len;
	len = snprintf(bp, dlen, "overlay0_unset: %08lu\n",
					mdp4_stat.overlay_unset[0]);
	bp += len;
	dlen -= len;
	len = snprintf(bp, dlen, "overlay0_play:  %08lu\n",
					mdp4_stat.overlay_play[0]);
	bp += len;
	dlen -= len;
	len = snprintf(bp, dlen, "overlay1_set:   %08lu\n",
					mdp4_stat.overlay_set[1]);
	bp += len;
	dlen -= len;
	len = snprintf(bp, dlen, "overlay1_unset: %08lu\n",
					mdp4_stat.overlay_unset[1]);
	bp += len;
	dlen -= len;
	len = snprintf(bp, dlen, "overlay1_play:  %08lu\n\n",
					mdp4_stat.overlay_play[1]);

	bp += len;
	dlen -= len;
	len = snprintf(bp, dlen, "pipe_rgb1:  %08lu\n", mdp4_stat.pipe[0]);
	bp += len;
	dlen -= len;
	len = snprintf(bp, dlen, "pipe_rgb2:  %08lu\n", mdp4_stat.pipe[1]);
	bp += len;
	dlen -= len;
	len = snprintf(bp, dlen, "pipe_vg1:   %08lu\n", mdp4_stat.pipe[2]);
	bp += len;
	dlen -= len;
	len = snprintf(bp, dlen, "pipe_vg2:   %08lu\n\n", mdp4_stat.pipe[3]);
	bp += len;
	dlen -= len;
	len = snprintf(bp, dlen, "err_mixer:  %08lu\n", mdp4_stat.err_mixer);
	bp += len;
	dlen -= len;
	len = snprintf(bp, dlen, "err_size:   %08lu\n", mdp4_stat.err_size);
	bp += len;
	dlen -= len;
	len = snprintf(bp, dlen, "err_scale:  %08lu\n", mdp4_stat.err_scale);
	bp += len;
	dlen -= len;
	len = snprintf(bp, dlen, "err_format: %08lu\n", mdp4_stat.err_format);
	bp += len;
	dlen -= len;

	tot = (uint32)bp - (uint32)debug_buf;
	*bp = 0;
	tot++;

	if (copy_to_user(buff, debug_buf, tot))
		return -EFAULT;

	if (tot < 0)
		return 0;

	*ppos += tot;	/* increase offset */

	return tot;
}

static const struct file_operations mdp_stat_fops = {
	.open = mdp_stat_open,
	.release = mdp_stat_release,
	.read = mdp_stat_read,
	.write = mdp_stat_write,
};

/*
 * MDDI
 *
 */

struct mddi_reg {
	char *name;
	int off;
};

static struct mddi_reg mddi_regs_list[] = {
	{"MDDI_CMD", MDDI_CMD},	 	/* 0x0000 */
	{"MDDI_VERSION", MDDI_VERSION},  /* 0x0004 */
	{"MDDI_PRI_PTR", MDDI_PRI_PTR},  /* 0x0008 */
	{"MDDI_BPS",  MDDI_BPS}, 	/* 0x0010 */
	{"MDDI_SPM", MDDI_SPM}, 	/* 0x0014 */
	{"MDDI_INT", MDDI_INT}, 	/* 0x0018 */
	{"MDDI_INTEN", MDDI_INTEN},	/* 0x001c */
	{"MDDI_REV_PTR", MDDI_REV_PTR},	/* 0x0020 */
	{"MDDI_	REV_SIZE", MDDI_REV_SIZE},/* 0x0024 */
	{"MDDI_STAT", MDDI_STAT},	/* 0x0028 */
	{"MDDI_REV_RATE_DIV", MDDI_REV_RATE_DIV}, /* 0x002c */
	{"MDDI_REV_CRC_ERR", MDDI_REV_CRC_ERR}, /* 0x0030 */
	{"MDDI_TA1_LEN", MDDI_TA1_LEN}, /* 0x0034 */
	{"MDDI_TA2_LEN", MDDI_TA2_LEN}, /* 0x0038 */
	{"MDDI_TEST", MDDI_TEST}, 	/* 0x0040 */
	{"MDDI_REV_PKT_CNT", MDDI_REV_PKT_CNT}, /* 0x0044 */
	{"MDDI_DRIVE_HI", MDDI_DRIVE_HI},/* 0x0048 */
	{"MDDI_DRIVE_LO", MDDI_DRIVE_LO},	/* 0x004c */
	{"MDDI_DISP_WAKE", MDDI_DISP_WAKE},/* 0x0050 */
	{"MDDI_REV_ENCAP_SZ", MDDI_REV_ENCAP_SZ}, /* 0x0054 */
	{"MDDI_RTD_VAL", MDDI_RTD_VAL}, /* 0x0058 */
	{"MDDI_PAD_CTL", MDDI_PAD_CTL},	 /* 0x0068 */
	{"MDDI_DRIVER_START_CNT", MDDI_DRIVER_START_CNT}, /* 0x006c */
	{"MDDI_CORE_VER", MDDI_CORE_VER}, /* 0x008c */
	{"MDDI_FIFO_ALLOC", MDDI_FIFO_ALLOC}, /* 0x0090 */
	{"MDDI_PAD_IO_CTL", MDDI_PAD_IO_CTL}, /* 0x00a0 */
	{"MDDI_PAD_CAL", MDDI_PAD_CAL},  /* 0x00a4 */
	{0, 0}
};

static int mddi_reg_open(struct inode *inode, struct file *file)
{
	/* non-seekable */
	file->f_mode &= ~(FMODE_LSEEK | FMODE_PREAD | FMODE_PWRITE);
	return 0;
}

static int mddi_reg_release(struct inode *inode, struct file *file)
{
	return 0;
}

static void mddi_reg_write(int ndx, uint32 off, uint32 data)
{
	char *base;

	if (ndx)
		base = (char *)msm_emdh_base;
	else
		base = (char *)msm_pmdh_base;

	writel(data, base + off);

	printk(KERN_INFO "%s: addr=%x data=%x\n",
			__func__, (int)(base+off), (int)data);
}

static int mddi_reg_read(int ndx)
{
	struct mddi_reg *reg;
	unsigned char *base;
	int data;
	char *bp;
	int len = 0;
	int tot = 0;
	int dlen;

	if (ndx)
		base = msm_emdh_base;
	else
		base = msm_pmdh_base;

	reg = mddi_regs_list;
	bp = debug_buf;
	dlen = sizeof(debug_buf);
	while (reg->name) {
		data = readl((u32)base + reg->off);
		len = snprintf(bp, dlen, "%s:0x%08x\t\t= 0x%08x\n",
					reg->name, reg->off, data);
		tot += len;
		bp += len;
		dlen -= len;
		reg++;
	}
	*bp = 0;
	tot++;

	return tot;
}

static ssize_t pmdh_reg_write(
	struct file *file,
	const char __user *buff,
	size_t count,
	loff_t *ppos)
{
	uint32 off, data;
	int cnt;

	if (count > sizeof(debug_buf))
		return -EFAULT;

	if (copy_from_user(debug_buf, buff, count))
		return -EFAULT;

	debug_buf[count] = 0;	/* end of string */

	cnt = sscanf(debug_buf, "%x %x", &off, &data);

	mddi_reg_write(0, off, data);

	return count;
}

static ssize_t pmdh_reg_read(
	struct file *file,
	char __user *buff,
	size_t count,
	loff_t *ppos)
{
	int tot = 0;

	if (*ppos)
		return 0;	/* the end */

	tot = mddi_reg_read(0);	/* pmdh */

	if (copy_to_user(buff, debug_buf, tot))
		return -EFAULT;

	if (tot < 0)
		return 0;

	*ppos += tot;	/* increase offset */

	return tot;
}


static const struct file_operations pmdh_fops = {
	.open = mddi_reg_open,
	.release = mddi_reg_release,
	.read = pmdh_reg_read,
	.write = pmdh_reg_write,
};


#ifdef MDP4_MDDI_DMA_SWITCH
static int vsync_reg_open(struct inode *inode, struct file *file)
{
	/* non-seekable */
	file->f_mode &= ~(FMODE_LSEEK | FMODE_PREAD | FMODE_PWRITE);
	return 0;
}

static int vsync_reg_release(struct inode *inode, struct file *file)
{
	return 0;
}

static ssize_t vsync_reg_write(
	struct file *file,
	const char __user *buff,
	size_t count,
	loff_t *ppos)
{
	uint32 enable;
	int cnt;

	if (count > sizeof(debug_buf))
		return -EFAULT;

	if (copy_from_user(debug_buf, buff, count))
		return -EFAULT;

	debug_buf[count] = 0;	/* end of string */

	cnt = sscanf(debug_buf, "%x", &enable);

	mdp_dmap_vsync_set(enable);

	return count;
}

static ssize_t vsync_reg_read(
	struct file *file,
	char __user *buff,
	size_t count,
	loff_t *ppos)
{
	char *bp;
	int len = 0;
	int tot = 0;
	int dlen;

	if (*ppos)
		return 0;	/* the end */

	bp = debug_buf;
	dlen = sizeof(debug_buf);
	len = snprintf(bp, dlen, "%x\n", mdp_dmap_vsync_get());
	tot += len;
	bp += len;
	*bp = 0;
	tot++;

	if (copy_to_user(buff, debug_buf, tot))
		return -EFAULT;

	*ppos += tot;	/* increase offset */

	return tot;
}


static const struct file_operations vsync_fops = {
	.open = vsync_reg_open,
	.release = vsync_reg_release,
	.read = vsync_reg_read,
	.write = vsync_reg_write,
};

#endif

static ssize_t emdh_reg_write(
	struct file *file,
	const char __user *buff,
	size_t count,
	loff_t *ppos)
{
	uint32 off, data;
	int cnt;

	if (count > sizeof(debug_buf))
		return -EFAULT;

	if (copy_from_user(debug_buf, buff, count))
		return -EFAULT;

	debug_buf[count] = 0;	/* end of string */

	cnt = sscanf(debug_buf, "%x %x", &off, &data);

	mddi_reg_write(1, off, data);

	return count;
}

static ssize_t emdh_reg_read(
	struct file *file,
	char __user *buff,
	size_t count,
	loff_t *ppos)
{
	int tot = 0;

	if (*ppos)
		return 0;	/* the end */

	tot = mddi_reg_read(1);	/* emdh */

	if (copy_to_user(buff, debug_buf, tot))
		return -EFAULT;

	if (tot < 0)
		return 0;

	*ppos += tot;	/* increase offset */

	return tot;
}

static const struct file_operations emdh_fops = {
	.open = mddi_reg_open,
	.release = mddi_reg_release,
	.read = emdh_reg_read,
	.write = emdh_reg_write,
};

/*
 * debugfs
 *
 */

int mdp4_debugfs_init(void)
{
	struct dentry *dent = debugfs_create_dir("mdp4", NULL);

	if (IS_ERR(dent)) {
		printk(KERN_ERR "%s(%d): debugfs_create_dir fail, error %ld\n",
			__FILE__, __LINE__, PTR_ERR(dent));
		return -1;
	}

	if (debugfs_create_file("off", 0644, dent, 0, &mdp_off_fops)
			== NULL) {
		printk(KERN_ERR "%s(%d): debugfs_create_file: index fail\n",
			__FILE__, __LINE__);
		return -1;
	}

	if (debugfs_create_file("reg", 0644, dent, 0, &mdp_reg_fops)
			== NULL) {
		printk(KERN_ERR "%s(%d): debugfs_create_file: debug fail\n",
			__FILE__, __LINE__);
		return -1;
	}

	if (debugfs_create_file("stat", 0644, dent, 0, &mdp_stat_fops)
			== NULL) {
		printk(KERN_ERR "%s(%d): debugfs_create_file: debug fail\n",
			__FILE__, __LINE__);
		return -1;
	}

	dent = debugfs_create_dir("mddi", NULL);

	if (IS_ERR(dent)) {
		printk(KERN_ERR "%s(%d): debugfs_create_dir fail, error %ld\n",
			__FILE__, __LINE__, PTR_ERR(dent));
		return -1;
	}

	if (debugfs_create_file("reg", 0644, dent, 0, &pmdh_fops)
			== NULL) {
		printk(KERN_ERR "%s(%d): debugfs_create_file: debug fail\n",
			__FILE__, __LINE__);
		return -1;
	}

#ifdef MDP4_MDDI_DMA_SWITCH
	if (debugfs_create_file("vsync", 0644, dent, 0, &vsync_fops)
			== NULL) {
		printk(KERN_ERR "%s(%d): debugfs_create_file: debug fail\n",
			__FILE__, __LINE__);
		return -1;
	}
#endif

	dent = debugfs_create_dir("emdh", NULL);

	if (IS_ERR(dent)) {
		printk(KERN_ERR "%s(%d): debugfs_create_dir fail, error %ld\n",
			__FILE__, __LINE__, PTR_ERR(dent));
		return -1;
	}

	if (debugfs_create_file("reg", 0644, dent, 0, &emdh_fops)
			== NULL) {
		printk(KERN_ERR "%s(%d): debugfs_create_file: debug fail\n",
			__FILE__, __LINE__);
		return -1;
	}

	return 0;
}
