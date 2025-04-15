// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (C) 2019-2021 Samsung Electronics Co. Ltd.
 */

#define pr_fmt(fmt) "[VIB] " fmt
#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/hrtimer.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/vibrator/sec_vibrator.h>
#include <linux/vibrator/sec_vibrator_notifier.h>
#include <linux/vibrator/sec_vibrator_kunit.h>
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
#if defined(CONFIG_BATTERY_GKI)
#include <linux/battery/sec_battery_common.h>
#else
#include <linux/battery/common/sb_psy.h>
#endif
#endif

static const int  kmax_buf_size = 7;
static const int kmax_haptic_step_size = 7;
static const char *str_newline = "\n";

static char vib_event_cmd[MAX_STR_LEN_EVENT_CMD];

static const char sec_vib_event_cmd[EVENT_CMD_MAX][MAX_STR_LEN_EVENT_CMD] = {
	[EVENT_CMD_NONE]					= "NONE",
	[EVENT_CMD_FOLDER_CLOSE]				= "FOLDER_CLOSE",
	[EVENT_CMD_FOLDER_OPEN]					= "FOLDER_OPEN",
	[EVENT_CMD_ACCESSIBILITY_BOOST_ON]			= "ACCESSIBILITY_BOOST_ON",
	[EVENT_CMD_ACCESSIBILITY_BOOST_OFF]			= "ACCESSIBILITY_BOOST_OFF",
	[EVENT_CMD_TENT_CLOSE]					= "FOLDER_TENT_CLOSE",
	[EVENT_CMD_TENT_OPEN]					= "FOLDER_TENT_OPEN",
};

static struct sec_vibrator_pdata *s_v_pdata;

static int sec_vibrator_pdata_init(void)
{
	if (s_v_pdata)
		goto skip;
	s_v_pdata = kzalloc(sizeof(struct sec_vibrator_pdata), GFP_KERNEL);
	if (!s_v_pdata)
		goto err;
	s_v_pdata->probe_done = false;
	s_v_pdata->normal_ratio = 100;
	s_v_pdata->high_temp_ratio = SEC_VIBRATOR_DEFAULT_HIGH_TEMP_RATIO;
	s_v_pdata->high_temp_ref = SEC_VIBRATOR_DEFAULT_HIGH_TEMP_REF;
skip:
	return 0;
err:
	return -ENOMEM;
}

#ifdef CONFIG_SEC_VIB_NOTIFIER
static struct vib_notifier_context vib_notifier;
static struct blocking_notifier_head sec_vib_nb_head
	= BLOCKING_NOTIFIER_INIT(sec_vib_nb_head);

int sec_vib_notifier_register(struct notifier_block *noti_block)
{
	int ret = 0;

	pr_info("%s\n", __func__);

	if (!noti_block)
		return -EINVAL;

	ret = blocking_notifier_chain_register(&sec_vib_nb_head, noti_block);
	if (ret < 0)
		pr_err("%s: failed(%d)\n", __func__, ret);

	return ret;
}
EXPORT_SYMBOL_GPL(sec_vib_notifier_register);

int sec_vib_notifier_unregister(struct notifier_block *noti_block)
{
	int ret = 0;

	pr_info("%s\n", __func__);

	if (!noti_block)
		return -EINVAL;

	ret = blocking_notifier_chain_unregister(&sec_vib_nb_head, noti_block);
	if (ret < 0)
		pr_err("%s: failed(%d)\n", __func__, ret);

	return ret;
}
EXPORT_SYMBOL_GPL(sec_vib_notifier_unregister);

int sec_vib_notifier_notify(int en, struct sec_vibrator_drvdata *ddata)
{
	int ret = 0;

	if (!ddata) {
		pr_err("%s : ddata is NULL\n", __func__);
		return -ENODATA;
	}

	vib_notifier.index = ddata->index;
	vib_notifier.timeout = ddata->timeout;

	pr_info("%s: %s, idx: %d timeout: %d\n", __func__, en ? "ON" : "OFF",
		vib_notifier.index, vib_notifier.timeout);

	ret = blocking_notifier_call_chain(&sec_vib_nb_head, en, &vib_notifier);

	switch (ret) {
	case NOTIFY_DONE:
	case NOTIFY_OK:
		pr_info("%s done(0x%x)\n", __func__, ret);
		break;
	default:
		pr_info("%s failed(0x%x)\n", __func__, ret);
		break;
	}

	return ret;
}
EXPORT_SYMBOL_GPL(sec_vib_notifier_notify);
#endif

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
static int sec_vibrator_check_temp(struct sec_vibrator_drvdata *ddata)
{
	int ret = 0;
	union power_supply_propval value = {0, };

	if (!ddata) {
		pr_err("%s : ddata is NULL\n", __func__);
		return -ENODATA;
	}

	if (!ddata->vib_ops->set_tuning_with_temp)
		return -ENOSYS;

	psy_do_property("battery", get, POWER_SUPPLY_PROP_TEMP, value);

	ret = ddata->vib_ops->set_tuning_with_temp(ddata->dev, value.intval);
	if (ret)
		pr_err("%s error(%d)\n", __func__, ret);

	return ret;
}
#endif

__visible_for_testing int sec_vibrator_set_enable(struct sec_vibrator_drvdata *ddata, bool en)
{
	int ret = 0;

	if (!ddata) {
		pr_err("%s : ddata is NULL\n", __func__);
		return -ENODATA;
	}

	if (!ddata->vib_ops->enable)
		return -ENOSYS;

	ret = ddata->vib_ops->enable(ddata->dev, en);
	if (ret)
		pr_err("%s error(%d)\n", __func__, ret);

	sec_vib_notifier_notify(en, ddata);

	return ret;
}

__visible_for_testing int sec_vibrator_set_intensity(struct sec_vibrator_drvdata *ddata, int intensity)
{
	int ret = 0;

	if (!ddata) {
		pr_err("%s : ddata is NULL\n", __func__);
		return -ENODATA;
	}

	if (!ddata->vib_ops->set_intensity)
		return -ENOSYS;

	if ((intensity < -(MAX_INTENSITY)) || (intensity > MAX_INTENSITY)) {
		pr_err("%s out of range(%d)\n", __func__, intensity);
		return -EINVAL;
	}

	ret = ddata->vib_ops->set_intensity(ddata->dev, intensity);
	if (ret)
		pr_err("%s error(%d)\n", __func__, ret);

	return ret;
}

__visible_for_testing int sec_vibrator_set_frequency(struct sec_vibrator_drvdata *ddata, int frequency)
{
	int ret = 0;

	if (!ddata) {
		pr_err("%s : ddata is NULL\n", __func__);
		return -ENODATA;
	}

	if (!ddata->vib_ops->set_frequency)
		return -ENOSYS;

	if ((frequency < FREQ_ALERT) ||
	    ((frequency >= FREQ_MAX) && (frequency < HAPTIC_ENGINE_FREQ_MIN)) ||
	    (frequency > HAPTIC_ENGINE_FREQ_MAX)) {
		pr_err("%s out of range(%d)\n", __func__, frequency);
		return -EINVAL;
	}

	ret = ddata->vib_ops->set_frequency(ddata->dev, frequency);
	if (ret)
		pr_err("%s error(%d)\n", __func__, ret);

	return ret;
}

__visible_for_testing int sec_vibrator_set_overdrive(struct sec_vibrator_drvdata *ddata, bool en)
{
	int ret = 0;

	if (!ddata) {
		pr_err("%s : ddata is NULL\n", __func__);
		return -ENODATA;
	}

	if (!ddata->vib_ops->set_overdrive)
		return -ENOSYS;

	ret = ddata->vib_ops->set_overdrive(ddata->dev, en);
	if (ret)
		pr_err("%s error(%d)\n", __func__, ret);

	return ret;
}

static void sec_vibrator_haptic_enable(struct sec_vibrator_drvdata *ddata)
{
	if (!ddata) {
		pr_err("%s : ddata is NULL\n", __func__);
		return;
	}

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	sec_vibrator_check_temp(ddata);
#endif

	sec_vibrator_set_frequency(ddata, ddata->frequency);
	sec_vibrator_set_intensity(ddata, ddata->intensity);
	sec_vibrator_set_enable(ddata, true);

	if (ddata->vib_ops->set_frequency)
		pr_info("freq:%d, intensity:%d, %dms\n", ddata->frequency, ddata->intensity, ddata->timeout);
	else if (ddata->vib_ops->set_intensity)
		pr_info("intensity:%d, %dms\n", ddata->intensity, ddata->timeout);
	else
		pr_info("%dms\n", ddata->timeout);
}

static void sec_vibrator_haptic_disable(struct sec_vibrator_drvdata *ddata)
{
	if (!ddata) {
		pr_err("%s : ddata is NULL\n", __func__);
		return;
	}

	/* clear common variables */
	ddata->index = 0;

	/* clear haptic engine variables */
	ddata->f_packet_en = false;
	ddata->packet_cnt = 0;
	ddata->packet_size = 0;

	sec_vibrator_set_enable(ddata, false);
	sec_vibrator_set_overdrive(ddata, false);
	sec_vibrator_set_frequency(ddata, FREQ_ALERT);
	sec_vibrator_set_intensity(ddata, 0);

	if (ddata->timeout > 0)
		pr_info("timeout, off\n");
	else
		pr_info("off\n");
}

static void stop_packet(struct sec_vibrator_drvdata *ddata, int intensity)
{
	if (ddata->packet_running) {
		pr_info("[haptic engine] motor stop\n");
		sec_vibrator_set_enable(ddata, false);
	}
	ddata->packet_running = false;
	sec_vibrator_set_intensity(ddata, intensity);
}

static void normal_run_packet(struct sec_vibrator_drvdata *ddata, int intensity)
{
	sec_vibrator_set_intensity(ddata, intensity);

	if (!ddata->packet_running) {
		pr_info("[haptic engine] motor run\n");
		sec_vibrator_set_enable(ddata, true);
	}
	ddata->packet_running = true;
}

static void process_normal_packet(struct sec_vibrator_drvdata *ddata, int frequency,
	int intensity)
{
	sec_vibrator_set_frequency(ddata, frequency);

	if (intensity)
		normal_run_packet(ddata, intensity);
	else
		stop_packet(ddata, intensity);
}

static void fifo_run_packet(struct sec_vibrator_drvdata *ddata, int file_num, int intensity)
{
	sec_vibrator_set_intensity(ddata, intensity);

	if (!ddata->packet_running) {
		pr_info("[haptic engine] motor run\n");
		sec_vibrator_set_enable(ddata, false);
		ddata->vib_ops->enable_fifo(ddata->dev, file_num);
	}
	ddata->packet_running = true;
}

static void process_fifo_packet(struct sec_vibrator_drvdata *ddata, int file_num,
	int intensity)
{
	ddata->vib_ops->set_fifo_intensity(ddata->dev, intensity);

	if (intensity)
		fifo_run_packet(ddata, file_num, intensity);
	else
		stop_packet(ddata, intensity);
}

__visible_for_testing void sec_vibrator_engine_run_packet(struct sec_vibrator_drvdata *ddata,
	struct vib_packet packet)
{
	int frequency = packet.freq;
	int intensity = packet.intensity;
	int overdrive = packet.overdrive;
	int fifo =  packet.fifo_flag;
	int file_num = frequency;

	if (!ddata) {
		pr_err("%s : ddata is NULL\n", __func__);
		return;
	}

	if (!ddata->f_packet_en) {
		pr_err("haptic packet is empty\n");
		return;
	}

	if (fifo == 0)
		pr_info("%s [%d] freq:%d, intensity:%d, time:%d overdrive: %d\n",
		__func__, ddata->packet_cnt, frequency, intensity,
		ddata->timeout, overdrive);
	else
		pr_info("%s [%d] fifo num:%d, intensity:%d, time:%d overdrive: %d\n",
		__func__, ddata->packet_cnt, frequency, intensity,
		ddata->timeout, overdrive);

	sec_vibrator_set_overdrive(ddata, overdrive);

	if (fifo == 0)
		process_normal_packet(ddata, frequency, intensity);
	else
		process_fifo_packet(ddata, file_num, intensity);

	pr_info("%s end\n", __func__);
}

static void sec_vibrator_on_timer(struct sec_vibrator_drvdata *ddata)
{
	struct hrtimer *timer = &ddata->timer;

	hrtimer_start(timer, ns_to_ktime((u64)ddata->timeout * NSEC_PER_MSEC), HRTIMER_MODE_REL);
}

static void sec_vibrator_start_engine_run_packet(struct sec_vibrator_drvdata *ddata)
{
	ddata->packet_running = false;
	ddata->timeout = ddata->vib_pac[0].time;
	sec_vibrator_engine_run_packet(ddata, ddata->vib_pac[0]);
}

static void sec_vibrator_process_engine_run_packet(struct sec_vibrator_drvdata *ddata)
{
	int fifo = 0;

	ddata->packet_cnt++;
	if (ddata->packet_cnt < ddata->packet_size) {
		ddata->timeout = ddata->vib_pac[ddata->packet_cnt].time;
		fifo = ddata->vib_pac[ddata->packet_cnt].fifo_flag;
		if (fifo == 0) {
			sec_vibrator_engine_run_packet(ddata, ddata->vib_pac[ddata->packet_cnt]);
			sec_vibrator_on_timer(ddata);
		} else {
			sec_vibrator_on_timer(ddata);
			sec_vibrator_engine_run_packet(ddata, ddata->vib_pac[ddata->packet_cnt]);
		}
	} else {
		ddata->f_packet_en = false;
		ddata->packet_cnt = 0;
		ddata->packet_size = 0;

		sec_vibrator_haptic_disable(ddata);
	}
}

static void timed_output_enable(struct sec_vibrator_drvdata *ddata, unsigned int value)
{
	struct hrtimer *timer = &ddata->timer;
	int ret = 0;

	if (!ddata) {
		pr_err("%s : ddata is NULL\n", __func__);
		return;
	}

	ret = hrtimer_cancel(timer);
	kthread_flush_worker(&ddata->kworker);

	mutex_lock(&ddata->vib_mutex);

	value = min_t(int, value, MAX_TIMEOUT);
	ddata->timeout = value;

	if (value) {
		if (ddata->f_packet_en)
			sec_vibrator_start_engine_run_packet(ddata);
		else
			sec_vibrator_haptic_enable(ddata);

		if (!ddata->index)
			sec_vibrator_on_timer(ddata);
	} else {
		sec_vibrator_haptic_disable(ddata);
	}

	mutex_unlock(&ddata->vib_mutex);
}

static enum hrtimer_restart haptic_timer_func(struct hrtimer *timer)
{
	struct sec_vibrator_drvdata *ddata;

	if (!timer) {
		pr_err("%s : timer is NULL\n", __func__);
		return -ENODATA;
	}

	ddata = container_of(timer, struct sec_vibrator_drvdata, timer);

	if (!ddata) {
		pr_err("%s : ddata is NULL\n", __func__);
		return -ENODATA;
	}

	pr_info("%s\n", __func__);
	kthread_queue_work(&ddata->kworker, &ddata->kwork);
	return HRTIMER_NORESTART;
}

static void sec_vibrator_work(struct kthread_work *work)
{
	struct sec_vibrator_drvdata *ddata;
	struct hrtimer *timer;

	if (!work) {
		pr_err("%s : work is NULL\n", __func__);
		return;
	}

	ddata = container_of(work, struct sec_vibrator_drvdata, kwork);

	if (!ddata) {
		pr_err("%s : ddata is NULL\n", __func__);
		return;
	}

	timer = &ddata->timer;

	if (!timer) {
		pr_err("%s : timer is NULL\n", __func__);
		return;
	}

	pr_info("%s\n", __func__);
	mutex_lock(&ddata->vib_mutex);

	if (ddata->f_packet_en)
		sec_vibrator_process_engine_run_packet(ddata);
	else
		sec_vibrator_haptic_disable(ddata);

	mutex_unlock(&ddata->vib_mutex);
}

__visible_for_testing inline bool is_valid_params(struct device *dev, struct device_attribute *attr,
	const char *buf)
{
	if (!dev) {
		pr_err("%s : dev is NULL\n", __func__);
		return false;
	}
	if (!attr) {
		pr_err("%s : attr is NULL\n", __func__);
		return false;
	}
	if (!buf) {
		pr_err("%s : buf is NULL\n", __func__);
		return false;
	}
	if (!dev_get_drvdata(dev)) {
		pr_err("%s : ddata is NULL\n", __func__);
		return false;
	}
	return true;
}

__visible_for_testing ssize_t intensity_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct sec_vibrator_drvdata *ddata;
	int intensity = 0, ret = 0;

	if (!is_valid_params(dev, attr, buf))
		return -ENODATA;

	ddata = dev_get_drvdata(dev);

	ret = kstrtoint(buf, 0, &intensity);
	if (ret) {
		pr_err("%s : fail to get intensity\n", __func__);
		return -EINVAL;
	}

	pr_info("%s %d\n", __func__, intensity);

	if ((intensity < 0) || (intensity > MAX_INTENSITY)) {
		pr_err("[VIB]: %s out of range\n", __func__);
		return -EINVAL;
	}

	ddata->intensity = intensity;

	return count;
}

__visible_for_testing ssize_t intensity_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sec_vibrator_drvdata *ddata;

	if (!is_valid_params(dev, attr, buf))
		return -ENODATA;

	ddata = dev_get_drvdata(dev);

	return snprintf(buf, VIB_BUFSIZE, "intensity: %u\n", ddata->intensity);
}

__visible_for_testing ssize_t fifo_store(struct device *dev,
			struct device_attribute *attr, const char *buf, size_t count)
{
	struct sec_vibrator_drvdata *ddata;
	int ret;
	int file_num;

	pr_info("%s +\n", __func__);

	if (!is_valid_params(dev, attr, buf)) {
		pr_err("%s param error.\n", __func__);
		return -ENODATA;
	}

	ddata = dev_get_drvdata(dev);

	ret = kstrtoint(buf, 0, &file_num);
	if (ret) {
		pr_err("fail to get file_num\n");
		return -EINVAL;
	}

	if (ddata->vib_ops->set_fifo_intensity && ddata->vib_ops->enable_fifo) {
		ret = ddata->vib_ops->set_fifo_intensity(ddata->dev, 10000);
		ret = ddata->vib_ops->enable_fifo(ddata->dev, file_num);
	} else {
		pr_err("%s unsupport fifo\n", __func__);
		return -EINVAL;
	}

	pr_info("%s -\n", __func__);
	return count;
}

__visible_for_testing ssize_t fifo_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct sec_vibrator_drvdata *ddata;
	int ret = 0;

	if (!is_valid_params(dev, attr, buf))
		return -ENODATA;

	ddata = dev_get_drvdata(dev);

	if (!ddata->vib_ops->get_fifo_filepath)
		return snprintf(buf, VIB_BUFSIZE, "NONE\n");

	ret = ddata->vib_ops->get_fifo_filepath(ddata->dev, buf);

	return ret;
}

__visible_for_testing ssize_t multi_freq_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct sec_vibrator_drvdata *ddata;
	int num, ret;

	if (!is_valid_params(dev, attr, buf))
		return -ENODATA;

	ddata = dev_get_drvdata(dev);

	ret = kstrtoint(buf, 0, &num);
	if (ret) {
		pr_err("fail to get frequency\n");
		return -EINVAL;
	}

	pr_info("%s %d\n", __func__, num);

	ddata->frequency = num;

	return count;
}

__visible_for_testing ssize_t multi_freq_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sec_vibrator_drvdata *ddata;

	if (!is_valid_params(dev, attr, buf))
		return -ENODATA;

	ddata = dev_get_drvdata(dev);

	return snprintf(buf, VIB_BUFSIZE, "frequency: %d\n", ddata->frequency);
}

static void configure_haptic_engine(struct sec_vibrator_drvdata *ddata,
	int _data, const char *buf)
{
	int index = 0, tmp = 0, buf_data = 0;

	ddata->packet_size = _data / VIB_PACKET_MAX;
	ddata->packet_cnt = 0;
	ddata->f_packet_en = true;
	buf = strstr(buf, " ");

	for (index = 0; index < ddata->packet_size; index++) {
		for (tmp = 0; tmp < VIB_PACKET_MAX; tmp++) {
			if (buf == NULL) {
				pr_err("%s, buf is NULL, Please check packet data again\n", __func__);
				ddata->f_packet_en = false;
				return;
			}

			if (sscanf(buf++, "%6d", &buf_data) != 1 || buf_data < 0) {
				pr_err("%s, packet data error, Please check packet data again\n", __func__);
				ddata->f_packet_en = false;
				return;
			}

			switch (tmp) {
			case VIB_PACKET_TIME:
				ddata->vib_pac[index].time = buf_data;
				break;
			case VIB_PACKET_INTENSITY:
				ddata->vib_pac[index].intensity = buf_data;
				break;
			case VIB_PACKET_FREQUENCY:
				ddata->vib_pac[index].freq = buf_data;
				break;
			case VIB_PACKET_OVERDRIVE:
				ddata->vib_pac[index].overdrive = buf_data;
				break;
			}
			buf = strstr(buf, " ");
		}
	}
}

__visible_for_testing ssize_t haptic_engine_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct sec_vibrator_drvdata *ddata;
	int _data = 0;

	if (!is_valid_params(dev, attr, buf))
		return -ENODATA;

	ddata = dev_get_drvdata(dev);

	if (sscanf(buf, "%6d", &_data) != 1)
		return count;

	if (_data > PACKET_MAX_SIZE * VIB_PACKET_MAX) {
		pr_info("%s, [%d] packet size over\n", __func__, _data);
		return count;
	}

	configure_haptic_engine(ddata, _data, buf);

	return count;
}

__visible_for_testing ssize_t haptic_engine_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sec_vibrator_drvdata *ddata;
	int index = 0;
	size_t size = 0;

	if (!is_valid_params(dev, attr, buf))
		return -ENODATA;

	ddata = dev_get_drvdata(dev);

	for (index = 0; index < ddata->packet_size && ddata->f_packet_en &&
	     ((4 * VIB_BUFSIZE + size) < PAGE_SIZE); index++) {
		size += snprintf(&buf[size], VIB_BUFSIZE, "%u,", ddata->vib_pac[index].time);
		size += snprintf(&buf[size], VIB_BUFSIZE, "%u,", ddata->vib_pac[index].intensity);
		size += snprintf(&buf[size], VIB_BUFSIZE, "%u,", ddata->vib_pac[index].freq);
		size += snprintf(&buf[size], VIB_BUFSIZE, "%u,", ddata->vib_pac[index].overdrive);
	}

	return size;
}

__visible_for_testing bool support_time_compensation(struct sec_vibrator_drvdata *ddata)
{
	if (ddata->time_compensation && ddata->max_delay_ms)
		return true;
	return false;
}

__visible_for_testing void calc_total_compensation(struct sec_vibrator_drvdata *ddata,
	int current_normal_packet_time, int *normal_pac_time_sum,
	int *way1_cmp_sum, int *way2_cmp_sum)
{
	*normal_pac_time_sum += current_normal_packet_time;
	*way1_cmp_sum += (current_normal_packet_time * ddata->time_compensation) / 100;
	*way2_cmp_sum += ddata->max_delay_ms;
}

__visible_for_testing int choose_compensation_way(struct sec_vibrator_drvdata *ddata,
	int normal_pac_time_sum, int way1_cmp_sum, int way2_cmp_sum)
{
	if (!support_time_compensation(ddata) || way2_cmp_sum <= ddata->max_delay_ms) {
		pr_info("%s no compensation\n", __func__);
		return VIB_NO_COMPEMSATION;
	}

	if (way1_cmp_sum < way2_cmp_sum) {
		pr_info("%s compensation way1:way1_cmp_sum=%d\n",
			__func__, way1_cmp_sum);
		return VIB_COMPENSATION_WAY1;
	}

	pr_info("%s compensation way2:way2_cmp_sum=%d,normal_pac_time_sum=%d\n",
		__func__, way2_cmp_sum, normal_pac_time_sum);
	return VIB_COMPENSATION_WAY2;
}

__visible_for_testing void normal_packet_time_correction(struct sec_vibrator_drvdata *ddata,
	int compensation_way, int normal_pac_time_sum, int way2_cmp_sum)
{
	int i, origin_time;

	for (i = 0; i < ddata->packet_size; i++) {
		if (ddata->vib_pac[i].fifo_flag == 1) {
			pr_info("%s i=%d, file=%d time=%d intensity=%d\n", __func__, i,
				ddata->vib_pac[i].freq,
				ddata->vib_pac[i].time,
				ddata->vib_pac[i].intensity);
			continue;
		}
		origin_time = ddata->vib_pac[i].time;
		if (compensation_way == VIB_COMPENSATION_WAY1)
			ddata->vib_pac[i].time -= (origin_time * ddata->time_compensation) / 100;
		else if (compensation_way == VIB_COMPENSATION_WAY2 && normal_pac_time_sum != 0)
			ddata->vib_pac[i].time -= (way2_cmp_sum*origin_time)/normal_pac_time_sum;

		pr_info("%s i=%d, origin_time=%d->time=%d intensity=%d freq=%d\n",
			__func__, i, origin_time,
			ddata->vib_pac[i].time,
			ddata->vib_pac[i].intensity,
			ddata->vib_pac[i].freq);
	}
}

static void configure_hybrid_haptic_engine(struct sec_vibrator_drvdata *ddata,
	int _data, const char *buf)
{
	int i = 0, tmp = 0, check_fifo = 0, buf_data = 0;
	int way1_cmp_sum = 0, way2_cmp_sum = 0;
	int normal_pac_time_sum = 0;
	int compensation_way = VIB_NO_COMPEMSATION;

	ddata->packet_size = _data / VIB_PACKET_MAX;
	ddata->packet_cnt = 0;
	ddata->f_packet_en = true;

	buf = strstr(buf, " ");

	/*
	 * hybrid haptic packet example (with fifo index)
	 *	ex) 8 1000 5000 2000 0 1000 10000 \#18 0
	 */
	for (i = 0; i < ddata->packet_size; i++) {
		check_fifo = 0;
		for (tmp = 0; tmp < VIB_PACKET_MAX; tmp++) {
			if (buf == NULL) {
				pr_err("%s, buf is NULL, Please check packet data again\n",
						__func__);
				ddata->f_packet_en = false;
				return;
			}

			if (*(buf+1) == '#') {
				buf = buf+2;
				check_fifo = 1;
			}

			if (sscanf(buf++, "%6d", &buf_data) != 1 || buf_data < 0) {
				pr_err("%s, packet data error, Please check packet data again\n",
						__func__);
				ddata->f_packet_en = false;
				return;
			}

			switch (tmp) {
			case VIB_PACKET_TIME:
				ddata->vib_pac[i].time = buf_data;
				break;
			case VIB_PACKET_INTENSITY:
				ddata->vib_pac[i].intensity = buf_data;
				break;
			case VIB_PACKET_FREQUENCY:
				ddata->vib_pac[i].freq = buf_data;
				if (check_fifo == 0) {
					ddata->vib_pac[i].fifo_flag = 0;
					if (support_time_compensation(ddata))
						calc_total_compensation(ddata, ddata->vib_pac[i].time,
							&normal_pac_time_sum, &way1_cmp_sum, &way2_cmp_sum);
				} else
					ddata->vib_pac[i].fifo_flag = 1;
				break;
			case VIB_PACKET_OVERDRIVE:
				ddata->vib_pac[i].overdrive = buf_data;
				break;
			}
			buf = strstr(buf, " ");
		}
	}

	compensation_way = choose_compensation_way(ddata,
		normal_pac_time_sum, way1_cmp_sum, way2_cmp_sum);

	normal_packet_time_correction(ddata, compensation_way,
		normal_pac_time_sum, way2_cmp_sum);
}

static ssize_t hybrid_haptic_engine_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct sec_vibrator_drvdata *ddata;
	int _data = 0;

	if (!is_valid_params(dev, attr, buf))
		return -ENODATA;

	ddata = dev_get_drvdata(dev);

	pr_info("%s +\n", __func__);

	if (sscanf(buf, "%6d", &_data) != 1)
		return count;

	if (_data > PACKET_MAX_SIZE * VIB_PACKET_MAX) {
		pr_info("%s, [%d] packet size over\n", __func__, _data);
		return count;
	}

	configure_hybrid_haptic_engine(ddata, _data, buf);

	pr_info("%s -\n", __func__);

	return count;
}

static ssize_t hybrid_haptic_engine_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct sec_vibrator_drvdata *ddata;
	int i = 0;
	size_t size = 0;

	if (!is_valid_params(dev, attr, buf))
		return -ENODATA;

	ddata = dev_get_drvdata(dev);

	for (i = 0; i < ddata->packet_size && ddata->f_packet_en &&
			((4 * VIB_BUFSIZE + size) < PAGE_SIZE); i++) {
		size += snprintf(&buf[size], VIB_BUFSIZE, "%u,", ddata->vib_pac[i].time);
		size += snprintf(&buf[size], VIB_BUFSIZE, "%u,", ddata->vib_pac[i].intensity);
		size += snprintf(&buf[size], VIB_BUFSIZE, "%u,", ddata->vib_pac[i].freq);
		size += snprintf(&buf[size], VIB_BUFSIZE, "%u,", ddata->vib_pac[i].overdrive);
	}

	return size;
}

__visible_for_testing ssize_t cp_trigger_index_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sec_vibrator_drvdata *ddata;
	int ret = 0;

	if (!is_valid_params(dev, attr, buf))
		return -ENODATA;

	ddata = dev_get_drvdata(dev);

	if (!ddata->vib_ops->get_cp_trigger_index)
		return -ENOSYS;

	ret = ddata->vib_ops->get_cp_trigger_index(ddata->dev, buf);

	return ret;
}

__visible_for_testing ssize_t cp_trigger_index_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct sec_vibrator_drvdata *ddata;
	int ret = 0;
	unsigned int index;

	if (!is_valid_params(dev, attr, buf))
		return -ENODATA;

	ddata = dev_get_drvdata(dev);

	if (!ddata->vib_ops->set_cp_trigger_index)
		return -ENOSYS;

	ret = kstrtou32(buf, 10, &index);
	if (ret)
		return -EINVAL;

	ddata->index = index;

	ret = ddata->vib_ops->set_cp_trigger_index(ddata->dev, buf);
	if (ret < 0)
		pr_err("%s error(%d)\n", __func__, ret);

	return count;
}

__visible_for_testing ssize_t cp_trigger_queue_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sec_vibrator_drvdata *ddata;
	int ret = 0;

	if (!is_valid_params(dev, attr, buf))
		return -ENODATA;

	ddata = dev_get_drvdata(dev);

	if (!ddata->vib_ops->get_cp_trigger_queue)
		return -ENOSYS;

	ret = ddata->vib_ops->get_cp_trigger_queue(ddata->dev, buf);
	if (ret < 0)
		pr_err("%s error(%d)\n", __func__, ret);

	return ret;
}

__visible_for_testing ssize_t cp_trigger_queue_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct sec_vibrator_drvdata *ddata;
	int ret = 0;

	if (!is_valid_params(dev, attr, buf))
		return -ENODATA;

	ddata = dev_get_drvdata(dev);

	if (!ddata->vib_ops->set_cp_trigger_queue)
		return -ENOSYS;

	ret = ddata->vib_ops->set_cp_trigger_queue(ddata->dev, buf);
	if (ret < 0)
		pr_err("%s error(%d)\n", __func__, ret);

	return count;
}

__visible_for_testing ssize_t pwle_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sec_vibrator_drvdata *ddata;
	int ret = 0;

	if (!is_valid_params(dev, attr, buf))
		return -ENODATA;

	ddata = dev_get_drvdata(dev);

	if (!ddata->vib_ops->get_pwle)
		return -ENOSYS;

	ret = ddata->vib_ops->get_pwle(ddata->dev, buf);
	if (ret < 0)
		pr_err("%s error(%d)\n", __func__, ret);

	return ret;
}

__visible_for_testing ssize_t pwle_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t count)
{
	struct sec_vibrator_drvdata *ddata;
	int ret = 0;

	if (!is_valid_params(dev, attr, buf))
		return -ENODATA;

	ddata = dev_get_drvdata(dev);

	if (!ddata->vib_ops->set_pwle)
		return -ENOSYS;

	ret = ddata->vib_ops->set_pwle(ddata->dev, buf);
	if (ret < 0)
		pr_err("%s error(%d)\n", __func__, ret);

	return count;
}

__visible_for_testing ssize_t virtual_composite_indexes_show(struct device *dev, struct device_attribute *attr,
	char *buf)
{
	struct sec_vibrator_drvdata *ddata;
	int ret = 0;

	if (!is_valid_params(dev, attr, buf))
		return -ENODATA;

	ddata = dev_get_drvdata(dev);

	if (!ddata->vib_ops->get_virtual_composite_indexes)
		return -ENOSYS;

	ret = ddata->vib_ops->get_virtual_composite_indexes(ddata->dev, buf);
	if (ret < 0)
		pr_err("%s error(%d)\n", __func__, ret);

	return ret;
}

__visible_for_testing ssize_t virtual_pwle_indexes_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sec_vibrator_drvdata *ddata;
	int ret = 0;

	if (!is_valid_params(dev, attr, buf))
		return -ENODATA;

	ddata = dev_get_drvdata(dev);

	if (!ddata->vib_ops->get_virtual_pwle_indexes)
		return -ENOSYS;

	ret = ddata->vib_ops->get_virtual_pwle_indexes(ddata->dev, buf);
	if (ret < 0)
		pr_err("%s error(%d)\n", __func__, ret);

	return ret;
}

__visible_for_testing ssize_t enable_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sec_vibrator_drvdata *ddata;
	struct hrtimer *timer;
	int remaining = 0;

	if (!is_valid_params(dev, attr, buf))
		return -ENODATA;

	ddata = dev_get_drvdata(dev);
	timer = &ddata->timer;

	if (hrtimer_active(timer)) {
		ktime_t remain = hrtimer_get_remaining(timer);
		struct timespec64 t = ns_to_timespec64(remain);

		remaining = t.tv_sec * 1000 + t.tv_nsec / 1000;
	}
	return sprintf(buf, "%d\n", remaining);
}

__visible_for_testing ssize_t enable_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t size)
{
	struct sec_vibrator_drvdata *ddata;
	int value;
	int ret;

	if (!is_valid_params(dev, attr, buf))
		return -ENODATA;

	ddata = dev_get_drvdata(dev);

	ret = kstrtoint(buf, 0, &value);
	if (ret != 0)
		return -EINVAL;

	timed_output_enable(ddata, value);
	return size;
}

__visible_for_testing ssize_t motor_type_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sec_vibrator_drvdata *ddata;
	int ret = 0;

	if (!is_valid_params(dev, attr, buf))
		return -ENODATA;

	ddata = dev_get_drvdata(dev);

	if (!ddata->vib_ops->get_motor_type)
		return snprintf(buf, VIB_BUFSIZE, "NONE\n");

	ret = ddata->vib_ops->get_motor_type(ddata->dev, buf);

	return ret;
}

__visible_for_testing ssize_t use_sep_index_store(struct device *dev, struct device_attribute *attr,
	const char *buf, size_t size)
{
	struct sec_vibrator_drvdata *ddata;
	int ret = 0;
	bool use_sep_index;

	if (!is_valid_params(dev, attr, buf))
		return -ENODATA;

	ddata = dev_get_drvdata(dev);

	ret = kstrtobool(buf, &use_sep_index);
	if (ret < 0) {
		pr_err("%s kstrtobool error : %d\n", __func__, ret);
		goto err;
	}
	
	pr_info("%s use_sep_index:%d\n", __func__, use_sep_index);
	
	if (ddata->vib_ops->set_use_sep_index) {
		ret = ddata->vib_ops->set_use_sep_index(ddata->dev, use_sep_index);
		if (ret) {
			pr_err("%s set_use_sep_index error : %d\n", __func__, ret);
			goto err;
		}
	} else {
		pr_info("%s this model doesn't need use_sep_index\n", __func__);
	}
err:
	return ret ? ret : size;
}

int sec_vibrator_recheck_ratio(struct sec_vibrator_drvdata *ddata)
{
	int temp = ddata->temperature;

	if (temp >= ddata->pdata->high_temp_ref)
		return ddata->pdata->high_temp_ratio;

	return ddata->pdata->normal_ratio;
}
EXPORT_SYMBOL_GPL(sec_vibrator_recheck_ratio);

__visible_for_testing ssize_t current_temp_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct sec_vibrator_drvdata *ddata;

	if (!is_valid_params(dev, attr, buf))
		return -ENODATA;

	ddata = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", ddata->temperature);
}

__visible_for_testing ssize_t current_temp_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct sec_vibrator_drvdata *ddata;
	int temp, ret = 0;

	if (!is_valid_params(dev, attr, buf))
		return -ENODATA;

	ddata = dev_get_drvdata(dev);

	ret = kstrtos32(buf, 0, &temp);
	if (ret < 0) {
		pr_err("%s kstrtos32 error : %d\n", __func__, ret);
		ddata->temperature = INT_MIN;
		goto err;
	}

	ddata->temperature = temp;
	pr_info("%s temperature : %d\n", __func__, ddata->temperature);
err:
	return ret ? ret : count;
}

__visible_for_testing ssize_t num_waves_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sec_vibrator_drvdata *ddata;
	int ret = 0;

	if (!is_valid_params(dev, attr, buf))
		return -ENODATA;

	ddata = dev_get_drvdata(dev);

	if (!ddata->vib_ops->get_num_waves)
		return -ENOSYS;

	ret = ddata->vib_ops->get_num_waves(ddata->dev, buf);

	return ret;
}

__visible_for_testing ssize_t array2str(struct sec_vibrator_drvdata *ddata,
	char *buf, int *arr_intensity, int size)
{
	int index, ret = 0;
	char *str_buf = NULL;

	if (!buf || !ddata)
		return -ENODATA;

	if (!arr_intensity || ((size < 1) && (size > kmax_haptic_step_size)))
		return -EINVAL;

	str_buf = kzalloc(kmax_buf_size, GFP_KERNEL);
	if (!str_buf)
		return -ENOMEM;

	mutex_lock(&ddata->vib_mutex);
	for (index = 0; index < size; index++) {
		if (index < (size - 1))
			snprintf(str_buf, kmax_buf_size, "%u,", arr_intensity[index]);
		else
			snprintf(str_buf, (kmax_buf_size - 1), "%u", arr_intensity[index]);
		strncat(buf, str_buf, strlen(str_buf));
	}
	strncat(buf, str_newline, strlen(str_newline));
	ret = strlen(buf);
	mutex_unlock(&ddata->vib_mutex);

	kfree(str_buf);
	return ret;
}

__visible_for_testing ssize_t intensities_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct sec_vibrator_drvdata *ddata;
	int *arr_intensity = NULL;
	int ret = 0, step_size = 0;

	pr_info("%s\n", __func__);

	if (!is_valid_params(dev, attr, buf))
		return -ENODATA;

	ddata = dev_get_drvdata(dev);

	if (!ddata->vib_ops->get_step_size || !ddata->vib_ops->get_intensities)
		return -EOPNOTSUPP;

	ret = ddata->vib_ops->get_step_size(ddata->dev, &step_size);
	if (ret)
		return -EINVAL;

	arr_intensity = kmalloc_array(kmax_haptic_step_size, sizeof(int), GFP_KERNEL);
	if (!arr_intensity)
		return -ENOMEM;

	if ((step_size > 0) && (step_size < kmax_haptic_step_size)) {
		ret = ddata->vib_ops->get_intensities(ddata->dev, arr_intensity);
		if (ret) {
			ret = -EINVAL;
			goto err_arr_alloc;
		}
		ret = array2str(ddata, buf, arr_intensity, step_size);
	} else {
		ret = -EINVAL;
	}

err_arr_alloc:
	kfree(arr_intensity);
	return ret;
}

__visible_for_testing ssize_t haptic_intensities_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct sec_vibrator_drvdata *ddata;
	int *arr_intensity = NULL;
	int ret = 0, step_size = 0;

	pr_info("%s\n", __func__);

	if (!is_valid_params(dev, attr, buf))
		return -ENODATA;

	ddata = dev_get_drvdata(dev);

	if (!ddata->vib_ops->get_step_size || !ddata->vib_ops->get_haptic_intensities)
		return -EOPNOTSUPP;

	ret = ddata->vib_ops->get_step_size(ddata->dev, &step_size);
	if (ret)
		return -EINVAL;

	arr_intensity = kmalloc_array(kmax_haptic_step_size, sizeof(int), GFP_KERNEL);
	if (!arr_intensity)
		return -ENOMEM;

	if ((step_size > 0) && (step_size < kmax_haptic_step_size)) {
		ret = ddata->vib_ops->get_haptic_intensities(ddata->dev, arr_intensity);
		if (ret) {
			ret = -EINVAL;
			goto err_arr_alloc;
		}
		ret = array2str(ddata, buf, arr_intensity, step_size);
	} else {
		ret = -EINVAL;
	}

err_arr_alloc:
	kfree(arr_intensity);
	return ret;
}

__visible_for_testing ssize_t haptic_durations_show(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	struct sec_vibrator_drvdata *ddata;
	int *arr_duration = NULL;
	int ret = 0, step_size = 0;

	pr_info("%s\n", __func__);

	if (!is_valid_params(dev, attr, buf))
		return -ENODATA;

	ddata = dev_get_drvdata(dev);

	if (!ddata->vib_ops->get_step_size || !ddata->vib_ops->get_haptic_durations)
		return -EOPNOTSUPP;

	ret = ddata->vib_ops->get_step_size(ddata->dev, &step_size);
	if (ret)
		return -EINVAL;

	arr_duration = kmalloc_array(kmax_haptic_step_size, sizeof(int), GFP_KERNEL);
	if (!arr_duration)
		return -ENOMEM;

	if ((step_size > 0) && (step_size < kmax_haptic_step_size)) {
		ret = ddata->vib_ops->get_haptic_durations(ddata->dev, arr_duration);
		if (ret) {
			ret = -EINVAL;
			goto err_arr_alloc;
		}
		ret = array2str(ddata, buf, arr_duration, step_size);
	} else {
		ret = -EINVAL;
	}

err_arr_alloc:
	kfree(arr_duration);
	return ret;
}

__visible_for_testing int get_event_index_by_command(char *cur_cmd)
{
	int cmd_idx;

	for (cmd_idx = 0; cmd_idx < EVENT_CMD_MAX; cmd_idx++) {
		if (!strcmp(cur_cmd, sec_vib_event_cmd[cmd_idx]))
			return cmd_idx;
	}

	return EVENT_CMD_NONE;
}

__visible_for_testing ssize_t event_cmd_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct sec_vibrator_drvdata *ddata;

	pr_info("%s event: %s\n", __func__, vib_event_cmd);

	if (!is_valid_params(dev, attr, buf))
		return -ENODATA;

	ddata = dev_get_drvdata(dev);

	return snprintf(buf, MAX_STR_LEN_EVENT_CMD, "%s\n", vib_event_cmd);
}

__visible_for_testing ssize_t event_cmd_store(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	struct sec_vibrator_drvdata *ddata;
	char *cmd;
	int ret = 0;
	int event_idx;

	if (!is_valid_params(dev, attr, buf))
		return -ENODATA;

	ddata = dev_get_drvdata(dev);

	if (!ddata->vib_ops->set_event_cmd)
		return -ENOSYS;

	if (count >= MAX_STR_LEN_EVENT_CMD) {
		pr_err("%s: size(%zu) is too long.\n", __func__, count);
		goto error;
	}

	cmd = kzalloc(count + 1, GFP_KERNEL);
	if (!cmd)
		goto error;

	ret = sscanf(buf, "%s", cmd);
	if (ret != 1)
		goto error1;

	event_idx = get_event_index_by_command(cmd);

	pr_info("%s: event: %s(%d)\n", __func__, cmd, event_idx);

	ret = ddata->vib_ops->set_event_cmd(ddata->dev, event_idx);
	if (ret)
		pr_err("%s error(%d)\n", __func__, ret);

	sscanf(cmd, "%s", vib_event_cmd);

error1:
	kfree(cmd);
error:
	return count;
}

__visible_for_testing ssize_t freq_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sec_vibrator_drvdata *ddata;

	ddata = dev_get_drvdata(dev);

	if (!is_valid_params(dev, attr, buf))
		return -ENODATA;

	ddata = dev_get_drvdata(dev);

	return snprintf(buf, VIB_BUFSIZE, "%d\n", ddata->frequency);
}

__visible_for_testing ssize_t freq_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int ret, num;
	struct sec_vibrator_drvdata *ddata;

	if (!is_valid_params(dev, attr, buf))
		return -ENODATA;

	ddata = dev_get_drvdata(dev);

	ret = kstrtoint(buf, 0, &num);
	if (ret) {
		pr_err("fail to get frequency\n");
		return -EINVAL;
	}

	pr_info("%s %d\n", __func__, num);

	if (!ddata->vib_ops->set_frequency)
		return -ENOSYS;

	ddata->frequency = num;

	ddata->vib_ops->set_frequency(ddata->dev, num);

	return count;
}

__visible_for_testing ssize_t freq_stored_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct sec_vibrator_drvdata *ddata;

	ddata = dev_get_drvdata(dev);

	if (!is_valid_params(dev, attr, buf))
		return -ENODATA;

	ddata = dev_get_drvdata(dev);
	if (!ddata->vib_ops->get_freq_stored)
		return -ENOSYS;

	return ddata->vib_ops->get_freq_stored(ddata->dev, buf);
}

__visible_for_testing ssize_t freq_stored_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct sec_vibrator_drvdata *ddata;
	int frequency = 0, ret = 0;

	if (!is_valid_params(dev, attr, buf))
		return -ENODATA;

	ddata = dev_get_drvdata(dev);

	ret = kstrtoint(buf, 0, &frequency);
	if (ret) {
		pr_err("%s : fail to get freq_stored\n", __func__);
		return -EINVAL;
	}

	if (!ddata->vib_ops->set_freq_stored)
		return -ENOSYS;

	ddata->vib_ops->set_freq_stored(ddata->dev, frequency);
	return count;
}

static DEVICE_ATTR_RW(haptic_engine);
static DEVICE_ATTR_RW(multi_freq);
static DEVICE_ATTR_RW(intensity);
static DEVICE_ATTR_RW(cp_trigger_index);
static DEVICE_ATTR_RW(cp_trigger_queue);
static DEVICE_ATTR_RW(pwle);
static DEVICE_ATTR_RO(virtual_composite_indexes);
static DEVICE_ATTR_RO(virtual_pwle_indexes);
static DEVICE_ATTR_RW(enable);
static DEVICE_ATTR_RO(motor_type);
static DEVICE_ATTR_WO(use_sep_index);
static DEVICE_ATTR_RW(current_temp);
static DEVICE_ATTR_RW(fifo);
static DEVICE_ATTR_RW(hybrid_haptic_engine);
static DEVICE_ATTR_RO(num_waves);
static DEVICE_ATTR_RO(intensities);
static DEVICE_ATTR_RO(haptic_intensities);
static DEVICE_ATTR_RO(haptic_durations);
static DEVICE_ATTR_RW(event_cmd);
static DEVICE_ATTR_RW(freq);
static DEVICE_ATTR_RW(freq_stored);

static struct attribute *sec_vibrator_attributes[] = {
	&dev_attr_enable.attr,
	&dev_attr_motor_type.attr,
	&dev_attr_use_sep_index.attr,
	&dev_attr_current_temp.attr,
	&dev_attr_fifo.attr,
	&dev_attr_hybrid_haptic_engine.attr,
	NULL,
};

static struct attribute_group sec_vibrator_attr_group = {
	.attrs = sec_vibrator_attributes,
};

static struct attribute *multi_freq_attributes[] = {
	&dev_attr_haptic_engine.attr,
	&dev_attr_multi_freq.attr,
	&dev_attr_freq.attr,
	&dev_attr_freq_stored.attr,
	NULL,
};

static struct attribute_group multi_freq_attr_group = {
	.attrs = multi_freq_attributes,
};

static struct attribute *cp_trigger_attributes[] = {
	&dev_attr_haptic_engine.attr,
	&dev_attr_num_waves.attr,
	&dev_attr_cp_trigger_index.attr,
	&dev_attr_cp_trigger_queue.attr,
	NULL,
};

static struct attribute_group cp_trigger_attr_group = {
	.attrs = cp_trigger_attributes,
};

static struct attribute *pwle_attributes[] = {
	&dev_attr_pwle.attr,
	&dev_attr_virtual_composite_indexes.attr,
	&dev_attr_virtual_pwle_indexes.attr,
	NULL,
};

static struct attribute_group pwle_attr_group = {
	.attrs = pwle_attributes,
};

int sec_vibrator_register(struct sec_vibrator_drvdata *ddata)
{
	struct task_struct *kworker_task;
	int ret = 0;

	if (!ddata) {
		pr_err("%s no ddata\n", __func__);
		return -ENODEV;
	}

	ret = sec_vibrator_pdata_init();
	if (ret) {
		pr_err("%s sec_vibrator_pdata_init error.\n", __func__);
		return ret;
	}

	mutex_init(&ddata->vib_mutex);
	kthread_init_worker(&ddata->kworker);
	kworker_task = kthread_run(kthread_worker_fn, &ddata->kworker, "sec_vibrator");

	if (IS_ERR(kworker_task)) {
		pr_err("Failed to create message pump task\n");
		ret = -ENOMEM;
		goto err_kthread;
	}

	kthread_init_work(&ddata->kwork, sec_vibrator_work);
	hrtimer_init(&ddata->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	ddata->timer.function = haptic_timer_func;

	/* create /sys/class/timed_output/vibrator */
#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 4, 0)
	ddata->to_class = class_create(THIS_MODULE, "timed_output");
#else
	ddata->to_class = class_create("timed_output");
#endif
	if (IS_ERR(ddata->to_class)) {
		ret = PTR_ERR(ddata->to_class);
		goto err_class_create;
	}
	ddata->to_dev = device_create(ddata->to_class, NULL, MKDEV(0, 0), ddata, "vibrator");
	if (IS_ERR(ddata->to_dev)) {
		ret = PTR_ERR(ddata->to_dev);
		goto err_device_create;
	}

	dev_set_drvdata(ddata->to_dev, ddata);

	ret = sysfs_create_group(&ddata->to_dev->kobj, &sec_vibrator_attr_group);
	if (ret) {
		ret = -ENODEV;
		pr_err("Failed to create sysfs1 %d\n", ret);
		goto err_sysfs1;
	}

	if (ddata->vib_ops->set_intensity) {
		ret = sysfs_create_file(&ddata->to_dev->kobj, &dev_attr_intensity.attr);
		if (ret) {
			ret = -ENODEV;
			pr_err("Failed to create sysfs2 %d\n", ret);
			goto err_sysfs2;
		}

		ddata->intensity = MAX_INTENSITY;
	}

	if (ddata->vib_ops->set_frequency) {
		ret = sysfs_create_group(&ddata->to_dev->kobj, &multi_freq_attr_group);
		if (ret) {
			ret = -ENODEV;
			pr_err("Failed to create sysfs3 %d\n", ret);
			goto err_sysfs3;
		}

		ddata->frequency = FREQ_ALERT;
	}

	if (ddata->vib_ops->set_cp_trigger_index) {
		ret = sysfs_create_group(&ddata->to_dev->kobj, &cp_trigger_attr_group);
		if (ret) {
			ret = -ENODEV;
			pr_err("Failed to create sysfs4 %d\n", ret);
			goto err_sysfs4;
		}

	}

	if (ddata->vib_ops->set_pwle) {
		ret = sysfs_create_group(&ddata->to_dev->kobj, &pwle_attr_group);
		if (ret) {
			ret = -ENODEV;
			pr_err("Failed to create sysfs5 %d\n", ret);
			goto err_sysfs5;
		}

	}

	if (ddata->vib_ops->set_event_cmd) {
		/* initialize event_cmd string for HAL init */
		sscanf(ddata->event_cmd, "%s", vib_event_cmd);
		pr_info("%s: vib_event_cmd: %s\n", __func__, vib_event_cmd);

		ret = sysfs_create_file(&ddata->to_dev->kobj, &dev_attr_event_cmd.attr);
		if (ret) {
			ret = -ENODEV;
			pr_err("Failed to create sysfs6 %d\n", ret);
			goto err_sysfs6;
		}
	}

	if (ddata->vib_ops->get_calibration && ddata->vib_ops->get_calibration(ddata->dev)) {
		if (ddata->vib_ops->get_intensities) {
			ret = sysfs_create_file(&ddata->to_dev->kobj, &dev_attr_intensities.attr);
			if (ret) {
				ret = -ENODEV;
				pr_err("Failed to create intensities %d\n", ret);
				goto err_cal1;
			}
		}

		if (ddata->vib_ops->get_haptic_intensities) {
			ret = sysfs_create_file(&ddata->to_dev->kobj, &dev_attr_haptic_intensities.attr);
			if (ret) {
				ret = -ENODEV;
				pr_err("Failed to create haptic_intensities %d\n", ret);
				goto err_cal2;
			}
		}

		if (ddata->vib_ops->get_haptic_durations) {
			ret = sysfs_create_file(&ddata->to_dev->kobj, &dev_attr_haptic_durations.attr);
			if (ret) {
				ret = -ENODEV;
				pr_err("Failed to create haptic_intensities %d\n", ret);
				goto err_cal3;
			}
		}
	}

	ddata->pdata = s_v_pdata;

	pr_info("%s done\n", __func__);

	ddata->is_registered = true;

	return ret;

err_cal3:
	if (ddata->vib_ops->get_calibration && ddata->vib_ops->get_calibration(ddata->dev)) {
		if (ddata->vib_ops->get_haptic_intensities)
			sysfs_remove_file(&ddata->to_dev->kobj, &dev_attr_haptic_intensities.attr);
	}
err_cal2:
	if (ddata->vib_ops->get_calibration && ddata->vib_ops->get_calibration(ddata->dev)) {
		if (ddata->vib_ops->get_intensities)
			sysfs_remove_file(&ddata->to_dev->kobj, &dev_attr_intensities.attr);
	}
err_cal1:
	if (ddata->vib_ops->set_event_cmd)
		sysfs_remove_file(&ddata->to_dev->kobj, &dev_attr_event_cmd.attr);
err_sysfs6:
	if (ddata->vib_ops->set_pwle)
		sysfs_remove_group(&ddata->to_dev->kobj, &pwle_attr_group);
err_sysfs5:
	if (ddata->vib_ops->set_cp_trigger_index)
		sysfs_remove_group(&ddata->to_dev->kobj, &cp_trigger_attr_group);
err_sysfs4:
	if (ddata->vib_ops->set_frequency)
		sysfs_remove_group(&ddata->to_dev->kobj, &multi_freq_attr_group);
err_sysfs3:
	if (ddata->vib_ops->set_intensity)
		sysfs_remove_file(&ddata->to_dev->kobj, &dev_attr_intensity.attr);
err_sysfs2:
	sysfs_remove_group(&ddata->to_dev->kobj, &sec_vibrator_attr_group);
err_sysfs1:
	dev_set_drvdata(ddata->to_dev, NULL);
	device_destroy(ddata->to_class, MKDEV(0, 0));
err_device_create:
	class_destroy(ddata->to_class);
err_class_create:
err_kthread:
	mutex_destroy(&ddata->vib_mutex);
	return ret;
}
EXPORT_SYMBOL_GPL(sec_vibrator_register);

int sec_vibrator_unregister(struct sec_vibrator_drvdata *ddata)
{
	if (!ddata || !ddata->is_registered) {
		pr_info("%s: sec_vibrator not registered, just return\n", __func__);
		return -ENODEV;
	}

	sec_vibrator_haptic_disable(ddata);

	if (ddata->vib_ops->get_calibration && ddata->vib_ops->get_calibration(ddata->dev)) {
		if (ddata->vib_ops->get_haptic_intensities)
			sysfs_remove_file(&ddata->to_dev->kobj, &dev_attr_haptic_intensities.attr);
		if (ddata->vib_ops->get_intensities)
			sysfs_remove_file(&ddata->to_dev->kobj, &dev_attr_intensities.attr);
	}
	if (ddata->vib_ops->set_event_cmd)
		sysfs_remove_file(&ddata->to_dev->kobj, &dev_attr_event_cmd.attr);
	if (ddata->vib_ops->set_pwle)
		sysfs_remove_group(&ddata->to_dev->kobj, &pwle_attr_group);
	if (ddata->vib_ops->set_cp_trigger_index)
		sysfs_remove_group(&ddata->to_dev->kobj, &cp_trigger_attr_group);
	if (ddata->vib_ops->set_frequency)
		sysfs_remove_group(&ddata->to_dev->kobj, &multi_freq_attr_group);
	if (ddata->vib_ops->set_intensity)
		sysfs_remove_file(&ddata->to_dev->kobj, &dev_attr_intensity.attr);

	sysfs_remove_group(&ddata->to_dev->kobj, &sec_vibrator_attr_group);

	dev_set_drvdata(ddata->to_dev, NULL);
	device_destroy(ddata->to_class, MKDEV(0, 0));
	class_destroy(ddata->to_class);
	mutex_destroy(&ddata->vib_mutex);
	return 0;
}
EXPORT_SYMBOL_GPL(sec_vibrator_unregister);

static int sec_vibrator_parse_dt(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	int ret = 0;
	int temp;

	pr_info("%s +++\n", __func__);

	if (unlikely(!np)) {
		pr_err("err: could not parse dt node\n");
		return -ENODEV;
	}

	ret = of_property_read_u32(np, "haptic,normal_ratio", &temp);
	if (ret) {
		pr_err("%s: WARNING! normal_ratio not found\n", __func__);
		s_v_pdata->normal_ratio = 100;  // 100%
	} else
		s_v_pdata->normal_ratio = (int)temp;

	ret = of_property_read_u32(np, "haptic,high_temp_ref", &temp);
	if (ret) {
		pr_info("%s: high temperature concept isn't used\n", __func__);
		s_v_pdata->high_temp_ref = SEC_VIBRATOR_DEFAULT_HIGH_TEMP_REF;
	} else {
		s_v_pdata->high_temp_ref = (int)temp;

		ret = of_property_read_u32(np, "haptic,high_temp_ratio", &temp);
		if (ret) {
			pr_info("%s: high_temp_ratio isn't used\n", __func__);
			s_v_pdata->high_temp_ratio = SEC_VIBRATOR_DEFAULT_HIGH_TEMP_RATIO;
		} else
			s_v_pdata->high_temp_ratio = (int)temp;
	}

	pr_info("%s : done! ---\n", __func__);
	return 0;
}

static int sec_vibrator_probe(struct platform_device *pdev)
{
	int ret = 0;

	pr_info("%s +++\n", __func__);

	ret = sec_vibrator_pdata_init();
	if (ret) {
		pr_err("%s sec_vibrator_pdata_init error.\n", __func__);
		return ret;
	}

	if (unlikely(sec_vibrator_parse_dt(pdev)))
		pr_err("%s: WARNING!>..parse dt failed\n", __func__);

	s_v_pdata->probe_done = true;
	pr_info("%s : done! ---\n", __func__);

	return 0;
}

static int sec_vibrator_remove(struct platform_device *pdev)
{
	kfree(s_v_pdata);
	return 0;
}

static const struct of_device_id sec_vibrator_id[] = {
	{ .compatible = "sec_vibrator" },
	{ }
};

static struct platform_driver sec_vibrator_driver = {
	.probe		= sec_vibrator_probe,
	.remove		= sec_vibrator_remove,
	.driver		= {
		.name	=  "sec_vibrator",
		.owner	= THIS_MODULE,
		.of_match_table = sec_vibrator_id,
	},
};

module_platform_driver(sec_vibrator_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("sec vibrator driver");
