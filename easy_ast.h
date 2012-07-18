#ifndef __easy_ast__
#define __easy_ast__

#include <assert.h>
#include "ast.h"

/* The macros in this file allow you to write code
   in the following matter:

   START_CODE
    IF 
      GT
        ADD
          PARAM(column1)
          PARAM(column2)
        END;
        PARAM(column3)
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

#define NODE_CLOSE do { \
        if(!(current_node->num_children >= current_node->type->min_args && \
             current_node->num_children <= current_node->type->max_args)) { \
            fprintf(stderr, "%s\n", current_node->type->name); \
        }; \
        assert(current_node->num_children >= current_node->type->min_args && \
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

#define LT_I NODE_BEGIN(&node_lt_i)
#define LTE_I NODE_BEGIN(&node_lte_i)
#define GT_I NODE_BEGIN(&node_gt_i)
#define GTE_I NODE_BEGIN(&node_gte_i)

#define MUL NODE_BEGIN(&node_mul)
#define DIV NODE_BEGIN(&node_div)
#define EQUALS NODE_BEGIN(&node_equals)

#define NOT NODE_BEGIN(&node_not)
#define EXP NODE_BEGIN(&node_exp)
#define SQR NODE_BEGIN(&node_sqr)
#define NEG NODE_BEGIN(&node_neg)
#define ABS NODE_BEGIN(&node_abs)
#define NOP NODE_BEGIN(&node_nop)

#define PARAM(index) NODE_BEGIN(&node_param, index)
#define RAW_PARAM(index) PARAM(index)

#define GENERIC_CONSTANT(c) do {NODE_BEGIN(&node_constant, c)}while(0);
#define BOOL_CONSTANT(b) NODE_BEGIN(&node_bool, ((int)(b)))
#define MISSING_CONSTANT NODE_BEGIN(&node_missing)
#define FLOAT_CONSTANT(f) NODE_BEGIN(&node_float, f)
#define STRING_CONSTANT(s) do {VERIFY_STRING(s);NODE_BEGIN(&node_string, s)}while(0);
#define ARRAY_CONSTANT(a) INSERT_NODE(node_new_array((a)))
#define INT_CONSTANT(args...) NODE_BEGIN(&node_int, ##args)
#define CATEGORY_CONSTANT(args...) NODE_BEGIN(&node_category, ##args)

#define SETLOCAL(i) NODE_BEGIN(&node_setlocal, i)
#define GETLOCAL(i) NODE_BEGIN(&node_getlocal, i)
#define INCLOCAL(i) NODE_BEGIN(&node_inclocal, i)

#define RETURN NODE_BEGIN(&node_return)

#define BOOL_TO_FLOAT NODE_BEGIN(&node_bool_to_float)
#define ARG_MAX_F NODE_BEGIN(&node_arg_max)
#define ARG_MAX_I NODE_BEGIN(&node_arg_max_i)
#define ARG_MIN_F NODE_BEGIN(&node_arg_min)
#define ARG_MIN_I NODE_BEGIN(&node_arg_min_i)
#define ARRAY_AT_POS NODE_BEGIN(&node_array_at_pos)

#define SORT_FLOAT_ARRAY_ASC NODE_BEGIN(&node_sort_float_array_asc)

#define TERM_FREQUENCY NODE_BEGIN(&node_term_frequency)

#define INC_ARRAY_AT_POS NODE_BEGIN(&node_inc_array_at_pos)
#define SET_ARRAY_AT_POS NODE_BEGIN(&node_set_array_at_pos)
#define ARRAY_ARG_MAX_I NODE_BEGIN(&node_array_arg_max_i)

#define NEW_ZERO_INT_ARRAY(size) NODE_BEGIN(&node_zero_int_array, size)
#define NEW_ZERO_FLOAT_ARRAY(size) NODE_BEGIN(&node_zero_float_array, size)

#define VERIFY_INT(n) do{if(0)(((char*)0)[(n)]);}while(0)
#define VERIFY_STRING(s) do{if(0){(s)[0];};}while(0)
        
#define FOR_LOCAL_FROM_N_TO_M(var) NODE_BEGIN(&node_for_local_from_n_to_m, var)

#define DEBUG_PRINT NODE_BEGIN(&node_debug_print)

#endif // __easy_ast__
