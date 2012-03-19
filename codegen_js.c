/* codegen_c.c
   Code generator for C

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

#include <assert.h>
#include "codegen.h"
#include "ast_transforms.h"

void js_write_node_block(node_t*n, state_t*s)
{
    int t;
    if(n->parent) {
        strf(s, "{\n");
        indent(s);
    }
    for(t=0;t<n->num_children;t++) {
        if(t)
            strf(s, "\n");
        write_node(s, n->child[t]);
        strf(s, ";");
    }
    if(n->parent) {
        dedent(s);
        strf(s, "\n}\n");
    }
}
void js_write_node_if(node_t*n, state_t*s)
{
    if(!node_terminates(n) && node_has_consumer_parent(n)) {
        strf(s, "(");
        write_node(s, n->child[0]);
        strf(s, ")?(");
        write_node(s, n->child[1]);
        strf(s, "):(");
        write_node(s, n->child[2]);
        strf(s, ")");
    } else {
        strf(s, "if(");
        write_node(s, n->child[0]);
        strf(s, ")\n");
        indent(s);write_node(s, n->child[1]);
        if(!node_is_missing(n->child[2])) {
            strf(s, ";");dedent(s);
            strf(s, "\nelse\n");
            indent(s);write_node(s, n->child[2]);
        }
        dedent(s);
    }
}
void js_write_node_add(node_t*n, state_t*s)
{
    int t;
    for(t=0;t<n->num_children;t++) {
        if(t && !node_has_minus_prefix(n->child[t])) strf(s, "+");
        write_node(s, n->child[t]);
    }
}
void js_write_node_sub(node_t*n, state_t*s)
{
    int t;
    for(t=0;t<n->num_children;t++) {
        if(t && !node_has_minus_prefix(n->child[t])) strf(s, "-");
        write_node(s, n->child[t]);
    }
}
void js_write_node_mul(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, "*");
    write_node(s, n->child[1]);
}
void js_write_node_div(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, "/");
    write_node(s, n->child[1]);
}
void js_write_node_lt(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, "<");
    write_node(s, n->child[1]);
}
void js_write_node_lte(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, "<=");
    write_node(s, n->child[1]);
}
void js_write_node_gt(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, ">");
    write_node(s, n->child[1]);
}
void js_write_node_gte(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, ">=");
    write_node(s, n->child[1]);
}
void js_write_node_in(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, ".index(");
    write_node(s, n->child[1]);
    strf(s, ")>=0");
}
void js_write_node_not(node_t*n, state_t*s)
{
    strf(s, "!");
    write_node(s, n->child[0]);
}
void js_write_node_neg(node_t*n, state_t*s)
{
    strf(s, "-");
    write_node(s, n->child[0]);
}
void js_write_node_exp(node_t*n, state_t*s)
{
    strf(s, "Math.exp(");
    write_node(s, n->child[0]);
    strf(s, ")");
}
void js_write_node_sqr(node_t*n, state_t*s)
{
    strf(s, "Math.pow(");
    write_node(s, n->child[0]);
    strf(s, ",2)");
}
void js_write_node_abs(node_t*n, state_t*s)
{
    strf(s, "Math.abs(");
    write_node(s, n->child[0]);
    strf(s, ")");
}
void js_write_node_param(node_t*n, state_t*s)
{
    if(s->model->sig->has_column_names) {
        strf(s, "%s", s->model->sig->column_names[n->value.i]);
    } else {
        strf(s, "p%d", n->value.i);
    }
}
void js_write_node_nop(node_t*n, state_t*s)
{
    strf(s, "");
}
void js_write_constant(constant_t*c, state_t*s)
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
            strf(s, "\"");
            write_escaped_string(s, c->s);
            strf(s, "\"");
            break;
        case CONSTANT_MISSING:
            strf(s, "null");
            break;
        case CONSTANT_INT_ARRAY:
        case CONSTANT_MIXED_ARRAY:
        case CONSTANT_FLOAT_ARRAY:
        case CONSTANT_CATEGORY_ARRAY:
        case CONSTANT_STRING_ARRAY:
            strf(s, "[");
            for(t=0;t<c->a->size;t++) {
                if(t)
                    strf(s, ",");
                js_write_constant(&c->a->entries[t], s);
            }
            strf(s, "]");
            break;
    }
}
void js_write_node_constant(node_t*n, state_t*s)
{
    js_write_constant(&n->value, s);
}
void js_write_node_bool(node_t*n, state_t*s)
{
    js_write_constant(&n->value, s);
}
void js_write_node_category(node_t*n, state_t*s)
{
    js_write_constant(&n->value, s);
}
void js_write_node_mixed_array(node_t*n, state_t*s)
{
    js_write_constant(&n->value, s);
}
void js_write_node_string_array(node_t*n, state_t*s)
{
    js_write_constant(&n->value, s);
}
void js_write_node_int_array(node_t*n, state_t*s)
{
    js_write_constant(&n->value, s);
}
void js_write_node_float_array(node_t*n, state_t*s)
{
    js_write_constant(&n->value, s);
}
void js_write_node_category_array(node_t*n, state_t*s)
{
    js_write_constant(&n->value, s);
}
void js_write_node_zero_int_array(node_t*n, state_t*s)
{
    js_write_constant(&n->value, s);
}
void js_write_node_float(node_t*n, state_t*s)
{
    js_write_constant(&n->value, s);
}
void js_write_node_int(node_t*n, state_t*s)
{
    js_write_constant(&n->value, s);
}
void js_write_node_string(node_t*n, state_t*s)
{
    js_write_constant(&n->value, s);
}
void js_write_node_missing(node_t*n, state_t*s)
{
    js_write_constant(&n->value, s);
}
void js_write_node_getlocal(node_t*n, state_t*s)
{
    strf(s, "v%d", n->value.i);
}
void js_write_node_setlocal(node_t*n, state_t*s)
{
    strf(s, "v%d = ", n->value.i);
    write_node(s, n->child[0]);
}
void js_write_node_inclocal(node_t*n, state_t*s)
{
    strf(s, "v%d++", n->value.i);
}
void js_write_node_bool_to_float(node_t*n, state_t*s)
{
    strf(s, "(");
    write_node(s, n->child[0]);
    strf(s, "*1.0)");
}
void js_write_node_equals(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, " == ");
    write_node(s, n->child[1]);
}
void js_write_node_arg_max(node_t*n, state_t*s)
{
    strf(s, "arg_max([");
    int t;
    for(t=0;t<n->num_children;t++) {
        if(t)
            strf(s, ", ");
        write_node(s, n->child[t]);
    }
    strf(s, "])");
}
void js_write_node_arg_max_i(node_t*n, state_t*s)
{
    js_write_node_arg_max(n,s);
}
void js_write_node_array_arg_max_i(node_t*n, state_t*s)
{
    strf(s, "arg_max(");
    write_node(s, n->child[0]);
    strf(s, ")");
}
void js_write_node_array_at_pos(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, "[");
    write_node(s, n->child[1]);
    strf(s, "]");
}
void js_write_node_array_at_pos_inc(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, "[");
    write_node(s, n->child[1]);
    strf(s, "]++");
}
void js_write_node_return(node_t*n, state_t*s)
{
    strf(s, "return ");
    write_node(s, n->child[0]);
}
void js_write_node_brackets(node_t*n, state_t*s)
{
    strf(s, "(");
    write_node(s, n->child[0]);
    strf(s, ")");
}
static void js_write_function_arg_max(state_t*s, char*suffix, char*type)
{
    strf(s,
"function arg_max(array)\n"
"{\n"
"    //TODO\n"
"}\n");
}
void js_write_header(model_t*model, state_t*s)
{
    node_t*root = (node_t*)model->code;
    constant_type_t type = node_type(root, model);

    if(node_has_child(root, &node_arg_max) ||
       node_has_child(root, &node_arg_max_i) ||
       node_has_child(root, &node_array_arg_max_i)) {
        js_write_function_arg_max(s, "", "double");
    }

    strf(s, "function predict(");
    int t;
    for(t=0;t<model->sig->num_inputs;t++) {
        if(t) strf(s, ", ");
        strf(s, "%s", s->model->sig->column_names[t]);
    }
    strf(s, ")\n");
    strf(s, "{\n");
    indent(s);

}
void js_write_footer(model_t*model, state_t*s)
{
    node_t*root = model->code;
    dedent(s);
    strf(s, "\n}\n");
}


codegen_t codegen_js = {
#define NODE(opcode, name) write_##name: js_write_##name,
LIST_NODES
#undef NODE
    write_header: js_write_header,
    write_footer: js_write_footer,
};

