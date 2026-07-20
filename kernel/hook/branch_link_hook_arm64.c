// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2026 \xx
 *
 * This file is a downstream extension and NOT affiliated, endorsed by,
 * or maintained by the official KernelSU developers.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef CONFIG_ARM64
#error "only meant for ARM64!"
#endif

#if !defined(CONFIG_KPROBES)
#error "kprobes is required for branch link hack!"
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 17, 0)
#error "probably impossible for sub 4.17, unless you have backported do_faccessat"
#endif

/**
 *  NOTE: theres no way to hijack sys_reboot and sys_newfstat cleanly.
 *
 *  however, this feature requires kprobes anyway. and this is still highly experimental. (260524)
 *  works the same as lsm_hooks_static.c, where we patch caller's site
 *
 *  tested to work on 4.19 ~ 6.12 aarch64 GKI
 *
 *  Changelog:
 *	- init, 260524
 *	- partial/probably-broken 4.19/5.4 compat, 260525
 *	- fixups for 4.19 ~ 6.6 CFI, resolve symbols via kprobe. (260630)
 *	- optimize ksu_vfs_statx (260630)
 *
 */

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0) || defined(KSU_HAS_FACCESSAT2)
static long (*do_faccessat_fn)(int dfd, const char __user *filename, int mode, int flags) __read_mostly = NULL;
static __nocfi long ksu_do_faccessat(int dfd, const char __user *filename, int mode, int flags)
{
	ksu_handle_faccessat(&dfd, &filename, &mode, NULL);
	return do_faccessat_fn(dfd, filename, mode, flags);
}
#else
extern long do_faccessat(int dfd, const char __user *filename, int mode);
static long ksu_do_faccessat(int dfd, const char __user *filename, int mode)
{
	ksu_handle_faccessat(&dfd, &filename, &mode, NULL);
	return do_faccessat(dfd, filename, mode);
}
#endif // 5.7+ || faccessat2

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0) // on some kernels vfs_fstatat calls gets inlined, so we have to handle it
static int (*vfs_statx_fn)(int dfd, struct filename *filename, int flags, struct kstat *stat, u32 request_mask) __read_mostly = NULL;
static __nocfi int ksu_vfs_statx(int dfd, struct filename *filename, int flags, struct kstat *stat, u32 request_mask)
{
	if (IS_ERR(filename))
		goto orig_fn;

	char *filename_ptr = (char *)filename->name;
	if (!is_su_allowed((const void **)&filename_ptr))
		goto orig_fn;

	// see sucompat.c
	const char su[16] = SU_PATH;
	uint64_t *su_p = (uint64_t *)su;
	uint64_t *fn_p = (uint64_t *)filename_ptr;

	if (likely((fn_p[1] & 0x00FFFFFFFFFFFFFFULL) != (su_p[1] & 0x00FFFFFFFFFFFFFFULL)))
		goto orig_fn;

	if (unlikely(fn_p[0] != su_p[0]))
		goto orig_fn;
	
	pr_info("vfs_statx su->sh\n");
	memcpy(filename_ptr, SH_PATH, sizeof(SH_PATH));

orig_fn:
	return vfs_statx_fn(dfd, filename, flags, stat, request_mask);
}
#else
static int (*vfs_statx_fn)(int dfd, const char __user *filename, int flags, struct kstat *stat, u32 request_mask) __read_mostly = NULL;
static __nocfi int ksu_vfs_statx(int dfd, const char __user *filename, int flags, struct kstat *stat, u32 request_mask)
{
	ksu_handle_stat(&dfd, &filename, &flags);
	return vfs_statx_fn(dfd, filename, flags, stat, request_mask);
}
#endif // >= 5.18

extern int vfs_fstatat(int dfd, const char __user *filename, struct kstat *stat, int flags);
static int ksu_vfs_fstatat(int dfd, const char __user *filename, struct kstat *stat, int flags)
{
	ksu_handle_stat(&dfd, &filename, &flags);
	return vfs_fstatat(dfd, filename, stat, flags);
}
#else // < 5.10
extern int vfs_statx(int dfd, const char __user *filename, int flags, struct kstat *stat, u32 request_mask);
static int ksu_vfs_statx(int dfd, const char __user *filename, int flags, struct kstat *stat, u32 request_mask)
{
	ksu_handle_stat(&dfd, &filename, &flags);
	return vfs_statx(dfd, filename, flags, stat, request_mask);
}
#endif // 5.10+

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
static int (*do_execveat_common_fn)(int fd, struct filename *filename, struct user_arg_ptr argv, struct user_arg_ptr envp, int flags) __read_mostly = NULL;
static __nocfi int ksu_do_execveat_common(int fd, struct filename *filename, struct user_arg_ptr argv, struct user_arg_ptr envp, int flags)
{
	ksu_handle_execveat((int *)AT_FDCWD, &filename, &argv, &envp, 0);
	return do_execveat_common_fn(fd, filename, argv, envp, flags);
}
#else
static int (*__do_execve_file_fn)(int fd, struct filename *filename, struct user_arg_ptr argv, struct user_arg_ptr envp, int flags, struct file *file) __read_mostly = NULL;
static __nocfi int ksu_do_execve_file(int fd, struct filename *filename, struct user_arg_ptr argv, struct user_arg_ptr envp, int flags, struct file *file)
{
	ksu_handle_execveat((int *)AT_FDCWD, &filename, &argv, &envp, 0);
	return __do_execve_file_fn(fd, filename, argv, envp, flags, file);
}

extern int do_execve(struct filename *filename, const char __user *const __user *__argv, const char __user *const __user *__envp);
static int ksu_do_execve(struct filename *filename, const char __user *const __user *__argv, const char __user *const __user *__envp)
{
	struct user_arg_ptr argv = { .ptr.native = __argv };
	struct user_arg_ptr envp = { .ptr.native = __envp };

	ksu_handle_execveat((int *)AT_FDCWD, &filename, &argv, &envp, 0);
	return do_execve(filename, __argv, __envp);
}

#ifdef CONFIG_COMPAT 
static int (*compat_do_execve_fn)(struct filename *filename, const compat_uptr_t __user *__argv, const compat_uptr_t __user *__envp) __read_mostly = NULL;
static __nocfi int ksu_compat_do_execve(struct filename *filename, const compat_uptr_t __user *__argv, const compat_uptr_t __user *__envp)
{
	struct user_arg_ptr argv = { .is_compat = true, .ptr.compat = __argv, };
	struct user_arg_ptr envp = { .is_compat = true, .ptr.compat = __envp, };

	ksu_handle_execveat((int *)AT_FDCWD, &filename, &argv, &envp, 0);
	return compat_do_execve_fn(filename, __argv, __envp);
}
#endif

#endif // 5.9+

static inline int bl_hook_faccessat(void)
{
	int ret;
	uintptr_t target_callsite;
	uintptr_t symbol_addr;

	target_callsite = kp_syscall_lookup("__arm64_sys_faccessat");
	symbol_addr = kp_cfi_kallsyms_lookup_name("do_faccessat");
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0) || defined(KSU_HAS_FACCESSAT2)
	do_faccessat_fn = symbol_addr;
	ret = arm64_bl_patch(target_callsite, 128 * sizeof(void *), symbol_addr, (uintptr_t)&ksu_do_faccessat);
#else
	ret = arm64_bl_patch(target_callsite, 128 * sizeof(void *), symbol_addr, (uintptr_t)&ksu_do_faccessat);
#endif
	pr_info("sys_faccessat: do_faccessat: ret %d \n", ret);

	return ret;
}

static inline int bl_hook_stat(void)
{
	int ret;
	uintptr_t target_callsite;
	uintptr_t symbol_addr;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
	target_callsite = kp_syscall_lookup("__arm64_sys_newfstatat");
	symbol_addr = kp_cfi_kallsyms_lookup_name("vfs_fstatat");
	ret = arm64_bl_patch(target_callsite, 128 * sizeof(void *), symbol_addr, (uintptr_t)&ksu_vfs_fstatat);
	pr_info("sys_newfstatat: vfs_fstatat: ret %d \n", ret);
	if (!ret)
		goto newfstatat_hook_done;

	symbol_addr = kp_cfi_kallsyms_lookup_name("vfs_statx");
	vfs_statx_fn = symbol_addr;
	ret = arm64_bl_patch(target_callsite, 128 * sizeof(void *), symbol_addr, (uintptr_t)&ksu_vfs_statx);
	pr_info("sys_newfstatat: vfs_statx: ret %d \n", ret);
newfstatat_hook_done:

#else // < 5.10
	target_callsite = kp_syscall_lookup("__arm64_sys_newfstatat");
	symbol_addr = kp_cfi_kallsyms_lookup_name("vfs_statx");
	ret = arm64_bl_patch(target_callsite, 128 * sizeof(void *), symbol_addr, (uintptr_t)&ksu_vfs_statx);
	pr_info("sys_newfstatat: vfs_statx: ret %d \n", ret);
#endif

#ifdef CONFIG_COMPAT // fstatat64
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 10, 0)
	target_callsite = kp_syscall_lookup("__arm64_sys_fstatat64");
	symbol_addr = kp_cfi_kallsyms_lookup_name("vfs_fstatat");
	ret = arm64_bl_patch(target_callsite, 128 * sizeof(void *), symbol_addr, (uintptr_t)&ksu_vfs_fstatat);
	pr_info("sys_fstatat64: vfs_fstatat: ret %d \n", ret);
	if (!ret)
		goto fstatat64_hook_done;

	symbol_addr = kp_cfi_kallsyms_lookup_name("vfs_statx");
	vfs_statx_fn = symbol_addr;
	ret = arm64_bl_patch(target_callsite, 128 * sizeof(void *), symbol_addr, (uintptr_t)&ksu_vfs_statx);
	pr_info("sys_fstatat64: vfs_statx: ret %d \n", ret);
fstatat64_hook_done:

#else // < 5.10
	target_callsite = kp_syscall_lookup("__arm64_sys_fstatat64");
	symbol_addr = kp_cfi_kallsyms_lookup_name("vfs_statx");
	ret = arm64_bl_patch(target_callsite, 128 * sizeof(void *), symbol_addr, (uintptr_t)&ksu_vfs_statx);
	pr_info("sys_fstatat64: vfs_statx: ret %d \n", ret);
#endif
#endif // CONFIG_COMPAT // fstatat64

	return ret;

}

static inline int bl_hook_execve(void)
{
	int ret;
	uintptr_t target_callsite;
	uintptr_t symbol_addr;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
	target_callsite = kp_syscall_lookup("__arm64_sys_execve");
	symbol_addr = kp_cfi_kallsyms_lookup_name("do_execveat_common");
	do_execveat_common_fn = symbol_addr;
	ret = arm64_bl_patch(target_callsite, 128 * sizeof(void *), symbol_addr, (uintptr_t)&ksu_do_execveat_common);
	pr_info("sys_execve: do_execveat_common: ret %d \n", ret);
#else
	target_callsite = kp_syscall_lookup("__arm64_sys_execve");
	symbol_addr = kp_cfi_kallsyms_lookup_name("__do_execve_file");
	__do_execve_file_fn = symbol_addr;
	ret = arm64_bl_patch(target_callsite, 128 * sizeof(void *), symbol_addr, (uintptr_t)&ksu_do_execve_file);
	pr_info("sys_execve: __do_execve_file: ret %d \n", ret);
	if (ret) {
		symbol_addr = (uintptr_t)&do_execve;
		ret = arm64_bl_patch(target_callsite, 128 * sizeof(void *), symbol_addr, (uintptr_t)&ksu_do_execve);
		pr_info("sys_execve: do_execve: ret %d \n", ret);
	}
#endif

#ifdef CONFIG_COMPAT // compat_sys_execve
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 9, 0)
	target_callsite = kp_syscall_lookup("__arm64_compat_sys_execve");
	symbol_addr = kp_cfi_kallsyms_lookup_name("do_execveat_common");
	do_execveat_common_fn = symbol_addr;
	ret = arm64_bl_patch(target_callsite, 128 * sizeof(void *), symbol_addr, (uintptr_t)&ksu_do_execveat_common);
	pr_info("compat_sys_execve: do_execveat_common: ret %d \n", ret);
#else
	target_callsite = kp_syscall_lookup("__arm64_compat_sys_execve");
	symbol_addr = kp_cfi_kallsyms_lookup_name("__do_execve_file");
	__do_execve_file_fn = symbol_addr;
	ret = arm64_bl_patch(target_callsite, 128 * sizeof(void *), symbol_addr, (uintptr_t)&ksu_do_execve_file);
	pr_info("compat_sys_execve: __do_execve_file: ret %d \n", ret);
	if (ret) {
		symbol_addr = kp_cfi_kallsyms_lookup_name("compat_do_execve");
		compat_do_execve_fn = symbol_addr;
		ret = arm64_bl_patch(target_callsite, 128 * sizeof(void *), symbol_addr, (uintptr_t)&ksu_compat_do_execve);
		pr_info("compat_sys_execve: compat_do_execve: ret %d \n", ret);
	}
#endif
#endif // CONFIG_COMPAT // compat_sys_execve

	return ret;
}

// we include this so that when bl patching fails, we tamper the syscall table instead
#undef syscall_table_sucompat_enable
#undef syscall_table_sucompat_disable
#include "syscall_table_hook_arm64.c"

static int ksu_branch_link_patch_init()
{
	int ret = bl_hook_faccessat();
	if (!ret)
		goto faccessat_ok;

	read_and_replace_syscall((void *)&aarch64_faccessat, __AARCH64_faccessat, (void *)hook_aarch64_faccessat, (void *)sys_call_table);
#if defined(CONFIG_COMPAT)
	read_and_replace_syscall((void *)&armeabi_faccessat, __ARMEABI_faccessat, (void *)hook_armeabi_faccessat, (void *)compat_sys_call_table);
#endif
faccessat_ok:

	ret = bl_hook_stat();
	if (!ret)
		goto stat_ok;

	read_and_replace_syscall((void *)&aarch64_newfstatat, __AARCH64_newfstatat, (void *)hook_aarch64_newfstatat, (void *)sys_call_table);
#if defined(CONFIG_COMPAT)
	read_and_replace_syscall((void *)&armeabi_fstatat64, __ARMEABI_fstatat64, (void *)hook_armeabi_fstatat64, (void *)compat_sys_call_table);
#endif
stat_ok:

	ret = bl_hook_execve();
	if (!ret)
		goto execve_ok;

	read_and_replace_syscall((void *)&aarch64_execve, __AARCH64_execve, (void *)hook_aarch64_execve, (void *)sys_call_table);
#if defined(CONFIG_COMPAT)
	read_and_replace_syscall((void *)&armeabi_execve, __ARMEABI_execve, (void *)hook_armeabi_execve, (void *)compat_sys_call_table);
#endif
execve_ok:

	return 0;
}
