#include <fcntl.h>
#include <stdio.h>

#include "../sr_relay/sr_relay.h"
#include "../../../lzo/lzo.h"

#define PAGE_SIZE	4096
#define MAX_CLEN	(PAGE_SIZE - 32)

#define HISTO_STEP	16
#define HISTO_MASK	(HISTO_STEP - 1)
#define HISTO_MIN	0
#define HISTO_MAX	(MAX_CLEN / HISTO_STEP)

static unsigned long histo_clen[HISTO_MAX + 1];
static struct sr_info sr_info;

static void dump_histo(void)
{
	int bucket;
	unsigned long total_writes = 0;
	unsigned long good_compress = 0; // pages with compression ratio <= 50%

	for (bucket = HISTO_MIN; bucket < HISTO_MAX; bucket++) {
		total_writes += histo_clen[bucket];
		if (bucket * HISTO_STEP <= PAGE_SIZE / 2)
			good_compress += histo_clen[bucket];
		printf("%u\t%u\n", bucket * HISTO_STEP, histo_clen[bucket]);
	}

	total_writes += histo_clen[HISTO_MAX];
	
	/*
	 * Additional stats.
	 * GNUplot ignores lines starting with #
	 */
	printf("# good_compress=%u\n", good_compress);
	printf("# total_writes=%u\n", total_writes);
	printf("# %% pages with compress ratio <= 50%%: %u\n",
		good_compress *100 / total_writes);
	printf("# No. of pages with compress length >= %u: %lu\n",
		MAX_CLEN, histo_clen[HISTO_MAX]);
}

static void parse_file(int fd, int raw)
{
	int rc;
	unsigned int bucket;

	while (1) {
		rc = read(fd, &sr_info, sizeof(struct sr_info));
		if (!rc)
			break;
		if (rc < 0) {
			printf("Error: read failed.\n");
			return;
		}
		if (rc != sizeof(struct sr_info)) {
			printf("Error: partial read.\n");
			return;
		}
		if (raw) {
			printf("%s %u %u\n", sr_info.page_rw & 1 ? "W" : "R",
						sr_info.page_rw >> 1,
						sr_info.clen);
			continue;
		}

		if (sr_info.page_rw & 1) {	// write
			bucket = (sr_info.clen / HISTO_STEP)
				+ !!(sr_info.clen & HISTO_MASK);
			bucket = bucket > HISTO_MAX ? HISTO_MAX : bucket;
			histo_clen[bucket]++;
		}
	};

	if (!raw)
		dump_histo();
}

int main(int argc, char **argv)
{
	int fd, raw = 0;
	char *fname;

	if (argc < 2 || argc > 3) {
		printf("Usage: sr_parse <file> [-raw]\n");
		return 0;
	}

	if (argc == 3 && !strcmp(argv[2], "-raw"))
		raw = 1;
	
	fname = argv[1];
	fd = open(fname, O_RDONLY | O_NONBLOCK);
	if (!fd) {
		printf("Error opening file: %s\n", fname);
		return 0;
	}
	
	parse_file(fd, raw);

	close(fd);
	return 0;
}
