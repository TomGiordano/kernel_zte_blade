#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <io.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include "minilzo.h"

const unsigned int PAGE_SIZE = 4096;

enum COMPRESS_ALGO {
	LZO,
	ZLIB,
	NONE
};

unsigned char *PAGE = NULL, *DST_DBLPAGE = NULL, *LZO_WORKMEM = NULL;

void Usage(void)
{
	printf("DumpFileStats <file>\n");
}

unsigned int GetCompressedLen_LZO(unsigned char *buf, unsigned int bufLen)
{
	int err;
	lzo_uint dstLen = 2 * PAGE_SIZE;
	
	err = lzo1x_1_compress(buf, bufLen, DST_DBLPAGE, &dstLen, LZO_WORKMEM);
	if (err != LZO_E_OK) {
		printf("LZO compression error: err=%d\n", err);
		dstLen = 0;
	}

	return (unsigned int)dstLen;
}

unsigned int GetCompressedLen(unsigned char *buf, unsigned int bufLen, enum COMPRESS_ALGO algo)
{
	unsigned int dstLen;

	switch (algo) {
		case LZO:
			dstLen = GetCompressedLen_LZO(buf, bufLen);
			break;		
		case ZLIB:
			break;
		case NONE:
			dstLen = bufLen;
	}

	return dstLen;
}

void dumpFileStats(int fd)
{
	int rc;
	unsigned int clen;
	unsigned long bytesRead = 0, totalCompressed = 0;
	enum COMPRESS_ALGO algo = LZO;

	while (1) {
		rc = _read(fd, PAGE, PAGE_SIZE);
		if (!rc) // EOF
			break;
		if (rc != PAGE_SIZE) { // Probably EOF (last <PAGE_SIZE block)
			printf("# DFS: partial read! (%u/%u bytes)\n", rc, PAGE_SIZE);
			break;
		}
		if (rc < 0) { // Error
			printf("DFS: read failed: err=%d\n", rc);
			return;
		}
		bytesRead += PAGE_SIZE;

		clen = GetCompressedLen(PAGE, PAGE_SIZE, algo);
		printf("%u\n", clen);
		totalCompressed += clen;
	}; // end of while

	printf("# bytesRead:\t%u\n", bytesRead);
	printf("# Compressed:\t%u\n", totalCompressed);
}

int main(int argc, char *argv[])
{
	const unsigned int MAX_FNAME_LEN = 256;
	int fd, err;

	char fileName[MAX_FNAME_LEN];

	if (argc < 2) {
		Usage();
		return 0;
	}
	strncpy(fileName, argv[1], MAX_FNAME_LEN);
	fileName[MAX_FNAME_LEN - 1] = '\0';

	fd = _open(fileName, _O_RDONLY);
	if (fd == -1) {
		printf("DFS: error opening file [%s]\n", fileName);
		err = -ENOENT;
		goto out;
	}

	err = _setmode(fd, _O_BINARY);
	if (err == -1) {
		printf( "Error switching to binary mode\n");
		goto out;
	}

	PAGE = (unsigned char *)calloc(PAGE_SIZE, 1);
	if (!PAGE) {
		printf("DFS: Error allocating memory page\n");
		err = -ENOMEM;
		goto out;
	}

	DST_DBLPAGE = (unsigned char *)calloc(2 * PAGE_SIZE, 1);
	if (!DST_DBLPAGE) {
		printf("DFS: Error allocating dest memory page\n");
		err = -ENOMEM;
		goto out;
	}

	LZO_WORKMEM = (unsigned char *)calloc(LZO1X_MEM_COMPRESS, 1);
	if (!LZO_WORKMEM) {
		printf("DFS: Error allocating LZO work memory\n");
		err = -ENOMEM;
		goto out;
	}

	dumpFileStats(fd);

out:
	// Its okay to call free(NULL)
	free(PAGE);
	free(DST_DBLPAGE);
	free(LZO_WORKMEM);
	if (fd != -1)
		_close(fd);
	return err;
}
