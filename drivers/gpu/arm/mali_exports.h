/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Namespace header for Mali KMD versioned exports.
 *
 * Included via Kbuild -include to prevent symbol conflicts between
 * multiple Mali versions. Each version defines MALI_VER_SUFFIX which
 * is appended to all externally-visible symbols.
 *
 * Usage in Kbuild:
 *   ccflags-y += -DMALI_VER_SUFFIX=_r38p1
 *   ccflags-y += -include $(srctree)/drivers/gpu/arm/mali_exports.h
 *
 * The redirector (mali_redirect.c) exports the generic names and
 * routes calls to the version selected by mali.version= cmdline.
 */

#ifndef _MALI_EXPORTS_H_
#define _MALI_EXPORTS_H_

#ifndef MALI_VER_SUFFIX
#error "MALI_VER_SUFFIX must be defined (e.g., _r44p1)"
#endif

/* Concatenation helpers */
#define __MALI_CONCAT2(a, b) a ## b
#define __MALI_CONCAT(a, b) __MALI_CONCAT2(a, b)
#define MALI_RENAME(name) __MALI_CONCAT(name, MALI_VER_SUFFIX)

/*
 * External-facing symbols renamed with version suffix.
 * These are called by external consumers through mali_redirect.
 */

/* gpu_dvfs API (mali_exynos_if.h / gpu_cooling.h) */
#define gpu_dvfs_get_cur_clock              MALI_RENAME(gpu_dvfs_get_cur_clock)
#define gpu_dvfs_get_clock                  MALI_RENAME(gpu_dvfs_get_clock)
#define gpu_dvfs_get_voltage                MALI_RENAME(gpu_dvfs_get_voltage)
#define gpu_dvfs_get_step                   MALI_RENAME(gpu_dvfs_get_step)
#define gpu_dvfs_get_utilization            MALI_RENAME(gpu_dvfs_get_utilization)
#define gpu_dvfs_get_max_freq               MALI_RENAME(gpu_dvfs_get_max_freq)
#define gpu_dvfs_get_min_freq               MALI_RENAME(gpu_dvfs_get_min_freq)
#define gpu_dvfs_get_max_locked_freq        MALI_RENAME(gpu_dvfs_get_max_locked_freq)
#define gpu_dvfs_get_min_locked_freq        MALI_RENAME(gpu_dvfs_get_min_locked_freq)
#define gpu_dvfs_get_freq_table             MALI_RENAME(gpu_dvfs_get_freq_table)
#define gpu_dvfs_get_time_in_state          MALI_RENAME(gpu_dvfs_get_time_in_state)
#define gpu_dvfs_get_tis_last_update        MALI_RENAME(gpu_dvfs_get_tis_last_update)
#define gpu_dvfs_get_job_queue_count        MALI_RENAME(gpu_dvfs_get_job_queue_count)
#define gpu_dvfs_get_job_queue_last_updated MALI_RENAME(gpu_dvfs_get_job_queue_last_updated)
#define gpu_dvfs_set_amigo_governor         MALI_RENAME(gpu_dvfs_set_amigo_governor)
#define gpu_dvfs_get_freq_margin            MALI_RENAME(gpu_dvfs_get_freq_margin)
#define gpu_dvfs_set_freq_margin            MALI_RENAME(gpu_dvfs_set_freq_margin)
#define gpu_dvfs_get_sustainable_info_array MALI_RENAME(gpu_dvfs_get_sustainable_info_array)
#define gpu_dvfs_get_max_lock               MALI_RENAME(gpu_dvfs_get_max_lock)
#define gpu_dvfs_get_need_cpu_qos           MALI_RENAME(gpu_dvfs_get_need_cpu_qos)
#define gpu_dvfs_get_weight_table_idx0      MALI_RENAME(gpu_dvfs_get_weight_table_idx0)
#define gpu_dvfs_set_weight_table_idx0      MALI_RENAME(gpu_dvfs_set_weight_table_idx0)
#define gpu_dvfs_get_weight_table_idx1      MALI_RENAME(gpu_dvfs_get_weight_table_idx1)
#define gpu_dvfs_set_weight_table_idx1      MALI_RENAME(gpu_dvfs_set_weight_table_idx1)
#define gpu_dvfs_register_utilization_notifier   MALI_RENAME(gpu_dvfs_register_utilization_notifier)
#define gpu_dvfs_unregister_utilization_notifier MALI_RENAME(gpu_dvfs_unregister_utilization_notifier)
#define gpex_dvfs_external_init             MALI_RENAME(gpex_dvfs_external_init)

/* GPU thermal/tmu API */
#define gpu_tmu_get_notifier                MALI_RENAME(gpu_tmu_get_notifier)
#define gpu_tmu_notifier                    MALI_RENAME(gpu_tmu_notifier)
#define exynos_tmu_extern_get_temp          MALI_RENAME(exynos_tmu_extern_get_temp)

/* TSG / stats API (exynos_stats_*) */
#define exynos_stats_get_gpu_cur_idx        MALI_RENAME(exynos_stats_get_gpu_cur_idx)
#define exynos_stats_get_job_state_cnt       MALI_RENAME(exynos_stats_get_job_state_cnt)
#define exynos_stats_get_gpu_coeff          MALI_RENAME(exynos_stats_get_gpu_coeff)
#define exynos_stats_get_gpu_table_size     MALI_RENAME(exynos_stats_get_gpu_table_size)
#define exynos_stats_get_gpu_freq_table     MALI_RENAME(exynos_stats_get_gpu_freq_table)
#define exynos_stats_get_gpu_volt_table     MALI_RENAME(exynos_stats_get_gpu_volt_table)
#define exynos_stats_get_gpu_time_in_state  MALI_RENAME(exynos_stats_get_gpu_time_in_state)
#define exynos_stats_get_gpu_max_lock       MALI_RENAME(exynos_stats_get_gpu_max_lock)
#define exynos_stats_get_gpu_min_lock       MALI_RENAME(exynos_stats_get_gpu_min_lock)
#define exynos_stats_set_queued_threshold_0 MALI_RENAME(exynos_stats_set_queued_threshold_0)
#define exynos_stats_set_queued_threshold_1 MALI_RENAME(exynos_stats_set_queued_threshold_1)
#define exynos_stats_get_gpu_queued_job_time   MALI_RENAME(exynos_stats_get_gpu_queued_job_time)
#define exynos_stats_get_gpu_queued_last_updated MALI_RENAME(exynos_stats_get_gpu_queued_last_updated)
#define exynos_stats_set_gpu_polling_speed   MALI_RENAME(exynos_stats_set_gpu_polling_speed)
#define exynos_stats_get_gpu_polling_speed   MALI_RENAME(exynos_stats_get_gpu_polling_speed)
#define exynos_stats_get_run_times           MALI_RENAME(exynos_stats_get_run_times)
#define exynos_stats_get_pid_list            MALI_RENAME(exynos_stats_get_pid_list)
#define exynos_stats_set_vsync               MALI_RENAME(exynos_stats_set_vsync)
#define exynos_stats_get_frame_info          MALI_RENAME(exynos_stats_get_frame_info)

/* Migov helpers */
#define exynos_migov_set_mode                MALI_RENAME(exynos_migov_set_mode)
#define exynos_migov_set_gpu_margin          MALI_RENAME(exynos_migov_set_gpu_margin)
#define exynos_migov_set_targetframetime     MALI_RENAME(exynos_migov_set_targetframetime)
#define exynos_migov_set_targettime_margin   MALI_RENAME(exynos_migov_set_targettime_margin)
#define exynos_migov_set_util_margin         MALI_RENAME(exynos_migov_set_util_margin)
#define exynos_migov_set_decon_time          MALI_RENAME(exynos_migov_set_decon_time)
#define exynos_migov_set_comb_ctrl           MALI_RENAME(exynos_migov_set_comb_ctrl)

/* SDP */
#define exynos_sdp_set_powertable            MALI_RENAME(exynos_sdp_set_powertable)
#define exynos_sdp_set_busy_domain           MALI_RENAME(exynos_sdp_set_busy_domain)
#define exynos_sdp_set_cur_freqlv            MALI_RENAME(exynos_sdp_set_cur_freqlv)

/* GTS */
#define gpu_register_out_data                MALI_RENAME(gpu_register_out_data)

/* STC config */
#define exynos_gpu_stc_config_show           MALI_RENAME(exynos_gpu_stc_config_show)
#define exynos_gpu_stc_config_store          MALI_RENAME(exynos_gpu_stc_config_store)

/* Notifiers */
#define register_frag_utils_change_notifier   MALI_RENAME(register_frag_utils_change_notifier)
#define unregister_frag_utils_change_notifier MALI_RENAME(unregister_frag_utils_change_notifier)

/* kbase internal exports (for other kbase consumers) */
#define kbase_get_create_info                MALI_RENAME(kbase_get_create_info)

#endif /* _MALI_EXPORTS_H_ */
