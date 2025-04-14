/*
 *  Copyright (C) 2020, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#ifndef __SHUB_PROXIMITY_FACTORY_H_
#define __SHUB_PROXIMITY_FACTORY_H_

#include <linux/device.h>
#include <linux/types.h>


struct proximity_factory_chipset_funcs {
	ssize_t (*trim_check_show)(char *buf);
	ssize_t (*prox_cal_show)(char *buf);
	ssize_t (*prox_cal_store)(const char *buf, size_t size, int type);
	ssize_t (*prox_trim_show)(char *buf, int type);
	ssize_t (*modify_settings_show)(char *buf);
	ssize_t (*modify_settings_store)(const char *buf, size_t size);
	ssize_t (*settings_thresh_high_show)(char *buf);
	ssize_t (*settings_thresh_high_store)(const char *buf, size_t size);
	ssize_t (*settings_thresh_low_show)(char *buf);
	ssize_t (*settings_thresh_low_store)(const char *buf, size_t size);
	ssize_t (*debug_info_show)(char *buf);
	ssize_t (*prox_led_test_show)(char *buf);
	ssize_t (*thresh_detect_high_show)(char *buf);
	ssize_t (*thresh_detect_high_store)(const char *buf, size_t size);
	ssize_t (*thresh_detect_low_show)(char *buf);
	ssize_t (*thresh_detect_low_store)(const char *buf, size_t size);
	ssize_t (*prox_trim_check_show)(char *name);
};

struct proximity_factory_chipset_funcs *get_proximity_gp2ap110s_chipset_func(char *name);
struct proximity_factory_chipset_funcs *get_proximity_stk3x6x_chipset_func(char *name);
struct proximity_factory_chipset_funcs *get_proximity_stk3328_chipset_func(char *name);
struct proximity_factory_chipset_funcs *get_proximity_tmd3725_chipset_func(char *name);
struct proximity_factory_chipset_funcs *get_proximity_tmd4912_chipset_func(char *name);
struct proximity_factory_chipset_funcs *get_proximity_stk3391x_chipset_func(char *name);
struct proximity_factory_chipset_funcs *get_proximity_stk33512_chipset_func(char *name);
struct proximity_factory_chipset_funcs *get_proximity_stk3afx_chipset_func(char *name);
struct proximity_factory_chipset_funcs *get_proximity_tmd4913_chipset_func(char *name);
struct proximity_factory_chipset_funcs *get_proximity_tmd4914_chipset_func(char *name);

#endif
