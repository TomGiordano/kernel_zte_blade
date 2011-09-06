obj-m += tlsf_kmod_test.o ../../allocators/tlsf-kmod/tlsf.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
