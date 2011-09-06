#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libsr_parse.h"
#include "../sr_relay/sr_relay.h"

#define READ		0
#define WRITE		1
#define MAX_SR_FILE_NAME_LEN	256

#define ERR_PREFIX	"libsr_parse: "
#define ERR(fmt, arg...) \
	printf(ERR_PREFIX fmt "\n", ##arg)

struct sr_handle {
	char sr_file[MAX_SR_FILE_NAME_LEN];
	swap_read_fn *swap_read;
	swap_write_fn *swap_write;
	replay_end_fn *replay_end;
	int init_done;
};

static void parse_file(struct sr_handle *handle, int fd)
{
	char rw;
	int rc;
	unsigned int index;
	struct sr_info sr_info;

	while (1) {
		rc = read(fd, &sr_info, sizeof(struct sr_info));
		if (!rc)
			break;
		if (rc < 0) {
			ERR("read failed: err=%d", rc);
			return;
		}
		if (rc != sizeof(struct sr_info)) {
			ERR("partial read!");
			return;
		}
		rw = sr_info.page_rw & 1;
		index = sr_info.page_rw >> 1;

		if (rw == READ)
			(*handle->swap_read)(index);
		else
			(*handle->swap_write)(index, sr_info.clen, sr_info.hash);
	};

	(*handle->replay_end)(rc);
}

void *sr_gethandle(void)
{
	struct sr_handle *handle;
	handle = calloc(1, sizeof(*handle));
	if (handle == NULL) {
		ERR("error allocating swap handle");
		return NULL;
	}
	handle->init_done = 0;
	return handle;
}

void sr_puthandle(void *handle)
{
	free(handle);
}

int set_sr_file(void *handle, const char *fname)
{
	int len;
	struct sr_handle *srh = (struct sr_handle *)handle;

	len = strlen(fname);
	if (len >= MAX_SR_FILE_NAME_LEN) {
		ERR("file name too long!");
		return -1;
	}
	strcpy(srh->sr_file, fname);
	return 0;
}

int set_sr_callbacks(void *handle,
			swap_read_fn sr,
			swap_write_fn sw,
			replay_end_fn end)
{
	struct sr_handle *srh = (struct sr_handle *)handle;

	if (srh == NULL || sr == NULL || sw == NULL || end == NULL)
		return -1;

	srh->swap_read = sr;
	srh->swap_write = sw;
	srh->replay_end = end;
	srh->init_done = 1;

	return 0;
}

int sr_start(void *handle)
{
	int fd;
	struct sr_handle *srh = (struct sr_handle *)handle;
	if (srh == NULL) {
		ERR("invalid handle");
		return -1;
	}

	if (srh->init_done != 1) {
		ERR("Callback functions not set");
		return -1;
	}

	fd = open(srh->sr_file, O_RDONLY | O_NONBLOCK);
	if (!fd) {
		ERR("Error opening file: %s", srh->sr_file);
		return -1;
	}
	
	parse_file(srh, fd);

	close(fd);
	return 0;
}
