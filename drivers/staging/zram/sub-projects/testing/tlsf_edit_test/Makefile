all:
	@gcc -Wall tlsf_edit_test.c -L../../swap_replay/sr_parse -lsr_parse \
			../../allocators/tlsf-edit/tlsf.o -o tlsf_edit_test_bin

clean:
	@rm -rf tlsf_edit_test_bin
