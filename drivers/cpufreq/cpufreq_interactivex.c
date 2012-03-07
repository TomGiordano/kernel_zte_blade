/*
 * drivers/cpufreq/cpufreq_interactivex.c
 *
 * Copyright (C) 2010 Google, Inc.
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
 * Author: Mike Chan (mike@android.com) - modified for suspend/wake by imoseyon
 *
 */

#include <linux/cpu.h>
#include <linux/cpumask.h>
#include <linux/cpufreq.h>
#include <linux/mutex.h>
#include <linux/sched.h>
#include <linux/tick.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/earlysuspend.h>

#include <asm/cputime.h>

static void (*pm_idle_old)(void);
static atomic_t active_count = ATOMIC_INIT(0);

static DEFINE_PER_CPU(struct timer_list, cpu_timer);

static DEFINE_PER_CPU(u64, time_in_idle);
static DEFINE_PER_CPU(u64, idle_exit_time);

static struct cpufreq_policy *policy;
static unsigned int target_freq;

/* Workqueues handle frequency scaling */
static struct workqueue_struct *up_wq;
static struct workqueue_struct *down_wq;
static struct work_struct freq_scale_work;

static u64 freq_change_time;
static u64 freq_change_time_in_idle;

static cpumask_t work_cpumask;

static unsigned int suspended = 0;
static unsigned int enabled = 0;

static unsigned int suspendfreq = 352000;

static unsigned int samples = 0;

/*
 * The minimum ammount of time to spend at a frequency before we can ramp down,
 * default is 50ms.
 */
#define DEFAULT_MIN_SAMPLE_TIME 44000;
static unsigned long min_sample_time;

static unsigned int freq_threshold = 614400;
static unsigned int resume_speed = 518400;

static int cpufreq_governor_interactivex(struct cpufreq_policy *policy,
		unsigned int event);

#ifndef CONFIG_CPU_FREQ_DEFAULT_GOV_INTERACTIVEX
static
#endif
struct cpufreq_governor cpufreq_gov_interactivex = {
	.name = "interactiveX",
	.governor = cpufreq_governor_interactivex,
	.max_transition_latency = 10000000,
	.owner = THIS_MODULE,
};

static void cpufreq_interactivex_timer(unsigned long data)
{
	u64 delta_idle;
	u64 update_time;
	u64 *cpu_time_in_idle;
	u64 *cpu_idle_exit_time;
	struct timer_list *t;

	u64 now_idle = get_cpu_idle_time_us(data,
						&update_time);


	cpu_time_in_idle = &per_cpu(time_in_idle, data);
	cpu_idle_exit_time = &per_cpu(idle_exit_time, data);

	if (update_time == *cpu_idle_exit_time)
		return;

	delta_idle = cputime64_sub(now_idle, *cpu_time_in_idle);

	/* Scale up if there were no idle cycles since coming out of idle */
	if (delta_idle == 0) {
		if (policy->cur == policy->max)
			return;

		if (nr_running() < 1)
			return;

		// imoseyon - when over 1.8Ghz jump less
		if (policy->max > freq_threshold) {
			if (samples > 0) {
			  target_freq = policy->max;
			  samples = 0;
			} else { 
			  samples++;
			  target_freq = freq_threshold;
			}  
		} else target_freq = policy->max;

		cpumask_set_cpu(data, &work_cpumask);
		queue_work(up_wq, &freq_scale_work);
		return;
	}
	samples = 0; // reset sample counter
	/*
	 * There is a window where if the cpu utlization can go from low to high
	 * between the timer expiring, delta_idle will be > 0 and the cpu will
	 * be 100% busy, preventing idle from running, and this timer from
	 * firing. So setup another timer to fire to check cpu utlization.
	 * Do not setup the timer if there is no scheduled work.
	 */
	t = &per_cpu(cpu_timer, data);
	if (!timer_pending(t) && nr_running() > 0) {
			*cpu_time_in_idle = get_cpu_idle_time_us(
					data, cpu_idle_exit_time);
			mod_timer(t, jiffies + 2);
	}

	if (policy->cur == policy->min)
		return;

	/*
	 * Do not scale down unless we have been at this frequency for the
	 * minimum sample time.
	 */
	if (cputime64_sub(update_time, freq_change_time) < min_sample_time)
		return;

	target_freq = policy->min;
	cpumask_set_cpu(data, &work_cpumask);
	queue_work(down_wq, &freq_scale_work);
}

static void cpufreq_idle(void)
{
	struct timer_list *t;
	u64 *cpu_time_in_idle;
	u64 *cpu_idle_exit_time;

	pm_idle_old();

	if (!cpumask_test_cpu(smp_processor_id(), policy->cpus))
			return;

	/* Timer to fire in 1-2 ticks, jiffie aligned. */
	t = &per_cpu(cpu_timer, smp_processor_id());
	cpu_idle_exit_time = &per_cpu(idle_exit_time, smp_processor_id());
	cpu_time_in_idle = &per_cpu(time_in_idle, smp_processor_id());

	if (timer_pending(t) == 0) {
		*cpu_time_in_idle = get_cpu_idle_time_us(
				smp_processor_id(), cpu_idle_exit_time);
		mod_timer(t, jiffies + 2);
	}
}

/*
 * Choose the cpu frequency based off the load. For now choose the minimum
 * frequency that will satisfy the load, which is not always the lower power.
 */
static unsigned int cpufreq_interactivex_calc_freq(unsigned int cpu)
{
	unsigned int delta_time;
	unsigned int idle_time;
	unsigned int cpu_load;
	unsigned int newfreq;
	u64 current_wall_time;
	u64 current_idle_time;;

	current_idle_time = get_cpu_idle_time_us(cpu, &current_wall_time);

	idle_time = (unsigned int) current_idle_time - freq_change_time_in_idle;
	delta_time = (unsigned int) current_wall_time - freq_change_time;

	cpu_load = 100 * (delta_time - idle_time) / delta_time;

	newfreq = policy->cur * cpu_load / 100;	

	return newfreq;
}


/* We use the same work function to sale up and down */
static void cpufreq_interactivex_freq_change_time_work(struct work_struct *work)
{
	unsigned int cpu;
	cpumask_t tmp_mask = work_cpumask;

	for_each_cpu(cpu, tmp_mask) {
		if (!suspended && (target_freq >= freq_threshold || target_freq == policy->max) ) {
			if (policy->cur < 400000) {
			  // avoid quick jump from lowest to highest
			  target_freq = resume_speed;
			}
			if (nr_running() == 1) {
				cpumask_clear_cpu(cpu, &work_cpumask);
				return;
			}
			__cpufreq_driver_target(policy, target_freq, CPUFREQ_RELATION_H);
		} else {
			if (!suspended) {
		  	  target_freq = cpufreq_interactivex_calc_freq(cpu);
			  __cpufreq_driver_target(policy, target_freq, CPUFREQ_RELATION_L);
			} else {  // special care when suspended
			  if (target_freq > suspendfreq) {
			     __cpufreq_driver_target(policy, suspendfreq, CPUFREQ_RELATION_H);
			  } else {
		  	    target_freq = cpufreq_interactivex_calc_freq(cpu);
			    if (target_freq < policy->cur) 
			      __cpufreq_driver_target(policy, target_freq, CPUFREQ_RELATION_H);
			  }
		       }
		}
	  freq_change_time_in_idle = get_cpu_idle_time_us(cpu, &freq_change_time);
	  cpumask_clear_cpu(cpu, &work_cpumask);
	}


}

static ssize_t show_min_sample_time(struct kobject *kobj,
				struct attribute *attr, char *buf)
{
	return sprintf(buf, "%lu\n", min_sample_time);
}

static ssize_t store_min_sample_time(struct kobject *kobj,
			struct attribute *attr, const char *buf, size_t count)
{
	return strict_strtoul(buf, 0, &min_sample_time);
}

static struct global_attr min_sample_time_attr = __ATTR(min_sample_time, 0644,
		show_min_sample_time, store_min_sample_time);

static struct attribute *interactivex_attributes[] = {
	&min_sample_time_attr.attr,
	NULL,
};

static struct attribute_group interactivex_attr_group = {
	.attrs = interactivex_attributes,
	.name = "interactivex",
};

static void interactivex_suspend(int suspend)
{
	unsigned int max_speed;

	max_speed = resume_speed;

	if (!enabled) return;
        if (!suspend) { // resume at max speed:
		suspended = 0;
                __cpufreq_driver_target(policy, max_speed, CPUFREQ_RELATION_L);
                pr_info("[imoseyon] interactivex awake at %d\n", policy->cur);
        } else {
		suspended = 1;
                __cpufreq_driver_target(policy, suspendfreq, CPUFREQ_RELATION_H);
                pr_info("[imoseyon] interactivex suspended at %d\n", policy->cur);
        }
}

static void interactivex_early_suspend(struct early_suspend *handler) {
     interactivex_suspend(1);
}

static void interactivex_late_resume(struct early_suspend *handler) {
     interactivex_suspend(0);
}

static struct early_suspend interactivex_power_suspend = {
        .suspend = interactivex_early_suspend,
        .resume = interactivex_late_resume,
        .level = EARLY_SUSPEND_LEVEL_DISABLE_FB + 1,
};

static int cpufreq_governor_interactivex(struct cpufreq_policy *new_policy,
		unsigned int event)
{
	int rc;
	switch (event) {
	case CPUFREQ_GOV_START:
		if (!cpu_online(new_policy->cpu))
			return -EINVAL;

		/*
		 * Do not register the idle hook and create sysfs
		 * entries if we have already done so.
		 */
		if (atomic_inc_return(&active_count) > 1)
			return 0;

		rc = sysfs_create_group(cpufreq_global_kobject,
				&interactivex_attr_group);
		if (rc)
			return rc;

		pm_idle_old = pm_idle;
		pm_idle = cpufreq_idle;
		policy = new_policy;
		enabled = 1;

        	register_early_suspend(&interactivex_power_suspend);
        	pr_info("[imoseyon] interactivex start - freq_threshold at %d, resume at %d\n", freq_threshold, resume_speed);
		break;

	case CPUFREQ_GOV_STOP:
		if (atomic_dec_return(&active_count) > 1)
			return 0;

		sysfs_remove_group(cpufreq_global_kobject,
				&interactivex_attr_group);

		pm_idle = pm_idle_old;
		del_timer(&per_cpu(cpu_timer, new_policy->cpu));
		enabled = 0;
        	unregister_early_suspend(&interactivex_power_suspend);
        	pr_info("[imoseyon] interactivex inactive\n");
			break;

	case CPUFREQ_GOV_LIMITS:
		if (new_policy->max < new_policy->cur)
			__cpufreq_driver_target(new_policy,
					new_policy->max, CPUFREQ_RELATION_H);
		else if (new_policy->min > new_policy->cur)
			__cpufreq_driver_target(new_policy,
					new_policy->min, CPUFREQ_RELATION_L);
		break;
	}
	return 0;
}

static int __init cpufreq_interactivex_init(void)
{
	unsigned int i;
	struct timer_list *t;
	min_sample_time = DEFAULT_MIN_SAMPLE_TIME;

	/* Initalize per-cpu timers */
	for_each_possible_cpu(i) {
		t = &per_cpu(cpu_timer, i);
		init_timer_deferrable(t);
		t->function = cpufreq_interactivex_timer;
		t->data = i;
	}

	/* Scale up is high priority */
	up_wq = create_rt_workqueue("kinteractivex_up");
	down_wq = create_workqueue("kinteractivex_down");

	INIT_WORK(&freq_scale_work, cpufreq_interactivex_freq_change_time_work);
    
    pr_info("[imoseyon] interactivex enter\n");
	return cpufreq_register_governor(&cpufreq_gov_interactivex);
}

#ifdef CONFIG_CPU_FREQ_DEFAULT_GOV_INTERACTIVEX
pure_initcall(cpufreq_interactivex_init);
#else
module_init(cpufreq_interactivex_init);
#endif

static void __exit cpufreq_interactivex_exit(void)
{
    pr_info("[imoseyon] interactivex exit\n");
	cpufreq_unregister_governor(&cpufreq_gov_interactivex);
	destroy_workqueue(up_wq);
	destroy_workqueue(down_wq);
}

module_exit(cpufreq_interactivex_exit);

MODULE_AUTHOR("Mike Chan <mike@android.com>");
MODULE_DESCRIPTION("'cpufreq_interactivex' - A cpufreq governor for "
	"Latency sensitive workloads");
MODULE_LICENSE("GPL");
