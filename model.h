#ifndef __model_h__
#define __model_h__
#include "io.h"

typedef int32_t category_t;

/* input variable (a.k.a. "free" variable) */
typedef struct _variable {
    union {
	category_t cls;
	float value;
    };
    enum {CATEGORICAL,CONTINUOUS,MISSING} type;
} variable_t;

/* a single "row" in the data, combining a single known output with
   the corresponding inputs */
typedef struct _example {
    float*inputs;
    category_t desired_output;
} example_t;

typedef struct _model {
    uint32_t id;
    int num_inputs;

    int (*description_length)();
    
    category_t (*predict)(variable_t*inputs);
    
    void (*write)(writer_t*writer);

    void*internal;
} model_t;

typedef struct _model_trainer {
    model_t*(*train)(example_t*examples, int num_examples);
    void*internal;
} model_trainer_t;

#endif
