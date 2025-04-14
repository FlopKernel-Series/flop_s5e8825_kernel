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

#ifndef __SHUB_MINI_DUMP_H_
#define __SHUB_MINI_DUMP_H_

#define MINI_DUMP_LENGTH	1024
#define LAST_CMD_SIZE		20

struct shub_cmd_t {
	u8 cmd;
	u8 type;
	u8 subcmd;
	u64 timestamp;
};

void push_last_cmd(u8 cmd, u8 type, u8 subcmd, u64 timestamp);
void shub_write_mini_dump(char *dump, int dump_size);
void initialize_shub_mini_dump(void);
void remove_shub_mini_dump(void);
char *get_shub_mini_dump(void);

#endif
