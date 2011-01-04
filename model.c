#include <stdlib.h>
#include <memory.h>
#include "model.h"
#include "ast.h"
#include "io.h"

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
variable_t variable_make_text(char*s)
{
    variable_t v;
    v.type = TEXT;
    v.text = s;
    return v;
}
variable_t variable_make_missing()
{
    variable_t v;
    v.type = MISSING;
    v.value = __builtin_nan("");
    return v;
}
double variable_value(variable_t*v)
{
    return (v->type == CATEGORICAL) ? v->category : v->value;
}

example_t*example_new(int num_inputs)
{
    example_t*r = (example_t*)malloc(sizeof(example_t)+sizeof(variable_t)*num_inputs);
    r->num_inputs = num_inputs;
    return r;
}
row_t*example_to_row(example_t*e)
{
    row_t*r = row_new(e->num_inputs);
    memcpy(r->inputs, e->inputs, sizeof(variable_t)*e->num_inputs);
    return r;
}
void examples_check_format(example_t**e, int num_examples)
{
    int t;
    for(t=0;t<num_examples;t++) {
        if(e[t]->num_inputs != e[0]->num_inputs) {
            fprintf(stderr, "Bad configuration: row %d has %d inputs, row %d has %d.\n", t, e[t]->num_inputs, 0, e[0]->num_inputs);
        }
        int s;
        for(s=0;s<e[t]->num_inputs;s++) {
            if(e[t]->inputs[s].type != e[0]->inputs[s].type)
                fprintf(stderr, "Bad configuration: item %d,%d is %d, item %d,%d is %d\n",
                        t, s, e[t]->inputs[s].type,
                        0, s, e[0]->inputs[s].type
                        );
        }
    }
}
void examples_print(example_t**e, int num_examples)
{
    int t;
    for(t=0;t<num_examples;t++) {
        int s;
        for(s=0;s<e[t]->num_inputs;s++) {
            variable_t v = e[t]->inputs[s];
            if(v.type == CATEGORICAL) {
                printf("C%d\t", v.category);
            } else if(v.type == CONTINUOUS) {
                printf("%.2f\t", v.value);
            } else if(v.type == TEXT) {
                printf("\"%s\"\t", v.value);
            }
        }
        printf("\n");
    }
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

category_t model_predict(model_t*m, row_t*row)
{
    environment_t e;
    e.row = row;
    node_t*code = (node_t*)m->code;
    constant_t c = node_eval(code, &e);
    return AS_CATEGORY(c);
}
void model_destroy(model_t*m)
{
    if(m->code)
        node_destroy(m->code);
    free(m);
}
