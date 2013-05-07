#ifndef PROBE_H
#define PROBE_H

/**
 * \file probe.h
 * \brief Header for probes
 */

#include <stdbool.h>
#include "stackedlist.h"
#include "field.h"
#include "layer.h"
#include "buffer.h"
#include "bitfield.h"
#include "dynarray.h"

#define WAITING_RESP 0
#define TTL_RESP 1
#define NO_RESP 2
#define END_RESP 3
#define OTHER_RESP 4

/**
 * \struct probe_t
 * \brief Structure representing a probe
 */
typedef struct {
    dynarray_t * layers;        /**< List of layers forming the packet header */
    buffer_t   * buffer;        /**< Buffer that will store probe fields content */
    bitfield_t * bitfield;      /**< Bitfield to keep track of modified fields (bits set to 1) vs. default ones (bits set to 0) */
    void       * caller;        /**< Algorithm instance which has created this probe */
    double       sending_time;  /**< Timestamp */
    double       queueing_time; /**< Timestamp */
} probe_t;

/**
 * \brief Create a probe
 * \return A pointer to a probe_t structure containing the probe
 */

probe_t * probe_create(void);

/**
 * \brief Duplicate a probe from probe skeleton
 * \return A pointer to a probe_t structure containing the probe
 */

probe_t * probe_dup(probe_t * probe_skel);

/**
 * \brief Free a probe
 * \param probe A pointer to a probe_t structure containing the probe
 */

void probe_free(probe_t * probe);

// Accessors

buffer_t * probe_get_buffer(const probe_t * probe);

/**
 * \brief Update a probe (including its forming layers) according to a buffer. 
 * \param probe A pointer to a preallocated probe that we're modifying
 * \param buffer The buffer we're assigning to this probe. This buffer
 *    is not duplicated.
 * \return true iif successful
 */

bool probe_set_buffer(probe_t * probe, buffer_t * buffer);

/**
 * \brief Print probe contents
 * \param probe The probe_t instance to print
 */

void probe_dump(const probe_t * probe);

/**
 * \brief Add a field to a probe
 * \param probe A pointer to a probe_t structure containing the probe
 * \param field A pointer to a field_t structure containing the field
 */

// TODO should return bool
void probe_add_field(probe_t * probe, field_t * field);

/**
 * \brief Set a field according to a given field name. 
 * \param probe The probe from which we're retrieving a field
 * \param name The name of the queried field
 * \param depth The index of the first layer from which the field
 *    can be set. 0 corresponds to the first layer.
 * \return true iif successfull  
 */

bool probe_set_field_ext(probe_t * probe, field_t * field, size_t depth);

bool probe_set_field(probe_t * probe, field_t * field);

/**
 * \brief Update 'length', 'checksum' and 'protocol' fields for each
 *   network protocol layer making the probe.
 * \return true iif successful
 */

bool probe_update_fields(probe_t * probe);

/**
 * \brief Assigns a set of fields to a probe
 * \param probe A pointer to a probe_t structure representing the probe
 * \param arg1 The first of a list of pointers to a field_t structure representing a field to add
 * \return 0 if successful,
 */

// TODO should return bool
int probe_set_fields(probe_t *probe, field_t *field1, ...);

/**
 * \brief Iterates through the fields in a probe, passing them as the argument to a callback function
 * \param probe A pointer to a probe_t structure representing the probe
 * \param data A pointer to hold the returned data from the callback (?)
 * \param callback A function pointer to a callback function
 * \return None
 */

void probe_iter_fields(probe_t *probe, void *data, void (*callback)(field_t *field, void *data));

/**
 * \brief Get an array of pointers to the fields of a probe
 * \param probe Pointer to a probe_t structure to get the fields from
 * \return NULL (NYI - Will be Array of pointers to field_t structures stored in the probe)
 */

field_t ** probe_get_fields(const probe_t * probe);

/**
 * \brief Retrieve a field according to a given field name. 
 * \param probe The probe from which we're retrieving a field
 * \param name The name of the queried field
 * \return A pointer to the corresponding field, NULL if not found 
 */

const field_t * probe_get_field(const probe_t * probe, const char * name);

/**
 * \brief Retrieve a field according to a given field name. 
 * \param probe The probe from which we're retrieving a field
 * \param name The name of the queried field
 * \param depth The index of the first layer from which the field
 *    can be extracted. 0 corresponds to the first layer.
 * \return A pointer to the corresponding field, NULL if not found
 */

const field_t * probe_get_field_ext(const probe_t * probe, const char * name, unsigned int depth);

/**
 * \brief Get the payload from a probe
 * \param probe Pointer to a probe_t structure to get the payload from
 * \return NULL (Not yet implemented)
 */

uint8_t * probe_get_payload(const probe_t * probe);

int probe_set_payload_size(probe_t * probe, unsigned int payload_size);

int probe_set_min_payload_size(probe_t * probe, unsigned int payload_size);

int probe_set_payload(probe_t *probe, buffer_t * payload);

int probe_write_payload(probe_t *probe, buffer_t * payload, unsigned int offset);

/**
 * \brief Get the size of the payload from a probe
 * \param probe Pointer to a probe_t structure to get the payload size from
 * \return 0 (NYI - Will be Size of the probe's payload)
 */

// TODO not implemented
size_t probe_get_payload_size(const probe_t * probe);

/**
 * \brief Get the protocol from the probe with the given index
 * \param i Integer index of the probe to use
 * \return NULL (NYI - Will be Pointer to the name of the protocol)
 */

// TODO not implemented
char * probe_get_protocol_by_index(unsigned int i);

/**
 * \brief Retrieve the number of layers (i.e. the number of layers related to a protocol + 1 (the payload)) 
 * \param probe Pointer to a probe_t structure to use
 * \return 0 (NYI)
 */

size_t probe_get_num_layers(const probe_t * probe);

/**
 * \brief Get the header fields from a probe
 * \param probe Pointer to a probe_t structure to use
 * \return NULL (NYI - Will be Array of pointers to the field_t structures contained in the given probe)
 */

field_t ** probe_get_fields(const probe_t * probe);

int probe_set_caller(probe_t * probe, void * caller);
void * probe_get_caller(const probe_t * probe);
int probe_set_sending_time(probe_t * probe, double time);
double probe_get_sending_time(const probe_t * probe);
int probe_set_queueing_time(probe_t * probe, double time);
double probe_get_queueing_time(const probe_t * probe);

/******************************************************************************
 * probe_reply_t
 ******************************************************************************/

typedef struct {
    probe_t * probe;
    probe_t * reply;
} probe_reply_t;

probe_reply_t * probe_reply_create(void);
void probe_reply_free(probe_reply_t * probe_reply);

// Accessors

int probe_reply_set_probe(probe_reply_t * probe_reply, probe_t * probe);
probe_t * probe_reply_get_probe(const probe_reply_t * probe_reply);
int probe_reply_set_reply(probe_reply_t *probe_reply, probe_t * reply);
probe_t * probe_reply_get_reply(const probe_reply_t * probe_reply);

/******************************************************************************
 * pt_loop_t
 ******************************************************************************/

struct pt_loop_s;

/**
 * \brief Callback function to handle the reply from a probe
 * \param loop Pointer to a pt_loop_s structure within which the probe is running (?)
 * \param probe Pointer to a probe_t structure containing the probe that was sent (?)
 * \param reply Pointer to an empty probe_t structure to hold the reply (?)
 * \return None
 */

void pt_probe_reply_callback(struct pt_loop_s * loop, probe_t * probe, probe_t * reply);


int probe_set_protocols(probe_t * probe, const char * name1, ...);

#endif
