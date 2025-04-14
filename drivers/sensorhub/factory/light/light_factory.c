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

#include "../../sensor/light.h"
#include "../../comm/shub_comm.h"
#include "../../factory/shub_factory.h"
#include "../../sensorhub/shub_device.h"
#include "../../sensormanager/shub_sensor.h"
#include "../../sensormanager/shub_sensor_manager.h"
#include "../../sensormanager/shub_vendor_type.h"
#include "../../utility/shub_dev_core.h"
#include "../../utility/shub_utility.h"
#include "../../utility/shub_file_manager.h"
#include "../../sensor/hub_debugger.h"
#include "../../others/shub_panel.h"

#include <linux/device.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/delay.h>

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
__visible_for_testing struct device *light_sysfs_device;
__visible_for_testing s32 light_position[12];

int get_light_type(void) {
	u8 fstate = get_shub_data()->fac_fstate;

	switch (fstate) {
	case FAC_FSTATE_MAIN:
		return SENSOR_TYPE_LIGHT;
	case FAC_FSTATE_FOLDERBLE_SUB:
	case FAC_FSTATE_TABLET_SUB:
		return SENSOR_TYPE_SUB_LIGHT;
	case FAC_FSTATE_TABLET_DUAL:
		return SENSOR_TYPE_LIGHT;
	default:
		return SENSOR_TYPE_LIGHT;
	}
}

static ssize_t name_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int type = get_light_type();
	struct shub_sensor *sensor = get_sensor(type);

	if (!sensor) {
		shub_infof("sensor is null");
		return -EINVAL;
	}

	return sprintf(buf, "%s\n", sensor->spec.name);
}

static ssize_t vendor_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int type = get_light_type();
	struct shub_sensor *sensor = get_sensor(type);
	char vendor[VENDOR_MAX] = "";

	if (!sensor) {
		shub_infof("sensor is null");
		return -EINVAL;
	}

	get_sensor_vendor_name(sensor->spec.vendor, vendor);

	return sprintf(buf, "%s\n", vendor);
}

static ssize_t lux_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct shub_sensor *sensor = get_sensor(SENSOR_TYPE_LIGHT);
	struct light_event *sensor_value;

	if (!sensor) {
		shub_infof("sensor is null");
		return -EINVAL;
	}

	sensor_value = (struct light_event *)(get_sensor_event(SENSOR_TYPE_LIGHT)->value);

	return sprintf(buf, "%d,%d,%d,%d,%d,%d\n", sensor_value->r, sensor_value->g, sensor_value->b, sensor_value->w,
		       sensor_value->a_time, sensor_value->a_gain);
}

static ssize_t raw_data_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct light_event *sensor_value;

	if (!get_sensor_probe_state(SENSOR_TYPE_LIGHT)) {
		shub_errf("sensor is not probed!");
		return 0;
	}

	sensor_value = (struct light_event *)(get_sensor_event(SENSOR_TYPE_LIGHT)->value);

	return sprintf(buf, "%d,%d,%d,%d,%d,%d\n", sensor_value->r, sensor_value->g, sensor_value->b, sensor_value->w,
		       sensor_value->a_time, sensor_value->a_gain);
}

static ssize_t light_circle_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct shub_sensor *sensor = get_sensor(SENSOR_TYPE_LIGHT);
	struct light_data *data;

	if (!sensor) {
		shub_infof("sensor is null");
		return -EINVAL;
	}

	data = sensor->data;

	if (data->light_dual) {
		return sprintf(buf, "%d.%02d %d.%02d %d.%d %d.%02d %d.%02d %d.%d\n",
			   light_position[0], light_position[1], light_position[2], light_position[3],
			   light_position[4], light_position[5], light_position[6], light_position[7],
			   light_position[8], light_position[9], light_position[10], light_position[11]);
	} else {
		return sprintf(buf, "%d.%02d %d.%02d %d.%d\n",
			   light_position[0], light_position[1], light_position[2],
			   light_position[3], light_position[4], light_position[5]);
	}
}

static ssize_t hall_ic_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int ret = 0;
	u8 hall_ic = 0;

	if (!get_sensor_probe_state(SENSOR_TYPE_LIGHT_AUTOBRIGHTNESS))
		return -ENOENT;
	if (!buf)
		return -EINVAL;

	if (kstrtou8(buf, 10, &hall_ic) < 0)
		return -EINVAL;

	shub_infof("%d", hall_ic);

	ret = shub_send_command(CMD_SETVALUE, SENSOR_TYPE_LIGHT_AUTOBRIGHTNESS, HALL_IC_STATUS, (char *)&hall_ic,
				sizeof(hall_ic));
	if (ret < 0) {
		shub_errf("CMD fail %d\n", ret);
		return size;
	}

	return size;
}

static ssize_t coef_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret = 0;
	char *coef_buf = NULL;
	int coef_buf_length = 0;
	int temp_coef[7] = {0, };
	struct shub_sensor *sensor = get_sensor(SENSOR_TYPE_LIGHT);
	struct light_data *data;

	if (!sensor) {
		shub_infof("sensor is null");
		return -EINVAL;
	}

	data = sensor->data;

	if (data->light_coef) {
		ret = shub_send_command_wait(CMD_GETVALUE, SENSOR_TYPE_LIGHT, LIGHT_COEF, 1000, NULL, 0, &coef_buf,
					&coef_buf_length, true);

		if (ret < 0) {
			shub_errf("shub_send_command_wait Fail %d", ret);
			return ret;
		}

		if (coef_buf_length != 28) {
			shub_errf("buffer length error %d", coef_buf_length);
			kfree(coef_buf);
			return -EINVAL;
		}

		memcpy(temp_coef, coef_buf, sizeof(temp_coef));

		shub_infof("%d %d %d %d %d %d %d\n", temp_coef[0], temp_coef[1], temp_coef[2], temp_coef[3],
			   temp_coef[4], temp_coef[5], temp_coef[6]);

		ret = snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d\n", temp_coef[0], temp_coef[1], temp_coef[2],
			       temp_coef[3], temp_coef[4], temp_coef[5], temp_coef[6]);

		kfree(coef_buf);
	} else {
		ret = snprintf(buf, PAGE_SIZE, "0,0,0,0,0,0,0\n");
	}

	return ret;
}

static ssize_t sensorhub_ddi_spi_check_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret = 0;
	int retries = 0;
	char *buffer = NULL;
	int buffer_len = 0;
	short copr = 0;

retry:
	ret = shub_send_command_wait(CMD_GETVALUE, SENSOR_TYPE_LIGHT, DDI_COPR, 1000, NULL, 0, &buffer, &buffer_len,
				     true);

	if (ret < 0) {
		shub_errf("shub_send_command_wait fail %d", ret);

		if (retries++ < 2) {
			shub_errf("fail, retry");
			mdelay(5);
			goto retry;
		}
		return ret;
	}

	if (buffer_len != sizeof(copr)) {
		shub_errf("buffer length error %d", buffer_len);
		kfree(buffer);
		return -EINVAL;
	}
	memcpy(&copr, buffer, sizeof(copr));

	shub_infof("%d", copr);

	return snprintf(buf, PAGE_SIZE, "%d\n", copr);
}

static ssize_t test_copr_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret = 0;
	int retries = 0;
	short copr[4];
	char *buffer = NULL;
	int buffer_len = 0;

	memset(copr, 0, sizeof(copr));
retry:
	ret = shub_send_command_wait(CMD_GETVALUE, SENSOR_TYPE_LIGHT, TEST_COPR, 1000, NULL, 0, &buffer, &buffer_len,
				     true);

	if (ret < 0) {
		shub_errf("shub_send_command_wait fail %d", ret);

		if (retries++ < 2) {
			shub_errf("fail, retry");
			mdelay(5);
			goto retry;
		}
		return ret;
	}

	if (buffer_len != sizeof(copr)) {
		shub_errf("buffer length error %d", buffer_len);
		kfree(buffer);
		return -EINVAL;
	}
	memcpy(&copr, buffer, sizeof(copr));

	shub_infof("%d, %d, %d, %d", copr[0], copr[1], copr[2], copr[3]);

	return snprintf(buf, PAGE_SIZE, "%d, %d, %d, %d\n", copr[0], copr[1], copr[2], copr[3]);
}

static ssize_t copr_roix_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret = 0;
	int retries = 0;
	char *buffer = NULL;
	int buffer_len = 0;
	short copr[15];
	long copr_ret[3] = {0,};

	memset(copr, 0, sizeof(copr));
retry:
	ret = shub_send_command_wait(CMD_GETVALUE, SENSOR_TYPE_LIGHT, COPR_ROIX, 1000, NULL, 0, &buffer, &buffer_len,
				     true);

	if (ret < 0) {
		shub_errf("shub_send_command_wait fail %d", ret);
		if (retries++ < 2) {
			shub_errf("fail, retry");
			mdelay(5);
			goto retry;
		}
		return ret;
	}

	if (buffer_len != sizeof(copr)) {
		shub_errf("buffer length error %d", buffer_len);
		kfree(buffer);
		return -EINVAL;
	}
	memcpy(&copr, buffer, sizeof(copr));
	copr_ret[0] = (long)copr[9]  + (long)copr[12] * 1000;
	copr_ret[1] = (long)copr[10] + (long)copr[13] * 1000;
	copr_ret[2] = (long)copr[11] + (long)copr[14] * 1000;

	shub_infof("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
		copr[0], copr[1], copr[2], copr[3], copr[4], copr[5],
		copr[6], copr[7], copr[8], copr[9], copr[10], copr[11],
		copr[12], copr[13], copr[14]);

	return snprintf(buf, PAGE_SIZE, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%ld,%ld,%ld\n",
		copr[0], copr[1], copr[2], copr[3], copr[4], copr[5], copr[6], copr[7],
		copr[8], copr_ret[0], copr_ret[1], copr_ret[2]);
}

static ssize_t light_cal_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int type = get_light_type();
	struct shub_sensor *sensor = get_sensor(type);
	struct light_data *data;
	u8 fstate = get_shub_data()->fac_fstate;

	if (!sensor) {
		shub_infof("sensor is null");
		return -EINVAL;
	}

	data = sensor->data;

	if (fstate == FAC_FSTATE_TABLET_DUAL) {
		struct shub_sensor *sensor = get_sensor(SENSOR_TYPE_SUB_LIGHT);
		struct light_data *sub_data;
		struct light_cal_data sub_cal_data;

		memset(&sub_cal_data, 0, sizeof(sub_cal_data));
		if (sensor) {
			sub_data = sensor->data;
			memcpy(&(sub_cal_data), (char *)&sub_data->cal_data, sizeof(sub_cal_data));
		}

		return snprintf(buf, PAGE_SIZE, "%u, %u, %u, %u, %u, %u\n",
				data->cal_data.result, data->cal_data.max, data->cal_data.lux,
				sub_cal_data.result, sub_cal_data.max, sub_cal_data.lux);
	} else {
		return snprintf(buf, PAGE_SIZE, "%u, %u, %u\n",
				data->cal_data.result, data->cal_data.max, data->cal_data.lux);
	}
}

static int init_light_cal_data(int type)
{
	int ret = 0;
	char send_buf = 1;
	struct shub_sensor *sensor = get_sensor(type);
	struct light_data *data;

	if (!get_sensor_probe_state(type))
		return -ENOENT;
	if (!sensor) {
		shub_infof("sensor(%d) is null", type);
		return -EINVAL;
	}

	data = sensor->data;

	ret = shub_send_command(CMD_SETVALUE, type, SENSOR_FACTORY, &send_buf, sizeof(send_buf));
	if (ret < 0) {
		shub_errf("%d : CMD failed, %d", type, ret);
		return -EINVAL;
	}

	memset(&data->cal_data, 0, sizeof(data->cal_data));

	return ret;
}

static int get_light_cal_data(int type)
{
	int ret = 0;
	struct shub_sensor *sensor = get_sensor(type);
	struct light_data *data;
	struct light_cal_data_legacy cal_data_legacy;
	int cal_data_size = 0;
	char *buffer = NULL;
	int buffer_length = 0;

	if (!get_sensor_probe_state(type))
		return -ENOENT;
	if (!sensor) {
		shub_infof("sensor(%d) is null", type);
		return -EINVAL;
	}

	data = sensor->data;

	if (sensor->spec.version >= LIGHT_CAL_CH0_SIZE_4BYTE_VERSION)
		cal_data_size = sizeof(data->cal_data);
	else
		cal_data_size = sizeof(cal_data_legacy);

	ret = shub_send_command_wait(CMD_GETVALUE, type, CAL_DATA, 1000, NULL, 0, &buffer,
					&buffer_length, false);
	if (ret < 0) {
		shub_errf("CMD fail %d", ret);
		return ret;
	}

	if (buffer_length != cal_data_size) {
		shub_errf("%d : buffer_length(%d) != cal_data_size(%d)", type, buffer_length, cal_data_size);
		return -EINVAL;
	}

	if (sensor->spec.version >= LIGHT_CAL_CH0_SIZE_4BYTE_VERSION) {
		memcpy(&(data->cal_data), buffer, sizeof(data->cal_data));
		ret = data->cal_data.result;
	} else {
		memset(&cal_data_legacy, 0, sizeof(cal_data_legacy));
		memcpy(&(cal_data_legacy), buffer, sizeof(cal_data_legacy));
		data->cal_data.result = cal_data_legacy.result;
		data->cal_data.max = (u32)cal_data_legacy.max;
		data->cal_data.lux = cal_data_legacy.lux;
		ret = data->cal_data.result;
	}

	return ret;
}

static ssize_t light_cal_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int ret;
	bool init, update, file_write = false;
	int type = get_light_type();
	u8 fstate = get_shub_data()->fac_fstate;

	struct shub_sensor *sensor = get_sensor(type);
	struct light_data *data;

	if (!sensor) {
		shub_infof("sensor(%d) is null", type);
		return -EINVAL;
	}

	data = sensor->data;

	if (!get_sensor_probe_state(type))
		return -ENOENT;
	if (!buf)
		return -EINVAL;

	init = sysfs_streq(buf, "0");
	update = sysfs_streq(buf, "1");

	if (init) {
		if (fstate == FAC_FSTATE_TABLET_DUAL) {
			int ret2 = 0;

			ret = init_light_cal_data(SENSOR_TYPE_LIGHT);
			ret2 = init_light_cal_data(SENSOR_TYPE_SUB_LIGHT);

			if (ret < 0 && ret2 < 0)
				return -EINVAL;
		} else {
			ret = init_light_cal_data(type);
			if (ret < 0)
				return ret;
		}

		file_write = true;
	} else if (update) {
		if (fstate == FAC_FSTATE_TABLET_DUAL) {
			int ret2 = 0;

			ret = get_light_cal_data(SENSOR_TYPE_LIGHT);
			ret2 = get_light_cal_data(SENSOR_TYPE_SUB_LIGHT);

			if (ret < 0 && ret2 < 0)
				return -EINVAL;

			file_write = ret | ret2;
		} else {
			ret = get_light_cal_data(type);
			if (ret < 0)
				return ret;

			file_write = ret;
		}
	} else {
		shub_errf("buf data is wrong %s", buf);
	}

	if (file_write) {
		ret = shub_file_write_no_wait(data->path_calibration, (u8 *)&(data->cal_data),
									sizeof(data->cal_data), 0);
		if (fstate == FAC_FSTATE_TABLET_DUAL)
			shub_infof("Skip saving sub_cal_data");

		if (ret != sizeof(data->cal_data))
			shub_errf("Can't write light cal to file");
	}

	return size;
}

static ssize_t factory_fstate_store(struct device *dev,
					  struct device_attribute *attr, const char *buf, size_t size)
{
	int ret = 0;
	u8 send_buf;

	shub_infof("%s", buf);

	ret = kstrtou8(buf, 10, &send_buf);
	if (ret < 0)
		return ret;

	ret = shub_send_command(CMD_SETVALUE, SENSOR_TYPE_LIGHT, LIGHT_SUBCMD_TWO_LIGHT_FACTORY_TEST,
							(char *)&send_buf, sizeof(send_buf));
	if (ret < 0) {
		shub_errf("CMD fail %d", ret);
		return ret;
	}

	get_shub_data()->fac_fstate = send_buf;

	return size;
}

static ssize_t trim_check_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret = 0;
	char *buffer = NULL;
	int buffer_len = 0;
	u8 trim_check;

	ret = shub_send_command_wait(CMD_GETVALUE, SENSOR_TYPE_LIGHT, LIGHT_SUBCMD_TRIM_CHECK, 1000, NULL, 0, &buffer,
								 &buffer_len, true);

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

struct light_debug_info {
	uint32_t stdev;
	uint32_t moving_stdev;
	uint32_t mode;
	uint32_t brightness;
	uint32_t min_div_max;
	uint32_t lux;
} __attribute__((__packed__));

static ssize_t debug_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret = 0;
	char *buffer = NULL;
	int buffer_len = 0;
	struct light_debug_info debug_info;

	ret = shub_send_command_wait(CMD_GETVALUE, SENSOR_TYPE_LIGHT, DEBUG_INFO, 1000, NULL, 0, &buffer, &buffer_len,
				     false);

	if (ret < 0) {
		shub_errf("shub_send_command_wait fail %d", ret);
		return ret;
	}

	if (buffer == NULL) {
		shub_errf("buffer is null");
		return -EINVAL;
	}

	if (buffer_len != sizeof(debug_info)) {
		shub_errf("buffer length error %d", buffer_len);
		kfree(buffer);
		return -EINVAL;
	}

	memcpy(&debug_info, buffer, sizeof(debug_info));

	return snprintf(buf, PAGE_SIZE, "%u, %u, %u, %u, %u, %u\n", debug_info.stdev, debug_info.moving_stdev,
			debug_info.mode, debug_info.brightness, debug_info.min_div_max, debug_info.lux);
}

static ssize_t fifo_data_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%s\n", get_hub_debugger_fifo_data());
}

static ssize_t screen_mode_store(struct device *dev,
					  struct device_attribute *attr, const char *buf, size_t size)
{
	int ret = 0;
	u8 send_buf;

	shub_infof("%s", buf);

	ret = kstrtou8(buf, 10, &send_buf);

	if (ret < 0)
		return ret;

	ret = send_screen_mode_information(0, send_buf);

	if (ret < 0) {
		shub_errf("CMD fail %d", ret);
		return ret;
	}

	return size;
}

static DEVICE_ATTR_RO(name);
static DEVICE_ATTR_RO(vendor);
static DEVICE_ATTR_RO(lux);
static DEVICE_ATTR_RO(raw_data);
static DEVICE_ATTR(hall_ic, 0220, NULL, hall_ic_store);
static DEVICE_ATTR(light_cal, 0664, light_cal_show, light_cal_store);
static DEVICE_ATTR_RO(debug_info);
static DEVICE_ATTR_RO(fifo_data);
static DEVICE_ATTR_RO(trim_check);
static DEVICE_ATTR_RO(light_circle);
static DEVICE_ATTR_RO(coef);
static DEVICE_ATTR_RO(copr_roix);
static DEVICE_ATTR_RO(test_copr);
static DEVICE_ATTR_RO(sensorhub_ddi_spi_check);
static DEVICE_ATTR(fac_fstate, 0220, NULL, factory_fstate_store);
static DEVICE_ATTR(screen_mode, 0220, NULL, screen_mode_store);

__visible_for_testing struct device_attribute *light_attrs[] = {
	&dev_attr_name,
	&dev_attr_vendor,
	&dev_attr_lux,
	&dev_attr_raw_data,
	&dev_attr_hall_ic,
	&dev_attr_light_cal,
	&dev_attr_debug_info,
	&dev_attr_fifo_data,
	&dev_attr_trim_check,
	&dev_attr_light_circle,
	&dev_attr_coef,
	&dev_attr_copr_roix,
	&dev_attr_test_copr,
	&dev_attr_sensorhub_ddi_spi_check,
	&dev_attr_fac_fstate,
	&dev_attr_screen_mode,
	NULL,
};

void initialize_light_sysfs(void)
{
	int ret;

	ret = sensor_device_create(&light_sysfs_device, NULL, "light_sensor");
	if (ret < 0) {
		shub_errf("fail to creat light_sensor sysfs device");
		return;
	}

	ret = add_sensor_device_attr(light_sysfs_device, light_attrs);
	if (ret < 0) {
		shub_errf("fail to add light_sensor sysfs device attr");
		return;
	}

}

void remove_light_sysfs(void)
{
	remove_sensor_device_attr(light_sysfs_device, light_attrs);
	sensor_device_unregister(light_sysfs_device);
	light_sysfs_device = NULL;
}

void remove_light_empty_sysfs(void)
{
	struct shub_sensor *sensor = get_sensor(SENSOR_TYPE_LIGHT);
	struct light_data *data = sensor->data;
	struct device_node *np = get_shub_device()->of_node;
	int light_position_size = 0;

	if (of_property_read_bool(np, "light-dual")) {
		data->light_dual = true;
		shub_info("support light_dual");
	} else {
		device_remove_file(light_sysfs_device, &dev_attr_fac_fstate);
	}

	if (data->light_dual)
		light_position_size = ARRAY_SIZE(light_position);
	else
		light_position_size = ARRAY_SIZE(light_position)/2;

	if (!of_property_read_u32_array(np, "light-position", (s32 *)light_position, light_position_size)) {
		if (data->light_dual) {
			shub_info("light-position - %d.%d %d.%d %d.%d %d.%d %d.%d %d.%d",
				light_position[0], light_position[1], light_position[2], light_position[3],
				light_position[4], light_position[5], light_position[6], light_position[7],
				light_position[8], light_position[9], light_position[10], light_position[11]);
		} else {
			shub_info("light-position - %d.%d %d.%d %d.%d",
				light_position[0], light_position[1], light_position[2],
				light_position[3], light_position[4], light_position[5]);
		}
	} else {
		device_remove_file(light_sysfs_device, &dev_attr_light_circle);
	}

	if (!data->light_coef)
		device_remove_file(light_sysfs_device, &dev_attr_coef);

	if (!data->ddi_support) {
		device_remove_file(light_sysfs_device, &dev_attr_sensorhub_ddi_spi_check);
		device_remove_file(light_sysfs_device, &dev_attr_test_copr);
		device_remove_file(light_sysfs_device, &dev_attr_copr_roix);
	}

	if (!is_support_system_feature(SF_LIGHT_FIFO_DATA_SUPPORT))
		device_remove_file(light_sysfs_device, &dev_attr_fifo_data);

	if (sensor->spec.vendor != VENDOR_AMS
		&& sensor->spec.vendor != VENDOR_CAPELLA
		&& sensor->spec.vendor != VENDOR_SITRONIX) {
		device_remove_file(light_sysfs_device, &dev_attr_trim_check);
	}

	shub_infof("support light sysfs");
}

void initialize_light_factory(bool en, int type)
{
	if (en)
		initialize_light_sysfs();
	else {
		if (type == INIT_FACTORY_MODE_REMOVE_EMPTY && get_sensor(SENSOR_TYPE_LIGHT)) {
			remove_light_empty_sysfs();
		} else {
			remove_light_sysfs();
		}
	}
}
