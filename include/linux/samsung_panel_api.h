/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _SAMSUNG_PANEL_UNIFIED_API_H_
#define _SAMSUNG_PANEL_UNIFIED_API_H_

#include <linux/types.h>
#include <linux/notifier.h>

struct stui_buf_info;
struct resolution_info;

/* USDM Panel API (a25x, m34x, gta4xls, a26xs) */
// extern int get_lcd_info(const char *mode);
extern int exynos_atomic_enter_tui(void);
extern int exynos_atomic_exit_tui(void);
extern void exynos_tui_set_stui_funcs(struct stui_buf_info *(*)(void),
				      void (*)(void));
extern void exynos_tui_get_resolution(struct resolution_info *res_info);
extern int exynos_tui_get_panel_info(u64 *buf, int size);

/* DECON Panel API (a33x, a53x, m33x) */
extern int decon_get_lcd_info(const char *mode);
extern int decon_exynos_atomic_enter_tui(void);
extern int decon_exynos_atomic_exit_tui(void);
extern void decon_exynos_tui_set_stui_funcs(struct stui_buf_info *(*)(void),
					    void (*)(void));
extern void decon_exynos_tui_get_resolution(struct resolution_info *res_info);
extern int decon_exynos_tui_get_panel_info(u64 *buf, int size);

/* Notifier API (unified via sec_panel_notifier_v2) */
#define decon_panel_notifier_register(nb) panel_notifier_register(nb)
#define decon_panel_notifier_unregister(nb) panel_notifier_unregister(nb)
#define usdm_panel_notifier_register(nb) panel_notifier_register(nb)
#define usdm_panel_notifier_unregister(nb) panel_notifier_unregister(nb)

#endif /* _SAMSUNG_PANEL_UNIFIED_API_H_ */
