#ifdef CONFIG_KSU_SUSFS
#include <linux/susfs_def.h>
#endif // #ifdef CONFIG_KSU_SUSFS

#ifdef CONFIG_KSU_SUSFS
static inline bool is_zygote_isolated_service_uid(uid_t uid)
{
    uid %= 100000;
    return (uid >= 99000 && uid < 100000);
}

static inline bool is_zygote_normal_app_uid(uid_t uid)
{
    uid %= 100000;
    return (uid >= 10000 && uid < 19999);
}

extern u32 susfs_zygote_sid;
extern struct cred *ksu_cred;
extern struct work_struct susfs_extra_works;

struct susfs_handle_setuid_tw {
    struct callback_head cb;
};

static void susfs_handle_setuid_tw_func(struct callback_head *cb)
{
    struct susfs_handle_setuid_tw *tw = container_of(cb, struct susfs_handle_setuid_tw, cb);
    const struct cred *saved = override_creds(ksu_cred);

    revert_creds(saved);
    kfree(tw);
}

static void ksu_handle_extra_susfs_work(void)
{
    struct susfs_handle_setuid_tw *tw = kzalloc(sizeof(*tw), GFP_ATOMIC);
    
    if (work_pending(&susfs_extra_works))
        return;

    schedule_work(&susfs_extra_works);

    if (!tw) {
        pr_err("susfs: No enough memory\n");
        return;
    }

    tw->cb.func = susfs_handle_setuid_tw_func;

    int err = task_work_add(current, &tw->cb, TWA_RESUME);
    if (err) {
        kfree(tw);
        pr_err("susfs: Failed adding task_work 'susfs_handle_setuid_tw', err: %d\n", err);
    }
}
#ifdef CONFIG_KSU_SUSFS_TRY_UMOUNT
extern void susfs_try_umount(uid_t uid);
#endif // #ifdef CONFIG_KSU_SUSFS_TRY_UMOUNT
#endif // #ifdef CONFIG_KSU_SUSFS

static __always_inline void ksu_handle_setresuid_cred(struct cred *new, const struct cred *old)
{
	if (!new || !old)
		return;

	uid_t new_uid = ksu_get_uid_t(new->uid);
	uid_t old_uid = ksu_get_uid_t(old->uid);

	// old process is not root, ignore it.
	if (unlikely(!!old_uid))
		return;
#ifdef CONFIG_KSU_SUSFS_SUS_MOUNT
    // Check if spawned process is isolated service first, and force to do umount if so  
    if (is_zygote_isolated_service_uid(new_uid)) {
        goto do_umount;
    }
#endif // #ifdef CONFIG_KSU_SUSFS_SUS_MOUNT

	if (IS_ENABLED(CONFIG_KSU_DEBUG))
		pr_info("handle_setresuid from %d to %d\n", old_uid, new_uid);

	// we dont have those new fancy things upstream has
	// lets just do the original thing where we disable seccomp
	if (unlikely(is_uid_manager(new_uid)))
		goto install_ksu_fd;

	if (ksu_is_allow_uid_for_current(new_uid))
		goto kill_seccomp;

	// Handle kernel umount
do_umount:
    // Handle kernel umount
#ifndef CONFIG_KSU_SUSFS_TRY_UMOUNT
    return ksu_handle_umount(new, old);
#else
    susfs_try_umount(new_uid);
#endif // #ifndef CONFIG_KSU_SUSFS_TRY_UMOUNT

#ifdef CONFIG_KSU_SUSFS_SUS_PATH
    //susfs_run_sus_path_loop(new_uid);
#endif // #ifdef CONFIG_KSU_SUSFS_SUS_PATH

#ifdef CONFIG_KSU_SUSFS
    ksu_handle_extra_susfs_work();

    susfs_set_current_proc_umounted();

    return;
#endif // #ifdef CONFIG_KSU_SUSFS
	return;

install_ksu_fd:
	pr_info("install fd for manager: %d\n", new_uid);
	ksu_install_fd();

kill_seccomp:
	disable_seccomp();
	return;
}
