all: test

test.o: test.c
	gcc -c test.c

language_js.o: language_js.c
	gcc -c -I/usr/include/js/ language_js.c

test: test.o language_js.o
	gcc test.o language_js.o -lmozjs -o test
