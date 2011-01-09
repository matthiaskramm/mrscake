#ifndef __model_select_h__
#define  __model_select_h__

typedef struct _model_factory {
    const char*name;
    model_t*(*train)(dataset_t*dataset);
    void*internal;
} model_factory_t;

extern model_factory_t dtree_model_factory;
extern model_factory_t ann_model_factory;

model_t* model_select(dataset_t*);
#endif
