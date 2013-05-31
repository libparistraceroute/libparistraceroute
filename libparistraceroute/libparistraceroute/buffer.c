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

    if (!buffer)                                goto ERR_INVALID_PARAMETER;
    if (!(ret = buffer_create()))               goto ERR_BUFFER_CREATE;
    if (!(ret->data = calloc(1, buffer->size))) goto ERR_BUFFER_DATA;

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
        if (buffer->data) {
           // printf("buffer = %x: free buffer->data = %x\n", buffer, buffer->data);
            free(buffer->data);
        }
        free(buffer);
    }
}

bool buffer_resize(buffer_t * buffer, size_t size)
{
    uint8_t * data2;
    bool      ret = true;
    size_t    old_size = buffer->size;

    if (old_size != size) {
        if (buffer->data) {
            data2 = realloc(buffer->data, size * sizeof(uint8_t));
            if (data2 && size > old_size) {
                memset(data2 + old_size, 0, size - old_size); 
            }
        } else {
            data2 = calloc(size, sizeof(uint8_t));
        }
        if (data2) {
            buffer->data = data2;
            buffer->size = size;
        }
        ret = (data2 != NULL);
    }
    return ret;
}

inline uint8_t * buffer_get_data(const buffer_t * buffer) {
    return buffer ? buffer->data : 0;
}

inline size_t buffer_get_size(const buffer_t * buffer) {
    return buffer ? buffer->size : 0;
}

bool buffer_write_bytes(buffer_t * buffer, const void * bytes, size_t num_bytes) {
    bool ret = buffer_resize(buffer, num_bytes);
    if (ret) memcpy(buffer->data, bytes, num_bytes);
    return ret;
}

inline void buffer_set_size(buffer_t * buffer, size_t size) {
    buffer->size = size;
}

// Dump

void buffer_dump(const buffer_t * buffer) {
    size_t i, n = buffer->size;

    for (i = 0; i < n; i++) {
        printf("%2x ", buffer->data[i]);
    }
}





