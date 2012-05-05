/* job.h
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

#ifndef __job_h__
#define __job_h__
#ifdef __cplusplus
extern "C" {
#endif

#include "model_select.h"
#include "dataset.h"

typedef struct _job {
    dataset_t*data;
    char* transforms;
    model_factory_t*factory;
    node_t*code;

    int score;

    struct _job*prev;
    struct _job*next;
} job_t;

typedef struct _jobqueue {
    job_t*first;
    job_t*last;
    int num;
} jobqueue_t;

jobqueue_t*jobqueue_new();
void jobqueue_append(jobqueue_t*queue, job_t*job);
void jobqueue_delete_job(jobqueue_t*queue, job_t*job);
void jobqueue_process(dataset_t*data, jobqueue_t*jobs);
void jobqueue_print(jobqueue_t*);
jobqueue_t*jobqueue_destroy();
void job_destroy(job_t*j);
job_t* job_new();
void job_process(job_t*job);

#ifdef __cplusplus
}
#endif
#endif //__job_h__

