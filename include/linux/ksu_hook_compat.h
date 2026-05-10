/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _LINUX_KSU_HOOK_COMPAT_H
#define _LINUX_KSU_HOOK_COMPAT_H

#include <linux/jump_label.h>
#include <linux/types.h>
#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 3, 0) || defined(KSU_HAS_MODERN_STATIC_KEY_INTERFACE)
#define KSU_HOOK_COMPAT_USE_STATIC_KEY
#endif

static inline bool ksu_init_rc_hook_active(void)
{
#if defined(CONFIG_KSU_MANUAL_HOOK)
#if defined(CONFIG_KSU_SUKI) && defined(CONFIG_KSU_MANUAL_HOOK_AUTO_INITRC_HOOK) && defined(KSU_HOOK_COMPAT_USE_STATIC_KEY)
	extern struct static_key_true ksu_init_rc_hook;

	return static_branch_unlikely(&ksu_init_rc_hook);
#else
	extern bool ksu_init_rc_hook;

	return unlikely(ksu_init_rc_hook);
#endif
#elif defined(CONFIG_KSU_SUKI) && defined(CONFIG_KSU_SUSFS)
	extern struct static_key_true ksu_is_init_rc_hook_enabled;

	return static_branch_unlikely(&ksu_is_init_rc_hook_enabled);
#elif defined(CONFIG_KSU)
	extern bool ksu_init_rc_hook;

	return unlikely(ksu_init_rc_hook);
#else
	return false;
#endif
}

static inline bool ksu_input_hook_active(void)
{
#if defined(CONFIG_KSU_MANUAL_HOOK)
#if defined(CONFIG_KSU_SUKI) && defined(CONFIG_KSU_MANUAL_HOOK_AUTO_INPUT_HOOK) && defined(KSU_HOOK_COMPAT_USE_STATIC_KEY)
	extern struct static_key_true ksu_input_hook;

	return static_branch_unlikely(&ksu_input_hook);
#else
	extern bool ksu_input_hook;

	return unlikely(ksu_input_hook);
#endif
#elif defined(CONFIG_KSU_SUKI) && defined(CONFIG_KSU_SUSFS)
	extern struct static_key_true ksu_is_input_hook_enabled;

	return static_branch_unlikely(&ksu_is_input_hook_enabled);
#elif defined(CONFIG_KSU)
	extern bool ksu_input_hook;

	return unlikely(ksu_input_hook);
#else
	return false;
#endif
}

#endif /* _LINUX_KSU_HOOK_COMPAT_H */
