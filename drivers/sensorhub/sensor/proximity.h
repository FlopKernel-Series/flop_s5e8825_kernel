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
#include "../sensormanager/shub_sensor.h"
#include "../sensormanager/shub_sensor_manager.h"
#include "../utility/shub_utility.h"

#include <linux/device.h>

#define PROX_RAW_ADDITIONAL_DEBUG_DATA_VERSION 2000

#define PROX_THRESH_HIGH		0
#define PROX_THRESH_LOW			1
#define PROX_THRESH_SIZE		2

#define DEFUALT_HIGH_THRESHOLD		50
#define DEFUALT_LOW_THRESHOLD		35
#define DEFUALT_DETECT_HIGH_THRESHOLD	200
#define DEFUALT_DETECT_LOW_THRESHOLD	190

#define PROX_ADC_BITS_NUM		14

#define PDATA_MIN			0
#define PDATA_MAX			0x0FFF

#define PROX_CALIBRATION_FILE_PATH		"/efs/FactoryApp/prox_cal_data"
#define PROX_SETTING_MODE_FILE_PATH		"/efs/FactoryApp/prox_settings"

#define SUB_PROX_CALIBRATION_FILE_PATH		"/efs/FactoryApp/sub_prox_cal_data"
#define SUB_PROX_SETTING_MODE_FILE_PATH		"/efs/FactoryApp/sub_prox_settings"

#define PROX_SUBCMD_CALIBRATION_START		128
#define PROX_SUBCMD_TRIM_CHECK				131
#define PROX_SUBCMD_DEBUG_DATA				132
enum {
	PROX_RAW_MIN = 0,
	PROX_RAW_AVG,
	PROX_RAW_MAX,
	PROX_RAW_DATA_SIZE,
};

struct prox_event {
	u8 prox;
	u16 prox_raw;
} __attribute__((__packed__));

struct prox_raw_event {
	u16 prox_raw;
	u16 prox;
	u16 call_min;
	u16 base_line;
	u16 ps_off;
	u16 ir_excpetion_state;
} __attribute__((__packed__));

struct prox_cal_event {
	u16 prox_cal[2];
} __attribute__((__packed__));

struct proximity_raw_data {
	unsigned int avg_data[PROX_RAW_DATA_SIZE];
};

struct prox_led_test {
	u8 ret;
	short adc[4];
};

struct proximity_data {
	char path_calibration[128];
	char path_setting_mode[128];

	void *threshold_data;

	u16 prox_threshold[PROX_THRESH_SIZE];
	bool need_compensation;
	void *cal_data;
	int cal_data_len;

	u8 setting_mode;
};

struct proximity_chipset_funcs {
	u8 (*get_proximity_threshold_mode)(void);
	void (*set_proximity_threshold_mode)(u8 mode);
	void (*set_proximity_threshold)(void);
	void (*sync_proximity_state)(int type);
	void (*pre_enable_proximity)(struct proximity_data *data);
	void (*pre_report_event_proximity)(void);
	int (*open_calibration_file)(int type);
};

struct proximity_gp2ap110s_data {
	u16 prox_setting_thresh[2];
	u16 prox_mode_thresh[PROX_THRESH_SIZE];
};

struct proximity_stk3x6x_data {
	u8 prox_cal_mode;
};

struct proximity_stk3328_data {
	u16 prox_cal_add_value;
	u16 prox_cal_thresh[PROX_THRESH_SIZE];
	u16 prox_thresh_default[PROX_THRESH_SIZE];
};

struct proximity_tmd3725_data {
	u16 prox_thresh_detect[PROX_THRESH_SIZE];
};

int open_default_proximity_calibration(int type);

void set_proximity_threshold(void);
int save_proximity_calibration(int type);
int set_proximity_calibration(int type);

int open_default_proximity_setting_mode(int type);
int save_proximity_setting_mode(int type);
int set_proximity_setting_mode(int type);

struct sensor_chipset_init_funcs *get_proximity_stk3x6x_function_pointer(char *name);
struct sensor_chipset_init_funcs *get_proximity_gp2ap110s_function_pointer(char *name);
struct sensor_chipset_init_funcs *get_proximity_stk3328_function_pointer(char *name);
struct sensor_chipset_init_funcs *get_proximity_stk3391x_function_pointer(char *name);
struct sensor_chipset_init_funcs *get_proximity_stk33512_function_pointer(char *name);
struct sensor_chipset_init_funcs *get_proximity_stk3afx_function_pointer(char *name);
struct sensor_chipset_init_funcs *get_proximity_tmd3725_function_pointer(char *name);
struct sensor_chipset_init_funcs *get_proximity_tmd4914_function_pointer(char *name);
struct sensor_chipset_init_funcs *get_proximity_tmd4913_function_pointer(char *name);

u16 get_prox_raw_data(void);
