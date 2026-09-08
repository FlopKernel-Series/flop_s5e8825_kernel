/* SPDX-License-Identifier: GPL-2.0 */

/*
 * (C) COPYRIGHT 2021 Samsung Electronics Inc. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 */

#include <linux/export.h>
#include <gpexbe_llc_coherency.h>

void gpexbe_llc_coherency_reg_map(void)
{
	return;
}
EXPORT_SYMBOL_GPL(gpexbe_llc_coherency_reg_map);

void gpexbe_llc_coherency_reg_unmap(void)
{
	return;
}
EXPORT_SYMBOL_GPL(gpexbe_llc_coherency_reg_unmap);

void gpexbe_llc_coherency_set_coherency_feature(void)
{
	return;
}
EXPORT_SYMBOL_GPL(gpexbe_llc_coherency_set_coherency_feature);

void gpexbe_llc_coherency_set_aruser(void)
{
	return;
}
EXPORT_SYMBOL_GPL(gpexbe_llc_coherency_set_aruser);

void gpexbe_llc_coherency_set_awuser(void)
{
	return;
}
EXPORT_SYMBOL_GPL(gpexbe_llc_coherency_set_awuser);

int gpexbe_llc_coherency_init(struct device **dev)
{
	return 0;
}

void gpexbe_llc_coherency_term(void)
{
	return;
}

int gpexbe_llc_coherency_set_ways(int ways)
{
	return 0;
}
