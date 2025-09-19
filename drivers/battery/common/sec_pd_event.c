/*
 * Copyrights (C) 2025 Samsung Electronics, Inc.
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

#include <linux/wait.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/battery/sec_pd.h>

#include <linux/usb/typec/manager/usb_typec_manager_notifier.h>

#include "sec_pd.h"
#include "sec_pd_event.h"

#define SEC_PD_OP_NAME	"SEC-PD-EVENT"
#define pd_log(str, ...) pr_info("[%s]:%s: "str, SEC_PD_OP_NAME, __func__, ##__VA_ARGS__)

struct pd_event {
	struct notifier_block usb_typec_nb;
	struct mutex lock;
	wait_queue_head_t wq;
	atomic_t count;

	struct workqueue_struct *wqueue;
	struct delayed_work retry_work;
	struct wakeup_source *retry_ws;

	bool init_state;
	const struct pd_event_op *op;
};
static struct pd_event g_event;

static int pd_typec_handle_notification(struct notifier_block *nb,
	unsigned long action, void *data)
{
	struct pd_event *event = container_of(nb, struct pd_event, usb_typec_nb);
	PD_NOTI_TYPEDEF *pdata = (PD_NOTI_TYPEDEF *)data;
	struct pdic_notifier_struct *noti;
	SEC_PD_SINK_STATUS *psink_status = NULL;

	if ((pdata->dest != PDIC_NOTIFY_DEV_BATT) && (pdata->dest != PDIC_NOTIFY_DEV_ALL))
		return 0;

	if (pdata->id != PDIC_NOTIFY_ID_POWER_STATUS)
		return 0;

	noti = pdata->pd;
	if (!noti)
		return 0;

	if (noti->event != PDIC_NOTIFY_EVENT_PD_SINK) {
		if ((noti->event == PDIC_NOTIFY_EVENT_PDIC_ATTACH) ||
			(noti->event == PDIC_NOTIFY_EVENT_DETACH)) {
			pd_log("reset, event = %d\n", noti->event);
			event->op->pd_reset();
		}

		return 0;
	}

	psink_status = &noti->sink_status;
	if (!psink_status)
		return 0;

	if (psink_status->selected_pdo_num == psink_status->current_pdo_num) {
		pd_log("ps ready\n");
		event->op->ps_ready();
	}

	return 0;
}

static void pd_retry_work(struct work_struct *work)
{
	pd_log("\n");
}

static void dummy_ps_ready(void)
{
	pd_log("\n");
}

static void dummy_pd_reset(void)
{
	pd_log("\n");
}

static const struct pd_event_op dummy_op = {
	.ps_ready = dummy_ps_ready,
	.pd_reset = dummy_pd_reset
};

static bool is_valid_op(const struct pd_event_op *op)
{
	return (op != NULL) &&
		(op->ps_ready != NULL) &&
		(op->pd_reset != NULL);
}

int init_pd_event(unsigned int reset_count)
{
	if (g_event.init_state)
		return 0;

	g_event.wqueue = create_singlethread_workqueue(SEC_PD_OP_NAME);
	if (!g_event.wqueue)
		goto err_workqueue;

	g_event.retry_ws = wakeup_source_register(NULL, SEC_PD_OP_NAME);
	if (!g_event.retry_ws)
		goto err_wakeup;

	INIT_DELAYED_WORK(&g_event.retry_work, pd_retry_work);
	mutex_init(&g_event.lock);
	init_waitqueue_head(&g_event.wq);
	atomic_set(&g_event.count, reset_count);

	g_event.op = &dummy_op;
	manager_notifier_register(&g_event.usb_typec_nb,
		pd_typec_handle_notification, (MANAGER_NOTIFY_PDIC_BATTERY + 1));

	g_event.init_state = true;
	return 0;

err_wakeup:
	destroy_workqueue(g_event.wqueue);
err_workqueue:
	return -ENOMEM;
}
EXPORT_SYMBOL(init_pd_event);

int change_pd_event_op(const struct pd_event_op *op)
{
	if (!is_valid_op(op))
		return -EINVAL;

	g_event.op = op;
	return 0;
}
EXPORT_SYMBOL(change_pd_event_op);

void lock_pd_event(void)
{
	mutex_lock(&g_event.lock);
}
EXPORT_SYMBOL(lock_pd_event);

void unlock_pd_event(void)
{
	mutex_unlock(&g_event.lock);
}
EXPORT_SYMBOL(unlock_pd_event);

void inc_pd_event(void)
{
	atomic_inc(&g_event.count);
}
EXPORT_SYMBOL(inc_pd_event);

void dec_pd_event(void)
{
	atomic_dec(&g_event.count);
}
EXPORT_SYMBOL(dec_pd_event);

void reset_pd_event(unsigned int reset_count)
{
	atomic_set(&g_event.count, reset_count);
}
EXPORT_SYMBOL(reset_pd_event);

int wait_pd_event(int def_count, int timeout)
{
	return wait_event_timeout(g_event.wq,
		(atomic_read(&g_event.count) <= def_count), msecs_to_jiffies(timeout));
}
EXPORT_SYMBOL(wait_pd_event);

void wakeup_pd_event(void)
{
	wake_up(&g_event.wq);
}
EXPORT_SYMBOL(wakeup_pd_event);

void start_retry_work(int delay)
{
	__pm_stay_awake(g_event.retry_ws);
	queue_delayed_work(g_event.wqueue, &g_event.retry_work, msecs_to_jiffies(delay));
}
EXPORT_SYMBOL(start_retry_work);

void cancel_retry_work(void)
{
	cancel_delayed_work_sync(&g_event.retry_work);
	__pm_relax(g_event.retry_ws);
}
EXPORT_SYMBOL(cancel_retry_work);
