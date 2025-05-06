// SPDX-License-Identifier: GPL-2.0
/*
 *  Samsung specific module in block layer
 *
 *  Copyright (C) 2021 Manjong Lee <mj0123.lee@samsung.com>
 *  Copyright (C) 2021 Junho Kim <junho89.kim@samsung.com>
 *  Copyright (C) 2021 Changheun Lee <nanich.lee@samsung.com>
 *  Copyright (C) 2021 Seunghwan Hyun <seunghwan.hyun@samsung.com>
 *  Copyright (C) 2021 Tran Xuan Nam <nam.tx2@samsung.com>
 */

#include <linux/sysfs.h>
#include <linux/module.h>
#include <linux/blk_types.h>
#include <linux/blkdev.h>
#include <linux/genhd.h>
#include <linux/part_stat.h>
#include <linux/sec_class.h>

#include "blk-sec.h"

struct disk_info {
	/* fields related with target device itself */
	struct gendisk *gd;
	struct request_queue *queue;
};

struct device *blk_sec_dev;
EXPORT_SYMBOL(blk_sec_dev);

struct workqueue_struct *blk_sec_common_wq;
EXPORT_SYMBOL(blk_sec_common_wq);

static struct disk_info internal_disk;
static unsigned int internal_min_size_mb = 10 * 1024; /* 10GB */

#define SECTORS2MB(x) ((x) / 2 / 1024)

#define SCSI_DISK0_MAJOR 8
#define MMC_BLOCK_MAJOR 179
#define MAJOR8_DEV_NUM 16	/* maximum number of minor devices in scsi disk0 */
#define SCSI_MINORS 16		/* first minor number of scsi disk0 */
#define MMC_TARGET_DEV 16	/* number of mmc devices set of target (maximum 256) */
#define MMC_MINORS 8		/* first minor number of mmc disk */

static bool is_internal_bdev(struct block_device *dev)
{
	int size_mb;

	if (bdev_is_partition(dev))
		return false;

	if (dev->bd_disk->flags & GENHD_FL_REMOVABLE)
		return false;

	size_mb = SECTORS2MB(get_capacity(dev->bd_disk));
	if (size_mb >= internal_min_size_mb)
		return true;

	return false;
}

static struct gendisk *find_internal_disk(void)
{
	struct block_device *bdev;
	struct gendisk *gd = NULL;
	int idx;
	dev_t devno = MKDEV(0, 0);

	for (idx = 0; idx < MAJOR8_DEV_NUM; idx++) {
		devno = MKDEV(SCSI_DISK0_MAJOR, SCSI_MINORS * idx);
		bdev = blkdev_get_by_dev(devno, FMODE_READ, NULL);
		if (IS_ERR(bdev))
			continue;

		if (bdev->bd_disk && is_internal_bdev(bdev))
			gd = bdev->bd_disk;

		blkdev_put(bdev, FMODE_READ);

		if (gd)
			return gd;
	}

	for (idx = 0; idx < MMC_TARGET_DEV; idx++) {
		devno = MKDEV(MMC_BLOCK_MAJOR, MMC_MINORS * idx);
		bdev = blkdev_get_by_dev(devno, FMODE_READ, NULL);
		if (IS_ERR(bdev))
			continue;

		if (bdev->bd_disk && is_internal_bdev(bdev))
			gd = bdev->bd_disk;

		blkdev_put(bdev, FMODE_READ);

		if (gd)
			return gd;
	}

	return NULL;
}

static inline int init_internal_disk_info(void)
{
	if (!internal_disk.gd) {
		internal_disk.gd = find_internal_disk();
		if (unlikely(!internal_disk.gd)) {
			pr_err("%s: can't find internal disk\n", __func__);
			return -ENODEV;
		}
		internal_disk.queue = internal_disk.gd->queue;
	}

	return 0;
}

static inline void clear_internal_disk_info(void)
{
	internal_disk.gd = NULL;
	internal_disk.queue = NULL;
}

struct gendisk *blk_sec_internal_disk(void)
{
	if (unlikely(!internal_disk.gd))
		init_internal_disk_info();

	return internal_disk.gd;
}
EXPORT_SYMBOL(blk_sec_internal_disk);

static int blk_sec_uevent(struct device *dev, struct kobj_uevent_env *env)
{
	return add_uevent_var(env, "DEVNAME=%s", dev->kobj.name);
}

static struct device_type blk_sec_type = {
	.uevent = blk_sec_uevent,
};

static int __init blk_sec_common_init(void)
{
	int retval;

#if IS_ENABLED(CONFIG_DRV_SAMSUNG)
	blk_sec_dev = sec_device_create(NULL, "blk_sec");
	if (unlikely(IS_ERR(blk_sec_dev))) {
		pr_err("%s: Failed to create blk-sec device\n", __func__);
		return PTR_ERR(blk_sec_dev);
	}

	blk_sec_dev->type = &blk_sec_type;
#endif

	blk_sec_common_wq = create_freezable_workqueue("blk_sec_common");

	retval = init_internal_disk_info();
	if (retval) {
		clear_internal_disk_info();
		pr_err("%s: Can't find internal disk info!", __func__);
	}

	return 0;
}

static void __exit blk_sec_common_exit(void)
{
	clear_internal_disk_info();
}

module_init(blk_sec_common_init);
module_exit(blk_sec_common_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Changheun Lee <nanich.lee@samsung.com>");
MODULE_DESCRIPTION("Samsung specific module in block layer");
MODULE_VERSION("1.0");
