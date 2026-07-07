// SPDX-License-Identifier: GPL-2.0
/*
 * Mali KMD external symbol redirector
 *
 * Exports symbols that external kernel modules resolve against,
 * forwarding calls to whichever Mali KMD module is active at runtime.
 * Modules register their implementation pointers on init.
 */
#include <linux/module.h>
#include <linux/ktime.h>
#include "mali_redirect.h"

static void (*kbase_get_create_info_fn)(u64 *cnt, ktime_t *us);

void mali_redirect_register_get_create_info(void (*fn)(u64 *cnt, ktime_t *us))
{
	kbase_get_create_info_fn = fn;
}
EXPORT_SYMBOL(mali_redirect_register_get_create_info);

void kbase_get_create_info(u64 *cnt, ktime_t *us)
{
	if (kbase_get_create_info_fn)
		kbase_get_create_info_fn(cnt, us);
}
EXPORT_SYMBOL(kbase_get_create_info);
