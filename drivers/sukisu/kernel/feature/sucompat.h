#ifndef __KSU_H_SUCOMPAT
#define __KSU_H_SUCOMPAT
#include <asm/ptrace.h>
#include <linux/types.h>
#include "compat/kernel_compat.h"

#ifdef KSU_COMPAT_USE_STATIC_KEY
extern struct static_key_true ksu_su_compat_enabled;
#else
extern bool ksu_su_compat_enabled;
#endif

void ksu_sucompat_init(void);
void ksu_sucompat_exit(void);

// Handler functions exported for hook_manager
int ksu_handle_faccessat(int *dfd, const char __user **filename_user, int *mode, int *__unused_flags);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0) && defined(CONFIG_KSU_SUSFS)
int ksu_handle_stat(int *dfd, struct filename **filename, int *flags);
#else
int ksu_handle_stat(int *dfd, const char __user **filename_user, int *flags);
#endif // #if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0) && defined(CONFIG_KSU_SUSFS)

#ifdef CONFIG_KSU_TRACEPOINT_HOOK
// WARNING! THERE HAVE TRYING TO CALL SYSCALL INTERNALLY
// ENSURE CALL IT ONLY IN TRACEPOINT SYSCALL REDIRECT
int ksu_handle_execve_sucompat_tp_internal(const char __user **filename_user, int orig_nr, const struct pt_regs *regs);
#else

#ifdef CONFIG_64BIT
// ensure this sync with susfs in 64bit kernel!!!
// we use our custom func replace susfs's mark func
#define TIF_PROC_UMOUNTED 33
#else
// 31 already used as TIF_KSU_DISABLE_ESCAPE_WITH_ROOT
#define TIF_PROC_UMOUNTED 30
#endif

static inline bool ksu_is_current_proc_umounted(void)
{
    return (likely(test_thread_flag(TIF_PROC_UMOUNTED)));
}

static inline void ksu_set_current_proc_umounted(void)
{
    set_thread_flag(TIF_PROC_UMOUNTED);
}
#endif

#endif