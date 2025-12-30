/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Namespace header for DECON panel driver.
 * Included via Makefile -include to prevent symbol conflicts with USDM.
 */

#ifndef _DECON_PANEL_NAMESPACE_H_
#define _DECON_PANEL_NAMESPACE_H_

/* API & TUI */
#define get_lcd_info decon_get_lcd_info
#define exynos_atomic_enter_tui decon_exynos_atomic_enter_tui
#define exynos_atomic_exit_tui decon_exynos_atomic_exit_tui
#define exynos_tui_set_stui_funcs decon_exynos_tui_set_stui_funcs
#define exynos_tui_get_resolution decon_exynos_tui_get_resolution
#define exynos_tui_get_panel_info decon_exynos_tui_get_panel_info

/* Utilities */
#define copy_from_sliced_byte_array decon_copy_from_sliced_byte_array
#define copy_to_sliced_byte_array decon_copy_to_sliced_byte_array
#define hextos32 decon_hextos32
#define s32tohex decon_s32tohex
#define of_get_panel_power_ctrl decon_of_get_panel_power_ctrl

/* maptbl */
#define maptbl_create decon_maptbl_create
#define maptbl_clone decon_maptbl_clone
#define maptbl_deepcopy decon_maptbl_deepcopy
#define maptbl_destroy decon_maptbl_destroy
#define maptbl_get_indexof_n_dimen_element                                     \
	decon_maptbl_get_indexof_n_dimen_element
#define maptbl_get_indexof_box decon_maptbl_get_indexof_box
#define maptbl_get_indexof_layer decon_maptbl_get_indexof_layer
#define maptbl_get_indexof_row decon_maptbl_get_indexof_row
#define maptbl_get_indexof_col decon_maptbl_get_indexof_col
#define maptbl_4d_index decon_maptbl_4d_index
#define maptbl_index decon_maptbl_index
#define maptbl_pos_to_index decon_maptbl_pos_to_index
#define maptbl_index_to_pos decon_maptbl_index_to_pos
#define maptbl_is_initialized decon_maptbl_is_initialized
#define maptbl_is_index_in_bound decon_maptbl_is_index_in_bound
#define maptbl_init decon_maptbl_init
#define maptbl_getidx decon_maptbl_getidx
#define maptbl_copy decon_maptbl_copy
#define maptbl_fill decon_maptbl_fill
#define maptbl_cmp_shape decon_maptbl_cmp_shape
#define maptbl_memcpy decon_maptbl_memcpy
#define maptbl_print decon_maptbl_print

/* panel_bl */
#define is_hbm_brightness decon_is_hbm_brightness
#define get_subdev_actual_brightness_index                                     \
	decon_get_subdev_actual_brightness_index
#define get_actual_brightness_index decon_get_actual_brightness_index
#define get_brightness_pac_step_by_subdev_id                                   \
	decon_get_brightness_pac_step_by_subdev_id
#define get_brightness_pac_step decon_get_brightness_pac_step
#define get_actual_brightness decon_get_actual_brightness
#define panel_bl_get_acl_pwrsave decon_panel_bl_get_acl_pwrsave
#define panel_bl_get_acl_opr decon_panel_bl_get_acl_opr
#define panel_bl_get_brightness_set_count                                      \
	decon_panel_bl_get_brightness_set_count
#define panel_bl_aor_interpolation_2 decon_panel_bl_aor_interpolation_2
#define panel_bl_irc_interpolation decon_panel_bl_irc_interpolation

/* copr */
#define get_copr_reg_size decon_get_copr_reg_size
#define get_copr_reg_packed_size decon_get_copr_reg_packed_size
#define get_copr_reg_name decon_get_copr_reg_name
#define get_copr_reg_offset decon_get_copr_reg_offset
#define get_copr_reg_ptr decon_get_copr_reg_ptr
#define find_copr_reg_by_name decon_find_copr_reg_by_name
#define copr_reg_to_byte_array decon_copr_reg_to_byte_array

/* DPU helper */
#define compare_exynos_panel_mode decon_compare_exynos_panel_mode
#define exynos_mode_snprintf decon_exynos_mode_snprintf
#define exynos_mode_debug_printmodeline decon_exynos_mode_debug_printmodeline
#define exynos_mode_info_printmodeline decon_exynos_mode_info_printmodeline
#define exynos_panel_mode_snprintf decon_exynos_panel_mode_snprintf
#define exynos_panel_mode_debug_printmodeline                                  \
	decon_exynos_panel_mode_debug_printmodeline
#define exynos_panel_mode_info_printmodeline                                   \
	decon_exynos_panel_mode_info_printmodeline
#define exynos_panel_find_panel_mode decon_exynos_panel_find_panel_mode
#define exynos_panel_mode_create decon_exynos_panel_mode_create
#define exynos_panel_mode_destroy decon_exynos_panel_mode_destroy
#define exynos_panel_desc_create decon_exynos_panel_desc_create
#define exynos_panel_desc_destroy decon_exynos_panel_desc_destroy
#define exynos_panel_desc_create_from_panel_display_modes                      \
	decon_exynos_panel_desc_create_from_panel_display_modes
#define exynos_drm_mode_set_name decon_exynos_drm_mode_set_name
#define drm_display_mode_from_panel_display_mode                               \
	decon_drm_display_mode_from_panel_display_mode
#define exynos_display_mode_from_panel_display_mode                            \
	decon_exynos_display_mode_from_panel_display_mode
#define exynos_panel_mode_from_panel_display_mode                              \
	decon_exynos_panel_mode_from_panel_display_mode

/* dpu */
#define get_panel_log_level decon_get_panel_log_level
#define mcd_drm_panel_get_modes decon_mcd_drm_panel_get_modes
#define exynos_panel_set_lp_mode decon_exynos_panel_set_lp_mode
#define exynos_drm_cmdset_add decon_exynos_drm_cmdset_add
#define exynos_drm_cmdset_cleanup decon_exynos_drm_cmdset_cleanup
#define exynos_drm_cmdset_flush decon_exynos_drm_cmdset_flush
#define drm_mipi_fcmd_write decon_drm_mipi_fcmd_write
#define exynos_panel_probe decon_exynos_panel_probe
#define exynos_panel_remove decon_exynos_panel_remove

/* dsim */
#define dsim_host_cmdset_transfer decon_dsim_host_cmdset_transfer
#define dsim_atomic_activate decon_dsim_atomic_activate
#define dsim_reg_set_cmd_transfer_mode decon_dsim_reg_set_cmd_transfer_mode
#define dsim_host_fcmd_transfer decon_dsim_host_fcmd_transfer
#define mcd_dsim_update_dsi_freq decon_mcd_dsim_update_dsi_freq

/* dpp */
#define exynos_dpuf_set_votf decon_exynos_dpuf_set_votf
#define of_find_dpp_by_node decon_of_find_dpp_by_node

/* recovery & debug */
#define exynos_recovery_register decon_exynos_recovery_register
#define g_log_obj decon_g_log_obj
#define g_errlog_obj decon_g_errlog_obj
#define no_display decon_no_display
#define bypass_display decon_bypass_display
#define commit_retry decon_commit_retry

/* tui */
#define is_tui_trans decon_is_tui_trans

/* dp */
#define dp_drvdata decon_dp_drvdata
#define dp_ado_notifier_head decon_dp_ado_notifier_head
#define dp_audio_config decon_dp_audio_config
#define dp_hdcp22_enable decon_dp_hdcp22_enable
#define dp_dpcd_write_for_hdcp22 decon_dp_dpcd_write_for_hdcp22
#define dp_dpcd_read_for_hdcp22 decon_dp_dpcd_read_for_hdcp22

/* helper */
#define mcd_decon_get_bts_fps decon_mcd_decon_get_bts_fps

/* connector */
#define exynos_drm_boost_bts_fps decon_exynos_drm_boost_bts_fps
#define exynos_drm_connector_get_properties                                    \
	decon_exynos_drm_connector_get_properties
#define exynos_drm_connector_init decon_exynos_drm_connector_init

/* crtc */
#define exynos_drm_crtc_add_crc_entry decon_exynos_drm_crtc_add_crc_entry

/* dp_ext_func */
#define secdp_bigdata_init decon_secdp_bigdata_init
#define secdp_bigdata_save_item decon_secdp_bigdata_save_item
#define secdp_bigdata_inc_error_cnt decon_secdp_bigdata_inc_error_cnt
#define secdp_bigdata_clr_error_cnt decon_secdp_bigdata_clr_error_cnt
#define secdp_bigdata_connection decon_secdp_bigdata_connection
#define secdp_bigdata_disconnection decon_secdp_bigdata_disconnection
#define dp_logger_set_max_count decon_dp_logger_set_max_count
#define dp_logger_print decon_dp_logger_print
#define dp_logger_hex_dump decon_dp_logger_hex_dump
#define dp_logger_init decon_dp_logger_init
#define self_test_get_dp_adapter_type decon_self_test_get_dp_adapter_type
#define self_test_on_process decon_self_test_on_process
#define self_test_get_edid decon_self_test_get_edid
#define self_test_resolution_update decon_self_test_resolution_update
#define self_test_init decon_self_test_init

/* panel_lib */
#define panel_lib_rdinfo_alloc_buffer decon_panel_lib_rdinfo_alloc_buffer
#define panel_lib_rdinfo_free_buffer decon_panel_lib_rdinfo_free_buffer
#define panel_lib_rdinfo_create decon_panel_lib_rdinfo_create
#define panel_lib_rdinfo_destroy decon_panel_lib_rdinfo_destroy
#define panel_lib_rdinfo_copy decon_panel_lib_rdinfo_copy
#define panel_lib_res_update_info_create decon_panel_lib_res_update_info_create
#define panel_lib_res_update_info_destroy                                      \
	decon_panel_lib_res_update_info_destroy
#define panel_lib_res_update_info_copy decon_panel_lib_res_update_info_copy
#define panel_lib_resinfo_alloc_buffer decon_panel_lib_resinfo_alloc_buffer
#define panel_lib_resinfo_free_buffer decon_panel_lib_resinfo_free_buffer
#define panel_lib_resinfo_create decon_panel_lib_resinfo_create
#define panel_lib_resinfo_destroy decon_panel_lib_resinfo_destroy
#define panel_lib_resinfo_copy decon_panel_lib_resinfo_copy
#define panel_lib_dumpinfo_create decon_panel_lib_dumpinfo_create
#define panel_lib_dumpinfo_destroy decon_panel_lib_dumpinfo_destroy
#define panel_lib_dumpinfo_copy decon_panel_lib_dumpinfo_copy

/* sysfs */
#define attr_store_for_each decon_attr_store_for_each
#define attr_show_for_each decon_attr_show_for_each
#define attr_exist_for_each decon_attr_exist_for_each

/* panel_drv */
#define panel_log_level decon_panel_log_level
#define panel_cmd_log decon_panel_cmd_log
#define panel_device_create decon_panel_device_create
#define panel_device_destroy decon_panel_device_destroy
#define panel_device_init decon_panel_device_init
#define panel_device_exit decon_panel_device_exit

/* panel_poc */
#define check_poc_partition_exists decon_check_poc_partition_exists
#define get_poc_partition_chksum decon_get_poc_partition_chksum
#define set_panel_poc decon_set_panel_poc

/* mdnie */
#define mdnie_get_maptbl_index decon_mdnie_get_maptbl_index
#define mdnie_find_maptbl decon_mdnie_find_maptbl
#define mdnie_find_etc_maptbl decon_mdnie_find_etc_maptbl
#define mdnie_set_def_wrgb decon_mdnie_set_def_wrgb
#define mdnie_set_cur_wrgb decon_mdnie_set_cur_wrgb
#define mdnie_cur_wrgb_to_byte_array decon_mdnie_cur_wrgb_to_byte_array
#define mdnie_update_wrgb decon_mdnie_update_wrgb

/* panel_regulator */
#define of_get_panel_regulator decon_of_get_panel_regulator
#define panel_regulator_create decon_panel_regulator_create
#define panel_regulator_destroy decon_panel_regulator_destroy

/* panel.c */
#define print_data decon_print_data
#define register_common_panel decon_register_common_panel
#define deregister_common_panel decon_deregister_common_panel
#define find_panel_maptbl_by_index decon_find_panel_maptbl_by_index
#define find_panel_maptbl_by_name decon_find_panel_maptbl_by_name
#define panel_do_seqtbl_by_index_nolock decon_panel_do_seqtbl_by_index_nolock
#define panel_do_seqtbl_by_index decon_panel_do_seqtbl_by_index
#define find_panel_resource decon_find_panel_resource
#define panel_resource_initialized decon_panel_resource_initialized
#define rescpy decon_rescpy
#define resource_copy decon_resource_copy
#define resource_copy_by_name decon_resource_copy_by_name
#define resource_copy_n_clear_by_name decon_resource_copy_n_clear_by_name
#define get_resource_size_by_name decon_get_resource_size_by_name
#define panel_resource_update_by_name decon_panel_resource_update_by_name

/* dimming */
#define disp_pow decon_disp_pow
#define disp_div64 decon_disp_div64
#define disp_interpolation64 decon_disp_interpolation64
#define gamma_table_interpolation_round decon_gamma_table_interpolation_round
#define get_dimming_gamma decon_get_dimming_gamma
#define init_dimming_info decon_init_dimming_info
#define init_dimming_mtp decon_init_dimming_mtp
#define process_dimming decon_process_dimming

/* panel_vrr */
#define get_panel_vrr decon_get_panel_vrr
#define get_panel_refresh_rate decon_get_panel_refresh_rate
#define get_panel_refresh_mode decon_get_panel_refresh_mode

/* panel_gpio */
#define of_get_panel_gpio decon_of_get_panel_gpio
#define panel_gpio_create decon_panel_gpio_create
#define panel_gpio_destroy decon_panel_gpio_destroy

/* dpui */
#define dpui_logging_notify decon_dpui_logging_notify
#define dpui_logging_register decon_dpui_logging_register
#define dpui_logging_unregister decon_dpui_logging_unregister
#define update_dpui_log decon_update_dpui_log
#define clear_dpui_log decon_clear_dpui_log
#define print_dpui_field decon_print_dpui_field
#define set_dpui_field decon_set_dpui_field
#define get_dpui_u32_field decon_get_dpui_u32_field
#define set_dpui_u32_field decon_set_dpui_u32_field
#define inc_dpui_u32_field decon_inc_dpui_u32_field
#define get_dpui_log decon_get_dpui_log

/* panel_modes */
#define panel_mode_debug_printmodeline decon_panel_mode_debug_printmodeline
#define panel_mode_info_printmodeline decon_panel_mode_info_printmodeline
#define refresh_mode_to_str decon_refresh_mode_to_str
#define str_to_refresh_mode decon_str_to_refresh_mode
#define panel_mode_vscan decon_panel_mode_vscan
#define panel_display_modes_release decon_panel_display_modes_release
#define of_get_panel_display_modes decon_of_get_panel_display_modes
#define panel_mode_set_name decon_panel_mode_set_name
#define panel_mode_create decon_panel_mode_create

#endif /* _DECON_PANEL_NAMESPACE_H_ */
