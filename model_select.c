/* model_select.c
   Automatic model selection.

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

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "mrscake.h"
#include "model_select.h"
#include "ast.h"
#include "io.h"
#include "dataset.h"
#include "codegen.h"
#include "serialize.h"
#include "net.h"
#include "settings.h"
#include "var_selection.h"
#include "job.h"

#define NUM(l) (sizeof(l)/sizeof((l)[0]))

extern model_factory_t* linear_models[];
extern int num_linear_models;

extern model_factory_t* ann_models[];
extern int num_ann_models;

extern model_factory_t* dtree_models[];
extern int num_dtree_models;

extern model_factory_t* svm_models[];
extern int num_svm_models;

typedef struct _model_collection {
    model_factory_t**models;
    int* num_models;
} model_collection_t;

model_collection_t collections[] = {
    {linear_models, &num_linear_models},
    {dtree_models, &num_dtree_models},
    {svm_models, &num_svm_models},
    {ann_models, &num_ann_models},
};

model_factory_t* model_factory_get_by_name(const char*name)
{
    int s,t;
    for(s=0;s<NUM(collections);s++) {
        model_collection_t*collection = &collections[s];
        for(t=0;t<*collection->num_models;t++) {
            model_factory_t*factory = collection->models[t];
            if(!strcmp(factory->name, name)) {
                return factory;
            }
        }
    }
    return 0;
}

static jobqueue_t* generate_jobs(varorder_t*order, sanitized_dataset_t*data)
{
    jobqueue_t* queue = jobqueue_new();
    int t;
    int s;
    int i;
#ifdef SUBSET_VARIABLES
    for(i=1;i<order->num;i++) {
        sanitized_dataset_t*newdata = sanitized_dataset_pick_columns(data, order->order, i);
        for(s=0;s<NUM(collections);s++) {
            model_collection_t*collection = &collections[s];
            for(t=0;t<*collection->num_models;t++) {
                model_factory_t*factory = collection->models[t];
                job_t* job = job_new();
                job->factory = factory;
                job->model = NULL;
                job->data = newdata;
                jobqueue_append(queue,job);
            }
        }
    }
#else
    for(s=0;s<NUM(collections);s++) {
        model_collection_t*collection = &collections[s];
        for(t=0;t<*collection->num_models;t++) {
            model_factory_t*factory = collection->models[t];
            job_t* job = job_new();
            job->factory = factory;
            job->model = NULL;
            job->data = data;
            jobqueue_append(queue,job);
        }
    }
#endif
    return queue;
}

extern varorder_t*dtree_var_order(sanitized_dataset_t*d);

model_t* jobqueue_extract_best_and_destroy(jobqueue_t*jobs, sanitized_dataset_t*data)
{
    model_t*best_model = NULL;
    int best_score = INT_MAX;
    job_t*job;
    for(job=jobs->first;job;job=job->next) {
	model_t*m = job->model;
	if(m) {
	    int size = model_size(m);
#ifdef DEBUG
	    printf("model size %d", size);fflush(stdout);
#endif
	    int errors = model_errors(m, data);
	    int score = size + errors * sizeof(uint32_t);
#ifdef DEBUG
	    printf(", %d errors (score: %d)\n", errors, score);fflush(stdout);
	    node_sanitycheck((node_t*)m->code);
#endif
//#define SHOW_CODE
#ifdef SHOW_CODE
	    //node_print((node_t*)m->code);
	    printf("# -------------------------------\n");
	    printf("%s\n", generate_code(&codegen_js, m));
	    printf("# -------------------------------\n");
#endif
	    if(score < best_score) {
		if(best_model) {
		    model_destroy(best_model);
		}
		best_score = score;
		best_model = m;
	    } else {
		model_destroy(m);
	    }
	} else {
#ifdef DEBUG
	    printf("failed\n");
#endif
	}
	job->model = 0;
    }
    jobqueue_destroy(jobs);
    return best_model;
}

model_t* model_select(trainingdata_t*trainingdata)
{
    sanitized_dataset_t*data = dataset_sanitize(trainingdata);
    if(!data)
        return 0;
#define DEBUG
#ifdef DEBUG
    printf("# %d classes, %d rows of examples (%d/%d columns)\n", data->desired_response->num_classes, data->num_rows,
            data->num_columns, sanitized_dataset_count_expanded_columns(data));
#endif

    varorder_t*order = dtree_var_order(data);

    jobqueue_t*jobs = generate_jobs(order, data);
    jobqueue_process(jobs);
    model_t*best_model = jobqueue_extract_best_and_destroy(jobs, data);
#ifdef DEBUG
    printf("# Using %s.\n", best_model->name);
#endif
    sanitized_dataset_destroy(data);
    return best_model;
}

int model_errors(model_t*m, sanitized_dataset_t*s)
{
    node_t*node = m->code;
    int t;
    int error = 0;
    node_t*code = (node_t*)m->code;
    row_t* row = row_new(s->num_columns);
    environment_t*env = environment_new(code, row);
    int y;
    for(y=0;y<s->num_rows;y++) {
        sanitized_dataset_fill_row(s, row, y);
        constant_t prediction = node_eval(code, env);
        constant_t* desired = &s->desired_response->classes[s->desired_response->entries[y].c];
        if(!constant_equals(&prediction, desired)) {
            error++;
        }
    }
    row_destroy(row);
    environment_destroy(env);
    return error;
}

int model_size(model_t*m)
{
    node_t*node = m->code;
    writer_t *w = nullwriter_new();
    node_write(node, w, SERIALIZE_FLAG_OMIT_STRINGS);
    int size = w->pos;
    w->finish(w);
    return size;
}

int training_set_size(int total_size)
{
    if(total_size < 25) {
        return total_size;
    } else {
        return (total_size+1)>>1;
    }
}
