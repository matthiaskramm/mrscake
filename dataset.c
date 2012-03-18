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
#include "mrscake.h"
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

column_t*column_new(int num_rows, bool is_categorical, int x)
{
    column_t*c = calloc(1, sizeof(column_t)+sizeof(c->entries[0])*num_rows);
    c->is_categorical = is_categorical;
    c->index = x;
    return c;
}
void column_destroy(column_t*c)
{
    if(c->classes) {
        free(c->classes);
    }
    if(c->class_occurence_count) {
        free(c->class_occurence_count);
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
        int alloc_size = builder->category_memsize;
        if(column->classes) {
            column->class_occurence_count = realloc(column->class_occurence_count, sizeof(column->class_occurence_count[0])*alloc_size);
            column->classes = realloc(column->classes, sizeof(constant_t)*alloc_size);
        } else {
            column->class_occurence_count = malloc(sizeof(column->class_occurence_count[0])*alloc_size);
            column->classes = malloc(sizeof(constant_t)*alloc_size);
        }
        assert(pos < alloc_size);

        if(e.type == CONSTANT_STRING) {
            dict_put(builder->string2pos, e.s, INT_TO_PTR(pos + 1));
        } else if(e.type == CONSTANT_INT) {
            dict_put(builder->int2pos, INT_TO_PTR(e.i), INT_TO_PTR(pos + 1));
	} else if(e.type == CONSTANT_CATEGORY) {
            dict_put(builder->int2pos, INT_TO_PTR(e.c), INT_TO_PTR(pos + 1));
        }
        column->classes[pos] = e;
        column->class_occurence_count[pos] = 0;
    }
    column->class_occurence_count[pos]++;
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

#define DATASET_SHUFFLE 1
#define DATASET_EVEN_OUT_CLASS_COUNT 2

example_t**example_list_to_array(trainingdata_t*d, int*_num_examples, int flags)
{
    int pos = 0;
    int num_examples = 0;
    example_t**examples = 0;
    if(!(flags&DATASET_EVEN_OUT_CLASS_COUNT)) {
        example_t*i = d->first_example;
        num_examples = d->num_examples;
        examples = (example_t**)malloc(sizeof(example_t*)*num_examples);
        while(i) {
            examples[pos++] = i;
            i = i->next;
        }
    } else {
        /* build a column out of the response column, thus making
           the column build process count the classes for us */
        int num_columns = d->first_example->num_inputs;
        column_t*c = column_new(d->num_examples, true, -1);
        columnbuilder_t*b = columnbuilder_new(c);
        int y;
        example_t*i = d->first_example;
        for(y=0;y<d->num_examples;y++) {
            columnbuilder_add(b,y,variable_to_constant(&i->desired_response));
            i = i->next;
        }
        columnbuilder_destroy(b);

        int t;
        int max = c->class_occurence_count[0];
        int*multiply = malloc(sizeof(int)*c->num_classes);

        for(t=1;t<c->num_classes;t++) {
            if(c->class_occurence_count[t] > max) {
                max = c->class_occurence_count[t];
            }
        }
        for(t=0;t<c->num_classes;t++) {
            multiply[t] = max / c->class_occurence_count[t];
            num_examples += multiply[t]*c->class_occurence_count[t];
            assert(multiply);
        }
        examples = (example_t**)malloc(sizeof(example_t*)*num_examples);
        i = d->first_example;
        int pos = 0;
        for(y=0;y<d->num_examples;y++) {
            int cls = c->entries[y].c;
            int t;
            for(t=0;t<multiply[cls];t++) {
                examples[pos++] = i;
            }
            i = i->next;
        }
        assert(pos == num_examples);
        free(multiply);
        column_destroy(c);
    }

    if(flags&DATASET_SHUFFLE) {
        int t;
        for(t=0;t<num_examples;t++) {
            example_t*old = examples[t];
            int from = t+lrand48()%(num_examples-t);
            examples[t] = examples[from];
            examples[from] = old;
        }
    }
    *_num_examples = num_examples;
    return examples;
}

signature_t* signature_from_columns(column_t**columns, int num_columns, bool has_column_names)
{
    signature_t*sig = malloc(sizeof(signature_t));
    sig->num_inputs = num_columns;
    sig->column_types = calloc(num_columns, sizeof(sig->column_types[0]));
    sig->column_names = calloc(num_columns, sizeof(sig->column_names[0]));
    int t;
    for(t=0;t<num_columns;t++) {
	sig->column_types[t] = CONTINUOUS;
        if(columns[t]->is_categorical) {
            if(columns[t]->classes[0].type==CONSTANT_STRING) {
	        sig->column_types[t] = TEXT;
            } else {
	        sig->column_types[t] = CATEGORICAL;
            }
        }
	sig->column_names[t] = columns[t]->name;
    }
    sig->has_column_names = has_column_names;

    return sig;
}

dataset_t* trainingdata_sanitize(trainingdata_t*dataset)
{
    dataset_t*s = malloc(sizeof(dataset_t));

    if(!trainingdata_check_format(dataset))
        return 0;

    dict_t*column_names = extract_column_names(dataset);

    int num_examples = 0;
    example_t** examples = example_list_to_array(dataset, &num_examples,
                                      DATASET_SHUFFLE | DATASET_EVEN_OUT_CLASS_COUNT
                                      );

    s->num_columns = dataset->first_example->num_inputs;
    s->num_rows = num_examples;
    s->columns = malloc(sizeof(column_t)*s->num_columns);
    example_t*first_row = dataset->first_example;

    /* copy columns from the old to the new dataset, mapping categories
       to numbers. */
    int x,y;
    columnbuilder_t**builders = malloc(sizeof(columnbuilder_t*)*s->num_columns);
    for(x=0;x<s->num_columns;x++) {
        columntype_t ltype = first_row->inputs[x].type;
        bool is_categorical = ltype!=CONTINUOUS;
        s->columns[x] = column_new(s->num_rows, is_categorical, x);
        builders[x] = columnbuilder_new(s->columns[x]);
    }
    example_t*example = dataset->first_example;
    for(y=0;y<num_examples;y++) {
        example_t*example = examples[y];
        for(x=0;x<s->num_columns;x++) {
            int col = x;
            if(example->input_names) {
                col = PTR_TO_INT(dict_lookup(column_names,example->input_names[x]))-1;
            }
            variable_t*var = &example->inputs[x];
            columnbuilder_add(builders[col],y,variable_to_constant(var));
        }
    }
    for(x=0;x<s->num_columns;x++) {
        if(builders[x]->count != s->num_rows) {
            fprintf(stderr, "Mixup between column names. (Column %d has only %d entries).\n", x, builders[x]->count);
        }
        columnbuilder_destroy(builders[x]);
    }
    free(builders);

    /* copy response column to the new dataset */
    s->desired_response = column_new(s->num_rows, true, -1);
    columnbuilder_t*builder = columnbuilder_new(s->desired_response);
    for(y=0;y<num_examples;y++) {
        example_t*example = examples[y];
        columnbuilder_add(builder,y,variable_to_constant(&example->desired_response));
    }
    columnbuilder_destroy(builder);
    free(examples);

    bool has_column_names = false;
    if(column_names) {
        DICT_ITERATE_ITEMS(column_names, char*, name, void*, _column) {
            int column = PTR_TO_INT(_column)-1;
            s->columns[column]->name = register_string(name);
        }
        dict_destroy(column_names);
        has_column_names = 1;
    } else {
        for(x=0;x<s->num_columns;x++) {
            char name[80];
            sprintf(name, "data[%d]", x);
            s->columns[x]->name = register_string(name);
        }
    }
    s->sig = signature_from_columns(s->columns, s->num_columns, has_column_names);
    return s;
}
void dataset_print(dataset_t*s)
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

bool dataset_has_categorical_columns(dataset_t*s)
{
    int x;
    for(x=0;x<s->num_columns;x++) {
        column_t*column = s->columns[x];
        if(column->is_categorical)
            return true;
    }
    return false;
}

constant_t dataset_map_response_class(dataset_t*dataset, int i)
{
    if(i>=0 && i<dataset->desired_response->num_classes) {
        return dataset->desired_response->classes[i];
    } else {
        return missing_constant();
    }
}

void dataset_destroy(dataset_t*s)
{
    int t;
    for(t=0;t<s->num_columns;t++) {
        column_destroy(s->columns[t]);
    }
    free(s->columns);
    column_destroy(s->desired_response);
    free(s);
}

int dataset_count_expanded_columns(dataset_t*s)
{
    int x;
    int num = 0;
    for(x=0;x<s->num_columns;x++) {
        num += s->columns[x]->is_categorical ? s->columns[x]->num_classes : 1;
    }
    return num;
}

expanded_columns_t* expanded_columns_new(dataset_t*s)
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
                    PARAM(e->dataset->columns[x]);
                    GENERIC_CONSTANT(e->dataset->columns[x]->classes[cls]);
                END;
            END;
        END_CODE;
        return program;
    } else {
        START_CODE(program);
            PARAM(e->dataset->columns[x]);
        END_CODE;
        return program;
    }
}
void expanded_columns_destroy(expanded_columns_t*e)
{
    free(e->columns);
    free(e);
}


array_t* dataset_classes_as_array(dataset_t*dataset)
{
    array_t*classes = array_new(dataset->desired_response->num_classes);
    int t;
    for(t=0;t<classes->size;t++) {
        classes->entries[t] = dataset->desired_response->classes[t];
    }
    return classes;
}
void dataset_fill_row(dataset_t*s, row_t*row, int y)
{
    int x;
    for(x=0;x<row->num_inputs;x++) {
        row->inputs[x].type = MISSING;
    }
    for(x=0;x<s->num_columns;x++) {
        column_t*c = s->columns[x];
        if(c->is_categorical) {
            row->inputs[c->index] = constant_to_variable(&c->classes[c->entries[y].c]);
        } else {
            row->inputs[c->index] = variable_new_continuous(c->entries[y].f);
        }
    }
}

model_t* model_new(dataset_t*dataset)
{
    model_t*m = (model_t*)calloc(1,sizeof(model_t));
    m->sig = dataset->sig;
    return m;
}

dataset_t* dataset_pick_columns(dataset_t*data, int*index, int num)
{
    dataset_t*newdata = malloc(sizeof(dataset_t)+sizeof(column_t*)*num);
    memcpy(newdata, data, sizeof(dataset_t));
    newdata->num_columns = num;
    newdata->columns = (column_t**)(&newdata[1]);
    int t;
    for(t=0;t<num;t++) {
        newdata->columns[t] = data->columns[index[t]];
    }
    return newdata;
}


