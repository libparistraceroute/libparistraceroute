#include <stdlib.h>
#include <stdio.h> //DEBUG
#include <string.h>
#include "buffer.h"

inline buffer_t * buffer_create() {
    buffer_t *buffer;
    buffer = malloc(sizeof(buffer_t));
    if (!buffer)
        goto error;

    buffer->data = NULL;
    buffer->size = 0;
    return buffer;

error:
    return NULL;
}

buffer_t * buffer_dup(buffer_t * buffer)
{
    buffer_t *buf;

    if (!buffer)
        return NULL;

    buf = buffer_create();
    if (!buf)
        goto error;

    buf->data = calloc(buffer->size, sizeof(unsigned char));
    if (!buf->data)
        goto error_buffer;

    memcpy(buf->data, buffer->data, buffer->size);
    buf->size = buffer->size;

    return buf;

error_buffer:
    free(buffer);
error:
    return NULL;
}

void buffer_free(buffer_t * buffer)
{
    if (buffer) {
        if (buffer->data) free(buffer->data);
        free(buffer);
    }
}

int buffer_resize(buffer_t *buffer, size_t size)
{
    unsigned char *tmp;

    if (buffer->size == size)
        return 0;

    if (!buffer->data) {
        // First time allocation
        buffer->data = calloc(size, sizeof(unsigned char));
        if (!buffer->data)
            return -1; // no allocation could be made
    } else {
        tmp = realloc(buffer->data, size * sizeof(unsigned char));
        if (!tmp)
            return -1; // cannot realloc, orig still valid
        buffer->data = tmp;
    }
    buffer->size = size;
    return 0;
}

inline unsigned char * buffer_get_data(buffer_t *buffer) {
    return buffer->data;
}

inline size_t buffer_get_size(const buffer_t *buffer) {
    return buffer->size;
}

inline void buffer_set_data(buffer_t *buffer, unsigned char *data, unsigned int size)
{
    buffer_resize(buffer, size);
    memcpy(buffer->data, data, size);
}

inline void buffer_set_size(buffer_t *buffer, size_t size) {
    buffer->size = size;
}

// Dump

void buffer_dump(const buffer_t * buffer)
{
    unsigned int i;
    char c;

    /* print data byte by byte */
    for (i = 0; i < buffer->size; i++)
    {
        c = buffer->data[i];
        //if (c < ' ' && c != '\n' && c != '\t')
        //    printf(".");
        //else
            printf("%2x ", c);
    }
}

