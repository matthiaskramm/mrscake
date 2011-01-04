#include "model.h"

model_t* model_select(example_t**examples, int num_examples)
{
    return dtree_model_factory.train(examples, num_examples);
}
