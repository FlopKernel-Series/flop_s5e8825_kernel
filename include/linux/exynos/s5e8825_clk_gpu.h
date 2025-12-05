/* SPDX-License-Identifier: GPL-2.0 */
#ifndef S5E8825_CCLK_GPU_H
#define S5E8825_CCLK_GPU_H

#include <linux/workarounds.h>

#define GPU_FREQ_STOCK_KHZ_MAX (897000)
#ifndef CPU_MAX
#define CPU_MAX INT_MAX
#endif

/* GPU Clocks */
/* Max frequency for the GPU - use OC values if superfloppy mode is enabled */
#define GPU_FREQ_KHZ_MAX (is_superfloppy_mode_fast() ? 1209000 : 897000)
#define GPU_FREQ_KHZ_MIN (104000)

/* Stock frequencies */
static const unsigned int gpu_stock_clock[] = {897000, 806000, 702000, 611000, 507000, 403000, 312000, 208000, 104000};
static const unsigned int gpu_stock_min_threshold[] = {78, 78, 78, 78, 70, 60, 50, 30, 0};
static const unsigned int gpu_stock_max_threshold[] = {100, 95, 95, 95, 90, 80, 70, 60, 40};
static const unsigned int gpu_stock_staycount[] = {5, 5, 5, 5, 3, 3, 2, 2, 1};
static const unsigned int gpu_stock_mem_freq[] = {1794000, 1539000, 1352000, 1352000, 1014000, 1014000, 845000, 676000, 676000};
static const unsigned int gpu_stock_lit[] = {1056000, 1056000, 1056000, 1056000, 1056000, 0, 0, 0, 0};

/* Overclocked frequencies */
static const unsigned int gpu_oc_clock[] = {1209000, 1105000, 1001000, 897000, 806000, 702000, 611000, 507000, 403000, 312000, 208000, 104000};
static const unsigned int gpu_oc_min_threshold[] = {90, 87, 85, 82, 80, 79, 78, 70, 60, 50, 30, 0};
static const unsigned int gpu_oc_max_threshold[] = {100, 96 ,95, 95, 95, 95, 95, 90, 80, 70, 60, 40};
static const unsigned int gpu_oc_staycount[] = {5, 5, 5, 5, 5, 5, 5, 3, 3, 2, 2, 1};
static const unsigned int gpu_oc_mem_freq[] = {2093000, 2093000, 1794000, 1794000, 1539000, 1352000, 1352000, 1014000, 1014000, 845000, 676000, 676000};
static const unsigned int gpu_oc_lit[] = {1536000, 1440000, 1248000, 1056000, 1056000, 1056000, 1056000, 1056000, 0, 0, 0, 0};

/* Runtime selection based on superfloppy mode */
static inline const unsigned int *gpu_custom_clock(void)
{
	return is_superfloppy_mode_fast() ? gpu_oc_clock : gpu_stock_clock;
}

static inline const unsigned int *gpu_custom_min_threshold(void)
{
	return is_superfloppy_mode_fast() ? gpu_oc_min_threshold : gpu_stock_min_threshold;
}

static inline const unsigned int *gpu_custom_max_threshold(void)
{
	return is_superfloppy_mode_fast() ? gpu_oc_max_threshold : gpu_stock_max_threshold;
}

static inline const unsigned int *gpu_custom_staycount(void)
{
	return is_superfloppy_mode_fast() ? gpu_oc_staycount : gpu_stock_staycount;
}

static inline const unsigned int *gpu_custom_mem_freq(void)
{
	return is_superfloppy_mode_fast() ? gpu_oc_mem_freq : gpu_stock_mem_freq;
}

static inline const unsigned int *gpu_custom_lit(void)
{
	return is_superfloppy_mode_fast() ? gpu_oc_lit : gpu_stock_lit;
}

static inline unsigned int gpu_custom_mid(void)
{
	return 0;
}

static inline unsigned int gpu_custom_big(void)
{
	return CPU_MAX;
}

static inline int gpu_custom_array_size(void)
{
	return is_superfloppy_mode_fast() ?
		(sizeof(gpu_oc_clock) / sizeof(gpu_oc_clock[0])) :
		(sizeof(gpu_stock_clock) / sizeof(gpu_stock_clock[0]));
}

#endif