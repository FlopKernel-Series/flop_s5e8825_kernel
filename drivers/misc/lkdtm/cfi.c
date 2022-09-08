// SPDX-License-Identifier: GPL-2.0
/*
 * This is for all the tests relating directly to Control Flow Integrity.
 */
#include "lkdtm.h"

static int called_count;

/* Function taking one argument, without a return value. */
static noinline void lkdtm_increment_void(int *counter)
{
	(*counter)++;
}

/* Function taking one argument, returning int. */
static noinline int lkdtm_increment_int(int *counter)
{
	(*counter)++;

	return *counter;
}

/* Don't allow the compiler to inline the calls. */
static noinline void lkdtm_indirect_call(void (*func)(int *))
{
	func(&called_count);
}

/*
 * This tries to call an indirect function with a mismatched prototype.
 */
void lkdtm_CFI_FORWARD_PROTO(void)
{
	/*
	 * Matches lkdtm_increment_void()'s prototype, but not
	 * lkdtm_increment_int()'s prototype.
	 */
	pr_info("Calling matched prototype ...\n");
	lkdtm_indirect_call(lkdtm_increment_void);

	pr_info("Calling mismatched prototype ...\n");
	lkdtm_indirect_call((void *)lkdtm_increment_int);

	pr_info("Fail: survived mismatched prototype function call!\n");
}
