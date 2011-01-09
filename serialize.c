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
            c.c = read_uint8(reader);
            break;
        }
        case CONSTANT_FLOAT: {
            c.f = read_float(reader);
            break;
        }
        case CONSTANT_INT: {
            c.i = read_uint8(reader);
            break;
        }
        case CONSTANT_STRING: {
            c.s = read_string(reader);
            break;
        }
        case CONSTANT_MISSING: {
            break;
        }
        default:
            fprintf(stderr, "Can't deserialize constant type %d\n", c.type);
            exit(1);
    }
    return c;
}

node_t* node_read(reader_t*reader)
{
    uint8_t opcode_root = read_uint8(reader);
    assert(opcode_root == node_root.opcode);
    START_CODE(root)

    do {
        uint8_t opcode = read_uint8(reader);
        nodetype_t*type = opcode_to_node(opcode);
        assert(type);
        if(type->flags & NODE_FLAG_HAS_VALUE) {
            if(type==&node_array) {
                uint8_t len = read_uint8(reader);
                int t;
                array_t*a = array_new(len);
                for(t=0;t<len;t++) {
                    a->entries[t] = constant_read(reader);
                }
                NODE_BEGIN(type, a);
            } else if(type==&node_category) {
                category_t c = read_uint8(reader);
                NODE_BEGIN(type, c);
            } else if(type==&node_float) {
                float f = read_float(reader);
                NODE_BEGIN(type, f);
            } else if(type==&node_var) {
                int var_index = read_uint8(reader);
                NODE_BEGIN(type, var_index);
            } else if(type==&node_string) {
                char*s = read_string(reader);
                NODE_BEGIN(type, s);
            } else if(type==&node_constant) {
                constant_t c = constant_read(reader);
                NODE_BEGIN(type, c);
            } else {
                fprintf(stderr, "Don't know how to deserialize node '%s' (%02x)\n", type->name, opcode);
                return 0;
            }
        } else {
            NODE_BEGIN(type);
        }
        while(current_node && current_node->num_children == current_node->type->max_args) {
            NODE_CLOSE;
        }
    } while(current_node);
    END_CODE;
    return root;
}

static void constant_write(constant_t*value, writer_t*writer)
{
    write_uint8(writer, value->type);
    switch(value->type) {
        case CONSTANT_CATEGORY: {
            category_t c = AS_CATEGORY(*value);
            write_uint8(writer, c);
            break;
        }
        case CONSTANT_FLOAT: {
            float f = AS_FLOAT(*value);
            write_float(writer, f);
            break;
        }
        case CONSTANT_INT: {
            int var_index = AS_INT(*value);
            write_uint8(writer, var_index);
            break;
        }
        case CONSTANT_STRING: {
            char*s = AS_STRING(*value);
            write_string(writer, s);
            break;
        }
        case CONSTANT_ARRAY: {
            array_t*a = AS_ARRAY(*value);
            int t;
            assert(a->size <= 255);
            write_uint8(writer, a->size);
            for(t=0;t<a->size;t++) {
                constant_write(&a->entries[t], writer);
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

static void node_write_internal_data(node_t*node, writer_t*writer)
{
    if(node->type==&node_array) {
        array_t*a = node->value.a;
        assert(a->size <= 255);
        write_uint8(writer, a->size);
        int t;
        for(t=0;t<a->size;t++) {
            constant_write(&a->entries[t], writer);
        }
    } else if(node->type==&node_category) {
        category_t c = AS_CATEGORY(node->value);
        write_uint8(writer, c);
    } else if(node->type==&node_float) {
        float f = AS_FLOAT(node->value);
        write_float(writer, f);
    } else if(node->type==&node_string) {
        char*s = AS_STRING(node->value);
        write_string(writer, s);
    } else if(node->type==&node_var) {
        int var_index = AS_INT(node->value);
        write_uint8(writer, var_index);
    } else if(node->type==&node_constant) {
        constant_write(&node->value, writer);
    }
}

void node_write(node_t*node, writer_t*writer)
{
    uint8_t opcode = node_get_opcode(node);
    write_uint8(writer, opcode);
    node_write_internal_data(node, writer);

    if(node->type->flags & NODE_FLAG_HAS_CHILDREN) {
        int t;
        for(t=0;t<node->num_children;t++) {
            node_write(node->child[t], writer);
        }
    }
}
model_t* model_load(const char*filename)
{
    model_t*m = (model_t*)malloc(sizeof(model_t));
    reader_t *r = filereader_new2(filename);
    m->code = (void*)node_read(r);
    m->wordmap = 0;
    r->dealloc(r);
    return m;
}
void model_save(model_t*m, const char*filename)
{
    node_t*code = (node_t*)m->code;
    writer_t *w = filewriter_new2(filename);
    node_write(code, w);
    w->finish(w);
}
