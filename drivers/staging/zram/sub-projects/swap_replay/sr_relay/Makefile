obj-m	+=	sr_relay.o ../../compression/lzo-kmod/lzo1x_compress.o

all:
	@make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	@make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean

