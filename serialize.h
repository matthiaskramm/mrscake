/* serialize.h
   Serialization of models and ASTs

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

#ifndef __serialize_h__
#define __serialize_h__

#include "io.h"
#include "ast.h"
#include "dataset.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SERIALIZE_DEFAULTS 0
#define SERIALIZE_FLAG_OMIT_STRINGS 1

node_t* node_read(reader_t*read);
void node_write(node_t*node, writer_t*writer, unsigned flags);

model_t* model_read(reader_t*r);
model_t* model_load(const char*filename);
void model_save(model_t*m, const char*filename);
void model_write(model_t*m, writer_t*w);

trainingdata_t* trainingdata_read(reader_t*r);
void trainingdata_write(trainingdata_t*d, writer_t*w);
void trainingdata_save(trainingdata_t*d, const char*filename);
trainingdata_t* trainingdata_load(const char*filename);

void dataset_save(dataset_t*d, const char*filename);
dataset_t* dataset_load(const char*filename);
void dataset_write(dataset_t*d, writer_t*w);
dataset_t*dataset_read(reader_t*r);

#ifdef __cplusplus
}
#endif

#endif
