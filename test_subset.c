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
#include "mrscake.h"
#include "ast.h"

int main()
{
    //config_parse_remote_servers("servers.txt");

    int s;
    for(s=0;s<16;s++) {

        trainingdata_t* data = trainingdata_new();

        int t;
        for(t=0;t<64;t++) {
            example_t*e = example_new(16);
            int i;
            for(i=0;i<16;i++) {
                e->inputs[i] = variable_new_continuous((lrand48()%256)/256.0);
            }
            e->inputs[s] = variable_new_continuous(t%4);
            e->desired_response = variable_new_categorical(t%4);
            trainingdata_add_example(data, e);

        }

        model_t*m = trainingdata_train(data);

        char*code = model_generate_code(m, "python");
        printf("%s\n", code);

        trainingdata_destroy(data);

        for(t=0;t<4;t++) {
            row_t*r = row_new(16);
            int i;
            for(i=0;i<16;i++) {
                r->inputs[i] = variable_new_continuous(0);
            }
            r->inputs[s] = variable_new_continuous(t);
            variable_t result = model_predict(m, r);
            row_destroy(r);
            printf("%d %d\n", t, result.category);
        }

        model_destroy(m);
    }
}
