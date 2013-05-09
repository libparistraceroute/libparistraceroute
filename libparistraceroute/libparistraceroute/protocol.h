#ifndef PROTOCOL_H
#define PROTOCOL_H

/**
 * \file protocol.h
 * \brief Header for a protocol
 */

#include <stdbool.h>

#include "protocol_field.h"
#include "buffer.h"

#define END_PROTOCOL_FIELDS { .key = NULL }

/**
 * \struct protocol_t
 * \brief Structure describing a protocol
 */
typedef struct {
    const char * name; /**< Name of the protocol */
    
    /**
     * Identifier of the protocol :
     * http://en.wikipedia.org/wiki/List_of_IP_protocol_numbers
     */

    uint8_t protocol;
	
    /**
     * Pointer to a function that will return the number of fields this protocol has
     */

    size_t (*get_num_fields)(void);
	
    /**
     * Pointer to a function that will return true if an external checksum
     * is needed, false otherwise (?) 
     */

    bool need_ext_checksum;

	/**
     * Pointer to a function to write a checksum for a pseudoheader into a buffer
	 * \param buf Pointer to a buffer
	 * \param psh Pointer to a pseudoheader_t structure
	 * \return true if success, false othewise 
	 */

	bool (*write_checksum)(unsigned char * buf, buffer_t * psh);

    // create_pseudo_header
    buffer_t * (*create_pseudo_header)(unsigned char * buffer);
	
    /**
     * Pointer to a protocol_field_t structure holding the header fields
     */
    protocol_field_t * fields;
	
    /** 
	 * \brief Pointer to a function which writes the default header into data
     * \param data Pointer to a pre-allocated header 
	 */

    void (*write_default_header)(unsigned char * data);

    size_t header_len;
    
    // socket_type
	
    /**
     * Pointer to a function that returns the size of the protocol header
     */

    size_t (*get_header_size)(void);
    
    /**
     * Pointer to a function that allows the protocol to do further processing
     * before the packet checksum is computed, and the packet is sent
     */

    int (*finalize)(uint8_t * packet);

    /**
     * Pointer to a function that detects the version of the protocol
     */
    bool (*instance_of)(uint8_t * packet);
} protocol_t;

/**
 * \brief Search a registered protocol in the library according to its name
 * \param name The name of the protocol (for example "udp", "ipv4", etc.)
 * \return A pointer to the corresponding protocol if any, NULL othewise
 */

const protocol_t * protocol_search(const char * name);

/**
 * \brief Search a registered protocol in the library according to its ID 
 * \param name The ID of the protocol (for example 17 corrresponds to UDP)
 * \return A pointer to the corresponding protocol if any, NULL othewise
 */

const protocol_t * protocol_search_by_id(uint8_t id);

/**
 * \brief Register a protocol
 * \param protocol Pointer to a protocol_t structure describing the protocol to register
 * \return None
 */

void protocol_register(protocol_t *protocol);

/**
 * \brief Apply a function to every field in a protocol
 * \param protocol Pointer to a protocol_t structure describing the
 *    protocol to iterate over
 * \param data Pointer to hold the returned data (?)
 * \param callback Pointer to a callback function that takes the data
 *    from a protocol_field_t structure (field) and sets the pointer
 *    'data' to the data from the field
 */

void protocol_iter_fields(protocol_t * protocol, void * data, void (*callback)(protocol_field_t * field, void * data));

/**
 * \brief Retrieve a field belonging to a protocol according to its name
 * \param name The field name
 * \param protocol The queried network protocol
 * \return A pointer to the corresponding protocol_field_t instance if any, NULL otherwise
 */
const protocol_field_t * protocol_get_field(const protocol_t * protocol, const char * name);

/**
 * \brief Calculate an Internet checksum.
 * \param bytes Bytes used to compute the checksum 
 * \param size Number of bytes to consider
 * \return The corresponding checksum
 */

uint16_t csum(const uint16_t * buf, size_t size);

#define PROTOCOL_REGISTER(MOD)	\
static void __init_ ## MOD (void) __attribute__ ((constructor));	\
static void __init_ ## MOD (void) {	\
	protocol_register(&MOD); \
}

#endif
