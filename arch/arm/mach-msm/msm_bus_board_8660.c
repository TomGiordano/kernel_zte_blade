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
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/module.h>
#include <mach/msm_bus.h>
#include <mach/msm_bus_board.h>
#include <mach/board.h>
#include "rpm.h"

static struct msm_bus_node_info apps_fabric_info[] = {
	{
		.id = MSM_BUS_APPSS_MASTER_SMPSS_M0,
		.masterp = GET_MPORT(MSM_BUS_APPSS_MASTER_SMPSS_M1),
		.tier = MSM_BUS_BW_TIER2,
	},
	{
		.id = MSM_BUS_APPSS_MASTER_SMPSS_M1,
		.masterp = GET_MPORT(MSM_BUS_APPSS_MASTER_SMPSS_M1),
		.tier = MSM_BUS_BW_TIER2,
	},
	{
		.id = MSM_BUS_APPSS_SLAVE_EBI_CH0,
		.slavep = GET_SLPORT(MSM_BUS_APPSS_SLAVE_EBI_CH0),
		.tier = MSM_BUS_APPSS_TIERED_SLAVE_EBI_CH0,
		.buswidth = 8,
		.slaveclk = "ebi1_clk",
		.a_slaveclk = "ebi1_a_clk",
	},
	{
		.id = MSM_BUS_APPSS_SLAVE_SMPSS_L2,
		.slavep = GET_SLPORT(MSM_BUS_APPSS_SLAVE_SMPSS_L2),
		.tier = MSM_BUS_APPSS_TIERED_SLAVE_SMPSS_L2,
		.buswidth = 8,
	},
	{
		.id = MSM_BUS_FAB_MMSS,
		.gateway = 1,
		.slavep = GET_SLPORT(MSM_BUS_APPSS_SLAVE_FAB_MMSS),
		.masterp = GET_MPORT(MSM_BUS_MMSS_MASTER_APPS_FAB),
		.buswidth = 8,
	},
	{
		.id = MSM_BUS_FAB_SYSTEM,
		.gateway = 1,
		.slavep = GET_SLPORT(MSM_BUS_APPSS_SLAVE_FAB_SYSTEM),
		.masterp = GET_MPORT(MSM_BUS_SYSTEM_MASTER_FAB_APPSS),
		.buswidth = 8,
	},
};

static struct msm_bus_node_info system_fabric_info[]  = {
	{
		.id = MSM_BUS_SYSTEM_MASTER_SPS,
		.masterp = GET_MPORT(MSM_BUS_SYSTEM_MASTER_SPS),
		.tier = MSM_BUS_BW_TIER2,
	},
	{
		.id = MSM_BUS_SYSTEM_MASTER_ADM0_PORT0,
		.masterp = GET_MPORT(MSM_BUS_SYSTEM_MASTER_ADM0_PORT0),
		.tier = MSM_BUS_BW_TIER2,
	},
	{
		.id = MSM_BUS_SYSTEM_MASTER_ADM0_PORT1,
		.masterp = GET_MPORT(MSM_BUS_SYSTEM_MASTER_ADM0_PORT1),
		.tier = MSM_BUS_BW_TIER2,
	},
	{
		.id = MSM_BUS_SYSTEM_MASTER_ADM1_PORT0,
		.masterp = GET_MPORT(MSM_BUS_SYSTEM_MASTER_ADM1_PORT0),
		.tier = MSM_BUS_BW_TIER2,
	},
	{
		.id = MSM_BUS_SYSTEM_MASTER_ADM1_PORT1,
		.masterp = GET_MPORT(MSM_BUS_SYSTEM_MASTER_ADM1_PORT1),
		.tier = MSM_BUS_BW_TIER2,
	},
	{
		.id = MSM_BUS_SYSTEM_MASTER_LPASS_PROC,
		.masterp = GET_MPORT(MSM_BUS_SYSTEM_MASTER_LPASS_PROC),
		.tier = MSM_BUS_BW_TIER2,
	},
	{
		.id = MSM_BUS_SYSTEM_MASTER_MSS_PROCI,
		.masterp = GET_MPORT(MSM_BUS_SYSTEM_MASTER_MSS_PROCI),
		.tier = MSM_BUS_BW_TIER2,
	},
	{
		.id = MSM_BUS_SYSTEM_MASTER_MSS_PROCD,
		.masterp = GET_MPORT(MSM_BUS_SYSTEM_MASTER_MSS_PROCD),
		.tier = MSM_BUS_BW_TIER2,
	},
	{
		.id = MSM_BUS_SYSTEM_MASTER_MSS_MDM_PORT0,
		.masterp = GET_MPORT(MSM_BUS_SYSTEM_MASTER_MSS_MDM_PORT0),
		.tier = MSM_BUS_BW_TIER2,
	},
	{
		.id = MSM_BUS_SYSTEM_MASTER_LPASS,
		.masterp = GET_MPORT(MSM_BUS_SYSTEM_MASTER_LPASS),
		.tier = MSM_BUS_BW_TIER2,
	},
	{
		.id = MSM_BUS_SYSTEM_MASTER_MMSS_FPB,
		.masterp = GET_MPORT(MSM_BUS_SYSTEM_MASTER_MMSS_FPB),
		.tier = MSM_BUS_BW_TIER2,
	},
	{
		.id = MSM_BUS_SYSTEM_MASTER_ADM1_CI,
		.masterp = GET_MPORT(MSM_BUS_SYSTEM_MASTER_ADM1_CI),
		.tier = MSM_BUS_BW_TIER2,
	},
	{
		.id = MSM_BUS_SYSTEM_MASTER_ADM0_CI,
		.masterp = GET_MPORT(MSM_BUS_SYSTEM_MASTER_ADM0_CI),
		.tier = MSM_BUS_BW_TIER2,
	},
	{
		.id = MSM_BUS_SYSTEM_MASTER_MSS_MDM_PORT1,
		.masterp = GET_MPORT(MSM_BUS_SYSTEM_MASTER_MSS_MDM_PORT1),
		.tier = MSM_BUS_BW_TIER2,
	},
	{
		.id = MSM_BUS_FAB_APPSS,
		.gateway = 1,
		.slavep = GET_SLPORT(MSM_BUS_SYSTEM_SLAVE_FAB_APPS),
		.masterp = GET_MPORT(MSM_BUS_APPSS_MASTER_FAB_SYSTEM),
		.tier = MSM_BUS_SYSTEM_TIERED_SLAVE_FAB_APPSS,
		.buswidth = 8,
	},
	{
		.id = MSM_BUS_FAB_SYSTEM_FPB,
		.gateway = 1,
		.slavep = GET_SLPORT(MSM_BUS_SYSTEM_SLAVE_SYSTEM_FPB),
		.masterp = GET_MPORT(MSM_BUS_SYSTEM_FPB_MASTER_SYSTEM),
		.buswidth = 4,
	},
	{
		.id = MSM_BUS_FAB_CPSS_FPB,
		.gateway = 1,
		.slavep = GET_SLPORT(MSM_BUS_SYSTEM_SLAVE_CPSS_FPB),
		.masterp = GET_MPORT(MSM_BUS_CPSS_FPB_MASTER_SYSTEM),
		.buswidth = 4,
	},
	{
		.id = MSM_BUS_SYSTEM_SLAVE_SPS,
		.slavep = GET_SLPORT(MSM_BUS_SYSTEM_SLAVE_SPS),
		.buswidth = 8,
	},
	{
		.id = MSM_BUS_SYSTEM_SLAVE_SYSTEM_IMEM,
		.slavep = GET_SLPORT(MSM_BUS_SYSTEM_SLAVE_SYSTEM_IMEM),
		.tier = MSM_BUS_SYSTEM_TIERED_SLAVE_SYSTEM_IMEM,
		.buswidth = 8,
	},
	{
		.id = MSM_BUS_SYSTEM_SLAVE_SMPSS,
		.slavep = GET_SLPORT(MSM_BUS_SYSTEM_SLAVE_SMPSS),
		.buswidth = 8,
	},
	{
		.id = MSM_BUS_SYSTEM_SLAVE_MSS,
		.slavep = GET_SLPORT(MSM_BUS_SYSTEM_SLAVE_MSS),
		.buswidth = 8,
	},
	{
		.id = MSM_BUS_SYSTEM_SLAVE_LPASS,
		.slavep = GET_SLPORT(MSM_BUS_SYSTEM_SLAVE_LPASS),
		.buswidth = 8,
	},
	{
		.id = MSM_BUS_SYSTEM_SLAVE_MMSS_FPB,
		.slavep = GET_SLPORT(MSM_BUS_SYSTEM_SLAVE_MMSS_FPB),
		.buswidth = 8,
	},
};

static struct msm_bus_node_info mmss_fabric_info[]  = {
	{
		.id = MSM_BUS_MMSS_MASTER_MDP_PORT0,
		.masterp = GET_MPORT(MSM_BUS_MMSS_MASTER_MDP_PORT0),
		.tier = MSM_BUS_BW_TIER1,
	},
	{
		.id = MSM_BUS_MMSS_MASTER_MDP_PORT1,
		.masterp = GET_MPORT(MSM_BUS_MMSS_MASTER_MDP_PORT1),
		.tier = MSM_BUS_BW_TIER1,
	},
	{
		.id = MSM_BUS_MMSS_MASTER_ADM1_PORT0,
		.masterp = GET_MPORT(MSM_BUS_MMSS_MASTER_ADM1_PORT0),
		.tier = MSM_BUS_BW_TIER2,
	},
	{
		.id = MSM_BUS_MMSS_MASTER_ROTATOR,
		.masterp = GET_MPORT(MSM_BUS_MMSS_MASTER_ROTATOR),
		.tier = MSM_BUS_BW_TIER2,
	},
	{
		.id = MSM_BUS_MMSS_MASTER_GRAPHICS_3D,
		.masterp = GET_MPORT(MSM_BUS_MMSS_MASTER_GRAPHICS_3D),
		.tier = MSM_BUS_BW_TIER2,
	},
	{
		.id = MSM_BUS_MMSS_MASTER_JPEG_DEC,
		.masterp = GET_MPORT(MSM_BUS_MMSS_MASTER_JPEG_DEC),
		.tier = MSM_BUS_BW_TIER2,
	},
	{
		.id = MSM_BUS_MMSS_MASTER_GRAPHICS_2D_CORE0,
		.masterp = GET_MPORT(MSM_BUS_MMSS_MASTER_GRAPHICS_2D_CORE0),
		.tier = MSM_BUS_BW_TIER2,
	},
	{
		.id = MSM_BUS_MMSS_MASTER_VFE,
		.masterp = GET_MPORT(MSM_BUS_MMSS_MASTER_VFE),
		.tier = MSM_BUS_BW_TIER2,
	},
	{
		.id = MSM_BUS_MMSS_MASTER_VPE,
		.masterp = GET_MPORT(MSM_BUS_MMSS_MASTER_VPE),
		.tier = MSM_BUS_BW_TIER2,
	},
	{
		.id = MSM_BUS_MMSS_MASTER_JPEG_ENC,
		.masterp = GET_MPORT(MSM_BUS_MMSS_MASTER_JPEG_ENC),
		.tier = MSM_BUS_BW_TIER2,
	},
	/* This port has been added for V2. It is absent in V1 */
	{
		.id = MSM_BUS_MMSS_MASTER_GRAPHICS_2D_CORE1,
		.masterp = GET_MPORT(MSM_BUS_MMSS_MASTER_GRAPHICS_2D_CORE1),
		.tier = MSM_BUS_BW_TIER2,
	},
	{
		.id = MSM_BUS_MMSS_MASTER_HD_CODEC_PORT0,
		.masterp = GET_MPORT(MSM_BUS_MMSS_MASTER_HD_CODEC_PORT0),
		.tier = MSM_BUS_BW_TIER2,
	},
	{
		.id = MSM_BUS_MMSS_MASTER_HD_CODEC_PORT1,
		.masterp = GET_MPORT(MSM_BUS_MMSS_MASTER_HD_CODEC_PORT1),
		.tier = MSM_BUS_BW_TIER2,
	},
	{
		.id = MSM_BUS_MMSS_SLAVE_SMI,
		.slavep = GET_SLPORT(MSM_BUS_MMSS_SLAVE_SMI),
		.tier = MSM_BUS_MMSS_TIERED_SLAVE_SMI,
		.buswidth = 16,
		.slaveclk = "smi_clk",
		.a_slaveclk = "smi_a_clk",
	},
	{
		.id = MSM_BUS_MMSS_SLAVE_FAB_APPS_1,
		.slavep = GET_SLPORT(MSM_BUS_MMSS_SLAVE_FAB_APPS_1),
		.buswidth = 8,
	},
	{
		.id = MSM_BUS_FAB_APPSS,
		.gateway = 1,
		.slavep = GET_SLPORT(MSM_BUS_MMSS_SLAVE_FAB_APPS),
		.masterp = GET_MPORT(MSM_BUS_APPSS_MASTER_FAB_MMSS),
		.tier = MSM_BUS_MMSS_TIERED_SLAVE_FAB_APPS,
		.buswidth = 8,
	},
	{
		.id = MSM_BUS_MMSS_SLAVE_MM_IMEM,
		.slavep = GET_SLPORT(MSM_BUS_MMSS_SLAVE_MM_IMEM),
		.tier = MSM_BUS_MMSS_TIERED_SLAVE_MM_IMEM,
		.buswidth = 8,
	},
};

static struct msm_bus_node_info sys_fpb_fabric_info[]  = {
	{
		.id = MSM_BUS_FAB_SYSTEM,
		.gateway = 1,
		.slavep = GET_SLPORT(MSM_BUS_SYSTEM_FPB_SLAVE_SYSTEM),
		.masterp = GET_MPORT(MSM_BUS_SYSTEM_MASTER_SYSTEM_FPB),
		.buswidth = 4,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_SYSTEM_FPB_MASTER_SPDM,
		.masterp = GET_MPORT(MSM_BUS_SYSTEM_FPB_MASTER_SPDM),
		.ahb = 1,
		.tier = MSM_BUS_BW_TIER2,
	},
	{
		.id = MSM_BUS_SYSTEM_FPB_MASTER_RPM,
		.masterp = GET_MPORT(MSM_BUS_SYSTEM_FPB_MASTER_RPM),
		.ahb = 1,
		.tier = MSM_BUS_BW_TIER2,
	},
	{
		.id = MSM_BUS_SYSTEM_FPB_SLAVE_SPDM,
		.slavep = GET_SLPORT(MSM_BUS_SYSTEM_FPB_SLAVE_SPDM),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_SYSTEM_FPB_SLAVE_RPM,
		.slavep = GET_SLPORT(MSM_BUS_SYSTEM_FPB_SLAVE_RPM),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_SYSTEM_FPB_SLAVE_RPM_MSG_RAM,
		.slavep = GET_SLPORT(MSM_BUS_SYSTEM_FPB_SLAVE_RPM_MSG_RAM),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_SYSTEM_FPB_SLAVE_MPM,
		.slavep = GET_SLPORT(MSM_BUS_SYSTEM_FPB_SLAVE_MPM),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_SYSTEM_FPB_SLAVE_PMIC1_SSBI1_A,
		.slavep = GET_SLPORT(MSM_BUS_SYSTEM_FPB_SLAVE_PMIC1_SSBI1_A),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_SYSTEM_FPB_SLAVE_PMIC1_SSBI1_B,
		.slavep = GET_SLPORT(MSM_BUS_SYSTEM_FPB_SLAVE_PMIC1_SSBI1_B),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_SYSTEM_FPB_SLAVE_PMIC1_SSBI1_C,
		.slavep = GET_SLPORT(MSM_BUS_SYSTEM_FPB_SLAVE_PMIC1_SSBI1_C),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_SYSTEM_FPB_SLAVE_PMIC2_SSBI2_A,
		.slavep = GET_SLPORT(MSM_BUS_SYSTEM_FPB_SLAVE_PMIC2_SSBI2_A),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_SYSTEM_FPB_SLAVE_PMIC2_SSBI2_B,
		.slavep = GET_SLPORT(MSM_BUS_SYSTEM_FPB_SLAVE_PMIC2_SSBI2_B),
		.buswidth = 4,
		.ahb = 1,
	},
};

static struct msm_bus_node_info cpss_fpb_fabric_info[] = {
	{
		.id = MSM_BUS_FAB_SYSTEM,
		.gateway = 1,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_SYSTEM),
		.masterp = GET_MPORT(MSM_BUS_SYSTEM_MASTER_CPSS_FPB),
		.buswidth = 4,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_GSBI1_UART,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_GSBI1_UART),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_GSBI2_UART,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_GSBI2_UART),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_GSBI3_UART,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_GSBI3_UART),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_GSBI4_UART,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_GSBI4_UART),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_GSBI5_UART,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_GSBI5_UART),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_GSBI6_UART,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_GSBI6_UART),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_GSBI7_UART,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_GSBI7_UART),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_GSBI8_UART,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_GSBI8_UART),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_GSBI9_UART,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_GSBI9_UART),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_GSBI10_UART,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_GSBI10_UART),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_GSBI11_UART,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_GSBI11_UART),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_GSBI12_UART,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_GSBI12_UART),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_GSBI1_QUP,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_GSBI1_QUP),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_GSBI2_QUP,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_GSBI2_QUP),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_GSBI3_QUP,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_GSBI3_QUP),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_GSBI4_QUP,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_GSBI4_QUP),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_GSBI5_QUP,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_GSBI5_QUP),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_GSBI6_QUP,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_GSBI6_QUP),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_GSBI7_QUP,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_GSBI7_QUP),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_GSBI8_QUP,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_GSBI8_QUP),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_GSBI9_QUP,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_GSBI9_QUP),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_GSBI10_QUP,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_GSBI10_QUP),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_GSBI11_QUP,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_GSBI11_QUP),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_GSBI12_QUP,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_GSBI12_QUP),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_EBI2_NAND,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_EBI2_NAND),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_EBI2_CS0,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_EBI2_CS0),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_EBI2_CS1,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_EBI2_CS1),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_EBI2_CS2,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_EBI2_CS2),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_EBI2_CS3,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_EBI2_CS3),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_EBI2_CS4,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_EBI2_CS4),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_EBI2_CS5,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_EBI2_CS5),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_USB_FS1,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_USB_FS1),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_USB_FS2,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_USB_FS2),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_TSIF,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_TSIF),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_MSM_TSSC,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_MSM_TSSC),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_MSM_PDM,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_MSM_PDM),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_MSM_DIMEM,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_MSM_DIMEM),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_MSM_TCSR,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_MSM_TCSR),
		.buswidth = 8,
		.ahb = 1,
	},
	{
		.id = MSM_BUS_CPSS_FPB_SLAVE_MSM_PRNG,
		.slavep = GET_SLPORT(MSM_BUS_CPSS_FPB_SLAVE_MSM_PRNG),
		.buswidth = 4,
		.ahb = 1,
	},
};

struct msm_bus_fabric_registration msm_bus_apps_fabric_pdata = {
	.id = MSM_BUS_FAB_APPSS,
	.name = "msm_apps_fab",
	.info = apps_fabric_info,
	.len = ARRAY_SIZE(apps_fabric_info),
	.ahb = 0,
	.fabclk = "afab_clk",
	.a_fabclk = "afab_a_clk",
	.haltid = MSM_RPM_ID_APPS_FABRIC_HALT_0,
	.offset = MSM_RPM_ID_APPS_FABRIC_ARB_0,
};

struct msm_bus_fabric_registration msm_bus_sys_fabric_pdata = {
	.id = MSM_BUS_FAB_SYSTEM,
	.name = "msm_sys_fab",
	system_fabric_info,
	ARRAY_SIZE(system_fabric_info),
	.ahb = 0,
	.fabclk = "sfab_clk",
	.a_fabclk = "sfab_a_clk",
	.haltid = MSM_RPM_ID_SYSTEM_FABRIC_HALT_0,
	.offset = MSM_RPM_ID_SYSTEM_FABRIC_ARB_0,
};

struct msm_bus_fabric_registration msm_bus_mm_fabric_pdata = {
	.id = MSM_BUS_FAB_MMSS,
	.name = "msm_mm_fab",
	mmss_fabric_info,
	ARRAY_SIZE(mmss_fabric_info),
	.ahb = 0,
	.fabclk = "mmfab_clk",
	.a_fabclk = "mmfab_a_clk",
	.haltid = MSM_RPM_ID_MM_FABRIC_HALT_0,
	.offset = MSM_RPM_ID_MM_FABRIC_ARB_0,
};

struct msm_bus_fabric_registration msm_bus_sys_fpb_pdata = {
	.id = MSM_BUS_FAB_SYSTEM_FPB,
	.name = "msm_sys_fpb",
	sys_fpb_fabric_info,
	ARRAY_SIZE(sys_fpb_fabric_info),
	.ahb = 1,
	.fabclk = "sfpb_clk",
	.fabclk = "sfpb_a_clk",
};

struct msm_bus_fabric_registration msm_bus_cpss_fpb_pdata = {
	.id = MSM_BUS_FAB_CPSS_FPB,
	.name = "msm_cpss_fpb",
	cpss_fpb_fabric_info,
	ARRAY_SIZE(cpss_fpb_fabric_info),
	.ahb = 1,
	.fabclk = "cfpb_clk",
	.a_fabclk = "cfpb_a_clk",
};
