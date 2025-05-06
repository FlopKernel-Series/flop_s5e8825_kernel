// SPDX-License-Identifier: GPL-2.0
/*
 *  Samsung Block Statistics
 *
 *  Copyright (C) 2021 Manjong Lee <mj0123.lee@samsung.com>
 *  Copyright (C) 2021 Junho Kim <junho89.kim@samsung.com>
 *  Copyright (C) 2021 Changheun Lee <nanich.lee@samsung.com>
 *  Copyright (C) 2021 Seunghwan Hyun <seunghwan.hyun@samsung.com>
 *  Copyright (C) 2021 Tran Xuan Nam <nam.tx2@samsung.com>
 */

#include <linux/sysfs.h>
#include <linux/blk_types.h>
#include <linux/blkdev.h>
#include <linux/cpufreq.h>
#include <linux/log2.h>
#include <linux/pm_qos.h>

#include "blk-sec.h"

struct traffic {
	u64 transferred_bytes;
	int level;
	unsigned int timestamp;

	struct gendisk *gd;
	struct work_struct update_work;
	struct delayed_work notify_work;
};

static DEFINE_PER_CPU(u64, transferred_bytes);
static DEFINE_PER_CPU(struct freq_qos_request, cpufreq_req);
static struct pm_qos_request cpu_pm_req;
static unsigned int interval_ms = 1000;
static unsigned int interval_bytes = 100 * 1024 * 1024;
static struct traffic traffic;

#define TL0_UEVENT_DELAY_MS 2000

#define UPDATE_WORK_TO_TRAFFIC(work) \
	container_of(work, struct traffic, update_work)
#define NOTIFY_WORK_TO_TRAFFIC(work) \
	container_of(to_delayed_work(work), struct traffic, notify_work)

static u64 get_transferred_bytes(void)
{
	u64 bytes = 0;
	int cpu;

	for_each_possible_cpu(cpu)
		bytes += per_cpu(transferred_bytes, cpu);

	return bytes;
}

/*
 * Convert throughput to level. Level is defined as below:
 * 0: 0   - "< 100" MB/s
 * 1: 100 - "< 200" MB/s
 * 2: 200 - "< 400" MB/s
 * 3: 400 - "< 800" MB/s
 * ...so on
 */
static int tp2level(int tput)
{
	if (tput < 100)
		return 0;
	return (int) ilog2(tput / 100) + 1;
}

static void notify_traffic_level(struct traffic *traffic)
{
#define BUF_SIZE 16
	char buf[BUF_SIZE];
	char *envp[] = { "NAME=IO_TRAFFIC", buf, NULL, };
	int ret;

	if (unlikely(IS_ERR(blk_sec_dev)))
		return;

	memset(buf, 0, BUF_SIZE);
	snprintf(buf, BUF_SIZE, "LEVEL=%d", traffic->level);
	ret = kobject_uevent_env(&blk_sec_dev->kobj, KOBJ_CHANGE, envp);
	if (ret)
		pr_err("%s: couldn't send uevent (%d)", __func__, ret);
}

#define MB(x) ((x) / 1024 / 1024)

static void update_traffic(struct work_struct *work)
{
	struct traffic *traffic = UPDATE_WORK_TO_TRAFFIC(work);
	struct traffic old = *traffic;
	unsigned int duration_ms;
	u64 amount;
	int tput;
	int delay = 0;

	traffic->transferred_bytes = get_transferred_bytes();
	traffic->timestamp = jiffies_to_msecs(jiffies);

	duration_ms = traffic->timestamp - old.timestamp;
	amount = traffic->transferred_bytes - old.transferred_bytes;
	tput = MB(amount) * 1000 / duration_ms;
	traffic->level = tp2level(tput);

	if (!!traffic->level == !!old.level)
		return;

	if (traffic->level == 0) /* level !0 -> 0 */
		delay = msecs_to_jiffies(TL0_UEVENT_DELAY_MS);

	cancel_delayed_work_sync(&traffic->notify_work);
	schedule_delayed_work(&traffic->notify_work, delay);
}

static void send_uevent(struct work_struct *work)
{
	struct traffic *traffic = NOTIFY_WORK_TO_TRAFFIC(work);
	notify_traffic_level(traffic);
}

void blk_sec_stat_traffic_update(struct request *rq, unsigned int data_size)
{
	unsigned int duration_ms;
	u64 amount;

	if (req_op(rq) > REQ_OP_WRITE)
		return;

	this_cpu_add(transferred_bytes, data_size);

	duration_ms = jiffies_to_msecs(jiffies) - traffic.timestamp;
	amount = get_transferred_bytes() - traffic.transferred_bytes;

	if ((duration_ms < interval_ms) && (amount < interval_bytes))
		return;

	traffic.gd = rq->rq_disk;
	schedule_work(&traffic.update_work);
}

static void init_traffic(struct traffic *traffic)
{
	traffic->transferred_bytes = 0;
	traffic->level = 0;
	traffic->timestamp = jiffies_to_msecs(jiffies);
	traffic->gd = NULL;
	INIT_WORK(&traffic->update_work, update_traffic);
	INIT_DELAYED_WORK(&traffic->notify_work, send_uevent);
}

static void allow_cpu_lpm(bool enable)
{
	if (enable)
		cpu_latency_qos_update_request(&cpu_pm_req, PM_QOS_DEFAULT_VALUE);
	else
		cpu_latency_qos_update_request(&cpu_pm_req, 0);
}

static ssize_t transferred_bytes_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%llu\n", get_transferred_bytes());
}

static ssize_t cpufreq_min_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	struct freq_qos_request *req;
	int len = 0;
	int i;

	for_each_possible_cpu(i) {
		req = &per_cpu(cpufreq_req, i);
		if (IS_ERR_OR_NULL(req->qos))
			continue;
		len += scnprintf(buf + len, PAGE_SIZE - len, "%d: %d, %d, %d\n",
				i,
				req->qos->min_freq.target_value,
				req->qos->min_freq.default_value,
				req->qos->min_freq.no_constraint_value);
	}

	return len;
}

static ssize_t cpufreq_min_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	struct freq_qos_request *req;
	struct cpufreq_policy *policy;
	s32 cpufreq_min;
	int i;
	int ret;

	ret = kstrtoint(buf, 10, &cpufreq_min);
	if (ret)
		return ret;

	for_each_possible_cpu(i) {
		req = &per_cpu(cpufreq_req, i);
		if (IS_ERR_OR_NULL(req->qos)) {
			policy = cpufreq_cpu_get(i);
			if (!policy)
				continue;

			freq_qos_add_request(&policy->constraints,
					req, FREQ_QOS_MIN, cpufreq_min);
			cpufreq_cpu_put(policy);
		}
		freq_qos_update_request(req, cpufreq_min);
	}

	return count;
}

static ssize_t cpu_lpm_enabled_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	if (IS_ERR_OR_NULL(cpu_pm_req.qos))
		return 0;

	return scnprintf(buf, PAGE_SIZE, "%d: %d, %d, %d\n",
				!!cpu_pm_req.qos->target_value,
				cpu_pm_req.qos->target_value,
				cpu_pm_req.qos->default_value,
				cpu_pm_req.qos->no_constraint_value);
}

static ssize_t cpu_lpm_enabled_store(struct kobject *kobj,
		struct kobj_attribute *attr, const char *buf, size_t count)
{
	int enable;
	int ret;

	ret = kstrtoint(buf, 10, &enable);
	if (ret)
		return ret;

	allow_cpu_lpm(!!enable);

	return count;
}

static struct kobj_attribute transferred_bytes_attr =
	__ATTR(transferred_bytes, 0444, transferred_bytes_show, NULL);
static struct kobj_attribute cpufreq_min_attr =
	__ATTR(cpufreq_min, 0600, cpufreq_min_show, cpufreq_min_store);
static struct kobj_attribute cpu_lpm_enable_attr =
	__ATTR(cpu_lpm_enable, 0600, cpu_lpm_enabled_show, cpu_lpm_enabled_store);

static const struct attribute *blk_sec_stat_traffic_attrs[] = {
	&transferred_bytes_attr.attr,
	&cpufreq_min_attr.attr,
	&cpu_lpm_enable_attr.attr,
	NULL,
};

int blk_sec_stat_traffic_init(struct kobject *kobj)
{
	if (!kobj)
		return -EINVAL;

	init_traffic(&traffic);

	cpu_latency_qos_add_request(&cpu_pm_req, PM_QOS_DEFAULT_VALUE);

	return sysfs_create_files(kobj, blk_sec_stat_traffic_attrs);
}

void blk_sec_stat_traffic_exit(struct kobject *kobj)
{
	if (!kobj)
		return;

	allow_cpu_lpm(true);
	cpu_latency_qos_remove_request(&cpu_pm_req);
	cancel_delayed_work_sync(&traffic.notify_work);

	sysfs_remove_files(kobj, blk_sec_stat_traffic_attrs);
}
