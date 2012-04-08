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

//#define HEIGHT 256
//#define WIDTH 16
#define HEIGHT 64
#define WIDTH 4

int main(int argn, char*argv[])
{
    //config_parse_remote_servers("servers.txt");

    trainingdata_t* data = trainingdata_new();

    int t;
    for(t=0;t<HEIGHT;t++) {
        example_t*e = example_new(WIDTH);
        int s;
        for(s=0;s<WIDTH;s++) {
            e->inputs[s] = variable_new_continuous(lrand48()&255);
        }
        e->inputs[2] = variable_new_continuous(((t%2)+1)*100+(lrand48()&15));
        //e->inputs[3] = variable_new_categorical(t&3);
        e->desired_response = variable_new_categorical(t&1);
        trainingdata_add_example(data, e);
    }

    /* test load&save */
    trainingdata_save(data, "/tmp/data.data");
    trainingdata_destroy(data);
    data = trainingdata_load("/tmp/data.data");

    model_t*m = 0;
    if(argn>1) {
        char*model_name = argv[1];
        m = trainingdata_train_specific_model(data, model_name);
        if(!m) {
            fprintf(stderr, "Error training %s (or no models called %s)\n", model_name, model_name);
            exit(0);
        }
    } else {
        m = trainingdata_train(data);
        if(!m) {
            fprintf(stderr, "Error selecting model\n");
            exit(0);
        }
    }

    dataset_t* dataset = trainingdata_sanitize(data);
    confusion_matrix_t* confusion = code_get_confusion_matrix(m->code, dataset);
    confusion_matrix_print(confusion);
    confusion_matrix_destroy(confusion);

    printf("model: %s\n", m->name);
    printf("size: %d\n", code_size(m->code));
    printf("errors: %d\n", code_errors_old(m->code, dataset));
    printf("confusion matrix errors: %d\n", code_errors(m->code, dataset));
    printf("score: %d\n", code_score(m->code, dataset));
    
//#define VERIFY_RESULT
#ifdef VERIFY_RESULT
    while(1) {
        row_t*r = row_new(WIDTH);
        int i;
        for(i=0;i<WIDTH;i++) {
            r->inputs[i] = variable_new_continuous(lrand48()&255);
        }
        variable_t v = model_predict(m, r);
    }

    example_t*e;
    int count = 0;
    for(e = data->first_example; e; e = e->next) {
        row_t*r = example_to_row(e, 0);
        variable_t v = model_predict(m, r);
        printf("%d) ", count);
        variable_print(&v, stdout);
        printf(" / ");
        variable_print(&e->desired_response, stdout);
        printf("\n");
        count++;
    }
#endif

    char*code = model_generate_code(m, "python");
    printf("%s\n", code);
    free(code);

    model_destroy(m);

    trainingdata_destroy(data);
}
