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

#include "dataset.h"

typedef struct _remote_job {
    int socket;
} remote_job_t;

int connect_to_host(char *host, int port);
model_t* process_job_remotely(const char*model_name, sanitized_dataset_t*dataset);
remote_job_t* remote_job_start(const char*model_name, sanitized_dataset_t*dataset);
bool remote_job_is_ready(remote_job_t*j);
model_t* remote_job_read_result(remote_job_t*j);
#endif
