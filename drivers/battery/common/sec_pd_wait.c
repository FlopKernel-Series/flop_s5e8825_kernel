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

#include <linux/battery/sec_pd.h>
#include <linux/usb/typec/manager/usb_typec_manager_notifier.h>
#include "sec_pd.h"
#include "sec_pd_core.h"
#include "sec_pd_event.h"

#define SEC_PD_OP_NAME	"SEC-PD-WAIT"
#define pd_log(str, ...) pr_info("[%s]:%s: "str, SEC_PD_OP_NAME, __func__, ##__VA_ARGS__)

static int pd_event_wait(void)
{
	int ret = 0;

	lock_pd_event();
	ret = wait_pd_event(DEFAULT_RESET_EVENT, DEFAULT_PD_WAIT_DELAY);
	if (ret == 0) {
		unlock_pd_event();
		return -EBUSY;
	}
	inc_pd_event();
	unlock_pd_event();
	return 0;
}

static void pd_event_done(void)
{
	dec_pd_event();
	wakeup_pd_event();
}

static void pd_event_reset(void)
{
	reset_pd_event(DEFAULT_RESET_EVENT);
	wakeup_pd_event();
}

static int wait_init(struct sec_pd *pd, SEC_PD_SINK_STATUS *pd_sink)
{
	pd_log("\n");
	pd->pd_sink = pd_sink;
	return 0;
}

static int wait_select_pdo(struct sec_pd *pd, int pdo)
{
	SEC_PD_SINK_STATUS *pd_sink = pd->pd_sink;
	int ret = 0;

	ret = pd_event_wait();
	if (ret < 0) {
		pd_log("timeout\n");
		return ret;
	}

	pd_sink->fp_sec_pd_select_pdo(pdo);
	return 0;
}

static int wait_select_pps(struct sec_pd *pd, int pdo, int volt, int cur)
{
	SEC_PD_SINK_STATUS *pd_sink = pd->pd_sink;
	int ret = 0;

	ret = pd_event_wait();
	if (ret < 0) {
		pd_log("timeout\n");
		return ret;
	}

	return pd_sink->fp_sec_pd_select_pps(pdo, volt, cur);
}

static int wait_get_current_pdo(struct sec_pd *pd, unsigned int *pdo)
{
	SEC_PD_SINK_STATUS *pd_sink = pd->pd_sink;
	int ret = 0;

	ret = pd_event_wait();
	if (ret < 0) {
		pd_log("timeout\n");
		return ret;
	}

	*pdo = pd_sink->current_pdo_num;
	pd_event_done();
	return 0;
}

static int wait_get_selected_pdo(struct sec_pd *pd, unsigned int *pdo)
{
	SEC_PD_SINK_STATUS *pd_sink = pd->pd_sink;
	int ret = 0;

	ret = pd_event_wait();
	if (ret < 0) {
		pd_log("timeout\n");
		return ret;
	}

	*pdo = pd_sink->selected_pdo_num;
	pd_event_done();
	return 0;
}

static int wait_is_apdo(struct sec_pd *pd, unsigned int pdo)
{
	SEC_PD_SINK_STATUS *pd_sink = pd->pd_sink;
	int ret = 0;

	ret = pd_event_wait();
	if (ret < 0) {
		pd_log("timeout\n");
		return ret;
	}

	if (!is_pdo_valid(pd_sink, pdo)) {
		pd_log("invalid argument(%d)\n", pdo);
		ret = -EINVAL;
		goto end_pd;
	}

	ret = is_apdo(pd_sink, pdo);
end_pd:
	pd_event_done();
	return ret;
}

static int wait_get_apdo_prog_volt(struct sec_pd *pd, unsigned int pdo_type, unsigned int max_volt)
{
	int ret = 0;

	ret = pd_event_wait();
	if (ret < 0) {
		pd_log("timeout\n");
		return ret;
	}

	ret = get_apdo_prog_volt(pdo_type, max_volt);
	pd_event_done();
	return ret;
}

static int wait_get_max_power(struct sec_pd *pd, unsigned int pdo_type, unsigned int min_volt, unsigned int max_volt, unsigned int max_curr)
{
	int ret = 0;

	ret = pd_event_wait();
	if (ret < 0) {
		pd_log("timeout\n");
		return ret;
	}

	ret = get_apdo_prog_volt(pdo_type, max_volt) * max_curr;
	pd_event_done();
	return ret;
}

static int wait_get_pdo_power(struct sec_pd *pd, unsigned int *pdo, unsigned int *min_volt, unsigned int *max_volt, unsigned int *max_curr)
{
	int ret = 0;

	ret = pd_event_wait();
	if (ret < 0) {
		pd_log("timeout\n");
		return ret;
	}

	ret = get_pdo_power(pd->pd_sink, pdo, min_volt, max_volt, max_curr);
	pd_event_done();
	return ret;
}

static int wait_get_apdo_max_power(struct sec_pd *pd, unsigned int *pdo_pos, unsigned int *taMaxVol, unsigned int *taMaxCur, unsigned int *taMaxPwr)
{
	int ret = 0;

	ret = pd_event_wait();
	if (ret < 0) {
		pd_log("timeout\n");
		return ret;
	}

	ret = get_apdo_max_power(pd->pd_sink, pdo_pos, taMaxVol, taMaxCur, taMaxPwr);
	pd_event_done();
	return ret;
}

static int wait_set_pd_voltage(struct sec_pd *pd, unsigned int volt)
{
	SEC_PD_SINK_STATUS *pd_sink = pd->pd_sink;
	int ret = 0, pdo = 0, iv = 0, icl = 0;

	ret = pd_event_wait();
	if (ret < 0) {
		pd_log("timeout\n");
		return ret;
	}

	iv = volt;
	ret = get_pdo_power(pd_sink, &pdo, &iv, &iv, &icl);
	if (ret <= 0) {
		pd_log("failed to get pod, ret = %d\n", ret);
		pd_event_done();
		return ret;
	}

	if (is_apdo(pd_sink, pdo)) {
		ret = pd_sink->fp_sec_pd_select_pps(pdo, volt, icl);
	} else {
		pd_sink->fp_sec_pd_select_pdo(pdo);
		ret = 0;
	}

	return ret;
}

static struct sec_pd pd_op = {
	.name = SEC_PD_OP_NAME,
	.op = {
		.init = wait_init,
		.select_pdo = wait_select_pdo,
		.select_pps = wait_select_pps,
		.get_current_pdo = wait_get_current_pdo,
		.get_selected_pdo = wait_get_selected_pdo,
		.get_apdo_prog_volt = wait_get_apdo_prog_volt,
		.is_apdo = wait_is_apdo,
		.get_max_power = wait_get_max_power,
		.get_pdo_power = wait_get_pdo_power,
		.get_apdo_max_power = wait_get_apdo_max_power,
		.set_pd_voltage = wait_set_pd_voltage,
	},
};

static const struct pd_event_op event_op = {
	.ps_ready = pd_event_done,
	.pd_reset = pd_event_reset,
};

struct sec_pd *get_wait_sec_pd(void)
{
	int ret = 0;

	ret = init_pd_event(DEFAULT_RESET_EVENT);
	if (ret < 0)
		return ERR_PTR(ret);

	change_pd_event_op(&event_op);
	return &pd_op;
}
EXPORT_SYMBOL(get_wait_sec_pd);
