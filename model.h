/* model.h
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

#ifndef __model_h__
#define __model_h__
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
        char*text;
    };
} variable_t;

variable_t variable_make_categorical(category_t c);
variable_t variable_make_continuous(float v);
variable_t variable_make_text(const char*s);
variable_t variable_make_missing();
double variable_value(variable_t*v);
bool variable_equals(variable_t*v1, variable_t*v2);

typedef struct _row {
    int num_inputs;
    variable_t inputs[0];
} row_t;

row_t*row_new(int num_inputs);
void row_destroy(row_t*row);

/* a dictionary maps identifiers (textual categories) to numerical category ids */
typedef struct _wordmap {
    void*internal;
} wordmap_t;

wordmap_t* wordmap_new();
category_t wordmap_find_word(wordmap_t*dict, const char*word);
category_t wordmap_find_or_add_word(wordmap_t*dict, const char*word);
const char* wordmap_find_category(wordmap_t*dict, category_t c);
void wordmap_destroy(wordmap_t*dict);

/* a single "row" in the data, combining a single known output with
   the corresponding inputs */
typedef struct _example {
    int num_inputs;
    struct _example*previous;
    variable_t desired_response;
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
void dataset_print(dataset_t*dataset);
void dataset_destroy(dataset_t*dataset);

typedef struct _model {
    int num_inputs;
    wordmap_t*wordmap;
    columntype_t*column_types;
    void*code;
} model_t;

model_t* dataset_get_model(dataset_t*dataset);

variable_t model_predict(model_t*m, row_t*row);
model_t* model_load(const char*filename);
void model_save(model_t*m, const char*filename);
void model_print(model_t*m);
void model_destroy(model_t*m);

model_t* model_select(dataset_t*dataset);

#ifdef __cplusplus
}
#endif

#endif
