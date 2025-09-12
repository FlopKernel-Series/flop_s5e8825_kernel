// SPDX-License-Identifier: GPL-2.0

#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/of_irq.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/debugfs.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/of_regulator.h>
#include <linux/power_supply.h>
#include <linux/pm_runtime.h>
#include <linux/sched/clock.h>
#include <linux/math64.h>
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
#include "../../common/sec_charging_common.h"
#include "../../common/sec_direct_charger.h"
#endif
#include "nu2111a_charger.h"
#if IS_ENABLED(CONFIG_SEC_ABC)
#include <linux/sti/abc_common.h>
#endif

#if defined(CONFIG_OF)
#include <linux/of_gpio.h>
#include <linux/of_device.h>
#endif

#define BITS(_end, _start)		((BIT(_end) - BIT(_start)) + BIT(_end))
#define MIN(a, b)				((a < b) ? (a):(b))

static int nu2111a_read_adc(struct nu2111a_charger *chg);

#define LOG_DBG(chg, fmt, args...) pr_info("[%s]:: " fmt, __func__, ## args)

static struct device_attribute nu2111a_charger_attrs[] = {
	NU2111A_CHARGER_ATTR(ta_v_offset),
	NU2111A_CHARGER_ATTR(ta_c_offset),
	NU2111A_CHARGER_ATTR(iin_c_offset),
	NU2111A_CHARGER_ATTR(test_mode),
	NU2111A_CHARGER_ATTR(pps_vol),
	NU2111A_CHARGER_ATTR(pps_cur),
	NU2111A_CHARGER_ATTR(vfloat),
	NU2111A_CHARGER_ATTR(iincc),
	NU2111A_CHARGER_ATTR(dcerr_reason),
};

static int nu2111a_set_charging_state(struct nu2111a_charger *chg, unsigned int charging_state)
{
	union power_supply_propval value = {0,};
	static int prev_val = DC_STATE_NOT_CHARGING;

	chg->charging_state = charging_state;

	pr_info("%s: chg->charging_state(%d)\n", __func__, chg->charging_state);

	switch (charging_state) {
	case DC_STATE_NOT_CHARGING:
		value.intval = SEC_DIRECT_CHG_MODE_DIRECT_OFF;
		break;
	case DC_STATE_CHECK_VBAT:
		value.intval = SEC_DIRECT_CHG_MODE_DIRECT_CHECK_VBAT;
		break;
	case DC_STATE_PRESET_DC:
		value.intval = SEC_DIRECT_CHG_MODE_DIRECT_PRESET;
		break;
	case DC_STATE_CHECK_ACTIVE:
	case DC_STATE_START_CC:
	case DC_STATE_ADJUST_TAVOL:
	case DC_STATE_ADJUST_TACUR:
		value.intval = SEC_DIRECT_CHG_MODE_DIRECT_ON_ADJUST;
		break;
	case DC_STATE_CC_MODE:
	case DC_STATE_CV_MODE:
	case DC_STATE_REVERSE_MODE:
		value.intval = SEC_DIRECT_CHG_MODE_DIRECT_ON;
		break;
#ifdef CONFIG_PASS_THROUGH
	case DC_STATE_PT_MODE:
	case DC_STATE_PTMODE_CHANGE:
		value.intval = SEC_DIRECT_CHG_MODE_DIRECT_BYPASS;
		break;
#endif
	case DC_STATE_CHARGING_DONE:
		value.intval = SEC_DIRECT_CHG_MODE_DIRECT_DONE;
		break;
	default:
		return -1;
	}

	if (prev_val == value.intval)
		return -1;

	prev_val = value.intval;
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	psy_do_property(chg->pdata->sec_dc_name, set,
		POWER_SUPPLY_EXT_PROP_DIRECT_CHARGER_MODE, value);
#endif
	return 0;
}

static int nu2111a_read_reg(struct nu2111a_charger *chg, int reg, void *value)
{
	int ret = 0, i;

	mutex_lock(&chg->i2c_lock);
	if (chg->client->addr == 0x6e) {
		for (i = 0; i < I2C_RETRY_CNT; ++i) {
			ret = regmap_read(chg->regmap, reg, value);
			if (ret >= 0)
				break;
			pr_info("%s:reg(0x%x), ret(%d), i2c_retry_cnt(%d/%d)\n",
				__func__, reg, ret, i + 1, I2C_RETRY_CNT);
		}
	} else
		ret = -EINVAL;
	mutex_unlock(&chg->i2c_lock);

	if (ret < 0) {
		pr_info("%s: reg(0x%x), ret(%d)\n", __func__, reg, ret);
#if IS_ENABLED(CONFIG_SEC_ABC) && !defined(CONFIG_SEC_FACTORY)
		if (chg->client->addr == 0x6e)
			sec_abc_send_event("MODULE=battery@WARN=dc_i2c_fail");
#endif
	}
	return ret;
}

static int nu2111a_read_reg_sub(struct nu2111a_charger *chg, int reg, void *value)
{
	int ret = 0, i;

	if (IS_ERR_OR_NULL(chg->regmap_sub))
		return -ENODEV;

	mutex_lock(&chg->i2c_lock);
	if (chg->client_sub->addr == 0x6e) {
		for (i = 0; i < I2C_RETRY_CNT; ++i) {
			ret = regmap_read(chg->regmap_sub, reg, value);
			if (ret >= 0)
				break;
			pr_info("%s:reg(0x%x), ret(%d), i2c_retry_cnt(%d/%d)\n",
				__func__, reg, ret, i + 1, I2C_RETRY_CNT);
		}
	} else
		ret = -EINVAL;
	mutex_unlock(&chg->i2c_lock);

	if (ret < 0)
		pr_info("%s: reg(0x%x), ret(%d)\n", __func__, reg, ret);

	return ret;
}

static int nu2111a_bulk_read_reg(struct nu2111a_charger *chg, int reg, void *val, int count)
{
	int ret = 0, i;

	mutex_lock(&chg->i2c_lock);
	if (chg->client->addr == 0x6e) {
		for (i = 0; i < I2C_RETRY_CNT; ++i) {
			ret = regmap_bulk_read(chg->regmap, reg, val, count);
			if (ret >= 0)
				break;
			pr_info("%s:reg(0x%x), ret(%d), i2c_retry_cnt(%d/%d)\n",
				__func__, reg, ret, i + 1, I2C_RETRY_CNT);
		}
	} else
		ret = -EINVAL;
	mutex_unlock(&chg->i2c_lock);

	if (ret < 0) {
		pr_info("%s: reg(0x%x), ret(%d)\n", __func__, reg, ret);
#if IS_ENABLED(CONFIG_SEC_ABC) && !defined(CONFIG_SEC_FACTORY)
		if (chg->client->addr == 0x6e)
			sec_abc_send_event("MODULE=battery@WARN=dc_i2c_fail");
#endif
	}
	return ret;
}

static int nu2111a_bulk_read_reg_sub(struct nu2111a_charger *chg, int reg, void *val, int count)
{
	int ret = 0, i;

	if (IS_ERR_OR_NULL(chg->regmap_sub))
		return -ENODEV;

	mutex_lock(&chg->i2c_lock);
	if (chg->client_sub->addr == 0x6e) {
		for (i = 0; i < I2C_RETRY_CNT; ++i) {
			ret = regmap_bulk_read(chg->regmap_sub, reg, val, count);
			if (ret >= 0)
				break;
			pr_info("%s:reg(0x%x), ret(%d), i2c_retry_cnt(%d/%d)\n",
				__func__, reg, ret, i + 1, I2C_RETRY_CNT);
		}
	} else
		ret = -EINVAL;
	mutex_unlock(&chg->i2c_lock);

	if (ret < 0)
		pr_info("%s: reg(0x%x), ret(%d)\n", __func__, reg, ret);
	return ret;
}

static int nu2111a_write_reg_main(struct nu2111a_charger *chg, int reg, u8 value)
{
	int ret = 0, i;

	mutex_lock(&chg->i2c_lock);
	if (chg->client->addr == 0x6e) {
		for (i = 0; i < I2C_RETRY_CNT; ++i) {
			ret = regmap_write(chg->regmap, reg, value);
			if (ret >= 0)
				break;
			pr_info("%s:reg(0x%x), ret(%d), i2c_retry_cnt(%d/%d)\n",
				__func__, reg, ret, i + 1, I2C_RETRY_CNT);
		}
	} else
		ret = -EINVAL;
	mutex_unlock(&chg->i2c_lock);

	if (ret < 0)
		pr_info("%s: reg(0x%x), ret(%d)\n", __func__, reg, ret);
	return ret;
}

static int nu2111a_write_reg_sub(struct nu2111a_charger *chg, int reg, u8 value)
{
	int ret = 0, i;

	if (IS_ERR_OR_NULL(chg->regmap_sub))
		return -ENODEV;

	mutex_lock(&chg->i2c_lock);
	if (chg->client_sub->addr == 0x6e) {
		for (i = 0; i < I2C_RETRY_CNT; ++i) {
			ret = regmap_write(chg->regmap_sub, reg, value);
			if (ret >= 0)
				break;
			pr_info("%s:reg(0x%x), ret(%d), i2c_retry_cnt(%d/%d)\n",
				__func__, reg, ret, i + 1, I2C_RETRY_CNT);
		}
	} else
		ret = -EINVAL;
	mutex_unlock(&chg->i2c_lock);

	if (ret < 0)
		pr_info("%s: reg(0x%x), ret(%d)\n", __func__, reg, ret);
	return ret;
}

static int nu2111a_write_reg(struct nu2111a_charger *chg, int reg, u8 value)
{
	int ret = 0;

	if (chg->client->addr == 0x6e) {
		ret = nu2111a_write_reg_main(chg, reg, value);
		if (ret >= 0 && chg->regmap_sub)
			ret = nu2111a_write_reg_sub(chg, reg, value);
	} else
		ret = -EINVAL;

	if (ret < 0) {
		pr_info("%s: reg(0x%x), ret(%d)\n", __func__, reg, ret);
#if IS_ENABLED(CONFIG_SEC_ABC) && !defined(CONFIG_SEC_FACTORY)
		if (chg->client->addr == 0x6e)
			sec_abc_send_event("MODULE=battery@WARN=dc_i2c_fail");
#endif
	}
	return ret;
}

static int nu2111a_update_reg(struct nu2111a_charger *chg, int reg, u8 mask, u8 val)
{
	int ret = 0;
	unsigned int temp;

	/* This is for raspberry pi only, it can be changed by customer */
	ret = nu2111a_read_reg(chg, reg, &temp);
	if (ret < 0) {
		pr_err("failed to read reg 0x%x, ret= %d\n", reg, ret);
	} else {
		temp &= ~mask;
		temp |= val & mask;
		ret = nu2111a_write_reg_main(chg, reg, temp);
		if (ret < 0)
			pr_err("failed to write reg0x%x, ret= %d\n", reg, ret);
	}
	if (ret >= 0 && chg->regmap_sub) {
		/* This is for raspberry pi only, it can be changed by customer */
		ret = nu2111a_read_reg_sub(chg, reg, &temp);
		if (ret < 0) {
			pr_err("failed to read reg 0x%x, ret= %d\n", reg, ret);
		} else {
			temp &= ~mask;
			temp |= val & mask;
			ret = nu2111a_write_reg_sub(chg, reg, temp);
			if (ret < 0)
				pr_err("failed to write reg0x%x, ret= %d\n", reg, ret);
		}
	}
	return ret;
}

static int nu2111a_update_reg_main(struct nu2111a_charger *chg, int reg, u8 mask, u8 val)
{
	int ret = 0;
	unsigned int temp;

	/* This is for raspberry pi only, it can be changed by customer */
	ret = nu2111a_read_reg(chg, reg, &temp);
	if (ret < 0) {
		pr_err("failed to read reg 0x%x, ret= %d\n", reg, ret);
	} else {
		temp &= ~mask;
		temp |= val & mask;
		ret = nu2111a_write_reg_main(chg, reg, temp);
		if (ret < 0)
			pr_err("failed to write reg0x%x, ret= %d\n", reg, ret);
	}
	return ret;
}

static int nu2111a_update_reg_sub(struct nu2111a_charger *chg, int reg, u8 mask, u8 val)
{
	int ret = 0;
	unsigned int temp;

	if (chg->regmap_sub) {
		/* This is for raspberry pi only, it can be changed by customer */
		ret = nu2111a_read_reg_sub(chg, reg, &temp);
		if (ret < 0) {
			pr_err("failed to read reg 0x%x, ret= %d\n", reg, ret);
		} else {
			temp &= ~mask;
			temp |= val & mask;
			ret = nu2111a_write_reg_sub(chg, reg, temp);
			if (ret < 0)
				pr_err("failed to write reg0x%x, ret= %d\n", reg, ret);
		}
	}
	return ret;
}

#if defined(CONFIG_OF)
static int nu2111a_parse_dt(struct device *dev, struct nu2111a_platform_data *pdata)
{
	struct device_node *np = dev->of_node;
	int rc = 0;

	if (!np)
		return -EINVAL;

	rc = of_property_read_u32(np, "nu2111a,vbat-reg", &pdata->vbat_reg);
	if (rc) {
		pr_info("%s: failed to get vbat-reg from dtsi", __func__);
		pdata->vbat_reg = NU2111A_VBAT_REG_DFT;
	}
	pr_info("[%s]:: vbat-reg:: %d\n", __func__,  pdata->vbat_reg);

	rc = of_property_read_u32(np, "nu2111a,input-current-limit", &pdata->iin_cfg);
	if (rc) {
		pr_info("%s: failed to get input-current-limit from dtsi", __func__);
		pdata->iin_cfg = NU2111A_IIN_CFG_DFT;
	}
	pr_info("[%s]:: input-current-limit:: %d\n", __func__,  pdata->iin_cfg);

	rc = of_property_read_u32(np, "nu2111a,topoff-current", &pdata->iin_topoff);
	if (rc) {
		pr_info("%s: failed to get topoff current from dtsi", __func__);
		pdata->iin_topoff = NU2111A_IIN_DONE_DFT;
	}
	pr_info("[%s]:: topoff-current %d\n", __func__,  pdata->iin_topoff);
	rc = of_property_read_u32(np, "nu2111a,fpdo_dc_input-itopoff",
							   &pdata->fpdo_dc_iin_topoff);
	if (rc) {
		pr_info("%s: nu2111a,fpdo_dc_input-itopoff is Empty\n", __func__);
		pdata->fpdo_dc_iin_topoff = 1700000; /* 1700mA */
	}
	pr_info("%s: nu2111a,fpdo_dc_iin_topoff is %d\n", __func__, pdata->fpdo_dc_iin_topoff);
	rc = of_property_read_u32(np, "nu2111a,fpdo_dc_vnow-topoff",
							&pdata->fpdo_dc_vnow_topoff);
	if (rc) {
		pr_info("%s: nu2111a,fpdo_dc_vnow-topoff is Empty\n", __func__);
		pdata->fpdo_dc_vnow_topoff = 5000000; /* Vnow 5000000uV means disable */
	}
	pr_info("%s: nu2111a,fpdo_dc_vnow_topoff is %d\n", __func__, pdata->fpdo_dc_vnow_topoff);

	rc = of_property_read_u32(np, "nu2111a,wd-tmr", &pdata->wd_tmr);
	if (rc) {
		pr_info("%s: failed to get wd_tmr from dtsi", __func__);
		pdata->wd_tmr = WD_TMR_5S;
	}
	pr_info("%s:: wd-tmr[0x%x]", __func__, pdata->wd_tmr);

	rc = of_property_read_u32(np, "nu2111a,sw-freq", &pdata->sw_freq);
	if (rc) {
		pr_info("%s: failed to get sw-freq from dtsi", __func__);
		pdata->sw_freq = FSW_CFG_730KHZ;
	}
	pr_info("%s:: sw-freq[0x%x]", __func__, pdata->sw_freq);

	pdata->wd_dis = of_property_read_bool(np, "nu2111a,wd-dis");
	pr_info("%s: wd-dis[%d]\n", __func__, pdata->wd_dis);

	pdata->use_dc_vbat = of_property_read_bool(np, "nu2111a,use-dc-vbat");
	pr_info("%s: use_dc_vbat[%d]\n", __func__, pdata->use_dc_vbat);

	pdata->np_sub = of_parse_phandle(np, "nu2111a,sub_i2c-bus", 0);
	if (pdata->np_sub) {
		pdata->sub_i2c = of_find_i2c_adapter_by_node(pdata->np_sub);
		of_node_put(pdata->np_sub);
		pr_info("%s: sub device\n", __func__);
		if (pdata->sub_i2c)
			pr_info("%s: sub_i2c\n", __func__);
		else
			pr_info("%s: sub_i2c NULL\n", __func__);
	} else {
		pr_info("%s: No sub device\n", __func__);
	}

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	np = of_find_node_by_name(NULL, "battery");
	if (!np) {
		pr_err("%s: np(battery) NULL\n", __func__);
	} else {
		rc = of_property_read_string(np, "battery,charger_name",
				(char const **)&pdata->sec_dc_name);
		if (rc) {
			pr_err("%s: direct_charger is Empty\n", __func__);
			pdata->sec_dc_name = "sec-direct-charger";
		}
		pr_info("%s: battery,charger_name is %s\n", __func__, pdata->sec_dc_name);

		/* Fuelgauge power supply name */
		rc = of_property_read_string(np, "battery,fuelgauge_name",
				(char const **)&pdata->fg_name);
		if (rc) {
			pr_info("## %s: fuelgauge_name name is Empty\n", __func__);
			pdata->fg_name = "battery";
		}

		rc = of_property_read_u32_array(np, "battery,dc_step_chg_cond_vol",
				&pdata->step1_vfloat, 1);
		if (rc) {
			pr_info("%s : battery,dc_step_chg_cond_vol is Empty\n", __func__);
			pdata->step1_vfloat = 4140;
		}
		pr_info("%s: parse step1_vfloat : %d, ret = %d\n", __func__, pdata->step1_vfloat, rc);
		rc = of_property_read_u32(np, "battery,fpdo_dc_charge_power",
								   &pdata->fpdo_dc_iin_lowest_limit);
		pdata->fpdo_dc_iin_lowest_limit *= NU2111A_SEC_DENOM_U_M;
		pdata->fpdo_dc_iin_lowest_limit /= NU2111A_SEC_FPDO_DC_IV;
		if (rc) {
			pr_info("%s: battery,fpdo_dc_charge_power is Empty\n", __func__);
			pdata->fpdo_dc_iin_lowest_limit = 10000000; /* 10A */
		}
		pr_info("%s: fpdo_dc_iin_lowest_limit is %d\n", __func__, pdata->fpdo_dc_iin_lowest_limit);
	}
#endif
	rc = 0;

	return rc;
}
#else
static int nu2111a_parse_dt(struct device *dev, struct nu2111a_platform_data *pdata)
{
	return 0;
}
#endif

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
char *charging_state_str[] = {
	"NO_CHARGING", "CHECK_VBAT", "PRESET_DC", "CHECK_ACTIVE", "ADJUST_CC",
	"START_CC", "CC_MODE", "CV_MODE", "CHARGING_DONE", "ADJUST_TAVOL",
	"ADJUST_TACUR"
#ifdef CONFIG_PASS_THROUGH
	, "PT_MODE", "PTMODE_CHANGE",
#endif
	"REVERSE_MODE",
	"FPDO_CV_MODE",
};

void nu2111a_show_reg(struct nu2111a_charger *chg)
{
	int ret, i;
	u8 data[50];
	char *str = NULL;
	char *str2 = NULL;

	str = kzalloc(sizeof(char) * 512, GFP_KERNEL);
	if (!str)
		return;

	ret = nu2111a_bulk_read_reg(chg, 0x00, data, 0x2B);

	sprintf(str + strlen(str),
	"%s", "Reg[0-0x2A] = ");

	for (i = 0 ; i < 0x2B ; i++)
		sprintf(str + strlen(str), "0x%02X,", data[i]);
	LOG_DBG(chg, "%s\n", str);

	kfree(str);

	if (chg->regmap_sub) {
		str2 = kzalloc(sizeof(char) * 512, GFP_KERNEL);
		if (!str2)
			return;

		ret = nu2111a_bulk_read_reg_sub(chg, 0x00, data, 0x2B);

		sprintf(str2 + strlen(str2),
		"%s", "Reg[0-0x2A]_Sub = ");

		for (i = 0 ; i < 0x2B ; i++)
			sprintf(str2 + strlen(str2), "0x%02X,", data[i]);
		LOG_DBG(chg, "%s\n", str2);
		kfree(str2);
	}
}

#ifdef _NU_DBG
static void nu211a_adc_work(struct work_struct *work)
{
	struct nu2111a_charger *chg = container_of(work, struct nu2111a_charger, adc_work.work);

	nu2111a_show_reg(chg);

	schedule_delayed_work(&chg->adc_work, msecs_to_jiffies(5000));
}
#endif

static void nu2111a_monitor_work(struct nu2111a_charger *chg)
{
	int ta_vol = chg->ta_vol / NU2111A_SEC_DENOM_U_M;
	int ta_cur = chg->ta_cur / NU2111A_SEC_DENOM_U_M;

	if (chg->charging_state == DC_STATE_NOT_CHARGING)
		return;

	nu2111a_read_adc(chg);
	if (chg->regmap_sub)
		pr_info("%s: state(%s),iin_cc(%dmA),vbat_reg(%dmV),vbat(%dmV),vin(%dmV),iin(%dmA,%d,%d),tdie(%d),pps_requested(%d/%dmV/%dmA)\n",
			__func__, charging_state_str[chg->charging_state],
			chg->iin_cc / NU2111A_SEC_DENOM_U_M, chg->pdata->vbat_reg / NU2111A_SEC_DENOM_U_M,
			chg->adc_vbat / NU2111A_SEC_DENOM_U_M, chg->adc_vin / NU2111A_SEC_DENOM_U_M,
			(chg->adc_iin+chg->adc_iin_sub) / NU2111A_SEC_DENOM_U_M,
			chg->adc_iin / NU2111A_SEC_DENOM_U_M, chg->adc_iin_sub / NU2111A_SEC_DENOM_U_M,
			chg->adc_tdie, chg->ta_objpos, ta_vol, ta_cur);
	else
		pr_info("%s: state(%s),iin_cc(%dmA),vbat_reg(%dmV),vbat(%dmV),vin(%dmV),iin(%dmA),tdie(%d),pps_requested(%d/%dmV/%dmA)\n",
			__func__, charging_state_str[chg->charging_state],
			chg->iin_cc / NU2111A_SEC_DENOM_U_M, chg->pdata->vbat_reg / NU2111A_SEC_DENOM_U_M,
			chg->adc_vbat / NU2111A_SEC_DENOM_U_M, chg->adc_vin / NU2111A_SEC_DENOM_U_M,
			chg->adc_iin / NU2111A_SEC_DENOM_U_M, chg->adc_tdie, chg->ta_objpos, ta_vol, ta_cur);

	nu2111a_show_reg(chg);
}
#endif

static int nu2111a_get_apdo_max_power(struct nu2111a_charger *chg)
{
	int ret = 0;
	//unsigned int ta_objpos = 0;
	unsigned int ta_max_vol = 0;
	unsigned int ta_max_cur = 0;
	unsigned int ta_max_pwr = 0;

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	ta_max_vol = chg->ta_max_vol / NU2111A_SEC_DENOM_U_M;

	ret = sec_pd_get_apdo_max_power(&chg->ta_objpos,
				&ta_max_vol, &ta_max_cur, &ta_max_pwr);
#else
	ret = max77958_get_apdo_max_power(&chg->ta_objpos,
				&ta_max_vol, &ta_max_cur, &ta_max_pwr);
#endif

	/* mA,mV,mW --> uA,uV,uW */
	chg->ta_max_vol = ta_max_vol * NU2111A_SEC_DENOM_U_M;
	chg->ta_max_cur = ta_max_cur * NU2111A_SEC_DENOM_U_M;
	chg->ta_max_pwr = ta_max_pwr;

	LOG_DBG(chg, "ta_max_vol=%d, ta_max_cur=%d, ta_max_pwr=%d\n",
			chg->ta_max_vol, chg->ta_max_cur, chg->ta_max_pwr);

	chg->pdo_index = chg->ta_objpos;
	chg->pdo_max_voltage = ta_max_vol;
	chg->pdo_max_current = ta_max_cur;
	return ret;
}

static int nu2111a_set_swcharger(struct nu2111a_charger *chg, bool en)
{
	int ret = 0;
	union power_supply_propval value = {0, };

	value.intval = en;
	ret = psy_do_property(chg->pdata->sec_dc_name, set,
		POWER_SUPPLY_EXT_PROP_CHARGING_ENABLED_DC, value);

	if (ret < 0)
		pr_info("%s: error switching_charger, ret=%d\n", __func__, ret);

	return 0;
}

static int nu2111a_enable_adc(struct nu2111a_charger *chg, bool enable)
{
	int ret;
	u8 val;

	if (enable)
		val = NU2111A_BIT_ADC_EN;
	else
		val = 0;

	ret = nu2111a_update_reg(chg, NU2111A_REG_ADC_CTRL, NU2111A_BIT_ADC_EN, val);
	return ret;
}

/*
 * enable/disable adc channel
 */
static int nu2111a_set_adc_scan(struct nu2111a_charger *chg, int channel, bool enable)
{
	int ret;
	u8 reg;
	u8 mask;
	u8 shift;
	u8 val;

	if (channel > ADC_MAX_NUM)
		return -EINVAL;

	reg = NU2111A_REG_ADC_FN_DIS;
	shift = 8 - channel;
	mask = 1 << shift;

	if (enable)
		val = 0 << shift;
	else
		val = 1 << shift;

	ret = nu2111a_update_reg(chg, reg, mask, val);

	return ret;
}

static int nu2111a_adc_oneshot(struct nu2111a_charger *chg, bool en)
{
	int ret = 0;

	if (en) {
		if (chg->adc_read) {
			LOG_DBG(chg, "wait adc read\n");
			ret = wait_event_timeout(chg->wait_q, !chg->adc_read, msecs_to_jiffies(35));
			LOG_DBG(chg, "end wait adc read %d %d\n", ret, chg->adc_read);
		} else {
			chg->adc_read = true;
			nu2111a_update_reg(chg, NU2111A_REG_ADC_CTRL, NU2111A_BIT_ADC_RATE, 0);
			nu2111a_update_reg(chg, NU2111A_REG_ADC_CTRL, NU2111A_BIT_ADC_RATE, 1);
			msleep(35);
		}
	} else {
		//nu2111a_update_reg(chg, NU2111A_REG_ADC_CTRL, NU2111A_BIT_ADC_RATE, 0);
		chg->adc_read = false;
	}

	return 0;
}

static int nu2111a_get_adc_channel(struct nu2111a_charger *chg, unsigned int channel)
{
	union power_supply_propval value = {0,};
	u8 r_data[2];
	u16 raw_adc;
	int ret = 0;
	int vbus_ovp_th, val;

	switch (channel) {
	case ADCCH_VIN:
		ret = nu2111a_bulk_read_reg(chg, NU2111A_REG_VBUS_ADC1, r_data, 2);
		if (ret < 0)
			goto Err;
		raw_adc = ((r_data[0] << 8) | r_data[1]);
		chg->adc_vin = raw_adc * NU2111A_SEC_DENOM_U_M;//uV
		break;
	case ADCCH_IIN:
		ret = nu2111a_bulk_read_reg(chg, NU2111A_REG_IBUS_ADC1, r_data, 2);
		if (ret < 0)
			goto Err;
		raw_adc = ((r_data[0] << 8) | r_data[1]);
		if (r_data[0] & 0x80) {
			chg->adc_iin = (raw_adc - (1 << 16)) * NU2111A_SEC_DENOM_U_M;
			chg->adc_iin_sub *= -1;
		} else
			chg->adc_iin = raw_adc * NU2111A_SEC_DENOM_U_M;//uV
		LOG_DBG(chg, "IBUS_ADC_Main %d\n", chg->adc_iin);
		if (chg->regmap_sub) {
			ret = nu2111a_bulk_read_reg_sub(chg, NU2111A_REG_IBUS_ADC1, r_data, 2);
			if (ret < 0)
				goto Err;
			raw_adc = ((r_data[0] << 8) | r_data[1]);
			if (r_data[0] & 0x80) {
				chg->adc_iin_sub = (raw_adc - (1 << 16)) * NU2111A_SEC_DENOM_U_M;
				chg->adc_iin_sub *= -1;
			} else
				chg->adc_iin_sub = raw_adc * NU2111A_SEC_DENOM_U_M;//uV
			LOG_DBG(chg, "IBUS_ADC_Sub %d\n", chg->adc_iin_sub);
		}
		break;
	case ADCCH_VBAT:
		ret = nu2111a_bulk_read_reg(chg, NU2111A_REG_VBAT_ADC1, r_data, 2);
		if (ret < 0)
			goto Err;
		raw_adc = ((r_data[0] << 8) | r_data[1]);
		chg->adc_vbat = raw_adc * NU2111A_SEC_DENOM_U_M;//uV

		if (chg->pdata->use_dc_vbat) {
			if (raw_adc == 0)
				ret = psy_do_property(chg->pdata->fg_name, get,
					POWER_SUPPLY_PROP_VOLTAGE_NOW, value);
			else
				value.intval = raw_adc;
		} else {
			ret = psy_do_property(chg->pdata->fg_name, get,
				POWER_SUPPLY_PROP_VOLTAGE_NOW, value);
		}

		if (ret < 0)
			LOG_DBG(chg, "Vnow read err\n");
		else
			chg->adc_vbat = value.intval * NU2111A_SEC_DENOM_U_M;

		LOG_DBG(chg, "Vbat %d, Vnow %d\n", raw_adc, value.intval);

		ret = nu2111a_bulk_read_reg(chg, NU2111A_REG_VBUS_ADC1, r_data, 2);
		if (ret < 0)
			goto Err;
		raw_adc = ((r_data[0] << 8) | r_data[1]);
		chg->adc_vin = raw_adc * NU2111A_SEC_DENOM_U_M;//uV

		vbus_ovp_th = MAX(chg->adc_vbat/NU2111A_SEC_DENOM_U_M*2 + 1000, raw_adc + 1000);
		vbus_ovp_th = (vbus_ovp_th/50)*50;
		val = (vbus_ovp_th - 6000)/50;
		if (vbus_ovp_th > chg->vbus_ovp_th) {
			LOG_DBG(chg,
				"ta-vol %d pre_ovp_th %d cur_ovp_th %d reg 0x%x\n",
				chg->ta_vol, chg->vbus_ovp_th, vbus_ovp_th, val);
			chg->vbus_ovp_th = vbus_ovp_th;
			nu2111a_write_reg(chg, NU2111A_REG_VBUS_OVP, val);
		}
		break;
	/* It depends on customer's scenario */
	case ADCCH_IBAT:
#ifdef PMIC_VNOW
		ret = nu2111a_bulk_read_reg(chg, NU2111A_REG_IBAT_ADC1, r_data, 2);
		if (ret < 0)
			goto Err;
		raw_adc = ((r_data[0] << 8) | r_data[1]);
		chg->adc_ibat = raw_adc * NU2111A_SEC_DENOM_U_M;//uV
#endif
		ret = psy_do_property(chg->pdata->fg_name, get,
				POWER_SUPPLY_PROP_CURRENT_NOW, value);
		if (ret < 0) {
			LOG_DBG(chg, "Inow read err\n");
			chg->adc_ibat = 0;
		} else
			chg->adc_ibat = value.intval;
		LOG_DBG(chg, "value %d adc_ibat %d\n", value.intval, chg->adc_ibat);
		break;

	case ADCCH_VOUT:
		break;
	case ADCCH_NTC:
		ret = nu2111a_bulk_read_reg(chg, NU2111A_REG_NTC_ADC1, r_data, 2);
		if (ret < 0)
			goto Err;
		raw_adc = ((r_data[0] << 8) | r_data[1]);
		chg->adc_ntc = (10000*raw_adc)/(1024-2*raw_adc);
		pr_info("[%s] ntc------r_data[0]:0x%x, r_data[1]:0x%x ; %d, %d\n",
			__func__, r_data[0], r_data[1], raw_adc, chg->adc_ntc);
		break;
	case ADCCH_TDIE:
		ret = nu2111a_bulk_read_reg(chg, NU2111A_REG_TDIE_ADC1, r_data, 2);
		if (ret < 0)
			goto Err;
		raw_adc = ((r_data[0] << 8) | r_data[1]);
		chg->adc_tdie = raw_adc / 2;
		break;
	default:
		LOG_DBG(chg, "unsupported channel[%d]\n", channel);
		break;
	}

Err:
	return ret;
}

static void nu2111a_set_wdt_time(struct nu2111a_charger *chg, int time)
{
	int ret = 0;

	ret = nu2111a_update_reg(chg, NU2111A_REG_CONTROL2, NU2111A_BITS_WDT, time);
	LOG_DBG(chg, "SET wdt time[%d], value[0x%x]", time, time);
}

static void nu2111a_enable_wdt(struct nu2111a_charger *chg, bool en)
{
	int ret = 0;

	if (en)
		ret = nu2111a_update_reg(chg, NU2111A_REG_CONTROL2, NU2111A_BITS_WDT, chg->pdata->wd_tmr);
	else
		ret = nu2111a_update_reg(chg, NU2111A_REG_CONTROL2, NU2111A_BITS_WDT, 0);

	LOG_DBG(chg, "set WDT[%d]!!\n", en);
}

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
static void nu2111a_check_wdt_control(struct nu2111a_charger *chg)
{
	struct device *dev = chg->dev;
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);

	pr_info("%s: chg->wdt_kick=%d, chg->pdata->wd_tmr=%d\n", __func__, chg->wdt_kick, chg->pdata->wd_tmr);

	if (chg->wdt_kick) {
		nu2111a_set_wdt_time(chg, chg->pdata->wd_tmr);
		chg->wdt_test_resume = 0;
		schedule_delayed_work(&chg->wdt_control_work, msecs_to_jiffies(NU2111A_BATT_WDT_CONTROL_T));
	} else {
		nu2111a_set_wdt_time(chg, chg->pdata->wd_tmr);
		if (client->addr == 0xff) {
			client->addr = 0x6e;
			i2c_mark_adapter_resumed(client->adapter);
		}

		if (chg->regmap_sub) {
			if (chg->client_sub->addr == 0xff) {
				chg->client_sub->addr = 0x6e;
				i2c_mark_adapter_resumed(chg->client_sub->adapter);
			}
		}
	}
}

static void nu2111a_set_done(struct nu2111a_charger *chg, bool enable)
{
	int ret = 0;
	union power_supply_propval value = {0, };

	value.intval = enable;
	ret = psy_do_property(chg->pdata->sec_dc_name, set,
			POWER_SUPPLY_EXT_PROP_DIRECT_DONE, value);

	if (ret < 0)
		pr_info("%s: error set_done, ret=%d\n", __func__, ret);
}
#endif

static int nu2111a_set_input_current(struct nu2111a_charger *chg, unsigned int iin)//?????  OCP
{
	int ret = 0;
	unsigned int value = 0;
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	if (chg->ta_type == TA_TYPE_PD_FPDO) {
		if (iin < chg->pdata->fpdo_dc_iin_lowest_limit) {
			LOG_DBG(chg, "IIN LOWEST LIMIT! IIN %d -> %d\n",
					iin, chg->pdata->fpdo_dc_iin_lowest_limit);
			iin = chg->pdata->fpdo_dc_iin_lowest_limit;
		}
	}
#endif

	if (chg->regmap_sub) {
		iin = iin/2 + NU2111A_IIN_OFFSET_CUR; //add 200mA-offset
		iin = MAX(iin, NU2111A_PARALLEL_TOPOFF + NU2111A_IIN_OFFSET_CUR);
	} else
		iin = iin + NU2111A_IIN_OFFSET_CUR; //add 200mA-offset

	if (iin >= NU2111A_IIN_CFG_MAX)
		iin = NU2111A_IIN_CFG_MAX;

	value = NU2111A_IIN_TO_HEX(iin);

	ret = nu2111a_update_reg(chg, NU2111A_REG_IBUS_OCP, NU2111A_BITS_IBUS_OCP_TH, value);
	LOG_DBG(chg, "iin hex [0x%x], real iin [%d]\n", value, iin);

	return ret;
}

static int nu2111a_set_input_current_main(struct nu2111a_charger *chg, unsigned int iin)//?????  OCP
{
	int ret = 0;
	unsigned int value = 0;

	iin = iin + NU2111A_IIN_OFFSET_CUR; //add 200mA-offset

	if (iin >= NU2111A_IIN_CFG_MAX)
		iin = NU2111A_IIN_CFG_MAX;

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	if (chg->ta_type == TA_TYPE_PD_FPDO) {
		if (iin < chg->pdata->fpdo_dc_iin_lowest_limit) {
			LOG_DBG(chg, "IIN LOWEST LIMIT! IIN %d -> %d\n",
					iin, chg->pdata->fpdo_dc_iin_lowest_limit);
			iin = chg->pdata->fpdo_dc_iin_lowest_limit;
		}
	}
#endif
	value = NU2111A_IIN_TO_HEX(iin);

	ret = nu2111a_update_reg_main(chg, NU2111A_REG_IBUS_OCP, NU2111A_BITS_IBUS_OCP_TH, value);
	LOG_DBG(chg, "main iin hex [0x%x], real iin [%d]\n", value, iin);

	return ret;
}

static int nu2111a_set_input_current_sub(struct nu2111a_charger *chg, unsigned int iin)//?????  OCP
{
	int ret = 0;
	unsigned int value = 0;

	iin = iin + NU2111A_IIN_OFFSET_CUR; //add 200mA-offset

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	if (chg->ta_type == TA_TYPE_PD_FPDO) {
		if (iin < chg->pdata->fpdo_dc_iin_lowest_limit) {
			LOG_DBG(chg, "IIN LOWEST LIMIT! IIN %d -> %d\n",
					iin, chg->pdata->fpdo_dc_iin_lowest_limit);
			iin = chg->pdata->fpdo_dc_iin_lowest_limit;
		}
	}
#endif
	if (iin >= NU2111A_IIN_CFG_MAX)
		iin = NU2111A_IIN_CFG_MAX;

	value = NU2111A_IIN_TO_HEX(iin);

	ret = nu2111a_update_reg_sub(chg, NU2111A_REG_IBUS_OCP, NU2111A_BITS_IBUS_OCP_TH, value);
	LOG_DBG(chg, "sub iin hex [0x%x], real iin [%d]\n", value, iin);
	return ret;
}

static int nu2111a_set_ocp_current(struct nu2111a_charger *chg, unsigned int iin)
{
	int ret = 0;
	unsigned int value = 0;

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	if (chg->ta_type == TA_TYPE_PD_FPDO) {
		if (iin < chg->pdata->fpdo_dc_iin_lowest_limit) {
			LOG_DBG(chg, "IIN LOWEST LIMIT! IIN %d -> %d\n",
					iin, chg->pdata->fpdo_dc_iin_lowest_limit);
			iin = chg->pdata->fpdo_dc_iin_lowest_limit;
		}
	}
#endif

	if (iin >= NU2111A_IIN_CFG_MAX)
		iin = NU2111A_IIN_CFG_MAX;

	value = NU2111A_IIN_TO_HEX(iin);

	ret = nu2111a_update_reg(chg, NU2111A_REG_IBUS_OCP, NU2111A_BITS_IBUS_OCP_TH, value);
	LOG_DBG(chg, "iin hex [0x%x], real iin [%d]\n", value, iin);

	return ret;
}

static int nu2111a_set_vbat_reg(struct nu2111a_charger *chg, unsigned int vbat_reg)
{
	int ret, val;

	/* vbat regulation voltage */
	if (vbat_reg < NU2111A_VBAT_REG_MIN) {
		vbat_reg = NU2111A_VBAT_REG_MIN;
		pr_info("%s: -> Vbat-REG=%d\n", __func__, vbat_reg);
	}
	val = NU2111A_VBAT_REG(vbat_reg);
	ret = nu2111a_update_reg_main(chg, NU2111A_REG_VBAT_OVP, NU2111A_BITS_VBAT_OVP_TH, val);
	LOG_DBG(chg, "VBAT-REG hex:[0x%x], real:[%d]\n", val, vbat_reg);

	return ret;
}

static int nu2111a_set_charging(struct nu2111a_charger *chg, bool en)
{
	int ret = 0;
	u8 value = 0;
	unsigned int  r_val = 0;

	value = en ? NU2111A_BIT_CHG_EN : (0);
	ret = nu2111a_update_reg(chg, NU2111A_REG_CONTROL1, NU2111A_BIT_CHG_EN, value);
	if (ret < 0)
		goto Err;

	 /* dis pmid to vout uvp, ovp is 5%  0x29-0x22(ovp 7.5%, uvp -3.75%) */
	if (!en) {
		ret = nu2111a_write_reg(chg, NU2111A_REG_PMID2VOUT_OVP_UVP, 0x33);
		if (ret < 0)
			return ret;
	}

	ret = nu2111a_read_reg(chg, NU2111A_REG_CONTROL1, &r_val);
Err:
	LOG_DBG(chg, "End, ret=%d, r_val=[%x]\n", ret, r_val);
	return ret;
}

static int nu2111a_set_charging_main(struct nu2111a_charger *chg, bool en)
{
	int ret = 0;
	u8 value = 0;
	unsigned int  r_val = 0;

	value = en ? NU2111A_BIT_CHG_EN : (0);
	ret = nu2111a_update_reg_main(chg, NU2111A_REG_CONTROL1, NU2111A_BIT_CHG_EN, value);
	if (ret < 0)
		goto Err;

	 /* dis pmid to vout uvp, ovp is 5%  0x29-0x22(ovp 7.5%, uvp -3.75%) */
	if (!en) {
		ret = nu2111a_write_reg_main(chg, NU2111A_REG_PMID2VOUT_OVP_UVP, 0x33);
		if (ret < 0)
			return ret;
	}

	ret = nu2111a_read_reg(chg, NU2111A_REG_CONTROL1, &r_val);
Err:
	LOG_DBG(chg, "End, ret=%d, r_val=[%x]\n", ret, r_val);
	return ret;
}

static int nu2111a_set_charging_sub(struct nu2111a_charger *chg, bool en)
{
	int ret = 0;
	u8 value = 0;
	unsigned int  r_val = 0;

	value = en ? NU2111A_BIT_CHG_EN : (0);
	ret = nu2111a_update_reg_sub(chg, NU2111A_REG_CONTROL1, NU2111A_BIT_CHG_EN, value);
	if (ret < 0)
		goto Err;

	 /* dis pmid to vout uvp, ovp is 5%  0x29-0x22(ovp 7.5%, uvp -3.75%) */
	if (!en) {
		ret = nu2111a_write_reg_sub(chg, NU2111A_REG_PMID2VOUT_OVP_UVP, 0x33);
		if (ret < 0)
			return ret;
	}

	ret = nu2111a_read_reg_sub(chg, NU2111A_REG_CONTROL1, &r_val);
Err:
	LOG_DBG(chg, "End, ret=%d, r_val=[%x]\n", ret, r_val);
	return ret;
}

static int nu2111a_set_reverse_mode(struct nu2111a_charger *chg, bool enable)
{
	int ret, val;

	pr_info("%s: enable=%d\n", __func__, enable);
	val = enable ? NU2111A_BIT_CHG_EN : (0);

	if (enable == true) {
		ret = nu2111a_update_reg(chg, NU2111A_REG_CONTROL2, NU2111A_BITS_SCC_MODE, 0xc0);
		if (ret < 0)
			goto error;
		ret = nu2111a_update_reg(chg, NU2111A_REG_IBUS_OCP, NU2111A_BIT_IBUS_OCP_DIS, 0);
		if (ret < 0)
			goto error;

		ret = nu2111a_set_charging(chg, true);
		if (ret < 0)
			goto error;
	} else {
		ret = nu2111a_set_charging(chg, false);
		if (ret < 0)
			goto error;

		ret = nu2111a_update_reg(chg, NU2111A_REG_IBUS_OCP, NU2111A_BIT_IBUS_OCP_DIS, 1);
		if (ret < 0)
			goto error;
		ret = nu2111a_update_reg(chg, NU2111A_REG_CONTROL2, NU2111A_BITS_SCC_MODE, 0x0);
		if (ret < 0)
			goto error;
	}

error:
	pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;
}
static int nu2111a_stop_charging(struct nu2111a_charger *chg)
{
	int ret = 0;
	unsigned int value;

	if (chg->charging_state != DC_STATE_NOT_CHARGING) {
		LOG_DBG(chg, "charging_state--> Not Charging\n");
		nu2111a_set_reverse_mode(chg, false);
		cancel_delayed_work(&chg->timer_work);
		cancel_delayed_work(&chg->pps_work);
		if (!chg->wdt_test_resume)
			cancel_delayed_work(&chg->wdt_control_work);
#ifdef NU2111A_STEP_CHARGING
		cancel_delayed_work(&chg->step_work);
		chg->current_step = STEP_DIS;
#endif

		mutex_lock(&chg->lock);
		chg->timer_id = TIMER_ID_NONE;
		chg->timer_period = 0;
		mutex_unlock(&chg->lock);

		__pm_relax(chg->monitor_ws);

		nu2111a_set_charging_state(chg, DC_STATE_NOT_CHARGING);
		chg->ret_state = DC_STATE_NOT_CHARGING;
		chg->ta_target_vol = NU2111A_TA_MAX_VOL;
		chg->ta_v_ofs = 0;
		chg->ta_v_ofs_thermal = 0;
		chg->ta_v_ofs_normal = 0;
		chg->prev_iin = 0;
		chg->low_ta = false;
		chg->prev_dec = false;
		chg->prev_inc = INC_NONE;
#if !IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
		chg->pdata->vbat_reg = chg->pdata->vbat_reg_max;
#endif
		chg->iin_high_count = 0;
		chg->adj_max_iin = 0;
		chg->adj_max_vol = 0;
		chg->iin_unbalance = 0;

		mutex_lock(&chg->lock);
		chg->req_new_iin = false;
		chg->req_new_vbat_reg = false;
		mutex_unlock(&chg->lock);

		/* For the abnormal TA detection */
		chg->ab_prev_cur = 0;
		chg->ab_try_cnt = 0;
		chg->ab_ta_connected = false;
		chg->fault_sts_cnt = 0;
		chg->tp_set = 0;
		chg->vbus_ovp_th = NU2111A_VBUS_OVP_TH;
		chg->retry_cnt = 0;
		chg->active_retry_cnt = 0;
		chg->sub_topoff = false;
		chg->chg_state_sub = 0;
#ifdef CONFIG_PASS_THROUGH
		chg->pdata->pass_through_mode = DC_NORMAL_MODE;
		chg->pass_through_mode = DC_NORMAL_MODE;
		/* Set new request flag for pass through mode */
		mutex_lock(&chg->lock);
		chg->req_pt_mode = false;
		mutex_unlock(&chg->lock);
#endif

		nu2111a_enable_adc(chg, false);

		/* when TA disconnected, vin-uvlo occurred, so it will clear the status */
		ret = nu2111a_bulk_read_reg(chg, NU2111A_REG_INT_FLAG1,  &value, 4);
		if (ret < 0) {
			dev_err(chg->dev, "Failed to read reg_int1\n");
			return -EINVAL;
		}
		LOG_DBG(chg, "check reg_int[0x%x]\n", value);

		ret = nu2111a_set_charging(chg, false);

		nu2111a_enable_wdt(chg, WDT_DISABLED);
	} else if (chg->timer_id == TIMER_VBATMIN_CHECK) {
		LOG_DBG(chg, "Charging Stop during VBAT Check !\n");
		cancel_delayed_work(&chg->timer_work);
		mutex_lock(&chg->lock);
		chg->timer_id = TIMER_ID_NONE;
		chg->timer_period = 0;
		mutex_unlock(&chg->lock);
	}

	pr_info("END, ret=%d\n", ret);
	return ret;
}

static int nu2111a_device_init(struct nu2111a_charger *chg)
{
	int ret;
	unsigned int value;
	int i;

	ret = nu2111a_read_reg(chg, REG_DEVICE_ID, &value);
	if (ret < 0) {
		/*Workaround for Raspberry PI*/
		ret = nu2111a_read_reg(chg, REG_DEVICE_ID, &value);
		if (ret < 0) {
			dev_err(chg->dev, "Read DEVICE_ID failed , val[0x%x]\n", value);
			return -EINVAL;
		}
	}
	dev_info(chg->dev, "Read DEVICE_ID [0x%x]\n", value);

	if (chg->regmap_sub) {
		ret = nu2111a_read_reg_sub(chg, REG_DEVICE_ID, &value);
		if (ret < 0) {
			/*Workaround for Raspberry PI*/
			ret = nu2111a_read_reg_sub(chg, REG_DEVICE_ID, &value);
			if (ret < 0) {
				dev_err(&chg->client_sub->dev, "Read DEVICE_ID failed , val[0x%x]\n", value);
				return -EINVAL;
			}
		}
		dev_info(&chg->client_sub->dev, "Read DEVICE_ID SUB [0x%x]\n", value);
	}

	ret = nu2111a_write_reg(chg, NU2111A_REG_IBUS_REG_UCP, 0x06);//0x06 -400mA ibus-reg = 2.6V - 400m
	if (ret < 0)
		goto I2C_FAIL;

	ret = nu2111a_write_reg(chg, NU2111A_REG_BAT_REG, 0x0B);//-200mV
	if (ret < 0)
		goto I2C_FAIL;

	if (chg->regmap_sub) {
		ret = nu2111a_write_reg_sub(chg, NU2111A_REG_BAT_REG, 0x03);//disable sub VBAT REG
		if (ret < 0)
			goto I2C_FAIL;

		ret = nu2111a_write_reg_sub(chg, NU2111A_REG_VBAT_OVP, 0x80);//disable sub VBAT OVP
		if (ret < 0)
			goto I2C_FAIL;
	}

	/* Disable WDT function */
	nu2111a_enable_wdt(chg, WDT_DISABLED);

	/* ADC off */
	ret = nu2111a_write_reg(chg, NU2111A_REG_ADC_CTRL, 0x18);
	if (ret < 0)
		goto I2C_FAIL;

	/* NTC off & IBAT off */
	nu2111a_set_adc_scan(chg, ADC_TS, false);
	nu2111a_set_adc_scan(chg, ADC_IBAT, false);
	nu2111a_set_adc_scan(chg, ADC_VAC, false);

	/* en pmid to vout uvp, ovp is back to 7.5% */
	ret = nu2111a_write_reg(chg, NU2111A_REG_PMID2VOUT_OVP_UVP, 0x33);
	if (ret < 0)
		goto I2C_FAIL;

	/* disable AC OVP ,disable vdrop ovp */
	ret = nu2111a_write_reg(chg, NU2111A_REG_AC_PROT_VDROP, 0x93);
	if (ret < 0)
		goto I2C_FAIL;

	/* dis reg-time-out; A4-AC dis NTC-FET */
	ret = nu2111a_write_reg(chg, NU2111A_REG_CONTROL4, 0xAC);
	if (ret < 0)
		goto I2C_FAIL;

	/* Vbus OVP :10V */
	ret = nu2111a_write_reg(chg, NU2111A_REG_VBUS_OVP, 0x50);
	if (ret < 0)
		goto I2C_FAIL;

	/* 3E 4.35V 3F 4.36V 40 4.37V 41 4.38V; 42 4.39V, 43 4.40V */
	ret = nu2111a_write_reg_main(chg, NU2111A_REG_VBAT_OVP, 0x43);
	if (ret < 0)
		goto I2C_FAIL;

	/* dis bat ocp */
	ret = nu2111a_write_reg(chg, NU2111A_REG_IBAT_OCP, 0xB4);
	if (ret < 0)
		goto I2C_FAIL;

	/* SS Time out 1.28sec */
	ret = nu2111a_write_reg(chg, NU2111A_REG_CONTROL3, 0x80);
	if (ret < 0)
		goto I2C_FAIL;

	/* mask int */
	ret = nu2111a_write_reg(chg, NU2111A_REG_INT_MASK1, 0xff);
	if (ret < 0)
		goto I2C_FAIL;

	/* mask int */
	ret = nu2111a_write_reg(chg, NU2111A_REG_INT_MASK2, 0xff);
	if (ret < 0)
		goto I2C_FAIL;

	/* mask int */
	ret = nu2111a_write_reg(chg, NU2111A_REG_INT_MASK3, 0xff);
	if (ret < 0)
		goto I2C_FAIL;

	/* input current limit */
	ret = nu2111a_set_input_current(chg, chg->pdata->iin_cfg);
	if (ret < 0)
		goto I2C_FAIL;

	/* Set Vbat Regualtion */
	ret = nu2111a_set_vbat_reg(chg, chg->pdata->vbat_reg);
	if (ret < 0)
		goto I2C_FAIL;

	/* default setting */
	chg->new_iin = chg->pdata->iin_cfg;
	chg->new_vbat_reg = chg->pdata->vbat_reg;

	/* clear new iin/vbat_reg flag */
	mutex_lock(&chg->lock);
	chg->req_new_iin = false;
	chg->req_new_vbat_reg = false;
	mutex_unlock(&chg->lock);

	/* 0x07=1000KHZ  0x04=700KHz */
	ret = nu2111a_update_reg(chg, NU2111A_REG_CONTROL1, NU2111A_BITS_FSW_SET, chg->pdata->sw_freq);
	if (ret < 0)
		goto I2C_FAIL;

	chg->pdata->vbat_reg_max = chg->pdata->vbat_reg;
	pr_info("vbat-reg-max[%d]\n", chg->pdata->vbat_reg_max);

	/* For the abnormal TA detection */
	chg->ab_prev_cur = 0;
	chg->ab_try_cnt = 0;
	chg->ab_ta_connected = false;
	chg->fault_sts_cnt = 0;
	chg->tp_set = 0;
	chg->vbus_ovp_th = NU2111A_VBUS_OVP_TH;
	chg->ta_v_offset = NU2111A_TA_V_OFFSET;
	chg->ta_c_offset = NU2111A_IIN_CFG_RANGE_CC*2;
	chg->iin_c_offset = NU2111A_IIN_CFG_OFFSET_CC;
	chg->iin_high_count = 0;
	chg->retry_cnt = 0;
	chg->active_retry_cnt = 0;
	chg->dcerr_retry_cnt = 0;
	chg->dcerr_ta_max_vol = NU2111A_TA_VOL_MIN_DCERR*NU2111A_SEC_DENOM_U_M;
	chg->sub_topoff = false;
	chg->iin_unbalance = 0;
	chg->reverse_suspend = 0;
#ifdef CONFIG_PASS_THROUGH
	chg->pass_through_mode = DC_NORMAL_MODE;
	chg->pdata->pass_through_mode = DC_NORMAL_MODE;
#endif
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	chg->fpdo_dc_iin_topoff = chg->pdata->fpdo_dc_iin_topoff;
	chg->fpdo_dc_vnow_topoff = chg->pdata->fpdo_dc_vnow_topoff;
#endif

	for (i = 8; i < 0x2A; i++) {
		nu2111a_read_reg(chg, i, &value);
		pr_info("[%s] dbg--reg[0x%2x], val[0x%2x]\n", __func__,  i, value);
	}

	return ret;
I2C_FAIL:
	//LOG_DBG(chg, chg, "Failed to update i2c\r\n");
	pr_err("[%s]:: Failed to update i2c\r\n", __func__);
	return ret;
}

static int nu2111a_read_adc(struct nu2111a_charger *chg)
{
	u8 r_data[2];
	u16 raw_adc;
	int ret = 0;

	nu2111a_adc_oneshot(chg, true);
	/* VBUS ADC */
	ret = nu2111a_bulk_read_reg(chg, NU2111A_REG_VBUS_ADC1, r_data, 2);
	if (ret < 0)
		goto Err;
	raw_adc = ((r_data[0] << 8) | r_data[1]);
	chg->adc_vin = raw_adc * NU2111A_SEC_DENOM_U_M;
	pr_info("[%s] vbus------r_data[0]:0x%x, r_data[1]:0x%x ; %d, %d\n",
		__func__, r_data[0], r_data[1], raw_adc, chg->adc_vin);

	/* IBUS ADC */
	ret = nu2111a_bulk_read_reg(chg, NU2111A_REG_IBUS_ADC1, r_data, 2);
	if (ret < 0)
		goto Err;
	raw_adc = ((r_data[0] << 8) | r_data[1]);
	chg->adc_iin = raw_adc * NU2111A_SEC_DENOM_U_M;//uV
	pr_info("[%s] ibus------r_data[0]:0x%x, r_data[1]:0x%x ; %d, %d\n",
		__func__, r_data[0], r_data[1], raw_adc, chg->adc_iin);

	/* VBAT ADC */
	ret = nu2111a_bulk_read_reg(chg, NU2111A_REG_VBAT_ADC1, r_data, 2);
	if (ret < 0)
		goto Err;
	raw_adc = ((r_data[0] << 8) | r_data[1]);
	chg->adc_vbat = raw_adc * NU2111A_SEC_DENOM_U_M;//uV
	pr_info("[%s] vbat------r_data[0]:0x%x, r_data[1]:0x%x ; %d, %d\n",
		__func__, r_data[0], r_data[1], raw_adc, chg->adc_vbat);

	/* TDIE ADC */
	ret = nu2111a_bulk_read_reg(chg, NU2111A_REG_TDIE_ADC1, r_data, 2);
	if (ret < 0)
		goto Err;
	raw_adc = ((r_data[0] << 8) | r_data[1]);
	chg->adc_tdie = raw_adc / 2;
	pr_info("[%s] tdie------r_data[0]:0x%x, r_data[1]:0x%x ; %d, %d\n",
		__func__, r_data[0], r_data[1], raw_adc, chg->adc_tdie);

	/* NTC ADC */
	ret = nu2111a_bulk_read_reg(chg, NU2111A_REG_NTC_ADC1, r_data, 2);
	if (ret < 0)
		goto Err;
	raw_adc = ((r_data[0] << 8) | r_data[1]);
	chg->adc_ntc = (10000*raw_adc)/(1024-2*raw_adc);
	pr_info("[%s] ntc------r_data[0]:0x%x, r_data[1]:0x%x ; %d, %d\n",
		__func__, r_data[0], r_data[1], raw_adc, chg->adc_ntc);

	if (chg->regmap_sub) {
		/* VBUS ADC */
		ret = nu2111a_bulk_read_reg_sub(chg, NU2111A_REG_VBUS_ADC1, r_data, 2);
		if (ret < 0)
			goto Err;
		raw_adc = ((r_data[0] << 8) | r_data[1]);
		chg->adc_vin_sub = raw_adc * NU2111A_SEC_DENOM_U_M;
		pr_info("[%s] vbus------r_data[0]:0x%x, r_data[1]:0x%x ; %d, %d\n",
			__func__, r_data[0], r_data[1], raw_adc, chg->adc_vin_sub);

		/* IBUS ADC */
		ret = nu2111a_bulk_read_reg_sub(chg, NU2111A_REG_IBUS_ADC1, r_data, 2);
		if (ret < 0)
			goto Err;
		raw_adc = ((r_data[0] << 8) | r_data[1]);
		chg->adc_iin_sub = raw_adc * NU2111A_SEC_DENOM_U_M;//uV
		pr_info("[%s] ibus------r_data[0]:0x%x, r_data[1]:0x%x ; %d, %d\n",
			__func__, r_data[0], r_data[1], raw_adc, chg->adc_iin_sub);

		/* VBAT ADC */
		ret = nu2111a_bulk_read_reg_sub(chg, NU2111A_REG_VBAT_ADC1, r_data, 2);
		if (ret < 0)
			goto Err;
		raw_adc = ((r_data[0] << 8) | r_data[1]);
		chg->adc_vbat_sub = raw_adc * NU2111A_SEC_DENOM_U_M;//uV
		pr_info("[%s] vbat------r_data[0]:0x%x, r_data[1]:0x%x ; %d, %d\n",
			__func__, r_data[0], r_data[1], raw_adc, chg->adc_vbat_sub);

		/* TDIE ADC */
		ret = nu2111a_bulk_read_reg_sub(chg, NU2111A_REG_TDIE_ADC1, r_data, 2);
		if (ret < 0)
			goto Err;
		raw_adc = ((r_data[0] << 8) | r_data[1]);
		chg->adc_tdie_sub = raw_adc / 2;
		pr_info("[%s] tdie------r_data[0]:0x%x, r_data[1]:0x%x ; %d, %d\n",
			__func__, r_data[0], r_data[1], raw_adc, chg->adc_tdie_sub);
	}
	nu2111a_adc_oneshot(chg, false);


Err:
	LOG_DBG(chg, "VIN_VBUS %d, IIN_IBUS %d, VBAT %d, TDIE %d\n",
		chg->adc_vin, chg->adc_iin, chg->adc_vbat, chg->adc_tdie);
	if (chg->regmap_sub) {
		LOG_DBG(chg, "VIN_VBUS_SUB %d, IIN_IBUS_SUB %d, VBAT_SUB %d, TDIE_SUB %d\n",
			chg->adc_vin_sub, chg->adc_iin_sub, chg->adc_vbat_sub, chg->adc_tdie_sub);
	}
	return ret;
}

static int nu2111a_check_sts_reg(struct nu2111a_charger *chg)
{
	int ret = 0;
	unsigned int sts3, sts3_sub;
	u8 r_state[7];
	int chg_state, mode, chg_state_sub = DEV_STATE_ACTIVE, mode_sub;
	int iin;
	union power_supply_propval value = {0, };

	/* Abnormal TA detection Flag */
	bool ab_ta_flag = false;

	ret = nu2111a_read_reg(chg, NU2111A_REG_INT_STAT3, &sts3);
	if (ret < 0)
		goto Err;
	if (chg->regmap_sub) {
		ret = nu2111a_read_reg_sub(chg, NU2111A_REG_INT_STAT3, &sts3_sub);
		if (ret < 0)
			goto Err;
	}

	LOG_DBG(chg, "REG10:[0x%x]\n", sts3);
	if (chg->regmap_sub)
		LOG_DBG(chg, "REG10_SUB:[0x%x]\n", sts3_sub);

	chg_state = (sts3 & NU2111A_BIT_CP_ACTIVE_STAT) >> 4;
	if (chg->regmap_sub)
		chg_state_sub = (sts3_sub & NU2111A_BIT_CP_ACTIVE_STAT) >> 4;

	nu2111a_adc_oneshot(chg, true);
	nu2111a_get_adc_channel(chg, ADCCH_VBAT);
	nu2111a_get_adc_channel(chg, ADCCH_IIN);
	nu2111a_get_adc_channel(chg, ADCCH_IBAT);
	nu2111a_adc_oneshot(chg, false);
	iin = chg->adc_iin + chg->adc_iin_sub;

	if (chg->charging_state == DC_STATE_CV_MODE) {
		if (iin < NU2111A_PARALLEL_TOPOFF && chg->regmap_sub) {
			if (chg->iin_cc > NU2111A_PARALLEL_TOPOFF) {
				LOG_DBG(chg, "IIN Total : %d, chg_state_sub : %d\n", iin, chg_state_sub);
				chg->sub_topoff = true;
				chg->chg_state_sub = chg_state_sub;
				chg_state_sub = 1;
			}
		}
	}

	if (chg->sub_topoff && chg->regmap_sub) {
		LOG_DBG(chg, "top off. chg_state_sub : %d\n", chg_state_sub);
		chg->chg_state_sub = chg_state_sub;
		chg_state_sub = 1;
	}

	if (chg->charging_state == DC_STATE_PRESET_DC ||
		chg->charging_state == DC_STATE_CHECK_ACTIVE ||
		chg->charging_state == DC_STATE_ADJUST_CC ||
		chg->charging_state == DC_STATE_START_CC) {
		if (iin >= NU2111A_PARALLEL_TOPOFF - NU2111A_IIN_CFG_RANGE_CC && chg->regmap_sub) {
			LOG_DBG(chg, "IIN Total : %d, sub_on[%d]\n", iin, chg_state_sub);
			if (!chg_state_sub)
				nu2111a_set_charging_sub(chg, true);
		} else if (iin >= chg->iin_cc - NU2111A_IIN_CFG_RANGE_CC
				&& chg->regmap_sub) {
			LOG_DBG(chg, "IIN Total : %d, sub_on[%d]\n", iin, chg_state_sub);
			if (!chg_state_sub)
				nu2111a_set_charging_sub(chg, true);
		} else if (chg->charging_state == DC_STATE_ADJUST_CC
				&& chg->adc_vbat >= chg->pdata->vbat_reg - PD_MSG_TA_VOL_STEP_DIV
				&& chg->regmap_sub) {
			LOG_DBG(chg, "VREG sub_on[%d]\n", chg_state_sub);
			if (!chg_state_sub)
				nu2111a_set_charging_sub(chg, true);
		}
		if (iin < (NU2111A_PARALLEL_TOPOFF + NU2111A_IIN_OFFSET_CUR) && chg->regmap_sub) {
			LOG_DBG(chg, "IIN Total : %d, wait_sub[%d]\n", iin, chg_state_sub);
			chg_state_sub = 1;
		}
	}

	if (chg->adc_vbat >= chg->pdata->vbat_reg) {
		LOG_DBG(chg, "VBAT REG : %d", chg->adc_vbat);
		mode = VBAT_REG_LOOP;
		mode_sub = VBAT_REG_LOOP;
	} else {
		mode = sts3 & NU2111A_BIT_REG_ACTIVE_STAT;
		if (chg->regmap_sub)
			mode_sub = sts3_sub & NU2111A_BIT_REG_ACTIVE_STAT;
	}

	if (chg->regmap_sub) {
		chg->chg_state = chg_state & chg_state_sub;
		chg->reg_state = mode|mode_sub;
	} else {
		chg->chg_state = chg_state;
		chg->reg_state = mode;
	}

	LOG_DBG(chg, "chg->chg_state : %d, chg->reg_state : %d\n", chg->chg_state, chg->reg_state);

	if (chg_state != DEV_STATE_ACTIVE) {
		ret = nu2111a_bulk_read_reg(chg, NU2111A_REG_INT_STAT1, r_state, 7);
		if (ret < 0)
			goto Err;

		LOG_DBG(chg, "0xe:[%x], 0xf:[%x], 0x10:[%x], 0x11:[%x], 0x12:[%x], 0x13:[%x], 0x14:[%x]\n",
			r_state[0], r_state[1], r_state[2], r_state[3], r_state[4], r_state[5], r_state[6]);

		nu2111a_read_adc(chg);

		LOG_DBG(chg, "NU2111A_REG_INT_STAT1 = 0x%02X\n", r_state[0]);

		if (r_state[0] & NU2111A_BIT_VOUT_OVP_STAT) {
			chg->dcerr_reason = VOUT_OVP;
			LOG_DBG(chg, "------ Vout OVP! --------\n");
		}

		if (r_state[0] & NU2111A_BIT_VBAT_OVP_STAT) {
			chg->dcerr_reason = VBAT_OVP;
			LOG_DBG(chg, "------ Vbat OVP! --------\n");
		}

		if (r_state[0] & NU2111A_BIT_IBAT_OCP_STAT) {
			chg->dcerr_reason = IBAT_OCP;
			LOG_DBG(chg, "------ Ibat OCP! --------\n");
		}

		if (r_state[0] & NU2111A_BIT_VBUS_OVP_STAT) {
			chg->dcerr_reason = VIN_OVP;
			LOG_DBG(chg, "------ Vbus OVP! --------\n");
		}

		if (r_state[0] & NU2111A_BIT_IBUS_OCP_STAT) {
			chg->dcerr_reason = IBUS_OCP;
			LOG_DBG(chg, "------ Ibus OCP! --------\n");
		}

		if (r_state[0] & NU2111A_BIT_IBUS_UCP_FALL_STAT) {
			chg->dcerr_reason = IBUS_UCP;
			LOG_DBG(chg, "------ Ibus UCP falling! --------\n");
		}

		if (!(r_state[0] & NU2111A_BIT_ADAPTER_INSERT_STAT) || chg->adc_vin < NU2111A_TA_MIN_VOL_PRESET) {
			chg->dcerr_reason = VIN_UVLO;
			if (chg->prev_iin > NU2111A_IIN_CFG_DFT && chg->dcerr_retry_cnt < 3)
				chg->dcerr_retry_cnt++;
			LOG_DBG(chg, "------ Adapter Not Present[%d] prev_iin [%d]!--------\n", chg->dcerr_retry_cnt, chg->prev_iin);
			ret = -EINVAL;

			goto Err;
		}

		if (!(r_state[0] & NU2111A_BIT_VBAT_INSERT_STAT)) {
			chg->dcerr_reason = VOUT_UVLO;
			LOG_DBG(chg, "------ Vout UVLO!--------\n");
		}

		LOG_DBG(chg, "NU2111A_REG_INT_STAT2 = 0x%02X\n", r_state[1]);

		if (r_state[1] & NU2111A_BIT_TSD_STAT) {
			chg->dcerr_reason = THERMAL_SHUTDOWN;
			LOG_DBG(chg, "------ Thermal Shutdown! --------\n");
		}

		if (r_state[1] & NU2111A_BIT_NTC_FLT_STAT) {
			chg->dcerr_reason = NTC_FLT;
			LOG_DBG(chg, "------ NTC Falling! --------\n");
		}

		if (r_state[1] & NU2111A_BIT_PMID2VOUT_OVP_STAT) {
			chg->dcerr_reason = MID2VOUT_OVP;
			LOG_DBG(chg, "------ PMID/2-VOUT OV! --------\n");
		}

		if (r_state[1] & NU2111A_BIT_PMID2VOUT_UVP_STAT) {
			chg->dcerr_reason = MID2VOUT_UVP;
			LOG_DBG(chg, "------ PMID/2-VOUT UV! --------\n");
		}

		if (r_state[1] & NU2111A_BIT_VBUS_ERRORHI_STAT) {
			chg->dcerr_reason = VBUS_ERR_HI;
			LOG_DBG(chg, "------ Vbus too high! --------\n");
		}

		if (r_state[1] & NU2111A_BIT_VBUS_ERRORLO_STAT) {
			chg->dcerr_reason = VBUS_ERR_LOW;
			LOG_DBG(chg, "------ Vbus too low! --------\n");
		}

		if (r_state[1] & NU2111A_BIT_VDRP_OVP_STAT) {
			chg->dcerr_reason = VDSQRB_OVP;
			LOG_DBG(chg, "------ VDRP OVP status not valid!--------\n");
		}

		if (r_state[1] & NU2111A_BIT_VAC_OVP_STAT) {
			chg->dcerr_reason = VIN_OVP;
			LOG_DBG(chg, "------ AC OVP status not valid!--------\n");
		}

		LOG_DBG(chg, "NU2111A_REG_INT_FLAG1 = 0x%02X\n", r_state[3]);

		if (r_state[3] & NU2111A_BIT_VOUT_OVP_FLAG) {
			chg->dcerr_reason = VOUT_OVP;
			LOG_DBG(chg, "------ VOUT OVP FLAG!--------\n");
		}

		if (r_state[3] & NU2111A_BIT_VBAT_OVP_FLAG) {
			chg->dcerr_reason = VBAT_OVP;
			LOG_DBG(chg, "------ VBAT OVP FLAG!--------\n");
		}

		if (r_state[3] & NU2111A_BIT_IBAT_OCP_FLAG) {
			chg->dcerr_reason = IBAT_OCP;
			LOG_DBG(chg, "------ IBAT OCP FLAG!--------\n");
		}

		if (r_state[3] & NU2111A_BIT_VBUS_OVP_FLAG) {
			chg->dcerr_reason = VIN_OVP;
			LOG_DBG(chg, "------ VIN OVP FLAG!--------\n");
		}

		if (r_state[3] & NU2111A_BIT_IBUS_OCP_FLAG) {
			chg->dcerr_reason = IBUS_OCP;
			LOG_DBG(chg, "------IBUS OCP FLAG!--------\n");
		}

		if (r_state[3] & NU2111A_BIT_IBUS_UCP_FALL_FLAG) {
			chg->dcerr_reason = IBUS_UCP;
			LOG_DBG(chg, "------ IBUS UCP FALL FLAG[%d]!--------\n", chg->retry_cnt);

			value.intval = true;
			psy_do_property(chg->pdata->sec_dc_name, set,
				POWER_SUPPLY_EXT_PROP_DC_IBUSUCP, value);

			if (chg->retry_cnt > 2) {
				chg->retry_cnt = 0;
				if (chg->dcerr_retry_cnt < 3)
					chg->dcerr_retry_cnt++;
			} else
				chg->retry_cnt++;
		}

		LOG_DBG(chg, "NU2111A_REG_INT_FLAG2 = 0x%02X\n", r_state[4]);


		if (r_state[4] & NU2111A_BIT_TSD_FLAG) {
			chg->dcerr_reason = THERMAL_SHUTDOWN;
			LOG_DBG(chg, "------ THERMAL SHUTDOWN FLAG!--------\n");
		}

		if (r_state[4] & NU2111A_BIT_NTC_FLT_FLAG) {
			chg->dcerr_reason = NTC_FLT;
			LOG_DBG(chg, "------ NTC FLT FLAG!--------\n");
		}

		if (r_state[4] & NU2111A_BIT_PMID2VOUT_OVP_FLAG) {
			chg->dcerr_reason = MID2VOUT_OVP;
			LOG_DBG(chg, "------ MID2VOUT OVP FLAG!--------\n");
		}

		if (r_state[4] & NU2111A_BIT_PMID2VOUT_UVP_FLAG) {
			chg->dcerr_reason = MID2VOUT_UVP;
			LOG_DBG(chg, "------ PMID2VOUT UVP FLAG!--------\n");
		}

		if (r_state[4] & NU2111A_BIT_VBUS_ERRORHI_FLAG) {
			chg->dcerr_reason = VBUS_ERR_HI;
			LOG_DBG(chg, "------ VBUS ERRORHI FLAG!--------\n");
		}

		if (r_state[4] & NU2111A_BIT_VBUS_ERRORLO_FLAG) {
			chg->dcerr_reason = VBUS_ERR_LOW;
			LOG_DBG(chg, "------ VBUS ERRORLO FLAG!--------\n");
		}

		if (r_state[4] & NU2111A_BIT_VDRP_OVP_FLAG) {
			chg->dcerr_reason = VDSQRB_OVP;
			LOG_DBG(chg, "------ VDRP OVP FLAG!--------\n");
		}

		if (r_state[4] & NU2111A_BIT_VAC_OVP_FLAG) {
			chg->dcerr_reason = VIN_OVP;
			LOG_DBG(chg, "------ VAC OVP FLAG!--------\n");
		}

		LOG_DBG(chg, "NU2111A_REG_INT_FLAG4 = 0x%02X\n", r_state[6]);

		if (r_state[6] & NU2111A_BIT_VOUT_PRES_FALL_FLAG) {
			chg->dcerr_reason = VOUT_UVLO;
			LOG_DBG(chg, "------ VOUT Presence fall --------\n");
		}

		if (r_state[6] & NU2111A_BIT_VBUS_PRES_FALL_FLAG) {
			chg->dcerr_reason = VIN_UVLO;
			LOG_DBG(chg, "------ VBUS Presence fall --------\n");
		}

		if (r_state[6] & NU2111A_BIT_POWER_NG_FLAG) {
			chg->dcerr_reason = POWER_NG;
			LOG_DBG(chg, "------ Power NG --------\n");
		}

		if (r_state[6] & NU2111A_BIT_WD_TIMEOUT_FLAG) {
			chg->dcerr_reason = WDT_TIMER;
			LOG_DBG(chg, "------ Watchdog Timeout --------\n");
		}

		if (r_state[6] & NU2111A_BIT_SS_TIMEOUT_FLAG) {
			chg->dcerr_reason = SS_TIMEOUT;
			LOG_DBG(chg, "------ Soft-Start Timeout--------\n");
			if (chg->retry_cnt > 2) {
				chg->retry_cnt = 0;
				if (chg->dcerr_retry_cnt < 3)
					chg->dcerr_retry_cnt++;
			} else
				chg->retry_cnt++;
		}

		if (r_state[6] & NU2111A_BIT_REG_TIMEOUT_FLAG)
			LOG_DBG(chg, "------ Loop Regulation Timeout--------\n");

		if (r_state[6] & NU2111A_BIT_PIN_DIAG_FAIL_FLAG) {
			chg->dcerr_reason = PIN_DIAG_FAIL;
			LOG_DBG(chg, "------ Pin Diagnostic Fall--------\n");
		}

		ret = -EINVAL;
	}

	if (chg_state_sub == DEV_STATE_NOT_ACTIVE && chg->regmap_sub) {
		ret = nu2111a_bulk_read_reg_sub(chg, NU2111A_REG_INT_STAT1, r_state, 7);
		if (ret < 0)
			goto Err;

		LOG_DBG(chg, "[SUB]0xe:[%x], 0xf:[%x], 0x10:[%x], 0x11:[%x], 0x12:[%x], 0x13:[%x], 0x14:[%x]\n",
			r_state[0], r_state[1], r_state[2], r_state[3], r_state[4], r_state[5], r_state[6]);

		LOG_DBG(chg, "NU2111A_REG_INT_STAT1 = 0x%02X\n", r_state[0]);

		if (r_state[0] & NU2111A_BIT_VOUT_OVP_STAT) {
			chg->dcerr_reason = VOUT_OVP;
			LOG_DBG(chg, "------ Vout OVP! --------\n");
		}

		if (r_state[0] & NU2111A_BIT_VBAT_OVP_STAT) {
			chg->dcerr_reason = VBAT_OVP;
			LOG_DBG(chg, "------ Vbat OVP! --------\n");
		}

		if (r_state[0] & NU2111A_BIT_IBAT_OCP_STAT) {
			chg->dcerr_reason = IBAT_OCP;
			LOG_DBG(chg, "------ Ibat OCP! --------\n");
		}

		if (r_state[0] & NU2111A_BIT_VBUS_OVP_STAT) {
			chg->dcerr_reason = VIN_OVP;
			LOG_DBG(chg, "------ Vbus OVP! --------\n");
		}

		if (r_state[0] & NU2111A_BIT_IBUS_OCP_STAT) {
			chg->dcerr_reason = IBUS_OCP;
			LOG_DBG(chg, "------ Ibus OCP! --------\n");
		}

		if (r_state[0] & NU2111A_BIT_IBUS_UCP_FALL_STAT) {
			chg->dcerr_reason = IBUS_UCP;
			LOG_DBG(chg, "------ Ibus UCP falling! --------\n");
		}

		if (!(r_state[0] & NU2111A_BIT_ADAPTER_INSERT_STAT) || chg->adc_vin < NU2111A_TA_MIN_VOL_PRESET) {
			chg->dcerr_reason = VIN_UVLO;
			if (chg->prev_iin > NU2111A_IIN_CFG_DFT && chg->dcerr_retry_cnt < 3)
				chg->dcerr_retry_cnt++;
			LOG_DBG(chg, "------ Adapter Not Present[%d] prev_iin [%d]!--------\n", chg->dcerr_retry_cnt, chg->prev_iin);
			ret = -EINVAL;

			goto Err;
		}

		if (!(r_state[0] & NU2111A_BIT_VBAT_INSERT_STAT)) {
			chg->dcerr_reason = VOUT_UVLO;
			LOG_DBG(chg, "------ Vout UVLO!--------\n");
		}

		LOG_DBG(chg, "NU2111A_REG_INT_STAT2 = 0x%02X\n", r_state[1]);

		if (r_state[1] & NU2111A_BIT_TSD_STAT) {
			chg->dcerr_reason = THERMAL_SHUTDOWN;
			LOG_DBG(chg, "------ Thermal Shutdown! --------\n");
		}

		if (r_state[1] & NU2111A_BIT_NTC_FLT_STAT) {
			chg->dcerr_reason = NTC_FLT;
			LOG_DBG(chg, "------ NTC Falling! --------\n");
		}

		if (r_state[1] & NU2111A_BIT_PMID2VOUT_OVP_STAT) {
			chg->dcerr_reason = MID2VOUT_OVP;
			LOG_DBG(chg, "------ PMID/2-VOUT OV! --------\n");
		}

		if (r_state[1] & NU2111A_BIT_PMID2VOUT_UVP_STAT) {
			chg->dcerr_reason = MID2VOUT_UVP;
			LOG_DBG(chg, "------ PMID/2-VOUT UV! --------\n");
		}

		if (r_state[1] & NU2111A_BIT_VBUS_ERRORHI_STAT) {
			chg->dcerr_reason = VBUS_ERR_HI;
			LOG_DBG(chg, "------ Vbus too high! --------\n");
		}

		if (r_state[1] & NU2111A_BIT_VBUS_ERRORLO_STAT) {
			chg->dcerr_reason = VBUS_ERR_LOW;
			LOG_DBG(chg, "------ Vbus too low! --------\n");
		}

		if (r_state[1] & NU2111A_BIT_VDRP_OVP_STAT) {
			chg->dcerr_reason = VDSQRB_OVP;
			LOG_DBG(chg, "------ VDRP OVP status not valid!--------\n");
		}

		if (r_state[1] & NU2111A_BIT_VAC_OVP_STAT) {
			chg->dcerr_reason = VIN_OVP;
			LOG_DBG(chg, "------ AC OVP status not valid!--------\n");
		}

		LOG_DBG(chg, "NU2111A_REG_INT_FLAG1 = 0x%02X\n", r_state[3]);

		if (r_state[3] & NU2111A_BIT_VOUT_OVP_FLAG) {
			chg->dcerr_reason = VOUT_OVP;
			LOG_DBG(chg, "------ VOUT OVP FLAG!--------\n");
		}

		if (r_state[3] & NU2111A_BIT_VBAT_OVP_FLAG) {
			chg->dcerr_reason = VBAT_OVP;
			LOG_DBG(chg, "------ VBAT OVP FLAG!--------\n");
		}

		if (r_state[3] & NU2111A_BIT_IBAT_OCP_FLAG) {
			chg->dcerr_reason = IBAT_OCP;
			LOG_DBG(chg, "------ IBAT OCP FLAG!--------\n");
		}

		if (r_state[3] & NU2111A_BIT_VBUS_OVP_FLAG) {
			chg->dcerr_reason = VIN_OVP;
			LOG_DBG(chg, "------ VIN OVP FLAG!--------\n");
		}

		if (r_state[3] & NU2111A_BIT_IBUS_OCP_FLAG) {
			chg->dcerr_reason = IBUS_OCP;
			LOG_DBG(chg, "------IBUS OCP FLAG!--------\n");
		}

		if (r_state[3] & NU2111A_BIT_IBUS_UCP_FALL_FLAG) {
			chg->dcerr_reason = IBUS_UCP;
			LOG_DBG(chg, "------ IBUS UCP FALL FLAG[%d]!--------\n", chg->retry_cnt);
			if (chg->retry_cnt > 2)
				chg->retry_cnt = 0;
			else
				chg->retry_cnt++;
		}

		LOG_DBG(chg, "NU2111A_REG_INT_FLAG2 = 0x%02X\n", r_state[4]);


		if (r_state[4] & NU2111A_BIT_TSD_FLAG) {
			chg->dcerr_reason = THERMAL_SHUTDOWN;
			LOG_DBG(chg, "------ THERMAL SHUTDOWN FLAG!--------\n");
		}

		if (r_state[4] & NU2111A_BIT_NTC_FLT_FLAG) {
			chg->dcerr_reason = NTC_FLT;
			LOG_DBG(chg, "------ NTC FLT FLAG!--------\n");
		}

		if (r_state[4] & NU2111A_BIT_PMID2VOUT_OVP_FLAG) {
			chg->dcerr_reason = MID2VOUT_OVP;
			LOG_DBG(chg, "------ MID2VOUT OVP FLAG!--------\n");
		}

		if (r_state[4] & NU2111A_BIT_PMID2VOUT_UVP_FLAG) {
			chg->dcerr_reason = MID2VOUT_UVP;
			LOG_DBG(chg, "------ PMID2VOUT UVP FLAG!--------\n");
		}

		if (r_state[4] & NU2111A_BIT_VBUS_ERRORHI_FLAG) {
			chg->dcerr_reason = VBUS_ERR_HI;
			LOG_DBG(chg, "------ VBUS ERRORHI FLAG!--------\n");
		}

		if (r_state[4] & NU2111A_BIT_VBUS_ERRORLO_FLAG) {
			chg->dcerr_reason = VBUS_ERR_LOW;
			LOG_DBG(chg, "------ VBUS ERRORLO FLAG!--------\n");
		}

		if (r_state[4] & NU2111A_BIT_VDRP_OVP_FLAG) {
			chg->dcerr_reason = VDSQRB_OVP;
			LOG_DBG(chg, "------ VDRP OVP FLAG!--------\n");
		}

		if (r_state[4] & NU2111A_BIT_VAC_OVP_FLAG) {
			chg->dcerr_reason = VIN_OVP;
			LOG_DBG(chg, "------ VAC OVP FLAG!--------\n");
		}

		LOG_DBG(chg, "NU2111A_REG_INT_FLAG4 = 0x%02X\n", r_state[6]);

		if (r_state[6] & NU2111A_BIT_VOUT_PRES_FALL_FLAG) {
			chg->dcerr_reason = VOUT_UVLO;
			LOG_DBG(chg, "------ VOUT Presence fall --------\n");
		}

		if (r_state[6] & NU2111A_BIT_VBUS_PRES_FALL_FLAG) {
			chg->dcerr_reason = VIN_UVLO;
			LOG_DBG(chg, "------ VBUS Presence fall --------\n");
		}

		if (r_state[6] & NU2111A_BIT_POWER_NG_FLAG) {
			chg->dcerr_reason = POWER_NG;
			LOG_DBG(chg, "------ Power NG --------\n");
		}

		if (r_state[6] & NU2111A_BIT_WD_TIMEOUT_FLAG) {
			chg->dcerr_reason = WDT_TIMER;
			LOG_DBG(chg, "------ Watchdog Timeout --------\n");
		}

		if (r_state[6] & NU2111A_BIT_SS_TIMEOUT_FLAG) {
			chg->dcerr_reason = SS_TIMEOUT;
			LOG_DBG(chg, "------ Soft-Start Timeout--------\n");
		}

		if (r_state[6] & NU2111A_BIT_REG_TIMEOUT_FLAG)
			LOG_DBG(chg, "------ Loop Regulation Timeout--------\n");

		if (r_state[6] & NU2111A_BIT_PIN_DIAG_FAIL_FLAG) {
			chg->dcerr_reason = PIN_DIAG_FAIL;
			LOG_DBG(chg, "------ Pin Diagnostic Fall--------\n");
		}

		ret = -EINVAL;
	}

Err:
	/* fault_sts_cnt will be increased in CC-MODE, it will be 0 in other modes because charging stops */
	if ((ab_ta_flag) || (chg->tp_set == 1)) {
		chg->fault_sts_cnt++;
		LOG_DBG(chg, "fault_sts_cnt[%d]\n", chg->fault_sts_cnt);
		ret = -EINVAL;
	}
	return ret;
}

static int nu2111a_send_pd_message(struct nu2111a_charger *chg, unsigned int type)
{
	unsigned int pdoIndex, ppsVol, ppsCur;
	int ret = 0;

	//LOG_DBG(chg, "start!\n");

	cancel_delayed_work(&chg->pps_work);

	pdoIndex = chg->ta_objpos;

	ppsVol = chg->ta_vol / NU2111A_SEC_DENOM_U_M;
	ppsCur = chg->ta_cur / NU2111A_SEC_DENOM_U_M;
	LOG_DBG(chg, "msg_type=%d, pdoIndex=%d, ppsVol=%dmV(max_vol=%dmV), ppsCur=%dmA(max_cur=%dmA)\n",
		type, pdoIndex, ppsVol, chg->pdo_max_voltage, ppsCur, chg->pdo_max_current);

	if (chg->test_mode) {
		if (chg->pps_vol != 0 && chg->pps_cur != 0) {
			ppsVol = chg->pps_vol;
			ppsCur = chg->pps_cur;
			LOG_DBG(chg, "@TESTMODE ppsVol=%dmV, ppsCur=%dmA\n",
				ppsVol, ppsCur);
		}
	}

	if (chg->charging_state == DC_STATE_NOT_CHARGING)
		return ret;

	mutex_lock(&chg->lock);
	switch (type) {
	case PD_MSG_REQUEST_APDO:
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
		if (chg->ta_type == TA_TYPE_PD_FPDO) {
			LOG_DBG(chg, "ta_type(%d)! skip pd_select_pps\n", chg->ta_type);
		} else {
			ret = sec_pd_select_pps(pdoIndex, ppsVol, ppsCur);
			if (ret == -EBUSY) {
				LOG_DBG(chg, "request again ret=%d\n", ret);

				msleep(NU2111A_PDMSG_WAIT_T);
				nu2111a_check_sts_reg(chg);
				if (chg->chg_state != DEV_STATE_ACTIVE) {
					if (chg->charging_state != DC_STATE_CHECK_VBAT &&
						chg->charging_state != DC_STATE_PRESET_DC) {
						LOG_DBG(chg, "Not Active. PPS %d\n", chg->charging_state);
						mutex_unlock(&chg->lock);
						return ret;
					}
				}
				ret = sec_pd_select_pps(pdoIndex, ppsVol, ppsCur);
			}
		}
#else
		ret = max77958_select_pps(pdoIndex, ppsVol, ppsCur);
		if (ret == -EBUSY) {
			LOG_DBG(chg, "request again ret=%d\n", ret);

			msleep(100);
			ret = max77958_select_pps(pdoIndex, ppsVol, ppsCur);
		}
#endif
		/* Start pps request timer */
		if (ret == 0)
			schedule_delayed_work(&chg->pps_work, msecs_to_jiffies(NU2111A_PPS_PERIODIC_T));

		break;

	case PD_MSG_REQUEST_FIXED_PDO:
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
		if (chg->ta_type == TA_TYPE_PD_FPDO) {
			LOG_DBG(chg, "ta_type(%d)! skip pd_select_pps\n", chg->ta_type);
		} else {
		ret = sec_pd_select_pps(pdoIndex, ppsVol, ppsCur);
		if (ret == -EBUSY) {
			LOG_DBG(chg, "request again ret=%d\n", ret);
			msleep(NU2111A_PDMSG_WAIT_T);
			ret = sec_pd_select_pps(pdoIndex, ppsVol, ppsCur);
			}
		}
#else
		ret = max77705_select_pps(pdoIndex, ppsVol, ppsCur);
		if (ret == -EBUSY) {
			LOG_DBG(chg, "request again ret=%d\n", ret);
			msleep(100);
			ret = max77958_select_pps(pdoIndex, ppsVol, ppsCur);
		}
#endif
		break;

	default:
		break;
	}
	mutex_unlock(&chg->lock);
	LOG_DBG(chg, "ret = %d\n", ret);
	return ret;
}

static void nu2111a_wdt_control_work(struct work_struct *work)
{
	struct nu2111a_charger *chg = container_of(work, struct nu2111a_charger, wdt_control_work.work);
	struct device *dev = chg->dev;
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);

	if (!chg->wdt_test_resume) {
		nu2111a_set_wdt_time(chg, chg->pdata->wd_tmr);

		/* For kick Watchdog */
		nu2111a_show_reg(chg);

		client->addr = 0xff;
		i2c_mark_adapter_suspended(client->adapter);
		if (chg->regmap_sub) {
			chg->client_sub->addr = 0xff;
			i2c_mark_adapter_suspended(chg->client_sub->adapter);
		}

		pr_info("%s: disable addr (vin:%duV, iin:%duA)\n",
			__func__, chg->adc_vin, chg->adc_iin);

		chg->wdt_test_resume = 1;

		if (chg->pdata->wd_tmr < WD_TMR_30S) {
			pr_info("%s: resume i2c bus after wdt timeout 7sec", __func__);
			schedule_delayed_work(&chg->wdt_control_work, msecs_to_jiffies(NU2111A_BATT_WDT_WAIT_5_T));
		} else {
			pr_info("%s: resume i2c bus after wdt timeout 35sec", __func__);
			schedule_delayed_work(&chg->wdt_control_work, msecs_to_jiffies(NU2111A_BATT_WDT_WAIT_30_T));
		}
	} else {
		pr_info("%s: resume i2c bus", __func__);
		i2c_mark_adapter_resumed(chg->client->adapter);
		if (chg->regmap_sub)
			i2c_mark_adapter_resumed(chg->client_sub->adapter);
	}
}

#ifdef NU2111A_STEP_CHARGING
static void nu2111a_step_charging_cccv_ctrl(struct nu2111a_charger *chg)
{
	int vbat = chg->adc_vbat;

	pr_info("%s: vbat %d chg->current_step %d\n", __func__, vbat, chg->current_step);

	if (vbat < NU2111A_STEP1_VBAT_REG || chg->current_step == 0) {// < 4.16V  4.2V
		/* Step1 Charging! */
		chg->current_step = STEP_ONE;
		pr_info("Step1 Charging Start!\n");
		chg->pdata->vbat_reg = NU2111A_STEP1_VBAT_REG; //4.16V
		chg->pdata->iin_cfg = NU2111A_STEP1_TARGET_IIN;
		chg->pdata->iin_topoff = NU2111A_STEP1_TOPOFF;//1575000 //1575mA
	} else if ((vbat >= NU2111A_STEP1_VBAT_REG) && (vbat < NU2111A_STEP2_VBAT_REG)) { // 4.12V - 4.28V  4.2V-4.3V
		/* Step2 Charging! */
		chg->current_step = STEP_TWO;
		pr_info("Step2 Charging Start!\n");
		chg->pdata->vbat_reg = NU2111A_STEP2_VBAT_REG; //4.29V
		chg->pdata->iin_cfg = NU2111A_STEP2_TARGET_IIN;
		chg->pdata->iin_topoff = NU2111A_STEP2_TOPOFF;//1250000 //1250mA
	} else {  // > 4.28V  > 4.3V
		/* Step3 Charging! */
		chg->current_step = STEP_THREE;
		pr_info("Step3 Charging Start!\n");
		chg->pdata->vbat_reg = NU2111A_STEP3_VBAT_REG;//4420000
		chg->pdata->iin_cfg = NU2111A_STEP3_TARGET_IIN;
		chg->pdata->iin_topoff = NU2111A_STEP3_TOPOFF;//500000  //500mA
	}
}
#endif

#ifdef CONFIG_PASS_THROUGH
static int nu2111a_set_ptmode_ta_current(struct nu2111a_charger *chg)
{
	int ret = 0;
	unsigned int val;

	chg->charging_state = DC_STATE_PT_MODE;
	LOG_DBG(chg, "new_iin[%d]\n", chg->new_iin);

	chg->pdata->iin_cfg = chg->new_iin;
	chg->iin_cc = chg->new_iin;

	/* Set IIN_CFG to IIN_CC */
	ret = nu2111a_set_input_current(chg, chg->iin_cc);
	if (ret < 0)
		goto Err;

	/* Clear Request flag */
	mutex_lock(&chg->lock);
	chg->req_new_iin = false;
	mutex_unlock(&chg->lock);

	val = chg->iin_cc/PD_MSG_TA_CUR_STEP;
	chg->iin_cc = val*PD_MSG_TA_CUR_STEP;
	chg->ta_cur = chg->iin_cc;

	LOG_DBG(chg, "ta_cur[%d], ta_vol[%d]\n", chg->ta_cur, chg->ta_vol);
	chg->iin_cc = chg->new_iin;

	mutex_lock(&chg->lock);
	chg->timer_id = TIMER_PDMSG_SEND;
	chg->timer_period = 0;
	mutex_unlock(&chg->lock);
	schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));

Err:
	LOG_DBG(chg, "ret = %d\n", ret);
	return ret;
}

static int nu2111a_set_ptmode_ta_voltage(struct nu2111a_charger *chg)
{
	int ret = 0;
	int vbat;

	chg->charging_state = DC_STATE_PT_MODE;

	LOG_DBG(chg, "new vbat-reg[%d]\n", chg->new_vbat_reg);

	vbat = chg->adc_vbat;

	ret = nu2111a_set_vbat_reg(chg, chg->new_vbat_reg);
	if (ret < 0)
		goto Err;

	/* clear req_new_vbat_reg */
	mutex_lock(&chg->lock);
	chg->req_new_vbat_reg = false;
	mutex_unlock(&chg->lock);

	if (chg->pdata->pass_through_mode == PTM_2TO1) {
		/* Set TA voltage to 2:1 bypass voltage */
		chg->ta_vol = 2*vbat + NU2111A_PT_VOL_PRE_OFFSET;
		LOG_DBG(chg, "2:1, ta_vol=%d, ta_cur=%d\n",
				chg->ta_vol, chg->ta_cur);
	} else {
		/* Set TA voltage to 1:1 voltage */
		chg->ta_vol = vbat + NU2111A_PT_VOL_PRE_OFFSET;
		LOG_DBG(chg, "1:1, ta_vol=%d, ta_cur=%d\n",
				chg->ta_vol, chg->ta_cur);
	}

	LOG_DBG(chg, "ta-vol[%d], ta_cur[%d], vbat[%d]\n", chg->ta_vol, chg->ta_cur, vbat);

	mutex_lock(&chg->lock);
	chg->timer_id = TIMER_PDMSG_SEND;
	chg->timer_period = 0;
	mutex_unlock(&chg->lock);
	schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));

Err:
	LOG_DBG(chg, "ret=%d\n", ret);
	return ret;
}

static int nu2111a_set_ptmode_ta_voltage_by_soc(struct nu2111a_charger *chg, int delta_soc)
{
	int ret = 0;
	unsigned int prev_ta_vol = chg->ta_vol;

	if (delta_soc > 0) { // decrease soc (ref_soc - soc_now)
		chg->ta_vol -= PD_MSG_TA_VOL_STEP;
	} else if (delta_soc < 0) { // increase soc (ref_soc - soc_now)
		chg->ta_vol += PD_MSG_TA_VOL_STEP;
	} else {
		pr_info("%s: abnormal delta_soc=%d\n", __func__, delta_soc);
		return -1;
	}

	pr_info("%s: delta_soc=%d, prev_ta_vol=%d, ta_vol=%d, ta_cur=%d\n",
		__func__, delta_soc, prev_ta_vol, chg->ta_vol, chg->ta_cur);

	/* Send PD Message */
	mutex_lock(&chg->lock);
	chg->timer_id = TIMER_PDMSG_SEND;
	chg->timer_period = 0;
	mutex_unlock(&chg->lock);
	schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));

	return ret;
}
#endif

static int nu2111a_set_new_vbat_reg(struct nu2111a_charger *chg)
{
	int ret = 0;
	int vbat;
	unsigned int val;

	/* Check the charging state */
	if (chg->charging_state == DC_STATE_NOT_CHARGING) {
		chg->pdata->vbat_reg = chg->new_vbat_reg;
	} else {
		if (chg->req_new_vbat_reg) {
			LOG_DBG(chg, "previous request is not finished yet\n");
			ret = -EBUSY;
		} else {
			//Workaround code for power off charging issue
			//because of too many requests, need to check the previous value here again.
			if (chg->new_vbat_reg == chg->pdata->vbat_reg) {
				LOG_DBG(chg, "new vbat-reg[%dmV] is same, No need to update\n", chg->new_vbat_reg/1000);
				/* cancel delayed_work and  restart queue work again */
				cancel_delayed_work(&chg->timer_work);
				mutex_lock(&chg->lock);
				if (chg->charging_state == DC_STATE_CC_MODE)
					chg->timer_id = TIMER_CHECK_CCMODE;
				else if (chg->charging_state == DC_STATE_CV_MODE)
					chg->timer_id = TIMER_CHECK_CVMODE;
				else if (chg->charging_state == DC_STATE_PT_MODE)
					chg->timer_id = TIMER_CHECK_PTMODE;
				chg->timer_period = 0;
				mutex_unlock(&chg->lock);
				schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
				return 0;
			}
			/* set new vbat reg voltage flag*/
			mutex_lock(&chg->lock);
			chg->req_new_vbat_reg = true;
			mutex_unlock(&chg->lock);

			/* checking charging state */
			if ((chg->charging_state == DC_STATE_CC_MODE) ||
				(chg->charging_state == DC_STATE_CV_MODE) ||
				(chg->charging_state == DC_STATE_PT_MODE)) {
				/* pass through mode */
				if (chg->pass_through_mode != PTM_NONE) {
					LOG_DBG(chg, "cancel queue and set ta-vol in ptmode\n");
					cancel_delayed_work(&chg->timer_work);
					ret = nu2111a_set_ptmode_ta_voltage(chg);
				} else {
					vbat = chg->adc_vbat;

					if (chg->new_vbat_reg > vbat) {
						cancel_delayed_work(&chg->timer_work);
						chg->pdata->vbat_reg = chg->new_vbat_reg;
						ret = nu2111a_set_vbat_reg(chg, chg->pdata->vbat_reg);
						if (ret < 0)
							goto Err;
						/* save the current iin_cc to iin_cfg */
						/* set new IIN to IIN_CC */
						if (chg->req_new_iin)
							chg->iin_cc = chg->new_iin;

						chg->pdata->iin_cfg = chg->iin_cc;
						chg->pdata->iin_cfg = MIN(chg->pdata->iin_cfg, chg->ta_max_cur);
						ret = nu2111a_set_input_current(chg, chg->pdata->iin_cfg);
						if (ret < 0)
							goto Err;

						chg->iin_cc = chg->pdata->iin_cfg;

						/* clear req_new_vbat_reg */
						mutex_lock(&chg->lock);
						chg->req_new_vbat_reg = false;
						mutex_unlock(&chg->lock);

						/*Calculate new TA Maxim Voltage */
						val = chg->iin_cc / PD_MSG_TA_CUR_STEP;
						chg->iin_cc = val * PD_MSG_TA_CUR_STEP;
						val = (chg->ta_max_pwr/100)*POWL/(chg->iin_cc/1000);
						val = val*1000/PD_MSG_TA_VOL_STEP;
						val = val*PD_MSG_TA_VOL_STEP;
						if (chg->ta_max_pwr > NU2111A_TA_25W)
							val = chg->pdo_max_voltage*NU2111A_SEC_DENOM_U_M;
						chg->ta_max_vol = MIN(val, NU2111A_TA_MAX_VOL);
						chg->ta_vol = max(NU2111A_TA_MIN_VOL_PRESET,
								((2 * vbat) + NU2111A_TA_VOL_PRE_OFFSET));
						if (chg->dcerr_retry_cnt > 0) {
							if (chg->dcerr_reason == SS_TIMEOUT ||
								chg->dcerr_reason == IBUS_UCP) {
								chg->ta_vol += NU2111A_TA_VOL_STEP_PRE_CC * chg->dcerr_retry_cnt;
								LOG_DBG(chg, "ucp;ssto cnt=%d,%d", chg->retry_cnt, chg->dcerr_retry_cnt);
							}
						}
						val = chg->ta_vol / PD_MSG_TA_VOL_STEP;
						chg->ta_vol = val * PD_MSG_TA_VOL_STEP;
						chg->ta_vol = MIN(chg->ta_vol, chg->ta_max_vol);

						chg->ta_cur = chg->iin_cc - NU2111A_TA_C_PRE_OFFSET;
						chg->iin_cc = chg->pdata->iin_cfg;
						LOG_DBG(chg, "ta_max_vol=%d, ta_max_cur=%d, ta_max_pwr=%d, iin_cc=%d\n",
							chg->ta_max_vol, chg->ta_max_cur, chg->ta_max_pwr, chg->iin_cc);


						/* Clear the flag for ajdust cc */
						chg->prev_iin = 0;
						chg->prev_inc = INC_NONE;
						chg->prev_dec = false;
						chg->low_ta = false;
						chg->adj_max_iin = 0;
						chg->adj_max_vol = 0;
						chg->iin_unbalance = 0;

						nu2111a_set_charging_state(chg, DC_STATE_ADJUST_CC);
						mutex_lock(&chg->lock);
						chg->timer_id = TIMER_PDMSG_SEND;
						chg->timer_period = 0;
						mutex_unlock(&chg->lock);
						schedule_delayed_work(&chg->timer_work,
							msecs_to_jiffies(chg->timer_period));
					} else {
						LOG_DBG(chg, "invalid vbat-reg[%d] higher than vbat[%d]!!\n",
							chg->new_vbat_reg, vbat);
						ret = -EINVAL;
					}
				}
			} else {
				LOG_DBG(chg, "Unsupported charging state[%d]\n", chg->charging_state);
			}
		}
	}
Err:
	LOG_DBG(chg, "ret = %d\n", ret);
	return ret;
}

static int nu2111a_adjust_ta_current(struct nu2111a_charger *chg)
{
	int ret = 0;
	int val, vbat;

	nu2111a_set_charging_state(chg, DC_STATE_ADJUST_TACUR);

	LOG_DBG(chg, "prev[%d], new[%d]\n", chg->pdata->iin_cfg, chg->iin_cc);
	if (chg->iin_cc > chg->ta_max_cur) {
		chg->iin_cc = chg->ta_max_cur;
		LOG_DBG(chg, "new IIN is higher than TA MAX Current!, iin-cc = ta_max_cur\n");
	}

	if (chg->ta_cur == chg->iin_cc) {
		/* Recover IIN_CC to the new_iin */
		chg->iin_cc = MIN(chg->new_iin, chg->ta_max_cur);

		chg->pdata->iin_cfg = chg->iin_cc;
		ret = nu2111a_set_input_current(chg, chg->iin_cc);
		if (ret < 0)
			goto Err;

		/* Clear req_new_iin */
		mutex_lock(&chg->lock);
		chg->req_new_iin = false;
		mutex_unlock(&chg->lock);

		LOG_DBG(chg, "Done. ta_max_vol=%d, ta_cur=%d, ta_vol=%d, iin_cc=%d\n",
		chg->ta_max_vol, chg->ta_cur, chg->ta_vol, chg->iin_cc);

		/* return charging state to the previous state */
		nu2111a_set_charging_state(chg, chg->ret_state);

		mutex_lock(&chg->lock);
		if (chg->charging_state == DC_STATE_CC_MODE)
			chg->timer_id = TIMER_CHECK_CCMODE;
		else if (chg->charging_state == DC_STATE_CV_MODE)
			chg->timer_id = TIMER_CHECK_CVMODE;
		chg->timer_period = 1000;
		mutex_unlock(&chg->lock);
	} else {
		if (chg->iin_cc >= chg->pdata->iin_cfg ||
			(chg->ret_state == DC_STATE_CV_MODE &&
			chg->iin_cc > chg->ta_cur)) {
			/* New IIN is higher than iin_cfg */
			chg->pdata->iin_cfg = chg->iin_cc;
			/* set IIN_CFG to new IIN */
			ret = nu2111a_set_input_current(chg, chg->iin_cc);
			if (ret < 0)
				goto Err;
			/* Clear req_new_iin */
			mutex_lock(&chg->lock);
			chg->req_new_iin = false;
			mutex_unlock(&chg->lock);
			/* Read ADC-VBAT */
			nu2111a_adc_oneshot(chg, true);
			ret = nu2111a_get_adc_channel(chg, ADCCH_VBAT);
			nu2111a_adc_oneshot(chg, false);
			if (ret < 0)
				goto Err;
			vbat = chg->adc_vbat;
			/* set IIN_CC to MIN(iin, ta_max_cur) */
			chg->iin_cc = MIN(chg->pdata->iin_cfg, chg->ta_max_cur);
			chg->pdata->iin_cfg = chg->iin_cc;

			/* Calculate new TA max current/voltage, it will be using in the dc charging */
			val = chg->iin_cc / PD_MSG_TA_CUR_STEP;
			chg->iin_cc = val * PD_MSG_TA_CUR_STEP;

			val = (chg->ta_max_pwr/100)*POWL/(chg->iin_cc/1000);
			val = val*1000/PD_MSG_TA_VOL_STEP;
			val = val*PD_MSG_TA_VOL_STEP;
			if (chg->ta_max_pwr > NU2111A_TA_25W)
				val = chg->pdo_max_voltage*NU2111A_SEC_DENOM_U_M;
			chg->ta_max_vol = MIN(val, NU2111A_TA_MAX_VOL);
			/* Set TA_CV to MAX[(2*VBAT_ADC + 300mV), 7.5V] */
			chg->ta_vol = max(NU2111A_TA_MIN_VOL_PRESET, ((2 * vbat) + NU2111A_TA_VOL_PRE_OFFSET));
			if (chg->dcerr_retry_cnt > 0) {
				if (chg->dcerr_reason == SS_TIMEOUT ||
					chg->dcerr_reason == IBUS_UCP) {
					chg->ta_vol += NU2111A_TA_VOL_STEP_PRE_CC * chg->dcerr_retry_cnt;
					LOG_DBG(chg, "ucp;ssto cnt=%d,%d", chg->retry_cnt, chg->dcerr_retry_cnt);
				}
			}
			val = chg->ta_vol / PD_MSG_TA_VOL_STEP;
			chg->ta_vol = val * PD_MSG_TA_VOL_STEP;
			chg->ta_vol = MIN(chg->ta_vol, chg->ta_max_vol);

			chg->ta_cur = chg->iin_cc - NU2111A_TA_C_PRE_OFFSET;
			chg->iin_cc = chg->pdata->iin_cfg;

			LOG_DBG(chg, "ta_max_vol=%d, ta_cur=%d, ta_max_cur=%d, iin_cc=%d\n",
			chg->ta_max_vol, chg->ta_cur, chg->ta_max_cur, chg->iin_cc);
			/* clear the flag for adjsut mode */
			chg->prev_iin = 0;
			chg->prev_inc = INC_NONE;
			chg->prev_dec = false;
			chg->low_ta = false;
			chg->adj_max_iin = 0;
			chg->adj_max_vol = 0;
			chg->iin_unbalance = 0;

			/* due to new ta-vol/ta-cur, should go to adjust cc, and pd msg  */
			nu2111a_set_charging_state(chg, DC_STATE_ADJUST_CC);
			mutex_lock(&chg->lock);
			chg->timer_id = TIMER_PDMSG_SEND;
			chg->timer_period = 0;
			mutex_unlock(&chg->lock);
		} else {
			LOG_DBG(chg, "iin_cc < iin_cfg\n");

			if (chg->iin_cc < chg->ta_cur) {
				if (chg->ta_v_ofs > chg->ta_v_ofs_normal)
					chg->ta_v_ofs_normal = chg->ta_v_ofs;
				chg->ta_v_ofs = chg->ta_v_ofs_thermal;
				chg->ta_vol = chg->adc_vbat*2 + chg->ta_v_ofs;
				LOG_DBG(chg, "ta_v_ofs=%d, ta_cur=%d, ta_vol=%d, iin_cc=%d\n",
				chg->ta_v_ofs, chg->ta_cur, chg->ta_vol, chg->iin_cc);
			} else {
				chg->ta_v_ofs = chg->ta_v_ofs_normal;
				chg->ta_vol = chg->adc_vbat*2 + chg->ta_v_ofs;
				LOG_DBG(chg, "ta_v_ofs=%d, ta_cur=%d, ta_vol=%d, iin_cc=%d\n",
				chg->ta_v_ofs, chg->ta_cur, chg->ta_vol, chg->iin_cc);
			}

			/* set ta-cur to IIN_CC */
			val = chg->iin_cc / PD_MSG_TA_CUR_STEP;
			chg->iin_cc = val * PD_MSG_TA_CUR_STEP;
			chg->ta_cur = chg->iin_cc;

			/* pd msg */
			mutex_lock(&chg->lock);
			chg->timer_id = TIMER_PDMSG_SEND;
			chg->timer_period = 0;
			mutex_unlock(&chg->lock);
		}
	}
	schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
Err:
	return ret;
}

static int nu2111a_adjust_ta_voltage(struct nu2111a_charger *chg)
{
	int ret = 0;
	int iin;


	nu2111a_set_charging_state(chg, DC_STATE_ADJUST_TAVOL);

	nu2111a_adc_oneshot(chg, true);
	ret = nu2111a_get_adc_channel(chg, ADCCH_IIN);
	nu2111a_adc_oneshot(chg, false);
	if (ret < 0)
		goto Err;

	iin = chg->adc_iin + chg->adc_iin_sub;

	if (iin > (chg->iin_cc + PD_MSG_TA_CUR_STEP)) {
		/* TA-Cur is higher than target-input-current */
		/* Decrease TA voltage (20mV) */
		LOG_DBG(chg, "iin > iin_cc + 20mV, Dec VOl(20mV)\n");
		chg->ta_vol = chg->ta_vol - PD_MSG_TA_VOL_STEP;
		mutex_lock(&chg->lock);
		chg->timer_id = TIMER_PDMSG_SEND;
		chg->timer_period = 0;
		mutex_unlock(&chg->lock);
	} else {
		if (iin < (chg->iin_cc - PD_MSG_TA_CUR_STEP)) {
			/* TA-CUR is lower than target-input-current */
			/* Compare TA MAX Voltage */
			if (chg->ta_vol == chg->ta_max_vol) {
				/* Set TA current and voltage to the same value */
				LOG_DBG(chg, "ta-vol == ta-max_vol\n");
				/* Clear req_new_iin */
				mutex_lock(&chg->lock);
				chg->req_new_iin = false;
				mutex_unlock(&chg->lock);

				/* return charging state to the previous state */
				nu2111a_set_charging_state(chg, chg->ret_state);

				/* pd msg */
				mutex_lock(&chg->lock);
				chg->timer_id = TIMER_PDMSG_SEND;
				chg->timer_period = 0;
				mutex_unlock(&chg->lock);
			} else {
				/* Increase TA voltage (20mV) */
				LOG_DBG(chg, "inc 20mV\n");
				chg->ta_vol = chg->ta_vol + PD_MSG_TA_VOL_STEP;
				/* pd msg */
				mutex_lock(&chg->lock);
				chg->timer_id = TIMER_PDMSG_SEND;
				chg->timer_period = 0;
				mutex_unlock(&chg->lock);
			}
		} else {
			/* IIN_CC - 50mA < IIN ADC < IIN_CC + 50mA  */
			/* IIN ADC is in valid range */
			/* Clear req_new_iin */
			mutex_lock(&chg->lock);
			chg->req_new_iin = false;
			mutex_unlock(&chg->lock);
			/* return charging state to the previous state */
			nu2111a_set_charging_state(chg, chg->ret_state);

			LOG_DBG(chg, "invalid value!\n");
			mutex_lock(&chg->lock);
			chg->timer_id = TIMER_PDMSG_SEND;
			chg->timer_period = 0;
			mutex_unlock(&chg->lock);
		}
	}
	schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
Err:

	return ret;
}

static int nu2111a_set_new_iin(struct nu2111a_charger *chg)
{
	int ret = 0;

	LOG_DBG(chg, "new_iin[%d]\n", chg->new_iin);

	if (chg->charging_state == DC_STATE_NOT_CHARGING) {
		chg->pdata->iin_cfg = chg->new_iin;
	} else {
		/* Checking the previous request! */
		if (chg->req_new_iin) {
			pr_err("[%s], previous request is not done yet!\n", __func__);
			ret = -EBUSY;
		} else {
			/* Workaround code for power off charging issue */
			/* because of too many requests, need to check the previous value here again. */
			if (chg->new_iin == chg->iin_cc) {
				LOG_DBG(chg, "new iin[%dmA] is same, No need to update\n", chg->new_iin/1000);
				/* cancel delayed_work and  restart queue work again */
				cancel_delayed_work(&chg->timer_work);
				mutex_lock(&chg->lock);
				if (chg->charging_state == DC_STATE_CC_MODE)
					chg->timer_id = TIMER_CHECK_CCMODE;
				else if (chg->charging_state == DC_STATE_CV_MODE)
					chg->timer_id = TIMER_CHECK_CVMODE;
				else if (chg->charging_state == DC_STATE_PT_MODE)
					chg->timer_id = TIMER_CHECK_PTMODE;
				chg->timer_period = 0;
				mutex_unlock(&chg->lock);
				schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
				return 0;
			}
			/* Set new request flag */
			mutex_lock(&chg->lock);
			chg->req_new_iin = true;
			mutex_unlock(&chg->lock);

			if ((chg->charging_state == DC_STATE_CC_MODE) ||
			(chg->charging_state == DC_STATE_CV_MODE) ||
			(chg->charging_state == DC_STATE_PT_MODE)) {
				if (chg->pass_through_mode != PTM_NONE) {
					cancel_delayed_work(&chg->timer_work);
					ret = nu2111a_set_ptmode_ta_current(chg);
				} else {
					/* cancel delayed work */
					cancel_delayed_work(&chg->timer_work);
					/* set new IIN to IIN_CC */
					chg->iin_cc = chg->new_iin;
					/* store current state */
					chg->ret_state = chg->charging_state;

					if (chg->iin_cc < NU2111A_TA_MIN_CUR) {//1000mA
						/* Set ta-cur to 1A */
						chg->ta_cur = NU2111A_TA_MIN_CUR;
						ret = nu2111a_adjust_ta_voltage(chg);
					} else {
						/* Need to adjust the ta current */
						ret = nu2111a_adjust_ta_current(chg);
					}
				}
			} else {
				/* unsupported state! */
				LOG_DBG(chg, "unsupported charging state[%d]!\n", chg->charging_state);
			}
		}
	}
	LOG_DBG(chg, "ret = %d\n", ret);
	return ret;
}

static int nu2111a_bad_ta_detection_trigger(struct nu2111a_charger *chg)
{
	int ret = 0;
	int vbat;
	unsigned int val;

	if (chg->charging_state == DC_STATE_CC_MODE) {
		cancel_delayed_work(&chg->timer_work);
		nu2111a_adc_oneshot(chg, true);
		ret = nu2111a_get_adc_channel(chg, ADCCH_VBAT);
		nu2111a_adc_oneshot(chg, false);
		if (ret < 0)
			goto Err;
		vbat = chg->adc_vbat;
		LOG_DBG(chg, "Vbat[%d]\n", vbat);

		/*Calculate new TA Maxim Voltage */
		val = chg->iin_cc / PD_MSG_TA_CUR_STEP;
		chg->iin_cc = val * PD_MSG_TA_CUR_STEP;
		val = (chg->ta_max_pwr/100)*POWL/(chg->iin_cc/1000);
		val = val*1000/PD_MSG_TA_VOL_STEP;
		val = val*PD_MSG_TA_VOL_STEP;
		if (chg->ta_max_pwr > NU2111A_TA_25W)
			val = chg->pdo_max_voltage*NU2111A_SEC_DENOM_U_M;
		chg->ta_max_vol = MIN(val, NU2111A_TA_MAX_VOL);

		chg->ta_vol = max(NU2111A_TA_MIN_VOL_PRESET, ((2 * vbat) + NU2111A_TA_VOL_PRE_OFFSET));
		val = chg->ta_vol / PD_MSG_TA_VOL_STEP;
		chg->ta_vol = val * PD_MSG_TA_VOL_STEP;
		chg->ta_vol = MIN(chg->ta_vol, chg->ta_max_vol);

		chg->ta_cur = chg->iin_cc - NU2111A_TA_C_PRE_OFFSET;
		chg->iin_cc = chg->pdata->iin_cfg;
		LOG_DBG(chg, "ta_max_vol=%d, ta_max_cur=%d, ta_max_pwr=%d, iin_cc=%d, TA-VOL[%d], TA-CUR[%d]\n",
			chg->ta_max_vol, chg->ta_max_cur, chg->ta_max_pwr,
			chg->iin_cc, chg->ta_vol, chg->ta_cur);

		/* Clear the flag for ajdust cc */
		chg->prev_iin = 0;
		chg->prev_inc = INC_NONE;
		chg->tp_set = 0;
		chg->prev_dec = false;
		chg->iin_high_count = 0;
		chg->low_ta = false;
		chg->adj_max_iin = 0;
		chg->adj_max_vol = 0;
		chg->iin_unbalance = 0;

		nu2111a_set_charging_state(chg, DC_STATE_ADJUST_CC);
		mutex_lock(&chg->lock);
		chg->timer_id = TIMER_PDMSG_SEND;
		chg->timer_period = 0;
		mutex_unlock(&chg->lock);
		schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));

		ret = 0;
	} else {
		/* Don't need to do anything when charging state isn't in CCMODE */
		ret = -EINVAL;
	}
Err:
	LOG_DBG(chg, "ret=%d\n", ret);
	return ret;
}

static int nu2111a_charge_start_reverse(struct nu2111a_charger *chg)
{
	int ret = 0;

	pr_info("%s: ======START=======\n", __func__);

	chg->charging_state = DC_STATE_REVERSE_MODE;

	nu2111a_enable_adc(chg, true);

	ret = nu2111a_set_reverse_mode(chg, true);
	if (ret < 0)
		goto error;

	/* Go to reverse mode */
	mutex_lock(&chg->lock);
	chg->timer_id = TIMER_CHECK_REVERSE_ACTIVE;
	chg->timer_period = REVERSE_WAIT_T;
	mutex_unlock(&chg->lock);
	schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));

error:
	pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;
}

/* Check active status before entering reverse mode */
static int nu2111a_check_reverse_active_state(struct nu2111a_charger *chg)
{
	int ret = 0;

	pr_info("%s: ======START=======\n", __func__);

	chg->charging_state = DC_STATE_REVERSE_MODE;

	ret = nu2111a_check_sts_reg(chg);

	if (ret < 0) {
		if (chg->active_retry_cnt > 10)
			goto Err;
		chg->active_retry_cnt++;
	}

	if (chg->chg_state == DEV_STATE_ACTIVE) {
		LOG_DBG(chg, "Reverse Active State\n");
		chg->active_retry_cnt = 0;
		ret = nu2111a_write_reg(chg, NU2111A_REG_PMID2VOUT_OVP_UVP, 0x12);

		mutex_lock(&chg->lock);
		chg->timer_id = TIMER_CHECK_REVERSE_MODE;
		chg->timer_period = 0;
		mutex_unlock(&chg->lock);
		schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
		ret = 0;
	} else if (chg->chg_state == DEV_STATE_NOT_ACTIVE) {
		LOG_DBG(chg, "Reverse Not Active State\n");
		ret = nu2111a_write_reg(chg, NU2111A_REG_PMID2VOUT_OVP_UVP, 0x33);

		ret = nu2111a_set_charging(chg, false);
		mutex_lock(&chg->lock);
		chg->timer_id = TIMER_START_REVERSE;
		chg->timer_period = 0;
		mutex_unlock(&chg->lock);
		schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
		ret = 0;
	} else {
		LOG_DBG(chg, "invalid charging state=%d\n", chg->chg_state);
		ret = -EINVAL;
	}

Err:
	LOG_DBG(chg, "End!, ret = %d\n", ret);
	return ret;
}

/* Check reverse mode status in polling time */
static int nu2111a_charge_reverse_mode(struct nu2111a_charger *chg)
{
	int ret = 0;
	int vin, iin, vbat;

	pr_info("%s: =========START=========\n", __func__);
	if (chg->reverse_suspend)
		goto out;

	__pm_stay_awake(chg->monitor_ws);

	ret = nu2111a_check_sts_reg(chg);
	if (ret < 0) {
		/* Error happens and stop reverse mode */
		pr_info("%s: Error happens in reverse mode\n", __func__);
	} else {
		/* Check reverse mode status in polling time */
		/* Read VIN_ADC, IIN_ADC, and VBAT_ADC */
		vin = chg->adc_vin;
		iin = chg->adc_iin;
		vbat = chg->adc_vbat;
		pr_info("%s: reverse mode, vin=%d, iin=%d, vbat=%d\n",
				__func__, vin, iin, vbat);

		/* Set timer - 5s */
		mutex_lock(&chg->lock);
		chg->timer_id = TIMER_CHECK_REVERSE_MODE;
		chg->timer_period = REVERSE_CHECK_T;	/* 10s */
		mutex_unlock(&chg->lock);
		schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
	}

	__pm_relax(chg->monitor_ws);
out:
	pr_info("%s: End, ret=%d suspend:%d\n", __func__, ret, chg->reverse_suspend);
	return ret;
}
#ifdef CONFIG_PASS_THROUGH
/* Set new direct charging mode */
static int nu2111a_set_pass_through_mode(struct nu2111a_charger *chg)
{
	int ret = 0;
	int vbat;

	LOG_DBG(chg, "======START=======\n");

	/* Check new pt mode */
	switch (chg->pass_through_mode) {
	case PTM_1TO1:
	case PTM_2TO1:
		/* Change normal mode to 1:1 or 2:1 bypass mode */
		/* Check current pt mode */
		if ((chg->pdata->pass_through_mode == PTM_1TO1) ||
			(chg->pdata->pass_through_mode == PTM_2TO1)) {
			ret = nu2111a_update_reg(chg, NU2111A_REG_IBUS_REG_UCP,
				NU2111A_BIT_IBUS_UCP_DIS, NU2111A_BIT_IBUS_UCP_DIS);
			if (ret < 0)
				goto error;

			LOG_DBG(chg, "PT mode=%d\n",
					chg->pass_through_mode);

			/* Enable Charging - recover dc as pt mode */
			ret = nu2111a_set_charging(chg, true);
			if (ret < 0)
				goto error;

			/* Clear request flag */
			mutex_lock(&chg->lock);
			chg->req_pt_mode = false;
			mutex_unlock(&chg->lock);

			mutex_lock(&chg->lock);
			chg->timer_id = TIMER_CHECK_PTMODE;
			chg->timer_period = NU2111A_CHECK_ACTIVE_DELAY_T;
			mutex_unlock(&chg->lock);
			schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
		} else {
			/* DC mode is normal mode */
			/* TA voltage is not changed to 1:1 or 2:1 pt mode yet */
			/* Disable charging */
			ret = nu2111a_set_charging(chg, false);

			msleep(50);

			/* Read VBAT ADC */
			nu2111a_adc_oneshot(chg, true);
			nu2111a_get_adc_channel(chg, ADCCH_VBAT);
			nu2111a_adc_oneshot(chg, false);
			vbat = chg->adc_vbat;
			if (ret < 0)
				goto error;
			/* Set dc mode to new dc mode, 1:1 or 2:1 pt mode */
			chg->pdata->pass_through_mode = chg->pass_through_mode;

			chg->ta_vol = 2*vbat + NU2111A_PT_VOL_PRE_OFFSET + chg->retry_cnt*NU2111A_TA_VOL_STEP_ADJ_CC;

			if (chg->dcerr_retry_cnt > 0) {
				if (chg->dcerr_reason == SS_TIMEOUT ||
					chg->dcerr_reason == IBUS_UCP) {
					chg->ta_vol += NU2111A_TA_VOL_STEP_PRE_CC * chg->dcerr_retry_cnt;
					LOG_DBG(chg, "ucp;ssto cnt=%d,%d", chg->retry_cnt, chg->dcerr_retry_cnt);
				}
			}
			LOG_DBG(chg, "New PT mode, Normal->2:1, ta_vol=%d, ta_cur=%d\n",
					chg->ta_vol, chg->ta_cur);

			/* Send PD Message and go to dcmode change state */
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
			nu2111a_set_charging_state(chg, DC_STATE_PTMODE_CHANGE);
#else
			chg->charging_state = DC_STATE_PTMODE_CHANGE;
#endif
			mutex_lock(&chg->lock);
			chg->timer_id = TIMER_PDMSG_SEND;
			chg->timer_period = 0;
			mutex_unlock(&chg->lock);
			schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
		}
		break;

	case PTM_NONE:
		/* Change bypass mode to normal mode */
		/* Disable charging */
		ret = nu2111a_set_charging(chg, false);
		if (ret < 0)
			goto error;

		ret = nu2111a_update_reg(chg, NU2111A_REG_IBUS_REG_UCP,
			NU2111A_BIT_IBUS_UCP_DIS, 0);
		if (ret < 0)
			goto error;

		/* Set pt mode to new pt mode, normal mode */
		chg->pdata->pass_through_mode = chg->pass_through_mode;
		chg->dcerr_retry_cnt = 0;
		chg->retry_cnt = 0;
		/* Clear request flag */
		mutex_lock(&chg->lock);
		chg->req_pt_mode = false;
		mutex_unlock(&chg->lock);
		LOG_DBG(chg, "New PT mode, pt->Normal\n");

		/* Go to DC_STATE_PRESET_DC */
		mutex_lock(&chg->lock);
		chg->timer_id = TIMER_PRESET_DC;
		chg->timer_period = 0;
		mutex_unlock(&chg->lock);
		schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));

		break;

	default:
		ret = -EINVAL;
		LOG_DBG(chg, "New PT mode, Invalid mode=%d\n", chg->pdata->pass_through_mode);
		break;
	}

error:
	LOG_DBG(chg, "ret=%d\n", ret);
	return ret;
}

static int nu2111a_pass_through_mode_process(struct nu2111a_charger *chg)
{
	int ret = 0;
	int iin, mode, dev_state;

	LOG_DBG(chg, "Start pt mode process!\n");
	nu2111a_set_charging_state(chg, DC_STATE_PT_MODE);

	ret = nu2111a_check_sts_reg(chg);
	if (ret < 0)
		goto Err;
	mode = chg->reg_state;
	dev_state = chg->chg_state;

	if (chg->timer_period == NU2111A_PT_ACTIVE_DELAY_T)
		ret = nu2111a_write_reg(chg, NU2111A_REG_PMID2VOUT_OVP_UVP, 0x12);

	if (chg->req_pt_mode) {
		LOG_DBG(chg, "update pt_mode flag in pt mode\n");
		mutex_lock(&chg->lock);
		chg->req_pt_mode = false;
		mutex_unlock(&chg->lock);
		ret = nu2111a_set_pass_through_mode(chg);
	} else if (chg->req_new_vbat_reg) {
		LOG_DBG(chg, "update new vbat in pt mode\n");
		mutex_lock(&chg->lock);
		chg->req_new_vbat_reg = false;
		mutex_unlock(&chg->lock);
		ret = nu2111a_set_new_vbat_reg(chg);
	} else if (chg->req_new_iin) {
		mutex_lock(&chg->lock);
		chg->req_new_iin = false;
		mutex_unlock(&chg->lock);
		ret = nu2111a_set_new_iin(chg);
	} else {
		iin = chg->adc_iin + chg->adc_iin_sub;

		switch (mode) {
		case INACTIVE_LOOP:
			LOG_DBG(chg, "Inactive loop\n");
			break;
		case VBAT_REG_LOOP:
			LOG_DBG(chg, "vbat-reg loop in pass through mode\n");
			break;
		case IIN_REG_LOOP:
			LOG_DBG(chg, "iin-reg loop in pass through mode\n");
			if (chg->ta_cur <= chg->iin_cc - NU2111A_PT_TA_CUR_LOW_OFFSET) {
				/* IIN_LOOP still happens even though TA current is less than IIN_CC - 200mA */
				chg->ta_vol = chg->ta_vol - PD_MSG_TA_VOL_STEP;
				LOG_DBG(chg, "[iin-reg]Abnormal behavior! iin[%d], TA-CUR[%d], TA-VOL[%d]\n",
				iin, chg->ta_cur, chg->ta_vol);
			} else {
				chg->ta_cur = chg->ta_cur - PD_MSG_TA_CUR_STEP;
				LOG_DBG(chg, "[iin-reg] iin[%d], TA-CUR[%d], TA-VOL[%d]\n",
					iin, chg->ta_cur, chg->ta_vol);
			}
			mutex_lock(&chg->lock);
			chg->timer_id = TIMER_PDMSG_SEND;
			chg->timer_period = 0;
			mutex_unlock(&chg->lock);
			schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
			return ret;
		case IBAT_REG_LOOP:
			LOG_DBG(chg, "ibat-reg loop in pass through mode\n");
			break;
		case T_DIE_REG_LOOP:
			LOG_DBG(chg, "tdie-reg loop in pass through mode\n");
			break;
		default:
			LOG_DBG(chg, "No cases!\n");
			break;
		}
#ifdef CONFIG_PASS_THROUGH
		/* Because PD IC can't know unpluged-detection, will send pd-msg every 1secs */
		mutex_lock(&chg->lock);
		chg->timer_id = TIMER_PDMSG_SEND;
		chg->timer_period = NU2111A_PTMODE_UNPLUGED_DETECTION_T;
		mutex_unlock(&chg->lock);
		schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
#else
		/* every 10s, Pass through mode function works */
		mutex_lock(&chg->lock);
		chg->timer_id = TIMER_CHECK_PTMODE;
		chg->timer_period = NU2111A_PTMODE_DELAY_T;
		mutex_unlock(&chg->lock);
		schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
#endif
	}

Err:
	if (ret < 0) {
		LOG_DBG(chg, "Retry %d DCERR:0x%x\n", chg->retry_cnt, chg->dcerr_reason);
		if (chg->retry_cnt > 0) {
				mutex_lock(&chg->lock);
				chg->timer_id = TIMER_PTMODE_CHANGE;
				chg->timer_period = 100;
				chg->pdata->pass_through_mode = PTM_NONE;
				mutex_unlock(&chg->lock);
				ret = 0;
				schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
		}
	}
	return ret;
}

/* Direct Charging DC mode Change Control */
static int nu2111a_ptmode_change(struct nu2111a_charger *chg)
{
	int ret = 0;

	LOG_DBG(chg, "======START=======\n");

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	nu2111a_set_charging_state(chg, DC_STATE_PTMODE_CHANGE);
#else
	chg->charging_state = DC_STATE_PTMODE_CHANGE;
#endif

	ret = nu2111a_set_pass_through_mode(chg);

	LOG_DBG(chg, "End, ret=%d\n", ret);
	return ret;
}

#endif

static int nu2111a_preset_dcmode(struct nu2111a_charger *chg)
{
	int vbat;
	int ret = 0;
	unsigned int val;
	unsigned int vfloat;

	LOG_DBG(chg, "Start! dcerrcnt : %d\n", chg->dcerr_retry_cnt);

	nu2111a_set_charging_state(chg, DC_STATE_PRESET_DC);

	nu2111a_adc_oneshot(chg, true);
	ret = nu2111a_get_adc_channel(chg, ADCCH_VBAT);
	nu2111a_adc_oneshot(chg, false);
	if (ret < 0)
		goto Err;

	vbat = chg->adc_vbat;
	vfloat = chg->float_voltage * NU2111A_SEC_DENOM_U_M;
	if (vfloat > chg->pdata->vbat_reg) {
		LOG_DBG(chg, "float voltage diff %d\n", chg->pdata->vbat_reg);
		if (chg->pdata->vbat_reg < vbat)
			chg->pdata->vbat_reg = vfloat;
	}
	LOG_DBG(chg, "Vbat is [%d] float voltage %d\n", vbat, vfloat);

#ifdef NU2111A_STEP_CHARGING
	nu2111a_step_charging_cccv_ctrl(chg);
#endif
	//need to check vbat to verify the start level of DC charging
	//need to check whether DC mode or not.

	if (chg->ta_type == TA_TYPE_PD_FPDO) {
		/* TA type is USBPD 2.0 and support only FPDO */
		LOG_DBG(chg, "ta type : fixed PDO\n");

		nu2111a_write_reg(chg, NU2111A_REG_VBUS_OVP, 0x50);
		nu2111a_write_reg(chg, NU2111A_REG_PMID2VOUT_OVP_UVP, 0xB3);

		if (ret < 0)
			goto Err;

		/* Set PDO object position to 9V FPDO */
		chg->ta_objpos = 2;
		/* Set TA voltage to 9V */
		chg->ta_vol = 9000000;
		/* Set TA maximum voltage to 9V */
		chg->ta_max_vol = 9000000;
		/* Set IIN_CC to iin_cfg */
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
		chg->iin_cc = chg->pdata->fpdo_dc_iin_lowest_limit;
#else
		chg->iin_cc = chg->pdata->iin_cfg;
#endif
		/* Set TA operating current and maximum current to iin_cc */
		chg->ta_cur = chg->iin_cc;
		chg->ta_max_cur = chg->iin_cc;
		/* Calculate TA maximum power */
		chg->ta_max_pwr = (chg->ta_max_vol/NU2111A_SEC_DENOM_U_M)*(chg->ta_max_cur/NU2111A_SEC_DENOM_U_M);

		LOG_DBG(chg, "Preset DC(FPDO), ta_max_vol=%d, ta_max_cur=%d, ta_max_pwr=%d, iin_cc=%d\n",
			chg->ta_max_vol, chg->ta_max_cur, chg->ta_max_pwr, chg->iin_cc);
	} else {
		chg->ta_max_cur = chg->pdata->iin_cfg;
		chg->ta_max_vol = NU2111A_TA_MAX_VOL;
		chg->ta_objpos = 0;

		ret = nu2111a_get_apdo_max_power(chg);
		if (ret < 0) {
			pr_err("%s: TA doesn't support any APDO\n", __func__);
			goto Err;
		}

		chg->iin_cc = MIN(chg->pdata->iin_cfg, chg->ta_max_cur);
		chg->pdata->iin_cfg = chg->iin_cc;

		val = chg->iin_cc / PD_MSG_TA_CUR_STEP;
		chg->iin_cc = val * PD_MSG_TA_CUR_STEP;

		if (chg->dcerr_retry_cnt > 1 &&
			chg->dcerr_reason == VIN_UVLO) {
			val = chg->dcerr_ta_max_vol/1000 - 2*(NU2111A_TA_VOL_STEP_ADJ_CC/1000);
			LOG_DBG(chg, "dcerr_ta_max_vol=%d, val=%d", chg->dcerr_ta_max_vol/1000, val);
			val = MAX(val, NU2111A_TA_VOL_MIN_DCERR);
		} else
			val = (chg->ta_max_pwr/100)*POWL/(chg->iin_cc/1000);

		val = (val * 1000) / PD_MSG_TA_VOL_STEP;
		val = val * PD_MSG_TA_VOL_STEP;
		if (chg->ta_max_pwr > NU2111A_TA_25W)
			val = chg->pdo_max_voltage*NU2111A_SEC_DENOM_U_M;
		chg->ta_max_vol = MIN(val, chg->ta_max_vol);

		/* Set TA_CV to MAX[(2*VBAT_ADC + 300mV), 7.5V] */
		/* NU2111A_TA_MIN_VOL_PRESET = 7.5V,300mV */
		chg->ta_vol = max(NU2111A_TA_MIN_VOL_PRESET, ((2 * vbat) + NU2111A_TA_VOL_PRE_OFFSET
					+ chg->retry_cnt*NU2111A_TA_VOL_STEP_ADJ_CC));

		if (chg->dcerr_retry_cnt > 0) {
			if (chg->dcerr_reason == SS_TIMEOUT ||
				chg->dcerr_reason == IBUS_UCP) {
				chg->ta_vol += NU2111A_TA_VOL_STEP_PRE_CC * chg->dcerr_retry_cnt;
				LOG_DBG(chg, "ucp;ssto cnt=%d,%d", chg->retry_cnt, chg->dcerr_retry_cnt);
			}
		}

		val = chg->ta_vol / PD_MSG_TA_VOL_STEP;//20mV
		chg->ta_vol = val * PD_MSG_TA_VOL_STEP;

		chg->ta_vol = MIN(chg->ta_vol, chg->ta_max_vol);// < 9.8V
		chg->ta_cur = chg->iin_cc - NU2111A_TA_C_PRE_OFFSET;
		chg->iin_cc = chg->pdata->iin_cfg;

		LOG_DBG(chg, "ta_max_vol=%d, ta_max_cur=%d, ta_max_pwr=%d, iin_cc=%d, TA-VOL[%d], TA-CUR[%d], Retry[%d]\n",
		chg->ta_max_vol, chg->ta_max_cur,
		chg->ta_max_pwr, chg->iin_cc, chg->ta_vol, chg->ta_cur, chg->retry_cnt);
	}

	mutex_lock(&chg->lock);
	chg->timer_id = TIMER_PDMSG_SEND;
	chg->timer_period = 0;
	mutex_unlock(&chg->lock);
	schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));

Err:
	LOG_DBG(chg, "End, ret =%d\n", ret);
	return ret;
}

static int nu2111a_preset_config(struct nu2111a_charger *chg)
{
	int ret;

	LOG_DBG(chg, "Start!, set iin:[%d], Vbat-REG:[%d]\n", chg->iin_cc, chg->pdata->vbat_reg);

	nu2111a_set_charging_state(chg, DC_STATE_PRESET_DC);

	/* Enable IBUS UCP */
	ret = nu2111a_update_reg(chg, NU2111A_REG_IBUS_REG_UCP,
		NU2111A_BIT_IBUS_UCP_DIS, 0);
	ret = nu2111a_update_reg(chg, NU2111A_REG_IBUS_OCP,
		NU2111A_BIT_IBUS_OCP_DIS, 1);

	ret = nu2111a_update_reg(chg, NU2111A_REG_CONTROL2, NU2111A_BITS_SCC_MODE, 0x0);

	/* Set IIN_CFG to IIN_CC */
	ret = nu2111a_set_input_current(chg, chg->iin_cc);
	if (ret < 0)
		goto error;

	ret = nu2111a_update_reg(chg, 0x04, 0x80, (0x01<<7));//dis ibus ocp
	if (ret < 0)
		goto error;

	/* Set Vbat Regualtion */
	ret = nu2111a_set_vbat_reg(chg, chg->pdata->vbat_reg);
	if (ret < 0)
		goto error;

	/* Enable nu2111a */
	ret = nu2111a_set_charging_main(chg, true);
	if (ret < 0)
		goto error;

	/* Clear previous iin adc */
	chg->prev_iin = 0;
	chg->iin_high_count = 0;
	chg->low_ta = false;
	chg->adj_max_iin = 0;
	chg->adj_max_vol = 0;
	chg->iin_unbalance = 0;
	chg->chg_state_sub = 0;

	/* Clear TA increment flag */
	chg->prev_inc = INC_NONE;
	chg->prev_dec = false;

	/* Go to CHECK_ACTIVE state after 150ms*/
	mutex_lock(&chg->lock);
	chg->timer_id = TIMER_CHECK_ACTIVE;
	chg->timer_period = NU2111A_CHECK_ACTIVE_DELAY_T;
	mutex_unlock(&chg->lock);
	schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
	ret = 0;

error:
	LOG_DBG(chg, "End, ret=%d\n", ret);
	return ret;
}


static int nu2111a_start_dc_charging(struct nu2111a_charger *chg)
{
	int ret;
	unsigned int val;
	u8 ints[4];
	union power_supply_propval prop_val = {0, };

	LOG_DBG(chg, "start! Ver[%s]\n", NU2111A_MODULE_VERSION);

	/* before charging starts, read on clear the status registers */
	ret = nu2111a_bulk_read_reg(chg, NU2111A_REG_INT_FLAG1, ints, 4);
	LOG_DBG(chg, "reg11-14: ints1[0x%x], ints2[0x%x], ints3[0x%x], ints4[0x%x]\n",
		ints[0], ints[1], ints[2], ints[3]);
	if (ret < 0)
		return ret;

	ret = nu2111a_read_reg(chg, NU2111A_REG_CONTROL4, &val);
	if (ret < 0)
		return ret;

	if (val != 0xAC) {
		LOG_DBG(chg, "re-init\n");
		ret = nu2111a_device_init(chg);
		if (ret < 0)
			return ret;
	}

	nu2111a_enable_adc(chg, true);
	psy_do_property("battery", get,
			POWER_SUPPLY_PROP_ONLINE, prop_val);

	/* Check the current power supply type */
	if (prop_val.intval == SEC_BATTERY_CABLE_FPDO_DC) {
		/* The present power supply type is USBPD charger with only fixed PDO */
		chg->ta_type = TA_TYPE_PD_FPDO;
	} else if (prop_val.intval == SEC_BATTERY_CABLE_PDIC_APDO) {
		/* The present power supply type is USBPD with APDO */
		chg->ta_type = TA_TYPE_PD_APDO;
	} else {
		/* DC cannot support the present power supply type - unknown power supply type */
		chg->ta_type = TA_TYPE_UNKNOWN;
	}
	pr_info("%s: ta_type = %d\n", __func__, chg->ta_type);

	__pm_stay_awake(chg->monitor_ws);

	ret = nu2111a_preset_dcmode(chg);

	LOG_DBG(chg, "End, ret=%d\n", ret);
	return ret;
}

static int nu2111a_check_starting_vbat_level(struct nu2111a_charger *chg)
{
	int ret = 0;
	int vbat;
	union power_supply_propval val;

	nu2111a_set_charging_state(chg, DC_STATE_CHECK_VBAT);

	if (chg->pdata->wd_dis) {
		/* Disable WDT function */
		nu2111a_enable_wdt(chg, WDT_DISABLED);
	} else {
		nu2111a_enable_wdt(chg, WDT_ENABLED);
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
		nu2111a_check_wdt_control(chg);
#else
		nu2111a_set_wdt_time(chg, chg->pdata->wd_tmr);
#endif
	}

	nu2111a_adc_oneshot(chg, true);
	ret = nu2111a_get_adc_channel(chg, ADCCH_VBAT);
	nu2111a_adc_oneshot(chg, false);
	if (ret < 0)
		goto EXIT;
	vbat = chg->adc_vbat;

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	ret = psy_do_property(chg->pdata->sec_dc_name, get,
		POWER_SUPPLY_EXT_PROP_CHARGING_ENABLED_DC, val);
#else
	ret = nu2111a_get_swcharger_property(POWER_SUPPLY_PROP_ONLINE, &val);
#endif

	if (ret < 0) {
		mutex_lock(&chg->lock);
		chg->timer_id = TIMER_VBATMIN_CHECK;
		chg->timer_period = NU2111A_VBATMIN_CHECK_T;
		mutex_unlock(&chg->lock);
		schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
		LOG_DBG(chg, "TIMER_VBATMIN_CHECK again!!\n");
		goto EXIT;
	}

	if (val.intval == 0) {
		mutex_lock(&chg->lock);
		chg->timer_id = TIMER_PRESET_DC;
		chg->timer_period = 0;
		mutex_unlock(&chg->lock);
		schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
	} else {
	/* Switching charger is not disabled */
		if (vbat > NU2111A_DC_VBAT_MIN)
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
			ret = nu2111a_set_swcharger(chg, false);
#else
			ret = nu2111a_set_swcharger(chg, false, 0, 0, 0);
#endif

		mutex_lock(&chg->lock);
		chg->timer_id = TIMER_VBATMIN_CHECK;
		chg->timer_period = NU2111A_VBATMIN_CHECK_T;
		mutex_unlock(&chg->lock);
		schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
	}

EXIT:
	LOG_DBG(chg, "End, ret = %d\n", ret);
	return ret;
}

static int nu2111a_check_active_state(struct nu2111a_charger *chg)
{
	int ret = 0;

	LOG_DBG(chg, "start!\n");

	nu2111a_set_charging_state(chg, DC_STATE_CHECK_ACTIVE);

	ret = nu2111a_check_sts_reg(chg);
	if (ret < 0) {
		if (chg->active_retry_cnt > 10)
			goto Err;
		chg->active_retry_cnt++;
	}

	if (chg->chg_state == DEV_STATE_ACTIVE) {
		LOG_DBG(chg, " Active State\n");
		chg->active_retry_cnt = 0;
		ret = nu2111a_write_reg(chg, NU2111A_REG_PMID2VOUT_OVP_UVP, 0x12);

		if (chg->ta_type == TA_TYPE_PD_FPDO) {
			mutex_lock(&chg->lock);
			chg->timer_id = TIMER_CHECK_FPDOCVMODE;
			chg->timer_period = 0;
			mutex_unlock(&chg->lock);
		} else {
			mutex_lock(&chg->lock);
			chg->timer_id = TIMER_ADJUST_CCMODE;
			chg->timer_period = 0;
			mutex_unlock(&chg->lock);
		}
		schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
		ret = 0;
	} else if (chg->chg_state == DEV_STATE_NOT_ACTIVE) {
		LOG_DBG(chg, "Not Active State\n");
		ret = nu2111a_write_reg(chg, NU2111A_REG_PMID2VOUT_OVP_UVP, 0x33);

		ret = nu2111a_set_charging(chg, false);
		mutex_lock(&chg->lock);
		chg->timer_id = DC_STATE_CHECK_VBAT;
		chg->timer_period = 0;
		mutex_unlock(&chg->lock);
		schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
		ret = 0;
	} else {
		LOG_DBG(chg, "invalid charging state=%d\n", chg->chg_state);
		ret = -EINVAL;
	}
Err:
	if (ret < 0) {
		LOG_DBG(chg, "Retry %d DCERR:0x%x\n", chg->retry_cnt, chg->dcerr_reason);
		if (chg->retry_cnt > 0 && !chg->active_retry_cnt) {
			mutex_lock(&chg->lock);
			chg->timer_id = TIMER_VBATMIN_CHECK;
			chg->timer_period = 100;
			mutex_unlock(&chg->lock);
			ret = 0;
			schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
		}
	}
	LOG_DBG(chg, "End!, ret = %d\n", ret);
	return ret;
}

static int nu2111a_adjust_ccmode(struct nu2111a_charger *chg)
{
	int iin, ccmode;
	int vbat, val;
	int ret = 0;

	LOG_DBG(chg, "start!\n");

	nu2111a_set_charging_state(chg, DC_STATE_ADJUST_CC);

	ret = nu2111a_check_sts_reg(chg);
	if (ret != 0)
		goto error;	// This is not active mode.

	ccmode = chg->reg_state;

	iin = chg->adc_iin + chg->adc_iin_sub;
	vbat = chg->adc_vbat;
	LOG_DBG(chg, "iin_target:[%d], ta-vol:[%d], ta-cur:[%d]", chg->iin_cc, chg->ta_vol, chg->ta_cur);
	LOG_DBG(chg, "IIN: [%d], VBAT: [%d]\n", iin, vbat);
	if (iin > chg->prev_iin) {
		if (iin > chg->adj_max_iin + 20000) {
			chg->adj_max_iin = iin;
			chg->adj_max_vol = chg->ta_vol;
			LOG_DBG(chg, "ADJ MAX VOL : %d MAX IIN : %d\n", chg->ta_vol, iin);
		}
	}
	if (iin < (NU2111A_IIN_CFG_DFT - NU2111A_TA_C_OFFSET)) {
		chg->ta_v_ofs_thermal = (chg->ta_vol - (2 * vbat));
		LOG_DBG(chg, "IIN: [%d], VBAT: [%d], V_OFFSET: [%d]\n", iin, vbat, chg->ta_v_ofs_thermal);
	}

	/* re-calculate OCP */
	if (chg->regmap_sub) {
		if (iin > NU2111A_IIN_CFG_DFT && !chg->iin_unbalance) {
			if (chg->adc_iin > chg->adc_iin_sub) {
				chg->iin_unbalance = (chg->pdata->iin_cfg*100/iin)*(chg->adc_iin - chg->adc_iin_sub)/200;
				nu2111a_set_input_current_main(chg, chg->pdata->iin_cfg/2 + chg->iin_unbalance);
			} else {
				chg->iin_unbalance = (chg->pdata->iin_cfg*100/iin)*(chg->adc_iin_sub - chg->adc_iin_main)/200;
				nu2111a_set_input_current_sub(chg, chg->pdata->iin_cfg/2 + chg->iin_unbalance);
			}
		}
	}

	if (vbat >= chg->pdata->vbat_reg) {
		chg->ta_target_vol = chg->ta_vol;
		LOG_DBG(chg, "Adjust end!!(VBAT >= vbat_reg) ta_target_vol [%d]", chg->ta_target_vol);
		chg->prev_inc = INC_NONE;
		chg->prev_dec = false;
		mutex_lock(&chg->lock);
		chg->timer_id = TIMER_CHECK_CVMODE;
		chg->timer_period = NU2111A_CHECK_ACTIVE_DELAY_T;
		mutex_unlock(&chg->lock);
		chg->prev_iin = 0;
		chg->retry_cnt = 0;
		schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
		goto error;
	}

	if (chg->prev_dec) {
		chg->prev_dec = false;
		//exit adjust mode
		//Calculate TA_V_ofs = TA_CV -(2 x VBAT_ADC) + 0.1V
#ifdef NU2111A_STEP_CHARGING
		//if (chg->ta_v_ofs == 0)
			chg->ta_v_ofs = (chg->ta_vol - (2 * vbat)) + chg->ta_v_offset;
#else
		chg->ta_v_ofs = (chg->ta_vol - (2 * vbat)) + NU2111A_TA_VOL_STEP_PRE_CC;
		chg->ta_v_ofs_max = chg->ta_v_ofs;
#endif
		val = (vbat * 2) + chg->ta_v_ofs;
		val = val/PD_MSG_TA_VOL_STEP;
		chg->ta_target_vol = val*PD_MSG_TA_VOL_STEP;
		if (chg->ta_target_vol > chg->ta_max_vol)
			chg->ta_target_vol = chg->ta_max_vol;

		LOG_DBG(chg, "Ener CC, already increased TA_CV! TA-V-OFS:[%d]\n", chg->ta_v_ofs);
		chg->prev_inc = INC_NONE;
		chg->prev_iin = 0;
		chg->retry_cnt = 0;

		mutex_lock(&chg->lock);
		chg->timer_id = TIMER_ENTER_CCMODE;
		chg->timer_period = 0;
		mutex_unlock(&chg->lock);
		schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
		goto error;
	}

	switch (ccmode) {
	case INACTIVE_LOOP:
		//IIN_ADC > IIN_TG + 25mA
		if (iin  > (chg->iin_cc + 50000)) {
			// Decrease TA_CC (50mA)
			chg->ta_cur = chg->ta_cur - PD_MSG_TA_CUR_STEP;
			LOG_DBG(chg, "(IIN>IIN TARGET+50mA), ta-cur[%d], ta-vol[%d], got ta_v_ofs\n",
			chg->ta_cur, chg->ta_vol);
			chg->prev_dec = true;
			//send pd msg
			mutex_lock(&chg->lock);
			chg->timer_id = TIMER_PDMSG_SEND;
			chg->timer_period = 0;
			mutex_unlock(&chg->lock);
			goto EXIT;
		} else {
			//|(IIN_ADC - IIN_TG)| < 25mA?
			if (iin >= chg->iin_cc) {
				LOG_DBG(chg, "iin >= iin_cc\n");
				goto ENTER_CC;
			} else {
				if (chg->ta_vol >= chg->ta_max_vol) {
					LOG_DBG(chg, "TA-VOL > TA-MAX-VOL\n");
					if (chg->ta_cur == chg->iin_cc - chg->ta_c_offset) {
						if (chg->iin_cc-PD_MSG_TA_CUR_STEP > 0) {
							chg->ta_cur = chg->ta_cur + PD_MSG_TA_CUR_STEP;
							if (chg->ta_cur > chg->ta_max_cur)
								chg->ta_cur = chg->ta_max_cur;
							val = (chg->ta_max_pwr/100)*POWL/((chg->iin_cc-PD_MSG_TA_CUR_STEP)/1000);
							val = (val * 1000) / PD_MSG_TA_VOL_STEP;
							val = val * PD_MSG_TA_VOL_STEP;
							if (chg->ta_max_pwr > NU2111A_TA_25W)
								val = chg->pdo_max_voltage*NU2111A_SEC_DENOM_U_M;
							chg->ta_max_vol = MIN(val, NU2111A_TA_MAX_VOL);
							LOG_DBG(chg, "INC = TA_CUR!! ta_max_vol %d iin_cc %d\n", chg->ta_max_vol, chg->iin_cc);
							chg->prev_inc = INC_NONE;
						}
						//send pd msg
						mutex_lock(&chg->lock);
						chg->timer_id = TIMER_PDMSG_SEND;
						chg->timer_period = 0;
						mutex_unlock(&chg->lock);
						goto EXIT;
					} else {
						LOG_DBG(chg, "Choose adj max vol : %d\n", chg->adj_max_vol);
						chg->low_ta = true;
						chg->ta_vol = chg->adj_max_vol;
						goto ENTER_CC;
					}
				} else {
					if (iin > (chg->prev_iin + 20000)) {
						//Increase TA_CV (40mV)
						LOG_DBG(chg, "iin[%d] > prev[%d]+20000, increase TA_CV\n",
							iin, chg->prev_iin);
#ifdef CONFIG_SEC_FACTORY
						if (iin < (chg->iin_cc - NU2111A_IIN_CFG_STEP)) {
							chg->ta_vol += NU2111A_TA_VOL_STEP_ADJ_CC;
							if (chg->iin_cc > NU2111A_PT_IIN_DFT &&
								iin < (chg->iin_cc - NU2111A_IIN_CFG_STEP*2))
								chg->ta_vol += PD_MSG_TA_VOL_STEP*2;
						} else
							chg->ta_vol += NU2111A_TA_VOL_STEP_ENTER_CC;
#else
						chg->ta_vol += NU2111A_TA_VOL_STEP_ADJ_CC;
#endif
						chg->prev_iin = iin;
						chg->prev_inc = INC_NONE;
						//send pd msg
						mutex_lock(&chg->lock);
						chg->timer_id = TIMER_PDMSG_SEND;
						chg->timer_period = 0;
						mutex_unlock(&chg->lock);
						goto EXIT;
					} else {
						if (chg->prev_inc < INC_TA_CUR &&
							(chg->ta_cur > chg->iin_cc + NU2111A_IIN_CFG_STEP ||
							iin < chg->ta_cur - NU2111A_TA_C_OFFSET)) {
							//Increase TA_CV (40mV)
							chg->ta_vol += NU2111A_TA_VOL_STEP_ADJ_CC;
							if (chg->ta_vol > chg->ta_max_vol)
								chg->ta_vol = chg->ta_max_vol;
							LOG_DBG(chg, "prev_inc[%d] < INC_TA_CUR  TA-VOL[%dmV]\n",
								chg->prev_inc, chg->ta_vol);
							chg->prev_inc++;

							//send pd msg
							mutex_lock(&chg->lock);
							chg->timer_id = TIMER_PDMSG_SEND;
							chg->timer_period = 0;
							mutex_unlock(&chg->lock);
							goto EXIT;
						} else {
							if (chg->ta_cur >= chg->ta_max_cur) {
								LOG_DBG(chg, "TA-CUR == TA-MAX-CUR\n");
								if ((iin + 20000) < chg->iin_cc) {
									chg->ta_vol += NU2111A_TA_VOL_STEP_ADJ_CC;
									if (chg->ta_vol > chg->ta_max_vol)
										chg->ta_vol = chg->ta_max_vol;
									LOG_DBG(chg, "IIN is not high, inc = TA-VOL[%dmV]\n",
										chg->ta_vol);
									chg->prev_inc = INC_NONE;
									//send pd msg
									mutex_lock(&chg->lock);
									chg->timer_id = TIMER_PDMSG_SEND;
									chg->timer_period = 0;
									mutex_unlock(&chg->lock);
									goto EXIT;
								} else {
									LOG_DBG(chg, "MID TA [%dmV]\n",
										chg->ta_vol);
									//chg->ta_vol += NU2111A_TA_VOL_STEP_ADJ_CC;
									LOG_DBG(chg, "Choose adj max vol : %d\n", chg->adj_max_vol);
									chg->low_ta = true;
									chg->ta_vol = chg->adj_max_vol;
									goto ENTER_CC;
								}
							} else {
								chg->ta_cur = chg->ta_cur + PD_MSG_TA_CUR_STEP;
								if (chg->ta_cur > chg->ta_max_cur)
									chg->ta_cur = chg->ta_max_cur;
								if (chg->dcerr_retry_cnt > 1  &&
									chg->dcerr_reason == VIN_UVLO) {
									val = chg->dcerr_ta_max_vol/1000 - 2*(NU2111A_TA_VOL_STEP_ADJ_CC/1000);
									LOG_DBG(chg, "dcerr_ta_max_vol=%d, val=%d", chg->dcerr_ta_max_vol/1000, val);
									val = MAX(val, NU2111A_TA_VOL_MIN_DCERR);
								} else
									val = (chg->ta_max_pwr/100)*POWL/(chg->ta_cur/1000);

								val = (val * 1000) / PD_MSG_TA_VOL_STEP;
								val = val * PD_MSG_TA_VOL_STEP;
								if (chg->ta_max_pwr > NU2111A_TA_25W)
									val = chg->pdo_max_voltage*NU2111A_SEC_DENOM_U_M;
								chg->ta_max_vol = MIN(val, NU2111A_TA_MAX_VOL);

								LOG_DBG(chg, "INC = INC_NONE!! ta_max_vol %d\n", chg->ta_max_vol);
								chg->prev_inc = INC_NONE;
								//send pd msg
								mutex_lock(&chg->lock);
								chg->timer_id = TIMER_PDMSG_SEND;
								chg->timer_period = 0;
								mutex_unlock(&chg->lock);
								goto EXIT;
							}
						}
					}
				}
			}
		}

ENTER_CC:
		//exit adjust mode
		//Calculate TA_V_ofs = TA_CV -(2 x VBAT_ADC) + 0.1V
#ifdef NU2111A_STEP_CHARGING
		if (chg->ta_v_ofs == 0) {
			chg->ta_v_ofs = (chg->ta_vol - (2 * vbat)) + chg->ta_v_offset - ajust_Volt;
			LOG_DBG(chg, "--chg->ta_vol[%d]-----chg->ta_v_ofs[%d], vbat:[%d], NU2111A_TA_V_OFFSET:%d",
				chg->ta_vol, chg->ta_v_ofs, vbat, (chg->ta_v_offset - ajust_Volt));
		}
#else
		chg->ta_v_ofs = (chg->ta_vol - (2 * vbat)) + NU2111A_TA_VOL_STEP_PRE_CC;
		chg->ta_v_ofs_max = chg->ta_v_ofs;
#endif
		val = (vbat * 2) + chg->ta_v_ofs;
		LOG_DBG(chg, "---1-----val:[%d]", val);
		val = val / PD_MSG_TA_VOL_STEP;
		chg->ta_target_vol = val*PD_MSG_TA_VOL_STEP;

		LOG_DBG(chg, "--------chg->ta_target_vol:[%d],chg->ta_max_vol:[%d]",
			chg->ta_target_vol, chg->ta_max_vol);
		if (chg->ta_target_vol > chg->ta_max_vol) {
			chg->ta_target_vol = chg->ta_max_vol;
			chg->ta_v_ofs = chg->ta_max_vol - (2 * vbat);
			LOG_DBG(chg, "MAX VOL! ta_v_ofs:[%d],chg->ta_target_vol:[%d],chg->ta_max_vol:[%d]",
				chg->ta_v_ofs, chg->ta_target_vol, chg->ta_max_vol);
		}

		LOG_DBG(chg, "Adjust ended!!,ta_vol [%d], TA-V-OFS:[%d]", chg->ta_vol, chg->ta_v_ofs);
		chg->prev_inc = INC_NONE;
		chg->prev_dec = false;
		chg->retry_cnt = 0;

		mutex_lock(&chg->lock);
		chg->timer_id = TIMER_ENTER_CCMODE;
		chg->timer_period = 0;
		mutex_unlock(&chg->lock);
EXIT:
		chg->prev_iin = iin;
		schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
		break;

	case VBAT_REG_LOOP:
		chg->ta_target_vol = chg->ta_vol;
		LOG_DBG(chg, "Ad end!!(VBAT-reg) ta_target_vol [%d]", chg->ta_target_vol);
		chg->prev_inc = INC_NONE;
		chg->prev_dec = false;
		chg->retry_cnt = 0;

		mutex_lock(&chg->lock);
		chg->timer_id = TIMER_CHECK_CVMODE;
		chg->timer_period = NU2111A_CHECK_ACTIVE_DELAY_T;
		mutex_unlock(&chg->lock);
		chg->prev_iin = 0;
		schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
		break;

	case IIN_REG_LOOP:
	case IBAT_REG_LOOP:
		chg->prev_dec = true;
		chg->ta_cur = chg->ta_cur - PD_MSG_TA_CUR_STEP;
		chg->prev_iin = iin;

		LOG_DBG(chg, "IIN-REG: Ad ended!! ta-cur[%d], ta-vol[%d", chg->ta_cur, chg->ta_vol);
		//send pd msg
		mutex_lock(&chg->lock);
		chg->timer_id = TIMER_PDMSG_SEND;
		chg->timer_period = 0;
		mutex_unlock(&chg->lock);
		schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
		break;

	case T_DIE_REG_LOOP:
		break;
	default:
		LOG_DBG(chg, "No Cases for Adjust![%d]\n", ccmode);
		break;
	}
error:
	if (ret < 0) {
		LOG_DBG(chg, "Retry %d DCERR:0x%x\n", chg->retry_cnt, chg->dcerr_reason);
		if (chg->retry_cnt > 0) {
			mutex_lock(&chg->lock);
			chg->timer_id = TIMER_VBATMIN_CHECK;
			chg->timer_period = 200;
			mutex_unlock(&chg->lock);
			ret = 0;
			schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
		}
		if (chg->dcerr_retry_cnt > 0 &&
			chg->dcerr_reason == VIN_UVLO) {
			LOG_DBG(chg, "dcerr TA Vol=%d\n", chg->ta_vol);
			chg->dcerr_ta_max_vol = chg->ta_vol;
		}
	}
	LOG_DBG(chg, "End, ret=%d\n", ret);
	return ret;
}


static int nu2111a_enter_ccmode(struct nu2111a_charger *chg)
{
	int ret = 0;
	int iin;

	if (chg->charging_state != DC_STATE_START_CC) {
		nu2111a_set_charging_state(chg, DC_STATE_START_CC);
		chg->ta_vol = chg->ta_vol + NU2111A_TA_VOL_STEP_ENTER_CC;
		if (chg->ta_vol >= chg->ta_max_vol) {
			chg->ta_vol = chg->ta_max_vol;
			chg->ta_v_ofs = chg->ta_vol - (2 * chg->adc_vbat);
			chg->ta_v_ofs_normal = chg->ta_v_ofs;
			nu2111a_set_charging_state(chg, DC_STATE_CC_MODE);
			LOG_DBG(chg, "TA-V_OFS:[%d],TA-V:[%d] MAX_VOL\n", chg->ta_v_ofs, chg->ta_vol);
		} else if (chg->low_ta) {
			chg->ta_vol = chg->ta_vol - NU2111A_TA_VOL_STEP_ENTER_CC;
			chg->ta_v_ofs = chg->ta_vol - (2 * chg->adc_vbat);
			chg->ta_v_ofs_normal = chg->ta_v_ofs;
			LOG_DBG(chg, "TA-V_OFS:[%d],TA-V:[%d] LOW_TA\n", chg->ta_v_ofs, chg->ta_vol);
			nu2111a_set_charging_state(chg, DC_STATE_CC_MODE);
		}
	} else {
		LOG_DBG(chg, "TA-Vol:[%d],TA-V_OFFSET[%d]\n", chg->ta_vol, chg->ta_v_ofs);
		nu2111a_adc_oneshot(chg, true);
		nu2111a_get_adc_channel(chg, ADCCH_IIN);
		nu2111a_get_adc_channel(chg, ADCCH_VBAT);
		nu2111a_adc_oneshot(chg, false);

		iin = chg->adc_iin + chg->adc_iin_sub;

		if (iin > chg->pdata->iin_cfg ||
			chg->ta_vol >= chg->ta_target_vol ||
			iin < chg->adj_max_iin) {
			chg->ta_v_ofs = chg->ta_vol - (2 * chg->adc_vbat);
			chg->ta_v_ofs_normal = chg->ta_v_ofs;
			nu2111a_set_charging_state(chg, DC_STATE_CC_MODE);
			LOG_DBG(chg, "IIN:[%d],TA-V_OFS:[%d],TA-V:[%d]\n", chg->adc_iin, chg->ta_v_ofs, chg->ta_vol);

		} else
			chg->ta_vol += NU2111A_TA_VOL_STEP_ENTER_CC;
	}

	//send pd msg
	mutex_lock(&chg->lock);
	chg->timer_id = TIMER_PDMSG_SEND;
	chg->timer_period = 0;
	mutex_unlock(&chg->lock);
	schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
	return ret;
}


static int nu2111a_ccmode_regulation_process(struct nu2111a_charger *chg)
{
	int ret = 0;
	int iin, vbat, ccmode, dev_state, delta, ibat_fg;
	int ta_v_ofs, new_ta_vol, new_ta_cur;
	unsigned int val;

	//LOG_DBG(chg, "STart!!\n");

	nu2111a_set_charging_state(chg, DC_STATE_CC_MODE);

	ret = nu2111a_check_sts_reg(chg);
	if (ret < 0) {
		if (chg->fault_sts_cnt == 1) {
			/* this code is only one-time-check */
			nu2111a_bad_ta_detection_trigger(chg);
			ret = 0;
		}
		goto Err;
	}
	/* Check new vbat-reg/new iin request first */
	if (chg->req_pt_mode) {
		mutex_lock(&chg->lock);
		chg->req_pt_mode = false;
		mutex_unlock(&chg->lock);
		ret = nu2111a_set_pass_through_mode(chg);
	} else if (chg->req_new_vbat_reg) {
		mutex_lock(&chg->lock);
		chg->req_new_vbat_reg = false;
		mutex_unlock(&chg->lock);
		ret = nu2111a_set_new_vbat_reg(chg);
	} else if (chg->req_new_iin) {
		mutex_lock(&chg->lock);
		chg->req_new_iin = false;
		mutex_unlock(&chg->lock);
		ret = nu2111a_set_new_iin(chg);
	} else {
		ccmode = chg->reg_state;
		dev_state = chg->chg_state;

		iin = chg->adc_iin + chg->adc_iin_sub;
		vbat = chg->adc_vbat;
		ibat_fg = chg->adc_ibat;
		if (chg->pdata->step1_vfloat*NU2111A_SEC_DENOM_U_M == chg->pdata->vbat_reg)
			chg->iin_c_offset = NU2111A_IIN_CFG_OFFSET_CC;
		else
			chg->iin_c_offset = 0;
		LOG_DBG(chg, "IIN_TARGET[%d], IIN[%d], VBAT[%d], IBAT_FG[%d], [%d]\n",
				chg->iin_cc, iin, vbat, ibat_fg, chg->iin_c_offset);

		if (chg->sub_topoff && chg->chg_state_sub) {
			LOG_DBG(chg, "sub topoff < 1A");
			if (chg->regmap_sub)
				nu2111a_set_charging_sub(chg, false);
		}

		switch (ccmode) {
		case INACTIVE_LOOP:

			new_ta_vol = chg->ta_vol;
			new_ta_cur = chg->ta_cur;

			/* Need to clear the flag for bad TA detection */
			if (chg->ab_try_cnt != 0) {
				LOG_DBG(chg, "clear ab_try_cnt for normal mode!ab_cnt[%d]\n", chg->ab_try_cnt);
				chg->ab_try_cnt = 0;
			}

			if ((chg->ab_ta_connected) && (iin > (chg->iin_cc+50000))) {
				/* Decrease TA VOL, because TA CUR can't be controlled */
				new_ta_vol = chg->ta_vol - PD_MSG_TA_VOL_STEP;
				LOG_DBG(chg, "[AB_TA]:IIN[%d], TA_VOL[%d], 20mV --\n", iin, chg->ta_vol);
			} else {
				if (chg->ab_ta_connected) {
					delta = chg->iin_cc - iin;
					if ((delta >= 200000) && (chg->iin_cc > chg->ta_cur))
						new_ta_cur = chg->iin_cc;

					if ((delta > 50000) && (delta < 100000)) {
						new_ta_vol = chg->ta_vol + PD_MSG_TA_VOL_STEP;
						LOG_DBG(chg, "[AB_TA]:IIN[%d], new_ta_vol[%d], delta[%d], 20mV ++\n",
							iin, new_ta_vol, delta);
					} else if (delta > 100000) {
						new_ta_vol = chg->ta_vol + (PD_MSG_TA_VOL_STEP*2);
						LOG_DBG(chg, "[AB_TA]:IIN[%d], new_ta_vol[%d], delta[%d], 40mV ++\n",
							iin, new_ta_vol, delta);
					} else
						new_ta_vol = chg->ta_vol;
				} else {
					if (abs(chg->ta_v_ofs - chg->ta_v_ofs_thermal) < NU2111A_IIN_CFG_RANGE_CC) {
						ta_v_ofs = chg->ta_v_ofs;
						new_ta_vol = (2*vbat) + ta_v_ofs;
						if (iin > chg->pdata->iin_cfg) {
							new_ta_cur = chg->ta_cur - PD_MSG_TA_CUR_STEP;
							if (new_ta_cur < NU2111A_TA_MIN_CUR) {
								new_ta_cur = NU2111A_TA_MIN_CUR;
								chg->ta_v_ofs_thermal -= PD_MSG_TA_VOL_STEP;
								chg->ta_v_ofs = chg->ta_v_ofs_thermal;
								LOG_DBG(chg, "[TM] --20mV, TAVOL[%d], TACUR[%d], IIN_CFG[%d]\n",
									new_ta_vol, chg->ta_cur, chg->pdata->iin_cfg);
							} else
								LOG_DBG(chg, "[TM] --50mA\n");
							ta_v_ofs = chg->ta_v_ofs;
							new_ta_vol = (2*vbat) + ta_v_ofs;

						} else {
							if ((iin <= (chg->iin_cc -
								NU2111A_IIN_CFG_RANGE_CC * 2)) &&
								(chg->ta_cur <= (chg->iin_cc - NU2111A_IIN_CFG_RANGE_CC * 2
								))) {
								new_ta_cur = chg->ta_cur + PD_MSG_TA_CUR_STEP;
								LOG_DBG(chg, "[TM] ++50mA\n");
							} else if (iin <= (chg->iin_cc -
									NU2111A_IIN_CFG_RANGE_CC * 2)) {
								if (chg->ta_v_ofs_thermal < (chg->ta_v_ofs_normal - NU2111A_TA_V_OFFSET)) {
										chg->ta_v_ofs_thermal += PD_MSG_TA_VOL_STEP;
										chg->ta_v_ofs = chg->ta_v_ofs_thermal;
										ta_v_ofs = chg->ta_v_ofs;
										new_ta_vol = (2*vbat) + ta_v_ofs;
										LOG_DBG(chg, "[TM] ++20mV\n");
								}
							}
						}
					} else if (iin > chg->pdata->iin_cfg + chg->iin_c_offset) {
						ta_v_ofs = chg->ta_v_ofs;
						new_ta_vol = (2*vbat) + ta_v_ofs;
						if (chg->iin_high_count > 0) {
							chg->iin_high_count = 0;
							if (ibat_fg > (chg->pdata->iin_cfg*2)) {
								new_ta_cur = chg->ta_cur - PD_MSG_TA_CUR_STEP;
								ta_v_ofs = chg->ta_v_ofs;
								new_ta_vol = (2*vbat) + ta_v_ofs;
								LOG_DBG(chg, "--50mA, TAVOL[%d], TACUR[%d], IIN_CFG[%d]\n",
									new_ta_vol, chg->ta_cur, chg->pdata->iin_cfg);
							}
						} else {
							chg->iin_high_count++;
							LOG_DBG(chg, "IIN high, cnt[%d] TAVOL[%d], TACUR[%d]\n",
								chg->iin_high_count, new_ta_vol, chg->ta_cur);
							mutex_lock(&chg->lock);
							chg->timer_id = TIMER_CHECK_CCMODE;
							chg->timer_period = NU2111A_CCMODE_CHECK_T;
							mutex_unlock(&chg->lock);
							schedule_delayed_work(&chg->timer_work,
								msecs_to_jiffies(chg->timer_period));
							return ret;
						}
					} else {
						ta_v_ofs = chg->ta_v_ofs;
						new_ta_vol = (2*vbat) + ta_v_ofs;
						/* Workaround code for low CC-Current issue */
						/* to keep high current, increase TA-CUR */
						if ((iin <= (chg->iin_cc -
							(NU2111A_IIN_CFG_RANGE_CC - chg->iin_c_offset))) &&
							(chg->ta_cur < chg->iin_cc)) {
							new_ta_cur = chg->ta_cur + PD_MSG_TA_CUR_STEP;
							LOG_DBG(chg, "++50mA\n");
						} else if (iin <= (chg->iin_cc -
							(NU2111A_IIN_CFG_RANGE_CC - chg->iin_c_offset))) {
							if (new_ta_vol < (2*vbat) + chg->ta_v_ofs_max &&
								chg->ta_cur > chg->pdata->iin_cfg - NU2111A_IIN_CFG_OFFSET_CC) {
								//if (chg->ta_max_pwr < NU2111A_TA_25W) {
									chg->ta_v_ofs += PD_MSG_TA_VOL_STEP;
									LOG_DBG(chg, "++20mV\n");
								//}
							}
						}
						LOG_DBG(chg, "TA-VOL:[%d],TA-CUR:[%d], ta_v_offs:[%d]\n",
							chg->ta_vol, chg->ta_cur, ta_v_ofs);
						chg->iin_high_count = 0;
					}
				}
			}
			/* if Power is higher than Max-Power, CC current can go lower than the target */
			val = (new_ta_vol/1000) * (new_ta_cur/1000);

			if (chg->dcerr_retry_cnt > 1  &&
				chg->dcerr_reason == VIN_UVLO) {
				LOG_DBG(chg, "dcerr retrr ta max vol calculate again.\n");
				LOG_DBG(chg, "new_ta_vol %d %d\n", new_ta_vol, chg->ta_max_vol);
				if (new_ta_vol > chg->ta_max_vol)
					new_ta_vol = chg->ta_max_vol;
			} else {
				if (val > (chg->ta_max_pwr/100)*POWL) {
					LOG_DBG(chg, "max power limit : new ta_vol[%d], new_ta_cur[%d], val[%d]\n"
						, new_ta_vol, new_ta_cur, val);
					chg->iin_cc -= PD_MSG_TA_CUR_STEP;
					/* re-calculate */
					val = (chg->ta_max_pwr/100)*POWL/(chg->iin_cc/1000);
					val = val*1000/PD_MSG_TA_VOL_STEP;
					val = val*PD_MSG_TA_VOL_STEP;
					if (chg->ta_max_pwr > NU2111A_TA_25W)
						val = chg->pdo_max_voltage*NU2111A_SEC_DENOM_U_M;
					chg->ta_max_vol = MIN(val, chg->pdo_max_voltage*NU2111A_SEC_DENOM_U_M);
					if (new_ta_cur > chg->iin_cc)
						new_ta_cur = chg->iin_cc;
				}
			}

			chg->ta_max_vol = MIN(chg->ta_max_vol, chg->pdo_max_voltage*NU2111A_SEC_DENOM_U_M);
			new_ta_vol = MIN(new_ta_vol, chg->ta_max_vol);

			if (chg->ab_ta_connected) {
				/* send pd msg */
				if (new_ta_vol != chg->ta_vol) {
					chg->ta_vol = new_ta_vol;
					mutex_lock(&chg->lock);
					chg->timer_id = TIMER_PDMSG_SEND;
					chg->timer_period = 0;
					mutex_unlock(&chg->lock);
				} else {
					mutex_lock(&chg->lock);
					chg->timer_id = TIMER_CHECK_CCMODE;
					chg->timer_period = NU2111A_CCMODE_CHECK_T;
					mutex_unlock(&chg->lock);
				}
			} else {
				/* send pd msg */
				val = new_ta_vol/PD_MSG_TA_VOL_STEP;	/* PPS voltage resolution is 20mV */
				new_ta_vol = val*PD_MSG_TA_VOL_STEP;
				if ((new_ta_vol > chg->ta_vol) ||
					(new_ta_cur != chg->ta_cur)) {
					LOG_DBG(chg, "New-Vol[%d], New-Cur[%d], TA-Vol[%d], TA-Cur[%d]\n",
						new_ta_vol/1000, new_ta_cur/1000,
						chg->ta_vol/1000, chg->ta_cur/1000);
					chg->ta_vol = new_ta_vol;
					chg->ta_cur = new_ta_cur;
					mutex_lock(&chg->lock);
					chg->timer_id = TIMER_PDMSG_SEND;
					chg->timer_period = 0;
					mutex_unlock(&chg->lock);
				} else {
					mutex_lock(&chg->lock);
					chg->timer_id = TIMER_CHECK_CCMODE;
					chg->timer_period = NU2111A_CCMODE_CHECK_T;
					mutex_unlock(&chg->lock);
				}
			}
			schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
			break;

		case VBAT_REG_LOOP:
			LOG_DBG(chg, "VBAT-REG, move to CV\n");
			mutex_lock(&chg->lock);
			chg->timer_id = TIMER_CHECK_CVMODE;
			chg->timer_period = 0;
			mutex_unlock(&chg->lock);
			schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
			break;

		case IIN_REG_LOOP:
		case IBAT_REG_LOOP:
			/* Request from samsung for abnormal TA connection */
			if (!chg->ab_ta_connected) {
				if (chg->ab_try_cnt > 3) {
					/* abnormal TA connected! */
					chg->ab_ta_connected = true;
					chg->ab_try_cnt = 0;
					/* Decrease TA VOL, because TA CUR can't be controlled */
					chg->ta_vol = chg->ta_vol - PD_MSG_TA_VOL_STEP;
					chg->ta_cur = chg->ta_cur - PD_MSG_TA_CUR_STEP;
					//chg->ta_cur = chg->iin_cc;
					LOG_DBG(chg, "[AB_TA][IIN-LOOP]:IIN[%d], TA_VOL[%d],TA_CUR[%d] 20mV --\n",
						iin, chg->ta_vol, chg->ta_cur);
				} else {
					if (abs(iin - chg->ab_prev_cur) < 20000)
						chg->ab_try_cnt++;
					else
						chg->ab_try_cnt = 0;

					chg->ta_cur = chg->ta_cur - PD_MSG_TA_CUR_STEP;
					LOG_DBG(chg, "[IIN-LOOP]:iin=%d, next_ta_cur=%d, abCNT[%d]\n",
							iin, chg->ta_cur, chg->ab_try_cnt);
				}
			} else {
				/* Decrease TA CUR, because IIN is still higher than target */
				if (((chg->ab_prev_cur - iin) < 20000) && (iin > chg->iin_cc))
					chg->ta_cur = chg->ta_cur - PD_MSG_TA_CUR_STEP;

				/* Decrease TA VOL, because TA CUR can't be controlled */
				chg->ta_vol = chg->ta_vol - PD_MSG_TA_VOL_STEP;
				LOG_DBG(chg, "[AB_TA][IIN-LOOP]:IIN[%d], TA_VOL[%d], 20mV --\n", iin, chg->ta_vol);
			}

			/* store iin to compare the change */
			chg->ab_prev_cur = iin;

			//send pd msg
			mutex_lock(&chg->lock);
			chg->timer_id = TIMER_PDMSG_SEND;
			chg->timer_period = 0;
			mutex_unlock(&chg->lock);
			schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
			break;

		case T_DIE_REG_LOOP:
			/* it depends on customer's scenario */
			break;
		default:
			LOG_DBG(chg, "Wait for CC Time\n");
			mutex_lock(&chg->lock);
			chg->timer_id = TIMER_CHECK_CCMODE;
			chg->timer_period = NU2111A_CCMODE_CHECK_T;
			mutex_unlock(&chg->lock);
			schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
			break;
		}
	}
Err:
	//pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;
}

static int nu2111a_cvmode_process(struct nu2111a_charger *chg)
{
	int ret = 0;
	int iin, vbat, cvmode, dev_state;

	//LOG_DBG(chg, "Start!!\n");

	nu2111a_set_charging_state(chg, DC_STATE_CV_MODE);

	ret = nu2111a_check_sts_reg(chg);
	if (ret < 0)
		goto Err;
	cvmode = chg->reg_state;
	dev_state = chg->chg_state;

	/* Check new vbat-reg/new iin request first */
	if (chg->req_pt_mode) {
		mutex_lock(&chg->lock);
		chg->req_pt_mode = false;
		mutex_unlock(&chg->lock);
		ret = nu2111a_set_pass_through_mode(chg);
	} else if (chg->req_new_vbat_reg) {
		mutex_lock(&chg->lock);
		chg->req_new_vbat_reg = false;
		mutex_unlock(&chg->lock);
		ret = nu2111a_set_new_vbat_reg(chg);
	} else if (chg->req_new_iin) {
		mutex_lock(&chg->lock);
		chg->req_new_iin = false;
		mutex_unlock(&chg->lock);
		ret = nu2111a_set_new_iin(chg);
	} else {
		iin = chg->adc_iin + chg->adc_iin_sub;
		vbat = chg->adc_vbat;

		if (iin < chg->pdata->iin_topoff) {
			LOG_DBG(chg, "Going to top off done!");
			cvmode = INACTIVE_LOOP;
		}

		LOG_DBG(chg, "IIN_TARGET[%d],TA_TARGET_VOL[%d], IIN[%d], VBAT[%d], TA-VOL[%d], TA-CUR[%d]\n",
				chg->iin_cc, chg->ta_target_vol, iin, vbat, chg->ta_vol, chg->ta_cur);

		if (chg->sub_topoff && chg->chg_state_sub) {
			LOG_DBG(chg, "sub topoff < 1A");
			if (chg->regmap_sub)
				nu2111a_set_charging_sub(chg, false);
		}

		if (vbat < chg->pdata->vbat_reg - chg->ta_v_offset) {
			LOG_DBG(chg, "VBAT[%d],VBAT_REG[%d] Go back to Adj CC", vbat, chg->pdata->vbat_reg);
			mutex_lock(&chg->lock);
			chg->timer_id = DC_STATE_ADJUST_CC;
			chg->timer_period = 0;
			mutex_unlock(&chg->lock);
			schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
			return ret;
		}

		switch (cvmode) {
		case INACTIVE_LOOP:
			if (iin < chg->pdata->iin_topoff) {
				LOG_DBG(chg, "DC charging done!");
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
				nu2111a_set_done(chg, true);
#endif

				if (chg->charging_state != DC_STATE_NOT_CHARGING) {
#ifdef NU2111A_STEP_CHARGING
					if (chg->pdata->vbat_reg == NU2111A_STEP3_VBAT_REG)
						nu2111a_stop_charging(chg);
					else
						schedule_delayed_work(&chg->step_work, 100);
#else
					nu2111a_stop_charging(chg);
#endif
				} else {
					LOG_DBG(chg, "Charging is already disabled!\n");
				}
			} else {
				LOG_DBG(chg, "No ACTIVE, Just Wait for 4s\n");
				mutex_lock(&chg->lock);
				chg->timer_id = TIMER_CHECK_CVMODE;
				chg->timer_period = NU2111A_CVMODE_CHECK_T;
				/*
				if (vbat > chg->pdata->vbat_reg_max - NU2111A_VBAT_STEP) {
					LOG_DBG(chg, "CV Wait 2sec\n");
					chg->timer_period = NU2111A_CVMODE_CHECK_T_3;
				}
				*/
				mutex_unlock(&chg->lock);
				schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
			}
			break;

		case VBAT_REG_LOOP:
			chg->ta_vol = chg->ta_vol - NU2111A_TA_VOL_STEP_PRE_CV;
			LOG_DBG(chg, "VBAT-REG, Decrease 20mV, TA-VOL:[%d], TA-CUR:[%d]\n", chg->ta_vol, chg->ta_cur);
			//send pd msg
			mutex_lock(&chg->lock);
			chg->timer_id = TIMER_PDMSG_SEND;
			chg->timer_period = 0;//0;//400ms
			mutex_unlock(&chg->lock);
			schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
			break;

		case IIN_REG_LOOP:
		case IBAT_REG_LOOP:
			chg->ta_cur = chg->ta_cur - PD_MSG_TA_CUR_STEP;
			LOG_DBG(chg, "IIN-REG/IBAT-REG, Decrease 50mA, TA-VOL[%d], TA-CUR[%d]\n",
				chg->ta_vol, chg->ta_cur);
			//send pd msg
			mutex_lock(&chg->lock);
			chg->timer_id = TIMER_PDMSG_SEND;
			chg->timer_period = 0;
			mutex_unlock(&chg->lock);
			schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
			break;

		case T_DIE_REG_LOOP:
		/* it depends on customer's scenario */
			break;
		default:
			LOG_DBG(chg, "Wait for CV Time 4sec\n");
			mutex_lock(&chg->lock);
			chg->timer_id = TIMER_CHECK_CVMODE;
			chg->timer_period = NU2111A_CVMODE_CHECK_T;
			mutex_unlock(&chg->lock);
			schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
			break;
		}
	}

Err:
	//pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;
}

static int nu2111a_fpdo_cvmode_process(struct nu2111a_charger *chg)
{
	int ret = 0;
	int iin, vbat, cvmode, dev_state;

	//LOG_DBG(chg, "Start!!\n");

	nu2111a_set_charging_state(chg, DC_STATE_FPDO_CV_MODE);

	ret = nu2111a_check_sts_reg(chg);
	if (ret < 0)
		goto Err;
	cvmode = chg->reg_state;
	dev_state = chg->chg_state;

	/* Check new vbat-reg/new iin request first */
	if (chg->req_new_vbat_reg) {
		/* Set VFLOAT to new vfloat */
		chg->pdata->vbat_reg = chg->new_vbat_reg;
		ret = nu2111a_set_vbat_reg(chg, chg->pdata->vbat_reg);
		if (ret < 0)
			goto Err;

		/* Clear req_new_vbat_reg */
		mutex_lock(&chg->lock);
		chg->req_new_vbat_reg = false;
		mutex_unlock(&chg->lock);

		/* Set timer - 1s */
		mutex_lock(&chg->lock);
		chg->timer_id = TIMER_CHECK_FPDOCVMODE;
		chg->timer_period = NU2111A_CVMODE_CHECK_T;
		mutex_unlock(&chg->lock);
		schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));

	} else if (chg->req_new_iin) {
		/* Set IIN_CC to new iin */
		chg->iin_cc = chg->new_iin;
		/* Update iin_cfg */
		chg->pdata->iin_cfg = chg->iin_cc;
		/* Set IIN_CFG to new IIN */
		ret = nu2111a_set_input_current(chg, chg->iin_cc);
		if (ret < 0)
			goto Err;

		/* Clear req_new_iin */
		mutex_lock(&chg->lock);
		chg->req_new_iin = false;
		mutex_unlock(&chg->lock);

		/* Set timer - 4s */
		mutex_lock(&chg->lock);
		chg->timer_id = TIMER_CHECK_FPDOCVMODE;
		chg->timer_period = NU2111A_CVMODE_CHECK_T;
		mutex_unlock(&chg->lock);
		schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));

	} else {
		iin = chg->adc_iin + chg->adc_iin_sub;
		vbat = chg->adc_vbat;

		LOG_DBG(chg, "IIN_TARGET[%d],TA_TARGET_VOL[%d], IIN[%d], VBAT[%d], TA-VOL[%d], TA-CUR[%d]\n",
				chg->iin_cc, chg->ta_target_vol, iin, vbat, chg->ta_vol, chg->ta_cur);

		switch (cvmode) {
		case INACTIVE_LOOP:
			if (vbat >= chg->fpdo_dc_vnow_topoff || iin < chg->fpdo_dc_iin_topoff) {
				LOG_DBG(chg, "FPDO DC charging done!");
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
				nu2111a_set_done(chg, true);
#endif

				if (chg->charging_state != DC_STATE_NOT_CHARGING) {
#ifdef NU2111A_STEP_CHARGING
					if (chg->pdata->vbat_reg == NU2111A_STEP3_VBAT_REG)
						nu2111a_stop_charging(chg);
					else
						schedule_delayed_work(&chg->step_work, 100);
#else
					nu2111a_stop_charging(chg);
#endif
				} else {
					LOG_DBG(chg, "Charging is already disabled!\n");
				}
			} else {
				LOG_DBG(chg, "FPDO No ACTIVE, Just Wait for 4s\n");
				mutex_lock(&chg->lock);
				chg->timer_id = TIMER_CHECK_FPDOCVMODE;
				chg->timer_period = NU2111A_CVMODE_CHECK_T;
				mutex_unlock(&chg->lock);
				schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
			}
			break;

		case VBAT_REG_LOOP:
			/* IIN_LOOP happens */
			pr_info("%s: FPDO CV VBAT REG LOOP\n", __func__);
			/* Need to stop DC by battery driver */

			/* Need to implement notification function */
			/* A customer should insert code */
			/* To do here */

			/* Check the charging status after notification function */
			if (chg->charging_state != DC_STATE_NOT_CHARGING) {
				/* Notification function does not stop timer work yet */
				/* Keep the current state */
				/* Set timer - 1s */
				mutex_lock(&chg->lock);
				chg->timer_id = TIMER_CHECK_FPDOCVMODE;
				chg->timer_period = NU2111A_CVMODE_CHECK_T;
				mutex_unlock(&chg->lock);
				schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
			} else {
				/* Already called stop charging by notification function */
				pr_info("%s: Already stop DC\n", __func__);
			}
			break;
		case IIN_REG_LOOP:
		case IBAT_REG_LOOP:
			/* IIN_LOOP happens */
			pr_info("%s: FPDO CV IIN_LOOP\n", __func__);
			/* Need to stop DC by battery driver */

			/* Need to implement notification function */
			/* A customer should insert code */
			/* To do here */

			/* Check the charging status after notification function */
			if (chg->charging_state != DC_STATE_NOT_CHARGING) {
				/* Notification function does not stop timer work yet */
				/* Keep the current state */
				/* Set timer - 1s */
				mutex_lock(&chg->lock);
				chg->timer_id = TIMER_CHECK_FPDOCVMODE;
				chg->timer_period = NU2111A_CVMODE_CHECK_T;
				mutex_unlock(&chg->lock);
				schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
			} else {
				/* Already called stop charging by notification function */
				pr_info("%s: Already stop DC\n", __func__);
			}
			break;
		case T_DIE_REG_LOOP:
		/* it depends on customer's scenario */
			break;
		default:
			LOG_DBG(chg, "FPDO Wait for CV Time\n");
			mutex_lock(&chg->lock);
			chg->timer_id = TIMER_CHECK_FPDOCVMODE;
			chg->timer_period = NU2111A_CVMODE_CHECK_T;
			mutex_unlock(&chg->lock);
			schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
			break;
		}
	}

Err:
	//pr_info("%s: End, ret=%d\n", __func__, ret);
	return ret;
}
static void nu2111a_timer_work(struct work_struct *work)
{
	struct nu2111a_charger *chg = container_of(work, struct nu2111a_charger, timer_work.work);
	int ret = 0;
	unsigned int val;

	LOG_DBG(chg, "start!, timer_id =[%d], charging_state=[%d]\n", chg->timer_id, chg->charging_state);

	switch (chg->timer_id) {
	case TIMER_VBATMIN_CHECK:
		ret = nu2111a_check_starting_vbat_level(chg);
		if (ret < 0)
			goto error;
		break;

	case TIMER_PRESET_DC:
		ret = nu2111a_start_dc_charging(chg);
		if (ret < 0)
			goto error;
		break;

	case TIMER_PRESET_CONFIG:
		ret = nu2111a_preset_config(chg);
		if (ret < 0)
			goto error;
		break;

	case TIMER_CHECK_ACTIVE:
		ret = nu2111a_check_active_state(chg);
		if (ret < 0)
			goto error;
		break;

	case TIMER_ADJUST_CCMODE:
		ret = nu2111a_adjust_ccmode(chg);
		if (ret < 0)
			goto error;
		break;

	case TIMER_ENTER_CCMODE:
		ret = nu2111a_enter_ccmode(chg);
		if (ret < 0)
			goto error;
		break;

	case TIMER_CHECK_CCMODE:
		ret = nu2111a_ccmode_regulation_process(chg);
		if (ret < 0)
			goto error;
		break;

	case TIMER_CHECK_CVMODE:
		ret = nu2111a_cvmode_process(chg);
		if (ret < 0)
			goto error;
		break;

	case TIMER_PDMSG_SEND:
		if (chg->ta_type == TA_TYPE_PD_FPDO) {
			/* Send PD Message */
			ret = nu2111a_send_pd_message(chg, PD_MSG_REQUEST_FIXED_PDO);
		} else {
			/* Adjust TA current and voltage step */
			val = chg->ta_vol/PD_MSG_TA_VOL_STEP;	/* PPS voltage resolution is 20mV */
			chg->ta_vol = val*PD_MSG_TA_VOL_STEP;
			val = chg->ta_cur/PD_MSG_TA_CUR_STEP;	/* PPS current resolution is 50mA */
			chg->ta_cur = val*PD_MSG_TA_CUR_STEP;
			if (chg->ta_cur < NU2111A_TA_MIN_CUR)	/* PPS minimum current is 1000mA */
				chg->ta_cur = NU2111A_TA_MIN_CUR;
			/* Send PD Message */
			ret = nu2111a_send_pd_message(chg, PD_MSG_REQUEST_APDO);
		}
		if (ret < 0)
			goto error;

		/* Go to the next state */
		mutex_lock(&chg->lock);
		switch (chg->charging_state) {
		case DC_STATE_PRESET_DC:
			chg->timer_id = TIMER_PRESET_CONFIG;
			break;
		case DC_STATE_ADJUST_CC:
			chg->timer_id = TIMER_ADJUST_CCMODE;
			break;
		case DC_STATE_START_CC:
			chg->timer_id = TIMER_ENTER_CCMODE;
			break;
		case DC_STATE_CC_MODE:
			chg->timer_id = TIMER_CHECK_CCMODE;
			break;
		case DC_STATE_CV_MODE:
			chg->timer_id = TIMER_CHECK_CVMODE;
			break;
		case DC_STATE_ADJUST_TAVOL:
			chg->timer_id = TIMER_ADJUST_TAVOL;
			break;
		case DC_STATE_ADJUST_TACUR:
			chg->timer_id = TIMER_ADJUST_TACUR;
			break;
#ifdef CONFIG_PASS_THROUGH
		case DC_STATE_PTMODE_CHANGE:
			chg->timer_id = TIMER_PTMODE_CHANGE;
			break;
		case DC_STATE_PT_MODE:
			chg->timer_id = TIMER_CHECK_PTMODE;
			break;
#endif
		default:
			ret = -EINVAL;
			mutex_unlock(&chg->lock);
			goto error;
		}

		chg->timer_period = NU2111A_PDMSG_WAIT_T;
		mutex_unlock(&chg->lock);
		schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
		break;

	case TIMER_ADJUST_TAVOL:
		ret = nu2111a_adjust_ta_voltage(chg);
		if (ret < 0)
			goto error;
		break;
	case TIMER_ADJUST_TACUR:
		ret = nu2111a_adjust_ta_current(chg);
		if (ret < 0)
			goto error;
		break;
#ifdef CONFIG_PASS_THROUGH
	case TIMER_PTMODE_CHANGE:
		ret = nu2111a_ptmode_change(chg);
		if (ret < 0)
			goto error;
		break;
	case TIMER_CHECK_PTMODE:
		ret = nu2111a_pass_through_mode_process(chg);
		if (ret < 0)
			goto error;
		break;
#endif
	case TIMER_START_REVERSE:
		ret = nu2111a_charge_start_reverse(chg);
		if (ret < 0)
			goto error;
		break;

	case TIMER_CHECK_REVERSE_ACTIVE:
		ret = nu2111a_check_reverse_active_state(chg);
		if (ret < 0)
			goto error;
		break;

	case TIMER_CHECK_REVERSE_MODE:
		ret = nu2111a_charge_reverse_mode(chg);
		if (ret < 0)
			goto error;
		break;
	case TIMER_CHECK_FPDOCVMODE:
		ret = nu2111a_fpdo_cvmode_process(chg);
		if (ret < 0)
			goto error;
		break;
	default:
		break;
	}

	if (chg->charging_state == DC_STATE_NOT_CHARGING) {
		LOG_DBG(chg, "Cancel timer_work, because NOT charging!!\n");
		cancel_delayed_work(&chg->timer_work);
		cancel_delayed_work(&chg->pps_work);
	}

	LOG_DBG(chg, "END!!\n");
	return;

error:
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
if (chg->charging_state > DC_STATE_CHECK_VBAT)
	chg->health_status = POWER_SUPPLY_EXT_HEALTH_DC_ERR;
else
	LOG_DBG(chg, "Charging not started[%d].\n", chg->health_status);
#endif
	nu2111a_stop_charging(chg);
}

static void nu2111a_pps_request_work(struct work_struct *work)
{
	struct nu2111a_charger *chg = container_of(work, struct nu2111a_charger,
						 pps_work.work);
	int ret = 0;

	/* Send PD message */
	ret = nu2111a_send_pd_message(chg, PD_MSG_REQUEST_APDO);
	LOG_DBG(chg, "Every 7.5 Sec Send PD Message End, ret = %d\n", ret);
}

#ifdef NU2111A_STEP_CHARGING
static int nu2111a_prepare_nextstep(struct nu2111a_charger *chg)
{
	int ret = 0;
	int vbat;
	unsigned int val;

	nu2111a_set_charging_state(chg, DC_STATE_PRESET_DC);

	nu2111a_adc_oneshot(chg, true);
	ret = nu2111a_get_adc_channel(chg, ADCCH_VBAT);
	nu2111a_adc_oneshot(chg, false);
	if (ret < 0)
		goto Err;

	vbat = chg->adc_vbat;
	LOG_DBG(chg, "Vbat is [%d]\n", vbat);
	LOG_DBG(chg, "(NU2111A_STEP1_VBAT_REG - 100000): [%d]\n", (NU2111A_STEP1_VBAT_REG - 100000));

	//if ((vbat >= NU2111A_STEP1_VBAT_REG) && (vbat < NU2111A_STEP2_VBAT_REG)) {//4.2 - 4.3  4160000 - 4290000
	if (chg->current_step == STEP_ONE) {//4.2 - 4.3  4160000 - 4290000
		/* Step2 Charging! */
		LOG_DBG(chg, "Step2 Charging Start!\n");
		chg->current_step = STEP_TWO;
		chg->pdata->vbat_reg = NU2111A_STEP2_VBAT_REG; //4.3V
		chg->pdata->iin_cfg = NU2111A_STEP2_TARGET_IIN;
		chg->pdata->iin_topoff = NU2111A_STEP2_TOPOFF;//1250000
	} else if (chg->current_step == STEP_TWO) {
		/* Step3 Charging! */
		LOG_DBG(chg, "Step3 Charging Start!\n");
		chg->current_step = STEP_THREE;
		chg->pdata->vbat_reg = NU2111A_STEP3_VBAT_REG;//4.42
		chg->pdata->iin_cfg = NU2111A_STEP3_TARGET_IIN;
		chg->pdata->iin_topoff = NU2111A_STEP3_TOPOFF;//500000
	} else {
		LOG_DBG(chg, " Step end???????????\n");
	}

	chg->iin_cc = MIN(chg->pdata->iin_cfg, chg->ta_max_cur);
	chg->pdata->iin_cfg = chg->iin_cc;

	val = chg->iin_cc / PD_MSG_TA_CUR_STEP;
	chg->iin_cc = val * PD_MSG_TA_CUR_STEP;
	chg->ta_cur = chg->iin_cc;
	chg->iin_cc = chg->pdata->iin_cfg;

	LOG_DBG(chg, "ta_max_vol=%d, ta_max_cur=%d, ta_max_pwr=%d, iin_cc=%d, TA-VOL[%d], TA-CUR[%d]\n",
		chg->ta_max_vol, chg->ta_max_cur,
		chg->ta_max_pwr, chg->iin_cc, chg->ta_vol, chg->ta_cur);

	mutex_lock(&chg->lock);
	chg->timer_id = TIMER_PDMSG_SEND;
	chg->timer_period = 0;
	mutex_unlock(&chg->lock);
	schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));

Err:
	LOG_DBG(chg, "End, ret =%d\n", ret);
	return ret;
}

static void nu2111a_step_charging_work(struct work_struct *work)
{
	struct nu2111a_charger *chg = container_of(work, struct nu2111a_charger, step_work.work);
	int ret = 0;

	if (chg->charging_state == DC_STATE_NOT_CHARGING) {
		/*Start from VBATMIN CHECK */
		mutex_lock(&chg->lock);
		chg->timer_id = TIMER_VBATMIN_CHECK;
		chg->timer_period = NU2111A_VBATMIN_CHECK_T;
		mutex_unlock(&chg->lock);
		schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
	} else if (chg->charging_state == DC_STATE_CV_MODE) {
		/* Go back to PRESET DC */
		ret = nu2111a_prepare_nextstep(chg);
		if (ret < 0)
			goto ERR;
	} else {
		LOG_DBG(chg, "invalid charging STATE, charging_state[%d]\n", chg->charging_state);
	}

ERR:
	LOG_DBG(chg, "ret = %d\n", ret);
}
#endif

static int nu2111a_psy_set_property(struct power_supply *psy, enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct nu2111a_charger *chg = power_supply_get_drvdata(psy);
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	enum power_supply_ext_property ext_psp = (enum power_supply_ext_property) psp;
#endif
	int ret = 0;
	unsigned int temp = 0;

	LOG_DBG(chg, "Start!, prop==[%d], val==[%d]\n", psp, val->intval);

	switch ((int)psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		LOG_DBG(chg, "ONLINE: [%d]\r\n", chg->dcerr_retry_cnt);
		if (chg->online) {
			/* charger is enabled, need to stop charging!! */
			if (!val->intval) {
				chg->online = false;
				ret = nu2111a_stop_charging(chg);
				chg->dcerr_retry_cnt = 0;
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
				chg->health_status = POWER_SUPPLY_HEALTH_GOOD;
#endif
				if (ret < 0)
					goto Err;
			} else {
				//pr_info("%s: charger is already enabled!\n", __func__);
			}
		} else {
			/* charger is disabled, need to start charging!! */
			if (val->intval) {
				chg->online = true;
#if !IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
#ifndef NU2111A_STEP_CHARGING
				mutex_lock(&chg->lock);
				chg->timer_id = TIMER_VBATMIN_CHECK;
				chg->timer_period = NU2111A_VBATMIN_CHECK_T;//1000
				mutex_unlock(&chg->lock);
				schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
#else
				schedule_delayed_work(&chg->step_work, 0);
#endif
#endif
				ret = 0;
			} else {
				if (delayed_work_pending(&chg->timer_work)) {
					cancel_delayed_work(&chg->timer_work);
					cancel_delayed_work(&chg->pps_work);
					LOG_DBG(chg, "cancel work queue!!");
				}

				LOG_DBG(chg, "charger is already disabled!!\n");
			}
		}
		break;

	case POWER_SUPPLY_PROP_STATUS:
		LOG_DBG(chg, "STATUS:\r\n");

		break;
	case POWER_SUPPLY_PROP_CHARGE_NOW:
		/* add this code for nuvolta switching charger */
		if (val->intval) {
			LOG_DBG(chg, "set from switching charger!\r\n");
			chg->online = true;
#ifndef NU2111A_STEP_CHARGING
			mutex_lock(&chg->lock);
			chg->timer_id = TIMER_VBATMIN_CHECK;
			chg->timer_period = NU2111A_VBATMIN_CHECK_T;
			mutex_unlock(&chg->lock);
			schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
#else
			schedule_delayed_work(&chg->step_work, 0);
#endif
			ret = 0;
		}
		break;

	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
		chg->float_voltage = val->intval;
		temp = chg->float_voltage * NU2111A_SEC_DENOM_U_M;
#endif
		LOG_DBG(chg, "request new vbat reg[%d] / old[%d]\n", temp, chg->new_vbat_reg);
		if (temp != chg->new_vbat_reg) {
			chg->new_vbat_reg = temp;
			if ((chg->charging_state == DC_STATE_NOT_CHARGING) ||
				(chg->charging_state == DC_STATE_CHECK_VBAT)) {
				/* Set new vbat-reg to pdata->vbat-reg, before PRESET_DC state */
				chg->pdata->vbat_reg = chg->new_vbat_reg;
			} else {
				if (chg->req_new_vbat_reg == true) {
					LOG_DBG(chg, "previous vbat-reg is not finished yet\n");
					ret = -EBUSY;
				} else {
					/* set new vbat reg voltage flag*/
					mutex_lock(&chg->lock);
					chg->req_new_vbat_reg = true;
					mutex_unlock(&chg->lock);

					/* Check the charging state */
					if ((chg->charging_state == DC_STATE_CC_MODE) ||
							(chg->charging_state == DC_STATE_CV_MODE) ||
							(chg->charging_state == DC_STATE_PT_MODE) ||
							(chg->charging_state == DC_STATE_CHARGING_DONE) ||
							(chg->charging_state == DC_STATE_FPDO_CV_MODE)) {
						/* cancel delayed_work and  restart queue work again */
						cancel_delayed_work(&chg->timer_work);
						mutex_lock(&chg->lock);
						chg->timer_period = 0;
						mutex_unlock(&chg->lock);
						schedule_delayed_work(&chg->timer_work,
								msecs_to_jiffies(chg->timer_period));
					} else {
						/* Wait for next valid state - cc, cv, or bypass state */
						LOG_DBG(chg, "Unsupported charging state[%d]\n", chg->charging_state);
					}
				}
			}
		}
		break;

	case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
		chg->input_current = val->intval;
		temp = chg->input_current * NU2111A_SEC_DENOM_U_M;
		if (chg->ta_type == TA_TYPE_PD_FPDO && temp < chg->pdata->fpdo_dc_iin_lowest_limit) {
			LOG_DBG(chg, "PSP_ICL, IIN LOWEST LIMIT! IIN %d -> %d\n",
					temp, chg->pdata->fpdo_dc_iin_lowest_limit);
			temp = chg->pdata->fpdo_dc_iin_lowest_limit;
		}
#endif
		LOG_DBG(chg, "INPUT_CURRENT_LIMIT: request new iin[%d]  / old[%d]\n", temp, chg->new_iin);
		if (temp != chg->new_iin || chg->new_iin != chg->pdata->iin_cfg) {
			chg->new_iin = temp;
			if ((chg->charging_state == DC_STATE_NOT_CHARGING) ||
				(chg->charging_state == DC_STATE_CHECK_VBAT)) {
				/* Set new iin to pdata->iin_cfg, before PRESET_DC state */
				chg->pdata->iin_cfg = chg->new_iin;
			} else {
				if (chg->req_new_iin == true) {
					LOG_DBG(chg, "previous iin-req is not finished yet\n");
					ret = -EBUSY;
				} else {
					/* set new iin flag */
					mutex_lock(&chg->lock);
					chg->req_new_iin = true;
					mutex_unlock(&chg->lock);

					/* Check the charging state */
					if ((chg->charging_state == DC_STATE_CC_MODE) ||
						(chg->charging_state == DC_STATE_CV_MODE) ||
						(chg->charging_state == DC_STATE_PT_MODE) ||
						(chg->charging_state == DC_STATE_CHARGING_DONE) ||
						(chg->charging_state == DC_STATE_FPDO_CV_MODE)) {
						/* cancel delayed_work and  restart queue work again */
						cancel_delayed_work(&chg->timer_work);
						mutex_lock(&chg->lock);
						chg->timer_period = 0;
						mutex_unlock(&chg->lock);
						schedule_delayed_work(&chg->timer_work,
								msecs_to_jiffies(chg->timer_period));
					} else {
						/* Wait for next valid state - cc, cv, or bypass state */
						LOG_DBG(chg, "Unsupported charging state[%d]\n", chg->charging_state);
					}
				}
			}
		}
		break;

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE_MAX:
		chg->pdata->vbat_reg_max = val->intval * NU2111A_SEC_DENOM_U_M;
		pr_info("%s: vbat_reg_max(%duV)\n", __func__, chg->pdata->vbat_reg_max);
		break;

	case POWER_SUPPLY_EXT_PROP_MIN ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
		case POWER_SUPPLY_EXT_PROP_DIRECT_WDT_CONTROL:
			if (val->intval) {
				chg->wdt_kick = true;
				chg->wdt_test_resume = 0;
			} else {
				chg->wdt_kick = false;
				i2c_mark_adapter_resumed(chg->client->adapter);
				if (chg->regmap_sub)
					i2c_mark_adapter_resumed(chg->client_sub->adapter);
				cancel_delayed_work(&chg->wdt_control_work);
			}
			pr_info("%s: wdt kick (%d)\n", __func__, chg->wdt_kick);
			break;

		case POWER_SUPPLY_EXT_PROP_DIRECT_CURRENT_MAX:
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
			chg->input_current = val->intval;
			temp = chg->input_current * NU2111A_SEC_DENOM_U_M;
			if (chg->ta_type == TA_TYPE_PD_FPDO && temp < chg->pdata->fpdo_dc_iin_lowest_limit) {
				LOG_DBG(chg, "PSP_ICL, IIN LOWEST LIMIT! IIN %d -> %d\n",
						temp, chg->pdata->fpdo_dc_iin_lowest_limit);
				temp = chg->pdata->fpdo_dc_iin_lowest_limit;
			}
#endif
			LOG_DBG(chg, "DIRECT_CURRENT_MAX: request new iin[%d] / old[%d]\n", temp, chg->new_iin);
			if (temp != chg->new_iin || chg->new_iin != chg->pdata->iin_cfg) {
				chg->new_iin = temp;
				if ((chg->charging_state == DC_STATE_NOT_CHARGING) ||
					(chg->charging_state == DC_STATE_CHECK_VBAT)) {
					/* Set new iin to pdata->iin_cfg, before PRESET_DC state */
					chg->pdata->iin_cfg = chg->new_iin;
				} else {
					if (chg->req_new_iin == true) {
						LOG_DBG(chg, "previous iin-req is not finished yet\n");
						ret = -EBUSY;
					} else {
						/* set new iin flag */
						mutex_lock(&chg->lock);
						chg->req_new_iin = true;
						mutex_unlock(&chg->lock);

						/* Check the charging state */
						if ((chg->charging_state == DC_STATE_CC_MODE) ||
							(chg->charging_state == DC_STATE_CV_MODE) ||
							(chg->charging_state == DC_STATE_PT_MODE) ||
							(chg->charging_state == DC_STATE_CHARGING_DONE) ||
							(chg->charging_state == DC_STATE_FPDO_CV_MODE)) {
							/* cancel delayed_work and	restart queue work again */
							cancel_delayed_work(&chg->timer_work);
							mutex_lock(&chg->lock);
							chg->timer_period = 0;
							mutex_unlock(&chg->lock);
							schedule_delayed_work(&chg->timer_work,
									msecs_to_jiffies(chg->timer_period));
						} else {
							/* Wait for next valid state - cc, cv, or bypass state */
							LOG_DBG(chg, "Unsupported charging state[%d]\n",
										chg->charging_state);
						}
					}
				}
			}
			break;
		case POWER_SUPPLY_EXT_PROP_CHARGING_ENABLED:
			if (val->intval == 0) {
				// Stop Direct Charging
				ret = nu2111a_stop_charging(chg);
				chg->health_status = POWER_SUPPLY_HEALTH_GOOD;
				if (ret < 0)
					goto Err;
			} else {
				if (chg->charging_state != DC_STATE_NOT_CHARGING) {
					pr_info("%s: duplicate charging enabled(%d)\n", __func__, val->intval);
					goto Err;
				}
				if (!chg->online) {
					pr_info("%s: online is not attached(%d)\n", __func__, val->intval);
					goto Err;
				}
#ifndef NU2111A_STEP_CHARGING
				pm_wakeup_ws_event(chg->monitor_ws, 10000, false);
				nu2111a_set_charging_state(chg, DC_STATE_CHECK_VBAT);
				mutex_lock(&chg->lock);
				chg->timer_id = TIMER_VBATMIN_CHECK;
				chg->timer_period = NU2111A_VBATMIN_CHECK_T;
				mutex_unlock(&chg->lock);
				schedule_delayed_work(&chg->timer_work, msecs_to_jiffies(chg->timer_period));
#else
				schedule_delayed_work(&chg->step_work, 0);
#endif
				ret = 0;
			}
			break;
#ifdef CONFIG_PASS_THROUGH
		case POWER_SUPPLY_EXT_PROP_PASS_THROUGH_MODE:
			LOG_DBG(chg, "[PASS_THROUGH] called\n");
			if (val->intval != chg->pass_through_mode) {
				/* Request new dc mode */
				chg->pass_through_mode = val->intval;
				/* Check the charging state */
				if (chg->charging_state == DC_STATE_NOT_CHARGING) {
					/* Not support state */
					LOG_DBG(chg, "Not support dc mode in charging state=%d\n", chg->charging_state);
					ret = -EINVAL;
				} else {
					/* Check whether the previous request is done or not */
					if (chg->charging_state == true) {
						/* The previous request is not done yet */
						LOG_DBG(chg, "There is the previous request for New DC mode\n");
						ret = -EBUSY;
					} else {
						/* Set request flag */
						mutex_lock(&chg->lock);
						chg->req_pt_mode = true;
						mutex_unlock(&chg->lock);
						/* Check the charging state */
						if ((chg->charging_state == DC_STATE_CC_MODE) ||
							(chg->charging_state == DC_STATE_CV_MODE) ||
							(chg->charging_state == DC_STATE_PT_MODE)) {
							/* cancel delayed_work */
							cancel_delayed_work(&chg->timer_work);
							/* do delayed work at once */
							mutex_lock(&chg->lock);
							chg->timer_period = 0;	// ms unit
							mutex_unlock(&chg->lock);
							schedule_delayed_work(&chg->timer_work,
								msecs_to_jiffies(chg->timer_period));
						} else {
							/* Wait for next valid state - cc, cv, or bypass state */
							LOG_DBG(chg, "Not support new dc mode yet in state=%d\n",
									chg->charging_state);
						}
					}
				}
			}
			break;
		case POWER_SUPPLY_EXT_PROP_PASS_THROUGH_MODE_TA_VOL:
			if (chg->pass_through_mode != PTM_NONE) {
				LOG_DBG(chg, "ptmode voltage!\n");
				nu2111a_set_ptmode_ta_voltage_by_soc(chg, val->intval);
			} else {
				LOG_DBG(chg, "not in ptmode!\n");
			}
			break;
#endif
		case POWER_SUPPLY_EXT_PROP_DC_VIN_OVERCURRENT:
			/* Set VIN OCP current */
			chg->iin_rev = val->intval;
			ret = nu2111a_set_ocp_current(chg, chg->iin_rev);
			break;
		case POWER_SUPPLY_EXT_PROP_DC_REVERSE_MODE:
			/* Set reverse mode */
			chg->rev_mode = val->intval;
			/* Set reverse mode */
			if (chg->rev_mode == POWER_SUPPLY_DC_REVERSE_STOP) {
				/* Cancel delayed_work */
				cancel_delayed_work(&chg->timer_work);
				/* Stop reverse mode */
				ret = nu2111a_stop_charging(chg);
			} else {
				if (chg->charging_state == DC_STATE_NOT_CHARGING) {
					/* Start Reverse Mode */
					mutex_lock(&chg->lock);
					chg->charging_state = DC_STATE_REVERSE_MODE;
					chg->timer_id = TIMER_START_REVERSE;
					chg->timer_period = 0;
					mutex_unlock(&chg->lock);
					schedule_delayed_work(&chg->timer_work,
						msecs_to_jiffies(chg->timer_period));

				} else {
					/* Charging state does not support reverse mode */
					LOG_DBG(chg, "Not support reverse mode in state=%d\n",
							chg->charging_state);
					ret = -EINVAL;
				}
			}
			break;
		default:
			return -EINVAL;
		}
		break;
#endif
	default:
		return -EINVAL;
	}

Err:
	//LOG_DBG(chg, "End, ret== %d\n", ret);
	return ret;
}

static int nu2111a_psy_get_property(struct power_supply *psy, enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct nu2111a_charger *chg = power_supply_get_drvdata(psy);
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	enum power_supply_ext_property ext_psp = (enum power_supply_ext_property) psp;
#endif
	int ret = 0;

	switch ((int)psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		LOG_DBG(chg, "ONLINE:, [%d]\n", chg->online);
		val->intval = chg->online;
		break;

	case POWER_SUPPLY_PROP_STATUS:
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
		val->intval = (chg->charging_state == DC_STATE_NOT_CHARGING) ?
			POWER_SUPPLY_STATUS_DISCHARGING : POWER_SUPPLY_STATUS_CHARGING;
#else
		val->intval = chg->online ? POWER_SUPPLY_STATUS_CHARGING : POWER_SUPPLY_STATUS_DISCHARGING;
#endif
		break;

	case POWER_SUPPLY_PROP_POWER_NOW:
		if ((chg->charging_state >= DC_STATE_CHECK_ACTIVE) && (chg->charging_state <= DC_STATE_CV_MODE)) {
			nu2111a_adc_oneshot(chg, true);
			ret = nu2111a_get_adc_channel(chg, ADCCH_VBAT);
			nu2111a_adc_oneshot(chg, false);
			if (ret < 0) {
				pr_err("[%s] can't read adc!\n", __func__);
				return ret;
			}
			val->intval = chg->adc_vbat;
		}
		break;
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	case POWER_SUPPLY_PROP_HEALTH:
		LOG_DBG(chg, "POWER_SUPPLY_PROP_HEALTH: [%d]\n", chg->health_status);
		val->intval = chg->health_status;
		break;

	case POWER_SUPPLY_PROP_TEMP:
		nu2111a_adc_oneshot(chg, true);
		nu2111a_get_adc_channel(chg, ADCCH_TDIE);
		nu2111a_adc_oneshot(chg, false);
		val->intval = chg->adc_ntc;
		break;

	case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
		val->intval = chg->float_voltage;
		break;

	case POWER_SUPPLY_EXT_PROP_MIN ... POWER_SUPPLY_EXT_PROP_MAX:
		switch (ext_psp) {
		case POWER_SUPPLY_EXT_PROP_MONITOR_WORK:
			nu2111a_monitor_work(chg);
			break;
		case POWER_SUPPLY_EXT_PROP_MEASURE_INPUT:
			switch (val->intval) {
			case SEC_BATTERY_IIN_MA:
				nu2111a_adc_oneshot(chg, true);
				nu2111a_get_adc_channel(chg, ADCCH_IIN);
				nu2111a_adc_oneshot(chg, false);
				if (chg->regmap_sub)
					val->intval = (chg->adc_iin + chg->adc_iin_sub) / NU2111A_SEC_DENOM_U_M;
				else
					val->intval = chg->adc_iin / NU2111A_SEC_DENOM_U_M;
				break;
			case SEC_BATTERY_IIN_UA:
				nu2111a_adc_oneshot(chg, true);
				nu2111a_get_adc_channel(chg, ADCCH_IIN);
				nu2111a_adc_oneshot(chg, false);
				if (chg->regmap_sub)
					val->intval = chg->adc_iin + chg->adc_iin_sub;
				else
					val->intval = chg->adc_iin;
				break;
			case SEC_BATTERY_VIN_MA:
				nu2111a_adc_oneshot(chg, true);
				nu2111a_get_adc_channel(chg, ADCCH_VIN);
				nu2111a_adc_oneshot(chg, false);
				val->intval = chg->adc_vin / NU2111A_SEC_DENOM_U_M;
				break;
			case SEC_BATTERY_VIN_UA:
				nu2111a_adc_oneshot(chg, true);
				nu2111a_get_adc_channel(chg, ADCCH_VIN);
				nu2111a_adc_oneshot(chg, false);
				val->intval = chg->adc_vin;
				break;
			default:
				val->intval = 0;
				break;
			}
			break;
		case POWER_SUPPLY_EXT_PROP_DIRECT_CHARGER_CHG_STATUS:
			val->strval = charging_state_str[chg->charging_state];
			break;
		case POWER_SUPPLY_EXT_PROP_DC_VIN_OVERCURRENT:
			/* Get vin_ocp_current_12_11 */
			val->intval = chg->iin_rev;
			break;
		case POWER_SUPPLY_EXT_PROP_DC_REVERSE_MODE:
			/* Get reverse mode */
			val->intval = chg->rev_mode;
			break;
		case POWER_SUPPLY_EXT_PROP_DC_ERROR_CAUSE:
			/* Get dc err cause */
			val->intval = chg->dcerr_reason;
			break;
		case POWER_SUPPLY_EXT_PROP_D2D_REVERSE_VBUS:
			nu2111a_adc_oneshot(chg, true);
			nu2111a_get_adc_channel(chg, ADCCH_VIN);
			nu2111a_adc_oneshot(chg, false);
			if (chg->adc_vin)
				val->intval = chg->adc_vin / NU2111A_SEC_DENOM_U_M;
			else
				val->intval = 0;
			break;
		default:
			return -EINVAL;
		}
		break;
#endif
	default:
		return -EINVAL;
	}

	return 0;
}

static int nu2111a_regmap_init(struct nu2111a_charger *chg)
{
	int ret;
	// LOG_DBG(chg, "Start!!\n");
	if (!i2c_check_functionality(chg->client->adapter, I2C_FUNC_I2C)) {
		dev_err(chg->dev, "%s: check_functionality failed.", __func__);
		return -ENODEV;
	}

	chg->regmap = devm_regmap_init_i2c(chg->client, &nu2111a_regmap_config);
	if (IS_ERR(chg->regmap)) {
		ret = PTR_ERR(chg->regmap);
		dev_err(chg->dev, "regmap init failed, err == %d\n", ret);
		return PTR_ERR(chg->regmap);
	}
	i2c_set_clientdata(chg->client, chg);

	return 0;
}

static int read_reg(void *data, u64 *val)
{
	struct nu2111a_charger *chg = data;
	int rc;
	unsigned int temp;

	LOG_DBG(chg, "Start!\n");
	rc = regmap_read(chg->regmap, chg->debug_address, &temp);
	if (rc) {
		pr_err("Couldn't read reg 0x%x rc = %d\n",
			chg->debug_address, rc);
		return -EAGAIN;
	}

	LOG_DBG(chg, "address = [0x%x], value = [0x%x]\n", chg->debug_address, temp);
	*val = temp;
	return 0;
}

static int write_reg(void *data, u64 val)
{
	struct nu2111a_charger *chg = data;
	int rc;
	u8 temp;

	LOG_DBG(chg, "Start! val == [%x]\n", (u8)val);
	temp = (u8) val;
	rc = regmap_write(chg->regmap, chg->debug_address, temp);
	if (rc) {
		pr_err("Couldn't write 0x%02x to 0x%02x rc= %d\n",
			temp, chg->debug_address, rc);
		return -EAGAIN;
	}
	return 0;
}

static int get_vbat_reg(void *data, u64 *val)
{
	struct nu2111a_charger *chg = data;
	unsigned int temp;

	LOG_DBG(chg, "Start!\n");

	temp = chg->pdata->vbat_reg;
	*val = temp;
	return 0;
}

static int set_vbat_reg(void *data, u64 val)
{
	struct nu2111a_charger *chg = data;
	union power_supply_propval value;
	struct power_supply *psy = power_supply_get_by_name(nu2111a_supplied_to[0]);

	LOG_DBG(chg, "Start! new_vbat_reg == [%d]\n", (unsigned int)val);
	value.intval = val;
	power_supply_set_property(psy, POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE, &value);
	LOG_DBG(chg, "new_vbat_reg == [%d]\n", chg->new_vbat_reg);
	return 0;
}

static int get_iin_cfg(void *data, u64 *val)
{
	struct nu2111a_charger *chg = data;
	unsigned int temp;

	LOG_DBG(chg, "Start!\n");

	temp = chg->pdata->iin_cfg;
	*val = temp;
	return 0;
}

static int set_iin_cfg(void *data, u64 val)
{
	struct nu2111a_charger *chg = data;
	union power_supply_propval value;
	struct power_supply *psy = power_supply_get_by_name(nu2111a_supplied_to[0]);

	LOG_DBG(chg, "Start! new_iin == [%d]\n", (unsigned int)val);
	value.intval = val;
	power_supply_set_property(psy, POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT, &value);
	LOG_DBG(chg, "new_iin == [%d]\n", chg->new_iin);
	return 0;
}

/* Below Debugfs us only used for the bad-ta_connection test! */
static int set_tp_set_cfg(void *data, u64 val)
{
	struct nu2111a_charger *chg = data;
	union power_supply_propval value;

	LOG_DBG(chg, "[Set tp_set:[%d]\n", (unsigned int)val);
	//chg->tp_set = val;
	chg->rev_mode = val;

	value.intval = val;
	 psy_do_property(chg->pdata->sec_dc_name, set,
		POWER_SUPPLY_EXT_PROP_OTG_VBUS_CTRL, value);

	return 0;
}

static int get_tp_set_cfg(void *data, u64 *val)
{
	struct nu2111a_charger *chg = data;
//	unsigned int temp;

	LOG_DBG(chg, "Start!\n");

	//temp = chg->tp_set;
	*val = chg->rev_mode;

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(register_debug_ops, read_reg, write_reg, "0x%02llx\n");
DEFINE_SIMPLE_ATTRIBUTE(vbat_reg_debug_ops, get_vbat_reg, set_vbat_reg, "%02lld\n");
DEFINE_SIMPLE_ATTRIBUTE(iin_cfg_debug_ops, get_iin_cfg, set_iin_cfg, "%02lld\n");
DEFINE_SIMPLE_ATTRIBUTE(tp_set_debug_ops, get_tp_set_cfg, set_tp_set_cfg, "%02lld\n");

static int nu2111a_create_debugfs_entries(struct nu2111a_charger *chg)
{
	struct dentry *ent;
	int rc = 0;

	chg->debug_root = debugfs_create_dir("nu2111a-debugfs", NULL);
	if (!chg->debug_root) {
		dev_err(chg->dev, "Couldn't create debug dir\n");
		rc = -ENOENT;
	} else {
		debugfs_create_x32("address", S_IFREG | 0644,
				chg->debug_root,
				&(chg->debug_address));

		ent = debugfs_create_file("data", S_IFREG | 0644,
				chg->debug_root, chg,
				&register_debug_ops);
		if (!ent) {
			dev_err(chg->dev, "Couldn't create data debug file\n");
			rc = -ENOENT;
		}

		ent = debugfs_create_file("vbat-reg", S_IFREG | 0644,
				chg->debug_root, chg,
				&vbat_reg_debug_ops);
		if (!ent) {
			dev_err(chg->dev, "Couldn't create vbat-reg file\n");
			rc = -ENOENT;
		}
		ent = debugfs_create_file("iin-cfg", S_IFREG | 0644,
				chg->debug_root, chg,
				&iin_cfg_debug_ops);
		if (!ent) {
			dev_err(chg->dev, "Couldn't create iin-cfg file\n");
			rc = -ENOENT;
		}
		ent = debugfs_create_file("tp-set", S_IFREG | 0644,
			chg->debug_root, chg,
			&tp_set_debug_ops);
		if (!ent) {
			dev_err(chg->dev, "Couldn't create tp-set file\n");
			rc = -ENOENT;
		}
	}

	return rc;
}

ssize_t nu2111a_chg_show_attrs(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct nu2111a_charger *chg =	power_supply_get_drvdata(psy);
	const ptrdiff_t offset = attr - nu2111a_charger_attrs;
	int i = 0;

	switch (offset) {
	case TA_V_OFFSET:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", chg->ta_v_offset);
		break;
	case TA_C_OFFSET:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", chg->ta_c_offset);
		break;
	case IIN_C_OFFSET:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", chg->iin_c_offset);
		break;
	case TEST_MODE:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", chg->test_mode);
		break;
	case PPS_VOL:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", chg->pps_vol);
		break;
	case PPS_CUR:
		i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", chg->pps_cur);
		break;
	case VFLOAT:
		if (chg)
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", chg->pdata->vbat_reg);
		break;
	case IINCC:
		if (chg)
			i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", chg->pdata->iin_cfg);
		break;
	case DCERR_REASON:
		i += scnprintf(buf + i, PAGE_SIZE - i, "0x%x\n", chg->dcerr_reason);
		break;
	default:
		return -EINVAL;
	}
	return i;
}

ssize_t nu2111a_chg_store_attrs(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct power_supply *psy = dev_get_drvdata(dev);
	struct nu2111a_charger *chg =	power_supply_get_drvdata(psy);
	const ptrdiff_t offset = attr - nu2111a_charger_attrs;
	int ret = 0;
	int x;

	switch (offset) {
	case TA_V_OFFSET:
		if (sscanf(buf, "%d", &x) == 1)
			chg->ta_v_offset = x;
		ret = count;
		break;
	case TA_C_OFFSET:
		if (sscanf(buf, "%d", &x) == 1)
			chg->ta_c_offset = x;
		ret = count;
		break;
	case IIN_C_OFFSET:
		if (sscanf(buf, "%d", &x) == 1)
			chg->iin_c_offset = x;
		ret = count;
		break;
	case TEST_MODE:
		if (sscanf(buf, "%d", &x) == 1)
			chg->test_mode = x;
		ret = count;
		break;
	case PPS_VOL:
		if (sscanf(buf, "%d", &x) == 1)
			chg->pps_vol = x;
		ret = count;
		break;
	case PPS_CUR:
		if (sscanf(buf, "%d", &x) == 1)
			chg->pps_cur = x;
		ret = count;
		break;
	case VFLOAT:
		pr_info("%s: vfloat store\n", __func__);
//		if (sscanf(buf, "%d", &x) == 1)
//			if (chg)
//				set_vbat_reg(chg, x);
		ret = count;
		break;
	case IINCC:
		pr_info("%s: iincc store\n", __func__);
//		if (sscanf(buf, "%d", &x) == 1)
//			if (chg)
//				set_iin_cfg(chg, x);
		ret = count;
		break;
	case DCERR_REASON:
		break;
	default:
		ret = -EINVAL;
	}
	return ret;
}

static int nu2111a_chg_create_attrs(struct device *dev)
{
	unsigned long i;
	int rc;

	for (i = 0; i < ARRAY_SIZE(nu2111a_charger_attrs); i++) {
		rc = device_create_file(dev, &nu2111a_charger_attrs[i]);
		if (rc)
			goto create_attrs_failed;
	}
	return rc;

create_attrs_failed:
	dev_err(dev, "%s: failed (%d)\n", __func__, rc);
	while (i--)
		device_remove_file(dev, &nu2111a_charger_attrs[i]);
	return rc;
}

static enum power_supply_property nu2111a_psy_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_ONLINE,
};

static const struct power_supply_desc nu2111a_psy_desc = {
	.name = "nu2111a-charger",
	.type = POWER_SUPPLY_TYPE_UNKNOWN,
	.get_property = nu2111a_psy_get_property,
	.set_property = nu2111a_psy_set_property,
	.properties = nu2111a_psy_props,
	.num_properties = ARRAY_SIZE(nu2111a_psy_props),
};

#if (KERNEL_VERSION(6, 3, 0) <= LINUX_VERSION_CODE)
static int nu2111a_charger_probe(struct i2c_client *client)
#else
static int nu2111a_charger_probe(struct i2c_client *client, const struct i2c_device_id *id)
#endif
{
	struct nu2111a_platform_data *pdata;
	struct nu2111a_charger *charger;
	struct power_supply_config psy_cfg = {};
	int ret;

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	pr_info("%s: -------NU2111A Charger Driver Loading\n", __func__);
#endif
	pr_info("[%s]:: Ver[%s]!\n", __func__, NU2111A_MODULE_VERSION);

	charger = devm_kzalloc(&client->dev, sizeof(*charger), GFP_KERNEL);
	if (!charger)
		return -ENOMEM;

#if defined(CONFIG_OF)
	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev, sizeof(struct nu2111a_platform_data), GFP_KERNEL);
		if (!pdata)
			return -ENOMEM;

		ret = nu2111a_parse_dt(&client->dev, pdata);
		if (ret < 0) {
			dev_err(&client->dev, "Failed to get device of node\n");
			return -ENOMEM;
		}

		client->dev.platform_data = pdata;
	} else {
		pdata = client->dev.platform_data;
	}
#else
	pdata = dev->platform_data;
#endif

	if (!pdata)
		return -EINVAL;

	charger->dev = &client->dev;
	charger->pdata = pdata;
	charger->client = client;
	charger->dc_state = DC_STATE_NOT_CHARGING;
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	charger->health_status = POWER_SUPPLY_HEALTH_GOOD;
#endif

	ret = nu2111a_regmap_init(charger);
	if (ret < 0) {
		dev_err(&client->dev, "Failed to initialize i2c,[%s]\n", NU2111A_I2C_NAME);
		return ret;
	}

	if (charger->pdata->sub_i2c) {
		pr_info("%s: Found sub_i2c\n", __func__);
		charger->client_sub = devm_i2c_new_dummy_device(&client->dev, charger->pdata->sub_i2c, 0x6e);

		if (!charger->client_sub) {
			dev_err(charger->dev, "Failed to register sub i2c device\n");
			return -ENODEV;
		}

		if (!i2c_check_functionality(charger->pdata->sub_i2c, I2C_FUNC_I2C)) {
			dev_err(&charger->client_sub->dev, "%s: check_functionality failed.", __func__);
			ret = -ENODEV;
		}

		charger->regmap_sub = devm_regmap_init_i2c(charger->client_sub, &nu2111a_regmap_config);
		if (IS_ERR(charger->regmap_sub)) {
			dev_err(&charger->client_sub->dev, "regmap init failed, err == %d\n", ret);
			ret = PTR_ERR(charger->regmap_sub);
		}
	}

	mutex_init(&charger->i2c_lock);
	mutex_init(&charger->lock);
	charger->monitor_ws = wakeup_source_register(&client->dev, "nu2111a-monitor");

	INIT_DELAYED_WORK(&charger->timer_work, nu2111a_timer_work);

#ifdef _NU_DBG
	INIT_DELAYED_WORK(&charger->adc_work, nu211a_adc_work);
#endif

	mutex_lock(&charger->lock);
	charger->timer_id = TIMER_ID_NONE;
	charger->timer_period = 0;
	mutex_unlock(&charger->lock);

	INIT_DELAYED_WORK(&charger->pps_work, nu2111a_pps_request_work);

#ifdef NU2111A_STEP_CHARGING
	/* step_charging schedule work*/
	INIT_DELAYED_WORK(&charger->step_work, nu2111a_step_charging_work);
#endif

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	INIT_DELAYED_WORK(&charger->wdt_control_work, nu2111a_wdt_control_work);
#endif

	init_waitqueue_head(&charger->wait_q);

	nu2111a_device_init(charger);

	psy_cfg.supplied_to = nu2111a_supplied_to;
	psy_cfg.num_supplicants = ARRAY_SIZE(nu2111a_supplied_to);
	psy_cfg.drv_data = charger;

	charger->psy_chg = power_supply_register(&client->dev, &nu2111a_psy_desc, &psy_cfg);
	if (IS_ERR(charger->psy_chg)) {
		dev_err(&client->dev, "failed to register power supply\n");
		return PTR_ERR(charger->psy_chg);
	}

	ret = nu2111a_chg_create_attrs(&charger->psy_chg->dev);
	if (ret) {
		dev_err(charger->dev, "%s : Failed to create_attrs\n", __func__);
		goto FAIL_DEBUGFS;
	}

#ifdef _NU_DBG
	schedule_delayed_work(&charger->adc_work, 0);
#endif

	ret = nu2111a_create_debugfs_entries(charger);
	if (ret < 0)
		goto FAIL_DEBUGFS;

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	sec_chg_set_dev_init(SC_DEV_DIR_CHG);

	pr_info("%s: NU2111A Charger Driver Loaded\n", __func__);
#endif

	return 0;

FAIL_DEBUGFS:
	power_supply_unregister(charger->psy_chg);
	wakeup_source_unregister(charger->monitor_ws);
	devm_kfree(&client->dev, charger);
	devm_kfree(&client->dev, pdata);
	mutex_destroy(&charger->i2c_lock);
	return ret;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
static int nu2111a_charger_remove(struct i2c_client *client)
#else
static void nu2111a_charger_remove(struct i2c_client *client)
#endif
{
	struct nu2111a_charger *chg = i2c_get_clientdata(client);
	int i;

	LOG_DBG(chg, "START!!");

	wakeup_source_unregister(chg->monitor_ws);

	for (i = 0; i < ARRAY_SIZE(nu2111a_charger_attrs); i++) {
		device_remove_file(&chg->psy_chg->dev, &nu2111a_charger_attrs[i]);
	}
	if (chg->psy_chg)
		power_supply_unregister(chg->psy_chg);

	mutex_destroy(&chg->i2c_lock);
	mutex_destroy(&chg->lock);

	debugfs_remove(chg->debug_root);
	kfree(chg);

#if LINUX_VERSION_CODE < KERNEL_VERSION(6, 1, 0)
	return 0;
#endif
}

static void nu2111a_charger_shutdown(struct i2c_client *client)
{
	struct nu2111a_charger *chg = i2c_get_clientdata(client);

	LOG_DBG(chg, "START!!");

	nu2111a_stop_charging(chg);
}

#if defined(CONFIG_PM)
static int nu2111a_charger_resume(struct device *dev)
{
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	struct nu2111a_charger *chg = dev_get_drvdata(dev);

	LOG_DBG(chg, "Start!!\r\n");
	chg->reverse_suspend = false;
#endif

	return 0;
}

static int nu2111a_charger_suspend(struct device *dev)
{
#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	struct nu2111a_charger *chg = dev_get_drvdata(dev);

	LOG_DBG(chg, "Start!!\r\n");
	chg->reverse_suspend = true;
#endif

	return 0;
}
#else
#define nu2111a_charger_suspend	NULL
#define nu2111a_charger_resume	NULL
#endif

static const struct i2c_device_id nu2111a_id_table[] = {
	{NU2111A_I2C_NAME, 0},
	{ },
};

static const struct of_device_id nu2111a_match_table[] = {
	{ .compatible = "nuvolta,nu2111a", },
	{},
};


const struct dev_pm_ops nu2111a_pm_ops = {
	.suspend = nu2111a_charger_suspend,
	.resume  = nu2111a_charger_resume,
};

static struct i2c_driver nu2111a_driver = {
	.driver = {
		.name = NU2111A_I2C_NAME,
#if defined(CONFIG_OF)
		.of_match_table = nu2111a_match_table,
#endif
#if defined(CONFIG_PM)
		.pm = &nu2111a_pm_ops,
#endif
	},
	.probe = nu2111a_charger_probe,
	.remove = nu2111a_charger_remove,
	.shutdown = nu2111a_charger_shutdown,
	.id_table = nu2111a_id_table,
};


static int __init nu2111a_charger_init(void)
{
	int err;

	err = i2c_add_driver(&nu2111a_driver);
	if (err)
		pr_err("%s: nu2111a_charger driver failed (errno = %d\n", __func__, err);
	else
		pr_info("%s: Successfully added driver %s\n", __func__, nu2111a_driver.driver.name);

	return err;
}

static void __exit nu2111a_charger_exit(void)
{
	i2c_del_driver(&nu2111a_driver);
}

module_init(nu2111a_charger_init);
module_exit(nu2111a_charger_exit);

MODULE_LICENSE("GPL");
MODULE_VERSION(NU2111A_MODULE_VERSION);
