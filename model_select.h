/* model_select.h
   Automatic model selection

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

#ifndef __model_select_h__
#define  __model_select_h__

#include "dataset.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _model_factory {
    const char*name;
    model_t*(*train)(struct _model_factory*factory, sanitized_dataset_t*dataset);
    void*internal;
} model_factory_t;

int training_set_size(int total_size);

typedef model_t*(*training_function_t)(model_factory_t*factory, sanitized_dataset_t*dataset);

model_t* model_select(trainingdata_t*);
int model_errors(model_t*m, sanitized_dataset_t*s);
int model_score(model_t*m, sanitized_dataset_t*d);
model_t* train_model(model_factory_t*factory, sanitized_dataset_t*data);

model_factory_t* model_factory_get_by_name(const char*name);

#ifdef __cplusplus
}
#endif

#endif //__model_select_h__
