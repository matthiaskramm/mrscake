#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "net.h"
#include "settings.h"
#include "test_datasets.h"

int main()
{
    if(!fork()) {
        config_dataset_cache_directory = "/tmp/mrscake1";
        start_server(8800);
        _exit(0);
    }
    if(!fork()) {
        config_dataset_cache_directory = "/tmp/mrscake2";
        start_server(8801);
        _exit(0);
    }

    config_add_remote_server("127.0.0.1", 8800);
    config_add_remote_server("127.0.0.1", 8801);

    dataset_t*dataset = dataset1(2, 4);
    int num_servers;
    distribute_dataset(dataset, &num_servers);
}


