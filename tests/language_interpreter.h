#ifndef __language_interpreter_h__
#define  __language_interpreter_h__

#include <stdbool.h>
#include "../mrscake.h"

typedef struct _language_interpreter {
    void*internal;
    const char*name;
    bool (*define_function)(struct _language_interpreter*li, const char*script);
    int (*call_function)(struct _language_interpreter*li, row_t*row);
    void (*destroy)(struct _language_interpreter*li);
} language_interpreter_t;

language_interpreter_t* javascript_interpreter_new();
language_interpreter_t* python_interpreter_new();

char* row_to_function_call(row_t*row, char*buffer, bool add_brackets);

#endif //__language_interpreter_h__
