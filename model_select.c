#include "model.h"

model_t* model_select(dataset_t*dataset)
{
    return dtree_model_factory.train(dataset);
}
