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
#if 0
    example_t* examples[5];
    examples[0] = example_new(3);
    examples[0]->inputs[0] = variable_make_categorical(7);
    examples[0]->inputs[1] = variable_make_continuous(2);
    examples[0]->inputs[2] = variable_make_continuous(1);
    examples[0]->desired_response = 15;
    examples[1] = example_new(3);
    examples[1]->inputs[0] = variable_make_categorical(7);
    examples[1]->inputs[1] = variable_make_continuous(2);
    examples[1]->inputs[2] = variable_make_continuous(2);
    examples[1]->desired_response = 14;
    examples[2] = example_new(3);
    examples[2]->inputs[0] = variable_make_categorical(7);
    examples[2]->inputs[1] = variable_make_continuous(3);
    examples[2]->inputs[2] = variable_make_continuous(3);
    examples[2]->desired_response = 13;
    examples[3] = example_new(3);
    examples[3]->inputs[0] = variable_make_categorical(7);
    examples[3]->inputs[1] = variable_make_continuous(3);
    examples[3]->inputs[2] = variable_make_continuous(4);
    examples[3]->desired_response = 12;
    examples[4] = example_new(3);
    examples[4]->inputs[0] = variable_make_categorical(7);
    examples[4]->inputs[1] = variable_make_continuous(3);
    examples[4]->inputs[2] = variable_make_continuous(5);
    examples[4]->desired_response = 11;

    model_t*m = dtree_model_factory.train(examples, 5);
    node_t*n = m->code;
    node_print(n);
    model_destroy(m);

    example_destroy(examples[0]);
    example_destroy(examples[1]);
    example_destroy(examples[2]);
    example_destroy(examples[3]);
    example_destroy(examples[4]);
#endif
}
