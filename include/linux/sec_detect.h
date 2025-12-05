// SPDX-License-Identifier: GPL-2.0-only
/*
 * Author: @Flopster101
 * Based on AkiraNoSushi's work for the Mi439 project.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _LINUX_SEC_H
#define _LINUX_SEC_H

#include <linux/types.h>
#ifdef CONFIG_JUMP_LABEL
#include <linux/jump_label.h>
#endif

#define SEC_DETECT_LOG(fmt, ...) printk(KERN_INFO "sec_detect: " fmt, ##__VA_ARGS__)
static const char *sec_detect_label = "sec_detect: ";

enum SEC_devices {
	DEVICE_UNKNOWN = -1,
	SEC_A25,
	SEC_A26XS,
	SEC_A33,
	SEC_A53,
	SEC_M33,
	SEC_M34,
	SEC_GTA4XLS
};

static const char *const device_names[] = {
	[SEC_A25] = "Galaxy A25 5G",
	[SEC_A26XS] = "Galaxy A26 5G (LATAM)",
	[SEC_A33] = "Galaxy A33 5G",
	[SEC_A53] = "Galaxy A53 5G",
	[SEC_M33] = "Galaxy M33 5G",
	[SEC_M34] = "Galaxy M34 5G",
	[SEC_GTA4XLS] = "Galaxy Tab S6 Lite 2024",
};

// Device feature flags
enum sec_feat {
	SEC_FEAT_NEEDS_DECON,		// Uses DECON protocol for display
	SEC_FEAT_NEEDS_BLIC,		// Uses BLIC (backlight IC?)
	SEC_FEAT_DOZE, 				// Uses Samsung DRM Doze
	SEC_FEAT_LCD_DEVICE,		// Uses LCD panel
	SEC_FEAT_LEGACY_SINPUT,		// Uses old sec_input driver
	SEC_FEAT_SLSI_USBPD,		// Uses slsi usbpd platform
	SEC_FEAT_COUNT
};

enum SEC_devices sec_get_current_device(void);

bool sec_get_feat(enum sec_feat feat);

// Optimized hot path versions using static branch
#ifdef CONFIG_JUMP_LABEL
extern struct static_key_false sec_feat_needs_decon_key;
static inline bool sec_get_feat_needs_decon_fast(void)
{
	return static_branch_unlikely(&sec_feat_needs_decon_key);
}

extern struct static_key_false sec_feat_lcd_device_key;
static inline bool sec_get_feat_lcd_device_fast(void)
{
	return static_branch_unlikely(&sec_feat_lcd_device_key);
}

extern struct static_key_false sec_feat_needs_blic_key;
static inline bool sec_get_feat_needs_blic_fast(void)
{
	return static_branch_unlikely(&sec_feat_needs_blic_key);
}
#else
static inline bool sec_get_feat_needs_decon_fast(void)
{
	return sec_get_feat(SEC_FEAT_NEEDS_DECON);
}

static inline bool sec_get_feat_lcd_device_fast(void)
{
	return sec_get_feat(SEC_FEAT_LCD_DEVICE);
}

static inline bool sec_get_feat_needs_blic_fast(void)
{
	return sec_get_feat(SEC_FEAT_NEEDS_BLIC);
}
#endif

// Camera feature flags
enum mcd_feat {
	MCD_DISABLE_DUAL_SYNC = 0,
	MCD_CAMERA_REAR_DUAL_CAL,
	MCD_USE_LEDS_FLASH_CHARGING_VOLTAGE_CONTROL,
	MCD_USE_CAMERA_ADAPTIVE_MIPI,
	MCD_USE_IMX258_13MP_FULL_SIZE,
	MCD_APPLY_MIRROR_VERTICAL_FLIP,
	MCD_SIMPLIFY_OIS_INIT,
	MCD_MODIFY_CAL_MAP_FOR_SWREMOSAIC_LIB,
	MCD_FRONT_OTPROM_EEPROM,
	MCD_CAMERA_UWIDE_DUALIZED,
	MCD_READ_DUAL_CAL_FIRMWARE_DATA,
	MCD_CAMERA_FRONT_FIXED_FOCUS,
	MCD_CONFIG_CAMERA_EEPROM_DUALIZED,
	MCD_CONFIG_CHECK_HW_VERSION_FOR_MCU_FW_UPLOAD,
	MCD_USE_CAMERA_ACT_DRIVER_SOFT_LANDING,
	MCD_USE_OIS_HALL_DATA_FOR_VDIS,
	MCD_USE_HI1336C_SETFILE,
	MCD_CAMERA_USE_AOIS,
	MCD_CAL_FOR_HW_GGC_A26X,
	MCD_USE_OIS_RESET_AUTOTEST,
	MCD_OIS_ANGLE_SUPPORT,
	MCD_FEAT_COUNT
};

bool sec_get_mcd_feat(enum mcd_feat feat);

bool sec_is_detection_complete(void);

#endif /* _LINUX_SEC_H */
