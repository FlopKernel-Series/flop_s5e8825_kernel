/* SPDX-License-Identifier: GPL-2.0+ */
/* fs1816.h -- fs1816 ALSA SoC Audio driver
 *
 * Copyright (C) Fourier Semiconductor Inc. 2016-2024.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __FS1816_H__
#define __FS1816_H__

#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/mutex.h>
#include <linux/regmap.h>
#include <linux/version.h>
#include <sound/soc.h>
#include <sound/tlv.h>
#include <sound/pcm_params.h>

#define FS1816_I2C_VERSION "v1.1.3"
#define FS1816_DEVID       (0x34)
#define FS1816_I2C_RETRY   (5)
#define FS1816_I2C_MDELAY  (2)
#define FS1816_WAIT_TIMES  (20)
#define FS1816_STRING_MAX  (32)
#define FS1816_FADE_IN_MS  (15)
#define FS1816_RECOVER_MAX (10)

#define KERNEL_VERSION_HIGHER(a, b, c) \
	(KERNEL_VERSION((a), (b), (c)) <= LINUX_VERSION_CODE)
#define FS1816_DELAY_US(x) \
	do { \
		if ((x) > 0) \
			usleep_range((x), (x)+10); \
	} while (0)
#define FS1816_DELAY_MS(x) FS1816_DELAY_US((x)*1000)
#define FS1816_FUNC_EXIT(dev, ret) \
	do { \
		if (ret) \
			dev_err(dev, "%s: ret:%d\n", __func__, ret); \
	} while (0)

#if KERNEL_VERSION_HIGHER(6, 1, 0)
#define i2c_remove_type void
#define i2c_remove_val
#else
#define i2c_remove_type int
#define i2c_remove_val (0)
#endif

#define FS1816_03H_DEVID		0x03
#define FS1816_04H_I2SCTRL		0x04
#define FS1816_06H_AUDIOCTRL		0x06
#define FS1816_08H_TEMPSEL		0x08
#define FS1816_09H_SYSCTRL		0x09
#define FS1816_0BH_ACCKEY		0x0B
#define FS1816_27H_ANACFG		0x27
#define FS1816_56H_INTCTRL		0x56
#define FS1816_57H_INTMASK		0x57
#define FS1816_58H_INTSTAT		0x58
#define FS1816_90H_CHIPINI		0x90
#define FS1816_96H_NGCTRL		0x96
#define FS1816_97H_AUTOCTRL		0x97
#define FS1816_98H_LNMCTRL		0x98
#define FS1816_A0H_I2SSET		0xA0
#define FS1816_A1H_DSPCTRL		0xA1
#define FS1816_AEH_DACCTRL		0xAE
#define FS1816_BDH_DIGSTAT		0xBD
#define FS1816_C0H_BSTCTRL		0xC0
#define FS1816_C1H_PLLCTRL1		0xC1
#define FS1816_C2H_PLLCTRL2		0xC2
#define FS1816_C3H_PLLCTRL3		0xC3
#define FS1816_C4H_PLLCTRL4		0xC4
#define FS1816_CFH_ADPBST		0xCF
#define FS1816_E9H_EADPENV		0xE9

#define FS1816_04H_I2SSR_SHIFT		12
#define FS1816_04H_I2SSR_MASK		GENMASK(15, 12)
#define FS1816_04H_I2SDOE_SHIFT		11
#define FS1816_04H_I2SDOE_MASK		BIT(11)
#define FS1816_04H_CHS12_SHIFT		3
#define FS1816_04H_CHS12_MASK		GENMASK(4, 3)
#define FS1816_04H_I2SF_SHIFT		0
#define FS1816_04H_I2SF_MASK		GENMASK(2, 0)
#define FS1816_06H_VOL_SHIFT		6
#define FS1816_06H_VOL_MASK		GENMASK(15, 6)
#define FS1816_58H_INTS13_MASK		BIT(13)
#define FS1816_58H_INTS12_MASK		BIT(12)
#define FS1816_58H_INTS10_MASK		BIT(10)
#define FS1816_58H_INTS9_MASK		BIT(9)
#define FS1816_58H_INTS1_MASK		BIT(1)
#define FS1816_96H_NGDLY_SHIFT		4
#define FS1816_96H_NGDLY_MASK		GENMASK(6, 4)
#define FS1816_96H_NGTHD_SHIFT		0
#define FS1816_96H_NGTHD_MASK		GENMASK(3, 0)
#define FS1816_97H_CHKTIME_SHIFT	10
#define FS1816_97H_CHKTIME_MASK		GENMASK(12, 10)
#define FS1816_97H_CHKTHR_SHIFT		0
#define FS1816_97H_CHKTHR_MASK		GENMASK(2, 0)
#define FS1816_98H_FOTIME_SHIFT		4
#define FS1816_98H_FOTIME_MASK		GENMASK(6, 4)
#define FS1816_98H_FITIME_SHIFT		0
#define FS1816_98H_FITIME_MASK		GENMASK(2, 0)
#define FS1816_A0H_CLKSP_SHIFT		5
#define FS1816_A0H_CLKSP_MASK		GENMASK(6, 5)
#define FS1816_BDH_DACRUN_SHIFT		1
#define FS1816_BDH_DACRUN_MASK		BIT(1)
#define FS1816_C0H_VOUT_SEL_SHIFT	10
#define FS1816_C0H_VOUT_SEL_MASK	GENMASK(13, 10)
#define FS1816_C0H_ILIM_SEL_SHIFT	6
#define FS1816_C0H_ILIM_SEL_MASK	GENMASK(9, 6)
#define FS1816_C0H_MODE_CTRL_SHIFT	4
#define FS1816_C0H_MODE_CTRL_MASK	GENMASK(5, 4)
#define FS1816_C0H_BSTEN_SHIFT		3
#define FS1816_C0H_BSTEN_MASK		BIT(3)
#define FS1816_C0H_AGAIN_SHIFT		0
#define FS1816_C0H_AGAIN_MASK		GENMASK(2, 0)

#define FS1816_04H_CHS12_DEFAULT	0x0003
#define FS1816_06H_VOL_DEFAULT		0x03FF
#define FS1816_08H_TEMPSEL_DEFAULT	0x0010
#define FS1816_08H_TEMPSEL_INITED	0x0168
#define FS1816_09H_SYSCTRL_RESET	0x0002
#define FS1816_09H_SYSCTRL_PLAY		0x0008
#define FS1816_09H_SYSCTRL_STOP		0x0001
#define FS1816_0BH_ACCKEY_ON		0xCA91
#define FS1816_0BH_ACCKEY_OFF		0x0000
#define FS1816_90H_CHIPINI_PASS		0x0003
#define FS1816_C4H_PLLCTRL4_ON		0x000B
#define FS1816_C4H_PLLCTRL4_OFF		0x0000

struct fs1816_rate {
	unsigned int rate;
	uint16_t i2ssr;
};

struct fs1816_pll {
	unsigned int bclk;
	uint16_t pll1;
	uint16_t pll2;
	uint16_t pll3;
};

struct fs1816_platform_data {
	struct gpio_desc *gpio_intz;
};

struct fs1816_hw_params {
	int rate;
	int bclk;
	int bit_width;
	int channels;
	unsigned int daifmt;
	unsigned int invfmt;
};

struct fs1816_dev {
	struct device *dev;
	struct i2c_client *i2c;
	struct regmap *regmap;
	struct mutex io_lock;
	struct workqueue_struct *wq;
	struct delayed_work restart_work;
	struct delayed_work irq_work;
#if IS_ENABLED(CONFIG_SND_SOC_SAMSUNG_AUDIO)
	struct delayed_work monitor_work;
#endif

	struct fs1816_platform_data pdata;
	struct fs1816_hw_params hw_params;

	uint32_t rx_volume;
	uint32_t rx_channel;
	int irq_id;
	int rec_count;

	bool force_init;
	bool start_up;
	bool amp_on;
	bool dac_event_up;
	bool stream_on;
	bool restart_work_on;
};

#endif // __FS1816_H__
