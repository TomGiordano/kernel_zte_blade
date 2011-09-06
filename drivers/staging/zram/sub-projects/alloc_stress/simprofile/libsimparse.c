#include <asm/errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "libsimparse.h"

#define unlikely(_exp) __builtin_expect((_exp), 0)

#define MAX_FILE_NAME_LEN 256

static const char MALLOC = 'M';
static const char FREE = 'F';
static const u32 MAX_TABLE_ENTRIES = (1 << 20);
static const u32 PAGE_SIZE = 4096;

#define ERR_PREFIX	"libsimparse: "
#define ERR(fmt, arg...) \
	printf(ERR_PREFIX fmt "\n", ##arg)

#define INIT_STAGE_1	1
#define INIT_STAGE_DONE	2

static u64 *locTable; // TODO: make it per-handle
static u32 currAllocCount = 0;

struct simHandle {
	char simFile[MAX_FILE_NAME_LEN];
	MallocWithBackRefFn *simMallocWithBackRef;
	MallocFn *simMalloc;
	FreeFn *simFree;
	simEndFn *simEnd;
	int initDone;
};

static void EventMalloc(struct simHandle *handle, u32 size)
{
	if (unlikely(currAllocCount == MAX_TABLE_ENTRIES)) {
		ERR("too many allocations\n");
		return;
	}

	if (handle->simMalloc) {
		locTable[currAllocCount] = (u64)(*handle->simMalloc)(size);
	} else if (handle->simMallocWithBackRef) {
		locTable[currAllocCount] = (u64)(*handle->simMallocWithBackRef)
						(size, currAllocCount);
	}
	if (locTable[currAllocCount]) {
		currAllocCount++;
	}
}

static void EventFree(struct simHandle *handle, u32 index)
{
	if (unlikely(currAllocCount == 0)) {
		ERR("nothing to free\n");
		return;
	}

	(*handle->simFree)(locTable[index]);

	currAllocCount--;
	locTable[index] = locTable[currAllocCount];
	locTable[currAllocCount] = 0;
}

static void ParseFile(struct simHandle *handle, FILE *file)
{
	char event;
	u32 value;
	char line[64];

	while (fgets(line, sizeof(line), file)) {
		sscanf(line, "%c %u\n", &event, &value);
		// printf("%c %u\n", event, value);
		if (event == MALLOC) {
			EventMalloc(handle, value);
		} else {
			EventFree(handle, value);
		}
	};

	(*handle->simEnd)();
}

void *simGetHandle(void)
{
	u64 locTableSizePages;
	struct simHandle *handle;

	handle = calloc(1, sizeof(*handle));
	if (handle == NULL) {
		ERR("error allocating swap handle");
		return NULL;
	}

	locTableSizePages = 8 * MAX_TABLE_ENTRIES / PAGE_SIZE + 1;
	if (posix_memalign((void **)&locTable, PAGE_SIZE,
			PAGE_SIZE * locTableSizePages)) {
		printf("Error allocating location table\n");
		free(handle);
		return NULL;
	}

	memset(locTable, 0, PAGE_SIZE * locTableSizePages);
	handle->initDone = 0;

	return handle;
}

void simPutHandle(void *handle)
{
	if (handle == NULL) {
		return;
	}

	free(locTable);
	free(handle);
}

void *simGetTable(void)
{
	return locTable;
}

int setSimFile(void *handle, const char *fname)
{
	int len;
	struct simHandle *sh = (struct simHandle *)handle;

	len = strlen(fname);
	if (len >= MAX_FILE_NAME_LEN) {
		ERR("file name too long!");
		return -1;
	}
	strcpy(sh->simFile, fname);

	return 0;
}

int setSimCallbacks(void *handle,
			MallocFn simMalloc,
			FreeFn simFree,
			simEndFn simEnd)
{
	struct simHandle *sh = (struct simHandle *)handle;

	if (sh == NULL ||
			simFree == NULL ||
			simEnd == NULL) {
		return -1;
	}

	sh->simMalloc = simMalloc;
	sh->simFree = simFree;
	sh->simEnd = simEnd;

	// some other malloc callback set later
	if (sh->simMalloc == NULL) {
		sh->initDone = INIT_STAGE_1;
	}

	return 0;
}

int setSimCallbackMallocWithBackRef(void *handle,
					MallocWithBackRefFn simMallocBackRef)
{
	struct simHandle *sh = (struct simHandle *)handle;
	if (sh == NULL) {
		return -1;
	}

	sh->simMallocWithBackRef = simMallocBackRef;

	if (sh->initDone == INIT_STAGE_1) {
		sh->initDone = INIT_STAGE_DONE;
	}
	return 0;
}

int simStart(void *handle)
{
	FILE *file;
	struct simHandle *sh = (struct simHandle *)handle;

	if (sh == NULL) {
		ERR("invalid handle");
		return -1;
	}

	if (sh->initDone != INIT_STAGE_DONE) {
		ERR("Callback functions not set");
		return -1;
	}

	file = fopen(sh->simFile, "r");
	if (file == NULL) {
		ERR("Error opening file: %s", sh->simFile);
		free(locTable);
		return -EINVAL;
	}
	
	ParseFile(sh, file);

	fclose(file);
	return 0;
}
