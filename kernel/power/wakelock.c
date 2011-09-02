/* kernel/power/wakelock.c
 *
 * Copyright (C) 2005-2008 Google, Inc.
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
  * 20110112 lhx     LHX_PM_20110112_02 disalbe sync_in_suspend temp for wakeup system
  * 20100519 liuyijian     support auto test ZTE_WAKELOCK_LYJ_001 
  * 2010-05-06  hyj         protect  active_wake_lock_buf against  memory overflow   ZTE_HYJ_PM_20100506
  * 2010-04-19  hyj        modify suspend_exception_timer expire time from 15Hz to 5*60Hz ZTE_HYJ_PM_20100419_01
  * 2010-04-06  zhengchao  add some logs for wakelock blocking suspend ZTE_ZHENGCHAO_PM_20100406_01
*  2010-01-18   hyj        debug wakelock info when suspend fail  ZTE_HYJ_WAKELOCK_TOOL
*/ 
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/rtc.h>
#include <linux/suspend.h>
#include <linux/syscalls.h> /* sys_sync */
#include <linux/wakelock.h>
#ifdef CONFIG_WAKELOCK_STAT
#include <linux/proc_fs.h>
#endif
#include "power.h"
//20100519 ZTE_WAKELOCK_LYJ_001 
#include <linux/moduleparam.h>

enum {
	DEBUG_EXIT_SUSPEND = 1U << 0,
	DEBUG_WAKEUP = 1U << 1,
	DEBUG_SUSPEND = 1U << 2,
	DEBUG_EXPIRE = 1U << 3,
	DEBUG_WAKE_LOCK = 1U << 4,
};
//static int debug_mask = DEBUG_EXIT_SUSPEND | DEBUG_WAKEUP;
static int debug_mask = DEBUG_EXIT_SUSPEND | DEBUG_WAKEUP | DEBUG_SUSPEND;
module_param_named(debug_mask, debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);

#define WAKE_LOCK_TYPE_MASK              (0x0f)
#define WAKE_LOCK_INITIALIZED            (1U << 8)
#define WAKE_LOCK_ACTIVE                 (1U << 9)
#define WAKE_LOCK_AUTO_EXPIRE            (1U << 10)
#define WAKE_LOCK_PREVENTING_SUSPEND     (1U << 11)

static DEFINE_SPINLOCK(list_lock);
static LIST_HEAD(inactive_locks);
static struct list_head active_wake_locks[WAKE_LOCK_TYPE_COUNT];
static int current_event_num;
struct workqueue_struct *suspend_work_queue;
struct wake_lock main_wake_lock;
suspend_state_t requested_suspend_state = PM_SUSPEND_MEM;
static struct wake_lock unknown_wakeup;

//ruanmeisi move do_sync from earlysuspend to thread
struct suspend_sync_work
{
	struct workqueue_struct *workqueue;
	struct wake_lock sync_wake_lock;
	struct work_struct sync_work;
	wait_queue_head_t wait_wq;
	int enable;
	int inprocess;//debug flag
};

static struct suspend_sync_work sync_in_suspend;


#include <linux/delay.h>


void enqueue_sync_work(signed long timeout)
{
	
	long ret = 0;
	DEFINE_WAIT(__wait);

	if (NULL == sync_in_suspend.workqueue ||
	    !sync_in_suspend.enable) {
		sys_sync();
		ret = 0;
		return ;
	}
	prepare_to_wait(&sync_in_suspend.wait_wq, &__wait, TASK_UNINTERRUPTIBLE);
	if(queue_work(sync_in_suspend.workqueue, &sync_in_suspend.sync_work)) {
		ret = schedule_timeout(timeout);
		if (ret == 0) {
			printk(KERN_ERR"rms: %s %d: wait sync timeout\n",
			       __FUNCTION__, __LINE__);
		}
	} else {
		printk(KERN_ERR"rms:%s %d: suspend_sync already in queue\n",
		       __FUNCTION__, __LINE__);
	}
	finish_wait(&sync_in_suspend.wait_wq, &__wait);
	return ;
}
void abort_sync_wait(void)
{
	if (NULL == sync_in_suspend.workqueue) {
		return ;
	}
	if (sync_in_suspend.inprocess) {
		printk(KERN_ERR"rms:%s %d:sync in processing", __FUNCTION__, __LINE__);
	}
	wake_up_all(&sync_in_suspend.wait_wq);
	return ;
}
int resume_work_pending(void);
static void do_sync_work(struct work_struct *work)
{
	struct suspend_sync_work *p =
		container_of(work, struct suspend_sync_work, sync_work);
	
	p->inprocess = 1;
	wake_lock(&p->sync_wake_lock);
	if (resume_work_pending()) {
		//wake up earlysuspend when last_resume_work has queued
		printk(KERN_ERR"rms:wake up earlysuspnd %s %d\n",
		       __FUNCTION__, __LINE__);
		wake_up_all(&p->wait_wq);
	}
	sys_sync();
	wake_unlock(&p->sync_wake_lock);
	wake_up_all(&p->wait_wq);
	p->inprocess = 0;
	
}
static void create_suspend_sync_work(void)
{
	sync_in_suspend.workqueue = create_singlethread_workqueue("suspend_sync");
	INIT_WORK(&(sync_in_suspend.sync_work), do_sync_work);
	wake_lock_init(&(sync_in_suspend.sync_wake_lock),
		       WAKE_LOCK_SUSPEND, "suspend_sync");
	init_waitqueue_head(&sync_in_suspend.wait_wq);
	sync_in_suspend.enable = 1;
	sync_in_suspend.inprocess = 0;
	if (sync_in_suspend.enable) {
		printk(KERN_ERR"pm: %s %d: sync in workqueue enable\n",
		       __FUNCTION__, __LINE__);
	}
}
void destroy_suspend_sync_work(void)
{
	if (sync_in_suspend.workqueue) {
		destroy_workqueue(sync_in_suspend.workqueue);
		sync_in_suspend.workqueue = NULL;
	}
	wake_lock_destroy(&sync_in_suspend.sync_wake_lock);
	wake_up_all(&sync_in_suspend.wait_wq);
	sync_in_suspend.enable = 0;
	return ;
}


static ssize_t suspend_sync_show(struct device *devp, struct device_attribute *attr, char *buf)
{
        int i = 0;
        i = scnprintf(buf, PAGE_SIZE, "%s\n", sync_in_suspend.enable? "enable":"disable");

        return i;
}
static ssize_t suspend_sync_store(struct device *dev,
                                   struct device_attribute *attr,
                                   const char *buf, size_t size)
{
        int value;
        sscanf(buf, "%d", &value);
	sync_in_suspend.enable = !!value;
        return size;
}

static DEVICE_ATTR(suspend_sync, S_IRUGO, suspend_sync_show, suspend_sync_store);


#ifdef CONFIG_ZTE_SUSPEND_WAKEUP_MONITOR
/*ZTE_HYJ_WAKELOCK_TOOL 2010.0114 begin*/
#define WAKE_LOCK_RECORD_NR (10)
/*ZTE_HYJ_PM_20100506 begin*/
#define RECORD_SIZE 400 
char active_wake_lock_buf[WAKE_LOCK_RECORD_NR][RECORD_SIZE];

static ssize_t active_wake_lock_show(struct device *devp, struct device_attribute *attr, char *buf)
{
	char * echo = buf;
	int used_size = 0;
	int ret =-1;
	int i ;

	for (i = 0; i < WAKE_LOCK_RECORD_NR; i++) {
		ret = snprintf(echo + used_size, PAGE_SIZE - used_size, "%s\n",active_wake_lock_buf[i]);
		if( ret < 0 ){
			echo[used_size] = '\0';
			return ret;
		}
		used_size += ret;
	}
	
	return used_size;
}
/*ZTE_HYJ_PM_20100506 end*/
static DEVICE_ATTR(active_wakelock, S_IRUGO, active_wake_lock_show, NULL);
/*ZTE_HYJ_WAKELOCK_TOOL 2010.0114 end*/
#endif
#ifdef CONFIG_WAKELOCK_STAT
static struct wake_lock deleted_wake_locks;
static ktime_t last_sleep_time_update;
static int wait_for_wakeup;

int get_expired_time(struct wake_lock *lock, ktime_t *expire_time)
{
	struct timespec ts;
	struct timespec kt;
	struct timespec tomono;
	struct timespec delta;
	unsigned long seq;
	long timeout;

	if (!(lock->flags & WAKE_LOCK_AUTO_EXPIRE))
		return 0;
	do {
		seq = read_seqbegin(&xtime_lock);
		timeout = lock->expires - jiffies;
		if (timeout > 0)
			return 0;
		kt = current_kernel_time();
		tomono = wall_to_monotonic;
	} while (read_seqretry(&xtime_lock, seq));
	jiffies_to_timespec(-timeout, &delta);
	set_normalized_timespec(&ts, kt.tv_sec + tomono.tv_sec - delta.tv_sec,
				kt.tv_nsec + tomono.tv_nsec - delta.tv_nsec);
	*expire_time = timespec_to_ktime(ts);
	return 1;
}


static int print_lock_stat(struct seq_file *m, struct wake_lock *lock)
{
	int lock_count = lock->stat.count;
	int expire_count = lock->stat.expire_count;
	ktime_t active_time = ktime_set(0, 0);
	ktime_t total_time = lock->stat.total_time;
	ktime_t max_time = lock->stat.max_time;

	ktime_t prevent_suspend_time = lock->stat.prevent_suspend_time;
	if (lock->flags & WAKE_LOCK_ACTIVE) {
		ktime_t now, add_time;
		int expired = get_expired_time(lock, &now);
		if (!expired)
			now = ktime_get();
		add_time = ktime_sub(now, lock->stat.last_time);
		lock_count++;
		if (!expired)
			active_time = add_time;
		else
			expire_count++;
		total_time = ktime_add(total_time, add_time);
		if (lock->flags & WAKE_LOCK_PREVENTING_SUSPEND)
			prevent_suspend_time = ktime_add(prevent_suspend_time,
					ktime_sub(now, last_sleep_time_update));
		if (add_time.tv64 > max_time.tv64)
			max_time = add_time;
	}

	return seq_printf(m,
		     "\"%s\"\t%d\t%d\t%d\t%lld\t%lld\t%lld\t%lld\t%lld\n",
		     lock->name, lock_count, expire_count,
		     lock->stat.wakeup_count, ktime_to_ns(active_time),
		     ktime_to_ns(total_time),
		     ktime_to_ns(prevent_suspend_time), ktime_to_ns(max_time),
		     ktime_to_ns(lock->stat.last_time));
}

static int wakelock_stats_show(struct seq_file *m, void *unused)
{
	unsigned long irqflags;
	struct wake_lock *lock;
	int ret;
	int type;

	spin_lock_irqsave(&list_lock, irqflags);

	ret = seq_puts(m, "name\tcount\texpire_count\twake_count\tactive_since"
			"\ttotal_time\tsleep_time\tmax_time\tlast_change\n");
	list_for_each_entry(lock, &inactive_locks, link)
		ret = print_lock_stat(m, lock);
	for (type = 0; type < WAKE_LOCK_TYPE_COUNT; type++) {
		list_for_each_entry(lock, &active_wake_locks[type], link)
			ret = print_lock_stat(m, lock);
	}
	spin_unlock_irqrestore(&list_lock, irqflags);
	return 0;
}

static void wake_unlock_stat_locked(struct wake_lock *lock, int expired)
{
	ktime_t duration;
	ktime_t now;
	if (!(lock->flags & WAKE_LOCK_ACTIVE))
		return;
	if (get_expired_time(lock, &now))
		expired = 1;
	else
		now = ktime_get();
	lock->stat.count++;
	if (expired)
		lock->stat.expire_count++;
	duration = ktime_sub(now, lock->stat.last_time);
	lock->stat.total_time = ktime_add(lock->stat.total_time, duration);
	if (ktime_to_ns(duration) > ktime_to_ns(lock->stat.max_time))
		lock->stat.max_time = duration;
	lock->stat.last_time = ktime_get();
	if (lock->flags & WAKE_LOCK_PREVENTING_SUSPEND) {
		duration = ktime_sub(now, last_sleep_time_update);
		lock->stat.prevent_suspend_time = ktime_add(
			lock->stat.prevent_suspend_time, duration);
		lock->flags &= ~WAKE_LOCK_PREVENTING_SUSPEND;
	}
}

static void update_sleep_wait_stats_locked(int done)
{
	struct wake_lock *lock;
	ktime_t now, etime, elapsed, add;
	int expired;

	now = ktime_get();
	elapsed = ktime_sub(now, last_sleep_time_update);
	list_for_each_entry(lock, &active_wake_locks[WAKE_LOCK_SUSPEND], link) {
		expired = get_expired_time(lock, &etime);
		if (lock->flags & WAKE_LOCK_PREVENTING_SUSPEND) {
			if (expired)
				add = ktime_sub(etime, last_sleep_time_update);
			else
				add = elapsed;
			lock->stat.prevent_suspend_time = ktime_add(
				lock->stat.prevent_suspend_time, add);
		}
		if (done || expired)
			lock->flags &= ~WAKE_LOCK_PREVENTING_SUSPEND;
		else
			lock->flags |= WAKE_LOCK_PREVENTING_SUSPEND;
	}
	last_sleep_time_update = now;
}
#endif


static void expire_wake_lock(struct wake_lock *lock)
{
#ifdef CONFIG_WAKELOCK_STAT
	wake_unlock_stat_locked(lock, 1);
#endif
	lock->flags &= ~(WAKE_LOCK_ACTIVE | WAKE_LOCK_AUTO_EXPIRE);
	list_del(&lock->link);
	list_add(&lock->link, &inactive_locks);
	if (debug_mask & (DEBUG_WAKE_LOCK | DEBUG_EXPIRE))
		pr_info("expired wake lock %s\n", lock->name);
}

/* Caller must acquire the list_lock spinlock */
static void print_active_locks(int type)
{
	struct wake_lock *lock;
	bool print_expired = true;

	BUG_ON(type >= WAKE_LOCK_TYPE_COUNT);
	list_for_each_entry(lock, &active_wake_locks[type], link) {
		if (lock->flags & WAKE_LOCK_AUTO_EXPIRE) {
			long timeout = lock->expires - jiffies;
			if (timeout > 0)
				pr_info("active wake lock %s, time left %ld\n",
					lock->name, timeout);
			else if (print_expired)
				pr_info("wake lock %s, expired\n", lock->name);
		} else {
			pr_info("active wake lock %s\n", lock->name);
			if (!(debug_mask & DEBUG_EXPIRE))
				print_expired = false;
		}
	}
}

static long has_wake_lock_locked(int type)
{
	struct wake_lock *lock, *n;
	long max_timeout = 0;

	BUG_ON(type >= WAKE_LOCK_TYPE_COUNT);
	list_for_each_entry_safe(lock, n, &active_wake_locks[type], link) {
		if (lock->flags & WAKE_LOCK_AUTO_EXPIRE) {
			long timeout = lock->expires - jiffies;
			if (timeout <= 0)
				expire_wake_lock(lock);
			else if (timeout > max_timeout)
				max_timeout = timeout;
		} else
			return -1;
	}
	return max_timeout;
}

long has_wake_lock(int type)
{
	long ret;
	unsigned long irqflags;
	spin_lock_irqsave(&list_lock, irqflags);
	ret = has_wake_lock_locked(type);
	if (ret && (debug_mask & DEBUG_SUSPEND) && type == WAKE_LOCK_SUSPEND)
		print_active_locks(type);
	spin_unlock_irqrestore(&list_lock, irqflags);
	return ret;
}

#ifdef CONFIG_ZTE_SUSPEND_WAKEUP_MONITOR
/*ZTE_HYJ_WAKELOCK_TOOL 2010.0114 begin*/
static void suspend_exception_handle(unsigned long data);
static DEFINE_TIMER(suspend_exception_timer, suspend_exception_handle, 0, 0);
static void dump_wake_locks(void);//ZTE_ZHENGCHAO_PM_20100406_01
/*ZTE_HYJ_PM_20100506 begin*/
static void suspend_exception_handle(unsigned long data)
{
	unsigned long irqflags;
	struct wake_lock *lock;
	static int record_index = 0;
	char *p_buf = active_wake_lock_buf[record_index];
	int used_size = 0;
	int ret =-1;
	int record_max_size = RECORD_SIZE -1;//the last char is '\0'
	struct timespec ts;
	struct rtc_time tm;

	dump_wake_locks();//ZTE_ZHENGCHAO_PM_20100406_01
	getnstimeofday(&ts);
	rtc_time_to_tm(ts.tv_sec, &tm);
	
	spin_lock_irqsave(&list_lock, irqflags);
	
	used_size = snprintf(p_buf, record_max_size, "(%d-%02d-%02d %02d:%02d:%02d.%09lu UTC)\n", 
		tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
		tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec);	
	if ( used_size <= 0 ){
		p_buf[0] = '\0';
		goto handle_out;
	}
		
	list_for_each_entry(lock, &active_wake_locks[WAKE_LOCK_SUSPEND], link) {
		if (lock->flags & WAKE_LOCK_AUTO_EXPIRE) {
			long timeout = lock->expires - jiffies;
			if (timeout <= 0)
				ret = snprintf(p_buf + used_size , record_max_size  - used_size, "wake lock %s, expired\n", lock->name);
			else
				ret  = snprintf(p_buf +  used_size, record_max_size  - used_size, "active wake lock %s, time left %ld\n",
					lock->name, timeout);
		} else
			ret = snprintf(p_buf + used_size, record_max_size  - used_size, "active wake lock %s\n", lock->name);

		if ( ret <= 0 ){
			p_buf[used_size] = '\0';
			goto handle_out;
		}
		used_size += ret;
	}
	
handle_out:	
	record_index = (record_index + 1) % WAKE_LOCK_RECORD_NR;
	/*ZTE_HYJ_PM_20100419_01 15Hz -> 5*60HZ*/
	mod_timer(&suspend_exception_timer, jiffies + 5*60*HZ);
	
	spin_unlock_irqrestore(&list_lock, irqflags);
}
/*ZTE_HYJ_WAKELOCK_TOOL 2010.0114 end*/
/*ZTE_HYJ_PM_20100506 end*/
#endif
//20100519 ZTE_WAKELOCK_LYJ_001 
static int unknown_wakeup_timeout = 500;
module_param_named(unknown_wakeup_timeout, unknown_wakeup_timeout, int, S_IRUGO | S_IWUSR | S_IWGRP)
static void suspend(struct work_struct *work)
{
	int ret;
	int entry_event_num;

     //ruanmeisi
	//	sys_sync();
	enqueue_sync_work(2 * HZ);
	//end
	
	if (has_wake_lock(WAKE_LOCK_SUSPEND)) {
		if (debug_mask & DEBUG_SUSPEND)
			pr_info("suspend: abort suspend\n");
		return;
	}
#ifdef	CONFIG_ZTE_SUSPEND_WAKEUP_MONITOR
	/*ZTE_HYJ_WAKELOCK_TOOL 2010.0114 begin*/
	if(timer_pending(&suspend_exception_timer))
		del_timer(&suspend_exception_timer);
/*ZTE_HYJ_WAKELOCK_TOOL 2010.0114 end*/	
#endif
	entry_event_num = current_event_num;
    //	sys_sync();
	
	if (debug_mask & DEBUG_SUSPEND)
		pr_info("suspend: enter suspend\n");
	ret = pm_suspend(requested_suspend_state);
	if (debug_mask & DEBUG_EXIT_SUSPEND) {
		struct timespec ts;
		struct rtc_time tm;
		getnstimeofday(&ts);
		rtc_time_to_tm(ts.tv_sec, &tm);
		pr_info("suspend: exit suspend, ret = %d "
			"(%d-%02d-%02d %02d:%02d:%02d.%09lu UTC)\n", ret,
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec, ts.tv_nsec);
	}
	if (current_event_num == entry_event_num) {
		if (debug_mask & DEBUG_SUSPEND)
			pr_info("suspend: pm_suspend returned with no event\n");
//20100519 ZTE_WAKELOCK_LYJ_001 
#if 0
        wake_lock_timeout(&unknown_wakeup, HZ / 2);
#else
        wake_lock_timeout(&unknown_wakeup, msecs_to_jiffies(unknown_wakeup_timeout));
#endif
	}
}
static DECLARE_WORK(suspend_work, suspend);

static void expire_wake_locks(unsigned long data)
{
	long has_lock;
	unsigned long irqflags;
	if (debug_mask & DEBUG_EXPIRE)
		pr_info("expire_wake_locks: start\n");
	spin_lock_irqsave(&list_lock, irqflags);
	if (debug_mask & DEBUG_SUSPEND)
		print_active_locks(WAKE_LOCK_SUSPEND);
	has_lock = has_wake_lock_locked(WAKE_LOCK_SUSPEND);
	if (debug_mask & DEBUG_EXPIRE)
		pr_info("expire_wake_locks: done, has_lock %ld\n", has_lock);
	if (has_lock == 0)
		queue_work(suspend_work_queue, &suspend_work);
	spin_unlock_irqrestore(&list_lock, irqflags);
}
static DEFINE_TIMER(expire_timer, expire_wake_locks, 0, 0);

static void dump_wake_locks(void)
{
	unsigned long irqflags;
	
	pr_info("[POWER]******expire_wake_locks: start********\n");
	spin_lock_irqsave(&list_lock, irqflags);	
	print_active_locks(WAKE_LOCK_SUSPEND);	
	spin_unlock_irqrestore(&list_lock, irqflags);
	pr_info("[POWER]********************\n");
}
static int power_suspend_late(struct device *dev)
{
	int ret = has_wake_lock(WAKE_LOCK_SUSPEND) ? -EAGAIN : 0;
#ifdef CONFIG_WAKELOCK_STAT
	wait_for_wakeup = 1;
#endif
	if (debug_mask & DEBUG_SUSPEND)
	{		
		pr_info("power_suspend_late return %d\n", ret);
		if (ret!=0)
			dump_wake_locks();
	}
	return ret;
}

static struct dev_pm_ops power_driver_pm_ops = {
	.suspend_noirq = power_suspend_late,
};

static struct platform_driver power_driver = {
	.driver.name = "power",
	.driver.pm = &power_driver_pm_ops,
};
static struct platform_device power_device = {
	.name = "power",
};

void wake_lock_init(struct wake_lock *lock, int type, const char *name)
{
	unsigned long irqflags = 0;

	if (name)
		lock->name = name;
	BUG_ON(!lock->name);

	if (debug_mask & DEBUG_WAKE_LOCK)
		pr_info("wake_lock_init name=%s\n", lock->name);
#ifdef CONFIG_WAKELOCK_STAT
	lock->stat.count = 0;
	lock->stat.expire_count = 0;
	lock->stat.wakeup_count = 0;
	lock->stat.total_time = ktime_set(0, 0);
	lock->stat.prevent_suspend_time = ktime_set(0, 0);
	lock->stat.max_time = ktime_set(0, 0);
	lock->stat.last_time = ktime_set(0, 0);
#endif
	lock->flags = (type & WAKE_LOCK_TYPE_MASK) | WAKE_LOCK_INITIALIZED;

	INIT_LIST_HEAD(&lock->link);
	spin_lock_irqsave(&list_lock, irqflags);
	list_add(&lock->link, &inactive_locks);
	spin_unlock_irqrestore(&list_lock, irqflags);
}
EXPORT_SYMBOL(wake_lock_init);

void wake_lock_destroy(struct wake_lock *lock)
{
	unsigned long irqflags;
	if (debug_mask & DEBUG_WAKE_LOCK)
		pr_info("wake_lock_destroy name=%s\n", lock->name);
	spin_lock_irqsave(&list_lock, irqflags);
	lock->flags &= ~WAKE_LOCK_INITIALIZED;
#ifdef CONFIG_WAKELOCK_STAT
	if (lock->stat.count) {
		deleted_wake_locks.stat.count += lock->stat.count;
		deleted_wake_locks.stat.expire_count += lock->stat.expire_count;
		deleted_wake_locks.stat.total_time =
			ktime_add(deleted_wake_locks.stat.total_time,
				  lock->stat.total_time);
		deleted_wake_locks.stat.prevent_suspend_time =
			ktime_add(deleted_wake_locks.stat.prevent_suspend_time,
				  lock->stat.prevent_suspend_time);
		deleted_wake_locks.stat.max_time =
			ktime_add(deleted_wake_locks.stat.max_time,
				  lock->stat.max_time);
	}
#endif
	list_del(&lock->link);
	spin_unlock_irqrestore(&list_lock, irqflags);
}
EXPORT_SYMBOL(wake_lock_destroy);

static void wake_lock_internal(
	struct wake_lock *lock, long timeout, int has_timeout)
{
	int type;
	unsigned long irqflags;
	long expire_in;

	spin_lock_irqsave(&list_lock, irqflags);
	type = lock->flags & WAKE_LOCK_TYPE_MASK;
	BUG_ON(type >= WAKE_LOCK_TYPE_COUNT);
	BUG_ON(!(lock->flags & WAKE_LOCK_INITIALIZED));
#ifdef CONFIG_WAKELOCK_STAT
	if (type == WAKE_LOCK_SUSPEND && wait_for_wakeup) {
		if (debug_mask & DEBUG_WAKEUP)
			pr_info("wakeup wake lock: %s\n", lock->name);
		wait_for_wakeup = 0;
		lock->stat.wakeup_count++;
	}
	if ((lock->flags & WAKE_LOCK_AUTO_EXPIRE) &&
	    (long)(lock->expires - jiffies) <= 0) {
		wake_unlock_stat_locked(lock, 0);
		lock->stat.last_time = ktime_get();
	}
#endif
	if (!(lock->flags & WAKE_LOCK_ACTIVE)) {
		lock->flags |= WAKE_LOCK_ACTIVE;
#ifdef CONFIG_WAKELOCK_STAT
		lock->stat.last_time = ktime_get();
#endif
	}
	list_del(&lock->link);
	if (has_timeout) {
		if (debug_mask & DEBUG_WAKE_LOCK)
			pr_info("wake_lock: %s, type %d, timeout %ld.%03lu\n",
				lock->name, type, timeout / HZ,
				(timeout % HZ) * MSEC_PER_SEC / HZ);
		lock->expires = jiffies + timeout;
		lock->flags |= WAKE_LOCK_AUTO_EXPIRE;
		list_add_tail(&lock->link, &active_wake_locks[type]);
	} else {
		if (debug_mask & DEBUG_WAKE_LOCK)
			pr_info("wake_lock: %s, type %d\n", lock->name, type);
		lock->expires = LONG_MAX;
		lock->flags &= ~WAKE_LOCK_AUTO_EXPIRE;
		list_add(&lock->link, &active_wake_locks[type]);
	}
	if (type == WAKE_LOCK_SUSPEND) {
#ifdef	CONFIG_ZTE_SUSPEND_WAKEUP_MONITOR			
	/*ZTE_HYJ_WAKELOCK_TOOL 2010.0114 begin*/	
		if (lock == &main_wake_lock) {
			current_event_num++;
			if(timer_pending(&suspend_exception_timer))
				del_timer(&suspend_exception_timer);
		}
	/*ZTE_HYJ_WAKELOCK_TOOL 2010.0114 end*/		
#endif		
#ifdef CONFIG_WAKELOCK_STAT
		if (lock == &main_wake_lock)
			update_sleep_wait_stats_locked(1);
		else if (!wake_lock_active(&main_wake_lock))
			update_sleep_wait_stats_locked(0);
#endif
		if (has_timeout)
			expire_in = has_wake_lock_locked(type);
		else
			expire_in = -1;
		if (expire_in > 0) {
			if (debug_mask & DEBUG_EXPIRE)
				pr_info("wake_lock: %s, start expire timer, "
					"%ld\n", lock->name, expire_in);
			mod_timer(&expire_timer, jiffies + expire_in);
		} else {
			if (del_timer(&expire_timer))
				if (debug_mask & DEBUG_EXPIRE)
					pr_info("wake_lock: %s, stop expire timer\n",
						lock->name);
			if (expire_in == 0)
				queue_work(suspend_work_queue, &suspend_work);
		}
	}
	spin_unlock_irqrestore(&list_lock, irqflags);
}

void wake_lock(struct wake_lock *lock)
{
	wake_lock_internal(lock, 0, 0);
}
EXPORT_SYMBOL(wake_lock);

void wake_lock_timeout(struct wake_lock *lock, long timeout)
{
	wake_lock_internal(lock, timeout, 1);
}
EXPORT_SYMBOL(wake_lock_timeout);

void wake_unlock(struct wake_lock *lock)
{
	int type;
	unsigned long irqflags;
	spin_lock_irqsave(&list_lock, irqflags);
	type = lock->flags & WAKE_LOCK_TYPE_MASK;
#ifdef CONFIG_WAKELOCK_STAT
	wake_unlock_stat_locked(lock, 0);
#endif
	if (debug_mask & DEBUG_WAKE_LOCK)
		pr_info("wake_unlock: %s\n", lock->name);
	lock->flags &= ~(WAKE_LOCK_ACTIVE | WAKE_LOCK_AUTO_EXPIRE);
	list_del(&lock->link);
	list_add(&lock->link, &inactive_locks);
#ifdef	CONFIG_ZTE_SUSPEND_WAKEUP_MONITOR	
/*ZTE_HYJ_WAKELOCK_TOOL 2010.0114 begin*/
	if (lock == &main_wake_lock) 
		mod_timer(&suspend_exception_timer,jiffies + 5*60*HZ); /*ZTE_HYJ_PM_20100419_01 15Hz -> 5*60HZ*/
/*ZTE_HYJ_WAKELOCK_TOOL 2010.0114 end*/	
#endif
	if (type == WAKE_LOCK_SUSPEND) {
		long has_lock = has_wake_lock_locked(type);
		if (has_lock > 0) {
			if (debug_mask & DEBUG_EXPIRE)
				pr_info("wake_unlock: %s, start expire timer, "
					"%ld\n", lock->name, has_lock);
			mod_timer(&expire_timer, jiffies + has_lock);
		} else {
			if (del_timer(&expire_timer))
				if (debug_mask & DEBUG_EXPIRE)
					pr_info("wake_unlock: %s, stop expire "
						"timer\n", lock->name);
			if (has_lock == 0)
				queue_work(suspend_work_queue, &suspend_work);
		}
		if (lock == &main_wake_lock) {
			if (debug_mask & DEBUG_SUSPEND)
				print_active_locks(WAKE_LOCK_SUSPEND);
#ifdef CONFIG_WAKELOCK_STAT
			update_sleep_wait_stats_locked(0);
#endif
		}
	}
	spin_unlock_irqrestore(&list_lock, irqflags);
}
EXPORT_SYMBOL(wake_unlock);

int wake_lock_active(struct wake_lock *lock)
{
	return !!(lock->flags & WAKE_LOCK_ACTIVE);
}
EXPORT_SYMBOL(wake_lock_active);

static int wakelock_stats_open(struct inode *inode, struct file *file)
{
	return single_open(file, wakelock_stats_show, NULL);
}

static const struct file_operations wakelock_stats_fops = {
	.owner = THIS_MODULE,
	.open = wakelock_stats_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init wakelocks_init(void)
{
	int ret;
	int i;

	for (i = 0; i < ARRAY_SIZE(active_wake_locks); i++)
		INIT_LIST_HEAD(&active_wake_locks[i]);

#ifdef CONFIG_WAKELOCK_STAT
	wake_lock_init(&deleted_wake_locks, WAKE_LOCK_SUSPEND,
			"deleted_wake_locks");
#endif
	wake_lock_init(&main_wake_lock, WAKE_LOCK_SUSPEND, "main");
	wake_lock(&main_wake_lock);
	wake_lock_init(&unknown_wakeup, WAKE_LOCK_SUSPEND, "unknown_wakeups");

	ret = platform_device_register(&power_device);
	if (ret) {
		pr_err("wakelocks_init: platform_device_register failed\n");
		goto err_platform_device_register;
	}
	ret = platform_driver_register(&power_driver);
	if (ret) {
		pr_err("wakelocks_init: platform_driver_register failed\n");
		goto err_platform_driver_register;
	}

	suspend_work_queue = create_singlethread_workqueue("suspend");

	//ruanmeisi
	create_suspend_sync_work();
	ret = device_create_file(&power_device.dev, &dev_attr_suspend_sync);
	if (ret) {
		pr_err("wakelocks_init: suspend_sync device_create_file failed\n");
	}

	//end

	if (suspend_work_queue == NULL) {
		ret = -ENOMEM;
		goto err_suspend_work_queue;
	}
#ifdef CONFIG_ZTE_SUSPEND_WAKEUP_MONITOR
	/*ZTE_HYJ_WAKELOCK_TOOL 2010.0114 begin*/
	ret = device_create_file(&power_device.dev,&dev_attr_active_wakelock);
	if (ret) {
		pr_err("wakelocks_init: device_create_file failed\n");
		goto err_suspend_work_queue;
	}
	/*ZTE_HYJ_WAKELOCK_TOOL 2010.0114 begin*/
#endif	

#ifdef CONFIG_WAKELOCK_STAT
	proc_create("wakelocks", S_IRUGO, NULL, &wakelock_stats_fops);
#endif

	return 0;

err_suspend_work_queue:
	platform_driver_unregister(&power_driver);
err_platform_driver_register:
	platform_device_unregister(&power_device);
err_platform_device_register:
	wake_lock_destroy(&unknown_wakeup);
	wake_lock_destroy(&main_wake_lock);
#ifdef CONFIG_WAKELOCK_STAT
	wake_lock_destroy(&deleted_wake_locks);
#endif
	return ret;
}

static void  __exit wakelocks_exit(void)
{
	//ruanmeisi
	device_remove_file(&power_device.dev, &dev_attr_suspend_sync);
	destroy_suspend_sync_work();
	//end
	 
#ifdef CONFIG_WAKELOCK_STAT
	remove_proc_entry("wakelocks", NULL);
#endif
	destroy_workqueue(suspend_work_queue);
	platform_driver_unregister(&power_driver);
	platform_device_unregister(&power_device);
	wake_lock_destroy(&unknown_wakeup);
	wake_lock_destroy(&main_wake_lock);
#ifdef CONFIG_WAKELOCK_STAT
	wake_lock_destroy(&deleted_wake_locks);
#endif
}

core_initcall(wakelocks_init);
module_exit(wakelocks_exit);
