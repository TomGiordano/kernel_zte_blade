/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Code Aurora Forum nor
 *       the names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * Alternatively, provided that this notice is retained in full, this software
 * may be relicensed by the recipient under the terms of the GNU General Public
 * License version 2 ("GPL") and only version 2, in which case the provisions of
 * the GPL apply INSTEAD OF those given above.  If the recipient relicenses the
 * software under the GPL, then the identification text in the MODULE_LICENSE
 * macro must be changed to reflect "GPLv2" instead of "Dual BSD/GPL".  Once a
 * recipient changes the license terms to the GPL, subsequent recipients shall
 * not relicense under alternate licensing terms, including the BSD or dual
 * BSD/GPL terms.  In addition, the following license statement immediately
 * below and between the words START and END shall also then apply when this
 * software is relicensed under the GPL:
 *
 * START
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License version 2 and only version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * END
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */
#define ZTE_CALCULATE_CAPACITY_PERCENTAGE_ARM9

#ifndef ZTE_CALCULATE_CAPACITY_PERCENTAGE_ARM9
#include <linux/earlysuspend.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

#include <asm/atomic.h>

#include "../../arch/arm/mach-msm/smd_private.h"

#include <mach/msm_battery.h>


#define MSM_BATTERY_DEBUGX
#ifdef MSM_BATTERY_DEBUG
#define DEBUG_MSM_BATTERY(fmt, args...)\
    do\
    {\
        printk("%s:%s:%d:", __FILE__, __FUNCTION__, __LINE__);\
        printk(fmt "\n", ## args);\
    }\
    while (0)
#else
#define DEBUG_MSM_BATTERY(fmt, args...) do{}while(0)
#endif

#define FALSE (0)
#define TRUE  (1)

#define BATTERY_LOW            	    2800
#define BATTERY_HIGH           	    4300

#define BATTERY_STATUS_POLL_TIME    (2*HZ)
#define BATTERY_CAPACITY_POLL_TIME  (10*HZ)

#define SUSPEND_EVENT		(1UL << 0)
#define RESUME_EVENT		(1UL << 1)
#define CLEANUP_EVENT		(1UL << 2)
#define CAPACITY_EVENT      (1UL << 3)

#define EVENT_CONDITION (SUSPEND_EVENT | RESUME_EVENT | CLEANUP_EVENT | CAPACITY_EVENT)

#define DEBUG  0

#if DEBUG
#define DBG(x...) printk(KERN_INFO x)
#else
#define DBG(x...) do {} while (0)
#endif


/*
 * This enum contains defintions of the charger hardware status
 */
enum chg_charger_status_type
{
    /* The charger is good      */
    CHARGER_STATUS_GOOD,
    /* The charger is bad       */
    CHARGER_STATUS_BAD,
    /* The charger is weak      */
    CHARGER_STATUS_WEAK,
    /* Invalid charger status.  */
    CHARGER_STATUS_INVALID
};

#ifdef MSM_BATTERY_DEBUGX
static char *charger_status[] = {
    "good", "bad", "weak", "invalid"
};
#endif

/*
 *This enum contains defintions of the charger hardware type
 */
enum chg_charger_hardware_type
{
    /* The charger is removed                 */
    CHARGER_TYPE_NONE,
    /* The charger is a regular wall charger   */
    CHARGER_TYPE_WALL,
    /* The charger is a PC USB                 */
    CHARGER_TYPE_USB_PC,
    /* The charger is a wall USB charger       */
    CHARGER_TYPE_USB_WALL,
    /* The charger is a USB carkit             */
    CHARGER_TYPE_USB_CARKIT,
    /* Invalid charger hardware status.        */
    CHARGER_TYPE_INVALID
};

#ifdef MSM_BATTERY_DEBUGX
static char *charger_type[] = {
    "No charger", "wall", "USB PC", "USB wall", "USB car kit",
    "invalid charger"
};
#endif

/*
 *  This enum contains defintions of the battery status
 */
enum chg_battery_status_type
{
    /* The battery is good        */
    BATTERY_STATUS_GOOD,
    /* The battery is cold/hot    */
    BATTERY_STATUS_BAD_TEMP,
    /* The battery is bad         */
    BATTERY_STATUS_BAD,
    /* Invalid battery status.    */
    BATTERY_STATUS_INVALID
};

#ifdef MSM_BATTERY_DEBUGX
static char *battery_status[] = {
    "good ", "bad temperature", "bad", "invalid"
};
#endif

/*
 *This enum contains defintions of the battery voltage level
 */
enum chg_battery_level_type
{
    /* The battery voltage is dead/very low (less than 3.2V)        */
    BATTERY_LEVEL_DEAD,
    /* The battery voltage is weak/low (between 3.2V and 3.4V)      */
    BATTERY_LEVEL_WEAK,
    /* The battery voltage is good/normal(between 3.4V and 4.2V)  */
    BATTERY_LEVEL_GOOD,
    /* The battery voltage is up to full (close to 4.2V)            */
    BATTERY_LEVEL_FULL,
    /* Invalid battery voltage level.                               */
    BATTERY_LEVEL_INVALID
};
#ifdef MSM_BATTERY_DEBUGX
static char *battery_level[] = {
    "dead", "weak", "good", "full", "invalid"
};
#endif

struct __attribute__((packed)) smem_batt_chg_t
{
    u8 charger_type;
    u8 charger_status;
    u8 charging;
    u8 chg_fulled;

    u8 battery_status;
    u8 battery_level;
    u16 battery_voltage;
    s16 battery_temp;

    u8 low_batt_alarm_triggered;
};
static struct smem_batt_chg_t rep_batt_chg;

struct msm_battery_info
{
    u32 voltage_max_design;
    u32 voltage_min_design;
    u32 chg_api_version;
    u32 batt_technology;

    u32 avail_chg_sources;
    u32 current_chg_source;

    u32 batt_status;
    u32 batt_health;
    u32 charger_valid;
    u32 batt_valid;
    u32 batt_capacity;

    u8 charger_status;
    u8 charger_type;
    u8 charging;
    u8 chg_fulled;

    u8 battery_status;
    u8 battery_level;
    u16 battery_voltage;
    s16 battery_temp;

    u8 low_batt_alarm_triggered;

    u32(*calculate_capacity) (u32 voltage);
    spinlock_t lock;

    struct power_supply *msm_psy_ac;
    struct power_supply *msm_psy_usb;
    struct power_supply *msm_psy_batt;

    struct workqueue_struct *msm_batt_wq;

    wait_queue_head_t wait_q;

    struct early_suspend early_suspend;

    struct timer_list *battery_timer;

    bool is_resume;
    bool is_poweron;

    atomic_t handle_event;

    u32 type_of_event;
};

static void msm_batt_wait_for_batt_chg_event(struct work_struct *work);

static DECLARE_WORK(msm_batt_cb_work, msm_batt_wait_for_batt_chg_event);

static int msm_batt_cleanup(void);

static struct msm_battery_info msm_batt_info = {
    .charger_status = -1,
    .charger_type = -1,
    .battery_status = -1,
    .battery_level = -1,
    .battery_voltage = -1,
    .battery_temp = -1,
};

static enum power_supply_property msm_power_props[] =
{
    POWER_SUPPLY_PROP_ONLINE,
};

static char *msm_power_supplied_to[] = {
    "battery",
};

static int msm_power_get_property(struct power_supply *psy,
                                  enum power_supply_property psp, union power_supply_propval *val)
{
    switch (psp)
    {
    case POWER_SUPPLY_PROP_ONLINE:

        if (psy->type == POWER_SUPPLY_TYPE_MAINS)
        {

            val->intval = msm_batt_info.current_chg_source & AC_CHG ? 1 : 0;
            DEBUG_MSM_BATTERY(KERN_INFO "%s(): power supply = %s online = %d\n", __func__, psy->name, val->intval);

        }

        if (psy->type == POWER_SUPPLY_TYPE_USB)
        {

            val->intval = msm_batt_info.current_chg_source & USB_CHG ? 1 : 0;

            DEBUG_MSM_BATTERY(KERN_INFO "%s(): power supply = %s online = %d\n", __func__, psy->name, val->intval);
        }

        break;
    default:
        return -EINVAL;
    }
    return 0;
}

static struct power_supply msm_psy_ac = {
    .name = "ac",
    .type = POWER_SUPPLY_TYPE_MAINS,
    .supplied_to = msm_power_supplied_to,
    .num_supplicants = ARRAY_SIZE(msm_power_supplied_to),
    .properties = msm_power_props,
    .num_properties = ARRAY_SIZE(msm_power_props),
    .get_property = msm_power_get_property,
};

static struct power_supply msm_psy_usb = {
    .name = "usb",
    .type = POWER_SUPPLY_TYPE_USB,
    .supplied_to = msm_power_supplied_to,
    .num_supplicants = ARRAY_SIZE(msm_power_supplied_to),
    .properties = msm_power_props,
    .num_properties = ARRAY_SIZE(msm_power_props),
    .get_property = msm_power_get_property,
};

static enum power_supply_property msm_batt_power_props[] =
{
    POWER_SUPPLY_PROP_STATUS,
    POWER_SUPPLY_PROP_HEALTH,
    POWER_SUPPLY_PROP_PRESENT,
    POWER_SUPPLY_PROP_TECHNOLOGY,
    POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
    POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
    POWER_SUPPLY_PROP_VOLTAGE_NOW,
    POWER_SUPPLY_PROP_CAPACITY,
    POWER_SUPPLY_PROP_TEMP,
};

static void msm_batt_external_power_changed(struct power_supply *psy)
{
    //DEBUG_MSM_BATTERY(KERN_INFO "%s() : external power supply changed for %s\n", __func__, psy->name);
    power_supply_changed(psy);
}

static int msm_batt_power_get_property(struct power_supply *psy,
                                       enum power_supply_property psp, union power_supply_propval *val)
{
    switch (psp)
    {
    case POWER_SUPPLY_PROP_STATUS:
        val->intval = msm_batt_info.batt_status;
        break;
    case POWER_SUPPLY_PROP_HEALTH:
        val->intval = msm_batt_info.batt_health;
        break;
    case POWER_SUPPLY_PROP_PRESENT:
        val->intval = msm_batt_info.batt_valid;
        break;
    case POWER_SUPPLY_PROP_TECHNOLOGY:
        val->intval = msm_batt_info.batt_technology;
        break;
    case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
        val->intval = msm_batt_info.voltage_max_design*1000;
        break;
    case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
        val->intval = msm_batt_info.voltage_min_design*1000;
        break;
    case POWER_SUPPLY_PROP_VOLTAGE_NOW:
        val->intval = msm_batt_info.battery_voltage*1000;
        break;
    case POWER_SUPPLY_PROP_CAPACITY:
        val->intval = msm_batt_info.batt_capacity;
        break;
    case POWER_SUPPLY_PROP_TEMP:
        val->intval = msm_batt_info.battery_temp;
        break;
    default:
        return -EINVAL;
    }
    return 0;
}

static struct power_supply msm_psy_batt = {
    .name = "battery",
    .type = POWER_SUPPLY_TYPE_BATTERY,
    .properties = msm_batt_power_props,
    .num_properties = ARRAY_SIZE(msm_batt_power_props),
    .get_property = msm_batt_power_get_property,
    .external_power_changed = msm_batt_external_power_changed,
};

static int msm_batt_get_batt_chg_status_v1(void)
{
    struct smem_batt_chg_t *batt_chg_ptr;

    memset(&rep_batt_chg, 0, sizeof(rep_batt_chg));

    batt_chg_ptr = smem_alloc(SMEM_BATT_INFO, sizeof(struct smem_batt_chg_t));
    if (batt_chg_ptr == NULL)
    {
        printk("%s: share memery read error!\n", __func__);
        return -EIO;
    }
    else
    {
        rep_batt_chg = *batt_chg_ptr;
        DEBUG_MSM_BATTERY("status read ok!base: %d size: %d\n", SMEM_BATT_INFO, sizeof(struct smem_batt_chg_t));

        DEBUG_MSM_BATTERY(KERN_INFO "charger_status = %s, charger_type = %s,"
               " batt_status = %s, batt_level = %s,"
               " batt_volt = %u, batt_temp = %u, chg_fulled = %d\n",
               charger_status[rep_batt_chg.charger_status],
               charger_type[rep_batt_chg.charger_type],
               battery_status[rep_batt_chg.battery_status],
               battery_level[rep_batt_chg.battery_level], 
               rep_batt_chg.battery_voltage, 
               rep_batt_chg.battery_temp, 
               rep_batt_chg.chg_fulled);

        if (rep_batt_chg.battery_voltage > msm_batt_info.voltage_max_design)
        {
            printk("battery voltage overvoltage: %d\n", rep_batt_chg.battery_voltage);
            rep_batt_chg.battery_voltage = msm_batt_info.voltage_max_design;
        }
        else if (rep_batt_chg.battery_voltage < msm_batt_info.voltage_min_design)
        {
            printk("battery voltage low: %d\n", rep_batt_chg.battery_voltage);
            rep_batt_chg.battery_voltage = msm_batt_info.voltage_min_design;
        }
    }

    return 0;
}
void msm_batt_update_psy_status_v1(void)
{
    msm_batt_get_batt_chg_status_v1();

    DEBUG_MSM_BATTERY();

    if (msm_batt_info.charger_status == rep_batt_chg.charger_status &&
        msm_batt_info.charger_type == rep_batt_chg.charger_type &&
        msm_batt_info.battery_status == rep_batt_chg.battery_status &&
        msm_batt_info.battery_level == rep_batt_chg.battery_level &&
        msm_batt_info.battery_voltage == rep_batt_chg.battery_voltage &&
        msm_batt_info.battery_temp == rep_batt_chg.battery_temp && 
        
        msm_batt_info.chg_fulled == rep_batt_chg.chg_fulled &&  
        msm_batt_info.charging == rep_batt_chg.charging)
    {
        DEBUG_MSM_BATTERY(KERN_NOTICE "%s() : Got unnecessary event from Modem "
               "PMIC VBATT driver. Nothing changed in Battery or " "charger status\n", __func__);
        return;
    }

    if (rep_batt_chg.battery_status != BATTERY_STATUS_INVALID)
    {
        msm_batt_info.batt_valid = 1;

        if (rep_batt_chg.battery_voltage > msm_batt_info.voltage_max_design)
        {
            msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
            msm_batt_info.batt_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
        }
        else if (rep_batt_chg.battery_voltage < msm_batt_info.voltage_min_design)
        {
            msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_DEAD;
            msm_batt_info.batt_status = POWER_SUPPLY_STATUS_UNKNOWN;
        }
        else if (rep_batt_chg.battery_status == BATTERY_STATUS_BAD)
        {
            printk("battery status bad\n");
            msm_batt_info.batt_capacity = 
                msm_batt_info.calculate_capacity(msm_batt_info.battery_voltage);
            msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_DEAD;
            msm_batt_info.batt_status = POWER_SUPPLY_STATUS_UNKNOWN;
        }
        else if (rep_batt_chg.battery_status == BATTERY_STATUS_BAD_TEMP)
        {
            printk("battery status bad temp\n");
            msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_OVERHEAT;

            if (rep_batt_chg.charger_status == CHARGER_STATUS_BAD
                || rep_batt_chg.charger_status == CHARGER_STATUS_INVALID)
                msm_batt_info.batt_status = POWER_SUPPLY_STATUS_UNKNOWN;
            else
                msm_batt_info.batt_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
        }
        else if ((rep_batt_chg.charger_status == CHARGER_STATUS_GOOD
                  || rep_batt_chg.charger_status == CHARGER_STATUS_WEAK)
                 && (rep_batt_chg.battery_status == BATTERY_STATUS_GOOD))
        {
            msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_GOOD;

            if (rep_batt_chg.chg_fulled)
            {
                msm_batt_info.batt_capacity = 100;
                msm_batt_info.batt_status = POWER_SUPPLY_STATUS_FULL;
            }
            else
                msm_batt_info.batt_status = POWER_SUPPLY_STATUS_CHARGING;
        }
        else if ((rep_batt_chg.charger_status == CHARGER_STATUS_BAD
                  || rep_batt_chg.charger_status == CHARGER_STATUS_INVALID) 
                  && (rep_batt_chg.battery_status == BATTERY_STATUS_GOOD))
        {
            msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_GOOD;
            msm_batt_info.batt_status = POWER_SUPPLY_STATUS_DISCHARGING;
        }

        //msm_batt_info.batt_capacity = msm_batt_info.calculate_capacity(msm_batt_info.battery_voltage);
    }
    else
    {
        printk("battery status invalid id\n");
        msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_UNKNOWN;
        msm_batt_info.batt_status = POWER_SUPPLY_STATUS_UNKNOWN;
        msm_batt_info.batt_capacity = 
            msm_batt_info.calculate_capacity(msm_batt_info.battery_voltage);
        msm_batt_info.batt_valid = 0;        
    }

    if (msm_batt_info.charger_type != rep_batt_chg.charger_type)
    {
        msm_batt_info.charger_type = rep_batt_chg.charger_type;

        if (msm_batt_info.charger_type == CHARGER_TYPE_WALL)
        {
            msm_batt_info.current_chg_source &= ~USB_CHG;
            msm_batt_info.current_chg_source |= AC_CHG;

            DEBUG_MSM_BATTERY(KERN_INFO "%s() : charger_type = WALL\n", __func__);

            power_supply_changed(&msm_psy_ac);

        }
        else if (msm_batt_info.charger_type ==
                 CHARGER_TYPE_USB_WALL || msm_batt_info.charger_type == CHARGER_TYPE_USB_PC)
        {
            msm_batt_info.current_chg_source &= ~AC_CHG;
            msm_batt_info.current_chg_source |= USB_CHG;

            DEBUG_MSM_BATTERY(KERN_INFO "%s() : charger_type = %s\n", __func__, charger_type[msm_batt_info.charger_type]);

            power_supply_changed(&msm_psy_usb);
            DEBUG_MSM_BATTERY();
        }
        else
        {
            DEBUG_MSM_BATTERY(KERN_INFO "%s() : charger_type = %s\n", __func__, charger_type[msm_batt_info.charger_type]);

            msm_batt_info.batt_status = POWER_SUPPLY_STATUS_DISCHARGING;

            if (msm_batt_info.current_chg_source & AC_CHG)
            {
                msm_batt_info.current_chg_source &= ~AC_CHG;

                DEBUG_MSM_BATTERY(KERN_INFO "%s() : AC WALL charger" " removed\n", __func__);

                power_supply_changed(&msm_psy_ac);

            }
            else if (msm_batt_info.current_chg_source & USB_CHG)
            {
                msm_batt_info.current_chg_source &= ~USB_CHG;
                DEBUG_MSM_BATTERY(KERN_INFO "%s() : USB charger removed\n", __func__);

                power_supply_changed(&msm_psy_usb);
            }
            else
                power_supply_changed(&msm_psy_batt);
        }
    }
    else
        power_supply_changed(&msm_psy_batt);

    msm_batt_info.charger_status = rep_batt_chg.charger_status;
    msm_batt_info.charger_type = rep_batt_chg.charger_type;
    msm_batt_info.battery_status = rep_batt_chg.battery_status;
    msm_batt_info.battery_level = rep_batt_chg.battery_level;
    msm_batt_info.battery_voltage = rep_batt_chg.battery_voltage;
    msm_batt_info.battery_temp = rep_batt_chg.battery_temp;
    
    msm_batt_info.chg_fulled = rep_batt_chg.chg_fulled;
    msm_batt_info.charging = rep_batt_chg.charging;
    
    DEBUG_MSM_BATTERY();
}

static int msm_batt_handle_capacity(void)
{
    static u32 temp_capacity = 0;
    bool need_update = FALSE;

    enum direction
    {
        DIRECTION_NONE,
        DIRECTION_UP,
        DIRECTION_DOWN,
    } dir;

    temp_capacity = msm_batt_info.calculate_capacity(msm_batt_info.battery_voltage);

    printk("%s: %d voltage: %d CAP:%d\n", __func__, __LINE__, msm_batt_info.battery_voltage, temp_capacity);

    printk(KERN_INFO "charger_status = %s, charger_type = %s,"
           " batt_status = %s, batt_level = %s,"
           " batt_volt = %u, batt_temp = %u, chg_fulled = %d\n",
           charger_status[rep_batt_chg.charger_status],
           charger_type[rep_batt_chg.charger_type],
           battery_status[rep_batt_chg.battery_status],
           battery_level[rep_batt_chg.battery_level], 
           rep_batt_chg.battery_voltage, 
           rep_batt_chg.battery_temp, 
           rep_batt_chg.chg_fulled);

    need_update = FALSE;

    /* battery level, id not valid */
    if ((rep_batt_chg.battery_status != BATTERY_STATUS_INVALID)
        && (rep_batt_chg.battery_status != BATTERY_STATUS_BAD))
    {
        /* get capacity direction */
        if (temp_capacity > msm_batt_info.batt_capacity)
        {
            dir = DIRECTION_UP;
        }
        else if (temp_capacity < msm_batt_info.batt_capacity)
        {
            dir = DIRECTION_DOWN;
        }
        else
        {
            dir = DIRECTION_NONE;
        }

        do
        {
            /* resume & StateMachine start, regardless of battery status and 
            charger status */
            if (msm_batt_info.is_resume || (msm_batt_info.is_poweron))
            {
                msm_batt_info.batt_capacity = temp_capacity;
                need_update = TRUE;
                break;
            }

            /* battery status, charging */
            if (msm_batt_info.charging)
            {
                /* direction up */
                if ((dir == DIRECTION_UP) && (msm_batt_info.batt_capacity < 99))
                {
                    msm_batt_info.batt_capacity++;
                    need_update = TRUE;
                }
                /* direction down */
                else if ((dir == DIRECTION_DOWN) && (msm_batt_info.batt_capacity > 0))
                {
                    msm_batt_info.batt_capacity--;
                    need_update = TRUE;
                }
            }
            /* battery status, discharging\not charging\unknown */
            else
            {
                /* direction down */
                if ((dir == DIRECTION_DOWN) && (msm_batt_info.batt_capacity > 0))
                {
                    msm_batt_info.batt_capacity--;
                    need_update = TRUE;
                }
            }
        }
        while (0);
    }
    else
    {
        msm_batt_info.batt_capacity = temp_capacity;
        need_update = TRUE;
    }

    /* normal status */
    if (need_update)
    {
        power_supply_changed(&msm_psy_batt);
    }

    return 0;
}

static int msm_batt_handle_suspend(void)
{
    if (msm_batt_info.battery_timer)
    {
        del_timer(msm_batt_info.battery_timer);
    }

    return 0;
}

static int msm_batt_handle_resume(void)
{
    msm_batt_info.is_resume = TRUE;

    if (msm_batt_info.battery_timer)
    {
        del_timer(msm_batt_info.battery_timer);
        msm_batt_info.battery_timer->expires = jiffies + BATTERY_CAPACITY_POLL_TIME;
        add_timer(msm_batt_info.battery_timer);
    }

    msm_batt_update_psy_status_v1();
    msm_batt_handle_capacity();

    msm_batt_info.is_resume = FALSE;

    return 0;
}

static int msm_batt_handle_status_timeout(void)
{
    msm_batt_update_psy_status_v1();
    return 0;
}

static int msm_batt_handle_event(void)
{
    int rc;

    if (!atomic_read(&msm_batt_info.handle_event))
    {
        printk(KERN_ERR "%s(): batt call back thread while in "
               "msm_rpc_read got signal. Signal is not from "
               "early suspend or  from late resume or from Clean up " "thread.\n", __func__);
        return 0;
    }

    DEBUG_MSM_BATTERY(KERN_INFO "%s(): batt call back thread" "got signal\n", __func__);

    if (msm_batt_info.type_of_event & SUSPEND_EVENT)
    {
        DEBUG_MSM_BATTERY(KERN_INFO "%s(): Handle Suspend event. event = %08x\n", __func__, msm_batt_info.type_of_event);

        rc = msm_batt_handle_suspend();

        msm_batt_info.type_of_event &= ~SUSPEND_EVENT;

        return rc;
    }
    else if (msm_batt_info.type_of_event & RESUME_EVENT)
    {
        DEBUG_MSM_BATTERY(KERN_INFO "%s(): Handle Resume event. event = %08x\n", __func__, msm_batt_info.type_of_event);

        rc = msm_batt_handle_resume();

        msm_batt_info.type_of_event &= ~RESUME_EVENT;

        return rc;
    }
    else if (msm_batt_info.type_of_event & CLEANUP_EVENT)
    {
        DEBUG_MSM_BATTERY(KERN_INFO "%s(): Cleanup event occured. event = %08x\n", __func__, msm_batt_info.type_of_event);

        msm_batt_info.type_of_event &= ~CLEANUP_EVENT;

        return 0;
    }
    else if (msm_batt_info.type_of_event & CAPACITY_EVENT)
    {
        DEBUG_MSM_BATTERY(KERN_INFO "%s(): capacity event occured. event = %08x\n", __func__, msm_batt_info.type_of_event);

        rc = msm_batt_handle_capacity();

        msm_batt_info.type_of_event &= ~CAPACITY_EVENT;

        return rc;
    }
    else
    {
        printk(KERN_ERR "%s(): Unknown event occured. event = %08x\n", __func__, msm_batt_info.type_of_event);
        return 0;
    }
}


static void msm_batt_wake_up_waiting_thread(u32 event)
{
    DEBUG_MSM_BATTERY();

    DEBUG_MSM_BATTERY("wake up wait queue!\n");
    atomic_set(&msm_batt_info.handle_event, 1);
    wake_up(&msm_batt_info.wait_q);

    DEBUG_MSM_BATTERY();
}


static void msm_batt_wait_for_batt_chg_event(struct work_struct *work)
{
    int ret;
    int rc;

    DEBUG_MSM_BATTERY(KERN_INFO "%s: Batt status call back thread started.(SMEM)\n", __func__);

    DEBUG_MSM_BATTERY(KERN_INFO "%s: First time Update Batt status without waiting for"
           " call back event from modem .\n", __func__);

    msm_batt_info.is_poweron = TRUE;
    msm_batt_update_psy_status_v1();
    msm_batt_handle_capacity();
    msm_batt_info.is_poweron = FALSE;
    while (1)
    {
        ret = wait_event_interruptible_timeout(msm_batt_info.wait_q,
                                               msm_batt_info.type_of_event & EVENT_CONDITION, BATTERY_STATUS_POLL_TIME);

        DEBUG_MSM_BATTERY("ret = %d", ret);
        DEBUG_MSM_BATTERY("%s: %d\n", __func__, __LINE__);

        if (ret != 0)
        {
            rc = msm_batt_handle_event();

            if (rc != 0)
            {
                printk("%s: handle event error!\n", __func__);
            }

            if (msm_batt_info.type_of_event & EVENT_CONDITION)
            {
                DEBUG_MSM_BATTERY();

                continue;
            }

            DEBUG_MSM_BATTERY();

            atomic_set(&msm_batt_info.handle_event, 0);
        }
        else
        {
            DEBUG_MSM_BATTERY();

            msm_batt_handle_status_timeout();
        }
    }
    DEBUG_MSM_BATTERY(KERN_INFO "%s: Batt status call back thread stopped.(SMEM)\n", __func__);
}

static int msm_batt_send_event(u32 type_of_event)
{
    int rc;
    unsigned long flags;

    rc = 0;

    spin_lock_irqsave(&msm_batt_info.lock, flags);

    if (type_of_event & SUSPEND_EVENT)
        DEBUG_MSM_BATTERY(KERN_INFO "%s() : Suspend event ocurred." "events = %08x\n", __func__, type_of_event);
    else if (type_of_event & RESUME_EVENT)
        DEBUG_MSM_BATTERY(KERN_INFO "%s() : Resume event ocurred." "events = %08x\n", __func__, type_of_event);
    else if (type_of_event & CLEANUP_EVENT)
        DEBUG_MSM_BATTERY(KERN_INFO "%s() : Cleanup event ocurred." "events = %08x\n", __func__, type_of_event);
    else if (type_of_event & CAPACITY_EVENT)
        DEBUG_MSM_BATTERY(KERN_INFO "%s() : CAPACITY event ocurred." "events = %08x\n", __func__, type_of_event);
    else
    {
        DEBUG_MSM_BATTERY(KERN_ERR "%s() : Unknown event ocurred." "events = %08x\n", __func__, type_of_event);

        spin_unlock_irqrestore(&msm_batt_info.lock, flags);
        return -EIO;
    }

    msm_batt_info.type_of_event |= type_of_event;

    if (msm_batt_info.type_of_event & CLEANUP_EVENT)
    {
        msm_batt_wake_up_waiting_thread(CLEANUP_EVENT);
    }
    else if (msm_batt_info.type_of_event & SUSPEND_EVENT)
    {
        msm_batt_wake_up_waiting_thread(SUSPEND_EVENT);
    }
    else if (msm_batt_info.type_of_event & RESUME_EVENT)
    {
        msm_batt_wake_up_waiting_thread(RESUME_EVENT);
    }
    else if (msm_batt_info.type_of_event & CAPACITY_EVENT)
    {
        msm_batt_wake_up_waiting_thread(CAPACITY_EVENT);
    }
    else
    {
        
    }

    spin_unlock_irqrestore(&msm_batt_info.lock, flags);

    return rc;
}

static void msm_batt_start_cb_thread(void)
{
    DEBUG_MSM_BATTERY("task queue start!\n");
    atomic_set(&msm_batt_info.handle_event, 0);
    queue_work(msm_batt_info.msm_batt_wq, &msm_batt_cb_work);
    DEBUG_MSM_BATTERY("task queue end!\n");
}

static void msm_batt_early_suspend(struct early_suspend *h);

static int msm_batt_cleanup(void)
{
    int rc = 0;

    if (msm_batt_info.battery_timer)
        del_timer(msm_batt_info.battery_timer);

    if (msm_batt_info.msm_batt_wq)
    {
        msm_batt_send_event(CLEANUP_EVENT);
        destroy_workqueue(msm_batt_info.msm_batt_wq);
    }

    if (msm_batt_info.msm_psy_ac)
        power_supply_unregister(msm_batt_info.msm_psy_ac);
    if (msm_batt_info.msm_psy_usb)
        power_supply_unregister(msm_batt_info.msm_psy_usb);
    if (msm_batt_info.msm_psy_batt)
        power_supply_unregister(msm_batt_info.msm_psy_batt);

#ifdef CONFIG_HAS_EARLYSUSPEND
    if (msm_batt_info.early_suspend.suspend == msm_batt_early_suspend)
        unregister_early_suspend(&msm_batt_info.early_suspend);
#endif
    return rc;
}

static u32 msm_batt_capacity(u32 current_voltage)
{
    u32 low_voltage = msm_batt_info.voltage_min_design;
    u32 high_voltage = msm_batt_info.voltage_max_design;

    return (current_voltage - low_voltage) * 100 / (high_voltage - low_voltage);
}

#ifdef CONFIG_HAS_EARLYSUSPEND
void msm_batt_early_suspend(struct early_suspend *h)
{
    DEBUG_MSM_BATTERY(KERN_INFO "%s(): going to early suspend\n", __func__);
    //msm_batt_send_event(SUSPEND_EVENT);
}

void msm_batt_late_resume(struct early_suspend *h)
{
    DEBUG_MSM_BATTERY(KERN_INFO "%s(): going to resume\n", __func__);
    //msm_batt_send_event(RESUME_EVENT);
}
#endif
static int msm_batt_suspend(struct platform_device *pdev, pm_message_t state)
{
    msm_batt_handle_suspend();
    return 0;
}

static int msm_batt_resume(struct platform_device *pdev)
{
    msm_batt_handle_resume();
    return 0;
}

void msm_batt_capacity_timeout(unsigned long unused)
{
    DEBUG_MSM_BATTERY(KERN_INFO "%s(): handle capacity report\n", __func__);

    msm_batt_send_event(CAPACITY_EVENT);

    del_timer(msm_batt_info.battery_timer);
    msm_batt_info.battery_timer->expires = jiffies + BATTERY_CAPACITY_POLL_TIME;
    add_timer(msm_batt_info.battery_timer);

}

static int __devinit msm_batt_probe(struct platform_device *pdev)
{
    int rc;
    struct msm_psy_batt_pdata *pdata = pdev->dev.platform_data;

    if (pdev->id != -1)
    {
        dev_err(&pdev->dev, "%s: MSM chipsets Can only support one" " battery ", __func__);
        return -EINVAL;
    }

    /* battery timer */
    msm_batt_info.battery_timer = kmalloc(sizeof(struct timer_list), GFP_KERNEL);
    if (msm_batt_info.battery_timer == NULL)
    {
        printk("unable to malloc memery space\n");
        return (-ENOMEM);
    }

    init_timer(msm_batt_info.battery_timer);
    msm_batt_info.battery_timer->function = msm_batt_capacity_timeout;
    msm_batt_info.battery_timer->data = 0;

    if (pdata->avail_chg_sources & AC_CHG)
    {
        rc = power_supply_register(&pdev->dev, &msm_psy_ac);
        if (rc < 0)
        {
            dev_err(&pdev->dev, "%s: power_supply_register failed" " rc = %d\n", __func__, rc);
            msm_batt_cleanup();
            return rc;
        }
        msm_batt_info.msm_psy_ac = &msm_psy_ac;
        msm_batt_info.avail_chg_sources |= AC_CHG;
    }

    if (pdata->avail_chg_sources & USB_CHG)
    {
        rc = power_supply_register(&pdev->dev, &msm_psy_usb);
        if (rc < 0)
        {
            dev_err(&pdev->dev, "%s: power_supply_register failed" " rc = %d\n", __func__, rc);
            msm_batt_cleanup();
            return rc;
        }
        msm_batt_info.msm_psy_usb = &msm_psy_usb;
        msm_batt_info.avail_chg_sources |= USB_CHG;
    }

    if (!msm_batt_info.msm_psy_ac && !msm_batt_info.msm_psy_usb)
    {

        dev_err(&pdev->dev, "%s: No external Power supply(AC or USB)" "is avilable\n", __func__);
        msm_batt_cleanup();
        return -ENODEV;
    }

    msm_batt_info.voltage_max_design = pdata->voltage_max_design;
    msm_batt_info.voltage_min_design = pdata->voltage_min_design;
    msm_batt_info.batt_technology = pdata->batt_technology;
    msm_batt_info.calculate_capacity = pdata->calculate_capacity;

    if (!msm_batt_info.voltage_min_design)
        msm_batt_info.voltage_min_design = BATTERY_LOW;
    if (!msm_batt_info.voltage_max_design)
        msm_batt_info.voltage_max_design = BATTERY_HIGH;

    if (msm_batt_info.batt_technology == POWER_SUPPLY_TECHNOLOGY_UNKNOWN)
        msm_batt_info.batt_technology = POWER_SUPPLY_TECHNOLOGY_LION;

    if (!msm_batt_info.calculate_capacity)
        msm_batt_info.calculate_capacity = msm_batt_capacity;

    rc = power_supply_register(&pdev->dev, &msm_psy_batt);
    if (rc < 0)
    {
        dev_err(&pdev->dev, "%s: power_supply_register failed" " rc=%d\n", __func__, rc);
        msm_batt_cleanup();
        return rc;
    }
    msm_batt_info.msm_psy_batt = &msm_psy_batt;

#ifdef CONFIG_HAS_EARLYSUSPEND
    msm_batt_info.early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
    msm_batt_info.early_suspend.suspend = msm_batt_early_suspend;
    msm_batt_info.early_suspend.resume = msm_batt_late_resume;
    register_early_suspend(&msm_batt_info.early_suspend);
#endif

    msm_batt_info.is_resume = FALSE;
    del_timer(msm_batt_info.battery_timer);
    msm_batt_info.battery_timer->expires = jiffies + BATTERY_CAPACITY_POLL_TIME;
    add_timer(msm_batt_info.battery_timer);

    msm_batt_start_cb_thread();

    return 0;
}



static struct platform_driver msm_batt_driver;
static int __devinit msm_batt_init_smem(void)
{
    int rc;

    spin_lock_init(&msm_batt_info.lock);

    init_waitqueue_head(&msm_batt_info.wait_q);

    msm_batt_info.msm_batt_wq = create_singlethread_workqueue("msm_battery");

    if (!msm_batt_info.msm_batt_wq)
    {
        printk(KERN_ERR "%s: create workque failed \n", __func__);
        return -ENOMEM;
    }

    rc = platform_driver_register(&msm_batt_driver);

    if (rc < 0)
    {
        printk(KERN_ERR "%s: platform_driver_register failed for " "batt driver. rc = %d\n", __func__, rc);
        return rc;
    }

    return 0;
}

static int __devexit msm_batt_remove(struct platform_device *pdev)
{
    int rc;

    rc = msm_batt_cleanup();

    if (rc < 0)
    {
        dev_err(&pdev->dev, "%s: msm_batt_cleanup  failed rc=%d\n", __func__, rc);
        return rc;
    }
    return 0;
}

static struct platform_driver msm_batt_driver = {
    .probe = msm_batt_probe,
    .remove = __devexit_p(msm_batt_remove),
    .driver = {
               .name = "msm-battery",
               .owner = THIS_MODULE,
               },
    .suspend = msm_batt_suspend,
    .resume = msm_batt_resume,
};

static int __init msm_batt_init(void)
{
    int rc;

    printk(KERN_INFO "2010_02_02 ------> msm_batt_init is entered!!!!!!!!1\n");
	   
    rc = msm_batt_init_smem();

    if (rc < 0)
    {
        printk(KERN_ERR "%s: msm_batt_init_rpc Failed  rc=%d\n", __func__, rc);
        msm_batt_cleanup();
        return rc;
    }

    return 0;
}

static void __exit msm_batt_exit(void)
{
    platform_driver_unregister(&msm_batt_driver);
}

module_init(msm_batt_init);
module_exit(msm_batt_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Kiran Kandi, Qualcomm Innovation Center, Inc.");
MODULE_DESCRIPTION("Battery driver for Qualcomm MSM chipsets.");
MODULE_VERSION("1.0");
MODULE_ALIAS("platform:msm_battery");

#else

#include <linux/earlysuspend.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/workqueue.h>

#include <asm/atomic.h>

#include "../../arch/arm/mach-msm/smd_private.h"

#include <mach/msm_battery.h>

#define FEATURE_ZTE_APP_ENABLE_USB_CHARGING

#ifdef CONFIG_SCREEN_ON_WITHOUT_KEYOCDE
#include <linux/wakelock.h>
static struct wake_lock charger_wake_lock;
static int wl_initialized = 0;//The rpc occur anytime ,so ,we must make sure that the batt driver already initialized
#endif


#define MSM_BATTERY_DEBUGX
#ifdef MSM_BATTERY_DEBUG
#define DEBUG_MSM_BATTERY(fmt, args...)\
    do\
    {\
        printk("%s:%s:%d:", __FILE__, __FUNCTION__, __LINE__);\
        printk(fmt "\n", ## args);\
    }\
    while (0)
#else
#define DEBUG_MSM_BATTERY(fmt, args...) do{}while(0)
#endif

#define FALSE (0)
#define TRUE  (1)

#define BATTERY_LOW            	    2800
#define BATTERY_HIGH           	    4300

#define BATTERY_STATUS_POLL_TIME    (2*HZ)
#define SUSPEND_EVENT		(1UL << 0)
#define RESUME_EVENT		(1UL << 1)
#define CLEANUP_EVENT		(1UL << 2)
//#define CAPACITY_EVENT      (1UL << 3)
#define EVENT_CONDITION (SUSPEND_EVENT | RESUME_EVENT | CLEANUP_EVENT)


#define DEBUG  0

#if DEBUG
#define DBG(x...) printk(KERN_INFO x)
#else
#define DBG(x...) do {} while (0)
#endif





/*
 * This enum contains defintions of the charger hardware status
 */
enum chg_charger_status_type
{
    /* The charger is good      */
    CHARGER_STATUS_GOOD,
    /* The charger is bad       */
    CHARGER_STATUS_BAD,
    /* The charger is weak      */
    CHARGER_STATUS_WEAK,
    /* Invalid charger status.  */
    CHARGER_STATUS_INVALID
};

#ifdef MSM_BATTERY_DEBUGX
static char *charger_status[] = {
    "good", "bad", "weak", "invalid"
};
#endif

/*
 *This enum contains defintions of the charger hardware type
 */
enum chg_charger_hardware_type
{
    /* The charger is removed                 */
    CHARGER_TYPE_NONE,
    /* The charger is a regular wall charger   */
    CHARGER_TYPE_WALL,
    /* The charger is a PC USB                 */
    CHARGER_TYPE_USB_PC,
    /* The charger is a wall USB charger       */
    CHARGER_TYPE_USB_WALL,
    /* The charger is a USB carkit             */
    CHARGER_TYPE_USB_CARKIT,
    /* Invalid charger hardware status.        */
    CHARGER_TYPE_INVALID
};

#ifdef MSM_BATTERY_DEBUGX
static char *charger_type[] = {
    "No charger", "wall", "USB PC", "USB wall", "USB car kit",
    "invalid charger"
};
#endif

/*
 *  This enum contains defintions of the battery status
 */
enum chg_battery_status_type
{
    /* The battery is good        */
    BATTERY_STATUS_GOOD,
    /* The battery is cold/hot    */
    BATTERY_STATUS_BAD_TEMP,
    /* The battery is bad         */
    BATTERY_STATUS_BAD,
    /* Invalid battery status.    */
    BATTERY_STATUS_INVALID
};

#ifdef MSM_BATTERY_DEBUGX
static char *battery_status[] = {
    "good ", "bad temperature", "bad", "invalid"
};
#endif

/*
 *This enum contains defintions of the battery voltage level
 */
enum chg_battery_level_type
{
    /* The battery voltage is dead/very low (less than 3.2V)        */
    BATTERY_LEVEL_DEAD,
    /* The battery voltage is weak/low (between 3.2V and 3.4V)      */
    BATTERY_LEVEL_WEAK,
    /* The battery voltage is good/normal(between 3.4V and 4.2V)  */
    BATTERY_LEVEL_GOOD,
    /* The battery voltage is up to full (close to 4.2V)            */
    BATTERY_LEVEL_FULL,
    /* Invalid battery voltage level.                               */
    BATTERY_LEVEL_INVALID
};
#ifdef MSM_BATTERY_DEBUGX
static char *battery_level[] = {
    "dead", "weak", "good", "full", "invalid"
};
#endif
struct __attribute__((packed)) smem_batt_chg_t
{
    u8 charger_type;
    u8 charger_status;
    u8 charging;
    u8 chg_fulled;

    u8 battery_status;
    u8 battery_level;
    u16 battery_voltage;
    s16 battery_temp;
    u8 battery_capacity;
};

static struct smem_batt_chg_t rep_batt_chg;

struct msm_battery_info
{
    u32 voltage_max_design;
    u32 voltage_min_design;
    u32 chg_api_version;
    u32 batt_technology;

    u32 avail_chg_sources;
    u32 current_chg_source;

    u32 batt_status;
    u32 batt_health;
    u32 charger_valid;
    u32 batt_valid;
    u32 battery_capacity;

    u8 charger_status;
    u8 charger_type;
    u8 charging;
    u8 chg_fulled;

    u8 battery_status;
    u8 battery_level;
    u16 battery_voltage;
    s16 battery_temp;
    u8 battery_temp_exceeded_count;

    u32(*calculate_capacity) (u32 voltage);
	
#ifdef FEATURE_ZTE_APP_ENABLE_USB_CHARGING	
	s32 batt_handle;
	struct msm_rpc_endpoint *chg_ep;
#endif

    spinlock_t lock;

    struct power_supply *msm_psy_ac;
    struct power_supply *msm_psy_usb;
    struct power_supply *msm_psy_batt;

    struct workqueue_struct *msm_batt_wq;

    wait_queue_head_t wait_q;

    struct early_suspend early_suspend;

    atomic_t handle_event;

    u32 type_of_event;
};

static void msm_batt_wait_for_batt_chg_event(struct work_struct *work);

static DECLARE_WORK(msm_batt_cb_work, msm_batt_wait_for_batt_chg_event);

static int msm_batt_cleanup(void);

static struct msm_battery_info msm_batt_info = {
    .charger_status = -1,
    .charger_type = -1,
    .battery_status = -1,
    .battery_level = -1,
    .battery_voltage = -1,
    .battery_temp = -1,
};

static enum power_supply_property msm_power_props[] =
{
    POWER_SUPPLY_PROP_ONLINE,
};

static char *msm_power_supplied_to[] = {
    "battery",
};

static int msm_power_get_property(struct power_supply *psy,
                                  enum power_supply_property psp, union power_supply_propval *val)
{
    switch (psp)
    {
    case POWER_SUPPLY_PROP_ONLINE:

        if (psy->type == POWER_SUPPLY_TYPE_MAINS)
        {

            val->intval = msm_batt_info.current_chg_source & AC_CHG ? 1 : 0;
            DEBUG_MSM_BATTERY(KERN_INFO "%s(): power supply = %s online = %d\n", __func__, psy->name, val->intval);

        }

        if (psy->type == POWER_SUPPLY_TYPE_USB)
        {

            val->intval = msm_batt_info.current_chg_source & USB_CHG ? 1 : 0;

            DEBUG_MSM_BATTERY(KERN_INFO "%s(): power supply = %s online = %d\n", __func__, psy->name, val->intval);
        }

        break;
    default:
        return -EINVAL;
    }
    return 0;
}

static struct power_supply msm_psy_ac = {
    .name = "ac",
    .type = POWER_SUPPLY_TYPE_MAINS,
    .supplied_to = msm_power_supplied_to,
    .num_supplicants = ARRAY_SIZE(msm_power_supplied_to),
    .properties = msm_power_props,
    .num_properties = ARRAY_SIZE(msm_power_props),
    .get_property = msm_power_get_property,
};

static struct power_supply msm_psy_usb = {
    .name = "usb",
    .type = POWER_SUPPLY_TYPE_USB,
    .supplied_to = msm_power_supplied_to,
    .num_supplicants = ARRAY_SIZE(msm_power_supplied_to),
    .properties = msm_power_props,
    .num_properties = ARRAY_SIZE(msm_power_props),
    .get_property = msm_power_get_property,
};

static enum power_supply_property msm_batt_power_props[] =
{
    POWER_SUPPLY_PROP_STATUS,
    POWER_SUPPLY_PROP_HEALTH,
    POWER_SUPPLY_PROP_PRESENT,
    POWER_SUPPLY_PROP_TECHNOLOGY,
    POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
    POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
    POWER_SUPPLY_PROP_VOLTAGE_NOW,
    POWER_SUPPLY_PROP_CAPACITY,
    POWER_SUPPLY_PROP_TEMP,
};

static void msm_batt_external_power_changed(struct power_supply *psy)
{
    //DEBUG_MSM_BATTERY(KERN_INFO "%s() : external power supply changed for %s\n", __func__, psy->name);
    power_supply_changed(psy);
}

static int msm_batt_power_get_property(struct power_supply *psy,
                                       enum power_supply_property psp, union power_supply_propval *val)
{
    switch (psp)
    {
    case POWER_SUPPLY_PROP_STATUS:
        val->intval = msm_batt_info.batt_status;
        break;
    case POWER_SUPPLY_PROP_HEALTH:
        val->intval = msm_batt_info.batt_health;
        break;
    case POWER_SUPPLY_PROP_PRESENT:
        val->intval = msm_batt_info.batt_valid;
        break;
    case POWER_SUPPLY_PROP_TECHNOLOGY:
        val->intval = msm_batt_info.batt_technology;
        break;
    case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
        val->intval = msm_batt_info.voltage_max_design*1000;
        break;
    case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
        val->intval = msm_batt_info.voltage_min_design*1000;
        break;
    case POWER_SUPPLY_PROP_VOLTAGE_NOW:
        val->intval = msm_batt_info.battery_voltage*1000;
        break;
    case POWER_SUPPLY_PROP_CAPACITY:
        val->intval = msm_batt_info.battery_capacity;
        break;
    case POWER_SUPPLY_PROP_TEMP:
        val->intval = msm_batt_info.battery_temp*10;
        break;
    default:
        return -EINVAL;
    }
    return 0;
}

static struct power_supply msm_psy_batt = {
    .name = "battery",
    .type = POWER_SUPPLY_TYPE_BATTERY,
    .properties = msm_batt_power_props,
    .num_properties = ARRAY_SIZE(msm_batt_power_props),
    .get_property = msm_batt_power_get_property,
    .external_power_changed = msm_batt_external_power_changed,
};

#ifdef FEATURE_ZTE_APP_ENABLE_USB_CHARGING
#include <mach/msm_rpcrouter.h>

#define USB_CHG_DISABLE 0
#define USB_CHG_ENABLE 1
static int usb_charger_enable = USB_CHG_ENABLE;	/*0 disable usb charging,1 enable usb charging*/
static int usb_charger_enable_pre = USB_CHG_ENABLE;	//used to record the previous state for usb charging
module_param_named(usb_chg_enable, usb_charger_enable, int, S_IRUGO | S_IWUSR | S_IWGRP);

#define CHG_RPC_PROG		0x3000001a
#define CHG_RPC_VERS		0x00010003

#define BATTERY_ENABLE_DISABLE_USB_CHG_PROC 		6
#define BATTERY_MAX_TEMP 	68
#define BATTERY_MAX_TEMP_EXCEEDED_COUNT 5

/*--------------------------------------------------------------
msm_batt_handle_control_usb_charging() is added according msm_chg_usb_charger_connected() in rpc_hsusb.c
and the rpc is used to stop or resume usb charging
---------------------------------------------------------------*/
static int msm_batt_handle_control_usb_charging(u32 usb_enable_disable)
{
	int rc;

	struct batt_modify_client_req {
		struct rpc_request_hdr hdr;
		u32 usb_chg_enable;
	} req;
		printk(KERN_INFO "%s: LHX msm_rpc_write usb switch enable/disable = %d start.\n",
				__func__,usb_enable_disable);

	req.usb_chg_enable = cpu_to_be32(usb_enable_disable);
    rc=msm_rpc_call(msm_batt_info.chg_ep, BATTERY_ENABLE_DISABLE_USB_CHG_PROC, &req,
                    sizeof(req), 5 * HZ);

	if (rc < 0) {
		printk(KERN_ERR
		       "%s(): msm_rpc_write failed.  proc = 0x%08x rc = %d\n",
		       __func__, BATTERY_ENABLE_DISABLE_USB_CHG_PROC, rc);
		return rc;
	}
		printk(KERN_INFO "%s: LHX msm_rpc_write usb switch enable/disable end rc = %d.\n",
				__func__,rc);

	return 0;
}
#endif

/*get the battery information form SMEM*/
static int msm_batt_get_batt_chg_status_v1(void)
{
    struct smem_batt_chg_t *batt_chg_ptr;

    memset(&rep_batt_chg, 0, sizeof(rep_batt_chg));

    batt_chg_ptr = smem_alloc(SMEM_BATT_INFO, sizeof(struct smem_batt_chg_t));
    if (batt_chg_ptr == NULL)
    {
        printk("%s: share memery read error!\n", __func__);
        return -EIO;
    }
    else
    {
        rep_batt_chg = *batt_chg_ptr;	/*get the battery information form SMEM*/
        DEBUG_MSM_BATTERY("status read ok!base: %d size: %d\n", SMEM_BATT_INFO, sizeof(struct smem_batt_chg_t));

        DEBUG_MSM_BATTERY(KERN_INFO "charger_status = %s, charger_type = %s,"
               " batt_status = %s, batt_level = %s,"
               " batt_volt = %u, batt_cap = %u, batt_temp = %u, chg_fulled = %d\n",
               charger_status[rep_batt_chg.charger_status],
               charger_type[rep_batt_chg.charger_type],
               battery_status[rep_batt_chg.battery_status],
               battery_level[rep_batt_chg.battery_level], 
               rep_batt_chg.battery_voltage, 
               rep_batt_chg.battery_capacity,
               rep_batt_chg.battery_temp, 
               rep_batt_chg.chg_fulled);

        if (rep_batt_chg.battery_voltage > msm_batt_info.voltage_max_design)
        {
//            printk("battery voltage overvoltage: %d\n", rep_batt_chg.battery_voltage);
            rep_batt_chg.battery_voltage = msm_batt_info.voltage_max_design;
        }
        else if (rep_batt_chg.battery_voltage < msm_batt_info.voltage_min_design)
        {
//            printk("battery voltage low: %d\n", rep_batt_chg.battery_voltage);
            rep_batt_chg.battery_voltage = msm_batt_info.voltage_min_design;
        }
    }

    return 0;
}

void msm_batt_update_psy_status_v1(void)
{
    msm_batt_get_batt_chg_status_v1();/*get battery information for SMEM*/

    DEBUG_MSM_BATTERY();

    if (msm_batt_info.charger_status == rep_batt_chg.charger_status &&
        msm_batt_info.charger_type == rep_batt_chg.charger_type &&
        msm_batt_info.battery_status == rep_batt_chg.battery_status &&
        msm_batt_info.battery_level == rep_batt_chg.battery_level &&
	/*if voltage not change,or changes within the voltage's range, return  */
	((msm_batt_info.battery_voltage == rep_batt_chg.battery_voltage) ||
		((msm_batt_info.battery_voltage != rep_batt_chg.battery_voltage) &&
			((msm_batt_info.voltage_min_design <= rep_batt_chg.battery_voltage) &&
			(msm_batt_info.voltage_max_design >= rep_batt_chg.battery_voltage)))) &&
        msm_batt_info.battery_capacity == rep_batt_chg.battery_capacity && 
        msm_batt_info.battery_temp == rep_batt_chg.battery_temp && 
#ifdef FEATURE_ZTE_APP_ENABLE_USB_CHARGING
	(((rep_batt_chg.charger_type == CHARGER_TYPE_USB_WALL || 
	rep_batt_chg.charger_type == CHARGER_TYPE_USB_PC)&&	   
       usb_charger_enable_pre == usb_charger_enable ) ||
       (rep_batt_chg.charger_type != CHARGER_TYPE_USB_WALL && 
	rep_batt_chg.charger_type != CHARGER_TYPE_USB_PC))&&
#endif        
        msm_batt_info.chg_fulled == rep_batt_chg.chg_fulled &&  
        msm_batt_info.charging == rep_batt_chg.charging)
    {
        DEBUG_MSM_BATTERY(KERN_NOTICE "%s() : Got unnecessary event from Modem "
               "PMIC VBATT driver. Nothing changed in Battery or " "charger status\n", __func__);
        return;
    }

	/*once one of the following change,such as charger_status,charger_type,battery_status
	,battery_level, battery_voltage,battery_temp,chg_fulled and charging.
	, printk the debug information.*/
    printk(KERN_INFO "charger_status = %s, charger_type = %s,"
           " batt_status = %s, batt_level = %s,"
           " batt_volt = %u, batt_cap = %u, batt_temp = %u, chg_fulled = %d\n",
           charger_status[rep_batt_chg.charger_status],
           charger_type[rep_batt_chg.charger_type],
           battery_status[rep_batt_chg.battery_status],
           battery_level[rep_batt_chg.battery_level], 
           rep_batt_chg.battery_voltage, 
           rep_batt_chg.battery_capacity, 
           rep_batt_chg.battery_temp, 
           rep_batt_chg.chg_fulled);

    if (rep_batt_chg.battery_status != BATTERY_STATUS_INVALID)
    {
        msm_batt_info.batt_valid = 1;

        if (rep_batt_chg.battery_voltage > msm_batt_info.voltage_max_design)
        {
            msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_OVERVOLTAGE;
            msm_batt_info.batt_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
        }
        else if (rep_batt_chg.battery_voltage < msm_batt_info.voltage_min_design)
        {
            msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_DEAD;
            msm_batt_info.batt_status = POWER_SUPPLY_STATUS_UNKNOWN;
        }
        else if (rep_batt_chg.battery_status == BATTERY_STATUS_BAD)
        {
            printk("battery status bad\n");
            msm_batt_info.battery_capacity = rep_batt_chg.battery_capacity;
            msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_DEAD;
            msm_batt_info.batt_status = POWER_SUPPLY_STATUS_UNKNOWN;
        }
        else if (rep_batt_chg.battery_status == BATTERY_STATUS_BAD_TEMP)
        {
            printk("battery status bad temp\n");
            msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_OVERHEAT;

            if (rep_batt_chg.charger_status == CHARGER_STATUS_BAD
                || rep_batt_chg.charger_status == CHARGER_STATUS_INVALID)
                msm_batt_info.batt_status = POWER_SUPPLY_STATUS_UNKNOWN;
            else
                msm_batt_info.batt_status = POWER_SUPPLY_STATUS_NOT_CHARGING;
        }
        else if ((rep_batt_chg.charger_status == CHARGER_STATUS_GOOD
                  || rep_batt_chg.charger_status == CHARGER_STATUS_WEAK)
                 && (rep_batt_chg.battery_status == BATTERY_STATUS_GOOD))
        {
            msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_GOOD;

            if (rep_batt_chg.chg_fulled)
            {
                msm_batt_info.battery_capacity = 100;
                msm_batt_info.batt_status = POWER_SUPPLY_STATUS_FULL;
            }
            else
	#ifdef FEATURE_ZTE_APP_ENABLE_USB_CHARGING
		{
	         if ((rep_batt_chg.charger_type == CHARGER_TYPE_USB_WALL || 
			 	rep_batt_chg.charger_type == CHARGER_TYPE_USB_PC))
			{
				if(USB_CHG_DISABLE == usb_charger_enable)//if disabled usb charging, show  discharging
				{
			                msm_batt_info.batt_status = POWER_SUPPLY_STATUS_DISCHARGING;
				}
				else
				{
			                msm_batt_info.batt_status = POWER_SUPPLY_STATUS_CHARGING;
				}
				/* 
				if it needs to disable usb charging and it is really charging ,call RPC to stop charging machine;
				if it needs to enable usb charging and it is really not charging,call RPC to resume charging machine.*/
				if((usb_charger_enable_pre != usb_charger_enable)&&
					((usb_charger_enable == USB_CHG_DISABLE && rep_batt_chg.charging)
					||(usb_charger_enable == USB_CHG_ENABLE && !rep_batt_chg.charging)))
				{
					printk(KERN_INFO "Before RPC charging = %d, usb_charger_enable_pre = %d,usb_charger_enable = %d \n",
						rep_batt_chg.charging,usb_charger_enable_pre,usb_charger_enable);
					usb_charger_enable_pre = usb_charger_enable;
					msm_batt_handle_control_usb_charging(usb_charger_enable);
				}
			}
		else
	                msm_batt_info.batt_status = POWER_SUPPLY_STATUS_CHARGING;
		}
	#else
                msm_batt_info.batt_status = POWER_SUPPLY_STATUS_CHARGING;
	#endif
        }
        else if ((rep_batt_chg.charger_status == CHARGER_STATUS_BAD
                  || rep_batt_chg.charger_status == CHARGER_STATUS_INVALID) 
                  && (rep_batt_chg.battery_status == BATTERY_STATUS_GOOD))
        {
            msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_GOOD;
            msm_batt_info.batt_status = POWER_SUPPLY_STATUS_DISCHARGING;
        }

        //msm_batt_info.battery_capacity = msm_batt_info.calculate_capacity(msm_batt_info.battery_voltage);
    }
    else
    {
        printk("battery status invalid id\n");
        msm_batt_info.batt_health = POWER_SUPPLY_HEALTH_UNKNOWN;
        msm_batt_info.batt_status = POWER_SUPPLY_STATUS_UNKNOWN;
        msm_batt_info.battery_capacity = rep_batt_chg.battery_capacity;
        msm_batt_info.batt_valid = 0;        
    }

    if (msm_batt_info.charger_type != rep_batt_chg.charger_type)/*charger's type changes,insert or pull out*/
    {
        msm_batt_info.charger_type = rep_batt_chg.charger_type;

        if (msm_batt_info.charger_type == CHARGER_TYPE_WALL)/*AC insert*/
        {
            msm_batt_info.current_chg_source &= ~USB_CHG;
            msm_batt_info.current_chg_source |= AC_CHG;

            DEBUG_MSM_BATTERY(KERN_INFO "%s() : charger_type = WALL\n", __func__);

            power_supply_changed(&msm_psy_ac);

        }
        else if (msm_batt_info.charger_type ==
                 CHARGER_TYPE_USB_WALL || msm_batt_info.charger_type == CHARGER_TYPE_USB_PC)/*USB insert*/
        {
            msm_batt_info.current_chg_source &= ~AC_CHG;
            msm_batt_info.current_chg_source |= USB_CHG;

            DEBUG_MSM_BATTERY(KERN_INFO "%s() : charger_type = %s\n", __func__, charger_type[msm_batt_info.charger_type]);

            power_supply_changed(&msm_psy_usb);
            DEBUG_MSM_BATTERY();
        }
        else	/*pull out charger */
        {
            DEBUG_MSM_BATTERY(KERN_INFO "%s() : charger_type = %s\n", __func__, charger_type[msm_batt_info.charger_type]);

            msm_batt_info.batt_status = POWER_SUPPLY_STATUS_DISCHARGING;

            if (msm_batt_info.current_chg_source & AC_CHG)	/*the previous  charger is AC*/
            {
                msm_batt_info.current_chg_source &= ~AC_CHG;

                DEBUG_MSM_BATTERY(KERN_INFO "%s() : AC WALL charger" " removed\n", __func__);

                power_supply_changed(&msm_psy_ac);

            }
            else if (msm_batt_info.current_chg_source & USB_CHG)/*the previous  charger is USB*/
            {
                msm_batt_info.current_chg_source &= ~USB_CHG;
                DEBUG_MSM_BATTERY(KERN_INFO "%s() : USB charger removed\n", __func__);

#ifdef FEATURE_ZTE_APP_ENABLE_USB_CHARGING
			usb_charger_enable_pre = USB_CHG_ENABLE;
			printk(KERN_INFO "USB remove: usb_charger_enable_pre set2 %d \n",usb_charger_enable_pre);
#endif        

                power_supply_changed(&msm_psy_usb);
            }
            else	/*? unnecessary?*/
                power_supply_changed(&msm_psy_batt);
        }
    }
    else
    {
               power_supply_changed(&msm_psy_batt);
    }
	
    msm_batt_info.charger_status = rep_batt_chg.charger_status;
    msm_batt_info.charger_type = rep_batt_chg.charger_type;
    msm_batt_info.battery_status = rep_batt_chg.battery_status;
    msm_batt_info.battery_level = rep_batt_chg.battery_level;
    msm_batt_info.battery_voltage = rep_batt_chg.battery_voltage;
    msm_batt_info.battery_capacity = rep_batt_chg.battery_capacity;

    if (rep_batt_chg.battery_temp > BATTERY_MAX_TEMP &&
             msm_batt_info.battery_temp_exceeded_count < BATTERY_MAX_TEMP_EXCEEDED_COUNT)
    {
        msm_batt_info.battery_temp = -1;
        msm_batt_info.battery_temp_exceeded_count += 1;
        printk("%() : Max Battery Temperature (%d) Exceeded for %d successive time.", __func__, rep_batt_chg.battery_temp, msm_batt_info.battery_temp_exceeded_count);
    }
    else    
    {
        msm_batt_info.battery_temp = rep_batt_chg.battery_temp;
        msm_batt_info.battery_temp_exceeded_count = 0;
    }

    msm_batt_info.chg_fulled = rep_batt_chg.chg_fulled;
    msm_batt_info.charging = rep_batt_chg.charging;

    DEBUG_MSM_BATTERY();
}

#ifdef CONFIG_SCREEN_ON_WITHOUT_KEYOCDE
void msm_batt_force_update(void)
{
                if (wl_initialized)
                {
			printk("No pity! update battery event\n");
                        wake_lock_timeout(&charger_wake_lock, 3 * HZ);
                        msm_batt_update_psy_status_v1();
                }
                else
                        printk("What a pity! charger driver unintialized\n");

}
#endif

static int msm_batt_handle_suspend(void)
{
    return 0;
}

static int msm_batt_handle_resume(void)
{
    msm_batt_update_psy_status_v1();
    return 0;
}

static int msm_batt_handle_status_timeout(void)
{
    msm_batt_update_psy_status_v1();
    return 0;
}

static int msm_batt_handle_event(void)
{
    int rc;

    if (!atomic_read(&msm_batt_info.handle_event))
    {
        printk(KERN_ERR "%s(): batt call back thread while in "
               "msm_rpc_read got signal. Signal is not from "
               "early suspend or  from late resume or from Clean up " "thread.\n", __func__);
        return 0;
    }

    DEBUG_MSM_BATTERY(KERN_INFO "%s(): batt call back thread" "got signal\n", __func__);

    if (msm_batt_info.type_of_event & SUSPEND_EVENT)
    {
        DEBUG_MSM_BATTERY(KERN_INFO "%s(): Handle Suspend event. event = %08x\n", __func__, msm_batt_info.type_of_event);

        rc = msm_batt_handle_suspend();

        msm_batt_info.type_of_event &= ~SUSPEND_EVENT;

        return rc;
    }
    else if (msm_batt_info.type_of_event & RESUME_EVENT)
    {
        DEBUG_MSM_BATTERY(KERN_INFO "%s(): Handle Resume event. event = %08x\n", __func__, msm_batt_info.type_of_event);

        rc = msm_batt_handle_resume();

        msm_batt_info.type_of_event &= ~RESUME_EVENT;

        return rc;
    }
    else if (msm_batt_info.type_of_event & CLEANUP_EVENT)
    {
        DEBUG_MSM_BATTERY(KERN_INFO "%s(): Cleanup event occured. event = %08x\n", __func__, msm_batt_info.type_of_event);

        msm_batt_info.type_of_event &= ~CLEANUP_EVENT;

        return 0;
    }
    else
    {
        printk(KERN_ERR "%s(): Unknown event occured. event = %08x\n", __func__, msm_batt_info.type_of_event);
        return 0;
    }
}


static void msm_batt_wake_up_waiting_thread(u32 event)
{
    DEBUG_MSM_BATTERY();

    DEBUG_MSM_BATTERY("wake up wait queue!\n");
    atomic_set(&msm_batt_info.handle_event, 1);
    wake_up(&msm_batt_info.wait_q);

    DEBUG_MSM_BATTERY();
}


static void msm_batt_wait_for_batt_chg_event(struct work_struct *work)
{
    int ret;
    int rc;

    DEBUG_MSM_BATTERY(KERN_INFO "%s: Batt status call back thread started.(SMEM)\n", __func__);

    DEBUG_MSM_BATTERY(KERN_INFO "%s: First time Update Batt status without waiting for"
           " call back event from modem .\n", __func__);

    msm_batt_update_psy_status_v1();
    while (1)
    {
        ret = wait_event_interruptible_timeout(msm_batt_info.wait_q,
                                               msm_batt_info.type_of_event & EVENT_CONDITION, BATTERY_STATUS_POLL_TIME);

        DEBUG_MSM_BATTERY("ret = %d", ret);
        DEBUG_MSM_BATTERY("%s: %d\n", __func__, __LINE__);

        if (ret != 0)
        {
            rc = msm_batt_handle_event();

            if (rc != 0)
            {
                printk("%s: handle event error!\n", __func__);
            }

            if (msm_batt_info.type_of_event & EVENT_CONDITION)
            {
                DEBUG_MSM_BATTERY();

                continue;
            }

            DEBUG_MSM_BATTERY();

            atomic_set(&msm_batt_info.handle_event, 0);
        }
        else
        {
            DEBUG_MSM_BATTERY();

            msm_batt_handle_status_timeout();
        }
    }
    DEBUG_MSM_BATTERY(KERN_INFO "%s: Batt status call back thread stopped.(SMEM)\n", __func__);
}

static int msm_batt_send_event(u32 type_of_event)
{
    int rc;
    unsigned long flags;

    rc = 0;

    spin_lock_irqsave(&msm_batt_info.lock, flags);

    if (type_of_event & SUSPEND_EVENT)
        DEBUG_MSM_BATTERY(KERN_INFO "%s() : Suspend event ocurred." "events = %08x\n", __func__, type_of_event);
    else if (type_of_event & RESUME_EVENT)
        DEBUG_MSM_BATTERY(KERN_INFO "%s() : Resume event ocurred." "events = %08x\n", __func__, type_of_event);
    else if (type_of_event & CLEANUP_EVENT)
        DEBUG_MSM_BATTERY(KERN_INFO "%s() : Cleanup event ocurred." "events = %08x\n", __func__, type_of_event);
    else
    {
        DEBUG_MSM_BATTERY(KERN_ERR "%s() : Unknown event ocurred." "events = %08x\n", __func__, type_of_event);

        spin_unlock_irqrestore(&msm_batt_info.lock, flags);
        return -EIO;
    }

    msm_batt_info.type_of_event |= type_of_event;

    if (msm_batt_info.type_of_event & CLEANUP_EVENT)
    {
        msm_batt_wake_up_waiting_thread(CLEANUP_EVENT);
    }
    else if (msm_batt_info.type_of_event & SUSPEND_EVENT)
    {
        msm_batt_wake_up_waiting_thread(SUSPEND_EVENT);
    }
    else if (msm_batt_info.type_of_event & RESUME_EVENT)
    {
        msm_batt_wake_up_waiting_thread(RESUME_EVENT);
    }
    else
    {
        
    }

    spin_unlock_irqrestore(&msm_batt_info.lock, flags);

    return rc;
}

static void msm_batt_start_cb_thread(void)
{
    DEBUG_MSM_BATTERY("task queue start!\n");
    atomic_set(&msm_batt_info.handle_event, 0);
    queue_work(msm_batt_info.msm_batt_wq, &msm_batt_cb_work);
    DEBUG_MSM_BATTERY("task queue end!\n");
}

static void msm_batt_early_suspend(struct early_suspend *h);

static int msm_batt_cleanup(void)
{
    int rc = 0;

    if (msm_batt_info.msm_batt_wq)
    {
        msm_batt_send_event(CLEANUP_EVENT);
        destroy_workqueue(msm_batt_info.msm_batt_wq);
    }

    if (msm_batt_info.msm_psy_ac)
        power_supply_unregister(msm_batt_info.msm_psy_ac);
    if (msm_batt_info.msm_psy_usb)
        power_supply_unregister(msm_batt_info.msm_psy_usb);
    if (msm_batt_info.msm_psy_batt)
        power_supply_unregister(msm_batt_info.msm_psy_batt);

#ifdef CONFIG_HAS_EARLYSUSPEND
    if (msm_batt_info.early_suspend.suspend == msm_batt_early_suspend)
        unregister_early_suspend(&msm_batt_info.early_suspend);
#endif
    return rc;
}

static u32 msm_batt_capacity(u32 current_voltage)
{
    u32 low_voltage = msm_batt_info.voltage_min_design;
    u32 high_voltage = msm_batt_info.voltage_max_design;

    return (current_voltage - low_voltage) * 100 / (high_voltage - low_voltage);
}

#ifdef CONFIG_HAS_EARLYSUSPEND
void msm_batt_early_suspend(struct early_suspend *h)
{
    DEBUG_MSM_BATTERY(KERN_INFO "%s(): going to early suspend\n", __func__);
    //msm_batt_send_event(SUSPEND_EVENT);
}

void msm_batt_late_resume(struct early_suspend *h)
{
    DEBUG_MSM_BATTERY(KERN_INFO "%s(): going to resume\n", __func__);
    //msm_batt_send_event(RESUME_EVENT);
}
#endif
static int msm_batt_suspend(struct platform_device *pdev, pm_message_t state)
{
    msm_batt_handle_suspend();
    return 0;
}

static int msm_batt_resume(struct platform_device *pdev)
{
    msm_batt_handle_resume();
    return 0;
}

static int __devinit msm_batt_probe(struct platform_device *pdev)
{
    int rc;
    struct msm_psy_batt_pdata *pdata = pdev->dev.platform_data;

    if (pdev->id != -1)
    {
        dev_err(&pdev->dev, "%s: MSM chipsets Can only support one" " battery ", __func__);
        return -EINVAL;
    }

    if (pdata->avail_chg_sources & AC_CHG)
    {
        rc = power_supply_register(&pdev->dev, &msm_psy_ac);
        if (rc < 0)
        {
            dev_err(&pdev->dev, "%s: power_supply_register failed" " rc = %d\n", __func__, rc);
            msm_batt_cleanup();
            return rc;
        }
        msm_batt_info.msm_psy_ac = &msm_psy_ac;
        msm_batt_info.avail_chg_sources |= AC_CHG;
    }

    if (pdata->avail_chg_sources & USB_CHG)
    {
        rc = power_supply_register(&pdev->dev, &msm_psy_usb);
        if (rc < 0)
        {
            dev_err(&pdev->dev, "%s: power_supply_register failed" " rc = %d\n", __func__, rc);
            msm_batt_cleanup();
            return rc;
        }
        msm_batt_info.msm_psy_usb = &msm_psy_usb;
        msm_batt_info.avail_chg_sources |= USB_CHG;
    }

    if (!msm_batt_info.msm_psy_ac && !msm_batt_info.msm_psy_usb)
    {

        dev_err(&pdev->dev, "%s: No external Power supply(AC or USB)" "is avilable\n", __func__);
        msm_batt_cleanup();
        return -ENODEV;
    }

    msm_batt_info.voltage_max_design = pdata->voltage_max_design;
    msm_batt_info.voltage_min_design = pdata->voltage_min_design;
    msm_batt_info.batt_technology = pdata->batt_technology;
    msm_batt_info.calculate_capacity = pdata->calculate_capacity;

    if (!msm_batt_info.voltage_min_design)
        msm_batt_info.voltage_min_design = BATTERY_LOW;
    if (!msm_batt_info.voltage_max_design)
        msm_batt_info.voltage_max_design = BATTERY_HIGH;

    if (msm_batt_info.batt_technology == POWER_SUPPLY_TECHNOLOGY_UNKNOWN)
        msm_batt_info.batt_technology = POWER_SUPPLY_TECHNOLOGY_LION;

    if (!msm_batt_info.calculate_capacity)
        msm_batt_info.calculate_capacity = msm_batt_capacity;

    rc = power_supply_register(&pdev->dev, &msm_psy_batt);
    if (rc < 0)
    {
        dev_err(&pdev->dev, "%s: power_supply_register failed" " rc=%d\n", __func__, rc);
        msm_batt_cleanup();
        return rc;
    }
    msm_batt_info.msm_psy_batt = &msm_psy_batt;

#ifdef CONFIG_HAS_EARLYSUSPEND
    msm_batt_info.early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
    msm_batt_info.early_suspend.suspend = msm_batt_early_suspend;
    msm_batt_info.early_suspend.resume = msm_batt_late_resume;
    register_early_suspend(&msm_batt_info.early_suspend);
#endif

    msm_batt_start_cb_thread();

    return 0;
}



static struct platform_driver msm_batt_driver;
static int __devinit msm_batt_init_smem(void)
{
    int rc;

    spin_lock_init(&msm_batt_info.lock);

    init_waitqueue_head(&msm_batt_info.wait_q);

    msm_batt_info.msm_batt_wq = create_singlethread_workqueue("msm_battery");

    if (!msm_batt_info.msm_batt_wq)
    {
        printk(KERN_ERR "%s: create workque failed \n", __func__);
        return -ENOMEM;
    }

    rc = platform_driver_register(&msm_batt_driver);

    if (rc < 0)
    {
        printk(KERN_ERR "%s: platform_driver_register failed for " "batt driver. rc = %d\n", __func__, rc);
        return rc;
    }

    return 0;
}

static int __devexit msm_batt_remove(struct platform_device *pdev)
{
    int rc;

    rc = msm_batt_cleanup();

    if (rc < 0)
    {
        dev_err(&pdev->dev, "%s: msm_batt_cleanup  failed rc=%d\n", __func__, rc);
        return rc;
    }
    return 0;
}

static struct platform_driver msm_batt_driver = {
    .probe = msm_batt_probe,
    .remove = __devexit_p(msm_batt_remove),
    .driver = {
               .name = "msm-battery",
               .owner = THIS_MODULE,
               },
    .suspend = msm_batt_suspend,
    .resume = msm_batt_resume,
};

static int __init msm_batt_init(void)
{
    int rc;

    rc = msm_batt_init_smem();

    if (rc < 0)
    {
        printk(KERN_ERR "%s: msm_batt_init_rpc Failed  rc=%d\n", __func__, rc);
        msm_batt_cleanup();
        return rc;
    }
#ifdef CONFIG_SCREEN_ON_WITHOUT_KEYOCDE
	wake_lock_init(&charger_wake_lock, WAKE_LOCK_SUSPEND, "chg_event");
	wl_initialized = 1;
#endif

#ifdef FEATURE_ZTE_APP_ENABLE_USB_CHARGING
	msm_batt_info.chg_ep =
	    msm_rpc_connect_compatible(CHG_RPC_PROG, CHG_RPC_VERS, 0);

	if (msm_batt_info.chg_ep == NULL) {
		return -ENODEV;
	} else if (IS_ERR(msm_batt_info.chg_ep)) {
		int rc = PTR_ERR(msm_batt_info.chg_ep);
		printk(KERN_ERR
		       "%s: rpc connect failed for CHG_RPC_PROG. rc = %d\n",
		       __func__, rc);
		msm_batt_info.chg_ep = NULL;
		return rc;
	}
#endif

    return 0;
}

static void __exit msm_batt_exit(void)
{
#ifdef CONFIG_SCREEN_ON_WITHOUT_KEYOCDE
	wl_initialized = 0;
	wake_lock_destroy(&charger_wake_lock);
#endif
    platform_driver_unregister(&msm_batt_driver);
}

module_init(msm_batt_init);
module_exit(msm_batt_exit);

MODULE_LICENSE("Dual BSD/GPL");
MODULE_AUTHOR("Kiran Kandi, Qualcomm Innovation Center, Inc.");
MODULE_DESCRIPTION("Battery driver for Qualcomm MSM chipsets.");
MODULE_VERSION("1.0");
MODULE_ALIAS("platform:msm_battery");
#endif
