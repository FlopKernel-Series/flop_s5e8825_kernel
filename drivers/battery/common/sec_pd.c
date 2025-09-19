/*
 * Copyrights (C) 2017 Samsung Electronics, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/battery/sec_pd.h>

#if defined(CONFIG_SEC_KUNIT)
#include <kunit/mock.h>

SEC_PD_SINK_STATUS *g_psink_status;
EXPORT_SYMBOL(g_psink_status);
#else
static SEC_PD_SINK_STATUS *g_psink_status;
#endif

#if defined(CONFIG_ARCH_MTK_PROJECT) || IS_ENABLED(CONFIG_SEC_MTK_CHARGER)
struct pdic_notifier_struct pd_noti;
EXPORT_SYMBOL(pd_noti);
#endif

#include "sec_pd.h"
#include "sec_pd_data.h"

const char* sec_pd_pdo_type_str(int pdo_type)
{
	switch (pdo_type) {
	case FPDO_TYPE:
		return "FIXED";
	case APDO_TYPE:
		return "APDO";
	case VPDO_TYPE:
		return "VPDO";
	default:
		return "UNKNOWN";
	}
}
EXPORT_SYMBOL(sec_pd_pdo_type_str);

int sec_pd_select_pdo(int num)
{
	return op_select_pdo(get_sec_pd(), num);
}
EXPORT_SYMBOL(sec_pd_select_pdo);

int sec_pd_select_pps(int num, int ppsVol, int ppsCur)
{
	return op_select_pps(get_sec_pd(), num, ppsVol, ppsCur);
}
EXPORT_SYMBOL(sec_pd_select_pps);

int sec_pd_get_current_pdo(unsigned int *pdo)
{
	if (pdo == NULL) {
		pr_err("%s: invalid argument\n", __func__);
		return -EINVAL;
	}

	return op_get_current_pdo(get_sec_pd(), pdo);
}
EXPORT_SYMBOL(sec_pd_get_current_pdo);

int sec_pd_get_selected_pdo(unsigned int *pdo)
{
	if (pdo == NULL) {
		pr_err("%s: invalid argument\n", __func__);
		return -EINVAL;
	}

	return op_get_selected_pdo(get_sec_pd(), pdo);
}
EXPORT_SYMBOL(sec_pd_get_selected_pdo);

int sec_pd_is_apdo(unsigned int pdo)
{
	return op_is_apdo(get_sec_pd(), pdo);
}
EXPORT_SYMBOL(sec_pd_is_apdo);

int sec_pd_detach_with_cc(int state)
{
	if (!g_psink_status) {
		pr_err("%s: g_psink_status is NULL\n", __func__);
		return -1;
	}

	if (!g_psink_status->fp_sec_pd_detach_with_cc) {
		pr_err("%s: not exist\n", __func__);
		return -1;
	}

	g_psink_status->fp_sec_pd_detach_with_cc(state);

	return 0;
}
EXPORT_SYMBOL(sec_pd_detach_with_cc);

int sec_pd_get_apdo_prog_volt(unsigned int pdo_type, unsigned int max_volt)
{
	return op_get_apdo_prog_volt(get_sec_pd(), pdo_type, max_volt);
}
EXPORT_SYMBOL(sec_pd_get_apdo_prog_volt);

int sec_pd_get_max_power(unsigned int pdo_type, unsigned int min_volt, unsigned int max_volt, unsigned int max_curr)
{
	return op_get_max_power(get_sec_pd(), pdo_type, min_volt, max_volt, max_curr);
}
EXPORT_SYMBOL(sec_pd_get_max_power);

int sec_pd_get_pdo_power(unsigned int *pdo, unsigned int *min_volt, unsigned int *max_volt, unsigned int *max_curr)
{
	return op_get_pdo_power(get_sec_pd(), pdo, min_volt, max_volt, max_curr);
}
EXPORT_SYMBOL(sec_pd_get_pdo_power);

int sec_pd_vpdo_auth(int auth, int d2d_type)
{
	if (!g_psink_status) {
		pr_err("%s: g_psink_status is NULL\n", __func__);
		return -1;
	}

	if (!g_psink_status->fp_sec_pd_vpdo_auth) {
		pr_err("%s: not exist\n", __func__);
		return -1;
	}

	g_psink_status->fp_sec_pd_vpdo_auth(auth, d2d_type);

	return 0;
}
EXPORT_SYMBOL(sec_pd_vpdo_auth);

int sec_pd_change_src(int max_cur)
{
	if (!g_psink_status) {
		pr_err("%s: g_psink_status is NULL\n", __func__);
		return -1;
	}

	if (!g_psink_status->fp_sec_pd_change_src) {
		pr_err("%s: not exist\n", __func__);
		return -1;
	}

	g_psink_status->fp_sec_pd_change_src(max_cur);

	return 0;
}
EXPORT_SYMBOL(sec_pd_change_src);

int sec_pd_get_apdo_max_power(unsigned int *pdo_pos, unsigned int *taMaxVol, unsigned int *taMaxCur, unsigned int *taMaxPwr)
{
	if ((pdo_pos == NULL) || (taMaxVol == NULL) || (taMaxCur == NULL) || (taMaxPwr == NULL)) {
		pr_err("%s: invalid argument\n", __func__);
		return -EINVAL;
	}

	return op_get_apdo_max_power(get_sec_pd(), pdo_pos, taMaxVol, taMaxCur, taMaxPwr);
}
EXPORT_SYMBOL(sec_pd_get_apdo_max_power);

int sec_pd_set_pd_voltage(int volt)
{
	return op_set_pd_voltage(get_sec_pd(), volt);
}
EXPORT_SYMBOL(sec_pd_set_pd_voltage);

void sec_pd_init_data(SEC_PD_SINK_STATUS* psink_status)
{
	g_psink_status = psink_status;
	if (g_psink_status)
		pr_info("%s: done.\n", __func__);
	else
		pr_err("%s: g_psink_status is NULL\n", __func__);

	op_init(get_sec_pd(), psink_status);
}
EXPORT_SYMBOL(sec_pd_init_data);

int sec_pd_register_chg_info_cb(void *cb)
{
	if (!g_psink_status) {
		pr_err("%s: g_psink_status is NULL\n", __func__);
		return -1;
	}
	g_psink_status->fp_sec_pd_ext_cb = cb;

	return 0;
}
EXPORT_SYMBOL(sec_pd_register_chg_info_cb);

void sec_pd_get_vid(unsigned short *vid)
{
	if (!g_psink_status) {
		pr_err("%s: g_psink_status is NULL\n", __func__);
		return;
	}
	*vid = g_psink_status->vid;
}
EXPORT_SYMBOL(sec_pd_get_vid);

void sec_pd_get_vid_pid(unsigned short *vid, unsigned short *pid, unsigned int *xid)
{
	if (!g_psink_status) {
		pr_err("%s: g_psink_status is NULL\n", __func__);
		return;
	}
	*vid = g_psink_status->vid;
	*pid = g_psink_status->pid;
	*xid = g_psink_status->xid;
}
EXPORT_SYMBOL(sec_pd_get_vid_pid);

void sec_pd_manual_ccopen_req(int is_on)
{
	if (!g_psink_status) {
		pr_err("%s: g_psink_status is NULL\n", __func__);
		return;
	}

	if (!g_psink_status->fp_sec_pd_manual_ccopen_req) {
		pr_err("%s: not exist\n", __func__);
		return;
	}

	g_psink_status->fp_sec_pd_manual_ccopen_req(is_on);
}
EXPORT_SYMBOL(sec_pd_manual_ccopen_req);

void sec_pd_manual_jig_ctrl(bool mode)
{
	if (!g_psink_status) {
		pr_err("%s: g_psink_status is NULL\n", __func__);
		return;
	}

	if (!g_psink_status->fp_sec_pd_manual_jig_ctrl) {
		pr_err("%s: not exist\n", __func__);
		return;
	}

	g_psink_status->fp_sec_pd_manual_jig_ctrl(mode);
}
EXPORT_SYMBOL(sec_pd_manual_jig_ctrl);

bool sec_pd_enable_rp_current_wa(void)
{
	return get_sec_pd_rp_current_wa_state();
}
EXPORT_SYMBOL(sec_pd_enable_rp_current_wa);

static int __init sec_pd_init(void)
{
	struct sec_pd *pd = NULL;

	pd = init_sec_pd();
	pr_info("%s: OP = %s\n", __func__, get_sec_pd_op_name(pd));

	sec_pd_eng_init(pd);
	return 0;
}

module_init(sec_pd_init);
MODULE_DESCRIPTION("Samsung PD control");
MODULE_AUTHOR("Samsung Electronics");
MODULE_LICENSE("GPL");
