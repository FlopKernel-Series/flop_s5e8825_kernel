/*
 *  bigdata_aw882xx_sysfs_cb.c
 *  Copyright (c) Samsung Electronics
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 */

#include <sound/soc.h>
#include <sound/samsung/sec_audio_sysfs.h>
#include "bigdata_aw882xx_sysfs_cb.h"


extern int aw_big_data_callback(unsigned int type, unsigned int ch);
extern int aw_dsp_write_surface_temperature(unsigned char dev_index, int temperature);
extern int aw_dsp_read_cur_te(unsigned int dev_index);


static struct snd_soc_component *aw882xx_component;

/*
 * Index 0: maximum excursion (in um); e.g.200 for 0.2 mm
 * Index 1: maximum temperature (in degC)
 * Index 2: over excursion cnt
 * Index 3: over temperature cnt
 * Index 4: maximum temperature keep
 * Index 5: current temperature
 */

enum {
	AW_MAX_EXCU = 0,
	AW_MAX_TEMP,
	AW_MAX_EXCU_CNT,
	AW_MAX_TEMP_CNT,
	AW_MAX_TEMP_KEEP,
	AW_IIC_ABNORMAL_CNT,
	AW_IIS_ABNORMAL_CNT,
	AW_SHORT_CIRCUIT_CNT,
	AW_BIGDATA_MAX,
};

static int get_aw882xx_amp_temperature_max(enum amp_id id)
{
	struct snd_soc_component *component = aw882xx_component;
	int value = 0;

	if (!component) {
		pr_err("%s: component NULL\n", __func__);
		return -EPERM;
	}

	if (id >= AMP_ID_MAX) {
		dev_err(component->dev, "%s: invalid id(%d)\n", __func__, id);
		return -EINVAL;
	}

	value = aw_big_data_callback(AW_MAX_TEMP, id);
	if (value < 0) {
		dev_err(component->dev, "%s: invalid value(%d)\n", __func__, value);
		return -EINVAL;
	}

	dev_info(component->dev, "%s: id %d value %d\n", __func__, id, value);

	return value;
}

static int get_aw882xx_amp_temperature_keep_max(enum amp_id id)
{
	struct snd_soc_component *component = aw882xx_component;
	int value = 0;

	if (!component) {
		pr_err("%s: component NULL\n", __func__);
		return -EPERM;
	}

	if (id >= AMP_ID_MAX) {
		dev_err(component->dev, "%s: invalid id(%d)\n", __func__, id);
		return -EINVAL;
	}

	value = aw_big_data_callback(AW_MAX_TEMP_KEEP, id);
	if (value < 0) {
		dev_err(component->dev, "%s: invalid value(%d)\n", __func__, value);
		return -EINVAL;
	}

	dev_info(component->dev, "%s: id %d value %d\n", __func__, id, value);

	return value;
}

static int get_aw882xx_amp_temperature_overcount(enum amp_id id)
{
	struct snd_soc_component *component = aw882xx_component;
	int value = 0;

	if (!component) {
		pr_err("%s: component NULL\n", __func__);
		return -EPERM;
	}

	if (id >= AMP_ID_MAX) {
		dev_err(component->dev, "%s: invalid id(%d)\n", __func__, id);
		return -EINVAL;
	}

	value = aw_big_data_callback(AW_MAX_TEMP_CNT, id);
	if (value < 0) {
		dev_err(component->dev, "%s: invalid value(%d)\n", __func__, value);
		return -EINVAL;
	}

	dev_info(component->dev, "%s: id %d value %d\n", __func__, id, value);

	return value;
}

static int get_aw882xx_amp_excursion_max(enum amp_id id)
{
	struct snd_soc_component *component = aw882xx_component;
	int value = 0;

	if (!component) {
		pr_err("%s: component NULL\n", __func__);
		return -EPERM;
	}

	if (id >= AMP_ID_MAX) {
		dev_err(component->dev, "%s: invalid id(%d)\n", __func__, id);
		return -EINVAL;
	}

	value = aw_big_data_callback(AW_MAX_EXCU, id);
	if (value < 0) {
		dev_err(component->dev, "%s: invalid value(%d)\n", __func__, value);
		return -EINVAL;
	}

	dev_info(component->dev, "%s: id %d value %d\n", __func__, id, value);

	return value;
}

static int get_aw882xx_amp_excursion_overcount(enum amp_id id)
{
	struct snd_soc_component *component = aw882xx_component;
	int value = 0;

	if (!component) {
		pr_err("%s: component NULL\n", __func__);
		return -EPERM;
	}

	if (id >= AMP_ID_MAX) {
		dev_err(component->dev, "%s: invalid id(%d)\n", __func__, id);
		return -EINVAL;
	}

	value = aw_big_data_callback(AW_MAX_EXCU_CNT, id);
	if (value < 0) {
		dev_err(component->dev, "%s: invalid value(%d)\n", __func__, value);
		return -EINVAL;
	}

	dev_info(component->dev, "%s: id %d value %d\n", __func__, id, value);

	return value;
}

static int get_aw882xx_amp_curr_temperature(enum amp_id id)
{
	struct snd_soc_component *component = aw882xx_component;
	int value = 0;

	if (!component) {
		pr_err("%s: component NULL\n", __func__);
		return -EPERM;
	}

	if (id >= AMP_ID_MAX) {
		dev_err(component->dev, "%s: invalid id(%d)\n", __func__, id);
		return -EINVAL;
	}

	value = aw_dsp_read_cur_te(id);
	if (value < 0) {
		dev_err(component->dev, "%s: invalid value(%d)\n", __func__, value);
		return -EINVAL;
	}

	dev_info(component->dev, "%s: id %d value %d\n", __func__, id, value);

	return value;
}

static int set_aw882xx_amp_surface_temperature(enum amp_id id, int temperature)
{
	struct snd_soc_component *component = aw882xx_component;
	int ret = 0;

	if (!component) {
		pr_err("%s: component NULL\n", __func__);
		return -EPERM;
	}

	dev_info(component->dev, "%s: id %d temperature %d\n", __func__, id, temperature);

	if (id >= AMP_ID_MAX) {
		dev_err(component->dev, "%s: invalid id(%d)\n", __func__, id);
		return -EINVAL;
	}

	ret = aw_dsp_write_surface_temperature(id, temperature);
	if (ret) {
		dev_err(component->dev, "%s: invalid ret(%d)\n", __func__, ret);
		return -EINVAL;
	}

	return ret;
}

#if 0
static int get_aw882xx_iic_abnormal_cnt(enum amp_id id)
{
	struct snd_soc_component *component = aw882xx_component;
	int value = 0;

	if (!component) {
		pr_err("%s: component NULL\n", __func__);
		return -EPERM;
	}

	if (id >= AMP_ID_MAX) {
		dev_err(component->dev, "%s: invalid id(%d)\n", __func__, id);
		return -EINVAL;
	}

	value = aw_big_data_callback(AW_IIC_ABNORMAL_CNT, id);
	if (value < 0) {
		dev_err(component->dev, "%s: invalid value(%d)\n", __func__, value);
		return -EINVAL;
	}

	dev_info(component->dev, "%s: id %d value %d\n", __func__, id, value);

	return value;
}

static int get_aw882xx_iis_abnormal_cnt(enum amp_id id)
{
	struct snd_soc_component *component = aw882xx_component;
	int value = 0;

	if (!component) {
		pr_err("%s: component NULL\n", __func__);
		return -EPERM;
	}

	if (id >= AMP_ID_MAX) {
		dev_err(component->dev, "%s: invalid id(%d)\n", __func__, id);
		return -EINVAL;
	}

	value = aw_big_data_callback(AW_IIS_ABNORMAL_CNT, id);
	if (value < 0) {
		dev_err(component->dev, "%s: invalid value(%d)\n", __func__, value);
		return -EINVAL;
	}

	dev_info(component->dev, "%s: id %d value %d\n", __func__, id, value);

	return value;
}

static int get_aw882xx_short_circuit_cnt(enum amp_id id)
{
	struct snd_soc_component *component = aw882xx_component;
	int value = 0;

	if (!component) {
		pr_err("%s: component NULL\n", __func__);
		return -EPERM;
	}

	if (id >= AMP_ID_MAX) {
		dev_err(component->dev, "%s: invalid id(%d)\n", __func__, id);
		return -EINVAL;
	}

	value = aw_big_data_callback(AW_SHORT_CIRCUIT_CNT, id);
	if (value < 0) {
		dev_err(component->dev, "%s: invalid value(%d)\n", __func__, value);
		return -EINVAL;
	}

	dev_info(component->dev, "%s: id %d value %d\n", __func__, id, value);

	return value;
}
#endif

void register_aw882xx_bigdata_cb(struct snd_soc_component *component)
{
	aw882xx_component = component;

	dev_info(component->dev, "%s\n", __func__);

	audio_register_temperature_max_cb(get_aw882xx_amp_temperature_max);
	audio_register_temperature_keep_max_cb(get_aw882xx_amp_temperature_keep_max);
	audio_register_temperature_overcount_cb(get_aw882xx_amp_temperature_overcount);
	audio_register_excursion_max_cb(get_aw882xx_amp_excursion_max);
	audio_register_excursion_overcount_cb(get_aw882xx_amp_excursion_overcount);
	audio_register_curr_temperature_cb(get_aw882xx_amp_curr_temperature);
	audio_register_surface_temperature_cb(set_aw882xx_amp_surface_temperature);
#if 0
	audio_register_iic_abnormal_cnt_cb(get_aw882xx_iic_abnormal_cnt);
	audio_register_iis_abnormal_cnt_cb(get_aw882xx_iis_abnormal_cnt);
	audio_register_short_circuit_cnt_cb(get_aw882xx_short_circuit_cnt);
#endif
}
EXPORT_SYMBOL_GPL(register_aw882xx_bigdata_cb);
