/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (C) 2023 Samsung Electronics Co. Ltd.
 */

#ifndef __SEC_VIBRATOR_KUNIT_H__
#define __SEC_VIBRATOR_KUNIT_H__

#if IS_ENABLED(CONFIG_SEC_KUNIT)
#include <kunit/mock.h>
#endif

#ifndef __mockable
#define __mockable
#endif

#ifndef __visible_for_testing
#define __visible_for_testing static
#endif

#ifndef EXPORT_SYMBOL_KUNIT
#define EXPORT_SYMBOL_KUNIT(sym)        /* nothing */
#endif

#endif /* __SEC_VIBRATOR_KUNIT_H__ */
