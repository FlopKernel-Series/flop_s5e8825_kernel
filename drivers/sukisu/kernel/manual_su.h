#ifndef __KSU_MANUAL_SU_H
#define __KSU_MANUAL_SU_H

#include <linux/types.h>
#include <linux/sched.h>
#include <linux/version.h>

#define KSU_SU_VERIFIED_BIT (1UL << 0)

struct su_request_arg {
    pid_t target_pid;
    const char __user *user_password;
};

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 10, 0)
static inline bool ksu_is_current_verified(void)
{
    return ((unsigned long)(current->cred->security) & KSU_SU_VERIFIED_BIT) != 0;
}

static inline void ksu_mark_current_verified(void)
{
    current->cred->security = (void *)((unsigned long)(current->cred->security) | KSU_SU_VERIFIED_BIT);
}
#else
static inline bool ksu_is_current_verified(void)
{
    return ((unsigned long)(current->security) & KSU_SU_VERIFIED_BIT) != 0;
}

static inline void ksu_mark_current_verified(void)
{
    current->security = (void *)((unsigned long)(current->security) | KSU_SU_VERIFIED_BIT);
}
#endif

int ksu_manual_su_escalate(uid_t target_uid, pid_t target_pid,
                           const char __user *user_password);

bool is_pending_root(uid_t uid);
void remove_pending_root(uid_t uid);
void add_pending_root(uid_t uid);
bool is_current_verified(void);
extern bool current_verified;
#endif