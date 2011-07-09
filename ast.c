/* ast.c

   AST representation of prediction programs.

   Copyright (c) 2010,2011 Matthias Kramm <kramm@quiss.org>

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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <limits.h>
#include <assert.h>
#include <math.h>
#include "ast.h"

#define EVAL_HEADER_2_LR(v1,v2) \
    constant_t v1 = n->child[0]->type->eval(n->child[0],env);\
    constant_t v2 = n->child[1]->type->eval(n->child[1],env);\
    constant_t r;

#define EVAL_HEADER_2(v1,v2) EVAL_HEADER_2_LR(v1,v2)

#define EVAL_CHILD(i) ((n)->child[(i)]->type->eval((n)->child[(i)],env))

// -------------------------- block node -------------------------------

constant_t node_block_eval(node_t*n, environment_t* env)
{
    int t;
    for(t=0;t<n->num_children-1;t++) {
	EVAL_CHILD(t);
    }
    return EVAL_CHILD(n->num_children-1);
}
nodetype_t node_block =
{
name:"block",
flags:NODE_FLAG_HAS_CHILDREN,
eval:node_block_eval,
min_args:1,
max_args:INT_MAX,
};

// -------------------------- x + y -----------------------------------

constant_t node_add_eval(node_t*n, environment_t* env)
{
    double sum = 0;
    int t;
    for(t=0;t<n->num_children;t++) {
	sum += AS_FLOAT(EVAL_CHILD(t));
    }
    return float_constant(sum);
}
nodetype_t node_add =
{
name:"add",
flags:NODE_FLAG_INFIX|NODE_FLAG_HAS_CHILDREN,
eval:node_add_eval,
min_args:1,
max_args:INT_MAX,
};

// -------------------------- x - y -----------------------------------

constant_t node_sub_eval(node_t*n, environment_t* env)
{
    EVAL_HEADER_2(left,right);
    return float_constant(AS_FLOAT(left) - AS_FLOAT(right));
}
nodetype_t node_sub =
{
name:"minus",
flags:NODE_FLAG_INFIX|NODE_FLAG_HAS_CHILDREN,
eval: node_sub_eval,
min_args:2,
max_args:2,
};

// -------------------------- x * y -----------------------------------

constant_t node_mul_eval(node_t*n, environment_t* env)
{
    EVAL_HEADER_2(left,right);
    return float_constant(AS_FLOAT(left) * AS_FLOAT(right));
}
nodetype_t node_mul =
{
name:"mul",
flags:NODE_FLAG_INFIX|NODE_FLAG_HAS_CHILDREN,
eval: node_mul_eval,
min_args:2,
max_args:2,
};

// -------------------------- x / y -----------------------------------

constant_t node_div_eval(node_t*n, environment_t* env)
{
    EVAL_HEADER_2(left,right);
    return float_constant(AS_FLOAT(left) / AS_FLOAT(right));
}
nodetype_t node_div =
{
name:"div",
flags:NODE_FLAG_INFIX|NODE_FLAG_HAS_CHILDREN,
eval: node_div_eval,
min_args:2,
max_args:2,
};

// -------------------------- exp x ----------------------------------

constant_t node_exp_eval(node_t*n, environment_t* env)
{
    constant_t v = EVAL_CHILD(0);
    return float_constant(exp(AS_FLOAT(v)));
}
nodetype_t node_exp =
{
name:"exp",
flags:NODE_FLAG_HAS_CHILDREN,
eval: node_exp_eval,
min_args:1,
max_args:1,
};

// -------------------------- sqr x ----------------------------------

constant_t node_sqr_eval(node_t*n, environment_t* env)
{
    double v = AS_FLOAT(EVAL_CHILD(0));
    return float_constant(v*v);
}
nodetype_t node_sqr =
{
name:"sqr",
flags:NODE_FLAG_HAS_CHILDREN,
eval: node_sqr_eval,
min_args:1,
max_args:1,
};

// -------------------------- not x ----------------------------------

constant_t node_not_eval(node_t*n, environment_t* env)
{
    constant_t condition = EVAL_CHILD(0);
    return bool_constant(!AS_BOOL(condition));
}
nodetype_t node_not =
{
name:"not",
flags:NODE_FLAG_HAS_CHILDREN,
eval: node_not_eval,
min_args:1,
max_args:1,
};

// -------------------------- neg x ----------------------------------

constant_t node_neg_eval(node_t*n, environment_t* env)
{
    constant_t value = EVAL_CHILD(0);
    return float_constant(-AS_FLOAT(value));
}
nodetype_t node_neg =
{
name:"neg",
flags:NODE_FLAG_HAS_CHILDREN,
eval: node_neg_eval,
min_args:1,
max_args:1,
};

// -------------------------- abs x ----------------------------------

constant_t node_abs_eval(node_t*n, environment_t* env)
{
    constant_t value = EVAL_CHILD(0);
    return float_constant(fabs(AS_FLOAT(value)));
}
nodetype_t node_abs =
{
name:"abs",
flags:NODE_FLAG_HAS_CHILDREN,
eval: node_abs_eval,
min_args:1,
max_args:1,
};

// -------------------------- nop ----------------------------------

constant_t node_nop_eval(node_t*n, environment_t* env)
{
    return missing_constant();
}
nodetype_t node_nop =
{
name:"nop",
flags:0,
eval: node_nop_eval,
min_args:0,
max_args:0,
};

// -------------------------- brackets ----------------------------------

constant_t node_brackets_eval(node_t*n, environment_t* env)
{
    return EVAL_CHILD(0);
}
nodetype_t node_brackets =
{
name:"()",
flags:NODE_FLAG_HAS_CHILDREN,
eval: node_brackets_eval,
min_args:1,
max_args:1,
};

// -------------------------- return ----------------------------------

constant_t node_return_eval(node_t*n, environment_t* env)
{
    return EVAL_CHILD(0);
}
nodetype_t node_return =
{
name:"return",
flags:NODE_FLAG_HAS_CHILDREN,
eval: node_return_eval,
min_args:1,
max_args:1,
};

// -------------------------- float(b) ----------------------------------

constant_t node_bool_to_float_eval(node_t*n, environment_t* env)
{
    constant_t b = EVAL_CHILD(0);
    return float_constant(AS_BOOL(b));
}
nodetype_t node_bool_to_float =
{
name:"bool_to_float",
flags:NODE_FLAG_HAS_CHILDREN,
eval: node_bool_to_float_eval,
min_args:1,
max_args:1,
};

// -------------------------- x < y -----------------------------------

constant_t node_lt_eval(node_t*n, environment_t* env)
{
    EVAL_HEADER_2(left,right);
    return bool_constant(AS_FLOAT(left) < AS_FLOAT(right));
}
nodetype_t node_lt =
{
name:"lt",
flags:NODE_FLAG_INFIX|NODE_FLAG_HAS_CHILDREN,
eval: node_lt_eval,
min_args:2,
max_args:2,
};

// -------------------------- x <= y -----------------------------------

constant_t node_lte_eval(node_t*n, environment_t* env)
{
    EVAL_HEADER_2(left,right);
    return bool_constant(AS_FLOAT(left) <= AS_FLOAT(right));
}
nodetype_t node_lte =
{
name:"lte",
flags:NODE_FLAG_INFIX|NODE_FLAG_HAS_CHILDREN,
eval: node_lte_eval,
min_args:2,
max_args:2,
};

// -------------------------- x > y -----------------------------------

constant_t node_gt_eval(node_t*n, environment_t* env)
{
    EVAL_HEADER_2(left,right);
    return bool_constant(AS_FLOAT(left) > AS_FLOAT(right));
}
nodetype_t node_gt =
{
name:"gt",
flags:NODE_FLAG_INFIX|NODE_FLAG_HAS_CHILDREN,
eval: node_gt_eval,
min_args:2,
max_args:2,
};

// -------------------------- x >= y -----------------------------------

constant_t node_gte_eval(node_t*n, environment_t* env)
{
    EVAL_HEADER_2(left,right);
    return bool_constant(AS_FLOAT(left) >= AS_FLOAT(right));
}
nodetype_t node_gte =
{
name:"gte",
flags:NODE_FLAG_INFIX|NODE_FLAG_HAS_CHILDREN,
eval: node_gte_eval,
min_args:2,
max_args:2,
};

// -------------------------- x == y -----------------------------------

constant_t node_equals_eval(node_t*n, environment_t* env)
{
    EVAL_HEADER_2(left,right);
    return bool_constant(constant_equals(&left,&right));
}
nodetype_t node_equals =
{
name:"equals",
flags:NODE_FLAG_INFIX|NODE_FLAG_HAS_CHILDREN,
eval: node_equals_eval,
min_args:2,
max_args:2,
};

// -------------------------- arg_max ------------------------------------

constant_t node_arg_max_eval(node_t*n, environment_t* env)
{
    float max = AS_FLOAT(EVAL_CHILD(0));
    int index = 0;
    int t;
    for(t=1;t<n->num_children;t++) {
        float c = AS_FLOAT(EVAL_CHILD(t));
        if(c>max) {
            max = c;
            index = t;
        }
    }
    return int_constant(index);
}
nodetype_t node_arg_max =
{
name:"arg_max",
flags:NODE_FLAG_HAS_CHILDREN,
eval: node_arg_max_eval,
min_args:1,
max_args:INT_MAX,
};

// -------------------------- arg_max_i ----------------------------------

constant_t node_arg_max_i_eval(node_t*n, environment_t* env)
{
    int max = AS_INT(EVAL_CHILD(0));
    int index = 0;
    int t;
    for(t=1;t<n->num_children;t++) {
        int c = AS_INT(EVAL_CHILD(t));
        if(c>max) {
            max = c;
            index = t;
        }
    }
    return int_constant(index);
}
nodetype_t node_arg_max_i =
{
name:"arg_max_i",
flags:NODE_FLAG_HAS_CHILDREN,
eval: node_arg_max_i_eval,
min_args:1,
max_args:INT_MAX,
};

// -------------------------- array_at_pos ------------------------------

constant_t node_array_at_pos_eval(node_t*n, environment_t* env)
{
    EVAL_HEADER_2(array,index);
    return AS_ARRAY(array)->entries[AS_INT(index)];
}
nodetype_t node_array_at_pos =
{
name:"array_at_pos",
flags:NODE_FLAG_HAS_CHILDREN,
eval: node_array_at_pos_eval,
min_args:2,
max_args:2,
};

// -------------------------- array_at_pos_inc --------------------------

constant_t node_array_at_pos_inc_eval(node_t*n, environment_t* env)
{
    EVAL_HEADER_2(array,index);
    int i = AS_INT(index);
    array_t*a = AS_ARRAY(array);
    a->entries[i] = int_constant(AS_INT(a->entries[i]) + 1);
    return a->entries[i];
}
nodetype_t node_array_at_pos_inc =
{
name:"array_at_pos_inc",
flags:NODE_FLAG_HAS_CHILDREN,
eval: node_array_at_pos_inc_eval,
min_args:2,
max_args:2,
};

// -------------------------- array_arg_max_i --------------------------

constant_t node_array_arg_max_i_eval(node_t*n, environment_t* env)
{
    constant_t _array = EVAL_CHILD(0);
    array_t*array = AS_ARRAY(_array);
    int max = AS_INT(array->entries[0]);
    int index = 0;
    int t;
    for(t=1;t<array->size;t++) {
        int c = AS_INT(array->entries[t]);
        if(c>max) {
            max = c;
            index = t;
        }
    }
    return int_constant(index);
}
nodetype_t node_array_arg_max_i =
{
name:"array_arg_max_i",
flags:NODE_FLAG_HAS_CHILDREN,
eval: node_array_arg_max_i_eval,
min_args:1,
max_args:1,
};

// -------------------------- string_array ------------------------------------

constant_t node_string_array_eval(node_t*n, environment_t* env)
{
    return n->value;
}
nodetype_t node_string_array =
{
name:"string_array",
flags:NODE_FLAG_ARRAY|NODE_FLAG_HAS_VALUE,
eval: node_string_array_eval,
min_args:0,
max_args:0,
};

// -------------------------- float_array ------------------------------------

constant_t node_float_array_eval(node_t*n, environment_t* env)
{
    return n->value;
}
nodetype_t node_float_array =
{
name:"float_array",
flags:NODE_FLAG_ARRAY|NODE_FLAG_HAS_VALUE,
eval: node_float_array_eval,
min_args:0,
max_args:0,
};

// -------------------------- category_array ------------------------------------

constant_t node_category_array_eval(node_t*n, environment_t* env)
{
    return n->value;
}
nodetype_t node_category_array =
{
name:"category_array",
flags:NODE_FLAG_ARRAY|NODE_FLAG_HAS_VALUE,
eval: node_category_array_eval,
min_args:0,
max_args:0,
};

// -------------------------- int_array ------------------------------------

constant_t node_int_array_eval(node_t*n, environment_t* env)
{
    return n->value;
}
nodetype_t node_int_array =
{
name:"int_array",
flags:NODE_FLAG_ARRAY|NODE_FLAG_HAS_VALUE,
eval: node_int_array_eval,
min_args:0,
max_args:0,
};

// -------------------------- mixed_array ------------------------------------

constant_t node_mixed_array_eval(node_t*n, environment_t* env)
{
    return n->value;
}
nodetype_t node_mixed_array =
{
name:"mixed_array",
flags:NODE_FLAG_ARRAY|NODE_FLAG_HAS_VALUE,
eval: node_mixed_array_eval,
min_args:0,
max_args:0,
};

// -------------------------- zero_int_array --------------------------------

constant_t node_zero_int_array_eval(node_t*n, environment_t* env)
{
    array_fill(n->value.a, int_constant(0));
    return n->value;
}
nodetype_t node_zero_int_array =
{
name:"zero_int_array",
flags:NODE_FLAG_ARRAY|NODE_FLAG_HAS_VALUE,
eval: node_zero_int_array_eval,
min_args:0,
max_args:0,
};

// -------------------------- float ------------------------------------

constant_t node_float_eval(node_t*n, environment_t* env)
{
    return n->value;
}
nodetype_t node_float =
{
name:"float",
flags:NODE_FLAG_HAS_VALUE,
eval: node_float_eval,
min_args:0,
max_args:0,
};

// -------------------------- int ------------------------------------

constant_t node_int_eval(node_t*n, environment_t* env)
{
    return n->value;
}
nodetype_t node_int =
{
name:"int",
flags:NODE_FLAG_HAS_VALUE,
eval: node_int_eval,
min_args:0,
max_args:0,
};

// -------------------------- x in y ----------------------------------

constant_t node_in_eval(node_t*n, environment_t* env)
{
    EVAL_HEADER_2(left,right);
    int i;
    array_t* a = AS_ARRAY(right);
    for(i=0;i<a->size;i++) {
        if(constant_equals(&left, &a->entries[i]))
            return bool_constant(true);
    }
    return bool_constant(false);
}
nodetype_t node_in =
{
name:"in",
flags:NODE_FLAG_INFIX|NODE_FLAG_HAS_CHILDREN,
eval: node_in_eval,
min_args:2,
max_args:2,
};

// -------------------- if(x) then y else z ----------------------------

constant_t node_if_eval(node_t*n, environment_t* env)
{
    bool condition = AS_BOOL(EVAL_CHILD(0));
    if(condition == true) {
	return EVAL_CHILD(1);
    } else {
	return EVAL_CHILD(2);
    }
}
nodetype_t node_if =
{
name:"if",
flags:NODE_FLAG_HAS_CHILDREN,
eval: node_if_eval,
min_args:3,
max_args:3,
};

// ---------------------- var i ---------------------------------------

constant_t node_param_eval(node_t*n, environment_t* env)
{
    assert(n->value.i >= 0 && n->value.i < env->row->num_inputs);
    variable_t v = env->row->inputs[n->value.i];
    if(v.type == CATEGORICAL) {
	return category_constant(v.category);
    } else if(v.type == CONTINUOUS) {
	return float_constant(v.value);
    } else if(v.type == MISSING) {
	return missing_constant();
    } else if(v.type == TEXT) {
	return string_constant(v.text);
    } else {
	assert(!"bad type for input value");
    }
}
nodetype_t node_param =
{
name:"param",
flags:NODE_FLAG_HAS_VALUE,
eval: node_param_eval,
min_args:0,
max_args:0,
};

// ---------------------- setlocal i ----------------------------------

constant_t node_setlocal_eval(node_t*n, environment_t* env)
{
    int index = n->value.i;
    constant_t value = EVAL_CHILD(0);
    assert(index >= 0 && index < env->num_locals);
    env->locals[index] = value;
    return value;
}
nodetype_t node_setlocal =
{
name:"setlocal",
flags:NODE_FLAG_HAS_CHILDREN|NODE_FLAG_HAS_VALUE,
eval: node_setlocal_eval,
min_args:1,
max_args:1,
};

// ---------------------- getlocal i ----------------------------------

constant_t node_getlocal_eval(node_t*n, environment_t* env)
{
    int index = n->value.i;
    assert(index >= 0 && index < env->num_locals);
    bool local_is_not_undefined = env->locals[index].type != 0;
    assert(local_is_not_undefined);
    return env->locals[index];
}
nodetype_t node_getlocal =
{
name:"getlocal",
flags:NODE_FLAG_HAS_VALUE,
eval: node_getlocal_eval,
min_args:0,
max_args:0,
};

// ---------------------- inclocal i ----------------------------------

constant_t node_inclocal_eval(node_t*n, environment_t* env)
{
    int index = n->value.i;
    assert(index >= 0 && index < env->num_locals);
    bool local_is_not_undefined = env->locals[index].type != 0;
    assert(local_is_not_undefined);
    assert(env->locals[index].type == CONSTANT_INT);
    env->locals[index].i++;
    return env->locals[index];
}
nodetype_t node_inclocal =
{
name:"inclocal",
flags:NODE_FLAG_HAS_VALUE,
eval: node_inclocal_eval,
min_args:0,
max_args:0,
};

// ---------------------- category i (return i) -------------------------

constant_t node_category_eval(node_t*n, environment_t* env)
{
    return category_constant(n->value.c);
}
nodetype_t node_category =
{
name:"category",
flags:NODE_FLAG_HAS_VALUE,
eval: node_category_eval,
min_args:0,
max_args:0,
};

// ---------------------- string s --------------------------------------

constant_t node_string_eval(node_t*n, environment_t* env)
{
    return n->value;
}
nodetype_t node_string =
{
name:"string",
flags:NODE_FLAG_HAS_VALUE,
eval: node_string_eval,
min_args:0,
max_args:0,
};

// ---------------------- bool b --------------------------------------

constant_t node_bool_eval(node_t*n, environment_t* env)
{
    return n->value;
}
nodetype_t node_bool =
{
name:"bool",
flags:NODE_FLAG_HAS_VALUE,
eval: node_bool_eval,
min_args:0,
max_args:0,
};

// ---------------------- missing --------------------------------------

constant_t node_missing_eval(node_t*n, environment_t* env)
{
    return n->value;
}
nodetype_t node_missing =
{
name:"missing",
flags:NODE_FLAG_HAS_VALUE,
eval: node_missing_eval,
min_args:0,
max_args:0,
};

// ---------------------- constant -------------------------------------

constant_t node_constant_eval(node_t*n, environment_t* env)
{
    return n->value;
}
nodetype_t node_constant =
{
name:"constant",
flags:NODE_FLAG_HAS_VALUE,
eval: node_constant_eval,
min_args:0,
max_args:0,
};

// ======================== node list ==================================

nodetype_t* nodes[] = {
#define NODE(opcode, name) &name,
LIST_NODES
#undef NODE
};

void nodelist_init()
{
    static bool is_initialized = false;
    if(is_initialized) 
        return;
#   define NODE(op, name) name._opcode = op;
    LIST_NODES
    #undef NODE
    is_initialized = true;
}

uint8_t node_get_opcode(node_t*n)
{
    nodelist_init();
    return n->type->_opcode;
}

// ======================== node handling ==============================

node_t* node_new(nodetype_t*t, node_t*parent)
{
    node_t*n = (node_t*)malloc(sizeof(node_t));
    n->type = t;
    n->parent = parent;
    n->child = 0;
    n->num_children = 0;
    n->value = missing_constant();
    return n;
}

node_t* node_new_with_args(nodetype_t*t,...)
{
    node_t*n = node_new(t, 0);
    va_list arglist;
    va_start(arglist, t);
    switch(node_get_opcode(n)) {
        case opcode_node_param:
	    n->value = int_constant(va_arg(arglist,int));
            break;
        case opcode_node_category:
	    n->value = category_constant(va_arg(arglist,category_t));
            break;
        case opcode_node_float:
	    n->value = float_constant(va_arg(arglist,double));
            break;
        case opcode_node_int:
	    n->value = int_constant(va_arg(arglist,int));
            break;
        case opcode_node_mixed_array:
	    n->value = mixed_array_constant(va_arg(arglist,array_t*));
            break;
        case opcode_node_string_array:
	    n->value = string_array_constant(va_arg(arglist,array_t*));
            break;
        case opcode_node_float_array:
	    n->value = float_array_constant(va_arg(arglist,array_t*));
            break;
        case opcode_node_int_array:
	    n->value = int_array_constant(va_arg(arglist,array_t*));
            break;
        case opcode_node_category_array:
	    n->value = category_array_constant(va_arg(arglist,array_t*));
            break;
        case opcode_node_string:
	    n->value = string_constant(va_arg(arglist,char*));
            break;
        case opcode_node_bool:
	    n->value = bool_constant(va_arg(arglist,int));
            break;
        case opcode_node_missing:
	    n->value = missing_constant();
            break;
        case opcode_node_zero_int_array: {
	    int size = va_arg(arglist,int);
            array_t*a = array_new(size);
            array_fill(a, int_constant(0));
            n->value = int_array_constant(a);
            break;
        }
        case opcode_node_constant:
	    n->value = va_arg(arglist,constant_t);
            break;
        case opcode_node_setlocal:
        case opcode_node_getlocal:
        case opcode_node_inclocal:
	    n->value = int_constant(va_arg(arglist,int));
            break;
    }
    va_end(arglist);
    return n;
}

node_t* node_new_array(array_t*a)
{
    if(!a->size) {
        return node_new_with_args(&node_float_array, a);
    }
    if(!array_is_homogeneous(a))
        return node_new_with_args(&node_mixed_array, a);
    switch(a->entries[0].type) {
        case CONSTANT_FLOAT:
            return node_new_with_args(&node_float_array, a);
        case CONSTANT_INT:
            return node_new_with_args(&node_int_array, a);
        case CONSTANT_CATEGORY:
            return node_new_with_args(&node_category_array, a);
        case CONSTANT_BOOL:
            assert(0);
            //return node_new_with_args(&node_bool_array, a);
        case CONSTANT_STRING:
            return node_new_with_args(&node_string_array, a);
        case CONSTANT_MISSING:
            assert(0);
            //return node_new_with_args(&node_missing_array, a);
        default:
            fprintf(stderr,"Arrays of arrays not supported yet");
            assert(0);
    }
    return 0;
}

void node_append_child(node_t*n, node_t*child)
{
    assert(n);
    assert(n->type);
    assert(n->type->name);
    if(n->num_children >= n->type->max_args) { \
        fprintf(stderr, "Too many arguments (%d) to node %s (max %d args)\n", \
                n->num_children, \
                n->type->name, \
                n->type->max_args); \
    }
    assert(n->num_children < n->type->max_args);

    child->parent = n;

    if(!n->num_children) {
	// first child
	n->child = malloc(1*sizeof(node_t*));
	((node_t**)n->child)[0] = child;
	n->num_children++;
	return;
    }

    int size = n->num_children;
    int highest_bit;
    do {
	highest_bit = size;
	size = size&(size-1);
    } while(size);

    if(n->num_children == highest_bit) {
	n->child = realloc((void*)n->child, (highest_bit<<1)*sizeof(node_t*));
    }
    ((node_t**)n->child)[n->num_children++] = child;
}

void node_set_child(node_t*n, int num, node_t*child)
{
    assert(num >= 0 && num < n->num_children);
    ((node_t**)n->child)[num] = child;
    child->parent = n;
}

void node_destroy(node_t*n)
{
    int t;
    if(n->type->flags&NODE_FLAG_HAS_VALUE) {
        constant_clear(&n->value);
    }
    if((n->type->flags&NODE_FLAG_HAS_CHILDREN) && n->child) {
	for(t=0;t<n->num_children;t++) {
	    node_destroy(n->child[t]);((node_t**)n->child)[t] = 0;
	}
	free((void*)n->child);
    }
    free(n);
}

void node_destroy_self(node_t*n)
{
    int t;
    if(n->type->flags&NODE_FLAG_HAS_VALUE) {
        constant_clear(&n->value);
    }
    free(n);
}

constant_t node_eval(node_t*n,environment_t* e)
{
    return n->type->eval(n, e);
}

/*
 |
 +-add
 | |
 | +-code
 | |
 | +-code
 |   |
 |   +
 | 
 +-div

*/
void node_print2(node_t*n, const char*p1, const char*p2, FILE*fi)
{
    if(n->type->flags & NODE_FLAG_HAS_VALUE) {
        fprintf(fi, "%s%s (", p1, n->type->name);
        constant_print(&n->value);
        fprintf(fi, ")\n");
    } else {
        fprintf(fi, "%s%s\n", p1, n->type->name);
    }
    if(n->type->flags&NODE_FLAG_HAS_CHILDREN) {
        int t;
        char*o2 = malloc(strlen(p2)+3);
        strcpy(o2, p2);strcat(o2, "| ");
        char*o3 = malloc(strlen(p2)+3);
        strcpy(o3, p2);strcat(o3, "+-");
        char*o4 = malloc(strlen(p2)+3);
        strcpy(o4, p2);strcat(o4, "  ");

        for(t=0;t<n->num_children;t++) {
            fprintf(fi, "%s\n", o2);
            node_print2(n->child[t], o3, t<n->num_children-1?o2:o4, fi);
        }
        free(o2);
        free(o3);
        free(o4);
    }
}

bool node_sanitycheck(node_t*n)
{
    int t;
    for(t=0;t<n->num_children;t++) {
	if(!n->child[t])
            return false;
	if(n->child[t] == n)
            return false;
	if(n->child[t]->parent != n) {
	    printf("%s -> %s (references back to: %s)", 
		    n->type->name, n->child[t]->type->name, 
		    n->child[t]->parent?n->child[t]->parent->type->name:"(null)"
		    );
	}
	if(n->child[t]->parent != n)
            return false;
	node_sanitycheck(n->child[t]);
    }
    return true;
}

void node_remove_child(node_t*n, int num)
{
    assert(num < n->num_children);
    int t;
    for(t=num;t<n->num_children-1;t++) {
        ((node_t**)n->child)[t] = n->child[t+1];
    }
    n->num_children--;
}

bool node_is_array(node_t*n)
{
    return !!(n->type->flags & NODE_FLAG_ARRAY);
}

void node_print(node_t*n)
{
    printf("------------ code -------------\n");
    node_print2(n,"","",stdout);
    printf("-------------------------------\n");
}

