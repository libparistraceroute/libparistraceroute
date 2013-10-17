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

struct layer_s;

/**
 * \struct protocol_t
 * \brief Structure describing a protocol
 */

typedef struct protocol_s {
    const char * name; /**< Name of the protocol */
    
    /**
     * Identifier of the protocol :
     * http://en.wikipedia.org/wiki/List_of_IP_protocol_numbers
     */

    uint8_t protocol;
    
    /**
     * Pointer to a function that will return the number of fields this protocol has
     */

    //size_t (*get_num_fields)(void);
    
    /**
     * \brief Points to a callback which updates the checksum of the segment related to
     *    this protocol.
     * \param buf Pointer to the protocol's segment
     * \param psh Pointer to the corresponding pseudo header (pass NULL if not needed)
     * \return true if success, false otherwise 
     */

    bool (*write_checksum)(uint8_t * buf, buffer_t * psh);

    /**
     * \brief Points to a callback which creates a buffer_t instance
     *    containing the pseudo header needed to compute the checksum
     *    of a segment of this protocol.
     * \param segment The address of the segment. For instance if you compute
     *    the UDP of an IPv6/UDP packet, pass the address of the IPv6 segment.
     * \return The corresponding buffer, NULL in case of failure.
     */

    buffer_t * (*create_pseudo_header)(const uint8_t * segment);
    
    /**
     * Pointer to a protocol_field_t structure holding the header fields
     */

    protocol_field_t * fields;
    
    /** 
     * \brief Points to a callback which writes the default header
     *   into a pre-allocated buffer. 
     * \param header The target buffer. You may pass NULL if you want
     *   to retrieve the size (in bytes) of the default header
     * \return The size of the default header.
     */

    size_t (*write_default_header)(uint8_t * header);

    /**
     * \brief Points to a callback which returns the size of a header.
     * \param A pointer to the header we want to get size. You may pass
     *    NULL to retrieve the default size or if the size of the header
     *    is always equal to the same value.
     * \return The header size
     */

    size_t (*get_header_size)(const uint8_t * header);

    /**
     * \brief Set unset parts of a header to coherent values 
     * \param header The header that must be updated
     * \return true iif successful
     */

    bool (*finalize)(uint8_t * header);

    /**
     * \brief Pointer to a function that detects the version of the protocol
     * \param packet The evaluated packet.
     * \return true iif the packet if using this protocol.
     */

    bool (*instance_of)(uint8_t * packet);

    /**
     * \brief Retrieve the protocol of a nested layer using its "protocol" field.
     * \sa protocol_get_next_protocol
     * \param layer The nesting layer
     * \return The corresponding protocol if any and if supported, NULL otherwise.
     */

    const struct protocol_s * (*get_next_protocol)(const struct layer_s * layer);

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
 * \param data User data pass to the callback 
 * \param callback Pointer to a function call whenever a protocol_field_t
 *    of the protocol_t instance is visited. This callback receives a
 *    pointer to the current protocol_field_t and to data.
 */

void protocol_iter_fields(const protocol_t * protocol, void * data, void (*callback)(const protocol_field_t * field, void * data));

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

/**
 * \brief Print information stored in a protocol instance
 * \param protocol A protocol_t instance
 */

void protocol_dump(const protocol_t * protocol);

/**
 * \brief Print every protocol managed by libparistraceroute 
 */

void protocols_dump(void);

/**
 * \brief Retrieve the protocol of a nested layer using its "protocol" field.
 * \param layer The nesting layer
 * \return The corresponding protocol if any and if supported, NULL otherwise.
 */

const protocol_t * protocol_get_next_protocol(const struct layer_s * layer);


#define PROTOCOL_REGISTER(MOD)    \
static void __init_ ## MOD (void) __attribute__ ((constructor));    \
static void __init_ ## MOD (void) {    \
    protocol_register(&MOD); \
}

#endif
