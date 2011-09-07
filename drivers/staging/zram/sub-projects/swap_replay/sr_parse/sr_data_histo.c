#include <fcntl.h>
#include <stdio.h>
#include <string.h>

#include "libsr_parse.h"
#include "../../lib/more_types.h"
#include "../../lib/hexdump.h"

/*
 * "-raw" user param: raw=0 if not given
 * 	0: Give "compressed page length" histogram (default)
 * 	1: dump <R/W <index> <clen>
 */
int raw;

#define PAGE_SIZE	4096
// #define MAX_CLEN	(PAGE_SIZE - 32)
#define MAX_CLEN	PAGE_SIZE

#define HISTO_STEP	16
#define HISTO_MASK	(HISTO_STEP - 1)
#define HISTO_MIN	0
#define HISTO_MAX	(MAX_CLEN / HISTO_STEP)

#define HASH_BYTES	16

static unsigned long histo_clen[HISTO_MAX + 1];
static unsigned long total_reads, total_writes;

static void hexdump(const unsigned char *buf, unsigned int len)
{
	print_hex_dump("", "", 0,
			16, 1,
			buf, len, 0);
}

static void dump_histo(void)
{
	unsigned int bucket;
	unsigned long good_compress = 0; // pages with compression ratio <= 50%

	for (bucket = HISTO_MIN; bucket < HISTO_MAX; bucket++) {
		if (bucket * HISTO_STEP <= PAGE_SIZE / 2)
			good_compress += histo_clen[bucket];
		printf("%u\t%lu\n", bucket * HISTO_STEP, histo_clen[bucket]);
	}

	/*
	 * Additional stats.
	 * GNUplot ignores lines starting with #
	 */
	printf("# Total: R=%lu\tW=%lu\n", total_reads, total_writes);
	printf("# %% pages with compress ratio <= 50%%: %lu\n",
		good_compress * 100 / total_writes);
	printf("# No. of pages with compress length >= %u: %lu\n",
		MAX_CLEN, histo_clen[HISTO_MAX]);
}

void event_read(unsigned long index)
{
	total_reads++;
	if (raw)
		printf("R %lu\n", index);
}

void event_write(unsigned long index, unsigned long len,
		const unsigned char *hash)
{
	int bucket;
	total_writes++;
	if (raw) {
		printf("W %lu %lu ", index, len);
		hexdump(hash, HASH_BYTES);
		return;
	}

	bucket = (len / HISTO_STEP)
		+ !!(len & HISTO_MASK);
	bucket = bucket > HISTO_MAX ? HISTO_MAX : bucket;
	histo_clen[bucket]++;
}

/*
 * Arg:
 *	0: Success
 *	>0: Error: partial read
 *	<0: Error: file read error
 */
void event_replay_end(int err)
{
	if (raw)
		return;
	dump_histo();
}

int main(int argc, char **argv)
{
	int ret = 0;
	char *fname;
	void *sr_handle;

	if (argc < 2 || argc > 3) {
		printf("Usage: sr_parse <file> [-raw]\n");
		printf("\t<file> is swap replay dump file from sr_collect\n");
		goto out;
	}

	if (argc == 3 && !strcmp(argv[2], "-raw"))
		raw = 1;

	fname = argv[1];

	sr_handle = sr_gethandle();

	ret = set_sr_file(sr_handle, fname);
	if (ret < 0) {
		printf("Error setting replay data file\n");
		goto out;
	}

	set_sr_callbacks(sr_handle, event_read, event_write, event_replay_end);

	sr_start(sr_handle);
out:
	return ret;
}
