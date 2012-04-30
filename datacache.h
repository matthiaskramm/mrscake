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
