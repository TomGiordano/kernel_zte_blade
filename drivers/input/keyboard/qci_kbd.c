/* Quanta I2C Keyboard Driver
 *
 * Copyright (C) 2009 Quanta Computer Inc.
 * Author: Hsin Wu <hsin.wu@quantatw.com>
 * Author: Austin Lai <austin.lai@quantatw.com>
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

 /*
 *
 *  The Driver with I/O communications via the I2C Interface for ON2 of AP BU.
 *  And it is only working on the nuvoTon WPCE775x Embedded Controller.
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/jiffies.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/keyboard.h>
#include <linux/gpio.h>
#include <linux/delay.h>

/* Keyboard special scancode */
#define RC_KEY_FN          0x70
#define RC_KEY_BREAK       0x80
#define KEY_ACK_FA      0xFA

/* Keyboard keycodes */
#define NOKEY           KEY_RESERVED
#define KEY_LEFTWIN     KEY_LEFTMETA
#define KEY_RIGHTWIN    KEY_RIGHTMETA
#define KEY_APPS        KEY_COMPOSE
#define KEY_PRINTSCR    KEY_SYSRQ

#define KEYBOARD_ID_NAME          "qci-i2ckbd"
#define KEYBOARD_NAME                "Quanta Keyboard"
#define KEYBOARD_DEVICE             "/i2c/input0"
#define KEYBOARD_CMD_ENABLE             0xF4

/*-----------------------------------------------------------------------------
 * Keyboard scancode to linux keycode translation table
 *---------------------------------------------------------------------------*/

static const unsigned char on2_keycode[256] = {
	[0]   = NOKEY,
	[1]   = NOKEY,
	[2]   = NOKEY,
	[3]   = KEY_5,
	[4]   = KEY_7,
	[5]   = KEY_9,
	[6]   = KEY_MINUS,
	[7]   = NOKEY,
	[8]   = NOKEY,
	[9]   = NOKEY,
	[10]  = NOKEY,
	[11]  = KEY_LEFTBRACE,
	[12]  = KEY_F10,
	[13]  = KEY_INSERT,
	[14]  = KEY_F11,
	[15]  = KEY_ESC,
	[16]  = NOKEY,
	[17]  = NOKEY,
	[18]  = NOKEY,
	[19]  = KEY_4,
	[20]  = KEY_6,
	[21]  = KEY_8,
	[22]  = KEY_0,
	[23]  = KEY_EQUAL,
	[24]  = NOKEY,
	[25]  = NOKEY,
	[26]  = NOKEY,
	[27]  = KEY_P,
	[28]  = KEY_F9,
	[29]  = KEY_DELETE,
	[30]  = KEY_F12,
	[31]  = KEY_GRAVE,
	[32]  = KEY_W,
	[33]  = NOKEY,
	[34]  = NOKEY,
	[35]  = KEY_R,
	[36]  = KEY_T,
	[37]  = KEY_U,
	[38]  = KEY_O,
	[39]  = KEY_RIGHTBRACE,
	[40]  = NOKEY,
	[41]  = NOKEY,
	[42]  = NOKEY,
	[43]  = KEY_APOSTROPHE,
	[44]  = KEY_BACKSPACE,
	[45]  = NOKEY,
	[46]  = KEY_F8,
	[47]  = KEY_F5,
	[48]  = KEY_S,
	[49]  = NOKEY,
	[50]  = NOKEY,
	[51]  = KEY_E,
	[52]  = KEY_H,
	[53]  = KEY_Y,
	[54]  = KEY_I,
	[55]  = KEY_ENTER,
	[56]  = NOKEY,
	[57]  = NOKEY,
	[58]  = NOKEY,
	[59]  = KEY_SEMICOLON,
	[60]  = KEY_3,
	[61]  = KEY_PAGEUP,
	[62]  = KEY_Q,
	[63]  = KEY_TAB,
	[64]  = KEY_A,
	[65]  = NOKEY,
	[66]  = NOKEY,
	[67]  = KEY_F,
	[68]  = KEY_G,
	[69]  = KEY_J,
	[70]  = KEY_L,
	[71]  = NOKEY,
	[72]  = KEY_RIGHTSHIFT,
	[73]  = NOKEY,
	[74]  = NOKEY,
	[75]  = KEY_SLASH,
	[76]  = KEY_2,
	[77]  = KEY_PAGEDOWN,
	[78]  = KEY_F4,
	[79]  = KEY_F1,
	[80]  = KEY_Z,
	[81]  = NOKEY,
	[82]  = NOKEY,
	[83]  = KEY_D,
	[84]  = KEY_V,
	[85]  = KEY_N,
	[86]  = KEY_K,
	[87]  = NOKEY,
	[88]  = KEY_LEFTSHIFT,
	[89]  = KEY_RIGHTCTRL,
	[90]  = NOKEY,
	[91]  = KEY_DOT,
	[92]  = KEY_UP,
	[93]  = KEY_RIGHT,
	[94]  = KEY_F3,
	[95]  = KEY_F2,
	[96]  = NOKEY,
	[97]  = NOKEY,
	[98]  = KEY_RIGHTALT,
	[99]  = KEY_X,
	[100] = KEY_C,
	[101] = KEY_B,
	[102] = KEY_COMMA,
	[103] = NOKEY,
	[104] = NOKEY,
	[105] = NOKEY,
	[106] = NOKEY,
	[107] = NOKEY,
	[108] = KEY_PRINTSCR,
	[109] = KEY_DOWN,
	[110] = KEY_1,
	[111] = KEY_CAPSLOCK,
	[112] = KEY_F24,
	[113] = KEY_HOME,
	[114] = KEY_LEFTALT,
	[115] = NOKEY,
	[116] = KEY_SPACE,
	[117] = KEY_BACKSLASH,
	[118] = KEY_M,
	[119] = KEY_COMPOSE,
	[120] = NOKEY,
	[121] = KEY_LEFTCTRL,
	[122] = NOKEY,
	[123] = NOKEY,
	[124] = KEY_PAUSE,
	[125] = KEY_LEFT,
	[126] = KEY_F7,
	[127] = KEY_F6,
	[128] = NOKEY,
	[129] = NOKEY,
	[130] = NOKEY,
	[131] = NOKEY,
	[132] = NOKEY,
	[133] = NOKEY,
	[134] = NOKEY,
	[135] = NOKEY,
	[136] = NOKEY,
	[137] = NOKEY,
	[138] = NOKEY,
	[139] = NOKEY,
	[140] = NOKEY,
	[141] = NOKEY,
	[142] = NOKEY,
	[143] = NOKEY,
	[144] = NOKEY,
	[145] = NOKEY,
	[146] = NOKEY,
	[147] = NOKEY,
	[148] = NOKEY,
	[149] = NOKEY,
	[150] = NOKEY,
	[151] = NOKEY,
	[152] = NOKEY,
	[153] = NOKEY,
	[154] = NOKEY,
	[155] = NOKEY,
	[156] = NOKEY,
	[157] = NOKEY,
	[158] = NOKEY,
	[159] = NOKEY,
	[160] = NOKEY,
	[161] = NOKEY,
	[162] = NOKEY,
	[163] = NOKEY,
	[164] = NOKEY,
	[165] = NOKEY,
	[166] = NOKEY,
	[167] = NOKEY,
	[168] = NOKEY,
	[169] = NOKEY,
	[170] = NOKEY,
	[171] = NOKEY,
	[172] = NOKEY,
	[173] = NOKEY,
	[174] = NOKEY,
	[175] = NOKEY,
	[176] = NOKEY,
	[177] = NOKEY,
	[178] = NOKEY,
	[179] = NOKEY,
	[180] = NOKEY,
	[181] = NOKEY,
	[182] = NOKEY,
	[183] = NOKEY,
	[184] = NOKEY,
	[185] = NOKEY,
	[186] = NOKEY,
	[187] = NOKEY,
	[188] = NOKEY,
	[189] = KEY_HOME,
	[190] = NOKEY,
	[191] = NOKEY,
	[192] = NOKEY,
	[193] = NOKEY,
	[194] = NOKEY,
	[195] = NOKEY,
	[196] = NOKEY,
	[197] = NOKEY,
	[198] = NOKEY,
	[199] = NOKEY,
	[200] = NOKEY,
	[201] = NOKEY,
	[202] = NOKEY,
	[203] = NOKEY,
	[204] = NOKEY,
	[205] = KEY_END,
	[206] = NOKEY,
	[207] = NOKEY,
	[208] = NOKEY,
	[209] = NOKEY,
	[210] = NOKEY,
	[211] = NOKEY,
	[212] = NOKEY,
	[213] = NOKEY,
	[214] = NOKEY,
	[215] = NOKEY,
	[216] = NOKEY,
	[217] = NOKEY,
	[218] = NOKEY,
	[219] = NOKEY,
	[220] = KEY_VOLUMEUP,
	[221] = KEY_BRIGHTNESSUP,
	[222] = NOKEY,
	[223] = NOKEY,
	[224] = NOKEY,
	[225] = NOKEY,
	[226] = NOKEY,
	[227] = NOKEY,
	[228] = NOKEY,
	[229] = NOKEY,
	[230] = NOKEY,
	[231] = NOKEY,
	[232] = NOKEY,
	[233] = NOKEY,
	[234] = NOKEY,
	[235] = NOKEY,
	[236] = NOKEY,
	[237] = KEY_VOLUMEDOWN,
	[238] = NOKEY,
	[239] = NOKEY,
	[240] = NOKEY,
	[241] = NOKEY,
	[242] = NOKEY,
	[243] = NOKEY,
	[244] = NOKEY,
	[245] = NOKEY,
	[246] = NOKEY,
	[247] = NOKEY,
	[248] = NOKEY,
	[249] = NOKEY,
	[250] = NOKEY,
	[251] = NOKEY,
	[252] = NOKEY,
	[253] = KEY_BRIGHTNESSDOWN,
	[254] = NOKEY,
	[255] = NOKEY,
};
/*-----------------------------------------------------------------------------
 * Global variables
 *---------------------------------------------------------------------------*/

struct input_dev *g_qci_keyboard_dev;

/* General structure to hold the driver data */
struct i2ckbd_drv_data {
	struct i2c_client *ki2c_client;
	struct work_struct work;
	struct input_dev *qcikbd_dev;
	unsigned int qcikbd_gpio; /* GPIO used for interrupt */
	unsigned int qcikbd_irq;
	unsigned int key_down;
	unsigned int escape;
	unsigned int pause_seq;
	unsigned int fn;
};
#ifdef CONFIG_PM
static int qcikbd_suspend(struct device *dev)
{
	return 0;
}

static int qcikbd_resume(struct device *dev)
{
	return 0;
}
#endif
static int __devinit qcikbd_probe(struct i2c_client *client,
	const struct i2c_device_id *id);
static int __devexit qcikbd_remove(struct i2c_client *kbd);

static const struct i2c_device_id qcikbd_idtable[] = {
	{ KEYBOARD_ID_NAME, 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, qcikbd_idtable);

#ifdef CONFIG_PM
static struct dev_pm_ops qcikbd_pm_ops = {
	.suspend  = qcikbd_suspend,
	.resume   = qcikbd_resume,
};
#endif
static struct i2c_driver i2ckbd_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name  = KEYBOARD_ID_NAME,
#ifdef CONFIG_PM
		.pm = &qcikbd_pm_ops,
#endif
	},
	.probe	  = qcikbd_probe,
	.remove = __devexit_p(qcikbd_remove),
	.id_table = qcikbd_idtable,
};

/*-----------------------------------------------------------------------------
 * Driver functions
 *---------------------------------------------------------------------------*/

static irqreturn_t qcikbd_interrupt(int irq, void *dev_id)
{
	struct i2ckbd_drv_data *ikbd_drv_data = dev_id;
	schedule_work(&ikbd_drv_data->work);
	return IRQ_HANDLED;
}

static void qcikbd_work_handler(struct work_struct *_work)
{
	unsigned char scancode;
	unsigned int  keycode;

	struct i2ckbd_drv_data *ikbd_drv_data =
		container_of(_work, struct i2ckbd_drv_data, work);

	struct i2c_client *ikbdclient = ikbd_drv_data->ki2c_client;
	struct input_dev *ikbdev = ikbd_drv_data->qcikbd_dev;

	scancode = i2c_smbus_read_byte(ikbdclient);

	if (scancode == KEY_ACK_FA) {
		return;
	} else if (scancode == RC_KEY_FN) {
		ikbd_drv_data->fn = 0x80;     /* select keycode table  > 0x7F */
	} else {
		ikbd_drv_data->key_down = 1;
		if (scancode & RC_KEY_BREAK) {
			ikbd_drv_data->key_down = 0;
			if ((scancode & 0x7F) == RC_KEY_FN)
				ikbd_drv_data->fn = 0;
		}
		keycode = on2_keycode[(scancode & 0x7F) | ikbd_drv_data->fn];
		if (keycode != NOKEY) {
			input_report_key(ikbdev,
					 keycode,
					 ikbd_drv_data->key_down);
			input_sync(ikbdev);
		}
	}
}


static int qcikbd_open(struct input_dev *dev)
{
	struct i2ckbd_drv_data *ikbd_drv_data = input_get_drvdata(dev);
	struct i2c_client *ikbdclient = ikbd_drv_data->ki2c_client;

	/* Send F4h - enable keyboard */
	i2c_smbus_write_byte(ikbdclient, KEYBOARD_CMD_ENABLE);
	return 0;
}

static int __devinit qcikbd_probe(struct i2c_client *client,
				    const struct i2c_device_id *id)
{
	int err;
	int i;
	struct i2ckbd_drv_data *context;
	context = kzalloc(sizeof(struct i2ckbd_drv_data), GFP_KERNEL);
	if (!context)
		return -ENOMEM;
	i2c_set_clientdata(client, context);
	context->ki2c_client = client;
	context->qcikbd_gpio = client->irq;
	client->driver = &i2ckbd_driver;

	INIT_WORK(&context->work, qcikbd_work_handler);

	err = gpio_request(context->qcikbd_gpio, "qci-kbd");
	if (err) {
		pr_err("[KBD] err gpio request\n");
		goto gpio_request_fail;
	}

	context->qcikbd_irq = gpio_to_irq(context->qcikbd_gpio);
	err = request_irq(context->qcikbd_irq,
			  qcikbd_interrupt,
			  IRQF_TRIGGER_FALLING,
			  KEYBOARD_ID_NAME,
			  context);
	if (err) {
		pr_err("[KBD] err unable to get IRQ\n");
		goto request_irq_fail;
	}

	context->qcikbd_dev = input_allocate_device();
	if (!context->qcikbd_dev) {
		pr_err("[KBD]allocting memory err\n");
		err = -ENOMEM;
		goto allocate_fail;
	}

	context->qcikbd_dev->name       = KEYBOARD_NAME;
	context->qcikbd_dev->phys       = KEYBOARD_DEVICE;
	context->qcikbd_dev->id.bustype = BUS_I2C;
	context->qcikbd_dev->id.vendor  = 0x1050;
	context->qcikbd_dev->id.product = 0x0006;
	context->qcikbd_dev->id.version = 0x0004;
	context->qcikbd_dev->open       = qcikbd_open;
	set_bit(EV_KEY, context->qcikbd_dev->evbit);
	set_bit(EV_REP, context->qcikbd_dev->evbit);

	/* Enable all supported keys */
	for (i = 1; i < ARRAY_SIZE(on2_keycode) ; i++)
		set_bit(on2_keycode[i], context->qcikbd_dev->keybit);

	set_bit(KEY_POWER, context->qcikbd_dev->keybit);
	set_bit(KEY_END, context->qcikbd_dev->keybit);
	set_bit(KEY_VOLUMEUP, context->qcikbd_dev->keybit);
	set_bit(KEY_VOLUMEDOWN, context->qcikbd_dev->keybit);
	set_bit(KEY_ZOOMIN, context->qcikbd_dev->keybit);
	set_bit(KEY_ZOOMOUT, context->qcikbd_dev->keybit);

	input_set_drvdata(context->qcikbd_dev, context);
	err = input_register_device(context->qcikbd_dev);
	if (err) {
		pr_err("[KBD] err input register device\n");
		goto register_fail;
	}
	g_qci_keyboard_dev = context->qcikbd_dev;
	return 0;
register_fail:
	input_free_device(context->qcikbd_dev);

allocate_fail:
	free_irq(context->qcikbd_irq, context);

request_irq_fail:
	gpio_free(context->qcikbd_gpio);

gpio_request_fail:
	i2c_set_clientdata(client, NULL);
	kfree(context);
	return err;
}

static int __devexit qcikbd_remove(struct i2c_client *dev)
{
	struct i2ckbd_drv_data *context = i2c_get_clientdata(dev);

	free_irq(context->qcikbd_irq, context);
	gpio_free(context->qcikbd_gpio);
	input_free_device(context->qcikbd_dev);
	input_unregister_device(context->qcikbd_dev);
	kfree(context);

	return 0;
}

static int __init qcikbd_init(void)
{
	return i2c_add_driver(&i2ckbd_driver);
}

static void __exit qcikbd_exit(void)
{
	i2c_del_driver(&i2ckbd_driver);
}

struct input_dev *nkbc_keypad_get_input_dev(void)
{
	return g_qci_keyboard_dev;
}
EXPORT_SYMBOL(nkbc_keypad_get_input_dev);
module_init(qcikbd_init);
module_exit(qcikbd_exit);

MODULE_AUTHOR("Quanta Computer Inc.");
MODULE_DESCRIPTION("Quanta Embedded Controller I2C Keyboard Driver");
MODULE_LICENSE("GPL v2");

