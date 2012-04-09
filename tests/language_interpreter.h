#ifndef __language_interpreter_h__
#define  __language_interpreter_h__

typedef struct _language_interpreter {
    void*internal;
    int (*run_script)(struct _language_interpreter*self, const char*script);
    void (*destroy)(struct _language_interpreter*self);
} language_interpreter_t;

language_interpreter_t* javascript_interpreter_new();

#endif //__language_interpreter_h__
