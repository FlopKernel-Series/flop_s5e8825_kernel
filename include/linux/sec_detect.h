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

// Device feature helpers
enum SEC_devices sec_get_current_device(void);
bool sec_feat_needs_decon(void);
bool sec_feat_needs_blic(void);
bool sec_feat_doze(void);
bool sec_feat_lcd_device(void);
bool sec_feat_legacy_sinput(void);
bool sec_feat_legacy_usbpd(void);
bool sec_feat_slsi_usbpd(void);


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

// Legacy wrappers
bool sec_has_mcd_disable_dual_sync(void);
bool sec_has_mcd_camera_rear_dual_cal(void);
bool sec_has_mcd_use_leds_flash_charging_voltage_control(void);
bool sec_has_mcd_use_camera_adaptive_mipi(void);
bool sec_has_mcd_use_imx258_13mp_full_size(void);
bool sec_has_mcd_apply_mirror_vertical_flip(void);
bool sec_has_mcd_simplify_ois_init(void);
bool sec_has_mcd_camera_front_fixed_focus(void);
bool sec_has_mcd_modify_cal_map_for_swremosaic_lib(void);
bool sec_has_mcd_front_otprom_eeprom(void);
bool sec_has_mcd_camera_uwide_dualized(void);
bool sec_has_mcd_read_dual_cal_firmware_data(void);
bool sec_has_mcd_config_camera_eeprom_dualized(void);
bool sec_has_mcd_config_check_hw_version_for_mcu_fw_upload(void);
bool sec_has_mcd_use_camera_act_driver_soft_landing(void);
bool sec_has_mcd_use_ois_hall_data_for_vdis(void);
bool sec_has_mcd_use_hi1336c_setfile(void);
bool sec_has_mcd_camera_use_aois(void);
bool sec_has_mcd_cal_for_hw_ggc_a26x(void);
bool sec_has_mcd_use_ois_reset_autotest(void);
bool sec_has_mcd_ois_angle_support(void);

bool sec_is_detection_complete(void);

#endif /* _LINUX_SEC_H */
