/*
 *
 * FocalTech TouchScreen driver.
 *
 * Copyright (c) 2012-2020, FocalTech Systems, Ltd., all rights reserved.
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
/*****************************************************************************
*
* File Name: focaltech_core.c
*
* Author: Focaltech Driver Team
*
* Created: 2016-08-08
*
* Abstract: entrance for focaltech ts driver
*
* Version: V1.0
*
*****************************************************************************/

/*****************************************************************************
* Included header files
*****************************************************************************/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include "focaltech_core.h"
#if IS_ENABLED(CONFIG_SAMSUNG_TUI)
#include <linux/input/stui_inf.h>
#endif

/*****************************************************************************
* Private constant and macro definitions using #define
*****************************************************************************/
#define FTS_DRIVER_NAME                     "focaltech_ts"
#define FTS_DRIVER_PEN_NAME                 "focaltech_ts,pen"
#define INTERVAL_READ_REG                   200  /* unit:ms */
#define TIMEOUT_READ_REG                    1000 /* unit:ms */
#if FTS_POWER_SOURCE_CUST_EN
#define FTS_VTG_MIN_UV                      2800000
#define FTS_VTG_MAX_UV                      3300000
#define FTS_I2C_VTG_MIN_UV                  1800000
#define FTS_I2C_VTG_MAX_UV                  1800000
#endif

/*****************************************************************************
* Global variable or extern global variabls/functions
*****************************************************************************/
struct fts_ts_data *fts_data;
void fts_irq_enable(void);
void fts_irq_disable(void);
void fts_release_all_finger(void);

#if IS_ENABLED(CONFIG_SAMSUNG_TUI)
extern int stui_i2c_lock(struct i2c_adapter *adap);
extern int stui_i2c_unlock(struct i2c_adapter *adap);

static int fts_stui_tsp_enter(void)
{
	int ret = 0;
	struct fts_ts_data *ts = fts_data;

	FTS_INFO("[STUI] %s\n", __func__);
	if (!ts)
		return -EINVAL;

	fts_irq_disable();
	fts_release_all_finger();

	ret = stui_i2c_lock(ts->client->adapter);
	if (ret) {
		pr_err("[STUI] stui_i2c_lock failed : %d\n", ret);
		fts_irq_enable();
		return -1;
	}

	return 0;
}

static int fts_stui_tsp_exit(void)
{
	int ret = 0;
	struct fts_ts_data *ts = fts_data;

	if (!ts)
		return -EINVAL;

	ret = stui_i2c_unlock(ts->client->adapter);
	if (ret)
		pr_err("[STUI] stui_i2c_unlock failed : %d\n", ret);

	fts_irq_enable();

	return ret;
}

static int fts_stui_tsp_type(void)
{
	return STUI_TSP_TYPE_FOCALTECH;
}
#endif

int fts_check_cid(struct fts_ts_data *ts_data, u8 id_h)
{
	int i = 0;
	struct ft_chip_id_t *cid = &ts_data->ic_info.cid;
	u8 cid_h = 0x0;

	if (cid->type == 0)
		return -ENODATA;

	for (i = 0; i < FTS_MAX_CHIP_IDS; i++) {
		cid_h = ((cid->chip_ids[i] >> 8) & 0x00FF);
		if (cid_h && (id_h == cid_h)) {
			return 0;
		}
	}

	return -ENODATA;
}

/*****************************************************************************
*  Name: fts_wait_tp_to_valid
*  Brief: Read chip id until TP FW become valid(Timeout: TIMEOUT_READ_REG),
*         need call when reset/power on/resume...
*  Input:
*  Output:
*  Return: return 0 if tp valid, otherwise return error code
*****************************************************************************/
int fts_wait_tp_to_valid(void)
{
	int ret = 0;
	int cnt = 0;
	u8 idh = 0;
	struct fts_ts_data *ts_data = fts_data;
	u8 chip_idh = ts_data->ic_info.ids.chip_idh;

	FTS_FUNC_ENTER();
	do {
		ret = fts_read_reg(FTS_REG_CHIP_ID, &idh);
		if ((idh == chip_idh) || (fts_check_cid(ts_data, idh) == 0)) {
			FTS_INFO("TP Ready,Device ID:0x%02x", idh);
			return 0;
		} else
			FTS_DEBUG("TP Not Ready,ReadData:0x%02x,ret:%d", idh, ret);

		cnt++;
		sec_delay(INTERVAL_READ_REG);
	} while ((cnt * INTERVAL_READ_REG) < TIMEOUT_READ_REG);
	FTS_FUNC_EXIT();

	return -EIO;
}

/*****************************************************************************
*  Name: fts_tp_state_recovery
*  Brief: Need execute this function when reset
*  Input:
*  Output:
*  Return:
*****************************************************************************/
void fts_tp_state_recovery(struct fts_ts_data *ts_data)
{
	FTS_FUNC_ENTER();
	/* wait tp stable */
	fts_wait_tp_to_valid();
	/* recover TP charger state 0x8B */
	/* recover TP glove state 0xC0 */
	/* recover TP cover state 0xC1 */
	fts_ex_mode_recovery(ts_data);
	/* recover TP gesture state 0xD0 */
	fts_gesture_recovery(ts_data);
	FTS_FUNC_EXIT();
}

int fts_reset_proc(int hdelayms)
{
	int ret = 0;
	FTS_DEBUG("tp reset");
	ret = fts_write_reg(0xFC, 0xAA);
	if (ret < 0) {
		FTS_ERROR("write FC AA fails");
	}

	ret = fts_write_reg(0xFC, 0x66);
	if (ret < 0) {
		FTS_ERROR("write FC 66 fails");
	}
	if (hdelayms) {
		sec_delay(hdelayms);
	}

	return 0;
}

void fts_irq_disable(void)
{
	mutex_lock(&fts_data->irq_lock);
	if (!fts_data->irq_disabled) {
		FTS_FUNC_ENTER();
		disable_irq_nosync(fts_data->irq);
		fts_data->irq_disabled = true;
	}
	mutex_unlock(&fts_data->irq_lock);
}

void fts_irq_enable(void)
{
	mutex_lock(&fts_data->irq_lock);
	if (fts_data->irq_disabled) {
		FTS_FUNC_ENTER();
		enable_irq(fts_data->irq);
		fts_data->irq_disabled = false;
	}
	mutex_unlock(&fts_data->irq_lock);
}

void fts_hid2std(void)
{
	int ret = 0;
	u8 buf[3] = {0xEB, 0xAA, 0x09};

	if (fts_data->bus_type != BUS_TYPE_I2C)
		return;

	ret = fts_write(buf, 3);
	if (ret < 0) {
		FTS_ERROR("hid2std cmd write fail");
	} else {
		sec_delay(10);
		buf[0] = buf[1] = buf[2] = 0;
		ret = fts_read(NULL, 0, buf, 3);
		if (ret < 0)
			FTS_ERROR("hid2std cmd read fail");
		else if ((buf[0] == 0xEB) && (buf[1] == 0xAA) && (buf[2] == 0x08))
			FTS_DEBUG("hidi2c change to stdi2c successful");
		else
			FTS_DEBUG("hidi2c change to stdi2c not support or fail");
	}
}

static int fts_get_chip_types(struct fts_ts_data *ts_data, u8 id_h, u8 id_l, bool fw_valid)
{
	u32 i = 0;
	struct ft_chip_t ctype[] = FTS_CHIP_TYPE_MAPPING;
	u32 ctype_entries = sizeof(ctype) / sizeof(struct ft_chip_t);

	FTS_DEBUG("verify id:0x%02x%02x", id_h, id_l);
	for (i = 0; i < ctype_entries; i++) {
		if (VALID == fw_valid) {
			if ((id_h == ctype[i].chip_idh) && (id_l == ctype[i].chip_idl))
				break;
		} else {
			if (((id_h == ctype[i].rom_idh) && (id_l == ctype[i].rom_idl))
				|| ((id_h == ctype[i].pb_idh) && (id_l == ctype[i].pb_idl))
				|| ((id_h == ctype[i].bl_idh) && (id_l == ctype[i].bl_idl))) {
				break;
			}
		}
	}

	if (i == ctype_entries)
		return -EINVAL;

	ts_data->ic_info.ids = ctype[i];
	return 0;
}

static int fts_read_bootid(struct fts_ts_data *ts_data, u8 *id)
{
	int ret = 0;
	u8 chip_id[2] = { 0 };
	u8 id_cmd[4] = { 0 };
	u32 id_cmd_len = 0;

	id_cmd[0] = FTS_CMD_START1;
	id_cmd[1] = FTS_CMD_START2;
	ret = fts_write(id_cmd, 2);
	if (ret < 0) {
		FTS_ERROR("start cmd write fail");
		return ret;
	}

	sec_delay(FTS_CMD_START_DELAY);
	id_cmd[0] = FTS_CMD_READ_ID;
	id_cmd[1] = id_cmd[2] = id_cmd[3] = 0x00;
	if (ts_data->ic_info.is_incell)
		id_cmd_len = FTS_CMD_READ_ID_LEN_INCELL;
	else
		id_cmd_len = FTS_CMD_READ_ID_LEN;
	ret = fts_read(id_cmd, id_cmd_len, chip_id, 2);
	if ((ret < 0) || (chip_id[0] == 0x0) || (chip_id[1] == 0x0)) {
		FTS_ERROR("read boot id fail,read:0x%02x%02x", chip_id[0], chip_id[1]);
		return -EIO;
	}

	id[0] = chip_id[0];
	id[1] = chip_id[1];
	return 0;
}

/*****************************************************************************
* Name: fts_get_ic_information
* Brief: read chip id to get ic information, after run the function, driver w-
*        ill know which IC is it.
*        If cant get the ic information, maybe not focaltech's touch IC, need
*        unregister the driver
* Input:
* Output:
* Return: return 0 if get correct ic information, otherwise return error code
*****************************************************************************/
static int fts_get_ic_information(struct fts_ts_data *ts_data)
{
	int ret = 0;
	int cnt = 0;
	u8 chip_id[2] = { 0 };
	u8 rbuff = 0, rbuff2 = 0;

	ts_data->ic_info.is_incell = FTS_CHIP_IDC;
	ts_data->ic_info.hid_supported = FTS_HID_SUPPORTTED;

	do {
		ret = fts_read_reg(FTS_REG_CHIP_ID, &chip_id[0]);
		if (ret < 0) {
			FTS_DEBUG("failed to read chip id");
			goto RETRY;
		}

		ret = fts_read_reg(FTS_REG_CHIP_ID2, &chip_id[1]);
		if (ret < 0) {
			FTS_DEBUG("failed to read chip id2");
			goto RETRY;
		}

		if ((chip_id[0] == 0x0) || (chip_id[1] == 0x0)) {
			FTS_DEBUG("chip id read invalid, read:0x%02x%02x",
				chip_id[0], chip_id[1]);
		} else {
			ret = fts_get_chip_types(ts_data, chip_id[0], chip_id[1], VALID);
			if (!ret)
				break;
			else
				FTS_DEBUG("TP not ready, read:0x%02x%02x",
					chip_id[0], chip_id[1]);
		}
RETRY:
		cnt++;
		sec_delay(INTERVAL_READ_REG);
	} while ((cnt * INTERVAL_READ_REG) < TIMEOUT_READ_REG);

	if ((cnt * INTERVAL_READ_REG) >= TIMEOUT_READ_REG) {
		FTS_INFO("fw is invalid, need read boot id");
		if (ts_data->ic_info.hid_supported)
			fts_hid2std();

		ret = fts_read_bootid(ts_data, &chip_id[0]);
		if (ret <  0) {
			FTS_ERROR("read boot id fail");
			return ret;
		}

		ret = fts_get_chip_types(ts_data, chip_id[0], chip_id[1], INVALID);
		if (ret < 0) {
			FTS_ERROR("can't get ic informaton");
			return ret;
		}
	}

	ret = fts_write_reg(FTS_REG_BANK_MODE, 0);
	if (ret < 0)
		FTS_ERROR("failed to write bank mode 0, ret=%d", ret);

	ret = fts_read_reg(FTS_REG_SEC_CHIP_NAME_H, &rbuff);
	if (ret < 0)
		FTS_ERROR("failed to read CHIP NAME H, ret=%d", ret);

	ret = fts_read_reg(FTS_REG_SEC_CHIP_NAME_L, &rbuff2);
	if (ret < 0)
		FTS_ERROR("failed to read CHIP NAME L, ret=%d", ret);

	ts_data->ic_name = rbuff << 8 | rbuff2;

	FTS_INFO("get ic information, chip id = 0x%02x%02x(cid type=0x%x), IC : FT%04X",
				ts_data->ic_info.ids.chip_idh, ts_data->ic_info.ids.chip_idl,
				ts_data->ic_info.cid.type, ts_data->ic_name);

	return 0;
}

/*****************************************************************************
*  Reprot related
*****************************************************************************/
static void fts_show_touch_buffer(u8 *data, int datalen)
{
	int i = 0;
	int count = 0;
	char *tmpbuf = NULL;

	tmpbuf = kzalloc(1024, GFP_KERNEL);
	if (!tmpbuf) {
		FTS_ERROR("tmpbuf zalloc fail");
		return;
	}

	for (i = 0; i < datalen; i++) {
		count += snprintf(tmpbuf + count, 1024 - count, "%02X,", data[i]);
		if (count >= 1024)
			break;
	}
	FTS_DEBUG("point buffer:%s", tmpbuf);

	if (tmpbuf) {
		kfree(tmpbuf);
		tmpbuf = NULL;
	}
}

void fts_release_all_finger(void)
{
	sec_input_release_all_finger(fts_data->dev);
}

/*****************************************************************************
* Name: fts_input_report_key
* Brief: process key events,need report key-event if key enable.
*        if point's coordinate is in (x_dim-50,y_dim-50) ~ (x_dim+50,y_dim+50),
*        need report it to key event.
*        x_dim: parse from dts, means key x_coordinate, dimension:+-50
*        y_dim: parse from dts, means key y_coordinate, dimension:+-50
* Input:
* Output:
* Return: return 0 if it's key event, otherwise return error code
*****************************************************************************/
/*static int fts_input_report_key(struct fts_ts_data *data, int index)
{
	int i = 0;
	int x = data->events[index].x;
	int y = data->events[index].y;
	int *x_dim = &data->key_x_coords[0];
	int *y_dim = &data->key_y_coords[0];

	if (!data->have_key) {
		return -EINVAL;
	}
	for (i = 0; i < data->key_number; i++) {
		if ((x >= x_dim[i] - FTS_KEY_DIM) && (x <= x_dim[i] + FTS_KEY_DIM) &&
		    (y >= y_dim[i] - FTS_KEY_DIM) && (y <= y_dim[i] + FTS_KEY_DIM)) {
			if (EVENT_DOWN(data->events[index].flag)
			    && !(data->key_state & (1 << i))) {
				input_report_key(data->input_dev, data->keys[i], 1);
				data->key_state |= (1 << i);
				FTS_DEBUG("Key%d(%d,%d) DOWN!", i, x, y);
			} else if (EVENT_UP(data->events[index].flag)
			           && (data->key_state & (1 << i))) {
				input_report_key(data->input_dev, data->keys[i], 0);
				data->key_state &= ~(1 << i);
				FTS_DEBUG("Key%d(%d,%d) Up!", i, x, y);
			}
			return 0;
		}
	}
	return -EINVAL;
}

static void location_detect(struct fts_ts_data *ts, char *loc, int x, int y)
{
	memset(loc, 0, SEC_TS_LOCATION_DETECT_SIZE);

	if (ts->pdata) {
		if (x < ts->pdata->area_edge)
			strncat(loc, "E.", 2);
		else if (x < (ts->pdata->max_x - ts->pdata->area_edge))
			strncat(loc, "C.", 2);
		else
			strncat(loc, "e.", 2);

		if (y < ts->pdata->area_indicator)
			strncat(loc, "S", 1);
		else if (y < (ts->pdata->max_y - ts->pdata->area_navigation))
			strncat(loc, "C", 1);
		else
			strncat(loc, "N", 1);
	}
}*/

#if FTS_PEN_EN
static int fts_input_pen_report(struct fts_ts_data *data)
{
	struct input_dev *pen_dev = data->pen_dev;
	struct pen_event *pevt = &data->pevent;
	u8 *buf = data->point_buf;


	if (buf[3] & 0x08)
		input_report_key(pen_dev, BTN_STYLUS, 1);
	else
		input_report_key(pen_dev, BTN_STYLUS, 0);

	if (buf[3] & 0x02)
		input_report_key(pen_dev, BTN_STYLUS2, 1);
	else
		input_report_key(pen_dev, BTN_STYLUS2, 0);

	pevt->inrange = (buf[3] & 0x20) ? 1 : 0;
	pevt->tip = (buf[3] & 0x01) ? 1 : 0;
	pevt->x = ((buf[4] & 0x0F) << 8) + buf[5];
	pevt->y = ((buf[6] & 0x0F) << 8) + buf[7];
	pevt->p = ((buf[8] & 0x0F) << 8) + buf[9];
	pevt->id = buf[6] >> 4;
	pevt->flag = buf[4] >> 6;
	pevt->tilt_x = (buf[10] << 8) + buf[11];
	pevt->tilt_y = (buf[12] << 8) + buf[13];
	pevt->tool_type = BTN_TOOL_PEN;

	if (data->log_level >= 2  ||
	    ((1 == data->log_level) && (FTS_TOUCH_DOWN == pevt->flag))) {
		FTS_DEBUG("[PEN]x:%d,y:%d,p:%d,inrange:%d,tip:%d,flag:%d DOWN",
		          pevt->x, pevt->y, pevt->p, pevt->inrange,
		          pevt->tip, pevt->flag);
	}

	if ( (data->log_level >= 1) && (!pevt->inrange)) {
		FTS_DEBUG("[PEN]UP");
	}

	input_report_abs(pen_dev, ABS_X, pevt->x);
	input_report_abs(pen_dev, ABS_Y, pevt->y);
	input_report_abs(pen_dev, ABS_PRESSURE, pevt->p);

	/* check if the pen support tilt event */
	if ((pevt->tilt_x != 0) || (pevt->tilt_y != 0)) {
		input_report_abs(pen_dev, ABS_TILT_X, pevt->tilt_x);
		input_report_abs(pen_dev, ABS_TILT_Y, pevt->tilt_y);
	}

	input_report_key(pen_dev, BTN_TOUCH, pevt->tip);
	input_report_key(pen_dev, BTN_TOOL_PEN, pevt->inrange);
	input_sync(pen_dev);

	return 0;
}
#endif

#define OUT_POCKET 10
#define IN_POCKET 11
static int fts_read_pocket_result(struct fts_ts_data *ts_data)
{
	int ret = 0;
	u8 val = 0;

	ret = fts_read_reg(POCKET_DATA_REG, &val);
	if (ret < 0) {
		FTS_ERROR("read pocket data fail");
		return ret;
	}

	if (ts_data->pocket_state == val)
		return 0;
	else
		ts_data->pocket_state = val;

	if (val == 0) {
		FTS_INFO("pocket status out");
		ts_data->hover_event = OUT_POCKET;
	} else if (val == 1) {
		FTS_INFO("pocket status in");
		ts_data->hover_event = IN_POCKET;
	}

	sec_input_proximity_report(ts_data->dev, ts_data->hover_event);

	return 0;
}

static int fts_read_proximity_result(struct fts_ts_data *ts_data)
{
	int ret = 0;
	u8 val = 0;

	ret = fts_read_reg(PROXIMITY_DATA_REG, &val);
	if (ret < 0) {
		FTS_ERROR("read pocket data fail");
		return ret;
	}

	if (ts_data->hover_event == (val >> 4))
		return 0;
	else
		ts_data->hover_event = (val >> 4);

	sec_input_proximity_report(ts_data->dev, ts_data->hover_event);

	return 0;
}

int fts_read_fod_data(struct fts_ts_data *ts_data)
{
	int ret = 0;
	u8 val = 0;
	u8 data[6] = { 0 };

	ret = fts_read_reg(FOD_TX_NUM, &val);
	if (ret < 0) {
		FTS_ERROR("read fod tx num fail");
		return ret;
	}
	FTS_INFO("fod tx num %d", val);
	data[0] = val;

	ret = fts_read_reg(FOD_RX_NUM, &val);
	if (ret < 0) {
		FTS_ERROR("read fod rx num fail");
		return ret;
	}
	FTS_INFO("fod rx num %d", val);
	data[1] = val;

	ret = fts_read_reg(FOD_VI_SIZE, &val);
	if (ret < 0) {
		FTS_ERROR("read fod vi size fail");
		return ret;
	}
	FTS_INFO("fod vi size %d", val);
	data[2] = val;

	sec_input_set_fod_info(ts_data->dev, data[0], data[1], data[2], data[3]);

	return ret;
}

int fts_set_fod_rect(struct fts_ts_data *ts_data)
{
	u8 data[10] = { 0 };
	int ret, i;

	FTS_INFO("l:%d, t:%d, r:%d, b:%d",
		ts_data->pdata->fod_data.rect_data[0], ts_data->pdata->fod_data.rect_data[1],
		ts_data->pdata->fod_data.rect_data[2], ts_data->pdata->fod_data.rect_data[3]);

	for (i = 0; i < 4; i++) {
		data[i * 2 + 1] = (ts_data->pdata->fod_data.rect_data[i] >> 8) & 0xFF;
		data[i * 2 + 2] = ts_data->pdata->fod_data.rect_data[i] & 0xFF;
	}

	data[0] = FTS_REG_FOD_RECT;
	ret = fts_write(data, 9);
	if (ret < 0)
		FTS_ERROR("Failed to write");

	return ret;
}

static int fts_read_fod_result(struct fts_ts_data *ts_data)
{
	int ret = 0;
	u8 val = 0;
	u8 fod_coordinate[4] = {0};
	int x, y;

	ret = fts_read_reg(FOD_EVENT_REG, &val);
	if (ret < 0) {
		FTS_ERROR("read fod data fail");
		return ret;
	}

	if (ts_data->fod_state == val)
		return 0;
	else
		ts_data->fod_state = val;

	fod_coordinate[0] = 0xE7;
	ret = fts_read(fod_coordinate, 1, fod_coordinate, 4);
	if (ret < 0) {
		FTS_ERROR("read fod coordinate fail");
		return ret;
	}

	x = (fod_coordinate[0] << 8) + fod_coordinate[1];
	y = (fod_coordinate[2] << 8) + fod_coordinate[3];

	// #0 : long press, #1 : press, #2 : release, #3 : out of area, 0xff : no event
	if (val == 0 || val == 1) {
		sec_cmd_send_gesture_uevent(&ts_data->sec, SPONGE_EVENT_TYPE_FOD_PRESS, x, y);
		FTS_INFO("FOD %s PRESS", val ? "" : "LONG");
	} else if (val == 2) {
		sec_cmd_send_gesture_uevent(&ts_data->sec, SPONGE_EVENT_TYPE_FOD_RELEASE, x, y);
	} else if (val == 3) {
		sec_cmd_send_gesture_uevent(&ts_data->sec, SPONGE_EVENT_TYPE_FOD_OUT, x, y);
	} else if (val == 0xff) {
		FTS_INFO("fod has no event");
	}

	return 0;
}

static int fts_read_touchdata(struct fts_ts_data *data)
{
	int ret = 0;
	int size = 0;
	u8 *buf = data->point_buf;
	int event_num = 0;

	memset(buf, 0xFF, data->pnt_buf_size);
	buf[0] = 0x01;

	if (atomic_read(&data->pdata->power_state) == SEC_INPUT_STATE_LPM) {
		if (0 == fts_gesture_readdata(data, NULL)) {
			FTS_INFO("succuss to get gesture data in irq handler");
			return 1;
		}
	}

	if (data->support_high_report) {
		size = FTS_ONE_TCH_LEN * 2 + IRQ_EVENT_HEAD_LEN;

		ret = fts_read(buf, 1, buf + 1, size);
		if (ret < 0) {
			FTS_ERROR("read touchdata failed, ret:%d", ret);
			return ret;
		}
		event_num = buf[1] & 0x0F;
//	   	FTS_INFO("event_num = %d", event_num);
		if (unlikely(event_num > 2)) {
			buf[0] += size;
		        ret = fts_read(buf, 1, buf + 1 + size, (event_num - 2) * FTS_ONE_TCH_LEN);
			if (ret < 0) {
				FTS_ERROR("read touchdata2 failed, ret:%d", ret);
				return ret;
			}
		}
	} else {
		size = data->pnt_buf_size - 1;
		ret = fts_read(buf, 1, buf + 1, size);
		if (ret < 0) {
			FTS_ERROR("read touchdata failed, ret:%d", ret);
			return ret;
		}
	}

	if (data->log_level >= 3) {
		fts_show_touch_buffer(buf, data->pnt_buf_size);
	}

	return 0;
}

static void fts_ts_report_finger(struct fts_ts_data *ts_data, unsigned int tid)
{
	int i;
	int recal_tc = 0;
	static int prev_tc;
	int max_touch_num = ts_data->max_touch_number;

	sec_input_coord_event_fill_slot(ts_data->dev, tid);

	//FTS_DEBUG("press_cnt: %d", ts_data->pdata->touch_count);

//	for checking total touch count
	for (i = 0; i < max_touch_num; i++) {
		if (ts_data->pdata->coord[i].action == SEC_TS_COORDINATE_ACTION_PRESS ||
				ts_data->pdata->coord[i].action == SEC_TS_COORDINATE_ACTION_MOVE) {
			recal_tc++;
		}
	}

	if (recal_tc != ts_data->pdata->touch_count && prev_tc != ts_data->pdata->touch_count)
		FTS_ERROR("recal_tc:%d != tc:%d", recal_tc, ts_data->pdata->touch_count);

	prev_tc = ts_data->pdata->touch_count;
}


static void fts_parse_finger(struct fts_ts_data *ts_data, unsigned int tid, u8 *buf)
{
	struct sec_ts_plat_data *pdata = ts_data->pdata;
	u8 touch_type = (buf[7] >> 6) + ((buf[6] & 0xC0) >> 4);
	bool wet_mode = 0;

	//FTS_DEBUG("tid:%d, touch_type:%d", tid, touch_type);
	if (tid >= 10) {
		FTS_ERROR("tid(%d) is too big", tid);
		return;
	}

	wet_mode = (touch_type == 6)? 1 : 0;
	if (wet_mode != ts_data->pdata->wet_mode) {
		ts_data->pdata->wet_mode = wet_mode;

		if (ts_data->pdata->wet_mode) {
			FTS_INFO("Waterproof mode on");
			ts_data->pdata->hw_param.wet_count++;
		} else {
			FTS_INFO("Waterproof mode off");
		}
	}

	pdata->prev_coord[tid] = pdata->coord[tid];
	pdata->coord[tid].id = tid;
	pdata->coord[tid].ttype = touch_type;
	pdata->coord[tid].action = (buf[0] & 0xC0) >> 6;
	pdata->coord[tid].x = buf[1] << 4 | (buf[3] & 0xF0) >> 4;
	pdata->coord[tid].y = buf[2] << 4 | (buf[3] & 0x0F);
	pdata->coord[tid].major = buf[4];
	pdata->coord[tid].minor = buf[5];
	pdata->coord[tid].z = buf[6] & 0x3F;
	pdata->coord[tid].noise_level = max(pdata->coord[tid].noise_level, buf[8]);
	pdata->coord[tid].max_strength = max(pdata->coord[tid].max_strength, buf[9]);
	pdata->coord[tid].noise_status = (u8)((buf[10] >> 4) & 0x03);
	pdata->coord[tid].hover_id_num = max(pdata->coord[tid].hover_id_num, (u8)(buf[10] & 0x0F));
	pdata->coord[tid].freq_id = buf[11] & 0x0F;
	pdata->coord[tid].fod_debug = (buf[11] >> 4) & 0x0F;

	if (!pdata->coord[tid].palm && (pdata->coord[tid].ttype == SEC_TS_TOUCHTYPE_PALM))
		pdata->coord[tid].palm_count++;

	pdata->coord[tid].palm = (touch_type == SEC_TS_TOUCHTYPE_PALM);
	if (pdata->coord[tid].palm)
		pdata->palm_flag |= (1 << tid);
	else
		pdata->palm_flag &= ~(1 << tid);

	if (pdata->coord[tid].z <= 0)
		pdata->coord[tid].z = 1;
}

#define REFRESH_BASE_FLAG 0x10
#define TURN_ON_RAISE_HAND 0x20
#define TURN_ON_REMOVE_WATER 0x40
#define TURN_ON_REMOVE_CONDUCTOR 0x60
#define LP2NP 0x80
#define NP2LP 0xA0

static void fts_base_refresh_reason(u8 base_flag)
{
	if ((base_flag & REFRESH_BASE_FLAG) == 0) return;

	switch(base_flag & 0xE0) {
		case TURN_ON_RAISE_HAND:
			FTS_INFO("TURN_ON_RAISE_HAND cause refresh base %x\n", base_flag);
			break;
		case TURN_ON_REMOVE_WATER:
			FTS_INFO("TURN_ON_REMOVE_WATER cause refresh base %x\n", base_flag);
			break;
		case TURN_ON_REMOVE_CONDUCTOR:
			FTS_INFO("TURN_ON_REMOVE_CONDUCTOR cause refresh base %x\n", base_flag);
			break;
		case LP2NP:
			FTS_INFO("LP2NP cause refresh base %x\n", base_flag);
			break;
		case NP2LP:
			FTS_INFO("NP2LP cause refresh base %x\n", base_flag);
			break;
		default:
			FTS_INFO("unkonwn reason cause refresh base %x\n", base_flag);
			break;
	}

	return;
}

static int fts_read_parse_touchdata(struct fts_ts_data *data)
{
	int ret = 0;
	u8 *buf = data->point_buf;
	u8 eid;
	u8 *event_data = buf + 1 + IRQ_EVENT_HEAD_LEN;
	int event_num;
	int i;

	ret = fts_read_touchdata(data);
	if (ret) {
		return ret;
	}

#if FTS_PEN_EN
	if ((buf[2] & 0xF0) == 0xB0) {
		fts_input_pen_report(data);
		return 2;
	}
#endif

	fts_base_refresh_reason(buf[1]);

	if ((buf[2] == 0xFF) && (buf[3] == 0xFF)
			&& (buf[4] == 0xFF) && (buf[5] == 0xFF) && (buf[6] == 0xFF)) {
		FTS_ERROR("touch buff is 0xff, need recovery state");
		fts_release_all_finger();
		fts_tp_state_recovery(data);
		return -EIO;
	}

	event_num = buf[1] & 0x0F;
	for (i = 0; i < event_num; i++) {
		eid = event_data[0] & 0x03;
		switch (eid) {
		case SEC_TS_COORDINATE_EVENT:
			if (!data->suspended) {
				unsigned int tid = (event_data[0] & 0x3C) >> 2;

				if (tid >= 10) {
					FTS_INFO("invalid touch id = %d", tid);
					break;
				}
				fts_parse_finger(data, tid, event_data);
				fts_ts_report_finger(data, tid);
			}
			break;
		case SEC_TS_STATUS_EVENT:
			break;
		case SEC_TS_GESTURE_EVENT:
			break;
		default:
			FTS_INFO("invalid event id[%x]", eid);
			break;
		}

		event_data += FTS_ONE_TCH_LEN;
	}
	sec_input_coord_event_sync_slot(data->dev);

	return 0;
}

static void fts_irq_read_report(void)
{
	int ret = 0;
	struct fts_ts_data *ts_data = fts_data;

#if FTS_ESDCHECK_EN
	fts_esdcheck_set_intr(1);
#endif

#if FTS_POINT_REPORT_CHECK_EN
	fts_prc_queue_work(ts_data);
#endif

	if (ts_data->pdata->pocket_mode)
		fts_read_pocket_result(ts_data);

	if (ts_data->pdata->ed_enable)
		fts_read_proximity_result(ts_data);

	ret = fts_read_parse_touchdata(ts_data);

	if (ts_data->fod_mode & 0x01)
		fts_read_fod_result(ts_data);

#if FTS_ESDCHECK_EN
	fts_esdcheck_set_intr(0);
#endif
}

#if defined(CONFIG_PM) && FTS_PATCH_COMERR_PM
static void fts_wakeup_source_init(struct fts_ts_data *ts_data)
{
	FTS_FUNC_ENTER();
	ts_data->pdata->sec_ws = wakeup_source_register(NULL, "TSP");
	FTS_FUNC_EXIT();
}

static void fts_wakeup_source_unregister(struct fts_ts_data *ts_data)
{
	if (!ts_data->pdata->sec_ws)
		return;

	FTS_FUNC_ENTER();
	wakeup_source_unregister(ts_data->pdata->sec_ws);
	FTS_FUNC_EXIT();
}
#endif

static void fts_handler_wait_resume_work(struct work_struct *work)
{
	struct fts_ts_data *ts_data = container_of(work, struct fts_ts_data, irq_work);
	struct irq_desc *desc = irq_to_desc(ts_data->irq);
	int ret;

	ret = wait_for_completion_interruptible_timeout(&ts_data->pdata->resume_done,
			msecs_to_jiffies(SEC_TS_WAKE_LOCK_TIME));
	if (ret == 0) {
		FTS_ERROR("LPM: pm resume is not handled");
		goto out;
	}
	if (ret < 0) {
		FTS_ERROR("LPM: -ERESTARTSYS if interrupted, %d", ret);
		goto out;
	}

	if (desc && desc->action && desc->action->thread_fn) {
		FTS_INFO("run irq thread");
		desc->action->thread_fn(ts_data->irq, desc->action->dev_id);
	}
out:
	fts_irq_enable();
}

static irqreturn_t fts_irq_handler(int irq, void *data)
{
	struct fts_ts_data *ts_data = fts_data;
#if defined(CONFIG_PM) && FTS_PATCH_COMERR_PM
	if (ts_data->suspended) {
		__pm_wakeup_event(ts_data->pdata->sec_ws, SEC_TS_WAKE_LOCK_TIME);

		if (!ts_data->pdata->resume_done.done) {
			if (!IS_ERR_OR_NULL(ts_data->irq_workqueue)) {
				FTS_INFO("disable irq and queue waiting work");
				fts_irq_disable();
				queue_work(ts_data->irq_workqueue, &ts_data->irq_work);
			} else {
				FTS_INFO("irq_workqueue not exist");
			}
			return IRQ_HANDLED;
		}
		FTS_INFO("run LPM interrupt handler");
	}
#endif

	fts_irq_read_report();

	return IRQ_HANDLED;
}

static int fts_irq_registration(struct fts_ts_data *ts_data)
{
	int ret = 0;
	struct sec_ts_plat_data *pdata = ts_data->pdata;

	ts_data->irq_workqueue = create_singlethread_workqueue("fts_irq_wq");
	if (!IS_ERR_OR_NULL(ts_data->irq_workqueue)) {
		INIT_WORK(&ts_data->irq_work, fts_handler_wait_resume_work);
		FTS_INFO("set fts_handler_wait_resume_work");
	} else {
		FTS_ERROR("failed to create irq_workqueue, err: %ld", PTR_ERR(ts_data->irq_workqueue));
	}

	ts_data->irq = gpio_to_irq(pdata->irq_gpio);
	FTS_INFO("irq:%d", ts_data->irq);
	ret = request_threaded_irq(ts_data->irq, NULL, fts_irq_handler,
				IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
				FTS_DRIVER_NAME, ts_data);

	return ret;
}

#if FTS_PEN_EN
static int fts_input_pen_init(struct fts_ts_data *ts_data)
{
	int ret = 0;
	struct input_dev *pen_dev;
	struct sec_ts_plat_data *pdata = ts_data->pdata;

	FTS_FUNC_ENTER();
	pen_dev = devm_input_allocate_device(ts_data->dev);
	if (!pen_dev) {
		FTS_ERROR("Failed to allocate memory for input_pen device");
		return -ENOMEM;
	}

	pen_dev->dev.parent = ts_data->dev;
	pen_dev->name = FTS_DRIVER_PEN_NAME;
	pen_dev->evbit[0] |= BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	__set_bit(ABS_X, pen_dev->absbit);
	__set_bit(ABS_Y, pen_dev->absbit);
	__set_bit(BTN_STYLUS, pen_dev->keybit);
	__set_bit(BTN_STYLUS2, pen_dev->keybit);
	__set_bit(BTN_TOUCH, pen_dev->keybit);
	__set_bit(BTN_TOOL_PEN, pen_dev->keybit);
	__set_bit(INPUT_PROP_DIRECT, pen_dev->propbit);
	input_set_abs_params(pen_dev, ABS_X, 0, pdata->max_x, 0, 0);
	input_set_abs_params(pen_dev, ABS_Y, 0, pdata->max_y, 0, 0);
	input_set_abs_params(pen_dev, ABS_PRESSURE, 0, 4096, 0, 0);

	ret = input_register_device(pen_dev);
	if (ret) {
		FTS_ERROR("Input device registration failed");
		return ret;
	}

	ts_data->pen_dev = pen_dev;
	FTS_FUNC_EXIT();
	return 0;
}
#endif

static int fts_report_buffer_init(struct fts_ts_data *ts_data)
{
	ts_data->max_touch_number = FTS_MAX_POINTS_SUPPORT;
	ts_data->pnt_buf_size = FTS_MAX_POINTS_SUPPORT * FTS_ONE_TCH_LEN + 3;

	ts_data->point_buf = (u8 *)devm_kzalloc(ts_data->dev, ts_data->pnt_buf_size + 1, GFP_KERNEL);
	if (!ts_data->point_buf) {
		FTS_ERROR("failed to alloc memory for point buf");
		return -ENOMEM;
	}

	ts_data->events = (struct ts_event *)devm_kzalloc(ts_data->dev, FTS_MAX_POINTS_SUPPORT * sizeof(struct ts_event), GFP_KERNEL);
	if (!ts_data->events) {
		FTS_ERROR("failed to alloc memory for point events");
		return -ENOMEM;
	}

	return 0;
}

#if FTS_POWER_SOURCE_CUST_EN
/*****************************************************************************
* Power Control
*****************************************************************************/
#if FTS_PINCTRL_EN
static int fts_pinctrl_init(struct fts_ts_data *ts)
{
	int ret = 0;

	FTS_FUNC_ENTER();
	ts->pinctrl = devm_pinctrl_get(ts->dev);
	if (IS_ERR_OR_NULL(ts->pinctrl)) {
		FTS_ERROR("Failed to get pinctrl, please check dts");
		ret = PTR_ERR(ts->pinctrl);
		goto err_pinctrl_get;
	}

	ts->pins_active = pinctrl_lookup_state(ts->pinctrl, "pmx_ts_active");
	if (IS_ERR_OR_NULL(ts->pins_active)) {
		FTS_ERROR("Pin state[active] not found");
		ret = PTR_ERR(ts->pins_active);
		goto err_pinctrl_lookup;
	}

	ts->pins_suspend = pinctrl_lookup_state(ts->pinctrl, "pmx_ts_suspend");
	if (IS_ERR_OR_NULL(ts->pins_suspend)) {
		FTS_ERROR("Pin state[suspend] not found");
		ret = PTR_ERR(ts->pins_suspend);
		goto err_pinctrl_lookup;
	}

	ts->pins_release = pinctrl_lookup_state(ts->pinctrl, "pmx_ts_release");
	if (IS_ERR_OR_NULL(ts->pins_release)) {
		FTS_ERROR("Pin state[release] not found");
		ret = PTR_ERR(ts->pins_release);
	}

	FTS_FUNC_EXIT();
	return 0;
err_pinctrl_lookup:
	if (ts->pinctrl) {
		devm_pinctrl_put(ts->pinctrl);
	}
err_pinctrl_get:
	ts->pinctrl = NULL;
	ts->pins_release = NULL;
	ts->pins_suspend = NULL;
	ts->pins_active = NULL;
	return ret;
}

static int fts_pinctrl_select_normal(struct fts_ts_data *ts)
{
	int ret = 0;

	if (ts->pinctrl && ts->pins_active) {
		ret = pinctrl_select_state(ts->pinctrl, ts->pins_active);
		if (ret < 0) {
			FTS_ERROR("Set normal pin state error:%d", ret);
		} else {
			FTS_INFO("success");
		}
	}

	return ret;
}

static int fts_pinctrl_select_suspend(struct fts_ts_data *ts)
{
	int ret = 0;

	if (ts->pinctrl && ts->pins_suspend) {
		ret = pinctrl_select_state(ts->pinctrl, ts->pins_suspend);
		if (ret < 0) {
			FTS_ERROR("Set suspend pin state error:%d", ret);
		} else {
			FTS_INFO("success");
		}
	}

	return ret;
}

static int fts_pinctrl_select_release(struct fts_ts_data *ts)
{
	int ret = 0;

	if (ts->pinctrl) {
		if (IS_ERR_OR_NULL(ts->pins_release)) {
			devm_pinctrl_put(ts->pinctrl);
			ts->pinctrl = NULL;
		} else {
			ret = pinctrl_select_state(ts->pinctrl, ts->pins_release);
			if (ret < 0)
				FTS_ERROR("Set gesture pin state error:%d", ret);
			else
				FTS_INFO("success");
		}
	}

	return ret;
}
#endif /* FTS_PINCTRL_EN */

static int fts_power_source_ctrl(struct fts_ts_data *ts_data, int enable)
{
	int ret = 0;

	if (IS_ERR_OR_NULL(ts_data->vdd)) {
		FTS_ERROR("vdd is invalid");
		return -EINVAL;
	}

	FTS_FUNC_ENTER();
	if (enable) {
		if (ts_data->power_disabled) {
			FTS_DEBUG("regulator enable !");
			ret = regulator_enable(ts_data->vdd);
			if (ret) {
				FTS_ERROR("enable vdd regulator failed,ret=%d", ret);
			}

			if (!IS_ERR_OR_NULL(ts_data->vcc_i2c)) {
				ret = regulator_enable(ts_data->vcc_i2c);
				if (ret) {
					FTS_ERROR("enable vcc_i2c regulator failed,ret=%d", ret);
				}
			}
			ts_data->power_disabled = false;
		}
	} else {
		if (!ts_data->power_disabled) {
			FTS_DEBUG("regulator disable !");
			ret = regulator_disable(ts_data->vdd);
			if (ret) {
				FTS_ERROR("disable vdd regulator failed,ret=%d", ret);
			}
			if (!IS_ERR_OR_NULL(ts_data->vcc_i2c)) {
				ret = regulator_disable(ts_data->vcc_i2c);
				if (ret) {
					FTS_ERROR("disable vcc_i2c regulator failed,ret=%d", ret);
				}
			}
			ts_data->power_disabled = true;
		}
	}

	FTS_FUNC_EXIT();
	return ret;
}

/*****************************************************************************
* Name: fts_power_source_init
* Brief: Init regulator power:vdd/vcc_io(if have), generally, no vcc_io
*        vdd---->vdd-supply in dts, kernel will auto add "-supply" to parse
*        Must be call after fts_gpio_configure() execute,because this function
*        will operate reset-gpio which request gpio in fts_gpio_configure()
* Input:
* Output:
* Return: return 0 if init power successfully, otherwise return error code
*****************************************************************************/
static int fts_power_source_init(struct fts_ts_data *ts_data)
{
	int ret = 0;

	FTS_FUNC_ENTER();
	ts_data->vdd = regulator_get(ts_data->dev, "vdd");
	if (IS_ERR_OR_NULL(ts_data->vdd)) {
		ret = PTR_ERR(ts_data->vdd);
		FTS_ERROR("get vdd regulator failed,ret=%d", ret);
		return ret;
	}

	if (regulator_count_voltages(ts_data->vdd) > 0) {
		ret = regulator_set_voltage(ts_data->vdd, FTS_VTG_MIN_UV,
		                            FTS_VTG_MAX_UV);
		if (ret) {
			FTS_ERROR("vdd regulator set_vtg failed ret=%d", ret);
			regulator_put(ts_data->vdd);
			return ret;
		}
	}

	ts_data->vcc_i2c = regulator_get(ts_data->dev, "vcc_i2c");
	if (!IS_ERR_OR_NULL(ts_data->vcc_i2c)) {
		if (regulator_count_voltages(ts_data->vcc_i2c) > 0) {
			ret = regulator_set_voltage(ts_data->vcc_i2c,
			                            FTS_I2C_VTG_MIN_UV,
			                            FTS_I2C_VTG_MAX_UV);
			if (ret) {
				FTS_ERROR("vcc_i2c regulator set_vtg failed,ret=%d", ret);
				regulator_put(ts_data->vcc_i2c);
			}
		}
	}

#if FTS_PINCTRL_EN
	fts_pinctrl_init(ts_data);
	fts_pinctrl_select_normal(ts_data);
#endif

	ts_data->power_disabled = true;
	ret = fts_power_source_ctrl(ts_data, ENABLE);
	if (ret) {
		FTS_ERROR("fail to enable power(regulator)");
	}

	FTS_FUNC_EXIT();
	return ret;
}

static int fts_power_source_exit(struct fts_ts_data *ts_data)
{
#if FTS_PINCTRL_EN
	fts_pinctrl_select_release(ts_data);
#endif

	fts_power_source_ctrl(ts_data, DISABLE);

	if (!IS_ERR_OR_NULL(ts_data->vdd)) {
		if (regulator_count_voltages(ts_data->vdd) > 0)
			regulator_set_voltage(ts_data->vdd, 0, FTS_VTG_MAX_UV);
		regulator_put(ts_data->vdd);
	}

	if (!IS_ERR_OR_NULL(ts_data->vcc_i2c)) {
		if (regulator_count_voltages(ts_data->vcc_i2c) > 0)
			regulator_set_voltage(ts_data->vcc_i2c, 0, FTS_I2C_VTG_MAX_UV);
		regulator_put(ts_data->vcc_i2c);
	}

	return 0;
}

static int fts_power_source_suspend(struct fts_ts_data *ts_data)
{
	int ret = 0;

#if FTS_PINCTRL_EN
	fts_pinctrl_select_suspend(ts_data);
#endif

	ret = fts_power_source_ctrl(ts_data, DISABLE);
	if (ret < 0) {
		FTS_ERROR("power off fail, ret=%d", ret);
	}

	return ret;
}

static int fts_power_source_resume(struct fts_ts_data *ts_data)
{
	int ret = 0;

#if FTS_PINCTRL_EN
	fts_pinctrl_select_normal(ts_data);
#endif

	ret = fts_power_source_ctrl(ts_data, ENABLE);
	if (ret < 0) {
		FTS_ERROR("power on fail, ret=%d", ret);
	}

	return ret;
}
#else
int fts_power_ctrl(struct device *dev, bool on)
{
	struct sec_ts_plat_data *pdata = dev->platform_data;
	int ret = 0;

	if (pdata->power_enabled == on) {
		FTS_INFO("power_enabled %d", pdata->power_enabled);
		return ret;
	}

	if (on) {
		if (!pdata->not_support_io_ldo) {
			ret = regulator_enable(pdata->dvdd);
			if (ret) {
				FTS_ERROR("Failed to enable iovdd:%d", ret);
				return ret;
			}
		}
		ret = regulator_enable(pdata->avdd);
		if (ret) {
			if (!pdata->not_support_io_ldo) {
				regulator_disable(pdata->dvdd);
			}
			FTS_ERROR("Failed to enable avdd:%d", ret);
			return ret;
		}
	} else {
		if (!pdata->not_support_io_ldo) {
			ret = regulator_disable(pdata->dvdd);
			if (ret)
				FTS_ERROR("Failed to disable iovdd:%d", ret);
		}
		ret = regulator_disable(pdata->avdd);
		if (ret)
			FTS_ERROR("Failed to disable avdd:%d", ret);
	}

	pdata->power_enabled = on;

	if (!pdata->not_support_io_ldo) {
		FTS_INFO("%s: avdd:%s, iovdd:%s", on ? "on" : "off",
				regulator_is_enabled(pdata->avdd) ? "on" : "off",
				regulator_is_enabled(pdata->dvdd) ? "on" : "off");
	} else {
		FTS_INFO("%s: avdd:%s", on ? "on" : "off", regulator_is_enabled(pdata->avdd) ? "on" : "off");
	}
	return ret;
}


int fts_ts_power_on(struct fts_ts_data *ts_data)
{
	int ret = 0;

	FTS_INFO("power on %d", atomic_read(&ts_data->pdata->power_state));
	ts_data->pdata->pinctrl_configure(ts_data->dev, true);

	if (atomic_read(&ts_data->pdata->power_state) == SEC_INPUT_STATE_POWER_ON) {
		FTS_ERROR("already power on");
		return 0;
	}

	ret = ts_data->pdata->power(ts_data->dev, true);
	sec_delay(5);
	if (!ret)
		atomic_set(&ts_data->pdata->power_state, SEC_INPUT_STATE_POWER_ON);
	else
		FTS_ERROR("failed power on, %d", ret);

	return ret;
}

int fts_ts_power_off(struct fts_ts_data *ts_data)
{
	int ret;

	FTS_INFO("power off %d", atomic_read(&ts_data->pdata->power_state));
	if (atomic_read(&ts_data->pdata->power_state) == SEC_INPUT_STATE_POWER_OFF) {
		FTS_ERROR("already power off");
		return 0;
	}

	ret = ts_data->pdata->power(ts_data->dev, false);
	if (!ret)
		atomic_set(&ts_data->pdata->power_state, SEC_INPUT_STATE_POWER_OFF);
	else
		FTS_ERROR("failed power off, %d", ret);

	ts_data->pdata->pinctrl_configure(ts_data->dev, false);

	return ret;
}
#endif /* FTS_POWER_SOURCE_CUST_EN */

static void fts_resume_work(struct work_struct *work)
{
	struct fts_ts_data *ts_data = container_of(work, struct fts_ts_data, resume_work);

	fts_ts_resume(ts_data);
}

static void fts_print_info(struct fts_ts_data *ts)
{
	ts->print_info_cnt_open++;

	if (ts->print_info_cnt_open > 0xfff0)
		ts->print_info_cnt_open = 0;

	if (ts->touchs == 0)
		ts->print_info_cnt_release++;

	FTS_INFO("tc:%ld lp:0x%02X pm:0x%02X gm:0x%02X // v:FT%02X%02X%02X%02X // #%d %d",
		FTS_TOUCH_COUNT(&ts->touchs), ts->pdata->lowpower_mode, ts->power_mode, ts->gesture_mode,
		ts->ic_fw_ver.ic_name, ts->ic_fw_ver.project_name,
		ts->ic_fw_ver.module_id, ts->ic_fw_ver.fw_ver,
		ts->print_info_cnt_open, ts->print_info_cnt_release);
}

static void fts_print_info_work(struct work_struct *work)
{
	struct fts_ts_data *ts = container_of(work, struct fts_ts_data,
			print_info_work.work);

	fts_print_info(ts);

	schedule_delayed_work(&ts->print_info_work, msecs_to_jiffies(TOUCH_PRINT_INFO_DWORK_TIME));
}

static void fts_read_info_work(struct work_struct *work)
{
	struct fts_ts_data *ts_data = container_of(work, struct fts_ts_data,
			read_info_work.work);

#if !IS_ENABLED(CONFIG_SEC_FACTORY)
	fts_run_rawdata_all(&ts_data->sec);
#endif

	if (!atomic_read(&ts_data->pdata->shutdown_called))
		schedule_work(&ts_data->print_info_work.work);
}

int fts_charger_attached(struct device *dev, bool status)
{
	struct fts_ts_data *ts_data = dev_get_drvdata(dev);
	int ret = 0;

	FTS_INFO("%s [START] %s", __func__, status ? "connected" : "disconnected");

	if (status == 1) {
		if (atomic_read(&ts_data->pdata->power_state) == SEC_INPUT_STATE_POWER_OFF) {
			FTS_DEBUG("enter charger mode & ic off");
			ts_data->charger_mode = ENABLE;
		} else {
			FTS_DEBUG("enter charger mode");
			ret = fts_write_reg(FTS_REG_CHARGER_MODE_EN, ENABLE);
			if (ret >= 0)
				ts_data->charger_mode = ENABLE;
		}
	} else if (status == 0) {
		if (atomic_read(&ts_data->pdata->power_state) == SEC_INPUT_STATE_POWER_OFF) {
			FTS_DEBUG("exit charger mode & ic off");
			ts_data->charger_mode = DISABLE;
		} else {
			FTS_DEBUG("exit charger mode");
			ret = fts_write_reg(FTS_REG_CHARGER_MODE_EN, DISABLE);
			if (ret >= 0)
				ts_data->charger_mode = DISABLE;
		}
	} else
		FTS_ERROR("%s [ERROR] Unknown value[%d]", __func__, status);

	FTS_INFO("%s [DONE]", __func__);
	return ret;
}

int fts_ts_enable(struct device *dev)
{
	struct fts_ts_data *ts_data = dev_get_drvdata(dev);

	FTS_INFO("called");
	atomic_set(&ts_data->pdata->enabled, true);
	ts_data->pdata->prox_power_off = 0;
	fts_ts_resume(ts_data);

	cancel_delayed_work(&ts_data->print_info_work);
	ts_data->pdata->print_info_cnt_open = 0;
	ts_data->pdata->print_info_cnt_release = 0;
	if (!atomic_read(&ts_data->pdata->shutdown_called))
		schedule_work(&ts_data->print_info_work.work);

	return 0;
}

int fts_ts_disable(struct device *dev)
{
	struct fts_ts_data *ts_data = dev_get_drvdata(dev);

	cancel_delayed_work(&ts_data->read_info_work);

	if (atomic_read(&ts_data->pdata->shutdown_called)) {
		FTS_INFO("shutdown was called");
		return 0;
	}

	FTS_INFO("called");
	atomic_set(&ts_data->pdata->enabled, false);

	cancel_delayed_work(&ts_data->print_info_work);
	sec_input_print_info(ts_data->dev, NULL);

	fts_ts_suspend(ts_data);
	return 0;
}

static void fts_parse_dt(struct fts_ts_data *ts_data)
{
	ts_data->support_high_report = of_property_read_bool(ts_data->client->dev.of_node, "focal,support_high_report");

	FTS_INFO(" %ssupport high report", ts_data->support_high_report ? "" : "Not ");
}

static int fts_ts_probe_entry(struct fts_ts_data *ts_data)
{
	int ret = 0;
	int pdata_size = (int)sizeof(struct sec_ts_plat_data);

	FTS_FUNC_ENTER();
	FTS_INFO("%s", FTS_DRIVER_VERSION);
	ts_data->pdata = devm_kzalloc(ts_data->dev, pdata_size, GFP_KERNEL);
	if (!ts_data->pdata) {
		FTS_ERROR("allocate memory for platform_data fail");
		return -ENOMEM;
	}

	ts_data->dev->platform_data = ts_data->pdata;

	if (ts_data->dev->of_node) {
		ret = sec_input_parse_dt(ts_data->dev);
		if (ret) {
			FTS_ERROR("device-tree parse fail");
			goto err_parse_dt;
		}
		fts_parse_dt(ts_data);
	} else {
		if (ts_data->dev->platform_data) {
			memcpy(ts_data->pdata, ts_data->dev->platform_data, pdata_size);
		} else {
			FTS_ERROR("platform_data is null");
			goto err_parse_dt;
		}
	}

	ts_data->ts_workqueue = create_singlethread_workqueue("fts_wq");
	if (!ts_data->ts_workqueue) {
		FTS_ERROR("create fts workqueue fail");
	}

	mutex_init(&ts_data->report_mutex);
	mutex_init(&ts_data->bus_lock);
	mutex_init(&ts_data->device_lock);
	mutex_init(&ts_data->irq_lock);

	/* Init communication interface */
	ret = fts_bus_init(ts_data);
	if (ret) {
		FTS_ERROR("bus initialize fail");
		goto err_bus_init;
	}

	ret = sec_input_device_register(ts_data->dev, ts_data);
	if (ret) {
		FTS_ERROR("failed to register input device, %d", ret);
		goto err_input_init;
	}

	mutex_init(&ts_data->pdata->enable_mutex);
	ts_data->pdata->pinctrl = devm_pinctrl_get(ts_data->dev);
	if (IS_ERR(ts_data->pdata->pinctrl))
		FTS_ERROR("could not get pinctrl");

	ts_data->pdata->pinctrl_configure = sec_input_pinctrl_configure;
	ts_data->pdata->power = fts_power_ctrl;
	ts_data->pdata->set_charger_mode = fts_charger_attached;

	ret = fts_report_buffer_init(ts_data);
	if (ret) {
		FTS_ERROR("report buffer init fail");
		goto err_report_buffer;
	}

#if FTS_POWER_SOURCE_CUST_EN
	ret = fts_power_source_init(ts_data);
#else
	ret = fts_ts_power_on(ts_data);
#endif
	if (ret) {
		FTS_ERROR("fail to get power(regulator)");
		goto err_power_init;
	}

	ret = fts_get_ic_information(ts_data);
	if (ret) {
		FTS_ERROR("not focal IC, unregister driver");
		goto err_irq_req;
	}

#if !IS_ENABLED(CONFIG_SAMSUNG_PRODUCT_SHIP)
	ret = fts_create_apk_debug_channel(ts_data);
	if (ret) {
		FTS_ERROR("create apk debug node fail");
	}

	ret = fts_create_sysfs(ts_data);
	if (ret) {
		FTS_ERROR("create sysfs node fail");
	}
#endif

#if FTS_POINT_REPORT_CHECK_EN
	ret = fts_point_report_check_init(ts_data);
	if (ret) {
		FTS_ERROR("init point report check fail");
	}
#endif

	ret = fts_ex_mode_init(ts_data);
	if (ret) {
		FTS_ERROR("init glove/cover/charger fail");
	}

	ret = fts_gesture_init(ts_data);
	if (ret) {
		FTS_ERROR("init gesture fail");
	}

#if FTS_ESDCHECK_EN
	ret = fts_esdcheck_init(ts_data);
	if (ret) {
		FTS_ERROR("init esd check fail");
	}
#endif

	ret = fts_irq_registration(ts_data);
	if (ret) {
		FTS_ERROR("request irq failed");
		goto err_irq_req;
	}

	ret = fts_fwupg_init(ts_data);
	if (ret) {
		FTS_ERROR("init fw upgrade fail");
		goto err_fwupg_init;
	}

	atomic_set(&ts_data->pdata->enabled, true);

	ts_data->pdata->enable = fts_ts_enable;
	ts_data->pdata->disable = fts_ts_disable;

	ret = fts_sec_cmd_init(ts_data);
	if (ret) {
		FTS_ERROR("init sec_cmd fail");
		goto err_cmd_init;
	}

	sec_input_register_vbus_notifier(ts_data->dev);

	if (ts_data->ts_workqueue)
		INIT_WORK(&ts_data->resume_work, fts_resume_work);

#if defined(CONFIG_PM) && FTS_PATCH_COMERR_PM
	init_completion(&ts_data->pdata->resume_done);
	complete_all(&ts_data->pdata->resume_done);
	fts_wakeup_source_init(ts_data);
#endif
	INIT_DELAYED_WORK(&ts_data->print_info_work, fts_print_info_work);
	INIT_DELAYED_WORK(&ts_data->read_info_work, fts_read_info_work);
	if (!atomic_read(&ts_data->pdata->shutdown_called))
		schedule_delayed_work(&ts_data->read_info_work, msecs_to_jiffies(50));

	FTS_FUNC_EXIT();
	return 0;

err_cmd_init:
	ts_data->pdata->enable = NULL;
	ts_data->pdata->disable = NULL;
err_fwupg_init:
	fts_irq_disable();
	free_irq(ts_data->irq, ts_data);
err_irq_req:
err_power_init:
#if FTS_POWER_SOURCE_CUST_EN
	fts_power_source_exit(ts_data);
#endif
	if (gpio_is_valid(ts_data->pdata->irq_gpio))
		gpio_free(ts_data->pdata->irq_gpio);
err_report_buffer:
err_input_init:
	if (ts_data->ts_workqueue)
		destroy_workqueue(ts_data->ts_workqueue);
err_bus_init:
	mutex_destroy(&ts_data->report_mutex);
	mutex_destroy(&ts_data->bus_lock);
	mutex_destroy(&ts_data->device_lock);
	mutex_destroy(&ts_data->irq_lock);
err_parse_dt:
	FTS_FUNC_EXIT();
	return ret;
}

static int fts_ts_remove_entry(struct fts_ts_data *ts_data)
{
	FTS_FUNC_ENTER();

	ts_data->pdata->enable = NULL;
	ts_data->pdata->disable = NULL;
	atomic_set(&ts_data->pdata->shutdown_called, true);

#if IS_ENABLED(CONFIG_SEC_PANEL_NOTIFIER_V2) && IS_ENABLED(CONFIG_SEC_FACTORY)
	panel_notifier_unregister(&ts_data->lcd_nb);
#endif

	disable_irq(ts_data->irq);
	cancel_delayed_work_sync(&ts_data->print_info_work);
	cancel_delayed_work_sync(&ts_data->read_info_work);

	sec_input_unregister_vbus_notifier(ts_data->dev);

#if FTS_POINT_REPORT_CHECK_EN
	fts_point_report_check_exit(ts_data);
#endif

	fts_sec_cmd_exit(ts_data);

#if !IS_ENABLED(CONFIG_SAMSUNG_PRODUCT_SHIP)
	fts_release_apk_debug_channel(ts_data);
	fts_remove_sysfs(ts_data);
#endif
	fts_ex_mode_exit(ts_data);

	fts_fwupg_exit(ts_data);

#if FTS_ESDCHECK_EN
	fts_esdcheck_exit(ts_data);
#endif

	fts_gesture_exit(ts_data);
	fts_bus_exit(ts_data);

	fts_wakeup_source_unregister(ts_data);

	free_irq(ts_data->irq, ts_data);

	if (ts_data->ts_workqueue)
		destroy_workqueue(ts_data->ts_workqueue);

	if (gpio_is_valid(ts_data->pdata->irq_gpio))
		gpio_free(ts_data->pdata->irq_gpio);

#if FTS_POWER_SOURCE_CUST_EN
	fts_power_source_exit(ts_data);
#endif
	mutex_destroy(&ts_data->report_mutex);
	mutex_destroy(&ts_data->bus_lock);
	mutex_destroy(&ts_data->device_lock);
	mutex_destroy(&ts_data->irq_lock);

	FTS_FUNC_EXIT();
	fts_data = NULL;

	return 0;
}

int fts_ts_suspend(struct fts_ts_data *ts_data)
{
	int ret = 0;

#if IS_ENABLED(CONFIG_SEC_PANEL_NOTIFIER_V2) && IS_ENABLED(CONFIG_SEC_FACTORY)
	if (ts_data->panel_attached == FTS_PANEL_DETACHED) {
		FTS_ERROR("%s: panel detached(%d) skip!\n", __func__, ts_data->panel_attached);
		return 0;
	}
#endif

	mutex_lock(&ts_data->device_lock);
	FTS_INFO("Enter. gesture_mode=0x%02X, prox_power_off=%d, lp:0x%02X pm:0x%02X ed:%d pocket_mode:%d fod_lp_mode:%d",
			ts_data->gesture_mode, ts_data->pdata->prox_power_off,
			ts_data->pdata->lowpower_mode, ts_data->power_mode, ts_data->pdata->ed_enable,
			ts_data->pdata->pocket_mode, ts_data->pdata->fod_lp_mode);

	if (ts_data->suspended) {
		FTS_INFO("Already in suspend state");
		mutex_unlock(&ts_data->device_lock);
		return 0;
	}

	if (ts_data->fw_loading) {
		FTS_INFO("fw upgrade in process, can't suspend");
		mutex_unlock(&ts_data->device_lock);
		return 0;
	}

#if FTS_ESDCHECK_EN
	fts_esdcheck_suspend();
#endif

	if (ts_data->pdata->prox_power_off == 1 && ts_data->aot_enable) {
		ts_data->gesture_mode &= ~GESTURE_DOUBLECLICK_EN;
		FTS_INFO("disable aot when prox_power_off");
	}

	if (ts_data->gesture_mode || ts_data->power_mode || !sec_input_need_ic_off(ts_data->pdata)) {
		fts_gesture_suspend(ts_data);

		ret = fts_write_reg(FTS_REG_HOST_POWER_MODE, 0x01);
		if (ret < 0) {
			FTS_ERROR("set power mode fail");
		} else {
			u8 val = 0;
			ret = fts_read_reg(FTS_REG_HOST_POWER_MODE_STATUS, &val);
			if (ret < 0)
				FTS_ERROR("read power mode status fail");
			else
				FTS_INFO("mode:0x%02X pm:0x%02X, AOT:%d, AOD:%d, ST:%d, SPAY:%d, ED:%d, Pocket:%d, FOD:%d",
						val, ts_data->power_mode,
						(val & FTS_POWER_MODE_DOUBLETAP_TO_WAKEUP) >> 7, (val & FTS_POWER_MODE_AOD) >> 6,
						(val & FTS_POWER_MODE_SINGLE_TAP) >> 5, (val & FTS_POWER_MODE_SWIPE) >> 4,
						(val & FTS_POWER_MODE_EAR_DETECT) >> 3, (val & FTS_POWER_MODE_POCKET) >> 2,
						(val & FTS_POWER_MODE_FOD) >> 1);
		}

		atomic_set(&ts_data->pdata->power_state, SEC_INPUT_STATE_LPM);
	} else {
		fts_irq_disable();
		if (!ts_data->ic_info.is_incell) {
#if FTS_POWER_SOURCE_CUST_EN
			ret = fts_power_source_suspend(ts_data);
#else
			ret = fts_ts_power_off(ts_data);
#endif
			if (ret < 0) {
				FTS_ERROR("power enter suspend fail");
			}
		}
	}

	fts_release_all_finger();
	ts_data->suspended = true;

	cancel_delayed_work(&ts_data->print_info_work);
	fts_print_info(ts_data);

	FTS_FUNC_EXIT();
	mutex_unlock(&ts_data->device_lock);
	return 0;
}

int fts_ts_resume(struct fts_ts_data *ts_data)
{
	ktime_t start_time = ktime_get();
	s64 time_diff_ms;
	int ret = 0;

#if IS_ENABLED(CONFIG_SEC_PANEL_NOTIFIER_V2) && IS_ENABLED(CONFIG_SEC_FACTORY)
	if (ts_data->panel_attached == FTS_PANEL_DETACHED) {
		FTS_ERROR("%s: panel detached(%d) skip!\n", __func__, ts_data->panel_attached);
		return 0;
	}
#endif

	mutex_lock(&ts_data->device_lock);
	FTS_FUNC_ENTER();
	if (!ts_data->suspended) {
		FTS_DEBUG("Already in awake state");
		mutex_unlock(&ts_data->device_lock);
		return 0;
	}

	fts_release_all_finger();

	if (!ts_data->ic_info.is_incell &&
		(atomic_read(&ts_data->pdata->power_state) == SEC_INPUT_STATE_POWER_OFF)) {
#if FTS_POWER_SOURCE_CUST_EN
		fts_power_source_resume(ts_data);
#else
		fts_ts_power_on(ts_data);
#endif
		fts_reset_proc(100);

		fts_wait_tp_to_valid();
		fts_ex_mode_recovery(ts_data);
	}

#if FTS_ESDCHECK_EN
	fts_esdcheck_resume();
#endif

	if (ts_data->gesture_mode ||
		(atomic_read(&ts_data->pdata->power_state) == SEC_INPUT_STATE_LPM)) {
		fts_gesture_resume(ts_data);

		ret = fts_write_reg(FTS_REG_HOST_POWER_MODE, 0x00);
		if (ret < 0) {
			FTS_ERROR("set power mode fail");
		}
	} else {
		fts_irq_enable();
	}

	atomic_set(&ts_data->pdata->power_state, SEC_INPUT_STATE_POWER_ON);

	if (ts_data->aot_enable)
		ts_data->gesture_mode |= GESTURE_DOUBLECLICK_EN;
	ts_data->pdata->prox_power_off = 0;

	ts_data->suspended = false;
	
	time_diff_ms = ktime_ms_delta(ktime_get(), start_time);
	FTS_INFO("Exit(%d). spent %lldms", __LINE__, time_diff_ms);
	mutex_unlock(&ts_data->device_lock);
	return 0;
}

#if defined(CONFIG_PM) && FTS_PATCH_COMERR_PM
static int fts_pm_suspend(struct device *dev)
{
	struct fts_ts_data *ts_data = dev_get_drvdata(dev);

	reinit_completion(&ts_data->pdata->resume_done);
	return 0;
}

static int fts_pm_resume(struct device *dev)
{
	struct fts_ts_data *ts_data = dev_get_drvdata(dev);

	complete_all(&ts_data->pdata->resume_done);
	return 0;
}

static const struct dev_pm_ops fts_dev_pm_ops = {
	.suspend = fts_pm_suspend,
	.resume = fts_pm_resume,
};
#endif

#if IS_ENABLED(CONFIG_SEC_PANEL_NOTIFIER_V2) && IS_ENABLED(CONFIG_SEC_FACTORY)
static int fts_notifier_call(struct notifier_block *n, unsigned long event, void *data)
{
	struct fts_ts_data *ts_data = container_of(n, struct fts_ts_data, lcd_nb);
	struct panel_notifier_event_data *evtdata = data;

	if (event == PANEL_EVENT_UB_CON_STATE_CHANGED) {
		FTS_INFO("%s: event = %ld, ev_data->state = %d\n", __func__, event, evtdata->state);

		if (evtdata->state == PANEL_EVENT_UB_CON_STATE_DISCONNECTED && ts_data->panel_attached == FTS_PANEL_ATTACHED) {
			FTS_INFO("%s: UB_CON_DISCONNECTED : fts_ts_suspend\n", __func__);
			fts_ts_suspend(ts_data);
			ts_data->panel_attached = FTS_PANEL_DETACHED;
		} else if (evtdata->state == PANEL_EVENT_UB_CON_STATE_CONNECTED && ts_data->panel_attached == FTS_PANEL_DETACHED) {
			FTS_INFO("%s: UB_CON_CONNECTED\n", __func__);
			ts_data->panel_attached = FTS_PANEL_ATTACHED;
		}
	}

	return 0;
}
#endif

/*****************************************************************************
* TP Driver
*****************************************************************************/

static int fts_ts_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	struct fts_ts_data *ts_data = NULL;

	FTS_INFO("Touch Screen(I2C BUS) driver probe...");
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		FTS_ERROR("I2C not supported");
		return -ENODEV;
	}

	/* malloc memory for global struct variable */
	ts_data = devm_kzalloc(&client->dev, sizeof(*ts_data), GFP_KERNEL);
	if (!ts_data) {
		FTS_ERROR("allocate memory for fts_data fail");
		return -ENOMEM;
	}

	fts_data = ts_data;
	ts_data->client = client;
	ts_data->dev = &client->dev;
	ts_data->log_level = 1;
	ts_data->fw_is_running = 0;
	ts_data->bus_type = BUS_TYPE_I2C;
	i2c_set_clientdata(client, ts_data);

	ret = fts_ts_probe_entry(ts_data);
	if (ret) {
		FTS_ERROR("Touch Screen(I2C BUS) driver probe fail");
		fts_data = NULL;
		return ret;
	}
#if IS_ENABLED(CONFIG_SEC_PANEL_NOTIFIER_V2) && IS_ENABLED(CONFIG_SEC_FACTORY)
	ts_data->lcd_nb.priority = 1;
	ts_data->lcd_nb.notifier_call = fts_notifier_call;
	ts_data->panel_attached = FTS_PANEL_ATTACHED;
	panel_notifier_register(&ts_data->lcd_nb);
#endif

#if IS_ENABLED(CONFIG_SAMSUNG_TUI)
	ptsp = &client->dev;
	ts_data->pdata->stui_tsp_enter = fts_stui_tsp_enter;
	ts_data->pdata->stui_tsp_exit = fts_stui_tsp_exit;
	ts_data->pdata->stui_tsp_type = fts_stui_tsp_type;
#endif

	FTS_INFO("Touch Screen(I2C BUS) driver probe successfully");
	return 0;
}

static int fts_ts_dev_remove(struct i2c_client *client)
{
	return fts_ts_remove_entry(i2c_get_clientdata(client));
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
static void fts_ts_remove(struct i2c_client *client)
{
	fts_ts_dev_remove(client);
}
#else
static int fts_ts_remove(struct i2c_client *client)
{
	fts_ts_dev_remove(client);
	return 0;
}
#endif

static void fts_ts_shutdown(struct i2c_client *client)
{
	fts_ts_remove_entry(i2c_get_clientdata(client));
}

static const struct i2c_device_id fts_ts_id[] = {
	{FTS_DRIVER_NAME, 0},
	{},
};
static const struct of_device_id fts_dt_match[] = {
	{.compatible = "focaltech,fts", },
	{},
};
MODULE_DEVICE_TABLE(of, fts_dt_match);

static struct i2c_driver fts_ts_driver = {
	.probe = fts_ts_probe,
	.remove = fts_ts_remove,
	.shutdown = fts_ts_shutdown,
	.driver = {
		.name = FTS_DRIVER_NAME,
		.owner = THIS_MODULE,
#if defined(CONFIG_PM) && FTS_PATCH_COMERR_PM
		.pm = &fts_dev_pm_ops,
#endif
		.of_match_table = of_match_ptr(fts_dt_match),
	},
	.id_table = fts_ts_id,
};

static int __init fts_ts_init(void)
{
	int ret = 0;

	FTS_FUNC_ENTER();
	ret = i2c_add_driver(&fts_ts_driver);
	if ( ret != 0 ) {
		FTS_ERROR("Focaltech touch screen driver init failed!");
	}
	FTS_FUNC_EXIT();
	input_log_fix();
	return ret;
}

static void __exit fts_ts_exit(void)
{
	i2c_del_driver(&fts_ts_driver);
}
module_init(fts_ts_init);
module_exit(fts_ts_exit);

MODULE_AUTHOR("FocalTech Driver Team");
MODULE_DESCRIPTION("FocalTech Touchscreen Driver");
MODULE_LICENSE("GPL v2");
