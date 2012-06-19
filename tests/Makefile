all: test_codegen test_net model

CC=gcc -g -DHAVE_SHA1 -I..
CXX=g++ -g -DHAVE_SHA1

RUBY_LIBS=-lruby18
PYTHON_LIBS=-lpthread -lrt -lpython2.6
JS_LIBS=-lmozjs185
LUA_LIBS=-llua
LIBS=$(JS_LIBS) $(PYTHON_LIBS) $(RUBY_LIBS) $(LUA_LIBS) -lz -lcrypto
O=o

RUBY_CFLAGS=-I/usr/lib/ruby/1.8/i686-linux -I. -D_FILE_OFFSET_BITS=64 -fPIC -O2 -march=core2 -mtune=core2 -pipe -fno-strict-aliasing -fPIC
RUBY_LDFLAGS=-L/usr/lib -Wl,-R/usr/lib -L. -Wl,-O1 -rdynamic -Wl,-export-dynamic -L.. -Wl,-R -Wl,/usr/lib -L/usr/lib -lruby18 -lz -ldl -lcrypt -lm -lc

../mrscake.a: ../*.c
	cd ..;make mrscake.a

test_datasets.o: test_datasets.c
	$(CC) -c test_datasets.c

test_codegen.o: test_codegen.c language_interpreter.h
	$(CC) -c test_codegen.c

test_net.o: test_net.c test_datasets.h
	$(CC) -c test_net.c

language_interpreter.o: language_interpreter.c language_interpreter.h
	$(CC) -c -I/usr/include/js/ language_interpreter.c

language_js.o: language_js.c language_interpreter.h
	$(CC) -c -I/usr/include/js/ language_js.c

language_py.o: language_py.c language_interpreter.h
	$(CC) -c -I/usr/include/python2.6/ language_py.c

language_rb.o: language_rb.c language_interpreter.h
	$(CC) -c -I /usr/lib/ruby/1.8/i686-linux language_rb.c

language_lua.o: language_lua.c language_interpreter.h
	$(CC) -c language_lua.c

test_codegen: test_codegen.o test_datasets.o language_js.o language_py.o language_rb.o language_lua.o language_interpreter.o ../mrscake.a
	$(CC) test_codegen.o test_datasets.o language_js.o language_py.o language_rb.o language_lua.o language_interpreter.o ../mrscake.a $(LIBS) -o $@ 

test_net: test_net.o test_datasets.o ../mrscake.a
	$(CC) test_net.o test_datasets.o language_js.o language_py.o language_rb.o language_interpreter.o ../mrscake.a $(LIBS) -o $@ 

test_model.$(O): test_model.c ../settings.h ../mrscake.h
	$(CC) -c $< -o $@

test_ast.$(O): test_ast.c ../mrscake.h ../ast.h
	$(CC) -c $< -o $@

test_subset.$(O): test_subset.c ../mrscake.h ../ast.h
	$(CC) -c $< -o $@

test_cv.$(O): test_cv.cpp
	$(CXX) -Ilib $< -c -o $@

ast: test_ast.$(O) $(OBJECTS) ../mrscake.a
	$(CXX) test_ast.$(O) $(OBJECTS) ../mrscake.a -o $@ $(LIBS)

model: test_model.$(O) $(OBJECTS) ../mrscake.a
	$(CXX) test_model.$(O) $(OBJECTS) ../mrscake.a -o $@ $(LIBS)

subset: test_subset.$(O) $(OBJECTS) ../mrscake.a
	$(CXX) test_subset.$(O) $(OBJECTS) ../mrscake.a -o $@ $(LIBS)

test_server: test_server.$(O) $(OBJECTS) ../mrscake.a
	$(CXX) test_server.$(O) $(OBJECTS) ../mrscake.a -o $@ $(LIBS)

test_cv: test_cv.$(O) lib/libml.a $(OBJECTS) ../mrscake.a 
	$(CXX) test_cv.$(O) $(OBJECTS) ../mrscake.a -o $@ $(LIBS)

clean:
	rm -f *.o test_codegen lua

.PHONY: all clean
