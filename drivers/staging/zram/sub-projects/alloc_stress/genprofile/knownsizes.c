#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef unsigned char u8;
typedef unsigned int u32;

const u32 MIN = 32;
const u32 MAX = 4096 / 2;

const u32 MAX_ALLOC_COUNT = (1 << 20);

// Gloabals
u32 currAllocCount;

/*
 * Randomly generate 0 or 1
 *
 * arg: biasForZero: how much to favor 0's --
 * 	0.5: almost same chance for 0/1
 * 	1:   always 0
 * 	0:   always 1
 */
u32 Flip(double biasForZero)
{
	double rval = rand() / (RAND_MAX + 1.0);
	return (rval > biasForZero ? 1 : 0);
}

// Generate random numbers in range [0, max)
u32 randomLimit(u32 max)
{
	double rval = rand() % max;
	return rval;
}

void EventAlloc(size)
{
	if (currAllocCount == MAX_ALLOC_COUNT) {
		return;
	}

	printf("M %u\n", size);
	currAllocCount++;
}

void EventFree(void)
{
	u32 freeIndex;

	if (currAllocCount == 0) {
		return;
	}

	freeIndex = randomLimit(currAllocCount);
	printf("F %u\n", freeIndex);	
	currAllocCount--;
}

static void Usage(void)
{
	printf("knowsizes <file>\n"
		"\t<file> contains list of valid malloc sizes\n");
}

static void ParseFile(FILE *file)
{
	char line[64];
	unsigned int size;

	while (1) {
		if (Flip(0.5)) {
			// alloc event
			if (fgets(line, sizeof(line), file)) {
				sscanf(line, "%u\n", &size);
				if (size <= MAX)
					EventAlloc(size);
			} else { // EOF
				break;
			}
		} else {
			EventFree();
		}
	};
}

int main(int argc, char *argv[])
{
#define MAX_FNAME_LEN 100
	u32 i;
	char fname[MAX_FNAME_LEN];
	FILE *file;

	// Initialize globals
	currAllocCount = 0;
	
	if (argc != 2) {
		Usage();
		return -EINVAL;
	}
	
	strncpy(fname, argv[1], MAX_FNAME_LEN);
	fname[MAX_FNAME_LEN - 1] = '\0';

	file = fopen(fname, "r");
	if (!file) {
		printf("Error opening file [%s]\n", fname);
		return -EINVAL;
	}

	// Seed rand gen
	srand((unsigned)time(NULL));
	
	/*
	 * Generate alloc events taking sizes from file.
	 * Also mix-in random free events.
	 */
	ParseFile(file);

	// Generate free events for all remaining blocks
	for (i = currAllocCount; i > 0; i--) {
		EventFree();
	}	

	fclose(file);

	return 0;
}

