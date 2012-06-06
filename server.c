#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "mrscake.h"
#include "dataset.h"
#include "net.h"

int main(int argn, char*argv[])
{
    int port = 3075;
    if(argn > 1)
        port = atoi(argv[1]);
    start_server(port);
    return 0;
}
