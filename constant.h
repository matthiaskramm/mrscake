#ifndef __constant_h__
#define __constant_h__

#include <stdint.h>
#include <stdbool.h>
#include "model.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _constant constant_t;
typedef struct _array array_t;

#define CONSTANT_FLOAT 1
#define CONSTANT_CATEGORY 2
#define CONSTANT_INT 3
#define CONSTANT_BOOL 4
#define CONSTANT_MISSING 5
#define CONSTANT_ARRAY 6
extern char*type_name[];

struct _constant {
    union {
        float f;
	category_t c;
	int i;
	bool b;
        array_t* a;
    };
    uint8_t type;
};

constant_t bool_constant(bool b);
constant_t float_constant(float f);
constant_t missing_constant();
constant_t category_constant(category_t c);
constant_t int_constant(int i);
constant_t array_constant(array_t*a);

void constant_print(constant_t*v);
void constant_clear(constant_t*v);

struct _array {
    int size;
    constant_t entries[0];
};
array_t* array_new(int size);
array_t* array_create(int size, ...);
void array_destroy(array_t*a);

int constant_check_type(constant_t v, uint8_t type);

#define AS_FLOAT(v) (constant_check_type((v),CONSTANT_FLOAT),(v).f)
#define AS_INT(v) (constant_check_type((v),CONSTANT_INT),(v).i)
#define AS_CATEGORY(v) (constant_check_type((v),CONSTANT_CATEGORY),(v).c)
#define AS_BOOL(v) (constant_check_type((v),CONSTANT_BOOL),(v).b)
#define AS_ARRAY(v) (constant_check_type((v),CONSTANT_ARRAY),(v).a)

#ifdef __cplusplus
}
#endif

#endif
