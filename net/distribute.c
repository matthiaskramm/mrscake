/* distribute.c
   distributed job processing

   Part of the data prediction package.
   
   Copyright (c) 2011/2012 Matthias Kramm <kramm@quiss.org> 
 
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
#include <sys/times.h>
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
#ifdef HAVE_SYS_TIMEB
#include <sys/timeb.h>
#endif
#include "net/distribute.h"
#include "net/protocol.h"
#include "io.h"
#include "dataset.h"
#include "model_select.h"
#include "serialize.h"
#include "settings.h"
#include "job.h"
#include "util.h"

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

int connect_to_remote_server(remote_server_t*server)
{
    int i, ret;
    char buf_ip[100];
    struct sockaddr_in sin;

    unsigned char*ip = server->ip;
    if(!ip) {
        struct hostent *he = gethostbyname(server->host);
        if(!he) {
            fprintf(stderr, "gethostbyname returned %d\n", h_errno);
            herror(server->host);
            remote_server_is_broken(server, hstrerror(h_errno));
            return -1;
        }
        ip = server->ip = memdup(he->h_addr_list[0], 4);
    }

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(server->port);
    memcpy(&sin.sin_addr.s_addr, ip, 4);

    int sock = socket(AF_INET, SOCK_STREAM, 6);
    if(sock < 0) {
        perror("socket");
        remote_server_is_broken(server, strerror(errno));
        return -2;
    }

    /* FIXME: connect has a very long timeout */
    ret = connect(sock, (struct sockaddr*)&sin, sizeof(struct sockaddr_in));
    if(ret < 0) {
        close(sock);
        perror("connect");
        remote_server_is_broken(server, strerror(errno));
        return -3;
    }

    /* receive header */
    reader_t*r = filereader_with_timeout_new(sock, config_remote_read_timeout);
    uint8_t header[3];
    int c = r->read(r, header, 3);
    if(c!= 3 ||
       (header[0] != RESPONSE_IDLE &&
        header[0] != RESPONSE_BUSY)) {
        close(sock);
        fprintf(stderr, "invalid header");
        remote_server_is_broken(server, "invalid header");
        return -4;
    }
    server->num_jobs = header[1];
    server->num_workers = header[2];
    if(header[0] == RESPONSE_BUSY) {
        /* TODO: we should allow transferring datasets even when jobs are running
                 on a server */
        close(sock);
        server->busy = true;
        return -5;
    }
    server->busy = false;
    return sock;
}

int connect_to_host(const char*name, int port)
{
    remote_server_t dummy;
    memset(&dummy, 0, sizeof(dummy));
    dummy.host = name;
    dummy.port = port;
    return connect_to_remote_server(&dummy);
}

static int send_dataset_to_remote_server(remote_server_t*server, dataset_t*data, remote_server_t*from_server)
{
    int sock = connect_to_remote_server(server);
    if(sock<0) {
        return RESPONSE_READ_ERROR;
    }
    if(server->busy) {
        return RESPONSE_BUSY;
    }

    writer_t*w = filewriter_new(sock);
    reader_t*r = filereader_with_timeout_new(sock, config_remote_read_timeout);

    bool ret = make_request_RECV_DATASET(r, w, data, from_server);
    int resp;
    if(!ret) {
        remote_server_is_broken(server, "read/write error in RECV_DATASET");
        resp = RESPONSE_READ_ERROR;
    } else {
        char hash[HASH_SIZE];
        r->read(r, hash, HASH_SIZE);
        resp = read_uint8(r);
        if(r->error) {
            remote_server_is_broken(server, "read error after RECV_DATASET");
            resp = RESPONSE_READ_ERROR;
        } else if(memcmp(hash, data->hash, HASH_SIZE)) {
            remote_server_is_broken(server, "bad data checksum after RECV_DATASET");
            resp = RESPONSE_DATA_ERROR;
        }
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
            case RESPONSE_BUSY:
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
                printf("%s: received dataset%s\n", server->name, resp==RESPONSE_DUPL_DATA?" (cached)":"");
                status[seed_nr] = 1;
                seeds[num_seeds++] = server;
            break;
            default:
            case RESPONSE_BUSY:
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

remote_job_t* remote_job_try_to_start(job_t*job, const char*model_name, const char*transforms, dataset_t*dataset, server_array_t*servers)
{
    remote_job_t*j = calloc(1, sizeof(remote_job_t));
    j->job = job;
    j->start_time = time(0);

    ftime(&j->profile_time[0]);
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
    int sock = connect_to_remote_server(s);
    if(sock<0) {
        free(j);
        return NULL;
    }
    j->server = s;
    j->running = true;
    printf("Starting job %d on %s\n", job->nr, s->name);

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

void remote_job_read_result(remote_job_t*j, int32_t*best_score)
{
    reader_t*r = filereader_with_timeout_new(j->socket, config_remote_read_timeout);
    writer_t*w = filewriter_new(j->socket);
    finish_request_TRAIN_MODEL(r, w, j, *best_score);
    if(config_limit_network_io && j->job->score < *best_score) {
        *best_score = j->job->score;
    }
    w->finish(w);
    r->dealloc(r);
}

time_t remote_job_age(remote_job_t*j)
{
    return time(0) - j->start_time;
}

#ifdef HAVE_SYS_TIMEB
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
#endif

void distribute_jobs_to_servers(dataset_t*dataset, jobqueue_t*jobs, server_array_t*servers)
{
    remote_job_t**r = malloc(sizeof(reader_t*)*jobs->num);
    /* Ignore sigpipe events. Write calls to closed network sockets 
       will now only return an error, not halt the program */
    sig_t old_sigpipe = signal(SIGPIPE, SIG_IGN);

    job_t*job = jobs->first;
    int open_jobs = jobs->num;
    int i;
    printf("%d open jobs\n", open_jobs);
    int32_t best_score = INT_MAX;
    float total_cpu_time = 0.0;

    int num = 0;
    int pos = 0;
    do {
        if(job) {
            r[num] = remote_job_try_to_start(job, job->factory->name, job->transforms, job->data, servers);
            if(r[num]) {
                job->code = NULL;
                num++;
                job = job->next;
            }
        }

        for(i=0;i<num;i++) {
            remote_job_t*j = r[i];
            job_t*job = j->job;
            if(j->running && !j->done && !job->code) {
                if(remote_job_is_ready(j)) {
                    ftime(&j->profile_time[3]);
                    remote_job_read_result(j, &best_score);
                    if(j->response == RESPONSE_OK) {
                        printf("Finished: %s (%.2f s)\n", job->factory->name, j->cpu_time);
                        total_cpu_time += j->cpu_time;
                    } else {
                        printf("Failed (%s, 0x%02x): %s\n", j->server->name, j->response, job->factory->name);
                    }
                    open_jobs--;
                    ftime(&j->profile_time[4]);
                    j->done = true;
                    close(j->socket);
                } else if(num == jobs->num && remote_job_age(j) > config_remote_worker_timeout) {
                    ftime(&j->profile_time[3]);
                    printf("Failed (%s, timeout): %s\n", j->server->name, job->factory->name);
                    open_jobs--;
                    ftime(&j->profile_time[4]);
                    j->done = true;
                    close(j->socket);
                }
            }
            pos++;
        }
    } while(open_jobs);

    printf("total cpu time: %.2f\n", total_cpu_time);

    for(i=0;i<num;i++) {
        remote_job_t*j = r[i];
#ifdef HAVE_SYS_TIMEB
        //store_times(j, i);
#endif
        free(j);
    }

    free(r);

    signal(SIGPIPE, old_sigpipe);
}

void process_jobs_remotely(dataset_t*dataset, jobqueue_t*jobs)
{
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

    distribute_jobs_to_servers(dataset, jobs, servers);

#ifdef HAVE_SYS_TIMEB
    ftime(&end_ftime);
    double end_time = end_ftime.time + end_ftime.millitm/1000.0;
    printf("total time: %.2f (%.2f for file transfer)\n", end_time - start_time, distribute_done_time - start_time);
#endif
}

