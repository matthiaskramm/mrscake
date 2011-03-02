/* net.c
   model training client/server.

   Part of the data prediction package.
   
   Copyright (c) 2011 Matthias Kramm <kramm@quiss.org> 
 
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <memory.h>
#include "io.h"
#include "net.h"
#include "dataset.h"
#include "model_select.h"
#include "serialize.h"
#include "settings.h"

#define TIME_LIMIT 3600
#define NUM_WORKERS 32

typedef struct _job {
    pid_t pid;
    int start_time;
} job_t;

void clean_old_workers(job_t*jobs, int*num)
{
    int t;
    for(t=0;t<*num;t++) {
        int status;
        pid_t p = waitpid(jobs[t].pid, &status, WNOHANG);
        if(p == jobs[t].pid) {
            printf("worker %d: finished\n", jobs[t].pid);
            jobs[t] = jobs[--(*num)];
        }

        if(time(0) - jobs[t].start_time > TIME_LIMIT) {
            printf("killing worker %d\n", jobs[t].pid);
            kill(jobs[t].pid, 9);
            jobs[t] = jobs[--(*num)];
        }
    }
}

void process_request(int socket)
{
    reader_t*r = filereader_new(socket);

    char*name = read_string(r);
    printf("worker %d: processing model %s\n", getpid(), name);
    model_factory_t* factory = model_factory_get_by_name(name);
    if(!factory) {
        printf("worker %d: unknown factory '%s'\n", getpid(), name);
        free(name);
        return;
    }
    free(name);

    sanitized_dataset_t* dataset = sanitized_dataset_read(r);
    printf("worker %d: %d rows of data\n", getpid(), dataset->num_rows);

    model_t*m = train_model(factory, dataset);
    if(m) {
        m->name = factory->name;
    }

    printf("worker %d: writing out model data\n", getpid());
    writer_t*w = filewriter_new(socket);
    model_write(m, w);
    w->finish(w);
}

int start_server(int port)
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
    if(sock<0) {
        perror("socket");
        exit(1);
    }
    ret = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof (val));
    if(ret<0) {
        perror("setsockopt");
        exit(1);
    }
    ret = bind(sock, (struct sockaddr*)&sin, sizeof(sin));
    if(ret<0) {
        perror("bind");
        exit(1);
    }
    ret = listen(sock, 5);
    if(ret<0) {
        perror("listen");
        exit(1);
    }
    ret = fcntl(sock, F_SETFL, O_NONBLOCK);
    if(ret<0) {
        perror("fcntl");
        exit(1);
    }

    job_t jobs[NUM_WORKERS];
    int num_workers = 0;

    printf("listing on port %d\n", port);
    while(1) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sock, &fds);

        ret = select(sock + 1, &fds, 0, 0, 0);
        if(ret<0) {
            perror("listen");
            exit(1);
        }
        if(FD_ISSET(sock, &fds)) {
            struct sockaddr_in sin;
            int len = sizeof(sin);

            int newsock = accept(sock, (struct sockaddr*)&sin, &len);
            if(newsock < 0) {
                perror("accept");
                exit(1);
            }

            // clear O_NONBLOCK
            ret = fcntl(sock, F_SETFL, 0);
            if(ret<0) {
                perror("fcntl");
                exit(1);
            }

            char buf[128];
            while(num_workers == NUM_WORKERS) {
                clean_old_workers(jobs, &num_workers);
                if(num_workers == NUM_WORKERS) {
                    sleep(1);
                }
            }

            pid_t pid = fork();
            if(!pid) {
                process_request(newsock);
                printf("worker %d: close\n", getpid());
                close(newsock);
                _exit(0);
            }
            jobs[num_workers].pid = pid;
            jobs[num_workers].start_time = time(0);
            num_workers++;

            close(newsock);
        }
    }
}

int connect_to_host(const char *host, int port)
{
    int i, ret;
    char buf_ip[100];
    struct sockaddr_in sin;

    struct hostent *he = gethostbyname(host);
    if(!he) {
        printf("gethostbyname returned %d\n", h_errno);
        herror(host);
        return -1;
    }

    unsigned char*ip = he->h_addr_list[0];
    printf("Connecting to %d.%d.%d.%d:%d...\n",
            ip[0], ip[1], ip[2], ip[3], port);

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    memcpy(&sin.sin_addr.s_addr, ip, 4);

    int sock = socket(AF_INET, SOCK_STREAM, 6);
    if(sock < 0) {
        perror("socket");
        return -1;
    }

    ret = connect(sock, (struct sockaddr*)&sin, sizeof(struct sockaddr_in));
    if(ret < 0) {
        perror("connect");
        return -1;
    }
    return sock;
}

remote_job_t* remote_job_start(const char*model_name, sanitized_dataset_t*dataset)
{
    int sock;
    while(1) {
        if(!config_num_remote_servers) {
            fprintf(stderr, "No remote servers configured.\n");
            exit(1);
        }
        remote_server_t*s = &config_remote_servers[lrand48()%config_num_remote_servers];
        sock = connect_to_host(s->host, s->port);
        if(sock>=0) {
            break;
        }
        sleep(1);
    }

    writer_t*w = filewriter_new(sock);
    write_string(w, model_name);
    sanitized_dataset_write(dataset, w);
    w->finish(w);

    remote_job_t*j = malloc(sizeof(remote_job_t));
    j->socket = sock;
    j->start_time = time(0);
    return j;
}

bool remote_job_is_ready(remote_job_t*j)
{
    struct timeval timeout;
    memset(&timeout, 0, sizeof(timeout));
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(j->socket, &readfds);
    while(1) {
        int ret = select(j->socket+1, &readfds, NULL, NULL, &timeout);
        if(ret<0 && (errno == EINTR || errno == EAGAIN))
            continue;
        break;
    }
    return !!FD_ISSET(j->socket, &readfds);
}

model_t* remote_job_read_result(remote_job_t*j)
{
    reader_t*r = filereader_with_timeout_new(j->socket, config_remote_read_timeout);
    model_t*m = model_read(r);
    r->dealloc(r);
    free(j);
    return m;
}

void remote_job_cancel(remote_job_t*j)
{
    close(j->socket);
    free(j);
}

time_t remote_job_age(remote_job_t*j)
{
    return time(0) - j->start_time;
}

model_t* process_job_remotely(const char*model_name, sanitized_dataset_t*dataset)
{
    remote_job_t*j = remote_job_start(model_name, dataset);
    return remote_job_read_result(j);
}
