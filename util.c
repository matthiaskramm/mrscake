#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/types.h>
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

char* allocprintf(const char*format, ...)
{
    va_list arglist1;
    va_start(arglist1, format);
    char dummy;
    int l = vsnprintf(&dummy, 1, format, arglist1);
    va_end(arglist1);

    va_list arglist2;
    va_start(arglist2, format);
    char*buf = malloc(l+1);
    vsnprintf(buf, l+1, format, arglist2);
    va_end(arglist2);
    return buf;
}

#if defined(CYGWIN)
static char path_seperator = '/';
#elif defined(WIN32)
static char path_seperator = '\\';
#else
static char path_seperator = '/';
#endif

char* concat_paths(const char*base, const char*add)
{

    int l1 = strlen(base);
    int l2 = strlen(add);
    int pos = 0;
    char*n = 0;
    while(l1 && base[l1-1] == path_seperator)
	l1--;
    while(pos < l2 && add[pos] == path_seperator)
	pos++;

    n = (char*)malloc(l1 + (l2-pos) + 2);
    memcpy(n,base,l1);
    n[l1]=path_seperator;
    strcpy(&n[l1+1],&add[pos]);
    return n;
}

void mkdir_p(const char*_path)
{
    char*path = strdup(_path);
    char*p = path;
    while(*p == path_seperator) {
        p++;
    }
    while(*p) {
        while(*p && *p != path_seperator) {
            p++;
        }
        if(!*p)
            break;
        *p = 0;
        mkdir(path, 0700);
        *p = path_seperator;
        p++;
    }
    mkdir(path, 0700);
}

int imin(int x, int y)
{
    return x<y?x:y;
}

bool str_starts_with(const char*str, const char*start)
{
    int l = strlen(start);
    return strncmp(str, start, l) == 0;
}
