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
#include <string.h>
#include "mrscake.h"
#include "ast.h"
#include "dataset.h"
#include "job.h"
#include "transform.h"

int main()
{
    //config_parse_remote_servers("servers.txt");

    int s;
    for(s=0;s<16;s++) {
        printf("---- test iteration %d ----\n", s);

        trainingdata_t* trainingdata = trainingdata_new();

        int t;
        for(t=0;t<64;t++) {
            example_t*e = example_new(16);
            int i;
            for(i=0;i<16;i++) {
                e->inputs[i] = variable_new_continuous((lrand48()%256)/256.0);
            }
            e->inputs[s] = variable_new_continuous(t%4);
            e->desired_response = variable_new_categorical(t%4);
            trainingdata_add_example(trainingdata, e);
        }

        dataset_t* data = trainingdata_sanitize(trainingdata);
        
        int num_columns = 0;
        int pos = 0;
        int positions[data->num_columns];
        while(1) {
            pos += (lrand48()%(data->num_columns-1)) + 1;
            if(pos >= data->num_columns)
                break;
            positions[num_columns++] = pos;
        }
        
        for(t=0;t<num_columns;t++)
            printf("%d ", positions[t]);
        printf("\n");

        job_t job;
        memset(&job, 0, sizeof(job_t));
        data = pick_columns(data, positions, num_columns);

        job.data = data;
        job.factory = model_factory_get_by_name("simplified linear svm");
        job_process(&job);
        if(!job.code) {
            continue;
        }

        data = dataset_revert_all_transformations(data, &job.code);
        //node_print(job.model->code);

        trainingdata_destroy(trainingdata);

#if 0
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
#endif
    }
}
