#ifndef __codegen_h__
#define  __codegen_h__
#include <stdarg.h>
#include "ast.h"
#include "io.h"

struct _state;
typedef struct _state state_t;

typedef struct _codegen {
#define NODE(opcode, name) void (*write_##name)(node_t*node, state_t*s);
LIST_NODES
#undef NODE
    void (*write_header)(model_t*model, state_t*s);
    void (*write_footer)(model_t*model, state_t*s);
} codegen_t;

struct _state {
    int indent;
    bool after_newline;
    writer_t*writer;
    codegen_t*codegen;
};

void strf(state_t*s, const char*format, ...);
void write_node(state_t*s, node_t*n);
void indent(state_t*s);
void dedent(state_t*s);

codegen_t codegen_python;

char*generate_code(codegen_t*codegen, model_t*m);

#endif

