/* sound/soc/samsung/abox/abox_synchronized_ipc.c
 *
 * ALSA SoC Audio Layer - Samsung Abox synchronized IPC driver
 *
 * Copyright (c) 2020 Samsung Electronics Co. Ltd.
  *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define DEBUG

#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_platform.h>
#include <linux/kernel.h>
#include <linux/version.h>

/*#include <sound/abox_synchronized_ipc.h>*/
#include "abox.h"
#include "abox_dma.h"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0))
#include <linux/time64.h>
#else
#include <linux/time.h>
#endif

#define AWINIC_IPC_VERSION	"v0.0.8"
#define AW_DSP_CMD_WRITE		(0x1001)
#define AW_DSP_CMD_READ			(0x1002)
#define AW_DSP_CMD_RECOVERY_DATA	(0x1003)
#define AW_DSP_CMD_SET_PARAMS		(0x1004)

#define AW_DSP_RES_WRITE_SUCCESS	(0x2001)
#define AW_DSP_RES_READ_SUCCESS		(0x2002)
#define AW_DSP_RES_WRITE_FAILED		(0x2003)
#define AW_DSP_RES_READ_FAILED		(0x2004)

#define AW_DSP_RES_BACKUP_SUCCESS	(0x2005)
#define AW_DSP_RES_BACKUP_FAILED	(0x2006)
#define AW_DSP_RES_RECOVERY_SUCCESS	(0x2007)
#define AW_DSP_RES_RECOVERY_FAILED	(0x2008)

#define AW_DSP_SUCCESS			(0)
#define AW_DSP_FAILED			(-1)

#define VENDORID_AWINIC_ID		0x0825

#define TIMEOUT_MS			100
#define DEBUG_SYNCHRONIZED_IPC
#define AW_MAX_SIZE			(740)  /*data_max_size = (int[188] - sizeof(aw_msg_hdr)) */
#define ALGO_PACKET_MEM_ADDR		(0x80667C00) /* reserved 12KB for spkamp_params.bin and 1KB for last data from 0x80664800 */
#define ALGO_INIT_MEM_ADDR			(0x80667C64)
#define AW_IPC_HEADER_SIZE		(12) /* 3 * sizeof(int) */

#ifdef DEBUG_SYNCHRONIZED_IPC
#define ipc_dbg(format, args...) \
pr_info("[Awinic][SYNC_IPC] %s: " format "\n", __func__, ## args)
#else
#define ipc_dbg(format, args...)
#endif /* DEBUG_SYNCHRONIZED_IPC */

#define ipc_err(format, args...) \
pr_err("[Awinic][SYNC_IPC] %s: " format "\n", __func__, ## args)

#define REALTIME_GEAR_ID		0x7007

static DECLARE_WAIT_QUEUE_HEAD(wq_read);
static DECLARE_WAIT_QUEUE_HEAD(wq_write);

typedef struct state_data {
	int32_t interval_time;
	int64_t tma_pre;
	int64_t tcm_pre;
} state_data_t;

struct  abox_algo_packet {
	long long last_time;
	long long cur_time;
	state_data_t data[2];
};

struct abox_synchronized_data {
	struct device *dev_abox;
	struct abox_data *abox_data;
};

static struct abox_synchronized_data *data;

static struct abox_algo_packet aw_algo_packet;

char *aw_drv_read_buf = NULL;
char *aw_drv_write_buf = NULL;
char *shmem_buf = NULL;
char *shmem_init_buf = NULL;
struct ERAP_RAW_PARAM rd_tem_buf;

int abox_ipc_read_error = 0;
int abox_ipc_write_error = 0;
bool abox_ipc_read_avail = false;
bool abox_ipc_write_avail = false;
bool g_aw882xx_ipc_loaded = false;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0))
static void aw_get_kernel_rtc_time(long long *rtc_ms)
{
	struct timespec64 tv;

	ktime_get_real_ts64(&tv);

	*rtc_ms = tv.tv_sec * 1000 + tv.tv_nsec / 1000000;
}
#else
static void aw_get_kernel_rtc_time(long long *rtc_ms)
{
	struct timeval tv;

	do_gettimeofday(&tv);

	*rtc_ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;
}
#endif

static void aw_clear_rtc_time(long long *rtc_ms)
{
	*rtc_ms = 0;
}

static void aw_get_interval_time(int *interval_time)
{
	long long time = 0;

	/* first playback or backup res failed*/
	if (aw_algo_packet.last_time == 0) {
		*interval_time = -1;
		return;
	}

	time = aw_algo_packet.cur_time - aw_algo_packet.last_time;
	if ((time > 0x7FFFFFFF) || (time <= 0)) {
		*interval_time = -1;
		return;
	}

	*interval_time = time / 100;
}

void aw882xx_abox_backup_algo_packet(unsigned int length)
{
	ipc_dbg("enter");

	if (length > sizeof(aw_algo_packet.data)) {
		ipc_err("algo_packet length overflow:%d", length);
		return;
	}

	aw_get_kernel_rtc_time(&aw_algo_packet.last_time);
	ipc_dbg("cur time:%lld, last time:%lld",
		aw_algo_packet.cur_time, aw_algo_packet.last_time);

	shmem_buf = (char *)abox_iova_to_virt(data->dev_abox, ALGO_PACKET_MEM_ADDR);

	memcpy(&aw_algo_packet.data[0], shmem_buf, length);
}

int aw882xx_abox_recover_algo_packet(char *init_data, int init_size)
{
	ABOX_IPC_MSG msg;
	int ret = 0;
	struct IPC_ERAP_MSG *erap_msg = &msg.msg.erap;

	ipc_dbg("enter");

	abox_request_cpu_gear_ext(data->dev_abox, REALTIME_GEAR_ID, 4, "aw882xx");

	aw_get_kernel_rtc_time(&aw_algo_packet.cur_time);
	aw_get_interval_time(&aw_algo_packet.data[0].interval_time);
	aw_get_interval_time(&aw_algo_packet.data[1].interval_time);
	ipc_dbg("cur time:%lld, last time:%lld, interval:%d",
		aw_algo_packet.cur_time, aw_algo_packet.last_time, aw_algo_packet.data[0].interval_time);

	msg.ipcid = IPC_ERAP;
	erap_msg->msgtype = REALTIME_EXTRA;
	erap_msg->param.raw.params[0] = VENDORID_AWINIC_ID;
	erap_msg->param.raw.params[1] = AW_DSP_CMD_RECOVERY_DATA;
	erap_msg->param.raw.params[2] = sizeof(aw_algo_packet.data);

	shmem_buf = (char *)abox_iova_to_virt(data->dev_abox, ALGO_PACKET_MEM_ADDR);
	memcpy(shmem_buf, (char *)&aw_algo_packet.data[0], sizeof(aw_algo_packet.data));
	shmem_init_buf = (char *)abox_iova_to_virt(data->dev_abox, ALGO_INIT_MEM_ADDR);
	if (init_data != NULL && shmem_init_buf != NULL)
		memcpy(shmem_init_buf, init_data, init_size);
	else
		ipc_err("init_data or shmem_init_buf is NULL");

	abox_ipc_write_avail = false;
	abox_ipc_write_error = AW_DSP_SUCCESS;

	ret = abox_request_ipc(data->dev_abox, IPC_ERAP,
				&msg, sizeof(msg), 0, 0);
	if (ret < 0) {
		ipc_err("abox_start_ipc_transaction is failed, error=%d", ret);
		ret = -1;
		goto release;
	}

	ret = wait_event_timeout(wq_write,
		abox_ipc_write_avail, msecs_to_jiffies(TIMEOUT_MS));
	if (!ret) {
		ipc_err("wait_event timeout %d", ret);
		ret = -1;
		goto release;
	} else {
		ret = 0;
	}

	if (abox_ipc_write_error == AW_DSP_FAILED) {
		ipc_err("abox_ipc_write_error = %d", abox_ipc_write_error);
		ret = -1;
		goto release;
	}

release:
	abox_request_cpu_gear_ext(data->dev_abox, REALTIME_GEAR_ID, 12, "aw882xx");

	return ret;
}
EXPORT_SYMBOL_GPL(aw882xx_abox_recover_algo_packet);

int aw882xx_abox_sync_algo_param_load(void)
{
	ABOX_IPC_MSG msg;
	int ret = 0;
	struct IPC_ERAP_MSG *erap_msg = &msg.msg.erap;

	ipc_dbg("enter");

	abox_request_cpu_gear_ext(data->dev_abox, REALTIME_GEAR_ID, 4, "aw882xx");

	msg.ipcid = IPC_ERAP;
	erap_msg->msgtype = REALTIME_EXTRA;
	erap_msg->param.raw.params[0] = VENDORID_AWINIC_ID;
	erap_msg->param.raw.params[1] = AW_DSP_CMD_SET_PARAMS;
	erap_msg->param.raw.params[2] = 0;

	abox_ipc_write_avail = false;
	abox_ipc_write_error = AW_DSP_SUCCESS;

	ret = abox_request_ipc(data->dev_abox, IPC_ERAP,
				&msg, sizeof(msg), 0, 0);
	if (ret < 0) {
		ipc_err("abox_start_ipc_transaction is failed, error=%d", ret);
		ret = -1;
		goto release;
	}

	ret = wait_event_timeout(wq_write,
		abox_ipc_write_avail, msecs_to_jiffies(TIMEOUT_MS));
	if (!ret) {
		ipc_err("wait_event timeout %d", ret);
		ret = -1;
		goto release;
	} else {
		ret = 0;
	}

	if (abox_ipc_write_error == AW_DSP_FAILED) {
		ipc_err("aw882xx_abox_sync_algo_param_load = %d", abox_ipc_write_error);
		ret = -1;
		goto release;
	}

release:
	abox_request_cpu_gear_ext(data->dev_abox, REALTIME_GEAR_ID, 12, "aw882xx");

	return ret;
}
EXPORT_SYMBOL_GPL(aw882xx_abox_sync_algo_param_load);

static void aw882xx_set_abox_ipc_loaded(bool enable)
{
	g_aw882xx_ipc_loaded = enable;
}

int aw882xx_abox_read(char *buf, int length)
{
	ABOX_IPC_MSG msg;
	int ret = 0;
	struct IPC_ERAP_MSG *erap_msg = &msg.msg.erap;

	if (!g_aw882xx_ipc_loaded) {
		ipc_err("aw882xx ipc not loaded");
		return -1;
	}

	if (length > AW_MAX_SIZE) {
		ipc_err("length overflow= %d", length);
		return -1;
	}

	ipc_dbg("length = %d", length);

	abox_request_cpu_gear_ext(data->dev_abox, REALTIME_GEAR_ID, 4, "aw882xx");

	aw_drv_read_buf = (char *)buf;

	msg.ipcid = IPC_ERAP;
	erap_msg->msgtype = REALTIME_EXTRA;
	erap_msg->param.raw.params[0] = VENDORID_AWINIC_ID;
	erap_msg->param.raw.params[1] = AW_DSP_CMD_READ;
	erap_msg->param.raw.params[2] = length;

	abox_ipc_read_avail = false;
	abox_ipc_read_error = AW_DSP_SUCCESS;

	ret = abox_request_ipc(data->dev_abox, IPC_ERAP,
				&msg, sizeof(msg), 0, 0);
	if (ret < 0) {
		ipc_err("abox_start_ipc_transaction is failed, error=%d", ret);
		ret = -1;
		goto release;
	}

	ret = wait_event_timeout(wq_read,
		abox_ipc_read_avail, msecs_to_jiffies(TIMEOUT_MS));
	if (!ret) {
		ipc_err("wait_event timeout %d", ret);
		ret = -1;
		goto release;
	} else {
		ret = 0;
	}

	if (abox_ipc_read_error == AW_DSP_FAILED) {
		ipc_err("error = %d", abox_ipc_read_error);
		ret = -1;
		goto release;
	}

	memcpy((void *)aw_drv_read_buf, (void *)&rd_tem_buf.params[0], min(length, AW_MAX_SIZE));
	ipc_dbg("read length = %d done", min(length, AW_MAX_SIZE));

release:
	abox_request_cpu_gear_ext(data->dev_abox, REALTIME_GEAR_ID, 12, "aw882xx");

	return ret;
}
EXPORT_SYMBOL_GPL(aw882xx_abox_read);

int aw882xx_abox_write(const char *buf, int length)
{
	ABOX_IPC_MSG msg;
	int ret = 0;
	struct IPC_ERAP_MSG *erap_msg = &msg.msg.erap;

	if (!g_aw882xx_ipc_loaded) {
		ipc_err("aw882xx ipc not loaded");
		return -1;
	}

	if (length > AW_MAX_SIZE) {
		ipc_err("length overflow= %d", length);
		return -1;
	}

	ipc_dbg("length = %d", length);

	abox_request_cpu_gear_ext(data->dev_abox, REALTIME_GEAR_ID, 4, "aw882xx");

	aw_drv_write_buf = (char *)buf;

	msg.ipcid = IPC_ERAP;
	erap_msg->msgtype = REALTIME_EXTRA;
	erap_msg->param.raw.params[0] = VENDORID_AWINIC_ID;
	erap_msg->param.raw.params[1] = AW_DSP_CMD_WRITE;
	erap_msg->param.raw.params[2] = length;

	memcpy(&erap_msg->param.raw.params[3], aw_drv_write_buf, length);
	abox_ipc_write_avail = false;
	abox_ipc_write_error = AW_DSP_SUCCESS;

	ret = abox_request_ipc(data->dev_abox, IPC_ERAP,
				&msg, sizeof(msg), 0, 0);
	if (ret < 0) {
		ipc_err("abox_start_ipc_transaction is failed, error=%d", ret);
		ret = -1;
		goto release;
	}

	ret = wait_event_timeout(wq_write,
		abox_ipc_write_avail, msecs_to_jiffies(TIMEOUT_MS));
	if (!ret) {
		ipc_err("wait_event timeout %d", ret);
		ret = -1;
		goto release;
	} else {
		ret = 0;
	}

	if (abox_ipc_write_error == AW_DSP_FAILED) {
		ipc_err("abox_ipc_write_error = %d", abox_ipc_write_error);
		ret = -1;
		goto release;
	}

release:
	abox_request_cpu_gear_ext(data->dev_abox, REALTIME_GEAR_ID, 12, "aw882xx");

	return ret;
}
EXPORT_SYMBOL_GPL(aw882xx_abox_write);

static irqreturn_t abox_synchronized_ipc_aw_handler(int irq,
					void *dev_id, ABOX_IPC_MSG *msg)
{
	struct IPC_ERAP_MSG *erap_msg = &msg->msg.erap;
	unsigned int vendor_id = erap_msg->param.raw.params[0];
	unsigned int res_id = erap_msg->param.raw.params[1];
	int size = erap_msg->param.raw.params[2];
	irqreturn_t ret = IRQ_HANDLED;

	if (vendor_id != VENDORID_AWINIC_ID) {
		ipc_err("vendor_id unsupported, %d", vendor_id);
		return IRQ_NONE;
	}

	switch (irq) {
	case IPC_ERAP:
	switch (erap_msg->msgtype) {
	case REALTIME_EXTRA:
	switch(res_id) {
		case AW_DSP_RES_READ_SUCCESS:
			size -=AW_IPC_HEADER_SIZE;
			memcpy((void *)&rd_tem_buf.params[0], (void *)&erap_msg->param.raw.params[3],
				min(size, AW_MAX_SIZE));
			abox_ipc_read_avail = true;
			abox_ipc_read_error = AW_DSP_SUCCESS;
			ipc_dbg("AW_DSP_CMD_READ DONE size= %d", size);
			if (waitqueue_active(&wq_read))
				wake_up(&wq_read);
		break;
		case AW_DSP_RES_READ_FAILED:
			abox_ipc_read_avail = true;
			abox_ipc_read_error = AW_DSP_FAILED;
			ipc_err("aw_dsp_read() AW_DSP_RES_READ_FAILED");
			if (waitqueue_active(&wq_read))
				wake_up(&wq_read);
		break;
		case AW_DSP_RES_BACKUP_SUCCESS:
			size -=AW_IPC_HEADER_SIZE;
			aw882xx_abox_backup_algo_packet(size);
			ipc_dbg("AW_DSP_RES_BACKUP_SUCCESS, size:%d", size);
		break;
		case AW_DSP_RES_BACKUP_FAILED:
			ipc_err("AW_DSP_RES_BACKUP_FAILED");
			aw_clear_rtc_time(&aw_algo_packet.last_time);
		break;
		case AW_DSP_RES_WRITE_SUCCESS:
			abox_ipc_write_avail = true;
			abox_ipc_write_error = AW_DSP_SUCCESS;
			ipc_dbg("AW_DSP_CMD_WRITE done");
			if (waitqueue_active(&wq_write))
				wake_up(&wq_write);
		break;
		case AW_DSP_RES_WRITE_FAILED:
			abox_ipc_write_avail = true;
			abox_ipc_write_error = AW_DSP_FAILED;
			ipc_err("aw_dsp_write() AW_DSP_RES_WRITE_FAILED");
			if (waitqueue_active(&wq_write))
				wake_up(&wq_write);
		break;
		case AW_DSP_RES_RECOVERY_SUCCESS:
			abox_ipc_write_avail = true;
			abox_ipc_write_error = AW_DSP_SUCCESS;
			ipc_dbg("AW_DSP_RES_RECOVERY_SUCCESS");
			if (waitqueue_active(&wq_write))
				wake_up(&wq_write);
		break;
		case AW_DSP_RES_RECOVERY_FAILED:
			abox_ipc_write_avail = true;
			abox_ipc_write_error = AW_DSP_FAILED;
			ipc_err("AW_DSP_RES_RECOVERY_FAILED");
			if (waitqueue_active(&wq_write))
				wake_up(&wq_write);
		break;
		default:
			ipc_err("unknown response type, RES_ID = 0x%x, size=%d",
				res_id, size);
			ret = IRQ_NONE;
		break;
	}
	break;
	default:
		ipc_err("unknown message type, msgtype = %d",
						erap_msg->msgtype);
		ret = IRQ_NONE;
	}
	break;
	default:
		ipc_err("unknown command, irq = %d", irq);
		ret = IRQ_NONE;
	break;
	}

	return ret;
}

static int samsung_abox_synchronized_ipc_aw_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *np = dev->of_node;
	struct device_node *np_abox;
	struct platform_device *pdev_abox;

	ipc_dbg(" ");

	data = devm_kzalloc(dev, sizeof(*data), GFP_KERNEL);
	if (!data) {
		dev_err(dev, "[SYNC_IPC] Failed to allocate memory\n");
		return -ENOMEM;
	}
	platform_set_drvdata(pdev, data);

	np_abox = of_parse_phandle(np, "abox", 0);
	if (!np_abox) {
		dev_err(dev, "[SYNC_IPC] Failed to get abox device node\n");
		return -EPROBE_DEFER;
	}

	pdev_abox = of_find_device_by_node(np_abox);
	data->dev_abox = &pdev_abox->dev;
	if (!data->dev_abox) {
		dev_err(dev, "[SYNC_IPC] Failed to get abox platform device\n");
		return -EPROBE_DEFER;
	}
	data->abox_data = platform_get_drvdata(pdev_abox);

	abox_register_ipc_handler(data->dev_abox, IPC_ERAP,
				abox_synchronized_ipc_aw_handler, pdev);

	aw882xx_set_abox_ipc_loaded(true);

	return 0;
}

static int samsung_abox_synchronized_ipc_aw_remove(struct platform_device *pdev)
{
	ipc_dbg(" ");

	return 0;
}

static const struct of_device_id samsung_abox_synchronized_ipc_aw_match[] = {
	{
		.compatible = "samsung,abox-synchronized-ipc-aw",
	},
	{},
};
MODULE_DEVICE_TABLE(of, samsung_abox_synchronized_ipc_aw_match);

static struct platform_driver samsung_abox_synchronized_ipc_aw_driver = {
	.probe  = samsung_abox_synchronized_ipc_aw_probe,
	.remove = samsung_abox_synchronized_ipc_aw_remove,
	.driver = {
		.name = "samsung-abox-synchronized-ipc-aw",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(samsung_abox_synchronized_ipc_aw_match),
	},
};
module_platform_driver(samsung_abox_synchronized_ipc_aw_driver);

/* Module information */
MODULE_DESCRIPTION("Samsung ASoC A-Box Synchronized IPC AWINIC Driver");
MODULE_ALIAS("platform:samsung-abox-synchronized-ipc-aw");
MODULE_LICENSE("GPL");
