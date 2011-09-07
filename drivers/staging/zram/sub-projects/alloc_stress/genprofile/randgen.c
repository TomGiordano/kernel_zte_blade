#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef unsigned char u8;
typedef unsigned int u32;

const u32 MIN = 32;
const u32 MAX = 4096 / 2;
//const u32 MAX = (4096 * 3/4);

const u32 PREFER_MIN = 580;
const u32 PREFER_MAX = 700;

//const u32 PREFER_MIN = 32;
//const u32 PREFER_MAX = 4096 / 2;

const u32 MAX_ALLOC_COUNT = (1 << 16);

typedef struct {
	u32 min, max;
} Range;

// Gloabals
u32 currAllocCount;
Range fullRange, preferRange;

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

u32 randomRanged(Range r, Range p, double biasForPrefRange)
{
	double rval;
	Range *s;

	if (Flip(biasForPrefRange)) {
		s = &r;
	} else {
		s = &p;
	}

	rval = (rand() / (RAND_MAX + 1.0)) * (s->max - s->min) + s->min;
	return (u32)rval;
}

void EventAlloc(void)
{
	u32 randSize;

	if (currAllocCount == MAX_ALLOC_COUNT) {
		return;
	}

	randSize = randomRanged(fullRange, preferRange, 0.8);
	printf("M %u\n", randSize);
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

int main()
{
	u32 i;

	// initialize globals
	currAllocCount = 0;
	
	fullRange.min = MIN;
	fullRange.max = MAX;

	preferRange.min = PREFER_MIN;
	preferRange.max = PREFER_MAX;

	// seed rand gen
	srand((unsigned)time(NULL));

	// randomly generate alloc/free events
	for (i = 0; i < 1000000; i++) {
		if (Flip(0.5))
			EventAlloc();
		else
			EventFree();
	}

	// generate free events for all remaining blocks
	for (i = currAllocCount; i > 0; i--) {
		EventFree();
	}	

	return 0;
}

