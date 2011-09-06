#ifndef _LINUX_VIRTIO_VSWAP_H
#define _LINUX_VIRTIO_VSWAP_H

/* should be defined in virtio_ids.h */
#define VIRTIO_ID_VSWAP		10

/* These two define direction */
#define VIRTIO_VSWAP_T_IN	0
#define VIRTIO_VSWAP_T_OUT	1

/* This is the first element of the read scatter-gather list */
struct virtio_vswap_outhdr {
	/* VIRTIO_VSWAP_T* */
	__u32 type;
	/* Sector (i.e. 512 byte offset) */
	__u64 sector;
};

struct virtio_vswap_inhdr {
	__u8 status;
};

#endif
