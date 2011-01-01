#ifndef __model_h__
#define __model_h__
#include <stdint.h>
#include "types.h"
//#include "io.h"

typedef enum {CATEGORICAL,CONTINUOUS,MISSING} columntype_t;

/* input variable (a.k.a. "free" variable) */
typedef struct _variable {
    columntype_t type;
    union {
	category_t category;
	float value;
    };
} variable_t;

variable_t variable_make_categorical(category_t c);
variable_t variable_make_continuous(float v);
variable_t variable_make_missing();

typedef struct _row {
    int num_inputs;
    variable_t inputs[0];
} row_t;

row_t*row_new(int num_inputs);
void row_destroy(row_t*row);

/* input variable with column info (for sparse rows) */
typedef struct _variable_and_position {
    int index;
    columntype_t type;
    union {
	category_t category;
	float value;
    };
} variable_and_position_t;

typedef struct _sparse_row {
    variable_and_position_t*inputs;
    int num_inputs;
} sparse_row_t;

/* a single "row" in the data, combining a single known output with
   the corresponding inputs */
typedef struct _example {
    int num_inputs;
    category_t desired_output;
    variable_t inputs[0];
} example_t;

example_t*example_new(int num_inputs);

typedef struct _model {
    int num_inputs;

    void* (*get_code)();
    category_t (*predict)(row_t row);

    void*internal;
} model_t;

typedef struct _model_factory {
    const char*name;
    model_t*(*train)(example_t**examples, int num_examples);
    void*internal;
} model_factory_t;

extern model_factory_t dtree_model_factory;

#endif
