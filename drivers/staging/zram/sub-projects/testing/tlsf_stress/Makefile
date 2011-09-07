all:
	@make -C ../../allocators/tlsf-edit
	@gcc -g -Wall tlsf_stress.c ../../allocators/tlsf-edit/tlsf.o \
		-L../../alloc_stress/simprofile -lsimparse -o tlsf_stress_bin

clean:
	@make -C ../../allocators/tlsf-edit clean
	@rm -rf *.o tlsf_stress_bin
