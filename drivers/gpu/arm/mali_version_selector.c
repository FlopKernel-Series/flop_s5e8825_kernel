// SPDX-License-Identifier: GPL-2.0
/*
 * Mali KMD version selector
 *
 * Parses mali.version= cmdline param and exports the selected version
 * string for Mali KMD modules to check at init time.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/string.h>

char mali_selected_version[8] = {0};
EXPORT_SYMBOL(mali_selected_version);

static int __init mali_version_setup(char *str)
{
	if (str)
		strscpy(mali_selected_version, str, sizeof(mali_selected_version));
	return 1;
}
__setup("mali.version=", mali_version_setup);
