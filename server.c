#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "mrscake.h"
#include "dataset.h"
#include "net.h"

int main()
{
    int port = 3075;
    start_server(port);
    return 0;
}
