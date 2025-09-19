/*
 * sb_mag.h
 * Samsung Mobile Wireless Magnetic Header
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

#ifndef __SB_MAG_H
#define __SB_MAG_H __FILE__

#include <linux/err.h>

struct sec_battery_info;

#if IS_ENABLED(CONFIG_HALL_NOTIFIER)
int sb_mag_init(struct sec_battery_info *battery);
int sb_mag_get_state(int *state);
#else
static inline int sb_mag_init(struct sec_battery_info *battery)
{ return -ENODEV; }
static inline int sb_mag_get_state(int *state)
{ return -ENODEV; }
#endif

#endif /* __SB_MAG_H */
