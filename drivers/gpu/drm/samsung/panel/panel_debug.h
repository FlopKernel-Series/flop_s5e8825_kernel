/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __PANEL_DEBUG_H__
#define __PANEL_DEBUG_H__

#include <linux/printk.h>
#include <linux/sec_debug.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include "util.h"

#ifndef PANEL_PR_TAG
#define PANEL_PR_TAG	"drv"
#endif

#define PANEL_PR_PREFIX	"panel-"
#define PANEL_DEV_PR_PREFIX	"panel%d-"

extern int panel_log_level;
extern int panel_cmd_log;

#define panel_err(fmt, ...)							\
	do {									\
		if (panel_log_level >= 3)					\
			pr_err(pr_fmt(PANEL_PR_PREFIX PANEL_PR_TAG ":E:%s:%d: " fmt), __func__, __LINE__, ##__VA_ARGS__);			\
	} while (0)

#define panel_warn(fmt, ...)							\
	do {									\
		if (panel_log_level >= 4)					\
			pr_warn(pr_fmt(PANEL_PR_PREFIX PANEL_PR_TAG ":W:%s:%d: " fmt), __func__, __LINE__, ##__VA_ARGS__);			\
	} while (0)

#define panel_info(fmt, ...)							\
	do {									\
		if (panel_log_level >= 6)					\
			pr_info(pr_fmt(PANEL_PR_PREFIX PANEL_PR_TAG ":I:%s: " fmt), __func__, ##__VA_ARGS__);			\
	} while (0)

#define panel_dbg(fmt, ...)							\
	do {									\
		if (panel_log_level >= 7)					\
			pr_info(pr_fmt(PANEL_PR_PREFIX PANEL_PR_TAG ":D:%s: " fmt), __func__, ##__VA_ARGS__);			\
	} while (0)


#define panel_ext_err(_tag_, fmt, ...)							\
	do {									\
		if (panel_log_level >= 3)					\
			pr_err(pr_fmt(PANEL_PR_PREFIX "%s:E:%s: " fmt), (_tag_), __func__, ##__VA_ARGS__);			\
	} while (0)

#define panel_ext_warn(_tag_, fmt, ...)							\
	do {									\
		if (panel_log_level >= 4)					\
			pr_warn(pr_fmt(PANEL_PR_PREFIX "%s:W:%s: " fmt), (_tag_), __func__, ##__VA_ARGS__);			\
	} while (0)

#define panel_ext_info(_tag_, fmt, ...)							\
	do {									\
		if (panel_log_level >= 6)					\
			pr_info(pr_fmt(PANEL_PR_PREFIX "%s:I:%s: " fmt), (_tag_), __func__, ##__VA_ARGS__);			\
	} while (0)

#define panel_ext_dbg(_tag_, fmt, ...)							\
	do {									\
		if (panel_log_level >= 7)					\
			pr_info(pr_fmt(PANEL_PR_PREFIX "%s:D:%s: " fmt), (_tag_), __func__, ##__VA_ARGS__);			\
	} while (0)


#define panel_dev_err(panel_dev, fmt, ...)							\
	do {									\
		if (panel_log_level >= 3)					\
			pr_err(pr_fmt(PANEL_DEV_PR_PREFIX PANEL_PR_TAG ":E:%s: " fmt), \
					(panel_dev) ? (panel_dev)->id : 0, \
					__func__, ##__VA_ARGS__);			\
	} while (0)

#define panel_dev_warn(panel_dev, fmt, ...)							\
	do {									\
		if (panel_log_level >= 4)					\
			pr_warn(pr_fmt(PANEL_DEV_PR_PREFIX PANEL_PR_TAG ":W:%s: " fmt), \
					(panel_dev) ? (panel_dev)->id : 0, \
					__func__, ##__VA_ARGS__);			\
	} while (0)

#define panel_dev_info(panel_dev, fmt, ...)							\
	do {									\
		if (panel_log_level >= 6)					\
			pr_info(pr_fmt(PANEL_DEV_PR_PREFIX PANEL_PR_TAG ":I:%s: " fmt), \
					(panel_dev) ? (panel_dev)->id : 0, \
					__func__, ##__VA_ARGS__);			\
	} while (0)

#define panel_dev_dbg(panel_dev, fmt, ...)							\
	do {									\
		if (panel_log_level >= 7)					\
			pr_info(pr_fmt(PANEL_DEV_PR_PREFIX PANEL_PR_TAG ":D:%s: " fmt), \
					(panel_dev) ? (panel_dev)->id : 0, \
					__func__, ##__VA_ARGS__);			\
	} while (0)

#define panel_cmd_log_enabled(_x_)	((panel_cmd_log) & (1 << (_x_)))

/*
 * debug level low return 0.
 * debug level mid or high return over 0.
 */
static inline int panel_debug_level(void)
{
#if IS_ENABLED(CONFIG_DRM_MEDIATEK_V2)
	return !is_debug_level_low();
#else
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 6, 0)
	return sec_debug_get_force_upload();
#else
	return secdbg_mode_enter_upload();
#endif
#endif
}

#define PANEL_BUG() \
	do { \
		if (!panel_debug_level()) \
			panel_err("PANEL_BUG detected\n"); \
		else \
			BUG(); \
	} while (0)

#define PANEL_BUG_ON(cond) do { if (unlikely(cond)) PANEL_BUG(); } while (0)

#endif /* __PANEL_DEBUG_H__ */
