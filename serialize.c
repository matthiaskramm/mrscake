/* serialize.c
   Serialization of models and ast trees.

   Part of the data prediction package.
   
   Copyright (c) 2010-2011 Matthias Kramm <kramm@quiss.org> 
 
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
#include <stdlib.h>
#include <memory.h>
#include "ast.h"
#include "io.h"
#include "stringpool.h"
#include "serialize.h"
#include "dataset.h"

static nodetype_t* opcode_to_node(uint8_t opcode)
{
    switch (opcode) {
#       define NODE(opcode, name) \
        case opcode: \
            return &name;
        LIST_NODES
#       undef NODE
        default:
            return 0;
    }
}

constant_t constant_read(reader_t*reader)
{
    constant_t c;
    c.type = read_uint8(reader);
    switch(c.type) {
        case CONSTANT_CATEGORY: {
            c.c = read_compressed_uint(reader);
            break;
        }
        case CONSTANT_FLOAT: {
            c.f = read_float(reader);
            break;
        }
        case CONSTANT_INT: {
            c.i = read_compressed_uint(reader);
            break;
        }
        case CONSTANT_STRING: {
            c.s = read_string(reader);
            break;
        }
        case CONSTANT_MISSING: {
            break;
        }
        default: {
            fprintf(stderr, "Can't deserialize constant type %d\n", c.type);
            c.type = 0;
            break;
        }
    }
    return c;
}

bool node_read_internal_data(node_t*node, reader_t*reader)
{
    nodetype_t*type = node->type;
    if(type==&node_mixed_array ||
       type==&node_int_array ||
       type==&node_category_array ||
       type==&node_string_array ||
       type==&node_float_array) {
        uint32_t len = read_compressed_uint(reader);
        int t;
        array_t*a = array_new(len);
        for(t=0;t<len;t++) {
            a->entries[t] = constant_read(reader);
            if(!a->entries[t].type)
                return false;
        }
        if(type==&node_mixed_array)
            node->value = mixed_array_constant(a);
        else if(type==&node_int_array)
            node->value = int_array_constant(a);
        else if(type==&node_category_array)
            node->value = category_array_constant(a);
        else if(type==&node_string_array)
            node->value = string_array_constant(a);
        else if(type==&node_float_array)
            node->value = float_array_constant(a);
    } else if(type==&node_zero_int_array) {
        int32_t len = read_compressed_uint(reader);
        array_t*a = array_new(len);
        array_fill(a, int_constant(0));
        node->value = int_array_constant(a);
    } else if(type==&node_category) {
        category_t c = read_compressed_uint(reader);
        node->value = category_constant(c);
    } else if(type==&node_float) {
        float f = read_float(reader);
        node->value = float_constant(f);
    } else if(type==&node_int) {
        int i = (int32_t)read_compressed_uint(reader);
        node->value = int_constant(i);
    } else if(type==&node_param) {
        int var_index = read_compressed_uint(reader);
        node->value = int_constant(var_index);
    } else if(type==&node_string) {
        char*s = read_string(reader);
        node->value = string_constant(s);
    } else if(type==&node_constant || type==&node_setlocal || type==&node_getlocal || type==&node_inclocal) {
        node->value = constant_read(reader);
        if(!node->value.type)
            return false;
    } else {
        fprintf(stderr, "Don't know how to deserialize node '%s' (%02x)\n", type->name, node_get_opcode(node));
        return false;
    }
    return true;
}

typedef struct _nodestack {
    struct _nodestack*prev;
    node_t*node;
    int num_children;
} nodestack_t;

nodestack_t*stack_new(node_t*node, nodestack_t*prev)
{
    nodestack_t*d = malloc(sizeof(nodestack_t));
    d->node = node;
    d->num_children = 0;
    d->prev = prev;
    return d;
}
nodestack_t*stack_pop(nodestack_t*stack)
{
    nodestack_t*old = stack;
    stack = stack->prev;
    free(old);
    return stack;
}

node_t* node_read(reader_t*reader)
{
    nodestack_t*stack = 0;
    node_t*top_node = 0;

    do {
        uint8_t opcode = read_uint8(reader);
        nodetype_t*type = opcode_to_node(opcode);
        if(!type)
            return NULL;
        node_t*node = node_new(type, stack?stack->node:0);
        stack = stack_new(node, stack);
        if(type->flags & NODE_FLAG_HAS_VALUE) {
            if(!node_read_internal_data(node, reader))
                return NULL;
        }
        if(type->flags&NODE_FLAG_HAS_CHILDREN) {
            if(type->min_args == type->max_args) {
                stack->num_children = type->min_args;
            } else {
                stack->num_children = read_compressed_uint(reader);
            }
        }
        if(stack->prev) {
            node_t*prev_node = stack->prev->node;
            node_append_child(prev_node, node);
        }
        while(stack && stack->num_children == stack->node->num_children) {
            top_node = stack->node;
            stack = stack_pop(stack);
        }
        if(reader->error)
            return NULL;
    } while(stack);

    if(!node_sanitycheck(top_node))
        return NULL;

    return top_node;
}

static void constant_write(constant_t*value, writer_t*writer, unsigned flags)
{
    write_uint8(writer, value->type);
    switch(value->type) {
        case CONSTANT_CATEGORY: {
            category_t c = AS_CATEGORY(*value);
            write_compressed_uint(writer, c);
            break;
        }
        case CONSTANT_FLOAT: {
            float f = AS_FLOAT(*value);
            write_float(writer, f);
            break;
        }
        case CONSTANT_INT: {
            int var_index = AS_INT(*value);
            write_compressed_uint(writer, var_index);
            break;
        }
        case CONSTANT_STRING: {
            const char*s = AS_STRING(*value);
            if(flags & SERIALIZE_FLAG_OMIT_STRINGS) {
                write_uint8(writer, 0);
            } else {
                write_string(writer, s);
            }
            break;
        }
        case CONSTANT_MIXED_ARRAY:
        case CONSTANT_INT_ARRAY:
        case CONSTANT_FLOAT_ARRAY:
        case CONSTANT_CATEGORY_ARRAY:
        case CONSTANT_STRING_ARRAY: {
            array_t*a = AS_ARRAY(*value);
            int t;
            assert(a->size <= 255);
            write_compressed_uint(writer, a->size);
            for(t=0;t<a->size;t++) {
                constant_write(&a->entries[t], writer, flags);
            }
            break;
        }
        case CONSTANT_MISSING: {
            break;
        }
        default:
            fprintf(stderr, "Can't serialize constant type %d\n", value->type);
            exit(1);
    }
}

static void node_write_internal_data(node_t*node, writer_t*writer, unsigned flags)
{
    if(node->type==&node_int_array ||
       node->type==&node_float_array ||
       node->type==&node_category_array ||
       node->type==&node_mixed_array ||
       node->type==&node_string_array) {
        array_t*a = node->value.a;
        assert(a->size <= 255);
        write_compressed_uint(writer, a->size);
        int t;
        for(t=0;t<a->size;t++) {
            constant_write(&a->entries[t], writer, flags);
        }
    } else if(node->type==&node_category) {
        category_t c = AS_CATEGORY(node->value);
        write_compressed_uint(writer, c);
    } else if(node->type==&node_float) {
        float f = AS_FLOAT(node->value);
        write_float(writer, f);
    } else if(node->type==&node_int) {
        int i = AS_INT(node->value);
        write_compressed_uint(writer, i);
    } else if(node->type==&node_string) {
        const char*s = AS_STRING(node->value);
        if(flags & SERIALIZE_FLAG_OMIT_STRINGS) {
            write_uint8(writer, 0);
        } else {
            write_string(writer, s);
        }
    } else if(node->type==&node_param) {
        int var_index = AS_INT(node->value);
        write_compressed_uint(writer, var_index);
    } else if(node->type==&node_zero_int_array) {
        int size = node->value.a->size;
        write_compressed_uint(writer, size);
    } else if(node->type->flags&NODE_FLAG_HAS_VALUE) {
        constant_write(&node->value, writer, flags);
    }
}

void node_write(node_t*node, writer_t*writer, unsigned flags)
{
    uint8_t opcode = node_get_opcode(node);
    write_uint8(writer, opcode);
    node_write_internal_data(node, writer, flags);

    if(node->type->flags & NODE_FLAG_HAS_CHILDREN) {
        int t;
        if(node->type->min_args == node->type->max_args) {
            assert(node->type->min_args == node->num_children);
        } else {
            write_compressed_uint(writer, node->num_children);
        }
        for(t=0;t<node->num_children;t++) {
            node_write(node->child[t], writer, flags);
        }
    }
}

signature_t* signature_read(reader_t*r)
{
    signature_t*sig = malloc(sizeof(signature_t));
    sig->num_inputs = read_compressed_uint(r);
    uint8_t flags = read_uint8(r);
    int t;
    if(flags&1) {
        sig->column_names = calloc(sig->num_inputs, sizeof(sig->column_names[0]));
        for(t=0;t<sig->num_inputs;t++) {
            char*s = read_string(r);
            sig->column_names[t] = register_and_free_string(s);
        }
    }
    if(flags&2) {
        sig->column_types = calloc(sig->num_inputs, sizeof(sig->column_types[0]));
        for(t=0;t<sig->num_inputs;t++) {
            sig->column_types[t] = read_compressed_uint(r);
        }
    }
    sig->has_column_names = (flags&4) != 0;
    return sig;
}

model_t* model_read(reader_t*r)
{
    model_t*m = (model_t*)calloc(1, sizeof(model_t));
    m->name = register_and_free_string(read_string(r));
    if(!*m->name)
        return NULL;

    m->sig = signature_read(r);
    m->code = (void*)node_read(r);
    if(!m->code) {
        free(m);
        return NULL;
    }
    return m;
}
model_t* model_load(const char*filename)
{
    reader_t *r = filereader_new2(filename);
    model_t*m = model_read(r);
    r->dealloc(r);
    return m;
}
void signature_write(signature_t*sig, writer_t*w)
{
    write_compressed_uint(w, sig->num_inputs);
    uint8_t flags = 0;
    if(sig->column_names)
        flags |= 1;
    if(sig->column_types)
        flags |= 2;
    if(sig->has_column_names)
        flags |= 4;
    write_uint8(w, flags);

    if(sig->column_names) {
        int t;
        for(t=0;t<sig->num_inputs;t++) {
            write_string(w, sig->column_names[t]);
        }
    }
    if(sig->column_types) {
        int t;
        for(t=0;t<sig->num_inputs;t++) {
            write_compressed_uint(w, sig->column_types[t]);
        }
    }
}
void model_write(model_t*m, writer_t*w)
{
    if(!m) {
        /* null model */
        write_uint8(w, 0);
        return;
    }
    assert(m->name && m->name[0]);
    write_string(w, m->name);

    signature_write(m->sig, w);

    node_t*node = (node_t*)m->code;
    node_write(node, w, 0);
}
void model_save(model_t*m, const char*filename)
{
    writer_t *w = filewriter_new2(filename);
    model_write(m, w);
    w->finish(w);
}

variable_t variable_read(reader_t*r)
{
    char*s;
    variable_t v;
    v.type = read_compressed_uint(r);
    switch(v.type) {
        case CATEGORICAL:
            v.category = read_compressed_uint(r);
            break;
        case CONTINUOUS:
            v.value = read_float(r);
            break;
        case TEXT:
            s = read_string(r);
            v.text = register_string(s);
            free(s);
            break;
    }
    return v;
}

void variable_write(variable_t*v, writer_t*w)
{
    write_compressed_uint(w, v->type);
    switch(v->type) {
        case CATEGORICAL:
            write_compressed_uint(w, v->category);
            break;
        case CONTINUOUS:
            write_float(w, v->value);
            break;
        case TEXT:
            write_string(w, v->text);
            break;
    }
}

trainingdata_t* trainingdata_read(reader_t*r)
{
    int num = read_compressed_uint(r);
    int t;
    trainingdata_t*data = trainingdata_new();
    for(t=0;t<num;t++) {
        int num_inputs = read_compressed_uint(r);
        example_t*e = example_new(num_inputs);
        uint8_t flags = read_uint8(r);
        if(flags&1) {
            e->input_names = (const char**)malloc(sizeof(const char*)*num_inputs);
        }
        int s;
        for(s=0;s<num_inputs;s++) {
            if(e->input_names)
                e->input_names[s] = read_string(r);
            e->inputs[s] = variable_read(r);
        }
        e->desired_response = variable_read(r);
        trainingdata_add_example(data, e);
    }
    return data;
}
trainingdata_t* trainingdata_load(const char*filename)
{
    reader_t *r = filereader_new2(filename);
    trainingdata_t*d = trainingdata_read(r);
    r->dealloc(r);
    return d;
}

void trainingdata_write(trainingdata_t*d, writer_t*w)
{
    write_compressed_uint(w, d->num_examples);
    example_t*e = d->first_example;
    int count = 0;
    while(e) {
        write_compressed_uint(w, e->num_inputs);
        write_uint8(w, e->input_names ? 1 : 0);
        int t;
        for(t=0;t<e->num_inputs;t++) {
            if(e->input_names)
                write_string(w, e->input_names[t]);
            variable_write(&e->inputs[t], w);
        }
        variable_write(&e->desired_response, w);
        e = e->next;
        count++;
    }
    assert(count == d->num_examples);
}
void trainingdata_save(trainingdata_t*d, const char*filename)
{
    writer_t *w = filewriter_new2(filename);
    trainingdata_write(d, w);
    w->finish(w);
}

void column_write(column_t*c, int num_rows, writer_t*w)
{
    write_string(w, c->name);
    write_compressed_int(w, c->index);
    write_uint8(w, c->is_categorical);
    int y;
    if(c->is_categorical) {
        write_compressed_uint(w, c->num_classes);
        int t;
        for(t=0;t<c->num_classes;t++) {
            constant_write(&c->classes[t], w, 0);
            write_compressed_uint(w, c->class_occurence_count[t]);
        }
        for(y=0;y<num_rows;y++) {
            write_compressed_uint(w, c->entries[y].c);
        }
    } else {
        for(y=0;y<num_rows;y++) {
            write_float(w, c->entries[y].f);
        }
    }
}
column_t* column_read(int num_rows, reader_t*r)
{
    char*name = read_string(r);
    int index = read_compressed_int(r);
    char is_categorical = read_uint8(r);
    column_t* c = column_new(num_rows, is_categorical, index);
    if(*name) {
        c->name = register_string(name);
    } else {
        c->name = 0;
    }
    free(name);
    int y;
    if(c->is_categorical) {
        c->num_classes = read_compressed_uint(r);
        c->classes = malloc(sizeof(c->classes[0])*c->num_classes);
        c->class_occurence_count = malloc(sizeof(c->class_occurence_count[0])*c->num_classes);
        int t;
        for(t=0;t<c->num_classes;t++) {
            c->classes[t] = constant_read(r);
            c->class_occurence_count[t] = read_compressed_uint(r);
        }
        for(y=0;y<num_rows;y++) {
            c->entries[y].c = read_compressed_uint(r);
        }
    } else {
        for(y=0;y<num_rows;y++) {
            c->entries[y].f = read_float(r);
        }
    }
    return c;
}
void sanitized_dataset_write(sanitized_dataset_t*d, writer_t*w)
{
    write_compressed_uint(w, d->num_columns);
    write_compressed_uint(w, d->num_rows);
    int t;
    for(t=0;t<d->num_columns;t++) {
        column_write(d->columns[t], d->num_rows, w);
    }
    column_write(d->desired_response, d->num_rows, w);
}
sanitized_dataset_t*sanitized_dataset_read(reader_t*r)
{
    sanitized_dataset_t*d = calloc(1, sizeof(sanitized_dataset_t));
    d->num_columns = read_compressed_uint(r);
    d->num_rows = read_compressed_uint(r);
    d->columns = malloc(sizeof(d->columns[0])*d->num_columns);
    int t;
    for(t=0;t<d->num_columns;t++) {
        d->columns[t] = column_read(d->num_rows, r);
    }
    d->desired_response = column_read(d->num_rows, r);
    return d;
}
void sanitized_dataset_save(sanitized_dataset_t*d, const char*filename)
{
    writer_t *w = filewriter_new2(filename);
    sanitized_dataset_write(d, w);
    w->finish(w);
}
sanitized_dataset_t* sanitized_dataset_load(const char*filename)
{
    reader_t *r = filereader_new2(filename);
    sanitized_dataset_t*d = sanitized_dataset_read(r);
    r->dealloc(r);
    return d;
}
