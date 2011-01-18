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
bool node_terminates(node_t*n)
{
    switch(node_get_opcode(n)) {
	case node_block_opcode:
	    return node_terminates(n->child[n->num_children-1]);
	case node_return_opcode:
	    return true;
	case node_if_opcode:
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
	case node_int_opcode:
	case node_float_opcode:
	case node_constant_opcode:
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
node_t* node_cascade_returns(node_t*n) 
{
    switch(node_get_opcode(n)) {
	case node_block_opcode:
	    node_set_child(n, n->num_children-1, node_cascade_returns(n->child[n->num_children-1]));
	    return n;
	case node_if_opcode:
	    node_set_child(n, 1, node_cascade_returns(n->child[1]));
	    node_set_child(n, 2, node_cascade_returns(n->child[2]));
	    return n;
	case node_nop_opcode:
	case node_setlocal_opcode:
	case node_inclocal_opcode:
	    return n;
	default: {
	    node_t*p = node_new(&node_return, n->parent);
	    node_append_child(p, n);
	    return p;
	}
    }
}
bool lower_precedence(nodetype_t*type1, nodetype_t*type2)
{
    /* FIXME */
    if((type1 == &node_add || type1 == &node_sub) && 
       (type2 == &node_mul || type2 == &node_div)) {
	return true;
    }
    if((type1 == &node_add || type1 == &node_sub || type2 == &node_mul || type2 == &node_div) &&
       (type2 == &node_sqr)) {
	return true;
    }
    return false;
}
node_t* node_insert_brackets(node_t*n) 
{
    int t;
    for(t=0;t<n->num_children;t++) {
	node_set_child(n, t, node_insert_brackets(n->child[t]));
    }
    if(n->parent) {
	if(lower_precedence(n->type, n->parent->type)) {
	    node_t*p = node_new(&node_brackets, n->parent);
	    node_append_child(p, n);
	    return p;
	}
    }
    return n;
}
node_t* node_prepare_for_code_generation(node_t*n)
{
    n = node_cascade_returns(n);
    n = node_insert_brackets(n);
    return n;
}
