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

#ifndef __SHUB_GYROSCOPE_FACTORY_H_
#define __SHUB_GYROSCOPE_FACTORY_H_

#include <linux/device.h>
#include <linux/types.h>

struct gyroscope_factory_chipset_funcs {
	ssize_t (*selftest)(int type, char *name);
};

struct gyroscope_factory_chipset_funcs *get_gyroscope_icm42605m_chipset_func(char *name);
struct gyroscope_factory_chipset_funcs *get_gyroscope_lsm6dsl_chipset_func(char *name);
struct gyroscope_factory_chipset_funcs *get_gyroscope_lsm6dsotr_chipset_func(char *name);
struct gyroscope_factory_chipset_funcs *get_gyroscope_lsm6dsvtr_chipset_func(char *name);
struct gyroscope_factory_chipset_funcs *get_gyroscope_icm42632m_chipset_func(char *name);

#endif
