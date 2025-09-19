/*
 * sec_pd_core.h
 * Samsung Mobile SEC PD Core Header
 *
 * Copyright (C) 2025 Samsung Electronics, Inc.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __SEC_PD_CORE_H
#define __SEC_PD_CORE_H __FILE__

#include <linux/battery/sec_pd.h>

struct sec_pd;
struct sec_pd_op {
	int (*init)(struct sec_pd *, SEC_PD_SINK_STATUS *);
	int (*select_pdo)(struct sec_pd *, int);
	int (*select_pps)(struct sec_pd *, int, int, int);
	int (*get_current_pdo)(struct sec_pd *, unsigned int *);
	int (*get_selected_pdo)(struct sec_pd *, unsigned int *);
	int (*is_apdo)(struct sec_pd *, unsigned int);
	int (*get_apdo_prog_volt)(struct sec_pd *, unsigned int, unsigned int);
	int (*get_max_power)(struct sec_pd *, unsigned int, unsigned int, unsigned int, unsigned int);
	int (*get_pdo_power)(struct sec_pd *, unsigned int *, unsigned int *, unsigned int *, unsigned int *);
	int (*get_apdo_max_power)(struct sec_pd *, unsigned int *, unsigned int *, unsigned int *, unsigned int *);
	int (*set_pd_voltage)(struct sec_pd *, unsigned int);
};

struct sec_pd {
	const char *name;
	SEC_PD_SINK_STATUS *pd_sink;
	const struct sec_pd_op op;
};

bool is_pdo_valid(SEC_PD_SINK_STATUS *, unsigned int);
bool is_apdo(SEC_PD_SINK_STATUS *, unsigned int);

int get_apdo_prog_volt(unsigned int, unsigned int);
int get_pdo_power(SEC_PD_SINK_STATUS *, unsigned int *, unsigned int *, unsigned int *, unsigned int *);
int get_apdo_max_power(SEC_PD_SINK_STATUS *, unsigned int *, unsigned int *, unsigned int *, unsigned int *);

#endif /* __SEC_PD_CORE_H */
