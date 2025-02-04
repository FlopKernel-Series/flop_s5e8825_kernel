/****************************************************************************
 *
 * Copyright (c) 2014 - 2017 Samsung Electronics Co., Ltd. All rights reserved
 *
 ****************************************************************************/

#ifndef __MIFMBOXMAN_H
#define __MIFMBOXMAN_H

#include <linux/mutex.h>

/* TODO:  Needs to define the max mem */

struct mifmboxman;
struct scsc_mif_abs;
struct mutex;

int mifmboxman_init(struct mifmboxman *mbox);
bool mifmboxman_alloc_mboxes(struct mifmboxman *mbox, int n, int *first_mbox_index);
void mifmboxman_free_mboxes(struct mifmboxman *mbox, int first_mbox_index, int n);
u32 *mifmboxman_get_mbox_ptr(struct mifmboxman *mbox,  struct scsc_mif_abs *mif_abs, int mbox_index);
#if IS_ENABLED(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM)
u32 *mifmboxman_get_mbox_ptr_wpan(struct mifmboxman *mbox,  struct scsc_mif_abs *mif_abs, int mbox_index);
#endif
#if defined(CONFIG_WLBT_DCXO_TUNE)
int mifmboxman_set_dcxo_tune_value(struct scsc_mif_abs *mif_abs, u32 value);
int mifmboxman_get_dcxo_tune_value(struct scsc_mif_abs *mif_abs, u32* value);
#endif
int mifmboxman_deinit(struct mifmboxman *mbox);

#ifdef CONFIG_SOC_EXYNOS7570
#define MIFMBOX_NUM	8
#elif defined(CONFIG_SOC_EXYNOS7872) || defined(CONFIG_SOC_EXYNOS7885) || defined(CONFIG_SOC_EXYNOS9610) || defined(CONFIG_SOC_EXYNOS9630)
#define MIFMBOX_NUM	4
#elif defined(CONFIG_SCSC_PCIE_MBOX_EMULATION)
#define MIFMBOX_NUM 8
#else /* default */
#define MIFMBOX_NUM 4
#endif

#if defined(CONFIG_WLBT_DCXO_TUNE)
#define MIFMBOX_APM_OP_GET_TUNE (0x4)
#define MIFMBOX_APM_OP_SET_TUNE (0x5)
#endif

/* Inclusion in core.c treat it as opaque */
struct mifmboxman {
	bool         in_use;
	u32          mbox_free;
	DECLARE_BITMAP(bitmap, MIFMBOX_NUM);
	struct mutex lock;
};
#endif
