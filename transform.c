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
#include "transform.h"
#include "easy_ast.h"

// ----------------------- expand columns transform ----------------------------

typedef struct _expanded_column {
    int source_column;
    int source_class;
    bool needs_to_be_freed;
} expanded_column_t;

typedef struct _transform_expand {
    transform_t head;
    int num;
    bool use_header_code;
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
                        PARAM(orig_dataset->columns[x]);
                        GENERIC_CONSTANT(orig_dataset->columns[x]->classes[cls]);
                    END;
                END;
            END_CODE;
            node_destroy(node);
            return program;
        } else {
            START_CODE(program);
                PARAM(orig_dataset->columns[x]);
            END_CODE;
            node_destroy(node);
            return program;
        }
    }
    int i;
    for(i=0;i<node->num_children;i++) {
        node_set_child(node, i, expand_revert_in_code(dataset, node->child[i]));
    }
}

static void expand_destroy(dataset_t*dataset)
{
    transform_expand_t* transform = (transform_expand_t*)dataset->transform;
    int i;
    for(i=0;i<transform->num;i++) {
        if(transform->ecolumns[i].needs_to_be_freed)
            free(dataset->columns[i]);
    }
    free(transform->ecolumns);
    free(transform);
}

static expanded_column_t* expanded_columns(dataset_t*dataset)
{
    /* build expanded column info (version of the data where every class of every
       input variable has its own 0/1 column)
     */
    transform_expand_t* transform = (transform_expand_t*)dataset->transform;
    int num = dataset_count_expanded_columns(dataset);
    
    expanded_column_t* columns = malloc(sizeof(expanded_column_t)*num);
    int pos=0;
    int x;
    for(x=0;x<dataset->num_columns;x++) {
        if(!dataset->columns[x]->is_categorical) {
            transform->ecolumns[pos].source_column = x;
            transform->ecolumns[pos].source_class = 0;
            pos++;
        } else {
            int c;
            for(c=0;c<dataset->columns[x]->num_classes;c++) {
                transform->ecolumns[pos].source_column = x;
                transform->ecolumns[pos].source_class = c;
                pos++;
            }
        }
    }
    assert(pos == num);
    return columns;
}

static dataset_t* expand_dataset(transform_expand_t*transform, dataset_t*orig_dataset)
{
    dataset_t* dataset = malloc(sizeof(dataset_t));
    *dataset = *orig_dataset;
    dataset->transform = (transform_t*)transform;
    dataset->columns = (column_t**)malloc(sizeof(column_t*)*transform->num);

    int i;
    for(i=0;i<transform->num;i++) {
        int x = transform->ecolumns[i].source_column;
        int cls = transform->ecolumns[i].source_class;
        column_t* source_column = orig_dataset->columns[x];

        if(orig_dataset->columns[x]->is_categorical) {
            column_t*c = column_new(dataset->num_rows, false, x);
            int y;
            for(y=0;y<dataset->num_rows;y++) {
                c->entries[y].f = (source_column->entries[y].c==cls) ? 1.0 : 0.0;
            }
            dataset->columns[i] = c;
        } else {
            dataset->columns[i] = orig_dataset->columns[x];
        }
    }
    return dataset;
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

// ----------------------------------------------------------------------------

dataset_t* reverse_transformation(dataset_t*dataset, node_t**code)
{
    if(!dataset->transform)
        return dataset;
    transform_t*transform = dataset->transform;
    dataset_t*original = transform->original;
    *code = transform->revert_in_code(dataset, *code);
    transform->destroy(dataset);
    return original;
}

dataset_t* reverse_transformations(dataset_t*dataset, node_t**code)
{
    while(dataset->transform) {
        dataset = reverse_transformations(dataset, code);
    }
    return dataset;
}
