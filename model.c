#include <stdlib.h>
#include "model.h"
#include "ast.h"

variable_t variable_make_categorical(category_t c)
{
    variable_t v;
    v.type = CATEGORICAL;
    v.category = c;
    return v;
}

variable_t variable_make_continuous(float f)
{
    variable_t v;
    v.type = CONTINUOUS;
    v.value = f;
    return v;
}

variable_t variable_make_missing()
{
    variable_t v;
    v.type = MISSING;
    v.value = __builtin_nan("");
    return v;
}

example_t*example_new(int num_inputs)
{
    example_t*r = (example_t*)malloc(sizeof(example_t)+sizeof(variable_t)*num_inputs);
    r->num_inputs = num_inputs;
    return r;
}
void example_destroy(example_t*example)
{
    free(example);
}
row_t*row_new(int num_inputs)
{
    row_t*r = (row_t*)malloc(sizeof(row_t)+sizeof(variable_t)*num_inputs);
    r->num_inputs = num_inputs;
    return r;
}
void row_destroy(row_t*row)
{
    free(row);
}

void model_destroy(model_t*m)
{
    if(m->code)
        node_destroy(m->code);
    free(m);
}
