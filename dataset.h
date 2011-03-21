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
typedef struct _column column_t;

typedef struct _sanitized_dataset {
    int num_columns;
    int num_rows;
    column_t**columns;

    column_t*desired_response;
} sanitized_dataset_t;

struct _column {
    int index;

    bool is_categorical;

    int num_classes;
    constant_t*classes;
    int* class_occurence_count;

    const char*name;

    union {
        float f;
        category_t c;
    } entries[0];
};

sanitized_dataset_t* dataset_sanitize(trainingdata_t*dataset);
void sanitized_dataset_print(sanitized_dataset_t*s);
constant_t sanitized_dataset_map_response_class(sanitized_dataset_t*dataset, int i);
void sanitized_dataset_destroy(sanitized_dataset_t*dataset);
int sanitized_dataset_count_expanded_columns(sanitized_dataset_t*s);
sanitized_dataset_t* sanitized_dataset_pick_columns(sanitized_dataset_t*data, int*index, int num);

/* structure for storing "exploded" version of columns where every class
   has its own column */
typedef struct _expanded_columns {
    sanitized_dataset_t*dataset;
    int num;
    bool use_header_code;
    struct {
        int source_column;
        int source_class;
    }* columns;
} expanded_columns_t;

expanded_columns_t* expanded_columns_new(sanitized_dataset_t*s);
node_t* expanded_columns_parameter_init(expanded_columns_t*e);
node_t* expanded_columns_parameter_code(expanded_columns_t*e, int num);
void expanded_columns_destroy(expanded_columns_t*e);

column_t*column_new(int num_rows, bool is_categorical, int x);

model_t* model_new(sanitized_dataset_t*dataset);
example_t**example_list_to_array(trainingdata_t*d, int*_num_examples, int flags);
node_t* parameter_code(sanitized_dataset_t*d, int num);
array_t* sanitized_dataset_classes_as_array(sanitized_dataset_t*d);
void sanitized_dataset_fill_row(sanitized_dataset_t*s, row_t*row, int y);

#ifdef __cplusplus
}
#endif
#endif
