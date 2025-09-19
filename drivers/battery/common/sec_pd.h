/*
 * sec_pd.h
 * Samsung Mobile SEC PD Header
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
#ifndef __SEC_PD_H
#define __SEC_PD_H __FILE__

#include <linux/battery/sec_pd.h>

struct sec_pd;

void op_init(struct sec_pd *, SEC_PD_SINK_STATUS *);
int op_select_pdo(struct sec_pd *, int);
int op_select_pps(struct sec_pd *, int, int, int);
int op_get_current_pdo(struct sec_pd *, unsigned int *);
int op_get_selected_pdo(struct sec_pd *, unsigned int *);
int op_is_apdo(struct sec_pd *, unsigned int);
int op_get_apdo_prog_volt(struct sec_pd *, unsigned int, unsigned int);
int op_get_max_power(struct sec_pd *, unsigned int, unsigned int, unsigned int, unsigned int);
int op_get_pdo_power(struct sec_pd *, unsigned int *, unsigned int *, unsigned int *, unsigned int *);
int op_get_apdo_max_power(struct sec_pd *, unsigned int *, unsigned int *, unsigned int *, unsigned int *);
int op_set_pd_voltage(struct sec_pd *, unsigned int);
int op_change_op_mode(struct sec_pd *, int);

#if IS_ENABLED(CONFIG_ENG_BATTERY_CONCEPT)
int sec_pd_eng_init(struct sec_pd *);
#else
static inline int sec_pd_eng_init(struct sec_pd *pd) { return -ENODEV; }
#endif

#endif /* __SEC_PD_H */
