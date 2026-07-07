/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Mali KMD external symbol redirector
 *
 * Provides exported symbols that external kernel consumers (e.g. exynos-migov)
 * resolve against. Each Mali KMD module registers its implementation with
 * this redirector during init; the redirector forwards calls to whichever
 * version is currently active.
 */
#ifndef _MALI_REDIRECT_H_
#define _MALI_REDIRECT_H_

#include <linux/types.h>
#include <linux/ktime.h>

/*
 * Register the active Mali module's kbase_get_create_info implementation.
 * Called by the Mali module init function after the version selection check
 * passes. Passing NULL unregisters (used on module exit).
 */
void mali_redirect_register_get_create_info(void (*fn)(u64 *cnt, ktime_t *us));

#endif /* _MALI_REDIRECT_H_ */
