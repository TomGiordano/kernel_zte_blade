/* Source for:
 * Cypress TrueTouch(TM) Standard Product I2C touchscreen driver.
 * drivers/input/touchscreen/cyttsp_spi.c
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/cyttsp.h>
#include "cyttsp_core.h"

#define DBG(x)

#define CY_SPI_WR_OP      0x00 /* r/~w */
#define CY_SPI_RD_OP      0x01
#define CY_SPI_CMD_BYTES  4
#define CY_SPI_SYNC_BYTE  2
#define CY_SPI_SYNC_ACK1  0x62
#define CY_SPI_SYNC_ACK2  0x9D
#define CY_SPI_DATA_SIZE  128
#define CY_SPI_DATA_BUF_SIZE (CY_SPI_CMD_BYTES + CY_SPI_DATA_SIZE)
#define CY_SPI_BITS_PER_WORD 8

struct cyttsp_spi {
	struct cyttsp_bus_ops ops;
	struct spi_device *spi_client;
	void *ttsp_client;
	u8 wr_buf[CY_SPI_DATA_BUF_SIZE];
	u8 rd_buf[CY_SPI_DATA_BUF_SIZE];
};

static void spi_complete(void *arg)
{
	complete(arg);
}

static int spi_sync_tmo(struct spi_device *spi, struct spi_message *message)
{
	DECLARE_COMPLETION_ONSTACK(done);
	int status;
	DBG(printk(KERN_INFO"%s: Enter\n", __func__);)

	message->complete = spi_complete;
	message->context = &done;
	status = spi_async(spi, message);
	if (status == 0) {
		int ret = wait_for_completion_interruptible_timeout(&done, HZ);
		if (!ret) {
			printk(KERN_ERR "%s: timeout\n", __func__);
			status = -EIO;
		} else
			status = message->status;
	}
	message->context = NULL;
	return status;
}

static int cyttsp_spi_xfer_(u8 op, struct cyttsp_spi *ts_spi,
			    u8 reg, u8 *buf, int length)
{
	struct spi_message msg;
	struct spi_transfer xfer[2];
	u8 *wr_buf = ts_spi->wr_buf;
	u8 *rd_buf = ts_spi->rd_buf;
	int retval;
	DBG(printk(KERN_INFO "%s: Enter\n", __func__);)
	if (length > CY_SPI_DATA_SIZE) {
		printk(KERN_ERR "%s: length %d is too big.\n",
			__func__, length);
		return -EINVAL;
	}
	DBG(printk(KERN_INFO "%s: OP=%s length=%d\n", __func__,
		   op == CY_SPI_RD_OP ? "Read" : "Write", length);)

	memset(wr_buf, 0, CY_SPI_DATA_BUF_SIZE);
	memset(rd_buf, 0, CY_SPI_DATA_BUF_SIZE);

	wr_buf[0] = 0x00; /* header byte 0 */
	wr_buf[1] = 0xFF; /* header byte 1 */
	wr_buf[2] = reg;  /* reg index */
	wr_buf[3] = op;   /* r/~w */
	if (op == CY_SPI_WR_OP)
		memcpy(wr_buf + CY_SPI_CMD_BYTES, buf, length);
	DBG(
	if (op == CY_SPI_RD_OP)
		memset(rd_buf, CY_SPI_SYNC_NACK, CY_SPI_DATA_BUF_SIZE);)
	DBG(
	for (i = 0; i < (length + CY_SPI_CMD_BYTES); i++) {
		if ((op == CY_SPI_RD_OP) && (i < CY_SPI_CMD_BYTES))
			printk(KERN_INFO "%s: read op. wr[%d]:0x%02x\n",
				__func__, i, wr_buf[i]);
		if (op == CY_SPI_WR_OP)
			printk(KERN_INFO "%s: write op. wr[%d]:0x%02x\n",
				__func__, i, wr_buf[i]);
	})

	memset((void *)xfer, 0, sizeof(xfer));
	spi_message_init(&msg);
	xfer[0].tx_buf = wr_buf;
	xfer[0].rx_buf = rd_buf;
	if (op == CY_SPI_WR_OP) {
		xfer[0].len = length + CY_SPI_CMD_BYTES;
		spi_message_add_tail(&xfer[0], &msg);
	} else if (op == CY_SPI_RD_OP) {
		xfer[0].len = CY_SPI_CMD_BYTES;
		spi_message_add_tail(&xfer[0], &msg);

		xfer[1].rx_buf = buf;
		xfer[1].len = length;
		spi_message_add_tail(&xfer[1], &msg);
	}

	retval = spi_sync_tmo(ts_spi->spi_client, &msg);
	if (retval < 0) {
		printk(KERN_ERR "%s: spi sync error %d, len=%d, op=%d\n",
			__func__, retval, xfer[1].len, op);
		retval = 0;
	}

	if ((rd_buf[CY_SPI_SYNC_BYTE] == CY_SPI_SYNC_ACK1) &&
		(rd_buf[CY_SPI_SYNC_BYTE+1] == CY_SPI_SYNC_ACK2))
		retval = 0;
	else {
		DBG(
		for (i = 0; i < (CY_SPI_CMD_BYTES); i++)
			printk(KERN_INFO "%s: test rd_buf[%d]:0x%02x\n",
				__func__, i, rd_buf[i]);
		for (i = 0; i < (length); i++)
			printk(KERN_INFO "%s: test buf[%d]:0x%02x\n",
				__func__, i, buf[i]);)
		retval = 1;
	}
	return retval;
}

static int cyttsp_spi_xfer(u8 op, struct cyttsp_spi *ts,
			    u8 reg, u8 *buf, int length)
{
	int tries;
	int retval;
	DBG(printk(KERN_INFO "%s: Enter\n", __func__);)

	if (op == CY_SPI_RD_OP) {
		for (tries = CY_NUM_RETRY; tries; tries--) {
			retval = cyttsp_spi_xfer_(op, ts, reg, buf, length);
			if (retval == 0)
				break;
			else
				msleep(10);
		}
	} else {
		retval = cyttsp_spi_xfer_(op, ts, reg, buf, length);
	}
	return retval;
}

static s32 ttsp_spi_read_block_data(void *handle, u8 addr,
				    u8 length, void *data)
{
	int retval;
	struct cyttsp_spi *ts = container_of(handle, struct cyttsp_spi, ops);

	DBG(printk(KERN_INFO "%s: Enter\n", __func__);)

	retval = cyttsp_spi_xfer(CY_SPI_RD_OP, ts, addr, data, length);
	if (retval < 0)
		printk(KERN_ERR "%s: ttsp_spi_read_block_data failed\n",
			__func__);

	/* Do not print the above error if the data sync bytes were not found.
	   This is a normal condition for the bootloader loader startup and need
	   to retry until data sync bytes are found. */
	if (retval > 0)
		retval = -1;	/* now signal fail; so retry can be done */

	return retval;
}

static s32 ttsp_spi_write_block_data(void *handle, u8 addr,
				     u8 length, const void *data)
{
	int retval;
	struct cyttsp_spi *ts = container_of(handle, struct cyttsp_spi, ops);

	DBG(printk(KERN_INFO "%s: Enter\n", __func__);)

	retval = cyttsp_spi_xfer(CY_SPI_WR_OP, ts, addr, (void *)data, length);
	if (retval < 0)
		printk(KERN_ERR "%s: ttsp_spi_write_block_data failed\n",
			__func__);

	/* Do not print the above error if the data sync bytes were not found.
	   This is a normal condition for the bootloader loader startup and need
	   to retry until data sync bytes are found. */
	if (retval > 0)
		retval = -1;	/* now signal fail; so retry can be done */

	return retval;
}

static s32 ttsp_spi_tch_ext(void *handle, void *values)
{
	int retval = 0;
	struct cyttsp_spi *ts = container_of(handle, struct cyttsp_spi, ops);

	DBG(printk(KERN_INFO "%s: Enter\n", __func__);)

	/* Add custom touch extension handling code here */
	/* set: retval < 0 for any returned system errors,
		retval = 0 if normal touch handling is required,
		retval > 0 if normal touch handling is *not* required */
	if (!ts || !values)
		retval = -EIO;

	return retval;
}

static int __devinit cyttsp_spi_probe(struct spi_device *spi)
{
	struct cyttsp_spi *ts_spi;
	int retval;
	DBG(printk(KERN_INFO"%s: Enter\n", __func__);)

	/* Set up SPI*/
	spi->bits_per_word = CY_SPI_BITS_PER_WORD;
	spi->mode = SPI_MODE_0;
	retval = spi_setup(spi);
	if (retval < 0) {
		printk(KERN_ERR "%s: SPI setup error %d\n", __func__, retval);
		return retval;
	}
	ts_spi = kzalloc(sizeof(*ts_spi), GFP_KERNEL);
	if (ts_spi == NULL) {
		printk(KERN_ERR "%s: Error, kzalloc\n", __func__);
		retval = -ENOMEM;
		goto error_alloc_data_failed;
	}
	ts_spi->spi_client = spi;
	dev_set_drvdata(&spi->dev, ts_spi);
	ts_spi->ops.write = ttsp_spi_write_block_data;
	ts_spi->ops.read = ttsp_spi_read_block_data;
	ts_spi->ops.ext = ttsp_spi_tch_ext;

	ts_spi->ttsp_client = cyttsp_core_init(&ts_spi->ops, &spi->dev);
	if (!ts_spi->ttsp_client)
		goto ttsp_core_err;
	printk(KERN_INFO "%s: Successful registration %s\n",
	       __func__, CY_SPI_NAME);

	return 0;

ttsp_core_err:
	kfree(ts_spi);
error_alloc_data_failed:
	return retval;
}

/* registered in driver struct */
static int __devexit cyttsp_spi_remove(struct spi_device *spi)
{
	struct cyttsp_spi *ts_spi = dev_get_drvdata(&spi->dev);
	DBG(printk(KERN_INFO"%s: Enter\n", __func__);)
	cyttsp_core_release(ts_spi->ttsp_client);
	kfree(ts_spi);
	return 0;
}


static struct spi_driver cyttsp_spi_driver = {
	.driver = {
		.name = CY_SPI_NAME,
		.bus = &spi_bus_type,
		.owner = THIS_MODULE,
	},
	.probe = cyttsp_spi_probe,
	.remove = __devexit_p(cyttsp_spi_remove),
};

static int __init cyttsp_spi_init(void)
{
	int err;

	err = spi_register_driver(&cyttsp_spi_driver);
	printk(KERN_INFO "%s: Cypress TrueTouch(R) Standard Product SPI "
		"Touchscreen Driver (Built %s @ %s) returned %d\n",
		 __func__, __DATE__, __TIME__, err);

	return err;
}
module_init(cyttsp_spi_init);

static void __exit cyttsp_spi_exit(void)
{
	spi_unregister_driver(&cyttsp_spi_driver);
	printk(KERN_INFO "%s: module exit\n", __func__);
}
module_exit(cyttsp_spi_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Cypress TrueTouch(R) Standard Product SPI driver");
MODULE_AUTHOR("Cypress");

