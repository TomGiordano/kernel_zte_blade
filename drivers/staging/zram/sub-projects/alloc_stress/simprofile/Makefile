all:
# Compile objects to be included in lib
	@gcc -fPIC -Wall -g -c libsimparse.c

# Create shared lib
	@gcc -g -shared -Wl,-soname,libsimparse.so.1 -o \
		libsimparse.so.1.0 libsimparse.o -lc

# Create symlinks
	@ln -sf libsimparse.so.1.0 libsimparse.so.1
	@ln -sf libsimparse.so.1 libsimparse.so

sample:
# Compile sample app: sim_profile_dump (uses libsimparse)
	@gcc -g -Wall sim_profile_dump.c -L. -lsimparse -o sim_profile_dump_bin

clean:
	@rm -rf *.o *.so* sim_profile_dump_bin
