/* test_model.c
   Test routines for model selection.

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

#include <stdio.h>
#include <stdlib.h>
#include "model.h"
#include "ast.h"

int main()
{
    trainingdata_t* data = trainingdata_new();

    int t;
    for(t=0;t<256;t++) {
        example_t*e = example_new(16);
        int s;
        for(s=0;s<16;s++) {
            e->inputs[s] = variable_make_continuous((lrand48()%256)/256.0);
        }
        e->desired_response = variable_make_categorical(t%3);
        trainingdata_add_example(data, e);
    }
    model_t*m = model_select(data);
    model_destroy(m);

    trainingdata_destroy(data);
}
