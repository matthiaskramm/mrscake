#ifndef __easy_ast__
#define __easy_ast__

#include "ast.h"

/* The macros in this file allow you to write code
   in the following matter:

   START_CODE
    IF 
      GT
	ADD
	  PARAM(1)
	  PARAM(2)
        END;
        PARAM(3)
      END;
    THEN
      INT_CONSTANT(1)
    ELSE
      INT_CONSTANT(2)
    END;
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
#define BLOCK NODE_BEGIN(&node_block)

#define IF NODE_BEGIN(&node_if)
#define THEN assert(current_node && current_node->type == &node_if && current_node->num_children == 1);
#define ELSE assert(current_node && current_node->type == &node_if && current_node->num_children == 2);

#define ADD NODE_BEGIN(&node_add)
#define SUB NODE_BEGIN(&node_sub)
#define IN NODE_BEGIN(&node_in)

#define LT NODE_BEGIN(&node_lt)
#define LTE NODE_BEGIN(&node_lte)
#define GT NODE_BEGIN(&node_gt)
#define GTE NODE_BEGIN(&node_gte)
#define MUL NODE_BEGIN(&node_mul)
#define DIV NODE_BEGIN(&node_div)
#define EQUALS NODE_BEGIN(&node_equals)

#define NOT NODE_BEGIN(&node_not)
#define EXP NODE_BEGIN(&node_exp)
#define SQR NODE_BEGIN(&node_sqr)
#define NEG NODE_BEGIN(&node_neg)
#define ABS NODE_BEGIN(&node_abs)
#define NOP NODE_BEGIN(&node_nop)

#define PARAM(i) NODE_BEGIN(&node_param, i)

#define GENERIC_CONSTANT(c) do {NODE_BEGIN(&node_constant, c)}while(0);
#define BOOL_CONSTANT(b) NODE_BEGIN(&node_bool, ((int)(b)))
#define MISSING_CONSTANT NODE_BEGIN(&node_missing)
#define FLOAT_CONSTANT(f) NODE_BEGIN(&node_float, f)
#define STRING_CONSTANT(s) do {VERIFY_STRING(s);NODE_BEGIN(&node_string, s)}while(0);
#define ARRAY_CONSTANT(args...) NODE_BEGIN(&node_array, ##args)
#define INT_CONSTANT(args...) NODE_BEGIN(&node_int, ##args)
#define CATEGORY_CONSTANT(args...) NODE_BEGIN(&node_category, ##args)

#define SETLOCAL(i) NODE_BEGIN(&node_setlocal, i)
#define GETLOCAL(i) NODE_BEGIN(&node_getlocal, i)
#define INCLOCAL(i) NODE_BEGIN(&node_inclocal, i)

#define BOOL_TO_FLOAT NODE_BEGIN(&node_bool_to_float)
#define ARG_MAX_F NODE_BEGIN(&node_arg_max)
#define ARG_MAX_I NODE_BEGIN(&node_arg_max_i)
#define ARRAY_AT_POS NODE_BEGIN(&node_array_at_pos)

#define ARRAY_AT_POS_INC NODE_BEGIN(&node_array_at_pos_inc)
#define ARRAY_ARG_MAX_I NODE_BEGIN(&node_array_arg_max_i)
#define ARRAY_NEW(size) NODE_BEGIN(&node_zero_array, size)

#define VERIFY_INT(n) do{if(0)(((char*)0)[(n)]);}while(0)
#define VERIFY_STRING(s) do{if(0){(s)[0];};}while(0)

#endif // __easy_ast__
