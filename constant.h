/* constant.h
   Primitive datatypes.

   Part of the data prediction package.
   
   Copyright (c) 2010-2011 Matthias Kramm <kramm@quiss.org> 
 
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#ifndef __constant_h__
#define __constant_h__

#include <stdint.h>
#include <stdbool.h>
#include "mrscake.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _constant constant_t;
typedef struct _array array_t;

typedef enum constant_type {
    CONSTANT_FLOAT=1,
    CONSTANT_CATEGORY=2,
    CONSTANT_INT=3,
    CONSTANT_BOOL=4,
    CONSTANT_MISSING=5,
    CONSTANT_STRING=7,
    CONSTANT_INT_ARRAY=8,
    CONSTANT_FLOAT_ARRAY=9,
    CONSTANT_CATEGORY_ARRAY=10,
    CONSTANT_STRING_ARRAY=11,
    CONSTANT_MIXED_ARRAY=12,
} constant_type_t;

extern char*type_name[];

struct _constant {
    union {
        float f;
	category_t c;
	int i;
	bool b;
        array_t* a;
        const char* s;
    };
    uint8_t type;
};

constant_t missing_constant();
constant_t bool_constant(bool b);
constant_t float_constant(float f);
constant_t category_constant(category_t c);
constant_t int_constant(int i);
constant_t int_array_constant(array_t*a);
constant_t float_array_constant(array_t*a);
constant_t string_array_constant(array_t*a);
constant_t mixed_array_constant(array_t*a);
constant_t category_array_constant(array_t*a);
constant_t string_constant(const char*s);

bool constant_equals(constant_t*c1, constant_t*c2);
void constant_print(constant_t*v);
void constant_clear(constant_t*v);

struct _array {
    int size;
    constant_t entries[0];
};
array_t* array_new(int size);
array_t* array_create(int size, ...);
void array_fill(array_t*a, constant_t c);
void array_destroy(array_t*a);
constant_type_t constant_array_subtype(constant_t*c);

int constant_check_type(constant_t v, uint8_t type);
constant_t variable_to_constant(variable_t*v);
variable_t constant_to_variable(constant_t* c);

#define CONSTANT_CHECK_TYPE(c,t) (assert(constant_check_type((c),(t))))

#define AS_FLOAT(v) (CONSTANT_CHECK_TYPE((v),CONSTANT_FLOAT),(v).f)
#define AS_INT(v) (CONSTANT_CHECK_TYPE((v),CONSTANT_INT),(v).i)
#define AS_CATEGORY(v) (CONSTANT_CHECK_TYPE((v),CONSTANT_CATEGORY),(v).c)
#define AS_BOOL(v) (CONSTANT_CHECK_TYPE((v),CONSTANT_BOOL),(v).b)
#define AS_INT_ARRAY(v) (CONSTANT_CHECK_TYPE((v),CONSTANT_INT_ARRAY),(v).a)
#define AS_CATEGORY_ARRAY(v) (CONSTANT_CHECK_TYPE((v),CONSTANT_CATEGORY_ARRAY),(v).a)
#define AS_STRING_ARRAY(v) (CONSTANT_CHECK_TYPE((v),CONSTANT_STRING_ARRAY),(v).a)
#define AS_FLOAT_ARRAY(v) (CONSTANT_CHECK_TYPE((v),CONSTANT_FLOAT_ARRAY),(v).a)
#define AS_MIXED_ARRAY(v) (CONSTANT_CHECK_TYPE((v),CONSTANT_MIXED_ARRAY),(v).a)
#define AS_ARRAY(v) (CONSTANT_CHECK_TYPE((v),CONSTANT_MIXED_ARRAY),(v).a)
#define AS_STRING(v) (CONSTANT_CHECK_TYPE((v),CONSTANT_STRING),(v).s)

#ifdef __cplusplus
}
#endif

#endif
