/* SPDX-License-Identifier: GPL-2.0
 * aw882xx_dsp.h
 *
 * Copyright (c) 2020 AWINIC Technology CO., LTD
 *
 * Author: Nick Li <liweilei@awinic.com.cn>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef __AW882XX_DSP_H__
#define __AW882XX_DSP_H__

//#define AW_QCOM_PLATFORM
/*#define AW_AUDIOREACH_PLATFORM*/

//#define AW_QCOM_ADM_MSG

/*#define AW_ALGO_AUTH_DSP*/
#define AW_LSI_PLATFORM

#define AW_ABOX_PLATFORM
#define AW_SKT_ALGO_DTC

/*factor form 12bit(4096) to 1000*/
#define AW_DSP_RE_TO_SHOW_RE(re)	(((re) * (1000)) >> (12))
#define AW_SHOW_RE_TO_DSP_RE(re)	(((re) << 12) / (1000))

/*factor form 24bit(16777216) to 1000*/
#define AW_DSP_PILOTTH_TO_SHOW_PILOTTH(th)	(((th) * (1000)) >> (24))
#define AW_SHOW_PILOTTH_TO_DSP_PILOTTH(th)	(((th) << 24) / (1000))

#define AW_DSP_TRY_TIME		(3)
#define AW_DSP_SLEEP_TIME	(10)
#define PREDICTED_TEMP_NUM	(10)

#define AW_TX_DEFAULT_TOPO_ID		(0x1000FF00)
#define AW_RX_DEFAULT_TOPO_ID		(0x1000FF01)
#define AW_TX_DEFAULT_PORT_ID		(0x1007)
#define AW_RX_DEFAULT_PORT_ID		(0x1006)

#define AW_DSP_MSG_VER			(0x10000000)
#define AW_DSP_CHANNEL_DEFAULT_NUM	(1)

#define AW_DSP_BD_TEMP_SHIFT		(10)
#define AW_DSP_BD_EXCU_SHIFT		(15)
#define AW_DSP_CUR_TEMP_SHIFT		(10)
#define AW_DSP_CMD_GET_BIGDATA		(0x1005)

#define AW_AMBIENT_TEMP_ACCURACY	(20)
#define AW_CALI_TEMP_ACCURACY		(20)

enum aw_dsp_msg_type {
	AW_DSP_MSG_TYPE_DATA = 0,
	AW_DSP_MSG_TYPE_CMD = 1,
};

enum {
	AW_SPIN_0 = 0,
	AW_SPIN_90,
	AW_SPIN_180,
	AW_SPIN_270,
	AW_SPIN_MAX,
};


enum {
	AW_AUDIO_MIX_DSIABLE = 0,
	AW_AUDIO_MIX_ENABLE,
};

#define AW_DSP_MSG_HDR_VER (1)
typedef struct aw_msg_hdr aw_dsp_msg_t;

enum {
	DSP_MSG_TYPE_WRITE_CMD = 0,
	DSP_MSG_TYPE_WRITE_DATA,
	DSP_MSG_TYPE_READ_DATA,
};

typedef struct aw_msg_hdr_v_1_0_0_0 {
	int32_t checksum;
	int32_t version;
	int32_t type;
	int32_t params_id;
	int32_t channel;
	int32_t num;
	int32_t data_size;
	int32_t reseriver[3];
} aw_msg_hdr_t;

struct aw_big_data {
	int32_t max_excursion;
	int32_t max_temperature;
	int32_t over_excursion_cnt;
	int32_t over_temperature_cnt;
	/*int32_t reserve[4];*/
	int32_t mute_num;
	int32_t reserve[3];
};

struct aw_init_data {
	int32_t env_te;
	int32_t bat_voltage;
	int32_t bat_current;
	int32_t cali_re[2];
	int32_t cali_te[2];
	int32_t offset[2];
};

int aw882xx_dsp_read_dsp_msg(struct aw_device *aw_dev, uint32_t msg_id, char *data_ptr, unsigned int size);
int aw882xx_dsp_write_dsp_msg(struct aw_device *aw_dev, uint32_t msg_id, char *data_ptr, unsigned int size);
int aw882xx_dsp_write_cali_cfg(struct aw_device *aw_dev, char *data, unsigned int data_len);
int aw882xx_dsp_read_cali_cfg(struct aw_device *aw_dev, char *data, unsigned int data_len);
int aw882xx_dsp_noise_en(struct aw_device *aw_dev, bool is_noise);
int aw882xx_dsp_write_vmax(struct aw_device *aw_dev, char *data, unsigned int data_len);
int aw882xx_dsp_read_vmax(struct aw_device *aw_dev, char *data, unsigned int data_len);
int aw_dsp_write_ambient_temperature(struct aw_device *aw_dev, int *temp);
int aw_dsp_recover_algo_packet(char *init_data, int init_size);
int aw_dsp_write_surface_temperature(unsigned char dev_index, int temperature);
int aw_dsp_read_surface_temperature(unsigned char dev_index, int *temperature);
int aw_dsp_read_predict_temperature(unsigned char dev_index, int *temperature);
int aw882xx_dsp_write_params(struct aw_device *aw_dev, char *data, unsigned int data_len);
int aw882xx_dsp_write_cali_re(struct aw_device *aw_dev, int32_t cali_re);
int aw882xx_dsp_read_cali_re(struct aw_device *aw_dev, int32_t *cali_re);
int aw882xx_dsp_read_r0(struct aw_device *aw_dev, int32_t *r0);
int aw882xx_dsp_read_st(struct aw_device *aw_dev, int32_t *r0, int32_t *te);
int aw882xx_dsp_read_te(struct aw_device *aw_dev, int32_t *te);
int aw882xx_dsp_get_dc_status(struct aw_device *aw_dev);
int aw882xx_dsp_hmute_en(struct aw_device *aw_dev, bool is_hmute);
int aw882xx_dsp_cali_en(struct aw_device *aw_dev, int32_t cali_msg_data);
int aw882xx_dsp_read_f0(struct aw_device *aw_dev, int32_t *f0);
int aw882xx_dsp_read_f0_q(struct aw_device *aw_dev, int32_t *f0, int32_t *q);
int aw882xx_dsp_read_cali_data(struct aw_device *aw_dev, char *data, unsigned int data_len);
int aw882xx_dsp_set_afe_module_en(int type, int enable);
int aw882xx_dsp_get_afe_module_en(int type, int *status);
int aw882xx_dsp_set_copp_module_en(bool enable);
int aw882xx_dsp_write_spin(int spin_mode);
int aw882xx_dsp_read_spin(int *spin_mode);
int aw882xx_get_algo_version(struct aw_device *aw_dev, char *algo_ver_buf);
void aw882xx_device_parse_topo_id_dt(struct aw_device *aw_dev);
void aw882xx_device_parse_port_id_dt(struct aw_device *aw_dev);
int aw882xx_dsp_set_mixer_en(struct aw_device *aw_dev, uint32_t mixer_en);
int aw882xx_dsp_write_voltage_offset(struct aw_device *aw_dev,
												int32_t voltage_offset);
int aw882xx_dsp_read_voltage_offset(struct aw_device *aw_dev,
												int32_t *voltage_offset);
int aw882xx_dsp_read_pilot_tone_threshold(struct aw_device *aw_dev,
												int32_t *pilot_tone_threshold);
int aw882xx_dsp_write_pilot_tone_threshold(struct aw_device *aw_dev,
												int32_t pilot_tone_threshold);

int aw882xx_dsp_read_spk_excursion(struct aw_device *aw_dev,
												char *data, int data_size);
int aw882xx_dsp_write_bat_property(struct aw_device *aw_dev, int *vol, int *cur);

int aw882xx_dsp_read_cali_te(struct aw_device *aw_dev, int32_t *cali_te);
int aw882xx_dsp_write_cali_te(struct aw_device *aw_dev, int32_t cali_te);
int aw882xx_dsp_get_big_data(struct aw_device * aw_dev, struct aw_big_data *data, int len);
int aw882xx_dsp_get_abox_data(struct aw_device * aw_dev, struct aw_big_data *buf, int len, int abox_id);
int aw_dsp_read_cur_te(unsigned char ch);
int aw882xx_dsp_get_abox_big_data(struct aw_device * aw_dev, struct aw_big_data *data, int len);

#ifdef AW_ALGO_AUTH_DSP
int aw882xx_dsp_read_algo_auth_data(struct aw_device *aw_dev,
		char *data, unsigned int data_len);
int aw882xx_dsp_write_algo_auth_data(struct aw_device *aw_dev,
		char *data, unsigned int data_len);
#endif

#endif

