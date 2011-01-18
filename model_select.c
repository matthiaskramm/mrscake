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
#include "model.h"
#include "model_select.h"
#include "ast.h"
#include "io.h"
#include "dataset.h"
#include "codegen.h"

#define NUM(l) (sizeof(l)/sizeof((l)[0]))

extern model_factory_t* ann_models[];
extern int num_ann_models;

extern model_factory_t* dtree_models[];
extern int num_dtree_models;

extern model_factory_t* svm_models[];
extern int num_svm_models;

extern model_factory_t* rtrees_models[];
extern int num_rtrees_models;

typedef struct _model_collection {
    model_factory_t**models;
    int* num_models;
} model_collection_t;

model_collection_t collections[] = {
    {dtree_models, &num_dtree_models},
    //{svm_models, &num_svm_models},
    {ann_models, &num_ann_models},
};

model_t* model_select(trainingdata_t*trainingdata)
{
    model_t*best_model = 0;
    model_factory_t*best_factory = 0;
    int best_score = INT_MAX;
    int t;
    int s;
    sanitized_dataset_t*data = dataset_sanitize(trainingdata);
#define DEBUG
#ifdef DEBUG
    printf("# %d classes, %d rows of examples\n", data->desired_response->num_classes, data->num_rows);
#endif
    for(s=0;s<NUM(collections);s++) {
        model_collection_t*collection = &collections[s];
        for(t=0;t<*collection->num_models;t++) {
            model_factory_t*factory = collection->models[t];
#ifdef DEBUG
            printf("# Trying %s... ", factory->name);fflush(stdout);
#endif
            model_t*m = factory->train(factory, data);

            if(m) {
                m->name = factory->name;
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
#ifdef SHOW_CODE
		printf("# -------------------------------\n");
		printf("%s\n", generate_code(&codegen_python, m));
		printf("# -------------------------------\n");
#endif

                if(score < best_score) {
                    if(best_model) {
                        model_destroy(best_model);
                    }
                    best_score = score;
                    best_factory = factory;
                    best_model = m;
                } else {
                    model_destroy(m);
                }
            } else {
#ifdef DEBUG
                printf("failed\n");
#endif
            }
        }
    }
            
    sanitized_dataset_destroy(data);
#ifdef DEBUG
    printf("# Using %s.\n", best_factory->name);
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
    node_write(node, w);
    int size = w->pos;
    w->finish(w);
    return size;
}

