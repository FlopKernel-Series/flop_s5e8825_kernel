/*
 * Copyrights (C) 2017 Samsung Electronics, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/battery/sec_pd.h>

#include <linux/battery/sb_sysfs.h>

#include "sec_pd.h"
#include "sec_pd_data.h"

static ssize_t show_attrs(struct device *dev,
	struct device_attribute *attr, char *buf);
static ssize_t store_attrs(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count);

#define PD_SYSFS_ATTR(_name)						\
{									\
	.attr = {.name = #_name, .mode = 0664},	\
	.show = show_attrs,					\
	.store = store_attrs,					\
}

static struct device_attribute pd_attr[] = {
	PD_SYSFS_ATTR(pd_op_mode),
};

enum sec_pd_attrs {
	PD_OP_MODE = 0,
};

static ssize_t show_attrs(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	const ptrdiff_t offset = attr - pd_attr;
	ssize_t count = 0;

	switch (offset) {
	case PD_OP_MODE:
		count = scnprintf(buf, PAGE_SIZE, "%s\n", get_sec_pd_op_name(get_sec_pd()));
		break;
	default:
		break;
	}

	return count;
}

static ssize_t store_attrs(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	const ptrdiff_t offset = attr - pd_attr;

	switch (offset) {
	case PD_OP_MODE:
	{
		int x = 0;

		if (sscanf(buf, "%10d\n", &x) == 1)
			change_sec_pd(x);
	}
		break;
	default:
		break;
	}

	return count;
}

int sec_pd_eng_init(struct sec_pd *pd)
{
	int ret = 0;

	ret = sb_sysfs_add_attrs("sec_pd_eng", pd, pd_attr, ARRAY_SIZE(pd_attr));
	pr_info("%s: sysfs ret = %s\n", __func__, (ret) ? "fail" : "success");

	return 0;
}
EXPORT_SYMBOL(sec_pd_eng_init);
