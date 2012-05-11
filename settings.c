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
#include "util.h"

int config_num_remote_servers = 0;
remote_server_t*config_remote_servers = 0;
int config_job_wait_timeout = 300;
int config_remote_read_timeout = 10;
int config_model_timeout = 15;
bool config_do_remote_processing = false;
int config_number_of_remote_workers = 2;
int config_verbosity = 1;
char*config_dataset_cache_directory = "/tmp/mrscake";
int config_num_seeded_hosts = 1;
bool config_subset_variables = true;
int config_remote_worker_timeout = 60;

static int remote_server_size = 0;

void remote_server_is_broken(remote_server_t*server, const char*error)
{
    if(server->broken)
        free((void*)server->broken);
    server->broken = strdup(error);
}
void remote_server_print(remote_server_t*server)
{
    printf("%s:%d", server->host, server->port);
    if(server->broken)
        printf(" [%s]", server->broken);
    printf("\n");
}
bool config_has_remote_servers()
{
    int i = 0;
    for(i=0;i<config_num_remote_servers;i++) {
        remote_server_t* r = &config_remote_servers[i];
        if(!r->broken)
            return true;
    }
    return false;
}
void config_print_remote_servers()
{
    int i = 0;
    for(i=0;i<config_num_remote_servers;i++) {
        printf("REMOTE SERVER %d: ", i);
        remote_server_print(&config_remote_servers[i]);
    }
}

void config_add_remote_server(const char*host, int port)
{
    if(!remote_server_size) {
        remote_server_size = 32;
        config_remote_servers = malloc(sizeof(remote_server_t)*remote_server_size);
    } else if(config_num_remote_servers >= remote_server_size) {
        remote_server_size *= 2;
        config_remote_servers = realloc(config_remote_servers, sizeof(remote_server_t)*remote_server_size);
    }
    remote_server_t*s = &config_remote_servers[config_num_remote_servers++];
    s->host = host;
    s->port = port;
    s->name = allocprintf("%s:%d", host, port);
    s->broken = NULL;
    config_do_remote_processing = true;
}

void config_parse_remote_servers(const char*filename)
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
                if(line[0]!='#') {
                    char*colon = strchr(line, ':');
                    char*server;
                    int port;
                    if(colon) {
                        *colon = 0;
                        server = line;
                        port = atoi(colon+1);
                    } else {
                        server = line;
                        port = MRSCAKE_DEFAULT_PORT;
                    }
                    config_add_remote_server(server, port);
                    printf("%s %d\n", server, port);
                }
            }
            line = p+1;
        }
        p++;
    }
}
