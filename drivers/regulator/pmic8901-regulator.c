/* Copyright (c) 2010, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <linux/err.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mfd/pmic8901.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/pmic8901-regulator.h>

/* Regulator types */
#define REGULATOR_TYPE_LDO		0
#define REGULATOR_TYPE_SMPS		1
#define REGULATOR_TYPE_VS		2

/* Bank select/write macros */
#define REGULATOR_BANK_SEL(n)           ((n) << 4)
#define REGULATOR_BANK_WRITE            0x80
#define LDO_TEST_BANKS			7

/* Pin mask resource register prgramming */
#define REGULATOR_PMR_STATE_MASK	0x60
#define REGULATOR_PMR_STATE_HPM		0x60
#define REGULATOR_PMR_STATE_LPM		0x40
#define REGULATOR_PMR_STATE_OFF		0x20

#define REGULATOR_IS_EN(pmr_reg) \
	((pmr_reg & REGULATOR_PMR_STATE_MASK) == REGULATOR_PMR_STATE_HPM || \
	 (pmr_reg & REGULATOR_PMR_STATE_MASK) == REGULATOR_PMR_STATE_LPM)

/* FTSMPS programming */
#define SMPS_VCTRL_BAND_MASK		0xC0
#define SMPS_VCTRL_BAND_OFF		0x00
#define SMPS_VCTRL_BAND_1		0x40
#define SMPS_VCTRL_BAND_2		0x80
#define SMPS_VCTRL_BAND_3		0xC0
#define SMPS_VCTRL_VPROG_MASK		0x3F

#define SMPS_BAND_1_UV_MIN		350000
#define SMPS_BAND_1_UV_MAX		650000
#define SMPS_BAND_1_UV_STEP		6250

#define SMPS_BAND_2_UV_MIN		700000
#define SMPS_BAND_2_UV_MAX		1487500
#define SMPS_BAND_2_UV_STEP		12500

#define SMPS_BAND_3_UV_MIN		1400000
#define SMPS_BAND_3_UV_MAX		3300000
#define SMPS_BAND_3_UV_STEP		50000

#define SMPS_UV_MIN			SMPS_BAND_1_UV_MIN
#define SMPS_UV_MAX			SMPS_BAND_3_UV_MAX

/* LDO programming */
#define LDO_CTRL_VPROG_MASK		0x1F

/* bank 2 */
#define LDO_TEST_VREF_UPDATE_MASK	0x08
#define LDO_TEST_VREF_MASK		0x04
#define LDO_TEST_FINE_STEP_MASK		0x02

/* bank 4 */
#define LDO_TEST_RANGE_EXTN_MASK	0x01

#define PLDO_LOW_UV_MIN			750000
#define PLDO_LOW_UV_MAX			1525000
#define PLDO_LOW_UV_STEP		25000
#define PLDO_LOW_FINE_STEP_UV		12500

#define PLDO_NORM_UV_MIN		1500000
#define PLDO_NORM_UV_MAX		3050000
#define PLDO_NORM_UV_STEP		50000
#define PLDO_NORM_FINE_STEP_UV		25000

#define PLDO_HIGH_UV_MIN		1750000
#define PLDO_HIGH_UV_MAX		3250000
#define PLDO_HIGH_UV_STEP		100000
#define PLDO_HIGH_FINE_STEP_UV		50000

#define NLDO_UV_MIN			750000
#define NLDO_UV_MAX			1525000
#define NLDO_UV_STEP			25000
#define NLDO_FINE_STEP_UV		12500

/* VS programming */
#define VS_CTRL_ENABLE_MASK		0xC0
#define VS_CTRL_ENABLE			0xC0

struct pm8901_vreg {
	u16			ctrl_addr;
	u16			pmr_addr;
	u16			test_addr;
	u8			type;
	u8			ctrl_reg;
	u8			pmr_reg;
	u8			test_reg[LDO_TEST_BANKS];
	u8			is_nmos;
	struct regulator_dev	*rdev;
};

#define LDO(_id, _ctrl_addr, _pmr_addr, _test_addr, _is_nmos) \
	[_id] = { \
		.ctrl_addr = _ctrl_addr, \
		.pmr_addr = _pmr_addr, \
		.test_addr = _test_addr, \
		.type = REGULATOR_TYPE_LDO, \
		.is_nmos = _is_nmos, \
	}

#define SMPS(_id, _ctrl_addr, _pmr_addr) \
	[_id] = { \
		.ctrl_addr = _ctrl_addr, \
		.pmr_addr = _pmr_addr, \
		.type = REGULATOR_TYPE_SMPS, \
	}

#define VS(_id, _ctrl_addr, _pmr_addr) \
	[_id] = { \
		.ctrl_addr = _ctrl_addr, \
		.pmr_addr = _pmr_addr, \
		.type = REGULATOR_TYPE_VS, \
	}

static struct pm8901_vreg pm8901_vreg[] = {
	/*  id                 ctrl   pmr    tst    n/p */
	LDO(PM8901_VREG_ID_L0, 0x02F, 0x0AB, 0x030, 1),
	LDO(PM8901_VREG_ID_L1, 0x031, 0x0AC, 0x032, 0),
	LDO(PM8901_VREG_ID_L2, 0x033, 0x0AD, 0x034, 0),
	LDO(PM8901_VREG_ID_L3, 0x035, 0x0AE, 0x036, 0),
	LDO(PM8901_VREG_ID_L4, 0x037, 0x0AF, 0x038, 0),
	LDO(PM8901_VREG_ID_L5, 0x039, 0x0B0, 0x03A, 0),
	LDO(PM8901_VREG_ID_L6, 0x03B, 0x0B1, 0x03C, 0),

	/*   id                 ctrl   pmr */
	SMPS(PM8901_VREG_ID_S0, 0x05B, 0x0A6),
	SMPS(PM8901_VREG_ID_S1, 0x06A, 0x0A7),
	SMPS(PM8901_VREG_ID_S2, 0x079, 0x0A8),
	SMPS(PM8901_VREG_ID_S3, 0x088, 0x0A9),
	SMPS(PM8901_VREG_ID_S4, 0x097, 0x0AA),

	/* id                       ctrl   pmr */
	VS(PM8901_VREG_ID_LVS0,     0x046, 0x0B2),
	VS(PM8901_VREG_ID_LVS1,     0x048, 0x0B3),
	VS(PM8901_VREG_ID_LVS2,     0x04A, 0x0B4),
	VS(PM8901_VREG_ID_LVS3,     0x04C, 0x0B5),
	VS(PM8901_VREG_ID_MVS0,     0x052, 0x0B6),
	VS(PM8901_VREG_ID_USB_OTG,  0x055, 0x0B7),
	VS(PM8901_VREG_ID_HDMI_MVS, 0x058, 0x0B8),
};

static int pm8901_vreg_write(struct pm8901_chip *chip,
		u16 addr, u8 val, u8 mask, u8 *reg_save)
{
	int rc;
	u8 reg;

	reg = (*reg_save & ~mask) | (val & mask);
	rc = pm8901_write(chip, addr, &reg, 1);
	if (!rc)
		*reg_save = reg;
	return rc;
}

static int pm8901_vreg_enable(struct regulator_dev *dev)
{
	struct pm8901_vreg *vreg = rdev_get_drvdata(dev);
	struct pm8901_chip *chip = dev_get_drvdata(dev->dev.parent);
	int rc;

	rc = pm8901_vreg_write(chip, vreg->pmr_addr,
			REGULATOR_PMR_STATE_HPM,
			REGULATOR_PMR_STATE_MASK,
			&vreg->pmr_reg);
	if (rc)
		pr_err("%s: pm8901_write failed\n", __func__);
	return rc;
}

static int pm8901_vreg_disable(struct regulator_dev *dev)
{
	struct pm8901_vreg *vreg = rdev_get_drvdata(dev);
	struct pm8901_chip *chip = dev_get_drvdata(dev->dev.parent);
	int rc;

	rc = pm8901_vreg_write(chip, vreg->pmr_addr,
			REGULATOR_PMR_STATE_OFF,
			REGULATOR_PMR_STATE_MASK,
			&vreg->pmr_reg);
	if (rc)
		pr_err("%s: pm8901_write failed\n", __func__);

	return rc;
}

static int pm8901_vreg_is_enabled(struct regulator_dev *dev)
{
	struct pm8901_vreg *vreg = rdev_get_drvdata(dev);
	return REGULATOR_IS_EN(vreg->pmr_reg);
}

static int pm8901_pldo_set_voltage(struct pm8901_chip *chip,
		struct pm8901_vreg *vreg, int uV)
{
	int min, max, step, fine_step, rc;
	u8 range_extn, vref, mask, val = 0;

	if (uV >= PLDO_LOW_UV_MIN &&
			uV <= PLDO_LOW_UV_MAX + PLDO_LOW_UV_STEP) {
		min = PLDO_LOW_UV_MIN;
		max = PLDO_LOW_UV_MAX;
		step = PLDO_LOW_UV_STEP;
		fine_step = PLDO_LOW_FINE_STEP_UV;
		range_extn = 0;
		vref = LDO_TEST_VREF_MASK;
	} else if (uV >= PLDO_NORM_UV_MIN &&
			uV <= PLDO_NORM_UV_MAX + PLDO_NORM_UV_STEP) {
		min = PLDO_NORM_UV_MIN;
		max = PLDO_NORM_UV_MAX;
		step = PLDO_NORM_UV_STEP;
		fine_step = PLDO_NORM_FINE_STEP_UV;
		range_extn = 0;
		vref = 0;
	} else {
		min = PLDO_HIGH_UV_MIN;
		max = PLDO_HIGH_UV_MAX;
		step = PLDO_HIGH_UV_STEP;
		fine_step = PLDO_HIGH_FINE_STEP_UV;
		range_extn = LDO_TEST_RANGE_EXTN_MASK;
		vref = 0;
	}

	if (uV > max) {
		uV -= fine_step;
		val = LDO_TEST_FINE_STEP_MASK;
	}

	/* update reference voltage and fine step selection if necessary */
	if ((vreg->test_reg[2] & LDO_TEST_FINE_STEP_MASK) != val ||
			(vreg->test_reg[2] & LDO_TEST_VREF_MASK) != vref) {
		val |= REGULATOR_BANK_SEL(2) | REGULATOR_BANK_WRITE |
			LDO_TEST_VREF_UPDATE_MASK | vref;
		mask = LDO_TEST_VREF_MASK | LDO_TEST_FINE_STEP_MASK | val;

		rc = pm8901_vreg_write(chip, vreg->test_addr, val,
				mask, &vreg->test_reg[2]);
		if (rc) {
			pr_err("%s: pm8901_write failed\n", __func__);
			return rc;
		}
	}

	/* update range extension if necessary */
	if ((vreg->test_reg[4] & LDO_TEST_RANGE_EXTN_MASK) != range_extn) {
		val = REGULATOR_BANK_SEL(4) | REGULATOR_BANK_WRITE |
			range_extn;
		mask = LDO_TEST_RANGE_EXTN_MASK | val;

		rc = pm8901_vreg_write(chip, vreg->test_addr, val,
				mask, &vreg->test_reg[4]);
		if (rc) {
			pr_err("%s: pm8901_write failed\n", __func__);
			return rc;
		}
	}

	/* voltage programming */
	val = (uV - min) / step;
	rc = pm8901_vreg_write(chip, vreg->ctrl_addr, val,
			LDO_CTRL_VPROG_MASK, &vreg->ctrl_reg);
	if (rc)
		pr_err("%s: pm8901_write failed\n", __func__);

	return rc;
}

static int pm8901_nldo_set_voltage(struct pm8901_chip *chip,
		struct pm8901_vreg *vreg, int uV)
{
	int rc;
	u8 mask, val = 0;

	if (uV > NLDO_UV_MAX) {
		uV -= NLDO_FINE_STEP_UV;
		val = LDO_TEST_FINE_STEP_MASK;
	}

	/* update reference voltage and fine step selection if necessary */
	if ((vreg->test_reg[2] & LDO_TEST_FINE_STEP_MASK) != val) {
		val |= REGULATOR_BANK_SEL(2) | REGULATOR_BANK_WRITE;
		mask = LDO_TEST_FINE_STEP_MASK | val;

		rc = pm8901_vreg_write(chip, vreg->test_addr, val,
				mask, &vreg->test_reg[2]);
		if (rc) {
			pr_err("%s: pm8901_write failed\n", __func__);
			return rc;
		}
	}

	/* voltage programming */
	val = (uV - NLDO_UV_MIN) / NLDO_UV_STEP;
	rc = pm8901_vreg_write(chip, vreg->ctrl_addr, val,
			LDO_CTRL_VPROG_MASK, &vreg->ctrl_reg);
	if (rc)
		pr_err("%s: pm8901_write failed\n", __func__);

	return rc;
}

static int pm8901_ldo_set_voltage(struct regulator_dev *dev,
		int min_uV, int max_uV)
{
	struct pm8901_vreg *vreg = rdev_get_drvdata(dev);
	struct pm8901_chip *chip = dev_get_drvdata(dev->dev.parent);
	int uV = (min_uV + max_uV) / 2;

	if (vreg->is_nmos)
		return pm8901_nldo_set_voltage(chip, vreg, uV);
	else
		return pm8901_pldo_set_voltage(chip, vreg, uV);
}

static int pm8901_pldo_get_voltage(struct pm8901_vreg *vreg)
{
	int uV, min, max, step, fine_step;
	u8 range_extn, vref, vprog, fine_step_sel;

	fine_step_sel = vreg->test_reg[2] & LDO_TEST_FINE_STEP_MASK;
	vref = vreg->test_reg[2] & LDO_TEST_VREF_MASK;
	range_extn = vreg->test_reg[4] & LDO_TEST_RANGE_EXTN_MASK;
	vprog = vreg->ctrl_reg & LDO_CTRL_VPROG_MASK;

	if (vref) {
		/* low range mode */
		fine_step = PLDO_LOW_FINE_STEP_UV;
		min = PLDO_LOW_UV_MIN;
		max = PLDO_LOW_UV_MAX;
		step = PLDO_LOW_UV_STEP;
	} else if (!range_extn) {
		/* normal mode */
		fine_step = PLDO_NORM_FINE_STEP_UV;
		min = PLDO_NORM_UV_MIN;
		max = PLDO_NORM_UV_MAX;
		step = PLDO_NORM_UV_STEP;
	} else {
		/* high range mode */
		fine_step = PLDO_HIGH_FINE_STEP_UV;
		min = PLDO_HIGH_UV_MIN;
		max = PLDO_HIGH_UV_MAX;
		step = PLDO_HIGH_UV_STEP;
	}

	uV = step * vprog + min;
	if (fine_step_sel)
		uV += fine_step;

	return uV;
}

static int pm8901_nldo_get_voltage(struct pm8901_vreg *vreg)
{
	int uV;
	u8 vprog, fine_step;

	fine_step = vreg->test_reg[2] & LDO_TEST_FINE_STEP_MASK;
	vprog = vreg->ctrl_reg & LDO_CTRL_VPROG_MASK;

	uV = NLDO_UV_STEP * vprog + NLDO_UV_MIN;
	if (fine_step)
		uV += NLDO_FINE_STEP_UV;

	return uV;
}

static int pm8901_ldo_get_voltage(struct regulator_dev *dev)
{
	struct pm8901_vreg *vreg = rdev_get_drvdata(dev);

	if (vreg->is_nmos)
		return pm8901_nldo_get_voltage(vreg);
	else
		return pm8901_pldo_get_voltage(vreg);
}

static int pm8901_vreg_set_mode(struct regulator_dev *dev, unsigned int mode)
{
	struct pm8901_vreg *vreg = rdev_get_drvdata(dev);
	struct pm8901_chip *chip = dev_get_drvdata(dev->dev.parent);
	int rc;
	u8 val;

	if (mode == REGULATOR_MODE_NORMAL) {
		/* high power mode */
		val = REGULATOR_PMR_STATE_HPM;
	} else if (mode == REGULATOR_MODE_STANDBY) {
		/* low power mode */
		val = REGULATOR_PMR_STATE_LPM;
	} else {
		return -EINVAL;
	}

	rc = pm8901_vreg_write(chip, vreg->pmr_addr,
			val, REGULATOR_PMR_STATE_MASK,
			&vreg->pmr_reg);
	if (rc)
		pr_err("%s: pm8901_vreg_write failed\n", __func__);

	return rc;
}

static unsigned int pm8901_vreg_get_mode(struct regulator_dev *dev)
{
	struct pm8901_vreg *vreg = rdev_get_drvdata(dev);
	u8 mode = vreg->pmr_reg & REGULATOR_PMR_STATE_MASK;

	if (mode == REGULATOR_PMR_STATE_HPM) {
		return REGULATOR_MODE_NORMAL;
	} else if (mode == REGULATOR_PMR_STATE_LPM) {
		return REGULATOR_MODE_STANDBY;
	} else {
		pr_err("%s: unexpected mode 0x%x\n",
				__func__, mode);
		return (unsigned int) -EINVAL;
	}
}

static int pm8901_smps_set_voltage(struct regulator_dev *dev,
		int min_uV, int max_uV)
{
	struct pm8901_vreg *vreg = rdev_get_drvdata(dev);
	struct pm8901_chip *chip = dev_get_drvdata(dev->dev.parent);
	int uV = (min_uV + max_uV) / 2;
	int rc;
	u8 val, band;

	if (uV < SMPS_BAND_2_UV_MIN) {
		val = ((uV - SMPS_BAND_1_UV_MIN) / SMPS_BAND_1_UV_STEP);
		band = SMPS_VCTRL_BAND_1;
	} else if (uV < SMPS_BAND_3_UV_MIN) {
		val = ((uV - SMPS_BAND_2_UV_MIN) / SMPS_BAND_2_UV_STEP);
		band = SMPS_VCTRL_BAND_2;
	} else {
		val = ((uV - SMPS_BAND_3_UV_MIN) / SMPS_BAND_3_UV_STEP);
		band = SMPS_VCTRL_BAND_3;
	}

	rc = pm8901_vreg_write(chip, vreg->ctrl_addr, band | val,
			SMPS_VCTRL_BAND_MASK | SMPS_VCTRL_VPROG_MASK,
			&vreg->ctrl_reg);
	if (rc)
		pr_err("%s: pm8901_write failed\n", __func__);

	return rc;
}

static int pm8901_smps_get_voltage(struct regulator_dev *dev)
{
	struct pm8901_vreg *vreg = rdev_get_drvdata(dev);
	u8 vprog, band;

	vprog = vreg->ctrl_reg & SMPS_VCTRL_VPROG_MASK;
	band = vreg->ctrl_reg & SMPS_VCTRL_BAND_MASK;

	if (band == SMPS_VCTRL_BAND_1)
		return vprog * SMPS_BAND_1_UV_STEP + SMPS_BAND_1_UV_MIN;
	else if (band == SMPS_VCTRL_BAND_2)
		return vprog * SMPS_BAND_2_UV_STEP + SMPS_BAND_2_UV_MIN;
	else
		return vprog * SMPS_BAND_3_UV_STEP + SMPS_BAND_3_UV_MIN;
}

static struct regulator_ops pm8901_ldo_ops = {
	.enable = pm8901_vreg_enable,
	.disable = pm8901_vreg_disable,
	.is_enabled = pm8901_vreg_is_enabled,
	.set_voltage = pm8901_ldo_set_voltage,
	.get_voltage = pm8901_ldo_get_voltage,
	.set_mode = pm8901_vreg_set_mode,
	.get_mode = pm8901_vreg_get_mode,
};

static struct regulator_ops pm8901_smps_ops = {
	.enable = pm8901_vreg_enable,
	.disable = pm8901_vreg_disable,
	.is_enabled = pm8901_vreg_is_enabled,
	.set_voltage = pm8901_smps_set_voltage,
	.get_voltage = pm8901_smps_get_voltage,
	.set_mode = pm8901_vreg_set_mode,
	.get_mode = pm8901_vreg_get_mode,
};

static struct regulator_ops pm8901_vs_ops = {
	.enable = pm8901_vreg_enable,
	.disable = pm8901_vreg_disable,
	.is_enabled = pm8901_vreg_is_enabled,
};

#define VREG_DESCRIP(_id, _name, _ops) \
	[_id] = { \
		.name = _name, \
		.id = _id, \
		.ops = _ops, \
		.type = REGULATOR_VOLTAGE, \
		.owner = THIS_MODULE, \
	}

static struct regulator_desc pm8901_vreg_descrip[] = {
	VREG_DESCRIP(PM8901_VREG_ID_L0, "8901_l0", &pm8901_ldo_ops),
	VREG_DESCRIP(PM8901_VREG_ID_L1, "8901_l1", &pm8901_ldo_ops),
	VREG_DESCRIP(PM8901_VREG_ID_L2, "8901_l2", &pm8901_ldo_ops),
	VREG_DESCRIP(PM8901_VREG_ID_L3, "8901_l3", &pm8901_ldo_ops),
	VREG_DESCRIP(PM8901_VREG_ID_L4, "8901_l4", &pm8901_ldo_ops),
	VREG_DESCRIP(PM8901_VREG_ID_L5, "8901_l5", &pm8901_ldo_ops),
	VREG_DESCRIP(PM8901_VREG_ID_L6, "8901_l6", &pm8901_ldo_ops),

	VREG_DESCRIP(PM8901_VREG_ID_S0, "8901_s0", &pm8901_smps_ops),
	VREG_DESCRIP(PM8901_VREG_ID_S1, "8901_s1", &pm8901_smps_ops),
	VREG_DESCRIP(PM8901_VREG_ID_S2, "8901_s2", &pm8901_smps_ops),
	VREG_DESCRIP(PM8901_VREG_ID_S3, "8901_s3", &pm8901_smps_ops),
	VREG_DESCRIP(PM8901_VREG_ID_S4, "8901_s4", &pm8901_smps_ops),

	VREG_DESCRIP(PM8901_VREG_ID_LVS0,     "8901_lvs0",     &pm8901_vs_ops),
	VREG_DESCRIP(PM8901_VREG_ID_LVS1,     "8901_lvs1",     &pm8901_vs_ops),
	VREG_DESCRIP(PM8901_VREG_ID_LVS2,     "8901_lvs2",     &pm8901_vs_ops),
	VREG_DESCRIP(PM8901_VREG_ID_LVS3,     "8901_lvs3",     &pm8901_vs_ops),
	VREG_DESCRIP(PM8901_VREG_ID_MVS0,     "8901_mvs0",     &pm8901_vs_ops),
	VREG_DESCRIP(PM8901_VREG_ID_USB_OTG,  "8901_usb_otg",  &pm8901_vs_ops),
	VREG_DESCRIP(PM8901_VREG_ID_HDMI_MVS, "8901_hdmi_mvs", &pm8901_vs_ops),
};

static int pm8901_init_regulator(struct pm8901_chip *chip,
		struct pm8901_vreg *vreg)
{
	int rc, i;
	u8 bank;

	if (vreg->type == REGULATOR_TYPE_LDO) {
		for (i = 0; i < LDO_TEST_BANKS; i++) {
			bank = REGULATOR_BANK_SEL(i);
			rc = pm8901_write(chip, vreg->test_addr,
					&bank, 1);
			if (rc)
				goto bail;

			rc = pm8901_read(chip, vreg->test_addr,
					&vreg->test_reg[i], 1);
			if (rc)
				goto bail;
		}
	}

	rc = pm8901_read(chip, vreg->ctrl_addr, &vreg->ctrl_reg, 1);
	if (rc)
		goto bail;

	rc = pm8901_read(chip, vreg->pmr_addr, &vreg->pmr_reg, 1);

bail:
	if (rc)
		pr_err("%s: pm8901_read failed\n", __func__);

	return rc;
}

static int __devinit pm8901_vreg_probe(struct platform_device *pdev)
{
	struct regulator_init_data *init_data;
	struct regulator_desc *rdesc;
	struct pm8901_chip *chip;
	struct pm8901_vreg *vreg;
	int rc = 0;

	if (pdev == NULL)
		return -EINVAL;

	if (pdev->id >= 0 && pdev->id < PM8901_VREG_MAX) {
		chip = platform_get_drvdata(pdev);
		rdesc = &pm8901_vreg_descrip[pdev->id];
		vreg = &pm8901_vreg[pdev->id];
		init_data = pdev->dev.platform_data;

		rc = pm8901_init_regulator(chip, vreg);
		if (rc)
			goto bail;

		vreg->rdev = regulator_register(rdesc, &pdev->dev,
				init_data, vreg);
		if (IS_ERR(vreg->rdev))
			rc = PTR_ERR(vreg->rdev);
	} else {
		rc = -ENODEV;
	}

bail:
	pr_info("%s: id=%d, rc=%d\n", __func__, pdev->id, rc);

	return rc;
}

static int __devexit pm8901_vreg_remove(struct platform_device *pdev)
{
	regulator_unregister(pm8901_vreg[pdev->id].rdev);
	return 0;
}

static struct platform_driver pm8901_vreg_driver = {
	.probe = pm8901_vreg_probe,
	.remove = __devexit_p(pm8901_vreg_remove),
	.driver = {
		.name = "pm8901-regulator",
		.owner = THIS_MODULE,
	},
};

static int __init pm8901_vreg_init(void)
{
	return platform_driver_register(&pm8901_vreg_driver);
}

static void __exit pm8901_vreg_exit(void)
{
	platform_driver_unregister(&pm8901_vreg_driver);
}

subsys_initcall(pm8901_vreg_init);
module_exit(pm8901_vreg_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("PMIC8901 regulator driver");
MODULE_VERSION("1.0");
MODULE_ALIAS("platform:pm8901-regulator");
