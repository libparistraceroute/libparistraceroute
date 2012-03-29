#ifndef BUFFER_H
#define BUFFER_H

/**
 * \struct buffer_t
 * \brief A buffer structure.
 */
typedef struct {
    unsigned char *data;
    unsigned int size;
    unsigned int max_size;
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

#endif
