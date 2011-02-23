/* server.c
   model training server.

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
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <memory.h>
#include "io.h"
#include "dataset.h"
#include "model_select.h"
#include "serialize.h"

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
        if(time(0) - jobs[t].start_time > TIME_LIMIT) {
            kill(jobs[t].pid, 9);
            jobs[t] = jobs[--(*num)];
        }
    }
}

void process_request(int socket)
{
    reader_t*r = filereader_new(socket);
    char*name = read_string(r);
    sanitized_dataset_t* dataset = sanitized_dataset_read(r);
    model_factory_t* factory = model_factory_get_by_name(name);
    free(name);

    model_t*m = factory->train(factory, dataset);
    if(m) {
        m->name = factory->name;
    }

    writer_t*w = filewriter_new(socket);
    model_write(m, w);
    w->finish(w);
}

int main()
{
    int port = 3075;

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

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(sock, &fds);

    job_t jobs[NUM_WORKERS];
    int num_workers = 0;

    while(1) {
        ret = select(sock + 1, &fds, 0, 0, 0);
        printf("%d\n");
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

            clean_old_workers(jobs, &num_workers);

            pid_t pid;
            pid = fork();

            jobs[num_workers].pid = pid;
            jobs[num_workers].start_time = time(0);
            num_workers++;

            if(!pid) {
                process_request(newsock);
                close(newsock);
                _exit(0);
            }
            close(newsock);
        }
    }
}
