/* mrscake.c
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

#include "model_select.h"

model_t* trainingdata_train(trainingdata_t*dataset)
{
    return model_select(dataset);
}

model_t* trainingdata_train_specific_model(trainingdata_t*trainingdata, const char*name)
{
    return model_train_specific_model(trainingdata, name);
}

