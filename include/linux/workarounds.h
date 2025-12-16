/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _WORKAROUNDS_H
#define _WORKAROUNDS_H

#include <linux/jump_label.h>

#if defined(CONFIG_DEFAULT_SUPPORT_AOSP)
static inline bool is_aosp_mode(void)
{
	return true;
}
#else
bool is_aosp_mode(void);

// Optimized hot path version using static branch
extern struct static_key_false aosp_mode_key;
static inline bool is_aosp_mode_fast(void)
{
	return static_branch_unlikely(&aosp_mode_key);
}
#endif

bool is_superfloppy_mode(void);
signed char get_superfloppy_mode(void);

// Optimized hot path version using static branch
extern struct static_key_false superfloppy_mode_key;
static inline bool is_superfloppy_mode_fast(void)
{
	return static_branch_unlikely(&superfloppy_mode_key);
}

bool is_superfloppy_overclock_mode(void);

// Optimized hot path version using static branch
extern struct static_key_false superfloppy_overclock_mode_key;
static inline bool is_superfloppy_overclock_mode_fast(void)
{
	return static_branch_unlikely(&superfloppy_overclock_mode_key);
}

bool is_force_perm_mode(void);

bool is_ems_efficient(void);

// Optimized hot path version using static branch
extern struct static_key_false ems_efficient_mode_key;
static inline bool is_ems_efficient_fast(void)
{
	return static_branch_unlikely(&ems_efficient_mode_key);
}

#endif /* _WORKAROUNDS_H */
