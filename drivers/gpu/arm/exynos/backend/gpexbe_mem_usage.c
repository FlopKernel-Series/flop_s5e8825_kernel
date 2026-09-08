/* SPDX-License-Identifier: GPL-2.0 */

/*
 * (C) COPYRIGHT 2021 Samsung Electronics Inc. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 */

/* Implements */
#include <linux/export.h>
#include <gpexbe_mem_usage.h>
#include <trace/hooks/mm.h>
/* Uses */
#include <gpex_utils.h>
#include <linux/oom.h>
#include <linux/seq_file.h>

static const struct gpex_mem_usage_ops *mem_usage_ops;

void gpexbe_mem_usage_set_ops(const struct gpex_mem_usage_ops *ops)
{
	mem_usage_ops = ops;
}
EXPORT_SYMBOL_GPL(gpexbe_mem_usage_set_ops);

static ssize_t show_kernel_sysfs_gpu_memory(char *buf)
{
	if (!buf)
		return 0;

	if (mem_usage_ops && mem_usage_ops->show_gpu_memory)
		return mem_usage_ops->show_gpu_memory(buf, PAGE_SIZE);

	return scnprintf(buf, PAGE_SIZE, "%9s %9s %12s\n", "tgid", "pid", "bytes_used");
}
CREATE_SYSFS_KOBJECT_READ_FUNCTION(show_kernel_sysfs_gpu_memory);

static int gpu_memory_status_dump(bool print_all_buffers)
{
	if (mem_usage_ops && mem_usage_ops->get_total_used_pages)
		return mem_usage_ops->get_total_used_pages(print_all_buffers);

	return 0;
}

static int mali_used_size_notifier(struct notifier_block *nb,
		unsigned long action, void *data)
{
	struct seq_file *s;
	int used_pages = gpu_memory_status_dump(false);

	if (used_pages < 0)
		return 0;

	used_pages = (used_pages << (PAGE_SHIFT - 10));

	s = (struct seq_file *)data;
	if (s != NULL)
		seq_printf(s, "mali:           %8lu kB\n", used_pages);
	else
		pr_cont("mali:%lukB ", used_pages);

	return 0;
}

static struct notifier_block mali_used_size_nb = {
	.notifier_call = mali_used_size_notifier,
};

static int mali_used_buffer_oom_notifier(struct notifier_block *nb,
		unsigned long action, void *data)
{
	gpu_memory_status_dump(true);

	return 0;
}

static struct notifier_block mali_used_buffer_oom_nb = {
	.notifier_call = mali_used_buffer_oom_notifier,
};

static void show_mali_used_size_mem(void *data, unsigned int filter,
		nodemask_t *nodemask)
{
	struct seq_file *s;
	int used_pages = gpu_memory_status_dump(false);

	if (used_pages < 0)
		return;

	used_pages = (used_pages << (PAGE_SHIFT - 10));

	s = (struct seq_file *)data;
	if (s != NULL)
		seq_printf(s, "GpuTotal:           %8lu kB\n", used_pages);
	else
		pr_cont("GpuTotal:%lukB ", used_pages);
}

static void show_mali_used_size_proc_meminfo(void *data, struct seq_file *m)
{
	int used_pages = gpu_memory_status_dump(false);

	if (used_pages < 0)
		return;

	used_pages = (used_pages << (PAGE_SHIFT - 10));

	if (m != NULL)
		seq_printf(m, "GpuTotal:           %8lu kB\n", used_pages);
	else
		pr_cont("GpuTotal:%lukB ", used_pages);
}

static void register_mali_used_mem_notifier(void) {
#ifdef CONFIG_MALI_EXYNOS_GPU_MEM_DUMP

	/*This is used to print mali used size when do: echo m > /proc/sysrq-trigger; dmesg*/
	register_trace_android_vh_show_mem(show_mali_used_size_mem,NULL);

	/*This is used to print mali used size when do: cat /proc/meminfo */
	register_trace_android_vh_meminfo_proc_show(show_mali_used_size_proc_meminfo,NULL);

	CSTD_UNUSED(mali_used_size_nb);
	CSTD_UNUSED(mali_used_buffer_oom_nb);
#else
	CSTD_UNUSED(show_mali_used_size_mem);
	CSTD_UNUSED(show_mali_used_size_proc_meminfo);
	CSTD_UNUSED(mali_used_size_nb);
	CSTD_UNUSED(mali_used_buffer_oom_nb);
#endif
}

int gpexbe_mem_usage_init(void)
{
	GPEX_UTILS_SYSFS_KOBJECT_FILE_ADD_RO(gpu_memory, show_kernel_sysfs_gpu_memory);

	register_mali_used_mem_notifier();

	return 0;
}

void gpexbe_mem_usage_term(void)
{
	mem_usage_ops = NULL;
}
