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

typedef struct _model_factory {
    const char*name;
    model_t*(*train)(dataset_t*dataset);
    void*internal;
} model_factory_t;

extern model_factory_t dtree_model_factory;
extern model_factory_t ann_model_factory;

model_t* model_select(dataset_t*);
#endif
