WARNINGS = -Wextra -Wall -Wwrite-strings -Wstrict-prototypes \
           -Wmissing-prototypes -Wno-long-long -Wstrict-aliasing=2
CFLAGS+=-g -O2 -I$(TLSF_PATH) $(WARNINGS) -DTLSF_USE_LOCKS=1
# CFLAGS+=-ftest-coverage -fprofile-arcs
CC=gcc -pg

all: tlsf.o test

test: tlsf.o test.o
# 	gcc -Wall test.c tlsf.o -o test

tlsf.o: tlsf.h tlsf.c

clean:
	$(RM) -rf *.o *~ *.c.gcov *.gcda *.gcno
