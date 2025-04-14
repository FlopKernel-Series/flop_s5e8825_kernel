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

#include "../comm/shub_comm.h"
#include "../sensorhub/shub_device.h"
#include "../sensormanager/shub_sensor.h"
#include "../sensormanager/shub_sensor_manager.h"
#include "../utility/shub_dev_core.h"
#include "../utility/shub_utility.h"
#include "../utility/shub_file_manager.h"
#include "../utility/shub_wakelock.h"
#include "accelerometer.h"
#include "gyroscope.h"

#include <linux/kernel.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>

#define GYRO_CALIBRATION_FILE_PATH "/efs/FactoryApp/gyro_cal_data"

void parse_dt_gyroscope(struct device *dev, int type)
{
	struct accelerometer_data *acc_data = get_sensor(type == SENSOR_TYPE_GYROSCOPE ? SENSOR_TYPE_ACCELEROMETER :
								 SENSOR_TYPE_ACCELEROMETER_SUB)->data;
	struct gyroscope_data *data = get_sensor(type)->data;

	data->position = acc_data->position;
	shub_infof("type[%d], position[%d]", type, data->position);
}

int set_gyro_position(int type, int position)
{
	int ret = 0;
	struct gyroscope_data *data = get_sensor(type)->data;

	data->position = position;

	ret = shub_send_command(CMD_SETVALUE, type, SENSOR_AXIS, (char *)&(data->position),
				sizeof(data->position));
	if (ret < 0) {
		shub_errf("CMD fail %d\n", ret);
		return ret;
	}

	shub_infof("G : %u", data->position);

	return ret;
}

int get_gyro_position(int type)
{
	struct gyroscope_data *data = get_sensor(type)->data;

	return data->position;
}

static int open_gyro_calibration_file(int type)
{
	int ret = 0;
	struct gyroscope_data *data = get_sensor(type)->data;

	shub_infof();

	ret = shub_file_read(GYRO_CALIBRATION_FILE_PATH, (char *)&data->cal_data, sizeof(data->cal_data), 0);
	if (ret != sizeof(data->cal_data))
		ret = -EIO;

	shub_infof("open gyro calibration %d, %d, %d", data->cal_data.x, data->cal_data.y, data->cal_data.z);

	return ret;
}

int save_gyro_calibration_file(s16 *cal_data)
{
	int ret;
	struct gyroscope_data *data = get_sensor(SENSOR_TYPE_GYROSCOPE)->data;

	data->cal_data.x = cal_data[0];
	data->cal_data.y = cal_data[1];
	data->cal_data.z = cal_data[2];

	shub_info("do gyro calibrate %d, %d, %d", data->cal_data.x, data->cal_data.y, data->cal_data.z);

	ret = shub_file_write_no_wait(GYRO_CALIBRATION_FILE_PATH, (char *)&data->cal_data, sizeof(data->cal_data), 0);
	if (ret != sizeof(data->cal_data)) {
		shub_err("Can't write gyro cal to file");
		ret = -EIO;
	}

	return ret;
}

int parsing_gyro_calibration(char *dataframe, int *index, int frame_len)
{
	s16 caldata[3] = {0, };

	if (*index + sizeof(caldata) > frame_len) {
		shub_errf("parsing error");
		return -EINVAL;
	}

	shub_infof("Gyro caldata received from MCU");
	memcpy(caldata, dataframe + (*index), sizeof(caldata));
	shub_wake_lock();
	save_gyro_calibration_file(caldata);
	shub_wake_unlock();
	(*index) += sizeof(caldata);

	return 0;
}

int set_gyro_cal(int type, struct gyroscope_data *data)
{
	int ret = 0;
	s16 gyro_cal[3] = {0, };

	if (!get_sensor_probe_state(type)) {
		shub_infof("[SHUB] Skip this function!!!, gyro sensor is not connected\n");
		return ret;
	}

	gyro_cal[0] = data->cal_data.x;
	gyro_cal[1] = data->cal_data.y;
	gyro_cal[2] = data->cal_data.z;

	ret = shub_send_command(CMD_SETVALUE, type, CAL_DATA, (char *)gyro_cal, sizeof(gyro_cal));

	if (ret < 0) {
		shub_errf("CMD Fail %d", ret);
		return ret;
	}

	shub_infof("set temp gyro cal data %d, %d, %d\n", gyro_cal[0], gyro_cal[1], gyro_cal[2]);
	shub_infof("set gyro cal data %d, %d, %d\n", data->cal_data.x, data->cal_data.y, data->cal_data.z);

	return ret;
}

int sync_gyroscope_status(int type)
{
	int ret = 0;
	struct gyroscope_data *data = get_sensor(type)->data;

	shub_infof();
	ret = set_gyro_position(type, data->position);
	if (ret < 0) {
		shub_errf("set_position failed");
		return ret;
	}

	ret = set_gyro_cal(type, data);
	if (ret < 0) {
		shub_errf("set_gyro_cal failed");
		return ret;
	}

	return ret;
}

void print_gyroscope_debug(int type)
{
	struct shub_sensor *sensor = get_sensor(type);
	struct sensor_event *event = &(sensor->last_event_buffer);
	struct gyro_event *sensor_value = (struct gyro_event *)(event->value);

	shub_info("%s(%u) : %d, %d, %d (%lld) (%ums, %dms)", sensor->name, sensor->type, sensor_value->x,
		  sensor_value->y, sensor_value->z, event->timestamp, sensor->sampling_period,
		  sensor->max_report_latency);
}

static struct gyroscope_data gyroscope_data;
static struct sensor_funcs gyroscope_sensor_func = {
	.sync_status = sync_gyroscope_status,
	.set_position = set_gyro_position,
	.get_position = get_gyro_position,
	.print_debug = print_gyroscope_debug,
	.parsing_data = parsing_gyro_calibration,
	.open_calibration_file = open_gyro_calibration_file,
	.parse_dt = parse_dt_gyroscope,
};

int init_gyroscope(bool en)
{
	int ret = 0;
	struct shub_sensor *sensor = get_sensor(SENSOR_TYPE_GYROSCOPE);

	if (!sensor)
		return 0;

	if (en) {
		ret = init_default_func(sensor, "gyro_sensor", 6, 6, sizeof(struct gyro_event));

		sensor->report_mode_continuous = true;
		sensor->data = (void *)&gyroscope_data;
		sensor->funcs = &gyroscope_sensor_func;
	} else {
		destroy_default_func(sensor);
	}

	return ret;
}

static struct gyroscope_data gyroscope_sub_data;

int init_gyroscope_sub(bool en)
{
	int ret = 0;
	struct shub_sensor *sensor = get_sensor(SENSOR_TYPE_GYROSCOPE_SUB);

	if (!sensor)
		return 0;

	if (en) {
		ret = init_default_func(sensor, "gyro_sub_sensor", 6, 6, sizeof(struct gyro_event));

		sensor->report_mode_continuous = true;
		sensor->data = (void *)&gyroscope_sub_data;
		sensor->funcs = &gyroscope_sensor_func;
	} else {
		destroy_default_func(sensor);
	}

	return ret;
}

int init_interrupt_gyroscope(bool en)
{
	int ret = 0;
	struct shub_sensor *sensor = get_sensor(SENSOR_TYPE_INTERRUPT_GYRO);

	if (!sensor)
		return 0;

	if (en) {
		ret = init_default_func(sensor, "interrupt_gyro_sensor", 6, 6, sizeof(struct gyro_event));
	} else {
		destroy_default_func(sensor);
	}

	return ret;
}

int init_vdis_gyroscope(bool en)
{
	int ret = 0;
	struct shub_sensor *sensor = get_sensor(SENSOR_TYPE_VDIS_GYROSCOPE);

	if (!sensor)
		return 0;

	if (en) {
		ret = init_default_func(sensor, "vdis_gyro_sensor", 6, 6, sizeof(struct gyro_event));
	} else {
		destroy_default_func(sensor);
	}

	return ret;
}

int init_super_steady_gyroscope(bool en)
{
	int ret = 0;
	struct shub_sensor *sensor = get_sensor(SENSOR_TYPE_SUPER_STEADY_GYROSCOPE);

	if (!sensor)
		return 0;

	if (en) {
		ret = init_default_func(sensor, "super_steady_gyro_sensor", 6, 6, sizeof(struct gyro_event));
	} else {
		destroy_default_func(sensor);
	}

	return ret;
}

