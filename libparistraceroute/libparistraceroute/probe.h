#ifndef PROBE_H
#define PROBE_H

#include <stdbool.h>
#include "stackedlist.h"
#include "field.h"

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
    stackedlist_t *fields;  /*!< Fields that have not yet been attributed to a protocol */
    //packet_t *packet;       /*!< Packet structure associated to the current probe */
    //bool packet_is_dirty;   /*!< Flag to tell whether the packet content need to be updated */
} probe_t;

/**
 * \brief Create a probe
 * \return A pointer to a probe_t structure containing the probe
 */
probe_t * probe_create(void);

/**
 * \brief Free a probe
 * \param probe A pointer to a probe_t structure containing the probe
 * \return None
 */
void probe_free(probe_t *probe);

/**
 * \brief Add a field to a probe
 * \param probe A pointer to a probe_t structure containing the probe
 * \param field A pointer to a field_t structure containing the field
 * \return None
 */
void probe_add_field(probe_t *probe, field_t *field);

/**
 * \brief Assigns a set of fields to a probe
 * \param probe A pointer to a probe_t structure representing the probe
 * \param arg1 The first of a list of pointers to a field_t structure representing a field to add
 * \return None
 */
void probe_set_fields(probe_t *probe, field_t *arg1, ...);

void probe_iter_fields(probe_t *probe, void *data, void (*callback)(field_t *field, void *data));

field_t ** probe_get_fields(probe_t *probe);
unsigned char *probe_get_payload(probe_t *probe);
unsigned int probe_get_payload_size(probe_t *probe);

char* probe_get_protocol_by_index(unsigned int i);
;
unsigned int probe_get_num_proto(probe_t *probe);
field_t ** probe_get_fields(probe_t *probe);

char* probe_get_protocol_by_index(unsigned int i);

/******************************************************************************
 * pt_loop_t
 ******************************************************************************/
struct pt_loop_s;

void pt_probe_reply_callback(struct pt_loop_s *loop, probe_t *probe, probe_t *reply);
void pt_probe_send(struct pt_loop_s *loop, probe_t *probe);


#endif
