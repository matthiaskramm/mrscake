/* transform.c
   Dataset transformations

   Part of the data prediction package.
   
   Copyright (c) 2012 Matthias Kramm <kramm@quiss.org> 
 
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

#include <stdlib.h>
#include <assert.h>
#include "transform.h"
#include "easy_ast.h"
#include "util.h"

// ----------------------- expand columns transform ----------------------------

typedef struct _expanded_column {
    int source_column;
    int source_class;
    bool from_category;
} expanded_column_t;

typedef struct _transform_expand {
    transform_t head;
    int num;
    expanded_column_t* ecolumns;
} transform_expand_t;

static node_t* expand_revert_in_code(dataset_t*dataset, node_t* node)
{
    transform_expand_t*transform = (transform_expand_t*)dataset->transform;
    dataset_t*orig_dataset = dataset->transform->original;
   
    if(node->type == &node_param) {
        int i = node->value.i;
        int x = transform->ecolumns[i].source_column;
        int cls = transform->ecolumns[i].source_class;

        if(orig_dataset->columns[x]->is_categorical) {
            START_CODE(program);
                BOOL_TO_FLOAT
                    EQUALS
                        PARAM(x);
                        GENERIC_CONSTANT(orig_dataset->columns[x]->classes[cls]);
                    END;
                END;
            END_CODE;
            node_destroy(node);
            return program;
        } else {
            START_CODE(program);
                PARAM(x);
            END_CODE;
            node_destroy(node);
            return program;
        }
    }
    int i;
    for(i=0;i<node->num_children;i++) {
        node_set_child(node, i, expand_revert_in_code(dataset, node->child[i]));
    }
    return node;
}

static expanded_column_t* expanded_columns(dataset_t*dataset)
{
    /* build expanded column info (version of the data where every class of every
       input variable has its own 0/1 column)
     */
    int num = dataset_count_expanded_columns(dataset);
    
    expanded_column_t* ecolumns = calloc(sizeof(expanded_column_t),num);
    int pos=0;
    int x;
    for(x=0;x<dataset->num_columns;x++) {
        if(!dataset->columns[x]->is_categorical) {
            ecolumns[pos].source_column = x;
            ecolumns[pos].source_class = 0;
            ecolumns[pos].from_category = false;
            pos++;
        } else {
            int c;
            for(c=0;c<dataset->columns[x]->num_classes;c++) {
                ecolumns[pos].source_column = x;
                ecolumns[pos].source_class = c;
                ecolumns[pos].from_category = true;
                pos++;
            }
        }
    }

    assert(pos == num);
    return ecolumns;
}

static dataset_t* expand_dataset(transform_expand_t*transform, dataset_t*orig_dataset)
{
    dataset_t* dataset = memdup(orig_dataset, sizeof(dataset_t));
    dataset->transform = (transform_t*)transform;
    dataset->columns = (column_t**)malloc(sizeof(column_t*)*transform->num);
    dataset->num_columns = transform->num;
    dataset->sig = 0;

    int i;
    for(i=0;i<transform->num;i++) {
        expanded_column_t*e = &transform->ecolumns[i];
        int x = e->source_column;
        int cls = e->source_class;
        column_t* source_column = orig_dataset->columns[x];

        int y;
        if(orig_dataset->columns[x]->is_categorical) {
            assert(e->from_category);
            column_t*c = column_new(dataset->num_rows, false);
            for(y=0;y<dataset->num_rows;y++) {
                c->entries[y].f = (source_column->entries[y].c==cls) ? 1.0 : 0.0;
            }
            dataset->columns[i] = c;
        } else {
            dataset->columns[i] = source_column;
        }
    }
    return dataset;
}

static void expand_destroy(dataset_t*dataset)
{
    transform_expand_t* transform = (transform_expand_t*)dataset->transform;
    int i;
    for(i=0;i<transform->num;i++) {
        expanded_column_t*e = &transform->ecolumns[i];
        if(e->from_category) {
            free(dataset->columns[i]);
        }
    }
    free(dataset->columns);
    free(dataset);
    free(transform->ecolumns);
    free(transform);
}

dataset_t* expand_categorical_columns(dataset_t*old_dataset)
{
    transform_expand_t*transform = calloc(1, sizeof(transform_expand_t));
    transform->head.revert_in_code = expand_revert_in_code;
    transform->head.destroy = expand_destroy;
    transform->head.original = old_dataset;

    transform->num = dataset_count_expanded_columns(old_dataset);
    transform->ecolumns = expanded_columns(old_dataset);

    return expand_dataset(transform, old_dataset);
}

// ----------------------- pick colummns transform -----------------------------

typedef struct _transform_pick_columns {
    transform_t head;
    int*original_column;
} transform_pick_columns_t;

static node_t* pick_columns_revert_in_code(dataset_t*dataset, node_t*node)
{
    transform_pick_columns_t*transform = (transform_pick_columns_t*)dataset->transform;
    dataset_t*orig_dataset = dataset->transform->original;
   
    if(node->type == &node_param) {
        int i = node->value.i;
        assert(i >= 0 && i < dataset->num_columns);
        int x = transform->original_column[i];
        assert(x >= 0 && x < orig_dataset->num_columns);

        START_CODE(new_node);
            PARAM(x);
        END_CODE;
        node_destroy(node);
        return new_node;
    }
    int i;
    for(i=0;i<node->num_children;i++) {
        node_set_child(node, i, pick_columns_revert_in_code(dataset, node->child[i]));
    }
    return node;
}

static void pick_columns_destroy(dataset_t*dataset)
{
    transform_pick_columns_t* transform = (transform_pick_columns_t*)dataset->transform;
    free(dataset->columns);
    free(dataset);
    free(transform->original_column);
    free(transform);
}

dataset_t* pick_columns(dataset_t*old_dataset, int*index, int num)
{
    dataset_t*newdata = memdup(old_dataset, sizeof(dataset_t));
    transform_pick_columns_t*transform = calloc(1, sizeof(transform_pick_columns_t));
    transform->head.revert_in_code = pick_columns_revert_in_code;
    transform->head.destroy = pick_columns_destroy;
    transform->head.original = old_dataset;
    newdata->transform = (transform_t*)transform;

    transform->original_column = (int*)memdup(index, sizeof(int)*num);
   
    newdata->num_columns = num;
    newdata->columns = (column_t**)malloc(sizeof(column_t*)*num);
    int i;
    for(i=0;i<num;i++) {
        newdata->columns[i] = old_dataset->columns[index[i]];
    }
    return newdata;
}

// ----------------------------------------------------------------------------

dataset_t* dataset_revert_one_transformation(dataset_t*dataset, node_t**code)
{
    if(!dataset->transform)
        return dataset;
    transform_t*transform = dataset->transform;
    dataset_t*original = transform->original;
    *code = transform->revert_in_code(dataset, *code);

    //in the presence of variable selection, some transformations are shared
    //between models (see generate_jobs() in model_select.c)
    //We need reference counting or instance pooling for transforms in order
    //to free them properly.
    //
    //transform->destroy(dataset);
    
    return original;
}

dataset_t* dataset_revert_all_transformations(dataset_t*dataset, node_t**code)
{
    while(dataset->transform) {
        dataset = dataset_revert_one_transformation(dataset, code);
    }
    return dataset;
}
