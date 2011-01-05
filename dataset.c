#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
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
void dataset_check_format(dataset_t*dataset)
{
    int t;
    example_t*last_example = dataset->examples;
    example_t*e;
    int last = dataset->num_examples-1;
    int pos = dataset->num_examples-1;
    for(e=dataset->examples;e;e=e->previous) {
        if(last_example->num_inputs != e->num_inputs) {
            fprintf(stderr, "Bad configuration: row %d has %d inputs, row %d has %d.\n", t, last_example->num_inputs, 0, e->num_inputs);
        }
        int s;
        for(s=0;s<e->num_inputs;s++) {
            if(last_example->inputs[s].type != e->inputs[s].type)
                fprintf(stderr, "Bad configuration: item %d,%d is %d, item %d,%d is %d\n",
                         pos, s,            e->inputs[s].type,
                        last, s, last_example->inputs[s].type
                        );
        }
        pos--;
    }
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
                printf("\"%s\"\t", v.value);
            }
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
sanitized_dataset_t* dataset_sanitize(dataset_t*dataset)
{
    return 0;
}

