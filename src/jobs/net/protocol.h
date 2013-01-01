#ifndef __protocol_h__
#define __protocol_h__

#include "datacache.h"
#include "io.h"
#include "net/distribute.h"

typedef enum {REQUEST_SEND_DATASET,
              REQUEST_RECV_DATASET,
              REQUEST_TRAIN_MODEL,
              REQUEST_SEND_CODE,
              REQUEST_DISCARD_CODE,
             } request_type_t;

typedef enum {RESPONSE_OK,
              RESPONSE_DATA_ERROR,
              RESPONSE_DUPL_DATA,
              RESPONSE_DATASET_UNKNOWN,
              RESPONSE_FACTORY_UNKNOWN,
              RESPONSE_GO_AHEAD,
              RESPONSE_DATA_FOLLOWS,
              RESPONSE_BUSY,
              RESPONSE_IDLE,
              RESPONSE_READ_ERROR=-1} response_type_t;

void make_request_TRAIN_MODEL(writer_t*w, const char*model_name, const char*transforms, dataset_t*dataset);
void process_request_TRAIN_MODEL(datacache_t*cache, reader_t*r, writer_t*w);
void finish_request_TRAIN_MODEL(reader_t*r, writer_t*w, remote_job_t*rjob, int32_t cutoff);

dataset_t* make_request_SEND_DATASET(reader_t*r, writer_t*w, uint8_t*hash);
void process_request_SEND_DATASET(datacache_t*datacache, reader_t*r, writer_t*w);

bool make_request_RECV_DATASET(reader_t*r, writer_t*w, dataset_t*dataset, remote_server_t*other_server);
void process_request_RECV_DATASET(datacache_t*datacache, reader_t*r, writer_t*w);

bool send_header(int sock, bool accept_request, int num_jobs, int num_workers);

void process_request(datacache_t*cache, int socket);
bool send_header(int sock, bool accept_request, int num_jobs, int num_workers);

#endif
