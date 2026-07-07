// SPDX-License-Identifier: GPL-2.0
/*
 * Mali KMD compatibility stub
 *
 * Provides /sys/module/mali_kbase/ for userspace tooling compatibility.
 * The real Mali KMD logic lives in versioned modules
 * (mali_kbase_r32p1, mali_kbase_r38p1, mali_kbase_r44p1).
 *
 * On init, loads the version selected by mali.version= cmdline
 * so the redirector is populated before external consumers probe.
 */
#include <linux/module.h>
#include <linux/kmod.h>
#include <linux/sysfs.h>

MODULE_DESCRIPTION("Mali KMD compatibility stub");
MODULE_LICENSE("GPL");
MODULE_SOFTDEP("pre: mali_kbase_r32p1 mali_kbase_r38p1 mali_kbase_r44p1");

extern char mali_selected_version[];

static const char *mali_kbase_version;

static ssize_t version_show(struct kobject *kobj,
			    struct kobj_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%s\n",
			 mali_kbase_version ? mali_kbase_version : "unknown");
}
static struct kobj_attribute version_attr = __ATTR_RO(version);

static int __init mali_kbase_compat_init(void)
{
	char mod_name[64];

	snprintf(mod_name, sizeof(mod_name), "mali_kbase_%s",
		 mali_selected_version[0] ? mali_selected_version : "r38p1");
	request_module(mod_name);

	if (strcmp(mali_selected_version, "r32p1") == 0)
		mali_kbase_version = "r32p1-01bet2 (UK version 11.31)";
	else if (strcmp(mali_selected_version, "r44p1") == 0)
		mali_kbase_version = "r44p1-01eac0 (UK version 11.39)";
	else
		mali_kbase_version = "r38p1-01eac0 (UK version 11.35)";

	sysfs_create_file(&THIS_MODULE->mkobj.kobj, &version_attr.attr);
	return 0;
}
module_init(mali_kbase_compat_init);
