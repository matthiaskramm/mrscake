/* net.h
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

#ifndef __server_h__
#define __server_h__

#include <time.h>
#include "dataset.h"
#include "settings.h"
#include "job.h"

typedef enum {REQUEST_SEND_DATASET,
              REQUEST_RECV_DATASET,
              REQUEST_TRAIN_MODEL} request_type_t;

typedef enum {RESPONSE_OK,
              RESPONSE_DATA_ERROR,
              RESPONSE_DUPL_DATA,
              RESPONSE_DATASET_UNKNOWN,
              RESPONSE_FACTORY_UNKNOWN,
              RESPONSE_READ_ERROR=-1} response_type_t;

typedef struct _remote_job {
    int socket;
    time_t start_time;
    response_type_t response;
} remote_job_t;

typedef struct _server_array {
    remote_server_t**servers;
    int num;
} server_array_t;

int connect_to_host(const char *host, int port);
node_t* process_job_remotely(const char*model_name, dataset_t*dataset);
remote_job_t* remote_job_start(const char*model_name, dataset_t*dataset);
bool remote_job_is_ready(remote_job_t*j);
time_t remote_job_age(remote_job_t*j);
node_t* remote_job_read_result(remote_job_t*j);
void remote_job_cancel(remote_job_t*j);

void server_array_destroy(server_array_t*);

dataset_t* dataset_read_from_server(const char*host, int port, uint8_t*hash);
server_array_t* distribute_dataset(dataset_t*data);

int start_server(int port);

void distribute_jobs_to_servers(dataset_t*dataset, jobqueue_t*jobs, server_array_t*servers);
#endif
