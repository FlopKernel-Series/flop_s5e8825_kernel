/*
 * linux/drivers/video/fbdev/exynos/panel/db7f2010b/db7f2010b_a26x_aod_panel.h
 *
 * Header file for AOD Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __DB7F2010B_A26X_AOD_PANEL_H__
#define __DB7F2010B_A26X_AOD_PANEL_H__

#include "oled_common_aod.h"
#include "db7f2010b_a26x_self_mask_img.h"
#include "db7f2010b_aod.h"

/* DEPRECATED: Please check "self_mask_crc_expects" */
#define DB7F2010B_A26XSELF_MASK_VALID_CRC_1 (0x35)
#define DB7F2010B_A26XSELF_MASK_VALID_CRC_2 (0xF6)

static u8 db7f2010b_a26x_self_mask_crc[] = {
	DB7F2010B_A26XSELF_MASK_VALID_CRC_1,
	DB7F2010B_A26XSELF_MASK_VALID_CRC_2,
};

static u8 DB7F2010B_AOD_KEY1_ENABLE[] = { 0x9F, 0xA5, 0xA5 };
static u8 DB7F2010B_AOD_KEY2_ENABLE[] = { 0xF0, 0x5A, 0x5A };
static u8 DB7F2010B_AOD_KEY3_ENABLE[] = { 0xFC, 0x5A, 0x5A };
static u8 DB7F2010B_AOD_KEY1_DISABLE[] = { 0x9F, 0x5A, 0x5A };
static u8 DB7F2010B_AOD_KEY2_DISABLE[] = { 0xF0, 0xA5, 0xA5 };
static u8 DB7F2010B_AOD_KEY3_DISABLE[] = { 0xFC, 0xA5, 0xA5 };


static DEFINE_STATIC_PACKET(db7f2010b_aod_l1_key_enable, DSI_PKT_TYPE_WR, DB7F2010B_AOD_KEY1_ENABLE, 0);
static DEFINE_STATIC_PACKET(db7f2010b_aod_l1_key_disable, DSI_PKT_TYPE_WR, DB7F2010B_AOD_KEY1_DISABLE, 0);

static DEFINE_STATIC_PACKET(db7f2010b_aod_l2_key_enable, DSI_PKT_TYPE_WR, DB7F2010B_AOD_KEY2_ENABLE, 0);
static DEFINE_STATIC_PACKET(db7f2010b_aod_l2_key_disable, DSI_PKT_TYPE_WR, DB7F2010B_AOD_KEY2_DISABLE, 0);

static DEFINE_STATIC_PACKET(db7f2010b_aod_l3_key_enable, DSI_PKT_TYPE_WR, DB7F2010B_AOD_KEY3_ENABLE, 0);
static DEFINE_STATIC_PACKET(db7f2010b_aod_l3_key_disable, DSI_PKT_TYPE_WR, DB7F2010B_AOD_KEY3_DISABLE, 0);

static DEFINE_PANEL_MDELAY(db7f2010b_aod_self_spsram_write_delay, 1);
static DEFINE_PANEL_UDELAY(db7f2010b_aod_self_spsram_sel_delay, 1);

static DEFINE_PANEL_UDELAY(db7f2010b_aod_self_mask_checksum_1frame_delay, 16700);
static DEFINE_PANEL_UDELAY(db7f2010b_aod_self_mask_checksum_2frame_delay, 33400);
static DEFINE_PANEL_UDELAY(db7f2010b_aod_time_update_delay, 33400);


static DEFINE_PANEL_KEY(db7f2010b_aod_l1_key_enable, CMD_LEVEL_1,
	KEY_ENABLE, &PKTINFO(db7f2010b_aod_l1_key_enable));
static DEFINE_PANEL_KEY(db7f2010b_aod_l1_key_disable, CMD_LEVEL_1,
	KEY_DISABLE, &PKTINFO(db7f2010b_aod_l1_key_disable));

static DEFINE_PANEL_KEY(db7f2010b_aod_l2_key_enable, CMD_LEVEL_2,
	KEY_ENABLE, &PKTINFO(db7f2010b_aod_l2_key_enable));
static DEFINE_PANEL_KEY(db7f2010b_aod_l2_key_disable, CMD_LEVEL_2,
	KEY_DISABLE, &PKTINFO(db7f2010b_aod_l2_key_disable));

static DEFINE_PANEL_KEY(db7f2010b_aod_l3_key_enable, CMD_LEVEL_3,
	KEY_ENABLE, &PKTINFO(db7f2010b_aod_l3_key_enable));
static DEFINE_PANEL_KEY(db7f2010b_aod_l3_key_disable, CMD_LEVEL_3,
	KEY_DISABLE, &PKTINFO(db7f2010b_aod_l3_key_disable));

static struct keyinfo KEYINFO(db7f2010b_aod_l1_key_enable);
static struct keyinfo KEYINFO(db7f2010b_aod_l1_key_disable);
static struct keyinfo KEYINFO(db7f2010b_aod_l2_key_enable);
static struct keyinfo KEYINFO(db7f2010b_aod_l2_key_disable);
static struct keyinfo KEYINFO(db7f2010b_aod_l3_key_enable);
static struct keyinfo KEYINFO(db7f2010b_aod_l3_key_disable);

static struct maptbl db7f2010b_aod_maptbl[] = {
};

// --------------------- Image for self mask image ---------------------

static char DB7F2010B_AOD_RESET_SD_PATH_1[] = {
	0x75,
	0x00,
};
static DEFINE_STATIC_PACKET(db7f2010b_aod_reset_sd_path_1, DSI_PKT_TYPE_WR, DB7F2010B_AOD_RESET_SD_PATH_1, 0);

static char DB7F2010B_AOD_RESET_SD_PATH_2[] = {
	0xF2,
	0x0F,
};
static DEFINE_STATIC_PACKET(db7f2010b_aod_reset_sd_path_2, DSI_PKT_TYPE_WR, DB7F2010B_AOD_RESET_SD_PATH_2, 0);


static char DB7F2010B_AOD_SELF_MASK_SD_PATH_1[] = {
	0x75,
	0x10,
};
static DEFINE_STATIC_PACKET(db7f2010b_aod_self_mask_sd_path_1, DSI_PKT_TYPE_WR, DB7F2010B_AOD_SELF_MASK_SD_PATH_1, 0);

static char DB7F2010B_AOD_SELF_MASK_SD_PATH_2[] = {
	0xF2,
	0x03,
};
static DEFINE_STATIC_PACKET(db7f2010b_aod_self_mask_sd_path_2, DSI_PKT_TYPE_WR, DB7F2010B_AOD_SELF_MASK_SD_PATH_2, 0);

// --------------------- End of self mask image ---------------------


// --------------------- Image for self mask control ---------------------
static char DB7F2010B_AOD_FACTORY_SELF_MASK_ENA[] = {
	0x7A,
	0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x09, 0x24, 0x09, 0x25, 0x09, 0x26, 0x09, 0x27
};

static DEFINE_STATIC_PACKET(db7f2010b_aod_factory_self_mask_ctrl_ena,
		DSI_PKT_TYPE_WR, DB7F2010B_AOD_FACTORY_SELF_MASK_ENA, 0);

static char DB7F2010B_AOD_SELF_MASK_ENA[] = {
	0x7A,
	0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x01, 0x2b, 0x07, 0xF8, 0x09, 0x23
};

static DEFINE_STATIC_PACKET(db7f2010b_aod_self_mask_ctrl_ena,
		DSI_PKT_TYPE_WR, DB7F2010B_AOD_SELF_MASK_ENA, 0);

static DEFINE_RULE_BASED_COND(a26x_cond_is_factory_selfmask,
		PANEL_PROPERTY_IS_FACTORY_MODE, EQ, 1);

static void *db7f2010b_a26x_aod_self_mask_ena_cmdtbl[] = {
	&KEYINFO(db7f2010b_aod_l2_key_enable),
	&CONDINFO_IF(a26x_cond_is_factory_selfmask),
		&PKTINFO(db7f2010b_aod_factory_self_mask_ctrl_ena),
	&CONDINFO_EL(a26x_cond_is_factory_selfmask),
		&PKTINFO(db7f2010b_aod_self_mask_ctrl_ena),
	&CONDINFO_FI(a26x_cond_is_factory_selfmask),
	&KEYINFO(db7f2010b_aod_l2_key_disable),
};

static char DB7F2010B_AOD_SELF_MASK_DISABLE[] = {
	0x7A,
	0x00,
};
static DEFINE_STATIC_PACKET(db7f2010b_aod_self_mask_disable, DSI_PKT_TYPE_WR, DB7F2010B_AOD_SELF_MASK_DISABLE, 0);

static void *db7f2010b_a26x_aod_self_mask_dis_cmdtbl[] = {
	&KEYINFO(db7f2010b_aod_l2_key_enable),
	&PKTINFO(db7f2010b_aod_self_mask_disable),
	&KEYINFO(db7f2010b_aod_l2_key_disable),
};
// --------------------- End of self mask control ---------------------

// --------------------- check sum control ----------------------------
static DEFINE_STATIC_PACKET_WITH_OPTION(db7f2010b_a26x_aod_self_mask_img_pkt,
		DSI_PKT_TYPE_WR_SR, DB7F2010B_A26X_SELF_MASK_IMG, 0, PKT_OPTION_SR_ALIGN_16);

static char DB7F2010B_AOD_SELF_MASK_CRC_ON1[] = {
	0xD8,
	0x16,
};
static DEFINE_STATIC_PACKET(db7f2010b_aod_self_mask_crc_on1, DSI_PKT_TYPE_WR, DB7F2010B_AOD_SELF_MASK_CRC_ON1, 0x27);

static char DB7F2010B_AOD_SELF_MASK_DBIST_ON[] = {
	0xBF,
	0x01, 0x07, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00
};
static DEFINE_STATIC_PACKET(db7f2010b_aod_self_mask_dbist_on, DSI_PKT_TYPE_WR, DB7F2010B_AOD_SELF_MASK_DBIST_ON, 0);

static char DB7F2010B_AOD_SELF_MASK_DBIST_OFF[] = {
	0xBF, 0x00
};
static DEFINE_STATIC_PACKET(db7f2010b_aod_self_mask_dbist_off, DSI_PKT_TYPE_WR, DB7F2010B_AOD_SELF_MASK_DBIST_OFF, 0);

static char DB7F2010B_AOD_SELF_MASK_ENABLE_FOR_CHECKSUM[] = {
	0x7A,
	0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x01, 0xF4, 0x02, 0x33,
	0x09, 0x60, 0x09, 0x61, 0x00, 0x00, 0xFF, 0xFF,
	0xFF
};

static DEFINE_STATIC_PACKET(db7f2010b_aod_self_mask_for_checksum, DSI_PKT_TYPE_WR, DB7F2010B_AOD_SELF_MASK_ENABLE_FOR_CHECKSUM, 0);

static char DB7F2010B_AOD_SELF_MASK_RESTORE[] = {
	0x7A,
	0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x09, 0x60, 0x09, 0x61,
	0x09, 0x62, 0x09, 0x63, 0x00, 0x00, 0x00, 0x00,
	0x00
};
static DEFINE_STATIC_PACKET(db7f2010b_aod_self_mask_restore, DSI_PKT_TYPE_WR, DB7F2010B_AOD_SELF_MASK_RESTORE, 0);

static void *db7f2010b_a26x_aod_self_mask_crc_cmdtbl[] = {
	&KEYINFO(db7f2010b_aod_l1_key_enable),
	&KEYINFO(db7f2010b_aod_l2_key_enable),
	&KEYINFO(db7f2010b_aod_l3_key_enable),
	&PKTINFO(db7f2010b_aod_self_mask_crc_on1),
	&PKTINFO(db7f2010b_aod_self_mask_dbist_on),
	&PKTINFO(db7f2010b_aod_self_mask_disable),
	&DLYINFO(db7f2010b_aod_self_mask_checksum_1frame_delay),
	&PKTINFO(db7f2010b_aod_self_mask_sd_path_1),
	&PKTINFO(db7f2010b_aod_self_mask_sd_path_2),
	&DLYINFO(db7f2010b_aod_self_spsram_sel_delay),
	&PKTINFO(db7f2010b_a26x_aod_self_mask_img_pkt),
	&DLYINFO(db7f2010b_aod_self_spsram_write_delay),
	&PKTINFO(db7f2010b_aod_reset_sd_path_1),
	&PKTINFO(db7f2010b_aod_reset_sd_path_2),
	&DLYINFO(db7f2010b_aod_self_spsram_sel_delay),
	&PKTINFO(db7f2010b_aod_self_mask_for_checksum),
	&DLYINFO(db7f2010b_aod_self_mask_checksum_2frame_delay),
	&db7f2010b_dmptbl[DUMP_SELF_MASK_CRC],
	&PKTINFO(db7f2010b_aod_self_mask_restore),
	&PKTINFO(db7f2010b_aod_self_mask_dbist_off),
	&KEYINFO(db7f2010b_aod_l3_key_disable),
	&KEYINFO(db7f2010b_aod_l2_key_disable),
	&KEYINFO(db7f2010b_aod_l1_key_disable),
};

// --------------------- end of check sum control ----------------------------

static void *db7f2010b_a26x_aod_self_mask_img_cmdtbl[] = {
	&KEYINFO(db7f2010b_aod_l2_key_enable),
	&PKTINFO(db7f2010b_aod_self_mask_sd_path_1),
	&PKTINFO(db7f2010b_aod_self_mask_sd_path_2),

	&DLYINFO(db7f2010b_aod_self_spsram_sel_delay),
	&PKTINFO(db7f2010b_a26x_aod_self_mask_img_pkt),
	&DLYINFO(db7f2010b_aod_self_spsram_write_delay),

	&PKTINFO(db7f2010b_aod_reset_sd_path_1),
	&PKTINFO(db7f2010b_aod_reset_sd_path_2),
	&KEYINFO(db7f2010b_aod_l2_key_disable),
};

static struct seqinfo db7f2010b_a26x_aod_seqtbl[] = {
	SEQINFO_INIT(SELF_MASK_IMG_SEQ, db7f2010b_a26x_aod_self_mask_img_cmdtbl),
	SEQINFO_INIT(SELF_MASK_ENA_SEQ, db7f2010b_a26x_aod_self_mask_ena_cmdtbl),
	SEQINFO_INIT(SELF_MASK_DIS_SEQ, db7f2010b_a26x_aod_self_mask_dis_cmdtbl),
	SEQINFO_INIT(SELF_MASK_CRC_SEQ, db7f2010b_a26x_aod_self_mask_crc_cmdtbl),
};

static struct aod_tune db7f2010b_a26x_aod = {
	.name = "db7f2010b_a26x_aod",
	.nr_seqtbl = ARRAY_SIZE(db7f2010b_a26x_aod_seqtbl),
	.seqtbl = db7f2010b_a26x_aod_seqtbl,
	.nr_maptbl = ARRAY_SIZE(db7f2010b_aod_maptbl),
	.maptbl = db7f2010b_aod_maptbl,
	.self_mask_en = true,
	.self_mask_crc = db7f2010b_a26x_self_mask_crc,
	.self_mask_crc_len = ARRAY_SIZE(db7f2010b_a26x_self_mask_crc),
};
#endif
