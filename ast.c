/* ast.c

   AST representation of prediction programs.

   Copyright (c) 2010 Matthias Kramm <kramm@quiss.org>

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
#include "ast.h"

#define EVAL_HEADER_1 \
    value_t left = n->child[0]->type->eval(n->child[0],locals); \
    value_t r;

#define EVAL_HEADER_2_LR(v1,v2) \
    value_t v1 = n->child[0]->type->eval(n->child[0],locals);\
    value_t v2 = n->child[1]->type->eval(n->child[1],locals);\
    value_t r;

#define EVAL_HEADER_2(v1,v2) EVAL_HEADER_2_LR(v1,v2)

#define EVAL_CHILD(i) ((n)->child[(i)]->type->eval((n)->child[(i)],locals))

#define FLOAT(v) (check_type((v),TYPE_FLOAT),(v).f)
#define INT(v) (check_type((v),TYPE_INT),(v).i)
#define CATEGORY(v) (check_type((v),TYPE_CATEGORY),(v).c)
#define BOOL(v) (check_type((v),TYPE_BOOL),(v).b)

static inline int check_type(value_t v, uint8_t type)
{
    assert(v.type == type);
    return 0;
}

static inline value_t bool_value(bool b)
{
    value_t v;
    v.type = TYPE_BOOL;
    v.b = b;
    return v;
}
static inline value_t float_value(float f)
{
    value_t v;
    v.type = TYPE_FLOAT;
    v.f = f;
    return v;
}
static inline value_t missing_value()
{
    value_t v;
    v.type = TYPE_MISSING;
    return v;
}
static inline value_t category_value(category_t c)
{
    value_t v;
    v.type = TYPE_CATEGORY;
    v.c = c;
    return v;
}
static inline value_t int_value(int i)
{
    value_t v;
    v.type = TYPE_INT;
    v.i = i;
    return v;
}

// -------------------------- root node -------------------------------

value_t node_root_eval(node_t*n, environment_t* locals)
{
    return EVAL_CHILD(0);
}
nodetype_t node_root =
{
name:"root",
flags:NODE_FLAG_HAS_CHILDREN,
eval:node_root_eval,
min_args:2,
max_args:2,
};

// -------------------------- x + y -----------------------------------

value_t node_add_eval(node_t*n, environment_t* locals)
{
    EVAL_HEADER_2(left,right);
    return float_value(FLOAT(left) + FLOAT(right));
}
nodetype_t node_add =
{
name:"add",
flags:NODE_FLAG_HAS_CHILDREN,
eval:node_add_eval,
min_args:2,
max_args:2,
};

// -------------------------- x - y -----------------------------------

value_t node_minus_eval(node_t*n, environment_t* locals)
{
    EVAL_HEADER_2(left,right);
    return float_value(FLOAT(left) - FLOAT(right));
}
nodetype_t node_minus =
{
name:"minus",
flags:NODE_FLAG_HAS_CHILDREN,
eval: node_minus_eval,
min_args:2,
max_args:2,
};

// -------------------------- x < y -----------------------------------

value_t node_lt_eval(node_t*n, environment_t* locals)
{
    EVAL_HEADER_2(left,right);
    return bool_value(FLOAT(left) < FLOAT(right));
}
nodetype_t node_lt =
{
name:"lt",
flags:NODE_FLAG_HAS_CHILDREN,
eval: node_lt_eval,
min_args:2,
max_args:2,
};

// -------------------------- x > y -----------------------------------

value_t node_gt_eval(node_t*n, environment_t* locals)
{
    EVAL_HEADER_2(left,right);
    return bool_value(FLOAT(left) > FLOAT(right));
}
nodetype_t node_gt =
{
name:"gt",
flags:NODE_FLAG_HAS_CHILDREN,
eval: node_gt_eval,
min_args:2,
max_args:2,
};

// -------------------- if(x) then y else z ----------------------------

value_t node_if_eval(node_t*n, environment_t* locals)
{
    bool condition = BOOL(EVAL_CHILD(0));
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

value_t node_var_eval(node_t*n, environment_t* locals)
{
    assert(n->value.i >= 0 && n->value.i < locals->row->num_inputs);
    variable_t v = locals->row->inputs[n->value.i];
    if(v.type == CATEGORICAL) {
	return category_value(v.category);
    } else if(v.type == CONTINUOUS) {
	return float_value(v.value);
    } else if(v.type == MISSING) {
	return missing_value(v.value);
    } else {
	assert(!"bad type for input value");
    }
}
nodetype_t node_var =
{
name:"var",
flags:0,
eval: node_var_eval,
min_args:0,
max_args:0,
};

// ---------------------- category i (return i) -------------------------

value_t node_category_eval(node_t*n, environment_t* locals)
{
    return category_value(n->value.c);
}
nodetype_t node_category =
{
name:"category",
flags:0,
eval: node_category_eval,
min_args:0,
max_args:0,
};

// ======================== node handling ==============================

node_t* node_new(nodetype_t*t,...)
{
    node_t*n = (node_t*)malloc(sizeof(node_t));
    n->type = t;
    n->parent = 0;
    assert(t->max_args < INT_MAX);
    n->child = 0;
    if(t->max_args) {
	n->child = (node_t**)malloc(sizeof(node_t*)*t->max_args);
    }
    n->num_children = 0;
	    
    va_list arglist;
    va_start(arglist, t);
    if(n->type == &node_var) {
	n->value = int_value(va_arg(arglist,int));
    } else if(n->type == &node_category) {
	n->value = category_value(va_arg(arglist,category_t));
    }
    va_end(arglist);
    return n;
}

node_t* node_extend(node_t*n, node_t*add)
{
    n->child = realloc(n->child, (n->num_children+1)*sizeof(node_t*));
    n->child[n->num_children] = add;
    n->num_children++;
    return n;
}

node_t* node_new1(nodetype_t*t, node_t*node)
{
    node_t*n = (node_t*)malloc(sizeof(node_t)+sizeof(node_t*)*2);
    node_t**x = (node_t**)&n[1];
    n->type = t;
    n->parent = 0;
    n->child = x;
    n->num_children = 1;
    x[0] = node;
    x[1] = 0;
    return n;
};

node_t* node_new2(nodetype_t*t, node_t*left, node_t*right)
{
    node_t*n = (node_t*)malloc(sizeof(node_t)+sizeof(node_t*)*3);
    node_t**x = (node_t**)&n[1];
    n->type = t;
    n->parent = 0;
    n->child = x;
    n->num_children = 2;
    x[0] = left;
    x[1] = right;
    x[2] = 0;
    return n;
}
node_t* node_new3(nodetype_t*t, node_t*one, node_t*two, node_t*three)
{
    node_t*n = (node_t*)malloc(sizeof(node_t)+sizeof(node_t*)*4);
    node_t**x = (node_t**)&n[1];
    n->type = t;
    n->parent = 0;
    n->child = x;
    n->num_children = 3;
    x[0] = one;
    x[1] = two;
    x[2] = three;
    x[3] = 0;
    return n;
}

void node_free(node_t*n)
{
    int t;
    if((n->type->flags&NODE_FLAG_HAS_CHILDREN) && n->child) {
	for(t=0;t<n->num_children;t++) {
	    node_free(n->child[t]);n->child[t] = 0;
	}
	free(n->child);
    }
    free(n);
}

value_t node_eval(node_t*n,environment_t* e)
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
    if(n->type->flags&NODE_FLAG_HAS_CHILDREN) {
        fprintf(fi, "%s%s\n", p1, n->type->name);
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
    } else if(n->type == &node_var) {
        fprintf(fi, "%s%s (%d)\n", p1, n->type->name, n->value.i);
    } else if(n->type == &node_category) {
        fprintf(fi, "%s%s (%d)\n", p1, n->type->name, n->value.c);
    } else {
        fprintf(fi, "%s%s\n", p1, n->type->name);
    }
}

void node_print(node_t*n)
{
    printf("------------VVVV---------------\n");
    node_print2(n,"","",stdout);
    printf("-------------------------------\n");
}

void value_print(value_t*v)
{
    switch(v->type) {
        case TYPE_FLOAT:
            printf("FLOAT(%f)\n", v->f);
        break;
        case TYPE_INT:
            printf("INT(%d)\n", v->i);
        break;
        case TYPE_CATEGORY:
            printf("CATEGORY(%d)\n", v->c);
        break;
        case TYPE_BOOL:
            printf("BOOL(%s)\n", v->b?"true":"false");
        break;
        case TYPE_MISSING:
            printf("BOOL(%s)\n", v->b?"true":"false");
        break;
        default:
            printf("BAD VALUE\n");
        break;
    }
}

