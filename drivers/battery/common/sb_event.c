/*
 *  sb_event.c
 *  Samsung Mobile Battery Event Module
 *
 *  Copyright (C) 2025 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "sb_event.h"
#include "sec_battery.h"

#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/debugfs.h>

const char *sb_get_misc_event_str(int event_val)
{
	switch (event_val) {
	case BATT_MISC_EVENT_UNDEFINED_RANGE_TYPE:
		return "UNDEFINED_RANGE_TYPE";
	case BATT_MISC_EVENT_WIRELESS_BACKPACK_TYPE:
		return "WIRELESS_BACKPACK_TYPE";
	case BATT_MISC_EVENT_TIMEOUT_OPEN_TYPE:
		return "TIMEOUT_OPEN_TYPE";
	case BATT_MISC_EVENT_BATT_RESET_SOC:
		return "BATT_RESET_SOC";
	case BATT_MISC_EVENT_WATER_HICCUP_TYPE:
		return "WATER_HICCUP_TYPE";
	case BATT_MISC_EVENT_WIRELESS_DET_LEVEL:
		return "WIRELESS_DET_LEVEL";
	case BATT_MISC_EVENT_WIRELESS_FOD:
		return "WIRELESS_FOD";
	case BATT_MISC_EVENT_WIRELESS_AUTH_START:
		return "WIRELESS_AUTH_START";
	case BATT_MISC_EVENT_WIRELESS_AUTH_RECVED:
		return "WIRELESS_AUTH_RECVED";
	case BATT_MISC_EVENT_WIRELESS_AUTH_FAIL:
		return "WIRELESS_AUTH_FAIL";
	case BATT_MISC_EVENT_WIRELESS_AUTH_PASS:
		return "WIRELESS_AUTH_PASS";
	case BATT_MISC_EVENT_TEMP_HICCUP_TYPE:
		return "TEMP_HICCUP_TYPE";
	case BATT_MISC_EVENT_DIRECT_POWER_MODE:
		return "DIRECT_POWER_MODE";
	case BATT_MISC_EVENT_WIRELESS_OVERHEAT:
		return "WIRELESS_OVERHEAT";
	case BATT_MISC_EVENT_BATTERY_HEALTH:
		return "BATTERY_HEALTH";
	case BATT_MISC_EVENT_HEALTH_OVERHEATLIMIT:
		return "HEALTH_OVERHEATLIMIT";
	// case BATT_MISC_EVENT_ABNORMAL_PAD:
	//	return "ABNORMAL_PAD";
	case BATT_MISC_EVENT_WIRELESS_MISALIGN:
		return "WIRELESS_MISALIGN";
	case BATT_MISC_EVENT_FULL_CAPACITY:
		return "FULL_CAPACITY";
	case BATT_MISC_EVENT_PASS_THROUGH:
		return "PASS_THROUGH";
	case BATT_MISC_EVENT_MAIN_POWERPATH:
		return "MAIN_POWERPATH";
	case BATT_MISC_EVENT_SUB_POWERPATH:
		return "SUB_POWERPATH";
	case BATT_MISC_EVENT_HV_BY_AICL:
		return "EVENT_HV_BY_AICL";
	case BATT_MISC_EVENT_WC_JIG_PAD:
		return "WC_JIG_PAD";
	default:
		return "MULTIPLE";
	}
}

const char *sb_get_current_event_str(int event_val)
{
	switch (event_val) {
	case SEC_BAT_CURRENT_EVENT_NONE:
		return "NONE";
	case SEC_BAT_CURRENT_EVENT_AFC:
		return "AFC";
	case SEC_BAT_CURRENT_EVENT_CHARGE_DISABLE:
		return "CHARGE_DISABLE";
	case SEC_BAT_CURRENT_EVENT_SKIP_HEATING_CONTROL:
		return "SKIP_HEATING_CONTROL";
	case SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_COOL1:
		return "LOW_TEMP_SWELLING_COOL1";
	case SEC_BAT_CURRENT_EVENT_HIGH_TEMP_SWELLING:
		return "HIGH_TEMP_SWELLING";
	case SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_COOL2:
		return "LOW_TEMP_SWELLING_COOL2";
	case SEC_BAT_CURRENT_EVENT_LOW_TEMP_SWELLING_COOL3:
		return "LOW_TEMP_SWELLING_COOL3";
	case SEC_BAT_CURRENT_EVENT_SWELLING_MODE:
		return "SWELLING_MODE";
	case SEC_BAT_CURRENT_EVENT_LOW_TEMP_MODE:
		return "LOW_TEMP_MODE";
	case SEC_BAT_CURRENT_EVENT_CHG_LIMIT:
		return "CHG_LIMIT";
	case SEC_BAT_CURRENT_EVENT_CALL:
		return "CALL";
	case SEC_BAT_CURRENT_EVENT_SLATE:
		return "SLATE";
	case SEC_BAT_CURRENT_EVENT_VBAT_OVP:
		return "VBAT_OVP";
	case SEC_BAT_CURRENT_EVENT_VSYS_OVP:
		return "VSYS_OVP";
	case SEC_BAT_CURRENT_EVENT_WPC_VOUT_LOCK:
		return "WPC_VOUT_LOCK";
	case SEC_BAT_CURRENT_EVENT_AICL:
		return "AICL";
	case SEC_BAT_CURRENT_EVENT_HV_DISABLE:
		return "HV_DISABLE";
	case SEC_BAT_CURRENT_EVENT_SELECT_PDO:
		return "SELECT_PDO";
	case SEC_BAT_CURRENT_EVENT_FG_RESET:
		return "FG_RESET";
	case SEC_BAT_CURRENT_EVENT_WDT_EXPIRED:
		return "WDT_EXPIRED";
	case SEC_BAT_CURRENT_EVENT_NOPD_HV_DISABLE:
		return "NOPD_HV_DISABLE";
#if defined(CONFIG_ISDB_CHARGING_CONTROL)
	case SEC_BAT_CURRENT_EVENT_ISDB:
		return "ISDB";
#endif
	case SEC_BAT_CURRENT_EVENT_DC_ERR:
		return "DC_ERR";
	case SEC_BAT_CURRENT_EVENT_SIOP_LIMIT:
		return "SIOP_LIMIT";
	case SEC_BAT_CURRENT_EVENT_TEMP_CTRL_TEST:
		return "TEMP_CTRL_TEST";
	case SEC_BAT_CURRENT_EVENT_25W_OCP:
		return "25W_OCP";
	case SEC_BAT_CURRENT_EVENT_AFC_DISABLE:
		return "AFC_DISABLE";
	case SEC_BAT_CURRENT_EVENT_SEND_UVDM:
		return "SEND_UVDM";
	case SEC_BAT_CURRENT_EVENT_USB_SUSPENDED:
		return "USB_SUSPENDED";
	case SEC_BAT_CURRENT_EVENT_USB_SUPER:
		return "USB_SUPER";
	case SEC_BAT_CURRENT_EVENT_USB_100MA:
		return "USB_100MA";
	case SEC_BAT_CURRENT_EVENT_USB_STATE:
		return "USB_STATE";
	case SEC_BAT_CURRENT_EVENT_WPC_EN:
		return "WPC_EN";
	default:
		return "MULTIPLE";
	}
}

void _sb_set_misc_event(struct sec_battery_info *battery, unsigned int event_val, unsigned int event_mask, const char *fname, int line)
{
	unsigned int prev_misc_event = battery->misc_event;
	unsigned int diff = 0;

	mutex_lock(&battery->misclock);

	battery->misc_event &= ~event_mask;
	battery->misc_event |= event_val;

	diff = prev_misc_event ^ battery->misc_event; // XOR to get the difference in bits
	if (diff) {
		const char *event_str = sb_get_misc_event_str(diff); // Get Name of the Difference
		bool event_set = battery->misc_event > prev_misc_event; // An Event was set

		pr_info("%s(%s:%d): %s %s Before(0x%x) -> After(0x%x)\n", __func__, fname, line,
			event_set ? "set" : "unset", event_str, prev_misc_event, battery->misc_event);
	} else { // No Change
		pr_info("%s(%s:%d): Unchanged (0x%x)\n", __func__, fname, line, battery->misc_event);
	}
	mutex_unlock(&battery->misclock);

	if (battery->prev_misc_event != battery->misc_event) {
		cancel_delayed_work(&battery->misc_event_work);
		__pm_stay_awake(battery->misc_event_ws);
		queue_delayed_work(battery->monitor_wqueue, &battery->misc_event_work, 0);
	}
}
EXPORT_SYMBOL(_sb_set_misc_event);

void _sb_set_current_event(struct sec_battery_info *battery, unsigned int event_val, unsigned int event_mask, const char *fname, int line)
{
	unsigned int prev_current_event = battery->current_event;
	unsigned int diff = 0;

	mutex_lock(&battery->current_eventlock);

	battery->current_event &= ~event_mask;
	battery->current_event |= event_val;

	diff = prev_current_event ^ battery->current_event; // XOR to get the difference in bits
	if (diff) {
		const char *event_str = sb_get_current_event_str(diff); // Get Name of the Difference
		bool event_set = battery->current_event > prev_current_event; // An Event was set

		pr_info("%s(%s:%d): %s %s Before(0x%x) -> After(0x%x)\n", __func__, fname, line,
			event_set ? "set" : "unset", event_str, prev_current_event, battery->current_event);
	} else { // No Change
		pr_info("%s(%s:%d): Unchanged (0x%x)\n", __func__, fname, line, battery->current_event);
	}
	mutex_unlock(&battery->current_eventlock);
}
EXPORT_SYMBOL(_sb_set_current_event);