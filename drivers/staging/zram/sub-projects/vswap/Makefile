KERNEL_BUILD_PATH ?= "/lib/modules/$(shell uname -r)/build"

EXTRA_CFLAGS	:=	-g -Wall -O0

obj-m		+=	virtio_vswap.o

all:
	make -C $(KERNEL_BUILD_PATH) M=$(PWD) modules

clean:
	make -C $(KERNEL_BUILD_PATH) M=$(PWD) clean
	@rm -rf *.ko
