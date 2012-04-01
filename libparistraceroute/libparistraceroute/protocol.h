#ifndef PROTOCOL_H
#define PROTOCOL_H

/**
 * \file protocol.h
 * \brief Header for a protocol
 */

#include <stdbool.h>

#include "protocol_field.h"
#include "pseudoheader.h"

//#define CAST_WRITE_CHECKSUM (int (*)(unsigned char *, pseudoheader_t *))
#define END_PROTOCOL_FIELDS { .key = NULL }

/**
 * \struct protocol_t
 * \brief Structure describing a protocol
 */
typedef struct {
	/** Name of the protocol */
    char* name;
    /** Identifier of the protocol :
     * http://en.wikipedia.org/wiki/List_of_IP_protocol_numbers
     */
    unsigned int protocol;
	/** Pointer to a function that will return the number of fields this protocol has */
    unsigned int (*get_num_fields)(void);
	/** Pointer to a function that will return true if an external checksum is needed, false otherwise (?) */
    bool need_ext_checksum;
	/** Pointer to a function to write a checksum for a pseudoheader into a buffer
	 * \param buf Pointer to a buffer
	 * \param psh Pointer to a pseudoheader_t structure
	 * \return int
	 */
	int (*write_checksum)(unsigned char *buf, pseudoheader_t *psh);
    // create_pseudo_header
	/** Pointer to a protocol_field_t structure holding the header fields */
    protocol_field_t *fields;
	/** Pointer to a function to create a default header with the given data (?)
	 * \param data Pointer to an array of bytes
	 */
    void (*write_default_header)(unsigned char *data);
    size_t header_len;
    //socket_type
	/** Pointer to a function that returns the size of the protocol header */
    unsigned int (*get_header_size)(void);
    /** Pointer to a function that allows the protocol to do further processing
     * before the packet checksum is computed, and the packet is sent */
    int (*finalize)(unsigned char *buffer);
} protocol_t;

/**
 * \brief Search for a protocol by name
 * \param name String containing the name to search for
 * \return Pointer to a protocol_t structure containing the desired protocol or NULL if not found
 */
protocol_t* protocol_search(char *name);
/**
 * \brief Register a protocol
 * \param protocol Pointer to a protocol_t structure describing the protocol to register
 * \return None
 */
void protocol_register(protocol_t *protocol);

/**
 * \brief Apply a function to every field in a protocol
 * \param protocol Pointer to a protocol_t structure describing the protocol to iterate over
 * \param data Pointer to hold the returned data (?)
 * \param callback Pointer to a callback function that takes the data from a protocol_field_t structure (field) and sets the pointer 'data' to the data from the field
 * \return None
 */
void protocol_iter_fields(protocol_t *protocol, void *data, void (*callback)(protocol_field_t *field, void *data));

protocol_field_t * protocol_get_field(protocol_t *protocol, const char *name);

/**
 * function to calc checksums (may be multi protocol)
 * @param len the size of header in bytes 
 * @param buff the buffer
 * @return the checksum
 */
unsigned short csum (unsigned short *buf, int nwords);

#define PROTOCOL_REGISTER(MOD)	\
static void __init_ ## MOD (void) __attribute__ ((constructor));	\
static void __init_ ## MOD (void) {	\
	protocol_register(&MOD); \
}

#endif
