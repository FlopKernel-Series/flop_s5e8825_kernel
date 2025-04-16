/*
 * linux/drivers/video/fbdev/exynos/panel/nt36672c_m33x_00/nt36672c_m33_00.h
 *
 * Header file for TFT_COMMON Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __DB7F2010B_A26X_H__
#define __DB7F2010B_A26X_H__
#include "../panel.h"
#include "../panel_drv.h"

#define DB7F2010B_A26X_BR_INDEX_PROPERTY ("db7f2010b_a26x_br_index")
#define DB7F2010B_A26X_FIRST_BR_PROP ("db7f2010b_a26x_first_br_prop")
#define DB7F2010B_A26X_OSC_CLK_PROPERTY ("db7f2010b_a26x_osc_clk_property")
#define DB7F2010B_A26X_OSC_CHANGE_PROPERTY ("a26x_osc_change_property")

#define DB7F2010B_A26X_MAX_NIGHT_MODE	(2)
#define DB7F2010B_A26X_MAX_NIGHT_LEVEL	(102)

enum {
	DB7F2010B_A26X_FIRST_BR_OFF = 0,
	DB7F2010B_A26X_FIRST_BR_ON,
	MAX_DB7F2010B_A26X_FIRST_BR
};

#endif /* __DB7F2010B_A26X_H__ */
