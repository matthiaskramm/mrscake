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
                    a->entries[t] = category_constant(read_uint8(reader));
                }
                NODE_BEGIN(type, a);
            } else if(type==&node_category) {
                category_t c = read_uint8(reader);
                NODE_BEGIN(type, c);
            } else if(type==&node_var) {
                int var_index = read_uint8(reader);
                NODE_BEGIN(type, var_index);
            } else {
                fprintf(stderr, "Don't know how to deserialize this node type (%02x)\n", opcode);
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

static void node_write_internal_data(node_t*node, writer_t*writer)
{    
    if(node->type==&node_array) {
        array_t*a = node->value.a;
        assert(a->size <= 255);
        write_uint8(writer, a->size);
        int t;
        for(t=0;t<a->size;t++) {
            category_t c = AS_CATEGORY(a->entries[t]);
            assert(c <= 255);
            write_uint8(writer, c);
        }
    } else if(node->type==&node_category) {
        category_t c = AS_CATEGORY(node->value);
        write_uint8(writer, c);
    } else if(node->type==&node_var) {
        int var_index = AS_INT(node->value);
        write_uint8(writer, var_index);
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

