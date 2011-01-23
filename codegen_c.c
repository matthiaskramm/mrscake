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

char* c_type_name(constant_type_t c)
{
    switch(c) {
        case CONSTANT_FLOAT:
            return "float";
        case CONSTANT_INT:
            return "int";
        case CONSTANT_CATEGORY:
            return "int";
        case CONSTANT_BOOL:
            return "bool";
        case CONSTANT_STRING:
            return "char*";
        case CONSTANT_INT_ARRAY:
            return "int*";
        case CONSTANT_FLOAT_ARRAY:
            return "float*";
        case CONSTANT_CATEGORY_ARRAY:
            return "int*";
        case CONSTANT_MIXED_ARRAY:
            return "void**";
        case CONSTANT_STRING_ARRAY:
            return "char**";
        case CONSTANT_MISSING:
            return "void";
        default:
            return "void*";
    }
}

void c_write_node_block(node_t*n, state_t*s)
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
void c_write_node_if(node_t*n, state_t*s)
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
void c_write_node_add(node_t*n, state_t*s)
{
    int t;
    for(t=0;t<n->num_children;t++) {
        if(t && !node_has_minus_prefix(n->child[t])) strf(s, "+");
        write_node(s, n->child[t]);
    }
}
void c_write_node_sub(node_t*n, state_t*s)
{
    int t;
    for(t=0;t<n->num_children;t++) {
        if(t && !node_has_minus_prefix(n->child[t])) strf(s, "-");
        write_node(s, n->child[t]);
    }
}
void c_write_node_mul(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, "*");
    write_node(s, n->child[1]);
}
void c_write_node_div(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, "/");
    write_node(s, n->child[1]);
}
void c_write_node_lt(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, "<");
    write_node(s, n->child[1]);
}
void c_write_node_lte(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, "<=");
    write_node(s, n->child[1]);
}
void c_write_node_gt(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, ">");
    write_node(s, n->child[1]);
}
void c_write_node_gte(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, ">=");
    write_node(s, n->child[1]);
}
void c_write_node_in(node_t*n, state_t*s)
{
    if(node_is_array(n->child[1])) {
        array_t*a = n->child[1]->value.a;
        if(!a->size) {
            strf(s, "false");
            return;
        } else {
            constant_type_t etype = constant_array_subtype(&n->child[1]->value);
            switch(etype) {
                case CONSTANT_STRING:
                    strf(s, "!!strstr(\"");
                    int t;
                    for(t=0;t<a->size;t++) {
                        assert(a->entries[t].type == CONSTANT_STRING);
                        if(t) 
                            strf(s, "\\x7f");
                        write_escaped_string(s, a->entries[t].s);
                    }
                    strf(s, "\",");
                    write_node(s, n->child[0]);
                    strf(s, ")");
                    return;
            }
        }
    }
    strf(s, "find_in(");
    write_node(s, n->child[0]);
    strf(s, ",");
    write_node(s, n->child[1]);
    strf(s, ")");
}
void c_write_node_not(node_t*n, state_t*s)
{
    strf(s, "!");
    write_node(s, n->child[0]);
}
void c_write_node_neg(node_t*n, state_t*s)
{
    strf(s, "-");
    write_node(s, n->child[0]);
}
void c_write_node_exp(node_t*n, state_t*s)
{
    strf(s, "exp(");
    write_node(s, n->child[0]);
    strf(s, ")");
}
void c_write_node_sqr(node_t*n, state_t*s)
{
    strf(s, "sqr");
    if(n->child[0]->type!=&node_brackets) strf(s, "(");
    write_node(s, n->child[0]);
    if(n->child[0]->type!=&node_brackets) strf(s, ")");
}
void c_write_node_abs(node_t*n, state_t*s)
{
    strf(s, "fabs(");
    write_node(s, n->child[0]);
    strf(s, ")");
}
void c_write_node_param(node_t*n, state_t*s)
{
    if(s->model->column_names) {
        strf(s, "%s", s->model->column_names[n->value.i]);
    } else {
        strf(s, "p%d", n->value.i);
    }
}
void c_write_node_nop(node_t*n, state_t*s)
{
    strf(s, "(void)");
}
void c_write_constant(constant_t*c, state_t*s)
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
                strf(s, "True");
            else
                strf(s, "False");
        case CONSTANT_STRING:
            strf(s, "\"");
            strf(s, "%s", c->s);
            strf(s, "\"");
            break;
        case CONSTANT_MISSING:
            strf(s, "NULL");
            break;
        case CONSTANT_INT_ARRAY:
        case CONSTANT_MIXED_ARRAY:
        case CONSTANT_FLOAT_ARRAY:
        case CONSTANT_CATEGORY_ARRAY:
        case CONSTANT_STRING_ARRAY:
            strf(s, "{");
            for(t=0;t<c->a->size;t++) {
                if(t)
                    strf(s, ",");
                c_write_constant(&c->a->entries[t], s);
            }
            strf(s, "}");
            break;
    }
}
void c_write_node_constant(node_t*n, state_t*s)
{
    c_write_constant(&n->value, s);
}
void c_write_node_bool(node_t*n, state_t*s)
{
    c_write_constant(&n->value, s);
}
void c_write_node_category(node_t*n, state_t*s)
{
    c_write_constant(&n->value, s);
}
void c_write_node_string_array(node_t*n, state_t*s)
{
    strf(s, "a%x", (long)n->value.a);
}
void c_write_node_int_array(node_t*n, state_t*s)
{
    strf(s, "a%x", (long)n->value.a);
}
void c_write_node_float_array(node_t*n, state_t*s)
{
    strf(s, "a%x", (long)n->value.a);
}
void c_write_node_mixed_array(node_t*n, state_t*s)
{
    strf(s, "a%x", (long)n->value.a);
}
void c_write_node_category_array(node_t*n, state_t*s)
{
    strf(s, "a%x", (long)n->value.a);
}
void c_write_node_zero_int_array(node_t*n, state_t*s)
{
    strf(s, "a%x", (long)n->value.a);
}
void c_write_node_float(node_t*n, state_t*s)
{
    c_write_constant(&n->value, s);
}
void c_write_node_int(node_t*n, state_t*s)
{
    c_write_constant(&n->value, s);
}
void c_write_node_string(node_t*n, state_t*s)
{
    c_write_constant(&n->value, s);
}
void c_write_node_missing(node_t*n, state_t*s)
{
    c_write_constant(&n->value, s);
}
void c_write_node_getlocal(node_t*n, state_t*s)
{
    strf(s, "v%d", n->value.i);
}
void c_write_node_setlocal(node_t*n, state_t*s)
{
    strf(s, "v%d = ", n->value.i);
    write_node(s, n->child[0]);
}
void c_write_node_inclocal(node_t*n, state_t*s)
{
    strf(s, "v%d++", n->value.i);
}
void c_write_node_bool_to_float(node_t*n, state_t*s)
{
    strf(s, "(float)(");
    write_node(s, n->child[0]);
    strf(s, ")");
}
void c_write_node_equals(node_t*n, state_t*s)
{
    constant_type_t type = node_type(n->child[0], s->model);
    if(type==CONSTANT_STRING) {
        strf(s, "!strcmp(");
        write_node(s, n->child[0]);
        strf(s, ",");
        write_node(s, n->child[1]);
        strf(s, ")");
    } else {
        write_node(s, n->child[0]);
        strf(s, " == ");
        write_node(s, n->child[1]);
    }
}
void c_write_node_arg_max(node_t*n, state_t*s)
{
    strf(s, "arg_max(%d, ", n->num_children);
    int t;
    for(t=0;t<n->num_children;t++) {
        if(t)
            strf(s, ", ");
        strf(s, "(double)");
        write_node(s, n->child[t]);
    }
    strf(s, ")");
}
void c_write_node_arg_max_i(node_t*n, state_t*s)
{
    strf(s, "arg_max_i(%d, ", n->num_children);
    int t;
    for(t=0;t<n->num_children;t++) {
        if(t)
            strf(s, ", ");
        strf(s, "(int)");
        write_node(s, n->child[t]);
    }
    strf(s, ")");
}
void c_write_node_array_arg_max_i(node_t*n, state_t*s)
{
    strf(s, "array_arg_max_i(");
    write_node(s, n->child[0]);
    strf(s, ", %d)", node_array_size(n->child[0]));
}
void c_write_node_array_at_pos(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, "[");
    write_node(s, n->child[1]);
    strf(s, "]");
}
void c_write_node_array_at_pos_inc(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, "[");
    write_node(s, n->child[1]);
    strf(s, "]++");
}
void c_write_node_return(node_t*n, state_t*s)
{
    strf(s, "return ");
    write_node(s, n->child[0]);
}
void c_write_node_brackets(node_t*n, state_t*s)
{
    strf(s, "(");
    write_node(s, n->child[0]);
    strf(s, ")");
}
static void c_write_function_arg_max(state_t*s, char*suffix, char*type)
{
    strf(s,
"int arg_max%s(int count, ...)\n"
"{\n"
"    va_list arglist;\n"
"    va_start(arglist, count);\n"
"    int i;\n"
"    double max = va_arg(arglist,%s)\n"
"    int best = 0;\n"
"    for(i=1;i<count;i++) {\n"
"        double a = va_arg(arglist,%s)\n"
"        if(a>best) {\n"
"            best = i;\n"
"            max = a;\n"
"        }\n"
"    }\n"
"    va_end(arglist);\n"
"    return best;\n"
"}\n", suffix, type, type
    );
}
static void c_write_function_array_arg_max_i(state_t*s)
{
    strf(s, "%s",
"int array_arg_max_i(int*array, int count)\n"
"{\n"
"    int max = array[0];\n"
"    int best = 0;\n"
"    int i;\n"
"    for(i=1;i<count;i++) {\n"
"        if(array[i]>max) {\n"
"            max = array[i];\n"
"            best = i;\n"
"        }\n"
"    }\n"
"    return best;\n"
"}\n"
    );
}
static void c_write_function_sqr(state_t*s)
{
    strf(s, "%s",
"static inline double sqr(const double v)\n"
"{\n"
"    return v*v\n"
"}\n"
    );
}
void c_enumerate_arrays(node_t*node, state_t*s)
{
    if(node_is_array(node)) {
        strf(s, "%s a%x[%d] = ", 
                c_type_name(constant_array_subtype(&node->value)),
                (long)(node->value.a),
                node->value.a->size
                );
        c_write_constant(&node->value, s);
        strf(s, ";\n");
    } else {
        int t;
        for(t=0;t<node->num_children;t++) {
            c_enumerate_arrays(node->child[t], s);
        }
    }
}
void c_write_header(model_t*model, state_t*s)
{
    node_t*root = (node_t*)model->code;
    constant_type_t type = node_type(root, model);

    if(node_has_child(root, &node_arg_max)) {
        c_write_function_arg_max(s, "", "double");
    }
    if(node_has_child(root, &node_arg_max_i)) {
        c_write_function_arg_max(s, "_i", "int");
    }
    if(node_has_child(root, &node_array_arg_max_i)) {
        c_write_function_array_arg_max_i(s);
    }
    if(node_has_child(root, &node_sqr)) {
        c_write_function_sqr(s);
    }

    strf(s, "%s predict(", c_type_name(type));
    int t;
    for(t=0;t<model->num_inputs;t++) {
        if(t) strf(s, ", ");
        if(s->model->column_names) {
            strf(s, "%s %s", c_type_name(model_param_type(s->model,t)), s->model->column_names[t]);
        } else {
            strf(s, "%s p%d", c_type_name(model_param_type(s->model,t)), t);
        }
    }
    strf(s, ")\n");
    strf(s, "{\n");
    indent(s);
    int num_locals = 0;
    constant_type_t*types = node_local_types(root, s->model, &num_locals);
    for(t=0;t<num_locals;t++) {
        if(types[t] != CONSTANT_MISSING) {
            strf(s, "%s v%d;\n", c_type_name(types[t]), t);
        }
    }
    c_enumerate_arrays(root, s);

}
void c_write_footer(model_t*model, state_t*s)
{
    node_t*root = model->code;
    dedent(s);
    strf(s, "\n}\n");
}


codegen_t codegen_c = {
#define NODE(opcode, name) write_##name: c_write_##name,
LIST_NODES
#undef NODE
    write_header: c_write_header,
    write_footer: c_write_footer,
};

