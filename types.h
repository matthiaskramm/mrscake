#ifndef __types_h__
#define __types_h__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t category_t;
typedef struct _value value_t;
typedef struct _array array_t;

#define TYPE_FLOAT 1
#define TYPE_CATEGORY 2
#define TYPE_INT 3
#define TYPE_BOOL 4
#define TYPE_MISSING 5
#define TYPE_ARRAY 6
extern char*type_name[];

struct _value {
    union {
        float f;
	category_t c;
	int i;
	bool b;
        array_t* a;
    };
    uint8_t type;
};

value_t bool_value(bool b);
value_t float_value(float f);
value_t missing_value();
value_t category_value(category_t c);
value_t int_value(int i);
value_t array_value(int size);

void value_print(value_t*v);
void value_clear(value_t*v);

struct _array {
    int size;
    value_t values[0];
};
array_t* array_new(int size);
void array_destroy(array_t*a);

int check_type(value_t v, uint8_t type);

#define AS_FLOAT(v) (check_type((v),TYPE_FLOAT),(v).f)
#define AS_INT(v) (check_type((v),TYPE_INT),(v).i)
#define AS_CATEGORY(v) (check_type((v),TYPE_CATEGORY),(v).c)
#define AS_BOOL(v) (check_type((v),TYPE_BOOL),(v).b)
#define AS_ARRAY(v) (check_type((v),TYPE_ARRAY),(v).a)

#ifdef __cplusplus
}
#endif

#endif
