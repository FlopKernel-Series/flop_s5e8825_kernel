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
#include "../../factory/shub_factory.h"
#include "../../sensor/pressure.h"
#include "../../sensorhub/shub_device.h"
#include "../../sensormanager/shub_sensor.h"
#include "../../sensormanager/shub_sensor_manager.h"
#include "../../utility/shub_dev_core.h"
#include "../../utility/shub_utility.h"
#include "pressure_factory.h"

#include <linux/slab.h>

#define PR_ABS_MAX 8388607 /* 24 bit 2'compl */
#define PR_ABS_MIN -8388608

#if defined(CONFIG_SHUB_KUNIT)
#include <kunit/mock.h>
#define __mockable __weak
#define __visible_for_testing
#else
#define __mockable
#define __visible_for_testing static
#endif

/*************************************************************************/
/* factory Sysfs                                                         */
/*************************************************************************/

__visible_for_testing struct device *pressure_sysfs_device;

static struct pressure_factory_chipset_funcs *chipset_func;
typedef struct pressure_factory_chipset_funcs* (*get_chipset_funcs_ptr)(char *);
get_chipset_funcs_ptr get_pressure_factory_chipset_funcs_ptr[] = {
	get_pressure_lps22hh_chipset_func,
	get_pressure_lps22df_chipset_func,
	get_pressure_lps25h_chipset_func,
	get_pressure_bmp580_chipset_func,
};

static ssize_t name_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct shub_sensor *sensor = get_sensor(SENSOR_TYPE_PRESSURE);

	if (!sensor) {
		shub_infof("sensor is null");
		return -EINVAL;
	}
	
	return sprintf(buf, "%s\n", sensor->spec.name);
}

static ssize_t vendor_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct shub_sensor *sensor = get_sensor(SENSOR_TYPE_PRESSURE);
	char vendor[VENDOR_MAX] = "";

	if (!sensor) {
		shub_infof("sensor is null");
		return -EINVAL;
	}
	
	get_sensor_vendor_name(sensor->spec.vendor, vendor);

	return sprintf(buf, "%s\n", vendor);
}

static ssize_t sea_level_pressure_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int ret = 0;
	struct shub_sensor *sensor = get_sensor(SENSOR_TYPE_PRESSURE);
	struct pressure_event *sensor_value;

	if (!sensor) {
		shub_infof("sensor is null");
		return -EINVAL;
	}

	sensor_value = (struct pressure_event *)(get_sensor_event(SENSOR_TYPE_PRESSURE)->value);

	ret = sscanf(buf, "%9d", &sensor_value->pressure_sealevel);
	if (ret < 0) {
		shub_errf("- failed = %d", ret);
		return -EINVAL;
	}

	if (sensor_value->pressure_sealevel == 0) {
		shub_infof("our->temperature = 0\n");
		sensor_value->pressure_sealevel = -1;
	}

	shub_infof("sea_level_pressure = %d\n", sensor_value->pressure_sealevel);

	return size;
}

static ssize_t pressure_calibration_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct shub_sensor *sensor = get_sensor(SENSOR_TYPE_PRESSURE);
	struct pressure_event *sensor_value;

	if (!sensor) {
		shub_infof("sensor is null");
		return -EINVAL;
	}

	sensor_value = (struct pressure_event *)(get_sensor_event(SENSOR_TYPE_PRESSURE)->value);
	sensor->funcs->open_calibration_file(SENSOR_TYPE_PRESSURE);

	return sprintf(buf, "%d\n", sensor_value->pressure_cal);
}

static ssize_t pressure_calibration_store(struct device *dev, struct device_attribute *attr, const char *buf,
					 size_t size)
{
	int pressure_cal = 0;
	int ret = 0;
	struct shub_sensor *sensor = get_sensor(SENSOR_TYPE_PRESSURE);
	struct pressure_event *sensor_value;

	if (!sensor) {
		shub_infof("sensor is null");
		return -EINVAL;
	}

	sensor_value = (struct pressure_event *)(get_sensor_event(SENSOR_TYPE_PRESSURE)->value);

	ret = kstrtoint(buf, 10, &pressure_cal);
	if (ret < 0) {
		shub_errf("kstrtoint failed.(%d)", ret);
		return ret;
	}

	if (pressure_cal < PR_ABS_MIN || pressure_cal > PR_ABS_MAX)
		return -EINVAL;

	sensor_value->pressure_cal = (s32)pressure_cal;

	return size;
}

static ssize_t pressure_selftest_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char *buffer = NULL;
	int buffer_length = 0;
	int ret = 0;

	ret = shub_send_command_wait(CMD_GETVALUE, SENSOR_TYPE_PRESSURE, SENSOR_FACTORY, 3000, NULL, 0, &buffer,
				     &buffer_length, true);

	if (ret < 0) {
		shub_errf("shub_send_command_wait Fail %d", ret);
		goto exit;
	}

	shub_infof("%d", *buffer);
	ret = snprintf(buf, PAGE_SIZE, "%d", *buffer);

exit:
	if (buffer != NULL) {
		kfree(buffer);
	}

	return ret;
}

static ssize_t pressure_sw_offset_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct shub_sensor *sensor = get_sensor(SENSOR_TYPE_PRESSURE);
	struct pressure_data *data;

	if (!sensor) {
		shub_infof("sensor is null");
		return -EINVAL;
	}

	data = sensor->data;

	return sprintf(buf, "%d\n", data->sw_offset);
}

static ssize_t pressure_sw_offset_store(struct device *dev, struct device_attribute *attr, const char *buf,
					 size_t size)
{
	struct shub_sensor *sensor = get_sensor(SENSOR_TYPE_PRESSURE);
	struct pressure_data *data;
	int sw_offset = 0;
	int ret;

	if (!sensor) {
		shub_infof("sensor is null");
		return -EINVAL;
	}

	data = sensor->data;

	ret = kstrtoint(buf, 10, &sw_offset);
	if (ret < 0) {
		shub_errf("kstrtoint failed.(%d)", ret);
		return ret;
	}

	data->sw_offset = sw_offset;

	shub_infof("%d", sw_offset);

	return size;
}

static ssize_t pressure_temperature_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (!chipset_func || !chipset_func->temperature_show)
		return -EINVAL;
	
	return chipset_func->temperature_show(buf);
}

static ssize_t pressure_esn_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (!chipset_func || !chipset_func->esn_show)
		return -EINVAL;

	return chipset_func->esn_show(buf);
}

static DEVICE_ATTR(name, S_IRUGO, name_show, NULL);
static DEVICE_ATTR(vendor, S_IRUGO, vendor_show, NULL);
static DEVICE_ATTR(calibration, S_IRUGO | S_IWUSR | S_IWGRP, pressure_calibration_show, pressure_calibration_store);
static DEVICE_ATTR(sea_level_pressure, S_IWUSR | S_IWGRP, NULL, sea_level_pressure_store);
static DEVICE_ATTR(selftest, S_IRUGO, pressure_selftest_show, NULL);
static DEVICE_ATTR(sw_offset, S_IRUGO | S_IWUSR | S_IWGRP, pressure_sw_offset_show, pressure_sw_offset_store);
static DEVICE_ATTR(temperature, S_IRUGO, pressure_temperature_show, NULL);
static DEVICE_ATTR(esn, S_IRUGO, pressure_esn_show, NULL);

__visible_for_testing struct device_attribute *pressure_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_calibration,
	&dev_attr_sea_level_pressure,
	&dev_attr_selftest,
	&dev_attr_sw_offset,
	&dev_attr_temperature,
	&dev_attr_esn,
	NULL,
};

void initialize_pressure_sysfs(void)
{
	int ret;

	ret = sensor_device_create(&pressure_sysfs_device, NULL, "barometer_sensor");
	if (ret < 0) {
		shub_errf("fail to creat barometer_sensor sysfs device");
		return;
	}

	ret = add_sensor_device_attr(pressure_sysfs_device, pressure_attrs);
	if (ret < 0) {
		shub_errf("fail to add barometer_sensor sysfs device attr");
		return;
	}
}

void remove_pressure_sysfs(void)
{
	remove_sensor_device_attr(pressure_sysfs_device, pressure_attrs);
	sensor_device_unregister(pressure_sysfs_device);
	pressure_sysfs_device = NULL;
}

void remove_pressure_empty_sysfs(void)
{
	struct shub_sensor *sensor = get_sensor(SENSOR_TYPE_PRESSURE);

	if (strcmp(sensor->spec.name, "BMP580") == 0 || strcmp(sensor->spec.name, "LPS22DFTR") == 0) {
		// support esn
	} else {
		device_remove_file(pressure_sysfs_device, &dev_attr_esn);
	}
}

void initialize_pressure_factory_chipset_func(void)
{
	int i;

	struct shub_sensor *sensor = get_sensor(SENSOR_TYPE_PRESSURE);

	for (i = 0; i < ARRAY_SIZE(get_pressure_factory_chipset_funcs_ptr); i++) {
		chipset_func = get_pressure_factory_chipset_funcs_ptr[i](sensor->spec.name);
		if(!chipset_func)
			continue;

		shub_infof("support pressure sysfs");
		break;
	}

	if (!chipset_func)
		remove_pressure_sysfs();
}

void initialize_pressure_factory(bool en, int mode)
{
	if (en)
		initialize_pressure_sysfs();
	else {
		if (mode == INIT_FACTORY_MODE_REMOVE_EMPTY && get_sensor(SENSOR_TYPE_PRESSURE)) {
			initialize_pressure_factory_chipset_func();
			remove_pressure_empty_sysfs();
		} else {
			remove_pressure_sysfs();
		}
	}
}
