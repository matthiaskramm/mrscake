/* dataset.c
   Conversion between representations of training data.

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

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <assert.h>
#include "model.h"
#include "dataset.h"
#include "dict.h"
#include "ast.h"

dataset_t* dataset_new()
{
    dataset_t*d = (dataset_t*)calloc(1,sizeof(dataset_t));
    return d;
}
void dataset_add(dataset_t*d, example_t*e)
{
    e->prev = d->last_example;
    if(d->last_example) {
        d->last_example->next = e;
    }
    d->last_example = e;
    e->next = 0;
    if(!d->first_example) {
        d->first_example = e;
    }
    d->num_examples++;
}
bool dataset_check_format(dataset_t*dataset)
{
    int t;
    example_t*e;
    int last = dataset->num_examples-1;
    int pos = dataset->num_examples-1;
    for(e=dataset->first_example;e;e=e->next) {
        if(dataset->first_example->num_inputs != e->num_inputs) {
            fprintf(stderr, "Bad configuration: row %d has %d inputs, row %d has %d.\n", t, dataset->first_example->num_inputs, 0, e->num_inputs);
            return false;
        }
        int s;
        for(s=0;s<e->num_inputs;s++) {
            if(dataset->first_example->inputs[s].type != e->inputs[s].type) {
                fprintf(stderr, "Bad configuration: item %d,%d is %d, item %d,%d is %d\n",
                         pos, s,                      e->inputs[s].type,
                        last, s, dataset->first_example->inputs[s].type
                        );
                return false;
            }
        }
        pos--;
    }
    return true;
}
void dataset_print(dataset_t*dataset)
{
    example_t*e;
    for(e=dataset->first_example;e;e=e->next) {
        int s;
        for(s=0;s<e->num_inputs;s++) {
            variable_t v = e->inputs[s];
            if(v.type == CATEGORICAL) {
                printf("C%d\t", v.category);
            } else if(v.type == CONTINUOUS) {
                printf("%.2f\t", v.value);
            } else if(v.type == TEXT) {
                printf("\"%s\"\t", v.text);
            }
        }
        if(e->desired_response.type == TEXT) {
            printf("|\t\"%s\"", e->desired_response.text);
        } else {
            printf("|\tC%d", e->desired_response.category);
        }
        printf("\n");
    }
}
void dataset_destroy(dataset_t*dataset)
{
    example_t*e = dataset->first_example;
    while(e) {
        example_t*next = e->next;
        example_destroy(e);
        e = next;
    }
    free(dataset);
}

example_t**example_list_to_array(dataset_t*d)
{
    example_t**examples = (example_t**)malloc(sizeof(example_t*)*d->num_examples);
    int pos = 0;
    example_t*i = d->first_example;
    while(i) {
        examples[pos++] = i;
        i = i->next;
    }
    return examples;
}
column_t*column_new(int num_rows, bool is_categorical)
{
    column_t*c = malloc(sizeof(column_t)+sizeof(c->entries[0])*num_rows);
    c->is_categorical = is_categorical;
    c->classes = 0;
    c->num_classes = 0;
    return c;
}
void column_destroy(column_t*c)
{
    if(c->classes) {
        free(c->classes);
    }
    free(c);
}

typedef struct _columnbuilder {
    column_t*column;
    int category_memsize;
    dict_t*string2pos;
    dict_t*int2pos;
} columnbuilder_t;

columnbuilder_t*columnbuilder_new(column_t*column)
{
    columnbuilder_t*builder = (columnbuilder_t*)malloc(sizeof(columnbuilder_t));
    builder->column = column;
    builder->category_memsize = 0;
    builder->string2pos = dict_new(&charptr_type);
    builder->int2pos = dict_new(&int_type);
    return builder;
}
void columnbuilder_add(columnbuilder_t*builder, int y, constant_t e)
{
    column_t*column = builder->column;

    if(!column->is_categorical) {
        assert(e.type == CONSTANT_FLOAT);
        column->entries[y].f = e.f;
        return;
    }

    int pos = 0;
    if(e.type == CONSTANT_STRING) {
        pos = PTR_TO_INT(dict_lookup(builder->string2pos, e.s)) - 1;
    } else if(e.type == CONSTANT_INT) {
        pos = PTR_TO_INT(dict_lookup(builder->int2pos, INT_TO_PTR(e.i))) - 1;
    } else if(e.type == CONSTANT_CATEGORY) {
        pos = PTR_TO_INT(dict_lookup(builder->int2pos, INT_TO_PTR(e.c))) - 1;
    } else {
        fprintf(stderr, "Bad constant type %d in column\n", e.type);
        assert(0);
    }
    if(pos<0) {
        pos = builder->column->num_classes++;
        if(builder->category_memsize <= pos) {
            builder->category_memsize++;
            builder->category_memsize*=2;
        }
        int alloc_size = sizeof(constant_t)*builder->category_memsize;
        if(column->classes)
            column->classes = realloc(column->classes, alloc_size);
        else
            column->classes = malloc(alloc_size);

        if(e.type == CONSTANT_STRING) {
            dict_put(builder->string2pos, e.s, INT_TO_PTR(pos + 1));
        } else if(e.type == CONSTANT_INT) {
            dict_put(builder->int2pos, INT_TO_PTR(e.i), INT_TO_PTR(pos + 1));
	} else if(e.type == CONSTANT_CATEGORY) {
            dict_put(builder->int2pos, INT_TO_PTR(e.c), INT_TO_PTR(pos + 1));
        }
        column->classes[pos] = e;
    }
    column->entries[y].c = pos;
}
void columnbuilder_destroy(columnbuilder_t*builder)
{
    dict_destroy(builder->string2pos);
    dict_destroy(builder->int2pos);
    free(builder);
}

sanitized_dataset_t* dataset_sanitize(dataset_t*dataset)
{
    sanitized_dataset_t*s = malloc(sizeof(sanitized_dataset_t));

    if(!dataset_check_format(dataset))
        return 0;
    s->num_columns = dataset->first_example->num_inputs;
    s->num_rows = dataset->num_examples;
    s->columns = malloc(sizeof(column_t)*s->num_columns);
    s->expanded_num_columns = 0;
    example_t*last_row = dataset->first_example;

    /* copy columns from the old to the new dataset, mapping categories
       to numbers */
    int x;
    for(x=0;x<s->num_columns;x++) {
        columntype_t ltype = last_row->inputs[x].type;
        bool is_categorical = ltype!=CONTINUOUS;
        s->columns[x] = column_new(s->num_rows, is_categorical);

        columnbuilder_t*builder = columnbuilder_new(s->columns[x]);
        int y;
        example_t*example = dataset->first_example;
        for(y=0;y<s->num_rows;y++) {
            columnbuilder_add(builder,y,variable_to_constant(&example->inputs[x]));
            example = example->next;
        }
        columnbuilder_destroy(builder);

        s->expanded_num_columns += 
            is_categorical ? s->columns[x]->num_classes
                           : 1;
    }


    /* build expanded column info (version of the data where every class of every
       input variable has it's own 0/1 column)
     */
    s->expanded_columns = malloc(sizeof(s->expanded_columns[0])*s->expanded_num_columns);
    int pos=0;
    for(x=0;x<s->num_columns;x++) {
        if(!s->columns[x]->is_categorical) {
            s->expanded_columns[pos].col = x;
            s->expanded_columns[pos].cls = 0;
            pos++;
        } else {
            int c;
            for(c=0;c<s->columns[x]->num_classes;c++) {
                s->expanded_columns[pos].col = x;
                s->expanded_columns[pos].cls = c;
                pos++;
            }
        }
    }
    assert(pos == s->expanded_num_columns);

    /* copy response column to the new dataset */
    s->desired_response = column_new(s->num_rows, true);
    columnbuilder_t*builder = columnbuilder_new(s->desired_response);
    example_t*example = dataset->first_example;
    int y;
    for(y=0;y<s->num_rows;y++) {
        columnbuilder_add(builder,y,variable_to_constant(&example->desired_response));
        example = example->next;
    }
    columnbuilder_destroy(builder);
    return s;
}
void sanitized_dataset_print(sanitized_dataset_t*s)
{
    int x,y;
    for(y=0;y<s->num_rows;y++) {
        for(x=0;x<s->num_columns;x++) {
            column_t*column = s->columns[x];
            if(column->is_categorical) {
                constant_t c = column->classes[column->entries[y].c];
                printf("%d(", column->entries[y].c);
                constant_print(&c);
                printf(")\t");
            } else {
                printf("%.2f\t", column->entries[y].f);
            }
        }
        printf("| ");
        constant_t c = s->desired_response->classes[s->desired_response->entries[y].c];
        constant_print(&c);
        printf("\n");
    }
}

constant_t sanitized_dataset_map_response_class(sanitized_dataset_t*dataset, int i)
{
    if(i>=0 && i<dataset->desired_response->num_classes) {
        return dataset->desired_response->classes[i];
    } else {
        return missing_constant();
    }
}

void sanitized_dataset_destroy(sanitized_dataset_t*s)
{
    int t;
    for(t=0;t<s->num_columns;t++) {
        column_destroy(s->columns[t]);
    }
    free(s->columns);
    column_destroy(s->desired_response);
    free(s);
}

node_t* parameter_code(sanitized_dataset_t*d, int num)
{
    int x = d->expanded_columns[num].col;
    int cls = d->expanded_columns[num].cls;

    if(d->columns[x]->is_categorical) {
        START_CODE(program);
            BOOL_TO_FLOAT
                EQUALS
                    VAR(x);
                    CONSTANT(d->columns[x]->classes[cls]);
                END;
            END;
        END_CODE;
        return program;
    } else {
        return node_new_with_args(&node_var, x);
    }
}

array_t* sanitized_dataset_classes_as_array(sanitized_dataset_t*dataset)
{
    array_t*classes = array_new(dataset->desired_response->num_classes);
    int t;
    for(t=0;t<classes->size;t++) {
        classes->entries[t] = dataset->desired_response->classes[t];
    }
    return classes;
}

