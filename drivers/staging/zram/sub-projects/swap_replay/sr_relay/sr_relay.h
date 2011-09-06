#ifndef _SR_RELAY_H_
#define _SR_RELAY_H_

#define HASH_BYTES	16

// sizeof(sr_info) should be divisible by PAGE_SIZE
struct sr_info {
	unsigned int page_rw;           // page no. + rw bit
	unsigned int clen;              // compressed data length (only for W)
	unsigned char hash[HASH_BYTES];	// MD5 hash of page
} __attribute__ ((packed));

#endif
