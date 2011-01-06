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
        if(e->desired_output.type == TEXT) {
            printf("|\t\"%s\"", e->desired_output.text);
        } else {
            printf("|\tC%d", e->desired_output.category);
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
column_t*column_new(int num_rows, columntype_t type)
{
    column_t*c = malloc(sizeof(column_t)+sizeof(c->entries[0])*num_rows);
    c->type = type;
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
    free(s->desired_output);
    free(s->columns);
    if(s->wordmap) {
        wordmap_destroy(s->wordmap);
        s->wordmap = 0;
    }
    free(s);
}
sanitized_dataset_t* dataset_sanitize(dataset_t*dataset)
{
    sanitized_dataset_t*s = malloc(sizeof(sanitized_dataset_t));

    if(!dataset_check_format(dataset))
        return 0;
    s->num_columns = dataset->examples->num_inputs;
    s->num_rows = dataset->num_examples;
    s->wordmap = wordmap_new();
    s->columns = malloc(sizeof(column_t)*s->num_columns);
    s->desired_output = malloc(sizeof(category_t)*s->num_rows);
    example_t*last_row = dataset->examples;
    int x;
    for(x=0;x<s->num_columns;x++) {
        columntype_t ltype = last_row->inputs[x].type;
        /* FIXME: deal with MISSING */
        s->columns[x] = column_new(s->num_rows, ltype);
        int y;
        example_t*example = dataset->examples;
        for(y=s->num_rows-1;y>=0;y--) {
            category_t c;
            columntype_t type = example->inputs[x].type;
            if(ltype == TEXT || ltype == CATEGORICAL) {
                assert(type == TEXT || type == CATEGORICAL);
                assert(type == ltype);
                if(type == TEXT) {
                    c = wordmap_find_or_add_word(s->wordmap, example->inputs[x].text);
                } else {
                    c = example->inputs[x].category;
                }
                s->columns[x]->entries[y].c = c;
            } else {
                assert(type == CONTINUOUS);
                s->columns[x]->entries[y].f = example->inputs[x].value;
            }
            example = example->previous;
        }
    }
    example_t*example = dataset->examples;
    int y;
    for(y=s->num_rows-1;y>=0;y--) {
        columntype_t type = example->desired_output.type;
        category_t c;
        if(type == TEXT) {
            c = wordmap_find_or_add_word(s->wordmap, example->desired_output.text);
            s->output_is_text = 1;
        } else if(type == CATEGORICAL) {
            c = example->desired_output.category;
        } else {
            c = example->desired_output.value;
        }
        s->desired_output[y] = c;
        example = example->previous;
    }
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
        printf("| C%d", s->desired_output[y]);
        printf("\n");
    }
}
