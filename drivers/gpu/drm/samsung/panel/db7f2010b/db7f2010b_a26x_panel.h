/*
 * linux/drivers/video/fbdev/exynos/panel/db7f2010b/db7f2010b_a26x_panel.h
 *
 * Header file for DB7F2010B Dimming Driver
 *
 * Copyright (c) 2016 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __DB7F2010B_A26X_PANEL_H__
#define __DB7F2010B_A26X_PANEL_H__

#include "../panel.h"
#include "../panel_drv.h"
#include "../panel_debug.h"
#include "oled_function.h"
#include "oled_property.h"
#include "db7f2010b.h"
#include "db7f2010b_a26x.h"
#include "db7f2010b_dimming.h"
#ifdef CONFIG_USDM_MDNIE
#include "db7f2010b_a26x_panel_mdnie.h"
#endif
#include "db7f2010b_a26x_panel_dimming.h"

#ifdef CONFIG_USDM_PANEL_SELF_DISPLAY
#include "db7f2010b_a26x_aod_panel.h"
#include "../aod/aod_drv.h"
#endif

#include "db7f2010b_a26x_resol.h"

#undef __pn_name__
#define __pn_name__	a26x

#undef __PN_NAME__
#define __PN_NAME__

/* ===================================================================================== */
/* ============================= [DB7F2010B READ INFO TABLE] ============================= */
/* ===================================================================================== */
/* <READINFO TABLE> IS DEPENDENT ON LDI. IF YOU NEED, DEFINE PANEL's RESOURCE TABLE */


/* ===================================================================================== */
/* ============================= [DB7F2010B RESOURCE TABLE] ============================== */
/* ===================================================================================== */
/* <RESOURCE TABLE> IS DEPENDENT ON LDI. IF YOU NEED, DEFINE PANEL's RESOURCE TABLE */


/* ===================================================================================== */
/* ============================== [DB7F2010B MAPPING TABLE] ============================== */
/* ===================================================================================== */

static u8 a26x_brt_table[DB7F2010B_A26X_TOTAL_STEP][2] = {
	{ 0x00, 0x04 }, { 0x00, 0x06 }, { 0x00, 0x08 }, { 0x00, 0x0A }, { 0x00, 0x0D },
	{ 0x00, 0x10 }, { 0x00, 0x14 }, { 0x00, 0x17 }, { 0x00, 0x1B }, { 0x00, 0x1E },
	{ 0x00, 0x22 }, { 0x00, 0x26 }, { 0x00, 0x2A }, { 0x00, 0x2F }, { 0x00, 0x33 },
	{ 0x00, 0x37 }, { 0x00, 0x3C }, { 0x00, 0x40 }, { 0x00, 0x45 }, { 0x00, 0x4A },
	{ 0x00, 0x4F }, { 0x00, 0x54 }, { 0x00, 0x59 }, { 0x00, 0x5E }, { 0x00, 0x63 },
	{ 0x00, 0x68 }, { 0x00, 0x6D }, { 0x00, 0x72 }, { 0x00, 0x78 }, { 0x00, 0x7D },
	{ 0x00, 0x82 }, { 0x00, 0x88 }, { 0x00, 0x8E }, { 0x00, 0x93 }, { 0x00, 0x99 },
	{ 0x00, 0x9F }, { 0x00, 0xA4 }, { 0x00, 0xAA }, { 0x00, 0xB0 }, { 0x00, 0xB6 },
	{ 0x00, 0xBC }, { 0x00, 0xC2 }, { 0x00, 0xC8 }, { 0x00, 0xCE }, { 0x00, 0xD4 },
	{ 0x00, 0xDA }, { 0x00, 0xE0 }, { 0x00, 0xE7 }, { 0x00, 0xED }, { 0x00, 0xF3 },
	{ 0x00, 0xFA }, { 0x01, 0x00 }, { 0x01, 0x07 }, { 0x01, 0x0D }, { 0x01, 0x14 },
	{ 0x01, 0x1A }, { 0x01, 0x21 }, { 0x01, 0x27 }, { 0x01, 0x2E }, { 0x01, 0x35 },
	{ 0x01, 0x3B }, { 0x01, 0x42 }, { 0x01, 0x49 }, { 0x01, 0x50 }, { 0x01, 0x57 },
	{ 0x01, 0x5E }, { 0x01, 0x65 }, { 0x01, 0x6B }, { 0x01, 0x72 }, { 0x01, 0x79 },
	{ 0x01, 0x81 }, { 0x01, 0x88 }, { 0x01, 0x8F }, { 0x01, 0x96 }, { 0x01, 0x9D },
	{ 0x01, 0xA4 }, { 0x01, 0xAB }, { 0x01, 0xB3 }, { 0x01, 0xBA }, { 0x01, 0xC1 },
	{ 0x01, 0xC9 }, { 0x01, 0xD0 }, { 0x01, 0xD7 }, { 0x01, 0xDF }, { 0x01, 0xE6 },
	{ 0x01, 0xEE }, { 0x01, 0xF5 }, { 0x01, 0xFD }, { 0x02, 0x04 }, { 0x02, 0x0C },
	{ 0x02, 0x14 }, { 0x02, 0x1B }, { 0x02, 0x23 }, { 0x02, 0x2B }, { 0x02, 0x32 },
	{ 0x02, 0x3A }, { 0x02, 0x42 }, { 0x02, 0x4A }, { 0x02, 0x51 }, { 0x02, 0x59 },
	{ 0x02, 0x61 }, { 0x02, 0x69 }, { 0x02, 0x71 }, { 0x02, 0x79 }, { 0x02, 0x81 },
	{ 0x02, 0x89 }, { 0x02, 0x91 }, { 0x02, 0x99 }, { 0x02, 0xA1 }, { 0x02, 0xA9 },
	{ 0x02, 0xB1 }, { 0x02, 0xB9 }, { 0x02, 0xC1 }, { 0x02, 0xC9 }, { 0x02, 0xD1 },
	{ 0x02, 0xDA }, { 0x02, 0xE2 }, { 0x02, 0xEA }, { 0x02, 0xF2 }, { 0x02, 0xFB },
	{ 0x03, 0x03 }, { 0x03, 0x0B }, { 0x03, 0x13 }, { 0x03, 0x1C }, { 0x03, 0x24 },
	{ 0x03, 0x2D }, { 0x03, 0x35 }, { 0x03, 0x3D }, { 0x03, 0x46 }, { 0x03, 0x4E },
	{ 0x03, 0x57 }, { 0x03, 0x5F }, { 0x03, 0x68 }, { 0x03, 0x71 }, { 0x03, 0x79 },
	{ 0x03, 0x82 }, { 0x03, 0x8A }, { 0x03, 0x93 }, { 0x03, 0x9C }, { 0x03, 0xA4 },
	{ 0x03, 0xAD }, { 0x03, 0xB6 }, { 0x03, 0xBE }, { 0x03, 0xC7 }, { 0x03, 0xD0 },
	{ 0x03, 0xD9 }, { 0x03, 0xE2 }, { 0x03, 0xEA }, { 0x03, 0xF3 }, { 0x03, 0xFC },
	{ 0x04, 0x05 }, { 0x04, 0x0E }, { 0x04, 0x17 }, { 0x04, 0x20 }, { 0x04, 0x29 },
	{ 0x04, 0x32 }, { 0x04, 0x3B }, { 0x04, 0x44 }, { 0x04, 0x4D }, { 0x04, 0x56 },
	{ 0x04, 0x5F }, { 0x04, 0x68 }, { 0x04, 0x71 }, { 0x04, 0x7A }, { 0x04, 0x83 },
	{ 0x04, 0x8C }, { 0x04, 0x95 }, { 0x04, 0x9E }, { 0x04, 0xA8 }, { 0x04, 0xB1 },
	{ 0x04, 0xBA }, { 0x04, 0xC3 }, { 0x04, 0xCC }, { 0x04, 0xD6 }, { 0x04, 0xDF },
	{ 0x04, 0xE8 }, { 0x04, 0xF2 }, { 0x04, 0xFB }, { 0x05, 0x04 }, { 0x05, 0x0E },
	{ 0x05, 0x17 }, { 0x05, 0x20 }, { 0x05, 0x2A }, { 0x05, 0x33 }, { 0x05, 0x3D },
	{ 0x05, 0x46 }, { 0x05, 0x50 }, { 0x05, 0x59 }, { 0x05, 0x63 }, { 0x05, 0x6C },
	{ 0x05, 0x76 }, { 0x05, 0x7F }, { 0x05, 0x89 }, { 0x05, 0x92 }, { 0x05, 0x9C },
	{ 0x05, 0xA5 }, { 0x05, 0xAF }, { 0x05, 0xB9 }, { 0x05, 0xC2 }, { 0x05, 0xCC },
	{ 0x05, 0xD6 }, { 0x05, 0xDF }, { 0x05, 0xE9 }, { 0x05, 0xF3 }, { 0x05, 0xFD },
	{ 0x06, 0x06 }, { 0x06, 0x10 }, { 0x06, 0x1A }, { 0x06, 0x24 }, { 0x06, 0x2D },
	{ 0x06, 0x37 }, { 0x06, 0x41 }, { 0x06, 0x4B }, { 0x06, 0x55 }, { 0x06, 0x5F },
	{ 0x06, 0x69 }, { 0x06, 0x72 }, { 0x06, 0x7C }, { 0x06, 0x86 }, { 0x06, 0x90 },
	{ 0x06, 0x9A }, { 0x06, 0xA4 }, { 0x06, 0xAE }, { 0x06, 0xB8 }, { 0x06, 0xC2 },
	{ 0x06, 0xCC }, { 0x06, 0xD6 }, { 0x06, 0xE0 }, { 0x06, 0xEA }, { 0x06, 0xF4 },
	{ 0x06, 0xFF }, { 0x07, 0x09 }, { 0x07, 0x13 }, { 0x07, 0x1D }, { 0x07, 0x27 },
	{ 0x07, 0x31 }, { 0x07, 0x3B }, { 0x07, 0x46 }, { 0x07, 0x50 }, { 0x07, 0x5A },
	{ 0x07, 0x64 }, { 0x07, 0x6E }, { 0x07, 0x79 }, { 0x07, 0x83 }, { 0x07, 0x8D },
	{ 0x07, 0x97 }, { 0x07, 0xA2 }, { 0x07, 0xAC }, { 0x07, 0xB6 }, { 0x07, 0xC1 },
	{ 0x07, 0xCB }, { 0x07, 0xD5 }, { 0x07, 0xE0 }, { 0x07, 0xEA }, { 0x07, 0xF5 },
	{ 0x07, 0xFF },
	{ 0x08, 0x87 }, { 0x08, 0x90 }, { 0x08, 0x9A }, { 0x08, 0xA3 }, { 0x08, 0xAC },
	{ 0x08, 0xB6 }, { 0x08, 0xBF }, { 0x08, 0xC9 }, { 0x08, 0xD2 }, { 0x08, 0xDB },
	{ 0x08, 0xE5 }, { 0x08, 0xEE }, { 0x08, 0xF8 }, { 0x09, 0x01 }, { 0x09, 0x0A },
	{ 0x09, 0x14 }, { 0x09, 0x1D }, { 0x09, 0x27 }, { 0x09, 0x30 }, { 0x09, 0x39 },
	{ 0x09, 0x43 }, { 0x09, 0x4C }, { 0x09, 0x56 }, { 0x09, 0x5F }, { 0x09, 0x68 },
	{ 0x09, 0x72 }, { 0x09, 0x7B }, { 0x09, 0x85 }, { 0x09, 0x8E }, { 0x09, 0x97 },
	{ 0x09, 0xA1 }, { 0x09, 0xAA }, { 0x09, 0xB4 }, { 0x09, 0xBD }, { 0x09, 0xC7 },
	{ 0x09, 0xD0 }, { 0x09, 0xD9 }, { 0x09, 0xE3 }, { 0x09, 0xEC }, { 0x09, 0xF6 },
	{ 0x09, 0xFF }, { 0x0A, 0x08 }, { 0x0A, 0x12 }, { 0x0A, 0x1B }, { 0x0A, 0x25 },
	{ 0x0A, 0x2E }, { 0x0A, 0x37 }, { 0x0A, 0x41 }, { 0x0A, 0x4A }, { 0x0A, 0x54 },
	{ 0x0A, 0x5D }, { 0x0A, 0x66 }, { 0x0A, 0x70 }, { 0x0A, 0x79 }, { 0x0A, 0x83 },
	{ 0x0A, 0x8C }, { 0x0A, 0x95 }, { 0x0A, 0x9F }, { 0x0A, 0xA8 }, { 0x0A, 0xB2 },
	{ 0x0A, 0xBB }, { 0x0A, 0xC4 }, { 0x0A, 0xCE }, { 0x0A, 0xD7 }, { 0x0A, 0xE1 },
	{ 0x0A, 0xEA }, { 0x0A, 0xF3 }, { 0x0A, 0xFD }, { 0x0B, 0x06 }, { 0x0B, 0x10 },
	{ 0x0B, 0x19 }, { 0x0B, 0x23 }, { 0x0B, 0x2C }, { 0x0B, 0x35 }, { 0x0B, 0x3F },
	{ 0x0B, 0x48 }, { 0x0B, 0x52 }, { 0x0B, 0x5B }, { 0x0B, 0x64 }, { 0x0B, 0x6E },
	{ 0x0B, 0x77 }, { 0x0B, 0x81 }, { 0x0B, 0x8A }, { 0x0B, 0x93 }, { 0x0B, 0x9D },
	{ 0x0B, 0xA6 }, { 0x0B, 0xB0 }, { 0x0B, 0xB9 }, { 0x0B, 0xC2 }, { 0x0B, 0xCC },
	{ 0x0B, 0xD5 }, { 0x0B, 0xDF }, { 0x0B, 0xE8 }, { 0x0B, 0xF1 }, { 0x0B, 0xFB },
	{ 0x0C, 0x04 }, { 0x0C, 0x0E }, { 0x0C, 0x17 }, { 0x0C, 0x20 }, { 0x0C, 0x2A },
	{ 0x0C, 0x33 }, { 0x0C, 0x3D }, { 0x0C, 0x46 }, { 0x0C, 0x4F }, { 0x0C, 0x59 },
	{ 0x0C, 0x62 }, { 0x0C, 0x6C }, { 0x0C, 0x75 }, { 0x0C, 0x7E }, { 0x0C, 0x88 },
	{ 0x0C, 0x91 }, { 0x0C, 0x9B }, { 0x0C, 0xA4 }, { 0x0C, 0xAE }, { 0x0C, 0xB7 },
	{ 0x0C, 0xC0 }, { 0x0C, 0xCA }, { 0x0C, 0xD3 }, { 0x0C, 0xDD }, { 0x0C, 0xE6 },
	{ 0x0C, 0xEF }, { 0x0C, 0xF9 }, { 0x0D, 0x02 }, { 0x0D, 0x0C }, { 0x0D, 0x15 },
	{ 0x0D, 0x1E }, { 0x0D, 0x28 }, { 0x0D, 0x31 }, { 0x0D, 0x3B }, { 0x0D, 0x44 },
	{ 0x0D, 0x4D }, { 0x0D, 0x57 }, { 0x0D, 0x60 }, { 0x0D, 0x6A }, { 0x0D, 0x73 },
	{ 0x0D, 0x7C }, { 0x0D, 0x86 }, { 0x0D, 0x8F }, { 0x0D, 0x99 }, { 0x0D, 0xA2 },
	{ 0x0D, 0xAB }, { 0x0D, 0xB5 }, { 0x0D, 0xBE }, { 0x0D, 0xC8 }, { 0x0D, 0xD1 },
	{ 0x0D, 0xDA }, { 0x0D, 0xE4 }, { 0x0D, 0xED }, { 0x0D, 0xF7 }, { 0x0E, 0x00 },
	{ 0x0E, 0x0A }, { 0x0E, 0x13 }, { 0x0E, 0x1C }, { 0x0E, 0x26 }, { 0x0E, 0x2F },
	{ 0x0E, 0x39 }, { 0x0E, 0x42 }, { 0x0E, 0x4B }, { 0x0E, 0x55 }, { 0x0E, 0x5E },
	{ 0x0E, 0x68 }, { 0x0E, 0x71 }, { 0x0E, 0x7A }, { 0x0E, 0x84 }, { 0x0E, 0x8D },
	{ 0x0E, 0x97 }, { 0x0E, 0xA0 }, { 0x0E, 0xA9 }, { 0x0E, 0xB3 }, { 0x0E, 0xBC },
	{ 0x0E, 0xC6 }, { 0x0E, 0xCF }, { 0x0E, 0xD8 }, { 0x0E, 0xE2 }, { 0x0E, 0xEB },
	{ 0x0E, 0xF5 }, { 0x0E, 0xFE }, { 0x0F, 0x07 }, { 0x0F, 0x11 }, { 0x0F, 0x1A },
	{ 0x0F, 0x24 }, { 0x0F, 0x2D }, { 0x0F, 0x36 }, { 0x0F, 0x40 }, { 0x0F, 0x49 },
	{ 0x0F, 0x53 }, { 0x0F, 0x5C }, { 0x0F, 0x65 }, { 0x0F, 0x6F }, { 0x0F, 0x78 },
	{ 0x0F, 0x82 }, { 0x0F, 0x8B }, { 0x0F, 0x95 }, { 0x0F, 0x9E }, { 0x0F, 0xA7 },
	{ 0x0F, 0xB1 }, { 0x0F, 0xBA }, { 0x0F, 0xC4 }, { 0x0F, 0xCD }, { 0x0F, 0xD6 },
	{ 0x0F, 0xE0 }, { 0x0F, 0xE9 }, { 0x0F, 0xF3 }, { 0x0F, 0xFC }, { 0x0F, 0xFF },
	/* HBM+ */
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
	{ 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF }, { 0x0F, 0xFF },
};

static u8 a26x_hbm_transition_table[SMOOTH_TRANS_MAX][MAX_DB7F2010B_NORMAL_HBM_TRANSITION][1] = {
	[SMOOTH_TRANS_OFF] = {
		[DB7F2010B_NORMAL_HBM_TRANSITION_NORMAL_TO_NORMAL] = { 0x20 },
		[DB7F2010B_NORMAL_HBM_TRANSITION_NORMAL_TO_HBM] = { 0x20 },
		[DB7F2010B_NORMAL_HBM_TRANSITION_HBM_TO_NORMAL] = { 0x20 },
		[DB7F2010B_NORMAL_HBM_TRANSITION_HBM_TO_HBM] = { 0x20 },
	},
	[SMOOTH_TRANS_ON] = {
		[DB7F2010B_NORMAL_HBM_TRANSITION_NORMAL_TO_NORMAL] = { 0x28 },
		[DB7F2010B_NORMAL_HBM_TRANSITION_NORMAL_TO_HBM] = { 0x28 },
		[DB7F2010B_NORMAL_HBM_TRANSITION_HBM_TO_NORMAL] = { 0x28 },
		[DB7F2010B_NORMAL_HBM_TRANSITION_HBM_TO_HBM] = { 0x28 },
	},
};

static u8 a26x_acl_frame_avg_table[][2] = {
	{ 0x04, 0x40 }, /* 16 Frame Avg */
	{ 0x05, 0x50 }, /* 32 Frame Avg */
};

static u8 a26x_acl_start_point_table[][2] = {
	[OLED_BR_HBM_OFF] = { 0x01, 0xFF }, /* 50 Percent */
	[OLED_BR_HBM_ON] = { 0x02, 0x65 }, /* 50 Percent */
};

static u8 a26x_acl_dim_speed_table[MAX_DB7F2010B_ACL_DIM][1] = {
	[DB7F2010B_ACL_DIM_OFF] = { 0x00 }, /* 0x00 : ACL Dimming Off */
	[DB7F2010B_ACL_DIM_ON] = { 0x20 }, /* 0x20 : ACL Dimming 32 Frames */
};

static u8 a26x_acl_opr_table[MAX_PANEL_HBM][MAX_DB7F2010B_ACL_OPR][1] = {
	[PANEL_HBM_OFF] = {
		[DB7F2010B_ACL_OPR_0] = { 0x00 }, /* ACL OFF, OPR 0% */
		[DB7F2010B_ACL_OPR_1] = { 0x01 }, /* ACL ON, OPR 8% */
		[DB7F2010B_ACL_OPR_2] = { 0x03 }, /* ACL ON, OPR 15% */
		[DB7F2010B_ACL_OPR_3] = { 0x03 }, /* ACL ON, OPR 15% */
	},
	[PANEL_HBM_ON] = {
		[DB7F2010B_ACL_OPR_0] = { 0x00 }, /* ACL OFF, OPR 0% */
		[DB7F2010B_ACL_OPR_1] = { 0x01 }, /* ACL ON, OPR 8% */
		[DB7F2010B_ACL_OPR_2] = { 0x01 }, /* ACL ON, OPR 8% */
		[DB7F2010B_ACL_OPR_3] = { 0x01 }, /* ACL ON, OPR 8% */
	},
};

static u8 a26x_tset_table[256][1] = {
	/* 0 ~ 127 */
	{ 0x00 }, { 0x01 }, { 0x02 }, { 0x03 }, { 0x04 }, { 0x05 }, { 0x06 }, { 0x07 },
	{ 0x08 }, { 0x09 }, { 0x0A }, { 0x0B }, { 0x0C }, { 0x0D }, { 0x0E }, { 0x0F },
	{ 0x10 }, { 0x11 }, { 0x12 }, { 0x13 }, { 0x14 }, { 0x15 }, { 0x16 }, { 0x17 },
	{ 0x18 }, { 0x19 }, { 0x1A }, { 0x1B }, { 0x1C }, { 0x1D }, { 0x1E }, { 0x1F },
	{ 0x20 }, { 0x21 }, { 0x22 }, { 0x23 }, { 0x24 }, { 0x25 }, { 0x26 }, { 0x27 },
	{ 0x28 }, { 0x29 }, { 0x2A }, { 0x2B }, { 0x2C }, { 0x2D }, { 0x2E }, { 0x2F },
	{ 0x30 }, { 0x31 }, { 0x32 }, { 0x33 }, { 0x34 }, { 0x35 }, { 0x36 }, { 0x37 },
	{ 0x38 }, { 0x39 }, { 0x3A }, { 0x3B }, { 0x3C }, { 0x3D }, { 0x3E }, { 0x3F },
	{ 0x40 }, { 0x41 }, { 0x42 }, { 0x43 }, { 0x44 }, { 0x45 }, { 0x46 }, { 0x47 },
	{ 0x48 }, { 0x49 }, { 0x4A }, { 0x4B }, { 0x4C }, { 0x4D }, { 0x4E }, { 0x4F },
	{ 0x50 }, { 0x51 }, { 0x52 }, { 0x53 }, { 0x54 }, { 0x55 }, { 0x56 }, { 0x57 },
	{ 0x58 }, { 0x59 }, { 0x5A }, { 0x5B }, { 0x5C }, { 0x5D }, { 0x5E }, { 0x5F },
	{ 0x60 }, { 0x61 }, { 0x62 }, { 0x63 }, { 0x64 }, { 0x65 }, { 0x66 }, { 0x67 },
	{ 0x68 }, { 0x69 }, { 0x6A }, { 0x6B }, { 0x6C }, { 0x6D }, { 0x6E }, { 0x6F },
	{ 0x70 }, { 0x71 }, { 0x72 }, { 0x73 }, { 0x74 }, { 0x75 }, { 0x76 }, { 0x77 },
	{ 0x78 }, { 0x79 }, { 0x7A }, { 0x7B }, { 0x7C }, { 0x7D }, { 0x7E }, { 0x7F },
	/* 0 ~ -127 */
	{ 0x80 }, { 0x81 }, { 0x82 }, { 0x83 }, { 0x84 }, { 0x85 }, { 0x86 }, { 0x87 },
	{ 0x88 }, { 0x89 }, { 0x8A }, { 0x8B }, { 0x8C }, { 0x8D }, { 0x8E }, { 0x8F },
	{ 0x90 }, { 0x91 }, { 0x92 }, { 0x93 }, { 0x94 }, { 0x95 }, { 0x96 }, { 0x97 },
	{ 0x98 }, { 0x99 }, { 0x9A }, { 0x9B }, { 0x9C }, { 0x9D }, { 0x9E }, { 0x9F },
	{ 0xA0 }, { 0xA1 }, { 0xA2 }, { 0xA3 }, { 0xA4 }, { 0xA5 }, { 0xA6 }, { 0xA7 },
	{ 0xA8 }, { 0xA9 }, { 0xAA }, { 0xAB }, { 0xAC }, { 0xAD }, { 0xAE }, { 0xAF },
	{ 0xB0 }, { 0xB1 }, { 0xB2 }, { 0xB3 }, { 0xB4 }, { 0xB5 }, { 0xB6 }, { 0xB7 },
	{ 0xB8 }, { 0xB9 }, { 0xBA }, { 0xBB }, { 0xBC }, { 0xBD }, { 0xBE }, { 0xBF },
	{ 0xC0 }, { 0xC1 }, { 0xC2 }, { 0xC3 }, { 0xC4 }, { 0xC5 }, { 0xC6 }, { 0xC7 },
	{ 0xC8 }, { 0xC9 }, { 0xCA }, { 0xCB }, { 0xCC }, { 0xCD }, { 0xCE }, { 0xCF },
	{ 0xD0 }, { 0xD1 }, { 0xD2 }, { 0xD3 }, { 0xD4 }, { 0xD5 }, { 0xD6 }, { 0xD7 },
	{ 0xD8 }, { 0xD9 }, { 0xDA }, { 0xDB }, { 0xDC }, { 0xDD }, { 0xDE }, { 0xDF },
	{ 0xE0 }, { 0xE1 }, { 0xE2 }, { 0xE3 }, { 0xE4 }, { 0xE5 }, { 0xE6 }, { 0xE7 },
	{ 0xE8 }, { 0xE9 }, { 0xEA }, { 0xEB }, { 0xEC }, { 0xED }, { 0xEE }, { 0xEF },
	{ 0xF0 }, { 0xF1 }, { 0xF2 }, { 0xF3 }, { 0xF4 }, { 0xF5 }, { 0xF6 }, { 0xF7 },
	{ 0xF8 }, { 0xF9 }, { 0xFA }, { 0xFB }, { 0xFC }, { 0xFD }, { 0xFE }, { 0xFF },
};

static u8 a26x_fps_table_1[MAX_DB7F2010B_VRR_FPS][1] = {
	[DB7F2010B_VRR_FPS_120] = { 0x00 },
	[DB7F2010B_VRR_FPS_60] = { 0x01 },
};

static u8 a26x_dimming_speed_table_1[SMOOTH_TRANS_MAX][MAX_DB7F2010B_NORMAL_HBM_TRANSITION][1] = {
	[SMOOTH_TRANS_OFF] = {
		[DB7F2010B_NORMAL_HBM_TRANSITION_NORMAL_TO_NORMAL] = { 0x20 },
		[DB7F2010B_NORMAL_HBM_TRANSITION_NORMAL_TO_HBM] = { 0x20 },
		[DB7F2010B_NORMAL_HBM_TRANSITION_HBM_TO_NORMAL] = { 0x20 },
		[DB7F2010B_NORMAL_HBM_TRANSITION_HBM_TO_HBM] = { 0x20 },
	},
	[SMOOTH_TRANS_ON] = {
		[DB7F2010B_NORMAL_HBM_TRANSITION_NORMAL_TO_NORMAL] = { 0x60 },
		[DB7F2010B_NORMAL_HBM_TRANSITION_NORMAL_TO_HBM] = { 0x20 },
		[DB7F2010B_NORMAL_HBM_TRANSITION_HBM_TO_NORMAL] = { 0x20 },
		[DB7F2010B_NORMAL_HBM_TRANSITION_HBM_TO_HBM] = { 0x60 },
	},
};

static u8 a26x_irc_mode_table[][1] = {
	[0 ... DB7F2010B_A26X_TOTAL_STEP - 2] = { 0x00 },		/* flat gamma */
	[DB7F2010B_A26X_TOTAL_STEP - 1] = { 0x02 },		/* moderato */
};

static struct maptbl a26x_maptbl[MAX_MAPTBL] = {
	[GAMMA_MODE2_MAPTBL] = __OLED_MAPTBL_OVERRIDE_INIT_INITIALIZER(a26x_brt_table, &DDI_FUNC(DB7F2010B_MAPTBL_INIT_GAMMA_MODE2_BRT), DB7F2010B_A26X_BR_INDEX_PROPERTY),
	[HBM_ONOFF_MAPTBL] = __OLED_MAPTBL_DEFAULT_INITIALIZER(a26x_hbm_transition_table, PANEL_BL_PROPERTY_SMOOTH_TRANSITION, DB7F2010B_NORMAL_HBM_TRANSITION_PROPERTY),
	[ACL_FRAME_AVG_MAPTBL] = __OLED_MAPTBL_DEFAULT_INITIALIZER(a26x_acl_frame_avg_table, PANEL_BL_PROPERTY_ACL_PWRSAVE),
	[ACL_START_POINT_MAPTBL] = __OLED_MAPTBL_DEFAULT_INITIALIZER(a26x_acl_start_point_table, OLED_BR_HBM_PROPERTY),
	[ACL_DIM_SPEED_MAPTBL] = __OLED_MAPTBL_DEFAULT_INITIALIZER(a26x_acl_dim_speed_table, DB7F2010B_ACL_DIM_PROPERTY),
	[ACL_OPR_MAPTBL] = __OLED_MAPTBL_DEFAULT_INITIALIZER(a26x_acl_opr_table, OLED_BR_HBM_PROPERTY, DB7F2010B_ACL_OPR_PROPERTY),
	[TSET_MAPTBL] = __OLED_MAPTBL_DEFAULT_INITIALIZER(a26x_tset_table, OLED_TSET_PROPERTY),
	[FPS_MAPTBL_1] = __OLED_MAPTBL_DEFAULT_INITIALIZER(a26x_fps_table_1, DB7F2010B_VRR_PROPERTY),
	[DIMMING_SPEED] = __OLED_MAPTBL_DEFAULT_INITIALIZER(a26x_dimming_speed_table_1, PANEL_BL_PROPERTY_SMOOTH_TRANSITION, DB7F2010B_NORMAL_HBM_TRANSITION_PROPERTY),
	[IRC_MODE_MAPTBL] = __OLED_MAPTBL_DEFAULT_INITIALIZER(a26x_irc_mode_table, DB7F2010B_A26X_BR_INDEX_PROPERTY),
};

/* ===================================================================================== */
/* ============================== [DB7F2010B COMMAND TABLE] ============================== */
/* ===================================================================================== */
static u8 A26X_KEY1_ENABLE[] = { 0x9F, 0x5A, 0x5A };
static u8 A26X_KEY2_ENABLE[] = { 0xF0, 0x5A, 0x5A };
static u8 A26X_KEY3_ENABLE[] = { 0xFC, 0x5A, 0x5A };

static u8 A26X_KEY1_DISABLE[] = { 0x9F, 0xA5, 0xA5 };
static u8 A26X_KEY2_DISABLE[] = { 0xF0, 0xA5, 0xA5 };
static u8 A26X_KEY3_DISABLE[] = { 0xFC, 0xA5, 0xA5 };
static u8 A26X_SLEEP_OUT[] = { 0x11 };
static u8 A26X_SLEEP_IN[] = { 0x10 };
static u8 A26X_DISPLAY_OFF[] = { 0x28 };
static u8 A26X_DISPLAY_ON[] = { 0x29 };


static DEFINE_STATIC_PACKET(a26x_level1_key_enable, DSI_PKT_TYPE_WR, A26X_KEY1_ENABLE, 0);
static DEFINE_STATIC_PACKET(a26x_level2_key_enable, DSI_PKT_TYPE_WR, A26X_KEY2_ENABLE, 0);
static DEFINE_STATIC_PACKET(a26x_level3_key_enable, DSI_PKT_TYPE_WR, A26X_KEY3_ENABLE, 0);

static DEFINE_STATIC_PACKET(a26x_level1_key_disable, DSI_PKT_TYPE_WR, A26X_KEY1_DISABLE, 0);
static DEFINE_STATIC_PACKET(a26x_level2_key_disable, DSI_PKT_TYPE_WR, A26X_KEY2_DISABLE, 0);
static DEFINE_STATIC_PACKET(a26x_level3_key_disable, DSI_PKT_TYPE_WR, A26X_KEY3_DISABLE, 0);

static DEFINE_STATIC_PACKET(a26x_sleep_out, DSI_PKT_TYPE_WR, A26X_SLEEP_OUT, 0);
static DEFINE_STATIC_PACKET(a26x_sleep_in, DSI_PKT_TYPE_WR, A26X_SLEEP_IN, 0);
static DEFINE_STATIC_PACKET(a26x_display_on, DSI_PKT_TYPE_WR, A26X_DISPLAY_ON, 0);
static DEFINE_STATIC_PACKET(a26x_display_off, DSI_PKT_TYPE_WR, A26X_DISPLAY_OFF, 0);

static u8 A26X_TSET_SET[] = {
	0xF2,
	0x00,
};
static DEFINE_PKTUI(a26x_tset_set, &a26x_maptbl[TSET_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(a26x_tset_set, DSI_PKT_TYPE_WR, A26X_TSET_SET, 0);

static DEFINE_PANEL_MDELAY(a26x_wait_6msec, 6);
static DEFINE_PANEL_MDELAY(a26x_wait_1msec, 1);
static DEFINE_PANEL_MDELAY(a26x_wait_10msec, 10);
static DEFINE_PANEL_MDELAY(a26x_wait_100msec, 100);
static DEFINE_PANEL_MDELAY(a26x_wait_30msec, 30);
static DEFINE_PANEL_MDELAY(a26x_wait_17msec, 17);

static DEFINE_PANEL_MDELAY(a26x_wait_34msec, 34);

static DEFINE_PANEL_MDELAY(a26x_wait_sleep_out_20msec, 20);
static DEFINE_PANEL_MDELAY(a26x_wait_display_off, 20);

static DEFINE_PANEL_MDELAY(a26x_wait_sleep_in, 120);
static DEFINE_PANEL_UDELAY(a26x_wait_1usec, 1);


static DEFINE_PANEL_KEY(a26x_level1_key_enable, CMD_LEVEL_1, KEY_ENABLE, &PKTINFO(a26x_level1_key_enable));
static DEFINE_PANEL_KEY(a26x_level2_key_enable, CMD_LEVEL_2, KEY_ENABLE, &PKTINFO(a26x_level2_key_enable));
static DEFINE_PANEL_KEY(a26x_level3_key_enable, CMD_LEVEL_3, KEY_ENABLE, &PKTINFO(a26x_level3_key_enable));

static DEFINE_PANEL_KEY(a26x_level1_key_disable, CMD_LEVEL_1, KEY_DISABLE, &PKTINFO(a26x_level1_key_disable));
static DEFINE_PANEL_KEY(a26x_level2_key_disable, CMD_LEVEL_2, KEY_DISABLE, &PKTINFO(a26x_level2_key_disable));
static DEFINE_PANEL_KEY(a26x_level3_key_disable, CMD_LEVEL_3, KEY_DISABLE, &PKTINFO(a26x_level3_key_disable));

static DEFINE_PANEL_VSYNC_DELAY(a26x_wait_1_vsync, 1);
#ifdef CONFIG_USDM_PANEL_MASK_LAYER
static DEFINE_PANEL_VSYNC_DELAY(a26x_wait_2_vsync, 2);
static DEFINE_RULE_BASED_COND(a26x_cond_is_60hz,
		PANEL_PROPERTY_PANEL_REFRESH_RATE, EQ, 60);
static DEFINE_RULE_BASED_COND(a26x_cond_is_120hz,
		PANEL_PROPERTY_PANEL_REFRESH_RATE, EQ, 120);
#endif

static u8 A26X_HBM_TRANSITION[] = {
	0x53, 0x20
};
static DEFINE_PKTUI(a26x_hbm_transition, &a26x_maptbl[HBM_ONOFF_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(a26x_hbm_transition, DSI_PKT_TYPE_WR, A26X_HBM_TRANSITION, 0);

static u8 A26X_ACL_SET_1_GPARA[] = {
	0xB0,
	0x00, 0xB8, 0x66, 0x03
};
static DEFINE_STATIC_PACKET(a26x_acl_set_1_gpara, DSI_PKT_TYPE_WR, A26X_ACL_SET_1_GPARA, 0);

static u8 A26X_ACL_SET_1[] = {
	0x66,
	0x05, 0x50
};
static DEFINE_PKTUI(a26x_acl_set_1, &a26x_maptbl[ACL_FRAME_AVG_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(a26x_acl_set_1, DSI_PKT_TYPE_WR, A26X_ACL_SET_1, 0);

static u8 A26X_ACL_SET_2_GPARA[] = {
	0xB0,
	0x00, 0xD3, 0x66, 0x03
};
static DEFINE_STATIC_PACKET(a26x_acl_set_2_gpara, DSI_PKT_TYPE_WR, A26X_ACL_SET_2_GPARA, 0);

static u8 A26X_ACL_SET_2[] = {
	0x66,
	0x20, 0x00, 0x10, 0x01, 0xFF
};

/*
 * ACL SETTING
 * 1st Para.
 * 0x00 : ACL dim off
 * 0x20 : 32 Frame Average
 * 4th 5th Para
 * 0x01, 0xFF : ACL Start Step 50%
 * 0x02, 0x65 : ACL Start Step 60%
 */

static DECLARE_PKTUI(a26x_acl_set_2) = {
	{ .offset = 1, .maptbl = &a26x_maptbl[ACL_DIM_SPEED_MAPTBL] },
	{ .offset = 4, .maptbl = &a26x_maptbl[ACL_START_POINT_MAPTBL] },
};
static DEFINE_VARIABLE_PACKET(a26x_acl_set_2, DSI_PKT_TYPE_WR, A26X_ACL_SET_2, 0);

static u8 A26X_ACL[] = {
	0x55, 0x03
};
static DEFINE_PKTUI(a26x_acl_control, &a26x_maptbl[ACL_OPR_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(a26x_acl_control, DSI_PKT_TYPE_WR, A26X_ACL, 0);

static u8 A26X_WRDISBV[] = {
	0x51, 0x03, 0xFF
};
static DEFINE_PKTUI(a26x_wrdisbv, &a26x_maptbl[GAMMA_MODE2_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(a26x_wrdisbv, DSI_PKT_TYPE_WR, A26X_WRDISBV, 0);

static u8 A26X_SMOOTH_DIMMING_GPARA[] = {
	0xB0,
	0x00, 0x12, 0x66, 0x03
};
static DEFINE_STATIC_PACKET(a26x_smooth_dimming_gpara, DSI_PKT_TYPE_WR, A26X_SMOOTH_DIMMING_GPARA, 0);

static u8 A26X_SMOOTH_DIMMING_1F[] = {
	0x66,
	0x01,
};
static DEFINE_STATIC_PACKET(a26x_smooth_dimming_1f, DSI_PKT_TYPE_WR, A26X_SMOOTH_DIMMING_1F, 0);

static u8 A26X_SMOOTH_DIMMING_15F[] = {
	0x66,
	0x0F,
};
static DEFINE_STATIC_PACKET(a26x_smooth_dimming_15f, DSI_PKT_TYPE_WR, A26X_SMOOTH_DIMMING_15F, 0);
static u8 A26X_PCD_DET_SET[] = {
	0xEA, 0x59
};
static DEFINE_STATIC_PACKET(a26x_pcd_det_set, DSI_PKT_TYPE_WR, A26X_PCD_DET_SET, 0);

static u8 A26X_FPS_1[] = { 0x60, 0x00 };
static DEFINE_PKTUI(a26x_fps_1, &a26x_maptbl[FPS_MAPTBL_1], 1);
static DEFINE_VARIABLE_PACKET(a26x_fps_1, DSI_PKT_TYPE_WR, A26X_FPS_1, 0);

static u8 A26X_PANEL_UPDATE[] = {
	0xF7,
	0x0F
};
static DEFINE_STATIC_PACKET(a26x_panel_update, DSI_PKT_TYPE_WR, A26X_PANEL_UPDATE, 0);

static u8 A26X_OTP_LOADING[] = {
	0xD0,
	0x00, 0x00
};
static DEFINE_STATIC_PACKET(a26x_otp_loading, DSI_PKT_TYPE_WR, A26X_OTP_LOADING, 0);

/* WARNING - DB7F2010B using normal data type for DSC setting */
static u8 A26X_DSC[] = { 0x9D, 0x01 };
static DEFINE_STATIC_PACKET(a26x_dsc, DSI_PKT_TYPE_WR, A26X_DSC, 0);
static u8 A26X_PPS[] = {
	//1080x2340 Slice Info : 540x45 / 2 slice mode
	0x9E,
	0x11, 0x00, 0x00, 0x89, 0x30, 0x80, 0x09, 0x24,
	0x04, 0x38, 0x00, 0x2D, 0x02, 0x1C, 0x02, 0x1C,
	0x02, 0x00, 0x02, 0x0E, 0x00, 0x20, 0x04, 0x5A,
	0x00, 0x07, 0x00, 0x0C, 0x02, 0x2F, 0x02, 0x43,
	0x18, 0x00, 0x10, 0xF0, 0x03, 0x0C, 0x20, 0x00,
	0x06, 0x0B, 0x0B, 0x33, 0x0E, 0x1C, 0x2A, 0x38,
	0x46, 0x54, 0x62, 0x69, 0x70, 0x77, 0x79, 0x7B,
	0x7D, 0x7E, 0x01, 0x02, 0x01, 0x00, 0x09, 0x40,
	0x09, 0xBE, 0x19, 0xFC, 0x19, 0xFA, 0x19, 0xF8,
	0x1A, 0x38, 0x1A, 0x78, 0x1A, 0xB6, 0x2A, 0xF6,
	0x2B, 0x34, 0x2B, 0x74, 0x3B, 0x74, 0x6B, 0xF4,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static DEFINE_STATIC_PACKET(a26x_pps, DSI_PKT_TYPE_WR, A26X_PPS, 0);

/* read gpara */
static u8 A26X_DATE_GPARA[] = {
	0xB0,
	0x00, 0x04, 0xA1, 0x03
};
static DEFINE_STATIC_PACKET(a26x_date_gpara, DSI_PKT_TYPE_WR, A26X_DATE_GPARA, 0);

static u8 A26X_OCTAID_0_GPARA[] = {
	0xB0,
	0x00, 0x0B, 0xA1, 0x03
};
static DEFINE_STATIC_PACKET(a26x_octaid_0_gpara, DSI_PKT_TYPE_WR, A26X_OCTAID_0_GPARA, 0);

/* flash setting */
static u8 A26X_FLASH_SETTING1_GPARA[] = {
	0xB0,
	0x00, 0x03, 0xF2, 0x03
};
static DEFINE_STATIC_PACKET(a26x_flash_setting1_gpara, DSI_PKT_TYPE_WR, A26X_FLASH_SETTING1_GPARA, 0);

static u8 A26X_FLASH_SETTING1 [] = {
	0xF2,
	0x40
};
static DEFINE_STATIC_PACKET(a26x_flash_setting1 , DSI_PKT_TYPE_WR, A26X_FLASH_SETTING1, 0);

static u8 A26X_FLASH_SETTING2_GPARA[] = {
	0xB0,
	0x00, 0x17, 0xC1, 0x03
};
static DEFINE_STATIC_PACKET(a26x_flash_setting2_gpara, DSI_PKT_TYPE_WR, A26X_FLASH_SETTING2_GPARA, 0);

static u8 A26X_FLASH_SETTING2 [] = {
	0xC1,
	0x11, 0x06, 0x50, 0x00
};
static DEFINE_STATIC_PACKET(a26x_flash_setting2 , DSI_PKT_TYPE_WR, A26X_FLASH_SETTING2, 0);

static u8 A26X_FLASH_SETTING3 [] = {
	0xC0,
	0x01
};
static DEFINE_STATIC_PACKET(a26x_flash_setting3 , DSI_PKT_TYPE_WR, A26X_FLASH_SETTING3, 0);

static u8 A26X_FLASH_SETTING4 [] = {
	0xC0,
	0x02
};
static DEFINE_STATIC_PACKET(a26x_flash_setting4 , DSI_PKT_TYPE_WR, A26X_FLASH_SETTING4, 0);

static u8 A26X_FLASH_SETTING5_GPARA[] = {
	0xB0,
	0x00, 0x17, 0xC1, 0x03
};
static DEFINE_STATIC_PACKET(a26x_flash_setting5_gpara, DSI_PKT_TYPE_WR, A26X_FLASH_SETTING5_GPARA, 0);

static u8 A26X_FLASH_SETTING5 [] = {
	0xC1,
	0x00
};
static DEFINE_STATIC_PACKET(a26x_flash_setting5 , DSI_PKT_TYPE_WR, A26X_FLASH_SETTING5, 0);

static u8 A26X_FLASH_SETTING6_GPARA[] = {
	0xB0,
	0x00, 0x03, 0xF2, 0x03
};
static DEFINE_STATIC_PACKET(a26x_flash_setting6_gpara, DSI_PKT_TYPE_WR, A26X_FLASH_SETTING6_GPARA, 0);

static u8 A26X_FLASH_SETTING6 [] = {
	0xF2,
	0x00
};
static DEFINE_STATIC_PACKET(a26x_flash_setting6 , DSI_PKT_TYPE_WR, A26X_FLASH_SETTING6, 0);

static u8 A26X_IRC_MODE_GPARA[] = {
	0xB0,
	0x00, 0x01, 0x6A, 0x03
};
static DEFINE_STATIC_PACKET(a26x_irc_mode_gpara, DSI_PKT_TYPE_WR, A26X_IRC_MODE_GPARA, 0);

static u8 A26X_IRC_MODE[] = {
	0x6A,
	0x00
};
static DEFINE_PKTUI(a26x_irc_mode, &a26x_maptbl[IRC_MODE_MAPTBL], 1);
static DEFINE_VARIABLE_PACKET(a26x_irc_mode, DSI_PKT_TYPE_WR, A26X_IRC_MODE, 0);

#ifdef CONFIG_USDM_FACTORY_CCD_TEST
static u8 A26X_KEY_F1_ENABLE[] = { 0xF1, 0x5A, 0x5A };
static u8 A26X_KEY_F1_DISABLE[] = { 0xF1, 0xA5, 0xA5 };
static DEFINE_STATIC_PACKET(a26x_key_f1_enable, DSI_PKT_TYPE_WR, A26X_KEY_F1_ENABLE, 0);
static DEFINE_STATIC_PACKET(a26x_key_f1_disable, DSI_PKT_TYPE_WR, A26X_KEY_F1_DISABLE, 0);

static u8 A26X_CCD_ENABLE[] = { 0xEA, 0x5C, 0x51, 0x01, 0x00 };
static u8 A26X_CCD_DISABLE[] = { 0xEA, 0x5C, 0x51, 0x00, 0x00 };
static DEFINE_STATIC_PACKET(a26x_ccd_test_enable, DSI_PKT_TYPE_WR, A26X_CCD_ENABLE, 0x00);
static DEFINE_STATIC_PACKET(a26x_ccd_test_disable, DSI_PKT_TYPE_WR, A26X_CCD_DISABLE, 0x00);
#endif

static DEFINE_PNOBJ_CONFIG(a26x_set_wait_tx_done_property_off, PANEL_PROPERTY_WAIT_TX_DONE, WAIT_TX_DONE_MANUAL_OFF);
static DEFINE_PNOBJ_CONFIG(a26x_set_wait_tx_done_property_auto, PANEL_PROPERTY_WAIT_TX_DONE, WAIT_TX_DONE_AUTO);

static DEFINE_PNOBJ_CONFIG(a26x_set_osc_un_change, DB7F2010B_A26X_OSC_CHANGE_PROPERTY, DB7F2010B_A26X_OSC_UN_CHANGE);
static DEFINE_PNOBJ_CONFIG(a26x_set_osc_change_done, DB7F2010B_A26X_OSC_CHANGE_PROPERTY, DB7F2010B_A26X_OSC_CHANGE_DONE);

static struct seqinfo SEQINFO(a26x_set_bl_param_seq);
static struct seqinfo SEQINFO(a26x_set_fps_param_seq);
#if defined(CONFIG_USDM_FACTORY)
static struct seqinfo SEQINFO(a26x_res_init_seq);
#endif

// static DEFINE_PNOBJ_CONFIG(a26x_set_separate_tx_off, PANEL_PROPERTY_SEPARATE_TX, SEPARATE_TX_OFF);
// static DEFINE_PNOBJ_CONFIG(a26x_set_separate_tx_on, PANEL_PROPERTY_SEPARATE_TX, SEPARATE_TX_ON);

static void *a26x_common_setting_cmdtbl[] = {
	&KEYINFO(a26x_level1_key_enable),
	&KEYINFO(a26x_level2_key_enable),
	&KEYINFO(a26x_level3_key_enable),

	&PKTINFO(a26x_dsc),
	&PKTINFO(a26x_pps),
	&PKTINFO(a26x_pcd_det_set),
	&PKTINFO(a26x_panel_update),

	&PKTINFO(a26x_acl_set_1_gpara),
	&PKTINFO(a26x_acl_set_1),
	&PKTINFO(a26x_acl_set_2_gpara),
	&PKTINFO(a26x_acl_set_2),
	&PKTINFO(a26x_smooth_dimming_gpara),
	&PKTINFO(a26x_smooth_dimming_1f),
	&PKTINFO(a26x_panel_update),
	&SEQINFO(a26x_set_bl_param_seq), /* includes FPS setting also */

	&KEYINFO(a26x_level3_key_disable),
	&KEYINFO(a26x_level2_key_disable),
	&KEYINFO(a26x_level1_key_disable),
};

static DEFINE_SEQINFO(a26x_common_setting_seq,
		a26x_common_setting_cmdtbl);

static void *a26x_init_cmdtbl[] = {
	&DLYINFO(a26x_wait_10msec),
	&KEYINFO(a26x_level2_key_enable),
	&PKTINFO(a26x_otp_loading),
	&KEYINFO(a26x_level2_key_disable),
	&PKTINFO(a26x_sleep_out),
	&DLYINFO(a26x_wait_100msec),

	&SEQINFO(a26x_common_setting_seq),

#if defined(CONFIG_USDM_FACTORY)
	&SEQINFO(a26x_res_init_seq),
#endif
};

static void *a26x_res_init_cmdtbl[] = {
	&KEYINFO(a26x_level1_key_enable),
	&KEYINFO(a26x_level2_key_enable),
	&KEYINFO(a26x_level3_key_enable),
	&db7f2010b_restbl[RES_ID],
	&db7f2010b_restbl[RES_CODE],
	&db7f2010b_restbl[RES_COORDINATE],
	&PKTINFO(a26x_date_gpara),
	&db7f2010b_restbl[RES_DATE],
	&PKTINFO(a26x_octaid_0_gpara),
	&db7f2010b_restbl[RES_OCTA_ID_0],
	&PKTINFO(a26x_flash_setting1_gpara),
	&PKTINFO(a26x_flash_setting1),
	&PKTINFO(a26x_flash_setting2_gpara),
	&PKTINFO(a26x_flash_setting2),
	&DLYINFO(a26x_wait_1msec),
	&PKTINFO(a26x_flash_setting3),
	&DLYINFO(a26x_wait_1msec),
	&db7f2010b_restbl[RES_OCTA_ID_1],
	&PKTINFO(a26x_flash_setting4),
	&PKTINFO(a26x_flash_setting5_gpara),
	&PKTINFO(a26x_flash_setting5),
	&PKTINFO(a26x_flash_setting6_gpara),
	&PKTINFO(a26x_flash_setting6),
#ifdef CONFIG_USDM_PANEL_DPUI
	&db7f2010b_restbl[RES_SELF_DIAG],
	&db7f2010b_restbl[RES_ERR_FG],
	&db7f2010b_restbl[RES_DSI_ERR],
#endif
	&KEYINFO(a26x_level3_key_disable),
	&KEYINFO(a26x_level2_key_disable),
	&KEYINFO(a26x_level1_key_disable),
};


#if defined(CONFIG_USDM_FACTORY)
static DEFINE_SEQINFO(a26x_res_init_seq, a26x_res_init_cmdtbl);
#endif

/* bl for normal */
static void *a26x_set_bl_param_cmdtbl[] = {
	&PKTINFO(a26x_fps_1),
	&PKTINFO(a26x_tset_set),
	&PKTINFO(a26x_acl_control),
	&PKTINFO(a26x_hbm_transition),
	&PKTINFO(a26x_wrdisbv),
	&PKTINFO(a26x_irc_mode_gpara),
	&PKTINFO(a26x_irc_mode),
	&PKTINFO(a26x_panel_update),
};

static DEFINE_SEQINFO(a26x_set_bl_param_seq, a26x_set_bl_param_cmdtbl);

static void *a26x_set_bl_cmdtbl[] = {
	&KEYINFO(a26x_level1_key_enable),
	&KEYINFO(a26x_level2_key_enable),
	&SEQINFO(a26x_set_bl_param_seq),
	&KEYINFO(a26x_level2_key_disable),
	&KEYINFO(a26x_level1_key_disable),
};

static DEFINE_RULE_BASED_COND(a26x_cond_is_panel_state_not_lpm,
		PANEL_PROPERTY_PANEL_STATE, NE, PANEL_STATE_ALPM);
static DEFINE_RULE_BASED_COND(a26x_cond_is_panel_state_lpm,
		PANEL_PROPERTY_PANEL_STATE, EQ, PANEL_STATE_ALPM);

static void *a26x_set_fps_cmdtbl[] = {
	&PNOBJ_CONFIG(a26x_set_wait_tx_done_property_off),
		&KEYINFO(a26x_level1_key_enable),
		&KEYINFO(a26x_level2_key_enable),
		&KEYINFO(a26x_level3_key_enable),
		&SEQINFO(a26x_set_bl_param_seq),
		&KEYINFO(a26x_level3_key_disable),
		&KEYINFO(a26x_level2_key_disable),
		&KEYINFO(a26x_level1_key_disable),
	&PNOBJ_CONFIG(a26x_set_wait_tx_done_property_auto),
};

static void *a26x_display_on_cmdtbl[] = {
	&DLYINFO(a26x_wait_sleep_out_20msec),
	&KEYINFO(a26x_level1_key_enable),
	&KEYINFO(a26x_level2_key_enable),
	&KEYINFO(a26x_level3_key_enable),
	&PKTINFO(a26x_display_on),
	&DLYINFO(a26x_wait_17msec),
	&PKTINFO(a26x_smooth_dimming_gpara),
	&PKTINFO(a26x_smooth_dimming_15f),
	&PKTINFO(a26x_panel_update),
	&KEYINFO(a26x_level3_key_disable),
	&KEYINFO(a26x_level2_key_disable),
	&KEYINFO(a26x_level1_key_disable),
};

static void *a26x_display_off_cmdtbl[] = {
	&KEYINFO(a26x_level1_key_enable),
	&KEYINFO(a26x_level2_key_enable),
	&KEYINFO(a26x_level3_key_enable),
	&PKTINFO(a26x_display_off),
	&KEYINFO(a26x_level3_key_disable),
	&KEYINFO(a26x_level2_key_disable),
	&KEYINFO(a26x_level1_key_disable),
	&DLYINFO(a26x_wait_display_off),
};

static void *a26x_exit_cmdtbl[] = {
	&KEYINFO(a26x_level1_key_enable),
	&KEYINFO(a26x_level2_key_enable),
	&KEYINFO(a26x_level3_key_enable),
#ifdef CONFIG_USDM_PANEL_DPUI
	&db7f2010b_dmptbl[DUMP_DSI_ERR],
	&db7f2010b_dmptbl[DUMP_SELF_DIAG],
#endif
	&PKTINFO(a26x_sleep_in),
	&KEYINFO(a26x_level3_key_disable),
	&KEYINFO(a26x_level2_key_disable),
	&KEYINFO(a26x_level1_key_disable),
	&DLYINFO(a26x_wait_sleep_in),
};

static void *a26x_dump_cmdtbl[] = {
	&KEYINFO(a26x_level1_key_enable),
	&KEYINFO(a26x_level2_key_enable),
	&KEYINFO(a26x_level3_key_enable),
	&db7f2010b_dmptbl[DUMP_RDDPM],
	&db7f2010b_dmptbl[DUMP_RDDSM],
	&db7f2010b_dmptbl[DUMP_DSI_ERR],
	&db7f2010b_dmptbl[DUMP_SELF_DIAG],
	&db7f2010b_dmptbl[DUMP_SELF_MASK_CHECKSUM],
	&KEYINFO(a26x_level3_key_disable),
	&KEYINFO(a26x_level2_key_disable),
	&KEYINFO(a26x_level1_key_disable),
};


static void *a26x_check_condition_cmdtbl[] = {
	&KEYINFO(a26x_level1_key_enable),
	&KEYINFO(a26x_level2_key_enable),
	&KEYINFO(a26x_level3_key_enable),
	&db7f2010b_dmptbl[DUMP_RDDPM],
	&KEYINFO(a26x_level3_key_disable),
	&KEYINFO(a26x_level2_key_disable),
	&KEYINFO(a26x_level1_key_disable),
};

#ifdef CONFIG_USDM_PANEL_MASK_LAYER
static void *a26x_mask_layer_workaround_cmdtbl[] = {
	&KEYINFO(a26x_level1_key_enable),
	&KEYINFO(a26x_level2_key_enable),
	&KEYINFO(a26x_level3_key_enable),
	&PKTINFO(a26x_wrdisbv),
	&PKTINFO(a26x_hbm_transition),
	&KEYINFO(a26x_level3_key_disable),
	&KEYINFO(a26x_level2_key_disable),
	&KEYINFO(a26x_level1_key_disable),
	&DLYINFO(a26x_wait_2_vsync),
};

static void *a26x_mask_layer_enter_br_cmdtbl[] = {
	&DLYINFO(a26x_wait_1_vsync),
	/* VFP start */
	&DLYINFO(a26x_wait_1msec),
	/* VFP CMD TX ALLOW END */

	&KEYINFO(a26x_level1_key_enable),
	&KEYINFO(a26x_level2_key_enable),
	&KEYINFO(a26x_level3_key_enable),
	/* BL CMD */
	&SEQINFO(a26x_set_bl_param_seq),
	&KEYINFO(a26x_level3_key_disable),
	&KEYINFO(a26x_level2_key_disable),
	&KEYINFO(a26x_level1_key_disable),

	/* VFP start */
	/* CMD will be flushed here */
	&CONDINFO_IF(a26x_cond_is_90hz),
		&DLYINFO(a26x_wait_1msec),
	&CONDINFO_FI(a26x_cond_is_90hz),

	&CONDINFO_IF(a26x_cond_is_60hz),
		&DLYINFO(a26x_wait_6msec),
	&CONDINFO_FI(a26x_cond_is_60hz),
	/* VSYNC start */
	/* DECON update for next frame */
};

static void *a26x_mask_layer_exit_br_cmdtbl[] = {
	&KEYINFO(a26x_level1_key_enable),
	&KEYINFO(a26x_level2_key_enable),
	&KEYINFO(a26x_level3_key_enable),
	/* BL CMD */
	&SEQINFO(a26x_set_bl_param_seq),
	&KEYINFO(a26x_level3_key_disable),
	&KEYINFO(a26x_level2_key_disable),
	&KEYINFO(a26x_level1_key_disable),

	/* VFP start */
	/* CMD will be flushed here */
	&CONDINFO_IF(a26x_cond_is_90hz),
		&DLYINFO(a26x_wait_1msec),
	&CONDINFO_FI(a26x_cond_is_90hz),

	&CONDINFO_IF(a26x_cond_is_60hz),
		&DLYINFO(a26x_wait_6msec),
	&CONDINFO_FI(a26x_cond_is_60hz),
	/* VSYNC start */
	/* DECON update for next frame */
};
#endif

#ifdef CONFIG_USDM_FACTORY_CCD_TEST
static void *a26x_ccd_test_cmdtbl[] = {
	&PKTINFO(a26x_key_f1_enable),
	&KEYINFO(a26x_level3_key_enable),
	&PKTINFO(a26x_ccd_test_enable),
	&DLYINFO(a26x_wait_17msec),
	&db7f2010b_restbl[RES_CCD_STATE],
	&PKTINFO(a26x_ccd_test_disable),
	&PKTINFO(a26x_key_f1_disable),
	&KEYINFO(a26x_level3_key_disable),
};
#endif

static void *a26x_dummy_cmdtbl[] = {
	NULL,
};

static struct seqinfo a26x_seqtbl[] = {
	SEQINFO_INIT(PANEL_INIT_SEQ, a26x_init_cmdtbl),
	SEQINFO_INIT(PANEL_RES_INIT_SEQ, a26x_res_init_cmdtbl),
	SEQINFO_INIT(PANEL_SET_BL_SEQ, a26x_set_bl_cmdtbl),
	SEQINFO_INIT(PANEL_DISPLAY_ON_SEQ, a26x_display_on_cmdtbl),
	SEQINFO_INIT(PANEL_DISPLAY_OFF_SEQ, a26x_display_off_cmdtbl),
	SEQINFO_INIT(PANEL_EXIT_SEQ, a26x_exit_cmdtbl),
	SEQINFO_INIT(PANEL_DISPLAY_MODE_SEQ, a26x_set_fps_cmdtbl),
#ifdef CONFIG_USDM_PANEL_MASK_LAYER
	SEQINFO_INIT(PANEL_MASK_LAYER_STOP_DIMMING_SEQ, a26x_mask_layer_workaround_cmdtbl),
	SEQINFO_INIT(PANEL_MASK_LAYER_ENTER_BR_SEQ, a26x_mask_layer_enter_br_cmdtbl),
	SEQINFO_INIT(PANEL_MASK_LAYER_EXIT_BR_SEQ, a26x_mask_layer_exit_br_cmdtbl),
#endif
#ifdef CONFIG_USDM_FACTORY_CCD_TEST
	SEQINFO_INIT(PANEL_CCD_TEST_SEQ, a26x_ccd_test_cmdtbl),
#endif
	SEQINFO_INIT(PANEL_DUMP_SEQ, a26x_dump_cmdtbl),
	SEQINFO_INIT(PANEL_CHECK_CONDITION_SEQ, a26x_check_condition_cmdtbl),
	SEQINFO_INIT(PANEL_DUMMY_SEQ, a26x_dummy_cmdtbl),
};

struct common_panel_info db7f2010b_a26x_panel_info = {
	.ldi_name = "db7f2010b",
	.name = "db7f2010b_a26x",
	.model = "AMS666HJ01",
	.vendor = "SDC",
	.id = 0x800040,
	.rev = 0,
	.ddi_props = {
		.gpara = (DDI_SUPPORT_WRITE_GPARA |
				DDI_SUPPORT_READ_GPARA | DDI_SUPPORT_POINT_GPARA | DDI_SUPPORT_2BYTE_GPARA),
		.err_fg_recovery = false,
		.support_vrr = true,
		.mdnie_enable_in_init_seq = true, /* To avoid cmd tx delay after video stream, tx cmd before starting video stream */
		.img_fifo_size = 129, /* 128 + 1 */
		.dft_osc_freq = 96500,
		/* Todo the hs_clk must be synchronized with the value of LK,
		 * It must be changed to be read and set in the LK.
		 */
		.dft_dsi_freq = 898000,
#if !defined(CONFIG_USDM_FACTORY)	
		.no_set_pcd_bypass = true,
#endif
	},
	.ddi_ops = {
		.get_cell_id = db7f2010b_get_cell_id,
		.get_octa_id = db7f2010b_get_octa_id,
		.get_manufacture_code = db7f2010b_get_manufacture_code,
		.get_manufacture_date = db7f2010b_get_manufacture_date,
	},

#if defined(CONFIG_USDM_PANEL_DISPLAY_MODE)
	.common_panel_modes = &db7f2010b_a26x_display_modes,
#endif
	.mres = {
		.nr_resol = ARRAY_SIZE(db7f2010b_a26x_default_resol),
		.resol = db7f2010b_a26x_default_resol,
	},
	.vrrtbl = db7f2010b_a26x_default_vrrtbl,
	.nr_vrrtbl = ARRAY_SIZE(db7f2010b_a26x_default_vrrtbl),
	.maptbl = a26x_maptbl,
	.nr_maptbl = ARRAY_SIZE(a26x_maptbl),
	.seqtbl = a26x_seqtbl,
	.nr_seqtbl = ARRAY_SIZE(a26x_seqtbl),
	.rditbl = db7f2010b_rditbl,
	.nr_rditbl = ARRAY_SIZE(db7f2010b_rditbl),
	.restbl = db7f2010b_restbl,
	.nr_restbl = ARRAY_SIZE(db7f2010b_restbl),
	.dumpinfo = db7f2010b_dmptbl,
	.nr_dumpinfo = ARRAY_SIZE(db7f2010b_dmptbl),

#ifdef CONFIG_USDM_MDNIE
	.mdnie_tune = &db7f2010b_a26x_mdnie_tune,
#endif
	.panel_dim_info = {
		[PANEL_BL_SUBDEV_TYPE_DISP] = &db7f2010b_a26x_panel_dimming_info,
	},
#ifdef CONFIG_USDM_PANEL_SELF_DISPLAY
	.aod_tune = &db7f2010b_a26x_aod,
#endif
};

#endif /* __DB7F2010B_A26X_PANEL_H__ */
