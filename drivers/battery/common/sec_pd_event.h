/*
 * sec_pd_event.h
 * Samsung Mobile SEC PD Event Header
 *
 * Copyright (C) 2025 Samsung Electronics, Inc.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#ifndef __SEC_PD_EVENT_H
#define __SEC_PD_EVENT_H __FILE__

#define DEFAULT_PD_WAIT_DELAY		700
#define DEFAULT_RESET_EVENT			0

union retry_event {
	unsigned long long value;

	struct {
		unsigned	type : 8;
	} base;

	struct {
		unsigned	type : 8,
					pdo : 8;
	} pdo;

	struct {
		unsigned	type : 8,
					pdo : 8,
					volt : 16,
					curr : 16;
	} pps;

	struct {
		unsigned	type : 8,
					volt : 16;
	} volt;
};

enum {
	PD_RETRY_EVENT_NONE = 0,
	PD_RETRY_EVENT_PDO,
	PD_RETRY_EVENT_PPS,
	PD_RETRY_EVENT_VOLT
};

#define SET_PDO_EVENT(pdo_num)	{ .pdo = { .type = PD_RETRY_EVENT_PDO, .pdo = pdo_num } }
#define SET_PPS_EVENT(pps_num, pps_volt, pps_curr)	{ .pps = { .type = PD_RETRY_EVENT_PPS, .pdo = pps_num, .volt =  pps_volt, .curr = pps_curr } }
#define SET_VOLT_EVENT(voltage)	{ .volt = { .type = PD_RETRY_EVENT_VOLT, .volt = voltage } }

struct pd_event_op {
	void (*ps_ready)(void);
	void (*pd_reset)(void);
};

int init_pd_event(unsigned int);
int change_pd_event_op(const struct pd_event_op *);
void lock_pd_event(void);
void unlock_pd_event(void);
void inc_pd_event(void);
void dec_pd_event(void);
void reset_pd_event(unsigned int);
int wait_pd_event(int, int);
void wakeup_pd_event(void);
void start_retry_work(int);
void cancel_retry_work(void);

#endif /* __SEC_PD_EVENT_H */
