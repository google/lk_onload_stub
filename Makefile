
.PHONY: all clean distclean lib bin test

all: lib bin test_lk_onload_stub

clean:

distclean: clean
	rm -f liblk_*.so test_lk_onload_stub

lib: liblk_onload_stub.so liblk_onload_stub_ext.so

bin: test_lk_onload_stub

lib%.so: %.c
	gcc -Wall -Werror -fPIC -shared -o $@ $+

test_%: test_%.c lib
	gcc -Wall -Werror -o $@ $< -L. -llk_onload_stub_ext

test: all
	@echo "without preload .."
	@LD_LIBRARY_PATH=. ./test_lk_onload_stub
	@echo "with preload .."
	@LD_LIBRARY_PATH=. LD_PRELOAD=./liblk_onload_stub.so LKOS_LOG_FD=2 ./test_lk_onload_stub && echo OK

