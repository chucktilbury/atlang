/*
    This module implements a generic character buffer that grows as content is
    added to it. It is intended to the transient storage for strings.
*/
#include "common.h"
#include "chbuffer.h"

typedef struct {
    size_t length;
    size_t capacity;
    char* buffer;
} __chbuf_t;

static void grow_buffer(__chbuf_t* buf, size_t size) {

    if(buf->length+size+2 > buf->capacity) {
        buf->capacity = buf->capacity << 1;
        buf->buffer = REALLOC(buf->buffer, buf->capacity);
    }
}

void init_char_buffer(char_buffer_t chbuf) {

    __chbuf_t* buf = (__chbuf_t*)chbuf;
    buf->length = 0;
    buf->capacity = 0x01 << 3;
    if(buf->buffer != NULL)
        FREE(buf->buffer);
    buf->buffer = MALLOC(buf->capacity);
    buf->buffer[0] = 0;
}

char_buffer_t create_char_buffer() {

    __chbuf_t* buf = ALLOC_DS(__chbuf_t);
    init_char_buffer(buf);
    return (char_buffer_t)buf;
}

void destroy_char_buffer(char_buffer_t chbuf) {

    if(chbuf != NULL) {
        __chbuf_t* buf = (__chbuf_t*)chbuf;
        if(buf->buffer != NULL)
            FREE(buf->buffer);
        FREE(buf);
    }
}

const char* get_char_buffer(char_buffer_t chbuf) {

    __chbuf_t* buf = (__chbuf_t*)chbuf;
    return buf->buffer;
}

// This will write any 8 bit value to the buffer.
void add_char_buffer(char_buffer_t chbuf, int ch) {

    __chbuf_t* buf = (__chbuf_t*)chbuf;

    grow_buffer(buf, 1);
    buf->buffer[buf->length] = ch;
    buf->length++;
    buf->buffer[buf->length] = 0;
}

void add_char_buffer_str(char_buffer_t chbuf, const char* str) {

    for(int i = 0; str[i] != 0; i ++)
        add_char_buffer(chbuf, str[i]);
}

// Find the minimum number of bytes that can represent this signed value and
// then write those binary (not ASCII) bytes to the buffer, up to 4 bytes.
void add_char_buffer_int(char_buffer_t chbuf, int val) {

    char tbuf[9]; // up to 4 bytes for 32 bit word
    char blist[4][3];
    int idx = 0;

    sprintf(tbuf, "%08X", (uint32_t)val);
    memset(blist, 0, sizeof(blist));
    for(int i = 0; i < 4; i++) {
        for(int j = 0; j < 2; j++) {
            blist[i][j] = tbuf[idx++];
        }
    }

    idx = 0;
    // skip leading zero bytes
    for(; idx < 4 && blist[idx][0] != 0; idx++)
        if(strcmp(blist[idx], "00"))
            break;

    if(idx >= 4) {
        // all byte values are zero
        add_char_buffer(chbuf, (int)0);
    }
    else {
        // write the remaining bytes
        for(; idx < 4 && blist[idx][0] != 0; idx++)
            add_char_buffer(chbuf, (int)strtol(blist[idx], NULL, 16));
    }
}

// Truncate the string at the index given by placing a zero there and fixing
// the length. If the index is greater than length, then silently fail. Does
// not change the capacity.
void truncate_char_buffer(char_buffer_t chbuf, int index) {

    __chbuf_t* buf = (__chbuf_t*)chbuf;
    if((size_t)index < buf->length) {
        buf->length = index;
        buf->buffer[index] = 0;
    }
}

// Copy the given string to the buffer, starting at the index given. If the buffer
// needs to grow as a result, then grow it.
void set_char_buffer_index_str(char_buffer_t chbuf, int index, const char* str) {

    truncate_char_buffer(chbuf, index);
    add_char_buffer_str(chbuf, str);
}
