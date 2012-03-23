/* mrscake.h
   Data prediction top level API.

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

#ifndef __mrscake_h__
#define __mrscake_h__
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t category_t;
typedef enum {CATEGORICAL,CONTINUOUS,TEXT,MISSING} columntype_t;

/* input variable (a.k.a. "free" variable) */
typedef struct _variable {
    columntype_t type;
    union {
        category_t category;
        float value;
        const char*text;
    };
} variable_t;

variable_t variable_new_categorical(category_t c);
variable_t variable_new_continuous(float v);
variable_t variable_new_text(const char*s);
variable_t variable_new_missing();
double variable_value(variable_t*v);
const char*variable_type(variable_t*v);
bool variable_equals(variable_t*v1, variable_t*v2);
void variable_print(variable_t*v, FILE*stream);

typedef struct _row {
    int num_inputs;
    variable_t inputs[0];
} row_t;

row_t*row_new(int num_inputs);
void row_destroy(row_t*row);

/* a single "row" in the data, combining a single known output with
   the corresponding inputs */
typedef struct _example {
    int num_inputs;
    const char**input_names;
    struct _example*prev;
    struct _example*next;
    variable_t desired_response;
    variable_t inputs[0];
} example_t;

example_t*example_new(int num_inputs);
row_t*example_to_row(example_t*e, const char**column_names);
void example_destroy(example_t*example);

typedef struct _trainingdata {
    example_t*first_example;
    example_t*last_example;
    int num_examples;
} trainingdata_t;

trainingdata_t* trainingdata_new();
void trainingdata_add_example(trainingdata_t*d, example_t*e);
bool trainingdata_check_format(trainingdata_t*trainingdata);
void trainingdata_print(trainingdata_t*dataset);
void trainingdata_destroy(trainingdata_t*dataset);
void trainingdata_save(trainingdata_t*d, const char*filename);
trainingdata_t* trainingdata_load(const char*filename);

typedef struct _signature {
    int num_inputs;
    columntype_t*column_types;
    const char**column_names;
    bool has_column_names;
} signature_t;

typedef struct _model {
    const char*name;
    signature_t*sig;
    void*code;
} model_t;

variable_t model_predict(model_t*m, row_t*row);
model_t* model_load(const char*filename);
void model_save(model_t*m, const char*filename);
void model_print(model_t*m);
void model_destroy(model_t*m);
char*model_generate_code(model_t*m, const char*language);

model_t* trainingdata_train(trainingdata_t*dataset);
model_t* trainingdata_train_specific_model(trainingdata_t*trainingdata, const char*name);

#ifdef __cplusplus
}
#endif

#endif
