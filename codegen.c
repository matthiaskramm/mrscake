#include <assert.h>
#include "codegen.h"

bool node_has_consumer_parent(node_t*n)
{
    while(1) {
        node_t*child = n;
        n = n->parent;
        if(!n) break;
        if(n->type == &node_block) {
            if(n->child[n->num_children-1] != child)
                return false;
        }
    }
    return true;
}
bool missing(node_t*n)
{
    return(n->type == &node_missing ||
           n->type == &node_nop);
}
void strf(state_t*state, const char*format, ...)
{
    char buf[1024];
    int l;
    va_list arglist;
    va_start(arglist, format);
    vsnprintf(buf, sizeof(buf)-1, format, arglist);
    va_end(arglist);
    char*s = buf;
    while(*s) {
        char*last = s;
        while(*s && *s!='\n') 
            s++;
        if(s>last) {
            state->writer->write(state->writer, last, s-last);
        }
        if(*s=='\n') {
            write_uint8(state->writer, '\n');
            int t;
            for(t=0;t<state->indent;t++) {
                write_uint8(state->writer, ' ');
            }
            s++;
            if(!*s) {
                state->after_newline = 1;
                return;
            }
        }
    }
    state->after_newline = 0;
}
void write_node(state_t*s, node_t*n)
{
    switch(node_get_opcode(n)) {
#define NODE(opcode, name) \
        case opcode: \
            s->codegen->write_##name(n, s); \
        break;
LIST_NODES
#undef NODE
        default:
            assert(0);
    }
}

void indent(state_t*s)
{
    s->indent += 4;
    if(s->after_newline) {
        /* if we're on a new line, indent this line, too */
        char*indent = "    ";
        s->writer->write(s->writer, indent, 4);
    }
}

void dedent(state_t*s)
{
    s->indent -= 4;
}

char*generate_code(codegen_t*codegen, node_t*n)
{
    state_t s;
    s.codegen = codegen;
    s.indent = 0;
    s.writer = growingmemwriter_new();
    write_node(&s, n);
    write_uint8(s.writer, 0);

    char*result = writer_growmemwrite_getmem(s.writer, 0);
    s.writer->finish(s.writer);
    return result;
}

