#include <stdio.h>
#include <stdlib.h>

#include "../../allocators/tlsf-copy/tlsf.h"
#include "../../swap_replay/sr_parse/libsr_parse.h"

#define PAGE_SIZE	4096
#define MAX_SWAP_SIZE	(64 * 1024 * 1024)
#define MAX_TABLE_SIZE	(MAX_SWAP_SIZE / PAGE_SIZE)
#define MAX_POOL_SIZE	(64 * 1024 * 1024)
#define KB(BYTES)	(BYTES >> 10)

unsigned long USED;
unsigned short *LEN_TBL;

void event_read(unsigned long index)
{
}

void event_write(unsigned long index, unsigned long len)
{
#if 0
	if (LEN_TBL[index] == len)
		printf("index: %lu\tval: %lu\n", index, len);
#endif
	if (LEN_TBL[index])
		USED -= LEN_TBL[index];
	LEN_TBL[index] = len;
	USED += len;
	printf("%lu\n", KB(USED));
}

/*
 * Arg:
 *	0: Success
 *	>0: Error: partial read
 *	<0: Error: file read error
 */
void event_replay_end(int err)
{
	printf("# replay end: err=%d\n", err);
}

int main(int argc, char **argv)
{
	int ret;
	char *fname;

	if (argc < 2) {
		printf("Usage: ideal_alloc_test <file>\n");
		printf("\t<file> is swap replay dump file from sr_collect\n");
		goto out;
	}

	fname = argv[1];	
	ret = set_sr_file(fname);
	if (ret < 0) {
		printf("Error setting replay data file.\n");
		goto out;
	}

	LEN_TBL = calloc(MAX_TABLE_SIZE, sizeof(*LEN_TBL));
	if (LEN_TBL == NULL) {
		printf("Error allocating lookup table.\n");
		goto out;
	}

	set_sr_callbacks(event_read, event_write, event_replay_end);
	
	sr_start();

	free(LEN_TBL);

out:
	return 0;
}

