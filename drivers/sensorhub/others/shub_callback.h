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

#ifndef __SHUB_CALLBACK_H__
#define __SHUB_CALLBACK_H__

#include <linux/list.h>

struct shub_callback {
	void (*init)(void);
	void (*remove)(void);
	void (*sync)(void);
	struct list_head list;
};

void initialize_shub_callback(void);
void shub_register_callback(struct shub_callback *cb);
void shub_unregister_callback(struct shub_callback *cb);
void shub_init_callback(void);
void shub_remove_callback(void);
void shub_sync_callback(void);

#endif /* __SHUB_CALLBACK_H__ */