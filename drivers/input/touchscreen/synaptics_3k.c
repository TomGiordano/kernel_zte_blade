/* drivers/input/keyboard/synaptics_i2c_rmi.c
 *
 * Copyright (C) 2007 Google, Inc.
 * Copyright (C) 2008 Texas Instrument Inc.
 * Copyright (C) 2009 Synaptics, Inc.
 *
 * provides device files /dev/input/event#
 * for named device files, use udev
 * 2D sensors report ABS_X_FINGER(0), ABS_Y_FINGER(0) through ABS_X_FINGER(7), ABS_Y_FINGER(7)
 * NOTE: requires updated input.h, which should be included with this driver
 * 1D/Buttons report BTN_0 through BTN_0 + button_count
 * TODO: report REL_X, REL_Y for flick, BTN_TOUCH for tap (on 1D/0D; done for 2D)
 * TODO: check ioctl (EVIOCGABS) to query 2D max X & Y, 1D button count
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

#include <linux/module.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/hrtimer.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/earlysuspend.h>
#include <linux/input/synaptics_i2c_rmi.h>
#include <linux/proc_fs.h>

static struct workqueue_struct *synaptics_wq;

#ifdef CONFIG_HAS_EARLYSUSPEND
static void synaptics_rmi4_early_suspend(struct early_suspend *h);
static void synaptics_rmi4_late_resume(struct early_suspend *h);
#endif

#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_I2C_RMI_UPDATE)
extern void ioctol(struct i2c_client *client);
static struct i2c_client *update_client;
#endif 

#if defined(CONFIG_TOUCHSCREEN_VIRTUAL_KEYS)
#define virtualkeys virtualkeys.synaptics-rmi4-ts
#if defined(CONFIG_MACH_V9PLUS)
static const char ts_keys_size[] = "0x01:102:100:1061:200:74:0x01:139:300:1061:200:74:0x01:158:500:1061:200:74";
#elif defined(CONFIG_MACH_SKATE)
static const char ts_keys_size[] = "0x01:102:43:850:60:50:0x01:139:235:850:60:50:0x01:158:436:850:60:50";
#else
static const char ts_keys_size[] = "0x01:102:51:503:102:1007:0x01:139:158:503:102:1007:0x01:158:266:503:102:1007";
#endif 
 
static ssize_t virtualkeys_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	sprintf(buf,"%s\n",ts_keys_size);
//	pr_info("%s\n",__FUNCTION__);
    return strlen(ts_keys_size)+2;
}
static DEVICE_ATTR(virtualkeys, 0444, virtualkeys_show, NULL);
extern struct kobject *android_touch_kobj;
static struct kobject * virtual_key_kobj;
static int ts_key_report_init(void)
{
	int ret;
	virtual_key_kobj = kobject_get(android_touch_kobj);
	if (virtual_key_kobj == NULL) {
		virtual_key_kobj = kobject_create_and_add("board_properties", NULL);
		if (virtual_key_kobj == NULL) {
			pr_err("%s: subsystem_register failed\n", __func__);
			ret = -ENOMEM;
			return ret;
		}
	}
 
	ret = sysfs_create_file(virtual_key_kobj, &dev_attr_virtualkeys.attr);
	if (ret) {
		pr_err("%s: sysfs_create_file failed\n", __func__);
		return ret;
	}
 
	return 0;
}
static void ts_key_report_deinit(void)
{
	sysfs_remove_file(virtual_key_kobj, &dev_attr_virtualkeys.attr);
	kobject_del(virtual_key_kobj);
}
#endif



#ifdef TOUCHSCREEN_DUPLICATED_FILTER
static int duplicated_filter(struct synaptics_ts_data *ts, int x,int y,int x2,int y2,
						const int finger2, const int z)
{
	int drift_x[2];
	int drift_y[2];
	static int ref_x[2], ref_y[2];
	uint8_t discard[2] = {0, 0};

	drift_x[0] = abs(ref_x[0] - x);
	drift_y[0] = abs(ref_y[0] - y);
	if (finger2) {
		drift_x[1] = abs(ref_x[1] - x2);
		drift_y[1] = abs(ref_y[1] - y2);
	}
	/* pr_info("ref_x :%d, ref_y: %d, x: %d, y: %d\n", ref_x, ref_y, pos[0][0], pos[0][1]); */
	if (drift_x[0] < ts->dup_threshold && drift_y[0] < ts->dup_threshold && z != 0) {
		/* pr_info("ref_x :%d, ref_y: %d, x: %d, y: %d\n", ref_x[0], ref_y[0], pos[0][0], pos[0][1]); */
		discard[0] = 1;
	}
	if (!finger2 || (drift_x[1] < ts->dup_threshold && drift_y[1] < ts->dup_threshold)) {
		discard[1] = 1;
	}
	if (discard[0] && discard[1]) {
		/* if finger 0 and finger 1's movement < threshold , discard it. */
		return 1;
	}
	ref_x[0] = x;
	ref_y[0] = y;
	if (finger2) {
		ref_x[1] = x2;
		ref_y[1] = y2;
	}
	if (z == 0) {
		ref_x[0] = ref_y[0] = 0;
		ref_x[1] = ref_y[1] = 0;
	}

	return 0;
}
#endif /* TOUCHSCREEN_DUPLICATED_FILTER */

/*
static void dump_regs(struct synaptics_rmi4_data *ts)
{
	int ret=0;
	int i;
	int buf_len=32;
	uint8_t buf[buf_len];

	ret = i2c_smbus_read_i2c_block_data(ts->client,
		ts->data_base, buf_len,buf);
	if (ret < 0){
		pr_err("ts dump_regs: failed to get ts intr state\n");
	}else{
		for (i=0;i<buf_len;i++)
			pr_info("addr=0x%x, value=0x%x\n",
				ts->data_base+i,buf[i]);
	}
}
*/
#if defined (CONFIG_TOUCHSCREEN_SYNAPTICS_I2C_RMI_UPDATE)
int update_result_flag=0;
struct synaptics_rmi4_data *ts_temp;
#endif

static int
proc_read_val(char *page, char **start, off_t off, int count, int *eof,
			  void *data)
{
			int len = 0;
			len += sprintf(page + len, "%s\n", "touchscreen module");
			len += sprintf(page + len, "name	 : %s\n", "synaptics");
	#if defined(CONFIG_MACH_R750)
			len += sprintf(page + len, "i2c address  : %x\n", 0x23);
	#else
			len += sprintf(page + len, "i2c address  : 0x%x\n", 0x22);
	#endif
	#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_3K)
			len += sprintf(page + len, "IC type    : %s\n", "3000 series");
	#else
			len += sprintf(page + len, "IC type    : %s\n", "2000 series");
	#endif
	#if defined(CONFIG_MACH_R750)
			len += sprintf(page + len, "firmware version	: %s\n", "TM1551");
	#elif  defined(CONFIG_MACH_JOE)
			len += sprintf(page + len, "firmware version	: %s\n", "TM1419-001");
	#elif  defined(CONFIG_MACH_BLADE)
			len += sprintf(page + len, "firmware version	: %s\n", "TM1541");
	#elif  defined(CONFIG_MACH_SKATE)
			len += sprintf(page + len, "firmware version	: %s\n", "TM1792");	
	#endif
			len += sprintf(page + len, "module : %s\n", "synaptics + TPK");
	#ifdef CONFIG_TOUCHSCREEN_SYNAPTICS_I2C_RMI_UPDATE
	  len += sprintf(page + len, "update flag : 0x%x\n", update_result_flag);
	#endif
			if (off + count >= len)
				*eof = 1;
			if (len < off)
				return 0;
			*start = page + off;
			return ((count < len - off) ? count : len - off);
		}
		//ZTE_WLY_CRDB00509514,END
		
static int proc_write_val(struct file *file, const char *buffer,
		   unsigned long count, void *data)
{
	unsigned long val;
	#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_I2C_RMI_UPDATE)
	int ret=0;
	update_result_flag=0;
	disable_irq(update_client->irq);
	ioctol(update_client);
	msleep(500);
	enable_irq(update_client->irq);

	ret = i2c_smbus_write_byte_data(update_client, ts_temp->f11.ctrl_base+2, 0x9);
	if (ret<0) pr_err("i2c_smbus_write_byte_data failed\n");
	ret = i2c_smbus_write_byte_data(update_client, ts_temp->f11.ctrl_base+3, 0x9);	
	if (ret<0) pr_err("i2c_smbus_write_byte_data failed\n");
	update_result_flag=2;
	#endif
	sscanf(buffer, "%lu", &val);				
	printk("ts_key_report_synaptics_show\n");
	return -EINVAL;
}

static int synaptics_rmi4_get_pdt(struct synaptics_rmi4_data *ts)
{
	int ret=0;
	int nFd =1;
	int page_select=0;
	int pdt_addr_base=0;
	int intr_count=0;
	__u8 query[11];
	struct rmi4_function_descriptor fd;
	int data_len=0;

	ts->data_base= 0xff;
	ts->data_len = 0;

	do {


		pdt_addr_base=PDT_ADDR_BASE;
		ret = i2c_smbus_read_i2c_block_data(ts->client,
			pdt_addr_base-PDT_BYTE_COUNT*nFd+1, sizeof(struct rmi4_function_descriptor),(uint8_t *)&fd);
		pr_info("function_number=0x%x, query_base=0x%x, cmd_base=0x%x, "\
			"ctrl_base=0x%x, data_base=0x%x, intr_src_count=0x%x\n",
			fd.function_number,fd.query_base,fd.cmd_base,
			fd.ctrl_base,fd.data_base,fd.intr_src_count);
		if (ret < 0){
			pr_err("%s: failed to get ts intr state\n",__func__);
			return ret;
		}else{
			/*
			 * -----  RMI functions  -----
			 * Function		Purpose
			 *	$01			RMI Device Control
			 *  $05			Analog
			 *	$08			BIST
			 *	$09			BIST
			 *	$11			2-D TouchPad sensors
			 *	$19			0-D capacitive button sensors
			 *	$30			GPIO/LEDs (includes mechanical buttons)
			 *	$32			Timer
			 *	$34			Flash Memory Management
			*/
			switch(fd.function_number){
			case 0x01:
				ts->f01.flag		= fd.function_number;
				ts->f01.ctrl_base	= fd.ctrl_base;
				ts->f01.data_offset	= fd.data_base;	//intr status
				ts->f01.intr_offset = intr_count/8;
				ts->f01.intr_mask 	= ((1<<(fd.intr_src_count &0x7))-1)<<(intr_count%8);
				//pr_info("synaptics_ts:intr_offset=0x%x, intr_mask=0x%x\n",
				//	ts->f01.intr_offset,ts->f01.intr_mask);
				ts->f01.data_len = data_len = 0 ;
				break;
			case 0x11:
				ts->f11.flag		= fd.function_number;
				ts->f11.ctrl_base 	= fd.ctrl_base;
				ts->f11.data_offset = fd.data_base;
				ts->f11.intr_offset = intr_count/8;
				ts->f11.intr_mask 	= ((1<<(fd.intr_src_count &0x7))-1)<<(intr_count%8);

				ts->max[0]=i2c_smbus_read_word_data(ts->client, fd.ctrl_base+6)-12;
				ts->max[1]=i2c_smbus_read_word_data(ts->client, fd.ctrl_base+8)-12;
				pr_info("synaptics_ts:max_x=%d, max_y=%d\n",ts->max[0],ts->max[1]);

				ret=i2c_smbus_read_word_data(ts->client, fd.query_base);
				ret = i2c_smbus_read_i2c_block_data(ts->client,
					fd.query_base,sizeof(query),query);
				if (ret < 0){
					pr_err("i2c_smbus_read_byte_data failed\n");
					return ret;
				}

				ts->f11.points_supported=(query[1]&0x7)+1;
				if (ts->f11.points_supported >5)
					ts->points_supported=10;
				else
					ts->points_supported=ts->f11.points_supported;
				pr_info("synaptics_ts:%d, points_supported=%d\n",
					ts->f11.points_supported,ts->points_supported);

				ts->f11.data_len = data_len =
					// DATA0 : finger status, four fingers per register 
					(ts->f11.points_supported/4+1)
					// DATA1.*~ DATA5.*: hasAbs =1, F11_2D_Query1.
					//absolute finger position data, 5 per finger 
					+ ( (query[1]&(0x1<<4)) ? (5*ts->f11.points_supported):0)
					// DATA6.*~ DATA7.*: hasRel = 1, F11_2D_Query1:
					+ ( (query[1]&(0x1<<3)) ? (2*ts->f11.points_supported):0)
					// DATA8: F11_2D_Query7 !=0
					+ ( query[7] ? 1 : 0)
					// DATA9: F11_2D_Query7 or 8 !=0
					+ ((query[7] || query[8]) ? 1 : 0)
					// DATA10: 	hasPinch=1 of hasFlick=1, F11_2D_Query7
					+ ( (query[7]&(0x1<<6))||(query[7]&(0x1<<4)) ? 1:0)
					// DATA11,12: hasRotate=1,F11_2D_Query8 or hasFlick=1 ,F11_2D_Query7
					+ ( (query[7]&(0x1<<4)|| query[8]&(0x1<<2)) ? 2:0)
					// DATA13.*:	hasTouchShapes =1 ,F11_2D_Query8
					+ ( (query[8]&(0x1<<3))? ((query[10]&0x1F)/8+1):0 )
					// DATA14,15:	HasScrollZones=1, F11_2D_Query8
					+ ( (query[8]&(0x1<<3)) ? 2 : 0)
					// DATA16,17:	IndividualScrollZones=1, F11_2D_Query8
					+ ( (query[8]&(0x1<<4)) ? 2 : 0)
					;
				//pr_info("synaptics_ts: data len=%d\n",ts->f11.data_len);
				break;
/*			case 0x05:
			case 0x08:
			case 0x09:
			case 0x19:
			case 0x30:
			case 0x34:
				break;*/
			case 0x00:
				page_select++;
				nFd=0;
				//pr_info("synaptics_ts: next page?\n");
			default:
				data_len=0;
				break;
			}
			
		}
		
		intr_count+=fd.intr_src_count;

		if ( fd.data_base && (ts->data_base > fd.data_base) )
			ts->data_base = fd.data_base;

		ts->data_len += data_len;

		nFd++;

	}while( page_select<1);

	ts->f01.data_len = 1+ (intr_count+7)/8;

	ts->data_len += ts->f01.data_len;
	pr_info("synaptics_ts: data_base=0x%x, data_len=%d\n",ts->data_base,ts->data_len);

	ts->f01.data_offset -= ts->data_base;
	ts->f11.data_offset -= ts->data_base;

	return ret;
}


static int synaptics_rmi4_set_panel_state(
	struct synaptics_rmi4_data *ts, 
	int state
	)
{
	int ret=0;
	uint8_t mode=0;

	if (!ts)
		return ret;

	switch (state){
	case TS_POWER_ON:
		/*
		 * ReportingMode = ¡®001¡¯: 
		 * Reduced reporting mode In this mode, the absolute data
		 * source interrupt is asserted whenever a finger arrives 
		 * or leaves. Fingers that are present but basically 
		 * stationary do not generate additional interrupts 
		 * unless their positions change significantly. 
		 * In specific, for fingers already touching the pad, 
		 * the interrupt is asserted whenever the change in finger 
		 * position exceeds either DeltaXPosThreshold or DeltaYPosThreshold.
		*/
		ret = i2c_smbus_write_byte_data(ts->client, ts->f11.ctrl_base+2, 0x9);
		if (ret<0) pr_err("i2c_smbus_write_byte_data failed\n");
		ret = i2c_smbus_write_byte_data(ts->client, ts->f11.ctrl_base+3, 0x9);	
		if (ret<0) pr_err("i2c_smbus_write_byte_data failed\n");
		mode = i2c_smbus_read_byte_data(ts->client, ts->f11.ctrl_base);
		mode &=~0x7;
		mode |=0x01;
		ret = i2c_smbus_write_byte_data(ts->client, ts->f11.ctrl_base, mode);
		if (ret<0) pr_err("i2c_smbus_write_byte_data failed\n");
	case TS_RESUME:	
		// set nomal mode
		mode = i2c_smbus_read_byte_data(ts->client, ts->f01.ctrl_base);
		mode &=~0x3;
		ret = i2c_smbus_write_byte_data(ts->client, ts->f01.ctrl_base, mode);
		if (ret<0) pr_err("i2c_smbus_write_byte_data failed\n");

		ret = i2c_smbus_write_byte_data(ts->client,ts->f01.ctrl_base+1, 0x7);
		break;
	case TS_SUSPEND:
		ret = i2c_smbus_write_byte_data(ts->client, ts->f01.ctrl_base+1, 0);
		if (ret <0) pr_err("i2c_smbus_write_byte_data failed\n");

		mode = i2c_smbus_read_byte_data(ts->client, ts->f01.ctrl_base);
		mode &=~0x3;
		mode |=0x01;
		ret = i2c_smbus_write_byte_data(ts->client, ts->f01.ctrl_base, mode);
		if (ret<0) pr_err("i2c_smbus_write_byte_data failed\n");
		break;
	default:
		break;
	}
	return ret;
}

static void synaptics_rmi4_work_func(struct work_struct *work)
{
	int ret=0;
	struct synaptics_rmi4_data *ts = container_of(work, struct synaptics_rmi4_data, work);
	__u16 interrupt	= 0;
	int buf_len		= ts->data_len;
	__u8 buf[buf_len];
	struct f11_2d_point_data *point_data=NULL;
	int i,x,y,w,z,finger;
	static uint finger_pre[10];

	ret = i2c_smbus_read_i2c_block_data(ts->client,ts->data_base, buf_len, buf);

	interrupt=buf[1];

	if ( ts->f11.flag && (interrupt & ts->f11.intr_mask) ) 
	{

		for (i=0; i<ts->points_supported; i++)
		{
			finger = 0x3 &(buf[ts->f11.data_offset+i/4] >> ((i%4)*2));

			if (1 == finger)finger_pre[i] =2;
			if ((0 == finger)&&(0!=finger_pre[i]))finger_pre[i] =1;
			point_data = (struct f11_2d_point_data *)
				( buf 
				+ ts->f11.data_offset 
				+ (1 + ts->points_supported/4)
				+ i * sizeof(struct f11_2d_point_data)
				);

			x =((__u16)((point_data->xh<<0x4)|(point_data->xyl &0xF)))-6;
			y =((__u16)((point_data->yh<<0x4)|(point_data->xyl &0xF0)>>0x4))-6;
			w = ( point_data->wxy & 0x0F ) + ( point_data->wxy >>4 & 0x0F );
			z = point_data->z;
			if (x < 0) x =0;
			else if (x >ts->max[0])x = ts->max[0];
			if (y < 0) y =0;
			else if (y >ts->max[1])y = ts->max[1];

			switch(finger_pre[i])
			{
				case 1:
				{
					finger_pre[i]=0;
				};
				case 2:
				{
					input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID, i);
			 		input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR, z);
					input_report_abs(ts->input_dev, ABS_MT_WIDTH_MAJOR, w);//default 10
					input_report_abs(ts->input_dev, ABS_MT_POSITION_X, x);
					input_report_abs(ts->input_dev, ABS_MT_POSITION_Y, y);
					input_mt_sync(ts->input_dev);
				}break;
				default:break;
			}
		}
		input_sync(ts->input_dev);
	}
	if (ts->use_irq)
		enable_irq(ts->client->irq);
}


static enum hrtimer_restart synaptics_rmi4_timer_func(struct hrtimer *timer)
{
	struct synaptics_rmi4_data *ts = container_of(timer,struct synaptics_rmi4_data, timer);
	queue_work(synaptics_wq, &ts->work);
	hrtimer_start(&ts->timer, ktime_set(0, 12 * NSEC_PER_MSEC),HRTIMER_MODE_REL);
	return HRTIMER_NORESTART;
}

irqreturn_t synaptics_rmi4_irq_handler(int irq, void *dev_id)
{
	struct synaptics_rmi4_data *ts = dev_id;
	disable_irq_nosync(ts->client->irq);
	queue_work(synaptics_wq, &ts->work);
	return IRQ_HANDLED;
}

static int synaptics_rmi4_probe(
	struct i2c_client *client, 
	const struct i2c_device_id *id)
{
	struct synaptics_rmi4_data *ts;
	struct synaptics_rmi4_data *pdata;
	int ret = 0;
	uint max_x,max_y;
	struct proc_dir_entry *dir, *refresh;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("synaptics_ts_probe: need I2C_FUNC_I2C\n");
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}

	ts = kzalloc(sizeof(*ts), GFP_KERNEL);
	if (ts == NULL) {
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}
	INIT_WORK(&ts->work, synaptics_rmi4_work_func);
	ts->client = client;
#if defined(CONFIG_TOUCHSCREEN_SYNAPTICS_I2C_RMI_UPDATE)
				update_client=client;
#endif	

	i2c_set_clientdata(client, ts);


	pdata = client->dev.platform_data;
	if (pdata){
		ts->power 	= pdata->power;
		ts->orientation	= pdata->orientation;
		ts->gpio_irq = pdata->gpio_irq;
	}else{
		pr_err("%s error!\n",__func__);
		goto err_detect_failed; 
	}
	if (ts->power) {
		ret = ts->power(1);
		if (ret < 0) {
			pr_err("%s power on failed\n",__func__);
			goto err_power_failed;
		}
		msleep(250);
	}
	ret = gpio_request(ts->gpio_irq, "gpio irq");
	if (ret) {
		pr_err("%s: unable to request gpio %d (%d)\n",
			__func__, ts->gpio_irq,ret);
			goto err_power_failed;
	}
#if defined (CONFIG_TOUCHSCREEN_SYNAPTICS_I2C_RMI_UPDATE)
	ts_temp=ts;
#endif
	ret = synaptics_rmi4_get_pdt(ts);
	if (ret <= 0) {
		pr_err("Error identifying device (%d)\n", ret);
		ret = -ENODEV;
		goto err_detect_failed;
	}

	max_x=ts->max[0];
#if defined(CONFIG_MACH_R750)
	max_y= 2739-6;
#elif defined(CONFIG_MACH_V9PLUS)
	max_y=3092-6;
#elif defined(CONFIG_MACH_SKATE)
	max_y=1869-6;
#else
	max_y=ts->max[1];
#endif

	ret = synaptics_rmi4_set_panel_state(ts, TS_POWER_ON); 
	if (ret < 0) {
		pr_err("synaptics_rmi4_set_panel_state failed\n");
		goto err_detect_failed;
	}

	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) {
		ret = -ENOMEM;
		pr_err("%s: Failed to allocate input device\n",__func__);
		goto err_input_dev_alloc_failed;
	}
	
	ts->input_dev->name = SYNAPTICS_I2C_RMI4_NAME;
	ts->input_dev->phys = client->name;

#ifdef TOUCHSCREEN_DUPLICATED_FILTER
	ts->dup_threshold=(max_y*10/LCD_MAX_Y+5)/10;	
	pr_info("dup_threshold %d\n", ts->dup_threshold);
#endif

	set_bit(EV_SYN, ts->input_dev->evbit);
	set_bit(EV_KEY, ts->input_dev->evbit);
	set_bit(BTN_TOUCH, ts->input_dev->keybit);
	set_bit(EV_ABS, ts->input_dev->evbit);
	set_bit(KEY_HOME, ts->input_dev->keybit);
	set_bit(KEY_MENU, ts->input_dev->keybit);
	set_bit(KEY_BACK, ts->input_dev->keybit);
	set_bit(ABS_SINGLE_TAP, ts->input_dev->absbit);
	set_bit(ABS_TAP_HOLD, ts->input_dev->absbit);
	set_bit(ABS_EARLY_TAP, ts->input_dev->absbit);
	set_bit(ABS_FLICK, ts->input_dev->absbit);
	set_bit(ABS_PRESS, ts->input_dev->absbit);
	set_bit(ABS_DOUBLE_TAP, ts->input_dev->absbit);
	set_bit(ABS_PINCH, ts->input_dev->absbit);
	set_bit(ABS_MT_TRACKING_ID, ts->input_dev->absbit);
	set_bit(ABS_MT_TOUCH_MAJOR, ts->input_dev->absbit);
	set_bit(ABS_MT_POSITION_X, ts->input_dev->absbit);
	set_bit(ABS_MT_POSITION_Y, ts->input_dev->absbit);
	set_bit(ABS_MT_WIDTH_MAJOR, ts->input_dev->absbit);
	set_bit(ABS_MT_ORIENTATION, ts->input_dev->absbit);

	input_set_abs_params(ts->input_dev, ABS_MT_ORIENTATION, 0, ts->orientation, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR, 0, 0xFF, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X, 0, max_x+1, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y, 0, max_y+1, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_WIDTH_MAJOR, 0, 30, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID, 0, 10, 0, 0);	
	input_set_abs_params(ts->input_dev, ABS_SINGLE_TAP, 0, 5, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_TAP_HOLD, 0, 5, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_EARLY_TAP, 0, 5, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_FLICK, 0, 5, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_PRESS, 0, 5, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_DOUBLE_TAP, 0, 5, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_PINCH, -0xFF, 0xFF, 0, 0);

	ret = input_register_device(ts->input_dev);
	if (ret)	{
		pr_err("%s: Unable to register %s input device\n", 
			__func__, ts->input_dev->name);
		goto err_input_register_device_failed;
	}

	if (client->irq) 
	{
        if (request_irq(client->irq, synaptics_rmi4_irq_handler, 
			IRQF_TRIGGER_FALLING, client->name, ts)==0)
        {
			pr_info("Received IRQ!\n");
			ts->use_irq = 1;
		}else{
			dev_err(&client->dev, "request_irq failed\n");
		}
	}
	
	if (!ts->use_irq){
		hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
		ts->timer.function = synaptics_rmi4_timer_func;
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
	}

#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = synaptics_rmi4_early_suspend;
	ts->early_suspend.resume = synaptics_rmi4_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif

  dir = proc_mkdir("touchscreen", NULL);
	refresh = create_proc_entry("ts_information", 0777, dir);
	if (refresh) {
		refresh->data		= NULL;
		refresh->read_proc  = proc_read_val;
		refresh->write_proc = proc_write_val;
	}
#if defined(CONFIG_TOUCHSCREEN_VIRTUAL_KEYS)
	ts_key_report_init();
#endif

	pr_info("%s: Start ts %s in %s mode\n",
		__func__,ts->input_dev->name, ts->use_irq ? "interrupt" : "polling");
	return 0;

err_input_register_device_failed:
	input_free_device(ts->input_dev);
err_input_dev_alloc_failed:
err_detect_failed:
err_power_failed:
	gpio_free(ts->gpio_irq);
	kfree(ts);
err_alloc_data_failed:
err_check_functionality_failed:
	return ret;

}

static int synaptics_rmi4_remove(struct i2c_client *client)
{
	int ret=0;
	struct synaptics_rmi4_data *ts = i2c_get_clientdata(client);

#if defined(CONFIG_TOUCHSCREEN_VIRTUAL_KEYS)
	ts_key_report_deinit();
#endif

	// power off
	if (ts->power) {
		ret = ts->power(0);
		if (ret < 0)
			pr_err("%s power off failed\n",__func__);
	}

	unregister_early_suspend(&ts->early_suspend);
	if (ts->use_irq)
		free_irq(client->irq, ts);
	else
		hrtimer_cancel(&ts->timer);
	input_unregister_device(ts->input_dev);

	kfree(ts);

	return 0;
}

static int synaptics_rmi4_suspend(struct i2c_client *client, pm_message_t mesg)
{
	int ret;

	struct synaptics_rmi4_data *ts = i2c_get_clientdata(client);

	if (ts->use_irq){
		disable_irq(client->irq);
	}
	else
		hrtimer_cancel(&ts->timer);

	ret = cancel_work_sync(&ts->work);
	if (ret && ts->use_irq)
		enable_irq(client->irq);

	synaptics_rmi4_set_panel_state(ts, TS_SUSPEND);

	return 0;
}

static int synaptics_rmi4_resume(struct i2c_client *client)
{
//	int ret;
	struct synaptics_rmi4_data *ts = i2c_get_clientdata(client);

	synaptics_rmi4_set_panel_state(ts, TS_RESUME);
	
	if (ts->use_irq){
		enable_irq(client->irq);
	}else{
		hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);
	}
	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void synaptics_rmi4_early_suspend(struct early_suspend *h)
{
	struct synaptics_rmi4_data *ts;
	ts = container_of(h, struct synaptics_rmi4_data, early_suspend);
	synaptics_rmi4_suspend(ts->client, PMSG_SUSPEND);
}

static void synaptics_rmi4_late_resume(struct early_suspend *h)
{
	struct synaptics_rmi4_data *ts;
	ts = container_of(h, struct synaptics_rmi4_data, early_suspend);
	synaptics_rmi4_resume(ts->client);

}
#endif

static const struct i2c_device_id synaptics_rmi4_id[] = {
	{ SYNAPTICS_I2C_RMI4_NAME, 0 },
	{ }
};
static struct i2c_driver synaptics_rmi4_driver = {
	.probe		= synaptics_rmi4_probe,
	.remove		= synaptics_rmi4_remove,
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= synaptics_rmi4_suspend,
	.resume		= synaptics_rmi4_resume,
#endif
	.id_table	= synaptics_rmi4_id,
	.driver = {
		.name	= SYNAPTICS_I2C_RMI4_NAME,
	},
};

static int __devinit synaptics_rmi4_init(void)
{
	synaptics_wq = create_singlethread_workqueue("synaptics_wq");
	if (!synaptics_wq){
		pr_err("Could not create work queue synaptics_wq: no memory");
		return -ENOMEM;
	}
	return i2c_add_driver(&synaptics_rmi4_driver);
}

static void __exit synaptics_rmi4_exit(void)
{
	i2c_del_driver(&synaptics_rmi4_driver);
	if (synaptics_wq)
		destroy_workqueue(synaptics_wq);

}

module_init(synaptics_rmi4_init);
module_exit(synaptics_rmi4_exit);

MODULE_DESCRIPTION("Synaptics RMI4 Driver");
MODULE_LICENSE("GPL");

