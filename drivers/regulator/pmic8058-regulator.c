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
#include <linux/mfd/pmic8058.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/regulator/pmic8058-regulator.h>

/* Regulator types */
#define REGULATOR_TYPE_LDO		0
#define REGULATOR_TYPE_SMPS		1
#define REGULATOR_TYPE_LVS		2
#define REGULATOR_TYPE_NCP		3

/* Common masks */
#define REGULATOR_EN_MASK		0x80
#define REGULATOR_BANK_SEL(n)		((n) << 4)
#define REGULATOR_BANK_WRITE		0x80

#define LDO_TEST_BANKS			7
#define SMPS_TEST_BANKS			8
#define REGULATOR_TEST_BANKS_MAX	SMPS_TEST_BANKS

#define REGULATOR_IS_EN(ctrl_reg)	((ctrl_reg & REGULATOR_EN_MASK) > 0)

/* LDO programming */
#define LDO_CTRL_VPROG_MASK		0x1F

#define LDO_CTRL_PM_MASK		0x20
#define LDO_CTRL_PM_NORMAL		0x00
#define LDO_CTRL_PM_LOW			0x20

/* bank 0 */
#define LDO_TEST_LPM_MASK		0x40
#define LDO_TEST_LPM_SEL_CTRL		0x00
#define LDO_TEST_LPM_SEL_TCXO		0x40

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

/* SMPS masks and values */
#define SMPS_VREF_SEL_MASK		0x20
#define SMPS_VPROG_MASK			0x1F

/* bank 1 */
#define SMPS_VLOW_SEL_MASK		0x01

#define SMPS_MODE1_UV_MIN		1500000
#define SMPS_MODE1_UV_MAX		3050000
#define SMPS_MODE1_STEP			50000

#define SMPS_MODE2_UV_MIN		750000
#define SMPS_MODE2_UV_MAX		1525000
#define SMPS_MODE2_STEP			25000

#define SMPS_MODE3_UV_MIN		375000
#define SMPS_MODE3_UV_MAX		1150000
#define SMPS_MODE3_STEP			25000

#define SMPS_UV_MIN			SMPS_MODE3_UV_MIN
#define SMPS_UV_MAX			SMPS_MODE1_UV_MAX

#define SMPS_CLK_CTRL_MASK		0x30
#define SMPS_CLK_CTRL_FOLLOW_TCXO	0x00
#define SMPS_CLK_CTRL_PWM		0x10
#define SMPS_CLK_CTRL_PFM		0x20

/* NCP masks and values */
#define NCP_VPROG_MASK			0x1F

#define NCP_UV_MIN			-3000000
#define NCP_UV_MAX			-1500000
#define NCP_UV_STEP			50000

struct pm8058_vreg {
	u16			ctrl_addr;
	u16			test_addr;
	u16			test2_addr;
	u16			clk_ctrl_addr;
	u8			type;
	u8			ctrl_reg;
	u8			test_reg[REGULATOR_TEST_BANKS_MAX];
	u8			test2_reg[REGULATOR_TEST_BANKS_MAX];
	u8			clk_ctrl_reg;
	u8			is_nmos;
	struct regulator_dev	*rdev;
};

#define LDO(_id, _ctrl_addr, _test_addr, _is_nmos) \
	[_id] = { \
		.ctrl_addr = _ctrl_addr, \
		.test_addr = _test_addr, \
		.type = REGULATOR_TYPE_LDO, \
		.is_nmos = _is_nmos, \
	}

#define SMPS(_id, _ctrl_addr, _test2_addr, _clk_ctrl_addr) \
	[_id] = { \
		.ctrl_addr = _ctrl_addr, \
		.test2_addr = _test2_addr, \
		.clk_ctrl_addr = _clk_ctrl_addr, \
		.type = REGULATOR_TYPE_SMPS, \
	}

#define LVS(_id, _ctrl_addr) \
	[_id] = { \
		.ctrl_addr = _ctrl_addr, \
		.type = REGULATOR_TYPE_LVS, \
	}

#define NCP(_id, _ctrl_addr) \
	[_id] = { \
		.ctrl_addr = _ctrl_addr, \
		.type = REGULATOR_TYPE_NCP, \
	}

static struct pm8058_vreg pm8058_vreg[] = {
	/*  id                  ctrl   test   n/p */
	LDO(PM8058_VREG_ID_L0,  0x009, 0x065, 1),
	LDO(PM8058_VREG_ID_L1,  0x00A, 0x066, 1),
	LDO(PM8058_VREG_ID_L2,  0x00B, 0x067, 0),
	LDO(PM8058_VREG_ID_L3,  0x00C, 0x068, 0),
	LDO(PM8058_VREG_ID_L4,  0x00D, 0x069, 0),
	LDO(PM8058_VREG_ID_L5,  0x00E, 0x06A, 0),
	LDO(PM8058_VREG_ID_L6,  0x00F, 0x06B, 0),
	LDO(PM8058_VREG_ID_L7,  0x010, 0x06C, 0),
	LDO(PM8058_VREG_ID_L8,  0x011, 0x06D, 0),
	LDO(PM8058_VREG_ID_L9,  0x012, 0x06E, 0),
	LDO(PM8058_VREG_ID_L10, 0x013, 0x06F, 0),
	LDO(PM8058_VREG_ID_L11, 0x014, 0x070, 0),
	LDO(PM8058_VREG_ID_L12, 0x015, 0x071, 0),
	LDO(PM8058_VREG_ID_L13, 0x016, 0x072, 0),
	LDO(PM8058_VREG_ID_L14, 0x017, 0x073, 0),
	LDO(PM8058_VREG_ID_L15, 0x089, 0x0E5, 0),
	LDO(PM8058_VREG_ID_L16, 0x08A, 0x0E6, 0),
	LDO(PM8058_VREG_ID_L17, 0x08B, 0x0E7, 0),
	LDO(PM8058_VREG_ID_L18, 0x11D, 0x125, 0),
	LDO(PM8058_VREG_ID_L19, 0x11E, 0x126, 0),
	LDO(PM8058_VREG_ID_L20, 0x11F, 0x127, 0),
	LDO(PM8058_VREG_ID_L21, 0x120, 0x128, 1),
	LDO(PM8058_VREG_ID_L22, 0x121, 0x129, 1),
	LDO(PM8058_VREG_ID_L23, 0x122, 0x12A, 1),
	LDO(PM8058_VREG_ID_L24, 0x123, 0x12B, 1),
	LDO(PM8058_VREG_ID_L25, 0x124, 0x12C, 1),

	/*   id                 ctrl   test2  clk_ctrl */
	SMPS(PM8058_VREG_ID_S0, 0x004, 0x084, 0x1D1),
	SMPS(PM8058_VREG_ID_S1, 0x005, 0x085, 0x1D2),
	SMPS(PM8058_VREG_ID_S2, 0x110, 0x119, 0x1D3),
	SMPS(PM8058_VREG_ID_S3, 0x111, 0x11A, 0x1D4),
	SMPS(PM8058_VREG_ID_S4, 0x112, 0x11B, 0x1D5),

	/*   id                   ctrl  */
	LVS(PM8058_VREG_ID_LVS0, 0x12D),
	LVS(PM8058_VREG_ID_LVS1, 0x12F),

	/*   id                  ctrl */
	NCP(PM8058_VREG_ID_NCP, 0x090),
};

static int pm8058_vreg_write(struct pm8058_chip *chip,
		u16 addr, u8 val, u8 mask, u8 *reg_save)
{
	int rc;
	u8 reg;

	reg = (*reg_save & ~mask) | (val & mask);
	rc = pm8058_write(chip, addr, &reg, 1);
	if (rc)
		pr_err("%s: pm8058_write failed\n", __func__);
	else
		*reg_save = reg;
	return rc;
}

static int pm8058_vreg_enable(struct regulator_dev *dev)
{
	struct pm8058_vreg *vreg = rdev_get_drvdata(dev);
	struct pm8058_chip *chip = dev_get_drvdata(dev->dev.parent);
	int rc = 0;

	rc = pm8058_vreg_write(chip, vreg->ctrl_addr,
			REGULATOR_EN_MASK, REGULATOR_EN_MASK, &vreg->ctrl_reg);
	if (rc)
		pr_err("%s: pm8058_vreg_write failed\n", __func__);

	return rc;
}

static int pm8058_vreg_is_enabled(struct regulator_dev *dev)
{
	struct pm8058_vreg *vreg = rdev_get_drvdata(dev);
	return REGULATOR_IS_EN(vreg->ctrl_reg);
}

static int pm8058_vreg_disable(struct regulator_dev *dev)
{
	struct pm8058_vreg *vreg = rdev_get_drvdata(dev);
	struct pm8058_chip *chip = dev_get_drvdata(dev->dev.parent);
	int rc = 0;

	rc = pm8058_vreg_write(chip, vreg->ctrl_addr, 0,
			REGULATOR_EN_MASK, &vreg->ctrl_reg);
	if (rc)
		pr_err("%s: pm8058_vreg_write failed\n", __func__);

	return rc;
}

static int pm8058_pldo_set_voltage(struct pm8058_chip *chip,
		struct pm8058_vreg *vreg, int uV)
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

		rc = pm8058_vreg_write(chip, vreg->test_addr, val,
				mask, &vreg->test_reg[2]);
		if (rc) {
			pr_err("%s: pm8058_write failed\n", __func__);
			return rc;
		}
	}

	/* update range extension if necessary */
	if ((vreg->test_reg[4] & LDO_TEST_RANGE_EXTN_MASK) != range_extn) {
		val = REGULATOR_BANK_SEL(4) | REGULATOR_BANK_WRITE |
			range_extn;
		mask = LDO_TEST_RANGE_EXTN_MASK | val;

		rc = pm8058_vreg_write(chip, vreg->test_addr, val,
				mask, &vreg->test_reg[4]);
		if (rc) {
			pr_err("%s: pm8058_write failed\n", __func__);
			return rc;
		}
	}

	/* voltage programming */
	val = (uV - min) / step;
	rc = pm8058_vreg_write(chip, vreg->ctrl_addr, val,
			LDO_CTRL_VPROG_MASK, &vreg->ctrl_reg);
	if (rc)
		pr_err("%s: pm8058_write failed\n", __func__);

	return rc;
}

static int pm8058_nldo_set_voltage(struct pm8058_chip *chip,
		struct pm8058_vreg *vreg, int uV)
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

		rc = pm8058_vreg_write(chip, vreg->test_addr, val,
				mask, &vreg->test_reg[2]);
		if (rc) {
			pr_err("%s: pm8058_write failed\n", __func__);
			return rc;
		}
	}

	/* voltage programming */
	val = (uV - NLDO_UV_MIN) / NLDO_UV_STEP;
	rc = pm8058_vreg_write(chip, vreg->ctrl_addr, val,
			LDO_CTRL_VPROG_MASK, &vreg->ctrl_reg);
	if (rc)
		pr_err("%s: pm8058_write failed\n", __func__);

	return rc;
}

static int pm8058_ldo_set_voltage(struct regulator_dev *dev,
		int min_uV, int max_uV)
{
	struct pm8058_vreg *vreg = rdev_get_drvdata(dev);
	struct pm8058_chip *chip = dev_get_drvdata(dev->dev.parent);
	int uV = (min_uV + max_uV) / 2;

	if (vreg->is_nmos)
		return pm8058_nldo_set_voltage(chip, vreg, uV);
	else
		return pm8058_pldo_set_voltage(chip, vreg, uV);
}

static int pm8058_pldo_get_voltage(struct pm8058_vreg *vreg)
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

static int pm8058_nldo_get_voltage(struct pm8058_vreg *vreg)
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

static int pm8058_ldo_get_voltage(struct regulator_dev *dev)
{
	struct pm8058_vreg *vreg = rdev_get_drvdata(dev);

	if (vreg->is_nmos)
		return pm8058_nldo_get_voltage(vreg);
	else
		return pm8058_pldo_get_voltage(vreg);
}

static int pm8058_ldo_set_mode(struct regulator_dev *dev, unsigned int mode)
{
	struct pm8058_vreg *vreg = rdev_get_drvdata(dev);
	struct pm8058_chip *chip = dev_get_drvdata(dev->dev.parent);
	int rc = 0;
	u8 ctrl, test, mask, val;

	if (mode == REGULATOR_MODE_NORMAL) {
		/* disable low power mode */
		test = LDO_TEST_LPM_SEL_TCXO;
		ctrl = LDO_CTRL_PM_NORMAL;
	} else if (mode == REGULATOR_MODE_IDLE) {
		/* enable low power mode but follow tcxo */
		test = LDO_TEST_LPM_SEL_TCXO;
		ctrl = LDO_CTRL_PM_LOW;
	} else if (mode == REGULATOR_MODE_STANDBY) {
		/* force low power mode */
		test = LDO_TEST_LPM_SEL_CTRL;
		ctrl = LDO_CTRL_PM_LOW;
	} else {
		return -EINVAL;
	}

	if ((vreg->test_reg[0] & LDO_TEST_LPM_MASK) != test) {
		val = REGULATOR_EN_MASK | REGULATOR_BANK_SEL(0) | test;
		mask = REGULATOR_EN_MASK | REGULATOR_BANK_SEL(0) |
			LDO_TEST_LPM_MASK;
		rc = pm8058_vreg_write(chip, vreg->test_addr,
				val, mask, &vreg->test_reg[0]);
		if (rc) {
			pr_err("%s: pm8058_vreg_write failed\n", __func__);
			return rc;
		}
	}

	if ((vreg->ctrl_reg & LDO_CTRL_PM_MASK) != ctrl) {
		val = ctrl;
		mask = LDO_CTRL_PM_MASK;
		rc = pm8058_vreg_write(chip, vreg->ctrl_addr,
				val, mask, &vreg->ctrl_reg);
		if (rc)
			pr_err("%s: pm8058_vreg_write failed\n", __func__);
	}

	return rc;
}

static unsigned int pm8058_ldo_get_mode(struct regulator_dev *dev)
{
	struct pm8058_vreg *vreg = rdev_get_drvdata(dev);
	u8 lp_sel, pm;

	lp_sel = vreg->test_reg[0] & LDO_TEST_LPM_MASK;
	pm = vreg->ctrl_reg & LDO_CTRL_PM_MASK;

	if (pm == LDO_CTRL_PM_NORMAL)
		return REGULATOR_MODE_NORMAL;
	else if (lp_sel == LDO_TEST_LPM_SEL_TCXO && pm == LDO_CTRL_PM_LOW)
		return REGULATOR_MODE_IDLE;
	else if (lp_sel == LDO_TEST_LPM_SEL_CTRL && pm == LDO_CTRL_PM_LOW)
		return REGULATOR_MODE_STANDBY;

	pr_err("%s: unexpected lpm config 0x%x 0x%x\n",
			__func__, lp_sel, pm);
	return (unsigned int) -EINVAL;
}

static int pm8058_smps_set_voltage(struct regulator_dev *dev,
		int min_uV, int max_uV)
{
	struct pm8058_vreg *vreg = rdev_get_drvdata(dev);
	struct pm8058_chip *chip = dev_get_drvdata(dev->dev.parent);
	int rc, uV = (min_uV + max_uV) / 2;
	u8 val, mask, vlow;

	if (uV >= SMPS_MODE3_UV_MIN && uV <= SMPS_MODE3_UV_MAX) {
		vlow = SMPS_VLOW_SEL_MASK;
		val = ((uV  - SMPS_MODE3_UV_MIN) / SMPS_MODE3_STEP) |
			SMPS_VREF_SEL_MASK;
	} else if (uV >= SMPS_MODE2_UV_MIN && uV <= SMPS_MODE2_UV_MAX) {
		vlow = 0;
		val = ((uV - SMPS_MODE2_UV_MIN) / SMPS_MODE2_STEP) |
			SMPS_VREF_SEL_MASK;
	} else if (uV >= SMPS_MODE1_UV_MIN && uV <= SMPS_MODE1_UV_MAX) {
		vlow = 0;
		val = (uV - SMPS_MODE1_UV_MIN) / SMPS_MODE1_STEP;
	} else {
		return -EINVAL;
	}

	/* set vlow bit for ultra low voltage mode */
	if ((vreg->test2_reg[1] & SMPS_VLOW_SEL_MASK) != vlow) {
		vlow |= REGULATOR_BANK_WRITE | REGULATOR_BANK_SEL(1);
		mask = vlow | SMPS_VLOW_SEL_MASK;
		rc = pm8058_vreg_write(chip, vreg->test2_addr,
				vlow, mask, &vreg->test2_reg[1]);
	}

	/* voltage setting */
	mask = SMPS_VREF_SEL_MASK | SMPS_VPROG_MASK;
	rc = pm8058_vreg_write(chip, vreg->ctrl_addr, val, mask,
			&vreg->ctrl_reg);
	if (rc)
		pr_err("%s: pm8058_vreg_write failed\n", __func__);

	return rc;
}

static int pm8058_smps_get_voltage(struct regulator_dev *dev)
{
	struct pm8058_vreg *vreg = rdev_get_drvdata(dev);
	u8 vlow, vref, vprog;
	int uV;

	vlow = vreg->test2_reg[1] & SMPS_VLOW_SEL_MASK;
	vref = vreg->ctrl_reg & SMPS_VREF_SEL_MASK;
	vprog = vreg->ctrl_reg & SMPS_VPROG_MASK;

	if (vlow && vref) {
		/* mode 3 */
		uV = vprog * SMPS_MODE3_STEP + SMPS_MODE3_UV_MIN;
	} else if (vref) {
		/* mode 2 */
		uV = vprog * SMPS_MODE2_STEP + SMPS_MODE2_UV_MIN;
	} else {
		/* mode 1 */
		uV = vprog * SMPS_MODE1_STEP + SMPS_MODE1_UV_MIN;
	}

	return uV;
}

static int pm8058_smps_set_mode(struct regulator_dev *dev, unsigned int mode)
{
	struct pm8058_vreg *vreg = rdev_get_drvdata(dev);
	struct pm8058_chip *chip = dev_get_drvdata(dev->dev.parent);
	int rc;
	u8 clk;

	if (mode == REGULATOR_MODE_NORMAL)
		clk = SMPS_CLK_CTRL_PWM;
	else if (mode == REGULATOR_MODE_IDLE)
		clk = SMPS_CLK_CTRL_FOLLOW_TCXO;
	else if (mode == REGULATOR_MODE_STANDBY)
		clk = SMPS_CLK_CTRL_PFM;
	else
		return -EINVAL;

	if ((vreg->clk_ctrl_reg & SMPS_CLK_CTRL_MASK) == clk)
		return 0;

	rc = pm8058_vreg_write(chip, vreg->clk_ctrl_addr,
			clk, SMPS_CLK_CTRL_MASK, &vreg->clk_ctrl_reg);
	if (rc)
		pr_err("%s: pm8058_vreg_write failed\n", __func__);

	return rc;
}

static unsigned int pm8058_smps_get_mode(struct regulator_dev *dev)
{
	struct pm8058_vreg *vreg = rdev_get_drvdata(dev);
	u8 mode = vreg->clk_ctrl_reg & SMPS_CLK_CTRL_MASK;

	if (mode == SMPS_CLK_CTRL_PWM)
		return REGULATOR_MODE_NORMAL;
	else if (mode == SMPS_CLK_CTRL_FOLLOW_TCXO)
		return REGULATOR_MODE_IDLE;
	else if (mode == SMPS_CLK_CTRL_PFM)
		return REGULATOR_MODE_STANDBY;

	pr_err("%s: unexpected smps mode 0x%x\n",
			__func__, mode);
	return (unsigned int) -EINVAL;
}

static int pm8058_ncp_set_voltage(struct regulator_dev *dev,
		int min_uV, int max_uV)
{
	struct pm8058_vreg *vreg = rdev_get_drvdata(dev);
	struct pm8058_chip *chip = dev_get_drvdata(dev->dev.parent);
	int rc, uV = (min_uV + max_uV) / 2;
	u8 val;

	val = (NCP_UV_MAX - uV) / NCP_UV_STEP;

	/* voltage setting */
	rc = pm8058_vreg_write(chip, vreg->ctrl_addr, val, NCP_VPROG_MASK,
			&vreg->ctrl_reg);
	if (rc)
		pr_err("%s: pm8058_vreg_write failed\n", __func__);

	return rc;
}

static int pm8058_ncp_get_voltage(struct regulator_dev *dev)
{
	struct pm8058_vreg *vreg = rdev_get_drvdata(dev);
	u8 vprog = vreg->ctrl_reg & NCP_VPROG_MASK;
	return NCP_UV_MAX - vprog * NCP_UV_STEP;
}

static struct regulator_ops pm8058_ldo_ops = {
	.enable = pm8058_vreg_enable,
	.disable = pm8058_vreg_disable,
	.is_enabled = pm8058_vreg_is_enabled,
	.set_voltage = pm8058_ldo_set_voltage,
	.get_voltage = pm8058_ldo_get_voltage,
	.set_mode = pm8058_ldo_set_mode,
	.get_mode = pm8058_ldo_get_mode,
};

static struct regulator_ops pm8058_smps_ops = {
	.enable = pm8058_vreg_enable,
	.disable = pm8058_vreg_disable,
	.is_enabled = pm8058_vreg_is_enabled,
	.set_voltage = pm8058_smps_set_voltage,
	.get_voltage = pm8058_smps_get_voltage,
	.set_mode = pm8058_smps_set_mode,
	.get_mode = pm8058_smps_get_mode,
};

static struct regulator_ops pm8058_lvs_ops = {
	.enable = pm8058_vreg_enable,
	.disable = pm8058_vreg_disable,
	.is_enabled = pm8058_vreg_is_enabled,
};

static struct regulator_ops pm8058_ncp_ops = {
	.enable = pm8058_vreg_enable,
	.disable = pm8058_vreg_disable,
	.is_enabled = pm8058_vreg_is_enabled,
	.set_voltage = pm8058_ncp_set_voltage,
	.get_voltage = pm8058_ncp_get_voltage,
};

#define VREG_DESCRIP(_id, _name, _ops) \
	[_id] = { \
		.id = _id, \
		.name = _name, \
		.ops = _ops, \
		.type = REGULATOR_VOLTAGE, \
		.owner = THIS_MODULE, \
	}

static struct regulator_desc pm8058_vreg_descrip[] = {
	VREG_DESCRIP(PM8058_VREG_ID_L0,  "8058_l0",  &pm8058_ldo_ops),
	VREG_DESCRIP(PM8058_VREG_ID_L1,  "8058_l1",  &pm8058_ldo_ops),
	VREG_DESCRIP(PM8058_VREG_ID_L2,  "8058_l2",  &pm8058_ldo_ops),
	VREG_DESCRIP(PM8058_VREG_ID_L3,  "8058_l3",  &pm8058_ldo_ops),
	VREG_DESCRIP(PM8058_VREG_ID_L4,  "8058_l4",  &pm8058_ldo_ops),
	VREG_DESCRIP(PM8058_VREG_ID_L5,  "8058_l5",  &pm8058_ldo_ops),
	VREG_DESCRIP(PM8058_VREG_ID_L6,  "8058_l6",  &pm8058_ldo_ops),
	VREG_DESCRIP(PM8058_VREG_ID_L7,  "8058_l7",  &pm8058_ldo_ops),
	VREG_DESCRIP(PM8058_VREG_ID_L8,  "8058_l8",  &pm8058_ldo_ops),
	VREG_DESCRIP(PM8058_VREG_ID_L9,  "8058_l9",  &pm8058_ldo_ops),
	VREG_DESCRIP(PM8058_VREG_ID_L10, "8058_l10", &pm8058_ldo_ops),
	VREG_DESCRIP(PM8058_VREG_ID_L11, "8058_l11", &pm8058_ldo_ops),
	VREG_DESCRIP(PM8058_VREG_ID_L12, "8058_l12", &pm8058_ldo_ops),
	VREG_DESCRIP(PM8058_VREG_ID_L13, "8058_l13", &pm8058_ldo_ops),
	VREG_DESCRIP(PM8058_VREG_ID_L14, "8058_l14", &pm8058_ldo_ops),
	VREG_DESCRIP(PM8058_VREG_ID_L15, "8058_l15", &pm8058_ldo_ops),
	VREG_DESCRIP(PM8058_VREG_ID_L16, "8058_l16", &pm8058_ldo_ops),
	VREG_DESCRIP(PM8058_VREG_ID_L17, "8058_l17", &pm8058_ldo_ops),
	VREG_DESCRIP(PM8058_VREG_ID_L18, "8058_l18", &pm8058_ldo_ops),
	VREG_DESCRIP(PM8058_VREG_ID_L19, "8058_l19", &pm8058_ldo_ops),
	VREG_DESCRIP(PM8058_VREG_ID_L20, "8058_l20", &pm8058_ldo_ops),
	VREG_DESCRIP(PM8058_VREG_ID_L21, "8058_l21", &pm8058_ldo_ops),
	VREG_DESCRIP(PM8058_VREG_ID_L22, "8058_l22", &pm8058_ldo_ops),
	VREG_DESCRIP(PM8058_VREG_ID_L23, "8058_l23", &pm8058_ldo_ops),
	VREG_DESCRIP(PM8058_VREG_ID_L24, "8058_l24", &pm8058_ldo_ops),
	VREG_DESCRIP(PM8058_VREG_ID_L25, "8058_l25", &pm8058_ldo_ops),

	VREG_DESCRIP(PM8058_VREG_ID_S0, "8058_s0", &pm8058_smps_ops),
	VREG_DESCRIP(PM8058_VREG_ID_S1, "8058_s1", &pm8058_smps_ops),
	VREG_DESCRIP(PM8058_VREG_ID_S2, "8058_s2", &pm8058_smps_ops),
	VREG_DESCRIP(PM8058_VREG_ID_S3, "8058_s3", &pm8058_smps_ops),
	VREG_DESCRIP(PM8058_VREG_ID_S4, "8058_s4", &pm8058_smps_ops),

	VREG_DESCRIP(PM8058_VREG_ID_LVS0, "8058_lvs0", &pm8058_lvs_ops),
	VREG_DESCRIP(PM8058_VREG_ID_LVS1, "8058_lvs1", &pm8058_lvs_ops),

	VREG_DESCRIP(PM8058_VREG_ID_NCP, "8058_ncp", &pm8058_ncp_ops),
};

static int pm8058_init_regulator(struct pm8058_chip *chip,
		struct pm8058_vreg *vreg)
{
	int rc = 0, i;
	u8 bank;

	/* save the current control register state */
	rc = pm8058_read(chip, vreg->ctrl_addr, &vreg->ctrl_reg, 1);
	if (rc)
		return rc;

	/* save the current test/test2 register state */
	if (vreg->type == REGULATOR_TYPE_LDO) {
		for (i = 0; i < LDO_TEST_BANKS; i++) {
			bank = REGULATOR_BANK_SEL(i);
			rc = pm8058_write(chip, vreg->test_addr,
					&bank, 1);
			if (rc)
				goto bail;

			rc = pm8058_read(chip, vreg->test_addr,
					&vreg->test_reg[i], 1);
			if (rc)
				goto bail;
		}
	} else if (vreg->type == REGULATOR_TYPE_SMPS) {
		for (i = 0; i < SMPS_TEST_BANKS; i++) {
			bank = REGULATOR_BANK_SEL(i);
			rc = pm8058_write(chip, vreg->test2_addr,
					&bank, 1);
			if (rc)
				goto bail;

			rc = pm8058_read(chip, vreg->test2_addr,
					&vreg->test2_reg[i], 1);
			if (rc)
				goto bail;
		}

		rc = pm8058_read(chip, vreg->clk_ctrl_addr,
				&vreg->clk_ctrl_reg, 1);
		if (rc)
			goto bail;
	}

bail:
	if (rc)
		pr_err("%s: pm8058_read/write failed\n", __func__);

	return rc;
}

static int __devinit pm8058_vreg_probe(struct platform_device *pdev)
{
	struct regulator_init_data *init_data;
	struct regulator_desc *rdesc;
	struct pm8058_chip *chip;
	struct pm8058_vreg *vreg;
	int rc = 0;

	if (pdev == NULL)
		return -EINVAL;

	if (pdev->id >= 0 && pdev->id < PM8058_VREG_MAX) {
		chip = platform_get_drvdata(pdev);
		rdesc = &pm8058_vreg_descrip[pdev->id];
		vreg = &pm8058_vreg[pdev->id];
		init_data = pdev->dev.platform_data;

		rc = pm8058_init_regulator(chip, vreg);
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

static int __devexit pm8058_vreg_remove(struct platform_device *pdev)
{
	regulator_unregister(pm8058_vreg[pdev->id].rdev);
	return 0;
}

static struct platform_driver pm8058_vreg_driver = {
	.probe = pm8058_vreg_probe,
	.remove = __devexit_p(pm8058_vreg_remove),
	.driver = {
		.name = "pm8058-regulator",
		.owner = THIS_MODULE,
	},
};

static int __init pm8058_vreg_init(void)
{
	return platform_driver_register(&pm8058_vreg_driver);
}

static void __exit pm8058_vreg_exit(void)
{
	platform_driver_unregister(&pm8058_vreg_driver);
}

subsys_initcall(pm8058_vreg_init);
module_exit(pm8058_vreg_exit);

MODULE_LICENSE("GPLv2");
MODULE_DESCRIPTION("PMIC8058 regulator driver");
MODULE_VERSION("1.0");
MODULE_ALIAS("platform:pm8058-regulator");
