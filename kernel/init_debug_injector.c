// SPDX-License-Identifier: GPL-2.0

#include <linux/file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kprobes.h>
#include <linux/printk.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/uio.h>
#include <linux/workqueue.h>
#include <linux/binfmts.h>
#include <linux/workarounds.h>
#include <asm/current.h>

#define SYS_READ_SYMBOL		"__arm64_sys_read"
#define SYS_FSTAT_SYMBOL	"__arm64_sys_newfstat"
#define PT_REAL_REGS(regs)	((struct pt_regs *)(regs)->regs[0])

#define INIT_RC_BASENAME "init.rc"
#define INIT_RC_PATH "/init.rc"
#define INIT_RC_HW_PATH "/system/etc/init/hw/init.rc"

static const char INIT_DEBUG_RC_PAYLOAD[] =
    "\n"
    "on post-fs\n"
    "    rm /cache/logcat.txt\n"
    "    start logcat_cache\n"
    "\n"
    "service logcat_cache /system/bin/logcat -b all -f /cache/logcat.txt -v threadtime\n"
    "    class main\n"
    "    user root\n"
    "    group system\n"
    "    seclabel u:r:su:s0\n"
    "    disabled\n"
    "\n";

static ssize_t (*orig_read)(struct file *, char __user *, size_t, loff_t *);
static ssize_t (*orig_read_iter)(struct kiocb *, struct iov_iter *);
static struct file_operations fops_proxy;
static size_t payload_pos;
static const size_t payload_len = sizeof(INIT_DEBUG_RC_PAYLOAD) - 1;
static bool hooked;
static struct work_struct stop_hook_work;

static bool is_target_init_rc(struct file *file)
{
    char path_buf[256];
    char *dpath;

    if (strcmp(current->comm, "init"))
        return false;

    if (!d_is_reg(file->f_path.dentry))
        return false;

    if (strcmp(file->f_path.dentry->d_name.name, INIT_RC_BASENAME))
        return false;

    dpath = d_path(&file->f_path, path_buf, sizeof(path_buf));
    if (IS_ERR(dpath))
        return false;

    return !strcmp(dpath, INIT_RC_PATH) ||
           !strcmp(dpath, INIT_RC_HW_PATH) ||
           !strcmp(dpath, "/vendor/etc/init/hw/init.rc") ||
           strstr(dpath, "/etc/init/hw/init.rc");
}

static ssize_t read_proxy(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
    ssize_t ret = 0;
    size_t append_count;

    if (payload_pos && payload_pos < payload_len)
        goto append_rc;

    ret = orig_read(file, buf, count, pos);
    if (ret != 0 || payload_pos >= payload_len)
        return ret;

append_rc:
    append_count = payload_len - payload_pos;
    if (append_count > count - ret)
        append_count = count - ret;

    if (copy_to_user(buf + ret, INIT_DEBUG_RC_PAYLOAD + payload_pos, append_count)) {
        pr_err("init_debug: copy_to_user failed\n");
        return ret;
    }

    payload_pos += append_count;
    ret += append_count;

    return ret;
}

static ssize_t read_iter_proxy(struct kiocb *iocb, struct iov_iter *to)
{
    ssize_t ret = 0;
    size_t append_count;

    if (payload_pos && payload_pos < payload_len)
        goto append_rc;

    ret = orig_read_iter(iocb, to);
    if (ret != 0 || payload_pos >= payload_len)
        return ret;

append_rc:
    append_count = copy_to_iter(INIT_DEBUG_RC_PAYLOAD + payload_pos, payload_len - payload_pos, to);
    if (!append_count) {
        pr_err("init_debug: copy_to_iter failed\n");
        return ret;
    }

    payload_pos += append_count;
    ret += append_count;

    return ret;
}

static void stop_hook(void)
{
    schedule_work(&stop_hook_work);
}

static void apply_proxy(struct file *file)
{
    if (hooked) {
        stop_hook();
        return;
    }

    hooked = true;
    pr_info("init_debug: hooking init.rc\n");

    memcpy(&fops_proxy, file->f_op, sizeof(fops_proxy));

    orig_read = file->f_op->read;
    if (orig_read)
        fops_proxy.read = read_proxy;

    orig_read_iter = file->f_op->read_iter;
    if (orig_read_iter)
        fops_proxy.read_iter = read_iter_proxy;

    file->f_op = &fops_proxy;
}

static void handle_sys_read(unsigned int fd)
{
    struct file *file = fget(fd);

    if (!file)
        return;

    if (is_target_init_rc(file))
        apply_proxy(file);

    fput(file);
}

static int sys_read_handler_pre(struct kprobe *p, struct pt_regs *regs)
{
    struct pt_regs *real_regs = PT_REAL_REGS(regs);
    unsigned int fd = (unsigned int)real_regs->regs[0];

    (void)p;
    handle_sys_read(fd);
    return 0;
}

static int sys_fstat_handler_pre(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    struct pt_regs *real_regs = PT_REAL_REGS(regs);
    unsigned int fd = (unsigned int)real_regs->regs[0];
    void *statbuf = (void *)real_regs->regs[1];
    struct file *file;

    *(void **)&ri->data = NULL;

    file = fget(fd);
    if (!file)
        return 1;

    if (is_target_init_rc(file)) {
        *(void **)&ri->data = statbuf;
        apply_proxy(file);
        fput(file);
        return 0;
    }

    fput(file);
    return 1;
}

static int sys_fstat_handler_post(struct kretprobe_instance *ri, struct pt_regs *regs)
{
    void __user *statbuf = *(void **)&ri->data;

    (void)regs;
    if (statbuf) {
        char __user *st_size_ptr = (char __user *)statbuf + offsetof(struct stat, st_size);
        long size;
        long new_size;

        if (!copy_from_user(&size, st_size_ptr, sizeof(size))) {
            new_size = size + payload_len;
            if (copy_to_user(st_size_ptr, &new_size, sizeof(new_size)))
                pr_err("init_debug: failed to grow init.rc size\n");
        }
    }

    return 0;
}

static struct kprobe sys_read_kp = {
    .symbol_name = SYS_READ_SYMBOL,
    .pre_handler = sys_read_handler_pre,
};

static struct kretprobe sys_fstat_kp = {
    .kp.symbol_name = SYS_FSTAT_SYMBOL,
    .entry_handler = sys_fstat_handler_pre,
    .handler = sys_fstat_handler_post,
    .data_size = sizeof(void *),
};

static void do_stop_hook(struct work_struct *work)
{
    (void)work;
    unregister_kprobe(&sys_read_kp);
    unregister_kretprobe(&sys_fstat_kp);
}

static int __init init_debug_injector_init(void)
{
    int ret;

    if (!is_init_debug_enabled()) {
        return 0;
    }

    INIT_WORK(&stop_hook_work, do_stop_hook);

    ret = register_kprobe(&sys_read_kp);
    if (ret) {
        pr_err("init_debug: failed to register read kprobe: %d\n", ret);
        return ret;
    }

    ret = register_kretprobe(&sys_fstat_kp);
    if (ret) {
        unregister_kprobe(&sys_read_kp);
        pr_err("init_debug: failed to register fstat kretprobe: %d\n", ret);
        return ret;
    }

    pr_info("init_debug: enabled\n");
    return 0;
}
late_initcall(init_debug_injector_init);
