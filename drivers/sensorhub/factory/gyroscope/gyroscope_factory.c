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

#include "../../sensor/gyroscope.h"
#include "../../sensorhub/shub_device.h"
#include "../../sensormanager/shub_sensor.h"
#include "../../sensormanager/shub_sensor_manager.h"
#include "../../factory/shub_factory.h"
#include "../../utility/shub_dev_core.h"
#include "../../utility/shub_utility.h"
#include "../../comm/shub_comm.h"
#include "gyroscope_factory.h"

#include <linux/slab.h>

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

#define SELFTEST_REVISED 1

__visible_for_testing struct device *gyro_sysfs_device;
__visible_for_testing struct device *gyro_sub_sysfs_device;

static struct gyroscope_factory_chipset_funcs *chipset_func;
typedef struct gyroscope_factory_chipset_funcs* (*get_chipset_funcs_ptr)(char *);
get_chipset_funcs_ptr get_gyroscope_factory_chipset_funcs_ptr[] = {
	get_gyroscope_icm42605m_chipset_func,
	get_gyroscope_lsm6dsl_chipset_func,
	get_gyroscope_lsm6dsotr_chipset_func,
	get_gyroscope_lsm6dsvtr_chipset_func,
	get_gyroscope_icm42632m_chipset_func,
};

static ssize_t name_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct shub_sensor *sensor = get_sensor(SENSOR_TYPE_GYROSCOPE);

	if (!sensor) {
		shub_infof("sensor is null");
		return -EINVAL;
	}

	return sprintf(buf, "%s\n", sensor->spec.name);
}

static ssize_t vendor_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct shub_sensor *sensor = get_sensor(SENSOR_TYPE_GYROSCOPE);
	char vendor[VENDOR_MAX] = "";

	if (!sensor) {
		shub_infof("sensor is null");
		return -EINVAL;
	}

	get_sensor_vendor_name(sensor->spec.vendor, vendor);

	return sprintf(buf, "%s\n", vendor);
}

int get_gyro_type(struct device *dev)
{
	const char *name = dev->kobj.name;

	if (!strcmp(name, "gyro_sensor")) {
		return SENSOR_TYPE_GYROSCOPE;
	}
	else if(!strcmp(name, "sub_gyro_sensor")) {
		return SENSOR_TYPE_GYROSCOPE_SUB;
	}
	else {
		return SENSOR_TYPE_GYROSCOPE;
	}
}


static ssize_t power_off_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	shub_infof();

	return sprintf(buf, "%d\n", 1);
}

static ssize_t power_on_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	shub_infof();

	return sprintf(buf, "%d\n", 1);
}

static ssize_t temperature_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char *buffer = NULL;
	int buffer_length = 0;
	unsigned char reg[2] = {0, };
	short temperature = 0;
	int type = get_gyro_type(dev);
	int ret = 0;

	ret = shub_send_command_wait(CMD_GETVALUE, type, GYROSCOPE_TEMPERATURE_FACTORY,
				     3000, NULL, 0, &buffer, &buffer_length, true);

	if (ret < 0) {
		shub_errf("shub_send_command_wait Fail %d", ret);
		goto exit;
	}

	if (buffer_length < 2) {
		shub_errf("length err %d", buffer_length);
		ret = -EINVAL;
		goto exit;
	}

	reg[0] = buffer[1];
	reg[1] = buffer[0];
	temperature = (short)(((reg[0]) << 8) | reg[1]);
	shub_infof("%d", temperature);

	ret = sprintf(buf, "%d\n", temperature);

exit:
	kfree(buffer);

	return ret;
}

static ssize_t selftest_dps_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (!buf)
		return -EINVAL;

	return sprintf(buf, "0\n");
}

static ssize_t selftest_dps_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	shub_info("Do not support gyro dps selftest");
	return size;
}

static ssize_t selftest_revised_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (!buf)
		return -EINVAL;

	return snprintf(buf, PAGE_SIZE, "%d\n", SELFTEST_REVISED);
}

static ssize_t selftest_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int type = get_gyro_type(dev);

	if (chipset_func && chipset_func->selftest)
		return chipset_func->selftest(type, buf);
	else
		return -EINVAL;
}

static DEVICE_ATTR_RO(name);
static DEVICE_ATTR_RO(vendor);
static DEVICE_ATTR_RO(power_off);
static DEVICE_ATTR_RO(power_on);
static DEVICE_ATTR_RO(temperature);
static DEVICE_ATTR_RO(selftest_revised);
static DEVICE_ATTR(selftest_dps, 0664, selftest_dps_show, selftest_dps_store);
static DEVICE_ATTR_RO(selftest);

__visible_for_testing struct device_attribute *gyro_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_power_on,
	&dev_attr_power_off,
	&dev_attr_temperature,
	&dev_attr_selftest_dps,
	&dev_attr_selftest_revised,
	&dev_attr_selftest,
	NULL,
};

__visible_for_testing struct device_attribute *gyro_sub_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_power_on,
	&dev_attr_power_off,
	&dev_attr_temperature,
	&dev_attr_selftest_dps,
	&dev_attr_selftest,
	NULL,
};

void initialize_gyroscope_sysfs(struct device **dev, struct device_attribute *attrs[], char *name)
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

void remove_gyroscope_sysfs(struct device **dev, struct device_attribute *attrs[])
{
	remove_sensor_device_attr(*dev, attrs);
	sensor_device_unregister(*dev);
	*dev = NULL;
}

void initialize_gyroscope_chipset_func(struct device **dev)
{
	int i;

	struct shub_sensor *sensor = get_sensor(get_gyro_type(*dev));

	for (i = 0; i < ARRAY_SIZE(get_gyroscope_factory_chipset_funcs_ptr); i++) {
		chipset_func = get_gyroscope_factory_chipset_funcs_ptr[i](sensor->spec.name);
		if(!chipset_func)
			continue;

		shub_infof("support %s sysfs", sensor->name);
		break;
	}

	if (!chipset_func) {
		if (sensor->type == SENSOR_TYPE_GYROSCOPE)
			remove_gyroscope_sysfs(&gyro_sysfs_device, gyro_attrs);
		else
			remove_gyroscope_sysfs(&gyro_sub_sysfs_device, gyro_sub_attrs);
	}
}

void initialize_gyroscope_factory(bool en, int mode)
{
	if (en)
		initialize_gyroscope_sysfs(&gyro_sysfs_device, gyro_attrs, "gyro_sensor");
	else {
		if (mode == INIT_FACTORY_MODE_REMOVE_EMPTY && get_sensor(SENSOR_TYPE_GYROSCOPE))
			initialize_gyroscope_chipset_func(&gyro_sysfs_device);
		else
			remove_gyroscope_sysfs(&gyro_sysfs_device, gyro_attrs);
	}
}

void initialize_gyroscope_sub_factory(bool en, int mode)
{
	if (en)
		initialize_gyroscope_sysfs(&gyro_sub_sysfs_device, gyro_sub_attrs, "sub_gyro_sensor");
	else {
		if (mode == INIT_FACTORY_MODE_REMOVE_EMPTY && get_sensor(SENSOR_TYPE_GYROSCOPE_SUB))
			initialize_gyroscope_chipset_func(&gyro_sub_sysfs_device);
		else
			remove_gyroscope_sysfs(&gyro_sub_sysfs_device, gyro_sub_attrs);

	}
}