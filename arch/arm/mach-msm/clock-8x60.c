/* Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
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
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/ctype.h>
#include <linux/bitops.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <mach/msm_iomap.h>
#include <mach/clk.h>

#include "clock.h"
#include "clock-8x60.h"

/* When enabling/disabling a clock, check the halt bit up to this number
 * number of times (with a 1 us delay in between) before continuing. */
#define HALT_CHECK_MAX_LOOPS	100

#define C(x) L_8X60_##x##_CLK
#define REG(off)	(MSM_CLK_CTL_BASE + (off))
#define REG_MM(off)	(MSM_MMSS_CLK_CTL_BASE + (off))
#define REG_LPA(off)	(MSM_LPASS_CLK_CTL_BASE + (off))

/* Peripheral clock registers. */
#define BBRX_SSBI_CLK_CTL_REG		REG(0x2CE0)
#define CLK_HALT_CFPB_STATEA_REG	REG(0x2FCC)
#define CLK_HALT_CFPB_STATEB_REG	REG(0x2FD0)
#define CLK_HALT_CFPB_STATEC_REG	REG(0x2FD4)
#define CLK_HALT_DFAB_STATE_REG		REG(0x2FC8)
#define CLK_HALT_SFPB_MISC_STATE_REG	REG(0x2FD8)
#define CLK_TEST_REG			REG(0x2FA0)
#define GSBI_COMMON_SIM_CLK_NS_REG	REG(0x29A0)
#define GSBIn_HCLK_CTL_REG(n)		REG(0x29C0+(0x20*((n)-1)))
#define GSBIn_QUP_APPS_NS_REG(n)	REG(0x29CC+(0x20*((n)-1)))
#define GSBIn_RESET_REG(n)		REG(0x29DC+(0x20*((n)-1)))
#define GSBIn_SIM_CLK_CTL_REG(n)	REG(0x29D8+(0x20*((n)-1)))
#define GSBIn_UART_APPS_NS_REG(n)	REG(0x29D4+(0x20*((n)-1)))
#define PDM_CLK_NS_REG			REG(0x2CC0)
#define PLL_ENA_SC0_REG			REG(0x34C0)
#define PRNG_CLK_NS_REG			REG(0x2E80)
#define RINGOSC_NS_REG			REG(0x2DC0)
#define RINGOSC_STATUS_REG		REG(0x2DCC)
#define RINGOSC_TCXO_CTL_REG		REG(0x2DC4)
#define SDCn_APPS_CLK_NS_REG(n)		REG(0x282C+(0x20*((n)-1)))
#define SDCn_HCLK_CTL_REG(n)		REG(0x2820+(0x20*((n)-1)))
#define TSIF_HCLK_CTL_REG		REG(0x2700)
#define TSIF_REF_CLK_NS_REG		REG(0x2710)
#define TSSC_CLK_CTL_REG		REG(0x2CA0)
#define USB_FS1_HCLK_CTL_REG		REG(0x2960)
#define USB_FS1_RESET_REG		REG(0x2974)
#define USB_FS1_SYSTEM_CLK_CTL_REG	REG(0x296C)
#define USB_FS1_XCVR_FS_CLK_NS_REG	REG(0x2968)
#define USB_FS2_HCLK_CTL_REG		REG(0x2980)
#define USB_FS2_RESET_REG		REG(0x2994)
#define USB_FS2_SYSTEM_CLK_CLK_REG	REG(0x298C)
#define USB_FS2_XCVR_FS_CLK_NS_REG	REG(0x2988)
#define USB_HS1_HCLK_CTL_REG		REG(0x2900)
#define USB_HS1_XCVR_FS_CLK_NS		REG(0x290C)
#define USB_PHY0_RESET_REG		REG(0x2E20)

/* Multimedia clock registers. */
#define AHB_EN_REG			REG_MM(0x0008)
#define AHB_EN2_REG			REG_MM(0x0038)
#define AHB_NS_REG			REG_MM(0x0004)
#define AXI_NS_REG			REG_MM(0x0014)
#define CAMCLK_NS_REG			REG_MM(0x0148)
#define CSI_CC_REG			REG_MM(0x0040)
#define CSI_NS_REG			REG_MM(0x0048)
#define DBG_BUS_VEC_A_REG		REG_MM(0x01C8)
#define DBG_BUS_VEC_B_REG		REG_MM(0x01CC)
#define DBG_BUS_VEC_C_REG		REG_MM(0x01D0)
#define DBG_BUS_VEC_D_REG		REG_MM(0x01D4)
#define DBG_BUS_VEC_E_REG		REG_MM(0x01D8)
#define DBG_BUS_VEC_F_REG		REG_MM(0x01DC)
#define DBG_BUS_VEC_H_REG		REG_MM(0x01E4)
#define DBG_CFG_REG_HS_REG		REG_MM(0x01B4)
#define DBG_CFG_REG_LS_REG		REG_MM(0x01B8)
#define DSI_NS_REG			REG_MM(0x0054)
#define GFX2D0_MD0_REG			REG_MM(0x0064)
#define GFX2D0_MD1_REG			REG_MM(0x0068)
#define GFX2D0_NS_REG			REG_MM(0x0070)
#define GFX2D1_MD0_REG			REG_MM(0x0078)
#define GFX2D1_MD1_REG			REG_MM(0x006C)
#define GFX2D1_NS_REG			REG_MM(0x007C)
#define GFX3D_MD0_REG			REG_MM(0x0084)
#define GFX3D_MD1_REG			REG_MM(0x0088)
#define GFX3D_NS_REG			REG_MM(0x008C)
#define IJPEG_NS_REG			REG_MM(0x00A0)
#define JPEGD_NS_REG			REG_MM(0x00AC)
#define MDP_MD0_REG			REG_MM(0x00C4)
#define MAXI_EN_REG			REG_MM(0x0018)
#define MAXI_EN2_REG			REG_MM(0x0020)
#define MAXI_EN3_REG			REG_MM(0x002C)
#define MDP_MD1_REG			REG_MM(0x00C8)
#define MDP_NS_REG			REG_MM(0x00D0)
#define MISC_CC_REG			REG_MM(0x0058)
#define MISC_CC2_REG			REG_MM(0x005C)
#define PIXEL_CC_REG			REG_MM(0x00D4)
#define PIXEL_NS_REG			REG_MM(0x00DC)
#define PLL0_CONFIG_REG			REG_MM(0x0310)
#define PLL0_L_VAL_REG			REG_MM(0x0304)
#define PLL0_M_VAL_REG			REG_MM(0x0308)
#define PLL0_MODE_REG			REG_MM(0x0300)
#define PLL0_N_VAL_REG			REG_MM(0x030C)
#define PLL1_CONFIG_REG			REG_MM(0x032C)
#define PLL1_L_VAL_REG			REG_MM(0x0320)
#define PLL1_M_VAL_REG			REG_MM(0x0324)
#define PLL1_MODE_REG			REG_MM(0x031C)
#define PLL1_N_VAL_REG			REG_MM(0x0328)
#define PLL2_CONFIG_REG			REG_MM(0x0348)
#define PLL2_L_VAL_REG			REG_MM(0x033C)
#define PLL2_M_VAL_REG			REG_MM(0x0340)
#define PLL2_MODE_REG			REG_MM(0x0338)
#define PLL2_N_VAL_REG			REG_MM(0x0344)
#define ROT_NS_REG			REG_MM(0x00E8)
#define SAXI_EN_REG			REG_MM(0x0030)
#define SW_RESET_AHB_REG		REG_MM(0x020C)
#define SW_RESET_ALL_REG		REG_MM(0x0204)
#define SW_RESET_AXI_REG		REG_MM(0x0208)
#define SW_RESET_CORE_REG		REG_MM(0x0210)
#define TV_CC_REG			REG_MM(0x00EC)
#define TV_CC2_REG			REG_MM(0x0124)
#define TV_NS_REG			REG_MM(0x00F4)
#define VCODEC_MD0_REG			REG_MM(0x00FC)
#define VCODEC_MD1_REG			REG_MM(0x0128)
#define VCODEC_NS_REG			REG_MM(0x0100)
#define VFE_CC_REG			REG_MM(0x0104)
#define VFE_NS_REG			REG_MM(0x010C)
#define VPE_NS_REG			REG_MM(0x0118)

/* Low-power Audio clock registers. */
#define LCC_CLK_LS_DEBUG_CFG_REG	REG_LPA(0x00A8)
#define LCC_CODEC_I2S_MIC_NS_REG	REG_LPA(0x0060)
#define LCC_CODEC_I2S_MIC_STATUS_REG	REG_LPA(0x0068)
#define LCC_CODEC_I2S_SPKR_NS_REG	REG_LPA(0x006C)
#define LCC_CODEC_I2S_SPKR_STATUS_REG	REG_LPA(0x0074)
#define LCC_MI2S_NS_REG			REG_LPA(0x0048)
#define LCC_MI2S_STATUS_REG		REG_LPA(0x0050)
#define LCC_PCM_NS_REG			REG_LPA(0x0054)
#define LCC_PCM_STATUS_REG		REG_LPA(0x005C)
#define LCC_PLL0_CONFIG_REG		REG_LPA(0x0014)
#define LCC_PLL0_L_VAL_REG		REG_LPA(0x0004)
#define LCC_PLL0_M_VAL_REG		REG_LPA(0x0008)
#define LCC_PLL0_MODE_REG		REG_LPA(0x0000)
#define LCC_PLL0_N_VAL_REG		REG_LPA(0x000C)
#define LCC_PRI_PLL_CLK_CTL_REG		REG_LPA(0x00C4)
#define LCC_SPARE_I2S_MIC_NS_REG	REG_LPA(0x0078)
#define LCC_SPARE_I2S_MIC_STATUS_REG	REG_LPA(0x0080)
#define LCC_SPARE_I2S_SPKR_NS_REG	REG_LPA(0x0084)
#define LCC_SPARE_I2S_SPKR_STATUS_REG	REG_LPA(0x008C)

/* MUX source input identifiers. */
#define NONE_SRC	-1
#define BB_PXO_SRC	0
#define	BB_MXO_SRC	1
#define BB_CXO_SRC	BB_PXO_SRC
#define	BB_PLL0_SRC	2
#define	BB_PLL8_SRC	3
#define	BB_PLL6_SRC	4
#define	MM_PXO_SRC	0
#define MM_PLL0_SRC	1
#define	MM_PLL1_SRC	1
#define	MM_PLL2_SRC	3
#define	MM_GPERF_SRC	2
#define	MM_GPLL0_SRC	3
#define	MM_MXO_SRC	4
#define XO_CXO_SRC	0
#define XO_PXO_SRC	1
#define XO_MXO_SRC	2
#define LPA_PXO_SRC	0
#define LPA_CXO_SRC	1
#define LPA_PLL0_SRC	2

/* Source name to PLL mappings. */
#define NONE_PLL	-1
#define BB_PXO_PLL	NONE_PLL
#define	BB_MXO_PLL	NONE_PLL
#define	BB_CXO_PLL	NONE_PLL
#define	BB_PLL0_PLL	PLL_0
#define	BB_PLL8_PLL	PLL_8
#define	BB_PLL6_PLL	PLL_6
#define	MM_PXO_PLL	NONE_PLL
#define MM_PLL0_PLL	PLL_1
#define	MM_PLL1_PLL	PLL_2
#define	MM_PLL2_PLL	PLL_3
#define	MM_GPERF_PLL	PLL_8
#define	MM_GPLL0_PLL	PLL_0
#define	MM_MXO_PLL	NONE_PLL
#define XO_CXO_PLL	NONE_PLL
#define XO_PXO_PLL	NONE_PLL
#define XO_MXO_PLL	NONE_PLL
#define LPA_PXO_PLL	NONE_PLL
#define LPA_CXO_PLL	NONE_PLL
#define LPA_PLL0_PLL	PLL_4

/* Bit manipulation macros. */
#define B(x)	BIT(x)
#define BM(msb, lsb)	(((((uint32_t)-1) << (31-msb)) >> (31-msb+lsb)) << lsb)
#define BVAL(msb, lsb, val)	(((val) << lsb) & BM(msb, lsb))

#define MND		1 /* Integer predivider and fractional MN:D divider. */
#define BASIC		2 /* Integer divider. */
#define NORATE		3 /* Just on/off. */
#define RESET		4 /* Reset only. */

#define CLK_LOCAL(id, t, ns_r, cc_r, md_r, r_r, r_m, h_r, h_c, h_b, br, root, \
			n_m, c_m, s_fn, tbl, bmnd, par, chld_lst, tv) \
	[C(id)] = { \
	.type = t, \
	.ns_reg = ns_r, \
	.cc_reg = cc_r, \
	.md_reg = md_r, \
	.reset_reg = r_r, \
	.halt_reg = h_r, \
	.halt_check = h_c, \
	.halt_bit = h_b, \
	.reset_mask = r_m, \
	.br_en_mask = br, \
	.root_en_mask = root, \
	.ns_mask = n_m, \
	.cc_mask = c_m, \
	.parent = C(par), \
	.children = chld_lst, \
	.set_rate = s_fn, \
	.freq_tbl = tbl, \
	.banked_mnd_masks = bmnd, \
	.current_freq = &dummy_freq, \
	.test_vector = tv, \
	}

#define F_RAW_PLL(f, p, m_v, n_v, c_v, m_m, p_r) { \
	.freq_hz = f, \
	.pll = p, \
	.md_val = m_v, \
	.ns_val = n_v, \
	.cc_val = c_v, \
	.mnd_en_mask = m_m, \
	.pll_rate = p_r, \
	}
#define F_RAW(f, p, m_v, n_v, c_v, m_m) \
		F_RAW_PLL(f, p, m_v, n_v, c_v, m_m, 0)
#define FREQ_END	0
#define F_END	F_RAW(FREQ_END, NONE_PLL, 0, 0, 0, 0)

#define PLL_RATE(r, l, m, n, v, d) { l, m, n, v, (d>>1) }
struct pll_rate {
	uint32_t	l_val;
	uint32_t	m_val;
	uint32_t	n_val;
	uint32_t	vco;
	uint32_t	post_div;
};

struct clk_freq_tbl {
	uint32_t	freq_hz;
	int		pll;
	uint32_t	md_val;
	uint32_t	ns_val;
	uint32_t	cc_val;
	uint32_t	mnd_en_mask;
	struct pll_rate *pll_rate;
};

/* Some clocks have two banks to avoid glitches when switching frequencies.
 * The unused bank is programmed while running on the other bank, and
 * switched to afterwards. The following two structs describe the banks. */
struct bank_mask_info {
	void		*md_reg;
	uint32_t	ns_mask;
	uint32_t	rst_mask;
	uint32_t	mnd_en_mask;
	uint32_t	mode_mask;
};
struct banked_mnd_masks {
	uint32_t		bank_sel_mask;
	struct bank_mask_info	bank0_mask;
	struct bank_mask_info	bank1_mask;
};

struct clk_local {
	uint32_t	count;
	uint32_t	type;
	void		*ns_reg;
	void		*cc_reg;
	void		*md_reg;
	void		*reset_reg;
	void		*const halt_reg;
	uint32_t	reset_mask;
	const uint16_t	halt_check;
	const uint16_t	halt_bit;
	uint32_t	br_en_mask;
	uint32_t	root_en_mask;
	uint32_t	ns_mask;
	uint32_t	cc_mask;
	int		parent;
	uint32_t	*children;
	uint32_t	test_vector;
	void		(*set_rate)(struct clk_local *, struct clk_freq_tbl *);
	struct clk_freq_tbl	*freq_tbl;
	struct banked_mnd_masks	*banked_mnd_masks;
	struct clk_freq_tbl	*current_freq;
};

/* _soc_clk_set_rate() match types. */
enum match_types {
	MATCH_EXACT,
	MATCH_MIN,
};

/*
 * Set-Rate Functions
 */
static void set_rate_basic(struct clk_local *clk, struct clk_freq_tbl *nf)
{
	uint32_t reg_val;

	reg_val = readl(clk->ns_reg);
	reg_val &= ~(clk->ns_mask);
	reg_val |= nf->ns_val;
	writel(reg_val, clk->ns_reg);
}

static void set_rate_mnd(struct clk_local *clk, struct clk_freq_tbl *nf)
{
	uint32_t ns_reg_val, cc_reg_val;

	/* Assert MND reset. */
	ns_reg_val = readl(clk->ns_reg);
	ns_reg_val |= B(7);
	writel(ns_reg_val, clk->ns_reg);

	/* Program M and D values. */
	writel(nf->md_val, clk->md_reg);

	/* Program NS register. */
	ns_reg_val &= ~(clk->ns_mask);
	ns_reg_val |= nf->ns_val;
	writel(ns_reg_val, clk->ns_reg);

	/* If the clock has a separate CC register, program it. */
	if (clk->ns_reg != clk->cc_reg) {
		cc_reg_val = readl(clk->cc_reg);
		cc_reg_val &= ~(clk->cc_mask);
		cc_reg_val |= nf->cc_val;
		writel(cc_reg_val, clk->cc_reg);
	}

	/* Deassert MND reset. */
	ns_reg_val &= ~B(7);
	writel(ns_reg_val, clk->ns_reg);
}

static void set_rate_cam(struct clk_local *clk, struct clk_freq_tbl *nf)
{
	uint32_t ns_reg_val, cc_reg_val;

	/* Assert MND reset. */
	cc_reg_val = readl(clk->cc_reg);
	cc_reg_val |= B(8);
	writel(cc_reg_val, clk->cc_reg);

	/* Program M and D values. */
	writel(nf->md_val, clk->md_reg);

	/* Program MN counter Enable and Mode. */
	cc_reg_val &= ~(clk->cc_mask);
	cc_reg_val |= nf->cc_val;
	writel(cc_reg_val, clk->cc_reg);

	/* Program N value, divider and source. */
	ns_reg_val = readl(clk->ns_reg);
	ns_reg_val &= ~(clk->ns_mask);
	ns_reg_val |= nf->ns_val;
	writel(ns_reg_val, clk->ns_reg);

	/* Deassert MND reset. */
	cc_reg_val &= ~B(8);
	writel(cc_reg_val, clk->cc_reg);
}

/* Unlike other clocks, the TV rate is adjusted through PLL
 * re-programming. It is also routed through an MND divider. */
static void set_rate_tv(struct clk_local *clk, struct clk_freq_tbl *nf)
{
	struct pll_rate *rate = nf->pll_rate;
	uint32_t pll_mode, pll_config;

	/* Disable PLL output. */
	pll_mode = readl(PLL2_MODE_REG);
	pll_mode &= ~B(0);
	writel(pll_mode, PLL2_MODE_REG);

	/* Assert active-low PLL reset. */
	pll_mode &= ~B(2);
	writel(pll_mode, PLL2_MODE_REG);

	/* Program L, M and N values. */
	writel(rate->l_val, PLL2_L_VAL_REG);
	writel(rate->m_val, PLL2_M_VAL_REG);
	writel(rate->n_val, PLL2_N_VAL_REG);

	/* Configure post-divide and VCO. */
	pll_config = readl(PLL2_CONFIG_REG);
	pll_config &= ~(BM(21, 20) | BM(17, 16));
	pll_config |= (BVAL(21, 20, rate->post_div));
	pll_config |= (BVAL(17, 16, rate->vco));
	writel(pll_config, PLL2_CONFIG_REG);

	/* Configure MND. */
	set_rate_mnd(clk, nf);

	/* De-assert active-low PLL reset. */
	pll_mode |= B(2);
	writel(pll_mode, PLL2_MODE_REG);

	/* Enable PLL output. */
	pll_mode |= B(0);
	writel(pll_mode, PLL2_MODE_REG);
}

static void set_rate_mnd_banked(struct clk_local *clk, struct clk_freq_tbl *nf)
{
	struct banked_mnd_masks *banks = clk->banked_mnd_masks;
	struct bank_mask_info *new_bank_masks, *old_bank_masks;
	uint32_t ns_reg_val, cc_reg_val;
	uint32_t bank_sel;

	/* Determine active bank and program the other one. If the clock is
	 * off, program the active bank since bank switching won't work if
	 * both banks aren't running. */
	cc_reg_val = readl(clk->cc_reg);
	bank_sel = !!(cc_reg_val & banks->bank_sel_mask);
	 /* If clock is disabled, don't switch banks. */
	bank_sel ^= !(clk->count);
	if (bank_sel == 0) {
		new_bank_masks = &banks->bank1_mask;
		old_bank_masks = &banks->bank0_mask;
	} else {
		new_bank_masks = &banks->bank0_mask;
		old_bank_masks = &banks->bank1_mask;
	}

	ns_reg_val = readl(clk->ns_reg);

	/* Assert bank MND reset. */
	ns_reg_val |= new_bank_masks->rst_mask;
	writel(ns_reg_val, clk->ns_reg);

	writel(nf->md_val, new_bank_masks->md_reg);

	/* Enable counter only if clock is enabled. */
	if (clk->count)
		cc_reg_val |= new_bank_masks->mnd_en_mask;
	else
		cc_reg_val &= ~(new_bank_masks->mnd_en_mask);

	cc_reg_val &= ~(new_bank_masks->mode_mask);
	cc_reg_val |= (nf->cc_val & new_bank_masks->mode_mask);
	writel(cc_reg_val, clk->cc_reg);

	ns_reg_val &= ~(new_bank_masks->ns_mask);
	ns_reg_val |= (nf->ns_val & new_bank_masks->ns_mask);
	writel(ns_reg_val, clk->ns_reg);

	/* Deassert bank MND reset. */
	ns_reg_val &= ~(new_bank_masks->rst_mask);
	writel(ns_reg_val, clk->ns_reg);

	/* Switch to the new bank if clock is on.  If it isn't, then no
	 * switch is necessary since we programmed the active bank. */
	if (clk->count) {
		cc_reg_val ^= banks->bank_sel_mask;
		writel(cc_reg_val, clk->cc_reg);

		/* Disable previous MN counter. */
		cc_reg_val &= ~(old_bank_masks->mnd_en_mask);
		writel(cc_reg_val, clk->cc_reg);
	}

	/* If this freq requires the MN counter to be enabled,
	 * update the enable mask to match the current bank. */
	if (nf->mnd_en_mask)
		nf->mnd_en_mask = new_bank_masks->mnd_en_mask;
}

static void set_rate_div_banked(struct clk_local *clk, struct clk_freq_tbl *nf)
{
	uint32_t ns_reg_val, ns_mask, bank_sel;

	/* Determine active bank and program the other one. If the clock is
	 * off, program the active bank since bank switching won't work if
	 * both banks aren't running. */
	ns_reg_val = readl(clk->ns_reg);
	bank_sel = !!(ns_reg_val & B(30));
	 /* If clock is disabled, don't switch banks. */
	bank_sel ^= !(clk->count);
	if (bank_sel == 0)
		ns_mask = (BM(29, 26) | BM(21, 19));
	else
		ns_mask = (BM(25, 22) | BM(18, 16));

	ns_reg_val &= ~(ns_mask);
	ns_reg_val |= (nf->ns_val & ns_mask);
	writel(ns_reg_val, clk->ns_reg);

	/* Switch to the new bank if clock is on.  If it isn't, then no
	 * switch is necessary since we programmed the active bank. */
	if (clk->count) {
		ns_reg_val ^= B(30);
		writel(ns_reg_val, clk->ns_reg);
	}
}

static void set_rate_nop(struct clk_local *clk, struct clk_freq_tbl *nf)
{
	/* Nothing to do for fixed-rate clocks. */
}

/*
 * Generic clock declaration macros
 */
#define CLK_NORATE(id, reg, br, h_r, h_c, h_b, tv) \
		CLK_LOCAL(id, NORATE, NULL, reg, NULL, NULL, 0, h_r, h_c, h_b, \
				br, 0, 0, 0, NULL, NULL, NULL, NONE, NULL, tv)

#define CLK_SLAVE(id, reg, br, h_r, h_c, h_b, par, tv) \
		CLK_LOCAL(id, NORATE, NULL, reg, NULL, NULL, 0, h_r, h_c, h_b, \
				br, 0, 0, 0, NULL, NULL, NULL, par, NULL, tv)

#define CLK_RESET(id, ns, res) \
		CLK_LOCAL(id, RESET, NULL, NULL, NULL, ns, res, NULL, 0, 0, \
				0, 0, 0, 0, NULL, NULL, NULL, NONE, NULL, 0)

#define CLK_SLAVE_RSET(id, reg, br, r_reg, res, h_r, h_c, h_b, par, tv) \
		CLK_LOCAL(id, NORATE, NULL, reg, NULL, r_reg, res, h_r, h_c, \
				h_b, br, 0, 0, 0, NULL, NULL, NULL, par, NULL, \
				tv)

#define CLK_NORATE_MM(id, reg, br, h_r, h_c, h_b, tv) \
		CLK_LOCAL(id, NORATE, NULL, reg, NULL, NULL, 0, h_r, h_c, h_b, \
				br, 0, 0, 0, NULL, NULL, NULL, NONE, NULL, tv)

#define CLK_SLAVE_MM(id, reg, br, h_r, h_c, h_b, par, tv) \
		CLK_LOCAL(id, NORATE, NULL, reg, NULL, NULL, 0, h_r, h_c, h_b, \
				br, 0, 0, 0, NULL, NULL, NULL, par, NULL, tv)

#define CLK_SLAVE_LPA(id, reg, br, h_r, h_c, h_b, par, tv) \
		CLK_LOCAL(id, NORATE, NULL, reg, NULL, NULL, 0, h_r, h_c, h_b, \
				br, 0, 0, 0, NULL, NULL, NULL, par, NULL, tv)

/*
 * Register value macros
 */
#define MN_MODE_DUAL_EDGE 0x2
#define MND_EN(b, n) (b * !!(n))

/* MD Registers */
#define MD4(m_lsb, m, n_lsb, n)	\
		(BVAL((m_lsb+3), m_lsb, m) | BVAL((n_lsb+3), n_lsb, ~(n)))
#define MD8(m_lsb, m, n_lsb, n)	\
		(BVAL((m_lsb+7), m_lsb, m) | BVAL((n_lsb+7), n_lsb, ~(n)))
#define MD16(m, n) (BVAL(31, 16, m) | BVAL(15, 0, ~(n)))

/* NS Registers */
#define NS(n_msb, n_lsb, n, m, mde_lsb, d_msb, d_lsb, d, s_msb, s_lsb, s) \
		(BVAL(n_msb, n_lsb, ~(n-m)) \
		| (BVAL((mde_lsb+1), mde_lsb, MN_MODE_DUAL_EDGE) * !!(n)) \
		| BVAL(d_msb, d_lsb, (d-1)) | BVAL(s_msb, s_lsb, s##_SRC))

#define NS_MM(n_msb, n_lsb, n, m, d_msb, d_lsb, d, s_msb, s_lsb, s) \
		(BVAL(n_msb, n_lsb, ~(n-m)) | BVAL(d_msb, d_lsb, (d-1)) \
		| BVAL(s_msb, s_lsb, s##_SRC))

#define NS_DIVSRC(d_msb , d_lsb, d, s_msb, s_lsb, s) \
		(BVAL(d_msb, d_lsb, (d-1)) | BVAL(s_msb, s_lsb, s##_SRC))

#define NS_DIV(d_msb , d_lsb, d) \
		BVAL(d_msb, d_lsb, (d-1))

#define NS_SRC(s_msb, s_lsb, s) \
		BVAL(s_msb, s_lsb, s##_SRC)

#define NS_MND_BANKED4(n0_lsb, n1_lsb, n, m, s0_lsb, s1_lsb, s) \
		 (BVAL((n0_lsb+3), n0_lsb, ~(n-m)) \
		| BVAL((n1_lsb+3), n1_lsb, ~(n-m)) \
		| BVAL((s0_lsb+2), s0_lsb, s##_SRC) \
		| BVAL((s1_lsb+2), s1_lsb, s##_SRC))

#define NS_MND_BANKED8(n0_lsb, n1_lsb, n, m, s0_lsb, s1_lsb, s) \
		 (BVAL((n0_lsb+7), n0_lsb, ~(n-m)) \
		| BVAL((n1_lsb+7), n1_lsb, ~(n-m)) \
		| BVAL((s0_lsb+2), s0_lsb, s##_SRC) \
		| BVAL((s1_lsb+2), s1_lsb, s##_SRC))

#define NS_DIVSRC_BANKED(d0_msb, d0_lsb, d1_msb, d1_lsb, d, \
	s0_msb, s0_lsb, s1_msb, s1_lsb, s) \
		 (BVAL(d0_msb, d0_lsb, (d-1)) | BVAL(d1_msb, d1_lsb, (d-1)) \
		| BVAL(s0_msb, s0_lsb, s##_SRC) \
		| BVAL(s1_msb, s1_lsb, s##_SRC))

/* CC Registers */
#define CC(mde_lsb, n) (BVAL((mde_lsb+1), mde_lsb, MN_MODE_DUAL_EDGE) * !!(n))
#define CC_BANKED(mde0_lsb, mde1_lsb, n) \
		((BVAL((mde0_lsb+1), mde0_lsb, MN_MODE_DUAL_EDGE) \
		| BVAL((mde1_lsb+1), mde1_lsb, MN_MODE_DUAL_EDGE)) \
		* !!(n))

/*
 * Clock Descriptions
 */

/* BBRX_SSBI */
#define CLK_BBRX_SSBI(id, ns, h_r, h_c, h_b, tv) \
		CLK_LOCAL(id, BASIC, ns, ns, NULL, NULL, 0, h_r, h_c, h_b, \
				B(4), 0, 0, 0, set_rate_nop, \
				clk_tbl_bbrx_ssbi, NULL, NONE, NULL, tv)
#define F_BBRX_SSBI(f, s, d, m, n) \
		F_RAW(f, s##_PLL, 0, 0, 0, 0)
static struct clk_freq_tbl clk_tbl_bbrx_ssbi[] = {
	F_BBRX_SSBI(19200000, NONE, 0, 0, 0),
	F_END,
};

/* GSBI_UART */
#define NS_MASK_GSBI_UART (BM(31, 16) | BM(6, 0))
#define CLK_GSBI_UART(id, ns, h_r, h_c, h_b, tv) \
		CLK_LOCAL(id, MND, ns, ns, (ns-4), (ns+8), B(0), h_r, h_c, \
				h_b, B(9), B(11), NS_MASK_GSBI_UART, 0, \
				set_rate_mnd, clk_tbl_gsbi_uart, NULL, NONE, \
				NULL, tv)
#define F_GSBI_UART(f, s, d, m, n) \
		F_RAW(f, s##_PLL, MD16(m, n), \
			NS(31, 16, n, m, 5, 4, 3, d, 2, 0, s), \
			0, MND_EN(B(8), n))
static struct clk_freq_tbl clk_tbl_gsbi_uart[] = {
	F_GSBI_UART( 3686400, BB_PLL8, 1,  6, 625),
	F_GSBI_UART( 7372800, BB_PLL8, 1, 12, 625),
	F_GSBI_UART(14745600, BB_PLL8, 1, 24, 625),
	F_GSBI_UART(16000000, BB_PLL8, 4,  1,   6),
	F_GSBI_UART(24000000, BB_PLL8, 4,  1,   4),
	F_GSBI_UART(32000000, BB_PLL8, 4,  1,   3),
	F_GSBI_UART(40000000, BB_PLL8, 1,  5,  48),
	F_GSBI_UART(46400000, BB_PLL8, 1, 29, 240),
	F_GSBI_UART(48000000, BB_PLL8, 4,  1,   2),
	F_GSBI_UART(51200000, BB_PLL8, 1,  2,  15),
	F_GSBI_UART(48000000, BB_PLL8, 4,  1,   2),
	F_GSBI_UART(51200000, BB_PLL8, 1,  2,  15),
	F_GSBI_UART(56000000, BB_PLL8, 1,  7,  48),
	F_GSBI_UART(58982400, BB_PLL8, 1, 96, 625),
	F_GSBI_UART(64000000, BB_PLL8, 2,  1,   3),
	F_END,
};

/* GSBI_QUP */
#define NS_MASK_GSBI_QUP (BM(23, 16) | BM(6, 0))
#define CLK_GSBI_QUP(id, ns, h_r, h_c, h_b, tv) \
		CLK_LOCAL(id, MND, ns, ns, (ns-4), (ns+16), B(0), h_r, h_c, \
				h_b, B(9), B(11), NS_MASK_GSBI_QUP, 0, \
				set_rate_mnd, clk_tbl_gsbi_qup, NULL, NONE, \
				NULL, tv)
#define F_GSBI_QUP(f, s, d, m, n) \
		F_RAW(f, s##_PLL, MD8(16, m, 0, n), \
			NS(23, 16, n, m, 5, 4, 3, d, 2, 0, s), \
			0, MND_EN(B(8), n))
static struct clk_freq_tbl clk_tbl_gsbi_qup[] = {
	F_GSBI_QUP( 1100000, BB_MXO,  1, 2, 49),
	F_GSBI_QUP( 5400000, BB_MXO,  1, 1,  5),
	F_GSBI_QUP(10800000, BB_MXO,  1, 2,  5),
	F_GSBI_QUP(15060000, BB_PLL8, 1, 2, 51),
	F_GSBI_QUP(24000000, BB_PLL8, 4, 1,  4),
	F_GSBI_QUP(25600000, BB_PLL8, 1, 1, 15),
	F_GSBI_QUP(48000000, BB_PLL8, 4, 1,  2),
	F_GSBI_QUP(51200000, BB_PLL8, 1, 2, 15),
	F_END,
};

/* GSBI_SIM */
#define NS_MASK_GSBI_SIM (BM(6, 3) | BM(1, 0))
#define CLK_GSBI_SIM(id, ns) \
		CLK_LOCAL(id, BASIC, ns, ns, NULL, NULL, 0, NULL, 0, 0, 0, \
				B(11), NS_MASK_GSBI_SIM, 0, set_rate_basic, \
				clk_tbl_gsbi_sim, NULL, NONE, \
				chld_gsbi_sim_src, 0)
#define F_GSBI_SIM(f, s, d, m, n) \
		F_RAW(f, s##_PLL, 0, NS_DIVSRC(6, 3, d, 1, 0, s), 0, 0)
static struct clk_freq_tbl clk_tbl_gsbi_sim[] = {
	F_GSBI_SIM(3860000, XO_MXO, 7, 0, 0),
	F_END,
};

/* PDM */
#define NS_MASK_PDM (BM(1, 0))
#define CLK_PDM(id, ns, h_r, h_c, h_b) \
		CLK_LOCAL(id, BASIC, ns, ns, NULL, ns, B(12), h_r, h_c, h_b, \
				B(9), B(11)|B(15), NS_MASK_PDM, 0, \
				set_rate_basic, clk_tbl_pdm, NULL, NONE, \
				NULL, 0)
#define F_PDM(f, s, d, m, n) \
		F_RAW(f, s##_PLL, 0, NS_SRC(1, 0, s), 0, 0)
static struct clk_freq_tbl clk_tbl_pdm[] = {
	F_PDM(27000000, XO_MXO, 1, 0, 0),
	F_END,
};

/* PRNG */
#define NS_MASK_PRNG (BM(6, 3) | BM(2, 0))
#define CLK_PRNG(id, ns, h_r, h_c, h_b, tv) \
		CLK_LOCAL(id, BASIC, ns, ns, NULL, ns, B(12), h_r, h_c, h_b, \
				0, B(11), NS_MASK_PRNG, 0, set_rate_basic, \
				clk_tbl_prng, NULL, NONE, NULL, tv)
#define F_PRNG(f, s, d, m, n) \
		F_RAW(f, s##_PLL, 0, NS_DIVSRC(6, 3, d, 2, 0, s), 0, 0)
static struct clk_freq_tbl clk_tbl_prng[] = {
	F_PRNG(32000000, BB_PLL8, 12, 0, 0),
	F_PRNG(64000000, BB_PLL8,  6, 0, 0),
	F_END,
};

/* SDC */
#define NS_MASK_SDC (BM(23, 16) | BM(6, 0))
#define CLK_SDC(id, ns, h_r, h_c, h_b, tv) \
		CLK_LOCAL(id, MND, ns, ns, (ns-4), (ns+4), B(0), h_r, h_c, \
				h_b, B(9), B(11), NS_MASK_SDC, 0, \
				set_rate_mnd, clk_tbl_sdc, NULL, NONE, NULL, tv)
#define F_SDC(f, s, d, m, n) \
		F_RAW(f, s##_PLL, MD8(16, m, 0, n), \
			NS(23, 16, n, m, 5, 4, 3, d, 2, 0, s), \
			0, MND_EN(B(8), n))
static struct clk_freq_tbl clk_tbl_sdc[] = {
	F_SDC(  400000, BB_PLL8,  4, 1, 240),
	F_SDC(16000000, BB_PLL8,  4, 1,   6),
	F_SDC(17070000, BB_PLL8,  1, 2,  45),
	F_SDC(20210000, BB_PLL8,  1, 1,  19),
	F_SDC(24000000, BB_PLL8,  4, 1,   4),
	F_SDC(48000000, BB_PLL8,  4, 1,   2),
	F_END,
};

/* TSIF_REF */
#define NS_MASK_TSIF_REF (BM(31, 16) | BM(6, 0))
#define CLK_TSIF_REF(id, ns, h_r, h_c, h_b, tv) \
		CLK_LOCAL(id, MND, ns, ns, (ns-4), NULL, 0, h_r, h_c, h_b, \
				B(9), B(11), NS_MASK_TSIF_REF, 0, \
				set_rate_mnd, clk_tbl_tsif_ref, NULL, \
				NONE, NULL, tv)
#define F_TSIF_REF(f, s, d, m, n) \
		F_RAW(f, s##_PLL, MD16(m, n), \
			NS(31, 16, n, m, 5, 4, 3, d, 2, 0, s), \
			0, MND_EN(B(8), n))
static struct clk_freq_tbl clk_tbl_tsif_ref[] = {
	F_TSIF_REF(105000, BB_MXO, 1, 1, 256),
	F_END,
};


/* TSSC */
#define NS_MASK_TSSC (BM(1, 0))
#define CLK_TSSC(id, ns, h_r, h_c, h_b, tv) \
		CLK_LOCAL(id, BASIC, ns, ns, NULL, NULL, 0, h_r, h_c, h_b, \
				B(4), B(11), NS_MASK_TSSC, 0, set_rate_basic, \
				clk_tbl_tssc, NULL, NONE, NULL, tv)
#define F_TSSC(f, s, d, m, n) \
		F_RAW(f, s##_PLL, 0, NS_SRC(1, 0, s), 0, 0)
static struct clk_freq_tbl clk_tbl_tssc[] = {
	F_TSSC(27000000, XO_MXO, 0, 0, 0),
	F_END,
};

/* USB_HS and USB_FS */
#define NS_MASK_USB (BM(23, 16) | BM(6, 0))
#define CLK_USB_HS(id, ns, h_r, h_c, h_b, tv) \
		CLK_LOCAL(id, MND, ns, ns, (ns-4), (ns+4), B(0), h_r, h_c, \
				h_b, B(9), B(11), NS_MASK_USB, 0, \
				set_rate_mnd, clk_tbl_usb, NULL, NONE, NULL, tv)
#define CLK_USB_FS(id, ns, chld_lst) \
		CLK_LOCAL(id, MND, ns, ns, (ns-4), NULL, 0, NULL, 0, 0, \
				0, B(11), NS_MASK_USB, 0, set_rate_mnd, \
				clk_tbl_usb, NULL, NONE, chld_lst, 0)
#define F_USB(f, s, d, m, n) \
		F_RAW(f, s##_PLL, MD8(16, m, 0, n), \
			NS(23, 16, n, m, 5, 4, 3, d, 2, 0, s), \
			0, MND_EN(B(8), n))
static struct clk_freq_tbl clk_tbl_usb[] = {
	F_USB(60000000, BB_PLL8, 1, 5, 32),
	F_END,
};

/* CAM */
#define NS_MASK_CAM (BM(31, 24) | BM(15, 14) | BM(2, 0))
#define CC_MASK_CAM (BM(7, 6))
#define CLK_CAM(id, ns, h_r, h_c, h_b, tv) \
		CLK_LOCAL(id, MND, ns, (ns-8), (ns-4), NULL, 0, h_r, h_c, h_b, \
				B(0), B(2), NS_MASK_CAM, CC_MASK_CAM, \
				set_rate_cam, clk_tbl_cam, NULL, NONE, NULL, tv)
#define F_CAM(f, s, d, m, n) \
		F_RAW(f, s##_PLL, MD8(8, m, 0, n), \
			NS_MM(31, 24, n, m, 15, 14, d, 2, 0, s), \
			CC(6, n), MND_EN(B(5), n))
static struct clk_freq_tbl clk_tbl_cam[] = {
	F_CAM(  6000000, MM_GPERF, 4, 1, 16),
	F_CAM(  8000000, MM_GPERF, 4, 1, 12),
	F_CAM( 12000000, MM_GPERF, 4, 1,  8),
	F_CAM( 16000000, MM_GPERF, 4, 1,  6),
	F_CAM( 19200000, MM_GPERF, 4, 1,  5),
	F_CAM( 24000000, MM_GPERF, 4, 1,  4),
	F_CAM( 32000000, MM_GPERF, 4, 1,  3),
	F_CAM( 48000000, MM_GPERF, 4, 1,  2),
	F_CAM( 64000000, MM_GPERF, 3, 1,  2),
	F_CAM( 96000000, MM_GPERF, 4, 0,  0),
	F_CAM(128000000, MM_GPERF, 3, 0,  0),
	F_END,
};

/* CSI */
#define NS_MASK_CSI (BM(15, 12) | BM(2, 0))
#define CLK_CSI(id, ns) \
		CLK_LOCAL(id, BASIC, ns, (ns-8), NULL, NULL, 0, NULL, 0, 0, \
				0, B(2), NS_MASK_CSI, 0, set_rate_basic, \
				clk_tbl_csi, NULL, NONE, chld_csi_src, 0)
#define F_CSI(f, s, d, m, n) \
		F_RAW(f, s##_PLL, 0, NS_DIVSRC(15, 12, d, 2, 0, s), 0, 0)
static struct clk_freq_tbl clk_tbl_csi[] = {
	F_CSI(192000000, MM_GPERF, 2, 0, 0),
	F_CSI(384000000, MM_GPERF, 1, 0, 0),
	F_END,
};

/* GFX2D0 and GFX2D1 */
struct banked_mnd_masks bmdn_info_gfx2d0 = {
	.bank_sel_mask =			B(11),
	.bank0_mask = {
			.md_reg = 		GFX2D0_MD0_REG,
			.ns_mask =	 	BM(23, 20) | BM(5, 3),
			.rst_mask =		B(25),
			.mnd_en_mask =		B(8),
			.mode_mask =		BM(10, 9),
	},
	.bank1_mask = {
			.md_reg = 		GFX2D0_MD1_REG,
			.ns_mask =		BM(19, 16) | BM(2, 0),
			.rst_mask =		B(24),
			.mnd_en_mask =		B(5),
			.mode_mask =		BM(7, 6),
	},
};
#define CLK_GFX2D0(id, ns, h_r, h_c, h_b, tv) \
		CLK_LOCAL(id, MND, ns, (ns-16), NULL, NULL, 0, h_r, h_c, h_b, \
				B(0), B(2), 0, 0, set_rate_mnd_banked, \
				clk_tbl_gfx2d, &bmdn_info_gfx2d0, NONE, \
				NULL, tv)
struct banked_mnd_masks bmdn_info_gfx2d1 = {
	.bank_sel_mask =		B(11),
	.bank0_mask = {
			.md_reg = 		GFX2D1_MD0_REG,
			.ns_mask =	 	BM(23, 20) | BM(5, 3),
			.rst_mask =		B(25),
			.mnd_en_mask =		B(8),
			.mode_mask =		BM(10, 9),
	},
	.bank1_mask = {
			.md_reg = 		GFX2D1_MD1_REG,
			.ns_mask =		BM(19, 16) | BM(2, 0),
			.rst_mask =		B(24),
			.mnd_en_mask =		B(5),
			.mode_mask =		BM(7, 6),
	},
};
#define CLK_GFX2D1(id, ns, h_r, h_c, h_b, tv) \
		CLK_LOCAL(id, MND, ns, (ns-8), NULL, NULL, 0, h_r, h_c, h_b, \
				B(0), B(2), 0, 0, set_rate_mnd_banked, \
				clk_tbl_gfx2d, &bmdn_info_gfx2d1, NONE, \
				NULL, tv)
#define F_GFX2D(f, s, d, m, n) \
		F_RAW(f, s##_PLL, MD4(4, m, 0, n), \
			NS_MND_BANKED4(20, 16, n, m, 3, 0, s), \
			CC_BANKED(9, 6, n), MND_EN((B(8) | B(5)), n))
static struct clk_freq_tbl clk_tbl_gfx2d[] = {
	F_GFX2D( 27000000, MM_MXO,   0, 0,  0),
	F_GFX2D( 48000000, MM_GPERF, 0, 1,  8),
	F_GFX2D( 54857000, MM_GPERF, 0, 1,  7),
	F_GFX2D( 64000000, MM_GPERF, 0, 1,  6),
	F_GFX2D( 76800000, MM_GPERF, 0, 1,  5),
	F_GFX2D( 96000000, MM_GPERF, 0, 1,  4),
	F_GFX2D(128000000, MM_GPERF, 0, 1,  3),
	F_GFX2D(145455000, MM_PLL1,  0, 2, 11),
	F_GFX2D(160000000, MM_PLL1,  0, 1,  5),
	F_GFX2D(177778000, MM_PLL1,  0, 2,  9),
	F_GFX2D(200000000, MM_PLL1,  0, 1,  4),
	F_GFX2D(228571000, MM_PLL1,  0, 2,  7),
	F_END,
};

/* GFX3D */
struct banked_mnd_masks bmdn_info_gfx3d = {
	.bank_sel_mask = 		B(11),
	.bank0_mask = {
			.md_reg = 		GFX3D_MD0_REG,
			.ns_mask =	 	BM(21, 18) | BM(5, 3),
			.rst_mask =		B(23),
			.mnd_en_mask =		B(8),
			.mode_mask =		BM(10, 9),
	},
	.bank1_mask = {
			.md_reg = 		GFX3D_MD1_REG,
			.ns_mask =		BM(17, 14) | BM(2, 0),
			.rst_mask =		B(22),
			.mnd_en_mask =		B(5),
			.mode_mask =		BM(7, 6),
	},
};
#define CLK_GFX3D(id, ns, h_r, h_c, h_b, par, tv) \
		CLK_LOCAL(id, MND, ns, (ns-12), NULL, NULL, 0, h_r, h_c, h_b, \
				B(0), B(2), 0, 0, set_rate_mnd_banked, \
				clk_tbl_gfx3d, &bmdn_info_gfx3d, par, NULL, tv)
#define F_GFX3D(f, s, d, m, n) \
		F_RAW(f, s##_PLL, MD4(4, m, 0, n), \
			NS_MND_BANKED4(18, 14, n, m, 3, 0, s), \
			CC_BANKED(9, 6, n), MND_EN((B(8) | B(5)), n))
static struct clk_freq_tbl clk_tbl_gfx3d[] = {
	F_GFX3D( 27000000, MM_MXO,   0, 0,  0),
	F_GFX3D( 48000000, MM_GPERF, 0, 1,  8),
	F_GFX3D( 54857000, MM_GPERF, 0, 1,  7),
	F_GFX3D( 64000000, MM_GPERF, 0, 1,  6),
	F_GFX3D( 76800000, MM_GPERF, 0, 1,  5),
	F_GFX3D( 96000000, MM_GPERF, 0, 1,  4),
	F_GFX3D(128000000, MM_GPERF, 0, 1,  3),
	F_GFX3D(145455000, MM_PLL1,  0, 2, 11),
	F_GFX3D(160000000, MM_PLL1,  0, 1,  5),
	F_GFX3D(177778000, MM_PLL1,  0, 2,  9),
	F_GFX3D(200000000, MM_PLL1,  0, 1,  4),
	F_GFX3D(228571000, MM_PLL1,  0, 2,  7),
	F_GFX3D(266667000, MM_PLL1,  0, 1,  3),
	F_GFX3D(320000000, MM_PLL1,  0, 2,  5),
	F_END,
};

/* IJPEG */
#define NS_MASK_IJPEG (BM(23, 16) | BM(15, 12) | BM(2, 0))
#define CC_MASK_IJPEG (BM(7, 6))
#define CLK_IJPEG(id, ns, h_r, h_c, h_b, tv) \
		CLK_LOCAL(id, MND, ns, (ns-8), (ns-4), NULL, 0, h_r, h_c, h_b, \
				B(0), B(2), NS_MASK_IJPEG, CC_MASK_IJPEG, \
				set_rate_mnd, clk_tbl_ijpeg, NULL, NONE, \
				NULL, tv)
#define F_IJPEG(f, s, d, m, n) \
		F_RAW(f, s##_PLL, MD8(8, m, 0, n), \
			NS_MM(23, 16, n, m, 15, 12, d, 2, 0, s), \
			CC(6, n), MND_EN(B(5), n))
static struct clk_freq_tbl clk_tbl_ijpeg[] = {
	F_IJPEG( 36570000, MM_GPERF, 1, 2, 21),
	F_IJPEG( 54860000, MM_GPERF, 7, 0,  0),
	F_IJPEG( 96000000, MM_GPERF, 4, 0,  0),
	F_IJPEG(109710000, MM_GPERF, 1, 2,  7),
	F_IJPEG(128000000, MM_GPERF, 3, 0,  0),
	F_IJPEG(153600000, MM_GPERF, 1, 2,  5),
	F_IJPEG(200000000, MM_PLL1,  4, 0,  0),
	F_IJPEG(228000000, MM_PLL1,  1, 2,  7),
	F_END,
};

/* JPEGD */
#define NS_MASK_JPEGD (BM(15, 12) | BM(2, 0))
#define CLK_JPEGD(id, ns, h_r, h_c, h_b, par, tv) \
		CLK_LOCAL(id, BASIC, ns, (ns-8), NULL, NULL, 0, h_r, h_c, h_b, \
				B(0), B(2), NS_MASK_JPEGD, 0, set_rate_basic, \
				clk_tbl_jpegd, NULL, par, NULL, tv)
#define F_JPEGD(f, s, d, m, n) \
		F_RAW(f, s##_PLL, 0, NS_DIVSRC(15, 12, d, 2, 0, s), 0, 0)
static struct clk_freq_tbl clk_tbl_jpegd[] = {
	F_JPEGD( 64000000, MM_GPERF, 6, 0, 0),
	F_JPEGD( 76800000, MM_GPERF, 5, 0, 0),
	F_JPEGD( 96000000, MM_GPERF, 4, 0, 0),
	F_JPEGD(160000000, MM_PLL1,  5, 0, 0),
	F_JPEGD(200000000, MM_PLL1,  4, 0, 0),
	F_END,
};

/* MDP */
struct banked_mnd_masks bmdn_info_mdp = {
	.bank_sel_mask =		B(11),
	.bank0_mask = {
			.md_reg = 		MDP_MD0_REG,
			.ns_mask =	 	BM(29, 22) | BM(5, 3),
			.rst_mask =		B(31),
			.mnd_en_mask =		B(8),
			.mode_mask =		BM(10, 9),
	},
	.bank1_mask = {
			.md_reg = 		MDP_MD1_REG,
			.ns_mask =		BM(21, 14) | BM(2, 0),
			.rst_mask =		B(30),
			.mnd_en_mask =		B(5),
			.mode_mask =		BM(7, 6),
	},
};
#define CLK_MDP(id, ns, h_r, h_c, h_b, tv) \
		CLK_LOCAL(id, MND, ns, (ns-16), NULL, NULL, 0, h_r, h_c, h_b, \
				B(0), B(2), 0, 0, set_rate_mnd_banked, \
				clk_tbl_mdp, &bmdn_info_mdp, NONE, NULL, tv)
#define F_MDP(f, s, d, m, n) \
		F_RAW(f, s##_PLL, MD8(8, m, 0, n), \
			NS_MND_BANKED8(22, 14, n, m, 3, 0, s), \
			CC_BANKED(9, 6, n), MND_EN((B(8) | B(5)), n))
static struct clk_freq_tbl clk_tbl_mdp[] = {
	F_MDP(  9600000, MM_GPERF, 0, 1, 40),
	F_MDP( 13710000, MM_GPERF, 0, 1, 28),
	F_MDP( 29540000, MM_GPERF, 0, 1, 13),
	F_MDP( 34910000, MM_GPERF, 0, 1, 11),
	F_MDP( 38400000, MM_GPERF, 0, 1, 10),
	F_MDP( 59080000, MM_GPERF, 0, 2, 13),
	F_MDP( 76800000, MM_GPERF, 0, 1,  5),
	F_MDP( 85330000, MM_GPERF, 0, 2,  9),
	F_MDP( 96000000, MM_GPERF, 0, 1,  4),
	F_MDP(128000000, MM_GPERF, 0, 1,  3),
	F_MDP(160000000, MM_PLL1,  0, 1,  5),
	F_MDP(200000000, MM_PLL1,  0, 1,  4),
	F_END,
};

/* MDP VSYNC */
#define NS_MASK_MDP_VSYNC BM(13, 13)
#define CLK_MDP_VSYNC(id, ns, h_r, h_c, h_b, tv) \
		CLK_LOCAL(id, BASIC, ns, (ns-4), NULL, NULL, 0, h_r, h_c, h_b, \
				B(6), 0, 0, 0, set_rate_basic, \
				clk_tbl_mdp_vsync, NULL, NONE, NULL, tv)
#define F_MDP_VSYNC(f, s, d, m, n) \
		F_RAW(f, s##_PLL, 0, NS_SRC(13, 13, s), 0, 0)
static struct clk_freq_tbl clk_tbl_mdp_vsync[] = {
	F_MDP_VSYNC(27000000, BB_MXO, 0, 0, 0),
	F_END,
};

/* PIXEL_MDP */
#define NS_MASK_PIXEL_MDP (BM(31, 16) | BM(15, 14) | BM(2, 0))
#define CC_MASK_PIXEL_MDP (BM(7, 6))
#define CLK_PIXEL_MDP(id, ns, h_r, h_c, h_b, tv) \
		CLK_LOCAL(id, MND, ns, (ns-8), (ns-4), NULL, 0, h_r, h_c, h_b, \
				B(0), B(2), NS_MASK_PIXEL_MDP, \
				CC_MASK_PIXEL_MDP, set_rate_mnd, \
				clk_tbl_pixel_mdp, NULL, NONE, \
				chld_pixel_mdp, tv)
#define F_PIXEL_MDP(f, s, d, m, n) \
		F_RAW(f, s##_PLL, MD16(m, n), \
			NS_MM(31, 16, n, m, 15, 14, d, 2, 0, s), \
			CC(6, n), MND_EN(B(5), n))
static struct clk_freq_tbl clk_tbl_pixel_mdp[] = {
	F_PIXEL_MDP(43192000, MM_GPERF, 1,  64, 569),
	F_PIXEL_MDP(48000000, MM_GPERF, 4,   1,   2),
	F_PIXEL_MDP(53990000, MM_GPERF, 2, 169, 601),
	F_END,
};

/* ROT */
#define CLK_ROT(id, ns, h_r, h_c, h_b, tv) \
		CLK_LOCAL(id, BASIC, ns, (ns-8), NULL, NULL, 0, h_r, h_c, h_b, \
				B(0), B(2), 0, 0, set_rate_div_banked, \
				clk_tbl_rot, NULL, NONE, NULL, tv)
#define F_ROT(f, s, d, m, n) \
		F_RAW(f, s##_PLL, 0, \
		NS_DIVSRC_BANKED(29, 26, 25, 22, d, 21, 19, 18, 16, s), 0, 0)
static struct clk_freq_tbl clk_tbl_rot[] = {
	F_ROT( 24580000, MM_PXO,    1, 0, 0),
	F_ROT( 27000000, MM_MXO,    1, 0, 0),
	F_ROT( 29540000, MM_GPERF, 13, 0, 0),
	F_ROT( 32000000, MM_GPERF, 12, 0, 0),
	F_ROT( 38400000, MM_GPERF, 10, 0, 0),
	F_ROT( 48000000, MM_GPERF,  8, 0, 0),
	F_ROT( 54860000, MM_GPERF,  7, 0, 0),
	F_ROT( 64000000, MM_GPERF,  6, 0, 0),
	F_ROT( 76800000, MM_GPERF,  5, 0, 0),
	F_ROT( 96000000, MM_GPERF,  4, 0, 0),
	F_ROT(100000000, MM_PLL1,   8, 0, 0),
	F_ROT(114290000, MM_PLL1,   7, 0, 0),
	F_ROT(133330000, MM_PLL1,   6, 0, 0),
	F_ROT(160000000, MM_PLL1,   5, 0, 0),
	F_END,
};

/* TV */
#define NS_MASK_TV (BM(23, 16) | BM(15, 14) | BM(2, 0))
#define CC_MASK_TV (BM(7, 6))
#define CLK_TV(id, ns) \
		CLK_LOCAL(id, MND, ns, (ns-8), (ns-4), NULL, 0, NULL, 0, 0, \
				0, B(2), NS_MASK_TV, CC_MASK_TV, set_rate_tv, \
				clk_tbl_tv, NULL, NONE, chld_tv_src, 0)
#define F_TV(f, s, p_r, d, m, n) \
		F_RAW_PLL(f, s##_PLL, MD8(8, m, 0, n), \
			NS_MM(23, 16, n, m, 15, 14, d, 2, 0, s), \
			CC(6, n), MND_EN(B(5), n), p_r)
/* Switching TV freqs requires PLL reconfiguration. */
static struct pll_rate mm_pll2_rate[] = {
	[0] = PLL_RATE( 50400500,  7, 6301, 13500, 0, 4),
	[1] = PLL_RATE( 54000000,  8,    0,     1, 0, 4),
	[2] = PLL_RATE( 54054000,  8,    1,   125, 0, 4),
	[3] = PLL_RATE(148500000, 22,    0,     1, 2, 4),
	[4] = PLL_RATE(297000000, 44,    0,     1, 2, 4),
};
static struct clk_freq_tbl clk_tbl_tv[] = {
	F_TV( 25200000, MM_PLL2, &mm_pll2_rate[0], 2, 0, 0),
	F_TV( 27000000, MM_PLL2, &mm_pll2_rate[1], 2, 0, 0),
	F_TV( 27030000, MM_PLL2, &mm_pll2_rate[2], 2, 0, 0),
	F_TV( 74250000, MM_PLL2, &mm_pll2_rate[3], 2, 0, 0),
	F_TV(148500000, MM_PLL2, &mm_pll2_rate[4], 2, 0, 0),
	F_END,
};

/* VCODEC */
struct banked_mnd_masks bmdn_info_vcodec = {
	.bank_sel_mask =		B(13),
	.bank0_mask = {
			.md_reg = 		VCODEC_MD0_REG,
			.ns_mask =	 	BM(18, 11) | BM(2, 0),
			.rst_mask =		B(31),
			.mnd_en_mask =		B(5),
			.mode_mask =		BM(7, 6),
	},
	.bank1_mask = {
			.md_reg = 		VCODEC_MD1_REG,
			.ns_mask =		BM(29, 27) | BM(26, 19),
			.rst_mask =		B(30),
			.mnd_en_mask =		B(10),
			.mode_mask =		BM(12, 11),
	},
};
#define CLK_VCODEC(id, ns, h_r, h_c, h_b, tv) \
		CLK_LOCAL(id, MND, ns, (ns-8), (ns-4), NULL, 0, h_r, h_c, h_b, \
				B(0), B(2), 0, 0, set_rate_mnd_banked, \
				clk_tbl_vcodec, &bmdn_info_vcodec, NONE, \
				NULL, tv)
#define F_VCODEC(f, s, d, m, n) \
		F_RAW(f, s##_PLL, MD8(8, m, 0, n), \
			NS_MND_BANKED8(11, 19, n, m, 0, 27, s), \
			CC_BANKED(6, 11, n), MND_EN((B(5) | B(10)), n))
static struct clk_freq_tbl clk_tbl_vcodec[] = {
	F_VCODEC( 24580000, MM_PXO,   0, 0,  0),
	F_VCODEC( 27000000, MM_MXO,   0, 0,  0),
	F_VCODEC( 32000000, MM_GPERF, 0, 1, 12),
	F_VCODEC( 48000000, MM_GPERF, 0, 1,  8),
	F_VCODEC( 54860000, MM_GPERF, 0, 1,  7),
	F_VCODEC( 96000000, MM_GPERF, 0, 1,  4),
	F_VCODEC(133330000, MM_PLL1,  0, 1,  6),
	F_VCODEC(200000000, MM_PLL1,  0, 1,  4),
	F_VCODEC(228570000, MM_PLL1,  0, 2,  7),
	F_END,
};

/* VPE */
#define NS_MASK_VPE (BM(15, 12) | BM(2, 0))
#define CLK_VPE(id, ns, h_r, h_c, h_b, tv) \
		CLK_LOCAL(id, BASIC, (ns), (ns-8), NULL, NULL, 0, h_r, h_c, \
				h_b, B(0), B(2), NS_MASK_VPE, 0, \
				set_rate_basic, clk_tbl_vpe, NULL, NONE, \
				NULL, tv)
#define F_VPE(f, s, d, m, n) \
		F_RAW(f, s##_PLL, 0, NS_DIVSRC(15, 12, d, 2, 0, s), 0, 0)
static struct clk_freq_tbl clk_tbl_vpe[] = {
	F_VPE( 24576000, MM_PXO,    1, 0, 0),
	F_VPE( 27000000, MM_MXO,    1, 0, 0),
	F_VPE( 34909000, MM_GPERF, 11, 0, 0),
	F_VPE( 38400000, MM_GPERF, 10, 0, 0),
	F_VPE( 64000000, MM_GPERF,  6, 0, 0),
	F_VPE( 76800000, MM_GPERF,  5, 0, 0),
	F_VPE( 96000000, MM_GPERF,  4, 0, 0),
	F_VPE(100000000, MM_PLL1,   8, 0, 0),
	F_VPE(160000000, MM_PLL1,   5, 0, 0),
	F_END,
};

/* VFE */
#define NS_MASK_VFE (BM(23, 16) | BM(11, 10) | BM(2, 0))
#define CC_MASK_VFE (BM(7, 6))
#define CLK_VFE(id, ns, h_r, h_c, h_b, par, tv) \
		CLK_LOCAL(id, MND, ns, (ns-8), (ns-4), NULL, 0, h_r, h_c, h_b, \
				B(0), B(2), NS_MASK_VFE, CC_MASK_VFE, \
				set_rate_mnd, clk_tbl_vfe, NULL, par, \
				chld_vfe, tv)
#define F_VFE(f, s, d, m, n) \
		F_RAW(f, s##_PLL, MD8(8, m, 0, n), \
			NS_MM(23, 16, n, m, 11, 10, d, 2, 0, s), \
			CC(6, n), MND_EN(B(5), n))
static struct clk_freq_tbl clk_tbl_vfe[] = {
	F_VFE( 13960000, MM_GPERF,  1, 2, 55),
	F_VFE( 36570000, MM_GPERF,  1, 2, 21),
	F_VFE( 38400000, MM_GPERF,  2, 1,  5),
	F_VFE( 45180000, MM_GPERF,  1, 2, 17),
	F_VFE( 48000000, MM_GPERF,  2, 1,  4),
	F_VFE( 54860000, MM_GPERF,  1, 1,  7),
	F_VFE( 64000000, MM_GPERF,  2, 1,  3),
	F_VFE( 76800000, MM_GPERF,  1, 1,  5),
	F_VFE( 96000000, MM_GPERF,  2, 1,  2),
	F_VFE(109710000, MM_GPERF,  1, 2,  7),
	F_VFE(128000000, MM_GPERF,  1, 1,  3),
	F_VFE(153600000, MM_GPERF,  2, 0,  0),
	F_VFE(200000000, MM_PLL1,   2, 1,  2),
	F_VFE(228570000, MM_PLL1,   1, 2,  7),
	F_END,
};

/* Audio Interface */
#define NS_MASK_AIF (BM(31, 24) | BM(6, 0))
#define CLK_AIF(id, ns, chld_lst) \
		CLK_LOCAL(id, MND, ns, ns, (ns+4), ns, B(19), NULL, 0, 0, \
				(B(15) | B(17)), B(9), NS_MASK_AIF, 0, \
				set_rate_mnd, clk_tbl_aif, NULL, NONE, \
				chld_lst, 0)
#define F_AIF(f, s, d, m, n) \
		F_RAW(f, s##_PLL, MD8(8, m, 0, n), \
			NS(31, 24, n, m, 5, 4, 3, d, 2, 0, s), \
			0, MND_EN(B(8), n))
static struct clk_freq_tbl clk_tbl_aif[] = {
	F_AIF(  512000, LPA_PLL0, 4, 1, 264),
	F_AIF(  768000, LPA_PLL0, 2, 1, 352),
	F_AIF( 1024000, LPA_PLL0, 4, 1, 132),
	F_AIF( 1536000, LPA_PLL0, 4, 1,  88),
	F_AIF( 2048000, LPA_PLL0, 4, 1,  66),
	F_AIF( 3072000, LPA_PLL0, 4, 1,  44),
	F_AIF( 4096000, LPA_PLL0, 4, 1,  33),
	F_AIF( 6144000, LPA_PLL0, 4, 1,  22),
	F_AIF( 8192000, LPA_PLL0, 2, 1,  33),
	F_AIF(12288000, LPA_PLL0, 4, 1,  11),
	F_AIF(24576000, LPA_PLL0, 2, 1,  11),
	F_END,
};

/* PCM */
#define NS_MASK_PCM (BM(31, 16) | BM(6, 0))
#define CLK_PCM(id, ns, h_r, h_c, h_b, tv) \
		CLK_LOCAL(id, MND, ns, ns, (ns+4), ns, B(13), h_r, h_c, h_b, \
				B(11), B(9), NS_MASK_PCM, 0, set_rate_mnd, \
				clk_tbl_pcm, NULL, NONE, NULL, tv)
#define F_PCM(f, s, d, m, n) \
		F_RAW(f, s##_PLL, MD16(m, n), \
			NS(31, 16, n, m, 5, 4, 3, d, 2, 0, s), \
			0, MND_EN(B(8), n))
static struct clk_freq_tbl clk_tbl_pcm[] = {
	F_PCM(  512000, LPA_PLL0, 4, 1, 264),
	F_PCM(  768000, LPA_PLL0, 2, 1, 352),
	F_PCM( 1024000, LPA_PLL0, 4, 1, 132),
	F_PCM( 1536000, LPA_PLL0, 4, 1,  88),
	F_PCM( 2048000, LPA_PLL0, 4, 1,  66),
	F_PCM( 3072000, LPA_PLL0, 4, 1,  44),
	F_PCM( 4096000, LPA_PLL0, 4, 1,  33),
	F_PCM( 6144000, LPA_PLL0, 4, 1,  22),
	F_PCM( 8192000, LPA_PLL0, 2, 1,  33),
	F_PCM(12288000, LPA_PLL0, 4, 1,  11),
	F_PCM(24580000, LPA_PLL0, 2, 1,  11),
	F_END,
};

static struct clk_freq_tbl dummy_freq = F_END;

static uint32_t pll_count[NUM_PLL];

static uint32_t chld_gsbi_sim_src[] = 	{C(GSBI1_SIM), C(GSBI2_SIM),
					 C(GSBI3_SIM), C(GSBI4_SIM),
					 C(GSBI4_SIM), C(GSBI5_SIM),
					 C(GSBI5_SIM), C(GSBI6_SIM),
					 C(GSBI7_SIM), C(GSBI8_SIM),
					 C(GSBI9_SIM), C(GSBI10_SIM),
					 C(GSBI11_SIM), C(GSBI12_SIM),
					 C(NONE)};
static uint32_t chld_usb_fs1_src[] = 	{C(USB_FS1_XCVR), C(USB_FS1_SYS),
					 C(NONE)};
static uint32_t chld_usb_fs2_src[] = 	{C(USB_FS2_XCVR), C(USB_FS2_SYS),
					 C(NONE)};
static uint32_t chld_csi_src[] = 	{C(CSI0), C(CSI1), C(NONE)};
static uint32_t chld_pixel_mdp[] = 	{C(PIXEL_LCDC), C(NONE)};
static uint32_t chld_tv_src[] =		{C(TV_ENC), C(TV_DAC), C(MDP_TV),
					 C(HDMI_TV), C(DSUB_TV), C(NONE)};
static uint32_t chld_vfe[] =		{C(CSI0_VFE), C(CSI1_VFE), C(NONE)};
static uint32_t chld_mi2s_src[] =	{C(MI2S), C(MI2S_M), C(NONE)};
static uint32_t chld_codec_i2s_mic_src[] =	{C(CODEC_I2S_MIC),
						 C(CODEC_I2S_MIC_M), C(NONE)};
static uint32_t chld_codec_i2s_spkr_src[] =	{C(CODEC_I2S_SPKR),
						 C(CODEC_I2S_SPKR_M), C(NONE)};
static uint32_t chld_spare_i2s_mic_src[] =	{C(SPARE_I2S_MIC),
						 C(SPARE_I2S_MIC_M), C(NONE)};
static uint32_t chld_spare_i2s_spkr_src[] =	{C(SPARE_I2S_SPKR),
						 C(SPARE_I2S_SPKR_M), C(NONE)};

/* Test Vector Macros */
#define TEST_TYPE_PER		1
#define TEST_TYPE_MMLS		2
#define TEST_TYPE_MMHS		3
#define TEST_TYPE_LPA		4

#define TEST_TYPE_SHIFT		8
#define TEST_VECTOR_MASK	BM(7, 0)
#define TEST_PER(v)	((TEST_TYPE_PER << TEST_TYPE_SHIFT)  | BVAL(7, 0, v))
#define TEST_MMLS(v)	((TEST_TYPE_MMLS << TEST_TYPE_SHIFT) | BVAL(7, 0, v))
#define TEST_MMHS(v)	((TEST_TYPE_MMHS << TEST_TYPE_SHIFT) | BVAL(7, 0, v))
#define TEST_LPA(v)	((TEST_TYPE_LPA << TEST_TYPE_SHIFT)  | BVAL(7, 0, v))

/* Halt/Status Checking Mode Macros */
#define NOCHECK 0	/* No bit to check, do nothing */
#define HALT	1	/* Bit polarity: 1 = halted */
#define ENABLE	2	/* Bit polarity: 1 = running */
#define DELAY	3	/* No bit to check, just delay */

static struct clk_local clk_local_tbl[] = {

	/*
	 * Peripheral Clocks
	 */
	CLK_BBRX_SSBI(BBRX_SSBI, BBRX_SSBI_CLK_CTL_REG,
		CLK_HALT_SFPB_MISC_STATE_REG, HALT, 8, TEST_PER(0x6E)),

	CLK_GSBI_UART(GSBI1_UART,  GSBIn_UART_APPS_NS_REG(1),
		CLK_HALT_CFPB_STATEA_REG, HALT, 10, TEST_PER(0x3E)),
	CLK_GSBI_UART(GSBI2_UART,  GSBIn_UART_APPS_NS_REG(2),
		CLK_HALT_CFPB_STATEA_REG, HALT,  6, TEST_PER(0x42)),
	CLK_GSBI_UART(GSBI3_UART,  GSBIn_UART_APPS_NS_REG(3),
		CLK_HALT_CFPB_STATEA_REG, HALT,  2, TEST_PER(0x46)),
	CLK_GSBI_UART(GSBI4_UART,  GSBIn_UART_APPS_NS_REG(4),
		CLK_HALT_CFPB_STATEB_REG, HALT, 26, TEST_PER(0x4A)),
	CLK_GSBI_UART(GSBI5_UART,  GSBIn_UART_APPS_NS_REG(5),
		CLK_HALT_CFPB_STATEB_REG, HALT, 22, TEST_PER(0x4E)),
	CLK_GSBI_UART(GSBI6_UART,  GSBIn_UART_APPS_NS_REG(6),
		CLK_HALT_CFPB_STATEB_REG, HALT, 18, TEST_PER(0x52)),
	CLK_GSBI_UART(GSBI7_UART,  GSBIn_UART_APPS_NS_REG(7),
		CLK_HALT_CFPB_STATEB_REG, HALT, 14, TEST_PER(0x56)),
	CLK_GSBI_UART(GSBI8_UART,  GSBIn_UART_APPS_NS_REG(8),
		CLK_HALT_CFPB_STATEB_REG, HALT, 10, TEST_PER(0x5A)),
	CLK_GSBI_UART(GSBI9_UART,  GSBIn_UART_APPS_NS_REG(9),
		CLK_HALT_CFPB_STATEB_REG, HALT,  6, TEST_PER(0x5E)),
	CLK_GSBI_UART(GSBI10_UART, GSBIn_UART_APPS_NS_REG(10),
		CLK_HALT_CFPB_STATEB_REG, HALT,  2, TEST_PER(0x62)),
	CLK_GSBI_UART(GSBI11_UART, GSBIn_UART_APPS_NS_REG(11),
		CLK_HALT_CFPB_STATEC_REG, HALT, 17, TEST_PER(0x66)),
	CLK_GSBI_UART(GSBI12_UART, GSBIn_UART_APPS_NS_REG(12),
		CLK_HALT_CFPB_STATEC_REG, HALT, 13, TEST_PER(0x6A)),

	CLK_GSBI_QUP(GSBI1_QUP,  GSBIn_QUP_APPS_NS_REG(1),
		CLK_HALT_CFPB_STATEA_REG, HALT,  9, TEST_PER(0x3F)),
	CLK_GSBI_QUP(GSBI2_QUP,  GSBIn_QUP_APPS_NS_REG(2),
		CLK_HALT_CFPB_STATEA_REG, HALT,  4, TEST_PER(0x44)),
	CLK_GSBI_QUP(GSBI3_QUP,  GSBIn_QUP_APPS_NS_REG(3),
		CLK_HALT_CFPB_STATEA_REG, HALT,  0, TEST_PER(0x48)),
	CLK_GSBI_QUP(GSBI4_QUP,  GSBIn_QUP_APPS_NS_REG(4),
		CLK_HALT_CFPB_STATEB_REG, HALT, 24, TEST_PER(0x4C)),
	CLK_GSBI_QUP(GSBI5_QUP,  GSBIn_QUP_APPS_NS_REG(5),
		CLK_HALT_CFPB_STATEB_REG, HALT, 20, TEST_PER(0x50)),
	CLK_GSBI_QUP(GSBI6_QUP,  GSBIn_QUP_APPS_NS_REG(6),
		CLK_HALT_CFPB_STATEB_REG, HALT, 16, TEST_PER(0x54)),
	CLK_GSBI_QUP(GSBI7_QUP,  GSBIn_QUP_APPS_NS_REG(7),
		CLK_HALT_CFPB_STATEB_REG, HALT, 12, TEST_PER(0x58)),
	CLK_GSBI_QUP(GSBI8_QUP,  GSBIn_QUP_APPS_NS_REG(8),
		CLK_HALT_CFPB_STATEB_REG, HALT,  8, TEST_PER(0x5C)),
	CLK_GSBI_QUP(GSBI9_QUP,  GSBIn_QUP_APPS_NS_REG(9),
		CLK_HALT_CFPB_STATEB_REG, HALT,  4, TEST_PER(0x60)),
	CLK_GSBI_QUP(GSBI10_QUP, GSBIn_QUP_APPS_NS_REG(10),
		CLK_HALT_CFPB_STATEB_REG, HALT,  0, TEST_PER(0x64)),
	CLK_GSBI_QUP(GSBI11_QUP, GSBIn_QUP_APPS_NS_REG(11),
		CLK_HALT_CFPB_STATEC_REG, HALT, 15, TEST_PER(0x68)),
	CLK_GSBI_QUP(GSBI12_QUP, GSBIn_QUP_APPS_NS_REG(12),
		CLK_HALT_CFPB_STATEC_REG, HALT, 11, TEST_PER(0x6C)),

	CLK_GSBI_SIM(GSBI_SIM_SRC, GSBI_COMMON_SIM_CLK_NS_REG),
	CLK_SLAVE_RSET(GSBI1_SIM,  GSBIn_SIM_CLK_CTL_REG(1), B(4),
		GSBIn_RESET_REG(1), B(0), CLK_HALT_CFPB_STATEA_REG,
		HALT, 8, GSBI_SIM_SRC,  TEST_PER(0x40)),
	CLK_SLAVE_RSET(GSBI2_SIM,  GSBIn_SIM_CLK_CTL_REG(2), B(4),
		GSBIn_RESET_REG(2), B(0), CLK_HALT_CFPB_STATEA_REG,
		HALT, 5, GSBI_SIM_SRC,  TEST_PER(0x43)),
	CLK_SLAVE_RSET(GSBI3_SIM,  GSBIn_SIM_CLK_CTL_REG(3), B(4),
		GSBIn_RESET_REG(3), B(0), CLK_HALT_CFPB_STATEA_REG,
		HALT, 1, GSBI_SIM_SRC,   TEST_PER(0x47)),
	CLK_SLAVE_RSET(GSBI4_SIM,  GSBIn_SIM_CLK_CTL_REG(4), B(4),
		GSBIn_RESET_REG(4), B(0), CLK_HALT_CFPB_STATEB_REG,
		HALT, 25, GSBI_SIM_SRC,   TEST_PER(0x4B)),
	CLK_SLAVE_RSET(GSBI5_SIM,  GSBIn_SIM_CLK_CTL_REG(5), B(4),
		GSBIn_RESET_REG(5), B(0), CLK_HALT_CFPB_STATEB_REG,
		HALT, 21, GSBI_SIM_SRC, TEST_PER(0x4F)),
	CLK_SLAVE_RSET(GSBI6_SIM,  GSBIn_SIM_CLK_CTL_REG(6), B(4),
		GSBIn_RESET_REG(6), B(0), CLK_HALT_CFPB_STATEB_REG,
		HALT, 17, GSBI_SIM_SRC, TEST_PER(0x53)),
	CLK_SLAVE_RSET(GSBI7_SIM,  GSBIn_SIM_CLK_CTL_REG(7), B(4),
		GSBIn_RESET_REG(7), B(0), CLK_HALT_CFPB_STATEB_REG,
		HALT, 13, GSBI_SIM_SRC, TEST_PER(0x57)),
	CLK_SLAVE_RSET(GSBI8_SIM,  GSBIn_SIM_CLK_CTL_REG(8), B(4),
		GSBIn_RESET_REG(8), B(0), CLK_HALT_CFPB_STATEB_REG,
		HALT, 9, GSBI_SIM_SRC, TEST_PER(0x5B)),
	CLK_SLAVE_RSET(GSBI9_SIM,  GSBIn_SIM_CLK_CTL_REG(9), B(4),
		GSBIn_RESET_REG(9), B(0), CLK_HALT_CFPB_STATEB_REG,
		HALT, 5, GSBI_SIM_SRC, TEST_PER(0x5F)),
	CLK_SLAVE_RSET(GSBI10_SIM, GSBIn_SIM_CLK_CTL_REG(10), B(4),
		GSBIn_RESET_REG(10), B(0), CLK_HALT_CFPB_STATEB_REG,
		HALT, 1, GSBI_SIM_SRC, TEST_PER(0x63)),
	CLK_SLAVE_RSET(GSBI11_SIM, GSBIn_SIM_CLK_CTL_REG(11), B(4),
		GSBIn_RESET_REG(11), B(0), CLK_HALT_CFPB_STATEC_REG,
		HALT, 16, GSBI_SIM_SRC, TEST_PER(0x67)),
	CLK_SLAVE_RSET(GSBI12_SIM, GSBIn_SIM_CLK_CTL_REG(12), B(4),
		GSBIn_RESET_REG(12), B(0), CLK_HALT_CFPB_STATEC_REG,
		HALT, 12, GSBI_SIM_SRC, TEST_PER(0x6B)),

	CLK_PDM(PDM, PDM_CLK_NS_REG, CLK_HALT_CFPB_STATEC_REG, HALT, 3),

	CLK_PRNG(PRNG, PRNG_CLK_NS_REG, CLK_HALT_SFPB_MISC_STATE_REG,
		HALT, 10, TEST_PER(0x7D)),

	CLK_SDC(SDC1, SDCn_APPS_CLK_NS_REG(1), CLK_HALT_DFAB_STATE_REG,
		HALT, 6,  TEST_PER(0x13)),
	CLK_SDC(SDC2, SDCn_APPS_CLK_NS_REG(2), CLK_HALT_DFAB_STATE_REG,
		HALT, 5,  TEST_PER(0x15)),
	CLK_SDC(SDC3, SDCn_APPS_CLK_NS_REG(3), CLK_HALT_DFAB_STATE_REG,
		HALT, 4,  TEST_PER(0x17)),
	CLK_SDC(SDC4, SDCn_APPS_CLK_NS_REG(4), CLK_HALT_DFAB_STATE_REG,
		HALT, 3,  TEST_PER(0x19)),
	CLK_SDC(SDC5, SDCn_APPS_CLK_NS_REG(5), CLK_HALT_DFAB_STATE_REG,
		HALT, 2,  TEST_PER(0x1B)),

	CLK_TSIF_REF(TSIF_REF, TSIF_REF_CLK_NS_REG,
		CLK_HALT_CFPB_STATEC_REG, HALT, 5, TEST_PER(0x91)),

	CLK_TSSC(TSSC, TSSC_CLK_CTL_REG,
		CLK_HALT_CFPB_STATEC_REG, HALT, 4, TEST_PER(0x94)),

	CLK_USB_HS(USB_HS_XCVR,  USB_HS1_XCVR_FS_CLK_NS,
		CLK_HALT_DFAB_STATE_REG, HALT, 0, TEST_PER(0x95)),
	CLK_RESET(USB_PHY0, USB_PHY0_RESET_REG, B(0)),

	CLK_USB_FS(USB_FS1_SRC, USB_FS1_XCVR_FS_CLK_NS_REG, chld_usb_fs1_src),
	CLK_SLAVE_RSET(USB_FS1_XCVR, USB_FS1_XCVR_FS_CLK_NS_REG, B(9),
			USB_FS1_RESET_REG, B(1), CLK_HALT_CFPB_STATEA_REG,
			HALT, 15, USB_FS1_SRC, TEST_PER(0x8B)),
	CLK_SLAVE_RSET(USB_FS1_SYS, USB_FS1_SYSTEM_CLK_CTL_REG, B(4),
			USB_FS1_RESET_REG, B(0), CLK_HALT_CFPB_STATEA_REG,
			HALT, 16, USB_FS1_SRC, TEST_PER(0x8A)),

	CLK_USB_FS(USB_FS2_SRC, USB_FS2_XCVR_FS_CLK_NS_REG, chld_usb_fs2_src),
	CLK_SLAVE_RSET(USB_FS2_XCVR,  USB_FS2_XCVR_FS_CLK_NS_REG, B(9),
			USB_FS2_RESET_REG, B(1), CLK_HALT_CFPB_STATEA_REG,
			HALT, 12, USB_FS2_SRC, TEST_PER(0x8E)),
	CLK_SLAVE_RSET(USB_FS2_SYS,   USB_FS2_SYSTEM_CLK_CLK_REG, B(4),
			USB_FS2_RESET_REG, B(0), CLK_HALT_CFPB_STATEA_REG,
			HALT, 13, USB_FS2_SRC, TEST_PER(0x8D)),

	/* Fast Peripheral Bus Clocks */
	CLK_NORATE(GSBI1_P,  GSBIn_HCLK_CTL_REG(1),  B(4),
		CLK_HALT_CFPB_STATEA_REG, HALT, 11, TEST_PER(0x3D)),
	CLK_NORATE(GSBI2_P,  GSBIn_HCLK_CTL_REG(2),  B(4),
		CLK_HALT_CFPB_STATEA_REG, HALT,  7, TEST_PER(0x41)),
	CLK_NORATE(GSBI3_P,  GSBIn_HCLK_CTL_REG(3),  B(4),
		CLK_HALT_CFPB_STATEA_REG, HALT, 3,  TEST_PER(0x45)),
	CLK_NORATE(GSBI4_P,  GSBIn_HCLK_CTL_REG(4),  B(4),
		CLK_HALT_CFPB_STATEB_REG, HALT, 27, TEST_PER(0x49)),
	CLK_NORATE(GSBI5_P,  GSBIn_HCLK_CTL_REG(5),  B(4),
		CLK_HALT_CFPB_STATEB_REG, HALT, 23, TEST_PER(0x4D)),
	CLK_NORATE(GSBI6_P,  GSBIn_HCLK_CTL_REG(6),  B(4),
		CLK_HALT_CFPB_STATEB_REG, HALT, 19, TEST_PER(0x51)),
	CLK_NORATE(GSBI7_P,  GSBIn_HCLK_CTL_REG(7),  B(4),
		CLK_HALT_CFPB_STATEB_REG, HALT, 15, TEST_PER(0x55)),
	CLK_NORATE(GSBI8_P,  GSBIn_HCLK_CTL_REG(8),  B(4),
		CLK_HALT_CFPB_STATEB_REG, HALT, 11, TEST_PER(0x59)),
	CLK_NORATE(GSBI9_P,  GSBIn_HCLK_CTL_REG(9),  B(4),
		CLK_HALT_CFPB_STATEB_REG, HALT, 7,  TEST_PER(0x5D)),
	CLK_NORATE(GSBI10_P, GSBIn_HCLK_CTL_REG(10), B(4),
		CLK_HALT_CFPB_STATEB_REG, HALT, 3,  TEST_PER(0x61)),
	CLK_NORATE(GSBI11_P, GSBIn_HCLK_CTL_REG(11), B(4),
		CLK_HALT_CFPB_STATEC_REG, HALT, 18, TEST_PER(0x65)),
	CLK_NORATE(GSBI12_P, GSBIn_HCLK_CTL_REG(12), B(4),
		CLK_HALT_CFPB_STATEC_REG, HALT, 14, TEST_PER(0x69)),

	CLK_NORATE(TSIF_P, TSIF_HCLK_CTL_REG, B(4),
		CLK_HALT_CFPB_STATEC_REG, HALT, 7, TEST_PER(0x8F)),

	CLK_NORATE(USB_FS1_P, USB_FS1_HCLK_CTL_REG, B(4),
		CLK_HALT_CFPB_STATEA_REG, HALT, 17, TEST_PER(0x89)),
	CLK_NORATE(USB_FS2_P, USB_FS2_HCLK_CTL_REG, B(4),
		CLK_HALT_CFPB_STATEA_REG, HALT, 14, TEST_PER(0x8C)),

	/*
	 * Multimedia Clocks
	 */

	CLK_CAM(CAM, CAMCLK_NS_REG, NULL, DELAY, 0, TEST_MMLS(0x3B)),

	CLK_CSI(CSI_SRC, CSI_NS_REG),
	CLK_SLAVE_MM(CSI0, CSI_CC_REG, B(0), DBG_BUS_VEC_B_REG,
		HALT, 13, CSI_SRC, TEST_MMHS(0x01)),
	CLK_SLAVE_MM(CSI1, CSI_CC_REG, B(7), DBG_BUS_VEC_B_REG,
		HALT, 14, CSI_SRC, TEST_MMHS(0x03)),

	CLK_GFX2D0(GFX2D0, GFX2D0_NS_REG, DBG_BUS_VEC_A_REG,
		HALT, 9,  TEST_MMHS(0x0F)),
	CLK_GFX2D1(GFX2D1, GFX2D1_NS_REG, DBG_BUS_VEC_A_REG,
		HALT, 14, TEST_MMHS(0x11)),
	CLK_GFX3D(GFX3D, GFX3D_NS_REG, DBG_BUS_VEC_A_REG,
		HALT, 4,  GMEM_AXI, TEST_MMHS(0x13)),

	CLK_IJPEG(IJPEG, IJPEG_NS_REG, DBG_BUS_VEC_A_REG,
		HALT, 24, TEST_MMHS(0x0B)),
	CLK_JPEGD(JPEGD, JPEGD_NS_REG, DBG_BUS_VEC_A_REG,
		HALT, 19, JPEGD_AXI, TEST_MMHS(0x15)),

	CLK_MDP(MDP, MDP_NS_REG, DBG_BUS_VEC_C_REG, HALT, 10, TEST_MMHS(0x35)),
	CLK_MDP_VSYNC(MDP_VSYNC, MISC_CC2_REG, DBG_BUS_VEC_B_REG, HALT, 22,
		TEST_MMLS(0x41)),

	CLK_PIXEL_MDP(PIXEL_MDP, PIXEL_NS_REG, DBG_BUS_VEC_C_REG, HALT, 23,
		TEST_MMLS(0x09)),
	CLK_SLAVE_MM(PIXEL_LCDC, PIXEL_CC_REG, B(8), DBG_BUS_VEC_C_REG,
		HALT, 21, PIXEL_MDP, TEST_MMLS(0x03)),

	CLK_ROT(ROT, ROT_NS_REG, DBG_BUS_VEC_C_REG, HALT, 15, TEST_MMHS(0x37)),

	CLK_TV(TV_SRC, TV_NS_REG),
	CLK_SLAVE_MM(TV_ENC,  TV_CC_REG,  B(8), DBG_BUS_VEC_D_REG,
		HALT, 8,  TV_SRC, TEST_MMLS(0x45)),
	CLK_SLAVE_MM(TV_DAC,  TV_CC_REG,  B(10), DBG_BUS_VEC_D_REG,
		HALT, 9,  TV_SRC, TEST_MMLS(0x43)),
	CLK_SLAVE_MM(MDP_TV,  TV_CC_REG,  B(0), DBG_BUS_VEC_D_REG,
		HALT, 11, TV_SRC, TEST_MMHS(0x3F)),
	CLK_SLAVE_MM(HDMI_TV, TV_CC_REG,  B(12), DBG_BUS_VEC_D_REG,
		HALT, 10, TV_SRC, TEST_MMHS(0x3D)),
	CLK_SLAVE_MM(DSUB_TV, TV_CC2_REG, B(11), DBG_BUS_VEC_E_REG,
		HALT, 31, TV_SRC, TEST_MMHS(0x4B)),

	CLK_NORATE_MM(HDMI_APP, MISC_CC2_REG, B(11), DBG_BUS_VEC_B_REG,
		HALT, 25, TEST_MMLS(0x3F)),

	CLK_VCODEC(VCODEC, VCODEC_NS_REG, DBG_BUS_VEC_C_REG,
		HALT, 29, TEST_MMHS(0x17)),

	CLK_VPE(VPE, VPE_NS_REG, DBG_BUS_VEC_A_REG, HALT, 28, TEST_MMHS(0x39)),

	CLK_VFE(VFE, VFE_NS_REG, DBG_BUS_VEC_B_REG, HALT, 6,
		VFE_AXI, TEST_MMHS(0x0D)),
	CLK_SLAVE_MM(CSI0_VFE, VFE_CC_REG, B(12), DBG_BUS_VEC_B_REG, HALT, 7,
		VFE, TEST_MMHS(0x07)),
	CLK_SLAVE_MM(CSI1_VFE, VFE_CC_REG, B(10), DBG_BUS_VEC_B_REG, HALT, 8,
		VFE, TEST_MMHS(0x09)),

	/* AXI Interfaces */
	CLK_NORATE_MM(GMEM_AXI,  MAXI_EN_REG, B(24), DBG_BUS_VEC_E_REG,
		HALT, 6, TEST_MMHS(0x23)),
	CLK_NORATE_MM(JPEGD_AXI, MAXI_EN_REG, B(25), DBG_BUS_VEC_E_REG,
		HALT, 5, TEST_MMHS(0x29)),
	CLK_NORATE_MM(VFE_AXI,   MAXI_EN_REG, B(18), DBG_BUS_VEC_E_REG,
		HALT, 0, TEST_MMHS(0x31)),

	/* AHB Interfaces */
	CLK_NORATE_MM(AMP_P,   AHB_EN_REG, B(24),
		DBG_BUS_VEC_F_REG, HALT, 18, TEST_MMLS(0x0D)),
	CLK_NORATE_MM(APU_P,    AHB_EN_REG, B(28),
		DBG_BUS_VEC_F_REG, HALT,  8, TEST_MMLS(0x49)),
	CLK_NORATE_MM(CSI0_P,   AHB_EN_REG, B(7),
		DBG_BUS_VEC_H_REG, HALT, 14, TEST_MMLS(0x0F)),
	CLK_NORATE_MM(CSI1_P,   AHB_EN_REG, B(20),
		DBG_BUS_VEC_H_REG, HALT, 13, TEST_MMLS(0x11)),
	CLK_NORATE_MM(DSI_M_P,  AHB_EN_REG, B(9),
		DBG_BUS_VEC_F_REG, HALT, 19, TEST_MMLS(0x13)),
	CLK_NORATE_MM(FAB_P,    AHB_EN_REG, B(31),
		DBG_BUS_VEC_F_REG, HALT,  1, TEST_MMLS(0x17)),
	CLK_NORATE_MM(IJPEG_P,  AHB_EN_REG, B(5),
		DBG_BUS_VEC_F_REG, HALT,  9,  TEST_MMLS(0x23)),
	CLK_NORATE_MM(JPEGD_P,  AHB_EN_REG, B(21),
		DBG_BUS_VEC_F_REG, HALT,  7,  TEST_MMLS(0x27)),
	CLK_NORATE_MM(MDP_P,    AHB_EN_REG, B(10),
		DBG_BUS_VEC_F_REG, HALT, 11,  TEST_MMLS(0x29)),
	CLK_NORATE_MM(ROT_P,    AHB_EN_REG, B(12),
		DBG_BUS_VEC_F_REG, HALT, 13, TEST_MMLS(0x2D)),
	CLK_NORATE_MM(TV_ENC_P, AHB_EN_REG, B(25),
		DBG_BUS_VEC_F_REG, HALT, 23, TEST_MMLS(0x33)),
	CLK_NORATE_MM(VFE_P,    AHB_EN_REG, B(13),
		DBG_BUS_VEC_F_REG, HALT, 14, TEST_MMLS(0x37)),
	CLK_NORATE_MM(VPE_P,    AHB_EN_REG, B(16),
		DBG_BUS_VEC_F_REG, HALT, 15, TEST_MMLS(0x39)),

	/*
	 * Low Power Audio Clocks
	 */

	CLK_AIF(MI2S_SRC, LCC_MI2S_NS_REG, chld_mi2s_src),
	CLK_SLAVE_LPA(MI2S, LCC_MI2S_NS_REG, B(15),
			LCC_MI2S_STATUS_REG, ENABLE, 0,
			MI2S_SRC, TEST_LPA(0x17)),
	CLK_SLAVE_LPA(MI2S_M, LCC_MI2S_NS_REG, B(17),
			LCC_MI2S_STATUS_REG, ENABLE, 1,
			MI2S_SRC, TEST_LPA(0x15)),

	CLK_AIF(CODEC_I2S_MIC_SRC, LCC_CODEC_I2S_MIC_NS_REG,
			chld_codec_i2s_mic_src),
	CLK_SLAVE_LPA(CODEC_I2S_MIC, LCC_CODEC_I2S_MIC_NS_REG, B(15),
			LCC_CODEC_I2S_MIC_STATUS_REG, ENABLE, 0,
			CODEC_I2S_MIC_SRC, TEST_LPA(0x1B)),
	CLK_SLAVE_LPA(CODEC_I2S_MIC_M, LCC_CODEC_I2S_MIC_NS_REG, B(17),
			LCC_CODEC_I2S_MIC_STATUS_REG, ENABLE, 1,
			CODEC_I2S_MIC_SRC, TEST_LPA(0x19)),

	CLK_AIF(SPARE_I2S_MIC_SRC, LCC_SPARE_I2S_MIC_NS_REG,
			chld_spare_i2s_mic_src),
	CLK_SLAVE_LPA(SPARE_I2S_MIC,   LCC_SPARE_I2S_MIC_NS_REG, B(15),
			LCC_SPARE_I2S_MIC_STATUS_REG, ENABLE, 0,
			SPARE_I2S_MIC_SRC, TEST_LPA(0x23)),
	CLK_SLAVE_LPA(SPARE_I2S_MIC_M, LCC_SPARE_I2S_MIC_NS_REG, B(17),
			LCC_SPARE_I2S_MIC_STATUS_REG, ENABLE, 1,
			SPARE_I2S_MIC_SRC, TEST_LPA(0x21)),

	CLK_AIF(CODEC_I2S_SPKR_SRC, LCC_CODEC_I2S_SPKR_NS_REG,
			chld_codec_i2s_spkr_src),
	CLK_SLAVE_LPA(CODEC_I2S_SPKR,   LCC_CODEC_I2S_SPKR_NS_REG, B(15),
			LCC_CODEC_I2S_SPKR_STATUS_REG, ENABLE, 0,
			CODEC_I2S_SPKR_SRC, TEST_LPA(0x1F)),
	CLK_SLAVE_LPA(CODEC_I2S_SPKR_M, LCC_CODEC_I2S_SPKR_NS_REG, B(17),
			LCC_CODEC_I2S_SPKR_STATUS_REG, ENABLE, 1,
			CODEC_I2S_SPKR_SRC, TEST_LPA(0x1D)),

	CLK_AIF(SPARE_I2S_SPKR_SRC, LCC_SPARE_I2S_SPKR_NS_REG,
			chld_spare_i2s_spkr_src),
	CLK_SLAVE_LPA(SPARE_I2S_SPKR,   LCC_SPARE_I2S_SPKR_NS_REG, B(15),
			LCC_SPARE_I2S_SPKR_STATUS_REG, ENABLE, 0,
			SPARE_I2S_SPKR_SRC, TEST_LPA(0x27)),
	CLK_SLAVE_LPA(SPARE_I2S_SPKR_M, LCC_SPARE_I2S_SPKR_NS_REG, B(17),
			LCC_SPARE_I2S_SPKR_STATUS_REG, ENABLE, 1,
			SPARE_I2S_SPKR_SRC, TEST_LPA(0x25)),

	CLK_PCM(PCM, LCC_PCM_NS_REG, LCC_PCM_STATUS_REG, ENABLE, 0,
			TEST_LPA(0x29)),
};

static DEFINE_SPINLOCK(clock_reg_lock);
static DEFINE_SPINLOCK(pll_vote_lock);

static int pll_is_voteable(int pll)
{
	switch (pll) {
	case PLL_0:
	case PLL_6:
	case PLL_8:
		return 1;
	default:
		return 0;
	}
}

#define PLL_ACTIVE_MASK	B(16)
void pll_enable(int pll)
{
	uint32_t reg_val;
	unsigned long flags;

	if (!pll_is_voteable(pll))
		return;

	spin_lock_irqsave(&pll_vote_lock, flags);
	if (!pll_count[pll]) {
		reg_val = readl(PLL_ENA_SC0_REG);
		reg_val |= (1 << pll);
		writel(reg_val, PLL_ENA_SC0_REG);
	}
	pll_count[pll]++;
	spin_unlock_irqrestore(&pll_vote_lock, flags);

	/* TODO:
	 * Once PLL voting is supported, wait here until the PLL is enabled.
	 */
}

void pll_disable(int pll)
{
	uint32_t reg_val;
	unsigned long flags;

	if (!pll_is_voteable(pll))
		return;

	spin_lock_irqsave(&pll_vote_lock, flags);
	if (pll_count[pll])
		pll_count[pll]--;
	else
		pr_warning("Reference count mismatch in PLL disable!\n");

	if (pll_count[pll] == 0) {
		reg_val = readl(PLL_ENA_SC0_REG);
		reg_val &= ~(1 << pll);
		writel(reg_val, PLL_ENA_SC0_REG);
	}
	spin_unlock_irqrestore(&pll_vote_lock, flags);
}

/*
 * SoC specific register-based control of clocks.
 */

/* Return non-zero if a clock status registers shows the clock is halted. */
static int soc_clk_is_halted(unsigned id)
{
	struct clk_local *clk = &clk_local_tbl[id];
	int invert = (clk->halt_check == ENABLE);
	int status_bit = readl(clk->halt_reg) & B(clk->halt_bit);
	return invert ? !status_bit : status_bit;
}

static int _soc_clk_enable(unsigned id)
{
	struct clk_local *clk = &clk_local_tbl[id];
	void *reg = clk->cc_reg;
	uint32_t reg_val;

	WARN((clk->type != NORATE) && (clk->current_freq == &dummy_freq),
		"Attempting to enable clock %d before setting its rate. "
		"Set the rate first!\n", id);

	/* Enable MN counter, if applicable. */
	reg_val = readl(reg);
	if (clk->type == MND) {
		reg_val |= clk->current_freq->mnd_en_mask;
		writel(reg_val, reg);
	}
	/* Enable root. */
	if (clk->root_en_mask) {
		reg_val |= clk->root_en_mask;
		writel(reg_val, reg);
	}
	/* Enable branch. */
	if (clk->br_en_mask) {
		reg_val |= clk->br_en_mask;
		writel(reg_val, reg);
	}

	/* Wait for clock to enable before returning. */
	if (clk->halt_check == DELAY)
		udelay(10);
	else if (clk->halt_check == ENABLE || clk->halt_check == HALT) {
		int halted, count = 0;
		/* Use a memory barrier since some halt status registers are
		 * not within the same 1K segment as the branch/root enable
		 * registers. */
		mb();

		/* Wait up to HALT_CHECK_MAX_LOOPS for clock to enable. */
		while ((halted = soc_clk_is_halted(id))
				&& count++ < HALT_CHECK_MAX_LOOPS)
			udelay(1);
		if (halted)
			pr_warning("%s: clock %d never turned on\n",
					__func__, id);
	}

	return 0;
}

static void _soc_clk_disable(unsigned id)
{
	struct clk_local *clk = &clk_local_tbl[id];
	void *reg = clk->cc_reg;
	uint32_t reg_val;

	/* Disable branch. */
	reg_val = readl(reg);
	if (clk->br_en_mask) {
		reg_val &= ~(clk->br_en_mask);
		writel(reg_val, reg);
	}

	/* Wait for clock to disable before continuing. */
	if (clk->halt_check == DELAY)
		udelay(10);
	else if (clk->halt_check == ENABLE || clk->halt_check == HALT) {
		int halted, count = 0;
		/* Use a memory barrier since some halt status registers are
		 * not within the same 1K segment as the branch/root enable
		 * registers. */
		mb();

		/* Wait up to HALT_CHECK_MAX_LOOPS for clock to disable. */
		while (!(halted = soc_clk_is_halted(id)) &&
					count++ < HALT_CHECK_MAX_LOOPS)
			udelay(1);
		if (!halted)
			pr_warning("%s: clock %d never turned off\n",
					__func__, id);
	}

	/* Disable root. */
	if (clk->root_en_mask) {
		reg_val &= ~(clk->root_en_mask);
		writel(reg_val, reg);
	}
	/* Disable MN counter, if applicable. */
	if (clk->type == MND) {
		reg_val &= ~(clk->current_freq->mnd_en_mask);
		writel(reg_val, reg);
	}
}

static int soc_clk_enable_nolock(unsigned id)
{
	struct clk_local *clk = &clk_local_tbl[id];
	int ret = 0;

	if (clk->type == RESET)
		return -EPERM;

	if (!clk->count) {
		if (clk->parent != C(NONE))
			soc_clk_enable_nolock(clk->parent);
		pll_enable(clk->current_freq->pll);
		ret = _soc_clk_enable(id);
	}
	clk->count++;

	return ret;
}

static void soc_clk_disable_nolock(unsigned id)
{
	struct clk_local *clk = &clk_local_tbl[id];

	if (!clk->count) {
		pr_warning("Reference count mismatch in clock disable!\n");
		return;
	}
	if (clk->count)
		clk->count--;
	if (clk->count == 0) {
		_soc_clk_disable(id);
		pll_disable(clk->current_freq->pll);
		if (clk->parent != C(NONE))
			soc_clk_disable_nolock(clk->parent);
	}

	return;
}

static int soc_clk_enable(unsigned id)
{
	int ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&clock_reg_lock, flags);
	ret = soc_clk_enable_nolock(id);
	spin_unlock_irqrestore(&clock_reg_lock, flags);

	return ret;
}

static void soc_clk_disable(unsigned id)
{
	unsigned long flags;

	spin_lock_irqsave(&clock_reg_lock, flags);
	soc_clk_disable_nolock(id);
	spin_unlock_irqrestore(&clock_reg_lock, flags);

	return;
}

static int soc_clk_reset(unsigned id, enum clk_reset_action action)
{
	struct clk_local *clk = &clk_local_tbl[id];
	uint32_t reg_val, ret = 0;
	unsigned long flags;

	if (clk->reset_reg == NULL)
		return -EPERM;

	spin_lock_irqsave(&clock_reg_lock, flags);

	reg_val = readl(clk->reset_reg);
	switch (action) {
	case CLK_RESET_ASSERT:
		reg_val |= clk->reset_mask;
		break;
	case CLK_RESET_DEASSERT:
		reg_val &= ~(clk->reset_mask);
		break;
	default:
		ret = -EINVAL;
	}
	writel(reg_val, clk->reset_reg);

	spin_unlock_irqrestore(&clock_reg_lock, flags);

	return ret;
}

static struct clk_freq_tbl *find_freq(struct clk_freq_tbl *freq_tbl,
				      unsigned rate, enum match_types match)
{
	struct clk_freq_tbl *nf;

	/* Find new frequency based on match rule. */
	switch (match) {
	case MATCH_MIN:
		for (nf = freq_tbl; nf->freq_hz != FREQ_END; nf++)
			if (nf->freq_hz >= rate)
				break;
		break;
	default:
	case MATCH_EXACT:
		for (nf = freq_tbl; nf->freq_hz != FREQ_END; nf++)
			if (nf->freq_hz == rate)
				break;
		break;
	}

	if (nf->freq_hz == FREQ_END)
		return ERR_PTR(-EINVAL);

	return nf;
}

static int _soc_clk_set_rate(unsigned id, unsigned rate, enum match_types match)
{
	struct clk_local *clk = &clk_local_tbl[id];
	struct clk_freq_tbl *cf;
	struct clk_freq_tbl *nf;
	uint32_t *chld = clk->children;
	uint32_t reg_val = 0;
	int i;
	unsigned long flags;

	if (clk->type == NORATE || clk->type == RESET)
		return -EPERM;

	/* Find new frequency. */
	nf = find_freq(clk->freq_tbl, rate, match);
	if (IS_ERR(nf))
		return PTR_ERR(nf);

	spin_lock_irqsave(&clock_reg_lock, flags);

	/* Check if frequency is actually changed. */
	cf = clk->current_freq;
	if (nf == cf)
		goto release_lock;

	/* Disable clocks if clock is not glitch-free banked. */
	if (clk->banked_mnd_masks == NULL) {
		/* Disable all branches to prevent jitter. */
		for (i = 0; chld && chld[i] != C(NONE); i++) {
			struct clk_local *ch = &clk_local_tbl[chld[i]];
			/* Don't bother turning off if it is already off.
			 * Checking ch->count is cheaper (cache) than reading
			 * and writing to a register (uncached/unbuffered). */
			if (ch->count) {
				reg_val = readl(ch->cc_reg);
				reg_val &= ~(ch->br_en_mask);
				writel(reg_val, ch->cc_reg);
			}
		}
		if (clk->count)
			_soc_clk_disable(id);
	}

	if (clk->count) {
		/* Turn on PLL of the new freq. */
		pll_enable(nf->pll);
	}

	/* Perform clock-specific frequency switch operations. */
	BUG_ON(!clk->set_rate);
	clk->set_rate(clk, nf);

	if (clk->count) {
		/* Turn off PLL of the old freq. */
		pll_disable(cf->pll);
	}

	/* Current freq must be updated before _soc_clk_enable() is called to
	 * make sure the MNCNTR_E bit is set correctly. */
	clk->current_freq = nf;

	/* Enable any clocks that were disabled. */
	if (clk->banked_mnd_masks == NULL) {
		if (clk->count)
			_soc_clk_enable(id);
		/* Enable only branches that were ON before. */
		for (i = 0; chld && chld[i] != C(NONE); i++) {
			struct clk_local *ch = &clk_local_tbl[chld[i]];
			if (ch->count) {
				reg_val = readl(ch->cc_reg);
				reg_val |= ch->br_en_mask;
				writel(reg_val, ch->cc_reg);
			}
		}
	}

release_lock:
	spin_unlock_irqrestore(&clock_reg_lock, flags);
	return 0;
}

static int soc_clk_set_rate(unsigned id, unsigned rate)
{
	return _soc_clk_set_rate(id, rate, MATCH_EXACT);
}

static int soc_clk_set_min_rate(unsigned id, unsigned rate)
{
	return _soc_clk_set_rate(id, rate, MATCH_MIN);
}

static int soc_clk_set_max_rate(unsigned id, unsigned rate)
{
	return -EPERM;
}

static int soc_clk_set_flags(unsigned id, unsigned flags)
{
	return -EPERM;
}

static unsigned soc_clk_get_rate(unsigned id)
{
	struct clk_local *clk = &clk_local_tbl[id];
	unsigned long flags;
	unsigned ret = 0;

	if (clk->type == NORATE || clk->type == RESET)
		return 0;

	spin_lock_irqsave(&clock_reg_lock, flags);
	ret = clk->current_freq->freq_hz;
	spin_unlock_irqrestore(&clock_reg_lock, flags);

	/* Return 0 if the rate has never been set. Might not be correct,
	 * but it's good enough. */
	if (ret == FREQ_END)
		ret = 0;

	return ret;
}

static uint32_t run_measurement(unsigned ticks)
{
	/* Stop counters and set the XO4 counter start value. */
	writel(0x0, RINGOSC_TCXO_CTL_REG);
	writel(ticks, RINGOSC_TCXO_CTL_REG);

	/* Wait for timer to become ready. */
	while ((readl(RINGOSC_STATUS_REG) & B(25)) != 0)
		cpu_relax();

	/* Run measurement and wait for completion. */
	writel(B(20)|ticks, RINGOSC_TCXO_CTL_REG);
	while ((readl(RINGOSC_STATUS_REG) & B(25)) == 0)
		cpu_relax();

	/* Stop counters. */
	writel(0x0, RINGOSC_TCXO_CTL_REG);

	/* Return measured ticks. */
	return readl(RINGOSC_STATUS_REG) & BM(24, 0);
}

/* FOR DEBUG USE ONLY: Measurements take ~15 ms! */
static signed soc_clk_measure_rate(unsigned id)
{
	struct clk_local *clk = &clk_local_tbl[id];
	unsigned long flags;
	uint32_t vector, pdm_reg_backup, ringosc_reg_backup;
	uint64_t raw_count_short, raw_count_full;
	signed ret;

	spin_lock_irqsave(&clock_reg_lock, flags);

	/* Program the test vector. */
	vector = clk->test_vector & TEST_VECTOR_MASK;
	switch (clk->test_vector >> TEST_TYPE_SHIFT) {
	case TEST_TYPE_PER:
		writel((0x4030D00|vector), CLK_TEST_REG);
		break;
	case TEST_TYPE_MMLS:
		writel(0x4030D97, CLK_TEST_REG);
		writel(vector, DBG_CFG_REG_LS_REG);
		break;
	case TEST_TYPE_MMHS:
		writel(0x402B800, CLK_TEST_REG);
		writel(vector, DBG_CFG_REG_HS_REG);
		break;
	case TEST_TYPE_LPA:
		writel(0x4030D98, CLK_TEST_REG);
		writel(vector, LCC_CLK_LS_DEBUG_CFG_REG);
		break;
	default:
		ret = -EPERM;
		goto err;
	}

	/* Enable CXO/4 and RINGOSC branch and root. */
	pdm_reg_backup = readl(PDM_CLK_NS_REG);
	ringosc_reg_backup = readl(RINGOSC_NS_REG);
	writel(0x2898, PDM_CLK_NS_REG);
	writel(0xA00, RINGOSC_NS_REG);

	/*
	 * The ring oscillator counter will not reset if the measured clock
	 * is not running.  To detect this, run a short measurement before
	 * the full measurement.  If the raw results of the two are the same
	 * then the clock must be off.
	 */

	/* Run a short measurement. (~1 ms) */
	raw_count_short = run_measurement(0x1000);
	/* Run a full measurement. (~14 ms) */
	raw_count_full = run_measurement(0x10000);

	writel(ringosc_reg_backup, RINGOSC_NS_REG);
	writel(pdm_reg_backup, PDM_CLK_NS_REG);

	/* Return 0 if the clock is off. */
	if (raw_count_full == raw_count_short)
		ret = 0;
	else {
		/* Compute rate in Hz. */
		raw_count_full = ((raw_count_full * 10) + 15) * 4800000;
		do_div(raw_count_full, ((0x10000 * 10) + 35));
		ret = (signed)raw_count_full;
	}

err:
	spin_unlock_irqrestore(&clock_reg_lock, flags);

	return ret;
}

static unsigned soc_clk_is_enabled(unsigned id)
{
	return !!(clk_local_tbl[id].count);
}

static long soc_clk_round_rate(unsigned id, unsigned rate)
{
	struct clk_local *clk = &clk_local_tbl[id];
	struct clk_freq_tbl *f;

	if (clk->type == NORATE || clk->type == RESET)
		return -EINVAL;

	for (f = clk->freq_tbl; f->freq_hz != FREQ_END; f++)
		if (f->freq_hz >= rate)
			return f->freq_hz;

	return -EPERM;
}

struct clk_ops clk_ops_8x60 = {
	.enable = soc_clk_enable,
	.disable = soc_clk_disable,
	.reset = soc_clk_reset,
	.set_rate = soc_clk_set_rate,
	.set_min_rate = soc_clk_set_min_rate,
	.set_max_rate = soc_clk_set_max_rate,
	.set_flags = soc_clk_set_flags,
	.get_rate = soc_clk_get_rate,
	.measure_rate = soc_clk_measure_rate,
	.is_enabled = soc_clk_is_enabled,
	.round_rate = soc_clk_round_rate,
};

void __init msm_clk_soc_set_ops(struct clk *clk)
{
	return;
}

static struct reg_init {
	void *reg;
	uint32_t mask;
	uint32_t val;
	uint32_t delay_us;
} ri_list[] __initdata = {

	/* Program MM_PLL0 (PLL1) @ 1320MHz */
	{PLL0_MODE_REG, B(0), 0},     /* Disable output */
	{PLL0_L_VAL_REG, 0xFF,   48}, /* LVAL */
	{PLL0_M_VAL_REG, 0x7FFFF, 8}, /* MVAL */
	{PLL0_N_VAL_REG, 0x7FFFF, 9}, /* NVAL */
	/* Ref = MXO, don't bypass, delay 10us after write. */
	{PLL0_MODE_REG, B(4)|B(1), B(4)|B(1), 10},
	/* Enable MN, set VCO, misc config. */
	{PLL0_CONFIG_REG, 0xFFFFFFFF, 0x14580},
	{PLL0_MODE_REG, B(2), B(2)}, /* Deassert reset */
	{PLL0_MODE_REG, B(0), B(0)}, /* Enable output */

	/* Program MM_PLL1 (PLL2) @ 800MHz */
	{PLL1_MODE_REG, B(0), 0},      /* Disable output */
	{PLL1_L_VAL_REG, 0x3FF,   29}, /* LVAL */
	{PLL1_M_VAL_REG, 0x7FFFF, 17}, /* MVAL */
	{PLL1_N_VAL_REG, 0x7FFFF, 27}, /* NVAL */
	/* Ref = MXO, don't bypass, delay 10us after write. */
	{PLL1_MODE_REG, B(4)|B(1), B(4)|B(1), 10},
	/* Enable MN, set VCO, main out. */
	{PLL1_CONFIG_REG, 0xFFFFFFFF, 0x00C22080},
	{PLL1_MODE_REG, B(2), B(2)}, /* Deassert reset */
	{PLL1_MODE_REG, B(0), B(0)}, /* Enable output */

	/* Program MM_PLL2 (PLL3) @ <Varies>, 50.4005MHz for now. */
	{PLL2_MODE_REG, B(0), 0},         /* Disable output */
	{PLL2_L_VAL_REG, 0x3FF,       7}, /* LVAL */
	{PLL2_M_VAL_REG, 0x7FFFF,  6301}, /* MVAL */
	{PLL2_N_VAL_REG, 0x7FFFF, 13500}, /* NVAL */
	/* Ref = MXO, don't bypass, delay 10us after write. */
	{PLL2_MODE_REG, B(4)|B(1), B(4)|B(1), 10},
	/* Enable MN, set VCO, main out, postdiv4. */
	{PLL2_CONFIG_REG, 0xFFFFFFFF, 0x00E02080},
	{PLL2_MODE_REG, B(2), B(2)}, /* Deassert reset */
	{PLL2_MODE_REG, B(0), B(0)}, /* Enable output */

	/* Program LPA_PLL (PLL4) @ 540.6720 MHz */
	{LCC_PRI_PLL_CLK_CTL_REG, B(0), B(0)}, /* PLL clock select = PLL0 */
	{LCC_PLL0_MODE_REG, B(0), 0},          /* Disable output */
	{LCC_PLL0_L_VAL_REG, 0x3FF,     20},   /* LVAL */
	{LCC_PLL0_M_VAL_REG, 0x7FFFF,   28},   /* MVAL */
	{LCC_PLL0_N_VAL_REG, 0x7FFFF, 1125},   /* NVAL */
	/* Ref = MXO, don't bypass, delay 10us after write. */
	{LCC_PLL0_MODE_REG, B(4)|B(1), B(4)|B(1), 10},
	/* Enable MN, set VCO, main out. */
	{LCC_PLL0_CONFIG_REG, 0xFFFFFFFF, 0x00822080},
	{LCC_PLL0_MODE_REG, B(2), B(2)}, /* Deassert reset */
	{LCC_PLL0_MODE_REG, B(0), B(0)}, /* Enable output */

	/* Turn on all SC0 voteable PLLs (PLL0, PLL6, PLL8). */
	{PLL_ENA_SC0_REG, 0x141, 0x141},

	/* Enable locally controlled peripheral HCLKs in software mode. */
	{TSIF_HCLK_CTL_REG,		0x70,	0x10},
	{SDCn_HCLK_CTL_REG(1),		0x70,	0x10},
	{SDCn_HCLK_CTL_REG(2),		0x70,	0x10},
	{SDCn_HCLK_CTL_REG(3),		0x70,	0x10},
	{SDCn_HCLK_CTL_REG(4),		0x70,	0x10},
	{SDCn_HCLK_CTL_REG(5),		0x70,	0x10},
	{USB_HS1_HCLK_CTL_REG,		0x70,	0x10},
	{USB_FS1_HCLK_CTL_REG,		0x70,	0x10},
	{USB_FS2_HCLK_CTL_REG,		0x70,	0x10},
	{GSBIn_HCLK_CTL_REG(1),		0x70,	0x10},
	{GSBIn_HCLK_CTL_REG(2),		0x70,	0x10},
	{GSBIn_HCLK_CTL_REG(3),		0x70,	0x10},
	{GSBIn_HCLK_CTL_REG(4),		0x70,	0x10},
	{GSBIn_HCLK_CTL_REG(5),		0x70,	0x10},
	{GSBIn_HCLK_CTL_REG(6),		0x70,	0x10},
	{GSBIn_HCLK_CTL_REG(7),		0x70,	0x10},
	{GSBIn_HCLK_CTL_REG(8),		0x70,	0x10},
	{GSBIn_HCLK_CTL_REG(9),		0x70,	0x10},
	{GSBIn_HCLK_CTL_REG(10),	0x70,	0x10},
	{GSBIn_HCLK_CTL_REG(11),	0x70,	0x10},
	{GSBIn_HCLK_CTL_REG(12),	0x70,	0x10},

	/* Deassert MM SW_RESET_ALL signal. */
	{SW_RESET_ALL_REG, 0x1, 0x0},

	/* Set up MM AHB clock to PLL2/10. */
	{AHB_NS_REG,		0x43C7,		0x0241},

	/* Enable MM AHB clocks that don't have clock API support.
	 * These will be put into hardware-controlled mode later. */
	{AHB_EN_REG,		0xFFFFFFFF,	0x8CC85F},
	{AHB_EN2_REG,		0xFFFFFFFF,	0x1},
	{SW_RESET_AHB_REG,	0xFFFFFFFF,	0x0},

	/* Set up MM Fabric (AXI) clock. */
	{AXI_NS_REG,		0x0FFFFFFF,	0x4248451},

	/* Enable MM AXI clocks that don't have clock API support.
	 * These will be put into hardware-controlled mode later. */
	{MAXI_EN_REG,		0xFFFFFFFF,	0x14F80001},
	{MAXI_EN2_REG,		0x7FFFFFFF,	0x75200400},
	{MAXI_EN3_REG,		0xFFFFFFFF,	0x200400},
	{SAXI_EN_REG,		0x3FFF,		0x1C7},

	/* De-assert MM AXI resets to all hardware blocks. */
	{SW_RESET_AXI_REG,	0xE37F,		0x0},

	/* Deassert all MM core resets. */
	{SW_RESET_CORE_REG,	0x1FFFFFF,	0x0},

	/* Set hdmi_ref_clk to MM_PLL2/2. */
	{MISC_CC2_REG,		B(28)|BM(21, 18), B(28)|BVAL(21, 18, 0x1)},
	/* Set hdmi_app_clk source to MXO and divider to 1 (27MHz). */
	{MISC_CC2_REG,		B(17),		B(17)},
	{MISC_CC_REG,		BM(19, 18),	0},

	/* Set audio bit clock sources to OSR (m_clk) and divider to 1/8. */
	{LCC_CODEC_I2S_MIC_NS_REG,  BM(14, 10), BVAL(14, 10, 0x7)},
	{LCC_CODEC_I2S_SPKR_NS_REG, BM(14, 10), BVAL(14, 10, 0x7)},
	{LCC_SPARE_I2S_MIC_NS_REG,  BM(14, 10), BVAL(14, 10, 0x7)},
	{LCC_SPARE_I2S_SPKR_NS_REG, BM(14, 10), BVAL(14, 10, 0x7)},
	{LCC_MI2S_NS_REG,           BM(14, 10), BVAL(14, 10, 0x7)},

};

#define set_1rate(clk) \
	soc_clk_set_rate(C(clk), clk_local_tbl[C(clk)].freq_tbl->freq_hz)
void __init msm_clk_soc_init(void)
{
	int i;
	uint32_t val;

	for (i = 0; i < ARRAY_SIZE(ri_list); i++) {
		val = readl(ri_list[i].reg);
		val &= ~ri_list[i].mask;
		val |= ri_list[i].val;
		writel(val, ri_list[i].reg);
		if (ri_list[i].delay_us)
			udelay(ri_list[i].delay_us);
	}

	soc_clk_enable(C(FAB_P));

	/* Initialize rates for clocks that only support one. */
	set_1rate(BBRX_SSBI);
	set_1rate(MDP_VSYNC);
	set_1rate(TSIF_REF);
	set_1rate(TSSC);
	set_1rate(USB_HS_XCVR);
	set_1rate(USB_FS1_SRC);
	set_1rate(USB_FS2_SRC);

	/* FIXME: Disabling or changing the rate of the GFX3D clock causes
	 * crashes.  Until this is fixed, leave the clock on at a constant
	 * rate. */
	soc_clk_set_rate(C(GFX3D), 266667000);
	soc_clk_enable(C(GFX3D));
}
