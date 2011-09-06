#ifndef _HEXDUMP_H_
#define _HEXDUMP_H_

void print_hex_dump(const char *level, const char *prefix_str, int prefix_type,
                        int rowsize, int groupsize,
                        const void *buf, size_t len, bool ascii);

#endif

