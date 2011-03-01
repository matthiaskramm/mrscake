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
#include "settings.h"

int config_num_remote_servers = 0;
remote_server_t*config_remote_servers = 0;
int config_remote_read_timeout = 10;
int config_model_timeout = 15;
bool config_do_remote_processing = true;

void config_parse_remote_servers(char*filename)
{
    FILE*fi = fopen(filename, "rb");
    if(!fi) {
        perror(filename);
        exit(1);
    }
    char*server = 0;
    int port = 0;
    int l;
    while(!feof(fi)) {
        if(fscanf(fi, "%as:%d\n", &server, &port) == 2) {
            continue;
        } else if(fscanf(fi, "%as\n", &server) == 1) {
            continue;
        } else if(fscanf(fi, "\n") == 0) {
            continue;
        } else break;
    }
}
