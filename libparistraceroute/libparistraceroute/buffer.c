#include <stdlib.h>
#include <string.h>
#include "buffer.h"

buffer_t *buffer_create(void)
{
    buffer_t * buffer;

    buffer = malloc(sizeof(buffer_t));
    buffer->data = NULL;
    buffer->size = 0;
    buffer->max_size = 0;

    return buffer;
}

void buffer_free(buffer_t * buffer)
{
    if (buffer->data)
        free(buffer->data);
    free(buffer);
    buffer = NULL;
}


