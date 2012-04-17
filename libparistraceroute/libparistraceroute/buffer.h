#ifndef BUFFER_H
#define BUFFER_H

/**
 * \struct buffer_t
 * \brief A buffer structure.
 */

typedef struct {
    unsigned char * data;     /**< Data stored in the buffer   */
    size_t          size;     /**< Size of the data (in bytes) */
    size_t          max_size; /**< Size available (in bytes)   */
} buffer_t;

//-----------------------------------------------------------------
// Allocation
//-----------------------------------------------------------------


/**
 * \brief Create a buffer structure
 * \return Pointer to the newly created buffer structure
 */

buffer_t * buffer_create(void);

/**
 * \brief Free a buffer structure.
 * \param buffer Pointer to the buffer structure to free
 */

void buffer_free(buffer_t *buffer);

//-----------------------------------------------------------------
// Accessors
//-----------------------------------------------------------------

/**
 * \brief Retrieve the address where data is stored
 * \param buffer A pointer to the buffer
 * \return The address of the data
 */

unsigned char * buffer_get_data(buffer_t * buffer);

/**
 * \brief Retrieve the size of the allocated memory area
 * managed by the buffer
 * \param buffer The address of the buffer
 * \return The size (in bytes)
 */

size_t buffer_get_size(const buffer_t * buffer);

/**
 * \brief (Re)allocate the buffer to a specified size.
 * \param buffer Pointer to a buffer_t structure to (re)allocate
 * \param size new size of the buffer
 * \return 0 if success, -1 otherwise
 */

int buffer_resize(buffer_t * buffer, size_t size);

/**
 * \brief Change the address of the memory managed by the buffer.
 *   The old address but be freed before if not more used.
 * \param buffer The address of the buffer
 */

void buffer_set_data(buffer_t * buffer, unsigned char * data);

/**
 * \brief Alter
 * \param buffer The address of the buffer
 */

void buffer_set_size(buffer_t * buffer, size_t size);

//-----------------------------------------------------------------

/**
 * \brief Print the buffer content on the standard output
 * \param buffer The address of the buffer
 */

void buffer_dump(const buffer_t * buffer);

#endif
