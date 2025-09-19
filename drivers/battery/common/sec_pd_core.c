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

#define SEC_PD_OP_NAME	"SEC-PD-CORE"
#define pd_log(str, ...) pr_info("[%s]:%s: "str, SEC_PD_OP_NAME, __func__, ##__VA_ARGS__)

bool is_pdo_valid(SEC_PD_SINK_STATUS *pd_sink, unsigned int pdo)
{
	return (pdo <= pd_sink->available_pdo_num);
}
EXPORT_SYMBOL(is_pdo_valid);

bool is_apdo(SEC_PD_SINK_STATUS *pd_sink, unsigned int pdo)
{
	return (pd_sink->power_list[pdo].pdo_type == APDO_TYPE);
}
EXPORT_SYMBOL(is_apdo);

#define PROG_0V 0
#define PROG_5V 5900
#define PROG_9V 11000
#define PROG_15V 16000
#define PROG_20V 21000
#define PROG_MIN 3300
int get_apdo_prog_volt(unsigned int pdo_type, unsigned int max_volt)
{
	if (pdo_type != APDO_TYPE)
		return max_volt;

	switch (max_volt) {
	case PROG_0V ... (PROG_5V - 1):
		return 0;
	case PROG_5V ... (PROG_9V - 1):
		return 5000;
	case PROG_9V ... (PROG_15V - 1):
		return 9000;
	case PROG_15V ... (PROG_20V - 1):
		return 15000;
	}
	return 20000;
}
EXPORT_SYMBOL(get_apdo_prog_volt);

static int check_pdo(SEC_PD_SINK_STATUS *pd_sink, unsigned int pdo, unsigned int min_volt, unsigned int max_volt, unsigned int max_curr)
{
	POWER_LIST *pwr = &pd_sink->power_list[pdo];

	if (pwr->pdo_type == APDO_TYPE) {
		if (min_volt > 0) {
			if (min_volt < pwr->min_voltage)
				return -1;

			if (min_volt > pwr->max_voltage)
				return -1;
		}

		if (max_volt > 0) {
			if (max_volt > pwr->max_voltage)
				return -1;

			if (max_volt < pwr->min_voltage)
				return -1;
		}
	} else {
		if (max_volt != pwr->max_voltage)
			return -1;
	}

	if ((max_curr > 0) && (max_curr > pwr->max_current))
		return -1;

	return get_apdo_prog_volt(pwr->pdo_type, pwr->max_voltage) * pwr->max_current;
}

int get_pdo_power(SEC_PD_SINK_STATUS *pd_sink, unsigned int *pdo, unsigned int *min_volt, unsigned int *max_volt, unsigned int *max_curr)
{
	unsigned int npdo = 0, nmin_volt = 0, nmax_volt = 0, ncurr = 0;
	int nidx = 0, npwr = 0;
	POWER_LIST *pwr;

	npdo = (pdo != NULL) ? (*pdo) : 0;
	nmin_volt = (min_volt != NULL) ? (*min_volt) : 0;
	nmax_volt = (max_volt != NULL) ? (*max_volt) : 0;
	ncurr = (max_curr != NULL) ? (*max_curr) : 0;

	if (!is_pdo_valid(pd_sink, npdo))
		return -EINVAL;

	if (npdo != 0) {
		npwr = check_pdo(pd_sink, npdo, nmin_volt, nmax_volt, ncurr);
		nidx = npdo;
	} else {
		int i, anidx = 0, anpwr = 0, tpwr = 0;

		for (i = 1; i <= pd_sink->available_pdo_num; i++) {
			pwr = &pd_sink->power_list[i];

			tpwr = check_pdo(pd_sink, i, nmin_volt, nmax_volt, ncurr);
			if (pwr->pdo_type == APDO_TYPE) {
				if (anpwr < tpwr) {
					anidx = i;
					anpwr = tpwr;
				}
			} else {
				if (npwr < tpwr) {
					nidx = i;
					npwr = tpwr;
				}
			}
		}

		if (!npwr) {
			nidx = anidx;
			npwr = anpwr;
		}
	}

	if ((nidx <= 0) || (npwr <= 0))
		return npwr;

	/* update values */
	pwr = &pd_sink->power_list[nidx];

	if (pdo != NULL)
		*pdo = nidx;
	if (min_volt != NULL)
		*min_volt = pwr->min_voltage;
	if (max_volt != NULL)
		*max_volt = pwr->max_voltage;
	if (max_curr != NULL)
		*max_curr = pwr->max_current;

	return npwr;
}
EXPORT_SYMBOL(get_pdo_power);

int get_apdo_max_power(SEC_PD_SINK_STATUS *pd_sink, unsigned int *pdo_pos, unsigned int *taMaxVol, unsigned int *taMaxCur, unsigned int *taMaxPwr)
{
	int i;
	int fpdo_max_power = 0;
	int apdo_max_prog_power = 0;

	if (!pd_sink->has_apdo)
		return -1;

	for (i = 1; i <= pd_sink->available_pdo_num; i++) {
		if (pd_sink->power_list[i].pdo_type != APDO_TYPE) {
			fpdo_max_power =
				(pd_sink->power_list[i].max_voltage * pd_sink->power_list[i].max_current) > fpdo_max_power ?
				(pd_sink->power_list[i].max_voltage * pd_sink->power_list[i].max_current) : fpdo_max_power;
		}
	}

	if (*pdo_pos == 0) {
		/* min(max power of all fpdo, max power of selected apdo) */
		for (i = 1; i <= pd_sink->available_pdo_num; i++) {
			if ((pd_sink->power_list[i].pdo_type == APDO_TYPE) &&
				pd_sink->power_list[i].accept &&
				(pd_sink->power_list[i].max_voltage >= *taMaxVol)) {
				*pdo_pos = i;
				*taMaxVol = pd_sink->power_list[i].max_voltage;
				*taMaxCur = pd_sink->power_list[i].max_current;
				apdo_max_prog_power = pd_sink->power_list[i].max_current *
					get_apdo_prog_volt(pd_sink->power_list[i].pdo_type,
						pd_sink->power_list[i].max_voltage);
				/*In some TA, fpdo_max_power < apdo_max_prog_power*/
				*taMaxPwr = min(max(fpdo_max_power, apdo_max_prog_power),
					(pd_sink->power_list[i].max_voltage * pd_sink->power_list[i].max_current));

				return 0;
			}
		}
	} else {
		/* If we already have pdo object position, we don't need to search max current */
		return -ENOTSUPP;
	}

	return -EINVAL;
}
EXPORT_SYMBOL(get_apdo_max_power);

const char *get_op_name(struct sec_pd *pd)
{
	return pd->name;
}
EXPORT_SYMBOL(get_op_name);

static bool is_pd_sink_null(struct sec_pd *pd)
{
	return (pd->pd_sink == NULL);
}

static bool is_fp_sec_pd_select_pdo_null(struct sec_pd *pd)
{
	return (pd->pd_sink->fp_sec_pd_select_pdo == NULL);
}

static bool is_fp_sec_pd_select_pps_null(struct sec_pd *pd)
{
	return (pd->pd_sink->fp_sec_pd_select_pps == NULL);
}

void op_init(struct sec_pd *pd, SEC_PD_SINK_STATUS *pd_sink)
{
	pd->op.init(pd, pd_sink);
}
EXPORT_SYMBOL(op_init);

int op_select_pdo(struct sec_pd *pd, int pdo)
{
	if (is_pd_sink_null(pd)) {
		pd_log("pd sink is null\n");
		return -1;
	}

	if (is_fp_sec_pd_select_pdo_null(pd)) {
		pd_log("select_pdo is not set\n");
		return -2;
	}

	return pd->op.select_pdo(pd, pdo);
}
EXPORT_SYMBOL(op_select_pdo);

int op_select_pps(struct sec_pd *pd, int pdo, int volt, int curr)
{
	if (is_pd_sink_null(pd)) {
		pd_log("pd sink is null\n");
		return -1;
	}

	if (is_fp_sec_pd_select_pps_null(pd)) {
		pd_log("select_pps is not set\n");
		return -2;
	}

	return pd->op.select_pps(pd, pdo, volt, curr);
}
EXPORT_SYMBOL(op_select_pps);

int op_get_current_pdo(struct sec_pd *pd, unsigned int *pdo)
{
	if (is_pd_sink_null(pd)) {
		pd_log("pd sink is null\n");
		return -1;
	}

	return pd->op.get_current_pdo(pd, pdo);
}
EXPORT_SYMBOL(op_get_current_pdo);

int op_get_selected_pdo(struct sec_pd *pd, unsigned int *pdo)
{
	if (is_pd_sink_null(pd)) {
		pd_log("pd sink is null\n");
		return -1;
	}

	return pd->op.get_selected_pdo(pd, pdo);
}
EXPORT_SYMBOL(op_get_selected_pdo);

int op_is_apdo(struct sec_pd *pd, unsigned int pdo)
{
	if (is_pd_sink_null(pd)) {
		pd_log("pd sink is null\n");
		return -1;
	}

	return pd->op.is_apdo(pd, pdo);
}
EXPORT_SYMBOL(op_is_apdo);

int op_get_apdo_prog_volt(struct sec_pd *pd, unsigned int pdo_type, unsigned int max_volt)
{
	if (is_pd_sink_null(pd)) {
		pd_log("pd sink is null\n");
		return get_apdo_prog_volt(pdo_type, max_volt);
	}

	return pd->op.get_apdo_prog_volt(pd, pdo_type, max_volt);
}
EXPORT_SYMBOL(op_get_apdo_prog_volt);

int op_get_max_power(struct sec_pd *pd, unsigned int pdo_type, unsigned int min_volt, unsigned int max_volt, unsigned int max_curr)
{
	if (is_pd_sink_null(pd)) {
		pd_log("pd sink is null\n");
		return get_apdo_prog_volt(pdo_type, max_volt) * max_curr;
	}

	return pd->op.get_max_power(pd, pdo_type, min_volt, max_volt, max_curr);
}
EXPORT_SYMBOL(op_get_max_power);

int op_get_pdo_power(struct sec_pd *pd, unsigned int *pdo, unsigned int *min_volt, unsigned int *max_volt, unsigned int *max_curr)
{
	if (is_pd_sink_null(pd)) {
		pd_log("pd sink is null\n");
		return -1;
	}

	return pd->op.get_pdo_power(pd, pdo, min_volt, max_volt, max_curr);
}
EXPORT_SYMBOL(op_get_pdo_power);

int op_get_apdo_max_power(struct sec_pd *pd, unsigned int *pdo_pos, unsigned int *taMaxVol, unsigned int *taMaxCur, unsigned int *taMaxPwr)
{
	if (is_pd_sink_null(pd)) {
		pd_log("pd sink is null\n");
		return -1;
	}

	return pd->op.get_apdo_max_power(pd, pdo_pos, taMaxVol, taMaxCur, taMaxPwr);
}
EXPORT_SYMBOL(op_get_apdo_max_power);

int op_set_pd_voltage(struct sec_pd *pd, unsigned int volt)
{
	if (is_pd_sink_null(pd)) {
		pd_log("pd sink is null\n");
		return -1;
	}

	if (is_fp_sec_pd_select_pdo_null(pd) ||
		is_fp_sec_pd_select_pps_null(pd)) {
		pr_err("%s: select_pdo / pps  is not set\n", __func__);
		return -2;
	}

	return pd->op.set_pd_voltage(pd, volt);
}
EXPORT_SYMBOL(op_set_pd_voltage);
