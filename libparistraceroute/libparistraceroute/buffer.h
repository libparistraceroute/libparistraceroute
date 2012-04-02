#ifndef BUFFER_H
#define BUFFER_H

/**
 * \struct buffer_t
 * \brief A buffer structure.
 */
typedef struct {
    unsigned char *data;
    size_t size;
    size_t max_size;
} buffer_t;

/**
 * \brief Create a buffer structure
 * \return Pointer to the newly created buffer structure
 */
buffer_t *buffer_create(void);

/**
 * \brief Free a buffer structure.
 * \param buffer Pointer to the buffer structure to free
 */
void buffer_free(buffer_t *buffer);

// Accessors

unsigned char * buffer_get_data(buffer_t *buffer);
size_t buffer_get_size(buffer_t *buffer);

/**
 * \brief (Re)allocate the buffer to a specified size.
 * \param buffer Pointer to a buffer_t structure to (re)allocate
 * \param size new size of the buffer
 * \return 0 if success, -1 otherwise
 */
int buffer_resize(buffer_t *buffer, size_t size);

void buffer_set_data(buffer_t *buffer, unsigned char *data);
void buffer_set_size(buffer_t *buffer, size_t size);

// Dump
void buffer_dump(buffer_t *buffer);

#endif
