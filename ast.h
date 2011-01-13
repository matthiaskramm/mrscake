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
    int flags;
    int min_args;
    int max_args;
    uint8_t opcode;
    constant_t (*eval)(node_t*n, environment_t* params);
};

struct _node {
    nodetype_t*type;
    node_t*parent;
    struct {
	node_t**child;
	int num_children;
    };
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
    NODE(0x13, node_category) \
    NODE(0x14, node_array) \
    NODE(0x15, node_float) \
    NODE(0x16, node_int) \
    NODE(0x17, node_string) \
    NODE(0x18, node_getlocal)  \
    NODE(0x19, node_setlocal) \
    NODE(0x1a, node_inclocal) \
    NODE(0x1b, node_bool_to_float) \
    NODE(0x1c, node_equals) \
    NODE(0x1d, node_arg_max) \
    NODE(0x1e, node_arg_max_i) \
    NODE(0x1f, node_array_at_pos) \

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


/* the following convenience macros allow to write code
   in the following matter:

   START_CODE
    IF 
      GT
	ADD
	  VAR(1)
	  VAR(2)
        VAR(3)
    THEN
      RETURN(int_constant(1))
    ELSE
      RETURN(int_constant(2))
   END_CODE

*/
#define NODE_BEGIN(new_type,args...) \
	    do { \
		node_t* new_node = node_new_with_args(new_type,##args); \
                if(current_node) { \
                    node_append_child(current_node, (new_node)); \
                } else { \
                    if(current_program) { \
                        assert(!(*current_program)); \
                        (*current_program) = new_node; \
                    } \
                } \
		if((new_node->type->flags) & NODE_FLAG_HAS_CHILDREN) { \
		    new_node->parent = current_node; \
		    current_node = new_node; \
		} \
	    } while(0);

#define INSERT_NODE(new_node) \
            do { \
                node_append_child(current_node, (new_node)); \
            } while(0);

#define NODE_CLOSE do {assert(current_node->num_children >= current_node->type->min_args && \
                           current_node->num_children <= current_node->type->max_args);\
                       current_node = current_node->parent; \
                   } while(0)

#define START_CODE(program) \
	node_t* program = 0; \
	{ \
	    node_t*current_node = 0; \
            node_t**current_program = &program;

#define END_CODE \
	}

#define END NODE_CLOSE
#define END_BLOCK NODE_CLOSE
#define IF NODE_BEGIN(&node_if)
#define NOT NODE_BEGIN(&node_not)
#define THEN assert(current_node && current_node->type == &node_if && current_node->num_children == 1);
#define ELSE assert(current_node && current_node->type == &node_if && current_node->num_children == 2);
#define ADD NODE_BEGIN(&node_add)
#define SUB NODE_BEGIN(&node_sub)
#define LT NODE_BEGIN(&node_lt)
#define LTE NODE_BEGIN(&node_lte)
#define GT NODE_BEGIN(&node_gt)
#define IN NODE_BEGIN(&node_in)
#define MUL NODE_BEGIN(&node_mul)
#define DIV NODE_BEGIN(&node_div)
#define EXP NODE_BEGIN(&node_exp)
#define SQR NODE_BEGIN(&node_sqr)
#define NEG NODE_BEGIN(&node_neg)
#define ABS NODE_BEGIN(&node_abs)
#define NOP NODE_BEGIN(&node_nop)
#define VAR(i) NODE_BEGIN(&node_var, i)
#define RETURN(c) do {NODE_BEGIN(&node_constant, c)}while(0);
#define CONSTANT(c) do {NODE_BEGIN(&node_constant, c)}while(0);
#define RETURN_STRING(s) do {VERIFY_STRING(s);NODE_BEGIN(&node_string, string_constant(s))}while(0);
#define FLOAT_CONSTANT(f) NODE_BEGIN(&node_float, f)
#define STRING_CONSTANT(s) NODE_BEGIN(&node_string, s)
#define ARRAY_CONSTANT(args...) NODE_BEGIN(&node_array, ##args)
#define INT_CONSTANT(args...) NODE_BEGIN(&node_int, ##args)
#define SETLOCAL(i) NODE_BEGIN(&node_setlocal, i)
#define GETLOCAL(i) NODE_BEGIN(&node_getlocal, i)
#define INCLOCAL(i) NODE_BEGIN(&node_inclocal, i)
#define BOOL_TO_FLOAT NODE_BEGIN(&node_bool_to_float)
#define EQUALS NODE_BEGIN(&node_equals)
#define BLOCK NODE_BEGIN(&node_block)
#define ARG_MAX NODE_BEGIN(&node_arg_max)
#define ARG_MAX_I NODE_BEGIN(&node_arg_max_i)
#define ARRAY_AT_POS NODE_BEGIN(&node_array_at_pos)

#define VERIFY_INT(n) do{if(0)(((char*)0)[(n)]);}while(0)
#define VERIFY_STRING(s) do{if(0){(s)[0];};}while(0)

#ifdef __cplusplus
}
#endif


#endif
