// SPDX-License-Identifier: GPL-2.0
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>

#ifdef CONFIG_SEC_DETECT_CMDLINE_PATCH
#include <linux/string.h>
#include <linux/sec_detect.h>

#define BUF_SIZE 4096

static void cmdline_panel_patch(char *dest, size_t dest_size, const char *src)
{
	char *buf;
	char *pos, *token;
	const char *delim = " ";
	int offset = 0;
	int remaining, written;

	buf = kmalloc(BUF_SIZE, GFP_KERNEL);
	if (!buf) {
		strncpy(dest, src, dest_size);
		dest[dest_size - 1] = '\0';
		return;
	}

	strncpy(buf, src, BUF_SIZE - 1);
	buf[BUF_SIZE - 1] = '\0';
	pos = buf;

	memset(dest, 0, dest_size);

	while ((token = strsep(&pos, delim)) != NULL) {
		remaining = dest_size - offset;

		if (remaining <= 0)
			break;

		if (sec_needs_decon) {
			if (strstr(token, "mcd-panel.") == token) {
				written = snprintf(dest + offset, remaining, "mcd-panel-decon%s ", token + strlen("mcd-panel"));
			} else if (strstr(token, "exynos-drm.") == token) {
				written = snprintf(dest + offset, remaining, "exynos-drm-decon%s ", token + strlen("exynos-drm"));
			} else if (strstr(token, "mcd-panel-samsung-drv.") == token) {
				written = snprintf(dest + offset, remaining, "mcd-panel-samsung-drv-decon%s ", token + strlen("mcd-panel-samsung-drv"));
			} else {
				written = snprintf(dest + offset, remaining, "%s ", token);
			}
		} else {
			if (strstr(token, "mcd-panel-samsung-drv.") == token) {
				written = snprintf(dest + offset, remaining, "mcd-panel-samsung-drv-usdm%s ", token + strlen("mcd-panel-samsung-drv"));
			} else {
				written = snprintf(dest + offset, remaining, "%s ", token);
			}
		}

		if (written < 0) {
			break;
		} else if (written >= remaining) {
			offset = dest_size - 1;
			break;
		} else {
			offset += written;
		}
	}

	if (offset > 0 && dest[offset - 1] == ' ') {
		dest[offset - 1] = '\0';
	} else {
		dest[offset] = '\0';
	}

	kfree(buf);
}
#endif

static int cmdline_proc_show(struct seq_file *m, void *v)
{
#ifdef CONFIG_SEC_DETECT_CMDLINE_PATCH
	char *patched_cmdline;

	patched_cmdline = kmalloc(BUF_SIZE, GFP_KERNEL);
	if (!patched_cmdline) {
		seq_puts(m, saved_command_line);
		goto out;
	}

	cmdline_panel_patch(patched_cmdline, BUF_SIZE, saved_command_line);
	seq_puts(m, patched_cmdline);
	kfree(patched_cmdline);
#else
	seq_puts(m, saved_command_line);
#endif

out:
	seq_putc(m, '\n');
	return 0;
}

static int __init proc_cmdline_init(void)
{
	proc_create_single("cmdline", 0, NULL, cmdline_proc_show);
	return 0;
}
fs_initcall(proc_cmdline_init);
