/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _WORKAROUNDS_H
#define _WORKAROUNDS_H

#if defined(CONFIG_DEFAULT_SUPPORT_AOSP)
static inline bool is_aosp_mode(void)
{
	return true;
}
#else
bool is_aosp_mode(void);
#endif

#endif /* _WORKAROUNDS_H */
