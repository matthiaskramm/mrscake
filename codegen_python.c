#include "codegen.h"

void python_write_node_block(node_t*n, state_t*s)
{
    int t;
    for(t=0;t<n->num_children;t++) {
        write_node(s, n->child[t]);
        strf(s, "\n");
    }
}
void python_write_node_if(node_t*n, state_t*s)
{
    if(node_has_consumer_parent(n)) {
        strf(s, "(");
        write_node(s, n->child[1]);
        strf(s, " if ");
        write_node(s, n->child[0]);
        strf(s, " else ");
        write_node(s, n->child[2]);
        strf(s, ")");
    } else {
        strf(s, "if ");
        write_node(s, n->child[0]);
        strf(s, ":\n");
        indent(s);write_node(s, n->child[1]);dedent(s);
        if(!missing(n->child[2])) {
            strf(s, "\nelse:\n");
            indent(s);write_node(s, n->child[2]);dedent(s);
        }
        strf(s, "\n");
    }
}
void python_write_node_add(node_t*n, state_t*s)
{
    int t;
    for(t=0;t<n->num_children;t++) {
        if(t) strf(s, "+");
        write_node(s, n->child[t]);
    }
}
void python_write_node_sub(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, "-");
    write_node(s, n->child[1]);
}
void python_write_node_mul(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, "*");
    write_node(s, n->child[1]);
}
void python_write_node_div(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, "/");
    write_node(s, n->child[1]);
}
void python_write_node_lt(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, "<");
    write_node(s, n->child[1]);
}
void python_write_node_lte(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, "<=");
    write_node(s, n->child[1]);
}
void python_write_node_gt(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, ">");
    write_node(s, n->child[1]);
}
void python_write_node_gte(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, ">=");
    write_node(s, n->child[1]);
}
void python_write_node_in(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, " in ");
    write_node(s, n->child[1]);
}
void python_write_node_not(node_t*n, state_t*s)
{
    strf(s, "not ");
    write_node(s, n->child[0]);
}
void python_write_node_neg(node_t*n, state_t*s)
{
    strf(s, "-");
    write_node(s, n->child[0]);
}
void python_write_node_exp(node_t*n, state_t*s)
{
    strf(s, "math.exp(");
    write_node(s, n->child[0]);
    strf(s, ")");
}
void python_write_node_sqr(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, " ** 2");
}
void python_write_node_abs(node_t*n, state_t*s)
{
    strf(s, "abs(");
    write_node(s, n->child[0]);
    strf(s, ")");
}
void python_write_node_var(node_t*n, state_t*s)
{
    strf(s, "param%d", n->value.i);
}
void python_write_node_nop(node_t*n, state_t*s)
{
}
void python_write_constant(constant_t*c, state_t*s)
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
            strf(s, "None");
            break;
        case CONSTANT_ARRAY:
            strf(s, "[");
            for(t=0;t<c->a->size;t++) {
                if(t)
                    strf(s, ",");
                python_write_constant(&c->a->entries[t], s);
            }
            strf(s, "]");
            break;
    }
}
void python_write_node_constant(node_t*n, state_t*s)
{
    python_write_constant(&n->value, s);
}
void python_write_node_bool(node_t*n, state_t*s)
{
    python_write_constant(&n->value, s);
}
void python_write_node_category(node_t*n, state_t*s)
{
    python_write_constant(&n->value, s);
}
void python_write_node_array(node_t*n, state_t*s)
{
    python_write_constant(&n->value, s);
}
void python_write_node_float(node_t*n, state_t*s)
{
    python_write_constant(&n->value, s);
}
void python_write_node_int(node_t*n, state_t*s)
{
    python_write_constant(&n->value, s);
}
void python_write_node_string(node_t*n, state_t*s)
{
    python_write_constant(&n->value, s);
}
void python_write_node_missing(node_t*n, state_t*s)
{
    python_write_constant(&n->value, s);
}
void python_write_node_getlocal(node_t*n, state_t*s)
{
    strf(s, "v%d", n->value.i);
}
void python_write_node_setlocal(node_t*n, state_t*s)
{
    strf(s, "v%d = ", n->value.i);
    write_node(s, n->child[0]);
}
void python_write_node_inclocal(node_t*n, state_t*s)
{
    strf(s, "v%d += 1", n->value.i);
}
void python_write_node_bool_to_float(node_t*n, state_t*s)
{
    strf(s, "float(");
    write_node(s, n->child[0]);
    strf(s, ")");
}
void python_write_node_equals(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, " == ");
    write_node(s, n->child[1]);
}
void python_write_node_arg_max(node_t*n, state_t*s)
{
    strf(s, "max([(v,nr) for nr,v in enumerate([");
    int t;
    for(t=0;t<n->num_children;t++) {
        if(t) strf(s, ",");
        write_node(s, n->child[t]);
    }
    strf(s, "])])[1]");
}
void python_write_node_arg_max_i(node_t*n, state_t*s)
{
    python_write_node_arg_max(n, s);
}
void python_write_node_array_at_pos(node_t*n, state_t*s)
{
    write_node(s, n->child[0]);
    strf(s, "[");
    write_node(s, n->child[1]);
    strf(s, "]");
}

codegen_t codegen_python = {
#define NODE(opcode, name) write_##name: python_write_##name,
LIST_NODES
#undef NODE
};
