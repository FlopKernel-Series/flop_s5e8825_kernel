/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Suppress EXPORT_SYMBOL in non-selected Mali KMD modules.
 * Only the version selected by mali.version= cmdline should
 * export symbols to avoid duplicate definitions.
 * Must include <linux/export.h> first.
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
