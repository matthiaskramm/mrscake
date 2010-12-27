all: libml.a

OBJECTS=alloc.o ann_mlp.o arithm.o array.o boost.o cnn.o convert.o copy.o data.o \
	datastructs.o ertrees.o estimate.o gbt.o inner_functions.o knearest.o mathfuncs.o matmul.o \
	matrix.o missing.o persistence.o precomp.o rand.o rtrees.o stat.o svm.o system.o tables.o \
	testset.o tree.o

%.o: %.cpp
	g++ -c -I. $< -o $@ -lz -lpthread -lrt

libml.a: $(OBJECTS)
	ar cru libml.a $(OBJECTS)

clean:
	rm -f $(OBJECTS) libml.a
