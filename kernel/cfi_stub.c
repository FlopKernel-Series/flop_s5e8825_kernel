// SPDX-License-Identifier: GPL-2.0
/*
 * Stub symbols for legacy Clang CFI LKMs.
 */

#include <linux/export.h>
#include <linux/types.h>

void __cfi_slowpath(u64 id, void *ptr)
{
}
EXPORT_SYMBOL(__cfi_slowpath);

void __cfi_slowpath_diag(u64 id, void *ptr, void *diag)
{
}
EXPORT_SYMBOL(__cfi_slowpath_diag);

void __cfi_check(u64 id, void *ptr, void *diag)
{
}
EXPORT_SYMBOL(__cfi_check);

void __ubsan_handle_cfi_check_fail(void *data, void *ptr, void *vtable)
{
}
EXPORT_SYMBOL(__ubsan_handle_cfi_check_fail);

void __ubsan_handle_cfi_check_fail_abort(void *data, void *ptr, void *vtable)
{
}
EXPORT_SYMBOL(__ubsan_handle_cfi_check_fail_abort);
