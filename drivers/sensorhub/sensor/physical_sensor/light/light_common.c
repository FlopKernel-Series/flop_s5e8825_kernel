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

#include <linux/slab.h>

#include "../../../sensor/light.h"
#include "../../../sensormanager/shub_sensor.h"
#include "../../../sensormanager/shub_sensor_manager.h"
#include "../../../comm/shub_comm.h"
#include "../../../sensorhub/shub_device.h"
#include "../../../utility/shub_utility.h"
#include "../../../utility/shub_file_manager.h"
#include "../../../others/shub_panel.h"

int init_light_common(int type)
{
	struct light_data *data = get_sensor(type)->data;

	data->use_cal_data = true;
	shub_info("light_use_cal_data true;");
	return 0;
}

struct sensor_chipset_init_funcs light_common_ops = {
	.init = init_light_common,
};

struct sensor_chipset_init_funcs *get_light_common_function_pointer(char *name)
{
	if (strstr(name, "STK33512") != NULL || strstr(name, "STK33F11") != NULL || strstr(name, "STK33F15") != NULL || 
		strstr(name, "STK33911") != NULL || strstr(name, "STK33917") != NULL || strstr(name, "TMD4914")!= NULL || \
		strstr(name, "TMD4913") != NULL || strstr(name, "VEML3235O") != NULL)
		return &light_common_ops;
	else
		return NULL;
}
