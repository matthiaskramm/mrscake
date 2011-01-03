#ifndef __serialize_h__
#define __serialize_h__

#ifdef __cplusplus
extern "C" {
#endif

node_t* node_read(reader_t*read);
void node_write(node_t*node, writer_t*writer);

#ifdef __cplusplus
}
#endif

#endif
