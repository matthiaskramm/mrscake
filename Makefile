all: multimodel ast model predict.so prediction.so

CC=gcc -pg -g -fPIC
CXX=g++ -pg -g -fPIC
PYTHON_LIB=-lpython2.6
PYTHON_INCLUDE=-I/usr/include/python2.6
RUBY_LIB=-lruby
RUBY_INCLUDE=-I/usr/lib/ruby/1.8/universal-darwin10.0

IS_MACOS:=$(shell test -d /Library && echo macos)

LIBS=-lz -lpthread
ifeq ($(IS_MACOS),)
    RUBY_LIB=-lruby18
    LIBS=-lz -lpthread -lrt
    RUBY_INCLUDE=-I/usr/lib/ruby/1.8/i686-linux/ 
endif

MODELS=model_cv_dtree.o model_cv_ann.o model_cv_svm.o model_cv_linear.o
CODE_GENERATORS=codegen_python.o codegen_c.o
OBJECTS=$(MODELS) $(CODE_GENERATORS) cvtools.o constant.o ast.o model.o serialize.o io.o list.o model_select.o dict.o dataset.o environment.o codegen.o ast_transforms.o stringpool.o

lib/libml.a: lib/*.cpp lib/*.hpp lib/*.h
	cd lib;make libml.a

multimodel.o: multimodel.cpp
	$(CXX) -Ilib $< -c -o $@

model.o: model.c constant.h ast.h
	$(CC) -c $< -o $@

model_select.o: model_select.c constant.h ast.h
	$(CC) -c $< -o $@

ast.o: ast.c ast.h model.h
	$(CC) -c $< -o $@

ast_transforms.o: ast_transforms.c ast_transforms.h ast.h
	$(CC) -c $< -o $@

constant.o: constant.c constant.h model.h
	$(CC) -c $< -o $@

environment.o: environment.c environment.h model.h
	$(CC) -c $< -o $@

dataset.o: dataset.c dataset.h model.h
	$(CC) -c $< -o $@

dict.o: dict.c dict.h model.h
	$(CC) -c $< -o $@

list.o: list.c list.h
	$(CC) -c $< -o $@

serialize.o: serialize.c serialize.h ast.h constant.h
	$(CC) -c $< -o $@

stringpool.o: stringpool.c stringpool.h
	$(CC) -c $< -o $@

codegen.o: codegen.c codegen.h
	$(CC) -c $< -o $@

codegen_python.o: codegen_python.c codegen.h
	$(CC) -c $< -o $@

io.o: io.c io.h
	$(CC) -c $< -o $@

cvtools.o: cvtools.cpp lib/ml.hpp dataset.h
	$(CXX) -Ilib $< -c -o $@

model_cv_dtree.o: model_cv_dtree.cpp model.h ast.h cvtools.h dataset.h easy_ast.h
	$(CXX) -Ilib $< -c -o $@

model_cv_svm.o: model_cv_svm.cpp model.h ast.h cvtools.h dataset.h easy_ast.h
	$(CXX) -Ilib $< -c -o $@

model_cv_ann.o: model_cv_ann.cpp model.h ast.h cvtools.h dataset.h easy_ast.h
	$(CXX) -Ilib $< -c -o $@

model_cv_linear.o: model_cv_linear.cpp model.h ast.h cvtools.h dataset.h easy_ast.h
	$(CXX) -Ilib $< -c -o $@

test_model.o: test_model.c model.h
	$(CC) -c $< -o $@

test_ast.o: test_ast.c model.h ast.h
	$(CC) -c $< -o $@

ast: test_ast.o $(OBJECTS) lib/libml.a
	$(CXX) test_ast.o $(OBJECTS) lib/libml.a -o $@ $(LIBS)

model: test_model.o $(OBJECTS) lib/libml.a
	$(CXX) test_model.o $(OBJECTS) lib/libml.a -o $@ $(LIBS)

# ------------ python interface --------------

predict.so: predict.py.c model.h list.h $(OBJECTS) lib/libml.a
	$(CC) $(PYTHON_INCLUDE) -shared predict.py.c $(OBJECTS) lib/libml.a -o $@ $(LIBS) $(PYTHON_LIB) -lstdc++

python_interpreter: python_interpreter.c
	$(CC) $(PYTHON_INCLUDE) python_interpreter.c -o python_interpreter $(PYTHON_LIB)

# ------------ ruby interface ----------------

prediction.so: predict.rb.c model.h $(OBJECTS)
	$(CC) $(RUBY_INCLUDE) -shared predict.rb.c $(OBJECTS) lib/libml.a -o $@ $(LIBS) $(RUBY_LIB) -lstdc++

# ------------ old test code -----------------

multimodel: multimodel.o lib/libml.a $(OBJECTS)
	$(CXX) multimodel.o $(OBJECTS) lib/libml.a -o $@ $(LIBS)

test: predict.so
	python test_python_module.py

local-clean:
	rm -f svm test ast ann multimodel *.o

clean: local-clean
	rm -f lib/*.o lib/*.a lib/*.gch


.PHONY: clean all test local-clean
