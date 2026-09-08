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

#include <linux/export.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/atomic.h>
#include <linux/ktime.h>
#include <soc/samsung/exynos_gpex.h>
#include <gpexbe_utilization.h>
#include <gpex_gts.h>

/* GPU metrics time shift (256ns as a unit) */
#define KBASE_PM_TIME_SHIFT 8

struct _util_info {
	struct device *dev;
	spinlock_t lock;
	ktime_t time_period_start;
	bool gpu_active;
	u64 time_busy;
	u64 time_idle;
	int pure_compute_time_rate;
	atomic_t time_compute_jobs;
	atomic_t time_vertex_jobs;
	atomic_t time_fragment_jobs;
	atomic_t cnt_compute_jobs;
	atomic_t cnt_fragment_jobs;
	atomic_t cnt_vertex_jobs;
	int cur_utilization;
};

static struct _util_info util_info;

/* External hook provided by gpex_platform when GPU driver registers */
extern const struct exynos_gpex_gpu_ops *exynos_gpex_get_gpu_ops(void);

static inline void atomic_add_shifted(u64 val, atomic_t *res)
{
	atomic_add(val >> KBASE_PM_TIME_SHIFT, res);
}

static inline void update_compute_job_load(u64 ns_elapsed)
{
	atomic_add_shifted(ns_elapsed, &util_info.time_compute_jobs);
}

static inline void update_fragment_job_load(u64 ns_elapsed)
{
	atomic_add_shifted(ns_elapsed, &util_info.time_fragment_jobs);
}

static inline void update_vertex_job_load(u64 ns_elapsed)
{
	atomic_add_shifted(ns_elapsed, &util_info.time_vertex_jobs);
}

static inline void increment_compute_job_cnt(void)
{
	atomic_inc(&util_info.cnt_compute_jobs);
}

static inline void increment_fragment_job_cnt(void)
{
	atomic_inc(&util_info.cnt_fragment_jobs);
}

static inline void increment_vertex_job_cnt(void)
{
	atomic_inc(&util_info.cnt_vertex_jobs);
}

void gpexbe_utilization_set_gpu_active(bool active)
{
	unsigned long flags;
	ktime_t now = ktime_get();
	ktime_t diff;
	u32 ns_time;

	spin_lock_irqsave(&util_info.lock, flags);
	diff = ktime_sub(now, util_info.time_period_start);
	ns_time = (u32)(ktime_to_ns(diff) >> KBASE_PM_TIME_SHIFT);

	if (util_info.gpu_active)
		util_info.time_busy += ns_time;
	else
		util_info.time_idle += ns_time;

	util_info.time_period_start = now;
	util_info.gpu_active = active;
	spin_unlock_irqrestore(&util_info.lock, flags);

	gpex_gts_set_jobslot_status(active);
}
EXPORT_SYMBOL_GPL(gpexbe_utilization_set_gpu_active);

void gpexbe_utilization_update_job_load(enum gpex_job_type type, u64 ns_spent)
{
	switch (type) {
	case GPEX_JOB_TYPE_COMPUTE:
		update_compute_job_load(ns_spent);
		increment_compute_job_cnt();
		break;
	case GPEX_JOB_TYPE_FRAGMENT:
		update_fragment_job_load(ns_spent);
		increment_fragment_job_cnt();
		break;
	case GPEX_JOB_TYPE_VERTEX:
		update_vertex_job_load(ns_spent);
		increment_vertex_job_cnt();
		break;
	default:
		break;
	}
}
EXPORT_SYMBOL_GPL(gpexbe_utilization_update_job_load);

int gpexbe_utilization_get_compute_job_time(void)
{
	return atomic_read(&util_info.time_compute_jobs);
}

int gpexbe_utilization_get_vertex_job_time(void)
{
	return atomic_read(&util_info.time_vertex_jobs);
}

int gpexbe_utilization_get_fragment_job_time(void)
{
	return atomic_read(&util_info.time_fragment_jobs);
}

int gpexbe_utilization_get_compute_job_cnt(void)
{
	int ret = atomic_read(&util_info.cnt_compute_jobs);

	atomic_set(&util_info.cnt_compute_jobs, 0);

	return ret;
}

int gpexbe_utilization_get_vertex_job_cnt(void)
{
	int ret = atomic_read(&util_info.cnt_vertex_jobs);

	atomic_set(&util_info.cnt_vertex_jobs, 0);

	return ret;
}

int gpexbe_utilization_get_fragment_job_cnt(void)
{
	int ret = atomic_read(&util_info.cnt_fragment_jobs);

	atomic_set(&util_info.cnt_fragment_jobs, 0);

	return ret;
}

int gpexbe_utilization_get_utilization(void)
{
	return util_info.cur_utilization;
}

int gpexbe_utilization_get_pure_compute_time_rate(void)
{
	return util_info.pure_compute_time_rate;
}

void gpexbe_utilization_calculate_compute_ratio(void)
{
	int compute_time = atomic_read(&util_info.time_compute_jobs);
	int vertex_time = atomic_read(&util_info.time_vertex_jobs);
	int fragment_time = atomic_read(&util_info.time_fragment_jobs);
	int total_time = compute_time + vertex_time + fragment_time;

	if (compute_time > 0 && total_time > 0)
		util_info.pure_compute_time_rate = (100 * compute_time) / total_time;
	else
		util_info.pure_compute_time_rate = 0;

	atomic_set(&util_info.time_compute_jobs, 0);
	atomic_set(&util_info.time_vertex_jobs, 0);
	atomic_set(&util_info.time_fragment_jobs, 0);
}

int gpexbe_utilization_calc_utilization(void)
{
	unsigned long flags;
	int utilisation = 0;
	ktime_t now = ktime_get();
	ktime_t diff;
	u32 ns_time;
	u64 total_time;
	const struct exynos_gpex_gpu_ops *ops = exynos_gpex_get_gpu_ops();

	/* Check if the active GPU driver provides its own utilization query */
	if (ops && ops->get_utilization) {
		utilisation = ops->get_utilization();
		if (utilisation >= 0) {
			util_info.cur_utilization = utilisation;
			gpex_gts_update_gpu_data();
			gpex_gts_clear();
			return utilisation;
		}
	}

	spin_lock_irqsave(&util_info.lock, flags);
	diff = ktime_sub(now, util_info.time_period_start);
	ns_time = (u32)(ktime_to_ns(diff) >> KBASE_PM_TIME_SHIFT);

	if (util_info.gpu_active)
		util_info.time_busy += ns_time;
	else
		util_info.time_idle += ns_time;

	util_info.time_period_start = now;
	gpex_gts_update_jobslot_util(util_info.gpu_active, ns_time);

	total_time = util_info.time_idle + util_info.time_busy;
	if (total_time == 0) {
		utilisation = -1;
	} else {
		utilisation = (int)((100 * util_info.time_busy) / total_time);
		gpex_gts_update_gpu_data();
	}

	util_info.time_idle = 0;
	util_info.time_busy = 0;
	gpex_gts_clear();
	util_info.cur_utilization = utilisation;
	spin_unlock_irqrestore(&util_info.lock, flags);

	return utilisation;
}

int gpexbe_utilization_init(struct device **dev)
{
	util_info.dev = dev ? *dev : NULL;
	spin_lock_init(&util_info.lock);
	util_info.time_period_start = ktime_get();
	util_info.gpu_active = false;
	util_info.time_busy = 0;
	util_info.time_idle = 0;

	atomic_set(&util_info.time_compute_jobs, 0);
	atomic_set(&util_info.time_vertex_jobs, 0);
	atomic_set(&util_info.time_fragment_jobs, 0);
	atomic_set(&util_info.cnt_compute_jobs, 0);
	atomic_set(&util_info.cnt_fragment_jobs, 0);
	atomic_set(&util_info.cnt_vertex_jobs, 0);

	util_info.pure_compute_time_rate = 0;
	util_info.cur_utilization = 0;

	return 0;
}

void gpexbe_utilization_term(void)
{
	util_info.dev = NULL;

	atomic_set(&util_info.time_compute_jobs, 0);
	atomic_set(&util_info.time_vertex_jobs, 0);
	atomic_set(&util_info.time_fragment_jobs, 0);
	atomic_set(&util_info.cnt_compute_jobs, 0);
	atomic_set(&util_info.cnt_fragment_jobs, 0);
	atomic_set(&util_info.cnt_vertex_jobs, 0);

	util_info.pure_compute_time_rate = 0;
	util_info.cur_utilization = 0;
}
