// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Samsung Electronics Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/fb.h>
#include <linux/notifier.h>
#include <linux/export.h>
#include "decon_dpui.h"
#include "decon_panel_debug.h"

/*
 * DPUI : display use info (panel common info)
 * DPCI : display controller info (ap dependent info)
 */
static BLOCKING_NOTIFIER_HEAD(dpui_notifier_list);
static BLOCKING_NOTIFIER_HEAD(dpci_notifier_list);
static DEFINE_MUTEX(dpui_lock);

static const char * const dpui_key_name[] = {
	[DPUI_KEY_NONE] = "NONE",
	/* common hw parameter */
	[DPUI_KEY_WCRD_X] = "WCRD_X",
	[DPUI_KEY_WCRD_Y] = "WCRD_Y",
	[DPUI_KEY_WOFS_R] = "WOFS_R",
	[DPUI_KEY_WOFS_G] = "WOFS_G",
	[DPUI_KEY_WOFS_B] = "WOFS_B",
	[DPUI_KEY_WOFS_R_ORG] = "WOFS_R_ORG",
	[DPUI_KEY_WOFS_G_ORG] = "WOFS_G_ORG",
	[DPUI_KEY_WOFS_B_ORG] = "WOFS_B_ORG",
	[DPUI_KEY_LCDID1] = "LCDM_ID1",
	[DPUI_KEY_LCDID2] = "LCDM_ID2",
	[DPUI_KEY_LCDID3] = "LCDM_ID3",
	[DPUI_KEY_MAID_DATE] = "MAID_DATE",
	[DPUI_KEY_CELLID] = "CELLID",
	[DPUI_KEY_OCTAID] = "OCTAID",
	[DPUI_KEY_PNDSIE] = "PNDSIE",
	[DPUI_KEY_PNELVDE] = "PNELVDE",
	[DPUI_KEY_PNVLI1E] = "PNVLI1E",
	[DPUI_KEY_PNVLO3E] = "PNVLO3E",
	[DPUI_KEY_PNSDRE] = "PNSDRE",
#ifdef CONFIG_SUPPORT_POC_FLASH
	[DPUI_KEY_PNPOCT] = "PNPOCT",
	[DPUI_KEY_PNPOCF] = "PNPOCF",
	[DPUI_KEY_PNPOCI] = "PNPOCI",
	[DPUI_KEY_PNPOCI_ORG] = "PNPOCI_ORG",
	[DPUI_KEY_PNPOC_ER_TRY] = "PNPOC_ER_T",
	[DPUI_KEY_PNPOC_ER_FAIL] = "PNPOC_ER_F",
	[DPUI_KEY_PNPOC_WR_TRY] = "PNPOC_WR_T",
	[DPUI_KEY_PNPOC_WR_FAIL] = "PNPOC_WR_F",
	[DPUI_KEY_PNPOC_RD_TRY] = "PNPOC_RD_T",
	[DPUI_KEY_PNPOC_RD_FAIL] = "PNPOC_RD_F",
#endif
#ifdef CONFIG_SUPPORT_DIM_FLASH
	[DPUI_KEY_PNGFLS] = "PNGFLS",
#endif
	[DPUI_KEY_UB_CON] = "UB_CON",

	/* dependent on processor */
	[DPUI_KEY_EXY_SWRCV] = "EXY_SWRCV",
};

static const char * const dpui_type_name[] = {
	[DPUI_TYPE_NONE] = "NONE",
	/* common hw parameter */
	[DPUI_TYPE_PANEL] = "PANEL",
	/* dependent on processor */
	[DPUI_TYPE_CTRL] = "CTRL",
};

static struct dpui_info dpui = {
	.pdata = NULL,
	.field = {
		/* common hw parameter */
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_VAR_U32, DPUI_AUTO_CLEAR_OFF, "0", DPUI_KEY_WCRD_X),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_VAR_U32, DPUI_AUTO_CLEAR_OFF, "0", DPUI_KEY_WCRD_Y),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_VAR_S32, DPUI_AUTO_CLEAR_OFF, "0", DPUI_KEY_WOFS_R),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_VAR_S32, DPUI_AUTO_CLEAR_OFF, "0", DPUI_KEY_WOFS_G),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_VAR_S32, DPUI_AUTO_CLEAR_OFF, "0", DPUI_KEY_WOFS_B),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_VAR_S32, DPUI_AUTO_CLEAR_OFF, "0", DPUI_KEY_WOFS_R_ORG),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_VAR_S32, DPUI_AUTO_CLEAR_OFF, "0", DPUI_KEY_WOFS_G_ORG),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_VAR_S32, DPUI_AUTO_CLEAR_OFF, "0", DPUI_KEY_WOFS_B_ORG),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_VAR_S32, DPUI_AUTO_CLEAR_OFF, "-1", DPUI_KEY_LCDID1),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_VAR_S32, DPUI_AUTO_CLEAR_OFF, "-1", DPUI_KEY_LCDID2),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_VAR_S32, DPUI_AUTO_CLEAR_OFF, "-1", DPUI_KEY_LCDID3),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_VAR_STR, DPUI_AUTO_CLEAR_OFF, "19000000 000000", DPUI_KEY_MAID_DATE),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_VAR_STR, DPUI_AUTO_CLEAR_OFF, "0000000000000000000000", DPUI_KEY_CELLID),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_VAR_STR, DPUI_AUTO_CLEAR_OFF, "00000000000000000000000", DPUI_KEY_OCTAID),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_VAR_U32, DPUI_AUTO_CLEAR_ON, "0", DPUI_KEY_PNDSIE),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_VAR_U32, DPUI_AUTO_CLEAR_ON, "0", DPUI_KEY_PNELVDE),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_VAR_U32, DPUI_AUTO_CLEAR_ON, "0", DPUI_KEY_PNVLI1E),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_VAR_U32, DPUI_AUTO_CLEAR_ON, "0", DPUI_KEY_PNVLO3E),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_VAR_U32, DPUI_AUTO_CLEAR_ON, "0", DPUI_KEY_PNSDRE),
#ifdef CONFIG_SUPPORT_POC_FLASH
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_VAR_S32, DPUI_AUTO_CLEAR_OFF, "-1", DPUI_KEY_PNPOCT),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_VAR_S32, DPUI_AUTO_CLEAR_OFF, "-1", DPUI_KEY_PNPOCF),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_VAR_S32, DPUI_AUTO_CLEAR_OFF, "-100", DPUI_KEY_PNPOCI),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_VAR_S32, DPUI_AUTO_CLEAR_OFF, "-100", DPUI_KEY_PNPOCI_ORG),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_VAR_U32, DPUI_AUTO_CLEAR_ON, "0", DPUI_KEY_PNPOC_ER_TRY),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_VAR_U32, DPUI_AUTO_CLEAR_ON, "0", DPUI_KEY_PNPOC_ER_FAIL),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_VAR_U32, DPUI_AUTO_CLEAR_ON, "0", DPUI_KEY_PNPOC_WR_TRY),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_VAR_U32, DPUI_AUTO_CLEAR_ON, "0", DPUI_KEY_PNPOC_WR_FAIL),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_VAR_U32, DPUI_AUTO_CLEAR_ON, "0", DPUI_KEY_PNPOC_RD_TRY),
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_VAR_U32, DPUI_AUTO_CLEAR_ON, "0", DPUI_KEY_PNPOC_RD_FAIL),
#endif
#ifdef CONFIG_SUPPORT_DIM_FLASH
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_VAR_S32, DPUI_AUTO_CLEAR_OFF, "0", DPUI_KEY_PNGFLS),
#endif
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_PANEL, DPUI_VAR_U32, DPUI_AUTO_CLEAR_ON, "0", DPUI_KEY_UB_CON),

		/* dependent on processor */
		DPUI_FIELD_INIT(DPUI_LOG_LEVEL_INFO, DPUI_TYPE_CTRL, DPUI_VAR_U32, DPUI_AUTO_CLEAR_ON, "0", DPUI_KEY_EXY_SWRCV),
	},
};

/**
 * decon_dpui_logging_notify - notify clients of fb_events
 * @val: dpui log type
 * @v : data
 *
 */
void decon_dpui_logging_notify(unsigned long val, enum dpui_type type, void *v)
{
	if (type == DPUI_TYPE_CTRL)
		blocking_notifier_call_chain(&dpci_notifier_list, val, v);
	else
		blocking_notifier_call_chain(&dpui_notifier_list, val, v);
}
EXPORT_SYMBOL_GPL(decon_dpui_logging_notify);

/**
 *	decon_dpui_logging_register - register a client notifier
 *	@n: notifier block to callback on events
 */
int decon_dpui_logging_register(struct notifier_block *n, enum dpui_type type)
{
	int ret;

	if (type <= DPUI_TYPE_NONE || type >= MAX_DPUI_TYPE) {
		panel_err("out of dpui_type range (%d)\n", type);
		return -EINVAL;
	}

	if (type == DPUI_TYPE_CTRL)
		ret = blocking_notifier_chain_register(&dpci_notifier_list, n);
	else
		ret = blocking_notifier_chain_register(&dpui_notifier_list, n);
	if (ret < 0) {
		panel_err("blocking_notifier_chain_register error(%d)\n", ret);
		return ret;
	}

	panel_info("register type %s\n", dpui_type_name[type]);
	return 0;
}
EXPORT_SYMBOL_GPL(decon_dpui_logging_register);

/**
 *	decon_dpui_logging_unregister - unregister a client notifier
 *	@n: notifier block to callback on events
 */
int decon_dpui_logging_unregister(struct notifier_block *n)
{
	return blocking_notifier_chain_unregister(&dpui_notifier_list, n);
}
EXPORT_SYMBOL_GPL(decon_dpui_logging_unregister);

static bool is_dpui_var_u32(enum dpui_key key)
{
	return (dpui.field[key].var_type == DPUI_VAR_U32);
}

void decon_update_dpui_log(enum dpui_log_level level, enum dpui_type type)
{
	if (level < 0 || level >= MAX_DPUI_LOG_LEVEL) {
		panel_err("invalid log level %d\n", level);
		return;
	}

	decon_dpui_logging_notify(level, type, &dpui);
	panel_info("update dpui log(%d) done\n", level);
}
EXPORT_SYMBOL_GPL(decon_update_dpui_log);

void decon_clear_dpui_log(enum dpui_log_level level, enum dpui_type type)
{
	size_t i;

	if (level < 0 || level >= MAX_DPUI_LOG_LEVEL) {
		panel_err("invalid log level %d\n", level);
		return;
	}

	mutex_lock(&dpui_lock);
	for (i = 0; i < ARRAY_SIZE(dpui.field); i++) {
		if (dpui.field[i].type != type)
			continue;
		if (dpui.field[i].auto_clear)
			dpui.field[i].initialized = false;
	}
	mutex_unlock(&dpui_lock);

	panel_info("clear dpui log(%d) done\n", level);
}
EXPORT_SYMBOL_GPL(decon_clear_dpui_log);

static int __get_dpui_field(enum dpui_key key, char *buf)
{
	if (!buf) {
		panel_err("buf is null\n");
		return 0;
	}

	if (!DPUI_VALID_KEY(key)) {
		panel_err("out of dpui_key range (%d)\n", key);
		return 0;
	}

	if (!dpui.field[key].initialized) {
		panel_dbg("%s not initialized, so use default value\n", dpui_key_name[key]);
		return snprintf(buf, MAX_DPUI_KEY_LEN + MAX_DPUI_VAL_LEN,
			"\"%s\":\"%s\"", dpui_key_name[key], dpui.field[key].default_value);
	}

	return snprintf(buf, MAX_DPUI_KEY_LEN + MAX_DPUI_VAL_LEN,
			"\"%s\":\"%s\"", dpui_key_name[key], dpui.field[key].buf);
}

void decon_print_dpui_field(enum dpui_key key)
{
	char tbuf[MAX_DPUI_KEY_LEN + MAX_DPUI_VAL_LEN];

	if (!DPUI_VALID_KEY(key)) {
		panel_err("out of dpui_key range (%d)\n", key);
		return;
	}

	__get_dpui_field(key, tbuf);
	panel_info("%s\n", tbuf);
}
EXPORT_SYMBOL_GPL(decon_print_dpui_field);

static int __decon_set_dpui_field(enum dpui_key key, char *buf, int size)
{
	if (!buf) {
		panel_err("buf is null\n");
		return -EINVAL;
	}

	if (!DPUI_VALID_KEY(key)) {
		panel_err("out of dpui_key range (%d)\n", key);
		return -EINVAL;
	}

	if (size > MAX_DPUI_VAL_LEN - 1) {
		panel_err("exceed dpui value size (%d)\n", size);
		return -EINVAL;
	}
	memcpy(dpui.field[key].buf, buf, size);
	dpui.field[key].buf[size] = '\0';
	dpui.field[key].initialized = true;

	return 0;
}

static int __decon_get_dpui_u32_field(enum dpui_key key, u32 *value)
{
	int rc, cur_val;

	if (value == NULL) {
		panel_err("invalid value pointer\n");
		return -EINVAL;
	}

	if (!DPUI_VALID_KEY(key)) {
		panel_err("out of dpui_key range (%d)\n", key);
		return -EINVAL;
	}

	rc = kstrtouint(dpui.field[key].buf, 0, &cur_val);
	if (rc < 0) {
		panel_err("failed to get value\n");
		return rc;
	}

	*value = cur_val;

	return 0;
}

static int __decon_set_dpui_u32_field(enum dpui_key key, u32 value)
{
	char tbuf[MAX_DPUI_VAL_LEN];
	int size;

	if (!DPUI_VALID_KEY(key)) {
		panel_err("out of dpui_key range (%d)\n", key);
		return -EINVAL;
	}

	if (!is_dpui_var_u32(key)) {
		panel_err("invalid type %d\n", dpui.field[key].var_type);
		return -EINVAL;
	}

	size = snprintf(tbuf, MAX_DPUI_VAL_LEN, "%u", value);
	if (size > MAX_DPUI_VAL_LEN) {
		panel_err("exceed dpui value size (%d)\n", size);
		return -EINVAL;
	}
	__decon_set_dpui_field(key, tbuf, size);

	return 0;
}

static int __decon_inc_dpui_u32_field(enum dpui_key key, u32 value)
{
	int ret;
	u32 cur_val = 0;

	if (!DPUI_VALID_KEY(key)) {
		panel_err("out of dpui_key range (%d)\n", key);
		return -EINVAL;
	}

	if (!is_dpui_var_u32(key)) {
		panel_err("invalid type %d\n", dpui.field[key].var_type);
		return -EINVAL;
	}

	if (dpui.field[key].initialized) {
		ret = __decon_get_dpui_u32_field(key, &cur_val);
		if (ret < 0) {
			panel_err("failed to get u32 field (%d)\n", ret);
			return -EINVAL;
		}
	}

	__decon_set_dpui_u32_field(key, cur_val + value);

	return 0;
}

int get_dpui_field(enum dpui_key key, char *buf)
{
	int ret;

	mutex_lock(&dpui_lock);
	ret = __get_dpui_field(key, buf);
	mutex_unlock(&dpui_lock);

	return ret;
}

int decon_set_dpui_field(enum dpui_key key, char *buf, int size)
{
	int ret;

	mutex_lock(&dpui_lock);
	ret = __decon_set_dpui_field(key, buf, size);
	mutex_unlock(&dpui_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(decon_set_dpui_field);

int decon_get_dpui_u32_field(enum dpui_key key, u32 *value)
{
	int ret;

	mutex_lock(&dpui_lock);
	ret = __decon_get_dpui_u32_field(key, value);
	mutex_unlock(&dpui_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(decon_get_dpui_u32_field);

int decon_set_dpui_u32_field(enum dpui_key key, u32 value)
{
	int ret;

	mutex_lock(&dpui_lock);
	ret = __decon_set_dpui_u32_field(key, value);
	mutex_unlock(&dpui_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(decon_set_dpui_u32_field);

int decon_inc_dpui_u32_field(enum dpui_key key, u32 value)
{
	int ret;

	mutex_lock(&dpui_lock);
	ret = __decon_inc_dpui_u32_field(key, value);
	mutex_unlock(&dpui_lock);

	return ret;
}
EXPORT_SYMBOL_GPL(decon_inc_dpui_u32_field);

int __decon_get_dpui_log(char *buf, enum dpui_log_level level, enum dpui_type type)
{
	int i, ret, len = 0;
	char tbuf[MAX_DPUI_KEY_LEN + MAX_DPUI_VAL_LEN];

	if (!buf) {
		panel_err("buf is null\n");
		return -EINVAL;
	}

	if (level < 0 || level >= MAX_DPUI_LOG_LEVEL) {
		panel_err("invalid log level %d\n", level);
		return -EINVAL;
	}

	mutex_lock(&dpui_lock);
	for (i = DPUI_KEY_NONE + 1; i < MAX_DPUI_KEY; i++) {
		if (level != DPUI_LOG_LEVEL_ALL && dpui.field[i].level != level) {
			panel_warn("%s different log level %d %d\n",
					dpui_key_name[dpui.field[i].key],
					dpui.field[i].level, level);
			continue;
		}

		if (type != dpui.field[i].type)
			continue;

		ret = __get_dpui_field(i, tbuf);
		if (ret == 0)
			continue;

		if (len)
			len += snprintf(buf + len, 3, ",");
		len += snprintf(buf + len, MAX_DPUI_KEY_LEN + MAX_DPUI_VAL_LEN,
				"%s", tbuf);
	}
	mutex_unlock(&dpui_lock);

	return len;
}

int decon_get_dpui_log(char *buf, enum dpui_log_level level, enum dpui_type type)
{
	return __decon_get_dpui_log(buf, level, type);
}
EXPORT_SYMBOL_GPL(decon_get_dpui_log);
