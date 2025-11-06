#include "linux/compiler.h"
#include "linux/printk.h"
#include "selinux/selinux.h"
#include <linux/dcache.h>
#include <linux/security.h>
#include <asm/current.h>
#include <linux/cred.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/kprobes.h>
#include <linux/tracepoint.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/ptrace.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0)
#include <linux/sched/task_stack.h>
#else
#include <linux/sched.h>
#endif
#include <asm/syscall.h>
#include <trace/events/syscalls.h>
#ifdef CONFIG_KSU_SUSFS_SUS_SU
#include <linux/susfs_def.h>
#endif

#include "objsec.h"
#include "allowlist.h"
#include "arch.h"
#include "feature.h"
#include "klog.h" // IWYU pragma: keep
#include "ksud.h"
#include "kernel_compat.h"
#include "sucompat.h"
#include "core_hook.h"
#include "sulog.h"

#define SU_PATH "/system/bin/su"
#define SH_PATH "/system/bin/sh"
extern void escape_to_root();
void ksu_sucompat_enable();
void ksu_sucompat_disable();

void ksu_mark_running_process(void)
{
    struct task_struct *p, *t;
    read_lock(&tasklist_lock);
    for_each_process_thread (p, t) {
        if (!t->mm) { // only user processes
            continue;
        }
        int uid = task_uid(t).val;
        bool ksu_root_process =
            uid == 0 && is_task_ksu_domain(get_task_cred(t));
        if (ksu_root_process || ksu_is_allow_uid(uid)) {
            ksu_set_task_tracepoint_flag(t);
            pr_info("sucompat: mark process: pid:%d, uid: %d, comm:%s\n",
                    t->pid, uid, t->comm);
        }
    }
    read_unlock(&tasklist_lock);
}

static void handle_process_mark(bool mark)
{
    struct task_struct *p, *t;
    read_lock(&tasklist_lock);
    for_each_process_thread(p, t) {
        if (mark)
            ksu_set_task_tracepoint_flag(t);
        else
            ksu_clear_task_tracepoint_flag(t);
    }
    read_unlock(&tasklist_lock);
}

static void mark_all_process(void)
{
    handle_process_mark(true);
    pr_info("sucompat: mark all user process done!\n");
}

static void unmark_all_process(void)
{
    handle_process_mark(false);
    pr_info("sucompat: unmark all user process done!\n");
}

bool ksu_su_compat_enabled __read_mostly = true;

static int su_compat_feature_get(u64 *value)
{
    *value = ksu_su_compat_enabled ? 1 : 0;
    return 0;
}

static int su_compat_feature_set(u64 value)
{
    bool enable = value != 0;

    if (enable == ksu_su_compat_enabled) {
        pr_info("su_compat: no need to change\n");
        return 0;
    }

    if (enable) {
        ksu_sucompat_enable();
    } else {
        ksu_sucompat_disable();
    }

    ksu_su_compat_enabled = enable;
    pr_info("su_compat: set to %d\n", enable);

    return 0;
}

static const struct ksu_feature_handler su_compat_handler = {
    .feature_id = KSU_FEATURE_SU_COMPAT,
    .name = "su_compat",
    .get_handler = su_compat_feature_get,
    .set_handler = su_compat_feature_set,
};

static const char sh_path[] = "/system/bin/sh";
static const char ksud_path[] = KSUD_PATH;
static const char su[] = SU_PATH;

bool ksu_sucompat_hook_state __read_mostly = true;

static inline void __user *userspace_stack_buffer(const void *d, size_t len)
{
    /* To avoid having to mmap a page in userspace, just write below the stack
   * pointer. */
    char __user *p = (void __user *)current_user_stack_pointer() - len;

    return copy_to_user(p, d, len) ? NULL : p;
}

static inline char __user *sh_user_path(void)
{
    return userspace_stack_buffer(sh_path, sizeof(sh_path));
}

static inline char __user *ksud_user_path(void)
{
    return userspace_stack_buffer(ksud_path, sizeof(ksud_path));
}

int ksu_handle_faccessat(int *dfd, const char __user **filename_user, int *mode,
             int *__unused_flags)
{

#ifndef KSU_HAVE_SYSCALL_TRACEPOINTS_HOOK
    if (!ksu_sucompat_hook_state) {
        return 0;
    }
#endif

#ifndef CONFIG_KSU_SUSFS_SUS_SU
    if (!ksu_is_allow_uid_for_current(current_uid().val)) {
        return 0;
    }
#endif

#ifdef CONFIG_KSU_SUSFS_SUS_SU
    char path[sizeof(su) + 1] = {0};
#else
    char path[sizeof(su) + 1];
    memset(path, 0, sizeof(path));
#endif
    ksu_strncpy_from_user_nofault(path, *filename_user, sizeof(path));

    if (unlikely(!memcmp(path, su, sizeof(su)))) {
#if __SULOG_GATE
        ksu_sulog_report_syscall(current_uid().val, NULL, "faccessat", path);
#endif
        pr_info("faccessat su->sh!\n");
        *filename_user = sh_user_path();
    }

    return 0;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0) && defined(CONFIG_KSU_SUSFS_SUS_SU)
struct filename* susfs_ksu_handle_stat(int *dfd, const char __user **filename_user, int *flags) {
    struct filename *name = getname_flags(*filename_user, getname_statx_lookup_flags(*flags), NULL);

    if (unlikely(IS_ERR(name) || name->name == NULL)) {
        return name;
    }

    if (likely(memcmp(name->name, su, sizeof(su)))) {
        return name;
    }

    const char sh[] = SH_PATH;
#if __SULOG_GATE
    ksu_sulog_report_syscall(current_uid().val, NULL, "vfs_fstatat", sh);
#endif
    pr_info("vfs_fstatat su->sh!\n");
    memcpy((void *)name->name, sh, sizeof(sh));
    return name;
}
#endif

int ksu_handle_stat(int *dfd, const char __user **filename_user, int *flags)
{

#ifndef KSU_HAVE_SYSCALL_TRACEPOINTS_HOOK
    if (!ksu_sucompat_hook_state) {
        return 0;
    }
#endif

#ifndef CONFIG_KSU_SUSFS_SUS_SU
    if (!ksu_is_allow_uid_for_current(current_uid().val)) {
        return 0;
    }
#endif

    if (unlikely(!filename_user)) {
        return 0;
    }

#ifdef CONFIG_KSU_SUSFS_SUS_SU
    char path[sizeof(su) + 1] = {0};
#else
    char path[sizeof(su) + 1];
    memset(path, 0, sizeof(path));
#endif
// Remove this later!! we use syscall hook, so this will never happen!!!!!
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 18, 0) && 0
    // it becomes a `struct filename *` after 5.18
    // https://elixir.bootlin.com/linux/v5.18/source/fs/stat.c#L216
    const char sh[] = SH_PATH;
    struct filename *filename = *((struct filename **)filename_user);
    if (IS_ERR(filename)) {
        return 0;
    }
    if (likely(memcmp(filename->name, su, sizeof(su))))
        return 0;
    pr_info("vfs_statx su->sh!\n");
    memcpy((void *)filename->name, sh, sizeof(sh));
#else
    ksu_strncpy_from_user_nofault(path, *filename_user, sizeof(path));

    if (unlikely(!memcmp(path, su, sizeof(su)))) {
#if __SULOG_GATE
        ksu_sulog_report_syscall(current_uid().val, NULL, "newfstatat", path);
#endif
        pr_info("newfstatat su->sh!\n");
        *filename_user = sh_user_path();
    }
#endif

    return 0;
}

int ksu_handle_execveat(int *fd, struct filename **filename_ptr, void *argv,
            void *envp, int *flags)
{
    return ksu_handle_execveat_sucompat(fd, filename_ptr, argv, envp, flags);
}

// the call from execve_handler_pre won't provided correct value for __never_use_argument, use them after fix execve_handler_pre, keeping them for consistence for manually patched code
int ksu_handle_execveat_sucompat(int *fd, struct filename **filename_ptr,
                 void *__never_use_argv, void *__never_use_envp,
                 int *__never_use_flags)
{
    struct filename *filename;

#ifndef KSU_HAVE_SYSCALL_TRACEPOINTS_HOOK
    if (!ksu_sucompat_hook_state) {
        return 0;
    }
#endif

    if (unlikely(!filename_ptr))
        return 0;

    filename = *filename_ptr;
    if (IS_ERR(filename)) {
        return 0;
    }

    if (likely(memcmp(filename->name, su, sizeof(su))))
        return 0;
    
#if __SULOG_GATE
    ksu_sulog_report_syscall(current_uid().val, NULL, "execve", filename->name);
#ifndef CONFIG_KSU_SUSFS_SUS_SU
    bool is_allowed = ksu_is_allow_uid_for_current(current_uid().val);
#endif
#endif

#ifndef CONFIG_KSU_SUSFS_SUS_SU

#if __SULOG_GATE
    if (!is_allowed)
        return 0;
    
    ksu_sulog_report_su_attempt(current_uid().val, NULL, filename->name, is_allowed);
#else
    if (!ksu_is_allow_uid_for_current(current_uid().val)) {
        return 0;
    }
#endif
#endif

    pr_info("do_execveat_common su found\n");
    memcpy((void *)filename->name, ksud_path, sizeof(ksud_path));

    escape_to_root();

    return 0;
}

int ksu_handle_execve_sucompat(int *fd, const char __user **filename_user,
                   void *__never_use_argv, void *__never_use_envp,
                   int *__never_use_flags)
{
    //const char su[] = SU_PATH;
#ifdef CONFIG_KSU_SUSFS_SUS_SU
    char path[sizeof(su) + 1] = {0};
#else
    char path[sizeof(su) + 1];
#endif

#ifndef KSU_HAVE_SYSCALL_TRACEPOINTS_HOOK
    if (!ksu_sucompat_hook_state) {
        return 0;
    }
#endif

    if (unlikely(!filename_user))
        return 0;

    /*
     * nofault variant fails silently due to pagefault_disable
     * some cpus dont really have that good speculative execution
     * access_ok to substitute set_fs, we check if pointer is accessible
     */
    if (!ksu_access_ok(*filename_user, sizeof(path)))
        return 0;

    // success = returns number of bytes and should be less than path
    long len = strncpy_from_user(path, *filename_user, sizeof(path));
    if (len <= 0 || len > sizeof(path))
        return 0;
    // strncpy_from_user_nofault does this too
    path[sizeof(path) - 1] = '\0';

    if (likely(memcmp(path, su, sizeof(su))))
        return 0;

#if __SULOG_GATE
    ksu_sulog_report_syscall(current_uid().val, NULL, "execve", path);
    bool is_allowed = ksu_is_allow_uid_for_current(current_uid().val);
    if (!is_allowed)
        return 0;
    
    ksu_sulog_report_su_attempt(current_uid().val, NULL, path, is_allowed);
#else
    if (!ksu_is_allow_uid_for_current(current_uid().val)) {
        return 0;
    }
#endif

    pr_info("sys_execve su found\n");
    *filename_user = ksud_user_path();

    escape_to_root();

    return 0;
}

// dummified
int ksu_handle_devpts(struct inode *inode)
{
    return 0;
}

int __ksu_handle_devpts(struct inode *inode)
{

#ifndef KSU_HAVE_SYSCALL_TRACEPOINTS_HOOK
    if (!ksu_sucompat_hook_state)
        return 0;
#endif

    if (!current->mm) {
        return 0;
    }

    uid_t uid = current_uid().val;
    if (uid % 100000 < 10000) {
        // not untrusted_app, ignore it
        return 0;
    }

    if (likely(!ksu_is_allow_uid_for_current(uid)))
        return 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 1, 0) || defined(KSU_OPTIONAL_SELINUX_INODE)
        struct inode_security_struct *sec = selinux_inode(inode);
#else
        struct inode_security_struct *sec =
            (struct inode_security_struct *)inode->i_security;
#endif
    if (ksu_devpts_sid && sec)
        sec->sid = ksu_devpts_sid;

    return 0;
}

#ifdef KSU_HAVE_SYSCALL_TRACEPOINTS_HOOK

// Tracepoint probe for sys_enter
static void sucompat_sys_enter_handler(void *data, struct pt_regs *regs,
                                       long id)
{
    // Handle newfstatat
    if (unlikely(id == __NR_newfstatat)) {
        int *dfd = (int *)&PT_REGS_PARM1(regs);
        const char __user **filename_user =
            (const char __user **)&PT_REGS_PARM2(regs);
        int *flags = (int *)&PT_REGS_SYSCALL_PARM4(regs);
        ksu_handle_stat(dfd, filename_user, flags);
        return;
    }

    // Handle faccessat
    if (unlikely(id == __NR_faccessat)) {
        int *dfd = (int *)&PT_REGS_PARM1(regs);
        const char __user **filename_user =
            (const char __user **)&PT_REGS_PARM2(regs);
        int *mode = (int *)&PT_REGS_PARM3(regs);
        ksu_handle_faccessat(dfd, filename_user, mode, NULL);
        return;
    }

    // Handle execve
    if (unlikely(id == __NR_execve)) {
        const char __user **filename_user =
            (const char __user **)&PT_REGS_PARM1(regs);
        ksu_handle_execve_sucompat(AT_FDCWD, filename_user, NULL, NULL, NULL);
        return;
    }
}

#endif // KSU_HAVE_SYSCALL_TRACEPOINTS_HOOK

#ifdef KSU_KPROBES_HOOK

static int pts_unix98_lookup_pre(struct kprobe *p, struct pt_regs *regs)
{
    struct inode *inode;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 6, 0)
    struct file *file = (struct file *)PT_REGS_PARM2(regs);
    inode = file->f_path.dentry->d_inode;
#else
    inode = (struct inode *)PT_REGS_PARM2(regs);
#endif

    return ksu_handle_devpts(inode);
}

static struct kprobe *init_kprobe(const char *name,
                                  kprobe_pre_handler_t handler)
{
    struct kprobe *kp = kzalloc(sizeof(struct kprobe), GFP_KERNEL);
    if (!kp)
        return NULL;
    kp->symbol_name = name;
    kp->pre_handler = handler;

    int ret = register_kprobe(kp);
    pr_info("sucompat: register_%s kprobe: %d\n", name, ret);
    if (ret) {
        kfree(kp);
        return NULL;
    }

    return kp;
}

static void destroy_kprobe(struct kprobe **kp_ptr)
{
    struct kprobe *kp = *kp_ptr;
    if (!kp)
        return;
    unregister_kprobe(kp);
    synchronize_rcu();
    kfree(kp);
    *kp_ptr = NULL;
}

static struct kprobe *pts_kp = NULL;
#endif

#ifdef CONFIG_KRETPROBES

static int tracepoint_reg_count = 0;
static DEFINE_MUTEX(tracepoint_reg_mutex);

static int syscall_regfunc_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    mutex_lock(&tracepoint_reg_mutex);
    if (tracepoint_reg_count < 1) {
        // while install our tracepoint, mark our processes
        unmark_all_process();
        ksu_mark_running_process();
    } else {
        // while installing other tracepoint, mark all processes
        mark_all_process();
    }
    tracepoint_reg_count++;
    mutex_unlock(&tracepoint_reg_mutex);
    return 0;
}

static int syscall_unregfunc_handler(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    mutex_lock(&tracepoint_reg_mutex);
    if (tracepoint_reg_count <= 1) {
        // while uninstall our tracepoint, unmark all processes
        unmark_all_process();
    } else {
        // while uninstalling other tracepoint, mark our processes
        unmark_all_process();
        ksu_mark_running_process();
    }
    tracepoint_reg_count--;
    mutex_unlock(&tracepoint_reg_mutex);
    return 0;
}

struct kretprobe syscall_regfunc_rp = {
    .kp.symbol_name = "syscall_regfunc",
    .handler = syscall_regfunc_handler,
    .entry_handler = NULL,
    .data_size = 0,
    .maxactive = 0,
};

struct kretprobe syscall_unregfunc_rp = {
    .kp.symbol_name = "syscall_unregfunc",
    .handler = syscall_unregfunc_handler,
    .entry_handler = NULL,
    .data_size = 0,
    .maxactive = 0,
};
#endif

void ksu_sucompat_enable()
{
    int ret;
    pr_info("sucompat: ksu_sucompat_enable called\n");

#ifdef KSU_KPROBES_HOOK
    // Register kprobe for pts_unix98_lookup
    pts_kp = init_kprobe("pts_unix98_lookup", pts_unix98_lookup_pre);
#endif

#ifdef CONFIG_KRETPROBES
    ret = register_kretprobe(&syscall_regfunc_rp);
    if (ret) {
        pr_err("sucompat: failed to register syscall_regfunc kretprobe: %d\n", ret);
    } else {
        pr_info("sucompat: syscall_regfunc kretprobe registered\n");
    }
    ret = register_kretprobe(&syscall_unregfunc_rp);
    if (ret) {
        pr_err("sucompat: failed to register syscall_unregfunc kretprobe: %d\n", ret);
    } else {
        pr_info("sucompat: syscall_unregfunc kretprobe registered\n");
    }
#endif

#ifdef KSU_HAVE_SYSCALL_TRACEPOINTS_HOOK
    // Register sys_enter tracepoint for syscall interception
    ret = register_trace_sys_enter(sucompat_sys_enter_handler, NULL);
#ifndef CONFIG_KRETPROBES
    unmark_all_process();
    ksu_mark_running_process();
#endif
    if (ret) {
        pr_err("sucompat: failed to register sys_enter tracepoint: %d\n", ret);
    } else {
        pr_info("sucompat: sys_enter tracepoint registered\n");
    }
#else
    ksu_sucompat_hook_state = true;
     pr_info("ksu_sucompat_init: hooks enabled: execve/execveat_su, faccessat, stat\n");
#endif
}

void ksu_sucompat_disable()
{
    pr_info("sucompat: ksu_sucompat_disable called\n");
#ifdef KSU_HAVE_SYSCALL_TRACEPOINTS_HOOK
    // Unregister sys_enter tracepoint
    unregister_trace_sys_enter(sucompat_sys_enter_handler, NULL);
    tracepoint_synchronize_unregister();
    pr_info("sucompat: sys_enter tracepoint unregistered\n");
#else
    ksu_sucompat_hook_state = false;
    pr_info("ksu_sucompat_exit: hooks disabled: execve/execveat_su, faccessat, stat\n");
#endif

#ifdef CONFIG_KRETPROBES
    // Unregister syscall_regfunc kretprobe
    unregister_kretprobe(&syscall_regfunc_rp);
    pr_info("sucompat: syscall_regfunc kretprobe unregistered\n");
    // Unregister syscall_unregfunc kretprobe
    unregister_kretprobe(&syscall_unregfunc_rp);
    pr_info("sucompat: syscall_unregfunc kretprobe unregistered\n");
#endif

#ifdef KSU_KPROBES_HOOK
    // Unregister pts_unix98_lookup kprobe
    destroy_kprobe(&pts_kp);
#endif
}

// sucompat: permited process can execute 'su' to gain root access.
void ksu_sucompat_init()
{
    if (ksu_register_feature_handler(&su_compat_handler)) {
        pr_err("Failed to register su_compat feature handler\n");
    }
    if (ksu_su_compat_enabled) {
        ksu_sucompat_enable();
    }
}

void ksu_sucompat_exit()
{
    if (ksu_su_compat_enabled) {
        ksu_sucompat_disable();
    }
    ksu_unregister_feature_handler(KSU_FEATURE_SU_COMPAT);
}

#ifdef CONFIG_KSU_SUSFS_SUS_SU
extern bool ksu_su_compat_enabled;
bool ksu_devpts_hook = false;
bool susfs_is_sus_su_hooks_enabled __read_mostly = false;
int susfs_sus_su_working_mode = 0;

static bool ksu_is_su_kps_enabled(void) {
#ifdef KSU_KPROBES_HOOK
    destroy_kprobe(&pts_kp);
#endif
    return false;
}

void ksu_susfs_disable_sus_su(void) {
    susfs_is_sus_su_hooks_enabled = false;
    ksu_devpts_hook = false;
    susfs_sus_su_working_mode = SUS_SU_DISABLED;
    // Re-enable the su_kps for user, users need to toggle off the kprobe hooks again in ksu manager if they want it disabled.
    if (!ksu_is_su_kps_enabled()) {
        ksu_sucompat_init();
        ksu_su_compat_enabled = true;
    }
}

void ksu_susfs_enable_sus_su(void) {
    if (ksu_is_su_kps_enabled()) {
        ksu_sucompat_exit();
        ksu_su_compat_enabled = false;
    }
    susfs_is_sus_su_hooks_enabled = true;
    ksu_devpts_hook = true;
    susfs_sus_su_working_mode = SUS_SU_WITH_HOOKS;
}
#endif // #ifdef CONFIG_KSU_SUSFS_SUS_SU