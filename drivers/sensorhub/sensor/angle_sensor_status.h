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

#ifndef __SHUB_ANGLE_SENSOR_STATUS_H__
#define __SHUB_ANGLE_SENSOR_STATUS_H__

#include <linux/types.h>

struct angle_sensor_status_event {
	s8 status;
} __attribute__((__packed__));

#endif /* __SHUB_ANGLE_SENSOR_STATUS_H_ */