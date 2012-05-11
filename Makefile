IS_MACOS:=$(shell test -d /Library && echo macos)

ifneq ($(IS_MACOS),) # Mac compile
    CPPFLAGS=-DHAVE_SHA1
    LIBS=-lz -lpthread -lcrypto
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

ifeq ($(IS_MACOS),) # Linux compile
    CPPFLAGS=-DHAVE_SHA1
    LIBS=-lz -lpthread -lcrypto -lrt
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

A=a
O=o
CC=gcc $(CPPFLAGS) $(CFLAGS) -pg -g -fPIC -Wimplicit
CXX=g++ $(CPPFLAGS) $(CXXFLAGS) -pg -g -fPIC -Wimplicit
INSTALL=/usr/bin/install -c

MODELS=model_cv_dtree.$(O) model_cv_ann.$(O) model_cv_svm.$(O) model_cv_linear.$(O) model_perceptron.$(O) model_knearest.$(O)
VAR_SELECTORS=varselect_cv_dtree.$(O)
CODE_GENERATORS=codegen_python.$(O) codegen_ruby.$(O) codegen_js.$(O) codegen_c.$(O)
OBJECTS=$(MODELS) $(VAR_SELECTORS) $(CODE_GENERATORS) cvtools.$(O) constant.$(O) ast.$(O) model.$(O) serialize.$(O) io.$(O) list.$(O) model_select.$(O) dict.$(O) dataset.$(O) datacache.$(O) text.$(O) environment.$(O) codegen.$(O) ast_transforms.$(O) stringpool.$(O) net.$(O) settings.$(O) job.$(O) var_selection.$(O) transform.$(O) mrscake.$(O) util.$(O)
CV_OBJECTS=lib/alloc.$(O) lib/ann_mlp.$(O) lib/arithm.$(O) lib/array.$(O) lib/boost.$(O) lib/cnn.$(O) lib/convert.$(O) lib/copy.$(O) lib/data.$(O) \
	lib/datastructs.$(O) lib/ertrees.$(O) lib/estimate.$(O) lib/gbt.$(O) lib/inner_functions.$(O) lib/knearest.$(O) lib/mathfuncs.$(O) lib/matmul.$(O) \
	lib/matrix.$(O) lib/missing.$(O) lib/persistence.$(O) lib/precomp.$(O) lib/rand.$(O) lib/rtrees.$(O) lib/stat.$(O) lib/svm.$(O) lib/system.$(O) lib/tables.$(O) \
	lib/testset.$(O) lib/tree.$(O)

all: mrscake-job-server mrscake.$(A) mrscake.$(SO_PYTHON) mrscake.$(SO_RUBY)

lib/libml.a: lib/*.cpp lib/*.hpp lib/*.h
	cd lib;make libml.a

model.$(O): model.c constant.h ast.h
	$(CC) -c $< -o $@

model_select.$(O): model_select.c constant.h ast.h
	$(CC) -c $< -o $@

ast.$(O): ast.c ast.h mrscake.h
	$(CC) -c $< -o $@

mrscake.$(O): mrscake.c mrscake.h mrscake.h
	$(CC) -c $< -o $@

net.$(O): net.c net.h mrscake.h settings.h
	$(CC) -c $< -o $@

job.$(O): job.c job.h settings.h
	$(CC) -c $< -o $@

ast_transforms.$(O): ast_transforms.c ast_transforms.h ast.h
	$(CC) -c $< -o $@

constant.$(O): constant.c constant.h mrscake.h
	$(CC) -c $< -o $@

environment.$(O): environment.c environment.h mrscake.h
	$(CC) -c $< -o $@

dataset.$(O): dataset.c dataset.h mrscake.h
	$(CC) -c $< -o $@

text.$(O): text.c text.h dataset.h mrscake.h
	$(CC) -c $< -o $@

transform.$(O): transform.c transform.h mrscake.h
	$(CC) -c $< -o $@

dict.$(O): dict.c dict.h mrscake.h
	$(CC) -c $< -o $@

list.$(O): list.c list.h
	$(CC) -c $< -o $@

serialize.$(O): serialize.c serialize.h ast.h constant.h
	$(CC) -c $< -o $@

stringpool.$(O): stringpool.c stringpool.h
	$(CC) -c $< -o $@

settings.$(O): settings.c settings.h
	$(CC) -c $< -o $@

var_selection.$(O): var_selection.c var_selection.h
	$(CC) -c $< -o $@

codegen.$(O): codegen.c codegen.h
	$(CC) -c $< -o $@

codegen_python.$(O): codegen_python.c codegen.h
	$(CC) -c $< -o $@

test_server.$(O): test_server.c
	$(CC) -c $< -o $@

io.$(O): io.c io.h
	$(CC) -c $< -o $@

cvtools.$(O): cvtools.cpp lib/ml.hpp dataset.h
	$(CXX) -Ilib $< -c -o $@

model_cv_dtree.$(O): model_cv_dtree.cpp mrscake.h ast.h cvtools.h dataset.h easy_ast.h
	$(CXX) -Ilib $< -c -o $@

model_cv_svm.$(O): model_cv_svm.cpp mrscake.h ast.h cvtools.h dataset.h easy_ast.h
	$(CXX) -Ilib $< -c -o $@

model_cv_ann.$(O): model_cv_ann.cpp mrscake.h ast.h cvtools.h dataset.h easy_ast.h
	$(CXX) -Ilib $< -c -o $@

model_cv_linear.$(O): model_cv_linear.cpp mrscake.h ast.h cvtools.h dataset.h easy_ast.h
	$(CXX) -Ilib $< -c -o $@

model_perceptron.$(O): model_perceptron.c mrscake.h ast.h cvtools.h dataset.h easy_ast.h
	$(CC) -Ilib $< -c -o $@

model_knearest.$(O): model_knearest.c mrscake.h ast.h cvtools.h dataset.h easy_ast.h
	$(CC) -Ilib $< -c -o $@

varselect_cv_dtree.$(O): varselect_cv_dtree.cpp mrscake.h cvtools.h dataset.h var_selection.h
	$(CXX) -Ilib $< -c -o $@

mrscake-job-server: server.$(O) $(OBJECTS) lib/libml.a
	$(CXX) server.$(O) $(OBJECTS) lib/libml.a -o $@ $(LIBS)

# ------------ static library ----------------

mrscake.$(A): $(OBJECTS) $(CV_OBJECTS)
	$(AR) cru $@ $(OBJECTS) $(CV_OBJECTS)

# ------------ python interface --------------

mrscake.$(SO_PYTHON): mrscake.py.c mrscake.h list.h $(OBJECTS) lib/libml.a
	$(CC) $(PYTHON_INCLUDE) -shared mrscake.py.c $(OBJECTS) lib/libml.a -o $@ $(LIBS) $(PYTHON_LIB) -lstdc++

python_interpreter: python_interpreter.c
	$(CC) $(PYTHON_INCLUDE) python_interpreter.c -o python_interpreter $(PYTHON_LIB)

# ------------ ruby interface ----------------

mrscake.$(SO_RUBY): mrscake.rb.c mrscake.h $(OBJECTS)
	$(CC) $(RUBY_LDFLAGS) $(RUBY_CPPFLAGS) $(RUBY_INCLUDE) mrscake.rb.c $(OBJECTS) lib/libml.a -o $@ $(LIBS) $(RUBY_LIB) -lstdc++

# ------------ installation ------------------

install:
	$(INSTALL) mrscake.$(SO_RUBY) $(RUBY_INSTALLDIR)/mrscake.$(SO_RUBY)
	$(INSTALL) mrscake.$(SO_PYTHON) $(PYTHON_INSTALLDIR)/mrscake.$(SO_PYTHON)

# ------------ old test code -----------------

test: mrscake.so
	python test_python_module.py

local-clean:
	rm -f *.o *.obj *.$(O) mrscake.$(SO) predict.$(SO) prediction.$(SO)

clean: local-clean
	rm -f lib/*.$(O) lib/*.o lib/*.obj lib/*.a lib/*.gch
	rm -rf *.dSYM


.PHONY: clean all test local-clean
