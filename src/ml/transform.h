/* transform.h
   Dataset transformations

   Part of the mrscake data prediction package.
   
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

#ifndef __transform_h__
#define __transform_h__

#include <stdbool.h>
#include "dataset.h"
#include "ast.h"

#ifdef __cplusplus
extern "C" {
#endif

/* transformations */
dataset_t* expand_categorical_columns(dataset_t*old_dataset);
dataset_t* pick_columns(dataset_t*old_dataset, int*index, int num);
dataset_t* remove_text_columns(dataset_t*old_dataset);
dataset_t* expand_text_columns(dataset_t*old_dataset);
dataset_t* find_clear_cut_columns(dataset_t*old_dataset);

dataset_t* dataset_apply_named_transformation(dataset_t*old_dataset, const char*transform);
dataset_t* dataset_apply_transformations(dataset_t*dataset, const char*transform);
dataset_t* dataset_revert_one_transformation(dataset_t*dataset, node_t**code);
dataset_t* dataset_revert_all_transformations(dataset_t*dataset, node_t**code);

/* transformation name generators */
char* pick_columns_transform(int*index, int num);

#ifdef __cplusplus
}
#endif
#endif
