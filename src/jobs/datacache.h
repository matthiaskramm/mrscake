/* datacache.h
   Cache for dataset_t

   Part of the mrscake data prediction package.
   
   Copyright (c) 2012 Matthias Kramm <kramm@quiss.org> 
 
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

#ifndef __datacache_h__
#define __datacache_h_

#include "dataset.h"

type_t dataset_hash_type;

typedef struct _datacache_entry {
    dataset_t*dataset;
    struct _datacache_entry* next;
} datacache_entry_t;

typedef struct _datacache {
    dict_t*dict;
} datacache_t;

char*hash_to_string(uint8_t*hash);

datacache_t* datacache_new();
dataset_t* datacache_find(datacache_t*cache, uint8_t*hash);
void datacache_store(datacache_t*cache, dataset_t*dataset);

#endif
