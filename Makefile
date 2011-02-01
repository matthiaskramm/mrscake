CC=gcc -pg -g -fPIC
CXX=g++ -pg -g -fPIC
INSTALL=/usr/bin/install -c

IS_MACOS:=$(shell test -d /Library && echo macos)

# Mac defaults
LIBS=-lz -lpthread
PYTHON_LIB?=-lpython2.6
PYTHON_INCLUDE?=-I/usr/include/python2.6
PYTHON_INSTALLDIR?=/usr/lib/python2.6/
RUBY_LDFLAGS?=-shared
RUBY_LIB?=-lruby
RUBY_INCLUDE?=-I/usr/lib/ruby/1.8/universal-darwin10.0
RUBY_INSTALLDIR?=/usr/lib/ruby/1.8/universal-darwin10.0
SO_PYTHON=so
SO_RUBY=bundle

ifeq ($(IS_MACOS),)
    # Linux defaults
    LIBS=-lz -lpthread -lrt
    PYTHON_LIB?=-lpython2.6
    PYTHON_INCLUDE?=-I/usr/include/python2.6
    PYTHON_INSTALLDIR?=/usr/lib/python2.6/site-packages/
    RUBY_LDFLAGS?=-shared 
    RUBY_LIB?=-lruby18
    RUBY_INCLUDE?=-I/usr/lib/ruby/1.8/i686-linux/ 
    RUBY_INSTALLDIR?=/usr/lib/ruby/1.8/i686-linux/ 
    SO_PYTHON=so
    SO_RUBY=rb.so
endif

MODELS=model_cv_dtree.o model_cv_ann.o model_cv_svm.o model_cv_linear.o
CODE_GENERATORS=codegen_python.o codegen_ruby.o codegen_js.o codegen_c.o
OBJECTS=$(MODELS) $(CODE_GENERATORS) cvtools.o constant.o ast.o model.o serialize.o io.o list.o model_select.o dict.o dataset.o environment.o codegen.o ast_transforms.o stringpool.o

all: multimodel ast model predict.$(SO_PYTHON) predict.$(SO_RUBY)

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

predict.$(SO_PYTHON): predict.py.c model.h list.h $(OBJECTS) lib/libml.a
	$(CC) $(PYTHON_INCLUDE) -shared predict.py.c $(OBJECTS) lib/libml.a -o $@ $(LIBS) $(PYTHON_LIB) -lstdc++

python_interpreter: python_interpreter.c
	$(CC) $(PYTHON_INCLUDE) python_interpreter.c -o python_interpreter $(PYTHON_LIB)

# ------------ ruby interface ----------------

predict.$(SO_RUBY): predict.rb.c model.h $(OBJECTS)
	$(CC) $(RUBY_LDFLAGS) $(RUBY_CPPFLAGS) $(RUBY_INCLUDE) predict.rb.c $(OBJECTS) lib/libml.a -o $@ $(LIBS) $(RUBY_LIB) -lstdc++

# ------------ installation ----------------

install:
	$(INSTALL) predict.$(SO_RUBY) $(RUBY_INSTALLDIR)/predict.$(SO_RUBY)
	$(INSTALL) predict.$(SO_PYTHON) $(PYTHON_INSTALLDIR)/predict.$(SO_PYTHON)

# ------------ old test code -----------------

multimodel: multimodel.o lib/libml.a $(OBJECTS)
	$(CXX) multimodel.o $(OBJECTS) lib/libml.a -o $@ $(LIBS)

test: predict.so
	python test_python_module.py

local-clean:
	rm -f svm test ast ann multimodel *.o predict.$(SO) prediction.$(SO)

clean: local-clean
	rm -f lib/*.o lib/*.a lib/*.gch
	rm -rf *.dSYM


.PHONY: clean all test local-clean
