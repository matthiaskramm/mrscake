all: test_codegen

test_codegen.o: test_codegen.c language_interpreter.h
	gcc -g -c test_codegen.c

language_js.o: language_js.c language_interpreter.h
	gcc -g -c -I/usr/include/js/ language_js.c

test_codegen: test_codegen.o language_js.o ../mrscake.a
	gcc -g test_codegen.o language_js.o -lmozjs185 ../mrscake.a -o $@ -lmozjs185 -lz -lpthread -lrt
