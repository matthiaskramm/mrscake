/* ast.h

   AST representation of prediction programs.

   Copyright (c) 2010 Matthias Kramm <kramm@quiss.org>
 
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

#ifndef __ast_h__
#define __ast_h__

#include <stdio.h>
#include <stdbool.h>
#include "constant.h"
#include "environment.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _node node_t;
typedef struct _nodetype nodetype_t;

#define NODE_FLAG_HAS_CHILDREN 1
#define NODE_FLAG_HAS_VALUE 2

struct _nodetype {
    char*name;
    int min_args;
    int max_args;
    uint8_t opcode;
    uint8_t flags;
    constant_t (*eval)(node_t*n, environment_t* params);
};

struct _node {
    nodetype_t*type;
    node_t*parent;

    node_t**child;
    int num_children;

    constant_t value;
};

/* all known node types & opcodes */
#define LIST_NODES \
    NODE(0x71, node_block) \
    NODE(0x01, node_if) \
    NODE(0x02, node_add) \
    NODE(0x03, node_sub) \
    NODE(0x04, node_mul) \
    NODE(0x05, node_div) \
    NODE(0x06, node_lt) \
    NODE(0x07, node_lte) \
    NODE(0x08, node_gt) \
    NODE(0x09, node_gte) \
    NODE(0x0a, node_in) \
    NODE(0x0b, node_not) \
    NODE(0x0c, node_neg) \
    NODE(0x0d, node_exp) \
    NODE(0x0e, node_sqr) \
    NODE(0x0f, node_abs) \
    NODE(0x10, node_var) \
    NODE(0x11, node_nop) \
    NODE(0x12, node_constant) \
    NODE(0x13, node_bool) \
    NODE(0x14, node_category) \
    NODE(0x15, node_array) \
    NODE(0x16, node_float) \
    NODE(0x17, node_int) \
    NODE(0x18, node_string) \
    NODE(0x19, node_missing) \
    NODE(0x1a, node_getlocal)  \
    NODE(0x1b, node_setlocal) \
    NODE(0x1c, node_inclocal) \
    NODE(0x1d, node_bool_to_float) \
    NODE(0x1e, node_equals) \
    NODE(0x1f, node_arg_max) \
    NODE(0x20, node_arg_max_i) \
    NODE(0x21, node_array_at_pos) \

#define NODE(opcode, name) extern nodetype_t name;
LIST_NODES
#undef NODE

extern nodetype_t* nodelist[];
void nodelist_init();
uint8_t node_get_opcode(node_t*n);

node_t* node_new(nodetype_t*t);
node_t* node_new_with_args(nodetype_t*t,...);
void node_append_child(node_t*n, node_t*child);
void node_free(node_t*n);
constant_t node_eval(node_t*n,environment_t* e);
int node_highest_local(node_t*node);
void node_print(node_t*n);

#ifdef __cplusplus
}
#endif


#endif
