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

#include "../../comm/shub_comm.h"
#include "../../sensor/proximity.h"
#include "../../sensorhub/shub_device.h"
#include "../../sensormanager/shub_sensor.h"
#include "../../sensormanager/shub_sensor_manager.h"
#include "../../utility/shub_utility.h"
#include "../../utility/shub_file_manager.h"
#include "proximity_factory.h"

#include <linux/delay.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

static ssize_t proximity_stk3328_prox_trim_show(char *buf, int type)
{
	int ret = 0;
	char *buffer = NULL;
	int buffer_length = 0;
	int trim = 0;

	if (!get_sensor_probe_state(type) || !is_shub_working()) {
		shub_infof("proximity sensor is not connected");
		return -EINVAL;
	}

	ret = shub_send_command_wait(CMD_GETVALUE, type, PROXIMITY_OFFSET, 1000, NULL, 0, &buffer,
				     &buffer_length, true);
	if (ret < 0) {
		shub_errf("shub_send_command_wait Fail %d", ret);
		return ret;
	}

	if (buffer_length != 2) {
		shub_errf("buffer length error %d", buffer_length);
		ret = snprintf(buf, PAGE_SIZE, "-1,0,0,0,0,0,0,0,0,0,0\n");
		if (buffer != NULL)
			kfree(buffer);

		return -EINVAL;
	}

	if (buffer[1] > 0)
		trim = (buffer[0]) * (-1);
	else
		trim = buffer[0];

	shub_infof("%d, 0x%x, 0x%x", trim, buffer[1], buffer[0]);

	ret = snprintf(buf, PAGE_SIZE, "%d\n", trim);

	kfree(buffer);

	return ret;
}

static int save_prox_cal_threshold_data(struct proximity_data *data)
{
	int ret = 0;

	shub_infof("thresh %d, %d ", data->prox_threshold[0], data->prox_threshold[1]);

	ret = shub_file_write(PROX_CALIBRATION_FILE_PATH, (char *)&data->prox_threshold,
			      sizeof(data->prox_threshold), 0);

	if (ret != sizeof(data->prox_threshold)) {
		shub_errf("can't write prox cal to file");
		ret = -EIO;
	}

	return ret;
}

static ssize_t proximity_stk3328_prox_cal_store(const char *buf, size_t size, int type)
{
	int ret = 0;
	u16 prox_raw;
	u16 prev_thresh[PROX_THRESH_SIZE];
	struct proximity_data *data = get_sensor(type)->data;
	struct proximity_stk3328_data *thd_data = data->threshold_data;

	prev_thresh[PROX_THRESH_HIGH] = data->prox_threshold[PROX_THRESH_HIGH];
	prev_thresh[PROX_THRESH_LOW] = data->prox_threshold[PROX_THRESH_LOW];

	if (sysfs_streq(buf, "1")) { /* calibrate */
		shub_infof("calibrate");

		prox_raw = get_prox_raw_data();
		if (prox_raw > thd_data->prox_cal_thresh[PROX_THRESH_LOW]
		    && prox_raw <= thd_data->prox_cal_thresh[PROX_THRESH_HIGH]) {
			data->prox_threshold[PROX_THRESH_HIGH] =
						thd_data->prox_thresh_default[PROX_THRESH_HIGH] + thd_data->prox_cal_add_value;
			data->prox_threshold[PROX_THRESH_LOW] =
						thd_data->prox_thresh_default[PROX_THRESH_LOW] + thd_data->prox_cal_add_value;

			shub_infof("crosstalk = %u, threshold = %u, %u",
				  prox_raw, data->prox_threshold[PROX_THRESH_HIGH], data->prox_threshold[PROX_THRESH_LOW]);
		} else if (prox_raw > thd_data->prox_cal_thresh[PROX_THRESH_HIGH]) {
			data->prox_threshold[PROX_THRESH_HIGH] = thd_data->prox_thresh_default[PROX_THRESH_HIGH];
			data->prox_threshold[PROX_THRESH_LOW] = thd_data->prox_thresh_default[PROX_THRESH_LOW];
			shub_infof("crosstalk(%u) > %d, calibration failed",
				  prox_raw, thd_data->prox_cal_thresh[PROX_THRESH_HIGH]);
		} else {
			data->prox_threshold[PROX_THRESH_HIGH] = thd_data->prox_thresh_default[PROX_THRESH_HIGH];
			data->prox_threshold[PROX_THRESH_LOW] = thd_data->prox_thresh_default[PROX_THRESH_LOW];
			shub_infof("crosstalk(%u)", prox_raw);
		}
	} else {
		shub_errf("invalid value %d", *buf);
		ret = -EINVAL;
	}

	if (prev_thresh[PROX_THRESH_HIGH] != data->prox_threshold[PROX_THRESH_HIGH]
	    || prev_thresh[PROX_THRESH_LOW] != data->prox_threshold[PROX_THRESH_LOW]) {
		set_proximity_threshold();
		ret = save_prox_cal_threshold_data(data);
	}

	return size;
}

struct proximity_factory_chipset_funcs proximity_stk3328_ops = {
	.prox_cal_store = proximity_stk3328_prox_cal_store,
	.prox_trim_show = proximity_stk3328_prox_trim_show,
};

struct proximity_factory_chipset_funcs *get_proximity_stk3328_chipset_func(char *name)
{
	if (strcmp(name, "STK3328") != 0)
		return NULL;

	return &proximity_stk3328_ops;
}
