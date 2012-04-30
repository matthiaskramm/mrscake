#include "test_datasets.h"

trainingdata_t* trainingdata1(int width, int height)
{
    trainingdata_t* data = trainingdata_new();

    int t;
    for(t=0;t<height;t++) {
        example_t*e = example_new(width);
        int s;
        for(s=0;s<width;s++) {
            e->inputs[s] = variable_new_continuous(lrand48()&255);
        }
        if(width>2)
            e->inputs[2] = variable_new_continuous(((t%2)+1)*100+(lrand48()&15));
        e->desired_response = variable_new_categorical(t&1);
        trainingdata_add_example(data, e);
    }
    return data;
}

trainingdata_t* trainingdata2(int width, int height)
{
    trainingdata_t* data = trainingdata_new();

    int t;
    for(t=0;t<height;t++) {
        example_t*e = example_new(width);
        int s;
        for(s=0;s<width;s++) {
            e->inputs[s] = variable_new_continuous(lrand48()&255);
        }
        if(width>2)
            e->inputs[2] = variable_new_continuous(((t%2)+1)*100+(lrand48()&15));
        if(width>3)
            e->inputs[3] = variable_new_continuous((((t/2)%2)+1)*100+(lrand48()&15));
        e->desired_response = variable_new_categorical(t&3);
        trainingdata_add_example(data, e);
    }
    return data;
}

dataset_t* dataset1(int width, int height)
{
    trainingdata_t*t1 = trainingdata1(width, height);
    return trainingdata_sanitize(t1);
}

dataset_t* dataset2(int width, int height)
{
    trainingdata_t*t2 = trainingdata2(width, height);
    return trainingdata_sanitize(t2);
}

