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
#include "packet.h"

/**
 * \struct probe_t
 * \brief Structure representing a probe
 * A probe is made of one or several layer.
 * The last layer is the payload (if any).
 * Otherwise, a layer is always related to a network protocol.
 * For instance a probe ipv4/udp/payload is made of 3 layers.
 */

typedef struct {
    dynarray_t * layers;        /**< List of layers forming the packet */
    buffer_t   * buffer;        /**< Buffer containing the packet we're crafting */ 
    bitfield_t * bitfield;      /**< Bitfield to keep track of modified fields (bits set to 1) vs. default ones (bits set to 0) */
    void       * caller;        /**< Algorithm instance which has created this probe */
    double       sending_time;  /**< Timestamp set by network layer just after sending the packet (0 if not set) */
    double       queueing_time; /**< Timestamp set by pt_loop just before sending the packet (0 if not set) */
    double       recv_time;     /**< Only set if this instance is related to a reply. Timestamp set by network layer just after sniffing the reply */
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
 * \brief Create a probe_t according to a packet_t instance
 * \return A pointer to a newly allocated probe_t instance if
 *   if successful, NULL otherwise
 */

probe_t * probe_wrap_packet(packet_t * packet);

/**
 * \brief Print probe contents
 * \param probe The probe_t instance to print
 */

void probe_dump(const probe_t * probe);

/**
 * \brief Set a field according to a given field name. The first
 *    matching field belonging to a i-th layer is updated,
 *    such that i > depth. 
 * \param probe The probe from which we're retrieving a field
 * \param depth The index of the first layer from which the field
 *    can be set. A valid depth is between 0 and
 *    probe_get_num_layers(probe) - 1. 
 * \param field The field assigned to the probe.
 * \return true iif successfull  
 */

bool probe_set_field_ext(probe_t * probe, size_t depth, field_t * field);

/**
 * \brief Set a field according to a given field name. The first
 *    matching field is updated.
 * \param probe The probe from which we're retrieving a field
 * \param field The field assigned to the probe.
 * \return true iif successfull  
 */

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
 * \param field1 The first of a list of pointers to a field_t structure
 *    representing a field to add. Each field is freed from the memory.
 * \return true iif successful,
 */

bool probe_set_fields(probe_t * probe, field_t * field1, ...);

/**
 * \brief Assigns a set of fields to a probe
 * \param probe A pointer to a probe_t structure representing the probe
 * \param depth The index of the first layer from which the field
 *    can be set. A valid depth is between 0 and
 *    probe_get_num_layers(probe) - 1. 
 * \param field1 The first of a list of pointers to a field_t structure
 *    representing a field to add. Each field is freed from the memory.
 * \return true iif successful,
 */

//bool probe_set_fields_ext(probe_t * probe, size_t depth, field_t * field1, ...);

/**
 * \brief Iterates through the fields in a probe, passing them as the argument to a callback function
 * \param probe A pointer to a probe_t structure representing the probe
 * \param data A pointer to hold the returned data from the callback (?)
 * \param callback A function pointer to a callback function
 */

void probe_iter_fields(probe_t *probe, void *data, void (*callback)(field_t *field, void *data));

/**
 * \brief Extract a value from a probe
 * \param probe The probe from which we're retrieving a field
 * \param name The name of the queried field
 * \param dst The place where probe_extract_ext writes. Acts as scanf.
 * \return true iif successful
 */

bool probe_extract(const probe_t * probe, const char * name, void * dst);

/**
 * \brief Extract a value from a probe
 * \param probe The probe from which we're retrieving a field
 * \param name The name of the queried field
 * \param depth The index of the first layer from which the field
 *    can be extracted. 0 corresponds to the first layer.
 * \param dst The place where probe_extract_ext writes. Acts as scanf.
 * \return true iif successful
 */

// TODO depth should be 2nd parameter
bool probe_extract_ext(const probe_t * probe, const char * name, size_t depth, void * dst); 

/**
 * \brief Allocate a field based on probe contents according to a given field name. 
 * \param probe The probe from which we're retrieving a field
 * \param name The name of the queried field
 * \return A pointer to the corresponding field, NULL if not found 
 */

field_t * probe_create_field(const probe_t * probe, const char * name);

/**
 * \brief Allocate a field according to a given field name. 
 * \param probe The probe from which we're retrieving a field
 * \param name The name of the queried field
 * \param depth The index of the first layer from which the field
 *    can be extracted. 0 corresponds to the first layer.
 * \return A pointer to the corresponding field, NULL if not found
 */

// TODO depth should be 2nd parameter
field_t * probe_create_field_ext(const probe_t * probe, const char * name, size_t depth);

/**
 * \brief Get the payload from a probe
 * \param probe Pointer to a probe_t structure to get the payload from
 * \return The corresponding pointer, NULL in case of failure 
 */

uint8_t * probe_get_payload(const probe_t * probe);

/**
 * \brief Resize the probe's payload
 * \param payload_size The new size of the payload. If smaller,
 *    previous data is trunked.  If greater the additionnal
 *    bytes are set to 0. 
 * \return true iif successfull
 */

bool probe_payload_resize(probe_t * probe, size_t payload_size);

/**
 * \brief Update the probe payload according to a buffer 
 * \param payload Data copied in the probe's payload 
 * \return true iif successfull
 */

bool probe_set_payload(probe_t * probe, buffer_t * payload);

/**
 * \brief Write data in the probe's payload at a given offset
 * \param probe The update payload
 * \param payload Data copied in the probe's payload 
 * \param offset The offset added to the probe's payload address
 * \return true iif successfull
 */

bool probe_write_payload(probe_t * probe, buffer_t * payload, unsigned int offset);

/**
 * \brief Get the size of the payload from a probe
 * \param probe Pointer to a probe_t structure to get the payload size from
 * \return 0 (NYI - Will be Size of the probe's payload)
 */

size_t probe_get_payload_size(const probe_t * probe);

/**
 * \brief Get the name of protocol involved in the probe 
 * \param i Index of the related layer
 *    (between 0 and probe_get_num_layers(probe) - 1) 
 * \return The name of the corresponding network protocol,
 *    NULL in case of failure 
 */

const char * probe_get_protocol_name(const probe_t * probe, size_t i);

/**
 * \brief Retrieve the number of layers (number of headers + 1 (payload))
 * \param probe The queried probe
 * \return The number of layer stored in this probe.
 */

size_t probe_get_num_layers(const probe_t * probe);

void probe_set_caller(probe_t * probe, void * caller);
void * probe_get_caller(const probe_t * probe);
void probe_set_sending_time(probe_t * probe, double time);
double probe_get_sending_time(const probe_t * probe);
void probe_set_queueing_time(probe_t * probe, double time);
double probe_get_queueing_time(const probe_t * probe);
void probe_set_recv_time(probe_t * probe, double time);
double probe_get_recv_time(const probe_t * probe);

/******************************************************************************
 * probe_reply_t
 ******************************************************************************/

typedef struct {
    probe_t * probe;
    probe_t * reply;
} probe_reply_t;

probe_reply_t * probe_reply_create(void);
void probe_reply_free(probe_reply_t * probe_reply);
void probe_reply_deep_free(probe_reply_t * probe_reply);

// Accessors

void probe_reply_set_probe(probe_reply_t * probe_reply, probe_t * probe);
probe_t * probe_reply_get_probe(const probe_reply_t * probe_reply);
void probe_reply_set_reply(probe_reply_t *probe_reply, probe_t * reply);
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

/**
 * \brief Create a new packet from a probe
 * \param probe Pointer to the probe to use
 * \return New packet_t structure
 */

packet_t * probe_create_packet(probe_t * probe);

#endif
