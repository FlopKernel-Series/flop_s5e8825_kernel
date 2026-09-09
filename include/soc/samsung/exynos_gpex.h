/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Samsung Exynos GPU Platform Extension (GPEX) Interface
 *
 * Copyright (c) 2021 Samsung Electronics Co., Ltd.
 */

#ifndef _LINUX_SOC_SAMSUNG_EXYNOS_GPEX_H_
#define _LINUX_SOC_SAMSUNG_EXYNOS_GPEX_H_

#include <linux/types.h>
#include <linux/device.h>

struct exynos_gpex_gpu_ops {
	/* Query current GPU utilization percentage (0 - 100), or -1 if idle/unavailable */
	int (*get_utilization)(void);

	/* Clean and flush hardware GPU caches (used during secure rendering transitions) */
	void (*clean_caches)(void);

	/* Optional: fine-grained job load tracking notification */
	void (*update_job_load)(u32 job_type, u64 ns_spent);
};

#if IS_REACHABLE(CONFIG_EXYNOS_GPEX)
/*
 * GPU Driver Registration API
 * Called by the active GPU KMD (Mali or Panfrost) during device probe.
 */
int exynos_gpex_register_gpu(struct device *dev, const struct exynos_gpex_gpu_ops *ops);
void exynos_gpex_unregister_gpu(struct device *dev);
struct device *exynos_gpex_get_gpu_device(void);
bool exynos_gpex_is_attached(void);

/*
 * Power Management & Frequency Control APIs
 */
int exynos_gpex_set_frequency(unsigned long freq_hz);
unsigned long exynos_gpex_get_frequency(void);
int exynos_gpex_pm_resume(struct device *dev);
int exynos_gpex_pm_suspend(struct device *dev);
void exynos_gpex_setup_coherency(void);
int exynos_gpex_init_opp_table(struct device *dev);
void exynos_gpex_sync_opp_table(int max_khz);
#else
static inline int exynos_gpex_register_gpu(struct device *dev, const struct exynos_gpex_gpu_ops *ops) { return 0; }
static inline void exynos_gpex_unregister_gpu(struct device *dev) {}
static inline struct device *exynos_gpex_get_gpu_device(void) { return NULL; }
static inline bool exynos_gpex_is_attached(void) { return false; }

static inline int exynos_gpex_set_frequency(unsigned long freq_hz) { return 0; }
static inline unsigned long exynos_gpex_get_frequency(void) { return 0; }
static inline int exynos_gpex_pm_resume(struct device *dev) { return 0; }
static inline int exynos_gpex_pm_suspend(struct device *dev) { return 0; }
static inline void exynos_gpex_setup_coherency(void) {}
static inline int exynos_gpex_init_opp_table(struct device *dev) { return -ENODEV; }
static inline void exynos_gpex_sync_opp_table(int max_khz) {}
#endif

#endif /* _LINUX_SOC_SAMSUNG_EXYNOS_GPEX_H_ */
