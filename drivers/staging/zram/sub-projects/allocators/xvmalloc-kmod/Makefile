EXTRA_CFLAGS	:=	-g -O2 -Wall
KERNEL_BUILD_PATH ?= "/lib/modules/$(shell uname -r)/build"

obj-m		+=	xvmalloc.o

all:
	make -C $(KERNEL_BUILD_PATH) M=$(PWD) modules

clean:
	make -C $(KERNEL_BUILD_PATH) M=$(PWD) clean
	@$(RM) -rf *.o *~ *.c.gcov *.gcda *.gcno cscope.* tags

