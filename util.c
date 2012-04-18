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

char*escape_string(const char *input) 
{
    static const char before[] = "\a\b\f\n\r\t\v\\\"\'";
    static const char after[]  = "abfnrtv\\\"\'";
    
    int l = strlen(input);
    char*output = malloc((l+1)*2);
    char*o = output;
    const char*p;
    for(p=input; *p; p++) {
        char*pos=strchr(before, *p);
        if(pos) {
            *o++ = '\\';
            *o++ = after[pos-before];
        } else {
            *o++ = *p;
        }
    }
    *o = 0;
    return output;
}


