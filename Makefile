all: multimodel svm ann ast model predict.so

CC=gcc -pg -g -fPIC
CXX=g++ -pg -g -fPIC
PYTHON_LIB=-lpython2.6
PYTHON_INCLUDE=-I/usr/include/python2.6

IS_MACOS:=$(shell test -d /Library && echo macos)

LIBS=-lz -lpthread
ifeq ($(IS_MACOS),)
    LIBS=-lz -lpthread -lrt
endif

MODELS=model_cv_dtree.o model_cv_ann.o model_cv_svm.o
OBJECTS=$(MODELS) cvtools.o constant.o ast.o model.o serialize.o io.o list.o model_select.o dict.o dataset.o environment.o

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

environment.o: environment.c environment.h model.h Makefile
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

model_cv_svm.o: model_cv_svm.cpp model.h ast.h cvtools.h Makefile
	$(CXX) -Ilib $< -c -o $@

model_cv_ann.o: model_cv_ann.cpp model.h ast.h cvtools.h Makefile
	$(CXX) -Ilib $< -c -o $@

test_model.o: test_model.c model.h Makefile
	$(CC) -c $< -o $@

test_ast.o: test_ast.c model.h ast.h Makefile
	$(CC) -c $< -o $@

ast: test_ast.o $(OBJECTS) lib/libml.a Makefile
	$(CXX) test_ast.o $(OBJECTS) lib/libml.a -o $@ $(LIBS)

model: test_model.o $(OBJECTS) lib/libml.a Makefile 
	$(CXX) test_model.o $(OBJECTS) lib/libml.a -o $@ $(LIBS)

# ------------ python interface --------------

predict.so: predict.py.c model.h list.h $(OBJECTS) lib/libml.a
	$(CC) $(PYTHON_INCLUDE) -shared predict.py.c $(OBJECTS) lib/libml.a -o predict.so $(LIBS) $(PYTHON_LIB) -lstdc++

python_interpreter: python_interpreter.c Makefile
	$(CC) $(PYTHON_INCLUDE) python_interpreter.c -o python_interpreter $(PYTHON_LIB)
# ------------ old test code -----------------

multimodel: multimodel.o lib/libml.a $(OBJECTS) Makefile 
	$(CXX) multimodel.o $(OBJECTS) lib/libml.a -o $@ $(LIBS)

svm.o: svm.cpp Makefile
	$(CXX) -Ilib $< -c -o $@

ann.o: ann.cpp Makefile
	$(CXX) -Ilib $< -c -o $@

svm: svm.o lib/libml.a Makefile 
	$(CXX) svm.o -o $@ lib/libml.a $(LIBS)

ann: ann.o lib/libml.a Makefile 
	$(CXX) ann.o -o $@ lib/libml.a $(LIBS)


test: predict.so
	python test_python_module.py

clean:
	rm -f svm test ast ann multimodel *.o


.PHONY: clean all
