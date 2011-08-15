#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "mrscake.h"
#include "dataset.h"
#include "net.h"

void test()
{
    trainingdata_t* data = trainingdata_new();
    int t;
    for(t=0;t<256;t++) {
        example_t*e = example_new(16);
        int s;
        for(s=0;s<16;s++) {
            e->inputs[s] = variable_new_continuous((lrand48()%256)/256.0);
        }
        e->desired_response = variable_new_categorical(t%2);
        trainingdata_add_example(data, e);
    }

    dataset_t*dataset = dataset_sanitize(data);
    model_t*m = process_job_remotely("dtree", dataset);
    if(m) {
        printf("%s\n", m->name);
    }
}

int main()
{
    int port = 3075;
    if(fork()) {
        start_server(port);
        _exit(0);
    }
    int t;
    for(t=0;t<32*2;t++) {
        test();
    }
}
