// SPDX-License-Identifier: GPL-2.0
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/kernel.h>

#ifdef CONFIG_SEC_DETECT_CMDLINE_PATCH
#include <linux/string.h>
#include <linux/sec_detect.h>

#define PATCHED_CMDLINE_BUF_SIZE (4096 + 256)

static int patch_dynamic_cmdline(const char *original_cmdline, char *patched_buf, size_t patched_buf_size)
{
	char *temp_src_buf;
	char *pos;
	char *token;
	const char *delim = " ";
	int offset = 0;
	int ret = 0;
	size_t original_cmdline_len;

	if (patched_buf_size == 0) {
		return -EINVAL;
	}
	patched_buf[0] = '\0';

	original_cmdline_len = strlen(original_cmdline);
	temp_src_buf = kstrndup(original_cmdline, original_cmdline_len, GFP_KERNEL);
	if (!temp_src_buf) {
		return -ENOMEM;
	}
	pos = temp_src_buf;

	while ((token = strsep(&pos, delim)) != NULL) {
		int chars_written_by_snprintf;

		if (offset >= patched_buf_size - 1) {
			pr_warn_once("cmdline_patch: patched_buf full before processing token '%s'.\n", token);
			ret = -ENOSPC;
			break;
		}

		if (!sec_is_detection_complete()) {
			chars_written_by_snprintf = snprintf(patched_buf + offset, patched_buf_size - offset, "%s ", token);
		} else if (sec_feat_needs_decon()) {
			if (strstr(token, "mcd-panel.") == token) {
				chars_written_by_snprintf = snprintf(patched_buf + offset, patched_buf_size - offset, "mcd-panel-decon%s ", token + strlen("mcd-panel"));
			} else if (strstr(token, "exynos-drm.") == token) {
				chars_written_by_snprintf = snprintf(patched_buf + offset, patched_buf_size - offset, "exynos-drm-decon%s ", token + strlen("exynos-drm"));
			} else if (strstr(token, "mcd-panel-samsung-drv.") == token) {
				chars_written_by_snprintf = snprintf(patched_buf + offset, patched_buf_size - offset, "mcd-panel-samsung-drv-decon%s ", token + strlen("mcd-panel-samsung-drv"));
			} else {
				chars_written_by_snprintf = snprintf(patched_buf + offset, patched_buf_size - offset, "%s ", token);
			}
		} else {
			if (strstr(token, "mcd-panel-samsung-drv.") == token) {
				chars_written_by_snprintf = snprintf(patched_buf + offset, patched_buf_size - offset, "mcd-panel-samsung-drv-usdm%s ", token + strlen("mcd-panel-samsung-drv"));
			} else {
				chars_written_by_snprintf = snprintf(patched_buf + offset, patched_buf_size - offset, "%s ", token);
			}
		}

		if (chars_written_by_snprintf < 0) {
			pr_err_once("cmdline_patch: snprintf error %d while processing token '%s'\n", chars_written_by_snprintf, token);
			ret = -EINVAL;
			break;
		}

		if (chars_written_by_snprintf >= (patched_buf_size - offset) ) {
			pr_warn_once("cmdline_patch: cmdline truncated during write of token (or its modification).\n");
			offset = patched_buf_size - 1;
			ret = -ENOSPC;
			break;
		}
		offset += chars_written_by_snprintf;
	}

	kfree(temp_src_buf);

	if (ret != -ENOSPC && offset > 0 && patched_buf[offset - 1] == ' ') {
		patched_buf[offset - 1] = '\0';
	}

	return ret;
}
#endif

static int cmdline_proc_show(struct seq_file *m, void *v)
{
#ifdef CONFIG_SEC_DETECT_CMDLINE_PATCH
	char *display_cmdline;
	int patch_ret;

	display_cmdline = kzalloc(PATCHED_CMDLINE_BUF_SIZE, GFP_KERNEL); // +128 for some wiggle room
	if (!display_cmdline) {
		// Fallback to original if allocation fails
		seq_puts(m, saved_command_line);
		seq_putc(m, '\n');
		return 0;
	}

	patch_ret = patch_dynamic_cmdline(saved_command_line, display_cmdline, PATCHED_CMDLINE_BUF_SIZE);

	seq_puts(m, display_cmdline);
	kfree(display_cmdline);
#else
	seq_puts(m, saved_command_line);
#endif
	seq_putc(m, '\n');
	return 0;
}

static int __init proc_cmdline_init(void)
{
	proc_create_single("cmdline", 0, NULL, cmdline_proc_show);
	return 0;
}
fs_initcall(proc_cmdline_init);
