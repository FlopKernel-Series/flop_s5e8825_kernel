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

#include <linux/module.h>
#include <linux/battery/sec_pd.h>
#include "sec_pd.h"
#include "sec_pd_core.h"

#define SEC_PD_OP_NAME	"SEC-PD-ASYNC"
#define pd_log(str, ...) pr_info("[%s]:%s: "str, SEC_PD_OP_NAME, __func__, ##__VA_ARGS__)

static int async_init(struct sec_pd *pd, SEC_PD_SINK_STATUS *pd_sink)
{
	pd_log("\n");
	pd->pd_sink = pd_sink;
	return 0;
}

static int async_select_pdo(struct sec_pd *pd, int pdo)
{
	SEC_PD_SINK_STATUS *pd_sink = pd->pd_sink;

	pd_sink->fp_sec_pd_select_pdo(pdo);
	return 0;
}

static int async_select_pps(struct sec_pd *pd, int pdo, int volt, int cur)
{
	SEC_PD_SINK_STATUS *pd_sink = pd->pd_sink;

	return pd_sink->fp_sec_pd_select_pps(pdo, volt, cur);
}

static int async_get_current_pdo(struct sec_pd *pd, unsigned int *pdo)
{
	SEC_PD_SINK_STATUS *pd_sink = pd->pd_sink;

	*pdo = pd_sink->current_pdo_num;
	return 0;
}

static int async_get_selected_pdo(struct sec_pd *pd, unsigned int *pdo)
{
	SEC_PD_SINK_STATUS *pd_sink = pd->pd_sink;

	*pdo = pd_sink->selected_pdo_num;
	return 0;
}

static int async_is_apdo(struct sec_pd *pd, unsigned int pdo)
{
	SEC_PD_SINK_STATUS *pd_sink = pd->pd_sink;

	if (!is_pdo_valid(pd_sink, pdo)) {
		pd_log("invalid argument(%d)\n", pdo);
		return -EINVAL;
	}

	return is_apdo(pd_sink, pdo);
}

static int async_get_apdo_prog_volt(struct sec_pd *pd, unsigned int pdo_type, unsigned int max_volt)
{
	return get_apdo_prog_volt(pdo_type, max_volt);
}

static int async_get_max_power(struct sec_pd *pd, unsigned int pdo_type, unsigned int min_volt, unsigned int max_volt, unsigned int max_curr)
{
	return get_apdo_prog_volt(pdo_type, max_volt) * max_curr;
}

static int async_get_pdo_power(struct sec_pd *pd, unsigned int *pdo, unsigned int *min_volt, unsigned int *max_volt, unsigned int *max_curr)
{
	return get_pdo_power(pd->pd_sink, pdo, min_volt, max_volt, max_curr);
}

static int async_get_apdo_max_power(struct sec_pd *pd, unsigned int *pdo_pos, unsigned int *taMaxVol, unsigned int *taMaxCur, unsigned int *taMaxPwr)
{
	return get_apdo_max_power(pd->pd_sink, pdo_pos, taMaxVol, taMaxCur, taMaxPwr);
}

static int async_set_pd_voltage(struct sec_pd *pd, unsigned int volt)
{
	return -ENOTSUPP; /* not supported */
}

static struct sec_pd pd_op = {
	.name = SEC_PD_OP_NAME,
	.op = {
		.init = async_init,
		.select_pdo = async_select_pdo,
		.select_pps = async_select_pps,
		.get_current_pdo = async_get_current_pdo,
		.get_selected_pdo = async_get_selected_pdo,
		.get_apdo_prog_volt = async_get_apdo_prog_volt,
		.is_apdo = async_is_apdo,
		.get_max_power = async_get_max_power,
		.get_pdo_power = async_get_pdo_power,
		.get_apdo_max_power = async_get_apdo_max_power,
		.set_pd_voltage = async_set_pd_voltage,
	},
};

struct sec_pd *get_async_sec_pd(void)
{
	return &pd_op;
}
EXPORT_SYMBOL(get_async_sec_pd);
