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

/* a dictionary maps identifiers (textual categories) to numerical category ids */
typedef struct _dictionary {
    void*internal;
} dictionary_t;

dictionary_t* dictionary_new();
category_t dictionary_find_word(const char*word);
category_t dictionary_find_or_add_word(const char*word);
const char* dictionary_find_category(category_t c);
void dictionary_destroy(dictionary_t*dict);

/* a single "row" in the data, combining a single known output with
   the corresponding inputs */
typedef struct _example {
    int num_inputs;
    struct _example*previous;
    category_t desired_output; // TODO: make this variable_t
    variable_t inputs[0];
} example_t;

example_t*example_new(int num_inputs);
row_t*example_to_row(example_t*e);

typedef struct _dataset {
    example_t*examples;
    int num_examples;
} dataset_t;

dataset_t* dataset_new();
void dataset_add(dataset_t*d, example_t*e);
void dataset_check_format(dataset_t*dataset);
void dataset_print(dataset_t*dataset);
void dataset_destroy(dataset_t*dataset);

typedef struct _model {
    int num_inputs;
    dictionary_t*word2category;
    columntype_t*column_types;
    void*code;
} model_t;

model_t* dataset_get_model(dataset_t*dataset);

category_t model_predict(model_t*m, row_t*row);
model_t* model_load(const char*filename);
void model_save(model_t*m, const char*filename);
void model_print(model_t*m);
void model_destroy(model_t*m);

typedef struct _model_factory {
    const char*name;
    model_t*(*train)(dataset_t*dataset);
    void*internal;
} model_factory_t;

extern model_factory_t dtree_model_factory;
model_t* model_select(dataset_t*dataset);

#ifdef __cplusplus
}
#endif

#endif
