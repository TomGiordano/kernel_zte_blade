#WARNINGS = -Wextra -Wall -Wwrite-strings -Wstrict-prototypes \
           -Wmissing-prototypes -Wno-long-long -Wstrict-aliasing=2
CFLAGS+=-g -O0 -Wall -DXV_STATS
# CFLAGS+=-ftest-coverage -fprofile-arcs
CC=gcc

all: xvmalloc.o

xvmalloc.o: xvmalloc_int.h xvmalloc.h xvmalloc.c

xref:
	@cscope -bkqRv
	@ctags -R

clean:
	@$(RM) -rf *.o *~ *.c.gcov *.gcda *.gcno cscope.* tags
