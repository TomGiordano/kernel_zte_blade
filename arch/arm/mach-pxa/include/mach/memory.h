/*
 *  arch/arm/mach-pxa/include/mach/memory.h
 *
 * Author:	Nicolas Pitre
 * Copyright:	(C) 2001 MontaVista Software Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_MEMORY_H
#define __ASM_ARCH_MEMORY_H

/*
 * Physical DRAM offset.
 */
#define PHYS_OFFSET	UL(0xa0000000)

/*
 * The nodes are matched with the physical SDRAM banks as follows:
 *
 * 	node 0:  0xa0000000-0xa3ffffff	-->  0xc0000000-0xc3ffffff
 * 	node 1:  0xa4000000-0xa7ffffff	-->  0xc4000000-0xc7ffffff
 * 	node 2:  0xa8000000-0xabffffff	-->  0xc8000000-0xcbffffff
 * 	node 3:  0xac000000-0xafffffff	-->  0xcc000000-0xcfffffff
 *
 * This needs a node mem size of 26 bits.
 */
#define NODE_MEM_SIZE_BITS	26

#if !defined(__ASSEMBLY__) && defined(CONFIG_MACH_ARMCORE) && defined(CONFIG_PCI)
void cmx2xx_pci_adjust_zones(int node, unsigned long *size,
			     unsigned long *holes);

#define arch_adjust_zones(node, size, holes) \
	cmx2xx_pci_adjust_zones(node, size, holes)

#define ISA_DMA_THRESHOLD	(PHYS_OFFSET + SZ_64M - 1)
#define MAX_DMA_ADDRESS		(PAGE_OFFSET + SZ_64M)
#endif

#endif
