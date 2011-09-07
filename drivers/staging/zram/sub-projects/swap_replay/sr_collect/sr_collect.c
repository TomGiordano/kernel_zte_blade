#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>

#define PAGE_SIZE	4096

static void dump_file(int read_fd, int write_fd)
{
	int rc;
	char *buf = malloc(PAGE_SIZE);
	struct pollfd pollfd;

	do {
		pollfd.fd = read_fd;
		pollfd.events = POLLIN;
		rc = poll(&pollfd, 1, 1);
		if (rc < 0) {
			if (errno != EINTR) {
				printf("poll error: %s\n", strerror(errno));
				break;
			}
			printf("poll warning: %s\n", strerror(errno));
		}
		rc = read(read_fd, buf, PAGE_SIZE);
		if (!rc)
			continue;
		if (rc < 0) {
			if (errno == EAGAIN)
				continue;
			perror("read");
			break;
		}
		// printf("Read bytes=%d\n", rc);

		if (write(write_fd, buf, rc) < 0) {
			printf("Couldn't write to output file: errcode = %d: %s\n", errno, strerror(errno));
			break;
		}
	} while(1);

	free(buf);
}

int main(int argc, char **argv)
{
	int in_fd, out_fd;
	char *in_fname, *out_fname;

	if (argc != 3) {
		printf("Usage: sr_collect <input file> <output file>\n");
		return 0;
	}

	in_fname = argv[1];
	in_fd = open(in_fname, O_RDONLY | O_NONBLOCK);
	if (!in_fd) {
		printf("Error opening input file: %s\n", in_fname);
		return 0;
	}

	out_fname = argv[2];
	out_fd = open(out_fname, O_CREAT | O_RDWR | O_TRUNC);
	if (!out_fd) {
		printf("Error opening output file: %s\n", out_fname);
		close(in_fd);
		return 0;
	}
	
	dump_file(in_fd, out_fd);

	close(in_fd);
	close(out_fd);
	return 0;
}
