#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <sys/timeb.h>

#include "config.h"
#include "../net/protocol.h"
#include "io.h"
#include "settings.h"
#include "job.h"
#include "test_datasets.h"
#include "var_selection.h"

jobqueue_t* generate_uniform_jobs(dataset_t*data)
{
    jobqueue_t* queue = jobqueue_new();
    int i;
    model_factory_t*factory = model_factory_get_by_name("knearest_1");
    for(i=0;i<128;i++) {
        job_t* job = job_new();
        job->factory = factory;
        job->code = NULL;
        job->data = data;
        job->transforms = NULL;
        jobqueue_append(queue,job);
    }
    return queue;
}

int main(int argn, char*argv[])
{
    config_parse_remote_servers("../servers.txt");

    dataset_t*dataset = dataset1(32, 1250);

#ifdef HAVE_SYS_TIMEB
    struct timeb start_ftime,distribute_done_ftime,end_ftime;
    ftime(&start_ftime);
    double start_time = start_ftime.time + start_ftime.millitm/1000.0;
#endif

    server_array_t*servers = distribute_dataset(dataset);

#ifdef HAVE_SYS_TIMEB
    ftime(&distribute_done_ftime);
    double distribute_done_time = distribute_done_ftime.time + distribute_done_ftime.millitm/1000.0;
#endif

    usleep(100);
    printf("\n");
    config_print_remote_servers();
    printf("\n");

    printf("---------------------------------------\n");
    jobqueue_t*jobs = generate_uniform_jobs(dataset);
    distribute_jobs_to_servers(dataset, jobs, servers);

#ifdef HAVE_SYS_TIMEB
    ftime(&end_ftime);
    double end_time = end_ftime.time + end_ftime.millitm/1000.0;
    printf("total time: %.2f\n", end_time - start_time);
#endif
}


