all: test svm

lib/libml.a: lib/*.cpp lib/*.hpp lib/*.h
	cd lib;make libml.a

multimodel.o: multimodel.cpp Makefile
	g++ -g -Ilib $< -c -o $@

svm.o: svm.cpp Makefile
	g++ -g -Ilib $< -c -o $@

test: multimodel.o lib/libml.a Makefile 
	g++ -g multimodel.o -o $@ lib/libml.a -lz -lpthread -lrt

svm: svm.o lib/libml.a Makefile 
	g++ -g svm.o -o $@ lib/libml.a -lz -lpthread -lrt

clean:
	rm -f svm test *.o


.PHONY: clean
