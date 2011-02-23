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

typedef struct _job {
    model_factory_t*factory;
    model_t*model;
} job_t;

model_factory_t* model_factory_get_by_name(const char*name)
{
    int s,t;
    for(s=0;s<NUM(collections);s++) {
        model_collection_t*collection = &collections[s];
        for(t=0;t<*collection->num_models;t++) {
            model_factory_t*factory = collection->models[t];
            if(!strcmp(factory, name)) {
                return factory;
            }
        }
    }
    return 0;
}

static job_t* generate_jobs(int*num_jobs)
{
    int t;
    int s;
    int num = 0;
    for(s=0;s<NUM(collections);s++) {
        model_collection_t*collection = &collections[s];
	num += *collection->num_models;
    }
    *num_jobs = num;
    job_t*jobs = malloc(sizeof(job_t)*num);
    int pos = 0;
    for(s=0;s<NUM(collections);s++) {
        model_collection_t*collection = &collections[s];
        for(t=0;t<*collection->num_models;t++) {
            model_factory_t*factory = collection->models[t];
	    jobs[pos].factory = factory;
	    jobs[pos].model = NULL;
	    pos++;
        }
    }
    assert(pos==num);
    return jobs;
}

#define FORK_FOR_TRAINING
model_t* train_model(model_factory_t*factory, sanitized_dataset_t*data)
{
#ifndef FORK_FOR_TRAINING
    model_t* m = factory->train(factory, data);
    if(m) {
        m->name = factory->name;
    }
    return m;
#else
    int p[2];
    int ret = pipe(p);
    int read_fd = p[0];
    int write_fd = p[1];
    if(ret) {
        perror("create pipe");
        exit(-1);
    }
    pid_t pid = fork();
    if(!pid) {
        //child
        close(read_fd); // close read
        model_t*m = factory->train(factory, data);
        if(m) {
            m->name = factory->name;
        }
        writer_t*w = filewriter_new(write_fd);
        model_write(m, w);
        w->finish(w);
        close(write_fd); // close write
        _exit(0);
    } else {
        //parent
        close(write_fd); // close write
        reader_t*r = filereader_with_timeout_new(read_fd, 300);
        model_t*m = model_read(r);
        r->dealloc(r);
        close(read_fd); // close read
        ret = waitpid(pid, NULL, WNOHANG);
        if(ret < 0) {
            kill(pid, SIGKILL);
        }
        return m;
    }
#endif
}

static void process_jobs(job_t*jobs, int num_jobs, sanitized_dataset_t*data)
{
    int t;
    for(t=0;t<num_jobs;t++) {
#ifdef DEBUG
	printf("# Trying %s... ", factory->name);fflush(stdout);
#endif
	jobs[t].model = train_model(jobs[t].factory, data);
    }
}

model_t* model_select(trainingdata_t*trainingdata)
{
    model_t*best_model = 0;
    int best_score = INT_MAX;
    int t;
    sanitized_dataset_t*data = dataset_sanitize(trainingdata);
    if(!data)
        return 0;
#define DEBUG
#ifdef DEBUG
    printf("# %d classes, %d rows of examples (%d/%d columns)\n", data->desired_response->num_classes, data->num_rows,
            data->num_columns, sanitized_dataset_count_expanded_columns(data));
#endif

    int num_jobs = 0;
    job_t*jobs = generate_jobs(&num_jobs);

    process_jobs(jobs, num_jobs, data);

    for(t=0;t<num_jobs;t++) {
	model_t*m = jobs[t].model;
	printf("# %s: ", jobs[t].factory->name);fflush(stdout);
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
	jobs[t].model = 0;
    }
    free(jobs);
            
    sanitized_dataset_destroy(data);
#ifdef DEBUG
    printf("# Using %s.\n", best_model->name);
#endif
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
