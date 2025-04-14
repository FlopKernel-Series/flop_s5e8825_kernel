/*
 *  Copyright (C) 2018, Samsung Electronics Co. Ltd. All Rights Reserved.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#include "../comm/shub_cmd.h"
#include "../sensor/light.h"
#include "../sensormanager/shub_sensor.h"
#include "../utility/shub_utility.h"

#include "shub_mini_dump.h"

char mini_dump[MINI_DUMP_LENGTH];
struct shub_cmd_t last_cmd[LAST_CMD_SIZE];
int index_last_cmd;

char *get_shub_mini_dump(void)
{
	return mini_dump;
}

void shub_write_mini_dump(char *dump, int dump_size)
{
	int i = index_last_cmd;
	int len = dump_size;
	int cnt = LAST_CMD_SIZE;

	memset(mini_dump, 0, sizeof(mini_dump));
	memcpy(mini_dump, dump, dump_size);

	while (cnt--) {
		i = i % LAST_CMD_SIZE;

		if (i == index_last_cmd - 1) {
			len += snprintf(&mini_dump[len], MINI_DUMP_LENGTH - len,
				 "/%d %d %d %lld",
				 last_cmd[i].cmd, last_cmd[i].type, last_cmd[i].subcmd, last_cmd[i].timestamp/1000);
		} else {
			len += snprintf(&mini_dump[len], MINI_DUMP_LENGTH - len,
				 "/%d %d %d", last_cmd[i].cmd, last_cmd[i].type, last_cmd[i].subcmd);
		}

		i++;
	}

	shub_infof("size : %d, mini dump : %s", dump_size, mini_dump);
}

bool skip_push_cmd(u8 cmd, u8 type, u8 subcmd)
{
	if (((cmd == CMD_SETVALUE && type == SENSOR_TYPE_LIGHT) &&
	    (subcmd == LIGHT_BRIGHTNESS || subcmd == LIGHT_SUBCMD_PANEL_INFORMATION)) ||
	    (cmd == CMD_SETVALUE && type == TYPE_HUB && subcmd == TIME_SYNC))
		return true;

	return false;
}

void push_last_cmd(u8 cmd, u8 type, u8 subcmd, u64 timestamp)
{
	if (skip_push_cmd(cmd, type, subcmd))
		return;

	index_last_cmd = index_last_cmd % LAST_CMD_SIZE;

	last_cmd[index_last_cmd].cmd = cmd;
	last_cmd[index_last_cmd].type = type;
	last_cmd[index_last_cmd].subcmd = subcmd;
	last_cmd[index_last_cmd].timestamp = timestamp;

	index_last_cmd++;
}

void initialize_shub_mini_dump(void)
{
	shub_infof();

	index_last_cmd = 0;
	memset(mini_dump, 0, sizeof(mini_dump));
}

void remove_shub_mini_dump(void)
{
	shub_infof();
}
