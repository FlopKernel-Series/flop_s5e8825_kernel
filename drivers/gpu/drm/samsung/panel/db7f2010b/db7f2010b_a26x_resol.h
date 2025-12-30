/*
 * linux/drivers/video/fbdev/exynos/panel/db7f2010b/db7f2010b_a26x_resol.h
 *
 * Header file for Panel Driver
 *
 * Copyright (c) 2019 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __DB7F2010B_A26X_RESOL_H__
#define __DB7F2010B_A26X_RESOL_H__

#include <dt-bindings/display/panel-display.h>
#include "../panel.h"
#include "db7f2010b.h"
#include "db7f2010b_dimming.h"

enum {
	DB7F2010B_A26X_DISPLAY_MODE_1080x2340_120HS,
	DB7F2010B_A26X_DISPLAY_MODE_1080x2340_60HS,
	MAX_DB7F2010B_A26X_DISPLAY_MODE,
};

struct panel_vrr db7f2010b_a26x_default_panel_vrr[] = {
	[DB7F2010B_VRR_120HS] = {
		.fps = 120,
		.te_sw_skip_count = 0,
		.te_hw_skip_count = 0,
		.mode = VRR_HS_MODE,
	},
	[DB7F2010B_VRR_60HS] = {
		.fps = 60,
		.te_sw_skip_count = 0,
		.te_hw_skip_count = 0,
		.mode = VRR_HS_MODE,
	},
};

static struct panel_vrr *db7f2010b_a26x_default_vrrtbl[] = {
	&db7f2010b_a26x_default_panel_vrr[DB7F2010B_VRR_120HS],
	&db7f2010b_a26x_default_panel_vrr[DB7F2010B_VRR_60HS],
};

static struct panel_resol db7f2010b_a26x_default_resol[] = {
	[DB7F2010B_RESOL_1080x2340] = {
		.w = 1080,
		.h = 2340,
		.comp_type = PN_COMP_TYPE_DSC,
		.comp_param = {
			.dsc = {
				.slice_w = 540,
				.slice_h = 40,
			},
		},
		.available_vrr = db7f2010b_a26x_default_vrrtbl,
		.nr_available_vrr = ARRAY_SIZE(db7f2010b_a26x_default_vrrtbl),
	},
};

#if defined(CONFIG_USDM_PANEL_DISPLAY_MODE)
static struct common_panel_display_mode db7f2010b_a26x_display_mode[] = {
	/* FHD */
	[DB7F2010B_A26X_DISPLAY_MODE_1080x2340_120HS] = {
		.name = PANEL_DISPLAY_MODE_1080x2340_120HS,
		.resol = &db7f2010b_a26x_default_resol[DB7F2010B_RESOL_1080x2340],
		.vrr = &db7f2010b_a26x_default_panel_vrr[DB7F2010B_VRR_120HS],
	},
	[DB7F2010B_A26X_DISPLAY_MODE_1080x2340_60HS] = {
		.name = PANEL_DISPLAY_MODE_1080x2340_60HS,
		.resol = &db7f2010b_a26x_default_resol[DB7F2010B_RESOL_1080x2340],
		.vrr = &db7f2010b_a26x_default_panel_vrr[DB7F2010B_VRR_60HS],
	},
};

static struct common_panel_display_mode *db7f2010b_a26x_display_mode_array[] = {
	[DB7F2010B_A26X_DISPLAY_MODE_1080x2340_120HS] = &db7f2010b_a26x_display_mode[DB7F2010B_A26X_DISPLAY_MODE_1080x2340_120HS],
	[DB7F2010B_A26X_DISPLAY_MODE_1080x2340_60HS] = &db7f2010b_a26x_display_mode[DB7F2010B_A26X_DISPLAY_MODE_1080x2340_60HS],
};

static struct common_panel_display_modes db7f2010b_a26x_display_modes = {
	.num_modes = ARRAY_SIZE(db7f2010b_a26x_display_mode),
	.modes = (struct common_panel_display_mode **)&db7f2010b_a26x_display_mode_array,
};
#endif /* CONFIG_USDM_PANEL_DISPLAY_MODE */
#endif /* __DB7F2010B_A26X_RESOL_H__ */
