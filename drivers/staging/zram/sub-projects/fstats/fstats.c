#include <error.h>
#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>

#include <csnappy.h>
#include <lzo/minilzo.h>
#include <zlib.h>

enum compress_algo {
	ZEROCHK, CSNAPPY, LZO, ZLIB, NUM_COMPRESS_ALGOS
};

struct compress_stats {
	unsigned long zero_pages;
	unsigned long original_size;
	unsigned long compressed_size[NUM_COMPRESS_ALGOS];
};

char compress_algo_name[16][16] = { "ZEROCHK", "CSNAPPY", "LZO", "ZLIB",
		"INVALID" };

char PROGNAME[64];
struct compress_stats CSTATS;

unsigned char *LZO_WORKMEM = NULL;
char *CSNAPPY_WORKMEM = NULL;

void print_progname(void)
{
	fprintf(stderr, "%s: ", PROGNAME);
}

int compress_init(enum compress_algo algo)
{
	int ret = 0;

	switch (algo) {
	case ZEROCHK:
		break;
	case CSNAPPY:
		if ((CSNAPPY_WORKMEM = calloc(1, CSNAPPY_WORKMEM_BYTES))
				== NULL) {
			ret = errno;
			error(0, ret, "Error allocating working memory for "
				"CSNAPPY");
		}
		break;
	case LZO:
		if ((LZO_WORKMEM = calloc(1, LZO1X_MEM_COMPRESS)) == NULL) {
			ret = errno;
			error(0, ret, "Error allocating working memory for LZO");
		}
		break;
	case ZLIB:
		break;
	default:
		break;
	}

	return ret;
}

void compress_exit(enum compress_algo algo)
{
	switch (algo) {
	case LZO:
		free(LZO_WORKMEM);
		break;
	case CSNAPPY:
		free(CSNAPPY_WORKMEM);
		break;
	default:
		break;
	}
}

/*
 * Returns 0 is buffer is completely zero filled, @size otherwise.
 */
unsigned int compress_zerochk(const unsigned char *buf, unsigned int size)
{
	int pos, zero_filled = 1;
	const unsigned long *p = (unsigned long *) buf;

	for (pos = 0; pos < size / sizeof(unsigned long); pos++) {
		if (p[pos]) {
			zero_filled = 0;
			break;
		}
	}

	return zero_filled ? 0 : size;
}

double get_elapsed_time_us(struct timeval start, struct timeval end)
{
	double elapsed;

	elapsed = (end.tv_sec - start.tv_sec) * 1000 * 1000; // sec to us
	elapsed += (end.tv_usec - start.tv_usec);

	return elapsed;
}

int get_compressed_size_csnappy(const unsigned char *buf, unsigned int size,
		unsigned int *csize, double *ctime)
{
	char *end;
	struct timeval t1, t2;

	char *dstbuf = malloc(2 * size);
	if (!dstbuf) {
		error(0, ENOMEM, "Error allocating LZO destination buffer");
		return ENOMEM;
	}

	gettimeofday(&t1, NULL);
	end = csnappy_compress_fragment((char *) buf, size, dstbuf,
			CSNAPPY_WORKMEM, CSNAPPY_WORKMEM_BYTES_POWER_OF_TWO);
	gettimeofday(&t2, NULL);

	*ctime = get_elapsed_time_us(t1, t2);
	*csize = (unsigned int) (end - dstbuf);

	free(dstbuf);
	return 0;
}

int get_compressed_size_lzo(const unsigned char *buf, unsigned int size,
		unsigned int *csize, double *ctime)
{
	int ret = 0;
	struct timeval t1, t2;

	unsigned char *dstbuf = malloc(2 * size);
	if (!dstbuf) {
		error(0, ENOMEM, "Error allocating LZO destination buffer");
		return ENOMEM;
	}
	lzo_uint dstlen = 2 * size;

	gettimeofday(&t1, NULL);
	ret = lzo1x_1_compress(buf, size, dstbuf, &dstlen, LZO_WORKMEM);
	gettimeofday(&t2, NULL);

	*ctime = get_elapsed_time_us(t1, t2);

	*csize = dstlen;
	if (ret != LZO_E_OK) {
		error(0, 0, "LZO compression error (%d)", ret);
		ret = EINVAL;
		*csize = 0;
	}

	free(dstbuf);
	return ret;
}

int get_compressed_size_zlib(const unsigned char *buf, unsigned int size,
		unsigned int *csize, double *ctime)
{
	int ret = 0;
	struct timeval t1, t2;
	z_stream zs;

	unsigned char *dstbuf = malloc(2 * size);
	if (!dstbuf) {
		error(0, ENOMEM, "Error allocating ZLIB destination buffer");
		return ENOMEM;
	}

	gettimeofday(&t1, NULL);

	ret = ENOMEM;
	zs.zalloc = Z_NULL;
	zs.zfree = Z_NULL;
	zs.opaque = Z_NULL;

	ret = deflateInit(&zs, Z_DEFAULT_COMPRESSION);
	if (ret != Z_OK)
		goto out;

	zs.avail_in = size;
	zs.next_in = (unsigned char *) buf;
	zs.avail_out = 2 * size;
	zs.next_out = dstbuf;

	ret = deflate(&zs, Z_FINISH);
	if (ret == Z_STREAM_ERROR) { // Z_STREAM_END
		deflateEnd(&zs);
		error(0, 0, "ZLIB compression error (%d)", ret);
		ret = EINVAL;
		*csize = 0;
		goto out;
	}

	deflateEnd(&zs);

	gettimeofday(&t2, NULL);

	*ctime = get_elapsed_time_us(t1, t2);
	*csize = 2 * size - zs.avail_out;

	out:

	free(dstbuf);
	return ret;
}

int get_compressed_size(const unsigned char *buf, unsigned int size,
		enum compress_algo algo, unsigned int *csize, double *ctime)
{
	int ret = 0;
	struct timeval t1, t2;

	switch (algo) {
	case ZEROCHK:
		gettimeofday(&t1, NULL);
		*csize = compress_zerochk(buf, size);
		gettimeofday(&t2, NULL);

		if (*csize == 0)
			CSTATS.zero_pages++;

		*ctime = get_elapsed_time_us(t1, t2);

		break;
	case CSNAPPY:
		get_compressed_size_csnappy(buf, size, csize, ctime);
		break;
	case LZO:
		get_compressed_size_lzo(buf, size, csize, ctime);
		break;
	case ZLIB:
		get_compressed_size_zlib(buf, size, csize, ctime);
		break;
	default:
		error(0, EINVAL, "Invalid compression request");
		break;
	}

	CSTATS.compressed_size[algo] += *csize;

	return ret;
}

/* Prefix each line with '#' so they are ignored by gnuplot */
void print_cstats(void)
{
	int i;

	printf("# zero_pages: %lu\n", CSTATS.zero_pages);
	printf("# original_size: %lu\n", CSTATS.original_size);
	for (i = 0; i < NUM_COMPRESS_ALGOS; i++) {
		printf("# compressed_size[%s]: %lu\n", compress_algo_name[i],
				CSTATS.compressed_size[i]);
	}
}

void dump_fstats(int fd)
{
	int ret = 0;
	ssize_t len = 0;
	size_t page_size = getpagesize();
	unsigned char *buf_page = NULL;

	unsigned int csize[NUM_COMPRESS_ALGOS];
	double ctime[NUM_COMPRESS_ALGOS];
	char *buf_stats = calloc(1,
			NUM_COMPRESS_ALGOS * sizeof(unsigned long) * 4 + 16);
	if (!buf_stats)
		return;

	ret = posix_memalign((void **) &buf_page, page_size, page_size);
	if (ret)
		return;

	memset(buf_page, 0, page_size);

	while ((len = read(fd, buf_page, page_size)) == page_size) {
		int i, stats_len = 0;
		CSTATS.original_size += page_size;
		for (i = 0; i < NUM_COMPRESS_ALGOS; i++) {
			ret = get_compressed_size(buf_page, page_size, i,
					&csize[i], &ctime[i]);
			if (ret)
				break;
		}

		if (ret) {
			error(0, 0, "Compression using algo %d failed at "
				"offset %lu\n", i, CSTATS.original_size);
			break;
		}

		/* Print stats: csize0 csize1 ... time0 time1 ... */
		for (i = 0; i < NUM_COMPRESS_ALGOS; i++)
			stats_len += sprintf(buf_stats + stats_len, "%d ",
					csize[i]);

		for (i = 0; i < NUM_COMPRESS_ALGOS; i++)
			stats_len += sprintf(buf_stats + stats_len, "%f ",
					ctime[i]);

		buf_stats[stats_len - 1] = '\0';
		printf("%s\n", buf_stats);

		memset(buf_page, 0, page_size);
	}

	print_cstats();

	free(buf_stats);
	free(buf_page);
}

int main(int argc, char *argv[])
{
	int i, ret, fd;

	strncpy(PROGNAME, basename(argv[0]), 63);
	PROGNAME[63] = '\0';

	error_print_progname = &print_progname;

	if (argc != 2)
		error(EINVAL, 0, "Usage: %s <filename>", PROGNAME);

	/* Initialize all compressors */
	for (i = 0; i < NUM_COMPRESS_ALGOS; i++) {
		ret = compress_init(i);
		if (ret) {
			int j;
			for (j = 0; j < i; j++)
				compress_exit(j);
			break;
		}
	}

	if ((fd = open(argv[1], O_RDONLY)) == -1) {
		error(EINVAL, errno, "Error opening file");
	}

	/* Reset compression stats */
	memset(&CSTATS, 0, sizeof(CSTATS));

	dump_fstats(fd);

	close(fd);
	for (i = 0; i < NUM_COMPRESS_ALGOS; i++)
		compress_exit(i);

	return 0;
}
