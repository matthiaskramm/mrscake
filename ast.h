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
#include "model.h"
#include "constant.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _node node_t;
typedef struct _nodetype nodetype_t;
typedef struct _environment environment_t;

#define NODE_FLAG_HAS_CHILDREN 1
#define NODE_FLAG_HAS_VALUE 2

struct _environment {
    row_t* row;
    constant_t* locals;
    int num_locals;
};

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
    union {
        struct {
            node_t**child;
            int num_children;
        };
	constant_t value;
    };
};

/* all known node types & opcodes */
#define LIST_NODES \
    NODE(0x71, node_root) \
    NODE(0x01, node_if) \
    NODE(0x02, node_add) \
    NODE(0x03, node_sub) \
    NODE(0x04, node_mul) \
    NODE(0x05, node_div) \
    NODE(0x06, node_lt) \
    NODE(0x07, node_lte) \
    NODE(0x08, node_gt) \
    NODE(0x09, node_in) \
    NODE(0x0a, node_not) \
    NODE(0x0b, node_exp) \
    NODE(0x0c, node_sqr) \
    NODE(0x0d, node_var) \
    NODE(0x0e, node_constant) \
    NODE(0x0f, node_category) \
    NODE(0x10, node_array) \
    NODE(0x11, node_float) \
    NODE(0x12, node_string) \
    NODE(0x13, node_getlocal)  \
    NODE(0x14, node_setlocal)

#define NODE(opcode, name) extern nodetype_t name;
LIST_NODES
#undef NODE

extern nodetype_t* nodelist[];
void nodelist_init();
uint8_t node_get_opcode(node_t*n);

node_t* node_new(nodetype_t*t, ...);
void node_append_child(node_t*n, node_t*child);
void node_free(node_t*n);
constant_t node_eval(node_t*n,environment_t* e);
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
		node_t* new_node = node_new(new_type,##args); \
		if(current_node) { \
		    assert(current_node->type); \
		    assert(current_node->type->name); \
		    if(current_node->num_children >= current_node->type->max_args) { \
			fprintf(stderr, "Too many arguments (%d) to node (max %d args)\n", \
				current_node->num_children, \
				current_node->type->max_args); \
		    } \
		    assert(current_node->num_children < current_node->type->max_args); \
		    node_append_child(current_node, new_node); \
		} \
		if((new_node->type->flags) & NODE_FLAG_HAS_CHILDREN) { \
		    new_node->parent = current_node; \
		    current_node = new_node; \
		} \
	    } while(0);

#define NODE_CLOSE do {assert(current_node->num_children >= current_node->type->min_args && \
                           current_node->num_children <= current_node->type->max_args);\
                    current_node = current_node->parent; \
                   } while(0)

#define START_CODE(program) \
	node_t* program; \
	{ \
	    node_t*current_node = program = node_new(&node_root); \
	    program = current_node;

#define END_CODE \
	}

#define END NODE_CLOSE
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
#define VAR(i) NODE_BEGIN(&node_var, i)
#define RETURN(c) do {NODE_BEGIN(&node_constant, c)}while(0);
#define RETURN_STRING(s) do {VERIFY_STRING(s);NODE_BEGIN(&node_string, string_constant(s))}while(0);
#define FLOAT_CONSTANT(f) NODE_BEGIN(&node_float, f)
#define STRING_CONSTANT(s) NODE_BEGIN(&node_string, s)
#define ARRAY_CONSTANT(args...) NODE_BEGIN(&node_array, ##args)
#define SETLOCAL(i) NODE_BEGIN(&node_setlocal, i)
#define GETLOCAL(i) NODE_BEGIN(&node_getlocal, i)

#define VERIFY_INT(n) do{if(0)(((char*)0)[(n)]);}while(0)
#define VERIFY_STRING(s) do{if(0){(s)[0];};}while(0)

#ifdef __cplusplus
}
#endif


#endif
