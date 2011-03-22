/* codegen_ruby.c
   Code generator for Ruby

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

#include "codegen.h"

void ruby_write_node_block(node_t*n, state_t*s)
{
    int t;
    for(t=0;t<n->num_children;t++) {
        if(t)
            strf(s, "\n");
        write_node(s, n->child[t]);
    }
}
void ruby_write_node_if(node_t*n, state_t*s)
{
    if(!node_terminates(n) && node_has_consumer_parent(n)) {
	strf(s, "if ");
	write_node(s, n->child[0]);
	strf(s, " then ");
	indent(s);write_node(s, n->child[1]);dedent(s);
	if(!node_is_missing(n->child[2])) {
	    strf(s, " else ");
	    indent(s);write_node(s, n->child[2]);dedent(s);
	}
	strf(s, " end");
    } else {
	strf(s, "if ");
	write_node(s, n->child[0]);
	strf(s, " then\n");
	indent(s);write_node(s, n->child[1]);dedent(s);
	if(!node_is_missing(n->child[2])) {
	    strf(s, "\nelse\n");
	    indent(s);write_node(s, n->child[2]);dedent(s);
	}
	strf(s, "\nend");
    }
}
void ruby_write_node_add(node_t*n, state_t*s)
{
    int t;
    for(t=0;t<n->num_children;t++) {
        if(t && !node_has_minus_prefix(n->child[t])) strf(s, "+");
        write_node(s, n->child[t]);
    }
}
void ruby_write_node_sub(node_t*n, state_t*s)
{
    int t;
    for(t=0;t<n->num_children;t++) {
        if(t && !node_has_minus_prefix(n->child[t])) strf(s, "-");
        write_node(s, n->child[t]);
    }
}
void ruby_write_node_mul(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, "*");
    write_node(s, n->child[1]);
}
void ruby_write_node_div(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, "/");
    write_node(s, n->child[1]);
}
void ruby_write_node_lt(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, "<");
    write_node(s, n->child[1]);
}
void ruby_write_node_lte(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, "<=");
    write_node(s, n->child[1]);
}
void ruby_write_node_gt(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, ">");
    write_node(s, n->child[1]);
}
void ruby_write_node_gte(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, ">=");
    write_node(s, n->child[1]);
}
void ruby_write_node_in(node_t*n, state_t*s)
{
    strf(s, "!!");
    write_node(s, n->child[1]);
    strf(s, ".index(");
    write_node(s, n->child[0]);
    strf(s, ")");
}
void ruby_write_node_not(node_t*n, state_t*s)
{
    strf(s, "!");
    write_node(s, n->child[0]);
}
void ruby_write_node_neg(node_t*n, state_t*s)
{
    strf(s, "-");
    write_node(s, n->child[0]);
}
void ruby_write_node_exp(node_t*n, state_t*s)
{
    strf(s, "Math.exp(");
    write_node(s, n->child[0]);
    strf(s, ")");
}
void ruby_write_node_sqr(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, "**2");
}
void ruby_write_node_abs(node_t*n, state_t*s)
{
    strf(s, "(");
    write_node(s, n->child[0]);
    strf(s, ").abs");
}
void ruby_write_node_param(node_t*n, state_t*s)
{
    if(s->model->sig->column_names) {
        strf(s, "%s", s->model->sig->column_names[n->value.i]);
    } else {
        strf(s, "data[%d]", n->value.i);
    }
}
void ruby_write_node_nop(node_t*n, state_t*s)
{
}
void ruby_write_constant(constant_t*c, state_t*s)
{
    int t;
    switch(c->type) {
        case CONSTANT_FLOAT:
            strf(s, "%f", c->f);
            break;
        case CONSTANT_INT:
        case CONSTANT_CATEGORY:
            strf(s, "%d", c->i);
            break;
        case CONSTANT_BOOL:
            if(c->b)
                strf(s, "true");
            else
                strf(s, "false");
        case CONSTANT_STRING:
            strf(s, ":");
            strf(s, "%s", c->s);
            break;
        case CONSTANT_MISSING:
            strf(s, "nil");
            break;
        case CONSTANT_MIXED_ARRAY:
        case CONSTANT_INT_ARRAY:
        case CONSTANT_FLOAT_ARRAY:
        case CONSTANT_CATEGORY_ARRAY:
        case CONSTANT_STRING_ARRAY:
            strf(s, "[");
            for(t=0;t<c->a->size;t++) {
                if(t)
                    strf(s, ",");
                ruby_write_constant(&c->a->entries[t], s);
            }
            strf(s, "]");
            break;
    }
}
void ruby_write_node_constant(node_t*n, state_t*s)
{
    ruby_write_constant(&n->value, s);
}
void ruby_write_node_bool(node_t*n, state_t*s)
{
    ruby_write_constant(&n->value, s);
}
void ruby_write_node_category(node_t*n, state_t*s)
{
    ruby_write_constant(&n->value, s);
}
void ruby_write_node_mixed_array(node_t*n, state_t*s)
{
    ruby_write_constant(&n->value, s);
}
void ruby_write_node_string_array(node_t*n, state_t*s)
{
    ruby_write_constant(&n->value, s);
}
void ruby_write_node_int_array(node_t*n, state_t*s)
{
    ruby_write_constant(&n->value, s);
}
void ruby_write_node_float_array(node_t*n, state_t*s)
{
    ruby_write_constant(&n->value, s);
}
void ruby_write_node_category_array(node_t*n, state_t*s)
{
    ruby_write_constant(&n->value, s);
}
void ruby_write_node_zero_int_array(node_t*n, state_t*s)
{
    ruby_write_constant(&n->value, s);
}
void ruby_write_node_float(node_t*n, state_t*s)
{
    ruby_write_constant(&n->value, s);
}
void ruby_write_node_int(node_t*n, state_t*s)
{
    ruby_write_constant(&n->value, s);
}
void ruby_write_node_string(node_t*n, state_t*s)
{
    ruby_write_constant(&n->value, s);
}
void ruby_write_node_missing(node_t*n, state_t*s)
{
    ruby_write_constant(&n->value, s);
}
void ruby_write_node_getlocal(node_t*n, state_t*s)
{
    strf(s, "v%d", n->value.i);
}
void ruby_write_node_setlocal(node_t*n, state_t*s)
{
    strf(s, "v%d = ", n->value.i);
    write_node(s, n->child[0]);
}
void ruby_write_node_inclocal(node_t*n, state_t*s)
{
    strf(s, "v%d += 1", n->value.i);
}
void ruby_write_node_bool_to_float(node_t*n, state_t*s)
{
    strf(s, "(");
    write_node(s, n->child[0]);
    strf(s, " ? 1.0 : 0.0");
    strf(s, ")");
}
void ruby_write_node_equals(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, " == ");
    write_node(s, n->child[1]);
}
void ruby_write_node_arg_max(node_t*n, state_t*s)
{
    strf(s, "([");
    int t;
    for(t=0;t<n->num_children;t++) {
        if(t) strf(s, ",");
        write_node(s, n->child[t]);
    }
    strf(s, "].each.inject([]) {|i,n| [i,[n,i[1]+1]].max})[1]");
}
void ruby_write_node_arg_max_i(node_t*n, state_t*s)
{
    ruby_write_node_arg_max(n, s);
}
void ruby_write_node_array_at_pos(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, "[");
    write_node(s, n->child[1]);
    strf(s, "]");
}
void ruby_write_node_array_at_pos_inc(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, "[");
    write_node(s, n->child[1]);
    strf(s, "]+=1");
}
void ruby_write_node_array_arg_max_i(node_t*n, state_t*s)
{
    strf(s, "(");
    write_node(s, n->child[0]);
    strf(s, ".each.inject([]) {|i,n| [i,[n,i[1]+1]].max})[1]");
}
void ruby_write_node_return(node_t*n, state_t*s)
{
    strf(s, "return ");
    write_node(s, n->child[0]);
}
void ruby_write_node_brackets(node_t*n, state_t*s)
{
    strf(s, "(");
    write_node(s, n->child[0]);
    strf(s, ")");
}
void ruby_write_header(model_t*model, state_t*s)
{
    strf(s, "def predict(");
    if(s->model->sig) {
        int t;
        node_t*root = (node_t*)model->code;
        for(t=0;t<model->sig->num_inputs;t++) {
            if(t) strf(s, ", ");
            strf(s, "%s", s->model->sig->column_names[t]);
        }
    } else {
        strf(s, "data");
    }
    strf(s, ")\n");
    indent(s);
}
void ruby_write_footer(model_t*model, state_t*s)
{
    dedent(s);
    strf(s, "\nend\n");
}


codegen_t codegen_ruby = {
#define NODE(opcode, name) write_##name: ruby_write_##name,
LIST_NODES
#undef NODE
    write_header: ruby_write_header,
    write_footer: ruby_write_footer,
};

