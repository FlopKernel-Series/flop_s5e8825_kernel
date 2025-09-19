/*
 *  sb_tx.c
 *  Samsung Mobile Wireless TX Driver
 *
 *  Copyright (C) 2021 Samsung Electronics
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/slab.h>
#include <linux/mutex.h>

#if !defined(CONFIG_ARCH_MEDIATEK)
#include <linux/hall/hall_ic_notifier.h>
#else
#define hall_notifier_register(nb)		(-EOPNOTSUPP)
#endif

#include "sec_battery.h"
#include "sb_mag.h"

#define mag_log(str, ...) pr_info("[SB-MAG]:%s: "str, __func__, ##__VA_ARGS__)

struct sb_mag {
	struct sec_battery_info *battery;
	struct notifier_block nb;
	struct mutex lock;

	union {
		unsigned long long state;

		struct {
			unsigned	hall_ic : 1,
						sysfs : 1;
		} states;
	};
};
static struct sb_mag *g_mag;

static int mag_set_psy(char *name, int prop, int data)
{
	union power_supply_propval value = { data, };

	return psy_do_property(name, set, prop, value);
}

static int mag_get_psy(char *name, int prop, int *data)
{
	union power_supply_propval value = { *data, };
	int ret = 0;

	ret = psy_do_property(name, get, prop, value);
	if (!ret)
		*data = value.intval;

	return ret;
}

#if !defined(CONFIG_ARCH_MEDIATEK)
static void set_mag_state_hall_ic(struct sb_mag *mag, bool state)
{
	struct sec_battery_info *battery = mag->battery;

	mutex_lock(&mag->lock);
	mag->states.hall_ic = state;
	mag_set_psy(battery->pdata->wireless_charger_name, POWER_SUPPLY_EXT_PROP_MPP_COVER, !!(mag->state));
	mutex_unlock(&mag->lock);
}
#endif

static void set_mag_state_sysfs(struct sb_mag *mag, bool state)
{
	struct sec_battery_info *battery = mag->battery;

	mutex_lock(&mag->lock);
	mag->states.sysfs = state;
	mag_set_psy(battery->pdata->wireless_charger_name, POWER_SUPPLY_EXT_PROP_MPP_COVER, !!(mag->state));
	mutex_unlock(&mag->lock);
}

static int get_mag_state(struct sb_mag *mag)
{
	int ret = 0;

	mutex_lock(&mag->lock);
	ret = !!mag->state;
	mutex_unlock(&mag->lock);

	return ret;
}

static ssize_t show_attrs(struct device *, struct device_attribute *, char *);
static ssize_t store_attrs(struct device *, struct device_attribute *, const char *, size_t);

#define SB_MAG_ATTR(_name) \
{ \
	.attr = { \
		.attr = {.name = #_name, .mode = 0664}, \
		.show = show_attrs, \
		.store = store_attrs, \
	}, \
}

static struct dev_ext_attribute mag_sysfs[] = {
	SB_MAG_ATTR(mag_cover),
	SB_MAG_ATTR(mag_cloak),
	SB_MAG_ATTR(mag_value),
};

enum {
	MAG_COVER = 0,
	MAG_CLOAK,
	MAG_VALUE,
};

static ssize_t show_attrs(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct dev_ext_attribute *ext_attr = container_of(attr, struct dev_ext_attribute, attr);
	const ptrdiff_t offset = ext_attr - mag_sysfs;
	struct sb_mag *mag = ext_attr->var;
	int i = 0;

	switch (offset) {
	case MAG_COVER:
		i += scnprintf(buf, PAGE_SIZE, "%d\n", get_mag_state(mag));
		break;
	case MAG_CLOAK:
		i += scnprintf(buf, PAGE_SIZE, "%s\n", "None");
		break;
	case MAG_VALUE:
	{
		struct sec_battery_info *battery = mag->battery;
		int mag_value = 0;

		mag_get_psy(battery->pdata->wireless_charger_name, POWER_SUPPLY_EXT_PROP_MPP_VALUE, &mag_value);
		i += scnprintf(buf, PAGE_SIZE, "%d\n", mag_value);
	}
		break;
	default:
		return -EINVAL;
	}

	return i;
}

static ssize_t store_attrs(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct dev_ext_attribute *ext_attr = container_of(attr, struct dev_ext_attribute, attr);
	const ptrdiff_t offset = ext_attr - mag_sysfs;
	struct sb_mag *mag = ext_attr->var;
	int x = 0, y = 0;

	switch (offset) {
	case MAG_COVER:
		if (sscanf(buf, "%10d\n", &x) == 1) {
			mag_log("@MPP: update mag_cover(%d)\n", x);

			set_mag_state_sysfs(mag, !!x);
		}
		break;
	case MAG_CLOAK:
		if (sscanf(buf, "%10d %10d\n", &x, &y) == 2) {
			int cloak_cmd = x;

			mag_log("@MPP: update mag_cloak(%d) - reason(%d)\n", x, y);

			if (x == 0)
				cloak_cmd = CLOAK_EXIT_CMD;
			else if (x != 0 && y == 5) /* MFC_TRX_MPP_CLOAK_END_OF_CHARGE*/
				cloak_cmd = CLOAK_END_OF_CHARGE;
			else if (x != 0 && y == 6) /* MFC_TRX_MPP_CLOAK_PTX_INITIATED*/
				cloak_cmd = CLOAK_PTX_INITIATED;
			mag_set_psy("wireless", POWER_SUPPLY_EXT_PROP_MPP_CLOAK, cloak_cmd);
		}
		break;
	case MAG_VALUE:
		break;
	default:
		return -EINVAL;
	}

	return count;
}

static int create_attrs(struct sb_mag *mag, struct device *dev)
{
	unsigned int i = 0;
	int ret = 0;

	for (i = 0; i < ARRAY_SIZE(mag_sysfs); i++) {
		ret = device_create_file(dev, &mag_sysfs[i].attr);
		if (ret)
			goto fail_attr;

		mag_sysfs[i].var = mag;
	}

	return 0;

fail_attr:
	while (i--)
		device_remove_file(dev, &mag_sysfs[i].attr);
	return ret;
}

static int hall_ic_notifier(struct notifier_block *nb, unsigned long action, void *data)
{
#if !defined(CONFIG_ARCH_MEDIATEK)
	struct sb_mag *mag = container_of(nb, struct sb_mag, nb);
	struct hall_notifier_context *context = (struct hall_notifier_context *)data;

	if ((!context) ||
		(strncmp(context->name, "back_cover", 10) != 0))
		return 0;

	mag_log("name:%s, hall_ic:%s\n", context->name, (action ? "CLOSE" : "OPEN"));
	set_mag_state_hall_ic(mag, !!action);
#endif
	return 0;
}

static int register_notifier(struct sb_mag *mag)
{
	struct device_node *np;
	bool noti_flag;

	np = of_find_node_by_name(NULL, "battery");
	if (!np) {
		mag_log("failed to find battery node!!\n");
		return -ENODEV;
	}

	noti_flag = of_property_read_bool(np, "battery,hall_ic_notifier");
	if (!noti_flag) {
		mag_log("hall ic notifier is not set!\n");
		return 0;
	}

	mag->nb.notifier_call = hall_ic_notifier;
	mag->nb.priority = 1;
	return hall_notifier_register(&mag->nb);
}

int sb_mag_init(struct sec_battery_info *battery)
{
	struct sb_mag *mag;
	int ret = 0;

	if (IS_ERR_OR_NULL(battery))
		return -EINVAL;

	mag = kzalloc(sizeof(struct sb_mag), GFP_KERNEL);
	if (!mag)
		return -ENOMEM;

	g_mag = mag;

	mutex_init(&mag->lock);
	mag->battery = battery;
	mag->state = 0;

	ret = create_attrs(mag, &battery->psy_bat->dev);
	if (ret)
		mag_log("failed to create attrs(%d)\n", ret);

	ret = register_notifier(mag);
	if (ret < 0)
		mag_log("failed to register noti(%d)\n", ret);

	mag_log("finish!!\n");
	return 0;
}
EXPORT_SYMBOL(sb_mag_init);

int sb_mag_get_state(int *state)
{
	if (IS_ERR_OR_NULL(g_mag))
		return -ENODEV;

	*state = get_mag_state(g_mag);
	return 0;
}
EXPORT_SYMBOL(sb_mag_get_state);
