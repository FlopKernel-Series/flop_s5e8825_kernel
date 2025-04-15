// SPDX-License-Identifier: GPL-2.0+
/* fs1816.c -- fs1816 ALSA SoC Audio driver
 *
 * Copyright (C) Fourier Semiconductor Inc. 2016-2024.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/debugfs.h>
#ifdef CONFIG_OF
#include <linux/of.h>
#include <linux/of_gpio.h>
#endif
#include "fs1816.h"

/* Supported rates and data formats */
#define FS1816_CODEC_RATES (SNDRV_PCM_RATE_8000|SNDRV_PCM_RATE_16000|\
		SNDRV_PCM_RATE_32000|SNDRV_PCM_RATE_44100|\
		SNDRV_PCM_RATE_48000)
#define FS1816_CODEC_FORMATS (SNDRV_PCM_FMTBIT_S16_LE|\
		SNDRV_PCM_FMTBIT_S24_LE|\
		SNDRV_PCM_FMTBIT_S24_3LE|\
		SNDRV_PCM_FMTBIT_S32_LE)

static DEFINE_MUTEX(fs1816_mutex);

static struct fs1816_rate fs1816_rates[] = {
	{   8000, 0x0 }, // RATE_8000
	{  16000, 0x3 }, // RATE_16000
	{  32000, 0x8 }, // RATE_32000
	{  44100, 0x7 }, // RATE_44100
	{  48000, 0x8 }, // RATE_48000
};

static struct fs1816_pll fs1816_plls[] = {
	{   256000, 0x01A0, 0x0180, 0x0001 },
	{   384000, 0x0260, 0x0100, 0x0001 },
	{   512000, 0x01A0, 0x0180, 0x0002 },
	{   768000, 0x0260, 0x0100, 0x0002 },
	{  1024000, 0x0260, 0x0120, 0x0003 },
	{  1411200, 0x01A0, 0x0100, 0x0004 },
	{  1536000, 0x0260, 0x0100, 0x0004 },
	{  2048000, 0x0260, 0x0120, 0x0006 },
	{  2304000, 0x0260, 0x0100, 0x0006 },
	{  2822400, 0x01A0, 0x0100, 0x0008 },
	{  3072000, 0x0260, 0x0100, 0x0008 },
	{  6144000, 0x0260, 0x0100, 0x0010 },
};

static const struct reg_sequence fs1816_init_list[] = {
	{ FS1816_04H_I2SCTRL,		0x800B },
	{ FS1816_06H_AUDIOCTRL,		0xF7C0 },
	{ FS1816_27H_ANACFG,		0x0900 },
	{ FS1816_56H_INTCTRL,		0x0801 },
	{ FS1816_57H_INTMASK,		0x0000 },
	{ FS1816_96H_NGCTRL,		0x0055 },
	{ FS1816_97H_AUTOCTRL,		0x0C04 },
	{ FS1816_98H_LNMCTRL,		0x1F11 },
	{ FS1816_A1H_DSPCTRL,		0x8015 },
	{ FS1816_AEH_DACCTRL,		0x0210 },
	{ FS1816_C0H_BSTCTRL,		0x1DCA },
	{ FS1816_CFH_ADPBST,		0x6000 },
	{ FS1816_0BH_ACCKEY,		0xCA91 },
	{ FS1816_E9H_EADPENV,		0x6506 },
	{ FS1816_0BH_ACCKEY,		0x0000 },
};

static int fs1816_reg_read(struct fs1816_dev *fs1816,
		uint8_t reg, uint16_t *pval);
static int fs1816_irq_switch(struct fs1816_dev *fs1816, bool enable);

int fs1816_i2c_reset(struct fs1816_dev *fs1816)
{
	struct i2c_msg msgs[1];
	uint8_t buf[3] = { 0x09, 0x00, 0x02 };
	uint8_t addr;
	int ret;
	uint16_t devid;

	if (fs1816 == NULL || fs1816->i2c == NULL)
		return -EINVAL;

	ret = fs1816_reg_read(fs1816, FS1816_03H_DEVID, &devid);
	/* If we read devid ok, skip scanning addresses */
	if (!ret && (devid >> 8) == FS1816_DEVID)
		return 0;

	for (addr = 0x34; addr <= 0x37; addr++) { /* scan 0x34 -> 0x37 */
		mutex_lock(&fs1816->io_lock);
		msgs[0].addr = addr;
		msgs[0].flags = 0;
		msgs[0].len = sizeof(buf);
		msgs[0].buf = buf;
		ret = i2c_transfer(fs1816->i2c->adapter,
				&msgs[0], ARRAY_SIZE(msgs));
		mutex_unlock(&fs1816->io_lock);
		if (ret > 0)
			dev_info(fs1816->dev, "reset addr:0x%02X ret:%d\n",
					addr, ret);
	}
	FS1816_DELAY_MS(10);

	return 0;
}

static const struct regmap_config fs1816_i2c_regmap = {
	.reg_bits = 8,
	.val_bits = 16,
	.max_register = 0xFF,
	.val_format_endian = REGMAP_ENDIAN_BIG,
	.cache_type = REGCACHE_NONE,
};

static int fs1816_regmap_init(struct fs1816_dev *fs1816)
{
	struct regmap *regmap;
	int ret;

	if (fs1816 == NULL || fs1816->i2c == NULL)
		return -EINVAL;

	regmap = devm_regmap_init_i2c(fs1816->i2c, &fs1816_i2c_regmap);
	if (IS_ERR_OR_NULL(regmap)) {
		ret = PTR_ERR(regmap);
		dev_err(fs1816->dev, "Failed to init regmap:%d\n", ret);
		return ret;
	}

	fs1816->regmap = regmap;

	return 0;
}

static int fs1816_reg_read(struct fs1816_dev *fs1816,
		uint8_t reg, uint16_t *pval)
{
	unsigned int val;
	int retries = 0;
	int ret;

	if (fs1816 == NULL || fs1816->regmap == NULL || pval == NULL)
		return -EINVAL;

	do {
		mutex_lock(&fs1816->io_lock);
		ret = regmap_read(fs1816->regmap, reg, &val);
		mutex_unlock(&fs1816->io_lock);
		if (!ret)
			break;
		FS1816_DELAY_MS(FS1816_I2C_MDELAY);
		retries++;
	} while (retries < FS1816_I2C_RETRY);

	if (ret) {
		dev_err(fs1816->dev, "Failed to read %02Xh:%d\n", reg, ret);
		return -EIO;
	}

	*pval = val;
	dev_dbg(fs1816->dev, "RR: %02x %04x\n", reg, *pval);

	return 0;
}

static int fs1816_reg_read_status(struct fs1816_dev *fs1816,
		uint8_t reg, uint16_t *stat)
{
	uint16_t old, value;
	int count;
	int ret;

	if (fs1816 == NULL)
		return -EINVAL;

	for (count = 0; count < FS1816_I2C_RETRY; count++) {
		ret = fs1816_reg_read(fs1816, reg, &value);
		if (ret)
			return ret;
		if (count > 0 && old == value)
			break;
		old = value;
	}

	if (stat)
		*stat = value;

	return ret;

}

static int fs1816_reg_write(struct fs1816_dev *fs1816,
		uint8_t reg, uint16_t val)
{
	int retries = 0;
	int ret;

	if (fs1816 == NULL || fs1816->regmap == NULL)
		return -EINVAL;

	do {
		mutex_lock(&fs1816->io_lock);
		ret = regmap_write(fs1816->regmap, reg, val);
		mutex_unlock(&fs1816->io_lock);
		if (!ret)
			break;
		FS1816_DELAY_MS(FS1816_I2C_MDELAY);
		retries++;
	} while (retries < FS1816_I2C_RETRY);

	if (ret) {
		dev_err(fs1816->dev, "Failed to write %02Xh:%d\n", reg, ret);
		return -EIO;
	}

	dev_dbg(fs1816->dev, "RW: %02x %04x\n", reg, val);

	return 0;
}

static int fs1816_multi_reg_write(struct fs1816_dev *fs1816,
		const struct reg_sequence *regs, int num_regs)
{
	int i, ret;

	if (fs1816 == NULL || regs == NULL)
		return -EINVAL;

	for (i = 0; i < num_regs; i++) {
		ret = fs1816_reg_write(fs1816, regs[i].reg, regs[i].def);
		if (ret)
			break;
		if (regs[i].delay_us)
			udelay(regs[i].delay_us);
	}

	return ret;
}

static int fs1816_reg_update_bits(struct fs1816_dev *fs1816,
		uint8_t reg, uint16_t mask, uint16_t val)
{
	int retries = 0;
	int ret;

	if (fs1816 == NULL || fs1816->regmap == NULL)
		return -EINVAL;

	do {
		mutex_lock(&fs1816->io_lock);
		ret = regmap_update_bits(fs1816->regmap, reg, mask, val);
		mutex_unlock(&fs1816->io_lock);
		if (!ret)
			break;
		FS1816_DELAY_MS(FS1816_I2C_MDELAY);
		retries++;
	} while (retries < FS1816_I2C_RETRY);

	if (ret) {
		dev_err(fs1816->dev,
				"Failed to update %02Xh:%d\n", reg, ret);
		return -EIO;
	}

	dev_dbg(fs1816->dev, "RU: %02x %04x %04x\n", reg, mask, val);

	return 0;
}

static int fs1816_wait_stable(struct fs1816_dev *fs1816,
		uint8_t reg, uint16_t mask, uint16_t val)
{
	uint16_t status;
	int i, ret;

	if (fs1816 == NULL)
		return -EINVAL;

	FS1816_DELAY_MS(FS1816_I2C_MDELAY);
	for (i = 0; i < FS1816_WAIT_TIMES; i++) {
		ret = fs1816_reg_read_status(fs1816, reg, &status);
		if (ret)
			return ret;
		dev_dbg(fs1816->dev, "WS: %02x %04x %04x\n", reg, mask, val);
		if ((status & mask) == val)
			return 0;
		FS1816_DELAY_MS(FS1816_I2C_MDELAY);
	}
	dev_err(fs1816->dev, "Wait %02Xh stable timeout!\n", reg);

	return -ETIMEDOUT;
}


static int fs1816_reg_dump(struct fs1816_dev *fs1816, uint8_t reg_max)
{
	uint16_t val[8] = { 0 };
	uint8_t idx, reg;
	int ret;

	if (fs1816 == NULL || fs1816->dev == NULL)
		return -EINVAL;

	dev_info(fs1816->dev, "Dump registers[0x00-0x%02X]:\n", reg_max);
	for (reg = 0x00; reg <= reg_max; reg++) {
		idx = (reg & 0x7);
		ret = fs1816_reg_read(fs1816, reg, &val[idx]);
		if (ret)
			break;
		if (idx != 0x7 && reg != reg_max)
			continue;
		dev_info(fs1816->dev, "%02Xh: %04x %04x %04x %04x %04x %04x %04x %04x\n",
				(reg & 0xF8), val[0], val[1], val[2],
				val[3], val[4], val[5], val[6], val[7]);
		memset(val, 0, sizeof(val));
	}

	FS1816_FUNC_EXIT(fs1816->dev, ret);
	return ret;
}

static int fs1816_detect_device(struct fs1816_dev *fs1816)
{
	uint16_t devid;
	int ret;

	ret = fs1816_reg_read(fs1816, FS1816_03H_DEVID, &devid);
	if (ret)
		return ret;

	if ((devid >> 8) != FS1816_DEVID) {
		dev_err(fs1816->dev, "Unknow devid:%04x\n", devid);
		return -ENODEV;
	}

	dev_info(fs1816->dev, "FS1816 detected!\n");

	return 0;
}

static int fs1816_self_checking(struct fs1816_dev *fs1816)
{
	uint16_t stat;
	int retry = 0;
	int ret;

	do {
		ret = fs1816_reg_write(fs1816, FS1816_09H_SYSCTRL,
				FS1816_09H_SYSCTRL_RESET);
		FS1816_DELAY_MS(10);
		ret = fs1816_reg_read_status(fs1816, FS1816_90H_CHIPINI, &stat);
		if (!ret && stat == FS1816_90H_CHIPINI_PASS)
			break;
	} while (retry++ < FS1816_I2C_RETRY);

	if (retry == FS1816_I2C_RETRY)
		return -ETIMEDOUT;

	return 0;
}

static int fs1816_chip_init(struct fs1816_dev *fs1816)
{
	uint16_t val;
	int ret;

	ret = fs1816_reg_read(fs1816, FS1816_08H_TEMPSEL, &val);
	if (ret)
		return ret;

	if (!fs1816->force_init && val == FS1816_08H_TEMPSEL_INITED)
		return 0;

	fs1816->force_init = false;
	ret = fs1816_self_checking(fs1816);
	if (ret) {
		dev_err(fs1816->dev, "Failed to self check\n");
		return ret;
	}

	ret = fs1816_multi_reg_write(fs1816, fs1816_init_list,
			ARRAY_SIZE(fs1816_init_list));
	if (ret) {
		dev_err(fs1816->dev, "Failed to write init list:%d\n", ret);
		return ret;
	}

	/* Disable INT when the intz gpio is unused */
	if (IS_ERR_OR_NULL(fs1816->pdata.gpio_intz)) {
		ret |= fs1816_reg_write(fs1816, FS1816_56H_INTCTRL, 0x0001);
		ret |= fs1816_reg_write(fs1816, FS1816_57H_INTMASK, 0x7FFF);
	}

	ret = fs1816_reg_update_bits(fs1816, FS1816_04H_I2SCTRL,
			FS1816_04H_CHS12_MASK,
			fs1816->rx_channel << FS1816_04H_CHS12_SHIFT);
	ret |= fs1816_reg_update_bits(fs1816, FS1816_06H_AUDIOCTRL,
			FS1816_06H_VOL_MASK,
			fs1816->rx_volume << FS1816_06H_VOL_SHIFT);
	ret |= fs1816_reg_write(fs1816, FS1816_08H_TEMPSEL,
			FS1816_08H_TEMPSEL_INITED);

	return ret;
}

static int fs1816_set_hw_params(struct fs1816_dev *fs1816)
{
	struct fs1816_hw_params *params;
	uint16_t val, mask;
	int idx;
	int ret;

	params = &fs1816->hw_params;
	for (idx = 0; idx < ARRAY_SIZE(fs1816_rates); idx++)
		if (fs1816_rates[idx].rate == params->rate)
			break;

	if (idx == ARRAY_SIZE(fs1816_rates)) {
		dev_err(fs1816->dev, "Invalid rate:%d\n", params->rate);
		return -EINVAL;
	}

	mask = FS1816_04H_I2SSR_MASK;
	val = fs1816_rates[idx].i2ssr << FS1816_04H_I2SSR_SHIFT;
	if (params->daifmt != 0xFF) { /* from dai.set_fmt */
		mask |= FS1816_04H_I2SF_MASK;
		val |= params->daifmt << FS1816_04H_I2SF_SHIFT;
	}
	ret = fs1816_reg_update_bits(fs1816, FS1816_04H_I2SCTRL, mask, val);

	if (params->invfmt != 0xFF) { /* from dai.set_fmt */
		mask = FS1816_A0H_CLKSP_MASK;
		val = params->invfmt << FS1816_A0H_CLKSP_SHIFT;
		ret |= fs1816_reg_update_bits(fs1816,
				FS1816_A0H_I2SSET, mask, val);
	}

	for (idx = 0; idx < ARRAY_SIZE(fs1816_plls); idx++)
		if (fs1816_plls[idx].bclk == params->bclk)
			break;

	if (idx == ARRAY_SIZE(fs1816_plls)) {
		dev_err(fs1816->dev, "Invalid bclk:%d\n", params->bclk);
		return -EINVAL;
	}

	ret |= fs1816_reg_write(fs1816, FS1816_C1H_PLLCTRL1,
			fs1816_plls[idx].pll1);
	ret |= fs1816_reg_write(fs1816, FS1816_C2H_PLLCTRL2,
			fs1816_plls[idx].pll2);
	ret |= fs1816_reg_write(fs1816, FS1816_C3H_PLLCTRL3,
			fs1816_plls[idx].pll3);

	return ret;
}

static int fs1816_startup(struct fs1816_dev *fs1816)
{
	int ret;

	ret = fs1816_reg_write(fs1816, FS1816_C4H_PLLCTRL4,
			FS1816_C4H_PLLCTRL4_ON);
	if (!ret)
		fs1816->start_up = true;

	return ret;
}

static int fs1816_check_params(struct fs1816_dev *fs1816)
{
	uint16_t val;
	int ret;

	/* Rewrite I2S rate and format */
	ret = fs1816_set_hw_params(fs1816);
	if (ret)
		dev_err(fs1816->dev, "Failed to rewrite hw params:%d\n", ret);

	/* Check Volume: 06h bit15..0 */
	ret = fs1816_reg_read(fs1816, FS1816_06H_AUDIOCTRL, &val);
	if (!ret && (val >> FS1816_06H_VOL_SHIFT) != fs1816->rx_volume) {
		dev_err(fs1816->dev, "Failed to check VOLUME\n");
		val = fs1816->rx_volume << FS1816_06H_VOL_SHIFT;
		ret = fs1816_reg_write(fs1816, FS1816_06H_AUDIOCTRL, val);
	}

	/* Check BSTEN: C0h bit3=1 */
	ret = fs1816_reg_read_status(fs1816, FS1816_C0H_BSTCTRL, &val);
	if (!ret && (val & (1 << 3)) == 0) { // bit3
		dev_err(fs1816->dev, "Failed to check BSTEN\n");
		val |= (1 << 3); // bit3
		ret = fs1816_reg_write(fs1816, FS1816_C0H_BSTCTRL, val);
	}

	/* Check DAC UNMUTE: AEh bit8=0 */
	ret = fs1816_reg_read(fs1816, FS1816_AEH_DACCTRL, &val);
	if (!ret && (val & (1 << 8)) != 0) {
		dev_err(fs1816->dev, "Failed to check DACMUTE\n");
		val &= ~(1 << 8);
		ret = fs1816_reg_write(fs1816, FS1816_AEH_DACCTRL, val);
	}

	return ret;
}

static int fs1816_check_status(struct fs1816_dev *fs1816)
{
	uint16_t val[15];
	uint16_t *pval;
	int ret;

	pval = val;
	FS1816_DELAY_MS(10); // wait system stable
	ret  = fs1816_reg_read_status(fs1816, 0x00, pval++); // STATUS
	ret |= fs1816_reg_read(fs1816, 0x04, pval++); // I2SCTRL
	ret |= fs1816_reg_read_status(fs1816, 0x05, pval++); // ANASTAT
	ret |= fs1816_reg_read(fs1816, 0x06, pval++); // VOLUME
	ret |= fs1816_reg_read(fs1816, 0xAE, pval++); // DACCTRL
	ret |= fs1816_reg_read_status(fs1816, 0xBD, pval++); // DIGSTAT
	ret |= fs1816_reg_read_status(fs1816, FS1816_C0H_BSTCTRL, pval);
	if (ret)
		dev_err(fs1816->dev, "Failed to check status:%d\n", ret);

	dev_info(fs1816->dev, "status: %04X %04X %04X %04X %04X %04X %04X\n",
			val[0], val[1], val[2], val[3],
			val[4], val[5], val[6]);

#if IS_ENABLED(CONFIG_SND_SOC_SAMSUNG_AUDIO)
	if ((val[0] & (1 << 6)) == 0)
		queue_delayed_work(fs1816->wq, &fs1816->monitor_work, 1);
#endif
	return ret;
}

static int fs1816_play(struct fs1816_dev *fs1816)
{
	uint16_t val;
	int ret;

	ret = fs1816_reg_read(fs1816, FS1816_08H_TEMPSEL, &val);
	if (!ret && val == FS1816_08H_TEMPSEL_DEFAULT) {
		fs1816->restart_work_on = true;
		return 0;
	}

	ret = fs1816_check_params(fs1816);
	ret |= fs1816_reg_write(fs1816, FS1816_C4H_PLLCTRL4,
			FS1816_C4H_PLLCTRL4_ON);
	ret |= fs1816_reg_write(fs1816, FS1816_09H_SYSCTRL,
			FS1816_09H_SYSCTRL_PLAY);
	fs1816_reg_read_status(fs1816, FS1816_58H_INTSTAT,
			&val); // clear
	if (!ret)
		fs1816->amp_on = true;

	fs1816_irq_switch(fs1816, true);
	fs1816_check_status(fs1816);

	return ret;
}

static int fs1816_stop(struct fs1816_dev *fs1816)
{
	uint16_t intstat;
	int ret;

	fs1816_irq_switch(fs1816, false);
	fs1816->restart_work_on = false;
	fs1816->rec_count = 0;

	ret = fs1816_reg_write(fs1816, FS1816_09H_SYSCTRL,
			FS1816_09H_SYSCTRL_STOP);
	fs1816_wait_stable(fs1816, FS1816_BDH_DIGSTAT,
			FS1816_BDH_DACRUN_MASK, 0);
	fs1816_reg_read_status(fs1816, FS1816_58H_INTSTAT,
			&intstat); // clear
	ret |= fs1816_reg_write(fs1816, FS1816_C4H_PLLCTRL4,
			FS1816_C4H_PLLCTRL4_OFF);
	if (!ret)
		fs1816->amp_on = false;

#if IS_ENABLED(CONFIG_SND_SOC_SAMSUNG_AUDIO)
	cancel_delayed_work_sync(&fs1816->monitor_work);
#endif
	return ret;
}

static int fs1816_shutdown(struct fs1816_dev *fs1816)
{
	int ret;

	ret = fs1816_reg_write(fs1816, FS1816_C4H_PLLCTRL4,
			FS1816_C4H_PLLCTRL4_OFF);
	if (!ret)
		fs1816->start_up = false;

	return ret;
}

static int fs1816_recover_device(struct fs1816_dev *fs1816)
{
	int ret;

	fs1816->force_init = true;
	ret = fs1816_chip_init(fs1816);
	if (ret)
		return ret;

	ret = fs1816_set_hw_params(fs1816);
	if (fs1816->dac_event_up)
		ret |= fs1816_startup(fs1816);

	if (fs1816->dac_event_up && fs1816->stream_on)
		ret |= fs1816_play(fs1816);

	return ret;
}

static int fs1816_apply(struct fs1816_dev *fs1816,
		int (*apply_func)(struct fs1816_dev *fs1816, int argv),
		int argv)
{
	int ret;

	if (fs1816 == NULL || apply_func == NULL)
		return -EINVAL;

	if (fs1816->amp_on) {
		ret  = fs1816_stop(fs1816);
		ret |= apply_func(fs1816, argv);
		ret |= fs1816_play(fs1816);
	} else {
		ret = apply_func(fs1816, argv);
	}

	return ret;
}

static int fs1816_set_boost_mode(struct fs1816_dev *fs1816, int mode)
{
	int mask, val;
	int ret;

	if (mode < 0 || mode > 3)
		return -EINVAL;

	if (mode == 0) {
		mask = FS1816_C0H_BSTEN_MASK;
		val = 0 << FS1816_C0H_BSTEN_SHIFT;
	} else {
		mask = FS1816_C0H_MODE_CTRL_MASK;
		val = mode << FS1816_C0H_MODE_CTRL_SHIFT;
		mask |= FS1816_C0H_BSTEN_MASK;
		val |= 1 << FS1816_C0H_BSTEN_SHIFT;
	}
	ret = fs1816_reg_update_bits(fs1816, FS1816_C0H_BSTCTRL, mask, val);

	return ret;
}

static int fs1816_set_boost_voltage(struct fs1816_dev *fs1816, int vout)
{
	int mask, val;
	int ret;

	if (vout < 0 || vout > 10)
		return -EINVAL;

	mask = FS1816_C0H_VOUT_SEL_MASK;
	val = vout << FS1816_C0H_VOUT_SEL_SHIFT;
	ret = fs1816_reg_update_bits(fs1816, FS1816_C0H_BSTCTRL, mask, val);

	return ret;
}

static int fs1816_set_boost_ilimit(struct fs1816_dev *fs1816, int ilim)
{
	int mask, val;
	int ret;

	if (ilim < 0 || ilim > 8)
		return -EINVAL;

	mask = FS1816_C0H_ILIM_SEL_MASK;
	val = ilim << FS1816_C0H_ILIM_SEL_SHIFT;
	ret = fs1816_reg_update_bits(fs1816, FS1816_C0H_BSTCTRL, mask, val);

	return ret;
}

static int fs1816_set_amp_again(struct fs1816_dev *fs1816, int gain)
{
	int mask, val;
	int ret;

	if (gain < 0 || gain > 7)
		return -EINVAL;

	mask = FS1816_C0H_AGAIN_MASK;
	val = gain << FS1816_C0H_AGAIN_SHIFT;
	ret = fs1816_reg_update_bits(fs1816, FS1816_C0H_BSTCTRL, mask, val);

	return ret;
}

static int fs1816_set_ng_window(struct fs1816_dev *fs1816, int delay)
{
	int mask, val;
	int ret;

	if (delay < 0 || delay > 7)
		return -EINVAL;

	mask = FS1816_96H_NGDLY_MASK;
	val = delay << FS1816_96H_NGDLY_SHIFT;
	ret = fs1816_reg_update_bits(fs1816, FS1816_C0H_BSTCTRL, mask, val);

	return ret;
}

static int fs1816_set_ng_threshold(struct fs1816_dev *fs1816, int thrd)
{
	int mask, val;
	int ret;

	if (thrd < 0 || thrd > 10)
		return -EINVAL;

	mask = FS1816_96H_NGTHD_MASK;
	val = thrd << FS1816_96H_NGTHD_SHIFT;
	ret = fs1816_reg_update_bits(fs1816, FS1816_C0H_BSTCTRL, mask, val);

	return ret;
}

static int fs1816_set_i2s_chn_sel(struct fs1816_dev *fs1816, int chn_sel)
{
	int mask, val;
	int ret;

	if (chn_sel < 0 || chn_sel > 2)
		return -EINVAL;

	mask = FS1816_04H_CHS12_MASK;
	val = (chn_sel+1) << FS1816_04H_CHS12_SHIFT;
	ret = fs1816_reg_update_bits(fs1816, FS1816_04H_I2SCTRL, mask, val);

	return ret;
}

static int fs1816_set_lnm_fo_time(struct fs1816_dev *fs1816, int fo_time)
{
	int mask, val;
	int ret;

	if (fo_time < 0 || fo_time > 5)
		return -EINVAL;

	mask = FS1816_98H_FOTIME_MASK;
	val = fo_time << FS1816_98H_FOTIME_SHIFT;
	ret = fs1816_reg_update_bits(fs1816, FS1816_98H_LNMCTRL, mask, val);

	return ret;
}

static int fs1816_set_lnm_fi_time(struct fs1816_dev *fs1816, int fi_time)
{
	int mask, val;
	int ret;

	if (fi_time < 0 || fi_time > 5)
		return -EINVAL;

	mask = FS1816_98H_FITIME_MASK;
	val = fi_time << FS1816_98H_FITIME_SHIFT;
	ret = fs1816_reg_update_bits(fs1816, FS1816_98H_LNMCTRL, mask, val);

	return ret;
}

static int fs1816_set_lnm_check_time(struct fs1816_dev *fs1816, int check_time)
{
	int mask, val;
	int ret;

	if (check_time < 0 || check_time > 7)
		return -EINVAL;

	mask = FS1816_97H_CHKTIME_MASK;
	val = check_time << FS1816_97H_CHKTIME_SHIFT;
	ret = fs1816_reg_update_bits(fs1816, FS1816_97H_AUTOCTRL, mask, val);

	return ret;
}

static int fs1816_set_lnm_check_thr(struct fs1816_dev *fs1816, int check_thr)
{
	int mask, val;
	int ret;

	if (check_thr < 0 || check_thr > 7)
		return -EINVAL;

	mask = FS1816_97H_CHKTHR_MASK;
	val = check_thr << FS1816_97H_CHKTHR_SHIFT;
	ret = fs1816_reg_update_bits(fs1816, FS1816_97H_AUTOCTRL, mask, val);

	return ret;
}

static const unsigned int fs1816_codec_rates[] = {
	8000, 16000, 32000, 44100, 48000
};

static const struct snd_pcm_hw_constraint_list fs1816_constraints = {
	.list = fs1816_codec_rates,
	.count = ARRAY_SIZE(fs1816_codec_rates),
};

static int fs1816_dai_startup(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct fs1816_dev *fs1816;
	int ret;

	fs1816 = snd_soc_component_get_drvdata(dai->component);
	if (fs1816 == NULL) {
		pr_err("dai_startup: fs1816 is null\n");
		return -EINVAL;
	}

	dev_info(fs1816->dev, "dai: start up\n");
	if (substream->runtime == NULL) {
		dev_dbg(fs1816->dev, "substream runtime is null\n");
		return 0;
	}

	ret = snd_pcm_hw_constraint_mask64(substream->runtime,
			SNDRV_PCM_HW_PARAM_FORMAT, FS1816_CODEC_FORMATS);
	if (ret < 0) {
		dev_err(fs1816->dev,
				"Failed to set param format:%d\n", ret);
		return ret;
	}

	ret = snd_pcm_hw_constraint_list(substream->runtime,
			0, SNDRV_PCM_HW_PARAM_RATE, &fs1816_constraints);
	if (ret < 0) {
		dev_err(fs1816->dev,
				"Failed to set param rate:%d\n", ret);
		return ret;
	}

	return 0;
}

static int fs1816_dai_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	struct fs1816_dev *fs1816;

	fs1816 = snd_soc_component_get_drvdata(dai->component);
	if (fs1816 == NULL) {
		pr_err("dai_set_fmt: fs1816 is null\n");
		return -EINVAL;
	}

	dev_info(fs1816->dev, "dai: set format: 0x%X\n", fmt);

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		/* Only supports slave mode */
		break;
	case SND_SOC_DAIFMT_CBM_CFM:
	default:
		dev_err(fs1816->dev, "Invalid DAI master/slave mode\n");
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		fs1816->hw_params.daifmt = 3; /* Philips standard I2S */
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		fs1816->hw_params.daifmt = 4; /* LSB justified (16-bit) */
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		fs1816->hw_params.daifmt = 2; /* MSB justified */
		break;
	case SND_SOC_DAIFMT_DSP_A:
		fs1816->hw_params.daifmt = 1; /* DSP mode */
		break;
	case SND_SOC_DAIFMT_DSP_B:
		fs1816->hw_params.daifmt = 1; /* DSP mode */
		break;
	default:
		dev_err(fs1816->dev, "Invalid DAI format:%d\n", fmt);
		return -EINVAL;
	}

	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		fs1816->hw_params.invfmt = 0; /* BCK:0, LRCK:0 */
		break;
	case SND_SOC_DAIFMT_NB_IF:
		fs1816->hw_params.invfmt = 1; /* BCK:0, LRCK:1 */
		break;
	case SND_SOC_DAIFMT_IB_NF:
		fs1816->hw_params.invfmt = 2; /* BCK:1, LRCK:0 */
		break;
	case SND_SOC_DAIFMT_IB_IF:
		fs1816->hw_params.invfmt = 3; /* BCK:1, LRCK:1 */
		break;
	default:
		dev_err(fs1816->dev, "Invalid INV format:%d\n", fmt);
		return -EINVAL;
	}

	return 0;
}

static int fs1816_dai_set_sysclk(struct snd_soc_dai *dai,
		int clk_id, unsigned int freq, int dir)
{
	struct fs1816_dev *fs1816;

	fs1816 = snd_soc_component_get_drvdata(dai->component);
	if (fs1816 == NULL) {
		pr_err("dai_set_sysclk: fs1816 is null\n");
		return -EINVAL;
	}

	if (freq == fs1816->hw_params.bclk)
		return 0;

	dev_info(fs1816->dev, "dai: set sysclk:%d\n", freq);
	fs1816->hw_params.bclk = freq;

	return 0;
}

static int fs1816_dai_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
	struct fs1816_hw_params *hw_params;
	struct fs1816_dev *fs1816;
	unsigned int format;
	int ret;

	if (substream->stream != SNDRV_PCM_STREAM_PLAYBACK)
		return 0;

	fs1816 = snd_soc_component_get_drvdata(dai->component);
	if (fs1816 == NULL) {
		pr_err("dai_hw_params: fs1816 is null\n");
		return -EINVAL;
	}

	mutex_lock(&fs1816_mutex);
	hw_params = &fs1816->hw_params;
	if (hw_params->daifmt == 4) { /* LSB justified (16-bit) */
		if (params_width(params) >= 24)
			hw_params->daifmt = 7; /* LSB justified (24-bit) */
		else if (params_width(params) >= 20)
			hw_params->daifmt = 6; /* LSB justified (20-bit) */
	}

	hw_params->rate = params_rate(params);
	hw_params->channels = params_channels(params);
	format = params_format(params);
	hw_params->bit_width = snd_pcm_format_physical_width(format);

	dev_info(fs1816->dev,
			"dai: RATE:%d CHN:%d WIDTH:%d\n", hw_params->rate,
			hw_params->channels, hw_params->bit_width);

	if (hw_params->channels < 2)
		hw_params->channels = 2;

	if (hw_params->bclk == 0)
		hw_params->bclk = hw_params->rate
				* hw_params->channels
				* hw_params->bit_width;

	ret = fs1816_set_hw_params(fs1816);
	mutex_unlock(&fs1816_mutex);

	FS1816_FUNC_EXIT(fs1816->dev, ret);
	return ret;
}

static int fs1816_dai_mute(struct snd_soc_dai *dai, int mute, int stream)
{
	struct fs1816_dev *fs1816;
	int ret = 0;

	if (stream != SNDRV_PCM_STREAM_PLAYBACK)
		return 0;

	fs1816 = snd_soc_component_get_drvdata(dai->component);
	if (fs1816 == NULL) {
		pr_err("dai_mute: fs1816 is null\n");
		return -EINVAL;
	}

	if ((fs1816->stream_on ^ mute) != 0) {
		dev_info(fs1816->dev, "dai: Skip playback %s\n",
				mute ? "mute" : "unmute");
		return 0;
	}

	if (mute) {
		if (fs1816->restart_work_on)
			cancel_delayed_work_sync(&fs1816->restart_work);
		mutex_lock(&fs1816_mutex);
		fs1816->stream_on = false;
		if (fs1816->dac_event_up)
			ret = fs1816_stop(fs1816);
		dev_info(fs1816->dev, "dai: playback mute\n");
		mutex_unlock(&fs1816_mutex);
	} else {
		mutex_lock(&fs1816_mutex);
		fs1816->stream_on = true;
		if (fs1816->dac_event_up)
			ret = fs1816_play(fs1816);
		dev_info(fs1816->dev, "dai: playback unmute\n");
		mutex_unlock(&fs1816_mutex);
		if (fs1816->restart_work_on)
			queue_delayed_work(fs1816->wq,
					&fs1816->restart_work, 0);
	}

	return ret;
}

static void fs1816_dai_shutdown(struct snd_pcm_substream *substream,
		struct snd_soc_dai *dai)
{
	struct fs1816_dev *fs1816;
	int ret;

	if (substream->stream != SNDRV_PCM_STREAM_PLAYBACK)
		return;

	fs1816 = snd_soc_component_get_drvdata(dai->component);
	if (fs1816 == NULL) {
		pr_err("dai_shutdown: fs1816 is null\n");
		return;
	}

	mutex_lock(&fs1816_mutex);
	if (fs1816->stream_on && fs1816->amp_on)
		ret = fs1816_stop(fs1816);

	if (fs1816->dac_event_up && fs1816->start_up)
		ret = fs1816_shutdown(fs1816);

	fs1816->stream_on = false;
	fs1816->dac_event_up = false;
	fs1816->hw_params.bclk = 0;
	dev_info(fs1816->dev, "dai: shut down\n");
	mutex_unlock(&fs1816_mutex);
}

static const struct snd_soc_dai_ops fs1816_codec_dai_ops = {
	.startup = fs1816_dai_startup,
	.set_sysclk = fs1816_dai_set_sysclk,
	.set_fmt = fs1816_dai_set_fmt,
	.hw_params = fs1816_dai_hw_params,
	.mute_stream = fs1816_dai_mute,
	.shutdown = fs1816_dai_shutdown,
	//.no_capture_mute = 1,
};

static struct snd_soc_dai_driver fs1816_codec_dai = {
	.name = "fs1816-codec-aif",
	.id = 1,
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 2,
		.rates = FS1816_CODEC_RATES,
		.formats = FS1816_CODEC_FORMATS,
	},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 2,
		.rates = FS1816_CODEC_RATES,
		.formats = FS1816_CODEC_FORMATS,
	},
	.ops = &fs1816_codec_dai_ops,
#if KERNEL_VERSION_HIGHER(5, 12, 0)
	.symmetric_rate = 1,
#else
	.symmetric_rates = 1,
#endif
};

static int fs1816_codec_playback_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *cmpnt;
	struct fs1816_dev *fs1816;
	int ret = 0;

	cmpnt = snd_soc_dapm_to_component(w->dapm);
	fs1816 = snd_soc_component_get_drvdata(cmpnt);
	if (fs1816 == NULL)
		return -EINVAL;

	mutex_lock(&fs1816_mutex);
	dev_dbg(fs1816->dev, "Playback event:%d\n", event);
	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		if (!fs1816->dac_event_up)
			ret = fs1816_startup(fs1816);
		fs1816->dac_event_up = true;
		if (fs1816->stream_on)
			ret = fs1816_play(fs1816);
		dev_info(fs1816->dev, "Playback POST_PMU\n");
		break;
	case SND_SOC_DAPM_PRE_PMD:
		if (fs1816->stream_on)
			ret = fs1816_stop(fs1816);
		if (fs1816->dac_event_up)
			ret = fs1816_shutdown(fs1816);
		fs1816->dac_event_up = false;
		dev_info(fs1816->dev, "Playback PRE_PMD\n");
		break;
	default:
		break;
	}
	mutex_unlock(&fs1816_mutex);

	FS1816_FUNC_EXIT(fs1816->dev, ret);
	return ret;
}

static int fs1816_codec_capture_event(struct snd_soc_dapm_widget *w,
	struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_component *cmpnt;
	struct fs1816_dev *fs1816;
	int ret;

	cmpnt = snd_soc_dapm_to_component(w->dapm);
	fs1816 = snd_soc_component_get_drvdata(cmpnt);
	if (fs1816 == NULL)
		return -EINVAL;

	mutex_lock(&fs1816_mutex);
	dev_dbg(fs1816->dev, "Capture event:%d\n", event);
	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		ret = fs1816_reg_update_bits(fs1816, FS1816_04H_I2SCTRL,
				FS1816_04H_I2SDOE_MASK, FS1816_04H_I2SDOE_MASK);
		dev_info(fs1816->dev, "Capture POST_PMU\n");
		break;
	case SND_SOC_DAPM_PRE_PMD:
		ret = fs1816_reg_update_bits(fs1816, FS1816_04H_I2SCTRL,
				FS1816_04H_I2SDOE_MASK, 0);
		dev_info(fs1816->dev, "Capture PRE_PMD\n");
		break;
	default:
		ret = 0;
		break;
	}
	mutex_unlock(&fs1816_mutex);

	FS1816_FUNC_EXIT(fs1816->dev, ret);
	return ret;
}

static const char * const fs1816_switch_state_txt[] = {
	"Off", "On"
};
static const char * const fs1816_bst_mode_txt[] = {
	"Disable", "Boost", "Adaptive", "Follow"
};
static const char * const fs1816_bst_voltage_txt[] = {
	"5.8V", "5.9V", "6.0V", "6.1V", "6.2V", "6.3V",
	"6.4V", "6.5V", "7.0V", "7.5V", "8.0V"
};
static const char * const fs1816_bst_ilimit_txt[] = {
	"0.8A", "1.0A", "1.2A", "1.4A",
	"1.6A", "1.8A", "2.0A", "2.2A", "2.4A"
};
static const char * const fs1816_amp_again_txt[] = {
	"1.5dB", "5dB", "6.5dB", "8dB", "14dB", "18.5dB", "20dB", "21.5dB"
};
static const char * const fs1816_ng_window_txt[] = {
	"200us", "20ms", "40ms", "80ms", "160ms", "320ms", "640ms", "1200ms"
};
static const char * const fs1816_ng_threshold_txt[] = {
	"-78dB", "-84dB", "-90dB", "-96dB", "-102dB",
	"-108dB", "-114dB", "-120dB", "-126dB", "-132dB", "-138dB"
};
static const char * const fs1816_i2s_chn_txt[] = {
	"Left", "Right", "MonoMix"
};
static const char * const fs1816_lnm_fade_time_txt[] = {
	"10us", "40us", "160us", "640us", "2.56ms", "10.24ms"
};
static const char * const fs1816_lnm_check_time_txt[] = {
	"128ms", "256ms", "512ms", "1024ms",
	"2048ms", "4096ms", "6144ms", "200us"
};
static const char * const fs1816_lnm_check_thr_txt[] = {
	"-78dB", "-84dB", "-90dB", "-96dB",
	"-102dB", "-108dB", "-114dB", "-120dB"
};

static const struct soc_enum fs1816_switch_state_enum =
		SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(fs1816_switch_state_txt),
		fs1816_switch_state_txt);
static const struct soc_enum fs1816_bst_mode_enum =
		SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(fs1816_bst_mode_txt),
		fs1816_bst_mode_txt);
static const struct soc_enum fs1816_bst_voltage_enum =
		SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(fs1816_bst_voltage_txt),
		fs1816_bst_voltage_txt);
static const struct soc_enum fs1816_bst_ilimit_enum =
		SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(fs1816_bst_ilimit_txt),
		fs1816_bst_ilimit_txt);
static const struct soc_enum fs1816_amp_again_enum =
		SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(fs1816_amp_again_txt),
		fs1816_amp_again_txt);
static const struct soc_enum fs1816_ng_window_enum =
		SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(fs1816_ng_window_txt),
		fs1816_ng_window_txt);
static const struct soc_enum fs1816_ng_threshold_enum =
		SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(fs1816_ng_threshold_txt),
		fs1816_ng_threshold_txt);
static const struct soc_enum fs1816_i2s_chn_enum =
		SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(fs1816_i2s_chn_txt),
		fs1816_i2s_chn_txt);
static const struct soc_enum fs1816_lnm_fade_time_enum =
		SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(fs1816_lnm_fade_time_txt),
		fs1816_lnm_fade_time_txt);
static const struct soc_enum fs1816_lnm_check_time_enum =
		SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(fs1816_lnm_check_time_txt),
		fs1816_lnm_check_time_txt);
static const struct soc_enum fs1816_lnm_check_thr_enum =
		SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(fs1816_lnm_check_thr_txt),
		fs1816_lnm_check_thr_txt);

static int fs1816_amp_switch_get(struct snd_kcontrol *kc,
		struct snd_ctl_elem_value *uc)
{
	struct fs1816_dev *fs1816;

	fs1816 = snd_soc_component_get_drvdata(snd_soc_kcontrol_component(kc));
	if (fs1816 == NULL) {
		pr_err("switch_get: fs1816 is null");
		return -EINVAL;
	}

	mutex_lock(&fs1816_mutex);
	uc->value.integer.value[0] = (fs1816->amp_on ? 1 : 0);
	mutex_unlock(&fs1816_mutex);

	return 0;
}

static int fs1816_amp_switch_put(struct snd_kcontrol *kc,
		struct snd_ctl_elem_value *uc)
{
	struct fs1816_dev *fs1816;
	bool enable;
	int ret;

	fs1816 = snd_soc_component_get_drvdata(snd_soc_kcontrol_component(kc));
	if (fs1816 == NULL) {
		pr_err("switch_put: fs1816 is null");
		return -EINVAL;
	}
	enable = !!uc->value.integer.value[0];
	if ((fs1816->amp_on ^ enable) == 0)
		return 0;

	mutex_lock(&fs1816_mutex);
	if (enable) {
		ret  = fs1816_startup(fs1816);
		ret |= fs1816_play(fs1816);
		FS1816_DELAY_MS(FS1816_FADE_IN_MS);
	} else {
		ret  = fs1816_stop(fs1816);
		ret |= fs1816_shutdown(fs1816);
	}
	mutex_unlock(&fs1816_mutex);

	return ret;
}

static int fs1816_boost_mode_get(struct snd_kcontrol *kc,
		struct snd_ctl_elem_value *uc)
{
	struct fs1816_dev *fs1816;
	uint16_t val;
	int ret;

	fs1816 = snd_soc_component_get_drvdata(snd_soc_kcontrol_component(kc));
	if (fs1816 == NULL) {
		pr_err("boost_mode_get: fs1816 is null");
		return -EINVAL;
	}

	mutex_lock(&fs1816_mutex);
	ret = fs1816_reg_read(fs1816, FS1816_C0H_BSTCTRL, &val);
	mutex_unlock(&fs1816_mutex);
	if (((val & FS1816_C0H_BSTEN_MASK) >> FS1816_C0H_BSTEN_SHIFT) == 0) {
		uc->value.integer.value[0] = 0;
		return ret;
	}

	val = (val & FS1816_C0H_MODE_CTRL_MASK) >> FS1816_C0H_MODE_CTRL_SHIFT;
	uc->value.integer.value[0] = (val == 0 ? 3 : val);

	return ret;
}

static int fs1816_boost_mode_put(struct snd_kcontrol *kc,
		struct snd_ctl_elem_value *uc)
{
	struct fs1816_dev *fs1816;
	struct soc_enum *e = (struct soc_enum *)kc->private_value;
	unsigned int mode = uc->value.enumerated.item[0];
	int ret;

	fs1816 = snd_soc_component_get_drvdata(snd_soc_kcontrol_component(kc));
	if (fs1816 == NULL) {
		pr_err("boost_mode_put: fs1816 is null");
		return -EINVAL;
	}

	if (mode > e->items) {
		dev_err(fs1816->dev, "invalid mode:%d", mode);
		return -EINVAL;
	}

	mutex_lock(&fs1816_mutex);
	ret = fs1816_apply(fs1816, fs1816_set_boost_mode, mode);
	mutex_unlock(&fs1816_mutex);
	if (ret)
		dev_err(fs1816->dev, "Failed to set boost mode:%d\n", ret);

	return ret;
}

static int fs1816_boost_voltage_get(struct snd_kcontrol *kc,
		struct snd_ctl_elem_value *uc)
{
	struct fs1816_dev *fs1816;
	uint16_t val;
	int ret;

	fs1816 = snd_soc_component_get_drvdata(snd_soc_kcontrol_component(kc));
	if (fs1816 == NULL) {
		pr_err("boost_voltage_get: fs1816 is null");
		return -EINVAL;
	}

	mutex_lock(&fs1816_mutex);
	ret = fs1816_reg_read(fs1816, FS1816_C0H_BSTCTRL, &val);
	val = (val & FS1816_C0H_VOUT_SEL_MASK) >> FS1816_C0H_VOUT_SEL_SHIFT;
	uc->value.integer.value[0] = val;
	mutex_unlock(&fs1816_mutex);

	return ret;
}

static int fs1816_boost_voltage_put(struct snd_kcontrol *kc,
		struct snd_ctl_elem_value *uc)
{
	struct fs1816_dev *fs1816;
	struct soc_enum *e = (struct soc_enum *)kc->private_value;
	unsigned int vout = uc->value.enumerated.item[0];
	int ret;

	fs1816 = snd_soc_component_get_drvdata(snd_soc_kcontrol_component(kc));
	if (fs1816 == NULL) {
		pr_err("boost_voltage_put: fs1816 is null");
		return -EINVAL;
	}

	if (vout > e->items) {
		dev_err(fs1816->dev, "invalid vout:%d", vout);
		return -EINVAL;
	}

	mutex_lock(&fs1816_mutex);
	ret = fs1816_apply(fs1816, fs1816_set_boost_voltage, vout);
	mutex_unlock(&fs1816_mutex);
	if (ret)
		dev_err(fs1816->dev, "Failed to set boost vout:%d\n", ret);

	return ret;
}

static int fs1816_boost_ilimit_get(struct snd_kcontrol *kc,
		struct snd_ctl_elem_value *uc)
{
	struct fs1816_dev *fs1816;
	uint16_t val;
	int ret;

	fs1816 = snd_soc_component_get_drvdata(snd_soc_kcontrol_component(kc));
	if (fs1816 == NULL) {
		pr_err("boost_ilimit_get: fs1816 is null");
		return -EINVAL;
	}

	mutex_lock(&fs1816_mutex);
	ret = fs1816_reg_read(fs1816, FS1816_C0H_BSTCTRL, &val);
	val = (val & FS1816_C0H_ILIM_SEL_MASK) >> FS1816_C0H_ILIM_SEL_SHIFT;
	uc->value.integer.value[0] = val;
	mutex_unlock(&fs1816_mutex);

	return ret;
}

static int fs1816_boost_ilimit_put(struct snd_kcontrol *kc,
		struct snd_ctl_elem_value *uc)
{
	struct fs1816_dev *fs1816;
	struct soc_enum *e = (struct soc_enum *)kc->private_value;
	unsigned int ilim = uc->value.enumerated.item[0];
	int ret;

	fs1816 = snd_soc_component_get_drvdata(snd_soc_kcontrol_component(kc));
	if (fs1816 == NULL) {
		pr_err("boost_ilimit_put: fs1816 is null");
		return -EINVAL;
	}

	if (ilim > e->items) {
		dev_err(fs1816->dev, "invalid ilim:%d", ilim);
		return -EINVAL;
	}

	mutex_lock(&fs1816_mutex);
	ret = fs1816_apply(fs1816, fs1816_set_boost_ilimit, ilim);
	mutex_unlock(&fs1816_mutex);
	if (ret)
		dev_err(fs1816->dev, "Failed to set boost ilim:%d\n", ret);

	return ret;
}

static int fs1816_amp_again_get(struct snd_kcontrol *kc,
		struct snd_ctl_elem_value *uc)
{
	struct fs1816_dev *fs1816;
	uint16_t val;
	int ret;

	fs1816 = snd_soc_component_get_drvdata(snd_soc_kcontrol_component(kc));
	if (fs1816 == NULL) {
		pr_err("amp_again_get: fs1816 is null");
		return -EINVAL;
	}

	mutex_lock(&fs1816_mutex);
	ret = fs1816_reg_read(fs1816, FS1816_C0H_BSTCTRL, &val);
	val = (val & FS1816_C0H_AGAIN_MASK) >> FS1816_C0H_AGAIN_SHIFT;
	uc->value.integer.value[0] = val;
	mutex_unlock(&fs1816_mutex);

	return ret;
}

static int fs1816_amp_again_put(struct snd_kcontrol *kc,
		struct snd_ctl_elem_value *uc)
{
	struct fs1816_dev *fs1816;
	struct soc_enum *e = (struct soc_enum *)kc->private_value;
	unsigned int gain = uc->value.enumerated.item[0];
	int ret;

	fs1816 = snd_soc_component_get_drvdata(snd_soc_kcontrol_component(kc));
	if (fs1816 == NULL) {
		pr_err("amp_again_put: fs1816 is null");
		return -EINVAL;
	}

	if (gain > e->items) {
		dev_err(fs1816->dev, "invalid gain:%d", gain);
		return -EINVAL;
	}

	mutex_lock(&fs1816_mutex);
	ret = fs1816_apply(fs1816, fs1816_set_amp_again, gain);
	mutex_unlock(&fs1816_mutex);
	if (ret)
		dev_err(fs1816->dev, "Failed to set amp again:%d\n", ret);

	return ret;
}

static int fs1816_ng_window_get(struct snd_kcontrol *kc,
		struct snd_ctl_elem_value *uc)
{
	struct fs1816_dev *fs1816;
	uint16_t val;
	int ret;

	fs1816 = snd_soc_component_get_drvdata(snd_soc_kcontrol_component(kc));
	if (fs1816 == NULL) {
		pr_err("ng_window_get: fs1816 is null");
		return -EINVAL;
	}

	mutex_lock(&fs1816_mutex);
	ret = fs1816_reg_read(fs1816, FS1816_96H_NGCTRL, &val);
	val = (val & FS1816_96H_NGDLY_MASK) >> FS1816_96H_NGDLY_SHIFT;
	uc->value.integer.value[0] = val;
	mutex_unlock(&fs1816_mutex);

	return ret;
}

static int fs1816_ng_window_put(struct snd_kcontrol *kc,
		struct snd_ctl_elem_value *uc)
{
	struct fs1816_dev *fs1816;
	struct soc_enum *e = (struct soc_enum *)kc->private_value;
	unsigned int delay = uc->value.enumerated.item[0];
	int ret;

	fs1816 = snd_soc_component_get_drvdata(snd_soc_kcontrol_component(kc));
	if (fs1816 == NULL) {
		pr_err("ng_window_put: fs1816 is null");
		return -EINVAL;
	}

	if (delay > e->items) {
		dev_err(fs1816->dev, "invalid ng window:%d", delay);
		return -EINVAL;
	}

	mutex_lock(&fs1816_mutex);
	ret = fs1816_apply(fs1816, fs1816_set_ng_window, delay);
	mutex_unlock(&fs1816_mutex);
	if (ret)
		dev_err(fs1816->dev, "Failed to set ng window:%d\n", ret);

	return ret;
}

static int fs1816_ng_threshold_get(struct snd_kcontrol *kc,
		struct snd_ctl_elem_value *uc)
{
	struct fs1816_dev *fs1816;
	uint16_t val;
	int ret;

	fs1816 = snd_soc_component_get_drvdata(snd_soc_kcontrol_component(kc));
	if (fs1816 == NULL) {
		pr_err("ng_threshold_get: fs1816 is null");
		return -EINVAL;
	}

	mutex_lock(&fs1816_mutex);
	ret = fs1816_reg_read(fs1816, FS1816_96H_NGCTRL, &val);
	val = (val & FS1816_96H_NGTHD_MASK) >> FS1816_96H_NGTHD_SHIFT;
	uc->value.integer.value[0] = val;
	mutex_unlock(&fs1816_mutex);

	return ret;
}

static int fs1816_ng_threshold_put(struct snd_kcontrol *kc,
		struct snd_ctl_elem_value *uc)
{
	struct fs1816_dev *fs1816;
	struct soc_enum *e = (struct soc_enum *)kc->private_value;
	unsigned int thrd = uc->value.enumerated.item[0];
	int ret;

	fs1816 = snd_soc_component_get_drvdata(snd_soc_kcontrol_component(kc));
	if (fs1816 == NULL) {
		pr_err("ng_threshold_put: fs1816 is null");
		return -EINVAL;
	}

	if (thrd > e->items) {
		dev_err(fs1816->dev, "invalid ng threshold:%d", thrd);
		return -EINVAL;
	}

	mutex_lock(&fs1816_mutex);
	ret = fs1816_apply(fs1816, fs1816_set_ng_threshold, thrd);
	mutex_unlock(&fs1816_mutex);
	if (ret)
		dev_err(fs1816->dev, "Failed to set ng threshold:%d\n", ret);

	return ret;
}

static int fs1816_i2s_chn_get(struct snd_kcontrol *kc,
		struct snd_ctl_elem_value *uc)
{
	struct fs1816_dev *fs1816;
	uint16_t val;
	int ret;

	fs1816 = snd_soc_component_get_drvdata(snd_soc_kcontrol_component(kc));
	if (fs1816 == NULL) {
		pr_err("i2s_chn_get: fs1816 is null");
		return -EINVAL;
	}

	mutex_lock(&fs1816_mutex);
	ret = fs1816_reg_read(fs1816, FS1816_04H_I2SCTRL, &val);
	val = (val & FS1816_04H_CHS12_MASK) >> FS1816_04H_CHS12_SHIFT;
	if (val <= 1)
		val = 0;
	else
		val -= 1;
	uc->value.integer.value[0] = val;
	mutex_unlock(&fs1816_mutex);

	return ret;
}

static int fs1816_i2s_chn_put(struct snd_kcontrol *kc,
		struct snd_ctl_elem_value *uc)
{
	struct fs1816_dev *fs1816;
	struct soc_enum *e = (struct soc_enum *)kc->private_value;
	unsigned int chn_sel = uc->value.enumerated.item[0];
	int ret;

	fs1816 = snd_soc_component_get_drvdata(snd_soc_kcontrol_component(kc));
	if (fs1816 == NULL) {
		pr_err("i2s_chn_put: fs1816 is null");
		return -EINVAL;
	}

	if (chn_sel > e->items) {
		dev_err(fs1816->dev, "invalid i2s chn sel:%d", chn_sel);
		return -EINVAL;
	}

	mutex_lock(&fs1816_mutex);
	ret = fs1816_apply(fs1816, fs1816_set_i2s_chn_sel, chn_sel);
	mutex_unlock(&fs1816_mutex);
	if (ret)
		dev_err(fs1816->dev, "Failed to set i2s chn sel:%d\n", ret);

	return ret;
}

static int fs1816_lnm_fo_time_get(struct snd_kcontrol *kc,
		struct snd_ctl_elem_value *uc)
{
	struct fs1816_dev *fs1816;
	uint16_t val;
	int ret;

	fs1816 = snd_soc_component_get_drvdata(snd_soc_kcontrol_component(kc));
	if (fs1816 == NULL) {
		pr_err("lnm_fo_time_get: fs1816 is null");
		return -EINVAL;
	}

	mutex_lock(&fs1816_mutex);
	ret = fs1816_reg_read(fs1816, FS1816_98H_LNMCTRL, &val);
	val = (val & FS1816_98H_FOTIME_MASK) >> FS1816_98H_FOTIME_SHIFT;
	uc->value.integer.value[0] = val;
	mutex_unlock(&fs1816_mutex);

	return ret;
}

static int fs1816_lnm_fo_time_put(struct snd_kcontrol *kc,
		struct snd_ctl_elem_value *uc)
{
	struct fs1816_dev *fs1816;
	struct soc_enum *e = (struct soc_enum *)kc->private_value;
	unsigned int fo_time = uc->value.enumerated.item[0];
	int ret;

	fs1816 = snd_soc_component_get_drvdata(snd_soc_kcontrol_component(kc));
	if (fs1816 == NULL) {
		pr_err("lnm_fo_time_put: fs1816 is null");
		return -EINVAL;
	}

	if (fo_time > e->items) {
		dev_err(fs1816->dev, "invalid lnm fo time:%d", fo_time);
		return -EINVAL;
	}

	mutex_lock(&fs1816_mutex);
	ret = fs1816_apply(fs1816, fs1816_set_lnm_fo_time, fo_time);
	mutex_unlock(&fs1816_mutex);
	if (ret)
		dev_err(fs1816->dev, "Failed to set lnm fo time:%d\n", ret);

	return ret;
}

static int fs1816_lnm_fi_time_get(struct snd_kcontrol *kc,
		struct snd_ctl_elem_value *uc)
{
	struct fs1816_dev *fs1816;
	uint16_t val;
	int ret;

	fs1816 = snd_soc_component_get_drvdata(snd_soc_kcontrol_component(kc));
	if (fs1816 == NULL) {
		pr_err("lnm_fi_time_get: fs1816 is null");
		return -EINVAL;
	}

	mutex_lock(&fs1816_mutex);
	ret = fs1816_reg_read(fs1816, FS1816_98H_LNMCTRL, &val);
	val = (val & FS1816_98H_FITIME_MASK) >> FS1816_98H_FITIME_SHIFT;
	uc->value.integer.value[0] = val;
	mutex_unlock(&fs1816_mutex);

	return ret;
}

static int fs1816_lnm_fi_time_put(struct snd_kcontrol *kc,
		struct snd_ctl_elem_value *uc)
{
	struct fs1816_dev *fs1816;
	struct soc_enum *e = (struct soc_enum *)kc->private_value;
	unsigned int fi_time = uc->value.enumerated.item[0];
	int ret;

	fs1816 = snd_soc_component_get_drvdata(snd_soc_kcontrol_component(kc));
	if (fs1816 == NULL) {
		pr_err("lnm_fi_time_put: fs1816 is null");
		return -EINVAL;
	}

	if (fi_time > e->items) {
		dev_err(fs1816->dev, "invalid lnm fi time:%d", fi_time);
		return -EINVAL;
	}

	mutex_lock(&fs1816_mutex);
	ret = fs1816_apply(fs1816, fs1816_set_lnm_fi_time, fi_time);
	mutex_unlock(&fs1816_mutex);
	if (ret)
		dev_err(fs1816->dev, "Failed to set lnm fi time:%d\n", ret);

	return ret;
}

static int fs1816_lnm_check_time_get(struct snd_kcontrol *kc,
		struct snd_ctl_elem_value *uc)
{
	struct fs1816_dev *fs1816;
	uint16_t val;
	int ret;

	fs1816 = snd_soc_component_get_drvdata(snd_soc_kcontrol_component(kc));
	if (fs1816 == NULL) {
		pr_err("lnm_check_time_get: fs1816 is null");
		return -EINVAL;
	}

	mutex_lock(&fs1816_mutex);
	ret = fs1816_reg_read(fs1816, FS1816_97H_AUTOCTRL, &val);
	val = (val & FS1816_97H_CHKTIME_MASK) >> FS1816_97H_CHKTIME_SHIFT;
	uc->value.integer.value[0] = val;
	mutex_unlock(&fs1816_mutex);

	return ret;
}

static int fs1816_lnm_check_time_put(struct snd_kcontrol *kc,
		struct snd_ctl_elem_value *uc)
{
	struct fs1816_dev *fs1816;
	struct soc_enum *e = (struct soc_enum *)kc->private_value;
	unsigned int check_time = uc->value.enumerated.item[0];
	int ret;

	fs1816 = snd_soc_component_get_drvdata(snd_soc_kcontrol_component(kc));
	if (fs1816 == NULL) {
		pr_err("lnm_check_time_put: fs1816 is null");
		return -EINVAL;
	}

	if (check_time > e->items) {
		dev_err(fs1816->dev, "invalid lnm check time:%d", check_time);
		return -EINVAL;
	}

	mutex_lock(&fs1816_mutex);
	ret = fs1816_apply(fs1816, fs1816_set_lnm_check_time, check_time);
	mutex_unlock(&fs1816_mutex);
	if (ret)
		dev_err(fs1816->dev, "Failed to set lnm check time:%d\n", ret);

	return ret;
}

static int fs1816_lnm_check_thr_get(struct snd_kcontrol *kc,
		struct snd_ctl_elem_value *uc)
{
	struct fs1816_dev *fs1816;
	uint16_t val;
	int ret;

	fs1816 = snd_soc_component_get_drvdata(snd_soc_kcontrol_component(kc));
	if (fs1816 == NULL) {
		pr_err("lnm_check_thr_get: fs1816 is null");
		return -EINVAL;
	}

	mutex_lock(&fs1816_mutex);
	ret = fs1816_reg_read(fs1816, FS1816_97H_AUTOCTRL, &val);
	val = (val & FS1816_97H_CHKTHR_MASK) >> FS1816_97H_CHKTHR_SHIFT;
	uc->value.integer.value[0] = val;
	mutex_unlock(&fs1816_mutex);

	return ret;
}

static int fs1816_lnm_check_thr_put(struct snd_kcontrol *kc,
		struct snd_ctl_elem_value *uc)
{
	struct fs1816_dev *fs1816;
	struct soc_enum *e = (struct soc_enum *)kc->private_value;
	unsigned int check_thr = uc->value.enumerated.item[0];
	int ret;

	fs1816 = snd_soc_component_get_drvdata(snd_soc_kcontrol_component(kc));
	if (fs1816 == NULL) {
		pr_err("lnm_check_thr_put: fs1816 is null");
		return -EINVAL;
	}

	if (check_thr > e->items) {
		dev_err(fs1816->dev, "Invalid lnm check thr:%d", check_thr);
		return -EINVAL;
	}

	mutex_lock(&fs1816_mutex);
	ret = fs1816_apply(fs1816, fs1816_set_lnm_check_thr, check_thr);
	mutex_unlock(&fs1816_mutex);
	if (ret)
		dev_err(fs1816->dev, "Failed to set lnm check thr:%d\n", ret);

	return ret;
}

static int fs1816_volume_get(struct snd_kcontrol *kc,
		struct snd_ctl_elem_value *uc)
{
	struct fs1816_dev *fs1816;
	uint16_t val;
	int ret;

	fs1816 = snd_soc_component_get_drvdata(snd_soc_kcontrol_component(kc));
	if (fs1816 == NULL) {
		pr_err("volume_get: fs1816 is null");
		return -EINVAL;
	}

	mutex_lock(&fs1816_mutex);
	ret = fs1816_reg_read(fs1816, FS1816_06H_AUDIOCTRL, &val);
	val = (val & FS1816_06H_VOL_MASK) >> FS1816_06H_VOL_SHIFT;
	uc->value.integer.value[0] = val;
	mutex_unlock(&fs1816_mutex);

	return ret;
}

static int fs1816_volume_put(struct snd_kcontrol *kc,
		struct snd_ctl_elem_value *uc)
{
	struct fs1816_dev *fs1816;
	unsigned int volume = uc->value.enumerated.item[0];
	int ret;

	fs1816 = snd_soc_component_get_drvdata(snd_soc_kcontrol_component(kc));
	if (fs1816 == NULL) {
		pr_err("volume_put: fs1816 is null");
		return -EINVAL;
	}

	mutex_lock(&fs1816_mutex);
	if (volume > 0x3FF) // max: bit15..6: 0x3FF
		volume = 0x3FF;

	ret = fs1816_reg_write(fs1816, FS1816_06H_AUDIOCTRL,
			volume << FS1816_06H_VOL_SHIFT);
	fs1816->rx_volume = volume;
	mutex_unlock(&fs1816_mutex);
	if (ret)
		dev_err(fs1816->dev, "Failed to set volume:%d\n", ret);

	return ret;
}

static const struct snd_kcontrol_new fs1816_codec_controls[] = {
	SOC_ENUM_EXT("fs1816_amp_switch", fs1816_switch_state_enum,
			fs1816_amp_switch_get, fs1816_amp_switch_put),
	SOC_ENUM_EXT("fs1816_boost_mode", fs1816_bst_mode_enum,
			fs1816_boost_mode_get, fs1816_boost_mode_put),
	SOC_ENUM_EXT("fs1816_boost_voltage", fs1816_bst_voltage_enum,
			fs1816_boost_voltage_get, fs1816_boost_voltage_put),
	SOC_ENUM_EXT("fs1816_boost_ilimit", fs1816_bst_ilimit_enum,
			fs1816_boost_ilimit_get, fs1816_boost_ilimit_put),
	SOC_ENUM_EXT("fs1816_amp_again", fs1816_amp_again_enum,
			fs1816_amp_again_get, fs1816_amp_again_put),
	SOC_ENUM_EXT("fs1816_ng_window", fs1816_ng_window_enum,
			fs1816_ng_window_get, fs1816_ng_window_put),
	SOC_ENUM_EXT("fs1816_ng_threshold", fs1816_ng_threshold_enum,
			fs1816_ng_threshold_get, fs1816_ng_threshold_put),
	SOC_ENUM_EXT("fs1816_i2s_chn", fs1816_i2s_chn_enum,
				fs1816_i2s_chn_get, fs1816_i2s_chn_put),
	SOC_ENUM_EXT("fs1816_lnm_fo_time", fs1816_lnm_fade_time_enum,
			fs1816_lnm_fo_time_get, fs1816_lnm_fo_time_put),
	SOC_ENUM_EXT("fs1816_lnm_fi_time", fs1816_lnm_fade_time_enum,
			fs1816_lnm_fi_time_get, fs1816_lnm_fi_time_put),
	SOC_ENUM_EXT("fs1816_lnm_check_time", fs1816_lnm_check_time_enum,
			fs1816_lnm_check_time_get, fs1816_lnm_check_time_put),
	SOC_ENUM_EXT("fs1816_lnm_check_thr", fs1816_lnm_check_thr_enum,
			fs1816_lnm_check_thr_get, fs1816_lnm_check_thr_put),
	SOC_SINGLE_EXT("fs1816_volume", SND_SOC_NOPM, 0, 0x3FF, 0,
			fs1816_volume_get, fs1816_volume_put),
	SOC_SINGLE("fs1816_i2s_out", FS1816_04H_I2SCTRL, 11, 1, 0),
	SOC_SINGLE("fs1816_rtz_switch", FS1816_27H_ANACFG, 11, 1, 0),
	SOC_SINGLE("fs1816_ng_switch", FS1816_96H_NGCTRL, 15, 1, 0),
	SOC_SINGLE("fs1816_lnm_switch", FS1816_98H_LNMCTRL, 15, 1, 0),
	SOC_SINGLE("fs1816_bclk_invert", FS1816_A0H_I2SSET, 6, 1, 0),
	SOC_SINGLE("fs1816_ws_invert", FS1816_A0H_I2SSET, 5, 1, 0),
	SOC_SINGLE("fs1816_mute", FS1816_AEH_DACCTRL, 8, 1, 0),
	SOC_SINGLE("fs1816_fade", FS1816_AEH_DACCTRL, 9, 1, 0),
};

static const struct snd_kcontrol_new fs1816_dac_port[] = {
	SOC_DAPM_SINGLE("Switch", SND_SOC_NOPM, 0, 1, 0)
};

static const struct snd_kcontrol_new fs1816_fb_port[] = {
	SOC_DAPM_SINGLE("Switch", SND_SOC_NOPM, 0, 1, 0)
};

static struct snd_soc_dapm_widget fs1816_codec_widgets[] = {
	SND_SOC_DAPM_AIF_IN("AIF IN", "Playback", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_AIF_OUT("AIF OUT", "Capture", 0, SND_SOC_NOPM, 0, 0),
	SND_SOC_DAPM_MIXER_E("FS DAC_Port", SND_SOC_NOPM, 0, 0,
		fs1816_dac_port, ARRAY_SIZE(fs1816_dac_port),
		fs1816_codec_playback_event,
		SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),
	SND_SOC_DAPM_MIXER_E("FS FB_Port", SND_SOC_NOPM, 0, 0,
		fs1816_fb_port, ARRAY_SIZE(fs1816_fb_port),
		fs1816_codec_capture_event,
		SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),
	SND_SOC_DAPM_OUTPUT("OUT"),
	SND_SOC_DAPM_INPUT("SDO"),
};

static const struct snd_soc_dapm_route fs1816_codec_routes[] = {
	{ "FS DAC_Port", "Switch", "AIF IN" },
	{ "OUT", NULL, "FS DAC_Port" },
	{ "FS FB_Port", "Switch", "SDO" },
	{ "AIF OUT", NULL, "FS FB_Port" },
};

static int fs1816_probe_widgets(struct snd_soc_component *cmpnt,
		struct snd_soc_dapm_context *dapm)
{
	const char *str_prefix;
	char *name;
	int ret;

	if (cmpnt == NULL || dapm == NULL)
		return -EINVAL;

	ret = snd_soc_dapm_new_controls(dapm, fs1816_codec_widgets,
			ARRAY_SIZE(fs1816_codec_widgets));
	ret |= snd_soc_add_component_controls(cmpnt, fs1816_codec_controls,
			ARRAY_SIZE(fs1816_codec_controls));
	ret |= snd_soc_dapm_add_routes(dapm, fs1816_codec_routes,
			ARRAY_SIZE(fs1816_codec_routes));
	if (ret)
		return ret;

	if (cmpnt->name_prefix == NULL) {
		snd_soc_dapm_ignore_suspend(dapm, "Playback");
		snd_soc_dapm_ignore_suspend(dapm, "OUT");
		snd_soc_dapm_sync(dapm);
		return 0;
	}

	name = kzalloc(FS1816_STRING_MAX, GFP_KERNEL);
	if (name == NULL)
		return -ENOMEM;

	str_prefix = cmpnt->name_prefix;
	snprintf(name, FS1816_STRING_MAX, "%s Playback", str_prefix);
	snd_soc_dapm_ignore_suspend(dapm, name);
	snprintf(name, FS1816_STRING_MAX, "%s OUT", str_prefix);
	snd_soc_dapm_ignore_suspend(dapm, name);
	kfree(name);

	snd_soc_dapm_sync(dapm);

	return 0;
}

static void fs1816_restart_work(struct work_struct *work)
{
	struct fs1816_dev *fs1816;
	int ret;

	if (!mutex_trylock(&fs1816_mutex))
		return;

	fs1816 = container_of(work, struct fs1816_dev, restart_work.work);

	fs1816->rec_count++;
	dev_info(fs1816->dev, "Recover device:%d\n", fs1816->rec_count);
	ret = fs1816_recover_device(fs1816);
	if (ret)
		dev_err(fs1816->dev, "Failed to cold start:%d\n", ret);

	mutex_unlock(&fs1816_mutex);
}

static void fs1816_interrupt_work(struct work_struct *work)
{
	struct fs1816_dev *fs1816;
	uint16_t intstat, val;
	bool rec_flag = false;
	int ret;

	if (work == NULL)
		return;

	if (!mutex_trylock(&fs1816_mutex))
		return;

	fs1816 = container_of(work, struct fs1816_dev, irq_work.work);

	ret = fs1816_reg_read(fs1816, FS1816_58H_INTSTAT, &intstat);
	ret |= fs1816_reg_read(fs1816, FS1816_08H_TEMPSEL, &val);

	if (intstat & FS1816_58H_INTS13_MASK)
		dev_err(fs1816->dev, "OC detected\n");
	if (intstat & FS1816_58H_INTS12_MASK)
		dev_err(fs1816->dev, "UV detected\n");
	if (intstat & FS1816_58H_INTS10_MASK)
		dev_err(fs1816->dev, "OTP detected\n");
	if (intstat & FS1816_58H_INTS9_MASK)
		dev_err(fs1816->dev, "BOV detected\n");
	if (intstat & FS1816_58H_INTS1_MASK)
		dev_err(fs1816->dev, "Clock lost detected\n");

	if (intstat)
		ret = fs1816_reg_dump(fs1816, 0xCF);

	if (!ret && val == FS1816_08H_TEMPSEL_DEFAULT) {
		fs1816->restart_work_on = true;
		rec_flag = true;
	}

	mutex_unlock(&fs1816_mutex);

	if (rec_flag && fs1816->rec_count < FS1816_RECOVER_MAX)
		queue_delayed_work(fs1816->wq, &fs1816->restart_work, 0);
}

static irqreturn_t fs1816_irq_handler(int irq, void *data)
{
	struct fs1816_dev *fs1816 = data;

	if (fs1816 == NULL)
		return IRQ_HANDLED;

	dev_info(fs1816->dev, "irq handling...\n");
	queue_delayed_work(fs1816->wq, &fs1816->irq_work, 0);

	return IRQ_HANDLED;
}

static int fs1816_irq_switch(struct fs1816_dev *fs1816, bool enable)
{
	if (fs1816 == NULL)
		return -EINVAL;

	if (fs1816->irq_id < 0)
		return 0;

	if (enable) {
		enable_irq(fs1816->irq_id);
		dev_dbg(fs1816->dev, "IRQ enabled\n");
	} else {
		disable_irq(fs1816->irq_id);
		dev_dbg(fs1816->dev, "IRQ disabled\n");
		cancel_delayed_work_sync(&fs1816->irq_work);
	}

	return 0;
}

static int fs1816_request_irq(struct fs1816_dev *fs1816)
{
	unsigned long irq_flags;
	int ret;

	if (IS_ERR_OR_NULL(fs1816->pdata.gpio_intz)) {
		dev_info(fs1816->dev, "Skip to request IRQ\n");
		fs1816->irq_id = -1;
		return 0;
	}

	fs1816->irq_id = gpiod_to_irq(fs1816->pdata.gpio_intz);
	if (fs1816->irq_id < 0) {
		dev_err(fs1816->dev, "Invalid irq id:%d\n", fs1816->irq_id);
		return fs1816->irq_id;
	}

	dev_info(fs1816->dev, "IRQ ID:%d\n", fs1816->irq_id);
	irq_flags = IRQF_TRIGGER_FALLING | IRQF_ONESHOT;
	ret = devm_request_threaded_irq(fs1816->dev,
			fs1816->irq_id, NULL, fs1816_irq_handler,
			irq_flags, dev_name(fs1816->dev), fs1816);
	if (ret) {
		dev_err(fs1816->dev, "Failed to request IRQ%d: %d\n",
				fs1816->irq_id, ret);
		return ret;
	}

	fs1816_irq_switch(fs1816, false);

	return 0;
}

#if IS_ENABLED(CONFIG_SND_SOC_SAMSUNG_AUDIO)
static void fs1816_monitor_work(struct work_struct *work)
{
	struct fs1816_dev *fs1816;
	uint16_t val;
	int ret;

	if (work == NULL)
		return;

	if (!mutex_trylock(&fs1816_mutex))
		return;

	fs1816 = container_of(work, struct fs1816_dev, monitor_work.work);
	ret = fs1816_reg_read(fs1816, 0x00, &val);
	mutex_unlock(&fs1816_mutex);

	if (ret || ((val & (1 << 6)) == 0))
		queue_delayed_work(fs1816->wq, &fs1816->monitor_work, 1);
	else
		dev_err(fs1816->dev, "%s: lrclk is stable [%04X]\n", __func__, val);
}
#endif

static int fs1816_codec_probe(struct snd_soc_component *cmpnt)
{
	struct snd_soc_dapm_context *dapm;
	struct fs1816_dev *fs1816;
	int ret;

	fs1816 = snd_soc_component_get_drvdata(cmpnt);
	if (fs1816 == NULL) {
		pr_err("codec_probe: fs1816 is null\n");
		return -EINVAL;
	}

	dapm = snd_soc_component_get_dapm(cmpnt);
	ret = fs1816_probe_widgets(cmpnt, dapm);
	if (ret) {
		dev_err(fs1816->dev, "Failed to probe widgets:%d\n", ret);
		return ret;
	}

	mutex_lock(&fs1816_mutex);
	fs1816->force_init = true;
	ret = fs1816_chip_init(fs1816);
	mutex_unlock(&fs1816_mutex);
	if (ret) {
		dev_err(fs1816->dev, "Failed to init chip:%d\n", ret);
		return ret;
	}

	fs1816->wq = create_singlethread_workqueue(dev_name(fs1816->dev));
	INIT_DELAYED_WORK(&fs1816->restart_work, fs1816_restart_work);
	INIT_DELAYED_WORK(&fs1816->irq_work, fs1816_interrupt_work);
#if IS_ENABLED(CONFIG_SND_SOC_SAMSUNG_AUDIO)
	INIT_DELAYED_WORK(&fs1816->monitor_work, fs1816_monitor_work);
#endif
	ret = fs1816_request_irq(fs1816);
	dev_info(fs1816->dev, "codec probed\n");

	FS1816_FUNC_EXIT(fs1816->dev, ret);
	return ret;
}

static void fs1816_codec_remove(struct snd_soc_component *cmpnt)
{
	struct fs1816_dev *fs1816;

	fs1816 = snd_soc_component_get_drvdata(cmpnt);
	if (fs1816 == NULL)
		return;

	if (fs1816->irq_id > 0)
		devm_free_irq(fs1816->dev, fs1816->irq_id, fs1816);

	dev_info(fs1816->dev, "cmpnt removed\n");
}

static const struct snd_soc_component_driver fs1816_cmpnt_drv = {
	.name = "fs1816-codec",
	.probe = fs1816_codec_probe,
	.remove = fs1816_codec_remove,
};

static int fs1816_codec_init(struct fs1816_dev *fs1816)
{
	struct snd_soc_dai_driver *dai_drv;
	int ret;

	if (fs1816 == NULL || fs1816->dev == NULL)
		return -EINVAL;

	dai_drv = devm_kzalloc(fs1816->dev,
			sizeof(struct snd_soc_dai_driver), GFP_KERNEL);
	if (dai_drv == NULL)
		return -ENOMEM;

	memcpy(dai_drv, &fs1816_codec_dai, sizeof(fs1816_codec_dai));
	ret = snd_soc_register_component(fs1816->dev,
			&fs1816_cmpnt_drv, dai_drv, 1);

	dev_info(fs1816->dev, "dev_name:%s dai:%s\n",
			dev_name(fs1816->dev), dai_drv->name);

	FS1816_FUNC_EXIT(fs1816->dev, ret);
	return ret;
}

static void fs1816_codec_deinit(struct fs1816_dev *fs1816)
{
	if (fs1816 == NULL || fs1816->dev == NULL)
		return;

	snd_soc_unregister_component(fs1816->dev);
}

static ssize_t dump_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct fs1816_dev *fs1816 = dev_get_drvdata(dev);
	uint16_t reg, val;
	ssize_t len;
	int ret;

	if (fs1816 == NULL)
		return -EINVAL;

	for (len = 0, reg = 0; reg <= 0xFF; reg++) {
		ret = fs1816_reg_read(fs1816, reg & 0xFF, &val);
		if (ret)
			return ret;
		len += snprintf(buf + len, PAGE_SIZE - len,
				"%02X:%04X%c", reg, val,
				(reg & 0x7) == 0x7 ? '\n' : ' ');
	}
	buf[len - 1] = '\n';

	return len;
}

static DEVICE_ATTR_RO(dump);

static struct attribute *fs1816_i2c_attributes[] = {
	&dev_attr_dump.attr,
	NULL
};

static struct attribute_group fs1816_i2c_attr_group = {
	.attrs = fs1816_i2c_attributes,
};

static int fs1816_parse_dts_pins(struct fs1816_dev *fs1816)
{
	struct gpio_desc *gpiod;

	gpiod = devm_gpiod_get(fs1816->dev, "intz", GPIOD_IN);
	if (!IS_ERR_OR_NULL(gpiod)) {
		fs1816->pdata.gpio_intz = gpiod;
		return 0;
	}

	if (PTR_ERR(gpiod) == -EBUSY)
		dev_info(fs1816->dev, "Assuming shared intz pin\n");
	else
		dev_info(fs1816->dev, "Assuming unused intz pin\n");

	return 0;
}

static int fs1816_parse_dts(struct fs1816_dev *fs1816)
{
	struct device_node *np = fs1816->dev->of_node;
	int ret;

	ret = fs1816_parse_dts_pins(fs1816);
	if (ret)
		return ret;

	ret = of_property_read_u32(np, "fs,rx_channel", &fs1816->rx_channel);
	if (ret)
		fs1816->rx_channel = FS1816_04H_CHS12_DEFAULT;

	ret = of_property_read_u32(np, "fs,rx_volume", &fs1816->rx_volume);
	if (ret)
		fs1816->rx_volume = FS1816_06H_VOL_DEFAULT;

	dev_info(fs1816->dev, "parse_dts: channel = %d, volume = 0x%X\n",
		fs1816->rx_channel, fs1816->rx_volume);

	return 0;
}

static int fs1816_dev_init(struct fs1816_dev *fs1816)
{
	int ret;

	ret = fs1816_regmap_init(fs1816);
	if (ret)
		return ret;

	fs1816_i2c_reset(fs1816);
	ret = fs1816_detect_device(fs1816);
	if (ret)
		return ret;

	ret = fs1816_codec_init(fs1816);
	if (ret)
		return ret;

	fs1816->hw_params.bclk = 0;
	fs1816->hw_params.daifmt = 0xFF; // Use reg default
	fs1816->hw_params.invfmt = 0xFF; // Use reg default

	ret = sysfs_create_group(&fs1816->dev->kobj, &fs1816_i2c_attr_group);
	if (ret)
		dev_err(fs1816->dev, "Failed to add sysfs:%d\n", ret);

	return 0;
}

static void fs1816_pins_deinit(struct fs1816_dev *fs1816)
{
	struct gpio_desc *gpiod;

	if (fs1816 == NULL || fs1816->dev == NULL)
		return;

	gpiod = fs1816->pdata.gpio_intz;
	if (!IS_ERR_OR_NULL(gpiod))
		devm_gpiod_put(fs1816->dev, gpiod);

	fs1816->pdata.gpio_intz = NULL;
}

static void fs1816_dev_deinit(struct fs1816_dev *fs1816)
{
	sysfs_remove_group(&fs1816->dev->kobj, &fs1816_i2c_attr_group);
	fs1816_codec_deinit(fs1816);
	fs1816_pins_deinit(fs1816);
}

#if KERNEL_VERSION_HIGHER(6, 3, 0)
static int fs1816_i2c_probe(struct i2c_client *i2c)
#else
static int fs1816_i2c_probe(struct i2c_client *i2c,
		const struct i2c_device_id *id)
#endif
{
	struct fs1816_dev *fs1816;
	int ret;

	dev_info(&i2c->dev, "Version: %s\n", FS1816_I2C_VERSION);

	if (!i2c_check_functionality(i2c->adapter, I2C_FUNC_I2C)) {
		dev_err(&i2c->dev, "Failed to check I2C_FUNC_I2C\n");
		return -EIO;
	}

	fs1816 = devm_kzalloc(&i2c->dev, sizeof(struct fs1816_dev), GFP_KERNEL);
	if (fs1816 == NULL)
		return -ENOMEM;

	fs1816->dev = &i2c->dev;
	fs1816->i2c = i2c;
	i2c_set_clientdata(i2c, fs1816);
	mutex_init(&fs1816->io_lock);

	ret = fs1816_parse_dts(fs1816);
	if (ret) {
		dev_err(&i2c->dev, "Failed to parse dts:%d\n", ret);
		return ret;
	}

	ret = fs1816_dev_init(fs1816);
	if (ret) {
		fs1816_pins_deinit(fs1816);
		dev_err(&i2c->dev, "Failed to init device:%d\n", ret);
		return ret;
	}

	dev_info(&i2c->dev, "i2c probed\n");

	return 0;
}

static i2c_remove_type fs1816_i2c_remove(struct i2c_client *i2c)
{
	struct fs1816_dev *fs1816 = i2c_get_clientdata(i2c);

	fs1816_dev_deinit(fs1816);
	dev_info(&i2c->dev, "i2c removed\n");

	return i2c_remove_val;
}

static void fs1816_i2c_shutdown(struct i2c_client *i2c)
{
	dev_info(&i2c->dev, "i2c shutdown\n");
}

static const struct i2c_device_id fs1816_i2c_id[] = {
	{ "fs1816", 0 },
	{}
};
MODULE_DEVICE_TABLE(i2c, fs1816_i2c_id);

#ifdef CONFIG_OF
static const struct of_device_id fs1816_of_id[] = {
	{ .compatible = "foursemi,fs1816", },
	{}
};
MODULE_DEVICE_TABLE(of, fs1816_of_id);
#endif

static struct i2c_driver fs1816_i2c_driver = {
	.driver = {
		.name = "fs1816",
#ifdef CONFIG_OF
		.of_match_table = fs1816_of_id,
#endif
		.owner = THIS_MODULE,
	},
	.probe = fs1816_i2c_probe,
	.remove = fs1816_i2c_remove,
	.shutdown = fs1816_i2c_shutdown,
	.id_table = fs1816_i2c_id,
};

module_i2c_driver(fs1816_i2c_driver);

MODULE_AUTHOR("FourSemi SW <support@foursemi.com>");
MODULE_DESCRIPTION("ASoC FourSemi Audio Amplifier Driver");
MODULE_VERSION(FS1816_I2C_VERSION);
MODULE_LICENSE("GPL");
