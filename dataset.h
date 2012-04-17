/* dataset.h
   Conversion between representations of training data (header file).

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

#ifndef __dataset_h__
#define __dataset_h__

#include <stdbool.h>
#include "mrscake.h"
#include "ast.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _column;
struct _transformation;
typedef struct _column column_t;
typedef struct _transform transform_t;

typedef struct _dataset {
    signature_t*sig;

    int num_columns;
    int num_rows;

    column_t**columns;
    column_t*desired_response;

    /* some datasets are transformed versions of the original. This
       field specifies the untransformed original and how to modify code 
       to target said untransformed version */
    transform_t* transform;
} dataset_t;


struct _column {
    const char*name;
    columntype_t type;

    int num_classes;
    constant_t*classes;
    int* class_occurence_count;

    union {
        float f;
        category_t c;
        const char* text;
    } entries[0];
};

bool trainingdata_check_format(trainingdata_t*trainingdata);
dataset_t* trainingdata_sanitize(trainingdata_t*dataset);
void dataset_print(dataset_t*s);
constant_t dataset_map_response_class(dataset_t*dataset, int i);
void dataset_destroy(dataset_t*dataset);
int dataset_count_expanded_columns(dataset_t*s);
bool dataset_has_categorical_columns(dataset_t*data);

typedef node_t* (transform_reverse_function_t)(dataset_t*, node_t* code);
struct _transform
{
    char*name;
    /* make code target the original dataset */
    node_t* (*revert_in_code)(dataset_t*, node_t* code);
    void (*destroy)(dataset_t*);
    dataset_t*original;
};

column_t*column_new(int num_rows, columntype_t columntype);

model_t* model_new(dataset_t*dataset);
example_t**example_list_to_array(trainingdata_t*d, int*_num_examples, int flags);
node_t* parameter_code(dataset_t*d, int num);
array_t* dataset_classes_as_array(dataset_t*d);
void dataset_fill_row(dataset_t*s, row_t*row, int y);

#ifdef __cplusplus
}
#endif
#endif
