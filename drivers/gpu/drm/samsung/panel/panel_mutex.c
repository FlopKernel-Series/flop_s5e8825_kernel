// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/device.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>

#include "panel_drv.h"
#include "panel_debug.h"
#include "panel_mutex.h"

void panel_mutex_init(void *ctx, struct panel_mutex *panel_lock)
{
	if (!ctx)
		return;

	panel_lock->ctx = ctx;
	panel_lock->magic = PANEL_MUTEX_MAGIC;
	mutex_init(&panel_lock->base);
}

void panel_mutex_lock(struct panel_mutex *panel_lock)
{
	static struct panel_device *panel;
#if IS_ENABLED(CONFIG_USDM_PANEL_BIG_LOCK)
	int commit_thread_pid;
	int panel_mutex_depth_before;
	int panel_mutex_depth_after;
	bool locked;
	int pid;
#endif

	if (!panel)
		panel = panel_lock->ctx;

	if (!panel) {
		panel_err("panel is null\n");
		return;
	}

	if (panel_lock->magic != PANEL_MUTEX_MAGIC) {
		panel_err("lock magic is abnormal\n");
		return;
	}

#if IS_ENABLED(CONFIG_USDM_PANEL_BIG_LOCK)
	commit_thread_pid = atomic_read(&panel->commit_thread_pid);
	pid = current->pid;

	if (pid != commit_thread_pid) {
		mutex_lock(&panel->panel_mutex_op_lock);
		panel_mutex_depth_before = panel->panel_mutex_depth;
		if (panel->panel_mutex_depth == 0)
			mutex_lock(&panel->panel_mutex_big_lock);
		panel->panel_mutex_depth++;
		panel_mutex_depth_after = panel->panel_mutex_depth;
		mutex_unlock(&panel->panel_mutex_op_lock);
	}

	mutex_lock(&panel_lock->base);

	locked = mutex_is_locked(&panel_lock->base);
	if (!locked) {
		panel_info("not locked\n");
		PANEL_BUG();
	}
#else
	mutex_lock(&panel_lock->base);
#endif
}

void panel_mutex_unlock(struct panel_mutex *panel_lock)
{
	static struct panel_device *panel;
#if IS_ENABLED(CONFIG_USDM_PANEL_BIG_LOCK)
	int commit_thread_pid;
	int panel_mutex_depth_before;
	int panel_mutex_depth_after;
	bool locked;
	int pid;
#endif

	if (!panel)
		panel = panel_lock->ctx;

	if (!panel) {
		panel_err("panel is null\n");
		return;
	}

	if (panel_lock->magic != PANEL_MUTEX_MAGIC) {
		panel_err("lock magic is abnormal\n");
		return;
	}

#if IS_ENABLED(CONFIG_USDM_PANEL_BIG_LOCK)
	locked = mutex_is_locked(&panel_lock->base);
	if (!locked) {
		panel_info("not locked\n");
		PANEL_BUG();
	}
#endif

	mutex_unlock(&panel_lock->base);

#if IS_ENABLED(CONFIG_USDM_PANEL_BIG_LOCK)
	commit_thread_pid = atomic_read(&panel->commit_thread_pid);
	pid = current->pid;

	if (pid != commit_thread_pid) {
		mutex_lock(&panel->panel_mutex_op_lock);
		panel_mutex_depth_before = panel->panel_mutex_depth;
		panel->panel_mutex_depth--;
		if (panel->panel_mutex_depth < 0)
			PANEL_BUG();
		if (panel->panel_mutex_depth == 0)
			mutex_unlock(&panel->panel_mutex_big_lock);
		panel_mutex_depth_after = panel->panel_mutex_depth;
		mutex_unlock(&panel->panel_mutex_op_lock);
	}
#endif
}

MODULE_DESCRIPTION("mutex driver for panel");
MODULE_LICENSE("GPL");
