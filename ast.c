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

#define EVAL_HEADER_1(v) \
    constant_t v = n->child[0]->type->eval(n->child[0],env); \
    constant_t r;

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
flags:NODE_FLAG_HAS_CHILDREN,
eval:node_add_eval,
min_args:2,
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
flags:NODE_FLAG_HAS_CHILDREN,
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
flags:NODE_FLAG_HAS_CHILDREN,
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
flags:NODE_FLAG_HAS_CHILDREN,
eval: node_div_eval,
min_args:2,
max_args:2,
};

// -------------------------- exp x ----------------------------------

constant_t node_exp_eval(node_t*n, environment_t* env)
{
    EVAL_HEADER_1(v);
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
    EVAL_HEADER_1(condition);
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

// -------------------------- float(b) ----------------------------------

constant_t node_bool_to_float_eval(node_t*n, environment_t* env)
{
    EVAL_HEADER_1(b);
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
flags:NODE_FLAG_HAS_CHILDREN,
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
flags:NODE_FLAG_HAS_CHILDREN,
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
flags:NODE_FLAG_HAS_CHILDREN,
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
flags:NODE_FLAG_HAS_CHILDREN,
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
flags:NODE_FLAG_HAS_CHILDREN,
eval: node_equals_eval,
min_args:2,
max_args:2,
};

// -------------------------- max_arg ------------------------------------

constant_t node_max_arg_eval(node_t*n, environment_t* env)
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
nodetype_t node_max_arg =
{
name:"max_arg",
flags:NODE_FLAG_HAS_CHILDREN,
eval: node_max_arg_eval,
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

// -------------------------- array ------------------------------------

constant_t node_array_eval(node_t*n, environment_t* env)
{
    return n->value;
}
nodetype_t node_array =
{
name:"array",
flags:NODE_FLAG_HAS_VALUE,
eval: node_array_eval,
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
flags:NODE_FLAG_HAS_CHILDREN,
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

constant_t node_var_eval(node_t*n, environment_t* env)
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
nodetype_t node_var =
{
name:"var",
flags:NODE_FLAG_HAS_VALUE,
eval: node_var_eval,
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

// ---------------------- text s ------------- -------------------------

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
#   define NODE(op, name) name.opcode = op;
    LIST_NODES
    #undef NODE
    is_initialized = true;
}

uint8_t node_get_opcode(node_t*n)
{
    nodelist_init();
    return n->type->opcode;
}

// ======================== node handling ==============================

node_t* node_new(nodetype_t*t)
{
    node_t*n = (node_t*)malloc(sizeof(node_t));
    n->type = t;
    n->parent = 0;
    n->child = 0;
    n->num_children = 0;
    n->value = missing_constant();
    return n;
}

node_t* node_new_with_args(nodetype_t*t,...)
{
    node_t*n = node_new(t);
    va_list arglist;
    va_start(arglist, t);
    if(n->type == &node_var) {
	n->value = int_constant(va_arg(arglist,int));
    } else if(n->type == &node_category) {
	n->value = category_constant(va_arg(arglist,category_t));
    } else if(n->type == &node_float) {
	n->value = float_constant(va_arg(arglist,double));
    } else if(n->type == &node_array) {
        array_t*array = va_arg(arglist,array_t*);
	n->value = array_constant(array);
    } else if(n->type == &node_string) {
	n->value = string_constant(va_arg(arglist,char*));
    } else if(n->type == &node_constant) {
	n->value = va_arg(arglist,constant_t);
    } else if(n->type == &node_setlocal || n->type == &node_getlocal) {
	n->value = int_constant(va_arg(arglist,int));
    }
    va_end(arglist);
    return n;
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

    if(!n->num_children) {
	// first child
	n->child = malloc(1*sizeof(node_t*));
	n->child[0] = child;
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
	n->child = realloc(n->child, (highest_bit<<1)*sizeof(node_t*));
    }
    n->child[n->num_children++] = child;
}

void node_destroy(node_t*n)
{
    int t;
    if(n->type->flags&NODE_FLAG_HAS_VALUE) {
        constant_clear(&n->value);
    }
    if((n->type->flags&NODE_FLAG_HAS_CHILDREN) && n->child) {
	for(t=0;t<n->num_children;t++) {
	    node_destroy(n->child[t]);n->child[t] = 0;
	}
	free(n->child);
    }
    free(n);
}

constant_t node_eval(node_t*n,environment_t* e)
{
    return n->type->eval(n, e);
}

bool node_is_primitive(node_t*n)
{
    return(n->type == &node_category ||
           n->type == &node_array ||
           n->type == &node_float ||
           n->type == &node_string
           );
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

int node_highest_local(node_t*node)
{
    int max = 0;
    if(node->type == &node_setlocal ||
       node->type == &node_getlocal) {
	max = node->value.i;
    }
    int t;
    for(t=0;t<node->num_children;t++) {
	int l = node_highest_local(node->child[t]);
	if(l>max)
	    max = l;
    }
    return max;
}

void node_print(node_t*n)
{
    printf("------------ code -------------\n");
    node_print2(n,"","",stdout);
    printf("-------------------------------\n");
}

