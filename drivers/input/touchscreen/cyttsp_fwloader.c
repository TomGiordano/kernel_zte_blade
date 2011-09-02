/* Source for:
 * Cypress TrueTouch(TM) Standard Product touchscreen driver.
 * File Based firmware field upgrade loader program.
 * vendor/cypress/cyttsp_fwloader/cyttsp_fwloader.c
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
#define LOG_TAG "cyttsp_fw_loader"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <limits.h>
#include <cutils/log.h>

#ifndef offsetof
#define offsetof(st, m) ((size_t) ((char *)&((st *)(0))->m - (char *)0))
#endif

#define BLK_SIZE     16
#define DATA_REC_LEN 64
#define START_ADDR   0x0b00
#define BLK_SEED     0xff
#define RECAL_REG    0x1b

enum bl_commands {
	BL_CMD_WRBLK     = 0x39,
	BL_CMD_INIT      = 0x38,
	BL_CMD_TERMINATE = 0x3b,
};
#define KEY_CS  (0 + 1 + 2 + 3 + 4 + 5 + 6 + 7)
#define KEY {0, 1, 2, 3, 4, 5, 6, 7}

static const  char _key[] = KEY;
#define KEY_LEN sizeof(_key)

struct fw_record {
	unsigned char seed;
	unsigned char cmd;
	unsigned char key[KEY_LEN];
	unsigned char blk_hi;
	unsigned char blk_lo;
	unsigned char data[DATA_REC_LEN];
	unsigned char data_cs;
	unsigned char rec_cs;
	int (*condition)(unsigned short status);
	int timeout_ms;
	int num_tries;
};
#define fw_rec_size offsetof(struct fw_record, condition)

struct cmd_record {
	unsigned char reg;
	unsigned char seed;
	unsigned char cmd;
	unsigned char key[KEY_LEN];
	int (*condition)(unsigned short status);
	int timeout_ms;
	int num_tries;
};
#define cmd_rec_size offsetof(struct cmd_record, condition)

struct fw_id {
	unsigned long tts_ver;
	unsigned long app_id;
	unsigned long app_ver;
	unsigned long cid;
};

enum bl_status {
	BL_RUNNING   = 0x1000,
	BL_BUSY      = 0x8000,
	BL_RECEPTIVE = 0x0020,
};

static const char *start_file = "fwloader";
static const char *fw_file = "firmware";

static int cond_init(unsigned short status)
{
	status &= BL_RUNNING | BL_BUSY;
	return status == BL_RUNNING;
}

static int cond_write(unsigned short status)
{
	status &= BL_RUNNING | BL_RECEPTIVE | BL_BUSY;
	return status == (BL_RUNNING | BL_RECEPTIVE);
}

static int cond_dummy(unsigned short status)
{
	LOGV("status 0x%04x\n", status);
	return 1;
}

static struct fw_record data_record = {
	.seed = BLK_SEED,
	.cmd = BL_CMD_WRBLK,
	.key = KEY,
	.condition = cond_write,
	.timeout_ms = 30,
	.num_tries = 10,
};

static const struct cmd_record terminate_rec = {
	.reg = 0,
	.seed = BLK_SEED,
	.cmd = BL_CMD_TERMINATE,
	.key = KEY,
	.condition = cond_init,
	.timeout_ms = 300,
	.num_tries = 30,
};
static const struct cmd_record initiate_rec = {
	.reg = 0,
	.seed = BLK_SEED,
	.cmd = BL_CMD_INIT,
	.key = KEY,
	.condition = cond_init,
	.timeout_ms = 300,
	.num_tries = 30,
};

#define BL_REC1_ADDR          0x0780
#define BL_REC2_ADDR          0x07c0
#define ID_INFO_REC           ":40078000"
#define ID_INFO_OFFSET_IN_REC 77
/*
* Hex record structure hints:
* :40064000570e5dcf3fa350013fa479bff662cd0151aa298060cf55acff7f30
*/
#define REC_START_CHR     ':'
#define REC_LEN_OFFSET     1
#define REC_ADDR_HI_OFFSET 3
#define REC_ADDR_LO_OFFSET 5
#define REC_TYPE_OFFSET    7
#define REC_DATA_OFFSET    9

static int get_current_fw_id(char *fname, struct fw_id *id)
{
	int rc = -1;
	char buf[128];
	FILE *fp = fopen(fname, "r");
	if (fp) {
		fgets(buf, sizeof(buf), fp);
		rc =  4 != sscanf(buf, "%lx %lx %lx %lx", &id->tts_ver,
				&id->app_id, &id->app_ver, &id->cid);
		if (rc)
			LOGE("%s: FW id is not complete: %s", __func__, buf);
		else
			LOGV("Current FW: tts_ver 0x%lx, app_id 0x%lx,"
				" app_ver 0x%lx, cid 0x%lx\n",
				id->tts_ver, id->app_id, id->app_ver, id->cid);
		fclose(fp);
		return rc;
	}
	LOGE("%s: Unable to open %s\n", __func__, fname);
	return rc;
}

static int get_fw_id(char *fw_file, struct fw_id *id)
{
	char *p;
	int i;
	int rc = -1;
	static char buf[1024];
	FILE *fp = fopen(fw_file, "r");

	if (!fp) {
		LOGE("%s: Unable to open %s\n", __func__, fw_file);
		return -1;
	}
	while (!feof(fp)) {
		if ((p = fgets(buf, sizeof(buf), fp))) {
			if (!strncmp(p, ID_INFO_REC, strlen(ID_INFO_REC))) {
				p = p + ID_INFO_OFFSET_IN_REC;
				rc = 4 != sscanf(p, "%04lx%04lx%04lx%06lx",
						  &id->tts_ver, &id->app_id,
						  &id->app_ver, &id->cid);
				break;
			}
		}
	}
	if (feof(fp)) {
		LOGE("%s: FW id not found", __func__);
		fclose(fp);
		return -1;
	}
	if (rc)
		LOGE("%s: New FW id is not complete: %s", __func__, buf);
	else
		LOGV("New FW: tts_ver 0x%lx, app_id 0x%lx,"
			" app_ver 0x%lx, cid 0x%lx\n",
			id->tts_ver, id->app_id, id->app_ver, id->cid);
	fclose(fp);
	return rc;
}

static int start_fw_loader(char *fname, int start)
{
	FILE *fp = fopen(fname, "w");
	if (fp) {
		fprintf(fp, "%d\n", !!start);
		fclose(fp);
		return 0;
	}
	LOGE("%s: Unable to open %s\n", __func__, fname);
	return -1;
}

static void usage(char *cmd)
{
	LOGE("update firmware:"
		" /system/bin/cyttsp_fwloader -dev <touchscreen device path>"
		" -fw <fw_hex_file>"
		" [-start] -stop [-force]\n");
	LOGE("check firmware version:"
		" /system/bin/cyttsp_fwloader -dev <touchscreen device path>"
		" -show_id\n");
	LOGE("execute bootloader command:"
		" /system/bin/cyttsp_fwloader -dev <touchscreen device path>"
		" -start -stop"
		" -cmd <command>\n");
	LOGE("write to device register:"
		" /system/bin/cyttsp_fwloader -dev <touchscreen device path>"
		"  -wr <reg_id> <reg_data>\n");
	LOGE("read from device register:"
		" /system/bin/cyttsp_fwloader -dev <touchscreen device path>"
		" -rd <reg_id>\n");
	LOGE("recalibrate device:"
		" /system/bin/cyttsp_fwloader -dev <touchscreen device path>"
		" -wr 27 1\n");
}

static unsigned char str2uc(char *str)
{
	char substr[3];
	char *p;
	unsigned char val;

	substr[0] = str[0];
	substr[1] = str[1];
	substr[2] = 0;
	val = strtoul(substr, &p, 16);
	if ((p - substr) != 2)
		LOGE("%s: error [%s]\n", __func__, substr);
	return val;
}

static void init_data_record(struct fw_record *rec, unsigned short addr)
{
	addr >>= 6;
	rec->blk_hi = (addr >> 8) & 0xff;
	rec->blk_lo = addr & 0xff;
	rec->rec_cs = rec->blk_hi + rec->blk_lo +
			(unsigned char)(BLK_SEED + BL_CMD_WRBLK + KEY_CS);
	rec->data_cs = 0;
}

static int check_record(char *rec)
{
	unsigned char r_len = str2uc(rec + REC_LEN_OFFSET);
	unsigned char type = str2uc(rec + REC_TYPE_OFFSET);
	unsigned short a;

	if (*rec != REC_START_CHR || r_len != DATA_REC_LEN || type != 0) {
		LOGD_IF(DEBUG, "record ignored %s", rec);
		return -1;
	}
	a = (str2uc(rec + REC_ADDR_HI_OFFSET) << 8) |
		str2uc(rec + REC_ADDR_LO_OFFSET);
	if (a >= START_ADDR || a == BL_REC1_ADDR || a == BL_REC2_ADDR)
		return 0;

	LOGD_IF(DEBUG, "record 0x%04x ignored\n", a);
	return -1;
}

static struct fw_record *prepare_record(char *rec)
{
	unsigned i;
	char *p;
	unsigned short addr = (str2uc(rec + REC_ADDR_HI_OFFSET) << 8) |
		str2uc(rec + REC_ADDR_LO_OFFSET);

	init_data_record(&data_record, addr);
	p = rec + REC_DATA_OFFSET;
	for (i = 0; i < DATA_REC_LEN; i++) {
		data_record.data[i] = str2uc(p);
		data_record.data_cs += data_record.data[i];
		data_record.rec_cs += data_record.data[i];
		p += 2;
	}
	data_record.rec_cs += data_record.data_cs;
	return &data_record;
}

static int wait_status(int fd, int (*condition)(unsigned short), int ms,
		       int tries)
{
	unsigned short status;
	ms *= 1000;
	do {
		usleep(ms);
		lseek(fd, 0, 0);
		if (sizeof(status) != read(fd, &status, sizeof(status))) {
			LOGE("%s status read error\n", __func__);
			return -1;
		}
	} while (tries-- && !condition(status));
	return tries < 0 ? -1 : 0;
}

static int flash_block(int fd, unsigned char *blk, int len)
{
	lseek(fd, 0, 0);
	if (len != write(fd, blk, len)) {
		LOGE("%s write error\n", __func__);
		return -1;
	}
	return 0;
}

static int flash_record(int fd, const struct fw_record *record)
{
	unsigned char *rec = (unsigned char *)record;
	unsigned char data[BLK_SIZE + 1];
	unsigned char blk_offset;
	int len = fw_rec_size;
	int blk_len;

	for (blk_offset = 0; len; len -= blk_len) {
		data[0] = blk_offset;
		blk_len = len > BLK_SIZE ? BLK_SIZE : len;
		memcpy(data + 1, rec, blk_len);
		rec += blk_len;
		if (flash_block(fd, data, blk_len + 1))
			return -1;
		blk_offset += blk_len;
	}
	return wait_status(fd, record->condition,
			   record->timeout_ms, record->num_tries);
}

static int flash_command(int fd, const struct cmd_record *record)
{
	if (flash_block(fd, (unsigned char *)record, cmd_rec_size))
		return -1;
	return wait_status(fd, record->condition,
			   record->timeout_ms, record->num_tries);
}

static int execute_cmd(int fd, char *cmd)
{
	unsigned char bin[16];
	int i;
	int cmdlen = strlen(cmd) / 2;

	if (!cmdlen)
		return -1;
	if (cmdlen > (int)sizeof(bin))
		cmdlen = sizeof(bin);
	for (i = 0; i < cmdlen; i++) {
		bin[i] = str2uc(cmd);
		cmd += 2;
	}
	lseek(fd, 0, 0);
	if (cmdlen != write(fd, bin, cmdlen)) {
		LOGE("%s write error\n", __func__);
		return -1;
	}
	return 0;
}

void flush_fw(char *fw_file, int fd)
{
	char *p;
	int i;
	int rc;
	struct fw_record *rec;
	static char buf[1024];
	FILE *fp = fopen(fw_file, "r");

	if (!fp) {
		LOGE("%s: Unable to open %s\n", __func__, fw_file);
		return;
	}
	if (flash_command(fd, &initiate_rec))
		goto exit;
	LOGV("Init record ok\n");

	while (!feof(fp)) {
		p = fgets(buf, sizeof(buf), fp);
		if (p) {
			if (check_record(p))
				continue;
			rec = prepare_record(p);
			rc = flash_record(fd, rec);
			if (rc)
				LOGE("Record 0x%02x%02x failed\n", rec->blk_hi,
					rec->blk_lo);
			else
				LOGD_IF(DEBUG, "Record 0x%02x%02x ok\n",
					rec->blk_hi, rec->blk_lo);
		}
	}
	if (flash_command(fd, &terminate_rec))
		goto exit;
	LOGV("Terminate record ok\n");
exit:
	fclose(fp);
}

static int start_loader(char *start_path, char *fw_path)
{
	int fd;

	if (access(fw_path, R_OK | W_OK)) {
		if (start_fw_loader(start_path, 1)) {
			LOGE("%s: Unable to start loader\n", __func__);
			return -1;
		}
		LOGV("Bootloader entered.\n");
	}
	fd = open(fw_path, O_RDWR | O_NONBLOCK);
	if (fd < 0) {
		LOGE("%s: Unable to open %s\n", __func__, fw_path);
			return -1;
	}
	if (wait_status(fd, cond_init, 100, 10)) {
		LOGE("%s: Loader start failed %s\n", __func__, fw_path);
		close(fd);
		return -1;
	}
	return fd;
}

static int start_dev(char *start_path, char *fw_path)
{
	int fd;

	if (access(fw_path, R_OK | W_OK)) {
		FILE *fp = fopen(start_path, "w");
		if (fp) {
			fprintf(fp, "%d\n", 2);
			fclose(fp);
		}
		else {
			LOGE("%s: Unable to open %s\n", __func__, start_path);
			return -1;
		}
	}
	fd = open(fw_path, O_RDWR | O_NONBLOCK);
	if (fd < 0) {
		LOGE("%s: Unable to open %s\n", __func__, fw_path);
			return -1;
	}
	return fd;
}

static int stop_dev(char *start_path)
{
	FILE *fp = fopen(start_path, "w");
	if (fp) {
		fprintf(fp, "%d\n", 3);
		fclose(fp);
		return 0;
	}
	LOGE("%s: Unable to open %s\n", __func__, start_path);
	return -1;
}

static int write_reg(int fd, unsigned char reg_id, unsigned char reg_data)
{
	unsigned char bin[2];
	int reglen = sizeof(bin);
	
	LOGV("%s write r=0x%02X d=0x%02X\n",
		__func__, reg_id, reg_data);
	bin[0] = reg_data;
	bin[1] = reg_id;
	lseek(fd, 0, 0);
	if (reglen != write(fd, bin, reglen)) {
		LOGE("%s write error r=0x%02X d=0x%02X\n",
			__func__, bin[0], bin[1]);
		return -1;
	}
	return 0;
}

static int read_reg(int fd, unsigned char reg_id, unsigned char *reg_data)
{
	unsigned char bin[2];
	int reglen = sizeof(bin);
	
	bin[0] = 0;
	bin[1] = reg_id | 0x80;
	lseek(fd, 0, 0);
	if (reglen != write(fd, bin, reglen)) {
		LOGE("%s read error r=0x%02X\n",
			__func__, bin[0]);
		return -1;
	}
	usleep(1000);
	lseek(fd, 0, 0);
	if (reglen != read(fd, bin, reglen)) {
		LOGE("%s status read error\n", __func__);
		return -1;
	}
	*reg_data = bin[1];
	LOGV("%s read r=0x%02X d=0x%02X\n",
		__func__, reg_id, *reg_data);
	return 0;
}

int main(int argc, char **argv)
{
	int i;
	char *fw_hex_name = NULL;
	char *dev_path = NULL;
	char *cmd = NULL;
	static char start_path[PATH_MAX];
	static char fw_path[PATH_MAX];
	int target_fd = -1;
	int start = 0;
	int stop = 0;
	int show_id = 0;
	int force_update = 0;
	struct fw_id current_fw_id;
	struct fw_id new_fw_id;
	unsigned char reg_id;
	unsigned char reg_data;
	int wr_data = 0;
	int wr_reg = 0;
	int rd_reg = 0;

	LOGV("Cypress TTSP FW Loader.");

	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-fw")) {
			if (++i < argc && argv[i])
				fw_hex_name = argv[i];
		} else if (!strcmp(argv[i], "-dev")) {
			if (++i < argc && argv[i])
				dev_path = argv[i];
		} else if (!strcmp(argv[i], "-cmd")) {
			if (++i < argc && argv[i])
				cmd = argv[i];
		} else if (!strcmp(argv[i], "-start")) {
			start = 1;
		} else if (!strcmp(argv[i], "-stop")) {
			stop = 1;
		} else if (!strcmp(argv[i], "-force")) {
			force_update = 1;
		} else if (!strcmp(argv[i], "-show_id")) {
			show_id = 1;
		} else if (!strcmp(argv[i], "-wr")) {
			if (++i < argc && argv[i]) {
				reg_id = atol(argv[i]);
				wr_reg = 1;
				if (++i < argc && argv[i]) {
					reg_data =  atol(argv[i]);
					wr_data = 1;
				}
			}
		} else if (!strcmp(argv[i], "-rd")) {
			if (++i < argc && argv[i]) {
				reg_id = atol(argv[i]);
				rd_reg = 1;
			}
		}
	}

	if (!dev_path || (wr_reg && rd_reg) || (wr_reg && !wr_data)) {
		usage(argv[0]);
		return -1;
	}
	
	strcpy(start_path, dev_path);
	if (start_path[strlen(start_path) - 1] != '/')
		strcat(start_path, "/");
	strcpy(fw_path, start_path);
	strcat(start_path, start_file);
	strcat(fw_path, fw_file);
	
	LOGV("start_path=\"%s\"", start_path);
	LOGV("   fw_path=\"%s\"", fw_path);

	if (access(start_path, R_OK | W_OK)) {
		LOGV("Not a Cypress TTSP touch screen or"
		     " FW loader is not supported.");
		return 0;
	}
	if (wr_reg && wr_data) {
		/* write register data to device */
		if (target_fd < 0) {
			target_fd = start_dev(start_path, fw_path);
			if (target_fd < 0)
				goto err_exit;
		}
		write_reg(target_fd, reg_id, reg_data);
		if (reg_id == RECAL_REG) {
			LOGV("Recal in process (wait 1/2 second)....\n");
			for (i = 0; i < 50; i++)
				usleep(10000);
		}
		stop_dev(start_path);
		return 0;
	}
	
	if (rd_reg) {
		/* read register data from device */
		if (target_fd < 0) {
			target_fd = start_dev(start_path, fw_path);
			if (target_fd < 0)
				goto err_exit;
		}
		read_reg(target_fd, reg_id, &reg_data);
		stop_dev(start_path);
		return 0;
	}
	
	if (start) {
		target_fd = start_loader(start_path, fw_path);
		if (target_fd < 0)
			goto err_exit;
	}
	if (show_id) {
		if (get_current_fw_id(start_path, &current_fw_id))
			goto err_exit;
	}
	if (cmd) {
		if (target_fd < 0) {
			target_fd = start_loader(start_path, fw_path);
			if (target_fd < 0)
				goto err_exit;
		}
		execute_cmd(target_fd, cmd);
		wait_status(target_fd, cond_dummy, 100, 1);
	}
	if (fw_hex_name) {
		if (get_current_fw_id(start_path, &current_fw_id))
			goto err_exit;
		if (get_fw_id(fw_hex_name, &new_fw_id))
			goto err_exit;
		if (force_update ||
			(current_fw_id.tts_ver != new_fw_id.tts_ver) ||
			(current_fw_id.app_id != new_fw_id.app_id) ||
			(current_fw_id.app_ver != new_fw_id.app_ver) ||
			(current_fw_id.cid != new_fw_id.cid)) {
			if (target_fd < 0) {
				target_fd = start_loader(start_path, fw_path);
				if (target_fd < 0)
					goto err_exit;
			}
			flush_fw(fw_hex_name, target_fd);
			if (start_fw_loader(start_path, 0)) {
				LOGE("%s: Unable to stop loader\n", __func__);
				goto err_exit;
			}
			stop = 0;
			if (get_current_fw_id(start_path, &current_fw_id))
				goto err_exit;
			if (!memcmp(&current_fw_id, &new_fw_id, sizeof(new_fw_id)))
				LOGV("Firmware updated.");
			else
				LOGE("Firmware update failed.");
		} else {
			LOGV("Firmware update is not required.");
		}
	}

err_exit:
	if (target_fd >= 0)
		close(target_fd);
	if (stop) {
		if (!access(fw_path, R_OK | W_OK)) {
			if (start_fw_loader(start_path, 0))
				LOGE("%s: Unable to stop loader\n", __func__);
		}
	}
	return 0;
}

