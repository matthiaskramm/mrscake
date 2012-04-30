all: test_codegen test_net

CC=gcc -g

RUBY_LIBS=-lruby18
PYTHON_LIBS=-lpthread -lrt -lpython2.6
JS_LIBS=-lmozjs185
LIBS=$(JS_LIBS) $(PYTHON_LIBS) $(RUBY_LIBS) -lz -lcrypto

RUBY_CFLAGS=-I/usr/lib/ruby/1.8/i686-linux -I. -D_FILE_OFFSET_BITS=64 -fPIC -O2 -march=core2 -mtune=core2 -pipe -fno-strict-aliasing -fPIC
RUBY_LDFLAGS=-L/usr/lib -Wl,-R/usr/lib -L. -Wl,-O1 -rdynamic -Wl,-export-dynamic -L.. -Wl,-R -Wl,/usr/lib -L/usr/lib -lruby18 -lz -ldl -lcrypt -lm -lc

../mrscake.a: ../*.c
	cd ..;make mrscake.a

test_datasets.o: test_datasets.c
	$(CC) -I.. -c test_datasets.c

test_codegen.o: test_codegen.c language_interpreter.h
	$(CC) -I.. -c test_codegen.c

test_net.o: test_net.c test_datasets.h
	$(CC) -I.. -c test_net.c

language_interpreter.o: language_interpreter.c language_interpreter.h
	$(CC) -c -I/usr/include/js/ language_interpreter.c

language_js.o: language_js.c language_interpreter.h
	$(CC) -c -I/usr/include/js/ language_js.c

language_py.o: language_py.c language_interpreter.h
	$(CC) -c -I/usr/include/python2.6/ language_py.c

language_rb.o: language_rb.c language_interpreter.h
	$(CC) -c -I /usr/lib/ruby/1.8/i686-linux language_rb.c

test_codegen: test_codegen.o test_datasets.o language_js.o language_py.o language_rb.o language_interpreter.o ../mrscake.a
	$(CC) test_codegen.o test_datasets.o language_js.o language_py.o language_rb.o language_interpreter.o ../mrscake.a $(LIBS) -o $@ 

test_net: test_net.o test_datasets.o ../mrscake.a
	$(CC) test_net.o test_datasets.o language_js.o language_py.o language_rb.o language_interpreter.o ../mrscake.a $(LIBS) -o $@ 

clean:
	rm -f *.o test_codegen

.PHONY: all clean
