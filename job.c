/* job.c
   Job processing

   Part of the data prediction package.
   
   Copyright (c) 2010-2011 Matthias Kramm <kramm@quiss.org> 
 
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

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "job.h"
#include "io.h"
#include "settings.h"
#include "net.h"
#include "serialize.h"

void job_train_and_score(job_t*job)
{
    node_t*code = job->factory->train(job->factory, job->data);
    if(code) {
        job->code = code;
        job->score = code_score(job->code, job->data);
    }
}

#define FORK_FOR_TRAINING
void job_process(job_t*job)
{
#ifndef FORK_FOR_TRAINING
    job_train_and_score(job);
    return;
#else
    int p[2];
    int ret = pipe(p);
    int read_fd = p[0];
    int write_fd = p[1];
    if(ret) {
        perror("create pipe");
        exit(-1);
    }
    pid_t pid = fork();
    if(!pid) {
        //child
        close(read_fd); // close read

        job_train_and_score(job);

        writer_t*w = filewriter_new(write_fd);
        write_compressed_int(w, job->score);
        node_write(job->code, w, SERIALIZE_DEFAULTS);
        w->finish(w);
        close(write_fd); // close write
        _exit(0);
    } else {
        //parent
        close(write_fd); // close write
        reader_t*r = filereader_with_timeout_new(read_fd, config_job_wait_timeout);
        job->score = read_compressed_int(r);
        job->code = node_read(r);
        r->dealloc(r);
        close(read_fd); // close read

        kill(pid, SIGKILL);
        ret = waitpid(pid, NULL, WNOHANG);
    }
#endif
}

static void process_jobs(jobqueue_t*jobs)
{
    job_t*job;
    int count = 0;
    for(job=jobs->first;job;job=job->next) {
        if(config_verbosity > 0)
            printf("\rJob %d / %d", count, jobs->num);fflush(stdout);
        job_process(job);
        count++;
    }
    if(config_verbosity > 0)
        printf("\n");
}

static void process_jobs_remotely(jobqueue_t*jobs)
{
    remote_job_t**r = malloc(sizeof(reader_t*)*jobs->num);

    /* Ignore sigpipe events. Write calls to closed network sockets 
       will now only return an error, not halt the program */
    sig_t old_sigpipe = signal(SIGPIPE, SIG_IGN);

    /* Right now, most of the total processing time spent in this loop, as
       remote_job_start() will block until the next worker becomes available */
    job_t*job;
    int pos = 0;
    for(job=jobs->first;job;job=job->next) {
        r[pos] = remote_job_start(job->factory->name, job->data);
        job->code = 0;
        pos++;
    }

    /* Wait for remaining servers and gather results */
    int open_jobs = jobs->num;
    printf("%d open jobs\n", open_jobs);
    while(open_jobs) {
        int pos = 0;
        for(job=jobs->first;job;job=job->next) {
            if(r[pos] && !job->code) {
                if(remote_job_is_ready(r[pos])) {
                    job->code = remote_job_read_result(r[pos]);
                    if(job->code) {
                        printf("Finished: %s\n", job->factory->name);
                    } else {
                        printf("Failed (bad data): %s\n", job->factory->name);
                    }
                    r[pos] = 0;
                    open_jobs--;
                } else if(remote_job_age(r[pos]) > config_model_timeout) {
                    printf("Failed (timeout): %s\n", job->factory->name);
                    remote_job_cancel(r[pos]);
                    r[pos] = 0;
                    open_jobs--;
                }
            }
            pos++;
        }
    }
    free(r);
    signal(SIGPIPE, old_sigpipe);
}

void jobqueue_process(jobqueue_t*jobs)
{
    if(config_do_remote_processing) {
        process_jobs_remotely(jobs);
    } else {
        process_jobs(jobs);
    }
}

void job_destroy(job_t*j)
{
    free(j);
}

jobqueue_t*jobqueue_new()
{
    jobqueue_t*q = calloc(1, sizeof(jobqueue_t));
    return q;
}

jobqueue_t*jobqueue_destroy(jobqueue_t*q)
{
    job_t*j = q->first;
    while(j) {
        job_t*next = j->next;
        job_destroy(j);
        j = next;
    }
    free(q);
}

void jobqueue_delete_job(jobqueue_t*queue, job_t*job)
{
    if(job->prev) {
        job->prev->next = job->next;
    } else {
        queue->first = job->next;
    }
    if(job->next) {
        job->next->prev = job->prev;
    } else {
        queue->last = job->prev;
    }
    job_destroy(job);
    queue->num--;
}

void jobqueue_append(jobqueue_t*queue, job_t*job)
{
    job->prev = queue->last;
    job->next = 0;
    if(!queue->first) {
        queue->first = job;
        queue->last = job;
    } else {
        queue->last->next = job;
        queue->last = job;
    }
    queue->num++;
}

void jobqueue_print(jobqueue_t*queue)
{
    job_t*job = queue->first;
    while(job) {
        printf("[%c] %s\n", job->code?'x':' ', job->factory->name);
        job = job->next;
    }
}

job_t* job_new()
{
    job_t*job = calloc(1,sizeof(job_t));
    return job;
}
