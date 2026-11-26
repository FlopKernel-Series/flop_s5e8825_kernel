// SPDX-License-Identifier: GPL-2.0-only
/*
 * arch/arm64/lib/xor-neon-glue.c
 *
 * Authors: Jackie Liu <liuyun01@kylinos.cn>
 * Copyright (C) 2018,Tianjin KYLIN Information Technology Co., Ltd.
 */

#include <linux/raid/xor.h>
#include <linux/module.h>

#include "xor-neon.h"

struct xor_block_template const xor_block_inner_neon = {
	.name	= "__inner_neon__",
	.do_2	= xor_arm64_neon_2,
	.do_3	= xor_arm64_neon_3,
	.do_4	= xor_arm64_neon_4,
	.do_5	= xor_arm64_neon_5,
};
EXPORT_SYMBOL(xor_block_inner_neon);

MODULE_AUTHOR("Jackie Liu <liuyun01@kylinos.cn>");
MODULE_DESCRIPTION("ARMv8 XOR Extensions");
MODULE_LICENSE("GPL");
