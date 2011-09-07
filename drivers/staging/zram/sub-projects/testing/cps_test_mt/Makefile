all:
	@make -C ../../lib
	@make -C ../../swap_replay
	@gcc -Wall cps_test_mt.c -lpthread ../../lib/hashtable.o \
		-L../../swap_replay/sr_parse -lsr_parse -o cps_test_mt_bin

clean:
	@rm -rf cps_test_mt_bin
