// SPDX-License-Identifier: GPL-2.0
/*
 * Mali KMD compatibility stub
 *
 * Provides /sys/module/mali_kbase/ for userspace tooling compatibility.
 * The real Mali KMD logic lives in versioned modules
 * (mali_kbase_r32p1, mali_kbase_r38p1, mali_kbase_r44p1, mali_kbase_r49p1,
 * mali_kbase_r54p3).
 *
 * On init, loads the version selected by mali.version= cmdline
 * so the redirector is populated before external consumers probe.
 */
#include <linux/module.h>
#include <linux/kmod.h>
#include <linux/sysfs.h>
#include <linux/fs.h>
#include <linux/uio.h>
#include <linux/namei.h>

MODULE_DESCRIPTION("Mali KMD compatibility stub");
MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: mali_kbase_r32p1 mali_kbase_r38p1 mali_kbase_r44p1 mali_kbase_r49p1 mali_kbase_r54p3");
MODULE_IMPORT_NS(VFS_internal_I_am_really_a_filesystem_and_am_NOT_a_driver);

extern char mali_selected_version[];

static const char *mali_kbase_version;

static ssize_t version_show(struct kobject *kobj,
			    struct kobj_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%s\n",
			 mali_kbase_version ? mali_kbase_version : "unknown");
}
static struct kobj_attribute version_attr = __ATTR_RO(version);

/*
 * Module parameters the selected KMD publishes. Userspace only knows the
 * non-versioned mali_kbase path, so mirror them under parameters/.
 */
static ssize_t kmd_param_show(const char *name, char *buf)
{
	char path[64];
	struct file *filp;
	struct kiocb kiocb;
	struct kvec kvec;
	struct iov_iter iter;
	ssize_t ret;

	snprintf(path, sizeof(path), "/sys/module/mali_kbase_%s/parameters/%s",
		 mali_selected_version[0] ? mali_selected_version : "r38p1",
		 name);

	filp = filp_open(path, O_RDONLY, 0);
	if (IS_ERR(filp))
		return -ENODEV;

	kvec.iov_base = buf;
	kvec.iov_len = PAGE_SIZE - 1;
	iov_iter_kvec(&iter, READ, &kvec, 1, kvec.iov_len);

	init_sync_kiocb(&kiocb, filp);
	kiocb.ki_pos = 0;

	ret = filp->f_op->read_iter(&kiocb, &iter);
	if (ret >= 0)
		buf[ret] = '\0';

	filp_close(filp, NULL);

	return ret;
}

static ssize_t large_page_conf_show(struct kobject *kobj,
				    struct kobj_attribute *attr, char *buf)
{
	return kmd_param_show("large_page_conf", buf);
}
static struct kobj_attribute large_page_conf_attr = __ATTR_RO(large_page_conf);

static ssize_t fully_backed_gpf_memory_show(struct kobject *kobj,
					    struct kobj_attribute *attr, char *buf)
{
	return kmd_param_show("fully_backed_gpf_memory", buf);
}
static struct kobj_attribute fully_backed_gpf_memory_attr =
	__ATTR_RO(fully_backed_gpf_memory);

static struct attribute *mali_kbase_params[] = {
	&large_page_conf_attr.attr,
	&fully_backed_gpf_memory_attr.attr,
	NULL,
};

static const struct attribute_group mali_kbase_params_group = {
	.name = "parameters",
	.attrs = mali_kbase_params,
};

static bool mali_kbase_params_added;

static int __init mali_kbase_compat_init(void)
{
	char mod_name[64];
	int ret;

	snprintf(mod_name, sizeof(mod_name), "mali_kbase_%s",
		 mali_selected_version[0] ? mali_selected_version : "r38p1");
	request_module(mod_name);

	if (strcmp(mali_selected_version, "r32p1") == 0)
		mali_kbase_version = "r32p1-01bet2 (UK version 11.31)";
	else if (strcmp(mali_selected_version, "r44p1") == 0)
		mali_kbase_version = "r44p1-01eac0 (UK version 11.39)";
	else if (strcmp(mali_selected_version, "r49p1") == 0)
		mali_kbase_version = "r49p1-04eac0 (UK version 11.46)";
	else if (strcmp(mali_selected_version, "r54p3") == 0)
		mali_kbase_version = "r54p3-01eac0 (UK version 11.46)";
	else
		mali_kbase_version = "r38p1-01eac0 (UK version 11.35)";

	sysfs_create_file(&THIS_MODULE->mkobj.kobj, &version_attr.attr);

	/* Only r49p1 onwards provide these parameters. */
	if (strcmp(mali_selected_version, "r49p1") == 0 ||
	    strcmp(mali_selected_version, "r54p3") == 0) {
		ret = sysfs_create_group(&THIS_MODULE->mkobj.kobj,
					 &mali_kbase_params_group);
		if (!ret)
			mali_kbase_params_added = true;
	}

	return 0;
}
module_init(mali_kbase_compat_init);

static void __exit mali_kbase_compat_exit(void)
{
	sysfs_remove_file(&THIS_MODULE->mkobj.kobj, &version_attr.attr);

	if (mali_kbase_params_added)
		sysfs_remove_group(&THIS_MODULE->mkobj.kobj,
				   &mali_kbase_params_group);
}
module_exit(mali_kbase_compat_exit);
