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

#include "net.h"
#include "io.h"
#include "settings.h"
#include "job.h"
#include "test_datasets.h"
#include "var_selection.h"

static pid_t ppid;
static void signal_alarm(int signal)
{
    if(kill(ppid, 0)==0) {
        // parent process still alive, try again in one second
        alarm(1);
    } else {
        kill(getpid(), 9);
    }
}
static void check_for_parent()
{
    ppid = getppid();
    signal(SIGALRM, signal_alarm);
    alarm(1);
}

typedef enum {CLOSE_ON_OPEN, LEAVE_HANGING, GARBAGE_DATA} failure_mode_t;
char* failure_mode_to_string(failure_mode_t f)
{
    switch(f) {
        case CLOSE_ON_OPEN: return "close on open";
        case LEAVE_HANGING: return "leave hanging";
        case GARBAGE_DATA: return "garbage data";
    }
}
static void start_broken_server(int port, failure_mode_t failure_mode)
{
    struct sockaddr_in sin;
    int sock;
    int ret, i;
    int val = 1;
    memset(&sin, 0, sizeof(sin));
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);
    sin.sin_family = AF_INET;

    sock = socket(AF_INET, SOCK_STREAM, 6);
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof (val));
    bind(sock, (struct sockaddr*)&sin, sizeof(sin));
    listen(sock, 5);
    fcntl(sock, F_SETFL, O_NONBLOCK);

    sig_t old_sigpipe = signal(SIGPIPE, SIG_IGN);

    printf("listing on port %d (%s)\n", port, failure_mode_to_string(failure_mode));
    while(1) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sock, &fds);
        do {
            ret = select(sock + 1, &fds, 0, 0, 0);
        } while(ret == -1 && errno == EINTR);

        if(!FD_ISSET(sock, &fds))
            continue;
        struct sockaddr_in sin;
        int len = sizeof(sin);
        int newsock = accept(sock, (struct sockaddr*)&sin, &len);
        ret = fcntl(sock, F_SETFL, 0);
        switch(failure_mode) {
            case CLOSE_ON_OPEN:
                printf("closing socket prematurely\n");
                close(newsock);
            break;
            case LEAVE_HANGING:
                continue;
            break;
            case GARBAGE_DATA: {
                int j;
                for(j=0;j<1024;j++) {
                    char block[1024];
                    for(i=0;i<1024;i++) {
                        block[i] = lrand48();
                    }
                    write(newsock, block, sizeof(block));
                }
                close(newsock);
                break;
            }
            default:
                printf("Unknown failure mode\n");
            break;
        }
    }
}

static void clear_dir(int port)
{
    char rm[128];
    sprintf(rm, "rm -rf /tmp/mrscake%d/", port);
    system(rm);
}

static void add_server(int port)
{
    clear_dir(port);

    if(!fork()) {
        check_for_parent();
        char dir[128];
        sprintf(dir, "/tmp/mrscake%d/", port);
        config_dataset_cache_directory = dir;
        start_server(port);
        _exit(0);
    }
    config_add_remote_server("127.0.0.1", port);
}

static void add_broken_server(int port, failure_mode_t failure_mode)
{
    clear_dir(port);
    if(!fork()) {
        check_for_parent();
        start_broken_server(port, failure_mode);
        _exit(0);
    }
    config_add_remote_server("127.0.0.1", port);
}

void distribute_jobs_to_servers(dataset_t*dataset, jobqueue_t*jobs, server_array_t*servers);
jobqueue_t* generate_jobs(varorder_t*order, dataset_t*data, const char*model_name);

int main()
{
    add_broken_server(8800, GARBAGE_DATA);
    add_server(8801);
    add_broken_server(8802, LEAVE_HANGING);
    add_broken_server(8803, CLOSE_ON_OPEN);
    add_server(8804);
    add_server(8805);
    add_broken_server(8806, LEAVE_HANGING);

    usleep(100);

    dataset_t*dataset = dataset1(2, 4);
    int num_servers = 0;
    server_array_t*servers = distribute_dataset(dataset);
    int i;
    usleep(100);
    int*ok = calloc(sizeof(int),num_servers);
    for(i=0;i<servers->num;i++) {
        dataset_t*d1 = dataset_read_from_server(servers->servers[i]->host, servers->servers[i]->port, dataset->hash);

        if(d1 && !memcmp(d1->hash, dataset->hash, HASH_SIZE)) {
            ok[i] = 1;
        }
        if(d1) {
            dataset_destroy(d1);
        }
    }
    usleep(100);

    printf("\n");
    config_print_remote_servers();
    printf("\n");
    printf("* working hosts:\n");
    for(i=0;i<servers->num;i++) {
        if(ok[i]) {
            printf(" OK ");
        } else {
            printf("ERR ");
        }
        remote_server_print(servers->servers[i]);
    }

    printf("---------------------------------------\n");
    jobqueue_t*jobs = generate_jobs(NULL, dataset, NULL);
    distribute_jobs_to_servers(dataset, jobs, servers);
}


