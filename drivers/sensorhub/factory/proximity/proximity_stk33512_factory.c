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
#include <linux/delay.h>
#include <linux/of_gpio.h>
#include <linux/uaccess.h>

#include "../../sensor/proximity.h"
#include "../../sensormanager/shub_sensor.h"
#include "../../sensormanager/shub_sensor_manager.h"
#include "../../comm/shub_comm.h"
#include "../../sensorhub/shub_device.h"
#include "../../utility/shub_utility.h"
#include "proximity_factory.h"
#include "../../others/shub_panel.h"

static int proximity_get_calibration_data(int type)
{
	int ret = 0;
	char *buffer = NULL;
	int buffer_length = 0;
	struct proximity_data *data = (struct proximity_data *)get_sensor(type)->data;

	ret = shub_send_command_wait(CMD_GETVALUE, type, CAL_DATA, 1000, NULL, 0, &buffer,
					&buffer_length, true);
	if (ret < 0) {
		shub_errf("shub_send_command_wait fail %d", ret);
		return ret;
	}

	if (buffer_length != data->cal_data_len) {
		shub_errf("buffer length error(%d)", buffer_length);
		kfree(buffer);
		return -EINVAL;
	}

	memcpy(data->cal_data, buffer, data->cal_data_len);

	save_proximity_calibration(type);

	kfree(buffer);

	return 0;
}

static ssize_t proximity_stk33512_prox_cal_store(const char *buf, size_t size, int type)
{
	int ret = 0;
	struct proximity_data *data = (struct proximity_data *)get_sensor(type)->data;

	ret = shub_send_command(CMD_SETVALUE, type, PROX_SUBCMD_CALIBRATION_START, NULL, 0);
	if (ret < 0) {
		shub_errf("shub_send_command_wait fail %d", ret);
		return ret;
	}

	msleep(500);

	proximity_get_calibration_data(type);

	shub_infof("ADC : %u, mode : %u", *((u16 *)(data->cal_data)), data->setting_mode);

	return size;
}

struct proximity_factory_chipset_funcs proximity_stk33512_ops = {
	.prox_cal_store = proximity_stk33512_prox_cal_store,
};

struct proximity_factory_chipset_funcs *get_proximity_stk33512_chipset_func(char *name)
{
	if (strcmp(name, "STK33512") != 0)
		return NULL;

	return &proximity_stk33512_ops;
}
