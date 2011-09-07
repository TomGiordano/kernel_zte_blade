all:
	@gcc -Wall dlm_test.c \
			 -L../../swap_replay/sr_parse -lsr_parse \
			../../allocators/dlmalloc-edit/dlmalloc.o -o dlm_test_bin

clean:
	@rm -rf dlm_test_bin
