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
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "mrscake.h"
#include "model.h"
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

extern model_factory_t* perceptron_models[];
extern int num_perceptron_models;

extern model_factory_t* twoclass_perceptron_models[];
extern int num_twoclass_perceptron_models;

typedef struct _model_collection {
    model_factory_t**models;
    int* num_models;
} model_collection_t;

model_collection_t collections[] = {
    {linear_models, &num_linear_models},
    {dtree_models, &num_dtree_models},
    {svm_models, &num_svm_models},
    {ann_models, &num_ann_models},
    {perceptron_models, &num_perceptron_models},
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

static jobqueue_t* generate_jobs(varorder_t*order, dataset_t*data)
{
    jobqueue_t* queue = jobqueue_new();
    int t;
    int s;
    int i;
//#define SUBSET_VARIABLES
#ifdef SUBSET_VARIABLES
    for(i=1;i<order->num;i++) {
        dataset_t*newdata = dataset_pick_columns(data, order->order, i);
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

extern varorder_t*dtree_var_order(dataset_t*d);

model_t* jobqueue_extract_best_and_destroy(jobqueue_t*jobs)
{
    model_t*best_model = NULL;
    int best_score = INT_MAX;
    job_t*job;
    for(job=jobs->first;job;job=job->next) {
        model_t*m = job->model;
        if(m) {
            if(job->score < best_score) {
                if(best_model) {
                    model_destroy(best_model);
                }
                best_score = job->score;
                best_model = m;
            } else {
                model_destroy(m);
            }
        }
        job->model = 0;
    }
    jobqueue_destroy(jobs);
    return best_model;
}

model_t* model_select(trainingdata_t*trainingdata)
{
    dataset_t*data = trainingdata_sanitize(trainingdata);
    if(!data)
        return 0;
#ifdef DEBUG
    printf("# %d classes, %d rows of examples (%d/%d columns)\n", data->desired_response->num_classes, data->num_rows,
            data->num_columns, dataset_count_expanded_columns(data));
#endif

    varorder_t*order = dtree_var_order(data);

    jobqueue_t*jobs = generate_jobs(order, data);
    jobqueue_process(jobs);
    model_t*best_model = jobqueue_extract_best_and_destroy(jobs);
    if(!best_model) {
        return 0;
    }

#define DEBUG
#ifdef DEBUG
    //model_errors_old(best_model, data);
    printf("# Using %s.\n", best_model->name);
    printf("# Confusion matrix:\n");
    confusion_matrix_t* cm = model_get_confusion_matrix(best_model, data);
    confusion_matrix_print(cm);
    confusion_matrix_destroy(cm);
#endif
    dataset_destroy(data);
    return best_model;
}

model_t* model_train_specific_model(trainingdata_t*trainingdata, const char*name)
{
    job_t job;
    memset(&job, 0, sizeof(job_t));
    job.data = trainingdata_sanitize(trainingdata);
    job.factory = model_factory_get_by_name(name);
    if(!job.factory)
        return 0;
    job_process(&job);
    dataset_destroy(job.data);
    return job.model;
}

confusion_matrix_t* confusion_matrix_new(dataset_t*d)
{
    confusion_matrix_t*m = (confusion_matrix_t*)malloc(sizeof(confusion_matrix_t));
    m->dataset = d;
    m->n = d->desired_response->num_classes;
    m->entries = malloc(sizeof(m->entries[0])*m->n);
    int i;
    for(i=0;i<m->n;i++) {
        m->entries[i] = calloc(1, sizeof(m->entries[0][0])*m->n);
    }
    return m;
}
void confusion_matrix_destroy(confusion_matrix_t*m)
{
    int t;
    for(t=0;t<m->n;t++) {
        free(m->entries[t]);
    }
    free(m->entries);
    free(m);
}
void confusion_matrix_print(confusion_matrix_t*m)
{
    int row,column;
    printf("p / r");
    for(column=0;column<m->n;column++) {
        printf("\t");
        constant_print(&m->dataset->desired_response->classes[column]);
    }
    printf("\n");
    for(row=0;row<m->n;row++) {
        constant_print(&m->dataset->desired_response->classes[row]);
        for(column=0;column<m->n;column++) {
            printf("\t");
            printf("%d", m->entries[row][column]);
        }
        printf("\n");
    }
    printf("\n");
}
confusion_matrix_t* model_get_confusion_matrix(model_t*m, dataset_t*s)
{
    dict_t*d = dict_new(&constant_hash_type);
    int t;
    for(t=0;t<s->desired_response->num_classes;t++) {
        dict_put(d, &s->desired_response->classes[t], INT_TO_PTR(t));
    }
    
    node_t*node = m->code;
    node_t*code = (node_t*)m->code;
    row_t* row = row_new(s->sig->num_inputs);
    environment_t*env = environment_new(code, row);

    confusion_matrix_t*matrix = confusion_matrix_new(s);

    int y;
    for(y=0;y<s->num_rows;y++) {
        dataset_fill_row(s, row, y);
        constant_t prediction = node_eval(code, env);
        constant_t* desired = &s->desired_response->classes[s->desired_response->entries[y].c];
        int column = PTR_TO_INT(dict_lookup(d, desired));
        int row = PTR_TO_INT(dict_lookup(d, &prediction));
        matrix->entries[row][column]++;
    }
    dict_destroy(d);
    row_destroy(row);
    environment_destroy(env);
    return matrix;
}

int model_errors_old(model_t*m, dataset_t*s)
{
    node_t*node = m->code;
    node_t*code = (node_t*)m->code;
    row_t* row = row_new(s->sig->num_inputs);
    environment_t*env = environment_new(code, row);

    int y;
    int error = 0;
    for(y=0;y<s->num_rows;y++) {
        dataset_fill_row(s, row, y);
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

int model_errors(model_t*m, dataset_t*s)
{
    confusion_matrix_t* c = model_get_confusion_matrix(m, s);
    int x,y,t;
    double error = 0;
    int total = 0;
    for(t=0;t<c->n;t++) {
        int row_error = 0;
        int column_error = 0;
        for(x=0;x<c->n;x++) {
            if(x!=t) {
                row_error += c->entries[t][x];
            }
        }
        for(y=0;y<c->n;y++) {
            if(y!=t) {
                column_error += c->entries[y][t];
            }
        }
        int correct = c->entries[t][t];
        if(column_error + correct) {
            error += column_error / (double)(column_error+correct);
        }
        if(row_error + correct) {
            error += row_error / (double)(row_error+correct);
        }
        total += correct + row_error; // add row sum
    }
    int cn = c->n;
    confusion_matrix_destroy(c);
    return (int)(error * total / cn / 2);
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

int model_score(model_t*m, dataset_t*data)
{
    if(!m)
        return INT_MAX;
    int size = model_size(m);
    int errors = model_errors(m, data);
    return size + errors * sizeof(uint32_t);
}


/* default for the number of training samples we should use for training, depending
   on the amount of training data available */
int training_set_size(int total_size)
{
    if(total_size < 25) {
        return total_size;
    } else {
        return (total_size+1)>>3;
    }
}
