all:
	@make -C ../../lib
	@make -C ../../swap_replay
	@gcc -Wall cps_test.c ../../lib/hashtable.o \
		-L../../swap_replay/sr_parse -lsr_parse -o cps_test_bin

clean:
	@rm -rf cps_test_bin
