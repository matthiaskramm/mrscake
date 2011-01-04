#ifndef __serialize_h__
#define __serialize_h__

#ifdef __cplusplus
extern "C" {
#endif

node_t* node_read(reader_t*read);
void node_write(node_t*node, writer_t*writer);

model_t* model_load(const char*filename);
void model_save(model_t*m, const char*filename);

#ifdef __cplusplus
}
#endif

#endif
