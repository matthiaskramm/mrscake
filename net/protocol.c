#include <sys/types.h>
#include <unistd.h>
#include <sys/times.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <memory.h>
#ifdef HAVE_SYS_TIMEB
#include <sys/timeb.h>
#endif
#include "protocol.h"
#include "serialize.h"
#include "io.h"

void make_request_TRAIN_MODEL(writer_t*w, const char*model_name, const char*transforms, dataset_t*dataset)
{
    write_uint8(w, REQUEST_TRAIN_MODEL);
    w->write(w, dataset->hash, HASH_SIZE);
    if(w->error)
        return;
    write_string(w, model_name);
    write_string(w, transforms);
}
void process_request_TRAIN_MODEL(datacache_t*cache, reader_t*r, writer_t*w)
{
    uint8_t hash[HASH_SIZE];
    r->read(r, &hash, HASH_SIZE);
    if(r->error)
        return;

    dataset_t*dataset = datacache_find(cache, hash);
    if(!dataset) {
        write_uint8(w, RESPONSE_DATASET_UNKNOWN);
        return;
    }

    char*name = read_string(r);
    char*transforms = read_string(r);
    if(r->error)
        return;

    printf("worker %d: processing model %s|%s\n", getpid(), transforms, name);
    model_factory_t* factory = model_factory_get_by_name(name);
    if(!factory) {
        printf("worker %d: unknown factory '%s'\n", getpid(), name);
        write_uint8(w, RESPONSE_FACTORY_UNKNOWN);
        return;
    }

    printf("worker %d: %d rows of data\n", getpid(), dataset->num_rows);

    struct tms tms_before, tms_after;
    times(&tms_before);

    job_t j;
    memset(&j, 0, sizeof(j));
    j.factory = factory;
    j.data = dataset;
    j.code = 0;
    j.transforms = transforms;
    j.flags = JOB_NO_FORK;
    job_process(&j);

    times(&tms_after);

    printf("worker %d: finished training (time: %.2f)\n", getpid(), (tms_after.tms_utime - tms_before.tms_utime) /  (float)sysconf(_SC_CLK_TCK));
    write_uint8(w, RESPONSE_OK);
    write_compressed_int(w, (tms_after.tms_utime - tms_before.tms_utime) * 1000ll / sysconf(_SC_CLK_TCK));
    write_compressed_int(w, j.score);

    uint8_t want_data = read_uint8(r);
    if(want_data == REQUEST_SEND_CODE) {
        printf("worker %d: sending out model data\n", getpid());
        write_uint8(w, RESPONSE_DATA_FOLLOWS);
        node_write(j.code, w, SERIALIZE_DEFAULTS);
    } else if(want_data == REQUEST_DISCARD_CODE) {
        printf("worker %d: discarding model data\n", getpid());
        // the server already has a better model for this data
    } else {
        printf("worker %d: Invalid response from server after training\n", getpid());
    }
}
void finish_request_TRAIN_MODEL(reader_t*r, writer_t*w, remote_job_t*rjob, int32_t cutoff)
{
    job_t*dest = rjob->job;
    rjob->response = read_uint8(r);
    if(rjob->response != RESPONSE_OK) {
        rjob->cpu_time = 0.0;
        dest->score = INT32_MAX;
        dest->code = NULL;
        return;
    }

    rjob->cpu_time = read_compressed_int(r) / 1000.0;
    dest->score = read_compressed_int(r);

    if(dest->score >= cutoff) {
        write_uint8(w, REQUEST_DISCARD_CODE);
        dest->code = NULL;
    } else {
        write_uint8(w, REQUEST_SEND_CODE);
        int resp = read_uint8(r);
        if(resp != RESPONSE_DATA_FOLLOWS) {
            rjob->response = resp|0x80;
            dest->score = INT32_MAX;
            dest->code = NULL;
        } else {
            dest->code = node_read(r);
        }
    }
}

dataset_t* make_request_SEND_DATASET(reader_t*r, writer_t*w, uint8_t*hash)
{
    write_uint8(w, REQUEST_SEND_DATASET);
    w->write(w, hash, HASH_SIZE);
    uint8_t response = read_uint8(r);
    if(response!=RESPONSE_OK)
        return NULL;
    return dataset_read(r);
}
void process_request_SEND_DATASET(datacache_t*datacache, reader_t*r, writer_t*w)
{
    uint8_t hash[HASH_SIZE];
    r->read(r, &hash, HASH_SIZE);
    if(r->error)
        return;
    char*hashstr = hash_to_string(hash);

    dataset_t*dataset = datacache_find(datacache, hash);
    if(!dataset) {
        printf("worker %d: dataset unknown\n", getpid());
        write_uint8(w, RESPONSE_DATASET_UNKNOWN);
        return;
    }
    printf("worker %d: sending out dataset %s\n", getpid(), hashstr);
    write_uint8(w, RESPONSE_OK);
    dataset_write(dataset, w);
}
bool make_request_RECV_DATASET(reader_t*r, writer_t*w, dataset_t*dataset, remote_server_t*other_server)
{
    write_uint8(w, REQUEST_RECV_DATASET);
    if(w->error) {
        printf("%s\n", w->error);
        return false;
    }
    w->write(w, dataset->hash, HASH_SIZE);
    if(w->error) {
        printf("%s\n", w->error);
        return false;
    }

    uint8_t status = read_uint8(r);
    if(status != RESPONSE_GO_AHEAD &&
       status != RESPONSE_DUPL_DATA) {
        printf("bad status (%02x)\n", status);
        return false;
    }
    if(status == RESPONSE_DUPL_DATA) {
        return true;
    }

    if(other_server) {
        write_string(w, other_server->host);
        write_compressed_uint(w, other_server->port);
    } else {
        write_string(w, "");
        write_compressed_uint(w, 0);
        dataset_write(dataset, w);
    }
    return true;
}
void process_request_RECV_DATASET(datacache_t*datacache, reader_t*r, writer_t*w)
{
    uint8_t hash[HASH_SIZE];
    r->read(r, &hash, HASH_SIZE);
    if(r->error)
        return;
    char*hashstr = hash_to_string(hash);
    printf("worker %d: reading dataset %s\n", getpid(), hashstr);

    dataset_t*dataset = datacache_find(datacache, hash);
    if(dataset!=NULL) {
        printf("worker %d: dataset already known\n", getpid());
        write_uint8(w, RESPONSE_DUPL_DATA);
        w->write(w, dataset->hash, HASH_SIZE);
        write_uint8(w, RESPONSE_DUPL_DATA);
        return;
    } else {
        write_uint8(w, RESPONSE_GO_AHEAD);
    }

    char*host = read_string(r);
    int port = read_compressed_uint(r);
    if(!*host) {
        dataset = dataset_read(r);
        if(r->error)
            return;
    } else {
        dataset = dataset_read_from_server(host, port, hash);
    }
    if(!dataset) {
        w->write(w, hash, HASH_SIZE);
        write_uint8(w, RESPONSE_DATA_ERROR);
        return;
    }
    if(memcmp(dataset->hash, hash, HASH_SIZE)) {
        printf("worker %d: dataset has bad hash\n", getpid());
        w->write(w, hash, HASH_SIZE);
        write_uint8(w, RESPONSE_DATA_ERROR);
        return;
    }
    datacache_store(datacache, dataset);
    w->write(w, dataset->hash, HASH_SIZE);
    write_uint8(w, RESPONSE_OK);
    printf("worker %d: dataset stored\n", getpid());
}

void process_request(datacache_t*cache, int socket)
{
    reader_t*r = filereader_new(socket);
    writer_t*w = filewriter_new(socket);

    uint8_t request_code = read_uint8(r);

    switch(request_code) {
        case REQUEST_TRAIN_MODEL:
            process_request_TRAIN_MODEL(cache, r, w);
        break;
        case REQUEST_RECV_DATASET:
            process_request_RECV_DATASET(cache, r, w);
        break;
        case REQUEST_SEND_DATASET:
            process_request_SEND_DATASET(cache, r, w);
        break;
    }
    w->finish(w);
}

bool send_header(int sock, bool accept_request, int num_jobs, int num_workers)
{
    uint8_t header[3] = {
        accept_request? RESPONSE_IDLE : RESPONSE_BUSY,
        num_jobs,
        num_workers};
    int ret = write(sock, header, 3);
    return ret == 3;
}

