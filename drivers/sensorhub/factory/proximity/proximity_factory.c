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

#include <linux/device.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/of.h>

#include "proximity_factory.h"
#include "../../sensor/proximity.h"
#include "../../sensorhub/shub_device.h"
#include "../../sensormanager/shub_sensor.h"
#include "../../sensormanager/shub_sensor_manager.h"
#include "../../factory/shub_factory.h"
#include "../../utility/shub_dev_core.h"
#include "../../utility/shub_utility.h"

#if defined(CONFIG_SHUB_KUNIT)
#include <kunit/mock.h>
#define __mockable __weak
#define __visible_for_testing
#else
#define __mockable
#define __visible_for_testing static
#endif

int get_prox_type(void) {
	u8 fstate = get_shub_data()->fac_fstate;

	switch (fstate) {
	case FAC_FSTATE_MAIN:
		return SENSOR_TYPE_PROXIMITY;
	case FAC_FSTATE_FOLDERBLE_SUB:
	case FAC_FSTATE_TABLET_SUB:
		return SENSOR_TYPE_SUB_PROXIMITY;
	default:
		return SENSOR_TYPE_PROXIMITY;
	}
}

/*************************************************************************/
/* factory Sysfs                                                         */
/*************************************************************************/

__visible_for_testing struct device *proximity_sysfs_device;

static struct proximity_factory_chipset_funcs *chipset_func;
typedef struct proximity_factory_chipset_funcs* (*get_chipset_funcs_ptr)(char *);
get_chipset_funcs_ptr get_proximity_factory_chipset_funcs_ptr[] = {
	get_proximity_gp2ap110s_chipset_func,
	get_proximity_stk3x6x_chipset_func,
	get_proximity_stk3328_chipset_func,
	get_proximity_tmd3725_chipset_func,
	get_proximity_tmd4912_chipset_func,
	get_proximity_stk3391x_chipset_func,
	get_proximity_stk33512_chipset_func,
	get_proximity_stk3afx_chipset_func,
	get_proximity_tmd4913_chipset_func,
	get_proximity_tmd4914_chipset_func,
};

static u32 position[6];

static ssize_t name_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int type = get_prox_type();
	struct shub_sensor *sensor = get_sensor(type);

	if (!sensor) {
		shub_infof("sensor is null");
		return -EINVAL;
	}

	return sprintf(buf, "%s\n", sensor->spec.name);
}

static ssize_t vendor_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int type = get_prox_type();
	struct shub_sensor *sensor = get_sensor(type);
	char vendor[VENDOR_MAX] = "";

	if (!sensor) {
		shub_infof("sensor is null");
		return -EINVAL;
	}

	get_sensor_vendor_name(sensor->spec.vendor, vendor);

	return sprintf(buf, "%s\n", vendor);
}

static ssize_t prox_probe_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	bool probe_pass_fail;

	probe_pass_fail = get_sensor_probe_state(SENSOR_TYPE_PROXIMITY);
	return snprintf(buf, PAGE_SIZE, "%d\n", probe_pass_fail);
}


#define PROX_THRESH_DUMMY_HIGH 8000
#define PROX_THRESH_DUMMY_LOW  2200

static ssize_t thresh_high_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", PROX_THRESH_DUMMY_HIGH);
}

static ssize_t thresh_low_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u\n", PROX_THRESH_DUMMY_LOW);
}

u16 get_prox_raw_data(void)
{
	u16 raw_data = 0;
	s32 ms_delay = 20;
	char temp_buf[8] = { 0, };
	struct shub_sensor *sensor = get_sensor(SENSOR_TYPE_PROXIMITY_RAW);
	struct prox_raw_event *sensor_value;

	if (!sensor) {
		shub_infof("sensor is null");
		return 0;
	}

	sensor_value = (struct prox_raw_event *)(get_sensor_event(SENSOR_TYPE_PROXIMITY_RAW)->value);

	memcpy(&temp_buf[0], &ms_delay, 4);

	if (!get_sensor_enabled(SENSOR_TYPE_PROXIMITY_RAW)) {
		batch_sensor(SENSOR_TYPE_PROXIMITY_RAW, 20, 0);
		enable_sensor(SENSOR_TYPE_PROXIMITY_RAW, NULL, 0);

		msleep(200);
		raw_data = sensor_value->prox_raw;
		disable_sensor(SENSOR_TYPE_PROXIMITY_RAW, NULL, 0);
	} else {
		raw_data = sensor_value->prox_raw;
	}

	return raw_data;
}


static ssize_t raw_data_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (!get_sensor_probe_state(SENSOR_TYPE_PROXIMITY_RAW)) {
		shub_errf("sensor is not probed!");
		return 0;
	}

	return sprintf(buf, "%u\n", get_prox_raw_data());
}

static ssize_t prox_avg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct shub_sensor *sensor = get_sensor(SENSOR_TYPE_PROXIMITY_RAW);
	struct proximity_raw_data *data;

	if (!sensor) {
		shub_infof("sensor is null");
		return -EINVAL;
	}

	data = sensor->data;

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d\n", data->avg_data[PROX_RAW_MIN], data->avg_data[PROX_RAW_AVG],
			data->avg_data[PROX_RAW_MAX]);
}

static ssize_t prox_avg_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	char temp_buf[8] = { 0, };
	int ret;
	int64_t enable;
	s32 ms_delay = 20;

	memcpy(&temp_buf[0], &ms_delay, 4);

	ret = kstrtoll(buf, 10, &enable);
	if (ret < 0)
		shub_errf("- failed = %d", ret);

	if (enable) {
		if (!get_sensor_enabled(SENSOR_TYPE_PROXIMITY_RAW)) {
			batch_sensor(SENSOR_TYPE_PROXIMITY_RAW, 20, 0);
			enable_sensor(SENSOR_TYPE_PROXIMITY_RAW, NULL, 0);
		}
	} else {
		if (get_sensor_enabled(SENSOR_TYPE_PROXIMITY_RAW))
			disable_sensor(SENSOR_TYPE_PROXIMITY_RAW, NULL, 0);
	}

	return size;
}

static ssize_t prox_offset_pass_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", 1);
}

static ssize_t prox_position_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%u.%u %u.%u %u.%u\n", position[0], position[1], position[2], position[3], position[4],
		       position[5]);
}

static ssize_t trim_check_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret = 0;
	char *buffer = NULL;
	int buffer_len = 0;
	u8 trim_check;

	if (chipset_func && chipset_func->trim_check_show)
		return chipset_func->trim_check_show(buf);

	ret = shub_send_command_wait(CMD_GETVALUE, SENSOR_TYPE_PROXIMITY, PROX_SUBCMD_TRIM_CHECK, 1000, NULL, 0,
								 &buffer, &buffer_len, true);

	if (ret < 0) {
		shub_errf("shub_send_command_wait fail %d", ret);
		return ret;
	}

	if (buffer_len != sizeof(trim_check)) {
		shub_errf("buffer length error %d", buffer_len);
		kfree(buffer);
		return -EINVAL;
	}

	memcpy(&trim_check, buffer, sizeof(trim_check));
	kfree(buffer);

	shub_infof("%d", trim_check);

	if (trim_check != 0 && trim_check != 1) {
		shub_errf("hub read trim NG");
		return -EINVAL;
	}

	return snprintf(buf, PAGE_SIZE, "%s\n", (trim_check == 0) ? "TRIM" : "UNTRIM");
}

static ssize_t prox_cal_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (!chipset_func || !chipset_func->prox_cal_show)
		return -EINVAL;

	return chipset_func->prox_cal_show(buf);
}

static ssize_t prox_cal_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int type = get_prox_type();

	if (!chipset_func || !chipset_func->prox_cal_store)
		return -EINVAL;

	return chipset_func->prox_cal_store(buf, size, type);
}

static ssize_t prox_trim_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int type = get_prox_type();

	if (!chipset_func || !chipset_func->prox_trim_show)
		return -EINVAL;

	return chipset_func->prox_trim_show(buf, type);
}

static ssize_t modify_settings_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (!chipset_func || !chipset_func->modify_settings_show)
		return -EINVAL;

	return chipset_func->modify_settings_show(buf);
}

static ssize_t modify_settings_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	if (!chipset_func || !chipset_func->modify_settings_store)
		return -EINVAL;

	return chipset_func->modify_settings_store(buf, size);
}

static ssize_t settings_thresh_high_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (!chipset_func || !chipset_func->settings_thresh_high_show)
		return -EINVAL;

	return chipset_func->settings_thresh_high_show(buf);
}

static ssize_t settings_thresh_high_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	if (!chipset_func || !chipset_func->settings_thresh_high_store)
		return -EINVAL;

	return chipset_func->settings_thresh_high_store(buf, size);
}

static ssize_t settings_thresh_low_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (!chipset_func || !chipset_func->settings_thresh_low_show)
		return -EINVAL;

	return chipset_func->settings_thresh_low_show(buf);
}

static ssize_t settings_thresh_low_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	if (!chipset_func || !chipset_func->settings_thresh_low_store)
		return -EINVAL;

	return chipset_func->settings_thresh_low_store(buf, size);
}

static ssize_t debug_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (!chipset_func || !chipset_func->debug_info_show)
		return -EINVAL;

	return chipset_func->debug_info_show(buf);
}

static ssize_t prox_led_test_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (!chipset_func || !chipset_func->prox_led_test_show)
		return -EINVAL;

	return chipset_func->prox_led_test_show(buf);
}

static ssize_t thresh_detect_high_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (!chipset_func || !chipset_func->thresh_detect_high_show)
		return -EINVAL;

	return chipset_func->thresh_detect_high_show(buf);
}

static ssize_t thresh_detect_high_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	if (!chipset_func || !chipset_func->thresh_detect_high_store)
		return -EINVAL;

	return chipset_func->thresh_detect_high_store(buf, size);
}

static ssize_t thresh_detect_low_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (!chipset_func || !chipset_func->thresh_detect_low_show)
		return -EINVAL;

	return chipset_func->thresh_detect_low_show(buf);
}

static ssize_t thresh_detect_low_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	if (!chipset_func || !chipset_func->thresh_detect_low_store)
		return -EINVAL;

	return chipset_func->thresh_detect_low_store(buf, size);
}

static ssize_t prox_trim_check_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	if (!chipset_func || !chipset_func->prox_trim_check_show)
		return -EINVAL;

	return chipset_func->prox_trim_check_show(buf);
}

static DEVICE_ATTR_RO(name);
static DEVICE_ATTR_RO(vendor);
static DEVICE_ATTR_RO(prox_probe);
static DEVICE_ATTR_RO(prox_position);
static DEVICE_ATTR_RO(thresh_high);
static DEVICE_ATTR_RO(thresh_low);
static DEVICE_ATTR_RO(raw_data);
static DEVICE_ATTR(prox_avg, 0664, prox_avg_show, prox_avg_store);
static DEVICE_ATTR_RO(prox_offset_pass);
static DEVICE_ATTR_RO(trim_check);
static DEVICE_ATTR(prox_cal, 0660, prox_cal_show, prox_cal_store);
static DEVICE_ATTR_RO(prox_trim);
static DEVICE_ATTR(modify_settings, 0664, modify_settings_show, modify_settings_store);
static DEVICE_ATTR(settings_thd_high, 0664, settings_thresh_high_show, settings_thresh_high_store);
static DEVICE_ATTR(settings_thd_low, 0664, settings_thresh_low_show, settings_thresh_low_store);
static DEVICE_ATTR_RO(debug_info);
static DEVICE_ATTR_RO(prox_led_test);
static DEVICE_ATTR(thresh_detect_high, 0664, thresh_detect_high_show, thresh_detect_high_store);
static DEVICE_ATTR(thresh_detect_low, 0664, thresh_detect_low_show, thresh_detect_low_store);
static DEVICE_ATTR_RO(prox_trim_check);

__visible_for_testing struct device_attribute *proximity_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_prox_probe,
	&dev_attr_prox_position,
	&dev_attr_thresh_high,
	&dev_attr_thresh_low,
	&dev_attr_raw_data,
	&dev_attr_prox_avg,
	&dev_attr_prox_offset_pass,
	&dev_attr_trim_check,
	&dev_attr_prox_cal,
	&dev_attr_prox_trim,
	&dev_attr_modify_settings,
	&dev_attr_settings_thd_high,
	&dev_attr_settings_thd_low,
	&dev_attr_debug_info,
	&dev_attr_prox_led_test,
	&dev_attr_thresh_detect_high,
	&dev_attr_thresh_detect_low,
	&dev_attr_prox_trim_check,
	NULL,
};

void initialize_proximity_sysfs(void)
{
	int ret;

	ret = sensor_device_create(&proximity_sysfs_device, NULL, "proximity_sensor");
	if (ret < 0) {
		shub_errf("fail to creat proximity_sensor sysfs device");
		return;
	}

	ret = add_sensor_device_attr(proximity_sysfs_device, proximity_attrs);
	if (ret < 0) {
		shub_errf("fail to add proximity_sensor sysfs device attr");
		return;
	}
}

void remove_proximity_sysfs(void)
{
	remove_sensor_device_attr(proximity_sysfs_device, proximity_attrs);
	sensor_device_unregister(proximity_sysfs_device);
	proximity_sysfs_device = NULL;
}

void remove_proximity_empty_sysfs(void)
{
	struct shub_sensor *sensor = get_sensor(SENSOR_TYPE_PROXIMITY);
	struct device_node *np = get_shub_device()->of_node;

	if (strcmp(sensor->spec.name, "GP2AP110S") != 0) {
		device_remove_file(proximity_sysfs_device, &dev_attr_modify_settings);
		device_remove_file(proximity_sysfs_device, &dev_attr_settings_thd_high);
		device_remove_file(proximity_sysfs_device, &dev_attr_settings_thd_low);
	}

	if (strcmp(sensor->spec.name, "TMD3725") != 0) {
		device_remove_file(proximity_sysfs_device, &dev_attr_thresh_detect_high);
		device_remove_file(proximity_sysfs_device, &dev_attr_thresh_detect_low);
		device_remove_file(proximity_sysfs_device, &dev_attr_prox_trim_check);
	}

	if (strcmp(sensor->spec.name, "STK33F11") != 0 && strcmp(sensor->spec.name, "TMD4914") != 0) {
		device_remove_file(proximity_sysfs_device, &dev_attr_debug_info);

		if (strcmp(sensor->spec.name, "TMD4912") != 0)
			device_remove_file(proximity_sysfs_device, &dev_attr_prox_led_test);
	}

	if (sensor->spec.vendor != VENDOR_AMS
	    && sensor->spec.vendor != VENDOR_CAPELLA
	    && sensor->spec.vendor != VENDOR_SITRONIX
	    && strcmp(sensor->spec.name, "TMD4912") != 0) {
		device_remove_file(proximity_sysfs_device, &dev_attr_trim_check);
	}

	if (!of_property_read_u32_array(np, "prox-position", position, ARRAY_SIZE(position))) {
		shub_info("prox-position - %u.%u %u.%u %u.%u",
			   position[0], position[1], position[2], position[3], position[4], position[5]);
	} else {
		device_remove_file(proximity_sysfs_device, &dev_attr_prox_position);
	}

}

void initialize_proximity_factory_chipset_func(void)
{
	int i;

	struct shub_sensor *sensor = get_sensor(SENSOR_TYPE_PROXIMITY);

	for (i = 0; i < ARRAY_SIZE(get_proximity_factory_chipset_funcs_ptr); i++) {
		chipset_func = get_proximity_factory_chipset_funcs_ptr[i](sensor->spec.name);
		if(!chipset_func)
			continue;

		shub_infof("support proximity sysfs");
		break;
	}

	if (!chipset_func)
		remove_proximity_sysfs();
}

void initialize_proximity_factory(bool en, int mode)
{
	if (en)
		initialize_proximity_sysfs();
	else {
		if (mode == INIT_FACTORY_MODE_REMOVE_EMPTY && get_sensor(SENSOR_TYPE_PROXIMITY)) {
			initialize_proximity_factory_chipset_func();
			remove_proximity_empty_sysfs();
		} else {
			remove_proximity_sysfs();
		}
	}
}
