/* constant.c
   Implementation of primitive datatypes.

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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include <string.h>
#include "constant.h"
#include "stringpool.h"

char*type_name[] = {"undefined","float","category","int","bool","missing","deprecated array","string",
                    "int_array", "float_array", "category_array", "string_array", "mixed_array"};


array_t* array_new(int size)
{
    array_t*array = (array_t*)calloc(1, sizeof(array_t)+sizeof(constant_t)*size);
    array->size = size;
    return array;
}
void array_fill(array_t*a, constant_t c)
{
    int t;
    for(t=0;t<a->size;t++) {
        a->entries[t] = c;
    }
}
array_t* array_create(int size, ...)
{
    va_list arglist;
    va_start(arglist, size);
    int t;
    array_t*array = array_new(size);
    for(t=0;t<size;t++) {
        array->entries[t] = category_constant(va_arg(arglist,category_t));
    }
    va_end(arglist);
    return array;
}

void array_destroy(array_t*a)
{
    free(a);
}

constant_type_t constant_array_subtype(constant_t*c)
{
    switch(c->type) {
        case CONSTANT_INT_ARRAY:
            return CONSTANT_INT;
        break;
        case CONSTANT_FLOAT_ARRAY:
            return CONSTANT_FLOAT;
        break;
        case CONSTANT_STRING_ARRAY:
            return CONSTANT_STRING;
        break;
        case CONSTANT_CATEGORY_ARRAY:
            return CONSTANT_CATEGORY;
        break;
        case CONSTANT_MIXED_ARRAY:
            return CONSTANT_MISSING;
        break;
        default:
            assert(!"can't determine array subtype of non-array");
    }
}

constant_t bool_constant(bool b)
{
    constant_t v;
    v.type = CONSTANT_BOOL;
    v.b = b;
    return v;
}
constant_t float_constant(float f)
{
    constant_t v;
    v.type = CONSTANT_FLOAT;
    v.f = f;
    return v;
}
constant_t missing_constant()
{
    constant_t v;
    v.type = CONSTANT_MISSING;
    return v;
}
constant_t category_constant(category_t c)
{
    constant_t v;
    v.type = CONSTANT_CATEGORY;
    v.c = c;
    return v;
}
constant_t int_constant(int i)
{
    constant_t v;
    v.type = CONSTANT_INT;
    v.i = i;
    return v;
}
bool array_is_homogeneous(array_t*a)
{
    int t;
    for(t=0;t<a->size;t++) {
        if(a->entries[0].type != a->entries[t].type)
            return false;
    }
    return true;
}
constant_t int_array_constant(array_t*a)
{
    constant_t v;
    v.type = CONSTANT_INT_ARRAY;
    v.a = a;
    return v;
}
constant_t float_array_constant(array_t*a)
{
    constant_t v;
    v.type = CONSTANT_FLOAT_ARRAY;
    v.a = a;
    return v;
}
constant_t string_array_constant(array_t*a)
{
    constant_t v;
    v.type = CONSTANT_STRING_ARRAY;
    v.a = a;
    return v;
}
constant_t category_array_constant(array_t*a)
{
    constant_t v;
    v.type = CONSTANT_CATEGORY_ARRAY;
    v.a = a;
    return v;
}
constant_t mixed_array_constant(array_t*a)
{
    constant_t v;
    v.type = CONSTANT_CATEGORY_ARRAY;
    v.a = a;
    return v;
}
constant_t string_constant(const char*s)
{
    constant_t v;
    v.type = CONSTANT_STRING;
    v.s = register_string(s);
    return v;
}
bool constant_equals(constant_t*c1, constant_t*c2)
{
    if(c1->type != c2->type)
        return false;
    switch(c1->type) {
        case CONSTANT_FLOAT:
            return c1->f == c2->f;
        case CONSTANT_INT:
            return c1->i == c2->i;
        case CONSTANT_CATEGORY:
            return c1->c == c2->c;
        case CONSTANT_BOOL:
            return c1->b == c2->b;
        case CONSTANT_STRING:
            return !strcmp(c1->s, c2->s);
        case CONSTANT_MISSING:
            return true;
        default:
            /* FIXME */
            //fprintf(stderr, "Can't compare types %s\n", type_name[c1->type]);
            return false;
    }
}
void constant_print(constant_t*v)
{
    int t;
    switch(v->type) {
        case CONSTANT_FLOAT:
            printf("%.2f", v->f);
        break;
        case CONSTANT_INT:
            printf("%d", v->i);
        break;
        case CONSTANT_CATEGORY:
            printf("C%d", v->c);
        break;
        case CONSTANT_BOOL:
            printf("%s", v->b?"true":"false");
        break;
        case CONSTANT_STRING:
            printf("\"%s\"", v->s);
        break;
        case CONSTANT_MISSING:
            printf("<missing>");
        break;
        case CONSTANT_MIXED_ARRAY:
        case CONSTANT_INT_ARRAY:
        case CONSTANT_FLOAT_ARRAY:
        case CONSTANT_CATEGORY_ARRAY:
        case CONSTANT_STRING_ARRAY:
            printf("[");
            array_t*a = AS_MIXED_ARRAY(*v);
            for(t=0;t<a->size;t++) {
                if(t>0)
                    printf(",");
                constant_print(&a->entries[t]);
            }
            printf("]");
        break;
        default:
            printf("<bad value>");
        break;
    }
}
void constant_clear(constant_t*v)
{
    switch(v->type) {
        case CONSTANT_INT_ARRAY:
        case CONSTANT_FLOAT_ARRAY:
        case CONSTANT_CATEGORY_ARRAY:
        case CONSTANT_MIXED_ARRAY:
        case CONSTANT_STRING_ARRAY:
            free(v->a);
            v->a = 0;
        break;
        case CONSTANT_STRING:
            /* FIXME */
            //free(v->s);
            v->s = 0;
        break;
    }
}

int constant_check_type(constant_t v, uint8_t type)
{
    if(type == CONSTANT_MIXED_ARRAY) {
        switch(v.type) {
            case CONSTANT_INT_ARRAY:
            case CONSTANT_FLOAT_ARRAY:
            case CONSTANT_CATEGORY_ARRAY:
            case CONSTANT_MIXED_ARRAY:
            case CONSTANT_STRING_ARRAY:
                return 0;
            default:
                fprintf(stderr, "expected array, got %s\n", type_name[v.type]);
        }
    }
    if(v.type!=type) {
        fprintf(stderr, "expected %s, got %s\n", type_name[type], type_name[v.type]);
    }
    assert(v.type == type);
    return 0;
}

constant_t variable_to_constant(variable_t*v)
{
    switch(v->type) {
        case CATEGORICAL:
            return category_constant(v->category);
        case CONTINUOUS:
            return float_constant(v->value);
        case TEXT:
            return string_constant(v->text);
        default:
            fprintf(stderr, "invalid variable type %d\n", v->type);
            assert(0);
    }
}

variable_t constant_to_variable(constant_t* c)
{
    switch(c->type) {
        case CONSTANT_STRING:
            return variable_make_text(c->s);
        break;
        case CONSTANT_CATEGORY:
            return variable_make_categorical(c->c);
        break;
        case CONSTANT_FLOAT:
            return variable_make_continuous(c->f);
        break;
        case CONSTANT_MISSING:
            return variable_make_missing();
        break;
        default:
            fprintf(stderr, "Can't convert constant type %d to variable\n", c->type);
        break;
    }
}



