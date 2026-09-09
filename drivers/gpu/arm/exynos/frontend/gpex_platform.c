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

#include <linux/module.h>
#include <soc/samsung/exynos_gpex.h>
#include <gpex_platform.h>
#include <gpex_utils.h>
#include <gpex_debug.h>
#include <gpex_pm.h>
#include <gpex_dvfs.h>
#include <gpex_qos.h>
#include <gpex_thermal.h>
#include <gpex_clock.h>
#include <gpex_ifpo.h>
#include <gpex_tsg.h>
#include <gpex_clboost.h>
#include <gpex_cmar_sched.h>
#include <gpexbe_devicetree.h>

#include <gpexbe_notifier.h>
#include <gpexbe_pm.h>
#include <gpexbe_clock.h>
#include <gpexbe_qos.h>
#include <gpexbe_bts.h>
#include <gpexbe_debug.h>
#include <gpexbe_utilization.h>
#include <gpexbe_llc_coherency.h>
#include <gpexbe_mem_usage.h>
#include <gpexbe_smc.h>
#include <gpex_gts.h>
#include <gpexwa_interactive_boost.h>

#include <runtime_test_runner.h>

int gpex_platform_init(struct device **dev)
{
	/* TODO: check return value */
	/* TODO: becareful with order */
	gpexbe_devicetree_init(*dev);
	gpex_utils_init(dev);
	gpex_debug_init(dev);

	gpexbe_utilization_init(dev);
	gpex_clboost_init();

	gpex_gts_init(dev);

	gpexbe_debug_init();

	gpex_thermal_init();
	gpexbe_notifier_init();

	gpexbe_llc_coherency_init(dev);

	gpexbe_pm_init();
	gpexbe_clock_init();
	gpex_pm_init();
	gpex_clock_init(dev);

	gpexbe_qos_init();
	gpexbe_bts_init();
	gpex_qos_init();

	gpex_ifpo_init();
	gpex_dvfs_init(dev);
	gpexbe_smc_init();
	gpex_cmar_sched_init();
	gpex_tsg_init(dev);

	gpexbe_mem_usage_init();

	gpexwa_interactive_boost_init();

	runtime_test_runner_init();

	gpex_utils_sysfs_kobject_files_create();
	gpex_utils_sysfs_device_files_create();


	return 0;
}
EXPORT_SYMBOL_GPL(gpex_platform_init);

void gpex_platform_term(void)
{
	runtime_test_runner_term();

	gpexbe_mem_usage_term();

	gpexwa_interactive_boost_term();

	gpex_tsg_term();
	gpex_cmar_sched_term();
	gpexbe_smc_term();
	gpex_ifpo_term();

	gpex_pm_term();
	gpexbe_pm_term();

	gpex_qos_term();
	gpexbe_qos_term();
	gpexbe_bts_term();

	/* DVFS stuff */
	gpex_dvfs_term();

	gpex_clock_term();
	gpexbe_clock_term();

	gpexbe_llc_coherency_term();

	gpexbe_notifier_term();
	gpex_thermal_term();

	gpexbe_debug_term();
	gpex_gts_term();

	gpex_clboost_term();
	gpexbe_utilization_term();
	gpex_utils_term();
}
EXPORT_SYMBOL_GPL(gpex_platform_term);

/*
 * Public GPEX registration and control interface for GPU drivers
 */
static const struct exynos_gpex_gpu_ops *active_gpu_ops;
static struct device *active_gpu_dev;

int exynos_gpex_register_gpu(struct device *dev, const struct exynos_gpex_gpu_ops *ops)
{
	int ret;

	active_gpu_dev = dev;
	active_gpu_ops = ops;

	ret = gpex_platform_init(&dev);
	if (ret) {
		active_gpu_dev = NULL;
		active_gpu_ops = NULL;
		return ret;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(exynos_gpex_register_gpu);

void exynos_gpex_unregister_gpu(struct device *dev)
{
	gpex_platform_term();
	active_gpu_dev = NULL;
	active_gpu_ops = NULL;
}
EXPORT_SYMBOL_GPL(exynos_gpex_unregister_gpu);

struct device *exynos_gpex_get_gpu_device(void)
{
	return active_gpu_dev;
}
EXPORT_SYMBOL_GPL(exynos_gpex_get_gpu_device);

bool exynos_gpex_is_attached(void)
{
	return (active_gpu_dev != NULL);
}
EXPORT_SYMBOL_GPL(exynos_gpex_is_attached);

const struct exynos_gpex_gpu_ops *exynos_gpex_get_gpu_ops(void)
{
	return active_gpu_ops;
}
EXPORT_SYMBOL_GPL(exynos_gpex_get_gpu_ops);

int exynos_gpex_set_frequency(unsigned long freq_hz)
{
	if (freq_hz > 10000000)
		freq_hz /= 1000;
	return gpex_clock_set((int)freq_hz);
}
EXPORT_SYMBOL_GPL(exynos_gpex_set_frequency);

unsigned long exynos_gpex_get_frequency(void)
{
	return (unsigned long)gpex_clock_get_cur_clock();
}
EXPORT_SYMBOL_GPL(exynos_gpex_get_frequency);

int exynos_gpex_pm_resume(struct device *dev)
{
	return gpex_pm_runtime_on_prepare(dev);
}
EXPORT_SYMBOL_GPL(exynos_gpex_pm_resume);

int exynos_gpex_pm_suspend(struct device *dev)
{
	gpex_pm_runtime_off_prepare(dev);
	return 0;
}
EXPORT_SYMBOL_GPL(exynos_gpex_pm_suspend);

void exynos_gpex_setup_coherency(void)
{
	gpexbe_llc_coherency_set_coherency_feature();
	gpexbe_llc_coherency_set_aruser();
	gpexbe_llc_coherency_set_awuser();
}
EXPORT_SYMBOL_GPL(exynos_gpex_setup_coherency);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Samsung Exynos GPU Platform Extension (GPEX)");
MODULE_AUTHOR("Samsung Electronics Co., Ltd.");
MODULE_SOFTDEP("pre: exynos-acme");
