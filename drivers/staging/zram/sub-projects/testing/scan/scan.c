#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

int main(int argc, char **argv)
{
	char *ptr;
	int size = -1;
	int loops = -1;
	if (argc > 1) {
		size = atoi(argv[1]);
	}
	if (argc > 2) {
		loops = atoi(argv[2]);
	}

	if (size <= 0) {
		printf("no size specified\n");
		return 0;
	}
	if (loops <= 0) {
		printf("no loops specified\n");
		return 0;
	}

	printf("Size: %dMB\n", size);
	printf("Loops: %d\n", loops);
	size *= 1024*1024;

	ptr = (char*)mmap(0, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, 0, 0);
	if (ptr) {
		for (;loops; --loops) {
			int i;
			for (i=0; i<size; ++i) {
				*(ptr + i) = loops;
			}
			printf(".");
			fflush(stdout);
		}
		printf("\n");
		munmap(ptr, size);
	}
	return 0;
}
