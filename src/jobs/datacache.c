/* datacache.c
   Caches for dataset_t

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

#include <stdlib.h>
#include <stdbool.h>
#include <strings.h>
#include <memory.h>
#include <unistd.h>
#include <sys/stat.h>
#include "io.h"
#include "dict.h"
#include "util.h"
#include "datacache.h"
#include "settings.h"
#include "serialize.h"

static bool dataset_hash_equals(const void*h1, const void*h2)
{
    return memcmp(h1, h2, HASH_SIZE)==0;
}
static unsigned int dataset_hash_hash(const void*h)
{
    return hash_block(h, HASH_SIZE);
}
static void* dataset_hash_dup(const void*o)
{
    return memdup(o, HASH_SIZE);
}
static void dataset_hash_free(void*o)
{
    free(o);
}
type_t dataset_hash_type = {
    equals: (equals_func)dataset_hash_equals,
    hash: (hash_func)dataset_hash_hash,
    dup: (dup_func)dataset_hash_dup,
    free: (free_func)dataset_hash_free,
};

datacache_t* datacache_new()
{
    datacache_t*cache = calloc(sizeof(datacache_t), 1);
    cache->dict = dict_new(&dataset_hash_type);
    mkdir_p(config_dataset_cache_directory);
    return cache;
}

char*hash_to_string(uint8_t*hash)
{
    char*str = malloc(HASH_SIZE*2+1);
    int i;
    int pos = 0;
    for(i=0;i<HASH_SIZE;i++) {
        pos += sprintf(str+pos, "%02x", hash[i]);
    }
    return str;
}

static char*dataset_filename(uint8_t*hash)
{
    char*basename = hash_to_string(hash);
    char*path = concat_paths(config_dataset_cache_directory, basename);
    free(basename);
    return path;
}

dataset_t* datacache_find(datacache_t*cache, uint8_t*hash)
{
    datacache_entry_t*e = dict_lookup(cache->dict, hash);
    if(e)
        return e->dataset;
    char*filename = dataset_filename(hash);
    reader_t*r = filereader_new2(filename);
    if(!r)
        return NULL;
    dataset_t*dataset = dataset_read(r);
    if(!dataset || r->error) {
        unlink(filename);
        return NULL;
    }
    return dataset;
}

void datacache_store(datacache_t*cache, dataset_t*dataset)
{
    dict_put(cache->dict, dataset->hash, dataset);

    char*filename = dataset_filename(dataset->hash);
    struct stat sb;
    if(stat(filename, &sb)!=0) {
        dataset_save(dataset, filename);
    }
}
