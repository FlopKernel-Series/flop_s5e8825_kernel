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

#ifndef __S6E8FC3_A26X_H__
#define __S6E8FC3_A26X_H__
#include "../panel.h"
#include "../panel_drv.h"

#define S6E8FC3_A26X_BR_INDEX_PROPERTY ("s6e8fc3_a26x_br_index")
#define S6E8FC3_A26X_FIRST_BR_PROPERTY ("s6e8fc3_a26x_first_br_property")

#define S6E8FC3_A26X_MAX_NIGHT_MODE	(2)
#define S6E8FC3_A26X_MAX_NIGHT_LEVEL	(102)

enum {
	S6E8FC3_A26X_FIRST_BR_OFF = 0,
	S6E8FC3_A26X_FIRST_BR_ON,
	MAX_S6E8FC3_A26X_FIRST_BR
};

#endif /* __S6E8FC3_A26X_H__ */
