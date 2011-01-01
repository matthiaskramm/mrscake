#include <stdio.h>
#include <stdlib.h>
#include "model.h"
#include "ast.h"

int main()
{
    example_t* examples[3];
    examples[0] = example_new(3);
    examples[0]->inputs[0] = variable_make_categorical(1);
    examples[0]->inputs[1] = variable_make_continuous(2);
    examples[0]->inputs[2] = variable_make_continuous(2);
    examples[0]->desired_output = 1;
    examples[1] = example_new(3);
    examples[1]->inputs[0] = variable_make_categorical(2);
    examples[1]->inputs[1] = variable_make_continuous(2);
    examples[1]->inputs[2] = variable_make_continuous(2);
    examples[1]->desired_output = 2;
    examples[2] = example_new(3);
    examples[2]->inputs[0] = variable_make_categorical(3);
    examples[2]->inputs[1] = variable_make_continuous(3);
    examples[2]->inputs[2] = variable_make_continuous(3);
    examples[2]->desired_output = 3;

    model_t*m = dtree_model_factory.train(examples, 3);
    node_t*n = m->code;
    node_print(n);
    model_destroy(m);

    example_destroy(examples[0]);
    example_destroy(examples[1]);
    example_destroy(examples[2]);
}
