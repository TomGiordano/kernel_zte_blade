all:
	@make -C ../../allocators/xvmalloc_defrag
	@gcc -g -Wall xvmalloc_defrag_stress.c ../../allocators/xvmalloc_defrag/xvmalloc.o \
		-L../../alloc_stress/simprofile -lsimparse -o xvmalloc_defrag_stress_bin

clean:
	@make -C ../../allocators/xvmalloc_defrag clean
	@rm -rf *.o xvmalloc_defrag_stress_bin
