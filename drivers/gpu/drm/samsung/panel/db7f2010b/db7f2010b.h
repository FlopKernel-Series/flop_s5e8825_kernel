/*
 * linux/drivers/video/fbdev/exynos/panel/db7f2010b/db7f2010b.h
 *
 * Header file for DB7F2010B Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __DB7F2010B_H__
#define __DB7F2010B_H__

#include <linux/types.h>
#include <linux/kernel.h>
#ifdef CONFIG_USDM_PANEL_DDI_FLASH
#include "../panel_poc.h"
#endif
#include "../panel_drv.h"
#include "../panel.h"
#include "../maptbl.h"
#include "oled_function.h"

/*
 * OFFSET ==> OFS means N-param - 1
 * <example>
 * XXX 1st param => DB7F2010B_XXX_OFS (0)
 * XXX 2nd param => DB7F2010B_XXX_OFS (1)
 * XXX 36th param => DB7F2010B_XXX_OFS (35)
 */

#define DB7F2010B_ADDR_OFS	(0)
#define DB7F2010B_ADDR_LEN	(1)
#define DB7F2010B_DATA_OFS	(DB7F2010B_ADDR_OFS + DB7F2010B_ADDR_LEN)

#define DB7F2010B_DATE_REG				0xA1
#define DB7F2010B_DATE_OFS				0		// 4
#define DB7F2010B_DATE_LEN				7

#define DB7F2010B_COORDINATE_REG				0xA1
#define DB7F2010B_COORDINATE_OFS				0
#define DB7F2010B_COORDINATE_LEN				4

#define DB7F2010B_OCTA_ID_0_REG				0xA1
#define DB7F2010B_OCTA_ID_0_OFS				0
#define DB7F2010B_OCTA_ID_0_LEN				4

#define DB7F2010B_ID_REG				0x04
#define DB7F2010B_ID_OFS				0
#define DB7F2010B_ID_LEN				(PANEL_ID_LEN)

#define DB7F2010B_CODE_REG			0xD6
#define DB7F2010B_CODE_OFS			0
#define DB7F2010B_CODE_LEN			5

#define DB7F2010B_OCTA_ID_1_REG			0xFF
#define DB7F2010B_OCTA_ID_1_OFS			0
#define DB7F2010B_OCTA_ID_1_LEN			16

#define DB7F2010B_OCTA_ID_LEN			(DB7F2010B_OCTA_ID_0_LEN + DB7F2010B_OCTA_ID_1_LEN)

/* for panel dump */
#define DB7F2010B_RDDPM_REG			0x0A
#define DB7F2010B_RDDPM_OFS			0
#define DB7F2010B_RDDPM_LEN			(PANEL_RDDPM_LEN)

#define DB7F2010B_RDDSM_REG			0x0E
#define DB7F2010B_RDDSM_OFS			0
#define DB7F2010B_RDDSM_LEN			(PANEL_RDDSM_LEN)

#define DB7F2010B_ERR_REG				0xE6
#define DB7F2010B_ERR_OFS				0
#define DB7F2010B_ERR_LEN				5

#define DB7F2010B_ERR_FG_REG			0xEE
#define DB7F2010B_ERR_FG_OFS			0
#define DB7F2010B_ERR_FG_LEN			1

#define DB7F2010B_DSI_ERR_REG			0x05
#define DB7F2010B_DSI_ERR_OFS			0
#define DB7F2010B_DSI_ERR_LEN			1

#define DB7F2010B_SELF_DIAG_REG			0x0F
#define DB7F2010B_SELF_DIAG_OFS			0
#define DB7F2010B_SELF_DIAG_LEN			1

#define DB7F2010B_SELF_MASK_CRC_REG		0x7F
#define DB7F2010B_SELF_MASK_CRC_OFS		5
#define DB7F2010B_SELF_MASK_CRC_LEN		4

#define DB7F2010B_SELF_MASK_CHECKSUM_REG		0x7F
#define DB7F2010B_SELF_MASK_CHECKSUM_OFS	1
#define DB7F2010B_SELF_MASK_CHECKSUM_LEN		4


#ifdef CONFIG_USDM_DDI_CMDLOG
#define DB7F2010B_CMDLOG_REG			0x9C
#define DB7F2010B_CMDLOG_OFS			0
#define DB7F2010B_CMDLOG_LEN			0x80
#endif

#ifdef CONFIG_USDM_PANEL_FREQ_HOP
#define DB7F2010B_MAX_MIPI_FREQ			3
#define DB7F2010B_DEFAULT_MIPI_FREQ		0
#endif

#ifdef CONFIG_USDM_FACTORY_CCD_TEST
#define DB7F2010B_CCD_STATE_REG				0xE7
#define DB7F2010B_CCD_STATE_OFS				0
#define DB7F2010B_CCD_STATE_LEN				1
#define DB7F2010B_CCD_STATE_PASS_LIST_LEN				2
#endif

#ifdef CONFIG_USDM_FACTORY_DSC_CRC_TEST
#define DB7F2010B_DECODER_TEST_REG			0xC7
#define DB7F2010B_DECODER_TEST_OFS			0x17
#define DB7F2010B_DECODER_TEST_LEN			2
#endif

enum db7f2010b_function {
	DB7F2010B_MAPTBL_INIT_GAMMA_MODE2_BRT,
	DB7F2010B_MAPTBL_GETIDX_HBM_TRANSITION,
	DB7F2010B_MAPTBL_GETIDX_NORMAL_HBM_TRANSITION,
	DB7F2010B_MAPTBL_GETIDX_SMOOTH_TRANSITION,
	DB7F2010B_MAPTBL_GETIDX_ACL_OPR,
	DB7F2010B_MAPTBL_GETIDX_HBM_ONOFF,
	DB7F2010B_MAPTBL_GETIDX_ACL_ONOFF,
	DB7F2010B_MAPTBL_GETIDX_ACL_DIM_ONOFF,
	DB7F2010B_MAPTBL_INIT_LPM_BRT,
	DB7F2010B_MAPTBL_GETIDX_LPM_BRT,
	DB7F2010B_MAPTBL_GETIDX_IRC_MODE,
#if defined(CONFIG_USDM_FACTORY) && defined(CONFIG_USDM_FACTORY_FAST_DISCHARGE)
	DB7F2010B_MAPTBL_GETIDX_FAST_DISCHARGE,
#endif
	DB7F2010B_MAPTBL_GETIDX_VRR_FPS,
	DB7F2010B_MAPTBL_GETIDX_VRR,
	MAX_DB7F2010B_FUNCTION,
};

extern struct pnobj_func db7f2010b_function_table[MAX_DB7F2010B_FUNCTION];

#undef DDI_FUNC
#define DDI_FUNC(_index) (db7f2010b_function_table[_index])

#ifdef CONFIG_USDM_PANEL_FREQ_HOP
enum {
	DB7F2010B_OSC_DEFAULT,
	MAX_DB7F2010B_OSC,
};
#endif


enum {
	GAMMA_MAPTBL,
	GAMMA_MODE2_MAPTBL,
	AOR_MAPTBL,
	TSET_MAPTBL,
	ACL_ONOFF_MAPTBL,
	ACL_FRAME_AVG_MAPTBL,
	ACL_START_POINT_MAPTBL,
	ACL_DIM_SPEED_MAPTBL,
	ACL_OPR_MAPTBL,
	FPS_MAPTBL_1,
	FPS_MAPTBL_2,
	FPS_MAPTBL_3,
	FPS_MAPTBL_4,
	DIMMING_SPEED,
	LPM_NIT_MAPTBL,
	LPM_MODE_MAPTBL,
	LPM_EXIT_NIT_MAPTBL,
	HBM_ONOFF_MAPTBL,
#if defined(CONFIG_USDM_FACTORY) && defined(CONFIG_USDM_FACTORY_FAST_DISCHARGE)
	FAST_DISCHARGE_MAPTBL,
#endif
	SET_FFC_MAPTBL,
	SET_FFC_MAPTBL_2,
	IRC_MODE_MAPTBL,
	MAX_MAPTBL,
};

enum {
#ifdef CONFIG_USDM_PANEL_COPR
	READ_COPR_SPI,
	READ_COPR_DSI,
#endif
	READ_ID,
	READ_COORDINATE,
	READ_DATE,
	READ_CODE,
	READ_ELVSS,
	READ_ELVSS_TEMP_0,
	READ_ELVSS_TEMP_1,
	READ_MTP,
	READ_OCTA_ID_0,
	READ_OCTA_ID_1,
	READ_IRC,
	READ_RDDPM,
	READ_RDDSM,
	READ_ERR,
	READ_ERR_FG,
	READ_DSI_ERR,
	READ_SELF_DIAG,
	READ_SELF_MASK_CRC,
	READ_SELF_MASK_CHECKSUM,
#ifdef CONFIG_USDM_POC_SPI
	READ_POC_SPI_READ,
	READ_POC_SPI_STATUS1,
	READ_POC_SPI_STATUS2,
#endif
#ifdef CONFIG_USDM_PANEL_POC_FLASH
	READ_POC_MCA_CHKSUM,
#endif
#ifdef CONFIG_USDM_DDI_CMDLOG
	READ_CMDLOG,
#endif
#ifdef CONFIG_USDM_FACTORY_CCD_TEST
	READ_CCD_STATE,
#endif
#ifdef CONFIG_SUPPORT_GRAYSPOT_TEST
	READ_GRAYSPOT_CAL,
#endif
	MAX_READTBL,
};

enum {
#ifdef CONFIG_USDM_PANEL_COPR
	RES_COPR_SPI,
	RES_COPR_DSI,
#endif
	RES_ID,
	RES_COORDINATE,
	RES_CODE,
	RES_DATE,
	RES_ELVSS,
	RES_ELVSS_TEMP_0,
	RES_ELVSS_TEMP_1,
	RES_MTP,
	RES_OCTA_ID_0,
	RES_OCTA_ID_1,
	/* RES_CHIP_ID, */
	/* for brightness debugging */
	RES_IRC,
	RES_RDDPM,
	RES_RDDSM,
	RES_ERR,
	RES_ERR_FG,
	RES_DSI_ERR,
	RES_SELF_DIAG,
#ifdef CONFIG_USDM_DDI_CMDLOG
	RES_CMDLOG,
#endif
#ifdef CONFIG_USDM_POC_SPI
	RES_POC_SPI_READ,
	RES_POC_SPI_STATUS1,
	RES_POC_SPI_STATUS2,
#endif
#ifdef CONFIG_USDM_PANEL_POC_FLASH
	RES_POC_MCA_CHKSUM,
#endif
#ifdef CONFIG_USDM_FACTORY_CCD_TEST
	RES_CCD_STATE,
	RES_CCD_CHKSUM_PASS_LIST,
#endif
#ifdef CONFIG_SUPPORT_GRAYSPOT_TEST
	RES_GRAYSPOT_CAL,
#endif
	RES_SELF_MASK_CRC,
	RES_SELF_MASK_CHECKSUM,
	MAX_RESTBL
};

enum {
	HLPM_ON_LOW,
	HLPM_ON,
	MAX_HLPM_ON
};


enum {
	DB7F2010B_VRR_FPS_120,
	DB7F2010B_VRR_FPS_60,
	MAX_DB7F2010B_VRR_FPS,
};


enum {
	DB7F2010B_SMOOTH_DIMMING_OFF,
	DB7F2010B_SMOOTH_DIMMING_ON,
	MAX_DB7F2010B_SMOOTH_DIMMING,
};

enum {
	DB7F2010B_NORMAL_HBM_TRANSITION_NORMAL_TO_NORMAL,
	DB7F2010B_NORMAL_HBM_TRANSITION_NORMAL_TO_HBM,
	DB7F2010B_NORMAL_HBM_TRANSITION_HBM_TO_NORMAL,
	DB7F2010B_NORMAL_HBM_TRANSITION_HBM_TO_HBM,
	MAX_DB7F2010B_NORMAL_HBM_TRANSITION,
};

#define DB7F2010B_VRR_FPS_PROPERTY ("db7f2010b_vrr_fps")
#define DB7F2010B_NORMAL_HBM_TRANSITION_PROPERTY ("db7f2010b_normal_hbm_transition")

static u8 DB7F2010B_ID[DB7F2010B_ID_LEN];
static u8 DB7F2010B_COORDINATE[DB7F2010B_COORDINATE_LEN];
static u8 DB7F2010B_DATE[DB7F2010B_DATE_LEN];
static u8 DB7F2010B_CODE[DB7F2010B_CODE_LEN];
static u8 DB7F2010B_OCTA_ID_0[DB7F2010B_OCTA_ID_0_LEN];
static u8 DB7F2010B_OCTA_ID_1[DB7F2010B_OCTA_ID_1_LEN];
/* for brightness debugging */
static u8 DB7F2010B_RDDPM[DB7F2010B_RDDPM_LEN];
static u8 DB7F2010B_RDDSM[DB7F2010B_RDDSM_LEN];
static u8 DB7F2010B_ERR[DB7F2010B_ERR_LEN];
static u8 DB7F2010B_ERR_FG[DB7F2010B_ERR_FG_LEN];
static u8 DB7F2010B_DSI_ERR[DB7F2010B_DSI_ERR_LEN];
static u8 DB7F2010B_SELF_DIAG[DB7F2010B_SELF_DIAG_LEN];
static u8 DB7F2010B_SELF_MASK_CRC[DB7F2010B_SELF_MASK_CRC_LEN];
static u8 DB7F2010B_SELF_MASK_CHECKSUM[DB7F2010B_SELF_MASK_CHECKSUM_LEN];
#ifdef CONFIG_USDM_FACTORY_CCD_TEST
static u8 DB7F2010B_CCD_STATE[DB7F2010B_CCD_STATE_LEN];
static u8 DB7F2010B_CCD_CHKSUM_PASS_LIST[DB7F2010B_CCD_STATE_PASS_LIST_LEN] = { 0x20, 0x00, };
#endif

static struct rdinfo db7f2010b_rditbl[MAX_READTBL] = {
	[READ_ID] = RDINFO_INIT(id, DSI_PKT_TYPE_RD, DB7F2010B_ID_REG, DB7F2010B_ID_OFS, DB7F2010B_ID_LEN),
	[READ_COORDINATE] = RDINFO_INIT(coordinate, DSI_PKT_TYPE_RD, DB7F2010B_COORDINATE_REG, DB7F2010B_COORDINATE_OFS, DB7F2010B_COORDINATE_LEN),
	[READ_CODE] = RDINFO_INIT(code, DSI_PKT_TYPE_RD, DB7F2010B_CODE_REG, DB7F2010B_CODE_OFS, DB7F2010B_CODE_LEN),
	[READ_DATE] = RDINFO_INIT(date, DSI_PKT_TYPE_RD, DB7F2010B_DATE_REG, DB7F2010B_DATE_OFS, DB7F2010B_DATE_LEN),
	[READ_OCTA_ID_0] = RDINFO_INIT(octa_id_0, DSI_PKT_TYPE_RD, DB7F2010B_OCTA_ID_0_REG, DB7F2010B_OCTA_ID_0_OFS, DB7F2010B_OCTA_ID_0_LEN),
	[READ_OCTA_ID_1] = RDINFO_INIT(octa_id_1, DSI_PKT_TYPE_RD, DB7F2010B_OCTA_ID_1_REG, DB7F2010B_OCTA_ID_1_OFS, DB7F2010B_OCTA_ID_1_LEN),
	[READ_RDDPM] = RDINFO_INIT(rddpm, DSI_PKT_TYPE_RD, DB7F2010B_RDDPM_REG, DB7F2010B_RDDPM_OFS, DB7F2010B_RDDPM_LEN),
	[READ_RDDSM] = RDINFO_INIT(rddsm, DSI_PKT_TYPE_RD, DB7F2010B_RDDSM_REG, DB7F2010B_RDDSM_OFS, DB7F2010B_RDDSM_LEN),
	[READ_ERR] = RDINFO_INIT(err, DSI_PKT_TYPE_RD, DB7F2010B_ERR_REG, DB7F2010B_ERR_OFS, DB7F2010B_ERR_LEN),
	[READ_ERR_FG] = RDINFO_INIT(err_fg, DSI_PKT_TYPE_RD, DB7F2010B_ERR_FG_REG, DB7F2010B_ERR_FG_OFS, DB7F2010B_ERR_FG_LEN),
	[READ_DSI_ERR] = RDINFO_INIT(dsi_err, DSI_PKT_TYPE_RD, DB7F2010B_DSI_ERR_REG, DB7F2010B_DSI_ERR_OFS, DB7F2010B_DSI_ERR_LEN),
	[READ_SELF_DIAG] = RDINFO_INIT(self_diag, DSI_PKT_TYPE_RD, DB7F2010B_SELF_DIAG_REG, DB7F2010B_SELF_DIAG_OFS, DB7F2010B_SELF_DIAG_LEN),
#ifdef CONFIG_USDM_FACTORY_CCD_TEST
	[READ_CCD_STATE] = RDINFO_INIT(ccd_state, DSI_PKT_TYPE_RD, DB7F2010B_CCD_STATE_REG, DB7F2010B_CCD_STATE_OFS, DB7F2010B_CCD_STATE_LEN),
#endif
	[READ_SELF_MASK_CRC] = RDINFO_INIT(self_mask_crc, DSI_PKT_TYPE_RD, DB7F2010B_SELF_MASK_CRC_REG, DB7F2010B_SELF_MASK_CRC_OFS, DB7F2010B_SELF_MASK_CRC_LEN),
	[READ_SELF_MASK_CHECKSUM] = RDINFO_INIT(self_mask_checksum, DSI_PKT_TYPE_RD, DB7F2010B_SELF_MASK_CHECKSUM_REG, DB7F2010B_SELF_MASK_CHECKSUM_OFS, DB7F2010B_SELF_MASK_CHECKSUM_LEN),
};

static DEFINE_RESUI(id, &db7f2010b_rditbl[READ_ID], 0);
static DEFINE_RESUI(coordinate, &db7f2010b_rditbl[READ_COORDINATE], 0);
static DEFINE_RESUI(code, &db7f2010b_rditbl[READ_CODE], 0);
static DEFINE_RESUI(date, &db7f2010b_rditbl[READ_DATE], 0);
static DEFINE_RESUI(elvss, &db7f2010b_rditbl[READ_ELVSS], 0);
static DEFINE_RESUI(mtp, &db7f2010b_rditbl[READ_MTP], 0);
static DEFINE_RESUI(octa_id_0, &db7f2010b_rditbl[READ_OCTA_ID_0], 0);
static DEFINE_RESUI(octa_id_1, &db7f2010b_rditbl[READ_OCTA_ID_1], 0);

/* for brightness debugging */
static DEFINE_RESUI(rddpm, &db7f2010b_rditbl[READ_RDDPM], 0);
static DEFINE_RESUI(rddsm, &db7f2010b_rditbl[READ_RDDSM], 0);
static DEFINE_RESUI(err, &db7f2010b_rditbl[READ_ERR], 0);
static DEFINE_RESUI(err_fg, &db7f2010b_rditbl[READ_ERR_FG], 0);
static DEFINE_RESUI(dsi_err, &db7f2010b_rditbl[READ_DSI_ERR], 0);
static DEFINE_RESUI(self_diag, &db7f2010b_rditbl[READ_SELF_DIAG], 0);
#ifdef CONFIG_USDM_FACTORY_CCD_TEST
static DEFINE_RESUI(ccd_state, &db7f2010b_rditbl[READ_CCD_STATE], 0);
#endif
static DEFINE_RESUI(self_mask_crc, &db7f2010b_rditbl[READ_SELF_MASK_CRC], 0);
static DEFINE_RESUI(self_mask_checksum, &db7f2010b_rditbl[READ_SELF_MASK_CHECKSUM], 0);

static struct resinfo db7f2010b_restbl[MAX_RESTBL] = {
	[RES_ID] = RESINFO_INIT(id, DB7F2010B_ID, RESUI(id)),
	[RES_COORDINATE] = RESINFO_INIT(coordinate, DB7F2010B_COORDINATE, RESUI(coordinate)),
	[RES_CODE] = RESINFO_INIT(code, DB7F2010B_CODE, RESUI(code)),
	[RES_DATE] = RESINFO_INIT(date, DB7F2010B_DATE, RESUI(date)),
	[RES_OCTA_ID_0] = RESINFO_INIT(octa_id_0, DB7F2010B_OCTA_ID_0, RESUI(octa_id_0)),
	[RES_OCTA_ID_1] = RESINFO_INIT(octa_id_1, DB7F2010B_OCTA_ID_1, RESUI(octa_id_1)),
	[RES_RDDPM] = RESINFO_INIT(rddpm, DB7F2010B_RDDPM, RESUI(rddpm)),
	[RES_RDDSM] = RESINFO_INIT(rddsm, DB7F2010B_RDDSM, RESUI(rddsm)),
	[RES_ERR] = RESINFO_INIT(err, DB7F2010B_ERR, RESUI(err)),
	[RES_ERR_FG] = RESINFO_INIT(err_fg, DB7F2010B_ERR_FG, RESUI(err_fg)),
	[RES_DSI_ERR] = RESINFO_INIT(dsi_err, DB7F2010B_DSI_ERR, RESUI(dsi_err)),
	[RES_SELF_DIAG] = RESINFO_INIT(self_diag, DB7F2010B_SELF_DIAG, RESUI(self_diag)),
#ifdef CONFIG_USDM_FACTORY_CCD_TEST
	[RES_CCD_STATE] = RESINFO_INIT(ccd_state, DB7F2010B_CCD_STATE, RESUI(ccd_state)),
	[RES_CCD_CHKSUM_PASS_LIST] = RESINFO_IMMUTABLE_INIT(ccd_chksum_pass_list, DB7F2010B_CCD_CHKSUM_PASS_LIST),
#endif
	[RES_SELF_MASK_CRC] = RESINFO_INIT(self_mask_crc, DB7F2010B_SELF_MASK_CRC, RESUI(self_mask_crc)),
	[RES_SELF_MASK_CHECKSUM] = RESINFO_INIT(self_mask_checksum, DB7F2010B_SELF_MASK_CHECKSUM, RESUI(self_mask_checksum)),
};

enum {
	DUMP_RDDPM = 0,
	DUMP_RDDPM_SLEEP_IN,
	DUMP_RDDSM,
	DUMP_ERR,
	DUMP_ERR_FG,
	DUMP_DSI_ERR,
	DUMP_SELF_DIAG,
	DUMP_SELF_MASK_CRC,
	DUMP_SELF_MASK_CHECKSUM,
#ifdef CONFIG_USDM_DDI_CMDLOG
	DUMP_CMDLOG,
#endif
	MAX_DUMP_SIZE,
};

/* TODO: check dump log */
static struct dump_expect rddpm_after_display_on_expects[] = {
	{ .offset = 0, .mask = 0x80, .value = 0x80, .msg = "Booster Mode : OFF(NG)" },
	{ .offset = 0, .mask = 0x40, .value = 0x00, .msg = "Idle Mode : ON(NG)" },
	{ .offset = 0, .mask = 0x10, .value = 0x10, .msg = "Sleep Mode : IN(NG)" },
	{ .offset = 0, .mask = 0x08, .value = 0x08, .msg = "Normal Mode : SLEEP(NG)" },
	{ .offset = 0, .mask = 0x04, .value = 0x04, .msg = "Display Mode : OFF(NG)" },
};

static struct dump_expect rddpm_before_sleep_in_expects[] = {
	{ .offset = 0, .mask = 0x80, .value = 0x80, .msg = "Booster Mode : OFF(NG)" },
	{ .offset = 0, .mask = 0x40, .value = 0x00, .msg = "Idle Mode : ON(NG)" },
	{ .offset = 0, .mask = 0x10, .value = 0x10, .msg = "Sleep Mode : IN(NG)" },
	{ .offset = 0, .mask = 0x08, .value = 0x08, .msg = "Normal Mode : SLEEP(NG)" },
	{ .offset = 0, .mask = 0x04, .value = 0x00, .msg = "Display Mode : ON(NG)" },
};

static struct dump_expect rddsm_expects[] = {
	{ .offset = 0, .mask = 0xFF, .value = 0x80, .msg = "TE Mode : OFF(NG)" },
};

static struct dump_expect dsi_err_expects[] = {
	{ .offset = 0, .mask = 0x80, .value = 0x00, .msg = "DSI Protocol Violation" },
	{ .offset = 0, .mask = 0x40, .value = 0x00, .msg = "Data P Lane Contention Detetion" },
	{ .offset = 0, .mask = 0x20, .value = 0x00, .msg = "Invalid Transmission Length" },
	{ .offset = 0, .mask = 0x10, .value = 0x00, .msg = "DSI VC ID Invalid" },
	{ .offset = 0, .mask = 0x08, .value = 0x00, .msg = "DSI Data Type Not Recognized" },
	{ .offset = 0, .mask = 0x04, .value = 0x00, .msg = "Checksum Error" },
	{ .offset = 0, .mask = 0x02, .value = 0x00, .msg = "ECC Error, multi-bit (detected, not corrected)" },
	{ .offset = 0, .mask = 0x01, .value = 0x00, .msg = "ECC Error, single-bit (detected and corrected)" },
	{ .offset = 1, .mask = 0x80, .value = 0x00, .msg = "Data Lane Contention Detection" },
	{ .offset = 1, .mask = 0x40, .value = 0x00, .msg = "False Control Error" },
	{ .offset = 1, .mask = 0x20, .value = 0x00, .msg = "HS RX Timeout" },
	{ .offset = 1, .mask = 0x10, .value = 0x00, .msg = "Low-Power Transmit Sync Error" },
	{ .offset = 1, .mask = 0x08, .value = 0x00, .msg = "Escape Mode Entry Command Error" },
	{ .offset = 1, .mask = 0x04, .value = 0x00, .msg = "EoT Sync Error" },
	{ .offset = 1, .mask = 0x02, .value = 0x00, .msg = "SoT Sync Error" },
	{ .offset = 1, .mask = 0x01, .value = 0x00, .msg = "SoT Error" },
	{ .offset = 2, .mask = 0xFF, .value = 0x00, .msg = "CRC Error Count" },
	{ .offset = 3, .mask = 0xFF, .value = 0x00, .msg = "CRC Error Count" },
	{ .offset = 4, .mask = 0xFF, .value = 0x00, .msg = "CRC Error Count" },
};

static struct dump_expect err_fg_expects[] = {
	{ .offset = 0, .mask = 0x40, .value = 0x00, .msg = "VLIN1 Error" },
	{ .offset = 0, .mask = 0x04, .value = 0x00, .msg = "VLOUT3 Error" },
	{ .offset = 0, .mask = 0x08, .value = 0x00, .msg = "ELVDD Error" },
};

static struct dump_expect dsie_cnt_expects[] = {
	{ .offset = 0, .mask = 0xFF, .value = 0x00, .msg = "DSI Error Count" },
};

static struct dump_expect self_diag_expects[] = {
	{ .offset = 0, .mask = 0x40, .value = 0x40, .msg = "Panel Boosting Error" },
};

#ifdef CONFIG_USDM_DDI_CMDLOG
static struct dump_expect cmdlog_expects[] = {
};
#endif

static struct dump_expect self_mask_crc_expects[] = {
	{ .offset = 0, .mask = 0xFF, .value = 0x00, .msg = "Self Mask CRC[0] Error(NG)" },
	{ .offset = 1, .mask = 0xFF, .value = 0xED, .msg = "Self Mask CRC[1] Error(NG)" },
	{ .offset = 2, .mask = 0xFF, .value = 0xE0, .msg = "Self Mask CRC[2] Error(NG)" },
	{ .offset = 3, .mask = 0xFF, .value = 0x0D, .msg = "Self Mask CRC[3] Error(NG)" },
};

static struct dump_expect self_mask_checksum_expects[] = {
};

static struct dumpinfo db7f2010b_dmptbl[MAX_DUMP_SIZE] = {
	[DUMP_RDDPM] = DUMPINFO_INIT_V2(rddpm, &db7f2010b_restbl[RES_RDDPM], &OLED_FUNC(OLED_DUMP_SHOW_RDDPM), rddpm_after_display_on_expects),
	[DUMP_RDDPM_SLEEP_IN] = DUMPINFO_INIT_V2(rddpm_sleep_in, &db7f2010b_restbl[RES_RDDPM], &OLED_FUNC(OLED_DUMP_SHOW_RDDPM_BEFORE_SLEEP_IN), rddpm_before_sleep_in_expects),
	[DUMP_RDDSM] = DUMPINFO_INIT_V2(rddsm, &db7f2010b_restbl[RES_RDDSM], &OLED_FUNC(OLED_DUMP_SHOW_RDDSM), rddsm_expects),
	[DUMP_ERR] = DUMPINFO_INIT_V2(err, &db7f2010b_restbl[RES_ERR], &OLED_FUNC(OLED_DUMP_SHOW_ERR), dsi_err_expects),
	[DUMP_ERR_FG] = DUMPINFO_INIT_V2(err_fg, &db7f2010b_restbl[RES_ERR_FG], &OLED_FUNC(OLED_DUMP_SHOW_ERR_FG), err_fg_expects),
	[DUMP_DSI_ERR] = DUMPINFO_INIT_V2(dsi_err, &db7f2010b_restbl[RES_DSI_ERR], &OLED_FUNC(OLED_DUMP_SHOW_DSI_ERR), dsie_cnt_expects),
	[DUMP_SELF_DIAG] = DUMPINFO_INIT_V2(self_diag, &db7f2010b_restbl[RES_SELF_DIAG], &OLED_FUNC(OLED_DUMP_SHOW_SELF_DIAG), self_diag_expects),
#ifdef CONFIG_USDM_DDI_CMDLOG
	[DUMP_CMDLOG] = DUMPINFO_INIT_V2(cmdlog, &db7f2010b_restbl[RES_CMDLOG], &OLED_FUNC(OLED_DUMP_SHOW_CMDLOG), cmdlog_expects),
#endif
	[DUMP_SELF_MASK_CRC] = DUMPINFO_INIT_V2(self_mask_crc, &db7f2010b_restbl[RES_SELF_MASK_CRC], &OLED_FUNC(OLED_DUMP_SHOW_EXPECTS), self_mask_crc_expects),
	//[DUMP_SELF_MASK_CHECKSUM] = DUMPINFO_INIT_V2(self_mask_checksum, &db7f2010b_restbl[RES_SELF_MASK_CHECKSUM], &OLED_FUNC(OLED_DUMP_SHOW_SELF_MASK_CHECKSUM), self_mask_checksum_expects),
};

/* Variable Refresh Rate */
enum {
	DB7F2010B_VRR_MODE_NS,
	DB7F2010B_VRR_MODE_HS,
	MAX_DB7F2010B_VRR_MODE,
};

enum {
	DB7F2010B_VRR_120HS,
	DB7F2010B_VRR_60HS,
	MAX_DB7F2010B_VRR,
};

enum {
	DB7F2010B_A26X_OSC_CLK_96_5,
	DB7F2010B_A26X_OSC_CLK_94_5,
	MAX_DB7F2010B_A26X_OSC_CLK,
};

enum {
	DB7F2010B_A26X_OSC_UN_CHANGE,
	DB7F2010B_A26X_OSC_CHANGE_DONE,
	MAX_DB7F2010B_A26X_OSC_CHANGE,
};

enum {
	DB7F2010B_RESOL_1080x2340
};

enum {
	DB7F2010B_RESOL_1080x2400
};

enum {
	DB7F2010B_VRR_KEY_REFRESH_RATE,
	DB7F2010B_VRR_KEY_REFRESH_MODE,
	DB7F2010B_VRR_KEY_TE_SW_SKIP_COUNT,
	DB7F2010B_VRR_KEY_TE_HW_SKIP_COUNT,
	MAX_DB7F2010B_VRR_KEY,
};

static u32 DB7F2010B_VRR_FPS[MAX_DB7F2010B_VRR][MAX_DB7F2010B_VRR_KEY] = {
	[DB7F2010B_VRR_120HS] = { 120, VRR_HS_MODE, 0, 0 },
	[DB7F2010B_VRR_60HS] = { 60, VRR_HS_MODE, 0, 0 },
};

enum {
	DB7F2010B_ACL_DIM_OFF,
	DB7F2010B_ACL_DIM_ON,
	MAX_DB7F2010B_ACL_DIM,
};

/* use according to adaptive_control sysfs */
enum {
	DB7F2010B_ACL_OPR_0,
	DB7F2010B_ACL_OPR_1,
	DB7F2010B_ACL_OPR_2,
	DB7F2010B_ACL_OPR_3,
	MAX_DB7F2010B_ACL_OPR
};

//#define DB7F2010B_TSET_PROPERTY ("db7f2010b_tset")
#define DB7F2010B_ACL_DIM_PROPERTY ("db7f2010b_acl_dim")
#define DB7F2010B_ACL_OPR_PROPERTY ("db7f2010b_acl_opr")
#define DB7F2010B_VRR_PROPERTY ("db7f2010b_vrr")
#define DB7F2010B_VRR_MODE_PROPERTY ("db7f2010b_vrr_mode")

int db7f2010b_get_octa_id(struct panel_device *panel, void *buf);
int db7f2010b_get_cell_id(struct panel_device *panel, void *buf);
int db7f2010b_get_manufacture_code(struct panel_device *panel, void *buf);
int db7f2010b_get_manufacture_date(struct panel_device *panel, void *buf);
int db7f2010b_init(struct common_panel_info *cpi);

#endif /* __DB7F2010B_H__ */
