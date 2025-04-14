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

#ifndef __SHUB_MAGNETOMETER_FACTORY_H_
#define __SHUB_MAGNETOMETER_FACTORY_H_

#include "../../sensor/magnetometer.h"

#include <linux/device.h>
#include <linux/types.h>


struct magnetometer_factory_chipset_funcs {
	ssize_t (*selftest_show)(char *name);
	ssize_t (*hw_offset_show)(char *name);
	ssize_t (*matrix_show)(char *name);
	ssize_t (*matrix_store)(const char *name, size_t size);
	ssize_t (*cover_matrix_show)(char *name);
	ssize_t (*cover_matrix_store)(const char *name, size_t size);
	ssize_t (*mpp_matrix_show)(char *name);
	ssize_t (*mpp_matrix_store)(const char *name, size_t size);
	int (*check_adc_data_spec)(s32 sensor_value[3]);
};

struct magnetometer_factory_chipset_funcs *get_magnetometer_ak09918c_chipset_func(char *name);
struct magnetometer_factory_chipset_funcs *get_magnetometer_mmc5633_chipset_func(char *name);
struct magnetometer_factory_chipset_funcs *get_magnetometer_yas539_chipset_func(char *name);
struct magnetometer_factory_chipset_funcs *get_magnetometer_mxg4300s_chipset_func(char *name);

void get_magnetometer_sensor_value_s32(struct mag_power_event *event, s32 *sensor_value);
#endif
