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

#ifndef __SHUB_DEVICE_COMMON_INFO_H__
#define __SHUB_DEVICE_COMMON_INFO_H__

#include <linux/types.h>

struct device_common_info_event {
    u8 device_type;
    u8 table_mode;
    u8 active_screen;
} __attribute__((__packed__));

#endif /* __SHUB_DEVICE_COMMON_INFO_H_ */