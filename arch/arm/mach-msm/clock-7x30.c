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
#include <mach/internal_power_rail.h>

#include "clock.h"
#include "clock-local.h"
#include "clock-7x30.h"
#include "proc_comm.h"

#define REG_BASE(off) (MSM_CLK_CTL_BASE + (off))
#define REG(off) (MSM_CLK_CTL_SH2_BASE + (off))

/* Shadow-region 2 (SH2) registers. */
#define	QUP_I2C_NS_REG		REG(0x04F0)
#define CAM_NS_REG		REG(0x0374)
#define CAM_VFE_NS_REG		REG(0x0044)
#define CLK_HALT_STATEA_REG	REG(0x0108)
#define CLK_HALT_STATEB_REG	REG(0x010C)
#define CLK_HALT_STATEC_REG	REG(0x02D4)
#define CSI_NS_REG		REG(0x0174)
#define EMDH_NS_REG		REG(0x0050)
#define GLBL_CLK_ENA_2_SC_REG	REG(0x03C0)
#define GLBL_CLK_ENA_SC_REG	REG(0x03BC)
#define GLBL_CLK_STATE_2_REG	REG(0x037C)
#define GLBL_CLK_STATE_REG	REG(0x0004)
#define GRP_2D_NS_REG		REG(0x0034)
#define GRP_NS_REG		REG(0x0084)
#define HDMI_NS_REG		REG(0x0484)
#define I2C_2_NS_REG		REG(0x02D8)
#define I2C_NS_REG		REG(0x0068)
#define JPEG_NS_REG		REG(0x0164)
#define LPA_CORE_CLK_MA0_REG	REG(0x04F4)
#define LPA_CORE_CLK_MA2_REG	REG(0x04FC)
#define LPA_NS_REG		REG(0x02E8)
#define MDC_NS_REG		REG(0x007C)
#define MDP_LCDC_NS_REG		REG(0x0390)
#define MDP_NS_REG		REG(0x014C)
#define MDP_VSYNC_REG		REG(0x0460)
#define MFC_NS_REG		REG(0x0154)
#define MI2S_CODEC_RX_DIV_REG	REG(0x02EC)
#define MI2S_CODEC_TX_DIV_REG	REG(0x02F0)
#define MI2S_DIV_REG		REG(0x02E4)
#define MI2S_NS_REG		REG(0x02E0)
#define MI2S_RX_NS_REG		REG(0x0070)
#define MI2S_TX_NS_REG		REG(0x0078)
#define MIDI_NS_REG		REG(0x02D0)
#define PLL_ENA_REG		REG(0x0264)
#define PMDH_NS_REG		REG(0x008C)
#define SDAC_NS_REG		REG(0x009C)
#define SDCn_NS_REG(n)		REG(0x00A4+(0x8*((n)-1)))
#define SPI_NS_REG		REG(0x02C8)
#define TSIF_NS_REG		REG(0x00C4)
#define TV_NS_REG		REG(0x00CC)
#define UART1DM_NS_REG		REG(0x00D4)
#define UART2DM_NS_REG		REG(0x00DC)
#define UART2_NS_REG		REG(0x0464)
#define UART_NS_REG		REG(0x00E0)
#define USBH2_NS_REG		REG(0x046C)
#define USBH3_NS_REG		REG(0x0470)
#define USBH_MD_REG		REG(0x02BC)
#define USBH_NS_REG		REG(0x02C0)
#define VPE_NS_REG		REG(0x015C)

/* Registers in the base (non-shadow) region. */
#define CLK_TEST_BASE_REG	REG_BASE(0x011C)
#define CLK_TEST_2_BASE_REG	REG_BASE(0x0384)
#define MISC_CLK_CTL_BASE_REG	REG_BASE(0x0110)
#define PRPH_WEB_NS_BASE_REG	REG_BASE(0x0080)
#define PLL0_STATUS_BASE_REG	REG_BASE(0x0318)
#define PLL1_STATUS_BASE_REG	REG_BASE(0x0334)
#define PLL2_STATUS_BASE_REG	REG_BASE(0x0350)
#define PLL3_STATUS_BASE_REG	REG_BASE(0x036C)
#define PLL4_STATUS_BASE_REG	REG_BASE(0x0254)
#define PLL5_STATUS_BASE_REG	REG_BASE(0x0258)
#define PLL6_STATUS_BASE_REG	REG_BASE(0x04EC)
#define RINGOSC_CNT_BASE_REG	REG_BASE(0x00FC)
#define SH2_OWN_APPS1_BASE_REG	REG_BASE(0x040C)
#define SH2_OWN_APPS2_BASE_REG	REG_BASE(0x0414)
#define SH2_OWN_APPS3_BASE_REG	REG_BASE(0x0444)
#define SH2_OWN_GLBL_BASE_REG	REG_BASE(0x0404)
#define SH2_OWN_ROW1_BASE_REG	REG_BASE(0x041C)
#define SH2_OWN_ROW2_BASE_REG	REG_BASE(0x0424)
#define TCXO_CNT_BASE_REG	REG_BASE(0x00F8)
#define TCXO_CNT_DONE_BASE_REG	REG_BASE(0x00F8)


/* MUX source input identifiers. */
#define SRC_SEL_PLL0	4 /* Modem PLL */
#define SRC_SEL_PLL1	1 /* Global PLL */
#define SRC_SEL_PLL3	3 /* Multimedia/Peripheral PLL or Backup PLL1 */
#define SRC_SEL_PLL4	2 /* Display PLL */
#define SRC_SEL_LPXO	6 /* Low-power XO */
#define SRC_SEL_TCXO	0 /* Used for sources that always source from TCXO */
#define SRC_SEL_AXI	0 /* Used for rates that sync to AXI */

/* Source name to PLL mappings. */
#define SRC_PLL0	PLL_0
#define SRC_PLL1	PLL_1
#define SRC_PLL3	PLL_3
#define SRC_PLL4	PLL_4
#define SRC_LPXO	LPXO
#define SRC_TCXO	TCXO
#define SRC_AXI		AXI

/* Clock declaration macros. */
#define MN_MODE_DUAL_EDGE	0x2
#define MD8(m, n)		(BVAL(15, 8, m) | BVAL(7, 0, ~(n)))
#define N8(msb, lsb, m, n)	(BVAL(msb, lsb, ~(n-m)) | BVAL(6, 5, \
					(MN_MODE_DUAL_EDGE * !!(n))))
#define MD16(m, n)		(BVAL(31, 16, m) | BVAL(15, 0, ~(n)))
#define N16(m, n)		(BVAL(31, 16, ~(n-m)) | BVAL(6, 5, \
					(MN_MODE_DUAL_EDGE * !!(n))))
#define SPDIV(s, d)		(BVAL(4, 3, d-1) | BVAL(2, 0, s))
#define SDIV(s, d)		(BVAL(6, 3, d-1) | BVAL(2, 0, s))
#define F_MASK_BASIC		(BM(6, 3)|BM(2, 0))
#define F_MASK_MND16		(BM(31, 16)|BM(6, 5)|BM(4, 3)|BM(2, 0))
#define F_MASK_MND8(m, l)	(BM(m, l)|BM(6, 5)|BM(4, 3)|BM(2, 0))

/*
 * Clock frequency definitions and macros
 */
#define F_BASIC(f, s, div, v) \
	F_RAW(f, SRC_##s, 0, SDIV(SRC_SEL_##s, div), 0, 0, v, NULL)
#define F_MND16(f, s, div, m, n, v) \
	F_RAW(f, SRC_##s, MD16(m, n), N16(m, n)|SPDIV(SRC_SEL_##s, div), \
		0, (B(8) * !!(n)), v, NULL)
#define F_MND8(f, nmsb, nlsb, s, div, m, n, v) \
	F_RAW(f, SRC_##s, MD8(m, n), \
		N8(nmsb, nlsb, m, n)|SPDIV(SRC_SEL_##s, div), 0, \
		(B(8) * !!(n)), v, NULL)

static struct clk_freq_tbl clk_tbl_csi[] = {
	F_MND8(153600000, 24, 17, PLL1, 2, 2, 5, NOMINAL),
	F_MND8(192000000, 24, 17, PLL1, 4, 0, 0, NOMINAL),
	F_MND8(384000000, 24, 17, PLL1, 2, 0, 0, NOMINAL),
	F_END,
};

static struct clk_freq_tbl clk_tbl_tcxo[] = {
	F_RAW(19200000, SRC_TCXO, 0, 0, 0, 0, NOMINAL, NULL),
	F_END,
};

static struct clk_freq_tbl clk_tbl_axi[] = {
	F_RAW(1, SRC_AXI, 0, 0, 0, 0, NOMINAL, NULL),
	F_END,
};

static struct clk_freq_tbl clk_tbl_uartdm[] = {
	F_MND16( 3686400, PLL3, 3,   3, 200, NOMINAL),
	F_MND16( 7372800, PLL3, 3,   3, 100, NOMINAL),
	F_MND16(14745600, PLL3, 3,   3,  50, NOMINAL),
	F_MND16(32000000, PLL3, 3,  25, 192, NOMINAL),
	F_MND16(40000000, PLL3, 3, 125, 768, NOMINAL),
	F_MND16(46400000, PLL3, 3, 145, 768, NOMINAL),
	F_MND16(48000000, PLL3, 3,  25, 128, NOMINAL),
	F_MND16(51200000, PLL3, 3,   5,  24, NOMINAL),
	F_MND16(56000000, PLL3, 3, 175, 768, NOMINAL),
	F_MND16(58982400, PLL3, 3,   6,  25, NOMINAL),
	F_MND16(64000000, PLL1, 4,   1,   3, NOMINAL),
	F_END,
};

static struct clk_freq_tbl clk_tbl_mdh[] = {
	F_BASIC( 49150000, PLL3, 15, NOMINAL),
	F_BASIC( 92160000, PLL3,  8, NOMINAL),
	F_BASIC(122880000, PLL3,  6, NOMINAL),
	F_BASIC(184320000, PLL3,  4, NOMINAL),
	F_BASIC(245760000, PLL3,  3, NOMINAL),
	F_BASIC(368640000, PLL3,  2, NOMINAL),
	F_BASIC(384000000, PLL1,  2, NOMINAL),
	F_BASIC(445500000, PLL4,  2, NOMINAL),
	F_END,
};

static struct clk_freq_tbl clk_tbl_grp[] = {
	F_BASIC( 24576000, LPXO,  1, NOMINAL),
	F_BASIC( 46080000, PLL3, 16, NOMINAL),
	F_BASIC( 49152000, PLL3, 15, NOMINAL),
	F_BASIC( 52662875, PLL3, 14, NOMINAL),
	F_BASIC( 56713846, PLL3, 13, NOMINAL),
	F_BASIC( 61440000, PLL3, 12, NOMINAL),
	F_BASIC( 67025454, PLL3, 11, NOMINAL),
	F_BASIC( 73728000, PLL3, 10, NOMINAL),
	F_BASIC( 81920000, PLL3,  9, NOMINAL),
	F_BASIC( 92160000, PLL3,  8, NOMINAL),
	F_BASIC(105325714, PLL3,  7, NOMINAL),
	F_BASIC(122880000, PLL3,  6, NOMINAL),
	F_BASIC(147456000, PLL3,  5, NOMINAL),
	F_BASIC(184320000, PLL3,  4, NOMINAL),
	F_BASIC(192000000, PLL1,  4, NOMINAL),
	F_BASIC(245760000, PLL3,  3, HIGH),
	/* Sync to AXI. Hence this "rate" is not fixed. */
	F_RAW(1, SRC_AXI, 0, B(14), 0, 0, NOMINAL, NULL),
	F_END,
};

static struct clk_freq_tbl clk_tbl_sdc1_3[] = {
	F_MND8(  144000, 19, 12, LPXO, 1,   1,  171, NOMINAL),
	F_MND8(  400000, 19, 12, LPXO, 1,   2,  123, NOMINAL),
	F_MND8(16027000, 19, 12, PLL3, 3,  14,  215, NOMINAL),
	F_MND8(17000000, 19, 12, PLL3, 4,  19,  206, NOMINAL),
	F_MND8(20480000, 19, 12, PLL3, 4,  23,  212, NOMINAL),
	F_MND8(24576000, 19, 12, LPXO, 1,   0,    0, NOMINAL),
	F_MND8(49152000, 19, 12, PLL3, 3,   1,    5, NOMINAL),
	F_END,
};

static struct clk_freq_tbl clk_tbl_sdc2_4[] = {
	F_MND8(  144000, 20, 13, LPXO, 1,   1,  171, NOMINAL),
	F_MND8(  400000, 20, 13, LPXO, 1,   2,  123, NOMINAL),
	F_MND8(16027000, 20, 13, PLL3, 3,  14,  215, NOMINAL),
	F_MND8(17000000, 20, 13, PLL3, 4,  19,  206, NOMINAL),
	F_MND8(20480000, 20, 13, PLL3, 4,  23,  212, NOMINAL),
	F_MND8(24576000, 20, 13, LPXO, 1,   0,    0, NOMINAL),
	F_MND8(49152000, 20, 13, PLL3, 3,   1,    5, NOMINAL),
	F_END,
};

static struct clk_freq_tbl clk_tbl_mdp_core[] = {
	F_BASIC( 46080000, PLL3, 16, NOMINAL),
	F_BASIC( 49152000, PLL3, 15, NOMINAL),
	F_BASIC( 52663000, PLL3, 14, NOMINAL),
	F_BASIC( 92160000, PLL3,  8, NOMINAL),
	F_BASIC(122880000, PLL3,  6, NOMINAL),
	F_BASIC(147456000, PLL3,  5, NOMINAL),
	F_BASIC(153600000, PLL1,  5, NOMINAL),
	F_BASIC(192000000, PLL1,  4, HIGH),
	F_END,
};

static struct clk_freq_tbl clk_tbl_mdp_lcdc[] = {
	F_MND16(24576000, LPXO, 1,   0,   0, NOMINAL),
	F_MND16(30720000, PLL3, 4,   1,   6, NOMINAL),
	F_MND16(32768000, PLL3, 3,   2,  15, NOMINAL),
	F_MND16(40960000, PLL3, 2,   1,   9, NOMINAL),
	F_MND16(73728000, PLL3, 2,   1,   5, NOMINAL),
	F_END,
};

static struct clk_freq_tbl clk_tbl_mdp_vsync[] = {
	F_RAW(24576000, SRC_LPXO, 0, 0, 0, 0, NOMINAL, NULL),
	F_END,
};

static struct clk_freq_tbl clk_tbl_mi2s_codec[] = {
	F_MND16( 2048000, LPXO, 4,   1,   3, NOMINAL),
	F_MND16(12288000, LPXO, 2,   0,   0, NOMINAL),
	F_END,
};

static struct clk_freq_tbl clk_tbl_mi2s[] = {
	F_MND16(12288000, LPXO, 2,   0,   0, NOMINAL),
	F_END,
};

static struct clk_freq_tbl clk_tbl_midi[] = {
	F_MND8(98304000, 19, 12, PLL3, 3,  2,  5, NOMINAL),
	F_END,
};

static struct clk_freq_tbl clk_tbl_sdac[] = {
	F_MND16( 256000, LPXO, 4,   1,    24, NOMINAL),
	F_MND16( 352800, LPXO, 1, 147, 10240, NOMINAL),
	F_MND16( 384000, LPXO, 4,   1,    16, NOMINAL),
	F_MND16( 512000, LPXO, 4,   1,    12, NOMINAL),
	F_MND16( 705600, LPXO, 1, 147,  5120, NOMINAL),
	F_MND16( 768000, LPXO, 4,   1,     8, NOMINAL),
	F_MND16(1024000, LPXO, 4,   1,     6, NOMINAL),
	F_MND16(1411200, LPXO, 1, 147,  2560, NOMINAL),
	F_MND16(1536000, LPXO, 4,   1,     4, NOMINAL),
	F_END,
};

static struct clk_freq_tbl clk_tbl_tv[] = {
	F_MND8(27000000, 23, 16, PLL4, 2,  2,  33, NOMINAL),
	F_MND8(74250000, 23, 16, PLL4, 2,  1,   6, NOMINAL),
	F_END,
};

static struct clk_freq_tbl clk_tbl_usb[] = {
	F_MND8(60000000, 23, 16, PLL1, 2,  5,  32, NOMINAL),
	F_END,
};

static struct clk_freq_tbl clk_tbl_vfe_jpeg[] = {
	F_MND16( 36864000, PLL3, 4,   1,   5, NOMINAL),
	F_MND16( 46080000, PLL3, 4,   1,   4, NOMINAL),
	F_MND16( 61440000, PLL3, 4,   1,   3, NOMINAL),
	F_MND16( 73728000, PLL3, 2,   1,   5, NOMINAL),
	F_MND16( 81920000, PLL3, 3,   1,   3, NOMINAL),
	F_MND16( 92160000, PLL3, 4,   1,   2, NOMINAL),
	F_MND16( 98304000, PLL3, 3,   2,   5, NOMINAL),
	F_MND16(105326000, PLL3, 2,   2,   7, NOMINAL),
	F_MND16(122880000, PLL3, 2,   1,   3, NOMINAL),
	F_MND16(147456000, PLL3, 2,   2,   5, NOMINAL),
	F_MND16(153600000, PLL1, 2,   2,   5, NOMINAL),
	F_END,
};

static struct clk_freq_tbl clk_tbl_cam[] = {
	F_MND16( 6000000, PLL1, 4,   1,  32, NOMINAL),
	F_MND16( 8000000, PLL1, 4,   1,  24, NOMINAL),
	F_MND16(12000000, PLL1, 4,   1,  16, NOMINAL),
	F_MND16(16000000, PLL1, 4,   1,  12, NOMINAL),
	F_MND16(19200000, PLL1, 4,   1,  10, NOMINAL),
	F_MND16(24000000, PLL1, 4,   1,   8, NOMINAL),
	F_MND16(32000000, PLL1, 4,   1,   6, NOMINAL),
	F_MND16(48000000, PLL1, 4,   1,   4, NOMINAL),
	F_MND16(64000000, PLL1, 4,   1,   3, NOMINAL),
	F_END,
};

static struct clk_freq_tbl clk_tbl_vpe[] = {
	F_MND8( 24576000, 22, 15, LPXO, 1,   0,   0, NOMINAL),
	F_MND8( 30720000, 22, 15, PLL3, 4,   1,   6, NOMINAL),
	F_MND8( 61440000, 22, 15, PLL3, 4,   1,   3, NOMINAL),
	F_MND8( 81920000, 22, 15, PLL3, 3,   1,   3, NOMINAL),
	F_MND8(122880000, 22, 15, PLL3, 3,   1,   2, NOMINAL),
	F_MND8(147456000, 22, 15, PLL3, 1,   1,   5, NOMINAL),
	F_MND8(153600000, 22, 15, PLL1, 1,   1,   5, NOMINAL),
	F_END,
};

static struct clk_freq_tbl clk_tbl_mfc[] = {
	F_MND8( 24576000, 24, 17, LPXO, 1,   0,   0, NOMINAL),
	F_MND8( 30720000, 24, 17, PLL3, 4,   1,   6, NOMINAL),
	F_MND8( 61440000, 24, 17, PLL3, 4,   1,   3, NOMINAL),
	F_MND8( 81920000, 24, 17, PLL3, 3,   1,   3, NOMINAL),
	F_MND8(122880000, 24, 17, PLL3, 3,   1,   2, NOMINAL),
	F_MND8(147456000, 24, 17, PLL3, 1,   1,   5, NOMINAL),
	F_MND8(153600000, 24, 17, PLL1, 1,   1,   5, NOMINAL),
	F_MND8(170667000, 24, 17, PLL1, 1,   2,   9, NOMINAL),
	F_END,
};

static struct clk_freq_tbl clk_tbl_spi[] = {
	F_MND8( 9963243, 19, 12, PLL3, 4,   7,   129, NOMINAL),
	F_MND8(26331429, 19, 12, PLL3, 4,  34,   241, NOMINAL),
	F_END,
};

static struct clk_freq_tbl clk_tbl_lpa_codec[] = {
	F_RAW(1, SRC_NONE, 0, 0, 0, 0, LOW, NULL), /* src = MI2S_CODEC_RX */
	F_RAW(2, SRC_NONE, 0, 1, 0, 0, LOW, NULL), /* src = ECODEC_CIF */
	F_RAW(3, SRC_NONE, 0, 2, 0, 0, LOW, NULL), /* src = MI2S */
	F_RAW(4, SRC_NONE, 0, 3, 0, 0, LOW, NULL), /* src = SDAC */
	F_END,
};

/*
 * Clock children lists
 */
static const uint32_t chld_grp_3d_src[] = 	{C(IMEM), C(GRP_3D), C(NONE)};
static const uint32_t chld_mdp_lcdc_p[] = 	{C(MDP_LCDC_PAD_PCLK), C(NONE)};
static const uint32_t chld_mfc[] = 		{C(MFC_DIV2), C(NONE)};
static const uint32_t chld_mi2s_codec_rx[] =	{C(MI2S_CODEC_RX_S), C(NONE)};
static const uint32_t chld_mi2s_codec_tx[] =	{C(MI2S_CODEC_TX_S), C(NONE)};
static const uint32_t chld_mi2s[] = 		{C(MI2S_S), C(NONE)};
static const uint32_t chld_sdac[] = 		{C(SDAC_M), C(NONE)};
static const uint32_t chld_tv[] = 		{C(TV_DAC), C(TV_ENC), C(HDMI),
						 C(TSIF_REF), C(NONE)};
static const uint32_t chld_usb_src[] = 	{C(USB_HS), C(USB_HS_CORE),
					 C(USB_HS2), C(USB_HS2_CORE),
					 C(USB_HS3), C(USB_HS3_CORE),
					 C(NONE)};
static uint32_t const chld_vfe[] =	{C(VFE_MDC), C(VFE_CAMIF), C(CSI0_VFE),
					 C(NONE)};

/*
 * Clock declaration macros
 */
#define CLK_BASIC(id, ns, br, root, tbl, par, h_r, h_c, h_b, tv) \
		CLK(id, BASIC, ns, ns, NULL, NULL, 0, h_r, h_c, \
			h_b, br, root, F_MASK_BASIC, 0, set_rate_basic, tbl, \
			NULL, par, NULL, tv)
#define CLK_MND8_P(id, ns, m, l, br, root, tbl, par, chld_lst, \
						h_r, h_c, h_b, tv) \
		CLK(id, MND, ns, ns, (ns-4), NULL, 0, h_r, h_c, \
			h_b, br, root, F_MASK_MND8(m, l), 0, set_rate_mnd, \
			tbl, NULL, par, chld_lst, tv)
#define CLK_MND8(id, ns, m, l, br, root, tbl, chld_lst, h_r, h_c, h_b, tv) \
		CLK_MND8_P(id, ns, m, l, br, root, tbl, NONE, chld_lst, \
							h_r, h_c, h_b, tv)
#define CLK_MND16(id, ns, br, root, tbl, par, chld_lst, h_r, h_c, h_b, tv) \
		CLK(id, MND, ns, ns, (ns-4), NULL, 0, h_r, h_c, \
			h_b, br, root, F_MASK_MND16, 0, set_rate_mnd, tbl, \
			NULL, par, chld_lst, tv)
#define CLK_1RATE(id, ns, br, root, tbl, h_r, h_c, h_b, tv) \
		CLK(id, BASIC, NULL, ns, NULL, NULL, 0, h_r, h_c, \
			h_b, br, root, 0, 0, set_rate_nop, tbl, \
			NULL, NONE, NULL, tv)
#define CLK_SLAVE(id, ns, br, par, h_r, h_c, h_b, tv) \
		CLK(id, NORATE, NULL, ns, NULL, NULL, 0, h_r, h_c, \
			h_b, br, 0, 0, 0, NULL, NULL, NULL, par, NULL, tv)
#define CLK_NORATE(id, ns, br, root, h_r, h_c, h_b, tv) \
		CLK(id, NORATE, NULL, ns, NULL, NULL, 0, h_r, h_c, \
			h_b, br, root, 0, 0, NULL, NULL, NULL, NONE, NULL, tv)
#define CLK_GLBL(id, glbl, br, h_r, h_c, h_b, tv) \
		CLK(id, NORATE, NULL, glbl, NULL, NULL, 0, h_r, h_c, \
			h_b, br, 0, 0, 0, NULL, NULL, NULL, NONE, NULL, tv)
#define CLK_BRIDGE(id, glbl, br, par, h_r, h_c, h_b, tv) \
		CLK(id, NORATE, NULL, glbl, NULL, NULL, 0, h_r, h_c, \
			h_b, br, 0, 0, 0, NULL, NULL, NULL, par, NULL, tv)

static DEFINE_CLK_PCOM(adsp_clk, ADSP_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(codec_ssbi_clk,	CODEC_SSBI_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(ebi1_clk, EBI1_CLK, CLKFLAG_SKIP_AUTO_OFF | CLKFLAG_MIN);
static DEFINE_CLK_PCOM(ebi1_fixed_clk, EBI1_FIXED_CLK, CLKFLAG_MIN |
						       CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(ecodec_clk, ECODEC_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(gp_clk, GP_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(uart3_clk, UART3_CLK, 0);
static DEFINE_CLK_PCOM(usb_phy_clk, USB_PHY_CLK, CLKFLAG_MIN);

static DEFINE_CLK_PCOM(p_grp_2d_clk, GRP_2D_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_grp_2d_p_clk, GRP_2D_P_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_hdmi_clk, HDMI_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_jpeg_clk, JPEG_CLK, CLKFLAG_MIN);
static DEFINE_CLK_PCOM(p_jpeg_p_clk, JPEG_P_CLK, 0);
static DEFINE_CLK_PCOM(p_lpa_codec_clk, LPA_CODEC_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_lpa_core_clk, LPA_CORE_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_lpa_p_clk, LPA_P_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_mi2s_m_clk, MI2S_M_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_mi2s_s_clk, MI2S_S_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_mi2s_codec_rx_m_clk, MI2S_CODEC_RX_M_CLK,
		CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_mi2s_codec_rx_s_clk, MI2S_CODEC_RX_S_CLK,
		CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_mi2s_codec_tx_m_clk, MI2S_CODEC_TX_M_CLK,
		CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_mi2s_codec_tx_s_clk, MI2S_CODEC_TX_S_CLK,
		CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_sdac_clk, SDAC_CLK, 0);
static DEFINE_CLK_PCOM(p_sdac_m_clk, SDAC_M_CLK, 0);
static DEFINE_CLK_PCOM(p_vfe_clk, VFE_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_vfe_camif_clk, VFE_CAMIF_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_vfe_mdc_clk, VFE_MDC_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_vfe_p_clk, VFE_P_CLK, 0);
static DEFINE_CLK_PCOM(p_grp_3d_clk, GRP_3D_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_grp_3d_p_clk, GRP_3D_P_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_imem_clk, IMEM_CLK, 0);
static DEFINE_CLK_PCOM(p_mdp_lcdc_pad_pclk_clk, MDP_LCDC_PAD_PCLK_CLK,
		CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_mdp_lcdc_pclk_clk, MDP_LCDC_PCLK_CLK,
		CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_mdp_p_clk, MDP_P_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_mdp_vsync_clk, MDP_VSYNC_CLK, 0);
static DEFINE_CLK_PCOM(p_tsif_ref_clk, TSIF_REF_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_tsif_p_clk, TSIF_P_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_tv_dac_clk, TV_DAC_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_tv_enc_clk, TV_ENC_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_emdh_clk, EMDH_CLK, CLKFLAG_MIN | CLKFLAG_MAX);
static DEFINE_CLK_PCOM(p_emdh_p_clk, EMDH_P_CLK, 0);
static DEFINE_CLK_PCOM(p_i2c_clk, I2C_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_i2c_2_clk, I2C_2_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_mdc_clk, MDC_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_pmdh_clk, PMDH_CLK, CLKFLAG_MIN | CLKFLAG_MAX);
static DEFINE_CLK_PCOM(p_pmdh_p_clk, PMDH_P_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_sdc1_clk, SDC1_CLK, 0);
static DEFINE_CLK_PCOM(p_sdc1_p_clk, SDC1_P_CLK, 0);
static DEFINE_CLK_PCOM(p_sdc2_clk, SDC2_CLK, 0);
static DEFINE_CLK_PCOM(p_sdc2_p_clk, SDC2_P_CLK, 0);
static DEFINE_CLK_PCOM(p_sdc3_clk, SDC3_CLK, 0);
static DEFINE_CLK_PCOM(p_sdc3_p_clk, SDC3_P_CLK, 0);
static DEFINE_CLK_PCOM(p_sdc4_clk, SDC4_CLK, 0);
static DEFINE_CLK_PCOM(p_sdc4_p_clk, SDC4_P_CLK, 0);
static DEFINE_CLK_PCOM(p_uart2_clk, UART2_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_usb_hs2_clk, USB_HS2_CLK, 0);
static DEFINE_CLK_PCOM(p_usb_hs2_core_clk, USB_HS2_CORE_CLK, 0);
static DEFINE_CLK_PCOM(p_usb_hs2_p_clk, USB_HS2_P_CLK, 0);
static DEFINE_CLK_PCOM(p_usb_hs3_clk, USB_HS3_CLK, 0);
static DEFINE_CLK_PCOM(p_usb_hs3_core_clk, USB_HS3_CORE_CLK, 0);
static DEFINE_CLK_PCOM(p_usb_hs3_p_clk, USB_HS3_P_CLK, 0);
static DEFINE_CLK_PCOM(p_qup_i2c_clk, QUP_I2C_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_spi_clk, SPI_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_spi_p_clk, SPI_P_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_uart1_clk, UART1_CLK, 0);
static DEFINE_CLK_PCOM(p_uart1dm_clk, UART1DM_CLK, 0);
static DEFINE_CLK_PCOM(p_uart2dm_clk, UART2DM_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_usb_hs_clk, USB_HS_CLK, 0);
static DEFINE_CLK_PCOM(p_usb_hs_core_clk, USB_HS_CORE_CLK, 0);
static DEFINE_CLK_PCOM(p_usb_hs_p_clk, USB_HS_P_CLK, 0);
static DEFINE_CLK_PCOM(p_cam_m_clk, CAM_M_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_camif_pad_p_clk, CAMIF_PAD_P_CLK, 0);
static DEFINE_CLK_PCOM(p_csi0_clk, CSI0_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_csi0_vfe_clk, CSI0_VFE_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_csi0_p_clk, CSI0_P_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_mdp_clk, MDP_CLK, CLKFLAG_MIN);
static DEFINE_CLK_PCOM(p_mfc_clk, MFC_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_mfc_div2_clk, MFC_DIV2_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_mfc_p_clk, MFC_P_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_vpe_clk, VPE_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_adm_clk, ADM_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_ce_clk, CE_CLK, CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_axi_rotator_clk, AXI_ROTATOR_CLK,
		CLKFLAG_SKIP_AUTO_OFF);
static DEFINE_CLK_PCOM(p_rotator_imem_clk, ROTATOR_IMEM_CLK, 0);
static DEFINE_CLK_PCOM(p_rotator_p_clk, ROTATOR_P_CLK, 0);

static DEFINE_CLK_VOTER(ebi_dtv_clk, &ebi1_fixed_clk.c);
static DEFINE_CLK_VOTER(ebi_grp_3d_clk, &ebi1_fixed_clk.c);
static DEFINE_CLK_VOTER(ebi_grp_2d_clk, &ebi1_fixed_clk.c);
static DEFINE_CLK_VOTER(ebi_lcdc_clk, &ebi1_fixed_clk.c);
static DEFINE_CLK_VOTER(ebi_mddi_clk, &ebi1_fixed_clk.c);
static DEFINE_CLK_VOTER(ebi_tv_clk, &ebi1_fixed_clk.c);
static DEFINE_CLK_VOTER(ebi_vcd_clk, &ebi1_fixed_clk.c);
static DEFINE_CLK_VOTER(ebi_vfe_clk, &ebi1_fixed_clk.c);
static DEFINE_CLK_VOTER(ebi_adm_clk, &ebi1_fixed_clk.c);

#ifdef CONFIG_DEBUG_FS

#define CLK_TEST_2(s) (s)
#define CLK_TEST_HS(s) (0x4000 | ((s) << 8))
#define CLK_TEST_LS(s) (0x4D40 | (s))

struct measure_sel {
	u32 test_vector;
	struct clk *clk;
};

/*
 * SoC-specific functions required by clock-local driver
 */

/* Update the sys_vdd voltage given a level. */
int soc_update_sys_vdd(enum sys_vdd_level level)
{
	int rc, target_mv;
	static const int mv[NUM_SYS_VDD_LEVELS] = {
		[NONE...LOW] = 1000,
		[NOMINAL] = 1100,
		[HIGH]    = 1200,
	};

	target_mv = mv[level];
	rc = msm_proc_comm(PCOM_CLKCTL_RPC_MIN_MSMC1, &target_mv, NULL);
	if (rc)
		goto out;
	if (target_mv) {
		rc = -EINVAL;
		goto out;
	}
out:
	return rc;
}

/* Enable/disable a power rail associated with a clock. */
int soc_set_pwr_rail(unsigned id, int enable)
{
	int pwr_id = 0;
	switch (id) {
	case C(AXI_ROTATOR):
		pwr_id = PWR_RAIL_ROTATOR_CLK;
		break;
	case C(GRP_2D):
		pwr_id = PWR_RAIL_GRP_2D_CLK;
		break;
	case C(GRP_3D):
		pwr_id = PWR_RAIL_GRP_CLK;
		break;
	case C(MDP):
		pwr_id = PWR_RAIL_MDP_CLK;
		break;
	case C(MFC):
		pwr_id = PWR_RAIL_MFC_CLK;
		break;
	case C(VFE):
		pwr_id = PWR_RAIL_VFE_CLK;
		break;
	case C(VPE):
		pwr_id = PWR_RAIL_VPE_CLK;
		break;
	default:
		return 0;
	}

	return internal_pwr_rail_ctl_auto(pwr_id, enable);
}

/* Sample clock for 'tcxo4_ticks' reference clock ticks. */
static uint32_t run_measurement(unsigned tcxo4_ticks)
{
	/* TCXO4_CNT_EN and RINGOSC_CNT_EN register values. */
	uint32_t reg_val_enable = readl(MISC_CLK_CTL_BASE_REG) | 0x3;
	uint32_t reg_val_disable = reg_val_enable & ~0x3;

	/* Stop counters and set the TCXO4 counter start value. */
	writel(reg_val_disable, MISC_CLK_CTL_BASE_REG);
	writel(tcxo4_ticks, TCXO_CNT_BASE_REG);

	/* Run measurement and wait for completion. */
	writel(reg_val_enable, MISC_CLK_CTL_BASE_REG);
	while (readl(TCXO_CNT_DONE_BASE_REG) == 0)
		cpu_relax();

	/* Stop counters. */
	writel(reg_val_disable, MISC_CLK_CTL_BASE_REG);

	return readl(RINGOSC_CNT_BASE_REG);
}

/* Perform a hardware rate measurement for a given clock.
   FOR DEBUG USE ONLY: Measurements take ~15 ms! */
signed soc_clk_measure_rate(unsigned id)
{
	struct clk_local *t = &soc_clk_local_tbl[id];
	unsigned long flags;
	uint32_t regval, prph_web_reg_old;
	uint64_t raw_count_short, raw_count_full;
	signed ret;

	if (t->test_vector == 0)
		return -EPERM;

	spin_lock_irqsave(&local_clock_reg_lock, flags);

	/* Program test vector. */
	if (t->test_vector <= 0xFF) {
		/* Select CLK_TEST_2 */
		writel(0x4D40, CLK_TEST_BASE_REG);
		writel(t->test_vector, CLK_TEST_2_BASE_REG);
	} else
		writel(t->test_vector, CLK_TEST_BASE_REG);

	/* Enable TCXO4 clock branch and root. */
	prph_web_reg_old = readl(PRPH_WEB_NS_BASE_REG);
	regval = prph_web_reg_old | B(9) | B(11);
	local_src_enable(TCXO);
	writel(regval, PRPH_WEB_NS_BASE_REG);

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

	/* Disable TCXO4 clock branch and root. */
	writel(prph_web_reg_old, PRPH_WEB_NS_BASE_REG);
	local_src_disable(TCXO);

	/* Return 0 if the clock is off. */
	if (raw_count_full == raw_count_short)
		ret = 0;
	else {
		/* Compute rate in Hz. */
		raw_count_full = ((raw_count_full * 10) + 15) * 4800000;
		do_div(raw_count_full, ((0x10000 * 10) + 35));
		ret = (signed)raw_count_full;
	}

	spin_unlock_irqrestore(&local_clock_reg_lock, flags);

	return ret;
}

/* Implementation for clk_set_flags(). */
int soc_clk_set_flags(unsigned id, unsigned clk_flags)
{
	uint32_t regval, ret = 0;
	unsigned long flags;

	spin_lock_irqsave(&local_clock_reg_lock, flags);
	switch (id) {
	case C(VFE):
		regval = readl(CAM_VFE_NS_REG);
		/* Flag values chosen for backward compatibility
		 * with proc_comm remote clock control. */
		if (clk_flags == 0x00000100) {
			/* Select external source. */
			regval |= B(14);
		} else if (clk_flags == 0x00000200) {
			/* Select internal source. */
			regval &= ~B(14);
		} else
			ret = -EINVAL;

		writel(regval, CAM_VFE_NS_REG);
		break;
	default:
		ret = -EPERM;
	}
	spin_unlock_irqrestore(&local_clock_reg_lock, flags);

	return ret;
}

/* Implementation for clk_reset(). */
int soc_clk_reset(unsigned id, enum clk_reset_action action)
{
	/* Local clock resets are not support on 7x30. */
	return -EPERM;
}

/* Enable function for TCXO and LPXO. */
static int soc_xo_enable(unsigned src, unsigned enable)
{
	unsigned pcom_xo_id;

	if (src == TCXO)
		pcom_xo_id = 0;
	else if (src == LPXO)
		pcom_xo_id = 1;
	else
		return 0;

	return msm_proc_comm(PCOM_CLKCTL_RPC_SRC_REQUEST, &pcom_xo_id, &enable);
}

/* Enable function for PLLs. */
static int soc_pll_enable(unsigned src, unsigned enable)
{
	static const struct pll_ena {
		uint32_t *const reg;
		const uint32_t mask;
	} soc_pll_ena[NUM_SRC] = {
		[PLL_0] = {PLL_ENA_REG, B(0)},
		[PLL_1] = {PLL_ENA_REG, B(1)},
		[PLL_2] = {PLL_ENA_REG, B(2)},
		[PLL_3] = {PLL_ENA_REG, B(3)},
		[PLL_4] = {PLL_ENA_REG, B(4)},
		[PLL_5] = {PLL_ENA_REG, B(5)},
		[PLL_6] = {PLL_ENA_REG, B(6)},
	};
	uint32_t *const soc_pll_status_reg[NUM_SRC] = {

		[PLL_0] = PLL0_STATUS_BASE_REG,
		[PLL_1] = PLL1_STATUS_BASE_REG,
		[PLL_2] = PLL2_STATUS_BASE_REG,
		[PLL_3] = PLL3_STATUS_BASE_REG,
		[PLL_4] = PLL4_STATUS_BASE_REG,
		[PLL_5] = PLL5_STATUS_BASE_REG,
		[PLL_6] = PLL6_STATUS_BASE_REG,
	};
	uint32_t reg_val;

	reg_val = readl(soc_pll_ena[src].reg);

	if (enable)
		reg_val |= soc_pll_ena[src].mask;
	else
		reg_val &= ~(soc_pll_ena[src].mask);

	writel(reg_val, soc_pll_ena[src].reg);

	if (enable) {
		/* Wait until PLL is enabled */
		while ((readl(soc_pll_status_reg[src]) & B(16)) == 0)
			cpu_relax();
	}

	return 0;
}

struct clk_source soc_clk_sources[NUM_SRC] = {
	[TCXO] =	{	.enable_func = soc_xo_enable,
				.par = SRC_NONE,
			},
	[LPXO] =	{	.enable_func = soc_xo_enable,
				.par = SRC_NONE,
			},
	[AXI] =		{	.enable_func = NULL,
				.par = LPXO,
			},
	[PLL_0] =	{	.enable_func = soc_pll_enable,
				.par = TCXO,
			},
	[PLL_1] =	{	.enable_func = soc_pll_enable,
				.par = TCXO,
			},
	[PLL_2] =	{	.enable_func = soc_pll_enable,
				.par = TCXO,
			},
	[PLL_3] =	{	.enable_func = soc_pll_enable,
				.par = LPXO,
			},
	[PLL_4] =	{	.enable_func = soc_pll_enable,
				.par = LPXO,
			},
	[PLL_5] =	{	.enable_func = soc_pll_enable,
				.par = TCXO,
			},
	[PLL_6] =	{	.enable_func = soc_pll_enable,
				.par = TCXO,
			},
};

/*
 * Clock ownership detection code
 */

enum {
	SH2_OWN_GLBL,
	SH2_OWN_APPS1,
	SH2_OWN_APPS2,
	SH2_OWN_ROW1,
	SH2_OWN_ROW2,
	SH2_OWN_APPS3,
	NUM_OWNERSHIP
};
static __initdata uint32_t ownership_regs[NUM_OWNERSHIP];

static void __init cache_ownership(void)
{
	ownership_regs[SH2_OWN_GLBL]  = readl(SH2_OWN_GLBL_BASE_REG);
	ownership_regs[SH2_OWN_APPS1] = readl(SH2_OWN_APPS1_BASE_REG);
	ownership_regs[SH2_OWN_APPS2] = readl(SH2_OWN_APPS2_BASE_REG);
	ownership_regs[SH2_OWN_ROW1]  = readl(SH2_OWN_ROW1_BASE_REG);
	ownership_regs[SH2_OWN_ROW2]  = readl(SH2_OWN_ROW2_BASE_REG);
	ownership_regs[SH2_OWN_APPS3] = readl(SH2_OWN_APPS3_BASE_REG);
}

static void __init print_ownership(void)
{
	pr_info("Clock ownership\n");
	pr_info("  GLBL  : %08x\n", ownership_regs[SH2_OWN_GLBL]);
	pr_info("  APPS  : %08x %08x %08x\n", ownership_regs[SH2_OWN_APPS1],
		ownership_regs[SH2_OWN_APPS2], ownership_regs[SH2_OWN_APPS3]);
	pr_info("  ROW   : %08x %08x\n", ownership_regs[SH2_OWN_ROW1],
		ownership_regs[SH2_OWN_ROW2]);
}

#define O(x) (&ownership_regs[(SH2_OWN_##x)])
#define OWN(r, b, name, clk, dev) \
	{ \
		.lk = CLK_LOOKUP(name, clk.c, dev), \
		.remote = &p_##clk.c, \
		.reg = O(r), \
		.bit = BIT(b), \
	}

static struct clk_local_ownership {
	struct clk_lookup lk;
	const u32 *reg;
	const u32 bit;
	struct clk *remote;
} ownership_map[] __initdata = {
	/* Sources */
	{ CLK_LOOKUP("pll1_clk",	pll1_clk.c,	"acpu") },
	{ CLK_LOOKUP("pll2_clk",	pll2_clk.c,	"acpu") },
	{ CLK_LOOKUP("pll3_clk",	pll3_clk.c,	"acpu") },
	{ CLK_LOOKUP("measure",		measure_clk,	"debug") },

	/* PCOM */
	{ CLK_LOOKUP("adsp_clk",	adsp_clk.c,	NULL) },
	{ CLK_LOOKUP("codec_ssbi_clk",	codec_ssbi_clk.c,	NULL) },
	{ CLK_LOOKUP("ebi1_clk",	ebi1_clk.c,	NULL) },
	{ CLK_LOOKUP("ebi1_fixed_clk",	ebi1_fixed_clk.c,	NULL) },
	{ CLK_LOOKUP("ecodec_clk",	ecodec_clk.c,	NULL) },
	{ CLK_LOOKUP("gp_clk",		gp_clk.c,	NULL) },
	{ CLK_LOOKUP("core_clk",	uart3_clk.c,	"msm_serial.2") },
	{ CLK_LOOKUP("usb_phy_clk",	usb_phy_clk.c,	NULL) },

	/* Voters */
	{ CLK_LOOKUP("ebi1_dtv_clk",	ebi_dtv_clk.c,	NULL) },
	{ CLK_LOOKUP("bus_clk",		ebi_grp_2d_clk.c, "kgsl-2d0.0") },
	{ CLK_LOOKUP("bus_clk",		ebi_grp_3d_clk.c, "kgsl-3d0.0") },
	{ CLK_LOOKUP("ebi1_lcdc_clk",	ebi_lcdc_clk.c,	NULL) },
	{ CLK_LOOKUP("ebi1_mddi_clk",	ebi_mddi_clk.c,	NULL) },
	{ CLK_LOOKUP("ebi1_tv_clk",	ebi_tv_clk.c,	NULL) },
	{ CLK_LOOKUP("mem_clk",		ebi_vcd_clk.c,	"msm_vidc.0") },
	{ CLK_LOOKUP("ebi1_vfe_clk",	ebi_vfe_clk.c,	NULL) },
	{ CLK_LOOKUP("mem_clk",		ebi_adm_clk.c,	"msm_dmov") },

	/*
	 * This is a many-to-one mapping because we don't know how the remote
	 * clock code has decided to handle the dependencies between clocks for
	 * a particular hardware block. We determine the ownership for all the
	 * clocks going into a block by checking the ownership bit of one
	 * register (usually the ns register).
	 */
	OWN(APPS1,  6, "core_clk",	grp_2d_clk,	"kgsl-2d0.0"),
	OWN(APPS1,  6, "core_clk",	grp_2d_clk,	"footswitch-pcom.0"),
	OWN(APPS1,  6, "iface_clk",	grp_2d_p_clk,	"kgsl-2d0.0"),
	OWN(APPS1,  6, "iface_clk",	grp_2d_p_clk,	"footswitch-pcom.0"),
	OWN(APPS1, 31, "hdmi_clk",	hdmi_clk,	NULL),
	OWN(APPS1,  0, "jpeg_clk",	jpeg_clk,	NULL),
	OWN(APPS1,  0, "jpeg_pclk",	jpeg_p_clk,	NULL),
	OWN(APPS1, 23, "lpa_codec_clk", lpa_codec_clk,	NULL),
	OWN(APPS1, 23, "lpa_core_clk",	lpa_core_clk,	NULL),
	OWN(APPS1, 23, "lpa_pclk",	lpa_p_clk,	NULL),
	OWN(APPS1, 28, "mi2s_m_clk",	mi2s_m_clk,	NULL),
	OWN(APPS1, 28, "mi2s_s_clk",	mi2s_s_clk,	NULL),
	OWN(APPS1, 12, "mi2s_codec_rx_m_clk", mi2s_codec_rx_m_clk, NULL),
	OWN(APPS1, 12, "mi2s_codec_rx_s_clk", mi2s_codec_rx_s_clk, NULL),
	OWN(APPS1, 14, "mi2s_codec_tx_m_clk", mi2s_codec_tx_m_clk, NULL),
	OWN(APPS1, 14, "mi2s_codec_tx_s_clk", mi2s_codec_tx_s_clk, NULL),
	{ CLK_LOOKUP("midi_clk",        midi_clk.c,     NULL),
		O(APPS1), BIT(22) },
	OWN(APPS1, 26, "sdac_clk",	sdac_clk,	NULL),
	OWN(APPS1, 26, "sdac_m_clk",	sdac_m_clk,	NULL),
	OWN(APPS1,  8, "vfe_clk",	vfe_clk,	NULL),
	OWN(APPS1,  8, "core_clk",	vfe_clk,	"footswitch-pcom.8"),
	OWN(APPS1,  8, "vfe_camif_clk", vfe_camif_clk,	NULL),
	OWN(APPS1,  8, "vfe_mdc_clk",	vfe_mdc_clk,	NULL),
	OWN(APPS1,  8, "vfe_pclk",	vfe_p_clk,	NULL),
	OWN(APPS1,  8, "iface_clk",	vfe_p_clk,	"footswitch-pcom.8"),

	OWN(APPS2,  0, "core_clk",	grp_3d_clk,	"kgsl-3d0.0"),
	OWN(APPS2,  0, "core_clk",	grp_3d_clk,	"footswitch-pcom.2"),
	OWN(APPS2,  0, "iface_clk",	grp_3d_p_clk,	"kgsl-3d0.0"),
	OWN(APPS2,  0, "iface_clk",	grp_3d_p_clk,	"footswitch-pcom.2"),
	{ CLK_LOOKUP("src_clk",     grp_3d_src_clk.c, "kgsl-3d0.0"),
		O(APPS2), BIT(0), &p_grp_3d_clk.c },
	{ CLK_LOOKUP("src_clk",     grp_3d_src_clk.c, "footswitch-pcom.2"),
		O(APPS2), BIT(0), &p_grp_3d_clk.c },
	OWN(APPS2,  0, "mem_clk",	imem_clk,	"kgsl-3d0.0"),
	OWN(APPS2,  4, "mdp_lcdc_pad_pclk_clk", mdp_lcdc_pad_pclk_clk, NULL),
	OWN(APPS2,  4, "mdp_lcdc_pclk_clk", mdp_lcdc_pclk_clk, NULL),
	OWN(APPS2,  4, "mdp_pclk",	mdp_p_clk,	NULL),
	OWN(APPS2,  4, "iface_clk",	mdp_p_clk,	"footswitch-pcom.4"),
	OWN(APPS2, 28, "mdp_vsync_clk", mdp_vsync_clk,	NULL),
	OWN(APPS2,  5, "ref_clk",	tsif_ref_clk,	"msm_tsif.0"),
	OWN(APPS2,  5, "iface_clk",	tsif_p_clk,	"msm_tsif.0"),
	{ CLK_LOOKUP("tv_src_clk",      tv_clk.c,       NULL),
		O(APPS2), BIT(2), &p_tv_enc_clk.c },
	OWN(APPS2,  2, "tv_dac_clk",	tv_dac_clk,	NULL),
	OWN(APPS2,  2, "tv_enc_clk",	tv_enc_clk,	NULL),

	OWN(ROW1,  7, "emdh_clk",	emdh_clk,	"msm_mddi.1"),
	OWN(ROW1,  7, "emdh_pclk",	emdh_p_clk,	"msm_mddi.1"),
	OWN(ROW1, 11, "core_clk",	i2c_clk,	"msm_i2c.0"),
	OWN(ROW1, 12, "core_clk",	i2c_2_clk,	"msm_i2c.2"),
	OWN(ROW1, 17, "mdc_clk",	mdc_clk,	NULL),
	OWN(ROW1, 19, "mddi_clk",	pmdh_clk,	NULL),
	OWN(ROW1, 19, "mddi_pclk",	pmdh_p_clk,	NULL),
	OWN(ROW1, 23, "core_clk",	sdc1_clk,	"msm_sdcc.1"),
	OWN(ROW1, 23, "iface_clk",	sdc1_p_clk,	"msm_sdcc.1"),
	OWN(ROW1, 25, "core_clk",	sdc2_clk,	"msm_sdcc.2"),
	OWN(ROW1, 25, "iface_clk",	sdc2_p_clk,	"msm_sdcc.2"),
	OWN(ROW1, 27, "core_clk",	sdc3_clk,	"msm_sdcc.3"),
	OWN(ROW1, 27, "iface_clk",	sdc3_p_clk,	"msm_sdcc.3"),
	OWN(ROW1, 29, "core_clk",	sdc4_clk,	"msm_sdcc.4"),
	OWN(ROW1, 29, "iface_clk",	sdc4_p_clk,	"msm_sdcc.4"),
	OWN(ROW1,  0, "core_clk",	uart2_clk,	"msm_serial.1"),
	OWN(ROW1,  2, "usb_hs2_clk",	usb_hs2_clk,	NULL),
	OWN(ROW1,  2, "usb_hs2_core_clk", usb_hs2_core_clk, NULL),
	OWN(ROW1,  2, "usb_hs2_pclk",	usb_hs2_p_clk,	NULL),
	OWN(ROW1,  4, "usb_hs3_clk",	usb_hs3_clk,	NULL),
	OWN(ROW1,  4, "usb_hs3_core_clk", usb_hs3_core_clk, NULL),
	OWN(ROW1,  4, "usb_hs3_pclk",	usb_hs3_p_clk,	NULL),

	OWN(ROW2,  3, "core_clk",	qup_i2c_clk,	"qup_i2c.4"),
	OWN(ROW2,  1, "core_clk",	spi_clk,	"spi_qsd.0"),
	OWN(ROW2,  1, "iface_clk",	spi_p_clk,	"spi_qsd.0"),
	OWN(ROW2,  9, "core_clk",	uart1_clk,	"msm_serial.0"),
	OWN(ROW2,  6, "core_clk",	uart1dm_clk,	"msm_serial_hs.0"),
	OWN(ROW2,  8, "core_clk",	uart2dm_clk,	"msm_serial_hs.1"),
	OWN(ROW2, 11, "usb_hs_clk",	usb_hs_clk,	NULL),
	OWN(ROW2, 11, "usb_hs_core_clk", usb_hs_core_clk, NULL),
	OWN(ROW2, 11, "usb_hs_pclk",	usb_hs_p_clk,	NULL),

	OWN(APPS3,  6, "cam_m_clk",	cam_m_clk,	NULL),
	OWN(APPS3,  6, "camif_pad_pclk", camif_pad_p_clk, NULL),
	OWN(APPS3,  6, "iface_clk",	camif_pad_p_clk, "qup_i2c.4"),
	OWN(APPS3, 11, "csi_clk",	csi0_clk,	NULL),
	OWN(APPS3, 11, "csi_vfe_clk",	csi0_vfe_clk,	NULL),
	OWN(APPS3, 11, "csi_pclk",	csi0_p_clk,	NULL),
	OWN(APPS3,  0, "mdp_clk",	mdp_clk,	NULL),
	OWN(APPS3,  0, "core_clk",	mdp_clk,	"footswitch-pcom.4"),
	OWN(APPS3,  2, "core_clk",	mfc_clk,	"msm_vidc.0"),
	OWN(APPS3,  2, "core_clk",	mfc_clk,	"footswitch-pcom.5"),
	OWN(APPS3,  2, "core_div2_clk",	mfc_div2_clk,	"msm_vidc.0"),
	OWN(APPS3,  2, "iface_clk",	mfc_p_clk,	"msm_vidc.0"),
	OWN(APPS3,  2, "iface_clk",	mfc_p_clk,	"footswitch-pcom.5"),
	OWN(APPS3,  4, "vpe_clk",	vpe_clk,	NULL),
	OWN(APPS3,  4, "core_clk",	vpe_clk,	"footswitch-pcom.9"),

	OWN(GLBL,  8, "core_clk",	adm_clk,	"msm_dmov"),
	{ CLK_LOOKUP("iface_clk",		adm_p_clk.c,	"msm_dmov"),
		O(GLBL), BIT(13), &dummy_clk },
	OWN(GLBL,  8, "core_clk",	ce_clk,		"qce.0"),
	OWN(GLBL,  8, "core_clk",	ce_clk,		"crypto.0"),
	OWN(GLBL, 13, "rotator_clk",	axi_rotator_clk, NULL),
	OWN(GLBL, 13, "core_clk",	axi_rotator_clk, "footswitch-pcom.6"),
	OWN(GLBL, 13, "rotator_imem_clk", rotator_imem_clk, NULL),
	OWN(GLBL, 13, "rotator_pclk",	rotator_p_clk,	NULL),
	OWN(GLBL, 13, "iface_clk",	rotator_p_clk,	"footswitch-pcom.6"),
	{ CLK_LOOKUP("iface_clk",     uart1dm_p_clk.c, "msm_serial_hs.0"),
		O(GLBL), BIT(8), &dummy_clk },
	{ CLK_LOOKUP("iface_clk",     uart2dm_p_clk.c, "msm_serial_hs.1"),
		O(GLBL), BIT(8), &dummy_clk },
};

static struct clk_lookup msm_clocks_7x30[ARRAY_SIZE(ownership_map)];

static void __init set_clock_ownership(void)
{
	uint32_t local, bit = ownership_map[id].bit;
	const uint32_t *reg = ownership_map[id].reg;

	BUG_ON(id >= ARRAY_SIZE(ownership_map) || !reg);

	local = *reg & bit;
	return local ? &soc_clk_ops_7x30 : NULL;
}

/* SoC-specific clk_ops initialization. */
void __init msm_clk_soc_set_ops(struct clk *clk)
{
	if (!clk->ops) {
		struct clk_ops *ops = clk_is_local(clk->id);
		if (ops)
			clk->ops = ops;
		else {
			clk->ops = &clk_ops_remote;
			clk->id = clk->remote_id;
		}
	}
}

/*
 * Miscellaneous clock register initializations
 */
static const struct reg_init {
	const void *reg;
	uint32_t mask;
	uint32_t val;
} ri_list[] __initconst = {
	/* Enable UMDX_P clock. Known to causes issues, so never turn off. */
	{GLBL_CLK_ENA_2_SC_REG, B(2), B(2)},

	{EMDH_NS_REG, BM(18, 17) , BVAL(18, 17, 0x3)}, /* RX div = div-4. */
	{PMDH_NS_REG, BM(18, 17), BVAL(18, 17, 0x3)}, /* RX div = div-4. */
	/* MI2S_CODEC_RX_S src = MI2S_CODEC_RX_M. */
	{MI2S_RX_NS_REG, B(14), 0x0},
	/* MI2S_CODEC_TX_S src = MI2S_CODEC_TX_M. */
	{MI2S_TX_NS_REG, B(14), 0x0},
	{MI2S_NS_REG, B(14), 0x0}, /* MI2S_S src = MI2S_M. */
	/* Allow DSP to decide the LPA CORE src. */
	{LPA_CORE_CLK_MA0_REG, B(0), B(0)},
	{LPA_CORE_CLK_MA2_REG, B(0), B(0)},
	{MI2S_CODEC_RX_DIV_REG, 0xF, 0xD}, /* MI2S_CODEC_RX_S div = div-8. */
	{MI2S_CODEC_TX_DIV_REG, 0xF, 0xD}, /* MI2S_CODEC_TX_S div = div-8. */
	{MI2S_DIV_REG, 0xF, 0x7}, /* MI2S_S div = div-8. */
	{MDC_NS_REG, 0x3, 0x3}, /* MDC src = external MDH src. */
	{SDAC_NS_REG, BM(15, 14), 0x0}, /* SDAC div = div-1. */
	/* Disable sources TCXO/5 & TCXO/6. UART1 src = TCXO*/
	{UART_NS_REG, BM(26, 25) | BM(2, 0), 0x0},
	{MDP_VSYNC_REG, 0xC, 0x4}, /* MDP VSYNC src = LPXO. */
	/* HDMI div = div-1, non-inverted. tv_enc_src = tv_clk_src */
	{HDMI_NS_REG, 0x7, 0x0},
	{TV_NS_REG, BM(15, 14), 0x0}, /* tv_clk_src_div2 = div-1 */

	/* USBH core clocks src = USB_HS_SRC. */
	{USBH_NS_REG, B(15), B(15)},
	{USBH2_NS_REG, B(6), B(6)},
	{USBH3_NS_REG, B(6), B(6)},
};

/* Local clock driver initialization. */
void __init msm_clk_soc_init(void)
{
	int i;
	uint32_t val;

	cache_ownership();
	print_ownership();

	/* When we have no local clock control, the rest of the code in this
	 * function is a NOP since writes to shadow regions that we don't own
	 * are ignored. */

	/* Disable all the child clocks of USB_HS_SRC. This needs to be done
	 * before the register init loop since it changes the source of the
	 * USB HS core clocks. */
	for (i = 0; chld_usb_src[i] != C(NONE); i++)
		if (clk_is_local(chld_usb_src[i]))
			local_clk_disable_reg(chld_usb_src[i]);

	if (clk_is_local(C(USB_HS_SRC)))
		local_clk_set_rate(C(USB_HS_SRC), clk_tbl_usb[0].freq_hz);

	for (i = 0; i < ARRAY_SIZE(ri_list); i++) {
		val = readl(ri_list[i].reg);
		val &= ~ri_list[i].mask;
		val |= ri_list[i].val;
		writel(val, ri_list[i].reg);
	}

	set_1rate(I2C);
	set_1rate(I2C_2);
	set_1rate(QUP_I2C);
	set_1rate(UART1);
	set_1rate(UART2);
	set_1rate(MI2S_M);
	set_1rate(MIDI);
	set_1rate(MDP_VSYNC);
	set_1rate(LPA_CODEC);
	set_1rate(GLBL_ROOT);

	/* Sync the GRP2D clock to AXI */
	local_clk_set_rate(C(GRP_2D), 1);
}

/*
 * Clock operation handler registration
 */
struct clk_ops soc_clk_ops_7x30 = {
	.enable = local_clk_enable,
	.disable = local_clk_disable,
	.auto_off = local_clk_auto_off,
	.set_rate = local_clk_set_rate,
	.set_min_rate = local_clk_set_min_rate,
	.set_max_rate = local_clk_set_max_rate,
	.get_rate = local_clk_get_rate,
	.list_rate = local_clk_list_rate,
	.is_enabled = local_clk_is_enabled,
	.round_rate = local_clk_round_rate,
	.reset = soc_clk_reset,
	.set_flags = soc_clk_set_flags,
	.measure_rate = soc_clk_measure_rate,
};

