/*
 * Arch specific extensions to struct device
 *
 * This file is released under the GPLv2
 */
#ifndef _ASM_SPARC_DEVICE_H
#define _ASM_SPARC_DEVICE_H

struct device_node;
struct of_device;

struct dev_archdata {
	void			*iommu;
	void			*stc;
	void			*host_controller;
	struct of_device	*op;
	int			numa_node;
};

struct pdev_archdata {
};

#endif /* _ASM_SPARC_DEVICE_H */
