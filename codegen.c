/* codegen.c
   Code generator

   Part of the data prediction package.
   
   Copyright (c) 2011 Matthias Kramm <kramm@quiss.org> 
 
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#include <assert.h>
#include <string.h>
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
void write_escaped_string(state_t*s, const char*p)
{
    while(*p) {
        switch(*p) {
            case '\n':
                write_uint8(s->writer, '\\');
                write_uint8(s->writer, 'n');
                break;
            case '\r':
                write_uint8(s->writer, '\\');
                write_uint8(s->writer, 'r');
                break;
            case '\t':
                write_uint8(s->writer, '\\');
                write_uint8(s->writer, 't');
                break;
            case '"':
                write_uint8(s->writer, '\\');
                write_uint8(s->writer, '"');
                break;
            default:
                if(*p>0x20 && *p<0x7f) {
                    write_uint8(s->writer, *p);
                } else {
                    write_uint8(s->writer, '\\');
                    write_uint8(s->writer, 'x');
                    write_uint8(s->writer, "0123456789abcdef"[((*p)>>4)&15]);
                    write_uint8(s->writer, "0123456789abcdef"[((*p)&15)]);
                }
                break;
        }
        p++;
    }
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
    s.model = m;
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
codegen_t* codegen_default = &codegen_python;

char*model_generate_code(model_t*m, const char*language)
{
    if(!language) {
        return generate_code(codegen_default, m);
    } else if(!strcmp(language,"python")) {
        return generate_code(&codegen_python, m);
    } else if(!strcmp(language,"c")) {
        return generate_code(&codegen_c, m);
    } else if(!strcmp(language,"c++")) {
        return generate_code(&codegen_c, m);
    } else if(!strcmp(language,"ruby")) {
        return generate_code(&codegen_ruby, m);
    } else if(!strcmp(language,"javascript")) {
        return generate_code(&codegen_js, m);
    } else if(!strcmp(language,"js")) {
        return generate_code(&codegen_js, m);
    } else {
        return generate_code(codegen_default, m);
    }
}

