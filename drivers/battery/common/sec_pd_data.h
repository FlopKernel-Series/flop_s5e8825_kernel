/*
 * sec_pd_data.h
 * Samsung Mobile SEC PD Data Header
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
#ifndef __SEC_PD_DATA_H
#define __SEC_PD_DATA_H __FILE__

struct sec_pd;

/* dt */
bool get_sec_pd_rp_current_wa_state(void);

/* sec_pd data */
struct sec_pd *init_sec_pd(void);
const char *get_sec_pd_op_name(struct sec_pd *);
int change_sec_pd(int);
struct sec_pd *get_sec_pd(void);

#endif /* __SEC_PD_DATA_H */
