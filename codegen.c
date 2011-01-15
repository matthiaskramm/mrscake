#include <assert.h>
#include "codegen.h"
#include "ast_transforms.h"

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

char*generate_code(codegen_t*codegen, model_t*m)
{
    node_t*n = (node_t*)m->code;
    state_t s;
    s.codegen = codegen;
    s.indent = 0;
    s.writer = growingmemwriter_new();
    n = node_prepare_for_code_generation(n);
    codegen->write_header(m, &s);
    write_node(&s, n);
    codegen->write_footer(m, &s);
    write_uint8(s.writer, 0);

    char*result = writer_growmemwrite_getmem(s.writer, 0);
    s.writer->finish(s.writer);
    return result;
}

