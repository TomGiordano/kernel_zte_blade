/*
 * Copyright (C) 2007 Google, Inc.
 * Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
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

#include <linux/init.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/clk.h>
#include <linux/clockchips.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/percpu.h>

#include <asm/mach/time.h>
#include <mach/msm_iomap.h>
#include <mach/irqs.h>

#if defined(CONFIG_MSM_SMD)
#include "smd_private.h"
#endif
#include "timer.h"

enum {
	MSM_TIMER_DEBUG_SYNC = 1U << 0,
};
static int msm_timer_debug_mask;
module_param_named(debug_mask, msm_timer_debug_mask, int, S_IRUGO | S_IWUSR | S_IWGRP);

#if defined(CONFIG_ARCH_MSM7X30) || defined(CONFIG_ARCH_MSM8X60)
#define MSM_GPT_BASE (MSM_TMR_BASE + 0x4)
#define MSM_DGT_BASE (MSM_TMR_BASE + 0x24)
#else
#define MSM_GPT_BASE MSM_TMR_BASE
#define MSM_DGT_BASE (MSM_TMR_BASE + 0x10)
#endif

#ifdef CONFIG_MSM7X00A_USE_GP_TIMER
	#define DG_TIMER_RATING 100
	#define MSM_GLOBAL_TIMER MSM_CLOCK_GPT
#else
	#define DG_TIMER_RATING 300
	#define MSM_GLOBAL_TIMER MSM_CLOCK_DGT
#endif

#if defined(CONFIG_ARCH_MSM_ARM11)
#define MSM_DGT_SHIFT (5)
#else
#define MSM_DGT_SHIFT (0)
#endif

#define TIMER_MATCH_VAL         0x0000
#define TIMER_COUNT_VAL         0x0004
#define TIMER_ENABLE            0x0008
#define TIMER_CLEAR             0x000C
#define DGT_CLK_CTL             0x0034
enum {
	DGT_CLK_CTL_DIV_1 = 0,
	DGT_CLK_CTL_DIV_2 = 1,
	DGT_CLK_CTL_DIV_3 = 2,
	DGT_CLK_CTL_DIV_4 = 3,
};
#define TIMER_ENABLE_EN              1
#define TIMER_ENABLE_CLR_ON_MATCH_EN 2

#define LOCAL_TIMER 0
#define GLOBAL_TIMER 1

/*
 * MSM_TMR_GLOBAL is added to the regbase of a timer to force the memory access
 * to come from the CPU0 region.
 */
#ifdef MSM_TMR0_BASE
#define MSM_TMR_GLOBAL		(MSM_TMR0_BASE - MSM_TMR_BASE)
#else
#define MSM_TMR_GLOBAL		0
#endif

#if defined(CONFIG_MSM_DIRECT_SCLK_ACCESS)
#define MPM_SCLK_COUNT_VAL    0x0024
#endif

#define NR_TIMERS ARRAY_SIZE(msm_clocks)

#if defined(CONFIG_ARCH_QSD8X50)
#define DGT_HZ 4800000	/* Uses TCXO/4 (19.2 MHz / 4) */
#elif defined(CONFIG_ARCH_MSM7X30) || defined(CONFIG_ARCH_MSM8X60)
#define DGT_HZ 6144000	/* Uses LPXO/4 (24.576 MHz / 4) */
#else
#define DGT_HZ 19200000	/* Uses TCXO (19.2 MHz) */
#endif

#define GPT_HZ 32768
#define SCLK_HZ 32768

#if defined(CONFIG_MSM_N_WAY_SMSM)
/* Time Master State Bits */
#define MASTER_BITS_PER_CPU        1
#define MASTER_TIME_PENDING \
	(0x01UL << (MASTER_BITS_PER_CPU * SMSM_APPS_STATE))

/* Time Slave State Bits */
#define SLAVE_TIME_REQUEST         0x0400
#define SLAVE_TIME_POLL            0x0800
#define SLAVE_TIME_INIT            0x1000
#endif

static irqreturn_t msm_timer_interrupt(int irq, void *dev_id);
static cycle_t msm_gpt_read(struct clocksource *cs);
static cycle_t msm_dgt_read(struct clocksource *cs);
static void msm_timer_set_mode(enum clock_event_mode mode,
			       struct clock_event_device *evt);
static int msm_timer_set_next_event(unsigned long cycles,
				    struct clock_event_device *evt);

enum {
	MSM_CLOCK_FLAGS_UNSTABLE_COUNT = 1U << 0,
	MSM_CLOCK_FLAGS_ODD_MATCH_WRITE = 1U << 1,
	MSM_CLOCK_FLAGS_DELAYED_WRITE_POST = 1U << 2,
};

struct msm_clock {
	struct clock_event_device   clockevent;
	struct clocksource          clocksource;
	struct irqaction            irq;
	void __iomem                *regbase;
	uint32_t                    freq;
	uint32_t                    shift;
	uint32_t                    flags;
	uint32_t                    write_delay;
	uint32_t                    rollover_offset;
	uint32_t                    index;
};

enum {
	MSM_CLOCK_GPT,
	MSM_CLOCK_DGT,
};


struct msm_clock_percpu_data {
	uint32_t                  last_set;
	uint32_t                  sleep_offset;
	uint32_t                  alarm_vtime;
	uint32_t                  alarm;
	uint32_t                  non_sleep_offset;
	uint32_t                  in_sync;
	cycle_t                   stopped_tick;
	int                       stopped;
	uint32_t                  last_sync_gpt;
	u64                       last_sync_jiffies;
};

struct msm_timer_sync_data_t {
	struct msm_clock *clock;
	uint32_t         timeout;
	int              exit_sleep;
};

static struct msm_clock msm_clocks[] = {
	[MSM_CLOCK_GPT] = {
		.clockevent = {
			.name           = "gp_timer",
			.features       = CLOCK_EVT_FEAT_ONESHOT,
			.shift          = 32,
			.rating         = 200,
			.set_next_event = msm_timer_set_next_event,
			.set_mode       = msm_timer_set_mode,
		},
		.clocksource = {
			.name           = "gp_timer",
			.rating         = 200,
			.read           = msm_gpt_read,
			.mask           = CLOCKSOURCE_MASK(32),
			.shift          = 17,
			.flags          = CLOCK_SOURCE_IS_CONTINUOUS,
		},
		.irq = {
			.name    = "gp_timer",
			.flags   = IRQF_DISABLED | IRQF_TIMER |
				   IRQF_TRIGGER_RISING,
			.handler = msm_timer_interrupt,
			.dev_id  = &msm_clocks[0].clockevent,
			.irq     = INT_GP_TIMER_EXP
		},
		.regbase = MSM_GPT_BASE,
		.freq = GPT_HZ,
		.index = MSM_CLOCK_GPT,
		.flags =
			MSM_CLOCK_FLAGS_UNSTABLE_COUNT |
			MSM_CLOCK_FLAGS_ODD_MATCH_WRITE |
			MSM_CLOCK_FLAGS_DELAYED_WRITE_POST,
		.write_delay = 9,
	},
	[MSM_CLOCK_DGT] = {
		.clockevent = {
			.name           = "dg_timer",
			.features       = CLOCK_EVT_FEAT_ONESHOT,
			.shift          = 32 + MSM_DGT_SHIFT,
			.rating         = DG_TIMER_RATING,
			.set_next_event = msm_timer_set_next_event,
			.set_mode       = msm_timer_set_mode,
		},
		.clocksource = {
			.name           = "dg_timer",
			.rating         = DG_TIMER_RATING,
			.read           = msm_dgt_read,
			.mask           = CLOCKSOURCE_MASK((32-MSM_DGT_SHIFT)),
			.shift          = 24 - MSM_DGT_SHIFT,
			.flags          = CLOCK_SOURCE_IS_CONTINUOUS,
		},
		.irq = {
			.name    = "dg_timer",
			.flags   = IRQF_DISABLED | IRQF_TIMER |
				   IRQF_TRIGGER_RISING,
			.handler = msm_timer_interrupt,
			.dev_id  = &msm_clocks[1].clockevent,
			.irq     = INT_DEBUG_TIMER_EXP
		},
		.regbase = MSM_DGT_BASE,
		.freq = DGT_HZ >> MSM_DGT_SHIFT,
		.index = MSM_CLOCK_DGT,
		.shift = MSM_DGT_SHIFT,
		.write_delay = 9,
	}
};

static struct clock_event_device *local_clock_event;

static DEFINE_PER_CPU(struct msm_clock_percpu_data[NR_TIMERS],
    msm_clocks_percpu);

static DEFINE_PER_CPU(struct msm_clock *, msm_active_clock);

static irqreturn_t msm_timer_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *evt = dev_id;
	if (smp_processor_id() != 0)
		evt = local_clock_event;
	if (evt->event_handler == NULL)
		return IRQ_HANDLED;
	evt->event_handler(evt);
	return IRQ_HANDLED;
}

static uint32_t msm_read_timer_count(struct msm_clock *clock, int global)
{
	uint32_t t1, t2;
	int loop_count = 0;

	if (global)
		t1 = readl(clock->regbase + TIMER_COUNT_VAL + MSM_TMR_GLOBAL);
	else
		t1 = readl(clock->regbase + TIMER_COUNT_VAL);

	if (!(clock->flags & MSM_CLOCK_FLAGS_UNSTABLE_COUNT))
		return t1;
	while (1) {
		if (global)
			t2 = readl(clock->regbase + TIMER_COUNT_VAL +
					MSM_TMR_GLOBAL);
		else
			t2 = readl(clock->regbase + TIMER_COUNT_VAL);
		if (t1 == t2)
			return t1;
		if (loop_count++ > 10) {
			printk(KERN_ERR "msm_read_timer_count timer %s did not"
			       "stabilize %u != %u\n", clock->clockevent.name,
			       t2, t1);
			return t2;
		}
		t1 = t2;
	}
}

static cycle_t msm_gpt_read(struct clocksource *cs)
{
	struct msm_clock *clock = &msm_clocks[MSM_CLOCK_GPT];
	struct msm_clock_percpu_data *clock_state =
		&per_cpu(msm_clocks_percpu, 0)[MSM_CLOCK_GPT];

	if (clock_state->stopped)
		return clock_state->stopped_tick;

	return msm_read_timer_count(clock, GLOBAL_TIMER) +
		clock_state->sleep_offset;
}

static cycle_t msm_dgt_read(struct clocksource *cs)
{
	struct msm_clock *clock = &msm_clocks[MSM_CLOCK_DGT];
	struct msm_clock_percpu_data *clock_state =
		&per_cpu(msm_clocks_percpu, 0)[MSM_CLOCK_DGT];

	if (clock_state->stopped)
		return clock_state->stopped_tick >> MSM_DGT_SHIFT;

	return (msm_read_timer_count(clock, GLOBAL_TIMER) +
		clock_state->sleep_offset) >> MSM_DGT_SHIFT;
}

#ifdef CONFIG_SMP
static struct msm_clock *clockevent_to_clock(struct clock_event_device *evt)
{
	int i;
	for (i = 0; i < NR_TIMERS; i++)
		if (evt == &(msm_clocks[i].clockevent))
			return &msm_clocks[i];
	return &msm_clocks[MSM_GLOBAL_TIMER];
}
#endif

static int msm_timer_set_next_event(unsigned long cycles,
				    struct clock_event_device *evt)
{
	int i;
	struct msm_clock *clock;
	struct msm_clock_percpu_data *clock_state;
	uint32_t now;
	uint32_t alarm;
	int late;

#ifdef CONFIG_SMP
	clock = clockevent_to_clock(evt);
#else
	clock = container_of(evt, struct msm_clock, clockevent);
#endif
	clock_state = &__get_cpu_var(msm_clocks_percpu)[clock->index];
	if (clock_state->stopped)
		return 0;
	now = msm_read_timer_count(clock, LOCAL_TIMER);
	alarm = now + (cycles << clock->shift);
	if (clock->flags & MSM_CLOCK_FLAGS_ODD_MATCH_WRITE)
		while (now == clock_state->last_set)
			now = msm_read_timer_count(clock, LOCAL_TIMER);

	clock_state->alarm = alarm;
	writel(alarm, clock->regbase + TIMER_MATCH_VAL);

	if (clock->flags & MSM_CLOCK_FLAGS_DELAYED_WRITE_POST) {
		/* read the counter four extra times to make sure write posts
		   before reading the time */
		for (i = 0; i < 4; i++)
			readl(clock->regbase + TIMER_COUNT_VAL);
	}
	now = msm_read_timer_count(clock, LOCAL_TIMER);
	clock_state->last_set = now;
	clock_state->alarm_vtime = alarm + clock_state->sleep_offset;
	late = now - alarm;
	if (late >= (int)(-clock->write_delay << clock->shift) &&
              late < clock->freq*5)
		return -ETIME;
	return 0;
}

static void msm_timer_set_mode(enum clock_event_mode mode,
			       struct clock_event_device *evt)
{
	struct msm_clock *clock;
	struct msm_clock_percpu_data *clock_state, *gpt_state;
	unsigned long irq_flags;

#ifdef CONFIG_SMP
	clock = clockevent_to_clock(evt);
#else
	clock = container_of(evt, struct msm_clock, clockevent);
#endif
	clock_state = &__get_cpu_var(msm_clocks_percpu)[clock->index];
	gpt_state = &__get_cpu_var(msm_clocks_percpu)[MSM_CLOCK_GPT];

	local_irq_save(irq_flags);

	switch (mode) {
	case CLOCK_EVT_MODE_RESUME:
	case CLOCK_EVT_MODE_PERIODIC:
		break;
	case CLOCK_EVT_MODE_ONESHOT:
		clock_state->stopped = 0;
		clock_state->sleep_offset =
			-msm_read_timer_count(clock, LOCAL_TIMER) +
			clock_state->stopped_tick;
		get_cpu_var(msm_active_clock) = clock;
		put_cpu_var(msm_active_clock);
		writel(TIMER_ENABLE_EN, clock->regbase + TIMER_ENABLE);
		if (clock != &msm_clocks[MSM_CLOCK_GPT])
			writel(TIMER_ENABLE_EN,
				msm_clocks[MSM_CLOCK_GPT].regbase +
			       TIMER_ENABLE);
		break;
	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
		get_cpu_var(msm_active_clock) = NULL;
		put_cpu_var(msm_active_clock);
		clock_state->in_sync = 0;
		clock_state->stopped = 1;
		clock_state->stopped_tick =
			msm_read_timer_count(clock, LOCAL_TIMER) +
			clock_state->sleep_offset;
#ifdef CONFIG_ARCH_MSM_SCORPIONMP
		if (clock != &msm_clocks[MSM_CLOCK_DGT])
#endif
			writel(0, clock->regbase + TIMER_ENABLE);
		if (clock != &msm_clocks[MSM_CLOCK_GPT]) {
			gpt_state->in_sync = 0;
			writel(0, msm_clocks[MSM_CLOCK_GPT].regbase +
			       TIMER_ENABLE);
		}
		break;
	}
	local_irq_restore(irq_flags);
}

/*
 * Retrieve the cycle count from sclk and optionally synchronize local clock
 * with the sclk value.
 *
 * time_start and time_expired are callbacks that must be specified.  The
 * protocol uses them to detect timeout.  The update callback is optional.
 * If not NULL, update will be called so that it can update local clock.
 *
 * The function does not use the argument data directly; it passes data to
 * the callbacks.
 *
 * Return value:
 *      0: the operation failed
 *      >0: the slow clock value after time-sync
 */
static void (*msm_timer_sync_timeout)(void);
#if defined(CONFIG_MSM_DIRECT_SCLK_ACCESS)
static uint32_t msm_timer_do_sync_to_sclk(
	void (*time_start)(struct msm_timer_sync_data_t *data),
	bool (*time_expired)(struct msm_timer_sync_data_t *data),
	void (*update)(struct msm_timer_sync_data_t *, uint32_t, uint32_t),
	struct msm_timer_sync_data_t *data)
{
	uint32_t t1, t2;
	int loop_count = 10;
	int loop_zero_count = 3;
	int tmp = USEC_PER_SEC/SCLK_HZ/(loop_zero_count-1);

	while (loop_zero_count--) {
		t1 = readl(MSM_RPM_MPM_BASE + MPM_SCLK_COUNT_VAL);
		do {
			udelay(1);
			t2 = t1;
			t1 = readl(MSM_RPM_MPM_BASE + MPM_SCLK_COUNT_VAL);
		} while ((t2 != t1) && --loop_count);

		if (!loop_count) {
			printk(KERN_EMERG "SCLK  did not stabilize\n");
			return 0;
		}

		if (t1)
			break;

		udelay(tmp);
	}

	if (!loop_zero_count) {
		printk(KERN_EMERG "SCLK reads zero\n");
		return 0;
	}

	if (update != NULL)
		update(data, t1, SCLK_HZ);
	return t1;
}
#elif defined(CONFIG_MSM_N_WAY_SMSM)
static uint32_t msm_timer_do_sync_to_sclk(
	void (*time_start)(struct msm_timer_sync_data_t *data),
	bool (*time_expired)(struct msm_timer_sync_data_t *data),
	void (*update)(struct msm_timer_sync_data_t *, uint32_t, uint32_t),
	struct msm_timer_sync_data_t *data)
{
	uint32_t *smem_clock;
	uint32_t smem_clock_val;
	uint32_t state;

	smem_clock = smem_alloc(SMEM_SMEM_SLOW_CLOCK_VALUE, sizeof(uint32_t));
	if (smem_clock == NULL) {
		printk(KERN_ERR "no smem clock\n");
		return 0;
	}

	state = smsm_get_state(SMSM_MODEM_STATE);
	if ((state & SMSM_INIT) == 0) {
		printk(KERN_ERR "smsm not initialized\n");
		return 0;
	}

	time_start(data);
	while ((state = smsm_get_state(SMSM_TIME_MASTER_DEM)) &
		MASTER_TIME_PENDING) {
		if (time_expired(data)) {
			printk(KERN_EMERG "get_smem_clock: timeout 1 still "
				"invalid state %x\n", state);
			msm_timer_sync_timeout();
		}
	}

	smsm_change_state(SMSM_APPS_DEM, SLAVE_TIME_POLL | SLAVE_TIME_INIT,
		SLAVE_TIME_REQUEST);

	time_start(data);
	while (!((state = smsm_get_state(SMSM_TIME_MASTER_DEM)) &
		MASTER_TIME_PENDING)) {
		if (time_expired(data)) {
			printk(KERN_EMERG "get_smem_clock: timeout 2 still "
				"invalid state %x\n", state);
			msm_timer_sync_timeout();
		}
	}

	smsm_change_state(SMSM_APPS_DEM, SLAVE_TIME_REQUEST, SLAVE_TIME_POLL);

	time_start(data);
	do {
		smem_clock_val = *smem_clock;
	} while (smem_clock_val == 0 && !time_expired(data));

	state = smsm_get_state(SMSM_TIME_MASTER_DEM);

	if (smem_clock_val) {
		if (update != NULL)
			update(data, smem_clock_val, SCLK_HZ);

		if (msm_timer_debug_mask & MSM_TIMER_DEBUG_SYNC)
			printk(KERN_INFO
				"get_smem_clock: state %x clock %u\n",
				state, smem_clock_val);
	} else {
		printk(KERN_EMERG
			"get_smem_clock: timeout state %x clock %u\n",
			state, smem_clock_val);
		msm_timer_sync_timeout();
	}

	smsm_change_state(SMSM_APPS_DEM, SLAVE_TIME_REQUEST | SLAVE_TIME_POLL,
		SLAVE_TIME_INIT);
	return smem_clock_val;
}
#else /* CONFIG_MSM_N_WAY_SMSM */
static uint32_t msm_timer_do_sync_to_sclk(
	void (*time_start)(struct msm_timer_sync_data_t *data),
	bool (*time_expired)(struct msm_timer_sync_data_t *data),
	void (*update)(struct msm_timer_sync_data_t *, uint32_t, uint32_t),
	struct msm_timer_sync_data_t *data)
{
	uint32_t *smem_clock;
	uint32_t smem_clock_val;
	uint32_t last_state;
	uint32_t state;

	smem_clock = smem_alloc(SMEM_SMEM_SLOW_CLOCK_VALUE,
				sizeof(uint32_t));

	if (smem_clock == NULL) {
		printk(KERN_ERR "no smem clock\n");
		return 0;
	}

	last_state = state = smsm_get_state(SMSM_MODEM_STATE);
	smem_clock_val = *smem_clock;
	if (smem_clock_val) {
		printk(KERN_INFO "get_smem_clock: invalid start state %x "
			"clock %u\n", state, smem_clock_val);
		smsm_change_state(SMSM_APPS_STATE,
				  SMSM_TIMEWAIT, SMSM_TIMEINIT);

		time_start(data);
		while (*smem_clock != 0 && !time_expired(data))
			;

		smem_clock_val = *smem_clock;
		if (smem_clock_val) {
			printk(KERN_EMERG "get_smem_clock: timeout still "
				"invalid state %x clock %u\n",
				state, smem_clock_val);
			msm_timer_sync_timeout();
		}
	}

	time_start(data);
	smsm_change_state(SMSM_APPS_STATE, SMSM_TIMEINIT, SMSM_TIMEWAIT);
	do {
		smem_clock_val = *smem_clock;
		state = smsm_get_state(SMSM_MODEM_STATE);
		if (state != last_state) {
			last_state = state;
			if (msm_timer_debug_mask & MSM_TIMER_DEBUG_SYNC)
				printk(KERN_INFO
					"get_smem_clock: state %x clock %u\n",
					state, smem_clock_val);
		}
	} while (smem_clock_val == 0 && !time_expired(data));

	if (smem_clock_val) {
		if (update != NULL)
			update(data, smem_clock_val, SCLK_HZ);
	} else {
		printk(KERN_EMERG
			"get_smem_clock: timeout state %x clock %u\n",
			state, smem_clock_val);
		msm_timer_sync_timeout();
	}

	smsm_change_state(SMSM_APPS_STATE, SMSM_TIMEWAIT, SMSM_TIMEINIT);
	return smem_clock_val;
}
#endif /* CONFIG_MSM_N_WAY_SMSM */

/*
 * Callback function that initializes the timeout value.
 */
static void msm_timer_sync_to_sclk_time_start(
	struct msm_timer_sync_data_t *data)
{
	/* approx 2 seconds */
	uint32_t delta = data->clock->freq << data->clock->shift << 1;
	data->timeout = msm_read_timer_count(data->clock, LOCAL_TIMER) + delta;
}

/*
 * Callback function that checks the timeout.
 */
static bool msm_timer_sync_to_sclk_time_expired(
	struct msm_timer_sync_data_t *data)
{
	uint32_t delta = msm_read_timer_count(data->clock, LOCAL_TIMER) -
		data->timeout;
	return ((int32_t) delta) > 0;
}

/*
 * Callback function that updates local clock from the specified source clock
 * value and frequency.
 */
static void msm_timer_sync_update(struct msm_timer_sync_data_t *data,
	uint32_t src_clk_val, uint32_t src_clk_freq)
{
	struct msm_clock *dst_clk = data->clock;
	struct msm_clock_percpu_data *dst_clk_state =
		&__get_cpu_var(msm_clocks_percpu)[dst_clk->index];
	uint32_t dst_clk_val = msm_read_timer_count(dst_clk, LOCAL_TIMER);
	uint32_t new_offset;

	if ((dst_clk->freq << dst_clk->shift) == src_clk_freq) {
		new_offset = src_clk_val - dst_clk_val;
	} else {
		uint64_t temp;

		/* separate multiplication and division steps to reduce
		   rounding error */
		temp = src_clk_val;
		temp *= dst_clk->freq << dst_clk->shift;
		do_div(temp, src_clk_freq);

		new_offset = (uint32_t)(temp) - dst_clk_val;
	}

	if (dst_clk_state->sleep_offset + dst_clk_state->non_sleep_offset !=
	    new_offset) {
		if (data->exit_sleep)
			dst_clk_state->sleep_offset =
				new_offset - dst_clk_state->non_sleep_offset;
		else
			dst_clk_state->non_sleep_offset =
				new_offset - dst_clk_state->sleep_offset;

		if (msm_timer_debug_mask & MSM_TIMER_DEBUG_SYNC)
			printk(KERN_INFO "sync clock %s: "
				"src %u, new offset %u + %u\n",
				dst_clk->clocksource.name, src_clk_val,
				dst_clk_state->sleep_offset,
				dst_clk_state->non_sleep_offset);
	}
}

/*
 * Synchronize GPT clock with sclk.
 */
static void msm_timer_sync_gpt_to_sclk(int exit_sleep)
{
	struct msm_clock *gpt_clk = &msm_clocks[MSM_CLOCK_GPT];
	struct msm_clock_percpu_data *gpt_clk_state =
		&__get_cpu_var(msm_clocks_percpu)[MSM_CLOCK_GPT];
	struct msm_timer_sync_data_t data;
	uint32_t ret;

	if (gpt_clk_state->in_sync)
		return;

	data.clock = gpt_clk;
	data.timeout = 0;
	data.exit_sleep = exit_sleep;

	ret = msm_timer_do_sync_to_sclk(
		msm_timer_sync_to_sclk_time_start,
		msm_timer_sync_to_sclk_time_expired,
		msm_timer_sync_update,
		&data);

	if (ret)
		gpt_clk_state->in_sync = 1;
}

/*
 * Synchronize clock with GPT clock.
 */
static void msm_timer_sync_to_gpt(struct msm_clock *clock, int exit_sleep)
{
	struct msm_clock *gpt_clk = &msm_clocks[MSM_CLOCK_GPT];
	struct msm_clock_percpu_data *gpt_clk_state =
		&__get_cpu_var(msm_clocks_percpu)[MSM_CLOCK_GPT];
	struct msm_clock_percpu_data *clock_state =
		&__get_cpu_var(msm_clocks_percpu)[clock->index];
	struct msm_timer_sync_data_t data;
	uint32_t gpt_clk_val;
	u64 gpt_period = (1ULL << 32) * HZ / GPT_HZ;
	u64 now = get_jiffies_64();

	BUG_ON(clock == gpt_clk);

	if (clock_state->in_sync &&
		(now - clock_state->last_sync_jiffies < (gpt_period >> 1)))
		return;

	gpt_clk_val = msm_read_timer_count(gpt_clk, LOCAL_TIMER)
		+ gpt_clk_state->sleep_offset + gpt_clk_state->non_sleep_offset;

	if (exit_sleep && gpt_clk_val < clock_state->last_sync_gpt)
		clock_state->non_sleep_offset -= clock->rollover_offset;

	data.clock = clock;
	data.timeout = 0;
	data.exit_sleep = exit_sleep;

	msm_timer_sync_update(&data, gpt_clk_val, GPT_HZ);

	clock_state->in_sync = 1;
	clock_state->last_sync_gpt = gpt_clk_val;
	clock_state->last_sync_jiffies = now;
}

static void msm_timer_reactivate_alarm(struct msm_clock *clock)
{
	struct msm_clock_percpu_data *clock_state =
		&__get_cpu_var(msm_clocks_percpu)[clock->index];
	long alarm_delta = clock_state->alarm_vtime -
		clock_state->sleep_offset -
		msm_read_timer_count(clock, LOCAL_TIMER);
	alarm_delta >>= clock->shift;
	if (alarm_delta < (long)clock->write_delay + 4)
		alarm_delta = clock->write_delay + 4;
	while (msm_timer_set_next_event(alarm_delta, &clock->clockevent))
		;
}

int64_t msm_timer_enter_idle(void)
{
	struct msm_clock *gpt_clk = &msm_clocks[MSM_CLOCK_GPT];
	struct msm_clock *clock = __get_cpu_var(msm_active_clock);
	struct msm_clock_percpu_data *clock_state =
		&__get_cpu_var(msm_clocks_percpu)[clock->index];
	uint32_t alarm;
	uint32_t count;
	int32_t delta;

	BUG_ON(clock != &msm_clocks[MSM_CLOCK_GPT] &&
		clock != &msm_clocks[MSM_CLOCK_DGT]);

	msm_timer_sync_gpt_to_sclk(0);
	if (clock != gpt_clk)
		msm_timer_sync_to_gpt(clock, 0);

	count = msm_read_timer_count(clock, LOCAL_TIMER);
	if (clock_state->stopped++ == 0)
		clock_state->stopped_tick = count + clock_state->sleep_offset;
	alarm = clock_state->alarm;
	delta = alarm - count;
	if (delta <= -(int32_t)((clock->freq << clock->shift) >> 10)) {
		/* timer should have triggered 1ms ago */
		printk(KERN_ERR "msm_timer_enter_idle: timer late %d, "
			"reprogram it\n", delta);
		msm_timer_reactivate_alarm(clock);
	}
	if (delta <= 0)
		return 0;
	return clocksource_cyc2ns((alarm - count) >> clock->shift,
		      clock->clocksource.mult,
		      clock->clocksource.shift);
}

void msm_timer_exit_idle(int low_power)
{
	struct msm_clock *gpt_clk = &msm_clocks[MSM_CLOCK_GPT];
	struct msm_clock *clock = __get_cpu_var(msm_active_clock);
	struct msm_clock_percpu_data *gpt_clk_state =
		&__get_cpu_var(msm_clocks_percpu)[MSM_CLOCK_GPT];
	struct msm_clock_percpu_data *clock_state =
		&__get_cpu_var(msm_clocks_percpu)[clock->index];
	uint32_t enabled;

	BUG_ON(clock != &msm_clocks[MSM_CLOCK_GPT] &&
		clock != &msm_clocks[MSM_CLOCK_DGT]);

	if (!low_power)
		goto exit_idle_exit;

	enabled = readl(gpt_clk->regbase + TIMER_ENABLE) & TIMER_ENABLE_EN;
	if (!enabled)
		writel(TIMER_ENABLE_EN, gpt_clk->regbase + TIMER_ENABLE);

#if defined(CONFIG_ARCH_MSM_SCORPION)
	gpt_clk_state->in_sync = 0;
#else
	gpt_clk_state->in_sync = gpt_clk_state->in_sync && enabled;
#endif
	msm_timer_sync_gpt_to_sclk(1);

	if (clock == gpt_clk)
		goto exit_idle_alarm;

	enabled = readl(clock->regbase + TIMER_ENABLE) & TIMER_ENABLE_EN;
	if (!enabled)
		writel(TIMER_ENABLE_EN, clock->regbase + TIMER_ENABLE);

#if defined(CONFIG_ARCH_MSM_SCORPION)
	clock_state->in_sync = 0;
#else
	clock_state->in_sync = clock_state->in_sync && enabled;
#endif
	msm_timer_sync_to_gpt(clock, 1);

exit_idle_alarm:
	msm_timer_reactivate_alarm(clock);

exit_idle_exit:
	clock_state->stopped--;
}

/*
 * Callback function that initializes the timeout value.
 */
static void msm_timer_get_sclk_time_start(
	struct msm_timer_sync_data_t *data)
{
	data->timeout = 200000;
}

/*
 * Callback function that checks the timeout.
 */
static bool msm_timer_get_sclk_time_expired(
	struct msm_timer_sync_data_t *data)
{
	udelay(10);
	return --data->timeout <= 0;
}

/*
 * Retrieve the cycle count from the sclk and convert it into
 * nanoseconds.
 *
 * On exit, if period is not NULL, it contains the period of the
 * sclk in nanoseconds, i.e. how long the cycle count wraps around.
 *
 * Return value:
 *      0: the operation failed; period is not set either
 *      >0: time in nanoseconds
 */
int64_t msm_timer_get_sclk_time(int64_t *period)
{
	struct msm_timer_sync_data_t data;
	uint32_t clock_value;
	int64_t tmp;

	memset(&data, 0, sizeof(data));
	clock_value = msm_timer_do_sync_to_sclk(
		msm_timer_get_sclk_time_start,
		msm_timer_get_sclk_time_expired,
		NULL,
		&data);

	if (!clock_value)
		return 0;

	if (period) {
		tmp = 1LL << 32;
		tmp = tmp * NSEC_PER_SEC / SCLK_HZ;
		*period = tmp;
	}

	tmp = (int64_t)clock_value;
	tmp = tmp * NSEC_PER_SEC / SCLK_HZ;
	return tmp;
}

int __init msm_timer_init_time_sync(void (*timeout)(void))
{
#if defined(CONFIG_MSM_N_WAY_SMSM)
	int ret = smsm_change_intr_mask(SMSM_TIME_MASTER_DEM, 0xFFFFFFFF, 0);

	if (ret) {
		printk(KERN_ERR	"%s: failed to clear interrupt mask, %d\n",
			__func__, ret);
		return ret;
	}

	smsm_change_state(SMSM_APPS_DEM,
		SLAVE_TIME_REQUEST | SLAVE_TIME_POLL, SLAVE_TIME_INIT);
#endif

	BUG_ON(timeout == NULL);
	msm_timer_sync_timeout = timeout;

	return 0;
}


unsigned long long sched_clock(void)
{
	static cycle_t last_ticks;
	static DEFINE_SPINLOCK(msm_timer_sched_clock_lock);

	struct msm_clock *clock;
	struct clocksource *cs;
	cycle_t ticks, delta;
	unsigned long irq_flags;

	clock = &msm_clocks[MSM_GLOBAL_TIMER];
	cs = &clock->clocksource;

	ticks  = cs->read(cs);

	spin_lock_irqsave(&msm_timer_sched_clock_lock, irq_flags);
	delta = (ticks - last_ticks) & cs->mask;

	if (delta < cs->mask/2)
		last_ticks += delta;

	ticks = last_ticks;
	spin_unlock_irqrestore(&msm_timer_sched_clock_lock, irq_flags);

	return clocksource_cyc2ns(ticks, cs->mult, cs->shift);
}

#ifdef CONFIG_ARCH_MSM_SCORPIONMP
int read_current_timer(unsigned long *timer_val)
{
	struct msm_clock *dgt = &msm_clocks[MSM_CLOCK_DGT];
	*timer_val = msm_read_timer_count(dgt, GLOBAL_TIMER);
	return 0;
}
#endif

static void __init msm_timer_init(void)
{
	int i;
	int res;

#ifdef CONFIG_ARCH_MSM8X60
	writel(DGT_CLK_CTL_DIV_4, MSM_TMR_BASE + DGT_CLK_CTL);
#endif

	for (i = 0; i < ARRAY_SIZE(msm_clocks); i++) {
		struct msm_clock *clock = &msm_clocks[i];
		struct clock_event_device *ce = &clock->clockevent;
		struct clocksource *cs = &clock->clocksource;
		writel(0, clock->regbase + TIMER_ENABLE);
		writel(1, clock->regbase + TIMER_CLEAR);
		writel(0, clock->regbase + TIMER_COUNT_VAL);
		writel(~0, clock->regbase + TIMER_MATCH_VAL);

		if ((clock->freq << clock->shift) == GPT_HZ) {
			clock->rollover_offset = 0;
		} else {
			uint64_t temp;

			temp = clock->freq << clock->shift;
			temp <<= 32;
			temp /= GPT_HZ;

			clock->rollover_offset = (uint32_t) temp;
		}

		ce->mult = div_sc(clock->freq, NSEC_PER_SEC, ce->shift);
		/* allow at least 10 seconds to notice that the timer wrapped */
		ce->max_delta_ns =
			clockevent_delta2ns(0xf0000000 >> clock->shift, ce);
		/* ticks gets rounded down by one */
		ce->min_delta_ns =
			clockevent_delta2ns(clock->write_delay + 4, ce);
		ce->cpumask = cpumask_of(0);

		cs->mult = clocksource_hz2mult(clock->freq, cs->shift);
		res = clocksource_register(cs);
		if (res)
			printk(KERN_ERR "msm_timer_init: clocksource_register "
			       "failed for %s\n", cs->name);

		res = setup_irq(clock->irq.irq, &clock->irq);
		if (res)
			printk(KERN_ERR "msm_timer_init: setup_irq "
			       "failed for %s\n", cs->name);

		clockevents_register_device(ce);
	}
#ifdef CONFIG_ARCH_MSM_SCORPIONMP
	writel(1, msm_clocks[MSM_CLOCK_DGT].regbase + TIMER_ENABLE);
	set_delay_fn(read_current_timer_delay_loop);
#endif
}

#ifdef CONFIG_SMP
void local_timer_setup(struct clock_event_device *evt)
{
	unsigned long flags;
	struct msm_clock *clock = &msm_clocks[MSM_GLOBAL_TIMER];

#ifdef CONFIG_ARCH_MSM8X60
	writel(DGT_CLK_CTL_DIV_4, MSM_TMR_BASE + DGT_CLK_CTL);
#endif

	if (!local_clock_event) {
		writel(0, clock->regbase  + TIMER_ENABLE);
		writel(1, clock->regbase + TIMER_CLEAR);
		writel(0, clock->regbase + TIMER_COUNT_VAL);
		writel(~0, clock->regbase + TIMER_MATCH_VAL);
		__get_cpu_var(msm_clocks_percpu)[clock->index].alarm = ~0;
	}
	evt->irq = clock->irq.irq;
	evt->name = "local_timer";
	evt->features = CLOCK_EVT_FEAT_ONESHOT;
	evt->rating = clock->clockevent.rating;
	evt->set_mode = msm_timer_set_mode;
	evt->set_next_event = msm_timer_set_next_event;
	evt->shift = clock->clockevent.shift;
	evt->mult = div_sc(clock->freq, NSEC_PER_SEC, evt->shift);
	evt->max_delta_ns =
		clockevent_delta2ns(0xf0000000 >> clock->shift, evt);
	evt->min_delta_ns = clockevent_delta2ns(clock->write_delay + 4, evt);
	evt->cpumask = cpumask_of(smp_processor_id());

	local_clock_event = evt;

	local_irq_save(flags);
	gic_clear_spi_pending(clock->irq.irq);
	get_irq_chip(clock->irq.irq)->unmask(clock->irq.irq);
	local_irq_restore(flags);

	clockevents_register_device(evt);
}

int local_timer_ack(void)
{
	return 1;
}
#endif

#ifdef CONFIG_HOTPLUG_CPU
void __cpuexit local_timer_stop(void)
{
	local_clock_event->set_mode(CLOCK_EVT_MODE_SHUTDOWN, local_clock_event);
	get_irq_chip(local_clock_event->irq)->mask(local_clock_event->irq);
	local_clock_event = NULL;
}
#endif

struct sys_timer msm_timer = {
	.init = msm_timer_init
};
