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

#ifndef __SHUB_FACTORY_H_
#define __SHUB_FACTORY_H_
#include <linux/types.h>

#define INIT_FACTORY_MODE_NONE		0
#define INIT_FACTORY_MODE_REMOVE_ALL	1
#define INIT_FACTORY_MODE_REMOVE_EMPTY	2

enum {
	FAC_FSTATE_NONE,
	FAC_FSTATE_MAIN,
	FAC_FSTATE_FOLDERBLE_SUB,
	FAC_FSTATE_TABLET_SUB,
	FAC_FSTATE_TABLET_DUAL = 13,
};

int initialize_factory(void);
void remove_factory(void);
void remove_empty_factory(void);

void initialize_accelerometer_factory(bool en, int mode);
void initialize_gyroscope_factory(bool en, int mode);
void initialize_light_factory(bool en, int mode);
void initialize_magnetometer_factory(bool en, int mode);
void initialize_pressure_factory(bool en, int mode);
void initialize_proximity_factory(bool en, int mode);
void initialize_flip_cover_detector_factory(bool en, int mode);
void initialize_accelerometer_sub_factory(bool en, int mode);
void initialize_gyroscope_sub_factory(bool en, int mode);

#endif

