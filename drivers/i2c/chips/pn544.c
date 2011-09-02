/*
 * Copyright (c) 2010 Trusted Logic S.A.
 * All Rights Reserved.
 *
 * This software is the confidential and proprietary information of
 * Trusted Logic S.A. ("Confidential Information"). You shall not
 * disclose such Confidential Information and shall use it only in
 * accordance with the terms of the license agreement you entered
 * into with Trusted Logic S.A.
 *
 * TRUSTED LOGIC S.A. MAKES NO REPRESENTATIONS OR WARRANTIES ABOUT THE
 * SUITABILITY OF THE SOFTWARE, EITHER EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, OR NON-INFRINGEMENT. TRUSTED LOGIC S.A. SHALL
 * NOT BE LIABLE FOR ANY DAMAGES SUFFERED BY LICENSEE AS A RESULT OF USING,
 * MODIFYING OR DISTRIBUTING THIS SOFTWARE OR ITS DERIVATIVES.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/smp_lock.h>
#include <linux/jiffies.h>
#include <asm/uaccess.h>
#include <linux/gpio.h>

#include <linux/delay.h>
#include <linux/hrtimer.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <asm-generic/gpio.h>

#define ZTE_NFC_DEBUG
#define ZTE_IRQ_LOW 

#define MAX_BUFFER_SIZE 256

#define PN544_MAJOR     0  /* Defaults to dynamic major number */
#define PN544_MINORS    1  /* Register for only one minor */
//#define DEVNAME         "nfc"
#define DEVNAME         "pn544"
#define CLASSNAME       "nfc-dev"

struct pn544_dev {
   wait_queue_head_t read_queue; /* Used to save blocked callers */
   struct semaphore  sem;        /* Used to synchronize IRQ value reading */
   struct i2c_client *client;    /* The structure for use with i2c functions */
   struct device     *dev;       /* The char device structure */
   int               use_irq;    /* Flag set to 1 if using IRQ, 0 if polling */
   struct hrtimer    timer;      /* Timer used for polling */
};

static struct pn544_dev    *pn544_dev = NULL;

/*
 * Our parameters which can be set at load time.
 */

int pn544_major = PN544_MAJOR;
int pn544_minor = 0;
int pn544_disable_irq = false;
int pn544_poll_value = 10000000;

module_param(pn544_major, int, S_IRUGO);
module_param(pn544_minor, int, S_IRUGO);
module_param(pn544_disable_irq, int, S_IRUGO);
module_param(pn544_poll_value, int, S_IRUGO);

MODULE_AUTHOR("Trusted Logic S.A. (Sylvain Fonteneau)");
MODULE_DESCRIPTION("PN544 /dev/nfc entry driver");
MODULE_LICENSE("GPL");

/* ------------------------------------------------------------------------- */

static irqreturn_t pn544_dev_irq_handler(int irq, void *dev_id)
{
   struct pn544_dev *pn544 = dev_id;

#ifdef ZTE_NFC_DEBUG
   printk("pn544-dev: interrupt  occur!!!!.\n");
#endif

   /* Wake up readers */
   //把当前进程从等待队列拉回执行队列
   wake_up_interruptible(&pn544->read_queue);

   return IRQ_HANDLED;
}

static enum hrtimer_restart pn544_dev_timer_handler(struct hrtimer *timer)
{
   struct pn544_dev *pn544 = container_of(timer, struct pn544_dev, timer);

   /* Wake up readers */
   //把当前进程从等待队列拉回执行队列
   wake_up_interruptible(&pn544->read_queue);

   /* Restart timer */
   hrtimer_start(&pn544->timer, ktime_set(0, pn544_poll_value), HRTIMER_MODE_REL);
   return HRTIMER_NORESTART;
}

/* Utility to display device name in sysfs */
static ssize_t show_adapter_name(struct device *dev,
             struct device_attribute *attr, char *buf)
{
   struct pn544_dev *pn544 = pn544_dev;

   if (!pn544) {
      return -ENODEV;
   }

   return sprintf(buf, "%s\n", DEVNAME);
}
static DEVICE_ATTR(name, S_IRUGO, show_adapter_name, NULL);

#ifdef ZTE_NFC_DEBUG
static void printk_buffer(char * buffer, int count)
{
   int i;

   printk("[ ");
   for(i=0;i<count;i++) {
      printk("%02X ", buffer[i]);
   }
   printk("]");
}
#else
/* Utility to display buffer in logs */
static void pr_debug_buffer(char * buffer, int count)
{
   int i;

   pr_debug("[ ");
   for(i=0;i<count;i++) {
      pr_debug("%02X ", buffer[i]);
   }
   pr_debug("]");
}
#endif

/* ------------------------------------------------------------------------- */
static ssize_t pn544_dev_read (struct file *filp, char __user *buf, size_t count,
                              loff_t *offset)
{
   struct pn544_dev  *pn544 = (struct pn544_dev *)filp->private_data;
   struct i2c_client *client = (struct i2c_client *)pn544->client;
   char *tmp;
   int ret;
 //  int irq_gpio = irq_to_gpio(client->irq);
  int irq_gpio = INT_TO_MSM_GPIO(client->irq);

   if (count > MAX_BUFFER_SIZE) {
      count = MAX_BUFFER_SIZE;
   }

   tmp = kmalloc(count,GFP_KERNEL);
   if (tmp == NULL) {
      return -ENOMEM;
   }

#ifdef ZTE_NFC_DEBUG
   printk("pn544-dev: reading %zu bytes.\n", count);
#else
   pr_debug("pn544-dev: reading %zu bytes.\n", count);
#endif

   /* Lock semaphore */
   if (down_interruptible(&pn544->sem)) {
      return -ERESTARTSYS;
   }
   /* Wait for IRQ if not already pending */
 #ifdef ZTE_IRQ_LOW   
   while (gpio_get_value(irq_gpio)) {
 #else
   while (!gpio_get_value(irq_gpio)) {
 #endif
      /* Not ready to read data, release semaphore */
      up(&pn544->sem);
      /* Handle non-blocking calls */
      if (filp->f_flags & O_NONBLOCK) {
         return -EAGAIN;
      }

#ifdef ZTE_NFC_DEBUG
     printk("pn544-dev: wait for incoming data.\n");
#else
      pr_debug("pn544-dev: wait for incoming data.\n");
#endif

     /* Sleep until the IRQ comes */
    /*这个函数睡眠之后不会被调度，需要在中断或定时器中wake_up_interruptible()	  
        之后才会被调度去判决gpio_get_value(irq_gpio)!=0  */
 #ifdef ZTE_IRQ_LOW
   if (wait_event_interruptible(pn544->read_queue, (gpio_get_value(irq_gpio)==0))) {
#else
  if (wait_event_interruptible(pn544->read_queue, (gpio_get_value(irq_gpio)!=0))) {
#endif
		 return -ERESTARTSYS; /* signal: tell the fs layer to handle it */
      }
      /* Loop, but first reacquire the lock (to avoid multiple read concurrency) */
      if (down_interruptible(&pn544->sem)) {
         return -ERESTARTSYS;
      }
   }

   /* Read data */
   ret = i2c_master_recv(client, tmp, count);
   if (ret >= 0) {
      ret = copy_to_user(buf, tmp, count)?-EFAULT:ret;

#ifdef ZTE_NFC_DEBUG 
      printk("pn544_dev_read: received ");
      printk_buffer(tmp, count);
      printk("\n");
#else	
      pr_debug("pn544_dev_read: received ");
      pr_debug_buffer(tmp, count);
      pr_debug("\n");
#endif	  
   }
   else if (ret < 0) {
      printk(KERN_WARNING "pn544_dev: failed to read from i2c (error code  %d)\n", ret);
   }

   /* Release semaphore */
   up (&pn544->sem);
   kfree(tmp);

   return ret;
}

static ssize_t pn544_dev_write (struct file *filp, const char __user *buf, size_t count,
                             loff_t *offset)
{
   int ret;
   char *tmp;

   struct pn544_dev  *pn544 = (struct pn544_dev *)filp->private_data;
   struct i2c_client *client = (struct i2c_client *)pn544->client;

   if (count > MAX_BUFFER_SIZE) {
      count = MAX_BUFFER_SIZE;
   }

   tmp = kmalloc(count,GFP_KERNEL);
   if (tmp == NULL)  {
      printk(KERN_WARNING "pn544_dev_write: failed to allocate buffer\n");
      return -ENOMEM;
   }
   if (copy_from_user(tmp, buf, count)) {
      printk(KERN_WARNING "pn544_dev_write: failed to copy from user space\n");
      kfree(tmp);
      return -EFAULT;
   }

#ifdef ZTE_NFC_DEBUG
   printk("pn544-dev: writing %zu bytes.\n", count);
#else
   pr_debug("pn544-dev: writing %zu bytes.\n", count);
#endif

   /* Write data (already semaphore-protected) */
   ret = i2c_master_send(client, tmp, count);
   if (ret < 0) {
      printk(KERN_WARNING "pn544_dev_write: i2c_master_send() returned %d\n", ret);
   }
   else
   {
#ifdef ZTE_NFC_DEBUG 
      printk("pn544_dev_write: sent ");
      printk_buffer(tmp, count);
      printk("\n");
#else
      pr_debug("pn544_dev_write: sent ");
      pr_debug_buffer(tmp, count);
      pr_debug("\n");
#endif	  
   }
   kfree(tmp);
   return ret;
}

/*
首先,系统调用open打开一个字符设备的时候, 通过一系列调用,
最终会执行到 chrdev_open.  

int chrdev_open(struct inode * inode, struct file * filp) 

chrdev_open()所做的事情可以概括如下: 
1. 根据设备号(inode->i_rdev), 在字符设备驱动模型(proc/devices)中查找对应
   的驱动程序,  这通过kobj_lookup() 来实现, kobj_lookup()会返回对应驱动程序cdev的kobject. 
2. 设置inode->i_cdev , 指向找到的cdev. 
3. 将inode添加到cdev->list的链表中. 
4. 使用cdev的ops 设置file对象的f_op 
5. 如果ops中定义了open方法,则调用该open方法 
6. 返回. 

执行完chrdev_open()之后,file对象的f_op指向cdev的ops,因而之后对设备进行的
read, write等操作,就会执行cdev的相应操作.
*/
static int pn544_dev_open(struct inode *inode, struct file *filp)
{
   unsigned int minor = iminor(inode);
   int ret = 0;

#ifdef ZTE_NFC_DEBUG
   printk("pn544_dev_open: %d,%d\n", imajor(inode), iminor(inode));
#else
   pr_debug("pn544_dev_open: %d,%d\n", imajor(inode), iminor(inode));
#endif

   lock_kernel();

   if (minor != pn544_minor) {
      ret = -ENODEV;
      goto out;
   }

   filp->private_data = pn544_dev;

out:
   unlock_kernel();
   return ret;
}

static int pn544_dev_release(struct inode *inode, struct file *file)
{
   return 0;
}

static const struct file_operations pn544_dev_fops = {
   .owner            = THIS_MODULE,
   .llseek           = no_llseek,
   .read             = pn544_dev_read,
   .write            = pn544_dev_write,
   .open             = pn544_dev_open,
   .release          = pn544_dev_release,
/*   .poll             = pn544_dev_poll, */
};

/* ------------------------------------------------------------------------- */

static struct class *pn544_dev_class;

static int pn544_probe(
   struct i2c_client *client, const struct i2c_device_id *id)
{
   int               ret;

   if (pn544_dev != NULL) {
      printk(KERN_ERR "pn544_probe: multiple devices NOT supported\n");
      ret = -ENODEV;
      goto err_single_device;
   }

   if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
      printk(KERN_ERR "pn544_probe: need I2C_FUNC_I2C\n");
      ret = -ENODEV;
      goto err_check_functionality_failed;
   }

   pn544_dev = kzalloc(sizeof(*pn544_dev), GFP_KERNEL);
   if (pn544_dev == NULL) {
      printk(KERN_ERR "pn544_probe: out of memory\n");
      ret = -ENOMEM;
      goto err_alloc_data_failed;
   }
   pn544_dev->client = client;

   /* init semaphore and queues */
   sema_init(&pn544_dev->sem, 1);
   init_waitqueue_head(&pn544_dev->read_queue);

   /* register this device with the driver core */
   /*3. slf note 20110328--->create:  sys/class/nfc-dev/pn544 , 注册设备节点/dev/pn544*/
   pn544_dev->dev = device_create(pn544_dev_class, &client->dev,
                 MKDEV(pn544_major, pn544_minor), NULL, DEVNAME);
   if (IS_ERR(pn544_dev->dev)) {
      printk(KERN_ERR "pn544_probe: device_create() failed\n");
      ret = PTR_ERR(pn544_dev->dev);
      goto err_device_create_failed;
   }

   /*4. slf note 20110328--->create:  sys/class/nfc-dev/pn544/name */
   ret = device_create_file(pn544_dev->dev, &dev_attr_name);
   if (ret) {
      goto err_device_create_file_failed;
   }

   /* set irq/polling mode */
   if (client->irq && !pn544_disable_irq) {
#ifdef ZTE_IRQ_LOW 	
     ret = request_irq(client->irq, pn544_dev_irq_handler, IRQF_TRIGGER_FALLING, client->name, pn544_dev);
#else
    ret = request_irq(client->irq, pn544_dev_irq_handler, IRQF_TRIGGER_RISING, client->name, pn544_dev);
#endif
	  if (ret == 0) {
         pn544_dev->use_irq = 1;
      }
      else {
         dev_err(&client->dev, "request_irq failed\n");
      }
   }
   if (!pn544_dev->use_irq) {
      hrtimer_init(&pn544_dev->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
      pn544_dev->timer.function = pn544_dev_timer_handler;
      hrtimer_start(&pn544_dev->timer, ktime_set(0, pn544_poll_value), HRTIMER_MODE_REL);
   }

   printk(KERN_INFO "pn544_probe: Start in %s mode,IRQ=%d,GPIO=%d\n", pn544_dev->use_irq ? "interrupt" : "polling",client->irq,INT_TO_MSM_GPIO(client->irq));

   return 0;

err_device_create_file_failed:
   device_destroy(pn544_dev_class, MKDEV(pn544_major, pn544_minor));
err_device_create_failed:
   kfree(pn544_dev);
   pn544_dev = NULL;
err_alloc_data_failed:
err_check_functionality_failed:
err_single_device:
   return ret;
}

static int pn544_remove(struct i2c_client *client)
{
   printk(KERN_INFO "pn544_remove()\n");

   if (pn544_dev->use_irq) {
      free_irq(client->irq, pn544_dev);
   }
   else {
      hrtimer_cancel(&pn544_dev->timer);
   }
   device_destroy(pn544_dev_class, MKDEV(pn544_major, pn544_minor));
   kfree(pn544_dev);
   pn544_dev = NULL;
   return 0;
}


static const struct i2c_device_id pn544_id[] = {
   { "pn544", 0 },
   { }
};


static struct i2c_driver pn544_driver = {
   .id_table   = pn544_id,
   .probe      = pn544_probe,
   .remove      = pn544_remove,
   .driver = {
      .name   = "pn544",
   },
};

/* ------------------------------------------------------------------------- */

/*
 * module load/unload record keeping
 */
extern int nxp_pn544_reset(void);
static int __init pn544_dev_init(void)
{
   int ret;

   printk(KERN_INFO "pn544 /dev/pn544 entry driver\n");

   ret =nxp_pn544_reset();
   if (ret < 0) {
      printk(KERN_ERR "pn544: can't reset device\n");
      return ret;
   }
   
   /*1. slf note 20110328--->申请一个设备号,并且将其注册到字符
   设备驱动模型中.:  proc/devices/pn544 。
   函数 register_chrdev   的第一个参数如果为 0，   表示由内核来确
   定该注册伪字符设备   的主设备号，这是该函数的返回为
   实际分配的主   设备号，如果 返回小于 0，表示注册失败。 */
   ret = register_chrdev(pn544_major, DEVNAME, &pn544_dev_fops);
   if (ret < 0) {
      printk(KERN_WARNING "pn544: can't register device\n");
      goto err_register_chrdev_failed;
   }
   else if (ret > 0) {
      pn544_major = ret;
   }

  /*2. slf note 20110328--->create:  sys/class/nfc-dev */
   pn544_dev_class = class_create(THIS_MODULE, CLASSNAME);
   if (IS_ERR(pn544_dev_class)) {
      ret = PTR_ERR(pn544_dev_class);
      printk(KERN_WARNING "pn544: can't create class\n");
      goto err_class_create_failed;
   }

   ret = i2c_add_driver(&pn544_driver);
   if (ret) {
      printk(KERN_WARNING "pn544: can't register i2c driver\n");
      goto err_i2c_add_driver_failed;
   }

   printk(KERN_INFO "pn544 registered with major number %d\n", pn544_major);

   return 0;

err_i2c_add_driver_failed:
   class_destroy(pn544_dev_class);
err_class_create_failed:
   unregister_chrdev(pn544_major, DEVNAME);
err_register_chrdev_failed:
   return ret;
}

static void __exit pn544_dev_exit(void)
{
   printk(KERN_INFO "Unloading pn544 /dev/nfc entry driver\n");
   i2c_del_driver(&pn544_driver);
   class_destroy(pn544_dev_class);
   unregister_chrdev(PN544_MAJOR, DEVNAME);
}

module_init(pn544_dev_init);
module_exit(pn544_dev_exit);
