#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <assert.h>
#include "model.h"
#include "dataset.h"

dataset_t* dataset_new()
{
    dataset_t*d = (dataset_t*)calloc(1,sizeof(dataset_t));
    return d;
}
void dataset_add(dataset_t*d, example_t*e)
{
    /* TODO: preprocess categories */
    e->previous = d->examples;
    d->examples = e;
    d->num_examples++;
}
bool dataset_check_format(dataset_t*dataset)
{
    int t;
    example_t*last_example = dataset->examples;
    example_t*e;
    int last = dataset->num_examples-1;
    int pos = dataset->num_examples-1;
    for(e=dataset->examples;e;e=e->previous) {
        if(last_example->num_inputs != e->num_inputs) {
            fprintf(stderr, "Bad configuration: row %d has %d inputs, row %d has %d.\n", t, last_example->num_inputs, 0, e->num_inputs);
            return false;
        }
        int s;
        for(s=0;s<e->num_inputs;s++) {
            if(last_example->inputs[s].type != e->inputs[s].type) {
                fprintf(stderr, "Bad configuration: item %d,%d is %d, item %d,%d is %d\n",
                         pos, s,            e->inputs[s].type,
                        last, s, last_example->inputs[s].type
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
    for(e=dataset->examples;e;e=e->previous) {
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
    example_t*e = dataset->examples;
    while(e) {
        example_t*prev = e->previous;
        example_destroy(e);
        e = prev;
    }
    free(dataset);
}

example_t**example_list_to_array(dataset_t*d)
{
    example_t**examples = (example_t**)malloc(sizeof(example_t*)*d->num_examples);
    int pos = d->num_examples;
    example_t*i = d->examples;
    while(i) {
        examples[--pos] = i;
        i = i->previous;
    }
    return examples;
}
column_t*column_new(int num_rows, bool is_categorical)
{
    column_t*c = malloc(sizeof(column_t)+sizeof(c->entries[0])*num_rows);
    c->is_categorical = is_categorical;
    return c;
}
void column_destroy(column_t*c)
{
    free(c);
}
void sanitized_dataset_destroy(sanitized_dataset_t*s)
{
    int t;
    for(t=0;t<s->num_columns;t++) {
        column_destroy(s->columns[t]);
    }
    free(s->desired_response);
    free(s->columns);
    if(s->wordmap) {
        wordmap_destroy(s->wordmap);
        s->wordmap = 0;
    }
    free(s);
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
void columnbuilder_add(columnbuilder_t*builder, constant_t*e)
{
    row_t*row = builder->row;
    int pos = 0;
    if(e->type == CONSTANT_STRING) {
        pos = dict_lookup(builder->string2category(e->s)) - 1;
    } else if(e->type == CONSTANT_INT) {
        pos = dict_lookup(builder->int2pos(e->s)) - 1;
    }
    if(pos<0) {
        pos = builder->row->num_categories++;
        if(builder->category_memsize <= pos) {
            builder->category_memsize++;
            builder->category_memsize*=2;
            row
        }
    }
/*
    columntype_t type = example->inputs[x].type;
    if(is_categorical) {
        assert(type == TEXT || type == CATEGORICAL);
        constant_t entry = variable_to_constant(&example->inputs[x]);
        category_t c = find_or_add_constant(wordmap, row, &entry);
        s->columns[x]->entries[y].c = c;
    } else {
        assert(type == CONTINUOUS);
        s->columns[x]->entries[y].f = example->inputs[x].value;
    }
    category_t c;
    if(type == TEXT) {
        c = wordmap_find_or_add_word(s->wordmap, example->desired_response.text);
    } else if(type == CATEGORICAL) {
        c = example->desired_response.category;
    } else {
        c = example->desired_response.value;
    }
    s->desired_response->entries[y].c = c;
    */
}
void columnbuilder_destroy(columnbuilder_t*builder)
{
    dict_destroy(builder->string2category);
    dict_destroy(builder->int2category);
    free(builder);
}

sanitized_dataset_t* dataset_sanitize(dataset_t*dataset)
{
    sanitized_dataset_t*s = malloc(sizeof(sanitized_dataset_t));

    if(!dataset_check_format(dataset))
        return 0;
    s->num_columns = dataset->examples->num_inputs;
    s->num_rows = dataset->num_examples;
    s->columns = malloc(sizeof(column_t)*s->num_columns);
    example_t*last_row = dataset->examples;

    /* copy columns from the old to the new dataset, mapping categories
       to numbers */
    int x;
    for(x=0;x<s->num_columns;x++) {
        bool is_categorical = ltype!=CONTINUOUS;
        s->columns[x] = column_new(s->num_rows, is_categorical);
        
        columnbuilder_t*builder = columnbuilder_new(s->columns[x]);
        int y;
        example_t*example = dataset->examples;
        for(y=s->num_rows-1;y>=0;y--) {
            columnbuilder_add(builder,y,variable_to_constant(&examples->inputs[x]));
            example = example->previous;
        }
        columnbuilder_destroy(builder);
    }
   
    /* copy response column to the new dataset */
    s->desired_response = column_new(s->num_rows, CATEGORICAL);
    columnbuilder_t*builder = columnbuilder_new(s->desired_response);
    example_t*example = dataset->examples;
    int y;
    for(y=s->num_rows-1;y>=0;y--) {
        columnbuilder_add(builder,y,variable_to_constant(&examples->inputs[x]));
        example = example->previous;
    }
    columnbuilder_destroy(builder);
    return s;
}
void sanitized_dataset_print(sanitized_dataset_t*s)
{
    int x,y;
    for(y=0;y<s->num_rows;y++) {
        for(x=0;x<s->num_columns;x++) {
            if(s->columns[x]->type == CATEGORICAL) {
                printf("C%d\t", s->columns[x]->entries[y].c);
            } else if(s->columns[x]->type == TEXT) {
                const char*text = wordmap_find_category(s->wordmap, s->columns[x]->entries[y].c);
                printf("\"%s\"\t", text);
            } else {
                printf("%.2f\t", s->columns[x]->entries[y].f);
            }
        }
        printf("| C%d", s->desired_response[y]);
        printf("\n");
    }
}
