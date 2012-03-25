#include <stdlib.h>
#include <string.h>
#include "util.h"

void*memdup(const void*ptr, size_t size)
{
    void*new_ptr = malloc(size);
    if(!new_ptr) 
        return NULL;
    memcpy(new_ptr, ptr, size);
    return new_ptr;
}
