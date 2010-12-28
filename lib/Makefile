all: libml.a

CXX=g++ -I. -O -g

HEADERS=precomp.hpp.gch precomp_core.hpp.gch

OBJECTS=alloc.o ann_mlp.o arithm.o array.o boost.o cnn.o convert.o copy.o data.o \
	datastructs.o ertrees.o estimate.o gbt.o inner_functions.o knearest.o mathfuncs.o matmul.o \
	matrix.o missing.o persistence.o precomp.o rand.o rtrees.o stat.o svm.o system.o tables.o \
	testset.o tree.o

precomp.hpp.gch: precomp.hpp
	$(CXX) -c $< -o $@

precomp_core.hpp.gch: precomp_core.hpp
	$(CXX) -c $< -o $@

%.o: %.cpp
	$(CXX) -c $< -o $@

libml.a: $(HEADERS) $(OBJECTS)
	ar cru libml.a $(OBJECTS)

clean:
	rm -f *.gch $(OBJECTS) libml.a
