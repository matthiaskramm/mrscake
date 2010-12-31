all: multimodel svm ann ast

CC=gcc -g
CXX=g++ -g

lib/libml.a: lib/*.cpp lib/*.hpp lib/*.h
	cd lib;make libml.a

multimodel.o: multimodel.cpp Makefile
	$(CXX) -Ilib $< -c -o $@

model.o: model.c Makefile
	$(CC) -c $< -o $@

ast.o: ast.c ast.h model.h Makefile
	$(CC) -c $< -o $@

svm.o: svm.cpp Makefile
	$(CXX) -Ilib $< -c -o $@

ann.o: ann.cpp Makefile
	$(CXX) -Ilib $< -c -o $@

test_ast.o: test_ast.c ast.h model.h Makefile
	$(CC) -c $< -o $@

ast: test_ast.o ast.o model.o
	$(CC) test_ast.o ast.o model.o -o $@

multimodel: multimodel.o lib/libml.a Makefile 
	$(CXX) multimodel.o -o $@ lib/libml.a -lz -lpthread -lrt

svm: svm.o lib/libml.a Makefile 
	$(CXX) svm.o -o $@ lib/libml.a -lz -lpthread -lrt

ann: ann.o lib/libml.a Makefile 
	$(CXX) ann.o -o $@ lib/libml.a -lz -lpthread -lrt

test: ast
	./ast	

clean:
	rm -f svm test ast ann multimodel *.o


.PHONY: clean
