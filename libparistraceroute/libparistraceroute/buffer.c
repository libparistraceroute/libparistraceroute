#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>            // ERRNO, EINVAL

#include "buffer.h"

buffer_t * buffer_create() {
    buffer_t * buffer;
    buffer = malloc(sizeof(buffer_t));
    if (buffer) {
        buffer->data = NULL;
        buffer->size = 0;
    } else errno = ENOMEM;
    return buffer;
}

buffer_t * buffer_dup(const buffer_t * buffer)
{
    buffer_t * buf;

    if (!buffer) return NULL;

    buf = buffer_create();
    if (!buf) goto ERROR;

    buf->data = calloc(buffer->size, sizeof(unsigned char));
    if (!buf->data) goto ERROR_BUFFER;

    memcpy(buf->data, buffer->data, buffer->size);
    buf->size = buffer->size;
    return buf;

ERROR_BUFFER:
    free(buf);
ERROR:
    return NULL;
}

void buffer_free(buffer_t * buffer)
{
    if (buffer) {
        if (buffer->data) free(buffer->data);
        free(buffer);
    }
}

bool buffer_resize(buffer_t * buffer, size_t size)
{
    /*
    unsigned char * tmp;

    if (buffer->size == size) return true;

    if (!buffer->data) {
        // First time allocation
        buffer->data = calloc(size, sizeof(unsigned char));
        if (!buffer->data) return false; // no allocation could be made
    } else {
        tmp = realloc(buffer->data, size * sizeof(unsigned char));
        if (!tmp) return -1; // cannot realloc, orig still valid
        buffer->data = tmp;
    }
    buffer->size = size;
    return 0;
    */
    uint8_t * data2;
    bool      ret = true;

    if (buffer->size != size) {
        data2 = buffer->data ?
            realloc(buffer->data, size * sizeof(uint8_t)):
            calloc(size, sizeof(uint8_t));
        if (data2) {
            buffer->data = data2;
            buffer->size = size;
        }
    }
    return ret;
}

inline uint8_t * buffer_get_data(buffer_t * buffer) {
    return buffer ? buffer->data : 0;
}

inline size_t buffer_get_size(const buffer_t * buffer) {
    return buffer ? buffer->size : 0;
}

bool buffer_set_data(buffer_t * buffer, uint8_t * data, size_t size)
{
    bool ret = buffer_resize(buffer, size);
    if (ret) memcpy(buffer->data, data, size);
    return ret;
}

inline void buffer_set_size(buffer_t * buffer, size_t size) {
    buffer->size = size;
}

// Dump

void buffer_dump(const buffer_t * buffer)
{
    size_t i;
    char   c;

    // Print data byte by byte
    for (i = 0; i < buffer->size; i++)
    {
        c = buffer->data[i];
        //if (c < ' ' && c != '\n' && c != '\t')
        //    printf(".");
        //else
            printf("%2x ", c);
    }
}

