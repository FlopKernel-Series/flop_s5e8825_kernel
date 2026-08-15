/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Mali KMD external symbol redirector
 *
 * Exports generic names that external kernel consumers (exynos-migov,
 * exynos-gpu-profiler, gpu_cooling) resolve against. The active Mali
 * version module registers its function table via mali_register_exports()
 * during probe; calls are forwarded to the versioned implementation
 * at runtime through the function pointer table.
 */
#ifndef _MALI_REDIRECT_H_
#define _MALI_REDIRECT_H_

#include <linux/types.h>
#include <linux/ktime.h>

/* Forward declarations for types used in the exports struct */
struct freq_table;

/*
 * Mali export function table filled in at registration time.
 */
struct mali_exports {
	/* gpu_dvfs API */
	int (*gpu_dvfs_get_cur_clock)(void);
	int (*gpu_dvfs_get_clock)(int level);
	int (*gpu_dvfs_get_voltage)(int clock);
	int (*gpu_dvfs_get_step)(void);
	int (*gpu_dvfs_get_utilization)(void);
	int (*gpu_dvfs_get_max_freq)(void);
	int (*gpu_dvfs_get_min_freq)(void);
	int (*gpu_dvfs_get_max_locked_freq)(void);
	int (*gpu_dvfs_get_min_locked_freq)(void);
	uint32_t *(*gpu_dvfs_get_freq_table)(void);
	ktime_t *(*gpu_dvfs_get_time_in_state)(void);
	ktime_t (*gpu_dvfs_get_tis_last_update)(void);
	ktime_t *(*gpu_dvfs_get_job_queue_count)(void);
	ktime_t (*gpu_dvfs_get_job_queue_last_updated)(void);
	void (*gpu_dvfs_set_amigo_governor)(int mode);
	void (*gpu_dvfs_set_freq_margin)(int freq_margin);
	int (*gpu_dvfs_register_utilization_notifier)(struct notifier_block *nb);
	int (*gpu_dvfs_unregister_utilization_notifier)(struct notifier_block *nb);

	/* GPU thermal/tmu */
	void (*gpu_tmu_get_notifier)(struct notifier_block **nb);
	int (*gpu_tmu_notifier)(struct notifier_block *notifier,
				unsigned long event, void *v);

	/* TSG / GPU stats API */
	unsigned long (*exynos_stats_get_job_state_cnt)(void);
	void (*exynos_stats_get_run_times)(u64 *times);
	void (*exynos_stats_set_vsync)(ktime_t timestamp);
	void (*exynos_stats_get_frame_info)(s32 *nrframe, u64 *nrvsync, u64 *delta_ms);

	/* Migov helpers */
	void (*exynos_migov_set_targetframetime)(int us);
	void (*exynos_migov_set_targettime_margin)(int us);
	void (*exynos_migov_set_util_margin)(int percentage);
	void (*exynos_migov_set_decon_time)(int us);
	void (*exynos_migov_set_comb_ctrl)(int enable);

	/* SDP */
	void (*exynos_sdp_set_powertable)(int id, int cnt, struct freq_table *table);
	void (*exynos_sdp_set_busy_domain)(int id);
	void (*exynos_sdp_set_cur_freqlv)(int id, int idx);

	/* STC config */
	int (*exynos_gpu_stc_config_show)(int page_size, char *buf);
	int (*exynos_gpu_stc_config_store)(const char *buf);

	/* kbase internal */
	void (*kbase_get_create_info)(u64 *cnt, ktime_t *us);
};

/*
 * Register the active Mali module's export table.
 * Called by the Mali module after probe initialization succeeds.
 * Passing NULL unregisters.
 */
void mali_register_exports(const struct mali_exports *exp);

#endif /* _MALI_REDIRECT_H_ */
