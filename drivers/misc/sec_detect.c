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

#include <linux/sec_detect.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/string.h>
#ifdef CONFIG_SEC_DETECT_SYSFS
#include <linux/kobject.h>
#include <linux/sysfs.h>
#endif

static int g_sec_current_device = DEVICE_UNKNOWN;
static char g_sec_current_device_name[32] = "Unknown";
static bool g_sec_needs_decon;
static bool g_sec_needs_blic = false;
static bool g_sec_doze = false; // Uses Samsung DRM Doze?
static bool g_sec_lcd_device = false;
static bool g_sec_legacy_sinput = false; // Uses old sec_input driver?
static bool g_sec_legacy_usbpd = false; // Uses old slsi usbpd platform?
static bool g_sec_slsi_usbpd = false;

// Helper functions for each g_sec_ variable
enum SEC_devices sec_get_current_device(void) { return g_sec_current_device; }
EXPORT_SYMBOL_GPL(sec_get_current_device);

bool sec_feat_needs_decon(void) { return g_sec_needs_decon; }
EXPORT_SYMBOL_GPL(sec_feat_needs_decon);

bool sec_feat_needs_blic(void) { return g_sec_needs_blic; }
EXPORT_SYMBOL_GPL(sec_feat_needs_blic);

bool sec_feat_doze(void) { return g_sec_doze; }
EXPORT_SYMBOL_GPL(sec_feat_doze);

bool sec_feat_lcd_device(void) { return g_sec_lcd_device; }
EXPORT_SYMBOL_GPL(sec_feat_lcd_device);

bool sec_feat_legacy_sinput(void) { return g_sec_legacy_sinput; }
EXPORT_SYMBOL_GPL(sec_feat_legacy_sinput);

bool sec_feat_legacy_usbpd(void) { return g_sec_legacy_usbpd; }
EXPORT_SYMBOL_GPL(sec_feat_legacy_usbpd);

bool sec_feat_slsi_usbpd(void) { return g_sec_slsi_usbpd; }
EXPORT_SYMBOL_GPL(sec_feat_slsi_usbpd);

// Camera params
static bool mcd_disable_dual_sync = false;
static bool mcd_camera_rear_dual_cal = false;
static bool mcd_use_leds_flash_charging_voltage_control = false;
static bool mcd_use_camera_adaptive_mipi = false;
static bool mcd_use_imx258_13mp_full_size = false;
static bool mcd_apply_mirror_vertical_flip = false;
static bool mcd_simplify_ois_init = false;
static bool mcd_modify_cal_map_for_swremosaic_lib = false;
static bool mcd_front_otprom_eeprom = false;
static bool mcd_camera_uwide_dualized = false;
static bool mcd_read_dual_cal_firmware_data = false;
static bool mcd_camera_front_fixed_focus = false;
static bool mcd_config_camera_eeprom_dualized = false;
static bool mcd_config_check_hw_version_for_mcu_fw_upload = false;
static bool mcd_use_camera_act_driver_soft_landing = false;
static bool mcd_use_ois_hall_data_for_vdis = false;
static bool mcd_use_hi1336c_setfile = false;
static bool mcd_camera_use_aois = false;
static bool mcd_cal_for_hw_ggc_a26x = false;
static bool mcd_use_ois_reset_autotest = false;
static bool mcd_ois_angle_support = false;

// Helper functions for each mcd_ variable
bool sec_has_mcd_disable_dual_sync(void) { return mcd_disable_dual_sync; }
EXPORT_SYMBOL_GPL(sec_has_mcd_disable_dual_sync);

bool sec_has_mcd_camera_rear_dual_cal(void) { return mcd_camera_rear_dual_cal; }
EXPORT_SYMBOL_GPL(sec_has_mcd_camera_rear_dual_cal);

bool sec_has_mcd_use_leds_flash_charging_voltage_control(void) { return mcd_use_leds_flash_charging_voltage_control; }
EXPORT_SYMBOL_GPL(sec_has_mcd_use_leds_flash_charging_voltage_control);

bool sec_has_mcd_use_camera_adaptive_mipi(void) { return mcd_use_camera_adaptive_mipi; }
EXPORT_SYMBOL_GPL(sec_has_mcd_use_camera_adaptive_mipi);

bool sec_has_mcd_use_imx258_13mp_full_size(void) { return mcd_use_imx258_13mp_full_size; }
EXPORT_SYMBOL_GPL(sec_has_mcd_use_imx258_13mp_full_size);

bool sec_has_mcd_apply_mirror_vertical_flip(void) { return mcd_apply_mirror_vertical_flip; }
EXPORT_SYMBOL_GPL(sec_has_mcd_apply_mirror_vertical_flip);

bool sec_has_mcd_simplify_ois_init(void) { return mcd_simplify_ois_init; }
EXPORT_SYMBOL_GPL(sec_has_mcd_simplify_ois_init);

bool sec_has_mcd_modify_cal_map_for_swremosaic_lib(void) { return mcd_modify_cal_map_for_swremosaic_lib; }
EXPORT_SYMBOL_GPL(sec_has_mcd_modify_cal_map_for_swremosaic_lib);

bool sec_has_mcd_front_otprom_eeprom(void) { return mcd_front_otprom_eeprom; }
EXPORT_SYMBOL_GPL(sec_has_mcd_front_otprom_eeprom);

bool sec_has_mcd_camera_uwide_dualized(void) { return mcd_camera_uwide_dualized; }
EXPORT_SYMBOL_GPL(sec_has_mcd_camera_uwide_dualized);

bool sec_has_mcd_read_dual_cal_firmware_data(void) { return mcd_read_dual_cal_firmware_data; }
EXPORT_SYMBOL_GPL(sec_has_mcd_read_dual_cal_firmware_data);

bool sec_has_mcd_camera_front_fixed_focus(void) { return mcd_camera_front_fixed_focus; }
EXPORT_SYMBOL_GPL(sec_has_mcd_camera_front_fixed_focus);

bool sec_has_mcd_config_camera_eeprom_dualized(void) { return mcd_config_camera_eeprom_dualized; }
EXPORT_SYMBOL_GPL(sec_has_mcd_config_camera_eeprom_dualized);

bool sec_has_mcd_config_check_hw_version_for_mcu_fw_upload(void) { return mcd_config_check_hw_version_for_mcu_fw_upload; }
EXPORT_SYMBOL_GPL(sec_has_mcd_config_check_hw_version_for_mcu_fw_upload);

bool sec_has_mcd_use_camera_act_driver_soft_landing(void) { return mcd_use_camera_act_driver_soft_landing; }
EXPORT_SYMBOL_GPL(sec_has_mcd_use_camera_act_driver_soft_landing);

bool sec_has_mcd_use_ois_hall_data_for_vdis(void) { return mcd_use_ois_hall_data_for_vdis; }
EXPORT_SYMBOL_GPL(sec_has_mcd_use_ois_hall_data_for_vdis);

bool sec_has_mcd_use_hi1336c_setfile(void) { return mcd_use_hi1336c_setfile; }
EXPORT_SYMBOL_GPL(sec_has_mcd_use_hi1336c_setfile);

bool sec_has_mcd_camera_use_aois(void) { return mcd_camera_use_aois; }
EXPORT_SYMBOL_GPL(sec_has_mcd_camera_use_aois);

bool sec_has_mcd_cal_for_hw_ggc_a26x(void) { return mcd_cal_for_hw_ggc_a26x; }
EXPORT_SYMBOL_GPL(sec_has_mcd_cal_for_hw_ggc_a26x);

bool sec_has_mcd_use_ois_reset_autotest(void) { return mcd_use_ois_reset_autotest; }
EXPORT_SYMBOL_GPL(sec_has_mcd_use_ois_reset_autotest);

bool sec_has_mcd_ois_angle_support(void) { return mcd_ois_angle_support; }
EXPORT_SYMBOL_GPL(sec_has_mcd_ois_angle_support);

static bool g_detection_complete = false;

bool sec_is_detection_complete(void) {
    return g_detection_complete;
}
EXPORT_SYMBOL_GPL(sec_is_detection_complete);

#ifdef CONFIG_SEC_DETECT_SYSFS
// Sysfs attribute to show the current device name
static ssize_t device_name_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return snprintf(buf, 32, "%s\n", g_sec_current_device_name);
}

// Sysfs attribute to show the current device model
static ssize_t device_model_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	const char *model_name = "Unknown";
	if (g_sec_current_device >= 0 && g_sec_current_device < ARRAY_SIZE(device_names))
		model_name = device_names[g_sec_current_device];
	return snprintf(buf, 32, "%s\n", model_name);
}

static struct kobj_attribute device_name_attr = __ATTR(device_name, 0444, device_name_show, NULL);
static struct kobj_attribute device_model_attr = __ATTR(device_model, 0444, device_model_show, NULL);

static struct attribute *attrs[] = {
	&device_name_attr.attr,
	&device_model_attr.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = attrs,
};

static struct kobject *device_kobj;
#endif

static inline void setup_camera_params(void) {
	switch (g_sec_current_device) {
	case SEC_A25:
		mcd_camera_rear_dual_cal = true;
		mcd_use_imx258_13mp_full_size = true;
		mcd_use_leds_flash_charging_voltage_control = true;
		mcd_use_ois_hall_data_for_vdis = true;
		mcd_apply_mirror_vertical_flip = true;
		mcd_front_otprom_eeprom = true;
		mcd_modify_cal_map_for_swremosaic_lib = true;
		mcd_simplify_ois_init = true;
		mcd_use_camera_adaptive_mipi = true;
		mcd_read_dual_cal_firmware_data = true;
		mcd_camera_use_aois = true;
		break;
	case SEC_A26XS:
		mcd_camera_rear_dual_cal = true;
		mcd_use_imx258_13mp_full_size = true;
		mcd_use_leds_flash_charging_voltage_control = true;
		mcd_use_ois_hall_data_for_vdis = true;
		mcd_apply_mirror_vertical_flip = true;
		mcd_front_otprom_eeprom = true;
		mcd_modify_cal_map_for_swremosaic_lib = true;
		mcd_simplify_ois_init = true;
		mcd_use_camera_adaptive_mipi = true;
		mcd_read_dual_cal_firmware_data = true;
		mcd_camera_use_aois = true;
		mcd_cal_for_hw_ggc_a26x = true;
		mcd_use_ois_reset_autotest = true;
		mcd_ois_angle_support = true;
		break;
	case SEC_A33:
		mcd_camera_front_fixed_focus = true;
		mcd_camera_rear_dual_cal = true;
		mcd_config_check_hw_version_for_mcu_fw_upload = true;
		mcd_disable_dual_sync = true;
		mcd_use_imx258_13mp_full_size = true;
		mcd_use_leds_flash_charging_voltage_control = true;
		mcd_use_ois_hall_data_for_vdis = true;
		mcd_camera_uwide_dualized = true;
		mcd_read_dual_cal_firmware_data = true;
		break;
	case SEC_A53:
		mcd_camera_front_fixed_focus = true;
		mcd_camera_rear_dual_cal = true;
		mcd_disable_dual_sync = true;
		mcd_use_ois_hall_data_for_vdis = true;
		mcd_camera_uwide_dualized = true;
		mcd_read_dual_cal_firmware_data = true;
		mcd_use_leds_flash_charging_voltage_control = true;
		mcd_use_hi1336c_setfile = true;
		break;
	case SEC_M33:
		mcd_camera_rear_dual_cal = true;
		mcd_disable_dual_sync = true;
		mcd_modify_cal_map_for_swremosaic_lib = true;
		mcd_use_camera_adaptive_mipi = true;
		mcd_use_camera_act_driver_soft_landing = true;
		mcd_read_dual_cal_firmware_data = true;
		break;
	case SEC_M34:
		mcd_camera_rear_dual_cal = true;
		mcd_use_ois_hall_data_for_vdis = true;
		mcd_apply_mirror_vertical_flip = true;
		mcd_modify_cal_map_for_swremosaic_lib = true;
		mcd_simplify_ois_init = true;
		mcd_use_camera_adaptive_mipi = true;
		mcd_read_dual_cal_firmware_data = true;
		mcd_use_leds_flash_charging_voltage_control = true;	// Not enabled in stock, but required here?
		mcd_camera_use_aois = true;
		break;
	case SEC_GTA4XLS:
		mcd_use_camera_act_driver_soft_landing = true;
		break;
	default:
		break;
	}
}

// New function to print machine name and sec_ variables
static inline void print_sec_variables(const char *machine_name) {
	SEC_DETECT_LOG("Current machine name: %s\n", machine_name);
	SEC_DETECT_LOG("g_sec_needs_blic = %s\n", g_sec_needs_blic ? "true" : "false");
	SEC_DETECT_LOG("g_sec_needs_decon = %s\n", g_sec_needs_decon ? "true" : "false");
	SEC_DETECT_LOG("g_sec_doze = %s\n", g_sec_doze ? "true" : "false");
	SEC_DETECT_LOG("g_sec_lcd_device = %s\n", g_sec_lcd_device ? "true" : "false");
	SEC_DETECT_LOG("g_sec_legacy_sinput = %s\n", g_sec_legacy_sinput ? "true" : "false");
	SEC_DETECT_LOG("g_sec_legacy_usbpd = %s\n", g_sec_legacy_usbpd ? "true" : "false");
	SEC_DETECT_LOG("g_sec_slsi_usbpd = %s\n", g_sec_slsi_usbpd ? "true" : "false");
}

static int __init sec_detect_init(void) {
	struct device_node *root;
	const char *machine_name;
	int retval = 0;
#ifdef CONFIG_SEC_DETECT_SYSFS
	int sysfs_ret = 0;
#endif

	root = of_find_node_by_path("/");
	if (!root) {
		SEC_DETECT_LOG("Failed to find device tree root\n");
		smp_wmb();
		g_detection_complete = true;
		retval = -ENOENT;
		goto exit_no_root;
	}

	machine_name = of_get_property(root, "model", NULL);
	if (!machine_name)
		machine_name = of_get_property(root, "compatible", NULL);

	if (!machine_name) {
		SEC_DETECT_LOG("Failed to find machine name\n");
		smp_wmb();
		g_detection_complete = true;
		retval = -ENOENT;
		goto exit_put_root;
	}

	if (strstr(machine_name, "A25") != NULL) {
		g_sec_current_device = SEC_A25;
		strscpy(g_sec_current_device_name, "a25x", sizeof(g_sec_current_device_name));
		g_sec_needs_decon = false;
		g_sec_doze = true;
		g_sec_slsi_usbpd = true;
	} else if (strstr(machine_name, "A26XS") != NULL) {
		g_sec_current_device = SEC_A26XS;
		strscpy(g_sec_current_device_name, "a26xs", sizeof(g_sec_current_device_name));
		g_sec_needs_decon = false;
		g_sec_doze = true;
		g_sec_slsi_usbpd = true;
	} else if (strstr(machine_name, "A33") != NULL) {
		g_sec_current_device = SEC_A33;
		strscpy(g_sec_current_device_name, "a33x", sizeof(g_sec_current_device_name));
		g_sec_needs_decon = true;
		g_sec_legacy_usbpd = true;
		g_sec_slsi_usbpd = true;
	} else if (strstr(machine_name, "A53") != NULL) {
		g_sec_current_device = SEC_A53;
		strscpy(g_sec_current_device_name, "a53x", sizeof(g_sec_current_device_name));
		g_sec_needs_decon = true;
		g_sec_doze = true;
	} else if (strstr(machine_name, "M33") != NULL) {
		g_sec_current_device = SEC_M33;
		strscpy(g_sec_current_device_name, "m33x", sizeof(g_sec_current_device_name));
		g_sec_needs_decon = true;
		g_sec_needs_blic = true;
		g_sec_lcd_device = true;
		g_sec_legacy_sinput = true;
	} else if (strstr(machine_name, "M34") != NULL) {
		g_sec_current_device = SEC_M34;
		strscpy(g_sec_current_device_name, "m34x", sizeof(g_sec_current_device_name));
		g_sec_needs_decon = false;
		g_sec_doze = true;
	} else if (strstr(machine_name, "GTA4XLS") != NULL) {
		g_sec_current_device = SEC_GTA4XLS;
		strscpy(g_sec_current_device_name, "gta4xls", sizeof(g_sec_current_device_name));
		g_sec_needs_decon = false;
		g_sec_needs_blic = true;
	}

	// Print machine name and sec_ variables
	print_sec_variables(machine_name);

#ifdef CONFIG_SEC_DETECT_SYSFS
	// Create the sysfs entry
	device_kobj = kobject_create_and_add("sec_detect", kernel_kobj);
	if (!device_kobj) {
		SEC_DETECT_LOG("Failed to create sysfs kobject\n");
		retval = -ENOMEM;
		goto exit_put_root;
	}

	sysfs_ret = sysfs_create_group(device_kobj, &attr_group);
	if (sysfs_ret) {
		SEC_DETECT_LOG("Failed to create sysfs group, error %d\n", sysfs_ret);
		kobject_put(device_kobj);
		device_kobj = NULL;
	}
#endif

	setup_camera_params();
exit_put_root:
	of_node_put(root);
exit_no_root:
	smp_wmb();
	g_detection_complete = true;
	if (!retval)
		SEC_DETECT_LOG("Initialization complete and ready.\n");

	return retval;
}

static void __exit sec_detect_exit(void) {
#ifdef CONFIG_SEC_DETECT_SYSFS
	kobject_put(device_kobj);
#endif
	return;
}

rootfs_initcall(sec_detect_init); // runs before regular drivers init
module_exit(sec_detect_exit);

MODULE_AUTHOR("Flopster101");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Detects the Samsung device currently running this kernel. Also exposes device information through sysfs.");
