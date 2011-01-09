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
#include "model.h"
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
    bool is_categorical;

    int num_classes;
    constant_t*classes;
    
    union {
        float f;
        category_t c;
    } entries[0];
};

void sanitized_dataset_destroy(sanitized_dataset_t*dataset);
example_t**example_list_to_array(dataset_t*d);
sanitized_dataset_t* dataset_sanitize(dataset_t*dataset);
void sanitized_dataset_print(sanitized_dataset_t*s);

#ifdef __cplusplus
}
#endif
#endif
