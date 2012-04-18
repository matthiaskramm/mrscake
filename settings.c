/* settings.c
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "settings.h"

int config_num_remote_servers = 0;
remote_server_t*config_remote_servers = 0;
int config_job_wait_timeout = 300;
int config_remote_read_timeout = 10;
int config_model_timeout = 15;
bool config_do_remote_processing = false;
int config_number_of_remote_workers = 32;
int config_verbosity = 1;

int config_remote_worker_timeout = 60;

static int remote_server_size = 0;

void config_add_remote_server(char*host, int port)
{
    if(!remote_server_size) {
        remote_server_size = 32;
        config_remote_servers = malloc(sizeof(remote_server_t)*remote_server_size);
    } else if(config_num_remote_servers <= remote_server_size) {
        remote_server_size *= 2;
        config_remote_servers = realloc(config_remote_servers, sizeof(remote_server_t)*remote_server_size);
    }
    remote_server_t*s = &config_remote_servers[config_num_remote_servers++];
    s->host = host;
    s->port = port;
    config_do_remote_processing = true;
}

void config_parse_remote_servers(char*filename)
{
    FILE*fi = fopen(filename, "rb");
    if(!fi) {
        perror(filename);
        exit(1);
    }
    fseek(fi, 0, SEEK_END);
    long size = ftell(fi);
    fseek(fi, 0, SEEK_SET);
    char*data = malloc(size+1);
    fread(data, size, 1, fi);
    data[size] = '\n';

    char*p = data;
    char*line = data;
    while(p < &data[size+1]) {
        if(*p == '\n') {
            char*end = p;
            while(end > line && strchr(" \t\r", end[-1]))
                end--;
            if(end > line) {
                *end = 0;
                //parse line
                char*colon = strchr(line, ':');
                char*server;
                int port;
                if(colon) {
                    *colon = 0;
                    server = line;
                    port = atoi(colon+1);
                } else {
                    server = line;
                    port = 3075;
                }
                config_add_remote_server(server, port);
                printf("%s %d\n", server, port);
            }
            line = p+1;
        }
        p++;
    }
}
