#ifndef __language_interpreter_h__
#define  __language_interpreter_h__

#include <stdbool.h>

typedef struct _language_interpreter {
    void*internal;
    const char*name;
    bool (*exec)(struct _language_interpreter*li, const char*script);
    int (*eval)(struct _language_interpreter*li, const char*script);
    void (*destroy)(struct _language_interpreter*li);
} language_interpreter_t;

language_interpreter_t* javascript_interpreter_new();

#endif //__language_interpreter_h__
