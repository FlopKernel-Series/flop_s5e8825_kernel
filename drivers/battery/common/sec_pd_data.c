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

#include <linux/of.h>
#include <linux/module.h>
#include <linux/notifier.h>
#include <linux/battery/sec_pd.h>

#include "sec_pd.h"
#include "sec_pd_core.h"
#include "sec_pd_data.h"

static struct sec_pd *g_pd;

struct sec_pd_dt {
	unsigned int op_mode;
	bool enable_rp_current_wa;
};
static struct sec_pd_dt g_dt;

enum {
	SEC_PD_OP_MODE_ASYNC = 0,
	SEC_PD_OP_MODE_SYNC,
	SEC_PD_OP_MODE_WAIT,
	SEC_PD_OP_MODE_MIX,
	SEC_PD_OP_MODE_MAX
};

extern struct sec_pd *get_async_sec_pd(void);
#if IS_ENABLED(CONFIG_USB_TYPEC_MANAGER_NOTIFIER)
extern struct sec_pd *get_sync_sec_pd(void);
extern struct sec_pd *get_wait_sec_pd(void);
extern struct sec_pd *get_mix_sec_pd(void);
#else
static inline struct sec_pd *get_sync_sec_pd(void)
{ return ERR_PTR(-ENODEV); }
static inline struct sec_pd *get_wait_sec_pd(void)
{ return ERR_PTR(-ENODEV); }
static inline struct sec_pd *get_mix_sec_pd(void)
{ return ERR_PTR(-ENODEV); }
#endif

static struct sec_pd *get_sec_pd_by_op_mode(int op_mode)
{
	if ((op_mode < SEC_PD_OP_MODE_ASYNC) ||
		(op_mode >= SEC_PD_OP_MODE_MAX))
		return ERR_PTR(-EINVAL);

	switch (op_mode) {
	case SEC_PD_OP_MODE_ASYNC:
		return get_async_sec_pd();
	case SEC_PD_OP_MODE_SYNC:
		return get_sync_sec_pd();
	case SEC_PD_OP_MODE_WAIT:
		return get_wait_sec_pd();
	case SEC_PD_OP_MODE_MIX:
		return get_mix_sec_pd();
	}

	return ERR_PTR(-ENODEV);
}

static struct sec_pd_dt *init_dt(struct sec_pd_dt *dt)
{
	dt->op_mode = SEC_PD_OP_MODE_ASYNC;
	dt->enable_rp_current_wa = false;

	return dt;
}

static int parse_dt(struct sec_pd_dt *dt)
{
	struct device_node *np;
	int ret = 0, pd_op_mode = SEC_PD_OP_MODE_ASYNC;

	np = of_find_node_by_name(NULL, "battery");
	if (!np) {
		pr_err("%s: can't find battery node\n", __func__);
		return -ENODEV;
	}

	ret = of_property_read_u32(np, "battery,sec_pd_op_mode", &pd_op_mode);
	if (ret < 0) {
		pr_err("%s: can't find sec_pd_op_mode dt.(%d)\n", __func__, ret);
	} else if (pd_op_mode >= SEC_PD_OP_MODE_MAX) {
		pr_err("%s: wrong sec_pd_op_mode value(%d), set default(%d)\n",
			__func__, pd_op_mode, SEC_PD_OP_MODE_ASYNC);
		pd_op_mode = SEC_PD_OP_MODE_ASYNC;
	}
	dt->op_mode = pd_op_mode;

	dt->enable_rp_current_wa = of_property_read_bool(np, "battery,enable_rp_current_wa");
	return 0;
}

bool get_sec_pd_rp_current_wa_state(void)
{
	return g_dt.enable_rp_current_wa;
}
EXPORT_SYMBOL(get_sec_pd_rp_current_wa_state);

struct sec_pd *init_sec_pd(void)
{
	struct sec_pd *pd;
	int ret = 0;

	ret = parse_dt(init_dt(&g_dt));
	if (ret < 0)
		pr_err("%s: failed to parse dt(ret = %d)\n", __func__, ret);

	pd = get_sec_pd_by_op_mode(g_dt.op_mode);
	if (IS_ERR(pd))
		pd = get_async_sec_pd();

	g_pd = pd;
	return g_pd;
}
EXPORT_SYMBOL(init_sec_pd);

const char *get_sec_pd_op_name(struct sec_pd *pd)
{
	return pd->name;
}
EXPORT_SYMBOL(get_sec_pd_op_name);

int change_sec_pd(int op_mode)
{
	struct sec_pd *pd;

	pd = get_sec_pd_by_op_mode(op_mode);
	if (IS_ERR(pd))
		return PTR_ERR(pd);

	op_init(pd, g_pd->pd_sink);
	g_pd = pd;
	return 0;
}
EXPORT_SYMBOL(change_sec_pd);

struct sec_pd *get_sec_pd(void)
{
	return g_pd;
}
EXPORT_SYMBOL(get_sec_pd);
