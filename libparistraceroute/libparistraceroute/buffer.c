#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>            // ERRNO, EINVAL

#include "buffer.h"

buffer_t * buffer_create() {
    buffer_t * buffer;

    if ((buffer = malloc(sizeof(buffer_t)))) {
        buffer->data = NULL;
        buffer->size = 0;
    }
    return buffer;
}

buffer_t * buffer_dup(const buffer_t * buffer)
{
    buffer_t * ret;

    if (!buffer) goto ERR_INVALID_PARAMETER;
    if (!(ret = buffer_create())) goto ERR_BUFFER_CREATE;
    if (!(ret->data = calloc(buffer->size, sizeof(uint8_t)))) goto ERR_BUFFER_DATA;

    memcpy(ret->data, buffer->data, buffer->size);
    ret->size = buffer->size;
    return ret;

ERR_BUFFER_DATA:
    free(ret);
ERR_BUFFER_CREATE:
ERR_INVALID_PARAMETER:
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

inline uint8_t * buffer_get_data(const buffer_t * buffer) {
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

uint8_t buffer_guess_ip_version(buffer_t * buffer) {
    return buffer->data[0] >> 4;
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





