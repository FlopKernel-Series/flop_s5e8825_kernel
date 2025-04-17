/* SPDX-License-Identifier: GPL-2.0 */

#ifndef _NU2111A_CHARGER_H_
#define _NU2111A_CHARGER_H_

//#define _NU_DBG

#define BITS(_end, _start)          ((BIT(_end) - BIT(_start)) + BIT(_end))
#define MAX(a, b)               ((a > b) ? (a):(b))
#define MIN(a, b)               ((a < b) ? (a):(b))
#define MASK2SHIFT(_mask)           __ffs(_mask)
#define I2C_RETRY_CNT 3

enum {
	TA_V_OFFSET = 0,
	TA_C_OFFSET = 1,
	IIN_C_OFFSET = 2,
	TEST_MODE = 3,
	PPS_VOL = 4,
	PPS_CUR = 5,
	VFLOAT = 6,
	IINCC = 7,
	DCERR_REASON = 8,
};

ssize_t nu2111a_chg_show_attrs(struct device *dev,
				struct device_attribute *attr, char *buf);

ssize_t nu2111a_chg_store_attrs(struct device *dev,
				struct device_attribute *attr, const char *buf, size_t count);

#define NU2111A_CHARGER_ATTR(_name)				\
{							                    \
	.attr = {.name = #_name, .mode = 0664},	    \
	.show = nu2111a_chg_show_attrs,			    \
	.store = nu2111a_chg_store_attrs,			\
}

#define REG_DEVICE_ID					    0x34

#define NU2111A_REG_VBAT_OVP					0x00
#define NU2111A_BIT_VBAT_OVP_DIS					BIT(7)
#define NU2111A_BITS_VBAT_OVP_TH					BITS(6, 0)

#define NU2111A_REG_IBAT_OCP					0x01
#define NU2111A_BIT_IBAT_OCP_DIS					BIT(7)
#define NU2111A_BITS_IBAT_OCP					BITS(6, 0)

#define NU2111A_REG_AC_PROT_VDROP			0x02
#define NU2111A_BIT_VDRP_OVP_DIS					BIT(7)
#define NU2111A_BIT_VDRP_OVP_TH_SET			BIT(6)
#define NU2111A_BIT_VAC_OVP_DIS				BIT(4)
#define NU2111A_BITS_VAC_OVP				BITS(2, 0)

#define NU2111A_REG_VBUS_OVP				0x03
#define NU2111A_BIT_VBUS_OVP_DIS				BIT(7)
#define NU2111A_BITS_VBUS_OVP_TH				BITS(6, 0)

#define NU2111A_REG_IBUS_OCP				0x04
#define NU2111A_BIT_IBUS_OCP_DIS				BIT(7)
#define NU2111A_BITS_IBUS_OCP_TH			BITS(5, 0)

#define NU2111A_REG_PMID2VOUT_OVP_UVP			0x05
#define NU2111A_BIT_PMID2VOUT_OVP_DIS			BIT(7)
#define NU2111A_BITS_PMID2VOUT_OVP			BITS(5, 4)
#define NU2111A_BIT_PMID2VOUT_UVP_DIS			BIT(3)
#define NU2111A_BITS_PMID2VOUT_UVP			BITS(1, 0)

#define NU2111A_REG_NTC_FLT				0x06
#define NU2111A_BITS_NTC_FLT				BITS(7, 0)

#define NU2111A_REG_CONTROL1				0x07
#define NU2111A_BIT_CHG_EN				BIT(7)
#define NU2111A_BIT_REG_RST				BIT(6)
#define NU2111A_BIT_FORCE_SLEEP			BIT(5)
#define NU2111A_BITS_FREQ_SHIFT			BITS(4, 3)
#define NU2111A_BITS_FSW_SET				BITS(2, 0)

#define NU2111A_REG_CONTROL2				0x08
#define NU2111A_BITS_SCC_MODE				BITS(7, 6)
#define NU2111A_BIT_RST_BY_VOUTUVLO                     BIT(5)
#define NU2111A_BITS_WDT				BITS(2, 0)

#define NU2111A_REG_CONTROL3				0x09
#define NU2111A_BITS_SS_TIMEOUT_SET                     BITS(7, 5)
#define NU2111A_BIT_VOUT_OVP_DIS			BIT(4)
#define NU2111A_BIT_VOUT_OVP_TH_SET                     BIT(3)
#define NU2111A_BIT_VBUS_PD_EN			BIT(2)
#define NU2111A_BIT_VAC_PD_EN			BIT(1)

#define NU2111A_REG_CONTROL4				0x0A
#define NU2111A_BIT_REG_TIMEOUT_DIS                     BIT(7)
#define NU2111A_BIT_PMID_PRECHG_DIS                     BIT(6)
#define NU2111A_BITS_CFLY_PRECHG_TIMEOUT                BITS(5, 4)
#define NU2111A_BIT_NTC_FLT_DIS			BIT(3)
#define NU2111A_BITS_VOUT_VALID_TH			BITS(2, 0)

#define NU2111A_REG_BAT_REG					0x0B
#define NU2111A_BIT_IBAT_REG_EN				BIT(7)
#define NU2111A_BITS_SET_IBAT_REG				BITS(5, 4)
#define NU2111A_BIT_VBAT_REG_EN				BITS(3)
#define NU2111A_BITS_SET_VBAT_REG				BITS(1, 0)

#define NU2111A_REG_IBUS_REG_UCP				0x0C
#define NU2111A_BIT_IBUS_UCP_DIS				BIT(7)
#define NU2111A_BIT_IBUS_UCP_TH					BIT(6)
#define NU2111A_BIT_IBUS_REG_EN					BIT(2)
#define NU2111A_BITS_SET_IBUS_REG				BITS(1, 0)

#define NU2111A_REG_CP_DEGLITCH					0x0D
#define NU2111A_BIT_VAC_PRNT_DEG				BIT(7)
#define NU2111A_BIT_VBUS_PRNT_DEG				BIT(6)
#define NU2111A_BIT_VBAT_OVP_DEG				BIT(4)
#define NU2111A_BIT_VDRP_OVP_DEG				BIT(3)
#define NU2111A_BIT_VBUS_VALID_TIME				BIT(2)
#define NU2111A_BITS_IBUS_UCP_FALL_DG_SET			BITS(1, 0)

#define NU2111A_REG_INT_STAT1					0x0E
#define NU2111A_BIT_VOUT_OVP_STAT				BIT(7)
#define NU2111A_BIT_VBAT_OVP_STAT				BIT(6)
#define NU2111A_BIT_IBAT_OCP_STAT				BIT(5)
#define NU2111A_BIT_VBUS_OVP_STAT				BIT(4)
#define NU2111A_BIT_IBUS_OCP_STAT				BIT(3)
#define NU2111A_BIT_IBUS_UCP_FALL_STAT				BIT(2)
#define NU2111A_BIT_ADAPTER_INSERT_STAT				BIT(1)
#define NU2111A_BIT_VBAT_INSERT_STAT				BIT(0)

#define NU2111A_REG_INT_STAT2					0x0F
#define NU2111A_BIT_TSD_STAT			BIT(7)
#define NU2111A_BIT_NTC_FLT_STAT			BIT(6)
#define NU2111A_BIT_PMID2VOUT_OVP_STAT			BIT(5)
#define NU2111A_BIT_PMID2VOUT_UVP_STAT			BIT(4)
#define NU2111A_BIT_VBUS_ERRORHI_STAT	BIT(3)
#define NU2111A_BIT_VBUS_ERRORLO_STAT	BIT(2)
#define NU2111A_BIT_VDRP_OVP_STAT		BIT(1)
#define	NU2111A_BIT_VAC_OVP_STAT		BIT(0)

#define NU2111A_REG_INT_STAT3					0x10
#define NU2111A_BIT_CP_ACTIVE_STAT	    BIT(4)
#define NU2111A_BIT_VBUS_PRESENT_STAT			BIT(3)
#define NU2111A_BIT_IBAT_REG_ACTIVE_STAT	BIT(2)
#define NU2111A_BIT_VBAT_REG_ACTIVE_STAT		BIT(1)
#define NU2111A_BIT_IBUS_REG_ACTIVE_STAT	BIT(0)
#define NU2111A_BIT_REG_ACTIVE_STAT	BITS(2, 0)

#define NU2111A_REG_INT_FLAG1					0x11
#define NU2111A_BIT_VOUT_OVP_FLAG			BIT(7)
#define NU2111A_BIT_VBAT_OVP_FLAG			BIT(6)
#define NU2111A_BIT_IBAT_OCP_FLAG			BIT(5)
#define NU2111A_BIT_VBUS_OVP_FLAG			BIT(4)
#define NU2111A_BIT_IBUS_OCP_FLAG	BIT(3)
#define NU2111A_BIT_IBUS_UCP_FALL_FLAG	BIT(2)
#define NU2111A_BIT_ADAPTER_INSERT_FLAG		BIT(1)
#define	NU2111A_BIT_VBAT_INSERT_FLAG		BIT(0)

#define NU2111A_REG_INT_FLAG2					0x12
#define NU2111A_BIT_TSD_FLAG			BIT(7)
#define NU2111A_BIT_NTC_FLT_FLAG			BIT(6)
#define NU2111A_BIT_PMID2VOUT_OVP_FLAG			BIT(5)
#define NU2111A_BIT_PMID2VOUT_UVP_FLAG			BIT(4)
#define NU2111A_BIT_VBUS_ERRORHI_FLAG	BIT(3)
#define NU2111A_BIT_VBUS_ERRORLO_FLAG	BIT(2)
#define NU2111A_BIT_VDRP_OVP_FLAG		BIT(1)
#define	NU2111A_BIT_VAC_OVP_FLAG		BIT(0)

#define NU2111A_REG_INT_FLAG3					0x13
#define NU2111A_BIT_REVERSE_PGOOD_FLAG			BIT(7)
#define NU2111A_BIT_VOUT_INVALID_REVERSE_FLAG			BIT(6)
#define NU2111A_BIT_BUS_UCP_RISE_FLAG			BIT(5)
#define NU2111A_BIT_VBUS_PRESENT_FLAG	BIT(3)
#define NU2111A_BIT_IBAT_REG_ACTIVE_FLAG	BIT(2)
#define NU2111A_BIT_VBAT_REG_ACTIVE_FLAG		BIT(1)
#define	NU2111A_BIT_IBUS_REG_ACTIVE_FLAG		BIT(0)

#define NU2111A_REG_INT_FLAG4					0x14
#define NU2111A_BIT_VOUT_PRES_FALL_FLAG			BIT(6)
#define NU2111A_BIT_VBUS_PRES_FALL_FLAG			BIT(5)
#define NU2111A_BIT_POWER_NG_FLAG			BIT(4)
#define NU2111A_BIT_WD_TIMEOUT_FLAG	BIT(3)
#define NU2111A_BIT_SS_TIMEOUT_FLAG	BIT(2)
#define NU2111A_BIT_REG_TIMEOUT_FLAG		BIT(1)
#define	NU2111A_BIT_PIN_DIAG_FAIL_FLAG		BIT(0)

#define NU2111A_REG_INT_MASK1				0x15
#define NU2111A_BIT_VOUT_OVP_MASK			BIT(7)
#define NU2111A_BIT_VBAT_OVP_MASK			BIT(6)
#define NU2111A_BIT_IBAT_OCP_MASK			BIT(5)
#define NU2111A_BIT_VBUS_OVP_MASK			BIT(4)
#define NU2111A_BIT_IBUS_OCP_MASK	BIT(3)
#define NU2111A_BIT_IBUS_UCP_FALL_MASK	BIT(2)
#define NU2111A_BIT_ADAPTER_INSERT_MASK		BIT(1)
#define	NU2111A_BIT_VBAT_INSERT_MASK		BIT(0)

#define NU2111A_REG_INT_MASK2				0x16
#define NU2111A_BIT_NTC_FLT_MASK			BIT(6)
#define NU2111A_BIT_PMID2VOUT_OVP_MSK			BIT(5)
#define NU2111A_BIT_PMID2VOUT_UVP_MSK			BIT(4)
#define NU2111A_BIT_VBUS_ERRORHI_MASK	BIT(3)
#define NU2111A_BIT_VBUS_ERRORLO_MASK	BIT(2)
#define NU2111A_BIT_VDRP_OVP_MASK		BIT(1)
#define	NU2111A_BIT_VAC_OVP_MASK		BIT(0)

#define NU2111A_REG_INT_MASK3				0x17
#define NU2111A_BIT_REVERSE_PGOOD_MASK			BIT(7)
#define NU2111A_BIT_VOUT_INVALID_REVERSE_MASK			BIT(6)
#define NU2111A_BIT_IBUS_UCP_RISE_MASK			BIT(5)
#define NU2111A_BIT_VBUS_PRESENT_MASK	BIT(3)
#define NU2111A_BIT_IBAT_REG_ACTIVE_MASK	BIT(2)
#define NU2111A_BIT_VBAT_REG_ACTIVE_MASK	BIT(1)
#define	NU2111A_BIT_IBUS_REG_ACTIVE_MASK	BIT(0)

#define NU2111A_REG_ADC_CTRL				0x18
#define NU2111A_BIT_ADC_EN			BIT(7)
#define NU2111A_BIT_ADC_RATE			BIT(6)
#define NU2111A_BIT_ADC_AVG			BIT(5)
#define NU2111A_BITS_ADC_SAMPLE_TIME	BITS(4, 3)
#define NU2111A_BIT_ADC_DONE_STAT	BIT(2)
#define NU2111A_BIT_ADC_DONE_FLAG		BIT(1)
#define	NU2111A_BIT_ADC_DONE_MASK		BIT(0)

#define NU2111A_REG_ADC_FN_DIS			0x19
#define NU2111A_BIT_NTC_ADC_DIS			BIT(7)
#define NU2111A_NTC_ADC_DIS_SHIFT			7
#define NU2111A_NTC_ADC_ENABLE			0
#define NU2111A_NTC_ADC_DISABLE		1

#define NU2111A_BIT_VBUS_ADC_DIS			BIT(6)
#define NU2111A_VBUS_ADC_DIS_SHIFT			6
#define NU2111A_VBUS_ADC_ENABLE			0
#define NU2111A_VBUS_ADC_DISABLE		1

#define NU2111A_BIT_VAC_ADC_DIS			BIT(5)
#define NU2111A_VAC_ADC_DIS_SHIFT			5
#define NU2111A_VAC_ADC_ENABLE			0
#define NU2111A_VAC_ADC_DISABLE		1

#define NU2111A_BIT_VOUT_BIT_ADC_DIS			BIT(4)
#define NU2111A_VOUT_ADC_DIS_SHIFT			4
#define NU2111A_VOUT_ADC_ENABLE			0
#define NU2111A_VOUT_ADC_DISABLE		1

#define NU2111A_BIT_VBAT_ADC_DIS			BIT(3)
#define NU2111A_VBAT_ADC_DIS_SHIFT			3
#define NU2111A_VBAT_ADC_ENABLE			0
#define NU2111A_VBAT_ADC_DISABLE		1

#define NU2111A_BIT_IBAT_ADC_DIS			BIT(2)
#define NU2111A_IBAT_ADC_DIS_SHIFT			2
#define NU2111A_IBAT_ADC_ENABLE			0
#define NU2111A_IBAT_ADC_DISABLE		1

#define NU2111A_BIT_IBUS_ADC_DIS			BIT(1)
#define NU2111A_IBUS_ADC_DIS_SHIFT			1
#define NU2111A_IBUS_ADC_ENABLE			0
#define NU2111A_IBUS_ADC_DISABLE		1

#define NU2111A_BIT_TDIE_ADC_DIS			BIT(0)
#define NU2111A_TDIE_ADC_DIS_SHIFT			0
#define NU2111A_TDIE_ADC_ENABLE			0
#define NU2111A_TDIE_ADC_DISABLE		1

#define NU2111A_REG_IBUS_ADC1				0x1A
#define NU2111A_BIT_IBUS_POLL			BIT(7)
#define NU2111A_BITS_IBUS_ADC1			BITS(4, 0)

#define NU2111A_REG_IBUS_ADC0				0x1B
#define NU2111A_BITS_IBUS_ADC0			BITS(7, 0)

#define NU2111A_REG_VBUS_ADC1				0x1C
#define NU2111A_BITS_VBUS_ADC1			BITS(6, 0)

#define NU2111A_REG_VBUS_ADC0				0x1D
#define NU2111A_BITS_VBUS_ADC0			BITS(7, 0)

#define NU2111A_REG_VAC_ADC1				0x1E
#define NU2111A_BITS_VAC_ADC1			BITS(6, 0)

#define NU2111A_REG_VAC_ADC0				0x1F
#define NU2111A_BITS_VAC_ADC0			BITS(7, 0)

#define NU2111A_REG_VOUT_ADC1				0x20
#define NU2111A_BITS_VOUT_ADC1			BITS(4, 0)

#define NU2111A_REG_VOUT_ADC0				0x21
#define NU2111A_BITS_VOUT_ADC0			BITS(7, 0)

#define NU2111A_REG_VBAT_ADC1				0x22
#define NU2111A_BITS_VBAT_ADC1			BITS(4, 0)

#define NU2111A_REG_VBAT_ADC0				0x23
#define NU2111A_BITS_VBAT_ADC0			BITS(7, 0)

#define NU2111A_REG_IBAT_ADC1				0x24
#define NU2111A_BIT_IBAT_POL			BIT(7)
#define NU2111A_BITS_IBAT_ADC1			BITS(4, 0)

#define NU2111A_REG_IBAT_ADC0				0x25
#define NU2111A_BITS_IBAT_ADC0			BITS(7, 0)

#define NU2111A_REG_TDIE_ADC1			0x26
#define NU2111A_BIT_TDIE_POL			BIT(7)
#define NU2111A_BIT_TDIE_ADC1			BIT(0)

#define NU2111A_REG_TDIE_ADC0				0x27
#define NU2111A_BITS_TDIE_ADC0			BITS(7, 0)

#define NU2111A_REG_NTC_ADC1				0x28
#define NU2111A_BITS_NTC_ADC1		BITS(1, 0)

#define NU2111A_REG_NTC_ADC0				0x28
#define NU2111A_BITS_NTC_ADC0		BITS(7, 0)

/* i2c regmap init setting */
#define NU2111A_REG_MAX         0x34
#define NU2111A_I2C_NAME "nu2111a"
#define NU2111A_MODULE_VERSION "JP7.5.9e1"

#define CONFIG_PASS_THROUGH

/* Input Current Limit Default Setting */
#define NU2111A_IIN_CFG_DFT                  2000000         // 2A
#define NU2111A_IIN_CFG_MIN                  500000          // 500mA
#define NU2111A_IIN_CFG_MAX                  5000000         // 5A

/* Charging Current Setting */
#define NU2111A_ICHG_CFG_DFT                 6000000         // 6A
#define NU2111A_ICHG_CFG_MIN                 0
#define NU2111A_ICHG_CFG_MAX                 8000000         // 8A

/*VBAT REG Default Setting */
#define NU2111A_VBAT_REG_DFT                 4450000         // 4.34V
#define NU2111A_VBAT_REG_MIN                 3725000         // 3.725V

#define NU2111A_IIN_DONE_DFT                 500000          // 500mA

#define POWL				     103

#define NU2111A_IIN_CFG_STEP                 50000           //50mA

#ifdef CONFIG_SEC_FACTORY
#define NU2111A_TA_VOL_STEP_ADJ_CC           80000           //80mV
#define NU2111A_TA_VOL_STEP_ENTER_CC         20000           //20mV
#else
#define NU2111A_TA_VOL_STEP_ADJ_CC           40000           //40mV
#define NU2111A_TA_VOL_STEP_ENTER_CC         40000           //40mV
#endif

#define FPDO_DONE_CNT	3
#define NU2111A_SEC_FPDO_DC_IV		9	// 9V
#define NU2111A_TA_VOL_MIN_DCERR           9200           //9200mV

//IIN_REG_TH (A) = 1A + DEC (5:0) * 100mA
#define NU2111A_IIN_TO_HEX(__iin)            ((__iin - 500000)/100000)

//VBAT_REG_TH (V) = 3.73V + DEC (6:0) * 10mV
#define NU2111A_VBAT_REG(_vbat_reg)          ((_vbat_reg+210000-3730000)/10000)
#define NU2111A_IIN_OFFSET_CUR               700000 //700mA 700-2.8  1.1-3.2

/* pre cc mode ta-vol step*/
#define NU2111A_TA_VOL_STEP_PRE_CC           100000  //100mV
#define NU2111A_TA_VOL_STEP_PRE_CV           20000   //20mV

#define NU2111A_TA_V_OFFSET                  200000 //100mV
#define NU2111A_TA_C_OFFSET                  200000 //200mA
#define NU2111A_TA_C_PRE_OFFSET              100000 //50mA

#define NU2111A_PARALLEL_TOPOFF				 1000000 //1A


/* TA TOP-OFF CURRENT */
#define NU2111A_TA_TOPOFF_CUR                500000 //500mA
/* PD Message Voltage and Current Step */
#define PD_MSG_TA_VOL_STEP_DIV		    10000 //10mV
#define PD_MSG_TA_VOL_STEP                  20000 //20mV
#define PD_MSG_TA_CUR_STEP                  50000 //50mA

/* Maximum TA voltage threshold */
#define NU2111A_TA_MAX_VOL                   11000000//9900000uV
/* Maximum TA current threshold */
#define NU2111A_TA_MAX_CUR                   3100000 // 2450000uA
/* Mimimum TA current threshold */
#define NU2111A_TA_MIN_CUR                   1000000 // 1000000uA - PPS minimum current
/* Minimum TA voltage threshold in Preset mode */

#define NU2111A_TA_MIN_VOL_PRESET            6500000 //6.5V

#define NU2111A_TA_25W            25000000 //25W

/* Preset TA VOLtage offset*/
#define NU2111A_TA_VOL_PRE_OFFSET            300000  // 300mV

/* CCMODE Charging Current offset */
#define NU2111A_IIN_CFG_RANGE_CC            100000   //100mA
#define NU2111A_IIN_CFG_OFFSET_CC            50000   //50mA
/* Denominator for TA Voltage and Current setting*/
#define NU2111A_SEC_DENOM_U_M                1000 // 1000, denominator
/* PPS request Delay Time */
#define NU2111A_PPS_PERIODIC_T	            7500	// 7500ms

/* VIN STEP for Register setting */
#define NU2111A_VIN_STEP                     15000   //15mV (0 ~ 15.345V)
/* IIN STEP for Register setting */
#define NU2111A_IIN_STEP                     4400// 4.4mA (from 0A to 4.5A in 4.4mA LSB)
/* VBAT STEP for register setting */
#define NU2111A_VBAT_STEP                    5000    //5mV (0 ~ 5.115V)
/* VTS STEP for register setting */
#define NU2111A_VTS_STEP                     1760    //1.76mV (0 ~ 1.8V)
/* TDIE STEP for register setting */
#define NU2111A_TDIE_STEP                    25    // 0.25degreeC (0 ~ 125)
#define NU2111A_TDIE_DENOM                   10// 10, denomitor
/* VOUT STEP for register setting */
#define NU2111A_VOUT_STEP					5000//5mV (0 ~ 5.115V)

/* Workqueue delay time for VBATMIN */
#ifdef CONFIG_SEC_FACTORY
#define NU2111A_VBATMIN_CHECK_T              0
#else
#define NU2111A_VBATMIN_CHECK_T              1000
#endif
/* Workqueue delay time for CHECK ACTIVE */
#define NU2111A_CHECK_ACTIVE_DELAY_T         200// 200ms/150ms
/* Workqueue delay time for CCMODE */
#define NU2111A_CCMODE_CHECK_T	            2000// 2000ms
/* Workqueue delay time for CVMODE */
#define NU2111A_CVMODE_CHECK_T	            4000// 4000ms
#define NU2111A_CVMODE_CHECK_T_2	    1000// 4000ms


/* Delay Time after PDMSG */
#define NU2111A_PDMSG_WAIT_T	                200// 200ms

/* Delay Time for WDT */
#define NU2111A_BATT_WDT_CONTROL_T           30000//30S
#define NU2111A_BATT_WDT_WAIT_5_T           5100//30S
#define NU2111A_BATT_WDT_WAIT_30_T           30100//30S

#define REVERSE_WAIT_T	250
#define REVERSE_CHECK_T	10000	// Reverse mode polling timer - 10000ms

/* Battery Threshold*/
#define NU2111A_DC_VBAT_MIN                  340000//3.4V

/* VBUS OVP Threshold */
#define NU2111A_VBUS_OVP_TH                  8000//8000mV

/* LOG BUFFER for RPI */
#define LOG_BUFFER_ENTRIES	                1024
#define LOG_BUFFER_ENTRY_SIZE	            128

#ifdef CONFIG_PASS_THROUGH
/* Default IIN default in pass through mode */
#define NU2111A_PT_IIN_DFT                   3000000 // 3A
/* Preset TA VOLtage offset in pass through mode */
#define NU2111A_PT_VOL_PRE_OFFSET            200000  // 200mV
/* Preset TA Current offset in pass through mode */
#define NU2111A_PT_TA_CUR_LOW_OFFSET         200000  // 200mA
/*  Vbat regulation Volatage in pass through mode */
#define NU2111A_PT_VBAT_REG                  4400000 //4.4V
/* Workqueue delay time for entering pass through mode */
#define NU2111A_PT_ACTIVE_DELAY_T            200     // 200ms
/* Workqueue delay time for pass through mode */
#define NU2111A_PTMODE_DELAY_T               10000	// 10000ms
/* Workqueu Delay time for unpluged detection issue in pass through mode */
#define NU2111A_PTMODE_UNPLUGED_DETECTION_T	1000	// 1000ms
#endif

/*STEP CHARGING SETTING */
//#define NU2111A_STEP_CHARGING
#ifdef NU2111A_STEP_CHARGING
#define NU2111A_STEP_CHARGING_CNT            3
#define NU2111A_STEP1_TARGET_IIN             2180000//2000mA
#define NU2111A_STEP2_TARGET_IIN             166500//1575mA
#define NU2111A_STEP3_TARGET_IIN             1325000//1250mA

#define NU2111A_STEP1_TOPOFF                 NU2111A_STEP2_TARGET_IIN//1575mA
#define NU2111A_STEP2_TOPOFF                 NU2111A_STEP3_TARGET_IIN//1250mA
#define NU2111A_STEP3_TOPOFF                 NU2111A_TA_TOPOFF_CUR//500mA

#define NU2111A_STEP1_VBAT_REG               4160000//4120000 //4.12V 4.4-200mV
#define NU2111A_STEP2_VBAT_REG               4290000//4280000 //4.28V 4.4-100mV
#define NU2111A_STEP3_VBAT_REG               4420000//4400000 //4.4V  4.4-50mV

enum CHARGING_STEP {
	STEP_DIS = 0,
	STEP_ONE = 1,
	STEP_TWO = 2,
	STEP_THREE = 3,
};
#endif

enum {
	ENUM_INT,
	ENUM_INT_MASK,
	ENUM_INT_STS_A,
	ENUM_INT_STS_B,
	ENUM_INT_MAX,
};

/* Switching Frequency */
enum {
	FSW_CFG_300KHZ = 0,
	FSW_CFG_400KHZ,
	FSW_CFG_500KHZ,
	FSW_CFG_615KHZ,
	FSW_CFG_730KHZ,
	FSW_CFG_800KHZ,
	FSW_CFG_890KHZ,
	FSW_CFG_1000KHZ,
};

/* Watchdong Timer */
enum {
	WDT_DISABLED = 0,
	WDT_ENABLED,
};

enum {
	WD_TMR_Off = 0,
	WD_TMR_200ms,
	WD_TMR_500ms,
	WD_TMR_1S,
	WD_TMR_5S,
	WD_TMR_30S,
};

/* Increment check for Adjust MODE */
enum {
	INC_NONE,
	INC_TA_VOL,
	INC_TA_VOL2,
	INC_TA_CUR,
};

/* TIMER ID for Charging Mode */
enum {
	TIMER_ID_NONE = 0,
	TIMER_VBATMIN_CHECK,
	TIMER_PRESET_DC,
	TIMER_PRESET_CONFIG,
	TIMER_CHECK_ACTIVE,
	TIMER_ADJUST_CCMODE,
	TIMER_ENTER_CCMODE,
	TIMER_CHECK_CCMODE,
	TIMER_CHECK_CVMODE,
	TIMER_PDMSG_SEND,

	TIMER_ADJUST_TAVOL,
	TIMER_ADJUST_TACUR,
#ifdef CONFIG_PASS_THROUGH
	TIMER_PTMODE_CHANGE,
	TIMER_CHECK_PTMODE,
#endif
	TIMER_START_REVERSE,
	TIMER_CHECK_REVERSE_ACTIVE,
	TIMER_CHECK_REVERSE_MODE,
	TIMER_CHECK_FPDOCVMODE,
};

/* DC STATE for matching Timer ID*/
enum {
	DC_STATE_NOT_CHARGING = 0,
	DC_STATE_CHECK_VBAT,
	DC_STATE_PRESET_DC,
	DC_STATE_CHECK_ACTIVE,
	DC_STATE_ADJUST_CC,
	DC_STATE_START_CC,
	DC_STATE_CC_MODE,
	DC_STATE_CV_MODE,
	DC_STATE_CHARGING_DONE,
	DC_STATE_ADJUST_TAVOL,
	DC_STATE_ADJUST_TACUR,
#ifdef CONFIG_PASS_THROUGH
	DC_STATE_PT_MODE,
	DC_STATE_PTMODE_CHANGE,
#endif
	DC_STATE_REVERSE_MODE,
	DC_STATE_FPDO_CV_MODE,
};

/* ADC channel 1 */
enum {
	ADC_TS = 1,
	ADC_VBUS = 2,
	ADC_VAC,
	ADC_VOUT,
	ADC_VBAT,
	ADC_IBAT,
	ADC_IBUS,
	ADC_TDIE,
	ADC_MAX_NUM,
};

/* ADC Channel 2 */
enum {
	ADCCH_VIN,
	ADCCH_IIN,
	ADCCH_VBAT,
	ADCCH_IBAT,
	ADCCH_NTC,
	ADCCH_VOUT,
	ADCCH_TDIE,
};

/* PD Message Type */
enum {
	PD_MSG_REQUEST_APDO,
	PD_MSG_REQUEST_FIXED_PDO,
};

/* Regulation Loop  */
enum {
	INACTIVE_LOOP   = 0,
	IIN_REG_LOOP     = 1,
	VBAT_REG_LOOP    = 2,
	IBAT_REG_LOOP    = 4,
	T_DIE_REG_LOOP   = 128,
};

/*DEV STATE */
enum {
	DEV_STATE_NOT_ACTIVE = 0,
	DEV_STATE_ACTIVE,
	DEV_STATE_WAIT_SUB,
};

/* TA Type for the direct charging */
enum {
	TA_TYPE_UNKNOWN,
	TA_TYPE_PD_APDO,
	TA_TYPE_PD_FPDO
};

/* DCERR REASON */
enum {
	VIN_UVLO = 0x400001,
	VIN_OVP = 0x400002,
	VOUT_OVP = 0x400003,
	VOUT_UVLO = 0x400004,
	VBAT_OVP = 0x400005,
	IBUS_OCP = 0x400006,
	IBUS_UCP = 0x400007,
	IBAT_OCP = 0x400008,
	THERMAL_SHUTDOWN = 0x40001C,
	WDT_TIMER = 0x40001D,
	NTC_FLT = 0x40001F,
	MID2VOUT_UVP = 0x400022,
	MID2VOUT_OVP = 0x400023,
	VDSQRB_OVP = 0x400024,
	SS_TIMEOUT = 0x400028,
	PIN_DIAG_FAIL = 0x400029,
	POWER_NG = 0x40002A,
	VBUS_ERR_HI = 0x400036,
	VBUS_ERR_LOW = 0x400037,
};

/*REGMAP CONFIG */
static const struct regmap_config nu2111a_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = NU2111A_REG_MAX,
};

/* Power Supply Name */
static char *nu2111a_supplied_to[] = {
	"nu2111a-charger",
};

/*PLATFORM DATA */
struct nu2111a_platform_data {
	unsigned int iin_cfg;
	unsigned int ichfg_cfg;
	unsigned int vbat_reg;
	unsigned int iin_topoff;
	unsigned int vbat_reg_max;
	unsigned int ntc_th;
	unsigned int wd_tmr;
	unsigned int sw_freq;
	bool wd_dis;
	unsigned int sub_reg;
	struct i2c_adapter *sub_i2c;
	struct device_node *np_sub;

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	unsigned int fpdo_dc_iin_topoff;
	unsigned int fpdo_dc_vnow_topoff;
	unsigned int fpdo_dc_iin_lowest_limit;

	char *sec_dc_name;
	char *fg_name;
	bool use_dc_vbat;
	unsigned int step1_vfloat;
#endif
#ifdef CONFIG_PASS_THROUGH
	int pass_through_mode;
#endif
};

/* DEV - CHIP DATA*/
struct nu2111a_charger {
	struct regmap                   *regmap;
	struct regmap                   *regmap_sub;
	struct device                   *dev;
	struct power_supply             *psy_chg;
	struct mutex                    i2c_lock;
	struct mutex                    lock;
	struct wakeup_source            *monitor_ws;
	struct nu2111a_platform_data     *pdata;
	struct i2c_client               *client;
	struct i2c_client				*client_sub;
	struct delayed_work             timer_work;
	struct delayed_work             pps_work;
	wait_queue_head_t wait_q;
#ifdef _NU_DBG
	struct delayed_work             adc_work;
#endif

#ifdef NU2111A_STEP_CHARGING
	struct delayed_work             step_work;
	unsigned int current_step;
#endif

	struct dentry                   *debug_root;

	u32  debug_address;

	unsigned int chg_state;
	unsigned int chg_state_sub;
	unsigned int reg_state;

	unsigned int charging_state;
	unsigned int dc_state;
	unsigned int timer_id;
	unsigned long timer_period;

	/* ADC CON */
	int adc_vin;
	int adc_iin;
	int adc_vbat;
	int adc_ibat;
	int adc_ntc;
	int adc_tdie;
	int adc_vin_sub;
	int adc_iin_sub;
	int adc_vbat_sub;
	int adc_ibat_sub;
	int adc_ntc_sub;
	int adc_tdie_sub;
	int adc_iin_main;

	unsigned int iin_cc;
	unsigned int ta_cur;
	unsigned int ta_vol;
	unsigned int ta_objpos;
	unsigned int ta_target_vol;
	unsigned int ta_adjust_vol;
	unsigned int ta_v_ofs;
	unsigned int ta_v_ofs_thermal;
	unsigned int ta_v_ofs_normal;
	unsigned int ta_v_ofs_max;
	unsigned int vbus_ovp_th;
	unsigned int ta_v_offset;
	unsigned int ta_c_offset;
	unsigned int iin_c_offset;
	unsigned int iin_high_count;
	unsigned int test_mode;
	unsigned int pps_vol;
	unsigned int pps_cur;

	unsigned int new_vbat_reg;
	unsigned int new_iin;
	bool         req_new_vbat_reg;
	bool         req_new_iin;

	unsigned int ta_max_cur;
	unsigned int ta_max_vol;
	unsigned int ta_max_pwr;

	unsigned int ret_state;

	unsigned int prev_iin;
	unsigned int prev_inc;
	bool         prev_dec;
	bool         low_ta;
	bool         adc_read;
	unsigned int adj_max_vol;
	unsigned int adj_max_iin;

	unsigned int pdo_index;
	unsigned int pdo_max_voltage;
	unsigned int pdo_max_current;

	bool online;
	int ta_type;
	int retry_cnt;
	int active_retry_cnt;
	int dcerr_ta_max_vol;
	int dcerr_retry_cnt;
	unsigned int dcerr_reason;
	int wdt_test_resume;
	bool sub_topoff;
	int iin_unbalance;
	int rev_mode;
	int iin_rev;
	bool reverse_suspend;

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	bool wdt_kick;
	struct delayed_work wdt_control_work;
#endif
	/* flag for abnormal TA conecction */
	bool ab_ta_connected;
	unsigned int ab_try_cnt;
	unsigned int ab_prev_cur;
	unsigned int fault_sts_cnt;
	unsigned int tp_set;

#ifdef CONFIG_DEBUG_FS
	struct dentry *dentry;
	/* lock for log buffer access */
	struct mutex logbuffer_lock;
	int logbuffer_head;
	int logbuffer_tail;
	u8 *logbuffer[LOG_BUFFER_ENTRIES];
#endif

#if IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
	unsigned int fpdo_dc_iin_topoff;
	unsigned int fpdo_dc_vnow_topoff;
	int input_current;
	int float_voltage;
	int health_status;
#endif
#ifdef CONFIG_PASS_THROUGH
	int pass_through_mode;
	bool req_pt_mode;
#endif
};

#if !IS_ENABLED(CONFIG_BATTERY_SAMSUNG)
extern int max77705_get_apdo_max_power(unsigned int *pdo_pos,
				unsigned int *taMaxVol, unsigned int *taMaxCur, unsigned int *taMaxPwr);
extern int max77705_select_pps(int num, int ppsVol, int ppsCur);
#endif
#endif /* _NU2111A_CHARGER_H_ */
