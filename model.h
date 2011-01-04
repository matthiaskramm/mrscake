#ifndef __model_h__
#define __model_h__
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t category_t;
typedef enum {CATEGORICAL,CONTINUOUS,TEXT,MISSING} columntype_t;

/* input variable (a.k.a. "free" variable) */
typedef struct _variable {
    columntype_t type;
    union {
	category_t category;
	float value;
        char*text;
    };
} variable_t;

variable_t variable_make_categorical(category_t c);
variable_t variable_make_continuous(float v);
variable_t variable_make_text(char*s);
variable_t variable_make_missing();
double variable_value(variable_t*v);

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
row_t*example_to_row(example_t*e);

void examples_check_format(example_t**e, int num_examples);
void examples_print(example_t**e, int num_examples);

typedef struct _model {
    int num_inputs;
    columntype_t*column_types;
    void*code;
} model_t;

category_t model_predict(model_t*m, row_t*row);
model_t* model_load(const char*filename);
void model_save(model_t*m, const char*filename);
void model_print(model_t*m);
void model_destroy(model_t*m);

typedef struct _model_factory {
    const char*name;
    model_t*(*train)(example_t**examples, int num_examples);
    void*internal;
} model_factory_t;

extern model_factory_t dtree_model_factory;

model_t* model_select(example_t**examples, int num_examples);

#ifdef __cplusplus
}
#endif

#endif
