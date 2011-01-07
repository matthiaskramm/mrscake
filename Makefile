all: multimodel svm ann ast model predict.so

CC=gcc -g -fPIC
CXX=g++ -g -fPIC

LIBS=-lz -lpthread -lrt
MODELS=model_cv_dtree.o model_cv_ann.o
OBJECTS=$(MODELS) cvtools.o constant.o ast.o model.o serialize.o io.o list.o model_select.o wordmap.o dict.o dataset.o

lib/libml.a: lib/*.cpp lib/*.hpp lib/*.h
	cd lib;make libml.a

multimodel.o: multimodel.cpp Makefile
	$(CXX) -Ilib $< -c -o $@

model.o: model.c constant.h ast.h Makefile
	$(CC) -c $< -o $@

model_select.o: model_select.c constant.h ast.h Makefile
	$(CC) -c $< -o $@

ast.o: ast.c ast.h model.h Makefile
	$(CC) -c $< -o $@

constant.o: constant.c constant.h model.h Makefile
	$(CC) -c $< -o $@

wordmap.o: wordmap.c wordmap.h model.h Makefile
	$(CC) -c $< -o $@

dataset.o: dataset.c dataset.h model.h Makefile
	$(CC) -c $< -o $@

dict.o: dict.c dict.h model.h Makefile
	$(CC) -c $< -o $@

list.o: list.c list.h Makefile
	$(CC) -c $< -o $@

serialize.o: serialize.c serialize.h ast.h constant.h Makefile
	$(CC) -c $< -o $@

io.o: io.c io.h Makefile
	$(CC) -c $< -o $@

cvtools.o: cvtools.cpp lib/ml.hpp Makefile
	$(CXX) -Ilib $< -c -o $@

model_cv_dtree.o: model_cv_dtree.cpp model.h ast.h cvtools.h Makefile
	$(CXX) -Ilib $< -c -o $@

model_cv_ann.o: model_cv_ann.cpp model.h ast.h cvtools.h Makefile
	$(CXX) -Ilib $< -c -o $@

test_model.o: test_model.c model.h model.h Makefile
	$(CC) -c $< -o $@

ast: test_ast.o $(OBJECTS) lib/libml.a Makefile
	$(CXX) test_ast.o $(OBJECTS) lib/libml.a -o $@ -lz -lpthread -lrt

model: test_model.o $(OBJECTS) lib/libml.a Makefile 
	$(CXX) test_model.o $(OBJECTS) lib/libml.a -o $@ -lz -lpthread -lrt

# ------------ python interface --------------

predict.so: predict.py.c model.h list.h $(OBJECTS) lib/libml.a
	$(CC) -shared -I/usr/include/python2.6/ predict.py.c $(OBJECTS) lib/libml.a -o predict.so $(LIBS) -lpython2.6 -lstdc++

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


test: predict.so
	python test_python_module.py

clean:
	rm -f svm test ast ann multimodel *.o


.PHONY: clean
