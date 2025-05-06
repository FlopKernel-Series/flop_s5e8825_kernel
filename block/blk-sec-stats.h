/* SPDX-License-Identifier: GPL-2.0 */
#ifndef BLK_SEC_STATS_H
#define BLK_SEC_STATS_H

struct pio_node {
	struct list_head list;

	pid_t tgid;
	char name[TASK_COMM_LEN];
	u64 start_time;

	atomic_t kb[REQ_OP_DISCARD + 1];

	atomic_t ref_count;
	struct pio_node *h_next; /* next pio_node for hash */
};

#if IS_ENABLED(CONFIG_BLK_SEC_STATS)
extern void blk_sec_stat_account_init(struct request_queue *q);
extern void blk_sec_stat_account_exit(struct elevator_queue *eq);
extern void blk_sec_stat_account_io_prepare(struct request *rq,
		void *ptr_pio);
extern void blk_sec_stat_account_io_complete(struct request *rq,
		unsigned int data_size, void *pio);
extern void blk_sec_stat_account_io_finish(struct request *rq,
		void *ptr_pio);
extern bool is_internal_disk(struct gendisk *gd);
#else
static inline void blk_sec_stat_account_init(struct request_queue *q)
{
}

static inline void blk_sec_stat_account_exit(struct elevator_queue *eq)
{
}

static inline void blk_sec_stat_account_io_prepare(struct request *rq,
		void *ptr_pio)
{
}

static inline void blk_sec_stat_account_io_complete(struct request *rq,
		unsigned int data_size, void *pio)
{
}

static inline void blk_sec_stat_account_io_finish(struct request *rq,
		void *ptr_pio)
{
}

static inline bool is_internal_disk(struct gendisk *gd);
{
	return false;
}
#endif

#endif // BLK_SEC_STATS_H
