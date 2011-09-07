all:
# Compile objects to be included in lib
	@gcc -fPIC -Wall -g -c libsr_parse.c

# Create shared lib
	@gcc -g -shared -Wl,-soname,libsr_parse.so.1 -o \
		libsr_parse.so.1.0 libsr_parse.o -lc

# Create symlinks
	@ln -sf libsr_parse.so.1.0 libsr_parse.so.1
	@ln -sf libsr_parse.so.1 libsr_parse.so

sample:
# Compile sample app: sr_data_histo (uses libsr_parse)
	@gcc -Wall sr_data_histo.c ../../lib/hexdump.c -L. -lsr_parse -o sr_data_histo_bin

clean:
	@rm -rf *.o *.so* dump_sr_data_bin
