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

#include "../../sensor/accelerometer.h"
#include "../../sensor/folding_angle.h"
#include "../../sensorhub/shub_device.h"
#include "../../sensormanager/shub_sensor.h"
#include "../../sensormanager/shub_sensor_manager.h"
#include "../../factory/shub_factory.h"
#include "../../utility/shub_dev_core.h"
#include "../../utility/shub_utility.h"
#include "../../utility/shub_file_manager.h"
#include "../../comm/shub_comm.h"

#if defined(CONFIG_SHUB_KUNIT)
#include <kunit/mock.h>
#define __mockable __weak
#define __visible_for_testing
#else
#define __mockable
#define __visible_for_testing static
#endif

#include <linux/delay.h>
#include <linux/slab.h>

#define MAX_ACCEL_1G_8G 4096
#define MAX_ACCEL_1G_16G 2048
#define MIN_ACCEL_1G_8G -4096
#define MIN_ACCEL_1G_16G -2048
#define MAX_ACCEL_2G 8192
#define MIN_ACCEL_2G -8192
#define MAX_ACCEL_4G 16384

#define CALIBRATION_DATA_AMOUNT 20
/*************************************************************************/
/* factory Sysfs                                                         */
/*************************************************************************/

__visible_for_testing struct device *accel_sysfs_device;
__visible_for_testing struct device *accel_sub_sysfs_device;

int get_accel_type(struct device *dev)
{
	const char *name = dev->kobj.name;

	if (!strcmp(name, "accelerometer_sensor")) {
		return SENSOR_TYPE_ACCELEROMETER;
	}
	else if(!strcmp(name, "sub_accelerometer_sensor")) {
		return SENSOR_TYPE_ACCELEROMETER_SUB;
	}
	else {
		return SENSOR_TYPE_ACCELEROMETER;
	}
}

static ssize_t name_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int type = get_accel_type(dev);
	struct shub_sensor *sensor = get_sensor(type);

	if (!sensor) {
		shub_infof("sensor is null");
		return -EINVAL;
	}

	return sprintf(buf, "%s\n", sensor->spec.name);
}

static ssize_t vendor_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int type = get_accel_type(dev);
	struct shub_sensor *sensor = get_sensor(type);
	char vendor[VENDOR_MAX] = "";

	if (!sensor) {
		shub_infof("sensor is null");
		return -EINVAL;
	}

	get_sensor_vendor_name(sensor->spec.vendor, vendor);

	return sprintf(buf, "%s\n", vendor);
}

static ssize_t accel_calibration_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret;
	int type = get_accel_type(dev);
	struct shub_sensor *sensor = get_sensor(type);
	struct accelerometer_data *data;

	if (!sensor) {
		shub_infof("%d sensor is null\n", type);
		return -EINVAL;
	}

	data = sensor->data;

	ret = sensor->funcs->open_calibration_file(type);
	if (ret < 0)
		shub_errf("calibration open failed(%d)", ret);

	shub_infof("Cal data : %d %d %d - %d", data->cal_data.x, data->cal_data.y, data->cal_data.z, ret);

	return sprintf(buf, "%d %d %d %d\n", ret, data->cal_data.x, data->cal_data.y, data->cal_data.z);
}

static int accel_do_calibrate(int type, int enable)
{
	int iSum[3] = {0, };
	int ret = 0;
	uint32_t backup_sampling_period;
	uint32_t backup_max_report_latency;
	struct shub_sensor *sensor = get_sensor(type);
	struct accelerometer_data *data;
	int range = MAX_ACCEL_1G_8G;
	struct accel_event *sensor_value;

	if (!sensor) {
		shub_errf("%d sensor is NULL", type);
		return -EINVAL;
	}

	backup_sampling_period = sensor->sampling_period;
	backup_max_report_latency = sensor->max_report_latency;
	data = sensor->data;
	range = MAX_ACCEL_1G_8G;
	sensor_value = (struct accel_event *)sensor->event_buffer.value;

	if (data->range == 16) {
		range = MAX_ACCEL_1G_16G;
	}

	if (enable) {
		int count;

		data->cal_data.x = 0;
		data->cal_data.y = 0;
		data->cal_data.z = 0;
		set_accel_cal(data);

		batch_sensor(type, 10, 0);
		enable_sensor(type, NULL, 0);

		msleep(300);

		for (count = 0; count < CALIBRATION_DATA_AMOUNT; count++) {
			iSum[0] += sensor_value->x;
			iSum[1] += sensor_value->y;
			iSum[2] += sensor_value->z;
			mdelay(10);
		}

		batch_sensor(type, backup_sampling_period, backup_max_report_latency);
		disable_sensor(type, NULL, 0);

		data->cal_data.x = (iSum[0] / CALIBRATION_DATA_AMOUNT);
		data->cal_data.y = (iSum[1] / CALIBRATION_DATA_AMOUNT);
		data->cal_data.z = (iSum[2] / CALIBRATION_DATA_AMOUNT);

		if (data->cal_data.z > 0)
			data->cal_data.z -= range;
		else if (data->cal_data.z < 0)
			data->cal_data.z += range;

	} else {
		data->cal_data.x = 0;
		data->cal_data.y = 0;
		data->cal_data.z = 0;
	}

	shub_infof("do accel calibrate %d, %d, %d", data->cal_data.x, data->cal_data.y, data->cal_data.z);

	ret = shub_file_write(ACCEL_CALIBRATION_FILE_PATH, (char *)&data->cal_data, sizeof(data->cal_data), 0);
	if (ret != sizeof(data->cal_data)) {
		shub_errf("Can't write the accelcal to file");
		ret = -EIO;
	}

	set_accel_cal(data);

	return ret;
}

static ssize_t accel_calibration_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int ret = 0;
	int64_t enable;
	int type = get_accel_type(dev);
	struct shub_sensor *sensor = get_sensor(type);

	if (!sensor) {
		shub_errf("%d sensor is NULL", type);
		return -EINVAL;
	}

	ret = kstrtoll(buf, 10, &enable);
	if (ret < 0)
		return ret;

	ret = accel_do_calibrate(type, (int)enable);
	if (ret < 0)
		shub_errf("accel_do_calibrate() failed");

	return size;
}

static ssize_t raw_data_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct accel_event *sensor_value;
	int type = get_accel_type(dev);

	if (!get_sensor_probe_state(type)) {
		shub_errf("sensor is not probed!");
		return 0;
	}

	sensor_value = (struct accel_event *)(get_sensor_event(type)->value);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n", sensor_value->x, sensor_value->y,
			sensor_value->z);
}

static ssize_t accel_reactive_alert_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	bool success = false;
	int type = get_accel_type(dev);
	struct shub_sensor *sensor = get_sensor(type);
	struct accelerometer_data *data;

	if (!sensor) {
		shub_errf("%d sensor is NULL", type);
		return -EINVAL;
	}

	data = sensor->data;

	if (data->is_accel_alert == true)
		success = true;
	else
		success = false;

	data->is_accel_alert = false;

	return sprintf(buf, "%u\n", success);
}

static ssize_t accel_reactive_alert_store(struct device *dev, struct device_attribute *attr, const char *buf,
					  size_t size)
{
	int ret = 0;
	char *buffer = NULL;
	int buffer_length = 0;
	int type = get_accel_type(dev);
	struct shub_sensor *sensor = get_sensor(type);
	struct accelerometer_data *data;

	if (!sensor) {
		shub_errf("%d sensor is NULL", type);
		return -EINVAL;
	}

	data = sensor->data;

	if (sysfs_streq(buf, "1")) {
		shub_infof("on");
	} else if (sysfs_streq(buf, "0")) {
		shub_infof("off");
	} else if (sysfs_streq(buf, "2")) {
		shub_infof("factory");

		data->is_accel_alert = 0;

		ret = shub_send_command_wait(CMD_GETVALUE, type, ACCELEROMETER_REACTIVE_ALERT, 3000, NULL, 0,
					     &buffer, &buffer_length, true);

		if (ret < 0) {
			shub_errf("shub_send_command_wait Fail %d", ret);
			goto exit;
		}

		if (buffer_length < 1) {
			shub_errf("length err %d", buffer_length);
			ret = -EINVAL;
			goto exit;
		}

		data->is_accel_alert = *buffer;

		shub_infof("factory test success!");
	} else {
		shub_errf("invalid value %d", *buf);
		ret = -EINVAL;
	}

exit:
	kfree(buffer);
	return size;
}

static ssize_t accel_lowpassfilter_store(struct device *dev, struct device_attribute *attr, const char *buf,
					 size_t size)
{
	int ret = 0;
	int new_enable = 1;
	int type = get_accel_type(dev);
	char temp = 0;

	if (sysfs_streq(buf, "1"))
		new_enable = 1;
	else if (sysfs_streq(buf, "0"))
		new_enable = 0;
	else
		shub_infof(" invalid value!");

	temp = new_enable;

	ret = shub_send_command(CMD_SETVALUE, type, ACCELEROMETER_LPF_ON_OFF, &temp, sizeof(temp));

	if (ret < 0)
		shub_errf("shub_send_command Fail %d", ret);

	return size;
}


static ssize_t selftest_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char *buffer = NULL;
	int buffer_length = 0;
	int type = get_accel_type(dev);
	s8 init_status = 0, result = -1;
	u16 diff_axis[3] = {0, };
	u16 shift_ratio_N[3] = {0, };
	int ret = 0;

	ret = shub_send_command_wait(CMD_GETVALUE, type, SENSOR_FACTORY, 3000, NULL, 0, &buffer,
				     &buffer_length, true);

	if (ret < 0) {
		shub_errf("shub_send_command_wait Fail %d", ret);
		ret = sprintf(buf, "%d,%d,%d,%d\n", -5, 0, 0, 0);
		return ret;
	}

	if (buffer_length != 14) {
		shub_errf("length err %d", buffer_length);
		ret = sprintf(buf, "%d,%d,%d,%d\n", -5, 0, 0, 0);
		kfree(buffer);

		return -EINVAL;
	}

	init_status = buffer[0];
	diff_axis[0] = ((s16)(buffer[2] << 8)) + buffer[1];
	diff_axis[1] = ((s16)(buffer[4] << 8)) + buffer[3];
	diff_axis[2] = ((s16)(buffer[6] << 8)) + buffer[5];

	/* negative axis */

	shift_ratio_N[0] = ((s16)(buffer[8] << 8)) + buffer[7];
	shift_ratio_N[1] = ((s16)(buffer[10] << 8)) + buffer[9];
	shift_ratio_N[2] = ((s16)(buffer[12] << 8)) + buffer[11];
	result = buffer[13];

	shub_infof("%d, %d, %d, %d, %d, %d, %d, %d\n", init_status, result, diff_axis[0], diff_axis[1], diff_axis[2],
		   shift_ratio_N[0], shift_ratio_N[1], shift_ratio_N[2]);

	ret = sprintf(buf, "%d,%d,%d,%d,%d,%d,%d\n", result, diff_axis[0], diff_axis[1], diff_axis[2], shift_ratio_N[0],
		      shift_ratio_N[1], shift_ratio_N[2]);

	kfree(buffer);

	return ret;
}

static ssize_t ref_angle_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char *buffer = NULL;
	int buffer_length = 0;
	int ret = 0;
	short angle = 0;

	ret = shub_send_command_wait(CMD_GETVALUE, SENSOR_TYPE_FOLDING_ANGLE, FA_SUBCMD_REF_ANGLE, 1000,
				     NULL, 0, &buffer, &buffer_length, true);
	if (ret < 0) {
		shub_errf("shub_send_command_wait Fail %d", ret);
		ret = sprintf(buf, "-1\n");
		return ret;
	}

	if (buffer_length != 2) {
		shub_errf("length err %d", buffer_length);
		ret = sprintf(buf, "-1\n");
		kfree(buffer);

		return -EINVAL;
	}

	memcpy(&angle, buffer, sizeof(angle));
	shub_infof("%d\n", angle);

	if (angle >= 0)
		ret = sprintf(buf, "%d,0\n", angle);
	else
		ret = sprintf(buf, "-1\n");

	kfree(buffer);

	return ret;
}

static DEVICE_ATTR_RO(name);
static DEVICE_ATTR_RO(vendor);
static DEVICE_ATTR_RO(raw_data);
static DEVICE_ATTR(calibration, 0664, accel_calibration_show, accel_calibration_store);
static DEVICE_ATTR(reactive_alert, 0664, accel_reactive_alert_show, accel_reactive_alert_store);
static DEVICE_ATTR(lowpassfilter, 0220, NULL, accel_lowpassfilter_store);
static DEVICE_ATTR_RO(selftest);
static DEVICE_ATTR_RO(ref_angle);

__visible_for_testing struct device_attribute *acc_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_calibration,
	&dev_attr_raw_data,
	&dev_attr_reactive_alert,
	&dev_attr_lowpassfilter,
	&dev_attr_selftest,
	NULL,
};

void initialize_accelerometer_sysfs(struct device **dev, struct device_attribute *attrs[], char *name)
{
	int ret;

	ret = sensor_device_create(&(*dev), NULL, name);
	if (ret < 0) {
		shub_errf("fail to create %s sysfs device", name);
		return;
	}

	ret = add_sensor_device_attr(*dev, attrs);
	if (ret < 0) {
		shub_errf("fail to add %s sysfs device attr", name);
		return;
	}
}

void remove_accelerometer_sysfs(struct device **dev, struct device_attribute *attrs[])
{
	remove_sensor_device_attr(*dev, attrs);
	sensor_device_unregister(*dev);
	*dev = NULL;
}

void initialize_accelerometer_factory(bool en, int mode)
{
	if (en)
		initialize_accelerometer_sysfs(&accel_sysfs_device, acc_attrs, "accelerometer_sensor");
	else {
		if (mode == INIT_FACTORY_MODE_REMOVE_EMPTY && get_sensor(SENSOR_TYPE_ACCELEROMETER))
			shub_infof("support accelerometer sysfs");
		else
			remove_accelerometer_sysfs(&accel_sysfs_device, acc_attrs);
	}
}

/**
 * accelerometer_sub
*/
__visible_for_testing struct device_attribute *acc_sub_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_calibration,
	&dev_attr_raw_data,
	&dev_attr_reactive_alert,
	&dev_attr_lowpassfilter,
	&dev_attr_selftest,
	&dev_attr_ref_angle,
	NULL,
};

void initialize_accelerometer_sub_factory(bool en, int mode)
{
	if (en)
		initialize_accelerometer_sysfs(&accel_sub_sysfs_device, acc_sub_attrs, "sub_accelerometer_sensor");
	else {
		if (mode == INIT_FACTORY_MODE_REMOVE_EMPTY && get_sensor(SENSOR_TYPE_ACCELEROMETER_SUB)) {
			shub_infof("support accelerometer sub sysfs");
		} else {
			remove_accelerometer_sysfs(&accel_sub_sysfs_device, acc_sub_attrs);
		}
	}
}

