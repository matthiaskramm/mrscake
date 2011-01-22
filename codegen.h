/* codegen.h
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
    model_t*model;
    writer_t*writer;
    codegen_t*codegen;
};

void strf(state_t*s, const char*format, ...);
void write_escaped_string(state_t*s, const char*p);
void write_node(state_t*s, node_t*n);
void indent(state_t*s);
void dedent(state_t*s);

codegen_t codegen_python;
codegen_t codegen_c;
codegen_t* codegen_default;

char*generate_code(codegen_t*codegen, model_t*m);

#endif

