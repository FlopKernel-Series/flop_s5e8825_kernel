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

/* Implements */
#include <linux/export.h>
#include <linux/errno.h>
#include <gpexbe_secure.h>
#include <gpexbe_smc.h>

int gpexbe_secure_protection_enable(void)
{
	return gpexbe_smc_protection_enable();
}
EXPORT_SYMBOL_GPL(gpexbe_secure_protection_enable);

int gpexbe_secure_protection_disable(void)
{
	return gpexbe_smc_protection_disable();
}
EXPORT_SYMBOL_GPL(gpexbe_secure_protection_disable);

struct protected_mode_ops *gpexbe_secure_get_protected_mode_ops(void)
{
	return NULL;
}

int gpexbe_secure_legacy_jm_enter_protected_mode(struct kbase_device *kbdev)
{
	return -ENOSYS;
}

int gpexbe_secure_legacy_jm_exit_protected_mode(struct kbase_device *kbdev)
{
	return -ENOSYS;
}

int gpexbe_secure_legacy_pm_exit_protected_mode(struct kbase_device *kbdev)
{
	return -ENOSYS;
}
