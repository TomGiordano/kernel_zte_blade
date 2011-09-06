all:
	@make -C ../../allocators/xvmalloc
	@gcc -g -Wall xvmalloc_stress.c ../../allocators/xvmalloc/xvmalloc.o \
		-L../../alloc_stress/simprofile -lsimparse -o xvmalloc_stress_bin

clean:
	@make -C ../../allocators/xvmalloc clean
	@rm -rf *.o xvmalloc_stress_bin
