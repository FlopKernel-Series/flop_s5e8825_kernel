/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Suppress EXPORT_SYMBOL in non-default Mali KMD modules (r32p1, r44p1)
 * to avoid duplicate exports at build time, only r38p1 defines the
 * internal kbase symbols. External consumers go through the built-in
 * mali_redirect instead.
 * Must include <linux/export.h> first so the include guard is set
 * before the empty macros are defined.
 */

#include <linux/export.h>

#undef EXPORT_SYMBOL
#define EXPORT_SYMBOL(x)

#undef EXPORT_SYMBOL_GPL
#define EXPORT_SYMBOL_GPL(x)

#undef EXPORT_SYMBOL_NS
#define EXPORT_SYMBOL_NS(x, ns)

#undef EXPORT_SYMBOL_NS_GPL
#define EXPORT_SYMBOL_NS_GPL(x, ns)
