// SPDX-License-Identifier: GPL-2.0
/*
 * Mali KMD external symbol redirector
 *
 * Exports symbols that external kernel modules resolve against,
 * forwarding calls to whichever Mali KMD module is active at runtime.
 * The active Mali module registers its exports struct during probe.
 */
#include <linux/module.h>
#include <linux/ktime.h>
#include <linux/notifier.h>
#include <linux/types.h>
#include "mali_redirect.h"

static const struct mali_exports *mali_exp;

void mali_register_exports(const struct mali_exports *exp)
{
	mali_exp = exp;
}
EXPORT_SYMBOL(mali_register_exports);

/*
 * gpu_dvfs API
 */
int gpu_dvfs_get_cur_clock(void)
{
	if (mali_exp && mali_exp->gpu_dvfs_get_cur_clock)
		return mali_exp->gpu_dvfs_get_cur_clock();
	return 0;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_cur_clock);

int gpu_dvfs_get_clock(int level)
{
	if (mali_exp && mali_exp->gpu_dvfs_get_clock)
		return mali_exp->gpu_dvfs_get_clock(level);
	return -1;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_clock);

int gpu_dvfs_get_voltage(int clock)
{
	if (mali_exp && mali_exp->gpu_dvfs_get_voltage)
		return mali_exp->gpu_dvfs_get_voltage(clock);
	return 0;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_voltage);

int gpu_dvfs_get_step(void)
{
	if (mali_exp && mali_exp->gpu_dvfs_get_step)
		return mali_exp->gpu_dvfs_get_step();
	return 0;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_step);

int gpu_dvfs_get_utilization(void)
{
	if (mali_exp && mali_exp->gpu_dvfs_get_utilization)
		return mali_exp->gpu_dvfs_get_utilization();
	return 0;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_utilization);

int gpu_dvfs_get_max_freq(void)
{
	if (mali_exp && mali_exp->gpu_dvfs_get_max_freq)
		return mali_exp->gpu_dvfs_get_max_freq();
	return 0;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_max_freq);

int gpu_dvfs_get_min_freq(void)
{
	if (mali_exp && mali_exp->gpu_dvfs_get_min_freq)
		return mali_exp->gpu_dvfs_get_min_freq();
	return 0;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_min_freq);

int gpu_dvfs_get_max_locked_freq(void)
{
	if (mali_exp && mali_exp->gpu_dvfs_get_max_locked_freq)
		return mali_exp->gpu_dvfs_get_max_locked_freq();
	return 0;
}
EXPORT_SYMBOL(gpu_dvfs_get_max_locked_freq);

int gpu_dvfs_get_min_locked_freq(void)
{
	if (mali_exp && mali_exp->gpu_dvfs_get_min_locked_freq)
		return mali_exp->gpu_dvfs_get_min_locked_freq();
	return 0;
}
EXPORT_SYMBOL(gpu_dvfs_get_min_locked_freq);

int *gpu_dvfs_get_freq_table(void)
{
	if (mali_exp && mali_exp->gpu_dvfs_get_freq_table)
		return mali_exp->gpu_dvfs_get_freq_table();
	return NULL;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_freq_table);

ktime_t *gpu_dvfs_get_time_in_state(void)
{
	if (mali_exp && mali_exp->gpu_dvfs_get_time_in_state)
		return mali_exp->gpu_dvfs_get_time_in_state();
	return NULL;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_time_in_state);

ktime_t gpu_dvfs_get_tis_last_update(void)
{
	if (mali_exp && mali_exp->gpu_dvfs_get_tis_last_update)
		return mali_exp->gpu_dvfs_get_tis_last_update();
	return 0;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_tis_last_update);

ktime_t *gpu_dvfs_get_job_queue_count(void)
{
	if (mali_exp && mali_exp->gpu_dvfs_get_job_queue_count)
		return mali_exp->gpu_dvfs_get_job_queue_count();
	return NULL;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_job_queue_count);

ktime_t gpu_dvfs_get_job_queue_last_updated(void)
{
	if (mali_exp && mali_exp->gpu_dvfs_get_job_queue_last_updated)
		return mali_exp->gpu_dvfs_get_job_queue_last_updated();
	return 0;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_get_job_queue_last_updated);

void gpu_dvfs_set_amigo_governor(int mode)
{
	if (mali_exp && mali_exp->gpu_dvfs_set_amigo_governor)
		mali_exp->gpu_dvfs_set_amigo_governor(mode);
}
EXPORT_SYMBOL_GPL(gpu_dvfs_set_amigo_governor);

void gpu_dvfs_set_freq_margin(int freq_margin)
{
	if (mali_exp && mali_exp->gpu_dvfs_set_freq_margin)
		mali_exp->gpu_dvfs_set_freq_margin(freq_margin);
}
EXPORT_SYMBOL_GPL(gpu_dvfs_set_freq_margin);

/*
 * GPU thermal/tmu API
 */
void gpu_tmu_get_notifier(struct notifier_block **nb)
{
	if (mali_exp && mali_exp->gpu_tmu_get_notifier)
		mali_exp->gpu_tmu_get_notifier(nb);
}
EXPORT_SYMBOL_GPL(gpu_tmu_get_notifier);

int gpu_tmu_notifier(struct notifier_block *notifier, unsigned long event, void *v)
{
	if (mali_exp && mali_exp->gpu_tmu_notifier)
		return mali_exp->gpu_tmu_notifier(notifier, event, v);
	return 0;
}
EXPORT_SYMBOL_GPL(gpu_tmu_notifier);

/*
 * Utilization notifier
 */
int gpu_dvfs_register_utilization_notifier(struct notifier_block *nb)
{
	if (mali_exp && mali_exp->gpu_dvfs_register_utilization_notifier)
		return mali_exp->gpu_dvfs_register_utilization_notifier(nb);
	return 0;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_register_utilization_notifier);

int gpu_dvfs_unregister_utilization_notifier(struct notifier_block *nb)
{
	if (mali_exp && mali_exp->gpu_dvfs_unregister_utilization_notifier)
		return mali_exp->gpu_dvfs_unregister_utilization_notifier(nb);
	return 0;
}
EXPORT_SYMBOL_GPL(gpu_dvfs_unregister_utilization_notifier);

/*
 * TSG / GPU stats API
 */
unsigned long exynos_stats_get_job_state_cnt(void)
{
	if (mali_exp && mali_exp->exynos_stats_get_job_state_cnt)
		return mali_exp->exynos_stats_get_job_state_cnt();
	return 0;
}
EXPORT_SYMBOL_GPL(exynos_stats_get_job_state_cnt);

void exynos_stats_get_run_times(u64 *times)
{
	if (mali_exp && mali_exp->exynos_stats_get_run_times)
		mali_exp->exynos_stats_get_run_times(times);
}
EXPORT_SYMBOL_GPL(exynos_stats_get_run_times);

void exynos_stats_set_vsync(ktime_t timestamp)
{
	if (mali_exp && mali_exp->exynos_stats_set_vsync)
		mali_exp->exynos_stats_set_vsync(timestamp);
}
EXPORT_SYMBOL_GPL(exynos_stats_set_vsync);

void exynos_stats_get_frame_info(s32 *nrframe, u64 *nrvsync, u64 *delta_ms)
{
	if (mali_exp && mali_exp->exynos_stats_get_frame_info)
		mali_exp->exynos_stats_get_frame_info(nrframe, nrvsync, delta_ms);
}
EXPORT_SYMBOL_GPL(exynos_stats_get_frame_info);

/*
 * Migov helpers
 */
void exynos_migov_set_targetframetime(int us)
{
	if (mali_exp && mali_exp->exynos_migov_set_targetframetime)
		mali_exp->exynos_migov_set_targetframetime(us);
}
EXPORT_SYMBOL_GPL(exynos_migov_set_targetframetime);

void exynos_migov_set_targettime_margin(int us)
{
	if (mali_exp && mali_exp->exynos_migov_set_targettime_margin)
		mali_exp->exynos_migov_set_targettime_margin(us);
}
EXPORT_SYMBOL_GPL(exynos_migov_set_targettime_margin);

void exynos_migov_set_util_margin(int percentage)
{
	if (mali_exp && mali_exp->exynos_migov_set_util_margin)
		mali_exp->exynos_migov_set_util_margin(percentage);
}
EXPORT_SYMBOL_GPL(exynos_migov_set_util_margin);

void exynos_migov_set_decon_time(int us)
{
	if (mali_exp && mali_exp->exynos_migov_set_decon_time)
		mali_exp->exynos_migov_set_decon_time(us);
}
EXPORT_SYMBOL_GPL(exynos_migov_set_decon_time);

void exynos_migov_set_comb_ctrl(int enable)
{
	if (mali_exp && mali_exp->exynos_migov_set_comb_ctrl)
		mali_exp->exynos_migov_set_comb_ctrl(enable);
}
EXPORT_SYMBOL_GPL(exynos_migov_set_comb_ctrl);

/*
 * SDP
 */
void exynos_sdp_set_powertable(int id, int cnt, struct freq_table *table)
{
	if (mali_exp && mali_exp->exynos_sdp_set_powertable)
		mali_exp->exynos_sdp_set_powertable(id, cnt, table);
}
EXPORT_SYMBOL_GPL(exynos_sdp_set_powertable);

void exynos_sdp_set_busy_domain(int id)
{
	if (mali_exp && mali_exp->exynos_sdp_set_busy_domain)
		mali_exp->exynos_sdp_set_busy_domain(id);
}
EXPORT_SYMBOL_GPL(exynos_sdp_set_busy_domain);

void exynos_sdp_set_cur_freqlv(int id, int idx)
{
	if (mali_exp && mali_exp->exynos_sdp_set_cur_freqlv)
		mali_exp->exynos_sdp_set_cur_freqlv(id, idx);
}
EXPORT_SYMBOL_GPL(exynos_sdp_set_cur_freqlv);

/*
 * STC config
 */
int exynos_gpu_stc_config_show(int page_size, char *buf)
{
	if (mali_exp && mali_exp->exynos_gpu_stc_config_show)
		return mali_exp->exynos_gpu_stc_config_show(page_size, buf);
	return 0;
}
EXPORT_SYMBOL_GPL(exynos_gpu_stc_config_show);

int exynos_gpu_stc_config_store(const char *buf)
{
	if (mali_exp && mali_exp->exynos_gpu_stc_config_store)
		return mali_exp->exynos_gpu_stc_config_store(buf);
	return 0;
}
EXPORT_SYMBOL_GPL(exynos_gpu_stc_config_store);

/*
 * kbase internal exports
 */
void kbase_get_create_info(u64 *cnt, ktime_t *us)
{
	if (mali_exp && mali_exp->kbase_get_create_info)
		mali_exp->kbase_get_create_info(cnt, us);
}
EXPORT_SYMBOL(kbase_get_create_info);
