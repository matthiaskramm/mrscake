/* server.c
   model training server.

   Part of the data prediction package.
   
   Copyright (c) 2012 Matthias Kramm <kramm@quiss.org> 
 
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
#include <sys/wait.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#ifdef HAVE_SYS_TIMEB
#include <sys/timeb.h>
#endif
#include "net/distribute.h"
#include "net/protocol.h"

typedef struct _worker {
    pid_t pid;
    int start_time;
} worker_t;

typedef struct _server {
    worker_t* jobs;
    int num_workers;
    datacache_t*datacache;
} server_t;

static volatile server_t server;
static sigset_t sigchld_set;

static void sigchild(int signal)
{
    sigprocmask(SIG_BLOCK, &sigchld_set, 0);
    while(1) {
        int status;
        pid_t pid = waitpid(-1, &status, WNOHANG);
        if(pid<=0)
            break;

        int i;
        for(i=0;i<server.num_workers;i++) {
            if(pid == server.jobs[i].pid) {
                printf("worker %d: finished: %s %d\n", pid,
                        WIFEXITED(status)?"exit": (WIFSIGNALED(status)?"signal": "abnormal"),
                        WIFEXITED(status)? WEXITSTATUS(status):WTERMSIG(status)
                        );
                server.jobs[i] = server.jobs[--server.num_workers];
                break;
            }
        }
    }
    sigprocmask(SIG_UNBLOCK, &sigchld_set, 0);
}

static void worker_timeout_signal(int signal)
{
    kill(getpid(), 9);
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
    ret = listen(sock, 256);
    if(ret<0) {
        perror("listen");
        exit(1);
    }
    ret = fcntl(sock, F_SETFL, O_NONBLOCK);
    if(ret<0) {
        perror("fcntl");
        exit(1);
    }

    server.jobs = malloc(sizeof(worker_t)*config_number_of_remote_workers);
    server.num_workers = 0;
    server.datacache = datacache_new();

    sigemptyset(&sigchld_set);
    sigaddset(&sigchld_set, SIGCHLD);

    signal(SIGCHLD, sigchild);

    printf("listing on port %d\n", port);
    while(1) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(sock, &fds);

        do {
            ret = select(sock + 1, &fds, 0, 0, 0);
        } while(ret == -1 && errno == EINTR);
        if(ret<0) {
            perror("select");
            exit(1);
        }
        if(!FD_ISSET(sock, &fds))
            continue;

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

        bool accept_request = (server.num_workers < config_number_of_remote_workers);
        ret = send_header(newsock, accept_request, server.num_workers, config_number_of_remote_workers);
        if(ret<0) {
            close(newsock);
            continue;
        }

        /* Wait for a free worker to become available. Only
           after we have a worker will we actually read the 
           job data. TODO: would it be better to just close
           the connection here and have the server decide what to
           do with the job now? */
        if(!accept_request) {
            close(newsock);
            continue;
        }

        /* block child signals while we're modifying num_workers / jobs */
        sigprocmask(SIG_BLOCK, &sigchld_set, 0);

        pid_t pid = fork();
        if(!pid) {
            sigprocmask(SIG_UNBLOCK, &sigchld_set, 0);
            signal(SIGALRM, worker_timeout_signal);
            alarm(config_remote_worker_timeout);
            process_request(server.datacache, newsock);
            printf("worker %d: closing socket\n", getpid());
            close(newsock);
            _exit(0);
        }
        server.jobs[server.num_workers].pid = pid;
        server.jobs[server.num_workers].start_time = time(0);
        server.num_workers++;

        sigprocmask(SIG_UNBLOCK, &sigchld_set, 0);

        close(newsock);
    }
}
