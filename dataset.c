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
#include "easy_ast.h"
#include "stringpool.h"

trainingdata_t* trainingdata_new()
{
    trainingdata_t*d = (trainingdata_t*)calloc(1,sizeof(trainingdata_t));
    return d;
}
void trainingdata_add_example(trainingdata_t*d, example_t*e)
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
bool trainingdata_check_format(trainingdata_t*trainingdata)
{
    if(!trainingdata || !trainingdata->first_example)
        return false;
    int t;
    example_t*e;
    int pos = 0;
    char has_names = 0;
    char has_no_names = 0;
    for(e=trainingdata->first_example;e;e=e->next) {
        if(e->input_names)
            has_names = 1;
        if(!e->input_names)
            has_no_names = 1;
        if(has_names && has_no_names) {
            fprintf(stderr, "Please specify examples as either arrays or as name->value mappings, but not both at once\n");
            return false;
        }
        if(trainingdata->first_example->num_inputs != e->num_inputs) {
            fprintf(stderr, "Bad configuration: row %d has %d inputs, row %d has %d.\n", t, trainingdata->first_example->num_inputs, 0, e->num_inputs);
            return false;
        }
        int s;
        for(s=0;s<e->num_inputs;s++) {
            if(trainingdata->first_example->inputs[s].type != e->inputs[s].type) {
                fprintf(stderr, "Bad configuration: item %d in row %d is %s, item %d in row %d is %s\n",
                         s, pos, variable_type(&e->inputs[s]),
                         s,   0, variable_type(&trainingdata->first_example->inputs[s])
                        );
                return false;
            }
        }
        pos++;
    }
    return true;
}
void trainingdata_print(trainingdata_t*trainingdata)
{
    example_t*e;
    for(e=trainingdata->first_example;e;e=e->next) {
        int s;
        for(s=0;s<e->num_inputs;s++) {
            variable_t v = e->inputs[s];
            if(e->input_names) {
                printf("%s=", e->input_names[s]);
            }
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
void trainingdata_destroy(trainingdata_t*trainingdata)
{
    example_t*e = trainingdata->first_example;
    while(e) {
        example_t*next = e->next;
        example_destroy(e);
        e = next;
    }
    free(trainingdata);
}

example_t**example_list_to_array(trainingdata_t*d)
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
    column_t*c = calloc(1, sizeof(column_t)+sizeof(c->entries[0])*num_rows);
    c->is_categorical = is_categorical;
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
    int count;
} columnbuilder_t;

columnbuilder_t*columnbuilder_new(column_t*column)
{
    columnbuilder_t*builder = (columnbuilder_t*)calloc(1,sizeof(columnbuilder_t));
    builder->column = column;
    builder->string2pos = dict_new(&charptr_type);
    builder->int2pos = dict_new(&int_type);
    return builder;
}
void columnbuilder_add(columnbuilder_t*builder, int y, constant_t e)
{
    column_t*column = builder->column;
    builder->count++;

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

dict_t*extract_column_names(trainingdata_t*dataset)
{
    dict_t*d = 0;
    example_t*e = dataset->first_example;
    int pos = 1;
    while(e) {
        if(e->input_names) {
            if(!d) {
                d = dict_new(&charptr_type);
            }
            int x;
            for(x=0;x<e->num_inputs;x++) {
                const char*name = e->input_names[x];
                if(!dict_lookup(d, name)) {
                    dict_put(d, name, INT_TO_PTR(pos));
                    pos++;
                }
            }
        }
        e = e->next;
    }
    return d;
}

sanitized_dataset_t* dataset_sanitize(trainingdata_t*dataset)
{
    sanitized_dataset_t*s = malloc(sizeof(sanitized_dataset_t));

    if(!trainingdata_check_format(dataset))
        return 0;

    dict_t*column_names = extract_column_names(dataset);

    s->num_columns = dataset->first_example->num_inputs;
    s->num_rows = dataset->num_examples;
    s->columns = malloc(sizeof(column_t)*s->num_columns);
    example_t*last_row = dataset->first_example;

    /* copy columns from the old to the new dataset, mapping categories
       to numbers. TODO: Also shuffle the rows */
    int x,y;
    columnbuilder_t**builders = malloc(sizeof(columnbuilder_t*)*s->num_columns);
    for(x=0;x<s->num_columns;x++) {
        columntype_t ltype = last_row->inputs[x].type;
        bool is_categorical = ltype!=CONTINUOUS;
        s->columns[x] = column_new(s->num_rows, is_categorical);
        builders[x] = columnbuilder_new(s->columns[x]);
    }
    example_t*example = dataset->first_example;
    for(y=0;y<s->num_rows;y++) {
        for(x=0;x<s->num_columns;x++) {
            int col = x;
            if(example->input_names) {
                col = PTR_TO_INT(dict_lookup(column_names,example->input_names[x]))-1;
            }
            variable_t*var = &example->inputs[x];
            columnbuilder_add(builders[col],y,variable_to_constant(var));
        }
        example = example->next;
    }
    for(x=0;x<s->num_columns;x++) {
        if(builders[x]->count != s->num_rows) {
            fprintf(stderr, "Mixup between column names. (Column %d has only %d entries).\n", x, builders[x]->count);
        }
        columnbuilder_destroy(builders[x]);
    }
    free(builders);

    /* copy response column to the new dataset */
    s->desired_response = column_new(s->num_rows, true);
    columnbuilder_t*builder = columnbuilder_new(s->desired_response);
    example = dataset->first_example;
    for(y=0;y<s->num_rows;y++) {
        columnbuilder_add(builder,y,variable_to_constant(&example->desired_response));
        example = example->next;
    }
    columnbuilder_destroy(builder);

    if(column_names) {
        DICT_ITERATE_ITEMS(column_names, char*, name, void*, _column) {
            int column = PTR_TO_INT(_column)-1;
            s->columns[column]->name = register_string(name);
        }
        dict_destroy(column_names);
    }
    return s;
}
void sanitized_dataset_print(sanitized_dataset_t*s)
{
    int x,y;
    if(s->columns[0]->name) {
        for(x=0;x<s->num_columns;x++) {
            if(x) {
                printf("\t");
            }
            printf("%s", s->columns[x]->name);
        }
        printf("| ");
        printf("desired_response");
        printf("\n");
    }

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

int sanitized_dataset_count_expanded_columns(sanitized_dataset_t*s)
{
    int x;
    int num = 0;
    for(x=0;x<s->num_columns;x++) {
        num += s->columns[x]->is_categorical ? s->columns[x]->num_classes : 1;
    }
    return num;
}

expanded_columns_t* expanded_columns_new(sanitized_dataset_t*s)
{
    /* build expanded column info (version of the data where every class of every
       input variable has it's own 0/1 column)
     */
    expanded_columns_t*e = (expanded_columns_t*)calloc(1,sizeof(expanded_columns_t));
    e->dataset = s;

    int x;
    for(x=0;x<s->num_columns;x++) {
        e->num += s->columns[x]->is_categorical ? s->columns[x]->num_classes : 1;
    }

    e->columns = malloc(sizeof(e->columns[0])*e->num);
    int pos=0;
    for(x=0;x<s->num_columns;x++) {
        if(!s->columns[x]->is_categorical) {
            e->columns[pos].source_column = x;
            e->columns[pos].source_class = 0;
            pos++;
        } else {
            int c;
            for(c=0;c<s->columns[x]->num_classes;c++) {
                e->columns[pos].source_column = x;
                e->columns[pos].source_class = c;
                pos++;
            }
        }
    }
    assert(pos == e->num);
    return e;
}
node_t* expanded_columns_parameter_init(expanded_columns_t*e)
{
    e->use_header_code = true;
    return node_new(&node_nop, 0);
}
node_t* expanded_columns_parameter_code(expanded_columns_t*e, int num)
{
    int x = e->columns[num].source_column;
    int cls = e->columns[num].source_class;

    if(e->dataset->columns[x]->is_categorical) {
        START_CODE(program);
            BOOL_TO_FLOAT
                EQUALS
                    VAR(x);
                    GENERIC_CONSTANT(e->dataset->columns[x]->classes[cls]);
                END;
            END;
        END_CODE;
        return program;
    } else {
        return node_new_with_args(&node_var, x);
    }
}
void expanded_columns_destroy(expanded_columns_t*e)
{
    free(e->columns);
    free(e);
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
void sanitized_dataset_fill_row(sanitized_dataset_t*s, row_t*row, int y)
{
    int x;
    for(x=0;x<s->num_columns;x++) {
        column_t*c = s->columns[x];
        if(c->is_categorical) {
            row->inputs[x] = constant_to_variable(&c->classes[c->entries[y].c]);
        } else {
            row->inputs[x] = variable_make_continuous(c->entries[y].f);
        }
    }
}

model_t* model_new(sanitized_dataset_t*dataset)
{
    model_t*m = (model_t*)calloc(1,sizeof(model_t));
    m->num_inputs = dataset->num_columns;
    m->column_types = calloc(dataset->num_columns, sizeof(m->column_types[0]));
    m->column_names = calloc(dataset->num_columns, sizeof(m->column_names[0]));
    bool has_column_names = false;
    int t;
    for(t=0;t<dataset->num_columns;t++) {
	m->column_types[t] = dataset->columns[t]->is_categorical ? CATEGORICAL : CONTINUOUS;
	m->column_names[t] = dataset->columns[t]->name;
        has_column_names |= !!dataset->columns[t]->name;
    }

    if(!has_column_names) {
        free(m->column_names);
        m->column_names = 0;
    }
    return m;
}

