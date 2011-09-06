#ifndef _LIB_SR_PARSE_H_
#define _LIB_SR_PARSE_H_

#include "../../include/types.h"

typedef u64 (MallocFn)(u32 size);
typedef u64 (MallocWithBackRefFn)(u32 size, u32 backRef);
typedef void (FreeFn)(u64 location);
typedef void (simEndFn)(void);

void * simGetHandle(void);
void simPutHandle(void *handle);
void *simGetTable(void);
int setSimFile(void *handle, const char *fileName);
int setSimCallbacks(void *handle, MallocFn, FreeFn, simEndFn);
int setSimCallbackMallocWithBackRef(void *handle,
				MallocWithBackRefFn simMallocBackRef);
int simStart(void *);

#endif
