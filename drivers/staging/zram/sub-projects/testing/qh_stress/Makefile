all:
	@make -C ../../allocators/xvmalloc
	@gcc -g -Wall qh_stress.c ../../allocators/xvmalloc/xvmalloc.o \
		-L../../alloc_stress/simprofile -lsimparse -o qh_stress_bin

clean:
	@make -C ../../allocators/xvmalloc clean
	@rm -rf *.o qh_stress_bin
