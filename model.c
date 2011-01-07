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
variable_t variable_make_text(const char*s)
{
    variable_t v;
    v.type = TEXT;
    v.text = strdup(s);
    return v;
}
variable_t variable_make_missing()
{
    variable_t v;
    v.type = MISSING;
    v.value = __builtin_nan("");
    return v;
}
void variable_print(variable_t*v)
{
    switch(v->type) {
        case CATEGORICAL:
            printf("C%d\n", v->category);
        break;
        case CONTINUOUS:
            printf("%.2f\n", v->value);
        break;
        case TEXT:
            printf("\"%s\"\n", v->text);
        break;
        default:
            printf("INVALID TYPE %d\n", v->type);
        break;
    }
}
bool variable_equals(variable_t*v1, variable_t*v2)
{
    if(v1->type != v2->type)
        return false;

    switch(v1->type) {
        case CATEGORICAL:
            return v1->category==v2->category;
        case CONTINUOUS:
            return v1->value==v2->value;
        case TEXT:
            return !strcmp(v1->text, v2->text);
        default:
            return false;
    }
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
void row_print(row_t*row)
{
    int t;
    for(t=0;t<row->num_inputs;t++) {
        printf("%d) ", t);
        variable_print(&row->inputs[t]);
    }
}
void row_destroy(row_t*row)
{
    free(row);
}

void model_print(model_t*m)
{
    node_print((node_t*)m->code);
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
        default:
            fprintf(stderr, "Can't convert constant type %d to variable\n", c->type);
        break;
    }
}

variable_t model_predict(model_t*m, row_t*row)
{
    environment_t e;
    e.row = row;
    if(m->wordmap) {
        wordmap_convert_row(m->wordmap, row);
    }
    node_t*code = (node_t*)m->code;
    constant_t c = node_eval(code, &e);
    return constant_to_variable(&c);
}
void model_destroy(model_t*m)
{
    if(m->code)
        node_destroy(m->code);
    free(m);
}
