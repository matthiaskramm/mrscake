/* settings.h
   Configuration data.

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

#ifndef __settings_h__
#define __settings_h__

#include <stdbool.h>

typedef struct remote_server {
    const char*host;
    int port;
    const char*name;

    const char*broken;
} remote_server_t;

extern int config_num_remote_servers;
extern remote_server_t*config_remote_servers;
extern int config_remote_worker_timeout;

extern int config_remote_read_timeout;
extern int config_job_wait_timeout;
extern int config_model_timeout;
extern bool config_do_remote_processing;
extern int config_number_of_remote_workers;
extern int config_verbosity;
extern char*config_dataset_cache_directory;
extern int config_num_seeded_hosts;

#define MRSCAKE_DEFAULT_PORT 3075

void config_parse_remote_servers(const char*filename);
void config_add_remote_server(const char*host, int port);

void config_print_remote_servers();

void remote_server_print(remote_server_t*server);
void remote_server_is_broken(remote_server_t*server, const char*error);
#endif
