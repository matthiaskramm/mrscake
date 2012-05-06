/*  io.c 
    Generic object-oriented reading/writing.

    Copyright (C) 2001-2011 Matthias Kramm <kramm@quiss.org>

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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/select.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_IO_H
#include <io.h>
#endif
#include <string.h>
#include <memory.h>
#define __USE_LARGEFILE64
#include <fcntl.h>
#include <errno.h>
#include <sys/select.h>

#include "config.h"

#ifdef HAVE_ZLIB
#include <zlib.h>
#define ZLIB_BUFFER_SIZE 16384
#endif

#ifdef HAVE_SHA1
#include <openssl/sha.h>
#endif

#include "io.h"

/* ---------------------------- null reader ------------------------------- */

static int reader_nullread(reader_t*r, void* data, int len)
{
    memset(data, 0, len);
    return len;
}
static void reader_nullread_dealloc(reader_t*r)
{
    free(r);
}
static int reader_nullseek(reader_t*r, int pos)
{
    return pos;
}
reader_t*nullreader_new()
{
    reader_t*r = (reader_t*)malloc(sizeof(reader_t));
    r->error = NULL;
    r->read = reader_nullread;
    r->seek = reader_nullseek;
    r->dealloc = reader_nullread_dealloc;
    r->internal = 0;
    r->type = READER_TYPE_NULL;
    r->mybyte = 0;
    r->bitpos = 8;
    r->pos = 0;
    return r;
}
/* ---------------------------- file reader ------------------------------- */

typedef struct _filereader_internal {
    int handle;
    int timeout;
} filereader_internal_t;

static int read_with_retry(reader_t*r, int handle, void*_data, int len)
{
    unsigned char*data = (unsigned char*)_data;
    int pos = 0;
    while(pos<len) {
        int ret = read(handle, data+pos, len-pos);
        if(ret<0) {
            if(errno == EINTR || errno == EAGAIN)
                continue;
            r->error = strerror(errno);
            return ret;
        }
        if(ret==0) {
            // EOF
            r->error = "short read";
            return pos;
        }
        pos += ret;
    }
    r->pos += len;
    return len;
}
static int reader_fileread(reader_t*r, void* data, int len)
{
    filereader_internal_t*i = (filereader_internal_t*)r->internal;
    int ret = read_with_retry(r, i->handle, data, len);
    return ret;
}
static int reader_fileread_with_timeout(reader_t*r, void* data, int len)
{
    filereader_internal_t*i = (filereader_internal_t*)r->internal;
    struct timeval timeout;
    timeout.tv_sec = i->timeout;
    timeout.tv_usec = 0;

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(i->handle, &readfds);
    int ret;

    int pos = 0;
    while(pos<len) {
        while(1) {
            ret = select(i->handle+1, &readfds, NULL, NULL, &timeout);
            if(ret<0) {
                if(errno == EINTR || errno == EAGAIN)
                    continue;
                perror("select");
                r->error = strerror(errno);
                return -1;
            }
            if(ret>=0)
                break;
        }
        if(!FD_ISSET(i->handle, &readfds)) {
            fprintf(stderr, "timeout while trying to read %d bytes\n", len);
            r->error = "timeout";
            return -1;
        }

        ret = read(i->handle, data+pos, len-pos);
        if(ret<0) {
            if(errno == EINTR || errno == EAGAIN)
                continue;
            perror("read");
            r->error = strerror(errno);
            return ret;
        }
        if(ret==0) {
            // EOF
            r->error = "short read";
            return pos;
        }
        pos += ret;
    }
    return len;
}
static void reader_fileread_dealloc(reader_t*r)
{
    filereader_internal_t*i = (filereader_internal_t*)r->internal;
    if(r->type == READER_TYPE_FILE2) {
        close(i->handle);
    }
    free(r->internal);
    free(r);
}
static int reader_fileread_seek(reader_t*r, int pos)
{
    filereader_internal_t*i = (filereader_internal_t*)r->internal;
    return lseek(i->handle, pos, SEEK_SET);
}
reader_t* filereader_new(int handle)
{
    filereader_internal_t*i = (filereader_internal_t*)malloc(sizeof(filereader_internal_t));
    reader_t*r = (reader_t*)malloc(sizeof(reader_t));
    r->error = NULL;
    r->read = reader_fileread;
    r->seek = reader_fileread_seek;
    r->dealloc = reader_fileread_dealloc;
    r->internal = i;
    r->type = READER_TYPE_FILE;
    r->mybyte = 0;
    r->bitpos = 8;
    r->pos = 0;
    i->handle = handle;
    return r;
}
reader_t* filereader_with_timeout_new(int handle, int seconds)
{
    reader_t*r = filereader_new(handle);
    r->read = reader_fileread_with_timeout;
    filereader_internal_t*i = (filereader_internal_t*)r->internal;
    i->timeout = seconds;
    return r;
}
reader_t* filereader_new2(const char*filename)
{
#ifdef HAVE_OPEN64
    int fi = open64
#else
    int fi = open
#endif
    (filename,
#ifdef O_BINARY
            O_BINARY|
#endif
            O_RDONLY);
    if(fi < 0)
        return NULL;
    reader_t*r = filereader_new(fi);
    r->type = READER_TYPE_FILE2;
    return r;
}

/* ---------------------------- mem reader ------------------------------- */

typedef struct _memread
{
    unsigned char*data;
    int length;
} memread_t;

static int reader_memread(reader_t*reader, void* data, int len) 
{
    memread_t*mr = (memread_t*)reader->internal;

    if(mr->length - reader->pos < len) {
        len = mr->length - reader->pos;
    }
    if(!len) return 0;
    memcpy(data, &mr->data[reader->pos], len);
    reader->pos += len;
    return len;
}
static int reader_memseek(reader_t*reader, int pos)
{
    memread_t*mr = (memread_t*)reader->internal;
    if(pos>=0 && pos<=mr->length) {
        reader->pos = pos;
        return pos;
    } else {
        return -1;
    }
}
static void reader_memread_dealloc(reader_t*reader)
{
    if(reader->internal)
        free(reader->internal);
    free(reader);
}
static void reader_memread_dealloc_withdata(reader_t*reader)
{
    memread_t*mr = (memread_t*)reader->internal;
    if(mr->data)
        free(mr->data);
    if(reader->internal)
        free(reader->internal);
    free(reader);
}
reader_t* memreader_new(void*newdata, int newlength)
{
    reader_t*r = (reader_t*)malloc(sizeof(reader_t));
    memread_t*mr = (memread_t*)malloc(sizeof(memread_t));
    mr->data = (unsigned char*)newdata;
    mr->length = newlength;
    r->error = NULL;
    r->read = reader_memread;
    r->seek = reader_memseek;
    r->dealloc = reader_memread_dealloc;
    r->internal = (void*)mr;
    r->type = READER_TYPE_MEM;
    r->mybyte = 0;
    r->bitpos = 8;
    r->pos = 0;
    return r;
} 

/* ---------------------------- zzip reader ------------------------------ */
#ifdef HAVE_ZZIP
static int reader_zzip_read(reader_t*reader, void* data, int len) 
{
    return zzip_file_read((ZZIP_FILE*)reader->internal, data, len);
}
static void reader_zzip_dealloc(reader_t*reader)
{
    free(reader);
}
static int reader_zzip_seek(reader_t*reader, int pos)
{
    return zzip_seek((ZZIP_FILE*)reader->internal, pos, SEEK_SET);
}
reader_t* zzipreader_new(ZZIP_FILE*z)
{
    reader_t*r = (reader_t*)malloc(sizeof(reader_t));
    r->error = NULL;
    r->read = reader_zzip_read;
    r->seek = reader_zzip_seek;
    r->dealloc = reader_zzip_dealloc;
    r->internal = z;
    r->type = READER_TYPE_ZZIP;
    r->mybyte = 0;
    r->bitpos = 8;
    r->pos = 0;
    return r;
}
#endif

/* ---------------------------- mem writer ------------------------------- */

typedef struct _memwrite
{
    unsigned char*data;
    int length;
} memwrite_t;

static int writer_memwrite_write(writer_t*w, void* data, int len) 
{
    memwrite_t*mw = (memwrite_t*)w->internal;
    if(mw->length - w->pos > len) {
        memcpy(&mw->data[w->pos], data, len);
        w->pos += len;
        return len;
    } else {
        memcpy(&mw->data[w->pos], data, mw->length - w->pos);
        w->pos = mw->length;
        return mw->length - w->pos;
    }
}
static void writer_memwrite_finish(writer_t*w)
{
    if(w->internal) 
        free(w->internal);
    w->internal = 0;
    free(w);
}
static void dummy_flush(writer_t*w)
{
}
writer_t* memwriter_new(void*data, int len)
{
    writer_t*w = (writer_t*)malloc(sizeof(writer_t));
    memwrite_t *mr;
    mr = (memwrite_t*)malloc(sizeof(memwrite_t));
    mr->data = (unsigned char *)data;
    mr->length = len;
    memset(w, 0, sizeof(writer_t));
    w->write = writer_memwrite_write;
    w->flush = dummy_flush;
    w->finish = writer_memwrite_finish;
    w->internal = (void*)mr;
    w->type = WRITER_TYPE_MEM;
    w->bitpos = 0;
    w->mybyte = 0;
    w->pos = 0;
    return w;
}

/* ------------------------- growing mem writer ------------------------------- */

typedef struct _growmemwrite
{
    unsigned char*data;
    int length;
    uint32_t grow;
} growmemwrite_t;
static int writer_growmemwrite_write(writer_t*w, void* data, int len) 
{
    growmemwrite_t*mw = (growmemwrite_t*)w->internal;
    if(!mw->data) {
        fprintf(stderr, "Illegal write operation: data already given away");
        exit(1);
    }
    if(mw->length - w->pos < len) {
        int newlength = mw->length;
        while(newlength - w->pos < len) {
            newlength += mw->grow;
        }
#ifdef NO_REALLOC
        unsigned char*newmem = (unsigned char*)malloc(newlength);
        memcpy(newmem, mw->data, mw->length);
        free(mw->data);
        mw->data = newmem;
#else
        mw->data = (unsigned char*)realloc(mw->data, newlength);
#endif
        mw->length = newlength;
    }
    memcpy(&mw->data[w->pos], data, len);
    w->pos += len;
    return len;
}
static void writer_growmemwrite_finish(writer_t*w)
{
    growmemwrite_t*mw = (growmemwrite_t*)w->internal;
    if(mw->data) {
        free(mw->data);mw->data = 0;
    }
    mw->length = 0;
    free(w->internal);mw=0;
    free(w);
}
void* writer_growmemwrite_memptr(writer_t*w, int*len)
{
    growmemwrite_t*mw = (growmemwrite_t*)w->internal;
    if(len) {
        *len = w->pos;
    }
    return mw->data;
}
void* writer_growmemwrite_getmem(writer_t*w, int*len)
{
    growmemwrite_t*mw = (growmemwrite_t*)w->internal;
    void*ret = mw->data;
    if(len) {
        *len = w->pos;
    }
    /* remove our own reference so that neither write() nor finish() can free it.
       It's property of the caller now.
    */
    mw->data = 0;
    return ret;
}
void writer_growmemwrite_reset(writer_t*w)
{
    growmemwrite_t*mw = (growmemwrite_t*)w->internal;
    w->pos = 0;
    w->bitpos = 0;
    w->mybyte = 0;
}
writer_t* growingmemwriter_new()
{
    writer_t*w = (writer_t*)malloc(sizeof(writer_t));
    growmemwrite_t *mw = (growmemwrite_t *)malloc(sizeof(growmemwrite_t));
    mw->length = 4096;
    mw->data = (unsigned char *)malloc(mw->length);
    mw->grow = 4096;
    memset(w, 0, sizeof(writer_t));
    w->write = writer_growmemwrite_write;
    w->flush = dummy_flush;
    w->finish = writer_growmemwrite_finish;
    w->internal = (void*)mw;
    w->type = WRITER_TYPE_GROWING_MEM;
    w->bitpos = 0;
    w->mybyte = 0;
    w->pos = 0;
    return w;
}
writer_t* growingmemwriter_new2(uint32_t grow)
{
    writer_t*w = growingmemwriter_new();
    growmemwrite_t*mw = (growmemwrite_t*)w->internal;
    mw->grow = grow;
    return w;
}
reader_t* growingmemwriter_getreader(writer_t*w)
{
    int len;
    void*mem = writer_growmemwrite_getmem(w, &len);
    reader_t*r = memreader_new(mem, len);
    r->dealloc = reader_memread_dealloc_withdata;
    return r;
}

/* ---------------------------- file writer ------------------------------- */

typedef struct _filewrite
{
    int handle;
    char free_handle;
} filewrite_t;

static int writer_filewrite_write(writer_t*w, void* data, int len)
{
    if(len == 0) {
        return 0;
    }
    filewrite_t * fw= (filewrite_t*)w->internal;
    w->pos += len;
    int ret = write(fw->handle, data, len);
    if(ret<=0) {
        w->error = strerror(errno);
    } else if(ret<len) {
        w->error = "short write";
    }
    return ret;
}
static void writer_filewrite_finish(writer_t*w)
{
    filewrite_t *mr = (filewrite_t*)w->internal;
    if(mr->free_handle) {
        close(mr->handle);
    }
    free(w->internal);
    free(w);
}
writer_t* filewriter_new(int handle)
{
    writer_t*w = (writer_t*)malloc(sizeof(writer_t));
    filewrite_t *mr = (filewrite_t *)malloc(sizeof(filewrite_t));
    mr->handle = handle;
    mr->free_handle = 0;
    memset(w, 0, sizeof(writer_t));
    w->write = writer_filewrite_write;
    w->finish = writer_filewrite_finish;
    w->internal = mr;
    w->type = WRITER_TYPE_FILE;
    w->bitpos = 0;
    w->mybyte = 0;
    w->pos = 0;
    return w;
}
writer_t* filewriter_new2(const char*filename)
{
#ifdef HAVE_OPEN64
    int fi = open64
#else
    int fi = open
#endif
    (filename,
#ifdef O_BINARY
            O_BINARY|
#endif
            O_WRONLY|O_CREAT|O_TRUNC, 0644);
    if(fi<0) {
        perror(filename);
        return NULL;
    }
    writer_t*w = filewriter_new(fi);
    ((filewrite_t*)w->internal)->free_handle = 1;
    return w;
}

/* ---------------------------- null writer ------------------------------- */

static int writer_nullwrite_write(writer_t*w, void* data, int len) 
{
    w->pos += len;
    return len;
}
static void writer_nullwrite_finish(writer_t*w)
{
    free(w);
}
writer_t* nullwriter_new()
{
    writer_t*w = (writer_t*)malloc(sizeof(writer_t));
    memset(w, 0, sizeof(writer_t));
    w->write = writer_nullwrite_write;
    w->flush = dummy_flush;
    w->finish = writer_nullwrite_finish;
    w->internal = 0;
    w->type = WRITER_TYPE_NULL;
    w->bitpos = 0;
    w->mybyte = 0;
    w->pos = 0;
    return w;
}

/* ---------------------------- sha1 writer ------------------------------- */

typedef struct _sha1write
{
#ifdef HAVE_SHA1
    SHA_CTX ctx;
#endif
    bool needs_free;
} sha1write_t;

static int writer_sha1_write(writer_t*w, void* data, int len) 
{
#ifdef HAVE_SHA1
    sha1write_t*sha1 = (sha1write_t*)w->internal;
    SHA1_Update(&sha1->ctx, data, len);
    w->pos += len;
    return len;
#else
    fprintf(stderr, "Error: compiled without crypto support");
    exit(1);
#endif
}
static void writer_sha1_finish(writer_t*w)
{
#ifdef HAVE_SHA1
    sha1write_t*sha1 = (sha1write_t*)w->internal;
    if(sha1->needs_free) {
        uint8_t hash[HASH_SIZE];
        SHA1_Final(hash, &sha1->ctx);
        sha1->needs_free = false;
    }
    free(sha1);w->internal = NULL;
    free(w);
#else
    fprintf(stderr, "Error: compiled without crypto support");
    exit(1);
#endif
}
uint8_t* writer_sha1_get(writer_t*w)
{
#ifdef HAVE_SHA1
    sha1write_t*sha1 = (sha1write_t*)w->internal;
    uint8_t* hash = malloc(HASH_SIZE);
    SHA1_Final(hash, &sha1->ctx);
    sha1->needs_free = false;
    return hash;
#else
    fprintf(stderr, "Error: compiled without crypto support");
    exit(1);
#endif
}
writer_t* sha1writer_new()
{
#ifdef HAVE_SHA1
    writer_t*w = (writer_t*)calloc(1, sizeof(writer_t));
    sha1write_t*sha1 = (sha1write_t*)calloc(1, sizeof(sha1write_t));
    SHA1_Init(&sha1->ctx);
    sha1->needs_free = true;
    w->internal = sha1;
    w->write = writer_sha1_write;
    w->flush = dummy_flush;
    w->finish = writer_sha1_finish;
    w->type = WRITER_TYPE_SHA1;
    w->bitpos = 0;
    w->mybyte = 0;
    w->pos = 0;
    return w;
#else
    fprintf(stderr, "Error: compiled without crypto support");
    exit(1);
#endif
}

/* ---------------------------- zlibinflate reader -------------------------- */

typedef struct _zlibinflate
{
#ifdef HAVE_ZLIB
    z_stream zs;
    reader_t*input;
    unsigned char readbuffer[ZLIB_BUFFER_SIZE];
#endif
} zlibinflate_t;

#ifdef HAVE_ZLIB
static void zlib_error(int ret, char* msg, z_stream*zs)
{
    fprintf(stderr, "%s: zlib error (%d): last zlib error: %s\n",
          msg,
          ret,
          zs->msg?zs->msg:"unknown");
    if(errno) perror("errno:");
    exit(1);
}
#endif

static int reader_zlibinflate(reader_t*reader, void* data, int len) 
{
#ifdef HAVE_ZLIB
    zlibinflate_t*z = (zlibinflate_t*)reader->internal;
    int ret;
    if(!z) {
        return 0;
    }
    if(!len)
        return 0;
    
    z->zs.next_out = (Bytef *)data;
    z->zs.avail_out = len;

    while(1) {
        if(!z->zs.avail_in) {
            z->zs.avail_in = z->input->read(z->input, z->readbuffer, ZLIB_BUFFER_SIZE);
            z->zs.next_in = z->readbuffer;
        }
        if(z->zs.avail_in)
            ret = inflate(&z->zs, Z_NO_FLUSH);
        else
            ret = inflate(&z->zs, Z_FINISH);
    
        if (ret != Z_OK &&
            ret != Z_STREAM_END) zlib_error(ret, "bitio:inflate_inflate", &z->zs);

        if (ret == Z_STREAM_END) {
                int pos = z->zs.next_out - (Bytef*)data;
                ret = inflateEnd(&z->zs);
                if (ret != Z_OK) zlib_error(ret, "bitio:inflate_end", &z->zs);
                free(reader->internal);
                reader->internal = 0;
                reader->pos += pos;
                return pos;
        }
        if(!z->zs.avail_out) {
            break;
        }
    }
    reader->pos += len;
    return len;
#else
    fprintf(stderr, "Error: compiled without zlib support");
    exit(1);
#endif
}
static int reader_zlibseek(reader_t*reader, int pos)
{
    fprintf(stderr, "Error: seeking not supported for zlib streams");
    return -1;
}
static void reader_zlibinflate_dealloc(reader_t*reader)
{
#ifdef HAVE_ZLIB
    zlibinflate_t*z = (zlibinflate_t*)reader->internal;
    /* test whether read() already did basic deallocation */
    if(reader->internal) {
        inflateEnd(&z->zs);
        free(reader->internal);
    }
    memset(reader, 0, sizeof(reader_t));
    free(reader);
#endif
}
reader_t*zlibinflatereader_new(reader_t*input)
{
    reader_t*r = (reader_t*)malloc(sizeof(reader_t));
#ifdef HAVE_ZLIB
    zlibinflate_t*z = (zlibinflate_t*)malloc(sizeof(zlibinflate_t));
    memset(z, 0, sizeof(zlibinflate_t));
    int ret;
    memset(r, 0, sizeof(reader_t));
    r->error = NULL;
    r->internal = z;
    r->read = reader_zlibinflate;
    r->seek = reader_zlibseek;
    r->dealloc = reader_zlibinflate_dealloc;
    r->type = READER_TYPE_ZLIB;
    r->pos = 0;
    z->input = input;
    memset(&z->zs,0,sizeof(z_stream));
    z->zs.zalloc = Z_NULL;
    z->zs.zfree  = Z_NULL;
    z->zs.opaque = Z_NULL;
    ret = inflateInit(&z->zs);
    if (ret != Z_OK) zlib_error(ret, "bitio:inflate_init", &z->zs);
    reader_resetbits(r);
#else
    fprintf(stderr, "Error: compiled without zlib support");
    exit(1);
#endif
    return r;
}

/* ---------------------------- zlibdeflate writer -------------------------- */

typedef struct _zlibdeflate
{
#ifdef HAVE_ZLIB
    z_stream zs;
    writer_t*output;
    unsigned char writebuffer[ZLIB_BUFFER_SIZE];
#endif
} zlibdeflate_t;

static int writer_zlibdeflate_write(writer_t*writer, void* data, int len) 
{
#ifdef HAVE_ZLIB
    zlibdeflate_t*z = (zlibdeflate_t*)writer->internal;
    int ret;
    if(writer->type != WRITER_TYPE_ZLIB) {
        fprintf(stderr, "Wrong writer ID (writer not initialized?)\n");
        return 0;
    }
    if(!z) {
        fprintf(stderr, "zlib not initialized!\n");
        return 0;
    }
    if(!len)
        return 0;
    
    z->zs.next_in = (Bytef *)data;
    z->zs.avail_in = len;

    while(1) {
        ret = deflate(&z->zs, Z_NO_FLUSH);
        
        if (ret != Z_OK) zlib_error(ret, "bitio:deflate_deflate", &z->zs);

        if(z->zs.next_out != z->writebuffer) {
            writer->pos += z->zs.next_out - (Bytef*)z->writebuffer;
            z->output->write(z->output, z->writebuffer, z->zs.next_out - (Bytef*)z->writebuffer);
            z->zs.next_out = z->writebuffer;
            z->zs.avail_out = ZLIB_BUFFER_SIZE;
        }

        if(!z->zs.avail_in) {
            break;
        }
    }
    return len;
#else
    fprintf(stderr, "Error: Compiled without zlib support");
    exit(1);
#endif
}

void writer_zlibdeflate_flush(writer_t*writer)
{
#ifdef HAVE_ZLIB
    zlibdeflate_t*z = (zlibdeflate_t*)writer->internal;
    int ret;
    if(writer->type != WRITER_TYPE_ZLIB) {
        fprintf(stderr, "Wrong writer ID (writer not initialized?)\n");
        return;
    }
    if(!z) {
        fprintf(stderr, "zlib not initialized!\n");
        return;
    }

    z->zs.next_in = 0;
    z->zs.avail_in = 0;
    while(1) {
        ret = deflate(&z->zs, Z_SYNC_FLUSH);
        if (ret != Z_OK) zlib_error(ret, "bitio:deflate_flush", &z->zs);
        if(z->zs.next_out != z->writebuffer) {
            writer->pos += z->zs.next_out - (Bytef*)z->writebuffer;
            z->output->write(z->output, z->writebuffer, z->zs.next_out - (Bytef*)z->writebuffer);
            z->zs.next_out = z->writebuffer;
            z->zs.avail_out = ZLIB_BUFFER_SIZE;
        } 
        /* TODO: how will zlib let us know it needs more buffer space? */
        break;
    }
    return;
#else
    fprintf(stderr, "Error: Compiled without zlib support");
    exit(1);
#endif
}

static void writer_zlibdeflate_finish(writer_t*writer)
{
#ifdef HAVE_ZLIB
    zlibdeflate_t*z = (zlibdeflate_t*)writer->internal;
    writer_t*output;
    int ret;
    if(writer->type != WRITER_TYPE_ZLIB) {
        fprintf(stderr, "Wrong writer ID (writer not initialized?)\n");
        return;
    }
    if(!z)
        return;
    output= z->output;
    while(1) {
        ret = deflate(&z->zs, Z_FINISH);
        if (ret != Z_OK &&
            ret != Z_STREAM_END) zlib_error(ret, "bitio:deflate_finish", &z->zs);

        if(z->zs.next_out != z->writebuffer) {
            writer->pos += z->zs.next_out - (Bytef*)z->writebuffer;
            z->output->write(z->output, z->writebuffer, z->zs.next_out - (Bytef*)z->writebuffer);
            z->zs.next_out = z->writebuffer;
            z->zs.avail_out = ZLIB_BUFFER_SIZE;
        }

        if (ret == Z_STREAM_END) {
            break;

        }
    }
    ret = deflateEnd(&z->zs);
    if (ret != Z_OK) zlib_error(ret, "bitio:deflate_end", &z->zs);
    free(writer->internal);
    free(writer);
    //output->finish(output); 
#else
    fprintf(stderr, "Error: Compiled without zlib support");
    exit(1);
#endif
}
writer_t* zlibdeflatewriter_new(writer_t*output)
{
#ifdef HAVE_ZLIB
    writer_t*w = (writer_t*)malloc(sizeof(writer_t));
    zlibdeflate_t*z;
    int ret;
    memset(w, 0, sizeof(writer_t));
    z = (zlibdeflate_t*)malloc(sizeof(zlibdeflate_t));
    memset(z, 0, sizeof(zlibdeflate_t));
    w->internal = z;
    w->write = writer_zlibdeflate_write;
    w->flush = writer_zlibdeflate_flush;
    w->finish = writer_zlibdeflate_finish;
    w->type = WRITER_TYPE_ZLIB;
    w->pos = 0;
    z->output = output;
    memset(&z->zs,0,sizeof(z_stream));
    z->zs.zalloc = Z_NULL;
    z->zs.zfree  = Z_NULL;
    z->zs.opaque = Z_NULL;
    ret = deflateInit(&z->zs, 9);
    if (ret != Z_OK) zlib_error(ret, "bitio:deflate_init", &z->zs);
    w->bitpos = 0;
    w->mybyte = 0;
    z->zs.next_out = z->writebuffer;
    z->zs.avail_out = ZLIB_BUFFER_SIZE;
    return w;
#else
    fprintf(stderr, "Error: No zlib support compiled in");
    exit(1);
#endif
}

/* ----------------------- bit handling routines -------------------------- */

void writer_writebit(writer_t*w, int bit)
{    
    if(w->bitpos==8) 
    {
        w->write(w, &w->mybyte, 1);
        w->bitpos = 0;
        w->mybyte = 0;
    }
    if(bit&1)
        w->mybyte |= 1 << (7 - w->bitpos);
    w->bitpos ++;
}
void writer_writebits(writer_t*w, unsigned int data, int bits)
{
    int t;
    for(t=0;t<bits;t++)
    {
        writer_writebit(w, (data >> (bits-t-1))&1);
    }
}
void writer_resetbits(writer_t*w)
{
    if(w->bitpos)
        w->write(w, &w->mybyte, 1);
    w->bitpos = 0;
    w->mybyte = 0;
}
 
unsigned int reader_readbit(reader_t*r)
{
    if(r->bitpos==8) 
    {
        r->bitpos=0;
        r->read(r, &r->mybyte, 1);
    }
    return (r->mybyte>>(7-r->bitpos++))&1;
}
unsigned int reader_readbits(reader_t*r, int num)
{
    int t;
    int val = 0;
    for(t=0;t<num;t++)
    {
        val<<=1;
        val|=reader_readbit(r);
    }
    return val;
}
void reader_resetbits(reader_t*r)
{
    r->mybyte = 0;
    r->bitpos = 8;

}

uint8_t read_uint8(reader_t*r)
{
    uint8_t b = 0;
    int ret = r->read(r, &b, 1);
    if(ret<1) {
        return 0;
    }
    return b;
}
int8_t read_int8(reader_t*r)
{
    int8_t b = 0;
    int ret = r->read(r, &b, 1);
    if(ret<1) {
        return 0;
    }
    return b;
}
uint16_t read_uint16(reader_t*r)
{
    uint8_t b1=0,b2=0;
    int ret = r->read(r, &b1, 1);
    if(ret<1) {
        return 0;
    }
    ret = r->read(r, &b2, 1);
    if(ret<1) {
        return 0;
    }
    return b1|b2<<8;
}
uint32_t read_uint32(reader_t*r)
{
    uint8_t b1=0,b2=0,b3=0,b4=0;
    if(r->read(r, &b1, 1)<1)
        return 0;
    if(r->read(r, &b2, 1)<1)
        return 0;
    if(r->read(r, &b3, 1)<1)
        return 0;
    if(r->read(r, &b4, 1)<1)
        return 0;
    return b1|b2<<8|b3<<16|b4<<24;
}
uint32_t read_compressed_uint(reader_t*r)
{
    uint32_t u = 0;
    uint32_t b;
    do {
        b = read_uint8(r);
        u = u<<7|b&0x7f;
    } while(b&0x80);
    return u;
}
int32_t read_compressed_int(reader_t*r)
{
    int32_t i = 0;
    int32_t b;

    b = read_int8(r);
    i = b&0x7f;

    if(b&0x40)
        i|=0xffffff80; //sign extension

    if(!(b&0x80))
        return i;

    do {
        b = read_int8(r);
        i = i<<7|b&0x7f;
    } while(b&0x80);

    return i;
}

float read_float(reader_t*r)
{
    float f;
    r->read(r, &f, 4);
    return f;

    uint16_t b1=0,b2=0,b3=0,b4=0;
    r->read(r, &b1, 1);
    r->read(r, &b2, 1);
    r->read(r, &b3, 1);
    r->read(r, &b4, 1);
    uint32_t w = (b1|b2<<8|b3<<16|b4<<24);
    return *(float*)&w;
}
double read_double(reader_t*r)
{
    double f;
    r->read(r, &f, 8);
    return f;
}
char*read_string(reader_t*r)
{
    writer_t*g = growingmemwriter_new(16);
    while(1) {
        uint8_t b = 0;
        int ret = r->read(r, &b, 1);
        if(ret<=0) {
            g->finish(g);
            return strdup("");
        }
        write_uint8(g, b);
        if(!b)
            break;
    }
    char*string = (char*)writer_growmemwrite_getmem(g, 0);
    g->finish(g);
    return string;
}

void write_string(writer_t*w, const char*s)
{
    char zero = 0;
    if(!s || !*s) {
        w->write(w, &zero, 1);
    } else {
        int l = strlen(s);
        w->write(w, (void*)s, l);
        w->write(w, &zero, 1);
    }
}
void write_uint8(writer_t*w, uint8_t b)
{
    w->write(w, &b, 1);
}
void write_uint16(writer_t*w, uint16_t v)
{
    unsigned char b1 = v;
    unsigned char b2 = v>>8;
    w->write(w, &b1, 1);
    w->write(w, &b2, 1);
}
void write_uint32(writer_t*w, uint32_t v)
{
    unsigned char b1 = v;
    unsigned char b2 = v>>8;
    unsigned char b3 = v>>16;
    unsigned char b4 = v>>24;
    w->write(w, &b1, 1);
    w->write(w, &b2, 1);
    w->write(w, &b3, 1);
    w->write(w, &b4, 1);
}
void write_float(writer_t*w, float f)
{
    w->write(w, &f, 4);
    return;
}
void write_double(writer_t*w, double f)
{
    w->write(w, &f, 8);
    return;
}
void write_compressed_uint(writer_t*w, uint32_t u)
{
    if(u<0x80) {
        write_uint8(w, u);
    } else if(u<0x4000) {
        write_uint8(w, u>>7|0x80);
        write_uint8(w, u&0x7f);
    } else if(u<0x200000) {
        write_uint8(w, u>>14|0x80);
        write_uint8(w, u>>7|0x80);
        write_uint8(w, u&0x7f);
    } else if(u<0x10000000) {
        write_uint8(w, u>>21|0x80);
        write_uint8(w, u>>14|0x80);
        write_uint8(w, u>>7|0x80);
        write_uint8(w, u&0x7f);
    } else {
        write_uint8(w, u>>28|0x80);
        write_uint8(w, u>>21|0x80);
        write_uint8(w, u>>14|0x80);
        write_uint8(w, u>>7|0x80);
        write_uint8(w, u&0x7f);
    }
}
void write_compressed_int(writer_t*w, int32_t i)
{
    if(i>=-0x40 && i<0x40) {
        write_uint8(w, i&0x7f);
    } else if(i>=-0x2000 && i<0x2000) {
        write_uint8(w, (i>>7)&0x7f|0x80);
        write_uint8(w, i&0x7f);
    } else if(i>=-0x100000 && i<0x100000) {
        write_uint8(w, (i>>14)&0x7f|0x80);
        write_uint8(w, (i>>7)&0x7f|0x80);
        write_uint8(w, (i)&0x7f);
    } else if(i>=-0x8000000 && i<0x8000000) {
        write_uint8(w, (i>>21)&0x7f|0x80);
        write_uint8(w, (i>>14)&0x7f|0x80);
        write_uint8(w, (i>>7)&0x7f|0x80);
        write_uint8(w, (i)&0x7f);
    } else {
        write_uint8(w, (i>>28)&0x7f|0x80);
        write_uint8(w, (i>>21)&0x7f|0x80);
        write_uint8(w, (i>>14)&0x7f|0x80);
        write_uint8(w, (i>>7)&0x7f|0x80);
        write_uint8(w, (i)&0x7f);
    }
}
