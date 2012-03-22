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
#include "model_select.h"

int main(int argn, char*argv[])
{
    char* model_name = "perceptron";

    if(argn>1)
        model_name = argv[1];

    //config_parse_remote_servers("servers.txt");

    trainingdata_t* data = trainingdata_new();

    int t;
    for(t=0;t<256;t++) {
        example_t*e = example_new(16);
        int s;
        for(s=0;s<16;s++) {
            e->inputs[s] = variable_new_continuous((lrand48()%256)/256.0);
        }
        e->desired_response = variable_new_categorical(t%2);
        trainingdata_add_example(data, e);
    }

    /* test load&save */
    trainingdata_save(data, "/tmp/data.data");
    trainingdata_destroy(data);
    data = trainingdata_load("/tmp/data.data");

    model_t*m = trainingdata_train_specific_model(data, model_name);
    if(!m) {
        fprintf(stderr, "Error training %s (or no models called %s)\n", model_name, model_name);
        exit(0);
    }

    dataset_t* dataset = trainingdata_sanitize(data);
    confusion_matrix_t* confusion = model_get_confusion_matrix(m, dataset);
    confusion_matrix_print(confusion);
    confusion_matrix_destroy(confusion);

    printf("size: %d\n", model_size(m));
    printf("errors: %d\n", model_errors_old(m, dataset));
    printf("confusion matrix errors: %d\n", model_errors(m, dataset));
    printf("score: %d\n", model_score(m, dataset));

    char*code = model_generate_code(m, "python");
    printf("%s\n", code);
    free(code);

    model_destroy(m);

    trainingdata_destroy(data);
}
