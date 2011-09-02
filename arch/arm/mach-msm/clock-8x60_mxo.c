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

/*
 *  This driver assumes the MXO runs at the 27MHz and PXO does not.
 *  Compiling this produces an alternate version of clock-8x60.c's
 *  clock and frequency tables based on the macros below.
 *
 *  This is a hack, but seems to be the only solution that allows
 *  efficient run-time selection between two frequency plans while
 *  avoiding code duplication and the need to maintain a separate
 *  clock drivers for each plan.
 */

/* MUX source input identifiers.
 * (MXO and PXO's swapped from clock-8x60.c) */
#define SRC_SEL_BB_PXO		1
#define SRC_SEL_BB_MXO		0
#define SRC_SEL_BB_CXO		SRC_SEL_BB_MXO
#define SRC_SEL_BB_PLL0		2
#define SRC_SEL_BB_PLL8		3
#define SRC_SEL_BB_PLL6		4
#define SRC_SEL_MM_PXO		4
#define SRC_SEL_MM_PLL0		1
#define SRC_SEL_MM_PLL1		1
#define SRC_SEL_MM_PLL2		3
#define SRC_SEL_MM_GPERF	2
#define SRC_SEL_MM_GPLL0	3
#define SRC_SEL_MM_MXO		0
#define SRC_SEL_XO_CXO		0
#define SRC_SEL_XO_PXO		2
#define SRC_SEL_XO_MXO		1
#define SRC_SEL_LPA_PXO		0
#define SRC_SEL_LPA_CXO		1
#define SRC_SEL_LPA_PLL0	2

#define _MXO_PLAN
#include "clock-8x60.c"
