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
#include <signal.h>
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <memory.h>
#include <sys/timeb.h>
#include "io.h"
#include "net.h"
#include "dataset.h"
#include "datacache.h"
#include "model_select.h"
#include "serialize.h"
#include "settings.h"
#include "job.h"
#include "util.h"

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

static void make_request_TRAIN_MODEL(writer_t*w, const char*model_name, const char*transforms, dataset_t*dataset)
{
    write_uint8(w, REQUEST_TRAIN_MODEL);
    w->write(w, dataset->hash, HASH_SIZE);
    if(w->error)
        return;
    write_string(w, model_name);
    write_string(w, transforms);
}
static void process_request_TRAIN_MODEL(datacache_t*cache, reader_t*r, writer_t*w)
{
    uint8_t hash[HASH_SIZE];
    r->read(r, &hash, HASH_SIZE);
    if(r->error)
        return;

    dataset_t*dataset = datacache_find(cache, hash);
    if(!dataset) {
        write_uint8(w, RESPONSE_DATASET_UNKNOWN);
        return;
    }

    char*name = read_string(r);
    char*transforms = read_string(r);
    if(r->error)
        return;

    printf("worker %d: processing model %s|%s\n", getpid(), transforms, name);
    model_factory_t* factory = model_factory_get_by_name(name);
    if(!factory) {
        printf("worker %d: unknown factory '%s'\n", getpid(), name);
        write_uint8(w, RESPONSE_FACTORY_UNKNOWN);
        return;
    }

    printf("worker %d: %d rows of data\n", getpid(), dataset->num_rows);
    job_t j;
    memset(&j, 0, sizeof(j));
    j.factory = factory;
    j.data = dataset;
    j.code = 0;
    j.transforms = transforms;
    j.flags = JOB_NO_FORK;
    job_process(&j);

    printf("worker %d: writing out model data\n", getpid());
    write_uint8(w, RESPONSE_OK);
    write_compressed_int(w, j.score);
    node_write(j.code, w, SERIALIZE_DEFAULTS);
}

static dataset_t* make_request_SEND_DATASET(reader_t*r, writer_t*w, uint8_t*hash)
{
    write_uint8(w, REQUEST_SEND_DATASET);
    w->write(w, hash, HASH_SIZE);
    uint8_t response = read_uint8(r);
    if(response!=RESPONSE_OK)
        return NULL;
    return dataset_read(r);
}
static void process_request_SEND_DATASET(datacache_t*datacache, reader_t*r, writer_t*w)
{
    uint8_t hash[HASH_SIZE];
    r->read(r, &hash, HASH_SIZE);
    if(r->error)
        return;
    char*hashstr = hash_to_string(hash);

    dataset_t*dataset = datacache_find(datacache, hash);
    if(!dataset) {
        printf("worker %d: dataset unknown\n", getpid());
        write_uint8(w, RESPONSE_DATASET_UNKNOWN);
        return;
    }
    printf("worker %d: sending out dataset %s\n", getpid(), hashstr);
    write_uint8(w, RESPONSE_OK);
    dataset_write(dataset, w);
}

static void make_request_RECV_DATASET(reader_t*r, writer_t*w, dataset_t*dataset, remote_server_t*other_server)
{
    write_uint8(w, REQUEST_RECV_DATASET);
    if(w->error)
        return;
    w->write(w, dataset->hash, HASH_SIZE);
    if(w->error)
        return;

    uint8_t status = read_uint8(r);
    if(status != RESPONSE_GO_AHEAD)
        return;

    if(other_server) {
        write_string(w, other_server->host);
        write_compressed_uint(w, other_server->port);
    } else {
        write_string(w, "");
        write_compressed_uint(w, 0);
        dataset_write(dataset, w);
    }
}
static void process_request_RECV_DATASET(datacache_t*datacache, reader_t*r, writer_t*w)
{
    uint8_t hash[HASH_SIZE];
    r->read(r, &hash, HASH_SIZE);
    if(r->error)
        return;
    char*hashstr = hash_to_string(hash);
    printf("worker %d: reading dataset %s\n", getpid(), hashstr);

    dataset_t*dataset = datacache_find(datacache, hash);
    if(dataset!=NULL) {
        printf("worker %d: dataset already known\n", getpid());
        write_uint8(w, RESPONSE_DUPL_DATA);
        w->write(w, dataset->hash, HASH_SIZE);
        write_uint8(w, RESPONSE_DUPL_DATA);
        return;
    } else {
        write_uint8(w, RESPONSE_GO_AHEAD);
    }

    char*host = read_string(r);
    int port = read_compressed_uint(r);
    if(!*host) {
        dataset = dataset_read(r);
        if(r->error)
            return;
    } else {
        dataset = dataset_read_from_server(host, port, hash);
    }
    if(!dataset) {
        w->write(w, hash, HASH_SIZE);
        write_uint8(w, RESPONSE_DATA_ERROR);
        return;
    }
    if(memcmp(dataset->hash, hash, HASH_SIZE)) {
        printf("worker %d: dataset has bad hash\n", getpid());
        w->write(w, hash, HASH_SIZE);
        write_uint8(w, RESPONSE_DATA_ERROR);
        return;
    }
    datacache_store(datacache, dataset);
    w->write(w, dataset->hash, HASH_SIZE);
    write_uint8(w, RESPONSE_OK);
    printf("worker %d: dataset stored\n", getpid());
}

dataset_t* dataset_read_from_server(const char*host, int port, uint8_t*hash)
{
    int sock = connect_to_host(host, port);
    if(sock<0)
        return NULL;

    writer_t*w = filewriter_new(sock);
    reader_t*r = filereader_with_timeout_new(sock, config_remote_read_timeout);

    dataset_t*dataset = make_request_SEND_DATASET(r, w, hash);
    if(r->error)
        dataset = NULL;

    w->finish(w);
    r->dealloc(r);

    return dataset;
}

static void process_request(datacache_t*cache, int socket)
{
    reader_t*r = filereader_new(socket);
    writer_t*w = filewriter_new(socket);

    uint8_t request_code = read_uint8(r);

    switch(request_code) {
        case REQUEST_TRAIN_MODEL:
            process_request_TRAIN_MODEL(cache, r, w);
        break;
        case REQUEST_RECV_DATASET:
            process_request_RECV_DATASET(cache, r, w);
        break;
        case REQUEST_SEND_DATASET:
            process_request_SEND_DATASET(cache, r, w);
        break;
    }
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

        /* Wait for a free worker to become available. Only
           after we have a worker will we actually read the 
           job data. TODO: would it be better to just close
           the connection here and have the server decide what to
           do with the job now? */
        while(server.num_workers >= config_number_of_remote_workers) {
            printf("Wait for free worker (%d/%d)\n", server.num_workers, config_number_of_remote_workers);
            sleep(1);
        }

        /* block child signals while we're modifying num_workers / jobs */
        sigprocmask(SIG_BLOCK, &sigchld_set, 0);

        pid_t pid = fork();
        if(!pid) {
            sigprocmask(SIG_UNBLOCK, &sigchld_set, 0);
            signal(SIGALRM, worker_timeout_signal);
            alarm(config_remote_worker_timeout);
            process_request(server.datacache, newsock);
            printf("pid %d: closing socket\n", pid);
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

int connect_to_remote_server(remote_server_t*server)
{
    int i, ret;
    char buf_ip[100];
    struct sockaddr_in sin;

    struct hostent *he = gethostbyname(server->host);
    if(!he) {
        fprintf(stderr, "gethostbyname returned %d\n", h_errno);
        herror(server->host);
        remote_server_is_broken(server, hstrerror(h_errno));
        return -1;
    }

    unsigned char*ip = he->h_addr_list[0];
    //printf("Connecting to %d.%d.%d.%d:%d...\n", ip[0], ip[1], ip[2], ip[3], port);

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(server->port);
    memcpy(&sin.sin_addr.s_addr, ip, 4);

    int sock = socket(AF_INET, SOCK_STREAM, 6);
    if(sock < 0) {
        perror("socket");
        remote_server_is_broken(server, strerror(errno));
        return -1;
    }

    /* FIXME: connect has a very long timeout */
    ret = connect(sock, (struct sockaddr*)&sin, sizeof(struct sockaddr_in));
    if(ret < 0) {
        perror("connect");
        remote_server_is_broken(server, strerror(errno));
        return -1;
    }
    return sock;
}

int connect_to_host(const char*host, int port)
{
    remote_server_t dummy;
    dummy.host = host;
    dummy.port = port;
    dummy.broken = NULL;
    return connect_to_remote_server(&dummy);
}

static int send_dataset_to_remote_server(remote_server_t*server, dataset_t*data, remote_server_t*from_server)
{
    int sock = connect_to_remote_server(server);
    if(sock<0) {
        return RESPONSE_READ_ERROR;
    }
    writer_t*w = filewriter_new(sock);
    reader_t*r = filereader_with_timeout_new(sock, config_remote_read_timeout);
    make_request_RECV_DATASET(r, w, data, from_server);

    char hash[HASH_SIZE];
    r->read(r, hash, HASH_SIZE);
    int resp = read_uint8(r);
    if(r->error) {
        remote_server_is_broken(server, "read error after RECV_DATASET");
        resp = RESPONSE_READ_ERROR;
    } else if(memcmp(hash, data->hash, HASH_SIZE)) {
        remote_server_is_broken(server, "bad data checksum after RECV_DATASET");
        resp = RESPONSE_DATA_ERROR;
    }

    w->finish(w);
    r->dealloc(r);
    close(sock);

    return resp;
}

void server_array_destroy(server_array_t*a)
{
    if(a) {
        if(a->servers)
            free(a->servers);
        free(a);
    }
}

server_array_t* distribute_dataset(dataset_t*data)
{
    /* write returns an error, instead of raising a signal */
    sig_t old_sigpipe = signal(SIGPIPE, SIG_IGN);

    int*status = calloc(sizeof(int), config_num_remote_servers);

    remote_server_t**seeds = calloc(sizeof(remote_server_t), config_num_remote_servers);

    /* send dataset to "seeded" nodes */
    printf("seeding %d/%d hosts...\n", config_num_seeded_hosts, config_num_remote_servers);
    int i;
    int num_errors = 0;
    int num_seeds = 0;
    int hosts_to_seed = imin(config_num_seeded_hosts, config_num_remote_servers);
    while(num_seeds < hosts_to_seed) {
        if(num_seeds + num_errors == config_num_remote_servers) {
            printf("error seeding %d/%d hosts: %d errors\n", hosts_to_seed-num_seeds, hosts_to_seed, num_errors);
            goto error;
        }

        int seed_nr;
        while(1) {
            seed_nr = lrand48() % config_num_remote_servers;
            if(!status[seed_nr])
                break;
        }
        remote_server_t*server = &config_remote_servers[seed_nr];
        printf("trying to seed host %s...\n", server->name);
        int resp = send_dataset_to_remote_server(server, data, NULL);
        switch(resp) {
            case RESPONSE_DUPL_DATA:
            case RESPONSE_OK:
                printf("seeded host %s%s\n", server->name, resp==RESPONSE_DUPL_DATA?" (cached)":"");
                status[seed_nr] = 1;
                seeds[num_seeds++] = server;
            break;
            default:
            case RESPONSE_READ_ERROR:
            case RESPONSE_DATA_ERROR:
                printf("error seeding host %s (%d)\n", server->name, resp);
                status[seed_nr] = -1;
                num_errors++;
                usleep(100);
            break;
        }
    }

    /* make nodes interchange the dataset */
    for(i=0;i<config_num_remote_servers;i++) {
        if(status[i]) {
            continue;
        }
        remote_server_t*server = &config_remote_servers[i];
        int seed_nr = lrand48() % num_seeds;
        remote_server_t*other_server = seeds[seed_nr];
        printf("sending dataset from host %s to host %s\n", other_server->name, server->name);
        int resp = send_dataset_to_remote_server(server, data, other_server);
        switch(resp) {
            case RESPONSE_DUPL_DATA:
            case RESPONSE_OK:
                printf("%s: received dataset\n", server->name, other_server->host);
                status[seed_nr] = 1;
                seeds[num_seeds++] = server;
            break;
            case RESPONSE_READ_ERROR:
            case RESPONSE_DATA_ERROR:
                printf("%s: error sending dataset from host %s\n", server->name, other_server->name);
                status[seed_nr] = -1;
            break;
        }
    }

    server_array_t*server_array = calloc(sizeof(server_array_t),1);
    server_array->servers = seeds;
    server_array->num = num_seeds;
    return server_array;
error:
    signal(SIGPIPE, old_sigpipe);
    return NULL;
}

remote_job_t* remote_job_start(job_t*job, const char*model_name, const char*transforms, dataset_t*dataset, server_array_t*servers)
{
    remote_job_t*j = calloc(1, sizeof(remote_job_t));
    j->job = job;
    j->start_time = time(0);

    ftime(&j->profile_time[0]);
    int sock;
    while(1) {
        if(!config_num_remote_servers) {
            fprintf(stderr, "No remote servers configured.\n");
            exit(1);
        }
        if(!config_has_remote_servers()) {
            fprintf(stderr, "No remote servers available.\n");
            exit(1);
        }
        static int round_robin = 0;
        remote_server_t*s = servers->servers[(round_robin++)%servers->num];
        sock = connect_to_remote_server(s);
        if(sock>=0) {
            printf("Starting %s on %s\n", model_name, s->name);fflush(stdout);
            break;
        }
        usleep(10000);
    }
    ftime(&j->profile_time[1]);

    writer_t*w = filewriter_new(sock);
    make_request_TRAIN_MODEL(w, model_name, transforms, dataset);
    w->finish(w);

    ftime(&j->profile_time[2]);

    j->socket = sock;
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

void remote_job_read_result(remote_job_t*j, job_t*dest)
{
    reader_t*r = filereader_with_timeout_new(j->socket, config_remote_read_timeout);

    j->response = read_uint8(r);
    if(j->response != RESPONSE_OK) {
        dest->score = INT32_MAX;
        dest->code = NULL;
    } else {
        dest->score = read_compressed_int(r);
        dest->code = node_read(r);
    }
    r->dealloc(r);
}

void remote_job_cancel(remote_job_t*j)
{
    close(j->socket);
}

time_t remote_job_age(remote_job_t*j)
{
    return time(0) - j->start_time;
}

static void store_times(remote_job_t*j, int nr)
{
    char filename[80];
    sprintf(filename, "job%d.txt", nr);
    FILE*fi = fopen(filename, "wb");
    int i;
    for(i=0;i<5;i++) {
        struct timeb*t = &j->profile_time[i];
        fprintf(fi, "%f\n", t->time+t->millitm/1000.0);
    }
    fclose(fi);
}

void distribute_jobs_to_servers(dataset_t*dataset, jobqueue_t*jobs, server_array_t*servers)
{
    remote_job_t**r = malloc(sizeof(reader_t*)*jobs->num);
    /* Ignore sigpipe events. Write calls to closed network sockets 
       will now only return an error, not halt the program */
    sig_t old_sigpipe = signal(SIGPIPE, SIG_IGN);

    /* Right now, most of the total processing time is spent in this loop, as
       remote_job_start() will block until the next worker becomes available */
    job_t*job;
    int pos = 0;
    for(job=jobs->first;job;job=job->next) {
        r[pos] = remote_job_start(job, job->factory->name, job->transforms, job->data, servers);
        job->code = 0;
        pos++;
    }
    int num = pos;

    /* Wait for remaining servers and gather results */
    int open_jobs = jobs->num;
    int i;
    printf("%d open jobs\n", open_jobs);
    while(open_jobs) {
        for(i=0;i<num;i++) {
            remote_job_t*j = r[i];
            job_t*job = j->job;
            if(!j->done && !job->code) {
                if(remote_job_is_ready(j)) {
                    ftime(&j->profile_time[3]);
                    remote_job_read_result(j, job);
                    if(job->code) {
                        printf("Finished: %s\n", job->factory->name);
                    } else {
                        printf("Failed (0x%02x): %s\n", j->response, job->factory->name);
                    }
                    open_jobs--;
                    ftime(&j->profile_time[4]);
                    j->done = true;
                } else if(remote_job_age(j) > config_model_timeout) {
                    ftime(&j->profile_time[3]);
                    printf("Failed (timeout): %s\n", job->factory->name);
                    remote_job_cancel(j);
                    open_jobs--;
                    ftime(&j->profile_time[4]);
                    j->done = true;
                }
            }
            pos++;
        }
    }

    for(i=0;i<num;i++) {
        remote_job_t*j = r[i];
        //store_times(j, i);
        free(j);
    }

    free(r);

    signal(SIGPIPE, old_sigpipe);
}

