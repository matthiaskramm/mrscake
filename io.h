/* io.h
   Generic object-oriented reading/writing.

   Part of the data prediction package.
   
   Copyright (c) 2001-2011 Matthias Kramm <kramm@quiss.org> 
 
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
#include <stdint.h>
#include "config.h"

#ifdef HAVE_ZZIP
#include "zzip/lib.h"
#endif

#ifndef __io_h__
#define __io_h__

#define READER_TYPE_FILE 1
#define READER_TYPE_MEM  2
#define READER_TYPE_ZLIB_U 3
#define READER_TYPE_ZLIB_C 4
#define READER_TYPE_ZLIB READER_TYPE_ZLIB_U
#define READER_TYPE_NULL 5
#define READER_TYPE_FILE2 6
#define READER_TYPE_ZZIP 7

#define WRITER_TYPE_FILE 1
#define WRITER_TYPE_MEM  2
#define WRITER_TYPE_ZLIB_C 3
#define WRITER_TYPE_ZLIB_U 4
#define WRITER_TYPE_NULL 5
#define WRITER_TYPE_GROWING_MEM  6
#define WRITER_TYPE_ZLIB WRITER_TYPE_ZLIB_C

typedef struct _reader
{
    int (*read)(struct _reader*, void*data, int len);
    int (*seek)(struct _reader*, int pos);
    void (*dealloc)(struct _reader*);

    void *internal;
    int type;
    unsigned char mybyte;
    unsigned char bitpos;
    int pos;

    char*error;
} reader_t;

typedef struct _writer
{
    int (*write)(struct _writer*, void*data, int len);
    void (*flush)(struct _writer*);
    void (*finish)(struct _writer*);

    void *internal;
    int type;
    unsigned char mybyte;
    unsigned char bitpos;
    int pos;
} writer_t;

void reader_resetbits(reader_t*r);
unsigned int reader_readbit(reader_t*r);
unsigned int reader_readbits(reader_t*r, int num);

uint8_t read_uint8(reader_t*r);
uint16_t read_uint16(reader_t*r);
uint32_t read_uint32(reader_t*r);
float read_float(reader_t*r);
double read_double(reader_t*r);
char*read_string(reader_t*r);

uint32_t read_compressed_uint(reader_t*r);

void writer_resetbits(writer_t*w);
void writer_writebit(writer_t*w, int bit);
void writer_writebits(writer_t*w, unsigned int data, int bits);

void write_uint8(writer_t*w, uint8_t b);
void write_uint16(writer_t*w, uint16_t v);
void write_uint32(writer_t*w, uint32_t v);
void write_float(writer_t*w, float f);
void write_double(writer_t*w, double f);
void write_string(writer_t*w, const char*s);

void write_compressed_uint(writer_t*w, uint32_t b);

/* standard readers / writers */

reader_t* filereader_new(int handle);
reader_t* filereader_new2(const char*filename);
reader_t* filereader_with_timeout_new(int handle, int seconds);
reader_t* zlibinflate_new(reader_t*input);
reader_t* memreader_new(void*data, int length);
reader_t* nullreader_new();
#ifdef HAVE_ZZIP
reader_t* zzipreader_new(ZZIP_FILE*z);
#endif

writer_t* filewriter_new(int handle);
writer_t* filewriter_new2(const char*filename);
writer_t* zlibdeflatewriter_new(writer_t*output);
writer_t* memwriter_new(void*data, int length);
writer_t* nullwriter_new();
writer_t* growingmemwriter_new();
writer_t* growingmemwriter_new2(uint32_t grow);

void* writer_growmemwrite_memptr(writer_t*w, int*len);
void* writer_growmemwrite_getmem(writer_t*w, int*len);
void writer_growmemwrite_reset(writer_t*w);
reader_t* growingmemwriter_getreader(writer_t*w);

#endif //__io_h__
