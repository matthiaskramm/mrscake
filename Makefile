all: multimodel svm ann ast model

CC=gcc -g
CXX=g++ -g

MODELS=model_cv_dtree.o
OBJECTS=$(MODELS) cvtools.o constant.o ast.o model.o

lib/libml.a: lib/*.cpp lib/*.hpp lib/*.h
	cd lib;make libml.a

multimodel.o: multimodel.cpp Makefile
	$(CXX) -Ilib $< -c -o $@

model.o: model.c constant.h ast.h Makefile
	$(CC) -c $< -o $@

ast.o: ast.c ast.h model.h Makefile
	$(CC) -c $< -o $@

constant.o: constant.c constant.h model.h Makefile
	$(CC) -c $< -o $@

cvtools.o: cvtools.cpp lib/ml.hpp Makefile
	$(CXX) -Ilib $< -c -o $@

model_cv_dtree.o: model_cv_dtree.cpp model.h ast.h Makefile
	$(CXX) -Ilib $< -c -o $@

test_model.o: test_model.c model.h model.h Makefile
	$(CC) -c $< -o $@

ast: test_ast.o $(OBJECTS) lib/libml.a Makefile
	$(CXX) test_ast.o $(OBJECTS) lib/libml.a -o $@ -lz -lpthread -lrt

model: test_model.o $(OBJECTS) lib/libml.a Makefile 
	$(CXX) test_model.o $(OBJECTS) lib/libml.a -o $@ -lz -lpthread -lrt

# ------------ old test code -----------------

multimodel: multimodel.o lib/libml.a Makefile 
	$(CXX) multimodel.o -o $@ lib/libml.a -lz -lpthread -lrt

svm.o: svm.cpp Makefile
	$(CXX) -Ilib $< -c -o $@

ann.o: ann.cpp Makefile
	$(CXX) -Ilib $< -c -o $@

svm: svm.o lib/libml.a Makefile 
	$(CXX) svm.o -o $@ lib/libml.a -lz -lpthread -lrt

ann: ann.o lib/libml.a Makefile 
	$(CXX) ann.o -o $@ lib/libml.a -lz -lpthread -lrt


test: ast
	./ast	

clean:
	rm -f svm test ast ann multimodel *.o


.PHONY: clean
