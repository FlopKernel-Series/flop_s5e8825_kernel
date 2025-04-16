
/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 * Gwanghui Lee <gwanghui.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/version.h>

#ifndef __PANEL_WRAPPER_H__
#define __PANEL_WRAPPER_H__

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 4, 0))
#define class_create_wrapper(owner, name) class_create(owner, name)
#else
#define class_create_wrapper(owner, name) class_create(name)
#endif

#endif /* __PANEL_WRAPPER_H__ */
