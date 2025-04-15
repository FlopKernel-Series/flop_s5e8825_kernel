/*
 *  sec_audio_sysfs.c
 *
 *  Copyright (c) 2017 Samsung Electronics
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 */

#include <linux/err.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/time.h>
#include <linux/uaccess.h>
#include <linux/wait.h>

#include <sound/samsung/sec_audio_sysfs.h>

#define EARJACK_DEV_ID 0
#define CODEC_DEV_ID 1
#define AMP_DEV_ID 2
#define ADSP_DEV_ID 3

#define I2C_FAIL_MAX 0xFF
#define I2C_FAIL_KEEP_MAX 10000

#define ADSP_SRCNT_MAX 1000
#define ADSP_SRCNT_SUM_MAX 10000

/* bigdata add */
#define DECLARE_AMP_BIGDATA_SYSFS(id) \
static ssize_t audio_amp_##id##_temperature_max_show(struct device *dev, \
	struct device_attribute *attr, char *buf) \
{ \
	int report = 0; \
	if (audio_data->get_amp_temperature_max) \
		report = audio_data->get_amp_temperature_max((id)); \
	else \
		dev_info(dev, "%s: No callback registered\n", __func__); \
	return snprintf(buf, PAGE_SIZE, "%d\n", report); \
} \
static DEVICE_ATTR(temperature_max_##id, 0664, \
			audio_amp_##id##_temperature_max_show, NULL); \
static ssize_t audio_amp_##id##_temperature_keep_max_show(struct device *dev, \
	struct device_attribute *attr, char *buf) \
{ \
	int report = 0; \
	if (audio_data->get_amp_temperature_keep_max) \
		report = audio_data->get_amp_temperature_keep_max((id)); \
	else \
		dev_info(dev, "%s: No callback registered\n", __func__); \
	return snprintf(buf, PAGE_SIZE, "%d\n", report); \
} \
static DEVICE_ATTR(temperature_keep_max_##id, 0664, \
			audio_amp_##id##_temperature_keep_max_show, NULL); \
static ssize_t audio_amp_##id##_temperature_overcount_show(struct device *dev, \
	struct device_attribute *attr, char *buf) \
{ \
	int report = 0; \
	if (audio_data->get_amp_temperature_overcount) \
		report = audio_data->get_amp_temperature_overcount((id)); \
	else \
		dev_info(dev, "%s: No callback registered\n", __func__); \
	return snprintf(buf, PAGE_SIZE, "%d\n", report); \
} \
static DEVICE_ATTR(temperature_overcount_##id, 0664, \
			audio_amp_##id##_temperature_overcount_show, NULL); \
static ssize_t audio_amp_##id##_excursion_max_show(struct device *dev, \
	struct device_attribute *attr, char *buf) \
{ \
	int report = 0; \
	if (audio_data->get_amp_excursion_max) \
		report = audio_data->get_amp_excursion_max((id)); \
	else \
		dev_info(dev, "%s: No callback registered\n", __func__); \
	return snprintf(buf, PAGE_SIZE, "%04d\n", report); \
} \
static DEVICE_ATTR(excursion_max_##id, 0664, \
			audio_amp_##id##_excursion_max_show, NULL); \
static ssize_t audio_amp_##id##_excursion_overcount_show(struct device *dev, \
	struct device_attribute *attr, char *buf) \
{ \
	int report = 0; \
	if (audio_data->get_amp_excursion_overcount) \
		report = audio_data->get_amp_excursion_overcount(id); \
	else \
		dev_info(dev, "%s: No callback registered\n", __func__); \
	return snprintf(buf, PAGE_SIZE, "%d\n", report); \
} \
static DEVICE_ATTR(excursion_overcount_##id, 0664, \
			audio_amp_##id##_excursion_overcount_show, NULL); \
static ssize_t audio_amp_##id##_curr_temperature_show(struct device *dev, \
	struct device_attribute *attr, char *buf) \
{ \
	int report = 0; \
	if (audio_data->get_amp_curr_temperature) \
		report = audio_data->get_amp_curr_temperature((id)); \
	else \
		dev_info(dev, "%s: No callback registered\n", __func__); \
	return snprintf(buf, PAGE_SIZE, "%d\n", report); \
} \
static DEVICE_ATTR(curr_temperature_##id, 0664, \
			audio_amp_##id##_curr_temperature_show, NULL); \
static ssize_t audio_amp_##id##_surface_temperature_store(struct device *dev, \
	struct device_attribute *attr, const char *buf, size_t size) \
{ \
	int ret, temp = 0; \
	ret = kstrtos32(buf, 10, &temp); \
	if (audio_data->set_amp_surface_temperature) \
		ret = audio_data->set_amp_surface_temperature((id), temp); \
	else \
		dev_info(dev, "%s: No callback registered\n", __func__); \
	return size; \
} \
static DEVICE_ATTR(surface_temperature_##id, 0664, \
			NULL, audio_amp_##id##_surface_temperature_store); \
static ssize_t audio_amp_##id##_aifecnt_show(struct device *dev, \
	struct device_attribute *attr, char *buf) \
{ \
	int report = audio_count->aifecnt[id].count; \
	audio_count->aifecnt[id].count = 0; \
	return snprintf(buf, PAGE_SIZE, "%d\n", report); \
} \
static DEVICE_ATTR(aifecnt_##id, 0664, \
			audio_amp_##id##_aifecnt_show, NULL); \
static ssize_t audio_amp_##id##_aifecnt_keep_show(struct device *dev, \
	struct device_attribute *attr, char *buf) \
{ \
	return snprintf(buf, PAGE_SIZE, "%d\n", \
		audio_count->aifecnt[id].count_sum); \
} \
static DEVICE_ATTR(aifecnt_keep_##id, 0664, \
			audio_amp_##id##_aifecnt_keep_show, NULL); \
static ssize_t audio_amp_##id##_ready_show(struct device *dev, \
	struct device_attribute *attr, char *buf) \
{ \
	int report = 0; \
	if (audio_data->get_amp_ready) \
		audio_ready->amp[id] = audio_data->get_amp_ready((id)); \
	report = audio_ready->amp[id]; \
	return snprintf(buf, PAGE_SIZE, "%d\n", report); \
} \
static DEVICE_ATTR(ready_##id, 0664, \
			audio_amp_##id##_ready_show, NULL); \
static struct attribute *audio_amp_##id##_attr[] = { \
	&dev_attr_temperature_max_##id.attr, \
	&dev_attr_temperature_keep_max_##id.attr, \
	&dev_attr_temperature_overcount_##id.attr, \
	&dev_attr_excursion_max_##id.attr, \
	&dev_attr_excursion_overcount_##id.attr, \
	&dev_attr_curr_temperature_##id.attr, \
	&dev_attr_surface_temperature_##id.attr, \
	&dev_attr_aifecnt_##id.attr, \
	&dev_attr_aifecnt_keep_##id.attr, \
	&dev_attr_ready_##id.attr, \
	NULL, \
}

static struct sec_audio_sysfs_data *audio_data;

static struct sec_audio_count_data *audio_count;

static struct sec_audio_ready_data *audio_ready;

int audio_register_jack_select_cb(int (*set_jack) (int))
{
	if (audio_data->set_jack_state) {
		dev_err(audio_data->jack_dev,
				"%s: Already registered\n", __func__);
		return -EEXIST;
	}

	audio_data->set_jack_state = set_jack;

	return 0;
}
EXPORT_SYMBOL_GPL(audio_register_jack_select_cb);

static ssize_t audio_jack_select_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	if (audio_data->set_jack_state) {
		if ((!size) || (buf[0] != '1')) {
			dev_info(dev, "%s: Forced remove jack\n", __func__);
			audio_data->set_jack_state(0);
		} else {
			dev_info(dev, "%s: Forced detect jack\n", __func__);
			audio_data->set_jack_state(1);
		}
	} else {
		dev_info(dev, "%s: No callback registered\n", __func__);
	}

	return size;
}

static DEVICE_ATTR(select_jack, 0664,
			NULL, audio_jack_select_store);

int audio_register_jack_state_cb(int (*jack_state) (void))
{
	if (audio_data->get_jack_state) {
		dev_err(audio_data->jack_dev,
				"%s: Already registered\n", __func__);
		return -EEXIST;
	}

	audio_data->get_jack_state = jack_state;

	return 0;
}
EXPORT_SYMBOL_GPL(audio_register_jack_state_cb);

static ssize_t audio_jack_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int report = 0;

	if (audio_data->get_jack_state)
		report = audio_data->get_jack_state();
	else
		dev_info(dev, "%s: No callback registered\n", __func__);

	return snprintf(buf, 4, "%d\n", report);
}

static DEVICE_ATTR(state, 0664,
			audio_jack_state_show, NULL);

int audio_register_key_state_cb(int (*key_state) (void))
{
	if (audio_data->get_key_state) {
		dev_err(audio_data->jack_dev,
				"%s: Already registered\n", __func__);
		return -EEXIST;
	}

	audio_data->get_key_state = key_state;

	return 0;
}
EXPORT_SYMBOL_GPL(audio_register_key_state_cb);

static ssize_t audio_key_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int report = 0;

	if (audio_data->get_key_state)
		report = audio_data->get_key_state();
	else
		dev_info(dev, "%s: No callback registered\n", __func__);

	return snprintf(buf, 4, "%d\n", report);
}

static DEVICE_ATTR(key_state, 0664,
			audio_key_state_show, NULL);

int audio_register_mic_adc_cb(int (*mic_adc) (void))
{
	if (audio_data->get_mic_adc) {
		dev_err(audio_data->jack_dev,
				"%s: Already registered\n", __func__);
		return -EEXIST;
	}

	audio_data->get_mic_adc = mic_adc;

	return 0;
}
EXPORT_SYMBOL_GPL(audio_register_mic_adc_cb);

static ssize_t audio_mic_adc_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int report = 0;

	if (audio_data->get_mic_adc)
		report = audio_data->get_mic_adc();
	else
		dev_info(dev, "%s: No callback registered\n", __func__);

	return snprintf(buf, 16, "%d\n", report);
}

static DEVICE_ATTR(mic_adc, 0664,
			audio_mic_adc_show, NULL);

int audio_register_force_enable_antenna_cb(int (*force_enable_antenna) (int))
{
	if (audio_data->set_force_enable_antenna) {
		dev_err(audio_data->jack_dev,
				"%s: Already registered\n", __func__);
		return -EEXIST;
	}

	audio_data->set_force_enable_antenna = force_enable_antenna;

	return 0;
}
EXPORT_SYMBOL_GPL(audio_register_force_enable_antenna_cb);

static ssize_t force_enable_antenna_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t size)
{
	if (audio_data->set_force_enable_antenna) {
		if ((!size) || (buf[0] != '1')) {
			dev_info(dev, "%s: antenna disable\n", __func__);
			audio_data->set_force_enable_antenna(0);
		} else {
			dev_info(dev, "%s: update antenna enable\n", __func__);
			audio_data->set_force_enable_antenna(1);
		}
	} else {
		dev_info(dev, "%s: No callback registered\n", __func__);
	}

	return size;
}

static DEVICE_ATTR(force_enable_antenna, 0664,
			NULL, force_enable_antenna_store);

int audio_register_antenna_state_cb(int (*antenna_state) (void))
{
	if (audio_data->get_antenna_state) {
		dev_err(audio_data->jack_dev,
				"%s: Already registered\n", __func__);
		return -EEXIST;
	}

	audio_data->get_antenna_state = antenna_state;

	return 0;
}
EXPORT_SYMBOL_GPL(audio_register_antenna_state_cb);

static ssize_t audio_antenna_state_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int report = 0;

	if (audio_data->get_antenna_state)
		report = audio_data->get_antenna_state();
	else
		dev_info(dev, "%s: No callback registered\n", __func__);

	return snprintf(buf, 4, "%d\n", report);
}

static DEVICE_ATTR(antenna_state, 0664,
			audio_antenna_state_show, NULL);

static struct attribute *sec_audio_jack_attr[] = {
	&dev_attr_select_jack.attr,
	&dev_attr_state.attr,
	&dev_attr_key_state.attr,
	&dev_attr_mic_adc.attr,
	&dev_attr_force_enable_antenna.attr,
	&dev_attr_antenna_state.attr,
	NULL,
};

static struct attribute_group sec_audio_jack_attr_group = {
	.attrs = sec_audio_jack_attr,
};

int audio_register_codec_id_state_cb(int (*codec_id_state) (void))
{
	if (audio_data->get_codec_id_state) {
		dev_err(audio_data->codec_dev,
				"%s: Already registered\n", __func__);
		return -EEXIST;
	}

	audio_data->get_codec_id_state = codec_id_state;

	return 0;
}
EXPORT_SYMBOL_GPL(audio_register_codec_id_state_cb);

static ssize_t audio_check_codec_id_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int report = 0;

	if (audio_data->get_codec_id_state)
		report = audio_data->get_codec_id_state();
	else
		dev_info(dev, "%s: No callback registered\n", __func__);

	return snprintf(buf, 4, "%d\n", report);
}

static DEVICE_ATTR(check_codec_id, 0664,
			audio_check_codec_id_show, NULL);

static ssize_t audio_cifecnt_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int report = audio_count->cifecnt.count;

	audio_count->cifecnt.count = 0;
	return snprintf(buf, 8, "%d\n", report);
}

static DEVICE_ATTR(cifecnt, 0664,
			audio_cifecnt_show, NULL);

static ssize_t audio_cifecnt_keep_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return snprintf(buf, 8, "%d\n", audio_count->cifecnt.count_sum);
}

static DEVICE_ATTR(cifecnt_keep, 0664,
			audio_cifecnt_keep_show, NULL);

void send_codec_i2c_fail_ev(void)
{
	if (audio_count->cifecnt.count < I2C_FAIL_MAX)
		audio_count->cifecnt.count++;
	if (audio_count->cifecnt.count_sum < I2C_FAIL_KEEP_MAX)
		audio_count->cifecnt.count_sum++;

	pr_info("%s: count %d, keep %d\n", __func__,
		audio_count->cifecnt.count,
		audio_count->cifecnt.count_sum);
}
EXPORT_SYMBOL_GPL(send_codec_i2c_fail_ev);

static struct attribute *sec_audio_codec_attr[] = {
	&dev_attr_check_codec_id.attr,
	&dev_attr_cifecnt.attr,
	&dev_attr_cifecnt_keep.attr,
	NULL,
};

static struct attribute_group sec_audio_codec_attr_group = {
	.attrs = sec_audio_codec_attr,
};

void send_amp_ready_ev(enum amp_id id, enum ready_state state)
{
	if (id < AMP_ID_MAX)
		audio_ready->amp[id] = state;

	pr_info("%s: amp id %d, state %d\n", __func__, id, state);
}
EXPORT_SYMBOL_GPL(send_amp_ready_ev);

enum ready_state get_amp_ready_state(enum amp_id id)
{
	if (id < AMP_ID_MAX)
		return audio_ready->amp[id];
	return NOT_SUPPORT;
}
EXPORT_SYMBOL_GPL(get_amp_ready_state);

/* bigdata */
int audio_register_temperature_max_cb(int (*temperature_max) (enum amp_id))
{
	if (audio_data->get_amp_temperature_max) {
		dev_err(audio_data->amp_dev,
				"%s: Already registered\n", __func__);
		return -EEXIST;
	}

	audio_data->get_amp_temperature_max = temperature_max;

	return 0;
}
EXPORT_SYMBOL_GPL(audio_register_temperature_max_cb);

int audio_register_temperature_keep_max_cb(int (*temperature_keep_max) (enum amp_id))
{
	if (audio_data->get_amp_temperature_keep_max) {
		dev_err(audio_data->amp_dev,
				"%s: Already registered\n", __func__);
		return -EEXIST;
	}

	audio_data->get_amp_temperature_keep_max = temperature_keep_max;

	return 0;
}
EXPORT_SYMBOL_GPL(audio_register_temperature_keep_max_cb);

int audio_register_temperature_overcount_cb(int (*temperature_overcount) (enum amp_id))
{
	if (audio_data->get_amp_temperature_overcount) {
		dev_err(audio_data->amp_dev,
				"%s: Already registered\n", __func__);
		return -EEXIST;
	}

	audio_data->get_amp_temperature_overcount = temperature_overcount;

	return 0;
}
EXPORT_SYMBOL_GPL(audio_register_temperature_overcount_cb);

int audio_register_excursion_max_cb(int (*excursion_max) (enum amp_id))
{
	if (audio_data->get_amp_excursion_max) {
		dev_err(audio_data->amp_dev,
				"%s: Already registered\n", __func__);
		return -EEXIST;
	}

	audio_data->get_amp_excursion_max = excursion_max;

	return 0;
}
EXPORT_SYMBOL_GPL(audio_register_excursion_max_cb);

int audio_register_excursion_overcount_cb(int (*excursion_overcount) (enum amp_id))
{
	if (audio_data->get_amp_excursion_overcount) {
		dev_err(audio_data->amp_dev,
				"%s: Already registered\n", __func__);
		return -EEXIST;
	}

	audio_data->get_amp_excursion_overcount = excursion_overcount;

	return 0;
}
EXPORT_SYMBOL_GPL(audio_register_excursion_overcount_cb);

int audio_register_curr_temperature_cb(int (*curr_temperature) (enum amp_id))
{
	if (audio_data->get_amp_curr_temperature) {
		dev_err(audio_data->amp_dev,
				"%s: Already registered\n", __func__);
		return -EEXIST;
	}

	audio_data->get_amp_curr_temperature = curr_temperature;

	return 0;
}
EXPORT_SYMBOL_GPL(audio_register_curr_temperature_cb);

int audio_register_surface_temperature_cb(int (*surface_temperature) (enum amp_id, int temperature))
{
	if (audio_data->set_amp_surface_temperature) {
		dev_err(audio_data->amp_dev,
				"%s: Already registered\n", __func__);
		return -EEXIST;
	}

	audio_data->set_amp_surface_temperature = surface_temperature;

	return 0;
}
EXPORT_SYMBOL_GPL(audio_register_surface_temperature_cb);

int audio_register_ready_cb(int (*ready) (enum amp_id))
{
	if (audio_data->get_amp_ready) {
		dev_err(audio_data->amp_dev,
				"%s: Already registered\n", __func__);
		return -EEXIST;
	}

	audio_data->get_amp_ready = ready;

	return 0;
}
EXPORT_SYMBOL_GPL(audio_register_ready_cb);

DECLARE_AMP_BIGDATA_SYSFS(0);
DECLARE_AMP_BIGDATA_SYSFS(1);
DECLARE_AMP_BIGDATA_SYSFS(2);
DECLARE_AMP_BIGDATA_SYSFS(3);

static struct attribute_group sec_audio_amp_big_data_attr_group[AMP_ID_MAX] = {
	[AMP_0] = {.attrs = audio_amp_0_attr, },
	[AMP_1] = {.attrs = audio_amp_1_attr, },
	[AMP_2] = {.attrs = audio_amp_2_attr, },
	[AMP_3] = {.attrs = audio_amp_3_attr, },
};

void send_amp_i2c_fail_ev(int amp_id)
{
	if (amp_id < 0 || amp_id >= audio_data->num_amp)
		return;
	if (audio_count->aifecnt[amp_id].count < I2C_FAIL_MAX)
		audio_count->aifecnt[amp_id].count++;
	if (audio_count->aifecnt[amp_id].count_sum < I2C_FAIL_KEEP_MAX)
		audio_count->aifecnt[amp_id].count_sum++;

	pr_info("%s: count %d, keep %d\n", __func__,
		audio_count->aifecnt[amp_id].count,
		audio_count->aifecnt[amp_id].count_sum);
}
EXPORT_SYMBOL_GPL(send_amp_i2c_fail_ev);

static ssize_t aifecnt_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int i, shift;
	unsigned int report = 0;

	for (i = 0; i < audio_data->num_amp; i++) {
		shift = i << 3;
		report |= (audio_count->aifecnt[i].count << shift);
		audio_count->aifecnt[i].count = 0;
	}
	dev_info(dev, "%s: %08x\n", __func__, report);

	return snprintf(buf, 8, "%u\n", report);
}

static DEVICE_ATTR_RO(aifecnt);

static ssize_t aifecnt_keep_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int i, report = 0;

	for (i = 0; i < audio_data->num_amp; i++)
		report += audio_count->aifecnt[i].count_sum;
	dev_info(dev, "%s: %d\n", __func__, report);

	return snprintf(buf, 8, "%d\n", report);
}

static DEVICE_ATTR_RO(aifecnt_keep);

static struct attribute *sec_audio_aifecnt_attr[] = {
	&dev_attr_aifecnt.attr,
	&dev_attr_aifecnt_keep.attr,
	NULL,
};

static struct attribute_group sec_audio_aifecnt_attr_group = {
	.attrs = sec_audio_aifecnt_attr,
};

void send_adsp_silent_reset_ev(void)
{
	if (audio_count->adsp_sr.count < ADSP_SRCNT_MAX)
		audio_count->adsp_sr.count++;
	if (audio_count->adsp_sr.count_sum < ADSP_SRCNT_SUM_MAX)
		audio_count->adsp_sr.count_sum++;
	pr_info("%s: count %d\n", __func__, audio_count->adsp_sr.count_sum);
}
EXPORT_SYMBOL_GPL(send_adsp_silent_reset_ev);

static ssize_t srcnt_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int report = audio_count->adsp_sr.count;

	audio_count->adsp_sr.count = 0;

	dev_info(dev, "%s: %d\n", __func__, report);

	return snprintf(buf, 8, "%d\n", report);
}

static DEVICE_ATTR_RO(srcnt);

static ssize_t srcnt_keep_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	int report = audio_count->adsp_sr.count_sum;

	dev_info(dev, "%s: %d\n", __func__, report);

	return snprintf(buf, 8, "%d\n", report);
}

static DEVICE_ATTR_RO(srcnt_keep);

static struct attribute *sec_audio_adsp_attr[] = {
	&dev_attr_srcnt.attr,
	&dev_attr_srcnt_keep.attr,
	NULL,
};

static struct attribute_group sec_audio_adsp_attr_group = {
	.attrs = sec_audio_adsp_attr,
};

static int sec_audio_sysfs_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	int i;

	if (audio_data == NULL) {
		dev_err(&pdev->dev, "%s: no audio_data\n", __func__);
		return -ENOMEM;
	}

	if (audio_count == NULL) {
		dev_err(&pdev->dev, "%s: no audio_count\n", __func__);
		return -ENOMEM;
	}

	if (audio_ready == NULL) {
		dev_err(&pdev->dev, "%s: no audio_ready\n", __func__);
		return -ENOMEM;
	}

	audio_data->no_earjack = of_property_read_bool(np, "audio,no-earjack");

	if (audio_data->no_earjack) {
		dev_info(&pdev->dev, "%s: remove earjack sysfs dev\n", __func__);
		if (audio_data->jack_dev) {
			sysfs_remove_group(&audio_data->jack_dev->kobj,
					&sec_audio_jack_attr_group);
			device_destroy(audio_data->audio_class, EARJACK_DEV_ID);
		}
	}

	of_property_read_u32(np, "audio,num-amp", &audio_data->num_amp);
	for (i = audio_data->num_amp; i < AMP_ID_MAX; i++) {
		sysfs_remove_group(&audio_data->amp_dev->kobj,
			&sec_audio_amp_big_data_attr_group[i]);
	}

	return 0;
}

static int sec_audio_sysfs_remove(struct platform_device *pdev)
{
	int i;

	if (audio_data->num_amp == 0)
		audio_data->num_amp = AMP_ID_MAX;

	for (i = 0; i < audio_data->num_amp; i++) {
		sysfs_remove_group(&audio_data->amp_dev->kobj,
			&sec_audio_amp_big_data_attr_group[i]);
	}

	return 0;
}

#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id sec_audio_sysfs_of_match[] = {
	{ .compatible = "samsung,audio-sysfs", },
	{},
};
MODULE_DEVICE_TABLE(of, sec_audio_sysfs_of_match);
#endif /* CONFIG_OF */

static struct platform_driver sec_audio_sysfs_driver = {
	.driver		= {
		.name	= "sec-audio-sysfs",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(sec_audio_sysfs_of_match),
	},

	.probe		= sec_audio_sysfs_probe,
	.remove		= sec_audio_sysfs_remove,
};

static int __init sec_audio_sysfs_init(void)
{
	int ret = 0;
	int i = 0;

	audio_data = kzalloc(sizeof(struct sec_audio_sysfs_data), GFP_KERNEL);
	if (audio_data == NULL)
		return -ENOMEM;

	audio_count = kzalloc(sizeof(struct sec_audio_count_data), GFP_KERNEL);
	if (audio_count == NULL) {
		kfree(audio_data);
		return -ENOMEM;
	}

	audio_ready = kzalloc(sizeof(struct sec_audio_ready_data), GFP_KERNEL);
	if (audio_ready == NULL) {
		kfree(audio_data);
		kfree(audio_count);
		return -ENOMEM;
	}

	audio_data->audio_class = class_create(THIS_MODULE, "audio");
	if (IS_ERR(audio_data->audio_class)) {
		pr_err("%s: Failed to create audio class\n", __func__);
		ret = PTR_ERR(audio_data->audio_class);
		goto err_alloc;
	}

	audio_data->jack_dev =
			device_create(audio_data->audio_class,
					NULL, EARJACK_DEV_ID, NULL, "earjack");
	if (IS_ERR(audio_data->jack_dev)) {
		pr_err("%s: Failed to create earjack device\n", __func__);
		ret = PTR_ERR(audio_data->jack_dev);
		goto err_class;
	}

	ret = sysfs_create_group(&audio_data->jack_dev->kobj,
				&sec_audio_jack_attr_group);
	if (ret) {
		pr_err("%s: Failed to create earjack sysfs\n", __func__);
		goto err_jack_device;
	}

	audio_data->codec_dev =
			device_create(audio_data->audio_class,
					NULL, CODEC_DEV_ID, NULL, "codec");
	if (IS_ERR(audio_data->codec_dev)) {
		pr_err("%s: Failed to create codec device\n", __func__);
		ret = PTR_ERR(audio_data->codec_dev);
		goto err_jack_attr;
	}

	ret = sysfs_create_group(&audio_data->codec_dev->kobj,
				&sec_audio_codec_attr_group);
	if (ret) {
		pr_err("%s: Failed to create codec sysfs\n", __func__);
		goto err_codec_device;
	}

	audio_data->amp_dev =
			device_create(audio_data->audio_class,
					NULL, AMP_DEV_ID, NULL, "amp");
	if (IS_ERR(audio_data->amp_dev)) {
		pr_err("%s: Failed to create amp device\n", __func__);
		ret = PTR_ERR(audio_data->amp_dev);
		goto err_codec_attr;
	}

	audio_data->num_amp = 0;

	for (i = 0; i < AMP_ID_MAX; i++) {
		audio_ready->amp[i] = NOT_SUPPORT;
		ret = sysfs_create_group(&audio_data->amp_dev->kobj,
			&sec_audio_amp_big_data_attr_group[i]);
		if (ret) {
			pr_err("%s: Failed to create amp sysfs\n", __func__);
			goto err_amp_attr;
		}
	}

	ret = sysfs_create_group(&audio_data->amp_dev->kobj,
				&sec_audio_aifecnt_attr_group);
	if (ret) {
		pr_err("%s: Failed to create amp aifecnt sysfs\n", __func__);
		goto err_amp_attr;
	}

	audio_data->adsp_dev =
			device_create(audio_data->audio_class,
					NULL, ADSP_DEV_ID, NULL, "dsp");
	if (IS_ERR(audio_data->adsp_dev)) {
		pr_err("%s: Failed to create adsp device\n", __func__);
		ret = PTR_ERR(audio_data->adsp_dev);
		goto err_amp_attr;
	}

	ret = sysfs_create_group(&audio_data->adsp_dev->kobj,
				&sec_audio_adsp_attr_group);
	if (ret) {
		pr_err("%s: Failed to create adsp sysfs\n", __func__);
		goto err_adsp_device;
	}
	ret = platform_driver_register(&sec_audio_sysfs_driver);
	if (ret) {
		pr_err("%s : fail to register sysfs driver\n", __func__);
		goto err_adsp_attr;
	}

	return ret;

err_adsp_attr:
	sysfs_remove_group(&audio_data->adsp_dev->kobj,
				&sec_audio_adsp_attr_group);
err_adsp_device:
	device_destroy(audio_data->audio_class, ADSP_DEV_ID);
	audio_data->adsp_dev = NULL;
err_amp_attr:
	while (--i >= 0)
		sysfs_remove_group(&audio_data->amp_dev->kobj,
			&sec_audio_amp_big_data_attr_group[i]);
	device_destroy(audio_data->audio_class, AMP_DEV_ID);
	audio_data->amp_dev = NULL;
err_codec_attr:
	sysfs_remove_group(&audio_data->codec_dev->kobj,
				&sec_audio_codec_attr_group);
err_codec_device:
	device_destroy(audio_data->audio_class, CODEC_DEV_ID);
	audio_data->codec_dev = NULL;
err_jack_attr:
	sysfs_remove_group(&audio_data->jack_dev->kobj,
				&sec_audio_jack_attr_group);
err_jack_device:
	device_destroy(audio_data->audio_class, EARJACK_DEV_ID);
	audio_data->jack_dev = NULL;
err_class:
	class_destroy(audio_data->audio_class);
	audio_data->audio_class = NULL;
err_alloc:
	kfree(audio_data);
	kfree(audio_count);
	kfree(audio_ready);
	audio_data = NULL;
	audio_count = NULL;
	audio_ready = NULL;

	return ret;
}
module_init(sec_audio_sysfs_init);

static void __exit sec_audio_sysfs_exit(void)
{
	int i;

	platform_driver_unregister(&sec_audio_sysfs_driver);

	if (audio_data->adsp_dev) {
		sysfs_remove_group(&audio_data->adsp_dev->kobj,
				&sec_audio_adsp_attr_group);
		device_destroy(audio_data->audio_class, ADSP_DEV_ID);
	}

	if (audio_data->amp_dev) {
		for (i = 0; i < AMP_ID_MAX; i++) {
			sysfs_remove_group(&audio_data->amp_dev->kobj,
						&sec_audio_amp_big_data_attr_group[i]);
		}
		device_destroy(audio_data->audio_class, AMP_DEV_ID);
	}

	if (audio_data->codec_dev) {
		sysfs_remove_group(&audio_data->codec_dev->kobj,
				&sec_audio_codec_attr_group);
		device_destroy(audio_data->audio_class, CODEC_DEV_ID);
	}

	if (audio_data->jack_dev) {
		sysfs_remove_group(&audio_data->jack_dev->kobj,
				&sec_audio_jack_attr_group);
		device_destroy(audio_data->audio_class, EARJACK_DEV_ID);
	}

	if (audio_data->audio_class)
		class_destroy(audio_data->audio_class);

	kfree(audio_data);
	kfree(audio_count);
}
module_exit(sec_audio_sysfs_exit);

MODULE_DESCRIPTION("Samsung Electronics Audio SYSFS driver");
MODULE_LICENSE("GPL");
