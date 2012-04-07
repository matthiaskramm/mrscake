CC=gcc -pg -g -fPIC -Wimplicit
CXX=g++ -pg -g -fPIC -Wimplicit
INSTALL=/usr/bin/install -c

IS_MACOS:=$(shell test -d /Library && echo macos)

ifneq ($(IS_MACOS),)
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
endif

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

MODELS=model_cv_dtree.o model_cv_ann.o model_cv_svm.o model_cv_linear.o model_perceptron.o model_knearest.o
VAR_SELECTORS=varselect_cv_dtree.o
CODE_GENERATORS=codegen_python.o codegen_ruby.o codegen_js.o codegen_c.o
OBJECTS=$(MODELS) $(VAR_SELECTORS) $(CODE_GENERATORS) cvtools.o constant.o ast.o model.o serialize.o io.o list.o model_select.o dict.o dataset.o environment.o codegen.o ast_transforms.o stringpool.o net.o settings.o job.o var_selection.o transform.o mrscake.o util.o

all: multimodel ast model subset mrscake-job-server mrscake.$(SO_PYTHON) mrscake.$(SO_RUBY)

lib/libml.a: lib/*.cpp lib/*.hpp lib/*.h
	cd lib;make libml.a

multimodel.o: multimodel.cpp
	$(CXX) -Ilib $< -c -o $@

model.o: model.c constant.h ast.h
	$(CC) -c $< -o $@

model_select.o: model_select.c constant.h ast.h
	$(CC) -c $< -o $@

ast.o: ast.c ast.h mrscake.h
	$(CC) -c $< -o $@

mrscake.o: mrscake.c mrscake.h mrscake.h
	$(CC) -c $< -o $@

net.o: net.c net.h mrscake.h
	$(CC) -c $< -o $@

job.o: job.c job.h
	$(CC) -c $< -o $@

ast_transforms.o: ast_transforms.c ast_transforms.h ast.h
	$(CC) -c $< -o $@

constant.o: constant.c constant.h mrscake.h
	$(CC) -c $< -o $@

environment.o: environment.c environment.h mrscake.h
	$(CC) -c $< -o $@

dataset.o: dataset.c dataset.h mrscake.h
	$(CC) -c $< -o $@

transform.o: transform.c transform.h mrscake.h
	$(CC) -c $< -o $@

dict.o: dict.c dict.h mrscake.h
	$(CC) -c $< -o $@

list.o: list.c list.h
	$(CC) -c $< -o $@

serialize.o: serialize.c serialize.h ast.h constant.h
	$(CC) -c $< -o $@

stringpool.o: stringpool.c stringpool.h
	$(CC) -c $< -o $@

settings.o: settings.c settings.h
	$(CC) -c $< -o $@

var_selection.o: var_selection.c var_selection.h
	$(CC) -c $< -o $@

codegen.o: codegen.c codegen.h
	$(CC) -c $< -o $@

codegen_python.o: codegen_python.c codegen.h
	$(CC) -c $< -o $@

test_server.o: test_server.c
	$(CC) -c $< -o $@

io.o: io.c io.h
	$(CC) -c $< -o $@

cvtools.o: cvtools.cpp lib/ml.hpp dataset.h
	$(CXX) -Ilib $< -c -o $@

model_cv_dtree.o: model_cv_dtree.cpp mrscake.h ast.h cvtools.h dataset.h easy_ast.h
	$(CXX) -Ilib $< -c -o $@

model_cv_svm.o: model_cv_svm.cpp mrscake.h ast.h cvtools.h dataset.h easy_ast.h
	$(CXX) -Ilib $< -c -o $@

model_cv_ann.o: model_cv_ann.cpp mrscake.h ast.h cvtools.h dataset.h easy_ast.h
	$(CXX) -Ilib $< -c -o $@

model_cv_linear.o: model_cv_linear.cpp mrscake.h ast.h cvtools.h dataset.h easy_ast.h
	$(CXX) -Ilib $< -c -o $@

model_perceptron.o: model_perceptron.c mrscake.h ast.h cvtools.h dataset.h easy_ast.h
	$(CC) -Ilib $< -c -o $@

model_knearest.o: model_knearest.c mrscake.h ast.h cvtools.h dataset.h easy_ast.h
	$(CC) -Ilib $< -c -o $@

varselect_cv_dtree.o: varselect_cv_dtree.cpp mrscake.h cvtools.h dataset.h var_selection.h
	$(CXX) -Ilib $< -c -o $@

test_model.o: test_model.c mrscake.h
	$(CC) -c $< -o $@

test_ast.o: test_ast.c mrscake.h ast.h
	$(CC) -c $< -o $@

test_subset.o: test_subset.c mrscake.h ast.h
	$(CC) -c $< -o $@

ast: test_ast.o $(OBJECTS) lib/libml.a
	$(CXX) test_ast.o $(OBJECTS) lib/libml.a -o $@ $(LIBS)

model: test_model.o $(OBJECTS) lib/libml.a
	$(CXX) test_model.o $(OBJECTS) lib/libml.a -o $@ $(LIBS)

subset: test_subset.o $(OBJECTS) lib/libml.a
	$(CXX) test_subset.o $(OBJECTS) lib/libml.a -o $@ $(LIBS)

test_server: test_server.o $(OBJECTS) lib/libml.a
	$(CXX) test_server.o $(OBJECTS) lib/libml.a -o $@ $(LIBS)

mrscake-job-server: server.o $(OBJECTS) lib/libml.a
	$(CXX) server.o $(OBJECTS) lib/libml.a -o $@ $(LIBS)

# ------------ python interface --------------

mrscake.$(SO_PYTHON): mrscake.py.c mrscake.h list.h $(OBJECTS) lib/libml.a
	$(CC) $(PYTHON_INCLUDE) -shared mrscake.py.c $(OBJECTS) lib/libml.a -o $@ $(LIBS) $(PYTHON_LIB) -lstdc++

python_interpreter: python_interpreter.c
	$(CC) $(PYTHON_INCLUDE) python_interpreter.c -o python_interpreter $(PYTHON_LIB)

# ------------ ruby interface ----------------

mrscake.$(SO_RUBY): mrscake.rb.c mrscake.h $(OBJECTS)
	$(CC) $(RUBY_LDFLAGS) $(RUBY_CPPFLAGS) $(RUBY_INCLUDE) mrscake.rb.c $(OBJECTS) lib/libml.a -o $@ $(LIBS) $(RUBY_LIB) -lstdc++

# ------------ installation ----------------

install:
	$(INSTALL) mrscake.$(SO_RUBY) $(RUBY_INSTALLDIR)/mrscake.$(SO_RUBY)
	$(INSTALL) mrscake.$(SO_PYTHON) $(PYTHON_INSTALLDIR)/mrscake.$(SO_PYTHON)

# ------------ old test code -----------------

multimodel: multimodel.o lib/libml.a $(OBJECTS)
	$(CXX) multimodel.o $(OBJECTS) lib/libml.a -o $@ $(LIBS)

test: mrscake.so
	python test_python_module.py

local-clean:
	rm -f svm ast ann multimodel *.o mrscake.$(SO) predict.$(SO) prediction.$(SO)

clean: local-clean
	rm -f lib/*.o lib/*.a lib/*.gch
	rm -rf *.dSYM


.PHONY: clean all test local-clean
