// SPDX-License-Identifier: GPL-2.0-only
/*
 * sec_debug_ksym.c
 *
 * Copyright (c) 2019 Samsung Electronics Co., Ltd
 *              http://www.samsung.com
 */

#include <linux/kernel.h>

#include <linux/sec_debug.h>

#include "sec_debug_internal.h"


void secdbg_ksym_set_kallsyms_info(struct sec_debug_ksyms *ksyms)
{
	pr_info("%s: skipped, DT kallsyms not valid\n", __func__);
}
