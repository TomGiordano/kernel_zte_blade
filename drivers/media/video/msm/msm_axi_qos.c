/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
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

#include <linux/pm_qos_params.h>
#include <mach/camera.h>
#define MSM_AXI_QOS_NAME "msm_camera"


int add_axi_qos(void)
{
	int rc = 0;

	rc = pm_qos_add_requirement(PM_QOS_SYSTEM_BUS_FREQ,
		MSM_AXI_QOS_NAME, PM_QOS_DEFAULT_VALUE);
	if (rc < 0)
		CDBG("request AXI bus QOS fails. rc = %d\n", rc);
	return rc;
}

int update_axi_qos(uint32_t rate)
{
	int rc = 0;
	rc = pm_qos_update_requirement(PM_QOS_SYSTEM_BUS_FREQ,
		MSM_AXI_QOS_NAME, rate);
	if (rc < 0)
		CDBG("update AXI bus QOS fails. rc = %d\n", rc);
	return rc;
}

void release_axi_qos(void)
{
	pm_qos_remove_requirement(PM_QOS_SYSTEM_BUS_FREQ,
		MSM_AXI_QOS_NAME);
}
