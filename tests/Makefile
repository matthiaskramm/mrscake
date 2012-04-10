all: test_codegen

CC=gcc -g

LIBS=-lmozjs185 -lz -lpthread -lrt -lpython2.6

test_codegen.o: test_codegen.c language_interpreter.h
	$(CC) -c test_codegen.c

language_interpreter.o: language_interpreter.c language_interpreter.h
	$(CC) -c -I/usr/include/js/ language_interpreter.c

language_js.o: language_js.c language_interpreter.h
	$(CC) -c -I/usr/include/js/ language_js.c

language_py.o: language_py.c language_interpreter.h
	$(CC) -c -I/usr/include/python2.6/ language_py.c

test_codegen: test_codegen.o language_js.o language_py.o language_interpreter.o ../mrscake.a
	$(CC) test_codegen.o language_js.o language_py.o language_interpreter.o ../mrscake.a $(LIBS) -o $@ 
