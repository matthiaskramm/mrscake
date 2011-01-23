/* ast_transforms.c
   AST transformations and queries

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

#include <stdlib.h>
#include <float.h>
#include <math.h>
#include <memory.h>
#include "ast_transforms.h"

bool node_has_consumer_parent(node_t*n)
{
    while(1) {
        node_t*child = n;
        n = n->parent;
        if(!n)
	    break;
	if(n->type == &node_return) 
	    break;
	if(n->type == &node_setlocal) 
	    return true;
	if(n->type == &node_array_at_pos_inc) 
	    return true;
        if(n->type == &node_block) {
            if(n->child[n->num_children-1] != child)
                return false;
        }
    }
    return true;
}
node_t* node_find_root(node_t*n)
{
    while(n->parent) {
        n = n->parent;
    }
    return n;
}
int node_highest_local(node_t*node)
{
    int max = 0;
    if(node->type == &node_setlocal) {
	max = node->value.i+1;
    }
    int t;
    for(t=0;t<node->num_children;t++) {
	int l = node_highest_local(node->child[t]);
	if(l>max)
	    max = l;
    }
    return max;
}
node_t* node_find_setlocal(node_t*node, int num)
{
    /* Search for the first setlocal() that assigns something
       to this variable.
       We assume the AST is in SSA form */
    int t;
    if(node->type == &node_setlocal &&
       node->value.i == num) {
        return node;
    }
    for(t=0;t<node->num_children;t++) {
        node_t*c = node_find_setlocal(node->child[t], num);
        if(c)
            return c;
    }
    return 0;
}
constant_type_t local_type(node_t*node, int num, model_t*m)
{
    node_t*setlocal = node_find_setlocal(node, num);
    if(!setlocal) {
        fprintf(stderr, "Couldn't find assignment of variable %d\n", num);
        return CONSTANT_MISSING;
    }
    return node_type(setlocal, m);
}
void fill_locals(node_t*node, model_t*m, constant_type_t*types)
{
    if(node->type == &node_setlocal) {
        types[node->value.i] = node_type(node->child[0], m);
    }
    int t;
    for(t=0;t<node->num_children;t++) {
        fill_locals(node->child[t], m, types);
    }
}
constant_type_t*node_local_types(node_t*node, model_t*m, int* num_locals)
{
    int num = *num_locals = node_highest_local(node);
    int t;
    constant_type_t*types = (constant_type_t*)malloc(sizeof(constant_type_t)*num);
    memset(types, -1, sizeof(constant_type_t)*num);
    fill_locals(node, m, types);
    return types;
}
constant_type_t model_param_type(model_t*m, int var)
{
    if(!m->column_types) {
        return CONSTANT_MISSING;
    }
    switch(m->column_types[var]) {
        case CONTINUOUS:
            return CONSTANT_FLOAT;
        break;
        case CATEGORICAL:
            return CONSTANT_CATEGORY;
        break;
        case TEXT:
            return CONSTANT_STRING;
        break;
    }
}
constant_type_t node_type(node_t*n, model_t*m)
{
    switch(node_get_opcode(n)) {
        case opcode_node_arg_max:
        case opcode_node_arg_max_i:
        case opcode_node_array_arg_max_i:
        case opcode_node_array_at_pos_inc:
        case opcode_node_int:
        case opcode_node_inclocal:
            return CONSTANT_INT;
        case opcode_node_nop:
        case opcode_node_missing:
            return CONSTANT_MISSING;
        case opcode_node_in:
        case opcode_node_not:
        case opcode_node_lt:
        case opcode_node_lte:
        case opcode_node_gt:
        case opcode_node_gte:
        case opcode_node_bool:
        case opcode_node_equals:
            return CONSTANT_BOOL;
        case opcode_node_bool_to_float:
        case opcode_node_float:
            return CONSTANT_FLOAT;
        case opcode_node_category:
            return CONSTANT_CATEGORY;
        case opcode_node_int_array:
            return CONSTANT_INT_ARRAY;
        case opcode_node_category_array:
            return CONSTANT_CATEGORY_ARRAY;
        case opcode_node_float_array:
            return CONSTANT_FLOAT_ARRAY;
        case opcode_node_string_array:
            return CONSTANT_STRING_ARRAY;
        case opcode_node_mixed_array:
            return CONSTANT_MIXED_ARRAY;
        case opcode_node_zero_int_array:
            return CONSTANT_INT_ARRAY;
        case opcode_node_string:
            return CONSTANT_STRING;
        case opcode_node_constant:
            return n->value.type;
        case opcode_node_return:
            /* strictly speaking, this node doesn't have a "type" at all
               (since it causes evaluation to terminate), but in order to
               make node_type(root) do the right thing, we treat it as if
               it would cascade its value up the tree */
	    return node_type(n->child[0],m);
        case opcode_node_brackets:
        case opcode_node_sqr:
        case opcode_node_abs:
        case opcode_node_neg:
        case opcode_node_exp:
	case opcode_node_add:
	case opcode_node_sub:
	case opcode_node_mul:
	case opcode_node_div:
        case opcode_node_setlocal:
	    return node_type(n->child[0],m);
	case opcode_node_block:
	    return node_type(n->child[n->num_children-1],m);
	case opcode_node_if:
	    return node_type(n->child[1],m);
        case opcode_node_array_at_pos:
            return node_array_element_type(n->child[0]);
        case opcode_node_param:
            return model_param_type(m, n->value.i);
        case opcode_node_getlocal:
            return local_type(node_find_root(n), n->value.i, m);
	default:
            fprintf(stderr, "Couldn't do type deduction for ast node %s\n", n->type->name);
	    return CONSTANT_MISSING;
    }
}
constant_type_t node_array_element_type(node_t*n)
{
    switch(node_get_opcode(n)) {
        case opcode_node_int_array:
        case opcode_node_zero_int_array:
            return CONSTANT_INT;
        case opcode_node_float_array:
            return CONSTANT_FLOAT;
        case opcode_node_category_array:
            return CONSTANT_CATEGORY;
        case opcode_node_string_array:
            return CONSTANT_STRING;
        case opcode_node_mixed_array:
            return CONSTANT_MISSING;
        case opcode_node_getlocal:
            n = node_find_setlocal(node_find_root(n), n->value.i);
            return node_array_element_type(n->child[0]);
	default:
            fprintf(stderr, "Couldn't do array type deduction for ast node %s\n", n->type->name);
	    return CONSTANT_MISSING;
    }
}
int node_array_size(node_t*n)
{
    switch(node_get_opcode(n)) {
        case opcode_node_float_array:
        case opcode_node_mixed_array:
        case opcode_node_int_array:
        case opcode_node_category_array:
        case opcode_node_string_array:
        case opcode_node_zero_int_array:
            return n->value.a->size;
        case opcode_node_getlocal:
            n = node_find_setlocal(node_find_root(n), n->value.i);
            return node_array_size(n->child[0]);
        case opcode_node_brackets:
            return node_array_size(n->child[0]);
	default:
            fprintf(stderr, "Couldn't do array size deduction for ast node %s\n", n->type->name);
	    return -1;
    }
}
bool node_terminates(node_t*n)
{
    switch(node_get_opcode(n)) {
	case opcode_node_block:
	    return node_terminates(n->child[n->num_children-1]);
	case opcode_node_return:
	    return true;
	case opcode_node_if:
	    return node_terminates(n->child[1]) &&
	           node_terminates(n->child[2]);
	default:
	    return false;
    }
}
bool is_infix(nodetype_t*type)
{
    return !!(type->flags & NODE_FLAG_INFIX);
}
bool node_has_minus_prefix(node_t*n)
{
    switch(node_get_opcode(n)) {
	case opcode_node_int:
	case opcode_node_float:
	case opcode_node_constant:
	    return (n->value.type == CONSTANT_INT && n->value.i<0) ||
	           (n->value.type == CONSTANT_FLOAT && n->value.f<0) ||
	           (n->value.type == CONSTANT_CATEGORY && n->value.c<0);
	default:
	    if(is_infix(n->type))
		return node_has_minus_prefix(n->child[0]);
	    else
		return false;
    }
}
bool node_is_missing(node_t*n)
{
    return(n->type == &node_missing ||
           n->type == &node_nop);
}
node_t* node_add_return(node_t*n)
{
    if(n->type != &node_return) {
	node_t*p = node_new(&node_return, n->parent);
	node_append_child(p, n);
	return p;
    }
    return n;
}
node_t* node_do_cascade_returns(node_t*n) 
{
    switch(node_get_opcode(n)) {
	case opcode_node_block:
	    node_set_child(n, n->num_children-1, node_do_cascade_returns(n->child[n->num_children-1]));
	    return n;
	case opcode_node_if:
	    node_set_child(n, 1, node_do_cascade_returns(n->child[1]));
	    node_set_child(n, 2, node_do_cascade_returns(n->child[2]));
	    return n;
	case opcode_node_nop:
	case opcode_node_setlocal:
	case opcode_node_inclocal:
	    return n;
	case opcode_node_return:
	    return n;
	default: {
	    node_t*p = node_new(&node_return, n->parent);
	    node_append_child(p, n);
	    return p;
	}
    }
}
int node_precedence(node_t*n)
{
    switch(node_get_opcode(n)) {
	case opcode_node_block:
            return 0;
	case opcode_node_if:
            return 1;
	case opcode_node_setlocal:
            return 2;
	case opcode_node_equals:
	case opcode_node_lt:
	case opcode_node_gt:
	case opcode_node_lte:
	case opcode_node_gte:
            return 4;
	case opcode_node_add:
	case opcode_node_sub:
            return 5;
	case opcode_node_mul:
	case opcode_node_div:
            return 6;
	case opcode_node_neg: // -x
	case opcode_node_sqr: // x ** 2
	case opcode_node_exp:
	case opcode_node_abs:
	case opcode_node_array_at_pos:
            return 9;
	case opcode_node_getlocal:
	case opcode_node_param:
            return 10;
        default:
            return 1024;
    }
}
bool lower_precedence(node_t*n1, node_t*n2)
{
    return node_precedence(n1) < node_precedence(n2);
}
node_t* node_insert_brackets(node_t*n) 
{
    int t;
    for(t=0;t<n->num_children;t++) {
	node_set_child(n, t, node_insert_brackets(n->child[t]));
    }
    if(n->parent) {
	if(lower_precedence(n, n->parent)) {
	    node_t*p = node_new(&node_brackets, n->parent);
	    node_append_child(p, n);
	    return p;
	}
    }
    return n;
}
node_t* node_prepare_for_code_generation(node_t*n)
{
    n = node_optimize(n);
    n = node_do_cascade_returns(n);
    n = node_insert_brackets(n);
    n = node_optimize(n);
    return n;
}
bool node_has_child(node_t*node, nodetype_t*type)
{
    if(node->type == type)
        return true;
    int t;
    for(t=0;t<node->num_children;t++) {
        bool b = node_has_child(node->child[t], type);
        if(b)
            return true;
    }
    return false;
}
bool float_is_zero(double f)
{
    return (fabs(f)<0.000001);
}
bool float_is_one(double f)
{
    return (fabs(f-1.0)<0.000001);
}
node_t* node_optimize(node_t*n)
{
    int t,num;
    switch(node_get_opcode(n)) {
	case opcode_node_block:
            for(t=0;t<n->num_children;t++) {
                if(n->child[t]->type == &node_nop) {
                    node_remove_child(n, t--);
                }
            }
            break;
	case opcode_node_in:
            /* convert 
                    a in [x]
               to
                    a == x
             */
            if(node_is_array(n->child[1]) &&
               n->child[1]->value.a->size == 1) {
                node_t*new_node = node_new(&node_equals, n->parent);
                node_t*constant = node_new_with_args(&node_constant, n->child[1]->value.a->entries[0]);
                node_append_child(new_node, n->child[0]);
                node_append_child(new_node, constant);
                node_destroy_self(n);
                return new_node;
            }
            break;
	case opcode_node_mul:
            /* convert 
                    a * 1.0
               or
                    1.0 * a
               to
                    a
             */
            if(n->child[1]->type == &node_float &&
               float_is_one(n->child[1]->value.f)) {
                node_t*new_node = n->child[0];
                node_destroy(n->child[1]);
                node_destroy_self(n);
                return new_node;
            }
            if(n->child[0]->type == &node_float &&
               float_is_one(n->child[0]->value.f)) {
                node_t*new_node = n->child[1];
                node_destroy(n->child[0]);
                node_destroy_self(n);
                return new_node;
            }
            /* convert 
                    a * 0.0
               or
                    0.0 * a
               to
                    0.0
             */
            if(n->child[0]->type == &node_float &&
               float_is_zero(n->child[0]->value.f)) {
                node_t*new_node = n->child[0];
                node_destroy(n->child[1]);
                node_destroy_self(n);
                return new_node;
            }
            if(n->child[1]->type == &node_float &&
               float_is_zero(n->child[1]->value.f)) {
                node_t*new_node = n->child[1];
                node_destroy(n->child[0]);
                node_destroy_self(n);
                return new_node;
            }
            break;
	case opcode_node_sub:
            /* convert 
                    a - 0.0
               to
                    a
             */
            if(n->child[1]->type == &node_float &&
               float_is_zero(n->child[1]->value.f)) {
                node_t*new_node = n->child[0];
                node_destroy_self(n);
                return new_node;
            }
            break;
	case opcode_node_div:
            /* convert 
                    a / 1.0
               to
                    a
             */
            if(n->child[1]->type == &node_float &&
               float_is_one(n->child[1]->value.f)) {
                node_t*new_node = n->child[0];
                node_destroy_self(n);
                return new_node;
            }
            break;
	case opcode_node_add:
            /* convert 
                    a + 0.0
               to
                    a
             */
            num = 0;
            for(t=0;t<n->num_children;t++) {
                node_set_child(n, num, n->child[t]);
                if(n->child[t]->type != &node_float ||
                   !float_is_zero(n->child[t]->value.f)) {
                    num++;
                }
            }
            if(!num) {
                node_set_child(n, num++, node_new_with_args(&node_float, 0.0));
            }
            n->num_children = num;
            break;
    }
    for(t=0;t<n->num_children;t++) {
        node_set_child(n, t, node_optimize(n->child[t]));
    }
    return n;
}
