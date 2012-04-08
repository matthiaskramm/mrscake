/* model.h
   Functions for model (function) data structures

   Part of the data prediction package.
   
   Copyright (c) 2010-2012 Matthias Kramm <kramm@quiss.org> 
 
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

#include "mrscake.h"

variable_t variable_new_categorical(category_t c);
variable_t variable_new_continuous(float f);
variable_t variable_new_text(const char*s);
variable_t variable_new_missing();
const char*variable_type(variable_t*v);
void variable_print(const variable_t*v, FILE*stream);
bool variable_equals(variable_t*v1, variable_t*v2);
double variable_value(variable_t*v);
example_t*example_new(int num_inputs);
row_t*example_to_row(example_t*e, const char**column_names);
void example_destroy(example_t*example);
row_t*row_new(int num_inputs);
void row_print(row_t*row);
void row_destroy(row_t*row);
void columntype_print(columntype_t c);
void signature_print(signature_t*sig);
void model_print(model_t*m);
void signature_destroy(signature_t*s);
variable_t model_predict(model_t*m, row_t*row);
void model_destroy(model_t*m);

#endif
