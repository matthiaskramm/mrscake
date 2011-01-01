#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "types.h"

#define TYPE_FLOAT 1
#define TYPE_CATEGORY 2
#define TYPE_INT 3
#define TYPE_BOOL 4
#define TYPE_MISSING 5
#define TYPE_ARRAY 6
char*type_name[] = {"undefined","float","category","int","bool","missing","array"};

array_t* array_new(int size)
{
    array_t*array = (array_t*)calloc(1, sizeof(array_t)+sizeof(value_t)*size);
    array->size = size;
    return array;
}
void array_destroy(array_t*a)
{
    free(a);
}

value_t bool_value(bool b)
{
    value_t v;
    v.type = TYPE_BOOL;
    v.b = b;
    return v;
}
value_t float_value(float f)
{
    value_t v;
    v.type = TYPE_FLOAT;
    v.f = f;
    return v;
}
value_t missing_value()
{
    value_t v;
    v.type = TYPE_MISSING;
    return v;
}
value_t category_value(category_t c)
{
    value_t v;
    v.type = TYPE_CATEGORY;
    v.c = c;
    return v;
}
value_t int_value(int i)
{
    value_t v;
    v.type = TYPE_INT;
    v.i = i;
    return v;
}
value_t array_value(int size)
{
    value_t v;
    v.type = TYPE_ARRAY;
    v.a = array_new(size);
    return v;
}
void value_print(value_t*v)
{
    int t;
    switch(v->type) {
        case TYPE_FLOAT:
            printf("FLOAT(%f)", v->f);
        break;
        case TYPE_INT:
            printf("INT(%d)", v->i);
        break;
        case TYPE_CATEGORY:
            printf("CATEGORY(%d)", v->c);
        break;
        case TYPE_BOOL:
            printf("BOOL(%s)", v->b?"true":"false");
        break;
        case TYPE_MISSING:
            printf("BOOL(%s)", v->b?"true":"false");
        break;
        case TYPE_ARRAY:
            printf("ARRAY[");
            for(t=0;t<v->a->size;t++) {
                if(t>0)
                    printf(",");
                value_print(&v->a->values[t]);
            }
            printf("]");
        break;
        default:
            printf("BAD VALUE");
        break;
    }
}
void value_clear(value_t*v)
{
    switch(v->type) {
        case TYPE_ARRAY:
            free(v->a);
            v->a = 0;
        break;
    }
}

int check_type(value_t v, uint8_t type)
{
    if(v.type!=type)
        fprintf(stderr, "expected %s, got %s\n", type_name[type], type_name[v.type]);
    assert(v.type == type);
    return 0;
}

